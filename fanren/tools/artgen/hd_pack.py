"""手绘高清美术打包（伙伴改版 E 路，docs/interfaces-companions.md 2.5、3.9；格式见 docs/art-sprites.md 5.2、5.3）。

把协调者定稿的人物图、头像、首领图（Seedream 出图、已还原成真像素）做成游戏能读的资产：

  assets/art/sprites/hd/<人>_walk.png、<人>_battle.png   人物走路表（64×60，4×4）与战斗表（132×84，4×2），原样转存
  assets/art/sprites/hd/<敌人 id>.png                     非人形敌人：四个动作统一进一套格子（身躯、脚底对齐），照源图原样
  assets/art/sprites/hd/index.json                        手绘索引：sheets、roles、role_variants、enemies、files（sha256）
  assets/art/portraits/<人>.png                           对白头像：四个表情排一行，格子统一、×1 画
  data/visual/portraits.json                              头像表（角色 id → 按章生效的头像列表）

    python tools/artgen/hd_pack.py [--src <定稿目录>]           # 写出上面这些（要源图）
    python tools/artgen/hd_pack.py [--src <定稿目录>] --check   # 核：图与索引对得上；有源图时再重打一遍比像素

产物进仓库，源图不进（定稿帧在主检出的 docs/companions-overhaul/，未入库）。所以 --check 分两半：
图与 hd/index.json 里的 sha256 一致、头像表引的图都在且放得下格子——这一半随时能跑；
从源图重新打包、与磁盘的像素相同——源目录在才跑，不在时打印 HD_PACK_IN_SYNC_PARTIAL 并写明「跳过，没核」。
源目录：--src 优先，其次环境变量 FANREN_HD_SRC，再其次仓库里的 docs/companions-overhaul（不入库）。
门禁规则 40 另核 sha256（validate.py）。

确定性：不读时钟、没有随机数；缩放、对齐都是整数运算或按固定次序的取舍；JSON 键排序、UTF-8 无 BOM、LF。
"""

from __future__ import annotations

import argparse
import hashlib
import io
import json
import math
import os
import sys
from dataclasses import dataclass
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
# 定稿源图（不入库）：命令行 --src 优先，其次环境变量 FANREN_HD_SRC，都没有就找仓库里的 docs/companions-overhaul
# （主检出里有、工作树与公开仓库里没有——那时 --check 只核不靠源图的那一半）。
SRC_ENV = "FANREN_HD_SRC"
REPO_SRC = REPO / "docs" / "companions-overhaul"


def default_src() -> Path:
    env = os.environ.get(SRC_ENV, "").strip()
    return Path(env) if env else REPO_SRC

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
    busts: tuple[int, ...]  # 用到的头像源图编号（<prefix>_bust_<n>.png），次序即头像表里的格子；路人为空（不出头像）
    faces: tuple[int, int, int, int]   # normal / smile / angry / hurt 各指第几格


PEOPLE = (
    Person("hanli_hd", "hanli", "hanli-final", "hanli", "韩立（手绘，第 7 章）", (0, 1, 2, 3), (0, 1, 2, 3)),
    # 面纱版只有「平常」一张：四个表情都指它
    Person("nangong_wan_veil_hd", "nangong_wan_veil", "nangong-veil-final", "nangong_veil",
           "南宫婉·面纱（手绘）", (0,), (0, 0, 0, 0)),
    # 露脸前期：bust_0..3 是前期的四个表情（后面 4..8 是面纱与后期，这次不用）
    Person("nangong_wan_ch07_hd", "nangong_wan_ch07", "nangong-final", "nangong",
           "南宫婉·露脸前期（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("chen_shimei_hd", "chen_shimei", "chen-final", "chen", "陈巧倩（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    # 第 7 章美术重构（2026-11-19）：路人与门派弟子只出走路表和战斗表，不出头像。原稿由 hd_frames.py 还原成真像素。
    Person("lingshoushan_dizi_hd", "lingshoushan_dizi", "passers-final", "lingshoushan_dizi", "灵兽山弟子（手绘）",
           (), (0, 0, 0, 0)),
    # 第 7 章美术重构批量（2026-11-19/20）：具名角色带四张头像，路人与门派弟子不带。原稿由 hd_frames.py 还原。
    Person("li_huayuan_hd", "li_huayuan", "named-final", "li_huayuan", "李师祖（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("ma_shibo_hd", "ma_shibo", "named-final", "ma_shibo", "马师伯（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("xiang_zhili_hd", "xiang_zhili", "named-final", "xiang_zhili", "向之礼（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("xu_lao_hd", "xu_lao", "named-final", "xu_lao", "许老（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("chou_han_hd", "chou_han", "named-final", "chou_han", "丑汉（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("han_yunzhi_hd", "han_yunzhi", "named-final", "han_yunzhi", "菡云芝（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("zhong_wu_hd", "zhong_wu", "named-final", "zhong_wu", "钟吾（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("fuyunzi_hd", "fuyunzi", "named-final", "fuyunzi", "浮云子（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("nichang_xianzi_hd", "nichang_xianzi", "named-final", "nichang_xianzi", "霓裳仙子（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("qiong_qianbei_hd", "qiong_qianbei", "named-final", "qiong_qianbei", "穹前辈（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("feng_yue_hd", "feng_yue", "named-final", "feng_yue", "封岳（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("lu_shixiong_hd", "lu_shixiong", "named-final", "lu_shixiong", "陆师兄（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("luosai_huzi_hd", "luosai_huzi", "named-final", "luosai_huzi", "络腮胡子（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("yan_wuchi_hd", "yan_wuchi", "named-final", "yan_wuchi", "赤脚大汉（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("yan_xiongdi_hd", "yan_xiongdi", "named-final", "yan_xiongdi", "严姓修士（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("chen_dage_hd", "chen_dage", "named-final", "chen_dage", "陈师兄（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("huangshan_shijie_hd", "huangshan_shijie", "named-final", "huangshan_shijie", "黄衫师姐（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("xiang_shaonian_hd", "xiang_shaonian", "named-final", "xiang_shaonian", "黄毛少年（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("jujianmen_gaoren_hd", "jujianmen_gaoren", "named-final", "jujianmen_gaoren", "巨剑门高人（手绘）", (0, 1, 2, 3), (0, 1, 2, 3)),
    Person("fangshi_tanzhu_hd", "fangshi_tanzhu", "passers-final", "fangshi_tanzhu", "收药摊主（手绘）", (), (0, 0, 0, 0)),
    Person("fangshi_zhishi_hd", "fangshi_zhishi", "passers-final", "fangshi_zhishi", "执事弟子（手绘）", (), (0, 0, 0, 0)),
    Person("huadaowu_dizi_hd", "huadaowu_dizi", "passers-final", "huadaowu_dizi", "化刀坞弟子（手绘）", (), (0, 0, 0, 0)),
    Person("huangfeng_shilian_hd", "huangfeng_shilian", "passers-final", "huangfeng_shilian", "黄衫弟子（手绘）", (), (0, 0, 0, 0)),
    Person("jujianmen_dizi_hd", "jujianmen_dizi", "passers-final", "jujianmen_dizi", "巨剑门弟子（手绘）", (), (0, 0, 0, 0)),
    Person("qingxumen_daoshi_hd", "qingxumen_daoshi", "passers-final", "qingxumen_daoshi", "清虚门道士（手绘）", (), (0, 0, 0, 0)),
    Person("tianquebao_dizi_hd", "tianquebao_dizi", "passers-final", "tianquebao_dizi", "天阙堡弟子（手绘）", (), (0, 0, 0, 0)),
    Person("wanbaolou_shicong_hd", "wanbaolou_shicong", "passers-final", "wanbaolou_shicong", "万宝楼伙计（手绘）", (), (0, 0, 0, 0)),
    Person("yanyue_dizi_hd", "yanyue_dizi", "passers-final", "yanyue_dizi", "掩月宗弟子（手绘）", (), (0, 0, 0, 0)),
    Person("yuanwu_xiushi_hd", "yuanwu_xiushi", "passers-final", "yuanwu_xiushi", "元武国修士（手绘）", (), (0, 0, 0, 0)),
)
# 批量这一批都只在第 7 章生效（李师祖、马师伯、陆师兄第 6 章也在，钟吾第 8 章也在）：挂 role_variants，别的章节落回生成的表。
CH07_ONLY = tuple(p.stem for p in PEOPLE if p.src_dir in ("named-final",) or
                  (p.src_dir == "passers-final" and p.stem != "lingshoushan_dizi"))

# 角色 → 手绘表。这三个角色只在第 7 章出场，直接接管。
ROLES = {
    "baiyi_shaonv": "nangong_wan_veil_hd",
    "nangong_wan_ch07": "nangong_wan_ch07_hd",
    "chen_shimei": "chen_shimei_hd",
    "lingshoushan_dizi": "lingshoushan_dizi_hd",   # 只在第 7 章禁地外出场
    # 同道录 2a·路 N（docs/ch07-tongdaolu.md 第 4 节）：菡云芝入册、能带进历练与切磋，战斗小人用这张手绘表的八个姿势，
    # 不看章节直接接管（与陈巧倩一样）；role_variants 里第 7 章那一条照旧留着，指的是同一张表。
    "han_yunzhi": "han_yunzhi_hd",
}
# 韩立每一章都在，只在第 7 章换手绘：第 1–2 章仍是生成的少年相，其余章节落回生成索引的 roles（hanli）。
ROLE_VARIANTS = {
    "hanli": [{"look": "hanli_child", "max_chapter": 2},
              {"look": "hanli_hd", "min_chapter": 7, "max_chapter": 7}],
    **{stem: [{"look": f"{stem}_hd", "min_chapter": 7, "max_chapter": 7}] for stem in CH07_ONLY},
}
# 头像：角色 id → (头像图的 stem, min_chapter, max_chapter)，0 = 这一端不限。
PORTRAITS = {
    "hanli": ("hanli", 7, 7),
    "baiyi_shaonv": ("nangong_wan_veil", 0, 0),
    "nangong_wan_ch07": ("nangong_wan_ch07", 0, 0),
    "chen_shimei": ("chen_shimei", 0, 0),
    # 批量的具名角色：头像同样只在第 7 章
    "li_huayuan": ("li_huayuan", 7, 7),
    "ma_shibo": ("ma_shibo", 7, 7),
    "xiang_zhili": ("xiang_zhili", 7, 7),
    "xu_lao": ("xu_lao", 7, 7),
    "chou_han": ("chou_han", 7, 7),
    "han_yunzhi": ("han_yunzhi", 7, 7),
    "zhong_wu": ("zhong_wu", 7, 7),
    "fuyunzi": ("fuyunzi", 7, 7),
    "nichang_xianzi": ("nichang_xianzi", 7, 7),
    "qiong_qianbei": ("qiong_qianbei", 7, 7),
    "feng_yue": ("feng_yue", 7, 7),
    "lu_shixiong": ("lu_shixiong", 7, 7),
    "luosai_huzi": ("luosai_huzi", 7, 7),
    "yan_wuchi": ("yan_wuchi", 7, 7),
    "yan_xiongdi": ("yan_xiongdi", 7, 7),
    "chen_dage": ("chen_dage", 7, 7),
    "huangshan_shijie": ("huangshan_shijie", 7, 7),
    "xiang_shaonian": ("xiang_shaonian", 7, 7),
    "jujianmen_gaoren": ("jujianmen_gaoren", 7, 7),
}


@dataclass(frozen=True)
class Enemy:
    enemy_id: str
    src_prefix: str        # bosses-final/<prefix>_act0..3.png：待机、出招、受击、倒下
    name: str
    kind: str
    match_size_of: str = ""   # 非空：整体缩到与这一只的待机帧一样大（同一条蛟，白的那套像素网格测得不同）
    scale: int = 1            # 世界层倍数（缺省 ENEMY_SCALE）
    battle_scale: int = 2     # 战斗层倍数（缺省 ENEMY_BATTLE_SCALE）


ENEMIES = (
    Enemy("mo_jiao", "boss_mojiao", "墨蛟", "beast"),
    Enemy("mo_jiao_tuipi", "boss_mojiao_white", "墨蛟（蜕皮）", "beast", match_size_of="boss_mojiao"),
    Enemy("bishui_e", "boss_bishuie", "碧水鳄", "beast"),
    # 第 7 章美术重构（2026-11-19）：普通妖兽与人同一种像素颗粒——世界层、战斗层都 ×2，原生约 48 宽（hd_frames.py 定）。
    Enemy("hanbing_chan", "beast_hanbing_chan", "寒冰蟾", "beast", scale=2, battle_scale=2),
    # 其余八种（都只在第 7 章出场；2026-11-20）
    Enemy("cai_e", "beast_cai_e", "彩蛾", "beast", scale=2, battle_scale=2),
    Enemy("chuanshan_shou", "beast_chuanshan_shou", "穿山甲兽", "beast", scale=2, battle_scale=2),
    Enemy("fei_she", "beast_fei_she", "飞蛇", "beast", scale=2, battle_scale=2),
    Enemy("huoyan_shu", "beast_huoyan_shu", "火焰鼠", "beast", scale=2, battle_scale=2),
    Enemy("tiebi_yuan", "beast_tiebi_yuan", "铁臂猿", "beast", scale=2, battle_scale=2),
    Enemy("tuishan_shou", "beast_tuishan_shou", "推山兽", "beast", scale=2, battle_scale=2),
    Enemy("zichi_bishe", "beast_zichi_bishe", "紫翅碧蛇", "beast", scale=2, battle_scale=2),
    Enemy("jinzhi_yanceng", "beast_jinzhi_yanceng", "封禁岩层", "beast", scale=2, battle_scale=2),
)
# 历练按境界分档：第 2、3 档是同一只妖兽换一档数值、另起一个角色 id，长相不变，借本体的图（与生成索引的 alias_of 同一个写法）。
HD_ENEMY_ALIASES = {"bishui_e_t2": "bishui_e", "bishui_e_t3": "bishui_e",
                    "hanbing_chan_t2": "hanbing_chan", "hanbing_chan_t3": "hanbing_chan",
                    "fei_she_t2": "fei_she", "fei_she_t3": "fei_she",
                    "huoyan_shu_t2": "huoyan_shu", "huoyan_shu_t3": "huoyan_shu"}
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
    with_busts = [person for person in PEOPLE if person.busts]
    for person in with_busts:
        busts[person.stem] = [load_rgba(src / person.src_dir / f"{person.prefix}_bust_{n}.png") for n in person.busts]
    cell = portrait_cell_size([b for group in busts.values() for b in group])
    for person in with_busts:
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
    # 不去「残影」：还原成真像素之后 alpha 只有 0 / 255，看着发虚的尾巴、腹下是远处的身圈与远侧的腿，
    # 是身体的一部分（协调者 2026-11-18 定的，E.md「去掉半透明的残影」那半句作废）。
    acts = [crop_to_content(a) for a in acts]
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
        "scale": enemy.scale, "battle_scale": enemy.battle_scale,
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
    for alias, target in sorted(HD_ENEMY_ALIASES.items()):
        enemies[alias] = {**enemies[target], "alias_of": target}
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


def check_committed(out: Path) -> list[str]:
    """不需要源图的那一半：仓库里的图与索引对得上。

    · hd/index.json 的 files：每个文件都在、sha256 与磁盘一致；hd/ 下除 index.json 外没有没登记的文件；
    · portraits.json：每一条引的头像图都在，宽高放得下它的格子（头像表的形状由读取器管，它不带 sha256）；
      portraits/ 下没有没被引用的图。"""
    problems: list[str] = []
    hd_index = out / HD_REL / "index.json"
    if not hd_index.exists():
        return [f"缺  {(HD_REL / 'index.json').as_posix()}"]
    files = json.loads(hd_index.read_text(encoding="utf-8")).get("files", {})
    for name, digest in sorted(files.items()):
        target = out / HD_REL / name
        if not target.exists():
            problems.append(f"缺  {(HD_REL / name).as_posix()}（index.json 登记了）")
        elif hashlib.sha256(target.read_bytes()).hexdigest() != digest:
            problems.append(f"变  {(HD_REL / name).as_posix()}（sha256 与 index.json 不一致）")
    for f in sorted((out / HD_REL).glob("*")):
        if f.is_file() and f.name != "index.json" and f.name not in files:
            problems.append(f"多  {(HD_REL / f.name).as_posix()}（index.json 没登记）")
    table_path = out / PORTRAIT_JSON_REL
    if not table_path.exists():
        return problems + [f"缺  {PORTRAIT_JSON_REL.as_posix()}"]
    used: set[str] = set()
    for role, entries in sorted(json.loads(table_path.read_text(encoding="utf-8")).get("portraits", {}).items()):
        if not isinstance(entries, list) or not all(isinstance(e, dict) and "file" in e for e in entries):
            problems.append(f"形  {PORTRAIT_JSON_REL.as_posix()} 的 {role}：应当是头像列表（每条一个带 file 的对象），"
                            f"是改版前一个角色一个对象的写法？")
            continue
        for entry in entries:
            image = out / PORTRAIT_REL / entry["file"]
            used.add(entry["file"])
            if not image.exists():
                problems.append(f"缺  {(PORTRAIT_REL / entry['file']).as_posix()}（{role} 的头像）")
                continue
            with Image.open(image) as im:
                need = (entry["frame_w"] * entry["cols"], entry["frame_h"])
                if im.width < need[0] or im.height < need[1]:
                    problems.append(f"小  {(PORTRAIT_REL / entry['file']).as_posix()}：{im.size}，格子要 {need}")
    for f in sorted((out / PORTRAIT_REL).glob("*")) if (out / PORTRAIT_REL).exists() else []:
        if f.is_file() and f.name not in used:
            problems.append(f"多  {(PORTRAIT_REL / f.name).as_posix()}（portraits.json 没引用）")
    return problems


def same_pixels(a: bytes, b: bytes) -> bool:
    """两份 PNG 解码后尺寸与 RGBA 像素是否相同。"""
    with Image.open(io.BytesIO(a)) as ia, Image.open(io.BytesIO(b)) as ib:
        return ia.size == ib.size and ia.convert("RGBA").tobytes() == ib.convert("RGBA").tobytes()


def check_repack(src: Path, out: Path) -> tuple[list[str], list[str]]:
    """要源图的那一半：从源图重新打一遍，与磁盘上的比。返回 (不同步, 提示)。

    图比的是解码后的尺寸与 RGBA 像素：PNG 的字节取决于本机 Pillow / zlib 的压缩，换机器或升级后没改过的图
    也可能字节不同——像素相同、字节不同只提示一行，不算不同步。hd/index.json 里的 sha256 跟着字节走，
    所以比索引时不比 files 的值（只比登记了哪些文件）；sha256 与磁盘一致由 check_committed 另核。"""
    files, _ = build(src)
    problems: list[str] = []
    notes: list[str] = []
    for rel, data in sorted(files.items()):
        target = out / rel
        if not target.exists():
            problems.append(f"缺  {rel.as_posix()}")
            continue
        disk = target.read_bytes()
        if disk == data:
            continue
        if rel.suffix == ".png":
            if same_pixels(disk, data):
                notes.append(f"{rel.as_posix()}：像素相同、PNG 字节不同（本机压缩库不同），不算不同步")
            else:
                problems.append(f"变  {rel.as_posix()}（像素与从源图重新打包的不一致）")
            continue
        if rel == HD_REL / "index.json":
            mine, theirs = json.loads(disk), json.loads(data)
            listed = sorted(mine.get("files", {})) == sorted(theirs.get("files", {}))
            mine.pop("files", None)
            theirs.pop("files", None)
            if listed and mine == theirs:
                notes.append(f"{rel.as_posix()}：只有 sha256 跟着 PNG 字节不同，内容相同")
                continue
        problems.append(f"变  {rel.as_posix()}（与从源图重新打包的不一致）")
    for d in (HD_REL, PORTRAIT_REL):
        for f in sorted((out / d).glob("*")) if (out / d).exists() else []:
            if f.is_file() and f.relative_to(out) not in files:
                problems.append(f"多  {f.relative_to(out).as_posix()}（打包脚本已不产出它）")
    return problems, notes


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="手绘高清美术打包")
    ap.add_argument("--src", type=Path, default=None,
                    help=f"定稿图目录（不入库；缺省取环境变量 {SRC_ENV}，再没有就是仓库里的 docs/companions-overhaul，"
                         "那里不在就当没有源图）")
    ap.add_argument("--out", type=Path, default=REPO, help="输出根（缺省：仓库的 fanren/）")
    ap.add_argument("--check", action="store_true",
                    help="只比对：图与索引的 sha256 对得上；有源图时再从源图重打一遍比像素")
    args = ap.parse_args(argv)
    src = args.src if args.src is not None else default_src()
    have_src = src.is_dir()
    if args.check:
        problems = check_committed(args.out)
        notes: list[str] = []
        if have_src:
            repacked, notes = check_repack(src, args.out)
            problems += repacked
        for line in problems:
            print("  " + line)
        for line in notes:
            print("  提示：" + line)
        if problems:
            print(f"HD_PACK_OUT_OF_SYNC：{len(problems)} 处不一致")
            return 1
        if not have_src:
            # 只核了一半：换一个标记，免得只抓 HD_PACK_IN_SYNC 的人当成全过。
            print(f"HD_PACK_IN_SYNC_PARTIAL：图与索引的 sha256 一致；源目录 {src} 不在，"
                  "「从源图重新打包」那一半跳过，没核")
            return 0
        print("HD_PACK_IN_SYNC：图与索引的 sha256 一致；从源图重新打包，像素与索引都相同")
        return 0
    if not have_src:
        print(f"hd_pack: 源目录 {src} 不在，打不了（用 --src 指定）")
        return 1
    files, notes = build(src)
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
