# The refactoring gate (Docs/RefactoringPlan.md). Runs every check, prints one PASS / FAIL line per check, writes a
# report to Saved/Verify/<timestamp>.md and exits non-zero if anything failed. Close the editor first.
#
#   .\Scripts\Verify.ps1                         # the full gate, about 25 minutes
#   .\Scripts\Verify.ps1 -Quick                  # incremental build, no perf runs (about 13 minutes)
#   .\Scripts\Verify.ps1 -Only G2,G3             # a subset
#   .\Scripts\Verify.ps1 -Only G5 -PerfRef HEAD  # perf noise check: this build against itself must pass
#   .\Scripts\Verify.ps1 -Only G5 -PerfInject gadget-wheel:0.1   # sensitivity check: must fail on gadget-wheel
#
# G1 build (clean rebuild, zero project warnings)   G2 automation tests       G3 menu input test (Slate input)
# G4 screenshots vs Docs/img (DiffScreens.ps1)      G5 perf vs a reference build, measured side by side
# G6 no ensures / project errors in logs
param(
    [switch]$Quick,
    [string[]]$Only = @(),
    # G5 compares against this commit (normally the last merged pass), built in a worktree under Saved/PerfRef.
    [string]$PerfRef = "master",
    # Alternating reference / current perf runs (ABBA order, so drift during the gate cancels out).
    [int]$PerfRounds = 3,
    # A scenario fails when the median, over rounds, of (current UI cost - reference UI cost) exceeds this.
    [double]$PerfToleranceMs = 0.05,
    # Passed to the current build only as -GothamPerfInject=<scenario>:<ms>, to prove G5 catches a known cost.
    [string]$PerfInject = ""
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

function Run-Game([string]$Flags, [int]$TimeoutSeconds, [string]$Project = $project) {
    $gameArgs = "`"$Project`" /Game/Maps/L_Arena -game -windowed -ResX=1920 -ResY=1080 -nosplash -unattended $Flags"
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
# One harness run of a project (this one or the reference worktree) -> UI cost table, or $null.
function Run-Perf([string]$Label, [string]$ProjectDir, [string]$Extra = "") {
    Run-Game "-GothamPerf=$Label $Extra" 300 (Join-Path $ProjectDir "MVVMSample.uproject") | Out-Null
    $path = Join-Path $ProjectDir "Saved\Perf\$Label.md"
    if (Test-Path $path) { return UI-Cost (Read-Perf $path) } else { return $null }
}

# Checks out $Ref into the reference worktree and builds it (only when the commit changed). Returns an error or "".
function Prepare-PerfRef([string]$Ref, [string]$Dir) {
    $sha = (git -C $root rev-parse --verify "$Ref^{commit}" 2>$null)
    if (-not $sha) { return "unknown reference '$Ref'" }
    if (-not (Test-Path (Join-Path $Dir ".git"))) {
        git -C $root worktree add --detach $Dir $sha *> $null
    } else {
        git -C $Dir checkout -q --detach $sha *> $null
    }
    if ((git -C $Dir rev-parse HEAD) -ne $sha) { return "could not check out $Ref into $Dir" }
    $marker = Join-Path $Dir "Binaries\gotham-built.txt"
    if ((Test-Path $marker) -and ((Get-Content $marker) -eq $sha)) { return "" }
    $log = Join-Path $verifyDir "$stamp-perfref-build.log"
    & "$engineDir\Build\BatchFiles\Build.bat" MVVMSampleEditor Win64 Development "-Project=$Dir\MVVMSample.uproject" -WaitMutex *> $log
    if (-not (Get-Content $log | Select-String -SimpleMatch "Result: Succeeded")) { return "reference build failed (see $log)" }
    Set-Content $marker $sha
    # A freshly built project's first launch does one-off work (asset registry, shader lookups); keep it out of the numbers.
    Run-Perf "Verify_${stamp}_warmup" $Dir | Out-Null
    return ""
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

# G5: perf against a reference build measured in the same session. Comparing against numbers recorded earlier does not
# work: on this machine one build's UI cost moves by more than the tolerance from one hour to the next. Measured side by
# side in ABBA order, both builds share the machine's state and it cancels out. Each run is read as UI cost (scenario
# game thread minus no-ui in the same run), each round gives current minus reference per scenario, and a scenario
# fails when the median over rounds is beyond the tolerance.
if (-not $Quick -and (Should-Run "G5")) {
    $refDir = Join-Path $root "Saved\PerfRef"
    $problem = Prepare-PerfRef $PerfRef $refDir
    if ($problem) {
        Record "G5" $false $problem
    } else {
        $extra = if ($PerfInject) { "-GothamPerfInject=$PerfInject" } else { "" }
        $diffs = @{}; $missing = 0
        for ($r = 1; $r -le $PerfRounds; $r++) {
            $order = if ($r % 2 -eq 1) { @("ref", "cur") } else { @("cur", "ref") }
            $costs = @{}
            foreach ($which in $order) {
                if ($which -eq "ref") { $costs.ref = Run-Perf "Verify_${stamp}_ref$r" $refDir }
                else { $costs.cur = Run-Perf "Verify_${stamp}_cur$r" $root $extra }
            }
            if (-not $costs.ref -or -not $costs.cur) { $missing++; continue }
            foreach ($k in $costs.ref.Keys) {
                if (-not $diffs.ContainsKey($k)) { $diffs[$k] = @() }
                $diffs[$k] += , ($costs.cur[$k] - $costs.ref[$k])
            }
        }
        if ($missing -gt 0) {
            Record "G5" $false ("{0} of {1} rounds produced no perf report" -f $missing, $PerfRounds)
        } else {
            $refSha = (git -C $refDir rev-parse --short HEAD)
            $lines = @("reference: $PerfRef ($refSha), $PerfRounds rounds, tolerance +$PerfToleranceMs ms on the median")
            if ($PerfInject) { $lines += "self-test: current build injects $PerfInject" }
            $bad = 0
            foreach ($k in ($diffs.Keys | Sort-Object)) {
                $sorted = @($diffs[$k] | Sort-Object)
                $median = $sorted[[int][math]::Floor($sorted.Count / 2)]
                $flag = if ($median -gt $PerfToleranceMs) { $bad++; "SLOWER" } else { "ok" }
                $rounds = ($diffs[$k] | ForEach-Object { "{0:+0.000;-0.000}" -f $_ }) -join " "
                $lines += ("{0,-7} {1,-15} median {2:+0.000;-0.000} ms   rounds {3}" -f $flag, $k, $median, $rounds)
            }
            Record "G5" ($bad -eq 0) ("{0} scenarios vs {1}, {2} slower than allowed" -f $diffs.Count, $PerfRef, $bad) ($lines -join "`n")
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
