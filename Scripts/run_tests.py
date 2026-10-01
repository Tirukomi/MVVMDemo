"""Runs the Gotham automation tests and exits non-zero on any failure (CI friendly). Close the editor first.

    python Scripts/run_tests.py [filter]        # default filter: Gotham (both passes)

Two passes:
- editor: every test with the editor context, headless (-nullrhi). Pure logic, view models, widgets.
- game:   Gotham.Functional.*, which need the running game (a world, a player, painted widgets for hit-testing), in a
          small window. Skipped when the filter names something else.
Writes the engine's JSON reports to Saved/AutomationReports/ (game pass: Saved/AutomationReports/Functional/).
"""
import json
import pathlib
import subprocess
import sys

from paths import BINARIES, PROJECT_DIR as ROOT  # engine and project roots (Scripts/Paths.cfg)
REPORTS = ROOT / "Saved" / "AutomationReports"
FUNCTIONAL = "Gotham.Functional"


def read_report(folder):
    """(passed, failed, [failure lines]) from the engine's index.json, or None if there is none."""
    report = folder / "index.json"
    if not report.exists():
        return None
    try:
        data = json.loads(report.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError):
        return None
    passed = failed = 0
    failures = []
    for test in data.get("tests", []):
        if test.get("state") == "Success":
            passed += 1
            continue
        failed += 1
        failures.append(test.get("fullTestPath", "?"))
        for entry in test.get("entries", []):
            event = entry.get("event", {})
            if event.get("type") == "Error":
                failures.append("    " + event.get("message", ""))
    return passed, failed, failures


def editor_pass(test_filter):
    folder = REPORTS
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "index.json").unlink(missing_ok=True)
    subprocess.run([
        str(BINARIES / "UnrealEditor-Cmd.exe"), str(ROOT / "MVVMSample.uproject"),
        f"-ExecCmds=Automation RunTests {test_filter}; Quit",
        f"-ReportExportPath={folder}",
        "-unattended", "-nosplash", "-nullrhi", "-NoSound", "-stdout", "-FullStdOutLogOutput",
    ], capture_output=True, text=True, errors="replace")
    return read_report(folder)


def game_pass(test_filter):
    folder = REPORTS / "Functional"
    folder.mkdir(parents=True, exist_ok=True)
    (folder / "index.json").unlink(missing_ok=True)
    try:
        subprocess.run([
            str(BINARIES / "UnrealEditor.exe"), str(ROOT / "MVVMSample.uproject"), "/Game/Maps/L_Arena",
            "-game", "-windowed", "-ResX=1280", "-ResY=720", "-nosplash", "-unattended", "-NoSound",
            f"-ExecCmds=Automation RunTests {test_filter}",
            "-TestExit=Automation Test Queue Empty",
            f"-ReportExportPath={folder}",
        ], timeout=300)
    except subprocess.TimeoutExpired:
        print("game pass: timed out")
    return read_report(folder)


def main():
    test_filter = sys.argv[1] if len(sys.argv) > 1 else "Gotham"
    passes = []
    if not test_filter.startswith(FUNCTIONAL):
        passes.append(("editor", editor_pass(test_filter)))
    if FUNCTIONAL.startswith(test_filter) or test_filter.startswith(FUNCTIONAL):
        passes.append(("game", game_pass(test_filter if test_filter.startswith(FUNCTIONAL) else FUNCTIONAL)))

    passed = failed = 0
    failures = []
    missing = []
    for name, result in passes:
        if result is None:
            missing.append(name)
            continue
        p, f, lines = result
        print(f"{name} pass: {p} passed, {f} failed")
        passed += p
        failed += f
        failures += lines

    print(f"{passed} passed, {failed} failed")
    for line in failures:
        print("  FAILED:", line) if not line.startswith("    ") else print("  " + line)
    if missing:
        print("no report from the", " and ".join(missing), "pass; is the editor still open, or did the build fail?")
        return 2
    if passed == 0 and failed == 0:
        print("no tests ran")
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
