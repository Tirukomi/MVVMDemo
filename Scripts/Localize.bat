@echo off
rem Localization round trip. Close the editor first.
rem   1. gather text from source and content (manifest + archives)
rem   2. import the translations from Content/Localization/Game/<culture>/Game.po (edit those, not the archives)
rem   3. export the .po files again, so new strings show up in them for translators
rem   4. regenerate the en-XA pseudo-locale .po, and import it
rem   5. compile the .locres files
rem A .po whose only change is the export's date stamps is put back as it was (StablePoDates.py).
rem An entry whose English source changed keeps its old translation in the .po, where the engine no longer matches
rem it: it shows in English until the .po is updated, never with a stale translation.
call "%~dp0Paths.bat"
set UE="%UE_CMD%"
set PROJECT="%PROJECT%"
set ARGS=-SCCProvider=None -unattended -nosplash -nullrhi
python "%~dp0StablePoDates.py" save
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Gather.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ImportPO.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ExportPO.ini" %ARGS%
python "%~dp0PseudoLocalize.py"
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_ImportPO.ini" %ARGS%
%UE% %PROJECT% -run=GatherText -config="Config/Localization/Game_Compile.ini" %ARGS%
python "%~dp0StablePoDates.py" restore
