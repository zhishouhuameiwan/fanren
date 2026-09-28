"""烘焙图预览：below + 人物占位 + above，×3 放大，粗略叠一层时辰光照。

    python tools/artgen/preview.py ch01_hanjiacun
    python tools/artgen/preview.py ch05_mofu --time night --out x.png
    python tools/artgen/preview.py ch01_qingniuzhen --crop 10 0 40 20 --collision

默认现烘一份（不写 assets/，改完生成器直接看）；--from-assets 则读 assets/art/maps/ 里的。
人物：地图上每个 npc 与默认出生点各站一个。assets/art/sprites/index.json 若已有人物那一路
的产物，就用真精灵（取每张图第一帧）；没有就画 16×24 的占位小人。

光照只是「看个大概」：环境色相乘 + 每盏灯一团径向加光 + 暗角。真正的光照、辉光、
景深在运行时（src/engine/PostFx）。
"""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import numpy as np  # noqa: E402
from PIL import Image  # noqa: E402

import mapbake  # noqa: E402
import maplights  # noqa: E402
import maptmj  # noqa: E402
from palette import c  # noqa: E402

ROOT = _HERE.parents[1]
T = 16
DEFAULT_OUT = Path(tempfile.gettempdir()) / "fanren_artgen_preview"


def placeholder_figure(seed: int, player: bool) -> np.ndarray:
    """16×24 的占位小人（RGBA）：发、脸、衣、腰带、鞋，衣色按 seed 轮换。"""
    robes = ["water2", "red2", "jade1", "violet2", "soil3", "ink4", "wood3"]
    robe = c("jade2") if player else c(robes[seed % len(robes)])
    dark = tuple(int(v * 0.65) for v in robe)
    hair, skin, shoe, belt = c("ink1"), c("skin2"), c("ink1"), c("gold1")
    im = np.zeros((24, 16, 4), np.uint8)

    def put(x0, y0, x1, y1, col):
        im[y0:y1, x0:x1, :3] = col
        im[y0:y1, x0:x1, 3] = 255

    put(5, 2, 11, 9, skin)
    put(5, 1, 11, 4, hair)
    put(4, 2, 5, 6, hair)
    put(11, 2, 12, 6, hair)
    put(7, 0, 9, 2, hair)
    put(4, 9, 12, 20, robe)
    put(3, 10, 4, 17, dark)
    put(12, 10, 13, 17, dark)
    put(10, 9, 12, 20, dark)
    put(4, 14, 12, 15, belt)
    put(5, 20, 7, 23, shoe)
    put(9, 20, 11, 23, shoe)
    put(6, 5, 7, 6, c("ink0"))
    put(9, 5, 10, 6, c("ink0"))
    # 描边：透明像素挨着实心的补一圈深色
    a = im[..., 3] > 0
    ring = np.zeros_like(a)
    for dy, dx in ((1, 0), (-1, 0), (0, 1), (0, -1)):
        ring |= np.roll(np.roll(a, dy, 0), dx, 1)
    ring &= ~a
    im[ring, :3] = c("ink0")
    im[ring, 3] = 255
    return im


def load_sprite_index():
    idx = ROOT / "assets" / "art" / "sprites" / "index.json"
    if not idx.exists():
        return None
    return json.loads(idx.read_text(encoding="utf-8"))


def sprite_for(index, role_id: str):
    """人物那一路的 index.json 只认两种写法：{role: {"image": 相对路径, "frame": [w,h]}} 或
    {"roles": {...}}。认不出就返回 None，退回占位小人——预览不该因为别人的格式挂掉。"""
    if index is None:
        return None
    roles = index.get("roles", index)
    ent = roles.get(role_id) if isinstance(roles, dict) else None
    if not isinstance(ent, dict) or "image" not in ent:
        return None
    path = ROOT / "assets" / "art" / "sprites" / ent["image"]
    if not path.exists():
        return None
    fw, fh = ent.get("frame", [16, 24])
    img = np.asarray(Image.open(path).convert("RGBA"))
    return img[:fh, :fw].copy()


def composite(below: np.ndarray, above: np.ndarray, md, figures: bool) -> np.ndarray:
    img = below.astype(np.float32) / 255.0
    if figures:
        index = load_sprite_index()
        people = []
        for o in md.objects:
            if o.type == "npc":
                people.append((o.y, o.x, o.props.get("role_id", "?"), False))
            elif o.type == "spawn" and o.props.get("default"):
                people.append((o.y, o.x, "han_li", True))
        for i, (y, x, role, player) in enumerate(sorted(people)):
            spr = sprite_for(index, role)
            if spr is None:
                spr = placeholder_figure(sum(map(ord, role)), player)
            h, w = spr.shape[:2]
            px = x * T + (T - w) // 2
            py = y * T + T - h
            # 脚下一团影
            sh_y, sh_x = y * T + T - 3, x * T + 2
            img[sh_y:sh_y + 3, sh_x:sh_x + 12] *= 0.72
            y0, x0 = max(0, py), max(0, px)
            sub = spr[y0 - py:, x0 - px:]
            hh = min(sub.shape[0], img.shape[0] - y0)
            ww = min(sub.shape[1], img.shape[1] - x0)
            a = sub[:hh, :ww, 3:4] / 255.0
            img[y0:y0 + hh, x0:x0 + ww] = img[y0:y0 + hh, x0:x0 + ww] * (1 - a) + sub[:hh, :ww, :3] / 255.0 * a
    a = above[..., 3:4] / 255.0
    img = img * (1 - a) + above[..., :3] / 255.0 * a
    return img


def light_up(img: np.ndarray, meta: dict, time: str) -> np.ndarray:
    h, w = img.shape[:2]
    amb = np.array(meta["ambient"] if time == meta["time"] else maplights.TIME_DEFAULTS[time]["ambient"],
                   np.float32) / 255.0
    lightmap = np.ones((h, w, 3), np.float32) * amb
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    for l in meta["lights"]:
        if time == "day" and l["kind"] not in ("furnace", "save", "fire"):
            continue
        cx, cy, r = l["x"] * T, l["y"] * T, l["r"] * T
        x0, x1 = int(max(0, cx - r)), int(min(w, cx + r + 1))
        y0, y1 = int(max(0, cy - r)), int(min(h, cy + r + 1))
        if x0 >= x1 or y0 >= y1:
            continue
        d = np.sqrt((xx[y0:y1, x0:x1] - cx) ** 2 + (yy[y0:y1, x0:x1] - cy) ** 2)
        f = np.clip(1.0 - d / r, 0, 1) ** 2 * l["intensity"]
        lightmap[y0:y1, x0:x1] += f[..., None] * (np.array(l["color"], np.float32) / 255.0)
    out = img * np.clip(lightmap, 0, 1.6)
    return np.clip(out, 0, 1)


def vignette(img: np.ndarray, strength: float) -> np.ndarray:
    h, w = img.shape[:2]
    yy, xx = np.mgrid[0:h, 0:w].astype(np.float32)
    d = np.sqrt(((xx - w / 2) / (w / 2)) ** 2 + ((yy - h / 2) / (h / 2)) ** 2)
    f = 1.0 - strength * np.clip(d - 0.55, 0, 1) ** 1.5
    return img * f[..., None]


def stage_preview(layers: list[np.ndarray], figures: bool) -> np.ndarray:
    """战斗背景 / 标题：各层按顺序叠起来；战斗背景在站位带上摆几个占位人（敌左我右）。"""
    img = np.zeros(layers[0].shape[:2] + (3,), np.float32)
    for lay in layers:
        a = lay[..., 3:4] / 255.0
        img = img * (1 - a) + lay[..., :3] / 255.0 * a
    if figures:
        for i, (x, y) in enumerate(((70, 128), (96, 142), (60, 150), (230, 126), (252, 140), (238, 152))):
            spr = placeholder_figure(i * 7 + 3, i >= 3)
            if i < 3:
                spr = spr[:, ::-1]
            h, w = spr.shape[:2]
            a = spr[..., 3:4] / 255.0
            img[y - h:y, x - w // 2:x - w // 2 + w] *= 1 - a
            img[y - h:y, x - w // 2:x - w // 2 + w] += spr[..., :3] / 255.0 * a
    return img


def main(argv=None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    if argv and argv[0] in ("--backdrop", "--title"):
        return main_stage(argv)
    ap = argparse.ArgumentParser(description="烘焙图预览（战斗背景：--backdrop <id>；标题：--title）")
    ap.add_argument("map_id")
    ap.add_argument("--time", choices=["day", "dusk", "night", "indoor"])
    ap.add_argument("--out")
    ap.add_argument("--crop", nargs=4, type=int, metavar=("X0", "Y0", "X1", "Y1"), help="格坐标，含左上不含右下")
    ap.add_argument("--scale", type=int, default=3)
    ap.add_argument("--from-assets", action="store_true")
    ap.add_argument("--no-light", action="store_true")
    ap.add_argument("--no-figures", action="store_true")
    ap.add_argument("--collision", action="store_true", help="挡路格叠一层红，查「看着能走其实挡路」")
    args = ap.parse_args(argv)

    md = maptmj.load(args.map_id)
    if args.from_assets:
        d = ROOT / "assets" / "art" / "maps" / args.map_id
        below = np.asarray(Image.open(d / "below.png").convert("RGB"))
        above = np.asarray(Image.open(d / "above.png").convert("RGBA"))
        meta = json.loads((d / "meta.json").read_text(encoding="utf-8"))
    else:
        below, above, meta, _ = mapbake.render(args.map_id, mapbake.load_presets()[args.map_id])
    img = composite(below, above, md, not args.no_figures)
    time = args.time or meta["time"]
    if not args.no_light:
        img = light_up(img, meta, time)
        img = vignette(img, meta["vignette"] * 0.6)
    if args.collision:
        blocked = np.repeat(np.repeat(md.collision != 0, T, 0), T, 1)
        img[blocked] = img[blocked] * 0.6 + np.array([0.9, 0.1, 0.1]) * 0.4
    out = (img * 255 + 0.5).astype(np.uint8)
    if args.crop:
        x0, y0, x1, y1 = args.crop
        out = out[y0 * T:y1 * T, x0 * T:x1 * T]
    im = Image.fromarray(out)
    if args.scale > 1:
        im = im.resize((im.width * args.scale, im.height * args.scale), Image.NEAREST)
    if args.out:
        path = Path(args.out)
    else:
        DEFAULT_OUT.mkdir(parents=True, exist_ok=True)
        path = DEFAULT_OUT / ("%s%s.png" % (args.map_id, "_" + args.time if args.time else ""))
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)
    print(path)
    return 0


def main_stage(argv) -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--backdrop")
    ap.add_argument("--title", action="store_true")
    ap.add_argument("--out")
    ap.add_argument("--scale", type=int, default=3)
    args = ap.parse_args(argv)
    if args.title:
        import title
        layers = title.render()
        order = ["sky", "far", "mid", "near"]
        name = "title"
    else:
        import backdrops
        layers = backdrops.render(args.backdrop)
        order = ["sky", "far", "mid", "ground"]
        name = "battle_" + args.backdrop
    arrs = [layers[k].to_array(True) for k in order]
    img = stage_preview(arrs, figures=not args.title)
    out = (np.clip(img, 0, 1) * 255 + 0.5).astype(np.uint8)
    im = Image.fromarray(out)
    im = im.resize((im.width * args.scale, im.height * args.scale), Image.NEAREST)
    path = Path(args.out) if args.out else DEFAULT_OUT / (name + ".png")
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)
    print(path)
    return 0


if __name__ == "__main__":
    sys.exit(main())
