#!/usr/bin/env python3
"""构建目录盘点与回收。

多个 agent 并行在同一棵源码树上干活，共用一个 build/ 会互相抢 ninja 的锁、
踩对方的 .obj 与 .exe（技术债 G-1）。build_slot.bat 因此按会话 ID 给每个
agent 分一个 build-<slot>/。代价是目录会堆积，这里负责盘点和回收：

    python tools/build_gc.py                  # 只列出，什么都不删
    python tools/build_gc.py --delete --days 3  # 删掉 3 天没动过的

默认是 dry run：不加 --delete 绝不删任何东西。两类目录永远不删，即使显式
要求——build/（人类开发者的默认构建目录，不属于任何会话）和当前会话自己的
目录（正在用）。

"最后活动时间"取目录树里最新的 mtime，而不是目录自身的 mtime：Windows 上
目录 mtime 只反映直接子项的增删，子目录深处刚写完的 .obj 不会让它变新。
"""

from __future__ import annotations

import argparse
import os
import pathlib
import shutil
import sys
import time

ROOT = pathlib.Path(__file__).resolve().parent.parent

# 人类开发者的默认构建目录，不归任何会话所有，不参与回收。
PROTECTED = {"build"}


def current_slot() -> str | None:
    """当前会话的 slot，与 build_slot.bat 的第 3 条规则保持一致。"""
    session = os.environ.get("CLAUDE_CODE_SESSION_ID")
    return session[:8] if session else None


def scan(directory: pathlib.Path) -> tuple[int, float]:
    """返回 (字节数, 最新 mtime)。目录为空时 mtime 退回目录自身。"""
    total = 0
    newest = directory.stat().st_mtime
    for base, _, files in os.walk(directory):
        for name in files:
            try:
                info = os.stat(os.path.join(base, name))
            except OSError:
                # 别的 agent 可能正在这个目录里构建，文件随时会消失。
                # 漏算一个文件不影响"这个目录该不该回收"的判断。
                continue
            total += info.st_size
            newest = max(newest, info.st_mtime)
    return total, newest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--delete", action="store_true", help="真的删除，默认只列出")
    parser.add_argument("--days", type=float, default=3.0,
                        help="只回收闲置超过这么多天的目录（默认 3）")
    args = parser.parse_args()

    mine = current_slot()
    now = time.time()
    cutoff = args.days * 86400

    candidates = sorted(p for p in ROOT.glob("build*") if p.is_dir())
    if not candidates:
        print("没有构建目录。")
        return 0

    reclaimed = 0
    print(f"{'目录':<24} {'大小':>9}  {'闲置':>9}  说明")
    print("-" * 68)
    for path in candidates:
        size, newest = scan(path)
        idle = now - newest
        size_mb = size / 1024 / 1024
        idle_txt = f"{idle / 3600:.1f}h" if idle < 86400 else f"{idle / 86400:.1f}d"

        if path.name in PROTECTED:
            note = "保护：人类默认构建目录"
        elif mine and path.name == f"build-{mine}":
            note = "保护：当前会话正在用"
        elif idle < cutoff:
            note = f"保留：闲置不足 {args.days} 天"
        elif args.delete:
            note = "删除中…"
        else:
            note = "可回收（加 --delete 才真删）"

        print(f"{path.name:<24} {size_mb:>8.0f}M  {idle_txt:>9}  {note}")

        if note == "删除中…":
            shutil.rmtree(path, ignore_errors=True)
            stale_log = ROOT / f"{path.name}.log"
            if stale_log.exists():
                stale_log.unlink()
            reclaimed += size

    if reclaimed:
        print(f"\n已回收 {reclaimed / 1024 / 1024:.0f}M。")
    elif not args.delete:
        print("\n这是 dry run，什么都没删。加 --delete 才会真删。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
