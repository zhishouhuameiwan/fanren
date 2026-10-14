#pragma once

// 第 12 章静态用例共用的读数工具与独立判据表。
// 判据一律抄 docs/ch12-design.md（3.1 挂点表与 teleport 清单、第 4 节、8.2、第 11 节、第 15 节验收、16.1 第 36–48 条）
// 与第 18 节施工偏差的落地口径，不从被测数据推导：表里的 id、旗标、数字都是字面量。
// 与 Ch11Data.h 同形、各自独立（不 include 别章的表，免得改一章牵动另一章）。
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

namespace fanren::test::ch12 {
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

// 施工图第 4 节：新建 10 张图（16.1 第 37 条：内殿拆四张）。
inline constexpr std::array<const char*, 10> kMaps{
    "ch12_tushan", "ch12_dixue", "ch12_dadian", "ch12_guiwu", "ch12_xiaojie",
    "ch12_binghuo", "ch12_baoguang", "ch12_taqian", "ch12_hanlitai", "ch12_lingquan"};
// 第 4 节表：每张图的 default spawn。
inline const std::map<std::string, std::string>& defaultSpawns() {
    static const std::map<std::string, std::string> kSpawns{
        {"ch12_tushan", "andeng"}, {"ch12_dixue", "taijie"}, {"ch12_dadian", "tongdao"}, {"ch12_guiwu", "tupo"},
        {"ch12_xiaojie", "caoyuan"}, {"ch12_binghuo", "guangbi"}, {"ch12_baoguang", "yuqiao"},
        {"ch12_taqian", "shishi"}, {"ch12_hanlitai", "taijiao"}, {"ch12_lingquan", "quanbian"}};
    return kSpawns;
}
// 施工图 8.2：四张必打编成，① ② ③ ④。
inline constexpr std::array<const char*, 4> kBattles{
    "b12_guiwu_guiwang", "b12_heishamo_tiehuoyi", "b12_neidian_duoding", "b12_hanlitai_xuangu"};
// 第 9 节来源表 / 验收第 24 条「物品新建 27」：本章新建的物品（23 种 story 之外另有金丝蚕、红光画轴、炼晶、神雷）。
inline constexpr std::array<const char*, 27> kNewItems{
    "story_hualan", "story_zhiyang_huazhou", "talisman_bixie_shenlei", "story_bihuo_baoyi", "story_jiuqu_lingshen",
    "story_xuanyin_huan", "story_baixi_pei", "story_hanbing_zhu", "story_tihun_shou", "story_minghun_zhu",
    "material_lianjing", "story_poluo_zhu", "story_qingming_zhen", "story_huanglin_jia", "story_xuese_pifeng",
    "story_wuse_tonghuan", "story_kuilei_canhai", "story_neidian_juanzhou", "story_jinsi_can", "story_yu_ruyi",
    "story_jiuqu_danfang", "story_xutian_ding", "story_qianlan_zhu", "story_xuangu_leigu", "story_jinlei_xiaojian",
    "story_wannian_lingru", "story_yanghun_mugen"};

struct Hook {
    const char* map;
    const char* object;
    const char* script;   // scripts/ch12/<script>.lua
    const char* guard;    // 空 = 不写 guard_flag
    const char* done;     // set_flag；空 = 不写 set_flag
    bool enter = false;
    bool once = true;
};

// 施工图 3.1 挂点表（地图上的 trigger），34 个。
inline constexpr std::array<Hook, 34> kTriggers{{
    {"ch12_tushan", "trigger_dengdao", "dengdao", "ch11.done", "ch12.dengdao", true},
    {"ch12_tushan", "trigger_huangwu", "pozhen", "ch12.dengdao", "ch12.pozhen"},
    {"ch12_tushan", "trigger_guaizhao", "guairen", "ch12.pozhen", "ch12.tuizhu", true},
    {"ch12_dixue", "trigger_shuichi", "qixialian", "ch12.tuizhu", "ch12.qixialian"},
    {"ch12_dixue", "trigger_shizhu", "duoshe", "ch12.qixialian", "ch12.duoshe", true},
    {"ch12_dadian", "trigger_rudian", "rudian", "ch12.lidao", "ch12.rudian", true},
    {"ch12_dadian", "trigger_qunxiang", "qunxiang", "ch12.jiumeng", "ch12.qunxiang", true},
    {"ch12_dadian", "trigger_chuansongzhen", "chuansong", "ch12.qunxiang", "ch12.chuansong"},
    {"ch12_guiwu", "trigger_tupo", "tupo", "ch12.chuansong", "ch12.tupo", true},
    {"ch12_guiwu", "trigger_zoupian_a", "zoupian_a", "ch12.tupo", "", true, false},
    {"ch12_guiwu", "trigger_zoupian_b", "zoupian_b", "ch12.tupo", "", true, false},
    {"ch12_guiwu", "trigger_zoupian_c", "zoupian_c", "ch12.tupo", "", true, false},
    {"ch12_guiwu", "trigger_yihai", "yihai", "ch12.tupo", "ch12.z1"},
    {"ch12_guiwu", "trigger_guiwang", "guiwang", "ch12.tupo", "ch12.guiwang", true},
    {"ch12_guiwu", "trigger_huayuan", "yuanyao", "ch12.guiwang", "ch12.yuanyao", true},
    {"ch12_guiwu", "trigger_tingzi", "xiaojie", "ch12.yuanyao", "ch12.xiaojie"},
    {"ch12_xiaojie", "trigger_caiyao", "caiyao", "ch12.xiaojie", "ch12.caiyao"},
    {"ch12_xiaojie", "trigger_shishan", "lingshen", "ch12.xiaojie", "ch12.lingshen"},
    {"ch12_xiaojie", "trigger_fanlian", "fanlian", "ch12.lingshen", "ch12.fanlian", true},
    {"ch12_xiaojie", "trigger_xiagu", "xiagu", "ch12.baishi", "ch12.xiagu", true},
    {"ch12_binghuo", "trigger_guangbi", "rongyan", "ch12.xiagu", "ch12.rongyan", true},
    {"ch12_binghuo", "trigger_shamo", "tihun", "ch12.rongyan", "ch12.tihun", true},
    {"ch12_binghuo", "trigger_jingzhu", "lianjing", "ch12.tihun", "ch12.lianjing"},
    {"ch12_binghuo", "trigger_shidian", "shidian", "ch12.lianjing", "ch12.shidian", true},
    {"ch12_baoguang", "trigger_yuqiao", "quba", "ch12.shidian", "ch12.quba", true},
    {"ch12_baoguang", "trigger_yutai", "yutai", "", "", false, false},
    {"ch12_taqian", "trigger_changlang", "huanjing", "ch12.quba", "ch12.huanjing", true},
    {"ch12_taqian", "trigger_taqian", "neidian", "ch12.huanjing", "ch12.neidian", true},
    {"ch12_taqian", "trigger_tamen", "cengjie", "ch12.neidian", "ch12.cengjie"},
    {"ch12_hanlitai", "trigger_qingshi", "juanzhou", "ch12.cengjie", "ch12.juanzhou"},
    {"ch12_hanlitai", "trigger_taijie", "ruyi", "ch12.juanzhou", "ch12.ruyi", true},
    {"ch12_hanlitai", "trigger_jitan", "butian", "ch12.ruyi", "ch12.butian", true},
    {"ch12_hanlitai", "trigger_dongkou", "xuangu", "ch12.butian", "ch12.xuangu"},
    {"ch12_hanlitai", "trigger_xiaojian", "canhun", "ch12.xuangu", "ch12.canhun"},
}};

struct NpcHook {
    const char* map;
    const char* object;
    const char* script;
    const char* visible;
    const char* done;     // 脚本开头先判的 done_flag（第 11 节「谁置」）
};

// 3.1 表里挂在 NPC 上的五个节点（n06、n07、n09、n18、n33）：无 once，脚本开头判 done_flag。
inline constexpr std::array<NpcHook, 5> kNpcHooks{{
    {"ch12_dixue", "npc_shaonian", "xiaocha", "ch12.duoshe", "ch12.canyuan"},
    {"ch12_tushan", "npc_wen_siyue", "lidao", "ch12.canyuan", "ch12.lidao"},
    {"ch12_dadian", "npc_xuan_gu", "jiumeng", "ch12.rudian", "ch12.jiumeng"},
    {"ch12_xiaojie", "npc_jiyin", "baishi", "ch12.fanlian", "ch12.baishi"},
    {"ch12_lingquan", "npc_yuan_yao_lq", "lingquan", "ch12.canhun", "ch12.done"},
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
    bool operator!=(const Cell& other) const { return !(*this == other); }
};

inline Cell cellOf(const Json& map, const Json& object) {
    return {static_cast<int>(object.at("x").get<double>()) / map.at("tilewidth").get<int>(),
            static_cast<int>(object.at("y").get<double>()) / map.at("tileheight").get<int>()};
}

// 对象盖住的全部格：横贯触发带是 5×1、7×1、1×8 的矩形（18.2 第 2 条），单格对象是 1×1。
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

// 剧本的活代码：每行从第一个 -- 起去掉（与 Ch11Data.h 同一口径）。
inline std::string executableLines(const std::string& source) {
    std::istringstream input(source);
    std::string line, out;
    while (std::getline(input, line)) {
        if (const auto comment = line.find("--"); comment != std::string::npos) line.resize(comment);
        out += line + '\n';
    }
    return out;
}

inline std::map<std::string, std::string> scripts(const std::string& chapterDir = "ch12") {
    std::map<std::string, std::string> result;
    for (const auto& file : fs::directory_iterator(root() / "scripts" / chapterDir))
        if (file.path().extension() == ".lua") result[file.path().stem().string()] = read(file.path());
    return result;
}

inline int countMatches(const std::string& text, const std::regex& pattern) {
    return static_cast<int>(std::distance(std::sregex_iterator(text.begin(), text.end(), pattern), std::sregex_iterator{}));
}

// FNV-1a 64：钉「与第 11 章集成版逐字节相同」的那几份文件（值取自 HEAD 6ec8073 的原文件）。
inline std::uint64_t fnv1a(const std::string& bytes) {
    std::uint64_t hash = 0xcbf29ce484222325ull;
    for (const unsigned char c : bytes) {
        hash ^= c;
        hash *= 0x100000001b3ull;
    }
    return hash;
}
} // namespace fanren::test::ch12
