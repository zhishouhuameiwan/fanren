// 战斗画面的纯计算（src/game/BattleFx.h、BattleView.h、BattleHud.h 里不碰引擎的那几样）：
// 站位、按类别挑特效、伤害数字的弹跳、震屏的衰减、行动序条的排版、BGM 的挑法、背景的挑法、
// 开战碎屏的几何、截图口的时刻、战果卡上那几行字。
//
// 这些东西只活在 render() 里的话，唯一的验法是截图——而截图看不出「第四个敌人的架势行压进了日志框」
// 「首领在第二波就不放首领曲」「钱在战果卡上叫了灵石」这种事。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleFx.h"
#include "game/BattleHud.h"
#include "game/BattleScene.h"
#include "game/BattleView.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::battle::Unit;
using namespace fanren::game;

Unit makeUnit(bool ally, int wave = 0) {
    Unit u;
    u.id = ally ? "hanli" : "wild_wolf";
    u.name = ally ? "韩立" : "野狼";
    u.hp = u.maxHp = 30;
    u.ally = ally;
    u.wave = wave;
    return u;
}

// ---------------------------------------------------------------------------
// 站位
// ---------------------------------------------------------------------------

TEST(BattleStage, EnemiesStandLeftAlliesRightAndEveryoneOnTheGround) {
    std::vector<Unit> units;
    for (int i = 0; i < 3; ++i) units.push_back(makeUnit(true));
    for (int i = 0; i < 6; ++i) units.push_back(makeUnit(false));
    const std::vector<StagePoint> at = standPoints(units);
    ASSERT_EQ(at.size(), units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        // 背景的地平线在 y≈384：脚底必须落在地面上，且不低于 HUD。
        EXPECT_GE(at[i].y, 400.f) << i;
        if (units[i].ally) {
            EXPECT_GT(at[i].x, 640.f) << "我方在右半场";
            EXPECT_LT(at[i].y, kPartyPanelRect.y - 16.f) << "我方的脚踩不进队伍面板";
        } else {
            EXPECT_LT(at[i].x, 640.f) << "敌人在左半场";
            // 脚下还有一行架势 / 破绽（约 50 像素），那一行不能压进左下的日志框。
            EXPECT_LE(at[i].y + 50.f, kLogRect.y) << i;
        }
    }
    // 同一边的人互不重叠：任意两人的脚底至少隔开 60 像素。
    for (std::size_t i = 0; i < at.size(); ++i) {
        for (std::size_t j = i + 1; j < at.size(); ++j) {
            if (units[i].ally != units[j].ally) continue;
            EXPECT_GT(std::hypot(at[i].x - at[j].x, at[i].y - at[j].y), 60.f) << i << " / " << j;
        }
    }
}

TEST(BattleStage, TheNextWaveTakesTheSameSpotsFromTheTop) {
    std::vector<Unit> units{makeUnit(true), makeUnit(false, 0), makeUnit(false, 0), makeUnit(false, 1),
                            makeUnit(false, 1)};
    const std::vector<StagePoint> at = standPoints(units);
    EXPECT_FLOAT_EQ(at[3].x, at[1].x) << "第二波从头占位";
    EXPECT_FLOAT_EQ(at[3].y, at[1].y);
    EXPECT_FLOAT_EQ(at[4].y, at[2].y);
}

TEST(BattleStage, ALoneEnemyStandsMidFieldNotAtTheTopRow) {
    const StagePoint alone = standPoint(false, 0, 1);
    const StagePoint top = standPoint(false, 0, 3);
    EXPECT_GT(alone.y, top.y) << "只有一个敌人时站中线，不贴着地平线";
}

// ---------------------------------------------------------------------------
// 特效
// ---------------------------------------------------------------------------

TEST(BattleStrike, EachCategoryGetsItsOwnEffect) {
    using namespace fanren::core;
    EXPECT_EQ(strikeLook(kCategorySword).sheet, FxSheetId::Slash);
    EXPECT_EQ(strikeLook(kCategoryBlade).sheet, FxSheetId::Slash);
    EXPECT_EQ(strikeLook(kCategoryFist).sheet, FxSheetId::Impact);
    EXPECT_EQ(strikeLook(kCategoryFire).sheet, FxSheetId::FireBurst);
    EXPECT_EQ(strikeLook(kCategoryPoison).sheet, FxSheetId::Poison);
    EXPECT_EQ(strikeLook(kCategoryWood).sheet, FxSheetId::Wind);
    EXPECT_EQ(strikeLook(0).sheet, FxSheetId::Wind) << "没有类别的法术（神识冲）是风旋";
    // 金光斩：刀光染金，不是白的那一道。
    const StrikeLook metal = strikeLook(kCategoryMetal);
    EXPECT_EQ(metal.sheet, FxSheetId::Slash);
    EXPECT_GT(static_cast<int>(metal.tint.r), static_cast<int>(metal.tint.b) + 60);
    EXPECT_EQ(std::string(strikeLook(kCategorySword).hitSfx), "hit_slash");
    EXPECT_EQ(std::string(strikeLook(kCategoryFist).hitSfx), "hit_blunt");
}

TEST(BattleStrike, APunchNeverDrawsABlade) {
    // 反例：拳脚打出刀光，是「按类别选特效」写反了一格的样子。
    using namespace fanren::core;
    EXPECT_NE(strikeLook(kCategoryFist).sheet, FxSheetId::Slash);
    // 带毒的火弹：元素的光效压过毒雾。
    EXPECT_EQ(strikeLook(kCategoryFire | kCategoryPoison).sheet, FxSheetId::FireBurst);
    EXPECT_EQ(std::string(castSfx(kCategoryFire)), "fire_cast");
    EXPECT_EQ(std::string(castSfx(kCategoryMetal)), "metal_cast");
}

TEST(BattleStrike, FxFramesStayInsideTheirSheet) {
    const FxSheet& aura = fxSheet(FxSheetId::BoostAura);
    EXPECT_EQ(aura.rows, 3) << "蓄劲三档各占一行";
    const fanren::engine::RectF third = fxFrameRect(aura, 2, 10.f, true);
    EXPECT_FLOAT_EQ(third.y, 80.f);
    EXPECT_LT(third.x + third.w, 96.5f);
    // 不循环的帧动画停在最后一帧。
    const FxSheet& slash = fxSheet(FxSheetId::Slash);
    EXPECT_FLOAT_EQ(fxFrameRect(slash, 0, 99.f, false).x, 144.f);
}

// ---------------------------------------------------------------------------
// 曲线
// ---------------------------------------------------------------------------

TEST(BattleCurves, DamageNumbersPopUpFallAndBounceOnce) {
    EXPECT_FLOAT_EQ(numberLift(0.f), 0.f);
    float peak = 0.f;
    float peakAt = 0.f;
    for (float t = 0.f; t <= 1.f; t += 0.005f) {
        const float lift = numberLift(t);
        EXPECT_GE(lift, 0.f) << "数字不会掉到基线以下：t=" << t;
        if (lift > peak) {
            peak = lift;
            peakAt = t;
        }
    }
    EXPECT_NEAR(peak, 38.f, 1.f);
    EXPECT_GT(peakAt, 0.1f);
    EXPECT_LT(peakAt, 0.16f);
    EXPECT_LT(numberLift(0.30f), 1.f) << "落回基线";
    EXPECT_GT(numberLift(0.35f), 5.f) << "再小弹一下";
    EXPECT_LT(numberLift(0.35f), 10.f);
    EXPECT_FLOAT_EQ(numberLift(0.5f), 0.f) << "然后停住";
    EXPECT_GT(numberPop(0.f), 1.3f);
    EXPECT_FLOAT_EQ(numberPop(0.2f), 1.f);
    EXPECT_FLOAT_EQ(floaterAlpha(0.2f, 1.f), 1.f);
    EXPECT_FLOAT_EQ(floaterAlpha(1.f, 1.f), 0.f);
}

TEST(BattleCurves, ScreenShakeDiesDownAndStaysDead) {
    EXPECT_FLOAT_EQ(shakeEnvelope(0.f, 0.5f), 1.f);
    float last = 1.f;
    for (float t = 0.f; t < 0.6f; t += 0.01f) {
        const float env = shakeEnvelope(t, 0.5f);
        EXPECT_LE(env, last + 1e-6f) << "只衰减不回弹：t=" << t;
        last = env;
        const StagePoint off = shakeOffset(t, 0.5f, 12.f);
        EXPECT_LE(std::abs(off.x), 12.f * env + 1e-4f);
        EXPECT_LE(std::abs(off.y), 12.f * env + 1e-4f);
    }
    EXPECT_FLOAT_EQ(shakeEnvelope(0.5f, 0.5f), 0.f);
    const StagePoint after = shakeOffset(0.8f, 0.5f, 12.f);
    EXPECT_FLOAT_EQ(after.x, 0.f);
    EXPECT_FLOAT_EQ(after.y, 0.f);
}

// ---------------------------------------------------------------------------
// 行动序条
// ---------------------------------------------------------------------------

TEST(BattleOrderStrip, CurrentFirstAndBiggerThenADividerThenNextRoundSmaller) {
    const OrderStrip strip = layoutOrderStrip({4, 0, 2}, {0, 4, 2, 1}, 16.f, 1000.f);
    ASSERT_EQ(strip.chips.size(), 7U);
    EXPECT_TRUE(strip.chips[0].current);
    EXPECT_EQ(strip.chips[0].unit, 4);
    EXPECT_FLOAT_EQ(strip.chips[0].size, kOrderChipCurrent);
    EXPECT_FLOAT_EQ(strip.chips[1].size, kOrderChip);
    EXPECT_FALSE(strip.chips[1].current);
    for (std::size_t i = 3; i < strip.chips.size(); ++i) {
        EXPECT_TRUE(strip.chips[i].nextRound);
        EXPECT_FLOAT_EQ(strip.chips[i].size, kOrderChipNext);
        EXPECT_GT(strip.chips[i].x, strip.dividerX) << "下回合的都在分隔右边";
    }
    EXPECT_GT(strip.dividerX, strip.chips[2].x + strip.chips[2].size);
    for (const OrderChip& chip : strip.chips) {
        EXPECT_FLOAT_EQ(chip.y + chip.size, kOrderBaseline) << "底边对齐";
    }
    for (std::size_t i = 1; i < strip.chips.size(); ++i) {
        EXPECT_GE(strip.chips[i].x, strip.chips[i - 1].x + strip.chips[i - 1].size) << "不叠";
    }
}

TEST(BattleOrderStrip, WhatDoesNotFitIsCutFromTheTailNotSqueezed) {
    std::vector<int> many(30);
    for (int i = 0; i < 30; ++i) many[static_cast<std::size_t>(i)] = i;
    const OrderStrip strip = layoutOrderStrip(many, many, 16.f, 400.f);
    ASSERT_FALSE(strip.chips.empty());
    for (const OrderChip& chip : strip.chips) EXPECT_LE(chip.x + chip.size, 400.f);
    EXPECT_FLOAT_EQ(strip.chips[1].size, kOrderChip) << "放不下不缩";
    EXPECT_LT(strip.dividerX, 0.f) << "本回合都放不下时，下回合一个也不画";
}

// ---------------------------------------------------------------------------
// BGM 与背景
// ---------------------------------------------------------------------------

TEST(BattleMusic, BossesSoulSeaAndEverythingElse) {
    std::vector<Unit> units{makeUnit(true), makeUnit(false), makeUnit(false)};
    EXPECT_EQ(battleBgm(units, false), "bgm_battle");
    units[2].actions = 2;
    EXPECT_EQ(battleBgm(units, false), "bgm_boss") << "一回合动两次的是首领";
    units[2].actions = 1;
    units[1].chargeEvery = 3;
    EXPECT_EQ(battleBgm(units, false), "bgm_boss") << "会蓄势的是首领";
    EXPECT_EQ(battleBgm(units, true), "bgm_soulsea") << "识海压过首领";
}

TEST(BattleMusic, AnAllyWhoActsTwiceIsNoBossAndALateBossStillCounts) {
    std::vector<Unit> units{makeUnit(true), makeUnit(false)};
    units[0].actions = 2;
    EXPECT_EQ(battleBgm(units, false), "bgm_battle") << "反例：我方动两次不算首领战";
    Unit late = makeUnit(false, 2);
    late.actions = 2;
    units.push_back(late);
    EXPECT_EQ(battleBgm(units, false), "bgm_boss") << "首领在第三波，开战就该是首领曲";
}

TEST(BattleBackdrop, PrecedenceAndTimeOfDay) {
    EXPECT_EQ(resolveBackdrop("manor_court", "mind", "secret_room", "inn_hall"), "manor_court");
    EXPECT_EQ(resolveBackdrop("", "indoor", "secret_room", "manor_night"), "secret_room");
    EXPECT_EQ(resolveBackdrop("", "mind", "", "inn_hall"), "soulsea") << "识海不看所在地图";
    EXPECT_EQ(resolveBackdrop("", "field", "", "mountain_forest_dusk"), "mountain_forest_dusk");
    EXPECT_EQ(resolveBackdrop("", "cave", "", ""), "cave_tunnel");
    EXPECT_TRUE(backdropLook("soulsea").soulsea);
    EXPECT_EQ(backdropLook("manor_night").time, fanren::core::TimeOfDay::Night);
    EXPECT_EQ(backdropLook("mountain_forest_dusk").time, fanren::core::TimeOfDay::Dusk);
    EXPECT_EQ(backdropLook("mountain_forest_dusk").particle, fanren::engine::ParticleKind::Firefly);
    EXPECT_EQ(backdropLook("inn_hall").time, fanren::core::TimeOfDay::Indoor);
    EXPECT_EQ(backdropLook("cave_tunnel").particle, fanren::engine::ParticleKind::Ember);
    EXPECT_EQ(backdropLook("no_such_place").time, fanren::core::TimeOfDay::Day);
}

// ---------------------------------------------------------------------------
// 开战碎屏
// ---------------------------------------------------------------------------

TEST(BattleShatter, PiecesTileTheWholeScreenAndAllFlyAway) {
    const std::vector<ShatterPiece> pieces = shatterPieces(10, 6, 12345u);
    ASSERT_EQ(pieces.size(), 120U);
    double area = 0.0;
    for (const ShatterPiece& p : pieces) {
        const auto& c = p.corner;
        area += std::abs((c[1].x - c[0].x) * (c[2].y - c[0].y) - (c[2].x - c[0].x) * (c[1].y - c[0].y)) / 2.0;
        for (std::size_t k = 0; k < 3; ++k) {
            EXPECT_NEAR(p.uv[k].x * 1280.f, c[k].x, 0.01f);
            EXPECT_NEAR(p.uv[k].y * 720.f, c[k].y, 0.01f);
        }
        EXPECT_FLOAT_EQ(shatterAlpha(p, 0.f), 1.f) << "开战那一帧是完整的世界画面";
        EXPECT_FLOAT_EQ(shatterAlpha(p, kShatterDuration), 0.f) << "碎屏收尾时一片不剩";
    }
    EXPECT_NEAR(area, 1280.0 * 720.0, 1.0) << "严丝合缝铺满整屏";
    // 同一个种子切出来的一模一样（截图口可复现）。
    const std::vector<ShatterPiece> again = shatterPieces(10, 6, 12345u);
    EXPECT_FLOAT_EQ(again[37].corner[1].x, pieces[37].corner[1].x);
    EXPECT_FLOAT_EQ(shatterPose(again[37], 0.5f)[2].y, shatterPose(pieces[37], 0.5f)[2].y);
}

// ---------------------------------------------------------------------------
// 截图口的时刻
// ---------------------------------------------------------------------------

TEST(BattleShotSpec, MomentsParseAndUnknownOnesAreRefused) {
    std::string id;
    BattleShot shot = BattleShot::None;
    ASSERT_TRUE(parseBattleShot("b03_gu_wai_elang", id, shot));
    EXPECT_EQ(id, "b03_gu_wai_elang");
    EXPECT_EQ(shot, BattleShot::Opening);
    ASSERT_TRUE(parseBattleShot("b05_ouyang_feitian:mid", id, shot));
    EXPECT_EQ(id, "b05_ouyang_feitian");
    EXPECT_EQ(shot, BattleShot::Mid);
    ASSERT_TRUE(parseBattleShot("b03_gu_wai_elang:victory", id, shot));
    EXPECT_EQ(shot, BattleShot::Victory);
    ASSERT_TRUE(parseBattleShot("b03_gu_wai_elang:intro", id, shot));
    EXPECT_EQ(shot, BattleShot::Intro);
    ASSERT_TRUE(parseBattleShot("b04_jia_tianlong:target", id, shot));
    EXPECT_EQ(shot, BattleShot::Target);
    // 反例：认不出的时刻如实拒绝，不悄悄拍一张开局。
    EXPECT_FALSE(parseBattleShot("b03_gu_wai_elang:middle", id, shot));
    EXPECT_FALSE(parseBattleShot(":mid", id, shot));
}

// ---------------------------------------------------------------------------
// 外观
// ---------------------------------------------------------------------------

TEST(BattleLooks, OverridesSheetsEnemiesAndTheFallback) {
    SpriteIndex index;
    fanren::core::CharacterSheet hero;
    hero.file = "chars/hanli.png";
    hero.battle = {{"idle", {12, 13}}, {"attack", {15}}};
    index.sheets["hanli"] = hero;
    index.roles["hanli"] = "hanli";
    fanren::core::EnemySheet orb;
    orb.file = "enemies/hanli_yuanshen.png";
    orb.battle = {{"idle", {0, 1, 2, 3}}};
    orb.battleFacing = fanren::core::BattleFacing::None;
    orb.floating = true;
    index.enemies["hanli_yuanshen"] = orb;
    fanren::core::EnemySheet wolf;
    wolf.file = "enemies/wild_wolf.png";
    wolf.battle = {{"idle", {0, 1}}, {"attack", {2}}};
    wolf.battleFacing = fanren::core::BattleFacing::Right;
    index.enemies["wild_wolf"] = wolf;
    index.battleOverrides["b03_shihai_duoshe"]["hanli"] = "hanli_yuanshen";
    const ChapterDone none = [](int) { return false; };

    const BattleLook body = battleLookFor(index, "b03_gu_wai_elang", "hanli", none);
    EXPECT_EQ(body.file, "chars/hanli.png");
    EXPECT_TRUE(body.humanoid);
    const BattleLook soul = battleLookFor(index, "b03_shihai_duoshe", "hanli", none);
    EXPECT_EQ(soul.file, "enemies/hanli_yuanshen.png") << "识海之战里韩立是光球";
    EXPECT_TRUE(soul.floating);
    EXPECT_EQ(battleLookFor(index, "x", "wild_wolf", none).file, "enemies/wild_wolf.png");
    EXPECT_TRUE(battleLookFor(index, "x", "no_such_role", none).file.empty()) << "缺图：退回旧画法";

    EXPECT_EQ(battleFrame(body, "attack", 0.f), 15);
    EXPECT_EQ(battleFrame(body, "cast", 0.f), 12) << "没有这个动作就退回待机";
    EXPECT_NE(battleFrame(body, "idle", 0.f), battleFrame(body, "idle", 0.45f)) << "待机在喘气";
    // 朝向：人物表面向左——站右边的我方不翻，站左边的人形敌人翻；兽面向右不翻；光球不分朝向。
    EXPECT_FALSE(battleFlip(body, true));
    EXPECT_TRUE(battleFlip(body, false));
    EXPECT_FALSE(battleFlip(battleLookFor(index, "x", "wild_wolf", none), false));
    EXPECT_FALSE(battleFlip(soul, true));
}

// ---------------------------------------------------------------------------
// HUD
// ---------------------------------------------------------------------------

TEST(BattleHudIcons, CategoriesMapToTheirCells) {
    using namespace fanren::core;
    const fanren::engine::RectF fire = iconRect(categoryIcon(kCategoryFire));
    EXPECT_FLOAT_EQ(fire.x, 0.f);
    EXPECT_FLOAT_EQ(fire.y, 16.f);
    const fanren::engine::RectF water = iconRect(categoryIcon(kCategoryWater));
    EXPECT_FLOAT_EQ(water.x, 112.f);
    EXPECT_EQ(categoryIcon(kCategorySword | kCategoryFire), BattleIcon::Unknown) << "多位不是一格";
    EXPECT_FLOAT_EQ(iconRect(BattleIcon::Silver).y, 32.f);
}

TEST(BattleResultCard, RewardLinesSaySuiyinNeverLingshi) {
    fanren::core::GameData data;
    fanren::core::Item salve;
    salve.id = "pill_jinchuang_yao";
    salve.name = "金疮药";
    salve.restoreHp = 20;
    data.items[salve.id] = salve;
    fanren::core::GameState state;   // 第 1–5 章：「修仙」这回事还没揭开
    GrantedReward granted;
    granted.cultivation = 30;
    granted.money = 24;
    granted.drops.push_back(fanren::core::BagEntry{"pill_jinchuang_yao", 2, 0});
    const std::vector<RewardLine> lines = rewardLines(granted, data, state, "修为");
    ASSERT_EQ(lines.size(), 3U);
    EXPECT_EQ(lines[0].text, "修为 +30");
    EXPECT_EQ(lines[0].icon, BattleIcon::Cultivation);
    EXPECT_EQ(lines[1].text, "碎银 24 块");
    EXPECT_EQ(lines[1].icon, BattleIcon::Silver);
    EXPECT_EQ(lines[2].text, "金疮药 ×2");
    EXPECT_EQ(lines[2].icon, BattleIcon::Medicine);
    for (const RewardLine& line : lines) {
        EXPECT_EQ(line.text.find("灵石"), std::string::npos) << "第 1–5 章的钱是碎银：" << line.text;
    }
    EXPECT_TRUE(rewardLines(GrantedReward{}, data, state, "修为").empty()) << "什么也没发就一行也不编";
}

// ---------------------------------------------------------------------------
// 无头：表现层一行都不跑
// ---------------------------------------------------------------------------

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) return candidate;
    }
    return ".";
}

TEST(BattleHeadless, NoStageIsBuiltAndTheFightStillFinishesInOneFrame) {
    Application app;
    const auto ready = app.init(assetRoot(), /*headless=*/true);
    ASSERT_TRUE(ready.ok) << ready.error;
    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app);
    // 有画面时开战那一帧不算不透明（要让世界层画出来拍快照）；无头时根本没有台，恒不透明。
    EXPECT_TRUE(scene.opaque());
    EXPECT_FALSE(scene.update(app, 1.0 / 60.0)) << "无头：一帧跑完、收场";
    EXPECT_NE(scene.battle().phase(), fanren::core::battle::BattlePhase::Ongoing);
    app.shutdown();
}

}  // namespace
