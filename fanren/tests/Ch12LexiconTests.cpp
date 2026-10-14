// 第 12 章验收第 21、22 条：施工图 12.2 的专项断言（docs/ch12-design.md 12.1、12.2、第 15 节 21–22）。
//
// LexiconTests 只管「原著首见晚于本章区间 → 不许出现」（表一 + 表二 {12, 499}）。它管不住三类事，这里专项点名：
//   · 章内先后：本章可以说、但不能在它那一节之前说的词（12.2 表「X 之前不含」各行）。
//   · 恰几处 / 只许在某处：极炫、萧诧各恰 1 处；玉真人、辟火宝衣只在 Z1 与 rongyan.* 那一句；血色披风只在 lingquan.*。
//   · 全章 0 处：灵犀佩、自焚、杀死玄骨，与 12.2 末两行（银月、紫罗极火……三阳老魔、天星城出了大事）。
// 另有三条读 role 名牌的：dixue_guiying、xuangu_shaonian 的 name 不含「玄」；yuan_yao_heipao 的 name 不含「元」；
// 任何 role 的 name 不含「萧诧」（12.2 玄骨、萧诧、元瑶三行）。新物品 / 新法术的显示名照上面同一套节点核
// （16.1 第 42 条「红光画轴」；血色披风是物品名例外）；quba.* 写「暗红披风」（12.2 血色披风一行的正向）。
//
// 节点怎么编（照施工图 12.2「断言的写法」，不照脚本推）：
//   · 场景前缀 ch12.<场景>.* 编在 3.4 目标链的 n 号上，统一乘 2：nX 里演的记 S(X) = 2X，nX 做完之后才露的记 A(X) = 2X+1；
//     旗标 → 场景：done_flag 去掉 ch12. 就是场景名，只有 tuizhu → guairen、canyuan → xiaocha、done → lingquan 三处例外。
//   · 走偏（zoupian.*）与 Z1（yihai.*、quest.maigu.*、辟火宝衣的描述）编在 tupo 与 guiwang 之间 = A(12)；
//     采药（caiyao.*）编在 xiaojie 与 lingshen 之间 = A(15)；玉台（yutai.*）在 shidian 送入之后 = A(23)；
//     Z2（quest.kuilei_canhai.*）在 neidian 接取之后 = A(26)。
//   · 路径行动：按 when 里最晚的链上旗标（A）；目标链第 k 步的提示按第 k−1 步（A），第 1 步记 1（= A(0)，C1 落岛）。
//   · 物品 / 法术描述按 give / learn 它的脚本（神雷法术随神雷 give 在 guiwang，青火雷随 ② 在 tihun）；战斗文案按调用它的脚本；
//     地图名按第一次 teleport 进去的脚本（荒岛 = C1 = A(0)），经门首入的地洞按开门的 guairen（A(3)）；
//     门的拒绝语按那扇门头一回走得到的时刻：n01 横贯带挡在落点北面，过带才到门前 = A(1)。
//   · 表里查不到的 key 直接判红：新加文案不登节点，等于绕开这一层。
//
// 这个文件看不见什么：文案 key 的场景段与「真正调用它的脚本」是否一致（信 key 名）；reveal 提示拼出的名牌（16.1 第 41 条
// 由 Ch12LedgerTests 断「13 条都不设 reveal」）；原创性（最长公共片段归复验脚本，不归测试）。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>
#include "Ch12Data.h"

namespace {
using namespace fanren::test::ch12;

constexpr int S(int n) { return 2 * n; }
constexpr int A(int n) { return 2 * n + 1; }
constexpr int kUnmapped = -1;

// 3.4 目标链 33 步的场景名（= 节点脚本名），下标 + 1 就是 n 号。
const std::vector<const char*>& sceneNames() {
    static const std::vector<const char*> kScenes{
        "dengdao", "pozhen", "guairen", "qixialian", "duoshe", "xiaocha", "lidao", "rudian", "jiumeng", "qunxiang",
        "chuansong", "tupo", "guiwang", "yuanyao", "xiaojie", "lingshen", "fanlian", "baishi", "xiagu", "rongyan",
        "tihun", "lianjing", "shidian", "quba", "huanjing", "neidian", "cengjie", "juanzhou", "ruyi", "butian",
        "xuangu", "canhun", "lingquan"};
    return kScenes;
}

int sceneNode(const std::string& scene) {
    const auto& all = sceneNames();
    for (std::size_t i = 0; i < all.size(); ++i)
        if (scene == all[i]) return S(static_cast<int>(i) + 1);
    return kUnmapped;
}

struct Prefix { const char* prefix; int node; };

// 场景前缀以外的那些 key（12.2「断言的写法」逐类）。
const std::vector<Prefix>& extraPrefixes() {
    static const std::vector<Prefix> kTable = {
        {"ch12.zoupian.", A(12)}, {"ch12.yihai.", A(12)}, {"ch12.quest.maigu.", A(12)}, {"ch12.caiyao.", A(15)},
        {"ch12.yutai.", A(23)}, {"ch12.quest.kuilei_canhai.", A(26)},
        {"ch12.battle.guiwu_guiwang.", S(13)}, {"ch12.battle.heishamo_tiehuoyi.", S(21)},
        {"ch12.battle.neidian_duoding.", S(29)}, {"ch12.battle.hanlitai_xuangu.", S(31)},
        {"ch12.map.tushan.", A(0)}, {"ch12.map.dixue.", A(3)}, {"ch12.map.dadian.", S(7)}, {"ch12.map.guiwu.", S(11)},
        {"ch12.map.xiaojie.", S(15)}, {"ch12.map.binghuo.", S(19)}, {"ch12.map.baoguang.", S(23)},
        {"ch12.map.taqian.", S(24)}, {"ch12.map.hanlitai.", S(27)}, {"ch12.map.lingquan.", S(32)},
        {"ch12.block.dixue", A(1)},
        {"magic.desc.magic_shijin_chongqun", S(3)}, {"magic.desc.magic_bixie_shenlei", S(13)},
        {"magic.desc.magic_qinghuo_lei", S(21)},
        // 物品描述：give 它的脚本（第 9 节来源表）。
        {"item.desc.story_hualan", S(3)}, {"item.desc.story_zhiyang_huazhou", S(3)},
        {"item.desc.talisman_bixie_shenlei", S(13)}, {"item.desc.story_bihuo_baoyi", A(12)},
        {"item.desc.story_jiuqu_lingshen", S(16)}, {"item.desc.story_xuanyin_huan", S(18)},
        {"item.desc.story_baixi_pei", S(19)}, {"item.desc.story_hanbing_zhu", S(19)},
        {"item.desc.story_tihun_shou", S(21)}, {"item.desc.story_minghun_zhu", S(21)},
        {"item.desc.material_lianjing", S(22)}, {"item.desc.story_poluo_zhu", S(23)},
        {"item.desc.story_qingming_zhen", S(23)}, {"item.desc.story_huanglin_jia", S(23)},
        {"item.desc.story_xuese_pifeng", S(24)}, {"item.desc.story_wuse_tonghuan", S(24)},
        {"item.desc.story_kuilei_canhai", S(27)}, {"item.desc.story_neidian_juanzhou", S(28)},
        {"item.desc.story_jinsi_can", S(29)}, {"item.desc.story_yu_ruyi", S(29)},
        {"item.desc.story_jiuqu_danfang", S(31)}, {"item.desc.story_xutian_ding", S(31)},
        {"item.desc.story_qianlan_zhu", S(31)}, {"item.desc.story_xuangu_leigu", S(31)},
        {"item.desc.story_jinlei_xiaojian", S(31)}, {"item.desc.story_wannian_lingru", S(33)},
        {"item.desc.story_yanghun_mugen", S(33)},
        // 路径行动：when 里最晚的链上旗标（16.3）。
        {"ch12.path.jinqing_yizhi_dating.", A(1)}, {"ch12.path.shidie_hongyue_dating.", A(1)},
        {"ch12.path.huyue_sanxiu_dating.", A(1)}, {"ch12.path.wenqiang_kuixing_dating.", A(6)},
        {"ch12.path.xuangu_jinzhi_dating.", A(9)}, {"ch12.path.ziling_canyuan_dating.", A(8)},
        {"ch12.path.zhifa_guiju_dating.", A(10)}, {"ch12.path.ziling_guiwang_dating.", A(12)},
        {"ch12.path.geli_guiwu_dating.", A(12)}, {"ch12.path.xuangu_lingshen_dating.", A(15)},
        {"ch12.path.qingyi_binghuo_dating.", A(18)}, {"ch12.path.wuchou_canyuan_dating.", A(25)},
        {"ch12.path.qingyi_qianlan_dating.", A(27)},
    };
    return kTable;
}

constexpr const char* kObjective = "objective.ch12.n";

int nodeOfKey(const std::string& key) {
    if (key.rfind(kObjective, 0) == 0) {
        // objective.ch12.nXX_<scene>：第 k 步按第 k−1 步做完之后，第 1 步记 1。
        const std::string rest = key.substr(std::string(kObjective).size());
        const auto bar = rest.find('_');
        if (bar != 2 || !std::isdigit(static_cast<unsigned char>(rest[0])) || !std::isdigit(static_cast<unsigned char>(rest[1])))
            return kUnmapped;
        const int n = std::stoi(rest.substr(0, 2));
        if (n < 1 || n > 33 || sceneNode(rest.substr(3)) != S(n)) return kUnmapped;
        return n == 1 ? 1 : A(n - 1);
    }
    int best = kUnmapped;
    std::size_t length = 0;
    for (const Prefix& entry : extraPrefixes()) {
        const std::string prefix(entry.prefix);
        if (key.rfind(prefix, 0) == 0 && prefix.size() > length) { best = entry.node; length = prefix.size(); }
    }
    if (best != kUnmapped) return best;
    // ch12.<场景>.<用途>
    if (key.rfind("ch12.", 0) == 0) {
        const auto dot = key.find('.', 5);
        if (dot != std::string::npos) return sceneNode(key.substr(5, dot - 5));
    }
    return kUnmapped;
}
// 12.2 表「X 之前不含」：一行一个场景。正向那一半按行：每一行至少有一个词真被说到（且不早于该场景），
// 防「整行词被删光、只查没有的那一半恒真」。「三首」是 guiwang 行补的（K8、K9 不许说，12.2 勾魂鬼音一行）。
struct Row { const char* scene; std::vector<const char*> words; };

const std::vector<Row>& rows() {
    static const std::vector<Row> kRows = {
        {"pozhen", {"北斗两仪阵", "翅恶"}},
        {"guairen", {"洪荒古宝", "封灵柱"}},
        {"qixialian", {"七霞莲", "千年石乳", "妖冠蛇", "脂阳鸟"}},
        {"duoshe", {"玄魂炼妖"}},
        {"xiaocha", {"玄骨", "毒龙会", "文思月", "文樯"}},
        {"jiumeng", {"九曲灵参"}},
        {"qunxiang", {"南鹤岛", "白壁山", "温夫人", "寿元果", "万天明", "天悟子", "天罗真功", "托天诀", "金丝蚕",
                      "离火岛", "火蟒", "青易居士", "补天丹", "虚天鼎"}},
        {"tupo", {"鬼冤之地", "鬼王", "啼魂兽", "葛笠"}},
        {"guiwang", {"勾魂鬼音", "鬼夜叉", "炜吾", "附灵术", "三首"}},
        {"yuanyao", {"元瑶"}},
        {"xiaojie", {"冰火道"}},
        {"lingshen", {"呲唬", "四象玄武阵", "麝兰兽"}},
        {"baishi", {"玄阴环", "阴阳环"}},
        {"xiagu", {"托天魔功", "玄晶道", "熔岩路", "白犀佩", "寒冰珠"}},
        {"tihun", {"黑沙漠", "铁火蚁", "天晶蚁", "鸣魂珠", "青火雷"}},
        {"lianjing", {"炼晶", "玄黄镜"}},
        {"shidian", {"宝光阁", "婆罗珠", "青冥针", "皇鳞甲", "极妙幻境"}},
        {"quba", {"五色铜环"}},
        {"cengjie", {"狼首傀儡", "蛇卫", "寒骊台"}},
        {"juanzhou", {"天罡罩"}},
        {"ruyi", {"天青花", "寒蛟", "离龟", "青棘鸟", "狂暴之术", "化石兽", "三目豹", "玉如意", "狼形器灵"}},
        {"butian", {"天罗真雷", "化身符", "贯日剑"}},
        {"xuangu", {"修罗圣火", "五行环", "乾蓝冰珠"}},
        {"lingquan", {"万年灵乳", "养魂木"}},
    };
    return kRows;
}

// 定点：只许出现在这几个前缀下，条数在 [least, most] 之间。
struct Only { const char* word; std::vector<const char*> where; int most; int least; };

const std::vector<Only>& onlies() {
    static const std::vector<const char*> kZ1{"ch12.yihai.", "ch12.quest.maigu.", "item.desc.story_bihuo_baoyi", "ch12.rongyan."};
    static const std::vector<Only> kRules = {
        {"极炫", {"ch12.duoshe."}, 1, 1},
        {"萧诧", {"ch12.xiaocha."}, 1, 1},
        {"玉真人", kZ1, 99, 1},
        {"辟火宝衣", kZ1, 99, 1},
        {"血色披风", {"ch12.lingquan."}, 99, 0},
    };
    return kRules;
}

// 全章 0 处（12.2 灵犀佩、自焚 / 杀死玄骨两行与末两行）。
const std::vector<const char*>& zeroWords() {
    static const std::vector<const char*> kWords = {
        "灵犀佩", "自焚", "杀死玄骨", "银月", "紫罗极火", "逆星盟", "万三谷", "凌玉灵", "玄阴经", "元磁神光", "银光鼠",
        "血冰林", "卜绪", "掌天瓶", "三阳老魔", "天都妖尸", "藏魂匣", "妍姐姐", "三仙宗", "天星城出了大事"};
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
    if (!said) out.push_back(std::string("no word of the ") + row.scene + " row is said at all");
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
    if (hits > rule.most || hits < rule.least) out.push_back(std::string(rule.word) + " appears " + std::to_string(hits) + " times");
    return out;
}

std::vector<std::string> allProblems(const Entries& entries) {
    std::vector<std::string> out;
    const auto append = [&out](const std::vector<std::string>& more) { out.insert(out.end(), more.begin(), more.end()); };
    for (const Entry& e : entries)
        if (nodeOfKey(e.key) == kUnmapped) out.push_back("unmapped " + e.key);
    for (const Row& row : rows()) append(rowProblems(entries, row));
    for (const Only& rule : onlies()) append(onlyProblems(entries, rule));
    int rongyan = 0;
    for (const Entry& e : entries) {
        for (const char* word : zeroWords())
            if (has(e, word)) out.push_back(std::string(word) + " in " + e.key);
        if (e.key.rfind("ch12.rongyan.", 0) == 0 && (has(e, "玉真人") || has(e, "辟火宝衣"))) ++rongyan;
    }
    if (rongyan > 1) out.push_back("rongyan.* names the robe in more than one line");
    return out;
}

// 第 15 节 21 条：分母先验 > 250 条；十一份文案文件（正文七份 ＋ battle / items / maps / objectives / path / quests 拆法）。
bool denominatorHolds(const Entries& entries, int files) { return entries.size() > 250u && files >= 11; }

Entries chapterEntries(int& files, std::string& error) {
    Entries out;
    files = 0;
    std::vector<fs::path> paths;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text"))
        if (file.path().extension() == ".json" && file.path().filename().string().rfind("ch12", 0) == 0) paths.push_back(file.path());
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
class Ch12Lexicon : public ::testing::Test {
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

// 节点表与落地的目标链同序：第 k 步的 done_flag 换成场景名（12.2 三处例外）恰是表里第 k 个。
std::vector<std::string> chainOrderProblems(const Json& steps) {
    std::vector<std::string> out;
    const std::map<std::string, std::string> renamed{
        {"ch12.tuizhu", "guairen"}, {"ch12.canyuan", "xiaocha"}, {"ch12.done", "lingquan"}};
    if (steps.size() != sceneNames().size()) out.push_back("chain has " + std::to_string(steps.size()) + " steps");
    for (std::size_t i = 0; i < steps.size() && i < sceneNames().size(); ++i) {
        const std::string flag = steps[i].value("done_flag", "");
        const auto special = renamed.find(flag);
        const std::string scene = special != renamed.end() ? special->second : flag.rfind("ch12.", 0) == 0 ? flag.substr(5) : flag;
        if (scene != sceneNames()[i]) out.push_back("step " + std::to_string(i + 1) + " is " + flag);
    }
    return out;
}

// ---- 先验：节点表本身（字面量抄施工图 3.4 / 12.2，不从 key 反推）----
TEST(Ch12LexiconTables, TheNodeTableIsSaneAndPinsTheDesignedNodes) {
    ASSERT_EQ(sceneNames().size(), 33u) << "施工图 3.4：目标链 33 步";
    const Json steps = json(root() / "data" / "objectives" / "ch12.json").at("steps");
    EXPECT_TRUE(chainOrderProblems(steps).empty()) << joined(chainOrderProblems(steps));
    Json swapped = steps;
    std::swap(swapped[20], swapped[21]);
    EXPECT_FALSE(chainOrderProblems(swapped).empty()) << "负向：tihun 与 lianjing 对调必须报";
    EXPECT_EQ(nodeOfKey("ch12.dengdao.shore"), 2);
    EXPECT_EQ(nodeOfKey("ch12.xiaocha.name"), 12);
    EXPECT_EQ(nodeOfKey("ch12.zoupian.fog"), 25) << "走偏编在 tupo（24）与 guiwang（26）之间";
    EXPECT_EQ(nodeOfKey("ch12.yihai.take"), 25);
    EXPECT_EQ(nodeOfKey("ch12.caiyao.pick"), 31) << "采药编在 xiaojie（30）与 lingshen（32）之间";
    EXPECT_EQ(nodeOfKey("ch12.battle.heishamo_tiehuoyi.intro"), 42) << "② 开场白跟 tihun";
    EXPECT_EQ(nodeOfKey("ch12.battle.hanlitai_xuangu.charge"), 62);
    EXPECT_EQ(nodeOfKey("ch12.map.guiwu.name"), 22) << "鬼雾首次由 chuansong 送入";
    EXPECT_EQ(nodeOfKey("ch12.map.dixue.name"), 7) << "经门首入，按开门的 guairen";
    EXPECT_EQ(nodeOfKey("ch12.path.geli_guiwu_dating.text"), 25) << "K9 在 tupo 之后、guiwang 之前";
    EXPECT_EQ(nodeOfKey("ch12.path.xuangu_jinzhi_dating.text"), 19) << "K5 when 最晚是 jiumeng";
    EXPECT_EQ(nodeOfKey("item.desc.story_zhiyang_huazhou"), 6);
    EXPECT_EQ(nodeOfKey("item.desc.story_xuese_pifeng"), 48);
    EXPECT_EQ(nodeOfKey("objective.ch12.n01_dengdao"), 1);
    EXPECT_EQ(nodeOfKey("objective.ch12.n13_guiwang"), 25) << "第 13 步的提示在 n12 做完之后上屏";
    EXPECT_EQ(nodeOfKey("objective.ch12.n33_lingquan"), 65);
    EXPECT_EQ(nodeOfKey("objective.ch12.n13_tupo"), kUnmapped) << "号与场景对不上";
    EXPECT_EQ(nodeOfKey("ch12.nosuch.line"), kUnmapped);
    EXPECT_EQ(nodeOfKey("ch11.dengdao.shore"), kUnmapped);
    for (const Row& row : rows()) EXPECT_NE(sceneNode(row.scene), kUnmapped) << row.scene;
}

// ---- 先验：每一类检查都抓得住（在真实文案上注入一处违规，其余不动）----
TEST(Ch12LexiconTables, EachKindOfCheckCatchesAnInjectedViolation) {
    int files = 0;
    std::string error;
    const Entries real = chapterEntries(files, error);
    ASSERT_TRUE(error.empty()) << error;
    ASSERT_TRUE(allProblems(real).empty()) << joined(allProblems(real));
    const auto caught = [](const Entries& broken) { return !allProblems(broken).empty(); };
    // 章内先后：早说（各幕的典型）。
    EXPECT_TRUE(caught(with(real, "ch12.dengdao.extra", "封灵柱")));
    EXPECT_TRUE(caught(with(real, "ch12.duoshe.extra", "玄骨")));
    EXPECT_TRUE(caught(with(real, "ch12.path.geli_guiwu_dating.text", "雾里那头三首的，最怕雷。")));
    EXPECT_TRUE(caught(with(real, "ch12.path.ziling_guiwang_dating.text", "鬼夜叉")));
    EXPECT_TRUE(caught(with(real, "ch12.guiwang.extra", "元瑶")));
    EXPECT_TRUE(caught(with(real, "ch12.map.guiwu.name", "鬼冤之地")));
    EXPECT_TRUE(caught(with(real, "ch12.battle.neidian_duoding.intro", "修罗圣火")));
    EXPECT_TRUE(caught(with(real, "objective.ch12.n21_tihun", "去黑沙漠。"))) << "第 21 步的提示按 n20 编号";
    EXPECT_TRUE(caught(with(real, "item.desc.story_xuese_pifeng", "血色披风")));
    // 整行删光（正向那一半）。
    Entries bare = real;
    for (const char* word : {"修罗圣火", "五行环", "乾蓝冰珠"}) bare = without(bare, word);
    EXPECT_TRUE(caught(bare));
    // 查不到节点。
    EXPECT_TRUE(caught(with(real, "ch12.nosuch.line", "好。")));
    // 定点：出了场景、超了条数、少了条数。
    EXPECT_TRUE(caught(with(real, "ch12.guiwang.extra", "辟火宝衣")));
    EXPECT_TRUE(caught(with(real, "ch12.duoshe.extra", "极炫")));
    EXPECT_TRUE(caught(without(real, "萧诧")));
    EXPECT_TRUE(caught(with(real, "ch12.rongyan.extra", "玉真人")));
    // 0 处。
    for (const char* word : {"灵犀佩", "自焚", "杀死玄骨", "紫罗极火", "掌天瓶", "三阳老魔", "妍姐姐", "天星城出了大事"})
        EXPECT_TRUE(caught(with(real, "ch12.lingquan.extra", word))) << word;
    // 分母先验：恰 250 条不够；少一份文件不够。
    Entries short250(real.begin(), real.begin() + 250);
    EXPECT_FALSE(denominatorHolds(short250, files));
    EXPECT_FALSE(denominatorHolds(real, 10));
    EXPECT_TRUE(denominatorHolds(real, files));
}

// ---- 正题：逐条扫 data/text/ch12*.json ----
TEST_F(Ch12Lexicon, EveryLineIsPinnedToANodeOfTheMainLine) {
    std::vector<std::string> loose;
    for (const Entry& e : entries_)
        if (nodeOfKey(e.key) == kUnmapped) loose.push_back(e.key);
    EXPECT_TRUE(loose.empty()) << joined(loose);
}

TEST_F(Ch12Lexicon, NoWordIsSaidBeforeItsSceneAndEveryRowIsSaidSomewhere) {
    std::vector<std::string> problems;
    for (const Row& row : rows()) {
        const auto more = rowProblems(entries_, row);
        problems.insert(problems.end(), more.begin(), more.end());
    }
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch12Lexicon, SceneBoundWordsStayInTheirScenes) {
    std::vector<std::string> problems;
    for (const Only& rule : onlies()) {
        const auto more = onlyProblems(entries_, rule);
        problems.insert(problems.end(), more.begin(), more.end());
    }
    EXPECT_TRUE(problems.empty()) << joined(problems);
}

TEST_F(Ch12Lexicon, TheWholeSpecialTableIsClean) {
    const auto problems = allProblems(entries_);
    EXPECT_TRUE(problems.empty()) << joined(problems);
}
// ---- 12.2 玄骨、萧诧、元瑶三行：名牌（role 的 name）----
std::vector<std::string> plateProblems(const std::map<std::string, std::string>& names) {
    std::vector<std::string> out;
    for (const char* id : {"dixue_guiying", "xuangu_shaonian"}) {
        const auto found = names.find(id);
        if (found == names.end() || found->second.find("玄") != std::string::npos) out.push_back(std::string(id) + " plate says 玄");
    }
    const auto heipao = names.find("yuan_yao_heipao");
    if (heipao == names.end() || heipao->second.find("元") != std::string::npos) out.push_back("yuan_yao_heipao plate says 元");
    for (const auto& [id, name] : names)
        if (name.find("萧诧") != std::string::npos) out.push_back(id + " plate says 萧诧");
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

TEST(Ch12LexiconPlates, TheHiddenNamesStayOffThePlates) {
    const auto names = rolePlates();
    ASSERT_GT(names.size(), 100u) << "先验：读得到 data/roles";
    const auto problems = plateProblems(names);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    auto bad = names;
    bad["xuangu_shaonian"] = "玄骨";
    EXPECT_FALSE(plateProblems(bad).empty());
    bad = names;
    bad["yuan_yao_heipao"] = "元瑶";
    EXPECT_FALSE(plateProblems(bad).empty());
    bad = names;
    bad["xinggong_zhifa_ci"] = "萧诧";
    EXPECT_FALSE(plateProblems(bad).empty());
}

// ---- 12.2 七霞莲一行、16.1 第 42 条：新物品 / 新法术的显示名也按 give / learn 它的节点编号 ----
// 名字不在 data/text 里（在 data/items/**、data/magics/** 的 name），上面那几条扫不到；拿到手那一刻就上屏，
// 所以节点照 extraPrefixes 里 item.desc.<id> / magic.desc.<id> 那一格。血色披风是物品名例外（12.2 末段、B 表 20）。
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
        for (const Only& rule : onlies()) {
            if (n.name.find(rule.word) == std::string::npos) continue;
            const bool exempt = std::string(rule.word) == "血色披风" && n.key == "item.desc.story_xuese_pifeng";
            if (!exempt && !under(n.key, rule.where)) out.push_back(n.key + " name says " + rule.word + " outside its scene");
        }
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

TEST(Ch12LexiconPlates, NewItemAndMagicNamesFollowTheSceneThatHandsThemOver) {
    const auto names = chapterNames();
    ASSERT_EQ(names.size(), 30u) << "先验：第 9 节来源表 27 件物品 ＋ L.2 三门法术";
    const auto problems = nameProblems(names);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    const auto scroll = std::find_if(names.begin(), names.end(), [](const Named& n) { return n.key == "item.desc.story_zhiyang_huazhou"; });
    ASSERT_NE(scroll, names.end());
    EXPECT_EQ(scroll->name, "红光画轴") << "16.1 第 42 条";
    // 负向：n03 给的画轴叫出脂阳鸟、别的物品叫血色披风、n03 学的法术叫出铁火蚁、一件物品查不到名字。
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_zhiyang_huazhou", "脂阳鸟画轴")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_wuse_tonghuan", "血色披风")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "magic.desc.magic_shijin_chongqun", "铁火蚁群")).empty());
    EXPECT_FALSE(nameProblems(renamed(names, "item.desc.story_hanbing_zhu", "")).empty());
}

// ---- 12.2 血色披风一行的正向：quba.* 写「暗红披风」（血色披风留到 lingquan.*）----
bool cloakIsDarkRedInQuba(const Entries& entries) {
    return std::any_of(entries.begin(), entries.end(), [](const Entry& e) {
        return e.key.rfind("ch12.quba.", 0) == 0 && has(e, "暗红披风");
    });
}

TEST_F(Ch12Lexicon, TheTreasureHallCallsTheCloakDarkRed) {
    EXPECT_TRUE(cloakIsDarkRedInQuba(entries_));
    Entries plain = entries_;
    for (Entry& e : plain)
        for (std::size_t at = e.text.find("暗红披风"); at != std::string::npos; at = e.text.find("暗红披风"))
            e.text.replace(at, std::string("暗红披风").size(), "披风");
    EXPECT_FALSE(cloakIsDarkRedInQuba(plain)) << "负向：quba.* 一处「暗红披风」都没有必须报";
}

// ---- 第 21 条前半：表一含 12.1 的 83 词与「紫罗极火」（读 tests/LexiconTests.cpp 原文，不经它的判据）----
struct Band { int from; int to; std::vector<const char*> words; };

const std::vector<Band>& tableOneBands() {
    static const std::vector<Band> kBands = {
        {416, 436, {"胡月", "石蝶", "北斗两仪阵", "翅恶", "洪荒古宝", "封灵柱", "七霞莲", "千年石乳", "妖冠蛇", "脂阳鸟", "极炫",
                    "玄魂炼妖", "萧诧", "毒龙会", "南鹤岛", "白壁山", "温夫人", "寿元果", "万天明", "天悟子", "天罗真功", "托天诀",
                    "金丝蚕", "离火岛", "火蟒", "青易居士"}},
        {437, 457, {"鬼冤之地", "鬼王", "啼魂兽", "葛笠", "玉真人", "辟火宝衣", "勾魂鬼音", "鬼夜叉", "炜吾", "附灵术", "冰火道",
                    "玄晶道", "熔岩路", "呲唬", "四象玄武阵", "麝兰兽", "托天魔功", "玄阴环", "阴阳环"}},
        {458, 478, {"白犀佩", "天青花", "寒冰珠", "黑沙漠", "极妙幻境", "灵犀佩", "铁火蚁", "天晶蚁", "鸣魂珠", "青火雷", "银光鼠",
                    "血冰林", "炼晶", "玄黄镜", "宝光阁", "婆罗珠", "青冥针", "皇鳞甲", "五色铜环", "狼首傀儡", "蛇卫", "寒骊台",
                    "天罡罩", "寒蛟", "离龟", "青棘鸟"}},
        {479, 499, {"狂暴之术", "化石兽", "三目豹", "天罗真雷", "玉如意", "化身符", "贯日剑", "修罗圣火", "五行环", "万年灵乳",
                    "养魂木", "血色披风"}},
        {731, 731, {"紫罗极火"}},
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

TEST(Ch12LexiconTables, TableOneCarriesTheEightyThreeWordsAndTheLateFire) {
    std::size_t total = 0;
    for (const Band& band : tableOneBands()) total += band.words.size();
    ASSERT_EQ(total, 84u) << "12.1：83 词 ＋ 紫罗极火";
    const std::string source = read(root() / "tests" / "LexiconTests.cpp");
    const auto problems = tableOneProblems(source);
    EXPECT_TRUE(problems.empty()) << joined(problems);
    const auto swap = [&source](const std::string& from, const std::string& to) {
        std::string text = source;
        const auto at = text.find(from);
        if (at != std::string::npos) text.replace(at, from.size(), to);
        return text;
    };
    ASSERT_NE(source.find("{\"葛笠\", "), std::string::npos);
    EXPECT_FALSE(tableOneProblems(swap("{\"葛笠\", ", "{\"葛立\", ")).empty()) << "少登一词必须报";
    EXPECT_FALSE(tableOneProblems(swap("{\"紫罗极火\", 731,", "{\"紫罗极火\", 499,")).empty()) << "首见章改早必须报";
}
}  // namespace
