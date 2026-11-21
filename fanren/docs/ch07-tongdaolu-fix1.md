# 第 7 章第二期「同道录」2a·返修第一批 施工记录

> 2026-11-21。来由：`docs/ch07-tongdaolu-integration.md` 第 2、5 节走查时记下、用户定了「先修再试玩」的 6 条。改法由主会话定，本文记实际怎么改的、与定下的行为有没有出入。
> 分支 `claude/tongdao-fix1-9e7cb1`（从 main `369932b` 切出），不推送、不合 main。构建目录 `build-f`（Intel Mac，CMake + Ninja + ctest，不入库）。门禁没跑（用户定的规矩）。

---

## 0. 一页结论

| 条 | 现象（修前） | 改成 | 主要文件 |
| --- | --- | --- | --- |
| 1 | 名额没满时加成全都自动生效，「带上在册加成」那一行却写「未带上」，勾不勾没区别 | 已解锁的总数不超过名额：这一行灰着写「名额未满，自动生效」，确认不切；超过名额才能勾、行尾「已带上 / 未带上」 | `src/game/TongdaoPage.*`、`data/text/ui_tongdao.json` |
| 2 | 剑幕只管张开的那一回合，陈巧倩排在回合末出手时等于白放 | 管到**施术者下一次轮到行动**为止（被定身跳过、陪着出合招也算轮到），她倒下立刻散；标记播到散的那一条事件才收 | `src/core/battle/*`、`src/core/model/Types.h`、`src/game/BattleScene.cpp`、`src/game/BattleFx.h`、`data/text/ui_tongdao.json`、`data/text/tongdao.json`、`data/magics/jianmu.json` |
| 3 | 切磋打赢，战果卡写「此战别无所得」，紧接着回话报「首胜之礼」 | 切磋打赢且卡上没有掉落行：副标题「点到为止」 | `src/game/BattleScene.*`、`data/text/ui_tongdao.json` |
| 4 | 赠礼列表按储物袋次序，合意的可能在第二页 | 二档起：合意的最前、不标的居中、不喜的垫底，每组里保持储物袋次序；一档不变 | `src/game/TongdaoPage.cpp` |
| 5 | 战斗里择法术 / 物品时，说明区先画上一手的战报，效果说明挤到最后一行、半句硬截 | 这两个选单不画上一手战报；在这一级刚被拒时理由占顶上一行、光标一动就让位；效果说明至少两行（没有提示时三行），放不下末行收「…」 | `src/game/BattleScene.*` |
| 6 | 同道录卡片那一行用详情的长句，被截断加「…」 | 新键 `tongdao.<id>.brief`，卡片优先用它；详情仍用 `intro` | `src/game/TongdaoPage.*`、`data/text/tongdao.json` |

与主会话定下的行为相比，**有两处出入**（都在 §11 写了理由）：第 2 条把「陪着出合招」也算作轮到她；第 5 条的「这个选单里刚产生的拒绝提示」落在「选中就出的法术 / 符在发起时被拒」上，被拒后留在原选单（原先退回根菜单）。其余照定下的做。存档版本、入库存档、红线用例都没动，没加存档字段与旗标。

---

## 1. 「带上在册加成」那一行

- `tongdaoActionRows`（`src/game/TongdaoPage.cpp`）：判完「没有在册加成」「还没生效」两种拒绝（照旧、在前）之后，再看 `tongdaoUnlockedBonusCount(app)`（新函数，数同道录里 `rules::rosterBonusUnlocked` 为真的几位）是否 `<= rules::rosterBonusSlots(realm)`：是就 `refuse(row, "名额未满，自动生效")`——灰着、行尾写这句；按确认走页面原来的拒绝路（`activate` 重问一遍、响 `ui_error`、不切）。超过名额才照旧能勾、行尾「已带上 / 未带上」。
- 规则层（`rules::activeRosterBonuses`、路 K 8.8）一个字没动。
- 新文案 `ui.tongdao.act.bonus.auto`「名额未满，自动生效」（`data/text/ui_tongdao.json`）。
- 光标落不到灰行上（`ui::ListView` 的上下键跳过禁用项），所以「按确认不切换」在界面上本来就按不到；测试另验了整列都灰时确认键什么都不改。

## 2. 剑幕的时效

**规则层**（`src/core/battle/`）：
- `Unit::Status::bulwarkRound` 换成 `bulwarkCaster`（张剑幕的人的单位下标，-1 = 没有）；`finalDamage` 只看 `bulwarkPercent > 0`。施放时照旧罩施放那一刻还站着的己方全员，成数不变；重放就是覆盖（刷新）。
- 新私有函数 `BattleState::dropBulwarkOf(caster)`：把这位张的剑幕从所有人身上撤掉，每撤一位发一条新事件 `BattleEventKind::BulwarkEnd`（追加在枚举末尾，`actor` = 施术者，`target` = 那一位），日志一句「某某 的剑幕散去」。调它的四处：
  1. `beginActorTurn`：轮到她，先散旧的，再由她决定这一手（散在 `GuardEnd` 之前）；
  2. `skipIneligibleActors` 的定身分支：她被定身跳过也算轮到，散在 `BoundSkip` 之前；
  3. `applyDuo`：她被拉去陪着出合招（这一回合的行动用掉了）时当场散——与守势在这里散是同一个口径（**主会话没写这一种，见 §11-1**）；
  4. `emitStrike`：她倒下（`Fall` 之后）立刻散。
- 规则层日志那一句改成「……，到其下次出手前众人受伤减 N%」。
- **战斗状态不进存档**：核过 `src/io/SaveFile.cpp`，存的是 `bonds`、`flags` 等，没有任何战斗中的状态（战斗里不能存档）。存档版本不变。

**界面**（`src/game/BattleScene.cpp`、`BattleFx.h`）：`syncShown` 与择人说明区（`allyStatusText`）只看 `bulwarkPercent > 0`；`RoundStart` 不再收剑幕标记；新事件 `BulwarkEnd` 播到哪一位收哪一位的标记（0.05 秒一拍），战报写「某某的剑幕散去」。`BattleHud`、`BattleView` 读的是显示副本里的 `bulwark`，没改代码，只是时机跟着事件走了。

**文案**（措辞：统一成「到施术者 / 其 / 她下次出手前」）：
| 键 | 改前 | 改后 |
| --- | --- | --- |
| `ui.tongdao.magic.bulwark`（法术说明） | 这一回合我方全员受伤减 {n}% | 到施术者下次出手前，我方全员受伤减 {n}% |
| `ui.tongdao.battle.log.bulwark`（战报） | {actor}布下剑幕，本回合众人受伤减 {n}% | {actor}布下剑幕，到其下次出手前众人受伤减 {n}% |
| `ui.tongdao.battle.log.bulwark_end`（新） | — | {actor}的剑幕散去 |
| `magic.desc.magic_jianmu`（`data/text/tongdao.json`，法术描述） | ……这一回合挨的打都轻上三成。 | ……到她再出手之前，挨的打都轻上三成。 |
| `BattleAction.cpp` 拼的日志 | 本回合众人受伤减 N% | 到其下次出手前众人受伤减 N% |

`data/magics/jianmu.json` 的 `note`（数据注释）同步改了一句。教学卡（`data/tutorials/ch07.json`、`tutorial.tongdao*`）里没有写剑幕或「这一回合」，不用改。法术行尾「全队减伤 30%」不带时效，没改。

**谁会放剑幕 / 对现有战斗的影响**：
- 敌方 AI（`BattleState::enemyDecision`）只挑 `offensiveMagic` 为真的法术，剑幕不伤人，**敌方（含切磋里当对手的陈巧倩）从来不放**；切磋编成 `spar_chen_qiaoqian.json` 的注释也写着「剑幕不放」。
- 我方自动打法（`allyDecision`）只多判「回血」一种；测试那只手（`tests/BattleHand.h`）只在被告知要放剑幕时才放（`picksNobody`），自己从不挑。
- 所以会放剑幕的只有：玩家菜单、截图口 `battle:<编成>:bulwark`、显式施放的测试（`TongdaoBattleCoreTests`、`TongdaoWiring.TheMenuOffersAlliesForHealAndNoTargetForBulwarkAndTheHandPlaysThem`、`TongdaoUiTests` 的战斗用例）。三位的切磋（`TongdaoWiring` / `TongdaoContent` / `TongdaoUi` 里的切磋用例）、历练明雷、各章通关的自动战斗**胜负与回合数都不受影响**——这些用例断言一条没改，全量照过（§10）。
- 唯一会变的是有人手动放剑幕的仗。走查 `TongdaoWalk.TheHongyanEventPictureAndAFightWithAllThree` 的历练一仗（火焰鼠 `bl07_huoyan_shu_t1`，陈巧倩头一手放剑幕）：修前修后都胜、战果一样（灵石 3 块、火鼠绒 ×1，图 `walk_y99_battle_result`），整仗从 1187 帧变成 1206 帧（多出来的是 `BulwarkEnd` 那几拍与之后少挨的伤），断言全过。
- 第 7 章主线不带陈巧倩同行、主线里也没人会剑幕：红线用例 `Ch07CompanionsTests` 的 `No5_…`、`Ch07AcceptanceTests`、`Ch08AcceptanceTests` 原样通过（§10）。

## 3. 切磋的战果卡

`BattleScene::buildResultCard` 的 `Won` 分支：照旧先按「有没有掉落行」取「此战别无所得 / 此战告捷」，然后**只在** `battle_.spar()` 且卡上没有掉落行时把副标题换成新文案 `ui.tongdao.spar.result.sub`「点到为止」。非切磋的仗、输了、逃了、敌人跑了都不碰。另加只读口 `BattleScene::resultCard()`（测试看副标题用）。

## 4. 赠礼列表的次序

`TongdaoPage::rebuildGifts`：每一件先连同它的 `GiftChoice` 成对收进一个临时表，带一个组号（二档起：合意 0、不标 1、不喜 2；一档全是 1），`std::stable_sort` 按组号排，再一起拆成行与 `giftChoices_`——两边同序，送出去的一定是光标指的那一件。一档不知道喜好，次序与储物袋相同。多过一页时让一行给刻度的那段（集成 4.2）照旧。

## 5. 战斗里择法术 / 物品时的说明区（各章共用）

先读清了 `feedback_`：只在 `issueFromMenu` 里写——成功时写这一手的战报（随即关菜单），失败时写拒绝理由（随即退回根菜单）；只在进场（`onEnter`）时清。所以法术 / 物品选单里看到的 `feedback_` **一定是上一手留下来的**。而灰着的行玩家按不到（`ListView` 上下键跳过禁用项，理由本来就写在行尾），这两个选单里真正「刚产生」的拒绝只有一种：**选中就出的法术 / 符（看破、剑幕，与装着它们的符）在发起时被拒**（局面在菜单开着的时候变了，比如那张符已经不在背包里）。

改法：
- 新自由函数 `layoutMenuNote(notice, effect, width, maxLines, measure)`（`BattleScene.h`，可单测）：提示至多一行（放不下截断加「…」）；效果说明占余下的行（有提示时 2 行，没有时 3 行），仍放不下时末行以「…」收尾，不再半句硬截。
- `renderMenu`：法术 / 物品选单一律走这一排法，**不画 `feedback_`**（光标停在「返回」上也不画）；说明区仍是 72 高、15 号字、行高 20（三行）。择敌 / 择人、主选单、合招、兵刃几级没动。
- `issueFromMenu` 失败时：若在法术 / 物品选单，**留在这一级**（照此刻重拼列表，光标尽量停回原处），把理由记作这一级的提示（`menuNotice_` / 行号 / 哪一级）；别的几级照旧退回根菜单。`feedback_` 照旧写上（回到根菜单时那里照旧显示）。
- 提示什么时候让位：光标一动（`updateMenu`）就清；换一级菜单（每个 `enter*`、`closeMenu`）就清；画的时候还要求光标仍在记下的那一行。只读口 `BattleScene::menuNotice()`。

改前改后（同一个走查镜头）：定身符的说明修前被上一手的战报挤到最后一行、截在「首领与」（`before/walk_y_han_yunzhi_0_magic_dingshen`），修后整句两行（`after/` 同名）。

**钉着这块版面的旧用例**：全量跑下来**没有一条因第 5 条变红**（现有像素 / 版面用例没有钉法术说明区的字），所以第 5 条没有改任何旧用例的期望。

## 6. 同道录卡片上的一行简介

- `data/text/tongdao.json` 新增三条（照主会话给的原句，没压字）：`tongdao.chen_qiaoqian.brief`「陈家小姐，双飞剑，专会补刀」、`tongdao.nangong_wan.brief`「出身掩月宗，朱雀环火势逼人」、`tongdao.han_yunzhi.brief`「灵兽山弟子，扔符是行家」。键名里的 id 照 `.intro` 的写法。
- `TongdaoEntry` 加 `brief`：遇到过的优先取 `.brief`，没有退回 `intro`（画时放不下仍截断加「…」）；没遇到的同 `intro`（提示 `.hint`，没动）。`drawCard` 画 `brief`，详情页照旧画 `intro`。
- 三句在真字体 16 号下都比卡片文字宽（260 − 24 = 236 像素）窄，整句放下、没有省略号（测试量过，图 `after/f6_cards`）。
- 路 N 钉文案的 `TongdaoContent`（三位都要有 `hint` / `intro`）照实补了 `brief`。

---

## 7. 改过期望的旧测试

| 用例 | 改前 | 改后 | 理由 |
| --- | --- | --- | --- |
| `TongdaoBattleMagic.ABulwarkCutsDamageForTheWholeSideUntilTheRoundEnds`（`tests/TongdaoBattleCoreTests.cpp`） | 名字与注释钉「过了这一回合剑幕就散了」 | **改名** `ABulwarkCutsDamageForTheWholeSideUntilTheCasterActsAgain`；原有断言一条没删（这一局里韩立在下一回合先于恶狼出手，所以「恶狼那一下照常」照样成立），补了「记着是谁张的」「轮到他时罩着的两位各散一条 `BulwarkEnd`」 | 第 2 条：时效从「本回合」改成「到施术者下次轮到」 |
| `TongdaoUi.MagicDescriptionsNameTheNewEffectsWithNumbersFromTheTable`（`tests/TongdaoUiTests.cpp`） | 剑幕说明含「这一回合我方全员受伤减 30%」 | 含「到施术者下次出手前，我方全员受伤减 30%」 | 第 2 条的文案 |
| `TongdaoUi.AtBaiyaoyuanEachActionAsksTheRulesAndMissingScriptsSayContentNotYet` | 末句 `EXPECT_TRUE(r[Bonus].enabled) << "请教过：加成解锁，能勾"` | 灰着、理由「名额未满，自动生效」 | 第 1 条：只解锁一条、没超名额 |
| `TongdaoUi.TheBonusToggleWritesOnlyItsOwnFlag` | 只解锁 probe_a 一条就去勾 | 先让 probe_d…g 也结识、请教过（共 5 条，超过名额 4）再勾；**断言本身没改** | 第 1 条：名额没满时这一行不能勾，勾选只在超额时有意义 |
| `TongdaoUi` 测试树（`SetUpTestSuite`） | probe_d…g 没有在册加成 | 各加一条 `talisman_power 10`（它们平时没遇到，不作数）；另加编成 `probe_plain_a`（第 3 条对照）、物品 `talisman_probe_wall`（装着探针剑幕的符，第 5 条） | 给新用例造局面；别的用例不受影响（全量照过） |
| `TongdaoContent.ElevenTalksAndThreeLessonsRunHeadlessAndKeepTheirBooks`（`tests/TongdaoContentTests.cpp`）里「三位都有」那段 | 查 `hint`、`intro` | 再查 `brief` | 第 6 条（任务要求「照实补上」） |

红线用例（`Ch07CompanionsTests` 的 `No5_…`、`Ch07AcceptanceTests`、`Ch08AcceptanceTests`）**没改、照过**；`saves/`、`tests/fixtures/` 一个字节没动（`git status` 为证）。

## 8. 新加的测试（14 条）

| 文件 | 用例 | 钉什么 |
| --- | --- | --- |
| `tests/TongdaoBattleCoreTests.cpp` | `ABulwarkCastLastInTheRoundStillCoversTheNextRoundUntilHerTurn` | 回合末施放，下一回合抢在她前头的敌人照样吃减伤；轮到她时散（两条 `BulwarkEnd`、状态清零） |
| 同上 | `ABoundCasterLosesHerTurnAndHerBulwarkWithIt` | 她被定身跳过也散，散在 `BoundSkip` 之前；只罩她那一边 |
| 同上 | `ABulwarkFallsWithItsCaster` | 她倒下立刻散，紧跟 `Fall`，别人身上的也散 |
| 同上 | `RecastingABulwarkRefreshesIt` | 轮到她时旧的先散，再张开的照新成数（30%），管到她再下一次 |
| 同上 | `BeingCalledIntoADuoCountsAsHerTurnForTheBulwark` | 她陪着出合招那一刻散 |
| `tests/TongdaoUiTests.cpp` | `TongdaoUi.TheBonusRowIsGreyedWhileEveryUnlockedBonusFitsAndOnlyTogglesOverTheLimit` | 第 1 条：两种旧拒绝在前；4 条 / 名额 4 灰着写「名额未满，自动生效」；整列都灰时确认什么都不改；5 条超额才亮、行尾已带上 / 未带上 |
| 同上 | `TongdaoUi.GiftListPutsLikedFirstAndDislikedLastAndGivesTheRowUnderTheCursor` | 第 4 条：一档照储物袋；二档合意 → 不标（保持原序）→ 不喜；光标挪到末行送出的就是那一件 |
| 同上 | `TongdaoUi.CardsUseTheShortBriefAndFallBackToTheIntro` | 第 6 条：三位用短句、详情仍是长句；没有短句退回 intro；没遇到的是提示 |
| 同上 | `TongdaoPixels.TheThreeBriefsFitOnTheCardWithoutAnEllipsis` | 第 6 条：真字体下三句都放得下卡片（长句本来放不下） |
| 同上 | `TongdaoBattleShots.AWonSparWithNothingToShowSaysItStoppedShortAndAPlainFightIsUnchanged` | 第 3 条：切磋赢「点到为止」；同样不掉东西的寻常一仗仍「此战别无所得」 |
| 同上 | `TongdaoBattleShots.TheBulwarkMarkStaysOverTheRoundBreakAndClearsWhenTheCasterComesUp` | 第 2 条（画面）：翻过回合（`RoundStart`）标记还在，播到 `BulwarkEnd` 才收 |
| 同上 | `TongdaoBattleShots.ARefusalInTheItemMenuTakesOneLineUntilTheCursorMoves` | 第 5 条：上一手战报不进物品选单；符在发起时被拒 → 留在物品选单、提示一行、列表重拼；光标一动提示让位 |
| `tests/Ch03BattleMenuTests.cpp` | `BattleMenuNote.WithoutANoticeTheEffectGetsAllThreeLinesAndEndsWithAnEllipsisWhenItStillDoesNotFit` | 第 5 条排行：放得下整句；正好三行不加省略号；放不下末行收「…」且不出框 |
| 同上 | `BattleMenuNote.ANoticeTakesOneLineAndTheEffectKeepsAtLeastTwo` | 第 5 条排行：提示一行（长的截断）、效果说明两行 |

三条 `TongdaoBattleShots` 战斗用例按住加速键、只在开头画几帧，各不到 1 秒。

## 9. 截图

全部不入库，在 `build-f/shots/`（构建目录被忽略）。`before/` 是改动前的程序（基线构建另存的 `build-f/fanren.before`、`fanren_tests.before`）拍的，`after/` 是改完的程序拍的，同名一一对应：

| 文件（before/ 与 after/ 同名） | 怎么拍的 | 看哪条 |
| --- | --- | --- |
| `f6_cards.png` | 截图口 `tongdao:cards`（检查点 `ch07-chujindi-first.sav`） | 6：修前三张卡末尾「…」，修后整句 |
| `f1_bonus_chen_unfilled.png` | `tongdao:detail=chen_qiaoqian@bond…=50,flag.tongdao.chen_qiaoqian.qingjiao=1` | 1：修前「未带上」，修后灰着「名额未满，自动生效」 |
| `f1_bonus_han_locked.png` | `tongdao:detail=han_yunzhi@bond…=30` | 1：没解锁的照旧「还没生效」 |
| `f4_gift_han_tier2.png`、`f4_gift_chen_tier2.png` | `tongdao:gift=<id>@bond…=30` | 4：修前合意的丹砂 / 养精丹在第 6 / 1 行之后，修后排第一 |
| `f4_gift_chen_tier1.png` | `tongdao:gift=chen_qiaoqian` | 4：一档次序不变 |
| `f2_battle_bulwark.png` | `battle:bl07_huoyan_shu_t1:bulwark`（三位在队的截图档，由 `TongdaoPlaySaves` 写到 `build-f/saves/`） | 2：战报改成「到其下次出手前」 |
| `f5_battle_magic_first.png` | `battle:bl07_huoyan_shu_t1:magic` | 5：头一个行动者（没有上一手战报）——修前修后一样，对照用 |
| `walk_y_han_yunzhi_0_magic_dingshen.png` 等五张 `walk_y_*` | 走查 `TongdaoWalk.*` | 5：修前战报压两行、说明截在半句；修后整句。2：`walk_y_chen_shimei_0_magic_jianmu` 说明改成新时效 |
| `walk_y_chen_shimei_0_cast_jianmu.png`、`walk_y99_battle_result.png` | 走查 | 2：张开那一刻；整仗结果修前修后一样 |
| `walk_w01_cards.png`、`walk_w03_han_gift_list.png`、`walk_w11_han_detail_bonus_on.png`、`walk_w15b_spar_result.png`、`walk_w16_spar_after.png` | 走查 | 6、4、1、3（`w15b`：修前「此战别无所得」，修后「点到为止」） |

只有 `after/` 才有的（新用例里拍的，`FANREN_TONGDAO_SHOTS` 另存）：`test_fix1_bulwark_roundbreak.png`（第 2 回合开头，两人面板上「剑幕」还在）、`test_fix1_bulwark_end.png`（散了之后）、`test_fix1_item_refusal.png`（物品选单里一行「背包里已经没有这件东西了」）、`test_fix1_spar_result.png`（「点到为止」）、`test_fix1_plain_result.png`（寻常一仗「此战别无所得」）。

走查两条修前修后都过（`build-f/walk-before.log`、`walk-after.log`）。

## 10. 构建与测试

- 机器：Intel Mac（macOS 15.8）。`cmake -S . -B build-f -G Ninja -DCMAKE_BUILD_TYPE=Release`、`cmake --build build-f --parallel`、`ctest --test-dir build-f -j 12 --output-on-failure`。编译没有新告警。
- 基线（main `369932b`，动手前）：`100% tests passed out of 2540`（3 条环境变量辅助记 Skipped）。
- 门禁没跑（用户定的规矩）。

最终一遍（全部改动之后、提交之前，`build-f`，`-j 12`，日志 `build-f-final.log`）：**`100% tests passed out of 2554`**，`Total Test time (real) = 187.50 sec`——基线 2540 + 新加 14；3 条环境变量辅助照旧 Skipped。红线 23 条（`Ch07Acceptance` / `Ch07Walkthrough` 18、`Ch08Walkthrough` / `Ch08WorldSprite` 4、`Ch07CompanionsHandOver.No5_TheMinimalRouteEndingDiffersFromTheOldOneOnlyWhereSectionOneAllows` 1）全过，三个文件与 `saves/`、`tests/fixtures/` 对 main 零差异。

提交按「战斗」「同道录页」「文档」分三个；中间那个提交（只有战斗那一半）没有单独跑全量，全量跑的是最后的代码。

## 11. 拿不准的地方

1. **陪着出合招算不算「轮到她」**（第 2 条）。主会话写了「被定身跳过也算轮到」，没提合招。她被拉去陪着出合招时，这一回合的行动就用掉了，轮到她的那一格会被跳过——我照守势的现成口径（`applyDuo` 里当场散守势）在合招那一刻散剑幕。另一种做法是等她那一格到了再散（中间那几下仍减伤）。差别只在「同一回合里，合招之后、她那一格之前」挨的打，很小。
2. **第 5 条的「拒绝提示」**。玩家在这两个选单里按不到灰行（理由本来就在行尾），能在这一级「刚产生」的拒绝只有「选中就出的法术 / 符在发起时被拒」，实战里要「菜单开着的时候局面变了」才碰得到（测试用「菜单开着时把那张符从背包拿掉」造出来）。原先这种情况退回根菜单、理由写在根菜单的说明区；现在留在原选单、理由占一行。若主会话的本意是「按灰行也要出一行提示」，那得先让光标能停到灰行上（改 `ui::ListView`，影响所有列表），没做。
3. 第 5 条的「光标一动就让位」：光标在两头动不了（只剩一行可选）时按上下键不算「动」，提示留着，直到换一级菜单。
4. 第 4 条只按「合意 / 不标 / 不喜」三组排，不按「寻常 +2」再细分（行尾不标的都是一组）。
5. 剑幕的「重放」：她在剑幕还在时再放，旧的在轮到她的那一刻已经先散了，所以「刷新」其实是「散了再张」，日志会先说一句「剑幕散去」再说「布下剑幕」。

## 12. 留给主会话定的事

1. §11-1、§11-2 两处与定下行为的出入，认不认。
2. 走查里顺带看到、这一批没动的：法术行尾「耗法力 14 · 金 · 2 段 · 蓄劲连发」放不下时截成「2 段…」（`walk_y_chen_shimei_0_magic_jianmu`），是行尾而不是说明区，不在这 6 条里；定身符的战报「下一回动弹不得」口气与「下一次轮到它时动弹不得」略有出入。
3. `docs/ch07-tongdaolu-u.md` 第 6 节表格与第 10 节第 7 条、`docs/ch07-tongdaolu-n.md` 里写剑幕「下一回合开头散掉」「这一回合」的是当时的施工记录，没改（任务只要求改施工图与路 K 记录）；要统一的话再说。
