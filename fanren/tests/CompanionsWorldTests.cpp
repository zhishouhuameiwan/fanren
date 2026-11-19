// 伙伴改版·C 路「世界逻辑」（docs/interfaces-companions.md 2.4、3.4–3.8、3.11，第 5 节「跟随」「四种对象」）。
//
// 全部驱动真的 WorldScene::tryStep / interact，挂在真的 Application 上：跟随者从 describeFrame 取（画面拿到的
// 就是这一份），对象状态从 Application::worldObjects() 与存档取，开战从 Application::battleStart() 取。
// 明雷的胜、逃、负各有一条**真打**的：BattleScene 由 tests/BattleHand.h 那只手按真菜单打完，收场照真流程
// 先回写气血、发奖励，再走明雷登记的回调。偷袭判定、敌人跑了、历练里清掉这几条只看回调，替战斗场景报战果。
//
// 资产根是一份临时拷贝（与 CompanionsBaseWiringTests 同一个做法）：data/companions/ 里放四位探针伙伴，
// maps/ 里放一张探针图 probe_world（四种新对象、插话触发器、伙伴 NPC 各一处），scripts/t/ 里放两段小脚本。
// 期望值照契约写死：谁跟在哪一格、宝箱给几件、偷袭算不算先手、提示说哪一句，都不从被测代码推。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "BattleHand.h"
#include "TempDir.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/DialogueScene.h"
#include "game/WorldScene.h"
#include "game/WorldView.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::Point;
using fanren::core::battle::BattlePhase;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::WorldFrame;
using fanren::game::WorldScene;

fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// ---- 探针伙伴：角色都是 data/roles 里现成的 ----
// probe_a 先后用张铁、厉飞雨两个角色（probe.grown 置了换人），会 break_seal；probe_b 墨大夫，会 illusion；
// probe_c、probe_d 只为凑够四位伙伴（第 4 位不上场）。韩母（han_mu）不是伙伴——傀儡、化身那一类的替身。
constexpr const char* kCompanionA = R"({"id": "probe_a", "met_flag": "probe.met_a", "field_skill": "break_seal",
  "roles": [{"role": "zhang_tie", "until_flag": "probe.grown"}, {"role": "li_feiyu"}]})";
constexpr const char* kCompanionB = R"({"id": "probe_b", "met_flag": "probe.met_b", "field_skill": "illusion",
  "roles": [{"role": "mo_daifu"}]})";
constexpr const char* kCompanionC = R"({"id": "probe_c", "met_flag": "probe.met_c", "roles": [{"role": "chen_shimei"}]})";
constexpr const char* kCompanionD = R"({"id": "probe_d", "met_flag": "probe.met_d", "roles": [{"role": "baiyi_shaonv"}]})";

// ---- 探针图 probe_world：16×12，全是空地 ----
constexpr int kW = 16;
constexpr int kH = 12;

std::string prop(const std::string& name, const std::string& value) {
    return R"({"name":")" + name + R"(","type":"string","value":")" + value + R"("})";
}
std::string propInt(const std::string& name, int value) {
    return R"({"name":")" + name + R"(","type":"int","value":)" + std::to_string(value) + "}";
}
std::string propBool(const std::string& name, bool value) {
    return R"({"name":")" + name + R"(","type":"bool","value":)" + (value ? "true" : "false") + "}";
}
std::string object(const std::string& name, const std::string& type, int x, int y,
                   const std::vector<std::string>& props, int w = 1) {
    std::string list;
    for (const std::string& p : props) list += (list.empty() ? "" : ",") + p;
    return R"({"id":0,"name":")" + name + R"(","type":")" + type + R"(","x":)" + std::to_string(x * 32) +
           R"(,"y":)" + std::to_string(y * 32) + R"(,"width":)" + std::to_string(w * 32) +
           R"(,"height":32,"rotation":0,"visible":true,)" + R"("properties":[)" + list + "]}";
}
std::string foeAt(const std::string& name, int x, int y, const std::string& facing, const std::string& battle,
                  const std::vector<std::string>& extra = {}) {
    std::vector<std::string> props{prop("id", "probe_world." + name), prop("battle", battle), prop("look", "wild_wolf"),
                                   prop("facing", facing)};
    props.insert(props.end(), extra.begin(), extra.end());
    return object(name, "foe", x, y, props);
}

// 两份不致命、可以逃的夹具编成：谷外孤狼（现成的野外遭遇，打得过）与 probe_foe_hard（同一张编成换上
// 第 13 章的强敌，凡人打不过）。明雷的编成一律不致命、普通明雷可逃（契约 3.4）。
constexpr const char* kWolfBattle = "be03_guwai_gulang";
constexpr const char* kHardBattle = "probe_foe_hard";

std::string probeMap() {
    std::string zeros;
    for (int i = 0; i < kW * kH; ++i) zeros += (i == 0 ? "0" : ",0");
    std::string layers;
    for (const char* name : {"ground", "overlay", "building", "front", "collision"}) {
        layers += R"({"name":")" + std::string(name) + R"(","type":"tilelayer","width":)" + std::to_string(kW) +
                  R"(,"height":)" + std::to_string(kH) + R"(,"data":[)" + zeros + "]},";
    }
    const std::vector<std::string> objects{
        object("spawn_main", "spawn", 1, 1, {prop("id", "spawn_main"), propBool("default", true), prop("facing", "down")}),
        object("spawn_east", "spawn", 10, 1, {prop("id", "spawn_east"), propBool("default", false), prop("facing", "left")}),
        object("trigger_step", "trigger", 4, 1, {prop("script", "t/noop.lua"), prop("mode", "enter"), propBool("once", false)}),
        object("portal_loop", "portal", 6, 6, {prop("target_map", "probe_world"), prop("target_spawn", "spawn_east")}),
        object("trigger_chat", "trigger", 1, 6,
               {prop("script", "t/chat.lua"), prop("mode", "enter"), propBool("once", false), prop("require_party", "probe_a")}),
        object("npc_zhang", "npc", 6, 2, {prop("role_id", "zhang_tie"), prop("facing", "down")}),
        object("npc_hanmu", "npc", 8, 2, {prop("role_id", "han_mu"), prop("facing", "down")}),
        object("chest_a", "chest", 2, 4, {prop("id", "probe_world.chest_a"), prop("loot", "material_dansha:2,material_fuzhi")}),
        object("gather_a", "gather", 4, 4,
               {prop("id", "probe_world.gather_a"), prop("item", "herb_huangjing_cao"), propInt("respawn_days", 3)}),
        foeAt("foe_a", 8, 4, "left", kWolfBattle, {propInt("respawn_days", 2)}),
        foeAt("foe_hard", 14, 2, "left", kHardBattle),           // 面前 (13,2)
        foeAt("foe_up", 2, 10, "up", kWolfBattle),               // 面前 (2,9)
        foeAt("foe_right", 5, 10, "right", kWolfBattle),         // 面前 (6,10)
        foeAt("foe_down", 8, 9, "down", kWolfBattle),            // 面前 (8,10)
        foeAt("foe_trap", 11, 10, "left", kWolfBattle),          // 面前 (10,10)，那一格另压着一个踏入触发器
        object("trigger_trap", "trigger", 10, 10, {prop("script", "t/noop.lua"), prop("mode", "enter"), propBool("once", false)}),
        // require_flag 没置时都不在。
        object("chest_r", "chest", 13, 9, {prop("id", "probe_world.chest_r"), prop("loot", "material_fuzhi"),
                                          prop("require_flag", "probe.show")}),
        object("gather_r", "gather", 14, 9, {prop("id", "probe_world.gather_r"), prop("item", "herb_huangjing_cao"),
                                            propInt("respawn_days", 1), prop("require_flag", "probe.show")}),
        foeAt("foe_r", 15, 11, "up", kWolfBattle, {prop("require_flag", "probe.show")}),   // 面前 (15,10)
        object("gate_wide", "gate", 12, 6, {prop("id", "probe_world.gate_wide"), prop("skill", "illusion")}, 2),
        object("gate_a", "gate", 10, 4, {prop("id", "probe_world.gate_a"), prop("skill", "break_seal"), prop("opens", "probe.gate_a_open")}),
        object("gate_b", "gate", 10, 6, {prop("id", "probe_world.gate_b"), prop("skill", "illusion")}),
        object("gate_c", "gate", 11, 6, {prop("id", "probe_world.gate_c"), prop("skill", "nobody"), prop("hint", "ui.facility.locked")}),
    };
    std::string list;
    for (const std::string& o : objects) list += (list.empty() ? "" : ",") + o;
    layers += R"({"name":"objects","type":"objectgroup","objects":[)" + list + "]}";
    return R"({"width":)" + std::to_string(kW) + R"(,"height":)" + std::to_string(kH) +
           R"(,"tilewidth":32,"tileheight":32,"orientation":"orthogonal","layers":[)" + layers + R"(],"properties":[)" +
           prop("map_id", "probe_world") + "," + prop("display_name_key", "ch01.map.hanjiacun.name") + "," +
           prop("region", "probe") + "," + prop("bgm", "none") + "," + propInt("chapter", 7) + "," +
           propBool("outdoor", true) + "," + propBool("can_leave_edge", false) + "]}";
}

class CompanionsWorld : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        root_ = std::make_unique<fanren::test::TempDir>("fanren_companions_world");
        for (const char* dir : {"data", "scripts", "maps"}) {
            fs::copy(repoRoot() / dir, root_->path() / dir, fs::copy_options::recursive);
        }
        // 真树里从样板段起有了伙伴数据（nangong_wan、chen_qiaoqian），它们的角色与下面的探针伙伴重叠
        // （一个角色只能属于一位伙伴，加载器拒收）。这些用例量的是探针伙伴，拷进来之后先把真伙伴拿掉。
        fs::remove_all(root_->path() / "data" / "companions");
        writeFile(root_->path() / "data" / "companions" / "probe_a.json", kCompanionA);
        writeFile(root_->path() / "data" / "companions" / "probe_b.json", kCompanionB);
        writeFile(root_->path() / "data" / "companions" / "probe_c.json", kCompanionC);
        writeFile(root_->path() / "data" / "companions" / "probe_d.json", kCompanionD);
        writeFile(root_->path() / "maps" / "probe_world.tmj", probeMap());
        writeFile(root_->path() / "scripts" / "t" / "noop.lua", R"(flag.set("probe.noop"))");
        writeFile(root_->path() / "scripts" / "t" / "chat.lua", R"(flag.set("probe.chatted"))");
        // probe_foe_hard：照谷外孤狼那张编成（可逃、不致命），敌人换成第 13 章的强敌。
        std::ifstream in(root_->path() / "data" / "battles" / (std::string(kWolfBattle) + ".json"), std::ios::binary);
        std::string battle((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (const auto& [from, to] : {std::pair<std::string, std::string>{kWolfBattle, kHardBattle},
                                       std::pair<std::string, std::string>{"wild_wolf", "feng_xi"}}) {
            for (std::size_t at = battle.find(from); at != std::string::npos; at = battle.find(from, at + to.size())) {
                battle.replace(at, from.size(), to);
            }
        }
        writeFile(root_->path() / "data" / "battles" / (std::string(kHardBattle) + ".json"), battle);
    }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
        ASSERT_EQ(app_.data().companions.size(), 4u) << "四位探针伙伴都读进来了";
        ASSERT_TRUE(app_.loadMap("probe_world", std::string{}).ok);
        app_.pushScene(std::make_unique<WorldScene>());
        tick();
        ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
    }
    void TearDown() override {
        app_.shutdown();
        std::error_code ec;
        fs::remove(app_.quickSavePath(), ec);
    }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    // 收掉压在世界层上的对话框，回到世界层。
    void drain() {
        tick();
        for (int guard = 0; guard < 16 && dynamic_cast<fanren::game::DialogueScene*>(app_.topScene()) != nullptr; ++guard) {
            app_.popScene();
            tick();
        }
        for (int frame = 0; frame < 64 && app_.scripts().isRunning(); ++frame) tick();
    }
    WorldScene& scene() {
        auto* w = dynamic_cast<WorldScene*>(app_.topScene());
        EXPECT_NE(w, nullptr) << "栈顶应是世界层，实际是 " << (app_.topScene() ? app_.topScene()->name() : "空");
        return *w;
    }
    bool step(int dx, int dy) {
        const bool moved = scene().tryStep(app_, dx, dy);
        drain();
        return moved;
    }
    void stand(Point at, int facing) {
        app_.state().position = at;
        app_.state().facing = facing;
    }
    void party(const std::vector<std::pair<std::string, bool>>& members) {
        app_.state().party.clear();
        for (const auto& [role, active] : members) app_.state().party.push_back({role, -1, active});
    }
    const WorldFrame& frame() { return scene().describeFrame(app_); }
    std::string lastLine() const {
        return app_.dialogueLog().empty() ? std::string{} : app_.dialogueLog().back().body;
    }
    // 收场：BattleScene 收场时调的是 completeCommand，明雷登记的回调由它转过去。这里先替它报一个战果
    //（仗的胜负不归本路，要的是回调对每一种战果做对了）。
    void finishBattle(bool won, const std::string& code) {
        ASSERT_EQ(app_.topScene() != nullptr ? app_.topScene()->name() : "", "Battle");
        fanren::script::CommandResult result;
        result.battleWon = won;
        result.code = code;
        app_.completeCommand(result);
        // 无头下那一仗下一帧自己跑完、自己收场出栈；回调已经用掉，它收场时不会再惊动谁。
        tick();
        ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
    }
    // 存档、读档：读档会新建一个 WorldScene（Application::continueJourney）。
    void saveAndReload() {
        const auto saved = app_.quickSave();
        ASSERT_TRUE(saved.ok) << saved.error;
        const auto loaded = app_.continueJourney(app_.quickSavePath());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        tick();
        ASSERT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr);
    }
    int count(const std::string& item) { return app_.state().itemCount(item); }

    // 真打一场：开战那一下之后一帧，战斗场景压上栈顶；那只手按真菜单打到分出胜负；再一帧，战斗场景照真流程
    // 收场（回写气血、发奖励、调 completeCommand → 明雷的回调）、出栈。flee 为真时一开打就逃。
    BattlePhase fight(bool flee = false) {
        tick();
        auto* battle = dynamic_cast<BattleScene*>(app_.topScene());
        EXPECT_NE(battle, nullptr) << "该开打了";
        if (battle == nullptr) return BattlePhase::Ongoing;
        fanren::test::BattleHand hand(app_);
        hand.fleeAtOnce(flee);
        const BattlePhase phase = hand.play(*battle);
        tick();
        EXPECT_NE(dynamic_cast<WorldScene*>(app_.topScene()), nullptr) << "收场之后回到世界层";
        return phase;
    }
    void sturdyHero() {
        app_.state().maxHp = 400;
        app_.state().hp = 400;
    }
    bool battling() {
        tick();
        return dynamic_cast<BattleScene*>(app_.topScene()) != nullptr;
    }

    static std::unique_ptr<fanren::test::TempDir> root_;
    Application app_;
};

std::unique_ptr<fanren::test::TempDir> CompanionsWorld::root_;

std::vector<Point> cells(const WorldFrame& frame) {
    std::vector<Point> out;
    for (const auto& f : frame.followers) out.push_back(f.cell);
    return out;
}
std::vector<std::string> roles(const WorldFrame& frame) {
    std::vector<std::string> out;
    for (const auto& f : frame.followers) out.push_back(f.roleId);
    return out;
}
// 这一帧里名为 name 的那个对象（宝箱、采集点、明雷、机关各一张表）；不在为 nullptr。
template <typename Entry>
const Entry* named(const std::vector<Entry>& list, const std::string& name) {
    for (const Entry& entry : list) {
        if (entry.object != nullptr && entry.object->name == name) return &entry;
    }
    return nullptr;
}

constexpr Point kSpawn{1, 1};
constexpr Point kSpawnEast{10, 1};

// ---------------------------------------------------------------------------
// 1. 跟随（契约 3.7）
// ---------------------------------------------------------------------------

TEST_F(CompanionsWorld, FieldedCompanionsWalkTheHeroTrailOneCellApart) {
    // 队伍：probe_a（张铁）、韩母（不是伙伴）、probe_b（墨大夫，不上场 active=false）、probe_c、probe_d、
    // 再往后没有了。上场的伙伴是 active 的前三位：张铁、陈师妹、白衣少女；韩母不跟、墨大夫不跟。
    party({{"zhang_tie", true}, {"han_mu", true}, {"mo_daifu", false}, {"chen_shimei", true}, {"baiyi_shaonv", true}});
    ASSERT_EQ(app_.state().position, kSpawn);

    // 还没走：全体叠在主角格上。
    EXPECT_EQ(roles(frame()), (std::vector<std::string>{"zhang_tie", "chen_shimei", "baiyi_shaonv"}));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{kSpawn, kSpawn, kSpawn}));

    // 走一步才排开：排得开的只有一格足迹，后面两位叠在那一格上。
    ASSERT_TRUE(step(0, 1));
    EXPECT_EQ(app_.state().position, (Point{1, 2}));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{1, 1}, {1, 1}, {1, 1}}));

    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(1, 0));
    EXPECT_EQ(app_.state().position, (Point{4, 2}));
    // 各隔一格，站的是主角走过的格子（拐过的那个弯也在）。
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{3, 2}, {2, 2}, {1, 2}}));
    // 面朝前面那个人：都在往右走。
    for (const auto& f : frame().followers) EXPECT_EQ(f.facing, 1);

    // 再走一步到 (5,2)；右边 (6,2) 站着张铁的 NPC（没入队时在场），撞上去不算一步：转了身，足迹不动。
    ASSERT_TRUE(step(1, 0));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{4, 2}, {3, 2}, {2, 2}}));
    party({{"chen_shimei", true}, {"baiyi_shaonv", true}});   // 换一队不含张铁的：图上的张铁在场、挡路
    EXPECT_FALSE(step(1, 0));
    EXPECT_EQ(app_.state().position, (Point{5, 2}));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{4, 2}, {3, 2}}));
}

TEST_F(CompanionsWorld, FollowersDoNotOccupyTheirCells) {
    party({{"zhang_tie", true}});
    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(-1, 0)) << "往回走一步：踩的正是跟随者站着的那一格，不占格";
    EXPECT_EQ(app_.state().position, kSpawn);
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{2, 1}}));
}

TEST_F(CompanionsWorld, TheFootprintIsRecordedOnTheStepThatFiresAnEnterTrigger) {
    party({{"zhang_tie", true}, {"chen_shimei", true}});
    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(1, 0));
    app_.state().setFlag("probe.noop", 0);
    ASSERT_TRUE(step(1, 0));   // (4,1) 是 enter 触发器：tryStep 起了脚本就提前返回
    EXPECT_EQ(app_.state().flag("probe.noop"), 1) << "触发器真的响了";
    EXPECT_EQ(app_.state().position, (Point{4, 1}));
    EXPECT_EQ(scene().footprints().front(), (Point{3, 1})) << "踩响剧情的那一步也记了足迹";
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{3, 1}, {2, 1}}));
}

TEST_F(CompanionsWorld, PassingAPortalClearsTheTrailAndStacksFollowersOnTheHero) {
    party({{"zhang_tie", true}, {"chen_shimei", true}});
    stand({3, 6}, 1);
    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(1, 0));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{4, 6}, {3, 6}}));
    ASSERT_TRUE(step(1, 0));   // (6,6) 是传送点：换图（同一张图的东口）
    EXPECT_EQ(app_.state().position, kSpawnEast);
    EXPECT_EQ(cells(frame()), (std::vector<Point>{kSpawnEast, kSpawnEast})) << "过门之后叠在主角格上";
    ASSERT_TRUE(step(-1, 0));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{kSpawnEast, kSpawnEast})) << "走一步才排开，排的是门这一侧的格子";
}

TEST_F(CompanionsWorld, AScriptTeleportClearsTheTrail) {
    party({{"zhang_tie", true}});
    ASSERT_TRUE(step(0, 1));
    ASSERT_TRUE(step(1, 0));
    writeFile(root_->path() / "scripts" / "t" / "warp.lua", R"(teleport("probe_world", 5, 5))");
    ASSERT_TRUE(app_.startEvent("t/warp.lua").ok);
    drain();
    EXPECT_EQ(app_.state().position, (Point{5, 5}));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{5, 5}})) << "剧情传送之后叠在主角格上";
}

TEST_F(CompanionsWorld, AfterLoadingASaveFollowersStandOnTheHero) {
    party({{"zhang_tie", true}, {"chen_shimei", true}});
    ASSERT_TRUE(step(0, 1));
    ASSERT_TRUE(step(1, 0));
    ASSERT_TRUE(step(1, 0));
    saveAndReload();
    EXPECT_EQ(app_.state().position, (Point{3, 2}));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{3, 2}, {3, 2}}));
    ASSERT_TRUE(step(1, 0));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{3, 2}, {3, 2}}));
    ASSERT_TRUE(step(0, 1));
    EXPECT_EQ(cells(frame()), (std::vector<Point>{{4, 2}, {3, 2}})) << "读档之后照常排开，主角身后";
}

TEST_F(CompanionsWorld, NoCompanionNoFollowers) {
    party({{"han_mu", true}});
    ASSERT_TRUE(step(0, 1));
    EXPECT_TRUE(frame().followers.empty()) << "不是伙伴的队员不跟随（改版前的样子）";
}

// ---------------------------------------------------------------------------
// 2. 宝箱（契约 2.4、3.5）
// ---------------------------------------------------------------------------

TEST_F(CompanionsWorld, AChestOpensOnceAndStaysOpenAcrossASave) {
    stand({2, 3}, 2);   // 面朝 (2,4) 的宝箱
    ASSERT_NE(named(frame().chests, "chest_a"), nullptr);
    EXPECT_FALSE(named(frame().chests, "chest_a")->opened);
    EXPECT_FALSE(step(0, 1)) << "宝箱占格";
    stand({2, 3}, 2);

    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("material_dansha"), 2);
    EXPECT_EQ(count("material_fuzhi"), 1);
    EXPECT_EQ(lastLine(), "打开箱子，得了丹砂×2、符纸。");
    EXPECT_EQ(app_.state().worldObjects.openedChests.count("probe_world.chest_a"), 1u);
    drain();
    EXPECT_TRUE(named(frame().chests, "chest_a")->opened);

    // 再按：空的，不再给。
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "箱子是空的。");
    EXPECT_EQ(count("material_dansha"), 2);
    drain();

    saveAndReload();
    EXPECT_TRUE(named(frame().chests, "chest_a")->opened) << "存档读回后还是开着的";
    stand({2, 3}, 2);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("material_dansha"), 2);
    EXPECT_FALSE(step(0, 1)) << "开过的宝箱仍然占格";
}

TEST_F(CompanionsWorld, AFullBagLeavesTheChestClosedAndTheBagUntouched) {
    app_.state().addItem("material_dansha", 998);
    stand({2, 3}, 2);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "身上再也装不下了，先腾些地方。");
    EXPECT_EQ(count("material_dansha"), 998);
    EXPECT_EQ(count("material_fuzhi"), 0);
    EXPECT_TRUE(app_.state().worldObjects.openedChests.empty());
    drain();
    EXPECT_FALSE(named(frame().chests, "chest_a")->opened);

    ASSERT_TRUE(app_.state().removeItem("material_dansha", 1));   // 腾出一件的地方：998 - 1 + 2 = 999 放得下
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("material_dansha"), 999);
    EXPECT_EQ(app_.state().worldObjects.openedChests.count("probe_world.chest_a"), 1u);
}

// ---------------------------------------------------------------------------
// 3. 采集点（按日历冷却）
// ---------------------------------------------------------------------------

TEST_F(CompanionsWorld, AGatherPointCoolsDownByTheCalendar) {
    stand({4, 3}, 2);   // 面朝 (4,4)
    const int day = app_.state().day;
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("herb_huangjing_cao"), 1);
    EXPECT_EQ(lastLine(), "采得黄精。");
    EXPECT_EQ(app_.state().worldObjects.gatheredDay.at("probe_world.gather_a"), day);
    drain();
    EXPECT_TRUE(named(frame().gathers, "gather_a")->depleted);

    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "这里刚采过，过些日子再来。");
    EXPECT_EQ(count("herb_huangjing_cao"), 1);
    drain();

    saveAndReload();
    EXPECT_TRUE(named(frame().gathers, "gather_a")->depleted) << "存档读回后仍在冷却";
    app_.advanceDays(2);
    EXPECT_TRUE(named(frame().gathers, "gather_a")->depleted) << "respawn_days=3：过两天还没好";
    app_.advanceDays(1);
    EXPECT_FALSE(named(frame().gathers, "gather_a")->depleted) << "第三天刷新";
    stand({4, 3}, 2);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("herb_huangjing_cao"), 2);
}

TEST_F(CompanionsWorld, AFullBagLeavesTheGatherPointReady) {
    app_.state().addItem("herb_huangjing_cao", 999);
    stand({4, 3}, 2);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "身上再也装不下了，先腾些地方。");
    EXPECT_EQ(count("herb_huangjing_cao"), 999);
    EXPECT_TRUE(app_.state().worldObjects.gatheredDay.empty());
    drain();
    EXPECT_FALSE(named(frame().gathers, "gather_a")->depleted);
}

// ---------------------------------------------------------------------------
// 4. 明雷（契约 3.4）
// ---------------------------------------------------------------------------

constexpr Point kFoe{8, 4};
constexpr Point kFoeFront{7, 4};   // 它面朝左

TEST_F(CompanionsWorld, SteppingInFrontOfAFoeStartsAPlainFightAndWinningClearsIt) {
    ASSERT_NE(named(frame().foes, "foe_a"), nullptr);
    EXPECT_EQ(named(frame().foes, "foe_a")->look, "wild_wolf");
    EXPECT_EQ(named(frame().foes, "foe_a")->facing, 3);
    stand({6, 4}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    tick();
    EXPECT_EQ(app_.state().position, kFoeFront);
    EXPECT_EQ(app_.battleStart().battleId, "be03_guwai_gulang") << "foe.battle 原样递过去";
    EXPECT_FALSE(app_.battleStart().preemptive) << "被看见：不加不减";

    const int day = app_.state().day;
    finishBattle(true, "");
    EXPECT_EQ(app_.state().worldObjects.clearedFoes.at("probe_world.foe_a"), day);
    EXPECT_EQ(named(frame().foes, "foe_a"), nullptr) << "打赢就消失";
    ASSERT_TRUE(step(1, 0)) << "不在场的明雷不占格";
    EXPECT_EQ(app_.state().position, kFoe);

    saveAndReload();
    EXPECT_EQ(named(frame().foes, "foe_a"), nullptr) << "存档读回后还是清掉的";
    app_.advanceDays(1);
    EXPECT_EQ(named(frame().foes, "foe_a"), nullptr);
    app_.advanceDays(1);
    EXPECT_NE(named(frame().foes, "foe_a"), nullptr) << "respawn_days=2：第二天重新出现";
}

TEST_F(CompanionsWorld, FacingAFoeFromTheSideOrBehindIsAnAmbush) {
    stand({8, 3}, 2);   // 侧面（它面朝左，上方是它的侧面）
    ASSERT_TRUE(scene().interact(app_));
    tick();
    EXPECT_TRUE(app_.battleStart().preemptive) << "侧面偷袭占先手";
    finishBattle(false, "escaped");
    EXPECT_EQ(app_.state().position, (Point{8, 3})) << "逃跑：退回开打前站的那一格";
    EXPECT_NE(named(frame().foes, "foe_a"), nullptr) << "逃跑：它留在原地";
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty());

    stand({9, 4}, 3);   // 背后
    ASSERT_TRUE(scene().interact(app_));
    tick();
    EXPECT_TRUE(app_.battleStart().preemptive) << "背后偷袭占先手";
    finishBattle(false, "escaped");

    stand(kFoeFront, 1);   // 正面：读档、逃跑之后人恰好站在它面前
    ASSERT_TRUE(scene().interact(app_));
    tick();
    EXPECT_FALSE(app_.battleStart().preemptive) << "从正面按确认不是偷袭";
    finishBattle(false, "escaped");
}

TEST_F(CompanionsWorld, ARealWinClearsTheFoeAndPaysTheBattleReward) {
    sturdyHero();
    const int stones = count("material_lingshi");
    const int cultivation = app_.state().cultivation;
    stand({6, 4}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    const int day = app_.state().day;
    ASSERT_EQ(fight(), BattlePhase::Won);
    EXPECT_EQ(app_.state().worldObjects.clearedFoes.at("probe_world.foe_a"), day) << "打赢记档";
    EXPECT_EQ(named(frame().foes, "foe_a"), nullptr);
    // 谷外孤狼的编成奖励：灵石 1、修为 3（data/battles/be03_guwai_gulang.json）。战斗场景收场时先发奖励、再走回调。
    EXPECT_EQ(count("material_lingshi"), stones + 1);
    EXPECT_EQ(app_.state().cultivation, cultivation + 3);
    EXPECT_EQ(app_.state().position, kFoeFront) << "打赢：人站在原地";
}

TEST_F(CompanionsWorld, ARealEscapeStepsBackToWhereTheHeroStood) {
    sturdyHero();
    stand({6, 4}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    ASSERT_EQ(fight(/*flee=*/true), BattlePhase::Escaped);
    EXPECT_EQ(app_.state().position, (Point{6, 4})) << "退回开打前站的那一格";
    EXPECT_NE(named(frame().foes, "foe_a"), nullptr) << "它留在原地";
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty());
}

TEST_F(CompanionsWorld, ARealLossSendsTheHeroToTheDefaultSpawn) {
    stand({12, 2}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));   // 踏进 foe_hard 面前那一格 (13,2)
    ASSERT_EQ(fight(), BattlePhase::Lost);
    EXPECT_EQ(app_.state().mapId, "probe_world");
    EXPECT_EQ(app_.state().position, kSpawn) << "打输回本图的缺省出生点";
    EXPECT_GE(app_.state().hp, 1) << "不致命：气血至少 1";
    EXPECT_NE(named(frame().foes, "foe_hard"), nullptr) << "打输：它还在";
}

TEST_F(CompanionsWorld, WhenTheEnemyFleesTheFoeStaysAndSoDoesTheHero) {
    stand({6, 4}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    tick();
    finishBattle(false, "enemy_fled");
    EXPECT_EQ(app_.state().position, kFoeFront);
    EXPECT_NE(named(frame().foes, "foe_a"), nullptr);
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty());
}

TEST_F(CompanionsWorld, EveryFacingSeesOnlyItsFrontCell) {
    struct Case {
        const char* foe;
        Point from;
        int dx, dy;     // 迈进面前那一格
        Point sideFrom;
        int sdx, sdy;   // 迈进它的侧面：不开打
    };
    const Case cases[] = {
        {"foe_up", {2, 8}, 0, 1, {0, 10}, 1, 0},      // 面前 (2,9)；侧面 (1,10)
        {"foe_right", {7, 10}, -1, 0, {5, 8}, 0, 1},  // 面前 (6,10)；侧面 (5,9)
        {"foe_down", {8, 11}, 0, -1, {6, 9}, 1, 0},   // 面前 (8,10)；侧面 (7,9)
    };
    for (const Case& c : cases) {
        SCOPED_TRACE(c.foe);
        stand(c.sideFrom, 1);
        ASSERT_TRUE(scene().tryStep(app_, c.sdx, c.sdy));
        EXPECT_FALSE(battling()) << "侧面不在它眼里";
        stand(c.from, 1);
        ASSERT_TRUE(scene().tryStep(app_, c.dx, c.dy));
        ASSERT_TRUE(battling()) << "踏进面前那一格就开打";
        EXPECT_FALSE(app_.battleStart().preemptive);
        finishBattle(false, "escaped");
        EXPECT_EQ(app_.state().position, c.from);
    }
}

TEST_F(CompanionsWorld, BeingSeenComesBeforeAnEnterTriggerOnTheSameCell) {
    app_.state().setFlag("probe.noop", 0);
    stand({9, 10}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));   // (10,10)：foe_trap 的面前，也压着 trigger_trap
    ASSERT_TRUE(battling());
    EXPECT_EQ(app_.state().flag("probe.noop"), 0) << "这一步先被看见，踏入触发器没响";
    finishBattle(false, "escaped");
}

TEST_F(CompanionsWorld, ObjectsWithAnUnsetRequireFlagAreNotThere) {
    auto present = [&] {
        const WorldFrame& f = frame();
        return std::make_tuple(named(f.chests, "chest_r") != nullptr, named(f.gathers, "gather_r") != nullptr,
                               named(f.foes, "foe_r") != nullptr);
    };
    EXPECT_EQ(present(), std::make_tuple(false, false, false));
    stand({12, 9}, 1);
    EXPECT_TRUE(step(1, 0)) << "不在的宝箱不占格";
    EXPECT_TRUE(step(1, 0)) << "不在的采集点不占格";
    stand({13, 8}, 2);
    EXPECT_FALSE(scene().interact(app_)) << "按了不响";
    stand({15, 10}, 2);
    EXPECT_TRUE(step(0, 1)) << "不在的明雷不占格";
    stand({14, 10}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    EXPECT_FALSE(battling()) << "不在的明雷看不见人";

    app_.state().setFlag("probe.show");
    EXPECT_EQ(present(), std::make_tuple(true, true, true));
    stand({12, 9}, 1);
    EXPECT_FALSE(step(1, 0)) << "置了旗标：宝箱占格";
    stand({13, 8}, 2);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(count("material_fuzhi"), 1);
    drain();
    stand({14, 10}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    EXPECT_TRUE(battling()) << "置了旗标：明雷看得见人";
    finishBattle(false, "escaped");
}

TEST_F(CompanionsWorld, ALosingTripInsideALilianEndsTheWholeTrip) {
    ASSERT_TRUE(app_.loadMap("ch01_hanjiacun", std::string{}).ok);
    const Point before = app_.state().position;
    ASSERT_TRUE(app_.enterLilian(fanren::game::LilianEntry{"probe_site", "probe_world", "", {"li_feiyu"}}).ok);
    tick();
    stand({12, 2}, 1);
    ASSERT_TRUE(scene().tryStep(app_, 1, 0));
    ASSERT_EQ(fight(), BattlePhase::Lost) << "真打：探针强敌，这一队打不过";
    EXPECT_FALSE(app_.inLilian()) << "历练里打输：整趟退出";
    EXPECT_EQ(app_.state().mapId, "ch01_hanjiacun");
    EXPECT_EQ(app_.state().position, before);
}

TEST_F(CompanionsWorld, AFoeClearedInsideALilianIsOnlyClearedForThatTrip) {
    ASSERT_TRUE(app_.enterLilian(fanren::game::LilianEntry{"probe_site", "probe_world", "", {"li_feiyu"}}).ok);
    tick();
    stand({8, 3}, 2);
    ASSERT_TRUE(scene().interact(app_));
    tick();
    finishBattle(true, "");
    EXPECT_EQ(named(frame().foes, "foe_a"), nullptr);
    EXPECT_TRUE(app_.state().worldObjects.clearedFoes.empty()) << "永久集合没被写";
    EXPECT_EQ(app_.worldObjects().clearedFoes.count("probe_world.foe_a"), 1u);
}

// ---------------------------------------------------------------------------
// 5. 机关（契约 3.6）
// ---------------------------------------------------------------------------

TEST_F(CompanionsWorld, AGateOpensForACompanionWithTheSkillAndStaysOpen) {
    stand({9, 4}, 1);   // 面朝 (10,4) 的残禁
    // 名册里还没人会：只说要别的法子。
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "似乎要用别的法子。");
    drain();
    // 名册里有了、队里没有：点她的名——用她此刻的角色名（张铁）。
    app_.state().setFlag("probe.met_a");
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "这里要张铁来才行。");
    drain();
    app_.state().setFlag("probe.grown");
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "这里要厉飞雨来才行。") << "换了角色就叫新名字";
    drain();
    EXPECT_FALSE(named(frame().gates, "gate_a")->open);
    EXPECT_FALSE(step(1, 0)) << "关着的机关占格";

    // 她入队（不上场也算：场地技不是战斗）。
    party({{"li_feiyu", false}});
    stand({9, 4}, 1);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "厉飞雨上前施为，前路开了。");
    EXPECT_EQ(app_.state().worldObjects.openedGates.count("probe_world.gate_a"), 1u);
    EXPECT_EQ(app_.state().flag("probe.gate_a_open"), 1) << "写了 opens 的置那面旗标";
    drain();
    EXPECT_TRUE(named(frame().gates, "gate_a")->open);
    EXPECT_FALSE(scene().interact(app_)) << "开了的机关按了不响";
    ASSERT_TRUE(step(1, 0)) << "开了之后不占格";
    EXPECT_EQ(app_.state().position, (Point{10, 4}));

    party({});
    saveAndReload();
    EXPECT_TRUE(named(frame().gates, "gate_a")->open) << "存档读回后还开着，人走了也开着";
}

TEST_F(CompanionsWorld, AGateWithoutOpensSetsNoFlagAndTheHintReplacesTheGenericLine) {
    party({{"mo_daifu", true}});
    stand({10, 5}, 2);   // 面朝 (10,6) 的幻阵：墨大夫会 illusion
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "墨大夫上前施为，前路开了。");
    EXPECT_EQ(app_.state().worldObjects.openedGates.count("probe_world.gate_b"), 1u);
    drain();

    stand({11, 5}, 2);   // 面朝 (11,6)：谁都不会，写了 hint
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "这里眼下还用不上。");
    EXPECT_EQ(app_.state().worldObjects.openedGates.count("probe_world.gate_c"), 0u);
}

TEST_F(CompanionsWorld, AWideGateBlocksAndOpensAsOne) {
    party({{"mo_daifu", true}});
    for (const Point at : {Point{12, 5}, Point{13, 5}}) {
        stand(at, 2);
        EXPECT_FALSE(step(0, 1)) << "两格都挡";
    }
    stand({13, 5}, 2);   // 面朝右半边按
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(app_.state().worldObjects.openedGates.count("probe_world.gate_wide"), 1u);
    drain();
    for (const Point at : {Point{12, 5}, Point{13, 5}}) {
        stand(at, 2);
        EXPECT_TRUE(step(0, 1)) << "开了之后两格都不挡";
    }
}

TEST_F(CompanionsWorld, TheOpeningLineNamesTheRoleActuallyInTheParty) {
    // 队里存的是张铁（脚本 party.add 定的），probe.grown 也置了——currentRole 会推成厉飞雨。
    // 旁白里的名字要是图上跟着的那个人：张铁。
    app_.state().setFlag("probe.grown");
    party({{"zhang_tie", true}});
    stand({9, 4}, 1);
    ASSERT_TRUE(scene().interact(app_));
    EXPECT_EQ(lastLine(), "张铁上前施为，前路开了。");
}

TEST_F(CompanionsWorld, AGateOpenedInsideALilianSetsNoFlagAndIsClosedNextTrip) {
    const fanren::game::LilianEntry trip{"probe_site", "probe_world", "", {"li_feiyu"}};
    ASSERT_TRUE(app_.enterLilian(trip).ok);
    tick();
    stand({9, 4}, 1);
    ASSERT_TRUE(scene().interact(app_));
    drain();
    EXPECT_TRUE(named(frame().gates, "gate_a")->open);
    EXPECT_EQ(app_.state().flag("probe.gate_a_open"), 0) << "历练里不置永久旗标";
    EXPECT_TRUE(app_.state().worldObjects.openedGates.empty());
    ASSERT_TRUE(app_.leaveLilian().ok);
    ASSERT_TRUE(app_.enterLilian(trip).ok);
    tick();
    EXPECT_FALSE(named(frame().gates, "gate_a")->open) << "第二趟进去是关着的";
}

// ---------------------------------------------------------------------------
// 6. 插话的 require_party（契约 3.8）与一人一身（3.11）
// ---------------------------------------------------------------------------

TEST_F(CompanionsWorld, AnInterjectionFiresOnlyWithThatCompanionInTheParty) {
    auto stepOnChat = [&] {
        app_.state().setFlag("probe.chatted", 0);
        stand({1, 5}, 2);
        EXPECT_TRUE(step(0, 1));
        EXPECT_EQ(app_.state().position, (Point{1, 6}));
        return app_.state().flag("probe.chatted") != 0;
    };
    party({});
    EXPECT_FALSE(stepOnChat()) << "她不在队里：不触发";
    party({{"han_mu", true}});
    EXPECT_FALSE(stepOnChat()) << "别人在队里也不触发";
    party({{"zhang_tie", true}});
    EXPECT_TRUE(stepOnChat()) << "probe_a 用张铁这个角色在队里";
    party({{"li_feiyu", false}});
    EXPECT_TRUE(stepOnChat()) << "换成她后来的角色 id、没上场，也算在队里";
}

TEST_F(CompanionsWorld, ACompanionInThePartyIsNotAlsoStandingOnTheMap) {
    const Point npcCell{6, 2};
    auto npcShown = [&](const std::string& role) {
        for (const auto& npc : frame().npcs) {
            if (npc.roleId == role) return true;
        }
        return false;
    };
    party({});
    EXPECT_TRUE(npcShown("zhang_tie"));
    stand({6, 1}, 2);
    EXPECT_FALSE(step(0, 1)) << "她不在队里时图上的她占格";

    party({{"li_feiyu", true}});   // 同一位伙伴的另一个角色 id
    EXPECT_FALSE(npcShown("zhang_tie")) << "图上没有第二个她";
    EXPECT_EQ(WorldScene::visibleNpcAt(app_.state(), *app_.currentMap(), npcCell, app_.data().companions), nullptr);
    stand({6, 1}, 2);
    EXPECT_FALSE(scene().interact(app_)) << "不可交互";
    EXPECT_TRUE(step(0, 1)) << "不占格";
    EXPECT_EQ(app_.state().position, npcCell);

    // 不是伙伴的队员不受影响：韩母在队里，图上的韩母照旧在。
    party({{"han_mu", true}});
    EXPECT_TRUE(npcShown("han_mu"));
}

}  // namespace
