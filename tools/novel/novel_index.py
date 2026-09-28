"""建立《凡人修仙传》原文章节索引，供 P0 校对定向检索。

原文为 GB18030 单文件，不入库。本工具只产出章节结构索引（卷/章/标题/行号），
不落盘任何正文内容。
"""
from __future__ import annotations

import json
import re
from pathlib import Path

NOVEL_DIR = Path(__file__).resolve().parents[2] / "novel"
INDEX_PATH = Path(__file__).resolve().parent / "chapters.json"
ENCODING = "gb18030"

CHAPTER_RE = re.compile(r"^\s*第([0-9一二三四五六七八九十百千零两]+)章\s*(.*)$")
VOLUME_RE = re.compile(r"^\s*第([0-9一二三四五六七八九十百千零两]+)卷\s*(.*)$")

CN_DIGITS = {"零": 0, "一": 1, "二": 2, "两": 2, "三": 3, "四": 4,
             "五": 5, "六": 6, "七": 7, "八": 8, "九": 9}


def cn_to_int(text: str) -> int | None:
    """中文数字转整数，支持到万位；纯阿拉伯数字直接转换。"""
    text = text.strip()
    if text.isdigit():
        return int(text)
    total, section, number = 0, 0, 0
    units = {"十": 10, "百": 100, "千": 1000}
    for char in text:
        if char in CN_DIGITS:
            number = CN_DIGITS[char]
        elif char in units:
            section += (number or 1) * units[char]
            number = 0
        elif char == "万":
            total += (section + number) * 10000
            section = number = 0
        else:
            return None
    result = total + section + number
    return result or None


def novel_path() -> Path:
    candidates = sorted(NOVEL_DIR.glob("*.txt"))
    if not candidates:
        raise FileNotFoundError(f"未在 {NOVEL_DIR} 找到原文 txt")
    return candidates[0]


def load_lines() -> list[str]:
    return novel_path().read_bytes().decode(ENCODING).split("\n")


def build() -> dict:
    lines = load_lines()
    chapters: list[dict] = []
    volume_no, volume_title = 0, ""
    for lineno, line in enumerate(lines):
        vol = VOLUME_RE.match(line)
        if vol and len(line.strip()) < 30:
            parsed = cn_to_int(vol.group(1))
            if parsed:
                volume_no, volume_title = parsed, vol.group(2).strip()
                continue
        chap = CHAPTER_RE.match(line)
        if chap and len(line.strip()) < 40:
            parsed = cn_to_int(chap.group(1))
            if parsed is None:
                continue
            chapters.append({
                "seq": len(chapters) + 1,
                "chapter": parsed,
                "title": chap.group(2).strip(),
                "volume": volume_no,
                "volume_title": volume_title,
                "line": lineno,
            })
    for current, following in zip(chapters, chapters[1:]):
        current["end_line"] = following["line"]
    if chapters:
        chapters[-1]["end_line"] = len(lines)
    index = {
        "source": novel_path().name,
        "encoding": ENCODING,
        "total_lines": len(lines),
        "chapter_count": len(chapters),
        "volumes": sorted({(c["volume"], c["volume_title"]) for c in chapters}),
        "chapters": chapters,
    }
    INDEX_PATH.write_text(json.dumps(index, ensure_ascii=False, indent=1), encoding="utf-8")
    return index


if __name__ == "__main__":
    data = build()
    print(f"chapters={data['chapter_count']} lines={data['total_lines']}")
    print(f"volumes={len(data['volumes'])}")
    print(f"index={INDEX_PATH}")
