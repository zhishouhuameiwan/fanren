# 地图美术：烘焙管线（A1）

地图、战斗背景、标题背景全部由 `tools/artgen/` 程序化生成，产物在 `assets/art/`，逻辑不变
（逻辑格仍 32，美术 16px/格、运行时 ×3 显示）。本文说明生成规则、主题表、`meta.json` 格式、
怎么加主题、以及门禁怎么用。人物精灵与特效（`sprites*.py`、`fx*.py`）是 A2 那一路，这里不写。

## 1. 入口与门禁

```
python tools/artgen/artgen.py                    # 全部生成（地图 + 战斗背景 + 标题；A2 的精灵/特效在就一并跑）
python tools/artgen/artgen.py --only maps        # 只生成一类：maps|backdrops|title|sprites|fx（可逗号分隔）
python tools/artgen/artgen.py --map ch05_mofu    # 只烘一张图
python tools/artgen/artgen.py --check            # 门禁
python tools/artgen/preview.py ch05_mofu --time night --out x.png   # 预览（×3，粗略光照）
python tools/artgen/preview.py --backdrop manor_night               # 战斗背景四层合成 + 占位人
python tools/artgen/preview.py --title
```

- `--check`：重生成到临时目录，与 `assets/art/` 比对——PNG 比 PIL 读出的 RGBA 像素（不比字节，
  zlib 版本不同压出的字节可以不同），JSON 比解析后的内容；生成器负责的目录里多出来的旧文件也算差异。
  全一致打印 `ARTGEN_IN_SYNC：N 个文件…` 退出 0，否则逐条列出 `缺/变/多` 退出 1。全量约 10–12s。
- 确定性：随机一律由 (图 id, 格坐标, 用途) 经 crc32 派生种子（`pix.seed_of` / `hash01`），
  不用 Python 的 `hash()`；同输入同输出，`--check` 连跑两次都一致。
- 改了生成器或 `data/visual/*.json` 之后必须重跑 `artgen.py` 再提交，否则 `--check` 报「变」。

## 2. 输出

| 路径 | 内容 |
|---|---|
| `assets/art/maps/<id>/below.png` | W×16 × H×16，地面 + 覆盖层 + 建筑/山石/道具，画在人物下面 |
| `assets/art/maps/<id>/above.png` | 同尺寸、透明：树冠、屋脊/檐口/门楼、崖檐松枝、床顶等，画在人物上面 |
| `assets/art/maps/<id>/meta.json` | 光照、粒子、水面、发射点、战斗背景（见 §5） |
| `assets/art/battle/<backdrop>/{sky,far,mid,ground}.png` | 320×180 带透明，运行时 ×4；sky/far/mid 横向首尾相接可视差平移，ground 有透视（消失点居中）不平移 |
| `assets/art/title/{sky,far,mid,near}.png` | 320×180；near 是右下一角亭台 + 左上一枝松，不平移 |

## 3. 画法约定（像素质感）

- 画布存「渐层 id + 明度级 + 不透明」；出图时 4×4 Bayer 量化。**纹理只用整数明度级**（成簇的像素），
  小数明度级只给窄的明暗过渡（接触阴影、影子边、地平线那一条雾）。整片小数级 = 满地棋盘格噪点，
  需要「比常用级亮/暗一点」时用 `mapmat.shifted(rid, t)` 把渐层挪小半档。
- 取色一律经 `palette.c()/mix()` 进 `mapmat` 的具名渐层，不写 RGB 常量；`palette.py` 只追加不改。
- 纹理按整图绝对坐标算（石板错缝、土路斑驳、瓦垄天然跨格连续，无接缝）；软质地面之间按
  格心插值 + 噪声取最大者，边界弯曲；硬质铺面按格切齐，交界压 1px 路沿、草叶伸过来。
- 统一左上光：实体登记进 `solid_px/tall_px`，`mapshade` 一次做投影（高物 5,4px、矮物 2,2px，
  内部整一档暗、只有边缘抖动）与立面脚下的接触阴影。
- 宁可简洁统一：花草只长在 gid 6，碎石只落在 gid 7；装饰不改变「看上去能不能走」
  （贴花只许贴在挡路格上，`paint_decals` 会查）。

## 4. 结构推断与自动拼接（`mapinfer.py`）

瓦片语义（与 `src/game/MapArt.h`、`genmaps.py` 同一契约）：1 主地表 2 路 3 特殊（挡路=水面，可走=翻土/碎石）
4 front 遮挡 5 墙体 6 花草 7 碎石 8 篱笆/摊架/家具。

- **树**：front 成团、底下一格是树干 → 树；树种按主题/季节表轮换（松、阔叶、桃、槐、柳、竹、枫、银杏），
  maps.json `trees` 可指定。树冠进 above，冠影压扁落在右下。
- **墙**：墙格南邻不是墙 → 立面，南邻也是墙 → 屋顶/墙顶。形态学开运算分出「细墙」（院墙、隔墙）
  与「厚块」（房子、山石）。
- **房子（roofed）**：厚墙块按立面上的门窗节奏切成一户户（lot）：立面每 4–6 格一户（大宅 6–9），
  进深 4–5（大宅 5–6）；墙上的门缺口先并进它所在那户。每户：屋脊（上方三成处、户与户高低差几像素、
  两端翘角、少数有宝顶）→ 竖向瓦垄（4px 一垄，瓦沟一线暗通到檐口，筒瓦每 6px 一节）→ 檐口（瓦当 + 暗线）
  → 立面（两格高：梁、柱、窗、门、台基、墙根返潮；门挑在门前最像门口的格）。相邻两户之间一道山墙墙头。
  瓦色按户轮换（青瓦：青灰/黛青/旧瓦；黛瓦：黛/黛蓝/青灰；琉璃：深青两种；茅草：新/旧），单栋的屋也按栋轮换。
  宗门、府邸宽 ≥8 格的殿用歇山顶（两端侧坡暗一档 + 垂脊）。屋顶上零星有天窗、烟囱、晾衣竹竿（府邸不晾衣）。
- **院落（compound）**：maps.json `structures` 指成 `built`+`compound` 的块，外圈画成院墙
  （粉墙/石墙 + 台基，不开窗），墙上的门洞画成府门（黑漆门、门钉、匾、石狮、门楼）。
- **屋内（rooms）**：墙围住的可走区（有门洞通外面）剖开画：地面整体暗一档；宗门是方砖、府邸/镇上是木地板、
  村屋是泥地；宗门与府邸的堂屋从门口铺一条红毡到后墙前。
- **front 行**：在房子南邻的可走行上 → 檐口/布棚（南檐，进 above）；在北侧整栋全宽 → 屋脊那一段进 above；
  院墙/府墙下 → 墙里伸出来的树枝；在岩体上 → 崖檐装饰（松枝、藤、洞顶垂石）；其余不画。
- **山石（rock）**：崖面按高度 1–3 行（`MAX_FACE`），竖向石柱纹、顶沿参差、崖脚两行暗、垂草；
  顶面按主题：rocky（低对比台面 + 疏草 + 矮松）、forest（林冠，边上伸出树冠）、grass（荒草坡）、
  shrubby（谷壁爬满灌木）、void（洞穴/室内墙顶的暗）。落日峰 `border: abyss` 画成崖沿 + 脚下云海。
- **水**：有机岸线、深浅、波纹、岸沫、岸上一圈湿；渡口加芦苇，谷地/府邸加荷叶。水格写进 meta `water`。
- **铺地与砌墙分开**：府邸院子（manor 的石板路）铺 14px 大方砖、室内砖地是 12px 方砖，缝只暗一档、
  砖面比常用级亮一档；错缝的长条小砖只留给墙面——地上铺错缝小砖，×3 下读起来就是一面墙。
- **路面**：土路软边；石板路错缝；水边的路格是栈桥木板；穿坡的路是石阶；满地 gid 7 的空地是铺过的
  （宗门沙地、镇/谷/府石坪、村里夯土）；翻土是 5px 垄沟 + 土块。

## 5. `meta.json`

坐标一律是格（可带小数，格心 = x+0.5）。

```json
{
  "map": "ch05_mofu", "tile": 16, "width": 48, "height": 36,
  "theme": "manor", "time": "night", "season": "autumn",
  "ambient": [84, 96, 142], "dof": 0.35, "bloom": 0.6, "vignette": 0.5,
  "particles": [{"kind": "leaf", "rate": 0.3}],
  "lights": [{"x": 38.5, "y": 8.5, "r": 2.0, "color": [235, 210, 136], "intensity": 0.55, "flicker": 0.05, "kind": "window"}],
  "water": [[12, 20], [13, 20]],
  "emitters": [{"kind": "leaf", "x": 17.5, "y": 9.0}],
  "backdrop": "manor_night"
}
```

- `time`：`day|dusk|night|indoor`；`ambient/dof/bloom/vignette` 缺省按 time 取
  （day 236,232,222/0.6/0.2/0.25；dusk 218,164,132/0.6/0.45/0.4；night 84,96,142/0.55/0.6/0.5；
  indoor 132,116,98/0.4/0.5/0.45），maps.json 写了就覆写。`season` 是附带信息（spring|summer|autumn）。
- `particles.kind`：dust firefly petal leaf rain snow ember mist。
- `lights.kind`：furnace（丹炉/药炉/地火，全天）、save（存档点，全天）、lantern（镇/府/宗门屋门口一对，
  黄昏半数、夜里全点）、window（窗纸透光，黄昏三成、夜里七成）、stone_lantern（非白天）、
  candle / sconce（室内的烛台、壁灯）、torch / fire（洞里崖面火把、守卫火盆）。白天不点灯——
  既省运行时的光照预算，也不显得假。
- `emitters`：花瓣（桃树）、落叶（秋天的阔叶树）、余烬（炉、火把）、雾（水边）、萤火（夜里草丛）从哪一格冒出来。

## 6. `data/visual/maps.json`（人工维护）

每张图一条：`theme / time / season / why / particles / backdrop`，推断不了的再加（每条都要写 `why`）：

| 字段 | 作用 |
|---|---|
| `zones` | `{"rect":[x0,y0,x1,y1], "ground"/"road"/"special": 地面类型}`：按 gid 1/2/3 覆写地面（最后生效） |
| `structures` | `{"rect", "kind": "built"|"rock"|"rockery"|"wall", "roof", "wall", "style", "compound"}`：指认结构 |
| `props` | `{"at":[x,y], "kind"}`：指认道具（石碾、药柜、床、丹炉、兵器架……） |
| `trees` | `{"at":[x,y], "species"}`：指定树种 |
| `decals` | `{"at", "w", "h", "kind": "boat"}`：只许贴在挡路格上 |
| `border` | `"abyss"`：图边外圈画成崖外云海 |
| `ambient` / `dof` / `bloom` / `vignette` | 覆写时辰缺省值；每张图都写了 `dof` 与 `dof_why`（室外 0.55–0.65，室内 0.35–0.45，窄图取低免得糊到主角） |
| `marks` | 可走地面上的平贴花：脚印、拖痕、零星碎石与草、旗杆与兵器架的长影（`mapmarks.py`，只压暗/提亮一档，不改碰撞） |

地面类型：grass earth sand gravel dirt_road stone_road flagstone steps plank tilled scree scorched
wood_floor stone_floor tile_floor carpet cave_floor cave_path courtyard。

### 每张图的预设

| 图 | 主题 | 时辰 | 季节 | 战斗背景 | 人工指认 |
|---|---|---|---|---|---|
| ch01_hanjiacun | village | day | spring | village_road | trees |
| ch01_qingniuzhen | town | day | spring | town_street | - |
| ch01_caixiashan | mountain_path | day | spring | mountain_forest | - |
| ch01_qixuanmen | sect | day | spring | sect_courtyard | zones |
| ch01_liangu_ya | cliff | day | spring | cliff_top | - |
| ch01_shenshougu | valley | day | spring | herb_valley | - |
| ch02_yaopu | valley | day | spring | herb_valley | props, trees |
| ch02_jusuo | indoor_wood | indoor | summer | inn_hall | zones, props |
| ch02_yabi | cliff | dusk | autumn | cliff_top_dusk | - |
| ch02_wairentang | sect | day | summer | drill_ground | zones |
| ch03_mishi | indoor_stone | indoor | summer | secret_room | zones, props, ambient |
| ch03_cangshu | indoor_stone | indoor | summer | secret_room | props, ambient |
| ch03_andao | cave | indoor | summer | cave_tunnel | props, ambient |
| ch03_guwai | wild | dusk | autumn | mountain_forest_dusk | structures |
| ch04_getang | sect | day | summer | sect_courtyard | - |
| ch04_luorifeng | cliff | dusk | autumn | cliff_top_dusk | border |
| ch04_yanwuchang | sect | day | summer | drill_ground | zones, props |
| ch04_shanxiazhen | town | day | autumn | town_street | - |
| ch05_dukou | dock | dusk | autumn | dock_river | decals |
| ch05_xicheng | town | dusk | autumn | dock_river | zones, decals |
| ch05_nancheng | town | day | autumn | town_street | structures, props |
| ch05_kezhan | indoor_wood | indoor | autumn | inn_hall | zones, props |
| ch05_mofu | manor | night | autumn | manor_night | zones, structures, props |
| ch05_dubashanzhuang | wild | night | autumn | wild_manor | structures, props |

每条的取舍理由写在 maps.json 的 `why` 里。

## 7. 主题表（`mapthemes.py`）

| 主题 | 类 | 屋顶 | 墙 | gid1 / gid2 / gid3 | 岩体 / 顶面 | 用在 |
|---|---|---|---|---|---|---|
| village | built | 茅草 | 夯土 | 草 / 土路 / 翻土 | cliff / 荒草 | 韩家村 |
| town | built | 青瓦 | 青砖 | 草 / 石板路 / 翻土 | cliff / rocky | 青牛镇、山下镇、南城、西城 |
| sect | built | 深青琉璃 | 宗门粉墙 | 草 / 石板路 / 石阶 | cliff / rocky | 七玄门、外刃堂、阁堂、演武场 |
| manor | built | 黛瓦 | 府邸粉墙 | 草 / 石板路 / 翻土 | valley / 荒草 | 墨府 |
| valley | natural | 青瓦 | 夯土 | 草 / 土路 / 翻土 | valley / 灌木 | 神手谷、药圃 |
| mountain_path | natural | 青瓦 | 夯土 | 草 / 土路 / 碎石 | cliff / 林冠 | 彩霞山 |
| cliff | natural | 青瓦 | 夯土 | 草 / 石阶 / 碎石 | cliff / rocky | 炼骨崖、崖壁、落日峰 |
| wild | natural | 青瓦 | 庄墙 | 草 / 土路 / 碎石 | cliff / 林冠 | 谷外、独霸山庄 |
| dock | natural | 茅草 | 夯土 | 草 / 土路 / 翻土 | 黄土坡 / 荒草 | 渡口 |
| indoor_wood | indoor | — | 木板墙 | 木地板 / 青砖 / — | 墙顶暗 | 居所、客栈 |
| indoor_stone | indoor | — | 石墙 | 方砖 / 石板 / — | 墙顶暗 | 密室、藏书处 |
| cave | cave | — | 石壁 | 洞底 / 洞道 / — | 墙顶暗 | 暗道 |

季节决定草色与树种（spring 桃花、summer 浓绿、autumn 枫/银杏/落叶）。

### 加一个主题

1. `mapthemes.py` 的 `THEMES` 加一条：`kind`（built/natural/indoor/cave）、`rock`、`rock_top`、`roof`、
   `facade`、`wall`、`ground/road/special`、`paving`、`trees`（按季节）、`fence_line/single/block`。
2. 用到的新材质在 `mapmat.py` 加具名渐层（只用 `c()/mix()`）；新墙体在 `mapwall.STYLES`、新立面在
   `mapfacade.FACADES` 加一条。`mapinfer.py` 里按主题取值的几张小表（屋内地面、铺过的空地）补上这个键。
3. maps.json 里把图的 `theme` 改过去，`python tools/artgen/preview.py <图>` 看效果，满意后
   `python tools/artgen/artgen.py` 重新生成、`--check` 确认。

## 8. 战斗背景（`backdrops.py` + `data/visual/battles.json`）

敌左我右，站位带在画面 55%–85% 高（y≈99–153），中央不放抢眼的东西；地面透视，远处细节逐渐淡出
（一像素里挤好几块纹理的地方只留底色，避免雪花噪点）。

| 背景 | 时辰 | 背景 | 时辰 |
|---|---|---|---|
| village_road | day | soulsea | 识海 |
| mountain_forest / _dusk | day / dusk | town_street / _dusk | day / dusk |
| cliff_top / _dusk | day / dusk | manor_night / manor_court | night / day |
| sect_courtyard | day | dock_river | dusk |
| drill_ground / _night | day / night | wild_manor | night |
| herb_valley | day | inn_hall | indoor |
| secret_room | indoor | cave_tunnel | indoor |

`battles.json`：`battle_id → {backdrop, map, why}`，覆盖 b03_*–b05_* 全部剧情战与切磋（23 场）和野外遭遇
be03_*–be05_*（8 场）；没写的战斗运行时退回所在地图 meta 的 `backdrop`。剧情战按设计文档里那一仗的时辰挑；
野外遭遇按遭遇所在的那张图与它烘焙的时辰挑（走在暮色里的图上，开打时不该换成白天）。

| 背景 | 用在 |
|---|---|
| mountain_forest_dusk | b03_gu_wai_elang、b05_yesu_yelang；遭遇 be03_guwai_gulang / yezhu、be04_guwai_langqun / yezhu |
| dock_river | b05_tiequanhui_jieren；遭遇 be05_dukou_jianjing / yezhu |
| wild_manor | b05_ouyang_feitian、b05_xunzhuang；遭遇 be05_linzi_langqun / yezhu |
| drill_ground / drill_ground_night | b04_qiecuo_feiyu、b04_qiecuo_maliu / b04_gongfang_weijian、b04_gongfang_xiadu、b04_yelangbang_laifan |
| sect_courtyard | b04_qiecuo_feiyu2 |
| town_street / town_street_dusk | b04_jia_tianlong / b05_heishuixiang、b05_matou_zhuishao |
| manor_night / manor_court | b05_duobang / b05_wu_jianming、b05_yange_qiecuo、b05_qiecuo_huyuan、b05_qiecuo_lingban |
| secret_room / cave_tunnel / soulsea / inn_hall | b05_mofu_shigui / b03_andao_shishou / b03_shihai_duoshe / b05_xiaoxiangyuan |
