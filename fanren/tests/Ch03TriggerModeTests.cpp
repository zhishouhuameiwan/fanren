// 两种杀是用什么键开始的——直接钉住，不经过走位、不经过脚本。
//
// ---------------------------------------------------------------------------
// 为什么要单独有这一个文件（第 3 章独立校对 HIGH-1）
// ---------------------------------------------------------------------------
// 本章的道德分量全在一处对比上（大纲注解、设计第 0.3 节）：
//
//   节点 10 夺舍  —— 墨居仁趁韩立**睡着**动手。玩家不该按下任何一个键，
//                    所以关卡侧必须是 mode=enter：走到床边就发生。
//   节点 11 处决  —— 韩立醒着，自己走过去、自己按下确认。所以必须是
//                    mode=interact：路过它一百次也不该开演。
//
// 这两行一度**恰好装反**。两个脚本的首部一直写着正确的那一半，整段设计论证
// 建立在上头，而地图那一侧写的是反的。没有任何一条门禁看得见这件事：
// 文案有禁词扫描、几何有 MAPGEN_IN_SYNC、校验器有 SELFTEST_OK，
// 唯独「脚本首部说自己怎么触发」全靠人看。
//
// 更难办的是：**当时的测试把错的那一半钉成了绿灯。** tests/Ch03SliceTests.cpp
// 是照着 tmj 写驱动的——哪个 trigger 是什么 mode，就拿哪个函数去驱动它，
// 于是地图当时的样子原样固化成了四条绿灯，把地图改回设计要求的样子反而会转红。
// 一条绿着的、钉住错误行为的测试比一条没牙的测试更难发现：没人会去动一条绿的。
//
// 根因不是「忘了断言」，是**断言的形状不对**：mode 当时只体现在「用哪个函数去
// 驱动」，而那是通关路线的实现细节，不是任何人会当成规格来读的东西。所以这一条
// 用例刻意做成另一个形状：
//
//   · 不走位、不起脚本、不看旗标，直接读 maps/ch02_jusuo.tmj 上那两个属性；
//   · 判据写死成设计文档那两句话，而不是「地图现在是什么样」；
//   · 另配一条**引擎语义**的断言：证明 enter 与 interact 在引擎里真的是两回事。
//     少了这一条，将来若有人让 tryStep 也去点 interact 触发，上面那两句照样绿，
//     而游戏里两种杀又会变成同一种。
//
// 与邻居的分工：
//   * tests/Ch03SliceTests.cpp        —— 整章走通；那里验的是「按这个 mode 玩得
//     下去」，本文件验的是「这个 mode 本身对不对」。两件事，两条用例。
//   * tests/TriggerTests.cpp          —— 触发器机制本身（once / guard_flag）。
#include <gtest/gtest.h>

#include <filesystem>
#include <string>

#include "core/model/Types.h"
#include "game/Application.h"
#include "game/WorldScene.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::MapObject;
using fanren::core::Point;
using fanren::game::Application;
using fanren::game::WorldScene;

constexpr const char* kMapJusuo = "ch02_jusuo";
constexpr const char* kDuoshe = "trigger_duoshe";     // 节点 10：夺舍
constexpr const char* kChujue = "trigger_chujue";     // 节点 11：处决

// 与 Ch03SliceTests 同一套找根目录的写法：找错了报的是「找不到 ch03 的东西」。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch03" / "duoshe.lua") &&
            fs::exists(root / "maps" / "ch02_jusuo.tmj")) {
            return candidate;
        }
    }
    return ".";
}

class Ch03TriggerMode : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        auto loaded = app_.loadMap(kMapJusuo, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }

    void TearDown() override { app_.shutdown(); }

    const fanren::core::TileMap& map() const { return *app_.currentMap(); }

    MapObject objectNamed(const std::string& name) const {
        for (const MapObject& object : map().objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    Application app_;
    WorldScene world_;
};

// ---------------------------------------------------------------------------
// 先验：这两个挂点确实在这张图上，而且确实指着这两个脚本
// ---------------------------------------------------------------------------
// 少了这一条，下面每一句都可能是在一个 MapObject{} 空壳上比较空字符串——
// 「找不到某个词就算过」那一类空转法，本项目栽过。
TEST_F(Ch03TriggerMode, BothHooksAreReallyOnThisMapAndPointAtTheTwoScripts) {
    const MapObject duoshe = objectNamed(kDuoshe);
    const MapObject chujue = objectNamed(kChujue);
    ASSERT_FALSE(duoshe.name.empty()) << kMapJusuo << " 上没有 " << kDuoshe;
    ASSERT_FALSE(chujue.name.empty()) << kMapJusuo << " 上没有 " << kChujue;
    EXPECT_EQ(duoshe.type, "trigger");
    EXPECT_EQ(chujue.type, "trigger");
    EXPECT_EQ(duoshe.property("script"), "ch03/duoshe.lua");
    EXPECT_EQ(chujue.property("script"), "ch03/chujue.lua");
    // mode 属性本身必须真的写了。空字符串等于「引擎两条路都不认」，
    // 而空字符串与任何一个错的取值在下面的比较里长得完全不一样——
    // 这一条保证下面那两句比较的是内容，不是缺失。
    EXPECT_FALSE(duoshe.property("mode").empty()) << kDuoshe << " 没写 mode";
    EXPECT_FALSE(chujue.property("mode").empty()) << kChujue << " 没写 mode";
}

// ---------------------------------------------------------------------------
// 正题：两种杀的操作感
// ---------------------------------------------------------------------------

TEST_F(Ch03TriggerMode, TheDispossessionHappensWithoutThePlayerPressingAnything) {
    const MapObject duoshe = objectNamed(kDuoshe);
    ASSERT_FALSE(duoshe.name.empty());
    EXPECT_EQ(duoshe.property("mode"), "enter")
        << "节点 10 夺舍必须是踏入型。这一节从头到尾没有一次 choice()，"
           "失重感一半来自那个，另一半就来自这里：他连「要不要开始」都没得选。"
           "挂成 interact 就等于让玩家自己点开自己被夺舍的那一幕。";
}

TEST_F(Ch03TriggerMode, TheExecutionOnlyStartsWhenThePlayerPressesConfirm) {
    const MapObject chujue = objectNamed(kChujue);
    ASSERT_FALSE(chujue.name.empty());
    EXPECT_EQ(chujue.property("mode"), "interact")
        << "节点 11 处决必须是交互型。ch03.chujue.after 那句「这一回是他自己"
           "走过去、自己叫的名字、自己按下的拇指」指的就是这一下确认键；"
           "挂成 enter 之后玩家走过门洞就自动开演，那一下拇指不是他按的了。";
}

TEST_F(Ch03TriggerMode, TheTwoHooksDoNotShareOneMode) {
    // 上面两条各自写死了取值，这一条只问「它们不一样」。
    // 三条一起，任何一种「两节合成同一种操作感」的改法都至少红一条：
    // 两个都改成 enter 红第二条，两个都改成 interact 红第一条，
    // 对调红前两条，而这一条盯的是**对比本身**——本章要的就是这个对比。
    const MapObject duoshe = objectNamed(kDuoshe);
    const MapObject chujue = objectNamed(kChujue);
    ASSERT_FALSE(duoshe.name.empty());
    ASSERT_FALSE(chujue.name.empty());
    EXPECT_NE(duoshe.property("mode"), chujue.property("mode"))
        << "两种杀挂成了同一种触发方式，本章的道德分量就没了（大纲注解）";
}

// ---------------------------------------------------------------------------
// 引擎语义：enter 与 interact 在引擎里真的是两回事
// ---------------------------------------------------------------------------
// 上面三条读的是数据。数据对而引擎把两种 mode 当成一回事，游戏里照样没有区别，
// 而那三条会全绿——「只钉一个数据点」那一类空转法。所以这里把两条路各跑一遍，
// 两个方向都验：
//   · 踩上 enter 触发那一格 → 起脚本；面朝它按确认 → 不起（引擎的 interact
//     只认 mode=interact）。
//   · 面朝 interact 触发按确认 → 起脚本；踩上那一格 → 不起。
//
// 用的是真图上这两个对象本身，所以这一条同时也是上面三条的落地验证。
// 两个脚本开头都有闸门（ch03.jieyao_xuan / ch03.shihai_done 未置位就说一句
// 就 return），所以这里不必满足前置——判据只问「脚本起没起来」，
// 而 Ch03SliceTests 那条注释早写明了：闸门拦住时脚本照样起来。
//
// 两节各占一条用例，且**一律把「起得来」放在最后一句**：脚本一旦起来，
// 引擎就把世界层整个封掉（tryStep / interact 开头那道防重入闸），
// 后面再验什么都会退化成「因为有脚本在跑所以没反应」的恒真。

TEST_F(Ch03TriggerMode, TheEngineOnlyStartsTheDispossessionByWalkingOntoIt) {
    const MapObject duoshe = objectNamed(kDuoshe);
    ASSERT_FALSE(duoshe.name.empty());
    const Point cell = duoshe.position;
    const Point beside{cell.x - 1, cell.y};
    ASSERT_TRUE(map().walkable(cell)) << "踏入型触发压在走不上去的格子上";
    ASSERT_TRUE(map().walkable(beside)) << "它旁边站不了人，下面两句都做不成";

    // 按：站在它旁边、脸朝它。引擎的 interact 只认 mode=interact，该毫无反应。
    app_.state().position = beside;
    app_.state().facing = 1;   // 右
    EXPECT_FALSE(world_.interact(app_))
        << "夺舍那一处按确认就开演了：它该是走到才发生的，不是玩家点开的";
    EXPECT_FALSE(app_.scripts().isRunning());

    // 踩：从同一格迈一步过去，这一下才该起脚本。这一半是上一句的先验——
    // 少了它，「按了没反应」可能只是因为这个触发器整个坏掉了。
    app_.state().position = beside;
    ASSERT_TRUE(world_.tryStep(app_, 1, 0)) << "走不上夺舍那一格";
    EXPECT_EQ(app_.state().position, cell);
    EXPECT_TRUE(app_.scripts().isRunning()) << "走到床边该发生，玩家一个键也没按";
}

TEST_F(Ch03TriggerMode, TheEngineOnlyStartsTheExecutionWhenConfirmIsPressed) {
    const MapObject chujue = objectNamed(kChujue);
    ASSERT_FALSE(chujue.name.empty());
    const Point cell = chujue.position;
    const Point beside{cell.x + 1, cell.y};
    ASSERT_TRUE(map().walkable(cell)) << "这一格连站都站不上去，谈什么面朝它按";
    ASSERT_TRUE(map().walkable(beside));

    // 踩：走过去，不该有任何事发生。这一句是本章那处对比的机制根据。
    app_.state().position = beside;
    ASSERT_TRUE(world_.tryStep(app_, -1, 0)) << "走不上处决那一格";
    EXPECT_EQ(app_.state().position, cell);
    EXPECT_FALSE(app_.scripts().isRunning())
        << "从处决那一格上走过去就开演了：那一下拇指必须由玩家自己按";

    // 按：站回旁边、脸朝它，这一下才该起脚本。
    app_.state().position = beside;
    app_.state().facing = 3;   // 左
    EXPECT_TRUE(world_.interact(app_)) << "面朝处决那一处按确认，点不着";
    EXPECT_TRUE(app_.scripts().isRunning());
}

}  // namespace
