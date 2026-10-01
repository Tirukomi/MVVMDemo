@echo off
rem Copyright IG. All Rights Reserved.
call "%~dp0Paths.bat"
"%UE_CMD%" "%PROJECT%" -ExecutePythonScript="%~dp0CreateForensicAssets.py" -unattended -nosplash -nullrhi
