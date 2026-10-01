"""Engine and project roots from Scripts/Paths.cfg, then Scripts/Paths.local.cfg if present.

    from paths import ENGINE_DIR, PROJECT_DIR, PROJECT, EDITOR, EDITOR_CMD
"""
import pathlib
import re

_SCRIPTS = pathlib.Path(__file__).resolve().parent
_values = {"EngineDir": r"D:\UnrealEngine\UE_5.8\Engine", "ProjectDir": ""}
for _name in ("Paths.cfg", "Paths.local.cfg"):
    _file = _SCRIPTS / _name
    if _file.exists():
        for _line in _file.read_text(encoding="utf-8").splitlines():
            _match = re.match(r"\s*(EngineDir|ProjectDir)\s*=\s*(.*?)\s*$", _line)
            if _match and _match.group(2):
                _values[_match.group(1)] = _match.group(2)

ENGINE_DIR = pathlib.Path(_values["EngineDir"])
PROJECT_DIR = pathlib.Path(_values["ProjectDir"]) if _values["ProjectDir"] else _SCRIPTS.parent
PROJECT = PROJECT_DIR / "MVVMSample.uproject"
BINARIES = ENGINE_DIR / "Binaries" / "Win64"
EDITOR = BINARIES / "UnrealEditor.exe"
EDITOR_CMD = BINARIES / "UnrealEditor-Cmd.exe"
