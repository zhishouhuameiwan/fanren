#pragma once
// 中文排版：按像素宽断行 + 标点禁则 + UTF-8 安全的逐字切分。
//
// 本文件刻意不依赖 SDL：宽度测量由调用方以回调注入，单测因此可以用一个
// 假的等宽测量函数跑完全部分支，不需要窗口、字体文件或显示设备。
#include <functional>
#include <string>
#include <vector>

namespace fanren::engine {

struct LayoutLine {
    std::string text;
    int width{};
};

// 按像素宽度断行，遵守中文标点禁则：
//   行首禁止：。，、；：？！）》」』】%
//   行尾禁止：（《「『【
// 西文单词不从中间断开；连续 ASCII 视为一个不可分单元。
// measure 为字符串宽度测量函数，便于脱离 SDL 单测。
std::vector<LayoutLine> layoutText(
    const std::string& utf8,
    int maxWidthPx,
    const std::function<int(const std::string&)>& measure);

// UTF-8 安全的字符切分：按「字符」而非字节前进，供逐字显示用。
std::vector<std::string> splitGraphemes(const std::string& utf8);

}  // namespace fanren::engine
