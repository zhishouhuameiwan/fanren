#include "io/CompanionLoader.h"

#include <algorithm>
#include <filesystem>
#include <initializer_list>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/rules/Tongdao.h"
#include "io/JsonUtil.h"

namespace fanren::io {
namespace {

namespace fs = std::filesystem;
using nlohmann::json;

// 表外字段一律报错：拼错一个 until_flag 会让她一辈子停在第一个角色上，而那不会自己暴露。
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

// 可选的字符串：没写保持空；写了就得是字符串。
[[nodiscard]] bool readOptionalString(const json& object, const char* key, std::string& out) {
    if (!object.contains(key)) return true;
    if (!object[key].is_string()) return false;
    out = object[key].get<std::string>();
    return true;
}

[[nodiscard]] bool readStringList(const json& node, std::vector<std::string>& out) {
    if (!node.is_array()) return false;
    for (const json& item : node) {
        if (!item.is_string() || item.get_ref<const std::string&>().empty()) return false;
        out.push_back(item.get<std::string>());
    }
    return true;
}

// 一个整数字段：没写取 fallback；写了就得是 [lo, hi] 里的整数。
[[nodiscard]] bool readIntIn(const json& object, const char* key, int lo, int hi, int fallback, int& out) {
    if (!object.contains(key)) {
        out = fallback;
        return true;
    }
    if (!object[key].is_number_integer()) return false;
    const long long value = object[key].get<long long>();
    if (value < lo || value > hi) return false;
    out = static_cast<int>(value);
    return true;
}

// 五行的数据名（与路径行动 reveals 的 category 同一套英文名）→ Element 位。
[[nodiscard]] int elementFromName(const std::string& name) {
    if (name == "metal") return core::kElementMetal;
    if (name == "wood") return core::kElementWood;
    if (name == "water") return core::kElementWater;
    if (name == "fire") return core::kElementFire;
    if (name == "earth") return core::kElementEarth;
    return 0;
}

// 被动 / 在册加成（同道录 2a）。slot 是 rules::kPerkSlotPassive 或 kPerkSlotRoster：
//   伙伴被动  {"kind": "...", "values": [三档起, 五档起]}（values 一项或两项）
//   在册加成  {"kind": "...", "value": N}
// 只认 rules::perkTable 里的种类；放错格子、数不在范围里、缺 / 多 element 都报错。
[[nodiscard]] bool readPerk(const json& node, const std::string& at, int slot, core::CompanionPerk& out,
                            std::string& why) {
    const bool passive = slot == rules::kPerkSlotPassive;
    std::string unknown;
    if (!node.is_object()) {
        why = at + "：应当是对象";
        return false;
    }
    if (passive ? firstUnknownKey(node, {"kind", "values", "element"}, unknown)
                : firstUnknownKey(node, {"kind", "value", "element"}, unknown)) {
        why = at + "：有不认识的字段 " + unknown;
        return false;
    }
    if (!node.contains("kind") || !node["kind"].is_string()) {
        why = at + " 的 kind：应当是字符串";
        return false;
    }
    const std::string name = node["kind"].get<std::string>();
    const rules::PerkInfo* info = rules::perkByName(name);
    if (info == nullptr) {
        why = at + " 的 kind：不认识的种类 " + name + "（只认 rules::perkTable 里的）";
        return false;
    }
    if ((info->slots & slot) == 0) {
        why = at + " 的 kind：" + name + (passive ? " 不能当伙伴被动" : " 不能当在册加成");
        return false;
    }
    out.kind = info->kind;
    const std::string range = "[" + std::to_string(info->minValue) + ", " + std::to_string(info->maxValue) + "]";
    if (passive) {
        const json* values = node.contains("values") ? &node["values"] : nullptr;
        if (values == nullptr || !values->is_array() || values->empty() || values->size() > 2) {
            why = at + " 的 values：应当是一项或两项的整数数组（三档起、五档起）";
            return false;
        }
        std::vector<int> read;
        for (const json& v : *values) {
            if (!v.is_number_integer() || v.get<long long>() < info->minValue || v.get<long long>() > info->maxValue) {
                why = at + " 的 values：每一项应当是 " + range + " 里的整数";
                return false;
            }
            read.push_back(v.get<int>());
        }
        out.value = read.front();
        out.value5 = read.back();
        if (out.value5 < out.value) {
            why = at + " 的 values：五档那一项不能比三档小（五档是加强一级）";
            return false;
        }
    } else if (!node.contains("value") || !readIntIn(node, "value", info->minValue, info->maxValue, 0, out.value)) {
        why = at + " 的 value：应当是 " + range + " 里的整数";
        return false;
    } else {
        out.value5 = out.value;
    }
    if (info->needsElement) {
        std::string element;
        if (!readOptionalString(node, "element", element) || (out.element = elementFromName(element)) == 0) {
            why = at + " 的 element：" + name + " 要写 metal / wood / water / fire / earth 之一";
            return false;
        }
    } else if (node.contains("element")) {
        why = at + "：" + name + " 不收 element";
        return false;
    }
    return true;
}

// roles[i].magics：[{"tier": 3, "magic": "..."}]，tier 2–5（一档就会的写在角色表的 magics 里）。
[[nodiscard]] bool readTierMagics(const json& node, const std::string& at, std::vector<core::TierMagic>& out,
                                  std::string& why) {
    if (!node.is_array()) {
        why = at + "：应当是数组";
        return false;
    }
    for (std::size_t i = 0; i < node.size(); ++i) {
        const json& item = node[i];
        const std::string here = at + "[" + std::to_string(i) + "]";
        std::string unknown;
        if (!item.is_object() || firstUnknownKey(item, {"tier", "magic"}, unknown)) {
            why = here + "：应当是只有 tier / magic 的对象";
            return false;
        }
        core::TierMagic magic;
        if (!item.contains("tier") || !readIntIn(item, "tier", 2, 5, 0, magic.tier)) {
            why = here + " 的 tier：应当是 2–5 的整数（一档就会的写在角色表的 magics 里）";
            return false;
        }
        if (!item.contains("magic") || !item["magic"].is_string() || item["magic"].get<std::string>().empty()) {
            why = here + " 的 magic：应当是非空字符串";
            return false;
        }
        magic.magicId = item["magic"].get<std::string>();
        out.push_back(std::move(magic));
    }
    return true;
}

// 物品清单：[{"item": "...", "count": 1}]，count 1–99，缺省 1。
[[nodiscard]] bool readItemList(const json& node, const std::string& at, std::vector<core::BagEntry>& out,
                                std::string& why) {
    if (!node.is_array() || node.empty()) {
        why = at + "：应当是非空数组";
        return false;
    }
    for (std::size_t i = 0; i < node.size(); ++i) {
        const json& item = node[i];
        const std::string here = at + "[" + std::to_string(i) + "]";
        std::string unknown;
        if (!item.is_object() || firstUnknownKey(item, {"item", "count"}, unknown)) {
            why = here + "：应当是只有 item / count 的对象";
            return false;
        }
        core::BagEntry entry;
        if (!item.contains("item") || !item["item"].is_string() || item["item"].get<std::string>().empty()) {
            why = here + " 的 item：应当是非空字符串";
            return false;
        }
        entry.itemId = item["item"].get<std::string>();
        if (!readIntIn(item, "count", 1, 99, 1, entry.count)) {
            why = here + " 的 count：应当是 1–99 的整数";
            return false;
        }
        out.push_back(std::move(entry));
    }
    return true;
}

// spar：{"battle": "spar_...", "prizes": [{"tier": 2, "items": [...]}]}。prizes 可不写；tier 2–5、严格升序。
[[nodiscard]] bool readSpar(const json& object, const std::string& where, core::Companion& out, std::string& why) {
    if (!object.contains("spar")) return true;
    const json& spar = object["spar"];
    const std::string at = where + " 的 spar";
    std::string unknown;
    if (!spar.is_object() || firstUnknownKey(spar, {"battle", "prizes"}, unknown)) {
        why = at + "：应当是只有 battle / prizes 的对象";
        return false;
    }
    if (!spar.contains("battle") || !spar["battle"].is_string() || spar["battle"].get<std::string>().empty()) {
        why = at + " 的 battle：应当是非空字符串（spar: true 的编成 id）";
        return false;
    }
    out.sparBattle = spar["battle"].get<std::string>();
    if (!spar.contains("prizes")) return true;
    const json& prizes = spar["prizes"];
    if (!prizes.is_array()) {
        why = at + " 的 prizes：应当是数组";
        return false;
    }
    int last = 1;
    for (std::size_t i = 0; i < prizes.size(); ++i) {
        const json& item = prizes[i];
        const std::string here = at + " 的 prizes[" + std::to_string(i) + "]";
        if (!item.is_object() || firstUnknownKey(item, {"tier", "items"}, unknown)) {
            why = here + "：应当是只有 tier / items 的对象";
            return false;
        }
        core::SparPrize prize;
        if (!item.contains("tier") || !readIntIn(item, "tier", 2, 5, 0, prize.tier) || prize.tier <= last) {
            why = here + " 的 tier：应当是 2–5 的整数，按升序、每档至多一条";
            return false;
        }
        last = prize.tier;
        if (!item.contains("items")) {
            why = here + "：缺少 items";
            return false;
        }
        if (!readItemList(item["items"], here + " 的 items", prize.items, why)) return false;
        out.sparPrizes.push_back(std::move(prize));
    }
    return true;
}

[[nodiscard]] bool readRoles(const json& node, const std::string& where, std::vector<core::CompanionRole>& out,
                             std::string& why) {
    if (!node.is_array() || node.empty()) {
        why = where + " 的 roles：应当是非空数组";
        return false;
    }
    for (std::size_t i = 0; i < node.size(); ++i) {
        const json& item = node[i];
        const std::string at = where + " 的 roles[" + std::to_string(i) + "]";
        std::string unknown;
        if (!item.is_object()) {
            why = at + "：应当是对象";
            return false;
        }
        if (firstUnknownKey(item, {"role", "until_flag", "magics", "passive", "duo"}, unknown)) {
            why = at + "：有不认识的字段 " + unknown + "（只认 role / until_flag / magics / passive / duo）";
            return false;
        }
        core::CompanionRole role;
        if (!item.contains("role") || !item["role"].is_string() || item["role"].get<std::string>().empty()) {
            why = at + " 的 role：应当是非空字符串";
            return false;
        }
        role.roleId = item["role"].get<std::string>();
        if (!readOptionalString(item, "until_flag", role.untilFlag)) {
            why = at + " 的 until_flag：应当是字符串";
            return false;
        }
        // ---- 同道录 2a：按档解锁的法术、三档被动、这一段的合招（只写给第 7 章的角色）----
        if (item.contains("magics") && !readTierMagics(item["magics"], at + " 的 magics", role.tierMagics, why)) {
            return false;
        }
        if (item.contains("passive") &&
            !readPerk(item["passive"], at + " 的 passive", rules::kPerkSlotPassive, role.passive, why)) {
            return false;
        }
        if (!readOptionalString(item, "duo", role.duo)) {
            why = at + " 的 duo：应当是字符串";
            return false;
        }
        out.push_back(std::move(role));
    }
    return true;
}

[[nodiscard]] bool readGifts(const json& object, const std::string& where, core::Companion& out, std::string& why) {
    if (!object.contains("gifts")) return true;
    const json& gifts = object["gifts"];
    std::string unknown;
    if (!gifts.is_object() || firstUnknownKey(gifts, {"like", "dislike"}, unknown)) {
        why = where + " 的 gifts：应当是只有 like / dislike 的对象";
        return false;
    }
    if ((gifts.contains("like") && !readStringList(gifts["like"], out.giftsLike)) ||
        (gifts.contains("dislike") && !readStringList(gifts["dislike"], out.giftsDislike))) {
        why = where + " 的 gifts：like / dislike 应当是物品 id 的数组";
        return false;
    }
    return true;
}

// 同道录 2a 的顶层字段：kind、line、bond_cap、roster_bonus、spar。都可不写，缺省是改版前的行为。
[[nodiscard]] bool readTongdao(const json& node, const std::string& where, core::Companion& out, std::string& why) {
    std::string text;
    if (!readOptionalString(node, "kind", text)) {
        why = where + " 的 kind：应当是字符串";
        return false;
    }
    if (text == "beast") {
        out.kind = core::CompanionKind::Beast;
        out.line = core::BondLine::Beast;   // 灵兽缺省走灵兽线
    } else if (!text.empty() && text != "person") {
        why = where + " 的 kind：只认 person / beast";
        return false;
    }
    text.clear();
    if (!readOptionalString(node, "line", text) || (!text.empty() && !rules::bondLineFromName(text, out.line))) {
        why = where + " 的 line：只认 general / hongyan / moni / beast";
        return false;
    }
    if ((out.kind == core::CompanionKind::Beast) != (out.line == core::BondLine::Beast)) {
        why = where + " 的 line：灵兽走 beast 线，人物不走";
        return false;
    }
    if (node.contains("bond_cap")) {
        const json& cap = node["bond_cap"];
        std::string unknown;
        if (!cap.is_object() || firstUnknownKey(cap, {"until_flag", "max_tier"}, unknown) ||
            !readOptionalString(cap, "until_flag", out.capFlag) || out.capFlag.empty() || !cap.contains("max_tier") ||
            !readIntIn(cap, "max_tier", 1, 4, 0, out.capTier)) {
            why = where + " 的 bond_cap：应当是 {until_flag: 非空旗标, max_tier: 1–4}";
            return false;
        }
    }
    if (node.contains("roster_bonus") &&
        !readPerk(node["roster_bonus"], where + " 的 roster_bonus", rules::kPerkSlotRoster, out.rosterBonus, why)) {
        return false;
    }
    if (!readSpar(node, where, out, why)) return false;
    if (out.kind == core::CompanionKind::Beast && !out.sparBattle.empty()) {
        why = where + " 的 spar：灵兽不切磋";
        return false;
    }
    return true;
}

[[nodiscard]] bool parseCompanion(const json& node, const std::string& where, core::Companion& out,
                                  std::string& why) {
    if (!node.is_object()) {
        why = where + "：顶层应当是对象";
        return false;
    }
    std::string unknown;
    if (firstUnknownKey(node,
                        {"id", "roles", "met_flag", "field_skill", "chase", "duo", "gifts", "note",
                         // 同道录 2a（docs/ch07-tongdaolu-k.md 第 1 节）
                         "kind", "line", "bond_cap", "roster_bonus", "spar"},
                        unknown)) {
        why = where + "：有不认识的字段 " + unknown;
        return false;
    }
    for (const char* key : {"id", "met_flag"}) {
        if (!node.contains(key) || !node[key].is_string() || node[key].get<std::string>().empty()) {
            why = where + " 的 " + key + "：应当是非空字符串";
            return false;
        }
    }
    out.id = node["id"].get<std::string>();
    out.metFlag = node["met_flag"].get<std::string>();
    if (!node.contains("roles")) {
        why = where + "：缺少 roles";
        return false;
    }
    if (!readRoles(node["roles"], where, out.roles, why)) return false;
    std::string note;
    if (!readOptionalString(node, "field_skill", out.fieldSkill) || !readOptionalString(node, "duo", out.duo) ||
        !readOptionalString(node, "note", note)) {
        why = where + "：field_skill / duo / note 应当是字符串";
        return false;
    }
    if (node.contains("chase")) {
        if (!node["chase"].is_boolean()) {
            why = where + " 的 chase：应当是 true / false";
            return false;
        }
        out.chase = node["chase"].get<bool>();
    }
    if (!readGifts(node, where, out, why)) return false;
    return readTongdao(node, where, out, why);
}

// 目录不存在 = 一位伙伴也没有；存在却不是目录、或遍历到一半出错，判失败——只读进一半的伙伴不报错，
// 少掉的那几位要到她该上场时才发现。
core::Result<std::vector<fs::path>> listCompanionFiles(const fs::path& dir) {
    using Out = core::Result<std::vector<fs::path>>;
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::exists(dir, ec)) {
        if (ec) return Out::failure("读不了伙伴目录 " + dir.string() + "：" + ec.message());
        return Out::success(std::move(files));
    }
    if (!fs::is_directory(dir, ec)) return Out::failure("伙伴目录 " + dir.string() + " 不是目录");
    for (auto it = fs::recursive_directory_iterator(dir, ec); !ec && it != fs::recursive_directory_iterator();
         it.increment(ec)) {
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") files.push_back(it->path());
        if (typeEc) return Out::failure("读不了 " + it->path().string() + "：" + typeEc.message());
    }
    if (ec) return Out::failure("遍历伙伴目录 " + dir.string() + " 出错：" + ec.message());
    // 排序：重复的报错要写明「哪两个文件」，不能跟着文件系统的次序在两台机器上报出两句话。
    std::sort(files.begin(), files.end());
    return Out::success(std::move(files));
}

}  // namespace

core::Result<core::Companion> loadCompanionFile(const std::string& path) {
    using Out = core::Result<core::Companion>;
    core::Result<json> parsed = detail::readJsonFile(fs::path(path));
    if (!parsed) return Out::failure(parsed.error);
    core::Companion companion;
    std::string why;
    if (!parseCompanion(parsed.value, path, companion, why)) return Out::failure(why);
    return Out::success(std::move(companion));
}

core::Result<std::map<std::string, core::Companion>> loadCompanions(const std::string& companionsDir) {
    using Out = core::Result<std::map<std::string, core::Companion>>;
    std::map<std::string, core::Companion> companions;
    std::map<std::string, std::string> fileOfId;
    std::map<std::string, std::string> fileOfRole;
    core::Result<std::vector<fs::path>> listed = listCompanionFiles(fs::path(companionsDir));
    if (!listed) return Out::failure(listed.error);
    for (const fs::path& file : listed.value) {
        core::Result<core::Companion> one = loadCompanionFile(file.string());
        if (!one) return Out::failure(one.error);
        if (const auto seen = fileOfId.find(one.value.id); seen != fileOfId.end()) {
            return Out::failure("伙伴 id 重复：" + one.value.id + "，见 " + seen->second + " 与 " + file.string());
        }
        fileOfId.emplace(one.value.id, file.string());
        for (const core::CompanionRole& role : one.value.roles) {
            const auto [seen, fresh] = fileOfRole.emplace(role.roleId, file.string());
            // 同一位伙伴里写两遍也算：先后两段不会是同一个角色表。
            if (!fresh) {
                return Out::failure("角色 " + role.roleId + " 挂在两处伙伴名下，见 " + seen->second + " 与 " +
                                    file.string());
            }
        }
        companions.emplace(one.value.id, std::move(one.value));
    }
    return Out::success(std::move(companions));
}

}  // namespace fanren::io
