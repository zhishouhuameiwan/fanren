// 测试注释里点名的用例，全仓都要搜得到（第 4 章复验判据 8、N-2）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一条
// ---------------------------------------------------------------------------
// 第 4 章的通关测试里写着：本章发的药材够不够炼本章要的药，由另一条用例「单独看着」，
// 还点了那条用例的名字。全仓搜那个名字，只有那一处注释——那条用例从来不存在，
// 而读注释的人会以为这件事有人管。那是 docs/README.md
// 「判据自己会说谎」的注释版：没人会去查一条写在注释里的用例到底在不在。
//
// 这一条就去查：扫 tests/ 下每一个 .cpp / .h 的**注释**，把长得像用例名的词挑出来，
// 每一个都得在仓库里找得到出处。
//
// ---------------------------------------------------------------------------
// 口径
// ---------------------------------------------------------------------------
// 「长得像用例名」：ASCII 连写、大写字母开头、第二个字母小写、至少四个大写字母起头的
// 段（TheChapterItselfRaises…、NailsEveryOneOfTheThirteenQiRefiningLayers）。
// 本仓的用例名都是这种整句驼峰；三段以内的（GameState、BattleScene）是类型名，不在此列。
//
// 「找得到出处」，任一即可：
//   1. 它是某条用例的名字或套件名（TEST / TEST_F / TEST_P 的两个参数、
//      INSTANTIATE_TEST_SUITE_P 的两个参数、参数化用例名字生成器里写的那几个名字）；
//   2. 它在 src/ tests/ tools/ 的**代码**（注释与字符串以外）里当标识符出现过；
//   3. 它是仓库里某个源文件的文件名（Ch03TriggerModeTests 这种指文件的写法）。
// 紧贴着「…」或「...」写的算缩写：后面跟省略号的按前缀找，前面跟省略号的按后缀找，
// 都只在用例名里找（不拿同前缀的标识符顶包）
//（「WithoutTheClockReset…」「…TheScriptRunsStartsNothing」）。
//
// 注释本身不算出处——那正是要抓的东西。
//
// 另外两条（二次复验 R-5 收紧的）：
//   · **以章号起头的名字**（「Ch」＋两位数字＋大写字母，如 Ch04Salary、Ch04SliceTests），
//     不论几个大写字母都查：本仓的套件名与测试文件名是这个样子，而它们多半不到四个大写字母；
//   · **「套件.用例」写法**（后一半长得像用例名）：前一半必须是真的套件名，文件名、类名不算。
// 收紧之前全仓注释里以章号起头的名字有 79 个、「套件.用例」1 处，逐个核过都找得到出处，没有误报。
//
// ---------------------------------------------------------------------------
// 这条检查看不见什么（如实申报，别以为它管的比实际多）
// ---------------------------------------------------------------------------
//   · 大写字母不到四个、又不以章号起头的名字：一个编出来的三段驼峰（套件名、辅助函数名那种长度）
//     写进注释，这里放行。要查它就得把 GameState、BattleScene 这类类型名一起拉进来，
//     而它们在注释里被提到的次数成百上千，出处判据（标识符、文件名）又放得很宽——
//     换来的主要是噪声，不是多抓到的错。
//   · 只点套件名、不带「.用例」的写法（「RealmCombat 那一组」）：套件名不到四个大写字母时同上。
//   · 名字找得到，但注释说它「管着」的事它其实不管：这条只查名字在不在，不读用例内容。
// 能保证的只有一件事：**长名字、章号名字、「套件.用例」这三种写法里，写出来的名字是真的**。
//
// ---------------------------------------------------------------------------
// 曾用名的写法（约定）
// ---------------------------------------------------------------------------
// 用例改了名，注释里要交代「从前叫什么」时，一律写成：
//
//     曾用名：<旧名字>
//
// 「曾用名」三个字加**全角冒号**，后面（可以空格）紧跟旧名字。只有紧跟着的那一个名字被放行；
// 同一句里再点别的名字，照样要找得到出处。别的说法（「从前这里叫 …」「原名 …」「曾用名:」
// 半角冒号）一律当作点名，找不到就红。
//
// 为什么要一个死板的前缀而不是一张例外表：例外表按「文件 ＋ 名字」放行，放进去的东西
// 没人再看；前缀写在注释原地，读的人一眼知道这个名字**已经不在了**，检查也只放行这一种
// 写法。**本文件不再有例外表，以后也不许加**——注释里的名字要么真有，要么明写是曾用名。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "tests" / "CommentReferenceTests.cpp") &&
            fs::exists(root / "src" / "game" / "Application.h")) {
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

bool isWordChar(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

// 一份 C++ 源码拆成三份：代码（注释与字面量换成等长的空白，换行保留，于是下标与原文对齐）、
// 注释（逐段）、字符串字面量（内容与它在原文里的起点）。
struct Literal {
    std::size_t offset = 0;
    std::string text;
};
struct SplitSource {
    std::string code;
    std::vector<std::string> comments;
    std::vector<Literal> strings;
};

SplitSource splitCpp(const std::string& src) {
    SplitSource out;
    out.code = src;
    const std::size_t n = src.size();
    const auto blank = [&](std::size_t from, std::size_t to) {
        for (std::size_t k = from; k < to && k < n; ++k) {
            if (out.code[k] != '\n') out.code[k] = ' ';
        }
    };
    std::size_t i = 0;
    while (i < n) {
        if (src.compare(i, 2, "//") == 0) {
            std::size_t end = src.find('\n', i);
            if (end == std::string::npos) end = n;
            out.comments.push_back(src.substr(i + 2, end - i - 2));
            blank(i, end);
            i = end;
        } else if (src.compare(i, 2, "/*") == 0) {
            std::size_t end = src.find("*/", i + 2);
            if (end == std::string::npos) end = n;
            out.comments.push_back(src.substr(i + 2, end - i - 2));
            blank(i, std::min(n, end + 2));
            i = std::min(n, end + 2);
        } else if (src.compare(i, 2, "R\"") == 0 && (i == 0 || !isWordChar(src[i - 1]))) {
            // 原始字符串 R"delim( ... )delim"
            const std::size_t open = src.find('(', i + 2);
            if (open == std::string::npos) {
                ++i;
                continue;
            }
            const std::string closer = ")" + src.substr(i + 2, open - i - 2) + "\"";
            std::size_t end = src.find(closer, open + 1);
            if (end == std::string::npos) end = n;
            out.strings.push_back(Literal{i, src.substr(open + 1, end - open - 1)});
            blank(i, std::min(n, end + closer.size()));
            i = std::min(n, end + closer.size());
        } else if (src[i] == '\'' && i > 0 && isWordChar(src[i - 1])) {
            ++i;   // 数字分隔符（100'000），不是字符字面量
        } else if (src[i] == '"' || src[i] == '\'') {
            const char quote = src[i];
            std::size_t j = i + 1;
            while (j < n && src[j] != quote && src[j] != '\n') {
                if (src[j] == '\\') ++j;
                ++j;
            }
            if (quote == '"') out.strings.push_back(Literal{i, src.substr(i + 1, std::min(j, n) - i - 1)});
            blank(i, std::min(n, j + 1));
            i = std::min(n, j + 1);
        } else {
            ++i;
        }
    }
    return out;
}


// 一段文字里所有 ASCII 标识符样的词，连同它前后紧挨着的是不是省略号。
struct Word {
    std::string text;
    bool ellipsisBefore = false;
    bool ellipsisAfter = false;
    bool formerName = false;   // 紧跟在「曾用名：」后面（见文件头「曾用名的写法」）
    std::size_t begin = 0;     // 在那段文字里的起止（不含 end）
    std::size_t end = 0;
};

// 「曾用名：」，UTF-8。冒号是全角的那一个，写法固定，不认半角、不认别的说法。
constexpr const char* kFormerNameMark = "\xE6\x9B\xBE\xE7\x94\xA8\xE5\x90\x8D\xEF\xBC\x9A";

// 这个词前面（跳过空格）是不是正好写着「曾用名：」。
bool followsFormerNameMark(const std::string& text, std::size_t begin) {
    std::size_t end = begin;
    while (end > 0 && text[end - 1] == ' ') --end;
    const std::string mark = kFormerNameMark;
    return end >= mark.size() && text.compare(end - mark.size(), mark.size(), mark) == 0;
}

bool endsWithEllipsis(const std::string& s, std::size_t end) {
    return (end >= 3 && s.compare(end - 3, 3, "\xE2\x80\xA6") == 0) ||
           (end >= 3 && s.compare(end - 3, 3, "...") == 0);
}

bool startsWithEllipsis(const std::string& s, std::size_t begin) {
    return s.compare(begin, 3, "\xE2\x80\xA6") == 0 || s.compare(begin, 3, "...") == 0;
}

std::vector<Word> wordsIn(const std::string& text) {
    std::vector<Word> out;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!isWordChar(text[i])) {
            ++i;
            continue;
        }
        std::size_t j = i;
        while (j < text.size() && isWordChar(text[j])) ++j;
        Word word;
        word.text = text.substr(i, j - i);
        word.ellipsisBefore = endsWithEllipsis(text, i);
        word.ellipsisAfter = startsWithEllipsis(text, j);
        word.formerName = followsFormerNameMark(text, i);
        word.begin = i;
        word.end = j;
        out.push_back(std::move(word));
        i = j;
    }
    return out;
}

// 长得像用例名：见文件头「口径」第一条。
bool looksLikeATestName(const std::string& word) {
    if (word.size() < 2 || word[0] < 'A' || word[0] > 'Z') return false;
    if (word[1] < 'a' || word[1] > 'z') return false;
    int capitals = 0;
    for (const char c : word) {
        if (c == '_') return false;
        if (c >= 'A' && c <= 'Z') ++capitals;
    }
    return capitals >= 4;
}

// 以章号起头的名字（Ch04Walkthrough、Ch04Salary、Ch04SliceTests……）：本仓的套件名、测试
// 文件名都是这个样子，大写字母常常不到四个，于是「长得像用例名」那一条管不到它们
//（二次复验 R-5）。「Ch」＋ 两位数字 ＋ 一个大写字母起头的那一段，不论几个大写字母都要查。
bool looksLikeAChapterName(const std::string& word) {
    return word.size() >= 5 && word.compare(0, 2, "Ch") == 0 && word[2] >= '0' && word[2] <= '9' &&
           word[3] >= '0' && word[3] <= '9' && word[4] >= 'A' && word[4] <= 'Z' &&
           word.find('_') == std::string::npos;
}

// 仓库里能当出处的名字。
struct Known {
    std::set<std::string> tests;         // 用例名与套件名
    std::set<std::string> identifiers;   // 代码里出现过的标识符
    std::set<std::string> fileStems;     // 源文件名（不带扩展名）
};

void collectTestNames(const std::string& code, std::set<std::string>& into) {
    for (const char* macro : {"TEST(", "TEST_F(", "TEST_P(", "INSTANTIATE_TEST_SUITE_P("}) {
        std::size_t pos = 0;
        const std::string m = macro;
        while ((pos = code.find(m, pos)) != std::string::npos) {
            // 宏名前面不能是标识符字符（排除 MY_TEST( 之类）。
            if (pos > 0 && isWordChar(code[pos - 1])) {
                pos += m.size();
                continue;
            }
            std::size_t cursor = pos + m.size();
            for (int arg = 0; arg < 2; ++arg) {
                while (cursor < code.size() && !isWordChar(code[cursor]) && code[cursor] != ')') {
                    ++cursor;
                }
                std::size_t end = cursor;
                while (end < code.size() && isWordChar(code[end])) ++end;
                if (end > cursor) into.insert(code.substr(cursor, end - cursor));
                cursor = end;
            }
            pos += m.size();
        }
    }
}

// 参数化用例的名字由生成器以字符串写出（"LetThemInAndSurround"），它们也是用例名。
// **只收 INSTANTIATE_TEST_SUITE_P(...) 那一句括号里的字符串**：别处字符串里的词不算出处——
// 否则一条断言消息里提到的名字，也能替一句悬空的注释作保。
void collectGeneratedTestNames(const SplitSource& split, std::set<std::string>& into) {
    const std::string macro = "INSTANTIATE_TEST_SUITE_P(";
    std::size_t pos = 0;
    while ((pos = split.code.find(macro, pos)) != std::string::npos) {
        const std::size_t open = pos + macro.size() - 1;
        int depth = 0;
        std::size_t close = open;
        for (; close < split.code.size(); ++close) {
            if (split.code[close] == '(') ++depth;
            if (split.code[close] == ')' && --depth == 0) break;
        }
        for (const Literal& literal : split.strings) {
            if (literal.offset < open || literal.offset > close) continue;
            for (const Word& w : wordsIn(literal.text)) {
                if (looksLikeATestName(w.text)) into.insert(w.text);
            }
        }
        pos = close;
    }
}

Known collectKnown(const fs::path& root) {
    Known known;
    for (const char* dir : {"src", "tests", "tools"}) {
        const fs::path base = root / dir;
        if (!fs::is_directory(base)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(base)) {
            if (!entry.is_regular_file()) continue;
            const fs::path& path = entry.path();
            known.fileStems.insert(path.stem().string());
            const std::string ext = path.extension().string();
            if (ext != ".cpp" && ext != ".h" && ext != ".py") continue;
            const std::string src = readFile(path);
            if (ext == ".py") {
                for (const Word& w : wordsIn(src)) known.identifiers.insert(w.text);
                continue;
            }
            const SplitSource split = splitCpp(src);
            for (const Word& w : wordsIn(split.code)) known.identifiers.insert(w.text);
            if (std::string(dir) == "tests") {
                collectTestNames(split.code, known.tests);
                collectGeneratedTestNames(split, known.tests);
            }
        }
    }
    return known;
}

bool resolves(const Word& word, const Known& known) {
    // 省略号缩写只许在**用例名**里找前缀 / 后缀：缩写的本来就是用例名，而拿任何一个
    // 恰好同前缀的变量名去顶包，正是这一条要防的「读注释的人以为有人管」。
    const auto anyOf = [&](const auto& matches) {
        for (const std::string& name : known.tests) {
            if (matches(name)) return true;
        }
        return false;
    };
    if (known.tests.count(word.text) || known.identifiers.count(word.text) ||
        known.fileStems.count(word.text)) {
        return true;
    }
    if (word.ellipsisAfter &&
        anyOf([&](const std::string& name) { return name.rfind(word.text, 0) == 0; })) {
        return true;
    }
    if (word.ellipsisBefore && anyOf([&](const std::string& name) {
            return name.size() >= word.text.size() &&
                   name.compare(name.size() - word.text.size(), word.text.size(), word.text) == 0;
        })) {
        return true;
    }
    return false;
}

// 一段注释里找不到出处的名字，按出现顺序（同一段里重复的只报一次）。三条口径：
//   1. 长得像用例名（≥4 个大写字母的整句驼峰）或以章号起头的名字，要找得到出处；
//      紧跟「曾用名：」的那一个放行；
//   2. 写成「套件.用例」时（后一半长得像用例名），**前一半必须是真的套件名**——
//      文件名、类名不算（Ch03TriggerModeTests.X 这种把文件名当套件名的写法，从前真出过）。
std::vector<std::string> danglingIn(const std::string& comment, const Known& known) {
    std::vector<std::string> out;
    const std::vector<Word> words = wordsIn(comment);
    const auto report = [&](const std::string& what) {
        if (std::find(out.begin(), out.end(), what) == out.end()) out.push_back(what);
    };
    for (std::size_t i = 0; i < words.size(); ++i) {
        const Word& word = words[i];
        const bool dottedSuite = i + 1 < words.size() && word.end < comment.size() &&
                                 comment[word.end] == '.' && words[i + 1].begin == word.end + 1 &&
                                 looksLikeATestName(words[i + 1].text);
        if (dottedSuite && !word.formerName && !known.tests.count(word.text)) {
            report(word.text + "." + words[i + 1].text);
            continue;
        }
        if (!looksLikeATestName(word.text) && !looksLikeAChapterName(word.text)) continue;
        if (word.formerName || resolves(word, known)) continue;
        report(word.text);
    }
    return out;
}

struct Dangling {
    std::string file;   // 相对仓库根，正斜杠
    std::string word;
};

std::vector<Dangling> danglingReferences(const fs::path& root, const Known& known, int& scanned) {
    std::vector<Dangling> out;
    scanned = 0;
    std::vector<fs::path> files;
    // 递归：与 collectKnown 收「出处」的范围对称，挪进子目录的测试文件也照样扫。
    for (const auto& entry : fs::recursive_directory_iterator(root / "tests")) {
        if (!entry.is_regular_file()) continue;
        const std::string ext = entry.path().extension().string();
        if (ext == ".cpp" || ext == ".h") files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& path : files) {
        ++scanned;
        const SplitSource split = splitCpp(readFile(path));
        std::set<std::string> reported;
        for (const std::string& comment : split.comments) {
            for (const std::string& name : danglingIn(comment, known)) {
                if (!reported.insert(name).second) continue;
                out.push_back(Dangling{fs::relative(path, root).generic_string(), name});
            }
        }
    }
    return out;
}

class CommentReferences : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        known_ = collectKnown(root_);
    }
    fs::path root_;
    Known known_;
};

TEST_F(CommentReferences, EveryTestNamedInATestCommentExistsSomewhereInTheRepo) {
    // 先验：出处那一头真的收到了东西（用例名少说几百条），否则一切都「找不到」或一切都放行。
    ASSERT_GT(known_.tests.size(), 500u) << "只收到 " << known_.tests.size() << " 个用例名";
    ASSERT_TRUE(known_.tests.count("TheChapterItselfRaisesHimFromWhereChapterThreeLeftHim"))
        << "先验：一条确实存在的第 4 章用例该在出处里";

    int scanned = 0;
    const std::vector<Dangling> dangling = danglingReferences(root_, known_, scanned);
    ASSERT_GT(scanned, 30) << "只扫到 " << scanned << " 个测试源文件";

    for (const Dangling& d : dangling) {
        ADD_FAILURE() << d.file << " 的注释里点了名：" << d.word
                      << "——全仓找不到这个用例（也不是哪个标识符或文件）。"
                      << "注释说有人看着的事，得真有人看着：把那条用例写出来，或者删掉那句注释。";
    }
}

// 判据自检：「曾用名：」这条约定放行的只是紧跟在它后面的那一个名字，换一种说法就不放行。
// 从前这里有一张例外表（按文件 ＋ 名字放行），协调者裁决不许再靠例外表放行：
// 改名说明一律照约定写，约定写错了就当悬空报。
TEST_F(CommentReferences, OnlyTheFormerNameMarkLetsARetiredNameStandInAComment) {
    const std::string retired = "SomeRetiredTestNameThatNoLongerExists";
    ASSERT_FALSE(known_.tests.count(retired)) << "先验：这个名字确实不存在";
    const auto caughtIn = [&](const std::string& source) {
        std::vector<std::string> caught;
        for (const std::string& comment : splitCpp(source).comments) {
            for (const std::string& name : danglingIn(comment, known_)) caught.push_back(name);
        }
        return caught;
    };
    const std::string mark = kFormerNameMark;
    // 照约定写：放行。冒号后面空一格也算。
    EXPECT_TRUE(caughtIn("// " + mark + retired + "。那时断言的是……").empty());
    EXPECT_TRUE(caughtIn("// " + mark + " " + retired).empty());
    // 换一种说法（从前的写法）：不放行。
    EXPECT_EQ(caughtIn("// 从前这里叫 " + retired + "，断言的是……"),
              std::vector<std::string>{retired});
    // 半角冒号、拆开写：都不算约定。
    EXPECT_EQ(caughtIn("// 曾用名:" + retired), std::vector<std::string>{retired});
    // 约定只管紧跟的那一个名字，同一句里后面再点的名照样要找得到出处。
    EXPECT_EQ(caughtIn("// " + mark + retired + "，现在由 NobodyEverWroteThisTestCase 看着"),
              std::vector<std::string>{"NobodyEverWroteThisTestCase"});
}

// 判据自检：扫描器有牙。一段就地写的注释喂进同一套口径。
TEST_F(CommentReferences, TheScannerCatchesATestThatOnlyExistsInAComment) {
    const std::string source =
        "// 这笔账由 TheYearsSalaryCoversWhatTheChapterAsksHimToBrew 单独看着。\n"
        "// 真有的：TheChapterItselfRaisesHimFromWhereChapterThreeLeftHim。\n"
        "// 缩写：TheChapterItselfRaises\xE2\x80\xA6 与 \xE2\x80\xA6"
        "FromWhereChapterThreeLeftHim。\n"
        "// 类型名不算：GameState、BattleScene。\n"
        "int x = 0;  // 行尾注释里的 NobodyEverWroteThisTestCase 也要抓\n"
        "const char* s = \"// 字符串里的 NotACommentAtAllReally 不算注释\";\n";
    const SplitSource split = splitCpp(source);
    std::vector<std::string> caught;
    for (const std::string& comment : split.comments) {
        for (const std::string& name : danglingIn(comment, known_)) caught.push_back(name);
    }
    const std::vector<std::string> expected = {"TheYearsSalaryCoversWhatTheChapterAsksHimToBrew",
                                               "NobodyEverWroteThisTestCase"};
    EXPECT_EQ(caught, expected);
}

// 判据自检：二次复验 R-5 收紧的那两条也有牙，而且不误报真名字。
//   · 以章号起头的短名字（大写字母不到四个）照样查：编一个不存在的必抓（见下面那段源码），
//     真有的套件名 Ch04Salary、测试文件名 Ch04SliceTests 放行；
//   · 「套件.用例」写法里前一半必须是真套件：把文件名当套件名（从前真出过）必抓。
TEST_F(CommentReferences, ShortChapterNamesAndWrongSuiteNamesAreCaughtToo) {
    ASSERT_TRUE(known_.tests.count("Ch04Salary")) << "先验：Ch04Salary 是真的套件名";
    ASSERT_TRUE(known_.tests.count("Ch03TriggerMode")) << "先验：Ch03TriggerMode 是真的套件名";
    ASSERT_FALSE(known_.tests.count("Ch03TriggerModeTests")) << "先验：带 Tests 的是文件名，不是套件名";
    const std::string source =
        "// 这笔账由 Ch04RestLedger 看着。\n"
        "// 真有的：Ch04Salary 那一组、tests/Ch04SliceTests.cpp 这个文件。\n"
        "// 真的套件：Ch03TriggerMode.TheExecutionOnlyStartsWhenThePlayerPressesConfirm。\n"
        "// 套件名写错：Ch03TriggerModeTests.TheExecutionOnlyStartsWhenThePlayerPressesConfirm。\n";
    std::vector<std::string> caught;
    for (const std::string& comment : splitCpp(source).comments) {
        for (const std::string& name : danglingIn(comment, known_)) caught.push_back(name);
    }
    const std::vector<std::string> expected = {
        "Ch04RestLedger",
        "Ch03TriggerModeTests.TheExecutionOnlyStartsWhenThePlayerPressesConfirm"};
    EXPECT_EQ(caught, expected);
}

}  // namespace
