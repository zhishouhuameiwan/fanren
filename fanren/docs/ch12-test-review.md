# 第 12 章测试质量独立审查

2026-10-13。审查范围：
- `tests/Ch12*.cpp`、`tests/Ch12*.h`，9 个文件，4367 行；
- 本章改过的 10 个旧用例文件（`git diff -- fanren/tests`，+189 / −36）；
- 两份重生成的 `ch11-end-*.sav`、两份新写的 `ch12-end-*.sav`。

本审查只读不改，唯一写入的就是本文件。口径照 `docs/handoff.md` 第 6 节与 `docs/ch11-test-review.md`。

## 结论：Approve（score 5）

主干扎实：
- **起点真实。** 两侧通关都从 T1 重生成版 `ch11-end-{first,second}.sav` 起步。`load()` 先逐项验 C.3 的九个「必须」：day 49378 / 49353、低阶 2339 / 2319、中阶 20、hp / mp 700 / 470、realm 31 / 31 / 22、party 恰曲魂、11 门法术、六件 story、站在 `ch12_tushan` (24,31)。然后经 `continueJourney` 进入，断言读档前后逐行相同。
- **入口真实。** 之后全程走 `WorldScene::tryStep / interact`、真门、`openFacility` 开出的 `CultivationScene`、真 `BattleScene`。
- **终点是字面量。** 逐项抄验收第 2 条：23 种 story、金丝蚕 3、炼晶 1、三件 0、七面旗标、低阶 `side.low − 8`、中阶 `side.mid`、`day − side.day == 182`。`side.*` 本身就是按真档写死的字面量。
- **检查自带负向。** 起点守卫换成另一侧的字面量必报 2 条；终点 9 处单点变异、逐节点账 3 处变异，各恰报 1 条。
- **两侧真不同。** 起点差 25 日、20 块低阶。两份 `ch12-end` 有 243 个字段不同（历程、田地、旧章分支旗标），本章旗标两侧完全相同。
- **负向与分母先验齐全。** 新检查几乎都带「内存里改坏 → 必报」的负向。分母先验都在：脚本 39 份、文案 > 250 条且 ≥ 11 份、路径行动 13 条、物品与法术名 30 个、role > 100、配方 > 15、商店 > 10。
- **第 11 章终档重生成只差地图与坐标**（见下节）。
- **实跑全绿。** 本审查跑了 161 + 197 + 16 个用例，全过。B 路全量日志 1944 / 1944，四道门禁全过，0 个编译警告。

没有 CRITICAL / HIGH。两条 MEDIUM 要协调者处理：T-1 是施工图回写，T-2 建议补一条切片。六条 LOW 不挡放行。

## 问题清单

### T-1 MEDIUM：测试按实测与派工定下的判据，施工图第 18 节还没登记；18.3 还留着调前的数

测试本身与数据、脚本的实际行为一致。但下列几处，施工图原文要么没有，要么与之相反。照「判据抄施工图原文」的纪律，要协调者拍板回写。协调者备注已说要并入第 18 节，本审查时还没落。

1. **回合窗 `kTargets`**（`Ch12AcceptanceTests.cpp:35`）：①12–20、②7–14、③6–14、④5–12。这是按调后实测（16 / 10 / 10 / 8 回合）放宽约四成定的；第 8 节与 10.2 B1 都没给回合数。窗本身有牙：调前 ③ 胜 5 回合、④ 胜 29 回合都落在窗外，①② 调前是败。
2. **调数没登记，18.3 留着旧数。** 10 个敌方 role 的调数、玄骨法力 860→96（协调者已裁定接受）都没进第 18 节。18.3 第 2、3 条仍写调前值：玄骨 1400 / 860、攻防 88 / 64；三首 1500、架势 8；大群 1200、架势 7……与 `data/roles/*.json` 现值（玄骨 450 / 96 / 36、架势 4）相反。测试不钉 role 数值，所以不会红；但下一个照 18.3 改数的人会把平衡改回去。
3. **意图手不吃药**（`Ch12Driver.h:49`，`pills = {}`）：8.1 没写这一条，与第 10、11 章的「五成吃药」不同。
4. **验收第 2 条「站在 quanbian」**：n33 挂在元瑶 (15,9) 身上，驱动说完话自己走回 (12,11)、朝上（`Ch12Driver.h:501`），所以 `position == kEndCell` 只证明驱动走到了那里。我核过 (12,11) 就是 `quanbian` 的 default spawn。措辞要改成「说完话站回 quanbian」，或把这一格写死。
5. **验收第 5 条「面板写『结丹中期』所需 5000」**：Slice Five 断言的是 `cultivationNeeded()`、`realmText()` 两个函数的返回值与 `disabledReason`，不是面板上显示的字。这与第 11 章同况（18.5 第 14 条），要照第 11 章那样登记。
6. **③ 的兜底打坐地点。** 8.1 写 ③ 的兜底是「塔前石室」，最低合法起点用例却在寒骊台台下静养。用例的做法是对的：n27 单向传送之后回不去塔前，按 10.2 B1「最近的打坐设施」就该在寒骊台。登一句即可。

改法：协调者把 `notes/ch12/balance.md` 第 0、3 节与上面 1–6 条并进第 18 节，并改掉 18.3 第 2、3 条的旧数。

### T-2 MEDIUM：辟邪神雷从没在任何运行时用例里放过一发，它的五行也没人钉

- **施工图要它收尾，用例从没用过它。** B1 写「④ 只有木要命、神雷 2 发 ＋ 青元剑芒收得了尾」，8.2 ① 写「神雷两发收尾」。可 `BattleHand` 不会用单体 castMagic 道具，`Ch12Driver.h:41–43` 的注释自己也承认。我开 `FANREN_CH12_BATTLE_LOG=1` 重跑，四场正赛加七场反证的出手记录里，「辟邪神雷」出现 0 次。
- **五行没人钉。** `magic_bixie_shenlei` 的 `element == 2`（木）全仓没有检查：`Ch12Ledger` 只核物品的 `castMagic` id；`validate.py` 第 2573 行起的 `give` 来路只核「毒」与兵刃类，不核 castMagic 的五行。按代码读，把它改成火，没有一道门禁、一条用例会红；而玩家在 ④ 拿神雷收尾，会被 killable_by 锁在 1。
- **两个余数分支运行时没走过。** `guiwang.lua` / `xuangu.lua` 战后「余数为 0 不调 take」那一支，和 `take` 失败说 `.keep` 那一支，运行时从没走到，只有正则静态钉住（`Ch12LedgerTests.cpp:140–148`）。

不挡放行的理由：④ 的木另有青元剑芒（`CHAPTER_MEANS[12]`），不会出死局；castMagic 的引擎路有第 7 章 `talisman_tianleizi` 的用例。

改法：
- 加一条切片，起点 `reachedAfter(walk, 30)`。进 ④ 后经真菜单发一个 `Item` 动作（神雷）打玄骨，断言这一击类别为木，且能把被剑压在 1 的玄骨打到 0；对照组是同一状态下剑只压到 1。
- 再分「用掉 1 发」「用掉 2 发」两版跑完 n31，断言余数分支各自的台词，以及 `talisman_bixie_shenlei == 0`。
- `Ch12Ledger` 补一行 `magic_bixie_shenlei.element == 2`，配一条「改成火 → 必报」的负向。

### T-3 LOW：元素反证的断言没钉住「被压在 1」

`OnlyTheDesignedElementsKill…`（`Ch12AcceptanceTests.cpp:175–193`）三条反证只断言 `phase != Won && enemyHpLeft > 0`（第 181、184、189 行）。

我数过出手记录，眼下这几条确实有牙：
- ② 捂住水金那一场，「却伤不到……要害」出现 42 处；
- ③ 只靠友军打，20 处，敌余恰 1；
- ④ 捂住木，6 处，敌余恰 1。

问题在 ②：那一场敌余 1162、是韩立先倒，断言分不出「被 killable_by 锁住」和「单纯打不过」。

改法：③④ 断言 `enemyHpLeft == 1`（或断言该 role 的单位 hp == 1）；② 断言出手记录里至少有一处「却伤不到」，或某个蚁群单位停在过 1。

### T-4 LOW：`lidao.lua` 的先后扫描有「找不到就算过」的口子

`Ch12LedgerTests.cpp:78–80` 用 `at()` 判锦帕门闸在 `advance_days(`、`flag.set("ch12.lidao")`、`teleport(` 之前。`at()` 找不到时返回 `size()`，所以删掉后三者任一，静态检查照样通过。

Slice Eight 的对照组（拨 4 日、进 `ch12_dadian`、`lidao == 1`）和通关兜着，所以只是 LOW。

改法：先断言三者都找得到，再比先后；补一条「删掉 teleport → 报」的负向。

### T-5 LOW：三处近乎恒真的「负向」

- `EachTargetRejectsAFightThatMissesIt` 末尾先 `print.back() += " "` 再 `EXPECT_NE`（`Ch12AcceptanceTests.cpp:254`）：比的是 vector 改动前后的自身，恒真，可删。
- `TheDepartedCompanionsRoleIsByteForByteChapterEleven` 的负向 `EXPECT_NE(fnv1a(edited), kQuHunHash)`（`Ch12LedgerTests.cpp:236`）只测了哈希函数，没测断言。改法：照 `sameRoleErrors` 的写法，把改过字节的内容喂给检查函数。
- Slice Five 的对照（`Ch12SliceTests.cpp:60`）只调了纯规则函数。同第 11 章 T-5，已接受。

### T-6 LOW：驱动自证与账口径

- **战斗收支从被测物推导。** 逐节点账的「战斗收支」取的是战前战后存档之差（`Ch12Driver.h:664`）。本章 `drops []`、手不吃药、神雷不在 `kAccounts` 里，这一项本应恒为 0。直接断言 0，就能把「战斗偷偷给了东西」也抓进来。眼下由 BattleData 的 drops 为空、终点各物恰 1 兜着。
- **打到哪一场是推出来的。** 场次由 `expectedBattle()` 按地图与 `ch12.ruyi` 推（`Ch12Driver.h:272、314`），不读真实编成 id。这一点靠逐场比对敌方阵容的 multiset 兜着；四场阵容两两不同，没有留洞。
- **最低合法起点比意图玩家更宽。** 它先静养到满血，而那只手本来就不吃药、不花中阶灵石，所以这条用例不是压力测试。它实际证明的是：②③④ 之前的打坐兜底走得通，`CultivationScene` 一日回一成。建议在注释里写明。

### T-7 LOW：引擎用例里有一处缺项即过

`Ch12EngineTests.cpp:707` 写的是 `if (unit == nullptr) continue;`：③ 里找不到玄骨或乌丑时，四个「友军不吃敌方字段」的断言全部跳过。

BattleData 的 `alliesOf(three)` 和通关的 `fights()[2].allies` 兜着。改法：换成 `ASSERT_NE`。

### T-8 LOW：第 13 节几处能按字扫的 0 处词没有回归守卫

第 13 节第 2、4、5、7、9 条里，能按字扫的有：
- 名号里的「上人」；
- 「炉鼎」「以身相许」；
- 「七十二」「化为乌有」；
- n26 / n31 的「碎尸」。

我用 Python 实读 `data/text/ch12*.json` 共 554 条，这些词全是 0 处，数据眼下合规，只是没有东西防回归。验收第 22 条把它们划给独立校对，所以这不是测试路的欠账。愿意的话，可加进 `zeroWords()`，或按场景做定点断言。

## 覆盖表（施工图第 15 节 26 条）

| # | 用例 | 钉住了吗 |
| --- | --- | --- |
| 1 | B 路 `build-ch12walk.log`（20:14）：VALIDATE_OK、MAPGEN_IN_SYNC（75 张）、SELFTEST_OK、ARTGEN_IN_SYNC（678 个），0 个 `warning C`，1944 / 1944，EXITCODE=0 | 是（门禁本身） |
| 2 | `Ch12Walkthrough` ×2、`Ch12Acceptance` ×3（含起点、终点、逐节点账的负向） | 是；quanbian 一格见 T-1 第 4 条 |
| 3 | `Ch12TriggerMode` 的 EveryHook、Layout，各配 Mutations | 是 |
| 4 | `Ch12BattleData` 2 条（16 条变异）、`Ch12RealFriendFoe`、`Ch12Balance` 5 条 | 是；神雷那半句见 T-2，反证断言见 T-3 |
| 5 | `Ch12Ledger` 扫描；Slice Five（大厅、寒骊台两处） | 部分：面板文字（T-1 第 5 条） |
| 6 | `Ch12Ledger`（remove 看返回值、add 0 处、曲魂哈希）；Slice Six（空队伍演 n05 并打 ①，带对照组） | 是 |
| 7 | `Ch12BattleData` SameRole 与变异；`Ch12TwoSided` / `Ch12Fixture` 真场景 | 是（本条只读数据，不要切片） |
| 8 | `Ch12Ledger`（门闸先后、rudian 0 处、全仓 take 0 处）；Slice Eight（对照组把锦帕放回） | 是；扫描口子见 T-4 |
| 9 | `Ch12Ledger`（binghuo_ri、两处常速、tihun 只战后拨一日）；Slice Nine（常速 +2 日；静养 30 日走 days_4） | 是；绕路那一支只有静态检查 |
| 10 | `Ch12Ledger`（三档费用链、两件无条件 give）；Slice Ten（零灵石两处各 +1 日；对照组中阶 1） | 是 |
| 11 | `Ch12Ledger`（xinjing 只拼位 1、2）；Slice Eleven ×2（幻境四版、拜师绕回） | 是 |
| 12 | `Ch12Ledger`（give 2 恰 2 处、take 在 battle 之后且看返回值、物品四个字段、learn 0 / 1 处） | 部分：五行与实战使用（T-2） |
| 13 | `Ch12Ledger`（0 店、take 灵石只在两处）；`Ch12Acceptance`（低阶差 20、中阶 20、182 日） | 是 |
| 14 | `Ch12BattleData` 调用点与目标链门；通关的 ids 与胜负序列；`doorErrors` 查 0 遭遇区 | 是 |
| 15 | `Ch12TriggerMode` Doors 与变异；Slice Fifteen（真 `tryStep`；对照组 tuizhu 之后落 taijie (15,21)） | 是 |
| 16 | 挂点表、Detour 扫描与变异；`ObjectiveTests` 五元 3 条；Slice Sixteen（三岔口字面量 (27,33)、(18,24)、(30,16)） | 是 |
| 17 | 规则 30；`NpcPresence`（`kLastChapter` 改 12、链外 8 面）；`Ch12TriggerMode` Presence 与变异 | 部分：同第 11 章，没有逐对象核笔记 H 表 |
| 18 | `Ch12Ledger` Quests 与 6 条变异 | 是 |
| 19 | `Ch12Ledger` PathActions 与 8 条变异；`PathActionTests` | 是 |
| 20 | `Ch12TriggerMode` Finale（zhuzhong 原文 687 B，FNV 与 HEAD 实算一致）；通关章节卡；`Ch11Acceptance` 改接 Opening(12)；`UiStyle` | 是 |
| 21 | `Ch12LexiconTables`：表一 84 词与负向；`Ch12Lexicon`：先后 25 行、定点 5 条、0 处 20 词、分母先验 | 是 |
| 22 | 12.2 末两行已在 `zeroWords()`；逐 key 核归独立校对 | 部分（T-8，不归测试） |
| 23 | 无 | 校对报告，不归测试 |
| 24 | 目标链 33 步、脚本 39 份、物品 27 件、图 10 张、编成 4 张都有先验；其余计数归 `tools/audit.py` | 部分，按设计分工 |
| 25 | `Ch12Engine` 23 条（0 跳过）；`Ch09Save.AllExistingFixtureAndPlayerSavesStillLoad` 读得进 `ch12-end-*` | 是 |
| 26 | `Ch12VisualWiring` ×2（真 `battleBackdropFor`、`loadMapMeta → mapVisualFrom`，带负向与配对对照） | 是 |

## 第 11 章终档重生成

用 Python 把 `git show HEAD:` 的旧版与工作树新版展平后逐字段比较。两版各 837 个字段，键集合一致。两侧都只差这 5 项：
- `mapId`：`ch11_dongfu` → `ch12_tushan`
- `position`：(14,24) → (24,31)，即 `maps/ch12_tushan.tmj` 的 `andeng`（default，朝上）
- `checksum`
- `playSecondsGameplay`：多一帧（+0.0167 s）

`facing` 原本就是 0，没有变。

新版实读：day 49378 / 49353，低阶 2339 / 2319，中阶 20，hp / mp 700 / 470，realm 31 / 31 / 22，party 恰曲魂（hp 820），法术 11 门，`ch11.done == 1`，`ch12.*` 0 面。与协调者备注、测试路 A 的摘要一致。

`zhuzhong.lua` 与 HEAD 的 diff 只有末尾一行 `teleport("ch12_tushan")`。HEAD 原文 687 B、FNV-1a 为 0x461b9e962c20c54d，与 `Ch12TriggerMode` 里的常量一致。

## 平衡用例：反证有没有牙，判据从哪来

- **判据不跟着数据走。** Ch12 的用例里没有一处读 role 的气血、攻防、架势来算期望值，也没有钉任何 role 数值。平衡只钉结局：胜、敌方清场、韩立站着、回合窗、① 的怪笑被打断。回合窗是字面量，但它的来源是调后实测（T-1 第 1 条）。
- **窗有牙。** 拿调前数据（balance.md 第 0 节）来对：①② 败，③ 5 回合，④ 29 回合，四场都会红。① 只有三首带蓄势，所以 `interrupted` 指的确实是怪笑。
- **手稳。** 四场两侧各从真实存档重打三遍，结局指纹（回合、胜负、出手记录、战后存档逐行）与通关那一趟一字不差。
- **反证。** 我开 `FANREN_CH12_BATTLE_LOG=1` 重跑，核过出手记录：
  - ② 捂住冰冻术和四门金类：败，27 回合，敌余 1162，「却伤不到」42 处；
  - ③ 韩立只防：败，40 回合，银形余 1，20 处；
  - ④ 捂住木：败，15 回合，玄骨余 1，6 处；
  - ④ 人人只防：败，24 回合，玄骨余 450；
  - 对照：同一存档、意图手照常打，三场全胜。

  去掉 killable_by 的话，③④ 就会赢，所以有牙。② 的断言偏弱，见 T-3。
- **最低合法起点。** ②③④ 两侧都胜：10 / 10 / 8 回合，韩立 700→479 / 634 / 551。这条用例的性质见 T-6。
- **法力。** 所有战后存档的 mp 都恒为 470，因为战后不回写法力。8.2 ② 说的「把法力留到第二、三波」，这只手量不出来；B 路已报。

## 实跑记录

**用的哪个可执行文件。**
- 可执行文件是 `build-ch12walk/fanren_tests.exe`，mtime 19:31:16，晚于 `tests/` 下全部源文件（最新的 `Ch12AcceptanceTests.cpp` 是 19:30:53）。`src/` 下没有比它新的文件。
- 比它新的只有 `data/`（19:33–19:37 的平衡调数与编成 note）和 `ch12-end-*.sav`，都是运行时读的，所以没有重建。
- B 路的 `build-ch12walk.log` 是 20:14，晚于这些数据，全量 1944 / 1944。
- 工作目录 `H:/Work/Kys/fanren`。

| 过滤 | 结果 | 耗时 |
| --- | --- | --- |
| `Ch12*:Ch11*` | 161 个用例、37 个套件，**全部通过**，RC=0（Ch12 82 条：Engine 23、测试路 A 40、B 19；Ch11 79 条） | 35.1 s |
| `MapLink*:NpcPresence*:Objective*:UiStyle*:Chapter*:Lexicon*:PathAction*:Ch07BattleData*:Ch09Engine*:Ch10*` | 197 个，**全部通过**，RC=0 | 16.9 s |
| `Ch09Save*:GameWiring*` | 16 个，**全部通过**，RC=0 | 0.5 s |
| `Ch12Balance.OnlyTheDesignedElements*`（开 `FANREN_CH12_BATTLE_LOG=1`） | 通过；这一趟只用来数出手记录 | — |

没有 SKIP，也没有 DISABLED。两侧通关各约 0.4 s。

## 第 2 轮复核（2026-10-13）

### 结论：Approve（score 5）

上一轮 T-1 到 T-8 全部关闭：
- T-2 到 T-8 改了用例；
- T-1 回写进了施工图 18.3、18.5、18.6。

逐条读了改后的用例，新切片与新负向没有空转。我在 scratchpad 复制树里做了两条阴性对照，都如期报错。整改没有带出 MEDIUM 及以上的新问题。剩下三条 LOW 不挡放行，都已在 18.5 写明，见下。

### 逐条复核

- **T-1 已关。**
  - 18.5 第 1–8 条把这些都登记了：回合窗及来由（含「调前四场都落在窗外」）、10 个 role 的调前 → 调后表、玄骨法力 96 的裁定与理由、意图手不吃药、quanbian 一格的落法、Slice Five 断的是函数返回值、③ 的兜底打坐在寒骊台、反证与手稳。
  - 18.3 第 2、3 条改成了现值，初值标作废。我用 Python 对过 `data/roles`：玄骨 450 / 96 / 36、三首 800 / 60 / 54、大群 500 / 40 / 42；架势 3 / 4 / 5 / 5 / 6 / 3 / 3 / 5 / 6 / 4，与 18.3、18.5 一致。
- **T-2 已关。**
  - 新切片 `Ch12Slice.TwelveTheWoodThunder…` 起点是驱动真走到的 n30 之后。神雷经真菜单放出：`openMenu → menuChoose(物品) → buildItemItems 找神雷 → buildTargetItems 找玄骨`；剑也走菜单「攻击 → 剑」。断言都是字面量：
    - 剑这一击的类别是 `kCategorySword`，玄骨仍是 1；
    - 神雷这一击的类别是 `kCategoryWood`，玄骨到 0，胜；
    - 用 2 发那一版，先放的那发也是木；
    - 战后余数分别为 2 / 1 / 0；三版都把 n31 走完，只说 `.spent`、不说 `.keep`，背包里的神雷归 0。
  - 剑版与 1 发版在「压到 1」那一刻的回合、韩立余血、出手记录逐字相同，所以对照与被测是同一个局面，只差收尾那一手，不是同源推导。
  - 这条切片没关 `audit`，n31 的逐节点账（天数、`kAccounts`、神雷 0）照样逐条核，用掉神雷的两版也过了。
  - 去掉 `if n > 0`，`take(id, 0)` 会被当成扣 1、失败，于是说 `.keep`，2 发那一版就会红。
  - 另有两道静态守卫：`Ch12Ledger` 补了 `element == 2`，配「改火」「删五行」两条负向；`validate.py::check_given_spell`（G-2）也开始核 castMagic 的五行、power 与 effect。
- **T-3 已关。** ③④ 断言敌余恰 1；②③④ 都断言出手记录里有「却伤不到」，这是引擎 `BattleAction.cpp` 拼的半句，不是文案。去掉 killable_by，三条都会红。
- **T-4 已关。** 先断言门闸、`advance_days(`、`flag.set("ch12.lidao")`、`teleport(` 四处都在，再比先后。变异表新增「删掉 teleport / advance_days(4) / flag.set」三条（`Ch12LedgerTests.cpp:187–189`）。
- **T-5 已关。**
  - 指纹的负向改成把改过回合、胜负、出手记录、战后气血的 `Fight` 喂给 `fightFingerprint`，并配一条「原样复制、指纹相同」的对照。
  - 曲魂那条抽成 `quHunErrors()`：同长改一个字节只报哈希；多一个字节，长度和哈希两条都报。
  - Slice Five 的纯函数对照照第 11 章口径保留。
- **T-6 已关。**
  - 逐节点账的战斗收支对 `kAccounts` 的每一样、每一场都断 0。新负向「① 里偷偷多给 3 块低阶、节点账也多 3 块」恰报 2 条，其中一条点名 `changed inside b12_guiwu_guiwang`；还原后一条不报。
  - 最低合法起点加了「压力版」：不静养、带余血上，并先验 ③④ 战前确实不满血，战前背包里没有 `pill_`、中阶 0。
  - 场次靠推断那一点照旧，有阵容比对兜着，接受。
- **T-7 已关。** `continue` 改成了 `ASSERT_NE`，并补了 `actions == role.actions`，以及玄骨恰 2。
- **T-8 已关。** `Ch12Ledger.TheSectionThirteenZeroWords…` 扫 `data/text/ch12*.json` 的值，六个词全章 0 处。分母先验是 > 250 条、≥ 11 份。负向逐词各塞一条，恰报那个词、那条 key。

### 阴性对照（scratchpad 复制树，仓库文件没动）

把 `data/`、`maps/`、`scripts/`、`tools/`、`assets/bgm` 复制到 scratchpad，在副本里跑 `python -I tools/validate.py`：

| 改动 | 结果 |
| --- | --- |
| 不改（对照） | VALIDATE_OK，RC=0 |
| `magic_bixie_shenlei` 的 `element` 2 → 8（火） | RC=1，2 条错误：① ④ 两场都报「施展的 magic_bixie_shenlei 五行不是『木』」 |
| `power` 70 → 0 | RC=1，① ④ 两场都报「不伤人（power 0）」 |
| 还原 | VALIDATE_OK，RC=0 |

C++ 用例的数据根取自 `__FILE__`，指不到副本；要在副本里跑就得整树重建（约 20 分钟），不算低成本，没做。切片那一半，以断言里的字面量类别和战后余数作判据。

### 剩余问题（均 LOW，不挡放行）

- **R-1**：`Ch12EngineTests.cpp:410` 的 `if (unit != nullptr) EXPECT_TRUE(unit->ally)` 仍是「缺项即过」的写法。同一函数里 `sortedAllyRoles == real.allies` 和 `allyCount == 1 + 友军数` 已经钉住了友军在场，不留洞。
- **R-2**：最低合法起点的压力版，数字与意图手那一趟相同（②③④ 700→479、619→553、553→404），因为那只手本来就不吃药、不花中阶。它证的是「拿走丹药与中阶灵石照样打得过」，18.5 第 6 条已写明。
- **R-3**：`.keep` 那一支（take 余数失败）在余数恰等于 `item.count` 时走不到，运行时永远不出现。切片拿「不说 `.keep`」反证余数为 0 时没调 take，这个用法成立，切片注释已写明。

### 实跑记录（第 2 轮）

- 可执行文件 `build-ch12tfix/fanren_tests.exe`（mtime 21:04:26），晚于 `tests/` 下全部源文件（最新的 `Ch12SliceTests.cpp` 是 21:04:08）；`src/` 下没有比它新的文件。
- 整改方日志 `build-ch12tfix.log`（21:43）：VALIDATE_OK、MAPGEN_IN_SYNC、SELFTEST_OK、ARTGEN_IN_SYNC，0 个 `warning C`，1945 / 1946，唯一一红是 `PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters`。
- 工作目录 `H:/Work/Kys/fanren`。

| 过滤 | 结果 | 耗时 |
| --- | --- | --- |
| `Ch12*` | 84 个用例、17 个套件，**全部通过**，RC=0；比上一轮多 2 条（Slice Twelve、Ledger 第 13 节零处词），另有 3 条改名 | 10.8 s |
| `Ch11*:Ch10*:MapLink*:NpcPresence*:Objective*:UiStyle*:Chapter*:Lexicon*:PathAction*:Ch07BattleData*:Ch09Save*:GameWiring*` | 272 个，**全部通过**，RC=0 | 20.6 s |

整改方日志里那一红，我这一轮实跑已经转绿：`data/text/ch12_path.json` 在 21:45 被文案整改路改过，晚于日志。归因是文案整改路，与测试路无关。没有 SKIP，也没有 DISABLED。

## 协调者收尾（2026-10-13，第 2 轮剩余 LOW）

- **R-1 已关**：`tests/Ch12EngineTests.cpp` 的 `expectRealSetupAllies` 里 `if (unit != nullptr) EXPECT_TRUE(unit->ally)` 改成先 `ASSERT_NE(unit, nullptr)` 再断 `ally`，缺项不再即过。
- **R-2、R-3**：照复核意见不改——R-2 的说明已在施工图 18.5 第 6 条；R-3 的 `.keep` 那一支按构造走不到，切片的用法成立。
