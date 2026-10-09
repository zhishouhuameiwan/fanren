// 第 7 章验收（docs/ch07-design.md 第 15 节）的第 5、7、8、9、12、13、14、16、17、18 条（文案那一半），外加几处
// 「判据依赖存档状态」的分支——扫脚本、读数据、切片跑单个脚本。**不经过驱动、判据写死成施工图原文**。
//
// ---------------------------------------------------------------------------
// 为什么是这个形状（handoff 第 6 节第 5 条、handoff-2026-09-23-ch04 第 8 节第一条）
// ---------------------------------------------------------------------------
// 通关测试（tests/Ch07AcceptanceTests.cpp）问「走不走得完」；它驱动脚本，脚本一变它就跟着变，所以它只发现得了「变了」，
// 发现不了「错了」。这里每一条都是另一个形状：读 maps/、scripts/、data/ 的字，拿施工图原文当判据比；要看「引擎里真的是
// 那样」的几条，起单个脚本（切片），起点是手摆的最小局面、旁边写明摆了什么。
// 「判据依赖存档状态的，把玩家真能处在的状态都走一遍」（handoff 第 6 节第 8 条）：吴风开口按第 6 章切磋的输赢分、
// 卖符少女的欠账按 ch06.qianyao 分、拆包试液按瓶里有没有液分、插刃中途收手——两个交接存档只走得到其中一面的，这里补另一面。
//
// 施工偏差（第 18 节，以它为准）：18.5（插刃是一个选项按八次、取消 = 收手）、18.14（欠账那两句在 19b）。
#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "core/rules/Quests.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/SaveFile.h"
#include "script/Command.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::game::BattleMenuMode;
using fanren::core::battle::Unit;
using fanren::game::AlchemyScene;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::rules::CraftKind;
using fanren::rules::Realm;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch07" / "zhuji.lua") && fs::exists(root / "data" / "objectives" / "ch07.json")) {
            return candidate;
        }
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
    }
    return out;
}

std::string codeOf(const std::string& line) {
    const std::size_t dash = line.find("--");
    return dash == std::string::npos ? line : line.substr(0, dash);
}

std::string codeOnly(const std::string& text) {
    std::string out;
    for (const std::string& line : linesOf(text)) out += codeOf(line) + "\n";
    return out;
}

int countOf(const std::string& haystack, const std::string& needle) {
    int n = 0;
    for (std::size_t at = haystack.find(needle); at != std::string::npos; at = haystack.find(needle, at + 1)) ++n;
    return n;
}

std::map<std::string, std::string> scriptsUnder(const std::string& root, const std::string& sub) {
    std::map<std::string, std::string> out;
    const fs::path base = fs::path(root) / "scripts";
    const fs::path dir = base / sub;
    if (!fs::exists(dir)) return out;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        out[fs::relative(entry.path(), base).generic_string()] = readFile(entry.path());
    }
    return out;
}

int indexOfLabel(const fanren::ui::ListView& list, const std::string& label) {
    for (int i = 0; i < list.count(); ++i) {
        if (list.items()[static_cast<std::size_t>(i)].label == label) return i;
    }
    return -1;
}

class Ch07Slice : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        chapterScripts_ = scriptsUnder(root_, "ch07");
        allScripts_ = scriptsUnder(root_, "");
        ASSERT_GE(chapterScripts_.size(), 46u) << "先验：scripts/ch07/ 读得到（分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }
    std::string text(const std::string& key) { return app_.data().lookupText(key); }

    MapObject objectOn(const std::string& mapId, const std::string& name) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok || app_.currentMap() == nullptr) return MapObject{};
        for (const MapObject& object : app_.currentMap()->objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    // 切片：起一个脚本，对白一路按确认；选择按 answers 的次序答（答完了取 fallback）。返回说过的文案 key。
    std::vector<std::string> runScript(const std::string& path, std::deque<int> answers = {}, int fallback = 0) {
        std::vector<std::string> said;
        const auto started = app_.startEvent(path);
        EXPECT_TRUE(started.ok) << path << "：" << started.error;
        if (!started.ok) return said;
        app_.clearSpokenKeys();
        for (int frame = 0; frame < 8000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (auto* fight = dynamic_cast<BattleScene*>(app_.topScene()); fight != nullptr) {
                if (fight->battle().phase() == fanren::core::battle::BattlePhase::Ongoing) {
                    fanren::test::BattleHand hand(app_);
                    hand.fleeAtOnce(fleeAtOnce_);
                    hand.play(*fight);
                }
                continue;
            }
            if (!app_.awaitingCommand()) continue;
            const bool talked = !app_.spokenKeys().empty();
            said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
            app_.clearSpokenKeys();
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            if (!talked) {
                result.choiceIndex = fallback;
                if (!answers.empty()) {
                    result.choiceIndex = answers.front();
                    answers.pop_front();
                }
                ++choices_;
            }
            app_.completeCommand(result);
            app_.popScene();
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
        app_.tick(1.0 / 60.0);
        said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
        app_.clearSpokenKeys();
        return said;
    }

    static bool has(const std::vector<std::string>& said, const std::string& key) {
        return std::find(said.begin(), said.end(), key) != said.end();
    }

    void freshEleventh() {
        state() = GameState{};
        state().setFlag("story.xiuxian_known");
        state().realm = state().realmCap = Realm::QiRefining11;
        state().hp = state().maxHp = fanren::rules::realmMaxHp(Realm::QiRefining11);
        state().mp = state().maxMp = fanren::rules::realmMaxMp(Realm::QiRefining11);
    }

    std::vector<std::string> listed(CraftKind kind) {
        std::vector<std::string> ids;
        for (const auto* recipe : AlchemyScene::visibleRecipes(app_.recipes(), kind, state())) ids.push_back(recipe->id);
        return ids;
    }

    Application app_;
    std::string root_;
    std::map<std::string, std::string> chapterScripts_;
    std::map<std::string, std::string> allScripts_;
    int choices_ = 0;
    bool fleeAtOnce_ = false;
};

// ===========================================================================
// 验收 5：境界——realm.advance 恰 4 处（yaolou 一处目标 11；zhuji 三处目标 12、13、21，次序也钉）；realm.cap 0 处
// ===========================================================================
TEST_F(Ch07Slice, No5_FourRealmStepsElevenThenTwelveThirteenFoundationAndNoCap) {
    static const std::regex kAdvance("realm\\.advance\\(([^)]*)\\)");
    std::map<std::string, std::vector<std::string>> advances;
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kAdvance); it != std::sregex_iterator(); ++it) {
            advances[path].push_back((*it)[1]);
        }
        EXPECT_EQ(countOf(code, "realm.cap("), 0) << path << " 写了 realm.cap（验收 5：realm.cap 0 处）";
    }
    EXPECT_EQ(advances, (std::map<std::string, std::vector<std::string>>{{"ch07/yaolou.lua", {"11"}},
                                                                         {"ch07/zhuji.lua", {"12", "13", "21"}}}))
        << "验收 5：yaolou 一处目标 11；zhuji 三处，次序 12 → 13 → 21";
    // 1.3：数值读表、不抄数——这里只钉施工图写死的那两行（十一层 156 / 110；筑基初期 ≥ 260 / 180，验收 2 的口径）。
    EXPECT_EQ(fanren::rules::realmMaxHp(Realm::QiRefining11), 156) << "1.3：十一层气血 24 + 12 × 11 = 156";
    EXPECT_EQ(fanren::rules::realmMaxMp(Realm::QiRefining11), 110) << "1.3：十一层法力 110";
    // 扫描器自检：同一个扫法对第 6 章那一处升境是看得见的。
    int chapterSix = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch06")) chapterSix += countOf(codeOnly(source), "realm.advance(");
    EXPECT_EQ(chapterSix, 1) << "扫描器自检：第 6 章有一处 realm.advance";

    // 切片：十一层、瓶子 3 / 3 滴、筑基丹 25 颗起步，服丹之后筑基初期 / 上限筑基初期、气血法力按表抬、瓶子 6 滴而滴数不动（E3）、
    // 日子 +150（3.3）、扣 8 颗、末尾 teleport 百药园、置 ch07.done、不 ending()。
    freshEleventh();
    state().setFlag("ch07.chengdan");
    state().addItem("pill_zhuji_dan", 25, 0);
    state().bottle.owned = true;
    state().bottle.matureKnown = true;
    state().bottle.capacity = 3;
    state().bottle.drops = 2;
    const int day = state().day;
    const auto said = runScript("ch07/zhuji.lua");
    EXPECT_EQ(state().realm, Realm::FoundationEarly) << "验收 5：终点筑基初期";
    EXPECT_EQ(state().realmCap, Realm::FoundationEarly) << "1.3：章末上限筑基初期";
    EXPECT_GE(state().maxHp, 260) << "验收 2：maxHp ≥ 260";
    EXPECT_GE(state().maxMp, 180) << "验收 2：maxMp ≥ 180";
    EXPECT_EQ(state().maxHp, fanren::rules::realmMaxHp(Realm::FoundationEarly)) << "1.3：读表";
    EXPECT_EQ(state().bottle.capacity, 6) << "E3：筑基之后瓶子 6 滴（「抬上限不送液」那一半在 Ch07EngineTests / ScriptApiTests："
                                           "这一段脚本还拨了 150 天，瓶子自己凝满了）";
    EXPECT_EQ(state().day - day, 150) << "3.3：服丹五个月";
    EXPECT_EQ(state().itemCount("pill_zhuji_dan"), 17) << "第 9 节第 2 条：扣 8，剩 17";
    EXPECT_EQ(state().flag("ch07.done"), 1);
    EXPECT_EQ(state().mapId, "ch06_baiyaoyuan") << "3.1 节点 36：末尾 teleport 百药园";
    EXPECT_EQ(countOf(codeOnly(chapterScripts_["ch07/zhuji.lua"]), "ending("), 0) << "3.2 节点 36：不 ending()";
    for (const char* key : {"ch07.zhuji.twelve", "ch07.zhuji.thirteen", "ch07.zhuji.foundation"}) {
        EXPECT_TRUE(has(said, key)) << "每一处 realm.advance 都看返回值，升成了才说那一句：" << key;
    }
}

// ===========================================================================
// 验收 7：借来的威能用完就收——剑符、金光砖各一对 take ＋ forget 同在一个脚本；magic.learn 恰 4 处
// ===========================================================================
TEST_F(Ch07Slice, No7_TheBorrowedPowersAreTakenBackWhereTheyBurnOut) {
    struct Pair {
        const char* take;
        const char* forget;
        const char* where;
    };
    for (const Pair& p : std::vector<Pair>{{"take(\"talisman_jianfu\"", "magic.forget(\"magic_ji_jianfu\")", "ch07/yeyu.lua"},
                                           {"take(\"talisman_jinguangzhuan\"", "magic.forget(\"magic_ji_jinguangzhuan\")",
                                            "ch07/jiaoshi.lua"}}) {
        int takes = 0;
        int forgets = 0;
        std::set<std::string> where;
        for (const auto& [path, source] : chapterScripts_) {
            const std::string code = codeOnly(source);
            const int t = countOf(code, p.take);
            const int f = countOf(code, p.forget);
            takes += t;
            forgets += f;
            if (t + f > 0) where.insert(path);
        }
        EXPECT_EQ(takes, 1) << "验收 7：" << p.take << " 恰 1 处";
        EXPECT_EQ(forgets, 1) << "验收 7：" << p.forget << " 恰 1 处";
        EXPECT_EQ(where, std::set<std::string>{p.where}) << "验收 7：同在 " << p.where << "（本章脚本里）";
    }
    std::map<std::string, std::set<std::string>> learns;
    static const std::regex kLearn("magic\\.learn\\(\"([a-z0-9_]+)\"\\)");
    int total = 0;
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kLearn); it != std::sregex_iterator(); ++it) {
            learns[path].insert((*it)[1]);
            ++total;
        }
    }
    EXPECT_EQ(total, 4) << "验收 7：magic.learn 恰 4 处";
    EXPECT_EQ(learns, (std::map<std::string, std::set<std::string>>{
                          {"ch07/wanbaolou.lua", {"magic_ji_jinfu", "magic_ji_jinguangzhuan"}},
                          {"ch07/yeyu.lua", {"magic_ji_qingjiao"}},
                          {"ch07/shulin.lua", {"magic_ji_qingning"}}}))
        << "验收 7：金蚨、金光砖在 wanbaolou.lua，青蛟在 yeyu.lua，青凝在 shulin.lua，另无";
    // 扫描器自检：第 5 章借一仗的那一对是看得见的。
    int chapterFive = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch05")) chapterFive += countOf(codeOnly(source), "magic.forget(\"magic_ji_jianfu\")");
    EXPECT_EQ(chapterFive, 1) << "扫描器自检：第 5 章那一处 magic.forget 该数得出来";

    // 切片：节点 31 演完，金光砖符宝与祭金光砖都没了（ch209：打穿地面时耗尽）。
    freshEleventh();
    state().setFlag("ch07.mojiao");
    state().addItem("talisman_jinguangzhuan", 1, 0);
    state().learnMagic("magic_ji_jinguangzhuan");
    runScript("ch07/jiaoshi.lua");
    EXPECT_EQ(state().itemCount("talisman_jinguangzhuan"), 0) << "验收 7：金光砖耗尽成了废纸";
    EXPECT_FALSE(state().knowsMagic("magic_ji_jinguangzhuan")) << "验收 7：祭金光砖当场 forget";
    EXPECT_EQ(state().flag("ch07.nangong"), 1);
}

// ===========================================================================
// 验收 8：配方——两张方子的门闸、输入、难度照第 7 节；结丹灵药方与四张制符方挂占位旗标；
// 从第 6 章终局读档，炼丹面板没有那三张、制符面板没有那四张；置 ch07.cangshi 之后炼丹面板多出两张
// ===========================================================================
TEST_F(Ch07Slice, No8_TheRecipesAreGatedTheWaySectionSevenSays) {
    const auto& recipes = app_.recipes();
    const auto zhuji = recipes.find("recipe_zhuji_dan");
    const auto dingyan = recipes.find("recipe_dingyan_dan");
    ASSERT_NE(zhuji, recipes.end());
    ASSERT_NE(dingyan, recipes.end());
    EXPECT_EQ(zhuji->second.requireFlag, "ch07.cangshi") << "第 7 节 / 验收 8";
    EXPECT_EQ(zhuji->second.difficulty, 87) << "第 7 节：难度 87";
    EXPECT_EQ(zhuji->second.requiredProficiency, 30) << "第 7 节：熟练度门槛 30";
    ASSERT_EQ(zhuji->second.inputs.size(), 1u) << "第 7 节：一炉份药粉 ×1";
    EXPECT_EQ(zhuji->second.inputs[0].itemId, "material_zhuji_yaofen");
    EXPECT_EQ(zhuji->second.inputs[0].count, 1);
    EXPECT_EQ(zhuji->second.productId, "pill_zhuji_dan");
    EXPECT_EQ(zhuji->second.kind, CraftKind::Alchemy);
    EXPECT_EQ(dingyan->second.requireFlag, "ch07.cangshi") << "第 7 节 / 验收 8";
    EXPECT_EQ(dingyan->second.difficulty, 60) << "第 7 节：难度 60";
    EXPECT_EQ(dingyan->second.requiredProficiency, 30) << "第 7 节：熟练度门槛 30";
    ASSERT_EQ(dingyan->second.inputs.size(), 1u);
    EXPECT_EQ(dingyan->second.inputs[0].itemId, "herb_huangjing_zhi") << "第 7 节：黄精芝 ×2、minAge 1000";
    EXPECT_EQ(dingyan->second.inputs[0].count, 2);
    EXPECT_EQ(dingyan->second.inputs[0].minAge, 1000);
    for (const char* id : {"recipe_jiedan_lingyao", "recipe_huoqiu_fu", "recipe_hushen_fu", "recipe_jinci_fu", "recipe_leiming_fu"}) {
        const auto it = recipes.find(id);
        ASSERT_NE(it, recipes.end()) << id;
        EXPECT_EQ(it->second.requireFlag, "story.recipe_later") << id << "：第 7 节「堵泄漏」";
    }
    const auto dingshen = recipes.find("recipe_dingshen_fu");
    ASSERT_NE(dingshen, recipes.end());
    EXPECT_TRUE(dingshen->second.requireFlag.empty()) << "契约 1.2：定神符方不加门闸";

    for (const char* file : {fanren::test::kChapterSixEndingFirst, fanren::test::kChapterSixEndingSecond}) {
        auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << file << "：" << loaded.error;
        state() = loaded.value;
        const auto alchemy = listed(CraftKind::Alchemy);
        const auto talisman = listed(CraftKind::Talisman);
        for (const char* id : {"recipe_zhuji_dan", "recipe_dingyan_dan", "recipe_jiedan_lingyao"}) {
            EXPECT_EQ(std::count(alchemy.begin(), alchemy.end(), id), 0) << file << "：炼丹面板上不该有 " << id;
        }
        for (const char* id : {"recipe_huoqiu_fu", "recipe_hushen_fu", "recipe_jinci_fu", "recipe_leiming_fu"}) {
            EXPECT_EQ(std::count(talisman.begin(), talisman.end(), id), 0) << file << "：制符面板上不该有 " << id;
        }
        EXPECT_EQ(std::count(talisman.begin(), talisman.end(), "recipe_dingshen_fu"), 1) << file << "：定神符方照旧在";
        state().setFlag("ch07.cangshi");
        const auto after = listed(CraftKind::Alchemy);
        EXPECT_EQ(after.size(), alchemy.size() + 2) << file << "：置 ch07.cangshi 之后炼丹面板多出两张";
        for (const char* id : {"recipe_zhuji_dan", "recipe_dingyan_dan"}) {
            EXPECT_EQ(std::count(after.begin(), after.end(), id), 1) << file << "：" << id;
        }
        EXPECT_EQ(std::count(after.begin(), after.end(), "recipe_jiedan_lingyao"), 0) << file << "：结丹灵药方照旧藏着";
    }
}

// ===========================================================================
// 验收 9：道具施法——四件符的 castMagic 与 E5 一致；**本章的真仗里**用得出来（契约 2.6 用的是第 4 章的编成）：
// 天雷子在 ③ 封岳身上——背包少一件、法力不变、伤得了他；土牢符在 ② 络腮胡子（架势 3）身上——当场破势。
// ===========================================================================
TEST_F(Ch07Slice, No9_TheTalismansCastWhatE5SaysInsideTheChaptersOwnFights) {
    const std::vector<std::pair<const char*, const char*>> kCasts = {{"talisman_tianleizi", "magic_tianleizi"},
                                                                     {"talisman_tulao_fu", "magic_tulao_shu"},
                                                                     {"talisman_huoqiu_fu", "magic_huoqiu_shu"},
                                                                     {"talisman_dingshen_fu", "magic_dingshen_shu"}};
    for (const auto& [item, magic] : kCasts) {
        const fanren::core::Item* found = app_.data().findItem(item);
        ASSERT_NE(found, nullptr) << item;
        EXPECT_EQ(found->castMagic, magic) << item << "：验收 9 / E5";
    }
    const fanren::core::Magic* tulao = app_.data().findMagic("magic_tulao_shu");
    ASSERT_NE(tulao, nullptr);
    EXPECT_EQ(tulao->effect, fanren::core::MagicEffect::Stagger);
    EXPECT_EQ(tulao->stagger, 3) << "E5：土牢术 stagger 3";
    const fanren::core::Magic* dingshen = app_.data().findMagic("magic_dingshen_shu");
    ASSERT_NE(dingshen, nullptr);
    EXPECT_EQ(dingshen->stagger, 1) << "E5：定神术 stagger 1";
    const fanren::core::Magic* lei = app_.data().findMagic("magic_tianleizi");
    ASSERT_NE(lei, nullptr);
    EXPECT_EQ(lei->element, fanren::core::kElementFire) << "E5：天雷子火";

    // 真菜单：物品 → 那一行 → 择敌 → 那个人。
    const auto useFromMenu = [&](BattleScene& scene, const std::string& itemName, const std::string& roleId) {
        const int actor = scene.runToAllyTurn();
        ASSERT_GE(actor, 0);
        scene.openMenu(app_);
        ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuItem));
        const int row = indexOfLabel(scene.menuList(), itemName);
        ASSERT_GE(row, 0) << "物品列表里没有「" << itemName << "」";
        ASSERT_TRUE(scene.menuList().items()[static_cast<std::size_t>(row)].enabled)
            << scene.menuList().items()[static_cast<std::size_t>(row)].disabledReason;
        ASSERT_TRUE(scene.menuChoose(app_, row));
        ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target) << "伤人的与削架势的都要择敌";
        const fanren::core::RoleTemplate* role = app_.data().findRole(roleId);
        ASSERT_NE(role, nullptr);
        const int who = indexOfLabel(scene.menuList(), role->name);
        ASSERT_GE(who, 0) << "择敌列表里没有「" << role->name << "」";
        ASSERT_TRUE(scene.menuChoose(app_, who)) << scene.feedback();
    };
    const auto unitOf = [](const BattleScene& scene, const std::string& roleId) -> const Unit* {
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally && u.id == roleId) return &u;
        }
        return nullptr;
    };

    // ③ 封岳：天雷子。
    freshEleventh();
    state().addItem("talisman_tianleizi", 1, 0);
    {
        BattleScene scene("b07_fengyue");
        scene.onEnter(app_);
        ASSERT_TRUE(scene.battle().hasItem("talisman_tianleizi")) << "本章的仗里天雷子登记得上";
        const int mpBefore = scene.battle().units()[0].mp;
        const Unit* fengyue = unitOf(scene, "feng_yue");
        ASSERT_NE(fengyue, nullptr);
        const int hpBefore = fengyue->hp;
        useFromMenu(scene, app_.data().findItem("talisman_tianleizi")->name, "feng_yue");
        ASSERT_FALSE(HasFatalFailure());
        EXPECT_EQ(state().itemCount("talisman_tianleizi"), 0) << "验收 9：用掉一件，背包少一件";
        EXPECT_EQ(scene.battle().units()[0].mp, mpBefore) << "验收 9：法力不变";
        fengyue = unitOf(scene, "feng_yue");
        ASSERT_NE(fengyue, nullptr);
        EXPECT_LT(fengyue->hp, hpBefore) << "天雷子伤得了封岳（8.2 ③ 的原著解法）";
    }
    // ② 络腮胡子：土牢符，架势 3 → 破势。
    freshEleventh();
    state().addItem("talisman_tulao_fu", 2, 0);
    {
        BattleScene scene("b07_yixiantian");
        scene.onEnter(app_);
        const int mpBefore = scene.battle().units()[0].mp;
        const Unit* huzi = unitOf(scene, "luosai_huzi");
        ASSERT_NE(huzi, nullptr);
        ASSERT_EQ(huzi->toughness, 3) << "先验：胡子架势 3（8.2 ②）";
        useFromMenu(scene, app_.data().findItem("talisman_tulao_fu")->name, "luosai_huzi");
        ASSERT_FALSE(HasFatalFailure());
        huzi = unitOf(scene, "luosai_huzi");
        ASSERT_NE(huzi, nullptr);
        EXPECT_TRUE(huzi->broken()) << "验收 9：土牢符把架势 3 的敌人打到破势";
        EXPECT_EQ(state().itemCount("talisman_tulao_fu"), 1);
        EXPECT_EQ(scene.battle().units()[0].mp, mpBefore) << "验收 9：法力不变";
    }
}

// ===========================================================================
// 验收 12：药园——field.unlock("field_baiyaoyuan_jiao", 2) 恰 1 处（chaibao.lua）；药篓的年份字面量 44、100、176；
// 园角 1000；万宝楼 1000。切片：拆包给的苗全是零年、园角两畦开出来；瓶里有液试一滴、没液跳过那一句（不设死局）
// ===========================================================================
TEST_F(Ch07Slice, No12_TheGardenLiteralsAreTheOnesOfSectionThreeTwo) {
    int unlocks = 0;
    std::string unlockedIn;
    for (const auto& [path, source] : chapterScripts_) {
        const int n = countOf(codeOnly(source), "field.unlock(");
        unlocks += n;
        if (n > 0) unlockedIn = path;
    }
    EXPECT_EQ(unlocks, 1) << "验收 12：本章 field.unlock 恰 1 处";
    EXPECT_EQ(unlockedIn, "ch07/chaibao.lua");
    EXPECT_NE(codeOnly(chapterScripts_["ch07/chaibao.lua"]).find("field.unlock(\"field_baiyaoyuan_jiao\", 2)"), std::string::npos)
        << "验收 12：field.unlock(\"field_baiyaoyuan_jiao\", 2)";
    const auto agedLiterals = [&](const std::string& path, const char* call) {
        std::multiset<std::string> out;
        const std::regex pattern(std::string(call) + "\\(\"([a-z0-9_]+)\",\\s*(?:[^,)]+,\\s*)?([0-9]+)\\)");
        const std::string code = codeOnly(chapterScripts_[path]);
        for (auto it = std::sregex_iterator(code.begin(), code.end(), pattern); it != std::sregex_iterator(); ++it) {
            out.insert(std::string((*it)[1]) + "@" + std::string((*it)[2]));
        }
        return out;
    };
    EXPECT_EQ(agedLiterals("ch07/yaolou.lua", "take_aged"),
              (std::multiset<std::string>{"herb_huangjing_cao@44", "herb_zishen_cao@100", "herb_xuehong_zhi@176"}))
        << "验收 12：药篓的 take_aged 年份字面量是 44、100、176";
    EXPECT_EQ(agedLiterals("ch07/yaolou.lua", "item\\.count_aged"),
              (std::multiset<std::string>{"herb_huangjing_cao@44", "herb_zishen_cao@100", "herb_xuehong_zhi@176"}))
        << "药篓先数够年份的，同一组字面量";
    EXPECT_EQ(agedLiterals("ch07/yuanjiao.lua", "item\\.count_aged"), (std::multiset<std::string>{"herb_huangjing_zhi@1000"}))
        << "验收 12：园角是 1000";
    EXPECT_EQ(agedLiterals("ch07/wanbaolou.lua", "take_aged"), (std::multiset<std::string>{"herb_huangjing_zhi@1000"}))
        << "验收 12：万宝楼是 1000";
    EXPECT_EQ(countOf(codeOnly(chapterScripts_["ch07/yuanjiao.lua"]), "take"), 0) << "3.2 节点 9：药不扣";
    // 扫描器自检：同一个正则认得出按年份扣的那一种写法（契约 4.2）。
    {
        const std::string probe = "take_aged(\"herb_x\", 3, 44)\n";
        std::smatch m;
        EXPECT_TRUE(std::regex_search(probe, m, std::regex("take_aged\\(\"([a-z0-9_]+)\",\\s*(?:[^,)]+,\\s*)?([0-9]+)\\)")));
    }

    // 切片：拆包（3.2 节点 1）。
    for (const int drops : {3, 0}) {
        freshEleventh();
        state().realm = state().realmCap = Realm::QiRefining9;
        state().setFlag("ch06.done");
        state().bottle.owned = true;
        state().bottle.matureKnown = true;
        state().bottle.drops = drops;
        const auto said = runScript("ch07/chaibao.lua");
        EXPECT_EQ(state().flag("ch07.chaibao"), 1) << "瓶里 " << drops << " 滴：拆包都走完";
        const fanren::rules::SpiritField* corner = state().findField("field_baiyaoyuan_jiao");
        ASSERT_NE(corner, nullptr);
        EXPECT_EQ(corner->slots.size(), 2u);
        const std::vector<std::pair<const char*, int>> seedlings = {
            {"herb_huangjing_cao", 3}, {"herb_zishen_cao", 2}, {"herb_xuehong_zhi", 1}, {"herb_huangjing_zhi", 2}};
        for (const auto& [herb, n] : seedlings) {
            EXPECT_EQ(state().itemCountOfAge(herb, 0), n) << "3.2 节点 1：give(\"" << herb << "\", " << n << ", 0)";
        }
        EXPECT_EQ(state().itemCount("material_lingshi_zhong"), 2) << "3.2 节点 1：中阶灵石 2";
        EXPECT_EQ(state().itemCount("talisman_tulao_fu"), 2) << "3.2 节点 1：土牢符 2";
        EXPECT_EQ(state().bottle.drops, drops > 0 ? drops - 1 : 0) << "3.2 节点 1：bottle.spend(1)，没液就跳过";
        EXPECT_EQ(has(said, "ch07.chaibao.test"), drops > 0);
        EXPECT_EQ(has(said, "ch07.chaibao.test_none"), drops == 0) << "瓶里没液就换一句，不设死局";
    }
}

// ===========================================================================
// 验收 13：嗅灵兽留幼苗——chukou.lua 对三味药用 count_aged(…, 100) ＋ take_aged(…, n, 100)，不用 take(id, item.count(id))
//（切片在 Ch07LedgerTests：两侧各跑一遍）
// ===========================================================================
TEST_F(Ch07Slice, No13_TheSnifferCountsAndTakesByAgeNotByTheWholePile) {
    const std::string code = codeOnly(chapterScripts_["ch07/chukou.lua"]);
    for (const char* herb : {"herb_yusui_zhi", "herb_zihou_hua", "herb_tianling_guo"}) {
        EXPECT_NE(code.find(std::string("item.count_aged(\"") + herb + "\", 100)"), std::string::npos)
            << "验收 13：count_aged(\"" << herb << "\", 100)";
        const std::regex taken(std::string("take_aged\\(\"") + herb + "\",\\s*[a-z_]+,\\s*100\\)");
        EXPECT_TRUE(std::regex_search(code, taken)) << "验收 13：take_aged(\"" << herb << "\", n, 100)";
        EXPECT_EQ(code.find(std::string("take(\"") + herb + "\""), std::string::npos) << "验收 13：不用 take(id, …)";
    }
    EXPECT_EQ(code.find("item.count(\""), std::string::npos) << "验收 13：不用 take(id, item.count(id))";
}

// ===========================================================================
// 验收 14：地火——facility_dihuo grade 4、require_flag ch07.feidan；意图存档下筑基丹方成算恰 33%；
// 35b 废丹 22 份、一盒废丹；药粉用尽而一颗未成时 35c 补药粉、不置旗标（切片在 Ch07LedgerTests）
// ===========================================================================
TEST_F(Ch07Slice, No14_TheEarthFireIsGradeFourAndAThirdOfTheBrewsTake) {
    const MapObject furnace = objectOn("ch07_dihuo", "facility_dihuo");
    ASSERT_FALSE(furnace.name.empty());
    EXPECT_EQ(furnace.property("grade"), "4") << "验收 14";
    EXPECT_EQ(furnace.property("require_flag"), "ch07.feidan") << "验收 14";
    const auto zhuji = app_.recipes().find("recipe_zhuji_dan");
    ASSERT_NE(zhuji, app_.recipes().end());
    // 意图存档 = 第 6 章交接存档（两侧同：熟练度 100、资质 50，施工图第 2 节）。
    for (const char* file : {fanren::test::kChapterSixEndingFirst, fanren::test::kChapterSixEndingSecond}) {
        auto loaded = fanren::io::loadGame(fanren::test::chapterFixturePath(root_, file).string());
        ASSERT_TRUE(loaded.ok) << file;
        const int chance = fanren::rules::successChance(zhuji->second, loaded.value.alchemyProficiency, loaded.value.aptitude,
                                                        std::stoi(furnace.property("grade")));
        EXPECT_EQ(chance, 33) << file << "：第 7 节「40 + 50 + 10 + 20 − 87 = 33%」";
    }
    // 切片：35b。
    freshEleventh();
    state().setFlag("ch07.dihuo");
    state().addItem("material_zhuji_yaofen", 40, 0);
    runScript("ch07/shijiu.lua");
    EXPECT_EQ(state().flag("ch07.feidan"), 1);
    EXPECT_EQ(state().itemCount("material_zhuji_yaofen"), 40 - 22) << "3.2 节点 35b：take 22";
    EXPECT_EQ(state().itemCount("story_feidan_yuhe"), 1) << "3.2 节点 35b：一盒废丹";
    // 施工图 3.2 节点 34：「不看余量多少，药粉一律 40 份」——身上一株禁地药也没有（全种进了田里）也给 40。
    freshEleventh();
    state().setFlag("ch07.xieshili");
    runScript("ch07/sannian.lua");
    EXPECT_EQ(state().itemCount("material_zhuji_yaofen"), 40) << "3.2 节点 34：药粉一律 40 份";
    EXPECT_EQ(state().flag("ch07.sannian"), 1);
}

// ===========================================================================
// 验收 16：支线 2 条，谓词与施工图第 6 节字面量相符；两处过期旗标；Z1 的两瓶 5 / 3
// ===========================================================================
struct Pred {
    std::string subject;
    fanren::core::QuestCondition::Op op;
    int value;
};
bool samePreds(const std::vector<fanren::core::QuestCondition>& got, const std::vector<Pred>& want) {
    if (got.size() != want.size()) return false;
    for (std::size_t i = 0; i < got.size(); ++i) {
        if (got[i].subject != want[i].subject || got[i].op != want[i].op || got[i].value != want[i].value) return false;
    }
    return true;
}

TEST_F(Ch07Slice, No16_TheTwoSideQuestsReadTheLiteralsOfSectionSix) {
    using Op = fanren::core::QuestCondition::Op;
    const fanren::core::Quest* z1 = nullptr;
    const fanren::core::Quest* z2 = nullptr;
    int chapterSeven = 0;
    for (const fanren::core::Quest& q : app_.data().quests) {
        if (q.kind != fanren::core::QuestKind::Side || q.chapter != 7) continue;
        ++chapterSeven;
        if (samePreds(q.accept, {{"ch07.baigong_qiu", Op::FlagAtLeast, 1}})) z1 = &q;
        if (samePreds(q.accept, {{"ch07.yujian_qiu", Op::FlagAtLeast, 1}})) z2 = &q;
    }
    EXPECT_EQ(chapterSeven, 2) << "施工图第 6 节：支线 2 条";
    ASSERT_NE(z1, nullptr) << "Z1（触发：ch07.baigong_qiu = 1）不在 data/quests";
    ASSERT_NE(z2, nullptr) << "Z2（触发：ch07.yujian_qiu = 1）不在 data/quests";
    EXPECT_TRUE(samePreds(z1->complete, {{"ch07.baigong", Op::FlagAtLeast, 1}})) << "Z1 完成：ch07.baigong = 1";
    EXPECT_TRUE(samePreds(z1->fail, {{"ch07.ma_songyao", Op::FlagAtLeast, 1}})) << "Z1 过期：ch07.ma_songyao 置位";
    EXPECT_TRUE(z1->rewards.empty()) << "Z1：不进 rewards";
    ASSERT_EQ(z1->steps.size(), 1u);
    EXPECT_EQ(z1->steps[0].targetObject, "trigger_yaolou") << "Z1：药篓交";
    EXPECT_TRUE(samePreds(z2->complete, {{"ch07.yujian_a", Op::FlagAtLeast, 1}, {"ch07.yujian_b", Op::FlagAtLeast, 1}}))
        << "Z2 完成：ch07.yujian_a == 1 且 ch07.yujian_b == 1";
    EXPECT_TRUE(samePreds(z2->fail, {{"ch07.didao", Op::FlagAtLeast, 1}})) << "Z2 过期：ch07.didao 置位";
    ASSERT_EQ(z2->steps.size(), 2u);
    EXPECT_EQ(z2->steps[0].targetObject, "trigger_yujian_dongpo");
    EXPECT_EQ(z2->steps[1].targetObject, "trigger_yujian_hantan");
    for (const fanren::core::Quest* q : {z1, z2}) {
        EXPECT_NE(text(q->failTextKey), q->failTextKey) << q->id << " 的过期原因查不到";
    }
    // 触发的那一节：Z1 在节点 3（药篓第二回）、Z2 在节点 24（铜门）。
    const auto settersOf = [&](const std::string& flagName) {
        std::set<std::string> setters;
        for (const auto& [path, source] : allScripts_) {
            if (codeOnly(source).find("flag.set(\"" + flagName + "\"") != std::string::npos) setters.insert(path);
        }
        return setters;
    };
    EXPECT_EQ(settersOf("ch07.baigong_qiu"), (std::set<std::string>{"ch07/yaolou.lua"}));
    EXPECT_EQ(settersOf("ch07.yujian_qiu"), (std::set<std::string>{"ch07/tongmen.lua"}));
    // 第 6 节 Z2 奖励：东坡玉髓芝 300 年 ×2、寒潭天灵果 300 年 ×2（成熟的，节点 32b 一并交上去）。
    EXPECT_NE(codeOnly(chapterScripts_["ch07/yujian_dongpo.lua"]).find("give(\"herb_yusui_zhi\", 2, 300)"), std::string::npos);
    EXPECT_NE(codeOnly(chapterScripts_["ch07/yujian_hantan.lua"]).find("give(\"herb_tianling_guo\", 2, 300)"), std::string::npos);
    // 切片：节点 17 两瓶——Z1 做了各 5、没做各 3（第 6 节「节点 17 两瓶各 5（未完成各 3）」）。
    for (const int done : {0, 1}) {
        freshEleventh();
        state().setFlag("ch07.baoming");
        state().setFlag("ch07.baigong_qiu");
        if (done) state().setFlag("ch07.baigong");
        runScript("ch07/ma_songyao.lua");
        EXPECT_EQ(state().itemCount("pill_yangjing_dan"), done ? 5 : 3) << (done ? "Z1 做了" : "Z1 没做");
        EXPECT_EQ(state().itemCount("pill_jinchuang_yao"), done ? 5 : 3);
        EXPECT_EQ(fanren::rules::questStatus(*z1, app_.data().objectives, state()),
                  done ? fanren::rules::QuestStatus::Completed : fanren::rules::QuestStatus::Failed);
    }
    // 切片：东坡两只铁臂猿「可逃——逃了不置旗标」（3.1 表 Z2a）。
    freshEleventh();
    state().setFlag("ch07.yujian_qiu");
    fleeAtOnce_ = true;
    runScript("ch07/yujian_dongpo.lua");
    fleeAtOnce_ = false;
    EXPECT_EQ(state().flag("ch07.yujian_a"), 0) << "3.1 Z2a：逃了不置旗标";
    EXPECT_EQ(state().itemCount("herb_yusui_zhi"), 0);
}

// ===========================================================================
// 验收 17：紧迫感的那一句——ch07.fudan.* 里有一条同时含「五年」「六十年」「最后」；
// 验收 18（文案那一半）：chukou.lua 里南宫婉登舟那一段的旁白含「没」「看」
// ===========================================================================
TEST_F(Ch07Slice, No17_And18_TheTwoSentencesTheDesignNames) {
    const std::string main = readFile(fs::path(root_) / "data" / "text" / "ch07_main.json");
    static const std::regex kPair("\"(ch07\\.fudan\\.[^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
    int fudan = 0;
    std::string hit;
    for (auto it = std::sregex_iterator(main.begin(), main.end(), kPair); it != std::sregex_iterator(); ++it) {
        ++fudan;
        const std::string value = (*it)[2];
        if (value.find("五年") != std::string::npos && value.find("六十年") != std::string::npos &&
            value.find("最后") != std::string::npos) {
            hit = (*it)[1];
        }
    }
    ASSERT_GT(fudan, 3) << "先验：ch07.fudan.* 读得到";
    EXPECT_FALSE(hit.empty()) << "验收 17：ch07.fudan.* 里要有一条同时含「五年」「六十年」「最后」";
    EXPECT_NE(codeOnly(chapterScripts_["ch07/chuangong.lua"]).find("\"" + hit + "\""), std::string::npos)
        << "验收 17：那一句要真的在节点 15 的脚本里说出来";
    // 验收 18：南宫婉登舟那一句（旁白，含「没」「看」），chukou.lua 里说。
    static const std::regex kTalk("talk\\(\"([a-z0-9_]*)\",\\s*\"(ch07\\.chukou\\.[a-z0-9_]+)\"\\)");
    const std::string code = codeOnly(chapterScripts_["ch07/chukou.lua"]);
    std::string ship;
    for (auto it = std::sregex_iterator(code.begin(), code.end(), kTalk); it != std::sregex_iterator(); ++it) {
        const std::string value = text((*it)[2]);
        if (std::string((*it)[1]).empty() && value.find("南宫婉") != std::string::npos && value.find("没") != std::string::npos &&
            value.find("看") != std::string::npos) {
            ship = (*it)[2];
        }
    }
    EXPECT_FALSE(ship.empty()) << "验收 18：chukou.lua 里南宫婉登舟那一段的旁白 key 含「没」「看」";
}

// ===========================================================================
// 判据依赖存档状态的几处，把玩家真能处在的状态都走一遍
// ===========================================================================
// 施工偏差 18.14：卖符少女的欠账（ch06.qianyao > 0）在 19b 那两句了结；== 0 一个字不提。两份交接存档只走得到一面。
TEST_F(Ch07Slice, TheTalismanGirlsDebtIsSettledOnlyWhenThereIsOne) {
    for (const int owed : {0, 2}) {
        freshEleventh();
        state().setFlag("ch07.xiang");
        if (owed > 0) state().setFlag("ch06.qianyao", owed);
        const auto said = runScript("ch07/liedui.lua");
        EXPECT_EQ(state().flag("ch07.qipai"), 1);
        EXPECT_EQ(has(said, "ch07.liedui.qian"), owed > 0) << "欠 " << owed << " 瓶";
        EXPECT_EQ(has(said, "ch07.liedui.qian_done"), owed > 0) << "欠 " << owed << " 瓶";
        EXPECT_EQ(state().flag("ch06.qianyao"), 0) << "欠 " << owed << " 瓶：说完就了结（施工偏差 18.14）";
        EXPECT_EQ(state().mapId, "ch07_jindi_wai") << "3.1 节点 19b：末尾 teleport 黄土坡（同图）";
    }
    // 读字：那两句只在 ch06.qianyao > 0 的分支里（不新建旗标、不给不扣）。
    const std::string liedui = codeOnly(chapterScripts_["ch07/liedui.lua"]);
    const std::size_t gate = liedui.find("if flag.get(\"ch06.qianyao\") > 0 then");
    ASSERT_NE(gate, std::string::npos) << "施工偏差 18.14：欠着才提";
    const std::size_t end = liedui.find("\nend", gate);
    const std::string branch = liedui.substr(gate, end - gate);
    EXPECT_NE(branch.find("ch07.liedui.qian\""), std::string::npos);
    EXPECT_EQ(branch.find("give("), std::string::npos) << "18.14：不要药、不另开买卖";
    EXPECT_EQ(branch.find("take("), std::string::npos) << "18.14：不要药、不另开买卖";
}

// 1.4：吴风开口第一句按 ch06.wufeng 分（赢：「那天的路数」；输：「底子补上来没有」）。两份交接存档都是赢的那一面。
TEST_F(Ch07Slice, WuFengOpensWithTheLineThatMatchesTheSparOfChapterSix) {
    for (const int wufeng : {1, 2}) {
        freshEleventh();
        state().setFlag("ch07.dufang");
        state().setFlag("ch06.wufeng", wufeng);
        const int day = state().day;
        const auto said = runScript("ch07/chuangong.lua");
        EXPECT_EQ(has(said, "ch07.chuangong.open_won"), wufeng == 1) << "ch06.wufeng = " << wufeng;
        EXPECT_EQ(has(said, "ch07.chuangong.open_lost"), wufeng == 2) << "ch06.wufeng = " << wufeng;
        EXPECT_EQ(state().flag("ch07.lianqi"), 1);
        EXPECT_EQ(state().day - day, 3) << "3.3：几夜未眠 3 日";
        EXPECT_TRUE(state().learnedMagics.empty()) << "3.2 节点 8 / 16.1 第 13 条：敛气术不进法术表";
    }
    // 传功阁第二回（节点 15）：返回之后才开；14 日；置 ch07.fudan_fa。再按一次只说一句闲话、日子不动。
    freshEleventh();
    state().setFlag("ch07.dufang");
    state().setFlag("ch07.lianqi");
    const auto before = runScript("ch07/chuangong.lua");
    EXPECT_EQ(state().flag("ch07.fudan_fa"), 0) << "节点 14 之前，传功阁只有一句闲话";
    EXPECT_TRUE(has(before, "ch07.chuangong.idle"));
    state().setFlag("ch07.fanhui");
    int day = state().day;
    runScript("ch07/chuangong.lua");
    EXPECT_EQ(state().flag("ch07.fudan_fa"), 1);
    EXPECT_EQ(state().day - day, 14) << "3.3：近半月 14 日";
    day = state().day;
    const auto idle = runScript("ch07/chuangong.lua");
    EXPECT_TRUE(has(idle, "ch07.chuangong.idle")) << "两场都演过了：一句闲话";
    EXPECT_EQ(state().day, day) << "闲话不走日子";
}

// 施工偏差 18.5：插刃是一个选项按八次；中途取消 = 收手——一柄也不算、不置旗标，回来重插。
TEST_F(Ch07Slice, PlantingTheBladesTakesEightPressesAndACancelLeavesNothingBehind) {
    const auto blocks = [&] {
        const std::string code = codeOnly(chapterScripts_["ch07/charen.lua"]);
        const std::size_t at = code.find("choice{");
        EXPECT_NE(at, std::string::npos);
        return code.substr(at, code.find('}', at) - at);
    }();
    EXPECT_EQ(countOf(blocks, "\"ch07."), 1) << "18.5：一个选项「插下一柄」";
    freshEleventh();
    state().setFlag("ch07.kuitan");
    choices_ = 0;
    runScript("ch07/charen.lua", {0, 0, 0, -1});
    EXPECT_EQ(choices_, 4) << "插了三柄、第四下取消";
    EXPECT_EQ(state().flag("ch07.mairen"), 0) << "18.5：中途取消 = 收手，不置旗标";
    choices_ = 0;
    runScript("ch07/charen.lua");
    EXPECT_EQ(choices_, 8) << "3.2 节点 26b：「插下去」八次";
    EXPECT_EQ(state().flag("ch07.mairen"), 1);
}

}  // namespace
