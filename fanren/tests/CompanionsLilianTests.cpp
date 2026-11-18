// 伙伴改版 D 路·历练（docs/interfaces-companions.md 3.12、第 5 节「历练」）。走真实入口：
// 世界层面朝 kind=lilian 的设施按确认 → 组队面板（LilianPartyScene）按键挑人 → 出发 → 历练图；
// 历练图上面朝出口按确认 → 回来。期望值照契约写死：
//   · 进去再出来，队伍、图、位置、朝向恢复，日历加一天；
//   · 开过的机关第二趟进去是关着的；永久集合与旗标一样没被写；
//   · 编成 id 按境界拼 _t1 / _t2 / _t3，高过最高一档取最高一档，历练外不拼；
//   · 同一天同一处 chance 的结果相同，换一天会变；没出现的明雷记成已清、宝箱记成已开；
//   · 名册上的人至多挑 3 位，每位用 currentRole 给的角色；没解锁不弹面板；
//   · 入口图与原图都读不回来时如实报两样失败、存档里记的仍是原图。
// 历练地定义的坏数据：加载器逐条报错，写坏一份整个 init 失败。
#include <algorithm>
#include <map>
#include <set>

#include "CompanionsLaneDFixture.h"
#include "core/rules/Lilian.h"
#include "game/LilianPartyScene.h"
#include "io/LilianLoader.h"

namespace {

using namespace fanren::test::laned;
using fanren::game::LilianPartyScene;
using fanren::rules::Realm;

class LilianD : public LaneD {
protected:
    LilianPartyScene* panel() { return dynamic_cast<LilianPartyScene*>(app_.topScene()); }
    // 面朝入口按确认，等面板上来。
    void openPanel() {
        press(SDL_SCANCODE_RETURN);
        tick();
        ASSERT_EQ(top(), "LilianParty");
    }
    // 面板上：往下移 rows 行，按确认。
    void pick(int rows) {
        for (int i = 0; i < rows; ++i) press(SDL_SCANCODE_DOWN);
        press(SDL_SCANCODE_RETURN);
    }
    // 面板上：往下 downs 行到「出发」（最后一行）按确认。
    void depart(int downs) {
        for (int i = 0; i < downs; ++i) press(SDL_SCANCODE_DOWN);
        press(SDL_SCANCODE_RETURN);
        tick(2);
    }
    // 从 probe_home 走真入口进历练，带上 probe_a（已长成 → li_feiyu）与 probe_b。
    void enterWithAB() {
        meet("a");
        meet("b");
        app_.state().setFlag("probe.a.grown");
        app_.state().setFlag("probe.lilian.open");
        openPanel();
        press(SDL_SCANCODE_RETURN);   // 第 1 行：probe_a
        pick(1);                      // 第 2 行：probe_b
        ASSERT_EQ(panel()->chosenRoles(), (std::vector<std::string>{"li_feiyu", "baiyi_shaonv"}));
        depart(1);   // 名册两位，第 3 行是出发
        ASSERT_TRUE(app_.inLilian());
        ASSERT_EQ(top(), "World");
    }
    // 历练图上面朝出口按确认，回来（再按掉那句「归来」）。
    void leaveByExit() {
        press(SDL_SCANCODE_RETURN);
        tick();
        ASSERT_FALSE(app_.inLilian());
        ASSERT_EQ(top(), "Dialogue");
        press(SDL_SCANCODE_RETURN);
        tick(2);
    }
};

// ---------------------------------------------------------------------------
// 入口、组队、出口
// ---------------------------------------------------------------------------

TEST_F(LilianD, TheFacilityOpensThePanelAndTheExitRestoresEverything) {
    app_.state().party = {{"qu_hun", 17, true}};
    const fanren::core::Point home = app_.state().position;
    ASSERT_EQ(home.x, 12);
    ASSERT_EQ(home.y, 11);
    ASSERT_EQ(app_.state().facing, 3);

    enterWithAB();
    EXPECT_EQ(app_.state().mapId, "lilian_probe_a");
    ASSERT_EQ(app_.state().party.size(), 2u);
    EXPECT_EQ(app_.state().party[0].roleId, "li_feiyu") << "probe.a.grown 已置：currentRole 给第二段";
    EXPECT_EQ(app_.state().party[1].roleId, "baiyi_shaonv");
    EXPECT_FALSE(app_.quickSave().ok) << "历练里存不了";

    leaveByExit();
    EXPECT_EQ(app_.state().mapId, "probe_home");
    EXPECT_EQ(app_.state().position.x, 12);
    EXPECT_EQ(app_.state().position.y, 11);
    EXPECT_EQ(app_.state().facing, 3);
    EXPECT_EQ(app_.state().day, 41) << "出来日历加一天";
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "qu_hun");
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.lilian.back"));
}

TEST_F(LilianD, BeforeItIsUnlockedThereIsNoPanel) {
    meet("a");
    press(SDL_SCANCODE_RETURN);
    tick();
    EXPECT_EQ(top(), "Dialogue");
    EXPECT_EQ(lastLine(), app_.data().lookupText("ui.lilian.locked"));
    EXPECT_FALSE(app_.inLilian());
}

TEST_F(LilianD, ThePanelListsOnlyTheRosterAndTakesAtMostThree) {
    for (const char* c : {"a", "b", "c", "d"}) meet(c);   // probe_e 没结识
    app_.state().setFlag("probe.lilian.open");
    openPanel();
    // 名册次序（伙伴 id）：probe_a（还没长成 → zhang_tie）、probe_b、probe_c、probe_d，末行出发。
    press(SDL_SCANCODE_RETURN);
    pick(1);
    pick(1);
    pick(1);   // 第四位：挑不上
    EXPECT_EQ(panel()->chosenRoles(), (std::vector<std::string>{"zhang_tie", "baiyi_shaonv", "chen_shimei"}));
    // 放下一位，第四位就挑得上了。
    press(SDL_SCANCODE_UP);
    press(SDL_SCANCODE_RETURN);
    press(SDL_SCANCODE_DOWN);
    press(SDL_SCANCODE_RETURN);
    EXPECT_EQ(panel()->chosenRoles(), (std::vector<std::string>{"zhang_tie", "baiyi_shaonv", "cao_lu"}));
    press(SDL_SCANCODE_DOWN);   // 第 5 行就是出发：名册上只有四位
    press(SDL_SCANCODE_RETURN);
    tick(2);
    ASSERT_TRUE(app_.inLilian());
    ASSERT_EQ(app_.state().party.size(), 3u);
    for (const auto& member : app_.state().party) {
        EXPECT_EQ(member.hp, -1) << "这一趟满血进去";
        EXPECT_TRUE(member.active);
    }
}

TEST_F(LilianD, AnEmptyRosterGoesAloneAndEscapeChangesNothing) {
    app_.state().setFlag("probe.lilian.open");
    app_.state().party = {{"qu_hun", 17, true}};
    openPanel();
    press(SDL_SCANCODE_ESCAPE);
    tick(2);
    EXPECT_EQ(top(), "World");
    EXPECT_FALSE(app_.inLilian());

    openPanel();
    press(SDL_SCANCODE_RETURN);   // 名册空着：唯一一行就是出发
    tick(2);
    ASSERT_TRUE(app_.inLilian());
    EXPECT_TRUE(app_.state().party.empty()) << "独自前往";
}

TEST_F(LilianD, TheSecondTripStartsWithGatesClosedAndNothingPermanentIsWritten) {
    app_.state().worldObjects.openedChests = {"keep.me"};
    enterWithAB();
    const std::map<std::string, int> flagsInside = app_.state().flags;
    // 世界逻辑开机关、开宝箱、清明雷，都经 worldObjects() 记账（C 路）；这里照它的口子记。
    app_.worldObjects().openedGates.insert("lilian_probe_a.gate");
    app_.worldObjects().openedChests.insert("lilian_probe_a.chest_fixed");
    app_.worldObjects().clearedFoes["lilian_probe_a.foe_fixed"] = app_.state().day;
    leaveByExit();

    const fanren::core::WorldObjectState& kept = app_.state().worldObjects;
    EXPECT_EQ(kept.openedChests, (std::set<std::string>{"keep.me"})) << "永久集合没被写";
    EXPECT_TRUE(kept.openedGates.empty());
    EXPECT_TRUE(kept.clearedFoes.empty());
    EXPECT_TRUE(kept.gatheredDay.empty());
    EXPECT_EQ(app_.state().flags, flagsInside) << "历练里没置任何旗标";

    openPanel();
    depart(2);
    ASSERT_TRUE(app_.inLilian());
    EXPECT_EQ(app_.worldObjects().openedGates.count("lilian_probe_a.gate"), 0u) << "第二趟机关是关着的";
    EXPECT_EQ(app_.worldObjects().openedChests.count("lilian_probe_a.chest_fixed"), 0u);
    EXPECT_EQ(app_.worldObjects().clearedFoes.count("lilian_probe_a.foe_fixed"), 0u);
}

// ---------------------------------------------------------------------------
// 按境界分档
// ---------------------------------------------------------------------------

TEST_F(LilianD, BattlesInsideALilianGetTheTierSuffixAndOutsideDoNot) {
    // 每次开仗只压一层 BattleScene、不推帧：这里只看开的是哪一场。
    const auto opened = [this](Realm realm) {
        app_.state().realm = realm;
        app_.startBattle(fanren::game::BattleStart{"probe_foe", true}, [](const auto&) {});
        const std::string id = app_.battleStart().battleId;
        EXPECT_TRUE(app_.battleStart().preemptive) << "先手原样带过去";
        return id;
    };
    enterWithAB();
    EXPECT_EQ(opened(Realm::QiRefining3), "probe_foe_t1");
    EXPECT_EQ(opened(Realm::QiRefining13), "probe_foe_t1") << "等于上限仍在这一档";
    EXPECT_EQ(opened(Realm::FoundationEarly), "probe_foe_t2");
    EXPECT_EQ(opened(Realm::FoundationMid), "probe_foe_t3");
    EXPECT_EQ(opened(Realm::FoundationLate), "probe_foe_t3") << "高过最高一档取最高一档";
    EXPECT_EQ(opened(Realm::CoreLate), "probe_foe_t3");
}

TEST_F(LilianD, OutsideALilianTheBattleIdIsPassedThrough) {
    app_.state().realm = Realm::FoundationMid;
    app_.startBattle(fanren::game::BattleStart{"probe_foe"}, [](const auto&) {});
    EXPECT_EQ(app_.battleStart().battleId, "probe_foe");
}

TEST(LilianTierRule, TiersCountFromOneAndASiteWithoutTiersKeepsTheId) {
    fanren::core::LilianSite site;
    EXPECT_EQ(fanren::rules::lilianTier(site, Realm::CoreLate), 0);
    EXPECT_EQ(fanren::rules::tieredBattleId("b", 0), "b");
    site.tiers = {{Realm::QiRefining5}};
    EXPECT_EQ(fanren::rules::lilianTier(site, Realm::Mortal), 1);
    EXPECT_EQ(fanren::rules::lilianTier(site, Realm::CoreLate), 1);
    EXPECT_EQ(fanren::rules::tieredBattleId("b", 2), "b_t2");
}

// ---------------------------------------------------------------------------
// chance
// ---------------------------------------------------------------------------

struct Rolled {
    std::set<std::string> chests;
    std::set<std::string> foes;
    bool operator==(const Rolled& other) const { return chests == other.chests && foes == other.foes; }
};

class LilianChance : public LilianD {
protected:
    // 在第 day 天进去，记下这一趟一进门就记进账里的东西，再出来。
    Rolled tripOn(int day) {
        app_.state().day = day;
        Rolled out;
        if (!app_.inLilian()) {
            meet("a");
            app_.state().setFlag("probe.lilian.open");
            openPanel();
            depart(1);
        }
        EXPECT_TRUE(app_.inLilian());
        out.chests = app_.worldObjects().openedChests;
        for (const auto& [id, clearedOn] : app_.worldObjects().clearedFoes) {
            EXPECT_EQ(clearedOn, day) << id;
            out.foes.insert(id);
        }
        leaveByExit();
        return out;
    }
};

TEST_F(LilianChance, TheSameDaySameSiteRollsTheSameAndAnotherDayDiffers) {
    const Rolled first = tripOn(40);
    const Rolled again = tripOn(40);
    EXPECT_TRUE(first == again) << "同一天同一处，结果相同";

    // 没出现的只会是带 chance 的；不带 chance 的每趟都在。两张图都抽。
    for (const std::set<std::string>* ids : {&first.chests, &first.foes}) {
        for (const std::string& id : *ids) {
            EXPECT_EQ(id.find("_fixed"), std::string::npos) << id;
            EXPECT_TRUE(id.rfind("lilian_probe_a.", 0) == 0 || id.rfind("lilian_probe_b.", 0) == 0) << id;
        }
    }
    // 50% 的 48 个对象：一个也没抽掉、或全抽掉，抽签就没在起作用。
    const std::size_t gone = first.chests.size() + first.foes.size();
    EXPECT_GT(gone, 0u);
    EXPECT_LT(gone, static_cast<std::size_t>(4 * kChanceObjects));

    bool someDayDiffers = false;
    for (int day = 41; day <= 43; ++day) someDayDiffers = someDayDiffers || !(tripOn(day) == first);
    EXPECT_TRUE(someDayDiffers) << "换一天会变";
}

TEST(LilianChanceRule, TheSeedDependsOnDayAndSiteAndTheEdgesAreFixed) {
    using fanren::rules::lilianChanceShows;
    using fanren::rules::lilianSeed;
    EXPECT_EQ(lilianSeed(40, "jindi"), lilianSeed(40, "jindi"));
    EXPECT_NE(lilianSeed(40, "jindi"), lilianSeed(41, "jindi"));
    EXPECT_NE(lilianSeed(40, "jindi"), lilianSeed(40, "jindi2"));
    EXPECT_TRUE(lilianChanceShows(lilianSeed(40, "x"), "a", 100));
    EXPECT_FALSE(lilianChanceShows(lilianSeed(40, "x"), "a", 0));
}

// ---------------------------------------------------------------------------
// 回滚：入口图进不去、原图也读不回来（底座把这一支的失败吞掉了，D 路补上）
// ---------------------------------------------------------------------------

TEST_F(LilianD, WhenNeitherTheEntranceNorTheOriginalMapLoadsBothFailuresAreReported) {
    writeMap(root_->path(), "probe_home_gone", standAndFacility("zhanwei", "probe_missing"));
    ASSERT_TRUE(app_.loadMap("probe_home_gone", std::string{}).ok);
    tick(2);
    app_.state().party = {{"qu_hun", 17, true}};
    app_.state().setFlag("probe.lilian.open");
    openPanel();
    fs::remove(root_->path() / "maps" / "probe_home_gone.tmj");   // 原图此刻读不回来了
    depart(0);   // 名册空着：唯一一行就是出发

    EXPECT_FALSE(app_.inLilian());
    ASSERT_EQ(top(), "Dialogue");
    const std::string said = lastLine();
    EXPECT_EQ(said.rfind("进不去：入口图进不去（", 0), 0u) << said;
    EXPECT_NE(said.find("原图也读不回来（"), std::string::npos) << said;
    // 存档里记的仍是原图、原来那一格：此刻存下去，读回来就是进去之前。
    EXPECT_EQ(app_.state().mapId, "probe_home_gone");
    EXPECT_EQ(app_.state().position.x, 12);
    EXPECT_EQ(app_.state().position.y, 11);
    EXPECT_EQ(app_.state().facing, 3);
    ASSERT_EQ(app_.state().party.size(), 1u);
    EXPECT_EQ(app_.state().party[0].roleId, "qu_hun");
    EXPECT_EQ(app_.state().party[0].hp, 17);
}

// ---------------------------------------------------------------------------
// 历练地定义的坏数据
// ---------------------------------------------------------------------------

class LilianSiteFile : public ::testing::Test {
protected:
    void SetUp() override { dir_ = std::make_unique<fanren::test::TempDir>("fanren_lilian_site"); }
    // 把 kSiteJson 里的 from 换成 to，写成 <name>.json 再读。
    fanren::core::Result<fanren::core::LilianSite> load(const std::string& from, const std::string& to,
                                                        const std::string& name = "probe_jindi") {
        std::string text = kSiteJson;
        if (!from.empty()) {
            const auto at = text.find(from);
            EXPECT_NE(at, std::string::npos) << from;
            if (at != std::string::npos) text.replace(at, from.size(), to);
        }
        const fs::path path = dir_->path() / (name + ".json");
        writeFile(path, text);
        return fanren::io::loadLilianSiteFile(path.string());
    }
    std::unique_ptr<fanren::test::TempDir> dir_;
};

TEST_F(LilianSiteFile, AGoodFileReadsEveryField) {
    const auto site = load("", "");
    ASSERT_TRUE(site.ok) << site.error;
    EXPECT_EQ(site.value.id, "probe_jindi");
    EXPECT_EQ(site.value.nameKey, "probe.lilian.name");
    EXPECT_EQ(site.value.unlockFlag, "probe.lilian.open");
    EXPECT_EQ(site.value.entryMap, "lilian_probe_a");
    EXPECT_EQ(site.value.entrySpawn, "rukou");
    EXPECT_EQ(site.value.maps, (std::vector<std::string>{"lilian_probe_a", "lilian_probe_b"}));
    ASSERT_EQ(site.value.tiers.size(), 3u);
    EXPECT_EQ(site.value.tiers[0].maxRealm, Realm::QiRefining13);
    EXPECT_EQ(site.value.tiers[1].maxRealm, Realm::FoundationEarly);
    EXPECT_EQ(site.value.tiers[2].maxRealm, Realm::FoundationMid);
    // spawn 可以不写。
    const auto noSpawn = load(R"(, "spawn": "rukou")", "");
    ASSERT_TRUE(noSpawn.ok) << noSpawn.error;
    EXPECT_TRUE(noSpawn.value.entrySpawn.empty());
}

TEST_F(LilianSiteFile, EveryBrokenShapeIsRefused) {
    const std::vector<std::pair<std::string, std::string>> broken = {
        {R"("note": "D 路用例的探针")", R"("notes": "x")"},                    // 表外字段
        {R"(, "unlock_flag": "probe.lilian.open")", ""},                       // 少了解锁旗标
        {R"("name_key": "probe.lilian.name")", R"("name_key": "")"},           // 名字空串
        {R"("entry": {"map": "lilian_probe_a", "spawn": "rukou"})", R"("entry": "lilian_probe_a")"},
        {R"("spawn": "rukou")", R"("spawn": "rukou", "facing": "up")"},       // entry 里表外字段
        {R"("lilian_probe_a", "lilian_probe_b")", R"("lilian_probe_a", "ch07_jindi")"},   // 不以 lilian_ 开头
        {R"("lilian_probe_a", "lilian_probe_b")", R"("lilian_probe_b")"},      // 入口图不在 maps 里
        {R"("lilian_probe_a", "lilian_probe_b")", R"("lilian_probe_a", "lilian_probe_a")"},   // 重复
        {R"("maps": ["lilian_probe_a", "lilian_probe_b"])", R"("maps": [])"},
        {R"({"max_realm": "QiRefining13"}, {"max_realm": "FoundationEarly"}, {"max_realm": "FoundationMid"})", ""},
        {R"({"max_realm": "FoundationMid"})", R"({"max_realm": "FoundationMid"}, {"max_realm": "FoundationLate"})"},
        {R"({"max_realm": "FoundationEarly"})", R"({"max_realm": "QiRefining13"})"},   // 不严格递增
        {R"({"max_realm": "FoundationEarly"})", R"({"max_realm": "Foundation"})"},     // 不是境界名
        {R"({"max_realm": "FoundationEarly"})", R"({"max_realm": "FoundationEarly", "suffix": "_x"})"},
    };
    for (const auto& [from, to] : broken) {
        const auto site = load(from, to);
        EXPECT_FALSE(site.ok) << "该报错却读进来了：" << from << " → " << to;
        EXPECT_FALSE(site.error.empty());
    }
    EXPECT_FALSE(load("", "", "another_name").ok) << "id 与文件名不一致";
}

TEST_F(LilianSiteFile, AMissingDirectoryIsNoSitesAndOneBadFileFailsTheWholeDirectory) {
    const auto none = fanren::io::loadLilianSites((dir_->path() / "no_such_dir").string());
    ASSERT_TRUE(none.ok) << none.error;
    EXPECT_TRUE(none.value.empty());
    ASSERT_TRUE(load("", "").ok);
    const auto one = fanren::io::loadLilianSites(dir_->path().string());
    ASSERT_TRUE(one.ok) << one.error;
    EXPECT_EQ(one.value.count("probe_jindi"), 1u);
    writeFile(dir_->path() / "broken.json", "{");
    EXPECT_FALSE(fanren::io::loadLilianSites(dir_->path().string()).ok);
}

// 真入口：data/lilian 里写坏一份，或名字的文案 key 查不到，整个 init 失败、说得出是哪一份。
class LilianBadData : public ::testing::Test {
protected:
    static void SetUpTestSuite() { root_ = makeRoot("fanren_lilian_bad"); }
    static void TearDownTestSuite() { root_.reset(); }
    void TearDown() override { writeFile(root_->path() / "data" / "lilian" / "probe_jindi.json", kSiteJson); }
    std::string initError(const std::string& siteText) {
        writeFile(root_->path() / "data" / "lilian" / "probe_jindi.json", siteText);
        fanren::game::Application app;
        auto ready = app.init(root_->path().string(), /*headless=*/true);
        app.shutdown();
        return ready.ok ? std::string{} : ready.error;
    }
    static inline std::unique_ptr<fanren::test::TempDir> root_;
};

TEST_F(LilianBadData, ABrokenSiteOrAMissingNameKeyStopsInit) {
    EXPECT_TRUE(initError(kSiteJson).empty()) << "对照：好的那份 init 得过";
    std::string tiers = kSiteJson;
    tiers.replace(tiers.find("QiRefining13"), 12, "Nonsense");
    const std::string badTier = initError(tiers);
    EXPECT_NE(badTier.find("probe_jindi"), std::string::npos) << badTier;
    EXPECT_NE(badTier.find("max_realm"), std::string::npos) << badTier;
    std::string name = kSiteJson;
    name.replace(name.find("probe.lilian.name"), 17, "probe.lilian.nope");
    const std::string badName = initError(name);
    EXPECT_NE(badName.find("probe.lilian.nope"), std::string::npos) << badName;
}

}  // namespace
