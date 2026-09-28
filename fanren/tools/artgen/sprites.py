"""人物精灵、非人形敌人、地图物件的生成入口。

    python tools/artgen/sprites.py [out_root]     # 缺省 out_root = assets/art

被 tools/artgen/artgen.py 调用时走 build(out_root)，--check 时 out_root 是临时目录。
全部确定性：没有随机数、不读时钟、字典一律按键排序输出——同输入同输出，
ARTGEN_IN_SYNC 逐像素比对才有意义。

产物（都在 out_root/sprites/ 下）：
  chars/<look_id>.png        人物表：行走 4 向 ×3 帧 + 战斗 9 帧（帧表见 index.json）
  enemies/<id>.png           非人形敌人与元神光球
  objects.png                地图物件（设施、传送标记、闪光、气泡）
  objects_emissive.png       同一张图里只留发光像素，给辉光目标用
  index.json                 帧表、角色→外观、敌人、物件
输入：data/visual/looks.json（外观表）。
"""

from __future__ import annotations

import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

from PIL import Image  # noqa: E402

from sprites_core import save_png, write_json  # noqa: E402
from sprites_enemies import BEASTS, ORBS, render_beast, render_orb  # noqa: E402
from sprites_looks import Look, load_table  # noqa: E402
from sprites_objects import CELL, pack_objects  # noqa: E402
from sprites_render import SHEET_COLS, render_sheet, sheet_layout  # noqa: E402

REPO = _HERE.parent.parent
LOOKS_JSON = REPO / "data" / "visual" / "looks.json"

# 设施 kind（map_spec 4.6）→ 物件 id
FACILITY_OBJECTS = {
    "alchemy": "facility.alchemy",
    "forge": "facility.forge",
    "talisman": "facility.talisman",
    "formation": "facility.formation",
    "field": "facility.field",
    "meditate": "facility.meditate",
    "shop": "facility.shop",
    "board": "facility.board",
    "save": "facility.save",
}

# 识海之战里韩立不是人形，是光球（b03_shihai_duoshe）
BATTLE_OVERRIDES = {"b03_shihai_duoshe": {"hanli": "hanli_yuanshen"}}

# 第 6 章以后与既有敌人同类的兽/傀，借用现成的图（没有的就不列，运行时退回旧画法）
ENEMY_ALIASES = {"heishajiao_shigui": "mofu_shigui"}


def _opaque_rows(img: Image.Image) -> tuple[int, int]:
    alpha = img.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return (0, img.height - 1)
    return (bbox[1], bbox[3] - 1)


def _sheet_entry(look: Look, fw: int, fh: int, sheet: Image.Image) -> dict:
    lay = sheet_layout()
    first = sheet.crop((0, 0, fw, fh))
    top, bottom = _opaque_rows(first)
    return {
        "file": f"chars/{look.id}.png",
        "name": look.name,
        "frame_w": fw,
        "frame_h": fh,
        "cols": SHEET_COLS,
        "walk": lay["walk"],
        "battle": lay["battle"],
        "battle_facing": "left",
        "foot_y": bottom,
        "head_y": top,
        "weapon": look.weapon,
    }


def build(out_root: Path) -> list[Path]:
    """生成全部精灵到 out_root/sprites/，返回写出的文件列表（按路径排序）。"""
    out_root = Path(out_root)
    sp = out_root / "sprites"
    written: list[Path] = []
    table = load_table(LOOKS_JSON)

    sheets: dict[str, dict] = {}
    all_looks: dict[str, Look] = {**table.archetypes, **table.looks}
    for lid in sorted(all_looks):
        look = all_looks[lid]
        img, fw, fh = render_sheet(look)
        written.append(save_png(img, sp / "chars" / f"{lid}.png"))
        sheets[lid] = _sheet_entry(look, fw, fh, img)

    enemies: dict[str, dict] = {}
    for beast in BEASTS:
        img, lay = render_beast(beast)
        written.append(save_png(img, sp / "enemies" / f"{beast.id}.png"))
        first = img.crop((0, 0, beast.w, beast.h))
        top, bottom = _opaque_rows(first)
        enemies[beast.id] = {
            "file": f"enemies/{beast.id}.png", "name": beast.name, "kind": "beast",
            "frame_w": beast.w, "frame_h": beast.h, "cols": img.width // beast.w,
            "battle": lay, "battle_facing": "right", "foot_y": bottom, "head_y": top,
            "note": beast.note,
        }
    for orb in ORBS:
        img, lay = render_orb(orb)
        written.append(save_png(img, sp / "enemies" / f"{orb.id}.png"))
        top, bottom = _opaque_rows(img.crop((0, 0, orb.size, orb.size)))
        enemies[orb.id] = {
            "file": f"enemies/{orb.id}.png", "name": orb.name, "kind": "orb",
            "frame_w": orb.size, "frame_h": orb.size, "cols": img.width // orb.size,
            "battle": lay, "battle_facing": "none", "foot_y": bottom, "head_y": top,
            "float": True, "note": orb.note,
        }
    for alias, target in sorted(ENEMY_ALIASES.items()):
        enemies[alias] = {**enemies[target], "alias_of": target}

    obj_img, obj_em, obj_index = pack_objects()
    written.append(save_png(obj_img, sp / "objects.png"))
    written.append(save_png(obj_em, sp / "objects_emissive.png"))

    roles: dict[str, str] = {}
    variants: dict[str, list[dict]] = {}
    for rid in sorted(table.roles):
        entry = table.roles[rid]
        roles[rid] = entry["look"]
        if entry.get("variants"):
            variants[rid] = [dict(v) for v in entry["variants"]]

    index = {
        "_说明": "由 tools/artgen/sprites.py 生成，勿手改。schema 见 docs/art-sprites.md。",
        "version": 1,
        "frame_w": 16,
        "frame_h": 24,
        "anchor": "bottom-center",
        "walk_cycle": [0, 1, 0, 2],
        "walk_fps": 8,
        "sheets": sheets,
        "roles": roles,
        "role_variants": variants,
        "enemies": enemies,
        "battle_overrides": BATTLE_OVERRIDES,
        "objects": {
            "file": "objects.png",
            "emissive": "objects_emissive.png",
            "cell": CELL,
            "items": obj_index,
        },
        "facilities": FACILITY_OBJECTS,
    }
    written.append(write_json(index, sp / "index.json"))
    return sorted(written)


def main(argv: list[str]) -> int:
    out = Path(argv[1]) if len(argv) > 1 else REPO / "assets" / "art"
    files = build(out)
    print(f"sprites: 写出 {len(files)} 个文件到 {out / 'sprites'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
