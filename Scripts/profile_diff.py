# Copyright IG. All Rights Reserved.
"""What each perf scenario adds to the game thread over no-ui, timer by timer (Scripts/ProfileUI.ps1 runs it).

    python Scripts/profile_diff.py <stats folder> <perf report .md> <scenario>... [--all]

Reads stats_<scenario>.csv (UnrealInsights' ExportTimerStatistics, one per timing region) and the harness report
(for each scenario's frame count, and whether it ran paused), and prints the timers whose exclusive time per frame grew the most.
By default only UI-related timers are listed (Slate, widgets, text, timers), because the rest of the difference
is the world: menus pause it, and particle and animation work varies between scenarios.
"""
import csv
import pathlib
import re
import sys

UI = re.compile(r"slate|widget|paint|invalidat|active timer|umg|text|font|prepass|hittest|tableview|listview|tick widgets", re.I)
TOP = 12


def frames(report):
    """{scenario: (sampled frames, ran paused)} from the harness report."""
    counts = {}
    for line in pathlib.Path(report).read_text(encoding="utf-8").splitlines():
        match = re.match(r"^\|\s*([a-z0-9\-]+)\s*\|\s*(\d+)\s*\|", line)
        if match:
            paused = re.search(r"\|\s*(yes|no)\s*\|\s*(yes|no)\s*\|\s*$", line)
            counts[match.group(1)] = (int(match.group(2)), bool(paused) and paused.group(2) == "yes")
    return counts


def load(folder, scenario, frame_count):
    """{timer: (inclusive ms per frame, exclusive ms per frame, calls per frame)}."""
    table = {}
    with open(pathlib.Path(folder) / f"stats_{scenario}.csv", encoding="utf-8") as file:
        for row in csv.DictReader(file):
            table[row["Name"]] = (float(row["Incl"]) * 1000 / frame_count, float(row["Excl"]) * 1000 / frame_count,
                                  int(row["Count"]) / frame_count)
    return table


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    show_all = "--all" in sys.argv
    folder, report, scenarios = args[0], args[1], args[2:]
    counts = frames(report)
    references = {name: load(folder, name, counts[name][0]) for name in ("no-ui", "no-ui-paused") if name in counts}
    for scenario in scenarios:
        current = load(folder, scenario, counts[scenario][0])
        # A scenario that paused the world is compared with the paused reference (see the harness).
        reference = "no-ui-paused" if counts[scenario][1] and "no-ui-paused" in references else "no-ui"
        base = references[reference]
        rows = []
        for name in set(current) | set(base):
            if not show_all and not UI.search(name):
                continue
            now, then = current.get(name, (0, 0, 0)), base.get(name, (0, 0, 0))
            rows.append((now[1] - then[1], now[0] - then[0], now[2] - then[2], name))
        total = sum(row[0] for row in rows)
        print(f"== {scenario}: {'all' if show_all else 'UI'} timers add {total:+.3f} ms/frame (exclusive) over {reference}")
        for exclusive, inclusive, calls, name in sorted(rows, reverse=True)[:TOP]:
            print(f"  {exclusive:+.4f} excl  {inclusive:+.4f} incl  {calls:+7.1f} calls/frame  {name[:80]}")


if __name__ == "__main__":
    main()
