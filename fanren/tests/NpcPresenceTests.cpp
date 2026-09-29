// 具名人物任一时刻只在一张图上。
//
// ---------------------------------------------------------------------------
// 钉的是什么
// ---------------------------------------------------------------------------
// 很多 NPC 没挂在场旗标：张铁同时站在彩霞山、炼骨崖、居所三张图上，墨大夫死后还在
// 炼骨崖与神手谷，第 4 章厉飞雨、卖药郎各在两处。玩家一张图一张图地走，每张图单看都
// 对，拼起来是一个人分身几处——任何只看一张图的检查都看不出来。
//
// 判据：沿主线从空存档起一步一步往前推，每个检查点上把全部地图的全部 npc 过一遍，
// 用 WorldScene::npcVisible（画面、占格、对话问的都是它）判在不在场，按 role_id 汇总
// 出现在哪几张图。除群像白名单（kCrowdRoles）外，每个 role 同一时刻最多在一张图上。
// 同一张图上同一个 role 摆几个不算（拿一个模板摆出来的几个路人、同一张图上前后换位的
// 一对都是这样），只数图。
//
// ---------------------------------------------------------------------------
// 为什么这样钉
// ---------------------------------------------------------------------------
// 1. **时间线走真的目标链。** data/objectives/ch01–ch05 经真加载器（io::loadGameData，
//    与 HUD、告示板读的是同一份）读进来，按章、按章内次序把每一步的 done_flag 依次置 1；
//    空存档检查一次，每一步之后各检查一次。目标链的 done_flag 就是剧情脚本自己置的完成旗标
//   （core::Objective 的注释），所以这条时间线就是玩家实际走过的那条。
//
// 2. **目标链之外的旗标，写成一张显式的表（kOffChainFlags），并且让测试自己去核。**
//    有几个旗标不是哪一步的 done_flag，却挂在 NPC 的 visible_flag / hidden_flag 上。
//    表里每一条写明「插在目标链哪一步之后、下一步之前」与「哪个脚本置它」，
//    EveryOffChainFlagIsSetWhereTheTableSays 拿目标链与那个脚本的原文逐条核（offChainProblems）：
//      · 那个脚本里确实有 flag.set("那个旗标"（注释里的不算）；
//      · 「之后」「之前」两步都在目标链上，而且紧挨着——窗口过期了（中间插进了新的一步）就红，
//        逼人回来重新确认它该插在哪；
//      · 脚本若也置了窗口两头那一步的 done_flag，先后次序要与表对得上。墨大夫之死
//        （ch03.mo_siwang）就是这么排的：chujue.lua 里它**先于** ch03.yuzitong_chujue 置，
//        所以插在处决那一步**之前**。插在之后，会凭空造出「处决已毕、墨大夫却还活着」
//        这个真机上不存在的存档，拿它考 NPC 在不在场只会考出假红灯。
//
// 3. **覆盖先验。** 凡是地图上 NPC 用到的 visible_flag / hidden_flag，都必须在这条时间线上
//    被置过，否则挂着它的 NPC 在整条时间线上只露了一种面目，根本没被考到——测试红，并列出
//    是哪几个旗标、挂在谁身上。不悄悄跳过。
//
// 4. **判据写成纯函数，另有手搭数据的自检。**「一组地图 + 一个状态 → 冲突清单」是
//    presenceConflicts；NpcPresence 那几条用内存里现搭的两张图证明它有牙：同 role、
//    无旗标的一对必报，互补旗标的一对（在两种状态下都恰好只有一个在场）必不报，
//    白名单里的不报、白名单外的照报。核表的 offChainProblems 也一样：拿手写的脚本原文
//    喂它，注释里的、前缀撞名的、次序反了的、窗口不挨着的，各报一条、报的是那件事。
//
// 5. **交接存档也拿来当检查点考**（独立审查 M1，本文件末尾那一节，理由写在那里）：
//    tests/fixtures/chNN-end-*.sav 必须真的走过第 1..NN 章，在场名单必须与主线走到同一章末
//    时一致。上面那条时间线是按链自己置旗标的，从不读这些存档，抓不到它们漂开。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/model/Types.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
#include "io/SaveFile.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Objective;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::WorldScene;

// ---------------------------------------------------------------------------
// 群像白名单
// ---------------------------------------------------------------------------
// 设计上就是「一群人」或「某一类人」的 role：同一时刻站在几张图上是本意。每加一个都要
// 写明理由——拿它去压一个具名人物的一人多处，等于把这条用例关掉。
constexpr const char* kCrowdRoles[] = {
    // 墨府护院：南城墨府门口站四个、墨府月洞门里夜间站一个暗哨，共用这一个角色模板
    //（data/roles/mofu_huyuan.json 的 note）。是一群护院，不是一个人。
    "mofu_huyuan",
    // 七玄门考官：青牛镇招徒处与炼骨崖考核处各站一位。原著没给考官具名，这是「七玄门来的
    // 考官」这一类人的通用模板（data/roles/qixuan_kaoguan.json 的 note），不是同一个人。
    "qixuan_kaoguan",
};

// ---------------------------------------------------------------------------
// 目标链之外、却挂在 NPC 在场旗标上的旗标（文件头第 2 条）
// ---------------------------------------------------------------------------
struct OffChainFlag {
    const char* flag;
    const char* after;    // 目标链上这一步的 done_flag 置位之后……
    const char* before;   // ……紧接着的下一步置位之前
    const char* script;   // 置它的那个脚本，相对仓库根
};

constexpr OffChainFlag kOffChainFlags[] = {
    // 第 1 章收尾：shenshougu_koujue.lua 末尾先置 ch01.koujue_received，紧跟着置它。
    {"ch01.done", "ch01.koujue_received", "ch02.renyao_done", "scripts/ch01/shenshougu_koujue.lua"},
    // 厉飞雨欠下的那份人情：zhitong.lua 里先置它，再置 ch02.duan4_start。
    {"ch02.renqing_jiexia", "ch02.chousui_jian", "ch02.duan4_start", "scripts/ch02/zhitong.lua"},
    // 墨大夫下山、自此不在药圃现身：ceng3.lua 开头就要 ch02.duan5_start，试药之前演。
    {"ch02.xiangqi_ping", "ch02.duan5_start", "ch02.shiyao_done", "scripts/ch02/ceng3.lua"},
    // 墨大夫之死：与 ch03.yuzitong_chujue 同在 chujue.lua，但先于它置——所以插在处决那一步
    // 之前（夺舍 ch03.shihai_done 之后），理由见文件头第 2 条。
    {"ch03.mo_siwang", "ch03.shihai_done", "ch03.yuzitong_chujue", "scripts/ch03/chujue.lua"},
};

// 目标链至少要覆盖到的章：少了一章，那一章的 NPC 就没被考到。第 6 章目标链 2026-09-29 落地（data/objectives/ch06.json）。
constexpr int kFirstChapter = 1;
constexpr int kLastChapter = 6;

// 与 tests/WorldViewTests.cpp 同一个找法。
std::string repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "maps" / "ch01_hanjiacun.tmj")) return candidate;
    }
    return ".";
}

// ---------------------------------------------------------------------------
// 判据（纯函数）
// ---------------------------------------------------------------------------

// 一个在场的人站在哪：哪张图上的哪个对象。
struct Placement {
    std::string map;
    std::string npc;
    friend bool operator==(const Placement&, const Placement&) = default;
};

// 此刻在场的人，按 role_id 归拢（role 按字典序，同一 role 内按地图、对象表的次序）。
// 没写 role_id 的 npc 不是具名人物，不收（validate.py 要求 npc 必须有 role_id）。
std::map<std::string, std::vector<Placement>> rolePlacements(const std::vector<TileMap>& maps,
                                                             const GameState& state) {
    std::map<std::string, std::vector<Placement>> out;
    for (const TileMap& map : maps) {
        for (const MapObject& object : map.objects) {
            if (object.type != "npc") continue;
            if (!WorldScene::npcVisible(state, object)) continue;
            const std::string role = object.property("role_id");
            if (role.empty()) continue;
            out[role].push_back(Placement{map.id, object.name});
        }
    }
    return out;
}

// 同一时刻站在不止一张图上的一个 role。
struct PresenceConflict {
    std::string role;
    std::vector<std::string> maps;         // 在哪几张图上（排好、去重）
    std::vector<Placement> placements;     // 具体是哪几个对象，给人看
};

// 一组地图 + 一个状态 → 冲突清单（按 role 排好）。crowd 里的 role 不报。
std::vector<PresenceConflict> presenceConflicts(const std::vector<TileMap>& maps, const GameState& state,
                                                const std::set<std::string>& crowd) {
    std::vector<PresenceConflict> out;
    for (const auto& [role, placements] : rolePlacements(maps, state)) {
        if (crowd.count(role) != 0) continue;
        std::set<std::string> where;
        for (const Placement& placement : placements) where.insert(placement.map);
        if (where.size() < 2) continue;
        out.push_back(PresenceConflict{role, std::vector<std::string>(where.begin(), where.end()), placements});
    }
    return out;
}

std::set<std::string> crowdRoles() {
    return std::set<std::string>(std::begin(kCrowdRoles), std::end(kCrowdRoles));
}

// ---------------------------------------------------------------------------
// 时间线
// ---------------------------------------------------------------------------

struct Checkpoint {
    std::string label;
    GameState state;
};

std::string stepLabel(const Objective& step) {
    return "第 " + std::to_string(step.chapter) + " 章 " + step.id;
}

// 空存档起，按目标链依次置 done_flag；表里的旗标插在它声称的那一步之后。
// 每置一个旗标就是一个检查点（同一个脚本先后置的两个旗标之间那一刻也算：真机上它存在）。
std::vector<Checkpoint> mainLine(const std::vector<Objective>& chain,
                                 const std::vector<OffChainFlag>& offChain) {
    std::vector<Checkpoint> out;
    GameState state;
    out.push_back(Checkpoint{"初始（空存档）", state});
    for (const Objective& step : chain) {
        state.setFlag(step.doneFlag, 1);
        out.push_back(Checkpoint{stepLabel(step) + "（" + step.doneFlag + "）之后", state});
        for (const OffChainFlag& extra : offChain) {
            if (step.doneFlag != extra.after) continue;
            state.setFlag(extra.flag, 1);
            out.push_back(Checkpoint{stepLabel(step) + " 之后又置 " + extra.flag, state});
        }
    }
    return out;
}

// 同一个冲突（同一个 role、同样那几处）连着出现的一段检查点。报错按段报，免得一个人
// 在八十几个检查点上各报一遍。
struct ConflictRun {
    PresenceConflict conflict;
    std::size_t firstIndex = 0;
    std::string first;
    std::string last;
    int count = 0;
};

std::vector<ConflictRun> conflictRuns(const std::vector<TileMap>& maps, const std::vector<Checkpoint>& line,
                                      const std::set<std::string>& crowd) {
    std::map<std::string, ConflictRun> open;
    std::vector<ConflictRun> closed;
    for (std::size_t i = 0; i < line.size(); ++i) {
        std::set<std::string> seen;
        for (PresenceConflict& conflict : presenceConflicts(maps, line[i].state, crowd)) {
            seen.insert(conflict.role);
            const auto it = open.find(conflict.role);
            if (it != open.end() && it->second.conflict.placements == conflict.placements) {
                it->second.last = line[i].label;
                ++it->second.count;
                continue;
            }
            if (it != open.end()) {
                closed.push_back(std::move(it->second));
                open.erase(it);
            }
            const std::string role = conflict.role;
            open.emplace(role, ConflictRun{std::move(conflict), i, line[i].label, line[i].label, 1});
        }
        for (auto it = open.begin(); it != open.end();) {
            if (seen.count(it->first) != 0) {
                ++it;
                continue;
            }
            closed.push_back(std::move(it->second));
            it = open.erase(it);
        }
    }
    for (auto& entry : open) closed.push_back(std::move(entry.second));
    std::sort(closed.begin(), closed.end(), [](const ConflictRun& a, const ConflictRun& b) {
        return a.conflict.role != b.conflict.role ? a.conflict.role < b.conflict.role
                                                  : a.firstIndex < b.firstIndex;
    });
    return closed;
}

std::string describeConflict(const PresenceConflict& conflict) {
    std::ostringstream out;
    out << "「" << conflict.role << "」同时站在 " << conflict.maps.size() << " 张图上：";
    for (std::size_t i = 0; i < conflict.maps.size(); ++i) out << (i == 0 ? "" : "、") << conflict.maps[i];
    out << "（";
    for (std::size_t i = 0; i < conflict.placements.size(); ++i) {
        out << (i == 0 ? "" : "，") << conflict.placements[i].map << "/" << conflict.placements[i].npc;
    }
    out << "）";
    return out.str();
}

std::string describeRun(const ConflictRun& run) {
    std::ostringstream out;
    out << describeConflict(run.conflict) << "——检查点「" << run.first << "」";
    if (run.count > 1) out << " 至「" << run.last << "」，连续 " << run.count << " 个";
    return out.str();
}

// 地图上 NPC 用到的在场旗标 → 挂着它的那几处（「图/对象 的 visible_flag」）。
std::map<std::string, std::vector<std::string>> npcFlagUsers(const std::vector<TileMap>& maps) {
    std::map<std::string, std::vector<std::string>> out;
    for (const TileMap& map : maps) {
        for (const MapObject& object : map.objects) {
            if (object.type != "npc") continue;
            for (const char* key : {"visible_flag", "hidden_flag"}) {
                const std::string flag = object.property(key);
                if (!flag.empty()) out[flag].push_back(map.id + "/" + object.name + " 的 " + key);
            }
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// 读脚本
// ---------------------------------------------------------------------------

std::string readText(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// 脚本里第一处真的会执行的 flag.set("<flag>"：同一行 -- 之后的（Lua 注释）不算。没有返回 npos。
// 带上收尾的引号，ch01.done 不会误中 ch01.done_xxx。
std::size_t liveFlagSet(const std::string& text, const std::string& flag) {
    const std::string needle = "flag.set(\"" + flag + "\"";
    for (std::size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) {
        const std::size_t newline = at == 0 ? std::string::npos : text.rfind('\n', at - 1);
        const std::size_t lineStart = newline == std::string::npos ? 0 : newline + 1;
        if (text.substr(lineStart, at - lineStart).find("--") == std::string::npos) return at;
    }
    return std::string::npos;
}

std::size_t stepIndex(const std::vector<Objective>& chain, const std::string& doneFlag) {
    for (std::size_t i = 0; i < chain.size(); ++i) {
        if (chain[i].doneFlag == doneFlag) return i;
    }
    return chain.size();
}

// 表里一条，对着目标链与它声称的那个脚本的原文核（文件头第 2 条）。返回问题清单，空 = 对得上。
std::vector<std::string> offChainProblems(const OffChainFlag& extra, const std::vector<Objective>& chain,
                                          const std::string& scriptText) {
    std::vector<std::string> problems;
    const std::string flag = extra.flag;
    const std::string after = extra.after;
    const std::string before = extra.before;
    const std::string script = extra.script;

    const std::size_t own = liveFlagSet(scriptText, flag);
    if (own == std::string::npos) {
        problems.push_back(script + " 里没有 flag.set(\"" + flag + "\"（注释里的不算）——表里说它在这里置");
    }

    // 窗口：「之后」「之前」两步都在目标链上，而且紧挨着。
    const std::size_t afterAt = stepIndex(chain, after);
    const std::size_t beforeAt = stepIndex(chain, before);
    if (afterAt == chain.size()) problems.push_back(after + " 不是目标链上任何一步的 done_flag");
    if (beforeAt == chain.size()) problems.push_back(before + " 不是目标链上任何一步的 done_flag");
    if (afterAt < chain.size() && beforeAt < chain.size() && beforeAt != afterAt + 1) {
        problems.push_back("表里说它插在 " + after + " 与 " + before +
                           " 之间，这两步在目标链上不挨着——中间插进了别的步骤，或者次序写反了，回去确认它该插在哪");
    }

    // 同一个脚本若也置了窗口两头那一步，先后要与表对得上。
    if (own == std::string::npos) return problems;
    const std::size_t afterSet = liveFlagSet(scriptText, after);
    if (afterSet != std::string::npos && afterSet > own) {
        problems.push_back(script + " 里 " + flag + " 先于 " + after + " 置，表里却把它插在 " + after + " 之后");
    }
    const std::size_t beforeSet = liveFlagSet(scriptText, before);
    if (beforeSet != std::string::npos && beforeSet < own) {
        problems.push_back(script + " 里 " + flag + " 晚于 " + before + " 置，表里却把它插在 " + before + " 之前");
    }
    return problems;
}

// ===========================================================================
// 交接存档检查点（独立审查 M1）
// ===========================================================================
//
// 钉的是什么
// ----------
// tests/fixtures/chNN-end-*.sav 是章与章之间的交接存档（tests/ChapterFixture.h）：第 NN 章通关
// 测试的终局，也是第 NN+1 章通关测试的起点。它们一度是从第 3 章通关测试「手搭的第 2 章章末」
// 一路传下来的，那份手搭状态里第 1 章的旗标只有 ch01.done——于是按 ch01.sanshu_met /
// climb_done / shoutu_done 撤场的人（三叔、韩母、山道上的张铁、两位考官）在这些存档里又站了
// 出来，那几个一次性触发器也重新备好，一踩还会重演。上面那条沿目标链的用例抓不到它：它的
// 时间线按链自己置旗标，从不读这些存档。
//
// 所以把每一份交接存档当成主线上的一个检查点来考（文件名 chNN-end-* 说它是第 NN 章的终局）：
//   1. 它真的走过了第 1..NN 章：这几章目标链上的每一步，doneFlag 都非 0——「可跳过」的除外，
//      口径见 skippableStep，不写例外名单。
//   2. 它的在场名单就是主线走到同一章末时的在场名单：
//      · 没有一人两地（presenceConflicts，群像白名单照旧）；
//      · 逐个 NPC 比（castDifferences）：主线走到第 NN 章末时已经退场的——出场区间在那之前
//        就结束了，hidden_flag 在主线上已经置过——存档里不许还站着；反过来，主线上此刻在场的，
//        存档里也得在。不点任何对象的名：同一个 WorldScene::npcVisible 在两份状态下各算一遍，
//        对不上的报出来，连同是哪个在场旗标对不上。「主线走到第 NN 章末」就是上面那条时间线
//        走完第 NN 章最后一步时的状态：第 1..NN 章的链上旗标，加上表里插在这几章里的旗标
//       （mainLineAtChapterEnd）。
//      第 2 条比「退场的不许在」多问了一个方向，因为两个方向是同一个判据：存档里的世界要与
//      真玩到这里的世界一样。只问一半，一份把后面章节的旗标提前置上的存档照样过得去。

// 一段内容的原文，连同它相对仓库根的路径（报错用）。
struct ContentText {
    std::string path;
    std::string text;
};

// 判「谁置、谁读某个旗标」要看的全部内容。
struct GameContent {
    std::vector<ContentText> scripts;   // scripts/**/*.lua
    std::vector<ContentText> tables;    // maps/*.tmj 与 data/**/*.json（flags.json、objectives/、text/ 除外）
};

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

// 一行 Lua 里注释之前的那一截。粗略口径：从第一个 -- 起都算注释（本仓脚本的字符串里没有 --，
// 与 liveFlagSet 同一个口径）。
std::string luaCodeOf(const std::string& line) {
    const std::size_t dash = line.find("--");
    return dash == std::string::npos ? line : line.substr(0, dash);
}

// 脚本的活代码里有没有「读」它：出现了 "<flag>"，而且不是 flag.set("<flag>" 的那个参数。
// flag.get、以及把旗标名当参数递给别的助手函数，都算读。
bool scriptReads(const std::string& text, const std::string& flag) {
    const std::string quoted = "\"" + flag + "\"";
    const std::string setCall = "flag.set(";
    for (const std::string& line : splitLines(text)) {
        const std::string code = luaCodeOf(line);
        for (std::size_t at = code.find(quoted); at != std::string::npos; at = code.find(quoted, at + 1)) {
            const bool setArgument =
                at >= setCall.size() && code.compare(at - setCall.size(), setCall.size(), setCall) == 0;
            if (!setArgument) return true;
        }
    }
    return false;
}

// 脚本里有没有一句顶格的 flag.set("<flag>"：这一行从第 0 列起就是这一句，前面没有缩进、
// 也没有同一行的条件（if x then flag.set(...) end 不算）。
bool scriptSetsAtTopLevel(const std::string& text, const std::string& flag) {
    const std::string statement = "flag.set(\"" + flag + "\"";
    for (const std::string& line : splitLines(text)) {
        if (line.rfind(statement, 0) == 0) return true;
    }
    return false;
}

// 地图或数据表里有没有 "<flag>" 这个**整串**：JSON 的字符串值带着引号。写在说明文字里的
// 旗标名前后另有字，不会整串相等，所以 data/ 里那些 note 提到它不算读。
bool tableMentions(const std::string& text, const std::string& flag) {
    return text.find("\"" + flag + "\"") != std::string::npos;
}

// 目标链上的这一步，在一份章末存档里可以没置吗（「可跳过」）。两条同时成立才算：
//   (a) 有的路走到章末也不置它：脚本里置它的每一处都不是一句顶格的独立语句（都缩在
//       if / 分支里，或者跟在同一行的条件后面）。ch04.liandan 就是这样——qiecuo.lua 里
//       身上揣着养精丹才置，没炼药的玩家照样演完这一章（data/flags.json 的说明）。
//   (b) 跳过了世界也不变：除了它自己那几处 flag.set，没有谁读它——没有地图对象的属性
//      （门的 require_flag、触发器的 guard_flag / set_flag、NPC 的 visible / hidden_flag、
//       遭遇区……），没有 data/ 下的表（任务、路径行动、章节表……），也没有脚本的活代码。
//       这样的旗标缺了，只有目标行看得出来，而目标行本来就跳得过洞：rules::currentObjective
//       取的是「走得最远的那一条已完成的」之后那一步。
// 只看 (a) 不够：分支里各置一次的选择旗标（ch03.yingdui_xuan、ch05.xiaoxiang……）也都缩在
// 分支里，可它们是触发器的 set_flag，缺了那一幕会重演——它们必须在。只看 (b) 也不够：
// ch01.koujue_received 今天没人读，可它在 shenshougu_koujue.lua 里是一句顶格语句，演完
// 第 1 章就一定有；缺了它，说明这份存档根本没演过那一幕。
// 从来没人置的旗标不算可跳过（那是另一种毛病，缺了照样报）。
// 不从 data/flags.json 的说明文字里认「可跳过」：那是写给人看的散文，没有可依赖的格式。
// 引擎代码（src/）不按剧情旗标办事，不在扫描之列——唯一读旗标名的一处是 io/SaveFile.cpp
// 给 v5 以前的老档补境界上限，只对老档起作用。
bool skippableStep(const std::string& flag, const GameContent& content) {
    bool setSomewhere = false;
    for (const ContentText& script : content.scripts) {
        if (scriptSetsAtTopLevel(script.text, flag)) return false;   // (a) 不成立
        if (scriptReads(script.text, flag)) return false;            // (b) 不成立：有脚本读它
        setSomewhere = setSomewhere || liveFlagSet(script.text, flag) != std::string::npos;
    }
    for (const ContentText& table : content.tables) {
        if (tableMentions(table.text, flag)) return false;           // (b) 不成立：地图或数据表读它
    }
    return setSomewhere;
}

// 第 1..chapter 章目标链上，这份存档没置、又不可跳过的那几步，按链上的次序。
std::vector<Objective> missingSteps(const std::vector<Objective>& chain, int chapter, const GameState& save,
                                    const std::set<std::string>& skippable) {
    std::vector<Objective> out;
    for (const Objective& step : chain) {
        if (step.chapter > chapter) continue;
        if (save.flag(step.doneFlag) != 0) continue;
        if (skippable.count(step.doneFlag) != 0) continue;
        out.push_back(step);
    }
    return out;
}

// 主线走完第 chapter 章最后一步时的存档：与 mainLine 同一个口径（链上的步依次置，
// 表里的旗标插在它声称的那一步之后），只是不留中间的检查点。
GameState mainLineAtChapterEnd(const std::vector<Objective>& chain, const std::vector<OffChainFlag>& offChain,
                               int chapter) {
    GameState state;
    for (const Objective& step : chain) {
        if (step.chapter > chapter) continue;
        state.setFlag(step.doneFlag, 1);
        for (const OffChainFlag& extra : offChain) {
            if (step.doneFlag == extra.after) state.setFlag(extra.flag, 1);
        }
    }
    return state;
}

// 一个 NPC 在两份状态下在不在场对不上。
struct CastDifference {
    std::string map;
    const MapObject* npc = nullptr;   // 指进传进来的 maps，maps 活着就有效
    bool presentOnMainLine = false;   // 主线上在不在场；存档里正好相反
};

std::vector<CastDifference> castDifferences(const std::vector<TileMap>& maps, const GameState& mainLine,
                                            const GameState& save) {
    std::vector<CastDifference> out;
    for (const TileMap& map : maps) {
        for (const MapObject& object : map.objects) {
            if (object.type != "npc") continue;
            const bool expected = WorldScene::npcVisible(mainLine, object);
            if (expected == WorldScene::npcVisible(save, object)) continue;
            out.push_back(CastDifference{map.id, &object, expected});
        }
    }
    return out;
}

std::string describeCastDifference(const CastDifference& diff, const GameState& mainLine, const GameState& save,
                                   int chapter) {
    std::ostringstream out;
    out << diff.map << "/" << diff.npc->name << "（" << diff.npc->property("role_id") << "）：主线走到第 "
        << chapter << " 章末时"
        << (diff.presentOnMainLine ? "在场，这份存档里却不在" : "已经退场，这份存档里却还站着") << "。在场旗标：";
    for (const char* key : {"visible_flag", "hidden_flag"}) {
        const std::string flag = diff.npc->property(key);
        if (flag.empty()) continue;
        out << key << " " << flag << "（主线 " << mainLine.flag(flag) << "，存档 " << save.flag(flag) << "） ";
    }
    return out.str();
}

// 主线走到这一步时已经退场的人数：空存档里在场、此刻不在场。
std::size_t departedCount(const std::vector<TileMap>& maps, const GameState& mainLine) {
    std::size_t count = 0;
    const GameState fresh{};
    for (const TileMap& map : maps) {
        for (const MapObject& object : map.objects) {
            if (object.type != "npc") continue;
            if (WorldScene::npcVisible(fresh, object) && !WorldScene::npcVisible(mainLine, object)) ++count;
        }
    }
    return count;
}

// 交接存档的文件名 chNN-end-<哪一侧>.sav → NN；不是这个形状返回 0。
int handoverChapter(const std::string& fileName) {
    const std::string middle = "-end-";
    const std::string suffix = ".sav";
    const std::size_t shortest = 4 + middle.size() + 1 + suffix.size();   // chNN + -end- + 至少一个字 + .sav
    if (fileName.size() < shortest || fileName.compare(0, 2, "ch") != 0) return 0;
    const char tens = fileName[2];
    const char ones = fileName[3];
    if (tens < '0' || tens > '9' || ones < '0' || ones > '9') return 0;
    if (fileName.compare(4, middle.size(), middle) != 0) return 0;
    if (fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) != 0) return 0;
    return (tens - '0') * 10 + (ones - '0');
}

// root 下的 dir 里（递归）扩展名为 extension 的文件原文，路径相对 root、按路径排好。
// skip(相对路径) 为真的不收。
template <typename Skip>
void collectTexts(const fs::path& root, const fs::path& dir, const std::string& extension, Skip skip,
                  std::vector<ContentText>& out) {
    std::vector<fs::path> files;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    for (const fs::path& file : files) {
        const std::string relative = file.lexically_relative(root).generic_string();
        if (skip(relative)) continue;
        out.push_back(ContentText{relative, readText(file)});
    }
}

GameContent loadGameContent(const fs::path& root) {
    GameContent content;
    const auto keepAll = [](const std::string&) { return false; };
    collectTexts(root, root / "scripts", ".lua", keepAll, content.scripts);
    collectTexts(root, root / "maps", ".tmj", keepAll, content.tables);
    // flags.json 是旗标的登记表，objectives/ 是目标链自己，text/ 是文案：都不是「读」旗标的地方。
    collectTexts(
        root, root / "data", ".json",
        [](const std::string& path) {
            return path == "data/flags.json" || path.rfind("data/objectives/", 0) == 0 ||
                   path.rfind("data/text/", 0) == 0;
        },
        content.tables);
    return content;
}

// ---------------------------------------------------------------------------
// 自检：判据有牙（手搭的内存数据，不读盘）
// ---------------------------------------------------------------------------

TileMap sketchMap(const std::string& id, std::vector<MapObject> npcs) {
    TileMap map;
    map.id = id;
    map.width = 4;
    map.height = 1;
    map.collision.assign(4, 0);
    map.objects = std::move(npcs);
    return map;
}

MapObject sketchNpc(const std::string& name, const std::string& role, int x,
                    const std::string& visibleFlag = {}, const std::string& hiddenFlag = {}) {
    MapObject npc;
    npc.name = name;
    npc.type = "npc";
    npc.position = Point{x, 0};
    npc.properties["role_id"] = role;
    if (!visibleFlag.empty()) npc.properties["visible_flag"] = visibleFlag;
    if (!hiddenFlag.empty()) npc.properties["hidden_flag"] = hiddenFlag;
    return npc;
}

TEST(NpcPresence, TheVerdictCatchesOneRoleStandingOnTwoMapsWithoutFlags) {
    const std::vector<TileMap> maps = {
        sketchMap("t_east", {sketchNpc("npc_wang", "t_wang", 1)}),
        sketchMap("t_west", {sketchNpc("npc_wang", "t_wang", 2)}),
    };
    const std::vector<PresenceConflict> conflicts = presenceConflicts(maps, GameState{}, {});
    ASSERT_EQ(conflicts.size(), 1u) << "两张图各站一个无旗标的同一个人，判据却没报";
    EXPECT_EQ(conflicts.front().role, "t_wang");
    EXPECT_EQ(conflicts.front().maps, (std::vector<std::string>{"t_east", "t_west"}));
    EXPECT_EQ(conflicts.front().placements.size(), 2u);
}

TEST(NpcPresence, TheVerdictLetsAComplementaryPairThrough) {
    // 同一个人从东图挪到西图：东边那个挂 hidden_flag，西边那个挂同一个旗标的 visible_flag。
    const std::vector<TileMap> maps = {
        sketchMap("t_east", {sketchNpc("npc_wang", "t_wang", 1, "", "t.moved")}),
        sketchMap("t_west", {sketchNpc("npc_wang", "t_wang", 2, "t.moved", "")}),
    };
    GameState before;
    GameState after;
    after.setFlag("t.moved", 1);
    for (const GameState* state : {&before, &after}) {
        EXPECT_TRUE(presenceConflicts(maps, *state, {}).empty()) << "互补旗标的一对被报成了一人两处";
        // 先验：不报，不是因为两个都不在场——此刻恰好有一个在。
        const auto where = rolePlacements(maps, *state);
        ASSERT_EQ(where.count("t_wang"), 1u) << "两个都不在场，这条用例什么也没验";
        EXPECT_EQ(where.at("t_wang").size(), 1u);
    }
    EXPECT_NE(rolePlacements(maps, before).at("t_wang").front().map,
              rolePlacements(maps, after).at("t_wang").front().map)
        << "旗标置上前后他站在同一张图上：互补旗标没有真的把人挪走";
}

TEST(NpcPresence, TheVerdictLeavesTheCrowdAloneButNotTheNamed) {
    const std::vector<TileMap> maps = {
        sketchMap("t_east", {sketchNpc("npc_guard", "t_guard", 1), sketchNpc("npc_wang", "t_wang", 2)}),
        sketchMap("t_west", {sketchNpc("npc_guard", "t_guard", 1), sketchNpc("npc_wang", "t_wang", 2)}),
    };
    const std::vector<PresenceConflict> conflicts = presenceConflicts(maps, GameState{}, {"t_guard"});
    ASSERT_EQ(conflicts.size(), 1u);
    EXPECT_EQ(conflicts.front().role, "t_wang") << "白名单只放行名单上的那一类人";
    // 对照：不给白名单，两个都报——上面放行 t_guard 的确是白名单的功劳。
    EXPECT_EQ(presenceConflicts(maps, GameState{}, {}).size(), 2u);
}

Objective sketchStep(const std::string& id, const std::string& doneFlag, int chapter = 1) {
    Objective step;
    step.id = id;
    step.doneFlag = doneFlag;
    step.chapter = chapter;
    return step;
}

// 恰好一条问题，而且说的正是 keyword 那件事——只数条数的话，报错的理由错了也照样过。
::testing::AssertionResult exactlyOneAbout(const std::vector<std::string>& problems, const std::string& keyword) {
    if (problems.size() == 1 && problems.front().find(keyword) != std::string::npos) {
        return ::testing::AssertionSuccess();
    }
    ::testing::AssertionResult failure = ::testing::AssertionFailure();
    failure << "要的是恰好一条讲「" << keyword << "」的问题，实际 " << problems.size() << " 条：";
    for (const std::string& problem : problems) failure << "\n  " << problem;
    return failure;
}

TEST(NpcPresence, TheOffChainTableCheckCatchesAWrongScriptAWrongWindowAndAWrongOrder) {
    const std::vector<Objective> chain = {sketchStep("n1", "t.a"), sketchStep("n2", "t.b"),
                                          sketchStep("n3", "t.c")};
    const OffChainFlag between{"t.x", "t.a", "t.b", "scripts/t/x.lua"};
    // 对照：脚本里先置 t.a、再置它、再置 t.b——与表一致，一条问题都没有。
    EXPECT_EQ(offChainProblems(between, chain, "flag.set(\"t.a\")\nflag.set(\"t.x\")\nflag.set(\"t.b\")\n"),
              std::vector<std::string>{})
        << "与表一致的脚本不该报任何问题";
    // 只在注释里出现、或只有同名前缀的旗标（t.xy），都不算置过。
    EXPECT_TRUE(exactlyOneAbout(
        offChainProblems(between, chain, "-- flag.set(\"t.x\")\nflag.set(\"t.xy\")\n"), "里没有 flag.set"));
    // 脚本里它先于 t.a 置，表却插在 t.a 之后——墨大夫之死差一点就这么写了。
    EXPECT_TRUE(exactlyOneAbout(offChainProblems(between, chain, "flag.set(\"t.x\")\nflag.set(\"t.a\")\n"), "先于"));
    // 脚本里它晚于 t.b 置，表却插在 t.b 之前。
    EXPECT_TRUE(exactlyOneAbout(offChainProblems(between, chain, "flag.set(\"t.b\")\nflag.set(\"t.x\")\n"), "晚于"));
    // 窗口不挨着（t.a 与 t.c 之间还隔着 t.b）。
    const OffChainFlag wide{"t.x", "t.a", "t.c", "scripts/t/x.lua"};
    EXPECT_TRUE(exactlyOneAbout(offChainProblems(wide, chain, "flag.set(\"t.x\")\n"), "不挨着"));
    // 窗口一头根本不在目标链上。
    const OffChainFlag stray{"t.x", "t.a", "t.nowhere", "scripts/t/x.lua"};
    EXPECT_TRUE(exactlyOneAbout(offChainProblems(stray, chain, "flag.set(\"t.x\")\n"), "不是目标链上任何一步"));
}

TEST(NpcPresence, SeveralOfOneRoleOnTheSameMapAreNotAConflict) {
    // 拿一个模板在同一张图上摆几个路人是常见写法：只数图，不数人。
    const std::vector<TileMap> maps = {
        sketchMap("t_town", {sketchNpc("npc_a", "t_passer", 1), sketchNpc("npc_b", "t_passer", 3)}),
    };
    EXPECT_TRUE(presenceConflicts(maps, GameState{}, {}).empty());
    ASSERT_EQ(rolePlacements(maps, GameState{}).at("t_passer").size(), 2u) << "先验：两个都在场";
}

// ---- 交接存档检查点的自检 ----

TEST(NpcPresence, HandoverSaveNamesSayWhichChapterTheyEnd) {
    EXPECT_EQ(handoverChapter("ch05-end-first.sav"), 5);
    EXPECT_EQ(handoverChapter("ch03-end-second.sav"), 3);
    EXPECT_EQ(handoverChapter("ch12-end-x.sav"), 12);
    EXPECT_EQ(handoverChapter("ch04-start.sav"), 0) << "章首存档不是哪一章的终局";
    EXPECT_EQ(handoverChapter("ch4-end-first.sav"), 0);
    EXPECT_EQ(handoverChapter("ch05-end-.sav"), 0) << "少了哪一侧";
    EXPECT_EQ(handoverChapter("ch05-end-first.json"), 0);
}

TEST(NpcPresence, OnlyAStepSetOnABranchThatNothingReadsMayBeSkipped) {
    GameContent content;
    content.scripts = {
        {"scripts/t/met.lua", "flag.set(\"t1.met\")\n"},
        {"scripts/t/brew.lua",
         "-- 这里不看 \"t1.brew\"，看身上几瓶\nif item.count(\"pill\") > 0 then\n    flag.set(\"t1.brew\")\nend\n"},
        {"scripts/t/inline.lua", "if lucky then flag.set(\"t1.inline\") end\n"},
        {"scripts/t/pick.lua", "if pick == 1 then\n    flag.set(\"t1.pick\")\nelse\n    flag.set(\"t1.pick\")\nend\n"},
        {"scripts/t/gated.lua", "if go then\n    flag.set(\"t1.gated\")\nend\n"},
        {"scripts/t/gate.lua", "if flag.get(\"t1.gated\") ~= 1 then\n    return\nend\n"},
        {"scripts/t/done.lua", "flag.set(\"t1.done\")\n"},
    };
    content.tables = {
        {"maps/t_east.tmj", "{\"name\": \"hidden_flag\", \"value\": \"t1.met\"}, "
                            "{\"name\": \"set_flag\", \"value\": \"t1.pick\"}"},
        // 说明文字里提到它：前后另有字，不是整串，不算读。
        {"data/t/note.json", "{\"note\": \"t1.brew 没炼就不置\"}"},
    };
    EXPECT_TRUE(skippableStep("t1.brew", content)) << "只在 if 里置、又没人读（注释与说明文字不算读）：可跳过";
    EXPECT_TRUE(skippableStep("t1.inline", content)) << "条件写在同一行，也不是顶格语句";
    EXPECT_FALSE(skippableStep("t1.pick", content)) << "缩在分支里，可地图读它（触发器的 set_flag）：缺了那一幕会重演";
    EXPECT_FALSE(skippableStep("t1.gated", content)) << "缩在分支里，可另一个脚本拿它当闸";
    EXPECT_FALSE(skippableStep("t1.met", content)) << "顶格置，又有 NPC 拿它退场";
    EXPECT_FALSE(skippableStep("t1.done", content)) << "没人读，可它是顶格语句：演完那一幕就一定有";
    EXPECT_FALSE(skippableStep("t1.nobody", content)) << "从来没人置：不算可跳过，缺了照样报";
}

TEST(NpcPresence, TheHandoverVerdictsCatchAnEndingThatSkippedTheMiddleOfAChapter) {
    // 两章的链：t1.brew 是可跳过的那种（与上一条同一份内容的缩写）。
    const std::vector<Objective> chain = {sketchStep("n1_met", "t1.met"), sketchStep("n2_brew", "t1.brew"),
                                          sketchStep("n3_done", "t1.done"), sketchStep("m1_done", "t2.done", 2)};
    GameContent content;
    content.scripts = {{"scripts/t/all.lua",
                        "flag.set(\"t1.met\")\nif pill then\n    flag.set(\"t1.brew\")\nend\n"
                        "flag.set(\"t1.done\")\nflag.set(\"t2.done\")\n"}};
    content.tables = {{"maps/t_east.tmj", "\"t1.met\" \"t2.done\""}};
    std::set<std::string> skippable;
    for (const Objective& step : chain) {
        if (skippableStep(step.doneFlag, content)) skippable.insert(step.doneFlag);
    }
    ASSERT_EQ(skippable, (std::set<std::string>{"t1.brew"})) << "先验：只有 t1.brew 可跳过";

    // M1 那种手搭的第 2 章章末：第 1 章只留了收尾那一步。
    GameState handBuilt;
    handBuilt.setFlag("t1.done", 1);
    handBuilt.setFlag("t2.done", 1);
    const std::vector<Objective> missing = missingSteps(chain, 2, handBuilt, skippable);
    ASSERT_EQ(missing.size(), 1u) << "缺的是 t1.met；t1.brew 可跳过，不该报";
    EXPECT_EQ(missing.front().doneFlag, "t1.met");
    // 只问第 1..NN 章：拿它当第 1 章的终局，t2.done 有没有不相干。
    GameState chapterOne;
    chapterOne.setFlag("t1.met", 1);
    chapterOne.setFlag("t1.done", 1);
    EXPECT_TRUE(missingSteps(chain, 1, chapterOne, skippable).empty());

    // 在场名单：三叔在 t1.met 之后退场，客人第 2 章末才来。
    const std::vector<TileMap> maps = {
        sketchMap("t_east", {sketchNpc("npc_uncle", "t_uncle", 1, "", "t1.met"),
                             sketchNpc("npc_guest", "t_guest", 2, "t2.done", "")}),
    };
    const GameState mainLine = mainLineAtChapterEnd(chain, {}, 2);
    ASSERT_NE(mainLine.flag("t1.met"), 0) << "先验：主线走到第 2 章末，t1.met 早置了";
    ASSERT_EQ(departedCount(maps, mainLine), 1u) << "先验：主线上三叔已经退场";
    const std::vector<CastDifference> ghosts = castDifferences(maps, mainLine, handBuilt);
    ASSERT_EQ(ghosts.size(), 1u) << "手搭的存档里三叔又站了出来，判据却没报";
    EXPECT_EQ(ghosts.front().npc->name, "npc_uncle");
    EXPECT_FALSE(ghosts.front().presentOnMainLine);
    // 反方向：存档少了 t2.done，该到的客人不在。
    GameState noGuest = mainLine;
    noGuest.flags.erase("t2.done");
    const std::vector<CastDifference> absent = castDifferences(maps, mainLine, noGuest);
    ASSERT_EQ(absent.size(), 1u);
    EXPECT_EQ(absent.front().npc->name, "npc_guest");
    EXPECT_TRUE(absent.front().presentOnMainLine);
    // 对照：补齐之后一处都不差。
    handBuilt.setFlag("t1.met", 1);
    EXPECT_TRUE(castDifferences(maps, mainLine, handBuilt).empty());
    EXPECT_TRUE(missingSteps(chain, 2, handBuilt, skippable).empty());

    // 表里的旗标插在它声称的那一步之后：走完第 1 章就有。
    const std::vector<OffChainFlag> extra = {{"t1.extra", "t1.met", "t1.brew", "scripts/t/all.lua"}};
    EXPECT_EQ(mainLineAtChapterEnd(chain, extra, 1).flag("t1.extra"), 1);
}

// ---------------------------------------------------------------------------
// 真的地图、真的目标链
// ---------------------------------------------------------------------------

class NpcPresenceShipped : public ::testing::Test {
protected:
    void SetUp() override {
        const fs::path root(repoRoot());
        std::vector<fs::path> files;
        for (const fs::directory_entry& entry : fs::directory_iterator(root / "maps")) {
            if (entry.path().extension() == ".tmj") files.push_back(entry.path());
        }
        std::sort(files.begin(), files.end());
        for (const fs::path& file : files) {
            auto loaded = fanren::io::loadTileMap(file.string());
            ASSERT_TRUE(loaded.ok) << loaded.error;
            maps_.push_back(std::move(loaded.value));
        }
        ASSERT_FALSE(maps_.empty()) << (root / "maps").string() << " 下一张地图都没有";

        auto data = fanren::io::loadGameData((root / "data").string());
        ASSERT_TRUE(data.ok) << data.error;
        chain_ = std::move(data.value.objectives);
        ASSERT_FALSE(chain_.empty()) << "目标链一步都没读到，时间线只剩空存档一个检查点";
    }

    struct HandoverSave {
        std::string file;   // 文件名
        int chapter = 0;    // 第几章的终局
        GameState state;
    };

    // tests/fixtures/*.sav，按文件名排好，经真的 io::loadGame 读（校验和、版本迁移都照走）。
    // 名字认不出、读不回来的当场报，不悄悄跳过。
    static std::vector<HandoverSave> handoverSaves() {
        const fs::path dir = fs::path(repoRoot()) / "tests" / "fixtures";
        std::vector<fs::path> files;
        if (fs::is_directory(dir)) {
            for (const fs::directory_entry& entry : fs::directory_iterator(dir)) {
                if (entry.path().extension() == ".sav") files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
        std::vector<HandoverSave> out;
        for (const fs::path& file : files) {
            const std::string name = file.filename().string();
            const int chapter = handoverChapter(name);
            if (chapter == 0) {
                ADD_FAILURE() << "tests/fixtures/" << name
                              << " 不是 chNN-end-<哪一侧>.sav 的形状，认不出它是第几章的终局。交接存档照这个形状命名，"
                                 "别的存档别放进这个目录（或者到这里写明它该怎么考）";
                continue;
            }
            auto loaded = fanren::io::loadGame(file.string());
            if (!loaded.ok) {
                ADD_FAILURE() << "tests/fixtures/" << name << " 读不回来：" << loaded.error;
                continue;
            }
            out.push_back(HandoverSave{name, chapter, std::move(loaded.value)});
        }
        return out;
    }

    std::vector<TileMap> maps_;
    std::vector<Objective> chain_;
};

// 第 3、4、5、6 章的终局各两侧（tests/ChapterFixture.h）。少于这个数，说明目录找错了或存档丢了。
constexpr std::size_t kMinHandoverSaves = 8;

TEST_F(NpcPresenceShipped, EveryOffChainFlagIsSetWhereTheTableSays) {
    for (const OffChainFlag& extra : kOffChainFlags) {
        const fs::path script = fs::path(repoRoot()) / extra.script;
        if (!fs::exists(script)) {
            ADD_FAILURE() << "表里说 " << extra.flag << " 在 " << extra.script << " 里置，这个脚本不在";
            continue;
        }
        for (const std::string& problem : offChainProblems(extra, chain_, readText(script))) {
            ADD_FAILURE() << "表里的 " << extra.flag << "：" << problem;
        }
    }
}

TEST_F(NpcPresenceShipped, EveryNamedRoleStandsOnAtMostOneMapAtEveryStepOfTheMainLine) {
    // 先验一：目标链覆盖第 1–5 章。
    std::set<int> chapters;
    for (const Objective& step : chain_) chapters.insert(step.chapter);
    for (int chapter = kFirstChapter; chapter <= kLastChapter; ++chapter) {
        EXPECT_EQ(chapters.count(chapter), 1u) << "目标链里没有第 " << chapter << " 章，那一章的 NPC 没被考到";
    }

    const std::vector<OffChainFlag> offChain(std::begin(kOffChainFlags), std::end(kOffChainFlags));
    const std::vector<Checkpoint> line = mainLine(chain_, offChain);

    // 先验二：表里每一条都真的插进了时间线（after 找不到时它一次都没置过）。
    std::size_t inserted = 0;
    for (const OffChainFlag& extra : offChain) {
        const auto hits = std::count_if(chain_.begin(), chain_.end(),
                                        [&](const Objective& step) { return step.doneFlag == extra.after; });
        EXPECT_EQ(hits, 1) << extra.flag << " 该插在 " << extra.after << " 之后，而目标链上这一步有 " << hits
                           << " 处";
        inserted += static_cast<std::size_t>(hits);
    }
    ASSERT_EQ(line.size(), 1 + chain_.size() + inserted) << "检查点数对不上：空存档 + 每一步 + 表里插进来的";

    // 先验三（覆盖）：地图上 NPC 用到的在场旗标，这条时间线都置过。旗标只置不清，
    // 终点的存档就是「一路上置过的全部」。
    const GameState& end = line.back().state;
    std::size_t npcs = 0;
    for (const TileMap& map : maps_) {
        npcs += static_cast<std::size_t>(std::count_if(map.objects.begin(), map.objects.end(),
                                                       [](const MapObject& o) { return o.type == "npc"; }));
    }
    ASSERT_GT(npcs, 0u) << "全部地图上一个 npc 都没有，这条用例什么也没验";
    for (const auto& [flag, users] : npcFlagUsers(maps_)) {
        if (end.flag(flag) != 0) continue;
        std::string who;
        for (const std::string& user : users) who += "\n    " + user;
        ADD_FAILURE() << "旗标 " << flag
                      << " 挂在下面这些 NPC 身上，却不在这条时间线上（不是目标链哪一步的 done_flag，"
                         "也不在 kOffChainFlags 里）——它们在整条时间线上只露了一种面目，没被考到。"
                         "查清是哪个脚本在哪一步置它，补进 kOffChainFlags："
                      << who;
    }

    // 判据：每个检查点上，白名单之外的 role 最多在一张图。
    for (const ConflictRun& run : conflictRuns(maps_, line, crowdRoles())) {
        ADD_FAILURE() << describeRun(run);
    }
}

// ---- 交接存档检查点（口径见「交接存档检查点」那一节的注释）----

TEST_F(NpcPresenceShipped, EveryHandoverSaveHasWalkedEveryStepOfItsChapters) {
    const std::vector<HandoverSave> saves = handoverSaves();
    ASSERT_GE(saves.size(), kMinHandoverSaves) << "tests/fixtures 下认得出的交接存档不足 " << kMinHandoverSaves << " 份";
    const GameContent content = loadGameContent(fs::path(repoRoot()));
    ASSERT_FALSE(content.scripts.empty()) << "一个脚本都没读到，「谁置、谁读」无从判起";
    ASSERT_FALSE(content.tables.empty()) << "一张地图、一个数据表都没读到，「谁读」无从判起";

    // 可跳过的步由内容算出来（skippableStep），不写名单。
    std::set<std::string> skippable;
    for (const Objective& step : chain_) {
        if (skippableStep(step.doneFlag, content)) skippable.insert(step.doneFlag);
    }
    std::string skippableList;
    for (const std::string& flag : skippable) skippableList += " " + flag;
    std::cout << "[交接存档] 目标链上可跳过的步：" << (skippable.empty() ? " （无）" : skippableList) << "\n";

    // 先验：每一章的最后一步都不可跳过——否则「这是第 NN 章的终局」这句话本身就没有凭据，
    // 也说明口径算歪了（比如把什么都当成了没人读）。
    std::map<int, Objective> lastOfChapter;
    for (const Objective& step : chain_) lastOfChapter[step.chapter] = step;
    for (const auto& [chapter, step] : lastOfChapter) {
        EXPECT_EQ(skippable.count(step.doneFlag), 0u)
            << "第 " << chapter << " 章最后一步 " << step.id << "（" << step.doneFlag << "）被算成了可跳过";
    }

    for (const HandoverSave& save : saves) {
        if (lastOfChapter.count(save.chapter) == 0) {
            ADD_FAILURE() << save.file << " 说它是第 " << save.chapter << " 章的终局，目标链里却没有第 " << save.chapter
                          << " 章";
            continue;
        }
        const std::vector<Objective> missing = missingSteps(chain_, save.chapter, save.state, skippable);
        if (missing.empty()) continue;
        std::string list;
        for (const Objective& step : missing) list += "\n    " + stepLabel(step) + "（" + step.doneFlag + "）";
        ADD_FAILURE() << save.file << " 是第 " << save.chapter << " 章的终局，第 1–" << save.chapter
                      << " 章目标链上却有 " << missing.size()
                      << " 步没置（可跳过的已经放过）——这份存档没有真的走过这几步，挂在这些旗标上的人与触发器"
                         "在它里面还停在那一步之前："
                      << list;
    }
}

TEST_F(NpcPresenceShipped, EveryHandoverSaveShowsTheCastOfTheMainLineAtThatChapterEnd) {
    const std::vector<HandoverSave> saves = handoverSaves();
    ASSERT_GE(saves.size(), kMinHandoverSaves) << "tests/fixtures 下认得出的交接存档不足 " << kMinHandoverSaves << " 份";
    const std::vector<OffChainFlag> offChain(std::begin(kOffChainFlags), std::end(kOffChainFlags));

    for (const HandoverSave& save : saves) {
        for (const PresenceConflict& conflict : presenceConflicts(maps_, save.state, crowdRoles())) {
            ADD_FAILURE() << save.file << "：" << describeConflict(conflict);
        }

        const GameState mainLine = mainLineAtChapterEnd(chain_, offChain, save.chapter);
        // 先验：主线走到这一章末，确实已经有人退场——否则下面比的只是「谁都没走」。
        EXPECT_GT(departedCount(maps_, mainLine), 0u)
            << save.file << "：主线走到第 " << save.chapter << " 章末一个退场的人都没有，这份存档的在场名单什么也没考";
        for (const CastDifference& diff : castDifferences(maps_, mainLine, save.state)) {
            ADD_FAILURE() << save.file << "：" << describeCastDifference(diff, mainLine, save.state, save.chapter);
        }
    }
}

// 手测检查点 saves/ch04-*.sav 由 tools/mksave 从第 3 章交接存档生成（它首部写明：与交接存档
// 不同的只有位置）。独立审查 M1 当初恰恰出在这两份上，而上面两条只扫 tests/fixtures：
// 以后重生成交接存档却忘了重跑 mksave，检查点就会悄悄漂开，手测看到的不再是测试验过的那一章。
// 所以这里只钉一件事——旗标对得上：章首那份与交接存档一字不差，开战前那份在它之上只多不少。
TEST_F(NpcPresenceShipped, TheManualCheckpointsCarryTheHandoverFlags) {
    const fs::path root(repoRoot());
    auto handover = fanren::io::loadGame((root / "tests" / "fixtures" / "ch03-end-first.sav").string());
    ASSERT_TRUE(handover.ok) << handover.error;
    // 先验：交接存档里确实有旗标，下面的「一字不差」「只多不少」才不是两张空表之间的比较。
    ASSERT_FALSE(handover.value.flags.empty()) << "第 3 章交接存档一个旗标都没有";

    auto start = fanren::io::loadGame((root / "saves" / "ch04-start.sav").string());
    ASSERT_TRUE(start.ok) << start.error;
    EXPECT_EQ(start.value.flags, handover.value.flags)
        << "saves/ch04-start.sav 的旗标与 tests/fixtures/ch03-end-first.sav 对不上：重生成交接存档之后要重跑 "
           "tools/mksave（build-<槽>\\mksave.exe . saves）";

    auto siege = fanren::io::loadGame((root / "saves" / "ch04-siege.sav").string());
    ASSERT_TRUE(siege.ok) << siege.error;
    for (const auto& [flag, value] : handover.value.flags) {
        EXPECT_EQ(siege.value.flag(flag), value)
            << "saves/ch04-siege.sav 丢了或改了交接存档里的 " << flag << "：它该在章首那份之上只多不少，重跑 tools/mksave";
    }

    // 第 6 章章首 saves/ch06-start.sav（2026-09-29）：第 5 章第一侧的交接存档只改了位置，旗标一字不差。
    auto chapterFive = fanren::io::loadGame((root / "tests" / "fixtures" / "ch05-end-first.sav").string());
    ASSERT_TRUE(chapterFive.ok) << chapterFive.error;
    ASSERT_EQ(chapterFive.value.flag("ch05.done"), 1) << "先验：第 5 章交接存档是第 5 章的终局";
    auto chapterSix = fanren::io::loadGame((root / "saves" / "ch06-start.sav").string());
    ASSERT_TRUE(chapterSix.ok) << chapterSix.error;
    EXPECT_EQ(chapterSix.value.flags, chapterFive.value.flags)
        << "saves/ch06-start.sav 的旗标与 tests/fixtures/ch05-end-first.sav 对不上：重生成交接存档之后要重跑 "
           "tools/mksave（build-<槽>\\mksave.exe . saves）";
    EXPECT_EQ(chapterSix.value.bag.size(), chapterFive.value.bag.size()) << "saves/ch06-start.sav 的背包与交接存档对不上";
    EXPECT_EQ(chapterSix.value.day, chapterFive.value.day) << "saves/ch06-start.sav 的日子与交接存档对不上";
}

}  // namespace
