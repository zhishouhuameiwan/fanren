"""特效贴图与 UI 图标的生成入口。

    python tools/artgen/fx.py [out_root]     # 缺省 out_root = assets/art

被 tools/artgen/artgen.py 调用时走 build(out_root)。全部确定性（解析式 + 整数哈希）。

产物：
  out_root/fx/*.png         特效贴图（多帧的横排成一条；蓄劲气焰三档排成三行）
  out_root/fx/index.json    每张图的帧尺寸、帧数、建议混合方式与染色、锚点
  out_root/ui/icons.png     UI 小图标（16×16 一格）
  out_root/ui/icons.json    图标 id → 矩形；攻击类别 → 图标
"""

from __future__ import annotations

import sys
from pathlib import Path

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

from PIL import Image  # noqa: E402

import fx_combat as C  # noqa: E402
import fx_particles as PT  # noqa: E402
import fx_textures as T  # noqa: E402
from fx_icons import CATEGORY_ICONS, SIZE as ICON_SIZE, build_icons  # noqa: E402
from sprites_core import save_png, write_json  # noqa: E402

REPO = _HERE.parent.parent


def _entry(file: str, img: Image.Image, fw: int, fh: int, frames: int, blend: str, note: str,
           fps: float = 0.0, tint: str | None = None, anchor: list[int] | None = None,
           rows: int = 1) -> dict:
    e = {
        "file": file, "w": img.width, "h": img.height, "frame_w": fw, "frame_h": fh, "frames": frames,
        "rows": rows, "blend": blend, "note": note,
    }
    if fps:
        e["fps"] = fps
    if tint:
        e["tint"] = tint
    if anchor:
        e["anchor"] = anchor
    return e


def build(out_root: Path) -> list[Path]:
    out_root = Path(out_root)
    fxd = out_root / "fx"
    written: list[Path] = []
    tex: dict[str, dict] = {}

    def put(name: str, img: Image.Image, fw: int, fh: int, frames: int, blend: str, note: str, **kw) -> None:
        written.append(save_png(img, fxd / f"{name}.png"))
        tex[name] = _entry(f"{name}.png", img, fw, fh, frames, blend, note, **kw)

    # ---- 柔光 ----
    put("glow", T.glow(64), 64, 64, 1, "add", "径向光斑：中心 255 平滑到 0。点光源、辉光、光晕。")
    put("softcircle", T.softcircle(32), 32, 32, 1, "alpha", "柔边圆：粒子、光点、柔影。")
    put("shaft", T.shaft(32, 128), 32, 128, 1, "add", "光束：顶端亮往下渐隐，运行时旋转成斜射体积光。")
    put("shadow", T.shadow(24, 8), 24, 8, 1, "alpha", "脚下柔影：乘黑色画在人物脚下。", tint="#000000")
    put("fog", T.fog_noise(128), 128, 128, 1, "alpha", "可平铺的柔噪声雾纹（四边无缝），慢速平移当薄雾。")

    # ---- 粒子 ----
    put("leaves", PT.leaves(), 8, 8, 4, "alpha", "叶片 ×4（灰度，运行时乘绿 / 枯黄）。", tint="#6FA65A")
    put("petals", PT.petals(), 8, 8, 4, "alpha", "花瓣 ×4（灰度，运行时乘粉）。", tint="#E0A6A0")
    put("embers", PT.embers(), 8, 8, 4, "add", "火星 4 帧明灭（自带火色）。", fps=10.0)
    put("rain", PT.rain(), 4, 16, 1, "alpha", "雨丝，运行时斜落。", tint="#B4C8D8")
    put("snow", PT.snow(), 8, 8, 3, "alpha", "雪片 ×3 大小。")
    put("dust", PT.dust(), 4, 4, 1, "add", "尘埃光点。", tint="#F2D98B")
    put("firefly", PT.firefly(), 8, 8, 2, "add", "萤火 2 帧（亮 / 暗）。", fps=3.0, tint="#C8E07A")
    put("spark", PT.spark(), 8, 8, 3, "add", "火花（四角星）3 帧由长到短。", fps=12.0)
    put("shards", PT.shards(), 12, 12, 6, "alpha", "破势碎片 ×6（碎玉般的菱形片，灰度，运行时乘玉青/金）。", tint="#BFE3D2")

    # ---- 战斗 ----
    put("slash", C.slash(48), 48, 48, 4, "add", "刀光弧线 4 帧（弧背朝左；敌方出招时水平翻转）。", fps=16.0)
    put("impact", C.impact(48), 48, 48, 3, "add", "钝击冲击 3 帧（拳、棍）。", fps=14.0)
    put("fireburst", C.fireburst(48), 48, 48, 4, "alpha", "火弹爆裂 4 帧（自带火色）。", fps=12.0)
    put("wind", C.wind(48), 48, 48, 3, "add", "风旋 3 帧。", fps=12.0, tint="#CFE8E0")
    put("poison", C.poison(48), 48, 48, 3, "alpha", "毒雾 3 帧（灰度，运行时乘紫绿）。", fps=6.0, tint="#8A6FA8")
    put("heal", C.heal(48), 48, 48, 3, "add", "治愈光点 3 帧（往上飘）。", fps=8.0, tint="#9FE0B8")
    put("boost_aura", C.boost_aura(), C.AURA_W, C.AURA_H, 3, "add",
        "蓄劲气焰：三行 = 蓄劲 1/2/3 档，每行 3 帧；金色，画在人物身后。anchor 对到人物的脚底点（人物帧的底边中点）。",
        fps=10.0, anchor=list(C.AURA_ANCHOR), rows=3)
    put("boss_aura", C.boss_aura(), C.AURA_W, C.AURA_H, 3, "alpha",
        "首领「蓄势」暗红气焰 3 帧；画在首领身后，anchor 同上；大体型敌人运行时按体型放大。", fps=8.0,
        anchor=list(C.AURA_ANCHOR))

    write_json({
        "_说明": "由 tools/artgen/fx.py 生成，勿手改。多帧图横排；rows>1 时每行是一组（boost_aura 每行一档）。"
                 "blend：add=加色、alpha=普通混合；tint 是建议乘色（白/灰度图才需要）。",
        "version": 1,
        "textures": tex,
    }, fxd / "index.json")
    written.append(fxd / "index.json")

    icons, idx = build_icons()
    written.append(save_png(icons, out_root / "ui" / "icons.png"))
    written.append(write_json({
        "_说明": "由 tools/artgen/fx.py 生成，勿手改。每个图标 16×16，运行时 ×2 绘制。",
        "version": 1,
        "file": "icons.png",
        "size": ICON_SIZE,
        "scale": 2,
        "icons": idx,
        "categories": CATEGORY_ICONS,
    }, out_root / "ui" / "icons.json"))
    return sorted(written)


def main(argv: list[str]) -> int:
    out = Path(argv[1]) if len(argv) > 1 else REPO / "assets" / "art"
    files = build(out)
    print(f"fx: 写出 {len(files)} 个文件到 {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
