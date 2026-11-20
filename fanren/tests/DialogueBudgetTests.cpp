// 第 7 章对白量的两道上限（地基包路 D，docs/ch07-foundation.md 4.1、4.6 第 1 条）：
//   · 主线对白字数在 9,000–10,000 之间：目标链上 50 个脚本（ch07-design.md 3.1 表的 44 个主线挂点、Z2 两处、
//     伙伴改版拆出来的困地、火灵石、破顶、问名四段）引用到的全部文案 key，去重后只数汉字（U+4E00–U+9FFF）；
//   · 两次可操作之间连续 talk 不超过 8 句：静态走脚本的控制流，choice / battle / shop 清零，脚本头尾清零，
//     if 的每条分支都走、取最坏的那条，循环按零次、一次、两次展开，return 结束这条路。bubble 不压场景、不算一句。
// 规则与 tools/dialogue_stats.py 逐条相同（那边打逐脚本的表，这边把上限钉住）。用例先自证：故意写坏的样例真的被抓住。
#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "TextKeys.h"

namespace fs = std::filesystem;

namespace {

std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "scripts" / "ch07" / "chaibao.lua")) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

const std::vector<std::string>& mainScripts() {
    static const std::vector<std::string> kMain{
        "chaibao",     "yaolou",     "yuelu_ru",    "cangshi",    "dihuo_wen", "shimen",        "murong",
        "dufang",      "chuangong",  "yuanjiao",    "chuling",    "fangshi_ru", "wanbaolou",    "fangshi_chu",
        "yeyu",        "fanhui",     "baoming",     "ma_songyao", "jihe",      "xiang",         "liedui",
        "pojin",       "wulongtan",  "liangshi",    "yixiantian", "shulin",    "tongmen",       "yueyang",
        "kuitan",      "charen",     "youdi",       "sichu",      "xiaoshidian", "qingshidian", "guanzhan",
        "jiaoshi",     "xiashan",    "chukou",      "xieshili",   "sannian",   "chouhan",       "shijiu",
        "banian",      "zhuji",      "yujian_dongpo", "yujian_hantan",
        "fengkou",     "huolingshi", "poding",      "wenming",
    };
    return kMain;
}

// ---- 切词 ----
struct Token {
    enum Kind { Name, Str, Op, Num, End } kind = End;
    std::string text;
};

std::vector<Token> tokenize(const std::string& src) {
    std::vector<Token> out;
    std::size_t i = 0;
    const auto longBracket = [&](std::size_t at, std::size_t& end) -> bool {
        // [[ … ]] / [==[ … ]==]
        if (src[at] != '[') return false;
        std::size_t j = at + 1;
        while (j < src.size() && src[j] == '=') ++j;
        if (j >= src.size() || src[j] != '[') return false;
        const std::string close = "]" + std::string(j - at - 1, '=') + "]";
        const std::size_t c = src.find(close, j + 1);
        end = c == std::string::npos ? src.size() : c + close.size();
        return true;
    };
    while (i < src.size()) {
        const char c = src[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            ++i;
        } else if (src.compare(i, 2, "--") == 0) {
            std::size_t end = 0;
            if (i + 2 < src.size() && longBracket(i + 2, end)) {
                i = end;
            } else {
                const std::size_t nl = src.find('\n', i);
                i = nl == std::string::npos ? src.size() : nl;
            }
        } else if (c == '"' || c == '\'') {
            std::size_t j = i + 1;
            std::string text;
            while (j < src.size() && src[j] != c) {
                if (src[j] == '\\' && j + 1 < src.size()) ++j;
                text += src[j++];
            }
            out.push_back({Token::Str, text});
            i = j + 1;
        } else if (std::size_t end = 0; c == '[' && longBracket(i, end)) {
            out.push_back({Token::Str, src.substr(i, end - i)});
            i = end;
        } else if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            std::size_t j = i;
            while (j < src.size() && (std::isalnum(static_cast<unsigned char>(src[j])) || src[j] == '_')) ++j;
            out.push_back({Token::Name, src.substr(i, j - i)});
            i = j;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            std::size_t j = i;
            while (j < src.size() && (std::isalnum(static_cast<unsigned char>(src[j])) || src[j] == '.')) ++j;
            out.push_back({Token::Num, src.substr(i, j - i)});
            i = j;
        } else {
            for (const char* op : {"...", "..", "==", "~=", "<=", ">=", "::"}) {
                if (src.compare(i, std::char_traits<char>::length(op), op) == 0) {
                    out.push_back({Token::Op, op});
                    i += std::char_traits<char>::length(op);
                    goto next;
                }
            }
            out.push_back({Token::Op, std::string(1, c)});
            ++i;
        next:;
        }
    }
    return out;
}

// ---- 走控制流 ----
using States = std::set<int>;
constexpr int kCap = 64;

class Walker {
public:
    explicit Walker(std::vector<Token> tokens) : t_(std::move(tokens)) {}
    int maxRun() const { return maxRun_; }
    int talks() const { return talks_; }
    void run() { (void)block({0}, {}); }

private:
    const Token& peek(std::size_t k = 0) const {
        static const Token kEnd;
        return i_ + k < t_.size() ? t_[i_ + k] : kEnd;
    }
    bool at(std::initializer_list<const char*> words) const {
        if (peek().kind != Token::Name) return false;
        return std::any_of(words.begin(), words.end(), [&](const char* w) { return peek().text == w; });
    }
    bool eof() const { return i_ >= t_.size(); }

    States balanced(States s) {
        const std::string open = peek().text;
        const std::string close = open == "(" ? ")" : open == "{" ? "}" : "]";
        ++i_;
        while (!eof()) {
            if (peek().kind == Token::Op && peek().text == close) {
                ++i_;
                return s;
            }
            if (peek().kind == Token::Op && (peek().text == "(" || peek().text == "{" || peek().text == "[")) {
                s = balanced(s);
                continue;
            }
            s = step(s);
        }
        return s;
    }
    States call(const std::string& name, States s) {
        if (peek().kind == Token::Op && (peek().text == "(" || peek().text == "{")) {
            s = balanced(s);
        } else if (peek().kind == Token::Str) {
            ++i_;
        }
        if (name == "talk") {
            ++talks_;
            States next;
            for (int v : s) next.insert(std::min(v + 1, kCap));
            if (!next.empty()) maxRun_ = std::max(maxRun_, *next.rbegin());
            return next;
        }
        return s.empty() ? States{} : States{0};   // choice / battle / shop
    }
    States step(States s) {
        const Token tok = peek();
        ++i_;
        if (tok.kind == Token::Name &&
            (tok.text == "talk" || tok.text == "choice" || tok.text == "battle" || tok.text == "shop")) {
            const bool field = i_ >= 2 && t_[i_ - 2].kind == Token::Op && (t_[i_ - 2].text == "." || t_[i_ - 2].text == ":");
            const bool callNext = (peek().kind == Token::Op && (peek().text == "(" || peek().text == "{")) ||
                                  peek().kind == Token::Str;
            if (!field && callNext) return call(tok.text, s);
        }
        return s;
    }
    States block(States s, std::set<std::string> stop) {
        while (!eof()) {
            if (peek().kind == Token::Name && stop.count(peek().text)) return s;
            if (at({"if"})) {
                s = ifStmt(s);
            } else if (at({"while", "for"})) {
                ++i_;
                while (!eof() && !at({"do"})) s = step(s);
                ++i_;
                const std::size_t start = i_;
                const States once = block(s, {"end"});
                const std::size_t endPos = i_;
                i_ = start;
                const States twice = block(once, {"end"});
                i_ = endPos + 1;
                States merged = s;
                merged.insert(once.begin(), once.end());
                merged.insert(twice.begin(), twice.end());
                s = merged;
            } else if (at({"do"})) {
                ++i_;
                s = block(s, {"end"});
                ++i_;
            } else if (at({"repeat"})) {
                ++i_;
                const States body = block(s, {"until"});
                ++i_;
                s.insert(body.begin(), body.end());
            } else if (at({"function"})) {
                int depth = 0;
                while (!eof()) {
                    const Token tok = peek();
                    ++i_;
                    if (tok.kind == Token::Name && (tok.text == "function" || tok.text == "if" || tok.text == "do")) ++depth;
                    if (tok.kind == Token::Name && tok.text == "end" && --depth == 0) break;
                }
            } else if (at({"return"})) {
                ++i_;
                while (!eof() && !at({"end", "else", "elseif", "until"})) s = step(s);
                s.clear();
            } else {
                s = step(s);
            }
        }
        return s;
    }
    States ifStmt(States s) {
        ++i_;
        States outs;
        States cur = s;
        for (;;) {
            while (!eof() && !at({"then"})) cur = step(cur);
            ++i_;
            const States branch = block(cur, {"elseif", "else", "end"});
            outs.insert(branch.begin(), branch.end());
            if (at({"elseif"})) {
                ++i_;
                continue;
            }
            if (at({"else"})) {
                ++i_;
                const States other = block(cur, {"end"});
                outs.insert(other.begin(), other.end());
                ++i_;
                return outs;
            }
            ++i_;
            outs.insert(cur.begin(), cur.end());
            return outs;
        }
    }

    std::vector<Token> t_;
    std::size_t i_ = 0;
    int maxRun_ = 0;
    int talks_ = 0;
};

int maxRunOf(const std::string& source) {
    Walker walker(tokenize(source));
    walker.run();
    return walker.maxRun();
}

// UTF-8 里 U+4E00–U+9FFF 都是三字节、首字节 0xE4–0xE9。
int hanCount(const std::string& s) {
    int n = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const auto b = static_cast<unsigned char>(s[i]);
        if (b >= 0xE4 && b <= 0xE9 && i + 2 < s.size()) {
            const unsigned cp = ((b & 0x0Fu) << 12) | ((static_cast<unsigned char>(s[i + 1]) & 0x3Fu) << 6) |
                                (static_cast<unsigned char>(s[i + 2]) & 0x3Fu);
            if (cp >= 0x4E00 && cp <= 0x9FFF) ++n;
            i += 2;
        }
    }
    return n;
}

std::map<std::string, std::string> allTexts(const fs::path& root) {
    std::map<std::string, std::string> out;
    for (const auto& entry : fs::directory_iterator(root / "data" / "text")) {
        if (entry.path().extension() != ".json") continue;
        for (auto& [k, v] : fanren::test::textKeyValues(readFile(entry.path()))) out[k] = v;
    }
    return out;
}

}  // namespace

// ---- 量具先自证 ----

TEST(DialogueBudget, TheWalkerCountsRunsAcrossBranchesAndResetsOnOperableCommands) {
    EXPECT_EQ(maxRunOf("talk('', 'a')\ntalk('', 'b')\n"), 2);
    EXPECT_EQ(maxRunOf("talk('', 'a')\nlocal p = choice{'x','y'}\ntalk('', 'b')\n"), 1) << "选择清零";
    EXPECT_EQ(maxRunOf("talk('', 'a')\nif battle('b') then talk('', 'c') end\ntalk('', 'd')\n"), 2)
        << "条件里的 battle 也清零：赢了那条是 c、d 两句；没进 then 的那条也是 battle 之后的 d 一句";
    EXPECT_EQ(maxRunOf("talk('', 'a')\nif x then talk('', 'b') talk('', 'c') else talk('', 'd') end\ntalk('', 'e')\n"), 4)
        << "最坏那条：a b c e";
    EXPECT_EQ(maxRunOf("talk('', 'a')\nif x then talk('', 'b') return end\ntalk('', 'c')\ntalk('', 'd')\n"), 3)
        << "return 结束那一条：a b 两句；另一条 a c d 三句";
    EXPECT_EQ(maxRunOf("-- talk('', 'x')\ntalk('', 'a')\n--[[ talk('', 'y') ]]\n"), 1) << "注释里的不算";
    EXPECT_EQ(maxRunOf("bubble('r', 'k', 3)\ntalk('', 'a')\n"), 1) << "气泡不算一句";
    EXPECT_EQ(maxRunOf("talk('', 'a')\nwhile n < 3 do talk('', 'b') end\n"), 3) << "循环展开到两次";
    EXPECT_EQ(hanCount("天亮以后，他先走了一遍。ab"), 10);
}

// ---- 两道上限 ----

TEST(DialogueBudget, NoChapter7ScriptRunsMoreThanEightTalksBetweenTwoPlayerActions) {
    const fs::path dir = fs::path(repoRoot()) / "scripts" / "ch07";
    int scripts = 0;
    int worst = 0;
    for (const auto& entry : fs::directory_iterator(dir)) {
        if (entry.path().extension() != ".lua") continue;
        ++scripts;
        const int run = maxRunOf(readFile(entry.path()));
        worst = std::max(worst, run);
        EXPECT_LE(run, 8) << entry.path().filename().string() << "：两次可操作之间连着 " << run
                          << " 句 talk（施工图 4.1：不超过 8 句）";
    }
    EXPECT_EQ(scripts, 56) << "先验：第 7 章 56 个脚本都扫到了";
    EXPECT_GE(worst, 3) << "先验：量具真的数到了句子";
}

TEST(DialogueBudget, Chapter7MainLineDialogueIsBetweenNineAndTenThousandCharacters) {
    const fs::path root = repoRoot();
    const std::map<std::string, std::string> texts = allTexts(root);
    ASSERT_GT(texts.size(), 1000u);
    std::set<std::string> keys;
    for (const std::string& name : mainScripts()) {
        const fs::path path = root / "scripts" / "ch07" / (name + ".lua");
        ASSERT_TRUE(fs::exists(path)) << path.string();
        for (const Token& tok : tokenize(readFile(path))) {
            if (tok.kind == Token::Str && texts.count(tok.text)) keys.insert(tok.text);
        }
    }
    ASSERT_GT(keys.size(), 300u) << "先验：主线脚本引用的文案够多";
    int chars = 0;
    for (const std::string& key : keys) chars += hanCount(texts.at(key));
    EXPECT_GE(chars, 9000) << "主线对白 " << chars << " 字（施工图 4.1：9,000–10,000）";
    EXPECT_LE(chars, 10000) << "主线对白 " << chars << " 字（施工图 4.1：9,000–10,000）";
}
