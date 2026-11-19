#pragma once
// 伙伴改版 D 路（界面与历练）用例的公共夹具：一份临时资产根（data/、scripts/、maps/ 的拷贝），
// 往里放探针伙伴、探针历练地、探针地图与编成，都不碰仓库。CompanionsLilianTests 与 CompanionsMenuTests 共用。
//
// 探针伙伴（角色都是 data/roles 里现成的）：
//   probe_a：zhang_tie（probe.a.grown 置了就过去）→ li_feiyu；met probe.a.met；
//            合意 material_dansha、不喜 material_fuzhi，别的算寻常
//   probe_b：baiyi_shaonv；probe_c：chen_shimei；probe_d：cao_lu（met 各是 probe.<x>.met）
//   probe_e：ding_lao；probe_f … probe_o：十位各挂一个现成角色（「名册一长」那条用例用）。
//   met 旗标都是 probe.<x>.met，用例自己置；没置的就不在名册上。
// 探针历练地 probe_jindi：入口 lilian_probe_a 的 rukou；用到 lilian_probe_a、lilian_probe_b；
//   三档上限 QiRefining13 / FoundationEarly / FoundationMid；解锁旗标 probe.lilian.open。
// 探针地图：probe_home（韩立站在 (12,11) 面朝左，(11,11) 是 kind=lilian、site=probe_jindi 的设施）；
//   lilian_probe_a（同样的站位，(11,11) 是 kind=lilian 的出口）；lilian_probe_b。两张历练图上各有
//   kChanceObjects 个带 chance=50 的 foe 与 chest，另各一个不带 chance 的 foe 与 chest、一道 gate。
// 另两处：probe_missing（入口图 lilian_probe_missing 不存在）；probe_lack（两档，图上一只 foe 的编成
//   只有 probe_lack_t1）。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "game/Application.h"
#include "game/WorldScene.h"

namespace fanren::test::laned {

namespace fs = std::filesystem;

inline constexpr int kChanceObjects = 12;   // 每张历练图上带 chance 的 foe、chest 各几个
inline constexpr const char* kSite = "probe_jindi";

inline fs::path repoRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) return candidate;
    }
    return ".";
}

inline void writeFile(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

inline std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// 测试目标拿不到 nlohmann（fanren_io 只 PRIVATE 链接它），地图与编成按原文拼。值里不含引号与反斜杠。
inline std::string prop(const std::string& name, const std::string& value) {
    return R"({"name": ")" + name + R"(", "type": "string", "value": ")" + value + R"("})";
}
inline std::string prop(const std::string& name, const char* value) { return prop(name, std::string(value)); }
inline std::string prop(const std::string& name, bool value) {
    return R"({"name": ")" + name + R"(", "type": "bool", "value": )" + (value ? "true" : "false") + "}";
}
inline std::string prop(const std::string& name, int value) {
    return R"({"name": ")" + name + R"(", "type": "int", "value": )" + std::to_string(value) + "}";
}

inline std::string object(int id, const std::string& name, const std::string& type, int cellX, int cellY,
                          std::initializer_list<std::string> props) {
    std::ostringstream out;
    out << R"({"id": )" << id << R"(, "name": ")" << name << R"(", "type": ")" << type << R"(", "x": )"
        << cellX * 32 << R"(, "y": )" << cellY * 32
        << R"(, "width": 32, "height": 32, "rotation": 0, "visible": true, "properties": [)";
    bool first = true;
    for (const std::string& p : props) {
        out << (first ? "" : ", ") << p;
        first = false;
    }
    out << "]}";
    return out.str();
}

// 拿 ch12_lingquan.tmj 的底图与属性，换上 map_id 与一套对象，写成 maps/<mapId>.tmj。
// 那张图的对象层是文件里最后一个图层，"objects": [ 只出现一次，后面直到文件末尾都是对象。
inline void writeMap(const fs::path& root, const std::string& mapId, const std::vector<std::string>& objects) {
    std::string text = readFile(repoRoot() / "maps" / "ch12_lingquan.tmj");
    const std::string idValue = R"("value": "ch12_lingquan")";
    const auto idAt = text.find(idValue);
    const auto objectsAt = text.find(R"("objects": [)");
    if (idAt == std::string::npos || objectsAt == std::string::npos) {
        ADD_FAILURE() << "ch12_lingquan.tmj 的原文变了，夹具要跟着改";
        return;
    }
    std::string out = text.substr(0, objectsAt) + R"("objects": [)";
    out.replace(idAt, idValue.size(), R"("value": ")" + mapId + R"(")");
    for (std::size_t i = 0; i < objects.size(); ++i) out += (i == 0 ? "\n" : ",\n") + objects[i];
    out += "\n      ]\n    }\n  ]\n}\n";
    writeFile(root / "maps" / (mapId + ".tmj"), out);
}

// 站位：(12,11) 面朝左，(11,11) 一处 kind=lilian 的设施（历练外是入口，历练里是出口）。
inline std::vector<std::string> standAndFacility(const std::string& spawnId, const std::string& site = kSite) {
    return {object(1, spawnId, "spawn", 12, 11, {prop("id", spawnId), prop("default", true), prop("facing", "left")}),
            object(2, "facility_lilian", "facility", 11, 11, {prop("kind", "lilian"), prop("site", site)})};
}

inline std::vector<std::string> lilianMapObjects(const std::string& mapId) {
    std::vector<std::string> objects = standAndFacility("rukou");
    int next = 10;
    for (int i = 0; i < kChanceObjects; ++i) {
        const std::string n = std::to_string(i);
        objects.push_back(object(next++, "foe" + n, "foe", 1 + i, 2,
                                 {prop("id", mapId + ".foe" + n), prop("battle", "probe_foe"),
                                  prop("look", "zhang_tie"), prop("facing", "down"), prop("chance", 50)}));
        objects.push_back(object(next++, "chest" + n, "chest", 1 + i, 4,
                                 {prop("id", mapId + ".chest" + n), prop("loot", "pill_huiqi_dan"),
                                  prop("chance", 50)}));
    }
    objects.push_back(object(next++, "foe_fixed", "foe", 1, 6,
                             {prop("id", mapId + ".foe_fixed"), prop("battle", "probe_foe"),
                              prop("look", "zhang_tie"), prop("facing", "down")}));
    objects.push_back(object(next++, "chest_fixed", "chest", 2, 6,
                             {prop("id", mapId + ".chest_fixed"), prop("loot", "pill_huiqi_dan")}));
    objects.push_back(object(next++, "gate", "gate", 3, 6, {prop("id", mapId + ".gate"), prop("skill", "break_seal")}));
    return objects;
}

inline std::string companionJson(const std::string& id, const std::string& roles, const std::string& extra = {}) {
    return R"({"id": ")" + id + R"(", "met_flag": "probe.)" + id.substr(6) + R"(.met", "roles": )" + roles +
           extra + "}";
}

// 一份编成：拿 b03_andao_shishou 换个 id。
inline void writeBattle(const fs::path& root, const std::string& id) {
    std::string text = readFile(repoRoot() / "data" / "battles" / "b03_andao_shishou.json");
    const std::string from = R"("id": "b03_andao_shishou")";
    const auto at = text.find(from);
    if (at == std::string::npos) {
        ADD_FAILURE() << "b03_andao_shishou.json 的原文变了，夹具要跟着改";
        return;
    }
    text.replace(at, from.size(), R"("id": ")" + id + R"(")");
    writeFile(root / "data" / "battles" / (id + ".json"), text);
}

inline constexpr const char* kSiteJson = R"({
  "id": "probe_jindi", "name_key": "probe.lilian.name", "unlock_flag": "probe.lilian.open",
  "entry": {"map": "lilian_probe_a", "spawn": "rukou"},
  "maps": ["lilian_probe_a", "lilian_probe_b"],
  "tiers": [{"max_realm": "QiRefining13"}, {"max_realm": "FoundationEarly"}, {"max_realm": "FoundationMid"}],
  "note": "D 路用例的探针"
})";

// 建一份资产根（夹具头上那一套探针）。
inline std::unique_ptr<TempDir> makeRoot(const char* prefix) {
    auto root = std::make_unique<TempDir>(prefix);
    for (const char* dir : {"data", "scripts", "maps"}) {
        fs::copy(repoRoot() / dir, root->path() / dir, fs::copy_options::recursive);
    }
    // 真树里从样板段起有了伙伴数据（nangong_wan、chen_qiaoqian），它们的角色与下面的探针伙伴重叠
    // （一个角色只能属于一位伙伴，加载器拒收）。这些用例量的是探针伙伴，拷进来之后先把真伙伴拿掉。
    fs::remove_all(root->path() / "data" / "companions");
    const fs::path companions = root->path() / "data" / "companions";
    writeFile(companions / "probe_a.json",
              companionJson("probe_a", R"([{"role": "zhang_tie", "until_flag": "probe.a.grown"}, {"role": "li_feiyu"}])",
                            R"(, "gifts": {"like": ["material_dansha"], "dislike": ["material_fuzhi"]})"));
    writeFile(companions / "probe_b.json", companionJson("probe_b", R"([{"role": "baiyi_shaonv"}])"));
    writeFile(companions / "probe_c.json", companionJson("probe_c", R"([{"role": "chen_shimei"}])"));
    writeFile(companions / "probe_d.json", companionJson("probe_d", R"([{"role": "cao_lu"}])"));
    writeFile(companions / "probe_e.json", companionJson("probe_e", R"([{"role": "ding_lao"}])"));
    // 名册一长的那条用例用：probe_f … probe_o，各挂一个现成的角色。
    const char* const longRoster[][2] = {{"f", "baichi_sanxiu_a"}, {"g", "baichi_sanxiu_b"}, {"h", "baichi_sanxiu_c"},
                                         {"i", "baimen_zhanggui"}, {"j", "baishuilou_huoji"}, {"k", "baiyi_qingnian"},
                                         {"l", "baiyi_xiushi"},    {"m", "budai_guairen"},    {"n", "cang_yunlong"},
                                         {"o", "chen_dage"}};
    for (const auto& [suffix, role] : longRoster) {
        const std::string id = std::string("probe_") + suffix;
        writeFile(companions / (id + ".json"), companionJson(id, std::string(R"([{"role": ")") + role + R"("}])"));
    }
    writeFile(root->path() / "data" / "lilian" / "probe_jindi.json", kSiteJson);
    // 入口图根本不存在的一处（形状是对的，加载器只查形状）：回滚那一支的用例用它。
    writeFile(root->path() / "data" / "lilian" / "probe_missing.json", R"({
      "id": "probe_missing", "name_key": "probe.lilian.name", "unlock_flag": "probe.lilian.open",
      "entry": {"map": "lilian_probe_missing"}, "maps": ["lilian_probe_missing"],
      "tiers": [{"max_realm": "QiRefining13"}]})");
    writeFile(root->path() / "data" / "text" / "zz_probe_laned.json", R"({"probe.lilian.name": "探针旧境"})");
    for (const char* id : {"probe_foe", "probe_foe_t1", "probe_foe_t2", "probe_foe_t3"}) writeBattle(root->path(), id);
    writeMap(root->path(), "probe_home", standAndFacility("zhanwei"));
    writeMap(root->path(), "lilian_probe_a", lilianMapObjects("lilian_probe_a"));
    writeMap(root->path(), "lilian_probe_b", lilianMapObjects("lilian_probe_b"));
    // 缺一档编成的历练地：两档，图上一只 foe 的编成只有 _t1。
    writeFile(root->path() / "data" / "lilian" / "probe_lack.json", R"({
      "id": "probe_lack", "name_key": "probe.lilian.name", "unlock_flag": "probe.lilian.open",
      "entry": {"map": "lilian_probe_lack"}, "maps": ["lilian_probe_lack"],
      "tiers": [{"max_realm": "QiRefining13"}, {"max_realm": "FoundationMid"}]})");
    writeBattle(root->path(), "probe_lack_t1");
    {
        std::vector<std::string> lack = standAndFacility("rukou", "probe_lack");
        lack.push_back(object(10, "foe_lack", "foe", 1, 2,
                              {prop("id", "lilian_probe_lack.foe"), prop("battle", "probe_lack"),
                               prop("look", "zhang_tie"), prop("facing", "down")}));
        writeMap(root->path(), "lilian_probe_lack", lack);
    }
    return root;
}

inline void pushKey(SDL_Scancode scancode, bool down) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.down = down;
    SDL_PushEvent(&event);
}

// 每条用例一个新开的 Application，站在 probe_home 的世界层上。
class LaneD : public ::testing::Test {
protected:
    static void SetUpTestSuite() { root_ = makeRoot("fanren_companions_laned"); }
    static void TearDownTestSuite() { root_.reset(); }

    void SetUp() override {
        auto ready = app_.init(root_->path().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        ASSERT_TRUE(app_.loadMap("probe_home", std::string{}).ok);
        app_.state().day = 40;
        app_.pushScene(std::make_unique<fanren::game::WorldScene>());
        tick(2);
        ASSERT_EQ(top(), "World");
    }
    void TearDown() override { app_.shutdown(); }

    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    void press(SDL_Scancode scancode) {
        pushKey(scancode, true);
        tick();
        pushKey(scancode, false);
        tick();
    }
    std::string top() {
        const fanren::game::Scene* scene = app_.topScene();
        return scene == nullptr ? std::string{} : scene->name();
    }
    void meet(const std::string& companion) { app_.state().setFlag("probe." + companion + ".met"); }
    std::string lastLine() const {
        const auto& log = app_.dialogueLog();
        return log.empty() ? std::string{} : log.back().body;
    }

    static inline std::unique_ptr<TempDir> root_;
    fanren::game::Application app_;
};

}  // namespace fanren::test::laned
