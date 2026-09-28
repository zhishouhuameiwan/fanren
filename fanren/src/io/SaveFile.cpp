#include "io/SaveFile.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <nlohmann/json.hpp>

#include "core/rules/Realm.h"

namespace fanren::io {

namespace {

namespace fs = std::filesystem;
using nlohmann::json;
using fanren::core::BagEntry;
using fanren::core::GameState;
using fanren::core::Point;
using fanren::rules::Realm;

// ---- 校验和 ----
//
// 选 FNV-1a 64 位，而不是接入一个真正的密码学哈希（如 SHA-256）：
//   1. 威胁模型是「防手滑/手改存档 JSON 文本」，不是防一个愿意重新编译游戏、
//      绕开校验代码本身的攻击者——真到那一步，换哪种哈希都挡不住，因为校验
//      逻辑和游戏本体在同一个可执行文件里。这个量级的防护，FNV-1a 的雪崩性
//      足够：改一个字节，校验和大概率不同。
//   2. 项目没有为存档这一件事单独 vendor 一个加密库；FNV-1a 十几行代码、零
//      依赖、纯整数运算，跨平台结果一定一致。
//   3. 校验和覆盖 "version|payload.dump()" 而不只是 payload：把版本号一起绑进
//      去，不给"拿旧版本的合法 payload+checksum 换一个 save_version 来绕过
//      迁移表"留空子。
// nlohmann::json 默认的对象类型按 key 字母序存储，dump() 输出与插入顺序无关，
// 所以同一份逻辑内容在写盘时和读盘校验时算出的 dump() 字符串必然一致，
// checksum 可以直接按字符串比较，不需要额外的规范化步骤。
std::uint64_t fnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string toHex16(std::uint64_t v) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << v;
    return oss.str();
}

std::string computeChecksum(int version, const json& payload) {
    return toHex16(fnv1a64(std::to_string(version) + "|" + payload.dump()));
}

// ---- GameState <-> json ----
//
// 字段列表对应 Types.h 的 core::GameState：位置、境界、修为、hp/mp、背包（含灵
// 草年份）、旗标、章节、日期、两个计时器——存档要求的「全部字段」照单序列化，
// 不挑着存，免得漏掉哪个字段导致读档后状态对不上。

json toJson(const GameState& s) {
    json bag = json::array();
    for (const BagEntry& e : s.bag) {
        bag.push_back(json{{"itemId", e.itemId}, {"count", e.count}, {"herbAge", e.herbAge}});
    }
    json payload;
    payload["mapId"] = s.mapId;
    payload["position"] = json{{"x", s.position.x}, {"y", s.position.y}};
    payload["facing"] = s.facing;
    payload["realm"] = rules::toValue(s.realm);
    payload["cultivation"] = s.cultivation;
    payload["hp"] = s.hp;
    payload["maxHp"] = s.maxHp;
    payload["mp"] = s.mp;
    payload["maxMp"] = s.maxMp;
    payload["day"] = s.day;
    payload["chapter"] = s.chapter;
    payload["bag"] = std::move(bag);
    payload["flags"] = s.flags;
    payload["playSecondsGameplay"] = s.playSecondsGameplay;
    payload["playSecondsSystem"] = s.playSecondsSystem;

    // P2 系统状态。掌天瓶的两个开关分别存：owned 与 matureKnown 对应原著里
    // 相隔四年的两个节点，合并成一个布尔会把第 2 章的结构压没。
    payload["bottle"] = json{{"owned", s.bottle.owned},
                             {"matureKnown", s.bottle.matureKnown},
                             {"drops", s.bottle.drops},
                             {"lastChargeDay", s.bottle.lastChargeDay},
                             {"capacity", s.bottle.capacity}};

    json fields = json::array();
    for (const rules::SpiritField& f : s.fields) {
        json slots = json::array();
        for (const rules::FieldSlot& slot : f.slots) {
            // maxAge 是这一株按种的年份上限，自然生长靠它停住。不存的话，
            // 读档之后这一畦就又能一路长到全局上限——所以它必须落盘。
            slots.push_back(json{{"seedId", slot.seedId},
                                 {"plantedDay", slot.plantedDay},
                                 {"age", slot.age},
                                 {"ripe", slot.ripe},
                                 {"maxAge", slot.maxAge}});
        }
        fields.push_back(json{{"id", f.id}, {"slots", std::move(slots)}});
    }
    payload["fields"] = std::move(fields);

    payload["alchemyProficiency"] = s.alchemyProficiency;
    payload["talismanProficiency"] = s.talismanProficiency;
    payload["forgeProficiency"] = s.forgeProficiency;
    payload["formationProficiency"] = s.formationProficiency;
    payload["aptitude"] = s.aptitude;
    payload["cultivationRemainder"] = s.cultivationRemainder;
    // 日课的水位。不落盘的话，读档之后面板会把水位拨到当天，存档与上次开面板
    // 之间那段日子的功课就白过了——而那正是剧情推时间最多的那一段。
    payload["lastPracticeDay"] = s.lastPracticeDay;

    // 队伍（save_version 3 起）。三个字段一个都不能省：hp = -1 是「按模板满血」
    // 而不是「血空了」，active = false 是「留着位置不上场」而不是「没这个人」，
    // 漏存任何一个，读档后同伴的状态就变成另一回事。
    json party = json::array();
    for (const core::PartyMember& m : s.party) {
        party.push_back(json{{"roleId", m.roleId}, {"hp", m.hp}, {"active", m.active}});
    }
    payload["party"] = std::move(party);

    // 已习得法术（save_version 5 起）。顺序照原样落盘：那是习得的先后，
    // 也就是战斗菜单的行序——排序一下看着更整齐，代价是火弹术与御风决在菜单里
    // 的先后从此由拼音决定，而那两件事的先后是本章的剧情（设计第 1 节约束 1）。
    //
    // GameState::learnMagic 幂等，所以这里写出来的必然无重复；读入侧另有一道去重，
    // 挡的是手改过的存档（见 fromJson）。
    payload["learnedMagics"] = s.learnedMagics;

    // 剧情给的境界上限（save_version 6 起）。存的是境界编号，与 realm 同一套。
    payload["realmCap"] = rules::toValue(s.realmCap);

    // 已揭开的破绽（save_version 7 起）：role_id → 类别中文名数组（按位序）。
    // 写名字不写位掩码，理由见 SaveFile.h 第 7 条。
    json known = json::object();
    for (const auto& [roleId, mask] : s.knownWeaknesses) {
        json names = json::array();
        for (const int bit : core::categoryBits(mask)) names.push_back(core::categoryName(bit));
        known[roleId] = std::move(names);
    }
    payload["knownWeaknesses"] = std::move(known);

    // 野外遭遇的计数器（save_version 8 起）。四个数原样写，字段名与 rules::EncounterState 一致。
    payload["encounter"] = {
        {"stepsSinceLast", s.encounter.stepsSinceLast},
        {"stepsUntilNext", s.encounter.stepsUntilNext},
        {"triggeredToday", s.encounter.triggeredToday},
        {"lastDay", s.encounter.lastDay},
    };
    return payload;
}

core::Result<GameState> failState(const std::string& msg) { return core::Result<GameState>::failure(msg); }

core::Result<GameState> fromJson(const json& p) {
    if (!p.is_object()) return failState("存档 payload 不是 JSON 对象");

    GameState s;

    if (!p.contains("mapId") || !p["mapId"].is_string()) return failState("存档 payload 缺少 mapId 字段");
    s.mapId = p["mapId"].get<std::string>();

    if (!p.contains("position") || !p["position"].is_object() || !p["position"].contains("x") ||
        !p["position"]["x"].is_number_integer() || !p["position"].contains("y") ||
        !p["position"]["y"].is_number_integer()) {
        return failState("存档 payload 缺少合法的 position 字段");
    }
    s.position = Point{p["position"]["x"].get<int>(), p["position"]["y"].get<int>()};

    if (!p.contains("facing") || !p["facing"].is_number_integer()) return failState("存档 payload 缺少合法的 facing 字段");
    s.facing = p["facing"].get<int>();

    if (!p.contains("realm") || !p["realm"].is_number_integer()) return failState("存档 payload 缺少合法的 realm 字段");
    const int realmValue = p["realm"].get<int>();
    const Realm realm = rules::fromValue(realmValue);
    if (!rules::isValid(realm)) {
        // 存档读入必须先过 isValid 这道闸（见 Realm.h 的注释）：篡改过的存档
        // 完全可能把 realm 改成一个不存在的编号，不能悄悄接受。
        return failState("存档 payload 的 realm 编号非法（可能是手改存档）: " + std::to_string(realmValue));
    }
    s.realm = realm;

    if (!p.contains("cultivation") || !p["cultivation"].is_number_integer())
        return failState("存档 payload 缺少合法的 cultivation 字段");
    s.cultivation = p["cultivation"].get<int>();

    if (!p.contains("hp") || !p["hp"].is_number_integer()) return failState("存档 payload 缺少合法的 hp 字段");
    s.hp = p["hp"].get<int>();
    if (!p.contains("maxHp") || !p["maxHp"].is_number_integer()) return failState("存档 payload 缺少合法的 maxHp 字段");
    s.maxHp = p["maxHp"].get<int>();
    if (!p.contains("mp") || !p["mp"].is_number_integer()) return failState("存档 payload 缺少合法的 mp 字段");
    s.mp = p["mp"].get<int>();
    if (!p.contains("maxMp") || !p["maxMp"].is_number_integer()) return failState("存档 payload 缺少合法的 maxMp 字段");
    s.maxMp = p["maxMp"].get<int>();

    if (!p.contains("day") || !p["day"].is_number_integer()) return failState("存档 payload 缺少合法的 day 字段");
    s.day = p["day"].get<int>();
    if (!p.contains("chapter") || !p["chapter"].is_number_integer())
        return failState("存档 payload 缺少合法的 chapter 字段");
    s.chapter = p["chapter"].get<int>();

    if (!p.contains("bag") || !p["bag"].is_array()) return failState("存档 payload 缺少 bag 数组");
    for (const json& entry : p["bag"]) {
        if (!entry.is_object() || !entry.contains("itemId") || !entry["itemId"].is_string() ||
            !entry.contains("count") || !entry["count"].is_number_integer() || !entry.contains("herbAge") ||
            !entry["herbAge"].is_number_integer()) {
            return failState("存档 payload 的 bag 条目缺少 itemId/count/herbAge");
        }
        s.bag.push_back(BagEntry{entry["itemId"].get<std::string>(), entry["count"].get<int>(),
                                  entry["herbAge"].get<int>()});
    }

    if (!p.contains("flags") || !p["flags"].is_object()) return failState("存档 payload 缺少 flags 对象");
    for (const auto& item : p["flags"].items()) {
        if (!item.value().is_number_integer()) {
            return failState("存档 payload 的 flags 里 \"" + item.key() + "\" 的值不是整数");
        }
        s.flags[item.key()] = item.value().get<int>();
    }

    if (!p.contains("playSecondsGameplay") || !p["playSecondsGameplay"].is_number()) {
        return failState("存档 payload 缺少合法的 playSecondsGameplay 字段");
    }
    s.playSecondsGameplay = p["playSecondsGameplay"].get<double>();
    if (!p.contains("playSecondsSystem") || !p["playSecondsSystem"].is_number()) {
        return failState("存档 payload 缺少合法的 playSecondsSystem 字段");
    }
    s.playSecondsSystem = p["playSecondsSystem"].get<double>();
    // 以下字段自 save_version 2 起存在。v1 档读不到它们是正常的，
    // 保持结构体默认值即可——这正是 v1→v2 迁移之所以是空操作。
    if (p.contains("bottle") && p["bottle"].is_object()) {
        const json& b = p["bottle"];
        s.bottle.owned = b.value("owned", false);
        s.bottle.matureKnown = b.value("matureKnown", false);
        s.bottle.drops = b.value("drops", 0);
        s.bottle.lastChargeDay = b.value("lastChargeDay", 0);
        s.bottle.capacity = b.value("capacity", 3);
    }
    if (p.contains("fields") && p["fields"].is_array()) {
        for (const json& f : p["fields"]) {
            if (!f.is_object()) continue;
            rules::SpiritField field;
            field.id = f.value("id", std::string{});
            if (f.contains("slots") && f["slots"].is_array()) {
                for (const json& sl : f["slots"]) {
                    if (!sl.is_object()) continue;
                    rules::FieldSlot slot;
                    slot.seedId = sl.value("seedId", std::string{});
                    slot.plantedDay = sl.value("plantedDay", 0);
                    slot.age = sl.value("age", 0);
                    slot.ripe = sl.value("ripe", false);
                    // 这次修复之前的存档没有这个字段，读作 0（未标定）：自然
                    // 生长退回全局上限，等玩家下次打开灵田面板时由 FieldScene
                    // 按 data 补齐。不在这里查 data 补——加载器拿不到 GameData，
                    // 硬塞一个默认值反而会把日后调平衡的数据覆盖掉。
                    slot.maxAge = sl.value("maxAge", 0);
                    field.slots.push_back(std::move(slot));
                }
            }
            s.fields.push_back(std::move(field));
        }
    }
    s.alchemyProficiency = p.value("alchemyProficiency", 0);
    s.talismanProficiency = p.value("talismanProficiency", 0);
    s.forgeProficiency = p.value("forgeProficiency", 0);
    s.formationProficiency = p.value("formationProficiency", 0);
    s.aptitude = p.value("aptitude", 50);
    s.cultivationRemainder = p.value("cultivationRemainder", 0);
    // 缺字段的老档落在 0 上，即「还没起课」：下次打开面板从那天重新起算，
    // 不会因为一次升级凭空补上几百天的修为。
    s.lastPracticeDay = p.value("lastPracticeDay", 0);

    // 队伍自 save_version 3 起存在。v2 档读不到它是正常的，保持空队伍即可
    // ——这正是 v2→v3 迁移之所以是空操作。
    //
    // 条目缺 roleId 的直接跳过：一个没有角色 id 的队员在战斗里是个找不到模板
    // 的空位，静默收下只会把一份坏存档的症状推迟到开打那一刻才发作。
    if (p.contains("party") && p["party"].is_array()) {
        for (const json& entry : p["party"]) {
            if (!entry.is_object()) continue;
            core::PartyMember member;
            member.roleId = entry.value("roleId", std::string{});
            if (member.roleId.empty()) continue;
            member.hp = entry.value("hp", -1);
            member.active = entry.value("active", true);
            s.party.push_back(std::move(member));
        }
    }

    // 已习得法术自 save_version 5 起存在。v4 档读不到它是正常的，保持空表即可
    // ——这正是 v4→v5 迁移之所以是空操作。
    //
    // 三种坏数据分三种态度，不是一刀切：
    //   · **数组里混着非字符串** → 判失败。一个数字或对象根本不是法术 id，
    //     静默丢掉只会把一份坏存档的症状推迟到玩家打开战斗菜单那一刻；
    //     与上面 flags 里「值不是整数就报错」是同一条口径。
    //   · **空串** → 跳过。它在 learnMagic 那边本就进不来（第一道闸就是
    //     magicId.empty()），出现在文件里只可能是手改的，丢掉即可。
    //   · **重复 id** → 去重，保留第一次出现的位置。learnedMagics 的语义是
    //     集合（见 core/model/Types.h），而这条不变量是**战斗菜单**在依赖：
    //     重复的 id 会让同一门法术在菜单里出现两行。
    //
    // 去重确实让「手写一份带重复项的 payload 再读回来」不是恒等变换，而往返
    // 测试是这一层最硬的那条断言。代价可接受，因为**带重复项的 payload 写不出来**：
    // toJson 的输入是 GameState，而 learnMagic 幂等，所以经由 API 到达的任何状态
    // 往返都仍是恒等（含顺序）。两条都有测试钉着。
    if (p.contains("learnedMagics")) {
        if (!p["learnedMagics"].is_array()) {
            return failState("存档 payload 的 learnedMagics 不是数组");
        }
        for (const json& entry : p["learnedMagics"]) {
            if (!entry.is_string()) {
                return failState("存档 payload 的 learnedMagics 里有非字符串条目");
            }
            // 走 learnMagic 而不是直接 push_back：去重与「空串不收」这两条规则
            // 只有一个定义处，读档与脚本走的是同一条。
            static_cast<void>(s.learnMagic(entry.get<std::string>()));
        }
    }

    // 剧情给的境界上限自 save_version 6 起存在。老档由 5→6 迁移补上，走到这里时
    // 一定已经有了；**真缺了**（手拼的档、夹具）照老档的口径推一遍，而不是给凡人：
    // 给凡人会把一份炼气三层的档锁死在「连下一层都按不动」上，而那不是它的剧情。
    //
    // 写了却写坏了（不是整数、不是合法境界编号）→ 判失败，与 realm 同一条口径：
    // 篡改过的存档可能把上限改成一个不存在的编号，不能悄悄接受。
    if (p.contains("realmCap")) {
        if (!p["realmCap"].is_number_integer()) {
            return failState("存档 payload 的 realmCap 不是整数");
        }
        const int capValue = p["realmCap"].get<int>();
        const Realm cap = rules::fromValue(capValue);
        if (!rules::isValid(cap)) {
            return failState("存档 payload 的 realmCap 编号非法（可能是手改存档）: " +
                             std::to_string(capValue));
        }
        s.realmCap = cap;
    } else {
        s.realmCap = legacyRealmCap(s.realm, s.flags);
    }

    // 已揭开的破绽自 save_version 7 起存在。v6 档读不到它是正常的，保持空表——
    // 旧档 = 全都不知道（6→7 迁移之所以是空操作，见 SaveFile.h）。
    //
    // 写了却写坏了（不是对象、某项不是数组、数组里有认不出的类别名）→ 判失败，
    // 与 learnedMagics 里混进非字符串同一条口径：悄悄丢掉只会把一份坏存档的症状
    // 推迟到下一场仗——那一格破绽忽然又成了「？」，没人会想到是存档坏了。
    if (p.contains("knownWeaknesses")) {
        const json& known = p["knownWeaknesses"];
        if (!known.is_object()) return failState("存档 payload 的 knownWeaknesses 不是对象");
        for (const auto& entry : known.items()) {
            if (!entry.value().is_array()) {
                return failState("存档 payload 的 knownWeaknesses 里 \"" + entry.key() + "\" 不是数组");
            }
            int mask = 0;
            for (const json& name : entry.value()) {
                const int bit = name.is_string() ? core::categoryFromName(name.get<std::string>()) : 0;
                if (bit == 0) {
                    return failState("存档 payload 的 knownWeaknesses 里 \"" + entry.key() +
                                     "\" 有认不出的类别");
                }
                mask |= bit;
            }
            s.learnWeaknesses(entry.key(), mask);
        }
    }

    // 野外遭遇的计数器自 save_version 8 起存在；v7 档读不到它是正常的，保持全 0（从没遇过）。
    // 写了却写坏了（不是对象、某项不是整数）→ 判失败，与 knownWeaknesses 同一条口径。
    if (p.contains("encounter")) {
        const json& counter = p["encounter"];
        if (!counter.is_object()) return failState("存档 payload 的 encounter 不是对象");
        const std::array<std::pair<const char*, int*>, 4> fields = {{
            {"stepsSinceLast", &s.encounter.stepsSinceLast},
            {"stepsUntilNext", &s.encounter.stepsUntilNext},
            {"triggeredToday", &s.encounter.triggeredToday},
            {"lastDay", &s.encounter.lastDay},
        }};
        for (const auto& [name, slot] : fields) {
            if (!counter.contains(name) || !counter[name].is_number_integer()) {
                return failState(std::string("存档 payload 的 encounter 缺少整数字段 ") + name);
            }
            *slot = counter[name].get<int>();
        }
    }

    return core::Result<GameState>::success(std::move(s));
}

// ---- 迁移表 ----
//
// key 是迁移的起始版本：从 v(key) 的 payload 转出 v(key+1) 的 payload。P1 只有
// v1，所以这张表现在是空的——特意留出这个结构，是因为「P1 之后每次 schema 变
// 更都要能加迁移函数」是任务硬要求。下次改 GameState 的可持久化字段时，流程
// 应该是：kSaveVersion 加一，在这里补一条 {旧版本, 迁移函数}，toJson/fromJson
// 只认最新 schema，不要为了兼容旧存档反过来把旧字段揉进新代码里。
using MigrationFn = core::Result<json> (*)(const json& payloadAtThisVersion);

const std::map<int, MigrationFn>& migrations() {
    static const std::map<int, MigrationFn> kTable{
        // 1 -> 2：加入掌天瓶、灵田、四艺熟练度、资质、打坐余数。
        // v1 档里这些字段根本不存在，fromJson 读不到就保持结构体默认值，
        // 所以迁移本身无事可做。仍然登记这一条：空操作与「忘了写」在
        // 代码里长得一样，留个显式记录才能分辨。
        {1, [](const json& payload) { return core::Result<json>::success(payload); }},
        // 2 -> 3：加入队伍（GameState::party）。v2 档里没有 party 字段，
        // fromJson 读不到就保持空队伍，所以迁移本身同样无事可做。
        //
        // 仍然登记这一条。第 2 章已经证明「空操作」与「忘了登记」在行为上天差
        // 地别：后者会让 while 循环在 migrations().find(2) 上落空，于是每一份
        // 老玩家的存档都被判成「无法识别的存档版本号」，一局都读不回来。
        {2, [](const json& payload) { return core::Result<json>::success(payload); }},
        // 3 -> 4：maxHp / maxMp 改由境界给出基准。**本作第一条真的做事的迁移。**
        //
        // v3 及更早的档里，这两个数从第 1 章的 10 / 0 一路原样带到存档那一刻
        // ——全作没有任何一处让它们增长。所以一份已经练到炼气三层的老档读进来
        // 必须当场拿到那一档该有的 60 / 30，否则老玩家修好之后仍然停在 10 点，
        // 而这正是这次要修的缺陷本身。
        //
        // 为什么落在迁移，而不是「每次读档都按境界重算」：
        //   * 重算会让 fromJson 不再是往返的恒等变换。存档往返测试是这一层最硬
        //     的那条断言（写下去什么，读回来就是什么），把它换成「读回来的可能
        //     和写下去的不一样」，往后每一个字段的往返用例都要先想一想自己有没有
        //     被这条规则改过。
        //   * 更要紧的是，重算会**每次读档都发生**。日后丹药、功法、装备给的任何
        //     一点上限都要先与境界基准较量一次；写成「只补不削」它当然不会被削掉，
        //     但那条规则从此长在读档路径上，谁都得记着它。迁移只在版本关口跑一次，
        //     跑完这份档就是一份普通的 v4 档。
        //   * 而 v4 档不会再走散：改 realm 的地方全作只有两处（这里，与
        //     game/CultivationScene.cpp 的突破），后者当场就把属性补齐了。
        //
        // **只补不削**（rules::liftToFloor）：上限已经高于基准就一个字节也不动。
        // 补上去的那一截当作根基，当前值同步抬高相同的量——只抬上限的话，一份
        // 满血的老档读进来会变成 10/60，玩家一进战斗就死。
        //
        // 读不到或读坏了 realm 就原样放行：这条迁移不是校验器，字段的合法性由
        // fromJson 统一把关（它会报出「realm 编号非法（可能是手改存档）」这种
        // 贴着真实原因的错）。在这里抢着报错只会让同一件事有两句不同的说法。
        {3, [](const json& payload) {
             json next = payload;
             if (!next.is_object() || !next.contains("realm") ||
                 !next["realm"].is_number_integer()) {
                 return core::Result<json>::success(std::move(next));
             }
             const Realm realm = rules::fromValue(next["realm"].get<int>());
             if (!rules::isValid(realm)) return core::Result<json>::success(std::move(next));

             const auto readInt = [&next](const char* key, int fallback) {
                 return next.contains(key) && next[key].is_number_integer()
                            ? next[key].get<int>()
                            : fallback;
             };
             const rules::Vitals hp = rules::liftToFloor(
                 {readInt("hp", 0), readInt("maxHp", 0)}, rules::realmMaxHp(realm));
             const rules::Vitals mp = rules::liftToFloor(
                 {readInt("mp", 0), readInt("maxMp", 0)}, rules::realmMaxMp(realm));
             next["hp"] = hp.current;
             next["maxHp"] = hp.max;
             next["mp"] = mp.current;
             next["maxMp"] = mp.max;
             return core::Result<json>::success(std::move(next));
         }},
        // 4 -> 5：加入已习得法术（GameState::learnedMagics）。v4 档里没有
        // learnedMagics 字段，fromJson 读不到就保持空表，所以迁移本身无事可做。
        //
        // 仍然登记这一条，理由与 1→2、2→3 一字不差：**空操作与「忘了登记」在
        // 代码里长得一样**，而行为上天差地别——后者会让 while 循环在
        // migrations().find(4) 上落空，于是每一份第 3 章玩家的存档都被判成
        // 「无法识别的存档版本号」，一局都读不回来。
        //
        // 「空表」在这里恰好也是正确的语义：v4 那会儿韩立一门法术都不会
        // （技术债 G-4），所以不补正是对的，不是偷懒。
        {4, [](const json& payload) { return core::Result<json>::success(payload); }},
        // 5 -> 6：加入剧情给的境界上限（GameState::realmCap）。**真的做事**：
        // 按 legacyRealmCap 的口径（max(当前境界, 旗标推出的上限)）补上。
        //
        // 与 3→4 同一个态度：realm 或 flags 读不到、读坏了就原样放行，合法性由
        // fromJson 统一把关——在这里抢着报错只会让同一件事有两句不同的说法。
        // 旗标里混着非整数的那几项这里跳过不算（fromJson 会为它报错）。
        {5, [](const json& payload) {
             json next = payload;
             if (!next.is_object() || !next.contains("realm") ||
                 !next["realm"].is_number_integer()) {
                 return core::Result<json>::success(std::move(next));
             }
             const Realm realm = rules::fromValue(next["realm"].get<int>());
             if (!rules::isValid(realm)) return core::Result<json>::success(std::move(next));

             std::map<std::string, int> flags;
             if (next.contains("flags") && next["flags"].is_object()) {
                 for (const auto& item : next["flags"].items()) {
                     if (item.value().is_number_integer()) {
                         flags[item.key()] = item.value().get<int>();
                     }
                 }
             }
             next["realmCap"] = rules::toValue(legacyRealmCap(realm, flags));
             return core::Result<json>::success(std::move(next));
         }},
        // 6 -> 7：加入已揭开的破绽（GameState::knownWeaknesses，八方旅人化改造）。
        // v6 档里没有这一项，fromJson 读不到就保持空表——**旧档 = 全都不知道**，
        // 那会儿战斗里还没有破绽这回事，所以不补正是对的。
        //
        // 仍然登记这一条，理由与 1→2、2→3、4→5 一字不差：空操作与「忘了登记」在
        // 代码里长得一样，而后者会让每一份第 5 章之前的存档都被判成「无法识别的存档
        // 版本号」——tests/fixtures/ 下那六份章末存档全是 v6，一份也读不回来。
        {6, [](const json& payload) { return core::Result<json>::success(payload); }},
        // 7 -> 8：加入野外遭遇的计数器（GameState::encounter）。v7 档里没有这一项，fromJson
        // 读不到就保持全 0——**旧档 = 从没遇过**，那会儿野外还没有遭遇，所以不补正是对的。
        // 仍然登记，理由与 6→7 一字不差：空操作与「忘了登记」在代码里长得一样。
        {7, [](const json& payload) { return core::Result<json>::success(payload); }},
    };
    return kTable;
}

core::Result<bool> saveGameImpl(const GameState& state, const std::string& pathStr) {
    const fs::path path(pathStr);
    const json payload = toJson(state);
    json root;
    root["save_version"] = kSaveVersion;
    root["checksum"] = computeChecksum(kSaveVersion, payload);
    root["payload"] = payload;

    if (path.has_parent_path()) {
        // 目录不存在就建一个：存档路径通常是 saves/slot1.json 这种，调用方不必
        // 先自己 mkdir。建不出来（比如权限问题）不当场报错，让下面打开文件失败
        // 时统一报错，错误信息更贴近真实原因（"打不开文件" 比 "建不出目录" 更
        // 直接对应用户能做的事）。
        std::error_code dirEc;
        fs::create_directories(path.parent_path(), dirEc);
    }

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream.is_open()) {
        return core::Result<bool>::failure("无法写入存档文件: " + path.string());
    }
    stream << root.dump(2);
    stream.flush();
    if (!stream.good()) {
        return core::Result<bool>::failure("写入存档文件失败: " + path.string());
    }
    return core::Result<bool>::success(true);
}

core::Result<GameState> loadGameImpl(const std::string& pathStr) {
    const fs::path path(pathStr);
    std::error_code existsEc;
    if (!fs::exists(path, existsEc)) {
        return failState("存档文件不存在: " + path.string());
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream.is_open()) {
        return failState("无法打开存档文件: " + path.string());
    }

    json root = json::parse(stream, /*callback*/ nullptr, /*allow_exceptions*/ false);
    if (root.is_discarded()) {
        return failState("存档 JSON 语法错误: " + path.string());
    }
    if (!root.is_object() || !root.contains("save_version") || !root["save_version"].is_number_integer() ||
        !root.contains("checksum") || !root["checksum"].is_string() || !root.contains("payload") ||
        !root["payload"].is_object()) {
        return failState("存档格式不完整（缺 save_version / checksum / payload 之一）: " + path.string());
    }

    const int version = root["save_version"].get<int>();
    const std::string storedChecksum = root["checksum"].get<std::string>();
    json payload = root["payload"];

    // 先核对校验和，再做版本迁移：校验和保护的是「文件里实际写着的那份
    // payload」，迁移只是读进来之后的加工步骤，顺序不能反过来，否则被改过的
    // payload 有可能在迁移过程中被"洗白"。
    if (computeChecksum(version, payload) != storedChecksum) {
        return failState("存档校验和不匹配，可能被手动修改过: " + path.string());
    }

    if (version < 1) {
        return failState("存档版本号非法: " + std::to_string(version));
    }
    if (version > kSaveVersion) {
        return failState("存档版本 v" + std::to_string(version) + " 高于当前游戏支持的 v" +
                          std::to_string(kSaveVersion) + "，请升级游戏后再读取");
    }

    int currentVersion = version;
    while (currentVersion < kSaveVersion) {
        const auto it = migrations().find(currentVersion);
        if (it == migrations().end()) {
            return failState("无法识别的存档版本号 v" + std::to_string(currentVersion) +
                              "：找不到升级到下一版本的迁移函数");
        }
        core::Result<json> migrated = it->second(payload);
        if (!migrated) return failState(migrated.error);
        payload = std::move(migrated.value);
        ++currentVersion;
    }

    return fromJson(payload);
}

}  // namespace

Realm legacyRealmCap(Realm realm, const std::map<std::string, int>& flags) {
    const auto flagOf = [&flags](const char* name) {
        const auto it = flags.find(name);
        return it == flags.end() ? 0 : it->second;
    };
    std::int32_t cap = rules::toValue(Realm::Mortal);
    // 授完口诀：他从这时起才可能有「第一层」（第 1 章节点 9；ch02-design 第 2 节，
    // 段四才到第二层）。
    if (flagOf("ch01.koujue_received") != 0) {
        cap = std::max(cap, rules::toValue(Realm::QiRefining1));
    }
    // 第 2 章剧情说到第几层：ceng2.lua 置 2、ceng3.lua 置 3。夹在炼气期之内——
    // 这个旗标的语义就是「口诀第几层」，一个手改成 99 的值不该把上限推进筑基。
    const int ceng = flagOf("ch02.koujue_ceng");
    if (ceng >= 1) {
        cap = std::max(cap, std::min(ceng, rules::toValue(Realm::QiRefining13)));
    }
    // 当前境界兜底：上限永远不低于他已经在的那一层。低于的话，面板上会是一句
    // 「卡住了」挂在一个他早就越过去的地方——而第 4 章那三次升境正是这样被覆盖的。
    cap = std::max(cap, rules::toValue(realm));
    return rules::fromValue(cap);
}

// 公开入口用 try/catch 兜底：fromJson/toJson 里几乎每个字段都先判过类型再
// .get<>()，理论上不会再抛，但 json::dump()/操作符在极端输入下仍可能抛
// out_of_range 等异常；不跨模块边界抛异常是硬约束（docs/interfaces.md 0 节），
// 这层兜底比在几十个取值点全部套 try/catch 更省代码、也更不容易漏。
core::Result<bool> saveGame(const core::GameState& state, const std::string& path) {
    try {
        return saveGameImpl(state, path);
    } catch (const std::exception& e) {
        return core::Result<bool>::failure("保存存档时出现未预期的异常: " + path + ": " + e.what());
    }
}

core::Result<core::GameState> loadGame(const std::string& path) {
    try {
        return loadGameImpl(path);
    } catch (const std::exception& e) {
        return core::Result<core::GameState>::failure("读取存档时出现未预期的异常: " + path + ": " + e.what());
    }
}

}  // namespace fanren::io
