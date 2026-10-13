// 第 11 章验收第 22、23 条：施工图 12.2 的专项断言（docs/ch11-design.md 12.2、第 15 节 22–23）。
//
// LexiconTests 只管「原著首见晚于本章区间 → 不许出现」。它管不住三类事，这里专项点名：
//   · 章内先后：一个词本章可以说，但不能在它那一节之前说（「鲁二」不能出现在指环那一场）。
//   · 恰几处：「虚天残图」「虚天殿」全章各恰 1 处、同在 wuchou.* 那一句；「一贫如洗」只此一处……
//   · 全章 0 处：首见早于本章、却被施工图点名不许出现的词（天晶木 ch399、白玉蜘蛛 ch398、
//     隆兴客栈 ch409……进了表一就等于对本章解禁，所以只能在这里拦）。
//
// 节点怎么编（照施工图 12.2「断言的写法」，不照脚本推）：
//   · 场景前缀取 J 表「谁置」那一列的脚本名，编在 3.1 的 n 号上；为了把「这一节里」和「这一节做完之后」
//     分开，统一乘 2：脚本 nX 里演的文案记 2X（S），nX 做完之后才露出来的记 2X+1（A）。
//   · 路径行动：按它 when 里最晚的那面链上旗标（A）；目标链第 k 步：按第 k−1 步（A），第 1 步记 1。
//   · 物品描述按 give 它的脚本；战斗开场白与蓄势按调用它的脚本；地图名按第一次 teleport 进去的脚本，
//     走门进去的按门的 require_flag（A）；门的拒绝语按那扇门所在图头一回走得到的时刻。
//   · 坊市的地名与三扇门：trigger_chudao 横在城门 spawn 北面五格宽的门洞上，进城必先踩它，所以门后的
//     东西记 A(1)。地名「天星城坊市」是施工图第 4 节写死的名字，读档落图时就上屏、与 n01 同一刻，记 S(1)
//     （12.2「天星城只许 chudao.* 起」与第 4 节相抵处，见施工图 18.5）。
//   · 表里查不到的 key 直接判红：新加文案不登节点，等于绕开这一层。
//
// 这个文件看不见什么：文案 key 的场景段与「真正调用它的脚本」是否一致（信 key 名）；脚本注释与 note；
// 原创性（最长公共片段归复验脚本，不归测试）。
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>
#include "Ch11Data.h"

namespace {
using namespace fanren::test::ch11;

constexpr int S(int n) { return 2 * n; }
constexpr int A(int n) { return 2 * n + 1; }
constexpr int kUnmapped = -1;
constexpr int kNoPositive = -2;

struct Prefix { const char* prefix; int node; };

const std::vector<Prefix>& prefixes() {
    static const std::vector<Prefix> kTable = {
        {"ch11.error.", 1},
        {"ch11.map.fangshi.", S(1)}, {"ch11.map.dongfu.", S(4)}, {"ch11.map.xingkong.", A(8)},
        {"ch11.map.ningcui_jie.", S(9)}, {"ch11.map.honghu.", A(10)}, {"ch11.map.yinsha_dao.", S(21)},
        {"ch11.map.xiaoxuan.", A(27)},
        {"ch11.block.dongfu", A(1)}, {"ch11.block.fangshi", A(1)}, {"ch11.block.xiaoxuan", A(1)},
        {"ch11.block.xingkong", S(4)}, {"ch11.block.honghu", S(9)},
        {"ch11.shop.name.fangshi_zahuo", A(8)}, {"ch11.shop.name.ningcui_zahuo", S(9)},
        {"ch11.shop.name.ningcui_shougou", S(9)}, {"ch11.shop.name.qingzhu_xiaoxuan", A(27)},
        {"ch11.chudao.", S(1)}, {"ch11.zhihuan.", S(2)}, {"ch11.luer.", S(3)}, {"ch11.zufu.", S(4)},
        {"ch11.anju.", S(5)}, {"ch11.kuilei.", S(6)}, {"ch11.ziliao.", S(7)}, {"ch11.nichang.", S(8)},
        {"ch11.xingkong.", S(9)}, {"ch11.huicheng_chuansong.", A(9)}, {"ch11.ningcui.", S(10)},
        {"ch11.z1.", A(10)}, {"ch11.quest.shougou_dingdan.", A(10)}, {"ch11.buzhen.", S(11)},
        {"ch11.youyao.", S(12)}, {"ch11.battle.ningcuidao_youyao.", S(12)}, {"ch11.battle.qianmu.", S(12)},
        {"ch11.battle.honghu_jiguanjiao.", S(12)}, {"ch11.battle.jiguan.", S(12)}, {"ch11.jiguan.", S(13)},
        {"ch11.huicheng.", S(14)}, {"ch11.liandan.", S(15)}, {"ch11.biguan.", S(16)}, {"ch11.chuguan.", S(17)},
        {"ch11.zhaolan.", S(18)}, {"ch11.z2.", A(18)}, {"ch11.quest.linju_dingyan.", A(18)},
        {"ch11.lianer.", S(19)}, {"ch11.chaguan.", S(20)}, {"ch11.zhuzhan.", S(21)}, {"ch11.guiju.", S(22)},
        {"ch11.battle.yinsha_shayao.", S(22)}, {"ch11.battle.shayao.", S(22)}, {"ch11.wuchou.", S(23)},
        {"ch11.daoge.", S(24)}, {"ch11.battle.tiandu_shi.", S(24)}, {"ch11.battle.tiandu.", S(24)},
        {"magic.desc.magic_zhizhu_tuwang", S(24)}, {"magic.desc.magic_kuilei_zihuo", S(24)},
        {"ch11.zhanglao.", S(25)}, {"ch11.peizhu.", S(26)}, {"ch11.kaidian.", S(27)}, {"ch11.guitai.", A(27)},
        {"ch11.jiesuan.", S(28)}, {"ch11.lianjian.", S(30)}, {"ch11.chujian.", S(30)}, {"ch11.zhuzhong.", S(31)},
        {"item.desc.story_jinzhi_lingpai", S(4)}, {"item.desc.story_kuilei_qun", S(6)},
        {"item.desc.story_yaoshou_ziliao", S(7)}, {"item.desc.herb_nichang_cao", S(8)},
        {"item.desc.story_waihai_tu", S(10)}, {"item.desc.material_yaodan_liuji", S(12)},
        {"item.desc.story_yaoshou_cailiao", S(14)}, {"item.desc.story_kuilei_can", S(14)},
        {"item.desc.story_kuilei_yuan_qun", S(18)}, {"item.desc.material_tianleizhu", S(25)},
        {"item.desc.story_miaoyin_yaopai", S(25)}, {"item.desc.material_lianjian_fuliao", S(26)},
        {"item.desc.story_bai_zhenzhu", S(31)},
        // 路径行动：when 里最晚的链上旗标（data/pathactions/ch11.json；施工图 18.2 第 1 条的 id）。
        {"ch11.path.baiyi_guiju_dating.", A(1)}, {"ch11.path.luer_shengshan_dating.", A(2)},
        {"ch11.path.fengle_mulu_dating.", A(6)}, {"ch11.path.chuwu_dai_dating.", A(6)},
        {"ch11.path.baimen_zhaixing_dating.", A(7)}, {"ch11.path.zaohong_jiguan_dating.", A(8)},
        {"ch11.path.haoyuantian_waihai_dating.", A(10)}, {"ch11.path.haoyuantian_qianmu_dating.", A(10)},
        {"ch11.path.fanxiaozi_jiage_dating.", A(10)}, {"ch11.path.zahuo_haiyu_dating.", A(10)},
        {"ch11.path.linju_dingyan_dating.", A(18)}, {"ch11.path.linju_miaoyin_dating.", A(20)},
        {"ch11.path.hexianshi_fanren_dating.", A(27)}, {"ch11.path.jinqing_luanxing_dating.", A(30)},
    };
    return kTable;
}

// 施工图 3.4 目标链 29 步：step id 里保留的就是 n 号。
const std::vector<std::pair<const char*, int>>& steps() {
    static const std::vector<std::pair<const char*, int>> kSteps = {
        {"n01_chudao", 1}, {"n02_zhihuan", 2}, {"n03_luer", 3}, {"n04_zufu", 4}, {"n05_anju", 5},
        {"n06_kuilei", 6}, {"n07_ziliao", 7}, {"n08_nichang", 8}, {"n09_xingkong", 9}, {"n10_ningcui", 10},
        {"n11_buzhen", 11}, {"n12_youyao", 12}, {"n14_huicheng", 14}, {"n15_liandan", 15}, {"n16_biguan", 16},
        {"n17_chuguan", 17}, {"n18_zhaolan", 18}, {"n19_lianer", 19}, {"n20_chaguan", 20}, {"n21_zhuzhan", 21},
        {"n22_guiju", 22}, {"n23_wuchou", 23}, {"n24_daoge", 24}, {"n25_zhanglao", 25}, {"n26_peizhu", 26},
        {"n27_kaidian", 27}, {"n29_liugen", 29}, {"n30_lianjian", 30}, {"n31_zhuzhong", 31}};
    return kSteps;
}

constexpr const char* kObjective = "objective.ch11.";

int nodeOfKey(const std::string& key) {
    if (key.rfind(kObjective, 0) == 0) {
        const std::string step = key.substr(std::string(kObjective).size());
        const auto& all = steps();
        for (std::size_t i = 0; i < all.size(); ++i)
            if (step == all[i].first) return i == 0 ? 1 : A(all[i - 1].second);
        return kUnmapped;
    }
    int best = kUnmapped;
    std::size_t length = 0;
    for (const Prefix& entry : prefixes()) {
        const std::string prefix(entry.prefix);
        if (key.rfind(prefix, 0) == 0 && prefix.size() > length) {
            best = entry.node;
            length = prefix.size();
        }
    }
    return best;
}
// 章内先后（12.2 那张表）：from = 最早许出现的节点；first = 头一回应出现在哪一节（kNoPositive = 不钉）。
// 正向那一半防「词被删光、只查没有的那一条恒真」。
struct Ordering { const char* word; int from; int first; };

const std::vector<Ordering>& orderings() {
    static const std::vector<Ordering> kRules = {
        {"天星城", S(1), S(1)}, {"星宫", S(1), S(1)}, {"内星海", S(1), S(1)}, {"外星海", S(1), S(1)},
        {"妖海", S(1), S(1)}, {"天星双圣", S(1), S(1)}, {"摘星大会", S(1), kNoPositive}, {"妖兽岛", S(1), S(1)},
        {"红色指环", S(2), S(2)}, {"蓝色指环", S(2), S(2)}, {"鲁二", A(2), S(3)},
        {"圣山", A(2), A(2)}, {"圣宫", A(2), A(2)}, {"八十一层", A(2), A(2)}, {"三十九层", S(4), S(4)},
        {"白门阁", S(6), S(6)}, {"丰乐拍卖行", S(6), kNoPositive}, {"储物袋铺", S(6), kNoPositive},
        {"星空殿", S(8), S(8)}, {"凝翠岛", S(8), S(8)}, {"传送符", S(8), S(9)},
        {"丰乐盟", S(10), A(10)}, {"碧磷鱼妖", S(10), S(10)}, {"郝远天", S(10), S(10)}, {"范小子", S(10), S(10)},
        {"红瑚岛", S(10), S(10)}, {"霓裳草", S(7), S(7)}, {"诱妖草", S(7), S(8)}, {"千目妖", S(7), S(7)},
        {"生死关", S(15), S(15)}, {"散功", S(15), S(15)}, {"筑基初期", S(15), S(15)},
        {"三级傀儡", S(18), S(18)}, {"妙音门", S(19), S(20)}, {"范静梅", S(19), S(19)}, {"莲儿", S(19), S(19)},
        {"清河茶馆", S(19), S(19)}, {"赵长老", S(20), S(20)}, {"范夫人", S(20), S(20)},
        {"范左使", S(20), kNoPositive}, {"天雷竹", S(20), S(20)}, {"三大神木", S(20), S(20)},
        {"卓如婷", S(21), S(25)}, {"右使", S(21), kNoPositive}, {"汪门主", S(21), S(21)},
        {"赤火老怪", S(21), S(21)}, {"元龟岛", S(21), S(21)}, {"葵水魔功", S(21), kNoPositive},
        {"紫灵", S(21), S(21)}, {"隐煞门", S(21), S(21)}, {"符长老", S(21), kNoPositive}, {"煞妖", S(21), S(22)},
        {"乌丑", S(23), S(23)}, {"极阴", S(23), S(23)}, {"天都尸火", S(23), kNoPositive}, {"禁神牌", S(23), S(23)},
        {"名义长老", S(25), S(25)}, {"年俸", S(25), S(25)}, {"膏玉", S(25), kNoPositive},
        {"白雷竹", S(25), S(26)}, {"金雷竹", S(25), S(25)}, {"辟邪之雷", S(25), S(26)},
        {"青竹小轩", S(27), S(27)}, {"李氏杂货", S(27), S(27)}, {"何仙师", S(27), S(27)}, {"入世", S(27), S(27)},
        {"镇店之宝", S(28), S(28)}, {"第九个年头", S(28), S(28)},
        {"七十二", S(30), S(30)}, {"黄道吉日", S(30), S(30)}, {"火柱", S(30), S(30)},
        {"金青", S(31), S(31)}, {"白珍珠", S(31), S(31)}, {"古修士", S(31), S(31)}, {"六连殿", S(31), S(31)},
        {"走火入魔", S(31), S(31)},
    };
    return kRules;
}

// 定点：只许出现在这几个前缀下，条数在 [least, most] 之间。
struct Only { const char* word; std::vector<const char*> where; int most; int least; };

const std::vector<Only>& onlies() {
    static const std::vector<Only> kRules = {
        {"鸡冠蛟", {"ch11.jiguan.", "ch11.battle.honghu_jiguanjiao.intro"}, 99, 1},
        {"四色", {"ch11.biguan.", "ch11.chuguan."}, 99, 1},
        {"剑影分光术", {"ch11.chuguan."}, 1, 1},
        {"幻形天罗阵", {"ch11.buzhen."}, 1, 1}, {"天风狂烈阵", {"ch11.buzhen."}, 1, 1}, {"颠倒五行阵", {"ch11.buzhen."}, 1, 1},
        {"妖尸", {"ch11.daoge.", "ch11.battle.tiandu_shi.", "ch11.battle.tiandu."}, 99, 1},
        {"煞丹分身", {"ch11.daoge."}, 1, 1}, {"一贫如洗", {"ch11.zhaolan."}, 1, 1},
        {"木龙碑", {"ch11.zhanglao."}, 1, 0}, {"汪凝", {"ch11.zhanglao."}, 1, 0}, {"倾家荡产", {"ch11.peizhu."}, 1, 0},
        {"每月", {"ch11.path.baiyi_guiju_dating.", "ch11.zhihuan."}, 99, 0},
        {"虚天残图", {"ch11.wuchou."}, 1, 1}, {"虚天殿", {"ch11.wuchou."}, 1, 1},
    };
    return kRules;
}

// 全章 0 处（12.1「仍禁」、12.2 各行的「全章 0 处」）。黄龙放行「黄龙丹」、十余年放行「二十余年」另判。
const std::vector<const char*>& zeroWords() {
    static const std::vector<const char*> kWords = {
        "明珠轩", "交换会", "文思月", "元瑶", "银月", "紫罗极火", "猎妖令", "掌天瓶", "聂盈", "乾蓝",
        "隆兴客栈", "元磁神光", "十八天都尸", "天音门", "文樯", "木龙真人", "白玉蜘蛛", "天晶木",
        "孟长老", "赵孟", "蓝雷", "三十多年"};
    return kWords;
}

struct Entry { std::string key; std::string text; };
using Entries = std::vector<Entry>;

bool has(const Entry& e, const std::string& word) { return e.text.find(word) != std::string::npos; }
bool under(const std::string& key, const std::vector<const char*>& where) {
    return std::any_of(where.begin(), where.end(), [&](const char* p) { return key.rfind(p, 0) == 0; });
}

// 「十余年」前一个字不是「二」、「黄龙」后一个字不是「丹」的才算。
bool bareWord(const std::string& text, const std::string& word, const std::string& notBefore, const std::string& notAfter) {
    for (std::size_t at = text.find(word); at != std::string::npos; at = text.find(word, at + 1)) {
        const bool before = !notBefore.empty() && at >= notBefore.size() && text.compare(at - notBefore.size(), notBefore.size(), notBefore) == 0;
        const bool after = !notAfter.empty() && text.compare(at + word.size(), notAfter.size(), notAfter) == 0;
        if (!before && !after) return true;
    }
    return false;
}

std::vector<std::string> orderingProblems(const Entries& entries, const Ordering& rule) {
    std::vector<std::string> out;
    int first = kUnmapped;
    for (const Entry& e : entries) {
        const int node = nodeOfKey(e.key);
        if (node == kUnmapped || !has(e, rule.word)) continue;
        if (node < rule.from) out.push_back(std::string(rule.word) + " too early in " + e.key);
        if (first == kUnmapped || node < first) first = node;
    }
    if (rule.first != kNoPositive && first != rule.first)
        out.push_back(std::string(rule.word) + " first at " + std::to_string(first) + ", design says " + std::to_string(rule.first));
    return out;
}

std::vector<std::string> onlyProblems(const Entries& entries, const Only& rule) {
    std::vector<std::string> out;
    int hits = 0;
    for (const Entry& e : entries) {
        if (!has(e, rule.word)) continue;
        ++hits;
        if (!under(e.key, rule.where)) out.push_back(std::string(rule.word) + " outside its scene: " + e.key);
    }
    if (hits > rule.most || hits < rule.least)
        out.push_back(std::string(rule.word) + " appears " + std::to_string(hits) + " times");
    return out;
}
std::vector<std::string> zeroProblems(const Entries& entries) {
    std::vector<std::string> out;
    for (const Entry& e : entries) {
        for (const char* word : zeroWords())
            if (has(e, word)) out.push_back(std::string(word) + " in " + e.key);
        if (bareWord(e.text, "黄龙", "", "丹")) out.push_back("黄龙 in " + e.key);
        if (bareWord(e.text, "十余年", "二", "")) out.push_back("十余年 in " + e.key);
    }
    return out;
}

// 12.2 的全部专项：查不到节点、章内先后、定点、0 处、两条单句断言。
std::vector<std::string> allProblems(const Entries& entries) {
    std::vector<std::string> out;
    const auto append = [&out](const std::vector<std::string>& more) { out.insert(out.end(), more.begin(), more.end()); };
    for (const Entry& e : entries)
        if (nodeOfKey(e.key) == kUnmapped) out.push_back("unmapped " + e.key);
    for (const Ordering& rule : orderings()) append(orderingProblems(entries, rule));
    for (const Only& rule : onlies()) append(onlyProblems(entries, rule));
    append(zeroProblems(entries));
    // 洞府北口拒绝语不提「星空殿」「传送」（18.1 第 2 条）；第 12 章钩子两个词同在一句。
    int blockLines = 0, hookLines = 0;
    for (const Entry& e : entries) {
        if (e.key == "ch11.block.xingkong") {
            ++blockLines;
            if (has(e, "星空殿") || has(e, "传送")) out.push_back("ch11.block.xingkong names the hall");
        }
        if (has(e, "虚天残图") && has(e, "虚天殿")) ++hookLines;
    }
    if (blockLines != 1) out.push_back("ch11.block.xingkong missing");
    if (hookLines != 1) out.push_back("the Xutian hook is not one line");
    return out;
}

// 施工图 12.2 第 15 节 22 条：分母先验 > 250 条；十一份文案文件。
bool denominatorHolds(const Entries& entries, int files) { return entries.size() > 250u && files >= 11; }

Entries chapterEntries(int& files, std::string& error) {
    Entries out;
    files = 0;
    std::vector<fs::path> paths;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text"))
        if (file.path().extension() == ".json" && file.path().filename().string().rfind("ch11_", 0) == 0)
            paths.push_back(file.path());
    std::sort(paths.begin(), paths.end());
    for (const fs::path& path : paths) {
        ++files;
        try {
            const Json file = json(path);
            for (auto it = file.begin(); it != file.end(); ++it) {
                if (!it.value().is_string()) error += it.key() + " is not a string; ";
                out.push_back({it.key(), it.value().is_string() ? it.value().get<std::string>() : std::string()});
            }
        } catch (const std::exception& e) {
            error += path.filename().string() + ": " + e.what() + "; ";
        }
    }
    return out;
}

std::string joined(const std::vector<std::string>& lines) {
    std::string out;
    for (const auto& line : lines) out += "\n  " + line;
    return out;
}

Entries without(Entries entries, const std::string& word) {
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](const Entry& e) { return has(e, word); }), entries.end());
    return entries;
}

Entries with(Entries entries, const std::string& key, const std::string& text) {
    for (Entry& e : entries)
        if (e.key == key) { e.text = text; return entries; }
    entries.push_back({key, text});
    return entries;
}

class Ch11Lexicon : public ::testing::Test {
protected:
    void SetUp() override {
        entries_ = chapterEntries(files_, error_);
        ASSERT_TRUE(error_.empty()) << error_;
        ASSERT_TRUE(denominatorHolds(entries_, files_)) << entries_.size() << " lines in " << files_ << " files";
    }
    Entries entries_;
    int files_ = 0;
    std::string error_;
};
// ---- 先验：节点表本身（字面量抄施工图 3.1 / 3.4，不从 key 反推）----
TEST(Ch11LexiconTables, TheNodeTableIsSaneAndPinsTheDesignedNodes) {
    ASSERT_EQ(steps().size(), 29u) << "施工图 3.4：目标链 29 步";
    for (const Prefix& entry : prefixes()) {
        EXPECT_NE(entry.prefix[0], '\0');
        EXPECT_GE(entry.node, 0);
        EXPECT_LE(entry.node, S(31));
    }
    EXPECT_EQ(nodeOfKey("ch11.luer.greet"), 6);
    EXPECT_EQ(nodeOfKey("ch11.jiguan.gone"), 26) << "n13 编在 youyao（24）之后";
    EXPECT_EQ(nodeOfKey("ch11.jiesuan.ninth"), 56) << "n28 编在 kaidian（54）之后";
    EXPECT_EQ(nodeOfKey("ch11.z1.offer"), 21);
    EXPECT_EQ(nodeOfKey("ch11.z2.greet"), 37);
    EXPECT_EQ(nodeOfKey("ch11.battle.tiandu_shi.intro"), 48);
    EXPECT_EQ(nodeOfKey("ch11.battle.tiandu.charge"), 48) << "最长前缀不能被 tiandu_shi 吞掉";
    EXPECT_EQ(nodeOfKey("ch11.path.luer_shengshan_dating.text"), 5);
    EXPECT_EQ(nodeOfKey("item.desc.story_bai_zhenzhu"), 62);
    EXPECT_EQ(nodeOfKey("objective.ch11.n01_chudao"), 1);
    EXPECT_EQ(nodeOfKey("objective.ch11.n04_zufu"), 7) << "第 4 步的提示在 n03 做完之后上屏";
    EXPECT_EQ(nodeOfKey("objective.ch11.n14_huicheng"), 25) << "第 13 步按第 12 步（n12）";
    EXPECT_EQ(nodeOfKey("objective.ch11.n29_liugen"), 55);
    EXPECT_EQ(nodeOfKey("ch11.nosuch.line"), kUnmapped);
    EXPECT_EQ(nodeOfKey("objective.ch11.n13_jiguan"), kUnmapped) << "n13 不在链上";
    EXPECT_EQ(nodeOfKey("ch10.chudao.road"), kUnmapped);
    for (const Ordering& rule : orderings())
        if (rule.first != kNoPositive) EXPECT_GE(rule.first, rule.from) << rule.word;
}

// ---- 先验：每一类检查都抓得住（在真实文案上注入一处违规，其余不动）----
TEST(Ch11LexiconTables, EachKindOfCheckCatchesAnInjectedViolation) {
    int files = 0;
    std::string error;
    const Entries real = chapterEntries(files, error);
    ASSERT_TRUE(error.empty()) << error;
    ASSERT_TRUE(allProblems(real).empty()) << joined(allProblems(real));
    const auto caught = [](const Entries& broken) { return !allProblems(broken).empty(); };
    // 章内先后：早说（指环那一场提鲁二）、删光（正向那一半）。
    EXPECT_TRUE(caught(with(real, "ch11.zhihuan.greet", "鲁二")));
    EXPECT_TRUE(caught(without(real, "鲁二")));
    // 审查点名的两处旧文：n04 提示早说三十九层（C-12）、雅间死分支早说紫灵（C-11）。
    EXPECT_TRUE(caught(with(real, "objective.ch11.n04_zufu", "山脚的楼阁管洞府租赁；三十九层那几座，百年一付。")));
    EXPECT_TRUE(caught(with(real, "ch11.chaguan.ziling", "紫灵姑娘的话，请前辈听她亲口说。")));
    // 查不到节点。
    EXPECT_TRUE(caught(with(real, "ch11.nosuch.line", "好。")));
    // 定点：出了场景、超了条数、少了条数。
    EXPECT_TRUE(caught(with(real, "ch11.huicheng.extra", "鸡冠蛟")));
    EXPECT_TRUE(caught(with(real, "ch11.wuchou.extra", "虚天殿")));
    EXPECT_TRUE(caught(without(real, "虚天残图")));
    EXPECT_TRUE(caught(with(real, "ch11.zhaolan.extra", "一贫如洗")));
    // 两个钩子词各一处、却拆成两句：定点那一条照样过，单句那一条必须报。
    Entries split = without(real, "虚天残图");
    split = with(split, "ch11.wuchou.hook_a", "虚天残图");
    split = with(split, "ch11.wuchou.hook_b", "虚天殿");
    EXPECT_TRUE(caught(split));
    // 0 处：普通词、按词边界的两个。
    for (const char* word : {"白玉蜘蛛", "天晶木", "隆兴客栈", "元磁神光", "十八天都尸", "天音门", "猎妖令", "三十多年"})
        EXPECT_TRUE(caught(with(real, "ch11.peizhu.extra", word))) << word;
    EXPECT_TRUE(caught(with(real, "ch11.peizhu.extra", "黄龙")));
    EXPECT_FALSE(caught(with(real, "ch11.peizhu.extra", "一粒黄龙丹")));
    EXPECT_TRUE(caught(with(real, "ch11.peizhu.extra", "闭关十余年")));
    EXPECT_FALSE(caught(with(real, "ch11.peizhu.extra", "开了二十余年")));
    // 洞府北口拒绝语点名星空殿。
    EXPECT_TRUE(caught(with(real, "ch11.block.xingkong", "星空殿在上面。")));
    // 分母先验：恰 250 条不够。
    Entries short250(real.begin(), real.begin() + 250);
    EXPECT_FALSE(denominatorHolds(short250, files));
    EXPECT_TRUE(denominatorHolds(real, files));
}

// ---- 正题：逐条扫 data/text/ch11_*.json ----
TEST_F(Ch11Lexicon, EveryLineIsPinnedToANodeOfTheMainLine) {
    std::vector<std::string> loose;
    for (const Entry& e : entries_)
        if (nodeOfKey(e.key) == kUnmapped) loose.push_back(e.key);
    EXPECT_TRUE(loose.empty()) << joined(loose);
}

TEST_F(Ch11Lexicon, NoWordIsSaidBeforeItsSceneAndEachIsSaidWhereTheDesignPutsIt) {
    std::vector<std::string> problems;
    for (const Ordering& rule : orderings()) {
        const auto more = orderingProblems(entries_, rule);
        problems.insert(problems.end(), more.begin(), more.end());
    }
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch11Lexicon, SceneBoundWordsStayInTheirScenesAndTheXutianHookIsOneLine) {
    std::vector<std::string> problems;
    for (const Only& rule : onlies()) {
        const auto more = onlyProblems(entries_, rule);
        problems.insert(problems.end(), more.begin(), more.end());
    }
    EXPECT_TRUE(problems.empty()) << joined(problems);
    int hook = 0;
    for (const Entry& e : entries_)
        if (has(e, "虚天残图") && has(e, "虚天殿")) {
            ++hook;
            EXPECT_EQ(e.key.rfind("ch11.wuchou.", 0), 0u) << e.key;
        }
    EXPECT_EQ(hook, 1);
}

TEST_F(Ch11Lexicon, TheWordsThisChapterMustNotSayAppearNowhere) {
    const auto problems = zeroProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch11Lexicon, TheWholeSpecialTableIsClean) {
    const auto problems = allProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

// E2：第十一章的章节卡两条都不提城与海（data/text/ui.json 的 ui.chapter.11.*）。
TEST(Ch11LexiconCards, TheChapterCardSaysNoneOfTheCityWords) {
    const Json ui = json(root() / "data" / "text" / "ui.json");
    int cards = 0;
    for (auto it = ui.begin(); it != ui.end(); ++it) {
        const std::string key = it.key();
        if (key.rfind("ui.chapter.11.", 0) != 0) continue;
        ++cards;
        const std::string text = it.value().get<std::string>();
        for (const char* word : {"天星城", "天星宫", "星宫", "内星海", "外星海", "妖海", "天星双圣", "摘星大会", "妖兽岛"})
            EXPECT_EQ(text.find(word), std::string::npos) << key << " says " << word;
    }
    EXPECT_EQ(cards, 2) << "ui.chapter.11.numeral / title";
}
}  // namespace
