"""结构推断：从 9 种瓦片语义推出「屋、院墙、崖、树、水、篱笆、家具、门洞」。

地图是生成器摆出来的（tools/mapgen/genmaps*.py），摆法有固定套路，推断就贴着这些套路：

  * 树：tree() 摆一格树干（building=5）+ 3×3 树冠（front=4）。韩家村那几棵树干在树冠
    底行正中，其余各章在树冠正中——两种都认：看哪个 3×3 框里的 front 格更多。
  * 屋：solid_rect(…, 5) 一整块 + 北侧一行 front（屋檐）。hall() 是空心的一圈墙 +
    门洞 + 北侧一行 front。镇子是「先铺满民居再挖街」，于是周边墙体又厚又大。
  * 院墙 / 谷壁 / 崖：border() 一圈，外加各种一两格厚的隔墙；山路图整张先 fill 成墙再挖路。
  * 水面：ground=3 + collision=1 + building=0（「水面不画墙，只挡路」）。
  * gid 8：连成线的是篱笆/石栏，成块的是家具/摊位，单格的是摊架/石灯/兵器架。
  * front 在墙北一行＝屋脊那一段（压在走到屋后的人身上）；在墙南一行＝檐口伸到路上；
    在岩壁上＝崖檐、松枝。

推断不了的（「这块 2×1 是石碾还是兔笼」）由 data/visual/maps.json 的 props 覆写，
不去猜脚本名字。
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass, field

import numpy as np

import pix
from maptmj import MapData
from mapthemes import THEMES

SURFACES = [
    "none", "grass", "earth", "sand", "gravel", "dirt_road", "stone_road", "flagstone", "steps",
    "plank", "tilled", "scree", "scorched", "wood_floor", "stone_floor", "tile_floor", "carpet",
    "cave_floor", "cave_path", "water", "courtyard",
]
S = {name: i for i, name in enumerate(SURFACES)}


@dataclass
class Tree:
    x: int
    y: int
    bx0: int
    by0: int
    bx1: int
    by1: int
    species: str


@dataclass
class Structure:
    sid: int
    kind: str          # rock / wall / roofed
    style: str
    mask: np.ndarray   # bool[H,W]
    lots: np.ndarray | None = None   # int[H,W]，roofed 用；-1 表示不属本结构
    compound: bool = False           # roofed 的外圈画成院墙而不是屋立面
    wall_style: str = ""             # compound 时外圈院墙的做法


@dataclass
class Prop:
    kind: str
    x: int
    y: int
    w: int
    h: int
    blocked: bool = True
    extra: dict = field(default_factory=dict)


@dataclass
class EaveRun:
    y: int
    x0: int
    x1: int
    side: str        # north（屋脊那一段，压在屋后）/ south（檐口伸到路上）
    sid: int         # 所属结构，-1 表示无
    pid: int = -1    # 压在道具（床帐、书架顶、亭子）上时是那件道具


@dataclass
class Gap:
    cells: list
    orient: str      # h：横墙上的门洞（南北向穿过）/ v：竖墙上的门洞（东西向穿过）
    depth: int       # 墙厚


@dataclass
class Scene:
    md: MapData
    preset: dict
    theme: str
    th: dict
    season: str
    time: str
    W: int
    H: int
    walk: np.ndarray
    wall: np.ndarray
    fence: np.ndarray
    water: np.ndarray
    surface: np.ndarray
    trees: list
    trunk: np.ndarray
    canopy: np.ndarray
    structs: list
    sid: np.ndarray
    props: list
    prop_at: np.ndarray
    eaves: list
    walldeco: list
    boughs: list
    gaps: list
    gap_at: np.ndarray
    dense6: np.ndarray
    dense7: np.ndarray
    rooms: np.ndarray


# ---------------------------------------------------------------------------
# 小工具
# ---------------------------------------------------------------------------
N4 = ((1, 0), (-1, 0), (0, 1), (0, -1))


def label4(mask: np.ndarray) -> tuple[np.ndarray, int]:
    """4 邻接连通块标号（小图，纯 Python 足够快）。"""
    h, w = mask.shape
    lab = np.full((h, w), -1, np.int32)
    n = 0
    for y in range(h):
        for x in range(w):
            if not mask[y, x] or lab[y, x] >= 0:
                continue
            q = deque([(x, y)])
            lab[y, x] = n
            while q:
                cx, cy = q.popleft()
                for dx, dy in N4:
                    nx, ny = cx + dx, cy + dy
                    if 0 <= nx < w and 0 <= ny < h and mask[ny, nx] and lab[ny, nx] < 0:
                        lab[ny, nx] = n
                        q.append((nx, ny))
            n += 1
    return lab, n


def runs(mask: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
    """每格所在横向连续段与纵向连续段的长度（不在 mask 里的格为 0）。"""
    h, w = mask.shape
    hr = np.zeros((h, w), np.int32)
    vr = np.zeros((h, w), np.int32)
    for y in range(h):
        x = 0
        while x < w:
            if not mask[y, x]:
                x += 1
                continue
            x1 = x
            while x1 + 1 < w and mask[y, x1 + 1]:
                x1 += 1
            hr[y, x:x1 + 1] = x1 - x + 1
            x = x1 + 1
    for x in range(w):
        y = 0
        while y < h:
            if not mask[y, x]:
                y += 1
                continue
            y1 = y
            while y1 + 1 < h and mask[y1 + 1, x]:
                y1 += 1
            vr[y:y1 + 1, x] = y1 - y + 1
            y = y1 + 1
    return hr, vr


def at(a: np.ndarray, x: int, y: int, default=False):
    h, w = a.shape
    if 0 <= x < w and 0 <= y < h:
        return a[y, x]
    return default


def nb(a: np.ndarray, dx: int, dy: int, fill=False) -> np.ndarray:
    """每格取 (x+dx, y+dy) 那一格的值；出图外的填 fill（不回绕）。"""
    return pix.shift(a, -dx, -dy, fill)


def count8(a: np.ndarray) -> np.ndarray:
    return sum(nb(a, dx, dy).astype(np.int32) for dy in (-1, 0, 1) for dx in (-1, 0, 1))


def rect_mask(h: int, w: int, x0: int, y0: int, x1: int, y1: int) -> np.ndarray:
    m = np.zeros((h, w), bool)
    m[max(0, y0):min(h, y1 + 1), max(0, x0):min(w, x1 + 1)] = True
    return m


# ---------------------------------------------------------------------------
# 树
# ---------------------------------------------------------------------------
def find_trees(md: MapData, wallish: np.ndarray, water: np.ndarray, th: dict, season: str,
               overrides: dict) -> tuple[list, np.ndarray]:
    """树干：一格带 front 的墙，左右与北邻都不是墙（南邻可以是墙——南城街南那棵树
    的树干就贴着后院墙根），且周围 3×3 里 front 占满大半。"""
    H, W = wallish.shape
    front = md.front == 4
    trunk = np.zeros((H, W), bool)
    trees = []
    near_water = pix.dilate(water, 3)
    pool = th["trees"].get(season) or th["trees"]["summer"]
    for y in range(H):
        for x in range(W):
            if not (wallish[y, x] and front[y, x]):
                continue
            # 左右最多贴一边的墙（神手谷东头那棵树的树干贴着谷壁），北邻不能是墙
            if (at(wallish, x - 1, y) and at(wallish, x + 1, y)) or at(wallish, x, y - 1):
                continue
            best = None
            for by0 in (y - 1, y - 2):
                cnt = int(front[max(0, by0):by0 + 3, max(0, x - 1):x + 2].sum())
                if best is None or cnt > best[0]:
                    best = (cnt, by0)
            if best[0] < 6:
                continue
            by0 = best[1]
            key = "%d,%d" % (x, y)
            if key in overrides:
                species = overrides[key]
            elif near_water[y, x] and "willow" in sum(th["trees"].values(), []):
                species = "willow"
            else:
                species = pool[int(pix.hash01(x, y, pix.salt_of(md.map_id, "tree")) * len(pool))]
            trees.append(Tree(x, y, x - 1, by0, x + 1, by0 + 2, species))
            trunk[y, x] = True
    return trees, trunk


# ---------------------------------------------------------------------------
# 屋顶群的分户（周边厚墙体 → 一户一户的屋顶）
# ---------------------------------------------------------------------------
def split_len(total: int, lo: int, hi: int, salt: int) -> list[int]:
    """把 total 切成若干段，每段在 [lo, hi] 之间，由杂凑决定——同输入同输出。"""
    if total <= hi:
        return [total]
    out = []
    left = total
    i = 0
    while left > hi:
        span = lo + int(pix.hash01(i, total, salt) * (hi - lo + 1))
        if left - span < lo:
            span = left - lo
        out.append(span)
        left -= span
        i += 1
    out.append(left)
    return out


def roofscape_lots(md: MapData, mask: np.ndarray, walk: np.ndarray, forced: list, big: bool = False) -> np.ndarray:
    """给一大片实心墙体分户。

    先沿「立面」（南邻可走的墙格）起屋：每段立面按 4–6 格宽切成几户，每户向北延伸
    4–5 格——沿街沿巷的房子都有正脸。剩下没被认领的内部格再按一张错位网格切块，
    那是街巷看不见的后排屋顶。forced 里的矩形（maps.json 指定「这是一栋」）先认领。
    big（府邸、山庄的院墙里）：一户 6–9 格宽、5–6 格深——大宅是几座大屋顶，不是一片小民居。
    """
    H, W = mask.shape
    # 立面上挖进去的门龛（墨府的大门：临街那面墙上两格）先当作墙分户，不然门洞两边、
    # 门洞上方会被分进三户，门就画不成一道门了。门龛格照样带着户号，画立面时认得出来。
    notch = np.zeros((H, W), bool)
    for y in range(1, H - 1):
        x = 0
        while x < W:
            if mask[y, x] or not walk[y, x]:
                x += 1
                continue
            x1 = x
            while x1 + 1 < W and walk[y, x1 + 1] and not mask[y, x1 + 1]:
                x1 += 1
            if (x1 - x + 1 <= 2 and x > 0 and x1 + 1 < W and mask[y, x - 1] and mask[y, x1 + 1]
                    and mask[y - 1, x:x1 + 1].all() and walk[y + 1, x:x1 + 1].all()):
                notch[y, x:x1 + 1] = True
            x = x1 + 1
    mask = mask | notch
    lots = np.full((H, W), -1, np.int32)
    nid = 0
    for (x0, y0, x1, y1) in forced:
        sub = mask & rect_mask(H, W, x0, y0, x1, y1) & (lots < 0)
        if sub.any():
            lots[sub] = nid
            nid += 1
    salt = pix.salt_of(md.map_id, "lots")
    south_open = mask & ~np.vstack([mask[1:], np.ones((1, W), bool)]) & np.vstack([walk[1:], np.zeros((1, W), bool)])
    for y in range(H - 1, -1, -1):
        x = 0
        while x < W:
            if not (south_open[y, x] and lots[y, x] < 0):
                x += 1
                continue
            x1 = x
            while x1 + 1 < W and south_open[y, x1 + 1] and lots[y, x1 + 1] < 0:
                x1 += 1
            cx = x
            lo, hi = (6, 9) if big else (4, 6)
            for span in split_len(x1 - x + 1, lo, hi, salt + y):
                depth = (5 if big else 4) + int(pix.hash01(cx, y, salt + 7) * 2)
                for xx in range(cx, cx + span):
                    yy = y
                    while yy >= 0 and y - yy < depth and mask[yy, xx] and lots[yy, xx] < 0:
                        lots[yy, xx] = nid
                        yy -= 1
                nid += 1
                cx += span
            x = x1 + 1
    # 内部：错位网格
    rest = mask & (lots < 0)
    if rest.any():
        band_y = 0
        b = 0
        while band_y < H:
            bh = (5 if big else 4) + int(pix.hash01(b, 0, salt + 11) * 2)
            off = int(pix.hash01(b, 1, salt + 13) * 5)
            col = -off
            k = 0
            while col < W:
                bw = (6 if big else 4) + int(pix.hash01(b, k, salt + 17) * (4 if big else 3))
                cell = rest & rect_mask(H, W, col, band_y, col + bw - 1, band_y + bh - 1)
                if cell.any():
                    lab, n = label4(cell)
                    for i in range(n):
                        lots[lab == i] = nid
                        nid += 1
                col += bw
                k += 1
            band_y += bh
            b += 1
    return lots


# ---------------------------------------------------------------------------
# gid 8 → 道具
# ---------------------------------------------------------------------------
def fence_props(md: MapData, fence: np.ndarray, water: np.ndarray, wall: np.ndarray, th: dict,
                overrides: dict) -> tuple[list, np.ndarray]:
    H, W = fence.shape
    lab, n = label4(fence)
    props = []
    prop_at = np.full((H, W), -1, np.int32)
    near_water = pix.dilate(water, 1)
    for i in range(n):
        ys, xs = np.nonzero(lab == i)
        x0, x1, y0, y1 = int(xs.min()), int(xs.max()), int(ys.min()), int(ys.max())
        w, h = x1 - x0 + 1, y1 - y0 + 1
        cells = len(xs)
        filled = cells == w * h
        key = "%d,%d" % (x0, y0)
        extra = {}
        if key in overrides:
            ov = overrides[key]
            kind = ov["kind"]
            extra = {k: v for k, v in ov.items() if k not in ("at", "kind")}
        elif cells == 1:
            kind = th["fence_single"]
        elif not filled or ((w == 1 or h == 1) and max(w, h) >= 3):
            # 线、L、U、整圈（村子外圈篱笆、药圃围栏、演武台石栏）都走这里，逐格按邻接画
            kind = th["fence_line"]
            if kind == "stall_row" and near_water[y0:y1 + 1, x0:x1 + 1].any():
                kind = "railing"
            if kind == "counter" and w == 1:
                kind = "bookshelf"
            if kind == "stall_row" and not filled:
                kind = "railing"
        elif filled and w >= 3 and h >= 3 and w * h >= 16:
            kind = "block_building"
        else:
            kind = th["fence_block"]
        props.append(Prop(kind, x0, y0, w, h, True, extra))
        prop_at[lab == i] = len(props) - 1
    return props, prop_at


# ---------------------------------------------------------------------------
# 门洞
# ---------------------------------------------------------------------------
def find_gaps(walk: np.ndarray, wallish: np.ndarray, fence: np.ndarray) -> tuple[list, np.ndarray]:
    """墙上的门洞：一段长 ≤3 的可走格，两端是同一种实体（墙对墙、篱笆对篱笆），
    且那两端的墙在穿过方向上是薄的（≤2 格）——不然就是两栋屋之间的巷子，不是门。"""
    H, W = walk.shape
    # 篱笆只有连成线的才算「墙」（村口篱笆门）；桌子与桌子之间的过道不是门
    line_fence = fence & (np.maximum(*runs(fence)) >= 3)
    solid = wallish | line_fence
    hr, vr = runs(solid)
    gap_at = np.full((H, W), -1, np.int32)
    gaps = []

    def same_kind(ax, ay, bx, by) -> bool:
        return bool(at(wallish, ax, ay) == at(wallish, bx, by))

    for y in range(H):
        x = 0
        while x < W:
            if not walk[y, x]:
                x += 1
                continue
            x1 = x
            while x1 + 1 < W and walk[y, x1 + 1]:
                x1 += 1
            L = x1 - x + 1
            ends = L <= 3 and at(solid, x - 1, y) and at(solid, x1 + 1, y) and same_kind(x - 1, y, x1 + 1, y)
            span = range(x, x1 + 1)
            through = (ends and vr[y, x - 1] <= 2 and vr[y, x1 + 1] <= 2
                       and all(at(walk, xx, y - 1) or at(walk, xx, y + 1) for xx in span))
            # 门龛：厚屋的南墙上挖进去一格（神手谷堂屋通密室那一格）——上面是墙、下面是路
            notch = (ends and L <= 2 and at(wallish, x - 1, y) and at(wallish, x1 + 1, y)
                     and all(at(wallish, xx, y - 1) for xx in span) and all(at(walk, xx, y + 1) for xx in span))
            if through or notch:
                cells = [(xx, y) for xx in span]
                gaps.append(Gap(cells, "h", 1))
                for cx, cy in cells:
                    gap_at[cy, cx] = len(gaps) - 1
            x = x1 + 1
    for x in range(W):
        y = 0
        while y < H:
            if not walk[y, x]:
                y += 1
                continue
            y1 = y
            while y1 + 1 < H and walk[y1 + 1, x]:
                y1 += 1
            L = y1 - y + 1
            if (L <= 3 and at(solid, x, y - 1) and at(solid, x, y1 + 1)
                    and hr[y - 1, x] <= 2 and hr[y1 + 1, x] <= 2 and same_kind(x, y - 1, x, y1 + 1)):
                ok = all(at(walk, x - 1, yy) or at(walk, x + 1, yy) for yy in range(y, y1 + 1))
                if ok and all(gap_at[yy, x] < 0 for yy in range(y, y1 + 1)):
                    cells = [(x, yy) for yy in range(y, y1 + 1)]
                    gaps.append(Gap(cells, "v", 1))
                    for cx, cy in cells:
                        gap_at[cy, cx] = len(gaps) - 1
            y = y1 + 1
    return gaps, gap_at


def find_rooms(walk: np.ndarray, solid: np.ndarray, gap_at: np.ndarray, front: np.ndarray,
               indoor: bool) -> np.ndarray:
    """hall() 掏出来的屋内：去掉门洞后被墙整圈围住、北墙外有一行屋檐的可走块。"""
    H, W = walk.shape
    inner = walk & (gap_at < 0)
    lab, n = label4(inner)
    rooms = np.zeros((H, W), bool)
    for i in range(n):
        m = lab == i
        ys, xs = np.nonzero(m)
        x0, x1, y0, y1 = xs.min(), xs.max(), ys.min(), ys.max()
        if x0 == 0 or y0 == 0 or x1 == W - 1 or y1 == H - 1:
            continue
        if m.sum() > 0.35 * walk.sum():
            continue
        ring_ok = True
        for xx in range(x0 - 1, x1 + 2):
            for yy in (y0 - 1, y1 + 1):
                if not (solid[yy, xx] or gap_at[yy, xx] >= 0):
                    ring_ok = False
        for yy in range(y0 - 1, y1 + 2):
            for xx in (x0 - 1, x1 + 1):
                if not (solid[yy, xx] or gap_at[yy, xx] >= 0):
                    ring_ok = False
        if not ring_ok:
            continue
        # 包围盒里不能还有别的可走块（那是院子里套院子，不是一间屋）
        box = np.zeros((H, W), bool)
        box[y0:y1 + 1, x0:x1 + 1] = True
        if (walk & box & ~m & (gap_at < 0)).any():
            continue
        eave = y0 - 2 >= 0 and front[y0 - 2, x0:x1 + 1].mean() >= 0.6
        if eave or indoor:
            rooms |= m
    return rooms


# ---------------------------------------------------------------------------
# 主入口
# ---------------------------------------------------------------------------
def infer(md: MapData, preset: dict) -> Scene:
    theme = preset["theme"]
    th = THEMES[theme]
    season = preset.get("season", "summer")
    H, W = md.h, md.w
    G, O, B, F, C = md.ground, md.overlay, md.building, md.front, md.collision
    walk = C == 0
    water = (G == 3) & (C != 0) & (B == 0)
    implicit = (C != 0) & (B == 0) & (G != 3)
    wallish = (B == 5) | implicit
    fence = B == 8
    front = F == 4

    prop_over = {"%d,%d" % tuple(p["at"]): p for p in preset.get("props", [])}
    tree_over = {"%d,%d" % tuple(t["at"]): t["species"] for t in preset.get("trees", [])}

    # gid 8 的大块（整间铺子）并进墙体：它是一栋屋，不是一张桌子
    fence_props_list, fence_prop_at = fence_props(md, fence, water, wallish, th, prop_over)
    for p in fence_props_list:
        if p.kind == "block_building":
            wallish[p.y:p.y + p.h, p.x:p.x + p.w] = True
            fence[p.y:p.y + p.h, p.x:p.x + p.w] = False
    props = [p for p in fence_props_list if p.kind != "block_building"]
    prop_at = np.full((H, W), -1, np.int32)
    for i, p in enumerate(props):
        sub = fence[p.y:p.y + p.h, p.x:p.x + p.w]
        prop_at[p.y:p.y + p.h, p.x:p.x + p.w][sub] = i

    trees, trunk = find_trees(md, wallish, water, th, season, tree_over)
    canopy = np.zeros((H, W), bool)
    for t in trees:
        canopy[max(0, t.by0):t.by1 + 1, max(0, t.bx0):t.bx1 + 1] |= front[max(0, t.by0):t.by1 + 1,
                                                                             max(0, t.bx0):t.bx1 + 1]
    body = wallish & ~trunk

    # ---- 覆写的结构（maps.json 的 structures） ----
    structs: list[Structure] = []
    sid = np.full((H, W), -1, np.int32)
    forced = [tuple(r) for r in preset.get("houses", [])]

    def add_struct(st: Structure) -> None:
        st.sid = len(structs)
        structs.append(st)
        sid[st.mask] = st.sid

    def add_built(m: np.ndarray, wall_style: str, roof_style: str, compound: bool) -> None:
        """周边厚墙体：一两格厚的是院墙，厚处分户成屋顶群。

        「厚」＝落在某个 3×3 全墙的方块里（形态学开运算）。不能用「横竖连续段取短」：
        一圈一格厚的围墙，拐角那一格横竖两段都很长，会被错当成厚墙体。
        """
        thick = m & pix.dilate(pix.erode(m, 1), 1)
        thin = m & ~thick
        if thick.any() and thin.any():
            # 贴在厚墙体上的一两格零碎薄格并回去，免得屋顶群边上冒出半截院墙
            lab_t, n_t = label4(thin)
            grow = pix.dilate(thick, 1)
            for j in range(n_t):
                piece = lab_t == j
                if piece.sum() <= 2 and (piece & grow).any():
                    thick |= piece
                    thin &= ~piece
        if thin.any():
            add_struct(Structure(0, "wall", wall_style, thin))
        if thick.any():
            st = Structure(0, "roofed", roof_style, thick, compound=compound, wall_style=wall_style)
            st.lots = roofscape_lots(md, thick, walk, forced, big=compound)
            add_struct(st)

    for so in preset.get("structures", []):
        x0, y0, x1, y1 = so["rect"]
        m = body & rect_mask(H, W, x0, y0, x1, y1) & (sid < 0)
        if not m.any():
            continue
        kind = so["kind"]
        body &= ~m
        if kind in ("rockery", "canopy_bed"):
            ys, xs = np.nonzero(m)
            props.append(Prop(kind, int(xs.min()), int(ys.min()), int(xs.max() - xs.min() + 1),
                              int(ys.max() - ys.min() + 1), True))
            prop_at[m] = len(props) - 1
        elif kind == "rock":
            add_struct(Structure(0, "rock", so["style"], m))
        elif kind == "wall":
            add_struct(Structure(0, "wall", so["style"], m))
        elif kind == "built":
            add_built(m, so["wall"], so["roof"], bool(so.get("compound", False)))
        else:
            raise ValueError("%s: structures 里不认识的 kind %r" % (md.map_id, kind))

    lab, n = label4(body)
    hr, vr = runs(body)
    for i in range(n):
        m = lab == i
        ys, xs = np.nonzero(m)
        x0, x1, y0, y1 = int(xs.min()), int(xs.max()), int(ys.min()), int(ys.max())
        touches = x0 == 0 or y0 == 0 or x1 == W - 1 or y1 == H - 1
        bw, bh = x1 - x0 + 1, y1 - y0 + 1
        interior = walk[y0 + 1:y1, x0 + 1:x1] if bw > 2 and bh > 2 else np.zeros((0, 0), bool)
        hollow = interior.size > 0 and interior.mean() > 0.3
        kind = th["kind"]
        if kind == "cave":
            add_struct(Structure(0, "rock", th["rock"], m))
        elif kind == "indoor":
            if touches or bw * bh > 40:
                add_struct(Structure(0, "wall", th["wall"], m))
            else:
                # 屋里独立的一块实心墙：床（居所那两张带帐幔的床就是这么摆的）
                props.append(Prop(prop_over.get("%d,%d" % (x0, y0), {}).get("kind", "canopy_bed"),
                                  x0, y0, bw, bh, True))
                prop_at[m] = len(props) - 1
        elif kind == "natural":
            if touches:
                add_struct(Structure(0, "rock", preset.get("border", th["rock"]), m))
            elif hollow:
                add_struct(Structure(0, "wall", "hall_" + th["facade"], m))
            else:
                st = Structure(0, "roofed", th["roof"], m)
                st.lots = np.where(m, 0, -1).astype(np.int32)
                add_struct(st)
        else:  # built
            if touches:
                add_built(m, th["wall"], th["roof"], False)
            elif hollow:
                add_struct(Structure(0, "wall", "hall_" + th["facade"], m))
            elif min(bw, bh) == 1 and not front[max(0, y0 - 1), x0:x1 + 1].any():
                # 一格厚、北边没有屋檐的一截：隔墙（墨府后园南墙被门洞截出来的那段）
                add_struct(Structure(0, "wall", th["wall"], m))
            else:
                st = Structure(0, "roofed", th["roof"], m)
                # 北边有一整行屋檐的是一座殿（外刃堂正堂二十格宽也是一个屋顶）；
                # 没有屋檐又特别宽的一大块才当成一排屋分户
                has_eave = y0 > 0 and front[y0 - 1, x0:x1 + 1].mean() >= 0.8
                if bw > 14 and not has_eave:
                    st.lots = roofscape_lots(md, m, walk, forced)
                else:
                    st.lots = np.where(m, 0, -1).astype(np.int32)
                add_struct(st)

    # ---- front 的其余用途：屋脊段 / 檐口 / 崖檐 / 伸到路上的树枝 ----
    eaves: list[EaveRun] = []
    walldeco = []
    boughs = []
    used = canopy.copy()
    solid = ~walk
    for y in range(H):
        x = 0
        while x < W:
            if not (front[y, x] and not used[y, x] and walk[y, x]):
                x += 1
                continue
            x1 = x
            while x1 + 1 < W and front[y, x1 + 1] and not used[y, x1 + 1] and walk[y, x1 + 1]:
                x1 += 1
            below = [int(at(sid, xx, y + 1, -1)) for xx in range(x, x1 + 1)]
            above = [int(at(sid, xx, y - 1, -1)) for xx in range(x, x1 + 1)]
            below_p = [int(at(prop_at, xx, y + 1, -1)) for xx in range(x, x1 + 1)]
            n_run = x1 - x + 1

            def major(vals):
                vs = [v for v in vals if v >= 0]
                return max(set(vs), key=vs.count) if vs and len(vs) * 2 >= n_run else -1

            s_below, s_above, p_below = major(below), major(above), major(below_p)
            # 屋脊那一段（北侧一行）只认「整栋屋的全宽」：hall()/村屋那种。镇子里挖街
            # 时随机撒的檐格落在巷子上、上下都是屋，归北边那栋的檐口——若归南边那栋
            # 画成整格屋脊，一格宽的巷子就被屋顶盖没了，看上去走不通。
            owns_width = False
            if s_below >= 0:
                st = structs[s_below]
                row = st.mask[y + 1] if y + 1 < H else np.zeros(W, bool)
                owns_width = n_run >= 0.8 * row.sum() and not st.compound
            # 岩体上的 front 不是屋檐：是崖檐、树冠边，交给装饰（松枝、藤、树枝）
            if s_below >= 0 and structs[s_below].kind == "rock":
                s_below = -1
            if s_above >= 0 and structs[s_above].kind == "rock":
                s_above = -1
            if p_below >= 0:
                eaves.append(EaveRun(y, x, x1, "north", -1, p_below))
            elif s_below >= 0 and owns_width:
                eaves.append(EaveRun(y, x, x1, "north", s_below))
            elif s_above >= 0:
                st = structs[s_above]
                if st.kind == "roofed" and not st.compound:
                    eaves.append(EaveRun(y, x, x1, "south", s_above))
                elif st.kind == "roofed" or st.style in ("manor", "fortress"):
                    # 院墙里伸出来的树枝
                    boughs.extend((xx, y) for xx in range(x, x1 + 1))
                # 镇墙下的檐格不画：城墙上挂一溜屋檐不成样子
            elif s_below >= 0:
                st = structs[s_below]
                if st.kind == "roofed" and not st.compound:
                    eaves.append(EaveRun(y, x, x1, "north", s_below))
                # 院墙 / 墙北侧街上的 front 不画：从北边街上看不见墙里伸出来的枝子挂在哪，
                # 画在街心就是一地散落的枝叶（南城北街第一版就是这样）
            # 前后都不挨结构的 front 同理不画
            used[y, x:x1 + 1] = True
            x = x1 + 1
    for y in range(H):
        for x in range(W):
            if front[y, x] and not used[y, x] and solid[y, x]:
                walldeco.append((x, y))

    # ---- 门洞与屋内 ----
    gaps, gap_at = find_gaps(walk, wallish, fence)
    rooms = find_rooms(walk, wallish | fence, gap_at, front, th["kind"] == "indoor")

    # ---- 地面 ----
    ov7 = O == 7
    ov6 = O == 6
    dense7 = ov7 & (count8(ov7) >= 7)
    dense6 = ov6 & (count8(ov6) >= 7)
    surface = np.zeros((H, W), np.int32)
    surface[:] = S[th["ground"]]
    surface[G == 2] = S[th["road"]]
    surface[(G == 3) & walk] = S[th["special"]]
    surface[water] = S["water"]
    if th["kind"] not in ("indoor", "cave"):
        # 宗门殿里是方砖（stone_floor）：细长的青砖地再压暗一档，一整座殿读成一个黑坑
        room_floor = {"sect": "stone_floor", "manor": "wood_floor", "town": "wood_floor",
                      "village": "earth", "valley": "wood_floor", "cliff": "stone_floor",
                      "mountain_path": "earth", "wild": "earth", "dock": "earth"}[theme]
        surface[rooms & (G == 1)] = S[room_floor]
        # 满地碎石（gid 7 一整片）的空地是铺过的：镇上的报名大院、神手谷的打坐石坪、
        # 外刃堂的演武沙地。一格一簇地撒石子会像一地垃圾。
        plaza = {"sect": "sand", "valley": "flagstone", "town": "flagstone", "manor": "flagstone",
                 "cliff": "flagstone", "village": "earth", "mountain_path": "scree", "wild": "scree",
                 "dock": "earth"}[theme]
        big7 = dense7 & (G == 1) & walk & ~rooms
        surface[pix.dilate(big7, 1) & ov7 & (G == 1) & walk & ~rooms] = S[plaza]
    elif th["kind"] == "indoor":
        # 屋里满地花草（藏书处阅览堂一整片 gid 6）：画成一张铺开的地毯 / 草席
        surface[dense6 & (G == 1)] = S["carpet"]
    # 路（gid 2）画成石阶只在它穿过坡面（gid 3 可走）的那几段：炼骨崖那条「每段路中央
    # 铺出一条踏步」穿过平台、歇脚台时是平的，画成台阶会让人以为那里还在爬坡。
    steps_from_road = (surface == S["steps"]) & (G == 2)
    if steps_from_road.any():
        slope = (G == 3) & walk
        near_slope = nb(slope, 1, 0) | nb(slope, -1, 0) | nb(slope, 2, 0) | nb(slope, -2, 0)
        surface[steps_from_road & ~near_slope] = S["flagstone"]
    # 水上的路是栈桥：路格的某个四邻是水
    touch_water = nb(water, 1, 0) | nb(water, -1, 0) | nb(water, 0, 1) | nb(water, 0, -1)
    plank = (G == 2) & walk & touch_water
    if plank.any():
        # 栈桥往岸上延伸的那一截也算，免得半截木板半截土路
        lab_p, n_p = label4((G == 2) & walk)
        for i in range(n_p):
            m = lab_p == i
            if (m & plank).any() and m.sum() <= 12:
                surface[m] = S["plank"]
            elif (m & plank).any():
                surface[m & plank] = S["plank"]
    # 人工指认的最后覆写（maps.json 的 zones）
    for z in preset.get("zones", []):
        x0, y0, x1, y1 = z["rect"]
        m = rect_mask(H, W, x0, y0, x1, y1)
        for key, gid in (("ground", 1), ("road", 2), ("special", 3)):
            if key in z:
                surface[m & (G == gid) & ~water] = S[z[key]]

    return Scene(md=md, preset=preset, theme=theme, th=th, season=season, time=preset["time"],
                 W=W, H=H, walk=walk, wall=wallish, fence=fence, water=water, surface=surface,
                 trees=trees, trunk=trunk, canopy=canopy, structs=structs, sid=sid, props=props,
                 prop_at=prop_at, eaves=eaves, walldeco=walldeco, boughs=boughs, gaps=gaps,
                 gap_at=gap_at, dense6=dense6, dense7=dense7, rooms=rooms)
