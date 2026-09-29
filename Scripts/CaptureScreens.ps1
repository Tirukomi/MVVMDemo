# Captures the documentation screenshots into Docs/img using the game's own dev flags (no OS-level input or
# screen grabs, so it never touches other windows). Close the editor first. ~30 s per image.
param([string]$Only = "")

$root = Split-Path -Parent $PSScriptRoot
$engine = "D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
$out = Join-Path $root "Docs\img"
New-Item -ItemType Directory -Force $out | Out-Null

$shots = @(
    @{ name = "hud";              flags = "-GothamHudDemo";                                shot = "gotham_hud";       res = "1280 720" },
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
    @{ name = "settings-ja";      flags = "-GothamOpenSettings -GothamLanguage=ja";        shot = "gotham_settings";  res = "1280 720" },
    @{ name = "settings-pseudo";  flags = "-GothamOpenSettings -GothamLanguage=en-XA";     shot = "gotham_settings";  res = "1280 720" },
    @{ name = "controls";         flags = "-GothamOpenControls";                           shot = "gotham_controls";  res = "1280 720" },
    @{ name = "colour-blind";     flags = "-GothamDetective -GothamColorMode=2";           shot = "gotham_detective"; res = "1280 720" },
    @{ name = "ui-scale-150-de";  flags = "-GothamOpenSettings -GothamUIScale=4 -GothamLanguage=de"; shot = "gotham_settings"; res = "1280 720" },
    @{ name = "ultrawide";        flags = "-GothamDetective";                              shot = "gotham_detective"; res = "2560 1080" }
)

foreach ($s in $shots) {
    if ($Only -and $s.name -ne $Only) { continue }
    $w, $h = $s.res -split " "
    Remove-Item (Join-Path $root "Saved\Screenshots") -Recurse -Force -ErrorAction SilentlyContinue
    $arguments = "`"$root\MVVMSample.uproject`" /Game/Maps/L_Arena -game -windowed -ResX=$w -ResY=$h -nosplash -unattended $($s.flags) -GothamShotDelay=8"
    $p = Start-Process $engine -ArgumentList $arguments -PassThru
    Start-Sleep -Seconds 30
    if (-not $p.HasExited) { $p.Kill() }
    Start-Sleep -Seconds 2
    $src = Join-Path $root "Saved\Screenshots\WindowsEditor\$($s.shot).png"
    if (Test-Path $src) { Copy-Item $src (Join-Path $out "$($s.name).png") -Force; Write-Host "captured $($s.name)" }
    else { Write-Host "MISSING $($s.name)" }
}
