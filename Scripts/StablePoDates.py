# Copyright IG. All Rights Reserved.
"""Keeps the .po files byte-identical when a Localize.bat run changes nothing but their header dates.

The .po export stamps POT-Creation-Date and PO-Revision-Date with the current time, so every run would otherwise
change all four .po files. Run by Scripts/Localize.bat:

    python StablePoDates.py save      (before the export: remembers each .po as it is)
    python StablePoDates.py restore   (at the end: puts back any .po whose only change is its dates)
"""
import pathlib
import re
import sys

from paths import PROJECT_DIR  # engine and project roots (Scripts/Paths.cfg)

ROOT = PROJECT_DIR / "Content" / "Localization" / "Game"
SAVED = PROJECT_DIR / "Saved" / "Localization" / "PoBeforeExport"
DATES = re.compile(rb'^"(POT-Creation-Date|PO-Revision-Date): [^"]*"\r?$', re.MULTILINE)


def without_dates(data):
    return DATES.sub(b"", data)


def main(mode):
    for po in sorted(ROOT.glob("*/Game.po")):
        saved = SAVED / po.parent.name / "Game.po"
        if mode == "save":
            saved.parent.mkdir(parents=True, exist_ok=True)
            saved.write_bytes(po.read_bytes())
        elif saved.exists() and without_dates(saved.read_bytes()) == without_dates(po.read_bytes()):
            if saved.read_bytes() != po.read_bytes():
                po.write_bytes(saved.read_bytes())
                print(f"{po.parent.name}: only the dates changed, kept the previous file")


if __name__ == "__main__":
    if len(sys.argv) != 2 or sys.argv[1] not in ("save", "restore"):
        sys.exit("usage: StablePoDates.py save|restore")
    main(sys.argv[1])
