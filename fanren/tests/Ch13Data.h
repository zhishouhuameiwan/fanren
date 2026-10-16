#pragma once

// 第 13 章静态用例共用的读数工具与独立判据表。
// 判据一律抄 docs/ch13-design.md（3.0 主线节点表、3.1 挂点表与 teleport 清单、第 4 节、5.1、8.2、第 9 节、第 11 节、
// 第 15 节验收、16.1 第 41–60 条）与第 18 节施工偏差的落地口径，不从被测数据推导：表里的 id、旗标、数字都是字面量。
// 与 Ch12Data.h 同形、各自独立（不 include 别章的表，免得改一章牵动另一章）。
#include <algorithm>
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

namespace fanren::test::ch13 {
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

// 施工图第 4 节：新建 4 张图。
inline constexpr std::array<const char*, 4> kMaps{"ch13_nanming", "ch13_tianxing", "ch13_waihai", "ch13_wuzhong"};
// 第 4 节表：每张图的 default spawn。
inline const std::map<std::string, std::string>& defaultSpawns() {
    static const std::map<std::string, std::string> kSpawns{
        {"ch13_nanming", "haishang"}, {"ch13_tianxing", "chengwai"}, {"ch13_waihai", "shiwu"}, {"ch13_wuzhong", "andeng"}};
    return kSpawns;
}
// 施工图 8.2：两张必打编成，① ②。
inline constexpr std::array<const char*, 2> kBattles{"b13_nanming_zhuibing", "b13_waixinghai_lieyao"};

struct Hook {
    const char* map;
    const char* object;
    const char* script;   // scripts/ch13/<script>.lua
    const char* guard;    // 空 = 不写 guard_flag
    const char* done;     // set_flag；空 = 不写 set_flag
    bool enter = false;
    bool once = true;
};

// 施工图 3.1 挂点表（地图上的 trigger），32 个：主线 26 ＋ 链外四处（坊市、告示墙、养魂木、废墟）＋ 往返两处。
// guard 即 3.0 主线节点表的「前置」，done 即 done_flag。
inline constexpr std::array<Hook, 32> kTriggers{{
    {"ch13_nanming", "trigger_wenxun", "wenxun", "ch12.done", "ch13.wenxun", true},
    {"ch13_nanming", "trigger_qiuling", "gongdao", "ch13.caigou", "ch13.gongdao", true},
    {"ch13_nanming", "trigger_chugang", "zhuibing", "ch13.panzha", "ch13.zhuibing", true},
    {"ch13_tianxing", "trigger_rucheng", "rucheng", "ch13.yinjian", "ch13.rucheng", true},
    {"ch13_tianxing", "trigger_dongfu39", "qifu", "ch13.rucheng", "ch13.qifu"},
    {"ch13_tianxing", "trigger_xingkong", "tanzhen", "ch13.qifu", "ch13.tanzhen", true},
    {"ch13_tianxing", "trigger_zhaiyuan", "qianru", "ch13.tanzhen", "ch13.qianru"},
    {"ch13_tianxing", "trigger_lanlu", "yirong", "ch13.qianru", "ch13.yirong", true},
    {"ch13_tianxing", "trigger_fangshi_bimen", "fangshi_bimen", "", "", false, false},
    {"ch13_waihai", "trigger_shiwu", "daoqiyuan", "ch13.chuansong", "ch13.daoqiyuan", true},
    {"ch13_waihai", "trigger_chucheng", "chucheng", "ch13.cunjin", "ch13.chucheng", true},
    {"ch13_waihai", "trigger_mishi", "mishi", "ch13.lidao", "ch13.jiaohuan"},
    {"ch13_waihai", "trigger_jiaoshi", "jianshi", "ch13.wenshi", "ch13.jianshi", true},
    {"ch13_waihai", "trigger_dongkou", "banyao", "ch13.jianshi", "ch13.done"},
    {"ch13_waihai", "trigger_xuanshang", "xuanshang", "", "", false, false},
    {"ch13_waihai", "trigger_feixu", "feixu", "ch13.shouchao", "", true, false},
    {"ch13_waihai", "trigger_hui_wuzhong", "hui_wuzhong", "ch13.lifu", "", true, false},
    {"ch13_wuzhong", "trigger_lifu", "lifu", "ch13.chucheng", "ch13.lifu", true},
    {"ch13_wuzhong", "trigger_chongshi", "chongshi", "ch13.lifu", "ch13.chongshi"},
    {"ch13_wuzhong", "trigger_shichuang", "xuanyin", "ch13.chongshi", "ch13.xuanyin"},
    {"ch13_wuzhong", "trigger_lianjianshi", "chonglian", "ch13.xuanyin", "ch13.chonglian"},
    {"ch13_wuzhong", "trigger_dizhen", "dujie", "ch13.chonglian", "ch13.dujie", true},
    {"ch13_wuzhong", "trigger_biguanshi", "wusezhu", "ch13.dujie", "ch13.wusezhu"},
    {"ch13_wuzhong", "trigger_buzhen", "buzhen", "ch13.wusezhu", "ch13.buzhen", true},
    {"ch13_wuzhong", "trigger_youyao", "youyao", "ch13.buzhen", "", false, false},
    {"ch13_wuzhong", "trigger_shoushou", "chongmo", "ch13.buzhen", "ch13.chongmo"},
    {"ch13_wuzhong", "trigger_danfang", "liandan", "ch13.chongmo", "ch13.liandan"},
    {"ch13_wuzhong", "trigger_jingshimen", "biguan", "ch13.liandan", "ch13.biguan"},
    {"ch13_wuzhong", "trigger_chuguan", "chuguan", "ch13.biguan", "ch13.chuguan", true},
    {"ch13_wuzhong", "trigger_lidao", "lidao", "ch13.gongsun", "ch13.lidao", true},
    {"ch13_wuzhong", "trigger_yanghun", "yanghun", "", "", false, false},
    {"ch13_wuzhong", "trigger_qiyuan_hang", "qiyuan_hang", "ch13.lifu", "", true, false},
}};

struct NpcHook {
    const char* map;
    const char* object;
    const char* script;
    const char* visible;  // 5.1 的出场旗标
    const char* done;     // 脚本开头判的 done_flag（第 11 节「谁置」）
    const char* prereq;   // 3.0 主线节点表的「前置」，由脚本开头判（16.1 第 47 条）
    bool prereqFirst;     // 第一句活代码必须是前置（验收 27：mingzhu.lua、cunjin.lua）
};

// 3.1 表里挂在 NPC 上的八个节点（n02、n04、n06、n12、n14、n15、n29、n32）：无 once，脚本开头判 done_flag 与前置。
inline constexpr std::array<NpcHook, 8> kNpcHooks{{
    {"ch13_nanming", "npc_yuanliao_zhanggui", "caigou", "ch13.wenxun", "ch13.caigou", "ch13.wenxun", false},
    {"ch13_nanming", "npc_cang_yunlong", "panzha", "ch13.gongdao", "ch13.panzha", "ch13.gongdao", false},
    {"ch13_nanming", "npc_ling_yuling", "yinjian", "ch13.zhuibing", "ch13.yinjian", "ch13.zhuibing", false},
    {"ch13_tianxing", "npc_gu_laozhe", "chuansong", "ch13.yirong", "ch13.chuansong", "ch13.yirong", false},
    {"ch13_waihai", "npc_ming_furen", "mingzhu", "ch13.chuansong", "ch13.mingzhu", "ch13.daoqiyuan", true},
    {"ch13_waihai", "npc_li_xun", "cunjin", "ch13.chuansong", "ch13.cunjin", "ch13.mingzhu", true},
    {"ch13_wuzhong", "npc_gongsun_xing", "gongsun", "ch13.chuguan", "ch13.gongsun", "ch13.chuguan", false},
    {"ch13_waihai", "npc_wen_siyue", "wenshi", "ch13.jiaohuan", "ch13.wenshi", "ch13.jiaohuan", false},
}};

struct Presence {
    const char* map;
    const char* object;
    const char* role;
    const char* visible;
    const char* hidden;   // 空 = 不撤
};

// 5.1「在场旗标的权威表」：四张新图上全部 22 个 NPC 对象（旗标省前缀的地方这里补全）。
inline constexpr std::array<Presence, 22> kPresence{{
    {"ch13_nanming", "npc_sanxian_laozhe", "sanxian_laozhe", "ch13.wenxun", "ch13.gongdao"},
    {"ch13_nanming", "npc_yuanliao_zhanggui", "nanming_zhanggui", "ch13.wenxun", ""},
    {"ch13_nanming", "npc_cang_yunlong", "cang_yunlong", "ch13.gongdao", "ch13.panzha"},
    {"ch13_nanming", "npc_liu_hufa", "liu_hufa", "ch13.gongdao", "ch13.panzha"},
    {"ch13_nanming", "npc_xin_ming", "xin_ming", "ch13.gongdao", "ch13.panzha"},
    {"ch13_nanming", "npc_ling_yuling", "ling_yuling", "ch13.zhuibing", "ch13.yinjian"},
    {"ch13_tianxing", "npc_zhao_hufa", "zhao_hufa", "ch13.yinjian", "ch13.chuansong"},
    {"ch13_tianxing", "npc_yu_hufa", "yu_hufa", "ch13.yinjian", "ch13.chuansong"},
    {"ch13_tianxing", "npc_gu_laozhe", "gu_laozhe", "ch13.yirong", "ch13.chuansong"},
    {"ch13_tianxing", "npc_qian_xiushi", "qian_xiushi", "ch13.yirong", "ch13.chuansong"},
    {"ch13_tianxing", "npc_zhang_xiushi", "zhang_xiushi", "ch13.yirong", "ch13.chuansong"},
    {"ch13_tianxing", "npc_yi_jing_tx", "yi_jing", "ch13.yirong", "ch13.chuansong"},
    {"ch13_waihai", "npc_yi_jing", "yi_jing", "ch13.chuansong", "ch13.shouchao"},
    {"ch13_waihai", "npc_xu_yun", "xu_yun", "ch13.chuansong", "ch13.shouchao"},
    {"ch13_waihai", "npc_ming_furen", "ming_furen", "ch13.chuansong", "ch13.shouchao"},
    {"ch13_waihai", "npc_paimaishi", "jingbao_paimaishi", "ch13.mingzhu", "ch13.shouchao"},
    {"ch13_waihai", "npc_li_xun", "li_xun", "ch13.chuansong", "ch13.shouchao"},
    {"ch13_waihai", "npc_gu_yu", "gu_yu", "ch13.chuansong", "ch13.shouchao"},
    {"ch13_waihai", "npc_huang_mingli", "huang_mingli", "ch13.lidao", "ch13.jiaohuan"},
    {"ch13_waihai", "npc_wen_siyue", "wen_siyue", "ch13.jiaohuan", "ch13.wenshi"},
    {"ch13_wuzhong", "npc_sun_shishu", "sun_shishu", "ch13.chuguan", "ch13.gongsun"},
    {"ch13_wuzhong", "npc_gongsun_xing", "gongsun_xing", "ch13.chuguan", "ch13.gongsun"},
}};

// 3.4 目标链 34 步的 done_flag（下标 + 1 就是 n 号）。
inline constexpr std::array<const char*, 34> kChain{
    "ch13.wenxun", "ch13.caigou", "ch13.gongdao", "ch13.panzha", "ch13.zhuibing", "ch13.yinjian", "ch13.rucheng",
    "ch13.qifu", "ch13.tanzhen", "ch13.qianru", "ch13.yirong", "ch13.chuansong", "ch13.daoqiyuan", "ch13.mingzhu",
    "ch13.cunjin", "ch13.chucheng", "ch13.lifu", "ch13.chongshi", "ch13.xuanyin", "ch13.chonglian", "ch13.dujie",
    "ch13.wusezhu", "ch13.buzhen", "ch13.lieyao", "ch13.chongmo", "ch13.liandan", "ch13.biguan", "ch13.chuguan",
    "ch13.gongsun", "ch13.lidao", "ch13.jiaohuan", "ch13.wenshi", "ch13.jianshi", "ch13.done"};

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
    bool operator!=(const Cell& other) const { return !(*this == other); }
};

inline Cell cellOf(const Json& map, const Json& object) {
    return {static_cast<int>(object.at("x").get<double>()) / map.at("tilewidth").get<int>(),
            static_cast<int>(object.at("y").get<double>()) / map.at("tileheight").get<int>()};
}

// 对象盖住的全部格：横贯触发带是 3×1 / 1×3 的矩形（18.2 第 4 条），单格对象是 1×1。
inline std::vector<Cell> cellsOf(const Json& map, const Json& object) {
    const int tw = map.at("tilewidth").get<int>(), th = map.at("tileheight").get<int>();
    const Cell origin = cellOf(map, object);
    const int w = std::max(1, static_cast<int>(object.value("width", static_cast<double>(tw))) / tw);
    const int h = std::max(1, static_cast<int>(object.value("height", static_cast<double>(th))) / th);
    std::vector<Cell> out;
    for (int dy = 0; dy < h; ++dy)
        for (int dx = 0; dx < w; ++dx) out.push_back({origin.x + dx, origin.y + dy});
    return out;
}

// 剧本的活代码：每行从第一个 -- 起去掉（与 Ch12Data.h 同一口径）。
inline std::string executableLines(const std::string& source) {
    std::istringstream input(source);
    std::string line, out;
    while (std::getline(input, line)) {
        if (const auto comment = line.find("--"); comment != std::string::npos) line.resize(comment);
        out += line + '\n';
    }
    return out;
}

// 活代码的非空行（去行首缩进、去行尾空白），按原文次序。
inline std::vector<std::string> liveLines(const std::string& source) {
    std::vector<std::string> out;
    std::istringstream in(executableLines(source));
    for (std::string line; std::getline(in, line);) {
        line.erase(0, line.find_first_not_of(" \t"));
        while (!line.empty() && (line.back() == ' ' || line.back() == '\t' || line.back() == '\r')) line.pop_back();
        if (!line.empty()) out.push_back(line);
    }
    return out;
}

// 负向用例改坏脚本时找改哪一处：第一个落在活代码里的 needle（头注释里常照抄着同一句，改到注释上等于没改）。
inline std::size_t findInCode(const std::string& text, const std::string& needle) {
    for (std::size_t at = text.find(needle); at != std::string::npos; at = text.find(needle, at + 1)) {
        if (at == 0 || text[at] == '\n') return at;   // needle 自己从行首（或换行）起
        const std::size_t newline = text.rfind('\n', at - 1);
        const std::size_t from = newline == std::string::npos ? 0 : newline + 1;
        if (text.substr(from, at - from).find("--") == std::string::npos) return at;
    }
    return std::string::npos;
}

inline std::map<std::string, std::string> scripts(const std::string& chapterDir = "ch13") {
    std::map<std::string, std::string> result;
    for (const auto& file : fs::directory_iterator(root() / "scripts" / chapterDir))
        if (file.path().extension() == ".lua") result[file.path().stem().string()] = read(file.path());
    return result;
}

inline int countMatches(const std::string& text, const std::regex& pattern) {
    return static_cast<int>(std::distance(std::sregex_iterator(text.begin(), text.end(), pattern), std::sregex_iterator{}));
}

inline std::map<std::string, Json> shippedMaps() {
    std::map<std::string, Json> maps;
    for (const char* id : kMaps) maps[id] = json(root() / "maps" / (std::string(id) + ".tmj"));
    return maps;
}

// FNV-1a 64：钉「与第 12 章集成版逐字节相同」的那几份文件（值取自 HEAD 46f6d37 的原文件）。
inline std::uint64_t fnv1a(const std::string& bytes) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (const unsigned char c : bytes) {
        hash ^= c;
        hash *= 0x100000001b3ull;
    }
    return hash;
}

inline std::string joined(const std::vector<std::string>& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}
} // namespace fanren::test::ch13
