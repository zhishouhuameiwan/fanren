// 中文排版单测。
//
// 全部用假的等宽测量函数，不碰 SDL、不读字体文件：排版规则是纯逻辑，
// 把它和字体渲染绑在一起测，出错时分不清是断行错了还是字体度量变了。
#include <algorithm>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "engine/TextLayout.h"

namespace {

using fanren::engine::LayoutLine;
using fanren::engine::layoutText;
using fanren::engine::splitGraphemes;

// 汉字 16px、ASCII 8px 的等宽模型。真实字体不等宽，但断行规则与具体
// 字形无关，固定宽度才能把期望值写死成可读的数字。
constexpr int kHan = 16;
constexpr int kAscii = 8;

int fakeMeasure(const std::string& text) {
    int width = 0;
    for (const std::string& g : splitGraphemes(text)) {
        width += (g.size() == 1) ? kAscii : kHan;
    }
    return width;
}

std::vector<std::string> texts(const std::vector<LayoutLine>& lines) {
    std::vector<std::string> out;
    out.reserve(lines.size());
    for (const LayoutLine& line : lines) {
        out.push_back(line.text);
    }
    return out;
}

std::string stripSpaces(const std::string& s) {
    std::string out;
    for (const char c : s) {
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            out += c;
        }
    }
    return out;
}

std::string joinAll(const std::vector<LayoutLine>& lines) {
    std::string out;
    for (const LayoutLine& line : lines) {
        out += line.text;
    }
    return out;
}

const std::vector<std::string>& leadingForbidden() {
    static const std::vector<std::string> kTable{"。", "，", "、", "；", "：", "？", "！",
                                                 "）", "》", "」", "』", "】", "%"};
    return kTable;
}

const std::vector<std::string>& trailingForbidden() {
    static const std::vector<std::string> kTable{"（", "《", "「", "『", "【"};
    return kTable;
}

bool startsWith(const std::string& line, const std::string& piece) {
    return line.size() >= piece.size() && line.compare(0, piece.size(), piece) == 0;
}

bool endsWith(const std::string& line, const std::string& piece) {
    return line.size() >= piece.size() &&
           line.compare(line.size() - piece.size(), piece.size(), piece) == 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// splitGraphemes
// ---------------------------------------------------------------------------

TEST(SplitGraphemes, ReturnsNothingForEmptyString) {
    EXPECT_TRUE(splitGraphemes("").empty());
}

TEST(SplitGraphemes, SplitsThreeByteChineseByCharacterNotByte) {
    const std::vector<std::string> gs = splitGraphemes("韩立修仙");
    ASSERT_EQ(gs.size(), 4u);
    EXPECT_EQ(gs[0], "韩");
    EXPECT_EQ(gs[3], "仙");
    for (const std::string& g : gs) {
        EXPECT_EQ(g.size(), 3u);
    }
}

TEST(SplitGraphemes, KeepsFourByteEmojiWhole) {
    const std::vector<std::string> gs = splitGraphemes("甲🙂乙");
    ASSERT_EQ(gs.size(), 3u);
    EXPECT_EQ(gs[0], "甲");
    EXPECT_EQ(gs[1], "🙂");
    EXPECT_EQ(gs[1].size(), 4u);
    EXPECT_EQ(gs[2], "乙");
}

TEST(SplitGraphemes, KeepsZeroWidthJoinerSequenceWhole) {
    // ZWJ 组合 emoji 必须整体出现，否则逐字显示会先蹦出半个家庭。
    const std::string family = "👨‍👩‍👧";
    const std::vector<std::string> gs = splitGraphemes(family);
    ASSERT_EQ(gs.size(), 1u);
    EXPECT_EQ(gs[0], family);
}

TEST(SplitGraphemes, MixesAsciiAndChineseInOrder) {
    const std::vector<std::string> gs = splitGraphemes("Hi韩立!");
    ASSERT_EQ(gs.size(), 5u);
    EXPECT_EQ(gs[0], "H");
    EXPECT_EQ(gs[1], "i");
    EXPECT_EQ(gs[2], "韩");
    EXPECT_EQ(gs[3], "立");
    EXPECT_EQ(gs[4], "!");
}

TEST(SplitGraphemes, SurvivesTruncatedUtf8WithoutHanging) {
    // 半个汉字后面跟一个 ASCII：不能死循环，也不能吞掉字节。
    std::string broken = std::string("韩").substr(0, 2) + "A";
    const std::vector<std::string> gs = splitGraphemes(broken);
    std::string rejoined;
    for (const std::string& g : gs) {
        rejoined += g;
    }
    EXPECT_EQ(rejoined, broken);
    EXPECT_FALSE(gs.empty());
}

TEST(SplitGraphemes, RejectsOverlongEncodings) {
    // "\xC0\xAF" 是 '/' 的 overlong 形式，历史上被用来绕过路径过滤。
    // 它必须被当成两个非法字节，而不是解成一个合法的 '/'。
    const std::string overlongTwoByte("\xC0\xAF", 2);
    const std::vector<std::string> a = splitGraphemes(overlongTwoByte);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[0] + a[1], overlongTwoByte);  // 不丢字节

    const std::string overlongThreeByte("\xE0\x80\xAF", 3);
    const std::vector<std::string> b = splitGraphemes(overlongThreeByte);
    EXPECT_EQ(b.size(), 3u);
    std::string rejoined;
    for (const std::string& g : b) {
        rejoined += g;
    }
    EXPECT_EQ(rejoined, overlongThreeByte);
}

TEST(SplitGraphemes, RejectsLoneSurrogatesAndOutOfRangeCodepoints) {
    // U+D800 的 UTF-8 形式与 F5 开头的超上限序列都不是合法 UTF-8，
    // 放行它们等于给存档名和将来的 mod 文案开一个脏数据入口。
    const std::string loneSurrogate("\xED\xA0\x80", 3);   // U+D800
    const std::string beyondMax("\xF5\x80\x80\x80", 4);   // > U+10FFFF

    for (const std::string& bad : {loneSurrogate, beyondMax}) {
        const std::vector<std::string> gs = splitGraphemes(bad);
        EXPECT_EQ(gs.size(), bad.size());  // 每个字节各自成 grapheme
        std::string rejoined;
        for (const std::string& g : gs) {
            rejoined += g;
            EXPECT_EQ(g.size(), 1u);
        }
        EXPECT_EQ(rejoined, bad);  // 不丢字节
    }
}

TEST(SplitGraphemes, StillAcceptsTheShortestLegalFormOfEachLength) {
    // 拒绝 overlong 不能误伤规范编码的边界值。
    EXPECT_EQ(splitGraphemes(std::string("\x7F", 1)).size(), 1u);                 // U+007F
    EXPECT_EQ(splitGraphemes(std::string("\xC2\x80", 2)).size(), 1u);             // U+0080
    EXPECT_EQ(splitGraphemes(std::string("\xE0\xA0\x80", 3)).size(), 1u);         // U+0800
    EXPECT_EQ(splitGraphemes(std::string("\xF0\x90\x80\x80", 4)).size(), 1u);     // U+10000
    EXPECT_EQ(splitGraphemes(std::string("\xF4\x8F\xBF\xBF", 4)).size(), 1u);     // U+10FFFF
}

// ---------------------------------------------------------------------------
// layoutText：基本断行
// ---------------------------------------------------------------------------

TEST(LayoutText, ReturnsNothingForEmptyInput) {
    EXPECT_TRUE(layoutText("", 100, fakeMeasure).empty());
}

TEST(LayoutText, WrapsPureChineseAtTheWidthLimit) {
    // 64px 正好四个汉字。
    const std::vector<std::string> lines =
        texts(layoutText("一二三四五六七八九十甲乙", 4 * kHan, fakeMeasure));
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "一二三四");
    EXPECT_EQ(lines[1], "五六七八");
    EXPECT_EQ(lines[2], "九十甲乙");
}

TEST(LayoutText, KeepsLatinWordsWhole) {
    const std::vector<std::string> lines =
        texts(layoutText("hello world foo", 10 * kAscii, fakeMeasure));
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "hello");
    EXPECT_EQ(lines[1], "world foo");
}

TEST(LayoutText, DropsWhitespaceAtWrapPoints) {
    const std::vector<std::string> lines = texts(layoutText("hello  world", 5 * kAscii, fakeMeasure));
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "hello");
    EXPECT_EQ(lines[1], "world");
}

TEST(LayoutText, TrimsTrailingWhitespace) {
    const std::vector<std::string> lines = texts(layoutText("hello   ", 100, fakeMeasure));
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "hello");
}

TEST(LayoutText, BreaksAnOverlongRunWithNoSpaces) {
    // 一个不可分单元比整行还宽时只能硬切，否则字会画到对话框外面。
    const std::vector<LayoutLine> lines = layoutText("abcdefghij", 5 * kAscii, fakeMeasure);
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0].text, "abcde");
    EXPECT_EQ(lines[1].text, "fghij");
    for (const LayoutLine& line : lines) {
        EXPECT_LE(line.width, 5 * kAscii);
    }
}

TEST(LayoutText, PutsOneCharacterPerLineWhenWidthIsBelowOneGlyph) {
    const std::vector<std::string> lines = texts(layoutText("韩立修", kHan / 2, fakeMeasure));
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "韩");
    EXPECT_EQ(lines[1], "立");
    EXPECT_EQ(lines[2], "修");
}

TEST(LayoutText, HandlesMixedChineseAndLatin) {
    const std::vector<std::string> lines =
        texts(layoutText("韩立 said hello 世界", 6 * kHan, fakeMeasure));
    ASSERT_FALSE(lines.empty());
    // 西文单词一个都不许被劈开。
    for (const std::string& piece : {"said", "hello"}) {
        const bool intact = std::any_of(lines.begin(), lines.end(), [&](const std::string& line) {
            return line.find(piece) != std::string::npos;
        });
        EXPECT_TRUE(intact) << piece;
    }
}

TEST(LayoutText, RespectsHardNewlines) {
    const std::vector<std::string> lines = texts(layoutText("一二\n\n三四", 10 * kHan, fakeMeasure));
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "一二");
    EXPECT_EQ(lines[1], "");  // 空行保留，段间距靠它撑
    EXPECT_EQ(lines[2], "三四");
}

TEST(LayoutText, ReportsMeasuredWidthForEachLine) {
    const std::vector<LayoutLine> lines = layoutText("一二三四五六七八", 4 * kHan, fakeMeasure);
    ASSERT_EQ(lines.size(), 2u);
    for (const LayoutLine& line : lines) {
        EXPECT_EQ(line.width, fakeMeasure(line.text));
    }
}

TEST(LayoutText, ReturnsWholeParagraphWhenWidthIsNotPositive) {
    // 行宽非正时「断在哪」无从定义，原样返回好过退化成一字一行。
    const std::vector<std::string> lines = texts(layoutText("一二三四", 0, fakeMeasure));
    ASSERT_EQ(lines.size(), 1u);
    EXPECT_EQ(lines[0], "一二三四");
}

// ---------------------------------------------------------------------------
// layoutText：标点禁则
// ---------------------------------------------------------------------------

TEST(LayoutKinsoku, PullsForbiddenLeadingPunctuationBackToPreviousLine) {
    // 贪心断行会把「。」甩到第二行行首，禁则必须把它拉回去。
    const std::vector<std::string> lines =
        texts(layoutText("一二三四。五六七八", 4 * kHan, fakeMeasure));
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(lines[0], "一二三四。");
    EXPECT_FALSE(startsWith(lines[1], "。"));
}

TEST(LayoutKinsoku, TreatsPercentSignAsForbiddenAtLineStart) {
    // %  是禁则表里唯一的 ASCII 成员，容易在实现里漏掉。
    const std::vector<std::string> lines = texts(layoutText("灵石九十%整", 4 * kHan, fakeMeasure));
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(lines[0], "灵石九十%");
    EXPECT_EQ(lines[1], "整");
}

TEST(LayoutKinsoku, NoLineButTheFirstStartsWithForbiddenPunctuation) {
    const std::vector<std::string> lines = texts(layoutText(
        "修仙一途，逆天而行。资质不佳者，纵有奇遇，亦难登顶！凡人之躯，如何撼动天地？",
        6 * kHan, fakeMeasure));
    ASSERT_GE(lines.size(), 3u);
    for (std::size_t i = 1; i < lines.size(); ++i) {
        for (const std::string& punct : leadingForbidden()) {
            EXPECT_FALSE(startsWith(lines[i], punct)) << "第 " << i << " 行以 " << punct << " 开头";
        }
    }
}

TEST(LayoutKinsoku, PushesForbiddenTrailingPunctuationDownToNextLine) {
    const std::vector<std::string> lines =
        texts(layoutText("一二三「四五六七八", 4 * kHan, fakeMeasure));
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(lines[0], "一二三");
    EXPECT_TRUE(startsWith(lines[1], "「"));
}

TEST(LayoutKinsoku, NoLineEndsWithForbiddenOpeningPunctuation) {
    const std::vector<std::string> lines = texts(layoutText(
        "他翻开《长春功》，又取出【碧玉瓶】，低声念道「灵气入体」，随后闭目调息。", 4 * kHan,
        fakeMeasure));
    ASSERT_GE(lines.size(), 3u);
    for (const std::string& line : lines) {
        for (const std::string& punct : trailingForbidden()) {
            EXPECT_FALSE(endsWith(line, punct)) << "行 \"" << line << "\" 以 " << punct << " 结尾";
        }
    }
}

TEST(LayoutKinsoku, SurvivesAStringMadeEntirelyOfPunctuation) {
    // 病态输入：每个字符都是行首禁则字符，禁则本身无解。
    // 要求退化得可控：不死循环、不丢字、也不把整段吸到第一行上去。
    const std::string input = "。。。。。。。。";
    const std::vector<LayoutLine> lines = layoutText(input, 4 * kHan, fakeMeasure);
    ASSERT_GE(lines.size(), 2u);
    EXPECT_EQ(joinAll(lines), input);
    // 每行最多悬挂 2 个标点，所以首行不会超过 4 + 2 个字。
    EXPECT_LE(lines[0].width, 6 * kHan);
}

TEST(LayoutKinsoku, SurvivesAlternatingOpeningAndClosingPunctuation) {
    const std::string input = "「」「」「」「」「」「」";
    const std::vector<LayoutLine> lines = layoutText(input, 3 * kHan, fakeMeasure);
    ASSERT_FALSE(lines.empty());
    EXPECT_EQ(joinAll(lines), input);
}

// ---------------------------------------------------------------------------
// 不变式：排版只重排空白，不许吞字
// ---------------------------------------------------------------------------

TEST(LayoutText, NeverLosesANonSpaceCharacter) {
    const std::vector<std::string> inputs{
        "韩立点了点头，道：「三叔，我愿意去。」",
        "Hello 世界, this is 一段 mixed text with 标点！",
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "。，、；：？！）》」』】%（《「『【",
        "一",
        "🙂🙂🙂修仙🙂",
        // 畸形字节混在正文里：overlong + 孤立代理项 + 超上限，正文仍须原样保留
        std::string("\xC0\xAF\xED\xA0\x80" "韩立" "\xF5\x80\x80\x80", 15),
    };
    for (const std::string& input : inputs) {
        for (const int width : {kHan / 2, kHan, 3 * kHan, 7 * kHan, 200}) {
            const std::vector<LayoutLine> lines = layoutText(input, width, fakeMeasure);
            EXPECT_EQ(stripSpaces(joinAll(lines)), stripSpaces(input))
                << "输入 \"" << input << "\"，行宽 " << width;
        }
    }
}
