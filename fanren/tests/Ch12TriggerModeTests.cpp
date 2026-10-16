// 第 12 章挂点、门、走偏与章末（只读数据 / 扫脚本 / 读地图那一半）。对应 docs/ch12-design.md 第 15 节验收：
//   第 3 条  挂点 mode / once / guard / set_flag 与 3.1 表一致；once=true 都有 set_flag；NPC 挂点（n06、n07、n09、n18、n33）
//            无 once、脚本开头判 done_flag；trigger_yutai once 否、无旗标、不 give；无 auto；无同格两个对象；没有 enter
//            触发器盖住任何 spawn；n01、n08、n12、n20 是横贯触发带（从 spawn 不踩它就走不到该图别的挂点）；yutai 从 yuqiao
//            走得到且不必经过 trigger_yuqiao；十处存档点与 spawn 相邻、不与触发器同格；四处打坐设施；三处落点不与触发器同格；
//            scripts/ch12 的 39 份脚本各恰被一个地图对象挂着。
//   第 15 条 只有荒岛 ↔ 地洞一对门；portal_dixue 的 require_flag / deny_text_key；不通第 11 章任何图（读地图那一半）。
//   第 16 条 走偏三触发器与三份脚本（读地图 ＋ 扫脚本那一半；kScriptTransfers 五元三条归 ObjectiveTests）。
//   第 17 条 同图同 role 的 NPC 首尾恰接；两名执法长老名牌不同；寒骊台乌丑、玄骨退场为 ch12.xuangu（读地图那一半）。
//   第 20 条 lingquan.lua 置 ch12.done、不 ending()；第 13 章之后仅末行一处 teleport("ch13_nanming")、此前活代码与
//            第 12 章集成版相同（ch13 施工图 16.1 第 58 条、契约 interfaces-p3-ch13.md 5.3）；zhuzhong.lua 与第 11 章集成版只差末尾一行。
// 每一条判据都有「在内存里改坏 → 必须报」的负向（Mutations*）。切片 / 通关那一半归测试路 B。
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <regex>
#include "Ch12Data.h"

namespace {
using namespace fanren::test::ch12;
using Maps = std::map<std::string, Json>;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

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

void dropProp(Json& map, const std::string& name, const std::string& key) {
    Json* object = findObject(map, name);
    ASSERT_NE(object, nullptr) << name;
    auto& props = (*object)["properties"];
    for (std::size_t i = 0; i < props.size(); ++i)
        if (props[i].at("name") == key) { props.erase(i); return; }
}

void moveObject(Json& map, const std::string& name, Cell cell) {
    Json* object = findObject(map, name);
    ASSERT_NE(object, nullptr) << name;
    (*object)["x"] = cell.x * map.at("tilewidth").get<int>();
    (*object)["y"] = cell.y * map.at("tileheight").get<int>();
}

void addCopy(Json& map, const std::string& name, const std::string& newName) {
    Json* object = findObject(map, name);
    ASSERT_NE(object, nullptr) << name;
    Json copy = *object;
    copy["name"] = newName;
    for (auto& layer : map["layers"])
        if (layer.value("type", "") == "objectgroup") { layer["objects"].push_back(copy); return; }
}

// 脚本第一句活代码：去注释、去空行后的第一行。
std::string firstLiveLine(const std::string& source) {
    std::istringstream in(executableLines(source));
    for (std::string line; std::getline(in, line);) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (!line.empty()) return line;
    }
    return {};
}

// ---- 第 3 条：挂点表 ----
Errors hookErrors(const Maps& maps, const Sources& sources) {
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
        check(*hook.guard != '\0' || !p.contains("guard_flag"), "unguarded hook carries guard_flag");
        check(p.value("set_flag", "") == hook.done, "set_flag");
        check(hook.once || !p.contains("set_flag"), "repeatable has set_flag");
        check(!hook.once || *hook.done != '\0', "once without set_flag");
        check(p.value("script", "") == "ch12/" + std::string(hook.script) + ".lua", "script");
        seen.insert(std::string(hook.map) + "/" + hook.object);
    }
    for (const auto& npc : kNpcHooks) {
        const auto all = objects(maps.at(npc.map));
        const auto found = all.find(npc.object);
        if (found == all.end()) { out.push_back(std::string("missing ") + npc.map + "/" + npc.object); continue; }
        const Json p = properties(found->second);
        if (typeOf(found->second) != "npc") out.push_back(std::string(npc.object) + " type");
        if (p.value("script", "") != "ch12/" + std::string(npc.script) + ".lua") out.push_back(std::string(npc.object) + " script");
        if (p.value("visible_flag", "") != npc.visible) out.push_back(std::string(npc.object) + " visible_flag");
        if (p.contains("once") || p.contains("set_flag")) out.push_back(std::string(npc.object) + " npc carries once/set_flag");
        // 脚本开头判自己的 done_flag，已置则 return（3.1 首段）。
        const auto source = sources.find(npc.script);
        const std::string first = source == sources.end() ? "" : firstLiveLine(source->second);
        const std::string head = std::string("if flag.get(\"") + npc.done + "\")";
        if (first.rfind(head, 0) != 0 || first.find("return") == std::string::npos)
            out.push_back(std::string(npc.script) + ".lua does not open on " + npc.done);
    }
    for (const auto& [mapId, map] : maps) {
        for (const auto& [name, object] : objects(map)) {
            const Json p = properties(object);
            if (p.value("mode", "") == "auto") out.push_back(mapId + "/" + name + " mode auto");
            if (typeOf(object) == "trigger" && !seen.count(mapId + "/" + name))
                out.push_back("undocumented trigger " + mapId + "/" + name);
        }
    }
    // trigger_yutai 不 give（拍板 14：一层玉台只给描述）。
    const auto yutai = sources.find("yutai");
    if (yutai == sources.end() || std::regex_search(executableLines(yutai->second), std::regex(R"lua(\bgive\s*\()lua")))
        out.push_back("yutai.lua missing or gives an item");
    return out;
}

// 每份 scripts/ch12/*.lua 恰被一个地图对象挂着（读地图与脚本目录，不数自己的表）。
Errors orphanErrors(const Maps& maps, const Sources& sources) {
    Errors out;
    std::map<std::string, int> hooked;
    for (const auto& [mapId, map] : maps)
        for (const auto& [name, object] : objects(map)) {
            const std::string script = properties(object).value("script", "");
            if (script.rfind("ch12/", 0) == 0) ++hooked[script];
        }
    for (const auto& [stem, source] : sources)
        if (hooked["ch12/" + stem + ".lua"] != 1)
            out.push_back(stem + ".lua hooked " + std::to_string(hooked["ch12/" + stem + ".lua"]) + " times");
    for (const auto& [script, times] : hooked)
        if (!sources.count(script.substr(5, script.size() - 9))) out.push_back(script + " hooked but missing");
    return out;
}

TEST(Ch12TriggerMode, EveryHookMatchesTheDesignTable) {
    const auto maps = shippedMaps();
    const auto sources = scripts();
    const auto errors = hookErrors(maps, sources);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    EXPECT_EQ(sources.size(), 39u) << "施工图 3.1 / 3.2：33 个主线节点 ＋ 走偏三份 ＋ Z1、采药、玉台";
    const auto orphans = orphanErrors(maps, sources);
    EXPECT_TRUE(orphans.empty()) << joined(orphans);
    auto twice = maps;
    addCopy(twice["ch12_lingquan"], "npc_yuan_yao_lq", "npc_yuan_yao_lq2");
    EXPECT_FALSE(orphanErrors(twice, sources).empty()) << "负向：同一脚本挂两处";
    auto extra = sources;
    extra["probe"] = "talk(\"\", \"ch12.probe.line\")\n";
    EXPECT_FALSE(orphanErrors(maps, extra).empty()) << "负向：一份脚本没人挂";
}

TEST(Ch12TriggerMode, MutationsOfModeOnceGuardSetFlagScriptOrAnAutoHookAreCaught) {
    const auto original = shippedMaps();
    const auto sources = scripts();
    ASSERT_TRUE(hookErrors(original, sources).empty()) << joined(hookErrors(original, sources));
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    const std::vector<Bad> bads{
        {"ch12_tushan", "trigger_dengdao", "mode", "interact"},          // n01 必须 enter
        {"ch12_guiwu", "trigger_zoupian_b", "once", true},              // 走偏可重复
        {"ch12_guiwu", "trigger_zoupian_c", "set_flag", "ch12.tupo"},  // 走偏无 set_flag
        {"ch12_baoguang", "trigger_yutai", "guard_flag", "ch12.shidian"},
        {"ch12_hanlitai", "trigger_dongkou", "set_flag", "ch12.canhun"},
        {"ch12_xiaojie", "trigger_caiyao", "script", "ch12/lingshen.lua"},
        {"ch12_dixue", "npc_shaonian", "once", false},
        {"ch12_lingquan", "npc_yuan_yao_lq", "visible_flag", "ch12.xuangu"},
        {"ch12_dadian", "facility_cunji_dadian", "mode", "auto"},
    };
    for (const auto& bad : bads) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(hookErrors(maps, sources).empty()) << bad.object << " " << bad.key;
    }
    auto unset = original;
    dropProp(unset["ch12_taqian"], "trigger_tamen", "set_flag");
    EXPECT_FALSE(hookErrors(unset, sources).empty()) << "once 无 set_flag";
    auto extra = original;
    addCopy(extra["ch12_guiwu"], "trigger_zoupian_a", "trigger_zoupian_d");
    EXPECT_FALSE(hookErrors(extra, sources).empty()) << "表外多出一个触发器必须报";
    auto lateGate = sources;
    lateGate["baishi"] = "talk(\"\", \"ch12.baishi.cloud\")\n" + lateGate["baishi"];
    EXPECT_FALSE(hookErrors(original, lateGate).empty()) << "NPC 脚本不先判 done_flag";
    auto gift = sources;
    gift["yutai"] += "\ngive(\"story_poluo_zhu\", 1)\n";
    EXPECT_FALSE(hookErrors(original, gift).empty()) << "玉台给物";
}
// ---- 第 3 条后半：同格、横贯带、存档点、打坐、落点 ----
bool walkable(const Json& map, Cell c) {
    const int w = map.at("width").get<int>(), h = map.at("height").get<int>();
    if (c.x < 0 || c.y < 0 || c.x >= w || c.y >= h) return false;
    for (const auto& layer : map.at("layers"))
        if (layer.value("name", "") == "collision") return layer.at("data").at(static_cast<std::size_t>(c.y * w + c.x)) == 0;
    return false;
}

int manhattan(Cell a, Cell b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

// 从 start 出发、不踩 wall 里的格，四向走得到的全部格。
std::set<Cell> flood(const Json& map, Cell start, const std::set<Cell>& wall) {
    std::set<Cell> seen{start};
    std::deque<Cell> queue{start};
    while (!queue.empty()) {
        const Cell c = queue.front();
        queue.pop_front();
        for (const Cell n : {Cell{c.x + 1, c.y}, Cell{c.x - 1, c.y}, Cell{c.x, c.y + 1}, Cell{c.x, c.y - 1}})
            if (!seen.count(n) && !wall.count(n) && walkable(map, n)) { seen.insert(n); queue.push_back(n); }
    }
    return seen;
}

// 对象「够得着」：踩得上它的某一格，或站在它某一格的四邻上（interact 挂在实心石台上，18.2 第 4 条）。
bool touches(const std::set<Cell>& reach, const std::vector<Cell>& cells) {
    for (const Cell c : cells)
        for (const Cell n : {c, Cell{c.x + 1, c.y}, Cell{c.x - 1, c.y}, Cell{c.x, c.y + 1}, Cell{c.x, c.y - 1}})
            if (reach.count(n)) return true;
    return false;
}

// 18.1 第 6 条：三处岔口落点照抄进 zoupian_a/b/c.lua 的 FORK_X, FORK_Y。
std::vector<Cell> forkCells(const Sources& sources) {
    std::vector<Cell> out;
    const std::regex fork(R"lua(local\s+FORK_X\s*,\s*FORK_Y\s*=\s*(\d+)\s*,\s*(\d+))lua");
    for (const char* name : {"zoupian_a", "zoupian_b", "zoupian_c"}) {
        std::smatch m;
        const auto found = sources.find(name);
        const std::string code = found == sources.end() ? "" : executableLines(found->second);
        if (std::regex_search(code, m, fork)) out.push_back({std::stoi(m[1].str()), std::stoi(m[2].str())});
        else out.push_back({0, 0});
    }
    return out;
}

Errors layoutErrors(const Maps& maps, const Sources& sources) {
    Errors out;
    // 横贯触发带（16.1 第 36 条）：n01、n08、n12、n20。
    const std::map<std::string, std::string> bands{
        {"ch12_tushan", "trigger_dengdao"}, {"ch12_dadian", "trigger_rudian"},
        {"ch12_guiwu", "trigger_tupo"}, {"ch12_binghuo", "trigger_guangbi"}};
    // 打坐设施四处（16.1 第 45 条）。
    const std::set<std::string> dazuo{"ch12_dadian", "ch12_binghuo", "ch12_taqian", "ch12_hanlitai"};
    for (const auto& [mapId, map] : maps) {
        const auto all = objects(map);
        std::map<Cell, std::string> taken;
        std::set<Cell> triggerCells, spawnCells;
        for (const auto& [name, object] : all) {
            for (const Cell c : cellsOf(map, object)) {
                const auto [it, fresh] = taken.emplace(c, name);
                if (!fresh) out.push_back(mapId + " " + name + " shares a cell with " + it->second);
                if (typeOf(object) == "trigger") triggerCells.insert(c);
                if (typeOf(object) == "spawn") spawnCells.insert(c);
            }
        }
        for (const auto& [name, object] : all)
            if (typeOf(object) == "trigger" && properties(object).value("mode", "") == "enter")
                for (const Cell c : cellsOf(map, object))
                    if (spawnCells.count(c)) out.push_back(mapId + " " + name + " covers a spawn");
        const auto spawn = all.find(defaultSpawns().at(mapId));
        if (spawn == all.end() || !properties(spawn->second).value("default", false)) {
            out.push_back(mapId + " default spawn is not " + defaultSpawns().at(mapId));
            continue;
        }
        const Cell home = cellOf(map, spawn->second);
        const std::string tail = mapId.substr(5);
        const auto cunji = all.find("facility_cunji_" + tail);
        if (cunji == all.end() || properties(cunji->second).value("kind", "") != "save" ||
            manhattan(cellOf(map, cunji->second), home) != 1)
            out.push_back(mapId + " facility_cunji_" + tail + " is not a save spot beside the spawn");
        int meditate = 0;
        for (const auto& [name, object] : all) {
            if (name.rfind("facility_cunji_", 0) == 0 && name != "facility_cunji_" + tail) out.push_back(mapId + " extra " + name);
            if (name.rfind("facility_dazuo_", 0) != 0) continue;
            const Json p = properties(object);
            if (name == "facility_dazuo_" + tail && p.value("kind", "") == "meditate" && p.value("effectiveness", 0) == 100) ++meditate;
            else out.push_back(mapId + " " + name + " is not the designed meditate spot");
        }
        if (meditate != (dazuo.count(mapId) ? 1 : 0)) out.push_back(mapId + " meditate spots " + std::to_string(meditate));
        const auto band = bands.find(mapId);
        if (band != bands.end()) {
            const auto strip = all.find(band->second);
            if (strip == all.end()) { out.push_back(mapId + " band missing"); continue; }
            const auto wallCells = cellsOf(map, strip->second);
            const std::set<Cell> wall(wallCells.begin(), wallCells.end());
            const auto reach = flood(map, home, wall);
            for (const auto& [name, object] : all) {
                const std::string type = typeOf(object);
                if (name == band->second || (type != "trigger" && type != "npc" && type != "portal")) continue;
                if (touches(reach, cellsOf(map, object))) out.push_back(mapId + " " + name + " reachable without crossing " + band->second);
            }
        }
    }
    // trigger_yutai 从 yuqiao 走得到、不必经过 trigger_yuqiao（16.1 第 38 条）。
    const Json& hall = maps.at("ch12_baoguang");
    const auto hallObjects = objects(hall);
    const auto yuqiaoCells = cellsOf(hall, hallObjects.at("trigger_yuqiao"));
    const auto beforeGate = flood(hall, cellOf(hall, hallObjects.at("yuqiao")), {yuqiaoCells.begin(), yuqiaoCells.end()});
    if (!touches(beforeGate, cellsOf(hall, hallObjects.at("trigger_yutai")))) out.push_back("trigger_yutai only reachable past trigger_yuqiao");
    // 三处岔口落点：不是 (0,0)、两两不同、不与任何触发器同格、站得住（18.1 第 6 条）。
    const Json& fog = maps.at("ch12_guiwu");
    const auto forks = forkCells(sources);
    std::set<Cell> fogTriggers;
    for (const auto& [name, object] : objects(fog))
        if (typeOf(object) == "trigger") for (const Cell c : cellsOf(fog, object)) fogTriggers.insert(c);
    for (const Cell c : forks) {
        if (c == Cell{0, 0} || !walkable(fog, c)) out.push_back("fork landing unusable");
        if (fogTriggers.count(c)) out.push_back("fork landing on a trigger");
    }
    if (std::set<Cell>(forks.begin(), forks.end()).size() != 3) out.push_back("fork landings are not three distinct cells");
    return out;
}

TEST(Ch12TriggerMode, BandsSaveSpotsMeditationAndForkLandingsSitWhereTheDesignPutsThem) {
    const auto errors = layoutErrors(shippedMaps(), scripts());
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch12TriggerMode, MutationsOfTheLayoutAreCaught) {
    const auto original = shippedMaps();
    const auto sources = scripts();
    ASSERT_TRUE(layoutErrors(original, sources).empty()) << joined(layoutErrors(original, sources));
    struct Move { const char* map; const char* object; Cell to; };
    for (const auto& move : {Move{"ch12_tushan", "trigger_dengdao", {22, 31}},     // 带压到 spawn 那一行
                             Move{"ch12_dadian", "trigger_rudian", {19, 20}},      // 带不再挡路
                             Move{"ch12_guiwu", "trigger_yihai", {27, 33}},        // 压在第一岔口落点
                             Move{"ch12_hanlitai", "facility_cunji_hanlitai", {19, 27}},
                             Move{"ch12_baoguang", "trigger_yutai", {2, 2}},       // 玉台埋进实心岩里
                             Move{"ch12_taqian", "trigger_tamen", {30, 10}}}) {    // 与塔前同格
        auto maps = original;
        moveObject(maps[move.map], move.object, move.to);
        EXPECT_FALSE(layoutErrors(maps, sources).empty()) << move.object;
    }
    // n24 横在玉桥三格宽的过道上（17–19 列）：玉台只剩踩过 n24 才走得到。
    auto gated = original;
    moveObject(gated["ch12_baoguang"], "trigger_yuqiao", {17, 20});
    (*findObject(gated["ch12_baoguang"], "trigger_yuqiao"))["width"] = 3 * gated["ch12_baoguang"].at("tilewidth").get<int>();
    EXPECT_FALSE(layoutErrors(gated, sources).empty()) << "玉台被 n24 隔在后面";
    auto weak = original;
    setProp(weak["ch12_binghuo"], "facility_dazuo_binghuo", "effectiveness", 50);
    EXPECT_FALSE(layoutErrors(weak, sources).empty()) << "打坐效力";
    auto missing = original;
    addCopy(missing["ch12_xiaojie"], "facility_cunji_xiaojie", "facility_dazuo_xiaojie");
    EXPECT_FALSE(layoutErrors(missing, sources).empty()) << "第五处打坐";
    auto same = sources;
    same["zoupian_c"] = std::regex_replace(same["zoupian_c"], std::regex(R"(= 30, 16)"), "= 27, 33");
    EXPECT_FALSE(layoutErrors(original, same).empty()) << "两处岔口落在同一格";
}
// ---- 第 15 条：只有荒岛 ↔ 地洞一对门 ----
Errors doorErrors(const Maps& maps) {
    Errors out;
    std::multiset<std::pair<std::string, std::string>> crossing;
    const std::set<std::string> ours(kMaps.begin(), kMaps.end());
    for (const auto& [mapId, map] : maps) {
        for (const auto& [name, object] : objects(map)) {
            const std::string type = typeOf(object);
            if (type == "encounter") out.push_back(mapId + "/" + name + " encounter zone");   // 8.4：本章不放遭遇区
            if (type != "portal") continue;
            const Json p = properties(object);
            const std::string target = p.value("target_map", "");
            if (!ours.count(target)) out.push_back(mapId + "/" + name + " leads out of chapter 12 to " + target);
            crossing.emplace(mapId + "/" + name, target);
            if (name == "portal_dixue") {
                if (p.value("require_flag", "") != "ch12.tuizhu") out.push_back("portal_dixue require_flag");
                if (p.value("deny_text_key", "") != "ch12.block.dixue") out.push_back("portal_dixue deny_text_key");
            } else if (p.contains("require_flag") || p.contains("deny_text_key")) {
                out.push_back(mapId + "/" + name + " has an undesigned gate");
            }
        }
    }
    const std::multiset<std::pair<std::string, std::string>> expected{
        {"ch12_tushan/portal_dixue", "ch12_dixue"}, {"ch12_dixue/portal_tushan", "ch12_tushan"}};
    if (crossing != expected) out.push_back("portals differ from the one designed pair");
    return out;
}

TEST(Ch12TriggerMode, OnlyTheIslandAndTheCaveShareADoorAndNothingLeadsBackToChapterEleven) {
    const auto errors = doorErrors(shippedMaps());
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch12TriggerMode, MutationsOfDoorsAreCaught) {
    const auto original = shippedMaps();
    ASSERT_TRUE(doorErrors(original).empty());
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    for (const auto& bad : {Bad{"ch12_dixue", "portal_tushan", "target_map", "ch11_dongfu"},
                            Bad{"ch12_tushan", "portal_dixue", "require_flag", "ch12.pozhen"},
                            Bad{"ch12_tushan", "portal_dixue", "deny_text_key", "ch12.tushan.deny"},
                            Bad{"ch12_dixue", "portal_tushan", "require_flag", "ch12.duoshe"}}) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(doorErrors(maps).empty()) << bad.object << " " << bad.key;
    }
    auto extra = original;
    addCopy(extra["ch12_hanlitai"], "trigger_xiaojian", "portal_huitai");
    (*findObject(extra["ch12_hanlitai"], "portal_huitai"))["type"] = "portal";
    setProp(extra["ch12_hanlitai"], "portal_huitai", "target_map", "ch12_taqian");
    EXPECT_FALSE(doorErrors(extra).empty()) << "殿内多一扇回头门";
}

// ---- 第 16 条：走偏（读地图那一半在挂点表；这里扫三份脚本）----
Errors detourErrors(const Sources& sources) {
    Errors out;
    for (const char* name : {"zoupian_a", "zoupian_b", "zoupian_c"}) {
        const auto found = sources.find(name);
        if (found == sources.end()) { out.push_back(std::string(name) + ".lua missing"); continue; }
        const std::string code = executableLines(found->second);
        if (code.find("flag.set(") != std::string::npos) out.push_back(std::string(name) + " sets a flag");
        // 开头判 ch12.guiwang（已灭鬼王则 return）：只许落点常量（local FORK_X, FORK_Y = x, y）写在它前面。
        std::string head = code;
        head = std::regex_replace(head, std::regex(R"lua((?:^|\n)\s*local\s+FORK_X\s*,\s*FORK_Y\s*=\s*\d+\s*,\s*\d+[ \t]*)lua"), "\n");
        const std::string first = firstLiveLine(head);
        if (first.rfind("if flag.get(\"ch12.guiwang\")", 0) != 0 || first.find("return") == std::string::npos)
            out.push_back(std::string(name) + " does not return first once ch12.guiwang is set");
        const std::regex hop(R"lua((?:^|\n)teleport\("ch12_guiwu",\s*FORK_X,\s*FORK_Y\))lua");
        if (countMatches(code, hop) != 1 || countMatches(code, std::regex(R"lua(\bteleport\s*\()lua")) != 1)
            out.push_back(std::string(name) + " does not hop back to its own fork exactly once");
    }
    return out;
}

TEST(Ch12TriggerMode, TheThreeDetoursHopBackToTheirOwnForksAndAdvanceNothing) {
    const auto errors = detourErrors(scripts());
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch12TriggerMode, MutationsOfTheDetourScriptsAreCaught) {
    const auto original = scripts();
    ASSERT_TRUE(detourErrors(original).empty()) << joined(detourErrors(original));
    auto flagged = original;
    flagged["zoupian_a"] += "\nflag.set(\"ch12.z1\")\n";
    EXPECT_FALSE(detourErrors(flagged).empty());
    auto ungated = original;
    ungated["zoupian_b"] = std::regex_replace(ungated["zoupian_b"], std::regex(R"lua(if flag\.get\("ch12\.guiwang"\)[^\n]*\n)lua"), "");
    EXPECT_FALSE(detourErrors(ungated).empty());
    auto elsewhere = original;
    elsewhere["zoupian_c"] = std::regex_replace(elsewhere["zoupian_c"], std::regex("teleport\\(\"ch12_guiwu\""), "teleport(\"ch12_tushan\"");
    EXPECT_FALSE(detourErrors(elsewhere).empty());
}
// ---- 第 17 条（读地图那一半）：同图同 role 首尾恰接；元瑶与两组分图；两名执法长老分名牌 ----
Errors presenceErrors(const Maps& maps, const std::map<std::string, std::string>& roleNames) {
    Errors out;
    for (const auto& [mapId, map] : maps) {
        std::map<std::string, std::vector<std::pair<std::string, Json>>> byRole;
        for (const auto& [name, object] : objects(map))
            if (typeOf(object) == "npc") byRole[properties(object).value("role_id", "")].push_back({name, properties(object)});
        for (const auto& [role, group] : byRole) {
            for (std::size_t i = 0; i < group.size(); ++i)
                for (std::size_t j = i + 1; j < group.size(); ++j) {
                    const Json& a = group[i].second;
                    const Json& b = group[j].second;
                    const bool chained = (a.contains("hidden_flag") && a.at("hidden_flag") == b.value("visible_flag", "")) ||
                                         (b.contains("hidden_flag") && b.at("hidden_flag") == a.value("visible_flag", ""));
                    if (!chained) out.push_back(mapId + " " + group[i].first + " / " + group[j].first + " overlap as " + role);
                }
        }
    }
    // 两个元瑶（塔前 / 灵泉）分图，寒骊台没有她（第 4 节表下、16.1 第 37 条）。
    const auto roleOn = [&](const char* mapId, const char* object) {
        const auto all = objects(maps.at(mapId));
        const auto found = all.find(object);
        return found == all.end() ? std::string() : properties(found->second).value("role_id", "");
    };
    if (roleOn("ch12_taqian", "npc_yuan_yao_tq") != "yuan_yao_jiedan") out.push_back("npc_yuan_yao_tq not on ch12_taqian");
    if (roleOn("ch12_lingquan", "npc_yuan_yao_lq") != "yuan_yao_jiedan") out.push_back("npc_yuan_yao_lq not on ch12_lingquan");
    for (const auto& [name, object] : objects(maps.at("ch12_hanlitai")))
        if (properties(object).value("role_id", "") == "yuan_yao_jiedan") out.push_back("hanlitai carries a Yuan Yao");
    // 寒骊台乌丑、玄骨退场为 ch12.xuangu。
    for (const char* name : {"npc_wu_chou_hl", "npc_xuan_gu_hl"}) {
        const auto all = objects(maps.at("ch12_hanlitai"));
        const auto found = all.find(name);
        if (found == all.end() || properties(found->second).value("hidden_flag", "") != "ch12.xuangu")
            out.push_back(std::string(name) + " does not leave on ch12.xuangu");
    }
    // 16.1 第 43 条：两名执法长老名牌「慈眉长老」「冷脸长老」。
    if (roleNames.at("xinggong_zhifa_ci") != "慈眉长老" || roleNames.at("xinggong_zhifa_leng") != "冷脸长老")
        out.push_back("the two enforcers do not carry the designed name plates");
    return out;
}

std::map<std::string, std::string> enforcerNames() {
    std::map<std::string, std::string> names;
    for (const char* id : {"xinggong_zhifa_ci", "xinggong_zhifa_leng"})
        names[id] = json(root() / "data" / "roles" / (std::string(id) + ".json")).at("name").get<std::string>();
    return names;
}

TEST(Ch12TriggerMode, SameRoleObjectsHandOverOnOneFlagAndTheTwoGroupsLiveOnTheirOwnMaps) {
    const auto errors = presenceErrors(shippedMaps(), enforcerNames());
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch12TriggerMode, MutationsOfPresenceAreCaught) {
    const auto original = shippedMaps();
    const auto names = enforcerNames();
    ASSERT_TRUE(presenceErrors(original, names).empty()) << joined(presenceErrors(original, names));
    struct Bad { const char* map; const char* object; const char* key; const char* value; };
    for (const auto& bad : {Bad{"ch12_guiwu", "npc_zi_ling_hy", "visible_flag", "ch12.tupo"},       // 16.1 第 43 条前的写法
                            Bad{"ch12_xiaojie", "npc_jiyin_xg", "visible_flag", "ch12.fanlian"},
                            Bad{"ch12_hanlitai", "npc_xuan_gu_hl", "hidden_flag", "ch12.butian"},
                            Bad{"ch12_hanlitai", "npc_qingyi_hl", "role_id", "yuan_yao_jiedan"}}) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(presenceErrors(maps, names).empty()) << bad.object << " " << bad.key;
    }
    auto plates = names;
    plates["xinggong_zhifa_leng"] = "执法长老";
    EXPECT_FALSE(presenceErrors(original, plates).empty());
}

// ---- 第 20 条：章末与开局（扫脚本那一半）----
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

// 第 11 章集成版 scripts/ch11/zhuzhong.lua（HEAD 6ec8073）：687 字节，FNV-1a 64 = 0x461b9e962c20c54d。
constexpr std::size_t kChapterElevenFinaleBytes = 687;
constexpr std::uint64_t kChapterElevenFinaleHash = 0x461b9e962c20c54dull;

// 第 12 章集成版 scripts/ch12/lingquan.lua（HEAD 46f6d37）的活代码（executableLines，注释一律剥掉）：815 字节，
// FNV-1a 64 = 0xdaa92bbe0608775f。第 13 章只在 flag.set("ch12.done") 之后补一行 teleport，头注释那半句改成现状
//（契约 interfaces-p3-ch13.md 5.3、6.4；ch13 施工图 16.1 第 1、58 条）——所以钉活代码，不钉整份字节。
constexpr std::size_t kChapterTwelveFinaleCodeBytes = 815;
constexpr std::uint64_t kChapterTwelveFinaleCodeHash = 0xdaa92bbe0608775full;
constexpr const char* kChapterThirteenHandOver = "teleport(\"ch13_nanming\")";

Errors finaleErrors(const std::string& lingquan, const std::string& zhuzhong, const Json& chapters) {
    Errors out;
    // 16.1 第 58 条：仅末行一处 teleport("ch13_nanming")，紧挨在 flag.set("ch12.done") 之后。
    const auto lines = liveLines(lingquan);
    if (lines.size() < 2 || lines.back() != kChapterThirteenHandOver)
        out.push_back("lingquan.lua does not end with teleport(\"ch13_nanming\")");
    else if (lines[lines.size() - 2] != "flag.set(\"ch12.done\")")
        out.push_back("lingquan.lua does not set ch12.done right before the hand-over");
    const std::string code = executableLines(lingquan);
    if (code.find("ending(") != std::string::npos) out.push_back("lingquan.lua calls ending()");
    if (countMatches(code, std::regex(R"lua(\bteleport\s*\()lua")) != 1)
        out.push_back("lingquan.lua teleports somewhere besides its one closing line");
    const std::string handOver = std::string(kChapterThirteenHandOver) + "\n";
    if (code.size() != kChapterTwelveFinaleCodeBytes + handOver.size() ||
        code.compare(kChapterTwelveFinaleCodeBytes, std::string::npos, handOver) != 0) {
        out.push_back("lingquan.lua is not the chapter-12 code plus exactly one closing teleport line");
    } else if (fnv1a(code.substr(0, kChapterTwelveFinaleCodeBytes)) != kChapterTwelveFinaleCodeHash) {
        out.push_back("lingquan.lua code changed before the patched line");
    }
    const std::string patch = "teleport(\"ch12_tushan\")\n";
    if (zhuzhong.size() != kChapterElevenFinaleBytes + patch.size() ||
        zhuzhong.compare(kChapterElevenFinaleBytes, std::string::npos, patch) != 0) {
        out.push_back("zhuzhong.lua is not the chapter-11 file plus exactly one closing teleport line");
    } else if (fnv1a(zhuzhong.substr(0, kChapterElevenFinaleBytes)) != kChapterElevenFinaleHash) {
        out.push_back("zhuzhong.lua changed before the patched line");
    }
    // 第 13 章进表之后，第 12 章是第 12 项、done_flag 仍是 ch12.done（谁是最后一章归 UiStyle 的 ChapterTableData）。
    const auto& table = chapters.at("chapters");
    if (table.size() < 12 || table[11].at("number") != 12 || table[11].at("done_flag") != "ch12.done" ||
        table[11].value("numeral_key", "") != "ui.chapter.12.numeral" || table[11].value("title_key", "") != "ui.chapter.12.title")
        out.push_back("chapter 12 is not the twelfth entry of data/chapters.json as contract 5.2 wrote it");
    return out;
}

TEST(Ch12TriggerMode, TheChapterEndsOnTheDoneFlagAndChapterElevenHandsOverWithOneLine) {
    const auto errors = finaleErrors(read(root() / "scripts" / "ch12" / "lingquan.lua"),
                                     read(root() / "scripts" / "ch11" / "zhuzhong.lua"),
                                     json(root() / "data" / "chapters.json"));
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch12TriggerMode, MutationsOfTheFinaleAreCaught) {
    const std::string lingquan = read(root() / "scripts" / "ch12" / "lingquan.lua");
    const std::string zhuzhong = read(root() / "scripts" / "ch11" / "zhuzhong.lua");
    const Json chapters = json(root() / "data" / "chapters.json");
    ASSERT_TRUE(finaleErrors(lingquan, zhuzhong, chapters).empty());
    EXPECT_FALSE(finaleErrors(lingquan + "ending(\"t\", \"b\")\n", zhuzhong, chapters).empty());
    EXPECT_FALSE(finaleErrors(lingquan + "teleport(\"ch12_tushan\")\n", zhuzhong, chapters).empty());
    EXPECT_FALSE(finaleErrors(lingquan + "teleport(\"ch13_nanming\")\n", zhuzhong, chapters).empty()) << "两处 teleport";
    EXPECT_FALSE(finaleErrors(lingquan + "talk(\"\", \"ch12.lingquan.after\")\n", zhuzhong, chapters).empty());
    EXPECT_FALSE(finaleErrors(lingquan, zhuzhong + "advance_days(1)\n", chapters).empty());
    // 第 13 章那一行：拿掉（第 12 章原样）、挪到 flag.set 之前、换个落点，都报。
    const std::size_t handOver = lingquan.rfind("teleport(\"ch13_nanming\")");
    ASSERT_NE(handOver, std::string::npos);
    const std::string unpatched = lingquan.substr(0, handOver);
    EXPECT_FALSE(finaleErrors(unpatched, zhuzhong, chapters).empty()) << "没补 teleport";
    const std::size_t done = unpatched.rfind("flag.set(\"ch12.done\")");
    ASSERT_NE(done, std::string::npos);
    EXPECT_FALSE(finaleErrors(unpatched.substr(0, done) + "teleport(\"ch13_nanming\")\nflag.set(\"ch12.done\")\n",
                              zhuzhong, chapters).empty()) << "teleport 写在置旗之前";
    EXPECT_FALSE(finaleErrors(unpatched + "teleport(\"ch13_tianxing\")\n", zhuzhong, chapters).empty()) << "落错图";
    // 补丁之前的活代码动一个字也报；只改注释的字（行数不变）不报——对照，证明钉的是活代码。
    auto days = lingquan;
    const std::size_t three = days.find("\nadvance_days(3)");   // 顶格那一句（头注释里也写着 advance_days(3)）
    ASSERT_NE(three, std::string::npos);
    days[three + 14] = '4';
    EXPECT_FALSE(finaleErrors(days, zhuzhong, chapters).empty()) << "advance_days(4)";
    auto remark = lingquan;
    const std::size_t note = remark.find("-- ");
    ASSERT_NE(note, std::string::npos);
    remark.insert(note + 3, "x");
    EXPECT_TRUE(finaleErrors(remark, zhuzhong, chapters).empty()) << joined(finaleErrors(remark, zhuzhong, chapters));
    auto edited = zhuzhong;
    edited[10] = edited[10] == 'a' ? 'b' : 'a';
    EXPECT_FALSE(finaleErrors(lingquan, edited, chapters).empty());
    EXPECT_FALSE(finaleErrors(lingquan, zhuzhong.substr(0, kChapterElevenFinaleBytes), chapters).empty());
    auto renamed = chapters;
    renamed["chapters"][11]["done_flag"] = "ch12.end";
    EXPECT_FALSE(finaleErrors(lingquan, zhuzhong, renamed).empty());
    auto shorter = chapters;
    shorter["chapters"].erase(std::size_t{11});
    EXPECT_FALSE(finaleErrors(lingquan, zhuzhong, shorter).empty());
    auto more = chapters;
    more["chapters"].push_back({{"number", 14}, {"done_flag", "ch14.done"}});
    EXPECT_TRUE(finaleErrors(lingquan, zhuzhong, more).empty()) << "对照：表往后长不牵动第 12 章这一项";
}
}  // namespace
