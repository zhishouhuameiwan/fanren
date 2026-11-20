#include "io/GuideLoader.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <set>
#include <utility>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

template <typename T>
using Out = core::Result<std::vector<T>>;

[[nodiscard]] bool firstUnknownKey(const json& object, std::initializer_list<const char*> known, std::string& unknown) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        const bool listed =
            std::any_of(known.begin(), known.end(), [&it](const char* key) { return it.key() == key; });
        if (!listed) {
            unknown = it.key();
            return true;
        }
    }
    return false;
}

// 必填的非空字符串。
[[nodiscard]] bool readRequired(const json& object, const char* key, std::string& out) {
    if (!object.contains(key) || !object[key].is_string() || object[key].get_ref<const std::string&>().empty()) {
        return false;
    }
    out = object[key].get<std::string>();
    return true;
}

// 可选的字符串：没写保持空；写了就得是字符串（可以是空串）。
[[nodiscard]] bool readOptional(const json& object, const char* key, std::string& out) {
    if (!object.contains(key)) return true;
    if (!object[key].is_string()) return false;
    out = object[key].get<std::string>();
    return true;
}

[[nodiscard]] bool readOptionalInt(const json& object, const char* key, int& out) {
    if (!object.contains(key)) return true;
    if (!object[key].is_number_integer()) return false;
    out = object[key].get<int>();
    return true;
}

// 目录下的 .json，递归、按路径排序。目录不存在返回空表。
[[nodiscard]] core::Result<std::vector<fs::path>> jsonFiles(const std::string& dir) {
    using R = core::Result<std::vector<fs::path>>;
    std::vector<fs::path> files;
    std::error_code ec;
    const fs::path root(dir);
    if (!fs::exists(root, ec)) return R::success(std::move(files));
    if (!fs::is_directory(root, ec)) return R::failure(root.string() + " 不是目录");
    for (auto it = fs::recursive_directory_iterator(root, ec); !ec && it != fs::recursive_directory_iterator();
         it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == ".json") files.push_back(it->path());
    }
    if (ec) return R::failure("读不了目录 " + root.string() + "：" + ec.message());
    std::sort(files.begin(), files.end());
    return R::success(std::move(files));
}

[[nodiscard]] core::Result<std::vector<core::TutorialCard>> loadTutorialFile(const fs::path& path,
                                                                           std::set<std::string>& seen) {
    using R = Out<core::TutorialCard>;
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return R::failure(parsed.error);
    const json& root = parsed.value;
    const std::string where = path.string();
    std::string unknown;
    if (!root.is_object()) return R::failure("教学卡文件顶层必须是对象: " + where);
    if (firstUnknownKey(root, {"id", "name", "cards"}, unknown)) {
        return R::failure("教学卡文件有表外字段 \"" + unknown + "\": " + where);
    }
    if (!root.contains("cards") || !root["cards"].is_array()) return R::failure("教学卡文件缺 cards 数组: " + where);

    std::vector<core::TutorialCard> cards;
    for (const json& node : root["cards"]) {
        core::TutorialCard card;
        if (!node.is_object()) return R::failure("cards 里有非对象项: " + where);
        if (firstUnknownKey(node, {"id", "title_key", "body_keys", "trigger", "min_chapter", "max_chapter"},
                            unknown)) {
            return R::failure("教学卡有表外字段 \"" + unknown + "\": " + where);
        }
        if (!readRequired(node, "id", card.id)) return R::failure("教学卡缺 id: " + where);
        const std::string at = "教学卡 " + card.id;
        if (!seen.insert(card.id).second) return R::failure(at + " 的 id 重复: " + where);
        if (!readRequired(node, "title_key", card.titleKey)) return R::failure(at + " 缺 title_key: " + where);
        if (!node.contains("body_keys") || !node["body_keys"].is_array() || node["body_keys"].empty()) {
            return R::failure(at + " 的 body_keys 应当是非空数组: " + where);
        }
        for (const json& key : node["body_keys"]) {
            if (!key.is_string() || key.get_ref<const std::string&>().empty()) {
                return R::failure(at + " 的 body_keys 里有空项: " + where);
            }
            card.bodyKeys.push_back(key.get<std::string>());
        }
        if (card.bodyKeys.size() > kTutorialMaxBodyLines) {
            return R::failure(at + " 的正文多于 " + std::to_string(kTutorialMaxBodyLines) + " 行: " + where);
        }
        if (!readOptional(node, "trigger", card.trigger)) return R::failure(at + " 的 trigger 不是字符串: " + where);
        if (!readOptionalInt(node, "min_chapter", card.minChapter) ||
            !readOptionalInt(node, "max_chapter", card.maxChapter)) {
            return R::failure(at + " 的 min_chapter / max_chapter 不是整数: " + where);
        }
        if (card.minChapter > 0 && card.maxChapter > 0 && card.minChapter > card.maxChapter) {
            return R::failure(at + " 的 min_chapter 大于 max_chapter: " + where);
        }
        cards.push_back(std::move(card));
    }
    return R::success(std::move(cards));
}

[[nodiscard]] core::Result<std::vector<core::LoreEntry>> loadLoreFile(const fs::path& path,
                                                                    std::set<std::string>& seen) {
    using R = Out<core::LoreEntry>;
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return R::failure(parsed.error);
    const json& root = parsed.value;
    const std::string where = path.string();
    std::string unknown;
    if (!root.is_object()) return R::failure("见闻文件顶层必须是对象: " + where);
    if (firstUnknownKey(root, {"id", "name", "chapter", "entries"}, unknown)) {
        return R::failure("见闻文件有表外字段 \"" + unknown + "\": " + where);
    }
    int chapter = 0;
    if (!readOptionalInt(root, "chapter", chapter)) return R::failure("见闻文件的 chapter 不是整数: " + where);
    if (!root.contains("entries") || !root["entries"].is_array()) {
        return R::failure("见闻文件缺 entries 数组: " + where);
    }

    const std::vector<std::string>& categories = loreCategories();
    std::vector<core::LoreEntry> entries;
    for (const json& node : root["entries"]) {
        core::LoreEntry entry;
        entry.chapter = chapter;
        if (!node.is_object()) return R::failure("entries 里有非对象项: " + where);
        if (firstUnknownKey(node, {"id", "category", "title_key", "body_key", "unlock_flag", "role_id"}, unknown)) {
            return R::failure("见闻条目有表外字段 \"" + unknown + "\": " + where);
        }
        if (!readRequired(node, "id", entry.id)) return R::failure("见闻条目缺 id: " + where);
        const std::string at = "见闻 " + entry.id;
        if (!seen.insert(entry.id).second) return R::failure(at + " 的 id 重复: " + where);
        if (!readRequired(node, "category", entry.category) ||
            std::find(categories.begin(), categories.end(), entry.category) == categories.end()) {
            return R::failure(at + " 的 category 应当是 shijian / yaoshou / lingcao / renwu 之一: " + where);
        }
        if (!readRequired(node, "title_key", entry.titleKey)) return R::failure(at + " 缺 title_key: " + where);
        if (!readRequired(node, "body_key", entry.bodyKey)) return R::failure(at + " 缺 body_key: " + where);
        if (!readOptional(node, "unlock_flag", entry.unlockFlag) || !readOptional(node, "role_id", entry.roleId)) {
            return R::failure(at + " 的 unlock_flag / role_id 不是字符串: " + where);
        }
        entries.push_back(std::move(entry));
    }
    return R::success(std::move(entries));
}

}  // namespace

const std::vector<std::string>& loreCategories() {
    static const std::vector<std::string> kCategories{"yaoshou", "lingcao", "renwu", "shijian"};
    return kCategories;
}

core::Result<std::vector<core::TutorialCard>> loadTutorialDir(const std::string& dir) {
    using R = Out<core::TutorialCard>;
    core::Result<std::vector<fs::path>> files = jsonFiles(dir);
    if (!files) return R::failure("教学卡：" + files.error);
    std::vector<core::TutorialCard> all;
    std::set<std::string> seen;
    for (const fs::path& file : files.value) {
        core::Result<std::vector<core::TutorialCard>> one = loadTutorialFile(file, seen);
        if (!one) return one;
        for (core::TutorialCard& card : one.value) all.push_back(std::move(card));
    }
    return R::success(std::move(all));
}

core::Result<std::vector<core::LoreEntry>> loadLoreDir(const std::string& dir) {
    using R = Out<core::LoreEntry>;
    core::Result<std::vector<fs::path>> files = jsonFiles(dir);
    if (!files) return R::failure("见闻：" + files.error);
    std::vector<core::LoreEntry> all;
    std::set<std::string> seen;
    for (const fs::path& file : files.value) {
        core::Result<std::vector<core::LoreEntry>> one = loadLoreFile(file, seen);
        if (!one) return one;
        for (core::LoreEntry& entry : one.value) all.push_back(std::move(entry));
    }
    return R::success(std::move(all));
}

}  // namespace fanren::io
