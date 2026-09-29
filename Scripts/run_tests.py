"""Runs the Gotham automation tests headlessly and exits non-zero on any failure (CI friendly).

    python Scripts/run_tests.py [filter]        # default filter: Gotham

Writes the engine's JSON report to Saved/AutomationReports/ and prints a summary. Close the editor first.
"""
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ENGINE = pathlib.Path(r"D:\UnrealEngine\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe")
REPORTS = ROOT / "Saved" / "AutomationReports"


def main():
    test_filter = sys.argv[1] if len(sys.argv) > 1 else "Gotham"
    REPORTS.mkdir(parents=True, exist_ok=True)
    command = [
        str(ENGINE), str(ROOT / "MVVMSample.uproject"),
        f"-ExecCmds=Automation RunTests {test_filter}; Quit",
        f"-ReportExportPath={REPORTS}",
        "-unattended", "-nosplash", "-nullrhi", "-NoSound", "-stdout", "-FullStdOutLogOutput",
    ]
    result = subprocess.run(command, capture_output=True, text=True, errors="replace")

    passed = failed = 0
    failures = []
    for line in result.stdout.splitlines():
        if "Test Completed" not in line:
            continue
        name = line.split("Path={")[-1].rstrip("}").strip() if "Path={" in line else line
        if "Result={Success}" in line:
            passed += 1
        else:
            failed += 1
            failures.append(name)

    report = REPORTS / "index.json"
    if report.exists():
        try:
            data = json.loads(report.read_text(encoding="utf-8-sig"))
            print(f"report: {report} (succeeded={data.get('succeeded')}, failed={data.get('failed')})")
        except (OSError, ValueError):
            pass

    print(f"{passed} passed, {failed} failed")
    for name in failures:
        print("  FAILED:", name)
    if passed == 0 and failed == 0:
        print("no tests ran; is the editor still open, or did the build fail?")
        return 2
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
