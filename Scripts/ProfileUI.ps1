# Copyright IG. All Rights Reserved.
# Where the UI's frame time goes, per perf scenario. Runs the perf harness once with an Unreal Insights trace (each
# scenario's sampled frames are a timing region), exports the game thread's timer statistics per scenario with
# UnrealInsights, and prints what each scenario adds over no-ui (Scripts/profile_diff.py). Close the editor first.
#
#   .\Scripts\ProfileUI.ps1                                   # all scenarios, summary of the UI-related timers
#   .\Scripts\ProfileUI.ps1 -Scenarios hud-idle,case-file-505 -All   # every timer, not only the UI-related ones
#
# Output in Saved/Profiling: the .utrace (open it in Unreal Insights for the timeline), stats_<scenario>.csv.
# Tracing costs time itself, so read the numbers relative to no-ui from the same trace, not against G5's.
param([string[]]$Scenarios = @("hud-idle", "hud-animating", "forensic", "gadget-wheel", "case-file-505", "case-file-browse", "settings", "pause-quit", "combat"),
      [switch]$All, [int]$Seconds = 8)

. (Join-Path $PSScriptRoot "Paths.ps1")
$out = Join-Path $ProjectDir "Saved\Profiling"
New-Item -ItemType Directory -Force $out | Out-Null
$trace = Join-Path $out "ProfileUI.utrace"
Remove-Item $trace, (Join-Path $out "stats_*.csv") -ErrorAction SilentlyContinue

$gameArgs = "`"$ProjectFile`" /Game/Maps/L_Arena -game -windowed -ResX=1920 -ResY=1080 -nosplash -unattended " +
    "-MvsPerf=ProfileUI -MvsPerfSeconds=$Seconds -trace=default,slate -tracefile=`"$trace`" -statnamedevents"
$game = Start-Process $UnrealEditor -ArgumentList $gameArgs -PassThru
if (-not $game.WaitForExit(600000)) { $game.Kill() }
# The launcher can exit before the game does: never leave a game running (it holds GPU memory).
Get-Process MVVMSample -ErrorAction SilentlyContinue | Stop-Process -Force
if (-not (Test-Path $trace)) { Write-Error "No trace written to $trace"; exit 1 }

# UnrealInsights reads its commands from a file; paths use forward slashes (it treats backslashes as escapes).
$fwd = $out -replace '\\', '/'
$regions = (@("no-ui", "no-ui-paused") + $Scenarios) -join ","
$rsp = Join-Path $out "export.rsp"
Set-Content -Encoding ascii $rsp "TimingInsights.ExportTimerStatistics `"$fwd/stats_{region}.csv`" -threads=`"GameThread`" -region=`"$regions`""
$insights = Join-Path $EngineDir "Binaries\Win64\UnrealInsights.exe"
$analysis = Start-Process $insights -ArgumentList "-OpenTraceFile=`"$trace`" -NoUI -AutoQuit -log -ExecOnAnalysisCompleteCmd=`"@=$fwd/export.rsp`"" -PassThru
if (-not $analysis.WaitForExit(600000)) { $analysis.Kill(); Write-Error "UnrealInsights did not finish"; exit 1 }

$diffArgs = @((Join-Path $PSScriptRoot "profile_diff.py"), $out, (Join-Path $ProjectDir "Saved\Perf\ProfileUI.md")) + $Scenarios
if ($All) { $diffArgs += "--all" }
& python @diffArgs
