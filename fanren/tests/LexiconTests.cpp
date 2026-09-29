// 名字是有时间的：一个词在原著里第一次出现的章号，决定了它在游戏里最早能出现在哪一章。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-5）
// ---------------------------------------------------------------------------
// `docs/handoff.md` 第 5 节那张表已经因为这件事改过四次设计：口诀的名字、灵石、
// 惊蛟会、升仙令、银月。提前给出去就再也收不回来——而**这条约束到现在为止全靠人守**：
//   · `tools/validate.py` 里没有任何禁词检查；
//   · `tests/PanelTests.cpp` 那张禁词表只扫**面板用词**，不扫 `data/text/`。
// 第 4 章复核时扫出来是干净的（291 条 ch04 文案里「灵石」0 命中），
// 但「今天干净」与「有人看着」是两件事。
//
// ---------------------------------------------------------------------------
// 判据写成规则，不写成清单
// ---------------------------------------------------------------------------
// 本文件**不列**「第 4 章禁哪几个词」。那是内容形状，章一多就得逐章维护，
// 而且新加一章时没人会记得回来补。这里只写两张事实表加一条规则：
//
//   表一：这个词在原著里第一次出现在第几章（我自己用 tools/novel/novel.py 查的）
//   表二：游戏第 N 章覆盖原著哪一段（docs/大纲.md v3.1）
//   规则：**首见章号晚于第 N 章覆盖区间的末章，这个词在第 N 章就不许出现。**
//
// 这条规则在内容翻倍之后仍然成立，也不必为第 5-14 章各写一行。
// 新加一个词只要填它的首见章号，它自己就知道该在哪几章闭嘴。
//
// ---------------------------------------------------------------------------
// 这条用例看不见什么
// ---------------------------------------------------------------------------
//   能 —— 玩家会读到的章节文案（`data/text/chNN*.json`）里有没有超前的词。
//   不能 —— **章内的先后**。惊蛟会首见 ch104，落在第 5 章（ch100-125）区间内，
//          于是本文件放行整个第 5 章；但它在第 5 章开头那几节仍然不该出现。
//          那一层只有人读得出来，写在这里免得后人以为它管得比实际多。
//   不能 —— 脚本注释与 data 的 note。`scripts/ch04/zhanlipin.lua` 的首部整段都在
//          讲升仙令，那是**给后人看的**，不是玩家看得见的字。故意不扫。
//   不能 —— 不按章分文件的那几份（`items.json` / `battles.json` / `shops.json`）。
//          它们跨章共用，没有章号可依。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// 表一：原著首见章号
// ---------------------------------------------------------------------------
// 每一条都是我自己用 `tools/novel/novel.py find` 查的（2026-09-22），
// 证据一并写在后面那一列——**不要照着大纲的模糊区间填这张表**，
// `docs/handoff.md` 第 3 节点名说那是最值钱的一步，四章里改过四次设计。
struct Term {
    const char* word;
    int firstSeenChapter;
    const char* evidence;
};

const std::vector<Term>& terms() {
    static const std::vector<Term> kTerms = {
        {"长春功", 30, "ch30 枭雄末路。第 1、2 章里它只是「那段口诀」"},
        {"筑基", 58, "ch58 修仙者"},
        {"惊蛟会", 104, "ch104 情报。第 3 章只能用 ch30 的诨号「鬼手」"},
        {"灵石", 131, "ch131 章题即「灵石与灵符」。第 1-5 章的钱是「碎银，按块计」"},
        {"元婴", 127, "ch127 灵根之说"},
        {"结丹", 127, "ch127 灵根之说"},
        {"升仙令", 141, "ch141 升仙令。第 4 章那块牌子不得有名字，另见 Ch04AcceptanceTests"},
        {"银月", 615, "ch615 银月狼族。**超出全 14 章**，所以哪一章都不许用，"
                      "第 14 章只能称「狼形器灵」"},
        // ---- 第 5 章施工图 12.1 补的 11 个（docs/ch05-design.md，2026-09-23 逐个 novel.py find 核过）----
        // 补之前扫过 data/text/ch0[1-4]*.json 共 16 个文件：11 个词命中 0 条，所以第 1-4 章不会因此转红
        //（扫描记录见 docs/interfaces-p3-ch05.md 第 5 节）。
        {"灵气", 126, "ch126 暖阳宝玉「可以容纳自己的灵气」。docs/tech-debt.md G-13 表里的 ch58 是错的"},
        {"炼气", 127, "ch127 灵根之说。第 1-5 章说「长春功第几层」"},
        {"筑基期", 127, "ch127。「筑基」ch58 那一条另算，不因这条收紧"},
        {"真元", 164, "ch164"},
        {"黄枫谷", 127, "ch127"},
        {"太南小会", 128, "ch128 章题。「太南谷」「太南山」首见 ch125，第 5 章末可以说"},
        {"万小山", 126, "ch126"},
        {"灵符", 131, "ch131 章题「灵石与灵符」"},
        {"坊市", 146, "ch146"},
        {"储物袋", 148, "ch148"},
        {"定颜丹", 155, "ch155"},
        // ---- 第 6 章施工图 12.1 补的 10 个（docs/ch06-design.md，2026-09-29 逐个 novel.py find 核过）----
        // 补之前扫过 data/text/ch0[1-5]*.json：10 个词命中 0 条，所以第 1-5 章不会因此转红。
        {"岳麓殿", 153, "ch153 藏室"},
        {"地火屋", 215, "ch215"},
        {"血色试炼", 161, "ch161（章题「血禁试炼」是 ch160）"},
        {"血禁", 160, "ch160 章题"},
        {"南宫婉", 209, "ch209 点名（ch182 只叫白衣少女）"},
        {"李化元", 212, "ch212。大纲第 6 章人物表列了他，本章不出场"},
        {"青元剑诀", 213, "ch213"},
        {"陈巧倩", 273, "ch273 点名（ch157 起只叫陈师妹）"},
        {"大衍诀", 230, "ch230"},
        {"妖丹", 344, "ch344。本作物品「一级妖丹」等早就存在（第 2 章制符配方的引），"
                     "名字不在扫描范围；第 6 章定神符方不用它、坊市不卖它"},
    };
    return kTerms;
}

// ---------------------------------------------------------------------------
// 表二：游戏第 N 章覆盖原著到第几章（docs/大纲.md v3.1 第 1 节那张表）
// ---------------------------------------------------------------------------
// 只记区间的**末章**：规则只问「这个词的首见有没有落进来」。
const std::map<int, int>& chapterCoversUpTo() {
    static const std::map<int, int> kRanges = {
        {1, 9},    {2, 27},   {3, 64},   {4, 99},   {5, 125},
        {6, 151},  {7, 216},  {8, 345},  {9, 363},  {10, 388},
        {11, 415}, {12, 499}, {13, 542}, {14, 596},
    };
    return kRanges;
}

// 规则本身。抽成函数是为了能单独验它——判据与被判之物要分得开。
[[nodiscard]] bool bannedInChapter(const Term& term, int gameChapter) {
    const auto it = chapterCoversUpTo().find(gameChapter);
    if (it == chapterCoversUpTo().end()) return false;   // 不认识的章不拦
    return term.firstSeenChapter > it->second;
}

// 第一个在这一章里说得太早的词；没有则返回空串。
[[nodiscard]] std::string firstTooEarlyWord(const std::string& text, int gameChapter,
                                            std::string& why) {
    for (const Term& term : terms()) {
        if (!bannedInChapter(term, gameChapter)) continue;
        if (text.find(term.word) != std::string::npos) {
            why = term.evidence;
            return term.word;
        }
    }
    why.clear();
    return std::string{};
}

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

// 文件名开头的 chNN → N；不是按章分的文件返回 0。
[[nodiscard]] int chapterOfFile(const std::string& filename) {
    if (filename.size() < 4) return 0;
    if (filename.compare(0, 2, "ch") != 0) return 0;
    if (!std::isdigit(static_cast<unsigned char>(filename[2]))) return 0;
    if (!std::isdigit(static_cast<unsigned char>(filename[3]))) return 0;
    return (filename[2] - '0') * 10 + (filename[3] - '0');
}

}  // namespace

// ---------------------------------------------------------------------------
// 先验一 · 两张表与那条规则本身
// ---------------------------------------------------------------------------
// 一张被清空的词表照样能让扫描器跑完并报通过——本项目在「禁词表被清空」
// 这一类空转法上栽过，`docs/README.md` 那张表里有它。
TEST(Lexicon, TheTablesAndTheRuleAreSaneBeforeAnythingIsScanned) {
    ASSERT_FALSE(terms().empty()) << "词表是空的，扫描器无词可扫";
    for (const Term& term : terms()) {
        EXPECT_NE(term.word[0], '\0') << "词表里有一条空串，它会命中任何文本";
        EXPECT_GT(term.firstSeenChapter, 0) << term.word << " 没填首见章号";
        EXPECT_NE(term.evidence[0], '\0') << term.word << " 没写出处";
    }
    ASSERT_EQ(chapterCoversUpTo().size(), 14u) << "大纲是 14 章";

    // 规则自己的几个定点。判据抄自 docs/handoff.md 第 5 节那张表的原话。
    const Term lingshi{"灵石", 131, ""};
    EXPECT_TRUE(bannedInChapter(lingshi, 4)) << "灵石首见 ch131，第 4 章（ch65-99）不许有";
    EXPECT_TRUE(bannedInChapter(lingshi, 5))
        << "第 5 章是 ch100-125，**仍然不到 ch131**——"
           "ch03/ch04 两份设计文档里「第 5 章进修仙界才该听说」那句话是错的";
    EXPECT_FALSE(bannedInChapter(lingshi, 6)) << "第 6 章是 ch126-151，ch131 落在里头";

    const Term shengxian{"升仙令", 141, ""};
    EXPECT_TRUE(bannedInChapter(shengxian, 4));
    EXPECT_TRUE(bannedInChapter(shengxian, 5));
    EXPECT_FALSE(bannedInChapter(shengxian, 6));

    const Term yinyue{"银月", 615, ""};
    for (int n = 1; n <= 14; ++n) {
        EXPECT_TRUE(bannedInChapter(yinyue, n))
            << "银月首见 ch615，超出全 14 章（末章到 ch596），第 " << n << " 章也不许用";
    }
}

// ---------------------------------------------------------------------------
// 先验二 · 扫描器真的抓得住
// ---------------------------------------------------------------------------
TEST(Lexicon, TheScannerCatchesAWordSaidTooEarly) {
    std::string why;
    EXPECT_EQ(firstTooEarlyWord("囊中灵石不足三十", 4, why), "灵石");
    EXPECT_FALSE(why.empty()) << "抓住了却说不出为什么，转红时没人知道该查哪一章";
    EXPECT_EQ(firstTooEarlyWord("他把长春功又默了一遍", 2, why), "长春功");
    EXPECT_EQ(firstTooEarlyWord("此物名唤升仙令", 4, why), "升仙令");

    // 配对的正向：同样的话放在它该出现的那一章必须**不**被判违规，
    // 否则上面几条只是因为扫描器见什么抓什么。
    EXPECT_TRUE(firstTooEarlyWord("囊中灵石不足三十", 6, why).empty());
    EXPECT_TRUE(firstTooEarlyWord("他把长春功又默了一遍", 3, why).empty());
    EXPECT_TRUE(firstTooEarlyWord("碎银十二块，按块计", 4, why).empty());
}

// ---------------------------------------------------------------------------
// 正题：逐章扫 data/text/chNN*.json
// ---------------------------------------------------------------------------
// 扫的是**文件原文**而不是解析后的值：tests 拿不到 nlohmann（不在 fanren_io 的
// include 路径上，`tests/IoTests.cpp` 首部写明了），而扫原文反而更严——
// 连 key 里写早了都算。
TEST(Lexicon, NoChapterSaysAWordItHasNotEarnedYet) {
    const fs::path textDir = fs::path(assetRoot()) / "data" / "text";
    ASSERT_TRUE(fs::is_directory(textDir)) << "找不到 " << textDir.string();

    std::map<int, int> filesPerChapter;
    std::size_t scannedBytes = 0;
    for (const auto& entry : fs::directory_iterator(textDir)) {
        if (entry.path().extension() != ".json") continue;
        const std::string name = entry.path().filename().string();
        const int chapter = chapterOfFile(name);
        if (chapter == 0) continue;   // items.json / battles.json / shops.json

        const std::string body = readFile(entry.path());
        ASSERT_FALSE(body.empty()) << name << " 是空的";
        filesPerChapter[chapter] += 1;
        scannedBytes += body.size();

        std::string why;
        const std::string hit = firstTooEarlyWord(body, chapter, why);
        EXPECT_TRUE(hit.empty())
            << name << " 里出现了「" << hit << "」，而第 " << chapter
            << " 章还轮不到它。" << why
            << "。提前给出去就再也收不回来——docs/handoff.md 第 5 节，已经改过四次设计。";
    }

    // 先验分母：一个文件也没扫到照样一条不报。第 1-4 章是已完工且冻结的，
    // 它们必须都在。第 5 章以后还没施工，不在这里要求。
    for (int built = 1; built <= 4; ++built) {
        EXPECT_GT(filesPerChapter[built], 0)
            << "第 " << built << " 章一个文案文件也没扫到，这条用例正在空转";
    }
    EXPECT_GT(scannedBytes, 40000u)
        << "只扫了 " << scannedBytes << " 字节，远少于已完工四章的文案量";
}
