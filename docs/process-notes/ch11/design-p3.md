<!-- 第 11 章「天星城·结丹」施工图 · 第 10–18 节（写手三，2026-10-12）。id、旗标、章号、数字全部照 notes/ch11/research.md；节号节名照 docs/ch10-design.md。 -->

## 10. 引擎前置

### 10.1 清单

| # | 项 | 必须 / 可选 | 谁做 | 说明 |
| --- | --- | --- | --- | --- |
| E1 | **开局接法**：patch `scripts/ch10/zhifadui.lua` 末尾——在 `flag.set("ch10.done")` 之后补一行 `teleport("ch11_fangshi", "chengmen")`（此前一个字不动、不删）；`data/chapters.json` 加 `{"number": 11, ..., "done_flag": "ch11.done"}`；`ui.json` 加 `ui.chapter.11.numeral`「第十一章」、`.title`「结丹」 | **必须** | 内容路（patch 一行）＋ 协调者（两份数据） | 研究笔记 E1、E2；与第 9→10 先例一致；卡上不写「天星城」 |
| E2 | `tests/fixtures/ch10-end-{first,second}.sav` 按 E1 patch 之后重生成：`mapId` 变 `ch11_fangshi`、站在 `chengmen` 那一格；第 10 章验收 2 末项措辞随之改 | **必须** | 测试路 | 笔记 O-10；照第 10 章 16.2 第 2 条做法 |
| E3 | 规则 25 的表：`CHAPTER_MEANS[11]` ＝ 拳、火、土、水、金、木、暗器、剑；`BATTLE_EXTRA_MEANS` 四场各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金；`MEANS_LAST_CHAPTER = 11`；**selftest 探针挪到 `wu_chou` ＋ `b12_neidian_duoding`**，措辞改「第 12 章范围探针实际引用乌丑」 | **必须** | 协调者（集成时） | 笔记 E19、D 表「规则 25」「selftest 探针」；探针不挪则本章写实 `b11` 后 selftest 红 |
| E4 | `tests/LexiconTests.cpp` 表一补词（12.1） | **必须** | 协调者 | 放在第 10 章集成之后 |
| E5 | `tests/ObjectiveTests.cpp` 的 `kScriptTransfers` 登 N.4 七条 teleport；`tests/NpcPresenceTests.cpp` 的 `kOffChainFlags` 登 `ch11.jiguan`、`ch11.dan_fu`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen` | **必须** | 测试路 | 笔记 E21；规则 30 用例 |
| E6 | 数据：角色新建 35、就地改写 1（`siji_haishou` 的 `weaknesses`）、一个字不动 3（`qu_hun_shadan`、`wu_chou`、`zi_ling`）；编成 1 改写（`b11_ningcuidao_youyao`）＋ 3 新；配方 1 改写（`recipe_qingzhu_fengyun_jian`）；物品新建 13；商店 4；任务 2；路径行动 14；地图 7（旧图 0 张修补） | **必须** | 内容路 | 清单在笔记 H、I、L、G、K、N.3；物品描述受第 12 节禁词约束 |
| E7 | **结丹初期锚点标定**：平衡路用 `tests/BattleHand.h` 量四场之后，把 `src/core/rules/Realm.h` 注释第 175 行「筑基后期与结丹档仍待对应章节标定」改成「第 11 章标定」；气血 700、攻防 62 / 46 不动 | **必须** | 平衡路 | 笔记 C.2；本章就是标定结丹初期的那一章 |
| E8 | 通关测试 `Ch11Walkthrough`：起点 E2 重生成的两份、终点写 `tests/fixtures/ch11-end-{first,second}.sav`（v9，本章不加存档字段）；`Ch11TriggerModeTests` / `Ch11BattleDataTests` / `Ch11LedgerTests` / `Ch11SliceTests` / `Ch11EngineTests`（O-5 的六次 3600 日用例） | **必须** | 测试路（**另一个代理**） | 第 15 节 |
| E9 | `data/visual/maps.json` 七张图的主题 / 时辰 / 背景（只用已有组合）、`data/visual/battles.json` 四场背景（全 `cliff_top`）；`data/text/battles.json` 里旧占位 `ch11.battle.*` 开场白按 I 表重写 | **必须** | 内容路 | 笔记 G 表附注 4、I 表附注 |
| E10 | `data/flags.json` 登第 11 节全部旗标 | **必须** | 协调者 | 施工方不碰 |
| E11 | 回写：大纲第 11 章人物删「文樯」「木龙真人」、加注「木龙碑 ch409 只在传闻一句」；人物总表 `fan_furen` summary、`zhao_meng_zhanglao` 加注、极阴祖师首见 ch382、天星双圣加注「传闻」；`material_jinleizhu` note 改「带根一节」 | **必须** | 协调者 | 笔记 B1–B3、B5、B6、B10、B13；E30 |
| O1 | 接 `breakthroughPillBonus`（降尘丹 / 雪灵水加冲关把握） | 可选 | — | **不做**：它仍是返回 0 的桩（`CultivationScene.cpp` 第 363 行）；本章不走面板冲结丹，接了也没人读（E4） |
| O2 | 抬 `kMaxScriptAdvanceDays`（3650） | 可选 | — | **不做**：六十年 ＝ 6 × `advance_days(3600)`，二十余年 ＝ 2 × 3600 ＋ 补足；Lua 分段调用（E5） |
| O3 | 丰乐拍卖行面板（数据目录 ＋ `io/AuctionLoader` ＋ `game/AuctionScene` ＋ Lua `auction(id)`） | 可选 | — | **不做**：门面 ＋ 路径行动两条；`bidStep` 继续只有规则层；留给第 12 / 13 章再议（E9、B22） |
| O4 | Lua 加炼器熟练命令 | 可选 | — | **不做**：配方 `difficulty 0 / requiredProficiency 0`，面板必成（E16） |
| O5 | 新法术（剑影分光术、丹火、青竹蜂云剑阵） | 可选 | — | **不做**：起点 11 门够打四场；剑只是兵刃（E17） |
| O6 | 遭遇区做诱妖循环 | 可选 | — | **不做**：`validate.py` 第 2716–2731 行遭遇编成掉落只许灵草，妖丹给不了（E15） |
| O7 | 名牌 realm 显示「？」（天眼术看不透） | 可选 | — | **不做**：`jiyin_fushen` 写 `CoreLate` 只为不越上限，文案说「看不透」（O-9） |
| O8 | 「岁月流逝」新画面 | 可选 | — | **不做**：三段蒙太奇只用 `fade` ＋ `talk`（E5） |
| O9 | 青竹小轩真实库存经营 | 可选 | — | **不做**：柜台触发器按日结算（E22） |
| — | 面板冲结丹、组队机制、通缉状态字段、拍卖面板、续租、大衍诀机制、「每月缴灵石」机制 | **不做** | — | 笔记 A4、A15、A32、A67、C.3、E28 |

**查过、本章不需要新做的**：`realm.advance` 跨大境界一步到位（23 → 31，`RealmAdvance` 直接写目标）且顺带抬 `storyCap`（契约 script 7.2）；`realm.cap` 只升不降；设施 `effectiveness` 属性（第 8 章 E1，门禁下限 100）；`field.unlock(id, slots)`；`bottle.spend / drops`、凝液间隔按境界（结丹后 3 日一滴，`Application.cpp` 第 130 行）；配方 `requireFlag`；facility `require_flag`；商店 `sellRate`、空 `entries`、只收 `tradeable`；任务板 `board` → `BoardScene`；`advance_days` 结算灵田、绿液、`restFor`；多波次、蓄势、破绽、`defeat_is_fatal false` 时 `battle()` 返回 `won=false`、`can_escape`；NPC `visible_flag / hidden_flag`；路径行动 `when / until / reveal`；`today()`；`flag.set(name, n)` 存整数日期（第 10 章 `ch10.kaifu_ri` 先例）；章节卡与「未完待续」。

### 10.2 判据的要点

- E1：第 10 章集成版 `zhifadui.lua` 与 patch 后的 diff 只有末尾一行；headless `Application` 演完它，`mapId == ch11_fangshi`、站在 `chengmen` 那一格、`ch10.done == 1`、章节卡队列是「第十章　终」「未完待续」接「第十一章　结丹」。**负向**：把那一行挪到 `flag.set` 之前 → 第 10 章验收 20 红。
- E3：`validate.py` 对四张编成「至少一样破绽是本章我方手段打得到的」全过（① 土 ② 金 ③ 剑 ④ 水）；selftest 探针用例把 `wu_chou.json` 临时写成 `weaknesses ["毒"]` 时报、改回不报；**负向**：`MEANS_LAST_CHAPTER` 留 10 → `b11_*` 四张全红。
- E5：`kScriptTransfers` 七条逐条跑脚本到那一行，`mapId` 等于落点图；`kOffChainFlags` 七条都不在 `data/objectives/ch11.json` 的 `done_flag` 序列里、却都在 `data/flags.json` 里。
- E7：`BattleHand` 四场各跑两侧起点，① 意图三到五回合、② 三回合内逃得出、③ 四回合内清场、④ 友军两技能各至少放出一次即胜；锚点注释改字之后 `Realm.h` 其余一行不动（diff 核）。
- O2：`Ch11EngineTests` 用 `ch11.sangong` 刚置位的存档连调六次 `advance_days(3600)`，`day` 恰加 21600、`field_tianxing_39` 六槽无异常、小瓶 `drops == capacity`、`restFor` 无溢出（O-5）。

---

## 11. 旗标清单（交协调者登记；施工方不碰 `data/flags.json`）

| 旗标 | 值 | 谁置 | 谁读 |
| --- | --- | --- | --- |
| `ch11.chudao` | 1 | chudao.lua | 星宫修士出场；n02 guard；K1 `when` |
| `ch11.zhihuan` | 1 红环（扣 8 中阶）/ 2 蓝环 | zhihuan.lua | 星宫修士撤场、鲁二出场；n03 guard；K2 `when`；K1 `until`；n04 执事的一句 |
| `ch11.luer` | 1 雇 / 2 不雇 | luer.lua | 星宫执事出场；n04 guard；n04 执事多说两句（＝2） |
| `ch11.zufu` | 1 | zufu.lua | 执事撤场；n05 guard |
| `ch11.anju` | 1 | anju.lua | 鲁二撤场；静室、丹房、药园 `require_flag`；n06 guard；K2 `until` |
| `ch11.kuilei` | 1 | kuilei.lua | 白门阁掌柜、丰乐伙计、储物袋铺掌柜出场；n07 guard；K3、K4 `when` |
| `ch11.ziliao` | 1 | ziliao.lua | n08 guard；K5 `when` |
| `ch11.nichang` | 1 | nichang.lua | 坊市杂货 `require_flag`；洞府北口 `require_flag`；星空殿四类 NPC 出场；n09 guard；K10 `when` |
| `ch11.chuanshong` | 1 | xingkong.lua（首次） | 妇人一伙、枣红老者撤场；凝翠岛守阵、杂货、范小子出场；n10 guard；K10 `until`；n09 「此后每次 7 中阶」那一支 |
| `ch11.ningcui` | 1 | ningcui.lua | 郝远天出场；东口 `require_flag`；n11 guard；K6–K9 `when`；Z1 `accept` |
| `ch11.buzhen` | 1 | buzhen.lua | `trigger_youyao` guard；`trigger_huicheng` guard（同格换触发器） |
| `ch11.lieyao` | 整数（累加） | youyao.lua | youyao.lua（`== 4` 且 `jiguan == 0` 转 ②）；n14 guard（`>= 3`） |
| `ch11.jiguan` | 1 | youyao.lua（胜 / 逃 / 败都置） | youyao.lua（不再转 ②） |
| `ch11.huicheng` | 1 | huicheng.lua | 凝翠岛三商 NPC、郝远天、储物袋铺掌柜撤场；xingkong.lua「本章不再出海」那一支；n15 guard；K4、K6–K9 `until`；Z1 `fail` |
| `ch11.sangong` | 1 | liandan.lua | n16 guard |
| `ch11.dan_fu` | 0–7（位掩码：降尘丹 1、雪灵水 2） | biguan.lua | biguan.lua 旁白三版 |
| `ch11.jiedan` | 1 | biguan.lua（`realm.advance` 之后） | n17 guard |
| `ch11.chuguan` | 1 | chuguan.lua | n18 guard；E27 外人改与韩立说话 |
| `ch11.zhaolan` | 1 | zhaolan.lua | 结丹邻居 _a / _b、莲儿出场；n19 guard；K12 `when`；Z2 `accept` |
| `ch11.lianer` | 1 | lianer.lua | 莲儿撤场；茶馆门 `require_flag`；范夫人、赵长老（雅间）出场；n20 guard |
| `ch11.chaguan` | 1 | chaguan.lua | 范夫人、赵长老（雅间）撤场；n21 guard；K11 `when` |
| `ch11.zhuzhan` | 1 | zhuzhan.lua | 礁岛八名 NPC 出场；n22 guard；K11 `until` |
| `ch11.guiju` | 1 | guiju.lua（胜） | 隐煞门门主、乌丑出场；`trigger_wuchou` guard |
| `ch11.jiyin` | 1 | wuchou.lua | 极阴祖师出场；`trigger_daoge` guard |
| `ch11.yaoshi` | 1 | daoge.lua | 礁岛全部 NPC 撤场；紫灵、范夫人、卓如婷（雅间）出场；n25 guard |
| `ch11.zhanglao` | 1 | zhanglao.lua | 雅间三人撤场；n26 guard |
| `ch11.zhu` | 0–3（1 白 2 蓝 3 金） | peizhu.lua | peizhu.lua |
| `ch11.jinlei` | 1 | peizhu.lua（`zhu == 3`） | 李氏店主出场；n27 guard；peizhu.lua 换 n29 分支 |
| `ch11.kaidian` | 1 | kaidian.lua | 李氏店主撤场；何仙师、邻店店主出场；店门 `require_flag`；n28 guard；K13 `when` |
| `ch11.kaidian_ri` | 整数日期（`today()`） | kaidian.lua（只此一处写） | peizhu.lua（n29 补足到 7200 日） |
| `ch11.jiesuan_ri` | 整数日期 | kaidian.lua（初值）、jiesuan.lua | jiesuan.lua |
| `ch11.jiesuan` | 整数（结算次数） | jiesuan.lua | jiesuan.lua（首次带旁白） |
| `ch11.zhu_gen` | 0–6 | peizhu.lua | peizhu.lua（`== 6` 收店） |
| `ch11.liugen` | 1 | peizhu.lua | 店门、柜台、何仙师、邻店店主撤场（`hidden_flag`）；炼器台 `require_flag`；配方 `requireFlag`；K13 `until` |
| `ch11.lianjian` | 1 | chujian.lua | 金青出场；n31 guard；K14 `when` |
| `ch11.done` | 1 | zhuzhong.lua | 章末；K 表全部 `until`；第 12 章入口 |
| `ch11.z1` | 0 / 1 接单 / 2 交付 | 收购行脚本（Z1） | Z1 `complete`；任务板条目 |
| `ch11.z2` | 0 / 1 | linju_z2.lua | Z2 `complete`；再按只一句 |
| `ch11.path.<id>` | 1 | 路径行动引擎 | 只由路径行动置（**等 `data/pathactions/ch11.json` 落地后登记**） |

不新增 `story.*`。**目标链之外的在场 / 分支旗标**：`ch11.jiguan`、`ch11.dan_fu`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`（登 `kOffChainFlags`，E5）。**两面日期旗标** `ch11.kaidian_ri`、`ch11.jiesuan_ri` 与 `ch10.kaifu_ri` 同一说明：「整数日期，不是开关；值约一万六千到四万」——`kaidian_ri` 只由 `kaidian.lua` 写、`peizhu.lua` 读；`jiesuan_ri` 由 `kaidian.lua` 给初值、`jiesuan.lua` 读写。开关一律 `flag.set(name)`，整数 `flag.set(name, n)`；`ch11.lieyao`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.jiesuan` 的累加都是「`flag.get` ＋ 1 再 `set`」，脚本里不写别的算法。

---

## 12. 禁词与首见章号

### 12.1 交协调者：`tests/LexiconTests.cpp` 表一要补的词

逐个 `novel.py find` 查过首见（2026-10-12，笔记 M 表）。第 10 章 12.1 已列进表一的（天星宫、妖海、摘星大会、天星双圣、妖兽岛、星空殿、丰乐拍卖行、白门阁、凝翠岛、霓裳草、诱妖草、妙音门、范夫人、紫灵仙子、虚天残图、青竹小轩，以及第 8、9 章的天星城、星宫、内星海、外星海、天雷竹、虚天殿、金雷竹）**不重列**；协调者补表之前按落地的文案再扫一遍 `data/text/ch11*.json`。

**首见落在 ch389-415（第 1–10 章禁、本章放行）**：

| 首见 | 词 |
| --- | --- |
| ch389-393 | 鲁二、红色指环、蓝色指环（390）；圣山、圣宫（391）；传送符（393） |
| ch394-398 | 丰乐盟、碧磷鱼妖、郝远天（394）；红瑚岛、幻形天罗阵（395）；千目妖、鸡冠蛟（396）；白玉蜘蛛（398） |
| ch399-404 | 范静梅、莲儿、清河茶馆、天晶木、三级傀儡（399）；赵长老（400）；三大神木、范左使（401）；卓如婷、右使、汪门主（402）；赤火老怪、元龟岛、葵水魔功（403）；隐煞门、符长老（404） |
| ch405-415 | 附身大法（405）；天都尸火、禁神牌（406）；煞丹分身（407）；妖尸（408）；木龙碑、隆兴客栈（409）；膏玉、名义长老（410）；白雷竹、金雷竹、辟邪之雷（411）；李氏杂货、何仙师（412）；镇店之宝（413）；金青、白珍珠、古修士遗址（415） |

**首见在 ch415 之后（第 1–11 章都禁）**——第 10 章 12.1 第二张表已列的（辟邪神雷 424、玄骨 428、蛮胡子 432、九曲灵参 433、虚天鼎 434、补天丹、六道极圣 435、啼魂 437、青阳门 464、乾蓝冰珠 494、逆星盟、万三谷 500 及以后）不重列；本章新查的：文思月（430）、元瑶再见（445，「元瑶」本章 0 处）、十八天都尸（409 但韩立不在场，判据③）、元磁神光（408，同前）。

**故意不进表的**（常用字组合）：星宫修士 / 星宫执事（「星宫」进表就够）；红环 / 蓝环（泛称，进表的是「红色指环」「蓝色指环」）；妖丹、五级 / 六级（泛用）；四色（「四色」ch266 早有，金丹四色由 12.2 管）；生死关（ch82 早有）；剑影分光术（ch217 早有）；名义长老、镇店之宝（句式）；茶馆（泛用，进表的是「清河茶馆」）；紫灵（会误伤「紫灵芝」）；左使 / 右使（「范左使」「卓如婷」进表就够）；传音符（ch152 前已有）；白衣修士、妖艳妇人、枣红脸老者、守阵修士、邻居（名牌，不是专名）。
**本章解禁的**：上面第一张表的全部，外加第 8–10 章表里的天星城、天星宫、星宫、内星海、外星海、妖海、天星双圣、摘星大会、妖兽岛、星空殿、丰乐拍卖行、白门阁、凝翠岛、霓裳草、诱妖草、妙音门、范夫人、紫灵仙子、虚天残图、虚天殿、天雷竹、金雷竹、青竹小轩。**仍禁**：第二张表、乾蓝冰焰（494）、银月（615）、紫罗极火（731）、明珠轩（513）、交换会（535）、黄龙、猎妖令，外加聂盈、掌天瓶。

### 12.2 规则放行、本章另有专项断言的

**断言的写法**同第 8–10 章：把目标链（F 表 31 步 `done_flag` 次序）当成场景次序，给每个文案 key 的场景前缀编号——**前缀取 J 表「谁置」那一列的脚本名**（`ch11.chudao.*`、`ch11.liandan.*`、`ch11.jiesuan.*`……），不取挂点名；Z1 `ch11.z1.*` 编在 `ningcui` 之后、Z2 `ch11.z2.*` 编在 `zhaolan` 之后；路径行动文案 `ch11.path.<id>.*` 按它 `when` 里最晚的那面链上旗标编号；物品描述按 `give` 它的脚本编号；战斗开场白按调用它的脚本编号；地图名按第一次 teleport 进去的脚本编号；门的拒绝语按那扇门所在图的编号。「X 之前不含」＝编号小于 X 的前缀下 0 处。本章没有语言两版（K 表附注 3）。

| 词 | 首见 | 为什么规则管不住 | 本章的断言 |
| --- | --- | --- | --- |
| 天星城、天星宫、星宫、内星海、外星海、妖海、天星双圣、摘星大会、妖兽岛 | ch389 | 表一；章内先后 | 只许 `chudao.*` 起；章节卡 `ui.chapter.11.*` 两条 0 处（E2）；「妖海」只在 `chudao.*` 读典籍那一句与 K1 |
| 红色指环、蓝色指环、「每月缴灵石」 | ch390 | 章内先后 | `chudao.*` 不含；「每月」只在 K1 与 `zhihuan.*`，机制 0 处（B7） |
| 鲁二 | ch390 | 章内先后 | `chudao.*`、`zhihuan.*` 不含（他在 `luer.*` 里自报） |
| 圣山、圣宫、八十一层、三十九层 | ch391 | 章内先后 | `chudao.*`、`zhihuan.*` 不含；「三十九层」在 `zufu.*` 之前只许 K2 提「前五十层」 |
| 白门阁、丰乐拍卖行、储物袋铺 | ch393 | 章内先后 | `kuilei.*` 之前不含 |
| 星空殿、凝翠岛、传送符 | ch393 | 章内先后 | `nichang.*` 之前不含；洞府北口拒绝语 `ch11.dongfu.xingkong_deny` 不含「星空殿」「传送」 |
| 丰乐盟、碧磷鱼妖、郝远天、范小子 | ch394 | 章内先后 | `ningcui.*` 之前不含；`hao_yuantian` 的 `name` 就是「郝远天」（名牌跟 role） |
| 红瑚岛 | ch395 | 章内先后 | `ningcui.*` 之前不含（东口拒绝语也不含） |
| 霓裳草、诱妖草 | ch395 | 章内先后 | `ziliao.*` 之前不含；`herb_nichang_cao` 的描述按 `nichang` 编号 |
| 幻形天罗阵、天风狂烈阵、颠倒五行阵 | ch395 / 373 / 226 | 规则放行 | 只在 `buzhen.*` 一处旁白（不发物品，A35）；`youyao.*` 里只说「阵」 |
| 千目妖 | ch396 | 章内先后 | `ziliao.*` 之前不含（图册里才有它）；K7 可提 |
| 鸡冠蛟、六级 | ch396 | 章内先后 | `jiguan.*` 与 ② 开场白之外 0 处——K10 枣红老者只说「六级蛟类」，不出「鸡冠蛟」三字 |
| 生死关、散功、筑基初期 | ch82 / — | 规则放行 | `liandan.*` 之前不含；`realm.demote` 0 处（A42） |
| 四色（金丹） | ch398 | 规则放行（「四色」ch266） | 只在 `biguan.*`、`chuguan.*` |
| 二转年数（三十多年 / 十余年） | ch398 | 原著自相矛盾（B16） | 「三十多年」「十余年」作二转年数讲 0 处；只写「数十年」 |
| 剑影分光术 | ch217 | 规则放行 | 至多 `chuguan.*` 一处，只提名（A46） |
| 白玉蜘蛛 | ch398 | 回忆（判据③） | 全章 0 处（本作只有血玉蜘蛛） |
| 妙音门、范静梅、莲儿、清河茶馆 | ch399 | 章内先后 | `lianer.*` 里范静梅自报之前不含；`zhaolan.*` 传音符那一段不含 |
| 三级傀儡、一贫如洗 | ch399 | 章内先后 | `zhaolan.*` 起；「一贫如洗」只此一处 |
| 天晶木 | ch399 | 不进文案 | 全章 0 处（辅料只写「辅料」） |
| 赵长老、范夫人（称谓）、范左使 | ch400 / 401 | 章内先后 | `chaguan.*` 之前不含；`lianer.*` 只叫「范静梅」 |
| 天雷竹、三大神木 | ch401 | 章内先后 | `chaguan.*` 里范夫人说出之前不含；`material_tianleizhu` 描述按 `zhanglao` 编号 |
| 卓如婷、右使、汪门主、劫货 | ch402 | 章内先后 | `zhuzhan.*` 之前不含（`chaguan.*` 末尾「劫货之事」可提劫货，不提汪门主） |
| 天音门 | ch402 | 原著笔误（B20） | 全章 0 处 |
| 赤火老怪、元龟岛、葵水魔功、紫灵仙子、孟道友 | ch402-403 | 章内先后 | `zhuzhan.*` 之前不含 |
| 隐煞门、符长老、煞妖 | ch404 | 章内先后 | `zhuzhan.*` 之前不含（礁岛地图名按 `zhuzhan` 编号） |
| 孟长老、赵孟两位长老 | ch405 / 410 | 原著称谓不一（B6） | 全章 0 处；叛的只写「赵长老」「符长老」 |
| 乌丑、极阴祖师、极阴岛 | ch382 | 第 10 章已解禁；章内先后 | `wuchou.*` 之前不含；极阴岛只作乌丑来历 |
| 附身大法、虚天残图、虚天殿、三百年 | ch405 | 第 12 章钩子 | 「虚天残图」「虚天殿」合计恰在 `wuchou.*` 那一句里、其余 0 处（笔记 0 节、A56）；K 表与物品描述 0 处 |
| 天都尸火、禁神牌、封某 | ch406 | 章内先后 | `wuchou.*` 起；「封某」只在旁白一句、不建 role |
| 煞丹分身 | ch407 | 章内先后 | 只在 `daoge.*` 一句（A58） |
| 妖尸、天都尸 | ch408 | 章内先后 | `daoge.*` 与 ④ 开场白之外 0 处；「十八天都尸」0 处 |
| 元磁神光 | ch408 | 暗场（判据③） | 全章 0 处 |
| 隆兴客栈 | ch409 | 改编（B23） | 全章 0 处；图鉴注改编 |
| 木龙碑、汪凝 | ch409 / 403 | 传闻 / 自称 | 各至多 `zhanglao.*` 一处 |
| 膏玉、名义长老、年俸三百、白雷竹、蓝雷竹、金雷竹、辟邪之雷 | ch410-411 | 章内先后 | `zhanglao.*` 之前不含；「年俸」只在台词、结算 0 处（N.1） |
| 倾家荡产 | ch411 | 质感 | 至多 `peizhu.*` 一处 |
| 青竹小轩、李氏杂货、何仙师、入世 | ch412 | 章内先后 | `kaidian.*` 之前不含；店门拒绝语不含「青竹小轩」 |
| 镇店之宝、第九个年头 | ch413 | 章内先后 | `jiesuan.*` 起 |
| 七十二、黄道吉日、火柱 | ch413-414 | 章内先后 | `lianjian.*` 起；「七十二」在 `zhanglao.*`、`peizhu.*` 里只许「一套」 |
| 金青、白珍珠、古修士遗址、六连殿（叛乱）、走火入魔 | ch415 | 章内先后 | `zhuzhong.*` 之前不含；「走火入魔」只到谣言（A72） |
| 文樯、木龙真人、天星双圣（人物） | B1、B2、B10 | 不出场 | 文樯、木龙真人 0 处；天星双圣只在典籍、传音符、`zhuzhong.*` 谣言 |
| 明珠轩、交换会、文思月、元瑶、银月、紫罗极火、黄龙、猎妖令、掌天瓶 | — | 表一 / 后章 | **全章 0 处**（接单口径是「悬赏」「订单」） |

---

## 13. 文案要求

与前十章同一标准：对 ch389-415 最长公共片段 ≤ 6 字、对全本 ≤ 7 字、≥ 10 字 0 条（汉字剥离、逐条精确最长公共片段、带正负对照）；换词不算重写；key 命名 `ch11.<场景>.<用途>`（场景＝脚本名）；台词 18-60 字为主、不超 80，选项 5-20 字；旁白每段不超 4 句。

**转写高风险点**：

1. **ch389 读典籍**：内外星海、星宫、双圣、猎妖流程、妖丹价——事实照 A1–A8；**典籍原文的讲法与次序全换**，做成他合上玉简之后自己理出来的几条，不是玉简念给玩家听。
2. **ch390 入城**：红蓝环的规矩、八块中阶灵石、滴血认主、结丹免费——事实；守门修士的穿戴、他对城高城阔的那一番比方、鲁二自荐时的神态与开价的说法，自写。
3. **ch391 租府**：三层各几座、两千七百、百年——事实；执事的口吻、三十九层那座洞府的内景描写，不照搬。
4. **ch393 星空殿**：七百一次、七人上限、拼阵付二百与五百——事实；殿的材质与那三个字的颜色（A25）只许提「白石大殿」，妇人一伙的衣饰与神情、执事点数的动作，自写。
5. **ch396 千目妖**：五级、滚圆带刺、数十只绿眼、一顿饭取丹——事实；它入阵的过程与取丹的手法、第二波海兽是本作改编（B11）——**开场白里不许出现它破阵的任何一句**。
6. **ch397-398 结丹**：六十年、黑云闪电、百余里漩涡、全城可感——事实；**天象只许一句旁白**，云电漩涡的整套写法与邻居议论的原话不用；「四色」可用，金丹的大小不写。
7. **ch399 莲儿**：炼气期少女拦路、媚术被拒、范静梅赔罪——事实；少女的年纪与相貌、媚术的手法、范静梅的衣着，一样不用。
8. **ch400-401 茶馆**：不饮茶、压价一成再加半成、一小堆中阶灵石、天雷竹线索——事实；赵长老的相貌、狐媚术试探的那几句、范夫人的笑，自写。
9. **ch405-407 乌丑与极阴**：劫货嫁祸、索残图、三百年一句、附身只发挥三分之一、禁神牌与三分之一元神、封某被焚、赤火倒戈、四结丹是煞丹分身——事实；**韩立退到最后、不交元神，是他唯一的动作**；乌丑的相貌、附身时的光与声、禁神牌的样子、众人的反应——只写「色变」一类。
10. **ch408 妖尸**：不怕法宝、自愈、瞬移、蛛网与紫火困住、遁走、入地一月——事实；它的形貌、十八之数、极阴的来历——0 处；胜利写「困住」。
11. **ch410-411 紫灵真容**：真容、识破化身、两节竹、名义长老与年俸、拒妾拒入门、匕首试竹——事实；她的容貌、地点（隆兴客栈）、三女议事——0 处；B23 改在茶馆雅间。
12. **ch412-415 开店与炼剑**：改名、只卖符与药材、何仙师、六根、三个月炼液、布阵半月、七十二根火柱、两个月出关——事实；店里的陈设、伙计、客人、火柱的高度、剑胚的颜色——自写或不写。
13. **名场面台词不许复刻**：他读完玉简的那一句感慨、鲁二开价与他应「满意给五枚」的那一来一回、执事提「结丹前辈免费」的那一句、枣红老者邀联手与曲魂回绝的那一句、郝远天自报名气的那一句、他第一次看见千目妖时的那个念头、邻居「又有人结丹了」的原话、范静梅赔罪的那一句、范夫人压价的说法、乌丑索残图的那一句、极阴说「三分之一元神」的那一句、紫灵说「以此竹加三百灵石」的那一句、金青送珠时的那一句——**意思保留，说法自己写**；F 表、13.1 为说明意图写的转述同样不许照抄进文案。
14. **暗场口径**：范赵密谋、极阴与赤火分赃、三女议事、乌丑怎么处置孙姓中年人、元磁神光——**一个字都不许替原著补**；玩家知道的只有 `daoge.*` 那一句「四结丹是煞丹分身」。
15. **死亡只写倒下**（E25）：封某「惨叫后再无声息」；煞妖「化为飞灰」可用（原文如此）；孙姓中年人「被困住」；妖尸「被困住」——不写杀死、不写灰烬（人）。
16. **曲魂代言**（E27）：n02–n10 外人先跟曲魂说话，第一次交代一句；n17 起外人直接跟韩立说话，**不再解释为什么**；曲魂的台词是韩立的意思。
17. **不写志向、不写去向**：章末停在金青递珠；古修士遗址只有金青一句；第 12 章的虚天殿只有 `wuchou.*` 那一句；「六级妖丹才有用」只在 `zhuzhong.*` 一句（O）。
18. **数字不进台词**：静室 125、`advance_days` 的天数、收购行 0.8、结算 15 低阶 / 30 日——文案里只写「快了四分之一还多」「六十年」「二十余年」「每月有些进项」这类原著口径的话。

### 13.1 每段框架件清单（写手与复验共用的判据）

**判据**（沿用第 5 章 13.1）：

- **剧情事实**＝框架件＝删了戏就塌的东西——事实、硬约束、谁在场、落点。下表逐段列全；它们与原著对得上是应当的。专名不算质感。
- **质感边界**＝表外的一切——人数、岁数、长相、手势、道具、次要人物的反应、次要的一句话、谁先谁后——**一律自出**。原著该场的质感项不许出现，**换了词也不行**；把原著的一个动作反过来写也算留着。「质感边界」一栏点名的是最容易被搬进来的那几样，不是全部，**只点名、不复述**。
- 设计语气卡点名的意象可以留，每段至多两处，写法要换。
- 复验：逐段做「原著质感项 → 现文」的并排表，剩下的对应项必须全在「剧情事实」里。

| 段 | 原著 | 剧情事实（删了就塌） | 质感边界（不许出现，换词也不行） | 在场 | 落点 |
| --- | --- | --- | --- | --- | --- |
| n01 初到 | 389-390 | ①一月赶路到天星城；②典籍：内星海 / 外星海（妖海）、星宫管辖、双圣近千年不露面、城规三条、猎妖流程、五级妖丹近千；③远望巨城；④通缉只一句（E28）；⑤曲魂代言的交代一句 | 典籍玉简的售价与铺子；他对城墙高阔的比方；城中人流的形容；双圣传说的细节；读玉简时的神情 | 韩立、曲魂 | `ch11.chudao` |
| n02 指环 | 390 | ①守门修士先与曲魂说话；②曲魂结丹、红环免费；③韩立：红环八中阶 / 蓝环三低阶、灵光七天；④滴血认主；⑤城内无禁空一句（A13） | 守门修士的穿戴与口吻；排队入城的其他修士；指环的样式与光色；滴血的动作 | 韩立、曲魂、星宫修士 | `ch11.zhihuan` |
| n03 向导 | 390 | ①凡人鲁二自荐；②开价两枚、韩立应「满意给五枚」只作二选一（雇 / 不雇）；③雇则他补坊市与租赁门道，不雇则 n04 执事多说 | 鲁二的身量与堆笑；他自报身世的话；街市吃食与叫卖 | 韩立、曲魂、鲁二 | `ch11.luer` |
| n04 租府 | 391 | ①圣山八十一层、前五十层对外；②三十一 / 三十九 / 四十七层各几座；③只能选三十九层；④两千七百、百年计；⑤有几块扣几块、零块记账（改编）；⑥禁制令牌 | 山脚楼阁的陈设；执事的相貌与冷淡；上山的方式与沿途景致；洞府内景 | 韩立、曲魂、星宫执事、（鲁二） | `ch11.zufu`；teleport 洞府 |
| n05 安顿 | 391-392 | ①布阵、镶玉密室、虫室放噬金虫；②过三十层修炼快四分之一还多；③药园六槽；④鲁二退场 | 密室的数目与用料；噬金虫「数百只」之外的习性描写；他盘坐试灵气的那一刻 | 韩立、曲魂 | `ch11.anju` |
| n06 傀儡 | 392 | ①半年育虫试验（死卵、饲灵丸）；②闭关两年整造二百余只兽形傀儡；③曲魂炼化混元钵一句 | 傀儡的形制与材质；育虫失败的细节；两年里的日常 | 韩立、曲魂 | `ch11.kuilei` |
| n07 资料 | 393 | ①白门阁书铺；②百余灵石复制全部妖兽资料；③赊账一支（改编） | 书铺的陈设与伙计；复制的手法；他翻阅的神情 | 韩立、曲魂、白门阁掌柜 | `ch11.ziliao` |
| n08 霓裳草 | 393-395 | ①读资料一个月悟出霓裳草（十三叶、百年一叶、展叶六七日、诱香）；②药园一滴试验，血玉蜘蛛扑草；③又过半年 | 草的形状与颜色；蜘蛛扑草的动作；他推演的过程 | 韩立 | `ch11.nichang` |
| n09 星空殿 | 393 | ①第五十层白石大殿、成排传送阵、白衣执事；②一次七百、最多七人；③首次拼阵付二百、妇人一伙付五百；④传送符一句；⑤此后每次七中阶、不足搭阵（改编） | 殿名三字的写法与颜色；妇人一伙的衣饰与神情；执事点数的动作；传送时的光 | 韩立、曲魂、星空殿执事、妖艳妇人一伙、枣红老者 | `ch11.chuanshong`；teleport 凝翠岛 |
| n10 凝翠岛 | 393-394 | ①传送石屋有守阵修士；②小街四间铺；③枣红老者邀联手，曲魂回绝；④郝远天卖碧磷鱼妖尸给范小子、招揽被婉拒；⑤精细海图一中阶（可不买）；⑥外星海规矩一句（A32） | 石屋与小街的建材（A28）；妖尸的样子；郝远天自报名气的那一句；范小子验货的动作 | 韩立、曲魂、守阵修士、枣红老者、郝远天、范小子、杂货掌柜 | `ch11.ningcui` |
| n11 布阵 | 395 | ①两日巡岛；②三处必经路各一座大阵；③环心小阵；④不发物品（A35） | 三阵的样式与光；岛上的珊瑚与海鸟；他布阵的手法 | 韩立、曲魂 | `ch11.buzhen` |
| n12 诱妖 | 395-396 | ①要草 ≥1、绿液 ≥1；②每次隔三日；③千目妖入阵（五级）；④第二波海兽（改编）；⑤一顿饭取丹 | 草展叶的过程；妖兽循香而来的写法；取丹的手法；傀儡群射的场面 | 韩立、曲魂 | `ch11.lieyao` ＋1；① |
| n13 鸡冠蛟 | 396 | ①草到四百年份引来六级；②可逃、不致命；③此后只催到三百年 | 它的形貌与声；他封草香的动作；它徘徊离去的写法 | 韩立、曲魂 | `ch11.jiguan`；② |
| n14 回城 | 396-397 | ①收手 / 再猎二选一；②五年换岛、数百颗五级妖丹（只在旁白）；③傀儡剩二三十只；④颠倒五行阵弃于七级妖兽；⑤珍稀妖兽材料 | 换过哪几座岛；七级妖兽的样子；傀儡损耗的过程；数十万灵石的算法 | 韩立、曲魂 | `ch11.huicheng`；teleport 洞府 |
| n15 散功 | 397 | ①两年炼丹；②零散卖材料给小商家；③霓裳草移到药园珊瑚礁；④散功半月到筑基初期（旁白）；⑤噬金虫吞草 | 炼了哪些丹；散功的痛感写法；小商家是谁 | 韩立、曲魂 | `ch11.sangong` |
| n16 闭关 | 397-398 | ①降尘丹五、雪灵水二有则服（三版）；②生死关、辟谷；③六十年三段蒙太奇；④噬金虫互噬、产卵一句；⑤天象一句、全城可感；⑥结丹初期 | 云电漩涡的整套写法；邻居议论的原话；金丹的大小；他闭关时的心境独白 | 韩立、（曲魂隔门） | `ch11.jiedan`；结丹初期 |
| n17 出关 | 398 | ①容貌仍二十许；②寿元五六百年、丹火一句；③剑诀第七层得剑影分光术（只提名）；④噬金虫数万、壳现金斑；⑤血玉蜘蛛四级顶峰收入灵兽袋；⑥曲魂煞丹早成、年数只写「数年」 | 金斑的样子；蜘蛛的尺寸；他照镜一类的动作；二转年数 | 韩立、曲魂 | `ch11.chuguan` |
| n18 招揽 | 399 | ①七八道传音符：恭贺与招揽，一律回绝；②三级傀儡一击相当筑基中期；③辅料贵到一贫如洗；④猿形傀儡一百余 | 传音符的颜色；招揽者是谁；傀儡的材质与颜色 | 韩立、曲魂、结丹邻居 | `ch11.zhaolan` |
| n19 莲儿 | 399 | ①炼气期少女拦路、媚术被拒；②范静梅现身赔罪；③请去清河茶馆 | 少女的年纪与相貌；媚术的手法；范静梅的衣着与笑 | 韩立、曲魂、莲儿、范静梅 | `ch11.lianer` |
| n20 茶馆 | 400-401 | ①范夫人、赵长老（结丹初期，天眼可见）；②不饮茶；③材料换一小堆中阶灵石；④压价一成再加半成只在台词；⑤狐媚术试探、曲魂挡赵长老；⑥天雷竹为三大神木、一小节下落；⑦劫货之事一句 | 雅间的陈设；赵长老的相貌；范夫人的笑与手势；茶的名目 | 韩立、曲魂、范夫人、赵长老 | `ch11.chaguan` |
| n21 助拳 | 402-403 | ①妙音门底细（左右双使、客卿）；②汪门主战死、押货遇劫；③酬劳＝天雷竹＋另挑一件；④半月赶制傀儡；⑤集合岛十几人、结丹五人；⑥赤火老怪携紫灵仙子；⑦次日清晨突袭 | 集合岛的样子；众人的相貌；赤火老怪的来头之外的描写；紫灵的面纱一类 | 韩立、曲魂、（莲儿带路） | `ch11.zhuzhan`；teleport 礁岛 |
| n22 规矩 | 404 | ①紫灵下令、赤火破阵；②韩立与曲魂圈住四名筑基邪修、化煞妖（编成取 4，B17）；③门主现身质问；④赵长老挑衅反噬、范夫人「中了圈套」 | 阵破的光与声；邪修的惨状；门主的相貌与姓氏 | 韩立、曲魂、礁岛众人、隐煞门门主 | `ch11.guiju`；③ |
| n23 乌丑 | 405-406 | ①乌丑现身、自认劫货嫁祸；②索虚天残图、「三百年的虚天殿快到了」只此一句；③极阴附身、只能发挥三分之一；④禁神牌、三分之一元神；⑤封某被焚（E25）；⑥紫灵问双圣禁令已松；⑦韩立退到最后、不交 | 乌丑的相貌与自称；附身时的光与声；禁神牌的样子；天都尸火的颜色；众人求饶的话 | 韩立、曲魂、礁岛众人、乌丑、极阴祖师 | `ch11.jiyin` |
| n24 倒戈 | 407-408 | ①赤火倒戈困住孙姓中年人；②众人四散；③四结丹是煞丹分身一句；④空间扭曲出妖尸：不怕法宝、自愈、瞬移；⑤蛛网与紫火困住、遁走；⑥入地一月；⑦回府门口收范夫人传音符 | 妖尸的形貌与数目；赤火倒戈的说辞；众人逃散的先后；他入地的手法 | 韩立、曲魂、妖尸、（血玉蜘蛛、猿傀） | `ch11.yaoshi`；④；teleport 洞府 |
| n25 长老 | 409-411 | ①紫灵真容、名牌换「紫灵」；②识破曲魂是化身、与本体同为结丹初期；③妙音门暂迁天星城；④两节竹，带根一节在她手；⑤名义长老、年俸三百、不受令；⑥拒妾、拒入门；⑦匕首试竹；⑧竹两寸、小指粗；⑨腰牌 | 她的容貌；隆兴客栈；三女议事；木龙碑之外的镇派之宝描写；匕首的来历 | 韩立、曲魂、紫灵、范夫人、卓如婷 | `ch11.zhanglao` |
| n26 培竹 | 411 | ①白→蓝→金各一滴；②金雷竹辟邪之雷；③白光小剑化烟一句；④两年凑辅料、倾家荡产、卖掉仅剩妖丹；⑤辅料必给 | 竹变色的过程；小剑的样子；凑辅料跑了哪些铺子 | 韩立、曲魂 | `ch11.zhu`；`ch11.jinlei` |
| n27 开店 | 412 | ①盘下李氏杂货、改名青竹小轩；②只卖符与药材；③易容三十余岁、曲魂扮仆人；④入世心境、炼器与阵法；⑤何仙师炼气三四层 | 铺面的陈设与招牌写法；李氏店主的身世；何仙师的相貌 | 韩立、曲魂、李氏店主、何仙师 | `ch11.kaidian` |
| n28 经营 | 412-413 | ①首次：读绢书、何仙师串门；②第九个年头炼阵旗阵盘；③中级符箓二三十张成一次；④店开二十余年、扮五十余岁；⑤每次结算（E22） | 客人的样子；符箓炼制的过程；镇店之宝的名目之外的描写 | 韩立、曲魂、何仙师、邻店店主 | `ch11.jiesuan` ＋1 |
| n29 六根 | 412-413 | ①三年一根、六根二十年；②幼竹再滴数月；③第六根到万年份即收店；④噬金虫慢、产卵一句；⑤转让店铺 | 收店那一天的场面；何仙师的反应；竹的高度 | 韩立、曲魂 | `ch11.zhu_gen`；`ch11.liugen` |
| n30 炼剑 | 413-415 | ①打坐三日；②六根三个月炼成液；③布阵半月、黄道吉日焚香两日；④面板一次必成（E16）；⑤两月后出关、七十二口入体；⑥二十多年心血 | 七十二根火柱的高度；剑胚的颜色；精血与辅料的先后；出关时的神情 | 韩立、曲魂 | `ch11.lianjian`；面板 |
| n31 竹钟 | 415 | ①噬金虫卵控神一句；②七八个势力一夜叛乱、六连殿在内；③双圣走火入魔——只到谣言；④六级妖丹才有用一句；⑤金青：白珍珠半幅古修士地图、两颗合一、请破阵 | 叛乱的经过；金青的相貌与岁数；珍珠的光；地图的内容 | 韩立、曲魂、金青 | `ch11.done` |
| Z1 订单 | 改编 | ①范小子接单：三颗五级妖丹；②酬中阶十、炼器药二；③`ch11.huicheng` 后过期 | 订单的来头（不说是哪家商盟）；范小子的身世 | 韩立、曲魂、范小子 | `ch11.z1` |
| Z2 邻居 | 399 | ①结丹邻居想要定颜丹；②一颗换三份炼器药；③没丹则一句 | 邻居的相貌与岁数；他要定颜丹的理由 | 韩立、结丹邻居 | `ch11.z2` |

**名牌对照**（名牌跟着 role 走，换名牌是换 role）：紫灵仙子（`zi_ling`，只名牌不上场）→ 紫灵（`zi_ling_zhenrong`，n25 起）；乌丑（`wu_chou`，只说话）→ 极阴祖师（`jiyin_fushen`，n23 附身起，同格换 NPC）；曲魂（身外化身）全章不换；隐煞门门主不写姓；范夫人在坊市雅间、礁岛、回访雅间是三个对象名同一 role。

---

## 14. 时长估算

按幕（笔记 N.2），目标 55 min：

| 桶 | 幕 | 项 | 估 |
| --- | --- | --- | --- |
| A | 一·入城 | n01–n05：三段对话（典籍、指环、租府）、鲁二二选一、一次 teleport | 7 min |
| A | 二·猎妖 | n06–n14：两次传送、凝翠岛小街、布阵、诱妖 4–6 轮含 ① ×4、② ×1、收手二选一 | 18 min |
| A | 三·结丹 | n15–n17：蒙太奇为主（三段六次 `advance_days`）、出关 | 6 min |
| A | 四·妙音门 | n18–n25：③ ④ 两场、茶馆 / 礁岛 / 回访三段长对话 | 15 min |
| A | 五·竹与剑 | n26–n31：培竹循环 8 次（3 阶 ＋ 幼竹 ＋ 6 根合并按键）、店铺两次回访、炼剑面板、金青 | 9 min |
| **A** | | | **≈ 55 min**（目标 55；`tools/audit.py` 落地后重算） |
| C | 可选 | Z1、Z2、路径行动 14 条、收购行卖丹、任务板 | 4-6 min |

四场仗各按 4 min（① 三到五回合、② 三回合内逃或胜、③ 四回合、④ 友军两技能）已含在幕内。**真要压，先压这三处**（都不动框架件）：n21 助拳的旁白（妙音门底细与集合岛两段并成一段，妙音门底细已有 K11 承担）、n25 长老的长对话（两节竹的来历与名义长老的条款合为一轮问答）、n01 初到的典籍旁白（五条压成三条，A3、A8 交给 K1、K5）。诱妖循环玩家若多打几轮、培竹每次「等几日」——那是玩家自己挑的节奏，记在 C 桶。

---

## 15. 验收

每一条都配一条**不经过驱动、直接读数据、判据写死成本文原文**的断言；每一条新检查都要配「故意写坏 → 确实被抓住」的用例。切片一律写明起点存档。

| # | 验收条目 | 直接读的断言 |
| --- | --- | --- |
| 1 | 四道门禁全过（validate / selftest / mapgen / artgen），零新增警告 | 门禁本身 |
| 2 | **一条自动化通关测试**，起点 `tests/fixtures/ch10-end-{first,second}.sav`（**E1 patch 之后重生成的那两份**），落在 `ch11.done`；终点写 `ch11-end-{first,second}.sav`（v9） | `SetUp` 先验 fixture：`ch10.done == 1`、`realm == realmCap == 23`、`formerRealm == 22`、`hp == maxHp == 480`、`mp == maxMp == 320`、`material_lingshi_zhong` 117 / 116、`material_lingshi` 254 / 234、`pill_jiangchen_dan == 5`、`material_xuelingshui == 2`、`pill_zhuji_dan == 13`、`material_yaodan_wuji == 1`、`learnedMagics` 恰 11 门、`party` 恰 `[qu_hun_shadan]`、`bottle.capacity == 6`、`story_shijin_chong == 1`、`story_xueyu_zhizhu == 1`、`story_hunyuan_bo == 1`、`mapId == ch11_fangshi` 且站在 `chengmen` 那一格；终点先验：`realm == realmCap == 31`、`formerRealm == 22`、`maxHp == rules::realmMaxHp(31)`（700）、`maxMp == rules::realmMaxMp(31)`、`learnedMagics` 恰起点那 11 门、`party` 恰 `[qu_hun_shadan]`、`pill_jiangchen_dan == 0`、`material_xuelingshui == 0`、`material_yaodan_wuji == 0`（n26 扣光）、`material_yaodan_liuji` 0 或 1（驱动记 ② 的胜 / 逃）、`material_tianleizhu == 0`、`material_jinleizhu == 0`、`material_lianjian_fuliao == 0`、`weapon_qingzhu_fengyun_jian == 1`、`story_jinzhi_lingpai == 1`、`story_kuilei_qun == 0`、`story_kuilei_can == 1`、`story_yaoshou_ziliao == 1`、`story_yaoshou_cailiao == 0`、`story_kuilei_yuan_qun == 1`、`story_miaoyin_yaopai == 1`、`story_bai_zhenzhu == 1`、`story_shijin_chong == 1`、`story_xueyu_zhizhu == 1`、`field_tianxing_39` 已开 6 槽、`ch11.lieyao >= 4`、`ch11.jiguan == 1`、`ch11.zhu == 3`、`ch11.zhu_gen == 6`、`ch11.jiesuan >= 1`、`material_lingshi_zhong` 恰 20 ＋ 驱动卖丹所得折算（驱动自己记账）、**`day` − 起点 `day` 两侧相等**：固定段合计 33,360 日（n06 900、n08 180、n14 1800、n15 735、n16 21600、n21 15、n24 30、n26 720、n29 补足到 7200、n30 180）＋ 诱妖每轮 3 ＋ 等绿液每次 5（驱动记账）、`mapId == ch11_dongfu` 且站在洞府门外那一格 |
| 3 | 挂点 `mode` / `once` / guard / set_flag 与 F 表一致；无 `auto`；`trigger_youyao`、`trigger_peizhu`、`trigger_jiesuan`、`trigger_huicheng_chuansong`、`npc_xingkong_xiushi` 挂的脚本 `once=false`；同格换触发器的四组（红瑚岛登岸、礁岛三连、密室出关、炼器台）按旗标互斥（O-7） | `Ch11TriggerModeTests` 读七张新图逐条对表 |
| 4 | 四张编成与 I 表一致：① 敌恰 `qianmu_yao` ×1 ＋ `siji_haishou` ×2、`sanji_haishou` 0、`defeat_is_fatal true`、`can_escape true`、`rewards.cultivation > 0`、`drops` 恰 `material_yaodan_wuji` ×1 rate 100、`qianmu_yao.weaknesses` 含「土」；② `jiguan_jiao` ×1、`defeat_is_fatal false`、`can_escape true`、`drops` 恰 `material_yaodan_liuji` ×1、`weaknesses` 含「金」；③ `yinsha_shayao` ×4、`defeat_is_fatal true`、`can_escape false`、`weaknesses` 含「剑」；④ `tiandu_shi` ×1、`killable_by` 空、`faction ally` 恰 `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2、`weaknesses` 恰 `["水","火"]`、`defeat_is_fatal true`；四场 `hero_absent false`、`spirit_stones == 0`；③④ `drops` 空、`cultivation == 0`；**② 三回合内逃得出、④ 只靠友军两技能也能到「困住」**（平衡路实测，记施工偏差） | `Ch11BattleDataTests` ＋ `BattleHand` |
| 5 | **境界**：`realm.advance` 本章恰 1 处（`biguan.lua`，`realm.CORE_EARLY`，看返回值）；`realm.cap` 0 处；`realm.demote` 0 处（全仓仍恰 1 处）；切片（起点 `ch10-end-first.sav`，走到 `ch11.sangong`）：n16 之前面板 `storyCap == 23`、n16 之后 `realmCap == 31` 且面板下一关写「结丹中期」所需 5000、硬按被 `BreakthroughBlock` 回绝 | 扫 `scripts/` ＋ 切片 |
| 6 | **降尘丹 / 雪灵水有则扣**：`take("pill_jiangchen_dan", 5)`、`take("material_xuelingshui", 2)` 各恰 1 处（`biguan.lua`）；`ch11.dan_fu` 只写位 1、2；切片三版（起点 `ch10-end-first.sav` 改背包）：五颗两滴 → `dan_fu == 3`；零 → 0、旁白换 key、旗标照置；`breakthroughPillBonus` 仍返回 0 | 扫脚本 ＋ 切片 |
| 7 | **六十年**：`biguan.lua` 里 `advance_days(3600)` 恰 6 处、无别的 `advance_days`；`playSecondsGameplay` 在整段蒙太奇里只按真实秒累计（切片：headless 演完 n16，`day` ＋21600、`playSecondsGameplay` 增量 < 5 秒） | 扫脚本 ＋ 切片 |
| 8 | **诱妖循环**：`youyao.lua` 开头判草与绿液；缺草 → 不扣不拨日、旁白指凝翠岛；缺绿液 → `advance_days(5)`、不打；成立 → `take("herb_nichang_cao", 1)`、`bottle.spend(1)`、`advance_days(3)`、`battle("b11_ningcuidao_youyao")`；`lieyao == 4 ∧ jiguan == 0` → `battle("b11_honghu_jiguanjiao")`、三支都置 `jiguan`；切片（起点 `ch10-end-second.sav` 走到 `ch11.buzhen`）：草 0 → `lieyao` 不变；连打五轮 → 第五轮是 ②；逃 → `jiguan == 1`、`material_yaodan_liuji == 0`、`lieyao` 不加 | 扫脚本 ＋ 切片 |
| 9 | **传送永远走得通**：`xingkong.lua` 首次 `take(..., 2)`、此后 `take(..., 7)`，两处都有不足那一支且仍 `teleport`；`ch11.huicheng >= 1` 时只一句、`return`、不 teleport；切片：中阶 0 块到星空殿 → 落在 `ch11_ningcui_jie`；`huicheng` 后再按 → `mapId` 不变 | 扫脚本 ＋ 切片 |
| 10 | **有几块付几块**：n02、n04、n07、n10、n18、n26、n27 的每一处 `take` 都看返回值、都有走得通的一支；切片（起点 `ch10-end-first.sav` 把中阶改 0）：n02 → `zhihuan == 2`；n04 → `zufu == 1`、`story_jinzhi_lingpai == 1`、执事「记账」那一句；n18 → `story_kuilei_yuan_qun == 1`；n26 → `material_lianjian_fuliao == 1` | 切片 |
| 11 | **培竹与六根**：`peizhu.lua` 两个分支由 `ch11.jinlei` 换；`zhu` 只在 0–3、`zhu_gen` 只在 0–6；`zhu == 3` 时 `take("material_tianleizhu", 1)` ＋ `give("material_jinleizhu", 1)` 恰各 1 处；n29 每次 `advance_days(1080)` ＋ `give("material_jinleizhu", 1)`；`zhu_gen == 6` → 补足到 `today() − ch11.kaidian_ri >= 7200`（至少 360）、`give("material_lingshi_zhong", 20)`、置 `liugen`；切片（起点走到 `ch11.kaidian`）：多结算 1000 日与不多结算，`liugen` 之后 `day − kaidian_ri` 都恰 7200 或 7200 ＋ 余数（照 `sanzhuan.lua` 的补足写法核）；`material_jinleizhu == 6` 且炼器台此时才开 | 扫脚本 ＋ 切片 |
| 12 | **炼剑走面板**：`facility_lianqi_39` kind `forge`、grade 3、`require_flag == "ch11.liugen"`；`recipe_qingzhu_fengyun_jian`：`productCount 1`、`inputs` 恰金雷竹 6 ＋ 辅料 1 ＋ 灵石 20、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`；本章脚本 0 处 `give("weapon_qingzhu_fengyun_jian")`；`trigger_chujian` 要 `item.count("weapon_qingzhu_fengyun_jian") >= 1`；切片：炼器熟练 0 的存档走 `openFacility` → `AlchemyScene(Forge, 3)` 炼一次 → 必得 1 件、金雷竹 0 | 读数据 ＋ 扫脚本 ＋ 切片 |
| 13 | **店铺回访结算**：`jiesuan.lua` 每满 30 日给 15 低阶、单次封顶 3600（7200 日）、再写 `jiesuan_ri = today()`、`jiesuan += 1`；首次带旁白、第二次起只结算；切片（起点走到 `ch11.kaidian`）：开店当日按 → 0 灵石、`jiesuan == 1`；拨 90 日再按 → ＋45；拨 9000 日再按 → ＋3600 | 扫脚本 ＋ 切片 |
| 14 | **分身**：`party.add / remove` 本章 0 处；`qu_hun_shadan.json` 与第 10 章终点逐字节相同；`wu_chou.json`、`zi_ling.json` 与第 10 章集成版逐字节相同（selftest 临时副本不算，O-2） | 读数据 ＋ 扫脚本 |
| 15 | **经济账**（N.1）：收购行 `sellRate == 0.8`、`entries` 空；青竹小轩 `sellRate 0.6`、`entries` 空；坊市杂货 `herb_nichang_cao` stock 10 / restock 5 日 ×5 / max 10、`require_flag ch11.nichang`；凝翠岛杂货 stock 5 / ×3；五级妖丹在收购行卖 760、六级 4800；两侧通关走同一组选项，章末两侧低阶灵石差等于起点差（20），中阶相等 | `Ch11LedgerTests` ＋ 读 `data/shops/` |
| 16 | **必打 4 场绕不过去**：① ② 由 `youyao.lua` 一处调用，③ `guiju.lua`、④ `daoge.lua` 各一处；③④ 的 `set_flag` 在目标链里且是下一步 guard；通关两侧各记实际打到的编成，恰是 ① ×≥4 ＋ ② ＋ ③ ＋ ④；本章没有别的 `battle(`；遭遇区 0 处 | 读数据 ＋ 扫脚本 |
| 17 | **走不回去的路**：七张新图只有四对门（坊市 ↔ 洞府、洞府 → 星空殿、凝翠岛 ↔ 红瑚岛、坊市 ↔ 青竹小轩）；礁岛无门；通第 10 章任何图的门 0 扇；切片：`ch11.huicheng` 置位后从星空殿到不了凝翠岛；`ch11.liugen` 置位后店门隐藏 | 读地图 ＋ 切片 |
| 18 | **NPC 在场链**：G 表每个 NPC「前一段的 hidden 恰是后一段的 visible」；范夫人三个对象同 role、紫灵两 role 首尾相接（`ch11.yaoshi`）；乌丑 → 极阴祖师同格换 NPC；`jiedan_linju` 两对象按 O-4 核 | 规则 30 ＋ `NpcPresenceTests` ＋ 读地图 |
| 19 | **支线 Z1、Z2** 谓词与 N.3 字面量相符：`q11_shougou_dingdan` `accept ch11.ningcui`、`fail ch11.huicheng`、交 3 颗五级妖丹、酬中阶 10 ＋ `material_lianqi_yao` 2、`origin "改编"`；`q11_linju_dingyan` `accept ch11.zhaolan`、`complete ch11.z2`、`linju_z2.lua` 的 `take("pill_dingyan_dan", 1)` 看返回值 | 读 `data/quests/q11_*.json` ＋ 扫脚本 |
| 20 | **路径行动 14 条**：全部 `realm 0`、`until` 含 `ch11.done`、`kind` 只有 inquire / buy、`spar` 0 条；两条 `reveal` 的类别在那个 role 的 `weaknesses` 里（`qianmu_yao` 土、`jiguan_jiao` 金）；`zaohong_dating_jiguan` 的 `until` 含 `ch11.chuanshong`（可错过） | `PathActionTests` ＋ 读 `data/pathactions/ch11.json` |
| 21 | **章末与开局**：`zhuzhong.lua` 最后置 `ch11.done`、不 `ending()`；`zhifadui.lua` 末尾一行 teleport 指 `ch11_fangshi` / `chengmen`，与第 10 章集成版 diff 只此一行；`data/chapters.json` 第 11 章是表里最后一章——headless 演完 `zhuzhong.lua`，章节卡队列是「第十一章　终」接「未完待续」 | 扫脚本 ＋ 真实入口 |
| 22 | **禁词**：表一含 12.1 两张表的词；12.2 的章内先后按 key 前缀逐条断言；「虚天残图」「虚天殿」全章各恰 1 处且都在 `wuchou.*`；分母先验 > 250 条 | `LexiconTests` ＋ 专项 |
| 23 | **暗场与死亡口径**（13 第 14、15 条）逐 key 核；「明珠轩」「交换会」「文思月」「元瑶」「银月」「紫罗极火」「黄龙」「猎妖令」「隆兴客栈」「元磁神光」「十八天都尸」「天音门」「文樯」「木龙真人」「白玉蜘蛛」「天晶木」0 处 | 独立校对 ＋ 12.2 专项 |
| 24 | 独立校对：A 表七十四条硬约束逐条核验；13.1 逐段并排；原创性人工复核（13 第 13 条那十三处） | 校对报告 |
| 25 | 主线节点 31、可重复循环 3（诱妖、培竹、结算）、分支 ≥ 5（n02、n03、n13、n14、n16 三版）、新地图 7、战斗 4 | `tools/audit.py` |
| 26 | **引擎**：E7 锚点注释改字、`Ch11EngineTests` 的 21600 日用例（O-5）；`tests/fixtures/*.sav` 与 `saves/*.sav` 在 v9 下全部读得进来 | 第 10 节 |

---

## 16. 拍板记录与开放问题

### 16.1 已拍板（按推荐，预授权——下表 30 条全部按推荐落地）

| # | 拍板 | 本稿怎么落 |
| --- | --- | --- |
| 1 | **开局 patch `scripts/ch10/zhifadui.lua` 末尾一行 teleport**；`chapters.json`、`ui.json` 加第 11 章；`ch10-end` 两份重生成 | 10.1 E1、E2；不在 `ch10_xiaohuan` 挂开局挂点（第 10 章纪律 46） |
| 2 | **章节卡写「第十一章」「结丹」**；设计名「天星城·结丹」保留；「天星城」三字不上卡 | 10.1 E1；12.2 第一行 |
| 3 | **结丹走剧情 `realm.advance(realm.CORE_EARLY)`**，不做面板冲结丹；本章 `realm.advance` 恰 1、`realm.cap` 0、`realm.demote` 0 | 验收 5；笔记 C.3 |
| 4 | **降尘丹 5、雪灵水 2 有则扣、旁白三版**；不接 `breakthroughPillBonus` | 11 节 `ch11.dan_fu`；验收 6；10.1 O1 |
| 5 | **六十年三段蒙太奇、6 × `advance_days(3600)`**；跳过的时间不计游玩时长（`playSecondsGameplay` 只按真实秒） | 验收 7；10.1 O2、O8 |
| 6 | **天星双圣、星宫长老不建 role、不上图** | 12.2「天星双圣（人物）」 |
| 7 | **新图 7 张、旧图 0 张修补、主题时辰全用现有**；坊市一图三区 | G 表；验收 17 |
| 8 | **商行四张商店**：坊市杂货、凝翠岛杂货、凝翠岛收购行（`sellRate 0.8`、只收）、青竹小轩（卖出 0.6） | L.4；验收 15 |
| 9 | **丰乐拍卖行不做面板**：门面 ＋ 路径行动两条 | 10.1 O3；16.3 第 3 条 |
| 10 | **悬赏走任务板一处 ＋ 两条可选支线**；`origin "改编"` 替代猎妖令 | N.3；验收 19 |
| 11 | **三十九层静室 `effectiveness 125`** | 验收 26 之外由 `Ch11TriggerModeTests` 读设施属性 |
| 12 | **新药园 `field.unlock("field_tianxing_39", 6)`**；旧田走不回去 | 验收 2 |
| 13 | **红环 8 中阶、洞府 27 中阶，有几块扣几块、零块记账**；不设门闸 | 验收 10 |
| 14 | **传送费：首次 2 中阶、此后 7 中阶，不足搭阵免费——传送永远走得通** | 验收 9 |
| 15 | **诱妖循环走可重复触发器 ＋ 剧情战**，不设遭遇区；第 5 次转 ② | 验收 8、16 |
| 16 | **七十二剑走炼器面板**，配方改一件、必成；`weapon_qingzhu_fengyun_jian` 一件代表七十二口 | 验收 12；L.3 |
| 17 | **本章不新学法术**；剑影分光术、血灵钻、丹火不做法术 | L.2；12.2 |
| 18 | **`qu_hun_shadan` 全章在队、数值名牌不动** | 验收 14 |
| 19 | **规则 25 登本章八类 ＋ 四场化身手段；探针挪到 `wu_chou` ＋ `b12_neidian_duoding`** | 10.1 E3 |
| 20 | **词表 M 表交 `LexiconTests`**；明珠轩、交换会、文思月、元瑶 0 处 | 12.1；验收 22、23 |
| 21 | **teleport 登 `kScriptTransfers`、链外旗标登 `kOffChainFlags`** | 10.1 E5 |
| 22 | **店铺回访结算**：每满 30 日 15 低阶、封顶 3600；收店尾款 20 中阶 | 验收 13 |
| 23 | **必打 4 场**：① 诱妖（致命、可逃）、② 鸡冠蛟（不致命、可逃、胜为改编）、③ 煞妖 ×4（致命、境界压制）、④ 妖尸（致命、困而不杀、四友军）；余 11 场给第 12–14 章 | I 表；验收 4 |
| 24 | **不演的场**：范赵密谋、极阴赤火分赃、三女议事、凡人店主群像 | 13 第 14 条 |
| 25 | **死亡只写倒下**：封某「再无声息」、煞妖「飞灰」可用、人不写灰烬 | 13 第 15 条 |
| 26 | **支线 2 可选 0 必经** | N.3；16.2 第 8 条 |
| 27 | **曲魂代言到 n17 为止**，之后外人直接与韩立说话；交代一句、不再解释 | 13 第 16 条 |
| 28 | **通缉不设状态字段**；只在 n01 旁白与 K1 一句 | 12.2；10.1「不做」 |
| 29 | **路径行动 14 条、全部 `realm 0`、两条 reveal**（千目妖「土」、鸡冠蛟「金」） | 16.3；验收 20 |
| 30 | **回写由协调者做**（B1–B3、B5、B6、B13）；本稿不动仓库 | 10.1 E11 |

### 16.2 与旧拍板的冲突、开放问题（拿不准、需协调者核的）

1. **O-1 战斗 ④「困而不杀」的数据表达**：`killable_by` 空 ＋ 低 `toughness` 能否表达「破两次架势即胜」？若引擎胜负只看气血，平衡路把 `tiandu_shi` 气血压低、自愈用 `charge` 表达——需 `BattleHand` 先证明（验收 4 末项）。
2. **O-2 探针挪到 `wu_chou`**：`validate_selftest.py` 第 1813 行把 role 文件临时写成 `weaknesses ["毒"]` 再改回；第 10 章 16.2 第 7 条「`wu_chou` 不动」指提交的数据，测试临时副本不算——协调者确认。
3. **O-3 路径行动 `give` 的返回值**：K12 改挂 interact 脚本 `linju_z2.lua`；若引擎已支持 `take` 条件，可挪回路径行动。
4. **O-4 `jiedan_linju` 两对象同 role**：规则 30 若判其为具名，拆 `jiedan_linju_jia` / `_yi`（验收 18）。
5. **O-5 `advance_days` 六次 3600**：`restFor` 与灵田按年推进在 21600 日内是否溢出——`Ch11EngineTests` 跑一遍（10.2 O2）。
6. **O-6 青竹小轩的卖出口**：玩家卖符给自己的店是否怪——若怪，`ch11_qingzhu_xiaoxuan` 改成只结算、不设 shop facility；验收 15 那一行随之删。
7. **O-7 同格两个 trigger**（红瑚岛登岸、礁岛三连、密室出关、炼器台）：按旗标换触发器是第 10 章 `zhenwei_*` 的写法；validate 若不许同格两个对象则各挪一格（验收 3）。
8. **O-8 支线配额**：本章 2 可选 0 必经；全作必经 14 条的账若要本章出 1 条，Z1 改必经并进目标链（链变 32 步，验收 25 随改）。
9. **O-9 「极阴祖师」的 realm**：写 `CoreLate` 只为不越上限，天眼术会显示「结丹后期」；文案里说「看不透」（10.1 O7）。
10. **O-10 `ch10-end` fixture 重生成**：E1 patch 之后两份 `mapId/position` 变 `ch11_fangshi` 城门格；第 10 章验收 2 末项措辞随改——是那条验收的一处落点改动，不是推翻那条板。
11. **第 10 章 16.1 第 35 条「探针改指第 11 章（`siji_haishou` ＋ `b11_ningcuidao_youyao`）」**：本章把那张编成写实了（B11），探针再挪到第 12 章——与第 9→10 的处理同一个套路，不是用户的板。
12. **第 10 章 16.2 第 9 条「降尘丹、雪灵水第 11 章定」**：本章定为有则扣、不接加成（16.1 第 4 条）；`breakthroughPillBonus` 桩留给要做面板冲结丹的章。
13. **lore 4.3「每月缴灵石」与 ch391「百年计费」**（B7）：两句原文都有；本作红环一次 800、洞府百年 2700，「每月」只在典籍台词——lore 加注。
14. **B6 赵 / 孟 / 符长老称谓**：原文前后不一；本作只写「赵长老」「符长老」，孟道友不叛——总表加注。
15. **B23 隆兴客栈并进清河茶馆雅间**：省一张图；图鉴注改编。
16. **战斗场数**：第 1–10 章 37 场 ＋ 本章 4 ＝ 41；方案 52 场余 11 给第 12–14 章（`data/battles/` 里 48 个 b 前缀文件含切磋与重复编成，口径以施工图为准）。
17. **时长 ≈ 55 min**（目标 55），真要压先压第 14 节那三处。
18. **`siji_haishou` 改破绽**：全仓仅 `b11_ningcuidao_youyao` 一张编成含它（2026-10-12 grep 过），不牵动第 1–10 章；`sanji_haishou` 从编成去掉后仍在 `data/roles/` 里（别处是否引用由协调者核）。

### 16.3 路径行动条目（施工用；`ch11.path.*` 旗标**等 `data/pathactions/ch11.json` 落地后由协调者登记**）

形状照 `data/pathactions/ch10.json`；文案在 `data/text/ch11_path.json`，key `ch11.path.<id>.text`；每条 `until` 另含 `ch11.done`；**一律 `realm 0`**；`spar` 0 条（城内不准斗法，A4）；本章不分语言两版。`when` 里一律写上链上旗标（12.2 的编号按它排）。

| id | 图 / NPC | kind | when | until | 效果 |
| --- | --- | --- | --- | --- | --- |
| `baiyi_dating_guiju` | 坊市 / `npc_baiyi_xiushi` | inquire | `ch11.chudao >= 1` | `ch11.zhihuan >= 1` | 城规三条（不斗法、每月缴灵石、不干涉）与红蓝环（A4、A9）；通缉一句（E28） |
| `luer_dating_shengshan` | 坊市 / `npc_lu_er` | inquire | `ch11.zhihuan >= 1` | `ch11.anju` | 圣山八十一层、前五十层对外、顶层圣宫（A14） |
| `fengle_dating_mulu` | 坊市 / `npc_fengle_menmian` | inquire | `ch11.kuilei >= 1` | — | 拍卖目录有五六级妖丹、进拍卖行贵一多半（A24）；无效果（E9） |
| `chuwu_qiugou_dai` | 坊市 / `npc_chuwu_zhanggui` | buy | `ch11.kuilei >= 1` | `ch11.huicheng` | 大容量储物袋上千灵石——只一句，不扣钱、不给物（A23）；`give` 空 |
| `baimen_dating_zhaixing` | 坊市 / `npc_baimen_zhanggui` | inquire | `ch11.ziliao >= 1` | — | 摘星大会百年一次、双圣近千年不露面（A3、A8） |
| `haoyuantian_dating_waihai` | 凝翠岛 / `npc_hao_yuantian` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 外星海规矩：不贴海面、不飞太高、不聚众（A32） |
| `haoyuantian_dating_qianmu` | 凝翠岛 / `npc_hao_yuantian` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | **`reveal` `qianmu_yao` 土**（滚圆带刺、怕被埋住——改编的理由写在文案里） |
| `fanxiaozi_dating_jiage` | 凝翠岛 / `npc_fan_xiaozi` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 五级妖丹近千、六级够一生、收购行是丰乐盟名下（A7、A30） |
| `zahuo_dating_haiyu` | 凝翠岛 / `npc_zahuo_nc` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 海域图要常更新、精细图上面定价（A30）；只做一条，文案不提有无图 |
| `zaohong_dating_jiguan` | 星空殿 / `npc_zaohong_laozhe` | inquire | `ch11.nichang >= 1` | `ch11.chuanshong` | **`reveal` `jiguan_jiao` 金**（六级蛟类鳞甲只惧金刃——改编；不出「鸡冠蛟」三字）；**可错过**：n09 首演前才打探得到 |
| `linju_dating_miaoyin` | 洞府 / `npc_jiedan_linju_a` | inquire | `ch11.chaguan >= 1` | `ch11.zhuzhan` | 妙音门底细：女弟子为主、左右双使、客卿长老、狐媚术（ch402） |
| `linju_qiugou_dingyan` | 洞府 / `npc_jiedan_linju_b` | buy | `ch11.zhaolan >= 1` | — | 只打探「邻居想要定颜丹」；交换本身挂 `linju_z2.lua`（Z2；O-3） |
| `hexianshi_dating_fanren` | 坊市 / `npc_he_xianshi` | inquire | `ch11.kaidian >= 1` | `ch11.liugen` | 坊市凡人店主的活法、何仙师炼气三四层（A66） |
| `jinqing_dating_luanxing` | 洞府 / `npc_jin_qing` | inquire | `ch11.lianjian >= 1` | — | 七八个势力叛乱、双圣谣言（只到谣言）（A72） |

`npc` 字段是各图上那个 NPC 对象的名字（关卡路起）；`reveal` 的类别必须落在那个 role 的 `weaknesses` 里（门禁查），战斗路若改破绽，两边一起改。③④ 的破绽由战中试探，不设 reveal。

### 16.4 拍板前的原表（保留，供追溯；照笔记 E 表）

| # | 问题 | 推荐 | 不选推荐会怎样 |
| --- | --- | --- | --- |
| E1 | 开局怎么接 | **patch `zhifadui.lua` 末尾一行 teleport**；`chapters.json`、`ui.json` 加行；fixture 重生成 | 在 `ch10_xiaohuan` 挂 guard 触发器：第 10 章图要 patch，违反纪律 46 |
| E2 | 章节卡标题 | **「结丹」** | 写「天星城」会把 n01 的揭示提前 |
| E3 | 结丹怎么上 | **剧情 `realm.advance`** | 面板冲关要 3500 修为、可反复摇骰，与「六十年一关」的重量相悖 |
| E4 | 降尘丹、雪灵水 | **有则扣、不接加成** | 接了要动 `CultivationScene.cpp` 读背包、再登测试；本章不走面板，接了也没人读 |
| E5 | 六十年怎么跳 | **三段蒙太奇、6 × 3600** | 抬 `kMaxScriptAdvanceDays` 是引擎项；新画面是美术项 |
| E6 | 天星双圣、星宫长老 | **不建 role、不上图** | 上图就得给他们台词，原著全程不露面 |
| E7 | 新图张数 | **7 张、旧图 0 修补** | 8 张以上超大纲；少于 7 张星空殿并进洞府，传送费与出海循环混在一张图上 |
| E8 | 商行怎么做 | **四张商店；收购行 `sellRate 0.8`** | 默认 0.5 则一颗五级妖丹只回 475，比传送费还薄，循环没有动机 |
| E9 | 丰乐拍卖行 | **不做面板** | 做面板至少一周引擎工；韩立「买不起」，做出来玩家也只能看 |
| E10 | 悬赏与商行订单 | **任务板一处 ＋ 两条可选支线** | 不做任务板则「悬赏」只剩台词，大纲改编标注落空 |
| E11 | 三十九层静室 | **`effectiveness 125`** | 不写＝100，与「过三十层快四分之一」对不上 |
| E12 | 新药园 | **`field_tianxing_39` 6 槽** | 复用 `field_xiaohuan` 违反契约第 3 节 |
| E13 | 租金与红环 | **有几块扣几块、零块记账、不设门闸** | 设门闸会在 first 侧 117 中阶之外的任何合法花法上死局 |
| E14 | 传送费 | **首次 2、此后 7，不足搭阵免费** | 700 硬门闸 ＋ 一趟只回 1–2 颗丹，玩家可能卡在凝翠岛 |
| E15 | 诱妖循环 | **可重复触发器 ＋ 剧情战，不设遭遇区** | 遭遇编成掉不了妖丹，妖丹经济不成立 |
| E16 | 七十二剑 | **配方改一件、面板必成** | 剧情直接 `give` 则炼器面板全作没用过一次；加 Lua 熟练命令是引擎项 |
| E17 | 本章法术 | **不新学** | 新法术要走 `data/magics` ＋ 规则 25 ＋ 平衡，预算给了结丹锚点 |
| E18 | 分身 | **全章在队、数值名牌不动** | 改它就牵动第 10 章 ④ 执法队的平衡 |
| E19 | 规则 25 与探针 | **登本章八类；探针挪 `wu_chou`** | 探针不挪则本章写实 `b11` 后 selftest 红 |
| E20 | 词表 | **M 表交 `LexiconTests`** | — |
| E21 | 测试登记 | **teleport 登 `kScriptTransfers`、链外旗标登 `kOffChainFlags`** | 规则 30 用例红 |
| E22 | 店铺回访结算 | **柜台触发器按日结算** | 做成真实库存经营要新模块 |
| E23 | 战斗场数 | **4 场** | 5 场以上挤第 12 章虚天殿 |
| E24 | 不演的场 | **四处** | 判据③ |
| E25 | 死亡口径 | **只写倒下** | 沿第 10 章 16.1 第 29 条 |
| E26 | 支线 | **2 可选 0 必经** | 必经支线要进目标链，本章链已 31 步 |
| E27 | 曲魂代言 | **到 n17 为止** | — |
| E28 | 通缉 | **不设状态字段** | — |
| E29 | 路径行动 | **14 条、`realm 0`、两条 reveal** | — |
| E30 | 回写 | **协调者做** | — |

---

## 17. 施工纪律（沿用前十章，外加本章的九条）

沿用 `docs/ch04-design.md` 第 8 节、`docs/ch05-design.md` 第 17 节、`docs/ch06-design.md` 第 17 节、`docs/ch07-design.md` 第 17 节、`docs/ch08-design.md` 第 17 节、`docs/ch09-design.md` 第 17 节与 `docs/ch10-design.md` 第 17 节（第 42–49 条）。本章另加：

50. **境界只经一处**：`realm.advance(realm.CORE_EARLY)` 只在 `biguan.lua`、看返回值；`realm.cap` 0 处、`realm.demote` 0 处——散功只写旁白；面板上 `storyCap` 保持 23 直到 n16，n16 之后不许过「结丹中期」。
51. **大跨度时间只用 `advance_days` 分段**：单次不超 3600、不抬 `kMaxScriptAdvanceDays`；六十年恰 6 次、二十余年按 `ch11.kaidian_ri` 补足到 7200（至少 360）；文案里不写天数。
52. **循环触发器自己判状态**：`trigger_youyao`、`trigger_peizhu`、`trigger_jiesuan`、星空殿与石屋传送阵都 `once=false`，脚本开头先读旗标与物品、每一条分支都有走得通的一支；缺草不扣、缺绿液只拨日、`huicheng` 后只一句。
53. **有几块扣几块**：n02、n04、n07、n09、n10、n18、n26、n27 的每一处 `take` 看返回值、零块换一句旁白、旗标照置；本章门闸 0 道花钱的——n26 的辅料不看扣到多少、必给。
54. **妖丹只从编成掉**：五级妖丹的来路只有 ① 的 `drops` 与 n14 的 8 颗、六级只有 ② 的 `drops`；脚本里不许别的 `give("material_yaodan_*")`；遭遇区 0 处。
55. **炼剑不剧情给**：`weapon_qingzhu_fengyun_jian` 只由面板产出；`trigger_chujian` 看 `item.count`；配方的 `requireFlag` 与炼器台的 `require_flag` 都是 `ch11.liugen`。
56. **旧图不 patch、旧脚本只补一行**：本章 0 处地图 patch；`scripts/ch10/zhifadui.lua` 只在末尾补一行 teleport；`genmaps_ch05.py`–`genmaps_ch10.py` 一个字不改；`wu_chou.json`、`zi_ling.json`、`qu_hun_shadan.json` 一个字不动。
57. **名字按节点解禁、钩子只许一句**：12.2 就是清单；「虚天残图」「虚天殿」全章各恰 1 处、都在 `wuchou.*`；「隐煞门」在 `zhuzhan.*` 之前 0 处；「鸡冠蛟」只在 `jiguan.*` 与 ② 开场白；名牌跟着 role 走（紫灵仙子 → 紫灵、乌丑 → 极阴祖师是换 role）。
58. **韩立不知道的不写、章末停在递珠**：范赵密谋、分赃、三女议事、乌丑怎么处置孙姓中年人——情报、旁白、路径行动、物品描述都不许写到；`ch11.zhuzhong.*` 的最后一条是金青递珠；不写去向、不 `ending()`；「未完待续」是章节卡的事，文案里 0 处。

## 18. 施工偏差（施工阶段填）
