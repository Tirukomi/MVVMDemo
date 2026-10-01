@echo off
call "%~dp0Paths.bat"
"%UE_CMD%" "%PROJECT%" -ExecutePythonScript="%~dp0CreateDetectiveAssets.py" -unattended -nosplash -nullrhi
