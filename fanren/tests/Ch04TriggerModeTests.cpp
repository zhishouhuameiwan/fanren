// 本章每一处挂点是怎么开始的——直接读 tmj，不经过走位、不经过脚本。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 4 章独立校对 MEDIUM-1）
// ---------------------------------------------------------------------------
// 第 3 章栽过一次：两种杀的触发方式**恰好装反**，而四条测试对它满格敏感却一条也
// 没发现——因为驱动函数是照着 tmj 写的，于是地图当时的样子原样固化成了四条绿灯。
// 那一课的产出是 `tests/Ch03TriggerModeTests.cpp`。
//
// 第 4 章把那一课学了一半：门禁现在拦得住「只改地图不改脚本」
//（`tools/validate.py` 的 `@hook` 一致性检查），但**两边一起改仍然全绿**——
// 因为设计文档从来没有写死过任何一处的 `mode`，没有「应该是什么」可抄。
// 判据三角的三条边，本章只有「脚本 ↔ 地图」那一条。
//
// 本文件补的是第二条边：**设计文档 ↔ 地图**。
//   · 不走位、不起脚本、不看旗标，直接读 `maps/*.tmj` 上的属性；
//   · 判据抄自 `docs/ch04-design.md` 第 3.1 节那张表的原文，
//     **不是「地图现在是什么样」**；
//   · 形状刻意与 `Ch04SliceTests` 不同——同一套驱动写两遍，只是把同一个错误钉两遍。
//
// 第三条边（引擎里 enter 与 interact 真的是两回事）已由
// `Ch03TriggerMode.TheExecutionOnlyStartsWhenThePlayerPressesConfirm` 一族钉住，
// 那是全局语义，不随章节变，这里不重复。
//
// ---------------------------------------------------------------------------
// 分法只有一条：这件事是他做的，还是发生在他身上的
// ---------------------------------------------------------------------------
// `enter`    —— 发生在他身上。副门主叫他、敌人来犯、金光上人堵上门、夜袭。
//               他只是正好在场，没有人问过他。
// `interact` —— 他做的。自己走到炉前、自己爬上崖台、自己上演武台、
//               自己蹲到焦土里拨灰、自己坐下写信、自己上车。
//
// 节点 10 那一条最要紧，理由与第 3 章的处决同一个：焦土不在任何一条路上，
// **他得自己走过去、自己按下确认**。改成 `enter`，那一堆战利品就成了他路过时
// 被塞进包里的东西，而第 6 章那条伏笔的分量全在「是他自己捡的」这四个字上。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/Application.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::MapObject;
using fanren::game::Application;

// ---------------------------------------------------------------------------
// 判据。逐行抄自 docs/ch04-design.md 第 3.1 节那张表
// ---------------------------------------------------------------------------
struct HookSpec {
    const char* mapId;
    const char* object;
    const char* script;
    const char* mode;
    bool once;
    const char* why;   // 设计文档给的理由，转红时原样打出来
};

const std::vector<HookSpec>& designSaysSo() {
    static const std::vector<HookSpec> kSpecs = {
        {"ch04_getang", "trigger_liuxia", "ch04/liuxia.lua", "enter", true,
         "节点 1：副门主叫他过去，不是他挑的时候"},
        {"ch04_luorifeng", "trigger_huodan", "ch04/huodan.lua", "enter", true,
         "节点 2：峰道是上峰唯一的口子；他住上来，接下来那一冬是叙述，不是一次按键"},
        {"ch04_luorifeng", "trigger_kailu", "ch04/kailu.lua", "interact", true,
         "节点 3：他自己走到炉前、自己动手"},
        {"ch04_luorifeng", "trigger_yufeng", "ch04/yufeng.lua", "interact", true,
         "节点 4：他自己爬上崖台去练，没人叫他"},
        {"ch04_yanwuchang", "trigger_qiecuo", "ch04/qiecuo.lua", "interact", true,
         "节点 5：他自己上的演武台。硬约束 2——这打法是他自己摸出来的"},
        {"ch04_yanwuchang", "trigger_kaizhan", "ch04/kaizhan.lua", "enter", true,
         "节点 6：敌人来犯，他只是正好在场"},
        {"ch04_shanxiazhen", "trigger_jia_tianlong", "ch04/jia_tianlong.lua", "enter", true,
         "节点 7：现身是对方的动作"},
        {"ch04_getang", "trigger_jinguang", "ch04/jinguang.lua", "enter", true,
         "节点 8：本章压迫感顶点——撞上，不是求见"},
        {"ch04_yanwuchang", "trigger_gongfang", "ch04/gongfang.lua", "enter", true,
         "节点 9：夜袭，没有人问过他"},
        {"ch04_yanwuchang", "trigger_zhanlipin", "ch04/zhanlipin.lua", "interact", true,
         "节点 10：焦土不在任何一条路上，他得自己走过去、自己蹲下拨灰"},
        {"ch01_hanjiacun", "trigger_huicun", "ch04/huicun.lua", "enter", true,
         "节点 11a：他回到村口就撞上了那场婚事，躲是撞上之后才做的"},
        {"ch01_hanjiacun", "trigger_xinbie", "ch04/xinbie.lua", "interact", true,
         "节点 11b：他自己坐下写的"},
        {"ch01_hanjiacun", "trigger_dongqu", "ch04/dongqu.lua", "interact", true,
         "节点 11c：他自己上的车"},
    };
    return kSpecs;
}

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch04_yanwuchang.tmj") &&
            fs::exists(root / "scripts" / "ch04" / "zhanlipin.lua")) {
            return candidate;
        }
    }
    return ".";
}

class Ch04TriggerMode : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }

    void TearDown() override { app_.shutdown(); }

    MapObject objectOn(const char* mapId, const char* name) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok) return MapObject{};
        const fanren::core::TileMap* map = app_.currentMap();
        if (map == nullptr) return MapObject{};
        for (const MapObject& object : map->objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    Application app_;
};

// ---------------------------------------------------------------------------
// 先验：这十三处挂点确实在图上，而且 mode 真的写了
// ---------------------------------------------------------------------------
// 少了这一条，下面每一句都可能是在一个 MapObject{} 空壳上比较空字符串——
// 「找不到某个词就算过」那一类空转法，本项目栽过。
// 空的 `mode` 与任何一个错的取值在下面的比较里长得完全不一样，
// 所以这一条保证下面比较的是内容，不是缺失。
TEST_F(Ch04TriggerMode, EveryHookTheDesignDocNamesIsReallyOnItsMap) {
    ASSERT_EQ(designSaysSo().size(), 13u)
        << "判据表的行数变了。改它之前先改 docs/ch04-design.md 第 3.1 节——"
           "那张表是上游，这里只是它的机器形式";
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty())
            << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.type, "trigger") << spec.object;
        EXPECT_FALSE(object.property("mode").empty())
            << spec.object << " 没写 mode——下面那条比较会变成在比两个空串";
        EXPECT_FALSE(object.property("script").empty()) << spec.object << " 没写 script";
    }
}

// ---------------------------------------------------------------------------
// 正题：每一处是他做的，还是发生在他身上的
// ---------------------------------------------------------------------------
TEST_F(Ch04TriggerMode, EveryHookStartsTheWayTheDesignDocSaysItDoes) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.property("mode"), spec.mode)
            << spec.mapId << " / " << spec.object << " 的触发方式与设计文档不符。"
            << "docs/ch04-design.md 3.1：" << spec.why;
        EXPECT_EQ(object.property("script"), spec.script)
            << spec.object << " 指着另一个脚本";
    }
}

// `once` 全为真是设计文档 3.1 节写死的。写成可重复触发，本章每一节都能反复演——
// 而 `advance_days` 会跟着反复走，玩家能把日历刷到任意一天。
TEST_F(Ch04TriggerMode, NoneOfThemCanBePlayedTwice) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.mapId << " 上没有 " << spec.object;
        const std::string once = object.property("once");
        ASSERT_FALSE(once.empty()) << spec.object << " 没写 once";
        EXPECT_EQ(once == "true", spec.once)
            << spec.object << " 的 once 与设计文档不符（读到 \"" << once << "\"）";
    }
}

}  // namespace
