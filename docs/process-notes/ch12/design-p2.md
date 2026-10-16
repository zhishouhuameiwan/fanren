# 第 12 章施工图·第二部分（第 4–9 节：地图、角色、支线、系统、战斗、经济）

> 写手二，2026-10-12。唯一事实来源：`notes/ch12/research.md`（下称「笔记」）；格式照 `fanren/docs/ch10-design.md` 第 4–9 节。
> 引用约定：「16.1 第 N 条」＝笔记 E 表第 N 条（拍板）；「16.2 O-N」＝笔记 O.3；「16.3 KN」＝笔记 K 表第 N 行（路径行动）；「3.2 nNN」＝主线节点；A、B 编号＝笔记 A、B 表。
> 本部分凡与笔记不一致处都**不擅改**，集中列在各节末「与笔记不一致、待协调者拍板」里，给出推荐改法；协调者改笔记后再定稿。
> NPC 在场写作「出场旗标 → 退场旗标」，**旗标一律省去 `ch12.` 前缀**（`tupo` ＝ `ch12.tupo`）；表外正文写全名。

## 4. 地图（新建 7 张，旧图 0 处修补）

| 地图 id | 名称（`display_name_key`）| 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| default spawn（另有）| 出入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `ch12_tushan` | 土山荒岛 | 48×36 | ✓ | `wild` / `dusk` / backdrop `mountain_forest_dusk` | `andeng`（南岸）；另 `dongkou`（`portal_dixue` 正南一格）| 北坡 `portal_dixue` → `ch12_dixue` `taijie`（往北出、进门朝北），`require_flag ch12.tuizhu`、`deny_text_key ch12.block.dixue`；脚本出口 n07 `teleport("ch12_dadian")` |
| `ch12_dixue` | 封灵柱地洞 | 30×24 | — | `cave` / `indoor` / backdrop `cave_tunnel` | `taijie`（南端台阶脚）| 南端 `portal_tushan` → `ch12_tushan` `dongkou`（往南出、进门朝南），不设门闸 |
| `ch12_dadian` | 虚天殿外殿大厅 | 44×40 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `tongdao`（南端通道口）| 无门；脚本出口 n11 `teleport("ch12_guiwu")` |
| `ch12_guiwu` | 鬼冤之地 | 56×48 | ✓ | `mountain_path` / `night` / backdrop `mountain_forest`；粒子 `mist`（`ch09_milin` 同一组合）| `tupo`（南端土坡）| 无门；脚本出口 n15 `teleport("ch12_xiaojie")`；走偏三处同图带坐标回岔口 |
| `ch12_xiaojie` | 小空间 | 64×40 | ✓ | `valley` / `day` / backdrop `herb_valley` | `caoyuan`（西端草原）| 无门；脚本出口 n19 `teleport("ch12_binghuo")` |
| `ch12_binghuo` | 冰火道·熔岩路 | 80×28 | ✓ | `cliff` / `dusk` / backdrop `cliff_top_dusk` | `guangbi`（西端光壁）| 无门；脚本出口 n23 `teleport("ch12_neidian")` |
| `ch12_neidian` | 宝光阁·极妙幻境·内殿 | 80×50 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `yuqiao`（西区玉桥头）| 无门；四区以墙隔开，只靠脚本落点：「石室」（n24 后）、「寒骊台下」（n27 后，台阶脚南一格）、「灵泉」（n32 后，池边）|

**图与图之间只有一对门**（荒岛 ↔ 地洞）；其余全是脚本单向 teleport——殿内走不回头（原著 ch447 一月后才有阵传回大厅、ch467 石殿规矩，A56、A78）。七处脚本出口都只写图 id，落该图 default spawn；`ch12_neidian` 三处、`ch12_guiwu` 三处岔口是带坐标的同图落点（坐标由关卡路定、照抄进脚本与 `kScriptTransfers` 注释），**落点格与任何触发器都不同格**。

### 4.1 每张图的挂点与在场 NPC

**`ch12_tushan` 土山荒岛**（第一幕：n01–n03、n07）
- 布局：南岸登岛处（spawn `andeng`）→ 石屋（存档点 `facility_cunji_tushan` 在石屋旁）→ 北坡黄雾（`trigger_huangwu`，interact）→ 光罩（`trigger_guaizhao`，enter，光罩前一格，与黄雾错开）→ 北坡地洞口；东岸一片礁滩（n07 文氏父女，改编地点，16.1 第 33 条）。
- 挂点：`trigger_dengdao`（enter，**落在 `andeng` 上**——C1 patch 的 `teleport("ch12_tushan")` 一落地就演 n01）、`trigger_huangwu`（n02）、`trigger_guaizhao`（n03）；n07 挂 NPC `npc_wen_siyue`。
- NPC：石屋前 `npc_jin_qing`、`npc_hu_yue`、`npc_jian_xiushi`、`npc_shi_die`（`dengdao → tuizhu`）；东岸 `npc_wen_siyue`、`npc_wen_qiang`（`canyuan → lidao`）。路径行动 K1–K4 挂在金青、石蝶、胡月、文樯上（16.3）。
- 「岛上无灵脉、草木不生」（A3、K1）：底图少植被，碎石与枯土为主；`wild` ＋ 黄昏够用。

**`ch12_dixue` 封灵柱地洞**（第一幕：n04–n06）
- 布局：南端台阶脚（spawn `taijie`，存档点 `facility_cunji_dixue` 在台阶旁）→ 中部水池（七霞莲、千年石乳，`trigger_shuichi`，interact，池边）→ 北壁封灵柱（`trigger_shizhu`，enter，**离池 ≥ 3 格**）。
- NPC：`npc_jin_qing`（`tuizhu → duoshe`）、`npc_hu_yue`、`npc_jian_xiushi`、`npc_shi_die`（`tuizhu → qixialian`）；`npc_shaonian`（role `xuangu_shaonian`，`duoshe → canyuan`，n06 挂点）。
- 四同伴「只写倒下」（16.1 第 22 条）：退场靠 hidden 旗标，地图上不留尸体装饰。

**`ch12_dadian` 虚天殿外殿大厅**（第二幕：n08–n11）
- 布局：南端通道口（spawn `tongdao`）→ 长厅两排巨柱（每人占一根柱，原著抢柱一段，n10）→ 北端尽头石门后的传送阵（`trigger_chuansongzhen`，interact）。存档点 `facility_cunji_dadian`；打坐 `facility_dazuo_dadian`（meditate，`effectiveness 100`，在一根空柱下）——让 n08、n10 的等候日子有事做（N.2 第二幕）。
- 挂点：`trigger_rudian`（enter，**落在 `tongdao` 上**，n08 开头判残图）、`trigger_qunxiang`（enter，大厅中段柱下）、`trigger_chuansongzhen`（n11）；n09 挂 NPC `npc_xuan_gu`。
- NPC：`npc_xuan_gu`、`npc_zi_ling`（role `zi_ling_xutian`）、`npc_miao_zhanglao`（`rudian → chuansong`）；`npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_wen_furen`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_man_huzi`、`npc_zhifa_ci`、`npc_zhifa_leng`（`qunxiang → chuansong`）。
- **0 战斗、0 `challenge`**：殿内禁制只容小法术（A39）。

**`ch12_guiwu` 鬼冤之地**（第三幕前半：n12–n15）
- 布局：南端土坡（spawn `tupo`，存档点 `facility_cunji_guiwu` 在土坡白光内）→ 鬼雾区：一条正路、**三处岔口**，每处一条错路，错路尽头各一格 `trigger_zoupian_a` / `_b` / `_c` → 第三岔之后一块空地（`trigger_guiwang`，enter；**空地做成去花园必经的一格宽咽喉**，绕不过）→ 北端花园入口（`trigger_huayuan`，enter）→ 花园中央亭（`trigger_tingzi`，interact）。正路旁一处死角放玉真人遗骸（`trigger_yihai`，interact，Z1）。
- 正路的标记（16.1 第 10 条）：沿正路摆白骨堆与「雾色发淡」的地砖装饰（啼魂兽开出的通道）；错路两侧雾浓、无白骨。K9 葛笠给这条提示。
- 挂点：`trigger_tupo`（enter，**落在 `tupo` 上**）、`trigger_zoupian_a/b/c`（enter、once 否、guard `ch12.tupo`、无 set_flag）、`trigger_yihai`、`trigger_guiwang`、`trigger_huayuan`、`trigger_tingzi`，**各占一格**；三处岔口落点也各占一格。
- NPC：土坡 `npc_zi_ling`（`tupo → guiwang`）、`npc_ge_li`（`tupo → guiwang`）；花园 `npc_zi_ling_hy`、`npc_yuan_yao`（role `yuan_yao_jiedan`）、`npc_xuan_gu`、`npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_wan_tianming`、`npc_man_huzi`、`npc_zhifa_ci`、`npc_zhifa_leng`（`yuanyao → xiaojie`）。
- ① 前的存档点：土坡那一处；走到 `trigger_guiwang` 要先过三处岔口，存档点在全部挂点之前。

**`ch12_xiaojie` 小空间**（第三幕后半：n16–n19）
- 布局：西端草原（spawn `caoyuan`，存档点 `facility_cunji_xiaojie` 紧挨它；草原一丛灵药 `trigger_caiyao`，interact、once、set `ch12.caiyao`）→ 东南小石山（巨石 `trigger_shishan`，interact）与山下树林边（`trigger_fanlian`，enter，与巨石错开）→ 东北峡谷口，两块三四尺高石碑（A68）之间 `trigger_xiagu`（enter）。
- NPC：石山 `npc_xuan_gu`（`xiaojie → lingshen`）；`npc_jiyin`（n18 挂点）、`npc_wu_chou`、`npc_qingyi`（`fanlian → baishi`）；峡谷口 `npc_jiyin_xg`、`npc_wu_chou_xg`、`npc_qingyi_xg`、`npc_man_huzi`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_yuan_yao`（`baishi → xiagu`）。石山与峡谷口两组极阴 / 乌丑 / 青易**首尾恰接**（`baishi`），规则 30 放行。
- 「一整天」（A56）：采药只此一格；其余灵药丛是装饰。

**`ch12_binghuo` 冰火道·熔岩路**（第四幕前半：n20–n23）
- 布局（西 → 东一条长道，**全程不能飞**，A69）：光壁（spawn `guangbi`）→ 熔岩夹道 → 草地 → 沼泽（n20 两处「罗烟步 / 常速」的地景，挂点只有 n20 一格）→ 黑沙漠西缘（`trigger_shamo`，enter）→ 沙漠中心晶柱（`trigger_jingzhu`，interact）→ 东端石殿（门内一格 `trigger_shidian`，enter）。
- 挂点：`trigger_guangbi`（enter，**落在 `guangbi` 上**）、`trigger_shamo`、`trigger_jingzhu`、`trigger_shidian`。
- 存档点 `facility_cunji_binghuo`：笔记 G 表写「石殿内」，同表又写「与 spawn 相邻」，两者打架——**本稿按「光壁内侧、紧挨 spawn」摆**，② 之前才有存档点（见本节末第 2 条）。
- NPC：石殿 `npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_man_huzi`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_xuan_gu`、`npc_zhifa_ci`、`npc_zhifa_leng`、`npc_yuan_yao`（`lianjing → shidian`）。

**`ch12_neidian` 宝光阁·极妙幻境·内殿**（第四幕后半与第五幕：n24–n33）
- **西区·宝光阁**：玉桥头（spawn `yuqiao`）→ 一层玉台（`trigger_yutai`，interact、once 否、无旗标，可反复看）→ 一层通二层的简易传送阵（n24 的灵石兜底在此）→ 二层光罩（只在 n24 文案里，不画可走区）。n24 挂 `trigger_yuqiao`（enter）——笔记写「落在 `yuqiao` 上」，本稿推荐挪到简易传送阵前一格（本节末第 1 条）。
- **中区·极妙幻境与塔前**：落点「石室」（n24 后；存档点 `facility_cunji_neidian` 在石室内）→ 石室出口 `trigger_changlang`（enter，n25 长廊与黑殿两段都在脚本里）→ 塔前空地 `trigger_taqian`（enter）→ 五层青石塔门 `trigger_tamen`（interact，与塔前**错开 ≥ 2 格**）。
- **东区·寒骊台**：落点「寒骊台下」（台阶脚南一格）→ 台背青石 `trigger_qingshi`（interact）→ 台阶顶 `trigger_taijie`（enter）→ 祭坛前 `trigger_jitan`（enter）→ 洞口旁 `trigger_dongkou`（interact）→ 台面另一角 `trigger_xiaojian`（interact）。台阶是唯一出入口（A92），台下一圈不留别的缺口。
- **南区·灵泉密室**：落点「灵泉」（池边）；`npc_yuan_yao_lq`（n33 挂点）。
- NPC：塔前 `npc_jiyin_tq`、`npc_wu_chou_tq`、`npc_qingyi_tq`、`npc_man_huzi_tq`、`npc_wan_tianming_tq`、`npc_tianwuzi_tq`、`npc_heishou_tq`、`npc_xuan_gu_tq`、`npc_yuan_yao_tq`（`huanjing → neidian`）；寒骊台 `npc_jiyin_hl`、`npc_qingyi_hl`、`npc_man_huzi_hl`、`npc_wan_tianming_hl`、`npc_tianwuzi_hl`、`npc_heishou_hl`（`cengjie → butian`）、`npc_wu_chou_hl`、`npc_xuan_gu_hl`（`cengjie → xuangu`）；灵泉 `npc_yuan_yao_lq`（`canhun →` 不撤）。
- ③④ 之前的存档点：中区石室那一处；n27 送进东区之后**没有存档点**——第 11 章礁岛同口径（`ch11-design.md` 第 4 节）；③ 与 ④ 之间隔着 n30 演出、n31 交出丹方与收鼎，平衡路按「③ 打完不回血」量 ④。若嫌重打太长，见本节末第 3 条。

### 4.2 命名、属性与美术

- `chapter=12`；`display_name_key = ch12.map.<短名>.name`；`region` 沿第 10–11 章 `luanxinghai_*` 前缀，由协调者定一个值（笔记无）。
- `bgm`：**只用已有曲子**（`bgm_cliff`、`bgm_wild`、`bgm_valley`、`bgm_town`、`bgm_river`、`bgm_night` 等），每图一首由关卡路挑，`tools/audiogen` 的 `MAP_BGM` 与 `docs/audio.md` 同一张表各加 7 条。
- **主题、时辰、背景全用现有组合**（16.1 第 29 条）；不加新主题、不做新背景。战斗背景见 8.2。
- `tools/mapgen/genmaps_ch12.py`（7 张）接进 `genmaps.py --check`；`tools/artgen/artgen.py` 烘 7 张并 `--check` 过；**本章不 patch 任何旧图**（C1 是改 `scripts/ch11/zhuzhong.lua` 一行，不是地图）。
- 存档点 7 处，每图一处，全部在主线挂点之外。
- **同一格只取第一个 trigger**：本章没有同格换触发器；三处走偏、`trigger_yutai`、`trigger_caiyao`、`trigger_yihai` 各占一格，与主线挂点、spawn、落点都错开（规则 16）。

### 4.3 与笔记不一致、待协调者拍板（地图）

1. **`trigger_yuqiao` 落在 spawn 上会让西区走不成**：n24 是 enter、末尾 `teleport("ch12_neidian", 石室)`，n23 的 teleport 一落 `yuqiao` 就演完 n24、直接送走——`trigger_yutai`（一层玉台，16.1 第 14 条）**永远走不到**，宝光阁一层也成了过场。触发器的完成旗标由脚本自己置（`WorldScene.cpp` 第 32–65 行），挪格子不影响 once 契约。**推荐**：`trigger_yuqiao` 仍是 enter、once、set `ch12.quba`，**挪到一层简易传送阵前一格**；spawn `yuqiao` 空着，玩家从玉桥走过一层、看玉台、踩到传送阵前才演 n24。`kScriptTransfers` 第 10 行（`quba.lua`）不变。
2. **`facility_cunji_binghuo` 的位置**：笔记「石殿内」在 ② 之后，② `defeat_is_fatal true`，败了要从小空间（n19 之前）重来。**推荐**：摆在光壁内侧、紧挨 spawn `guangbi`（G 表「与 spawn 相邻」那一句）；n20 演完后玩家必经它，② 前有档。
3. **`ch12_neidian` 东区无存档点**：③④ 两场致命仗之前最后的档在中区石室，中间夹着 n27–n30 四段演出。本稿按第 11 章礁岛先例接受；若协调者要加，推荐东区「寒骊台下」落点旁加一处 save 设施（对象名由协调者定，笔记无），存档点总数变 8。
4. 规则 30 的四处撞名（同 role 不恰接、不同 role 同名牌）见 5.5，地图上的修法是改 NPC 旗标或名牌，不动布局。

---

## 5. 角色

**新建 32、就地改写 0、删 2、一个字不动 4**（笔记 H 表）。凡说话、上场的人都有 `data/roles/` 条目；外观进 `data/visual/looks.json`，`sprite_preview.py --coverage` 缺图为 0。境界写 realm 枚举，「只说话」的 role 数值照 `Realm.h` 锚点（笔记 C.2）与第 10–11 章路人写法。**元婴诸人名牌一律 `CoreLate`**（本作上限，天眼术显示「结丹后期」，文案说「看不透」，16.1 第 9 条、16.2 O-9）。

### 5.1 剧情角色（地图上有对象的）

| id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 在场（图 `对象`：出场 → 退场）|
| --- | --- | --- | --- | --- | --- |
| `qu_hun_shadan` | 曲魂（身外化身）| 386 | CoreEarly | **不动** | 队伍（起点在队）→ n05 `party.remove` |
| `jin_qing` | 金青 | 415 | CoreEarly | 不动 | tushan `npc_jin_qing`：`dengdao → tuizhu`；dixue：`tuizhu → duoshe` |
| `hu_yue` | 胡月 | 416 | CoreEarly | 新建 | tushan：`dengdao → tuizhu`；dixue：`tuizhu → qixialian` |
| `jian_xiushi` | 简姓修士 | 416 | CoreMid | 新建 | 同胡月 |
| `shi_die` | 石蝶 | 416 | FoundationMid | 新建 | 同胡月 |
| `xuangu_shaonian` | 少年 | 427 | CoreLate | 新建 | dixue `npc_shaonian`：`duoshe → canyuan`（n06 挂点）|
| `wen_siyue` | 文思月 | 430 | FoundationEarly | 新建 | tushan `npc_wen_siyue`：`canyuan → lidao`（n07 挂点）|
| `wen_qiang_zhuji` | 文樯 | 369 | FoundationMid | 新建（旧 `wen_qiang` 不动，B11）| tushan `npc_wen_qiang`：`canyuan → lidao` |
| `xuan_gu` | 玄骨 | 428 | CoreLate | 新建（③ 友军、④ 敌人）| dadian `rudian → chuansong`（n09 挂点）；guiwu 花园 `yuanyao → xiaojie`；xiaojie 石山 `xiaojie → lingshen`；binghuo `lianjing → shidian`；neidian 塔前 `huanjing → neidian`、寒骊台 `cengjie → xuangu` |
| `zi_ling_xutian` | 紫灵 | 403 | FoundationLate | 新建（B8；① 友军）| dadian `npc_zi_ling` `rudian → chuansong`；guiwu 土坡 `npc_zi_ling` `tupo → guiwang`、花园 `npc_zi_ling_hy` `yuanyao → xiaojie` |
| `miao_zhanglao` | 苗长老 | 379 | CoreEarly | 不动 | dadian `rudian → chuansong` |
| `jiyin_zushi` | 极阴祖师 | 382 | CoreLate | 新建（`jiyin_fushen` 不动）| dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；xiaojie 石山 `fanlian → baishi`（n18 挂点）、峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；塔前 `huanjing → neidian`；寒骊台 `cengjie → butian` |
| `wu_chou` | 乌丑 | 382 | CoreMid | 不动（③ 友军；B4）| 同极阴，唯寒骊台 `cengjie → xuangu` |
| `qingyi_jushi` | 青易居士 | 435 | CoreLate | 新建 | 同极阴 |
| `wen_furen` | 温夫人 | 432 | CoreLate | 新建 | dadian `qunxiang → chuansong` |
| `wan_tianming` | 万天明 | 434 | CoreLate | 新建 | dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；塔前 `huanjing → neidian`；寒骊台 `cengjie → butian` |
| `man_huzi` | 蛮胡子 | 432 | CoreLate | 新建 | 同万天明 |
| `tianwuzi` | 天悟子 | 434 | CoreLate | 新建 | 同万天明，**guiwu 不在** |
| `heishou_laozhe` | 黑瘦老者 | 434 | CoreLate | 新建 | 同天悟子 |
| `xinggong_zhifa_ci` | 星宫执法长老 | 435 | CoreLate | 新建 | dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；binghuo `lianjing → shidian` |
| `xinggong_zhifa_leng` | 星宫执法长老 | 435 | CoreLate | 新建 | 同上（**名牌撞名，见 5.5**）|
| `ge_li` | 葛笠 | 437 | CoreEarly | 新建 | guiwu 土坡 `tupo → guiwang` |
| `yuan_yao_jiedan` | 元瑶 | 375 | CoreEarly | 新建（② 友军；B10）| guiwu 花园 `yuanyao → xiaojie`；峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；塔前 `huanjing → neidian`；灵泉 `npc_yuan_yao_lq` `canhun →` 章末不撤（n33 挂点）|

- **玄骨三段**：少年（`xuangu_shaonian`，n05 成形到 n06 自报）→ 玄骨（`xuan_gu`，n06 起）；`xuangu_shaonian` 与 `xuan_gu` 外观同一张「眉清目秀少年」（A23），`xuan_gu` 换绿袍（A55）。**名牌不出现「萧诧」**（他自报一次，在台词里）。
- **元瑶两段**：鬼雾里是「黑袍人」（`yuan_yao_heipao`，只在 ① 编成里，无地图对象），n14 现为女修之后是 `yuan_yao_jiedan`；旧 `yuan_yao`（第 10 章炼气）不动。文案「元瑶」二字只许在 key ≥ `ch12.yuanyao` 的段（笔记 M 表章内先后）。
- **紫灵**：本章是假丹（A44），旧 `zi_ling`（第 11 章在用）不动；过第一关后不闯冰火道，在 n15 之后退场（B6）。
- **乌丑**：`wu_chou.json` 一个字不动（第 11 章验收要它逐字节同第 10 章，16.2 O-11）；本章他只做 ③ 友军与群像，死于玄骨之手只写倒下（A106、16.1 第 22 条）。

### 5.2 只说话的（无地图对象，n 脚本 `talk` 用）

| id | 名牌 | 首见章 | 境界 | 用在 |
| --- | --- | --- | --- | --- |
| `dao_guairen` | 怪人 | 419 | CoreMid | n03（噬金虫收场，不打）|
| `dixue_guiying` | 怪影 | 425 | CoreLate | n05（问师门）|
| `xuangu_lvying` | 绿影 | 446 | CoreLate | n17（被电弧击碎，旁白）|

### 5.3 只在战斗里的（编成用，无地图对象）

| id | 名牌 | 首见章 | 境界 | 阵营 · 场次 |
| --- | --- | --- | --- | --- |
| `yuan_yao_heipao` | 黑袍人 | 437 | CoreEarly | 友军 · ① |
| `weiwu_hanggui` | 炜吾 | 441 | FoundationLate | 敌 · ① 第 0 波 |
| `gui_yecha` | 鬼夜叉 | 440 | CoreEarly | 敌 · ① 第 0 波 |
| `hui_ying` | 灰影 | 440 | CoreMid | 敌 · ① 第 1 波 |
| `shuangtou_guihu` | 双头怪虎 | 442 | CoreMid | 敌 · ① 第 1 波 |
| `sanshou_guiwu` | 三首鬼物 | 443 | CoreLate | 敌（首领）· ① 第 2 波 |
| `tiehuo_yiqun` | 铁火蚁群 | 462 | CoreEarly | 敌 · ② 第 0、1 波 |
| `tiehuo_yiqun_da` | 铁火蚁大群 | 463 | CoreMid | 敌（首领）· ② 第 2 波 |
| `langxing_qiling` | 狼形器灵 | 483 | CoreMid | 敌 · ③ 第 0 波（双头火形）|
| `langxing_qiling_yin` | 狼形器灵 | 485 | CoreMid | 敌（首领）· ③ 第 1 波（独角银形）|

另：`zi_ling_xutian`（① 友军）、`yuan_yao_jiedan`（② 友军）、`wu_chou` 与 `xuan_gu`（③ 友军）、`xuan_gu`（④ 敌）见 5.1。**器灵永远叫「狼形器灵」**，「银月」全作禁用（A126、B14）。

### 5.4 不动、删除、外观

- **不动**：`qu_hun_shadan`、`jin_qing`、`wu_chou`、`miao_zhanglao`（本章用），以及 `qu_hun`、`zi_ling`、`yuan_yao`、`wen_qiang`、`jiyin_fushen` 等旧 role（本章不用）。
- **删**（16.1 第 18 条、B15）：`xutian_hudian_kuilei`、`duoding_xiushi`，连同 `looks.json` 两条与 `data/text/battles.json` 两条旧 intro——协调者在第 11 章集成**之后**删（16.2 O-7）。
- **外观**：人形 23 个按名牌身份配（少年与玄骨同一张脸）；**非人形 9 个**（炜吾、鬼夜叉、灰影、双头怪虎、三首鬼物、两种蚁群、两形狼）进 `tools/artgen/sprites_enemies.py`，待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下（第 8、10 章同口径）。两形狼一张火色双头、一张银色独角（A99、A101）。

### 5.5 撤场 / 出场与规则 30（`NpcPresenceTests`、`validate.py` 规则 30）

- **跨图**（任一时刻一个 role 只在一张图）：上表每个 role 前一张图的退场旗标都在后一张图的出场旗标之前（目标链序，笔记 J.1），全部过得去；第 11 章 `ch11_yinsha_dao` 的 `npc_wu_chou` 以 `ch11.jiyin` 撤场、早于本章。**本章 NPC 在场旗标全在目标链上**，链外 8 个旗标（J.2）没有一个挂在 NPC 上。
- **同图同 role**：规则 30 只放行「一个的 `hidden_flag` **恰是**另一个的 `visible_flag`」（`validate.py` 第 3132–3142 行），「首尾不重叠」不够。逐对核过：
  - xiaojie 石山 / 峡谷两组（极阴、乌丑、青易）：`baishi` ＝ `baishi`，**过**。
  - **guiwu 两个紫灵**：土坡 hidden `guiwang`、花园 visible `yuanyao`——**不恰接，会红**。推荐花园 `npc_zi_ling_hy` 的 visible 改 `ch12.guiwang`（鬼王一死她就跟他出雾；花园在咽喉格之后，提早出场玩家看不见）。
  - **neidian 塔前 / 寒骊台八个 role**（极阴、乌丑、青易、蛮胡子、万天明、天悟子、黑瘦老者、玄骨）：塔前 hidden `neidian`、寒骊台 visible `cengjie`——**会红**。推荐寒骊台八个对象的 visible 一律改 `ch12.neidian`（东区以墙隔开，n27 送进去之前玩家到不了）；连带 K13 `qingyi_qianlan_dating` 的 `when` 补 `ch12.neidian >= 1`（「when 含 visible_flag」口径）。
  - **neidian 两个元瑶**：塔前 hidden `neidian`、灵泉 visible `canhun`——**会红**。推荐灵泉 `npc_yuan_yao_lq` 的 visible 改 `ch12.neidian`（南区同理到不了；n33 是 NPC 挂点，`lingquan.lua` 开头判 `ch12.canhun` 未置则一句 return）。
- **不同 role 同名牌**：`xinggong_zhifa_ci` 与 `xinggong_zhifa_leng` 名牌都是「星宫执法长老」，在 dadian、guiwu 花园、binghuo 石殿**三处同框**——规则 30 同名一条必红。推荐照原著两人的样子分名牌：一位「慈眉善目的白衣老者」、一位「冷脸」（ch435，`novel.py read 435` 核过），名牌写「慈眉长老」「冷脸长老」，台词里仍称「星宫执法长老」；或协调者另拟，只要两块不同。
- 上面四处都只改旗标或名牌，不改 id、不改布局；改后 H 表、J.1「谁读」列与 K13 一并回写笔记。

---

## 6. 支线（2 条，都可选）

| | Z1 埋骨（可选）| Z2 傀儡残骸（可选）|
| --- | --- | --- |
| 出处 | ch438–439：鬼雾里一具快到结丹中期的修士遗骸（玉真人，A49），他取了辟火宝衣、把骨埋了，说是埋骨的报酬 | ch475–476：内殿狼首傀儡、蛇卫被蛮胡子砸碎，他把残骸一一收进储物袋（A91）|
| 接取 | `ch12.tupo`（进了鬼冤之地）| `ch12.neidian`（进了塔）|
| 步骤 | 一步：正路旁死角 `trigger_yihai`（interact、once、guard `ch12.tupo`、set_flag `ch12.z1`）；抉择「取衣而后埋骨 / 只埋骨」，前者 `give("story_bihuo_baoyi", 1)`，两支都置 `ch12.z1` | 一步：**无独立挂点**，在 n27 `cengjie.lua` 里一处抉择「收残骸 / 不收」；收则 `give("story_kuilei_canhai", 1)`、`flag.set("ch12.z2")` |
| 完成 | `ch12.z1 >= 1` | `ch12.z2 >= 1` |
| 失败 / 过期 | `ch12.guiwang`（鬼王一战之后）| `ch12.cengjie`（n27 演完）|
| 奖励 | 不进 rewards（`story_bihuo_baoyi` 由脚本 `give`；n20 旁白多一句「宝衣挡了热浪」）| 不进 rewards（第 13 章傀儡研究用）|
| 文件 | `data/quests/q12_maigu.json`，`origin "原著"` | `data/quests/q12_kuilei_canhai.json`，`origin "原著"` |

- **完成压过失败**（`Quests.cpp` 第 106–113 行先判 complete）。所以：Z1 的 `yihai.lua` **开头判 `ch12.guiwang` 已置 → 一句旁白（鬼王既灭、雾势已变，那具遗骸寻不着了）return、不置 `ch12.z1`**（触发器完成旗标由脚本置，return 不烧格子，`WorldScene.cpp` 第 32–65 行），否则玩家打完鬼王回头埋骨，任务会从「已过期」翻成「完成」；Z2 在 `cengjie.lua` 里 **`flag.set("ch12.z2")` 写在 `flag.set("ch12.cengjie")` 之前**——`kOffChainFlags` 的窗口是「`ch12.neidian` 之后、`ch12.cengjie` 之前」，`ch12.z1` 的窗口是「`ch12.tupo` 之后、`ch12.guiwang` 之前」。
- **为什么只有这两条**：本章他亲身在场、有前因后果、又不在主线上的旁枝只这两件；其余可问可看的都落在路径行动（16.3，13 条 inquire）与 `trigger_caiyao`（采药一次，不立任务）。**不凑数**。
- **不做成支线的**：寿元果（葛笠的事，他原路返回，A48）；文思月被同门逼婚（他听出来了、没插手，B12）；紫灵买图（只闯第一关，K6）；温夫人、万天明一行（他与之无交集）；补天丹（他没得，A105）。
- **配额**：本章 2 可选、0 必经；全作 45 条 / 必经 14 条的账由协调者核，若要本章出一条必经，见 16.2 O-8（Z1 改必经、链变 34 步）。

---

## 7. 系统：残图门闸、三关玩法、依附极阴岛、曲魂离队、虚天鼎与乾蓝冰珠、多方混战

**先查了已有的，再定做多少**：物品门闸 `item.count` 与 `take` 返回值、`choice` 循环、`party.remove`（`api.lua` 第 292–321 行）、`today()` 与 `advance_days`（单次 ≤ 3650，本章单次 ≤ 90）、`teleport(map_id, x, y)`、`kScriptTransfers` 的五元写法（第 11 章回城阵先例）、道具 `castMagic`（`talisman_tianleizi` 先例）、编成的 `wave` / `faction "ally"` / `killable_by` / `charge` / `actions`、NPC `visible_flag` / `hidden_flag`、路径行动的 `when` / `until` / `reveal`。**本章不加任何 Lua 命令、不加存档字段（仍 v9）、不改 `src/`**；契约只有两条判据项：**E1 空队伍**、**E2 同一 role 先友后敌**（10.1）。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **残图门闸** | n08 `rudian.lua` 开头判 `item.count("story_jinpa_ditu") >= 1`；不足一支照写（一句旁白后 return），**数据上不可达**——第 8 章 `shadan.lua` 给、第 9–11 章脚本 0 处 `take`、本章 0 处 `take`。文案称它「虚天残图」（物品名不改）。「醒」只在 n06：玄骨肋骨里取出同样一块，他的锦帕现金剑、恒指西北（A27–A28），置 `ch12.canyuan`；n07 起「循图」 | 不做「收集残图」、不新给残图、不 `take` 残图；不做 portal 门闸（大厅无门，n08 是落点上的 enter）| 原著一人一图（A33）、各凭手中那一块入殿（A32）；大纲「遗址取残图」与原著相反（B1，回写大纲）|
| **第一关·鬼冤之地＝迷路** | 三处岔口、三条错路；错路尽头 `trigger_zoupian_a/b/c`（enter、once 否、guard `ch12.tupo`、无 set_flag）→ `zoupian.lua`：开头判 `ch12.guiwang` 已置则 return，否则一句旁白（鬼雾把他推回）＋ `teleport("ch12_guiwu", x, y)` 回**那一处**岔口（三条错路三组坐标；一份脚本认不出是哪条错路，见本节末，0 处 `flag.set`）；正路由白骨与淡雾标出，K9 葛笠给提示；第三岔之后 ①（16.1 第 10、11 条）| 不做随机迷宫、不计步数、不做雾中遭遇、走错不扣血不扣日 | 原著是跟着啼魂兽开出的通道走、偏了就被鬼雾推回（ch437–438）；做成战斗则与 ① 重复，做成纯对白则地图无用 |
| **第二关·冰火道＝限时** | n20 `flag.set("ch12.binghuo_ri", today())`；草地、沼泽两处「罗烟步（不耗日）/ 常速（＋1 日）」；n21 开头算 `today() − ch12.binghuo_ri`，绕路一支说「至少多耽误两日、五日内过不去」挡回（A72）；固定段 2 日 ＋ 选择至多 2 日 ＝ **至多 4 日，永不超时**；禁飞只在旁白（A69）；② 是「耐久」：三波、波间不回血不回法（16.1 第 12、13 条）| 不做超时致死、不做屏幕计时器、不做「法力当燃料」的走路消耗 | 「五日」（A71）要落在玩家手上——两处抉择让他看见日子在走；真做超时就是死局。法力的紧，落在 ② 波间不回 |
| **第三关·宝光阁＝取舍** | 一层玉台 `trigger_yutai` 只给描述、不给物；去二层的简易传送阵 `take("material_lingshi", 4)`，不成扣中阶 1，再不成「以阵法心得推演半日」`advance_days(1)` 照走；二层先取一件（血色披风 / 五色铜环二选一，记 `ch12.quba_xian` 1 / 2），再选「噬金虫结甲」取另一件——**两件都到手**（A83–A84，16.1 第 14 条）| 不做「真的只许一件」、不做背包上限、不做抢先竞速 | 只给一件会让 ③（铜环套狼，A101）或第 13 章（披风逃命）断线；「取舍」的分量在规矩（每人一样，A78）与他破规矩的那一下 |
| **极妙幻境＝心境** | n25 两段抉择（色：长廊白云仙音；惧：黑殿里自称小妹的幻象，A85–A86）；错选只换一句旁白 ＋ 婆罗珠灼热提示（A87）、再选一次，**不失败**；一次就选对的段置 `ch12.xinjing` 对应位（位 1 色、位 2 惧），只供 n33 章末一句（16.1 第 15 条）| 不做失败分支、不做回档点、不做心魔数值 | 55 分钟装不下第二条路；心境是「他怎么走过去」的质感，不是关卡 |
| **依附极阴岛** | n18（挂 NPC `npc_jiyin`）：黑云、蛛丝（A62）、放出蜘蛛（`story_xueyu_zhizhu` **不扣**）、青易点破「取鼎最佳灵兽」（A63）；三方传音——三选一「拜极阴 / 拜青易 / 两边都不拜」，后两项各接对方一句传音再回到同一选择，只有拜极阴往下走；玄骨那条传音劝他拜，理由是九曲灵参的丹方只在玄骨手里（A65）；`give("story_xuanyin_huan", 1)`；此后极阴一行在 binghuo、neidian 群像里，路径行动 0 条挂他（16.1 第 16 条）| 不做阵营声望、不做「拜青易」的另一条线、记名弟子不给任何特权 | 极阴图的是蜘蛛、不是他（B5）；拜师是被三方夹着点的头。n32 噬金虫除去追踪暗记（A115），第 13 章不必再演追杀 |
| **曲魂离队与队伍变化** | n05 `party.remove("qu_hun_shadan")` 看返回值，false 只记一句照走；`qu_hun_shadan.json` 一个字不动；夺舍后的躯体是新 role `xuangu_shaonian` → `xuan_gu`；抉择「敲引魂钟」必落空、钟不扣（A22）；**此后队伍为空到章末**；临时同伴只写死在编成里（① 紫灵 ＋ 黑袍人、② 元瑶、③ 乌丑 ＋ 玄骨）；本章 `party.add` 0 处（16.1 第 5、6 条）| 不做临时入队、不做分身复活或重炼、不在菜单队伍页留空位提示 | 曲魂之躯在 ch493 随玄骨化灰，只许旁白提；`party.add` 临时同伴要配对 `remove`，存档留人会弄脏第 13 章接口。**空队伍从未被任何一章跑过**——地图、战斗进场（`BattleScene.cpp` 第 418 行）、存读档、队伍页都要有判据（契约 E1）|
| **噬金虫与辟邪神雷**（两样新手段）| n03 `magic.learn("magic_shijin_chongqun")` 看返回值（金，`boost hits`，A9）；`story_shijin_chong` 不动。神雷做道具施法：`talisman_bixie_shenlei`（kind `talisman`、`castMagic magic_bixie_shenlei`〔木〕、`tradeable false`、`price 0`），n13、n31 战前各 `give` 2、战后 `take` 余数（ch444 耗尽、ch494 荡然无存）；全仓 `magic.learn("magic_bixie_shenlei")` 0 处（16.1 第 19、21 条）| 神雷不做常驻法术；不做剑阵 / 分光法术，「九柄」只在文案（A20）| 原著 ①②④ 的主手段都是噬金虫与神雷；神雷是剑上攒的那一点，打完就没了 |
| **虚天鼎与乾蓝冰珠** | 全是 story：n31 `give` 丹方玉简、虚天鼎（A109–A110）；④ 后 `give` 乾蓝冰珠、玄骨肋骨、金雷竹小箭（A113–A114）；n29 ③ 后 `give` 狼首玉如意；n33 `give` 万年灵乳、养魂木根须；章末一句「鼎盖如生铸、打不开」（A121）、玉如意里的器灵不受控（A122）| **不开鼎**（ch641 才清点）；冰珠不做药效（第 14 章 ch544 破结丹后期用）；灵乳不做回法；玉如意不给 weapon / castMagic；九曲灵参不建配方（16.1 第 20、31 条）；**补天丹不给**（落入蛮胡子手中，A105）；青火雷不给（元瑶留着自用，A75）| 本章拿到的全是「以后的东西」：结丹初期打不开、用不上；做成可用道具会被第 13–14 章当手段重复消耗 |
| **多方混战的编成** | **不加第三阵营**：③ 敌方只有狼形器灵两波，友军写死乌丑、玄骨——他们是**争夺者**，伤害照算、各抢各的；第 1 波银形 `killable_by ["金"]`，**两名友军不配金类法术**，所以只有玩家的金（五色铜环的文案、祭金蚨、噬金虫）要得了命＝「抢在他们之前收服」；元婴大战只在 ③ 开场白、蓄势文案与 n29 / n30 旁白（不点名谁胜）；胜利文案「收服」，战后两人各自恼怒（16.1 第 17 条）| 不做三方阵营、不做 AI 互殴、不做「抢夺计分」；元婴们不进任何编成 | 引擎只有 ally / enemy（`BattleLoader.cpp` 第 106 行），加第三阵营是 `BattleState` 与 AI 的大项；原著他在元婴大战里只能退后（ch482），他能抢的就是逃下台的那只狼。`xuan_gu` 在 ③ 是友军、在 ④ 是敌人，`wu_chou` 在 ③ 是友军——友军是否忽略 `toughness / weaknesses / killable_by / charge` 没有用例钉着，要判据（契约 E2）|

**队伍与手段对照**（写测试时读表，不抄数；供第 13 章参考）：

| | 起点（ch11-end，估，笔记 C.3）| n05 之后 | ①④ 战中 | 章末 |
| --- | --- | --- | --- | --- |
| 境界 / 上限 / `formerRealm` | 31 / 31 / 22 | 同左 | 同左 | 31 / 31 / 22（`realm.*` 0 处）|
| 队伍 | `[qu_hun_shadan]` | 空 | 空（友军写死在编成）| **空** |
| 法术 | 第 10 章那 11 门 | 12 门（n03 已学噬金虫）| 12 门 ＋ 神雷（道具）| 12 门 |
| `talisman_bixie_shenlei` | 0 | 0 | 2（战前给）| 0（战后收尽）|
| 入殿凭据 | `story_jinpa_ditu` ×1 | 同左 | 同左 | 同左（不扣）|

**与笔记不一致、待协调者拍板（系统）**：笔记 N.4 把三处走偏都挂同一份 `scripts/ch12/zoupian.lua`、要它回「那一处」岔口——但 Lua 侧拿不到是哪个触发器起的事件、也拿不到玩家坐标（`api.lua` 只有旗标、物品、境界、日历、队伍等查询，`ScriptHost.cpp` 第 250–329 行），一份脚本只能把三条错路送回同一点。**推荐**拆成 `zoupian_a.lua` / `zoupian_b.lua` / `zoupian_c.lua` 三份（内容只差坐标），`kScriptTransfers` 三条五元各写各的脚本名；退一步是保留一份、三处都送回第一处岔口（惩罚变重，迷路仍成立）。两种都不改旗标与挂点。

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** ＝ 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **4 场必打**（① n13 鬼王、② n21 铁火蚁、③ n29 狼形器灵、④ n31 玄骨），四场都是原著的仗；**可选：无**（无遭遇区、殿内禁斗、无切磋）。
账：方案全作 52 场，第 1–11 章合计 41 场，**本章 4 场**，余 **7 场**给第 13 章（低强度，约 2）与第 14 章（约 5）。遗址看守怪人（ch419–421）做旁白；若第 14 章用不满 5 场，可升格为本章第 5 场，id 预留 `b12_tushan_guairen`（16.2 O-1）。

**这四场就是「独行」的说明书**（曲魂 n05 离队之后，队伍为空，第 7 节）：

| # | 谁上场 | 他与谁并肩 |
| --- | --- | --- |
| ① | 韩立 ＋ 紫灵 ＋ 黑袍人（写死友军）| **临时同伴**：雾里三人议定同行，各出其力 |
| ② | 韩立 ＋ 元瑶（写死友军）| **受托护送**：她在光团里求救，他开价啼魂兽 |
| ③ | 韩立 ＋ 乌丑 ＋ 玄骨（写死友军）| **争夺者**：同一只狼三个人抢，元婴在头顶打 |
| ④ | 韩立一人 | **独战**：第 1–11 章首领战都有队友或友军，这是他第一次真正一个人 |

**友军由玩家操纵**（`BattleScene.cpp` 第 974 行起 `runToAllyTurn`：轮到 ally 就停给玩家）。所以 ③ 的「争夺」只在文案；机制上靠「友军不配金类法术」把收服留给韩立自己（7 节末行、8.2 ③）。

### 8.1 意图玩家

- **共同**：结丹初期（700 / 470，攻 62、防 46——笔记 C.2，`Realm.cpp` 注释「暂定」）；全章 31 / 31，修为给了也冲不了关（`BreakthroughBlock` 照第 11 章口径回绝）。法术 12 门：第 10 章章末那 11 门（火弹术、流沙术、冰冻术、天眼术、御风决、祭金蚨子母刃、祭青蛟旗、青元剑芒、祭青火瘴、祭小刀符宝、祭红线遁光针——`ch10-design.md` 8.1 ③；**以 ch11-end 真档为准**，笔记 C.3 标「必须核」）＋ n03 学的噬金虫群（金）。兵刃：剑（`weapon_qingzhu_fengyun_jian`，一件代表「九柄」）、拳、暗器（`weapon_wuming_sixian`）。道具：中阶灵石约 20（估，回法力）、①④ 战前各给辟邪神雷 2 枚（木）。**队伍为空**。
- **①**：刚过 n12–n13 的迷路，满状态进场（存档点在土坡）。友军紫灵（筑基后期，剑）、黑袍人（结丹初期，火）。
- **②**：n20 之后、n21 一段旁白（探路者殒命、绕路被挡）之后进场；**三波之间不回血不回法**（引擎既有）。友军元瑶（结丹初期，火：青火雷）。
- **③**：n27–n29 一路演出、无战斗；东区无存档点。友军乌丑（结丹中期，`wu_chou` 900 / 78 / 62，水：冰箭术）、玄骨（结丹后期档，火）。
- **④**：紧接 n30 演出（他被斩飞、皇鳞甲救命，A104）与 n31（乌丑倒下、交出丹方、收鼎），**③ 之后不回血**、中间无存档点——平衡路按「刚打完 ③」量 ④（第 11 章 ③④ 同口径）。无友军。

### 8.2 四张必打编成

| # | 编成 id | 敌方（按波次）| 我方 / 友军 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b12_guiwu_guiwang`（新）| **第 0 波** 炜吾 ×2（`weiwu_hanggui`，筑基后期；破绽火、剑）、鬼夜叉 ×1（`gui_yecha`，结丹初期；破绽木、火）；**第 1 波** 灰影 ×1（`hui_ying`，结丹中期；金、火）、双头怪虎 ×1（`shuangtou_guihu`，结丹中期；剑、木）；**第 2 波** 三首鬼物 ×1（`sanshou_guiwu`，结丹后期，首领；破绽木、火，`killable_by ["木","火"]`；蓄势「怪笑」`all true`）| 韩立 ＋ `zi_ling_xutian`、`yuan_yao_heipao` | 否 | **致命** | **越打越大**：第一波交给友军、自己省法；第二波噬金虫咬灰影、剑与剑芒对怪虎；第三波首领攒怪笑（全体）——**破势打断蓄势**，神雷两发收尾。K8 先揭「木」（鬼物畏雷）、K9 先揭鬼夜叉「火」（16.3）|
| ② | `b12_heishamo_tiehuoyi`（新）| **第 0 波** 铁火蚁群 ×2（`tiehuo_yiqun`，结丹初期；破绽水、金，`killable_by ["水","金"]`）；**第 1 波** 铁火蚁群 ×3；**第 2 波** 铁火蚁大群 ×1（`tiehuo_yiqun_da`，结丹中期，首领；同上破绽与 killable_by；蓄势「黑炎」`all true`、`actions 2`）| 韩立 ＋ `yuan_yao_jiedan` | 否 | **致命** | **耐久**：剑、拳、火最多把蚁群压到 1，要命的只有冰冻术（水）与噬金虫（金）；三波不回法，玩家得把法力留到第二、三波；元瑶的火只帮着削架势。原著「冰水法术方能伤」（A73）就落在 `killable_by` 上 |
| ③ | `b12_neidian_duoding`（**就地改写**）| **第 0 波** 狼形器灵 ×1（`langxing_qiling`，结丹中期，双头火形；破绽水、金，`killable_by` 不写）；**第 1 波** 狼形器灵 ×1（`langxing_qiling_yin`，结丹中期，独角银形，首领；破绽金，`killable_by ["金"]`；蓄势「银光弹」`all true`）| 韩立 ＋ `wu_chou`、`xuan_gu`（争夺者）| 否 | **致命** | **多方混战的缩影**：第 0 波归零＝火狼被缚、化银（文案）；第 1 波只有金要得了命（祭金蚨、噬金虫；文案写五色铜环套住）——**两名友军都不配金类法术**，收服只能是他。乌丑的水能削火形的架势，玄骨的火打不到破绽 |
| ④ | `b12_hanlitai_xuangu`（新）| 玄骨 ×1（`xuan_gu`，结丹后期档；破绽木，`killable_by ["木"]`；蓄势「修罗圣火」`every 3`、`actions 2`）| 韩立一人 | 否 | **致命** | **独战首领**：他一回合动两次、每三回合攒一发圣火；剑与噬金虫只能压到 1，**只有木**（青元剑芒、神雷）要得了命；神雷用早了，圣火压上来就没有收尾的了。五色铜环对圣火无效（ch492），**不进破绽** |

- **必须守住的**（数值归平衡路，按 `tests/BattleHand.h` 那只手量；本表只写意图与破绽）：四张 `hero_absent false`、`can_escape false`、`defeat_is_fatal true`、`rewards.spirit_stones 0`、`drops []`（物品一律由脚本 `give`）、`rewards.cultivation > 0`；① 三波、友军恰两名、首领 `killable_by` 恰「木、火」；② 三波、友军恰 `yuan_yao_jiedan`、两种蚁群 `killable_by` 恰「水、金」、大群 `actions 2`；③ 两波、友军恰 `wu_chou` 与 `xuan_gu`、**两名友军的 `magics` 里没有金类**、银形 `killable_by` 恰「金」；④ 无友军、`killable_by` 恰「木」、`charge.every 3`、`actions 2`。`killable_by` 一律非空且 ⊆ `weaknesses`；胜负只看气血归零。
- **E2 判据**（契约）：`xuan_gu` 在 ③ 做 ally、在 ④ 做 enemy，`wu_chou` 在 ③ 做 ally——ally 不吃 `toughness / weaknesses / killable_by / charge`、可被敌方打倒不报错；enemy 照常生效（16.2 O-4）。
- **E1 判据**（契约）：空队伍下四场进场都只有韩立 ＋ 编成里写死的友军；战后 `units[0]` 回写韩立气血（`BattleScene.cpp` 第 996 行）不越界。
- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，C2）：`CHAPTER_MEANS[12]` ＝ 拳（hand）、火（`magic_huodan_shu`）、土（`magic_liusha_shu`）、水（`magic_bingdong_shu`）、金（`magic_ji_jinfu`；外加 n03 学的 `magic_shijin_chongqun`）、木（`magic_qingyuan_jianmang`）、暗器（`weapon_wuming_sixian`）、剑（`weapon_lieyang_jian`，第 6 章）；`BATTLE_EXTRA_MEANS`：① `("木","give","talisman_bixie_shenlei","ch12")` ＋ 两友军的 `role_magic`；② `("火","role_magic","magic_qinghuo_lei","yuan_yao_jiedan")`；③ `("水","role_magic","magic_bingjian_shu","wu_chou")`、`("火","role_magic","magic_xiuluo_shenghuo","xuan_gu")`；④ `("木","give","talisman_bixie_shenlei","ch12")`；`MEANS_LAST_CHAPTER = 12`；selftest 探针挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`、`probe["chapter"] = 12`（16.1 第 25 条）。
- **战斗背景**（`data/visual/battles.json`，都是现有名）：① `mountain_forest_dusk`、② `cliff_top`、③④ `secret_room`；每条都要指得出读它的那一行代码、有走真实入口的测试（handoff 第 6 节第 5 条）。
- **开场白与文案**（`data/text/ch12_battle.json`）：`ch12.battle.<短名>.intro`、蓄势 `ch12.battle.<短名>.charge`；① 蓄势文案用「附灵术」（M 表 442）；③ 开场白写头顶元婴混战、**不点名谁胜**；四场胜利文案：① 鬼物飞灰、② 蚁群退散、③ 「收服」、④ 「圣火反噬」（**不写「杀死玄骨」**，B3）。旧的两条 intro 与 `b12_xutian_hudian_kuilei` 由协调者删（16.2 O-7）。
- **与笔记不一致、待协调者拍板（战斗）**：「修罗圣火」原著到 ch491–492 才成（`novel.py find 修罗` 核过：此前只有第 8 章的黑煞修罗功），③ 是 ch483–485——`xuan_gu` 的 `magics` 若放 `magic_xiuluo_shenghuo`，玩家操纵 ③ 的玄骨时技能栏就先露了它（章级词表放行，章内先后不对）。**推荐**：`xuan_gu` 的 `magics` 只放一门现有火法（平衡路从 `magic_moyan_huo` 一类复用名里挑），④ 的修罗圣火只走 `charge.text_key`（蓄势文案点名）；`BATTLE_EXTRA_MEANS` ③ 那一条跟着改成该火法；`magic_xiuluo_shenghuo` 是否还建由协调者定（不建则 L.2 新法术 4 → 3）。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 鬼王 | ch437：曾有一队遇神智大开的鬼王全灭（A47）；ch440–444：勾魂鬼音、鬼夜叉、炜吾、双头怪虎接连扑来，最后三首鬼物现身，神雷耗尽、鬼物化灰 | 三波：悍鬼与鬼夜叉 → 灰影与双头怪虎 → 三首鬼物；神雷战前给、战后收 | 第 11 章 ④ 妖尸是「困而不杀」，本场鬼物可杀；前十一章没有鬼物首领。飞剑「四化八、散去四」是分化之影（B9），**不写损剑** |
| ② 铁火蚁 | ch462–464：近万铁火蚁围住光团里的元瑶，只有冰水之法伤得了它们；他出手解围，开价要啼魂兽 | 一场、三波、不可逃；胜后 `give` 啼魂兽、鸣魂珠 | 第 11 章 ① 诱妖可重复、可逃；本场一次、不可逃；「近万」用三波群体单位表达 |
| ③ 狼形器灵 | ch483–485：鼎中古宝化双头火狼破鼎、逃向结丹修士处；他以花篮与五枚连串的铜环收服化银的独角狼，得玉如意；头顶元婴混战 | 两波；友军是同在场的两个争夺者；胜利＝收服 | 前十一章没有「收服」战；本作第一场抢东西的仗。元婴大战他在原著里只能退后（ch482），**不进编成** |
| ④ 玄骨 | ch491–493：玄骨融冰焰为修罗圣火要他死，他以剑芒封路、以神雷克火，圣火失控反噬，玄骨化为灰烬 | 独战；破绽只有木；胜利文案「圣火反噬」| 原著玄骨是**自己的火失控**而亡（B3）——「气血归零」在文案里落成反噬，不写他杀了玄骨；元婴级敌人可写灰烬（16.1 第 22 条）|
| 看守怪人 | ch419–421：噬金虫数万当众吞食 | 旁白（n03，抉择两支都以噬金虫收场）| 一边倒，做成仗只剩按一下；见 O-1 |
| 小石山绿影 | ch452：扑来被金色电弧击碎 | 旁白（n17）| 一击之事 |
| 毒龙会三人 | ch430：三道青元剑芒 | 旁白（n07）| 一瞬间 |
| 内殿傀儡 | ch475–476：蛮胡子赤手砸碎 | 旁白 ＋ Z2 | 他没出手 |
| 乌丑之死 | ch489：死于玄骨鬼爪 | 旁白（n31），只写倒下 | 他在旁未出手（B4）|
| 翅恶、妖冠蛇 | ch418、ch424 | n02 布水旗、n04 旁白 | 解谜与演出，不进编成 |

**不做的仗**：正魔元婴大战（他退后）、卜绪与艳女闯关、星宫二人密谈（他不在场，16.1 第 23 条）。

### 8.4 野外遭遇

**本章不放遭遇区**：荒岛无灵脉、草木不生（A3），地洞与遗址里能动的东西都在主线演出里；进殿之后一路是禁制、关卡与正魔元婴，殿内禁斗（A39），关卡里的敌人（鬼物、铁火蚁、器灵）都是剧情的。遭遇表的掉落又只许灵草——殿内采药只有小空间一天（`trigger_caiyao`）。修为的来路是四场战斗的 `rewards.cultivation`，本章冲不了关，不需要刷。

---

## 9. 经济账

**四本账：灵石（`material_lingshi`、`material_lingshi_zhong`）、辟邪神雷（`talisman_bixie_shenlei`）、story 物品、材料与灵草**。本章 **0 店、0 配方、0 门闸要钱**（殿内不卖不买，原著无坊市；16.1 第 28 条）。起点全部是「估」（笔记 C.3：`tests/fixtures/ch11-end-*.sav` 尚未生成），下面只写本章的去向与恒等式，不先宣称终局数。

**来源**

| 项 | 低阶灵石 | 中阶灵石 | 神雷 | 其他 |
| --- | --- | --- | --- | --- |
| 起点（ch11-end，**估**）| 数百到四千，两侧差 20 | 20 / 20 | 0 | 残图、白珍珠、噬金虫、血玉蜘蛛、引魂钟、青竹蜂云剑各 1（**必须拿真档核**）|
| n03 怪人 | | | | 花篮古宝、脂阳鸟画轴 |
| n13 鬼王（战前）| | | ＋2 | |
| Z1 埋骨（取衣一支）| | | | 辟火宝衣 |
| n16 捕参 | | | | 九曲灵参 |
| n18 拜师 | | | | 玄阴环 |
| n19 峡谷口 | | | | 白犀佩、寒冰珠（n23 转赠只改文案）|
| `trigger_caiyao` | | | | 现有灵草一种 ×2（id 由协调者从 `data/items/herbs/` 选，16.2 O-13）|
| n21 铁火蚁（战后）| | | | 啼魂兽、鸣魂珠 |
| n22 晶柱 | | | | 炼晶 ×1（`tradeable false`）|
| n23 石殿 | | | | 婆罗珠、青冥针符宝、皇鳞甲 |
| n24 宝光阁 | | | | 血色披风、五色铜环 |
| n27 Z2（收）| | | | 傀儡残骸 |
| n28 台背 | | | | 内殿卷轴 |
| n29 夺鼎 | | | | 金丝蚕 ×3（讨要一支）、狼首玉如意（③ 后）|
| n31 玄骨 | | | ＋2（战前）| 丹方玉简、虚天鼎；④ 后乾蓝冰珠、玄骨肋骨、金雷竹小箭 |
| n33 灵泉 | | | | 万年灵乳、养魂木根须 |
| 四场战斗 | 0 | 0 | | 只给修为（`rewards.cultivation > 0`）|

**去向**

| 项 | 低阶灵石 | 中阶灵石 | 神雷 | 其他 |
| --- | --- | --- | --- | --- |
| n01 登岛 | | | | 白珍珠 −1（看返回值，0 也照走）|
| n05 夺舍 | | | | 脂阳鸟画轴 −1（看返回值）；引魂钟**不扣**；曲魂离队 |
| n13 战后 | | | −余数 | |
| n24 简易传送阵 | −4（`take` 成则止）| −1（只在低阶不足时）| | 再不成：`advance_days(1)` 照走 |
| n30 化身符 | | | | 一只血玉蜘蛛两截——**不扣**（一件代表一对，A103、A107）|
| n31 战后 | | | −余数 | |
| n32 密室传送阵 | −4（同 n24）| −1（同 n24）| | 同 n24 |

**账要对得上的几条**（交测试路，T 前缀；直接读 `scripts/ch12/*.lua` 与 `data/battles/b12_*.json`）：

1. **本章 `give("material_lingshi")` 0 处、`take("material_lingshi", 4)` 恰 2 处**（`quba.lua`、`canhun.lua`）；**`take("material_lingshi_zhong", 1)` 恰 2 处**，只在同一脚本里前一个 `take` 返回 false 时走；再不成一句「以阵法心得推演半日」＋ `advance_days(1)`。**没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训）。原著两处都是「往阵里装几块灵石」（ch469、ch496），数目原著没给，取 4。
2. **辟邪神雷**：`give("talisman_bixie_shenlei", 2)` 恰 2 处（`guiwang.lua`、`xuangu.lua`，都在 `battle()` 之前）；`take` 恰 2 处、数量取 `item.count("talisman_bixie_shenlei")` 的余数、看返回值（都在 `battle()` 之后）；**章末 0**；物品 `tradeable false`、`price 0`。全仓 `magic.learn("magic_bixie_shenlei")` 0 处。
3. **story 的 `take`** 恰 2 处：`story_bai_zhenzhu`（n01）、`story_zhiyang_huazhou`（n05）；`story_jinpa_ditu`、`story_yinhun_zhong`、`story_xueyu_zhizhu`、`story_shijin_chong` 全章 0 处 `take`。
4. **材料与灵草**：`give("material_lianjing", 1)` 恰 1 处（n22）；灵草 `give` 恰 1 处、数量 2（`trigger_caiyao`，once）；遭遇区 0（8.4）。
5. 四张必打编成 `rewards.spirit_stones` 都是 0、`drops` 都空、`rewards.cultivation` 都大于 0。
6. **本章 `shop(` 0 处**、新配方 0 张（九曲灵参丹方是 story 玉简，16.1 第 31 条）、`data/shops/ch12_*` 0 份。
7. **章末净增**（意图玩家：Z1 取衣、Z2 收、n29 讨要）：story 24 种（金丝蚕 3 件，其余各 1；画轴给了又收不计）、炼晶 1、灵草 2、白珍珠 −1、神雷 0；低阶灵石 **−8**、中阶 **0**。不讨要金丝蚕、不做 Z1 / Z2 时分别少对应一种，账照样闭合。
8. **两侧同账**：通关两侧走同一组选项（笔记 N.1 第 5 条那一串：n01 应允、n02 一次选对……n32 吞；Z1 取衣埋骨、Z2 收、采药 1 次）；章末 `day` − 起点 `day` ＝ **182**（字面量写死；笔记 N.2；兜底那一日不在意图玩家路上）；两侧低阶灵石差 ＝ 起点差、中阶同。起点若与估值不同（16.2 O-3），增量不变、起点按真档改。
9. **退路**：本章所有门闸都是旗标或必有物品（残图第 8 章给、0 处 `take`）；`party.remove` 返回 false 只记一句照走；白珍珠、画轴、两次神雷余数的 `take` 全看返回值——**没有一处会因物品或钱不够而卡死**。

**对照：原著这一段的量级**。虚天殿一行他没有赚一块灵石，花出去的也只是两次启阵的几块；他带出来的是一口打不开的鼎、一颗要到结丹后期才用得上的冰珠、一株凝婴用的灵参和它的丹方、一截藏着功法的肋骨——**全是以后的东西**。所以本章的账在物品不在灵石：24 种 story 本章一种都卖不掉（殿内无店），第 13–14 章按需取用（笔记 O.2）。


