@echo off
rem Localization round trip. Close the editor first.
rem   1. gather text from source and content (manifest + archives)
rem   2. import the translations from Content/Localization/Game/<culture>/Game.po (edit those, not the archives)
rem   3. export the .po files again, so new strings show up in them for translators
rem   4. regenerate the en-XA pseudo-locale .po, and import it
rem   5. compile the .locres files
rem An entry whose English source changed keeps its old translation in the .po, where the engine no longer matches
rem it: it shows in English until the .po is updated, never with a stale translation.
set UE="D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set PROJECT="%~dp0..\MVVMSample.uproject"
set ARGS=-SCCProvider=None -unattended -nosplash -nullrhi
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Gather.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ImportPO.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ExportPO.ini" %ARGS%
python "%~dp0PseudoLocalize.py"
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ImportPO.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Compile.ini" %ARGS%
