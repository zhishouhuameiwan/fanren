// loadGameData 的实现。loadTileMap 在 TileMapLoader.cpp 里：两者都声明在
// DataLoader.h，但地图解析逻辑自成一块，拆开放更符合「文件按内聚而非按头文件」
// 的组织原则。
#include "io/DataLoader.h"

#include <algorithm>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/rules/Bottle.h"   // herbMaxAgeForGrade / kMaxHerbAge：灵草年份上限的唯一出处
#include "core/rules/Realm.h"
#include "io/JsonUtil.h"
#include "io/PathActionLoader.h"
#include "io/QuestLoader.h"

namespace fanren::io {

namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using fanren::core::GameData;
using fanren::core::Item;
using fanren::core::ItemKind;
using fanren::core::Magic;
using fanren::core::RoleTemplate;
using fanren::core::categoryFromName;
using fanren::core::categoryNames;
using fanren::core::kAllCategories;
using fanren::core::kCategoryNone;
using fanren::core::kWeaponCategories;
using fanren::rules::Realm;

// 递归收集一个目录下的全部 .json 文件，按路径排序。
//
// 排序不是装饰：重复 id 报错要写明「哪两个文件冲突」，遍历顺序一旦跟着文件系统
// 的原始返回顺序走，报错信息在不同机器/不同次运行会变来变去，排序后是确定的。
// 目录不存在时返回空列表——对 loadGameData 来说，这只表示该分类 0 条记录，
// 不是错误（调用方可能只提供 items/ 而没有 magics/）。
std::vector<fs::path> listJsonFilesSorted(const fs::path& dir) {
    std::vector<fs::path> files;
    std::error_code existsEc;
    if (!fs::exists(dir, existsEc) || !fs::is_directory(dir, existsEc)) return files;

    std::error_code iterEc;
    for (auto it = fs::recursive_directory_iterator(
             dir, fs::directory_options::skip_permission_denied, iterEc);
         it != fs::recursive_directory_iterator(); it.increment(iterEc)) {
        if (iterEc) break;
        std::error_code typeEc;
        if (it->is_regular_file(typeEc) && it->path().extension() == ".json") {
            files.push_back(it->path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

bool parseItemKind(const std::string& s, ItemKind& out) {
    static const std::map<std::string, ItemKind> kTable = {
        {"story", ItemKind::Story},     {"artifact", ItemKind::Artifact},
        {"armor", ItemKind::Armor},     {"trinket", ItemKind::Trinket},
        {"pill", ItemKind::Pill},       {"herb", ItemKind::Herb},
        {"talisman", ItemKind::Talisman}, {"manual", ItemKind::Manual},
        {"material", ItemKind::Material},
    };
    const auto it = kTable.find(s);
    if (it == kTable.end()) return false;
    out = it->second;
    return true;
}

}  // namespace

// Realm 的字符串名与 C++ 枚举标识符一一对应（如 "FoundationEarly"），不用中文：
// 中文只出现在文案 JSON 里（map_spec.md 第 6 节的精神同样适用于 data/ 下的标识
// 符），且这样与 src/core/rules/Realm.h 的定义肉眼可对照，不需要额外的映射表
// 文档来维护两边同步。（遭遇表的加载器也读境界名，所以这一份从匿名空间里挪出来、
// 在 DataLoader.h 公开：名字与枚举的对照只许有一份。）
bool parseRealmName(const std::string& s, Realm& out) {
    static const std::map<std::string, Realm> kTable = {
        {"Mortal", Realm::Mortal},
        {"QiRefining1", Realm::QiRefining1}, {"QiRefining2", Realm::QiRefining2},
        {"QiRefining3", Realm::QiRefining3}, {"QiRefining4", Realm::QiRefining4},
        {"QiRefining5", Realm::QiRefining5}, {"QiRefining6", Realm::QiRefining6},
        {"QiRefining7", Realm::QiRefining7}, {"QiRefining8", Realm::QiRefining8},
        {"QiRefining9", Realm::QiRefining9}, {"QiRefining10", Realm::QiRefining10},
        {"QiRefining11", Realm::QiRefining11}, {"QiRefining12", Realm::QiRefining12},
        {"QiRefining13", Realm::QiRefining13},
        {"FoundationEarly", Realm::FoundationEarly}, {"FoundationMid", Realm::FoundationMid},
        {"FoundationLate", Realm::FoundationLate},
        {"CoreEarly", Realm::CoreEarly}, {"CoreMid", Realm::CoreMid}, {"CoreLate", Realm::CoreLate},
    };
    const auto it = kTable.find(s);
    if (it == kTable.end()) return false;
    out = it->second;
    return true;
}

namespace {

[[nodiscard]] bool hasNonEmptyString(const json& j, const char* field) {
    return j.contains(field) && j[field].is_string() && !j[field].get<std::string>().empty();
}

// 可选的非负整数字段。字段不在就保持 out 不变；在但不是整数、或是负数，
// 一律判失败由调用方报错。
//
// 负数当场拦下而不是夹到 0：写 -3 的人本意多半是别的意思，夹成 0 会让这条
// 数据「生效了但什么也没发生」，而那是最难查的一种数据错。
[[nodiscard]] bool readNonNegativeInt(const json& j, const char* field, int& out) {
    if (!j.contains(field)) return true;
    if (!j[field].is_number_integer()) return false;
    const int value = j[field].get<int>();
    if (value < 0) return false;
    out = value;
    return true;
}

// 一列攻击类别的中文名 → 位掩码（docs/octopath-battle.md 第 5 节）。
//
// 三种写坏一律判失败，error 带出是哪一项：不是数组、某一项不是十种类别之一
//（allowed 之外的也算——兵刃那一栏写个「火」）、同一样写了两遍。
// 最后一种不是洁癖：界面按掩码画格子，写两遍的那一样只画一格，
// 而写数据的人以为自己给了两个破绽。
[[nodiscard]] bool readCategoryList(const json& j, const char* field, int allowed, int& out,
                                    std::string& error) {
    if (!j[field].is_array()) {
        error = std::string(field) + " 必须是类别名的数组";
        return false;
    }
    int mask = kCategoryNone;
    for (const json& entry : j[field]) {
        if (!entry.is_string()) {
            error = std::string(field) + " 里有非字符串项";
            return false;
        }
        const std::string name = entry.get<std::string>();
        const int bit = categoryFromName(name);
        if (bit == kCategoryNone || (bit & allowed) == 0) {
            error = std::string(field) + " 里的「" + name + "」不是可用的类别（只许：" +
                    categoryNames(allowed) + "）";
            return false;
        }
        if ((mask & bit) != 0) {
            error = std::string(field) + " 里「" + name + "」写了两遍";
            return false;
        }
        mask |= bit;
    }
    out = mask;
    return true;
}

// ---- items ----

core::Result<bool> loadOneItem(const fs::path& path, std::map<std::string, Item>& items,
                                std::map<std::string, fs::path>& firstSeenBy) {
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return core::Result<bool>::failure(parsed.error);
    const json& j = parsed.value;

    if (!j.is_object() || !hasNonEmptyString(j, "id")) {
        return core::Result<bool>::failure("物品缺少 id 字段: " + path.string());
    }
    std::string id = j["id"].get<std::string>();
    if (!hasNonEmptyString(j, "name")) {
        return core::Result<bool>::failure("物品 \"" + id + "\" 缺少 name 字段: " + path.string());
    }

    const auto dup = firstSeenBy.find(id);
    if (dup != firstSeenBy.end()) {
        return core::Result<bool>::failure("物品 id 重复: \"" + id + "\" 同时出现在 " +
                                            dup->second.string() + " 与 " + path.string());
    }

    Item item;
    item.id = id;
    item.name = j["name"].get<std::string>();
    item.descKey = j.value("descKey", item.descKey);
    if (j.contains("kind")) {
        if (!j["kind"].is_string() || !parseItemKind(j["kind"].get<std::string>(), item.kind)) {
            return core::Result<bool>::failure("物品 \"" + id + "\" 的 kind 字段不合法: " + path.string());
        }
    }
    item.grade = j.value("grade", item.grade);
    item.price = j.value("price", item.price);
    item.tradeable = j.value("tradeable", item.tradeable);
    item.herbAge = j.value("herbAge", item.herbAge);

    // 灵草的年份上限。绿液与自然生长都不能把它推过这个数，因此这个字段缺失
    // 时**绝不能**退化成「无上限」：那正是「把整瓶绿液砸在同一株低阶药上」
    // 刷出天价的口子。缺字段就按品阶推默认阶梯（rules::herbMaxAgeForGrade），
    // 连 grade 都没写的老数据按一阶处理。
    if (item.kind == ItemKind::Herb) {
        item.maxAge = rules::herbMaxAgeForGrade(item.grade);
        if (j.contains("maxAge")) {
            if (!j["maxAge"].is_number_integer()) {
                return core::Result<bool>::failure("物品 \"" + id + "\" 的 maxAge 字段必须是整数: " +
                                                    path.string());
            }
            const int declared = j["maxAge"].get<int>();
            // 越界当场报错而不是悄悄夹住：写 0（或负数）本意多半是「不限」，
            // 夹成 0 会让这味药一滴也浇不动；写超过万年则是把绝对溢出闸绕过去。
            // 两种都是数据错，要在加载时就喊出来。
            if (declared <= 0 || declared > rules::kMaxHerbAge) {
                return core::Result<bool>::failure(
                    "物品 \"" + id + "\" 的 maxAge 必须在 1 到 " + std::to_string(rules::kMaxHerbAge) +
                    " 之间，实为 " + std::to_string(declared) + ": " + path.string());
            }
            item.maxAge = declared;
        }
    }

    item.restoreHp = j.value("restoreHp", item.restoreHp);
    item.restoreMp = j.value("restoreMp", item.restoreMp);
    item.addAttack = j.value("addAttack", item.addAttack);
    item.addDefence = j.value("addDefence", item.addDefence);

    // 用毒（契约 docs/interfaces-p3-ch03.md 第 2.3 节）。毒药写 poison +
    // poisonPower，解毒药写 curesPoison。两条路径都在数据里，game 层不认识
    // 任何一个具体物品 id。
    //
    // 只写了一半（有回合数没强度、或反过来）当场报错，不悄悄按 0 收下：
    // 那样这颗毒药会正常地被用掉、正常地扣掉一件，唯独不中毒——而症状是
    // 「这药好像没用」，查起来要一路查到结算代码里去。
    if (!readNonNegativeInt(j, "poison", item.poison)) {
        return core::Result<bool>::failure("物品 \"" + id + "\" 的 poison 必须是非负整数: " +
                                            path.string());
    }
    if (!readNonNegativeInt(j, "poisonPower", item.poisonPower)) {
        return core::Result<bool>::failure("物品 \"" + id + "\" 的 poisonPower 必须是非负整数: " +
                                            path.string());
    }
    if ((item.poison > 0) != (item.poisonPower > 0)) {
        return core::Result<bool>::failure(
            "物品 \"" + id + "\" 的 poison 与 poisonPower 必须同时给出（写了一半的毒不会中毒）: " +
            path.string());
    }
    if (j.contains("curesPoison")) {
        if (!j["curesPoison"].is_boolean()) {
            return core::Result<bool>::failure("物品 \"" + id + "\" 的 curesPoison 必须是布尔值: " +
                                                path.string());
        }
        item.curesPoison = j["curesPoison"].get<bool>();
    }
    if (item.curesPoison && item.poison > 0) {
        // 一件既下毒又解毒的东西，在结算里先解后中，玩家看到的是「解毒药把人
        // 毒了」。这多半是把两份数据抄混了，当场拦下比让它上线强。
        return core::Result<bool>::failure("物品 \"" + id + "\" 既下毒又解毒，二者不能并存: " +
                                            path.string());
    }

    // 兵器给的攻击类别（docs/octopath-battle.md 2.2）："weapon": "剑"。只许一个兵刃类别：
    // 一件兵器就是一种打法，写成数组或写个「火」都是数据错，当场报出来——
    // 悄悄收下的话，韩立的普攻菜单里会多出一行谁也没打算给他的兵刃。
    //
    // 从前这里读 useRange（使用距离，技术债 G-7）。格子没了，这个字段不再读；
    // 数据里残留的由 tools/validate.py 报错，不在这里悄悄吞掉之外再造一道闸。
    if (j.contains("weapon")) {
        const int bit = j["weapon"].is_string() ? categoryFromName(j["weapon"].get<std::string>())
                                                : kCategoryNone;
        if ((bit & kWeaponCategories) == 0) {
            return core::Result<bool>::failure("物品 \"" + id + "\" 的 weapon 必须是兵刃类别之一（" +
                                                categoryNames(kWeaponCategories) + "）: " +
                                                path.string());
        }
        item.weapon = bit;
    }

    // 道具施法（契约 docs/interfaces-p3-ch07.md 2.2 / 2.4）："castMagic": "magic_tianleizi"——
    // 在战斗里用它 = 以那门法术施展一次。写了就得是非空字符串；与四样药效同写报错：
    // 一件东西要么是药、要么是符（结算只走得了一条路，另一半会悄悄作废）。
    // 指向的法术存不存在、施展不施展得了，要等法术也读完才查得了（loadGameDataImpl 的汇总处）。
    if (j.contains("castMagic")) {
        if (!j["castMagic"].is_string() || j["castMagic"].get<std::string>().empty()) {
            return core::Result<bool>::failure("物品 \"" + id + "\" 的 castMagic 必须是非空的法术 id: " +
                                                path.string());
        }
        item.castMagic = j["castMagic"].get<std::string>();
        if (item.restoreHp > 0 || item.restoreMp > 0 || item.poison > 0 || item.curesPoison) {
            return core::Result<bool>::failure(
                "物品 \"" + id + "\" 的 castMagic 不能与 restoreHp / restoreMp / poison / curesPoison 同写"
                "（一件东西要么是药、要么是符）: " + path.string());
        }
    }

    firstSeenBy.emplace(id, path);
    items.emplace(std::move(id), std::move(item));
    return core::Result<bool>::success(true);
}

// ---- magics ----

core::Result<bool> loadOneMagic(const fs::path& path, std::map<std::string, Magic>& magics,
                                 std::map<std::string, fs::path>& firstSeenBy) {
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return core::Result<bool>::failure(parsed.error);
    const json& j = parsed.value;

    if (!j.is_object() || !hasNonEmptyString(j, "id")) {
        return core::Result<bool>::failure("法术缺少 id 字段: " + path.string());
    }
    std::string id = j["id"].get<std::string>();
    if (!hasNonEmptyString(j, "name")) {
        return core::Result<bool>::failure("法术 \"" + id + "\" 缺少 name 字段: " + path.string());
    }

    const auto dup = firstSeenBy.find(id);
    if (dup != firstSeenBy.end()) {
        return core::Result<bool>::failure("法术 id 重复: \"" + id + "\" 同时出现在 " +
                                            dup->second.string() + " 与 " + path.string());
    }

    Magic magic;
    magic.id = id;
    magic.name = j["name"].get<std::string>();
    magic.descKey = j.value("descKey", magic.descKey);
    magic.element = j.value("element", magic.element);
    magic.needMp = j.value("needMp", magic.needMp);
    magic.power = j.value("power", magic.power);
    // castRange（施法距离）不再读：横版战斗没有距离。数据里残留的由门禁报错。
    if (j.contains("needRealm")) {
        if (!j["needRealm"].is_string() || !parseRealmName(j["needRealm"].get<std::string>(), magic.needRealm)) {
            return core::Result<bool>::failure("法术 \"" + id + "\" 的 needRealm 字段不合法: " + path.string());
        }
    }

    // 下毒的法术（契约第 2.3 节要求物品与法术各一条路径）。口径与物品那边一致：
    // 只写了一半的毒不会中毒，当场报错而不是悄悄按 0 收下。
    if (!readNonNegativeInt(j, "poison", magic.poison)) {
        return core::Result<bool>::failure("法术 \"" + id + "\" 的 poison 必须是非负整数: " +
                                            path.string());
    }
    if (!readNonNegativeInt(j, "poisonPower", magic.poisonPower)) {
        return core::Result<bool>::failure("法术 \"" + id + "\" 的 poisonPower 必须是非负整数: " +
                                            path.string());
    }
    if ((magic.poison > 0) != (magic.poisonPower > 0)) {
        return core::Result<bool>::failure(
            "法术 \"" + id + "\" 的 poison 与 poisonPower 必须同时给出（写了一半的毒不会中毒）: " +
            path.string());
    }

    // 蓄劲的方式（docs/octopath-battle.md 2.3）："hits" 连发，"power" 威力翻倍。缺省 power。
    // 写了别的一律报错：拼错成 "hit" 的人本意是连发，悄悄按 power 收下，
    // 火弹术蓄满劲就从四发变成一发，而症状只是「怎么蓄了劲也破不了势」。
    if (j.contains("boost")) {
        const std::string boost = j["boost"].is_string() ? j["boost"].get<std::string>() : std::string{};
        if (boost == "hits") {
            magic.boost = core::MagicBoost::Hits;
        } else if (boost == "power") {
            magic.boost = core::MagicBoost::Power;
        } else {
            return core::Result<bool>::failure("法术 \"" + id +
                                                "\" 的 boost 只许 \"hits\" 或 \"power\": " +
                                                path.string());
        }
    }

    // 不伤人的效果（契约 docs/interfaces-p3-ch06.md 1.2、docs/interfaces-p3-ch07.md 2.2）：只认
    // "reveal"（天眼术的看破）与 "stagger"（削架势），缺省没有。
    // 写了别的（含空串、大小写、多一个空格）一律报错，口径同上面的 boost。
    // 与 power > 0 / poison > 0 同写也报错：一门法术要么伤人、要么看破或削架势，免得有人给火弹术
    // 挂个 reveal 变成「打一下顺便全看穿」。power 缺省是 10，所以带效果的法术得明写 "power": 0。
    if (j.contains("effect")) {
        const std::string effect = j["effect"].is_string() ? j["effect"].get<std::string>() : std::string{};
        if (effect == "reveal") {
            magic.effect = core::MagicEffect::Reveal;
        } else if (effect == "stagger") {
            magic.effect = core::MagicEffect::Stagger;
        } else {
            return core::Result<bool>::failure("法术 \"" + id + "\" 的 effect 只许 \"reveal\" 或 \"stagger\": " +
                                                path.string());
        }
        if (magic.power > 0 || magic.poison > 0) {
            return core::Result<bool>::failure("法术 \"" + id +
                                                "\" 的 effect 不能与 power > 0 或 poison > 0 同写"
                                                "（一门法术要么伤人、要么看破或削架势）: " + path.string());
        }
    }
    // 削几点架势（契约 docs/interfaces-p3-ch07.md 2.2）：1–9 的整数，只许与 effect "stagger" 同写；
    // effect "stagger" 而不写它取 1。写在别的法术上等于一个谁也不读的数，写成 "3" 或 0 同样当场报出来。
    if (j.contains("stagger")) {
        if (magic.effect != core::MagicEffect::Stagger) {
            return core::Result<bool>::failure("法术 \"" + id + "\" 的 stagger 只许与 effect \"stagger\" 同写: " +
                                                path.string());
        }
        const int value = j["stagger"].is_number_integer() ? j["stagger"].get<int>() : 0;
        if (value < 1 || value > 9) {
            return core::Result<bool>::failure("法术 \"" + id + "\" 的 stagger 必须是 1 到 9 的整数: " +
                                                path.string());
        }
        magic.stagger = value;
    }

    if (j.contains("target")) {
        const std::string target = j["target"].is_string() ? j["target"].get<std::string>() : std::string{};
        if (target == "all") {
            magic.target = core::MagicTarget::All;
        } else if (target != "single") {
            return core::Result<bool>::failure("法术 \"" + id +
                "\" 的 target 只许 \"single\" 或 \"all\": " + path.string());
        }
        if (magic.target == core::MagicTarget::All &&
            (magic.effect != core::MagicEffect::None || !core::offensiveMagic(magic))) {
            return core::Result<bool>::failure("法术 \"" + id +
                "\" 的 target \"all\" 只许用于伤人法术，不与 reveal / stagger 同写: " + path.string());
        }
    }

    firstSeenBy.emplace(id, path);
    magics.emplace(std::move(id), std::move(magic));
    return core::Result<bool>::success(true);
}

// 首领的蓄势（docs/octopath-battle.md 2.6）：{"every": 3, "mult": 2.5, "all": true, "text_key": "…"}。
// every ≥ 2：它自己的第 every 个出手回合宣告、下一手出重招，而出重招那一回合也算一个出手回合，
// 所以 every = 1 会让它每一手都在宣告；every = 2 已经是「宣告、重招」首尾相接，不再有普通的一手。
// mult 必须是正数（写 0 的重招比普通一击还轻，那不是重招）。text_key 必须写：宣告时玩家要看见
// 一句预告，否则「蓄势」只是一个日志里的词。
core::Result<bool> readCharge(const json& node, const std::string& id, const fs::path& path,
                              core::ChargeSpec& out) {
    const auto fail = [&](const std::string& what) {
        return core::Result<bool>::failure("角色 \"" + id + "\" 的 charge." + what + ": " + path.string());
    };
    if (!node.is_object()) return fail("整项必须是对象");
    if (!node.contains("every") || !node["every"].is_number_integer() || node["every"].get<int>() < 2) {
        return fail("every 必须是 ≥ 2 的整数");
    }
    if (!node.contains("mult") || !node["mult"].is_number() || node["mult"].get<double>() <= 0.0) {
        return fail("mult 必须是正数");
    }
    if (node.contains("all") && !node["all"].is_boolean()) return fail("all 必须是 true / false");
    if (!hasNonEmptyString(node, "text_key")) return fail("text_key 必须是非空文案 key");
    out.every = node["every"].get<int>();
    out.mult = node["mult"].get<double>();
    out.all = node.value("all", false);
    out.textKey = node["text_key"].get<std::string>();
    return core::Result<bool>::success(true);
}

// ---- roles ----

core::Result<bool> loadOneRole(const fs::path& path, std::map<std::string, RoleTemplate>& roles,
                                std::map<std::string, fs::path>& firstSeenBy) {
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return core::Result<bool>::failure(parsed.error);
    const json& j = parsed.value;

    if (!j.is_object() || !hasNonEmptyString(j, "id")) {
        return core::Result<bool>::failure("角色缺少 id 字段: " + path.string());
    }
    std::string id = j["id"].get<std::string>();
    if (!hasNonEmptyString(j, "name")) {
        return core::Result<bool>::failure("角色 \"" + id + "\" 缺少 name 字段: " + path.string());
    }

    const auto dup = firstSeenBy.find(id);
    if (dup != firstSeenBy.end()) {
        return core::Result<bool>::failure("角色 id 重复: \"" + id + "\" 同时出现在 " +
                                            dup->second.string() + " 与 " + path.string());
    }

    RoleTemplate role;
    role.id = id;
    role.name = j["name"].get<std::string>();
    if (j.contains("realm")) {
        if (!j["realm"].is_string() || !parseRealmName(j["realm"].get<std::string>(), role.realm)) {
            return core::Result<bool>::failure("角色 \"" + id + "\" 的 realm 字段不合法: " + path.string());
        }
    }
    role.maxHp = j.value("maxHp", role.maxHp);
    role.maxMp = j.value("maxMp", role.maxMp);
    role.attack = j.value("attack", role.attack);
    role.defence = j.value("defence", role.defence);
    role.speed = j.value("speed", role.speed);
    role.element = j.value("element", role.element);
    if (j.contains("magics")) {
        if (!j["magics"].is_array()) {
            return core::Result<bool>::failure("角色 \"" + id + "\" 的 magics 字段必须是数组: " + path.string());
        }
        for (const json& entry : j["magics"]) {
            if (!entry.is_string()) {
                return core::Result<bool>::failure("角色 \"" + id + "\" 的 magics 数组里有非字符串项: " +
                                                    path.string());
            }
            role.magics.push_back(entry.get<std::string>());
        }
    }

    // ---- 破势与蓄劲（docs/octopath-battle.md 第 5 节）----
    // 字段都可选（不上场的 NPC 一样也不写），写了就得写对，写错报出文件与字段。
    const auto fail = [&](const std::string& what) {
        return core::Result<bool>::failure("角色 \"" + id + "\" 的 " + what + ": " + path.string());
    };
    std::string error;
    if (j.contains("weapons")) {
        if (!readCategoryList(j, "weapons", kWeaponCategories, role.weapons, error)) return fail(error);
        // 空数组 = 连拳都不会打，那不是任何一个活人。要「空手」就不写这一项（缺省即拳）。
        if (role.weapons == kCategoryNone) return fail("weapons 不能是空数组（空手就不写，缺省是拳）");
    }
    if (j.contains("weaknesses")) {
        if (!readCategoryList(j, "weaknesses", kAllCategories, role.weaknesses, error)) return fail(error);
    }
    if (j.contains("toughness")) {
        // 写 0 的人本意多半是「没有架势」——那就不写这一项。收下 0 会让一个上场的敌人
        // 永远破不了势，而门禁那一条（上场的敌人必须有架势）也会因为「写了」而放过它。
        if (!j["toughness"].is_number_integer() || j["toughness"].get<int>() < 1) {
            return fail("toughness 必须是 ≥ 1 的整数（没有架势就不写这一项）");
        }
        role.toughness = j["toughness"].get<int>();
    }
    if (role.toughness > 0 && role.weaknesses == kCategoryNone) {
        return fail("有架势却一样破绽都没有：永远破不了势（weaknesses 至少写一样）");
    }
    if (j.contains("actions")) {
        if (!j["actions"].is_number_integer() || j["actions"].get<int>() < 1) {
            return fail("actions 必须是 ≥ 1 的整数");
        }
        role.actions = j["actions"].get<int>();
    }
    if (j.contains("charge")) {
        core::Result<bool> charge = readCharge(j["charge"], id, path, role.charge);
        if (!charge) return charge;
    }
    if (j.contains("killable_by")) {
        if (!readCategoryList(j, "killable_by", kAllCategories, role.killableBy, error)) return fail(error);
        // 空数组 = 什么都要不了他的命：那是一个打不死的敌人，这一场永远赢不下来。
        if (role.killableBy == kCategoryNone) return fail("killable_by 不能是空数组（谁都杀得死就不写）");
    }

    firstSeenBy.emplace(id, path);
    roles.emplace(std::move(id), std::move(role));
    return core::Result<bool>::success(true);
}

// ---- text ----

core::Result<bool> loadOneTextFile(const fs::path& path, std::map<std::string, std::string>& text,
                                    std::map<std::string, fs::path>& firstSeenBy) {
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return core::Result<bool>::failure(parsed.error);
    const json& j = parsed.value;

    if (!j.is_object()) {
        return core::Result<bool>::failure("文案文件顶层必须是一个对象: " + path.string());
    }
    for (const auto& item : j.items()) {
        const std::string& key = item.key();
        const json& value = item.value();
        if (!value.is_string()) {
            return core::Result<bool>::failure("文案 \"" + key + "\" 的值不是字符串: " + path.string());
        }
        const auto dup = firstSeenBy.find(key);
        if (dup != firstSeenBy.end()) {
            return core::Result<bool>::failure("文案 key 重复: \"" + key + "\" 同时出现在 " +
                                                dup->second.string() + " 与 " + path.string());
        }
        firstSeenBy.emplace(key, path);
        text.emplace(key, value.get<std::string>());
    }
    return core::Result<bool>::success(true);
}

// ---- objectives ----

// 一章的目标链。文件形如 data/objectives/ch01.json：顶层是一条数据条目
//（id + name，与 data/ 下别的文件同规矩），steps 数组就是这一章的推进顺序。
//
// 一章一个文件而不是一步一个文件：这条链的价值有一半在「顺序」上，而顺序摊成
// 四十来个文件之后就只剩下文件名里那个序号在维持——那种顺序改起来必错。
//
// **同一趟里另产出一条主线任务**（契约 docs/interfaces-p3-ch05.md 1.5，Q8 原地收编）：
// 文件不动、不改名、内容不改，out 那条链一个字节不变——HUD 与告示板上半截仍只读它；
// mainQuest 与它逐步对应，只是换了个「任务」的身份，让告示板与测试能把主线和支线
// 放在同一张表里问状态。两份出自同一次解析，不会走散。
core::Result<bool> loadOneObjectiveFile(const fs::path& path,
                                        std::vector<core::Objective>& out,
                                        core::Quest& mainQuest) {
    core::Result<json> parsed = detail::readJsonFile(path);
    if (!parsed) return core::Result<bool>::failure(parsed.error);
    const json& j = parsed.value;

    if (!j.is_object()) {
        return core::Result<bool>::failure("目标链文件顶层必须是一个对象: " + path.string());
    }
    if (!j.contains("chapter") || !j["chapter"].is_number_integer()) {
        return core::Result<bool>::failure("目标链缺少整数 chapter: " + path.string());
    }
    if (!j.contains("steps") || !j["steps"].is_array()) {
        return core::Result<bool>::failure("目标链缺少 steps 数组: " + path.string());
    }
    // 顶层 id 是主线任务的 id。门禁（collect_data）一直要求每个数据文件有 id，
    // 从前加载器不读它，所以不查；现在要用，就得查——缺 id 的主线任务在告示板与
    // 测试里连名字都叫不出来。
    if (!hasNonEmptyString(j, "id")) {
        return core::Result<bool>::failure("目标链缺少 id: " + path.string());
    }

    const int chapter = j["chapter"].get<int>();
    mainQuest = core::Quest{};
    mainQuest.id = j["id"].get<std::string>();
    if (j.contains("name") && j["name"].is_string()) mainQuest.name = j["name"].get<std::string>();
    mainQuest.chapter = chapter;
    mainQuest.kind = core::QuestKind::Main;
    for (const json& step : j["steps"]) {
        if (!step.is_object()) {
            return core::Result<bool>::failure("目标链的 steps 里有非对象项: " + path.string());
        }
        // 五个字段一个都不能少。缺一个就是一步指不出去或判不出完成的目标，
        // 而那样的一步在画面上与「目标系统坏了」没有区别——宁可加载失败。
        static constexpr const char* kRequired[] = {"id", "text_key", "done_flag", "target_map",
                                                    "target_object"};
        for (const char* field : kRequired) {
            if (!step.contains(field) || !step[field].is_string() ||
                step[field].get<std::string>().empty()) {
                return core::Result<bool>::failure(std::string("目标链的某一步缺少 ") + field +
                                                    ": " + path.string());
            }
        }

        core::Objective objective;
        objective.id = step["id"].get<std::string>();
        objective.textKey = step["text_key"].get<std::string>();
        objective.doneFlag = step["done_flag"].get<std::string>();
        objective.targetMap = step["target_map"].get<std::string>();
        objective.targetObject = step["target_object"].get<std::string>();
        objective.chapter = chapter;

        // 主线任务的这一步：形状照抄，done 留空——主线的完成只认目标链判据
        // （doneFlag 置位），不在任务里另写一份，见 core::QuestStep::done 的注释。
        core::QuestStep questStep;
        questStep.id = objective.id;
        questStep.textKey = objective.textKey;
        questStep.targetMap = objective.targetMap;
        questStep.targetObject = objective.targetObject;
        mainQuest.steps.push_back(std::move(questStep));

        out.push_back(std::move(objective));
    }
    return core::Result<bool>::success(true);
}

// 实际加载逻辑。各 loadOneXxx 在触碰字段前都先判过 is_object()/is_string()，
// 理论上不会再抛；但公开入口仍然包一层 try/catch 兜底（见 loadGameData），
// 防止某个遗漏的类型判断在畸形输入下把异常甩出模块边界。
core::Result<GameData> loadGameDataImpl(const std::string& dataRootStr) {
    const fs::path dataRoot(dataRootStr);
    std::error_code ec;
    if (!fs::exists(dataRoot, ec) || !fs::is_directory(dataRoot, ec)) {
        return core::Result<GameData>::failure("数据目录不存在: " + dataRoot.string());
    }

    GameData data;

    std::map<std::string, fs::path> itemSeen;
    for (const fs::path& file : listJsonFilesSorted(dataRoot / "items")) {
        core::Result<bool> r = loadOneItem(file, data.items, itemSeen);
        if (!r) return core::Result<GameData>::failure(r.error);
    }

    std::map<std::string, fs::path> magicSeen;
    for (const fs::path& file : listJsonFilesSorted(dataRoot / "magics")) {
        core::Result<bool> r = loadOneMagic(file, data.magics, magicSeen);
        if (!r) return core::Result<GameData>::failure(r.error);
    }

    // 道具施法的引用（契约 docs/interfaces-p3-ch07.md 2.4）：物品与法术都读完了才查得了。
    // 指向不存在的法术，或指向施展不了的法术（护身罡这类不伤人、也没有效果的），当场报出来——
    // 悄悄收下的话，这件符在战斗菜单里要么点不动、要么点下去什么也不发生。
    for (const auto& [itemId, item] : data.items) {
        if (item.castMagic.empty()) continue;
        const Magic* magic = data.findMagic(item.castMagic);
        const std::string where = itemSeen.at(itemId).string();
        if (magic == nullptr) {
            return core::Result<GameData>::failure("物品 \"" + itemId + "\" 的 castMagic 指向不存在的法术 \"" +
                                                   item.castMagic + "\": " + where);
        }
        if (!core::castableMagic(*magic)) {
            return core::Result<GameData>::failure("物品 \"" + itemId + "\" 的 castMagic 指向的法术 \"" +
                                                   item.castMagic + "\" 施展不了（不伤人、也没有效果）: " + where);
        }
    }

    std::map<std::string, fs::path> roleSeen;
    for (const fs::path& file : listJsonFilesSorted(dataRoot / "roles")) {
        core::Result<bool> r = loadOneRole(file, data.roles, roleSeen);
        if (!r) return core::Result<GameData>::failure(r.error);
    }

    std::map<std::string, fs::path> textSeen;
    for (const fs::path& file : listJsonFilesSorted(dataRoot / "text")) {
        core::Result<bool> r = loadOneTextFile(file, data.text, textSeen);
        if (!r) return core::Result<GameData>::failure(r.error);
    }

    // 目标链。listJsonFilesSorted 按路径排序，文件名就是 ch01…ch04，于是链内
    // 顺序 = steps 的数组顺序，链间顺序 = 章号顺序，两者都不依赖文件系统的
    // 返回次序。再按 chapter 稳定排一次，免得将来有人把文件改名成别的样子。
    std::vector<core::Quest> mainQuests;
    std::map<int, fs::path> mainQuestOfChapter;
    for (const fs::path& file : listJsonFilesSorted(dataRoot / "objectives")) {
        core::Quest mainQuest;
        core::Result<bool> r = loadOneObjectiveFile(file, data.objectives, mainQuest);
        if (!r) return core::Result<GameData>::failure(r.error);
        // 一章一条主线（契约 1.5）：两个文件同属一章，「这一章的主线在进行中」就说不清
        // 指的是哪一条，而 questStatus 按章号圈链上那一段，会把两条的步骤圈成一条。
        const auto dup = mainQuestOfChapter.find(mainQuest.chapter);
        if (dup != mainQuestOfChapter.end()) {
            return core::Result<GameData>::failure(
                "两条目标链同属第 " + std::to_string(mainQuest.chapter) + " 章: " +
                dup->second.string() + " 与 " + file.string());
        }
        mainQuestOfChapter.emplace(mainQuest.chapter, file);
        mainQuests.push_back(std::move(mainQuest));
    }
    std::stable_sort(data.objectives.begin(), data.objectives.end(),
                     [](const core::Objective& a, const core::Objective& b) {
                         return a.chapter < b.chapter;
                     });
    std::stable_sort(mainQuests.begin(), mainQuests.end(),
                     [](const core::Quest& a, const core::Quest& b) { return a.chapter < b.chapter; });

    // 支线（契约 1.2 / 1.6）。形状由 QuestLoader 查，引用由门禁查。
    core::Result<std::vector<core::Quest>> sideQuests =
        loadQuests((dataRoot / "quests").string());
    if (!sideQuests) return core::Result<GameData>::failure(sideQuests.error);

    // 主线与支线共用一个 id 空间：告示板与测试按 id 认任务，撞车就认错人。
    std::map<std::string, std::string> questIdSeen;
    for (const core::Quest& quest : mainQuests) questIdSeen.emplace(quest.id, "data/objectives");
    for (const core::Quest& quest : sideQuests.value) {
        if (questIdSeen.count(quest.id) != 0) {
            return core::Result<GameData>::failure("支线任务 id 与目标链撞车: " + quest.id);
        }
        questIdSeen.emplace(quest.id, "data/quests");
    }

    data.quests = std::move(mainQuests);
    for (core::Quest& quest : sideQuests.value) data.quests.push_back(std::move(quest));

    // 路径行动（契约 docs/interfaces-octo-pathactions.md 第 4.3 节）。只查形状：这里只拿得到 data/，
    // 好几组测试只把 data/ 抄进临时根目录；地图、编成、旗标登记的引用归门禁与 PathActionTests。
    core::Result<std::vector<core::PathAction>> pathActions =
        loadPathActionDir((dataRoot / "pathactions").string());
    if (!pathActions) return core::Result<GameData>::failure(pathActions.error);
    data.pathActions = std::move(pathActions.value);

    return core::Result<GameData>::success(std::move(data));
}

}  // namespace

core::Result<GameData> loadGameData(const std::string& dataRootStr) {
    try {
        return loadGameDataImpl(dataRootStr);
    } catch (const std::exception& e) {
        return core::Result<GameData>::failure("加载数据目录 " + dataRootStr + " 时出现未预期的异常: " + e.what());
    }
}

}  // namespace fanren::io
