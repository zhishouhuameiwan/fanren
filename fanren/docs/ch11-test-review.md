# 第 11 章测试质量独立审查

2026-10-13。审查范围：`tests/Ch11*.cpp`、`tests/Ch11*.h`（8 个文件，3270 行），以及本章改过的 11 个旧用例文件和两份 `ch10-end-*.sav`。本审查只读不改，唯一写入的就是本文件。

## 结论：RequestChanges（score 3）

主干是扎实的：
- 两侧通关都从真实的第 10 章终档起步，经 `continueJourney` 进入；
- 终点断言基本都是施工图写死的字面量；
- 两侧起点和终点确实不同；
- 绝大多数新检查都配了「改坏 → 报错」的负向；
- 第 10 章终档重生成正确；
- 实跑全绿。

不能放行的原因有三条：
- 验收第 22、23 条要求的 12.2 专项断言一条都没有（T-1）；
- 验收第 17 条的两处运行时切片缺失（T-2）；
- 有 5 处判据偏离施工图原文，测试本身做得对，但施工图第 18 节没有登记，需要协调者拍板（T-3）。

## 问题清单

### T-1 HIGH：验收第 22、23 条的「12.2 专项」没有任何用例

`LexiconTests.cpp` 这次只给表一补了 42 个词，管的是「章级首见」。施工图 12.2 那张表要求的专项断言，全仓找不到对应用例，具体有四类：
- **章内先后**：按目标链次序给文案 key 的前缀编号，约 40 行规定。
- **计数**：「虚天残图」「虚天殿」各恰 1 处，且都在 `wuchou.*`；分母先验要 > 250。
- **全章 0 处**：0 处清单里有些词没有任何表在拦，包括白玉蜘蛛、隆兴客栈、元磁神光、十八天都尸、天音门、交换会、紫罗极火、黄龙（要放行「黄龙丹」）、猎妖令，以及作二转年数讲的「三十多年」「十余年」（要排除「二十余年」）。另有一处更隐蔽：天晶木在表一里登记的首见是 ch399，所以本章对它是放行的，可施工图要求本章 0 处。
- **定点**：「鸡冠蛟」只许出现在 `youyao` / `jiguan` 支与 ② 开场白；`ch11.block.xingkong` 不含「星空殿」「传送」。

我用 Python 实读核过：当前 `data/text/ch11*.json` 共 414 条，两个「虚天」词都只在 `ch11.wuchou.map`，0 处清单全部是 0（「十余年」那 1 处是 `peizhu.close` 里的「二十余年」）。也就是说**数据眼下合规，但没有任何东西防回归**。第 5–7 章有先例（`Ch05LexiconTests` 等）。

要补：一个 `Ch11LexiconTests`，覆盖上面这些断言，并给每类配一条注入负向。

### T-2 MEDIUM：验收第 17 条切片缺两处

`Ch11TriggerMode` 静态核了门对、闸门旗标和 deny key。切片里只有「huicheng 之后到不了凝翠岛」这一处（Slice Nine）。下面两处缺失：
1. `ch11.zufu` 未置时，真 `tryStep` 踩 `portal_shangshan` 应该被拦下，并说出 `ch11.block.dongfu`。
2. `ch11.liugen` 置位后，`trigger_guitai` 和 `trigger_jiesuan` 应该各只说一句：栈顶是 `DialogueScene`、不开 `ShopScene`、低阶灵石不变。`jiesuan.lua` / `guitai.lua` 的 liugen 早退，现在没有任何用例钉住。

要补：一条切片，用 Slice Eleven 的 liugen 存档；对照组是 liugen 之前，guitai 会开 `ShopScene`。

### T-3 MEDIUM：判据偏离施工图原文，未登记，需协调者拍板

以下 5 处都核过脚本或数据，测试的写法与实际行为一致。但施工图第 15 节 / 10.1 的原文没改，第 18 节也没登记。照「判据抄施工图原文」的纪律，必须由协调者裁定后回写：
1. **霓裳草买 2、章末剩 1、低阶 +2085**（`Ch11Driver.h:461`、`:635`）。属实：`youyao.lua` 第 4 行先判草 ≥ 1，② 那一支同样要过这道门。要么改施工图第 2 条（买 2、章末草 1），要么让 ② 那一支不判草。
2. **开店当日回访 `jiesuan == 0`**（`Ch11SliceTests.cpp:366`）。验收第 13 条写的是 1，3.2 n28 写的是「n == 0 → 一句 return」；`jiesuan.lua` 第 6 行照 3.2 实现。
3. **`kOffChainFlags` 少登了 `ch11.jiguan`**（`NpcPresenceTests.cpp`）。10.1 T2 要求登 8 条，实际登了 7 条。注释给的理由（`offChainProblems` 按脚本原文的先后判，而 ② 分支写在 lieyao 累加之前）成立，但这是对 T2 的偏离，没有替代断言。
4. **`MapLinkWalk` 换了同图门的判据**。施工图 18.4 第 6 条写的是「待办，地图路与测试路共同定」，测试路自行定了新判据。新判据合理，跨图门那条一字未改，同图门配了一条对照和两条负向。需要协调者追认，并回写 18.4。
5. **`Ch10Driver.h:520–521`** 在白名单外。改成 `ch11_fangshi` (28,38) 是对的：我核过 `ch11_fangshi.tmj` 的 `chengmen` spawn 就是 (28,38)，朝向 up，default。契约 8.3 漏列了这一处，需要补进契约。

### T-4 LOW：扫描判据有「找不到就算过」的口子

`Ch11SliceTests.cpp` 第 47、54 行用 `at(A) < at(B)` 判先后。B 找不到时 `npos` 最大，于是恒真：
- 删掉 `local lieyao`，或删掉 `flag.set("ch11.jiguan")`，扫描都照样通过。

目前有切片和通关兜着（lieyao 只在胜后加、② 逃后 `jiguan == 1`），所以只是 LOW。要补：两侧先断言 `!= npos`，再加两条「删掉 B → 报」的负向。

第 62–69 行验收第 10 条的「take 看返回值」只要求同一行出现 `if` / `and` / `elseif`。`if x then take(...) end` 这种不看返回值的写法也能通过，判据偏弱。

### T-5 LOW：有两处「对照」不是真对照

- `Ch11SliceTests.cpp:386` 的 `EXPECT_NE(15 * (9000 / 30), 3600)` 是常量比较，恒真，不抓任何东西。真正抓封顶的是扫描负向 `n > 300`，这一行可以删掉，或换成「改封顶 → 切片报」。
- Slice Five 第 176 行的对照只调了纯规则函数，没有在面板上做扰动。验收第 5 条「面板下一关写『结丹中期』所需 5000」没有断言文字：B 路说明引擎被剧情上限挡住时只给通用灰字。这是引擎或施工图的取舍，同样要登第 18 节。

### T-6 LOW：驱动自证

- `choose()` 排进去的选项没有在节点末断言已经用完。好在所有选择都是 0 号，语义由终点旗标兜着（zhihuan 1、luer 1、waihai_tu 1、z1 2、z2 1、huicheng）。
- 终点坐标 `kEndCell` (14,24) 是驱动自己选的站位：n31 脚本不挪人，所以 `position == kEndCell` 只证明驱动走到了那里。施工图没给坐标，建议在第 18 节写死 (14,24)。
- `jiesuanVisits_ == 2` 是驱动自己的计数器，`ch11.jiesuan == 2` 才是真判据，前者可删。
- 逐节点账里，「战斗收支」取的是战前战后存档之差，等于从被测物推导。不过妖丹掉落已由 BattleData（rate 100、×1）和 Slice Eight（四胜 +4，字面量）钉住，没有留出洞。

## 覆盖表（施工图第 15 节 26 条）

| # | 用例 | 钉住了吗 |
| --- | --- | --- |
| 1 | 构建门禁（B 路 `build-ch11walk.log` 01:49，`EXITCODE=0`、四道门禁全绿） | 是（门禁本身） |
| 2 | `Ch11Walkthrough` ×2、`Ch11Acceptance.BothSides…`，负向 3 条（起点、终点、逐节点） | 是。起点和终点是字面量，起点经 `continueJourney`；与原文有偏差，见 T-3 第 1 条 |
| 3 | `Ch11TriggerMode` Every/SameCell 加两组 Mutations | 是 |
| 4 | `Ch11BattleData` 加 14 条变异；`Ch11Balance` 三条（手稳、达标、负向，④ 人人只防必输） | 是 |
| 5 | SliceScan 加 Slice Five | 部分：面板「结丹中期」文字未断言（T-5） |
| 6 | SliceScan 加 Slice Six&Seven（三版背包） | 是 |
| 7 | 同上（day +21600，游玩秒数 0 < Δ < 5）；`Ch11Fixture` 21600 日 | 是 |
| 8 | SliceScan 加 Slice Eight（草 0、① 逃、五轮、第六轮） | 是；有扫描口子（T-4） |
| 9 | SliceScan 加 Slice Nine | 是 |
| 10 | SliceScan 加 Slice Ten（n02、n04、n18、n26 加对照） | 是；扫描判据偏弱（T-4） |
| 11 | SliceScan 加 Slice Eleven（0 / 1000 / 2000 三版，金雷竹 6，炼器台） | 是 |
| 12 | `Ch11Ledger` Sword 加变异；`Ch11ForgePanel`；通关 n30 走真面板 | 是 |
| 13 | SliceScan 加 Slice Thirteen | 是；开店当日偏离原文（T-3 第 2 条） |
| 14 | `Ch11Ledger` Avatar 加变异（哈希已对 HEAD 实算核过） | 是 |
| 15 | `Ch11Ledger` Shops 加变异；通关账（中阶两侧 20、低阶差 20） | 是 |
| 16 | TriggerMode BattleCall 加变异；通关 ids 和胜负序列 | 是；目标链那半没有负向 |
| 17 | TriggerMode Doors 加变异；Slice Nine | 部分（T-2） |
| 18 | 规则 30、`NpcPresence` 通用用例 | 部分：没有逐对象核 G 表；`jiguan` 未登（T-3 第 3 条） |
| 19 | `Ch11Ledger` Quests 加变异 | 是 |
| 20 | `Ch11Ledger` PathActions 加变异；`PathActionTests` | 是 |
| 21 | TriggerMode Finale 加变异（哈希已对 HEAD 实算核过）；通关章节卡；`Ch10Acceptance` Opening(11)；`UiStyle` | 是 |
| 22 | `LexiconTests` 表一 | 部分：12.2 专项、计数、分母都缺（T-1） |
| 23 | 无 | 否：12.2 专项缺（T-1）；独立校对不归测试 |
| 24 | 无（校对报告） | 不归测试 |
| 25 | `ObjectiveTests` / TriggerMode 只钉了目标链 29 步 | 其余计数归 `tools/audit.py`，不归测试 |
| 26 | `Ch11Engine`（E1 guaranteed、21600 日）；`Ch09Engine` 全量读取 fixtures 和 saves | 是 |

## 第 10 章终档重生成

用 Python 逐字段比对 `git show HEAD:` 旧版与工作树新版，两份都只差以下几项，其余字段完全相同（键集合一致）：
- `mapId`：`ch10_xiaohuan` → `ch11_fangshi`
- `position`：(50,39) → (28,38)
- `facing`：3 → 0，即 `chengmen` 的 up
- `checksum`

`ch11-end-{first,second}.sav` 实读结果：`day` 49378 / 49353，两侧减去起点都是 33375；`realm` 31；站在 `ch11_dongfu` (14,24)。`zhifadui.lua` 等于 HEAD 原文（633 B，FNV 与测试常量一致）后接一行 `teleport("ch11_fangshi")`。

## 实跑记录

可执行文件 `build-ch11walk/fanren_tests.exe`，mtime 01:24:16，晚于 `tests/`、`src/`、`scripts/`、`data/`、`maps/` 下的全部文件。工作目录 `H:/Work/Kys/fanren`。

| 过滤 | 结果 | 耗时 |
| --- | --- | --- |
| `Ch11*:Ch10*` | 133 个用例，34 个套件，**全部通过**，RC=0 | 11.0 s |
| `MapLink*:NpcPresence*:Objective*:UiStyle*:Chapter*:Lexicon*:PathAction*:Ch07BattleData*:Ch03BattleKit*:Ch09Engine*` | 137 个用例，**全部通过**，RC=0 | 7.6 s |

两侧通关各约 0.25 s，与第 10 章同量级。Ch11 共 66 个用例，全部 OK，没有 SKIP 或 DISABLED。

## 第 2 轮复验（2026-10-13）

### 结论：Approve（score 5）

上一轮 6 条全部关闭：T-1、T-2、T-4、T-6 改了测试，T-3、T-5 在施工图 18.5 第 12、14 条登记。整改没有带出 MEDIUM 及以上的新问题。剩下 3 条 LOW 不挡放行，见下。

### 逐条复核

- **T-1 已关。** 新增 `tests/Ch11LexiconTests.cpp`，共 8 个用例：
  - 节点表是字面量，按 3.1 的 n 号乘 2 编号，查不到节点的 key 直接判红。
  - 先后 76 词，每词带正向，防「词被删光、只剩恒真的 0 处检查」。
  - 定点 15 条，其中两个虚天词各恰 1 处，且必须同在一句。
  - 0 处 22 词，另加两条按词边界判的：「黄龙」放行「黄龙丹」，「十余年」放行「二十余年」。
  - 分母要求 > 250 条且至少 11 份文件；章节卡单独一条。
  - 负向是在真实文案上注入的，C-11、C-12 两处旧文的回放也在里面。我用 Python 核过注入点：`zhihuan.greet` 被替换后，「红色指环」仍在 `zhihuan.rule` / `.red` 里，所以这条负向抓的确实是「鲁二早说」，不是误伤别的词。
  - 与 12.2 逐行对过：除 R-1 列的几处，断言都在，从严的地方（剑影分光术恰 1、七十二在 lianjian 之前 0 处）与现有数据相容。
- **T-2 已关。** 补了两条切片。
  - `Seventeen…MountainGate…` 走真 `tryStep`：踩门不换图，栈顶对白恰是 `ch11.block.dongfu`。对照组租府之后过同一扇门，换到 `ch11_dongfu`。
  - `Seventeen…ShopIsSold…` 的对照组在 liugen 之前，柜台开出 `ShopScene`。liugen 之后，柜台和结算格各只说一句：spokenKeys 恰为单元素，不开店，低阶、`jiesuan`、day 都不变。
- **T-3 已登记**（18.5 第 12 条，注明「协调者复核」）。第 1–4 条照实际行为登记，`kOffChainFlags` 写了替代判据。第 5 条（`Ch10Driver.h` 520–521 补进契约 8.3）还没做：`interfaces-p3-ch11.md` 里没有 `Ch10Driver`。见 R-2。
- **T-4 已关。** 新增 `ordered()`，两头都必须找得到。补了三条「删后一头」的负向（`local lieyao`、`flag.set("ch11.jiguan")`、`bottle.drops()`）。「take 看返回值」改为 take( 必须落在同一行 if / elseif 与 then 之间，配了一条「挪到 then 之后」的负向。C-7 的新扫描也配了两条负向（删除那一行、挪到扣绿液之后）。
- **T-5 已关。** 恒真的常量比较删了。Slice Five 加了一条对照：上限抬到 32 时不再被 StoryCap 挡。这条对照仍是纯规则函数，没有在面板上做扰动，但「面板不写下一关名字」已在 18.5 第 14 条登记，接受。
- **T-6 已关。** `runTo` 每个节点结束时都断言选项队列已清空，没有排到的选择会以 `Unplanned choice` 失败。`jiesuanVisits_` 删了。(14,24) 已在 18.5 第 15 条写死。
- **C-7 切片**：`ElevenNoSecondRootGrowsBeforeTheShopIsBought` 起点写明，断言 `shop_first` 那句话说了，zhu_gen、day、绿液、金雷竹都不动。对照组盘店之后到 2。
- **第 10 章终档**：自上一轮起没有再动过（mtime 00:32），不重核。`ch11-end-*.sav` 没有重生成，两侧通关在写开关关闭时与它们比对全绿。

### 剩余问题（均 LOW，不挡放行）

- **R-1**：12.2 有四处定点没有断言：
  - 「妖海」只在典籍那一句与 K1；
  - 「天星双圣」只在典籍、传音符、zhuzhong；
  - 「年俸」结算 0 处；
  - 「劫货」在 zhuzhan 之前只许 `chaguan.*`。

  Python 实读：当前数据都合规（妖海只在 `chudao.book_sea`，天星双圣只在 `chudao.book_gong`，年俸只在 `zhanglao.offer`，劫货在 `chaguan.robbed` / `zhuzhan.wang` / `guiju.zhao`），只是没有东西防回归。往 `onlies()` 里加四行即可。
- **R-2**：契约 8.3 漏列 `Ch10Driver.h` 终点断言这一处（上一轮 T-3 第 5 条），仍待协调者补。
- **R-3**：先后表的「头一回在哪一节」那一列（例如 传送符 S(9)、妙音门 S(20)、卓如婷 S(25)），施工图 12.2 没有给出，是照现有文案填的。它只用来防「词被删光」，不会把违规放过去，但文案正当改写时会误报。建议注释写明这一列的来历，或放宽成「至少出现 1 处」。

### 实跑记录（第 2 轮）

可执行文件 `build-ch11fix/fanren_tests.exe`（mtime 02:49），`tests/`、`src/`、`scripts/`、`data/`、`maps/` 下没有比它更新的文件。日志 `build-ch11fix.log`（02:55）末尾为 `100% tests passed, 0 tests failed out of 1862`、`EXITCODE=0`。工作目录 `H:/Work/Kys/fanren`。

| 过滤 | 结果 | 耗时 |
| --- | --- | --- |
| `Ch11*:Ch10*` | 144 个用例，37 个套件，**全部通过**，RC=0 | 12.4 s |
| `MapLink*:NpcPresence*:Objective*:UiStyle*:Chapter*:Lexicon*:PathAction*:Ch07BattleData*:Ch03BattleKit*:Ch09Engine*` | 137 个用例，**全部通过**，RC=0 | 10.2 s |

Ch11 共 79 个用例，比上一轮多 13 个。其中我逐条核过的是 Lexicon 8 条和 Slice 3 条，另 2 条没有逐条辨认。79 个全部 OK，没有 DISABLED。
