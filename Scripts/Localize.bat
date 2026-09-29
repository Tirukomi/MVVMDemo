@echo off
rem Gather -> translate -> compile the game's localization. Close the editor first.
set UE="D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT="%~dp0..\MVVMSample.uproject"
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Gather.ini" -SCCProvider=None -unattended -nosplash -nullrhi
python "%~dp0TranslateLocalization.py"
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Compile.ini" -SCCProvider=None -unattended -nosplash -nullrhi
