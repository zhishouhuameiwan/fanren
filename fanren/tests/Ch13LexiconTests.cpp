// 第 13 章验收第 22、23 条：施工图 12.2 的专项断言（docs/ch13-design.md 12.1、12.2、16.1 第 43、44、55、56 条、第 15 节 22–23）。
//
// LexiconTests 只管「原著首见晚于本章区间 → 不许出现」（表一 + 表二 {13, 542}）。它管不住三类事，这里专项点名：
//   · 章内先后：本章可以说、但不能在它那一节之前说的词（12.2 表「X 之前不含」各行）；化名「曲」还有个尾巴：cunjin 之后 0 处。
//   · 只许在某处 / 某处不许：妙鹤只在 chongmo、mishi、wenshi；万三谷、六道极圣只在 wenxun；迷幻砚、辟邪神雷不进 ① 的
//     开场白、蓄势与 zhuibing.*；② 的开场白不点「巨虾」。
//   · 全章 0 处：12.2 「全章 0 处」各行（七煞决、刘护法、厌瑙；第 14 章及以后；暗场；原著笔误；摘要造词与尺度）、
//     「妖修之战」、16.1 第 42 条的「导火索」「大祸」、13 第 14 条的「未完待续」。
// 另有读 role 名牌的一条：liu_hufa、min_xiushi、xuan_xiushi 的名牌不含姓（16.1 第 44 条）；新物品 / 新法术的显示名照同一套
// 节点核（名字不在 data/text 里）；表一含 12.1 的 82 词与两个禁词（读 tests/LexiconTests.cpp 原文）。
//
// 节点怎么编（照施工图 12.2「断言的写法」，不照脚本推）：
//   · 场景前缀 ch13.<场景>.* 编在 3.4 目标链的 n 号上（场景名 ＝ 节点脚本名；done_flag 去掉 ch13. 就是场景名，只有
//     lieyao → youyao、jiaohuan → mishi、done → banyao 三处例外）；第 12 章 lingquan（本章开局那一行 teleport）记 0。
//   · 「编在 X」一律记 X 的号：往返两份（qiyuan_hang.*、hui_wuzhong.*）编在 lifu；Z2（xuanshang.*）编在 daoqiyuan；
//     Z3（yanghun.*）编在 buzhen；feixu.* 编在 gongsun；路径行动按 when 里最晚的链上旗标；物品描述按 give 它的脚本
//    （玛瑙角没有 give，按最早合法的 caigou，18.3 第 7 条；至元丹随配方在 liandan）；战斗文案按调用它的脚本（① zhuibing、
//     ② youyao；儒生的黑雾怪鸟随 ①）；地图名按第一次 teleport 进去的脚本（nanming = 0、tianxing → yinjian、
//     waihai → chuansong、wuzhong → chucheng）；目标链第 k 步的提示按第 k−1 步，第 1 步记 0。
//   · 12.2 没写的几类照同一口径补：坊市闭门（fangshi_bimen.*）在 tianxing 落地之后 → yinjian；任务文案按接取旗标
//    （18.3 第 2、3 条：公孙杏那条在 gongsun 之前不点她的名、千叶露那条不提伴妖草）；店名按开店的脚本。
//   · 表里查不到的 key 直接判红：新加文案不登节点，等于绕开这一层。
//
// 这个文件看不见什么：文案 key 的场景段与「真正调用它的脚本」是否一致（信 key 名）；「黄道友」lidao 起是否只指黄明礼（语义）；
// 原创性（最长公共片段归复验脚本，不归测试）。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include "Ch13Data.h"

namespace {
using namespace fanren::test::ch13;

constexpr int kUnmapped = -1;

// 3.4 目标链 34 步的场景名（= 节点脚本名），下标 + 1 就是 n 号。
const std::vector<const char*>& sceneNames() {
    static const std::vector<const char*> kScenes{
        "wenxun", "caigou", "gongdao", "panzha", "zhuibing", "yinjian", "rucheng", "qifu", "tanzhen", "qianru", "yirong",
        "chuansong", "daoqiyuan", "mingzhu", "cunjin", "chucheng", "lifu", "chongshi", "xuanyin", "chonglian", "dujie",
        "wusezhu", "buzhen", "youyao", "chongmo", "liandan", "biguan", "chuguan", "gongsun", "lidao", "mishi", "wenshi",
        "jianshi", "banyao"};
    return kScenes;
}

int sceneNode(const std::string& scene) {
    const auto& all = sceneNames();
    for (std::size_t i = 0; i < all.size(); ++i)
        if (scene == all[i]) return static_cast<int>(i) + 1;
    return kUnmapped;
}

struct Prefix { const char* prefix; int node; };

// 场景前缀以外的那些 key（12.2「断言的写法」逐类；n 号见 sceneNames）。
const std::vector<Prefix>& extraPrefixes() {
    static const std::vector<Prefix> kTable = {
        {"ch13.qiyuan_hang.", 17}, {"ch13.hui_wuzhong.", 17}, {"ch13.xuanshang.", 13}, {"ch13.yanghun.", 23},
        {"ch13.feixu.", 29}, {"ch13.fangshi_bimen.", 6},
        {"ch13.quest.qianyelu.", 2}, {"ch13.quest.gongsun_qiuyao.", 28}, {"ch13.quest.mingzhu_dingdan.", 14},
        {"ch13.quest.haishou_xuanshang.", 13}, {"ch13.quest.yanghun_zhu.", 17},
        {"ch13.shop.name.nanming_fangshi", 2}, {"ch13.shop.name.mingzhu_shougou", 14}, {"ch13.shop.name.cunjin_ge", 15},
        {"ch13.shop.name.mishi_tan", 31},
        {"ch13.battle.nanming_zhuibing.", 5}, {"ch13.battle.waixinghai_lieyao.", 24},
        {"ch13.map.nanming.", 0}, {"ch13.map.tianxing.", 6}, {"ch13.map.waihai.", 12}, {"ch13.map.wuzhong.", 16},
        {"magic.desc.magic_ji_wuxinghuan", 1}, {"magic.desc.magic_fengyun_heji", 20}, {"magic.desc.magic_mihuan_guniao", 5},
        // 物品描述：give 它的脚本（第 9 节来源表、3.2）。
        {"item.desc.story_mihuan_yan", 5}, {"item.desc.story_baiyu_dayin", 5}, {"item.desc.story_huangse_zhihuan", 7},
        {"item.desc.story_manao_jiao", 2}, {"item.desc.story_guyu_yujian", 15}, {"item.desc.story_qiyuan_haitu", 16},
        {"item.desc.story_wuse_yuanzhu", 18}, {"item.desc.story_xuanyin_jing", 19}, {"item.desc.story_yanghun_zhu", 23},
        {"item.desc.story_liuqi_cailiao", 25}, {"item.desc.pill_zhiyuan_dan", 26}, {"item.desc.story_tongpian", 31},
        {"item.desc.story_banyao_cao", 34},
        // 路径行动：when 里最晚的链上旗标（16.3）。
        {"ch13.path.sanxian_tianxingling_dating.", 1}, {"ch13.path.liuhufa_shaozhu_dating.", 3},
        {"ch13.path.xinming_xinwu_dating.", 3}, {"ch13.path.qianxiushi_chuansong_dating.", 11},
        {"ch13.path.zhaohufa_jieyan_dating.", 6}, {"ch13.path.yuhufa_chuansong_dating.", 7},
        {"ch13.path.zhangxiushi_jiama_dating.", 11}, {"ch13.path.yijing_waihai_dating.", 12},
        {"ch13.path.xuyun_qiyuan_dating.", 13}, {"ch13.path.paimaishi_shouluan_dating.", 14},
        {"ch13.path.guyu_shanggu_dating.", 15}, {"ch13.path.sunshishu_shouchao_dating.", 28},
        {"ch13.path.huangmingli_mishi_dating.", 30},
    };
    return kTable;
}

constexpr const char* kObjective = "objective.ch13.n";

int nodeOfKey(const std::string& key) {
    if (key.rfind(kObjective, 0) == 0) {
        // objective.ch13.nXX_<scene>：第 k 步的提示按第 k−1 步编号，第 1 步记 0（第 12 章 lingquan）。
        const std::string rest = key.substr(std::string(kObjective).size());
        const auto bar = rest.find('_');
        if (bar != 2 || !std::isdigit(static_cast<unsigned char>(rest[0])) || !std::isdigit(static_cast<unsigned char>(rest[1])))
            return kUnmapped;
        const int n = std::stoi(rest.substr(0, 2));
        if (n < 1 || n > 34 || sceneNode(rest.substr(3)) != n) return kUnmapped;
        return n - 1;
    }
    int best = kUnmapped;
    std::size_t length = 0;
    for (const Prefix& entry : extraPrefixes()) {
        const std::string prefix(entry.prefix);
        if (key.rfind(prefix, 0) == 0 && prefix.size() > length) { best = entry.node; length = prefix.size(); }
    }
    if (best != kUnmapped) return best;
    if (key.rfind("ch13.", 0) == 0) {   // ch13.<场景>.<用途>
        const auto dot = key.find('.', 5);
        if (dot != std::string::npos) return sceneNode(key.substr(5, dot - 5));
    }
    return kUnmapped;
}

// 12.2 表「X 之前不含」：一行一个场景（同一场景的几行并在一起）。正向那一半按行：每一行至少有一个词真被说到
//（且不早于该场景），防「整行词被删光、只查没有的那一半恒真」——迷幻砚那一行除外：它只在儒生心里叫出（12.2），
// 文案里本就一处不该有，名字只作物品名（自 zhuibing 的 give 起）。「厉」单字不登，按称呼的几种写法查。
struct Row { const char* scene; std::vector<const char*> words; bool mustBeSaid = true; };

const std::vector<Row>& rows() {
    static const std::vector<Row> kRows = {
        {"wenxun", {"千叶露"}},
        {"caigou", {"玛瑙角", "玛瑙兽"}},
        {"gongdao", {"苍云龙", "王长老", "辛明", "开天门", "白水剑", "少主", "圣祖"}},
        {"zhuibing", {"迷幻砚"}, false},
        {"yinjian", {"外事执法", "天香索"}},
        {"rucheng", {"赵护法", "于护法", "定颜丹", "黄色指环"}},
        {"qianru", {"流水阵", "刘夫人", "易兄"}},
        {"chuansong", {"奇渊"}},
        {"daoqiyuan", {"易敬", "许云", "黑石城", "奇渊岛", "竞斗场", "联合大拍卖"}},
        {"mingzhu", {"明夫人", "鉴宝厅", "竞宝厅", "蓝光石", "紫云鹰", "妖兽海渊", "寸金阁"}},
        {"cunjin", {"离寻", "古玉", "伴妖草"}},
        {"chucheng", {"雾中岛", "雾海", "吞海鱼", "银翼鸟"}},
        {"chongshi", {"幻焰蛾", "五色圆珠"}},
        {"xuanyin", {"玄阴经", "玄阴大法", "天都尸火", "辟邪神雷"}},
        {"dujie", {"化形雷劫", "毒蛟", "金霞", "霍氏", "伪洞府"}},
        {"buzhen", {"巨虾"}},
        {"chongmo", {"琉璃兽", "三色藻", "四法上人", "虫魔", "碧云门", "三色噬金虫"}},
        {"liandan", {"至元丹"}},
        {"chuguan", {"青灵门", "鹰鸢兽", "兽潮"}},
        {"gongsun", {"公孙杏", "青灵玄功", "金真功", "缠玉决"}},
        {"lidao", {"换形诀", "黄明礼", "南黎岛", "秘市", "听风阁", "厉某", "厉前辈", "厉道友", "厉兄", "黄道友"}},
        {"mishi", {"交换会", "云天啸", "盘黎蚓", "卧虎鲨", "化海兽", "天灵子", "青夫人", "灵骨", "白骨阴火", "幻梦石", "裂风兽"}},
        {"wenshi", {"厉先生"}},
        {"jianshi", {"妖修", "三头六臂"}},
    };
    return kRows;
}

// 化名「曲」（16.1 第 43 条）：yirong 之前不含、cunjin 之后 0 处——只在 [yirong, cunjin] 之间，且至少说过一次。
// 「姓曲」不算：panzha.* 里「一位姓韩，一位姓曲」说的是第 11 章那位客卿（曲魂），不是化名。
const std::vector<const char*>& aliasWords() {
    static const std::vector<const char*> kWords{"曲道友", "曲兄", "曲某"};
    return kWords;
}

// 只许出现在这几个前缀下（12.2「只闻其名」两行）。
struct Only { const char* word; std::vector<const char*> where; };

const std::vector<Only>& onlies() {
    static const std::vector<Only> kRules = {
        {"妙鹤", {"ch13.chongmo.", "ch13.mishi.", "ch13.wenshi."}},
        {"万三谷", {"ch13.wenxun."}},
        {"六道极圣", {"ch13.wenxun."}},
    };
    return kRules;
}

// 这几个前缀下不许出现（12.2 迷幻砚、辟邪神雷、巨虾三行）。
struct Never { const char* word; std::vector<const char*> where; };

const std::vector<Never>& nevers() {
    static const std::vector<Never> kRules = {
        {"迷幻砚", {"ch13.zhuibing.", "ch13.battle.nanming_zhuibing."}},
        {"辟邪神雷", {"ch13.zhuibing.", "ch13.battle.nanming_zhuibing."}},
        {"巨虾", {"ch13.battle.waixinghai_lieyao.intro"}},
    };
    return kRules;
}

// 全章 0 处（12.2「全章 0 处」各行与「妖修之战」；16.1 第 42 条；13 第 14 条）。
const std::vector<const char*>& zeroWords() {
    static const std::vector<const char*> kWords = {
        "七煞决", "刘护法", "厌瑙", "妖修之战", "风希", "风某", "风雷翅", "碧焰酒", "梵圣真片", "温天仁", "银月", "紫罗极火",
        "磷火潭", "九级", "化形第二阶", "极阴", "齐姓", "漩矶", "玄阴岛", "云啸天", "历道友", "历某", "万年灵液", "只出不进",
        "雾阵", "六眼巨虾", "物价", "灭族", "丧夫", "炉鼎", "双修", "导火索", "大祸", "未完待续"};
    return kWords;
}

struct Entry { std::string key; std::string text; };
using Entries = std::vector<Entry>;

bool has(const Entry& e, const std::string& word) { return e.text.find(word) != std::string::npos; }
bool under(const std::string& key, const std::vector<const char*>& where) {
    return std::any_of(where.begin(), where.end(), [&](const char* p) { return key.rfind(p, 0) == 0; });
}

std::vector<std::string> rowProblems(const Entries& entries, const Row& row) {
    std::vector<std::string> out;
    const int from = sceneNode(row.scene);
    bool said = false;
    for (const char* word : row.words)
        for (const Entry& e : entries) {
            const int node = nodeOfKey(e.key);
            if (node == kUnmapped || !has(e, word)) continue;
            if (node < from) out.push_back(std::string(word) + " too early in " + e.key);
            else said = true;
        }
    if (row.mustBeSaid && !said) out.push_back(std::string("no word of the ") + row.scene + " row is said at all");
    return out;
}

std::vector<std::string> aliasProblems(const Entries& entries) {
    std::vector<std::string> out;
    const int from = sceneNode("yirong"), to = sceneNode("cunjin");
    int said = 0;
    for (const Entry& e : entries)
        for (const char* word : aliasWords()) {
            if (!has(e, word)) continue;
            const int node = nodeOfKey(e.key);
            if (node != kUnmapped && (node < from || node > to)) out.push_back(std::string(word) + " outside yirong..cunjin in " + e.key);
            else ++said;
        }
    if (said == 0) out.push_back("the alias Qu is never said");
    return out;
}

std::vector<std::string> placeProblems(const Entries& entries) {
    std::vector<std::string> out;
    for (const Entry& e : entries) {
        for (const Only& rule : onlies())
            if (has(e, rule.word) && !under(e.key, rule.where)) out.push_back(std::string(rule.word) + " outside its scene: " + e.key);
        for (const Never& rule : nevers())
            if (has(e, rule.word) && under(e.key, rule.where)) out.push_back(std::string(rule.word) + " in " + e.key);
        for (const char* word : zeroWords())
            if (has(e, word)) out.push_back(std::string(word) + " in " + e.key);
    }
    return out;
}

std::vector<std::string> allProblems(const Entries& entries) {
    std::vector<std::string> out;
    const auto append = [&out](const std::vector<std::string>& more) { out.insert(out.end(), more.begin(), more.end()); };
    for (const Entry& e : entries)
        if (nodeOfKey(e.key) == kUnmapped) out.push_back("unmapped " + e.key);
    for (const Row& row : rows()) append(rowProblems(entries, row));
    append(aliasProblems(entries));
    append(placeProblems(entries));
    return out;
}

// 第 15 节 22 条：分母先验 > 250 条；十一份文案文件（正文六份 ＋ battle / items / maps / objectives / path / quests 拆法，
// 18.1 首段与第 11 条）。
bool denominatorHolds(const Entries& entries, int files) { return entries.size() > 250u && files >= 11; }

Entries chapterEntries(int& files, std::string& error) {
    Entries out;
    files = 0;
    std::vector<fs::path> paths;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text"))
        if (file.path().extension() == ".json" && file.path().filename().string().rfind("ch13", 0) == 0) paths.push_back(file.path());
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

class Ch13Lexicon : public ::testing::Test {
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

// 节点表与落地的目标链同序：第 k 步的 done_flag 换成场景名（三处例外）恰是表里第 k 个。
std::vector<std::string> chainOrderProblems(const Json& steps) {
    std::vector<std::string> out;
    const std::map<std::string, std::string> renamed{{"ch13.lieyao", "youyao"}, {"ch13.jiaohuan", "mishi"}, {"ch13.done", "banyao"}};
    if (steps.size() != sceneNames().size()) out.push_back("chain has " + std::to_string(steps.size()) + " steps");
    for (std::size_t i = 0; i < steps.size() && i < sceneNames().size(); ++i) {
        const std::string flag = steps[i].value("done_flag", "");
        const auto special = renamed.find(flag);
        const std::string scene = special != renamed.end() ? special->second : flag.rfind("ch13.", 0) == 0 ? flag.substr(5) : flag;
        if (scene != sceneNames()[i]) out.push_back("step " + std::to_string(i + 1) + " is " + flag);
        if (steps[i].value("id", "") != "n" + std::string(i < 9 ? "0" : "") + std::to_string(i + 1) + "_" + sceneNames()[i])
            out.push_back("step " + std::to_string(i + 1) + " id " + steps[i].value("id", ""));
    }
    return out;
}

// ---- 先验：节点表本身（字面量抄施工图 3.4 / 12.2，不从 key 反推）----
TEST(Ch13LexiconTables, TheNodeTableIsSaneAndPinsTheDesignedNodes) {
    ASSERT_EQ(sceneNames().size(), 34u) << "施工图 3.4：目标链 34 步";
    const Json steps = json(root() / "data" / "objectives" / "ch13.json").at("steps");
    EXPECT_TRUE(chainOrderProblems(steps).empty()) << joined(chainOrderProblems(steps));
    Json swapped = steps;
    std::swap(swapped[13], swapped[14]);
    EXPECT_FALSE(chainOrderProblems(swapped).empty()) << "负向：mingzhu 与 cunjin 对调必须报";
    EXPECT_EQ(nodeOfKey("ch13.wenxun.sea"), 1);
    EXPECT_EQ(nodeOfKey("ch13.banyao.voice"), 34);
    EXPECT_EQ(nodeOfKey("ch13.youyao.win"), 24) << "done_flag 是 lieyao，场景名是 youyao";
    EXPECT_EQ(nodeOfKey("ch13.qiyuan_hang.go"), 17) << "往返编在 lifu";
    EXPECT_EQ(nodeOfKey("ch13.xuanshang.short"), 13) << "Z2 编在 daoqiyuan";
    EXPECT_EQ(nodeOfKey("ch13.yanghun.young"), 23) << "Z3 编在 buzhen";
    EXPECT_EQ(nodeOfKey("ch13.feixu.ruin"), 29) << "feixu 编在 gongsun";
    EXPECT_EQ(nodeOfKey("ch13.map.nanming.name"), 0) << "南明岛随第 12 章 lingquan 那一行 teleport";
    EXPECT_EQ(nodeOfKey("ch13.map.wuzhong.name"), 16);
    EXPECT_EQ(nodeOfKey("ch13.battle.waixinghai_lieyao.intro"), 24);
    EXPECT_EQ(nodeOfKey("ch13.path.xuyun_qiyuan_dating.text"), 13) << "K9 when 最晚是 daoqiyuan";
    EXPECT_EQ(nodeOfKey("ch13.path.yijing_waihai_dating.text"), 12) << "K8 编在 chuansong（18.3 第 7 条）";
    EXPECT_EQ(nodeOfKey("item.desc.story_manao_jiao"), 2) << "18.3 第 7 条";
    EXPECT_EQ(nodeOfKey("ch13.quest.gongsun_qiuyao.title"), 28);
    EXPECT_EQ(nodeOfKey("objective.ch13.n01_wenxun"), 0);
    EXPECT_EQ(nodeOfKey("objective.ch13.n02_caigou"), 1) << "16.1 第 55 条：第 2 步提示编在 wenxun";
    EXPECT_EQ(nodeOfKey("objective.ch13.n16_chucheng"), 15) << "16.1 第 55 条：第 16 步提示编在 cunjin";
    EXPECT_EQ(nodeOfKey("objective.ch13.n24_lieyao"), kUnmapped) << "号与场景对不上";
    EXPECT_EQ(nodeOfKey("ch13.nosuch.line"), kUnmapped);
    EXPECT_EQ(nodeOfKey("ch12.wenxun.sea"), kUnmapped);
    EXPECT_EQ(nodeOfKey("ch13.block.yaoyuan"), kUnmapped) << "验收 6、16.1 第 45 条：ch13.block.* 不建，建了即「查不到节点」";
    for (const Row& row : rows()) EXPECT_NE(sceneNode(row.scene), kUnmapped) << row.scene;
}

// ---- 先验：每一类检查都抓得住（在真实文案上注入一处违规，其余不动）----
TEST(Ch13LexiconTables, EachKindOfCheckCatchesAnInjectedViolation) {
    int files = 0;
    std::string error;
    const Entries real = chapterEntries(files, error);
    ASSERT_TRUE(error.empty()) << error;
    ASSERT_TRUE(allProblems(real).empty()) << joined(allProblems(real));
    const auto caught = [](const Entries& broken) { return !allProblems(broken).empty(); };
    // 章内先后：早说（各幕的典型）。
    EXPECT_TRUE(caught(with(real, "ch13.wenxun.extra", "玛瑙角")));
    EXPECT_TRUE(caught(with(real, "ch13.map.nanming.name", "千叶露"))) << "地图名记 0，早于 wenxun";
    EXPECT_TRUE(caught(with(real, "ch13.chuansong.extra", "易敬"))) << "n12 只称易兄、锦衣大汉";
    EXPECT_TRUE(caught(with(real, "ch13.path.yijing_waihai_dating.text", "黑石城"))) << "K8 编在 chuansong";
    EXPECT_TRUE(caught(with(real, "ch13.path.zhaohufa_jieyan_dating.text", "于护法"))) << "K5 编在 yinjian";
    EXPECT_TRUE(caught(with(real, "ch13.quest.gongsun_qiuyao.title", "公孙杏之求"))) << "18.3 第 2 条";
    EXPECT_TRUE(caught(with(real, "ch13.quest.qianyelu.s2", "伴妖草"))) << "18.3 第 3 条";
    EXPECT_TRUE(caught(with(real, "objective.ch13.n15_cunjin", "去寸金阁问伴妖草"))) << "第 15 步提示编在 mingzhu";
    EXPECT_TRUE(caught(with(real, "ch13.qiyuan_hang.extra", "五色圆珠"))) << "往返编在 lifu，早于 chongshi";
    EXPECT_TRUE(caught(with(real, "ch13.dujie.extra", "妖修"))) << "妖修自 jianshi 起";
    EXPECT_TRUE(caught(with(real, "ch13.battle.nanming_zhuibing.intro", "外事执法")));
    // 整行删光（正向那一半）。
    Entries bare = real;
    for (const char* word : {"妖修", "三头六臂"}) bare = without(bare, word);
    EXPECT_TRUE(caught(bare));
    // 查不到节点。
    EXPECT_TRUE(caught(with(real, "ch13.nosuch.line", "好。")));
    // 化名「曲」：早说、晚说、一处不说。
    EXPECT_TRUE(caught(with(real, "ch13.qianru.extra", "曲某")));
    EXPECT_TRUE(caught(with(real, "ch13.chucheng.extra", "曲道友"))) << "cunjin 之后 0 处";
    Entries noAlias = real;
    for (const char* word : aliasWords()) noAlias = without(noAlias, word);
    EXPECT_TRUE(caught(noAlias));
    // 定点：只许在某处、某处不许。
    EXPECT_TRUE(caught(with(real, "ch13.chongmo.extra", "万三谷")));
    EXPECT_TRUE(caught(with(real, "ch13.lidao.extra", "妙鹤")));
    EXPECT_TRUE(caught(with(real, "ch13.zhuibing.extra", "迷幻砚")));
    EXPECT_TRUE(caught(with(real, "ch13.battle.nanming_zhuibing.charge", "辟邪神雷")));
    EXPECT_TRUE(caught(with(real, "ch13.battle.waixinghai_lieyao.intro", "巨虾")));
    EXPECT_FALSE(caught(with(real, "ch13.battle.waixinghai_lieyao.charge", "巨虾"))) << "对照：第 1 波现身之后的蓄势可以点名";
    EXPECT_FALSE(caught(with(real, "ch13.mishi.extra", "妙鹤真人"))) << "对照：秘市里范夫人提起";
    // 0 处。
    for (const char* word : {"七煞决", "刘护法", "梵圣真片", "风希", "九级", "极阴", "漩矶", "云啸天", "万年灵液", "导火索", "未完待续"})
        EXPECT_TRUE(caught(with(real, "ch13.banyao.extra", word))) << word;
    // 分母先验：恰 250 条不够；少一份文件不够。
    Entries short250(real.begin(), real.begin() + 250);
    EXPECT_FALSE(denominatorHolds(short250, files));
    EXPECT_FALSE(denominatorHolds(real, 10));
    EXPECT_TRUE(denominatorHolds(real, files));
}

// ---- 正题：逐条扫 data/text/ch13*.json ----
TEST_F(Ch13Lexicon, EveryLineIsPinnedToANodeOfTheMainLine) {
    std::vector<std::string> loose;
    for (const Entry& e : entries_)
        if (nodeOfKey(e.key) == kUnmapped) loose.push_back(e.key);
    EXPECT_TRUE(loose.empty()) << joined(loose);
}

TEST_F(Ch13Lexicon, NoWordIsSaidBeforeItsSceneAndEveryRowIsSaidSomewhere) {
    std::vector<std::string> problems;
    for (const Row& row : rows()) {
        const auto more = rowProblems(entries_, row);
        problems.insert(problems.end(), more.begin(), more.end());
    }
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch13Lexicon, TheAliasQuLivesOnlyFromTheDisguiseToTheHouseOfAnInch) {
    const auto problems = aliasProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch13Lexicon, SceneBoundAndZeroWordsStayWhereTheDesignPutsThem) {
    const auto problems = placeProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch13Lexicon, TheWholeSpecialTableIsClean) {
    const auto problems = allProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

// ---- 16.1 第 44 条：名字韩立从没当面听到的三人用不含姓的描述名牌（role 的 name），role id 不动 ----
std::vector<std::string> plateProblems(const std::map<std::string, std::string>& names) {
    std::vector<std::string> out;
    const std::pair<const char*, const char*> hidden[] = {{"liu_hufa", "刘"}, {"min_xiushi", "闵"}, {"xuan_xiushi", "宣"}};
    for (const auto& [id, surname] : hidden) {
        const auto found = names.find(id);
        if (found == names.end() || found->second.empty() || found->second.find(surname) != std::string::npos)
            out.push_back(std::string(id) + " plate carries the surname " + surname);
    }
    return out;
}

std::map<std::string, std::string> rolePlates() {
    std::map<std::string, std::string> names;
    for (const auto& file : fs::recursive_directory_iterator(root() / "data" / "roles")) {
        if (file.path().extension() != ".json") continue;
        const Json role = json(file.path());
        names[role.value("id", file.path().stem().string())] = role.value("name", "");
    }
    return names;
}

TEST(Ch13LexiconPlates, TheNamesHanLiNeverHeardStayOffThePlates) {
    const auto names = rolePlates();
    ASSERT_GT(names.size(), 100u) << "先验：读得到 data/roles";
    const auto problems = plateProblems(names);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    for (const auto& [id, plate] : {std::pair<const char*, const char*>{"liu_hufa", "刘护法"}, {"min_xiushi", "闵姓修士"},
                                    {"xuan_xiushi", "宣姓修士"}}) {
        auto bad = names;
        bad[id] = plate;
        EXPECT_FALSE(plateProblems(bad).empty()) << id;
    }
}

// ---- 新物品 / 新法术的显示名也按 give / learn 它的节点编号 ----
// 名字不在 data/text 里（在 data/items/**、data/magics/** 的 name），上面那几条扫不到；拿到手那一刻就上屏，
// 所以节点照 extraPrefixes 里 item.desc.<id> / magic.desc.<id> 那一格。迷幻砚的物品名自 zhuibing 的 give 起（12.2）。
struct Named { std::string key; std::string name; };

std::vector<std::string> nameProblems(const std::vector<Named>& names) {
    std::vector<std::string> out;
    for (const Named& n : names) {
        const int node = nodeOfKey(n.key);
        if (node == kUnmapped || n.name.empty()) { out.push_back("no node or no name for " + n.key); continue; }
        for (const Row& row : rows())
            for (const char* word : row.words)
                if (n.name.find(word) != std::string::npos && node < sceneNode(row.scene))
                    out.push_back(n.key + " name says " + word + " too early");
        for (const char* word : zeroWords())
            if (n.name.find(word) != std::string::npos) out.push_back(n.key + " name says " + word);
        for (const Only& rule : onlies())
            if (n.name.find(rule.word) != std::string::npos) out.push_back(n.key + " name says " + rule.word);
    }
    return out;
}

std::vector<Named> chapterNames() {
    std::map<std::string, std::string> byId;
    for (const char* dir : {"items", "magics"})
        for (const auto& file : fs::recursive_directory_iterator(root() / "data" / dir)) {
            if (file.path().extension() != ".json") continue;
            const Json data = json(file.path());
            if (data.is_object() && data.contains("id") && data.contains("name") && data.at("name").is_string())
                byId[data.at("id").get<std::string>()] = data.at("name").get<std::string>();
        }
    std::vector<Named> out;
    for (const Prefix& entry : extraPrefixes()) {
        const std::string key(entry.prefix);
        for (const std::string head : {"item.desc.", "magic.desc."}) {
            if (key.rfind(head, 0) != 0) continue;
            const auto found = byId.find(key.substr(head.size()));
            out.push_back({key, found == byId.end() ? std::string() : found->second});
        }
    }
    return out;
}

std::vector<Named> renamed(std::vector<Named> names, const std::string& key, const std::string& name) {
    for (Named& n : names)
        if (n.key == key) n.name = name;
    return names;
}

TEST(Ch13LexiconPlates, NewItemAndMagicNamesFollowTheSceneThatHandsThemOver) {
    const auto names = chapterNames();
    ASSERT_EQ(names.size(), 16u) << "先验：L.1 十三件物品 ＋ L.2 三门法术";
    const auto problems = nameProblems(names);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    const auto slip = std::find_if(names.begin(), names.end(), [](const Named& n) { return n.key == "item.desc.story_tongpian"; });
    ASSERT_NE(slip, names.end());
    EXPECT_EQ(slip->name, "铜片") << "16.1 第 27 条";
    // 负向：n18 给的那一匣叫出 n19 才有的名字、铜片叫真名、n01 学的环叫出黑石城、一件物品查不到名字。
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_wuse_yuanzhu", "玄阴经匣")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_tongpian", "梵圣真片")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "magic.desc.magic_ji_wuxinghuan", "黑石城五行环")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_banyao_cao", "")).empty());
}

// ---- 第 22 条前半：表一含 12.1 的 82 词与两个禁词（读 tests/LexiconTests.cpp 原文，不经它的判据）----
// 首见章号的区间照 12.1 表的五档；「早于 499」三词与两个禁词各钉死 12.1 写的那一章。三仙宗（499）第 12 章已登，照登在表一。
struct Band { int from; int to; std::vector<const char*> words; };

const std::vector<Band>& tableOneBands() {
    static const std::vector<Band> kBands = {
        {499, 505, {"三仙宗", "天星令", "逆星令", "千叶露", "玛瑙角", "玛瑙兽", "苍云龙", "王长老", "辛明", "开天门", "白水剑",
                    "刘护法", "七煞决", "迷幻砚", "白玉大印", "外事执法", "天香索"}},
        {506, 517, {"于护法", "赵护法", "黄色指环", "流水阵", "刘夫人", "易敬", "许云", "竞斗场", "明夫人", "鉴宝厅", "竞宝厅",
                    "妖兽海渊", "蓝光石", "紫云鹰", "联合大拍卖", "寸金阁", "离寻", "古玉", "伴妖草", "吞海鱼", "银翼鸟"}},
        {518, 529, {"幻焰蛾", "五色圆珠", "化形雷劫", "毒蛟", "金霞", "霍氏", "妖修", "伪洞府", "巨虾", "琉璃兽", "三色藻",
                    "四法上人", "虫魔", "碧云门", "妙鹤真人", "三色噬金虫"}},
        {530, 541, {"青灵门", "鹰鸢兽", "兽潮", "公孙杏", "青灵玄功", "至元丹", "金真功", "缠玉决", "换形诀", "黄明礼", "南黎岛",
                    "秘市", "听风阁", "交换会", "云天啸", "盘黎蚓", "卧虎鲨", "化海兽", "天灵子", "青夫人", "灵骨", "白骨阴火",
                    "幻梦石", "厉先生", "三头六臂"}},
        {427, 427, {"圣祖"}}, {407, 407, {"玄阴大法"}}, {345, 345, {"奇虫榜"}},
        {546, 546, {"梵圣真片"}}, {1038, 1038, {"困妖"}},
    };
    return kBands;
}

std::vector<std::string> tableOneProblems(const std::string& source) {
    std::vector<std::string> out;
    for (const Band& band : tableOneBands())
        for (const char* word : band.words) {
            const std::string head = "{\"" + std::string(word) + "\", ";
            const auto at = source.find(head);
            if (at == std::string::npos) { out.push_back(std::string(word) + " missing from table one"); continue; }
            std::size_t end = at + head.size();
            while (end < source.size() && std::isdigit(static_cast<unsigned char>(source[end]))) ++end;
            const std::string digits = source.substr(at + head.size(), end - at - head.size());
            const int chapter = digits.empty() ? 0 : std::stoi(digits);
            if (chapter < band.from || chapter > band.to) out.push_back(std::string(word) + " first seen " + digits);
        }
    return out;
}

TEST(Ch13LexiconTables, TableOneCarriesTheEightyTwoWordsAndTheTwoBannedOnes) {
    std::size_t total = 0;
    for (const Band& band : tableOneBands()) total += band.words.size();
    ASSERT_EQ(total, 84u) << "12.1：82 词 ＋ 禁词 2";
    const std::string source = read(root() / "tests" / "LexiconTests.cpp");
    const auto problems = tableOneProblems(source);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    const auto swap = [&source](const std::string& from, const std::string& to) {
        std::string text = source;
        const auto at = text.find(from);
        if (at != std::string::npos) text.replace(at, from.size(), to);
        return text;
    };
    ASSERT_NE(source.find("{\"盘黎蚓\", "), std::string::npos);
    EXPECT_FALSE(tableOneProblems(swap("{\"盘黎蚓\", ", "{\"盘犁蚓\", ")).empty()) << "少登一词必须报";
    EXPECT_FALSE(tableOneProblems(swap("{\"困妖\", 1038,", "{\"困妖\", 499,")).empty()) << "禁词的首见章改早必须报";
    EXPECT_FALSE(tableOneProblems(swap("{\"至元丹\", 532,", "{\"至元丹\", 552,")).empty()) << "首见章出了档必须报";
}
}  // namespace
