# Captures the documentation screenshots into Docs/img using the game's own dev flags (no OS-level input or
# screen grabs, so it never touches other windows). Close the editor first. ~20 s per image, -Parallel of them at once.
# -GothamIgnoreHover: the window opens under wherever the real cursor rests, which must not highlight or focus
# the menu item there.
# -OutDir writes somewhere else (Scripts/Verify.ps1 captures into Saved/Verify/Screens and diffs against Docs/img).
param([string]$Only = "", [string]$OutDir = "", [int]$Parallel = 3)

$root = Split-Path -Parent $PSScriptRoot
$engine = "D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$out = if ($OutDir) { $OutDir } else { Join-Path $root "Docs\img" }
New-Item -ItemType Directory -Force $out | Out-Null

$shots = @(
    @{ name = "hud";              flags = "-GothamHudDemo";                                shot = "gotham_hud";       res = "1280 720" },
    @{ name = "combat";           flags = "-GothamCombatDemo";                             shot = "gotham_hud";       res = "1280 720" },
    @{ name = "combat-access";    flags = "-GothamCombatDemo -GothamLanguage=de -GothamUIScale=4 -GothamColorMode=2 -GothamHighContrast"; shot = "gotham_hud"; res = "1280 720" },
    @{ name = "hud-wheel";        flags = "-GothamOpenWheel";                              shot = "gotham_wheel";     res = "1280 720" },
    @{ name = "detective";        flags = "-GothamDetective";                              shot = "gotham_detective"; res = "1280 720" },
    @{ name = "detective-reveal"; flags = "-GothamDetective -GothamDetectiveReveal";      shot = "gotham_detective"; res = "1280 720" },
    @{ name = "detective-analyse"; flags = "-GothamDetective -GothamDetectiveAnalyse";    shot = "gotham_detective"; res = "1280 720" },
    @{ name = "case-file";        flags = "-GothamClueLog=200";                            shot = "gotham_cluelog";   res = "1280 720" },
    @{ name = "pause";            flags = "-GothamOpenPause";                              shot = "gotham_pause";     res = "1280 720" },
    @{ name = "pause-quit";       flags = "-GothamOpenQuit";                               shot = "gotham_quit";      res = "1280 720" },
    @{ name = "settings-en";      flags = "-GothamOpenSettings";                           shot = "gotham_settings";  res = "1280 720" },
    @{ name = "settings-access";  flags = "-GothamOpenSettings -GothamSettingsTab=Accessibility"; shot = "gotham_settings"; res = "1280 720" },
    @{ name = "settings-de";      flags = "-GothamOpenSettings -GothamLanguage=de";        shot = "gotham_settings";  res = "1280 720" },
    # German capitals: the detail header of "Untertitelgröße" must read UNTERTITELGRÖSSE (ß becomes SS).
    @{ name = "settings-de-caps"; flags = "-GothamOpenSettings -GothamLanguage=de -GothamSettingsTab=Display -GothamSettingsItem=1"; shot = "gotham_settings"; res = "1280 720" },
    @{ name = "settings-ja";      flags = "-GothamOpenSettings -GothamLanguage=ja";        shot = "gotham_settings";  res = "1280 720" },
    @{ name = "settings-pseudo";  flags = "-GothamOpenSettings -GothamLanguage=en-XA";     shot = "gotham_settings";  res = "1280 720" },
    @{ name = "controls";         flags = "-GothamOpenControls";                           shot = "gotham_controls";  res = "1280 720" },
    @{ name = "colour-blind";     flags = "-GothamDetective -GothamColorMode=2";           shot = "gotham_detective"; res = "1280 720" },
    @{ name = "ui-scale-150-de";  flags = "-GothamOpenSettings -GothamUIScale=4 -GothamLanguage=de"; shot = "gotham_settings"; res = "1280 720" },
    @{ name = "ultrawide";        flags = "-GothamDetective";                              shot = "gotham_detective"; res = "2560 1080" }
)

# Menu screens pause the world, so they are static: up to $Parallel of them run at once, each writing its own file
# (-GothamShotName). Shots of the live world are timing-sensitive (damage flash, combo decay, detective pulses) and
# games slow each other down, so those run one at a time, after the menus.
$solo = @("hud", "combat", "combat-access", "hud-wheel", "detective", "detective-reveal", "detective-analyse", "colour-blind", "ultrawide")
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
        $arguments = "`"$root\MVVMSample.uproject`" /Game/Maps/L_Arena -game -windowed -ResX=$w -ResY=$h -nosplash -unattended $($s.flags) -GothamShotDelay=8 -GothamIgnoreHover -GothamShotName=$file"
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
