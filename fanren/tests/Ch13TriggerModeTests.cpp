// 第 13 章挂点、地图布局、传送、在场与章末（只读数据 / 扫脚本 / 读地图那一半）。对应 docs/ch13-design.md 第 15 节验收：
//   第 3 条  挂点 mode / once / guard / set_flag 与 3.1 表一致：once=true 都有 set_flag（＝ done_flag）；NPC 挂点（n02、n04、
//            n06、n12、n14、n15、n29、n32）无 once、脚本开头判 done_flag 与前置；trigger_youyao once 否、无 set_flag；坊市 /
//            告示墙 / 养魂木 once 否、无旗标；trigger_feixu once 否、无 set_flag、guard ch13.shouchao；无 auto；同格 0 处两个对象；
//            没有 enter 触发器盖住 spawn；n01、n07、n13、n17 是 spawn 前方的横贯带；往返两处与 n16 / n30 的带错开；#5、#6
//            两处带坐标的落点格上无对象、#6 不压 n16 的带；dizhen 与 chuguan 不同行；五处存档点与落点相邻、不与触发器同格；
//            scripts/ch13 的 40 份脚本各恰被一个地图对象挂着。另：打坐与设施（第 4 节、7.1、16.1 第 45 条）、
//            洞府院子在内侧带之内（4.1「推荐（本稿拍板）」、4.3 第 2 条）。
//   第 5 条  chuguan.lua 第一条可执行语句是 realm.at_least(realm.CORE_MID) 的判断（扫脚本那一半；realm.cap 归 Ch13Ledger）。
//   第 12 条 黑石城六个 NPC 的 hidden_flag == ch13.shouchao；四张新图上 shop 类 facility 0 个（读地图那一半）。
//   第 13 条 往返两个触发器的属性（10.2 T2）；两份脚本各恰 1 处 advance_days(12)、0 处 flag.set，取消 / 拨日 / teleport 顶格
//            （16.1 第 60 条），开头各判 ch13.biguan / ch13.lidao。
//   第 16 条 四张新图 portal 0 个、遭遇区 0 个；第 12 章及以前的图没有一个对象指向第 13 章（kScriptTransfers 7 条归 ObjectiveTests）。
//   第 17 条 5.1 每个对象的 role / 出场 / 退场逐条对表；同图可能同时在场的名牌两两不同；yi_jing 两个对象分属两图（读地图那一半）。
//   第 20 条 banyao.lua 最后一条可执行语句是 flag.set("ch13.done")、give 伴妖草在它之前、0 处 ending() / teleport；
//   第 21 条 data/chapters.json 第 13 项照 10.1 C1 字面量（lingquan.lua 那一行归 Ch12TriggerMode）。
//   第 27 条 mingzhu.lua、cunjin.lua 第一个可执行语句判前置；biguan.lua、lidao.lua 的确认 choice 在拨日、置旗之前，取消即 return。
// 每一条判据都有「在内存里改坏 → 必须报」的负向（Mutations*）。切片 / 通关那一半归测试路 B。
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <regex>
#include "Ch13Data.h"

namespace {
using namespace fanren::test::ch13;
using Maps = std::map<std::string, Json>;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

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

// 「if flag.get("<flag>") == 0 then … return end」这一种前置门闸（一行写完）。
bool gatesOn(const std::string& line, const std::string& flag) {
    return line.rfind("if flag.get(\"" + flag + "\") == 0 then", 0) == 0 && line.find("return") != std::string::npos &&
           line.size() >= 3 && line.compare(line.size() - 3, 3, "end") == 0;
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
        check(p.value("script", "") == "ch13/" + std::string(hook.script) + ".lua", "script");
        seen.insert(std::string(hook.map) + "/" + hook.object);
    }
    for (const auto& npc : kNpcHooks) {
        const auto all = objects(maps.at(npc.map));
        const auto found = all.find(npc.object);
        if (found == all.end()) { out.push_back(std::string("missing ") + npc.map + "/" + npc.object); continue; }
        const Json p = properties(found->second);
        if (typeOf(found->second) != "npc") out.push_back(std::string(npc.object) + " type");
        if (p.value("script", "") != "ch13/" + std::string(npc.script) + ".lua") out.push_back(std::string(npc.object) + " script");
        if (p.value("visible_flag", "") != npc.visible) out.push_back(std::string(npc.object) + " visible_flag");
        if (p.contains("once") || p.contains("set_flag")) out.push_back(std::string(npc.object) + " npc carries once/set_flag");
        // 脚本开头：前置门闸（一句 return、不置旗）与 done_flag 的判断，二者都在头两句活代码里（3.0 表头、16.1 第 47 条）。
        const auto source = sources.find(npc.script);
        const auto lines = source == sources.end() ? std::vector<std::string>{} : liveLines(source->second);
        if (lines.size() < 2) { out.push_back(std::string(npc.script) + ".lua too short"); continue; }
        const bool prereqFirst = gatesOn(lines[0], npc.prereq);
        if (npc.prereqFirst && !prereqFirst) out.push_back(std::string(npc.script) + ".lua does not open on its prerequisite");
        if (!prereqFirst && !gatesOn(lines[1], npc.prereq)) out.push_back(std::string(npc.script) + ".lua lacks the prerequisite gate");
        const std::string& head = lines[prereqFirst ? 1 : 0];
        if (head.rfind(std::string("if flag.get(\"") + npc.done + "\")", 0) != 0)
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
    // 链外两处只一句（3.1：坊市闭门；16.1 第 49 条：废墟旁白、不拦路）。
    for (const char* name : {"fangshi_bimen", "feixu"}) {
        const auto found = sources.find(name);
        const auto lines = found == sources.end() ? std::vector<std::string>{} : liveLines(found->second);
        if (lines.size() != 1 || lines[0].rfind("talk(", 0) != 0) out.push_back(std::string(name) + ".lua is not one line of talk");
    }
    return out;
}

// 每份 scripts/ch13/*.lua 恰被一个地图对象挂着（读地图与脚本目录，不数自己的表）。
Errors orphanErrors(const Maps& maps, const Sources& sources) {
    Errors out;
    std::map<std::string, int> hooked;
    for (const auto& [mapId, map] : maps)
        for (const auto& [name, object] : objects(map)) {
            const std::string script = properties(object).value("script", "");
            if (script.rfind("ch13/", 0) == 0) ++hooked[script];
        }
    for (const auto& [stem, source] : sources)
        if (hooked["ch13/" + stem + ".lua"] != 1)
            out.push_back(stem + ".lua hooked " + std::to_string(hooked["ch13/" + stem + ".lua"]) + " times");
    for (const auto& [script, times] : hooked)
        if (!sources.count(script.substr(5, script.size() - 9))) out.push_back(script + " hooked but missing");
    return out;
}

TEST(Ch13TriggerMode, EveryHookMatchesTheDesignTable) {
    const auto maps = shippedMaps();
    const auto sources = scripts();
    const auto errors = hookErrors(maps, sources);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    EXPECT_EQ(sources.size(), 40u) << "18.1：主线 34 ＋ fangshi_bimen、xuanshang、yanghun、feixu、qiyuan_hang、hui_wuzhong";
    const auto orphans = orphanErrors(maps, sources);
    EXPECT_TRUE(orphans.empty()) << joined(orphans);
    auto twice = maps;
    addCopy(twice["ch13_waihai"], "npc_ming_furen", "npc_ming_furen2");
    EXPECT_FALSE(orphanErrors(twice, sources).empty()) << "负向：同一脚本挂两处";
    auto extra = sources;
    extra["probe"] = "talk(\"\", \"ch13.probe.line\")\n";
    EXPECT_FALSE(orphanErrors(maps, extra).empty()) << "负向：一份脚本没人挂";
}

TEST(Ch13TriggerMode, MutationsOfModeOnceGuardSetFlagScriptOrAnAutoHookAreCaught) {
    const auto original = shippedMaps();
    const auto sources = scripts();
    ASSERT_TRUE(hookErrors(original, sources).empty()) << joined(hookErrors(original, sources));
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    const std::vector<Bad> bads{
        {"ch13_nanming", "trigger_wenxun", "mode", "interact"},             // n01 必须 enter
        {"ch13_wuzhong", "trigger_youyao", "once", true},                  // 诱妖可重复
        {"ch13_wuzhong", "trigger_youyao", "set_flag", "ch13.lieyao"},     // 计数由脚本累加、不写 set_flag
        {"ch13_waihai", "trigger_feixu", "guard_flag", "ch13.gongsun"},
        {"ch13_waihai", "trigger_feixu", "set_flag", "ch13.shouchao"},
        {"ch13_tianxing", "trigger_fangshi_bimen", "guard_flag", "ch13.rucheng"},
        {"ch13_wuzhong", "trigger_yanghun", "set_flag", "ch13.z3"},
        {"ch13_waihai", "trigger_dongkou", "set_flag", "ch13.jianshi"},
        {"ch13_wuzhong", "trigger_shichuang", "script", "ch13/chongshi.lua"},
        {"ch13_wuzhong", "trigger_qiyuan_hang", "guard_flag", "ch13.chucheng"},
        {"ch13_waihai", "npc_wen_siyue", "once", false},
        {"ch13_waihai", "npc_li_xun", "visible_flag", "ch13.mingzhu"},
        {"ch13_tianxing", "facility_cunji_tianxing", "mode", "auto"},
    };
    for (const auto& bad : bads) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(hookErrors(maps, sources).empty()) << bad.object << " " << bad.key;
    }
    auto unset = original;
    dropProp(unset["ch13_wuzhong"], "trigger_danfang", "set_flag");
    EXPECT_FALSE(hookErrors(unset, sources).empty()) << "once 无 set_flag";
    auto extra = original;
    addCopy(extra["ch13_waihai"], "trigger_xuanshang", "trigger_xuanshang_b");
    EXPECT_FALSE(hookErrors(extra, sources).empty()) << "表外多出一个触发器必须报";
    // 验收 27：mingzhu.lua 的前置门闸挪到 done_flag 判断之后（16.1 第 47 条之前的写法）。
    auto late = sources;
    const std::string gate = "if flag.get(\"ch13.daoqiyuan\") == 0 then talk(\"ming_furen\", \"ch13.mingzhu.early\"); return end\n";
    const auto at = late["mingzhu"].find(gate);
    ASSERT_NE(at, std::string::npos);
    late["mingzhu"].erase(at, gate.size());
    late["mingzhu"] += gate;
    EXPECT_FALSE(hookErrors(original, late).empty()) << "mingzhu.lua 不先判前置";
    auto ungated = sources;
    ungated["cunjin"] = std::regex_replace(ungated["cunjin"], std::regex(R"lua(if flag\.get\("ch13\.mingzhu"\) == 0 then[^\n]*\n)lua"), "");
    EXPECT_FALSE(hookErrors(original, ungated).empty()) << "cunjin.lua 没有前置门闸";
    auto noPrereq = sources;
    noPrereq["gongsun"] = std::regex_replace(noPrereq["gongsun"], std::regex(R"lua(if flag\.get\("ch13\.chuguan"\) == 0 then[^\n]*\n)lua"), "");
    EXPECT_FALSE(hookErrors(original, noPrereq).empty()) << "gongsun.lua 没有前置门闸";
    auto lateDone = sources;
    lateDone["wenshi"] = "talk(\"\", \"ch13.wenshi.gone\")\n" + lateDone["wenshi"];
    EXPECT_FALSE(hookErrors(original, lateDone).empty()) << "NPC 脚本不先判 done_flag";
    auto chatty = sources;
    chatty["feixu"] += "\ntalk(\"\", \"ch13.feixu.ruin\")\n";
    EXPECT_FALSE(hookErrors(original, chatty).empty()) << "废墟旁白多于一句";
}

// ---- 第 3 条后半：同格、横贯带、错开、落点、存档点、设施、洞府院子 ----
bool walkable(const Json& map, Cell c) {
    const int w = map.at("width").get<int>(), h = map.at("height").get<int>();
    if (c.x < 0 || c.y < 0 || c.x >= w || c.y >= h) return false;
    for (const auto& layer : map.at("layers"))
        if (layer.value("name", "") == "collision") return layer.at("data").at(static_cast<std::size_t>(c.y * w + c.x)) == 0;
    return false;
}

int manhattan(Cell a, Cell b) { return std::abs(a.x - b.x) + std::abs(a.y - b.y); }

// 两组格之间最近的一对隔几格（Chebyshev：斜着挨上也算 1，所以「隔 ≥ N」连对角相邻都排除）。
int gap(const std::vector<Cell>& a, const std::vector<Cell>& b) {
    int best = 1 << 20;
    for (const Cell p : a)
        for (const Cell q : b) best = std::min(best, std::max(std::abs(p.x - q.x), std::abs(p.y - q.y)));
    return best;
}

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

// 对象「够得着」：踩得上它的某一格，或站在它某一格的四邻上（interact 挂点画成实心格，18.2 第 5 条）。
bool touches(const std::set<Cell>& reach, const std::vector<Cell>& cells) {
    for (const Cell c : cells)
        for (const Cell n : {c, Cell{c.x + 1, c.y}, Cell{c.x - 1, c.y}, Cell{c.x, c.y + 1}, Cell{c.x, c.y - 1}})
            if (reach.count(n)) return true;
    return false;
}

std::set<Cell> cellSet(const Json& map, const Json& object) {
    const auto cells = cellsOf(map, object);
    return {cells.begin(), cells.end()};
}

// 带坐标的两处落点（18.2 第 3 条）：脚本里 teleport("ch13_waihai", x, y) 的 x、y。
Cell landingOf(const Sources& sources, const char* script) {
    const auto found = sources.find(script);
    const std::string code = found == sources.end() ? "" : executableLines(found->second);
    std::smatch m;
    if (std::regex_search(code, m, std::regex(R"lua(teleport\("ch13_waihai",\s*(\d+),\s*(\d+)\))lua")))
        return {std::stoi(m[1].str()), std::stoi(m[2].str())};
    return {0, 0};
}

struct Facility {
    const char* map;
    const char* name;
    const char* kind;
    const char* require;   // 空 = 不写 require_flag
    int effectiveness;     // 0 = 不写（缺省 100）
    const char* ref;       // field 的 ref_id；空 = 不写
    int slots;
    int grade;
};

// 第 4 节 / 4.1 / 7.1：四张图的全部设施（存档 5、打坐 3、旧灵田与静室、雾中岛三设施）；无炼器台、无商店（验收 12）。
constexpr Facility kFacilities[] = {
    {"ch13_nanming", "facility_cunji_nanming", "save", "", 0, "", 0, 0},
    {"ch13_nanming", "facility_dazuo_nanming", "meditate", "", 0, "", 0, 0},
    {"ch13_tianxing", "facility_cunji_tianxing", "save", "", 0, "", 0, 0},
    {"ch13_tianxing", "facility_yaoyuan_39", "field", "ch13.rucheng", 0, "field_tianxing_39", 6, 0},
    {"ch13_tianxing", "facility_jingshi_39", "meditate", "ch13.rucheng", 125, "", 0, 0},
    {"ch13_waihai", "facility_cunji_qiyuan", "save", "", 0, "", 0, 0},
    {"ch13_waihai", "facility_cunji_mishi", "save", "", 0, "", 0, 0},
    {"ch13_wuzhong", "facility_cunji_wz", "save", "", 0, "", 0, 0},
    {"ch13_wuzhong", "facility_jingshi_wz", "meditate", "ch13.lifu", 150, "", 0, 0},
    {"ch13_wuzhong", "facility_danlu_wz", "alchemy", "ch13.lifu", 0, "", 0, 4},
    {"ch13_wuzhong", "facility_yaoyuan_wz", "field", "ch13.lifu", 0, "field_wuzhong", 6, 0},
};

Errors facilityErrors(const Maps& maps) {
    Errors out;
    std::set<std::string> designed;
    for (const auto& f : kFacilities) {
        designed.insert(std::string(f.map) + "/" + f.name);
        const auto all = objects(maps.at(f.map));
        const auto found = all.find(f.name);
        if (found == all.end() || typeOf(found->second) != "facility") { out.push_back(std::string("missing ") + f.name); continue; }
        const Json p = properties(found->second);
        const auto check = [&](bool ok, const char* field) { if (!ok) out.push_back(std::string(f.name) + " " + field); };
        check(p.value("kind", "") == f.kind, "kind");
        check(p.value("require_flag", "") == f.require && (*f.require != '\0' || !p.contains("require_flag")), "require_flag");
        check(!p.contains("deny_text_key"), "deny_text_key (16.1 #45)");
        check(f.effectiveness == 0 ? !p.contains("effectiveness") : p.value("effectiveness", 0) == f.effectiveness, "effectiveness");
        check(p.value("ref_id", "") == f.ref && p.value("slots", 0) == f.slots, "ref_id/slots");
        check(p.value("grade", 0) == f.grade, "grade");
    }
    for (const auto& [mapId, map] : maps)
        for (const auto& [name, object] : objects(map))
            if (typeOf(object) == "facility" && !designed.count(mapId + "/" + name)) out.push_back("undesigned facility " + mapId + "/" + name);
    return out;
}

// 18.2 第 3 条：#5 lidao.lua → (14,46) 秘市岸、#6 qiyuan_hang.lua → (7,12) 黑石城西门外礁（同一字面量写进脚本、
// ObjectiveTests 与这里）。
constexpr Cell kMishiLanding{14, 46};
constexpr Cell kWestGateLanding{7, 12};

Errors layoutErrors(const Maps& maps, const Sources& sources) {
    Errors out;
    // 每张图的首个 enter 挂点是 default spawn 前方的横贯带（3.1 首段、验收 3）。
    const std::map<std::string, std::string> bands{{"ch13_nanming", "trigger_wenxun"}, {"ch13_tianxing", "trigger_rucheng"},
                                                   {"ch13_waihai", "trigger_shiwu"}, {"ch13_wuzhong", "trigger_lifu"}};
    // 存档点紧挨落点（3.1 末行「设施」）：四个 default spawn ＋ 秘市岸。
    const std::map<std::string, std::string> saveBySpawn{{"ch13_nanming", "facility_cunji_nanming"},
        {"ch13_tianxing", "facility_cunji_tianxing"}, {"ch13_waihai", "facility_cunji_qiyuan"}, {"ch13_wuzhong", "facility_cunji_wz"}};
    for (const auto& [mapId, map] : maps) {
        const auto all = objects(map);
        std::map<Cell, std::string> taken;
        std::set<Cell> spawnCells;
        for (const auto& [name, object] : all)
            for (const Cell c : cellsOf(map, object)) {
                const auto [it, fresh] = taken.emplace(c, name);
                if (!fresh) out.push_back(mapId + " " + name + " shares a cell with " + it->second);
                if (typeOf(object) == "spawn") spawnCells.insert(c);
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
        const auto save = all.find(saveBySpawn.at(mapId));
        if (save == all.end() || manhattan(cellOf(map, save->second), home) != 1)
            out.push_back(mapId + " " + saveBySpawn.at(mapId) + " is not beside the spawn");
        const auto strip = all.find(bands.at(mapId));
        if (strip == all.end()) { out.push_back(mapId + " band missing"); continue; }
        const auto reach = flood(map, home, cellSet(map, strip->second));
        if (save != all.end() && !touches(reach, cellsOf(map, save->second)))
            out.push_back(mapId + " save spot sits past " + bands.at(mapId));
        // 带内只许够得着存档点：trigger / npc / portal / facility 一律在带外（facility 测试审查 T-7 补；紧挨 spawn
        // 的那个存档点是 3.1 末行定的、本就在带内，排除）。
        for (const auto& [name, object] : all) {
            const std::string type = typeOf(object);
            if (name == bands.at(mapId) || name == saveBySpawn.at(mapId)) continue;
            if (type != "trigger" && type != "npc" && type != "portal" && type != "facility") continue;
            if (touches(reach, cellsOf(map, object))) out.push_back(mapId + " " + name + " reachable without crossing " + bands.at(mapId));
        }
    }
    const auto at = [&](const char* mapId, const char* name) -> std::vector<Cell> {
        const auto all = objects(maps.at(mapId));
        const auto found = all.find(name);
        return found == all.end() ? std::vector<Cell>{} : cellsOf(maps.at(mapId), found->second);
    };
    // 3.1 往返两行、n25 行：错开的格数（4.2 末条：同格只取第一个 trigger，错开免得一步两响）。
    const auto apart = [&](const char* mapId, const char* a, const char* b, int least) {
        const auto ca = at(mapId, a), cb = at(mapId, b);
        if (ca.empty() || cb.empty() || gap(ca, cb) < least) out.push_back(std::string(a) + " and " + b + " are closer than " + std::to_string(least));
    };
    apart("ch13_waihai", "trigger_hui_wuzhong", "trigger_chucheng", 3);
    apart("ch13_wuzhong", "trigger_qiyuan_hang", "trigger_lidao", 2);
    apart("ch13_wuzhong", "trigger_youyao", "trigger_shoushou", 2);
    // trigger_dizhen 与 trigger_chuguan 分在两行（验收 3）。
    for (const Cell a : at("ch13_wuzhong", "trigger_dizhen"))
        for (const Cell b : at("ch13_wuzhong", "trigger_chuguan"))
            if (a.y == b.y) out.push_back("dizhen and chuguan share a row");
    // 带坐标的两处落点：字面量、站得住、格上无对象；#6 在 hui_wuzhong 与西门带之间、不压带；各自走得到要用的挂点。
    const Json& sea = maps.at("ch13_waihai");
    std::set<Cell> occupied;
    for (const auto& [name, object] : objects(sea))
        for (const Cell c : cellsOf(sea, object)) occupied.insert(c);
    const Cell mishi = landingOf(sources, "lidao"), gate = landingOf(sources, "qiyuan_hang");
    if (mishi != kMishiLanding || gate != kWestGateLanding) out.push_back("landing cells differ from 18.2 #3");
    for (const Cell c : {mishi, gate})
        if (!walkable(sea, c) || occupied.count(c)) out.push_back("a landing cell is blocked or carries an object");
    const auto hui = at("ch13_waihai", "trigger_hui_wuzhong"), west = at("ch13_waihai", "trigger_chucheng");
    if (hui.empty() || west.empty() || !(hui.front().x < gate.x && gate.x < west.front().x))
        out.push_back("#6 landing is not between trigger_hui_wuzhong and the west gate band");
    if (!touches(flood(sea, gate, {}), hui)) out.push_back("#6 landing cannot walk back to trigger_hui_wuzhong");
    if (!touches(flood(sea, mishi, {}), at("ch13_waihai", "trigger_mishi"))) out.push_back("#5 landing cannot reach trigger_mishi");
    const auto mishiSave = at("ch13_waihai", "facility_cunji_mishi");
    if (mishiSave.size() != 1 || manhattan(mishiSave.front(), mishi) != 1) out.push_back("facility_cunji_mishi is not beside #5");
    // facility_dazuo_nanming 在 trigger_chugang 之前的路上（4.1、E 表 39：① 之前要有回血处）。
    {
        const Json& isle = maps.at("ch13_nanming");
        const auto chugang = at("ch13_nanming", "trigger_chugang");
        if (!touches(flood(isle, cellOf(isle, objects(isle).at("haishang")), {chugang.begin(), chugang.end()}),
                     at("ch13_nanming", "facility_dazuo_nanming")))
            out.push_back("facility_dazuo_nanming only reachable past trigger_chugang");
    }
    // 洞府院子（4.1「推荐（本稿拍板）」、4.3 第 2 条）：静室、丹炉、药园、养魂木与六扇门都在内侧带 dizhen 之内；
    // 外侧带 chuguan 与登岸礁在外——n27 之后反复坐关不必跨过 trigger_chuguan。
    {
        const Json& isle = maps.at("ch13_wuzhong");
        const auto room = at("ch13_wuzhong", "facility_jingshi_wz");
        const auto inner = at("ch13_wuzhong", "trigger_dizhen");
        const auto yard = room.empty() ? std::set<Cell>{} : flood(isle, room.front(), {inner.begin(), inner.end()});
        for (const char* name : {"trigger_chongshi", "trigger_shichuang", "trigger_lianjianshi", "trigger_biguanshi", "trigger_danfang",
                                 "trigger_jingshimen", "facility_danlu_wz", "facility_yaoyuan_wz", "trigger_yanghun"})
            if (!touches(yard, at("ch13_wuzhong", name))) out.push_back(std::string(name) + " is outside the inner band");
        for (const char* name : {"trigger_chuguan", "andeng"})
            if (touches(yard, at("ch13_wuzhong", name))) out.push_back(std::string(name) + " is inside the inner band");
    }
    return out;
}

TEST(Ch13TriggerMode, BandsLandingsSaveSpotsAndTheCaveYardSitWhereTheDesignPutsThem) {
    const auto maps = shippedMaps();
    const auto errors = layoutErrors(maps, scripts());
    EXPECT_TRUE(errors.empty()) << joined(errors);
    const auto facilities = facilityErrors(maps);
    EXPECT_TRUE(facilities.empty()) << joined(facilities);
}

TEST(Ch13TriggerMode, MutationsOfTheLayoutAreCaught) {
    const auto original = shippedMaps();
    const auto sources = scripts();
    ASSERT_TRUE(layoutErrors(original, sources).empty()) << joined(layoutErrors(original, sources));
    struct Move { const char* map; const char* object; Cell to; };
    for (const auto& move : {Move{"ch13_nanming", "trigger_wenxun", {5, 29}},          // 带压到 spawn 那一行
                             Move{"ch13_tianxing", "trigger_rucheng", {2, 2}},           // 带不再挡路
                             Move{"ch13_waihai", "trigger_hui_wuzhong", {8, 12}},        // 与西门带只隔两格、落点不在两者之间
                             Move{"ch13_wuzhong", "trigger_qiyuan_hang", {56, 15}},      // 贴着码头带
                             Move{"ch13_wuzhong", "trigger_shoushou", {7, 31}},          // 贴着阵心
                             Move{"ch13_waihai", "facility_cunji_mishi", {13, 40}},
                             Move{"ch13_wuzhong", "trigger_chuguan", {33, 15}},          // 与 dizhen 同一行
                             Move{"ch13_nanming", "facility_dazuo_nanming", {46, 14}},   // 打坐处挪到出海口外
                             Move{"ch13_wuzhong", "trigger_jingshimen", {58, 30}},       // 静室门挪出院子
                             Move{"ch13_waihai", "trigger_jiaoshi", {7, 12}}}) {         // 压在 #6 落点上
        auto maps = original;
        moveObject(maps[move.map], move.object, move.to);
        EXPECT_FALSE(layoutErrors(maps, sources).empty()) << move.object;
    }
    // T-7：设施也看。旧灵田、静室挪到城门外 spawn 那一侧 (22,39)（带外够得着、格上无对象）→ 恰报这一处、点名 rucheng；
    // 紧挨 spawn 的存档点本就在带内、不报（上面的基线 0 条即对照）。
    for (const char* name : {"facility_yaoyuan_39", "facility_jingshi_39"}) {
        auto maps = original;
        moveObject(maps["ch13_tianxing"], name, {22, 39});
        const auto errors = layoutErrors(maps, sources);
        EXPECT_EQ(errors.size(), 1u) << name << joined(errors);
        EXPECT_NE(joined(errors).find(std::string(name) + " reachable without crossing trigger_rucheng"), std::string::npos)
            << joined(errors);
    }
    struct Hop { const char* script; const char* from; const char* to; };
    for (const auto& hop : {Hop{"lidao", "teleport(\"ch13_waihai\", 14, 46)", "teleport(\"ch13_waihai\", 14, 45)"},
                            Hop{"qiyuan_hang", "teleport(\"ch13_waihai\", 7, 12)", "teleport(\"ch13_waihai\", 10, 12)"}}) {
        auto edited = sources;
        const auto found = findInCode(edited[hop.script], hop.from);
        ASSERT_NE(found, std::string::npos) << hop.script;
        edited[hop.script].replace(found, std::string(hop.from).size(), hop.to);
        EXPECT_FALSE(layoutErrors(original, edited).empty()) << hop.script;
    }
    ASSERT_TRUE(facilityErrors(original).empty()) << joined(facilityErrors(original));
    struct Bad { const char* map; const char* object; const char* key; Json value; };
    for (const auto& bad : {Bad{"ch13_wuzhong", "facility_jingshi_wz", "effectiveness", 100},
                            Bad{"ch13_nanming", "facility_dazuo_nanming", "effectiveness", 100},   // 「不写」也要钉
                            Bad{"ch13_tianxing", "facility_yaoyuan_39", "deny_text_key", "ch13.block.yaoyuan"},
                            Bad{"ch13_tianxing", "facility_yaoyuan_39", "ref_id", "field_tianxing_39b"},
                            Bad{"ch13_tianxing", "facility_jingshi_39", "require_flag", "ch13.qifu"},
                            Bad{"ch13_wuzhong", "facility_danlu_wz", "grade", 3},
                            Bad{"ch13_wuzhong", "facility_yaoyuan_wz", "slots", 4}}) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(facilityErrors(maps).empty()) << bad.object << " " << bad.key;
    }
    auto shop = original;
    addCopy(shop["ch13_waihai"], "facility_cunji_qiyuan", "facility_shop_qiyuan");
    setProp(shop["ch13_waihai"], "facility_shop_qiyuan", "kind", "shop");
    EXPECT_FALSE(facilityErrors(shop).empty()) << "验收 12：新图上多一个商店设施";
    auto forge = original;
    addCopy(forge["ch13_wuzhong"], "facility_danlu_wz", "facility_lianqi_wz");
    setProp(forge["ch13_wuzhong"], "facility_lianqi_wz", "kind", "forge");
    EXPECT_FALSE(facilityErrors(forge).empty()) << "E 表 17：无炼器台";
}

// ---- 第 16 条：无门、无遭遇区、旧图 0 处修补 ----
Errors doorErrors(const Maps& ours, const Maps& older) {
    Errors out;
    for (const auto& [mapId, map] : ours)
        for (const auto& [name, object] : objects(map)) {
            const std::string type = typeOf(object);
            if (type == "portal") out.push_back(mapId + "/" + name + " is a portal (chapter 13 has none)");
            if (type == "encounter") out.push_back(mapId + "/" + name + " encounter zone (8.4)");
        }
    for (const auto& [mapId, map] : older)
        for (const auto& [name, object] : objects(map)) {
            const Json p = properties(object);
            if (p.value("script", "").rfind("ch13/", 0) == 0) out.push_back(mapId + "/" + name + " hooks a chapter-13 script");
            if (p.value("target_map", "").rfind("ch13_", 0) == 0) out.push_back(mapId + "/" + name + " leads into chapter 13");
        }
    return out;
}

Maps olderMaps() {
    Maps maps;
    for (const auto& file : fs::directory_iterator(root() / "maps")) {
        const std::string stem = file.path().stem().string();
        if (file.path().extension() == ".tmj" && stem.rfind("ch13_", 0) != 0) maps[stem] = json(file.path());
    }
    return maps;
}

TEST(Ch13TriggerMode, NoDoorNoEncounterZoneAndNoOldMapPointsIntoChapterThirteen) {
    const auto ours = shippedMaps();
    const auto older = olderMaps();
    ASSERT_GE(older.size(), 75u) << "先验：第 1–12 章 75 张图都读到了（第 4 节：全作 75 → 79）";
    const auto errors = doorErrors(ours, older);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    auto door = ours;
    addCopy(door["ch13_wuzhong"], "trigger_qiyuan_hang", "portal_qiyuan");
    (*findObject(door["ch13_wuzhong"], "portal_qiyuan"))["type"] = "portal";
    EXPECT_FALSE(doorErrors(door, older).empty()) << "雾中岛多一扇门";
    auto zone = ours;
    addCopy(zone["ch13_waihai"], "trigger_jiaoshi", "encounter_shenhai");
    (*findObject(zone["ch13_waihai"], "encounter_shenhai"))["type"] = "encounter";
    EXPECT_FALSE(doorErrors(zone, older).empty()) << "外海多一片遭遇区";
    auto patched = older;
    ASSERT_TRUE(patched.count("ch12_lingquan"));
    setProp(patched["ch12_lingquan"], "npc_yuan_yao_lq", "script", "ch13/wenxun.lua");
    EXPECT_FALSE(doorErrors(ours, patched).empty()) << "旧图挂上第 13 章脚本";
}

// ---- 第 13 条与 16.1 第 60 条：往返两份脚本（触发器属性归上面的挂点表）----
// 顶格：这一行从第 0 列起就是它，前面没有缩进、也没有同一行的条件（与 ObjectiveTests 的 hasTopLevel 同口径）。
bool topLevel(const std::string& code, const std::regex& line) {
    std::istringstream in(code);
    for (std::string text; std::getline(in, text);)
        if (std::regex_match(text, line)) return true;
    return false;
}

std::size_t where(const std::string& code, const std::string& needle) {
    const auto found = code.find(needle);
    return found == std::string::npos ? code.size() : found;
}

Errors roundTripErrors(const Sources& sources) {
    Errors out;
    struct Trip { const char* script; const char* closedBy; const char* teleport; };
    for (const auto& trip : {Trip{"qiyuan_hang", "ch13.biguan", "teleport(\"ch13_waihai\", 7, 12)"},
                             Trip{"hui_wuzhong", "ch13.lidao", "teleport(\"ch13_wuzhong\")"}}) {
        const std::string name = trip.script;
        const auto found = sources.find(name);
        if (found == sources.end()) { out.push_back(name + ".lua missing"); continue; }
        const std::string code = executableLines(found->second);
        const auto lines = liveLines(found->second);
        const std::string gate = std::string("if flag.get(\"") + trip.closedBy + "\") ~= 0 then";
        if (lines.empty() || lines[0].rfind(gate, 0) != 0 || lines[0].find("return") == std::string::npos)
            out.push_back(name + ".lua does not open on " + trip.closedBy);
        if (code.find("flag.set(") != std::string::npos) out.push_back(name + ".lua sets a flag");
        if (countMatches(code, std::regex(R"lua(\badvance_days\s*\()lua")) != 1 || !topLevel(code, std::regex(R"(advance_days\(12\)\s*)")))
            out.push_back(name + ".lua does not spend exactly one top-level advance_days(12)");
        if (countMatches(code, std::regex(R"lua(\bteleport\s*\()lua")) != 1 || code.find(std::string("\n") + trip.teleport + "\n") == std::string::npos)
            out.push_back(name + ".lua does not end on one top-level " + trip.teleport);
        if (!topLevel(code, std::regex(R"(if (\w+) ~= 1 then return end\s*)")))
            out.push_back(name + ".lua cancel is not a top-level return");
        const std::size_t ask = where(code, "choice("), spend = where(code, "advance_days("), hop = where(code, "teleport(");
        if (!(ask < spend && spend < hop && hop < code.size())) out.push_back(name + ".lua order is not choice → days → teleport");
    }
    return out;
}

TEST(Ch13TriggerMode, TheTwoRoundTripsAskSpendTwelveDaysAndHopAtTopLevel) {
    const auto original = scripts();
    const auto errors = roundTripErrors(original);
    EXPECT_TRUE(errors.empty()) << joined(errors);
    struct Bad { const char* script; const char* from; const char* to; };
    for (const auto& bad : {Bad{"qiyuan_hang", "advance_days(12)", "advance_days(10)"},
                            Bad{"hui_wuzhong", "advance_days(12)", "if go == 1 then advance_days(12) end"},
                            Bad{"qiyuan_hang", "if go ~= 1 then return end", "if go == 1 then"},   // 取消不再顶格 return
                            Bad{"hui_wuzhong", "teleport(\"ch13_wuzhong\")", "flag.set(\"ch13.lifu\")\nteleport(\"ch13_wuzhong\")"},
                            Bad{"qiyuan_hang", "if flag.get(\"ch13.biguan\") ~= 0", "if flag.get(\"ch13.chuguan\") ~= 0"},
                            Bad{"hui_wuzhong", "teleport(\"ch13_wuzhong\")", "    teleport(\"ch13_wuzhong\")"}}) {
        auto sources = original;
        const auto at = findInCode(sources[bad.script], bad.from);
        ASSERT_NE(at, std::string::npos) << bad.script << ": " << bad.from;
        sources[bad.script].replace(at, std::string(bad.from).size(), bad.to);
        EXPECT_FALSE(roundTripErrors(sources).empty()) << bad.script << " <- " << bad.to;
    }
}

// ---- 第 17 条（读地图那一半）与第 12 条：在场旗标照 5.1、同时在场的名牌两两不同 ----
// 旗标在主线上的先后：目标链第 k 步记 2k；链外的 ch13.shouchao 插在 chuguan 与 gongsun 之间（第 11 节末表）。
int stamp(const std::string& flag) {
    if (flag.empty()) return 1 << 20;
    if (flag == "ch13.shouchao") {
        const auto chuguan = std::find(kChain.begin(), kChain.end(), std::string("ch13.chuguan"));
        return 2 * static_cast<int>(chuguan - kChain.begin()) + 1;
    }
    for (std::size_t i = 0; i < kChain.size(); ++i)
        if (flag == kChain[i]) return 2 * static_cast<int>(i);
    return -1;
}

Errors presenceErrors(const Maps& maps, const std::map<std::string, std::string>& plates) {
    Errors out;
    std::set<std::string> designed;
    for (const auto& row : kPresence) {
        designed.insert(std::string(row.map) + "/" + row.object);
        const auto all = objects(maps.at(row.map));
        const auto found = all.find(row.object);
        if (found == all.end() || typeOf(found->second) != "npc") { out.push_back(std::string("missing npc ") + row.object); continue; }
        const Json p = properties(found->second);
        if (p.value("role_id", "") != row.role) out.push_back(std::string(row.object) + " role_id");
        if (p.value("visible_flag", "") != row.visible) out.push_back(std::string(row.object) + " visible_flag");
        if (p.value("hidden_flag", "") != row.hidden || (*row.hidden == '\0' && p.contains("hidden_flag")))
            out.push_back(std::string(row.object) + " hidden_flag");
    }
    for (const auto& [mapId, map] : maps) {
        std::vector<std::pair<std::string, Json>> npcs;
        for (const auto& [name, object] : objects(map)) {
            if (typeOf(object) != "npc") continue;
            if (!designed.count(mapId + "/" + name)) out.push_back("undesigned npc " + mapId + "/" + name);
            npcs.push_back({name, properties(object)});
        }
        for (std::size_t i = 0; i < npcs.size(); ++i)
            for (std::size_t j = i + 1; j < npcs.size(); ++j) {
                const Json& a = npcs[i].second;
                const Json& b = npcs[j].second;
                const int from = std::max(stamp(a.value("visible_flag", "")), stamp(b.value("visible_flag", "")));
                const int to = std::min(stamp(a.value("hidden_flag", "")), stamp(b.value("hidden_flag", "")));
                if (from >= to) continue;   // 不会同时在场
                const std::string ra = a.value("role_id", ""), rb = b.value("role_id", "");
                if (ra == rb) out.push_back(mapId + " " + npcs[i].first + " / " + npcs[j].first + " overlap as " + ra);
                const auto pa = plates.find(ra), pb = plates.find(rb);
                if (pa == plates.end() || pb == plates.end() || pa->second == pb->second)
                    out.push_back(mapId + " " + npcs[i].first + " / " + npcs[j].first + " share a name plate");
            }
    }
    return out;
}

std::map<std::string, std::string> plates() {
    std::map<std::string, std::string> names;
    for (const auto& row : kPresence)
        names[row.role] = json(root() / "data" / "roles" / (std::string(row.role) + ".json")).at("name").get<std::string>();
    return names;
}

TEST(Ch13TriggerMode, EveryNpcComesAndGoesOnTheFlagsOfSectionFiveOne) {
    const auto errors = presenceErrors(shippedMaps(), plates());
    EXPECT_TRUE(errors.empty()) << joined(errors);
    EXPECT_EQ(stamp("ch13.shouchao"), 55) << "先验：shouchao 在 chuguan（54）与 gongsun（56）之间";
}

TEST(Ch13TriggerMode, MutationsOfPresenceAreCaught) {
    const auto original = shippedMaps();
    const auto names = plates();
    ASSERT_TRUE(presenceErrors(original, names).empty()) << joined(presenceErrors(original, names));
    struct Bad { const char* map; const char* object; const char* key; const char* value; };
    for (const auto& bad : {Bad{"ch13_waihai", "npc_paimaishi", "visible_flag", "ch13.chuansong"},
                            Bad{"ch13_waihai", "npc_gu_yu", "hidden_flag", "ch13.gongsun"},        // 验收 12：随兽潮撤
                            Bad{"ch13_tianxing", "npc_yi_jing_tx", "hidden_flag", "ch13.daoqiyuan"},
                            Bad{"ch13_nanming", "npc_yuanliao_zhanggui", "hidden_flag", "ch13.gongdao"},
                            Bad{"ch13_wuzhong", "npc_sun_shishu", "role_id", "gongsun_xing"}}) {
        auto maps = original;
        setProp(maps[bad.map], bad.object, bad.key, bad.value);
        EXPECT_FALSE(presenceErrors(maps, names).empty()) << bad.object << " " << bad.key;
    }
    auto extra = original;
    addCopy(extra["ch13_waihai"], "npc_xu_yun", "npc_xu_yun_2");
    EXPECT_FALSE(presenceErrors(extra, names).empty()) << "表外多一个 NPC";
    auto twin = names;
    twin["jingbao_paimaishi"] = names.at("ming_furen");
    EXPECT_FALSE(presenceErrors(original, twin).empty()) << "明珠轩里两人同名牌";
    auto apart = names;
    apart["huang_mingli"] = names.at("xu_yun");
    EXPECT_TRUE(presenceErrors(original, apart).empty()) << "对照：许云撤场之后黄明礼才上图，同名牌不算同时在场";
}

// ---- 第 5 条、第 20 条、第 21 条、第 27 条（扫脚本那一半）----
Errors finaleErrors(const Sources& sources, const Json& chapters) {
    Errors out;
    const auto code = [&](const char* name) {
        const auto found = sources.find(name);
        return found == sources.end() ? std::string() : executableLines(found->second);
    };
    const auto lines = [&](const char* name) {
        const auto found = sources.find(name);
        return found == sources.end() ? std::vector<std::string>{} : liveLines(found->second);
    };
    // 第 20 条：banyao.lua 末句置 ch13.done、伴妖草在它之前给、不 ending()、不 teleport（第 14 章 patch 本脚本）。
    const std::string banyao = code("banyao");
    const auto last = lines("banyao");
    if (last.empty() || last.back() != "flag.set(\"ch13.done\")") out.push_back("banyao.lua does not end on ch13.done");
    if (where(banyao, "give(\"story_banyao_cao\", 1)") >= where(banyao, "flag.set(\"ch13.done\")"))
        out.push_back("banyao.lua does not give the grass before ch13.done");
    if (banyao.find("ending(") != std::string::npos || banyao.find("teleport(") != std::string::npos)
        out.push_back("banyao.lua calls ending() or teleport");
    // 第 5 条：chuguan.lua 第一句就判境界，不到只一句 return（不拨日、不置旗）。
    const auto gate = lines("chuguan");
    if (gate.empty() || gate[0].rfind("if not realm.at_least(realm.CORE_MID) then", 0) != 0 ||
        gate[0].find("return") == std::string::npos || gate[0].find("advance_days") != std::string::npos ||
        gate[0].find("flag.set") != std::string::npos)
        out.push_back("chuguan.lua does not open on realm.at_least(realm.CORE_MID)");
    // 第 27 条：两处确认在拨日、置旗之前，取消即 return（16.1 第 52 条）。
    for (const char* name : {"biguan", "lidao"}) {
        const std::string text = code(name);
        const std::size_t ask = where(text, "choice("), spend = where(text, "advance_days("),
                          stampAt = where(text, std::string("flag.set(\"ch13.") + name + "\")");
        if (!(ask < spend && spend < stampAt && stampAt < text.size()) || text.substr(ask, spend - ask).find("return") == std::string::npos)
            out.push_back(std::string(name) + ".lua does not confirm before spending days and setting its flag");
    }
    {   // 3.2 n27：拨日 → realm.cap → 置 ch13.biguan（抬上限在置旗之前；不到置旗就不算闭关）。
        const std::string text = code("biguan");
        const std::size_t cap = where(text, "realm.cap("), spend = where(text, "advance_days(");
        if (!(spend < cap && cap < where(text, "flag.set(\"ch13.biguan\")"))) out.push_back("biguan.lua does not raise the cap between the days and its flag");
    }
    if (!topLevel(code("lidao"), std::regex(R"(if (\w+) ~= 1 then return end\s*)"))) out.push_back("lidao.lua cancel is not top-level");
    // 第 21 条：第 13 项照 10.1 C1 字面量。
    const auto& table = chapters.at("chapters");
    const Json thirteen{{"number", 13}, {"numeral_key", "ui.chapter.13.numeral"}, {"title_key", "ui.chapter.13.title"}, {"done_flag", "ch13.done"}};
    if (table.size() < 13 || table[12] != thirteen) out.push_back("chapter 13 is not the thirteenth entry of data/chapters.json as C1 wrote it");
    return out;
}

TEST(Ch13TriggerMode, TheChapterEndsOnTheDoneFlagAndTheTwoPointsOfNoReturnAskFirst) {
    const auto errors = finaleErrors(scripts(), json(root() / "data" / "chapters.json"));
    EXPECT_TRUE(errors.empty()) << joined(errors);
}

TEST(Ch13TriggerMode, MutationsOfTheFinaleAndTheConfirmationsAreCaught) {
    const auto original = scripts();
    const Json chapters = json(root() / "data" / "chapters.json");
    ASSERT_TRUE(finaleErrors(original, chapters).empty()) << joined(finaleErrors(original, chapters));
    struct Bad { const char* script; const char* from; const char* to; };   // from 为空 = 末尾追加 to
    for (const auto& bad : {Bad{"banyao", "", "teleport(\"ch14_fengxi\")\n"},
                            Bad{"banyao", "", "talk(\"\", \"ch13.banyao.after\")\n"},
                            Bad{"banyao", "", "ending(\"t\", \"b\")\n"},
                            Bad{"chuguan", "if not realm.at_least(realm.CORE_MID) then", "advance_days(1)\nif not realm.at_least(realm.CORE_MID) then"},
                            Bad{"chuguan", "realm.at_least(realm.CORE_MID)", "realm.at_least(realm.CORE_EARLY)"},
                            Bad{"biguan", "local pick = choice(", "advance_days(30)\nlocal pick = choice("},
                            Bad{"biguan", "local ok = realm.cap(", "flag.set(\"ch13.biguan\")\nlocal ok = realm.cap("},
                            Bad{"lidao", "if go ~= 1 then return end", "if go == 2 then return end"},
                            Bad{"lidao", "local go = choice(", "advance_days(20)\nlocal go = choice("}}) {
        auto sources = original;
        std::string& text = sources[bad.script];
        if (*bad.from == '\0') {
            text += bad.to;
        } else {
            const auto at = findInCode(text, bad.from);
            ASSERT_NE(at, std::string::npos) << bad.script << ": " << bad.from;
            text.replace(at, std::string(bad.from).size(), bad.to);
        }
        EXPECT_FALSE(finaleErrors(sources, chapters).empty()) << bad.script << " <- " << bad.to;
    }
    auto moved = original;
    const auto give = moved["banyao"].find("give(\"story_banyao_cao\", 1)\n");
    ASSERT_NE(give, std::string::npos);
    moved["banyao"].erase(give, std::string("give(\"story_banyao_cao\", 1)\n").size());
    moved["banyao"] = std::regex_replace(moved["banyao"], std::regex(R"lua(flag\.set\("ch13\.done"\))lua"), "give(\"story_banyao_cao\", 1)\nflag.set(\"ch13.done\")");
    EXPECT_TRUE(finaleErrors(moved, chapters).empty()) << "对照：伴妖草挪到紧挨置旗之前仍合法";
    auto renamed = chapters;
    renamed["chapters"][12]["title_key"] = "ui.chapter.13.name";
    EXPECT_FALSE(finaleErrors(original, renamed).empty());
    auto shorter = chapters;
    shorter["chapters"].erase(std::size_t{12});
    EXPECT_FALSE(finaleErrors(original, shorter).empty());
}
}  // namespace
