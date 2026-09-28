#include "io/ChapterLoader.h"

#include <utility>

#include <nlohmann/json.hpp>

#include "io/StrictJson.h"

namespace fanren::io {
namespace {

using nlohmann::json;

// 成员存在且是字符串才取，否则空串——空串随后按「缺字段」报错（与原先 ui::Json::stringAt 同一口径）。
std::string stringAt(const json& owner, const char* key) {
    const auto it = owner.find(key);   // 不是对象时 find 返回 end()
    return it != owner.end() && it->is_string() ? it->get<std::string>() : std::string{};
}

// 章号：是整数（1.0 也算）且不过十亿才取，否则 0——0 随后按「缺 number」报错。
int numberAt(const json& owner, const char* key) {
    const auto it = owner.find(key);
    int n = 0;
    if (it == owner.end() || !detail::toInt(*it, n) || n > 1'000'000'000) return 0;
    return n;
}

}  // namespace

core::Result<ChapterFile> loadChapterFile(const std::string& path) {
    using R = core::Result<ChapterFile>;
    const auto text = detail::readTextFile(path);
    if (!text) return R::failure(text.error);
    const auto parsed = detail::parseStrictJson(text.value);
    if (!parsed) return R::failure(path + "：" + parsed.error);
    const json& root = parsed.value;

    ChapterFile file;
    file.closingKey = stringAt(root, "closing_key");
    file.toBeContinuedKey = stringAt(root, "to_be_continued_key");
    const auto list = root.find("chapters");
    if (file.closingKey.empty() || file.toBeContinuedKey.empty() || list == root.end() || !list->is_array()) {
        return R::failure(path + "：缺 closing_key / to_be_continued_key / chapters");
    }
    for (const json& item : *list) {
        ChapterRow row;
        row.number = numberAt(item, "number");
        row.numeralKey = stringAt(item, "numeral_key");
        row.titleKey = stringAt(item, "title_key");
        row.doneFlag = stringAt(item, "done_flag");
        if (row.number <= 0 || row.numeralKey.empty() || row.titleKey.empty() || row.doneFlag.empty()) {
            return R::failure(path + "：第 " + std::to_string(file.chapters.size() + 1) +
                              " 行缺 number / numeral_key / title_key / done_flag");
        }
        // 严格升序：「下一章」就是表里的下一行，次序乱了排出来的开篇卡就是错的那一章。
        if (!file.chapters.empty() && row.number <= file.chapters.back().number) {
            return R::failure(path + "：章号必须严格升序（第 " + std::to_string(row.number) + " 章排在第 " +
                              std::to_string(file.chapters.back().number) + " 章之后）");
        }
        file.chapters.push_back(std::move(row));
    }
    return R::success(std::move(file));
}

}  // namespace fanren::io
