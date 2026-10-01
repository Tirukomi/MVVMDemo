# Copyright IG. All Rights Reserved.
# Captures the documentation screenshots into Docs/img using the game's own dev flags (no OS-level input or
# screen grabs, so it never touches other windows). Close the editor first. ~20 s per image, -Parallel of them at once.
# -MvsIgnoreHover: the window opens under wherever the real cursor rests, which must not highlight or focus
# the menu item there.
# -OutDir writes somewhere else (Scripts/Verify.ps1 captures into Saved/Verify/Screens and diffs against Docs/img).
param([string]$Only = "", [string]$OutDir = "", [int]$Parallel = 3)

. (Join-Path $PSScriptRoot "Paths.ps1")
$root = $ProjectDir
$engine = $UnrealEditor
$out = if ($OutDir) { $OutDir } else { Join-Path $root "Docs\img" }
New-Item -ItemType Directory -Force $out | Out-Null

$shots = @(
    @{ name = "hud";              flags = "-MvsHudDemo";                                shot = "mvs_hud";       res = "1280 720" },
    @{ name = "combat";           flags = "-MvsCombatDemo";                             shot = "mvs_hud";       res = "1280 720" },
    @{ name = "combat-access";    flags = "-MvsCombatDemo -MvsLanguage=de -MvsUIScale=4 -MvsColorMode=2 -MvsHighContrast"; shot = "mvs_hud"; res = "1280 720" },
    @{ name = "hud-wheel";        flags = "-MvsOpenWheel";                              shot = "mvs_wheel";     res = "1280 720" },
    @{ name = "forensic";        flags = "-MvsForensic";                              shot = "mvs_forensic"; res = "1280 720" },
    @{ name = "forensic-reveal"; flags = "-MvsForensic -MvsForensicReveal";      shot = "mvs_forensic"; res = "1280 720" },
    @{ name = "forensic-analyse"; flags = "-MvsForensic -MvsForensicAnalyse";    shot = "mvs_forensic"; res = "1280 720" },
    @{ name = "case-file";        flags = "-MvsClueLog=200";                            shot = "mvs_cluelog";   res = "1280 720" },
    @{ name = "pause";            flags = "-MvsOpenPause";                              shot = "mvs_pause";     res = "1280 720" },
    @{ name = "pause-quit";       flags = "-MvsOpenQuit";                               shot = "mvs_quit";      res = "1280 720" },
    @{ name = "settings-en";      flags = "-MvsOpenSettings";                           shot = "mvs_settings";  res = "1280 720" },
    @{ name = "settings-access";  flags = "-MvsOpenSettings -MvsSettingsTab=Accessibility"; shot = "mvs_settings"; res = "1280 720" },
    @{ name = "settings-de";      flags = "-MvsOpenSettings -MvsLanguage=de";        shot = "mvs_settings";  res = "1280 720" },
    # German capitals: the detail header of "Untertitelgröße" must read UNTERTITELGRÖSSE (ß becomes SS).
    @{ name = "settings-de-caps"; flags = "-MvsOpenSettings -MvsLanguage=de -MvsSettingsTab=Display -MvsSettingsItem=1"; shot = "mvs_settings"; res = "1280 720" },
    @{ name = "settings-ja";      flags = "-MvsOpenSettings -MvsLanguage=ja";        shot = "mvs_settings";  res = "1280 720" },
    @{ name = "settings-pseudo";  flags = "-MvsOpenSettings -MvsLanguage=en-XA";     shot = "mvs_settings";  res = "1280 720" },
    @{ name = "controls";         flags = "-MvsOpenControls";                           shot = "mvs_controls";  res = "1280 720" },
    @{ name = "colour-blind";     flags = "-MvsForensic -MvsColorMode=2";           shot = "mvs_forensic"; res = "1280 720" },
    @{ name = "ui-scale-150-de";  flags = "-MvsOpenSettings -MvsUIScale=4 -MvsLanguage=de"; shot = "mvs_settings"; res = "1280 720" },
    @{ name = "ultrawide";        flags = "-MvsForensic";                              shot = "mvs_forensic"; res = "2560 1080" }
)

# Menu screens pause the world, so they are static: up to $Parallel of them run at once, each writing its own file
# (-MvsShotName). Shots of the live world are timing-sensitive (damage flash, combo decay, forensic pulses) and
# games slow each other down, so those run one at a time, after the menus.
$solo = @("hud", "combat", "combat-access", "hud-wheel", "forensic", "forensic-reveal", "forensic-analyse", "colour-blind", "ultrawide")
$shotDir = Join-Path $root "Saved\Screenshots\WindowsEditor"
Remove-Item (Join-Path $root "Saved\Screenshots") -Recurse -Force -ErrorAction SilentlyContinue
$selected = @($shots | Where-Object { -not $Only -or $_.name -eq $Only })
$queue = [System.Collections.Queue]::new(@($selected | Where-Object { $solo -notcontains $_.name }) + @($selected | Where-Object { $solo -contains $_.name }))
$running = @()
while ($queue.Count -gt 0 -or $running.Count -gt 0) {
    while ($queue.Count -gt 0 -and $running.Count -lt [math]::Max(1, $Parallel)) {
        $isSolo = $solo -contains $queue.Peek().name
        if (($isSolo -and $running.Count -gt 0) -or ($running | Where-Object { $solo -contains $_.Shot.name })) { break }
        $s = $queue.Dequeue()
        $w, $h = $s.res -split " "
        $file = "cap_$($s.name)"
        $arguments = "`"$root\MVVMSample.uproject`" /Game/Maps/L_Arena -game -windowed -ResX=$w -ResY=$h -nosplash -unattended $($s.flags) -MvsShotDelay=8 -MvsIgnoreHover -MvsShotName=$file"
        $running += [pscustomobject]@{ Shot = $s; Process = (Start-Process $engine -ArgumentList $arguments -PassThru); Src = (Join-Path $shotDir "$file.png"); Deadline = (Get-Date).AddSeconds(60) }
    }
    Start-Sleep -Milliseconds 500
    # Stop each game as soon as its screenshot is on disk (and fully written) rather than always waiting the worst case.
    $still = @()
    foreach ($r in $running) {
        $done = Test-Path $r.Src
        if (-not $done -and (Get-Date) -lt $r.Deadline) { $still += $r; continue }
        Start-Sleep -Seconds 1
        if (-not $r.Process.HasExited) { $r.Process.Kill() }
        if ($done) { Copy-Item $r.Src (Join-Path $out "$($r.Shot.name).png") -Force; Write-Host "captured $($r.Shot.name)" }
        else { Write-Host "MISSING $($r.Shot.name)" }
    }
    $running = $still
}
Start-Sleep -Seconds 2
