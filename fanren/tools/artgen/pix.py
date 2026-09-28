"""像素底层：渐层簿、「渐层 + 明度级」画布、有序抖动、确定性噪声与杂凑。

为什么画布存「渐层 id + 明度级」而不是直接存 RGB：
  地图上的一切明暗——墙根的接触阴影、树冠投在地上的影子、檐下那一道暗——
  都是「在这种材质上暗一两级」。存成明度级之后，阴影就是一次减法，最后一步
  按 4×4 Bayer 阈值量化回渐层上的某一格颜色：
    * 出图的每一个像素都来自某条渐层，而渐层只由 palette.c()/mix() 调出来，
      色板统一这件事由结构保证，不靠自觉；
    * 半级明度自然变成棋盘状抖动——像素画的阴影本来就该这样过渡，而不是糊成
      一片连续的灰。
  人物那一路（sprites_core.py）用的是同一个思路，两边的明暗分级因此对得上。

确定性：
  随机一律经 rng(...) / hash01(...)，种子由调用方给出的一串部件（地图 id、格坐标、
  用途名）经 crc32 派生。**不许用内置 hash()**：它对字符串逐进程加盐，同一份输入
  两次运行会得到两张图，--check 门禁当场就红。
"""

from __future__ import annotations

import sys
import zlib
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

import numpy as np  # noqa: E402
from PIL import Image, ImageDraw  # noqa: E402

import palette as P  # noqa: E402

RGB = tuple[int, int, int]

# 4×4 Bayer 阈值，均值 0.5。量化规则 q = floor(level + t)：整数明度级出纯色，
# 带小数的明度级按小数部分的比例落到上一级——0.25 就是四格里亮一格。
_BAYER4 = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]], np.float32)
BAYER = (_BAYER4 + 0.5) / 16.0


# ---------------------------------------------------------------------------
# 种子与杂凑
# ---------------------------------------------------------------------------
def seed_of(*parts: object) -> int:
    return zlib.crc32("|".join(str(p) for p in parts).encode("utf-8"))


def rng(*parts: object) -> np.random.Generator:
    return np.random.default_rng(seed_of(*parts))


def hash01(x, y, salt: int):
    """整数坐标 → [0,1) 的确定性杂凑，标量与数组都收。

    逐格的「这一格长哪种花」「这棵树往哪边歪」全靠它：与遍历顺序无关，
    改了某一格不会像共用一个随机流那样把后面所有格子的结果一起带偏。
    """
    xa = np.asarray(x, dtype=np.int64) & 0xFFFF_FFFF
    ya = np.asarray(y, dtype=np.int64) & 0xFFFF_FFFF
    s = ((int(salt) & 0xFFFF_FFFF) * 2246822519) & 0xFFFF_FFFF
    h = (xa * 374761393 + ya * 668265263 + s) & 0xFFFF_FFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    h = h ^ (h >> 16)
    out = h / 4294967296.0
    if np.ndim(out) == 0:
        return float(out)
    return out


def salt_of(*parts: object) -> int:
    return seed_of(*parts) & 0x7FFFFFFF


# ---------------------------------------------------------------------------
# 渐层簿
# ---------------------------------------------------------------------------
class RampBook:
    """具名渐层的登记处。id 只在一次运行里有效，出图时一次性展开成查找表。

    0 号建簿时就留给「空白」：画布上从没画过的像素 rid 是 0，出图取它的颜色（透明画布上就是
    那些透明像素的 RGB）。
    0 号若让给第一个登记的渐层，这些像素的 RGB 就看同一进程里先画过什么——单烘一张图
    （--map）、只跑一类（--only backdrops）与全量烘焙出的同一张图 alpha 一样、RGB 不一样，
    --check 逐像素比就报「变」。空白取 stone0：从前全量烘焙时 0 号恰好落给 paving_town，
    它的首格就是 stone0，沿用下来，已提交的产物一个像素都不变。
    """

    MAX_LEN = 8
    BLANK = "(空白)"

    def __init__(self) -> None:
        self._names: list[str] = []
        self._colors: list[list[RGB]] = []
        self._index: dict[str, int] = {}
        self.add(self.BLANK, [P.c("stone0")])

    def add(self, name: str, colors: list[RGB]) -> int:
        if name in self._index:
            return self._index[name]
        if not 1 <= len(colors) <= self.MAX_LEN:
            raise ValueError("渐层 %s 的长度 %d 不在 1..%d" % (name, len(colors), self.MAX_LEN))
        self._index[name] = len(self._names)
        self._names.append(name)
        self._colors.append([tuple(int(v) for v in col) for col in colors])
        return self._index[name]

    def __getitem__(self, name: str) -> int:
        return self._index[name]

    def __contains__(self, name: str) -> bool:
        return name in self._index

    def length(self, name: str) -> int:
        return len(self._colors[self._index[name]])

    def colors(self, name: str) -> list[RGB]:
        return list(self._colors[self._index[name]])

    def name_of(self, rid: int) -> str:
        return self._names[rid]

    def table(self) -> tuple[np.ndarray, np.ndarray]:
        n = len(self._names)
        tab = np.zeros((max(n, 1), self.MAX_LEN, 3), np.uint8)
        lens = np.ones(max(n, 1), np.int32)
        for i, cols in enumerate(self._colors):
            for j in range(self.MAX_LEN):
                tab[i, j] = cols[min(j, len(cols) - 1)]
            lens[i] = len(cols)
        return tab, lens


BOOK = RampBook()


def ramp_id(name: str, colors: list[RGB] | None = None) -> int:
    """按名字取渐层 id；第一次见到时须给出颜色。"""
    if colors is None:
        return BOOK[name]
    return BOOK.add(name, colors)


# ---------------------------------------------------------------------------
# 画布
# ---------------------------------------------------------------------------
class Canvas:
    """一张「渐层 id + 明度级 + 不透明」三层的画布。

    on 是硬 alpha：像素画不要半透明边——树冠、屋檐的边缘在 ×3 放大后若带半透明，
    会糊成一圈脏边。柔和的东西（影子）走明度级 + 抖动，不走 alpha。
    """

    def __init__(self, w: int, h: int, opaque: bool = True) -> None:
        self.w, self.h = w, h
        self.rid = np.zeros((h, w), np.int32)  # 0 = 空白渐层（RampBook 建簿时占住的 0 号）
        self.lv = np.zeros((h, w), np.float32)
        self.on = np.full((h, w), opaque, bool)

    # -- 区域裁剪：把放在 (x0,y0) 的一块 (mh,mw) 裁进画布 --
    def _clip(self, x0: int, y0: int, mh: int, mw: int):
        cx0, cy0 = max(0, x0), max(0, y0)
        cx1, cy1 = min(self.w, x0 + mw), min(self.h, y0 + mh)
        if cx0 >= cx1 or cy0 >= cy1:
            return None
        return (slice(cy0, cy1), slice(cx0, cx1),
                slice(cy0 - y0, cy1 - y0), slice(cx0 - x0, cx1 - x0))

    def paint(self, x0: int, y0: int, mask: np.ndarray, rid: int, lv) -> None:
        """在 mask 为真的像素上写渐层与明度级。lv 可以是标量或与 mask 同形的数组。"""
        mh, mw = mask.shape
        cl = self._clip(x0, y0, mh, mw)
        if cl is None:
            return
        sy, sx, my, mx = cl
        m = mask[my, mx]
        self.rid[sy, sx][m] = rid
        if np.ndim(lv) == 0:
            self.lv[sy, sx][m] = lv
        else:
            self.lv[sy, sx][m] = np.asarray(lv, np.float32)[my, mx][m]
        self.on[sy, sx][m] = True

    def paint_tex(self, x0: int, y0: int, mask: np.ndarray, rid: int, tex: np.ndarray, delta: float = 0.0) -> None:
        """像 paint，但明度级从一张整图大小的纹理里按绝对坐标取——区域伸出画布也对得上。"""
        mh, mw = mask.shape
        cl = self._clip(x0, y0, mh, mw)
        if cl is None:
            return
        sy, sx, my, mx = cl
        m = mask[my, mx]
        self.rid[sy, sx][m] = rid
        self.lv[sy, sx][m] = tex[sy, sx][m] + delta
        self.on[sy, sx][m] = True

    def rect(self, x0: int, y0: int, w: int, h: int, rid: int, lv) -> None:
        if w <= 0 or h <= 0:
            return
        self.paint(x0, y0, np.ones((h, w), bool), rid, lv)

    def px(self, x: int, y: int, rid: int, lv: float) -> None:
        if 0 <= x < self.w and 0 <= y < self.h:
            self.rid[y, x] = rid
            self.lv[y, x] = lv
            self.on[y, x] = True

    def shade(self, x0: int, y0: int, delta, mask: np.ndarray | None = None) -> None:
        """在一块区域上加减明度级（只动已有像素）。delta 标量或数组。"""
        if mask is None:
            if np.ndim(delta) == 0:
                raise ValueError("标量 delta 必须配 mask")
            mh, mw = np.shape(delta)
            mask = np.ones((mh, mw), bool)
        mh, mw = mask.shape
        cl = self._clip(x0, y0, mh, mw)
        if cl is None:
            return
        sy, sx, my, mx = cl
        m = mask[my, mx] & self.on[sy, sx]
        if np.ndim(delta) == 0:
            self.lv[sy, sx][m] += delta
        else:
            self.lv[sy, sx][m] += np.asarray(delta, np.float32)[my, mx][m]

    def erase(self, x0: int, y0: int, mask: np.ndarray) -> None:
        mh, mw = mask.shape
        cl = self._clip(x0, y0, mh, mw)
        if cl is None:
            return
        sy, sx, my, mx = cl
        self.on[sy, sx][mask[my, mx]] = False

    def to_array(self, alpha: bool) -> np.ndarray:
        tab, lens = BOOK.table()
        t = np.tile(BAYER, (self.h // 4 + 1, self.w // 4 + 1))[: self.h, : self.w]
        q = np.floor(self.lv + t).astype(np.int32)
        q = np.clip(q, 0, lens[self.rid] - 1)
        rgb = tab[self.rid, q]
        if not alpha:
            return rgb
        a = np.where(self.on, 255, 0).astype(np.uint8)
        return np.dstack([rgb, a])


# ---------------------------------------------------------------------------
# 噪声（确定性、可选横向无缝）
# ---------------------------------------------------------------------------
def _smoothstep(t: np.ndarray) -> np.ndarray:
    return t * t * (3.0 - 2.0 * t)


def value_noise(h: int, w: int, cell: float, seed: int, wrap_x: bool = False) -> np.ndarray:
    """格点随机值 + smoothstep 双线性插值，取值 [0,1]。

    wrap_x 时横向首尾相接（战斗背景要做横向视差与云雾漂移，层必须能无缝平铺）。
    """
    g = np.random.default_rng(seed)
    if wrap_x:
        gw = max(1, int(round(w / cell)))
        cell_x = w / gw
    else:
        gw = int(w / cell) + 2
        cell_x = cell
    gh = int(h / cell) + 2
    grid = g.random((gh, gw + 1)).astype(np.float32)
    if wrap_x:
        grid[:, gw] = grid[:, 0]
    ys = (np.arange(h, dtype=np.float32) + 0.5) / cell
    xs = (np.arange(w, dtype=np.float32) + 0.5) / cell_x
    y0 = np.floor(ys).astype(np.int32)
    x0 = np.floor(xs).astype(np.int32)
    fy = _smoothstep(ys - y0)[:, None]
    fx = _smoothstep(xs - x0)[None, :]
    if wrap_x:
        x0 = x0 % gw
    x1 = x0 + 1
    y1 = y0 + 1
    a = grid[y0][:, x0]
    b = grid[y0][:, x1]
    c_ = grid[y1][:, x0]
    d = grid[y1][:, x1]
    top = a + (b - a) * fx
    bot = c_ + (d - c_) * fx
    return top + (bot - top) * fy


def fbm(h: int, w: int, seed: int, octaves=((24, 0.55), (12, 0.3), (6, 0.15)),
        wrap_x: bool = False) -> np.ndarray:
    out = np.zeros((h, w), np.float32)
    total = 0.0
    for i, (cell, wt) in enumerate(octaves):
        out += wt * value_noise(h, w, cell, seed * 7 + i * 131 + 1, wrap_x)
        total += wt
    return out / total


def white(h: int, w: int, seed: int) -> np.ndarray:
    return np.random.default_rng(seed).random((h, w)).astype(np.float32)


# ---------------------------------------------------------------------------
# 形状（全部返回 bool 掩码）
# ---------------------------------------------------------------------------
def disc(r: float) -> np.ndarray:
    n = int(np.ceil(r))
    yy, xx = np.mgrid[-n:n + 1, -n:n + 1]
    return (xx * xx + yy * yy) <= r * r + 0.3


def ellipse(w: int, h: int) -> np.ndarray:
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy = (w - 1) / 2.0, (h - 1) / 2.0
    rx, ry = max(w / 2.0, 0.5), max(h / 2.0, 0.5)
    return ((xx - cx) / rx) ** 2 + ((yy - cy) / ry) ** 2 <= 1.0


def poly_mask(w: int, h: int, pts: list[tuple[float, float]]) -> np.ndarray:
    im = Image.new("L", (w, h), 0)
    ImageDraw.Draw(im).polygon(pts, fill=255)
    return np.asarray(im) > 0


def line_mask(w: int, h: int, pts: list[tuple[float, float]], width: int = 1) -> np.ndarray:
    im = Image.new("L", (w, h), 0)
    ImageDraw.Draw(im).line(pts, fill=255, width=width)
    return np.asarray(im) > 0


def dilate(m: np.ndarray, r: int = 1) -> np.ndarray:
    out = m.copy()
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            if dx == 0 and dy == 0:
                continue
            out |= shift(m, dx, dy)
    return out


def erode(m: np.ndarray, r: int = 1) -> np.ndarray:
    return ~dilate(~m, r)


def shift(a: np.ndarray, dx: int, dy: int, fill=False) -> np.ndarray:
    """整体平移（不回绕），空出来的地方填 fill。"""
    out = np.full_like(a, fill)
    h, w = a.shape[:2]
    sx0, sx1 = max(0, -dx), min(w, w - dx)
    sy0, sy1 = max(0, -dy), min(h, h - dy)
    if sx0 >= sx1 or sy0 >= sy1:
        return out
    out[sy0 + dy:sy1 + dy, sx0 + dx:sx1 + dx] = a[sy0:sy1, sx0:sx1]
    return out


def box_blur(a: np.ndarray, r: int) -> np.ndarray:
    """可分离的方框模糊（边缘按边界值延伸），给柔和阴影与接边场用。"""
    if r <= 0:
        return a.astype(np.float32)
    f = a.astype(np.float32)
    k = 2 * r + 1
    p = np.pad(f, ((0, 0), (r, r)), mode="edge")
    cs = np.cumsum(np.pad(p, ((0, 0), (1, 0))), axis=1)
    f = (cs[:, k:] - cs[:, :-k]) / k
    p = np.pad(f, ((r, r), (0, 0)), mode="edge")
    cs = np.cumsum(np.pad(p, ((1, 0), (0, 0))), axis=0)
    return (cs[k:, :] - cs[:-k, :]) / k


def upsample_cells(cells: np.ndarray, tile: int, smooth: bool) -> np.ndarray:
    """格值场 → 像素场。smooth 时按格心双线性插值（接边用），否则逐格铺满。"""
    ch, cw = cells.shape
    f = cells.astype(np.float32)
    if not smooth:
        return np.repeat(np.repeat(f, tile, axis=0), tile, axis=1)
    p = np.pad(f, 1, mode="edge")
    ys = (np.arange(ch * tile, dtype=np.float32) + 0.5) / tile + 0.5
    xs = (np.arange(cw * tile, dtype=np.float32) + 0.5) / tile + 0.5
    y0 = np.floor(ys).astype(np.int32)
    x0 = np.floor(xs).astype(np.int32)
    fy = (ys - y0)[:, None]
    fx = (xs - x0)[None, :]
    a = p[y0][:, x0]
    b = p[y0][:, x0 + 1]
    c_ = p[y0 + 1][:, x0]
    d = p[y0 + 1][:, x0 + 1]
    top = a + (b - a) * fx
    bot = c_ + (d - c_) * fx
    return top + (bot - top) * fy


# ---------------------------------------------------------------------------
# 出图
# ---------------------------------------------------------------------------
def save_png(arr: np.ndarray, path: Path, fast: bool = False) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = "RGBA" if arr.shape[2] == 4 else "RGB"
    Image.fromarray(np.ascontiguousarray(arr), mode).save(
        path, optimize=False, compress_level=1 if fast else 6)
    return path


def mixc(a, b, t: float) -> RGB:
    return P.mix(a, b, t)


def c(name: str) -> RGB:
    return P.c(name)


def shade_rgb(col: RGB, k: float) -> RGB:
    """把一个 RGB 按比例提亮 (k>0) 或压暗 (k<0)，只给派生渐层用。"""
    if k >= 0:
        return P.mix(col, (255, 255, 255), k)
    return P.mix(col, (0, 0, 0), -k)
