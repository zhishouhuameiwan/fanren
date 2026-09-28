// 第 3 章的自动化通关测试（设计文档 docs/ch03-design.md 第 9 节验收第 2 条），
// 外加第 4 条要的「曲魂入队后存档往返仍在队里」。
//
// 这一条测试驱动的是玩家实际跑的那一整套东西：真的 Application（无头）、真的
// maps/ch03_*.tmj 与 ch01_shenshougu / ch02_jusuo、真的 WorldScene 走位规则、
// 真的 scripts/ch03/*.lua、真的 BattleScene。从第 2 章章末踏出药圃那一步走到
// 章末离谷（ch03.done），十二个节点一个不落，沿途每一道闸门都验两次——条件
// 不满足时确实拦得住，条件满足后确实开得了。
//
// 为什么非得这样跑不可：第 1、2 章各栽过一次「每个脚本自己都对，错的是拼起来
// 之后」。本章的拼法比前两章复杂得多——十二个节点散在**七张图**上（谷外、
// 神手谷、密室、藏书处、暗道，外加第 2 章留下的药圃与居所），节点 7 要走回
// 第 2 章的药圃，节点 5、10、11 要走回第 2 章的居所去演。一段闸门写成不可
// 满足，整章就通不过去，而这件事在任何单测里都看不出来。
//
// 与邻居的分工（写之前逐个读过，不重复钉同一件事）：
//   * tests/Ch02SliceTests.cpp        —— 第 2 章的通关范例。走位、BFS、faceCell
//     全部照它写，那段「interact 看面朝的前一格」的规则原样适用（见下）。
//   * tests/Ch03BattleKitTests.cpp    —— 战场的法术/物品登记表，规则层。
//   * tests/Ch03BattleMenuTests.cpp   —— 动作菜单每一条的可否与理由，规则层。
//   * tests/Ch03ShihaiTests.cpp       —— 吞噬与逃遁的算术，规则层。
//   * tests/Ch03PartyTests.cpp        —— party 结构与存档字段，规则层。
//   * tests/Ch03TutorialBattleTests.cpp —— 三场战斗各自在玩家手里的样子。
//   * 本文件                          —— 整章一趟走通，钉的是闸门顺序、两种杀
//     的分别、章末的账，以及同伴过一次存档还在不在。
//
// ---------------------------------------------------------------------------
// 一条走位规则，与 Ch02SliceTests 同源，本章两种都有，照抄一遍免得再踩
// ---------------------------------------------------------------------------
// WorldScene::interact 看的是「面朝的前一格」，而 tryStep 先转向再迈步：迈得动
// 就走过去（这时前一格是再往前那一格），迈不动才只转向。于是要站在 S 上脸朝着
// 相邻的 F，只有两条路：
//   1. F 进不去（墙 / NPC / 设施占格）—— 站到 S 上，朝 F 按一下方向键，人不动、
//      脸转过去。ch03_mishi 的 trigger_shichong 就紧挨着墨大夫站的那一格。
//   2. F 进得去 —— 那就得是「从 F 的另一侧那一格出发，朝 F 的方向迈一步落到 S」。
//      ch03_andao 的 trigger_yunchi、trigger_tiezhe 都摆在走得进去的地面上。
// faceCell 把两条都实现了。只实现一种会有一半触发器点不着。
//
// ---------------------------------------------------------------------------
// 本章多出来的一条：路上不许顺手把剧情演了
// ---------------------------------------------------------------------------
// 第 2 章的 BFS 只躲墙、NPC、设施与传送点。本章不够用：ch03_mishi 一进门就是
// mode=enter 的 trigger_tanpai，ch02_jusuo 屋里床边那一排是 mode=enter 的
// trigger_duoshe，而它们都压在必经之路附近。路过时被踩响，「闸门拦得住」那一组
// 断言就会在一个早已被演过的世界里空转。所以 standable() 多问一句：这一格上有
// 没有**此刻还备着**的踏入型触发（WorldScene::triggerReady，与引擎同一个判据）。
// 绕不过去时才退回允许踩，并且退回这件事本身会在天数与旗标上露出来。
//
// ---------------------------------------------------------------------------
// 本章第三条：**两种杀是用什么键开始的，本身就是内容**
// ---------------------------------------------------------------------------
// 节点 10（夺舍）挂 enter、节点 11（处决）挂 interact，这一对不是接线细节，是
// 大纲注解给本章的全部道德分量（设计第 0.3 节）。一度装反过，而本文件当时是
// **照着 tmj 写驱动**的：哪个 trigger 是什么 mode，就拿哪个函数去驱动它。
// 于是这里的 fireTrigger / enterTrigger 把地图当时的样子原样固化成了绿灯，
// 地图改回设计要求的样子反而会转红——比没牙更难发现，因为没人去动一条绿的。
//
// 教训落成两条做法，**下一章照办**：
//   1. 驱动方式不再是随手选的。下面每一处 fireTrigger / enterTrigger 都按设计
//      文档要的操作感选，选错了会红；注释里写明为什么是这一个。
//   2. 更要紧的是：**驱动方式不能当断言用**。mode 这件事另由
//      tests/Ch03TriggerModeTests.cpp 直接断言两个 trigger 的属性本身——
//      那一条不经过走位、不经过脚本，改坏 tmj 它立刻红。
#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/Scene.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::game::Application;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// ---- 本章踏过的七张图 ----
constexpr const char* kMapShenshougu = "ch01_shenshougu";
constexpr const char* kMapYaopu = "ch02_yaopu";
constexpr const char* kMapJusuo = "ch02_jusuo";
constexpr const char* kMapGuwai = "ch03_guwai";
constexpr const char* kMapMishi = "ch03_mishi";
constexpr const char* kMapCangshu = "ch03_cangshu";
constexpr const char* kMapAndao = "ch03_andao";

// ---- 本章动到的物品 id ----
constexpr const char* kMoneyId = "material_lingshi";
constexpr const char* kHerbId = "herb_huangjing_cao";
constexpr const char* kHerb2Id = "herb_zishen_cao";
constexpr const char* kTuguId = "herb_tugu_hua";
constexpr const char* kWuduId = "pill_wudu_shui";
constexpr const char* kQiduId = "pill_qidu_shui";
constexpr const char* kShixinId = "pill_shixin_san";
constexpr const char* kQingduId = "pill_qingdu_san";
constexpr const char* kSwordId = "weapon_yudai_duanjian";
constexpr const char* kShouzhaId = "story_mo_shouzha";
constexpr const char* kJiashuId = "story_mo_jiashu";

// 章末入队的那一位。**是傀儡 qu_hun，不是原著后期那个身外化身**——后者在
// data/roles 里另有其人（qu_hun_huashen），入错了在测试里是看不出来的，
// 除非像下面那样把两个 id 一起钉住。
constexpr const char* kCompanion = "qu_hun";
constexpr const char* kCompanionLate = "qu_hun_huashen";

// ---- 第 2 章章末交到本章手里的东西 ----
// 逐条对着 tests/Ch02SliceTests.cpp 的实算：三笔进账 6 + 22 + 145 = 173，
// 章末取第一项「捎一大半回家」扣掉三分之二，剩 173 - 115 = 58 块碎银。
// 这几个数在本章只有一个用处：节点 7 那把 30 块的玉带短剑买不买得起。
// 所以下面另有一条断言钉住「买得起」这件事本身，而不是只对一次数目——
// 数目将来若随第 2 章一起变，要红的是那条，不是这里。
constexpr int kStartPurse = 58;
constexpr int kStartSeedlings = 7;   // 零年份的黄精种苗
constexpr int kStartZishen = 2;      // 厉飞雨那两株紫参草
constexpr int kSwordPrice = 30;      // data/items/weapons/yudai_duanjian.json

// ---- 本章的日历 ----
// 每一条都取自对应脚本里那句 advance_days。两处分支（节点 6 偷秘籍、节点 9
// 解药）都刻意把多花的日子从末尾那次里扣回去，所以合计与选哪一项无关——
// 这一点本身要验，见 TheOtherSideOfEveryChoiceAlsoReachesTheEnd。
constexpr int kElangDays = 330 + 3;   // elang.lua：谷里又是一年，配五毒水又三日
constexpr int kMoguiDays = 1;         // mogui.lua
constexpr int kYingduiDays = 30;      // yingdui.lua
constexpr int kMijiDays = 60;         // miji.lua（两条分支都是 60）
constexpr int kBeiduDays = 45;        // beidu.lua
constexpr int kYunchiDays = 3;        // yunchi.lua
constexpr int kAndaoDays = 120;       // andao_zhan.lua
// 暗道那一课的账（docs/octopath-battle.md 第 7 节僵兽那一行）：两包蚀心散正好把僵兽打到破势一次，
// 破势时刀才砍得动。会打的那只手（tests/BattleHand.h）赢下来时撒掉的正是这两包。
constexpr int kAndaoPoisonDoses = 2;
constexpr int kJieyaoDays = 77 + 22;  // jieyao.lua（两条分支都是 77 + 22）
constexpr int kDuosheDays = 4;        // duoshe.lua
constexpr int kLiguDays = 1;          // ligu.lua

constexpr int kChapterDays = kElangDays + kMoguiDays + kYingduiDays + kMijiDays + kBeiduDays +
                             kYunchiDays + kAndaoDays + kJieyaoDays + kDuosheDays + kLiguDays;

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

constexpr double kFrame = 1.0 / 60.0;
// 死循环闸。本章最长的一场是 chujue.lua（三十余句），远够用；脚本卡住时
// 宁可测试失败，也不要挂住整个 ctest。识海那一场在无头里是整场自动打完的，
// 也算在这几千帧里。
constexpr int kMaxScriptFrames = 6000;

// 仓库根：测试可能从 build/ 或工程根启动。判据用本章自己的脚本与地图，
// 找错根目录时报的是「找不到 ch03 的东西」，而不是一串莫名其妙的空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch03" / "ligu.lua") &&
            fs::exists(root / "maps" / "ch03_andao.tmj")) {
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
// 谷里各处之间的走法
// ---------------------------------------------------------------------------
// 本章要在七张图之间来回跑十几趟（节点 5、10、11 在第 2 章的居所，节点 6 在
// 藏书处，节点 8、12 在暗道）。把地图之间那几条路写成一张表，好过在每一节里
// 重抄一遍「先回神手谷、再进药圃、再进居所」。
//
// 表里只有「哪张图能通到哪张图、走哪个传送点」，**不写任何 require_flag**：
// 闸开没开由引擎说了算，走不过去时 usePortal 会如实报 false，而那正是
// 「这道闸此刻拦得住」要的那个返回值。把闸门条件抄进这张表，等于让测试自己
// 判卷。
struct MapLink {
    const char* from;
    const char* to;
    const char* portal;
};

// 神手谷那三条的顺序有讲究：密室排在谷外前面，于是从居所去暗道时 BFS 取的是
// 「经密室」那条（两条都是四跳）。两条都通，但经密室那条与剧情顺序一致。
constexpr MapLink kMapLinks[] = {
    {kMapShenshougu, kMapMishi, "portal_to_ch03_mishi"},
    {kMapShenshougu, kMapGuwai, "portal_to_ch03_guwai"},
    {kMapShenshougu, kMapYaopu, "portal_to_ch02_yaopu"},
    {kMapGuwai, kMapShenshougu, "portal_to_shenshougu"},
    {kMapGuwai, kMapAndao, "portal_to_andao"},
    {kMapMishi, kMapShenshougu, "portal_to_shenshougu"},
    {kMapMishi, kMapCangshu, "portal_to_cangshu"},
    {kMapMishi, kMapAndao, "portal_to_andao"},
    {kMapCangshu, kMapMishi, "portal_to_mishi"},
    {kMapAndao, kMapMishi, "portal_to_mishi"},
    {kMapAndao, kMapGuwai, "portal_to_guwai"},
    {kMapYaopu, kMapShenshougu, "portal_to_shenshougu"},
    {kMapYaopu, kMapJusuo, "portal_to_jusuo"},
    {kMapJusuo, kMapYaopu, "portal_to_yaopu"},
};

// ---------------------------------------------------------------------------
// 章末的资产判据
// ---------------------------------------------------------------------------
// 做成一个具名判据而不是就地摊一串 EXPECT，是为了能拿同一份判据去跑一条
// 「故意摆错 → 判据确实说不」的自检（见本文件末尾那几条 Ch03EndState 用例）。
//
// 本章的账里有两笔是「某样东西该没了」：交给墨居仁的五毒水、处决时灌下去的
// 七毒水。这种形状的断言有个天然的空转法——背包整个是空的时候它照样通过。
// 所以下面每一条「该没了」的旁边都钉着一条「该还在」的先验判据（蚀心散、
// 清毒散、那把短剑、七株种苗），少了它们，一条把背包清空的缺陷会让「用掉了」
// 这一组断言集体变成装饰。
struct EndBag {
    int money = 0;
    int tugu = 0;      // 挖回来没配掉的土菇花
    int shixin = 0;    // 蚀心散
    int qingdu = 0;    // 清毒散
    int wudu = 0;      // 五毒水：交出去了就该是 0
    int qidu = 0;      // 七毒水：处决时灌了就该是 0
    bool sword = true; // 那把 30 块的玉带短剑
};

struct BagVerdict {
    bool ok = true;
    std::string why;
};

BagVerdict chapterEndBag(const GameState& state, const EndBag& want) {
    const auto fail = [](std::string why) { return BagVerdict{false, std::move(why)}; };
    const auto expect = [&](const char* id, int wanted, const char* what) -> BagVerdict {
        if (state.itemCount(id) == wanted) return BagVerdict{};
        return fail(std::string(what) + "该有 " + std::to_string(wanted) + " 件，实为 " +
                    std::to_string(state.itemCount(id)) + " 件");
    };

    // ---- 先验判据：这个背包里确实还有东西 ----
    // 下面那两条「该没了」全靠这一组撑着。第 2 章的章末判据正是在这里补过一刀。
    if (state.itemCount(kHerbId) != kStartSeedlings) {
        return fail("第 2 章带过来的七株黄精种苗该原封不动（本章一株也没动过），实为 " +
                    std::to_string(state.itemCount(kHerbId)) + " 株");
    }
    if (state.itemCount(kHerb2Id) != kStartZishen) {
        return fail("厉飞雨那两株紫参草该原封不动，实为 " +
                    std::to_string(state.itemCount(kHerb2Id)) + " 株");
    }
    if (BagVerdict v = expect(kShixinId, want.shixin, "蚀心散"); !v.ok) return v;
    if (BagVerdict v = expect(kQingduId, want.qingdu, "清毒散"); !v.ok) return v;
    if (BagVerdict v = expect(kTuguId, want.tugu, "土菇花"); !v.ok) return v;
    if (want.sword && state.itemCount(kSwordId) != 1) {
        return fail("那把玉带短剑该在身上：节点 7 手头 " + std::to_string(kStartPurse) +
                    " 块，买得起 " + std::to_string(kSwordPrice) + " 块的剑");
    }
    if (!want.sword && state.itemCount(kSwordId) != 0) {
        return fail("这一趟没买剑，身上不该有");
    }
    // 两件剧情物：偷来的手札与搜出来的家书。它们是节点 6 与节点 12 真的发生过
    // 的物证——只看旗标的话，give() 漏写一行是看不出来的。
    if (state.itemCount(kShouzhaId) != 1) {
        return fail("节点 6 偷出来的那叠手札该在身上");
    }
    if (state.itemCount(kJiashuId) != 1) {
        return fail("节点 12 从尸身上搜出来的那封家书该在身上");
    }
    if (state.itemCount(kMoneyId) != want.money) {
        return fail("钱袋该剩 " + std::to_string(want.money) + " 块，实为 " +
                    std::to_string(state.itemCount(kMoneyId)) + " 块");
    }

    // ---- 该没了的两笔 ----
    if (BagVerdict v = expect(kWuduId, want.wudu, "五毒水"); !v.ok) return v;
    if (BagVerdict v = expect(kQiduId, want.qidu, "七毒水"); !v.ok) return v;
    return BagVerdict{};
}

// ---------------------------------------------------------------------------
// 两种杀的判据
// ---------------------------------------------------------------------------
// 本章的要害：ch03.mo_siwang（墨居仁暴毙）与 ch03.yuzitong_chujue（余子童被
// 处决）章末都是 1，但它们来自完全不同的操作路径。合成一条断言（「两个旗标
// 都置上了」）会把这一章的道德分量整个抹掉——那正是设计文档第 0 节第 3 条
// 反复强调的东西。
//
// 分别钉住的办法是**看它们各自是在什么时候、在玩家做了什么之后变的**。
// 两件事同在 chujue.lua 里，所以只钉脚本文件名没有用；这里把整场戏逐句录下来，
// 在每一句话弹出来的那一刻采一次样：旗标、玩家做过几次选择、背包里那瓶七毒水
// 还在不在。
struct SceneStep {
    int index = 0;
    std::string key;         // 这一句的文案 key；轮到玩家选时为空
    bool choice = false;     // 这一步是不是在问玩家
    int moSiwang = 0;
    int chujue = 0;
    int poison = 0;          // 背包里七毒水的数目
};

struct KillEvidence {
    std::vector<SceneStep> steps;
    int finalMoSiwang = 0;
    int finalChujue = 0;
    int poisonBefore = 0;
    int poisonAfter = 0;
};

struct KillVerdict {
    bool ok = true;
    std::string why;
};

KillVerdict twoKillsVerdict(const KillEvidence& e) {
    const auto fail = [](std::string why) { return KillVerdict{false, std::move(why)}; };

    // ---- 先验判据：这一场戏确实演过，而且确实问过玩家 ----
    // 少了这一组，下面每一条「在玩家被问到之前」都会在一个空的 trace 上恒真。
    if (e.steps.empty()) {
        return fail("一句话也没录到：判据跑在一场没演过的戏上，下面每一条都是恒真的");
    }
    const auto firstChoice =
        std::find_if(e.steps.begin(), e.steps.end(), [](const SceneStep& s) { return s.choice; });
    if (firstChoice == e.steps.end()) {
        return fail("整场戏一次也没问过玩家：「墨之死在玩家动手之前」这句话就无从谈起");
    }
    if (e.finalMoSiwang != 1) return fail("ch03.mo_siwang 没置上：墨居仁没死");
    if (e.finalChujue != 1) return fail("ch03.yuzitong_chujue 没置上：余子童没被处决");

    // ---- 第一种杀：墨居仁暴毙，玩家一下也没动 ----
    const auto flip = std::find_if(e.steps.begin(), e.steps.end(),
                                   [](const SceneStep& s) { return s.moSiwang == 1; });
    if (flip == e.steps.end()) {
        return fail("整场戏里 ch03.mo_siwang 一直是 0，到收场才不知从哪冒出来——"
                    "这一件是「身醒敌亡」当场发生的，不是收尾时补记的");
    }
    for (auto it = e.steps.begin(); it != flip; ++it) {
        if (it->choice) {
            return fail("墨居仁之死发生在玩家做过选择之后：那就成了玩家动的手，"
                        "而硬约束 #6 写的是元神被吞、他自己暴毙");
        }
        if (it->key.rfind("ch03.shenxing.", 0) != 0) {
            return fail("墨居仁之死之前混进了不属于「身醒」那一段的台词：" + it->key);
        }
    }
    if (flip->poison != e.poisonBefore) {
        return fail("墨居仁死的时候玩家背包已经动过了：那一瓶七毒水是留给余子童的");
    }
    if (flip->chujue != 0) {
        return fail("两个旗标在同一步里一起翻了——合成一条正是这一章最不该出的错");
    }

    // ---- 第二种杀：余子童被处决，这一件是玩家主动做的 ----
    for (const SceneStep& step : e.steps) {
        if (step.chujue != 0) {
            return fail("ch03.yuzitong_chujue 在戏还没演完时就置上了：处决该是"
                        "最后那几句之后的事");
        }
    }
    if (firstChoice < flip) {
        return fail("玩家在墨居仁死之前就被问了话：两件事的先后颠倒了");
    }
    const bool choiceAfterFlip =
        std::any_of(flip, e.steps.end(), [](const SceneStep& s) { return s.choice; });
    if (!choiceAfterFlip) {
        return fail("墨居仁死后玩家一句话也没被问过：那处决也成了自动发生的");
    }
    if (e.poisonAfter != e.poisonBefore - 1) {
        return fail("那一瓶七毒水没从背包里少：台词说灌下去了，引擎里却什么也没发生"
                    "（背包 " + std::to_string(e.poisonBefore) + " → " +
                    std::to_string(e.poisonAfter) + "）");
    }
    return KillVerdict{};
}

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch03Walkthrough : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 本章开头：第 2 章章末把玩家留在谷里，下一步是出谷办事。
        // 落点取神手谷通往药圃那个 spawn——那正是第 2 章四年都在走的那条路。
        auto loaded = app_.loadMap(kMapShenshougu, "spawn_from_ch02_yaopu");
        ASSERT_TRUE(loaded.ok) << loaded.error;
        applyChapterTwoEnding();
        startDay_ = state().day;
    }

    void TearDown() override { app_.shutdown(); }

    // 第 2 章章末交到本章手里的那份存档。
    //
    // 为什么不真跑一遍第 2 章：那一趟已经由 tests/Ch02SliceTests.cpp 逐条钉过，
    // 在这里重跑一遍只会让第 3 章的红灯有一半来自第 2 章。代价是这份手搭的
    // 起点可能与真实的第 2 章漂移，所以下面每一行都写明它对应第 2 章的哪一条，
    // 而真正要紧的那一条（钱够不够买剑）另有断言钉着，不靠这里的数目。
    void applyChapterTwoEnding() {
        GameState& s = state();
        // 第 1、2 章目标链上每一步的完成旗标：真实存档走到这里，它们必然都已置上——
        // 目标链的 doneFlag 就是剧情脚本自己置的那个旗标（core::Objective 的注释）。
        // 从前这里只手写了几道门要的那几个，于是这份手搭的章末，连同从它一路传下去的
        // 第 3、4、5 章交接存档与 saves/ch04-*.sav，都少了 ch01.sanshu_met、ch01.climb_done
        // 这一批：地图上按它们撤场的第 1 章人物（三叔、韩母、山道张铁、两位考官）在那些
        // 存档里又站了出来，一按还会把第 1 章重演一遍（docs/tech-debt.md G-23）。
        // 从目标链读而不手列：链上将来多一步，这里不用跟着改。
        for (const fanren::core::Objective& step : app_.data().objectives) {
            if (step.chapter <= 2) s.setFlag(step.doneFlag);
        }
        s.setFlag("ch01.done");            // 神手谷 → 药圃那道门（章末脚本与 koujue_received 一起置，不在链上）
        s.setFlag("ch02.renqing_jiexia");  // 段三：人情结下（不在链上、主线必经；外刃堂的厉飞雨按它出场）
        s.setFlag("ch02.xiangqi_ping");    // 段五：墨大夫自此不在药圃现身（不在链上）
        s.setFlag("ch02.koujue_ceng", 3);  // 四年苦修的终点
        s.setFlag("ch02.qian_quxiang", 1); // 章末取第一项：钱捎回家。节点 4 回响它
        s.setFlag("ch02.done");            // 谷外那道门
        s.realm = Realm::QiRefining3;      // Ch02SliceTests 钉死的章末境界
        s.realmCap = Realm::QiRefining3;   // 第 2 章 ceng3.lua 抬的剧情境界上限（技术债 G-14）
        // 境界给出的气血 / 法力基准（core/rules/Realm.h）。真机上这两个数是跟着
        // 那几次突破一起上来的（修炼面板当场补齐，老存档由 v3→v4 迁移补齐），
        // 所以这份手搭的章末存档也得带上，否则本章三场仗打的是一个 10 点气血、
        // 现实中不存在的韩立。
        s.hp = s.maxHp = fanren::rules::realmMaxHp(Realm::QiRefining3);
        s.mp = s.maxMp = fanren::rules::realmMaxMp(Realm::QiRefining3);
        s.bottle.owned = true;             // 掌天瓶随身，本章不用它，但它确实在
        s.bottle.matureKnown = true;
        s.addItem(kHerbId, kStartSeedlings, 0);
        s.addItem(kHerb2Id, kStartZishen, 0);
        s.addItem(kMoneyId, kStartPurse, 0);
    }

    GameState& state() { return app_.state(); }
    int flag(const std::string& name) { return state().flag(name); }

    // 刚才那一节里说过这一句没有。
    //
    // 判据只认「说过 / 没说过」，所以每一次 fireTrigger / enterTrigger 都先把
    // 台词本清空——不清的话 spokenKeys 攒到 128 条就开始丢最旧的那几条
    // （Application::kSpokenLogCap），于是隔了几节再回头问「当时说过没有」
    // 会拿到一个假的「没说过」。这正是「找不到某个词就算过」那一类空转法。
    bool spoke(const std::string& key) const {
        const std::vector<std::string>& keys = app_.spokenKeys();
        return std::find(keys.begin(), keys.end(), key) != keys.end();
    }

    // ---- 脚本 ----

    // 像主循环那样把脚本推到结束，替玩家按确认；碰上战斗就交给那只手打。
    //
    // **战斗那一条命令不能在这里替它回填。** 脚本拿到的得是一场真打过的战果，
    // 本章三场仗的分支全押在那个返回值上。BattleScene 是在帧末才压进场景栈的，
    // tick 之后立刻接手、赶在它自己 update 之前打完；下一帧 update 见胜负已分，
    // 自己 finish 并回填给脚本。替玩家出手的是第 3、4、5 章共用的那只手
    //（tests/BattleHand.h）：从前这里让规则层的我方 AI 自己打，那一套不撒毒、不吃药，
    // 暗道那一课它根本学不会。
    bool playBattleOnTop() {
        auto* fight = dynamic_cast<fanren::game::BattleScene*>(app_.topScene());
        if (fight == nullptr) return false;
        if (fight->battle().phase() == fanren::core::battle::BattlePhase::Ongoing) static_cast<void>(hand_.play(*fight));
        return true;
    }

    void pumpScripts(int choiceIndex) {
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (playBattleOnTop()) continue;
            if (!app_.awaitingCommand()) continue;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = choiceIndex;
            app_.completeCommand(result);
            // 对话框是脚本压进来的，演完就收掉。不收的话整章三百多句话会在
            // 场景栈上摞成三百多层，后面拿 topScene() 验「被拦下时有没有说明」
            // 就全是噪声。
            app_.popScene();
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        // 最后一句话的对话框还挂在栈上，收场时的那次 popScene 也还压着没生效。
        // 多推一帧把两者一起结清。
        app_.tick(kFrame);
    }

    // 同上，但把每一句录下来。两种杀的分别只有靠这份逐句的样本才说得清。
    void traceScripts(int choiceIndex, std::vector<SceneStep>& out) {
        app_.clearSpokenKeys();
        std::size_t spoken = 0;
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (playBattleOnTop()) continue;
            if (!app_.awaitingCommand()) continue;

            SceneStep step;
            step.index = static_cast<int>(out.size());
            // talk 会把文案 key 记进 spokenKeys，choice 不会（见 Application::dispatch）。
            // 于是「这一步问的是不是玩家」不必去翻 DialogueScene 的私有字段。
            if (app_.spokenKeys().size() > spoken) {
                spoken = app_.spokenKeys().size();
                step.key = app_.lastSpokenKey();
            } else {
                step.choice = true;
            }
            step.moSiwang = flag("ch03.mo_siwang");
            step.chujue = flag("ch03.yuzitong_chujue");
            step.poison = state().itemCount(kQiduId);
            out.push_back(std::move(step));

            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = choiceIndex;
            app_.completeCommand(result);
            app_.popScene();
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        app_.tick(kFrame);
    }

    // ---- 地图与走位 ----

    const TileMap& map() const { return *app_.currentMap(); }

    // 按名字取地图对象。返回副本而不是指针：换一次图 TileMap 就整个重建，
    // 留着的指针会悄悄变成悬空的。
    MapObject objectNamed(const std::string& name) const {
        for (const MapObject& object : map().objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    // 这一格能不能进：WorldScene::tryStep 的真实规则，一条不多一条不少。
    bool enterable(Point p) {
        if (!map().walkable(p)) return false;
        if (WorldScene::visibleNpcAt(app_.state(), map(), p) != nullptr) return false;
        if (map().objectAt(p, "facility") != nullptr) return false;
        return true;
    }

    // 这一格上有没有此刻还备着的踏入型触发。判据直接用引擎那一个
    // （WorldScene::triggerReady），比引擎宽一格就会路过时把剧情演了，
    // 比引擎严一格则会绕不出去——两头都是假红灯。
    bool freeOfLiveEnterTrigger(Point p) {
        const MapObject* trigger = map().objectAt(p, "trigger");
        if (trigger == nullptr) return true;
        if (trigger->property("mode") != "enter") return true;
        return !WorldScene::triggerReady(app_.state(), *trigger);
    }

    // 这一格能不能当落脚点 / 中途格。传送点另算：踩上去就换图了，
    // 不能拿它当路过的格子。
    bool standable(Point p, bool avoidTriggers) {
        if (!enterable(p)) return false;
        if (map().objectAt(p, "portal") != nullptr) return false;
        return !avoidTriggers || freeOfLiveEnterTrigger(p);
    }

    // 迈一步，并把可能被踩发的踏入型触发演完。
    bool step(Point direction, int choiceIndex) {
        const bool moved = world_.tryStep(app_, direction.x, direction.y);
        if (app_.scripts().isRunning()) pumpScripts(choiceIndex);
        return moved;
    }

    // 从当前位置到 goal 的一条路。用 BFS 而不是写死路线：地图一改，写死的
    // 路线会悄悄失效，而 BFS 会如实报告「走不到」。
    std::vector<Point> routeTo(Point goal, bool avoidTriggers) {
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
                if (next.x < 0 || next.y < 0 || next.x >= map().width || next.y >= map().height) {
                    continue;
                }
                if (previous.count(index(next))) continue;
                if (next != goal && !standable(next, avoidTriggers)) continue;
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
        // 先找一条不碰任何活着的踏入型触发的路；实在绕不过去才退回允许踩。
        std::vector<Point> path = routeTo(goal, /*avoidTriggers=*/true);
        if (path.empty()) path = routeTo(goal, /*avoidTriggers=*/false);
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
            if (!standable(stand, /*avoidTriggers=*/true)) continue;

            if (!enterable(target)) {
                if (!walkTo(stand, choiceIndex)) continue;
                step(direction, choiceIndex);
            } else {
                const Point approach{stand.x - direction.x, stand.y - direction.y};
                if (!standable(approach, /*avoidTriggers=*/true)) continue;
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
                    if (!standable(stand, /*avoidTriggers=*/true)) continue;
                    if (!walkTo(stand, choiceIndex)) continue;
                    step(direction, choiceIndex);
                    return true;
                }
            }
        }
        return false;
    }

    // 走到目标格旁边、迈最后一步，**但不把脚本演完**。
    //
    // 两个用处：踏入型那一节要逐句录样本时用它；验「走过去不该有事发生」时
    // 也用它——踩完看 scripts().isRunning() 就知道这一格是不是会自己开演。
    bool stepOntoWithoutPumping(const MapObject& object) {
        for (int dy = 0; dy < std::max(1, object.height); ++dy) {
            for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                const Point target{object.position.x + dx, object.position.y + dy};
                if (!map().walkable(target)) continue;
                for (const Point& direction : kDirections) {
                    const Point stand{target.x - direction.x, target.y - direction.y};
                    if (!standable(stand, /*avoidTriggers=*/true)) continue;
                    if (!walkTo(stand)) continue;
                    world_.tryStep(app_, direction.x, direction.y);
                    return true;
                }
            }
        }
        return false;
    }

    // 站到 interact 触发器跟前、按下确认，**但不把脚本演完**。
    // 与 stepOntoWithoutPumping 成对：一个踩、一个按，两种杀各用一个。
    bool interactWithoutPumping(const MapObject& object) {
        if (!faceObject(object, /*choiceIndex=*/0)) return false;
        app_.clearSpokenKeys();
        return world_.interact(app_);
    }

    // ---- 三个动作 ----

    // 点着一处 interact 触发器，并把它演完。
    //
    // 返回值是「脚本真的起来了」，而不是「旗标变了没有」——闸门拦住时脚本照样
    // 起来（说一句话就 return），与「压根没点着」长得一模一样。验闸门拦得住的
    // 那几条断言全靠这个区分。
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
        app_.clearSpokenKeys();   // spoke() 问的是「这一节说了什么」，见那里的注释
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
        app_.clearSpokenKeys();
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

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    // 一路走到某张图。地图之间那几跳按 kMapLinks 做 BFS，每一跳都走真的传送点。
    bool travelTo(const std::string& target, int choiceIndex = 0) {
        for (int hop = 0; hop < 8; ++hop) {
            if (app_.state().mapId == target) return true;
            const std::string portal = nextPortalToward(app_.state().mapId, target);
            if (portal.empty()) {
                ADD_FAILURE() << "从 " << app_.state().mapId << " 没有路通到 " << target;
                return false;
            }
            if (!usePortal(portal, choiceIndex)) {
                ADD_FAILURE() << "从 " << app_.state().mapId << " 走 " << portal << " 没过去";
                return false;
            }
        }
        ADD_FAILURE() << "地图之间绕了八跳还没到 " << target;
        return false;
    }

    // 下一跳该走哪个传送点。BFS 只认 kMapLinks 那张表，不认任何闸门条件。
    static std::string nextPortalToward(const std::string& from, const std::string& target) {
        std::map<std::string, std::string> cameFrom;
        std::map<std::string, std::string> viaPortal;
        std::deque<std::string> queue{from};
        cameFrom[from] = from;
        while (!queue.empty()) {
            const std::string here = queue.front();
            queue.pop_front();
            if (here == target) break;
            for (const MapLink& link : kMapLinks) {
                if (here != link.from) continue;
                if (cameFrom.count(link.to)) continue;
                cameFrom[link.to] = here;
                viaPortal[link.to] = link.portal;
                queue.push_back(link.to);
            }
        }
        if (!cameFrom.count(target)) return {};
        std::string node = target;
        while (cameFrom[node] != from) node = cameFrom[node];
        return viaPortal[node];
    }

    // -----------------------------------------------------------------------
    // 十二个节点，每一节只管「把玩家送到那一步并演完」
    // -----------------------------------------------------------------------
    // 断言一律留给用例：同一条路要被三条用例走（通关、另一侧的选择、两种杀），
    // 把断言塞进这里会让三条用例共用一套判据，而那正是「一处写错三处一起瞎」
    // 的长相。
    bool nodeElang(int c) {          // 1 谷外遇狼 · 战棋教学一
        return travelTo(kMapGuwai, c) && enterTrigger("trigger_elang", c);
    }
    bool nodeMogui(int c) {          // 2 墨大夫归谷
        return travelTo(kMapShenshougu, c) && enterTrigger("trigger_mogui", c);
    }
    bool nodeTanpai(int c) {         // 3 摊牌：他是墨居仁
        return travelTo(kMapMishi, c) && enterTrigger("trigger_tanpai", c);
    }
    bool nodeShichong(int c) {       // 4 尸虫丸
        return travelTo(kMapMishi, c) && fireTrigger("trigger_shichong", c);
    }
    bool nodeYingdui(int c) {        // 5 表面顺从，暗中盘算
        return travelTo(kMapJusuo, c) && fireTrigger("trigger_yingdui", c);
    }
    bool nodeMiji(int c) {           // 6 偷秘籍
        return travelTo(kMapCangshu, c) && fireTrigger("trigger_miji", c);
    }
    bool nodeBeidu(int c) {          // 7 备毒
        // 药圃，不是密室。这一节是挖土菇花配毒，戏全在畦头上；挂点 2026-09-21
        // 从 ch03_mishi 挪到 ch02_yaopu（裁决见 scripts/ch03/beidu.lua 首部）。
        // 于是这一节比从前多走两跳：藏书处 → 密室 → 神手谷 → 药圃。
        return travelTo(kMapYaopu, c) && fireTrigger("trigger_beidu", c);
    }
    bool nodeYunchi(int c) {         // 8 上 云翅鸟：余子童浮出
        return travelTo(kMapAndao, c) && fireTrigger("trigger_yunchi", c);
    }
    bool nodeAndaoZhan(int c) {      // 8 下 暗道那一仗 · 战棋教学二
        return travelTo(kMapAndao, c) && enterTrigger("trigger_andao_zhan", c);
    }
    bool nodeJieyao(int c) {         // 9 解药
        return travelTo(kMapMishi, c) && fireTrigger("trigger_jieyao", c);
    }
    // 节点 10 与 11 的驱动方式**是内容的一部分**，不是随手选的（见文件头第三段）。
    bool nodeDuoshe(int c) {         // 10 夺舍：识海之战
        // enterTrigger：走到床边就发生，玩家不按任何一个键。这一节连 choice()
        // 都没有，失重感的另一半正来自这里。用 fireTrigger 驱动它就等于把
        // 「他睡着的时候，事情就已经办完了」演成了他自己点开的。
        return travelTo(kMapJusuo, c) && enterTrigger("trigger_duoshe", c);
    }
    bool nodeChujue(int c) {         // 11 身醒敌亡；处决余子童
        // fireTrigger：走过去、面朝它、按确认。ch03.chujue.after「这一回是他
        // 自己走过去、自己叫的名字、自己按下的拇指」说的就是这一下确认键。
        return travelTo(kMapJusuo, c) && fireTrigger("trigger_chujue", c);
    }
    bool nodeTiezhe(int c) {         // 12 上 三大铁则
        return travelTo(kMapAndao, c) && fireTrigger("trigger_tiezhe", c);
    }
    bool nodeQuhun(int c) {          // 12 中 曲魂入队
        return travelTo(kMapAndao, c) && fireTrigger("trigger_quhun", c);
    }
    bool nodeLigu(int c) {           // 12 下 离谷
        return travelTo(kMapGuwai, c) && fireTrigger("trigger_ligu", c);
    }

    // 走到节点 11 跟前，一句断言也不做。两种杀那条用例与同伴存档那条都要用。
    void playUpToTheExecution(int c) {
        ASSERT_TRUE(nodeElang(c));
        ASSERT_TRUE(nodeMogui(c));
        ASSERT_TRUE(nodeTanpai(c));
        ASSERT_TRUE(nodeShichong(c));
        ASSERT_TRUE(nodeYingdui(c));
        ASSERT_TRUE(nodeMiji(c));
        ASSERT_TRUE(nodeBeidu(c));
        ASSERT_TRUE(nodeYunchi(c));
        ASSERT_TRUE(nodeAndaoZhan(c));
        ASSERT_TRUE(nodeJieyao(c));
        ASSERT_TRUE(nodeDuoshe(c));
        ASSERT_EQ(flag("ch03.shihai_done"), 1);
    }

    // 这一趟的终局与交接存档 tests/fixtures/<fileName> **逐字段**一致（第 4 章复验判据 1）。
    //
    // 那份存档是第 4 章通关测试的起点（tests/Ch04SliceTests.cpp 直接读它），所以
    // 这一条看住的是**两章之间的缝**：第 3 章改了而交接存档没跟着重生成，这里红；
    // 有人手改了交接存档，这里也红。口径（比哪些行、为什么不比那三行）见
    // tests/ChapterFixture.h 文件头。
    //
    // 置了 FANREN_WRITE_CH03_FIXTURES 时改为写出这份存档——只在第 3 章的剧情
    // 真的改了、终局本就该变的时候用。
    void settleEndingAgainstFixture(const char* fileName) {
        const fs::path fixture = fanren::test::chapterFixturePath(assetRoot(), fileName);
        if (fanren::test::environmentFlagSet(fanren::test::kWriteChapterThreeFixturesEnv)) {
            auto wrote = fanren::io::saveGame(state(), fixture.string());
            ASSERT_TRUE(wrote.ok) << wrote.error;
            std::cout << "[ch03 交接存档] 已写出 " << fixture.string() << std::endl;
            return;
        }
        ASSERT_TRUE(fs::exists(fixture))
            << "交接存档 " << fixture.string() << " 不在。它是第 4 章通关测试的起点，"
            << "用 " << fanren::test::kWriteChapterThreeFixturesEnv << "=1 跑一遍本用例生成";
        auto handedOver = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(handedOver.ok) << "交接存档读不回来：" << handedOver.error;
        const std::vector<std::string> expected =
            fanren::test::comparableSaveLines(handedOver.value);
        const std::vector<std::string> actual = fanren::test::comparableSaveLines(state());
        // 先验：两边都真的写出了东西。两份空表逐行比是恒等的。
        ASSERT_GT(expected.size(), 20u) << "交接存档写出来只有 " << expected.size() << " 行";
        ASSERT_GT(actual.size(), 20u) << "这一趟的终局写出来只有 " << actual.size() << " 行";
        const std::vector<std::string> diff =
            fanren::test::saveLineDifferences(expected, actual);
        EXPECT_TRUE(diff.empty())
            << "第 3 章这一趟的终局与交接存档 " << fileName << " 对不上——第 4 章是从那份存档起步的，"
            << "两章之间已经漂开了。第 3 章若是有意改的，按 tests/ChapterFixture.h 文件头重生成，"
            << "再跑一遍第 4 章：" << fanren::test::joinLines(diff);
    }

    Application app_;
    // 替玩家出手的那只手（tests/BattleHand.h），缺省取舍：本章没有养精丹，吃药那一条用不上。
    fanren::test::BattleHand hand_{app_};
    WorldScene world_;
    int startDay_ = 1;
};

// ---------------------------------------------------------------------------
// 通关：一趟走到底，沿途的闸门逐一验两次
// ---------------------------------------------------------------------------
TEST_F(Ch03Walkthrough, WalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    constexpr int kFirst = 0;
    GameState& s = state();

    // ======================= 节点 1 · 谷外遇狼 =======================
    // 落地就打，中间一步也不多走：谷外那条东西向的道只有两格宽（y = 14、15），
    // 而 trigger_elang 正好把两格都占了。这是关卡刻意设的门——第一场仗跳不过去。
    // 于是「这一仗之前谷外还有哪些闸拦着」只能在**打完之后**验，它们那时仍旧关着。
    ASSERT_TRUE(travelTo(kMapGuwai, kFirst));
    ASSERT_EQ(s.mapId, kMapGuwai);

    const int dayBeforeElang = s.day;
    ASSERT_TRUE(enterTrigger("trigger_elang", kFirst)) << "谷外那条道上该撞见狼";
    EXPECT_EQ(flag("ch03.elang_done"), 1) << "赢也好逃也好，这一仗算是打过了";
    EXPECT_EQ(s.day - dayBeforeElang, kElangDays) << "谷里又过了一年，配五毒水又三日";
    EXPECT_EQ(s.itemCount(kWuduId), 1) << "那一仗之后他连夜配出了第一瓶五毒水";

    // 赢与逃两条台词只能有一条演过。两条都没演 = battle() 的返回值没接上；
    // 两条都演了 = 脚本把 if/else 写成了两段顺序执行。
    const bool wolvesBeaten = spoke("ch03.elang.win1");
    const bool wolvesFled = spoke("ch03.elang.flee1");
    EXPECT_NE(wolvesBeaten, wolvesFled)
        << "谷外那一仗的胜负没能传回脚本：赢了 " << wolvesBeaten << "，退了 " << wolvesFled;

    // 防跳过：这一仗打完了，通往暗道那条路仍旧关着（那是章末才走的）。
    EXPECT_FALSE(usePortal("portal_to_andao")) << "ch03.andao_zhan 未置位，暗道不该放行";
    EXPECT_EQ(s.mapId, kMapGuwai);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
    closePanel();

    // 章末那一场同样拦着：曲魂还没入队，离谷的戏不该开演。
    ASSERT_TRUE(fireTrigger("trigger_ligu", kFirst));
    EXPECT_EQ(flag("ch03.done"), 0) << "人都还没遇齐，离什么谷";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.ligu.gate") << "拦住了就要说是为什么";

    // ======================= 节点 2 · 墨大夫归谷 =======================
    ASSERT_TRUE(travelTo(kMapShenshougu, kFirst));
    // 防跳过：墨大夫还没归谷，密室那道门是关着的。
    EXPECT_FALSE(usePortal("portal_to_ch03_mishi")) << "ch03.mo_gui_gu 未置位，密室不该放行";
    EXPECT_EQ(s.mapId, kMapShenshougu);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue");
    closePanel();

    const int dayBeforeMogui = s.day;
    ASSERT_TRUE(enterTrigger("trigger_mogui", kFirst));
    EXPECT_EQ(flag("ch03.mo_gui_gu"), 1);
    EXPECT_EQ(s.day - dayBeforeMogui, kMoguiDays);
    // 这一节的要害是**还没摊牌**：玩家此刻不该知道他是谁。
    EXPECT_EQ(flag("ch03.tanpai_done"), 0) << "归谷与摊牌是两件事，不能并成一件";

    // ======================= 节点 3 · 摊牌 =======================
    // 与谷外同一个道理：密室的入口是一条两格宽的甬道（x = 11、12），
    // 而 trigger_tanpai 把两格都占了。走进密室的第一步就是摊牌，跳不过去。
    // 换句话说，「摊牌之前不许服尸虫丸」这道闸是**地图本身**给的，不是脚本给的
    //（脚本里那句 ch03.shichong.gate 是给读档回放之类的路径兜底的）。
    ASSERT_TRUE(travelTo(kMapMishi, kFirst));
    ASSERT_EQ(s.mapId, kMapMishi);
    ASSERT_TRUE(enterTrigger("trigger_tanpai", kFirst));
    EXPECT_EQ(flag("ch03.tanpai_done"), 1) << "本章第一个转折";
    EXPECT_EQ(s.itemCount(kWuduId), 0)
        << "取第一项：交出手里那瓶五毒水。take() 的返回值这一处必须接上"
           "——一边说交了、一边背包里还留着，是第 2 章栽过两次的那种错";
    EXPECT_TRUE(spoke("ch03.tanpai.take")) << "他把那瓶东西收下了";

    // ======================= 节点 4 · 尸虫丸 =======================
    // 摊牌刚过，密室里其余三处一个也不该开（解药那一节、暗道那道门、藏书处
    // 那道门）。三条一起验：这一节的错法是「某道闸的条件写成了刚刚置上的
    // 那一个」，只验一条看不出来。第四道是备毒，它挂在药圃，验在下面那一段
    // ——去居所的路上必经药圃，顺手就验了，不必为它专程跑一趟。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_jieyao", kFirst));
    EXPECT_EQ(flag("ch03.jieyao_xuan"), 0) << "暗道那一仗还没打，解药不该拿得到";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.jieyao.gate");

    EXPECT_FALSE(usePortal("portal_to_andao")) << "ch03.beidu_done 未置位，暗道不该放行";
    EXPECT_EQ(s.mapId, kMapMishi);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue");
    closePanel();

    EXPECT_FALSE(usePortal("portal_to_cangshu")) << "ch03.shichong_wan 未置位，藏书处不该放行";
    EXPECT_EQ(s.mapId, kMapMishi);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue");
    closePanel();

    ASSERT_TRUE(fireTrigger("trigger_shichong", kFirst));
    EXPECT_EQ(flag("ch03.shichong_wan"), 1);
    // 尸虫丸是**剧情状态**，不走战斗毒（设计第 3 节）。背包里不该多出任何
    // 可解的东西，玩家也不该以为吃颗解毒丹就能摆脱他。
    EXPECT_EQ(s.itemCount(kQingduId), 0) << "此刻手上一包清毒散也没有，尸虫丸与它无关";
    // 第 2 章章末那次选择在这里回响：钱捎回家的那一条，墨居仁说的是另一句话。
    EXPECT_TRUE(spoke("ch03.shichong.money_send"))
        << "ch02.qian_quxiang == 1（钱捎回家）那一条的回响没接上";
    EXPECT_FALSE(spoke("ch03.shichong.money_keep"));

    // 备毒那一处挂在药圃（灵田东侧那道畦背），而密室去居所本来就要穿过药圃。
    // 方子还压在藏书处没到手，这一处该和密室里那三处一样拦得住——**换了张图
    // 不等于换了道闸**，挪挂点最容易碰坏的正是这一条。
    ASSERT_TRUE(travelTo(kMapYaopu, kFirst));
    ASSERT_EQ(s.mapId, kMapYaopu);
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_beidu", kFirst));
    EXPECT_EQ(flag("ch03.beidu_done"), 0) << "方子还没到手，配什么毒";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.beidu.gate");

    // ======================= 节点 5 · 三选一 =======================
    ASSERT_TRUE(travelTo(kMapJusuo, kFirst));
    ASSERT_EQ(s.mapId, kMapJusuo);

    // 居所东西两半之间只有一格宽的门洞（x = 14，y = 8 与 9），处决那一场正压在
    // 门洞上，而去西半间那两处（应对、夺舍）必得从这里穿过去。
    //
    // **穿过去不该有任何事发生。** 处决挂的是 interact：这一节的全部分量在于
    // 那一下确认键是玩家自己按的（ch03.chujue.after），路过就开演等于把它抹掉。
    // 从前这里挂的是 enter，于是这一段写成了「必先踩它一脚——这一脚此刻该什么
    // 也不发生」，把一个副作用当成了设计，还替它写了理由。留下这条断言正是为了
    // 让那条错路不能再悄悄回来。
    const MapObject chujueDoorway = objectNamed("trigger_chujue");
    ASSERT_FALSE(chujueDoorway.name.empty());
    ASSERT_EQ(chujueDoorway.property("mode"), "interact")
        << "处决那一场的 mode 变了——这一段下面的每一句都是按 interact 写的";
    app_.clearSpokenKeys();
    ASSERT_TRUE(stepOntoWithoutPumping(chujueDoorway)) << "走不到门洞上";
    EXPECT_FALSE(app_.scripts().isRunning())
        << "从门洞上走过去就把处决那一场开演了——那一下确认键必须由玩家自己按";
    EXPECT_EQ(app_.lastSpokenKey(), std::string{}) << "路过不该说任何一句话";

    // 闸门：人都还没醒，这时候按下去只该被挡回来。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_chujue", kFirst));
    EXPECT_EQ(flag("ch03.yuzitong_chujue"), 0) << "人都还没醒，处决谁去";
    EXPECT_EQ(flag("ch03.mo_siwang"), 0) << "墨居仁此刻还好端端地活着";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.shenxing.gate");

    // 防跳过：识海那一场要等解药那一节之后。它挂的是 enter——走到床边就发生，
    // 玩家按不按键都一样，所以这里也用踩的。once 的判据看的是 set_flag 的值，
    // 脚本这一趟只播一句 gate 就 return、一个旗标也没置，所以踩过一次不会把
    // 触发器烧掉，节点 10 照样起得来（map_spec 4.4）。
    ASSERT_TRUE(enterTrigger("trigger_duoshe", kFirst));
    EXPECT_EQ(flag("ch03.shihai_done"), 0) << "解药都还没拿到，谈什么夺舍";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.duoshe.gate");

    const int dayBeforeYingdui = s.day;
    ASSERT_TRUE(fireTrigger("trigger_yingdui", kFirst));
    EXPECT_EQ(flag("ch03.yingdui_xuan"), 1) << "取第一项：表面顺从";
    EXPECT_EQ(s.day - dayBeforeYingdui, kYingduiDays);

    // ======================= 节点 6 · 偷秘籍 =======================
    ASSERT_TRUE(travelTo(kMapCangshu, kFirst));
    ASSERT_EQ(s.mapId, kMapCangshu);

    const int dayBeforeMiji = s.day;
    ASSERT_TRUE(fireTrigger("trigger_miji", kFirst));
    EXPECT_EQ(flag("ch03.miji_done"), 1);
    EXPECT_EQ(flag("ch03.miji_juewu"), 1) << "取第一项：当场就看";
    EXPECT_EQ(s.itemCount(kShouzhaId), 1) << "偷出来的那叠手札该进背包";
    EXPECT_EQ(s.day - dayBeforeMiji, kMijiDays);

    // ======================= 节点 7 · 备毒 =======================
    ASSERT_TRUE(travelTo(kMapMishi, kFirst));

    // 防跳过：暗道那道门要等备毒之后。
    EXPECT_FALSE(usePortal("portal_to_andao")) << "ch03.beidu_done 未置位，暗道不该放行";
    EXPECT_EQ(s.mapId, kMapMishi);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue");
    closePanel();

    // 解药那一节同样：暗道那一仗还没打。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_jieyao", kFirst));
    EXPECT_EQ(flag("ch03.jieyao_xuan"), 0);
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.jieyao.gate");

    // 备毒本身在药圃：方子到手之后他还得走回畦头去挖那一丛土菇花，
    // 再原路回密室才推得开暗道那道门。这两趟路正是这个挂点的代价，
    // 也是它与「药圃的戏挂在石屋里」那个老样子的全部分别。
    ASSERT_TRUE(travelTo(kMapYaopu, kFirst));
    ASSERT_EQ(s.mapId, kMapYaopu);

    const int dayBeforeBeidu = s.day;
    const int purseBeforeBeidu = s.itemCount(kMoneyId);
    ASSERT_GE(purseBeforeBeidu, kSwordPrice)
        << "第 2 章章末留下的 " << kStartPurse << " 块必须够买那把 " << kSwordPrice
        << " 块的剑，否则下面这一整条分支根本走不到";
    ASSERT_TRUE(fireTrigger("trigger_beidu", kFirst));
    EXPECT_EQ(flag("ch03.beidu_done"), 1) << "用毒系统在这一节交到玩家手上";
    EXPECT_EQ(s.day - dayBeforeBeidu, kBeiduDays);
    // 取第一项：只挖两株、只配一副。教学战二要用的三样东西这一刻齐了。
    EXPECT_EQ(s.itemCount(kQiduId), 1) << "七毒水一瓶——章末处决要用的正是它";
    EXPECT_EQ(s.itemCount(kShixinId), 2) << "蚀心散两包：暗道那只僵兽砍不动，得靠它";
    EXPECT_EQ(s.itemCount(kQingduId), 3) << "清毒散三包：那东西反过来也会给玩家上毒";
    EXPECT_EQ(s.itemCount(kTuguId), 1) << "挖回来两株，配掉一株";
    EXPECT_EQ(s.itemCount(kSwordId), 1) << "三十块买下的玉带短剑";
    EXPECT_EQ(purseBeforeBeidu - s.itemCount(kMoneyId), kSwordPrice)
        << "剑钱必须真的从钱袋里出去：take() 报了成功就得扣，这一处第 2 章白送过 167 块";

    // ======================= 节点 8 · 云翅鸟 + 暗道那一仗 =======================
    ASSERT_TRUE(travelTo(kMapAndao, kFirst));
    ASSERT_EQ(s.mapId, kMapAndao);

    // 防跳过：线索还没浮出，那一仗不该开打。这一条尤其要验——它是踏入型的，
    // 而暗道是纵向的一条道，玩家往深处走必然会经过它。
    app_.clearSpokenKeys();
    ASSERT_TRUE(enterTrigger("trigger_andao_zhan", kFirst));
    EXPECT_EQ(flag("ch03.andao_zhan"), 0) << "还不知道笼子里关的是什么，这一仗不该开打";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.andao.gate");

    // 章末那两节同样拦着。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_tiezhe", kFirst));
    EXPECT_EQ(flag("ch03.tiezhe_zhi"), 0);
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.tiezhe.gate");
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_quhun", kFirst));
    EXPECT_EQ(flag("ch03.quhun_rudui"), 0);
    EXPECT_TRUE(s.party.empty()) << "闸门拦住时队伍里不该先多出一个人";
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.quhun.gate");

    const int dayBeforeYunchi = s.day;
    ASSERT_TRUE(fireTrigger("trigger_yunchi", kFirst));
    EXPECT_EQ(flag("ch03.yuzitong_lu"), 1) << "余子童的线索浮出水面";
    EXPECT_EQ(s.day - dayBeforeYunchi, kYunchiDays);

    const int dayBeforeAndao = s.day;
    const int qingduBeforeAndao = s.itemCount(kQingduId);
    const int shixinBeforeAndao = s.itemCount(kShixinId);
    ASSERT_TRUE(enterTrigger("trigger_andao_zhan", kFirst));
    EXPECT_EQ(flag("ch03.andao_zhan"), 1) << "设计写明这一场可败，输了照样往下走";
    EXPECT_EQ(s.day - dayBeforeAndao, kAndaoDays);
    // 输赢两条台词同样只能有一条。而输的那一条要真的吃掉两包清毒散——
    // 脚本在那里特地看了 take() 的返回值，这一条钉的就是那个。
    const bool beastBeaten = spoke("ch03.andao.win1");
    const bool beastWon = spoke("ch03.andao.lose1");
    EXPECT_NE(beastBeaten, beastWon) << "暗道那一仗的胜负没能传回脚本";
    // 这一场可败，但**不该必败**：会用毒的那只手得赢得下来，而且赢法是「砍不动就下毒」。
    EXPECT_TRUE(beastBeaten) << "会撒毒、会蓄劲的那只手也赢不下暗道，这一课成了必败";
    EXPECT_EQ(shixinBeforeAndao - s.itemCount(kShixinId), kAndaoPoisonDoses)
        << "赢下暗道撒掉的蚀心散不是两包——「两包正好破一次势」那笔账对不上了";
    if (beastWon) {
        EXPECT_EQ(qingduBeforeAndao - s.itemCount(kQingduId), 2)
            << "输的那一条说他吃了两包清毒散压毒，背包里就得真的少两包";
        EXPECT_TRUE(spoke("ch03.andao.lose2"));
    } else {
        EXPECT_EQ(s.itemCount(kQingduId), qingduBeforeAndao) << "赢的那一条不吃药";
    }
    // 无论输赢都要接上的那一条：他从巨汉的脚步声里推出了什么。
    EXPECT_TRUE(spoke("ch03.andao.giant")) << "这条推论到节点 12 掀开帽兜时才结账";

    // ======================= 节点 9 · 解药 =======================
    ASSERT_TRUE(travelTo(kMapMishi, kFirst));
    const int dayBeforeJieyao = s.day;
    ASSERT_TRUE(fireTrigger("trigger_jieyao", kFirst));
    EXPECT_EQ(flag("ch03.jieyao_xuan"), 1) << "取第一项：当场吞下去";
    EXPECT_EQ(s.day - dayBeforeJieyao, kJieyaoDays) << "两条分支都是 77 + 22 日";
    EXPECT_TRUE(spoke("ch03.jieyao.check_sword")) << "身上有剑那一句";

    // ======================= 节点 10 · 识海之战 =======================
    // 先验：这一场的战果之所以说明问题，全靠原著那两条体积关系。
    // duoshe.lua 在「不是敌人逃走」那一支里把 33 写死了，于是**光看旗标分不出**
    // 「它带着伤跑了」与「被当场打死了」——前者是这一章的转折，后者是硬约束
    // 被推翻。所以先把那两条体积关系钉在数据上：第一团比韩立小（吞得下），
    // 第二团比韩立大（这是它能脱开逃走的唯一理由，见 devourCanBreakAway）。
    // 少了这一组，下面那条 33 ≤ bitten ≤ 50 会在一场「全歼」上照样绿。
    const fanren::core::RoleTemplate* soulRole = app_.data().findRole("mo_juren_yuanshen");
    const fanren::core::RoleTemplate* yuRole = app_.data().findRole("yu_zitong");
    ASSERT_NE(soulRole, nullptr);
    ASSERT_NE(yuRole, nullptr);
    EXPECT_LT(soulRole->maxHp, fanren::core::battle::kDevourPlayerVolume)
        << "第一团该比韩立小好几倍（拇指大），否则「靠体积轻易吞掉」不成立";
    EXPECT_GT(yuRole->maxHp, fanren::core::battle::kDevourPlayerVolume)
        << "第二团该比韩立大一圈有余，否则它脱不开、跑不了，这一场会打成全歼";

    ASSERT_TRUE(travelTo(kMapJusuo, kFirst));
    const int dayBeforeDuoshe = s.day;
    // 踩上去就发生，玩家一个键也没按——这一节的操作感就是这么定义的。
    ASSERT_TRUE(enterTrigger("trigger_duoshe", kFirst));
    EXPECT_EQ(flag("ch03.shihai_done"), 1);
    EXPECT_EQ(s.day - dayBeforeDuoshe, kDuosheDays);
    EXPECT_FALSE(app_.quitRequested())
        << "识海那一场走到了 game_over()：那是 how == \"lost\" 才该走的一条，"
           "而第二团光球是**必然逃脱**的，不是把韩立打败";
    // 第二场的战果按咬下的比例计。只断言「赢了没有」会把「敌人带着伤跑了」
    // 读成「韩立输了」——本章后面全押在这个分别上。
    const int bitten = flag("ch03.shihai_yaoxia");
    EXPECT_GE(bitten, 33) << "原著是咬下三分之一，实测 " << bitten;
    EXPECT_LE(bitten, 50) << "咬下的比例失控了，实测 " << bitten;
    EXPECT_TRUE(spoke("ch03.duoshe.flee1")) << "那一团带着伤跑了";
    EXPECT_FALSE(spoke("ch03.duoshe.lost1")) << "韩立没有输";

    // ======================= 节点 11 · 身醒敌亡；处决 =======================
    ASSERT_TRUE(travelTo(kMapJusuo, kFirst));
    const int poisonBeforeChujue = s.itemCount(kQiduId);
    ASSERT_EQ(poisonBeforeChujue, 1) << "那一瓶七毒水从节点 7 一直留到这里";
    // 这一趟他是**自己走过去按的**。上一节（夺舍）用的是 enterTrigger，
    // 这一节用 fireTrigger，两行并排摆着就是本章那一处对比在测试里的样子。
    ASSERT_TRUE(fireTrigger("trigger_chujue", kFirst));
    // 两个旗标**分别**断言。合成一条（「两个都是 1」）就把这一章的要害抹掉了；
    // 它们各自的来路由 TheTwoKillsComeFromDifferentPlaces 逐句钉住。
    EXPECT_EQ(flag("ch03.mo_siwang"), 1) << "墨居仁暴毙——元神被吞，不是韩立醒着动手";
    EXPECT_EQ(flag("ch03.yuzitong_chujue"), 1) << "余子童被处决——这一件是玩家主动做的";
    EXPECT_EQ(s.itemCount(kQiduId), 0) << "处决用掉了那一瓶七毒水";
    EXPECT_TRUE(spoke("ch03.shenxing.notme")) << "「不是我杀的」那一句是第一种杀的落点";
    EXPECT_TRUE(spoke("ch03.chujue.why1")) << "取第一项：问他为什么";

    // ======================= 节点 12 · 曲魂；三大铁则；离谷 =======================
    ASSERT_TRUE(travelTo(kMapAndao, kFirst));

    // 防跳过：铁则还没交代，曲魂不该先入队。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_quhun", kFirst));
    EXPECT_EQ(flag("ch03.quhun_rudui"), 0);
    EXPECT_TRUE(s.party.empty());
    EXPECT_EQ(app_.lastSpokenKey(), "ch03.quhun.gate");

    ASSERT_TRUE(fireTrigger("trigger_tiezhe", kFirst));
    EXPECT_EQ(flag("ch03.tiezhe_zhi"), 1) << "三大铁则在这一节交代（硬约束 #9）";
    EXPECT_EQ(s.itemCount(kJiashuId), 1) << "从尸身上搜出来的那封家书";

    ASSERT_TRUE(fireTrigger("trigger_quhun", kFirst));
    EXPECT_EQ(flag("ch03.quhun_rudui"), 1);
    ASSERT_EQ(s.party.size(), 1u) << "第一个可控同伴";
    EXPECT_EQ(s.party[0].roleId, kCompanion)
        << "入队的该是那具无魂躯壳 qu_hun，不是原著后期那个身外化身 " << kCompanionLate;
    EXPECT_NE(s.party[0].roleId, kCompanionLate);
    EXPECT_TRUE(s.party[0].active);
    EXPECT_EQ(s.party[0].hp, -1) << "刚入队、还没打过仗，按模板满血";
    EXPECT_TRUE(spoke("ch03.quhun.follow")) << "party.add 的返回值要接上";

    // 章末：离谷。
    ASSERT_TRUE(travelTo(kMapGuwai, kFirst));
    ASSERT_EQ(s.mapId, kMapGuwai) << "暗道→谷外那道门这一刻该开了";
    const int dayBeforeLigu = s.day;
    ASSERT_TRUE(fireTrigger("trigger_ligu", kFirst));
    EXPECT_EQ(flag("ch03.done"), 1) << "章末：确实抵达终点";
    EXPECT_EQ(s.day - dayBeforeLigu, kLiguDays);

    // ---- 章末的机制状态 ----
    EndBag want;
    want.money = kStartPurse - kSwordPrice;
    want.tugu = 1;
    want.shixin = 2 - kAndaoPoisonDoses;   // 节点 7 配了两包，暗道里撒光
    want.qingdu = beastWon ? 1 : 3;
    want.wudu = 0;
    want.qidu = 0;
    want.sword = true;
    const BagVerdict bag = chapterEndBag(s, want);
    EXPECT_TRUE(bag.ok) << bag.why;

    // 十二个节点的推进旗标一个不落。
    for (const char* gate : {"ch03.elang_done", "ch03.mo_gui_gu", "ch03.tanpai_done",
                             "ch03.shichong_wan", "ch03.miji_done", "ch03.beidu_done",
                             "ch03.yuzitong_lu", "ch03.andao_zhan", "ch03.shihai_done",
                             "ch03.mo_siwang", "ch03.yuzitong_chujue", "ch03.tiezhe_zhi",
                             "ch03.quhun_rudui", "ch03.done"}) {
        EXPECT_EQ(flag(gate), 1) << gate << " 没置上，这一节其实没走过去";
    }
    EXPECT_GE(flag("ch03.yingdui_xuan"), 1);
    EXPECT_GE(flag("ch03.jieyao_xuan"), 1);

    EXPECT_EQ(s.day - startDay_, kChapterDays)
        << "整章的天数该等于各脚本 advance_days 之和；对不上就是某一处跳时被吞了或多走了";

    // 交到第 4 章手里的就是这一份（每一处选择都取第一项的那一侧）。
    settleEndingAgainstFixture(fanren::test::kChapterThreeEndingFirst);

    std::cout << "\n[ch03 通关实测] 第 " << s.day << " 日（共 " << (s.day - startDay_) << " 日）　"
              << "咬下 " << flag("ch03.shihai_yaoxia") << "%　"
              << "队伍 " << s.party.size() << " 人　"
              << "钱袋 " << s.itemCount(kMoneyId) << "　"
              << "谷外那一仗" << (wolvesBeaten ? "打赢了" : "退了") << "　"
              << "暗道那一仗" << (beastBeaten ? "打赢了" : "没打完") << std::endl;
}

// ---------------------------------------------------------------------------
// 另一侧的选择同样到得了终点
// ---------------------------------------------------------------------------
// 第 1 章栽过「某结局 0/81 不可达」，两次人工点过都没拦住。上面那一趟取的是
// 每个选择的第一项，这一趟全取第二项。本章的分支比第 2 章有牙：节点 3 取第二项
// 是**不交那瓶五毒水**，于是章末的账整个不一样。
TEST_F(Ch03Walkthrough, TheOtherSideOfEveryChoiceAlsoReachesTheEnd) {
    constexpr int kSecond = 1;
    GameState& s = state();

    ASSERT_TRUE(nodeElang(kSecond));
    ASSERT_TRUE(nodeMogui(kSecond));

    ASSERT_TRUE(nodeTanpai(kSecond));
    EXPECT_EQ(flag("ch03.tanpai_done"), 1);
    // 取第二项：摊开的手心里什么也没有。那瓶五毒水留在了自己身上。
    EXPECT_EQ(s.itemCount(kWuduId), 1) << "没交出去就该还在——两条分支的账必须真的不同";
    EXPECT_TRUE(spoke("ch03.tanpai.gone"));
    EXPECT_FALSE(spoke("ch03.tanpai.take"));

    ASSERT_TRUE(nodeShichong(kSecond));
    ASSERT_TRUE(nodeYingdui(kSecond));
    EXPECT_EQ(flag("ch03.yingdui_xuan"), 2) << "取第二项：拖";

    ASSERT_TRUE(nodeMiji(kSecond));
    EXPECT_EQ(flag("ch03.miji_juewu"), 2) << "取第二项：先藏起来";

    ASSERT_TRUE(nodeBeidu(kSecond));
    // 取第二项：多挖两株、多配一副。数目翻倍，土菇花照样剩一半。
    EXPECT_EQ(s.itemCount(kQiduId), 2);
    EXPECT_EQ(s.itemCount(kShixinId), 4);
    EXPECT_EQ(s.itemCount(kQingduId), 3) << "清毒散是定数三包，与挖几株无关";
    EXPECT_EQ(s.itemCount(kTuguId), 2) << "挖回来四株，配掉两株";

    ASSERT_TRUE(nodeYunchi(kSecond));
    ASSERT_TRUE(nodeAndaoZhan(kSecond));
    // 当场记下来，不要等到章末再问：台词本只留最近的一百多条。
    const bool beastWon = spoke("ch03.andao.lose1");
    ASSERT_TRUE(nodeJieyao(kSecond));
    EXPECT_EQ(flag("ch03.jieyao_xuan"), 2) << "取第二项：先拿别的试一试";

    ASSERT_TRUE(nodeDuoshe(kSecond));
    EXPECT_EQ(flag("ch03.shihai_done"), 1);
    EXPECT_GE(flag("ch03.shihai_yaoxia"), 33) << "识海那两场与选了什么无关，走的是同一条";
    EXPECT_LE(flag("ch03.shihai_yaoxia"), 50);

    ASSERT_TRUE(nodeChujue(kSecond));
    EXPECT_EQ(flag("ch03.mo_siwang"), 1);
    EXPECT_EQ(flag("ch03.yuzitong_chujue"), 1);
    EXPECT_TRUE(spoke("ch03.chujue.giant1")) << "取第二项：先问那个巨汉是什么";

    ASSERT_TRUE(nodeTiezhe(kSecond));
    ASSERT_TRUE(nodeQuhun(kSecond));
    ASSERT_TRUE(nodeLigu(kSecond));
    EXPECT_EQ(flag("ch03.done"), 1) << "另一侧的六次选择同样到得了章末";

    // 章末的账与上一趟确实不同：五毒水没交、毒配了双份。
    EndBag want;
    want.money = kStartPurse - kSwordPrice;
    want.tugu = 2;
    want.shixin = 4 - kAndaoPoisonDoses;   // 配了双份，暗道里撒掉两包
    want.qingdu = beastWon ? 1 : 3;
    want.wudu = 1;   // 这一条正是两趟的分水岭
    want.qidu = 1;   // 配了两瓶，处决用掉一瓶
    want.sword = true;
    const BagVerdict bag = chapterEndBag(s, want);
    EXPECT_TRUE(bag.ok) << bag.why;

    // 天数与选哪一项无关：节点 6 与节点 9 都把多花的日子从末尾那次里扣回去了。
    EXPECT_EQ(s.day - startDay_, kChapterDays)
        << "两条分支的天数该一样；对不上说明某一处的 elapsed 没扣干净";

    // 另一侧的终局同样交给第 4 章（第 4 章的通关测试两份都读）。
    settleEndingAgainstFixture(fanren::test::kChapterThreeEndingSecond);
}

// ---------------------------------------------------------------------------
// 两种杀：分别钉住，不许合成一条
// ---------------------------------------------------------------------------
// 大纲的要害注解：墨大夫**不是**被韩立主动反杀的，是夺舍过程中元神反噬致其暴毙；
// 余子童才是事后被主动处决的。两件事在玩家手上的操作感必须完全不同。
//
// 两件事同在 chujue.lua 里，所以「它们来自不同的脚本」这种钉法在这里根本不成立。
// 这一条把整场戏逐句录下来，在每一句弹出来的那一刻采一次样，再拿一份具名判据
// 去问：墨居仁是在玩家被问到任何事之前就死的吗？余子童是在玩家做了选择、
// 并且真的把那瓶七毒水灌下去之后才死的吗？
//
// ---------------------------------------------------------------------------
// 这一条从前漏掉的那一半（第 3 章校对 HIGH-1）
// ---------------------------------------------------------------------------
// 「两种杀的操作感必须不同」是两件事：**戏里发生的先后**，与**玩家是怎么让它
// 开始的**。twoKillsVerdict 管的自始至终只有前者——它读的全是 chujue.lua 内部
// 的 trace。后者一个字也没验，于是这一条用例顶着「两种杀」的名字，对本章唯一的
// 道德设计点视而不见；更糟的是它当时用 stepOntoWithoutPumping 驱动处决，并断言
// 「踩上去该起脚本」——**恰好把主动那一半钉成了被动的**。
//
// 现在两半都在这里：
//   · 被动那一半：playUpToTheExecution 里的节点 10 走 enterTrigger（踩上去就演）；
//     下面再直接断言 trigger_duoshe 的 mode 确实是 enter。
//   · 主动那一半：处决先踩一脚**要求什么也不发生**，再面朝它按确认才起脚本。
// mode 属性本身另由 tests/Ch03TriggerModeTests.cpp 钉住，那一条不经过走位。
TEST_F(Ch03Walkthrough, TheTwoKillsComeFromDifferentPlaces) {
    constexpr int kFirst = 0;
    playUpToTheExecution(kFirst);
    GameState& s = state();

    KillEvidence evidence;
    evidence.poisonBefore = s.itemCount(kQiduId);
    ASSERT_EQ(evidence.poisonBefore, 1) << "那一瓶七毒水必须还在，否则下面那条判据是空转的";

    // ---- 第一种杀：他睡着的时候办的。玩家没有按下任何一个键 ----
    // 节点 10 刚刚由 playUpToTheExecution 用 enterTrigger 走完（走到床边就发生）。
    // 先验一句：那一节挂的确实是踏入型，否则「踩着就演完了」只是驱动函数的口径，
    // 不是这个游戏的行为。
    const MapObject duoshe = objectNamed("trigger_duoshe");
    ASSERT_FALSE(duoshe.name.empty()) << "居所上没有夺舍那一处挂点";
    EXPECT_EQ(duoshe.property("mode"), "enter")
        << "夺舍挂成了要玩家自己按的：那一节的失重感一半靠没有 choice()，"
           "另一半就靠这里——他连「要不要开始」都不该有得选";
    EXPECT_EQ(flag("ch03.shihai_done"), 1) << "先验：夺舍那一场确实演过了";

    // ---- 第二种杀：走过去、面朝它、自己按下 ----
    const MapObject chujue = objectNamed("trigger_chujue");
    ASSERT_FALSE(chujue.name.empty());
    EXPECT_EQ(chujue.property("mode"), "interact")
        << "处决挂成了走过去就自动开演：那一下拇指就不是玩家按的了";

    // 先踩一脚：**什么也不该发生**。这一句是两种杀的分界线本身——
    // 少了它，把 mode 改回 enter 之后下面那段照样能绿（踩上去脚本也会起）。
    app_.clearSpokenKeys();
    ASSERT_TRUE(stepOntoWithoutPumping(chujue)) << "走不到处决那一格上";
    ASSERT_FALSE(app_.scripts().isRunning())
        << "从它上头走过去就把处决开演了——第二种杀必须由玩家自己按那一下";
    ASSERT_TRUE(app_.lastSpokenKey().empty()) << "路过不该说任何一句话";

    // 再按确认：这一下才该起脚本。不用 fireTrigger，那个会把整场一口气演完。
    ASSERT_TRUE(interactWithoutPumping(chujue)) << "面朝它按确认，点不着";
    ASSERT_TRUE(app_.scripts().isRunning()) << "他自己按下的那一下该起脚本";

    traceScripts(kFirst, evidence.steps);
    evidence.finalMoSiwang = flag("ch03.mo_siwang");
    evidence.finalChujue = flag("ch03.yuzitong_chujue");
    evidence.poisonAfter = s.itemCount(kQiduId);

    const KillVerdict verdict = twoKillsVerdict(evidence);
    EXPECT_TRUE(verdict.ok) << verdict.why;

    // 判据之外再钉一条人眼看得懂的：第一种杀的落点是那句「不是我」。
    // 下标而不是迭代器——迭代器塞进 gtest 的宏里，失败时打印出来的是一串没人
    // 读得懂的字节。
    int flipIndex = -1;
    for (const SceneStep& step : evidence.steps) {
        if (step.moSiwang == 1) {
            flipIndex = step.index;
            break;
        }
    }
    ASSERT_GT(flipIndex, 0) << "ch03.mo_siwang 不是在「身醒」那一段中途翻的";
    EXPECT_EQ(evidence.steps[static_cast<std::size_t>(flipIndex) - 1].key, "ch03.shenxing.notme")
        << "墨居仁那条旗标该紧接着「不是我」那一句置上";

    std::cout << "\n[ch03 两种杀实测] 全场 " << evidence.steps.size() << " 步，"
              << "墨居仁在第 " << flipIndex << " 步之前已死（此前玩家一次也没被问过），"
              << "七毒水 " << evidence.poisonBefore << " → " << evidence.poisonAfter
              << std::endl;
}

// ---------------------------------------------------------------------------
// 同伴持久化：曲魂入队后存档往返仍在队里
// ---------------------------------------------------------------------------
// 设计第 9 节验收第 4 条。tests/Ch03PartyTests.cpp 已经钉过 saveGame/loadGame
// 对 party 每一个字段的往返，那是规则层；这一条钉的是**玩家实际走到那一步之后
// 存的那份档**——队伍是脚本在暗道里 party.add 进去的，气血是 -1（还没打过仗），
// 而存档版本刚从 2 升到 3。三件事任何一件断了，读回来的都是一支空队伍，
// 而空队伍在游戏里长得就像「这个功能还没做」。
TEST_F(Ch03Walkthrough, TheCompanionSurvivesASaveRoundTrip) {
    constexpr int kFirst = 0;
    playUpToTheExecution(kFirst);
    ASSERT_TRUE(nodeChujue(kFirst));
    ASSERT_TRUE(nodeTiezhe(kFirst));
    ASSERT_TRUE(nodeQuhun(kFirst));
    ASSERT_TRUE(nodeLigu(kFirst));
    ASSERT_EQ(flag("ch03.done"), 1);

    GameState& s = state();
    ASSERT_EQ(s.party.size(), 1u);
    ASSERT_EQ(s.party[0].roleId, kCompanion);

    // 曲魂是本章这一具傀儡，不是原著后期那个身外化身。属性照 data/roles 走。
    const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(role->realm, Realm::Mortal) << "无魂躯壳仍是凡人之身";
    EXPECT_EQ(role->maxHp, 55);
    EXPECT_EQ(role->attack, 9);
    EXPECT_EQ(role->defence, 8);
    EXPECT_EQ(role->speed, 2);
    ASSERT_NE(app_.data().findRole(kCompanionLate), nullptr)
        << "身外化身那一条也在 data 里；正因为两条都在，入错了才看不出来";

    const fs::path save = fanren::test::uniqueTempPath("fanren_ch03_companion", ".json");
    auto wrote = fanren::io::saveGame(s, save.string());
    ASSERT_TRUE(wrote.ok) << wrote.error;
    auto read = fanren::io::loadGame(save.string());
    ASSERT_TRUE(read.ok) << read.error;
    std::error_code ec;
    fs::remove(save, ec);

    const GameState& back = read.value;
    ASSERT_EQ(back.party.size(), 1u) << "存了一趟档，队伍空了——这正是验收第 4 条要挡的那件事";
    EXPECT_EQ(back.party[0].roleId, kCompanion);
    EXPECT_NE(back.party[0].roleId, kCompanionLate);
    EXPECT_EQ(back.party[0].hp, -1) << "还没打过仗，hp 仍是「按模板满血」的 -1";
    EXPECT_TRUE(back.party[0].active);
    // 顺带把章末那几条旗标也验一遍：队伍回来了而剧情没回来，同样是半个存档。
    EXPECT_EQ(back.flag("ch03.done"), 1);
    EXPECT_EQ(back.flag("ch03.quhun_rudui"), 1);
    EXPECT_EQ(back.flag("ch03.mo_siwang"), 1);
    EXPECT_EQ(back.flag("ch03.yuzitong_chujue"), 1);
    EXPECT_EQ(back.day, state().day);
}

// ---------------------------------------------------------------------------
// 判据自检（一）：证明章末那组账目断言真的有牙
// ---------------------------------------------------------------------------
// 「背包里找不到那瓶七毒水」在背包整个是空的时候照样通过。凡这种形状的断言都
// 要另配一条用例，先证明判据本身咬得动人，再拿它去验真实的运行结果。
// 这个项目最惨的一次事故正是校验器的正则被 heredoc 吃掉转义、从此永远报通过
// 而无人发现——因为没人做过负向验证。
EndBag firstRunExpectation() {
    EndBag want;
    want.money = kStartPurse - kSwordPrice;
    want.tugu = 1;
    want.shixin = 2;
    want.qingdu = 3;
    want.wudu = 0;
    want.qidu = 0;
    want.sword = true;
    return want;
}

GameState cleanChapterEndBag() {
    GameState s;
    s.addItem(kHerbId, kStartSeedlings, 0);
    s.addItem(kHerb2Id, kStartZishen, 0);
    s.addItem(kShixinId, 2, 0);
    s.addItem(kQingduId, 3, 0);
    s.addItem(kTuguId, 1, 0);
    s.addItem(kSwordId, 1, 0);
    s.addItem(kShouzhaId, 1, 0);
    s.addItem(kJiashuId, 1, 0);
    s.addItem(kMoneyId, kStartPurse - kSwordPrice, 0);
    return s;
}

TEST(Ch03EndState, TheBagVerdictAcceptsACleanChapterEnd) {
    const GameState clean = cleanChapterEndBag();
    const BagVerdict verdict = chapterEndBag(clean, firstRunExpectation());
    EXPECT_TRUE(verdict.ok) << verdict.why;
}

TEST(Ch03EndState, TheBagVerdictCatchesPoisonThatWasNeverPouredOut) {
    // 处决那一场的台词说他把那瓶七毒水灌了下去。脚本里若少写一行 take()，
    // 旗标照置、台词照说，而瓶子原封不动留在背包里——这正是第 2 章认定的
    // 要害类别：台词断言了某个状态，而引擎里不是那样。
    GameState broken = cleanChapterEndBag();
    broken.addItem(kQiduId, 1, 0);
    const BagVerdict caught = chapterEndBag(broken, firstRunExpectation());
    EXPECT_FALSE(caught.ok) << "七毒水还在背包里，判据却说没事——它没有牙";
    EXPECT_NE(caught.why.find("七毒水"), std::string::npos) << caught.why;
}

TEST(Ch03EndState, TheBagVerdictCatchesAPotionHandedOverThatWasNeverTaken) {
    // 同一个形状的另一头：节点 3 把五毒水交出去那一条。
    GameState broken = cleanChapterEndBag();
    broken.addItem(kWuduId, 1, 0);
    const BagVerdict caught = chapterEndBag(broken, firstRunExpectation());
    EXPECT_FALSE(caught.ok) << "五毒水说交出去了却还在身上，判据却说没事";
    EXPECT_NE(caught.why.find("五毒水"), std::string::npos) << caught.why;
}

TEST(Ch03EndState, TheBagVerdictRefusesAnEmptyBagInsteadOfWavingItThrough) {
    // 空背包能同时满足「找不到七毒水」与「找不到五毒水」。判据必须先咬住
    // 「该在的东西还在」，否则一条把背包清空的缺陷会让上面两条集体变成装饰。
    const GameState empty;
    const BagVerdict verdict = chapterEndBag(empty, firstRunExpectation());
    EXPECT_FALSE(verdict.ok) << "空背包也算通关，那这组断言等于没写";
    EXPECT_EQ(verdict.why.find("七毒水"), std::string::npos)
        << "空背包该栽在「该在的东西不在」这一关，而不是「该没的东西没了」：" << verdict.why;
}

TEST(Ch03EndState, TheBagVerdictCatchesASwordThatWasPaidForButNeverDelivered) {
    // 节点 7 扣了 30 块却没给剑，是 take() 看了返回值而 give() 漏写的长相。
    GameState broken = cleanChapterEndBag();
    ASSERT_TRUE(broken.removeItem(kSwordId, 1));
    const BagVerdict caught = chapterEndBag(broken, firstRunExpectation());
    EXPECT_FALSE(caught.ok) << "钱扣了、剑没给，判据却说没事";
    EXPECT_NE(caught.why.find("玉带短剑"), std::string::npos) << caught.why;
}

TEST(Ch03EndState, TheBagVerdictCatchesTheTwoStoryPapersGoingMissing) {
    // 偷来的手札与搜出来的家书是节点 6、12 真的发生过的物证。
    // 只看旗标的话，give() 漏写一行完全看不出来。
    GameState noLetter = cleanChapterEndBag();
    ASSERT_TRUE(noLetter.removeItem(kShouzhaId, 1));
    EXPECT_FALSE(chapterEndBag(noLetter, firstRunExpectation()).ok) << "手札没了也算通关？";

    GameState noHomeLetter = cleanChapterEndBag();
    ASSERT_TRUE(noHomeLetter.removeItem(kJiashuId, 1));
    EXPECT_FALSE(chapterEndBag(noHomeLetter, firstRunExpectation()).ok) << "家书没了也算通关？";
}

// ---------------------------------------------------------------------------
// 判据自检（二）：证明「两种杀」那条判据分得开两件事
// ---------------------------------------------------------------------------
// 这一条比上面几条更要紧。twoKillsVerdict 的全部价值就在于它拒绝把两件事
// 合成一条，而「它真的拒绝得了吗」只有喂坏样本才知道。下面每一条坏样本都是
// 一种实际写得出来的错法。
KillEvidence faithfulExecutionScene() {
    KillEvidence e;
    e.poisonBefore = 1;
    e.poisonAfter = 0;
    e.finalMoSiwang = 1;
    e.finalChujue = 1;
    int n = 0;
    // 身醒那一段：一句玩家的话也没有，旗标在「不是我」之后翻。
    for (const char* key : {"ch03.shenxing.cold", "ch03.shenxing.eye", "ch03.shenxing.first",
                            "ch03.shenxing.wait", "ch03.shenxing.check", "ch03.shenxing.dead",
                            "ch03.shenxing.notme"}) {
        e.steps.push_back(SceneStep{n++, key, false, 0, 0, 1});
    }
    e.steps.push_back(SceneStep{n++, "ch03.shenxing.paper", false, 1, 0, 1});
    e.steps.push_back(SceneStep{n++, "ch03.chujue.name", false, 1, 0, 1});
    e.steps.push_back(SceneStep{n++, std::string{}, true, 1, 0, 1});   // 玩家被问了话
    e.steps.push_back(SceneStep{n++, "ch03.chujue.door", false, 1, 0, 0});   // 毒灌下去了
    e.steps.push_back(SceneStep{n++, "ch03.chujue.end", false, 1, 0, 0});
    return e;
}

TEST(Ch03KillTrace, TheVerdictAcceptsTheFaithfulScene) {
    const KillVerdict verdict = twoKillsVerdict(faithfulExecutionScene());
    EXPECT_TRUE(verdict.ok) << verdict.why;
}

TEST(Ch03KillTrace, TheVerdictCatchesMoJurenDyingBecauseThePlayerChoseIt) {
    // 最要命的那种改法：把墨居仁之死挪到玩家做过选择之后。旗标照样两个都是 1，
    // 台词也一字不改，但这一章的道德分量整个没了——他就成了被玩家杀的。
    KillEvidence broken = faithfulExecutionScene();
    broken.steps.insert(broken.steps.begin() + 3, SceneStep{3, std::string{}, true, 0, 0, 1});
    const KillVerdict caught = twoKillsVerdict(broken);
    EXPECT_FALSE(caught.ok) << "玩家先做了选择、墨居仁才死，判据却说没事";
    EXPECT_NE(caught.why.find("玩家"), std::string::npos) << caught.why;
}

TEST(Ch03KillTrace, TheVerdictCatchesTheTwoFlagsBeingSetTogether) {
    // 「两个旗标都置上了」这条合成断言的实际长相：一步之内一起翻。
    KillEvidence broken = faithfulExecutionScene();
    for (SceneStep& step : broken.steps) {
        if (step.moSiwang == 1) step.chujue = 1;
    }
    const KillVerdict caught = twoKillsVerdict(broken);
    EXPECT_FALSE(caught.ok) << "两件事在同一步里一起成立，判据却说没事";
}

TEST(Ch03KillTrace, TheVerdictCatchesAnExecutionThatSpentNothing) {
    // 处决那一条只置了个旗标：台词说灌了七毒水，背包却一瓶没少。
    KillEvidence broken = faithfulExecutionScene();
    broken.poisonAfter = broken.poisonBefore;
    for (SceneStep& step : broken.steps) step.poison = broken.poisonBefore;
    const KillVerdict caught = twoKillsVerdict(broken);
    EXPECT_FALSE(caught.ok) << "七毒水一瓶没少也算处决过了？";
    EXPECT_NE(caught.why.find("七毒水"), std::string::npos) << caught.why;
}

TEST(Ch03KillTrace, TheVerdictRefusesASceneThatNeverPlayed) {
    // 先验判据。空 trace 能同时满足「翻旗标之前没有选择」与「台词都在身醒段」
    // ——两条都是在空集合上恒真。判据必须先咬住「这场戏确实演过」。
    KillEvidence empty;
    empty.finalMoSiwang = 1;
    empty.finalChujue = 1;
    empty.poisonBefore = 1;
    empty.poisonAfter = 0;
    const KillVerdict verdict = twoKillsVerdict(empty);
    EXPECT_FALSE(verdict.ok) << "一句话也没演也算过，那这条判据等于没写";

    // 同一个形状的另一面：演了，但一次也没问过玩家。那么「处决是玩家主动做的」
    // 这句话就没有证据，判据同样该说不。
    KillEvidence silent = faithfulExecutionScene();
    for (SceneStep& step : silent.steps) step.choice = false;
    EXPECT_FALSE(twoKillsVerdict(silent).ok) << "整场没问过玩家也算玩家主动处决？";
}

TEST(Ch03KillTrace, TheVerdictCatchesAnExecutionFlagSetTooEarly) {
    // 处决那条旗标在戏演完之前就置上，等于「他一开口就死了」。
    KillEvidence broken = faithfulExecutionScene();
    broken.steps.back().chujue = 1;
    EXPECT_FALSE(twoKillsVerdict(broken).ok) << "处决在最后一句之前就成立了，判据却说没事";
}

TEST(Ch03KillTrace, TheVerdictCatchesEitherKillGoingMissingAltogether) {
    KillEvidence noMo = faithfulExecutionScene();
    noMo.finalMoSiwang = 0;
    EXPECT_FALSE(twoKillsVerdict(noMo).ok);

    KillEvidence noYu = faithfulExecutionScene();
    noYu.finalChujue = 0;
    EXPECT_FALSE(twoKillsVerdict(noYu).ok);
}

}  // namespace
