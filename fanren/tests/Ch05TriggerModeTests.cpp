// 第 5 章每一处挂点是怎么开始的——直接读 tmj，不经过走位、不经过脚本（验收 3）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 3、4 章的教训，docs/README.md「判据是从被测物推导出来的」）
// ---------------------------------------------------------------------------
// 第 3 章两种杀的触发方式装反，四条测试对它满格敏感却一条没发现：驱动是照着 tmj 写的。
// 门禁拦得住「只改地图不改脚本」（@hook 一致性），拦不住「两边一起改」——
// 判据三角缺的是「设计文档 ↔ 地图」那一条边。
//
// 本文件就是那条边：
//   · 不走位、不起脚本、不看旗标，直接读 maps/*.tmj 上的属性；
//   · 判据**逐格抄自 docs/ch05-design.md 3.1 那张表的原文**（mode / once / guard_flag / set_flag），
//     旁边附施工图给的理由，转红时原样打出来；
//   · 形状与 Ch05SliceTests 不同——同一套驱动写两遍，只是把同一个错误钉两遍。
//
// 外加 3.1 表下那几条纪律：全章不用 auto、同一格只挂一个 trigger、内视不写 set_flag、
// 除内视外每一条都写 set_flag 且挂着的脚本真的会置它；以及第 4 节的三道闸门。
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "core/model/Types.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::MapObject;
using fanren::game::Application;

// ---------------------------------------------------------------------------
// 判据。逐行抄自 docs/ch05-design.md 3.1 那张表（"" 表示那一格是「—」）
// ---------------------------------------------------------------------------
struct HookSpec {
    const char* row;       // 表里 # 那一列
    const char* object;
    const char* mapId;
    const char* mode;
    bool once;
    const char* guard;
    const char* setFlag;
    const char* why;
};

const std::vector<HookSpec>& designSaysSo() {
    static const std::vector<HookSpec> kSpecs = {
        {"1a", "trigger_dongqu_lu", "ch05_dukou", "enter", true, "", "ch05.kaipian",
         "狼扑营、毒发作，都是找上他的。放在出生点通往渡口的唯一一条路上"},
        {"1b", "trigger_shangchuan", "ch05_dukou", "interact", true, "ch05.kaipian", "ch05.shangchuan",
         "包一条船顺流而下是他自己动的念头（ch101）"},
        {"2", "trigger_matou", "ch05_xicheng", "enter", true, "", "ch05.matou", "一上岸苦力就围上来"},
        {"3a", "trigger_heishuixiang", "ch05_xicheng", "enter", true, "ch05.matou", "ch05.shoufu",
         "伏击是对方的动作"},
        {"3b", "trigger_zhuishao", "ch05_xicheng", "enter", true, "ch05.shoufu", "ch05.zhuishao",
         "被人跟上是对方的动作。放在西城去南城那道门之前的必经巷口"},
        {"4a", "trigger_qingbao", "ch05_kezhan", "interact", true, "ch05.zhuishao", "ch05.qingbao",
         "他自己坐下把遗书摊开，等孙二狗上门（ch104）"},
        {"4b", "trigger_jieren", "ch05_xicheng", "interact", true, "ch05.qingbao", "ch05.jieren",
         "踹开仓房那扇门是他：孙二狗的人跑来报信，去不去救由他"},
        {"5", "trigger_jiulou", "ch05_nancheng", "interact", true, "ch05.jieren", "ch05.jiulou",
         "他自己上楼挑了临街的座（ch106）"},
        {"6a", "trigger_yeru", "ch05_nancheng", "interact", true, "ch05.jiulou", "ch05.yeru",
         "三更出门翻墙是他定的（ch107）"},
        {"6b", "trigger_toutin", "ch05_mofu", "interact", true, "ch05.yeru", "ch05.toutin",
         "他自己贴到小楼墙上去听"},
        {"7a", "trigger_dengmen", "ch05_mofu", "interact", true, "ch05.toutin", "ch05.dengmen",
         "扔戒指、敲门是他的决定（ch109）"},
        {"7b", "trigger_jianmianli", "ch05_mofu", "enter", true, "ch05.dengmen", "ch05.jianmianli",
         "墨彩环半路站住不走了——是她拦他（ch112-113）"},
        {"8", "trigger_huayuan", "ch05_mofu", "enter", true, "ch05.jianmianli", "ch05.huayuan",
         "撞见吴剑鸣是碰巧（ch114）"},
        {"9", "trigger_duizhi", "ch05_mofu", "interact", true, "ch05.huayuan", "ch05.duizhi",
         "推不推这扇门是他的事，他先放开灵识数了人才进（ch115）"},
        {"10a", "trigger_dingji", "ch05_kezhan", "interact", true, "ch05.duizhi", "ch05.dingji",
         "孙二狗在等，定计的是他（ch119-120）"},
        {"10b", "trigger_xiaoxiangyuan", "ch05_xicheng", "interact", true, "ch05.dingji", "ch05.xiaoxiang",
         "扮小厮敲包房的门是他（ch121）"},
        {"10c", "trigger_shuijiao", "ch05_kezhan", "interact", true, "ch05.xiaoxiang", "ch05.duobang",
         "回客栈倒头就睡是他的老习惯（ch122）；镜头切到四平帮总舵"},
        {"11a", "trigger_jiaoyi", "ch05_mofu", "interact", true, "ch05.duobang", "ch05.jiaoyi",
         "他上门要答复（ch119「最迟明天一早」）"},
        {"11b", "trigger_yange", "ch05_mofu", "enter", true, "ch05.jiaoyi", "ch05.yange",
         "燕歌在他出府的路上拦下他——是燕歌找他"},
        {"11c", "trigger_anpai", "ch05_kezhan", "interact", true, "ch05.yange", "ch05.anpai",
         "他回客栈安排后路（ch124-125）"},
        {"12a", "trigger_majiu", "ch05_mofu", "interact", true, "ch05.anpai", "ch05.shigui",
         "他自己去马厩牵马，这才撞上被撬开的地窖"},
        {"12b", "trigger_zhuwu", "ch05_mofu", "enter", true, "ch05.shigui", "ch05.chuzheng",
         "吴剑鸣从侧门夺路，是撞上的；脚本末尾出发去山庄"},
        {"12c", "trigger_tancha", "ch05_dubashanzhuang", "interact", true, "ch05.chuzheng", "ch05.tancha",
         "刺探是他（ch126「不停刺探和潜入」）"},
        {"12d", "trigger_shangyue", "ch05_dubashanzhuang", "interact", true, "ch05.tancha", "ch05.cisha",
         "挑欧阳飞天独自赏月那一夜下手——是他挑的时候"},
        {"12e", "trigger_huanyu", "ch05_mofu", "interact", true, "ch05.cisha", "ch05.done",
         "他提着首级回来交换。章末"},
        {"Z1", "trigger_chaoxie", "ch05_kezhan", "interact", true, "ch05.fengwu_qiu", "ch05.fengwu_chao",
         "誊抄是他"},
        {"Z2", "trigger_lian_jianfu", "ch05_kezhan", "interact", true, "ch05.jianfu_qiu", "ch05.lian_jianfu",
         "练符是他"},
        {"Z2′", "trigger_lian_jianfu_lin", "ch05_dubashanzhuang", "interact", true, "ch05.jianfu_qiu",
         "ch05.lian_jianfu", "同一个脚本挂两处：给「没练就来了、打不过退出来」的人留一条路（8.4）"},
        {"—", "trigger_neishi", "ch05_kezhan", "interact", false, "", "",
         "内视看寒毒到了哪一段，可重复，不写 set_flag（规则 15）"},
    };
    return kSpecs;
}

// 施工图第 4 节：新建的六张图。
constexpr const char* kChapterMaps[] = {"ch05_dukou", "ch05_xicheng", "ch05_nancheng",
                                         "ch05_kezhan", "ch05_mofu", "ch05_dubashanzhuang"};

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch05_mofu.tmj") &&
            fs::exists(root / "scripts" / "ch05" / "huanyu.lua")) {
            return candidate;
        }
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

// 去掉 lua 注释之后的正文（只认行注释 --，本章脚本没有块注释）。
std::string codeOnly(const std::string& source) {
    std::istringstream lines(source);
    std::string line;
    std::string out;
    while (std::getline(lines, line)) {
        const std::size_t dash = line.find("--");
        out += (dash == std::string::npos ? line : line.substr(0, dash)) + "\n";
    }
    return out;
}

class Ch05TriggerMode : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    std::vector<MapObject> objectsOn(const std::string& mapId) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok || app_.currentMap() == nullptr) return {};
        return app_.currentMap()->objects;
    }
    MapObject objectOn(const std::string& mapId, const std::string& name) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    Application app_;
    std::string root_;
};

// ---------------------------------------------------------------------------
// 先验：表里每一处挂点确实在图上，而且 mode / once / script 真的写了
// ---------------------------------------------------------------------------
TEST_F(Ch05TriggerMode, EveryHookTheDesignTableNamesIsReallyOnItsMap) {
    ASSERT_EQ(designSaysSo().size(), 29u)
        << "判据表的行数变了。改它之前先改 docs/ch05-design.md 3.1——那张表是上游";
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.row << "：" << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.type, "trigger") << spec.object;
        EXPECT_FALSE(object.property("mode").empty()) << spec.object << " 没写 mode——下面会变成比两个空串";
        EXPECT_FALSE(object.property("once").empty()) << spec.object << " 没写 once";
        EXPECT_FALSE(object.property("script").empty()) << spec.object << " 没写 script";
    }
}

// ---------------------------------------------------------------------------
// 正题：mode / once / guard_flag / set_flag 逐格对 3.1 表
// ---------------------------------------------------------------------------
TEST_F(Ch05TriggerMode, EveryHookStartsTheWayTheDesignTableSays) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.property("mode"), spec.mode)
            << spec.row << " " << spec.object << " 的触发方式与施工图 3.1 不符：" << spec.why;
        EXPECT_EQ(object.property("once") == "true", spec.once)
            << spec.row << " " << spec.object << " 的 once 与施工图 3.1 不符（读到 \""
            << object.property("once") << "\"）";
        EXPECT_EQ(object.property("guard_flag"), spec.guard)
            << spec.row << " " << spec.object << " 的 guard_flag 与施工图 3.1 不符";
        EXPECT_EQ(object.property("set_flag"), spec.setFlag)
            << spec.row << " " << spec.object << " 的 set_flag 与施工图 3.1 不符";
    }
}

// 3.1 表下第一条：once 除内视外全为真，每一条都必须写 set_flag——而且挂着的脚本**真的会置它**
//（否则 once 永远烧不掉，那一幕可以无限重演，advance_days 跟着把日历刷到任意一天）。
// 内视不写 set_flag，脚本里也一个旗标都不置（规则 15：可重复的挂点不许有副作用）。
TEST_F(Ch05TriggerMode, EveryOnceHookIsBurntByTheScriptItStartsAndLookingInwardBurnsNothing) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.object;
        const fs::path script = fs::path(root_) / "scripts" / object.property("script");
        const std::string code = codeOnly(readFile(script));
        ASSERT_GT(code.size(), 40u) << "先验：读得到 " << script.string();
        if (spec.once) {
            EXPECT_NE(code.find(std::string("flag.set(\"") + spec.setFlag + "\""), std::string::npos)
                << spec.row << " " << spec.object << " 挂的脚本 " << object.property("script")
                << " 一处也不置 " << spec.setFlag << "：once 永远烧不掉";
        } else {
            EXPECT_EQ(code.find("flag.set("), std::string::npos)
                << spec.object << " 可重复，却在脚本里置旗标（施工图 3.1：不写 set_flag，规则 15）";
        }
    }
}

// 3.1 表下第二条：同一格只挂一个 trigger（objectAt 只取第一个，叠着的第二个永远轮不到）；
// 本章不用 auto（map_spec 列了它，WorldScene 只实现了 enter 与 interact）；
// 六张图上的挂点都是表里的挂点，没有表外的。
TEST_F(Ch05TriggerMode, NoAutoNoTwoTriggersOnOneCellAndNothingOutsideTheTable) {
    std::set<std::string> listed;
    for (const HookSpec& spec : designSaysSo()) listed.insert(std::string(spec.mapId) + "/" + spec.object);
    int triggersSeen = 0;
    for (const char* mapId : kChapterMaps) {
        std::map<std::pair<int, int>, std::string> owner;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type != "trigger") continue;
            ++triggersSeen;
            EXPECT_NE(object.property("mode"), "auto")
                << mapId << " / " << object.name << " 用了 auto（施工图 3.1：本章不用 auto）";
            EXPECT_TRUE(listed.count(std::string(mapId) + "/" + object.name))
                << mapId << " / " << object.name << " 不在施工图 3.1 的表里";
            for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                    const auto cell = std::make_pair(object.position.x + dx, object.position.y + dy);
                    const auto [it, fresh] = owner.emplace(cell, object.name);
                    EXPECT_TRUE(fresh) << mapId << " (" << cell.first << "," << cell.second << ") 上挂了两个 trigger："
                                       << it->second << " 与 " << object.name << "（施工图 3.1：同一格只挂一个）";
                }
            }
        }
    }
    EXPECT_EQ(triggersSeen, 29) << "先验：六张图上一共 29 处挂点，与表的行数一致";
}

// 内视摆在蒲团上，不与 10c 的床同格（施工图 3.1 最后一行）。
TEST_F(Ch05TriggerMode, LookingInwardSitsApartFromTheBedOfTheNightOfTheTakeover) {
    const MapObject neishi = objectOn("ch05_kezhan", "trigger_neishi");
    const MapObject bed = objectOn("ch05_kezhan", "trigger_shuijiao");
    ASSERT_FALSE(neishi.name.empty());
    ASSERT_FALSE(bed.name.empty());
    for (int y = neishi.position.y; y < neishi.position.y + std::max(1, neishi.height); ++y) {
        for (int x = neishi.position.x; x < neishi.position.x + std::max(1, neishi.width); ++x) {
            const bool onBed = x >= bed.position.x && x < bed.position.x + std::max(1, bed.width) &&
                               y >= bed.position.y && y < bed.position.y + std::max(1, bed.height);
            EXPECT_FALSE(onBed) << "内视 (" << x << "," << y << ") 与 10c 的床同格";
        }
    }
}

// ---------------------------------------------------------------------------
// 施工图第 4 节那张地图表「要点」一列里点名的设施：存档点、药炉（1 品）、告示板、药铺
// ---------------------------------------------------------------------------
// 只钉表里写了的：渡口、西城、客栈、墨府、山庄各有存档点；客栈有 1 品药炉与告示板；南城有那家药铺。
// 南城表里没写存档点，这里不判它有没有（写明，免得以为它管着）。
TEST_F(Ch05TriggerMode, TheFacilitiesTheMapTableNamesAreOnTheirMaps) {
    const auto facilitiesOf = [&](const char* mapId) {
        std::vector<MapObject> out;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "facility") out.push_back(object);
        }
        return out;
    };
    const auto has = [&](const char* mapId, const std::string& kind, const std::string& key,
                         const std::string& value) {
        for (const MapObject& f : facilitiesOf(mapId)) {
            if (f.property("kind") == kind && (key.empty() || f.property(key) == value)) return true;
        }
        return false;
    };
    for (const char* mapId : {"ch05_dukou", "ch05_xicheng", "ch05_kezhan", "ch05_mofu", "ch05_dubashanzhuang"}) {
        EXPECT_TRUE(has(mapId, "save", "", "")) << mapId << "：施工图第 4 节写了存档点";
    }
    EXPECT_TRUE(has("ch05_kezhan", "alchemy", "grade", "1")) << "施工图第 4 节：客栈药炉（alchemy 1 品）";
    EXPECT_TRUE(has("ch05_kezhan", "board", "", "")) << "施工图第 4 节：客栈告示板";
    EXPECT_TRUE(has("ch05_nancheng", "shop", "ref_id", "ch05_nancheng_yaopu")) << "施工图第 4、9 节：南城药铺";
}

// ---------------------------------------------------------------------------
// 施工图第 4 节的三道闸门：各指一张不同的图，deny_text_key 按目标图命名，不许有第四道
// ---------------------------------------------------------------------------
TEST_F(Ch05TriggerMode, TheThreeGatesAreTheOnesSectionFourDrawsAndThereIsNoFourth) {
    struct Gate {
        const char* mapId;
        const char* portal;
        const char* target;
        const char* requireFlag;
        const char* denyKey;
    };
    // 连通图：韩家村 ─(ch04.done)→ 渡口；西城 ─(ch05.zhuishao)→ 南城；南城 ─(ch05.dengmen)→ 墨府正门。
    const std::vector<Gate> kGates = {
        {"ch01_hanjiacun", "portal_to_dukou", "ch05_dukou", "ch04.done", "ch05.block.dukou"},
        {"ch05_xicheng", "portal_to_nancheng", "ch05_nancheng", "ch05.zhuishao", "ch05.block.nancheng"},
        {"ch05_nancheng", "portal_to_mofu", "ch05_mofu", "ch05.dengmen", "ch05.block.mofu"},
    };
    for (const Gate& gate : kGates) {
        const MapObject portal = objectOn(gate.mapId, gate.portal);
        ASSERT_FALSE(portal.name.empty()) << gate.mapId << " 上没有 " << gate.portal;
        EXPECT_EQ(portal.type, "portal");
        EXPECT_EQ(portal.property("target_map"), gate.target) << gate.portal;
        EXPECT_EQ(portal.property("require_flag"), gate.requireFlag) << gate.portal;
        EXPECT_EQ(portal.property("deny_text_key"), gate.denyKey) << gate.portal << "（规则 20：按目标图命名）";
        const std::string deny = app_.data().lookupText(gate.denyKey);
        EXPECT_NE(deny, gate.denyKey) << gate.denyKey << " 查不到：拦下玩家时屏幕上会是一串 key";
    }
    // 不许有第四道：六张图上带 require_flag 的门只有上面两道（第三道在韩家村）。
    int gated = 0;
    for (const char* mapId : kChapterMaps) {
        for (const MapObject& object : objectsOn(mapId)) {
            // 只数进第 5 章的图的闸门：南城东门（portal_to_tainan_cun，往第 6 章的太南山脚）
            // 归 tools/mapgen/genmaps_ch06.py 所有，不是这一节说的「第四道」。
            if (object.type == "portal" && !object.property("require_flag").empty() &&
                object.property("target_map").rfind("ch05_", 0) == 0) {
                ++gated;
            }
        }
    }
    EXPECT_EQ(gated, 2) << "施工图第 4 节：不要再加第四道指向南城或墨府的闸门";
    // 山庄那张图没有任何门：去是 12b 的 teleport，回是 12d 得手的 teleport（第 4 节连通图）。
    for (const MapObject& object : objectsOn("ch05_dubashanzhuang")) {
        EXPECT_NE(object.type, "portal") << "独霸山庄上出现了门 " << object.name << "：第 4 节只用 teleport 进出";
    }
}

}  // namespace
