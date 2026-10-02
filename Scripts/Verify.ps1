# Copyright IG. All Rights Reserved.
# The refactoring gate (its origin: Docs/History/RefactoringPlan.md). Runs every check, prints one PASS / FAIL line per check, writes a
# report to Saved/Verify/<timestamp>.md and exits non-zero if anything failed. Close the editor first.
#
#   .\Scripts\Verify.ps1                         # the full gate, about 30 minutes: 6 without G5, 2 without G4 either
#
# G4 and G5 run only when something that can change their result differs from -PerfRef (normally master): G5 for the
# game's code, config, content and project file; G4 for those plus the baselines and capture scripts. Test-only code
# (Source/MVVMSampleTests, the menu-input rules) counts for neither. The gate prints its decision first;
# -ForceScreens / -ForcePerf run them anyway.
#   .\Scripts\Verify.ps1 -Quick                  # incremental build, no perf runs (about 6 minutes)
#   .\Scripts\Verify.ps1 -Only G2,G4             # a subset
#   .\Scripts\Verify.ps1 -Only G5 -PerfRef HEAD -ForcePerf  # perf noise check: this build against itself must pass
#   .\Scripts\Verify.ps1 -Only G5 -PerfRef HEAD -PerfInject gadget-wheel:0.1   # sensitivity: must fail on gadget-wheel
#
# G1 build (clean rebuild, zero project warnings)   G2 automation tests (editor pass + game pass, which includes the
#                                                   menu input rules that were G3)
# G4 screenshots vs Docs/img (DiffScreens.ps1)      G5 perf vs a reference build, measured side by side
# G6 no ensures / project errors in logs           G7 every string translated (CheckTranslations.py)
param(
    [switch]$Quick,
    [string[]]$Only = @(),
    # G5 compares against this commit (normally the last merged pass), built in a worktree under Saved/PerfRef.
    [string]$PerfRef = "master",
    # Alternating reference / current perf runs (ABBA order, so drift during the gate cancels out). Even, so every
    # round that runs the reference first has a partner that runs the current build first.
    [int]$PerfRounds = 6,
    # A scenario fails when the median, over round pairs, of (current UI cost - reference UI cost) exceeds this. Each
    # pair averages a reference-first and a current-first round: whichever build runs second in a round measured up to
    # 0.2 ms slower (second review S1), which decided the median of single rounds on its own.
    [double]$PerfToleranceMs = 0.05,
    # Passed to the current build only as -MvsPerfInject=<scenario>:<ms>, to prove G5 catches a known cost.
    [string]$PerfInject = "",
    # Sampled seconds per perf scenario. 5 was tried and measured noisier (an injected 0.1 ms read as +0.054), so 8.
    [int]$PerfSampleSeconds = 8,
    # Measure perf even when nothing the game runs differs from the reference.
    [switch]$ForcePerf,
    # Capture screenshots even when nothing that can change a pixel differs from the reference.
    [switch]$ForceScreens
)

# Continue, not Stop: native tools (the build, python) write progress to stderr, which must not abort the gate.
$ErrorActionPreference = "Continue"
. (Join-Path $PSScriptRoot "Paths.ps1")   # engine and project roots (Scripts/Paths.cfg)
$root = $ProjectDir
$engineDir = $EngineDir
$editor = $UnrealEditor
$project = "$root\MVVMSample.uproject"
$verifyDir = Join-Path $root "Saved\Verify"
$perfDir = Join-Path $root "Saved\Perf"
$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$started = Get-Date
New-Item -ItemType Directory -Force $verifyDir | Out-Null

$results = [ordered]@{}
$details = [ordered]@{}
function Should-Run([string]$Id) { return ($Only.Count -eq 0) -or ($Only -contains $Id) }

# What differs from the reference ($PerfRef, normally master): committed on this branch, uncommitted, or untracked.
# G4 and G5 are skipped when none of it can change what they measure (second review S1: a gate is ~30 minutes, of
# which G5 is ~24 and G4 ~4; the build, tests, log scan and translations run every time).
$referenceSha = (git -C $root rev-parse --verify "$PerfRef^{commit}" 2>$null)
$changedFiles = @()
if ($referenceSha) {
    $changedFiles = @(git -C $root diff --name-only $referenceSha) + @(git -C $root ls-files --others --exclude-standard) |
        Where-Object { $_ } | Sort-Object -Unique
}
# Code that runs only in tests: the test module (the menu-input rules moved into it in second review S2).
$testOnly = '^Source/MVVMSampleTests/'
# What the game runs: its code, config, content and project file.
$runtimePattern = '^(Source/|Config/|Content/|MVVMSample\.uproject$)'
# What a screenshot depends on: the same, plus the baselines and the capture and diff scripts.
$screensPattern = '^(Source/|Config/|Content/|MVVMSample\.uproject$|Docs/img/|Scripts/(CaptureScreens|DiffScreens)\.ps1$|Scripts/ScreenNoise\.json$)'
function Get-Relevant([string]$Pattern) {
    return @($changedFiles | Where-Object { $_ -match $Pattern -and $_ -notmatch $testOnly })
}
function Skip-Note([string[]]$Relevant, [string]$What) {
    if (-not $referenceSha) { return "" }
    if ($Relevant.Count -gt 0) { return "" }
    return "skipped: nothing $What differs from $PerfRef ($($changedFiles.Count) changed files, none relevant)"
}
# Said up front, so a skipped check is never a surprise.
foreach ($check in @(@("G4", $screensPattern, [bool]$ForceScreens), @("G5", $runtimePattern, ($ForcePerf -or [bool]$PerfInject)))) {
    $relevant = Get-Relevant $check[1]
    $decision = if (-not $referenceSha -or $check[2] -or $relevant.Count -gt 0) { "runs" } else { "skipped" }
    $examples = if ($relevant.Count) { " (" + (($relevant | Select-Object -First 3) -join ", ") + $(if ($relevant.Count -gt 3) { ", ..." }) + ")" } else { "" }
    Write-Host ("{0} {1}: {2} of {3} changed files are relevant{4}" -f $check[0], $decision, $relevant.Count, $changedFiles.Count, $examples)
}
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
# Scenarios the world was paused in (the "Paused" column, from review S0 on; older reports have none).
function Read-Paused([string]$Path) {
    $paused = @{}
    foreach ($line in Get-Content $Path) {
        if ($line -match '^\|\s*([a-z0-9\-]+)\s*\|.*\|\s*(yes|no)\s*\|\s*(yes|no)\s*\|\s*$') { $paused[$Matches[1]] = ($Matches[3] -eq "yes") }
    }
    return $paused
}
# UI cost per scenario: its game-thread time minus the matching reference from the same run (no-ui-paused for a
# scenario that ran paused, when the report has it; no-ui otherwise).
function UI-Cost($Table, $Paused = @{}) {
    $cost = @{}
    foreach ($k in $Table.Keys) {
        if ($k -like "no-ui*") { continue }
        $reference = if ($Paused[$k] -and $Table.ContainsKey("no-ui-paused")) { "no-ui-paused" } else { "no-ui" }
        $cost[$k] = $Table[$k] - $Table[$reference]
    }
    return $cost
}
# A reference from before R8 (review 36, 37) takes the old flag prefix and calls the forensic scenario "detective".
function Get-FlagPrefix([string]$ProjectDir) {
    if (Test-Path (Join-Path $ProjectDir "Source\MVVMSample\Core\GothamPerfHarness.cpp")) { return "Gotham" } else { return "Mvs" }
}
$scenarioRenames = @{ "detective" = "forensic" }
# One harness run of a project (one of the two perf worktrees) -> UI cost table, or $null.
function Run-Perf([string]$Label, [string]$ProjectDir, [string]$Extra = "") {
    $prefix = Get-FlagPrefix $ProjectDir
    Run-Game "-${prefix}Perf=$Label -${prefix}PerfSeconds=$PerfSampleSeconds $Extra" 300 (Join-Path $ProjectDir "MVVMSample.uproject") | Out-Null
    $path = Join-Path $ProjectDir "Saved\Perf\$Label.md"
    if (-not (Test-Path $path)) { return $null }
    # Scenarios the harness found not in the state they measure (the "Valid" column, from review S0 on).
    foreach ($line in Get-Content $path) {
        if ($line -match '^\|\s*([a-z0-9\-]+)\s*\|.*\|\s*(yes|no)\s*\|\s*(yes|no)\s*\|\s*$' -and $Matches[2] -eq "no") { $script:invalidScenarios += "$Label/$($Matches[1])" }
    }
    $cost = @{}
    foreach ($entry in (UI-Cost (Read-Perf $path) (Read-Paused $path)).GetEnumerator()) {
        $name = if ($scenarioRenames.ContainsKey($entry.Key)) { $scenarioRenames[$entry.Key] } else { $entry.Key }
        $cost[$name] = $entry.Value
    }
    return $cost
}

# Prepares one perf worktree: checks out $Sha, optionally mirrors this checkout's Source and Config over it (so the
# current side includes uncommitted edits), and builds it the same way for both sides. Returns an error or "".
function Prepare-PerfTree([string]$Sha, [string]$Dir, [bool]$MirrorWorkingTree, [string]$Name) {
    if (-not (Test-Path (Join-Path $Dir ".git"))) {
        git -C $root worktree add --detach $Dir $Sha *> $null
    } else {
        git -C $Dir checkout -q --force --detach $Sha *> $null
    }
    if ((git -C $Dir rev-parse HEAD) -ne $Sha) { return "could not check out $Sha into $Dir" }
    if ($MirrorWorkingTree) {
        foreach ($sub in @("Source", "Config")) {
            robocopy (Join-Path $root $sub) (Join-Path $Dir $sub) /MIR /NFL /NDL /NJH /NJS /NP *> $null
        }
        # robocopy keeps the source timestamps, which can be older than this worktree's last build: touch every file
        # that differs from HEAD so the build always sees it as changed.
        foreach ($line in (git -C $Dir status --porcelain --untracked-files=all)) {
            $file = Join-Path $Dir ($line.Substring(3).Trim('"'))
            if (Test-Path $file -PathType Leaf) { (Get-Item $file).LastWriteTime = Get-Date }
        }
    }
    $log = Join-Path $verifyDir "$stamp-perf$Name-build.log"
    & "$engineDir\Build\BatchFiles\Build.bat" MVVMSampleEditor Win64 Development "-Project=$Dir\MVVMSample.uproject" -WaitMutex *> $log
    $text = Get-Content $log
    if (-not ($text | Select-String -SimpleMatch "Result: Succeeded")) { return "$Name build failed (see $log)" }
    # A freshly built project's first launch does one-off work (asset registry, shader lookups); keep it out of the numbers.
    if (-not ($text | Select-String -SimpleMatch "Target is up to date")) { Run-Game "-MvsQuitAfterLoad" 60 (Join-Path $Dir "MVVMSample.uproject") | Out-Null }
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

# G4: screenshots against the committed set.
$screensNote = if ($ForceScreens) { "" } else { Skip-Note (Get-Relevant $screensPattern) "that can change a pixel" }
if ((Should-Run "G4") -and $screensNote) {
    Record "G4" $true "$screensNote (-ForceScreens captures anyway)"
} elseif (Should-Run "G4") {
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
# side in ABBA order, both builds share the machine's state and it cancels out. Both sides run from sibling worktrees
# (Saved/PerfRef, Saved/PerfCur) built the same way: running the current side from this checkout instead showed a
# steady +0.05 ms on HUD scenarios whichever code it held, so the location and build had to be identical too. Each run is read as UI cost (scenario
# game thread minus no-ui in the same run), each round gives current minus reference per scenario, and a scenario
# fails when the median over rounds is beyond the tolerance.
if (-not $Quick -and (Should-Run "G5")) {
    $refDir = Join-Path $root "Saved\PerfRef"
    $curDir = Join-Path $root "Saved\PerfCur"
    $refSha = $referenceSha
    $perfNote = if ($ForcePerf -or $PerfInject) { "" } else { Skip-Note (Get-Relevant $runtimePattern) "the game runs" }
    if ($perfNote) {
        Record "G5" $true "$perfNote (-ForcePerf measures anyway)"
    } else {
    $problem = if (-not $refSha) { "unknown reference '$PerfRef'" } else { Prepare-PerfTree $refSha $refDir $false "ref" }
    if (-not $problem) { $problem = Prepare-PerfTree (git -C $root rev-parse HEAD) $curDir $true "cur" }
    if ($problem) {
        Record "G5" $false $problem
    } else {
        $extra = if ($PerfInject) { "-MvsPerfInject=$PerfInject" } else { "" }
        $diffs = @{}; $current = @{}; $missing = 0
        $script:invalidScenarios = @()
        # Absolute budgets (UI cost of the current build, median over rounds): relative checks alone let small
        # regressions add up across merges.
        $budgets = @{}
        $budgetFile = Join-Path $root "Scripts\PerfBudgets.json"
        if (Test-Path $budgetFile) {
            (Get-Content $budgetFile -Raw | ConvertFrom-Json).PSObject.Properties | Where-Object { $_.Name -notlike "_*" } |
                ForEach-Object { $budgets[$_.Name] = [double]$_.Value }
        }
        for ($r = 1; $r -le $PerfRounds; $r++) {
            $order = if ($r % 2 -eq 1) { @("ref", "cur") } else { @("cur", "ref") }
            $costs = @{}
            foreach ($which in $order) {
                if ($which -eq "ref") { $costs.ref = Run-Perf "Verify_${stamp}_ref$r" $refDir }
                else { $costs.cur = Run-Perf "Verify_${stamp}_cur$r" $curDir $extra }
            }
            if (-not $costs.ref -or -not $costs.cur) { $missing++; continue }
            foreach ($k in $costs.ref.Keys) {
                if (-not $diffs.ContainsKey($k)) { $diffs[$k] = @() }
                $diffs[$k] += , ($costs.cur[$k] - $costs.ref[$k])
            }
            foreach ($k in $costs.cur.Keys) {
                if (-not $current.ContainsKey($k)) { $current[$k] = @() }
                $current[$k] += , $costs.cur[$k]
            }
        }
        $invalidCurrent = @($script:invalidScenarios | Where-Object { $_ -match '_cur\d+/' })
        if ($missing -gt 0) {
            Record "G5" $false ("{0} of {1} rounds produced no perf report" -f $missing, $PerfRounds)
        } elseif ($invalidCurrent.Count -gt 0) {
            Record "G5" $false ("{0} scenario runs were not in the state they measure" -f $invalidCurrent.Count) ($invalidCurrent -join "`n")
        } else {
            $refSha = (git -C $refDir rev-parse --short HEAD)
            $lines = @("reference: $PerfRef ($refSha), $PerfRounds rounds, tolerance +$PerfToleranceMs ms on the median of round pairs")
            if ($PerfInject) { $lines += "self-test: current build injects $PerfInject" }
            $bad = 0
            foreach ($k in ($diffs.Keys | Sort-Object)) {
                $pairs = @()
                for ($i = 0; $i + 1 -lt $diffs[$k].Count; $i += 2) { $pairs += , (($diffs[$k][$i] + $diffs[$k][$i + 1]) / 2) }
                if ($pairs.Count -eq 0) { $pairs = @($diffs[$k]) }
                $sorted = @($pairs | Sort-Object)
                $median = $sorted[[int][math]::Floor($sorted.Count / 2)]
                $flag = if ($median -gt $PerfToleranceMs) { $bad++; "SLOWER" } else { "ok" }
                $rounds = ($diffs[$k] | ForEach-Object { "{0:+0.000;-0.000}" -f $_ }) -join " "
                $lines += ("{0,-7} {1,-15} median {2:+0.000;-0.000} ms   rounds {3}" -f $flag, $k, $median, $rounds)
            }
            $over = 0
            $lines += "budgets (Scripts/PerfBudgets.json): UI cost of the current build, median over rounds"
            foreach ($k in ($current.Keys | Sort-Object)) {
                $sorted = @($current[$k] | Sort-Object)
                $median = $sorted[[int][math]::Floor($sorted.Count / 2)]
                if (-not $budgets.ContainsKey($k)) { $over++; $lines += ("NO BUDGET {0,-15} median {1:+0.000;-0.000} ms" -f $k, $median); continue }
                $flag = if ($median -gt $budgets[$k]) { $over++; "OVER" } else { "ok" }
                $lines += ("{0,-7} {1,-15} median {2:+0.000;-0.000} ms   budget {3:0.000} ms" -f $flag, $k, $median, $budgets[$k])
            }
            Record "G5" ($bad -eq 0 -and $over -eq 0) ("{0} scenarios vs {1}, {2} slower than allowed, {3} over budget" -f $diffs.Count, $PerfRef, $bad, $over) ($lines -join "`n")
        }
    }
    }
}

# G6: every game and editor log written during this run.
$g6Pattern = 'Ensure condition failed|LogMvs\w*: (Error|Warning)|LogMaterial: (Error|Warning)|LogSkeletalMesh: (Error|Warning)|missing usage flag|Default Material will be used|LogLinker: (Error|Warning)|LogStreaming: (Error|Warning)|Failed to load (package|asset|object)'
if (Should-Run "G6") {
    $logs = Get-ChildItem (Join-Path $root "Saved\Logs\*.log") | Where-Object { $_.LastWriteTime -ge $started }
    $hits = @()
    foreach ($l in $logs) {
        # Project errors and warnings, ensures, and content-integrity warnings (a material without a usage flag
        # silently renders as the engine default, which slipped through V5 because only LogMvs was scanned).
        $hits += Select-String -Path $l.FullName -Pattern $g6Pattern |
            ForEach-Object { "$($l.Name): $($_.Line)" }
    }
    Record "G6" ($hits.Count -eq 0) ("{0} logs scanned, {1} ensures, project errors/warnings or content warnings" -f $logs.Count, $hits.Count) (($hits | Select-Object -First 40) -join "`n")
}

# G7: every English string has a current translation in each shipped culture.
if (Should-Run "G7") {
    $out = & python (Join-Path $root "Scripts\CheckTranslations.py") 2>&1 | ForEach-Object { "$_" }
    Record "G7" ($LASTEXITCODE -eq 0) (($out | Select-Object -Last 1)) (($out | Select-Object -SkipLast 1 | Select-Object -First 40) -join "`n")
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
