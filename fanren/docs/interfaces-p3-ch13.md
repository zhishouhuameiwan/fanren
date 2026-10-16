# P3 第 13 章引擎增补契约

> 本文是**契约**：引擎方照它写判据，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-10-13 · 依据第 13 章研究笔记（`notes/ch13/research.md`）C 表（境界锚点）、D 表（引擎现状）、E 表（拍板）、F 表（节点）、G 表（地图）、I 表（战斗）、J 表（旗标）、L 表（物品 / 配方 / 商店）、N.4（teleport）、O 表（接口与开放问题）；格式沿 `docs/interfaces-p3-ch12.md`。
> **本章 0 个代码项**：笔记 D 表逐行核过，本章要的引擎行为全部已有，`src/` 一行不改。**E1–E2 是验证项**：查过、不新做代码，只在 `tests/Ch13EngineTests.cpp` 把内容路要倚仗、而此前从没有一章跑过的两种局面钉住，红了再议（先改笔记、再改本文、最后才动 `src/`）。
> **编号对照**：本文 E1 ＝ 笔记 D 表「面板冲关」「打坐效率」两行（笔记称契约 E1），E2 ＝「灵田」行（契约 E2）；笔记的拍板号写作「E 表 N」、开放问题写作「O-N」，与本文 E 项不是一套；施工图 10.1 其余各项用 C（协调者：门禁表、登记、回写）、T（测试路）、B（平衡路）前缀，不进本文编号。
> 门禁：引擎路不改 `tools/`。规则 25 的 `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER`、selftest 探针搬家归协调者（第 5.2 节）。
> **不做**（笔记 E 表已拍板）：拍卖 / 竞价面板（E 表 25：竞宝厅、交换会全是选项）；物价曲线、`buyRate` 按旗标切换（E 表 14：按地点分店）；「一键采收」Lua 命令（E 表 16：玩家自采）；`aptitude` 命令（E 表 38）；`breakthroughPillBonus`（仍恒 0）；炼器配方（E 表 8：重炼走脚本，`guaranteed` 全仓仍一张）；玄阴经做功法、天都尸火 / 换形诀 / 剑上神雷做法术（E 表 9、L.2）；按 spawn 名落点；遭遇区（D 表）；`party.*`（E 表 37）；存档字段（仍 v9）。
> **本章在第 12 章集成之后开工**：第 12 章 E1（空队伍）、E2（同一 role 先友后敌）与此前各章引擎项都当成已有的用。第 12 章仍在同一棵工作树里施工，`ch12-end-*.sav` 尚未生成——本文判据的夹具一律用 `tests/fixtures/ch11-end-first.sav`（`realm 31 / realmCap 31 / formerRealm 22 / aptitude 50`，2026-10-13 读档核过），判据不依赖章节进度。

---

## 0. 各引擎项现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | n27 `realm.cap(realm.CORE_MID)` 之后，玩家在雾中岛静室（`facility_jingshi_wz`，`effectiveness 150`）按「闭关一年」坐到 5000、面板自冲 31 → 32；n28 开头 `realm.at_least(realm.CORE_MID)` 门闸；本章 `realm.advance` / `realm.demote` 0 处（笔记 C.1–C.2、E 表 10–11、B20） | `rules::breakthroughBlock(realm, cultivation, storyCap)`（`src/core/rules/Cultivation.cpp` 第 151 行）依次判 `SeriesMax` → `StoryCap`（下一关编号 > 上限）→ `NotEnough` → `None`——**上限先于修为**；`tryBreakthrough`（第 161 行）按不下去就不掷骰、一分不扣。`breakthroughChance`（第 101 行）＝ 同档升层 60 ＋ `aptitude / 2` ＋ `pillBonus` ＋ `min(20, 超额 × 20 / 所需)`，夹在 `[kBreakthroughMinChance 5, kBreakthroughMaxChance 95]`（`Cultivation.h` 第 73–74 行）；失败倒扣所需的 20%、再判 15% 走火入魔扣 40%（第 81–83 行）。所需修为 `CoreEarly 5000`、`CoreMid 7000`（`Realm.cpp` 第 146–147 行）；气血 700 → 900、法力 470 → 620（第 217–218、234–235 行）。面板入口 `CultivationScene::breakthrough(Application&)`（`CultivationScene.cpp` 第 536 行，public）：先 `settleDailyPractice`，再 `tryBreakthrough(…, breakthroughSeed(state))`（种子混 `day`、`cultivation`、`realm`，第 93–97 行）；成功则 `tryNext`、修为扣所需、`applyRealmAttributes`（`liftToFloor`，只补差额）。`realm.cap` → `CommandKind::RealmCap`（`Application.cpp` 第 1385–1394 行）→ `raiseRealmCap`（第 1400 行，只升不降，**不动 `realm` 与气血**），非法编号 `ok false`、`code "no_realm"`。打坐：`openFacility` 读设施 `effectiveness`（第 697–700 行）交给 `CultivationScene(sitePercent)`；`bankMeditation(state, days, seed, sitePercent)`（第 459 行）有效率 ＝ `meditationEffectiveness`（非重修 100）× `sitePercent / 100`，按余数账 `cultivationRemainder` 差分——**余数账为 0 时恰等于 `rules::meditate(realm, aptitude, eff, days, seed)`**；`meditationSeed` 只混 `realm` 与 `aptitude`（第 86–89 行）。「闭关一年」＝ `rules::kDaysPerYear`（第 385 行，360 日）。**结丹档面板冲关从没有一章跑过**：第 8 章是筑基档，第 10 章是重修八倍，第 11 章 E5 只钉了「被 `StoryCap` 挡住」 |
| E2 | `ch13_tianxing` 的 `facility_yaoyuan_39`（`field`、`ref_id field_tianxing_39`、`slots 6`、`require_flag ch13.rucheng`）打开第 11 章那块田，玩家自采旧作物；脚本不搬作物（笔记 E 表 16、O-2） | `openFacility`（`Application.cpp` 第 682 行起）先判 `require_flag`（第 690–694 行，未置 → `showMessage("ui.facility.locked")`）；`field` 分支（第 703–720 行）`findField(ref_id)` 已有就直接 `pushScene(FieldScene(id))`，没有才按 `slots`（缺省 `kDefaultFieldSlots 4`，`Application.h` 第 42 行）新建——**已有的田不按这一处的 `slots` 改尺寸**。`FieldScene::harvestAt(app, slot)`（`FieldScene.cpp` 第 179 行，public）先腾空槽位、再 `addItem(seedId, 1, age)`，年份随包；未熟返回 false、背包不动。门禁：`validate.py` 里 0 处提到 `ref_id`（不查跨图唯一；第 146 行那张属性名表在 `tools/mapgen/genmaps.py`，只给 `obj()` 排键序）；**先例已在仓**——`shenshougu_yaopu` 同时挂在 `ch01_shenshougu`（`slots 6`）与 `ch02_yaopu`（`slots 8`）上（2026-10-13 扫 `maps/*.tmj`），门禁与地图测试一向是绿的，笔记 O-2 的疑问由此结案，**C 项不必改 `tools/`**。存档事实：`ch11-end-{first,second}.sav` 的 `field_tianxing_39` 6 槽**全空**（2026-10-13 读档）——意图玩家在这块田里没有作物，E2 守的是在第 11 章种了药的玩家 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。本批三段**都不改**；E1、E2 各自只有判据。

**查过、本章不需要新做的**（笔记 D 表「缺什么：无」那几行）：`realm.at_least`（`api.lua` 第 148 行）；`realm.cap` 只升不降、看返回值（第 178–182 行）；`teleport(map_id, x, y)` 落 default spawn、`x`、`y` 不同时为 0 才改坐标，落地**不判**脚下触发器（只有 `WorldScene::tryStep` 迈步时判）；`advance_days` 单次 ≤ 3650（`kMaxScriptAdvanceDays`，超了静默夹，本章单次最大 3600）且每日回一成气血；`shop(id)` 脚本开店（第 2、11 章用过）、`entries` 为空的只收店（第 11 章 E4）；配方 `requireFlag`、`successChance` 夹 [5, 95]；法术 `effect "stagger"`（第 7 章先例）；多波次、`charge`、`actions`、`killable_by` 非空、`can_escape`；第 11 章诱妖口径（`item.count`、`bottle.drops`、`bottle.spend(1)`、`advance_days(3)`、`battle()` 返回后扣草）；`choice`（取消为 nil）；`item.count` 与 `take` 看返回值；NPC `visible_flag / hidden_flag`；portal `require_flag` ＋ `deny_text_key`、facility 只读 `require_flag`（拦下时只播 `ui.facility.locked`，`Application.cpp` 第 690–694 行；全仓只有 `WorldScene.cpp` 第 169 行的传送点读 `deny_text_key`）；路径行动 `when / until`；章节卡按完成旗标排、最后一章之后接「未完待续」；小瓶结丹 3 日一滴、容量 9（第 11 章 E5）。

**不是引擎的事、写在这里免得找错人**：本章两场仗的量法（「结丹初期单人对两名同阶」「结丹初期单人对五级两只 ＋ 六级首领」，笔记 C.2）归平衡路，用 `tests/BattleHand.h` 量；结丹中期攻防锚点（76 / 58，「暂定」，`Realm.cpp` 第 258、276 行；`Realm.h` 第 175 行是说明它们的注释）本章**不标定**——本章没有一场仗发生在玩家结丹中期之后，留第 14 章（O-9）。意图玩家按几次「闭关一年」**以施工图 8.1 为准**（真档前上界约 6 次、M ≲ 2150 日；笔记 C.2 的 6–14 次作废，施工图 16.1 第 48 条），最终由 `ch12-end` 真档 C0 定（O-3），增量 11790 日不变。`fan_furen.json`、`wen_siyue.json` 一个字不动（笔记 H 表）。

---

## 1. E1 结丹档面板冲关（验证项，不改代码）

### 1.1 一句话口径

**本章的境界只经一处上限：n27 一句 `realm.cap(realm.CORE_MID)` 把 `realmCap` 31 → 32，`realm`、气血、法力一概不动；此后面板上「冲关」按修为判——不足 5000 报 `NotEnough`（不掷骰、不扣），到线报 `None`、成功率恰 85（60 ＋ 25 ＋ 0 ＋ 0）；成功则 `realm 32`、修为扣 5000、气血法力上限抬到 900 / 620（按差额上抬）；再按一次报 `StoryCap`（下一关 33 高过上限 32），不是 `NotEnough`。雾中岛静室 `effectiveness 150` 的进账是普通蒲团的恰 1.5 倍（同种子、余数账清零时整除）。n28 用 `realm.at_least(realm.CORE_MID)` 判是否已冲成。** 任一处与此不符，才改 `src/`（先改笔记 E 表 10、再改本文）。

- 规则层（`rules::breakthroughBlock / tryBreakthrough / breakthroughChance / meditate`）、加载器（地图对象属性照读，无新键）、游戏层（`Application::openFacility`、`CultivationScene::breakthrough / bankMeditation`）：**都不改**。
- 内容路要知道的：`realm.cap` 返回 false 只会是编号写错——`talk` 一句、照走，**不许静默吞掉**；冲关是玩家在面板上按的，脚本里 0 处替他冲；失败只倒扣修为（1000，走火入魔 2000、气血减半），静室随时可进，不设死局。

### 1.2 数据形状（内容路写；只是形状，id 以笔记为准）

```lua
-- scripts/ch13/biguan.lua（n27），本章唯一一处境界调用
local ok = realm.cap(realm.CORE_MID)
if not ok then talk("", "<文案 key，内容路 C 定>") end
-- scripts/ch13/chuguan.lua（n28）开头：不到只一句，不置旗、不拨日
if not realm.at_least(realm.CORE_MID) then talk("", "<文案 key，内容路 C 定>") return end
```

雾中岛静室（`maps/ch13_wuzhong.tmj` 的对象属性；facility 门闸**只写 `require_flag`**——引擎拦设施只播 `ui.facility.locked`、不读 `deny_text_key`，规则 20 也只查 portal；施工图 16.1 第 45 条）：`{"name": "facility_jingshi_wz", "kind": "meditate", "effectiveness": 150, "require_flag": "ch13.lifu"}`。天星城三十九层静室 `facility_jingshi_39` 照此写 `effectiveness 125`、`require_flag ch13.rucheng`；南明岛 `facility_dazuo_nanming` **不写** `effectiveness`（＝ 100，第 11 章契约第 7 节「其余打坐设施不写」；笔记 G 表写的「`effectiveness 100`」按此收口）。三处都在门禁的 100–300 之内。

### 1.3 判据（`tests/Ch13EngineTests.cpp`，引擎路写；每条直接读 `GameState` / `rules::` 返回值 / 地图对象，不经 `Driver`）

夹具 `Ch13Fixture` 照 `Ch11Fixture`：headless `Application` 载入 `tests/fixtures/ch11-end-first.sav`，先验 `realm 31 / realmCap 31 / formerRealm 22 / aptitude 50 / maxHp 700 / maxMp 470`；探针脚本写法照第 11 章契约 2.3 的偏差记录（`TempDir/scripts/` 重挂 scriptRoot、`startEvent` 后泵帧）。

1. **起点被上限挡**：`cultivation` 置 5000 → `breakthroughBlock == StoryCap`；`tryBreakthrough(…, 任意种子)` 的 `blocked == StoryCap`、`success false`、`cultivationLost 0`。**负向**：同一状态把 `realmCap` 直接改 32 → `None`（证明挡它的是上限，不是修为）。
2. **剧情只抬上限**：探针脚本 `local ok = realm.cap(realm.CORE_MID)`，把 `ok` 写进探针旗标 → 旗标 1、`realmCap == 32`、`realm` 仍 31、`maxHp` 仍 700、`hp` 不变、`cultivation` 不变。**负向**：`realm.cap(99)` → 旗标 0（`no_realm`）、`realmCap` 不变；再跑一次 `realm.cap(realm.CORE_EARLY)` → `ok` 真而 `realmCap` 仍 32（只升不降）。
3. **门槛与成功率（字面量写死，不从实现推导）**：上限 32 之后，`cultivation 4999` → `NotEnough`，`breakthroughChance(CoreEarly, 4999, 50, 0) == 0`；`5000` → `None`、**恰 85**；`6000` → **恰 89**（超额 1000 × 20 / 5000 ＝ 4）；`10000` → **恰 95**（60 ＋ 25 ＋ 20 ＝ 105，夹到 95）。**负向**：`cultivation 4999` 时 `tryBreakthrough` 的 `cultivationLost == 0`（不足不算一次尝试）。
4. **面板冲成**：真 `CultivationScene` 栈顶，`realmCap 32`、`cultivation 5000`，调 `breakthrough(app)`；**每次按之前**都把 `cultivation` 复位 5000、`hp` 砍到 300；失败（15%）则 `day += 1` 再按（失败后再判 15% 走火入魔、`hp` 减半，所以每次都要重砍），至多 50 次（种子混 `day`，全败概率可忽略）→ 必有一次 `success`：`realm == 32`、`cultivation == 0`（扣恰 5000）、`maxHp == 900`、`maxMp == 620`、成功那一次按前按后 `hp` 恰差 200（300 → 500，钉「按差额上抬、不补满」）。接着**再按一次**：`blocked == StoryCap`（不是 `NotEnough`——修为此刻是 0）、`realm` 仍 32。**负向**：不跑第 2 条的 `realm.cap`、`cultivation 5000` 照按 → `blocked == StoryCap`、`realm` 仍 31、修为分毫不扣（笔记「去掉 `realm.cap` → 面板永远 `StoryCap`」）。
5. **面板那一行**：`CultivationScene::buildMainItems(state)` 的「冲关」行：上限 31、修为 5000 → `enabled false`、`disabledReason` 非空；上限 32、修为 5000 → `enabled true`。两态成对。
6. **静室 150**：两份状态都把 `cultivation`、`cultivationRemainder` 清 0，同一颗 `meditationSeed`：`y = bankMeditation(state, 360, seed, 150).cultivation`、`x = …(…, 100)` → **`2y == 3x` 恰等**（结丹档每日 6000 / 9000 万分点、顿悟每次加 30 日的功，360 日两者都整除）；且 `y == rules::meditate(CoreEarly, 50, 150, 360, seed).cultivation`。**对照**：`cultivationRemainder` 不清（夹具原值 1350）时 `y == meditate(…, 1710, seed) − meditate(…, 1350, seed)`（差分，不是 `meditate(…, 360, …)`）——钉住「余数账先清零」这一前提，笔记 D 表「逐点相等」那一句只在这个前提下成立。真 `openFacility(MapObject{meditate, effectiveness "150", require_flag ch13.lifu})`：旗标 0 → 栈顶 `"Dialogue"`；置旗 → 栈顶 `"Cultivation"`、`siteBonusLine(state)` 含 "50"。
7. **出关门闸**：探针脚本把 `realm.at_least(realm.CORE_MID)` 写进探针旗标：`realm 31` → 0；`realm 32` → 1。
8. **真数据（内容路落地后）**：`maps/ch13_wuzhong.tmj` 的 `facility_jingshi_wz` `kind meditate`、`effectiveness == "150"`、`require_flag ch13.lifu`；`maps/ch13_tianxing.tmj` 的 `facility_jingshi_39` `effectiveness == "125"`、`require_flag ch13.rucheng`；`scripts/ch13/**` 里 `realm.cap(` 恰 1 处且在 `biguan.lua`、参数 `realm.CORE_MID`，`realm.advance(` / `realm.demote(` 各 0 处，`chuguan.lua` 第一个可执行语句块含 `realm.at_least(realm.CORE_MID)`。**负向**：同一扫描函数喂一段内存字符串 `realm.advance(realm.CORE_MID)` → 报 1 处。任一文件不存在即 `FAIL()`（集成后；施工期为 `GTEST_SKIP`，第 13 章门禁审查 G-4）。
9. **负向自检**（改坏 → 增量构建 → 确实红 → 还原，日志 `build-ch13eng-neg-*.log`）：`breakthroughBlock` 里把 `StoryCap` 与 `NotEnough` 两句对调 → 第 4 条「再按一次」红；`raiseRealmCap` 改成直接赋值（可降）→ 第 2 条负向红。

### 1.4 实现记录（引擎路交付时填）

2026-10-13 引擎路交付，`src/` 零改动，判据全在 `tests/Ch13EngineTests.cpp`（夹具 `Ch13Fixture`，照 `Ch11Fixture` / `Ch12Fixture`：读 `ch11-end-first.sav`，先验六项照 1.3 头；探针脚本照第 11 章契约 2.3 偏差记录重挂 scriptRoot、`startEvent` 后泵帧，探针停下来等玩家即 `ADD_FAILURE`）。E1 共 17 条用例（正向 11、单列的负向 / 对照 6；第 5、7 条与第 6 条真设施那条另有用例内的成对负向段）。

- **第 1 条** `FiveThousandAtThirtyOneIsBlockedByTheStoryCapAndNothingIsCharged`（五颗种子，`blocked / success / cultivationLost / cultivationSpent / backlash` 全零）；负向 `NegativeTheSameStateWithTheCapAtThirtyTwoIsNotBlocked`。
- **第 2 条** `StoryCapToCoreMidLiftsOnlyTheCap`：先把 `hp` 写成 555（非满），探针旗标先写哨兵 99；负向 `NegativeAnInvalidCapIsRefusedAndALowerCapNeverLowersIt`（`realm.cap(99)` 在上限 31、32 时各跑一次：`ok` 假、`code == "no_realm"`、上限不动；`realm.cap(realm.CORE_EARLY)` → `ok` 真、上限仍 32）。
- **第 3 条** `Ch13CoreMidRule.TheThresholdIsFiveThousand…`（4999 / 0、5000 / 85、6000 / 89、10000 / 95，字面量）；负向 `NegativeOnePointShortIsNotAnAttemptAndLosesNothing`（200 颗种子）。
- **第 4 条** `ThePanelBreaksThroughToCoreMid…`：面板经真 `openFacility(meditate)` 开出；每按前复位修为 5000、`hp` 300；失败那几次顺带钉「倒扣 1000 / 走火 2000 且 `hp` 150、`realm` 仍 31」；成功那次 `realm 32`、修为 0、`900 / 620`、`hp` 300 → 500；再按 `StoryCap`。负向 `NegativeWithoutTheCapThePanelReportsStoryCapAndChargesNothing`。
- **第 5 条** `ThePushRowIsOffAtCapThirtyOneAndOnAtCapThirtyTwo`（两态成对，另核行名是 `pushLabel`）。
- **第 6 条** 三条：`ARoomAtOneHundredFifty…`（十颗种子 `2y == 3x`、`y == meditate(…, 150, 360, seed)`、`x == meditate(…, 100, 360, seed)`、余数账记 360）；对照 `ControlAnUnclearedLedgerPaysTheDifferenceOnTheSameCurve`（余数账 1350：32 颗种子逐颗等于 `meditate(1710) − meditate(1350)`、余数账记 1710，且至少一颗与 `meditate(360)` 不等——证明这条对照真区分得开）；`TheWuzhongRoomIsGatedByLifuAndItsPanelPaysOneHundredFifty`（`ch13.lifu` 0 → `Dialogue`；置旗 → `Cultivation`、`siteBonusLine` 含 50；**偏差**：`meditationSeed` 是 `CultivationScene.cpp` 的文件内函数，测试拿不到，契约「同一颗 `meditationSeed`」在规则层改为逐颗种子比，另在真面板上各坐一年 `meditateFor(app, 360)`——用的正是面板自己的 `meditationSeed`——再断 `2y == 3x`；负向是施工图 10.2 那条：同一设施去掉 `effectiveness` → 没有加成行、进账回到 100 那条）。
- **第 7 条** `TheLeavingGateReadsFalseAtThirtyOneAndTrueAtThirtyTwo`（哨兵 99）。
- **第 8 条** 三条读真数据 ＋ 一条扫描器负向：`TheWuzhongRoom…`、`TheTianxingRoom…`、`TheChapterScriptsCapOnceInBiguanNeverAdvanceAndChuguanOpensWithTheGate`，文件缺则 `GTEST_SKIP() << "等内容路 <文件> 落地"`；扫描只看活代码（去掉 `--` 行注释与 `--[[ ]]` / `--[==[ ]==]` 块注释——`biguan.lua` 头注释里就写着「`realm.advance` / `realm.demote` 0 处」，不去注释会误报）；「`chuguan.lua` 第一个可执行语句块」按施工图验收 5 的说法取「第一条非空活代码行」。扫描器负向 `NegativeTheScannerCountsALiveAdvanceAndIgnoresCommentedOnes` 不依赖内容路、随时跑。**交付时内容路已落 `maps/ch13_{wuzhong,tianxing}.tmj`（23:40）与 `scripts/ch13/{biguan,chuguan}.lua`（23:52），三条实跑、全绿、0 跳过**；`GTEST_SKIP` 守卫已在集成时（协调者，构建槽 `integ13`，2026-10-14）改成 `FAIL()`，文件缺即红。
- **第 9 条负向自检**：为不碰共享 `src/`，沿用第 12 章引擎路的 scratch 复制树（`src/ tests/ CMakeLists.txt` 按字节同步，`vendor` 目录联接），改坏 → 增量构建 `fanren_tests` → 跑 `Ch13*` → 还原，日志 `build-ch13eng-neg-{base,A..H}.log`（末尾附 `TESTRC` 与失败用例名）。base 27/27 绿。**A**（契约）`breakthroughBlock` 两句对调 → 只有第 4 条红，红在「再按一次」（报 `NotEnough`）；**B**（契约）`raiseRealmCap` 改直接赋值 → 只有第 2 条负向红（上限被压回 31）；另加 **F** `bankMeditation` 丢掉 `sitePercent` → 第 6 条三条全红；**G** `realm_at_least` 的 `>=` 改 `>` → 只有第 7 条红；**H** 突破后 `hp` 补满 → 只有第 4 条红（`hp` 900 ≠ 500）；E2 的 C、D、E 见 2.4。仓库里的 `src/` 自始至终没改过（`git status src` 空）。

---

## 2. E2 跨图同一块田（验证项，不改代码）

### 2.1 一句话口径

**灵田的身份是 `ref_id`，不是地图：`ch13_tianxing` 的 `facility_yaoyuan_39` 与 `ch11_dongfu` 的同名设施指向同一块 `field_tianxing_39`，存档里已有它就原样打开——不新建、不清空、不按这一处的 `slots` 改尺寸；采收的灵草带着年份进背包。门闸只靠 `require_flag ch13.rucheng`（facility 不读 `visible_flag / hidden_flag`）。门禁对同一 `ref_id` 挂在两张图上不报错（`shenshougu_yaopu` 先例），不改 `tools/`。** 任一处与此不符，才改（先改笔记 O-2、再改本文）。

- 规则层（`rules::SpiritField`、`plant / growField`）、加载器（`io::SaveFile`、地图对象）、游戏层（`Application::openFacility`、`FieldScene`）：**都不改**。
- 内容路要知道的：两张图上的 `slots` 都写 6（与第 11 章 `field.unlock("field_tianxing_39", 6)` 一致）；写成别的数不会报错，但对已有的田不起作用，只会让新档（没走过第 11 章的测试档）建出另一种尺寸——所以 2.3 第 5 条把「两处都是 6」钉死。

### 2.2 数据形状（内容路 A 写进 `maps/ch13_tianxing.tmj`）

`{"name": "facility_yaoyuan_39", "kind": "field", "ref_id": "field_tianxing_39", "slots": 6, "require_flag": "ch13.rucheng"}`——对象名与 `ch11_dongfu` 的同名（对象名只要求图内唯一，`validate.py` 第 347 行）；**不写 `deny_text_key`**：facility 不读它，`check_reference_pair` 却会要求那条 key 在文案表里存在，写了就是一条永远播不出的文案（`ch11_dongfu` 同名设施也只写 `require_flag`；施工图 4.3 第 1 条、16.1 第 45 条）。脚本里 0 处 `field.unlock("field_tianxing_39"`、0 处搬作物。

### 2.3 判据（`tests/Ch13EngineTests.cpp`；每条直接读 `state.fields` / 背包 / 栈顶场景名，不经 `Driver`）

夹具同 1.3（`ch11-end-first.sav`，先验 `fields` 恰 6 块、`field_tianxing_39` 恰 6 槽且全空——真档事实，见第 0 节）。每条开头在内存里给 `field_tianxing_39` 槽 0 种一株灵草：`seedId herb_huangjing_cao`、`age 60`、`ripe true`；槽 1 种同一种、`age 5`、`ripe false`（只改内存、不写存档）。

1. **已有的田原样打开**：手构 `MapObject{kind field, ref_id field_tianxing_39, slots "6", require_flag ch13.rucheng}`，旗标置 1 → `openFacility` 后栈顶 `"Field"`；`state.fields.size()` 仍 6；槽 0 `seedId / age / ripe` 与开前逐字段相同。**负向**：`ref_id` 换成存档里没有的 `field_ch13_probe` → `fields.size() == 7`、新田 6 槽全空（证明第 1 条的「没新建」来自「已有」，不是 `openFacility` 从不建田）。
2. **不改尺寸**：同一手构对象 `slots` 写 `"4"` → `field_tianxing_39` 仍 6 槽、槽 0 作物仍在。**负向**：`ref_id field_ch13_probe2`、`slots "4"` → 新田恰 4 槽。
3. **门闸**：`ch13.rucheng` 为 0 → 栈顶 `"Dialogue"`（`ui.facility.locked`），不是 `"Field"`，`fields.size()` 不变；置 1 → `"Field"`。两态成对。
4. **采收带年份**：`FieldScene("field_tianxing_39").harvestAt(app, 0)` → true；背包 `herb_huangjing_cao` 多 1 株且那一堆 `herbAge == 60`；槽 0 `seedId` 空。**负向**：`harvestAt(app, 1)`（未熟）→ false、背包与槽 1 都不变；`harvestAt(app, 2)`（空槽）→ false。
5. **真数据（内容路落地后）**：`maps/ch13_tianxing.tmj` 的 `facility_yaoyuan_39` 恰是 2.2 的五个属性（`name / kind / ref_id / slots / require_flag`，0 个 `deny_text_key`）；`maps/ch11_dongfu.tmj` 的同名设施 `ref_id` 相同、`slots == 6`——辅助函数 `fieldSlotsByRefId(maps)` 返回每个 `ref_id` 在各图上的 `slots` 集合，对 `field_tianxing_39` 恰 `{6}`。**负向**：同一函数喂内存里把 `ch13_tianxing` 那一处改成 8 的副本 → 集合 `{6, 8}`；对 `shenshougu_yaopu` 返回 `{6, 8}`（先例本来如此，本条只钉 `field_tianxing_39`，不立「全仓同 `ref_id` 同尺寸」的规矩）。任一图不存在即 `FAIL()`（集成后；施工期为 `GTEST_SKIP`，第 13 章门禁审查 G-4）。
6. **门禁（只读，不改 `tools/`）**：内容路落地后 `python tools/validate.py` 对两张图同一 `ref_id` 不出任何一条报错（交付时贴日志里与 `field_tianxing_39` 相关的行数：0）。

### 2.4 实现记录（引擎路交付时填）

2026-10-13 引擎路交付，`src/`、`tools/` 零改动。E2 共 10 条用例（正向 7、单列负向 3；第 5、6 条另有用例内负向段）。夹具 `Ch13FieldFixture` 继承 `Ch13Fixture`，`SetUp` 先验「`fields` 恰 6 块、`field_tianxing_39` 恰 6 槽且全空」，再只在内存里种 2.3 头那两株（`plantedDay` 记当天，不拨日、不触发生长）。

- **第 1 条** `TheFieldInTheSaveOpensAsItIsWithoutANewFieldAndWithTheCropUntouched`（槽 0、槽 1 的 `seedId / age / ripe` 逐字段；`FieldScene::onEnter` 只同步 `maxAge`，契约本就不比它）；负向 `NegativeAnIdTheSaveLacksIsBuiltEmptyAtTheWrittenSize`（7 块、新田 6 槽全空）。
- **第 2 条** `ADifferentSlotsOnTheSecondMapDoesNotResizeTheExistingField`；负向 `NegativeANewIdWithSlotsFourIsBuiltWithExactlyFour`。
- **第 3 条** `RuchengUnsetShowsTheLockedMessageAndSetOpensTheField`：比契约多一段——门闸未开时换一个存档里没有的 `ref_id` 也只播 `Dialogue`、不建田（`fields` 仍 6）。
- **第 4 条** `HarvestingTheRipeHerbPutsOneSixtyYearStalkInTheBagAndEmptiesTheSlot`（`itemCount` ＋1、`itemCountOfAge(…, 60)` ＋1、槽 0 腾空）；负向 `NegativeAnUnripeSlotAndAnEmptySlotRefuseAndNothingMoves`。
- **第 5 条** 两条：`TheHelperSeesThePrecedentAtSixAndEightAndAnEightCopyOfTheField`（随时跑：`fieldSlotsByRefId` 对 `ch01_shenshougu ＋ ch02_yaopu ＋ ch11_dongfu` 给出 `shenshougu_yaopu {6, 8}`、`field_tianxing_39 {6}`；负向是把 `ch11_dongfu` 的副本改名、`slots` 改 8 塞进去 → `{6, 8}`）；`TheTianxingFacilityCarriesExactlyTheFourPropertiesAndTheFieldIsSixEverywhere`（读全仓 `maps/*.tmj`；`ch13_tianxing` 那一处 `name` 之外恰 `kind / ref_id / slots / require_flag` 四个属性、值照 2.2，`ch11_dongfu` 同名设施 `ref_id` 同、`slots` 6，全仓 `{6}`；负向在内存里把 `ch13_tianxing` 那一处改 8 → `{6, 8}`）。缺图原为 `GTEST_SKIP`，集成时（`integ13`）改成 `FAIL()`；交付时图已落地，实跑全绿。
- **第 6 条** 门禁日志：`python tools/validate.py` 2026-10-13 23:41（四张新图刚落、别路数据在途）81 错，全是别路在途（新图上 role / 脚本 / 地名文案未落、guard 旗标暂不可达）；10-14 00:32 `build_logged.bat ch13eng` 里再跑一次 27 错（`ch13.path.*` 26 面未登记 ＋ `ch13_battle.json` 与 `battles.json` 重复 key 1，均归协调者 5.2）。两次**与 `field_tianxing_39`、`facility_yaoyuan_39` 相关的行数都是 0**，含 `facility` 的行数也是 0。另把这条的前提写成 C++ 判据 `TheGateNeverReadsRefIdAndBothFieldFacilitiesPassTheRulesThatReadThem`：`tools/validate.py` 全文 0 处 `ref_id`（契约第 0 节；计数器自带负向），图落地后再核门禁真正会读这两个对象的两条规则——规则 11 对象名图内唯一、规则 8 `require_flag`（`ch11.anju`、`ch13.rucheng`）已在 `data/flags.json` 登记。只读，不改 `tools/`。
- **负向自检**（scratch 复制树，同 1.4）：**C** `openFacility` 不论田在不在都新建 → 第 1、2、3 条红（3 红 / 24 绿）；**D** 采收不带年份 → 只有第 4 条红；**E** facility 不判 `require_flag` → 第 3 条与 1.3 第 6 条真设施那条红（2 红 / 25 绿）。日志 `build-ch13eng-neg-{C,D,E}.log`。

---

## 3. 第 14 章接口（本批**不实现**，写成约定；笔记 O.2 逐行照抄）

第 14 章施工图与它的契约照这张表接；要改表里任何一行，先改本文。

| 接口 | 本章留下什么 | 第 14 章怎么用 |
| --- | --- | --- |
| **开局** | 章末在 `ch13_waihai` 东南小岛洞口（n34 `trigger_dongkou` 那一格旁）、`ch13.done`；不 `ending()`、不 teleport；身后「一个男子声音」已响 | patch `scripts/ch13/banyao.lua` 末尾补一行 `teleport("ch14_…")`（磷火潭那张图）；第 14 章开场接这句话：风某现身、自称化形第二阶、居磷火潭（都在 ch542），自报「风希」（ch543） |
| **境界** | 32 / 32（玩家在静室冲成；n28 已判）；`realm.cap` 停在 32 | ch544 冰珠异动破后期：`realm.advance(realm.CORE_LATE)` 一处（顺带抬上限）；第 14 章是**第一章在结丹中期打仗**，结丹中期攻防锚点（76 / 58，「暂定」，`Realm.cpp` 第 258、276 行；`Realm.h` 第 175 行是说明它们的注释）由第 14 章 B 项标定 |
| **法术** | 14 门（含五行环、蜂云合剑） | 「同御二十四柄」（ch544）若做，在蜂云合剑上加威力或另建，第 14 章定 |
| **伴妖草、铜片** | `story_banyao_cao`、`story_tongpian` 各 1 | ch543 起风希索草？（第 14 章核）；ch546 铜片＝海族圣物 |
| **乾蓝冰珠** | story | ch542 中段（第 29 段：风某现身、自称化形第二阶之后，被带进洞府之前）他吞冰珠备自爆——本章**未演**（停在人声之前），第 14 章开场可接 |
| **玄阴经** | story | 秘术（换形诀）第 14 章再用时仍只在文案 |
| **养魂木** | 根须 1 或珠 1（Z3 二选其一） | 还魂术（ch566 起）若要它，**读两种 id 之一**（O-5） |
| **万年灵乳** | 1（文案「灵乳」） | ch544 炼酒时滴灵液——第 14 章文案可改称「灵液」（≥ 519 合法），物品不改 |
| **文思月、妙音门、范夫人、云天啸、妙鹤真人** | 文思月在秘市荒岛小洞府（`ch13.wenshi` 后不在图上）；其余只说话 | ch554–560 再闯妙音门、杀云天啸、妙鹤夹击——role `yun_tianxiao`、`fan_furen` 照用；妙鹤真人第 14 章新建 |
| **逆星盟少主** | n04 壮汉当面一句「圣祖唯一传人」（ch502 第 37 段）＋ K2 可再问，0 处出名（施工图 16.1 第 57 条）| 温天仁（570）在第 14 章出名 |
| **地图** | `ch13_waihai` 三区（黑石城已成废墟，ch531；北区入口 `trigger_feixu` 一句废墟旁白，施工图 16.1 第 49 条） | 第 14 章外星海深处另建；本章图不 patch |
| **规则 25** | `MEANS_LAST_CHAPTER = 13`；探针在 `feng_xi` ＋ `b14_fengxi_xiezhi` | 第 14 章写实那张编成后**无处可挪**（全作末章）：探针改为 selftest 在测试副本里**临时写一张 `chapter 15` 的合成编成**，验「范围外不报 / 改 14 报」，还原字节相同；`MEANS_LAST_CHAPTER = 14` |
| **时间** | 单次 ≤ 3600 | 照旧 |
| **战斗预算** | 剧情战第 1–13 章 47 场 | 第 14 章 **5 场** 到 52 |
| **地图预算** | 全作 79 张 | 大纲第 5 节地图上限回写为「80–85」（施工图 16.1 第 3 条、C7）；第 14 章在此之内 |
| **队伍** | 空；`party.*` 0 处 | 照旧（本行笔记 O.2 未列，按 E 表 37 补） |
| **Lua 只读 / 存档** | 本章 0 个新命令、仍 v9 | 第 14 章若要按 spawn 名落点或新命令，自己开引擎项（本行笔记 O.2 未列，按 O.1「Lua 只读」行补） |

---

## 4. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让几路对同一件事只有一种写法（id、旗标、数字一律以笔记为准；同一件事实的权威在施工图主表，本节只引用）：

- **境界**：`realm.cap(realm.CORE_MID)` 恰 1 处（`scripts/ch13/biguan.lua`，1.2 的写法）；`realm.advance`、`realm.demote` 全章 **0 处**；`chuguan.lua` 开头判 `realm.at_least(realm.CORE_MID)`，不到只一句、不置旗、不拨日（E 表 10–11）；脚本不读 `formerRealm`。这一条推翻第 12 章契约第 3 节「境界」行（B20），协调者先改那一行。
- **teleport**：只写图 id 或图 id ＋ 整数坐标，不传 spawn 名，坐标不许是 `(0, 0)`。笔记 N.4 共 7 条（三元 5、五元 2）：`scripts/ch12/lingquan.lua` 末尾 patch 的 `teleport("ch13_nanming")`；`yinjian.lua` → `ch13_tianxing`、`chuansong.lua` → `ch13_waihai`、`chucheng.lua` → `ch13_wuzhong`（都只传图 id，落点 spawn `haishang` / `chengwai` / `shiwu` / `andeng` 标 default）；`lidao.lua` → `teleport("ch13_waihai", 14, 46)`（秘市岸，坐标为笔记建议值，与任何对象不同格）；五元两条见下。**落地不触发 enter**：n01、n07、n13、n17 的挂点是 spawn 前方横贯通道的触发带，不压 spawn。挂 NPC 的传送（n06、n12）与首次置旗的传送（n16、n30），`flag.set` 与 `teleport` 写在顶格、早退用顶格 `return`。
- **往返黑石城（五元）**：`trigger_qiyuan_hang`（`ch13_wuzhong` 码头尽头）与 `trigger_hui_wuzhong`（`ch13_waihai` 西门外礁头）一律 `type trigger`、`mode enter`、`once false`、`guard_flag ch13.lifu`、`set_flag` 空，分别挂 `qiyuan_hang.lua` / `hui_wuzhong.lua` 两份脚本，每份 0 处 `flag.set(`；开头分别判 `ch13.biguan` / `ch13.lidao` 已置 → 一句 `return`，否则 `choice` 去 / 不去，取消或「不去」都在**顶格** `return`（如 `if go ~= 1 then return end`），`advance_days(12)` 与 `teleport(…)` 也都写在顶格——`kScriptTransfers` 对五元行同样要求顶格 teleport（`tests/ObjectiveTests.cpp` 第 719–723、890–892 行），写进 `if` 块就报「没有顶格的 teleport」；#6 落点写死坐标（施工图 3.1 #6）。一份脚本拿不到玩家坐标、也拿不到是哪个触发器起的事件，所以按来处拆成两份。
- **触发器与门闸**：同一格只取对象表里第一个 trigger，挂点错开格子（同格叠 enter 被规则 16 报「踏不响」）；`once true` 的必有 `set_flag`（＝ done_flag），**由脚本末尾自己 `flag.set`**；挂 NPC 的节点（n02、n04、n06、n12、n14、n15、n29、n32）脚本开头看 done_flag 判已演过；n14、n15 另在开头判前置（`mingzhu.lua` 判 `ch13.daoqiyuan`、`cunjin.lua` 判 `ch13.mingzhu`，未到一句 return，施工图 16.1 第 47 条）。portal / facility 不读 `visible_flag / hidden_flag`：portal 门闸用 `require_flag ＋ deny_text_key`（本章 0 个 portal），**facility 只写 `require_flag`**（施工图 16.1 第 45 条），**黑石城的店全挂在 NPC 脚本上、不做 shop facility**，兽潮（`ch13.shouchao`）后随 NPC 撤（E 表 23）。规则 13：每张图只从 default spawn BFS，三区的挂点靠 guard 旗标惰性、不靠隔断。规则 30：同图同 role 只认「一个的 `hidden_flag` 恰是另一个的 `visible_flag`」；`yi_jing` 两个对象分属两张图。
- **诱妖循环**（E 表 18，照第 11 章口径）：`trigger_youyao` interact、`once false`；开头 `ch13.biguan` 已置 → 一句 `return`；草 ≥ 1 且 `bottle.drops() ≥ 1` → `bottle.spend(1)` 看返回值 → `advance_days(3)` → `battle("b13_waixinghai_lieyao")`；胜扣草 1、`flag.set("ch13.lieyao", flag.get("ch13.lieyao") + 1)`，逃不扣，败 game over；缺草一句指寸金阁（灵石 < 10 时 `advance_days` 后兜底 `give` 霓裳草 1，施工图 3.2 n24）；缺液 `advance_days(5)` 后 `return`。n25 `trigger_shoushou` 判 `ch13.lieyao >= 2`，不足一句。
- **日历**：每句 `advance_days` ≤ 3600；n28 二十余年写两句 `advance_days(3600)`；n25 `advance_days(2415)`、n26 `15 ＋ 1080`、n20 / n22 各约一年；固定增量合计 11790（笔记 N.2），玩家打坐另计。
- **扣东西**：`take` 一律看返回值；偷渡（n12）、至元丹（n29）、铜片（n31）三处都有零成本的一支，选项只列付得起的（E 表 12、22、27）；余数写成 `local n = item.count("story_jinsi_can"); if n > 0 then take("story_jinsi_can", n) end`——`take(id, 0)` 在引擎里会被当成扣 1。`story_miaoyin_yaopai` 只读不扣（n04）。
- **整数旗标**：`ch13.lieyao`（计数）、`ch13.toudu`、`ch13.zhiyuan`、`ch13.tongpian`（各 1 / 2 / 3）用 `flag.set(name, n)`；开关旗标 `flag.set(name)`（笔记 J 表）。
- **法术**：`magic.learn("magic_ji_wuxinghuan")`（n01，金、`effect "stagger"`、`stagger 3`、`power 0`）与 `magic.learn("magic_fengyun_heji")`（n20，木）各恰 1 处、看返回值；`magic_mihuan_guniao` 只在 `nixing_rusheng.magics`。物品 `story_wuse_tonghuan` 不动。
- **配方 / 商店**：`recipe_zhiyuan_dan`（`requireFlag ch13.liandan`、不写 `guaranteed`）；炼器配方 0 张、`guaranteed` 全仓仍一张。商店 4 张（笔记 L.3）：`ch13_mingzhu_shougou` `entries []`、`sellRate 0.8`；全部由脚本 `shop(id)` 打开（三家在 NPC 脚本、秘市在挂点 `mishi.lua` 一次），不做 shop facility。
- **两场编成**（笔记 I 表）：全部 `hero_absent false`、`defeat_is_fatal true`、`rewards.spirit_stones 0`；① `b13_nanming_zhuibing` `can_escape false`、`drops []`；② `b13_waixinghai_lieyao` **就地改写**、两波、`can_escape true`、`drops` 六级 1 ＋ 五级 2；`killable_by` 非空且 ⊆ `weaknesses`（规则 24 与加载器只拒空、不查子集）；`charge` 四键 `every / mult / all / text_key` 都必写（`mult` 缺了或 ≤ 0，`src/io/DataLoader.cpp` 第 435–436 行整份拒；数值平衡路定）；开场白与蓄势文案在 `data/text/ch13_battle.json`，key `ch13.battle.<短名>.intro / .charge`。
- **章末**：`banyao.lua` 最后 `flag.set("ch13.done")`，不 `ending()`、不 teleport（E 表 28）。
- **文案**：`data/text/ch13*.json` 0 处 `{key.` 与 `{pad.`；全章 0 处笔记 M 表末段那串禁词（风希、风雷翅、碧焰酒、梵圣真片、温天仁、银月、紫罗极火、磷火潭、玄阴岛、万年灵液、只出不进、炉鼎、双修、困妖）；「黄道友」`lidao.*` 之前 0 处（ch508 笔误）、之后只许指黄明礼；章内先后照施工图 12.2（笔记 M 表末段为底）。

---

## 5. 门禁与既有测试的依赖

### 5.1 引擎路

本章不改 `src/`、不改 `tools/`。共享内容不齐时只报告 `tests/Ch13EngineTests.cpp` 的证据，不报全量 data 绿色；内容路未落地的那几条（1.3 第 8 条、2.3 第 5–6 条）按 `GTEST_SKIP` 交付，集成后由协调者重跑一次看实跑未跳过。

### 5.2 协调者：集成时落

- **规则 25 的表**（`tools/validate.py` 第 2289 行起；`MEANS_LAST_CHAPTER` 第 2385 行，2026-10-13 仍是 11，第 12 章集成后 12）：`CHAPTER_MEANS[13]` ＝ 拳（`hand`）、火（`learn magic_huodan_shu ch04`）、土（`learn magic_liusha_shu ch06`）、水（`learn magic_bingdong_shu ch06`）、金（`learn magic_ji_jinfu ch07`，外加 `("金","learn","magic_shijin_chongqun","ch12")`；**五行环不登**，施工图 8.2、16.1 第 46 条）、木（`learn magic_qingyuan_jianmang ch08`）、暗器（`give weapon_wuming_sixian ch07`）、剑（`give weapon_lieyang_jian ch06`）；`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]` ＝ `[("木","learn","magic_fengyun_heji","ch13")]`（n20 之后才开 ②，E 表 30）；`MEANS_LAST_CHAPTER = 13`。`check_means_sources` 只核五行与 `scripts/ch13/` 里真有 `magic.learn`，不查 `power`——正因如此，五行环（`power 0`、`effect stagger`）登上去门禁也不拦，却会让规则 25 对 `killable_by` 含金的敌人放水，所以不登。
- **selftest 探针**（`tools/validate_selftest.py`；行号以第 12 章集成后为准，现第 1801–1830、2466、2495–2526 行）：② 写实后 `wuji_haishou` 在第 13 章编成里是敌方、不再是「范围外」，探针挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`（2026-10-13 grep：`data/battles` 里只有这一张含 `feng_xi`）——`outside_role` → `data/roles/feng_xi.json`、`outside_battle` → `b14_fengxi_xiezhi`，先验改「`chapter == 14` 且敌方含 `feng_xi`」，`probe["chapter"] = 13`，措辞改「第 14 章」；`future_id` 同法替换，`scoped(14)` 应静、`scoped(13)` 应报；`MEANS_LAST_CHAPTER == 12` 的断言与用例名「末章12」改 13。临时写回的是测试副本，还原字节相同。
- `tests/LexiconTests.cpp` 表一补笔记 M 表 82 词 ＋ 禁词 2（`{"梵圣真片", 546, …}`、`{"困妖", 1038, …}`）；表二 `{13, 542}`（第 441 行）已在，不动。
- `data/flags.json`：笔记 J 表 **41 面**（J.1 链上 34 ＋ J.2 链外 7）在内容路派工之前登记，`ch13.lieyao` 说明写「计数，诱妖胜场」、`ch13.toudu / zhiyuan / tongpian` 写「1 / 2 / 3 三支」；`ch13.path.<id>` 13 条等 `data/pathactions/ch13.json` 落地后登记。
- `data/chapters.json` 加 `{"number":13,"numeral_key":"ui.chapter.13.numeral","title_key":"ui.chapter.13.title","done_flag":"ch13.done"}`；`data/text/ui.json` 加 `ui.chapter.13.numeral`「第十三章」、`ui.chapter.13.title`「逆星之乱」（E 表 2）。
- `data/text/battles.json`：**删**旧占位 `ch13.battle.waixinghai_lieyao.intro`（2026-10-13 在第 8 行）——② 的开场白由内容路 C 写进 `ch13_battle.json`，不删就是重复 key，`loadGameData` 整个起不来（第 11、12 章同一个坑）。
- `data/items/weapons/qingzhu_fengyun_jian.json` 的 note：「自带辟邪神雷」补「神雷随修炼时间恢复（ch504、519）」（B7）；只改 note。
- **KeyPrompts 白名单**（`tests/KeyBindingTests.cpp` 第 556、605 行两组 `allowed`）：本章文案 0 处占位则不动；若某条 `objective.ch13.*` 要带 `{key.*}`，两组各加一条、`.pad` 版同写、先验计数随之加。
- 回写（笔记 O.4）：大纲第 13 章（B8、B18、B27）、lore 九处（B1–B4、B10–B14、B22）、人物总表（B15 只加注、不改「政敌」；B21、B28、B29）；大纲第 5 节地图上限「65–75」→「80–85」与支线目标改为实际可达值（施工图 16.1 第 3、51 条）；`docs/ch12-design.md` 验收 20 与纪律 61 的「`lingquan.lua` 不 teleport」改成「仅末行一处 `teleport("ch13_nanming")`」（施工图 16.1 第 58 条）；**第 12 章契约第 3 节「境界」「金丝蚕」两行**与第 12 章笔记 O.2 同两行（B19、B20）——施工前改，免得两份契约各说各的。`docs/handoff.md` 第 3–4 节按惯例更新。

### 5.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/Ch12*`（第 12 章测试路写的通关 / 验收）| 与 `ch12-end-{first,second}.sav` 逐字段比；章节卡断「未完待续」 | `scripts/ch12/lingquan.lua` 末尾补 `teleport("ch13_nanming")` 之后重生成 `ch12-end-*`（`mapId ch13_nanming`、`position` 为 `haishang`）；章节卡断言改 `Opening(13)`；第 12 章验收 20 / 纪律 61 那条扫脚本断言（`lingquan.lua` 不 teleport）改成「仅末行一处 `teleport("ch13_nanming")`」（施工图 16.1 第 58 条）| 测试路 |
| `tests/UiStyleTests.cpp` `ChapterTableData` / `ChapterCards` | 2026-10-13 钉到第 11 章（第 203、232、241–242 行）；第 12 章集成后应钉到第 12 章、并断 `ch13.done` 不排卡 | 表长 13、末项「第十三章」「逆星之乱」`ch13.done`；最后一章改 `ch13.done` → `{Closing,13}` ＋ `ToBeContinued`，`headline`「第十三章　终」、`kicker`「逆星之乱」；「`ch13.done` 不排卡」那条改断 `ch14.done` | 测试路 |
| `tests/ObjectiveTests.cpp` `kScriptTransfers` | 第 594 行起；第 12 章集成后到 `ch12.*` | 三元 5 条：`{"ch12.done", "scripts/ch12/lingquan.lua", "ch13_nanming"}`、`{"ch13.yinjian", "scripts/ch13/yinjian.lua", "ch13_tianxing"}`、`{"ch13.chuansong", "scripts/ch13/chuansong.lua", "ch13_waihai"}`、`{"ch13.chucheng", "scripts/ch13/chucheng.lua", "ch13_wuzhong"}`、`{"ch13.lidao", "scripts/ch13/lidao.lua", "ch13_waihai"}`；五元 2 条 `{"ch13.lifu", "scripts/ch13/qiyuan_hang.lua", "ch13_waihai", "trigger_qiyuan_hang", "ch13_wuzhong"}`、`{"ch13.lifu", "scripts/ch13/hui_wuzhong.lua", "ch13_wuzhong", "trigger_hui_wuzhong", "ch13_waihai"}`（笔记 N.4）。O-11：先核 `ObjectiveRoute` 只认目的图、不认坐标（`lidao.lua` 带坐标落非 spawn 格） | 测试路 |
| `tests/ObjectiveTests.cpp` `kOffChainDoorKeys` | 只有 `ch01.done` | 不动：本章四张图之间没有一扇门，三处设施门闸 `ch13.rucheng`、`ch13.lifu` 都在链上 | — |
| `tests/NpcPresenceTests.cpp` `kOffChainFlags` / `kLastChapter` | 第 103 行起；第 133 行 `kLastChapter = 11`（第 12 章集成后 12） | 登笔记 J.2 七条，窗口照施工图第 11 节末表（`toudu` yirong→chuansong、`z2` daoqiyuan→mingzhu、`z1` mingzhu→cunjin〔`mingzhu.lua` 首谈分支须写在前〕、`z3` buzhen→lieyao、`shouchao` 与 `zhiyuan` chuguan→gongsun、`tongpian` lidao→jiaohuan）；`kLastChapter` 改 13 | 测试路 |
| `tests/PathActionTests.cpp` `realmCapOfChapter()` | 第 895–903 行到 `{11, Realm::CoreEarly}`（第 12 章集成后 `{12, CoreEarly}`） | 加 `{13, Realm::CoreMid}`（n27 抬上限在 `ch13.done` 之前）；本章 13 条全 `realm 0` | 测试路 |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | 加 `ch13-end` 两份（v9，本章不加存档字段） | 测试路 |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | ② 的 `drops` 是妖丹（剧情战，第 11 章同类先例）；红了按遭遇 / 剧情战分开判，不改编成 | 测试路 |
| `tests/Ch11EngineTests.cpp`、`tests/Ch12EngineTests.cpp`、`RealmCapTests.cpp`、`PanelTests.cpp` | 第 11 章 E5 钉「结丹中期被 `StoryCap` 挡」；第 12 章借 `b08_*`、`b11_tiandu_shi` | 不动：本文 E1 用自己的夹具与内存状态，不改任何既有用例的前提；`tests/` 下 0 处引用 `wuji_haishou` / `b13_waixinghai_lieyao`（2026-10-13 grep），就地改写不牵动既有用例 | — |

---

## 6. 白名单（按文件分；两两不重叠，共享文件归唯一一路）

### 6.1 引擎路

- `tests/Ch13EngineTests.cpp`（新；E1–E2 全部判据）
- `docs/interfaces-p3-ch13.md`（只填 1.4、2.4 与末尾「实现记录」）
- `src/**`、`tools/**`：**不在白名单**。判据红了先报协调者、改笔记与本文，再另派。

### 6.2 内容路 A（地图美术）

- `maps/ch13_{nanming,tianxing,waihai,wuzhong}.tmj`（新 4 张）、`maps/tilesets/terrain_ch13_*.tsj`（生成器要的话，新）
- `tools/mapgen/genmaps_ch13.py`（新）、`tools/mapgen/genmaps.py`（只加登记，排在第 12 章之后）
- `data/visual/maps.json`、`data/visual/battles.json`（只加条目；主题时辰背景只用现有名：`dock/day`、`town/dusk`、`valley/day`，① `dock_river`、② `cliff_top`）、`data/visual/looks.json`（只加本章条目）
- `tools/artgen/sprites.py`、`tools/artgen/sprites_enemies.py`（只加本章 role 的立绘登记；非人形只有青色巨虾与五级海兽）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` **上方的注释表**加 4 行 `# ch13_xxx -> bgm_yyy`，元组不动，理由同第 12 章契约 6.2）、`docs/audio.md`（同一张表）

### 6.3 内容路 B（数据）

- `data/roles/*.json`（新建 H 表 30 个；**就地改写** `wuji_haishou.json`〔`weaknesses` 补「剑」〕；`fan_furen.json`、`wen_siyue.json` 一个字不动）
- `data/items/**`（只新建 L.1 那 13 件；已有物品一件不改——`qingzhu_fengyun_jian.json` 的 note 归协调者）、`data/magics/*.json`（新建 L.2 那 3 门）、`data/recipes/alchemy/zhiyuan_dan.json`（新）
- `data/battles/b13_nanming_zhuibing.json`（新）、`data/battles/b13_waixinghai_lieyao.json`（就地改写）
- `data/shops/ch13_{nanming_fangshi,mingzhu_shougou,cunjin_ge,mishi_tan}.json`、`data/quests/q13_{qianyelu,gongsun_qiuyao,mingzhu_dingdan,haishou_xuanshang,yanghun_zhu}.json`、`data/pathactions/ch13.json`、`data/objectives/ch13.json`
- **平衡路**不另开白名单、不与 B / C 并行：**B、C 两路都收尾之后串行接手**，只改上面这些 role / battle / recipe 文件的数值（② 的 `rewards.cultivation` ≤ 60，O-10），测量记录写进编成的 `note`。

### 6.4 内容路 C（脚本文案）

- `scripts/ch13/**`（新）；`scripts/ch12/lingquan.lua`（**只在末尾 `flag.set("ch12.done")` 之后补一行 `teleport("ch13_nanming")`**，此前一个字不动）
- `data/text/ch13*.json`（新，按前几章的拆法：正文、`ch13_battle.json`、`ch13_path.json` 等；0 处 `{key.` / `{pad.`）
- `docs/ch13-design.md`（**只追加「施工偏差」一节**；平衡路实测由协调者从 `note` 并入）

### 6.5 协调者

- `tools/validate.py`（**只** `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段）、`tools/validate_selftest.py`（只挪探针与 `MEANS_LAST_CHAPTER` 那一处断言）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第十三章」「逆星之乱」）；`data/text/battles.json`（**只删**那一条旧占位）；`data/items/weapons/qingzhu_fengyun_jian.json`（只改 note）
- `tests/LexiconTests.cpp`（表一补词）；`tests/KeyBindingTests.cpp`（只在 5.2 那一条触发时）
- `docs/ch13-design.md`（**只在**内容路 C 追加完「施工偏差」之后，把平衡路写在编成 `note` 里的实测并入第 18 节；不与 C 同时写）
- `docs/interfaces-p3-ch12.md`（只改第 3 节「境界」「金丝蚕」两行）；`docs/ch12-design.md`（只改验收 20 与纪律 61 两行，施工图 16.1 第 58 条）；`docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（含 `人物总表.md`，回写）

### 6.6 测试路（引擎路与内容路都落地、协调者 5.2 之后另派）

- `tests/Ch13*.cpp`（除 `Ch13EngineTests.cpp`）：通关、挂点、编成、账、切片、验收、章内先后词表专项（笔记 M 表末段）
- `tests/Ch12*.cpp`（除 `Ch12EngineTests.cpp`；只改通关终点与章节卡断言）、`tests/UiStyleTests.cpp`（只改 `ChapterTableData` / `ChapterCards` 那两处）
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`、`tests/BattleRewardTests.cpp`（只按 5.3）
- `tests/ChapterFixture.h`、`tests/fixtures/ch12-end-{first,second}.sav`（重生成）、`tests/fixtures/ch13-end-{first,second}.sav`（新）、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记 J 表旗标、改第 12 章契约两行 → 引擎路（E1–E2 判据）∥ 内容路 A / B / C → 平衡路（B、C 收尾之后）→ 协调者 5.2 与集成 → 测试路（先改第 12 章通关终点、重生成 `ch12-end`，再写第 13 章通关）→ 独立校对。**本章在第 12 章集成之后开工**；第 12 章仍在同一棵工作树里施工 / 审查时，各路只碰自己白名单里的文件。
- **跨路依赖**：引擎路与内容路之间没有字段依赖（本章 0 个代码项）；1.3 第 8 条、2.3 第 5–6 条要等内容路 A / C 落地，之前 `GTEST_SKIP`。
- 每路用自己的构建槽：引擎路 `ch13eng`、内容路 `ch13content`、测试路 `ch13test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。几路共用一棵工作树，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- **门禁先于编译跑**：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`；引擎路交付前跑 `python tools/validate_selftest.py`（仓库本身不净时它连锁 FAIL，照第 11 章实现记录写明与本路无关的条目）。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- **旧图不 patch**：本章 0 张；`genmaps_ch05.py`–`genmaps_ch12.py` 一个字不改。**旧脚本只** `scripts/ch12/lingquan.lua` 末尾补那一行 `teleport`；`scripts/ch13/banyao.lua` 不 teleport（留给第 14 章 patch）。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- 原著文本只在本地核对事实（`tools/novel/novel.py`），不复制、不进仓库；对白字句自己写，换词不算重写。
- **不派子代理**，不整份重读大文件，报告精炼。**不碰 git**：各路不 add / commit / stash / checkout，提交由协调者按 `docs/handoff.md` 第 3 节的流程做；不动 `fanren/build*` 目录。

---

## 实现记录

- **引擎路（2026-10-13 至 10-14，构建槽 `ch13eng`）**：只新增 `tests/Ch13EngineTests.cpp`（27 条：E1 17、E2 10；正向 18、单列负向 / 对照 9；读内容路文件、缺文件即 `GTEST_SKIP() << "等内容路 <文件> 落地"` 的 5 条——1.3 第 8 条三条、2.3 第 5 条后一条、第 6 条后半段；集成时由协调者改成 `FAIL()`），`src/`、`tools/` 零改动；本章 0 个代码项，判据**没有一条红**，引擎无缺口。
  - **全量构建**：`build_logged.bat ch13eng` 两次都停在门禁（`build-ch13eng.log`，`EXITCODE=1`）：23:38 是 `lingquan.lua` 已 patch、`ch13_nanming` 图未落（1 错）；00:32 是 27 错（见 2.4 第 6 条），全是别路在途、归协调者 5.2。绕过门禁的编译（`build-ch13eng-compile.log`，同一套 vcvars ＋ cmake 步骤）`EXITCODE=0`、零警告。
  - **测试**：`Ch13*` 27/27 绿、0 跳过——内容路的四张图（23:40）与 `biguan.lua` / `chuguan.lua`（23:52）在交付前落地，5 条 `GTEST_SKIP` 守卫全部实跑。**集成注意**：内容路 C 落下 `data/text/ch13_battle.json` 之后，`battles.json` 第 6 行旧占位 `ch13.battle.waixinghai_lieyao.intro` 未删（契约 5.2，协调者）时 `loadGameData` 报「文案 key 重复」、`Application::init` 失败，本文件 18 条依赖整仓数据的用例全红，且第 10–12 章通关驱动（`root()` 取自 `__FILE__`，绕不开仓库数据）SEH 连锁、全量测试在 Ch11Acceptance 处栈溢出中止；在只删这一行的数据快照上（scratch，`mkroot.py`）`Ch13*` 27/27 绿。占位删除前最后一次完整的全量（23:43）：1972 条，1933 绿 / 39 红 / 1 跳过（即本文件当时未落的脚本扫描那条）；39 红全在别路——`Ch12*` 16 条（`lingquan.lua` 补了 teleport，`ch12-end` 夹具与章节卡断言待测试路 5.3）、`UiStyleTests` 章节表 / 卡 3 条（表长已 13，测试路）、`ObjectiveTests` 4 条与 `NpcPresenceTests` 1 条（`objective.ch13.*` 文案未落、`kScriptTransfers` / `kOffChainFlags` 待测试路）、`PathActionData` 15 条（`ch13.path.*` 文案 / 旗标未落）。
  - **`python tools/validate_selftest.py`**（`-u`，23:57–00:28）：411 条、未如期抓住 23 条、`SELFTEST_FAIL`——23 条全是「不动的副本 → 全量校验干净 / 通过」一类的基线用例，红在仓库本身不净（`ch13_battle.json` 重复 key、`ch13.path.*` 未登记），与本路文件无关（第 11 章实现记录同一口径）。`genmaps.py --check` `MAPGEN_IN_SYNC`（79 张）；`artgen.py --check` 红在 `data/visual/maps.json` 缺四张新图的预设（内容路 A 在途）。
  - **负向自检** 8 处（契约 2 处 ＋ 自加 6 处），全部只红在预期的用例上，记录见 1.4、2.4；日志 `build-ch13eng-neg-{base,A..H}.log`。
  - **两份文档的出入**：无矛盾。三处按字面落不了、按意图落的写在 1.4 第 6、8 条与 2.4 第 3、6 条（`meditationSeed` 文件内私有、「第一个可执行语句块」取首条活代码行、门闸未开不建田多钉一段、第 6 条的 C++ 前提判据）。另：施工期间共用 scratchpad 里有别的代理也写 `selftest.txt`，本路第一次 selftest 的输出被截断作废、重跑进了 `scratchpad/ch13eng/`——各路的临时文件应放各自子目录。

