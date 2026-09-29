@echo off
"D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\MVVMSample.uproject" -ExecutePythonScript="%~dp0CreateDetectiveAssets.py" -unattended -nosplash -nullrhi
