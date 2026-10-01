# Copyright IG. All Rights Reserved.
"""Fails if any game string is untranslated in a shipped culture (Verify.ps1's G7, or run by hand).

For every entry of the English .po, each translated culture must have the same entry with
- a non-empty msgstr, and
- the same msgid: when the English text changes, the engine stops using the old translation and shows English
  until the .po is updated (see Scripts/Localize.bat), so a stale entry is as untranslated as a missing one.

    python Scripts/CheckTranslations.py          # exit code 1 and a list of entries if anything is missing
"""
import re
import sys

from paths import PROJECT_DIR  # engine and project roots (Scripts/Paths.cfg)

LOCALIZATION = PROJECT_DIR / "Content" / "Localization" / "Game"
# en-XA is generated from English by PseudoLocalize.py, so it cannot fall behind.
CULTURES = ["de", "ja"]


def entries(culture):
    """{msgctxt: (msgid, msgstr)} of one culture's Game.po."""
    text = (LOCALIZATION / culture / "Game.po").read_text(encoding="utf-8-sig").replace("\r\n", "\n")
    result = {}
    for block in text.split("\n\n"):
        context = re.search(r'^msgctxt "(.*)"$', block, re.M)
        if not context:
            continue

        def quoted(tag):
            match = re.search(r"^" + tag + r' "(.*)"((?:\n".*")*)', block, re.M)
            if not match:
                return ""
            return match.group(1) + "".join(line[1:-1] for line in match.group(2).split("\n") if line)

        result[context.group(1)] = (quoted("msgid"), quoted("msgstr"))
    return result


def main():
    english = entries("en")
    problems = []
    for culture in CULTURES:
        translated = entries(culture)
        for context, (source, _) in sorted(english.items()):
            if not source:
                continue
            entry = translated.get(context)
            if entry is None:
                problems.append(f"{culture}: missing {context} \"{source}\"")
            elif entry[0] != source:
                problems.append(f"{culture}: stale {context}: the English is now \"{source}\", the translation is for \"{entry[0]}\"")
            elif not entry[1]:
                problems.append(f"{culture}: untranslated {context} \"{source}\"")
    for problem in problems:
        print(problem)
    print(f"{len(english)} strings, {len(CULTURES)} cultures, {len(problems)} problems")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
