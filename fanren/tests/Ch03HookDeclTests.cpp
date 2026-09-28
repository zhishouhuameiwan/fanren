// 脚本首部那一行 `-- @hook` 说的是不是设计文档要的那件事。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件，以及它与两个邻居的分工
// ---------------------------------------------------------------------------
// 「触发方式」这件事现在有三个地方说得出口，任意两处走散都必须有人报：
//
//   设计文档 ── tests/Ch03TriggerModeTests.cpp ── maps/ch02_jusuo.tmj
//       │                                              │
//       └────── 本文件 ──── scripts/ch03/*.lua ── tools/validate.py ──┘
//
//   · maps ↔ 设计     Ch03TriggerModeTests.cpp（读 tmj，判据写死成设计原文）
//   · 脚本 ↔ maps     tools/validate.py 的 check_hook_declarations（门禁）
//   · 脚本 ↔ 设计     本文件
//
// 三条边都在，任何一处漂移都至少红两条；少了任何一条，就又回到第 3 章那个局面
// ——两种杀的触发方式在地图上装反了，脚本首部写的正好相反，而四条对它满格敏感
// 的测试一条也没发现，因为它们的驱动方式是**照着 tmj 写出来的**。
// 从被测物推导出来的判据永远发现不了被测物与规格不一致。
//
// 本文件刻意不碰引擎、不建 Application、不走位：它只读文本文件。形状与两个邻居
// 都不一样是有意的——同一套驱动写两遍，只是把同一个错误钉两遍。
//
// 与门禁的分工也说清楚：validate.py 保证「声明与地图一致」，本文件保证
// 「声明就是设计要的那个值」。只有前者的话，两边一起改错仍然全绿；
// 只有后者的话，地图偷偷改了没人管。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

// 与 Ch03TriggerModeTests / Ch03SliceTests 同一套找根目录的写法。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch03" / "duoshe.lua") &&
            fs::exists(root / "maps" / "ch02_jusuo.tmj")) {
            return candidate;
        }
    }
    return ".";
}

struct HookDecl {
    std::string mapId;
    std::string objectName;
    std::string mode;
    bool once = false;
};

// 一行声明解析成结构；不是合法声明就返回 false。
//
// 刻意**不**用正则：本项目栽过四次「反斜杠转义被吃掉、检查器从此永远报通过」，
// 而一个手写的分词器在源码里是什么样就是什么样，没有第二层可以被吃掉的转义。
// 判据本身（三或四个字段、第四个只能是 once）与 tools/validate.py 里那一份
// 是同一套，两边各写一遍是有意的：门禁用 python、测试用 C++，
// 一处写坏不会同时把两处放倒。
bool parseHookLine(const std::string& line, HookDecl* out) {
    const std::string::size_type marker = line.find("@hook");
    if (marker == std::string::npos) return false;
    // 必须是注释行，且 @hook 之前只有 `--` 和空白。
    const std::string before = line.substr(0, marker);
    const std::string::size_type dashes = before.find("--");
    if (dashes == std::string::npos) return false;
    for (std::string::size_type i = 0; i < before.size(); ++i) {
        if (i >= dashes && i < dashes + 2) continue;
        if (before[i] != ' ' && before[i] != '\t') return false;
    }
    // @hook 后面必须是分隔符，`@hookpoint` 不是声明。
    const std::string rest = line.substr(marker + 5);
    if (!rest.empty() && rest[0] != ' ' && rest[0] != '\t' && rest[0] != '\r') return false;

    std::istringstream stream(rest);
    std::vector<std::string> fields;
    std::string field;
    while (stream >> field) fields.push_back(field);
    if (fields.size() < 3 || fields.size() > 4) return false;
    if (fields[2] != "enter" && fields[2] != "interact" && fields[2] != "npc") return false;
    bool once = false;
    if (fields.size() == 4) {
        if (fields[3] != "once") return false;
        if (fields[2] == "npc") return false;   // npc 对象没有 once 属性
        once = true;
    }
    if (out != nullptr) {
        out->mapId = fields[0];
        out->objectName = fields[1];
        out->mode = fields[2];
        out->once = once;
    }
    return true;
}

std::vector<std::string> readLines(const fs::path& path) {
    std::vector<std::string> lines;
    std::ifstream input(path, std::ios::binary);
    std::string line;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

bool isCommentOrBlank(const std::string& line) {
    std::string::size_type i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= line.size()) return true;
    return line.compare(i, 2, "--") == 0;
}

// 只收**首部**（第一行代码之前）的声明。埋在正文中间的不算数，理由与门禁一致：
// 这一行的全部作用是「打开文件第一眼就看得见」。
std::vector<HookDecl> headerHooks(const fs::path& path, int* strayBelowCode) {
    std::vector<HookDecl> out;
    bool inHeader = true;
    if (strayBelowCode != nullptr) *strayBelowCode = 0;
    for (const std::string& line : readLines(path)) {
        if (inHeader && !isCommentOrBlank(line)) inHeader = false;
        HookDecl decl;
        if (!parseHookLine(line, &decl)) continue;
        if (inHeader) {
            out.push_back(decl);
        } else if (strayBelowCode != nullptr) {
            ++(*strayBelowCode);
        }
    }
    return out;
}

const HookDecl* findHook(const std::vector<HookDecl>& hooks, const std::string& objectName) {
    for (const HookDecl& hook : hooks) {
        if (hook.objectName == objectName) return &hook;
    }
    return nullptr;
}

}  // namespace

// ---------------------------------------------------------------------------
// 先验：这个文件自己的解析器有没有牙
// ---------------------------------------------------------------------------
// 少了这一条，下面每一句「它声明的是 enter」都可能建立在一个什么也解析不出来
// 的解析器上——那是本项目那张空转法表里「正则被吃掉转义」的同一件事，
// 只是这里换成了手写分词。所以先拿**确实是坏的**输入喂它一遍。
TEST(Ch03HookDeclParser, RejectsEveryShapeOfBrokenDeclaration) {
    HookDecl decl;
    EXPECT_FALSE(parseHookLine("-- @hook ch02_jusuo trigger_duoshe", &decl))
        << "字段少了一个也算合法，那这个解析器什么都拦不住";
    EXPECT_FALSE(parseHookLine("-- @hook a b c d e", &decl)) << "字段多了也该拒";
    EXPECT_FALSE(parseHookLine("-- @hook ch02_jusuo trigger_duoshe touch", &decl))
        << "touch 不是引擎认的取值，从前有脚本这么写过";
    EXPECT_FALSE(parseHookLine("-- @hook ch02_jusuo trigger_duoshe enter twice", &decl))
        << "第四个字段只能是字面量 once";
    EXPECT_FALSE(parseHookLine("-- @hook ch03_mishi npc_mo_daifu npc once", &decl))
        << "npc 对象没有 once 属性";
    EXPECT_FALSE(parseHookLine("-- 关卡侧把这个位置叫作 @hookpoint xx yy enter", &decl))
        << "@hookpoint 不是声明；认成声明说明词边界没了";
    EXPECT_FALSE(parseHookLine("local s = \"@hook ch02_jusuo trigger_duoshe enter\"", &decl))
        << "正文里的字符串不是声明";
}

TEST(Ch03HookDeclParser, AcceptsTheTwoShapesThatAreActuallyUsed) {
    HookDecl trigger;
    ASSERT_TRUE(parseHookLine("-- @hook ch02_jusuo trigger_duoshe enter once", &trigger));
    EXPECT_EQ(trigger.mapId, "ch02_jusuo");
    EXPECT_EQ(trigger.objectName, "trigger_duoshe");
    EXPECT_EQ(trigger.mode, "enter");
    EXPECT_TRUE(trigger.once);

    HookDecl npc;
    ASSERT_TRUE(parseHookLine("-- @hook ch03_mishi npc_mo_daifu npc", &npc));
    EXPECT_EQ(npc.mode, "npc");
    EXPECT_FALSE(npc.once) << "没写 once 就是 once=false，不是「没说」";
}

// ---------------------------------------------------------------------------
// 正题：两种杀，脚本自己声明的触发方式必须是设计要的那个
// ---------------------------------------------------------------------------

TEST(Ch03HookDecl, TheDispossessionScriptDeclaresItselfStepActivated) {
    const std::vector<HookDecl> hooks =
        headerHooks(fs::path(assetRoot()) / "scripts" / "ch03" / "duoshe.lua", nullptr);
    ASSERT_FALSE(hooks.empty()) << "duoshe.lua 首部一条 @hook 也没有";
    const HookDecl* hook = findHook(hooks, "trigger_duoshe");
    ASSERT_NE(hook, nullptr) << "duoshe.lua 没有声明 trigger_duoshe 这一处挂点";
    EXPECT_EQ(hook->mapId, "ch02_jusuo");
    EXPECT_EQ(hook->mode, "enter")
        << "节点 10 夺舍必须是踏入型：这一节从头到尾没有一次 choice()，"
           "失重感一半来自那个，另一半就来自这里——他连「要不要开始」都没得选。";
    EXPECT_TRUE(hook->once) << "夺舍演完就完了，不是可反复撞的";
}

TEST(Ch03HookDecl, TheExecutionScriptDeclaresItselfConfirmActivated) {
    const std::vector<HookDecl> hooks =
        headerHooks(fs::path(assetRoot()) / "scripts" / "ch03" / "chujue.lua", nullptr);
    ASSERT_FALSE(hooks.empty()) << "chujue.lua 首部一条 @hook 也没有";
    const HookDecl* hook = findHook(hooks, "trigger_chujue");
    ASSERT_NE(hook, nullptr) << "chujue.lua 没有声明 trigger_chujue 这一处挂点";
    EXPECT_EQ(hook->mapId, "ch02_jusuo");
    EXPECT_EQ(hook->mode, "interact")
        << "节点 11 处决必须是交互型：ch03.chujue.after 那句「这一回是他自己"
           "走过去、自己叫的名字、自己按下的拇指」指的就是这一下确认键。";
    EXPECT_TRUE(hook->once);
}

TEST(Ch03HookDecl, TheTwoScriptsDoNotDeclareTheSameMode) {
    const fs::path root(assetRoot());
    // 两个 vector 都得**先落成具名变量**：findHook 返回的是指进容器里的指针，
    // 直接喂一个临时 vector，指针在同一条语句结束时就悬空了，而悬空之后读到的
    // 值恰好可能让下面那条 EXPECT_NE 绿着——第一版就是这么写的，当场红了一条。
    const std::vector<HookDecl> duosheHooks =
        headerHooks(root / "scripts" / "ch03" / "duoshe.lua", nullptr);
    const std::vector<HookDecl> chujueHooks =
        headerHooks(root / "scripts" / "ch03" / "chujue.lua", nullptr);
    const HookDecl* duoshe = findHook(duosheHooks, "trigger_duoshe");
    const HookDecl* chujue = findHook(chujueHooks, "trigger_chujue");
    ASSERT_NE(duoshe, nullptr);
    ASSERT_NE(chujue, nullptr);
    // 上面两条各自写死了取值，这一条只问「它们不一样」——本章要的就是这个对比。
    EXPECT_NE(duoshe->mode, chujue->mode)
        << "两种杀声明成了同一种触发方式，本章的道德分量就没了（大纲注解）";
}

// ---------------------------------------------------------------------------
// 覆盖面：三章的每一个事件脚本都得自己说得出挂在哪儿
// ---------------------------------------------------------------------------
// 这一条盯的是「这套声明被悄悄放弃」这种长相：门禁那一侧只比对存在的声明，
// 一个脚本把整行删掉当然也会被 validate.py 抓住，但那要等到有人跑门禁；
// 而这一条在 ctest 里就红，且报得出是哪一个文件。
// scripts/common/ 是 API 实现本身，不是事件脚本，不在此列。
TEST(Ch03HookDecl, EveryChapterScriptDeclaresAtLeastOneHookInItsHeader) {
    const fs::path root(assetRoot());
    int checked = 0;
    std::vector<std::string> missing;
    std::vector<std::string> stray;
    for (const char* chapter : {"ch01", "ch02", "ch03"}) {
        const fs::path dir = root / "scripts" / chapter;
        ASSERT_TRUE(fs::is_directory(dir)) << "找不到 " << dir.string();
        for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() != ".lua") continue;
            ++checked;
            int below = 0;
            const std::vector<HookDecl> hooks = headerHooks(entry.path(), &below);
            if (hooks.empty()) missing.push_back(entry.path().filename().string());
            if (below > 0) stray.push_back(entry.path().filename().string());
        }
    }
    // 先验分母：一个脚本也没扫到的话，下面两句是恒真的。
    ASSERT_GE(checked, 55) << "只扫到 " << checked
                           << " 个脚本，三章加起来不该这么少——先查目录找对了没有";
    EXPECT_TRUE(missing.empty())
        << "这些脚本首部没有 @hook 声明，改了地图不会有人发现："
        << ::testing::PrintToString(missing);
    EXPECT_TRUE(stray.empty())
        << "这些脚本把 @hook 写在了正文中间，等于没写："
        << ::testing::PrintToString(stray);
}
