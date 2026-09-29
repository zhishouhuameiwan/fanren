// 第 6 章验收（docs/ch06-design.md 第 15 节）的第 5、7、8、9、12、13、14、16、17、19 条——扫脚本、读数据、
// 切片跑单个脚本。**不经过驱动、判据写死成施工图原文**。
//
// ---------------------------------------------------------------------------
// 为什么是这个形状（handoff 第 6 节第 5 条、handoff-2026-09-23-ch04 第 8 节第一条）
// ---------------------------------------------------------------------------
// 通关测试（tests/Ch06AcceptanceTests.cpp）问「走不走得完」；它驱动脚本，脚本一变它就跟着变，所以它只发现得了
// 「变了」，发现不了「错了」。这里每一条都是另一个形状：读 maps/、scripts/、data/ 的字，拿施工图原文当判据比；
// 要看「引擎里真的是那样」的几条，起单个脚本（切片），起点是手摆的最小局面、旁边写明摆了什么。
// 每一条都配了「故意写坏 → 确实被抓住」的变异（证据在测试路的回复里），另有几条扫描器自检，防「扫描器本身没牙」。
//
// 施工偏差（第 18 节，以它为准）：
//   · 18.2 第 5 条：Z2 的了结谓词是 ch06.zhang_done（sanhui.lua 出谷前数一遍），步骤谓词是灵石 ≥ 10；
//   · 18.4 第 1 条：认药三题出子夜花、白鹤芝、望月草，取消按选错算（同题重来）；
//   · 18.4 第 5 条：17a 取消就卷起竹简、return、不置旗标。
// 协调者落地时的一处偏差（tools/validate.py 注释里写着，施工图第 18 节没有）：验收 9 写「CHAPTER_MEANS[6] 含金、土」，
// 实际登在 BATTLE_EXTRA_MEANS 的三场 9b 之后的仗上——节点 6 的叶家寻衅在 9b 之前，整章登记会让规则 25 对那一仗放水。
// 本文件按落地的形状判「② 那一仗的必有手段含金、土」，差别写进测试路的回复。
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
#include "core/battle/Battle.h"
#include "core/model/AttackCategory.h"
#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "game/WorldScene.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::rules::Realm;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch06" / "kuxiu.lua") && fs::exists(root / "data" / "objectives" / "ch06.json")) {
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

// 一行去掉 -- 行注释之后的正文。本章脚本只有行注释；字符串里没有 --。
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

// scripts/<sub> 下所有 .lua（相对 scripts/ 的路径 → 全文）。
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

// 一个脚本里每一处 choice{ … } 的选项 key（按出现次序）。
std::vector<std::vector<std::string>> choiceBlocks(const std::string& source) {
    std::vector<std::vector<std::string>> out;
    const std::string code = codeOnly(source);
    static const std::regex kKey("\"([a-z0-9_.]+)\"");
    for (std::size_t at = code.find("choice{"); at != std::string::npos; at = code.find("choice{", at + 1)) {
        const std::size_t close = code.find('}', at);
        if (close == std::string::npos) break;
        const std::string body = code.substr(at, close - at);
        std::vector<std::string> keys;
        for (auto it = std::sregex_iterator(body.begin(), body.end(), kKey); it != std::sregex_iterator(); ++it) {
            keys.push_back((*it)[1]);
        }
        out.push_back(keys);
    }
    return out;
}

std::map<std::string, int> bagCounts(const GameState& state) {
    std::map<std::string, int> out;
    for (const auto& entry : state.bag) {
        if (entry.count != 0) out[entry.itemId] += entry.count;
    }
    return out;
}

// 施工图 18.4 第 1 条：认药三题的答案（按出题次序）。
constexpr const char* kQuizHerbs[] = {"子夜花", "白鹤芝", "望月草"};
// 施工图 E3：修炼面板灵根一行。
constexpr const char* kSpiritRootText = "四属性缺金·伪灵根";

class Ch06Slice : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        chapterScripts_ = scriptsUnder(root_, "ch06");
        allScripts_ = scriptsUnder(root_, "");
        ASSERT_GE(chapterScripts_.size(), 26u) << "先验：scripts/ch06/ 读得到（分母不能塌）";
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

    // 切片：起一个脚本，对白一路按确认；选择按 answers 的次序答（答完了取 fallback）。每一回要作答之前先叫一声
    // onChoice（看「选择还没定之前」的局面）。返回说过的文案 key。
    std::vector<std::string> runScript(const std::string& path, std::deque<int> answers = {}, int fallback = 0,
                                       const std::function<void()>& onChoice = {}) {
        std::vector<std::string> said;
        const auto started = app_.startEvent(path);
        EXPECT_TRUE(started.ok) << path << "：" << started.error;
        if (!started.ok) return said;
        app_.clearSpokenKeys();
        for (int frame = 0; frame < 4000 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
            if (!app_.awaitingCommand()) continue;
            const bool talked = !app_.spokenKeys().empty();
            said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
            app_.clearSpokenKeys();
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = 0;
            if (!talked) {
                if (onChoice) onChoice();
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
        EXPECT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束（选择一直答不对？）";
        app_.tick(1.0 / 60.0);
        said.insert(said.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
        app_.clearSpokenKeys();
        return said;
    }

    void setCount(const std::string& item, int count) {
        while (state().removeItem(item, 1)) {
        }
        if (count > 0) state().addItem(item, count, 0);
    }

    // 节点 9b 之后的他（施工图 1.3、8.1）：九层、132 / 90、六门法术、软剑、养精丹约 1 瓶、金疮药 4。
    void afterNineB() {
        state() = GameState{};
        state().realm = Realm::QiRefining9;
        state().realmCap = Realm::QiRefining9;
        state().hp = state().maxHp = fanren::rules::realmMaxHp(Realm::QiRefining9);
        state().mp = state().maxMp = fanren::rules::realmMaxMp(Realm::QiRefining9);
        for (const char* id : {"magic_huodan_shu", "magic_yufeng_jue", "magic_tianyan_shu", "magic_liusha_shu",
                               "magic_bingdong_shu", "magic_ji_jianfu"}) {
            state().learnMagic(id);
        }
        state().setFlag(fanren::game::kXiuxianKnownFlag);
        state().addItem("weapon_yudai_duanjian", 1, 0);
        state().addItem("pill_yangjing_dan", 1, 0);
        state().addItem("pill_jinchuang_yao", 4, 0);
    }

    Application app_;
    std::string root_;
    std::map<std::string, std::string> chapterScripts_;
    std::map<std::string, std::string> allScripts_;
    int choices_ = 0;
};

// ===========================================================================
// 验收 5：本章升一层——realm.advance 恰 1 处（kuxiu.lua）、目标 9；realm.cap 0 处
// ===========================================================================
TEST_F(Ch06Slice, No5_TheOneRealmStepIsInNineBAndNothingRaisesTheCap) {
    int advances = 0;
    std::string advancedIn;
    // 扫的是去掉注释的正文：脚本首部的注释照例会复述「realm.advance 只此一处、realm.cap 0 处」这句话。
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        const int n = countOf(code, "realm.advance(");
        advances += n;
        if (n > 0) advancedIn = path;
        EXPECT_EQ(countOf(code, "realm.cap("), 0) << path << " 写了 realm.cap（验收 5：realm.cap 0 处）";
    }
    EXPECT_EQ(advances, 1) << "验收 5：realm.advance 恰 1 处";
    EXPECT_EQ(advancedIn, "ch06/kuxiu.lua") << "验收 5：在 kuxiu.lua";
    EXPECT_NE(codeOnly(chapterScripts_["ch06/kuxiu.lua"]).find("realm.advance(realm.QI_REFINING_9)"), std::string::npos)
        << "验收 5：目标 9";
    // 扫描器自检：同一个扫法对第 4 章那三处升境是看得见的。
    int chapterFour = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch04")) chapterFour += countOf(codeOnly(source), "realm.advance(");
    EXPECT_EQ(chapterFour, 3) << "扫描器自检：第 4 章有三处 realm.advance";

    // 切片：八层 / 上限八层起步，苦修之后九层 / 上限九层、132 / 90（施工图 1.3），日子 +12（3.3），
    // 不扣养精丹（3.2 节点 9：「脚本不扣养精丹」），三门法术学会（10.1、验收 9）。
    state() = GameState{};
    state().realm = Realm::QiRefining8;
    state().realmCap = Realm::QiRefining8;
    state().hp = state().maxHp = 120;
    state().mp = state().maxMp = 80;
    state().setFlag("ch06.zhifu");
    state().addItem("pill_yangjing_dan", 1, 0);
    const int day = state().day;
    runScript("ch06/kuxiu.lua");
    EXPECT_EQ(state().realm, Realm::QiRefining9);
    EXPECT_EQ(state().realmCap, Realm::QiRefining9) << "realm.advance 顺带抬上限（施工图 1.3）";
    EXPECT_EQ(state().maxHp, 132) << "施工图 1.3：九层气血 24 + 12 × 9 = 132";
    EXPECT_EQ(state().maxMp, 90) << "施工图 1.3：九层法力 90";
    EXPECT_EQ(state().day - day, 12) << "施工图 3.3：9b 苦修 12 日";
    EXPECT_EQ(state().itemCount("pill_yangjing_dan"), 1) << "施工图 3.2 节点 9：脚本不扣养精丹";
    for (const char* id : {"magic_liusha_shu", "magic_bingdong_shu", "magic_ji_jianfu"}) {
        EXPECT_TRUE(state().knowsMagic(id)) << id;
    }
    EXPECT_EQ(state().flag("ch06.jiuceng"), 1);
}

// ===========================================================================
// 验收 7：牌子有了名字——take("material_heipai") 恰 1 处、give("story_shengxianling") 恰 1 处，同在 bilu.lua；
// material_heipai 在 scripts/ch06/ 里只出现这一处
// ===========================================================================
TEST_F(Ch06Slice, No7_ThePlaqueGetsItsNameInBiluAndNowhereElse) {
    int takes = 0;
    int gives = 0;
    int mentions = 0;
    std::set<std::string> where;
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        const int t = countOf(code, "take(\"material_heipai\"");
        const int g = countOf(code, "give(\"story_shengxianling\"");
        takes += t;
        gives += g;
        mentions += countOf(code, "material_heipai");
        if (t + g > 0) where.insert(path);
    }
    EXPECT_EQ(takes, 1) << "验收 7：take(\"material_heipai\") 恰 1 处";
    EXPECT_EQ(gives, 1) << "验收 7：give(\"story_shengxianling\") 恰 1 处";
    EXPECT_EQ(where, (std::set<std::string>{"ch06/bilu.lua"})) << "验收 7：同在 bilu.lua";
    EXPECT_EQ(mentions, 1) << "验收 7：material_heipai 在 scripts/ch06/ 里只出现这一处";
    // 顺序：先收牌子（看返回值），收到了才给升仙令。
    const std::string bilu = codeOnly(chapterScripts_["ch06/bilu.lua"]);
    EXPECT_NE(bilu.find("if not take(\"material_heipai\", 1)"), std::string::npos) << "施工图 3.2 节点 10：看返回值";
    EXPECT_LT(bilu.find("take(\"material_heipai\""), bilu.find("give(\"story_shengxianling\"")) << "先收牌子、再给升仙令";
    // 物品：升仙令的名字就是「升仙令」（E9）。
    const fanren::core::Item* token = app_.data().findItem("story_shengxianling");
    ASSERT_NE(token, nullptr);
    EXPECT_EQ(token->name, "升仙令");

    // 切片：有牌子 → 换成升仙令；没牌子 → 一句话 return，不给、不置旗标。
    state() = GameState{};
    state().setFlag("ch06.canpian");
    state().addItem("material_heipai", 1, 0);
    runScript("ch06/bilu.lua");
    EXPECT_EQ(state().itemCount("material_heipai"), 0);
    EXPECT_EQ(state().itemCount("story_shengxianling"), 1);
    EXPECT_EQ(state().flag("ch06.shengxianling"), 1);
    state() = GameState{};
    state().setFlag("ch06.canpian");
    runScript("ch06/bilu.lua");
    EXPECT_EQ(state().itemCount("story_shengxianling"), 0) << "没有牌子就不该凭空出一块升仙令";
    EXPECT_EQ(state().flag("ch06.shengxianling"), 0) << "没有牌子：不置旗标，挂点留着";
}

// ===========================================================================
// 验收 8：天眼术——learn 恰 1 处（cunkou.lua）；数据 effect == reveal、power == 0；施展后 knownWeaknesses 含本场
// 全部敌人；护身罡照旧点不动。战斗那一半用本章那一场生死仗（② b06_shanqiu_xisha），走真菜单。
// ===========================================================================
TEST_F(Ch06Slice, No8_TianyanIsLearnedOnceAtTheVillageMouth) {
    int learns = 0;
    std::string learnedIn;
    for (const auto& [path, source] : allScripts_) {
        const int n = countOf(codeOnly(source), "magic.learn(\"magic_tianyan_shu\")");
        learns += n;
        if (n > 0) learnedIn = path;
    }
    EXPECT_EQ(learns, 1) << "验收 8 / 12.2：magic.learn(\"magic_tianyan_shu\") 恰 1 处";
    EXPECT_EQ(learnedIn, "ch06/cunkou.lua") << "验收 8：在 cunkou.lua（1b）";
    const MapObject hook = objectOn("ch06_tainan_cun", "trigger_cunkou");
    EXPECT_EQ(hook.property("script"), "ch06/cunkou.lua") << "1b 的挂点挂的就是它";
    const fanren::core::Magic* tianyan = app_.data().findMagic("magic_tianyan_shu");
    ASSERT_NE(tianyan, nullptr);
    EXPECT_EQ(tianyan->effect, fanren::core::MagicEffect::Reveal) << "验收 8：effect == reveal";
    EXPECT_EQ(tianyan->power, 0) << "验收 8：power == 0";
    EXPECT_EQ(tianyan->needMp, 5) << "施工图 E5：needMp 5";
    EXPECT_EQ(tianyan->name, "天眼术");
}

TEST_F(Ch06Slice, No8_OneCastOfTianyanLaysBareBothFoesOfTheAmbushAndTheShieldStillCannotBeCast) {
    afterNineB();
    state().learnMagic("magic_hushen_gang");   // 手摆：护身罡本章不学，这里借来看「照旧点不动」
    ASSERT_TRUE(state().knownWeaknesses.empty()) << "先验：本章敌人一个也没见过";
    BattleScene scene("b06_shanqiu_xisha");
    scene.onEnter(app_);
    ASSERT_GE(scene.runToAllyTurn(), 0);
    int foes = 0;
    for (const Unit& u : scene.battle().units()) {
        if (u.ally) continue;
        ++foes;
        ASSERT_EQ(u.revealed, 0) << u.id << "：先验，开局零揭开";
        ASSERT_NE(u.weaknesses, 0) << u.id << "：先验，他有破绽可揭";
    }
    ASSERT_EQ(foes, 2) << "先验：黄衣人与土甲大汉";
    // 菜单：天眼术点得动；护身罡、御风决（不伤人、没有 effect）照旧点不动，理由还是那一句。
    std::vector<std::string> ids;
    const auto rows = BattleScene::buildMagicItems(app_.data(), state(), scene.battle(), 0, ids);
    ASSERT_GE(rows.size(), ids.size()) << "行与法术 id 同源同序（ids 之外的是返回那一行）";
    bool sawTianyan = false;
    bool sawShield = false;
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == "magic_tianyan_shu") {
            sawTianyan = true;
            EXPECT_TRUE(rows[i].enabled) << "天眼术不伤人，却点得动：" << rows[i].disabledReason;
        }
        if (ids[i] == "magic_hushen_gang") {
            sawShield = true;
            EXPECT_FALSE(rows[i].enabled) << "护身罡照旧点不动";
            EXPECT_EQ(rows[i].disabledReason, "【护身罡】不是伤人的法术") << "理由还是原来那一句";
        }
        if (ids[i] == "magic_yufeng_jue") EXPECT_FALSE(rows[i].enabled) << "御风决不伤人、没有 effect，照旧点不动";
    }
    ASSERT_TRUE(sawTianyan);
    ASSERT_TRUE(sawShield);
    // 走真菜单点天眼术：施法 → 那一行（看破不挑目标，点下去就施展了）。
    scene.openMenu(app_);
    ASSERT_TRUE(scene.menuChoose(app_, fanren::game::kBattleMenuCast));
    int row = -1;
    for (int i = 0; i < scene.menuList().count(); ++i) {
        if (scene.menuList().items()[static_cast<std::size_t>(i)].label == "天眼术") row = i;
    }
    ASSERT_GE(row, 0) << "法术列表里没有天眼术";
    ASSERT_TRUE(scene.menuChoose(app_, row)) << scene.feedback();
    for (const Unit& u : scene.battle().units()) {
        if (!u.ally) EXPECT_EQ(u.revealed, u.weaknesses) << u.id << "：施展一次，本场所有敌人的破绽全部揭开（E1）";
    }
    // 打完（那只手），收场把揭开的并进存档：knownWeaknesses 含本场全部敌人（验收 8）。
    fanren::test::BattleHand hand(app_);
    hand.play(scene);
    scene.update(app_, 1.0 / 60.0);
    for (const char* id : {"huangyi_ren", "tujia_dahan"}) {
        const fanren::core::RoleTemplate* role = app_.data().findRole(id);
        ASSERT_NE(role, nullptr);
        EXPECT_EQ(state().knownWeaknessesOf(id), role->weaknesses) << id << "：验收 8「施展后 knownWeaknesses 含本场全部敌人」";
    }
}

// ===========================================================================
// 验收 9：祭剑符常驻——magic.learn("magic_ji_jianfu") 恰 1 处（kuxiu.lua），magic.forget 0 处；
// 规则 25 登了本章 ② 的金、土
// ===========================================================================
TEST_F(Ch06Slice, No9_TheSwordTalismanStaysFromNineBAndRuleTwentyFiveKnowsMetalAndEarth) {
    int learns = 0;
    int forgets = 0;
    std::string learnedIn;
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        const int l = countOf(code, "magic.learn(\"magic_ji_jianfu\")");
        learns += l;
        forgets += countOf(code, "magic.forget(");
        if (l > 0) learnedIn = path;
    }
    EXPECT_EQ(learns, 1) << "验收 9：magic.learn(\"magic_ji_jianfu\") 恰 1 处";
    EXPECT_EQ(learnedIn, "ch06/kuxiu.lua") << "验收 9：在 kuxiu.lua";
    EXPECT_EQ(forgets, 0) << "验收 9：magic.forget 0 处（第 7 章 ch171 剑符报废时才收回）";
    // 扫描器自检：第 5 章借一仗的那一对是看得见的。
    int chapterFive = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch05")) chapterFive += countOf(codeOnly(source), "magic.forget(\"magic_ji_jianfu\")");
    EXPECT_EQ(chapterFive, 1) << "扫描器自检：第 5 章那一处 magic.forget 该数得出来";

    // tools/validate.py：MEANS_LAST_CHAPTER = 6；② 那一场的必有手段里有金（祭剑符）与土（流沙术），来路都在 ch06。
    const std::string validate = readFile(fs::path(root_) / "tools" / "validate.py");
    ASSERT_GT(validate.size(), 10000u) << "先验：读得到 tools/validate.py";
    EXPECT_NE(validate.find("\nMEANS_LAST_CHAPTER = 6\n"), std::string::npos) << "施工图 E6：MEANS_LAST_CHAPTER = 6";
    const std::size_t chapterMeans = validate.find("\nCHAPTER_MEANS = {");
    ASSERT_NE(chapterMeans, std::string::npos);
    EXPECT_NE(validate.find("\n    6: [", chapterMeans), std::string::npos) << "施工图 E6：CHAPTER_MEANS 有第 6 章";
    const std::size_t extra = validate.find("\nBATTLE_EXTRA_MEANS = {");
    ASSERT_NE(extra, std::string::npos);
    const std::size_t extraEnd = validate.find("\n}", extra);
    ASSERT_NE(extraEnd, std::string::npos);
    const std::string table = validate.substr(extra, extraEnd - extra);
    const std::size_t xisha = table.find("\"b06_shanqiu_xisha\": [");
    ASSERT_NE(xisha, std::string::npos) << "② 那一场没登必有手段：killable_by 金会被规则 25 放过（施工图 E6）";
    const std::string row = table.substr(xisha, table.find(']', xisha) - xisha);
    EXPECT_NE(row.find("(\"金\", \"learn\", \"magic_ji_jianfu\", \"ch06\")"), std::string::npos) << "施工图 8.2：金 ← 祭剑符（9b）";
    EXPECT_NE(row.find("(\"土\", \"learn\", \"magic_liusha_shu\", \"ch06\")"), std::string::npos) << "施工图 8.2：土 ← 流沙术（9b）";
}

// ===========================================================================
// 验收 12：用词切换——flag.set("story.xiuxian_known") 恰 1 处（guaipo.lua）；置后 currencyName == 灵石、magicWord == 法术
// ===========================================================================
TEST_F(Ch06Slice, No12_TheWordsOfTheImmortalsSwitchOnInGuaipoOnly) {
    int sets = 0;
    std::string setIn;
    for (const auto& [path, source] : allScripts_) {
        const int n = countOf(codeOnly(source), "flag.set(\"story.xiuxian_known\"");
        sets += n;
        if (n > 0) setIn = path;
    }
    EXPECT_EQ(sets, 1) << "验收 12：flag.set(\"story.xiuxian_known\") 恰 1 处（全仓，第 1–5 章不置）";
    EXPECT_EQ(setIn, "ch06/guaipo.lua") << "验收 12：在 guaipo.lua";

    // 切片：怪坡那一节演完，面板用词换成修仙界的（src/game/Wording.h）。
    state() = GameState{};
    state().setFlag("ch06.wan_met");
    EXPECT_EQ(std::string(fanren::game::currencyName(state())), "碎银") << "先验：置旗标之前还是碎银";
    EXPECT_EQ(std::string(fanren::game::magicWord(fanren::game::wordingStage(state()))), "法门");
    runScript("ch06/guaipo.lua");
    EXPECT_EQ(state().flag("story.xiuxian_known"), 1);
    EXPECT_EQ(std::string(fanren::game::currencyName(state())), "灵石") << "验收 12：currencyName == 灵石";
    EXPECT_EQ(std::string(fanren::game::magicWord(fanren::game::wordingStage(state()))), "法术") << "验收 12：magicWord == 法术";
    EXPECT_EQ(std::string(fanren::game::mpWord(fanren::game::wordingStage(state()))), "法力") << "施工图 3.2 节点 2：法力";
    EXPECT_EQ(state().mapId, "ch06_tainan_gu") << "施工图 3.1 节点 2：脚本末尾 teleport 太南谷";
    EXPECT_EQ(state().flag("ch06.rugu"), 1);
}

// ===========================================================================
// 验收 13：灵根一行——ch06.linggen 置后修炼面板含「四属性缺金·伪灵根」，置前不含
// ===========================================================================
TEST_F(Ch06Slice, No13_TheSpiritRootLineAppearsOnceHeHasBeenTested) {
    state() = GameState{};
    state().setFlag(fanren::game::kXiuxianKnownFlag);   // 节点 13 在 2 之后：已进修仙界
    state().setFlag("ch06.xisha");
    EXPECT_EQ(fanren::game::CultivationScene::spiritRootLine(state()), "") << "验收 13：测灵根之前不含";
    const int day = state().day;
    runScript("ch06/linggen.lua");
    EXPECT_EQ(state().flag("ch06.linggen"), 1);
    const std::string line = fanren::game::CultivationScene::spiritRootLine(state());
    EXPECT_NE(line.find(kSpiritRootText), std::string::npos) << "验收 13：置后含「四属性缺金·伪灵根」：" << line;
    EXPECT_EQ(state().day - day, 4) << "施工图 3.3：13 迎宾楼 4 天";
    EXPECT_EQ(state().aptitude, 50) << "施工图第 7 节：aptitude 不动";
    // 负向：面板那一行认的就是这个旗标——撤掉它，那一行就没了。
    state().flags.erase("ch06.linggen");
    EXPECT_EQ(fanren::game::CultivationScene::spiritRootLine(state()), "");
}

// ===========================================================================
// 验收 14：灵田——field.unlock("field_baiyaoyuan", 6) 恰 1 处（maiping.lua）
// ===========================================================================
TEST_F(Ch06Slice, No14_TheHerbFieldIsOpenedOnceWithSixSlotsAtTheChaptersEnd) {
    int unlocks = 0;
    std::string unlockedIn;
    for (const auto& [path, source] : allScripts_) {
        const int n = countOf(codeOnly(source), "field.unlock(");
        if (n == 0 || path.rfind("ch06/", 0) != 0) continue;
        unlocks += n;
        unlockedIn = path;
    }
    EXPECT_EQ(unlocks, 1) << "验收 14：本章 field.unlock 恰 1 处";
    EXPECT_EQ(unlockedIn, "ch06/maiping.lua") << "验收 14：在 maiping.lua";
    EXPECT_NE(codeOnly(chapterScripts_["ch06/maiping.lua"]).find("field.unlock(\"field_baiyaoyuan\", 6)"), std::string::npos)
        << "验收 14：field.unlock(\"field_baiyaoyuan\", 6)";
    // 章末：不 ending()、不 teleport（施工图 3.2 节点 18b「不 ending()」）。
    const std::string maiping = codeOnly(chapterScripts_["ch06/maiping.lua"]);
    EXPECT_EQ(countOf(maiping, "ending("), 0);
    EXPECT_EQ(countOf(maiping, "teleport("), 0);

    // 切片：18b 演完，灵田在、6 槽、ch06.done 置下；小瓶的状态一样不动（施工图 3.2 节点 18b）。
    state() = GameState{};
    state().setFlag("ch06.renyao");
    state().setFlag("ch06.huinuo");
    state().setFlag("ch06.rangdan");
    state().bottle.owned = true;
    state().bottle.drops = 2;
    const int day = state().day;
    runScript("ch06/maiping.lua");
    const fanren::rules::SpiritField* field = state().findField("field_baiyaoyuan");
    ASSERT_NE(field, nullptr) << "验收 14：灵田开出来";
    EXPECT_EQ(field->slots.size(), 6u);
    EXPECT_EQ(state().flag("ch06.done"), 1);
    EXPECT_EQ(state().day - day, 4) << "施工图 3.3：18b 送物 4 天";
    EXPECT_TRUE(state().bottle.owned) << "施工图 3.2 节点 18b：bottle 状态不动";
    EXPECT_EQ(state().bottle.drops, 2) << "施工图 3.2 节点 18b：bottle 状态不动";
}

// ===========================================================================
// 验收 16：支线 2 条，谓词与施工图第 6 节字面量相符（Z2 的了结按施工偏差 18.2 第 5 条）
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

TEST_F(Ch06Slice, No16_TheTwoSideQuestsReadTheLiteralsOfSectionSix) {
    using Op = fanren::core::QuestCondition::Op;
    const fanren::core::Quest* z1 = nullptr;
    const fanren::core::Quest* z2 = nullptr;
    int chapterSix = 0;
    for (const fanren::core::Quest& q : app_.data().quests) {
        if (q.kind != fanren::core::QuestKind::Side || q.chapter != 6) continue;
        ++chapterSix;
        if (samePreds(q.accept, {{"ch06.dingshen_qiu", Op::FlagAtLeast, 1}})) z1 = &q;
        // Z2 的接取谓词：校对整改 16.4（HIGH-3）改到 8b 之后——ch06.jinzhubi（原为 5a 末尾置的 ch06.zhang_qiu）。
        if (samePreds(q.accept, {{"ch06.jinzhubi", Op::FlagAtLeast, 1}})) z2 = &q;
    }
    EXPECT_EQ(chapterSix, 2) << "施工图第 6 节：支线 2 条";
    ASSERT_NE(z1, nullptr) << "Z1（触发：ch06.dingshen_qiu = 1）不在 data/quests";
    ASSERT_NE(z2, nullptr) << "Z2（触发：ch06.jinzhubi = 1，校对整改 16.4）不在 data/quests";
    // Z1：步骤 / 完成 talisman_dingshen_fu ≥ 1；过期 ch06.chugu 置位；不进 rewards。
    ASSERT_EQ(z1->steps.size(), 1u);
    EXPECT_TRUE(samePreds(z1->steps[0].done, {{"talisman_dingshen_fu", Op::ItemAtLeast, 1}})) << "Z1 步骤";
    EXPECT_EQ(z1->steps[0].targetMap, "ch06_sanxiu_lou") << "Z1：在制符桌上画";
    EXPECT_EQ(z1->steps[0].targetObject, "facility_zhifu");
    EXPECT_TRUE(samePreds(z1->complete, {{"talisman_dingshen_fu", Op::ItemAtLeast, 1}})) << "Z1 完成";
    EXPECT_TRUE(samePreds(z1->fail, {{"ch06.chugu", Op::FlagAtLeast, 1}})) << "Z1 过期";
    EXPECT_TRUE(z1->rewards.empty()) << "Z1：不进 rewards";
    // Z2：步骤灵石 ≥ 10；了结 ch06.zhang_done（施工偏差 18.2 第 5 条）；过期 ch06.chugu；无物品。
    ASSERT_EQ(z2->steps.size(), 1u);
    EXPECT_TRUE(samePreds(z2->steps[0].done, {{"material_lingshi", Op::ItemAtLeast, 10}})) << "Z2 步骤：≥ 10 块";
    EXPECT_TRUE(samePreds(z2->complete, {{"ch06.zhang_done", Op::FlagAtLeast, 1}})) << "Z2 了结（施工偏差 18.2 第 5 条）";
    EXPECT_TRUE(samePreds(z2->fail, {{"ch06.chugu", Op::FlagAtLeast, 1}})) << "Z2 过期";
    EXPECT_TRUE(z2->rewards.empty()) << "Z2：无物品";
    for (const fanren::core::Quest* q : {z1, z2}) {
        EXPECT_NE(text(q->failTextKey), q->failTextKey) << q->id << " 的过期原因查不到";
    }
    // 触发的那一节（第 6 节「触发」一行）：Z1 在 9a 末尾（zhifu.lua）；Z2 在 8b 换完金竺笔之后（shaonv.lua 置
    // ch06.jinzhubi；校对整改 16.4，原在 5a 末尾，ch06.zhang_qiu 随之删掉、全仓没有脚本再置它）；
    // zhang_done 由 11 的脚本（sanhui.lua）出谷前数一遍再置，排在 ch06.chugu 之前。
    const auto settersOf = [&](const std::string& flagName) {
        std::set<std::string> setters;
        for (const auto& [path, source] : allScripts_) {
            if (codeOnly(source).find("flag.set(\"" + flagName + "\"") != std::string::npos) setters.insert(path);
        }
        return setters;
    };
    EXPECT_EQ(settersOf("ch06.dingshen_qiu"), (std::set<std::string>{"ch06/zhifu.lua"}));
    EXPECT_EQ(settersOf("ch06.jinzhubi"), (std::set<std::string>{"ch06/shaonv.lua"}));
    EXPECT_TRUE(settersOf("ch06.zhang_qiu").empty()) << "校对整改 16.4：ch06.zhang_qiu 删掉了，不该再有脚本置它";
    EXPECT_EQ(settersOf("ch06.zhang_done"), (std::set<std::string>{"ch06/sanhui.lua"}));
    const std::string sanhui = codeOnly(chapterScripts_["ch06/sanhui.lua"]);
    EXPECT_LT(sanhui.find("flag.set(\"ch06.zhang_done\""), sanhui.find("flag.set(\"ch06.chugu\"")) << "先数一遍、再出谷";

    // 切片：出谷前 9 块 → 不了结；10 块 → 了结（第 6 节「≥ 10」）。
    for (const int stones : {9, 10}) {
        state() = GameState{};
        state().setFlag("ch06.jinzhubi");   // 8b 换完了笔：Z2 接着了（校对整改 16.4）
        state().setFlag("ch06.shengxianling");
        setCount("material_lingshi", stones);
        runScript("ch06/sanhui.lua");
        EXPECT_EQ(state().flag("ch06.zhang_done"), stones >= 10 ? 1 : 0) << "出谷前身上 " << stones << " 块";
        EXPECT_EQ(state().flag("ch06.chugu"), 1);
        EXPECT_EQ(fanren::rules::questStatus(*z2, app_.data().objectives, state()),
                  stones >= 10 ? fanren::rules::QuestStatus::Completed : fanren::rules::QuestStatus::Failed)
            << "出谷前身上 " << stones << " 块";
    }
}

// 校对整改 16.4（HIGH-3）新增：Z2 的接取时机。从前 5a 走完就接，任务目标直指坊市、note 写「卖一瓶养精丹 7 块」——
// 最顺手的做法是卖两瓶，8b 的七瓶就凑不齐。现在 5a 走完还没接，8b 换完金竺笔才接（接取谓词 ch06.jinzhubi）。
TEST_F(Ch06Slice, No16_TheSecondSideQuestIsTakenOnlyOnceTheBrushIsBought) {
    using fanren::rules::QuestStatus;
    const fanren::core::Quest* z2 = nullptr;
    for (const fanren::core::Quest& q : app_.data().quests) {
        if (q.id == "q06_zhang") z2 = &q;
    }
    ASSERT_NE(z2, nullptr) << "先验：data/quests 里有 q06_zhang";
    state() = GameState{};
    state().setFlag("ch06.ruhuo");
    runScript("ch06/lingshi.lua");
    ASSERT_EQ(state().flag("ch06.lingshi"), 1) << "先验：5a 走完了";
    EXPECT_EQ(fanren::rules::questStatus(*z2, app_.data().objectives, state()), QuestStatus::NotAccepted)
        << "5a 走完还不该接 Z2：那时去坊市卖养精丹，8b 就差瓶（校对 HIGH-3）";
    EXPECT_EQ(state().flag("ch06.zhang_qiu"), 0) << "ch06.zhang_qiu 删掉了";
    state().setFlag("ch06.shuangshou");
    setCount("pill_yangjing_dan", 9);
    runScript("ch06/shaonv.lua");
    ASSERT_EQ(state().flag("ch06.jinzhubi"), 1) << "先验：8b 换完了笔";
    EXPECT_EQ(fanren::rules::questStatus(*z2, app_.data().objectives, state()), QuestStatus::Active)
        << "8b 换完金竺笔，Z2 就接着了";
}

// ===========================================================================
// 验收 17：R-3 提示在前——目标链第 7、11、16 步的文案含「丹药」「数一遍」「练熟」；顺带把 3.4 那张表整张判卷
// ===========================================================================
TEST_F(Ch06Slice, No17_TheObjectiveChainIsTableThreeFourAndItsHintsNameTheRemedy) {
    struct StepSpec {
        const char* id;
        const char* doneFlag;
        const char* map;
        const char* object;
    };
    const std::vector<StepSpec> kChain = {
        {"n01_chudu", "ch06.chudu", "ch06_tainan_cun", "trigger_chudu"},
        {"n02_cunkou", "ch06.wan_met", "ch06_tainan_cun", "trigger_cunkou"},
        {"n03_guaipo", "ch06.rugu", "ch06_tainan_cun", "trigger_guaipo"},
        {"n04_qingyan", "ch06.qingyan", "ch06_tainan_gu", "trigger_qingyan"},
        {"n05_ruhuo", "ch06.ruhuo", "ch06_tainan_gu", "trigger_ruhuo"},
        {"n06_lingshi", "ch06.lingshi", "ch06_tainan_gu", "trigger_lingshi"},
        {"n07_feixingfu", "ch06.feixingfu", "ch06_tainan_gu", "npc_caomao_qingnian"},
        {"n08_xunxin", "ch06.xunxin", "ch06_tainan_gu", "trigger_ye_xunxin"},
        {"n09_yishi", "ch06.yishi", "ch06_sanxiu_lou", "trigger_yishi"},
        {"n10_shuangshou", "ch06.shuangshou", "ch06_tainan_gu", "trigger_shuangshou"},
        {"n11_jinzhubi", "ch06.jinzhubi", "ch06_tainan_gu", "npc_maifu_shaonv"},
        {"n12_zhifu", "ch06.zhifu", "ch06_sanxiu_lou", "trigger_zhifu"},
        {"n13_kuxiu", "ch06.jiuceng", "ch06_sanxiu_lou", "trigger_kuxiu"},
        {"n14_canpian", "ch06.canpian", "ch06_tainan_gu", "trigger_canpian"},
        {"n15_bilu", "ch06.shengxianling", "ch06_sanxiu_lou", "trigger_bilu"},
        {"n16_sanhui", "ch06.chugu", "ch06_tainan_gu", "trigger_sanhui"},
        {"n17_xisha", "ch06.xisha", "ch06_shanqiu", "trigger_xisha"},
        {"n18_linggen", "ch06.linggen", "ch06_huangfenggu", "trigger_ce_linggen"},
        {"n19_rangdan", "ch06.rangdan", "ch06_huangfenggu", "trigger_ye_maidan"},
        {"n20_dadian", "ch06.rumen", "ch06_huangfenggu", "trigger_dadian"},
        {"n21_lingqu", "ch06.chuwudai", "ch06_huangfenggu", "trigger_lin_lingqu"},
        {"n22_wufeng", "ch06.wufeng", "ch06_huangfenggu", "trigger_wufeng"},
        {"n23_zawu", "ch06.zawu", "ch06_huangfenggu", "trigger_zawu"},
        {"n24_juanzong", "ch06.huinuo", "ch06_huangfenggu", "trigger_juanzong"},
        {"n25_renyao", "ch06.renyao", "ch06_baiyaoyuan", "trigger_jinzhi"},
        {"n26_maiping", "ch06.done", "ch06_baiyaoyuan", "trigger_maiping"},
    };
    std::vector<const fanren::core::Objective*> chain;
    for (const fanren::core::Objective& o : app_.data().objectives) {
        if (o.chapter == 6) chain.push_back(&o);
    }
    ASSERT_EQ(chain.size(), kChain.size()) << "施工图 3.4：第 6 章目标链 26 步";
    for (std::size_t i = 0; i < kChain.size(); ++i) {
        EXPECT_EQ(chain[i]->id, kChain[i].id) << "第 " << (i + 1) << " 步";
        EXPECT_EQ(chain[i]->doneFlag, kChain[i].doneFlag) << kChain[i].id;
        EXPECT_EQ(chain[i]->targetMap, kChain[i].map) << kChain[i].id;
        EXPECT_EQ(chain[i]->targetObject, kChain[i].object) << kChain[i].id;
        EXPECT_NE(text(chain[i]->textKey), chain[i]->textKey) << kChain[i].id << " 的文案查不到";
    }
    const auto lineOf = [&](std::size_t number) { return text(chain[number - 1]->textKey); };
    EXPECT_NE(lineOf(7).find("丹药"), std::string::npos) << "验收 17：第 7 步含「丹药」：" << lineOf(7);
    EXPECT_NE(lineOf(11).find("数一遍"), std::string::npos) << "验收 17：第 11 步含「数一遍」：" << lineOf(11);
    EXPECT_NE(lineOf(16).find("练熟"), std::string::npos) << "验收 17：第 16 步含「练熟」：" << lineOf(16);
    // 3.4 表那三格还写了整句：第 7 步「别一次全掏出来」、第 11 步「书两颗、笔七瓶」、第 16 步「路上未必太平」。
    EXPECT_NE(lineOf(7).find("别一次全掏出来"), std::string::npos) << lineOf(7);
    EXPECT_NE(lineOf(11).find("书两颗"), std::string::npos) << lineOf(11);
    EXPECT_NE(lineOf(11).find("笔七瓶"), std::string::npos) << lineOf(11);
    EXPECT_NE(lineOf(16).find("路上未必太平"), std::string::npos) << lineOf(16);
}

// ===========================================================================
// 验收 19：认药三题——选错回到选择、不置旗标、不扣物品；三题全对才置 ch06.renyao
// ===========================================================================
// 选项从 jinzhi.lua 的 choice{} 读（key），哪一项算对只看施工图（18.4 第 1 条的三味草名）。
TEST_F(Ch06Slice, No19_AWrongHerbSendsHimBackToTheSameQuestionAndOnlyThreeRightAnswersPass) {
    const auto blocks = choiceBlocks(chapterScripts_["ch06/jinzhi.lua"]);
    ASSERT_EQ(blocks.size(), 3u) << "施工偏差 18.4 第 1 条：三题";
    std::vector<int> right;
    for (std::size_t q = 0; q < blocks.size(); ++q) {
        ASSERT_EQ(blocks[q].size(), 4u) << "施工图 3.2 节点 18：四个选项";
        int at = -1;
        int herbs = 0;
        for (std::size_t i = 0; i < blocks[q].size(); ++i) {
            const std::string label = text(blocks[q][i]);
            if (label == kQuizHerbs[q]) at = static_cast<int>(i);
            for (const char* herb : kQuizHerbs) herbs += label == herb ? 1 : 0;
        }
        ASSERT_GE(at, 0) << "第 " << (q + 1) << " 题的选项里没有「" << kQuizHerbs[q] << "」（施工偏差 18.4 第 1 条）";
        EXPECT_EQ(herbs, 1) << "第 " << (q + 1) << " 题：四个选项里只该有一味是园里那三味之一（干扰项用黄精、紫参、土骨花与一味瞎编的）";
        right.push_back(at);
    }
    // 直接读：每一题的循环认的那一项（`} ~= N do`，1 起算）就是施工图那一味所在的位置。
    {
        const std::string code = codeOnly(chapterScripts_["ch06/jinzhi.lua"]);
        static const std::regex kLoop("\\}\\s*~=\\s*([0-9]+)\\s*do");
        std::vector<int> accepted;
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kLoop); it != std::sregex_iterator(); ++it) {
            accepted.push_back(std::stoi((*it)[1]) - 1);
        }
        EXPECT_EQ(accepted, right) << "认药每一题认作「对」的那一项，与施工图 18.4 第 1 条的三味草名不在同一个位置";
    }
    // 每题先错一次（第一题再加一次取消——18.4 第 1 条「取消按选错算」），再答对。
    std::deque<int> answers;
    std::vector<int> wrongPerQuestion;
    int wrongPicks = 0;
    for (std::size_t q = 0; q < right.size(); ++q) {
        answers.push_back((right[q] + 1) % 4);
        int wrong = 1;
        if (q == 0) {
            answers.push_back(-1);
            ++wrong;
        }
        answers.push_back(right[q]);
        wrongPerQuestion.push_back(wrong);
        wrongPicks += wrong;
    }
    const int totalPicks = static_cast<int>(answers.size());
    state() = GameState{};
    state().setFlag("ch06.huinuo");
    state().addItem("pill_yangjing_dan", 1, 0);
    state().addItem("material_lingshi", 7, 0);
    const auto bagBefore = bagCounts(state());
    int checked = 0;
    const auto duringQuiz = [&]() {
        ++checked;
        EXPECT_EQ(state().flag("ch06.renyao"), 0) << "第 " << checked << " 次作答之前就置了 ch06.renyao";
        EXPECT_EQ(bagCounts(state()), bagBefore) << "第 " << checked << " 次作答之前背包变了：选错不扣任何东西";
    };
    choices_ = 0;
    const std::vector<std::string> said = runScript("ch06/jinzhi.lua", answers, 0, duringQuiz);
    EXPECT_EQ(choices_, totalPicks) << "选错（含取消）回到同一题：一共要答这么多次";
    EXPECT_EQ(checked, totalPicks);
    int wrongLines = 0;
    for (const std::string& key : said) wrongLines += key.rfind("ch06.jinzhi.wrong", 0) == 0 ? 1 : 0;
    EXPECT_EQ(wrongLines, wrongPicks) << "每选错一次（取消同），马师伯冷笑一声、同一题重来";
    // 逐题：第 q 题冷笑的次数就是那一题答错（含取消）的次数，「过了」那一句各说一次。
    // （文案 key 只拿来认出「演到了哪一题」，判据是上面按施工图排好的答题次序。）
    for (std::size_t q = 0; q < wrongPerQuestion.size(); ++q) {
        const std::string wrongKey = "ch06.jinzhi.wrong" + std::to_string(q + 1);
        const std::string rightKey = "ch06.jinzhi.right" + std::to_string(q + 1);
        EXPECT_EQ(std::count(said.begin(), said.end(), wrongKey), wrongPerQuestion[q]) << "第 " << (q + 1) << " 题";
        EXPECT_EQ(std::count(said.begin(), said.end(), rightKey), 1) << "第 " << (q + 1) << " 题认对之后才往下走";
    }
    EXPECT_EQ(state().flag("ch06.renyao"), 1) << "三题全对才置 ch06.renyao";
    auto bagAfter = bagCounts(state());
    EXPECT_EQ(bagAfter["story_mupai"], 1) << "木牌 give(\"story_mupai\")";
    bagAfter.erase("story_mupai");
    EXPECT_EQ(bagAfter, bagBefore) << "认药不扣任何东西";
}

// 施工偏差 18.4 第 5 条：17a 取消——卷起竹简、return、不置旗标；3.2 节点 17：前三项各回一句再回到竹简。
TEST_F(Ch06Slice, No19_TheSlipOfChoresRollsUpOnCancelAndOnlyTheHerbGardenGoesThrough) {
    state() = GameState{};
    state().setFlag("ch06.wufeng");
    state().setFlag("ch06.rangdan");
    const std::vector<std::string> cancelled = runScript("ch06/zawu.lua", {-1});
    EXPECT_EQ(state().flag("ch06.zawu"), 0) << "取消：不置旗标，挂点留着";
    EXPECT_NE(std::find(cancelled.begin(), cancelled.end(), "ch06.zawu.roll"), cancelled.end()) << "取消：卷起竹简那一句";
    const auto blocks = choiceBlocks(chapterScripts_["ch06/zawu.lua"]);
    ASSERT_EQ(blocks.size(), 1u);
    ASSERT_EQ(blocks[0].size(), 4u) << "施工图 3.2 节点 17：四选一";
    int garden = -1;
    for (std::size_t i = 0; i < blocks[0].size(); ++i) {
        if (text(blocks[0][i]).find("百药园") != std::string::npos) garden = static_cast<int>(i);
    }
    ASSERT_GE(garden, 0) << "竹简上没有「接管青石岭百药园」那一条";
    std::deque<int> answers;
    for (int i = 0; i < 4; ++i) {
        if (i != garden) answers.push_back(i);
    }
    answers.push_back(garden);
    choices_ = 0;
    const std::vector<std::string> said = runScript("ch06/zawu.lua", answers);
    EXPECT_EQ(choices_, 4) << "前三项各回一句、回到竹简；第四次才走得下去";
    EXPECT_EQ(state().flag("ch06.zawu"), 1);
    for (const char* key : {"ch06.zawu.no_tree", "ch06.zawu.no_shen", "ch06.zawu.no_mei"}) {
        EXPECT_NE(std::find(said.begin(), said.end(), key), said.end()) << "施工图 3.2 节点 17a：于执事各回一句（" << key << "）";
    }
}

// ===========================================================================
// 施工图 3.1 表末行：制符桌 facility_zhifu「require_flag=ch06.zhifu，9a 之后玩家自己练」
// ===========================================================================
// map_spec 4.6 把 facility 的 require_flag 定成「解锁旗标」；这一条按施工图原文判：9a 之前按制符桌，不该开出制符面板；
// 9a 之后开得出来。走 WorldScene::interact 的真实入口（站到桌前、面朝它按确认）。
TEST_F(Ch06Slice, TheTalismanDeskOnlyOpensAfterNineA) {
    const auto pressDesk = [&]() -> std::string {
        auto loaded = app_.loadMap("ch06_sanxiu_lou", std::string{});
        EXPECT_TRUE(loaded.ok) << loaded.error;
        const MapObject desk = objectOn("ch06_sanxiu_lou", "facility_zhifu");
        EXPECT_FALSE(desk.name.empty());
        // 站在桌子下面一格、面朝上（桌在二楼房间北墙边，下面一格是地板）。
        state().position = fanren::core::Point{desk.position.x, desk.position.y + 1};
        state().facing = 0;
        fanren::game::WorldScene world;
        const bool handled = world.interact(app_);
        app_.tick(1.0 / 60.0);
        std::string top = app_.topScene() == nullptr ? std::string{} : app_.topScene()->name();
        if (handled && app_.topScene() != nullptr) app_.popScene();
        app_.tick(1.0 / 60.0);
        return top;
    };
    state() = GameState{};
    state().setFlag("ch06.jinzhubi");   // 8b 做完、9a 还没做：笔在手上，桌子摆着
    state().addItem("story_jinzhu_bi", 1, 0);
    state().addItem("material_fuzhi", 12, 0);
    state().addItem("material_dansha", 6, 0);
    const std::string before = pressDesk();
    EXPECT_NE(before, "Alchemy") << "施工图 3.1：9a 之前（ch06.zhifu 未置）制符桌不该开出制符面板——开了，玩家就能先把"
                                    "9a 要用的一打符纸、三份丹砂画掉";
    // 拦下时说一句为什么（与闸门拦人同一种提示；校对整改 16.4 第二阶段，引擎补的那一处）。
    EXPECT_EQ(before, "Dialogue") << "拦下时该说一句，而不是默不作声";
    ASSERT_NE(text("ui.facility.locked"), "ui.facility.locked") << "先验：ui.json 里查得到这句（查不到时下一条会空转成恒过）";
    ASSERT_FALSE(app_.dialogueLog().empty());
    EXPECT_EQ(app_.dialogueLog().back().body, text("ui.facility.locked"));
    state().setFlag("ch06.zhifu");
    const std::string after = pressDesk();
    EXPECT_EQ(after, "Alchemy") << "施工图 3.1 / 第 7 节：9a 之后制符桌是可用的设施（facility kind=talisman）";
}

// ===========================================================================
// 药田 facility_yaotian「require_flag=ch06.done」（施工图第 4 节、3.2 节点 18b：灵田开出来给第 7 章）
// ===========================================================================
// 校对整改 16.4 第二阶段·引擎缺口（协调者派工）：map_spec 4.6 的解锁旗标引擎从前不判——18b 之前走到药田边按确认，
// Application::openFacility 就地建出 field_baiyaoyuan、开出种田面板，第 7 章的田在第 6 章当中就有了。
// 现在：旗标还是 0 就不开面板、不建灵田，只说一句 ui.facility.locked；18b 之后照常。走 WorldScene::interact 的真实入口。
TEST_F(Ch06Slice, TheHerbFieldIsNotBuiltBeforeEighteenB) {
    const auto pressField = [&]() -> std::string {
        auto loaded = app_.loadMap("ch06_baiyaoyuan", std::string{});
        EXPECT_TRUE(loaded.ok) << loaded.error;
        const MapObject field = objectOn("ch06_baiyaoyuan", "facility_yaotian");
        EXPECT_FALSE(field.name.empty());
        EXPECT_EQ(field.property("require_flag"), "ch06.done") << "先验：药田认 ch06.done";
        // 站在药田下沿正下方一格、面朝上（田是 3×2 的一块，下面一格是田埂）。
        state().position = fanren::core::Point{field.position.x, field.position.y + std::max(1, field.height)};
        state().facing = 0;
        fanren::game::WorldScene world;
        const bool handled = world.interact(app_);
        app_.tick(1.0 / 60.0);
        std::string top = app_.topScene() == nullptr ? std::string{} : app_.topScene()->name();
        if (handled && app_.topScene() != nullptr) app_.popScene();
        app_.tick(1.0 / 60.0);
        return top;
    };
    state() = GameState{};
    state().setFlag("ch06.renyao");   // 18a 认完药、18b 还没埋瓶
    const std::string before = pressField();
    EXPECT_NE(before, "Field") << "18b 之前按药田，不该开出种田面板";
    EXPECT_EQ(state().findField("field_baiyaoyuan"), nullptr) << "18b 之前按药田，不该就地建出灵田";
    EXPECT_TRUE(state().fields.empty()) << "一块田也不该建";
    EXPECT_EQ(before, "Dialogue") << "拦下时该说一句为什么";
    ASSERT_NE(text("ui.facility.locked"), "ui.facility.locked") << "先验：ui.json 里查得到这句";
    ASSERT_FALSE(app_.dialogueLog().empty());
    EXPECT_EQ(app_.dialogueLog().back().body, text("ui.facility.locked"));
    state().setFlag("ch06.done");
    const std::string after = pressField();
    EXPECT_EQ(after, "Field") << "18b 之后药田是可用的设施";
    const fanren::rules::SpiritField* built = state().findField("field_baiyaoyuan");
    ASSERT_NE(built, nullptr) << "18b 之后按药田：灵田在（章末 field.unlock 之前按也照地图就地建）";
    EXPECT_EQ(built->slots.size(), 6u) << "地图上的 slots=6";
}

}  // namespace
