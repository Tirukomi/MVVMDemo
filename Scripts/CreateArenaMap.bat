@echo off
"D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%~dp0..\MVVMSample.uproject" -run=pythonscript -script="%~dp0CreateArenaMap.py" -unattended -nosplash
