"""女角色高清立绘打包（docs/heroine-art-build.md 第 2 节）。

把 Seedream 出的透明底原图（1024×1536，膝上半身，一人一图，不入库）做成游戏能读的立绘表：

  assets/art/busts/<文件>.png     一行若干格，每格 840×1260；格子依次是 normal、smile、angry、hurt 里**画了的**那几个，
                                   没画的表情在表里指回 normal 那一格（不占格子）
  data/visual/busts.json          立绘表（角色 id → 按章生效的立绘列表，形状照 portraits.json）＋ files（文件名 → sha256）

    python tools/artgen/busts.py [--src <原图目录>]           # 写出上面这些（要原图）
    python tools/artgen/busts.py [--src <原图目录>] --check   # 核：图与表的 sha256 对得上；有原图时再重打一遍比字节

清单（哪个角色用哪几张原图、按章生效、微调值）入库：tools/artgen/busts_manifest.json。原图不入库，
在 --src 指的目录下（缺省：环境变量 FANREN_BUST_SRC，再没有就是仓库里的 docs/heroine-art/final/）。
所以 --check 分两半：图与 busts.json 的 files 一致、表里每一格都在图里——随时能跑；从原图重新打包、与磁盘逐字节相同——
原图都在才跑，不在时打印 BUSTS_IN_SYNC_PARTIAL 并写明「跳过，没核」。

一张原图怎么进格子：
  1. 按透明通道（alpha > ALPHA_CUT）找出人的外框，裁掉四周空白；
  2. 等比缩放，使「头顶 → 外框底边」正好落在「头顶线 → 格子底边」：头顶线 = HEAD_LINE + 清单里这个角色的 head_adjust
     （各角色的头顶在格子里高度一致，换人说话时头不跳）；外框比格子宽时改按宽缩、底边仍贴格子底；
  3. 水平按外框居中（加清单里的 x_adjust）；
  4. 缩放在预乘透明度上做（不出黑边），LANCZOS。

游戏里一格画成 420×630（存两倍）。头顶线 60 在屏幕上是 30（施工图 3.5.1）：人物从头顶到格底在屏幕上高 600；
对话框在底部时格子顶边在 y = 18，头顶约在 y = 48，画到框顶为止（头到腰胯）。各人脸的大小、下巴的高度用清单的 head_adjust 对齐。

确定性：不读时钟、没有随机数；缩放参数全由整数算出；PNG 固定压缩级别、不写时间戳；JSON 键排序、UTF-8 无 BOM、LF。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import sys
from dataclasses import dataclass, field
from pathlib import Path

from PIL import Image

REPO = Path(__file__).resolve().parents[2]
SRC_ENV = "FANREN_BUST_SRC"
REPO_SRC = REPO / "docs" / "heroine-art" / "final"
MANIFEST = Path(__file__).resolve().parent / "busts_manifest.json"

BUST_REL = Path("assets") / "art" / "busts"
BUST_JSON_REL = Path("data") / "visual" / "busts.json"

CELL_W = 840
CELL_H = 1260
HEAD_LINE = 60           # 格子里头顶所在的行（缺省；清单里每个角色可加 head_adjust）
ALPHA_CUT = 16           # 找外框时 alpha 不超过它的当空白（抠图边缘的淡雾）
FACES = ("normal", "smile", "angry", "hurt")

DOC = ("女角色高清立绘（由 tools/artgen/busts.py 按 tools/artgen/busts_manifest.json 生成，docs/heroine-art-build.md 第 2 节）："
       "角色 id → 立绘列表，形状照 portraits.json（每条可带 min_chapter / max_chapter，取第一条成立的）。"
       "图相对 assets/art/busts/，每格 840×1260、屏幕上画成 420×630；没画的表情指回 normal 那一格。"
       "files 是每张图的 sha256（busts.py --check 核）。没登记的角色对白照旧画像素小头像。")


def default_src() -> Path:
    env = os.environ.get(SRC_ENV, "").strip()
    return Path(env) if env else REPO_SRC


# ---------------------------------------------------------------------------
# 清单
# ---------------------------------------------------------------------------

@dataclass
class Entry:
    file: str                          # 产物文件名（assets/art/busts/ 下）
    roles: list[str]                   # 共用这一张表的角色 id（卖符少女 / 菡云芝是同一个人）
    faces: dict[str, str]              # 表情 → 原图（相对 --src）；至少有 normal
    head_adjust: int = 0               # 头顶线微调（格子像素，正数往下）
    x_adjust: int = 0                  # 水平微调（格子像素，正数往右）
    min_chapter: int = 0
    max_chapter: int = 0
    order: list[str] = field(default_factory=list)   # 画了的表情，按 FACES 的次序


def load_manifest(path: Path = MANIFEST) -> list[Entry]:
    raw = json.loads(path.read_text(encoding="utf-8"))
    entries: list[Entry] = []
    files: set[str] = set()
    for i, item in enumerate(raw.get("busts", [])):
        where = f"busts_manifest.json 的 busts[{i}]"
        unknown = set(item) - {"file", "roles", "faces", "head_adjust", "x_adjust", "min_chapter", "max_chapter", "_note"}
        if unknown:
            raise SystemExit(f"busts: {where}：认不出的字段 {sorted(unknown)}")
        faces = item.get("faces", {})
        bad = set(faces) - set(FACES)
        if bad:
            raise SystemExit(f"busts: {where}.faces：认不出的表情 {sorted(bad)}（只有 {' / '.join(FACES)}）")
        if not faces.get("normal"):
            raise SystemExit(f"busts: {where}.faces：缺 normal")
        if not item.get("file") or not item.get("roles"):
            raise SystemExit(f"busts: {where}：file、roles 都要写")
        if item["file"] in files:
            raise SystemExit(f"busts: {where}：文件名 {item['file']} 重复")
        files.add(item["file"])
        e = Entry(file=item["file"], roles=list(item["roles"]), faces={k: v for k, v in faces.items() if v},
                  head_adjust=int(item.get("head_adjust", 0)), x_adjust=int(item.get("x_adjust", 0)),
                  min_chapter=int(item.get("min_chapter", 0)), max_chapter=int(item.get("max_chapter", 0)))
        e.order = [f for f in FACES if f in e.faces]
        entries.append(e)
    return entries


# ---------------------------------------------------------------------------
# 打包
# ---------------------------------------------------------------------------

def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=False, compress_level=9)
    return buf.getvalue()


def json_bytes(payload: dict) -> bytes:
    return (json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode("utf-8")


def content_box(img: Image.Image) -> tuple[int, int, int, int]:
    """人的外框（左、上、右、下，右下不含）：alpha > ALPHA_CUT 的像素。"""
    mask = img.getchannel("A").point(lambda a: 255 if a > ALPHA_CUT else 0)
    box = mask.getbbox()
    if box is None:
        raise SystemExit("busts: 有一张原图整张透明")
    return box


def place(img: Image.Image, head_adjust: int, x_adjust: int, where: str, notes: list[str]) -> Image.Image:
    """一张原图 → 一格 840×1260。"""
    left, top, right, bottom = content_box(img)
    w, h = right - left, bottom - top
    head = HEAD_LINE + head_adjust
    if not 0 <= head < CELL_H - 100:
        raise SystemExit(f"busts: {where}：头顶线 {head} 不合理")
    # 目标高 = 头顶线到格底；比格子宽就改按宽缩（底边仍贴格底，头顶往下让）。整数算，保证确定。
    th = CELL_H - head
    tw = (w * th + h // 2) // h
    if tw > CELL_W:
        notes.append(f"{where}：按头顶线缩放后宽 {tw} 超过格宽 {CELL_W}，改按宽缩（头顶比别人低）")
        tw = CELL_W
        th = (h * tw + w // 2) // w
    crop = img.crop((left, top, right, bottom))
    scaled = crop.convert("RGBa").resize((tw, th), Image.Resampling.LANCZOS).convert("RGBA")
    cell = Image.new("RGBA", (CELL_W, CELL_H), (0, 0, 0, 0))
    x = (CELL_W - tw) // 2 + x_adjust
    x = max(0, min(CELL_W - tw, x))
    cell.paste(scaled, (x, CELL_H - th), scaled)
    return cell


def load_source(src: Path, rel: str, where: str) -> Image.Image:
    path = src / rel
    if not path.is_file():
        raise SystemExit(f"busts: {where}：找不到原图 {path}")
    return Image.open(path).convert("RGBA")


def build(src: Path, entries: list[Entry]) -> tuple[dict[Path, bytes], list[str]]:
    out: dict[Path, bytes] = {}
    notes: list[str] = []
    table: dict[str, list[dict]] = {}
    digests: dict[str, str] = {}
    for e in entries:
        cells = [place(load_source(src, e.faces[f], f"{e.file}.{f}"), e.head_adjust, e.x_adjust, f"{e.file}.{f}", notes)
                 for f in e.order]
        sheet = Image.new("RGBA", (CELL_W * len(cells), CELL_H), (0, 0, 0, 0))
        for i, cell in enumerate(cells):
            sheet.paste(cell, (i * CELL_W, 0))
        data = png_bytes(sheet)
        out[BUST_REL / e.file] = data
        digests[e.file] = hashlib.sha256(data).hexdigest()
        faces = {f: (e.order.index(f) if f in e.order else 0) for f in FACES}
        row: dict = {"file": e.file, "frame_w": CELL_W, "frame_h": CELL_H, "cols": len(cells), "faces": faces}
        if e.min_chapter:
            row["min_chapter"] = e.min_chapter
        if e.max_chapter:
            row["max_chapter"] = e.max_chapter
        for role in e.roles:
            table.setdefault(role, []).append(row)
    out[BUST_JSON_REL] = json_bytes({"_doc": DOC, "busts": table, "files": digests})
    return out, notes


def sources_present(src: Path, entries: list[Entry]) -> list[str]:
    return [f"{e.file}.{f} → {src / e.faces[f]}" for e in entries for f in e.order if not (src / e.faces[f]).is_file()]


# ---------------------------------------------------------------------------
# 核对
# ---------------------------------------------------------------------------

def check_committed(out: Path) -> list[str]:
    """不靠原图的那一半：busts.json 读得进来；files 里每张图都在、sha256 一致；busts/ 下没有没登记的图；
    表里每一格都在图里。"""
    problems: list[str] = []
    table_path = out / BUST_JSON_REL
    if not table_path.is_file():
        return [f"缺  {BUST_JSON_REL.as_posix()}"]
    try:
        table = json.loads(table_path.read_text(encoding="utf-8"))
    except ValueError as err:
        return [f"坏  {BUST_JSON_REL.as_posix()}：{err}"]
    files = table.get("files", {})
    for name, digest in sorted(files.items()):
        target = out / BUST_REL / name
        if not target.is_file():
            problems.append(f"缺  {(BUST_REL / name).as_posix()}")
        elif hashlib.sha256(target.read_bytes()).hexdigest() != digest:
            problems.append(f"变  {(BUST_REL / name).as_posix()}（sha256 与 busts.json 不一致）")
    folder = out / BUST_REL
    if folder.is_dir():
        for f in sorted(folder.iterdir()):
            if f.is_file() and f.name not in files:
                problems.append(f"多  {(BUST_REL / f.name).as_posix()}（busts.json 的 files 没登记）")
    for role, rows in sorted(table.get("busts", {}).items()):
        for i, row in enumerate(rows):
            if row.get("file") not in files:
                problems.append(f"表  busts.{role}[{i}] 引的 {row.get('file')} 不在 files 里")
                continue
            target = out / BUST_REL / row["file"]
            if not target.is_file():
                continue
            with Image.open(target) as img:
                iw, ih = img.size
            cols = max(1, int(row.get("cols", 1)))
            fw, fh = int(row.get("frame_w", 0)), int(row.get("frame_h", 0))
            for face, frame in sorted(row.get("faces", {}).items()):
                x, y = (frame % cols) * fw, (frame // cols) * fh
                if x + fw > iw or y + fh > ih:
                    problems.append(f"格  busts.{role}[{i}].{face} 那一格出了图 {row['file']}（{iw}×{ih}）")
    return problems


def check_repack(src: Path, out: Path, entries: list[Entry]) -> list[str]:
    built, _ = build(src, entries)
    problems: list[str] = []
    for rel, data in sorted(built.items()):
        target = out / rel
        if not target.is_file():
            problems.append(f"缺  {rel.as_posix()}（从原图打得出来）")
        elif target.read_bytes() != data:
            problems.append(f"变  {rel.as_posix()}（与从原图重打的不同）")
    return problems


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="女角色高清立绘打包")
    ap.add_argument("--src", type=Path, default=None,
                    help=f"原图目录（不入库；缺省取环境变量 {SRC_ENV}，再没有就是仓库里的 docs/heroine-art/final）")
    ap.add_argument("--out", type=Path, default=REPO, help="输出根（缺省：仓库的 fanren/）")
    ap.add_argument("--manifest", type=Path, default=MANIFEST, help="清单（缺省：tools/artgen/busts_manifest.json）")
    ap.add_argument("--check", action="store_true", help="只比对：图与表对得上；原图都在时再重打一遍比字节")
    args = ap.parse_args(argv)
    src = args.src if args.src is not None else default_src()
    entries = load_manifest(args.manifest)
    missing = sources_present(src, entries)
    if args.check:
        problems = check_committed(args.out)
        if not missing:
            problems += check_repack(src, args.out, entries)
        for line in problems:
            print("  " + line)
        if problems:
            print(f"BUSTS_OUT_OF_SYNC：{len(problems)} 处不一致")
            return 1
        if missing:
            print(f"BUSTS_IN_SYNC_PARTIAL：图与表的 sha256 一致；原图缺 {len(missing)} 张（{src}），"
                  "「从原图重新打包」那一半跳过，没核")
            return 0
        print("BUSTS_IN_SYNC：图与表的 sha256 一致；从原图重新打包，逐字节相同")
        return 0
    if missing:
        for line in missing:
            print("  缺原图：" + line)
        print("busts: 原图不全，打不了（用 --src 指定原图目录）")
        return 1
    files, notes = build(src, entries)
    # 清单里删掉的图：busts/ 下多出来的一并删掉，免得 --check 报「多」。
    folder = args.out / BUST_REL
    keep = {rel.name for rel in files if rel.parent == BUST_REL}
    if folder.is_dir():
        for f in sorted(folder.iterdir()):
            if f.is_file() and f.suffix == ".png" and f.name not in keep:
                f.unlink()
    for rel, data in sorted(files.items()):
        target = args.out / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    for line in notes:
        print("注意：" + line)
    print(f"BUSTS_WROTE：{len(files)} 个文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
