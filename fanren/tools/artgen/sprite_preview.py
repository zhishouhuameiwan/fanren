"""精灵对照表：把生成物排成几张放大的总览图，给人眼逐张检查。

    python tools/artgen/sprite_preview.py [--art assets/art] [--out <目录>] [--scale 4]

读的是 out_root 里**已经生成**的 PNG 与 index.json（不是重新画一遍），
所以看到的就是要进仓库的东西。缺省输出到协调者的 scratchpad/sprites/。

产出：
  chars_lineup.png     全员正面站立一排（带名字）——区分身份、认韩立就看这张
  chars_walk_<n>.png   每人一行：下/左/右/上 各 3 帧
  chars_battle_<n>.png 每人一行：待机×2、蓄势、出招、施法、受击、倒地、胜利、防御
  chars_context.png    人物放在草地色 / 夜色 / 室内木地板三种底上（×3，接近游戏里的大小）
  enemies.png          非人形敌人与光球的全部帧
  objects.png          地图物件每一帧（带 id）
  fx.png / icons.png   特效贴图与 UI 图标（由 fx.py 生成后才有）
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

from PIL import Image, ImageDraw, ImageFont  # noqa: E402

import palette as P  # noqa: E402

REPO = _HERE.parent.parent
FONT_PATH = REPO / "assets" / "fonts" / "LXGWWenKai-Regular.ttf"
# 八方旅人化这一轮，协调者指定预览写到他的 scratchpad；那个目录只在这一轮存在，
# 不在的机器上就退到系统临时目录（--out 可随时改）
_ROUND_SCRATCH = Path(r"C:\Users\htx-lyh\AppData\Local\Temp\claude\H--Work-Kys\72d03c6b-2b5f-46cb-ae43-1ef00b7360da\scratchpad")
DEFAULT_OUT = (_ROUND_SCRATCH / "sprites") if _ROUND_SCRATCH.is_dir() else Path(tempfile.gettempdir()) / "fanren_sprite_preview"

BG = (70, 76, 72)
CHECK_A = (88, 94, 90)
CHECK_B = (80, 86, 82)
LABEL = (240, 234, 216)


def _font(size: int) -> ImageFont.ImageFont:
    try:
        return ImageFont.truetype(str(FONT_PATH), size)
    except OSError:
        return ImageFont.load_default()


def _checker(w: int, h: int, cell: int) -> Image.Image:
    img = Image.new("RGBA", (w, h), CHECK_A + (255,))
    d = ImageDraw.Draw(img)
    for y in range(0, h, cell):
        for x in range(0, w, cell):
            if ((x // cell) + (y // cell)) % 2:
                d.rectangle([x, y, x + cell - 1, y + cell - 1], fill=CHECK_B + (255,))
    return img


def _up(img: Image.Image, s: int) -> Image.Image:
    return img.resize((img.width * s, img.height * s), Image.NEAREST)


def _frames(sheet: Image.Image, fw: int, fh: int, cols: int, idxs: list[int]) -> list[Image.Image]:
    out = []
    for i in idxs:
        x, y = (i % cols) * fw, (i // cols) * fh
        out.append(sheet.crop((x, y, x + fw, y + fh)))
    return out


def rows_sheet(rows: list[tuple[str, list[Image.Image]]], scale: int, gap: int = 4,
               label_w: int = 260, title: str = "") -> Image.Image:
    """每行：左边一段文字，右边一串帧。"""
    font = _font(18)
    tfont = _font(22)
    cell_w = max((f.width for _, fs in rows for f in fs), default=16) * scale
    cell_h = max((f.height for _, fs in rows for f in fs), default=24) * scale
    ncol = max((len(fs) for _, fs in rows), default=1)
    top = 40 if title else 8
    W = label_w + ncol * (cell_w + gap) + 8
    H = top + len(rows) * (cell_h + gap) + 8
    canvas = Image.new("RGBA", (W, H), BG + (255,))
    d = ImageDraw.Draw(canvas)
    if title:
        d.text((10, 8), title, font=tfont, fill=LABEL + (255,))
    for r, (name, fs) in enumerate(rows):
        y = top + r * (cell_h + gap)
        d.text((10, y + cell_h // 2 - 10), name, font=font, fill=LABEL + (255,))
        for c, f in enumerate(fs):
            x = label_w + c * (cell_w + gap)
            canvas.alpha_composite(_checker(cell_w, cell_h, scale * 4), (x, y))
            big = _up(f, scale)
            canvas.alpha_composite(big, (x + (cell_w - big.width) // 2, y + (cell_h - big.height)))
    return canvas


def grid_sheet(items: list[tuple[str, Image.Image]], scale: int, cols: int, title: str = "",
               bg: tuple[int, int, int] = BG, checker: bool = True) -> Image.Image:
    font = _font(15)
    tfont = _font(22)
    cw = max(im.width for _, im in items) * scale + 12
    ch = max(im.height for _, im in items) * scale + 28
    rows = (len(items) + cols - 1) // cols
    top = 40 if title else 4
    canvas = Image.new("RGBA", (cols * cw + 8, top + rows * ch + 8), bg + (255,))
    d = ImageDraw.Draw(canvas)
    if title:
        d.text((10, 8), title, font=tfont, fill=LABEL + (255,))
    for i, (name, im) in enumerate(items):
        x = 6 + (i % cols) * cw
        y = top + (i // cols) * ch + 22
        big = _up(im, scale)
        if checker:
            canvas.alpha_composite(_checker(big.width, big.height, scale * 4), (x, y))
        canvas.alpha_composite(big, (x, y))
        d.text((x, y - 20), name, font=font, fill=LABEL + (255,))
    return canvas


def preview(art: Path, out: Path, scale: int) -> list[Path]:
    out.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []
    idx_path = art / "sprites" / "index.json"
    index = json.loads(idx_path.read_text(encoding="utf-8"))
    sheets = index["sheets"]
    roles = index["roles"]
    # 外观 → 用到它的角色（给行标签用）
    users: dict[str, list[str]] = {}
    for rid, lid in roles.items():
        users.setdefault(lid, []).append(rid)
    for rid, vs in index.get("role_variants", {}).items():
        for v in vs:
            users.setdefault(v["look"], []).append(f"{rid}≤ch{v.get('max_chapter', '')}")

    order = sorted(sheets, key=lambda k: (k.startswith("arch_"), k))
    lineup: list[tuple[str, Image.Image]] = []
    lineup_ids: list[str] = []
    walk_rows: list[tuple[str, list[Image.Image]]] = []
    battle_rows: list[tuple[str, list[Image.Image]]] = []
    for lid in order:
        e = sheets[lid]
        sheet = Image.open(art / "sprites" / e["file"]).convert("RGBA")
        fw, fh, cols = e["frame_w"], e["frame_h"], e["cols"]
        down = _frames(sheet, fw, fh, cols, e["walk"]["down"])
        lineup.append((e["name"].split("（")[0][:6], down[0]))
        lineup_ids.append(lid)
        walk = []
        for d in ("down", "left", "right", "up"):
            walk += _frames(sheet, fw, fh, cols, e["walk"][d])
        label = f"{e['name'][:12]}\n{lid}"
        walk_rows.append((label, walk))
        b = e["battle"]
        bidx = b["idle"] + b["ready"] + b["attack"] + b["cast"] + b["hurt"] + b["down"] + b["victory"] + b["guard"]
        battle_rows.append((label, _frames(sheet, fw, fh, cols, bidx)))

    img = grid_sheet(lineup, scale, 14, title=f"全员正面（×{scale}）")
    written.append(_save(img, out / "chars_lineup.png"))
    per = 18
    for n in range(0, len(walk_rows), per):
        img = rows_sheet(walk_rows[n:n + per], scale, title=f"行走：下×3 左×3 右×3 上×3（×{scale}）")
        written.append(_save(img, out / f"chars_walk_{n // per + 1}.png"))
        img = rows_sheet(battle_rows[n:n + per], scale,
                         title=f"战斗（面向左）：待机×2 蓄势 出招 施法 受击 倒地 胜利 防御（×{scale}）")
        written.append(_save(img, out / f"chars_battle_{n // per + 1}.png"))

    # 放在三种底色上，×3（与游戏里行走人物的放大倍数一致）
    ctx_items = [x for x, lid in zip(lineup, lineup_ids) if not lid.startswith("arch_")]
    bgs = [("草地（昼）", P.c("leaf3")), ("石板（黄昏）", P.mix("stone2", "violet1", 0.25)), ("夜", P.c("ink2")),
           ("室内木地板", P.c("wood2"))]
    tiles = []
    for name, col in bgs:
        tiles.append(grid_sheet(ctx_items, 3, 18, title=name, bg=col, checker=False))
    W = max(t.width for t in tiles)
    H = sum(t.height for t in tiles)
    ctx = Image.new("RGBA", (W, H), BG + (255,))
    y = 0
    for t in tiles:
        ctx.alpha_composite(t, (0, y))
        y += t.height
    written.append(_save(ctx, out / "chars_context.png"))

    # 敌人
    en_items: list[tuple[str, Image.Image]] = []
    for eid, e in sorted(index["enemies"].items()):
        if "alias_of" in e:
            continue
        sheet = Image.open(art / "sprites" / e["file"]).convert("RGBA")
        fw, fh = e["frame_w"], e["frame_h"]
        names = []
        for k, v in e["battle"].items():
            for i, fi in enumerate(v):
                names.append((fi, f"{e['name']}·{k}{i if len(v) > 1 else ''}"))
        for fi, nm in sorted(names):
            en_items.append((nm, sheet.crop((fi * fw, 0, fi * fw + fw, fh))))
    written.append(_save(grid_sheet(en_items, scale, 6, title=f"非人形敌人（面向右）与元神光球（×{scale}）"),
                         out / "enemies.png"))

    # 物件
    obj = index["objects"]
    osheet = Image.open(art / "sprites" / obj["file"]).convert("RGBA")
    oitems = []
    for oid, e in obj["items"].items():
        for i, (x, y, w, h) in enumerate(e["frames"]):
            oitems.append((f"{e['name']}{i if len(e['frames']) > 1 else ''}", osheet.crop((x, y, x + w, y + h))))
    written.append(_save(grid_sheet(oitems, scale, 8, title=f"地图物件（×{scale}）"), out / "objects.png"))
    written.append(_save(grid_sheet([("objects.png 原图", osheet)], 2, 1, title="物件图集原图（×2）"),
                         out / "objects_atlas.png"))

    # 特效与图标（fx.py 生成后才有）
    fx_idx = art / "fx" / "index.json"
    if fx_idx.exists():
        fx = json.loads(fx_idx.read_text(encoding="utf-8"))
        groups = {
            "fx_soft.png": (("glow", "softcircle", "shaft", "shadow", "fog"), scale, f"柔光类（×{scale}，深底）"),
            "fx_particles.png": (("leaves", "petals", "embers", "rain", "snow", "dust", "firefly", "spark", "shards"),
                                 scale, f"粒子（×{scale}，深底；灰度的运行时乘色）"),
            "fx_combat.png": (("slash", "impact", "fireburst", "wind", "poison", "heal"), scale,
                              f"战斗特效（×{scale}，深底）"),
        }
        for fname, (ids, sc, title) in groups.items():
            fitems = []
            for fid in ids:
                e = fx["textures"][fid]
                im = Image.open(art / "fx" / e["file"]).convert("RGBA")
                fw, fh = e.get("frame_w", im.width), e.get("frame_h", im.height)
                n = e.get("frames", 1)
                for i in range(n):
                    fitems.append((f"{fid}{i if n > 1 else ''}", im.crop((i * fw, 0, i * fw + fw, fh))))
            cols = {"fx_soft.png": 5, "fx_particles.png": 12, "fx_combat.png": 6}[fname]
            written.append(_save(grid_sheet(fitems, sc, cols, title=title, bg=(30, 32, 40), checker=False),
                                 out / fname))
        # 气焰：叠在韩立（战斗待机帧）身后看效果，另给单独一份
        e_b = fx["textures"]["boost_aura"]
        e_x = fx["textures"]["boss_aura"]
        aura_b = Image.open(art / "fx" / e_b["file"]).convert("RGBA")
        aura_x = Image.open(art / "fx" / e_x["file"]).convert("RGBA")
        aw, ah = e_b["frame_w"], e_b["frame_h"]
        foot = e_b["anchor"]
        hero = sheets.get("hanli")
        boss = sheets.get("jia_tianlong")
        aitems = []

        def over(aura_img: Image.Image, who: dict | None, flip: bool) -> Image.Image:
            base = Image.new("RGBA", (aw, ah), (0, 0, 0, 0))
            glow_layer = aura_img
            base.alpha_composite(glow_layer)
            if who is not None:
                sh = Image.open(art / "sprites" / who["file"]).convert("RGBA")
                idx = who["battle"]["idle"][0]
                fr = _frames(sh, who["frame_w"], who["frame_h"], who["cols"], [idx])[0]
                if flip:
                    fr = fr.transpose(Image.FLIP_LEFT_RIGHT)
                x = foot[0] - who["frame_w"] // 2
                y = foot[1] - (who["foot_y"] + 1)
                base.alpha_composite(fr, (x, y))
            return base

        for lv in range(3):
            for i in range(3):
                fr = aura_b.crop((i * aw, lv * ah, i * aw + aw, lv * ah + ah))
                aitems.append((f"蓄劲{lv + 1}档·{i}", fr))
                aitems.append((f"+韩立", over(fr, hero, False)))
        for i in range(3):
            fr = aura_x.crop((i * aw, 0, i * aw + aw, ah))
            aitems.append((f"首领蓄势·{i}", fr))
            aitems.append((f"+贾天龙", over(fr, boss, True)))
        written.append(_save(grid_sheet(aitems, scale, 6, title=f"气焰（×{scale}，深底；画在人物身后，加色）",
                                        bg=(24, 26, 34), checker=False), out / "fx_aura.png"))
    icons_idx = art / "ui" / "icons.json"
    if icons_idx.exists():
        ic = json.loads(icons_idx.read_text(encoding="utf-8"))
        isheet = Image.open(art / "ui" / ic["file"]).convert("RGBA")
        iitems = []
        for iid, e in ic["icons"].items():
            x, y, w, h = e["rect"]
            iitems.append((f"{e['name']}", isheet.crop((x, y, x + w, y + h))))
        written.append(_save(grid_sheet(iitems, scale, 10, title=f"UI 图标（×{scale}，墨色底）", bg=(14, 17, 25),
                                        checker=False),
                             out / "icons.png"))
    return written


# ---------------------------------------------------------------------------
# 覆盖面核对：地图上的 NPC、b03–b05 的战斗单位、主角与同伴，每一个都要有图
# ---------------------------------------------------------------------------

PARTY = ("hanli", "qu_hun", "li_feiyu")


def _walk_role_ids(x: object, out: list[str]) -> None:
    if isinstance(x, dict):
        if "role_id" in x and isinstance(x["role_id"], str):
            out.append(x["role_id"])
        for v in x.values():
            _walk_role_ids(v, out)
    elif isinstance(x, list):
        for v in x:
            _walk_role_ids(v, out)


def coverage(art: Path) -> tuple[str, int]:
    """返回 (markdown 表, 缺图个数)。"""
    index = json.loads((art / "sprites" / "index.json").read_text(encoding="utf-8"))
    roles = index["roles"]
    variants = index.get("role_variants", {})
    enemies = index["enemies"]
    sheets = index["sheets"]
    names = {}
    for p in sorted((REPO / "data" / "roles").glob("*.json")):
        d = json.loads(p.read_text(encoding="utf-8"))
        names[d["id"]] = d.get("name", d["id"])

    where: dict[str, list[str]] = {}
    world_roles: set[str] = set()
    for p in sorted((REPO / "maps").glob("*.tmj")):
        d = json.loads(p.read_text(encoding="utf-8"))
        for layer in d.get("layers", []):
            for o in layer.get("objects", []) or []:
                if (o.get("type") or o.get("class")) != "npc":
                    continue
                props = {pp["name"]: pp.get("value") for pp in o.get("properties", [])}
                rid = props.get("role_id")
                if rid:
                    where.setdefault(rid, []).append(f"地图 {p.stem}")
                    world_roles.add(rid)
    # b0N 是剧情战，be0N 是野外遭遇战（第 3–5 章）
    battle_files = sorted((REPO / "data" / "battles").glob("b0[345]_*.json")) + \
        sorted((REPO / "data" / "battles").glob("be0[345]_*.json"))
    for p in battle_files:
        ids: list[str] = []
        _walk_role_ids(json.loads(p.read_text(encoding="utf-8")), ids)
        for rid in ids:
            where.setdefault(rid, []).append(f"战斗 {p.stem}")
    for rid in PARTY:
        where.setdefault(rid, []).append("主角/同伴")

    lines = ["| role_id | 名字 | 出场（去重） | 行走/战斗图 | 备注 |", "| --- | --- | --- | --- | --- |"]
    missing = 0
    for rid in sorted(where):
        srcs = sorted(set(where[rid]))
        short = "、".join(s.split(" ", 1)[1] if " " in s else s for s in srcs[:4]) + ("…" if len(srcs) > 4 else "")
        kinds = sorted({s.split(" ")[0] for s in srcs})
        look = roles.get(rid)
        note = ""
        if look:
            got = f"`{look}`"
            if look not in sheets:
                got += "（表缺！）"
                missing += 1
            if rid in variants:
                got += "；" + "、".join(f"≤第{v.get('max_chapter')}章 `{v['look']}`" for v in variants[rid])
            if rid in enemies:
                note = f"另有非人形战斗图 `{enemies[rid]['file']}`"
            if look.startswith("arch_"):
                note = (note + "；" if note else "") + "原型兜底"
        elif rid in world_roles:
            got = "**缺世界人物表**"
            note = "战斗 enemies 不能替代 WorldView 的 roles -> sheets"
            missing += 1
        elif rid in enemies:
            got = f"敌人图 `{enemies[rid]['file']}`（{enemies[rid]['kind']}）"
        else:
            got = "**缺**"
            missing += 1
        lines.append(f"| {rid} | {names.get(rid, '?')} | {'/'.join(kinds)}：{short} | {got} | {note} |")
    # 识海之战里韩立换成元神光球
    for bid, ov in sorted(index.get("battle_overrides", {}).items()):
        for rid, eid in sorted(ov.items()):
            ok = eid in enemies
            lines.append(f"| {rid}（{bid}） | {names.get(rid, '?')}（元神） | 战斗：{bid} | "
                         f"{'敌人图 `' + enemies[eid]['file'] + '`' if ok else '**缺**'} | battle_overrides |")
            if not ok:
                missing += 1
    # 全部 78 个角色的归属
    lines.append("")
    lines.append(f"data/roles 共 {len(names)} 个角色；不在上表（不上地图、不进第 3–5 章战斗、不在队伍里）的其余角色归属：")
    rest = []
    for rid in sorted(names):
        if rid in where:
            continue
        if rid in roles:
            rest.append(f"{rid}→{roles[rid]}")
        elif rid in enemies:
            rest.append(f"{rid}→敌人图")
        else:
            rest.append(f"{rid}→（无，运行时退回旧画法）")
    lines.append("、".join(rest))
    return "\n".join(lines), missing


def _save(img: Image.Image, path: Path) -> Path:
    img.save(path)
    return path


def main() -> int:
    ap = argparse.ArgumentParser(description="精灵对照表")
    ap.add_argument("--art", default=str(REPO / "assets" / "art"))
    ap.add_argument("--out", default=str(DEFAULT_OUT))
    ap.add_argument("--scale", type=int, default=4)
    ap.add_argument("--build", action="store_true", help="先重新生成（sprites.build + fx.build）到 --art")
    ap.add_argument("--coverage", action="store_true", help="只打印覆盖面核对表（缺图时退出码 1）")
    a = ap.parse_args()
    art = Path(a.art)
    if a.coverage:
        table, missing = coverage(art)
        print(table)
        print(f"\n缺图 {missing} 个")
        return 1 if missing else 0
    if a.build:
        import sprites

        sprites.build(art)
        try:
            import fx

            fx.build(art)
        except ImportError:
            pass
    files = preview(art, Path(a.out), a.scale)
    for f in files:
        print(f)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
