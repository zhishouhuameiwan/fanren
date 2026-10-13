// 第 11 章挂点、门与章末（只读数据 / 扫脚本 / 读地图那一半）。对应 docs/ch11-design.md 第 15 节验收：
//   第 3 条  挂点 mode / once / guard / set_flag 与 3.1 表一致；无 auto；六处循环挂点 once=false；同格换触发器
//            三组按 18.1 第 1 条落成相邻格；facility_cunji_39 与 spawn men 相邻、n21 不与 spawn 同格。
//   第 16 条 必打 4 场的调用点与目标链（只扫脚本 + 读目标链；通关两侧实打的编成归通关用例）。
//   第 17 条 只有四对门、礁岛无门、不通第 10 章（读地图那一半；切片归切片用例）。
//   第 21 条 zhuzhong.lua 最后置 ch11.done、不 ending()；zhifadui.lua 与第 10 章集成版只差末尾一行；
//            第 11 章是章节表最后一章（读数据那一半；headless 卡片队列归 Ch10Acceptance 与第 11 章通关）。
// 每一条判据都有「在内存里改坏 → 必须报」的负向（Mutations*）。
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <regex>
#include "Ch11Data.h"

namespace {
using namespace fanren::test::ch11;
using Maps = std::map<std::string, Json>;
using Errors = std::vector<std::string>;

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

Maps shippedMaps() {
    Maps maps;
    for (const char* id : kMaps) maps[id] = json(root() / "maps" / (std::string(id) + ".tmj"));
    return maps;
}

Json* findObject(Json& map, const std::string& name) {
    for (auto& layer : map.at("layers")) {
        if (layer.value("type", "") != "objectgroup") continue;
        for (auto& object : layer.at("objects")) if (object.at("name") == name) return &object;
    }
    return nullptr;
}

void setProp(Json& map, const std::string& name, const std::string& key, const Json& value) {
    Json* object = findObject(map, name);
    ASSERT_NE(object, nullptr) << name;
    for (auto& p : (*object)["properties"]) if (p.at("name") == key) { p["value"] = value; return; }
    (*object)["properties"].push_back({{"name", key}, {"type", value.is_boolean() ? "bool" : "string"}, {"value", value}});
}

void moveObject(Json& map, const std::string& name, Cell cell) {
    Json* object = findObject(map, name);
    ASSERT_NE(object, nullptr) << name;
    (*object)["x"] = cell.x * map.at("tilewidth").get<int>();
    (*object)["y"] = cell.y * map.at("tileheight").get<int>();
}

// ---- 第 3 条：挂点表 ----
Errors hookErrors(const Maps& maps) {
    Errors out;
    std::set<std::string> seen;
    for (const auto& hook : kTriggers) {
        const auto all = objects(maps.at(hook.map));
        const auto found = all.find(hook.object);
        if (found == all.end()) { out.push_back(std::string("missing ") + hook.map + "/" + hook.object); continue; }
        const Json p = properties(found->second);
        const auto check = [&](bool ok, const char* field) { if (!ok) out.push_back(std::string(hook.object) + " " + field); };
        check(typeOf(found->second) == "trigger", "type");
        check(p.value("mode", "") == std::string(hook.enter ? "enter" : "interact"), "mode");
        check(p.contains("once") && p.at("once").is_boolean() && p.at("once").get<bool>() == hook.once, "once");
        check(p.value("guard_flag", "") == hook.guard, "guard_flag");
        check(p.value("set_flag", "") == hook.done, "set_flag");
        check(!hook.once ? !p.contains("set_flag") : true, "repeatable has set_flag");
        check(hook.once ? std::string(hook.done) != "" : true, "once without set_flag");
        check(p.value("script", "") == "ch11/" + std::string(hook.script) + ".lua", "script");
        seen.insert(std::string(hook.map) + "/" + hook.object);
    }
    for (const auto& npc : kNpcHooks) {
        const auto all = objects(maps.at(npc.map));
        const auto found = all.find(npc.object);
        if (found == all.end()) { out.push_back(std::string("missing ") + npc.map + "/" + npc.object); continue; }
        const Json p = properties(found->second);
        if (typeOf(found->second) != "npc") out.push_back(std::string(npc.object) + " type");
        if (p.value("script", "") != "ch11/" + std::string(npc.script) + ".lua") out.push_back(std::string(npc.object) + " script");
        if (p.value("visible_flag", "") != npc.visible) out.push_back(std::string(npc.object) + " visible_flag");
        if (p.contains("once") || p.contains("set_flag")) out.push_back(std::string(npc.object) + " npc carries once/set_flag");
    }
    for (const auto& [mapId, map] : maps) {
        for (const auto& [name, object] : objects(map)) {
            const Json p = properties(object);
            if (p.value("mode", "") == "auto") out.push_back(mapId + "/" + name + " mode auto");
            if (typeOf(object) == "trigger" && !seen.count(mapId + "/" + name))
                out.push_back("undocumented trigger " + mapId + "/" + name);
        }
    }
    return out;
}

TEST(Ch11TriggerMode, EveryHookMatchesTheDesignTableAndTheLandedDeviations) {
    const auto maps = shippedMaps();
    const auto errors = hookErrors(maps);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    // 3.1 与验收 3 点名的六处循环挂点：五个 trigger once=false，xingkong 挂在 NPC 上（无 once）。
    int repeatable = 0;
    for (const auto& hook : kTriggers) if (!hook.once) ++repeatable;
    EXPECT_EQ(repeatable, 5);
    EXPECT_EQ(kTriggers.size() + kNpcHooks.size(), 34u) << "先验：34 个脚本各有一个挂点（施工图 18.3 第 9 条）";
    EXPECT_EQ(scripts().size(), 34u);
}

TEST(Ch11TriggerMode, MutationsOfModeOnceGuardSetFlagScriptOrAnAutoHookAreCaught) {
    const auto original = shippedMaps();
    ASSERT_TRUE(hookErrors(original).empty()) << joined(hookErrors(original));
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    const std::vector<Bad> bads{
        {"ch11_dongfu", "trigger_zhuzhan", "mode", "enter"},            // n21 必须 interact
        {"ch11_honghu", "trigger_youyao", "once", true},                // 循环挂点
        {"ch11_dongfu", "trigger_lianjian", "set_flag", ""},            // n30a 必须置 lianjian_qian
        {"ch11_ningcui_jie", "trigger_huicheng_chuansong", "set_flag", "ch11.chuansong"},
        {"ch11_yinsha_dao", "trigger_daoge", "guard_flag", "ch11.guiju"},
        {"ch11_dongfu", "trigger_biguan", "script", "ch11/liandan.lua"},
        {"ch11_xingkong", "npc_xingkong_xiushi", "once", false},
        {"ch11_dongfu", "npc_jiedan_linju_b", "script", "ch11/linju_z1.lua"},
        {"ch11_fangshi", "facility_cunji_fangshi", "mode", "auto"},
    };
    for (const auto& bad : bads) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(hookErrors(maps).empty()) << bad.object << " " << bad.key;
    }
    auto extra = original;
    auto copy = *findObject(extra["ch11_honghu"], "trigger_youyao");
    copy["name"] = "trigger_youyao_er";
    for (auto& layer : extra["ch11_honghu"]["layers"])
        if (layer.value("type", "") == "objectgroup") { layer["objects"].push_back(copy); break; }
    EXPECT_FALSE(hookErrors(extra).empty()) << "表外多出一个触发器必须报";
}

// ---- 第 3 条后半：同格、相邻与存档点 ----
bool walkable(const Json& map, Cell c) {
    const int w = map.at("width").get<int>(), h = map.at("height").get<int>();
    if (c.x < 0 || c.y < 0 || c.x >= w || c.y >= h) return false;
    for (const auto& layer : map.at("layers"))
        if (layer.value("name", "") == "collision") return layer.at("data").at(static_cast<std::size_t>(c.y * w + c.x)) == 0;
    return false;
}

Cell at(const Json& map, const char* name) {
    const auto all = objects(map);
    return cellOf(map, all.at(name));
}

int manhattan(Cell a, Cell b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

Errors layoutErrors(const Maps& maps) {
    Errors out;
    // 引擎对一格只取第一个 trigger（18.1 第 1 条）：七张图上没有两个 trigger 同格。
    for (const auto& [mapId, map] : maps) {
        std::map<Cell, std::string> taken;
        for (const auto& [name, object] : objects(map)) {
            if (typeOf(object) != "trigger") continue;
            const auto [it, fresh] = taken.emplace(cellOf(map, object), name);
            if (!fresh) out.push_back(mapId + " " + name + " shares a cell with " + it->second);
        }
    }
    const auto& honghu = maps.at("ch11_honghu");
    if (at(honghu, "trigger_buzhen") == at(honghu, "trigger_huicheng")) out.push_back("honghu landing pair on one cell");
    {   // interact 挂在礁石上：四邻至少一格站得住，才面朝得到它。
        const Cell h = at(honghu, "trigger_huicheng");
        if (!walkable(honghu, {h.x + 1, h.y}) && !walkable(honghu, {h.x - 1, h.y}) &&
            !walkable(honghu, {h.x, h.y + 1}) && !walkable(honghu, {h.x, h.y - 1}))
            out.push_back("honghu trigger_huicheng cannot be faced from any cell");
    }
    // 礁岛：spawn haishang (13,19) → guiju (13,18) → wuchou (13,17) → daoge (13,16)，一格宽、绕不过。
    const auto& dao = maps.at("ch11_yinsha_dao");
    const Cell expected[] = {{13, 19}, {13, 18}, {13, 17}, {13, 16}};
    const char* names[] = {"haishang", "trigger_guiju", "trigger_wuchou", "trigger_daoge"};
    for (int i = 0; i < 4; ++i)
        if (!(at(dao, names[i]) == expected[i])) out.push_back(std::string("yinsha_dao ") + names[i] + " cell");
    for (int y = 16; y <= 18; ++y)
        if (walkable(dao, {12, y}) || walkable(dao, {14, y})) out.push_back("yinsha_dao path is wider than one cell at y=" + std::to_string(y));
    // 炼器台：trigger_lianjian 在台前一格，台只从这一格够得着。
    const auto& dongfu = maps.at("ch11_dongfu");
    const Cell forge = at(dongfu, "facility_lianqi_39");
    const Cell before = at(dongfu, "trigger_lianjian");
    if (manhattan(forge, before) != 1) out.push_back("trigger_lianjian not beside the forge");
    for (const Cell n : {Cell{forge.x + 1, forge.y}, Cell{forge.x - 1, forge.y}, Cell{forge.x, forge.y + 1}, Cell{forge.x, forge.y - 1}})
        if (!(n == before) && walkable(dongfu, n)) out.push_back("forge reachable without crossing trigger_lianjian");
    // 存档点与 spawn men 相邻；n21 触发格不与 spawn 同格。
    const Cell men = at(dongfu, "men");
    if (manhattan(at(dongfu, "facility_cunji_39"), men) != 1) out.push_back("facility_cunji_39 not beside spawn men");
    if (at(dongfu, "trigger_zhuzhan") == men) out.push_back("trigger_zhuzhan on spawn men");
    // 密室门外 n17 与 n30b 不同格。
    if (at(dongfu, "trigger_chuguan") == at(dongfu, "trigger_chujian")) out.push_back("chuguan and chujian share a cell");
    return out;
}

TEST(Ch11TriggerMode, SameCellSwapsLandedAsAdjacentCellsAndTheSaveSpotSitsByTheDoor) {
    const auto errors = layoutErrors(shippedMaps());
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch11TriggerMode, MutationsOfTheLayoutAreCaught) {
    const auto original = shippedMaps();
    ASSERT_TRUE(layoutErrors(original).empty());
    struct Move { const char* map; const char* object; Cell to; };
    for (const auto& move : {Move{"ch11_honghu", "trigger_huicheng", {4, 14}},
                             Move{"ch11_yinsha_dao", "trigger_wuchou", {13, 15}},
                             Move{"ch11_dongfu", "trigger_lianjian", {7, 17}},
                             Move{"ch11_dongfu", "facility_cunji_39", {14, 20}},
                             Move{"ch11_dongfu", "trigger_zhuzhan", {16, 20}},
                             Move{"ch11_dongfu", "trigger_chujian", {11, 15}}}) {
        auto maps = original;
        moveObject(maps[move.map], move.object, move.to);
        EXPECT_FALSE(layoutErrors(maps).empty()) << move.object;
    }
}

// ---- 第 17 条：门 ----
using Pair = std::pair<std::string, std::string>;
Errors doorErrors(const Maps& maps) {
    Errors out;
    std::multiset<Pair> crossing;
    int sameMap = 0;
    // 18.1 第 2 条：五道闸门的 require_flag 与按规则 20 命名的 deny key。
    const std::map<std::string, Pair> gates{
        {"portal_shangshan", {"ch11.zufu", "ch11.block.dongfu"}},
        {"portal_xingkong", {"ch11.nichang", "ch11.block.xingkong"}},
        {"portal_xiaoxuan", {"ch11.kaidian", "ch11.block.xiaoxuan"}},
        {"portal_chaguan", {"ch11.lianer", "ch11.block.fangshi"}},
        {"portal_chuhai", {"ch11.ningcui", "ch11.block.honghu"}}};
    int gated = 0;
    const std::set<std::string> ours(kMaps.begin(), kMaps.end());
    for (const auto& [mapId, map] : maps) {
        for (const auto& [name, object] : objects(map)) {
            const std::string type = typeOf(object);
            if (type == "encounter") out.push_back(mapId + "/" + name + " encounter zone");
            if (type != "portal") continue;
            const Json p = properties(object);
            const std::string target = p.value("target_map", "");
            if (!ours.count(target)) out.push_back(mapId + "/" + name + " leads out of chapter 11 to " + target);
            if (target == mapId) ++sameMap; else crossing.emplace(mapId, target);
            const auto gate = gates.find(name);
            if (gate != gates.end()) {
                ++gated;
                if (p.value("require_flag", "") != gate->second.first) out.push_back(name + " require_flag");
                if (p.value("deny_text_key", "") != gate->second.second) out.push_back(name + " deny_text_key");
            } else if (p.contains("require_flag")) {
                out.push_back(mapId + "/" + name + " has an undesigned require_flag");
            }
        }
    }
    const std::multiset<Pair> expected{
        {"ch11_fangshi", "ch11_dongfu"}, {"ch11_dongfu", "ch11_fangshi"},
        {"ch11_dongfu", "ch11_xingkong"}, {"ch11_xingkong", "ch11_dongfu"},
        {"ch11_ningcui_jie", "ch11_honghu"}, {"ch11_honghu", "ch11_ningcui_jie"},
        {"ch11_fangshi", "ch11_xiaoxuan"}, {"ch11_xiaoxuan", "ch11_fangshi"}};
    if (crossing != expected) out.push_back("cross-map door pairs differ from the four designed pairs");
    if (sameMap != 2) out.push_back("tea house same-map pair count " + std::to_string(sameMap));
    if (gated != 5) out.push_back("gated portals " + std::to_string(gated));
    return out;
}

TEST(Ch11TriggerMode, OnlyTheFourDoorPairsExistTheReefHasNoneAndNothingLeadsBackToChapterTen) {
    const auto maps = shippedMaps();
    const auto errors = doorErrors(maps);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    for (const auto& [name, object] : objects(maps.at("ch11_yinsha_dao")))
        EXPECT_NE(typeOf(object), "portal") << "礁岛无门：" << name;
}

TEST(Ch11TriggerMode, MutationsOfDoorsAreCaught) {
    const auto original = shippedMaps();
    ASSERT_TRUE(doorErrors(original).empty());
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    for (const auto& bad : {Bad{"ch11_dongfu", "portal_xiashan", "target_map", "ch10_xiaohuan"},
                            Bad{"ch11_fangshi", "portal_shangshan", "require_flag", "ch11.luer"},
                            Bad{"ch11_fangshi", "portal_chaguan", "deny_text_key", "ch11.fangshi.chaguan_deny"},
                            Bad{"ch11_xiaoxuan", "portal_chudian", "require_flag", "ch11.kaidian"},
                            Bad{"ch11_honghu", "portal_huijie", "target_map", "ch11_xingkong"}}) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(doorErrors(maps).empty()) << bad.object << " " << bad.key;
    }
}

// ---- 第 16 条：必打 4 场绕不过去（扫脚本 + 读目标链）----
using Sources = std::map<std::string, std::string>;

Errors battleCallErrors(const Sources& sources) {
    Errors out;
    // 施工图 8.2 / 验收 16：① ② 由 youyao.lua 各一处，③ guiju.lua、④ daoge.lua 各一处；本章没有别的 battle(。
    const std::map<std::string, std::string> owner{
        {"b11_ningcuidao_youyao", "youyao"}, {"b11_honghu_jiguanjiao", "youyao"},
        {"b11_yinsha_shayao", "guiju"}, {"b11_tiandu_shi", "daoge"}};
    const std::regex anyCall(R"lua(\bbattle\s*\()lua");
    const std::regex named(R"lua(\bbattle\s*\(\s*"([^"]+)")lua");
    int total = 0;
    std::map<std::string, int> calls;
    for (const auto& [name, source] : sources) {
        const std::string code = executableLines(source);
        total += countMatches(code, anyCall);
        for (std::sregex_iterator it(code.begin(), code.end(), named), end; it != end; ++it) {
            const std::string id = (*it)[1].str();
            ++calls[id];
            const auto expected = owner.find(id);
            if (expected == owner.end()) out.push_back(name + " calls undesigned battle " + id);
            else if (expected->second != name) out.push_back(id + " called from " + name);
        }
    }
    for (const auto& [id, script] : owner)
        if (calls[id] != 1) out.push_back(id + " called " + std::to_string(calls[id]) + " times");
    if (total != 4) out.push_back("battle( occurs " + std::to_string(total) + " times in chapter 11");
    return out;
}

TEST(Ch11TriggerMode, TheFourMandatoryBattlesHaveOneCallSiteEachAndSitOnTheObjectiveChain) {
    const auto errors = battleCallErrors(scripts());
    EXPECT_TRUE(errors.empty()) << joined(errors);
    // ①（循环挂点）在目标链上；③④ 的 set_flag 在目标链上且是下一步的门闸：③ 置 ch11.guiju → n23 guard；
    // ④ 置 ch11.yaoshi → n25 紫灵 visible_flag。② 是 ① 的分支、不在链上（不登目标链）。
    const auto steps = json(root() / "data" / "objectives" / "ch11.json").at("steps");
    ASSERT_EQ(steps.size(), 29u) << "施工图 3.4：目标链 29 步";
    const auto stepOf = [&](const std::string& done) -> int {
        for (std::size_t i = 0; i < steps.size(); ++i) if (steps[i].at("done_flag") == done) return static_cast<int>(i);
        return -1;
    };
    const auto maps = shippedMaps();
    struct Gate { const char* done; const char* object; const char* nextObject; const char* nextKey; };
    ASSERT_GE(stepOf("ch11.lieyao"), 0);
    EXPECT_EQ(steps[static_cast<std::size_t>(stepOf("ch11.lieyao"))].at("target_object"), "trigger_youyao");
    for (const auto& gate : {Gate{"ch11.guiju", "trigger_guiju", "trigger_wuchou", "guard_flag"},
                             Gate{"ch11.yaoshi", "trigger_daoge", "npc_zi_ling", "visible_flag"}}) {
        const int index = stepOf(gate.done);
        ASSERT_GE(index, 0) << gate.done;
        EXPECT_EQ(steps[static_cast<std::size_t>(index)].at("target_object"), gate.object);
        ASSERT_LT(index + 1, static_cast<int>(steps.size()));
        const auto& next = steps[static_cast<std::size_t>(index + 1)];
        EXPECT_EQ(next.at("target_object"), gate.nextObject) << gate.done;
        const auto all = objects(maps.at(next.at("target_map").get<std::string>()));
        EXPECT_EQ(properties(all.at(gate.nextObject)).value(gate.nextKey, ""), gate.done) << gate.nextObject;
    }
    EXPECT_EQ(stepOf("ch11.jiguan"), -1) << "n13 不进目标链";
}

TEST(Ch11TriggerMode, AnExtraOrMovedBattleCallIsCaught) {
    const auto original = scripts();
    ASSERT_TRUE(battleCallErrors(original).empty());
    auto extra = original;
    extra["wuchou"] += "\nbattle(\"b11_yinsha_shayao\")\n";
    EXPECT_FALSE(battleCallErrors(extra).empty());
    auto stranger = original;
    stranger["buzhen"] += "\nlocal won = battle(\"b10_zhifadui\")\n";
    EXPECT_FALSE(battleCallErrors(stranger).empty());
    auto moved = original;
    const auto at = moved["daoge"].find("battle(\"b11_tiandu_shi\")");
    ASSERT_NE(at, std::string::npos);
    moved["daoge"].replace(at, 6, "fight");
    moved["zhuzhan"] += "\nbattle(\"b11_tiandu_shi\")\n";
    EXPECT_FALSE(battleCallErrors(moved).empty());
    auto commented = original;
    commented["kuilei"] += "\n-- battle(\"b11_tiandu_shi\")\n";
    EXPECT_TRUE(battleCallErrors(commented).empty()) << "注释里的不算";
}

// ---- 第 21 条：章末与开局（读数据 / 扫脚本那一半）----
std::vector<std::string> liveLines(const std::string& source) {
    std::vector<std::string> out;
    std::istringstream in(executableLines(source));
    for (std::string line; std::getline(in, line);) {
        line.erase(0, line.find_first_not_of(" \t"));
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();
        if (!line.empty()) out.push_back(line);
    }
    return out;
}

// 第 10 章集成版 scripts/ch10/zhifadui.lua（HEAD 68aa3f7）：633 字节，FNV-1a 64 = 0x1cbd3fc84917131e。
constexpr std::size_t kChapterTenFinaleBytes = 633;
constexpr std::uint64_t kChapterTenFinaleHash = 0x1cbd3fc84917131eull;

Errors finaleErrors(const std::string& zhuzhong, const std::string& zhifadui, const Json& chapters) {
    Errors out;
    const auto lines = liveLines(zhuzhong);
    if (lines.empty() || lines.back() != "flag.set(\"ch11.done\")") out.push_back("zhuzhong.lua does not end with ch11.done");
    if (executableLines(zhuzhong).find("ending(") != std::string::npos) out.push_back("zhuzhong.lua calls ending()");
    const std::string patch = "teleport(\"ch11_fangshi\")\n";
    if (zhifadui.size() != kChapterTenFinaleBytes + patch.size() ||
        zhifadui.compare(kChapterTenFinaleBytes, std::string::npos, patch) != 0) {
        out.push_back("zhifadui.lua is not the chapter-10 file plus exactly one closing teleport line");
    } else if (fnv1a(zhifadui.substr(0, kChapterTenFinaleBytes)) != kChapterTenFinaleHash) {
        out.push_back("zhifadui.lua changed before the patched line");
    }
    const auto& table = chapters.at("chapters");
    if (table.empty() || table.back().at("number") != 11 || table.back().at("done_flag") != "ch11.done")
        out.push_back("chapter 11 is not the last chapter of data/chapters.json");
    return out;
}

TEST(Ch11TriggerMode, TheChapterEndsOnTheDoneFlagAndChapterTenHandsOverWithOneLine) {
    const auto errors = finaleErrors(read(root() / "scripts" / "ch11" / "zhuzhong.lua"),
                                     read(root() / "scripts" / "ch10" / "zhifadui.lua"),
                                     json(root() / "data" / "chapters.json"));
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch11TriggerMode, MutationsOfTheFinaleAreCaught) {
    const std::string zhuzhong = read(root() / "scripts" / "ch11" / "zhuzhong.lua");
    const std::string zhifadui = read(root() / "scripts" / "ch10" / "zhifadui.lua");
    const Json chapters = json(root() / "data" / "chapters.json");
    ASSERT_TRUE(finaleErrors(zhuzhong, zhifadui, chapters).empty());
    EXPECT_FALSE(finaleErrors(zhuzhong + "ending(\"t\", \"b\")\n", zhifadui, chapters).empty());
    EXPECT_FALSE(finaleErrors(zhuzhong + "talk(\"\", \"ch11.zhuzhong.after\")\n", zhifadui, chapters).empty());
    EXPECT_FALSE(finaleErrors(zhuzhong, zhifadui + "advance_days(1)\n", chapters).empty());
    auto edited = zhifadui;
    edited[10] = edited[10] == 'a' ? 'b' : 'a';
    EXPECT_FALSE(finaleErrors(zhuzhong, edited, chapters).empty());
    auto noPatch = zhifadui.substr(0, kChapterTenFinaleBytes);
    EXPECT_FALSE(finaleErrors(zhuzhong, noPatch, chapters).empty());
    auto more = chapters;
    more["chapters"].push_back({{"number", 12}, {"done_flag", "ch12.done"}});
    EXPECT_FALSE(finaleErrors(zhuzhong, zhifadui, more).empty());
}
} // namespace
