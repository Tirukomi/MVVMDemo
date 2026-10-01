# Copyright IG. All Rights Reserved.
# Compares two folders of screenshots (same file names) and reports how much of each image changed.
# Used by Scripts/Verify.ps1 as the screenshot regression check (G4). No downloads: System.Drawing plus a few lines
# of C# compiled on the fly, because a per-pixel loop in PowerShell is far too slow for 1280x720 images.
#
#   .\Scripts\DiffScreens.ps1 -Baseline Docs\img -Current Saved\Verify\Screens [-Threshold 3.0] [-ColorTolerance 48]
#
# Images are compared in 8x8 blocks: a block "changes" when its average colour moves by more than ColorTolerance
# (summed RGB). Rain and idle animation still change a little in every run, so an image only needs review when its
# changed share of blocks is above Threshold percent. Writes <name>.diff.png next to the current image for every
# image over the threshold (changed blocks in red).
# Exit code: 0 if every image is within the threshold, 1 otherwise.
#
# Per-image noise: -RecordNoise diffs two captures of the same code and writes each image's changed share to the
# noise file (Scripts/ScreenNoise.json). Later runs hold deterministic images (menus: no noise) to 0.1% and allow
# animated ones (Forensic Mode, rain) 3x their noise + 0.5 points. Images without an entry use -Threshold.
param(
    [Parameter(Mandatory = $true)][string]$Baseline,
    [Parameter(Mandatory = $true)][string]$Current,
    [double]$Threshold = 3.0,
    [int]$ColorTolerance = 24,
    [string]$NoiseFile = (Join-Path $PSScriptRoot "ScreenNoise.json"),
    [switch]$RecordNoise
)
$noise = @{}
if (-not $RecordNoise -and (Test-Path $NoiseFile)) {
    (Get-Content $NoiseFile -Raw | ConvertFrom-Json).PSObject.Properties | ForEach-Object { $noise[$_.Name] = [double]$_.Value }
}
$measured = [ordered]@{}

Add-Type -AssemblyName System.Drawing
if (-not ("MvsImageDiff" -as [type])) {
    Add-Type -ReferencedAssemblies System.Drawing -TypeDefinition @"
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class MvsImageDiff
{
    // Returns the percentage of pixels whose summed RGB distance exceeds tolerance; writes a diff image if asked.
    public static double Compare(string a, string b, int tolerance, string diffPath)
    {
        using (var A = new Bitmap(a)) using (var B = new Bitmap(b))
        {
            if (A.Width != B.Width || A.Height != B.Height) { return 100.0; }
            var rect = new Rectangle(0, 0, A.Width, A.Height);
            var da = A.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            var db = B.LockBits(rect, ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
            int bytes = Math.Abs(da.Stride) * A.Height;
            var pa = new byte[bytes]; var pb = new byte[bytes];
            Marshal.Copy(da.Scan0, pa, 0, bytes); Marshal.Copy(db.Scan0, pb, 0, bytes);
            A.UnlockBits(da); B.UnlockBits(db);

            // Compare 8x8 block averages, not single pixels: animated scanlines and rain streaks a few pixels apart
            // average out, while a real layout change (text or a panel moving a few pixels) still shifts its blocks.
            const int Block = 8;
            int stride = Math.Abs(da.Stride);
            int bw = (A.Width + Block - 1) / Block, bh = (A.Height + Block - 1) / Block;
            var hitBlock = new bool[bw * bh];
            long changedBlocks = 0;
            for (int by = 0; by < bh; by++)
            for (int bx = 0; bx < bw; bx++)
            {
                long sa0 = 0, sa1 = 0, sa2 = 0, sb0 = 0, sb1 = 0, sb2 = 0; int n = 0;
                for (int y = by * Block; y < Math.Min((by + 1) * Block, A.Height); y++)
                for (int x = bx * Block; x < Math.Min((bx + 1) * Block, A.Width); x++)
                {
                    int i = y * stride + x * 4;
                    sa0 += pa[i]; sa1 += pa[i + 1]; sa2 += pa[i + 2];
                    sb0 += pb[i]; sb1 += pb[i + 1]; sb2 += pb[i + 2];
                    n++;
                }
                long d = (Math.Abs(sa0 - sb0) + Math.Abs(sa1 - sb1) + Math.Abs(sa2 - sb2)) / n;
                if (d > tolerance) { hitBlock[by * bw + bx] = true; changedBlocks++; }
            }
            var diff = new byte[bytes];
            for (int y = 0; y < A.Height; y++)
            for (int x = 0; x < A.Width; x++)
            {
                int i = y * stride + x * 4;
                bool hit = hitBlock[(y / Block) * bw + (x / Block)];
                // BGRA: changed blocks red, the rest a dimmed copy of the current image.
                diff[i] = hit ? (byte)0 : (byte)(pb[i] / 3);
                diff[i + 1] = hit ? (byte)0 : (byte)(pb[i + 1] / 3);
                diff[i + 2] = hit ? (byte)255 : (byte)(pb[i + 2] / 3);
                diff[i + 3] = 255;
            }
            long changed = changedBlocks;
            double percent = 100.0 * changed / (bw * (double)bh);
            if (!string.IsNullOrEmpty(diffPath))
            {
                using (var D = new Bitmap(A.Width, A.Height, PixelFormat.Format32bppArgb))
                {
                    var dd = D.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                    Marshal.Copy(diff, 0, dd.Scan0, bytes);
                    D.UnlockBits(dd);
                    D.Save(diffPath, ImageFormat.Png);
                }
            }
            return percent;
        }
    }
}
"@
}

$failed = 0
$rows = @()
foreach ($base in Get-ChildItem (Join-Path $Baseline "*.png")) {
    $cur = Join-Path $Current $base.Name
    if (-not (Test-Path $cur)) {
        $rows += "MISSING  $($base.Name)"
        $failed++
        continue
    }
    try {
        $pct = [MvsImageDiff]::Compare($base.FullName, (Resolve-Path $cur).Path, $ColorTolerance, "")
    } catch {
        # An unreadable image is a failure, never a silent pass.
        $rows += "ERROR    $($base.Name): $($_.Exception.Message)"
        $failed++
        continue
    }
    $measured[$base.Name] = [math]::Round($pct, 3)
    # Deterministic images (no measured noise) are held to 0.1%; animated ones get 3x their noise plus half a point,
    # because one pair of captures understates noise that depends on when the frame lands (forensic-reveal read
    # 0.7% in one pair and 2.3% in another).
    $allowed = if (-not $noise.ContainsKey($base.Name)) { $Threshold }
               elseif ($noise[$base.Name] -le 0.01) { 0.1 }
               else { 3 * $noise[$base.Name] + 0.5 }
    if (-not $RecordNoise -and $pct -gt $allowed) {
        $curPath = (Resolve-Path $cur).Path
        [void][MvsImageDiff]::Compare($base.FullName, $curPath, $ColorTolerance, ($curPath -replace '\.png$', '.diff.png'))
        $rows += ("REVIEW   {0,-22} {1,6:N2}% changed (allowed {2:N2}%)" -f $base.Name, $pct, $allowed)
        $failed++
    } else {
        $rows += ("ok       {0,-22} {1,6:N2}% changed (allowed {2:N2}%)" -f $base.Name, $pct, $allowed)
    }
}
$rows | ForEach-Object { Write-Host $_ }
if ($RecordNoise) {
    $measured | ConvertTo-Json | Set-Content -Encoding utf8 $NoiseFile
    Write-Host "Recorded noise for $($measured.Count) images to $NoiseFile"
    exit 0
}
if ($failed -gt 0) { exit 1 } else { exit 0 }
