<!-- 第 13 章施工图（逆星之乱）第二部分：第 4–9 节。写手二，2026-10-13。唯一依据 notes/ch13/research.md（下称「笔记」）；笔记没有、另用 novel.py 核过的原著事实注了章号；引擎与门禁现状另用 grep 核过的注了文件与行号。同一件事实只在一处写全：节点与挂点的权威是第 3 节主线节点表，在场旗标的权威是 5.1，门闸的权威是 7 节「门闸一览」，编成的权威是 8.2，数目的权威是第 9 节；别处只引用。 -->

## 4. 地图（新建 4 张，旧图 0 处修补）

| 地图 id | 名称（`display_name_key`）| 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| default spawn | 出入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `ch13_nanming` | 南明岛 | 48×36 | ✓ | `dock` / `day` / 粒子 `mist` / backdrop `dock_river` | `haishang`（西南礁）| 无门；脚本入口 C1（`scripts/ch12/lingquan.lua` 末行 `teleport("ch13_nanming")`，落 `haishang`）；脚本出口 n06 `teleport("ch13_tianxing")` |
| `ch13_tianxing` | 天星城（戒严）| 56×44 | ✓ | `town` / `dusk` / 粒子 `dust` / backdrop `dock_river` | `chengwai`（南端码头）| 无门；脚本入口 n06；脚本出口 n12 `teleport("ch13_waihai")` |
| `ch13_waihai` | 外星海（北区文案称「奇渊岛」「黑石城」）| 72×56 | ✓ | `dock` / `day` / 粒子 `mist` / backdrop `dock_river` | `shiwu`（北区祭坛石屋内）| 无门；脚本入口 n12（落 `shiwu`）、n30（带坐标落秘市岸，建议 (14, 46)）、五元 #6（带坐标落 `trigger_hui_wuzhong` 东侧两格）；脚本出口 n16 与五元 #7 → `ch13_wuzhong` |
| `ch13_wuzhong` | 雾中岛 | 64×48 | ✓ | `valley` / `day` / 粒子 `mist` / backdrop `herb_valley`；山脉凹谷里的洞府几间石室用室内底图块画在山体里（第 10 章小寰山的画法）| `andeng`（东岸登岸礁）| 无门；脚本入口 n16 与五元 #7（都落 `andeng`）；脚本出口 n30 与五元 #6 → `ch13_waihai`（都带坐标）|

**图与图之间没有一扇门**：跨图全是脚本传送，三元 5 条、五元 2 条（笔记 N.4，登 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`）。外星海那一头原著就是「能出不能进」（A48）：n12 之后回不去内星海，所以天星城与南明岛都不留回程。主题、时辰、背景全是现有组合（笔记 G 表 2026-10-13 逐张列过 `dock/day/dock_river`、`town/dusk/dock_river`、`valley/day/herb_valley`）。全作地图 75 → 79，超大纲上限 4 张（O-6 记账，E 表 3）。

**为什么外星海是一张图三区**（E 表 4）：黑石城、秘市荒岛、裂风兽小岛原著相隔数日到一月的路，三区三图则全章 6 张；秘市单列则 5 张。合成一张、以中部礁路相连（规则 13 只从 default spawn 做 BFS，三区必须连通），区与区之间不设阻隔——三区的挂点靠 guard 旗标惰性（未到时踩了不响），不靠隔断。秘市地下两层与交换会走 n31 脚本场，不建地下图。

**落地不触发 enter**（引擎口径 1）：C1、n06、n12、n16 的落点 `haishang`、`chengwai`、`shiwu`、`andeng` 都不挂触发器，每张图的首个 enter 挂点是落点前方横贯通道的触发带（n01、n07、n13、n17），玩家迈出第一步才响；通关驱动要先迈一步。带坐标的落点（n30、#6）格上同样不放任何对象。

### 4.1 每张图的挂点与在场 NPC

各 NPC 的出场 / 退场旗标以 5.1 为准，这里只列对象与位置；各挂点的 guard 与完成旗标以第 3 节主线节点表为准。

**`ch13_nanming` 南明岛**（第一幕：n01–n06）
- 布局：西南礁（spawn `haishang`；存档点 `facility_cunji_nanming` 紧挨它、在带南侧）→ 北面一行横贯礁路的 n01 触发带 → 港口小城（原料铺）→ 码头 → 港后丘陵坡口 → 港口客栈一角 → 港口东出海口 → 出海口外礁头。
- 挂点：`trigger_wenxun`（n01，enter，横贯带）、`trigger_qiuling`（n03，enter，丘陵坡口）、`trigger_chugang`（n05，enter，出海口一行横贯带）；n02 挂 NPC `npc_yuanliao_zhanggui`、n04 挂 `npc_cang_yunlong`、n06 挂 `npc_ling_yuling`。
- 设施：`facility_cunji_nanming`（存档）、`facility_dazuo_nanming`（meditate、`effectiveness 100`，客栈一角，**在 `trigger_chugang` 之前的路上**——第 12 章 ④ 之后血未必满，① 前要有回血处，E 表 39）。
- NPC：小城 `npc_yuanliao_zhanggui`；码头 `npc_sanxian_laozhe`、`npc_cang_yunlong`、`npc_liu_hufa`、`npc_xin_ming`；出海口外礁头 `npc_ling_yuling`（在出海口带之外，n05 之后才在）。路径行动 K1–K3 挂三仙宗老者、刘护法、辛明。
- 逆星盟攻港（n03）、银衫布阵绿袍搜岛只在脚本里，不摆群众 NPC。

**`ch13_tianxing` 天星城（戒严）**（第二幕：n07–n12）
- 布局（南 → 北，按圣山层数往上）：南端码头（spawn `chengwai`；存档点 `facility_cunji_tianxing` 紧挨它、在带南侧）→ 城门前横贯带 → 城门 → 圣山脚坊市门 → 第四层宅院 → 第三十九层洞府 → 第四十九层石阶 → 第五十层外廊 → 星空殿传送大厅。
- 挂点：`trigger_rucheng`（n07，enter，`chengwai` 北面城门前横贯带）、`trigger_dongfu39`（n08，interact，洞府门）、`trigger_xingkong`（n09，enter，五十层外廊横贯带）、`trigger_zhaiyuan`（n10，interact，宅院院外）、`trigger_lanlu`（n11，enter，四十九层石阶横贯带，在 `xingkong` 之下）、`trigger_fangshi_bimen`（interact、once 否、无旗标，只一句「坊市闭门」，A34）；n12 挂 NPC `npc_gu_laozhe`。
- 设施：`facility_cunji_tianxing`（存档）；三十九层洞府里 `facility_yaoyuan_39`（field、`ref_id field_tianxing_39`、`slots 6`）与 `facility_jingshi_39`（meditate、`effectiveness 125`），门闸见 7 节。
- NPC：城门 `npc_zhao_hufa`、`npc_yu_hufa`；传送大厅 `npc_gu_laozhe`、`npc_qian_xiushi`、`npc_yi_jing_tx`、`npc_zhang_xiushi`。K4–K7 挂乾姓修士、赵护法、于护法、张姓修士。**不摆路人**：戒严，空街、凡人闭户（A30）。
- 走位次序靠 guard：n08（三十九层）→ n09（五十层，途经四十九层时 `trigger_lanlu` 的 guard `ch13.qianru` 未置、踩了不响）→ n10（下到第四层）→ n11（回四十九层）→ n12（大厅）。
- 对象名 `facility_yaoyuan_39`、`facility_jingshi_39` 与 `ch11_dongfu` 上的同名对象重名（`tools/mapgen/genmaps_ch11.py` 第 205–209 行）：对象名按图分，`validate.py` 无跨图重名规则（2026-10-13 grep）；同一个 `ref_id` 跨两图由契约 E2 钉。

**`ch13_waihai` 外星海**（第三幕 n13–n16；第五幕 n31–n34）
- **北区黑石城**：祭坛石屋（spawn `shiwu`；存档点 `facility_cunji_qiyuan` 在石屋内紧挨它）→ 石屋门外横贯带 `trigger_shiwu`（n13，enter）→ 祭坛高台与城中（明珠轩、寸金阁、告示墙）→ 西门横贯带 `trigger_chucheng`（n16，enter）→ 西门外礁头 `trigger_hui_wuzhong`（五元 #7，与西门带错开 ≥ 2 格；#6 的落点在它东侧两格）。告示墙 `trigger_xuanshang`（interact、once 否、无旗标，Z2）。
- **中部礁路**连三区，全程走得通。
- **西南秘市荒岛**：秘市岸（n30 落点，建议 (14, 46)，不与任何对象同格；存档点 `facility_cunji_mishi` 在岸旁）→ 乱石堆石壁 `trigger_mishi`（n31，interact）；东侧绿山小洞府前是文思月（n32）。
- **东南小岛**：外礁一行横贯带 `trigger_jiaoshi`（n33，enter）→ 半山巨石后洞口 `trigger_dongkou`（n34，interact）。
- NPC：北区 `npc_xu_yun`、`npc_yi_jing`（祭坛下）、`npc_ming_furen`、`npc_paimaishi`（明珠轩）、`npc_li_xun`、`npc_gu_yu`（寸金阁）；秘市荒岛 `npc_huang_mingli`（乱石堆旁）、`npc_wen_siyue`（绿山小洞府前）。K8–K11、K13 挂易敬、许云、竞宝厅老者、古玉、黄明礼；n14、n15、n32 挂明夫人、离寻、文思月（挂点 NPC 不挂路径行动，笔记 K 表末）。
- **0 战斗、0 `challenge`**：黑石城除竞斗场外严禁斗法（A52）。
- **兽潮之后的北区**：六个 NPC 全撤（5.1）；明珠轩、寸金阁本就挂在 NPC 脚本上，随人撤；告示墙脚本开头判 `ch13.shouchao` 只一句；玩家幕五走回北区只见空城，不设阻隔带（E 表 23、4）。

**`ch13_wuzhong` 雾中岛**（第四幕：n17–n30）
- **东岸**：登岸礁（spawn `andeng`；存档点 `facility_cunji_wz` 紧挨它、在带南侧）→ 北面横贯带 `trigger_lifu`（n17，enter）；码头入口一行横贯带 `trigger_lidao`（n30，enter）、码头尽头一格 `trigger_qiyuan_hang`（五元 #6，与码头带隔 ≥ 2 格）；林边光幕前 `npc_gongsun_xing`（n29 挂点）、`npc_sun_shishu`（K12）。
- **山脉凹谷洞府**：洞府甬道两条横贯带分在两行——内侧 `trigger_dizhen`（n21，enter）、外侧 `trigger_chuguan`（n28，enter）；六扇门各一格：`trigger_chongshi`（n18）、`trigger_shichuang`（n19）、`trigger_lianjianshi`（n20）、`trigger_biguanshi`（n22）、`trigger_danfang`（n26）、`trigger_jingshimen`（n27），都 interact；设施 `facility_jingshi_wz`（meditate、`effectiveness 150`）、`facility_danlu_wz`（alchemy、`grade 4`）、`facility_yaoyuan_wz`（field、`ref_id field_wuzhong`、`slots 6`），**无炼器台**（E 表 17）；药园旁一格 `trigger_yanghun`（interact、once 否、无旗标，Z3）。
- **推荐（本稿拍板）**：静室、丹炉、药园与六扇门都在**内侧带之内**——n27 之后玩家在静室反复坐关不必跨过 `trigger_chuguan`；n28 的「瓶颈未破」只在他往外走时响（`chuguan.lua` 早退不置旗，这条带每次跨过都会再判一次，直到破关）。
- **西海珊瑚礁**（同图，礁路与东岸连通）：登礁横贯带 `trigger_buzhen`（n23，enter）→ 阵心礁 `trigger_youyao`（n24，interact、once 否）→ 礁外收阵石 `trigger_shoushou`（n25，interact，与阵心隔 ≥ 2 格）。「西飞一月」只在 n23 的 `advance_days(30)` 与文案里，图上是一段礁路。
- 巨猿傀儡打理洞府、退鹰鸢兽只在文案（A83、A117），不摆 NPC。

### 4.2 命名、属性与美术

- `chapter=13`；`display_name_key = ch13.map.<短名>.name`（`nanming`、`tianxing`、`waihai`、`wuzhong`）；`region` 由协调者定一个值（笔记无；第 11 章 `luanxinghai_zhongbu`）。显示名「南明岛」（501）、「天星城」（389）、「外星海」（389）、「雾中岛」（517）都 ≤ 542。
- `bgm`：**只用已有曲子**，每图一首由关卡路挑（建议南明岛 `bgm_river`、天星城 `bgm_town`、外星海 `bgm_river`、雾中岛 `bgm_valley`）；`tools/audiogen/catalog.py` 在 `MAP_BGM` 上方的注释表加 4 行 `# ch13_xxx -> bgm_yyy`（第 45–51 行是第 11 章的样子），`docs/audio.md` 同一张表加 4 行。
- `tools/mapgen/genmaps_ch13.py`（4 张）接进 `genmaps.py --check`；`tools/artgen/artgen.py` 烘 4 张并 `--check` 过；**本章不 patch 任何旧图**（C1 是改一行脚本，不是地图）。
- 存档点 5 处（南明岛、天星城、黑石城石屋、秘市岸、雾中岛东岸），打坐设施 3 处（南明岛 100、天星城 125、雾中岛 150）。
- **同一格只取第一个 trigger**（引擎口径 4）：本章没有同格换触发器；`trigger_youyao` 与 `trigger_shoushou`、`trigger_qiyuan_hang` 与 `trigger_lidao`、`trigger_hui_wuzhong` 与 `trigger_chucheng`、`trigger_dizhen` 与 `trigger_chuguan` 都错开；spawn 与带坐标的落点格上不放对象。

### 4.3 地图的整改拍板（与笔记不一致处，推荐做法）

1. **设施不写 `deny_text_key`**：笔记 G 表给 `facility_yaoyuan_39`、`facility_jingshi_39`、雾中岛三设施写了 `deny_text_key ch13.block.yaoyuan / jingshi / dongfu`。引擎拦设施时只播 `ui.facility.locked`（`src/game/Application.cpp` 第 688–692 行），全仓只有 `WorldScene.cpp` 第 169 行的**传送点**读 `deny_text_key`；规则 20（`validate.py` 第 440–470 行）也只查传送点。写了就是三条永远播不出的文案、还要在文案表里登 key（`check_reference_pair` 查 `_text_key` 字段存在）。所以照第 11 章 `ch11_dongfu` 的写法（`genmaps_ch11.py` 第 205–209 行）**只写 `require_flag`**，这三条 key 不建。本章没有传送点，规则 20 无实例。
2. **雾中岛洞府内外两条带的相对位置**（上文「推荐」）：笔记只写「分在两行」；本稿定内侧 `dizhen`、外侧 `chuguan`，静室与丹炉在内侧带之内。

---

## 5. 角色

**新建 30、就地改写 1（`wuji_haishou`）、一个字不动 2（`fan_furen`、`wen_siyue`）**（笔记 H 表）。凡说话、上场的人都有 `data/roles/` 条目；外观进 `data/visual/looks.json`，`sprite_preview.py --coverage` 缺图为 0。境界写 realm 枚举，「只说话」的 role 数值照 `Realm.h` 锚点（笔记 C.2）与第 10–12 章路人写法；名牌就是 role 的 `name`。**万三谷、六道极圣、妙鹤真人、极阴祖师、漩矶本章 0 处上场**（只闻其名或不提，A5、A102、A109–A110）。

### 5.1 剧情角色（地图上有对象的；**在场旗标的权威表**）

| id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 在场（图 `对象`：出场 → 退场）|
| --- | --- | --- | --- | --- | --- |
| `sanxian_laozhe` | 三仙宗老者 | 499 | FoundationLate（估）| 新建 | nanming `npc_sanxian_laozhe`：`wenxun → gongdao` |
| `nanming_zhanggui` | 原料铺掌柜 | 501 | FoundationMid（估）| 新建 | nanming `npc_yuanliao_zhanggui`：`wenxun →`（不撤；封市后只一句）|
| `cang_yunlong` | 苍云龙 | 502 | CoreEarly | 新建 | nanming `npc_cang_yunlong`：`gongdao → panzha` |
| `liu_hufa` | 刘护法 | 503 | CoreEarly（估）| 新建 | nanming `npc_liu_hufa`：`gongdao → panzha` |
| `xin_ming` | 辛明 | 502 | FoundationMid（估）| 新建 | nanming `npc_xin_ming`：`gongdao → panzha` |
| `ling_yuling` | 凌玉灵 | 505 | CoreEarly（估；文案不报境界）| 新建 | nanming `npc_ling_yuling`：`zhuibing → yinjian` |
| `zhao_hufa` | 赵护法 | 505 | CoreEarly | 新建 | tianxing `npc_zhao_hufa`：`yinjian → chuansong` |
| `yu_hufa` | 于护法 | 506 | CoreMid | 新建 | tianxing `npc_yu_hufa`：`yinjian → chuansong` |
| `gu_laozhe` | 顾姓老者 | 509 | CoreEarly | 新建 | tianxing `npc_gu_laozhe`：`yirong → chuansong` |
| `qian_xiushi` | 乾姓修士 | 509 | CoreEarly | 新建 | tianxing `npc_qian_xiushi`：`yirong → chuansong` |
| `zhang_xiushi` | 张姓修士 | 507 | FoundationMid | 新建 | tianxing `npc_zhang_xiushi`：`yirong → chuansong` |
| `yi_jing` | 易敬 | 508（易兄）/ 511 | FoundationLate | 新建 | tianxing `npc_yi_jing_tx`：`yirong → chuansong`；waihai `npc_yi_jing`：`chuansong → shouchao` |
| `xu_yun` | 许云 | 511 | FoundationLate | 新建 | waihai `npc_xu_yun`：`chuansong → shouchao` |
| `ming_furen` | 明夫人 | 513 | FoundationMid | 新建 | waihai `npc_ming_furen`：`chuansong → shouchao` |
| `jingbao_paimaishi` | 竞宝厅老者 | 513 | FoundationLate（估）| 新建 | waihai `npc_paimaishi`：`mingzhu → shouchao` |
| `li_xun` | 离寻 | 515 | CoreEarly | 新建 | waihai `npc_li_xun`：`chuansong → shouchao` |
| `gu_yu` | 古玉 | 515 | QiRefining13（估：原文只说炼气期）| 新建 | waihai `npc_gu_yu`：`chuansong → shouchao` |
| `sun_shishu` | 孙师叔 | 529 | FoundationEarly | 新建 | wuzhong `npc_sun_shishu`：`chuguan → gongsun` |
| `gongsun_xing` | 公孙杏 | 531 | QiRefining（估：原文炼气期）| 新建 | wuzhong `npc_gongsun_xing`：`chuguan → gongsun` |
| `huang_mingli` | 黄明礼 | 534 | FoundationLate（假丹）| 新建 | waihai `npc_huang_mingli`：`lidao → jiaohuan` |
| `wen_siyue` | 文思月 | 430 | （第 12 章所建）| **不动** | waihai `npc_wen_siyue`：`jiaohuan → wenshi` |

（旗标一律省 `ch13.` 前缀。）`gongsun_xing` 的境界列照笔记写「QiRefining（估）」——realm 枚举没有不带层数的炼气，施工时取 `QiRefining1`–`QiRefining13` 中一个具体层（`Realm.h` 第 17–29 行），由内容路定，不影响任何判据。

### 5.2 只说话的（无地图对象，节点脚本 `talk` 用）

| id | 名牌 | 首见章 | 境界 | 在哪一段说话 |
| --- | --- | --- | --- | --- |
| `liu_furen` | 刘夫人 | 508 | FoundationEarly（估）| n10 七人议事 |
| `qiyuan_kanhu` | 看护修士 | 511 | FoundationEarly（估）| n13 石屋 |
| `qiu_laozhe` | 鸠面老者 | 524 | CoreMid | n25 |
| `min_xiushi` | 闵姓修士 | 524 | CoreEarly | n25 |
| `xuan_xiushi` | 宣姓修士 | 524 | CoreEarly | n25 |
| `yun_tianxiao` | 云天啸 | 536 | CoreMid | n31（东主；白骨阴火只在文案，不建法术）|
| `sun_heifu` | 孙姓修士 | 537 | CoreMid | n31（铜片主人）|
| `fan_furen` | 范夫人 | 400 | FoundationLate | n31（**不动**）|

### 5.3 只在战斗里的（编成用，无地图对象）

| id | 名牌 | 境界 | 新建 / 改写 | 编成 |
| --- | --- | --- | --- | --- |
| `nixing_rusheng` | 白面儒生 | CoreEarly | 新建（敌）| ① |
| `nixing_dahan` | 黑壮大汉 | CoreEarly | 新建（敌）| ① |
| `wuji_haishou` | 五级海兽 | CoreEarly | **就地改写**（B25：`weaknesses` 补「剑」，note 改写）| ② 第 0 波 |
| `qingse_juxia` | 青色巨虾 | CoreMid | 新建（敌，非人形）| ② 第 1 波 |

破绽、架势、蓄势与 `killable_by` 以 8.2 为准。

### 5.4 不建、外观

- **不建 role、只旁白**（笔记 H 表末）：三仙宗同行者、银衫 / 绿袍逆星盟众、王长老、赵护法手下一队、七人里其余四人、明珠轩带路女子、竞宝厅拍卖师与竞宝诸修、巨龟、毒蛟、金霞道士、霍氏兄弟、鹰鸢兽、巨猿傀儡、青灵门其余三人、齐姓胖老者、红巾怪人、青夫人、吴姓老者、两名青衣修士、秘市守门老者、章末那个声音（E 表 28：不写名、不现身、不建出场 role）。
- **外观**：人形按名牌身份配；**非人形 2 个**——五级海兽、青色巨虾（三四丈长、六只碧绿的眼，ch523），进 `tools/artgen/sprites_enemies.py`，待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下（第 8、10–12 章同口径）。2026-10-13 grep：`wuji_haishou` 在 `looks.json` 与 `sprites_enemies.py` 里都没有条目（第 11 章 `siji_haishou` 同样没有）——美术路先核 `--coverage` 对只在编成里的 role 怎么算，再定两张都补还是照第 11 章口径。

### 5.5 撤场 / 出场与规则 30（`NpcPresenceTests`、`validate.py` 规则 30）

- **跨图**：`yi_jing` 天星城那个的退场旗标 `ch13.chuansong` 恰是外星海那个的出场旗标，首尾相接；`wen_siyue` 第 12 章 `ch12_tushan` 那个以 `ch12.lidao` 撤场，早于本章 `ch13.jiaohuan`。其余 role 只在一张图上有一个对象。
- **同图同 role**：本章 0 例（规则 30 只放行「一个的 hidden 恰是另一个的 visible」，用不上）。
- **不同 role 同名牌**：同图同时在场的名牌两两不同（逐图核过：南明岛 6 个、天星城 6 个、外星海 8 个、雾中岛 2 个）；「孙师叔」（wuzhong）与「孙姓修士」（只说话）、三个「老者」（三仙宗 / 顾姓 / 竞宝厅）都不同名、不同图或不同时。
- **链外在场旗标**：黑石城六个 NPC 的退场旗标 `ch13.shouchao` 不在目标链上（笔记 J.2）——登 `kOffChainFlags`，`after ch13.chuguan`、`before ch13.gongsun`（`gongsun.lua` 里先置 `shouchao`、后置 `gongsun`）。其余在场旗标全在目标链上。

---

## 6. 支线（5 条：必经 2、可选 3）

| | `q13_qianyelu`「千叶露」| `q13_gongsun_qiuyao`「公孙杏之求」| Z1 `q13_mingzhu_dingdan`「明珠轩订单」| Z2 `q13_haishou_xuanshang`「海兽悬赏」| Z3 `q13_yanghun_zhu`「养魂木珠」|
| --- | --- | --- | --- | --- | --- |
| 必经 / 可选 | **必经** | **必经** | 可选 | 可选 | 可选 |
| 出处 | ch501 掌柜不识千叶露（A12）→ ch516 古玉典籍查明（A63）→ ch540 文思月给巢穴位置（A142）| ch531–533 为瘫父求药（A120–A123）| 改编：大纲「商行订单」；明珠轩收购海外所得（A54）| 改编：黑石城告示墙「悬赏制度」| ch518 新绿液催养魂木（A76）、ch523 已成形 |
| 接取 | `ch13.caigou` | `ch13.chuguan` | `ch13.mingzhu` | `ch13.daoqiyuan` | `ch13.lifu` |
| 步骤与挂点 | s1 `ch13.cunjin` → s2 `ch13.wenshi`；无独立挂点，全在主线 | 无独立挂点，即 n29 | `mingzhu.lua` 再谈分支：六级妖丹 ≥ 1 才列「交货」→ `take` 1、`give("material_lingshi_zhong", 30)`、置 `ch13.z1` | `trigger_xuanshang`：开头判 `ch13.shouchao` 一句 return、判 `ch13.z2` 已置一句 return；五级妖丹 ≥ 3 才列「交」→ `take` 3、`give` 中阶 15 ＋ 霓裳草 3、置 `ch13.z2` | `trigger_yanghun`：`ch13.buzhen` 未置一句（木未成形）return；`ch13.z3` 已置一句 return；根须 ≥ 1 且绿液 ≥ 3 → `bottle.spend(3)`、`advance_days(60)`、`take` 根须 1、`give` 养魂木珠 1、置 `ch13.z3`；不足一句 |
| 完成 | `ch13.done` | `ch13.gongsun` | `ch13.z1` | `ch13.z2` | `ch13.z3` |
| 失败 / 过期 | — | — | `ch13.biguan` | `ch13.biguan` | `ch13.lidao` |
| 奖励 | 不进 rewards | 不进 rewards | 不进 rewards（脚本 `give`）| 同左 | 同左 |
| 文件 / origin | `data/quests/q13_qianyelu.json`，原著 | `q13_gongsun_qiuyao.json`，原著（改编：去炉鼎，E 表 21）| `q13_mingzhu_dingdan.json`，改编 | `q13_haishou_xuanshang.json`，改编 | `q13_yanghun_zhu.json`，原著 |

- **完成压过失败**（`Quests.cpp` 先判 complete）：Z1、Z2 失败于 `ch13.biguan`——n27 起往返黑石城关闭（7 节门闸），`biguan` 之后玩家要到 n30 才回外星海，那时 `ch13.shouchao` 已置、明夫人已撤、告示墙脚本开头就 return，完成旗标不会在过期之后被补上。Z3 失败于 `ch13.lidao`，之后再回不了雾中岛。
- **Z3 的门闸**：笔记 N.3 写接取 `ch13.lifu`、J.1 写「`ch13.buzhen` 后 Z3 可交」、F 表 n23 写「养魂木两年已成形（Z3 自此可接）」——三处合起来的意思是**接得早、交得晚**：任务在立府时出现在任务栏，`yanghun.lua` 开头判 `ch13.buzhen`。本稿照此写，不改接取谓词。
- **意图玩家不做 Z1–Z3**（笔记 N.1 第 4 条）；O-5：Z3 把根须换成珠，第 14 章若要读养魂木，两种 id 都认。
- **配额**（如实，笔记 N.3）：第 12 章后 16 条（明标必经 1）；本章 ＋5（必经 2）→ **21 / 必经 3**；大纲 45 / 14 缺 24 / 11，第 14 章一章补不上，见 O-7。**不凑数**：秘市诸摊、竞宝厅、鉴宝厅、听风阁、联合大拍卖都只落在路径行动与 n14、n31 的选项里。
- **不做成支线的**：苍云龙托话给紫灵（n04 一句；紫灵第 14 章以后）；范夫人幻梦石之邀（他回绝，A138）；文思月为妾（原著作罢，E 表 26）；黄明礼之后的去向（原著没写）；虫魔真相（判据③，E 表 35）。

---

## 7. 系统：战乱封锁、偷渡、立府经营、诱妖与炼丹、结丹中期自坐、旧灵田、秘市

**先查了已有的，再定做多少**（笔记 D 表）：`realm.cap` / `realm.at_least` 与面板冲关 `tryBreakthrough`、facility `effectiveness`、`field.unlock` 与 facility `field` 的 `ref_id`、`shop(id)` 与 `Shop{buyRate,sellRate,entries}`、配方 `requireFlag`、`magic.learn` 看返回值、`effect "stagger"`、编成的 `wave` / `charge` / `actions` / `killable_by` / `can_escape`、第 11 章诱妖循环口径（`item.count`、`bottle.drops`、`bottle.spend`、`advance_days`）、`teleport(map_id, x, y)` 与五元 `kScriptTransfers`、NPC `visible_flag` / `hidden_flag`、路径行动 `when` / `until`。**本章不加 Lua 命令、不加存档字段（仍 v9）、不改 `src/`**；契约只有两条判据项：**E1 结丹档面板冲关**、**E2 跨图同一块田**（10.1）。拍卖面板（`AuctionLot` 只有规则层）本章不做（E 表 25）。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **战乱封锁** | **按地点分店**（E 表 14）：南明岛原料铺 `ch13_nanming_fangshi`（`buyRate 0.9`、限量不补）由 n02 掌柜脚本 `shop()` 开，`ch13.gongdao` 之后再谈只一句「封市」；天星城坊市 `trigger_fangshi_bimen` 只一句闭门；传送阵改条件在 n07 于护法口中（任务或大战当众杀同阶、先前不算），「报名 / 安分」两支都领 `story_huangse_zhihuan`、只换一句；外海「能出不能进」在 n13 看护一句，图上不留回程 | 不做物价曲线（`buyRate` 按旗标切换是新引擎项）；不让 ① 抵传送条件（A32「先前不算」）；不做竞价 | 原著 ch500–542「物价」0 命中，只有略便宜、闭门、改条件、黑价、只出（B8）——封锁是这一章的底色，不是一条涨价线 |
| **偷渡三支付法** | n12 `choice` 只列付得起的支：**灵石 10000**（`item.count` 够才列，`take` 看返回值）/ **五级妖丹 4** / **把柄 ＋ 胡月飞刀两柄**（旁白，不扣物，**永远在**）；记 `ch13.toudu` 1 / 2 / 3，n13 只换易敬一句（E 表 12）；「结丹给多少都不放」由 n11 易容压成假丹落地（A38、A41）| 不硬收五千；飞刀不进背包（A45 只旁白）；不做「被识破」分支 | 起点灵石估不过四千（C.3），硬收是死局；「五千一人」不能只剩台词，所以有钱的玩家可以照价付双倍（A43）|
| **易容与两个化名** | 天星城「曲」（n11–n12）、秘市「厉」（n31–n33）只在他人对白里；换形诀、法力七成只在文案（A125）| 不换名牌、不换主角 role、不做伪装值（E 表 34）| 换名牌要换主角 role，牵动全部战斗 |
| **两样新手段** | **五行环**：n01 `magic.learn("magic_ji_wuxinghuan")` 看返回值（金、`effect stagger`、`stagger 3`、`power 0`）；`story_wuse_tonghuan` 不动（E 表 7）。**蜂云合剑**：n20 `take("material_lianjing", 1)` 看返回值（false 一句照走）→ `advance_days(360)` → `magic.learn("magic_fengyun_heji")` 看返回值（木，七道青光合剑，A98；改编名，图鉴标「改编」）| 剑上辟邪神雷不给（A26：此时无神雷）；血色披风不做战斗手段（A9）；**本章 0 张炼器配方**、重炼不走面板（E 表 8）；五行环不做 `castMagic` 道具 | 古宝不该「用一件扣一件」；炼晶只一块，面板炼器失败折半材料＝死局，再开一张 `guaranteed` 违反「全仓只许一张」（B31）|
| **立府经营** | n17 `field.unlock("field_wuzhong", 6)`、`give("herb_nichang_cao", 3)`；静室（150）、丹炉（grade 4）、药园三设施与码头往返自此开（4.1）；往返黑石城两条五元传送（#6 / #7），每趟 `advance_days(12)`、`choice`「去 / 不去」，取消即不去——**卖妖丹、买霓裳草与黄精、交 Z1 / Z2 都靠它** | 不做建造、年度结算界面、奇虫养成（虫室、幻焰蛾、新噬金虫只旁白，A72–A74、A91）；不给炼器台（E 表 17）| 原著全作唯一一段慢日子（导读第 4 句）；不给往返则 ② 的妖丹无处卖，经营循环断（E 表 24）|
| **诱妖与收手** | n24 `trigger_youyao`（interact、once 否）照第 11 章口径（E 表 18）：草 ≥ 1 且绿液 ≥ 1 → `bottle.spend(1)` → `advance_days(3)` → ②；胜扣草 1、`ch13.lieyao` ＋1，逃不扣草（液已扣），败 game over；缺草一句指寸金阁，缺液 `advance_days(5)` return。n25 `trigger_shoushou` 判 `ch13.lieyao >= 2`，够则七年蒙太奇 `advance_days(2415)`、`give` 六级 6 ＋ 五级 10 ＋ `story_liuqi_cailiao` 1（E 表 19）| 不做「数百枚」真数量（A107 按比例收成）；诱妖阵只叫「法阵」「阵」，「困妖」是禁词（M 表，首见 1038）；不做深海遭遇 | 第 11 章已验过的整套断言不重写；「四百年霓裳草」只在文案（A92），三日一滴恰好一轮 |
| **炼丹与至元丹** | n26 `advance_days(1080)`、置 `ch13.liandan` → 解锁 `recipe_zhiyuan_dan`（五级妖丹 1 ＋ 黄精 1 → 至元丹 3，`guaranteed` 不写，`difficulty` 由平衡路定、意图玩家成功率 ≥ 50%）；n29 至元丹**三支兜底**（E 表 22）：至元丹 ≥ 3 → `take` 3 / 六级妖丹 ≥ 1 → `take` 1 / 再否则 `advance_days(30)` 照走，记 `ch13.zhiyuan`；公孙杏之约改「入阵为仆」（E 表 21）| 不做「炼丹大师」数值；不强制炼几炉；九曲灵参配方不建 | 只认自炼的丹则炼丹失败的玩家卡死；照原著写「稚女炉鼎」越过内容尺度（B30）|
| **结丹中期自坐**（契约 E1）| n27 `local ok = realm.cap(realm.CORE_MID)`（false 一句照走），之后玩家在 `facility_jingshi_wz` 按「闭关一年」自冲；n28 开头 `realm.at_least(realm.CORE_MID)` 不到只一句「瓶颈未破」、不置旗、不拨日（E 表 10、11）。数：基准每日 0.9 点、计顿悟期望约 1.02 点；冲关 5000 点、到线 85%（60 ＋ 资质 25 ＋ 丹 0），失败倒扣 1000（笔记 C.2）| **`realm.advance` / `realm.demote` 本章 0 处**；不改 `aptitude`（五色圆珠只旁白，A144，E 表 38）；红光丹药 0 处成物（A114）；`breakthroughPillBonus` 仍恒 0 | 经营章最该给玩家的那一下是自己坐出来的；推翻第 12 章契约「境界」行（B20，O.1 已写）|
| **旧灵田**（契约 E2）| `ch13_tianxing` 三十九层挂 `facility_yaoyuan_39`（`ref_id field_tianxing_39`），玩家自采第 11 章种下的作物；n08 只旁白装虫、弃府（E 表 16）| 不加「一键采收」Lua 命令；脚本不搬作物；n08 之后不设「采完才能走」 | 不放则第 11 章的药白种；同一块田挂两张图从没有过，要判据（O-2）|
| **三件旧物落地** | n19 `take("story_xuangu_leigu", 1)` 看返回值、`give("story_xuanyin_jing", 1)`；n18 `give("story_wuse_yuanzhu", 1)`、n22 `take` 1；n22 `story_jinsi_can` 余数 > 0 才 `take` 余数（E 表 29、B19）| 玄阴经不给常驻功法与法术、天都尸火不做（A78–A79，E 表 9）；虚天鼎不开（A80）；冰珠不碰 | 他修青元剑诀、不重修玄阴大法；全是「以后的东西」|
| **兽潮只经人口** | n29 孙师叔一句、置 `ch13.shouchao`；K12 是想多问的细节；黑石城六人撤场、两店随人没（5.1）| 不演兽潮、不演黑石城被破（判据③，E 表 23）| 他在雾海闭关二十余年，不在场 |
| **秘市与交换会** | n31 一份脚本场（E 表 25）：引荐 → `shop("ch13_mishi_tan")` → 听风阁一句 → 交换会三处抉择（铜片三支必得 E 表 27 / 以整堆材料换她自由，「再看看」再给一次 / 范夫人之邀）→ 云天啸一击是旁白 → 带文思月离场 | 拍卖、竞价全是选项，不做面板；云天啸不做战斗（E 表 5、O-1）| 原著十二柄小剑一击了事（A137），做成仗是灌水 |
| **章末人声** | n34 入洞 `give("story_banyao_cao", 1)` → 身后一个男子声音（一句）→ 置 `ch13.done`；不 `ending()`、不 teleport（E 表 28）| 不写名、不现身、不建出场 role；「风某」「风希」「九级」「化形第二阶」0 处 | 写到他现身就用掉第 14 章开场（风希 ch543）|

### 7.1 门闸一览（**本章门闸的权威表**）

| 门闸 | 挂在哪 | 怎么拦 | 不到时 | 为什么不会卡死 |
| --- | --- | --- | --- | --- |
| 三十九层药园、静室 | `facility_yaoyuan_39`、`facility_jingshi_39` | `require_flag ch13.rucheng`（不写 deny，4.3）| 引擎播 `ui.facility.locked` | `rucheng` 是城门前的带，进城即置；数据上几乎碰不到 |
| 雾中岛三设施 | `facility_jingshi_wz`、`facility_danlu_wz`、`facility_yaoyuan_wz` | `require_flag ch13.lifu` | 同上 | `lifu` 是登岸礁北面的带 |
| 南明岛原料铺 | `caigou.lua`（NPC）| 脚本判 `ch13.gongdao` | 一句「封市」| 店是便利，不是门槛 |
| 天星城坊市 | `trigger_fangshi_bimen` | 永远一句 | — | 只是封锁的样子 |
| 南明岛盘查 | `panzha.lua`（NPC）| `item.count("story_miaoyin_yaopai")` | 无腰牌 → 报名号、画像对得上，照放（数据上不可达）| 腰牌只读不扣（E 表 15）|
| 偷渡付费 | `chuansong.lua`（NPC）| 选项按 `item.count` 只列付得起的 | — | 第三支永远在（E 表 12）|
| 去黑石城 | `trigger_qiyuan_hang`（五元 #6）| guard `ch13.lifu`；脚本开头判 `ch13.biguan` | 一句「闭关在即」return | 往返是可选的经营，不在意图玩家路上 |
| 回雾中岛 | `trigger_hui_wuzhong`（五元 #7）| guard `ch13.lifu`；脚本开头判 `ch13.lidao` | 一句 return | 已离岛 |
| 诱妖 | `trigger_youyao` | 草 ≥ 1 且绿液 ≥ 1 | 缺草一句（指寸金阁）；缺液 `advance_days(5)` return | n17 必给霓裳草 3；液随日子回 |
| 收手 | `trigger_shoushou` | `ch13.lieyao >= 2` | 一句 return、不置旗 | ② 可重复、可逃 |
| 出关 | `trigger_chuguan` | `realm.at_least(realm.CORE_MID)` | 一句「瓶颈未破」、不置旗、不拨日 | 静室随时可进，失败只折修为 |
| 至元丹 | `gongsun.lua`（NPC）| 三支只列有的 | — | 第三支 `advance_days(30)` 永远在（E 表 22）|
| 铜片 | `mishi.lua` | 三支只列付得起的；`story_liuqi_cailiao` 的 `take` 看返回值 | false 只换一句 | 第三支以桌上材料抵（旁白不扣）永远在（E 表 27）|
| 旧物 `take` | n19、n20、n22 | 看返回值 | false 只换一句照走（**推荐**：笔记只给 n20 写明，n19、n22 同口径）| 三件都由第 12 章给、0 处中途扣 |
| Z2 / Z3 | `trigger_xuanshang`、`trigger_yanghun` | 见第 6 节 | 一句 | 可选 |

区域挂点（外星海三区、雾中岛西海礁）**不设门闸**：未到时 guard 未置、踩了不响（引擎口径 2：portal / facility 不读 visible / hidden，门闸只用 `require_flag` 或脚本开头判旗标）。

### 7.2 起点与章末对照（写测试时读表，不抄数；供第 14 章参考）

| | 起点（ch12-end，估）| ① 时（n05）| ② 时（n24）| n27 之后 | 章末 |
| --- | --- | --- | --- | --- | --- |
| 境界 / 上限 / `formerRealm` | 31 / 31 / 22 | 同左 | 同左 | 31 / **32** / 22 | **32 / 32 / 22**（玩家自冲；n28 已判）|
| 气血 / 法力上限 | 700 / 470 | 同左 | 同左 | 同左 | **900 / 620** |
| 面板冲关回报 | `StoryCap` | 同左 | 同左 | 修为 < 5000 `NotEnough`、≥ 5000 `None`（85%）| `StoryCap`（上限 32）|
| 最近的打坐处 | — | 南明岛 100 | 雾中岛静室 150 | 150 | — |
| 法术 | 12 门（第 10 章 11 门 ＋ 噬金虫群）| 13（＋五行环）| 14（＋蜂云合剑）| 14 | 14 |
| 队伍 | 空 | 空 | 空 | 空 | 空（`party.*` 0 处，E 表 37）|
| 小瓶 | 容量 9、结丹 3 日一滴 | 同左 | 同左 | 同左 | 同左 |
| 旧物 | 腰牌 1、肋骨 1、炼晶 1、金丝蚕 0 或 3（估）| 同左 | 肋骨 0、炼晶 0、玄阴经 1、五色圆珠 0、金丝蚕 0 | 同左 | ＋铜片、伴妖草；腰牌仍 1 |

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** ＝ 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **2 场必打**：① n05 `b13_nanming_zhuibing`（新）、② n24 `b13_waixinghai_lieyao`（循环，可重复，**按一场计**——第 11 章 ① 诱妖先例；n25 要 `ch13.lieyao >= 2`，意图玩家胜 2 次）。两场都是原著的仗。**可选：无**（无遭遇区、黑石城禁斗、路径行动 0 条 `challenge`）。
账（笔记 I 表、E 表 5）：方案全作 52 场，第 1–12 章合计 45 场，**本章 2 场**，第 14 章留 **5 场**。云天啸一击、黄明礼救援原著都是一招了事，不升格；若第 14 章用不满 5 场，预留 id `b13_mishi_yuntianxiao`（O-1，不推荐）。

**这两场就是「古宝之威」与「重炼之剑」的说明书**（第 7 节「两样新手段」）：

| # | 谁上场 | 这一仗教玩家什么 |
| --- | --- | --- |
| ① | 韩立一人，无友军、无队伍 | **五行环第一次上手**：锁住一个（削架势 → 破势），趁窗口收掉，再转向另一个——全作第一次「一人对两名同阶修士」 |
| ② | 韩立一人 | **重炼的回报在最后一刀**：五级随手斩，六级首领只有剑与蜂云合剑（木）要得了命 |

### 8.1 意图玩家

- **共同**：结丹初期（700 / 470，攻 62、防 46——笔记 C.2，`Realm.cpp` 注释「暂定」）；本章没有一场仗发生在结丹中期之后（② 在 n27 之前收口），结丹中期锚点不标定（O-9，留第 14 章）。兵刃：剑（`weapon_qingzhu_fengyun_jian`，**无神雷**，A26）、拳、暗器（`weapon_wuming_sixian`）。道具：中阶灵石约 20（估，回法力）、南明岛店的回气丹（可选，意图玩家不买）。**队伍为空**（E 表 37）。法术与旧物以 7.2 为准。
- **①**：法术 13 门（第 10 章 11 门 ＋ 噬金虫群 ＋ n01 学的五行环；蜂云合剑还没有）。进场前无仗（n01–n04 全是演出）；第 12 章 ④ 之后血未必满（C.3），`facility_dazuo_nanming` 在出海口带之前（E 表 39）；存档点在 `haishang`。
- **②**：法术 14 门（＋ n20 学的蜂云合剑）。每轮诱妖先 `advance_days(3)`（每日回一成气血）；洞府静室（150）是兜底。意图玩家胜 2 次、不逃。
- **修为**：① `rewards.cultivation` 建议 300；② 建议 ≤ 60（O-10：可重复的仗给多了，就成了冲结丹中期的捷径）。**C0 修正**（协调者备注 2026-10-13）：第 11 章真档章末修为 2572 / 2511，C0 ≥ 该数 ＋ ① 300（第 12 章四场与 ② 所得另加，按 0 计下界），即两侧 C0 ≥ 2872 / 2811 → 坐到 5000 至多约 2090 / 2150 日（按期望每日 1.02 点），**意图玩家按「闭关一年」约 6 次以内**；笔记 C.2 的「6–14 次」是真档之前的上界。次数仍以 `ch12-end` 真档为准（O-3），增量 11790 不变。
- **最低合法起点**：中阶灵石 0、无丹药、气血不满。① 前有打坐设施；② 可逃（逃不扣草）、静室随时可进；两场之前都有存档点——没有「存在仗前、打不过、也回不了血」的死局。平衡路两种都要量。

### 8.2 两张必打编成（**编成的权威表**）

| # | 编成 id | 敌方（按波次）| 我方 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b13_nanming_zhuibing`（新）| **第 0 波** 白面儒生 ×1（`nixing_rusheng`，结丹初期；`toughness 3`；破绽金、剑，`killable_by ["金","剑"]`；`magics ["magic_mihuan_guniao"]`——黑雾怪鸟，水、全体小伤）＋ 黑壮大汉 ×1（`nixing_dahan`，结丹初期；`toughness 4`；破绽金、木、剑，`killable_by ["木","剑"]`；蓄势白玉大印 `charge {every 3, all false, text_key "ch13.battle.nanming_zhuibing.charge"}`）| 韩立一人 | 否 | **致命** | **先锁一个**：五行环（削架势 3）一下打穿儒生的架势，破势窗口里用剑或祭金蚨收掉；大汉每三回合攒一发单体重击，要在那一回合前把他打破势、或防御硬接；怪鸟是全体小伤，压的是法力。打赢不给灵石（灵石由 n05 脚本从储物袋给）|
| ② | `b13_waixinghai_lieyao`（**就地改写**，E 表 6）| **第 0 波** 五级海兽 ×2（`wuji_haishou`，结丹初期；`toughness 3`；破绽土、剑——就地补「剑」，B25；`killable_by` 不写）；**第 1 波** 青色巨虾 ×1（`qingse_juxia`，结丹中期，首领；`toughness 5`；破绽金、木、剑，`killable_by ["木","剑"]`；`actions 2`；`magics ["magic_haichao_jue"]`；蓄势黑气 `charge {every 3, all true, text_key "ch13.battle.waixinghai_lieyao.charge"}`）| 韩立一人 | **可**（逃不扣草）| **致命** | **先省后收**：第 0 波原著是飞剑随手斩，玩家用剑、土省法；巨虾一回合动两次、每三回合攒一发全体，靠五行环削势或剑破势打断；**收尾只有剑与蜂云合剑（木）要得了命**——金能削它的势、要不了它的命 |

- **必须守住的**（数值归平衡路，按 `tests/BattleHand.h` 那只手量；本表只写意图与破绽）：两张 `hero_absent false`、无友军、`defeat_is_fatal true`、`terrain sea`、`rewards.spirit_stones 0`；① 一波两敌、`can_escape false`、`drops []`、`rewards.cultivation > 0`；② 两波、`can_escape true`、`drops` 恰六级妖丹 1 ＋ 五级妖丹 2（都 `rate 100`）、`rewards.cultivation` ≤ 60。`killable_by` 一律非空且 ⊆ `weaknesses`（不写即不限）；胜负只看气血归零。
- **② 占位就地改写的几处**（B24）：现文件 `rewards` 是 `cultivation 1400`、`spirit_stones 320`、`drops` 只有五级 2，第 1 波不存在；`name`「外星海·五级海兽群」与 `note`「立府雾中岛…ch512–535」一并改写成 ch523 珊瑚礁诱妖。`wuji_haishou` 的 note「写到那一章时重填」同时改写。
- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，C2；E 表 30）：`CHAPTER_MEANS[13]` ＝ 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`weapon_wuming_sixian`，ch07）、剑（`weapon_lieyang_jian`，ch06）；`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]` ＝ `("木","learn","magic_fengyun_heji","ch13")`（`learn` 种类由 `check_means_sources` 去 `scripts/ch13/` 核 `magic.learn`，`validate.py` 第 2537–2552 行；木在通表里已有，这一条是给重炼留的出处）；`MEANS_LAST_CHAPTER = 13`。**推荐**：笔记 E 表 30 在「金」后括注了噬金虫与五行环——噬金虫可作补充出处；**五行环不登**：`power 0`、`effect stagger`，削得了势、要不了命，登成必有的「金」会让规则 25 对 `killable_by` 含金的敌人放水（友军那一侧 `role_means_available` 也明确排除带 `effect` 的法术，第 2496 行）。
- **selftest 探针**（笔记 I 表末，C2）：② 写实后探针从 `wuji_haishou` ＋ `b13_waixinghai_lieyao` 挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`，`probe["chapter"] = 13`，`scoped(14)` 应静、`scoped(13)` 应报；`MEANS_LAST_CHAPTER == 12` 的用例改 13。临时写回的是测试副本，还原字节相同。
- **战斗背景**（`data/visual/battles.json`，现有名；2026-10-13 grep 该文件尚无 `b13_*` 条目）：① `dock_river`、② `cliff_top`（第 11 章红瑚岛同组）。每条都要指得出读它的那一行代码、有走真实入口的测试（handoff 第 6 节第 5 条）。
- **开场白与文案**（`data/text/ch13_battle.json`）：`ch13.battle.<短名>.intro` / `.charge`；① 开场白写两名银衫修士要他随行验容貌（不抄原著台词），蓄势写大印；② 开场白写霓裳草香与礁外法阵（叫「法阵」，不叫「困妖」）、蓄势写巨虾裹着的黑气。**旧占位 `ch13.battle.waixinghai_lieyao.intro` 在 `data/text/battles.json` 第 8 行**，由协调者删（重复 key 让 `loadGameData` 起不来，第 11、12 章同一个坑）。胜利文案：① 两人俱亡、他收两只储物袋；② 聚魂钵抽魂取丹（A96）。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 追兵 | ch503–504：两名逆星盟结丹初期正道修士借故要他随行验容貌；交手中大印压垮他的青色巨剑，六道青虹被符文护罩挡住（剑无神雷，A26）；他以花篮卷走怪鸟与大印，五行环锁住儒生真元、剑气洞穿；大汉弃宝遁走，五行环再锁、一道青光斩之（ch504）| 一场、一波两敌；「随行 / 不随」两支都打；五行环＝削架势 3；花篮只在胜利文案 | 原著就是他一人对两人、无援手；「锁真元、功法施展不得」落成破势窗口（A25）。花篮（第 12 章 `story_hualan`）不做手段：做成要再登一类，而原著胜负在五行环与剑上 |
| ② 诱妖 | ch523：霓裳草诱来三四只五级妖兽，飞剑即斩；继而六级青色巨虾裹黑气而来，被阵盘黄光罩住、五行环锁死、两道青虹斩成数截；聚魂钵抽魂取丹（ch523）| 两波；可逃、可重复；胜后旁白抽魂取丹，妖丹由编成 `drops` 给 | 第 11 章 ① 是五级千目妖、有曲魂同行；本场独战、首领六级。黄光罩是「法阵」，只在开场白；**可逃、可重复是游戏的让步**——原著七年猎妖（A97）落成循环 ＋ n25 蒙太奇 |
| 逆星盟攻港 | ch501–502：上千人攻南明岛，他隐身丘陵旁观 | 演出（n03）| 他没有出手（A15）|
| 毒蛟与巨龟 | ch520–522：化形雷劫、八级毒蛟与金霞、霍氏交手 | 演出（n21），他远看后遁走 | 八级碰见只能逃（A14、A85）|
| 琉璃兽 | ch524：七道青光合剑，一剑斩首 | 蒙太奇（n25）| 一剑之事；同一招就是 ② 里的蜂云合剑 |
| 鸠面老者三人 | ch525–527：三色甲虫凝矛、颈圈逼回 | 演出（n25），他否认虫魔、放人 | 没有交手（A103）|
| 碧云门一队 | ch527：十万噬金虫灭敌 | 回想（n25）| 七年里的事，判据② |
| 鹰鸢兽 | ch530：傀儡与光幕退兽 | 旁白（n28）| 他隔阵问话、没出手（A117）|
| 黄明礼之围 | ch534：救下黄明礼 | 旁白一招（n30）| 一招（A126）|
| 云天啸 | ch538：十二柄寸许小剑穿透护体火焰 | 旁白一击（n31）| 一击（A137，E 表 25）|

### 8.4 野外遭遇

**本章不放遭遇区**（笔记 D 表）：黑石城除竞斗场外严禁斗法（A52）；雾中岛是他自己布了幻阵的岛（A70、n17）；南明岛与天星城一路戒严、被盘查。外海猎妖原著的收获是妖丹，而遭遇表的掉落只许灵草——做遭遇区只会给玩家一堆打了没妖丹的海兽；猎妖的循环就是 ②。修为的来路是静室（第 7 节），不是遭遇。

---

## 9. 经济账（**数目的权威表**）

**四本账：低阶灵石（`material_lingshi`）、中阶灵石（`material_lingshi_zhong`）、妖丹（`material_yaodan_wuji` / `material_yaodan_liuji`）、诱妖资源（`herb_nichang_cao`、`herb_huangjing_cao`〔数据里显示名「黄精」〕、小瓶绿液）**，另记 story 物品。起点全部是「估」（`tests/fixtures/ch12-end-*.sav` 尚未生成，笔记 C.3）：协调者备注给出第 11 章真档章末低阶 2339 / 2319、中阶 20 / 20，第 12 章账面低阶 −8、中阶 0，**本章起点估低阶约 2331 / 2311、中阶 20 / 20**（比笔记 C.3「数百到四千」窄；仍须拿真档核）。下面只写本章的进出与恒等式，不先宣称终局数。

**来源**

| 项 | 低阶灵石 | 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| 起点（ch12-end，**估**）| 约 2331 / 2311 | 20 / 20 | 0 / 0–1 | 霓裳草余数、小瓶（容量 9）、腰牌、肋骨、炼晶、金丝蚕 0 或 3、养魂木根须、灵乳等（**必须拿真档核**）|
| n05 储物袋 | ＋600 | | | 迷幻砚、白玉大印 |
| n07 / n15 / n16 | | | | 黄色指环 / 上古典籍玉简 / 奇渊海图 |
| n17 立府 | | | | 霓裳草 ＋3；灵田 `field_wuzhong` 6 槽 |
| n18 / n19 | | | | 五色圆珠一匣 / 玄阴经 |
| ② 每胜（编成 `drops`）| | | ＋2 / ＋1 | |
| n25 收手 | | | ＋10 / ＋6 | 六七级妖兽材料 1 |
| 丹炉（玩家）| | | | 至元丹 ＋3 / 炉 |
| Z1 / Z2 / Z3 | | ＋30 / ＋15 / — | | Z2 霓裳草 ＋3；Z3 养魂木珠 |
| n31 / n34 | | | | 铜片 / 伴妖草 |
| 卖（玩家，可选）| 明珠轩 `sellRate 0.8`：五级 760、六级 4800；寸金阁 0.5；秘市 0.6（收至元丹、妖丹）| | | 南明岛原料铺 0.5 |

**去向**

| 项 | 低阶灵石 | 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| 南明岛原料铺（可选，`buyRate 0.9`）| 照价 × 0.9（霓裳草 9、回气丹 450）| | | 限量不补货 |
| n12 偷渡 | 第一支 −10000 | | 第二支 −4 / — | 第三支 0（把柄 ＋ 飞刀，旁白）|
| 寸金阁 / 秘市（可选）| 霓裳草 −10、黄精 −5、玛瑙角 −5000 | | | |
| 丹炉每炉（玩家）| | | −1 / — | 黄精 −1 |
| n24 每轮 | | | | 绿液 −1（战前扣，逃也扣）；胜则霓裳草 −1 |
| n19 / n20 / n22 | | | | 肋骨 −1 / 炼晶 −1 / 五色圆珠 −1、金丝蚕 −余数 |
| n29 至元丹 | | | 第二支 — / −1 | 第一支至元丹 −3；第三支 `advance_days(30)` |
| n31 交换会 | 铜片第二支 −5000 | | 铜片第一支 −1 / — | 换她自由：六七级材料 −1（看返回值）|
| Z1 / Z2 / Z3 | | | Z1 — / −1；Z2 −3 / — | Z3 绿液 −3、根须 −1 |
| 往返黑石城（可选）| | | | 每趟 `advance_days(12)`，不花钱 |

**账要对得上的几条**（交测试路，T 前缀；直接读 `scripts/ch13/*.lua` 与 `data/battles/b13_*.json`）：

1. **`give("material_lingshi")` 恰 1 处**（`zhuibing.lua`，600）；**`take("material_lingshi"` 恰 2 处**（`chuansong.lua` 10000、`mishi.lua` 5000），都只在 `item.count` 够时才列出那一支、`take` 看返回值。**没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训）：偷渡、至元丹、铜片三处付出都有零成本一支（纪律 70）。
2. **中阶灵石**：`give` 恰 2 处（`mingzhu.lua` 30、`xuanshang.lua` 15）；`take` 0 处。
3. **妖丹**：脚本 `give` 恰 2 处、都在 `chongmo.lua`（五级 10、六级 6）；`take` 恰 5 处——`chuansong.lua` 五级 4、`gongsun.lua` 六级 1、`mishi.lua` 五级 1、`mingzhu.lua` 六级 1（Z1）、`xuanshang.lua` 五级 3（Z2），每一处都在「够才列」的支里；编成来源只有 ② 的 `drops`。
4. **story**：本章新 story 各 `give` 1 处（迷幻砚、白玉大印、黄色指环、上古典籍玉简、奇渊海图、五色圆珠、玄阴经、六七级妖兽材料、铜片、伴妖草、养魂木珠）；`take` 恰 7 处（肋骨 n19、炼晶 n20、五色圆珠 n22、金丝蚕余数 n22、六七级材料 n31、养魂木根须 Z3、至元丹 3 于 n29 第一支），全部看返回值；`story_miaoyin_yaopai` 0 处 `take`；`story_wannian_lingru` 0 处读写（文案「灵乳」）。
5. **小瓶**：`bottle.spend(1)` 恰 1 处（`youyao.lua`）、`bottle.spend(3)` 恰 1 处（`yanghun.lua`）；缺液的 `advance_days(5)` 不计入意图玩家。
6. **编成**：① `spirit_stones 0`、`drops []`；② `spirit_stones 0`、`drops` 恰六级 1 ＋ 五级 2。
7. **店与配方**：`data/shops/ch13_*` 恰 4 份；脚本 `shop(` 恰 4 处（`caigou.lua`、`mingzhu.lua`、`cunjin.lua`、`mishi.lua`）；新配方恰 1 张（`recipe_zhiyuan_dan`，`requireFlag ch13.liandan`，不写 `guaranteed`）；炼器配方 0；全仓 `guaranteed:true` 仍恰 1 张。
8. **意图玩家章末账**（两侧同，笔记 N.1 第 5 条）：低阶 ＝ 起点 ＋ 600、中阶 ＝ 起点；六级 ＝ 起点 ＋ 2 ＋ 6 − 1 ＝ 起点 ＋ 7；五级 ＝ 起点 ＋ 4 ＋ 10 − 1 ＝ 起点 ＋ 13；霓裳草 ＝ 起点 ＋ 3 − 2 ＝ 起点 ＋ 1；六七级材料 0；`story_wuse_yuanzhu`、`story_xuangu_leigu`、`material_lianjing`、`story_jinsi_can` 都是 0；本章新 story 各 1（迷幻砚、白玉大印、黄色指环、上古典籍玉简、奇渊海图、玄阴经、铜片、伴妖草）。
9. **两侧同账**：通关两侧走同一组选项（笔记 N.1 第 4 条：n12 第三支、② 胜 2 次不逃、0 次往返、0 条可选支线、n29 第二支、n31 铜片第一支 ＋「以整堆材料换」＋ 回绝）；章末 `day` − 起点 `day` ＝ **11790 ＋ 360 × N**，N 是该侧按「闭关一年」的次数，由该侧真档 C0 与冲关种子定（两侧 C0 差约 61 点，通常同 N；不同则各写各的，不抹平）；两侧低阶灵石差 ＝ 起点差、中阶同。
10. **退路**：本章门闸全在 7.1；没有一处会因物品或钱不够而卡死。

**对照：原著这一段的量级**。偷渡是五千一人的黑价（A37），而他一块灵石没花——两颗五级妖丹、一个把柄、两柄死人的飞刀（A44–A46）；玛瑙角一只五千（A64）；七年深海存下数百枚六七级妖丹（A107），本作收成 16 颗。所以本章的钱在**妖丹**不在灵石：意图玩家章末多出的六级 7、五级 13，按明珠轩价折算约 4.3 万低阶，却几乎无处可花（兽潮后黑石城空了，秘市只开一次）——这是他「外海一趟的身家」，留给第 14 章（笔记 O.2）。

