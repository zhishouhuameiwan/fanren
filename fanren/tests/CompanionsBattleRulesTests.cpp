// 伙伴改版·B 路「战斗」里不必开一整个 Application 就能钉住的几样：
//   * 战斗层的倍数读每张表的 battle_scale；写了 battle_sheet 的表，战斗帧的文件、格子、脚底、头顶经 SpriteAtlas 取；
//     旧表画出来与改版前逐项相同，首领 ×5 的老规矩对旧表不变（docs/interfaces-companions.md 2.5）；
//   * 四人同场：AI 与全体攻击处处只认「活着的」（契约第 4 节「四人同场」说没核过，这里核）。
#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Visual.h"
#include "game/BattleView.h"
#include "game/SpriteAtlas.h"

namespace {

using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattleEventKind;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;

fanren::core::CharacterSheet oldSheet() {
    fanren::core::CharacterSheet sheet;
    sheet.file = "chars/old.png";
    sheet.frameW = 16;
    sheet.frameH = 24;
    sheet.cols = 3;
    sheet.footY = 23;
    sheet.headY = 2;
    sheet.battle = {{"idle", {4, 5}}};
    return sheet;
}

fanren::core::CharacterSheet hdSheet() {
    fanren::core::CharacterSheet sheet;
    sheet.file = "hd/walk.png";
    sheet.frameW = 64;
    sheet.frameH = 60;
    sheet.cols = 4;
    sheet.footY = 58;
    sheet.headY = 3;
    sheet.scale = 2;
    sheet.battleScale = 2;
    sheet.battle = {{"idle", {5}}};
    sheet.battleSheet = fanren::core::BattleSheet{"hd/battle.png", 132, 84, 4, 82, 18};
    return sheet;
}

fanren::core::SpriteIndex indexWith(const fanren::core::CharacterSheet& sheet) {
    fanren::core::SpriteIndex index;
    index.sheets["look"] = sheet;
    index.roles["someone"] = "look";
    return index;
}

const fanren::game::ChapterDone kNoChapters = [](int) { return false; };

TEST(CompanionsBattleLook, AnOldSheetLooksExactlyAsBefore) {
    const fanren::game::BattleLook look =
        fanren::game::battleLookFor(indexWith(oldSheet()), "b_any", "someone", kNoChapters);
    EXPECT_EQ(look.file, "chars/old.png");
    EXPECT_EQ(look.frameW, 16);
    EXPECT_EQ(look.frameH, 24);
    EXPECT_EQ(look.footY, 23);
    EXPECT_EQ(look.headY, 2);
    EXPECT_EQ(look.scale, 4) << "缺省的 battle_scale";
    // 帧 5 在 3 列的表上是第 2 行第 3 格：(32, 24) 起 16×24——改版前的公式。
    const fanren::engine::RectF r = fanren::game::battleLookFrameRect(look, 5);
    EXPECT_FLOAT_EQ(r.x, 32.f);
    EXPECT_FLOAT_EQ(r.y, 24.f);
    EXPECT_FLOAT_EQ(r.w, 16.f);
    EXPECT_FLOAT_EQ(r.h, 24.f);
    EXPECT_FLOAT_EQ(fanren::game::battleScale(look.scale, false), 4.f);
    EXPECT_FLOAT_EQ(fanren::game::battleScale(look.scale, true), 5.f) << "首领 ×5 的老规矩";
    EXPECT_FLOAT_EQ(fanren::game::battleScale(true), 5.f);
    EXPECT_FLOAT_EQ(fanren::game::battleScale(false), 4.f);
}

TEST(CompanionsBattleLook, ASheetWithABattleSheetDrawsFromThatSheetsGridAtItsOwnScale) {
    const fanren::game::BattleLook look =
        fanren::game::battleLookFor(indexWith(hdSheet()), "b_any", "someone", kNoChapters);
    EXPECT_EQ(look.file, "hd/battle.png");
    EXPECT_EQ(look.frameW, 132);
    EXPECT_EQ(look.frameH, 84);
    EXPECT_EQ(look.footY, 82);
    EXPECT_EQ(look.headY, 18);
    EXPECT_EQ(look.scale, 2);
    // 帧 5 在 4 列、132×84 的格子上：(132, 84)。
    const fanren::engine::RectF r = fanren::game::battleLookFrameRect(look, 5);
    EXPECT_FLOAT_EQ(r.x, 132.f);
    EXPECT_FLOAT_EQ(r.y, 84.f);
    EXPECT_FLOAT_EQ(r.w, 132.f);
    EXPECT_FLOAT_EQ(r.h, 84.f);
    EXPECT_FLOAT_EQ(fanren::game::battleScale(look.scale, false), 2.f);
    EXPECT_FLOAT_EQ(fanren::game::battleScale(look.scale, true), 2.f) << "×2 的表首领不另放大（按整数取）";
}

// ---------------------------------------------------------------------------
// 只认活着的
// ---------------------------------------------------------------------------

Unit ally(const std::string& id, int hp) {
    Unit u;
    u.id = id;
    u.name = id;
    u.hp = hp;
    u.maxHp = 100;
    u.attack = 5;
    u.defence = 1;
    u.speed = 5;
    u.ally = true;
    u.magicsExhaustive = true;
    return u;
}

TEST(CompanionsBattleAlive, EnemyAiAndAnAllOutHeavyBlowOnlyTouchStandingAllies) {
    std::vector<Unit> units{ally("hanli", 100), ally("a", 100), ally("b", 0), ally("c", 100)};
    Unit boss;
    boss.id = "boss";
    boss.name = "boss";
    boss.hp = boss.maxHp = 500;
    boss.attack = 10;
    boss.speed = 50;
    boss.chargeEvery = 1;
    boss.chargeMult = 2.0;
    boss.chargeAll = true;
    boss.actions = 2;
    boss.magicsExhaustive = true;
    units.push_back(boss);
    BattleState battle;
    battle.setup(units, 7u);
    const int fallen = 2;
    ASSERT_FALSE(battle.units()[fallen].alive());

    for (int step = 0; step < 200 && battle.phase() == fanren::core::battle::BattlePhase::Ongoing; ++step) {
        const int actor = battle.currentActor();
        if (actor < 0) {
            battle.endTurn();
            continue;
        }
        const Action a = battle.decideAi(actor);
        EXPECT_NE(a.targetIndex, fallen) << "倒下的那位不是任何一手的目标";
        if (a.actorIndex >= 0) static_cast<void>(battle.apply(a));
        battle.endTurn();
    }
    int heavyHits = 0;
    for (const auto& e : battle.events()) {
        if (e.kind != BattleEventKind::Hit) continue;
        EXPECT_NE(e.target, fallen) << "横扫全场也不打倒下的人";
        if (e.actor == 4) ++heavyHits;
    }
    EXPECT_GT(heavyHits, 0) << "首领真的出过手";
    EXPECT_EQ(battle.units()[fallen].hp, 0);
}

}  // namespace
