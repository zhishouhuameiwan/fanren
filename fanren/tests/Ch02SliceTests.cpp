// 第 2 章的自动化通关测试（设计文档 docs/ch02-design.md 第 8 节验收第 2 条，
// 校对报告 docs/ch02-review.md 的 BLOCKER-3）。
//
// 这一条测试驱动的是玩家实际跑的那一整套东西：真的 Application（无头）、真的
// maps/ch02_*.tmj、真的 WorldScene 走位规则、真的 scripts/ch02/*.lua、真的灵田
// 与修炼面板。从进谷第一步走到章末那句「这事不能让第二个人知道」，中间六道
// 闸门一道不落，每一道都验两次——条件不满足时确实拦得住，条件满足后确实
// 开得了。
//
// 「六道」的口径：docs/ch02-design.md 第 8 节写的是五道，那是 2026-09-20 给
// renqing.lua 补上「口诀要一层一层上去」那道闸之前写的。以脚本为准，文档随后跟进。
//
// 为什么非得这样跑不可：第 1 章在这类问题上栽过两次（旗标清不掉导致情感转折
// 全路径不可达、小游戏某结局 0/81 不可达），两次都是靠人工点过、都没拦住。
// 本章六道闸门里任何一道写成不可满足，整章就通不过去，而这件事在单测里是看
// 不出来的：每个脚本自己都对，错的是拼起来之后。校对报告的原话是前两条
// BLOCKER（take() 丢年份、绿液不消耗）「各只要一行断言就会当场红」。
//
// 与邻居的分工：
//   * tests/SliceTests.cpp        —— 第 1 章的 headless 驱动范例，走位与触发的路子照它写。
//   * tests/Ch02BottleTests.cpp   —— 段五那三场戏的端到端，钉的是绿液与年份的账。
//   * tests/ScriptApiTests.cpp    —— 脚本 API 每一条的语义与失败路径。
//   * 本文件                       —— 整章一趟走通，钉的是六道闸门与章末的资产状态。
//
// ---------------------------------------------------------------------------
// 一条走位规则，写在这里免得后来人重新踩一遍
// ---------------------------------------------------------------------------
// WorldScene::interact 看的是「面朝的前一格」，而 tryStep 先转向再迈步：迈得动
// 就走过去（这时前一格是再往前那一格），迈不动才只转向。于是要站在 S 上脸朝着
// 相邻的 F，只有两条路：
//   1. F 进不去（墙 / NPC / 设施占格）—— 站到 S 上，朝 F 按一下方向键，人不动、
//      脸转过去。tests/SliceTests.cpp 撞 NPC 那一段用的就是这条。
//   2. F 进得去 —— 那就得是「从 F 的另一侧那一格出发，朝 F 的方向迈一步落到 S」，
//      落地时朝向正是那一步的方向，前一格于是正好是 F。
// 本章的 interact 触发器两种都有（trigger_zhitong 在不可通行的石碾上，
// trigger_maiyao 在可通行的柜台前），faceCell 把两条都实现了。
#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Bottle.h"
#include "core/rules/Calendar.h"
#include "core/rules/Field.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/Scene.h"
#include "game/WorldScene.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::Application;
using fanren::game::CultivationScene;
using fanren::game::FieldScene;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// ---- 本章用到的 id。契约见 docs/interfaces-p3-script.md 第 4 节 ----
constexpr const char* kFieldId = "shenshougu_yaopu";
constexpr const char* kHerbId = "herb_huangjing_cao";
constexpr const char* kHerb2Id = "herb_zishen_cao";
constexpr const char* kPillId = "pill_zhitong_san";
constexpr const char* kMoneyId = "material_lingshi";

constexpr const char* kMapYaopu = "ch02_yaopu";
constexpr const char* kMapJusuo = "ch02_jusuo";
constexpr const char* kMapYabi = "ch02_yabi";
constexpr const char* kMapWairentang = "ch02_wairentang";

// ---- 三次交易的价钱。照 rules::herbPrice 实算（黄精基价 5，(age+10)^2/100 倍），
// 与 docs/ch02-design.md 第 3 节那张表逐个对上：1 年 6、11 年 22、44 年 145。
constexpr int kPriceOneYear = 6;
constexpr int kPriceElevenYear = 22;
constexpr int kPriceFortyFourYear = 145;

// 走到章末那个二选一的那一刻，钱袋里该有的数。
// 三笔进账：段一卖一年份那株 6，章末卖十一年那株 22、卖四十四年那株 145。
constexpr int kGrossIncome = kPriceOneYear + kPriceElevenYear + kPriceFortyFourYear;

// ---- 章末二选一的结算。
// suanzhang.lua 按玩家手头实际有多少钱算，不写死数目——玩家自己多种多卖几株
// 再回来，数就不是下面这些了。所以真正的判据是相对量（「钱确实少了」「种苗
// 确实多了」），下面这三个绝对值只在「走主线最小路径」这个前提下成立，
// 拿来钉的是那条按比例结算的规则本身。
constexpr int kSeedlingBuyPrice = 6;   // 收药处买价：基价 5 × buyRate 1.2
// 捎回家：扣掉三分之二，留三分之一给他自己周转。
constexpr int kPurseAfterSendingHome = kGrossIncome - kGrossIncome * 2 / 3;
// 全换成药材：尽量多买，除不尽的零头留在身上。
constexpr int kSeedlingsBought = kGrossIncome / kSeedlingBuyPrice;
constexpr int kPurseAfterBuyingSeed = kGrossIncome - kSeedlingsBought * kSeedlingBuyPrice;

// ---- 本章的日历。每一条都取自对应脚本里那句 advance_days（或闸门要求的天数），
// 合起来就是「四年真的过去了」在账面上的全部证据。少掉任何一条都会在章末的
// 天数断言上当场红。
constexpr int kGateOneMeditateDays = 30;   // caiyao.lua：today() - diyike_day >= 30
constexpr int kMaiyaoDays = 180;           // maiyao.lua   段一末
constexpr int kKaigaiDays = 7;             // kaigai.lua   第八日开盖
constexpr int kCangpingDays = 180;         // cangping.lua 段二末
constexpr int kZhitongNightDays = 1;       // zhitong.lua  碾了一夜
constexpr int kZhitongYearDays = 365;      // zhitong.lua  段三末
constexpr int kRenqingYearDays = 365;      // renqing.lua  段四末
constexpr int kShiyaoNightDays = 1;        // shiyao.lua   那一夜过去，变化才看得见
constexpr int kCuanmanWaitDays = 21;       // cuishu.lua   第二场：攒满一瓶

constexpr int kChapterDays = kGateOneMeditateDays + kMaiyaoDays + kKaigaiDays + kCangpingDays +
                             kZhitongNightDays + kZhitongYearDays + kRenqingYearDays +
                             kShiyaoNightDays + kCuanmanWaitDays;

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

constexpr double kFrame = 1.0 / 60.0;
// 死循环闸。整章最长的一场是 chousui.lua（二十余句），远够用；脚本卡住时
// 宁可测试失败，也不要挂住整个 ctest。
constexpr int kMaxScriptFrames = 4000;

// 仓库根：测试可能从 build/ 或工程根启动。判据用本章自己的脚本与地图，
// 找错根目录时报的是「找不到 ch02 的东西」，而不是一串莫名其妙的空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch02" / "cuishu.lua") &&
            fs::exists(root / "maps" / "ch02_yaopu.tmj")) {
            return candidate;
        }
    }
    return ".";
}

int facingOf(Point direction) {
    if (direction.x > 0) return 1;
    if (direction.x < 0) return 3;
    if (direction.y < 0) return 0;
    return 2;
}

// ---------------------------------------------------------------------------
// 章末的资产判据
// ---------------------------------------------------------------------------
// 做成一个具名判据而不是就地摊一串 EXPECT，是为了能拿同一份判据去跑一条
// 「故意摆错 → 判据确实说不」的自检（见本文件末尾那两条 Ch02EndState 用例）。
//
// 「背包里找不到十一年 / 四十四年的黄精」这种断言有个天然的空转法：背包整个
// 是空的时候它照样通过。所以这里把种苗的数目一并断上——那是前一条断言的先验
// 判据，少了它，一条把背包清空的缺陷会让「卖掉了」这一组断言集体变成装饰。
struct BagVerdict {
    bool ok = true;
    std::string why;
};

BagVerdict chapterEndBag(const GameState& state, int seedlings, int zishen, int purse) {
    const auto fail = [](std::string why) { return BagVerdict{false, std::move(why)}; };

    // 先查这两条：章末那两笔买卖扣的必须正是台词里说的那两株。
    if (state.itemCountOfAge(kHerbId, 11) != 0) {
        return fail("十一年那株还在背包里——章末的算账收了钱却没收药，拿去商店还能再卖一次");
    }
    if (state.itemCountOfAge(kHerbId, 44) != 0) {
        return fail("四十四年那株还在背包里——同上，而它一株就值 " +
                    std::to_string(kPriceFortyFourYear));
    }
    // 这两条是上面那两条的先验判据，缺了它们上面就是两条空转的断言：
    // 背包整个被清空时，「找不到十一年那株」照样成立。
    if (state.itemCount(kHerbId) != seedlings) {
        return fail("背包里的黄精共 " + std::to_string(state.itemCount(kHerbId)) + " 株，该是 " +
                    std::to_string(seedlings) + " 株（章末卖掉两株之后剩下的种苗）");
    }
    if (state.itemCountOfAge(kHerbId, 0) != seedlings) {
        return fail("剩下的该全是零年份的种苗，实为 " +
                    std::to_string(state.itemCountOfAge(kHerbId, 0)) + " 株");
    }
    if (state.itemCount(kHerb2Id) != zishen) {
        return fail("紫参草该有 " + std::to_string(zishen) + " 株，实为 " +
                    std::to_string(state.itemCount(kHerb2Id)) + " 株");
    }
    if (state.itemCount(kPillId) != 0) {
        return fail("止痛散该交出去了，背包里不该还留着");
    }
    // 钱袋。三笔进账（6 + 22 + 145 = kGrossIncome）之后，章末那个二选一还要
    // 按手头实际有多少钱花掉一部分，所以期望值由调用方按分支给。
    if (state.itemCount(kMoneyId) != purse) {
        return fail("钱袋该剩 " + std::to_string(purse) + "（三笔进账 " +
                    std::to_string(kGrossIncome) + " 减去章末那个二选一花掉的），实为 " +
                    std::to_string(state.itemCount(kMoneyId)));
    }
    if (state.itemCount(kMoneyId) >= kGrossIncome) {
        return fail("走完章末的二选一钱袋一分没少——那两句台词（捎了一大半回去 / "
                    "全换成了药材）说的事情在引擎里没发生");
    }
    if (state.itemCount(kMoneyId) <= 0) {
        return fail("章末把他掏空了：两条分支都刻意留了周转的余地，"
                    "全扣光会让第 3 章开局身无分文");
    }
    return BagVerdict{};
}

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch02Walkthrough : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 本章开头：第 1 章的神手谷把玩家送进药圃，落点就是这个 spawn。
        auto loaded = app_.loadMap(kMapYaopu, "spawn_from_shenshougu");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        startDay_ = state().day;
    }

    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }
    int flag(const std::string& name) { return state().flag(name); }

    // ---- 脚本 ----

    // 像主循环那样把脚本推到结束，替玩家按确认。
    // choiceIndex 一律用同一个：本章六次选择只改口吻不改走向（校对报告第四节
    // 4.1：972 / 972 全部走到 ch02.done），所以取哪一项都到得了章末——而这件事
    // 本身要验，见 EverySecondChoiceAlsoReachesTheEnd。
    void pumpScripts(int choiceIndex) {
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = choiceIndex;
                app_.completeCommand(result);
                // 对话框是脚本压进来的，演完就收掉。不收的话整章两百多句话会
                // 在场景栈上摞成两百多层，后面拿 topScene() 验「被拦下时有没有
                // 说明」就全是噪声。
                app_.popScene();
            }
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        // 最后一句话的对话框还挂在栈上，收场时的那次 popScene 也还压着没生效。
        // 多推一帧把两者一起结清，场景栈回到空——后面拿 topScene() 验「被拦下时
        // 有没有说明」才不会读到上一场戏的残留。
        app_.tick(kFrame);
    }

    // ---- 地图与走位 ----

    const TileMap& map() const { return *app_.currentMap(); }

    // 按名字取地图对象。返回副本而不是指针：换一次图 TileMap 就整个重建，
    // 留着的指针会悄悄变成悬空的（SliceTests 第一版正是栽在这里）。
    MapObject objectNamed(const std::string& name) const {
        for (const MapObject& object : map().objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    // 这一格能不能进：WorldScene::tryStep 的真实规则。
    //
    // NPC 那一道走 visibleNpcAt 而不是 objectAt：不在场的 NPC 不占格
    // （2026-09-21 起，见 WorldScene::visibleNpcAt）。这个辅助函数自称照搬引擎，
    // 那它就必须真的照搬 —— 比引擎严一格，这里算出来的「走不到」会是假的，
    // 而红灯出在测试自己身上，最难查。
    //
    // 不再是 const 成员：visibleNpcAt 要读存档里的旗标，而 Application::state()
    // 只有非 const 的重载。调用它的那几个（routeTo / walkTo / faceCell …）本来
    // 就都是非 const 的，跟着去掉即可。
    bool enterable(Point p) {
        if (!map().walkable(p)) return false;
        if (WorldScene::visibleNpcAt(app_.state(), map(), p) != nullptr) return false;
        if (map().objectAt(p, "facility") != nullptr) return false;
        return true;
    }

    // 这一格能不能当落脚点 / 中途格。传送点另算：踩上去就换图了，
    // 不能拿它当路过的格子。
    bool standable(Point p) {
        return enterable(p) && map().objectAt(p, "portal") == nullptr;
    }

    // 迈一步，并把可能被踩发的踏入型触发演完。
    bool step(Point direction, int choiceIndex) {
        const bool moved = world_.tryStep(app_, direction.x, direction.y);
        if (app_.scripts().isRunning()) pumpScripts(choiceIndex);
        return moved;
    }

    // 从当前位置到 goal 的一条路。用 BFS 而不是写死路线：地图一改，写死的
    // 路线会悄悄失效，而 BFS 会如实报告「走不到」。
    std::vector<Point> routeTo(Point goal) {
        const int width = map().width;
        const auto index = [width](Point p) { return p.y * width + p.x; };
        const Point from = app_.state().position;
        if (from == goal) return {};

        std::map<int, int> previous;
        previous[index(from)] = index(from);
        std::deque<Point> queue{from};
        bool found = false;
        while (!queue.empty() && !found) {
            const Point current = queue.front();
            queue.pop_front();
            for (const Point& direction : kDirections) {
                const Point next{current.x + direction.x, current.y + direction.y};
                if (previous.count(index(next))) continue;
                if (next != goal && !standable(next)) continue;
                if (next == goal && !enterable(next)) continue;
                previous[index(next)] = index(current);
                if (next == goal) {
                    found = true;
                    break;
                }
                queue.push_back(next);
            }
        }
        if (!found) return {};

        std::vector<Point> path;
        int node = index(goal);
        while (node != index(from)) {
            path.push_back(Point{node % width, node / width});
            node = previous[node];
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

    bool walkTo(Point goal, int choiceIndex = 0) {
        if (app_.state().position == goal) return true;
        const std::vector<Point> path = routeTo(goal);
        if (path.empty()) return false;
        const std::string mapBefore = app_.state().mapId;
        for (const Point& cell : path) {
            const Point here = app_.state().position;
            if (!step(Point{cell.x - here.x, cell.y - here.y}, choiceIndex)) return false;
            if (app_.state().mapId != mapBefore) return false;   // 半路换图了
        }
        return app_.state().position == goal;
    }

    // 站到 target 的旁边、脸朝着它。两条路见文件头那段注释。
    bool faceCell(Point target, int choiceIndex) {
        for (const Point& direction : kDirections) {
            const Point stand{target.x - direction.x, target.y - direction.y};
            if (!standable(stand)) continue;

            if (!enterable(target)) {
                // target 进不去：站过去，朝它按一下方向键，人不动、脸转过去。
                if (!walkTo(stand, choiceIndex)) continue;
                step(direction, choiceIndex);
            } else {
                // target 进得去：只能从它另一侧那一格朝它迈一步落到 stand 上。
                const Point approach{stand.x - direction.x, stand.y - direction.y};
                if (!standable(approach)) continue;
                if (!walkTo(approach, choiceIndex)) continue;
                if (!step(direction, choiceIndex)) continue;
            }
            if (app_.state().position == stand && app_.state().facing == facingOf(direction)) {
                return true;
            }
        }
        return false;
    }

    // 站到某个地图对象跟前。对象可能占好几格，逐格试。
    bool faceObject(const MapObject& object, int choiceIndex) {
        for (int dy = 0; dy < std::max(1, object.height); ++dy) {
            for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                if (faceCell(Point{object.position.x + dx, object.position.y + dy}, choiceIndex)) {
                    return true;
                }
            }
        }
        return false;
    }

    // 踏上某个对象占的格子。踏入型触发与传送点都靠这一条。
    bool stepOnto(const MapObject& object, int choiceIndex) {
        for (int dy = 0; dy < std::max(1, object.height); ++dy) {
            for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                const Point target{object.position.x + dx, object.position.y + dy};
                if (!map().walkable(target)) continue;
                for (const Point& direction : kDirections) {
                    const Point stand{target.x - direction.x, target.y - direction.y};
                    if (!standable(stand)) continue;
                    if (!walkTo(stand, choiceIndex)) continue;
                    step(direction, choiceIndex);
                    return true;
                }
            }
        }
        return false;
    }

    // ---- 三个动作 ----

    // 点着一处 interact 触发器，并把它演完。
    //
    // 返回值是「脚本真的起来了」，而不是「旗标变了没有」——闸门拦住时脚本照样
    // 起来（说一句话就 return），与「压根没点着」长得一模一样。验闸门拦得住的
    // 那几条断言全靠这个区分，没有它，一个走不到跟前的 bug 会让所有「拦住了」
    // 的断言集体变成装饰。
    bool fireTrigger(const std::string& name, int choiceIndex = 0) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            ADD_FAILURE() << app_.state().mapId << " 上没有 " << name;
            return false;
        }
        if (!faceObject(trigger, choiceIndex)) {
            ADD_FAILURE() << "走不到 " << name << " 跟前";
            return false;
        }
        if (!world_.interact(app_)) {
            ADD_FAILURE() << name << " 点不着";
            return false;
        }
        if (!app_.scripts().isRunning()) {
            ADD_FAILURE() << name << " 点着的不是脚本";
            return false;
        }
        pumpScripts(choiceIndex);
        return true;
    }

    // 踩上踏入型触发。
    bool enterTrigger(const std::string& name, int choiceIndex = 0) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            ADD_FAILURE() << app_.state().mapId << " 上没有 " << name;
            return false;
        }
        return stepOnto(trigger, choiceIndex);
    }

    // 踩上传送点。返回「真的换图了」——闸门未满足时引擎只弹一句提示，人还站在
    // 原地，正是这个返回值把「过去了」与「被拦下了」分开。
    bool usePortal(const std::string& name, int choiceIndex = 0) {
        const MapObject portal = objectNamed(name);
        if (portal.name.empty()) {
            ADD_FAILURE() << app_.state().mapId << " 上没有 " << name;
            return false;
        }
        const std::string before = app_.state().mapId;
        if (!stepOnto(portal, choiceIndex)) {
            ADD_FAILURE() << "走不到 " << name << " 跟前";
            return false;
        }
        return app_.state().mapId != before;
    }

    // 打开面朝的设施面板，返回压进来的那个场景。
    fanren::game::Scene* openFacility(const std::string& name, int choiceIndex = 0) {
        const MapObject facility = objectNamed(name);
        if (facility.name.empty()) {
            ADD_FAILURE() << app_.state().mapId << " 上没有 " << name;
            return nullptr;
        }
        if (!faceObject(facility, choiceIndex)) {
            ADD_FAILURE() << "走不到 " << name << " 跟前";
            return nullptr;
        }
        if (!world_.interact(app_)) {
            ADD_FAILURE() << name << " 点不着";
            return nullptr;
        }
        app_.tick(kFrame);   // 场景是帧末才真正入栈的
        return app_.topScene();
    }

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    // 在药圃的灵田里种下一株。走的是玩家真会走的那条路：走到田边、开面板、
    // 挑畦、挑种。
    bool plantOneHerb() {
        auto* field = dynamic_cast<FieldScene*>(openFacility("facility_lingtian"));
        if (field == nullptr) {
            ADD_FAILURE() << "灵田面板没开起来";
            return false;
        }
        const bool planted = field->plantAt(app_, 0, kHerbId);
        EXPECT_TRUE(planted) << field->feedback();
        closePanel();
        return planted;
    }

    // 在居所打坐。days 为 0 表示只开一下面板——那也不是白开：面板一开就把欠的
    // 日课结清（CultivationScene.h 里那段「四年苦修不能只认按钮次数」），
    // 剧情整年整年推过去的日子于是才进得了修为的账。
    void meditate(int days) {
        auto* panel = dynamic_cast<CultivationScene*>(openFacility("facility_dazuo"));
        ASSERT_NE(panel, nullptr) << "修炼面板没开起来";
        if (days > 0) panel->meditateFor(app_, days);
        // 够得着就冲一关。玩家站在面板前不会放着不点。
        for (int guard = 0; guard < 16; ++guard) {
            const auto items = CultivationScene::buildMainItems(state());
            if (items.size() < 2 || !items[1].enabled) break;
            panel->breakthrough(app_);
        }
        closePanel();
    }

    Application app_;
    WorldScene world_;
    int startDay_ = 1;
};

// ---------------------------------------------------------------------------
// 通关：一趟走到底，六道闸门逐一验两次
// ---------------------------------------------------------------------------
TEST_F(Ch02Walkthrough, WalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    GameState& s = state();

    // ======================= 段一 · 入谷首年·春 =======================
    ASSERT_TRUE(enterTrigger("trigger_renyao")) << "进药圃第一步就该撞上入谷第一课";
    EXPECT_EQ(flag("ch02.renyao_done"), 1);
    EXPECT_EQ(flag("ch02.diyike_day"), startDay_) << "三十日的闸门从这一天起算";
    ASSERT_NE(s.findField(kFieldId), nullptr) << "第一课该把药圃那块田交给玩家";
    EXPECT_EQ(s.findField(kFieldId)->slots.size(), 8u) << "契约说八槽";
    EXPECT_EQ(s.itemCount(kHerbId), 3) << "管事发的三株种苗";

    // 防跳过（设计第 8 节验收第 5 条）：没打过坐就去外刃堂，门是关着的。
    EXPECT_FALSE(usePortal("portal_to_wairentang")) << "ch02.dazuo_done 未置位，这道门不该放行";
    EXPECT_EQ(s.mapId, kMapYaopu);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
    closePanel();

    ASSERT_TRUE(usePortal("portal_to_jusuo")) << "ch02.renyao_done 已置位，居所该进得去";
    ASSERT_EQ(s.mapId, kMapJusuo);
    ASSERT_TRUE(fireTrigger("trigger_dazuo"));
    EXPECT_EQ(flag("ch02.dazuo_done"), 1);
    meditate(0);   // 第一次开面板：日课的水位从今天起算
    ASSERT_TRUE(usePortal("portal_to_yaopu"));
    ASSERT_EQ(s.mapId, kMapYaopu);

    // 防跳过：段二还没开始，崖壁那条山道也是关着的。
    EXPECT_FALSE(usePortal("portal_to_yabi")) << "ch02.duan2_start 未置位，崖壁不该放行";
    EXPECT_EQ(s.mapId, kMapYaopu);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
    closePanel();

    // ---- 闸门一（段一 → 段二）：三个条件，逐条验它拦得住 ----
    // 条件 1 已满足（dazuo_done），条件 2「种下至少一株」还没有。
    ASSERT_TRUE(fireTrigger("trigger_caiyao"));
    EXPECT_EQ(flag("ch02.caiyao_done"), 0) << "一株都没种就来采药，闸门该拦住";

    ASSERT_TRUE(plantOneHerb());
    EXPECT_EQ(s.itemCount(kHerbId), 2) << "播种要消耗一株";

    // 条件 3「入谷第一课起满三十日」还没有。
    ASSERT_TRUE(fireTrigger("trigger_caiyao"));
    EXPECT_EQ(flag("ch02.caiyao_done"), 0) << "种是种下了，日子还没熬够，闸门该拦住";

    // 顺手验段一末那道：caiyao_done 未置位时卖药也不该开始。
    ASSERT_TRUE(usePortal("portal_to_wairentang")) << "dazuo_done 已置位，外刃堂该进得去";
    ASSERT_EQ(s.mapId, kMapWairentang);
    ASSERT_TRUE(fireTrigger("trigger_maiyao"));
    EXPECT_EQ(flag("ch02.duan2_start"), 0) << "还没采到药就来卖，段一不该结束";
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // 三十日只能靠打坐推——修炼面板每坐一回都推日历，「熬过了一段日子」是真熬的。
    ASSERT_TRUE(usePortal("portal_to_jusuo"));
    const int dayBeforeSitting = s.day;
    meditate(kGateOneMeditateDays);
    EXPECT_EQ(s.day - dayBeforeSitting, kGateOneMeditateDays) << "打坐必须真的推日历";
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // ---- 闸门一：三条齐了，这回该放行 ----
    ASSERT_TRUE(fireTrigger("trigger_caiyao"));
    EXPECT_EQ(flag("ch02.caiyao_done"), 1) << "种下了、也熬够了三十日，闸门该开";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 1) << "采下的是一年份那株，段五算账的对照组";

    // 段一末：第一次卖药，六块。
    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    const int dayBeforeMaiyao = s.day;
    ASSERT_TRUE(fireTrigger("trigger_maiyao"));
    EXPECT_EQ(flag("ch02.duan2_start"), 1) << "段一该结束了";
    EXPECT_EQ(s.itemCount(kMoneyId), kPriceOneYear) << "一年份的药只卖得出六块——后面那两笔的对照组";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 1), 0) << "卖出去的该是那株一年份的";
    EXPECT_EQ(s.day - dayBeforeMaiyao, kMaiyaoDays) << "段一末推半年";

    // ======================= 段二 · 第一年·秋 =======================
    ASSERT_TRUE(usePortal("portal_to_yaopu"));
    ASSERT_TRUE(usePortal("portal_to_yabi")) << "duan2_start 已置位，崖壁该放行了";
    ASSERT_EQ(s.mapId, kMapYabi);

    const int dayAtPickup = s.day;
    ASSERT_TRUE(enterTrigger("trigger_shiping")) << "山道半途该踢到那只瓶子";
    EXPECT_EQ(flag("ch02.shiping_done"), 1);
    EXPECT_TRUE(s.bottle.owned) << "瓶子到手";
    EXPECT_EQ(s.bottle.drops, 0) << "拾瓶当场不该有绿液——第八日才有";
    EXPECT_EQ(s.bottle.lastChargeDay, dayAtPickup) << "凝液的计时必须从拾瓶当日起算";

    ASSERT_TRUE(usePortal("portal_to_yaopu"));
    ASSERT_TRUE(usePortal("portal_to_jusuo"));

    // ---- 闸门二（段二 → 段三）：先验它拦得住 ----
    ASSERT_TRUE(fireTrigger("trigger_kaigai"));
    EXPECT_EQ(flag("ch02.kaigai_done"), 0) << "还没砸过泡过就等开盖，该拦住";

    ASSERT_TRUE(fireTrigger("trigger_zaping"));
    EXPECT_EQ(flag("ch02.zaping_done"), 1);

    ASSERT_TRUE(fireTrigger("trigger_cangping"));
    EXPECT_EQ(flag("ch02.duan3_start"), 0) << "盖还没开就谈绿液，该拦住";

    // ---- 闸门二：条件满足，开盖 ----
    const int dayBeforeKaigai = s.day;
    ASSERT_TRUE(fireTrigger("trigger_kaigai"));
    EXPECT_EQ(flag("ch02.kaigai_done"), 1) << "砸过泡过了，该轮到第八日";
    EXPECT_EQ(s.day - dayBeforeKaigai, kKaigaiDays) << "补的正好是那七天";
    EXPECT_EQ(s.day - dayAtPickup, fanren::rules::kBaseChargeDays)
        << "拾瓶当日起算、第八天得第一滴，这一条是原著硬约束";
    EXPECT_EQ(s.bottle.drops, 1) << "开盖时瓶里正好一滴";

    const int dayBeforeCangping = s.day;
    ASSERT_TRUE(fireTrigger("trigger_cangping"));
    EXPECT_EQ(flag("ch02.duan3_start"), 1) << "段二该结束了";
    EXPECT_EQ(s.day - dayBeforeCangping, kCangpingDays) << "段二末推半年";
    EXPECT_EQ(s.bottle.drops, s.bottle.capacity) << "半年下来瓶子早满了";
    EXPECT_EQ(s.bottle.capacity, 3) << "炼气期的容量是三滴";

    // ======================= 段三 · 第二年 =======================
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // ---- 闸门三（段三 → 段四）：还没见过抽髓丸之祸，配不了药 ----
    ASSERT_TRUE(fireTrigger("trigger_zhitong"));
    EXPECT_EQ(flag("ch02.renqing_jiexia"), 0) << "人都还没遇上，人情从何结起";

    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    ASSERT_TRUE(enterTrigger("trigger_chousui")) << "后院那一场该在走进去时发生";
    EXPECT_EQ(flag("ch02.chousui_jian"), 1);

    // ---- 闸门四（段四 → 段五）：段四还没开始，人情也就还不到还的时候 ----
    ASSERT_TRUE(fireTrigger("trigger_renqing"));
    EXPECT_EQ(flag("ch02.duan5_start"), 0) << "药都还没配出来，厉飞雨不该来还人情";

    // ---- 闸门三：见过了，手上也有药材，该过得去 ----
    ASSERT_TRUE(usePortal("portal_to_yaopu"));
    EXPECT_GE(s.itemCount(kHerbId), 1) << "配药要药材，这是闸门三的另一半条件";
    const int dayBeforeZhitong = s.day;
    ASSERT_TRUE(fireTrigger("trigger_zhitong"));
    EXPECT_EQ(flag("ch02.renqing_jiexia"), 1) << "人情结下了";
    EXPECT_EQ(flag("ch02.duan4_start"), 1) << "段三该结束了";
    EXPECT_EQ(flag("ch02.zhitong_fangzi"), 1) << "取第一项，配的是重剂麻沸";
    EXPECT_EQ(s.itemCount(kPillId), 0) << "药配出来就交出去了，不该留在背包里";
    EXPECT_EQ(s.day - dayBeforeZhitong, kZhitongNightDays + kZhitongYearDays)
        << "碾了一夜，再跨一年";

    // ======================= 段四 · 第三年 =======================
    // ---- 闸门五（口诀一层一层上去）：段四开始了，但屋里那一节还没补 ----
    // 这道闸挡的是「整个段四只跑外刃堂、不回屋里坐」——那样 koujue_ceng 会从
    // 第一层直接跳到第三层，四年苦修的骨架断掉一节（校对报告 MEDIUM-3）。
    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    EXPECT_LT(flag("ch02.koujue_ceng"), 2)
        << "段四刚开始，屋里那一节还没补——本章第一次动这个旗标的正是 ceng2";
    ASSERT_TRUE(fireTrigger("trigger_renqing"));
    EXPECT_EQ(flag("ch02.duan5_start"), 0) << "口诀还没上第二层，段四不该就这么过去";
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    ASSERT_TRUE(usePortal("portal_to_jusuo"));
    ASSERT_TRUE(fireTrigger("trigger_ceng2"));
    EXPECT_EQ(flag("ch02.koujue_ceng"), 2) << "口诀第二层落在段四";
    meditate(0);
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // ---- 闸门四与闸门五：段四开始了、屋里那一节也补上了，人情该还得成 ----
    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    const int dayBeforeRenqing = s.day;
    const int herbsBeforeRenqing = s.itemCount(kHerbId);
    ASSERT_TRUE(fireTrigger("trigger_renqing"));
    EXPECT_EQ(flag("ch02.duan5_start"), 1) << "段四该结束了";
    EXPECT_EQ(flag("ch02.renqing_huan"), 1) << "取第一项，那箱药材收下了";
    EXPECT_EQ(s.itemCount(kHerbId) - herbsBeforeRenqing, 6) << "一箱黄精";
    EXPECT_EQ(s.itemCount(kHerb2Id), 2) << "外加两株紫参草";
    EXPECT_EQ(s.day - dayBeforeRenqing, kRenqingYearDays) << "段四末再跨一年";

    // ======================= 段五 · 第四年 =======================
    ASSERT_TRUE(usePortal("portal_to_yaopu"));
    ASSERT_TRUE(usePortal("portal_to_jusuo"));
    ASSERT_TRUE(fireTrigger("trigger_ceng3"));
    EXPECT_EQ(flag("ch02.koujue_ceng"), 3) << "四年苦修的终点：口诀第三层";
    EXPECT_EQ(flag("ch02.xiangqi_ping"), 1) << "他重新想起了那只瓶子";
    meditate(0);
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // ---- 闸门六（段五之内）：试药还没发生，催熟无从谈起 ----
    ASSERT_TRUE(fireTrigger("trigger_cuishu"));
    EXPECT_EQ(flag("ch02.cuishou_unlocked"), 0) << "兔子还没死，东头那畦不该有任何变化";

    const int dropsBeforeShiyao = s.bottle.drops;
    ASSERT_EQ(dropsBeforeShiyao, 3) << "段五开场该是满瓶，否则下面的差值没有意义";
    const int dayBeforeShiyao = s.day;
    ASSERT_TRUE(fireTrigger("trigger_shiyao"));
    EXPECT_EQ(flag("ch02.shiyao_done"), 1);
    EXPECT_EQ(s.bottle.drops, dropsBeforeShiyao - 1)
        << "倒进碗里的那一滴必须真的从瓶子里没了（BLOCKER-2）";
    EXPECT_FALSE(s.bottle.matureKnown) << "试药不该顺手解锁催熟，那是明早的事";
    EXPECT_EQ(s.day - dayBeforeShiyao, kShiyaoNightDays) << "这一夜要真的过去";

    // 第一场：发现催熟。
    ASSERT_TRUE(fireTrigger("trigger_cuishu"));
    EXPECT_EQ(flag("ch02.cuishou_unlocked"), 1) << "闸门六该开了";
    EXPECT_TRUE(s.bottle.matureKnown);
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 11), 1) << "被泼到的那几株里最壮的一株";
    EXPECT_EQ(s.bottle.drops, 2) << "这一场不该再扣绿液：那一滴昨天就倒进碗里了";
    EXPECT_EQ(flag("ch02.cuanman_done"), 0);

    // 章末那道：还没攒满一瓶浇过，算账的戏不该开演。
    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    ASSERT_TRUE(fireTrigger("trigger_suanzhang"));
    EXPECT_EQ(flag("ch02.done"), 0) << "四十四年那株都还没有，算什么账";
    ASSERT_TRUE(usePortal("portal_to_yaopu"));

    // 第二场：攒满一瓶浇同一株。1 → 11 → 22 → 44。
    const int dayBeforeCuanman = s.day;
    ASSERT_TRUE(fireTrigger("trigger_cuishu"));
    EXPECT_EQ(flag("ch02.cuanman_done"), 1);
    EXPECT_EQ(s.day - dayBeforeCuanman, kCuanmanWaitDays) << "「他数着日子等瓶里凝满」那二十来天";
    EXPECT_EQ(s.bottle.drops, 0) << "三滴全浇出去了，瓶子该是空的";
    const fanren::core::Item* herb = app_.data().findItem(kHerbId);
    ASSERT_NE(herb, nullptr);
    EXPECT_EQ(herb->maxAge, 44) << "一阶药的年份上限，实算表 44 年 / 145 灵石 / 24 倍全钉在它上面";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, herb->maxAge), 1)
        << "年份该正好落在这一味药自己的上限上，不是脚本写死的数";

    // ======================= 章末 =======================
    ASSERT_TRUE(usePortal("portal_to_wairentang"));
    const int seedlingsBeforeReckoning = s.itemCountOfAge(kHerbId, 0);
    ASSERT_TRUE(fireTrigger("trigger_suanzhang"));
    EXPECT_EQ(flag("ch02.done"), 1) << "章末：确实抵达终点";
    EXPECT_EQ(flag("ch02.qian_quxiang"), 1) << "取第一项，钱捎回家";

    // ---- 章末的机制状态 ----
    // 只验剧情旗标是不够的：BLOCKER-1 与 BLOCKER-2 藏的正是这一组。
    const BagVerdict bag = chapterEndBag(s, /*seedlings=*/7, /*zishen=*/2,
                                         /*purse=*/kPurseAfterSendingHome);
    EXPECT_TRUE(bag.ok) << bag.why;

    // 章末二选一必须真的动账。这一条从前是假的：脚本只置一个旗标，钱一分没动、
    // 药材一件没给，两句台词说的事情一件也没发生——正是本章校对认定的要害类别
    // （台词断言了某个状态，而引擎里不是那样）。
    // 「捎回家」这一条不买东西，所以种苗的数目不该变；变了说明两条分支串了。
    EXPECT_LT(s.itemCount(kMoneyId), kGrossIncome)
        << "台词说捎了一大半回去，钱袋却一分没少";
    EXPECT_GT(s.itemCount(kMoneyId), 0) << "也不该把他掏空，第 3 章开局还要周转";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0), seedlingsBeforeReckoning)
        << "捎回家这一条不买药材，种苗的数目不该变";

    EXPECT_TRUE(s.bottle.owned);
    EXPECT_TRUE(s.bottle.matureKnown) << "两个解锁节点都过了";
    EXPECT_LT(s.bottle.drops, s.bottle.capacity) << "走完一整章绿液还是满的，说明一滴也没被用掉";
    EXPECT_EQ(s.bottle.drops, 0);

    const fanren::rules::SpiritField* field = s.findField(kFieldId);
    ASSERT_NE(field, nullptr);
    EXPECT_EQ(field->slots.size(), 8u);
    int occupied = 0;
    for (const fanren::rules::FieldSlot& slot : field->slots) {
        if (!slot.seedId.empty()) ++occupied;
    }
    EXPECT_EQ(occupied, 1) << "段一种下的那一株一直没收";
    EXPECT_EQ(field->slots[0].seedId, kHerbId);
    EXPECT_TRUE(field->slots[0].ripe) << "地里那株熬了三年多，早该足年了";
    EXPECT_GE(field->slots[0].age, 3) << "跳时必须连灵田一起结算——四年真的过去了，最实在的证据";
    EXPECT_LE(field->slots[0].age, herb->maxAge) << "自然生长同样受这一味药自己的上限封顶";

    // 剧情说到第三层，引擎的账上也必须正好是第三层。
    // 只断旗标是不够的：那四年绝大部分是脚本推的日历，advance_days 一分修为
    // 也不给，修为要靠修炼面板的日课补（见 CultivationScene.h 那段长注释）。
    // 这一条断的就是那条补账链路真的接上了——接不上，旗标照样是 3，修为却是 0。
    // 硬约束 #5 同时管上限：第三层是大纲给第 2 章的终点，不是中途站。
    EXPECT_EQ(flag("ch02.koujue_ceng"), 3);
    EXPECT_EQ(s.realm, Realm::QiRefining3)
        << "硬约束 #5：四年苦修，口诀至第三层，不多不少；实为编号 "
        << fanren::rules::toValue(s.realm);

    EXPECT_EQ(s.day - startDay_, kChapterDays)
        << "整章的天数该等于各脚本 advance_days 与那道三十日闸门之和；"
           "对不上就是某一处跳时被吞了或多走了";

    // 五个段落的推进旗标一个不落。
    for (const char* gate : {"ch02.duan2_start", "ch02.duan3_start", "ch02.duan4_start",
                             "ch02.duan5_start", "ch02.done"}) {
        EXPECT_EQ(flag(gate), 1) << gate << " 没置上，这一段其实没走过去";
    }

    std::cout << "\n[ch02 通关实测] 第 " << s.day << " 日　"
              << "绿液 " << s.bottle.drops << "/" << s.bottle.capacity << "　"
              << "田里第一畦 " << field->slots[0].age << " 年　"
              << "钱袋 " << s.itemCount(kMoneyId) << "　"
              << "境界编号 " << fanren::rules::toValue(s.realm) << std::endl;
}

// ---------------------------------------------------------------------------
// 另一条分支同样到得了终点
// ---------------------------------------------------------------------------
// 校对报告第四节 4.1 的结论是 972 / 972 全部走到 ch02.done。上面那一趟取的是
// 每个选择的第一项，这一趟全取第二项——第 1 章「某结局 0/81 不可达」那种事，
// 只有把另一侧也跑一遍才拦得住。
TEST_F(Ch02Walkthrough, EverySecondChoiceAlsoReachesTheEnd) {
    constexpr int kSecond = 1;
    GameState& s = state();

    ASSERT_TRUE(enterTrigger("trigger_renyao", kSecond));
    ASSERT_TRUE(usePortal("portal_to_jusuo", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_dazuo", kSecond));
    EXPECT_EQ(flag("ch02.kugong_jiezou"), 2) << "取第二项：加紧的那条作息";
    meditate(0);
    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(plantOneHerb());
    ASSERT_TRUE(usePortal("portal_to_jusuo", kSecond));
    meditate(kGateOneMeditateDays);
    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_caiyao", kSecond));
    ASSERT_EQ(flag("ch02.caiyao_done"), 1);

    ASSERT_TRUE(usePortal("portal_to_wairentang", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_maiyao", kSecond));
    ASSERT_EQ(flag("ch02.duan2_start"), 1);

    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(usePortal("portal_to_yabi", kSecond));
    ASSERT_TRUE(enterTrigger("trigger_shiping", kSecond));
    EXPECT_EQ(flag("ch02.shiping_taidu"), 2) << "取第二项：走出十来步又折回来";
    EXPECT_TRUE(s.bottle.owned) << "两条分支都会把瓶子捡起来";

    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(usePortal("portal_to_jusuo", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_zaping", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_kaigai", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_cangping", kSecond));
    EXPECT_EQ(flag("ch02.ping_gaozhi"), 0) << "取第二项：一个字也没提";
    ASSERT_EQ(flag("ch02.duan3_start"), 1);

    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(usePortal("portal_to_wairentang", kSecond));
    ASSERT_TRUE(enterTrigger("trigger_chousui", kSecond));
    ASSERT_EQ(flag("ch02.chousui_jian"), 1);

    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_zhitong", kSecond));
    EXPECT_EQ(flag("ch02.zhitong_fangzi"), 2) << "取第二项：温和不误事的那一副";
    ASSERT_EQ(flag("ch02.duan4_start"), 1);

    ASSERT_TRUE(usePortal("portal_to_jusuo", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_ceng2", kSecond));
    meditate(0);
    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(usePortal("portal_to_wairentang", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_renqing", kSecond));
    EXPECT_EQ(flag("ch02.renqing_huan"), 0) << "取第二项：那箱药材没收";
    EXPECT_EQ(s.itemCount(kHerb2Id), 0) << "不收就真的没进背包";
    ASSERT_EQ(flag("ch02.duan5_start"), 1);

    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(usePortal("portal_to_jusuo", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_ceng3", kSecond));
    meditate(0);
    ASSERT_TRUE(usePortal("portal_to_yaopu", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_shiyao", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_cuishu", kSecond));
    ASSERT_TRUE(fireTrigger("trigger_cuishu", kSecond));
    ASSERT_EQ(flag("ch02.cuanman_done"), 1);

    ASSERT_TRUE(usePortal("portal_to_wairentang", kSecond));
    const int seedlingsBeforeReckoning = s.itemCountOfAge(kHerbId, 0);
    ASSERT_TRUE(fireTrigger("trigger_suanzhang", kSecond));
    EXPECT_EQ(flag("ch02.done"), 1) << "另一侧的六次选择同样到得了章末";
    EXPECT_EQ(flag("ch02.qian_quxiang"), 2) << "取第二项：钱留着买药材";

    // 章末那一项选的是「全换成药材」：钱变成了种苗，这才是这次修的东西本身
    // ——从前它只置一个旗标，台词说「一块没动，全换成了药材」，背包里一株也没多。
    EXPECT_GT(s.itemCountOfAge(kHerbId, 0), seedlingsBeforeReckoning)
        << "台词说全换成了药材，背包里却一株也没多";
    EXPECT_EQ(s.itemCountOfAge(kHerbId, 0) - seedlingsBeforeReckoning, kSeedlingsBought)
        << "按收药处的买价 " << kSeedlingBuyPrice << " 块一株，手上 " << kGrossIncome
        << " 块该换到 " << kSeedlingsBought << " 株";

    // 「不收那箱药材」是六次选择里唯一有机制差别的一次（校对报告第四节 4.1），
    // 所以进算账那一场时手上只有一株种苗；换完药材是 1 + 28。
    // 钱剩的是除不尽的零头，与上一条通关路径那三分之一不同——两条分支各花各的。
    const BagVerdict bag = chapterEndBag(s, /*seedlings=*/1 + kSeedlingsBought, /*zishen=*/0,
                                         /*purse=*/kPurseAfterBuyingSeed);
    EXPECT_TRUE(bag.ok) << bag.why;
    EXPECT_EQ(s.bottle.drops, 0);
    EXPECT_EQ(s.day - startDay_, kChapterDays);
}

// ---------------------------------------------------------------------------
// 判据自检：证明上面那组章末断言真的有牙
// ---------------------------------------------------------------------------
// 「背包里找不到四十四年的黄精」在背包整个是空的时候照样通过。凡这种形状的
// 断言都要另配一条用例，先证明判据本身咬得动人，再拿它去验真实的运行结果。
// 这个项目最惨的一次事故正是校验器的正则被 heredoc 吃掉转义、从此永远报通过
// 而无人发现——因为没人做过负向验证。
TEST(Ch02EndState, TheBagVerdictCatchesAnUnsoldFortyFourYearHerb) {
    GameState clean;
    clean.addItem(kHerbId, 7, 0);
    clean.addItem(kHerb2Id, 2, 0);
    clean.addItem(kMoneyId, kPurseAfterSendingHome, 0);
    const BagVerdict ok = chapterEndBag(clean, 7, 2, kPurseAfterSendingHome);
    ASSERT_TRUE(ok.ok) << ok.why;

    // BLOCKER-1 的原样：take("黄精", 1, 44) 的年份被引擎丢掉，扣走的是背包里
    // 最便宜的那株种苗，钱照拿，四十四年那株原封不动留着，可以再卖一次。
    // 背包总数看着没错（还是七株），错的是扣掉了哪一堆。
    GameState broken;
    broken.addItem(kHerbId, 6, 0);
    broken.addItem(kHerbId, 1, 44);
    broken.addItem(kHerb2Id, 2, 0);
    broken.addItem(kMoneyId, kPurseAfterSendingHome, 0);
    const BagVerdict caught = chapterEndBag(broken, 7, 2, kPurseAfterSendingHome);
    EXPECT_FALSE(caught.ok) << "四十四年那株留在背包里，判据却说没事——它没有牙";
    EXPECT_NE(caught.why.find("四十四年"), std::string::npos) << caught.why;
}

TEST(Ch02EndState, TheBagVerdictRefusesAnEmptyBagInsteadOfWavingItThrough) {
    // 空背包能同时满足「找不到十一年那株」与「找不到四十四年那株」。
    // 判据必须先咬住「种苗还在、数目对得上」，否则一条把背包清空的缺陷会让
    // 上面那两条断言集体变成装饰。
    GameState empty;
    const BagVerdict verdict = chapterEndBag(empty, 7, 2, kPurseAfterSendingHome);
    EXPECT_FALSE(verdict.ok) << "空背包也算通关，那这组断言等于没写";
    EXPECT_NE(verdict.why.find("黄精"), std::string::npos) << verdict.why;

    GameState noMoney;
    noMoney.addItem(kHerbId, 7, 0);
    noMoney.addItem(kHerb2Id, 2, 0);
    const BagVerdict broke = chapterEndBag(noMoney, 7, 2, kPurseAfterSendingHome);
    EXPECT_FALSE(broke.ok) << "三次交易一分钱没进账也算通关？";
}

TEST(Ch02EndState, TheBagVerdictCatchesAChapterEndChoiceThatNeverTouchedTheMoney) {
    // 章末那个二选一从前只置一个旗标：钱一分没动、药材一件没给，而两句台词
    // 说的正是「捎了一大半回去」与「一块没动，全换成了药材」。校对把这一类
    // 列为要害——台词断言了某个状态，引擎里却不是那样。
    // 判据必须咬得动这个形状，否则修没修都一个样。
    GameState untouched;
    untouched.addItem(kHerbId, 7, 0);
    untouched.addItem(kHerb2Id, 2, 0);
    untouched.addItem(kMoneyId, kGrossIncome, 0);   // 三笔进账原封不动
    const BagVerdict caught = chapterEndBag(untouched, 7, 2, kPurseAfterSendingHome);
    EXPECT_FALSE(caught.ok) << "钱袋一分没少，判据却说没事";

    // 另一头也要咬住：不能把他掏空。
    GameState stripped;
    stripped.addItem(kHerbId, 7, 0);
    stripped.addItem(kHerb2Id, 2, 0);
    const BagVerdict broke = chapterEndBag(stripped, 7, 2, 0);
    EXPECT_FALSE(broke.ok) << "全扣光也算通关？第 3 章开局还要周转";
}

}  // namespace
