# Copyright IG. All Rights Reserved.
# Engine and project roots from Scripts/Paths.cfg, then Scripts/Paths.local.cfg if present. Dot-source it:
#   . (Join-Path $PSScriptRoot "Paths.ps1")
# and use $EngineDir, $ProjectDir, $ProjectFile, $UnrealEditor, $UnrealEditorCmd.
$EngineDir = "D:\UnrealEngine\UE_5.8\Engine"
$ProjectDir = ""
foreach ($file in @("Paths.cfg", "Paths.local.cfg")) {
    $path = Join-Path $PSScriptRoot $file
    if (-not (Test-Path $path)) { continue }
    foreach ($line in Get-Content $path) {
        if ($line -match '^\s*(EngineDir|ProjectDir)\s*=\s*(.*?)\s*$' -and $Matches[2]) {
            Set-Variable -Name $Matches[1] -Value $Matches[2]
        }
    }
}
if (-not $ProjectDir) { $ProjectDir = Split-Path -Parent $PSScriptRoot }
$ProjectFile = Join-Path $ProjectDir "MVVMSample.uproject"
$UnrealEditor = Join-Path $EngineDir "Binaries\Win64\UnrealEditor.exe"
$UnrealEditorCmd = Join-Path $EngineDir "Binaries\Win64\UnrealEditor-Cmd.exe"
