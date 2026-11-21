#!/usr/bin/env python3
"""第 7 章对白量具（地基包路 D，docs/ch07-foundation.md 第 4 节）。

量两个数，逐脚本列表：

1. **主线对白字数**：口径同 docs/ch07-design.md 第 14 节——46 个主线脚本
   （3.1 表里 1–36 号的 44 个挂点 ＋ Z2 两处）里出现的全部文案 key（含各分支与选项），
   去重后只数汉字（U+4E00–U+9FFF），标点与空白不算。
2. **两次可操作之间连续 talk 的最大句数**：静态走一遍脚本的控制流，
   `choice` / `battle` / `shop` 算「可操作」，把连续计数清零；脚本开头与结尾也算
   （进脚本前是走图，脚本结束后是走图）。if 的各个分支都走，取最坏的那条路；
   循环按「零次、一次、两次」展开；`return` 结束这条路。

用法：

    python tools/dialogue_stats.py            # 打表
    python tools/dialogue_stats.py --json     # 机器可读
    python tools/dialogue_stats.py --chapter ch07 --markdown

只读，不改任何文件。不是门禁（用户定的规矩：门禁不跑），是施工用的量具；
单测 tests/DialogueBudgetTests.cpp 用同一套规则把上限钉住。
"""
from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
CJK = re.compile(r"[一-鿿]")

# docs/ch07-design.md 3.1 表：44 个主线挂点 ＋ Z2 两处。其余 10 个（插话 4、历练 2、
# 后来补的破鼎、火灵石、问明、见闻）不在第 14 节的口径里，单独列出。
MAIN_SCRIPTS_CH07 = [
    "chaibao", "yaolou", "yuelu_ru", "cangshi", "dihuo_wen", "shimen", "murong",
    "dufang", "chuangong", "yuanjiao", "chuling", "fangshi_ru", "wanbaolou",
    "fangshi_chu", "yeyu", "fanhui", "baoming", "ma_songyao", "jihe", "xiang",
    "liedui", "pojin", "wulongtan", "liangshi", "yixiantian", "shulin", "tongmen",
    "yueyang", "kuitan", "charen", "youdi", "sichu", "xiaoshidian", "qingshidian",
    "guanzhan", "jiaoshi", "xiashan", "chukou", "xieshili", "sannian", "chouhan",
    "shijiu", "banian", "zhuji", "yujian_dongpo", "yujian_hantan",
    # 伙伴改版样板（2026-11-18）把节点 31 拆出来的四段，都在目标链上（各置一个 done_flag）。
    "fengkou", "huolingshi", "poding", "wenming",
]

# 要玩家动手的命令：选择、战斗、商店。
OPERABLE = {"choice", "battle", "shop"}
# bubble 不压场景、不收输入：不算一句 talk，也不清零（人照走，下一句照接）。

TOKEN = re.compile(
    r"""
    (?P<ws>\s+)
  | (?P<lcomment>--\[(?P<leq>=*)\[.*?\](?P=leq)\])
  | (?P<comment>--[^\n]*)
  | (?P<lstr>\[(?P<seq>=*)\[.*?\](?P=seq)\])
  | (?P<str>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<num>0[xX][0-9a-fA-F]+|\d+(?:\.\d*)?(?:[eE][-+]?\d+)?)
  | (?P<name>[A-Za-z_][A-Za-z0-9_]*)
  | (?P<op>\.\.\.|\.\.|==|~=|<=|>=|::|[-+*/%^#&~|<>=(){}\[\];:,.])
    """,
    re.VERBOSE | re.DOTALL,
)


def tokenize(source: str) -> list[tuple[str, str]]:
    out = []
    pos = 0
    while pos < len(source):
        m = TOKEN.match(source, pos)
        if not m:
            raise ValueError(f"无法切词：{source[pos:pos + 20]!r}")
        pos = m.end()
        kind = m.lastgroup
        if kind in ("ws", "comment", "lcomment"):
            continue
        text = m.group(0)
        if kind == "str":
            out.append(("str", text[1:-1]))
        elif kind == "lstr":
            out.append(("str", re.sub(r"^\[=*\[|\]=*\]$", "", text)))
        elif kind in ("leq", "seq"):
            continue
        else:
            out.append((kind, text))
    return out


CAP = 64  # 连续句数的状态上限，防止循环展开爆掉


class Walker:
    """把脚本当成一棵语句树走一遍，状态是「此刻已经连着说了几句」的集合。"""

    def __init__(self, tokens):
        self.t = tokens
        self.i = 0
        self.max_run = 0
        self.talks = 0

    def peek(self, k=0):
        j = self.i + k
        return self.t[j] if j < len(self.t) else ("eof", "")

    def at(self, *words):
        kind, text = self.peek()
        return kind == "name" and text in words

    def skip_balanced(self, states):
        """当前 token 是 ( 或 {：吃掉整段实参，实参里的调用照样计数。"""
        open_tok = self.peek()[1]
        close_tok = {"(": ")", "{": "}", "[": "]"}[open_tok]
        self.i += 1
        while True:
            kind, text = self.peek()
            if kind == "eof":
                return states
            if kind == "op" and text == close_tok:
                self.i += 1
                return states
            if kind == "op" and text in "({[":
                states = self.skip_balanced(states)
                continue
            states = self.step(states)

    def call(self, name, states):
        """name 后面跟着实参：( … ) 或 { … } 或 "…"。"""
        kind, text = self.peek()
        if kind == "op" and text in "({":
            states = self.skip_balanced(states)
        elif kind == "str":
            self.i += 1
        if name == "talk":
            self.talks += 1
            states = {min(s + 1, CAP) for s in states}
            if states:
                self.max_run = max(self.max_run, max(states))
        elif name in OPERABLE:
            states = {0} if states else set()
        return states

    def step(self, states):
        """吃掉一个表达式里的 token；遇到调用就计数。"""
        kind, text = self.peek()
        self.i += 1
        if kind == "name" and text in ({"talk"} | OPERABLE):
            nk, nt = self.peek()
            prev = self.t[self.i - 2] if self.i >= 2 else ("", "")
            is_field = prev == ("op", ".") or prev == ("op", ":")
            if not is_field and ((nk == "op" and nt in "({") or nk == "str"):
                return self.call(text, states)
        return states

    def block(self, states, stop):
        while True:
            kind, text = self.peek()
            if kind == "eof":
                return states
            if kind == "name" and text in stop:
                return states
            if kind == "name" and text == "if":
                states = self.if_stmt(states)
            elif kind == "name" and text in ("while", "for"):
                self.i += 1
                while not self.at("do"):
                    states = self.step(states)
                self.i += 1
                start = self.i
                once = self.block(set(states), {"end"})
                end_pos = self.i
                self.i = start
                twice = self.block(set(once), {"end"})
                self.i = end_pos + 1
                states = set(states) | once | twice
            elif kind == "name" and text == "do":
                self.i += 1
                states = self.block(states, {"end"})
                self.i += 1
            elif kind == "name" and text == "repeat":
                self.i += 1
                body = self.block(states, {"until"})
                self.i += 1
                states = body | states
            elif kind == "name" and text == "function":
                depth = 0
                while True:
                    k, t = self.peek()
                    self.i += 1
                    if k == "name" and t in ("function", "if", "do"):
                        depth += 1
                    elif k == "name" and t == "end":
                        depth -= 1
                        if depth == 0:
                            break
                    elif k == "eof":
                        break
            elif kind == "name" and text == "return":
                self.i += 1
                while not (self.at("end", "else", "elseif", "until") or self.peek()[0] == "eof"):
                    states = self.step(states)
                states = set()
            else:
                states = self.step(states)

    def if_stmt(self, states):
        self.i += 1  # if
        outs = set()
        cur = set(states)
        while True:
            while not self.at("then"):
                cur = self.step(cur)
            self.i += 1  # then
            outs |= self.block(set(cur), {"elseif", "else", "end"})
            if self.at("elseif"):
                self.i += 1
                continue
            if self.at("else"):
                self.i += 1
                outs |= self.block(set(cur), {"end"})
                self.i += 1
                return outs
            self.i += 1  # end
            outs |= cur  # 没有 else：条件不成立的那条路
            return outs


def analyse_script(path: pathlib.Path, texts: dict[str, str]):
    source = path.read_text(encoding="utf-8")
    tokens = tokenize(source)
    walker = Walker(tokens)
    walker.block({0}, set())
    keys = []
    seen = set()
    for kind, text in tokens:
        if kind == "str" and text in texts and text not in seen:
            seen.add(text)
            keys.append(text)
    return {
        "script": path.stem,
        "talks": walker.talks,
        "max_run": walker.max_run,
        "keys": keys,
    }


def load_texts() -> dict[str, str]:
    texts = {}
    for p in sorted((ROOT / "data" / "text").glob("*.json")):
        try:
            payload = json.loads(p.read_text(encoding="utf-8"))
        except (json.JSONDecodeError, UnicodeDecodeError):
            continue
        if isinstance(payload, dict):
            for k, v in payload.items():
                if isinstance(v, str):
                    texts[k] = v
    return texts


def han(s: str) -> int:
    return len(CJK.findall(s))


def measure(chapter: str = "ch07"):
    texts = load_texts()
    rows = []
    for path in sorted((ROOT / "scripts" / chapter).glob("*.lua")):
        r = analyse_script(path, texts)
        r["chars"] = sum(han(texts[k]) for k in r["keys"])
        r["main"] = r["script"] in MAIN_SCRIPTS_CH07
        rows.append(r)
    main_keys = set()
    for r in rows:
        if r["main"]:
            main_keys.update(r["keys"])
    all_keys = set()
    for r in rows:
        all_keys.update(r["keys"])
    summary = {
        "main_chars": sum(han(texts[k]) for k in main_keys),
        "main_keys": len(main_keys),
        "all_chars": sum(han(texts[k]) for k in all_keys),
        "all_keys": len(all_keys),
        "max_run": max((r["max_run"] for r in rows), default=0),
        "max_run_script": max(rows, key=lambda r: r["max_run"])["script"] if rows else "",
        "talks": sum(r["talks"] for r in rows),
        "scripts": len(rows),
        "main_scripts": sum(1 for r in rows if r["main"]),
    }
    return rows, summary


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--chapter", default="ch07")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--markdown", action="store_true")
    args = ap.parse_args(argv)
    rows, summary = measure(args.chapter)
    if args.json:
        json.dump({"rows": [{k: v for k, v in r.items() if k != "keys"} for r in rows],
                   "summary": summary}, sys.stdout, ensure_ascii=False, indent=1)
        print()
        return 0
    if args.markdown:
        print("| 脚本 | 主线 | talk 处数 | 字数 | 最长连续 |")
        print("| --- | --- | --- | --- | --- |")
        for r in rows:
            print(f"| `{r['script']}` | {'✓' if r['main'] else ''} | {r['talks']} | {r['chars']} | {r['max_run']} |")
    else:
        for r in rows:
            mark = "*" if r["main"] else " "
            print(f"{mark} {r['script']:<16} talk {r['talks']:>3}  字 {r['chars']:>5}  连续 {r['max_run']:>2}")
    s = summary
    print(f"主线对白 {s['main_chars']} 字（{s['main_keys']} 个 key，{s['main_scripts']} 个脚本）；"
          f"全章脚本 {s['all_chars']} 字（{s['all_keys']} 个 key，{s['scripts']} 个脚本，talk {s['talks']} 处）；"
          f"最长连续 talk {s['max_run']} 句（{s['max_run_script']}）")
    return 0


if __name__ == "__main__":
    sys.exit(main())
