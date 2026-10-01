@echo off
rem Copyright IG. All Rights Reserved.
rem Rebuilds L_Arena (night rooftop) and re-places the clues. Close the editor first.
call "%~dp0Paths.bat"
"%UE_CMD%" "%PROJECT%" -ExecutePythonScript="%~dp0CreateArenaMap.py" -unattended -nosplash -nullrhi
"%UE_CMD%" "%PROJECT%" -ExecutePythonScript="%~dp0CreateForensicAssets.py" -unattended -nosplash -nullrhi
