@echo off
rem Sets ENGINE_DIR, PROJECT_DIR, PROJECT (the .uproject), UE_CMD (UnrealEditor-Cmd.exe) and UE_EDITOR from
rem Scripts\Paths.cfg, then Scripts\Paths.local.cfg if present. Use with: call "%~dp0Paths.bat"
set "ENGINE_DIR=D:\UnrealEngine\UE_5.8\Engine"
set "PROJECT_DIR="
for %%F in ("%~dp0Paths.cfg" "%~dp0Paths.local.cfg") do (
    if exist %%F (
        for /f "usebackq eol=; tokens=1,* delims==" %%A in (%%F) do (
            if /i "%%A"=="EngineDir" if not "%%B"=="" set "ENGINE_DIR=%%B"
            if /i "%%A"=="ProjectDir" if not "%%B"=="" set "PROJECT_DIR=%%B"
        )
    )
)
if "%PROJECT_DIR%"=="" for %%P in ("%~dp0..") do set "PROJECT_DIR=%%~fP"
set "PROJECT=%PROJECT_DIR%\MVVMSample.uproject"
set "UE_CMD=%ENGINE_DIR%\Binaries\Win64\UnrealEditor-Cmd.exe"
set "UE_EDITOR=%ENGINE_DIR%\Binaries\Win64\UnrealEditor.exe"
