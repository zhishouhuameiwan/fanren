#pragma once

// 第 11 章静态用例共用的读数工具与独立判据表。
// 判据一律抄 docs/ch11-design.md（3.1 挂点表、第 8.2 节、第 11 节、第 15 节验收）与第 18 节施工偏差的落地口径，
// 不从被测数据推导：表里的 id、旗标、数字都是字面量。
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "../vendor/json.hpp"

namespace fanren::test::ch11 {
namespace fs = std::filesystem;
using Json = nlohmann::json;

inline fs::path root() { return fs::absolute(fs::path(__FILE__)).parent_path().parent_path(); }

inline std::string read(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read " + path.string());
    std::ostringstream out;
    out << input.rdbuf();
    return out.str();
}

inline Json json(const fs::path& path) { return Json::parse(read(path)); }

// 施工图第 4 节：新建 7 张图。
inline constexpr std::array<const char*, 7> kMaps{
    "ch11_fangshi", "ch11_dongfu", "ch11_xingkong", "ch11_ningcui_jie",
    "ch11_honghu", "ch11_yinsha_dao", "ch11_xiaoxuan"};
// 施工图 8.2：四张编成，① ② ③ ④。
inline constexpr std::array<const char*, 4> kBattles{
    "b11_ningcuidao_youyao", "b11_honghu_jiguanjiao", "b11_yinsha_shayao", "b11_tiandu_shi"};

struct Hook {
    const char* map;
    const char* object;
    const char* script;   // scripts/ch11/<script>.lua
    const char* guard;
    const char* done;     // set_flag；空 = 不写 set_flag
    bool enter = false;
    bool once = true;
};

// 施工图 3.1 挂点表（地图上的 trigger），按第 18 节落地口径：
//   18.1 第 4 条 trigger_chujian 加 guard ch11.liugen；n14 trigger_huicheng 挪到礁石格、仍 interact。
inline constexpr std::array<Hook, 22> kTriggers{{
    {"ch11_fangshi", "trigger_chudao", "chudao", "ch10.done", "ch11.chudao", true},
    {"ch11_dongfu", "trigger_anju", "anju", "ch11.zufu", "ch11.anju", true},
    {"ch11_dongfu", "trigger_kuilei", "kuilei", "ch11.anju", "ch11.kuilei"},
    {"ch11_dongfu", "trigger_nichang", "nichang", "ch11.ziliao", "ch11.nichang"},
    {"ch11_ningcui_jie", "trigger_ningcui", "ningcui", "ch11.chuansong", "ch11.ningcui", true},
    {"ch11_honghu", "trigger_buzhen", "buzhen", "ch11.ningcui", "ch11.buzhen", true},
    {"ch11_honghu", "trigger_youyao", "youyao", "ch11.buzhen", "", false, false},
    {"ch11_honghu", "trigger_huicheng", "huicheng", "ch11.buzhen", "ch11.huicheng"},
    {"ch11_dongfu", "trigger_liandan", "liandan", "ch11.huicheng", "ch11.sangong"},
    {"ch11_dongfu", "trigger_biguan", "biguan", "ch11.sangong", "ch11.jiedan"},
    {"ch11_dongfu", "trigger_chuguan", "chuguan", "ch11.jiedan", "ch11.chuguan", true},
    {"ch11_dongfu", "trigger_zhaolan", "zhaolan", "ch11.chuguan", "ch11.zhaolan", true},
    {"ch11_dongfu", "trigger_zhuzhan", "zhuzhan", "ch11.chaguan", "ch11.zhuzhan"},
    {"ch11_yinsha_dao", "trigger_guiju", "guiju", "ch11.zhuzhan", "ch11.guiju", true},
    {"ch11_yinsha_dao", "trigger_wuchou", "wuchou", "ch11.guiju", "ch11.jiyin", true},
    {"ch11_yinsha_dao", "trigger_daoge", "daoge", "ch11.jiyin", "ch11.yaoshi", true},
    {"ch11_dongfu", "trigger_peizhu", "peizhu", "ch11.zhanglao", "", false, false},
    {"ch11_xiaoxuan", "trigger_jiesuan", "jiesuan", "ch11.kaidian", "", false, false},
    {"ch11_xiaoxuan", "trigger_guitai", "guitai", "ch11.kaidian", "", false, false},
    {"ch11_dongfu", "trigger_lianjian", "lianjian", "ch11.liugen", "ch11.lianjian_qian", true},
    {"ch11_dongfu", "trigger_chujian", "chujian", "ch11.liugen", "ch11.lianjian", true},
    {"ch11_ningcui_jie", "trigger_huicheng_chuansong", "huicheng_chuansong", "ch11.chuansong", "", true, false},
}};

struct NpcHook {
    const char* map;
    const char* object;
    const char* script;
    const char* visible;
};

// 3.1 表里挂在 NPC 上的节点（NPC 没有 once，脚本开头判已演过）。18.4 第 3 条：Z2 的对象是 _b。
inline constexpr std::array<NpcHook, 12> kNpcHooks{{
    {"ch11_fangshi", "npc_baiyi_xiushi", "zhihuan", "ch11.chudao"},
    {"ch11_fangshi", "npc_lu_er", "luer", "ch11.zhihuan"},
    {"ch11_fangshi", "npc_xinggong_zhishi", "zufu", "ch11.luer"},
    {"ch11_fangshi", "npc_baimen_zhanggui", "ziliao", "ch11.kuilei"},
    {"ch11_xingkong", "npc_xingkong_xiushi", "xingkong", "ch11.nichang"},
    {"ch11_fangshi", "npc_lian_er", "lianer", "ch11.zhaolan"},
    {"ch11_fangshi", "npc_fan_furen", "chaguan", "ch11.lianer"},
    {"ch11_fangshi", "npc_zi_ling", "zhanglao", "ch11.yaoshi"},
    {"ch11_fangshi", "npc_li_dianzhu", "kaidian", "ch11.jinlei"},
    {"ch11_dongfu", "npc_jin_qing", "zhuzhong", "ch11.lianjian"},
    {"ch11_ningcui_jie", "npc_fan_xiaozi", "shougou_dingdan", "ch11.chuansong"},
    {"ch11_dongfu", "npc_jiedan_linju_b", "linju_z2", "ch11.zhaolan"},
}};

inline std::map<std::string, Json> objects(const Json& map) {
    std::map<std::string, Json> result;
    for (const auto& layer : map.at("layers")) {
        if (layer.value("type", "") != "objectgroup") continue;
        for (const auto& object : layer.at("objects")) {
            const auto name = object.at("name").get<std::string>();
            if (!result.emplace(name, object).second) throw std::runtime_error("Duplicate object " + name);
        }
    }
    return result;
}

inline Json properties(const Json& object) {
    Json result = Json::object();
    for (const auto& property : object.value("properties", Json::array()))
        result[property.at("name").get<std::string>()] = property.at("value");
    return result;
}

inline std::string typeOf(const Json& object) { return object.value("type", object.value("class", "")); }

struct Cell {
    int x = 0;
    int y = 0;
    bool operator<(const Cell& other) const { return x != other.x ? x < other.x : y < other.y; }
    bool operator==(const Cell& other) const { return x == other.x && y == other.y; }
};

inline Cell cellOf(const Json& map, const Json& object) {
    return {static_cast<int>(object.at("x").get<double>()) / map.at("tilewidth").get<int>(),
            static_cast<int>(object.at("y").get<double>()) / map.at("tileheight").get<int>()};
}

// 剧本的活代码：每行从第一个 -- 起去掉（与 Ch10LedgerTests 同一口径）。
inline std::string executableLines(const std::string& source) {
    std::istringstream input(source);
    std::string line, out;
    while (std::getline(input, line)) {
        if (const auto comment = line.find("--"); comment != std::string::npos) line.resize(comment);
        out += line + '\n';
    }
    return out;
}

inline std::map<std::string, std::string> scripts(const std::string& chapterDir = "ch11") {
    std::map<std::string, std::string> result;
    for (const auto& file : fs::directory_iterator(root() / "scripts" / chapterDir))
        if (file.path().extension() == ".lua") result[file.path().stem().string()] = read(file.path());
    return result;
}

inline int countMatches(const std::string& text, const std::regex& pattern) {
    return static_cast<int>(std::distance(std::sregex_iterator(text.begin(), text.end(), pattern), std::sregex_iterator{}));
}

// FNV-1a 64：钉「与第 10 章集成版逐字节相同」的那几份文件（值取自 HEAD 68aa3f7 的原文件）。
inline std::uint64_t fnv1a(const std::string& bytes) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (const unsigned char c : bytes) {
        hash ^= c;
        hash *= 0x100000001b3ull;
    }
    return hash;
}
} // namespace fanren::test::ch11
