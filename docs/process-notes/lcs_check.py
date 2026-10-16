"""Longest common Han substring between chapter text JSON and the novel.

usage:
  python -I lcs_check.py --from 416 --to 499 <text.json> [<text.json> ...]

For every string value in the given JSON files, prints the longest run of
Han characters (U+4E00-U+9FFF, punctuation ignored) that also occurs in
  (a) the novel chapters [from, to]  -> limit 6
  (b) the whole novel                -> limit 7
and flags any run of 10 or more. Exit code 1 when any limit is exceeded.

Self-check first: a 12-character run copied from chapter <from> must be
reported as 12 against both corpora, otherwise the tool refuses to judge.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

NOVEL_INDEX = Path("H:/Work/Kys/tools/novel/chapters.json")
NOVEL_DIR = Path("H:/Work/Kys/novel")
HAN = re.compile(r"[\u4e00-\u9fff]")
RANGE_LIMIT = 6
WHOLE_LIMIT = 7
HARD_LIMIT = 10


def han_only(text: str) -> str:
    return "".join(HAN.findall(text))


def load_corpora(first: int, last: int) -> tuple[str, str]:
    index = json.loads(NOVEL_INDEX.read_text(encoding="utf-8"))
    lines = (NOVEL_DIR / index["source"]).read_bytes().decode(index["encoding"]).split("\n")
    whole = han_only("\n".join(lines))
    parts = [
        "\n".join(lines[c["line"]:c["end_line"]])
        for c in index["chapters"]
        if first <= c["chapter"] <= last
    ]
    if not parts:
        sys.exit(f"no chapters in [{first}, {last}]")
    return han_only("\n".join(parts)), whole


def longest_shared(text: str, corpus: str) -> str:
    h = han_only(text)
    best = ""
    for i in range(len(h)):
        length = len(best) + 1
        while i + length <= len(h) and h[i:i + length] in corpus:
            best = h[i:i + length]
            length += 1
    return best


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--from", dest="first", type=int, required=True)
    parser.add_argument("--to", dest="last", type=int, required=True)
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()

    in_range, whole = load_corpora(args.first, args.last)
    probe = in_range[1000:1012]
    if len(longest_shared(probe, in_range)) != 12 or len(longest_shared(probe, whole)) != 12:
        print("SELF_CHECK_FAILED: a copied 12-char probe was not reported as 12")
        return 2
    if longest_shared("甲乙丙丁戊己庚辛壬癸子丑寅卯", whole) and len(longest_shared("甲乙丙丁戊己庚辛壬癸子丑寅卯", whole)) > 4:
        print("SELF_CHECK_FAILED: negative probe matched too long")
        return 2
    print("SELF_CHECK_OK probe=12")

    bad = 0
    total = 0
    for name in args.files:
        data = json.loads(Path(name).read_text(encoding="utf-8"))
        for key, value in data.items():
            if not isinstance(value, str):
                continue
            total += 1
            r = longest_shared(value, in_range)
            w = longest_shared(value, whole)
            if len(r) > RANGE_LIMIT or len(w) > WHOLE_LIMIT or len(w) >= HARD_LIMIT:
                bad += 1
                print(f"OVER {key}\trange={len(r)}:{r}\twhole={len(w)}:{w}")
    print(f"checked={total} over={bad} (limits: range<={RANGE_LIMIT}, whole<={WHOLE_LIMIT}, none>={HARD_LIMIT})")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
