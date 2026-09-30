"""Fills the en-XA pseudo-locale's .po file: every string accented, about 40% longer and bracketed, for layout testing.

Run by Scripts/Localize.bat between the .po export and the second import. The other cultures' .po files are edited by
hand (or by a translator) and are never generated.
"""
import re
import sys

PO = r"D:\UEProjects\MVVMSample\Content\Localization\Game\en-XA\Game.po"

ACCENTS = str.maketrans("aeiouAEIOUcnysCNYS", "áéíóúÅÉÍÓÚçñýšÇÑÝŠ")
ESCAPES = {"\\\\": "\\", '\\"': '"', "\\n": "\n", "\\t": "\t", "\\r": "\r"}


def pseudo(text):
    """Accented, ~40% longer, bracketed. {n} placeholders pass through untouched."""
    parts = re.split(r"(\{\d+\})", text)
    body = "".join(p if re.fullmatch(r"\{\d+\}", p) else p.translate(ACCENTS) for p in parts)
    padding = "~" * max(2, int(len(re.sub(r"\{\d+\}", "", text)) * 0.4))
    return f"[{body} {padding}]"


def unescape(s):
    return re.sub(r'\\[\\"ntr]', lambda m: ESCAPES[m.group(0)], s)


def escape(s):
    return s.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n").replace("\t", "\\t").replace("\r", "\\r")


def quoted(lines, start):
    """The string that starts on lines[start] ('msgid "..."' or 'msgstr "..."') plus "..." continuation lines."""
    first = lines[start]
    value = first[first.index('"') + 1:first.rindex('"')]
    end = start + 1
    while end < len(lines) and lines[end].startswith('"'):
        value += lines[end][1:lines[end].rindex('"')]
        end += 1
    return unescape(value), end


def main():
    text = open(PO, encoding="utf-8-sig", newline="").read()
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.split(newline)
    out = []
    i = 0
    filled = 0
    msgid = None
    while i < len(lines):
        line = lines[i]
        if line.startswith("msgid "):
            msgid, end = quoted(lines, i)
            out.extend(lines[i:end])
            i = end
            continue
        if line.startswith("msgstr ") and msgid:
            _, end = quoted(lines, i)
            out.append(f'msgstr "{escape(pseudo(msgid))}"')
            filled += 1
            msgid = None
            i = end
            continue
        if line.startswith("msgstr "):
            msgid = None  # the header entry (empty msgid) stays as it is
        out.append(line)
        i += 1
    open(PO, "w", encoding="utf-8-sig", newline="").write(newline.join(out))
    print(f"en-XA: {filled} strings pseudo-localized")


if __name__ == "__main__":
    main()
    sys.exit(0)
