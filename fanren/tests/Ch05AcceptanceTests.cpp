// 第 5 章验收（docs/ch05-design.md 第 15 节）逐条落地：**不经过驱动、直接读数据、判据写死成施工图原文**。
//
// ---------------------------------------------------------------------------
// 为什么是这个形状（handoff 第 8 节第一条、docs/README.md「判据是从被测物推导出来的」）
// ---------------------------------------------------------------------------
// 通关测试（tests/Ch05SliceTests.cpp）问「走不走得完」；它驱动脚本，脚本一变它就跟着变，
// 所以它只发现得了「变了」，发现不了「错了」。这里每一条都是另一个形状：读 maps/、scripts/、
// data/ 的字，拿施工图原文当判据比。施工图没写死的东西这里不判，写明出来。
//
// 本文件覆盖的验收条目（其余条目的落点见文末那张表）：
//   5 不升境 · 6 禁词（本文件这一份与 Ch05LexiconTests 形状不同，见那一条的注释）· 7 牌子 ·
//   8 前提补种 · 9 天眼与祭剑符 · 10 寒毒常量与「不因寒毒 game_over」· 11 曲魂 · 13 支线 ·
//   14 R-3 提示 · 15 ① 十场必打的挂点链 · 18 分支、地图数、字数
//
// 每一条都配了「故意写坏 → 确实被抓住」的变异（证据在测试路的回复里），另有几条扫描器自检，
// 防「扫描器本身没牙」那一类空转（docs/README.md 那张表的第 2、3 行）。
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
#include <tuple>
#include <utility>
#include <vector>

#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/WorldScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::Application;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch05" / "huanyu.lua") &&
            fs::exists(root / "data" / "objectives" / "ch05.json")) {
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

// 一行去掉行注释之后的正文。本章脚本只有 -- 行注释，没有块注释；字符串里也没有 --。
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

// scripts/ 下所有 .lua（相对 scripts/ 的路径 → 全文）。
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

// 施工图第 4 节的六张新图。
constexpr const char* kChapterMaps[] = {"ch05_dukou", "ch05_xicheng", "ch05_nancheng",
                                         "ch05_kezhan", "ch05_mofu", "ch05_dubashanzhuang"};

class Ch05Acceptance : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        chapterScripts_ = scriptsUnder(root_, "ch05");
        allScripts_ = scriptsUnder(root_, "");
        ASSERT_GE(chapterScripts_.size(), 25u) << "先验：scripts/ch05/ 读得到（分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    MapObject objectOn(const std::string& mapId, const std::string& name) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok || app_.currentMap() == nullptr) return MapObject{};
        for (const MapObject& object : app_.currentMap()->objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }
    // 某一处挂点（或 NPC）挂的脚本，相对 scripts/ 的路径与全文。
    std::pair<std::string, std::string> hookScript(const std::string& mapId, const std::string& name) {
        const MapObject object = objectOn(mapId, name);
        const std::string path = object.property("script");
        const auto it = allScripts_.find(path);
        return {path, it == allScripts_.end() ? std::string{} : it->second};
    }
    std::string text(const std::string& key) { return app_.data().lookupText(key); }

    Application app_;
    std::string root_;
    std::map<std::string, std::string> chapterScripts_;
    std::map<std::string, std::string> allScripts_;
};

// ===========================================================================
// 验收 5：本章不升境、不抬上限
// ===========================================================================
// 判据原文：「扫 scripts/ch05/：realm.advance / realm.cap 0 处」。字面扫全文（注释也算）。
TEST_F(Ch05Acceptance, No5_NoChapterFiveScriptRaisesTheRealmOrTheCap) {
    for (const auto& [path, source] : chapterScripts_) {
        EXPECT_EQ(countOf(source, "realm.advance"), 0) << path << " 写了 realm.advance（施工图 1.3：本章不升境）";
        EXPECT_EQ(countOf(source, "realm.cap"), 0) << path << " 写了 realm.cap（施工图 1.3：本章不抬上限）";
    }
    // 扫描器自检：同一个扫法对第 4 章那三处升境是看得见的。
    int chapterFour = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch04")) chapterFour += countOf(source, "realm.advance");
    EXPECT_EQ(chapterFour, 3) << "扫描器自检：第 4 章有三处 realm.advance（ch04-design 1.2），这里该数得出来";
}

// ===========================================================================
// 验收 6：禁词（施工图 12.1 / 12.2）
// ===========================================================================
// 12.1：LexiconTests 表一含这十一个词，章号照 12.1。读 tests/LexiconTests.cpp 的字——
// 它是协调者的那张表，本条只看它有没有照 12.1 补齐，不改它。
TEST_F(Ch05Acceptance, No6_TheLexiconTableCarriesTheElevenWordsOfSectionTwelveOne) {
    const std::string lexicon = readFile(fs::path(root_) / "tests" / "LexiconTests.cpp");
    ASSERT_GT(lexicon.size(), 1000u) << "先验：读得到 tests/LexiconTests.cpp";
    const std::vector<std::pair<const char*, int>> kWords = {
        {"灵气", 126}, {"炼气", 127}, {"筑基期", 127}, {"真元", 164}, {"黄枫谷", 127}, {"太南小会", 128},
        {"万小山", 126}, {"灵符", 131}, {"坊市", 146}, {"储物袋", 148}, {"定颜丹", 155}};
    for (const auto& [word, chapter] : kWords) {
        const std::string row = std::string("{\"") + word + "\", " + std::to_string(chapter) + ",";
        EXPECT_NE(lexicon.find(row), std::string::npos)
            << "LexiconTests 表一没有「" << word << "」首见 ch" << chapter << " 这一行（施工图 12.1）";
    }
}

// 12.2：「ch05.」文案不含「升仙」「仙令」「天眼」（分母先验 > 150 条）。
// 扫的是**所有文件名带 ch05 的文案** ＋ 其余文件里 key 以 ch05. 开头的（battles.json 里那一条孤儿也在内）。
// 与 Ch05LexiconTests 的分工：那一份按 key 前缀判章内先后（寒毒 9、惊蛟会 4…）；章内先后本测试路另在
// Ch05SliceTests 里**按玩家实际读到的顺序**判（expectGatedWordsArriveWhereTheDesignSays），形状不同。
TEST_F(Ch05Acceptance, No6_NoChapterFiveLineSaysShengxianXianlingOrTianyan) {
    std::vector<std::pair<std::string, std::string>> lines;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "data" / "text")) {
        if (entry.path().extension() != ".json") continue;
        const std::string file = entry.path().filename().string();
        const std::string body = readFile(entry.path());
        const bool chapterFile = file.rfind("ch05", 0) == 0;
        // 逐个 "key": "value" 取出（文案文件是一层的扁平对象）。
        static const std::regex kPair("\"([^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), kPair); it != std::sregex_iterator(); ++it) {
            const std::string key = (*it)[1];
            if (chapterFile || key.rfind("ch05.", 0) == 0) lines.emplace_back(key, (*it)[2]);
        }
    }
    ASSERT_GT(lines.size(), 150u) << "先验：施工图验收 6「分母先验 > 150 条」";
    const std::vector<std::string> kBanned = {"升仙", "仙令", "天眼"};
    // 扫描器自检：同一个扫法对一句坏文案要有牙。
    bool teeth = false;
    for (const std::string& word : kBanned) teeth = teeth || std::string("他想起升仙大会").find(word) != std::string::npos;
    ASSERT_TRUE(teeth) << "扫描器自检：禁词表该抓得住「升仙大会」";
    for (const auto& [key, value] : lines) {
        for (const std::string& word : kBanned) {
            EXPECT_EQ(value.find(word), std::string::npos) << key << " 里有「" << word << "」（施工图 12.2）：" << value;
        }
    }
}

// 12.2 末段：本章给出的物品，描述里不得有 12.1 / 12.2 的禁词；节点 1 给的三样还不得有
// 章内先后晚于节点 1 的那几个词（寒毒 9、惊蛟会 4、五色门 / 独霸山庄 9、太南谷 / 太南山 11）。
TEST_F(Ch05Acceptance, No6_TheItemsThisChapterHandsOutCarryNoWordTheyHaveNotEarned) {
    std::set<std::string> given;
    static const std::regex kGive("give\\(\"([a-z0-9_]+)\"");
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kGive); it != std::sregex_iterator(); ++it) {
            given.insert((*it)[1]);
        }
    }
    ASSERT_GE(given.size(), 5u) << "先验：本章脚本给出的物品读得到（施工图 E6 的五件剧情物品）";
    const std::vector<std::string> kAlways = {"灵气", "炼气", "筑基期", "真元", "黄枫谷", "太南小会", "万小山",
                                              "灵符", "坊市", "储物袋", "定颜丹", "升仙", "仙令", "天眼"};
    const std::vector<std::string> kLater = {"寒毒", "惊蛟会", "五色门", "独霸山庄", "太南谷", "太南山"};
    const std::set<std::string> kNodeOne = {"story_mo_yishu", "story_mo_qinbixin", "story_wenlong_jie"};
    for (const std::string& id : given) {
        const fanren::core::Item* item = app_.data().findItem(id);
        ASSERT_NE(item, nullptr) << id;
        const std::string desc = text(item->descKey);
        for (const std::string& word : kAlways) {
            EXPECT_EQ(desc.find(word), std::string::npos) << id << " 的描述里有「" << word << "」";
            EXPECT_EQ(item->name.find(word), std::string::npos) << id << " 的名字里有「" << word << "」";
        }
        if (!kNodeOne.count(id)) continue;
        ASSERT_NE(desc, item->descKey) << "先验：" << id << " 的描述查得到";
        for (const std::string& word : kLater) {
            EXPECT_EQ(desc.find(word), std::string::npos) << id << " 节点 1 就到手，描述里却有「" << word << "」";
        }
    }
}

// ===========================================================================
// 验收 7：那块牌子没有人提（scripts/ch05/ 里 material_heipai 0 处；整棵树的白名单由
// Ch04PlaqueHasNoName.OnlyTheSpoilsSceneEverTouchesIt 看着）
// ===========================================================================
TEST_F(Ch05Acceptance, No7_NoChapterFiveScriptTouchesThePlaque) {
    for (const auto& [path, source] : chapterScripts_) {
        EXPECT_EQ(countOf(source, "material_heipai"), 0) << path << " 提到了那块牌子（施工图 17 第 11 条）";
    }
    int elsewhere = 0;
    for (const auto& [path, source] : allScripts_) elsewhere += countOf(source, "material_heipai");
    EXPECT_GT(elsewhere, 0) << "扫描器自检：第 4 章战利品那一节给它的那一行该数得出来";
}

// ===========================================================================
// 验收 8：前提补种（读 scripts/ch05/dongqu_lu.lua）
// ===========================================================================
TEST_F(Ch05Acceptance, No8_TheOpeningPlantsTheLetterTheLetterOfProofAndTheRingAndSplitsOnTheHook) {
    const auto [path, source] = hookScript("ch05_dukou", "trigger_dongqu_lu");
    EXPECT_EQ(path, "ch05/dongqu_lu.lua") << "施工图验收 8 点名的就是这个文件";
    const std::string code = codeOnly(source);
    ASSERT_GT(code.size(), 200u) << "先验：读得到 1a 的脚本";
    for (const char* item : {"story_mo_yishu", "story_mo_qinbixin", "story_wenlong_jie"}) {
        EXPECT_EQ(countOf(code, std::string("give(\"") + item + "\""), 1)
            << "1a 该给 " << item << " 正好一次（施工图 3.2 节点 1）";
    }
    EXPECT_NE(code.find("flag.get(\"ch03.jieyao_xuan\")"), std::string::npos)
        << "1a 该按 ch03.jieyao_xuan 分两条（施工图 3.2：「== 2」「== 1」两条都要被通关测试走到）";
    // 起表：1a 记下 ch05.yindu_qi（施工图 3.3）。
    EXPECT_NE(code.find("flag.set(\"ch05.yindu_qi\""), std::string::npos) << "1a 该起表（ch05.yindu_qi）";
}

// ===========================================================================
// 验收 9：本章没有天眼术；「祭剑符」只借一仗
// ===========================================================================
TEST_F(Ch05Acceptance, No9_NoTianyanAndTheSwordTalismanIsLentForOneFightOnly) {
    for (const auto& [path, source] : chapterScripts_) {
        EXPECT_EQ(countOf(source, "magic_tianyan_shu"), 0) << path << "（施工图 1.1 第 14 条：天眼术推到第 6 章）";
    }
    int learns = 0;
    int forgets = 0;
    std::string learnedIn;
    std::string forgottenIn;
    // 只数第 5 章：第 6 章节点 9b（scripts/ch06/kuxiu.lua）起祭剑符常驻，那是另一章的事
    //（docs/ch06-design.md 3.2 节点 9、10.1 E5）。
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        const int l = countOf(code, "magic.learn(\"magic_ji_jianfu\")");
        const int f = countOf(code, "magic.forget(\"magic_ji_jianfu\")");
        learns += l;
        forgets += f;
        if (l > 0) learnedIn = path;
        if (f > 0) forgottenIn = path;
    }
    const auto [shangyue, source] = hookScript("ch05_dubashanzhuang", "trigger_shangyue");
    EXPECT_EQ(shangyue, "ch05/shangyue.lua");
    EXPECT_EQ(learns, 1) << "施工图验收 9：magic.learn(\"magic_ji_jianfu\") 恰 1 处";
    EXPECT_EQ(forgets, 1) << "施工图验收 9：magic.forget(\"magic_ji_jianfu\") 恰 1 处";
    EXPECT_EQ(learnedIn, shangyue) << "同在 shangyue.lua";
    EXPECT_EQ(forgottenIn, shangyue) << "同在 shangyue.lua";

    // data/magics/ji_jianfu.json 与施工图 10.3 相符。
    const fanren::core::Magic* jianfu = app_.data().findMagic("magic_ji_jianfu");
    ASSERT_NE(jianfu, nullptr) << "施工图 10.3：magic_ji_jianfu";
    EXPECT_EQ(jianfu->name, "祭剑符");
    // 施工图 10.3 写的是 element 0（无属性）。八方旅人化改造把它改成 1（金）：法术这一击的
    // 攻击类别取自五行，无属性的法术打不出任何破绽；祭剑符祭的是飞剑，归金最顺。
    // 欧阳飞天本身无属性，五行相克不起作用，下面那一招取首级的算术一个数也不变
    //（docs/octopath-battle.md 第 8 节）。
    EXPECT_EQ(jianfu->element, 1);
    EXPECT_EQ(jianfu->needMp, 40);
    EXPECT_EQ(jianfu->power, 40);
    // 施工图 10.3 的 castRange 3（开场距离正好够得着）随横版改造删了：没有距离。
    EXPECT_EQ(jianfu->needRealm, fanren::rules::Realm::QiRefining1);
    // (40×2−32×0.5)×1.6 ≥ 90：按 8.3 欧阳飞天的防与血、韩立炼气八层，用战斗公式重算。
    const fanren::core::RoleTemplate* ouyang = app_.data().findRole("ouyang_feitian");
    ASSERT_NE(ouyang, nullptr);
    ASSERT_EQ(ouyang->defence, 32) << "施工图 8.3：欧阳飞天防 32";
    ASSERT_EQ(ouyang->maxHp, 90) << "施工图 8.3：欧阳飞天血 90";
    const int blow = fanren::core::battle::magicDamage(jianfu->power, ouyang->defence,
                                                       fanren::rules::Realm::QiRefining8, ouyang->realm,
                                                       jianfu->element, ouyang->element);
    EXPECT_GE(blow, ouyang->maxHp) << "祭剑符一下 " << blow << "，取不了首级（施工图 8.3：一招取首级，与 ch126 对得上）";
    EXPECT_LE(jianfu->needMp, fanren::rules::realmMaxMp(fanren::rules::Realm::QiRefining8))
        << "炼气八层的法力放不出这一招";
}

// ===========================================================================
// 验收 10：寒毒倒计时——六处写同一组常量（五个检查点 ＋ 内视）；没有一条因寒毒而 game_over 的路
// ===========================================================================
// 检查点照施工图 3.3：10a、11c、12b、12c、12e 的脚本开头；内视只说不扣，但施工偏差 18.3 第 7 条
// 说它「也写了同一组数」，这里一并读（共六处）。脚本按 3.1 表的挂点名从地图上找，不写文件名。
TEST_F(Ch05Acceptance, No10_SixPlacesNameTheSameColdPoisonStagesAndTheyAreTheDesignsNumbers) {
    const std::vector<std::pair<const char*, const char*>> kPlaces = {
        {"ch05_kezhan", "trigger_dingji"},         {"ch05_kezhan", "trigger_anpai"},
        {"ch05_mofu", "trigger_zhuwu"},            {"ch05_dubashanzhuang", "trigger_tancha"},
        {"ch05_mofu", "trigger_huanyu"},           {"ch05_kezhan", "trigger_neishi"},
    };
    static const std::regex kConst("^local\\s+([A-Z][A-Z0-9_]*)\\s*=\\s*([0-9]+)\\b");
    std::map<std::string, int> first;
    std::string firstPath;
    for (const auto& [mapId, object] : kPlaces) {
        const auto [path, source] = hookScript(mapId, object);
        ASSERT_FALSE(source.empty()) << mapId << " / " << object << " 挂的脚本读不到";
        std::map<std::string, int> constants;
        for (const std::string& line : linesOf(source)) {
            std::smatch m;
            const std::string code = codeOf(line);
            if (std::regex_search(code, m, kConst)) constants[m[1]] = std::stoi(m[2]);
        }
        std::set<int> values;
        for (const auto& [name, value] : constants) values.insert(value);
        EXPECT_TRUE(values.count(65)) << path << "：施工图 3.3 第二段从 d = 65 起，首部常量里没有 65";
        EXPECT_TRUE(values.count(80)) << path << "：施工图 3.3 第三段从 d = 80 起，首部常量里没有 80";
        EXPECT_TRUE(values.count(90)) << path << "：施工图 3.3 期限 d = 90，首部常量里没有 90";
        if (firstPath.empty()) {
            first = constants;
            firstPath = path;
        } else {
            EXPECT_EQ(constants, first) << path << " 与 " << firstPath << " 的常量不一致（施工图 17 第 10 条：改一处就改五处）";
        }
    }
}

// 「全章没有一条因寒毒而 game_over 的路（扫脚本）」：scripts/ch05/ 里每一处 game_over() 都排在同一个
// 脚本的 battle( 之后（只有打输了才 game_over）；没有 battle( 的脚本一处 game_over 也不许有。
// 寒毒检查点在 zhuwu / tancha 里排在 battle( 之前，所以写进检查点的 game_over 也会被这一条抓住。
TEST_F(Ch05Acceptance, No10_EveryGameOverInTheChapterFollowsALostFightNeverThePoison) {
    int gameOvers = 0;
    for (const auto& [path, source] : chapterScripts_) {
        int firstBattle = -1;
        int lineNo = 0;
        for (const std::string& line : linesOf(source)) {
            ++lineNo;
            const std::string code = codeOf(line);
            if (firstBattle < 0 && code.find("battle(") != std::string::npos) firstBattle = lineNo;
            if (code.find("game_over(") == std::string::npos) continue;
            ++gameOvers;
            EXPECT_TRUE(firstBattle > 0 && lineNo > firstBattle)
                << path << " 第 " << lineNo << " 行的 game_over 不在一场仗的后面——寒毒超期不许 game over"
                << "（施工图 3.3、16.1 第 1 条）";
        }
    }
    EXPECT_GT(gameOvers, 0) << "扫描器自检：本章打输即 game_over 的那几处该数得出来";
}

// 两条施工图写死的「不许」：
//   · 10c：「这一仗韩立在睡觉，不该让他死在里头（defeat_is_fatal=false）」——夺帮那一节的脚本里一处 game_over 也没有；
//   · 12e：「不调 teleport()、不调 ending()」——章末不换图、不收结局，第 6 章从哪儿开局归关卡侧与主控。
TEST_F(Ch05Acceptance, No10_TheNightOfTheTakeoverCannotEndTheGameAndTheChapterEndNeitherMovesNorEnds) {
    const auto [duobangPath, duobang] = hookScript("ch05_kezhan", "trigger_shuijiao");
    ASSERT_FALSE(duobang.empty()) << "10c 的脚本读不到";
    EXPECT_NE(codeOnly(duobang).find("battle(\"b05_duobang\")"), std::string::npos) << "先验：这就是夺帮那一节";
    EXPECT_EQ(countOf(codeOnly(duobang), "game_over("), 0) << duobangPath << "：韩立在睡觉，不许 game over（施工图 3.2 10c）";
    const auto [huanyuPath, huanyu] = hookScript("ch05_mofu", "trigger_huanyu");
    ASSERT_FALSE(huanyu.empty()) << "12e 的脚本读不到";
    EXPECT_NE(codeOnly(huanyu).find("flag.set(\"ch05.done\")"), std::string::npos) << "先验：这就是章末那一节";
    EXPECT_EQ(countOf(codeOnly(huanyu), "teleport("), 0) << huanyuPath << "：施工图 3.2 12e「不调 teleport()」";
    EXPECT_EQ(countOf(codeOnly(huanyu), "ending("), 0) << huanyuPath << "：施工图 3.2 12e「不调 ending()」";
}

// ===========================================================================
// 验收 11：曲魂留下
// ===========================================================================
TEST_F(Ch05Acceptance, No11_QuhunLeavesTheTeamOnceAndNeverComesBack) {
    int removes = 0;
    int adds = 0;
    std::string removedIn;
    for (const auto& [path, source] : chapterScripts_) {
        const std::string code = codeOnly(source);
        const int r = countOf(code, "party.remove(\"qu_hun\")");
        removes += r;
        adds += countOf(code, "party.add(\"qu_hun\")");
        if (r > 0) removedIn = path;
    }
    const auto [dingji, source] = hookScript("ch05_kezhan", "trigger_dingji");
    EXPECT_EQ(dingji, "ch05/dingji.lua");
    EXPECT_EQ(removes, 1) << "施工图验收 11：party.remove(\"qu_hun\") 恰 1 处";
    EXPECT_EQ(removedIn, dingji) << "施工图验收 11：在 dingji.lua";
    EXPECT_EQ(adds, 0) << "施工图验收 11：party.add(\"qu_hun\") 0 处";
    const fanren::core::BattleSetup* duobang = app_.battleSetup("b05_duobang");
    ASSERT_NE(duobang, nullptr);
    bool quhunAsAlly = false;
    for (const auto& unit : duobang->units) quhunAsAlly = quhunAsAlly || (unit.roleId == "qu_hun" && unit.ally);
    EXPECT_TRUE(quhunAsAlly) << "施工图验收 11：b05_duobang.json 里 qu_hun 以 ally 出场";
    int anyAdd = 0;
    for (const auto& [path, source2] : allScripts_) anyAdd += countOf(codeOnly(source2), "party.add(\"qu_hun\")");
    EXPECT_GT(anyAdd, 0) << "扫描器自检：第 3 章曲魂入队那一处该数得出来";
}

// ===========================================================================
// 验收 13：支线（读 data/quests/，谓词与施工图第 6 节字面量相符；rewards 与脚本 give 相符）
// ===========================================================================
// 任务 id 由内容路定（契约 1.2），这里按施工图第 6 节「触发」那一行的旗标认出是哪一条。
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

TEST_F(Ch05Acceptance, No13_TheThreeSideQuestsReadTheFlagsSectionSixNames) {
    using Op = fanren::core::QuestCondition::Op;
    const fanren::core::Quest* z1 = nullptr;
    const fanren::core::Quest* z2 = nullptr;
    const fanren::core::Quest* z3 = nullptr;
    int chapterFive = 0;
    for (const fanren::core::Quest& q : app_.data().quests) {
        if (q.kind != fanren::core::QuestKind::Side || q.chapter != 5) continue;
        ++chapterFive;
        if (samePreds(q.accept, {{"ch05.fengwu_qiu", Op::FlagAtLeast, 1}})) z1 = &q;
        if (samePreds(q.accept, {{"ch05.jianfu_qiu", Op::FlagAtLeast, 1}})) z2 = &q;
        if (samePreds(q.accept, {{"ch05.qingling_qiu", Op::FlagAtLeast, 1}})) z3 = &q;
    }
    EXPECT_EQ(chapterFive, 3) << "施工图第 6 节：支线 3 条，不凑数";
    ASSERT_NE(z1, nullptr) << "Z1（接：ch05.fengwu_qiu = 1）不在 data/quests";
    ASSERT_NE(z2, nullptr) << "Z2（接：ch05.jianfu_qiu = 1）不在 data/quests";
    ASSERT_NE(z3, nullptr) << "Z3（接：ch05.qingling_qiu = 1）不在 data/quests";

    // Z1：完成 ch05.fengwu_yigao = 1；过期 ch05.done 置位时未交；两步：誊抄（客栈书案）→ 药圃交给她。
    EXPECT_TRUE(samePreds(z1->complete, {{"ch05.fengwu_yigao", Op::FlagAtLeast, 1}})) << "Z1 完成";
    EXPECT_TRUE(samePreds(z1->fail, {{"ch05.done", Op::FlagAtLeast, 1}})) << "Z1 过期";
    ASSERT_EQ(z1->steps.size(), 2u) << "Z1 两步";
    EXPECT_TRUE(samePreds(z1->steps[0].done, {{"ch05.fengwu_chao", Op::FlagAtLeast, 1}}));
    EXPECT_EQ(z1->steps[0].targetMap, "ch05_kezhan");
    EXPECT_EQ(z1->steps[0].targetObject, "trigger_chaoxie");
    EXPECT_TRUE(samePreds(z1->steps[1].done, {{"ch05.fengwu_yigao", Op::FlagAtLeast, 1}}));
    EXPECT_EQ(z1->steps[1].targetMap, "ch05_mofu");
    EXPECT_EQ(z1->steps[1].targetObject, "npc_mo_fengwu");
    // 奖励：黄精 6、紫参 3（40 年），与交付那一节脚本里的 give 一一相符。
    std::multiset<std::tuple<std::string, int, int>> quest;
    for (const auto& r : z1->rewards) quest.insert({r.itemId, r.count, r.herbAge});
    const std::multiset<std::tuple<std::string, int, int>> design = {{"herb_huangjing_cao", 6, 40},
                                                                     {"herb_zishen_cao", 3, 40}};
    EXPECT_EQ(quest, design) << "Z1 的 rewards 与施工图第 6 节（黄精 6、紫参 3，40 年）不符";
    const auto [fengwuPath, fengwu] = hookScript("ch05_mofu", "npc_mo_fengwu");
    std::multiset<std::tuple<std::string, int, int>> scripted;
    static const std::regex kGive("give\\(\"([a-z0-9_]+)\",\\s*([0-9]+)(?:,\\s*([0-9]+))?\\)");
    const std::string fengwuCode = codeOnly(fengwu);
    for (auto it = std::sregex_iterator(fengwuCode.begin(), fengwuCode.end(), kGive); it != std::sregex_iterator(); ++it) {
        scripted.insert({(*it)[1], std::stoi((*it)[2]), (*it)[3].matched ? std::stoi((*it)[3]) : 0});
    }
    EXPECT_EQ(scripted, design) << fengwuPath << " 交付时 give 的与施工图第 6 节不符（验收 13：rewards 与脚本的 give 相符）";

    // Z2：完成 ch05.lian_jianfu = 1；**不会过期**（施工图第 6 节，16.1 第 13 条之后剑符是取首级的唯一办法，
    // ch05.cisha 置得上就必然练成了）；回报是一招，不进 rewards。
    EXPECT_TRUE(samePreds(z2->complete, {{"ch05.lian_jianfu", Op::FlagAtLeast, 1}})) << "Z2 完成";
    EXPECT_TRUE(z2->fail.empty()) << "Z2 不该再有过期条件：火弹杀不了欧阳飞天";
    EXPECT_TRUE(z2->failTextKey.empty()) << "Z2 不会过期，就不该挂一句过期原因";
    EXPECT_TRUE(z2->rewards.empty()) << "Z2 的回报是 12d 多一招，不是物品（契约 1.2 奖励表）";

    // Z3：完成 ch05.xiaoxiang == 1；过期 ch05.xiaoxiang == 2；第一步是身上备一份清灵散。
    EXPECT_TRUE(samePreds(z3->complete, {{"ch05.xiaoxiang", Op::FlagEquals, 1}})) << "Z3 完成";
    EXPECT_TRUE(samePreds(z3->fail, {{"ch05.xiaoxiang", Op::FlagEquals, 2}})) << "Z3 过期";
    ASSERT_FALSE(z3->steps.empty());
    EXPECT_TRUE(samePreds(z3->steps[0].done, {{"pill_qingling_san", Op::ItemAtLeast, 1}}))
        << "Z3 第一步：身上备一份清灵散（施工图第 6 节）";
    EXPECT_TRUE(z3->rewards.empty()) << "Z3 的回报是潇湘院不开战斗，不是物品";

    // 过期都要有一句原因（契约 Q7）。Z2 不会过期，不在这一圈里。
    for (const fanren::core::Quest* q : {z1, z3}) {
        EXPECT_FALSE(q->failTextKey.empty()) << q->id;
        EXPECT_NE(text(q->failTextKey), q->failTextKey) << q->id << " 的过期原因查不到";
    }

    // 前置（施工图第 6 节「前置」一行）：Z1、Z2 在 ch05.jiaoyi 那一节挂起；Z3 在 ch05.dingji 那一节。
    // 做法：置接取旗标的那个脚本，也就是置前置旗标的那个脚本（同一节里同时置下）。
    const auto setterOf = [&](const std::string& flagName) {
        std::vector<std::string> setters;
        for (const auto& [path, source] : allScripts_) {
            if (codeOnly(source).find("flag.set(\"" + flagName + "\"") != std::string::npos) setters.push_back(path);
        }
        return setters;
    };
    for (const auto& [acceptFlag, prerequisite] : std::vector<std::pair<std::string, std::string>>{
             {"ch05.fengwu_qiu", "ch05.jiaoyi"}, {"ch05.jianfu_qiu", "ch05.jiaoyi"}, {"ch05.qingling_qiu", "ch05.dingji"}}) {
        const std::vector<std::string> a = setterOf(acceptFlag);
        const std::vector<std::string> b = setterOf(prerequisite);
        ASSERT_EQ(a.size(), 1u) << acceptFlag << " 该由且只由一个脚本置";
        ASSERT_EQ(b.size(), 1u) << prerequisite << " 该由且只由一个脚本置";
        EXPECT_EQ(a[0], b[0]) << acceptFlag << " 不在 " << prerequisite << " 那一节挂起（施工图第 6 节「前置」）";
    }
}

// ===========================================================================
// 验收 14：R-3 提示在前——目标链第 16、21、23 步的文案 key 存在且分别含「清灵散」「药」「剑符」
// ===========================================================================
// 顺带把施工图 3.4 那张目标链表整张逐格判卷（id、done_flag、target_map、target_object 与顺序）。
TEST_F(Ch05Acceptance, No14_TheObjectiveChainIsTableThreeFourAndItsHintsNameTheRemedy) {
    struct StepSpec {
        const char* id;
        const char* doneFlag;
        const char* map;
        const char* object;
    };
    const std::vector<StepSpec> kChain = {
        {"n01_dongqu", "ch05.kaipian", "ch05_dukou", "trigger_dongqu_lu"},
        {"n02_shangchuan", "ch05.shangchuan", "ch05_dukou", "trigger_shangchuan"},
        {"n03_matou", "ch05.matou", "ch05_xicheng", "trigger_matou"},
        {"n04_heishuixiang", "ch05.shoufu", "ch05_xicheng", "trigger_heishuixiang"},
        {"n05_zhuishao", "ch05.zhuishao", "ch05_xicheng", "trigger_zhuishao"},
        {"n06_qingbao", "ch05.qingbao", "ch05_kezhan", "trigger_qingbao"},
        {"n07_jieren", "ch05.jieren", "ch05_xicheng", "trigger_jieren"},
        {"n08_jiulou", "ch05.jiulou", "ch05_nancheng", "trigger_jiulou"},
        {"n09_yeru", "ch05.yeru", "ch05_nancheng", "trigger_yeru"},
        {"n10_toutin", "ch05.toutin", "ch05_mofu", "trigger_toutin"},
        {"n11_dengmen", "ch05.dengmen", "ch05_mofu", "trigger_dengmen"},
        {"n12_jianmianli", "ch05.jianmianli", "ch05_mofu", "trigger_jianmianli"},
        {"n13_huayuan", "ch05.huayuan", "ch05_mofu", "trigger_huayuan"},
        {"n14_duizhi", "ch05.duizhi", "ch05_mofu", "trigger_duizhi"},
        {"n15_dingji", "ch05.dingji", "ch05_kezhan", "trigger_dingji"},
        {"n16_xiaoxiang", "ch05.xiaoxiang", "ch05_xicheng", "trigger_xiaoxiangyuan"},
        {"n17_duobang", "ch05.duobang", "ch05_kezhan", "trigger_shuijiao"},
        {"n18_jiaoyi", "ch05.jiaoyi", "ch05_mofu", "trigger_jiaoyi"},
        {"n19_yange", "ch05.yange", "ch05_mofu", "trigger_yange"},
        {"n20_anpai", "ch05.anpai", "ch05_kezhan", "trigger_anpai"},
        {"n21_majiu", "ch05.shigui", "ch05_mofu", "trigger_majiu"},
        {"n22_zhuwu", "ch05.chuzheng", "ch05_mofu", "trigger_zhuwu"},
        {"n23_tancha", "ch05.tancha", "ch05_dubashanzhuang", "trigger_tancha"},
        {"n24_cisha", "ch05.cisha", "ch05_dubashanzhuang", "trigger_shangyue"},
        {"n25_huanyu", "ch05.done", "ch05_mofu", "trigger_huanyu"},
    };
    std::vector<const fanren::core::Objective*> chain;
    for (const fanren::core::Objective& o : app_.data().objectives) {
        if (o.chapter == 5) chain.push_back(&o);
    }
    ASSERT_EQ(chain.size(), kChain.size()) << "施工图 3.4：第 5 章目标链 25 步";
    for (std::size_t i = 0; i < kChain.size(); ++i) {
        EXPECT_EQ(chain[i]->id, kChain[i].id) << "第 " << (i + 1) << " 步";
        EXPECT_EQ(chain[i]->doneFlag, kChain[i].doneFlag) << kChain[i].id;
        EXPECT_EQ(chain[i]->targetMap, kChain[i].map) << kChain[i].id;
        EXPECT_EQ(chain[i]->targetObject, kChain[i].object) << kChain[i].id;
        EXPECT_NE(text(chain[i]->textKey), chain[i]->textKey) << kChain[i].id << " 的文案查不到";
    }
    const auto lineOf = [&](std::size_t number) { return text(chain[number - 1]->textKey); };
    EXPECT_NE(lineOf(16).find("清灵散"), std::string::npos) << "施工图验收 14：第 16 步含「清灵散」：" << lineOf(16);
    EXPECT_NE(lineOf(21).find("药"), std::string::npos) << "施工图验收 14：第 21 步含「药」：" << lineOf(21);
    EXPECT_NE(lineOf(23).find("剑符"), std::string::npos) << "施工图验收 14：第 23 步含「剑符」：" << lineOf(23);
    // 3.4 表第 8 步：写明阴毒「顶多两个月」。
    EXPECT_NE(lineOf(8).find("顶多"), std::string::npos) << "施工图 3.4 第 8 步：「顶多两个月」：" << lineOf(8);
    EXPECT_NE(lineOf(8).find("两个月"), std::string::npos) << "施工图 3.4 第 8 步：「顶多两个月」：" << lineOf(8);
}

// ===========================================================================
// 验收 15 ①：十场必打一场绕不过去——直接读数据
// ===========================================================================
// 判据原文：「8.0 表里的 10 个编成 id 各自只被一个脚本调用，那个脚本挂在 3.1 表标了战斗的挂点上，
// 挂点的 set_flag 出现在 data/objectives/ch05.json 的 done_flag 序列里，且是下一节点挂点的 guard_flag」；
// 施工图 17 第 12 条：「每一场必打战斗的挂点前面都要有存档点」。
// 另加两条这张判据背后的意思（「必打」靠这条链守住）：
//   · 那场仗在脚本里是**无条件**的一句（顶格写，不在任何 if 里），置完成旗标的每一句都排在它后面；
//   · 完成旗标只有这一个脚本置（别处置了，这一节就能被跳过）。
struct MustFightHook {
    const char* mark;
    const char* battleId;
    const char* mapId;
    const char* hook;
    const char* nextMap;
    const char* nextHook;
};

const std::vector<MustFightHook>& mustFightHooks() {
    // 施工图 3.1 表「战斗」一列 ＋ 3.4 目标链的顺序（下一节点）。
    static const std::vector<MustFightHook> kHooks = {
        {"①", "b05_yesu_yelang", "ch05_dukou", "trigger_dongqu_lu", "ch05_dukou", "trigger_shangchuan"},
        {"②", "b05_heishuixiang", "ch05_xicheng", "trigger_heishuixiang", "ch05_xicheng", "trigger_zhuishao"},
        {"③", "b05_matou_zhuishao", "ch05_xicheng", "trigger_zhuishao", "ch05_kezhan", "trigger_qingbao"},
        {"④", "b05_tiequanhui_jieren", "ch05_xicheng", "trigger_jieren", "ch05_nancheng", "trigger_jiulou"},
        {"⑤", "b05_duobang", "ch05_kezhan", "trigger_shuijiao", "ch05_mofu", "trigger_jiaoyi"},
        {"⑥", "b05_yange_qiecuo", "ch05_mofu", "trigger_yange", "ch05_kezhan", "trigger_anpai"},
        {"⑦", "b05_mofu_shigui", "ch05_mofu", "trigger_majiu", "ch05_mofu", "trigger_zhuwu"},
        {"⑧", "b05_wu_jianming", "ch05_mofu", "trigger_zhuwu", "ch05_dubashanzhuang", "trigger_tancha"},
        {"⑨", "b05_xunzhuang", "ch05_dubashanzhuang", "trigger_tancha", "ch05_dubashanzhuang", "trigger_shangyue"},
        {"⑩", "b05_ouyang_feitian", "ch05_dubashanzhuang", "trigger_shangyue", "ch05_mofu", "trigger_huanyu"},
    };
    return kHooks;
}

TEST_F(Ch05Acceptance, No15_EachOfTheTenMustFightsIsCalledOnceFromItsHookAndGuardsTheNextNode) {
    ASSERT_EQ(mustFightHooks().size(), 10u) << "施工图 8.0：必打 10 场";
    std::vector<std::string> doneFlags;
    std::vector<std::string> targets;
    for (const fanren::core::Objective& o : app_.data().objectives) {
        if (o.chapter != 5) continue;
        doneFlags.push_back(o.doneFlag);
        targets.push_back(o.targetObject);
    }
    ASSERT_EQ(doneFlags.size(), 25u) << "先验：第 5 章目标链读得到";

    for (const MustFightHook& h : mustFightHooks()) {
        // 各自只被一个脚本调用。
        std::vector<std::string> callers;
        const std::string call = std::string("battle(\"") + h.battleId + "\")";
        for (const auto& [path, source] : allScripts_) {
            if (codeOnly(source).find(call) != std::string::npos) callers.push_back(path);
        }
        ASSERT_EQ(callers.size(), 1u) << h.mark << " " << h.battleId << " 该只被一个脚本调用";
        // 那个脚本挂在 3.1 表标了这场战斗的挂点上。
        const MapObject hook = objectOn(h.mapId, h.hook);
        ASSERT_FALSE(hook.name.empty()) << h.mapId << " 上没有 " << h.hook;
        EXPECT_EQ(hook.property("script"), callers[0]) << h.mark << "：" << h.battleId << " 不是从 " << h.hook << " 打的";
        const std::string setFlag = hook.property("set_flag");
        ASSERT_FALSE(setFlag.empty()) << h.hook << " 没写 set_flag";
        // 挂点的 set_flag 在目标链的 done_flag 序列里，且下一节点的挂点认它。
        const auto at = std::find(doneFlags.begin(), doneFlags.end(), setFlag);
        ASSERT_NE(at, doneFlags.end()) << h.mark << "：" << setFlag << " 不在目标链的 done_flag 序列里";
        const std::size_t index = static_cast<std::size_t>(at - doneFlags.begin());
        EXPECT_EQ(targets[index], h.hook) << h.mark << "：目标链指着别的挂点";
        ASSERT_LT(index + 1, targets.size()) << h.mark << " 后面没有下一步了";
        EXPECT_EQ(targets[index + 1], h.nextHook) << h.mark << "：目标链的下一步不是施工图 3.4 的下一节点";
        const MapObject next = objectOn(h.nextMap, h.nextHook);
        ASSERT_FALSE(next.name.empty()) << h.nextMap << " 上没有 " << h.nextHook;
        EXPECT_EQ(next.property("guard_flag"), setFlag)
            << h.mark << "：下一节点 " << h.nextHook << " 的 guard 不认 " << setFlag << "——这一场绕得过去";

        // 那场仗是无条件的一句，置完成旗标的每一句都在它后面；完成旗标只有这一个脚本置。
        const std::vector<std::string> lines = linesOf(allScripts_[callers[0]]);
        int battleLine = -1;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const std::string code = codeOf(lines[i]);
            if (code.find(call) == std::string::npos) continue;
            battleLine = static_cast<int>(i);
            EXPECT_TRUE(!code.empty() && code[0] != ' ' && code[0] != '\t')
                << callers[0] << " 第 " << (i + 1) << " 行：那一场不是顶格的一句——套在某个分支里，就有一条路不打它";
        }
        ASSERT_GE(battleLine, 0);
        const std::string setter = "flag.set(\"" + setFlag + "\"";
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (codeOf(lines[i]).find(setter) == std::string::npos) continue;
            EXPECT_GT(static_cast<int>(i), battleLine)
                << callers[0] << " 第 " << (i + 1) << " 行：在打 " << h.battleId << " 之前就置了 " << setFlag;
        }
        for (const auto& [path, source] : allScripts_) {
            if (path == callers[0]) continue;
            EXPECT_EQ(codeOnly(source).find(setter), std::string::npos)
                << path << " 也置 " << setFlag << "：" << h.mark << " 那一节能被它跳过去";
        }
    }
}

// 施工图 17 第 12 条：「每一场必打战斗的挂点前面都要有存档点」。
// 口径：从那张图的默认出生点出发（六张图的默认出生点都是这一章进图的那一格），
// 不踩本图任何一处必打战斗的踏入型挂点，走得到一个存档设施（facility kind=save）的旁边。
// 静止的 NPC（不带 visible / hidden 旗标的）挡路；设施与门不能踩。
TEST_F(Ch05Acceptance, No15_EveryMustFightHookHasASavePointReachableBeforeIt) {
    for (const char* mapId : kChapterMaps) {
        auto loaded = app_.loadMap(mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const TileMap& map = *app_.currentMap();
        std::set<std::pair<int, int>> blocked;
        bool hasMustFight = false;
        Point spawn{-1, -1};
        std::vector<const MapObject*> saves;
        for (const MapObject& object : map.objects) {
            const auto cells = [&](const std::function<void(int, int)>& f) {
                for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                    for (int dx = 0; dx < std::max(1, object.width); ++dx) f(object.position.x + dx, object.position.y + dy);
                }
            };
            if (object.type == "spawn" && object.property("default") == "true") spawn = object.position;
            if (object.type == "facility" || object.type == "portal") cells([&](int x, int y) { blocked.insert({x, y}); });
            if (object.type == "npc" && object.property("visible_flag").empty() && object.property("hidden_flag").empty()) {
                cells([&](int x, int y) { blocked.insert({x, y}); });
            }
            if (object.type == "facility" && object.property("kind") == "save") saves.push_back(&object);
            for (const MustFightHook& h : mustFightHooks()) {
                if (std::string(h.mapId) != mapId || object.name != h.hook) continue;
                hasMustFight = true;
                if (object.property("mode") == "enter") cells([&](int x, int y) { blocked.insert({x, y}); });
            }
        }
        if (!hasMustFight) continue;
        ASSERT_GE(spawn.x, 0) << mapId << " 没有默认出生点";
        ASSERT_FALSE(saves.empty()) << mapId << " 挂着必打战斗，图上却没有存档点（施工图 17 第 12 条）";
        std::set<std::pair<int, int>> seen{{spawn.x, spawn.y}};
        std::deque<Point> queue{spawn};
        while (!queue.empty()) {
            const Point p = queue.front();
            queue.pop_front();
            for (const Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
                const Point n{p.x + d.x, p.y + d.y};
                if (!map.walkable(n) || blocked.count({n.x, n.y}) || seen.count({n.x, n.y})) continue;
                seen.insert({n.x, n.y});
                queue.push_back(n);
            }
        }
        bool reachable = false;
        for (const MapObject* save : saves) {
            for (const Point d : {Point{1, 0}, Point{-1, 0}, Point{0, 1}, Point{0, -1}}) {
                reachable = reachable || seen.count({save->position.x + d.x, save->position.y + d.y}) > 0;
            }
        }
        EXPECT_TRUE(reachable) << mapId << "：从进图那一格出发，不踩必打的挂点就走不到存档点（施工图 17 第 12 条）";
    }
}

// ===========================================================================
// 验收 18：主线节点、分支、新地图、字数（与 tools/audit.py 的数对照写在测试路的回复里）
// ===========================================================================
TEST_F(Ch05Acceptance, No18_BranchesMapsAndWordsAreWhereSectionFifteenPutsThem) {
    // 分支 ≥ 8 处：扫 scripts/ch05/ 的 choice{。
    int choices = 0;
    for (const auto& [path, source] : chapterScripts_) {
        choices += countOf(codeOnly(source), "choice{") + countOf(codeOnly(source), "choice {");
    }
    EXPECT_GE(choices, 8) << "施工图验收 18：分支 ≥ 8 处";
    // 更细一层：3.1 表里施工图 3.2 写了选择的十处挂点，脚本里各有一处 choice；写了「无选择」的两处没有。
    const std::vector<std::pair<const char*, const char*>> kWithChoice = {
        {"ch05_xicheng", "trigger_heishuixiang"}, {"ch05_kezhan", "trigger_qingbao"},
        {"ch05_nancheng", "trigger_yeru"},        {"ch05_mofu", "trigger_toutin"},
        {"ch05_mofu", "trigger_dengmen"},         {"ch05_mofu", "trigger_jianmianli"},
        {"ch05_mofu", "trigger_huayuan"},         {"ch05_mofu", "trigger_duizhi"},
        {"ch05_mofu", "trigger_jiaoyi"},          {"ch05_kezhan", "trigger_anpai"}};
    for (const auto& [mapId, hook] : kWithChoice) {
        const auto [path, source] = hookScript(mapId, hook);
        EXPECT_GT(countOf(codeOnly(source), "choice{"), 0) << hook << "（" << path << "）：施工图 3.1 的主动分支在这里";
    }
    for (const auto& [mapId, hook] : std::vector<std::pair<const char*, const char*>>{
             {"ch05_xicheng", "trigger_matou"}, {"ch05_nancheng", "trigger_jiulou"}}) {
        const auto [path, source] = hookScript(mapId, hook);
        ASSERT_FALSE(source.empty()) << hook;
        EXPECT_EQ(countOf(codeOnly(source), "choice{"), 0) << hook << "：施工图 3.2 写的是「无选择」";
    }
    // 新地图 6 张。
    int maps = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "maps")) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("ch05_", 0) == 0 && entry.path().extension() == ".tmj") ++maps;
    }
    EXPECT_EQ(maps, 6) << "施工图第 4 节 / 验收 18：新地图 6 张";
    // 主线对白约 10500 字（第 14 节按 audit.py 的口径：key 以 ch05. 开头的文案，只数汉字）。
    // 「约」没有给容差，这里只防塌：少于施工图的八成就报；实数打印出来，与施工图的对照写在回复里。
    int chars = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "data" / "text")) {
        if (entry.path().filename().string().rfind("ch05", 0) != 0) continue;
        const std::string body = readFile(entry.path());
        static const std::regex kPair("\"(ch05\\.[^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), kPair); it != std::sregex_iterator(); ++it) {
            const std::string value = (*it)[2];
            for (std::size_t i = 0; i < value.size();) {
                const unsigned char c = static_cast<unsigned char>(value[i]);
                if (c >= 0xE0 && c < 0xF0 && i + 2 < value.size()) {
                    const unsigned cp = ((c & 0x0Fu) << 12) | ((static_cast<unsigned char>(value[i + 1]) & 0x3Fu) << 6) |
                                        (static_cast<unsigned char>(value[i + 2]) & 0x3Fu);
                    if ((cp >= 0x3400 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF)) ++chars;
                    i += 3;
                } else {
                    i += c < 0x80 ? 1 : (c < 0xE0 ? 2 : 4);
                }
            }
        }
    }
    std::cout << "[ch05 字数] ch05.* 文案汉字 " << chars << "（施工图第 3 / 14 节：约 10500）" << std::endl;
    EXPECT_GE(chars, 10500 * 8 / 10) << "主线对白比施工图的「约 10500 字」少了两成以上";
}

// ---------------------------------------------------------------------------
// 验收第 15 节各条的落点（不在本文件的那几条）
// ---------------------------------------------------------------------------
//   1 门禁：build.bat 的三道门禁本身（genmaps --check 覆盖 genmaps_ch05.py 与韩家村修补）。不写 C++ 断言。
//   2 通关：Ch05SliceTests（起点读 ch04-end-*.sav，SetUp 与终点的先验逐条抄验收 2）。
//   3 挂点：Ch05TriggerModeTests。
//   4 编成：Ch05BattleDataTests。
//   12 经济账：Ch05LedgerTests。
//   15 ② 走一遍：Ch05SliceTests（两侧 × 四种备法记下实际打到的编成；「试着绕过去」那一条）。
//   16 韩立不在场：数据侧 Ch05BattleDataTests（hero_absent 只有 ⑤）；负向加载 Ch05HeroAbsentTests（引擎路）；
//      ⑤ 开场没有 hanli、战后气血法力不动在 Ch05SliceTests 第一侧通关里。
//   17 意图玩家：Ch05SliceTests 的 Ch05HandSweep / Ch05LeastPills。
//   19 独立校对：人工，不是测试。

}  // namespace
