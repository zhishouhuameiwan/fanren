// Chapter 10 E1/E2: docs/interfaces-p3-ch10.md sections 1.4 and 2.4.
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <limits>

#include "core/rules/Cultivation.h"

// Compile the pure rules before the Chapter 9 GameState dependency is synchronized.
// The normal CMake target does not define this switch and always includes the panel tests.
#ifndef FANREN_CH10_RULES_ONLY
#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/Wording.h"
#endif

namespace {

namespace rules = fanren::rules;
using rules::Realm;

constexpr std::array<int, 19> kImmortalRealms{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 21, 22, 23, 31, 32, 33,
};

TEST(Ch10ReclimbRule, TheContractExamplesRejectMortalsGapsAndTheReachedOldRealm) {
    struct Example {
        int current;
        int former;
        bool expected;
    };
    constexpr std::array<Example, 9> examples{{
        {3, 22, true}, {21, 22, true}, {13, 21, true},
        {22, 22, false}, {23, 22, false}, {3, 0, false},
        {0, 22, false}, {15, 22, false}, {3, 15, false},
    }};
    for (const auto& example : examples) {
        EXPECT_EQ(rules::reclimbing(rules::fromValue(example.current),
                                   rules::fromValue(example.former)), example.expected)
            << example.current << " / " << example.former;
    }
}

TEST(Ch10ReclimbRule, EveryImmortalPairUsesStrictOrderAndInvalidEnumsNeverQualify) {
    for (const int current : kImmortalRealms) {
        for (const int former : kImmortalRealms) {
            EXPECT_EQ(rules::reclimbing(rules::fromValue(current), rules::fromValue(former)),
                      current < former) << current << " / " << former;
        }
    }
    for (const int invalid : {std::numeric_limits<int>::min(), -1, 0, 14, 15, 20,
                             24, 30, 34, std::numeric_limits<int>::max()}) {
        for (const int valid : kImmortalRealms) {
            EXPECT_FALSE(rules::reclimbing(rules::fromValue(invalid), rules::fromValue(valid)))
                << invalid << " / " << valid;
            EXPECT_FALSE(rules::reclimbing(rules::fromValue(valid), rules::fromValue(invalid)))
                << valid << " / " << invalid;
        }
        EXPECT_FALSE(rules::reclimbing(rules::fromValue(invalid), rules::fromValue(invalid)));
    }
}

TEST(Ch10ReclimbRule, TheApprovedConstantsAreEightfoldAndOneHundredPercentagePoints) {
    EXPECT_EQ(rules::kReclimbEffectivenessPercent, 800);
    EXPECT_EQ(rules::kReclimbBreakthroughBonus, 100);
    EXPECT_LE(rules::kReclimbEffectivenessPercent, rules::kMaxEffectiveness);
    EXPECT_EQ(rules::kReclimbEffectivenessPercent * 125 / 100, rules::kMaxEffectiveness);
}

TEST(Ch10ReclimbRule, MeditationIsEightfoldAndOnlyTheRulesClampTheSiteProduct) {
    for (std::uint32_t seed = 1; seed <= 32; ++seed) {
        const auto normal = rules::meditate(Realm::QiRefining5, 50, 100, 60, seed);
        const auto oldRoad = rules::meditate(Realm::QiRefining5, 50,
                                           rules::kReclimbEffectivenessPercent, 60, seed);
        ASSERT_GT(normal.cultivation, 0);
        EXPECT_GE(oldRoad.cultivation, 8 * normal.cultivation);
        EXPECT_LT(oldRoad.cultivation, 8 * normal.cultivation + 8);
        EXPECT_EQ(oldRoad.insight, normal.insight);

        const auto maximum = rules::meditate(Realm::QiRefining5, 50, 1000, 60, seed);
        for (const int sitePercent : {125, 300}) {
            const auto composed = rules::meditate(Realm::QiRefining5, 50,
                rules::kReclimbEffectivenessPercent * sitePercent / 100, 60, seed);
            EXPECT_EQ(composed.cultivation, maximum.cultivation) << seed << " / " << sitePercent;
            EXPECT_EQ(composed.days, maximum.days);
            EXPECT_EQ(composed.insight, maximum.insight);
        }
    }
}

TEST(Ch10ReclimbRule, TheBonusGivesNinetyFiveButCannotBypassShortfallOrStoryCap) {
    for (const int value : kImmortalRealms) {
        const Realm realm = rules::fromValue(value);
        const int need = rules::cultivationNeeded(realm);
        if (need < 0) continue;
        for (const int aptitude : {0, 50, 100}) {
            EXPECT_EQ(rules::breakthroughChance(realm, need, aptitude,
                                               rules::kReclimbBreakthroughBonus), 95);
            EXPECT_EQ(rules::breakthroughChance(realm, need - 1, aptitude,
                                               rules::kReclimbBreakthroughBonus), 0);
        }
        const auto capped = rules::tryBreakthrough(realm, need, realm, 0,
                                                   rules::kReclimbBreakthroughBonus, 7);
        EXPECT_EQ(capped.blocked, rules::BreakthroughBlock::StoryCap);
        EXPECT_FALSE(capped.success);
        EXPECT_EQ(capped.cultivationSpent, 0);
        EXPECT_EQ(capped.cultivationLost, 0);
    }
}

#ifndef FANREN_CH10_RULES_ONLY

namespace game = fanren::game;
using fanren::core::GameState;
using game::CultivationScene;
using game::PanelStage;

GameState oldRoadSave() {
    GameState state;
    state.realm = Realm::QiRefining5;
    state.formerRealm = Realm::FoundationMid;
    state.realmCap = Realm::QiRefining9;
    state.aptitude = 50;
    state.cultivation = 0;
    state.day = 1;
    state.setFlag(game::kXiuxianKnownFlag);
    return state;
}

TEST(Ch10ReclimbMeditation, SixtyDaysPayEightfoldAtTheThreeArgumentEntry) {
    GameState oldRoad = oldRoadSave();
    GameState ordinary = oldRoad;
    ordinary.formerRealm = Realm::Mortal;
    constexpr std::uint32_t seed = 1;
    const auto normal = CultivationScene::bankMeditation(ordinary, 60, seed);
    const auto boosted = CultivationScene::bankMeditation(oldRoad, 60, seed);
    ASSERT_GT(normal.cultivation, 0);
    ASSERT_LT(normal.cultivation, rules::cultivationNeeded(Realm::QiRefining5));
    ASSERT_LT(boosted.cultivation, rules::cultivationNeeded(Realm::QiRefining5));
    EXPECT_GE(boosted.cultivation, 8 * normal.cultivation);
    EXPECT_LT(boosted.cultivation, 8 * normal.cultivation + 8);
    EXPECT_EQ(boosted.days, 60);
    EXPECT_EQ(boosted.insight, normal.insight);
    EXPECT_EQ(oldRoad.cultivation, boosted.cultivation);
    EXPECT_EQ(ordinary.cultivation, normal.cultivation);
    EXPECT_EQ(oldRoad.cultivationRemainder, ordinary.cultivationRemainder);
}

TEST(Ch10ReclimbMeditation, CatchingOrPassingTheOldRealmImmediatelyRestoresOrdinaryGain) {
    for (const Realm reached : {Realm::FoundationMid, Realm::FoundationLate}) {
        GameState caughtUp = oldRoadSave();
        caughtUp.realm = reached;
        caughtUp.realmCap = reached;
        GameState ordinary = caughtUp;
        ordinary.formerRealm = Realm::Mortal;
        EXPECT_EQ(game::meditationEffectiveness(caughtUp), 100);
        EXPECT_EQ(game::reclimbBreakthroughBonus(caughtUp), 0);
        for (const int days : {1, 10, 60}) {
            const auto a = CultivationScene::bankMeditation(caughtUp, days, 7);
            const auto b = CultivationScene::bankMeditation(ordinary, days, 7);
            EXPECT_EQ(a.cultivation, b.cultivation);
            EXPECT_EQ(a.days, b.days);
            EXPECT_EQ(a.insight, b.insight);
            EXPECT_EQ(caughtUp.cultivation, ordinary.cultivation);
            EXPECT_EQ(caughtUp.cultivationRemainder, ordinary.cultivationRemainder);
        }
    }
}

TEST(Ch10ReclimbMeditation, SiteOneHundredTwentyFiveAndThreeHundredBothReachTheRulesCeiling) {
    for (std::uint32_t seed = 1; seed <= 32; ++seed) {
        GameState ordinary = oldRoadSave();
        ordinary.formerRealm = Realm::Mortal;
        const auto maximum = CultivationScene::bankMeditation(ordinary, 60, seed, 1000);
        ASSERT_GT(maximum.cultivation, 0);
        for (const int sitePercent : {125, 300}) {
            GameState oldRoad = oldRoadSave();
            const auto composed = CultivationScene::bankMeditation(oldRoad, 60, seed, sitePercent);
            EXPECT_EQ(composed.cultivation, maximum.cultivation) << seed << " / " << sitePercent;
            EXPECT_EQ(composed.days, maximum.days);
            EXPECT_EQ(composed.insight, maximum.insight);
            EXPECT_EQ(oldRoad.cultivation, ordinary.cultivation);
            EXPECT_EQ(oldRoad.cultivationRemainder, ordinary.cultivationRemainder);
        }
    }
}

TEST(Ch10ReclimbMeditation, NamedBonusesReadOnlyTheTwoRealmFields) {
    struct Example {
        int current;
        int former;
        bool boosted;
    };
    for (const Example example : {Example{5, 22, true}, Example{21, 22, true},
                                 Example{22, 22, false}, Example{23, 22, false},
                                 Example{5, 0, false}, Example{0, 22, false},
                                 Example{15, 22, false}, Example{5, 15, false}}) {
        GameState state = oldRoadSave();
        state.realm = rules::fromValue(example.current);
        state.formerRealm = rules::fromValue(example.former);
        for (const int known : {0, 1}) {
            state.setFlag(game::kXiuxianKnownFlag, known);
            EXPECT_EQ(game::meditationEffectiveness(state),
                      example.boosted ? rules::kReclimbEffectivenessPercent : 100);
            EXPECT_EQ(game::reclimbBreakthroughBonus(state),
                      example.boosted ? rules::kReclimbBreakthroughBonus : 0);
            EXPECT_EQ(game::breakthroughPillBonus(state), 0);
        }
    }
}

std::filesystem::path projectRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const auto root = std::filesystem::path(candidate);
        if (std::filesystem::exists(root / "maps/ch01_hanjiacun.tmj")) return root;
    }
    return {};
}

class Ch10ReclimbPanel : public ::testing::Test {
protected:
    void SetUp() override {
        const auto root = projectRoot();
        ASSERT_FALSE(root.empty());
        const auto ready = app_.init(root.string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }
    game::Application app_;
};

TEST_F(Ch10ReclimbPanel, TheDisplayedChanceIsNinetyFiveAndOrdinaryOddsStayUnchanged) {
    app_.state() = oldRoadSave();
    app_.state().aptitude = 0;
    app_.state().cultivation = rules::cultivationNeeded(app_.state().realm);
    CultivationScene scene;
    scene.onEnter(app_);
    const auto boosted = CultivationScene::buildMainItems(app_.state());
    ASSERT_EQ(boosted.size(), 3u);
    EXPECT_TRUE(boosted[1].enabled);
    EXPECT_EQ(boosted[1].detail.find("95%"), 0u) << boosted[1].detail;

    app_.state().formerRealm = Realm::Mortal;
    const auto ordinary = CultivationScene::buildMainItems(app_.state());
    const int chance = rules::breakthroughChance(app_.state().realm,
                                                app_.state().cultivation, 0, 0);
    ASSERT_LT(chance, 95);
    EXPECT_TRUE(ordinary[1].enabled);
    EXPECT_EQ(ordinary[1].detail.find(std::to_string(chance) + "%"), 0u);
    EXPECT_NE(boosted[1].detail, ordinary[1].detail);
}

TEST_F(Ch10ReclimbPanel, PressingUsesTheSameBonusAndNeverTurnsAnOrdinarySuccessIntoFailure) {
    GameState oldRoad = oldRoadSave();
    oldRoad.aptitude = 0;
    oldRoad.cultivation = rules::cultivationNeeded(oldRoad.realm);
    GameState ordinary = oldRoad;
    ordinary.formerRealm = Realm::Mortal;
    int improved = 0;
    int residualFailures = 0;
    for (int day = 1; day <= 200; ++day) {
        oldRoad.day = ordinary.day = day;
        app_.state() = ordinary;
        CultivationScene ordinaryPanel;
        const auto normal = ordinaryPanel.breakthrough(app_);
        app_.state() = oldRoad;
        CultivationScene boostedPanel;
        const auto boosted = boostedPanel.breakthrough(app_);
        EXPECT_EQ(normal.blocked, rules::BreakthroughBlock::None);
        EXPECT_EQ(boosted.blocked, rules::BreakthroughBlock::None);
        EXPECT_FALSE(normal.success && !boosted.success) << "day " << day;
        if (boosted.success && !normal.success) ++improved;
        if (!boosted.success) ++residualFailures;
    }
    EXPECT_GT(improved, 0) << "The actual button never received the displayed bonus";
    EXPECT_GT(residualFailures, 0) << "The rules must still leave five percent failure";
}

TEST_F(Ch10ReclimbPanel, TheOldRoadLineCoexistsWithTheSiteLineAndVanishesAfterRecovery) {
    app_.state() = oldRoadSave();
    CultivationScene spring(125);
    spring.onEnter(app_);
    const auto& words = game::cultivationLexicon(PanelStage::Immortal);
    const std::string line = spring.reclimbLine(app_.state());
    EXPECT_EQ(line, std::string(words.reclimbPrefix) + "8" + words.reclimbSuffix);
    EXPECT_NE(line.find("8"), std::string::npos);
    EXPECT_FALSE(spring.siteBonusLine(app_.state()).empty());
    spring.render(app_);

    for (const Realm recovered : {Realm::FoundationMid, Realm::FoundationLate}) {
        app_.state().realm = recovered;
        EXPECT_TRUE(spring.reclimbLine(app_.state()).empty());
    }
    app_.state() = oldRoadSave();
    app_.state().formerRealm = Realm::Mortal;
    EXPECT_TRUE(spring.reclimbLine(app_.state()).empty());
    spring.render(app_);
}

TEST_F(Ch10ReclimbPanel, StoryCapUsesTheOldRoadWordsWithoutSpendingAndPreservesOrdinaryWords) {
    const auto& words = game::cultivationLexicon(PanelStage::Immortal);
    ASSERT_NE(std::string(words.pushAtStoryCap).find("瓶颈"), std::string::npos);
    ASSERT_NE(std::string(words.atStoryCap).find("瓶颈"), std::string::npos);
    for (const Realm cap : {Realm::QiRefining5, Realm::QiRefining9, Realm::FoundationEarly}) {
        for (const bool boosted : {true, false}) {
            GameState state = oldRoadSave();
            state.realm = state.realmCap = cap;
            state.cultivation = 100000;
            if (!boosted) state.formerRealm = Realm::Mortal;
            app_.state() = state;
            CultivationScene scene;
            scene.onEnter(app_);
            const auto items = CultivationScene::buildMainItems(app_.state());
            ASSERT_EQ(items.size(), 3u);
            EXPECT_FALSE(items[1].enabled);
            EXPECT_EQ(items[1].disabledReason,
                      boosted ? words.pushAtStoryCapReclimb : words.pushAtStoryCap);
            const auto attempt = scene.breakthrough(app_);
            EXPECT_EQ(attempt.blocked, rules::BreakthroughBlock::StoryCap);
            EXPECT_FALSE(attempt.success);
            EXPECT_FALSE(attempt.backlash);
            EXPECT_EQ(attempt.cultivationSpent, 0);
            EXPECT_EQ(attempt.cultivationLost, 0);
            EXPECT_EQ(app_.state().cultivation, state.cultivation);
            EXPECT_EQ(app_.state().realm, state.realm);
            EXPECT_EQ(app_.state().formerRealm, state.formerRealm);
            EXPECT_EQ(scene.feedback(), boosted ? words.atStoryCapReclimb : words.atStoryCap);
            if (boosted) {
                EXPECT_EQ(items[1].disabledReason.find("瓶颈"), std::string::npos);
                EXPECT_EQ(scene.feedback().find("瓶颈"), std::string::npos);
            }
        }
    }
}

TEST_F(Ch10ReclimbPanel, ARealBreakthroughToTheOldRealmStopsEveryBonusOnThatSameState) {
    GameState state = oldRoadSave();
    state.realm = Realm::FoundationEarly;
    state.realmCap = Realm::FoundationMid;
    state.cultivation = rules::cultivationNeeded(state.realm);
    bool caughtUp = false;
    for (int day = 1; day <= 200 && !caughtUp; ++day) {
        state.day = day;
        app_.state() = state;
        CultivationScene scene;
        if (!scene.breakthrough(app_).success) continue;
        caughtUp = true;
        EXPECT_EQ(app_.state().realm, Realm::FoundationMid);
        EXPECT_EQ(app_.state().formerRealm, Realm::FoundationMid);
        EXPECT_EQ(game::meditationEffectiveness(app_.state()), 100);
        EXPECT_EQ(game::reclimbBreakthroughBonus(app_.state()), 0);
        EXPECT_TRUE(scene.reclimbLine(app_.state()).empty());
        GameState ordinary = app_.state();
        ordinary.formerRealm = Realm::Mortal;
        const auto a = CultivationScene::bankMeditation(app_.state(), 60, 1);
        const auto b = CultivationScene::bankMeditation(ordinary, 60, 1);
        EXPECT_EQ(a.cultivation, b.cultivation);
        EXPECT_EQ(a.insight, b.insight);
        EXPECT_EQ(app_.state().cultivation, ordinary.cultivation);
    }
    EXPECT_TRUE(caughtUp);
}

TEST(Ch10ReclimbWording, BothStagesExposeAllFourNewStringsToTheExistingScanners) {
    for (const PanelStage stage : {PanelStage::Mortal, PanelStage::Immortal}) {
        const auto& words = game::cultivationLexicon(stage);
        const std::vector<std::string> strings = game::cultivationPanelStrings(stage);
        for (const char* text : {words.reclimbPrefix, words.reclimbSuffix,
                                words.pushAtStoryCapReclimb, words.atStoryCapReclimb}) {
            ASSERT_NE(text, nullptr);
            EXPECT_STRNE(text, "");
            EXPECT_NE(std::find(strings.begin(), strings.end(), text), strings.end()) << text;
        }
    }
}

#endif  // FANREN_CH10_RULES_ONLY

}  // namespace
