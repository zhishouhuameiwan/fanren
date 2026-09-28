"""《凡人修仙传》原文检索 CLI —— P0 设定校对专用。

用法:
  python novel.py find <关键词> [--limit N] [--window N] [--vol N] [--from N] [--to N]
  python novel.py read <章号> [--start N] [--lines N]
  python novel.py toc [--vol N]
  python novel.py where <章号>

设计约束:
  * find 只返回定位信息与短上下文片段(默认 60 字, 硬上限 200), 用于判定设定,
    不用于摘抄; read 单次最多 120 行, 供定向核对。
  * 全部输出 UTF-8, 不依赖终端代码页。
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

INDEX_PATH = Path(__file__).resolve().parent / "chapters.json"
SNIPPET_MAX = 200
READ_MAX_LINES = 120


def load_index() -> dict:
    if not INDEX_PATH.exists():
        sys.exit("索引不存在，请先运行 novel_index.py")
    return json.loads(INDEX_PATH.read_text(encoding="utf-8"))


def load_lines(index: dict) -> list[str]:
    novel_dir = Path(__file__).resolve().parents[2] / "novel"
    path = novel_dir / index["source"]
    return path.read_bytes().decode(index["encoding"]).split("\n")


def locate(index: dict, lineno: int) -> dict | None:
    lo, hi = 0, len(index["chapters"]) - 1
    found = None
    while lo <= hi:
        mid = (lo + hi) // 2
        chapter = index["chapters"][mid]
        if chapter["line"] <= lineno:
            found, lo = chapter, mid + 1
        else:
            hi = mid - 1
    return found


def cmd_find(args: argparse.Namespace) -> None:
    index = load_index()
    lines = load_lines(index)
    window = min(args.window, SNIPPET_MAX)
    pattern = re.compile(args.pattern) if args.regex else None
    hits, scanned = [], 0
    for lineno, line in enumerate(lines):
        matched = bool(pattern.search(line)) if pattern else (args.pattern in line)
        if not matched:
            continue
        scanned += 1
        chapter = locate(index, lineno)
        if chapter is None:
            continue
        if args.vol and chapter["volume"] != args.vol:
            continue
        if args.start and chapter["chapter"] < args.start:
            continue
        if args.end and chapter["chapter"] > args.end:
            continue
        if len(hits) >= args.limit:
            continue
        column = line.find(args.pattern) if not pattern else pattern.search(line).start()
        head = max(0, column - window // 2)
        hits.append({
            "chapter": chapter["chapter"],
            "title": chapter["title"],
            "volume": chapter["volume"],
            "volume_title": chapter["volume_title"],
            "line": lineno,
            "offset_in_chapter": lineno - chapter["line"],
            "snippet": line[head:head + window].strip(),
        })
    print(json.dumps({"total_matches": scanned, "returned": len(hits), "hits": hits},
                     ensure_ascii=False, indent=1))


def cmd_read(args: argparse.Namespace) -> None:
    index = load_index()
    lines = load_lines(index)
    chapter = next((c for c in index["chapters"] if c["chapter"] == args.chapter), None)
    if chapter is None:
        sys.exit(f"未找到第 {args.chapter} 章")
    begin = chapter["line"] + max(0, args.start)
    count = min(args.lines, READ_MAX_LINES)
    end = min(chapter["end_line"], begin + count)
    body = [ln.strip() for ln in lines[begin:end] if ln.strip()]
    print(json.dumps({
        "chapter": chapter["chapter"], "title": chapter["title"],
        "volume": chapter["volume"], "volume_title": chapter["volume_title"],
        "chapter_lines": chapter["end_line"] - chapter["line"],
        "range": [args.start, args.start + count], "text": body,
    }, ensure_ascii=False, indent=1))


def cmd_toc(args: argparse.Namespace) -> None:
    index = load_index()
    rows = [c for c in index["chapters"] if not args.vol or c["volume"] == args.vol]
    print(json.dumps({
        "volumes": index["volumes"],
        "count": len(rows),
        "chapters": [{"ch": c["chapter"], "t": c["title"], "vol": c["volume"]} for c in rows],
    }, ensure_ascii=False, indent=1))


def cmd_where(args: argparse.Namespace) -> None:
    index = load_index()
    chapter = next((c for c in index["chapters"] if c["chapter"] == args.chapter), None)
    if chapter is None:
        sys.exit(f"未找到第 {args.chapter} 章")
    print(json.dumps(chapter, ensure_ascii=False, indent=1))


def main() -> None:
    parser = argparse.ArgumentParser(description="凡人修仙传原文检索")
    sub = parser.add_subparsers(dest="cmd", required=True)

    find = sub.add_parser("find", help="关键词检索，返回章节定位与短片段")
    find.add_argument("pattern")
    find.add_argument("--limit", type=int, default=12)
    find.add_argument("--window", type=int, default=60)
    find.add_argument("--vol", type=int, default=0)
    find.add_argument("--from", dest="start", type=int, default=0)
    find.add_argument("--to", dest="end", type=int, default=0)
    find.add_argument("--regex", action="store_true")
    find.set_defaults(func=cmd_find)

    read = sub.add_parser("read", help="定向读取某章的一段")
    read.add_argument("chapter", type=int)
    read.add_argument("--start", type=int, default=0)
    read.add_argument("--lines", type=int, default=40)
    read.set_defaults(func=cmd_read)

    toc = sub.add_parser("toc", help="目录")
    toc.add_argument("--vol", type=int, default=0)
    toc.set_defaults(func=cmd_toc)

    where = sub.add_parser("where", help="章节定位信息")
    where.add_argument("chapter", type=int)
    where.set_defaults(func=cmd_where)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    main()
