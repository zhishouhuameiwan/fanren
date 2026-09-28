#include "io/QuestLoader.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using core::QuestCondition;

// 章号的合法范围：大纲 v3.1 是 14 章。
constexpr int kFirstChapter = 1;
constexpr int kLastChapter = 14;

// 解析失败时带回来的那一句。每一层把自己的位置拼在前面，最后形如
// 「data/quests/q.json 的 steps[1].done[0]：op 只认 ">=" 或 "=="」。
struct Problem {
    std::string message;
};

[[nodiscard]] Problem problem(const std::string& where, const std::string& what) {
    return Problem{where + "：" + what};
}

// 表外字段一律报错（契约 1.2）。拼错一个 fail 会让任务永远不会过期，拼错一个
// target_object 会让它悄悄变成「不指路」——那两种错都不会自己暴露。
[[nodiscard]] bool firstUnknownKey(const json& object, std::initializer_list<const char*> known,
                                   std::string& unknown) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        const bool listed = std::any_of(known.begin(), known.end(),
                                        [&it](const char* key) { return it.key() == key; });
        if (!listed) {
            unknown = it.key();
            return true;
        }
    }
    return false;
}

// 必填的非空字符串。
[[nodiscard]] bool readRequiredString(const json& object, const char* field, std::string& out) {
    if (!object.contains(field) || !object[field].is_string()) return false;
    out = object[field].get<std::string>();
    return !out.empty();
}

// 可选字符串：不在就留空；在却不是字符串判失败（不当成「没写」悄悄放过）。
[[nodiscard]] bool readOptionalString(const json& object, const char* field, std::string& out) {
    if (!object.contains(field)) return true;
    if (!object[field].is_string()) return false;
    out = object[field].get<std::string>();
    return true;
}

// 整数，且落在 int 里。true / 1.0 / "1" 都不是整数。
[[nodiscard]] bool readInt(const json& node, int& out) {
    if (!node.is_number_integer()) return false;
    if (node.is_number_unsigned()) {
        const auto value = node.get<std::uint64_t>();
        if (value > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) return false;
        out = static_cast<int>(value);
        return true;
    }
    const auto value = node.get<std::int64_t>();
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

// ---- 谓词（契约 1.2 第三张表）----
//
// 恰好三种写法。每一种的下限都有来由：`flag >= 0` 与 `item >= 0` 恒真，是一句空话，
// 写出来的人本意多半是 `>= 1`；`flag == 0` 是「没置过」，有用，所以 == 放行 0。
[[nodiscard]] bool parseCondition(const json& node, const std::string& where,
                                  QuestCondition& out, Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "谓词必须是对象");
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"flag", "item", "op", "value"}, unknown)) {
        why = problem(where, "谓词里有不认识的字段 " + unknown + "（只认 flag / item / op / value）");
        return false;
    }
    const bool hasFlag = node.contains("flag");
    const bool hasItem = node.contains("item");
    if (hasFlag == hasItem) {
        why = problem(where, "flag 与 item 必须恰好出现一个");
        return false;
    }
    const char* subjectField = hasFlag ? "flag" : "item";
    if (!readRequiredString(node, subjectField, out.subject)) {
        why = problem(where, std::string(subjectField) + " 必须是非空字符串");
        return false;
    }
    std::string op;
    if (!readRequiredString(node, "op", op) || (op != ">=" && op != "==")) {
        why = problem(where, "op 只认 \">=\" 或 \"==\"");
        return false;
    }
    if (!node.contains("value") || !readInt(node["value"], out.value)) {
        why = problem(where, "value 必须是整数");
        return false;
    }

    if (hasItem) {
        if (op != ">=") {
            why = problem(where, "物品谓词只认 \">=\"（背包里「正好几件」不是一个站得住的条件）");
            return false;
        }
        if (out.value < 1) {
            why = problem(where, "物品谓词的 value 至少为 1（>= 0 恒真）");
            return false;
        }
        out.op = QuestCondition::Op::ItemAtLeast;
        return true;
    }
    if (op == ">=") {
        if (out.value < 1) {
            why = problem(where, "旗标 >= 的 value 至少为 1（>= 0 恒真）");
            return false;
        }
        out.op = QuestCondition::Op::FlagAtLeast;
        return true;
    }
    if (out.value < 0) {
        why = problem(where, "旗标 == 的 value 不许为负");
        return false;
    }
    out.op = QuestCondition::Op::FlagEquals;
    return true;
}

// 一张谓词表。requireNonEmpty 为真时缺字段或空表都判失败。
[[nodiscard]] bool parseConditionList(const json& parent, const char* field,
                                      const std::string& where, bool requireNonEmpty,
                                      std::vector<QuestCondition>& out, Problem& why) {
    if (!parent.contains(field)) {
        if (!requireNonEmpty) return true;
        why = problem(where, std::string("缺少 ") + field);
        return false;
    }
    const json& list = parent[field];
    if (!list.is_array()) {
        why = problem(where, std::string(field) + " 必须是数组");
        return false;
    }
    if (requireNonEmpty && list.empty()) {
        why = problem(where, std::string(field) + " 不许为空");
        return false;
    }
    for (std::size_t i = 0; i < list.size(); ++i) {
        QuestCondition condition;
        if (!parseCondition(list[i], where + " 的 " + field + "[" + std::to_string(i) + "]",
                            condition, why)) {
            return false;
        }
        out.push_back(std::move(condition));
    }
    return true;
}

[[nodiscard]] bool parseStep(const json& node, const std::string& where, core::QuestStep& out,
                             Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "步骤必须是对象");
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"id", "text_key", "done", "target_map", "target_object"}, unknown)) {
        why = problem(where, "步骤里有不认识的字段 " + unknown);
        return false;
    }
    if (!readRequiredString(node, "id", out.id)) {
        why = problem(where, "步骤缺少 id");
        return false;
    }
    const std::string here = where + "（" + out.id + "）";
    if (!readRequiredString(node, "text_key", out.textKey)) {
        why = problem(here, "缺少 text_key");
        return false;
    }
    if (!parseConditionList(node, "done", here, /*requireNonEmpty=*/true, out.done, why)) {
        return false;
    }
    if (!readOptionalString(node, "target_map", out.targetMap) ||
        !readOptionalString(node, "target_object", out.targetObject)) {
        why = problem(here, "target_map / target_object 必须是字符串");
        return false;
    }
    // 成对（契约 Q9）：只写了图没写对象，告示板说得出地名却指不出是谁；
    // 只写了对象没写图，那个对象名在哪张图上都可能不存在。
    if (out.targetMap.empty() != out.targetObject.empty()) {
        why = problem(here, "target_map 与 target_object 必须成对：要么都写，要么都不写");
        return false;
    }
    return true;
}

[[nodiscard]] bool parseReward(const json& node, const std::string& where, core::BagEntry& out,
                               Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "奖励必须是对象");
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, {"item_id", "count", "herb_age"}, unknown)) {
        why = problem(where, "奖励里有不认识的字段 " + unknown);
        return false;
    }
    if (!readRequiredString(node, "item_id", out.itemId)) {
        why = problem(where, "奖励缺少 item_id");
        return false;
    }
    if (!node.contains("count") || !readInt(node["count"], out.count) || out.count < 1) {
        why = problem(where, "奖励的 count 必须是 ≥ 1 的整数");
        return false;
    }
    out.herbAge = 0;
    if (node.contains("herb_age") && (!readInt(node["herb_age"], out.herbAge) || out.herbAge < 0)) {
        why = problem(where, "奖励的 herb_age 必须是 ≥ 0 的整数");
        return false;
    }
    return true;
}

[[nodiscard]] bool parseQuest(const json& j, const std::string& where, core::Quest& out,
                              Problem& why) {
    if (!j.is_object()) {
        why = problem(where, "任务文件顶层必须是对象");
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(j,
                        {"id", "name", "chapter", "kind", "title_key", "summary_key", "accept",
                         "steps", "complete", "fail", "fail_text_key", "rewards", "origin",
                         "note"},
                        unknown)) {
        why = problem(where, "有不认识的字段 " + unknown + "（字段表见 docs/interfaces-p3-ch05.md 1.2）");
        return false;
    }
    if (!readRequiredString(j, "id", out.id)) {
        why = problem(where, "缺少 id");
        return false;
    }
    if (!readRequiredString(j, "name", out.name)) {
        why = problem(where, "缺少 name");
        return false;
    }
    if (!j.contains("chapter") || !readInt(j["chapter"], out.chapter) ||
        out.chapter < kFirstChapter || out.chapter > kLastChapter) {
        why = problem(where, "chapter 必须是 1–14 的整数");
        return false;
    }
    std::string kind;
    if (!readRequiredString(j, "kind", kind)) {
        why = problem(where, "缺少 kind");
        return false;
    }
    if (kind != "side") {
        // 主线只从 data/objectives/ 来（契约 1.5）。两处都能写主线，就得定「两处都写了
        // 算谁的」，而那种规则没人记得住。
        why = problem(where, "data/quests/ 里只许 kind = \"side\"：主线由目标链原地生成，见契约 1.5");
        return false;
    }
    out.kind = core::QuestKind::Side;
    if (!readRequiredString(j, "title_key", out.titleKey)) {
        why = problem(where, "缺少 title_key");
        return false;
    }
    if (!readRequiredString(j, "summary_key", out.summaryKey)) {
        why = problem(where, "缺少 summary_key");
        return false;
    }
    if (!parseConditionList(j, "accept", where, /*requireNonEmpty=*/true, out.accept, why)) {
        return false;
    }

    if (!j.contains("steps") || !j["steps"].is_array() || j["steps"].empty()) {
        why = problem(where, "steps 必须是非空数组");
        return false;
    }
    std::set<std::string> stepIds;
    for (std::size_t i = 0; i < j["steps"].size(); ++i) {
        core::QuestStep step;
        if (!parseStep(j["steps"][i], where + " 的 steps[" + std::to_string(i) + "]", step, why)) {
            return false;
        }
        if (!stepIds.insert(step.id).second) {
            why = problem(where, "步骤 id 重复：" + step.id);
            return false;
        }
        out.steps.push_back(std::move(step));
    }

    if (!parseConditionList(j, "complete", where, /*requireNonEmpty=*/true, out.complete, why)) {
        return false;
    }
    if (!parseConditionList(j, "fail", where, /*requireNonEmpty=*/false, out.fail, why)) {
        return false;
    }
    // fail 与 fail_text_key 成对（契约 Q7）。有 fail 没原因：过期了只写「已过期」三个字，
    // 玩家不知道为什么；有原因没 fail：一句永远说不出口的话，多半是 fail 拼错了。
    if (!out.fail.empty()) {
        if (!readRequiredString(j, "fail_text_key", out.failTextKey)) {
            why = problem(where, "写了 fail 就必须写 fail_text_key（过期要给一句原因）");
            return false;
        }
    } else if (j.contains("fail_text_key")) {
        why = problem(where, "fail 为空却写了 fail_text_key：这句话永远说不出口");
        return false;
    }

    if (j.contains("rewards")) {
        if (!j["rewards"].is_array()) {
            why = problem(where, "rewards 必须是数组");
            return false;
        }
        for (std::size_t i = 0; i < j["rewards"].size(); ++i) {
            core::BagEntry reward;
            if (!parseReward(j["rewards"][i], where + " 的 rewards[" + std::to_string(i) + "]",
                             reward, why)) {
                return false;
            }
            out.rewards.push_back(std::move(reward));
        }
    }
    if (!readOptionalString(j, "origin", out.origin) || !readOptionalString(j, "note", out.note)) {
        why = problem(where, "origin / note 必须是字符串");
        return false;
    }
    return true;
}

std::vector<fs::path> listQuestFiles(const fs::path& dir) {
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::exists(dir, ec) || !fs::is_directory(dir, ec)) return files;
    for (auto it = fs::recursive_directory_iterator(dir, fs::directory_options::skip_permission_denied, ec);
         it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (ec) break;
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") {
            files.push_back(it->path());
        }
    }
    // 排序不是装饰：重复 id 的报错要写明「哪两个文件」，跟着文件系统的返回次序走，
    // 同一份数据在两台机器上会报出两句不同的话。
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace

core::Result<core::Quest> loadQuestFile(const std::string& path) {
    using Out = core::Result<core::Quest>;
    core::Result<json> parsed = detail::readJsonFile(fs::path(path));
    if (!parsed) return Out::failure(parsed.error);
    core::Quest quest;
    Problem why;
    if (!parseQuest(parsed.value, path, quest, why)) return Out::failure(why.message);
    return Out::success(std::move(quest));
}

core::Result<std::vector<core::Quest>> loadQuests(const std::string& questsDir) {
    using Out = core::Result<std::vector<core::Quest>>;
    std::vector<core::Quest> quests;
    std::map<std::string, std::string> sourceOf;
    for (const fs::path& file : listQuestFiles(fs::path(questsDir))) {
        core::Result<core::Quest> one = loadQuestFile(file.string());
        if (!one) return Out::failure(one.error);
        const auto seen = sourceOf.find(one.value.id);
        if (seen != sourceOf.end()) {
            return Out::failure("任务 id 重复：" + one.value.id + "，见 " + seen->second + " 与 " +
                                file.string());
        }
        sourceOf.emplace(one.value.id, file.string());
        quests.push_back(std::move(one.value));
    }
    std::stable_sort(quests.begin(), quests.end(), [](const core::Quest& a, const core::Quest& b) {
        if (a.chapter != b.chapter) return a.chapter < b.chapter;
        return a.id < b.id;
    });
    return Out::success(std::move(quests));
}

}  // namespace fanren::io
