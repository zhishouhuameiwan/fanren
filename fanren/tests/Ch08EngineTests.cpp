// 第 8 章引擎增补（契约 docs/interfaces-p3-ch08.md）：E1 打坐设施按地点给效率（洞府的灵眼之泉）。
//
// 判据照契约 1.5 原文写死（筑基初期、资质 50、120 日与 90 日、125 : 100 相差不超过一点、凡人日课的
// 改动前数字），不从被测物推。真实入口：125 那一侧加载一张测试图——ch06_baiyaoyuan 的副本，蒲团上
// 多一条 Tiled int 属性 effectiveness=125（与内容路给 ch08_dongfu 的 facility_lingquan 同一种写法），
// 经 io::loadTileMap 读进来的那个对象交给 Application::openFacility；100 那一侧是真 ch06_baiyaoyuan
// 上的同一张蒲团，面朝它按确认（WorldScene::interact → openFacility）。内容路的洞府落地之后，测试路
// 可以照同一个比法拿 facility_lingquan 再比一遍。
// E3 的门禁形状检查在 tools/validate.py（check_facility_effectiveness），负例在 tools/validate_selftest.py 的 U 节。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/WorldScene.h"
#include "game/Wording.h"
#include "io/DataLoader.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::Application;
using fanren::game::CultivationScene;
using fanren::game::MeditateOutcome;
using fanren::game::PanelStage;
using fanren::game::WorldScene;
using fanren::rules::Realm;
using fanren::test::TempDir;

constexpr int kSpring = 125;    // 灵眼之泉（契约 1.2）
constexpr int kCushion = 100;   // 普通蒲团
constexpr double kFrame = 1.0 / 60.0;

// 100 那一侧的蒲团：第 6 章百药园那一张（契约 1.5 第 3 条点名的就是它）。
constexpr const char* kCushionMap = "ch06_baiyaoyuan";
constexpr const char* kCushionName = "facility_dazuo";

// 凡人日课的改动前数字（契约 1.5 第 5 条）：炼气二层、资质 40、水位第 1 日、今天第 101 日。
// 是改动之前的代码跑出来的（隔离副本里原样的 CultivationScene 加一段探针），不是照着现在的实现推的：
// 一百日 = 二十块 → 折六十日静坐，其中碰上一次顿悟；余数账记六十日，水位推到第 101 日。
constexpr int kDailyBeforeGain = 9;
constexpr int kDailyBeforeRemainder = 60;
constexpr int kDailyBeforeWatermark = 101;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / (std::string(kCushionMap) + ".tmj"))) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void writeFile(const fs::path& path, const std::string& content) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

// 契约 1.5 第 1 条的比法：125 那一侧 ×100 与 100 那一侧 ×125 相差不超过 125（取整误差不超过 1 点）。
bool fiveQuarters(int spring, int cushion) {
    return std::abs(spring * 100 - cushion * 125) <= 125;
}

// 筑基初期、资质 50、修为离门槛尚远；修仙阶段（没有日课来搅账）。
GameState foundationSave() {
    GameState s;
    s.realm = Realm::FoundationEarly;
    s.aptitude = 50;
    s.day = 1;
    s.cultivation = 0;
    s.setFlag(fanren::game::kXiuxianKnownFlag);
    return s;
}

// 凡人阶段、欠着一百日日课的存档（契约 1.5 第 5 条）。
GameState mortalSaveOwingDailyPractice() {
    GameState s;
    s.realm = Realm::QiRefining2;
    s.aptitude = 40;
    s.day = 101;
    s.lastPracticeDay = 1;
    return s;
}

const MapObject* objectNamed(const TileMap& map, const std::string& name) {
    const auto it = std::find_if(map.objects.begin(), map.objects.end(),
                                 [&](const MapObject& o) { return o.name == name; });
    return it == map.objects.end() ? nullptr : &*it;
}

// 测试图：在 tmj 原文里那个对象的属性表最前面加一条 Tiled int 属性 effectiveness（genmaps.py 的 prop()
// 对 int 写的就是 "type": "int"）。找不到那一段返回空串。
std::string withEffectiveness(std::string tmj, const std::string& objectName, int percent) {
    const std::size_t at = tmj.find(R"("name": ")" + objectName + R"(")");
    if (at == std::string::npos) return {};
    const std::string list = R"("properties": [)";
    const std::size_t props = tmj.find(list, at);
    if (props == std::string::npos) return {};
    tmj.insert(props + list.size(),
               R"({"name": "effectiveness", "type": "int", "value": )" + std::to_string(percent) + "}, ");
    return tmj;
}

// tmj 引用的外部 tileset（相对地图目录的路径）。loadTileMap 要它在，测试图得把它一并带过去。
std::string tilesetSource(const std::string& tmj) {
    const std::string key = R"("source": ")";
    const std::size_t at = tmj.find(key);
    if (at == std::string::npos) return {};
    const std::size_t begin = at + key.size();
    const std::size_t end = tmj.find('"', begin);
    return end == std::string::npos ? std::string{} : tmj.substr(begin, end - begin);
}

// ===========================================================================
// 1.5 第 1、2、5 条：bankMeditation 与日课
// ===========================================================================

// 1.5 第 1 条：同一份存档复制两份、同一颗种子各坐 120 日，125 那一侧是 100 那一侧的五四之比，顿悟一样。
// 种子扫一段：有顿悟的、没顿悟的都要真出现过，「顿悟一样」才不是空话。
TEST(Ch08SiteMeditation, TheSpringPaysFiveQuartersOfTheCushionWithTheSameInsight) {
    int withInsight = 0;
    int without = 0;
    for (std::uint32_t seed = 1; seed <= 32; ++seed) {
        GameState cushionSide = foundationSave();
        GameState springSide = foundationSave();
        const MeditateOutcome cushion = CultivationScene::bankMeditation(cushionSide, 120, seed, kCushion);
        const MeditateOutcome spring = CultivationScene::bankMeditation(springSide, 120, seed, kSpring);
        ASSERT_GT(cushion.cultivation, 0) << "种子 " << seed;
        EXPECT_GT(spring.cultivation, cushion.cultivation) << "种子 " << seed;
        EXPECT_TRUE(fiveQuarters(spring.cultivation, cushion.cultivation))
            << "种子 " << seed << "：灵泉 " << spring.cultivation << " 点，蒲团 " << cushion.cultivation << " 点";
        EXPECT_EQ(spring.insight, cushion.insight) << "种子 " << seed << "：顿悟是逐日的骰子，与坐在哪儿无关";
        EXPECT_EQ(spring.days, 120);
        EXPECT_EQ(springSide.cultivation, spring.cultivation) << "进账就是那么多";
        (cushion.insight ? withInsight : without) += 1;
    }
    EXPECT_GT(withInsight, 0) << "扫的种子里一次顿悟都没有，「顿悟一样」没验到";
    EXPECT_GT(without, 0);
}

// 1.5 第 2 条：三参调用与显式传 100 逐字段相等——tests/PanelTests.cpp 那一串三参调用的结果一个点都不变。
// 带着余数账接着坐几段：零头落在哪儿也得一样。
TEST(Ch08SiteMeditation, TheThreeArgumentCallIsExactlyAnExplicitHundred) {
    for (const std::uint32_t seed : {7u, 20260929u}) {
        GameState threeArgs = foundationSave();
        GameState explicitHundred = foundationSave();
        threeArgs.cultivationRemainder = 17;
        explicitHundred.cultivationRemainder = 17;
        for (const int days : {1, 10, 120, 360}) {
            const MeditateOutcome a = CultivationScene::bankMeditation(threeArgs, days, seed);
            const MeditateOutcome b = CultivationScene::bankMeditation(explicitHundred, days, seed, kCushion);
            EXPECT_EQ(a.cultivation, b.cultivation) << days << " 日";
            EXPECT_EQ(a.days, b.days) << days << " 日";
            EXPECT_EQ(a.insight, b.insight) << days << " 日";
            EXPECT_EQ(threeArgs.cultivation, explicitHundred.cultivation) << days << " 日";
            EXPECT_EQ(threeArgs.cultivationRemainder, explicitHundred.cultivationRemainder) << days << " 日";
        }
    }
}

// 1.5 第 5 条：凡人阶段的日课与改动前逐字相等（数字见 kDailyBefore*）。
TEST(Ch08SiteMeditation, DailyPracticeIsExactlyWhatItWasBeforeTheSpring) {
    GameState s = mortalSaveOwingDailyPractice();
    ASSERT_EQ(fanren::game::wordingStage(s), PanelStage::Mortal) << "先验：这是凡人阶段的存档";
    EXPECT_EQ(fanren::game::settleDailyPractice(s), kDailyBeforeGain);
    EXPECT_EQ(s.cultivation, kDailyBeforeGain);
    EXPECT_EQ(s.cultivationRemainder, kDailyBeforeRemainder);
    EXPECT_EQ(s.lastPracticeDay, kDailyBeforeWatermark);
}

// ===========================================================================
// 1.5 第 4 条：面板那一行
// ===========================================================================

TEST(Ch08SitePanel, OnlyASpotAboveTheCushionAddsTheLineAndBothStagesListItsWords) {
    const GameState immortal = foundationSave();
    const GameState mortal = mortalSaveOwingDailyPractice();
    const CultivationScene spring(kSpring);
    const CultivationScene cushion(kCushion);
    const CultivationScene plain;   // std::make_unique<CultivationScene>() 那种：缺省就是普通蒲团

    const auto& words = fanren::game::cultivationLexicon(PanelStage::Immortal);
    const std::string line = spring.siteBonusLine(immortal);
    EXPECT_NE(line.find("25"), std::string::npos) << line;
    EXPECT_EQ(line, std::string(words.siteBonusPrefix) + "25" + words.siteBonusSuffix);
    EXPECT_TRUE(cushion.siteBonusLine(immortal).empty()) << "100 的地方不画这一行";
    EXPECT_TRUE(plain.siteBonusLine(immortal).empty());

    // 凡人阶段用凡人那一套（实际走不到：灵眼之泉在第 8 章；禁词扫描照样扫它，见 PanelWording）。
    const auto& mortalWords = fanren::game::cultivationLexicon(PanelStage::Mortal);
    EXPECT_EQ(spring.siteBonusLine(mortal),
              std::string(mortalWords.siteBonusPrefix) + "25" + mortalWords.siteBonusSuffix);
    // 契约 1.4 点名的两个词。「修为」不在 PanelWording 那张禁词表里（凡人那一套说「火候」），这里单独问一句。
    for (const std::string word : {"灵气", "修为"}) {
        EXPECT_EQ(spring.siteBonusLine(mortal).find(word), std::string::npos) << word;
    }

    // 两个新字段都进了禁词扫描扫的那张清单（按值逐条比，不是找子串）。
    for (const PanelStage stage : {PanelStage::Mortal, PanelStage::Immortal}) {
        const auto& table = fanren::game::cultivationLexicon(stage);
        const std::vector<std::string> strings = fanren::game::cultivationPanelStrings(stage);
        EXPECT_STRNE(table.siteBonusPrefix, "");
        EXPECT_NE(std::find(strings.begin(), strings.end(), std::string(table.siteBonusPrefix)), strings.end())
            << table.siteBonusPrefix;
        EXPECT_NE(std::find(strings.begin(), strings.end(), std::string(table.siteBonusSuffix)), strings.end())
            << table.siteBonusSuffix;
    }
}

// ===========================================================================
// 1.5 第 3、5 条：真实入口
// ===========================================================================

class Ch08SiteEntry : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto loaded = app_.loadMap(kCushionMap, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        arrived_ = app_.state();
    }
    void TearDown() override { app_.shutdown(); }

    // 契约 1.5 第 3 条的「同一存档」：刚走进百药园的那一刻，换成筑基初期、资质 50、修仙阶段。
    [[nodiscard]] GameState foundationHere() const {
        GameState s = arrived_;
        s.realm = Realm::FoundationEarly;
        s.aptitude = 50;
        s.cultivation = 0;
        s.cultivationRemainder = 0;
        s.setFlag(fanren::game::kXiuxianKnownFlag);
        return s;
    }

    // 场景栈顶的修炼面板（面板是帧末才真正入栈的）；不是就返回 nullptr。
    CultivationScene* panelOnTop() {
        app_.tick(kFrame);
        fanren::game::Scene* top = app_.topScene();
        if (top == nullptr || top->name() != "Cultivation") return nullptr;
        return dynamic_cast<CultivationScene*>(top);
    }

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    Application app_;
    GameState arrived_;
};

// 1.5 第 3 条：125 那一侧走测试图上的设施 → openFacility；100 那一侧面朝真图上的蒲团按确认。
// 同一份存档各坐 90 日，比法同第 1 条；125 那一块面板的状态栏多一行「……25%」，蒲团那一块没有。
TEST_F(Ch08SiteEntry, ASpringOfOneHundredTwentyFiveOnTheMapPaysFiveQuartersOfTheCushion) {
    constexpr int kDays = 90;

    // ---- 125：测试图 ----
    const fs::path maps = fs::path(assetRoot()) / "maps";
    const std::string original = readFile(maps / (std::string(kCushionMap) + ".tmj"));
    const std::string patched = withEffectiveness(original, kCushionName, kSpring);
    ASSERT_FALSE(patched.empty()) << kCushionMap << ".tmj 里找不到 " << kCushionName << " 的属性表";
    const std::string tileset = tilesetSource(original);
    ASSERT_FALSE(tileset.empty());

    TempDir tmp{"fanren_ch08_spring"};
    const fs::path testMap = tmp.path() / (std::string(kCushionMap) + ".tmj");
    writeFile(testMap, patched);
    writeFile(tmp.path() / tileset, readFile(maps / tileset));
    const auto loaded = fanren::io::loadTileMap(testMap.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const MapObject* springSpot = objectNamed(loaded.value, kCushionName);
    ASSERT_NE(springSpot, nullptr);
    ASSERT_EQ(springSpot->property("kind"), "meditate");
    ASSERT_EQ(springSpot->property("effectiveness"), "125") << "Tiled 的 int 属性读进来就是这串数字";

    app_.state() = foundationHere();
    app_.openFacility(*springSpot);
    CultivationScene* springPanel = panelOnTop();
    ASSERT_NE(springPanel, nullptr) << "openFacility 没有压修炼面板";
    const std::string springLine = springPanel->siteBonusLine(app_.state());
    const MeditateOutcome springSit = springPanel->meditateFor(app_, kDays);
    const int springGain = app_.state().cultivation;
    EXPECT_EQ(springSit.cultivation, springGain) << "修仙阶段没有日课：进账全是这一次打坐";
    closePanel();

    // ---- 100：真图上的同一张蒲团，面朝它按确认 ----
    const MapObject* cushionSpot = objectNamed(*app_.currentMap(), kCushionName);
    ASSERT_NE(cushionSpot, nullptr);
    ASSERT_TRUE(cushionSpot->property("effectiveness").empty()) << "先验：真图上的蒲团没写 effectiveness";
    app_.state() = foundationHere();
    app_.state().position = Point{cushionSpot->position.x, cushionSpot->position.y + 1};
    app_.state().facing = 0;   // 朝上，正对蒲团
    WorldScene world;
    ASSERT_TRUE(world.interact(app_));
    CultivationScene* cushionPanel = panelOnTop();
    ASSERT_NE(cushionPanel, nullptr);
    const std::string cushionLine = cushionPanel->siteBonusLine(app_.state());
    cushionPanel->meditateFor(app_, kDays);
    const int cushionGain = app_.state().cultivation;
    closePanel();

    EXPECT_GT(cushionGain, 0);
    EXPECT_GT(springGain, cushionGain) << "地图上写的 125 没有走到打坐的收益里";
    EXPECT_TRUE(fiveQuarters(springGain, cushionGain))
        << "灵泉 " << springGain << " 点，蒲团 " << cushionGain << " 点：不是 125 : 100";
    EXPECT_NE(springLine.find("25"), std::string::npos) << springLine;
    EXPECT_TRUE(cushionLine.empty()) << cushionLine;
}

// 1.5 第 5 条走真面板：在一块 125 的面板上打坐，先补的那笔日课仍是改动前的数——灵脉的加成
// 只给「在那一处坐下」的人，不给这些日子里零零碎碎的功课。
TEST_F(Ch08SiteEntry, TheDailyPracticeOwedBeforeSittingAtTheSpringTakesNoShareOfIt) {
    app_.state() = mortalSaveOwingDailyPractice();
    CultivationScene spring(kSpring);
    const MeditateOutcome sit = spring.meditateFor(app_, 10);
    EXPECT_GT(sit.cultivation, 0);
    EXPECT_EQ(app_.state().cultivation - sit.cultivation, kDailyBeforeGain)
        << "补进来的日课沾了此地的光";
}

}  // namespace
