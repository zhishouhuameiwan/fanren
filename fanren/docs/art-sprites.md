# 人物精灵 · 特效贴图 · UI 图标（生成器说明）

> 八方旅人化改造（`docs/octopath-overhaul.md`）的「精灵 / 特效 / 图标」一路。
> 施工图第 1 节的共享决策（像素尺度、资产目录）在这里全部照办；本文只写这一路自己定的东西。

---

## 1. 一句话

`data/visual/looks.json`（人工维护的外观表）→ `tools/artgen/sprites.py` / `fx.py`（Python + PIL，确定性）
→ `assets/art/sprites/**`、`assets/art/fx/**`、`assets/art/ui/**`（生成物，进仓库，运行时只读它们）。

```
python tools/artgen/sprites.py [out_root]        # 人物、非人形敌人、地图物件 + sprites/index.json
python tools/artgen/fx.py [out_root]             # 特效贴图 + fx/index.json；UI 图标 + ui/icons.json
python tools/artgen/sprite_preview.py            # 对照表（×4）；缺省写到本轮协调者 scratchpad/sprites/，那里不存在就写系统临时目录，--out 可改
python tools/artgen/sprite_preview.py --coverage # 覆盖面核对表：地图 NPC、b03–b05 剧情战与 be03–be05 野外遭遇战的单位、主角同伴，缺图退出码 1
```

`out_root` 缺省是 `assets/art`。`tools/artgen/artgen.py`（地图烘焙那一路写）会 `import sprites, fx` 并调用
`sprites.build(out_root) -> list[Path]`、`fx.build(out_root) -> list[Path]`；`--check` 时 `out_root` 是临时目录，
逐像素比对。两个 `build` 合计约 1–2 秒。

**确定性**：没有随机数、不读时钟；噪声一律是整数哈希；渐变按像素中心的解析式算后量化；
JSON 键排序、UTF-8 无 BOM、LF。两次独立运行（不同 `PYTHONHASHSEED`）102 个文件逐像素 / 逐字节一致。

## 2. 模块

| 文件 | 管什么 |
| --- | --- |
| `sprites.py` | 入口：`build(out_root)`，写人物表、敌人表、物件图集、`sprites/index.json` |
| `sprites_core.py` | 材质画布、字符模板盖印、自动描边（按相邻材质取最暗色）、布料四级色阶 |
| `sprites_body.py` | 身体模板：长衣 / 短衣两个剪影族 × 行走 6 帧 + 战斗 8 帧；体型手术 |
| `sprites_head.py` | 脸（5 种）、发型（12 种）、头饰（11 种）、胡须（5 种）；正 / 侧 / 背三向 |
| `sprites_props.py` | 随身配件（背剑、腰刀、药篓、扁担、锄头、朴刀、葫芦、扇、拂尘、算盘、刀囊、包袱、金链、药箱、灯笼）与战斗兵刃（剑、刀、朴刀、飞刀、棍、扇）× 8 种姿态 |
| `sprites_looks.py` | 读 / 校验 / 继承外观表；衣料色表 `CLOTH`；衣型 → 区域配色 |
| `sprites_render.py` | 一条外观 → 一张 21 帧的人物表 |
| `sprites_enemies.py` | 非人形敌人（恶狼、笼中兽、僵兽、墨府尸傀、野猪）与元神光球（韩立、墨居仁、余子童） |
| `sprites_objects.py` | 地图物件：九种设施、传送箭头、门光、交互闪光、头顶气泡 |
| `fx.py` | 入口：`build(out_root)`，写特效贴图、`fx/index.json`、`ui/icons.png`、`ui/icons.json` |
| `fx_textures.py` / `fx_particles.py` / `fx_combat.py` / `fx_icons.py` | 柔光与雾纹 / 粒子 / 战斗特效与气焰 / UI 图标 |
| `sprite_preview.py` | 对照表与覆盖面核对 |

颜色：全部经主色板 `palette.py` 的 `c()` / `mix()` 取；衣料色（`sprites_looks.CLOTH`）也是主色板颜色的调和，
**没有往 `palette.py` 追加任何颜色**（见 §8 决策 D-3）。

## 3. 外观表 `data/visual/looks.json`

顶层三个对象：`archetypes`（原型）、`looks`（专门外观）、`roles`（角色 → 外观）。原型与外观字段相同、
不许重名；两者都会各生成一张人物表。

### 3.1 外观字段

| 字段 | 取值 | 说明 |
| --- | --- | --- |
| `base` | 另一条外观 / 原型的 id | 继承，只写不同的字段；`colors` 按槽位合并 |
| `name` / `note` | 文本 | 给人看的名字与设计依据（引到本作文本的 key 或 role note） |
| `gender` | `m` / `f` | 女性缺省窄一列、成年女性矮一行 |
| `age` | `child` / `youth` / `adult` / `elder` | 定身高：孩子上下身各短两行，少年各短一行 |
| `build` | `slim` / `normal` / `stout` / `fat` / `giant` / `dwarf` | 定肩宽；`giant` 帧放大到 24×32（头不变，身子加宽加高），`dwarf` 是孩子的身高、常人的肩宽 |
| `height` | 整数 −3…3 | 在年龄缺省上再加减几行（张铁少年版 `+2`：比韩立高半头）；`giant` 不能写（身高固定） |
| `skin` | `fair` / `pale` / `tan` / `dark` / `ruddy` / `corpse` | 肤色色阶 |
| `face` | `round` / `narrow` / `old` / `fierce` / `blank` | 缺省：孩子、少年、女性 `round`，成年男子 `narrow`，老者 `old` |
| `brows` / `blush` / `mole` | 布尔 | 浓眉 / 腮红 / 嘴角一颗痣 |
| `hair` | `topknot` `topknot_fringe` `ponytail` `bun_low` `loose` `wild` `bald` `tufts` `twin_buns` `high_bun` `maiden` `woman_bun` | 发髻、额前碎发的发髻、马尾、脑后松束、披发、乱发、光头、总角、双髻、高髻、垂髻、农妇低髻 |
| `hair_color` | `black` / `darkbrown` / `brown` / `grey` / `white` | |
| `band` | 衣料色名 | 髻根发带的颜色（缺省同发色，即看不出来） |
| `headwear` | `none` `ribbon` `scarf` `bandana` `bamboo_hat` `straw_hat` `hood` `cowl` `hairpin` `crown` `scholar_cap` `flower` `kerchief` | 飘带（韩立专用记号）、头巾、马贼头巾、斗笠、草帽、帽兜（露脸）、低兜帽（帽檐影子盖住眉眼，曲魂）、簪、小冠、方巾、头花、包头巾；帽类会把被罩住的头发裁掉 |
| `headwear_colors` | `[主色, 副色]` | 缺省 `["ma", "he"]` |
| `outfit` | 见 3.2 | 衣型 |
| `colors` | `{main, sub, belt, trim, shoe, wrap, armor}` | 衣料色名；`main` 必填 |
| `props` | 配件 id 列表 | 见 §2 的 `sprites_props.py` |
| `weapon` | `none` `fist` `sword` `dao` `knife` `staff` `fan` `podao` | 战斗帧手里拿什么；缺省取配件里的兵刃（背剑 → `sword`、朴刀 → `podao`），都没有就 `fist` |
| `beard` | `none` `stubble` `mustache` `goatee` `full` `long` | |

所有字段、取值都是白名单，拼错当场报错（外观表是给人改的，静默退回缺省只会在预览里才被发现）。

### 3.2 衣型

| 衣型 | 剪影族 | 区域配色（上身 / 袖 / 下摆 / 裤） | 其他 |
| --- | --- | --- | --- |
| `changshan` 交领长衫 | 长衣 | main / main / main / sub | 领口缺省杏白（露出中衣） |
| `daopao` 道袍 | 长衣 | main / main / main / sub | 宽袖；领缘用副色 |
| `kuanpao` 宽袍 | 长衣 | main / main / main / sub | 宽袖 |
| `ruqun` 襦裙 | 长衣 | main / main / **sub** / sub | 齐腰裙：腰带以下换成裙色 |
| `shanqun` 大袖衫裙 | 长衣 | main / main / sub / sub | 宽袖 |
| `duanda` 短打 | 短衣 | main / main / main / sub | 绑腿 |
| `duanhe` 短褐 | 短衣 | main / main / main / sub | 领口同衣色（粗布） |
| `jinzhuang` 劲装 | 短衣 | main / main / main / sub | |
| `pijia` 皮甲 | 短衣 | **armor** / main / armor / sub | 上身与甲裙用 `colors.armor`（缺省 `pi`） |
| `open` 敞怀短打 | 短衣 | main / main / main / sub | 正面两襟敞开露胸，侧面露出前胸 |
| `beixin` 坎肩 | 短衣 | **sub** / main / sub / main | 坎肩罩在衫外 |
| `doupeng` 斗篷 | 长衣 | main / main / **sub** / sub | main 是斗篷、sub 是底下的袍子：斗篷盖住腰带、再往下披 3 行（最后一行压暗当下摆），宽袖 |

衣料色名（`sprites_looks.CLOTH`，都由主色板调出来）：
青蓝 `qinghui` 青灰（韩立）、`qingbu` 青布、`zangqing` 藏青、`tianqing` 天青、`lanshan` 蓝衫、`shuilv` 水绿、`cang` 苍、`huilv` 灰绿、`songlv` 松绿；
灰黑白 `huibu` 灰布、`qianhui` 浅灰、`hei` 黑、`xuan` 玄、`bai` 白、`xingbai` 杏白；
土褐 `ma` 本色麻、`he` 褐、`tuhuang` 土黄、`cha` 茶、`zhe` 赭；
红紫 `zhehong` 赭红（野狼帮）、`zhu` 朱红、`jiang` 绛、`zi` 紫、`ouhe` 藕荷、`fen` 桃粉、`yanzhi` 胭脂；
黄金 `huang` 黄衫、`xiang` 香色、`jin` 金；皮 `pi` 皮甲、`heipi` 黑皮。
也可以直接写主色板里的名字（如 `jade3`）。

### 3.3 `roles`

```json
"han_mu": "han_mu",
"mo_juren": {"look": "mo_daifu", "note": "与墨大夫同一具身子"},
"hanli": {"look": "hanli", "variants": [{"max_chapter": 2, "look": "hanli_child"}]}
```

`variants`：当前章（`GameState::chapter`）≤ `max_chapter` 时用变体（也可写 `min_chapter`：≥ 时用）；每条至少写一个，都是整数；
多条按顺序取第一条命中的。
韩立、张铁第 1–2 章用少年版。第 6 章以后的角色按原型兜底（`arch_*`）；海兽等非人形暂无图，运行时退回旧画法。

### 3.4 设计原则（为什么这样画）

- **不编造原著外貌**：只按身份、年纪、气质类型化；外貌细节只取本作文本里写明的
  （例：墨大夫「头发全白了，在脑后松松束着一截……青布袍子」；金光上人「三尺来高……红袍滚着金线，手上脖子上都是金」；
  贾天龙「个子不高，脸白……一件寻常青布袍子，手上什么也没拿」；曲魂「从头到脚罩着一件宽袍，帽兜压得很低」「斗篷底下那位」——
  第 5 章文案里的「斗笠」是笔误（docs/ch05-review.md LOW-6），不照它画）。
  每条外观的 `note` 写着依据。
- **韩立可辨**：衣色朴素（青灰），不比路人花哨；靠剪影认人——髻根一截天青发带，正面、侧面、背面都飘着，
  全作只有他一个人这样扎；童年版保留同一个发带，第 3 章不用重新认人。
- **势力配色**：野狼帮赭红、铁拳会黑与铁灰、独霸山庄绛红、四平帮藏青——同一帮的人站在一起一眼归堆。

## 4. 人物表布局 `sprites/chars/<look_id>.png`

每帧 16×24（巨汉 24×32），3 列一行：

| 行 | 内容 | 帧号 |
| --- | --- | --- |
| 0 | 行走·下：站、迈甲、迈乙 | 0 1 2 |
| 1 | 行走·左 | 3 4 5 |
| 2 | 行走·右（左向帧水平翻转，光仍当左上来看） | 6 7 8 |
| 3 | 行走·上 | 9 10 11 |
| 4 | 战斗：待机 1、待机 2（上身压低一格的呼吸）、蓄势（起手：压低重心、兵刃收到身后） | 12 13 14 |
| 5 | 战斗：出招（前冲递出）、施法（剑指）、受击（后仰） | 15 16 17 |
| 6 | 战斗：倒地（单膝跪倒、手撑地）、胜利（抱拳）、防御（兵刃竖在身前） | 18 19 20 |

- **战斗帧一律面向左**（我方站在右侧面朝敌人）；敌方人形单位运行时水平翻转。
- 走路播放序 `walk_cycle = [0, 1, 0, 2]`（站、迈甲、站、迈乙），建议 8 帧/秒。迈步帧身子低一格（上下起伏）、手臂前后摆。
- 头部比例：头约 9–10 行、身约 13 行的 Q 版；1px 描边取相邻材质的最暗色（不是一圈纯黑）；每种材质四级明暗、左上光。
- 放置：把帧内的点 `(frame_w / 2, foot_y + 1)` 对准人物脚底（格子底边中点）。人物表的 `foot_y` 恒为
  `frame_h - 1`（脚底描边压在最后一行），所以就是「帧的底边中点」；非人形敌人的帧下面可能留空，必须用 `foot_y`。
  `head_y` 是最高的不透明行（名牌、头顶气泡往上放）。

## 5. `sprites/index.json`

```jsonc
{
  "version": 1, "frame_w": 16, "frame_h": 24, "anchor": "bottom-center",
  "walk_cycle": [0, 1, 0, 2], "walk_fps": 8,
  "sheets": {                                  // 外观 id → 人物表（含 arch_* 原型）
    "hanli": {
      "file": "chars/hanli.png", "name": "韩立（第 3 章起）",
      "frame_w": 16, "frame_h": 24, "cols": 3,
      "walk":   {"down": [0,1,2], "left": [3,4,5], "right": [6,7,8], "up": [9,10,11]},
      "battle": {"idle": [12,13], "ready": [14], "attack": [15], "cast": [16],
                 "hurt": [17], "down": [18], "victory": [19], "guard": [20]},
      "battle_facing": "left", "foot_y": 23, "head_y": 0, "weapon": "sword"
    }
  },
  "roles": {"han_mu": "han_mu", "mo_juren": "mo_daifu", ...},       // 角色 → 外观（缺省）
  "role_variants": {"hanli": [{"max_chapter": 2, "look": "hanli_child"}], ...},
  "enemies": {                                 // 非人形敌人与光球，面朝右，不翻转
    "wild_wolf": {"file": "enemies/wild_wolf.png", "kind": "beast", "frame_w": 32, "frame_h": 24, "cols": 5,
                  "battle": {"idle": [0,1], "attack": [2], "hurt": [3], "down": [4]},
                  "battle_facing": "right", "foot_y": 22, "head_y": 3},
    "yu_zitong": {"file": "enemies/yu_zitong.png", "kind": "orb", "frame_w": 40, "frame_h": 40, "cols": 6,
                  "battle": {"idle": [0,1,2,3], "hurt": [4], "down": [5]}, "float": true, ...},
    "heishajiao_shigui": {..., "alias_of": "mofu_shigui"}
  },
  "battle_overrides": {"b03_shihai_duoshe": {"hanli": "hanli_yuanshen"}},   // 识海之战韩立是光球
  "objects": {
    "file": "objects.png", "emissive": "objects_emissive.png", "cell": 16,
    "items": {"facility.alchemy": {"name": "丹炉", "frames": [[x,y,w,h], ...], "anchor": [16,28],
                                   "fps": 8.0, "blend": "alpha", "note": "..."}, ...}
  },
  "facilities": {"alchemy": "facility.alchemy", "forge": "facility.forge", ...}   // map_spec 4.6 的 kind → 物件
}
```

帧号 → 像素：`x = (i % cols) * frame_w`，`y = (i / cols) * frame_h`（物件直接给矩形）。
取人物：`role_id` → 先看 `battle_overrides[战斗id]`，再看 `role_variants`（按当前章），再看 `roles`；
非人形单位查 `enemies`；都没有 → 旧画法 + 一行 WARNING（施工图 1.2「缺图不崩」）。

物件：`anchor` 是帧内的落地点（像素）——最低一行不透明像素的下沿、那一行的中点，由生成器从图里算出（不手填，
改模板不会让物件悬空）；运行时对到对象所在格的底边中点。平铺在格子上的标记（传送箭头、闪光）一帧就是一格，
锚点取帧底边中点 (8,16)；头顶气泡的锚点是尾尖，对到 NPC 的 `head_y` 上方；
物件帧宽高都是 16 的倍数（以 16px 为一格，丹炉 / 香炉 / 货摊 / 告示板占 2×2 格，灵田木牌、门光占 1×2 格），
图集里的位置也就对齐到格。`blend = add` 的（传送箭头、门光、闪光）是纯光，运行时加色；`objects_emissive.png` 与 `objects.png` 同尺寸、同位置，
只留发光像素（丹炉 / 器炉的火、阵盘的纹、光类物件），给辉光目标用。
头顶气泡：`bubble.exclaim`（！）、`bubble.ellipsis`（…）、`bubble.path`（通用路径行动）、
`bubble.path_inquire`（打探）、`bubble.path_buy`（求购）、`bubble.path_duel`（切磋）。

## 6. 特效与图标

`fx/index.json`：`textures.<名>` = `{file, w, h, frame_w, frame_h, frames, rows, blend, tint?, fps?, anchor?, note}`。
多帧横排；`rows > 1` 时每行一组（`boost_aura` 三行 = 蓄劲 1/2/3 档）。`tint` 是给白 / 灰度图的建议乘色。

| 名 | 尺寸 × 帧 | 说明 |
| --- | --- | --- |
| `glow` | 64×64 | 白色径向光斑，alpha 中心 255 平滑到 0 |
| `softcircle` / `shadow` | 32×32 / 24×8 | 柔边圆 / 脚下柔影 |
| `shaft` | 32×128 | 体积光光束 |
| `fog` | 128×128 | 四边无缝的柔噪声雾纹 |
| `leaves` `petals` | 8×8 ×4 | 叶片、花瓣（灰度） |
| `embers` | 8×8 ×4 | 火星（带色） |
| `rain` `snow` `dust` `firefly` `spark` | 4×16 / 8×8×3 / 4×4 / 8×8×2 / 8×8×3 | 雨丝、雪片、尘埃、萤火、火花 |
| `shards` | 12×12 ×6 | 破势碎片（碎玉菱形片） |
| `slash` | 48×48 ×4 | 刀光弧线（弧背朝左，敌方出招翻转） |
| `impact` `wind` `poison` `heal` | 48×48 ×3 | 钝击冲击、风旋、毒雾、治愈光点 |
| `fireburst` | 48×48 ×4 | 火弹爆裂（带色） |
| `boost_aura` | 32×40 ×3 帧 ×3 档 | 蓄劲金色气焰；`anchor` (16,39) 对到人物的脚底点（人物帧底边中点），画在人物身后 |
| `boss_aura` | 32×40 ×3 | 首领「蓄势」暗红气焰；大体型敌人按体型放大 |

`ui/icons.json`：`icons.<id>` = `{name, rect}`（16×16，运行时 ×2），`categories` = 攻击类别 → 图标：
剑 `attack.sword`、刀 `attack.blade`、拳 `attack.fist`、暗器 `attack.hidden`、毒 `attack.poison`、
金 `element.metal`、木 `element.wood`、水 `element.water`、火 `element.fire`、土 `element.earth`；
另有 `mark.unknown`（？）、`mark.stance`（架势）、`mark.bp_full` / `mark.bp_empty`（劲珠）、`mark.pointer`（金菱形指针）、
`item.cultivation`（修为）、`item.silver`（碎银）、`item.medicine`（药）、`item.chest`（宝箱）。

## 7. 怎么加一个角色

1. 读这个角色的 `data/roles/<id>.json` 的 `note` 与出场文本，只挑文本里写明的外貌；其余按身份类型化。
2. 在 `looks.json` 的 `looks` 里加一条（能继承就 `"base": "<相近的外观或原型>"`，只写不同字段），`note` 写依据。
3. 在 `roles` 里加 `"<role_id>": "<look_id>"`；分年龄段的加 `variants`。
4. `python tools/artgen/sprites.py && python tools/artgen/sprite_preview.py`，看 `chars_lineup.png`
   （和别人分得开吗？）、`chars_walk_*.png`、`chars_battle_*.png`。
5. `python tools/artgen/sprite_preview.py --coverage` 确认零缺图；`artgen.py --check` 过门禁。

加一种新发型 / 头饰 / 配件：在 `sprites_head.py` / `sprites_props.py` 里按正、侧、背三向画字符模板（码表见各文件头），
取值自动进白名单。加衣型：`sprites_looks.OUTFITS` 里加一行「区域 → 颜色」映射（剪影仍是长衣 / 短衣两族之一）。

## 8. 这一路自己定的事

| # | 决策 | 为什么 |
| --- | --- | --- |
| D-1 | 人物表 3 列 7 行、21 帧；战斗帧多一张「防御」 | 施工图 2.4 有防御动作，多画一帧比运行时拿待机顶替像样 |
| D-2 | 巨汉（曲魂）帧 24×32，其余 16×24；帧尺寸写在每张表里 | 「比常人大一号」在 16×24 里画不出来；锚点仍是底边中点，运行时不用特判 |
| D-3 | 衣料色全部由主色板 `mix()` 调出，不往 `palette.py` 追加 | 另一路同时在改 `palette.py`，只加不改的颜色一旦加错就收不回；调和色天然和地图同一套色 |
| D-4 | 身体只分长衣 / 短衣两个剪影族，衣型差别用「区域 → 颜色」映射 | 16×24 里区分人靠剪影；两族 × 14 帧的手画模板就够全员，体型用插删行列的「手术」派生 |
| D-5 | 非人形敌人给 待机×2 + 出招 + 受击 + 倒下；光球 脉动×4 + 受击 + 消散 | 出招一帧是加的：野兽扑咬若拿待机帧平移，看不出是在进攻 |
| D-6 | 识海之战用 `battle_overrides` 把韩立换成元神光球 | 该战没有肉身；按战斗 id 覆写比在角色表里加假角色干净 |
| D-7 | 发光物件另出一张 `objects_emissive.png` | 施工图 1.4 的辉光管线要「只画发光体」的目标；同位置图集运行时直接画 |
| D-8 | 黑熊（hei_xiong）画成人 | role note 明写是铁拳会码头头目、使拳脚；「黑熊」是绰号 |
| D-9 | 第 6 章以后的人形角色按原型兜底，海兽 / 沼泽妖兽暂不画 | 本轮只做第 1–5 章；兜底让它们至少不是方块，兽类没有可靠的类型参照，宁缺不画错 |
