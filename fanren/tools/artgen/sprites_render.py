"""把一条外观（Look）画成一张人物精灵表。

一帧的画法（先后即遮挡关系）：
  背后配件（behind）→ 手持兵刃（behind，收在身后的刀）→ 身体 → 身上配件（body）
  → 脸 → 胡须 → 头发（戴帽时把帽子罩住的部分裁掉）→ 头饰
  → 身前配件（front）→ 手持兵刃（front）→ 描边。
右向帧不单独画：左向帧画完、描完边后整张水平翻转。

帧表（sheet_layout）：3 列一行，行走四向各占一行（站、迈甲、迈乙），
战斗九帧占三行：待机×2、蓄势、出招、施法、受击、倒地、胜利、防御。
"""

from __future__ import annotations

from PIL import Image

from sprites_body import FAMILIES, BodyFrame, _del_row, surgery
from sprites_core import Canvas, Code, add_outline, grid_sheet, to_image
from sprites_head import BEARDS, FACE_BACK, FACES, HAIRS, HEADWEAR, Part
from sprites_looks import OUTFITS, Look, body_surgery, materials_for
from sprites_props import CARRY, WEAPONS
from sprites_quhun import CLOAK_GIANT

# 身体模板族：两个通用剪影族 + 曲魂那一套手画的巨汉斗篷
BODY_FAMILIES = {**FAMILIES, "cloak_giant": CLOAK_GIANT}

# ---------------------------------------------------------------------------
# 帧表
# ---------------------------------------------------------------------------

SHEET_COLS = 3
WALK_ORDER = ("down", "left", "right", "up")


def sheet_layout() -> dict[str, object]:
    walk = {d: [i * 3 + k for k in range(3)] for i, d in enumerate(WALK_ORDER)}
    b0 = 12
    battle = {
        "idle": [b0, b0 + 1], "ready": [b0 + 2], "attack": [b0 + 3], "cast": [b0 + 4],
        "hurt": [b0 + 5], "down": [b0 + 6], "victory": [b0 + 7], "guard": [b0 + 8],
    }
    return {"walk": walk, "battle": battle, "count": 21}


# ---------------------------------------------------------------------------
# 码表
# ---------------------------------------------------------------------------

def body_codes(look: Look) -> dict[str, Code | None]:
    spec = OUTFITS[look.outfit]
    wide = bool(spec.get("wide"))
    opened = bool(spec.get("open"))
    return {
        "A": ("torso", 3), "B": ("torso", 2), "C": ("torso", 1), "D": ("torso", 0),
        "I": ("sleeve", 3), "J": ("sleeve", 2), "K": ("sleeve", 1), "L": ("sleeve", 0),
        "N": ("lower", 3), "O": ("lower", 2), "P": ("lower", 1), "R": ("lower", 0),
        "E": ("pants", 3), "F": ("pants", 2), "G": ("pants", 1), "H": ("pants", 0),
        "T": ("trim", 2), "t": ("trim", 1),
        "Y": ("belt", 2), "y": ("belt", 1),
        "Q": ("skin", 3), "S": ("skin", 2), "s": ("skin", 1),
        "M": ("shoe", 2), "m": ("shoe", 1),
        "W": ("wrap", 2), "w": ("wrap", 1),
        "x": ("skin", 2) if opened else ("torso", 2),
        "z": ("skin", 1) if opened else ("torso", 1),
        "i": ("sleeve", 3) if wide else None,
        "j": ("sleeve", 2) if wide else None,
        "k": ("sleeve", 1) if wide else None,
    }


HAND_ANCHORS: dict[str, Code] = {"1": ("skin", 2), "2": ("skin", 1)}


def face_codes(look: Look) -> dict[str, Code | None]:
    stubble = look.beard in ("stubble", "full")
    return {
        "Q": ("skin", 3), "S": ("skin", 2), "s": ("skin", 1), "K": ("skin", 0),
        "e": ("eye", 1),
        "k": ("hair", 0),
        "v": ("hair", 0) if look.brows else ("skin", 2),
        "p": ("blush", 2) if look.blush else ("skin", 2),
        "n": ("stubble", 1) if stubble else ("skin", 2),
    }


HAIR_CODES: dict[str, Code] = {
    "h": ("hair", 2), "i": ("hair", 3), "j": ("hair", 1), "k": ("hair", 0),
    "T": ("band", 2), "t": ("band", 1),
    "Q": ("skin", 3), "S": ("skin", 2), "s": ("skin", 1),
}
HAT_CODES: dict[str, Code] = {
    "A": ("hat", 3), "B": ("hat", 2), "C": ("hat", 1), "D": ("hat", 0),
    "E": ("hat2", 3), "F": ("hat2", 2), "G": ("hat2", 1),
    "T": ("hat2", 2), "t": ("hat2", 1),
    "U": ("band", 2), "u": ("band", 1),
    "Z": ("gold", 3), "X": ("jade", 3), "r": ("lacquer", 3),
    "q": ("eye", 1), "w": ("skin", 1),
}
BEARD_CODES: dict[str, Code] = {"h": ("beard", 2), "i": ("beard", 3), "j": ("beard", 1), "k": ("beard", 0)}


def prop_codes(main: str, sub: str) -> dict[str, Code | None]:
    return {
        "A": (main, 3), "B": (main, 2), "C": (main, 1), "D": (main, 0),
        "E": (sub, 3), "F": (sub, 2), "G": (sub, 1),
        "Z": ("gold", 3), "r": ("lacquer", 3), "W": ("paper", 3),
        "+": None,
    }


# ---------------------------------------------------------------------------
# 敞怀：正面帧把交领改成两襟敞开
# ---------------------------------------------------------------------------

_OPEN_SWAP = {
    "BBTTBC": "BTSSTC",
    "BBTBBC": "BTSsTC",
    "BTBBBC": "BTSsTC",
    "TBBBBC": "BTssTC",
}


def _open_front(rows: list[str]) -> list[str]:
    out = []
    for r in rows:
        mid = r[5:11]
        out.append(r[:5] + _OPEN_SWAP.get(mid, mid) + r[11:] if mid in _OPEN_SWAP else r)
    return out


_CLOAK_OVER = str.maketrans({"Y": "B", "y": "C", "N": "A", "O": "B", "P": "C", "R": "D"})
_CLOAK_HEM = str.maketrans({"A": "C", "B": "C", "C": "D", "Y": "C", "y": "D", "N": "C", "O": "C", "P": "D", "R": "D"})


def _cloak(rows: list[str], extra: int) -> list[str]:
    """斗篷：从腰带那一行起往下 extra 行都换成上身（斗篷）色，最后一行压暗当下摆的影子。

    按「腰带所在行」找位置，而不是写死行号——战斗姿势里腰带高低不一，体型手术还会增删行。
    """
    counts = [sum(1 for ch in r if ch in "Yy") for r in rows]
    if not counts or max(counts) == 0:
        return rows
    belt = counts.index(max(counts))
    out = list(rows)
    last = min(len(out) - 1, belt + extra)
    for j in range(belt, last + 1):
        out[j] = out[j].translate(_CLOAK_HEM if j == last else _CLOAK_OVER)
    return out


def _to_back(rows: list[str]) -> list[str]:
    tr = str.maketrans({"T": "B", "t": "C", "x": "B", "z": "C"})
    return [r.translate(tr) for r in rows]


# ---------------------------------------------------------------------------
# 一帧
# ---------------------------------------------------------------------------

def _stamp_part(canvas: Canvas, p: Part, hx: int, hy: int, codes) -> None:
    canvas.stamp(list(p.rows), hx + p.ox, hy + p.oy, codes)


def _belt_anchor(rows: list[str], x0: int, y0: int) -> tuple[tuple[int, int], tuple[int, int]]:
    best_row, best_n = None, 0
    for j, r in enumerate(rows):
        n = sum(1 for ch in r if ch in "Yy")
        if n > best_n:
            best_row, best_n = j, n
    if best_row is None:
        mid_y = y0 + len(rows) // 2
        return (x0 + 5, mid_y), (x0 + 10, mid_y)
    r = rows[best_row]
    xs = [i for i, ch in enumerate(r) if ch in "Yy"]
    return (x0 + xs[0], y0 + best_row), (x0 + xs[-1], y0 + best_row)


def _grip(rows: tuple[str, ...]) -> tuple[int, int]:
    for j, r in enumerate(rows):
        i = r.find("+")
        if i >= 0:
            return i, j
    raise ValueError("兵刃模板缺握持点 '+'")


def _body_rows(look: Look, bf: BodyFrame, view: str, bob: int) -> tuple[list[str], float, int, int]:
    """取身体模板、按朝向/衣型改写、做体型手术。返回 (身体行, 中线 x, 帧宽, 帧高)。"""
    fw, fh, widen, torso_rows, leg_rows = body_surgery(look)
    rows = list(bf.rows)
    if bob:
        rows = ["." * len(rows[0])] + _del_row(rows, bf.torso_row)
    if view == "B":
        rows = _to_back(rows)
    elif view == "F" and OUTFITS[look.outfit].get("open"):
        rows = _open_front(rows)
    if OUTFITS[look.outfit].get("prebuilt"):
        # 预制模板：宽高已经是成品，人物中线就在帧中线上
        return rows, fw / 2.0 - 0.5, fw, fh
    rows, center = surgery(bf, rows, widen, torso_rows, leg_rows, fw)
    cloak = OUTFITS[look.outfit].get("cloak_rows")
    if cloak:
        rows = _cloak(rows, cloak)
    return rows, center, fw, fh


def _carried(look: Look, head_view: str, battle: bool) -> list[tuple[object, object]]:
    """这一帧要画的随身配件：(配件, 摆放)。战斗帧里兵刃改握在手上，手上的家什也放下了。"""
    items = []
    for pid in look.props:
        c = CARRY[pid]
        if battle and (c.weapon is not None or pid in ("fan", "whisk", "lantern", "pole", "hoe", "basket")):
            # 兵刃（含扛在肩上的朴刀）战斗时握在手上，别处那一把不再画
            continue
        for pl in c.views.get(head_view, ()):
            items.append((c, pl))
    return items


def _draw_head(canvas: Canvas, look: Look, hv: str, hx: int, hy: int) -> None:
    """脸 → 胡须 → 头发（帽子罩住的部分裁掉）→ 头饰。"""
    if hv == "B":
        _stamp_part(canvas, FACE_BACK, hx, hy, face_codes(look))
    else:
        _stamp_part(canvas, FACES[look.face][hv], hx, hy, face_codes(look))
        if look.mole:
            mx, my = (7, 8) if hv == "F" else (3, 8)
            canvas.set(hx + mx, hy + my, ("eye", 1))
        if look.beard in BEARDS:
            _stamp_part(canvas, BEARDS[look.beard][hv], hx, hy, BEARD_CODES)
    hat = HEADWEAR.get(look.headwear) if look.headwear != "none" else None
    hair_c = Canvas(canvas.w, canvas.h)
    _stamp_part(hair_c, HAIRS[look.hair][hv], hx, hy, HAIR_CODES)
    if hat is not None and hat.clip > 0:
        for y in range(0, min(canvas.h, hy + hat.clip)):
            hair_c.px[y] = [None] * canvas.w
    canvas.paste(hair_c, 0, 0)
    if hat is not None:
        _stamp_part(canvas, hat.views[hv], hx, hy, HAT_CODES)


def render_frame(look: Look, key: str, view: str, bob: int = 0) -> Canvas:
    """画一帧（不描边、不翻转）。

    key 是身体模板名（F0…L2、stance…guard）；view 是 F/L/B（B 用 F 的模板改背面）。
    bob=1 时把上身压低一行（待机第二帧的呼吸）。
    """
    bf: BodyFrame = BODY_FAMILIES[look.family][key]
    rows, center, fw, fh = _body_rows(look, bf, view, bob)
    body_top = fh - 1 - len(rows)
    codes = body_codes(look)

    # 身体先画到一张临时画布上，顺便取出手的位置
    found: dict[str, list[tuple[int, int]]] = {}
    body = Canvas(fw, fh)
    body.stamp(rows, 0, body_top, codes, anchors=HAND_ANCHORS, found=found)
    hand1 = found.get("1", [(int(center) - 3, body_top + 7)])[0]
    hand2 = found.get("2", [(int(center) + 3, body_top + 7)])[0]
    belt_l, belt_r = _belt_anchor(rows, 0, body_top)

    head_view = bf.head if view != "B" else "B"
    hx = int(round(center - 5.5)) + bf.head_dx
    hy = body_top - 8 + bf.head_dy + (1 if bob else 0)
    anchors = {"head": (hx, hy), "belt_l": belt_l, "belt_r": belt_r, "hand1": hand1, "hand2": hand2}

    battle = key not in ("F0", "F1", "F2", "L0", "L1", "L2")
    carry_items = _carried(look, head_view, battle)
    weapon = WEAPONS.get(look.weapon) if battle else None
    held = weapon.states.get(bf.weapon) if weapon is not None else None
    canvas = Canvas(fw, fh)

    def draw_carry(z: str) -> None:
        for c, pl in carry_items:
            if pl.z == z:
                ax, ay = anchors[pl.anchor]
                canvas.stamp(list(pl.rows), ax + pl.dx, ay + pl.dy, prop_codes(c.main, c.sub))

    def draw_weapon(z: str) -> None:
        if held is not None and held.z == z:
            gx, gy = _grip(held.rows)
            canvas.stamp(list(held.rows), hand1[0] - gx, hand1[1] - gy, prop_codes(weapon.main, weapon.sub))

    draw_carry("behind")
    draw_weapon("behind")
    canvas.paste(body, 0, 0)
    draw_carry("body")
    _draw_head(canvas, look, head_view, hx, hy)
    draw_carry("front")
    draw_weapon("front")
    return canvas


# 行走帧：(身体模板, 朝向)；背面借正面模板，迈步的先后对调
WALK_FRAMES: dict[str, list[tuple[str, str]]] = {
    "down": [("F0", "F"), ("F1", "F"), ("F2", "F")],
    "left": [("L0", "L"), ("L1", "L"), ("L2", "L")],
    "right": [("L0", "L"), ("L1", "L"), ("L2", "L")],
    "up": [("F0", "B"), ("F2", "B"), ("F1", "B")],
}
BATTLE_FRAMES: list[tuple[str, int]] = [
    ("stance", 0), ("stance", 1), ("ready", 0), ("attack", 0), ("cast", 0),
    ("hurt", 0), ("down", 0), ("victory", 0), ("guard", 0),
]


def render_sheet(look: Look) -> tuple[Image.Image, int, int]:
    """整张精灵表。返回 (图, 帧宽, 帧高)。"""
    mats = materials_for(look)
    fw, fh = body_surgery(look)[:2]
    frames: list[Image.Image] = []
    for d in WALK_ORDER:
        for key, view in WALK_FRAMES[d]:
            c = add_outline(render_frame(look, key, view), mats)
            if d == "right":
                c = c.mirrored()
            frames.append(to_image(c, mats))
    for key, bob in BATTLE_FRAMES:
        c = add_outline(render_frame(look, key, "L", bob=bob), mats)
        frames.append(to_image(c, mats))
    return grid_sheet(frames, SHEET_COLS, fw, fh), fw, fh
