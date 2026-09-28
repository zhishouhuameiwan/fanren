// 游戏层的接线：规则层、加载器都在、都有测试，游戏里却够不着——终审（docs/octopath-review.md）
// 抓到的那几处，这里各钉一条，走的都是真实的 Application 与真实的 data/。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件
// ---------------------------------------------------------------------------
// HIGH-1 就是这么漏过去的：data/visual/battles.json 的加载器有三条测试（VisualLoaderTests），
// 可那三条拿 <仓库>/data/visual/battles.json 直接喂加载器；游戏里的战斗画面却拿引擎的资产根
//（<仓库>/assets）去找这张表，一次也没找到过，14 场背景是错的（夜袭画成白天）。
// 所以这里的判据一律从 Application 的入口问进去，不替它把路径拼好。
//
//   1. 战斗背景：开战画面问的那个函数（battleBackdropFor），在真地图上答出 art-maps.md 第 8 节的那张背景；
//   2. 叫法：战斗界面与主菜单同词（凡人阶段「法门 / 气力」，LOW-9）；
//   3. 前冲：冲到位时不压在同伴身上（LOW-5）；
//   4. 贴底的面板压着主角就上移让开（LOW-6）。
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleFx.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/MenuScene.h"
#include "game/Wording.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::Magic;
using fanren::core::battle::BattleState;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::MenuScene;
using fanren::game::PanelStage;
using fanren::game::StagePoint;
using fanren::rules::Realm;

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
            return candidate;
        }
    }
    return ".";
}

class GameWiring : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(repoRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    Application app_;
};

// ---------------------------------------------------------------------------
// 1. 战斗背景（HIGH-1）
// ---------------------------------------------------------------------------
// 判据照抄 docs/art-maps.md 第 8 节那张表：门派攻防战是夜袭，在演武场上开打也得是夜里；
// 墨府尸傀在马厩角上那扇地窖石门底下，不是月下的院子。两个字面量都不从 battles.json 读——
// 从被测的表里读判据，表写错了测试照样绿。
TEST_F(GameWiring, TheBattleScreenPaintsTheBackdropThatBattlesJsonAssigns) {
    auto loaded = app_.loadMap("ch04_yanwuchang", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "b04_gongfang_weijian"), "drill_ground_night")
        << "攻防战画成了演武场的白天：battles.json 没接到开战画面上";

    loaded = app_.loadMap("ch05_mofu", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "b05_mofu_shigui"), "secret_room")
        << "尸傀那一仗画成了墨府的院子：battles.json 没接到开战画面上";
}

// [配对的负向] 同一张图上，表里没有的那一场取的是地图自己的背景。少了这一条，上面那两个字面量
// 也可能是地图 meta 碰巧就写着它——那就证明不了是表在起作用。
TEST_F(GameWiring, ABattleTheTableDoesNotNameFallsBackToTheMapsOwnBackdrop) {
    auto loaded = app_.loadMap("ch04_yanwuchang", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(fanren::game::battleBackdropFor(app_, "no_such_battle"), "drill_ground");

    loaded = app_.loadMap("ch05_mofu", std::string{});
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_NE(fanren::game::battleBackdropFor(app_, "no_such_battle"), "secret_room");
}

// ---------------------------------------------------------------------------
// 2. 叫法（LOW-9）
// ---------------------------------------------------------------------------
// 战斗界面的词从 Wording.h 取，主菜单的词从 ui.json 与修炼面板的用词表取。两边各有出处，
// 这一条钉住它们说的是同一个词——哪一边改了另一边没跟上，这里先红。
TEST_F(GameWiring, TheBattleCallsMagicAndItsPoolWhatTheMainMenuCallsThem) {
    for (const PanelStage stage : {PanelStage::Mortal, PanelStage::Immortal}) {
        EXPECT_EQ(fanren::game::magicWord(stage), MenuScene::pageLabel(app_.data(), stage, MenuScene::Page::Magic));
        const std::string pool = fanren::game::cultivationLexicon(stage).mpLabel;
        EXPECT_EQ(pool.rfind(fanren::game::mpWord(stage), 0), 0u) << pool;
    }
    EXPECT_STREQ(fanren::game::magicWord(PanelStage::Mortal), "法门");
    EXPECT_STREQ(fanren::game::mpWord(PanelStage::Mortal), "气力");
}

TEST(StageWords, RetellsTheRulesLayersSentencesInTheMortalsWords) {
    using fanren::game::stageWords;
    EXPECT_EQ(stageWords(PanelStage::Mortal, "法力不足，施展不了【火弹术】"), "气力不足，施展不了【火弹术】");
    EXPECT_EQ(stageWords(PanelStage::Mortal, "【御风诀】不是伤人的法术"), "【御风诀】不是伤人的法门");
    EXPECT_EQ(stageWords(PanelStage::Mortal, "回复 5 点法力、法力回满，法术照放"), "回复 5 点气力、气力回满，法门照放");
    EXPECT_EQ(stageWords(PanelStage::Immortal, "法力不足，施展不了【火弹术】"), "法力不足，施展不了【火弹术】");
    EXPECT_EQ(stageWords(PanelStage::Mortal, "韩立 攻击 恶狼"), "韩立 攻击 恶狼");
}

// 真搭一场：差一点法力的术者。凡人阶段的菜单上一个「法术」「法力」都不许有；
// 置上旗标（修仙阶段）同一张菜单换回修仙说法——证明换词跟着开关走，而不是把词删光了事。
TEST_F(GameWiring, AMortalsBattleMenuNeverSaysMagicOrMana) {
    Magic spell;
    spell.id = "magic_probe";
    spell.name = "试探术";
    spell.needMp = 20;
    spell.power = 5;
    spell.needRealm = Realm::QiRefining1;

    Unit caster;
    caster.id = caster.name = "术者";
    caster.hp = caster.maxHp = 100;
    caster.mp = spell.needMp - 1;
    caster.maxMp = 40;
    caster.attack = 10;
    caster.speed = 12;
    caster.realm = Realm::QiRefining3;
    caster.ally = true;
    caster.magicsExhaustive = true;
    caster.magics = {spell.id};
    Unit target = caster;
    target.id = target.name = "靶子";
    target.ally = false;
    target.speed = 8;
    target.magics.clear();

    BattleState battle;
    battle.setup({caster, target}, 9u);
    battle.addMagic(spell);
    ASSERT_EQ(battle.currentActor(), 0);

    fanren::core::GameData data;
    data.magics.emplace(spell.id, spell);
    fanren::core::GameState mortal;
    fanren::core::GameState immortal;
    immortal.setFlag(fanren::game::kXiuxianKnownFlag);

    const auto words = [&](const fanren::core::GameState& state) {
        std::string all;
        for (const fanren::ui::ListItem& row : BattleScene::buildRootItems(data, state, battle, 0)) {
            all += row.label + "|" + row.detail + "|" + row.disabledReason + "\n";
        }
        std::vector<std::string> ids;
        for (const fanren::ui::ListItem& row : BattleScene::buildMagicItems(data, state, battle, 0, ids)) {
            all += row.label + "|" + row.detail + "|" + row.disabledReason + "\n";
        }
        return all;
    };
    const std::string plain = words(mortal);
    EXPECT_EQ(plain.find("法术"), std::string::npos) << plain;
    EXPECT_EQ(plain.find("法力"), std::string::npos) << plain;
    EXPECT_NE(plain.find("法门"), std::string::npos) << plain;
    EXPECT_NE(plain.find("耗气力 20"), std::string::npos) << plain;
    EXPECT_NE(plain.find("气力不足"), std::string::npos) << plain;

    const std::string learned = words(immortal);
    EXPECT_NE(learned.find("法术"), std::string::npos) << learned;
    EXPECT_NE(learned.find("耗法力 20"), std::string::npos) << learned;
    EXPECT_NE(learned.find("法力不足"), std::string::npos) << learned;
}

// F5 存盘那句提示（LOW-2）：从前写死在 WorldScene 里、说的是「重开时加 --load 读回」，
// 在禁词闸之外，也不是说给玩家听的。现在它在 ui.json 里，这一条替它过一遍禁词与「开发者话」。
// 禁词表与 tests/UiStyleTests.cpp 主菜单那张同源。
TEST_F(GameWiring, TheSaveNoticeSpeaksToThePlayer) {
    const std::string notice = app_.text("ui.world.save.ok");
    ASSERT_NE(notice, "ui.world.save.ok") << "ui.json 里没有这条";
    for (const char* dev : {"--load", "quick.sav", "saves/"}) {
        EXPECT_EQ(notice.find(dev), std::string::npos) << "「" << dev << "」是说给开发者听的：" << notice;
    }
    for (const char* word : {"修仙", "灵根", "法术", "灵气", "气感", "炼气", "筑基", "真元", "走火入魔", "法力",
                             "境界", "灵石", "元神", "金丹", "神通", "法宝", "丹田", "真气", "储物袋", "灵符", "坊市"}) {
        EXPECT_EQ(notice.find(word), std::string::npos) << word << "：" << notice;
        EXPECT_EQ(app_.text("ui.battle.hint.menu").find(word), std::string::npos) << word;
    }
    EXPECT_NE(notice.find("继续旅程"), std::string::npos) << "读回的路是标题画面的「继续旅程」，得说出来：" << notice;
}

// ---------------------------------------------------------------------------
// 3. 前冲不压同伴（LOW-5）
// ---------------------------------------------------------------------------
// 身子的大小照精灵索引（assets/art/sprites/index.json）：人 16×24 帧、曲魂 24×32 帧，都 ×4。
// 两个框横向、纵向都叠上，就是截图 42 那样「曲魂冲到韩立身上」。
using fanren::game::StageBody;
constexpr StageBody kMan{64.f, 96.f};
constexpr StageBody kQuHun{96.f, 128.f};

bool bodiesOverlap(StagePoint a, StageBody ba, StagePoint b, StageBody bb) {
    return std::abs(a.x - b.x) < (ba.w + bb.w) / 2.f && a.y - ba.h < b.y && b.y - bb.h < a.y;
}

std::vector<Unit> lineUp(int allies, int enemies) {
    std::vector<Unit> units;
    for (int i = 0; i < allies + enemies; ++i) {
        Unit u;
        u.id = "u" + std::to_string(i);
        u.ally = i < allies;
        units.push_back(u);
    }
    return units;
}

// 编成：allies 个我方（第 tall 位是曲魂，-1 = 全是人）、3 个敌人。
struct Formation {
    std::vector<Unit> units;
    std::vector<StagePoint> stand;
    std::vector<StageBody> bodies;
};

Formation formation(int allies, int tall) {
    Formation f;
    f.units = lineUp(allies, 3);
    f.stand = fanren::game::standPoints(f.units);
    for (int i = 0; i < static_cast<int>(f.units.size()); ++i) f.bodies.push_back(i == tall ? kQuHun : kMan);
    return f;
}

TEST(Lunge, NoAllyChargesIntoAnotherAlly) {
    for (int allies = 1; allies <= 5; ++allies) {
        for (int tall = -1; tall < allies; ++tall) {
            const Formation f = formation(allies, tall);
            for (std::size_t i = 0; i < f.units.size(); ++i) {
                if (!f.units[i].ally) continue;
                const StagePoint reach = fanren::game::lungeReach(f.units, f.stand, f.bodies, i);
                const StagePoint end{f.stand[i].x + reach.x, f.stand[i].y + reach.y};
                EXPECT_LE(reach.x, 0.f) << "我方往左冲";
                for (std::size_t j = 0; j < f.units.size(); ++j) {
                    if (j == i || !f.units[j].ally) continue;
                    // 站着就已经叠着的（曲魂比人宽，挤在他身后那位本来就挨着）不归前冲管；
                    // 要钉的是「站着分得开，冲过去就压上了」。
                    if (bodiesOverlap(f.stand[i], f.bodies[i], f.stand[j], f.bodies[j])) continue;
                    EXPECT_FALSE(bodiesOverlap(end, f.bodies[i], f.stand[j], f.bodies[j]))
                        << allies << " 人（曲魂在第 " << tall << " 位）时第 " << i << " 位冲到位压在第 " << j << " 位身上";
                }
            }
        }
    }
}

// [配对的负向] 不能靠「不冲了」来躲：截图 42 那一对（韩立在前、曲魂在后）与三个人以内的编成，
// 每个人都冲满 70；站最前的那位一步不让；敌人照旧直冲、不拐弯。
TEST(Lunge, TheChargeIsStillACharge) {
    for (int allies = 1; allies <= 3; ++allies) {
        for (const int tall : {-1, allies == 2 ? 1 : -1}) {
            const Formation f = formation(allies, tall);
            for (std::size_t i = 0; i < f.units.size(); ++i) {
                const StagePoint reach = fanren::game::lungeReach(f.units, f.stand, f.bodies, i);
                EXPECT_FLOAT_EQ(std::abs(reach.x), 70.f) << allies << " 人时第 " << i << " 位";
                if (!f.units[i].ally || i == 0) EXPECT_FLOAT_EQ(reach.y, 0.f) << "没人挡着就不该拐弯：第 " << i << " 位";
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 4. 贴底的面板压着主角就上移让开（LOW-6）
// ---------------------------------------------------------------------------
TEST(KeepHeroInSight, APanelOverTheHeroMovesUpJustClearOfHimAndOnlyThen) {
    using fanren::engine::Rect;
    const Rect panel{380, 500, 520, 172};
    // 镜头贴着地图下沿，韩立（连同身边一圈）站在屏幕下半，正好在面板底下：面板底边抬到那一圈的上沿。
    const Rect hero{560, 530, 144, 144};
    const Rect moved = fanren::game::keepHeroInSight(panel, hero, 48);
    EXPECT_EQ(moved.y + moved.h, hero.y) << "只抬必要的那一截";
    EXPECT_EQ(moved.x, panel.x);
    EXPECT_EQ(moved.w, panel.w);
    EXPECT_EQ(moved.h, panel.h) << "只挪不缩";
    // 框比他头顶以上的地方还高：贴到顶边为止，不出屏。
    EXPECT_EQ(fanren::game::keepHeroInSight(Rect{48, 272, 1184, 400}, Rect{560, 300, 144, 144}, 48).y, 48);
    // 人在屏幕正中（镜头没贴边的常态）：面板留在底边。
    EXPECT_EQ(fanren::game::keepHeroInSight(panel, Rect{616, 312, 48, 96}, 48).y, panel.y);
    // 人在下半但偏到面板左边外头：也不翻。
    EXPECT_EQ(fanren::game::keepHeroInSight(panel, Rect{200, 560, 48, 96}, 48).y, panel.y);
    // 底下不是世界层（战斗、标题画面）：没有要躲的人。
    EXPECT_EQ(fanren::game::keepHeroInSight(panel, Rect{}, 48).y, panel.y);
}

}  // namespace
