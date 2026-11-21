# 第 7 章可玩性改版·第一期地基包·路 G：指引（2026-11-20）

> 施工图：`docs/ch07-foundation.md` 第 2 节（主检出里，未提交）。同一天另外两路并行：路 M（地图与说明）、路 D（对白减负）。
> 分支 `claude/ch07-foundation-g-96ce1c`，构建目录 `fanren/build-g`。

---

## 0. 一览

| 项 | 做了什么 | 主要文件 |
| --- | --- | --- |
| 2.1 目标框两行式 | `Objective` 加 `hint_key` / `detail_key` / `progress`；50 步重写；目标框画要点行、换目标闪一下响一声 | `core/model/Types.h`、`core/rules/Objectives.*`、`io/DataLoader.cpp`、`game/WorldHud.*`、`game/Guide.*`、`data/objectives/ch07.json`、`data/text/ch07_objectives.json` |
| 2.2 跨图指路 | `rules::routeToward` 返回整条路；照新版式写的步骤「前往」行写「前往　岳麓殿（经黄枫谷）」 | `core/rules/Objectives.*`、`game/WorldScene.cpp` |
| 2.3 教学卡 | 读取、规则、卡片场景、脚本 `tutorial("id")`、引擎事件、设置「教学提示」；第 7 章 16 张 | `io/GuideLoader.*`、`core/rules/Tutorials.*`、`game/TutorialScene.*`、`game/Application.*`、`data/tutorials/ch07.json`、`data/text/ch07_tutorials.json` |
| 2.4 记事四页 | 主线、支线（写奖励，G-25）、指南、见闻 | `game/BoardScene.*`、`io/QuestLoader.cpp`（`reward_key`） |
| 2.5 见闻 | 读取、规则、页面、解锁提示「见闻：XXX」；5 条妖兽条目 | `io/GuideLoader.*`、`core/rules/Lore.*`、`game/LoreNotices.*`、`data/lore/ch07.json`、`data/text/ch07_lore.json` |
| 2.6 头顶标记 | 「支」（支线眼下要找的人 / 触发区）、「商」（铺子旁的掌柜） | `game/WorldScene.cpp`（`markHeads`）、`game/WorldView.*`、`game/WorldHud.*` |
| 截图口 | `--scene guide:<一屏>[@记号…]` | `game/ShotScenes.*` |

存档版本不动（仍是 10）：教学卡看过没有记旗标 `tutorial.seen.<id>`（16 面，已登记进 `data/flags.json`），见闻解锁只读剧情旗标，
见闻提示「哪些已经报过」只在内存里（读档、新开局时重记一遍，不报读进来时就有的）。

---

## 1. 目标框两行式（2.1）

**数据**。`core::Objective` 末尾追加三项，都可空：

| 字段 | JSON | 用处 |
| --- | --- | --- |
| `hintKey` | `hint_key` | 要点：行动句下面一行小字（至多 28 字） |
| `detailKey` | `detail_key` | 详情：只在记事里显示，放原来的长句 |
| `progress` | `progress` | `{"kind":"item","item","need"}` / `{"kind":"aged_item","item","min_age","need"}` / `{"kind":"flags","flags":[…]}` |

加载器（`readObjectiveExtras`）写了就查：`kind` 认不出、`need` 不是正整数、`aged_item` 缺 `min_age`、`flags` 为空数组都判加载失败。
规则：`rules::objectiveProgress` 只从背包与旗标读（`aged_item` 与脚本 `item.count_aged` 同一个口径：年份不低于 `min_age` 的件数），
`have` 夹在 `[0, need]`；`progressLabel` 写成「1/3」。

**内容**。第 7 章 50 步全部重写：行动句最长 11 字（判据要求至多 16）；原来的长句一字不改挂到 `.detail`；12 步有要点；6 步带进度：

| 步 | 行动句 | 进度 | 依据 |
| --- | --- | --- | --- |
| n02 | 催熟黄精，放进药篓 | 黄精 ≥44 年 × 3 | `yaolou.lua` 的 `item.count_aged("herb_huangjing_cao", 44)` |
| n03 | 催熟紫参，放进药篓 | 紫参 ≥100 年 × 2 | 同上 |
| n05 | 备足灵石，上藏室翻书 | 灵石 22 | `cangshi.lua` 先后扣 1、1、20 |
| n11 | 催熟黄精芝到千年 | 黄精芝 ≥1000 年 × 2 | `yuanjiao.lua` |
| n37c | 在沼泽深处找火灵石 | 火灵石 1 | `huolingshi.lua` |
| n45 | 炼出筑基丹，去石凳盘点 | 筑基丹 4 | `banian.lua`：不足 4 颗就 return |

原句本来就短的几步（n16、n22、n23、n31、n40、n41、n44）行动句与原句相同或只顺了一下字。

**画面**（`hud::drawObjectiveBox` 多了带 `hint` 与 `flash` 的重载）：行动句 18 号字、要点 15 号字 `theme.gold`、「前往」行照旧 16 号亮金。
换了一步（`WorldFrame::objectiveId` 变了，且不是读档后头一回）框亮两下 1.2 秒、响 `ui_open`。**没有要点的步骤走老重载，逐像素不变**（判据 5）。

## 2. 跨图指路（2.2）

* `rules::routeToward(links, from, goal, state)`：与 `nextPortalToward` 同一趟广度优先、同一个平手规则，返回整条路的门；
  第一道门恒等于 `nextPortalToward` 给的那一道（单测钉着）。`routeWaypoints` 取途中的图。
* `WorldScene::objectiveWhereText`：**照新版式写过的步骤**（`guide::upgradedObjective`：有要点、详情或进度的那些，眼下只有第 7 章）
  写「前往　目标图（经甲、乙）」，至多写 3 个，多了接「…」；直达或目标在本图不写「经」。别的章照旧「前往　X　先到　Y」——
  第 4 章那条要经 7 张图，整条写出来一行放不下，判据 5 也要它不变。
* 单测 `GuideRoute.EveryChapterSevenStepSaysWhereToGoAndWhichMapsToPass`：从第 7 章章首存档起，一步步置完成旗标，人站在上一步把他留下的那张图
  （上一步脚本 `teleport` 走了就是落脚那张），断言每一步要么目标在本图（不报地名）、要么「前往」行写着目标图并写全途经的每张图，且 50 步全都走得到。

## 3. 教学卡（2.3）

**数据** `data/tutorials/ch07.json`，格式照施工图；文案 `data/text/ch07_tutorials.json`（`tutorial.<id>.title / .l1–.l3`）。16 张，全部 `min_chapter = max_chapter = 7`。

**判章不看 `GameState::chapter`**：那个字段剧情不维护，第 7 章章首存档里它还是 1。改用章节表的 `done_flag`，与对白头像同一个判据：
`minChapter` 的前一章演完了、`maxChapter` 那一章还没演完（`rules::tutorialInChapter`，传 `ChapterDoneFn`）。

**引擎事件**（`trigger` 认这些，单测查每张卡挂的事件有人发）：

| 事件 | 谁发 | 卡 |
| --- | --- | --- |
| `world.idle` | 每帧，顶上是走图时 | 记事与地图（keys） |
| `objective.<步 id>` | 主线换到这一步（读档后头一回也发） | 交药看年份（`objective.n02_jiaoyao`） |
| `facility.<kind>` | `openFacility`（过了解锁闸之后） | 小瓶绿液（field）、打坐回气（meditate）、炼丹成功率（alchemy） |
| `battle.command` / `battle.round2` / `battle.item_cast` | `BattleScene::openMenu`（轮到我方）；第 2 回合起发 round2；第 4 回合起、背包里有能施法的道具才发 item_cast | 破绽与架势（第一场第 1 回合）、蓄劲（同一场第 2 回合）、道具施法 |
| `battle.after.<卡 id>` | `BattleScene::openMenu`：对**开战前**就看过的每一张卡各发一个（开战时 `Application::noteBattleStart` 记下） | 防御换先手（`battle.after.boost`：蓄劲讲过之后的**下一场**第 1 回合） |
| `foe.near` / `foe.alert` | `WorldScene::noteGuideEvents`：明雷 6 格以内 / 在追 | 偷袭明雷、明雷追击与绕行 |
| `loot.near` / `gate.near` | 同上：没开的宝箱、没采的采集点 / 关着的机关 2 格以内 | 宝箱与采集、机关与场地技 |
| `roster.joined` / `bond.gained` / `world.hurt` | 每帧走图时：名册有人 / 谁的默契 > 0 / 气血不到一半 | 伙伴上场与默契、赠礼、战斗外用药 |

**行为**（`Application::tutorialEvent / requestTutorial / flushTutorials`）：

* 排队，一帧弹一张。**战斗里**：顶上是 `BattleScene` 就弹（事件只在轮到我方开菜单时发），卡片压在战斗上，战斗只更新最上面那一层，于是输入暂停。
  **不在战斗里**：脚本演完、没有命令等着回填、顶上是走图 / 灵田 / 打坐 / 炼制面板才弹；对白、选项、章节卡、菜单开着都等。
* 弹的那一刻置 `tutorial.seen.<id>`、记进 `tutorialLog()`；设置「教学提示」关着时照样置旗标、不压卡片——指南页翻得到。
* `TutorialScene`：720 宽的面板，标题、至多三行、右下「[Z] 知道了」（`{key.confirm}` / `.pad` 版 `{pad.confirm}`）；确认或取消关掉；
  非无头时弹出 0.35 秒内不收键（正连按确认翻对白时第一下不把卡顺手关掉）。
* **缺省关着**（`Application::setTutorialsEnabled`，与野外遭遇同一个套路）：`main.cpp` 只在交互游玩时打开（无头、截图不弹）。
  于是几百条无头测试与通关验收不会被卡片拦住，章末状态也不会多出 `tutorial.seen.*`——这就是施工图要的「给测试一个关掉它的办法」。
* 脚本 `tutorial("id")`（`api.lua`，命令 `CommandKind::Tutorial` 追加在末尾）：不等，返回 `ok, code`（`no_card` / `skipped`）。本路没有在脚本里用它（脚本归路 D）。
* 设置：`io::Settings::tutorialCards`，文件键 `tutorial_cards`，缺省开；设置面板「游玩」组第 4 行（`SettingsScene::kTutorialCards = 9`，面板高 622 → 652）。

**规则数字逐条核过引擎**（核对出处）：

| 卡 | 数字 | 出处 |
| --- | --- | --- |
| 破绽与架势 | 打中一次削 1 点；破势本回合与下回合动不了、挨打 ×2；打中过的破绽记进存档 | `BattleAction.cpp:645`、`Battle.h:39-40`、`BattleScene.cpp:1131` |
| 蓄劲 | 开战 1 点、每回合 +1、至多存 5、一次至多 3；用过劲下回合不攒；普攻每点多一下、法术每点多一倍威力 | `Battle.h:32-34`、`BattleState.cpp:250-256`、`BattleAction.cpp:300-307, 467-469` |
| 防御换先手 | 受伤减半到自己下次出手；下一回合先手；不耗劲不耽误攒劲 | `Battle.h:42`、`BattleState.cpp:196-203, 276-285` |
| 道具施法 | 不耗法力、一次一张、不能蓄劲；土牢符削 3 点不伤人；已破势或没架势用不出去不耗符 | `BattleAction.cpp:95-97, 159-165, 540-569`、`data/items` |
| 偷袭明雷 | 侧面或身后面朝它按确认；全队开场多 1 点劲、第一回合全部先手 | `WorldScene.cpp:268-275`、`BattleState.cpp:63, 180-202` |
| 明雷追击 | 正前方 3 格看得见；比主角慢；离巡逻路线 6 格以上或 2 秒追不上就回去；本章打赢不重生 | `FoePatrol.h:35-45`、`FoePatrol.cpp:313-328`、`WorldObjects.cpp:124-131` |
| 宝箱与采集 | 宝箱只开一次；采集点 30 天重生；找齐有奖励 | `data/exploration`、`maps/ch07_*` 的 `respawn_days` |
| 小瓶绿液 | 炼气 7 天一滴、至多 3 滴、满了不凝；满一年才浇得进；一滴翻倍至少 +10 年；打坐也凝 | `Application.cpp:135-146`、`Bottle.cpp:39-43, 102-127` |
| 打坐回气 | 一日 / 十日 / 一月 / 一年；每天回一成，伙伴回气血 | `CultivationScene.cpp:372-389`、`Application.cpp:707-728` |
| 炼丹成功率 | 看熟练度、炉品、难度；失败全毁；成败都长熟练度；熟练度不够开不了炉 | `Crafting.cpp:216-253` |
| 伙伴与默契 | 至多 3 人上场；并肩打赢 +2、每人每天至多 +6；相熟（20）追击、知己（70）合招 | `Companions.h:24-43` |
| 赠礼 | 每人每天一次；合心意 +5、寻常 +2、不喜 0 | `Companions.cpp:64-78` |
| 战斗外用药 | 金疮药 15、养精丹 55；伙伴只回气血；满了不吃 | `data/items/pills`、`FieldItems.h` |

单测 `GuideTutorials.TheRuleNumbersOnTheCardsMatchTheEngine` 钉了最容易随平衡走散的几条（土牢符、金疮药、七天、五点 / 三点）。

## 4. 记事四页（2.4）

`BoardScene` 改成四页，左右键翻（回绕），页签在标题下面；面板从 880×580 加高到 920×648（主线页下半截仍放得下 ≥10 行，`ObjectiveTests` 钉着）。

* **主线**：上半截「眼下」——行动句（带进度）、要点（金）、详情长句（暗）、地点；下半截做过的步骤（倒序，行字照旧是行动句），
  光标那一步的**详情**写在最底下一栏。
* **支线**：左列任务名 + 状态，右栏标题、摘要、当前步骤（地名）/ 过期原因 / 已了结、**奖励**。
  奖励读 `Quest::rewards`（物品名 × 数量，钱按 `Wording::currencyName` 叫碎银 / 灵石——G-25 的那条「第 1–5 章不许上屏灵石」单测钉着）。
  第 7 章两条支线的回报是脚本按情形发的、刻意不进 `rewards`（`Ch07SliceTests` 钉着 Z1 的 `rewards` 为空），所以 `Quest` 加了可选的
  `reward_key`（一句话），两条各写一句，文案在 `data/text/ch07_quests.json`（`ch07.quest.*.reward`；`ui_guide.json` 只放界面固有字）。**收掉技术债 G-25**（`docs/tech-debt.md` 那一条留给集成时改状态）。
* **指南**：看过的教学卡，右栏写全文，确认键重看（`Application::replayTutorial`，不看开关、不改旗标）。
* **见闻**：按类别（妖兽、灵草、人物、所闻）列解锁了的条目；妖兽条目自动补「已知破绽　火、土」（没记下写「尚未看破」）与「掉落　寒蟾酥」
  （`rules::loreDrops`：把这个角色编进敌方的每一场仗的 `rewards.drops` 并起来）。

`BoardScene::buildRows` 保留（支线在前、主线在后那一整张表，第 5、6 章的测试读它），行字与从前逐字相同；面板把它拆成两页画。

## 5. 见闻（2.5）

* 格式照施工图定死；读取 `io::loadLoreDir`（只查形状：表外字段、id 重复、`category` 四选一）。条目 `chapter` 取文件顶层。
* 本路写了 5 条妖兽：寒冰蟾、彩蛾（`ch07.pojin` 解锁）、飞蛇（`ch07.yixiantian`）、火焰鼠（`ch07.yueyang`）、碧水鳄（`ch07.kundi`）。
  正文**不写破绽**——破绽由页面按存档里看破了几样自动补，正文先说了就没得看破了。标题 ≤10 字、正文 ≤120 字。
* 解锁提示：`Application::watchGuide` 每帧比一次解锁表，多出来的推一条「见闻：XXX」（`game/LoreNotices.*`），屏幕上沿正中、淡入 0.25 秒停 2.6 秒淡出 0.6 秒，
  一条放完下一条；不进场景栈、不收键，画在世界层之上、对话框之下。读档 / 新开局 / 返回首页重记基线，不报读进来时就有的。
* **集成时与路 D 的合并**：两边都建了 `data/lore/ch07.json` 与 `data/text/ch07_lore.json`。合并办法：把路 D 的 `entries` 接在本路 5 条之后，文案两份并成一份；
  `id` 不许撞（加载器会报）。页面按类别排，不靠文件次序。

## 6. 头顶标记（2.6）

`WorldScene::markHeads`（`describe` 末尾）：

* **支线**：进行中的支线、当前步骤的 `target_map` 是这张图——目标是 NPC 就在他名牌左边贴一枚翠色小圆章「支」；目标是触发区就在那一格头顶浮一枚。
  做完一步标记跟着挪（单测：Z2 两处触发区）。
* **商店**：站在 `kind=shop` 设施两格以内的 NPC 名牌左边贴金色「商」（坊市药摊的摊主）。
* 「可接」这一半**没做成**：任务数据里没有「谁给的」，第 7 章两条支线都是剧情脚本在主线触发器上自动接的（`yaolou.lua`、`tongmen.lua`），
  没有可以挂「可接」的人。要做得给 `data/quests` 加一个「谁给、什么时候能接」的字段，留给第二期（伙伴支线多起来时）定。

## 7. 改了哪些旧测试

| 测试 | 改了什么 | 为什么 |
| --- | --- | --- |
| `SettingsWiringTests.cpp` 顶上的 `static_assert` | `kTutorialCards = 9`，`kKeys` / `kRestore` / `kRowCount` 各 +1 | 设置面板插了「教学提示」一行 |
| `SettingsWiring.TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack` | 从「手柄震动」往下多走一格 | 同上 |
| `KeyPrompts.PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText` | 白名单加 `ui.guide.tutorial.close` | 教学卡底下那一行按键提示 |
| `KeyPrompts.PadPlaceholdersOnlyLiveInTheElevenPadPrompts` | 白名单加 `.pad` 版；`{pad.` 总数 21 → 22 | 同上 |

第 1–6、8–13 章的通关验收与对账测试**一条没改**：教学卡缺省关着，目标框只对照新版式写过的步骤生效。

## 8. 测试

新增 `tests/GuideTests.cpp`（24 条）：目标数据与进度规则、加载器拒收坏进度、整条路与门锁、第 7 章 50 步逐步指路、别的章不受影响、
教学卡数据 / 数字 / 只弹一次 / 关掉记看过 / 对白中不弹 / 脚本命令 / 战斗事件、记事四页翻页 / 支线奖励与钱的叫法 / 指南 / 见闻破绽与掉落、
见闻数据与提示、头顶标记、判章规则。

全量 ctest：见第 10 节。

## 9. 截图

`--scene guide:<一屏>[@记号,…]`（`game/ShotScenes.cpp` 的 `pushGuideShot`）：记号是旗标名，或 `weak.<角色 id>`（把这个角色的破绽全记下）。演示状态，不存盘。
图在 `fanren/build-g/shots/`，清单见第 10 节。

## 10. 结果与没做成的

### 10.1 验收判据（2.8）

| # | 判据 | 结果 | 证据 |
| --- | --- | --- | --- |
| 1 | 50 步行动句 ≤16 字；要数量的都有进度；长句都在 detail_key | 满足（最长 11 字；6 步带进度；50 步都有详情） | `GuideObjectives.*` |
| 2 | 第 7 章每一步都写得出「去哪、经哪」，或目标在本图；有单测 | 满足（50 步逐步走过，全部走得到） | `GuideRoute.EveryChapterSevenStepSaysWhereToGoAndWhichMapsToPass`、`GuideRoute.TheWholeRoute…` |
| 3 | 教学卡 ≥14 张、各只弹一次；关掉设置后不弹；规则数字与引擎一致 | 满足（16 张；数字逐条核过，见第 3 节表） | `GuideTutorials.*`、`TutorialFlow.*` |
| 4 | 记事四页能翻；支线写出奖励；指南列得出看过的卡；见闻页显示妖兽已知破绽与掉落 | 满足 | `BoardPages.*`、`GuideBoard.*`；截图 g05–g08 |
| 5 | 第 1–6、8–13 章目标框画面不变 | 满足（没新字段的步骤走老重载、「前往」行照旧；那些章的测试一条没改） | `GuideRoute.OtherChaptersKeepTheirOldWhereLine` |
| 6 | 全量 ctest 通过 | **2335 / 2335**（审核返修后，多了返修的两条） | `build-g/ctest_full4.log` |

全量跑了三遍：第一遍 4 条红（见 10.3）、修完后第二遍 1 条红（`SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`：129 ms 超了 120 ms 的上界——那会儿我同时在拍截图，
单独重跑 3/3 过），第三遍全绿（2333/2333）。审核返修后第四遍：2335/2335。

### 10.2 截图（`fanren/build-g/shots/`，脚本 `build-g/shots.bat`）

| 图 | 内容 |
| --- | --- |
| `g01_objective_n01.png` | 章首：第 1 步，没有要点（与老版式同样子） |
| `g02_objective_hint_progress.png` | 第 2 步：行动句 + 「0/3」+ 要点小字 |
| `g03_objective_via.png` | 第 4 步：「前往　岳麓殿（经黄枫谷）」 |
| `g04_tutorial_card.png` | 教学卡「蓄劲」 |
| `g05_board_main.png` | 记事·主线：眼下（行动句、要点、详情、地点）+ 做过的 + 底栏详情 |
| `g06_board_side.png` | 记事·支线：右栏摘要、眼下、奖励 |
| `g07_board_guide.png` | 记事·指南 |
| `g08_board_lore.png` | 记事·见闻：寒冰蟾的已知破绽「火、土」与掉落「寒蟾酥」 |
| `g09_lore_notice.png` | 「见闻：飞蛇」提示 |
| `g10_head_mark_shop.png` | 坊市收药摊主的「商」 |
| `g11_head_mark_quest.png` | 百药园药篓（Z1 的目标触发区）头顶的「支」 |
| `g12_settings.png` | 设置面板「教学提示」一行 |
| `g13_objective_hint_qiannian.png` | 第 11 步：黄精芝进度与要点（站在黄枫谷，「前往　百药园」直达不写经） |
| `g14_tutorial_field_item.png` | 返修：「战斗外用药」写主菜单「储物袋」（第 7 章那一页的实际叫法） |
| `g15_tutorial_gift.png` | 返修：「赠礼」写「队伍」→「赠与」 |

返修后重拍了全部 15 张（g07 指南页里「记事与地图」第一行跟着改了）。

### 10.3 第一遍全量的四条红与处理

| 测试 | 原因 | 处理 |
| --- | --- | --- |
| `Lexicon.NoChapterSaysAWordItHasNotEarnedYet` | 卡名写了「掌天瓶」——全 14 章禁用（人间篇只叫小瓶 / 绿瓶）。**施工图 2.3 的卡名清单里就写着「掌天瓶绿液」**，照抄会犯 | 卡名改「小瓶绿液」 |
| `Ch07Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens` | 判卷要求目标行含 3.4 的 R-3 提示原文，原文现在挪进了详情 | `expectHint` 改为在行动句、要点、详情三者里找（原文一字未改） |
| `Ch07Acceptance.No6_EachGatedWordStaysOutOfTheKeysBeforeItsNode` | 新文案（`tutorial.*`、`lore.ch07.*`）不在节点表里 | 节点表登记：教学卡步序 0；五条妖兽按解锁那张图的步序 |
| `SettingsWording.ThePanelAndBothEntrancesPassTheForbiddenWordScan` | 设置行名表少了「教学提示」 | 表里加一行 |

这三条旧测试的改动加上第 7 节那四处，是本路改过的全部旧测试。

### 10.4 没做成的、拿不准的

1. **「可接」标记不做**（2.6，主会话审定）：等第二期给任务数据加「谁给的」字段时一起做。眼下只做「进行中、这一步要找他 / 去这里」与「商店」。
2. **肩键翻页不做**（2.4，主会话审定）：LB 已由路 M 配给地图键；记事翻页用左右键（手柄十字键左右）。
3. **共享文件的冲突点**（集成时要手合）：
   * `SettingsScene`：三路都在「游玩」组插一行，`kTutorialCards = 9` 与路 M「小地图」、路 D「自动播放」的行号要重排；面板高我从 622 改到 652，三行都进来要再加高或挪位；
     `tests/SettingsWiringTests.cpp` 的 `static_assert` 与 `UiStyleTests` 的行名表要一起改。
   * `script::CommandKind`：我在末尾追加 `Tutorial`，路 D 会追加 `Bubble`——两个都要，次序随便但不能互相覆盖；`ScriptHost` 的命令名表、`api.lua` 尾部同理。
   * `WorldScene.cpp` / `WorldView.*` / `WorldHud.*`：我动了目标框（`describe` 里那几行、`drawHud` 的目标框调用、`objectiveBoxRect / drawObjectiveBox` 新重载）、
     `describe` 末尾两行（`markHeads`、`noteGuideEvents`）、`drawNamePlates` 里头顶标记几行、`WorldFrame` 追加字段。路 M 会改同一个 `drawHud`（存盘提示挪位、小地图）。
   * `Application.*`：教学卡、见闻提示一整块追加在头文件 BGM 段之前与私有成员末尾；`tick` 里两行、`drawScenes` 里一段、`openFacility` 一行、`dispatch` 一个 case、三处 `resetGuideWatch()`。
   * `tests/Ch07AcceptanceTests.cpp` 的节点表：路 D 加的见闻文案也要在那里登记（不然 No6 会红），删掉的对白 key 若被节点表引用要同步。
   * `data/lore/ch07.json`、`data/text/ch07_lore.json`：两路同名，合并办法见第 5 节。
4. **教学卡「记事与地图」第二行**写着「区域地图也在主菜单」，指的是路 M 加的那一项；我这一支上主菜单还没有它，单独看这一支这句话是超前的。
5. **教学卡的触发时机是我定的**（第 3 节那张表），没在真机上从头玩一遍看节奏。第一场仗已按审核改成两张（破绽、蓄劲），防御挪到下一场；章首第一帧仍弹「记事与地图」。
6. **头顶「支」与目标菱形会同在一处**（主线与支线指着同一个触发区时，如药篓）：两样都画，菱形在上、圆章贴边，没做避让。
7. **门禁要补的规则**（不跑门禁，只记下）：`data/tutorials`（trigger 在事件表里、文案 key 在、`tutorial.seen.<id>` 已登记、≤3 行 ≤30 字）、
   `data/lore`（category 四选一、key 在、unlock_flag 已登记、role_id 存在、标题 ≤10 正文 ≤120）、目标链的 `hint_key / detail_key / progress`（key 在、物品存在）、
   `data/quests` 新字段 `reward_key`（key 在）。`GuideTests` 在真树上已经查着这些，门禁照抄即可。
8. 奖励那两句（`reward_key`）已按审核挪进 `data/text/ch07_quests.json`。

### 10.5 审核返修（2026-11-20，主会话）

| # | 要求 | 做了什么 |
| --- | --- | --- |
| 1 | 卡片里带「」的界面词要与第 7 章屏幕上的字一致 | 逐张核过 16 张：「战斗外用药」主菜单「物品」→「储物袋」（第 7 章领了储物袋，`MenuScene::pageLabel` 走 `bagWord`）；「道具施法」写明是战斗里的「物品」指令（战斗指令仍叫物品）；「赠礼」补上「赠与」；「记事与地图」改成主菜单「记事」。新单测 `GuideTutorials.TheWordsInQuotesAreTheOnesTheChapterSevenScreensShow`：卡片里紧跟「主菜单」的「X」必须是第 7 章状态下主菜单页表里的一页，其余「X」必须是战斗指令、赠与、默契档名、记事页签之一 |
| 2 | 第一场仗只讲两张，防御挪到下一场 | 删掉 `battle.round3`；新事件 `battle.after.<卡 id>`（开战时记下看过哪几张，轮到我方时对它们各发一个），「防御换先手」挂 `battle.after.boost`。新单测 `TutorialFlow.DefendWaitsForTheBattleAfterBoostWasTaught` |
| 3 | 奖励文案挪出 `ui_guide.json` | 挪进 `data/text/ch07_quests.json` |

返修后全量 ctest 见 10.1（第四遍）。
