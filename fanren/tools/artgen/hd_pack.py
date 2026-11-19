"""手绘高清美术打包（伙伴改版 E 路，docs/interfaces-companions.md 2.5、3.9；格式见 docs/art-sprites.md 5.2、5.3）。

把协调者定稿的人物图、头像、首领图（Seedream 出图、已还原成真像素）做成游戏能读的资产：

  assets/art/sprites/hd/<人>_walk.png、<人>_battle.png   人物走路表（64×60，4×4）与战斗表（132×84，4×2），原样转存
  assets/art/sprites/hd/<敌人 id>.png                     非人形敌人：四个动作统一进一套格子（身躯、脚底对齐）、去残影
  assets/art/sprites/hd/index.json                        手绘索引：sheets、roles、role_variants、enemies、files（sha256）
  assets/art/portraits/<人>.png                           对白头像：四个表情排一行，格子统一、×1 画
  data/visual/portraits.json                              头像表（角色 id → 按章生效的头像列表）

    python tools/artgen/hd_pack.py [--src <定稿目录>]           # 写出上面这些
    python tools/artgen/hd_pack.py [--src <定稿目录>] --check   # 磁盘上的与重新打出来的逐字节一致才退出 0

确定性：不读时钟、没有随机数；缩放、对齐、去残影都是整数运算或按固定次序的取舍；JSON 键排序、UTF-8 无 BOM、LF。
重跑逐字节相同（--check 就是这么比的）。源目录在主检出里、不入库，所以 --check 不进四道门禁；
门禁规则 40 另核 index.json 里登记的 sha256 与磁盘上的图一致。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
DEFAULT_SRC = Path(r"H:\Work\Kys\fanren\docs\companions-overhaul")
HD_REL = Path("assets") / "art" / "sprites" / "hd"
PORTRAIT_REL = Path("assets") / "art" / "portraits"
PORTRAIT_JSON_REL = Path("data") / "visual" / "portraits.json"

# 定稿人物表的几何（量过：走路格 64×60、脚底第 57 行；战斗格 132×84、脚底第 81 行）。
WALK_GRID = (64, 60, 4, 4)       # 格宽、格高、列、行；行序：朝下、朝左、朝右、朝上
BATTLE_GRID = (132, 84, 4, 2)
BATTLE_ACTIONS = ("idle", "ready", "attack", "cast", "hurt", "down", "victory", "guard")
WALK_ROWS = ("down", "left", "right", "up")
HD_SCALE = 2                     # 世界层、战斗层都 ×2（契约 2.5）

# 头像格子：放得进 128×144、按 ×1 画（art-sprites.md 5.3）。源图大小不一，底边、中线对齐摆进去。
PORTRAIT_SLOT = (128, 144)
FACES = ("normal", "smile", "angry", "hurt")


@dataclass(frozen=True)
class Person:
    sheet_id: str          # 手绘表 id（不许与生成的表同名）
    stem: str              # 输出文件名前缀
    src_dir: str           # 定稿目录下的子目录
    prefix: str            # 源图文件名前缀
    name: str
    busts: tuple[int, ...]  # 用到的头像源图编号（<prefix>_bust_<n>.png），次序即头像表里的格子
    faces: tuple[int, int, int, int]   # normal / smile / angry / hurt 各指第几格


PEOPLE = (
    Person("hanli_hd", "hanli", "hanli-final", "hanli", "韩立（手绘，第 7 章）", (0, 1, 2, 3), (0, 1, 2, 3)),
    # 面纱版只有「平常」一张：四个表情都指它
    Person("nangong_wan_veil_hd", "nangong_wan_veil", "nangong-veil-final", "nangong_veil",
           "南宫婉·面纱（手绘）", (0,), (0, 0, 0, 0)),
    # 露脸前期：bust_0..3 是前期的四个表情（后面 4..8 是面纱与后期，这次不用）
    Person("nangong_wan_ch07_hd", "nangong_wan_ch07", "nangong-final", "nangong",
           "南宫婉·露脸前期（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("chen_shimei_hd", "chen_shimei", "chen-final", "chen", "陈师妹（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
)

# 角色 → 手绘表。这三个角色只在第 7 章出场，直接接管。
ROLES = {
    "baiyi_shaonv": "nangong_wan_veil_hd",
    "nangong_wan_ch07": "nangong_wan_ch07_hd",
    "chen_shimei": "chen_shimei_hd",
}
# 韩立每一章都在，只在第 7 章换手绘：第 1–2 章仍是生成的少年相，其余章节落回生成索引的 roles（hanli）。
ROLE_VARIANTS = {
    "hanli": [{"look": "hanli_child", "max_chapter": 2},
              {"look": "hanli_hd", "min_chapter": 7, "max_chapter": 7}],
}
# 头像：角色 id → (头像图的 stem, min_chapter, max_chapter)，0 = 这一端不限。
PORTRAITS = {
    "hanli": ("hanli", 7, 7),
    "baiyi_shaonv": ("nangong_wan_veil", 0, 0),
    "nangong_wan_ch07": ("nangong_wan_ch07", 0, 0),
    "chen_shimei": ("chen_shimei", 0, 0),
}


@dataclass(frozen=True)
class Enemy:
    enemy_id: str
    src_prefix: str        # bosses-final/<prefix>_act0..3.png：待机、出招、受击、倒下
    name: str
    kind: str
    match_size_of: str = ""   # 非空：整体缩到与这一只的待机帧一样大（同一条蛟，白的那套像素网格测得不同）
    ghost: str = ""           # 去残影的规则名（见 GHOST_RULES）


ENEMIES = (
    Enemy("mo_jiao", "boss_mojiao", "墨蛟", "beast", ghost="mojiao"),
    Enemy("mo_jiao_tuipi", "boss_mojiao_white", "墨蛟（蜕皮）", "beast", match_size_of="boss_mojiao", ghost="mojiao"),
    Enemy("bishui_e", "boss_bishuie", "碧水鳄", "beast", ghost="bishuie"),
)
ENEMY_ACTIONS = ("idle", "attack", "hurt", "down")
# 世界层（明雷）×1、战斗层 ×2：战斗里人高约 64×2 = 128 像素，墨蛟待机约 92×2 = 184，明显比人大，又不顶出画面。
# 世界层人是 ×2；这几张原生就有人的一倍多大，×1 已经占两三格。量的过程写在 docs/art-sprites.md 5.2。
ENEMY_SCALE = 1
ENEMY_BATTLE_SCALE = 2
ENEMY_MARGIN = 2          # 格子四周留的透明边


# ---------------------------------------------------------------------------
# 小工具
# ---------------------------------------------------------------------------

def load_rgba(path: Path) -> Image.Image:
    if not path.exists():
        raise SystemExit(f"hd_pack: 找不到源图 {path}")
    return Image.open(path).convert("RGBA")


def png_bytes(img: Image.Image) -> bytes:
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=False, compress_level=9)
    return buf.getvalue()


def json_bytes(payload: dict) -> bytes:
    return (json.dumps(payload, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode("utf-8")


def alpha_mask(img: Image.Image) -> np.ndarray:
    return np.asarray(img.getchannel("A")) > 0


def opaque_rows(img: Image.Image) -> tuple[int, int]:
    """最高、最低的不透明行（含）。"""
    box = img.getchannel("A").getbbox()
    if box is None:
        raise SystemExit("hd_pack: 有一格整格透明")
    return box[1], box[3] - 1


def grid_cells(img: Image.Image, grid: tuple[int, int, int, int], where: str) -> list[Image.Image]:
    fw, fh, cols, rows = grid
    if img.size != (fw * cols, fh * rows):
        raise SystemExit(f"hd_pack: {where} 尺寸 {img.size}，应当是 {fw * cols}×{fh * rows}（{cols}×{rows} 格 {fw}×{fh}）")
    return [img.crop((c * fw, r * fh, (c + 1) * fw, (r + 1) * fh)) for r in range(rows) for c in range(cols)]


# ---------------------------------------------------------------------------
# 人物表
# ---------------------------------------------------------------------------

def person_sheet(src: Path, person: Person, out: dict[str, bytes]) -> dict:
    base = src / person.src_dir
    walk = load_rgba(base / f"{person.prefix}_walk16.png")
    battle = load_rgba(base / f"{person.prefix}_battle8.png")
    walk_cells = grid_cells(walk, WALK_GRID, f"{person.prefix}_walk16.png")
    battle_cells = grid_cells(battle, BATTLE_GRID, f"{person.prefix}_battle8.png")
    walk_rows = [opaque_rows(c) for c in walk_cells]
    battle_rows = [opaque_rows(c) for c in battle_cells]
    walk_file, battle_file = f"{person.stem}_walk.png", f"{person.stem}_battle.png"
    out[walk_file] = png_bytes(walk)
    out[battle_file] = png_bytes(battle)
    cols = WALK_GRID[2]
    return {
        "name": person.name,
        "file": walk_file, "frame_w": WALK_GRID[0], "frame_h": WALK_GRID[1], "cols": cols,
        "scale": HD_SCALE, "walk_cycle": [0, 1, 2, 3], "battle_scale": HD_SCALE,
        "walk": {d: [r * cols + i for i in range(cols)] for r, d in enumerate(WALK_ROWS)},
        "battle": {a: [i] for i, a in enumerate(BATTLE_ACTIONS)},
        "battle_facing": "left",
        # 脚底取各格最低的不透明行（全是同一行），头顶取各格最高的不透明行里最高的那个
        "foot_y": max(b for _, b in walk_rows), "head_y": min(t for t, _ in walk_rows),
        "battle_sheet": {"file": battle_file, "frame_w": BATTLE_GRID[0], "frame_h": BATTLE_GRID[1],
                         "cols": BATTLE_GRID[2], "foot_y": max(b for _, b in battle_rows),
                         "head_y": min(t for t, _ in battle_rows)},
    }


# ---------------------------------------------------------------------------
# 头像
# ---------------------------------------------------------------------------

def portrait_cell_size(busts: list[Image.Image]) -> tuple[int, int]:
    """所有头像共用一种格子：放得下最大的那张，但不超过 128×144（×1 画）。超出的宽度两侧对半裁掉。"""
    w = min(PORTRAIT_SLOT[0], max(b.width for b in busts))
    h = min(PORTRAIT_SLOT[1], max(b.height for b in busts))
    return w, h


def place_bust(bust: Image.Image, cell: tuple[int, int]) -> tuple[Image.Image, int]:
    """底边对齐、中线对齐摆进格子，不缩放。返回 (格子图, 裁掉的不透明像素数)。"""
    cw, ch = cell
    canvas = Image.new("RGBA", cell, (0, 0, 0, 0))
    x = (cw - bust.width) // 2
    y = ch - bust.height
    canvas.paste(bust, (x, y))
    lost = int(alpha_mask(bust).sum()) - int(alpha_mask(canvas).sum())
    return canvas, lost


def build_portraits(src: Path, out_files: dict[str, bytes], notes: list[str]) -> dict:
    busts: dict[str, list[Image.Image]] = {}
    for person in PEOPLE:
        busts[person.stem] = [load_rgba(src / person.src_dir / f"{person.prefix}_bust_{n}.png") for n in person.busts]
    cell = portrait_cell_size([b for group in busts.values() for b in group])
    for person in PEOPLE:
        group = busts[person.stem]
        sheet = Image.new("RGBA", (cell[0] * len(group), cell[1]), (0, 0, 0, 0))
        for i, bust in enumerate(group):
            placed, lost = place_bust(bust, cell)
            if lost:
                notes.append(f"头像 {person.prefix}_bust_{person.busts[i]}（{bust.width}×{bust.height}）"
                             f"摆进 {cell[0]}×{cell[1]} 的格子裁掉了 {lost} 个不透明像素")
            sheet.paste(placed, (i * cell[0], 0))
        out_files[f"{person.stem}.png"] = png_bytes(sheet)
    by_stem = {p.stem: p for p in PEOPLE}
    portraits = {}
    for role, (stem, lo, hi) in sorted(PORTRAITS.items()):
        person = by_stem[stem]
        entry = {"file": f"{stem}.png", "frame_w": cell[0], "frame_h": cell[1], "cols": len(person.busts),
                 "faces": {face: person.faces[i] for i, face in enumerate(FACES)}}
        if lo:
            entry["min_chapter"] = lo
        if hi:
            entry["max_chapter"] = hi
        portraits[role] = [entry]
    return {
        "_doc": "对白头像（由 tools/artgen/hd_pack.py 生成，形状见 docs/art-sprites.md 5.3）：角色 id → 头像列表，"
                "每条可带 min_chapter / max_chapter（与 role_variants 同一个意思），取第一条成立的；都不成立就不带头像。"
                "图相对 assets/art/portraits/，normal / smile / angry / hurt 四个表情各指一格。没登记的角色对白照旧不带头像。",
        "portraits": portraits,
    }


# ---------------------------------------------------------------------------
# 非人形敌人
# ---------------------------------------------------------------------------

def majority_downscale(img: Image.Image, tw: int, th: int) -> Image.Image:
    """像素图按非整数倍缩小：目标每一格看它盖住的源像素，过半不透明才算实心，颜色取出现最多的那种
    （同票取 RGB 最小的，确定性）。不用插值：插值会糊出半透明的边和图里没有的颜色。"""
    src = np.asarray(img)
    h, w = src.shape[:2]
    out = np.zeros((th, tw, 4), dtype=np.uint8)
    for ty in range(th):
        y0, y1 = ty * h // th, max(ty * h // th + 1, (ty + 1) * h // th)
        for tx in range(tw):
            x0, x1 = tx * w // tw, max(tx * w // tw + 1, (tx + 1) * w // tw)
            block = src[y0:y1, x0:x1].reshape(-1, 4)
            solid = block[block[:, 3] > 0]
            if len(solid) * 2 < len(block):
                continue
            colors, counts = np.unique(solid[:, :3], axis=0, return_counts=True)
            best = colors[int(np.argmax(counts))]   # np.unique 按字典序排好，argmax 取第一个最大值 = RGB 最小的那种
            out[ty, tx, :3] = best
            out[ty, tx, 3] = 255
    return Image.fromarray(out, "RGBA")


def crop_to_content(img: Image.Image) -> Image.Image:
    box = img.getchannel("A").getbbox()
    if box is None:
        raise SystemExit("hd_pack: 敌人有一个动作整张透明")
    return img.crop(box)


def components(mask: np.ndarray) -> list[list[tuple[int, int]]]:
    """八连通的块，按先行后列扫到的次序给出（确定性）。每块是 (y, x) 的列表。"""
    h, w = mask.shape
    seen = np.zeros_like(mask, dtype=bool)
    out = []
    for y0 in range(h):
        for x0 in range(w):
            if not mask[y0, x0] or seen[y0, x0]:
                continue
            seen[y0, x0] = True
            stack, block = [(y0, x0)], []
            while stack:
                y, x = stack.pop()
                block.append((y, x))
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        yy, xx = y + dy, x + dx
                        if 0 <= yy < h and 0 <= xx < w and mask[yy, xx] and not seen[yy, xx]:
                            seen[yy, xx] = True
                            stack.append((yy, xx))
            out.append(block)
    return out


def erase(img: Image.Image, blocks: list[list[tuple[int, int]]]) -> Image.Image:
    px = np.asarray(img).copy()
    for block in blocks:
        for y, x in block:
            px[y, x] = (0, 0, 0, 0)
    return Image.fromarray(px, "RGBA")


def ghost_mojiao(img: Image.Image) -> Image.Image:
    """墨蛟：去掉尾巴那段半透明的残影——最左边那截细尾尖。

    判据：竖着量不透明的那一段不超过 6 像素厚（细），落在图的左四分之一，连成 10 像素以上的一块。
    后层的身圈也泛蓝、对比度低，但它连着身子，去掉身子就断了，所以留着。"""
    a = alpha_mask(img)
    h, w = a.shape
    thickness = np.zeros((h, w), dtype=np.int32)
    for x in range(w):
        y = 0
        while y < h:
            if not a[y, x]:
                y += 1
                continue
            top = y
            while y < h and a[y, x]:
                y += 1
            thickness[top:y, x] = y - top
    thin = a & (thickness <= 6)
    thin[:, w // 4:] = False
    img = erase(img, [b for b in components(thin) if len(b) >= 10])
    # 尾尖去掉之后剩下的碎点：整块落在左四分之一、不足 30 像素、不连着身子的，一并清掉。
    left = w // 4
    return erase(img, [b for b in components(alpha_mask(img))
                       if len(b) < 30 and all(x < left for _, x in b)])


def ghost_bishuie(img: Image.Image) -> Image.Image:
    """碧水鳄：去掉腹下那几块半透明的残影——远侧腿脚背光处泛蓝的暗色块（原稿在 Seedream 的蓝灰垫底色上抠出，
    半透明的地方混进了垫底色）。

    判据：偏蓝（蓝 ≥ 绿 × 0.85、且绿 > 红；鳄身是黄绿，蓝远小于绿），连成 6 像素以上的一块，
    重心在图的下 40%、横向在 20%–85% 之间（腹下；尾巴在左上，留着）。"""
    px = np.asarray(img).astype(np.int32)
    r, g, b, a = px[..., 0], px[..., 1], px[..., 2], px[..., 3]
    h, w = a.shape
    bluish = (a > 0) & (b * 100 >= g * 85) & (g > r)
    kill = []
    for block in components(bluish):
        if len(block) < 6:
            continue
        cy = sum(y for y, _ in block) / len(block)
        cx = sum(x for _, x in block) / len(block)
        if cy > h * 0.6 and w * 0.2 < cx < w * 0.85:
            kill.append(block)
    return erase(img, kill)


GHOST_RULES = {"mojiao": ghost_mojiao, "bishuie": ghost_bishuie}


def best_shift(ref: np.ndarray, mask: np.ndarray, limit: int) -> int:
    """两张底边对齐的剪影，水平挪多少重合得最多（身躯对齐）。同分取挪得少的、再取往左的，确定性。"""
    rh, rw = ref.shape
    mh, mw = mask.shape
    h = min(rh, mh)
    ref_b, mask_b = ref[rh - h:], mask[mh - h:]
    best, best_score = 0, -1
    for dx in sorted(range(-limit, limit + 1), key=lambda d: (abs(d), d)):
        # mask 的第 0 列对到 ref 的第 dx 列
        x0, x1 = max(0, dx), min(rw, dx + mw)
        if x1 <= x0:
            continue
        score = int((ref_b[:, x0:x1] & mask_b[:, x0 - dx:x1 - dx]).sum())
        if score > best_score:
            best, best_score = dx, score
    return best


def build_enemy(src: Path, enemy: Enemy, out: dict[str, bytes]) -> dict:
    base = src / "bosses-final"
    acts = [load_rgba(base / f"{enemy.src_prefix}_act{i}.png") for i in range(len(ENEMY_ACTIONS))]
    if enemy.match_size_of:
        ref = load_rgba(base / f"{enemy.match_size_of}_act0.png")
        # 按面积配：同一条蛟两套像素网格测得不同，体形一处没变，所以整体乘同一个比例。
        factor = math.sqrt((ref.width * ref.height) / (acts[0].width * acts[0].height))
        acts = [majority_downscale(a, max(1, round(a.width * factor)), max(1, round(a.height * factor))) for a in acts]
    rule = GHOST_RULES[enemy.ghost] if enemy.ghost else (lambda im: im)
    acts = [crop_to_content(rule(a)) for a in acts]
    # 身躯对齐：每个动作底边对齐，水平方向挪到与待机帧的剪影重合最多的地方。
    idle_mask = alpha_mask(acts[0])
    shifts = [0] + [best_shift(idle_mask, alpha_mask(a), acts[0].width // 4) for a in acts[1:]]
    left = min(shifts)
    right = max(s + a.width for s, a in zip(shifts, acts))
    tall = max(a.height for a in acts)
    fw, fh = right - left + ENEMY_MARGIN * 2, tall + ENEMY_MARGIN * 2
    sheet = Image.new("RGBA", (fw * len(acts), fh), (0, 0, 0, 0))
    for i, (shift, act) in enumerate(zip(shifts, acts)):
        x = i * fw + ENEMY_MARGIN + shift - left
        y = fh - ENEMY_MARGIN - act.height
        sheet.paste(act, (x, y))
    file_name = f"{enemy.enemy_id}.png"
    out[file_name] = png_bytes(sheet)
    return {
        "name": enemy.name, "kind": enemy.kind, "file": file_name,
        "frame_w": fw, "frame_h": fh, "cols": len(acts),
        "battle": {a: [i] for i, a in enumerate(ENEMY_ACTIONS)}, "battle_facing": "right",
        "foot_y": fh - ENEMY_MARGIN - 1, "head_y": ENEMY_MARGIN + min(tall - a.height for a in acts),
        "scale": ENEMY_SCALE, "battle_scale": ENEMY_BATTLE_SCALE,
    }


# ---------------------------------------------------------------------------
# 总装
# ---------------------------------------------------------------------------

def build(src: Path) -> tuple[dict[Path, bytes], list[str]]:
    """返回 {相对仓库根的路径: 字节}，以及要写进汇报的备注。"""
    notes: list[str] = []
    hd_files: dict[str, bytes] = {}
    sheets = {p.sheet_id: person_sheet(src, p, hd_files) for p in PEOPLE}
    enemies = {e.enemy_id: build_enemy(src, e, hd_files) for e in ENEMIES}
    index = {
        "_doc": "手绘高清表（由 tools/artgen/hd_pack.py 生成，形状见 docs/art-sprites.md 5.2）。"
                "file 相对本目录；files 里的 sha256 由门禁规则 40 核。",
        "sheets": sheets,
        "roles": dict(ROLES),
        "role_variants": ROLE_VARIANTS,
        "enemies": enemies,
        "files": {name: hashlib.sha256(data).hexdigest() for name, data in sorted(hd_files.items())},
    }
    portrait_files: dict[str, bytes] = {}
    portrait_table = build_portraits(src, portrait_files, notes)
    result: dict[Path, bytes] = {}
    for name, data in hd_files.items():
        result[HD_REL / name] = data
    result[HD_REL / "index.json"] = json_bytes(index)
    for name, data in portrait_files.items():
        result[PORTRAIT_REL / name] = data
    result[PORTRAIT_JSON_REL] = json_bytes(portrait_table)
    return result, notes


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="手绘高清美术打包")
    ap.add_argument("--src", type=Path, default=DEFAULT_SRC, help="定稿图目录（缺省：主检出 docs/companions-overhaul）")
    ap.add_argument("--out", type=Path, default=REPO, help="输出根（缺省：仓库的 fanren/）")
    ap.add_argument("--check", action="store_true", help="只比对：磁盘上的与重新打出来的逐字节一致才退出 0")
    args = ap.parse_args(argv)
    files, notes = build(args.src)
    if args.check:
        diffs = []
        for rel, data in sorted(files.items()):
            target = args.out / rel
            if not target.exists():
                diffs.append(f"缺  {rel.as_posix()}")
            elif target.read_bytes() != data:
                diffs.append(f"变  {rel.as_posix()}")
        for d in (HD_REL, PORTRAIT_REL):
            for f in sorted((args.out / d).glob("*")) if (args.out / d).exists() else []:
                if f.is_file() and f.relative_to(args.out) not in files:
                    diffs.append(f"多  {f.relative_to(args.out).as_posix()}（打包脚本已不产出它）")
        for line in diffs:
            print("  " + line)
        if diffs:
            print(f"HD_PACK_OUT_OF_SYNC：{len(diffs)} 个文件与打包结果不一致")
            return 1
        print(f"HD_PACK_IN_SYNC：{len(files)} 个文件与打包结果逐字节一致")
        return 0
    for rel, data in sorted(files.items()):
        target = args.out / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
    for line in notes:
        print("注意：" + line)
    print(f"HD_PACK_WROTE：{len(files)} 个文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
