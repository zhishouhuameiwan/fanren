#pragma once
// 章节表 data/chapters.json 的读取器：章号、文案 key 与章末旗标，按表里的次序。
// 把 key 换成正文、问存档「那一章演完没有」在 game/ChapterCardScene.h（loadChapterTable）。
//
// 原先由界面层的手写 JSON 读取器（ui::Json）读——nlohmann 只链给 io 层，界面路当时碰不到 io。
// 美术描述文件统一进 io 层时，那个读取器随之退役，这张表也就收到这里。
// 口径与原先逐条相同：缺 closing_key / to_be_continued_key / chapters、某一行缺 number / numeral_key /
// title_key / done_flag、章号不严格升序，都是整份报错（「下一章」就是表里的下一行，次序乱了开篇卡就排错）。
//
// 结构体只有章节卡一个用处，不进 core/model，放在这里。
#include <string>
#include <vector>

#include "core/Result.h"

namespace fanren::io {

struct ChapterRow {
    int number = 0;
    std::string numeralKey;   // 「第三章」的文案 key
    std::string titleKey;     // 「神手谷惊变」的文案 key
    std::string doneFlag;     // 「ch03.done」
};

struct ChapterFile {
    std::string closingKey;         // 「终」的文案 key
    std::string toBeContinuedKey;   // 「未完待续」的文案 key
    std::vector<ChapterRow> chapters;   // 章号严格升序（读的时候查过）
};

// 读不了、语法错、缺字段、章号不升序都是失败，error 里带路径。文件不存在同样是失败——
// 「没有章节表是合法的」是调用方的口径（loadChapterTable 先查文件在不在）。
[[nodiscard]] core::Result<ChapterFile> loadChapterFile(const std::string& path);

}  // namespace fanren::io
