#include "io/PathActionLoader.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <limits>
#include <map>
#include <regex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/model/AttackCategory.h"
#include "core/rules/Realm.h"
#include "io/BattleLoader.h"
#include "io/DataLoader.h"
#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using core::PathAction;
using core::PathActionKind;
using core::QuestCondition;
using nlohmann::json;

// 章号的合法范围：大纲 v3.1 是 14 章。
constexpr int kFirstChapter = 1;
constexpr int kLastChapter = 14;

// 破绽的类别：与战斗数据、存档同一张表（core/model/AttackCategory.h 的十个中文名）。
//
// 这里原先另有一张英文标识的表（sword/fist/fire……），与战斗那一路的中文名各说各的，
// 于是「打探揭开的破绽」与「角色真有的破绽」要靠一张翻译表才对得上账——而对账恰恰
// 是门禁要做的事。战斗合回时（2026-09-26）统一成中文名，这一处只认 categoryFromName。
[[nodiscard]] bool isWeaknessCategory(const std::string& name) {
    return core::categoryFromName(name) != core::kCategoryNone;
}

// 每种行动认得的字段。表外的一律报错：拼错一个 until 会让条目挂到天荒地老，拼错一个
// refuse_key 会让阅历不足的回绝变成一句 key 本身——两种错都不会自己暴露。
// tools/validate.py 的 PATH_*_FIELDS 是同一张表，两边要一起改。
const std::set<std::string>& commonFields() {
    static const std::set<std::string> kFields = {
        "id", "kind", "map", "npc", "when", "until", "realm", "done_flag", "text_key",
        "refuse_key", "origin", "note",
    };
    return kFields;
}

const std::set<std::string>& kindFields(PathActionKind kind) {
    static const std::set<std::string> kInquire = {"reveal", "give", "set_flags"};
    static const std::set<std::string> kPurchase = {"item_id", "count", "herb_age", "price",
                                                    "deal_key", "poor_key"};
    static const std::set<std::string> kChallenge = {"battle", "pending", "win_key", "lose_key",
                                                     "reward"};
    switch (kind) {
        case PathActionKind::Inquire: return kInquire;
        case PathActionKind::Purchase: return kPurchase;
        case PathActionKind::Challenge: return kChallenge;
    }
    return kInquire;
}

// 条目 id 的尾巴就是它的种类：打探 dating、求购 qiugou、切磋 qiecuo，可再带一个序号。
// 同一个人一章里可以有几条同类条目（时段不重叠），序号就是给第二条用的。
const char* kindWord(PathActionKind kind) {
    switch (kind) {
        case PathActionKind::Inquire: return "dating";
        case PathActionKind::Purchase: return "qiugou";
        case PathActionKind::Challenge: return "qiecuo";
    }
    return "dating";
}

[[nodiscard]] bool parseKind(const std::string& text, PathActionKind& out) {
    if (text == "inquire") out = PathActionKind::Inquire;
    else if (text == "purchase") out = PathActionKind::Purchase;
    else if (text == "challenge") out = PathActionKind::Challenge;
    else return false;
    return true;
}

// 解析失败时带回来的那一句。每一层把自己的位置拼在前面，最后形如
// 「data/pathactions/ch04.json 的 entries[2]（feiyu_dating）的 until：……」。
struct Problem {
    std::string message;
};

[[nodiscard]] Problem problem(const std::string& where, const std::string& what) {
    return Problem{where + "：" + what};
}

[[nodiscard]] std::string chapterTag(int chapter) {
    return std::string("ch") + (chapter < 10 ? "0" : "") + std::to_string(chapter);
}

[[nodiscard]] bool firstUnknownKey(const json& object, const std::set<std::string>& known,
                                   const std::set<std::string>& alsoKnown, std::string& unknown) {
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (known.count(it.key()) == 0 && alsoKnown.count(it.key()) == 0) {
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

// ---- 谓词：与 io/QuestLoader.cpp 的 parseCondition 逐条同口径 ----
//
// 契约写的是「谓词照任务系统那套」，所以这里的每一条拒收都与任务那边一样：
// flag 与 item 恰好一个；op 只认 ">=" "=="；value 是整数；flag >= 与 item >= 的 value 至少为 1
// （>= 0 恒真，是一句空话）；flag == 不许为负；物品不许 ==。
// 那边改了这边要跟，tests/PathActionTests.cpp 拿同一批写法喂两个加载器，看它们是否同进同退。
[[nodiscard]] bool parseCondition(const json& node, const std::string& where, QuestCondition& out,
                                  Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "谓词必须是对象");
        return false;
    }
    std::string unknown;
    static const std::set<std::string> kConditionFields = {"flag", "item", "op", "value"};
    if (firstUnknownKey(node, kConditionFields, {}, unknown)) {
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

// 一张谓词表：必填、非空。when 与 until 都不许空——空的 when 是「一开局就挂着」，
// 空的 until 是「挂到天荒地老」，两种都是漏写。
[[nodiscard]] bool parseConditionList(const json& parent, const char* field,
                                      const std::string& where, std::vector<QuestCondition>& out,
                                      Problem& why) {
    if (!parent.contains(field) || !parent[field].is_array() || parent[field].empty()) {
        why = problem(where, std::string(field) + " 必须是非空数组");
        return false;
    }
    const json& list = parent[field];
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

// 这一条谓词成立时，旗标 flag 一定不为 0。
[[nodiscard]] bool requiresNonZero(const QuestCondition& c) {
    if (c.op == QuestCondition::Op::FlagAtLeast) return c.value >= 1;
    if (c.op == QuestCondition::Op::FlagEquals) return c.value >= 1;
    return false;
}

// until 里必须有「本章收尾即收起」那一条：chNN.done >= 1。
//
// 不写它，一条第 1 章「明日崖上怎么考」的打探会一直挂到第 5 章——青牛镇那几个人在
// 第 4、5 章仍然站在街上（地图对象不随章消失）。规则写成「必须写本章的 done」，
// 而不是「时段要合理」，是因为后者没有判据，前者一行就查得死。
[[nodiscard]] bool closesAtChapterEnd(const std::vector<QuestCondition>& until, int chapter) {
    const std::string doneFlag = chapterTag(chapter) + ".done";
    return std::any_of(until.begin(), until.end(), [&doneFlag](const QuestCondition& c) {
        return c.op == QuestCondition::Op::FlagAtLeast && c.subject == doneFlag && c.value == 1;
    });
}

// when 必须锚在本章：至少一条要求「本章某个旗标已置」或「上一章已收尾」。
//
// 同一个 npc 对象会跨章站在原地（韩家村、青牛镇、外刃堂那些人），没有锚的条目会在
// 本章开始之前就挂出来——第 4 章的情报在第 1 章就问得到。
[[nodiscard]] bool anchoredInChapter(const std::vector<QuestCondition>& when, int chapter) {
    const std::string own = chapterTag(chapter) + ".";
    const std::string previousDone = chapter > kFirstChapter ? chapterTag(chapter - 1) + ".done" : "";
    return std::any_of(when.begin(), when.end(), [&](const QuestCondition& c) {
        if (!requiresNonZero(c)) return false;
        if (c.subject.rfind(own, 0) == 0) return true;
        return !previousDone.empty() && c.subject == previousDone;
    });
}

// 一件物品：{item_id, count ≥ 1, herb_age ≥ 0（可缺省）}。与任务 rewards、编成 drops 同一套字段名。
[[nodiscard]] bool parseBagEntry(const json& node, const std::string& where, core::BagEntry& out,
                                 Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "物件必须是对象");
        return false;
    }
    std::string unknown;
    static const std::set<std::string> kBagFields = {"item_id", "count", "herb_age"};
    if (firstUnknownKey(node, kBagFields, {}, unknown)) {
        why = problem(where, "物件里有不认识的字段 " + unknown);
        return false;
    }
    if (!readRequiredString(node, "item_id", out.itemId)) {
        why = problem(where, "物件缺少 item_id");
        return false;
    }
    if (!node.contains("count") || !readInt(node["count"], out.count) || out.count < 1) {
        why = problem(where, "count 必须是 ≥ 1 的整数");
        return false;
    }
    out.herbAge = 0;
    if (node.contains("herb_age") && (!readInt(node["herb_age"], out.herbAge) || out.herbAge < 0)) {
        why = problem(where, "herb_age 必须是 ≥ 0 的整数");
        return false;
    }
    return true;
}

[[nodiscard]] bool parseBagList(const json& parent, const char* field, const std::string& where,
                                std::vector<core::BagEntry>& out, Problem& why) {
    if (!parent.contains(field)) return true;
    if (!parent[field].is_array()) {
        why = problem(where, std::string(field) + " 必须是数组");
        return false;
    }
    const json& list = parent[field];
    for (std::size_t i = 0; i < list.size(); ++i) {
        core::BagEntry entry;
        if (!parseBagEntry(list[i], where + " 的 " + field + "[" + std::to_string(i) + "]", entry,
                           why)) {
            return false;
        }
        out.push_back(std::move(entry));
    }
    return true;
}

[[nodiscard]] bool parseInquire(const json& node, const std::string& where, PathAction& out,
                                Problem& why) {
    if (node.contains("reveal")) {
        if (!node["reveal"].is_array()) {
            why = problem(where, "reveal 必须是数组");
            return false;
        }
        static const std::set<std::string> kRevealFields = {"role", "category"};
        for (std::size_t i = 0; i < node["reveal"].size(); ++i) {
            const json& item = node["reveal"][i];
            const std::string here = where + " 的 reveal[" + std::to_string(i) + "]";
            std::string unknown;
            if (!item.is_object() || firstUnknownKey(item, kRevealFields, {}, unknown)) {
                why = problem(here, "只认 {role, category}" +
                                        (unknown.empty() ? std::string() : "，多了 " + unknown));
                return false;
            }
            core::PathReveal reveal;
            if (!readRequiredString(item, "role", reveal.roleId) ||
                !readRequiredString(item, "category", reveal.category)) {
                why = problem(here, "role 与 category 都必须是非空字符串");
                return false;
            }
            if (!isWeaknessCategory(reveal.category)) {
                why = problem(here, "category " + reveal.category + " 不是十种攻击类别之一（" +
                                        core::categoryNames(core::kAllCategories) + "）");
                return false;
            }
            out.reveals.push_back(std::move(reveal));
        }
    }
    if (!parseBagList(node, "give", where, out.gives, why)) return false;
    if (node.contains("set_flags")) {
        if (!node["set_flags"].is_array()) {
            why = problem(where, "set_flags 必须是数组");
            return false;
        }
        // 只许置本文登记的路径行动旗标（chNN.path.*，本章的）：打探是添头，不许改写主线——
        // 一条打探若能置 ch04.kaizhan，玩家就能靠问一句话跳过开战。
        const std::string own = chapterTag(out.chapter) + ".path.";
        for (const json& flag : node["set_flags"]) {
            if (!flag.is_string() || flag.get<std::string>().rfind(own, 0) != 0 ||
                flag.get<std::string>().size() == own.size()) {
                why = problem(where, "set_flags 里只许写本章的路径行动旗标（" + own + "…）");
                return false;
            }
            const std::string name = flag.get<std::string>();
            if (name == out.doneFlag ||
                std::find(out.setFlags.begin(), out.setFlags.end(), name) != out.setFlags.end()) {
                why = problem(where, "set_flags 里的 " + name + " 与 done_flag 或前一项重复");
                return false;
            }
            out.setFlags.push_back(name);
        }
    }
    return true;
}

[[nodiscard]] bool parsePurchase(const json& node, const std::string& where, PathAction& out,
                                 Problem& why) {
    if (!readRequiredString(node, "item_id", out.goods.itemId)) {
        why = problem(where, "求购缺少 item_id");
        return false;
    }
    if (!node.contains("count") || !readInt(node["count"], out.goods.count) || out.goods.count < 1) {
        why = problem(where, "求购的 count 必须是 ≥ 1 的整数");
        return false;
    }
    out.goods.herbAge = 0;
    if (node.contains("herb_age") &&
        (!readInt(node["herb_age"], out.goods.herbAge) || out.goods.herbAge < 0)) {
        why = problem(where, "求购的 herb_age 必须是 ≥ 0 的整数");
        return false;
    }
    // 白送不叫求购：price 0 的条目是漏写，想白给就写成打探的 give。
    if (!node.contains("price") || !readInt(node["price"], out.price) || out.price < 1) {
        why = problem(where, "求购的 price 必须是 ≥ 1 的整数（碎银，按块计）");
        return false;
    }
    if (!readRequiredString(node, "deal_key", out.dealKey)) {
        why = problem(where, "求购缺少 deal_key（成交那一句）");
        return false;
    }
    if (!readRequiredString(node, "poor_key", out.poorKey)) {
        why = problem(where, "求购缺少 poor_key（钱不够那一句）");
        return false;
    }
    return true;
}

[[nodiscard]] bool parseChallenge(const json& node, const std::string& where, PathAction& out,
                                  Problem& why) {
    if (!readRequiredString(node, "battle", out.battleId)) {
        why = problem(where, "切磋缺少 battle（编成 id）");
        return false;
    }
    if (node.contains("pending")) {
        if (!node["pending"].is_boolean()) {
            why = problem(where, "pending 必须是 true / false");
            return false;
        }
        out.pending = node["pending"].get<bool>();
    }
    if (!readRequiredString(node, "win_key", out.winKey)) {
        why = problem(where, "切磋缺少 win_key");
        return false;
    }
    if (!readRequiredString(node, "lose_key", out.loseKey)) {
        why = problem(where, "切磋缺少 lose_key");
        return false;
    }
    if (!node.contains("reward") || !node["reward"].is_object()) {
        why = problem(where, "切磋缺少 reward 对象（赢了有奖励，施工图 5.1）");
        return false;
    }
    const json& reward = node["reward"];
    const std::string here = where + " 的 reward";
    std::string unknown;
    static const std::set<std::string> kRewardFields = {"cultivation", "items"};
    if (firstUnknownKey(reward, kRewardFields, {}, unknown)) {
        why = problem(here, "有不认识的字段 " + unknown + "（只认 cultivation / items）");
        return false;
    }
    if (reward.contains("cultivation") &&
        (!readInt(reward["cultivation"], out.rewardCultivation) || out.rewardCultivation < 0)) {
        why = problem(here, "cultivation 必须是 ≥ 0 的整数");
        return false;
    }
    if (!parseBagList(reward, "items", here, out.rewardItems, why)) return false;
    if (out.rewardCultivation == 0 && out.rewardItems.empty()) {
        why = problem(here, "修为与物件至少给一样：赢了什么也没有，就不叫「赢了有奖励」");
        return false;
    }
    return true;
}

[[nodiscard]] bool parseEntry(const json& node, const std::string& where, int chapter,
                              PathAction& out, Problem& why) {
    if (!node.is_object()) {
        why = problem(where, "条目必须是对象");
        return false;
    }
    out.chapter = chapter;
    if (!readRequiredString(node, "id", out.id)) {
        why = problem(where, "条目缺少 id");
        return false;
    }
    const std::string here = where + "（" + out.id + "）";
    std::string kind;
    if (!readRequiredString(node, "kind", kind) || !parseKind(kind, out.kind)) {
        why = problem(here, "kind 只认 inquire / purchase / challenge");
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node, commonFields(), kindFields(out.kind), unknown)) {
        why = problem(here, "有 " + kind + " 不认识的字段 " + unknown +
                                "（字段表见 docs/interfaces-octo-pathactions.md 第 2 节）");
        return false;
    }
    static const std::regex kIdShape("^[a-z0-9]+(?:_[a-z0-9]+)*_(dating|qiugou|qiecuo)[0-9]*$");
    std::smatch shape;
    if (!std::regex_match(out.id, shape, kIdShape) || shape[1].str() != kindWord(out.kind)) {
        why = problem(here, std::string("id 的形状是 <npc 短名>_") + kindWord(out.kind) +
                                "[序号]，小写字母、数字、下划线");
        return false;
    }
    if (!readRequiredString(node, "map", out.mapId) || !readRequiredString(node, "npc", out.npc)) {
        why = problem(here, "map 与 npc 都必须是非空字符串");
        return false;
    }
    if (!parseConditionList(node, "when", here, out.when, why)) return false;
    if (!parseConditionList(node, "until", here, out.until, why)) return false;
    if (!anchoredInChapter(out.when, chapter)) {
        why = problem(here, "when 没有锚在本章：至少要有一条「" + chapterTag(chapter) +
                                ".* 已置」或「上一章 done」，否则本章开始之前就问得到");
        return false;
    }
    if (!closesAtChapterEnd(out.until, chapter)) {
        why = problem(here, "until 里必须有 {\"flag\": \"" + chapterTag(chapter) +
                                ".done\", \"op\": \">=\", \"value\": 1}：本章收尾时一律收起");
        return false;
    }

    int realm = 0;
    if (node.contains("realm")) {
        if (!readInt(node["realm"], realm) || !rules::isValid(rules::fromValue(realm))) {
            why = problem(here, "realm 必须是合法的境界编号（与存档同一个数：凡人 0、炼气 N 层即 N、"
                                "筑基 21–23、结丹 31–33）");
            return false;
        }
    }
    out.minRealm = rules::fromValue(realm);
    const bool gated = out.minRealm != rules::Realm::Mortal;
    if (!readOptionalString(node, "refuse_key", out.refuseKey)) {
        why = problem(here, "refuse_key 必须是字符串");
        return false;
    }
    // 成对：有门槛就得有回绝那一句（否则阅历不足时对方一声不吭）；没门槛却写了回绝，
    // 是一句永远说不出口的话，多半是 realm 漏写了。
    if (gated && out.refuseKey.empty()) {
        why = problem(here, "有阅历门槛（realm > 0）就必须写 refuse_key");
        return false;
    }
    if (!gated && node.contains("refuse_key")) {
        why = problem(here, "没有阅历门槛却写了 refuse_key：这句话永远说不出口");
        return false;
    }

    const std::string expectedDone = chapterTag(chapter) + ".path." + out.id;
    if (!readRequiredString(node, "done_flag", out.doneFlag) || out.doneFlag != expectedDone) {
        why = problem(here, "done_flag 必须是 " + expectedDone);
        return false;
    }
    if (!readRequiredString(node, "text_key", out.textKey)) {
        why = problem(here, "缺少 text_key");
        return false;
    }
    if (!readOptionalString(node, "origin", out.origin) || !readOptionalString(node, "note", out.note)) {
        why = problem(here, "origin / note 必须是字符串");
        return false;
    }

    switch (out.kind) {
        case PathActionKind::Inquire: return parseInquire(node, here, out, why);
        case PathActionKind::Purchase: return parsePurchase(node, here, out, why);
        case PathActionKind::Challenge: return parseChallenge(node, here, out, why);
    }
    return false;
}

[[nodiscard]] bool parseFile(const json& j, const std::string& path, std::vector<PathAction>& out,
                             Problem& why) {
    if (!j.is_object()) {
        why = problem(path, "文件顶层必须是对象");
        return false;
    }
    std::string unknown;
    static const std::set<std::string> kTopFields = {"id", "name", "chapter", "entries", "note"};
    if (firstUnknownKey(j, kTopFields, {}, unknown)) {
        why = problem(path, "有不认识的字段 " + unknown + "（只认 id / name / chapter / entries / note）");
        return false;
    }
    int chapter = 0;
    if (!j.contains("chapter") || !readInt(j["chapter"], chapter) || chapter < kFirstChapter ||
        chapter > kLastChapter) {
        why = problem(path, "chapter 必须是 1–14 的整数");
        return false;
    }
    // 一章一个文件，文件名就是章号：chNN.json，顶层 id 是 pathactions_chNN。
    // 名字钉死是为了让「第 4 章的路径行动在哪」只有一个答案。
    const std::string tag = chapterTag(chapter);
    if (fs::path(path).stem().string() != tag) {
        why = problem(path, "第 " + std::to_string(chapter) + " 章的文件名必须是 " + tag + ".json");
        return false;
    }
    std::string id;
    if (!readRequiredString(j, "id", id) || id != "pathactions_" + tag) {
        why = problem(path, "顶层 id 必须是 pathactions_" + tag);
        return false;
    }
    std::string name;
    if (!readRequiredString(j, "name", name)) {
        why = problem(path, "缺少 name");
        return false;
    }
    std::string note;
    if (!readOptionalString(j, "note", note)) {
        why = problem(path, "note 必须是字符串");
        return false;
    }
    if (!j.contains("entries") || !j["entries"].is_array() || j["entries"].empty()) {
        why = problem(path, "entries 必须是非空数组");
        return false;
    }
    std::set<std::string> ids;
    for (std::size_t i = 0; i < j["entries"].size(); ++i) {
        PathAction action;
        if (!parseEntry(j["entries"][i], path + " 的 entries[" + std::to_string(i) + "]", chapter,
                        action, why)) {
            return false;
        }
        if (!ids.insert(action.id).second) {
            why = problem(path, "条目 id 在本章内重复：" + action.id);
            return false;
        }
        out.push_back(std::move(action));
    }
    return true;
}

std::vector<fs::path> listChapterFiles(const fs::path& dir) {
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
    // 排序不是装饰：报错要写明「哪两个文件」，跟着文件系统的返回次序走，
    // 同一份数据在两台机器上会报出两句不同的话。
    std::sort(files.begin(), files.end());
    return files;
}

// 一个目录里的全部章节：条目按章号排好，外加「第几章出自哪个文件」（查引用时报错要写明出处）。
struct ChapterFiles {
    std::vector<PathAction> all;
    std::map<int, std::string> fileOfChapter;
};

[[nodiscard]] core::Result<ChapterFiles> readChapterFiles(const fs::path& dir) {
    using Out = core::Result<ChapterFiles>;
    ChapterFiles out;
    for (const fs::path& file : listChapterFiles(dir)) {
        core::Result<std::vector<PathAction>> one = loadPathActionFile(file.string());
        if (!one) return Out::failure(one.error);
        // 一章一个文件：两个文件同属一章时，「这一章的路径行动」就说不清是哪一份，
        // 两份里各挂一条同一个人的打探，重叠检查也只在各自文件里看得见一半。
        const int chapter = one.value.front().chapter;
        const auto seen = out.fileOfChapter.find(chapter);
        if (seen != out.fileOfChapter.end()) {
            return Out::failure("两个路径行动文件同属第 " + std::to_string(chapter) + " 章：" +
                                seen->second + " 与 " + file.string());
        }
        out.fileOfChapter.emplace(chapter, file.string());
        for (PathAction& action : one.value) out.all.push_back(std::move(action));
    }
    std::stable_sort(out.all.begin(), out.all.end(),
                     [](const PathAction& a, const PathAction& b) { return a.chapter < b.chapter; });
    return Out::success(std::move(out));
}

// ---- 引用 ----

[[nodiscard]] core::Result<std::set<std::string>> registeredFlags(const fs::path& flagsFile) {
    using Out = core::Result<std::set<std::string>>;
    core::Result<json> parsed = detail::readJsonFile(flagsFile);
    if (!parsed) return Out::failure("旗标登记表读不到：" + parsed.error);
    if (!parsed.value.is_object()) return Out::failure("旗标登记表必须是对象：" + flagsFile.string());
    std::set<std::string> names;
    for (auto it = parsed.value.begin(); it != parsed.value.end(); ++it) names.insert(it.key());
    return Out::success(std::move(names));
}

struct References {
    const core::GameData& data;
    std::set<std::string> flags;
    std::map<std::string, std::set<std::string>> npcsOnMap;   // 只放读过的图
    std::map<std::string, core::BattleSetup> battles;
    fs::path mapsDir;
};

// 挂的那张图上有没有这个 npc 对象。一张图只读一次。
[[nodiscard]] bool npcExists(References& refs, const std::string& mapId, const std::string& npc,
                             std::string& error) {
    auto found = refs.npcsOnMap.find(mapId);
    if (found == refs.npcsOnMap.end()) {
        const fs::path tmj = refs.mapsDir / (mapId + ".tmj");
        std::error_code ec;
        if (!fs::exists(tmj, ec)) {
            error = "地图不存在：" + tmj.string();
            return false;
        }
        core::Result<core::TileMap> map = loadTileMap(tmj.string());
        if (!map) {
            error = map.error;
            return false;
        }
        std::set<std::string> npcs;
        for (const core::MapObject& object : map.value.objects) {
            if (object.type == "npc") npcs.insert(object.name);
        }
        found = refs.npcsOnMap.emplace(mapId, std::move(npcs)).first;
    }
    if (found->second.count(npc) == 0) {
        error = mapId + " 上没有名为 " + npc + " 的 npc 对象";
        return false;
    }
    return true;
}

[[nodiscard]] bool checkReferences(const PathAction& action, References& refs,
                                   const std::set<std::string>& doneFlags, const std::string& where,
                                   Problem& why) {
    const core::GameData& data = refs.data;
    for (const std::string* key : {&action.textKey, &action.refuseKey, &action.dealKey,
                                   &action.poorKey, &action.winKey, &action.loseKey}) {
        if (!key->empty() && data.text.count(*key) == 0) {
            why = problem(where, "文案 key 不存在：" + *key);
            return false;
        }
    }

    std::vector<std::string> flagsUsed = {action.doneFlag};
    std::vector<std::string> itemsUsed;
    for (const std::vector<QuestCondition>* list : {&action.when, &action.until}) {
        for (const QuestCondition& c : *list) {
            (c.op == QuestCondition::Op::ItemAtLeast ? itemsUsed : flagsUsed).push_back(c.subject);
        }
    }
    flagsUsed.insert(flagsUsed.end(), action.setFlags.begin(), action.setFlags.end());
    for (const std::string& flag : flagsUsed) {
        if (refs.flags.count(flag) == 0) {
            why = problem(where, "旗标未在 data/flags.json 登记：" + flag);
            return false;
        }
    }
    // 打探置的旗标不许是别的条目的 done_flag：问一句话就把另一条标成「买过」「赢过」，
    // 那一条就再也挂不出来了。
    for (const std::string& flag : action.setFlags) {
        if (doneFlags.count(flag) != 0) {
            why = problem(where, "set_flags 里的 " + flag + " 是别的条目的 done_flag");
            return false;
        }
    }

    for (const core::BagEntry& item : action.gives) itemsUsed.push_back(item.itemId);
    for (const core::BagEntry& item : action.rewardItems) itemsUsed.push_back(item.itemId);
    if (!action.goods.itemId.empty()) itemsUsed.push_back(action.goods.itemId);
    for (const std::string& item : itemsUsed) {
        if (data.items.count(item) == 0) {
            why = problem(where, "物品 id 不存在：" + item);
            return false;
        }
    }
    for (const core::PathReveal& reveal : action.reveals) {
        if (data.roles.count(reveal.roleId) == 0) {
            why = problem(where, "reveal 的角色不存在：" + reveal.roleId);
            return false;
        }
    }

    std::string error;
    if (!npcExists(refs, action.mapId, action.npc, error)) {
        why = problem(where, error);
        return false;
    }

    if (action.kind == PathActionKind::Challenge) {
        const auto battle = refs.battles.find(action.battleId);
        if (action.pending) {
            // pending 的意思是「编成还没建」。建好了还挂着 pending，这条切磋就一直不上屏，
            // 而那一点在画面上看不出来——所以反过来也要拦。
            if (battle != refs.battles.end()) {
                why = problem(where, "编成 " + action.battleId + " 已经建好了，去掉 pending");
                return false;
            }
            return true;
        }
        if (battle == refs.battles.end()) {
            why = problem(where, "编成不存在：" + action.battleId +
                                     "（还没建好就写 \"pending\": true）");
            return false;
        }
        const core::BattleSetup& setup = battle->second;
        if (setup.defeatIsFatal) {
            why = problem(where, "切磋的编成 " + action.battleId + " 输了会 game over：defeat_is_fatal 必须为 false");
            return false;
        }
        // 奖励只从条目发一次（契约第 4 节）。编成若也发，赢一场就拿两份。
        if (setup.reward.cultivation != 0 || setup.reward.spiritStones != 0 ||
            !setup.reward.drops.empty()) {
            why = problem(where, "切磋的编成 " + action.battleId + " 自己带了 rewards：奖励只由条目发，编成的必须全 0");
            return false;
        }
    }
    return true;
}

}  // namespace

core::Result<std::vector<PathAction>> loadPathActionFile(const std::string& path) {
    using Out = core::Result<std::vector<PathAction>>;
    core::Result<json> parsed = detail::readJsonFile(fs::path(path));
    if (!parsed) return Out::failure(parsed.error);
    std::vector<PathAction> actions;
    Problem why;
    if (!parseFile(parsed.value, path, actions, why)) return Out::failure(why.message);
    return Out::success(std::move(actions));
}

core::Result<std::vector<PathAction>> loadPathActionDir(const std::string& pathActionsDir) {
    using Out = core::Result<std::vector<PathAction>>;
    core::Result<ChapterFiles> read = readChapterFiles(fs::path(pathActionsDir));
    if (!read) return Out::failure(read.error);
    return Out::success(std::move(read.value.all));
}

core::Result<std::vector<PathAction>> loadPathActions(const std::string& dataRoot,
                                                      const core::GameData& data) {
    using Out = core::Result<std::vector<PathAction>>;
    const fs::path root(dataRoot);
    core::Result<ChapterFiles> read = readChapterFiles(root / "pathactions");
    if (!read) return Out::failure(read.error);
    std::vector<PathAction>& all = read.value.all;
    std::map<int, std::string>& fileOfChapter = read.value.fileOfChapter;
    if (all.empty()) return Out::success(std::move(all));

    core::Result<std::set<std::string>> flags = registeredFlags(root / "flags.json");
    if (!flags) return Out::failure(flags.error);
    // 「data 的上一级」按字面算（"x/data/" 带尾斜杠时 parent_path 只会剥掉那个斜杠）。
    References refs{data, std::move(flags.value), {}, {}, (root / "..").lexically_normal() / "maps"};
    const bool anyChallenge = std::any_of(all.begin(), all.end(), [](const PathAction& a) {
        return a.kind == PathActionKind::Challenge;
    });
    if (anyChallenge) {
        auto battles = loadBattles((root / "battles").string());
        if (!battles) return Out::failure(battles.error);
        refs.battles = std::move(battles.value);
    }

    std::set<std::string> doneFlags;
    for (const PathAction& action : all) doneFlags.insert(action.doneFlag);
    for (const PathAction& action : all) {
        const std::string where = fileOfChapter[action.chapter] + " 的 " + action.id;
        Problem why;
        if (!checkReferences(action, refs, doneFlags, where, why)) return Out::failure(why.message);
    }
    return Out::success(std::move(all));
}

}  // namespace fanren::io
