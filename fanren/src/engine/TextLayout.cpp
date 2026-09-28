#include "engine/TextLayout.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace fanren::engine {
namespace {

// ---------------------------------------------------------------------------
// UTF-8 解码
//
// 这里刻意不用 std::codecvt / mbrtoc32：前者已废弃，后者依赖 locale，
// 而对白文本必须在任何 locale 下都按 UTF-8 解，不能随机器设置飘。
// ---------------------------------------------------------------------------

std::uint32_t decodeCodepoint(const std::string& s, std::size_t pos, std::size_t len) {
    const auto lead = static_cast<unsigned char>(s[pos]);
    if (len == 1) {
        return lead;
    }
    static constexpr std::array<std::uint32_t, 5> kLeadMask{0u, 0x7Fu, 0x1Fu, 0x0Fu, 0x07u};
    std::uint32_t cp = lead & kLeadMask[len];
    for (std::size_t i = 1; i < len; ++i) {
        cp = (cp << 6) | (static_cast<unsigned char>(s[pos + i]) & 0x3Fu);
    }
    return cp;
}

// 返回 pos 处一个码点占的字节数。遇到非法、被截断或非规范的序列一律返回 1，
// 保证调用方在任何输入（包括半个汉字、随机二进制）下都能推进，不会死循环，
// 也不会丢字节——非法字节会被当成一个单字节 grapheme 原样留下。
//
// 只检查「长度与续字节」是不够的：存档名、将来的 mod 文案都是玩家可控文本，
// 非规范序列如果被当成合法码点放行，就成了脏数据入口。因此额外拒绝三类：
//   - overlong：用超出必要的字节数编码小码点（经典的 "\xC0\xAF" 绕过过滤手法）
//   - 孤立代理项 U+D800–U+DFFF：UTF-8 里根本不该出现
//   - 超出 Unicode 上限 U+10FFFF（首字节 F5–F7）
std::size_t codepointLength(const std::string& s, std::size_t pos) {
    const auto lead = static_cast<unsigned char>(s[pos]);
    std::size_t len = 0;
    if ((lead & 0x80u) == 0x00u) {
        return 1;
    } else if ((lead & 0xE0u) == 0xC0u) {
        len = 2;
    } else if ((lead & 0xF0u) == 0xE0u) {
        len = 3;
    } else if ((lead & 0xF8u) == 0xF0u) {
        len = 4;
    } else {
        return 1;  // 续字节，或 0xF8 以上的非法首字节
    }
    if (pos + len > s.size()) {
        return 1;
    }
    for (std::size_t i = 1; i < len; ++i) {
        if ((static_cast<unsigned char>(s[pos + i]) & 0xC0u) != 0x80u) {
            return 1;  // 序列中途断掉
        }
    }

    // 每种长度能合法表示的最小码点；低于它就是 overlong。
    static constexpr std::array<std::uint32_t, 5> kMinCodepoint{0u, 0u, 0x80u, 0x800u, 0x10000u};
    const std::uint32_t cp = decodeCodepoint(s, pos, len);
    if (cp < kMinCodepoint[len]) {
        return 1;
    }
    if (cp >= 0xD800u && cp <= 0xDFFFu) {
        return 1;
    }
    if (cp > 0x10FFFFu) {
        return 1;
    }
    return len;
}

// 附着字符：组合符号、变体选择符、肤色修饰符。它们没有独立宽度，
// 必须跟基字符粘在同一个 grapheme 里，否则逐字显示时会先蹦出一个孤零零的音调符。
bool isExtendCodepoint(std::uint32_t cp) {
    return (cp >= 0x0300u && cp <= 0x036Fu) ||    // 组合附加符号
           (cp >= 0x1AB0u && cp <= 0x1AFFu) ||    // 组合附加符号扩展
           (cp >= 0x1DC0u && cp <= 0x1DFFu) ||    // 组合附加符号补充
           (cp >= 0x20D0u && cp <= 0x20FFu) ||    // 组合用符号（含 U+20E3 keycap）
           (cp >= 0xFE00u && cp <= 0xFE0Fu) ||    // 变体选择符 VS1-16
           (cp >= 0xFE20u && cp <= 0xFE2Fu) ||    // 半标记
           (cp >= 0x1F3FBu && cp <= 0x1F3FFu) ||  // emoji 肤色修饰符
           (cp >= 0xE0100u && cp <= 0xE01EFu);    // 变体选择符补充
}

bool isRegionalIndicator(std::uint32_t cp) {
    return cp >= 0x1F1E6u && cp <= 0x1F1FFu;
}

constexpr std::uint32_t kZeroWidthJoiner = 0x200Du;

// ---------------------------------------------------------------------------
// 标点禁则表
//
// 表按 UTF-8 串比对而非码点：排版全程都在跟 std::string 打交道，
// 多一层码点转换只会多一处出错的地方。
// ---------------------------------------------------------------------------
bool isLeadingForbidden(const std::string& g) {
    static const std::array<const char*, 13> kTable{
        "\xE3\x80\x82",  // 。 U+3002
        "\xEF\xBC\x8C",  // ， U+FF0C
        "\xE3\x80\x81",  // 、 U+3001
        "\xEF\xBC\x9B",  // ； U+FF1B
        "\xEF\xBC\x9A",  // ： U+FF1A
        "\xEF\xBC\x9F",  // ？ U+FF1F
        "\xEF\xBC\x81",  // ！ U+FF01
        "\xEF\xBC\x89",  // ） U+FF09
        "\xE3\x80\x8B",  // 》 U+300B
        "\xE3\x80\x8D",  // 」 U+300D
        "\xE3\x80\x8F",  // 』 U+300F
        "\xE3\x80\x91",  // 】 U+3011
        "%",
    };
    return std::find(kTable.begin(), kTable.end(), g) != kTable.end();
}

bool isTrailingForbidden(const std::string& g) {
    static const std::array<const char*, 5> kTable{
        "\xEF\xBC\x88",  // （ U+FF08
        "\xE3\x80\x8A",  // 《 U+300A
        "\xE3\x80\x8C",  // 「 U+300C
        "\xE3\x80\x8E",  // 『 U+300E
        "\xE3\x80\x90",  // 【 U+3010
    };
    return std::find(kTable.begin(), kTable.end(), g) != kTable.end();
}

// ---------------------------------------------------------------------------
// 断行单元
// ---------------------------------------------------------------------------

// 可见 ASCII（不含空格）。连续的这类字符合成一个不可分单元，
// 于是 "Hello," 与 "HP:100/200" 不会被从中间劈开。
bool isAsciiWordByte(unsigned char c) {
    return c > 0x20u && c < 0x7Fu;
}

bool isBreakableSpace(const std::string& g) {
    return g == " " || g == "\t";
}

struct Unit {
    std::string text;
    bool space = false;
};

// 把 grapheme 序列合并成断行单元：连续 ASCII 一个、空白各自一个、其余每字一个。
std::vector<Unit> buildUnits(const std::vector<std::string>& graphemes) {
    std::vector<Unit> units;
    units.reserve(graphemes.size());
    for (const std::string& g : graphemes) {
        if (isBreakableSpace(g)) {
            units.push_back(Unit{g, true});
            continue;
        }
        const bool ascii = g.size() == 1 && isAsciiWordByte(static_cast<unsigned char>(g[0]));
        const bool extendsPrevious =
            ascii && !units.empty() && !units.back().space &&
            isAsciiWordByte(static_cast<unsigned char>(units.back().text.back()));
        if (extendsPrevious) {
            units.back().text += g;
        } else {
            units.push_back(Unit{g, false});
        }
    }
    return units;
}

using MeasureFn = std::function<int(const std::string&)>;

std::string firstGrapheme(const std::string& unit) {
    if (unit.empty()) {
        return {};
    }
    return unit.substr(0, codepointLength(unit, 0));
}

std::string lastGrapheme(const std::string& unit) {
    const std::vector<std::string> gs = splitGraphemes(unit);
    return gs.empty() ? std::string{} : gs.back();
}

// 单元宽度超过整行可用宽度时按字符硬切。
// 契约说西文单词不从中间断开，但那是「放得下时」的约定；一个 300px 的单词
// 塞进 200px 的对话框如果不切，字会直接画到框外，比断词更糟。
std::vector<Unit> splitOversizedUnits(const std::vector<Unit>& units, int maxWidthPx,
                                      const MeasureFn& measure) {
    std::vector<Unit> out;
    out.reserve(units.size());
    for (const Unit& u : units) {
        if (u.space || measure(u.text) <= maxWidthPx) {
            out.push_back(u);
            continue;
        }
        const std::vector<std::string> gs = splitGraphemes(u.text);
        if (gs.size() <= 1) {
            out.push_back(u);  // 单个字符就超宽（行宽不足一字），切无可切
            continue;
        }
        std::string chunk;
        for (const std::string& g : gs) {
            if (!chunk.empty() && measure(chunk + g) > maxWidthPx) {
                out.push_back(Unit{chunk, false});
                chunk.clear();
            }
            chunk += g;
        }
        if (!chunk.empty()) {
            out.push_back(Unit{chunk, false});
        }
    }
    return out;
}

// 每行最多挪 2 个单元。不设上限的话，一串「）））））…」会把整段文字
// 全部回退到第一行上去；悬挂两个标点是排版上通行的折中。
constexpr int kMaxKinsokuShift = 2;

using UnitLine = std::vector<std::string>;

// 行首禁则：把落在行首的禁止字符拉回上一行（悬挂标点）。
// 上一行因此可能略微超宽，这是禁则本身要求的代价。
void applyLeadingKinsoku(std::vector<UnitLine>& lines) {
    for (std::size_t i = 1; i < lines.size();) {
        // hung 跨越「整行被拉空后接着拉下一行」的情况继续累计，
        // 否则 "。。。。" 这种一字一行的病态输入会被整段吸到第一行上去。
        int hung = 0;
        while (hung < kMaxKinsokuShift && !lines[i].empty() && !lines[i - 1].empty() &&
               isLeadingForbidden(firstGrapheme(lines[i].front()))) {
            lines[i - 1].push_back(lines[i].front());
            lines[i].erase(lines[i].begin());
            ++hung;
            if (lines[i].empty() && i + 1 < lines.size()) {
                lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(i));
            }
        }
        if (lines[i].empty()) {
            lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

// 行尾禁则：把落在行尾的禁止字符挤到下一行。
// size() >= 2 的门槛保证不会把一行挤空——否则一个孤立的「（」会永远往下掉。
void applyTrailingKinsoku(std::vector<UnitLine>& lines) {
    for (std::size_t i = 0; i < lines.size(); ++i) {
        for (int moved = 0; moved < kMaxKinsokuShift; ++moved) {
            if (lines[i].size() < 2 || !isTrailingForbidden(lastGrapheme(lines[i].back()))) {
                break;
            }
            std::string unit = lines[i].back();
            lines[i].pop_back();
            if (i + 1 >= lines.size()) {
                lines.emplace_back();
            }
            lines[i + 1].insert(lines[i + 1].begin(), std::move(unit));
        }
    }
}

// 贪心按宽度填行。行首空白丢弃，断行处的空白也丢弃，
// 否则居中或右对齐时会出现看不见却占位的尾随空格。
std::vector<UnitLine> greedyFill(const std::vector<Unit>& units, int maxWidthPx,
                                 const MeasureFn& measure) {
    std::vector<UnitLine> lines;
    UnitLine current;
    std::string currentText;

    auto flush = [&]() {
        while (!current.empty() && isBreakableSpace(current.back())) {
            current.pop_back();
        }
        lines.push_back(current);
        current.clear();
        currentText.clear();
    };

    for (const Unit& u : units) {
        if (current.empty() && u.space) {
            continue;
        }
        // 空行时无条件收下第一个单元：超宽单元此前已被切过，收不下也只能这样放。
        if (current.empty() || measure(currentText + u.text) <= maxWidthPx) {
            current.push_back(u.text);
            currentText += u.text;
            continue;
        }
        flush();
        if (u.space) {
            continue;
        }
        current.push_back(u.text);
        currentText = u.text;
    }
    if (!current.empty()) {
        flush();
    }
    return lines;
}

std::string joinUnits(const UnitLine& line) {
    std::string out;
    for (const std::string& u : line) {
        out += u;
    }
    return out;
}

// 按硬换行切段。末尾的单个空段丢掉：「甲。\n」是一行而不是两行。
std::vector<std::string> splitParagraphs(const std::string& utf8) {
    std::vector<std::string> parts;
    std::string current;
    for (const char ch : utf8) {
        if (ch == '\n') {
            parts.push_back(current);
            current.clear();
        } else if (ch != '\r') {
            current += ch;
        }
    }
    parts.push_back(current);
    if (parts.size() > 1 && parts.back().empty()) {
        parts.pop_back();
    }
    return parts;
}

}  // namespace

std::vector<std::string> splitGraphemes(const std::string& utf8) {
    std::vector<std::string> out;
    // 每个 grapheme 至少占 1 字节，所以字节数是元素数的上界：一次 reserve 之后
    // 不会再发生扩容。本函数在热路径上（逐段、逐单元、lastGrapheme 都会调）。
    out.reserve(utf8.size());
    std::size_t i = 0;
    while (i < utf8.size()) {
        const std::size_t start = i;
        std::size_t len = codepointLength(utf8, i);
        std::uint32_t cp = decodeCodepoint(utf8, i, len);
        i += len;

        // 区域指示符成对才是一面国旗，落单的就是落单的。
        if (isRegionalIndicator(cp) && i < utf8.size()) {
            const std::size_t nextLen = codepointLength(utf8, i);
            if (isRegionalIndicator(decodeCodepoint(utf8, i, nextLen))) {
                i += nextLen;
            }
        }

        // 吞掉后续附着字符；碰到 ZWJ 就连同它后面那个码点一起并进来
        // （👨‍👩‍👧 这类组合 emoji 在逐字显示时必须整体出现）。
        while (i < utf8.size()) {
            len = codepointLength(utf8, i);
            cp = decodeCodepoint(utf8, i, len);
            if (isExtendCodepoint(cp)) {
                i += len;
                continue;
            }
            if (cp == kZeroWidthJoiner) {
                i += len;
                if (i < utf8.size()) {
                    i += codepointLength(utf8, i);
                }
                continue;
            }
            break;
        }
        out.push_back(utf8.substr(start, i - start));
    }
    return out;
}

std::vector<LayoutLine> layoutText(const std::string& utf8, int maxWidthPx,
                                   const std::function<int(const std::string&)>& measure) {
    std::vector<LayoutLine> result;
    if (utf8.empty()) {
        return result;
    }

    const std::vector<std::string> paragraphs = splitParagraphs(utf8);

    // 没有测量函数、或行宽非正时，「断在哪」无从定义。原样返回整段，
    // 至少文字还显示得出来，而不是退化成一字一行的上万行。
    if (!measure || maxWidthPx <= 0) {
        for (const std::string& p : paragraphs) {
            result.push_back(LayoutLine{p, measure ? measure(p) : 0});
        }
        return result;
    }

    for (const std::string& paragraph : paragraphs) {
        if (paragraph.empty()) {
            result.push_back(LayoutLine{std::string{}, 0});  // 保留空行
            continue;
        }
        std::vector<Unit> units = buildUnits(splitGraphemes(paragraph));
        units = splitOversizedUnits(units, maxWidthPx, measure);

        std::vector<UnitLine> lines = greedyFill(units, maxWidthPx, measure);
        applyLeadingKinsoku(lines);
        applyTrailingKinsoku(lines);

        for (const UnitLine& line : lines) {
            std::string text = joinUnits(line);
            const int width = measure(text);
            result.push_back(LayoutLine{std::move(text), width});
        }
    }
    return result;
}

}  // namespace fanren::engine
