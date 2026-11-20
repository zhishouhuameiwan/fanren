"""Seedream 原稿 → 真像素帧（第 7 章美术重构，2026-11-19）。

hd_pack.py 吃的是「已经还原成真像素」的定稿帧：<人>_walk16.png（4×4 格 64×60）、<人>_battle8.png（4×2 格 132×84）、
bosses-final/<敌人>_act0..3.png（四个动作各一张、裁到内容）。从 Seedream 原稿到这一步，伙伴改版那一轮用的是
work-notes 里没入库的脚本；这里重写成入库的一份，规矩照 docs/companions-overhaul.md 2.4、2.4b 记下的那几条：

  · 按连通块切帧（按格子切会把伸出去的手、喷出去的寒气切到邻格）；
  · 每一条走路按站立帧定高（Seedream 每条画的人高不一样）——四个朝向逐行拉齐；
  · 按下半身的中线对齐（按外框居中，飘带和伸出去的手会带着人左右抖）；
  · 带渐变的原稿用「深色优先」缩：一格里深色（描线、头发、眼睛）占四成半以上取深色的平均，否则只拿浅色求平均——
    按票数取色会把白衣缩出雪花点、眼睛时有时无；
  · 最后整张表取一个有限色板、不抖动。

原稿放在 <源目录>/raw/<id>/（不入库），产物写进 <源目录>/<src_dir>/ 与 <源目录>/bosses-final/，再由 hd_pack.py 打包。

    python tools/artgen/hd_frames.py [--src <定稿目录>] [--only <id>]

确定性：不读时钟、没有随机数；同一份原稿两次跑出的帧逐像素相同。
"""

from __future__ import annotations

import argparse
import os
import sys
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image
from scipy import ndimage

REPO = Path(__file__).resolve().parents[2]
SRC_ENV = "FANREN_HD_SRC"
REPO_SRC = REPO / "docs" / "companions-overhaul"

WALK_CELL = (64, 60)
WALK_FOOT = 57          # 脚底那一行（hd_pack 量过：定稿人物表脚底第 57 行）
BATTLE_CELL = (132, 84)
BATTLE_FOOT = 81
ALPHA_SOLID = 128       # 原稿 alpha 过半才算人身上的像素
DARK_LUMA = 50          # 深色：描线、头发、眼睛（72 时黝黑的脸上眉眼糊成一条，样张比过 4 档）
DARK_SHARE = 0.45


@dataclass(frozen=True)
class PersonRaw:
    pid: str                    # raw/<pid>/ 下：walk_down.png、walk_right.png、walk_up.png、battle.png
    src_dir: str                # 写到 <源目录>/<src_dir>/<prefix>_walk16.png、_battle8.png
    prefix: str
    walk_h: int = 47            # 站立帧高（定稿人物 46–48）
    battle_h: int = 64          # 战斗待机帧高（韩立、陈巧倩的待机都是 64）
    colors: int = 96            # 40 时白袍会把肤色并成灰白（清虚门道士，2026-11-20 比过 40/64/96），人物一律 96
    busts: bool = False         # 具名角色：raw/<pid>/busts.png 四张脸横排（平常、笑、怒、伤）→ <prefix>_bust_0..3.png


@dataclass(frozen=True)
class EnemyRaw:
    eid: str                    # raw/<eid>/acts.png：四个动作横排一行（待机、出招、受击、倒下）
    prefix: str                 # 写到 <源目录>/bosses-final/<prefix>_act0..3.png
    idle_w: int                 # 待机帧缩到多宽
    colors: int = 40
    join: int = 24              # 切帧时把相隔这么多像素以内的碎块并进同一帧（喷出去的寒气、碎冰）


# 战斗表的并块距离：飞刀、掉在一旁的兵刃与人隔开二三十像素，12 会把它们当碎屑丢掉（2026-11-20 封岳、陆师兄）。
BATTLE_JOIN = 30
BUST_H = 128            # 头像缩到多高（头像格最大 128×144、×1 画；定稿头像 86–136 高）
BUST_W = 128


def named(pid: str, walk_h: int = 47, battle_h: int = 64) -> PersonRaw:
    return PersonRaw(pid, "named-final", pid, walk_h, battle_h, busts=True)


def passer(pid: str, walk_h: int = 47, battle_h: int = 64) -> PersonRaw:
    return PersonRaw(pid, "passers-final", pid, walk_h, battle_h)


# 身高按三视图里的体型定：矮的 42–44，高的 50；战斗待机按同一比例。
PEOPLE: tuple[PersonRaw, ...] = (
    passer("lingshoushan_dizi"),
    named("li_huayuan"), named("ma_shibo", 42, 58), named("xiang_zhili"), named("xu_lao"), named("chou_han"),
    named("han_yunzhi", 46, 62), named("zhong_wu", 44, 60), named("fuyunzi"), named("nichang_xianzi", 46, 62),
    named("qiong_qianbei"), named("feng_yue", 50, 68), named("lu_shixiong"), named("luosai_huzi"),
    named("yan_wuchi"), named("yan_xiongdi"), named("chen_dage"), named("huangshan_shijie", 46, 62),
    named("xiang_shaonian", 44, 60), named("jujianmen_gaoren"),
    passer("fangshi_tanzhu"), passer("fangshi_zhishi"), passer("huadaowu_dizi"), passer("huangfeng_shilian"),
    passer("jujianmen_dizi"), passer("qingxumen_daoshi"), passer("tianquebao_dizi"), passer("wanbaolou_shicong"),
    passer("yanyue_dizi", 46, 62), passer("yuanwu_xiushi"),
)
# 待机帧宽：照原著体型与旧图在屏幕上的大小定（世界层、战斗层都 ×2）。
ENEMIES: tuple[EnemyRaw, ...] = (
    EnemyRaw("hanbing_chan", "beast_hanbing_chan", idle_w=48),
    EnemyRaw("cai_e", "beast_cai_e", idle_w=44),
    EnemyRaw("chuanshan_shou", "beast_chuanshan_shou", idle_w=52),
    EnemyRaw("fei_she", "beast_fei_she", idle_w=60),
    EnemyRaw("huoyan_shu", "beast_huoyan_shu", idle_w=34),
    EnemyRaw("tiebi_yuan", "beast_tiebi_yuan", idle_w=46),
    EnemyRaw("tuishan_shou", "beast_tuishan_shou", idle_w=80, join=6),   # 四个动作挨得近：并块距离 24 会把两帧并成一块
    EnemyRaw("zichi_bishe", "beast_zichi_bishe", idle_w=50),
    EnemyRaw("jinzhi_yanceng", "beast_jinzhi_yanceng", idle_w=88),
)


def default_src() -> Path:
    env = os.environ.get(SRC_ENV, "").strip()
    return Path(env) if env else REPO_SRC


# ---------------------------------------------------------------------------
# 切帧
# ---------------------------------------------------------------------------

def split_frames(img: Image.Image, count: int, rows: int, join: int) -> list[Image.Image]:
    """按连通块切出 count 帧（rows 行、每行 count // rows 帧），次序先行后列。每帧只留自己那一块的像素。"""
    rgba = np.asarray(img.convert("RGBA"))
    solid = rgba[:, :, 3] >= ALPHA_SOLID
    joined = ndimage.binary_dilation(solid, iterations=join) if join > 0 else solid
    labels, n = ndimage.label(joined)
    if n < count:
        raise SystemExit(f"hd_frames: 只切出 {n} 块，要 {count} 帧")
    sizes = ndimage.sum(solid, labels, index=range(1, n + 1))
    keep = sorted(range(1, n + 1), key=lambda k: (-sizes[k - 1], k))[:count]
    boxes = {k: ndimage.find_objects((labels == k).astype(np.int32))[0] for k in keep}
    centers = {k: ((b[0].start + b[0].stop) / 2, (b[1].start + b[1].stop) / 2) for k, b in boxes.items()}
    by_y = sorted(keep, key=lambda k: (centers[k][0], centers[k][1]))
    per_row = count // rows
    ordered: list[int] = []
    for r in range(rows):
        ordered += sorted(by_y[r * per_row:(r + 1) * per_row], key=lambda k: centers[k][1])
    frames = []
    for k in ordered:
        ys, xs = boxes[k]
        part = rgba[ys, xs].copy()
        mine = (labels[ys, xs] == k) & solid[ys, xs]
        part[~mine] = 0
        part[mine, 3] = 255
        frames.append(Image.fromarray(part, "RGBA"))
    return frames


# ---------------------------------------------------------------------------
# 缩图
# ---------------------------------------------------------------------------

def clean_downscale(img: Image.Image, tw: int, th: int) -> Image.Image:
    src = np.asarray(img.convert("RGBA")).astype(np.float64)
    h, w = src.shape[:2]
    luma = src[:, :, 0] * 0.299 + src[:, :, 1] * 0.587 + src[:, :, 2] * 0.114
    out = np.zeros((th, tw, 4), dtype=np.uint8)
    for ty in range(th):
        y0, y1 = ty * h // th, max(ty * h // th + 1, (ty + 1) * h // th)
        for tx in range(tw):
            x0, x1 = tx * w // tw, max(tx * w // tw + 1, (tx + 1) * w // tw)
            block = src[y0:y1, x0:x1].reshape(-1, 4)
            lum = luma[y0:y1, x0:x1].reshape(-1)
            on = block[:, 3] >= ALPHA_SOLID
            if on.sum() * 2 < len(block):
                continue
            dark = on & (lum < DARK_LUMA)
            pick = dark if dark.sum() >= DARK_SHARE * on.sum() else on & ~dark
            if not pick.any():
                pick = on
            out[ty, tx, :3] = np.round(block[pick, :3].mean(axis=0)).astype(np.uint8)
            out[ty, tx, 3] = 255
    return Image.fromarray(out, "RGBA")


def scaled(frame: Image.Image, factor: float) -> Image.Image:
    return clean_downscale(frame, max(1, round(frame.width * factor)), max(1, round(frame.height * factor)))


def crop(img: Image.Image) -> Image.Image:
    box = img.getchannel("A").getbbox()
    if box is None:
        raise SystemExit("hd_frames: 有一帧整帧透明")
    return img.crop(box)


def limit_palette(sheet: Image.Image, colors: int) -> Image.Image:
    """整张表共用一个有限色板，不抖动。透明的格子不参与取色。"""
    arr = np.asarray(sheet)
    on = arr[:, :, 3] > 0
    if not on.any():
        return sheet
    pixels = arr[on][:, :3]
    strip = Image.fromarray(pixels.reshape(1, -1, 3), "RGB")
    pal = strip.quantize(colors=colors, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE)
    mapped = np.asarray(pal.convert("RGB")).reshape(-1, 3)
    out = arr.copy()
    out[on, :3] = mapped
    return Image.fromarray(out, "RGBA")


def skin_mask(arr: np.ndarray) -> np.ndarray:
    """肤色像素：偏暖（R > G > B、R 比 B 高出一截），不算描线、头发这类很暗的和高光很亮的。"""
    r, g, b = (arr[:, :, i].astype(np.float64) for i in range(3))
    luma = r * 0.299 + g * 0.587 + b * 0.114
    return (arr[:, :, 3] > 0) & (r > g) & (g > b) & (r - b > 25) & (luma > 35) & (luma < 220)


def skin_luma(img: Image.Image) -> float:
    arr = np.asarray(img)
    m = skin_mask(arr)
    if not m.any():
        return 0.0
    return float((arr[m][:, :3].astype(np.float64) @ np.array([0.299, 0.587, 0.114])).mean())


# Seedream 画「受伤」时常把整张脸压暗一层（2026-11-20 钟吾 1.26、浮云子 1.15、向之礼 1.09 等八人），配上嘴角的血像是脸色发灰。
# 受伤那张的肤色比另外三张平均暗出这么多时，只把肤色像素提回去，衣服、头发、血不动。
HURT_SKIN_MIN_GAIN = 1.04


def match_hurt_skin(hurt: Image.Image, others: list[Image.Image]) -> Image.Image:
    have = skin_luma(hurt)
    want = float(np.mean([skin_luma(f) for f in others]))
    if have <= 0 or want / have < HURT_SKIN_MIN_GAIN:
        return hurt
    arr = np.asarray(hurt).copy()
    m = skin_mask(arr)
    arr[m, :3] = np.clip(np.round(arr[m, :3].astype(np.float64) * (want / have)), 0, 255).astype(np.uint8)
    return Image.fromarray(arr, "RGBA")


def lower_center(frame: Image.Image) -> float:
    """下半身（最低的 35% 行）不透明像素的平均 x。"""
    a = np.asarray(frame.getchannel("A")) > 0
    h = a.shape[0]
    lower = a[h - max(1, int(h * 0.35)):]
    xs = np.nonzero(lower)[1]
    return float(xs.mean()) if len(xs) else frame.width / 2


def place(sheet: Image.Image, frame: Image.Image, cell: tuple[int, int], index: int, cols: int, foot: int) -> None:
    cw, ch = cell
    cx, cy = (index % cols) * cw, (index // cols) * ch
    x = cx + int(round(cw / 2 - lower_center(frame)))
    y = cy + foot + 1 - frame.height
    x = max(cx, min(x, cx + cw - frame.width))
    y = max(cy, y)
    sheet.alpha_composite(frame, (x, y))


# ---------------------------------------------------------------------------
# 人物、敌人
# ---------------------------------------------------------------------------

def build_person(src: Path, p: PersonRaw) -> list[Path]:
    raw = src / "raw" / p.pid
    rows: dict[str, list[Image.Image]] = {}
    for d in ("down", "right", "up"):
        frames = [crop(f) for f in split_frames(Image.open(raw / f"walk_{d}.png"), 4, 1, 12)]
        stand = max(frames[0].height, frames[2].height)
        rows[d] = [crop(scaled(f, p.walk_h / stand)) for f in frames]
    rows["left"] = [f.transpose(Image.Transpose.FLIP_LEFT_RIGHT) for f in rows["right"]]
    walk = Image.new("RGBA", (WALK_CELL[0] * 4, WALK_CELL[1] * 4), (0, 0, 0, 0))
    for r, d in enumerate(("down", "left", "right", "up")):
        for i, f in enumerate(rows[d]):
            place(walk, f, WALK_CELL, r * 4 + i, 4, WALK_FOOT)
    battle_frames = [crop(f) for f in split_frames(Image.open(raw / "battle.png"), 8, 2, BATTLE_JOIN)]
    factor = p.battle_h / battle_frames[0].height
    battle = Image.new("RGBA", (BATTLE_CELL[0] * 4, BATTLE_CELL[1] * 2), (0, 0, 0, 0))
    for i, f in enumerate(battle_frames):
        place(battle, crop(scaled(f, factor)), BATTLE_CELL, i, 4, BATTLE_FOOT)
    out_dir = src / p.src_dir
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = [out_dir / f"{p.prefix}_walk16.png", out_dir / f"{p.prefix}_battle8.png"]
    limit_palette(walk, p.colors).save(paths[0])
    limit_palette(battle, p.colors).save(paths[1])
    if p.busts:
        faces = [crop(f) for f in split_frames(Image.open(raw / "busts.png"), 4, 1, 12)]
        factor = min(BUST_H / max(f.height for f in faces), BUST_W / max(f.width for f in faces))
        small = [crop(scaled(f, factor)) for f in faces]
        small[3] = match_hurt_skin(small[3], small[:3])
        # 四张脸共用一个色板（头像比小人细，色板放宽一倍）
        row = Image.new("RGBA", (sum(f.width for f in small), max(f.height for f in small)), (0, 0, 0, 0))
        x = 0
        for f in small:
            row.alpha_composite(f, (x, row.height - f.height))
            x += f.width
        row = limit_palette(row, p.colors * 2)
        x = 0
        for i, f in enumerate(small):
            path = out_dir / f"{p.prefix}_bust_{i}.png"
            crop(row.crop((x, 0, x + f.width, row.height))).save(path)
            paths.append(path)
            x += f.width
    return paths


def build_enemy(src: Path, e: EnemyRaw) -> list[Path]:
    frames = [crop(f) for f in split_frames(Image.open(src / "raw" / e.eid / "acts.png"), 4, 1, e.join)]
    factor = e.idle_w / frames[0].width
    small = [crop(scaled(f, factor)) for f in frames]
    # 四个动作共用一个色板：拼成一行取色，再拆回去
    widths = [f.width for f in small]
    row = Image.new("RGBA", (sum(widths), max(f.height for f in small)), (0, 0, 0, 0))
    x = 0
    for f in small:
        row.alpha_composite(f, (x, 0))
        x += f.width
    row = limit_palette(row, e.colors)
    out_dir = src / "bosses-final"
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = []
    x = 0
    for i, f in enumerate(small):
        part = crop(row.crop((x, 0, x + f.width, row.height)))
        x += f.width
        path = out_dir / f"{e.prefix}_act{i}.png"
        part.save(path)
        paths.append(path)
    return paths


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--src", type=Path, default=None)
    ap.add_argument("--only", default="", help="只做这一个 id")
    args = ap.parse_args(argv)
    src = args.src or default_src()
    done = []
    for p in PEOPLE:
        if not args.only or args.only == p.pid:
            done += build_person(src, p)
    for e in ENEMIES:
        if not args.only or args.only == e.eid:
            done += build_enemy(src, e)
    for path in done:
        print(path.relative_to(src).as_posix())
    print(f"HD_FRAMES_WRITTEN：{len(done)} 个文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
