# The refactoring gate (Docs/RefactoringPlan.md). Runs every check, prints one PASS / FAIL line per check, writes a
# report to Saved/Verify/<timestamp>.md and exits non-zero if anything failed. Close the editor first.
#
#   .\Scripts\Verify.ps1                         # the full gate, about 20 minutes
#   .\Scripts\Verify.ps1 -Quick                  # incremental build, no perf runs (about 13 minutes)
#   .\Scripts\Verify.ps1 -Only G2,G3             # a subset
#   .\Scripts\Verify.ps1 -RecordPerfBaseline     # record the perf baseline G5 compares against (two runs)
#
# G1 build (clean rebuild, zero project warnings)   G2 automation tests       G3 menu input test (Slate input)
# G4 screenshots vs Docs/img (DiffScreens.ps1)      G5 perf vs baseline       G6 no ensures / project errors in logs
param(
    [switch]$Quick,
    [string[]]$Only = @(),
    [switch]$RecordPerfBaseline,
    # A scenario's UI cost (its game-thread time minus no-ui in the same run) may move this much plus the baseline's
    # own run-to-run spread before G5 fails.
    [double]$PerfToleranceMs = 0.05
)

# Continue, not Stop: native tools (the build, python) write progress to stderr, which must not abort the gate.
$ErrorActionPreference = "Continue"
$root = Split-Path -Parent $PSScriptRoot
$engineDir = "D:\UnrealEngine\UE_5.8\Engine"
$editor = "$engineDir\Binaries\Win64\UnrealEditor.exe"
$project = "$root\MVVMSample.uproject"
$verifyDir = Join-Path $root "Saved\Verify"
$perfDir = Join-Path $root "Saved\Perf"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$started = Get-Date
New-Item -ItemType Directory -Force $verifyDir | Out-Null

$results = [ordered]@{}
$details = [ordered]@{}
function Should-Run([string]$Id) { return ($Only.Count -eq 0) -or ($Only -contains $Id) }
function Record([string]$Id, [bool]$Ok, [string]$Summary, [string]$Detail = "") {
    $results[$Id] = @{ Ok = $Ok; Summary = $Summary }
    if ($Detail) { $details[$Id] = $Detail }
    $tag = if ($Ok) { "PASS" } else { "FAIL" }
    Write-Host ("{0}  {1}  {2}" -f $tag, $Id, $Summary)
}

function Run-Game([string]$Flags, [int]$TimeoutSeconds) {
    $gameArgs = "`"$project`" /Game/Maps/L_Arena -game -windowed -ResX=1920 -ResY=1080 -nosplash -unattended $Flags"
    $p = Start-Process $editor -ArgumentList $gameArgs -PassThru
    if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { $p.Kill(); return $false }
    return $true
}

# Perf report -> @{ scenario = game-thread ms }
function Read-Perf([string]$Path) {
    $table = @{}
    foreach ($line in Get-Content $Path) {
        if ($line -match '^\|\s*([a-z0-9\-]+)\s*\|\s*\d+\s*\|\s*[\d.]+\s*\|\s*[\d.]+\s*\|\s*([\d.]+)\s*\|') {
            $table[$Matches[1]] = [double]$Matches[2]
        }
    }
    return $table
}
# UI cost per scenario: its game-thread time minus no-ui from the same run.
function UI-Cost($Table) {
    $cost = @{}
    foreach ($k in $Table.Keys) { if ($k -ne "no-ui") { $cost[$k] = $Table[$k] - $Table["no-ui"] } }
    return $cost
}
function Run-Perf([string]$Label) {
    Run-Game "-GothamPerf=$Label" 300 | Out-Null
    $path = Join-Path $perfDir "$Label.md"
    if (Test-Path $path) { return Read-Perf $path } else { return $null }
}

if ($RecordPerfBaseline) {
    foreach ($s in @("a", "b")) {
        Run-Game "-GothamPerf=Baseline_$s" 300 | Out-Null
        if (-not (Test-Path (Join-Path $perfDir "Baseline_$s.md"))) { Write-Host "FAIL  baseline run $s produced no report"; exit 1 }
    }
    Write-Host "Recorded Saved/Perf/Baseline_a.md and Baseline_b.md"
    exit 0
}

# G1: build. A clean rebuild compiles everything in unity blobs, which is where name clashes show up.
if (Should-Run "G1") {
    $bat = if ($Quick) { "Build.bat" } else { "Rebuild.bat" }
    $log = Join-Path $verifyDir "$stamp-build.log"
    & "$engineDir\Build\BatchFiles\$bat" MVVMSampleEditor Win64 Development "-Project=$project" -WaitMutex *> $log
    $text = Get-Content $log
    $succeeded = [bool]($text | Select-String -SimpleMatch "Result: Succeeded")
    $problems = $text | Select-String -Pattern '\\Source\\MVVMSample\\.*: (warning|error) '
    $ok = $succeeded -and ($problems.Count -eq 0)
    Record "G1" $ok ("{0}; {1} project warnings/errors" -f $bat, $problems.Count) (($problems | ForEach-Object { $_.Line }) -join "`n")
}

# G2: automation tests.
if (Should-Run "G2") {
    $out = & python (Join-Path $root "Scripts\run_tests.py") 2>&1
    $summary = ($out | Select-String -Pattern '\d+ passed, \d+ failed' | Select-Object -Last 1).Line
    Record "G2" ($LASTEXITCODE -eq 0 -and $summary) ("tests: {0}" -f $summary) (($out | Select-String "FAILED") -join "`n")
}

# G3: menu input rules through Slate's input path.
if (Should-Run "G3") {
    Run-Game "-GothamMenuInputTest" 90 | Out-Null
    $gameLog = Join-Path $root "Saved\Logs\MVVMSample.log"
    $line = (Select-String -Path $gameLog -Pattern "Menu input test: (\d+) passed, (\d+) failed" | Select-Object -Last 1)
    $fails = Select-String -Path $gameLog -Pattern "LogGothamMenuTest: Display: FAIL" | ForEach-Object { $_.Line }
    $ok = $line -and ([int]$line.Matches[0].Groups[2].Value -eq 0)
    Record "G3" $ok ($(if ($line) { $line.Matches[0].Value } else { "no result line (test did not finish)" })) ($fails -join "`n")
}

# G4: screenshots against the committed set.
if (Should-Run "G4") {
    $screens = Join-Path $verifyDir "Screens"
    Remove-Item $screens -Recurse -Force -ErrorAction SilentlyContinue
    New-Item -ItemType Directory -Force $screens | Out-Null
    & (Join-Path $root "Scripts\CaptureScreens.ps1") -OutDir $screens | Out-Null
    $diff = & (Join-Path $root "Scripts\DiffScreens.ps1") -Baseline (Join-Path $root "Docs\img") -Current $screens 6>&1 | ForEach-Object { "$_" }
    $ok = ($LASTEXITCODE -eq 0)
    $review = @($diff | Where-Object { $_ -notmatch '^ok ' }).Count
    Record "G4" $ok ("{0} images, {1} need review (diff images in Saved/Verify/Screens)" -f @($diff).Count, $review) ($diff -join "`n")
}

# G5: perf against the recorded baseline, compared as UI cost so machine-wide noise cancels out.
if (-not $Quick -and (Should-Run "G5")) {
    $baseA = Join-Path $perfDir "Baseline_a.md"
    $baseB = Join-Path $perfDir "Baseline_b.md"
    if (-not ((Test-Path $baseA) -and (Test-Path $baseB))) {
        Record "G5" $false "no baseline: run .\Scripts\Verify.ps1 -RecordPerfBaseline on the pass-0 code first"
    } else {
        $bA = UI-Cost (Read-Perf $baseA); $bB = UI-Cost (Read-Perf $baseB)
        $runA = Run-Perf "Verify_${stamp}_a"; $runB = Run-Perf "Verify_${stamp}_b"
        if (-not $runA -or -not $runB) {
            Record "G5" $false "a perf run produced no report"
        } else {
            $cA = UI-Cost $runA; $cB = UI-Cost $runB
            $lines = @(); $bad = 0
            foreach ($k in ($bA.Keys | Sort-Object)) {
                $base = ($bA[$k] + $bB[$k]) / 2
                $now = ($cA[$k] + $cB[$k]) / 2
                $allow = $PerfToleranceMs + [math]::Abs($bA[$k] - $bB[$k])
                $flag = if ($now - $base -gt $allow) { $bad++; "SLOWER" } else { "ok" }
                $lines += ("{0,-7} {1,-15} base {2,6:N3} ms  now {3,6:N3} ms  (allowed +{4:N3})" -f $flag, $k, $base, $now, $allow)
            }
            Record "G5" ($bad -eq 0) ("{0} scenarios, {1} slower than allowed" -f $lines.Count, $bad) ($lines -join "`n")
        }
    }
}

# G6: every game and editor log written during this run.
$g6Pattern = 'Ensure condition failed|LogGotham\w*: (Error|Warning)|LogMaterial: (Error|Warning)|LogSkeletalMesh: (Error|Warning)|missing usage flag|Default Material will be used|LogLinker: (Error|Warning)|LogStreaming: (Error|Warning)|Failed to load (package|asset|object)'
if (Should-Run "G6") {
    $logs = Get-ChildItem (Join-Path $root "Saved\Logs\*.log") | Where-Object { $_.LastWriteTime -ge $started }
    $hits = @()
    foreach ($l in $logs) {
        # Project errors and warnings, ensures, and content-integrity warnings (a material without a usage flag
        # silently renders as the engine default, which slipped through V5 because only LogGotham was scanned).
        $hits += Select-String -Path $l.FullName -Pattern $g6Pattern |
            ForEach-Object { "$($l.Name): $($_.Line)" }
    }
    Record "G6" ($hits.Count -eq 0) ("{0} logs scanned, {1} ensures, project errors/warnings or content warnings" -f $logs.Count, $hits.Count) (($hits | Select-Object -First 40) -join "`n")
}

# Report.
$failed = @($results.Keys | Where-Object { -not $results[$_].Ok })
$md = @("# Verify $stamp", "", "| Check | Result | Summary |", "|---|---|---|")
foreach ($k in $results.Keys) { $md += ("| {0} | {1} | {2} |" -f $k, $(if ($results[$k].Ok) { "PASS" } else { "FAIL" }), $results[$k].Summary) }
foreach ($k in $details.Keys) { $md += @("", "## $k", "", '```', $details[$k], '```') }
$reportPath = Join-Path $verifyDir "$stamp.md"
$md | Set-Content -Encoding utf8 $reportPath
Write-Host ""
Write-Host ("{0}: {1}  (report: {2})" -f $(if ($failed.Count -eq 0) { "GATE PASSED" } else { "GATE FAILED" }), ($failed -join ", "), $reportPath)
if ($failed.Count -eq 0) { exit 0 } else { exit 1 }
