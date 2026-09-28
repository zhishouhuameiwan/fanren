// 第 5 章的专项禁词与「章内先后」（docs/ch05-design.md 12.2）。
//
// ---------------------------------------------------------------------------
// 为什么 LexiconTests 之外还要这一个文件
// ---------------------------------------------------------------------------
// LexiconTests 的规则是「原著首见晚于本章区间末章 → 本章不许出现」，它管得住跨章，管不住两类事：
//   · **首见早于本章、却被人为推后的词**：「升仙」（全本首见 ch93，就是第 4 章那块牌子上的古篆）、
//     「天眼」（ch67 就有，用户拍板推到第 6 章）。规则层面它们都合法，只能专项点名。
//   · **章内的先后**：「寒毒」首见 ch117、「惊蛟会」ch104、「五色门」「独霸山庄」ch118、
//     「太南谷」「太南山」ch125，都落在第 5 章区间（ch100-125）里，规则放行整章；
//     可它们在本章开头那几节仍然不该出现。LexiconTests 文件头自己写着这一层它看不见。
//
// 章内先后要知道「这一句是在第几节被玩家读到的」。文案 key 的场景段（ch05.<场景>.<用途>）
// 就是这件事的记录：下面那张表把每一种场景前缀钉到施工图第 3 节的节点号上。
// **表里查不到的 key 直接判红**——新加一条文案而不给它登记节点，等于绕开了这一层检查。
//
// ---------------------------------------------------------------------------
// 节点号怎么定（照施工图写死，不照脚本推）
// ---------------------------------------------------------------------------
//   · 剧情挂点的文案：挂点所属的节点（施工图 3.1 那张表的 # 列：1a → 1、12e → 12）。
//   · NPC 闲话：这个 NPC **最早**在场、说得出这句话的那一节（墨凤舞登门之后就在药圃里 → 7，
//     她开口求医书之后的那几句 → 11）。
//   · 目标链：第 N 步的文案是在第 N−1 步做完之后才上屏的，所以按第 N−1 步所属的节点算；
//     第 1 步按 0 算（第 4 章刚结束）。
//   · 韩家村村东新口的拦路话 ch05.block.dukou：第 1-4 章走到那里就看得见，按 0 算。
//   · 内视：客栈在节点 3 打完、西城那道门一开就进得去，按 3 算。
//
// 判据的原文（施工图 12.2）：
//   「升仙」「仙令」「天眼」：ch05. 文案一条也不许有（分母先验 > 150 条）；scripts/ch05/ 里
//     magic_tianyan_shu 0 处。
//   「寒毒」：节点 1-8 不含；「惊蛟会」：节点 1-3 不含；「五色门」「独霸山庄」：节点 1-8 不含；
//   「太南谷」「太南山」：节点 1-10 不含。
// 每条另配一句正向的：这个词本章**确实说了，而且头一回就说在施工图写的那一节**
// （寒毒 9、惊蛟会 4、五色门与独霸山庄 9、太南谷与太南山 11）。只查「没有」的用例，
// 在词被整段删光、或者节点表整张错位时照样全绿——「找不到某个词就算过」，docs/README.md 那张表上有它。
//
// 另有一张本章不许出现的词（施工图 12.1 与第 12 节开头那一串）：它们大多已经进了
// LexiconTests 表一，这里再钉一遍，是为了不让第 5 章的安全依赖另一个文件里那张表有没有人记得补。
//
// ---------------------------------------------------------------------------
// 这个文件看不见什么
// ---------------------------------------------------------------------------
//   · 脚本注释与 data 的 note（给后人看的，不是玩家读得到的字）。
//   · 文案所在的节点**是否真的等于**脚本里调用它的那一节：这里信的是 key 的场景段，
//     一条挂在 ch05.qingbao 名下、却被 dongqu_lu.lua 调用的文案，本文件看不出来。
//   · 不按章分文件的 items.json / battles.json / shops.json（跨章共用）。本章给出的物品与法术
//     描述另放在 data/text/ch05_items.json，在扫描范围之内。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// 表一：文案 key 的场景前缀 → 节点号（施工图第 3 节）。最长前缀优先。
// ---------------------------------------------------------------------------
struct KeyNode {
    const char* prefix;
    int node;
};

const std::vector<KeyNode>& keyNodes() {
    static const std::vector<KeyNode> kTable = {
        // 地名与拦路话
        {"ch05.map.dubashanzhuang.", 12},   // 那张图 12b 之后才进得去，地名那时才上屏
        {"ch05.map.", 1},
        {"ch05.block.dukou", 0},            // 韩家村村东新口：第 1-4 章走到那里就看得见
        {"ch05.block.nancheng", 2},         // 下船之后、黑水巷之前
        {"ch05.block.mofu", 3},             // 西城那道门一开，南城就进得去
        {"ch05.shop.", 3},
        // 节点 1
        {"ch05.dongqu.", 1},
        {"ch05.shangchuan.", 1},
        {"ch05.npc.chuanjia.", 1},
        {"ch05.battle.yesu_yelang.", 1},
        {"item.desc.story_mo_yishu", 1},
        {"item.desc.story_mo_qinbixin", 1},
        {"item.desc.story_wenlong_jie", 1},
        // 节点 2、3
        {"ch05.matou.", 2},
        {"ch05.heishui.", 3},
        {"ch05.battle.heishuixiang.", 3},
        {"ch05.zhuishao.", 3},
        {"ch05.battle.matou_zhuishao.", 3},
        {"ch05.npc.jiefang.", 3},
        {"ch05.neishi.", 3},
        // 节点 4、5、6
        {"ch05.qingbao.", 4},
        {"ch05.jieren.", 4},
        {"ch05.battle.tiequanhui_jieren.", 4},
        {"ch05.jiulou.", 5},
        {"ch05.yeru.", 6},
        {"ch05.toutin.", 6},
        // 节点 7、8、9
        {"ch05.dengmen.", 7},
        {"ch05.jianmian.", 7},
        {"ch05.npc.fengwu.pre", 7},         // 墨凤舞登门之后就在药圃里，那时她还没开口
        {"ch05.huayuan.", 8},
        {"ch05.npc.yange.", 8},
        {"ch05.duizhi.", 9},
        // 节点 10
        {"ch05.dingji.", 10},
        {"ch05.handu.", 10},                // 寒毒检查点：头一个在 10a
        {"ch05.xiaoxiang.", 10},
        {"ch05.battle.xiaoxiangyuan.", 10},
        {"ch05.duobang.", 10},
        {"ch05.battle.duobang.", 10},
        {"ch05.quest.qingling.", 10},       // 支线 Z3：10a 挂起
        // 节点 11
        {"ch05.jiaoyi.", 11},
        {"ch05.yange.", 11},
        {"ch05.battle.yange_qiecuo.", 11},
        {"ch05.anpai.", 11},
        {"ch05.chaoxie.", 11},
        {"ch05.lianfu.", 11},
        {"ch05.npc.fengwu.", 11},
        {"ch05.quest.fengwu.", 11},         // 支线 Z1：11a 挂起
        {"ch05.quest.jianfu.", 11},         // 支线 Z2：11a 挂起
        {"item.desc.story_yigao_chaoben", 11},
        // 节点 12
        {"ch05.majiu.", 12},
        {"ch05.battle.shigui.", 12},
        {"ch05.zhuwu.", 12},
        {"ch05.battle.wu_jianming.", 12},
        {"ch05.tancha.", 12},
        {"ch05.battle.xunzhuang.", 12},
        {"ch05.shangyue.", 12},
        {"ch05.battle.ouyang_feitian.", 12},
        {"ch05.huanyu.", 12},
        {"item.desc.story_nuanyang_baoyu", 12},
        {"magic.desc.magic_ji_jianfu", 12},  // 只在 12d 那一仗里借用
        // 路径行动（data/pathactions/ch05.json，文案在 data/text/ch05_path.json）：一条条目一行，
        // 节点是它挂出来的那一节——when 里那个剧情旗标所属的节点。尾巴上的点是有意的：
        // 没有它，哪天 jiefang_dating2 那一行被删，它的文案会被 jiefang_dating 这一行悄悄认领成第 3 节，
        // 而不是按「查不到」判红。
        {"ch05.path.chuanjia_dating.", 1},   // ch05.kaipian（1a）
        {"ch05.path.jiefang_dating.", 3},    // ch05.zhuishao（3b）
        {"ch05.path.jiefang_dating2.", 5},   // ch05.jiulou（5）
        {"ch05.path.fengwu_dating.", 7},     // ch05.dengmen（7a），下同
        {"ch05.path.fengwu_qiugou.", 7},
        {"ch05.path.huyuan_c_qiugou.", 7},
        {"ch05.path.huyuan_b_qiecuo.", 7},
        {"ch05.path.huyuan_a_dating.", 8},   // ch05.huayuan（8），下同
        {"ch05.path.yange_dating.", 8},
        {"ch05.path.huyuan_a_qiecuo.", 9},   // ch05.duizhi（9）
        {"ch05.path.luren_dating.", 10},     // ch05.dingji（10a）
        {"ch05.path.huyuan_d_dating.", 11},  // ch05.anpai（11c）
    };
    return kTable;
}

// 目标链二十五步所属的节点（施工图 3.4 那张表 × 3.1 那张表）。第 N 步的文案按第 N−1 步算。
const std::vector<std::pair<const char*, int>>& objectiveSteps() {
    static const std::vector<std::pair<const char*, int>> kSteps = {
        {"n01_dongqu", 1},     {"n02_shangchuan", 1},   {"n03_matou", 2},
        {"n04_heishuixiang", 3}, {"n05_zhuishao", 3},   {"n06_qingbao", 4},
        {"n07_jieren", 4},     {"n08_jiulou", 5},       {"n09_yeru", 6},
        {"n10_toutin", 6},     {"n11_dengmen", 7},      {"n12_jianmianli", 7},
        {"n13_huayuan", 8},    {"n14_duizhi", 9},       {"n15_dingji", 10},
        {"n16_xiaoxiang", 10}, {"n17_duobang", 10},     {"n18_jiaoyi", 11},
        {"n19_yange", 11},     {"n20_anpai", 11},       {"n21_majiu", 12},
        {"n22_zhuwu", 12},     {"n23_tancha", 12},      {"n24_cisha", 12},
        {"n25_huanyu", 12},
    };
    return kSteps;
}

constexpr const char* kObjectivePrefix = "objective.ch05.";
constexpr int kUnmapped = -1;

// key → 玩家最早在第几节读到它。查不到返回 kUnmapped。
[[nodiscard]] int nodeOfKey(const std::string& key) {
    if (key.rfind(kObjectivePrefix, 0) == 0) {
        const std::string step = key.substr(std::char_traits<char>::length(kObjectivePrefix));
        const auto& steps = objectiveSteps();
        for (std::size_t i = 0; i < steps.size(); ++i) {
            if (step == steps[i].first) return i == 0 ? 0 : steps[i - 1].second;
        }
        return kUnmapped;
    }
    int best = kUnmapped;
    std::size_t bestLength = 0;
    for (const KeyNode& entry : keyNodes()) {
        const std::string prefix(entry.prefix);
        if (key.rfind(prefix, 0) == 0 && prefix.size() > bestLength) {
            best = entry.node;
            bestLength = prefix.size();
        }
    }
    return best;
}

// ---------------------------------------------------------------------------
// 表二：章内先后（施工图 12.2）。「节点 ≤ lastSilentNode 不许出现；头一回出现在 firstNode」
// ---------------------------------------------------------------------------
struct Ordering {
    const char* word;
    int lastSilentNode;
    int firstNode;
    const char* evidence;
};

const std::vector<Ordering>& orderings() {
    static const std::vector<Ordering> kRules = {
        {"寒毒", 8, 9, "ch117 首见；施工图 3.2 节点 9：「寒毒」一词在这里第一次出现，此前只叫阴毒"},
        {"惊蛟会", 3, 4, "ch104 首见；施工图 3.2 节点 4a：「惊蛟会」三个字在本节第一次出现"},
        {"五色门", 8, 9, "ch118 首见；施工图 3.2 节点 9：「五色门」「独霸山庄」首次出现"},
        {"独霸山庄", 8, 9, "ch118 首见；施工图 3.2 节点 9：「五色门」「独霸山庄」首次出现"},
        {"太南谷", 10, 11, "ch125 首见；施工图 3.2 节点 11c：席铁牛复述那一晚"},
        {"太南山", 10, 11, "ch125 首见；施工图 3.2 节点 11c：孙二狗说山在广贵城西四十里"},
    };
    return kRules;
}

// 本章一个也不许有的词。
//   「升仙」「仙令」「天眼」：施工图 12.2 的专项断言（规则层放行，人为推后）；
//   其余：施工图 12.1 与第 12 节开头那一串（首见晚于 ch125，或只在本章不演的那段对话里）。
const std::vector<std::pair<const char*, const char*>>& bannedWords() {
    static const std::vector<std::pair<const char*, const char*>> kWords = {
        {"升仙", "全本首见 ch93（第 4 章那块牌子上的古篆）；ch106 蓝衣人与同伴的对话里再现，韩立没听见（施工图 0 第 5 条）"},
        {"仙令", "第 4 章那块牌子本章一个字不提（施工图 12.2）"},
        {"天眼", "用户拍板：天眼术整个推到第 6 章，本章不交代、不学会、不使用（施工图 1.1 第 14 条）"},
        {"灵石", "ch131；第 1-5 章的钱是碎银，按块计"},
        {"元婴", "ch127"},
        {"结丹", "ch127"},
        {"灵气", "ch126（宝玉「可以容纳自己的灵气」那一句属第 6 章）"},
        {"炼气", "ch127"},
        {"筑基期", "ch127"},
        {"真元", "ch164"},
        {"黄枫谷", "ch127"},
        {"太南小会", "ch128；太南谷、太南山本章末可以说，这个不行"},
        {"万小山", "ch126"},
        {"灵符", "ch131"},
        {"坊市", "ch146"},
        {"储物袋", "ch148"},
        {"定颜丹", "ch155"},
        {"筑基丹", "只在 ch106 蓝衣人与同伴那段对话里，本章不演那段（施工图 12.2）"},
        {"修仙之人", "同上"},
        {"银月", "ch615，超出全 14 章"},
    };
    return kWords;
}

// ---------------------------------------------------------------------------
// 读文件：data/text/ch05*.json 是扁平的 {"key": "文本", ...}。
// tests 拿不到 nlohmann（LexiconTests 文件头写明了），这里自己解一层。
// \uXXXX 一律还原成 UTF-8：一个写成转义的禁词不该因此溜过去。
// ---------------------------------------------------------------------------
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
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

void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

struct Cursor {
    const std::string& s;
    std::size_t i = 0;
    bool ok = true;

    void skipSpace() {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t')) ++i;
    }
    bool eat(char c) {
        skipSpace();
        if (i < s.size() && s[i] == c) {
            ++i;
            return true;
        }
        return false;
    }
    std::uint32_t hex4() {
        if (i + 4 > s.size()) {
            ok = false;
            return 0;
        }
        std::uint32_t v = 0;
        for (int k = 0; k < 4; ++k) {
            const char c = s[i++];
            v <<= 4;
            if (c >= '0' && c <= '9') v |= static_cast<std::uint32_t>(c - '0');
            else if (c >= 'a' && c <= 'f') v |= static_cast<std::uint32_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') v |= static_cast<std::uint32_t>(c - 'A' + 10);
            else ok = false;
        }
        return v;
    }
    std::string str() {
        std::string out;
        if (!eat('"')) {
            ok = false;
            return out;
        }
        while (i < s.size() && s[i] != '"') {
            if (s[i] != '\\') {
                out += s[i++];
                continue;
            }
            ++i;
            if (i >= s.size()) break;
            const char e = s[i++];
            switch (e) {
                case 'n': out += '\n'; break;
                case 't': out += '\t'; break;
                case 'r': out += '\r'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case 'u': {
                    std::uint32_t cp = hex4();
                    if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < s.size() && s[i] == '\\' && s[i + 1] == 'u') {
                        i += 2;
                        const std::uint32_t low = hex4();
                        cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                    }
                    appendUtf8(out, cp);
                    break;
                }
                default: out += e; break;   // \" \\ \/
            }
        }
        if (i >= s.size()) ok = false;
        else ++i;   // 收尾的引号
        return out;
    }
};

// 解不开就返回空表并把 ok 置假：调用方要先验 ok，别把「没解出来」读成「一条违规也没有」。
std::vector<std::pair<std::string, std::string>> parseFlatObject(const std::string& body, bool& ok) {
    std::vector<std::pair<std::string, std::string>> out;
    Cursor c{body};
    ok = c.eat('{');
    if (!ok) return out;
    if (c.eat('}')) return out;
    while (c.ok) {
        std::string key = c.str();
        if (!c.eat(':')) {
            c.ok = false;
            break;
        }
        std::string value = c.str();
        if (!c.ok) break;
        out.emplace_back(std::move(key), std::move(value));
        if (c.eat(',')) continue;
        if (!c.eat('}')) c.ok = false;
        break;
    }
    ok = c.ok;
    return out;
}

struct Entry {
    std::string file;
    std::string key;
    std::string text;
};

// data/text/ 下文件名以 ch05 开头的全部 .json。
std::vector<Entry> chapterFiveEntries(bool& allParsed, int& files) {
    std::vector<Entry> out;
    allParsed = true;
    files = 0;
    const fs::path textDir = fs::path(assetRoot()) / "data" / "text";
    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator(textDir)) {
        const std::string name = entry.path().filename().string();
        if (entry.path().extension() == ".json" && name.rfind("ch05", 0) == 0) {
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    for (const fs::path& path : paths) {
        ++files;
        bool ok = false;
        for (auto& [key, text] : parseFlatObject(readFile(path), ok)) {
            out.push_back(Entry{path.filename().string(), key, text});
        }
        if (!ok) allParsed = false;
    }
    return out;
}

// 一个词在这批文案里最早出现在第几节；一条都没有返回 kUnmapped。
[[nodiscard]] int firstNodeOf(const std::vector<Entry>& entries, const std::string& word) {
    int first = kUnmapped;
    for (const Entry& e : entries) {
        if (e.text.find(word) == std::string::npos && e.key.find(word) == std::string::npos) continue;
        const int node = nodeOfKey(e.key);
        if (node == kUnmapped) continue;
        if (first == kUnmapped || node < first) first = node;
    }
    return first;
}

// 违反章内先后的条目：「文件 key（第 N 节）」一条一行。
[[nodiscard]] std::vector<std::string> tooEarly(const std::vector<Entry>& entries, const Ordering& rule) {
    std::vector<std::string> out;
    for (const Entry& e : entries) {
        const int node = nodeOfKey(e.key);
        if (node == kUnmapped || node > rule.lastSilentNode) continue;
        if (e.text.find(rule.word) != std::string::npos || e.key.find(rule.word) != std::string::npos) {
            out.push_back(e.file + " " + e.key + "（第 " + std::to_string(node) + " 节）");
        }
    }
    return out;
}

const Ordering& ruleFor(const std::string& word) {
    for (const Ordering& rule : orderings()) {
        if (word == rule.word) return rule;
    }
    static const Ordering kNone{"", 0, 0, ""};
    return kNone;
}

class Ch05Lexicon : public ::testing::Test {
protected:
    void SetUp() override {
        entries_ = chapterFiveEntries(parsed_, files_);
        // 先验分母（施工图 12.2：> 150 条）。一个文件也没读到、或读到了却解不开，
        // 下面每一条「一个也没有」都会恒真。
        ASSERT_TRUE(parsed_) << "data/text/ch05*.json 有文件解不开，扫描结果不可信";
        ASSERT_GE(files_, 3) << "第 5 章的文案文件少于三个（主线、杂项、目标链），根目录找错了？";
        ASSERT_GT(entries_.size(), 150u) << "只读到 " << entries_.size() << " 条第 5 章文案";
    }

    std::vector<Entry> entries_;
    bool parsed_ = false;
    int files_ = 0;
};

void expectOrdering(const std::vector<Entry>& entries, const std::string& word) {
    const Ordering& rule = ruleFor(word);
    ASSERT_FALSE(std::string(rule.word).empty()) << word << " 不在章内先后那张表里";
    const std::vector<std::string> hits = tooEarly(entries, rule);
    EXPECT_TRUE(hits.empty()) << "「" << word << "」说得太早——第 " << rule.lastSilentNode
                              << " 节以前不许出现（" << rule.evidence << "）："
                              << ::testing::PrintToString(hits);
    EXPECT_EQ(firstNodeOf(entries, word), rule.firstNode)
        << "「" << word << "」头一回应当出现在第 " << rule.firstNode << " 节（" << rule.evidence
        << "）。一条都没有时这里是 -1：这个词被删光了，上面那条「不许出现」就成了空话";
}

}  // namespace

// ---------------------------------------------------------------------------
// 先验一 · 节点表与规则表本身
// ---------------------------------------------------------------------------
TEST(Ch05LexiconTables, TheNodeTableIsSaneAndPinsTheDesignedNodes) {
    ASSERT_FALSE(keyNodes().empty());
    ASSERT_EQ(objectiveSteps().size(), 25u) << "施工图 3.4 的目标链是 25 步";
    for (const KeyNode& entry : keyNodes()) {
        EXPECT_NE(entry.prefix[0], '\0') << "空前缀会吞掉所有 key";
        EXPECT_GE(entry.node, 0);
        EXPECT_LE(entry.node, 12) << entry.prefix << "：主线只有 12 个节点";
    }

    // 定点：判据抄自施工图 3.1 / 3.4 的原话，不从 key 表里反推。
    EXPECT_EQ(nodeOfKey("ch05.dongqu.read1"), 1);
    EXPECT_EQ(nodeOfKey("ch05.qingbao.gang2"), 4);
    EXPECT_EQ(nodeOfKey("ch05.duizhi.card"), 9);
    EXPECT_EQ(nodeOfKey("ch05.anpai.where"), 11);
    EXPECT_EQ(nodeOfKey("ch05.huanyu.end"), 12);
    EXPECT_EQ(nodeOfKey("ch05.map.dubashanzhuang.name"), 12) << "最长前缀要压过 ch05.map.";
    EXPECT_EQ(nodeOfKey("ch05.map.dukou.name"), 1);
    EXPECT_EQ(nodeOfKey("ch05.npc.fengwu.pre"), 7) << "最长前缀要压过 ch05.npc.fengwu.";
    EXPECT_EQ(nodeOfKey("ch05.npc.fengwu.give1"), 11);
    // 目标链按上一步：第 14 步（上小楼对质，节点 9）是在第 13 步（花园，节点 8）做完之后上屏的。
    EXPECT_EQ(nodeOfKey("objective.ch05.n14_duizhi"), 8);
    EXPECT_EQ(nodeOfKey("objective.ch05.n15_dingji"), 9);
    EXPECT_EQ(nodeOfKey("objective.ch05.n01_dongqu"), 0);
    // 表外的 key 必须查不到——否则「每条都登记了节点」那一条会恒真。
    EXPECT_EQ(nodeOfKey("ch05.nosuchscene.x"), kUnmapped);
    EXPECT_EQ(nodeOfKey("objective.ch05.n99_nothing"), kUnmapped);
    EXPECT_EQ(nodeOfKey("ch04.dongqu.last"), kUnmapped);

    ASSERT_EQ(orderings().size(), 6u);
    for (const Ordering& rule : orderings()) {
        EXPECT_LT(rule.lastSilentNode, rule.firstNode) << rule.word;
        EXPECT_NE(rule.evidence[0], '\0') << rule.word << " 没写出处";
    }
    ASSERT_GE(bannedWords().size(), 3u);
    for (const auto& [word, why] : bannedWords()) {
        EXPECT_NE(word[0], '\0') << "禁词表里有一条空串，它会命中任何文本";
        EXPECT_NE(why[0], '\0') << word << " 没写出处";
    }
}

// ---------------------------------------------------------------------------
// 先验二 · 扫描器真的抓得住（喂一批手写的条目，不碰仓库）
// ---------------------------------------------------------------------------
TEST(Ch05LexiconTables, TheScannerCatchesAWordSaidInTheWrongNode) {
    const std::vector<Entry> early = {
        {"t.json", "ch05.matou.x", "码头上有人提起寒毒"},             // 节点 2
        {"t.json", "ch05.heishui.x", "惊蛟会的人也在"},                // 节点 3
        {"t.json", "ch05.jiulou.x", "五色门的探子"},                   // 节点 5
        {"t.json", "objective.ch05.n11_dengmen", "去独霸山庄"},        // 按第 10 步算：节点 6
        {"t.json", "ch05.duobang.x", "听说太南谷"},                    // 节点 10
    };
    EXPECT_EQ(tooEarly(early, ruleFor("寒毒")).size(), 1u);
    EXPECT_EQ(tooEarly(early, ruleFor("惊蛟会")).size(), 1u);
    EXPECT_EQ(tooEarly(early, ruleFor("五色门")).size(), 1u);
    EXPECT_EQ(tooEarly(early, ruleFor("独霸山庄")).size(), 1u);
    EXPECT_EQ(tooEarly(early, ruleFor("太南谷")).size(), 1u);

    // 配对的正向：同样的词放在它该出现的那一节，必须**不**被判违规。
    const std::vector<Entry> onTime = {
        {"t.json", "ch05.duizhi.x", "寒毒"},
        {"t.json", "ch05.qingbao.x", "惊蛟会"},
        {"t.json", "ch05.duizhi.y", "五色门与独霸山庄"},
        {"t.json", "ch05.anpai.x", "太南谷"},
    };
    for (const Ordering& rule : orderings()) {
        EXPECT_TRUE(tooEarly(onTime, rule).empty()) << rule.word;
    }
    EXPECT_EQ(firstNodeOf(onTime, "寒毒"), 9);
    EXPECT_EQ(firstNodeOf(onTime, "惊蛟会"), 4);
    EXPECT_EQ(firstNodeOf(onTime, "太南山"), kUnmapped) << "没说过的词，最早节点必须是查无";

    // 转义写法也得认出来：一个写成 \u 的禁词不能溜过去。
    bool ok = false;
    const auto parsed = parseFlatObject(
        "{\"ch05.matou.x\": \"\\u5bd2\\u6bd2\", \"ch05.matou.y\": \"a\\\"b\"}", ok);
    ASSERT_TRUE(ok);
    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed[0].second, "寒毒");
    EXPECT_EQ(parsed[1].second, "a\"b");
    bool bad = true;
    (void)parseFlatObject("{\"ch05.matou.x\": \"没收尾", bad);
    EXPECT_FALSE(bad) << "解不开的文件必须报出来，不能当成零条";
}

// ---------------------------------------------------------------------------
// 正题 · 逐条扫 data/text/ch05*.json
// ---------------------------------------------------------------------------
TEST_F(Ch05Lexicon, EveryChapterFiveKeyIsPinnedToANode) {
    std::vector<std::string> loose;
    for (const Entry& e : entries_) {
        if (nodeOfKey(e.key) == kUnmapped) loose.push_back(e.file + " " + e.key);
    }
    EXPECT_TRUE(loose.empty())
        << "这些第 5 章文案查不到属于哪一节——章内先后管不到它们。"
           "在本文件的节点表里登记它的场景前缀：" << ::testing::PrintToString(loose);
}

TEST_F(Ch05Lexicon, NoLineSaysTheWordsThisChapterMustNotSay) {
    for (const auto& [word, why] : bannedWords()) {
        std::vector<std::string> hits;
        for (const Entry& e : entries_) {
            if (e.text.find(word) != std::string::npos || e.key.find(word) != std::string::npos) {
                hits.push_back(e.file + " " + e.key);
            }
        }
        EXPECT_TRUE(hits.empty()) << "第 5 章文案里出现了「" << word << "」（" << why
                                  << "）：" << ::testing::PrintToString(hits);
    }
}

TEST_F(Ch05Lexicon, NoChapterFiveScriptTeachesTheHeavenlyEye) {
    // 施工图 12.2：scripts/ch05/ 里 magic_tianyan_shu 0 处。先验分母：目录里真有脚本。
    const fs::path dir = fs::path(assetRoot()) / "scripts" / "ch05";
    ASSERT_TRUE(fs::is_directory(dir)) << "找不到 " << dir.string();
    int scanned = 0;
    std::vector<std::string> hits;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".lua") continue;
        ++scanned;
        if (readFile(entry.path()).find("magic_tianyan_shu") != std::string::npos) {
            hits.push_back(entry.path().filename().string());
        }
    }
    ASSERT_GE(scanned, 25) << "第 5 章的脚本只扫到 " << scanned << " 个";
    EXPECT_TRUE(hits.empty()) << "天眼术本章不交代、不学会、不使用：" << ::testing::PrintToString(hits);
}

TEST_F(Ch05Lexicon, ColdPoisonIsNamedOnlyFromTheConfrontationOn) {
    expectOrdering(entries_, "寒毒");
}

TEST_F(Ch05Lexicon, TheFlyingDragonSocietyIsNamedOnlyFromTheInnOn) {
    expectOrdering(entries_, "惊蛟会");
}

TEST_F(Ch05Lexicon, TheTwoEnemyHousesAreNamedOnlyFromTheConfrontationOn) {
    expectOrdering(entries_, "五色门");
    expectOrdering(entries_, "独霸山庄");
}

TEST_F(Ch05Lexicon, TheSouthernValleyIsNamedOnlyAtTheLastArrangement) {
    expectOrdering(entries_, "太南谷");
    expectOrdering(entries_, "太南山");
}

// ---------------------------------------------------------------------------
// 两条只落在文案上的拍板（第 5 章复验 LOW-c：变异 U1、U2 当时没有一条测试抓得住）
// ---------------------------------------------------------------------------
// 判据原文，照抄写死：
//   施工图 16.1 第 14 条（⑨ 巡庄）：「三个庄丁不杀：韩立是用毒的人，一包药让他们睡到天亮、
//     醒来只当自己打了盹；仗照打，不留尸体、不埋人」。
//   初审 MEDIUM-1 → 复验判据 4（⑩ 得手）：「两侧终局都是剑符取首级，不再有『顺得不像真的』」。
//     ch126 追叙那一句「整个过程出奇的顺利和容易，一点波澜也没起」是原著的质感，得手之后一个字也不许回来。
// 直接读文案与脚本，不走位、不开仗：
//   1. 文案 ch05.tancha.* 一条也不带「杀」「埋」「尸」「死」「坑」；
//   2. scripts/ch05/tancha.lua 里 battle("b05_xunzhuang") 到 flag.set("ch05.tancha") 之间只说
//      lost（输）与 drugged、drugged2（赢），赢的那两句一句不少；
//   3. scripts/ch05/shangyue.lua 从 won_fu 那一句起（含）说的每一句、以及 ch05.huanyu.* 全部，
//      一条也不带「顺」「容易」「不像真的」「波澜」。
namespace {

std::map<std::string, std::string> textByKey(const std::vector<Entry>& entries) {
    std::map<std::string, std::string> out;
    for (const Entry& e : entries) out[e.key] = e.text;
    return out;
}

// 去掉 Lua 注释（-- 到行尾；本章脚本的字符串里没有「--」）。
std::string luaCodeOnly(const std::string& source) {
    std::istringstream in(source);
    std::string line;
    std::string out;
    while (std::getline(in, line)) {
        const std::size_t dash = line.find("--");
        out += (dash == std::string::npos ? line : line.substr(0, dash)) + "\n";
    }
    return out;
}

std::vector<std::string> talkKeysIn(const std::string& code) {
    static const std::regex kTalk(R"re(talk\("[^"]*",\s*"([^"]+)"\))re");
    std::vector<std::string> keys;
    for (auto it = std::sregex_iterator(code.begin(), code.end(), kTalk); it != std::sregex_iterator(); ++it) {
        keys.push_back((*it)[1]);
    }
    return keys;
}

std::vector<std::string> hitsOf(const std::map<std::string, std::string>& texts, const std::vector<std::string>& keys,
                                const std::vector<const char*>& words) {
    std::vector<std::string> hits;
    for (const std::string& key : keys) {
        const auto found = texts.find(key);
        if (found == texts.end()) {
            hits.push_back(key + "（文案里没有这一条）");
            continue;
        }
        for (const char* word : words) {
            if (found->second.find(word) != std::string::npos) hits.push_back(key + "「" + word + "」");
        }
    }
    return hits;
}

}  // namespace

TEST_F(Ch05Lexicon, ThePatrolIsDruggedNotKilledAndTheKillIsNeverCalledSmooth) {
    const std::map<std::string, std::string> texts = textByKey(entries_);
    const fs::path dir = fs::path(assetRoot()) / "scripts" / "ch05";

    // 1. ⑨ 巡庄：文案里没有杀人、埋人。
    std::vector<std::string> tancha;
    for (const auto& [key, text] : texts) {
        if (key.rfind("ch05.tancha.", 0) == 0) tancha.push_back(key);
    }
    ASSERT_GE(tancha.size(), 6u) << "先验：ch05.tancha.* 读得到";
    const std::vector<const char*> killing = {"杀", "埋", "尸", "死", "坑"};
    EXPECT_TRUE(hitsOf(texts, tancha, killing).empty())
        << "施工图 16.1 第 14 条「三个庄丁不杀……不留尸体、不埋人」：" << ::testing::PrintToString(hitsOf(texts, tancha, killing));

    // 2. ⑨ 巡庄：赢了那一支说的是迷倒的两句，也只说这两句。
    const std::string tanchaCode = luaCodeOnly(readFile(dir / "tancha.lua"));
    const std::size_t fight = tanchaCode.find("battle(\"b05_xunzhuang\")");
    const std::size_t done = tanchaCode.find("flag.set(\"ch05.tancha\")");
    ASSERT_NE(fight, std::string::npos) << "tancha.lua 里找不到 ⑨ 那一仗";
    ASSERT_NE(done, std::string::npos) << "tancha.lua 里找不到 flag.set(\"ch05.tancha\")";
    ASSERT_LT(fight, done);
    const std::vector<std::string> after = talkKeysIn(tanchaCode.substr(fight, done - fight));
    const std::vector<std::string> allowed = {"ch05.tancha.lost", "ch05.tancha.drugged", "ch05.tancha.drugged2"};
    for (const std::string& key : after) {
        EXPECT_NE(std::find(allowed.begin(), allowed.end(), key), allowed.end())
            << "⑨ 打完之后多说了一句 " << key << "（16.1 第 14 条：迷倒不杀，只许 drugged / drugged2）";
    }
    for (const char* key : {"ch05.tancha.drugged", "ch05.tancha.drugged2"}) {
        EXPECT_NE(std::find(after.begin(), after.end(), key), after.end())
            << "⑨ 赢了该说 " << key << "（一包药让他们睡到天亮）";
    }

    // 3. ⑩ 得手之后：不说顺、不说容易。
    const std::string shangyueCode = luaCodeOnly(readFile(dir / "shangyue.lua"));
    const std::size_t won = shangyueCode.find("\"ch05.shangyue.won_fu\"");
    ASSERT_NE(won, std::string::npos) << "shangyue.lua 里找不到得手那一句 won_fu";
    const std::size_t wonTalk = shangyueCode.rfind("talk(", won);
    ASSERT_NE(wonTalk, std::string::npos) << "won_fu 不是用 talk 说的？";
    std::vector<std::string> afterKill = talkKeysIn(shangyueCode.substr(wonTalk));
    ASSERT_GE(afterKill.size(), 3u) << "先验：得手之后至少 won_fu、head、back 三句";
    std::size_t huanyu = 0;
    for (const auto& [key, text] : texts) {
        if (key.rfind("ch05.huanyu.", 0) == 0) {
            afterKill.push_back(key);
            ++huanyu;
        }
    }
    ASSERT_GE(huanyu, 10u) << "先验：ch05.huanyu.* 读得到";
    const std::vector<const char*> smooth = {"顺", "容易", "不像真的", "波澜"};
    EXPECT_TRUE(hitsOf(texts, afterKill, smooth).empty())
        << "复验判据 4「两侧终局都是剑符取首级，不再有『顺得不像真的』」：" << ::testing::PrintToString(hitsOf(texts, afterKill, smooth));
}
