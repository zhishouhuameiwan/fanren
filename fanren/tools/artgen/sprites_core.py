"""像素精灵的底层：材质画布、字符模板盖印、自动描边、按色阶上色。

为什么画布上存「材质 + 明暗级」而不是直接存颜色：
  同一套身体模板要给五十来个人穿不同颜色的衣服。模板里只写「这一格是上衣的
  暗面」，颜色到最后一步才按这个人的外观查色阶——换衣色不用重画模板，
  描边也能按「挨着的是什么材质」取那种材质的最暗色（施工要求：不要纯黑一圈）。

模板约定（所有 sprites_* / fx_* 模块共用）：
  * 模板是多行字符串，每个字符一个像素；'.' 透明，'_' 擦除（把下层已画的像素抠掉）。
  * 其余字符经「码表」映射成 (材质, 明暗级)。明暗级 0 最暗、3 最亮；-1 是描边色。
  * 码表里映射到 None 的字符视为透明——可选部件（宽袖、敞怀）就靠这个开关。
  * 锚点字符（码表之外、登记在 anchors 里的）盖印时记下坐标，同时按给定材质画出来。
"""

from __future__ import annotations

import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

from PIL import Image  # noqa: E402

import palette as P  # noqa: E402

RGB = tuple[int, int, int]
Code = tuple[str, int]  # (材质名, 明暗级)


# ---------------------------------------------------------------------------
# 模板
# ---------------------------------------------------------------------------

def tpl(text: str) -> list[str]:
    """把三引号模板切成行。

    去掉首尾空行；每行去掉行首的公共缩进，保留行内的 '.'。
    行长不齐时右侧补 '.'——手写模板时行尾的透明像素常被省掉，补齐比报错省心，
    而真正的错位（某行多了字符）会在预览里一眼看出来。
    """
    lines = text.split("\n")
    while lines and not lines[0].strip():
        lines.pop(0)
    while lines and not lines[-1].strip():
        lines.pop()
    indent = min((len(l) - len(l.lstrip(" ")) for l in lines if l.strip()), default=0)
    rows = [l[indent:].rstrip() for l in lines]
    width = max((len(r) for r in rows), default=0)
    return [r.ljust(width, ".") for r in rows]


# ---------------------------------------------------------------------------
# 画布
# ---------------------------------------------------------------------------

class Canvas:
    """材质画布：每格存 (材质, 明暗级) 或 None。"""

    def __init__(self, w: int, h: int) -> None:
        self.w = w
        self.h = h
        self.px: list[list[Code | None]] = [[None] * w for _ in range(h)]

    def inside(self, x: int, y: int) -> bool:
        return 0 <= x < self.w and 0 <= y < self.h

    def get(self, x: int, y: int) -> Code | None:
        if not self.inside(x, y):
            return None
        return self.px[y][x]

    def set(self, x: int, y: int, code: Code | None) -> None:
        if self.inside(x, y):
            self.px[y][x] = code

    def copy(self) -> "Canvas":
        c = Canvas(self.w, self.h)
        c.px = [row[:] for row in self.px]
        return c

    def stamp(
        self,
        rows: list[str],
        x0: int,
        y0: int,
        codes: Mapping[str, Code | None],
        anchors: Mapping[str, Code] | None = None,
        found: dict[str, list[tuple[int, int]]] | None = None,
    ) -> None:
        """把模板盖到 (x0, y0)。

        codes 之外的字符若也不在 anchors 里，当场抛 KeyError——模板里的错字
        （比如把 'B' 敲成 'b'）静默变透明的话，要到预览里对着像素找半天。
        """
        for j, row in enumerate(rows):
            for i, ch in enumerate(row):
                if ch == ".":
                    continue
                x, y = x0 + i, y0 + j
                if ch == "_":
                    self.set(x, y, None)
                    continue
                if anchors is not None and ch in anchors:
                    if found is not None:
                        found.setdefault(ch, []).append((x, y))
                    code = anchors[ch]
                elif ch in codes:
                    code = codes[ch]
                else:
                    raise KeyError(f"模板字符 {ch!r} 不在码表里")
                if code is None:
                    continue
                self.set(x, y, code)

    def paste(self, other: "Canvas", x0: int, y0: int) -> None:
        for y in range(other.h):
            for x in range(other.w):
                v = other.px[y][x]
                if v is not None:
                    self.set(x0 + x, y0 + y, v)

    def shifted(self, dx: int, dy: int) -> "Canvas":
        c = Canvas(self.w, self.h)
        c.paste(self, dx, dy)
        return c

    def mirrored(self) -> "Canvas":
        c = Canvas(self.w, self.h)
        c.px = [row[::-1] for row in self.px]
        return c

    def bbox(self) -> tuple[int, int, int, int] | None:
        xs = [x for y in range(self.h) for x in range(self.w) if self.px[y][x] is not None]
        ys = [y for y in range(self.h) for x in range(self.w) if self.px[y][x] is not None]
        if not xs:
            return None
        return (min(xs), min(ys), max(xs), max(ys))


# ---------------------------------------------------------------------------
# 色阶
# ---------------------------------------------------------------------------

COOL_DARK: RGB = P.c("ink1")
WARM_LIGHT: RGB = P.c("paper")


def lum(rgb: RGB) -> float:
    r, g, b = rgb
    return 0.299 * r + 0.587 * g + 0.114 * b


def cloth_ramp(base: RGB) -> list[RGB]:
    """由一个底色推出四级明暗：0 深影、1 暗面、2 底色、3 受光。

    暗面往墨色（偏冷）走、受光往宣纸色（偏暖）走——像素画常用的「冷影暖光」，
    比单纯调亮度更有体积感，也让整张图的影子统一落在同一种冷色上。
    深色布料的受光面拉得更开一些：黑衣若只亮一丁点，三级明暗在屏幕上糊成一片。
    """
    l = lum(base)
    lift = 0.22 if l > 150 else (0.40 if l < 70 else 0.28)
    sink1 = 0.26 if l > 150 else (0.22 if l < 70 else 0.30)
    sink0 = 0.50 if l > 150 else (0.45 if l < 70 else 0.56)
    return [
        P.mix(base, COOL_DARK, sink0),
        P.mix(base, COOL_DARK, sink1),
        base,
        P.mix(base, WARM_LIGHT, lift),
    ]


def outline_of(ramp: list[RGB]) -> RGB:
    """描边色：该材质最暗一级再往墨里压一半。"""
    return P.mix(ramp[0], P.c("ink0"), 0.5)


@dataclass
class Materials:
    """材质名 → 四级色阶（+ 描边色）。"""

    ramps: dict[str, list[RGB]]
    outlines: dict[str, RGB]

    @staticmethod
    def build(ramps: Mapping[str, list[RGB]], outlines: Mapping[str, RGB] | None = None) -> "Materials":
        rs = {k: list(v) for k, v in ramps.items()}
        ol = {k: outline_of(v) for k, v in rs.items()}
        if outlines:
            ol.update(outlines)
        return Materials(rs, ol)

    def color(self, code: Code) -> RGB:
        mat, shade = code
        if mat not in self.ramps:
            raise KeyError(f"材质 {mat!r} 没有色阶")
        if shade < 0:
            return self.outlines[mat]
        ramp = self.ramps[mat]
        return ramp[max(0, min(len(ramp) - 1, shade))]


def fixed_ramp(*names: str) -> list[RGB]:
    return [P.c(n) for n in names]


# 与人物外观无关的固定材质（兵刃、木器、金饰……）。
FIXED_RAMPS: dict[str, list[RGB]] = {
    "eye": [P.c("ink0"), P.c("ink1"), P.c("ink1"), P.c("ink3")],
    "steel": fixed_ramp("ink3", "ink5", "ink7", "ink8"),
    "wood": fixed_ramp("wood1", "wood2", "wood3", "wood4"),
    "darkwood": fixed_ramp("wood0", "wood1", "wood2", "wood3"),
    "straw": fixed_ramp("wood2", "wood3", "wood4", "wood5"),
    "gold": fixed_ramp("gold0", "gold1", "gold2", "gold3"),
    "jade": fixed_ramp("jade0", "jade1", "jade2", "jade3"),
    "lacquer": fixed_ramp("red0", "red1", "red2", "red3"),
    "paper": fixed_ramp("stone3", "stone4", "stone5", "paper"),
    "cloth_w": fixed_ramp("stone2", "stone4", "plaster2", "plaster3"),
    "ink": fixed_ramp("ink0", "ink1", "ink2", "ink3"),
    "bronze": [P.mix("gold0", "jade0", 0.5), P.mix("gold1", "jade1", 0.45), P.mix("gold2", "jade2", 0.4), P.c("gold3")],
    "stone": fixed_ramp("stone1", "stone2", "stone3", "stone4"),
    "leather": fixed_ramp("wood0", "wood1", "wood2", "wood3"),
    "blush": [P.mix("red3", "skin2", 0.4)] * 4,
}


# ---------------------------------------------------------------------------
# 描边与出图
# ---------------------------------------------------------------------------

def add_outline(canvas: Canvas, mats: Materials, diagonal: bool = False) -> Canvas:
    """在剪影外面加 1px 描边。

    每个空格看四邻（diagonal=True 时看八邻），挑「描边色最暗」的那种材质来描——
    头发挨着脸的地方描发色、衣服挨着手的地方描衣色，而不是全身一圈同一种黑。
    只看四邻让斜角处留空，剪影的转角因此是圆的，这是 16px 人物常见的处理。
    """
    src = canvas
    out = canvas.copy()
    nbrs = [(1, 0), (-1, 0), (0, 1), (0, -1)]
    if diagonal:
        nbrs += [(1, 1), (-1, 1), (1, -1), (-1, -1)]
    for y in range(src.h):
        for x in range(src.w):
            if src.px[y][x] is not None:
                continue
            best: Code | None = None
            best_l = 1e9
            for dx, dy in nbrs:
                v = src.get(x + dx, y + dy)
                if v is None or v[1] == -1:
                    continue
                l = lum(mats.outlines[v[0]])
                if l < best_l:
                    best_l = l
                    best = (v[0], -1)
            if best is not None:
                out.px[y][x] = best
    return out


def to_image(canvas: Canvas, mats: Materials) -> Image.Image:
    img = Image.new("RGBA", (canvas.w, canvas.h), (0, 0, 0, 0))
    px = img.load()
    for y in range(canvas.h):
        for x in range(canvas.w):
            v = canvas.px[y][x]
            if v is None:
                continue
            r, g, b = mats.color(v)
            px[x, y] = (r, g, b, 255)
    return img


# ---------------------------------------------------------------------------
# 图集拼装与确定性写盘
# ---------------------------------------------------------------------------

def grid_sheet(frames: list[Image.Image], cols: int, fw: int, fh: int) -> Image.Image:
    rows = (len(frames) + cols - 1) // cols
    sheet = Image.new("RGBA", (cols * fw, rows * fh), (0, 0, 0, 0))
    for i, f in enumerate(frames):
        sheet.paste(f, ((i % cols) * fw, (i // cols) * fh))
    return sheet


def save_png(img: Image.Image, path: Path) -> Path:
    """写 PNG。不带时间戳等元数据，压缩级固定——同输入同字节（逐像素比对之外多一重保险）。"""
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path, format="PNG", optimize=False, compress_level=9)
    return path


def write_json(obj: object, path: Path) -> Path:
    """写 JSON：UTF-8 无 BOM、LF、键排序、缩进 1——生成物进仓库，要能 diff。"""
    import json

    path.parent.mkdir(parents=True, exist_ok=True)
    text = json.dumps(obj, ensure_ascii=False, indent=1, sort_keys=True) + "\n"
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    return path
