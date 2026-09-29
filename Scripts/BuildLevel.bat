@echo off
rem Rebuilds L_Arena (night rooftop) and re-places the clues. Close the editor first.
set UE="D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT="%~dp0..\MVVMSample.uproject"
%UE% %PROJECT% -ExecutePythonScript="%~dp0CreateArenaMap.py" -unattended -nosplash -nullrhi
%UE% %PROJECT% -ExecutePythonScript="%~dp0CreateDetectiveAssets.py" -unattended -nosplash -nullrhi
