# 第 13 章测试质量独立审查

2026-10-14。审查范围：
- `tests/Ch13*.cpp`、`tests/Ch13*.h`，9 个文件，5434 行。其中 `Ch13EngineTests.cpp`（877 行）归引擎路，只扫了「缺项即过」与「跳过」的写法。
- 本章改过的 10 个旧用例文件（`git diff -- fanren/tests`，+220 / −43），以及两份重生成的 `ch12-end-*.sav`。
- 两份新写的 `ch13-end-*.sav`。

本审查只读不改，唯一写入的就是本文件。口径照 `docs/handoff.md` 第 6 节与 `docs/ch12-test-review.md`。

## 结论：Approve（score 5）

主干扎实：
- **起点真实。** 两侧都从 T1 重生成版 `ch12-end-{first,second}.sav` 起步。`Driver::load` 先验落在 `ch13_nanming` (6,29)，再逐项验验收第 2 条的 SetUp：
  - day 49560 / 49535，低阶 2331 / 2311，中阶 20；
  - hp 614，mp 470，修为 4072 / 4011，realm 31 / 31 / 22；
  - party 空，恰 12 门法术；五级、六级妖丹都是 0，金丝蚕 3，霓裳草 1；
  - 腰牌、肋骨、炼晶、青竹蜂云剑各 1。

  之后经 `continueJourney` 进入，断言读档前后逐行相同。
- **入口真实。** 全程走真实入口：
  - `WorldScene::tryStep / interact` 与脚本 teleport；
  - 脚本 `shop()` 开出的 `ShopScene`，真按 `leave`；
  - `openFacility` 开出的 `CultivationScene`，真按「闭关一年」与冲关；
  - 真 `BattleScene`。
- **终点是字面量。** 逐项抄验收第 2 条：
  - realm 32 / 32 / 22，`realmMaxHp / realmMaxMp(CoreMid)`，恰 14 门法术；
  - 四面旗标：lieyao 2、toudu 3、zhiyuan 2、tongpian 1；
  - 低阶 `side.low + 600`，中阶 `side.mid`，六级 +7，五级 +13，霓裳草 +1；
  - 新 story 八件各 1，耗掉的五件各 0；
  - `day − side.day == 11790 + 360 × N`，N 是每侧写死的字面量 2；
  - 落在 `ch13_waihai` (60,44)，朝东。
- **检查自带负向。**
  - 起点守卫换成另一侧的字面量，恰报 3 条（day、低阶、修为）。
  - 终点 17 处单点变异，各恰报 1 条且点名。
  - 逐节点账 3 处变异，各恰报 1 条；「仗里偷偷多给 3 块低阶」恰报 2 条，还原后 0 条。
- **两侧真不同，但本章走同一条分支。**
  - 起点差 25 日、20 块低阶、61 点修为。两份 `ch13-end` 有 250 个字段不同，本章 38 面旗标完全相同。
  - `scripts/ch13/` 40 份脚本 0 处读第 1–12 章的旗标，所以两侧在本章走的是同一条分支，只差数值。冲关种子随 day 与修为变，两侧都是 N = 2。
  - 这是「两侧同一组选项」的设计（验收 2、第 9 节），不算问题。
- **第 12 章终档重生成只差地图与坐标**（见下文）。第 12 章用例没有放宽平衡与回合窗。
- **反证有牙。** 本审查开出手记录重跑，三场反证里「却伤不到」各出现 3 / 20 / 1 处；不捂时同一存档全胜。
- **实跑全绿。** 本审查跑了 166 + 207 个用例，全过。B 路全量日志 2028 / 2028，四道门禁全过，0 个编译警告。

没有 CRITICAL / HIGH。三条 MEDIUM 要协调者处理：
- T-1：施工图回写；
- T-2：回合窗代码与记录不一致；
- T-3：五行环从没在运行时放过。

四条 LOW 不挡放行。

## 问题清单

### T-1 MEDIUM：测试路的判据与调数还没进施工图第 18 节，18.3 的表还是调前的数

第 18 节现在只有 18.1–18.4（内容路 C、A、B，集成），测试路 A、B 的偏差一条都没进。下面几处，施工图原文要么没写，要么与现状相反：

1. **18.3 第 6 条的数值表还是调前值。**
   - 表里写：儒生气血 480；海兽法力 54、攻 34；巨虾气血 820、攻 36。
   - `data/roles/*.json` 的现值：360；18、30；700、30（balance.md 第 1 节）。
   - 测试不钉 role 数值，所以不会红；但下一个照 18.3 改数的人，会把平衡改回去。这与第 12 章 T-1 第 2 条是同一个坑。
2. **坐关次数。** 8.1 写「约 6 次以内」，并注明它是「本章坐关次数的唯一权威」。实测 N = 2 / 2（`Ch13Driver.h` 的 `kFirst` / `kSecond`）。协调者备注已裁定按 2 次，8.1、1.3、3.3、第 14 节都还没改。
3. **回合窗**（见 T-2）与「① 蓄势至少被打断一次」：两条判据都按实测定，8.2 没给回合数，要登记。
4. **意图手的取舍**（`Ch13Driver.h:42–47`）：不吃药、不按五行环。8.1 没写这两条，而 8.2 ①「先锁一个」写的正是五行环。
5. **验收 5 的种子。**
   - 切片在 day 53941 冲关不成，在静室坐 1 日、到 53942 才成。原因是 `breakthroughSeed` 取 day、修为、境界的哈希。验收 5 写的是「固定一颗冲得成的种子」，要写明这一颗是怎么定的。
   - 「面板下一关报 StoryCap」断的是 `disabledReason` 与函数返回值，不是面板上的字。第 11、12 章同况，照样登记。
6. **G-31 被钉成了现状。** `Ch13AcceptanceTests.cpp:75` 断言读档后 `facing == 2`（朝下），而章末人实际朝东。将来修了 G-31，这一行就会红。注释已写明，18 节也要写一句。
7. **测试路 A 报的措辞口径**，以及几处切片口径，都还没登记：
   - 「`shop(` 恰 4 处」实为 4 个脚本、6 处调用；
   - 12.2 未编号类的补编口径；
   - `caigou.lua` 首句判 `ch13.wenxun` 是死代码；
   - n29「回话」取第一支；
   - 验收 6、12、13、27 的切片起点。

改法：
- 协调者把 `notes/ch13/balance.md` 第 0–5 节、`tests-summary.md` 的「施工图要改」与上面 1–7 条并进第 18 节，新开 18.5「测试路」。
- 改掉 18.3 第 6 条的旧数，初值标作废。

### T-2 MEDIUM：回合窗代码是 6–16 / 6–16，记录是 10–22 / 6–18；① 实测 16 回合，正卡在上沿

- **代码与记录对不上。** `Ch13AcceptanceTests.cpp:35` 写的是 `kTargets{{{6, 16}, {6, 16}}}`；`notes/ch13/balance.md` 第 0 节与 `tests-summary.md` 都写「① 10–22、② 6–18（实测放宽约四成）」。
  - ① 的下沿，代码比记录松：6 对 10。
  - 上沿，代码比记录紧：① 16 对 22，② 16 对 18。
- **① 零余量。** ① 两侧实测都是 16 回合，等于上沿，「放宽约四成」不成立。手是稳的（三遍指纹一字不差），眼下不会抖红；但任何让 ① 多一回合的改动都会红。另一头，下沿 6 又放得太宽。
- **两版窗的牙不一样。** 拿调前数据（balance.md 第 0 节）对：
  - ② 第二轮调前是败，两版都红；
  - ② 第一轮调前胜 14 回合，两版都在窗内；
  - ① 调前胜 19 回合、韩立只剩 75：**代码版红，记录版（10–22）绿**。

  平衡路调 ① 的理由是「险胜」，可断言里只有代码版那个 16 的上沿兜着它：`onTarget` 对韩立只要求 `heroHpLeft > 0`，没有一条量「险胜」的判据。

改法：
- 两版里拍板一版，写进 18 节。
- 把「不险胜」直接写成判据：① ② 各加韩立余血下限。调后实测余血 312 / 343 / 283，下限可取 150 左右。这样回合窗就能按实测放回四成。
- `EachTargetRejectsAFightThatMissesIt` 补一条「余血跌破下限」的负向，再补一条：把调前 ① 的结局（19 回合、余 75）构造成 `Fight` 喂给 `onTarget`，必须判不达标。

### T-3 MEDIUM：五行环从没在运行时放过一发；② 巨虾的蓄势两侧两轮都没被打断

- **本章的头号新手段，运行时出手 0 次。**
  - 第 7 节「两样新手段」与 8.2 表头都说：这两场仗是「古宝之威」与「重炼之剑」的说明书；① 的意图是「五行环一下打穿儒生的架势」。
  - 可是 `BattleHand::scoredOptions` 只列 power > 0 或带毒的法术。五行环 power 0、effect stagger，这只手永远不按它（`Ch13Driver.h:42–47` 自己写明了）。
  - `tests/` 里 `magic_ji_wuxinghuan` 只出现在三处：静态检查（`Ch13BattleData` 钉金、stagger 3、power 0；`Ch13Ledger` 钉全仓 learn 一处），以及终点的法术表。
  - 我开 `FANREN_CH13_BATTLE_LOG=1` 重跑 `OnlyTheDesignedKinds…`，18 段出手记录里「五行环」出现 0 次，「蜂云合剑」出现 22 次。
- **不知道它在菜单里出不出得来。**
  - 没有一条用例经真菜单（`menuChoose(法术) → 选五行环 → 选目标`）用过它。
  - 也没有断言儒生的架势 3 会被它一下削到 0、进入破势。
  - stagger 这条引擎路径，只有第 7 章的通用用例走过。
- **② 的「靠五行环削势或剑破势打断」也没兑现。**
  - 两侧四场 ② 的 `interrupted` 全是 0，巨虾的全体重招照落（第一轮两次，balance.md 第 2 节）。
  - 平衡目标没要求 ② 打断，所以不算错；但施工图这句意图在运行时没有一条证据。测试路 B 已建议登技术债（与 G-36 同类）。

不挡放行的理由：
- 胜负不靠五行环。两场都由剑与木收尾，`OnlyTheDesignedKinds` 已证，不会出死局。
- 五行环的数据有静态守卫；施展门槛由 validate 的 `check_learned_castable` 管（第 13 章起点 CoreEarly / 470）。

改法（照第 12 章 T-2 的整改样式）：
- 加一条切片，起点 `reachedAfter(walk, 4)`：
  - 踩出海口带进 ①，第一手经真菜单对儒生放五行环；
  - 断言这一手类别是金、儒生架势 3 → 0（破势）、法力 −30；
  - 下一手经真菜单用剑打儒生；
  - 对照组：同一局面改放一发祭金蚨，架势只减 1、不破势。
- 给 ② 第一轮补一版：巨虾第 1 波现身后先放五行环，断言削 3；看接下来那一发蓄势有没有被打断，结果记进 18 节。
- 技术债登一条「BattleHand 不按 effect 类法术」。

### T-4 LOW：两处常量比较或自数入判据

- `Ch13Driver.h:677` 的 `EXPECT_LE(side.retreats, kRetreatCeiling)` 两边都是字面量（2 ≤ 6），恒真，是一处常量比较。验收第 2 条要求「断言 N ≤ 6」，照字面落了，但它不是运行时检查。改成 `static_assert`，意图更直白。
- `Ch13Driver.h:728`：逐节点账里 n28 的日数取 `kRetreatDays * retreats_`。`retreats_` 是驱动自己数的按键次数，它进了断言右边，而验收第 2 条写的是「不进断言右边」。
  - 眼下有 `checkEnding` 兜着：字面量 N 进了日数断言，另有 `retreats_ == side.retreats`。
  - 改法：`checkEntry` 带上 `Side`，n28 改用 `side.retreats`。

### T-5 LOW：账里两处先后比较缺项即过

`Ch13LedgerTests.cpp` 第 111、174 行用 `at()` 比先后：`toudu` 在 `chuansong` 之前、`tongpian` 在 `jiaohuan` 之前。`at()` 找不到时返回 `size()`。

- 前一项（toudu、tongpian）有 `onlyIn` 保证在；后一项（chuansong、jiaohuan）没人保证。
- 所以删掉 `flag.set("ch13.chuansong")` 或 `flag.set("ch13.jiaohuan")`，这两条照样通过。
- 通关会在 n12、n31 的 done_flag 上红，所以只是 LOW。

改法：先断言两处都在，再比先后；变异表补「删掉后一项」各一条（同第 12 章 T-4）。

### T-6 LOW：最低合法起点里，两版与意图手那一趟同数

`TheMinimumLegalStartStillWinsBothFights` 里有两版，打出来的数与意图手那一趟完全相同：① 700→312，② 第二轮 553→283。
- 一版是 ① 只拿走丹药与中阶灵石；
- 另一版是 ② 第二轮带着残血直接上。

原因是那只手本来就不吃药、不花中阶灵石。真正加压的是另外两版：① 气血压回 614；② 在静室坐十日之后再打。这与第 12 章 R-2 同况，在注释里写明即可。

### T-7 LOW：两条「抹回 0」的切片不空转；配套的静态检查漏看设施

被问到的两条：验收 6（`Ch13SliceTests.cpp:163`，把 `ch13.rucheng` 抹回 0）与验收 27（`:578`，把 `ch13.daoqiyuan` 抹回 0）。

- **不空转。** 两条都配了只差这一面旗的对照：
  - 验收 6：同一时刻 rucheng = 1 时，开得出 Field / Cultivation 两个场景；
  - 验收 27：不抹旗，先找明夫人、再找离寻，玉简到手。

  按代码推了一遍，两条都有牙：
  - 把 `facility_*_39` 的 `require_flag` 删掉，或改成 n07 之前已置的任一旗，抹旗那一支就开得出场景，会红。切片另外直接钉了 `require_flag == "ch13.rucheng"`。
  - 把 `mingzhu.lua` 的前置删掉，或改判 `ch13.chuansong`，抹旗那一支就会置 `ch13.mingzhu`，会红。
- **造出来的状态玩家到不了，这是对的。**
  - 切片证的是「旗没置时闸是关的」。
  - 「不跨带就站不到闸前」由 `Ch13TriggerMode` 的 `layoutErrors` 静态证：从 spawn 出发不跨 n07、n13 的带，够不着任何 trigger / npc / portal。
  - 两半合起来，才是验收 6、27 的原意。
- **小缺口。**
  - `layoutErrors` 第 368 行只看 trigger、npc、portal 三类，不看 facility，所以两处 `_39` 设施在不在 rucheng 带之内，没有静态检查。
  - 两处设施自带 `require_flag`，这个缺口不会把人放进去。
  - 改法：类型过滤加上 facility，排除紧挨 spawn 的那个存档点。
- 另：验收 27 抹旗那一支里对离寻的断言，与不抹那一支相同（离寻的前置是 mingzhu，两支都是 0）。冗余，但无害。

## 覆盖表（施工图第 15 节 27 条）

| # | 用例 | 钉住了吗 |
| --- | --- | --- |
| 1 | B 路 `build-ch13walk.log`（04:49）：四道门禁全过（MAPGEN 79 张、ARTGEN 721 个），0 个 `warning C`，2028 / 2028，EXITCODE=0 | 是（门禁本身） |
| 2 | `Ch13Walkthrough` ×2；`Ch13Acceptance` 3 条（两侧差、起点负向、终点 17 处与节点账 4 处变异） | 是；N ≤ 6 是常量比较（T-4） |
| 3 | `Ch13TriggerMode` 挂点表、布局、设施，各配变异；40 份脚本各恰挂一处 | 是；设施不在带检查里（T-7） |
| 4 | `Ch13BattleData` 20 种变异、目录与两个 role 的字节；`Ch13Balance` 5 条 | 是；五行环只有静态（T-3） |
| 5 | `Ch13Ledger`（realm.cap 一处且看返回值，advance / demote 0 处）；`Finale`（chuguan 首句）；Slice Five（修为 4000 踩两次；5000 冲成、冲不成两版；上限 31 的对照） | 是；面板文字断的是函数（T-1 第 5 条） |
| 6 | Slice Six（同一块田、采收带年份、抹 rucheng 两处都拦、读图与文案）；引擎 `Ch13FieldFixture` | 是（T-7） |
| 7 | `Ch13Ledger` 的 chuansong 部分；Slice Seven 七行（用越界取消证明列了几项） | 是 |
| 8 | `Ch13Ledger` 的 youyao、chongmo 部分；Slice Eight（四条安静分支；胜 / 逃；lieyao 1、2 收阵石） | 是 |
| 9 | `Ch13Ledger`（三处 take；两处 learn 全仓各 1；金丝蚕按余数）；Slice Nine（删炼晶、金丝蚕 0、意图对照） | 是 |
| 10 | `Ch13Ledger` 的 recipe、gongsun 部分；Slice Ten 四份（含三支都列时选开炉的对照） | 是 |
| 11 | `Ch13Ledger` 的 mishi 部分；Slice Eleven（零钱只列支三、越界取消、意图对照） | 是 |
| 12 | `Ch13Ledger`（shouchao 全仓 1 处；店各归一个脚本）；`TriggerMode`（在场、0 个 shop 设施）；Slice Twelve（兽潮前后两版） | 是 |
| 13 | `TriggerMode` 往返（6 种变异，含 biguan → chuguan）；Slice Thirteen（去 / 不去，biguan、lidao 两处关闭） | 是 |
| 14 | `Ch13Ledger` 四家店、T1–T7；`BattleData` spirit_stones 0；`Acceptance` 两侧差 20 / 0 | 是 |
| 15 | `BattleData`（battle 调用 2 处、门闸、5 种变异 ＋ 注释对照）；通关的 ids 与胜负序列；`TriggerMode` 0 遭遇区 | 是 |
| 16 | `TriggerMode` 门（先验旧图 ≥ 75 张）；`ObjectiveTests` kScriptTransfers 7 条 | 是 |
| 17 | `TriggerMode` 在场（22 个对象逐条；同时在场的名牌；7 种变异 ＋ 1 个对照）；`NpcPresence`（kLastChapter 13、链外 7 面） | 是 |
| 18 | `Ch13Ledger` 路径行动（13 条字面量、9 种变异 ＋ 撤场对不上）；`PathActionTests` {13, CoreMid} | 是 |
| 19 | `Ch13Ledger` 支线（5 条、8 种变异） | 是 |
| 20 | `TriggerMode` 章末（banyao 末句、9 种变异 ＋ 对照）；通关章节卡 Closing 13 → ToBeContinued；`UiStyle` | 是 |
| 21 | `Ch12TriggerMode` 章末（活代码哈希 ＋ 末行一处 teleport，5 种负向 ＋ 注释对照）；chapters.json 第 13 项；Ch12 通关落 (6,29) | 是 |
| 22 | `Ch13LexiconTables`（84 词与 3 种负向）；`Ch13Lexicon` 5 条与逐类注入；分母 > 250 条、≥ 11 份 | 是 |
| 23 | `Ch13Ledger` 第 13 节的 0 处词 8 个（逐词注入，恰报那一条） | 部分：逐 key 核归独立校对 |
| 24 | 无 | 校对报告，不归测试 |
| 25 | 目标链 34 步、脚本 40 份、新图 4 张、编成 2 张都有先验；其余归 `tools/audit.py` | 部分，按设计分工 |
| 26 | `Ch13Engine` 27 条（0 跳过）；`Ch09Save.AllExistingFixtureAndPlayerSavesStillLoad` 读得进 `ch13-end` | 是 |
| 27 | Slice TwentySeven（抹 daoqiyuan、不抹、按序三版；n27、n30 选「再等」与取消，再按一次兑现）；`TriggerMode` 首句判前置 | 是（T-7） |

## 第 12 章终档重生成与第 12 章用例的改动

**两份终档。** 我用 Python 把 `git show HEAD:` 的旧版与工作树新版展平，逐字段比较。两版各 963 个字段，键集合一致。两侧都只差下面 5 项：
- `checksum`；
- `mapId`：`ch12_lingquan` → `ch13_nanming`；
- `position`：(12,11) → (6,29)，即 `haishang`；
- `playSecondsGameplay`：少 5 帧（−0.083 s），因为驱动不再走回 quanbian。

`facing` 本来就是 0，没变。新版实读：day 49560 / 49535，hp 614，修为 4072 / 4011，realm 31 / 31 / 22。这与协调者备注、`Driver::load` 的字面量一致。

**`lingquan.lua`。** 与 HEAD 的 diff 只有两处：头注释那半句，和末尾一行 `teleport("ch13_nanming")`。我照 `executableLines` 的口径用 Python 算了 HEAD 的活代码：815 字节，FNV-1a 为 0xdaa92bbe0608775f，与 `Ch12TriggerMode` 里的常量一致。新版活代码等于旧版加上这一行。

**第 12 章用例仍有牙，没有放宽。**
- **`Ch12Driver`。** `settleAtSpring` 换成了 `landedOnNanming`，它只核图与格。`checkEnding` 照旧断言：`day − side.day == 182`、`facing == 0`，以及新的 `mapId == ch13_nanming`、(6,29)。
- **`Ch12Acceptance`。** 章节卡断言改成 `Opening(13)`。终点变异补了两条：「还在灵泉图上」点名 `ch13_nanming`，「往北迈了一步」点名 position。`kTargets {{12,20},{7,14},{6,14},{5,12}}` 与 `Ch12Balance` 一行没动。
- **`Ch12TriggerMode` 章末判据。** 「不 teleport」改成三条：
  - 末行恰是 `teleport("ch13_nanming")`，紧挨在 `flag.set("ch12.done")` 之后；
  - 全文恰 1 处 teleport；
  - 补丁行之前的活代码逐字节等于 HEAD。

  负向 5 种：两处 teleport、没补、写在置旗之前、落错图、改活代码。另有一个对照：只改注释不报。
- **章节表。** 从「第 12 章是末项」改成「是第 12 项」。「谁是末章」挪给 `UiStyle` 的 `ChapterTableData`（恰 13 项），覆盖面没少。

## 平衡用例：反证有没有牙，判据从哪来

**判据不跟着数据走。**
- `Ch13Balance` 没有一处读 role 的气血、法力、攻防来算期望值。
- `Ch13BattleData` 只钉 8.2 的结构字段：架势、境界、破绽、killable_by、蓄势的 every / all / text_key、行动次数、法术。被调的五个数（儒生气血、海兽法力与攻、巨虾气血与攻）一个都没钉。
- 所以平衡路调数不用改测试，测试也没有跟着数据走。
- 回合窗是字面量，但它的来源是调后实测，而且与记录不一致（T-2）。

**反证。** 我开 `FANREN_CH13_BATTLE_LOG=1` 重跑 `OnlyTheDesignedKindsKillAndStandingStillLoses`，两侧逐场结果相同：

| 扰动 | 结局 | 回合 | 敌余 | 「却伤不到」 |
| --- | --- | --- | --- | --- |
| ① 捂住四门金类法术 ＋ 剑 | 败，700→0 | 22 | 1（儒生） | 3 处 |
| ① 捂住青元剑芒、青蛟旗 ＋ 剑 | 败，700→0；大印被打断 4 次 | 42 | 1（大汉） | 20 处 |
| ② 捂住青元剑芒、青蛟旗、蜂云合剑 ＋ 剑 | 逃，700→97 | 25 | 1（巨虾） | 1 处 |
| ① 只防 | 败 | 26 | 960 | — |
| 对照：不捂 | ①② 全胜（16、13 回合） | — | 0 | 0 |

- **有牙。** 去掉 killable_by，被锁的那一击就会收掉对手：① 两场会赢，② 不会逃。
- ② 只出现一处「却伤不到」。那只手一碰到锁，就判无路可进、随即逃了。巨虾是 ② 里唯一写了 killable_by 的 role（海兽不写），所以这一处只能是巨虾。
- ① 的 `enemyHpLeft == 1` 是两名敌人的余血之和，分不出是谁。但按捂住的类别推（捂金剑时大汉的木没捂，大汉死得了），不留洞。
- 「捂住剑」是从背包里拿走剑类兵刃。出手记录里还有「刀」：刀是另一类，不在两场的 killable_by 里，不影响结论。

**手稳。** 三场两侧各重打三遍，结局指纹（回合、胜负、出手记录、战后存档逐行）一字不差，并与通关那一趟同一场一字不差。

**最低合法起点。** 四版两侧都胜。其中两版的数与意图手相同，性质见 T-6。

**「抹回 0」当扰动。** 见 T-7：不空转。抹旗造出的状态玩家到不了，这一半由静态的带检查兜着，只有设施漏看。

## 实跑记录

**用的哪个可执行文件。**
- `build-ch13walk/fanren_tests.exe`，mtime 04:37:07。
- `tests/` 下最新的是 `Ch13SliceTests.cpp`（03:53:18），`src/` 下最新的是 `Realm.h`（10-13 01:22），都早于它。
- `data/`、`scripts/`、`maps/`、`tools/`、`tests/fixtures/` 下没有一个文件晚于 B 路日志（04:49:39）。所以没有重建。
- B 路 `build-ch13walk.log` 按 gb18030 读字节：VALIDATE_OK、MAPGEN_IN_SYNC（79 张）、SELFTEST_OK、ARTGEN_IN_SYNC（721 个）；0 个 `warning C`；2028 / 2028；EXITCODE=0。
- 工作目录 `H:/Work/Kys/fanren`。

| 过滤 | 结果 | 耗时 |
| --- | --- | --- |
| `Ch13*:Ch12*` | 166 个用例、33 个套件，**全部通过**，RC=0。Ch13 82 条（引擎 27、测试路 A 35、B 20），Ch12 84 条 | 25.3 s |
| `Ch09Save*:GameWiring*:UiStyle*:Chapter*:NpcPresence*:Objective*:PathAction*:Lexicon*:Ch07BattleData*:MapLink*:Ch11*` | 207 个、38 个套件，**全部通过**，RC=0 | 20.9 s |
| `Ch13Balance.OnlyTheDesignedKinds*`（开 `FANREN_CH13_BATTLE_LOG=1`） | 通过；这一趟只用来数出手记录 | — |

没有 SKIP，也没有 DISABLED。
