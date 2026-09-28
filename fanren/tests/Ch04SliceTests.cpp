// 第 4 章的自动化通关测试（设计文档 docs/ch04-design.md 第 7 节验收第 2 条），
// 外加第 3 条要的「三样引擎前置各有测试」里**玩家实际走的那一段**。
//
// 这一条测试驱动的是玩家真会碰的那一整套：真的 Application（无头）、真的
// maps/ch04_*.tmj 与 ch01_hanjiacun、真的 WorldScene 走位规则、真的
// scripts/ch04/*.lua、真的 BattleScene 与 AlchemyScene。从第 3 章章末那份存档
// 出发，走过十一个节点，落在 ch04.done 上，沿途每一道闸门都验两次——条件不满足
// 时确实拦得住，条件满足后确实开得了。
//
// 与邻居的分工（写之前逐个读过，不重复钉同一件事）：
//   * tests/Ch03SliceTests.cpp   —— 上一章的通关范例。走位、BFS、faceCell 全部
//     照它写，那段「interact 看面朝的前一格」的规则原样适用（见下）。
//   * tests/Ch04MagicTests.cpp   —— 已习得法术的**规则层**：集合语义、存档往返、
//     4→5 迁移、脚本 API、菜单。它用的是 data 里本来就有的火球术（写那一批时
//     火弹术还没进 data），钉的是「随便哪一门法术都走得通」。
//   * tests/Ch04WaveTests.cpp    —— 波次的**规则层**：推波、不回血、加载器校验。
//     它用的是就地搭出来的两波编成与仓库里随便一场多波仗。
//   * tests/Ch04AlchemyTests.cpp —— 炼制的**规则层**：配方加载器、面板条目、
//     成功率、失败策略。它用的是金疮药方（难度 5、熟练度 0），不是本章的方子。
//   * 本文件                     —— 整章一趟走通；外加那三样各自**在玩家走的
//     那条路上**是什么样：火弹术是他在节点 2 学会的、波次是节点 9 那一场真的
//     在分波、丹炉是节点 3 之后那座真的炉子。
//
// ---------------------------------------------------------------------------
// 一条走位规则，与 Ch02/Ch03SliceTests 同源，本章两种都有，照抄一遍免得再踩
// ---------------------------------------------------------------------------
// WorldScene::interact 看的是「面朝的前一格」，而 tryStep 先转向再迈步：迈得动
// 就走过去（这时前一格是再往前那一格），迈不动才只转向。于是要站在 S 上脸朝着
// 相邻的 F，只有两条路：
//   1. F 进不去（墙 / NPC / 设施占格）—— 站到 S 上，朝 F 按一下方向键，人不动、
//      脸转过去。ch04_luorifeng 的 trigger_kailu 与 facility_danlu 同格，设施占格
//      不可进，走的就是这一条。
//   2. F 进得去 —— 那就得是「从 F 的另一侧那一格出发，朝 F 的方向迈一步落到 S」。
//      trigger_yufeng（崖台台面）、trigger_qiecuo（演武台台面）、trigger_zhanlipin
//      （焦土）、trigger_xinbie（屋南墙根）、trigger_dongqu（老树底下）都在走得
//      进去的地面上。
// faceCell 把两条都实现了。只实现一种会有一半触发器点不着。
//
// ---------------------------------------------------------------------------
// 本章多出来的第一条：**战斗不能交给无头自动战，本章的高潮要求赢**
// ---------------------------------------------------------------------------
// 无头模式下 BattleScene::update 会调 runToCompletion 把整场自动打完，而那一套
// 两边都用同一个 decideAi——它**近战优先于法术**（BattleAi.cpp 第 2、3 条）。
// 金光上人防 30，韩立平砍（炼气八层攻 15）15×2−30 破不了防，每下保底 1 点；于是自动战里他会站在
// 一个砍不动的人面前砍到死。而 scripts/ch04/gongfang.lua 明写「赢是唯一一种往下
// 走的落点，输就是输（game_over）」——自动战跑不出 ch04.done。
//（上面说的是战棋时代的 decideAi。八方旅人化改造之后我方 AI 改成了找破绽、攒劲打破势，
// 见 docs/octopath-battle.md 第 3 节；本文件仍自己驱动每一场，理由不变：通关测试
// 量的是玩家那条菜单路径，不是 AI。）
//
// 所以本文件自己驱动每一场：走的是**玩家真会走的那条菜单路径**
//（openMenu → 蓄劲 → 攻击 / 施法 / 物品 → 挑目标 → 确认，最终收在 issuePlayerAction）。
// 替玩家按菜单的是第 3、4、5 章共用的那只手（tests/BattleHand.h，取舍全写在它的文件头）：
// 蓄劲、挑已揭开的破绽、没揭开的拿没试过的类别去探、破势时蓄满打、首领蓄势时抢着破势或防御；
// 按物品 id 只吃养精丹，门槛六成，外加「再挨一轮就死」的兜底；不喂曲魂。
//
// 这只手比自动战强，但**它不是一个完美的玩家**：它不省最后几点法力、不替同伴挡刀。
// 所以「它赢了」证明得了这一章走得通，「它输了」证明不了这一章打不过——真输了要先查这只手。
// 它自己稳不稳，由 Ch04HandSweep 每次重扫一遍
//（吃药门槛、药数、背包里多一样少一样，三张攻防编成一格也不许翻）。
//
// ---------------------------------------------------------------------------
// 本章多出来的第二条：**起点不许自己挑**（独立校对 CRITICAL-1 的整改）
// ---------------------------------------------------------------------------
// 这里从前写的是 `kStartRealm = QiRefining13`，来路是「照着怎么才能让这条测试绿
// 反推出来的」。文件头当时诚实地写明了这一点——**但写明一件事不等于那件事可以
// 成立**：第 3 章章末交过来的是炼气三层，两章脚本一处也改不了境界，靠打坐补这十层
// 要四十多个游戏年，而本章日历只有八百余天。于是那条「自动化通关测试」证明的其实是
// 「**如果**韩立有 130 点法力，这一章走得完」，而那个「如果」在游戏里不成立。
//
// **一条通关测试若可以自己挑起点，它就不再是通关测试，是一份「在某个起点上剧情
// 走得通」的报告。** 独立校对把它判成 CRITICAL，判得对。
//
// 那一次整改把境界这一位接上了，但章首仍是手搭的，与第 3 章真实的终局差着九个背包堆、
// 日期、掌天瓶与二十三个旗标，一行注释也没有；复验把第 3 章的终局真的读进来，当场红
//（复验判据 1）。**现在起点不在本文件里写**：它是 tests/fixtures/ch03-end-first.sav /
// ch03-end-second.sav，由第 3 章两条通关用例走到终点时写出、并且每次都逐字段比着
//（tests/ChapterFixture.h）。本文件原样读进来，与那份存档不同的只有一处——位置——
// 有一段说明它怎么变过来的（startFromChapterThreeEnding；从前还有 ch01.muqin_bie 一处，
// 2026-09-27 交接存档带齐第 1、2 章目标链旗标之后删了）。
// 三条 TEST_P 两侧的终局各跑一遍。
//
// 这一章**自己**把境界推到炼气八层，三次，各挂在一个节点上，各有原著出处——
// 见 `docs/ch04-design.md` 1.2 节与下面那三个 kRealmAfter*。
// 三次上涨本身也是断言（`TheChapterItselfRaisesHimFromWhereChapterThreeLeftHim`），
// 不是夹具：它们是设计文档写死的取值，得有人钉着。
//
// ---------------------------------------------------------------------------
// 本章多出来的第四条：**药只许来自本章真发得出的料**（复验判据 2、N-1）
// ---------------------------------------------------------------------------
// 这是同一个形状的第二次：上一次测试替玩家准备了境界，这一次替玩家准备了药——
// 每一炉之前 topUpHerbs() 把料补齐，于是测试里永远炼得出十六瓶，而本章实际发的料
// 一瓶也炼不出。那个函数已经删掉。现在的药全部出自 kailu.lua 与 yufeng.lua 分次发的俸禄，
// 炼法见 brewEverything；开了几炉必须**正好**等于那些料够开的炉数（kFurnaces*），
// 多一炉就说明料有别的来路。发料的总账在设计 3.3，不经过驱动直接核对它的是
// tests/Ch04SalaryTests.cpp。
//
// ---------------------------------------------------------------------------
// 本章多出来的第三条：路上不许顺手把剧情演了
// ---------------------------------------------------------------------------
// 与第 3 章同一条。ch04_yanwuchang 西辕门那条甬道里前后挨着两个 enter 触发
//（trigger_kaizhan 在 (8,20)(8,21)、trigger_gongfang 在 (9,20)(9,21)），而甬道是
// 上校场唯一的路；ch04_getang 的二门 (23,32)(24,32) 是外院进内院唯一的口子。
// 所以 standable() 多问一句：这一格上有没有**此刻还备着**的踏入型触发
//（WorldScene::triggerReady，与引擎同一个判据）。绕不过去时才退回允许踩。
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <deque>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "core/rules/Objectives.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/CultivationScene.h"
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
using fanren::core::battle::Action;
using fanren::core::battle::ActionKind;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::Unit;
using fanren::game::AlchemyScene;
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::WorldScene;
using fanren::rules::CraftKind;
using fanren::rules::Realm;

// ---- 本章踏过的五张图（第 1 章的韩家村是就地修补，不是新建）----
constexpr const char* kMapHanjiacun = "ch01_hanjiacun";
constexpr const char* kMapShanxiazhen = "ch04_shanxiazhen";
constexpr const char* kMapGetang = "ch04_getang";
constexpr const char* kMapLuorifeng = "ch04_luorifeng";
constexpr const char* kMapYanwuchang = "ch04_yanwuchang";

// ---- 本章动到的东西 ----
constexpr const char* kFireMagic = "magic_huodan_shu";     // 节点 2，全作第一门
constexpr const char* kWindMagic = "magic_yufeng_jue";     // 节点 4
constexpr const char* kPlaque = "material_heipai";         // 节点 10，那块没名字的
constexpr const char* kTalisman = "talisman_jianfu";       // 节点 10，剑符
constexpr const char* kPill = "pill_yangjing_dan";         // 节点 3 炼、节点 11 留
constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZishen = "herb_zishen_cao";
constexpr const char* kRecipePill = "recipe_yangjing_dan";
constexpr const char* kRecipeEasy = "recipe_qingling_san";  // 熟练度 0 的那张，垫脚用
constexpr const char* kCompanion = "qu_hun";

// ---- 本章的日历 ----
// 每一条都取自对应脚本里那句 advance_days。两条分支（节点 1 的留不留、节点 3 的
// 报价）都不改日子，所以合计与选哪一项无关——这一点本身要验。
constexpr int kLiuxiaDays = 180;        // liuxia.lua：门里先拿底下弟子试他半年
constexpr int kHuodanDays = 150 + 120;  // huodan.lua：一冬啃古文，再四百来道记号
constexpr int kKailuDays = 2;           // kailu.lua
constexpr int kYufengDays = 360 + 3;    // yufeng.lua
constexpr int kKaizhanDays = 0;         // kaizhan.lua：布置完就散，那三天挪走了（见下一行）
// 三天备战：jinguang.lua 末尾，殿上那一场之后、夜袭之前（复验 N-11）。
// 从前在 kaizhan.lua 里，排在坊门那一战之前，于是静养一刀也落不到攻防战上。
constexpr int kBeizhanDays = 3;
constexpr int kHuicunDays = 5;          // huicun.lua：五天的路
constexpr int kDongquDays = 1;          // dongqu.lua

constexpr int kChapterDays = kLiuxiaDays + kHuodanDays + kKailuDays + kYufengDays + kKaizhanDays +
                             kBeizhanDays + kHuicunDays + kDongquDays;

// ---- 章首那份存档 ----
//
// **起点不是在这里写的**：它是 tests/fixtures/ch03-end-*.sav，由第 3 章两条通关用例
// 写出、并由它们逐字段看着（tests/ChapterFixture.h 文件头）。下面这个数只用来**核对**
// 读进来的那一份：`tests/Ch02SliceTests.cpp` 断言第 2 章章末 `realm == QiRefining3`，
// 第 3 章全章不改境界。
constexpr Realm kChapterThreeEnding = Realm::QiRefining3;

// ---- 本章自己把他推到哪儿（设计文档 1.2 节，每一行带原著出处）----
//
// 判据抄自设计文档，**不是从脚本读出来的**——脚本写错了正该由这三行抓住。
constexpr Realm kRealmAfterLiuxia = Realm::QiRefining5;   // 节点 1，依据 ch43
constexpr Realm kRealmAfterHuodan = Realm::QiRefining7;   // 节点 2，依据 ch65
constexpr Realm kRealmAfterYufeng = Realm::QiRefining8;   // 节点 4，依据 ch75
// 章末仍是八层：原著 ch96「像韩立这样修炼长春功到了第八层的人」。本章没有第四次。
constexpr Realm kChapterEndRealm = kRealmAfterYufeng;

// ---- 那只手的取舍（缺省值就是通关测试用的那一只）----
//
// 气血不高于上限的这个百分比就吃药。扫描用例（Ch04HandSweep）在 40–70 之间换它，
// 三张攻防编成的胜负一格也不许翻——复验判据 3。
constexpr int kHealAtPercent = 60;

// ---- 药的账（docs/ch04-design.md 3.3，判据抄自设计原文，不从脚本或实测推）----
//
// 攻防战最费的那一档要几瓶：三张编成 × 吃药门槛 40–70% 各档里，身上最少得揣几瓶才赢。
// 扫描用例（Ch04HandSweep）每次都重量一遍，量出来比这个数大就红——那说明编成或那只手
// 变了，这张账要重算。
constexpr int kSiegeNeedFromDesign = 8;
// 峰上那一趟用本章发的料炼出来的，**设计口径的坏情况**（5% 分位，照价那一条 45 炉）。
// 通关测试炼出来的比这个少，要么是发料少了一批，要么这一趟的骰子比设计口径的坏情况还坏。
constexpr int kPillsBadCase = 16;
// 本章发的料够开几炉（清灵散与养精丹一样，每炉黄精 2、紫参 1）：照价那一条 45 炉，
// 只收药钱那一条 46 炉。brewEverything 炼到开不了工为止，所以开了几炉就该**正好**是这个数——
// 多一炉，说明料不是本章发的（从前的无限补料就是这个长相）；少一炉，说明发料少了一批。
constexpr int kFurnacesOnThePriceBranch = 45;
constexpr int kFurnacesOnTheCostBranch = 46;

// ---- 一炉不炼的玩家在攻防战之前被提醒到（二次复验 R-3，docs/ch04-design.md 3.3 末段）----
// 判据写死成设计原文，不从目标链或脚本里读：
//   · 目标链里御风决之后那一步是炼药：id n4b_liandan，指着落日峰那座丹炉；
//   · 它的完成旗标 ch04.liandan 只在揣着养精丹去切磋时置（qiecuo.lua）；
//   · 身上 0 瓶走进攻防战，厉飞雨在甬道口问那一句 ch04.gongfang.nopill，第一项是回峰上开炉。
constexpr const char* kBrewStepId = "n4b_liandan";
constexpr const char* kBrewStepMap = "ch04_luorifeng";
constexpr const char* kBrewStepObject = "facility_danlu";
constexpr const char* kBrewFlag = "ch04.liandan";
constexpr const char* kNoPillLine = "ch04.gongfang.nopill";
constexpr const char* kNoPillBackLine = "ch04.gongfang.back";

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

constexpr double kFrame = 1.0 / 60.0;
// 死循环闸。本章一条路走下来最长的一场是 jia_tianlong.lua（二十一句 talk），战斗由本文件
// 自己驱动、不占帧，所以这个数宽裕得很。脚本卡住时宁可测试失败，也不要挂住 ctest。
constexpr int kMaxScriptFrames = 6000;
// 一场仗最多推多少步。推到这个数还没分出胜负，按卡住算——宁可红，不要挂住 ctest。
constexpr int kMaxBattleSteps = 8000;

// 仓库根：测试可能从 build/ 或工程根启动。判据用本章自己的脚本与地图，
// 找错根目录时报的是「找不到 ch04 的东西」，而不是一串莫名其妙的空断言。
std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch04" / "dongqu.lua") &&
            fs::exists(root / "maps" / "ch04_yanwuchang.tmj")) {
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
// 五张图之间的走法
// ---------------------------------------------------------------------------
// 表里只有「哪张图能通到哪张图、走哪个传送点」，**不写任何 require_flag**：
// 闸开没开由引擎说了算，走不过去时 usePortal 会如实报 false，而那正是
// 「这道闸此刻拦得住」要的那个返回值。把闸门条件抄进这张表，等于让测试自己判卷。
struct MapLink {
    const char* from;
    const char* to;
    const char* portal;
};

constexpr MapLink kMapLinks[] = {
    {kMapHanjiacun, kMapShanxiazhen, "portal_to_ch04_shanxiazhen"},
    {kMapShanxiazhen, kMapHanjiacun, "portal_to_hanjiacun"},
    {kMapShanxiazhen, kMapGetang, "portal_to_getang"},
    {kMapGetang, kMapShanxiazhen, "portal_to_shanxiazhen"},
    {kMapGetang, kMapLuorifeng, "portal_to_luorifeng"},
    {kMapGetang, kMapYanwuchang, "portal_to_yanwuchang"},
    {kMapLuorifeng, kMapGetang, "portal_to_getang"},
    {kMapYanwuchang, kMapGetang, "portal_to_getang"},
};

// ---------------------------------------------------------------------------
// 章末的账
// ---------------------------------------------------------------------------
// 做成一个具名判据而不是就地摊一串 EXPECT，是为了能拿同一份判据去跑一条
// 「故意摆错 → 判据确实说不」的自检（见本文件末尾那几条 Ch04EndState 用例）。
//
// 本章的账里有一笔是「某样东西该没了」：节点 11 留给厉飞雨的三瓶养精丹。
// 这种形状的断言有个天然的空转法——背包整个是空的时候它照样通过。所以下面
// 那一条「该没了」的旁边钉着一组「该还在」的先验判据（剑符、那块牌子、
// 两门法术），少了它们，一条把背包清空的缺陷会让「留下了」这一条变成装饰。
struct EndBag {
    int pillsBefore = 0;   // 走到节点 11 跟前时手上有几瓶养精丹
    int pillsAfter = 0;    // 演完之后还剩几瓶
    bool saidHave = false; // 演的是「摆了三只瓶子」那一条
    bool saidNone = false; // 演的是「翻遍箱底也没有」那一条
    bool packedFull = false;
    bool packedBare = false;
    int plaque = 0;
    int talisman = 0;
    bool knowsFire = false;
    bool knowsWind = false;
};

struct BagVerdict {
    bool ok = true;
    std::string why;
};

// 留药那一条的判据。**它不规定玩家手上该有几瓶**——那要看他这一章炼没炼成，
// 两侧都是合法的结局（xinbie.lua 首部原话）。它规定的是**说的和做的必须一致**：
// 说留了三瓶就真的少三瓶，说一瓶也没有就一瓶也不许少。
// 第 2 章在 take() 上栽过两次，第二次让玩家白拿了 167 块，防的就是这一类。
BagVerdict chapterEndBag(const EndBag& e) {
    const auto fail = [](std::string why) { return BagVerdict{false, std::move(why)}; };

    // ---- 先验判据：这一章确实发生过 ----
    // 下面那条「药少了三瓶」全靠这一组撑着：一个把背包清空的缺陷会让它自动成立。
    if (e.plaque != 1) {
        return fail("节点 10 那块牌子该在身上，实为 " + std::to_string(e.plaque) + " 件");
    }
    if (e.talisman != 1) {
        return fail("节点 10 那张剑符该在身上，实为 " + std::to_string(e.talisman) + " 件");
    }
    if (!e.knowsFire) return fail("节点 2 学会的火弹术不在他的清单上");
    if (!e.knowsWind) return fail("节点 4 学会的御风决不在他的清单上");

    // ---- 说了哪一条 ----
    if (e.saidHave == e.saidNone) {
        return fail(std::string("留药那一节两条台词") + (e.saidHave ? "都演了" : "一条也没演") +
                    "：take() 的返回值没接上");
    }
    if (e.packedFull != e.saidHave || e.packedBare != e.saidNone) {
        return fail("包起来那一句与前面「有没有药」那一句对不上：一条脚本里同一个"
                    "返回值分了两次岔，两次答案却不一样");
    }

    // ---- 说的和做的对不对得上 ----
    if (e.saidHave) {
        if (e.pillsBefore < 3) {
            return fail("台词说摆了三只瓶子，可进这一节时手上只有 " +
                        std::to_string(e.pillsBefore) + " 瓶");
        }
        if (e.pillsAfter != e.pillsBefore - 3) {
            return fail("台词说留下了三瓶，背包却从 " + std::to_string(e.pillsBefore) + " 变成 " +
                        std::to_string(e.pillsAfter) + "：一边说留了、一边一瓶没少");
        }
    } else {
        if (e.pillsBefore >= 3) {
            return fail("手上有 " + std::to_string(e.pillsBefore) +
                        " 瓶，台词却说翻遍箱底也没有：take() 扣得到却报了扣不到");
        }
        if (e.pillsAfter != e.pillsBefore) {
            return fail("这一条一瓶也不该扣，背包却从 " + std::to_string(e.pillsBefore) +
                        " 变成 " + std::to_string(e.pillsAfter) +
                        "：take() 失败时不许动背包");
        }
    }
    return BagVerdict{};
}

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch04Walkthrough : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 缺省从第 3 章「每一处都取第一项」那一侧的终局起步；要另一侧的用例自己再调一次。
        startFromChapterThreeEnding(fanren::test::kChapterThreeEndingFirst);
    }

    void TearDown() override { app_.shutdown(); }

    // 第 3 章章末交到本章手里的那份存档，**原样读进来**（复验判据 1）。
    //
    // 它是 tests/Ch03SliceTests.cpp 两条通关用例走到终点时写出的那两份，那一头每次都
    // 逐字段比着，所以这里读到的就是第 3 章此刻真实的终局——背包、日期、掌天瓶、旗标、
    // 曲魂、修为与剧情境界上限，一样不少也一样不多。
    //
    // **与那份存档不同的字段只有一处**：位置，写在下面，说明它是怎么变过来的。
    // 除此之外一处也不许手摆。要摆，照那一处的样子写一行说明它的来路。
    //（从前还有第二处 `ch01.muqin_bie`，见下面第 2 条为什么删了。）
    void startFromChapterThreeEnding(const char* fileName) {
        const fs::path fixture = fanren::test::chapterFixturePath(assetRoot(), fileName);
        auto handedOver = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(handedOver.ok) << "第 3 章的交接存档读不进来（" << fixture.string()
                                   << "）：" << handedOver.error
                                   << "。它由 Ch03Walkthrough 写出，见 tests/ChapterFixture.h";
        // 先验：读进来的确实是第 3 章的终点，而且落在谷外——下面那条「走六张图」的
        // 说明是按谷外写的，终点挪了地方，这一段要跟着重看。
        ASSERT_EQ(handedOver.value.flag("ch03.done"), 1) << "交接存档不是第 3 章的终局";
        ASSERT_EQ(handedOver.value.mapId, "ch03_guwai") << "第 3 章不再在谷外收场了";
        state() = handedOver.value;

        // ---- 1. 位置：谷外 → 韩家村村口 ----
        // 第 3 章在谷外离谷收场（ligu.lua）；本章的入口开在韩家村东
        //（ch01_hanjiacun 的 portal_to_ch04_shanxiazhen，require_flag=ch03.done）。
        // 中间是谷外 → 神手谷 → 炼骨崖 → 七玄门 → 彩霞山 → 青牛镇 → 韩家村六跳，
        // 六道门一道也没有 require_flag。路上的踏入型挂点全是第 1 章的 once 挂点
        //（青牛镇 trigger_baoming、彩霞山 trigger_yu_zhangtie……），它们的 set_flag
        // 在真实存档与交接存档里都早已置上，所以走过去只改位置，不改任何别的字段。
        // 这里直接摆到村口出生点，省掉六张图的走位。
        auto loaded = app_.loadMap(kMapHanjiacun, "spawn_main");
        ASSERT_TRUE(loaded.ok) << loaded.error;

        // ---- 2.（已删）ch01.muqin_bie ----
        // 从前这里单独补了这一个第 1 章旗标：第 3 章通关测试的起点是手搭的第 2 章章末，
        // 那份只带了 ch01.done（docs/tech-debt.md G-23）。2026-09-27 起那份手搭的章末
        // 从目标链读、带齐第 1、2 章每一步的完成旗标（Ch03SliceTests::applyChapterTwoEnding），
        // 交接存档里本来就有它，这一行成了空操作，删掉。补齐的起因：第 1 章的人物如今
        // 按那些旗标撤场（docs/map-links.md），缺旗标的存档里三叔、韩母、山道张铁、
        // 两位考官会在第 4、5 章又站出来，一按还会重演。

        startDay_ = state().day;
    }

    // 这一趟的终局与交接存档 tests/fixtures/<fileName> **逐字段**一致
    //（契约 docs/interfaces-p3-ch05.md 第 3 节，照第 3 章那一头的做法）。
    //
    // 那份存档是第 5 章通关测试的起点，所以这一条看住的是**第 4、5 两章之间的缝**：
    // 第 4 章改了而交接存档没跟着重生成，这里红；有人手改了交接存档，这里也红。
    // 置了 FANREN_WRITE_CH04_FIXTURES 时改为写出这份存档。
    void settleEndingAgainstFixture(const char* fileName) {
        const fanren::test::FixtureVerdict verdict = fanren::test::settleAgainstFixture(
            state(), assetRoot(), fileName, fanren::test::kWriteChapterFourFixturesEnv, "第 5 章");
        if (verdict.wrote) {
            std::cout << "[ch04 交接存档] 已写出 "
                      << fanren::test::chapterFixturePath(assetRoot(), fileName).string() << std::endl;
        }
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
    }

    GameState& state() { return app_.state(); }
    int flag(const std::string& name) { return state().flag(name); }

    // 刚才那一节里说过这一句没有。
    //
    // 判据只认「说过 / 没说过」，所以每一次 fireTrigger / enterTrigger 都先把
    // 台词本清空——不清的话 spokenKeys 攒到 128 条就开始丢最旧的那几条
    //（Application::kSpokenLogCap），于是隔了几节再回头问「当时说过没有」
    // 会拿到一个假的「没说过」。这正是「找不到某个词就算过」那一类空转法。
    bool spoke(const std::string& key) const {
        const std::vector<std::string>& keys = app_.spokenKeys();
        return std::find(keys.begin(), keys.end(), key) != keys.end();
    }

    // ---- 脚本 ----

    // 像主循环那样把脚本推到结束，替玩家按确认；碰上战斗就自己打。
    //
    // 战斗那一条命令**不能在这里回一个 ok 了事**：脚本的分支全押在那个返回值上。
    // BattleScene 是在帧末才真的压进场景栈的（Application::tick 的 pendingPush_），
    // 所以 tick 之后立刻就能接手；接手在它自己 update 之前，无头自动战因此跑不起来。
    void pumpScripts(int choiceIndex) {
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            if (auto* fight = dynamic_cast<BattleScene*>(app_.topScene()); fight != nullptr) {
                battlesFought_.push_back(hand_.play(*fight) == BattlePhase::Won);
                continue;   // 下一帧 update 见到胜负已分，自己 finish 并回填给脚本
            }
            if (!app_.awaitingCommand()) continue;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = choiceIndex;
            app_.completeCommand(result);
            // 对话框是脚本压进来的，演完就收掉。不收的话整章三百多句话会在场景栈上
            // 摞成三百多层，后面拿 topScene() 验「被拦下时有没有说明」就全是噪声。
            app_.popScene();
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << "脚本没能跑到结束";
        // 最后一句话的对话框还挂在栈上，收场时那次 popScene 也还压着没生效。
        // 多推一帧把两者一起结清。
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
    //（WorldScene::triggerReady），比引擎宽一格就会路过时把剧情演了，
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

    // 从当前位置到 goal 的一条路。用 BFS 而不是写死路线：地图一改，写死的路线
    // 会悄悄失效，而 BFS 会如实报告「走不到」。
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

    // strict 为真时只走「一处活着的踏入型触发都不碰」的路，走不到就认输；
    // 为假时走不到才退回允许踩。分开两档是为了让 stepOnto 先试一圈干净的落脚点：
    // 本章西辕门那条甬道里两个 enter 触发前后相邻，不先试一圈的话，
    // 去踩前一个的路上会先把它自己踩掉一次。
    bool walkTo(Point goal, int choiceIndex = 0, bool strict = false) {
        if (app_.state().position == goal) return true;
        std::vector<Point> path = routeTo(goal, /*avoidTriggers=*/true);
        if (path.empty() && !strict) path = routeTo(goal, /*avoidTriggers=*/false);
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
    // 先试一圈「不碰任何活着的踏入型触发就走得到」的落脚点（strict），
    // 一圈下来都不行才放宽。
    bool stepOnto(const MapObject& object, int choiceIndex) {
        for (const bool strict : {true, false}) {
            for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                    const Point target{object.position.x + dx, object.position.y + dy};
                    if (!map().walkable(target)) continue;
                    for (const Point& direction : kDirections) {
                        const Point stand{target.x - direction.x, target.y - direction.y};
                        if (!standable(stand, /*avoidTriggers=*/true)) continue;
                        if (!walkTo(stand, choiceIndex, strict)) continue;
                        step(direction, choiceIndex);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // 走到目标格旁边、迈最后一步，**但不把脚本演完**。
    // 用处：验「走过去不该有事发生」——踩完看 scripts().isRunning() 就知道
    // 这一格是不是会自己开演。
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

    // ---- 三个动作 ----

    // 就地按一下确认并把脚本演完（人已经站好、脸已经朝对了）。
    // 与 fireTrigger 的分别：这一条不走路。验「把旗标撤掉这道闸就拦得住」时要用它
    // ——那种局面下再走一步都可能踩响别的东西。
    bool pressHere(int choiceIndex = 0) {
        app_.clearSpokenKeys();
        if (!world_.interact(app_)) {
            ADD_FAILURE() << "面朝的那一格按不出任何东西";
            return false;
        }
        if (!app_.scripts().isRunning()) {
            ADD_FAILURE() << "这一下点着的不是脚本";
            return false;
        }
        pumpScripts(choiceIndex);
        return true;
    }

    // 点着一处 interact 触发器（或它让位之后的那个设施），并把脚本演完。
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

    // 面朝某个对象按确认，**不问点着的是什么**。
    // 用来验「触发器烧掉之后同格的设施接手」——那一下点着的是面板，不是脚本。
    bool pressAt(const MapObject& object, int choiceIndex = 0) {
        if (!faceObject(object, choiceIndex)) return false;
        app_.clearSpokenKeys();
        const bool handled = world_.interact(app_);
        app_.tick(kFrame);   // 场景栈在帧末才真的改
        return handled;
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
    // 十一个节点，每一节只管「把玩家送到那一步并演完」
    // -----------------------------------------------------------------------
    // 断言一律留给用例：同一条路要被好几条用例走，把断言塞进这里会让它们共用
    // 一套判据，而那正是「一处写错几处一起瞎」的长相。
    bool nodeLiuxia(int c) {        // 1 以韩神医的身份留下
        return travelTo(kMapGetang, c) && enterTrigger("trigger_liuxia", c);
    }
    bool nodeHuodan(int c) {        // 2 习火弹术
        return travelTo(kMapLuorifeng, c) && enterTrigger("trigger_huodan", c);
    }
    bool nodeKailu(int c) {         // 3 开炉：药物买卖
        return travelTo(kMapLuorifeng, c) && fireTrigger("trigger_kailu", c);
    }
    bool nodeYufeng(int c) {        // 4 习御风决
        return travelTo(kMapLuorifeng, c) && fireTrigger("trigger_yufeng", c);
    }
    bool nodeQiecuo(int c) {        // 5 法武并用（一场切磋）
        // mode=interact 是设计本身：这一打是他自己要打的（qiecuo.lua 首部）。
        // 摆成踏入型就等于把「主动」摊薄成「路过」。
        return travelTo(kMapYanwuchang, c) && fireTrigger("trigger_qiecuo", c);
    }
    bool nodeKaizhan(int c) {       // 6 野狼帮来犯，开战（三选一）
        // mode=enter：号角是在他背后响的（kaizhan.lua 首部）。
        return travelTo(kMapYanwuchang, c) && enterTrigger("trigger_kaizhan", c);
    }
    bool nodeJia(int c) {           // 7 贾天龙现身
        return travelTo(kMapShanxiazhen, c) && enterTrigger("trigger_jia_tianlong", c);
    }
    bool nodeJinguang(int c) {      // 8 金光上人与王绝楚
        return travelTo(kMapGetang, c) && enterTrigger("trigger_jinguang", c);
    }
    bool nodeGongfang(int c) {      // 9 门派攻防战（多波次）
        return travelTo(kMapYanwuchang, c) && enterTrigger("trigger_gongfang", c);
    }
    bool nodeZhanlipin(int c) {     // 10 战利品：剑符与那块牌子
        return travelTo(kMapYanwuchang, c) && fireTrigger("trigger_zhanlipin", c);
    }
    bool nodeHuicun(int c) {        // 11 上 回村
        return travelTo(kMapHanjiacun, c) && enterTrigger("trigger_huicun", c);
    }
    bool nodeXinbie(int c) {        // 11 中 留药与信别
        return travelTo(kMapHanjiacun, c) && fireTrigger("trigger_xinbie", c);
    }
    bool nodeDongqu(int c) {        // 11 下 东去（章末）
        return travelTo(kMapHanjiacun, c) && fireTrigger("trigger_dongqu", c);
    }

    // 走到峰上那一年炼完药为止（节点 1–4 ＋ 开炉把俸禄炼掉），一句断言也不做。
    void playUpToTheBrewing(int c) {
        ASSERT_TRUE(nodeLiuxia(c));
        ASSERT_TRUE(nodeHuodan(c));
        ASSERT_TRUE(nodeKailu(c));
        ASSERT_TRUE(nodeYufeng(c));
        // 峰上那一年开炉卖药。战斗之内吃药是唯一的疗伤手段（tests/BattleHand.h 文件头第 1 条），
        // 药从哪来就是这一步：本章分次发下来的俸禄，炼掉。
        brew_ = brewEverything();
    }

    // 攻防战前后的一份记录：扫描表与几条通关用例都打印它。
    struct SiegeRecord {
        int hpBefore = 0, maxHp = 0;
        int companionBefore = 0, companionMax = 0;
        int pillsBefore = 0;
        int hpAfter = 0, companionAfter = 0, pillsAfter = 0;
        bool won = false;
    };

    int companionHp() {
        if (state().party.empty()) return -99;
        const int hp = state().party[0].hp;
        if (hp >= 0) return hp;
        const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
        return role == nullptr ? -99 : role->maxHp;   // -1 = 还没打过仗，按模板满血
    }
    int companionMax() {
        const fanren::core::RoleTemplate* role = app_.data().findRole(kCompanion);
        return role == nullptr ? 0 : role->maxHp;
    }

    // 从节点 5 走到节点 10 演完为止，一句断言也不做。攻防战前后记进 siege_。
    void playFromTheSparringToTheSpoils(int c, int kaizhanChoice) {
        playFromTheSparringToTheSiege(c, kaizhanChoice);
        if (HasFatalFailure()) return;
        if (!siege_.won || app_.quitRequested()) return;   // 输了没有节点 10，由用例判
        ASSERT_TRUE(nodeZhanlipin(c));
    }

    // 从节点 5 走到节点 9 打完为止。扫描表只要这一段。
    void playFromTheSparringToTheSiege(int c, int kaizhanChoice) {
        ASSERT_TRUE(nodeQiecuo(c));
        ASSERT_TRUE(nodeKaizhan(kaizhanChoice));
        ASSERT_TRUE(nodeJia(c));
        ASSERT_TRUE(nodeJinguang(c));
        siege_ = SiegeRecord{};
        siege_.hpBefore = state().hp;
        siege_.maxHp = state().maxHp;
        siege_.companionBefore = companionHp();
        siege_.companionMax = companionMax();
        siege_.pillsBefore = state().itemCount(kPill);
        const std::size_t battlesBefore = battlesFought_.size();
        ASSERT_TRUE(nodeGongfang(c));
        siege_.won = battlesFought_.size() == battlesBefore + 1 && battlesFought_.back();
        siege_.hpAfter = state().hp;
        siege_.companionAfter = companionHp();
        siege_.pillsAfter = state().itemCount(kPill);
        // 这一节说了哪几句，留给用例去问：spokenKeys 每进一节就清一次，
        // 走完节点 10 再回头问「攻防战那一节说了什么」只会拿到一个假的「没说过」。
        siegeKeys_ = app_.spokenKeys();
    }

    // 把世界拨回某一刻（扫描用）。**整个世界就是这一份 GameState**：挂点烧没烧掉看的是
    // set_flag，NPC 在不在看的是旗标，背包、日子、队伍都在里头——存档存的也就是它。
    // 另要重载那张图、把人摆回原处，再清掉本文件自己记的两本账。
    void restoreTo(const GameState& snapshot) {
        state() = snapshot;
        auto loaded = app_.loadMap(snapshot.mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state().position = snapshot.position;
        state().facing = snapshot.facing;
        battlesFought_.clear();
        siegeKeys_.clear();
    }

    // 走到节点 10 演完为止，一句断言也不做。留药那两条用例都要用。
    void playUpToTheSpoils(int c, int kaizhanChoice) {
        playUpToTheBrewing(c);
        if (HasFatalFailure()) return;
        playFromTheSparringToTheSpoils(c, kaizhanChoice);
        if (HasFatalFailure()) return;
        ASSERT_TRUE(siege_.won) << "门派攻防战输了：gongfang.lua 走的是 game_over 那一条，"
                                << "这一章到此为止。" << siegeLine();
        ASSERT_FALSE(app_.quitRequested());
    }

    std::string siegeLine() const {
        return "攻防战前 韩立 " + std::to_string(siege_.hpBefore) + "/" +
               std::to_string(siege_.maxHp) + "、曲魂 " + std::to_string(siege_.companionBefore) +
               "/" + std::to_string(siege_.companionMax) + "、养精丹 " +
               std::to_string(siege_.pillsBefore) + "；收场 韩立 " +
               std::to_string(siege_.hpAfter) + "、曲魂 " + std::to_string(siege_.companionAfter) +
               "、养精丹 " + std::to_string(siege_.pillsAfter);
    }

    // 收一份留药那一节的证词。
    EndBag collectEndBag(int c) {
        EndBag e;
        e.pillsBefore = state().itemCount(kPill);
        EXPECT_TRUE(nodeXinbie(c));
        e.pillsAfter = state().itemCount(kPill);
        e.saidHave = spoke("ch04.xinbie.have");
        e.saidNone = spoke("ch04.xinbie.none");
        e.packedFull = spoke("ch04.xinbie.pack_full");
        e.packedBare = spoke("ch04.xinbie.pack_bare");
        e.plaque = state().itemCount(kPlaque);
        e.talisman = state().itemCount(kTalisman);
        const std::vector<std::string>& learned = state().learnedMagics;
        e.knowsFire = std::find(learned.begin(), learned.end(), kFireMagic) != learned.end();
        e.knowsWind = std::find(learned.begin(), learned.end(), kWindMagic) != learned.end();
        return e;
    }

    // ---- 炼丹：走到丹房、开炉 ----

    // 丹炉那一格。trigger_kailu 与 facility_danlu 同格（kailu.lua 首部），
    // 节点 3 演完之后 once 烧掉、触发器让位，同一下确认键开的就是炼丹面板。
    AlchemyScene* openFurnace() {
        const MapObject furnace = objectNamed("facility_danlu");
        if (furnace.name.empty()) return nullptr;
        if (!pressAt(furnace)) return nullptr;
        return dynamic_cast<AlchemyScene*>(app_.topScene());
    }

    int recipeRow(CraftKind kind, const std::string& recipeId) const {
        const auto picked = AlchemyScene::visibleRecipes(app_.recipes(), kind);
        for (std::size_t i = 0; i < picked.size(); ++i) {
            if (picked[i]->id == recipeId) return static_cast<int>(i);
        }
        return -1;
    }

    // 上峰去，把手上够年份的药材**全部**炼掉。走的是真的炉子、真的方子、真的面板。
    //
    // **料只有背包里本来有的**——也就是 kailu.lua（节点 3，头一个月）与 yufeng.lua
    //（节点 4，峰上那一年）分次发下来的俸禄（复验判据 2）。从前这里每一炉之前都由
    // topUpHerbs() 把料补齐，于是测试里永远炼得出十六瓶，而本章真发的料一瓶也炼不出；
    // 那个函数已经删掉，**本文件再没有任何一处往背包里塞药材**。
    //
    // 打法是一个不肯浪费料的玩家会怎么打：养精丹有熟练度门槛 6，起点是 0，所以先垫
    // 门槛为 0 的清灵散，垫到够了就停；然后把够年份的料全部下炉炼养精丹，炼到开不了工
    // 为止。第 3 章带来的零年黄精、紫参过不了清灵散那道 10 年门槛，一株也用不上。
    // 炼丹的骰子是确定的（AlchemyScene::seedFor 由方子、日期、熟练度、第几炉派生），
    // 同一个起点每次炼出同样多。
    //
    // 发料的总账（每一批多少、按设计口径期望炼出几瓶、坏情况几瓶）在 docs/ch04-design.md
    // 3.3；不经过驱动、直接读脚本核对那张账的，是 tests/Ch04SalaryTests.cpp 那一组用例。
    struct Brew {
        int warmups = 0;       // 垫了几炉清灵散
        int attempts = 0;      // 开了几炉养精丹
        int pills = 0;         // 炼成几瓶（这一趟新炼的）
        int proficiency = 0;   // 炼完时的熟练度
    };
    Brew brewEverything() {
        Brew brew;
        if (!travelTo(kMapLuorifeng)) return brew;
        AlchemyScene* panel = openFurnace();
        if (panel == nullptr) return brew;
        const int easy = recipeRow(CraftKind::Alchemy, kRecipeEasy);
        const int pill = recipeRow(CraftKind::Alchemy, kRecipePill);
        if (easy < 0 || pill < 0) return brew;
        const int pillsBefore = state().itemCount(kPill);
        // 两道闸都只是防死循环：craftAt 开不了工就返回 false（料不够、火候不够）。
        for (int i = 0; i < 100 && state().alchemyProficiency < 6; ++i) {
            if (!panel->craftAt(app_, easy)) break;
            ++brew.warmups;
        }
        for (int i = 0; i < 400; ++i) {
            if (!panel->craftAt(app_, pill)) break;
            ++brew.attempts;
        }
        panel->leave(app_);
        app_.tick(kFrame);
        brew.pills = state().itemCount(kPill) - pillsBefore;
        brew.proficiency = state().alchemyProficiency;
        std::cout << "[ch04 峰上炼药] 清灵散 " << brew.warmups << " 炉，养精丹 " << brew.attempts
                  << " 炉成 " << brew.pills << " 瓶，熟练度 " << brew.proficiency
                  << "；剩黄精 " << state().itemCount(kHuangjing) << " 紫参 "
                  << state().itemCount(kZishen) << std::endl;
        return brew;
    }

    Application app_;
    WorldScene world_;
    int startDay_ = 1;
    // 那只手（tests/BattleHand.h）。缺省就是通关测试用的那一只（门槛六成）；扫描用例换门槛。
    fanren::test::BattleHand hand_{app_, fanren::test::HandPolicy{kHealAtPercent}};
    Brew brew_;                          // 峰上那一趟炼药的账（playUpToTheBrewing 里炼的那一次）
    SiegeRecord siege_;                  // 攻防战前后（playFromTheSparringToTheSpoils 记的）
    std::vector<bool> battlesFought_;   // 每一场的胜负，按发生顺序
    std::vector<std::string> siegeKeys_; // 攻防战那一节说过的文案 key
};

// ---------------------------------------------------------------------------
// 通关：一趟走到底，沿途的闸门逐一验两次
// ---------------------------------------------------------------------------
TEST_F(Ch04Walkthrough, WalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    constexpr int kFirst = 0;
    GameState& s = state();

    // ===================== 本章的入口 =====================
    // 村东那条道认的是 ch03.done。先把它撤掉验一次「拦得住」，再放回来验
    //「开得了」——只验后一半的话，一个把 require_flag 写丢的地图照样全绿。
    ASSERT_EQ(s.mapId, kMapHanjiacun);
    s.flags.erase("ch03.done");
    EXPECT_FALSE(usePortal("portal_to_ch04_shanxiazhen")) << "ch03 还没完，这条道不该放行";
    EXPECT_EQ(s.mapId, kMapHanjiacun);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
    closePanel();
    s.setFlag("ch03.done");

    // ===================== 节点 1 · 以韩神医的身份留下 =====================
    // 二门是外院进内院唯一的口子，trigger_liuxia 把那两格都占了，跳不过去。
    ASSERT_TRUE(travelTo(kMapShanxiazhen, kFirst));
    ASSERT_TRUE(travelTo(kMapGetang, kFirst));
    ASSERT_EQ(s.mapId, kMapGetang);

    const int dayBeforeLiuxia = s.day;
    ASSERT_TRUE(enterTrigger("trigger_liuxia", kFirst)) << "进内院那一脚该把开场演出来";
    EXPECT_EQ(flag("ch04.liuxia"), 1) << "取第一项：应下";
    EXPECT_EQ(s.day - dayBeforeLiuxia, kLiuxiaDays) << "门里先拿底下弟子试了他半年";
    EXPECT_TRUE(spoke("ch04.liuxia.stay1"));
    EXPECT_FALSE(spoke("ch04.liuxia.leave1")) << "两条口吻只能演一条";

    // 峰上那条路认的是 ch04.liuxia。**这道闸没法靠「走过去试试」验反面**：
    // 二门横在外院与内院之间，是上峰的必经之路，所以「还没留下就想上峰」这个
    // 局面在图上根本走不出来——去撞那道闸的路上先把节点 1 演了。
    // 于是这里照本章入口那一处的办法：撤掉旗标验一次拦得住，放回来再验开得了。
    // 只验后一半的话，一个把 require_flag 写丢的地图照样全绿。
    const MapObject upTheMountain = objectNamed("portal_to_luorifeng");
    ASSERT_FALSE(upTheMountain.name.empty());
    EXPECT_EQ(upTheMountain.property("require_flag"), std::string("ch04.liuxia"));
    s.flags.erase("ch04.liuxia");
    EXPECT_FALSE(usePortal("portal_to_luorifeng")) << "ch04.liuxia 未置位，落日峰不该放行";
    EXPECT_EQ(s.mapId, kMapGetang);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
    closePanel();
    s.setFlag("ch04.liuxia", 1);

    // 防跳过：校场那道门认的是御风决，此刻还差两门功课。
    EXPECT_FALSE(usePortal("portal_to_yanwuchang")) << "ch04.yufeng_xue 未置位，校场不该放行";
    EXPECT_EQ(s.mapId, kMapGetang);
    app_.tick(kFrame);
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue");
    closePanel();

    // ===================== 节点 2 · 习火弹术 =====================
    ASSERT_TRUE(travelTo(kMapLuorifeng, kFirst));
    ASSERT_EQ(s.mapId, kMapLuorifeng) << "留下了，峰上那条路该开了";
    ASSERT_TRUE(s.learnedMagics.empty()) << "先验：上峰之前他一门法术也不会";

    const int dayBeforeHuodan = s.day;
    ASSERT_TRUE(enterTrigger("trigger_huodan", kFirst)) << "峰道口那一脚该把习法演出来";
    EXPECT_EQ(flag("ch04.huodan_xue"), 1);
    EXPECT_EQ(s.day - dayBeforeHuodan, kHuodanDays);
    // **全作第一次「他学会了一门法术」真的落盘**（技术债 G-4 的落点）。
    EXPECT_TRUE(spoke("ch04.huodan.learn")) << "magic.learn 回了 false：data 里那一条出了事";
    EXPECT_FALSE(spoke("ch04.huodan.fail_learn"));
    ASSERT_EQ(s.learnedMagics.size(), 1u);
    EXPECT_EQ(s.learnedMagics[0], kFireMagic);

    // ===================== 节点 3 · 开炉 =====================
    // 丹房那道闸是**脚本自己守的**（kailu.lua 开头查 ch04.huodan_xue），不在地图上。
    // 它的反面在图上走不出来：峰道口是上峰唯一的入口，去丹房的路上先把节点 2
    // 演了。所以照本章入口那一处的办法——人先站到炉前，再把旗标撤掉按一下。
    // **站好了再撤**：撤着旗标走路会让已经烧掉的 trigger_huodan 复活。
    const MapObject stove = objectNamed("trigger_kailu");
    ASSERT_FALSE(stove.name.empty());
    ASSERT_TRUE(faceObject(stove, kFirst)) << "走不到丹炉跟前";
    s.flags.erase("ch04.huodan_xue");
    ASSERT_TRUE(pressHere(kFirst));
    EXPECT_EQ(flag("ch04.kailu"), 0) << "火弹术还没学会，开什么炉";
    EXPECT_EQ(app_.lastSpokenKey(), "ch04.kailu.gate") << "拦住了就要说是为什么";
    s.setFlag("ch04.huodan_xue");

    // 崖台那道闸同样是脚本自己守的（yufeng.lua 开头查 ch04.kailu），而它的反面
    // **是真的走得出来的**：崖台不在任何一条必经的路上。所以这一条不撤旗标，
    // 照玩家的走法直接去撞。一道闸兜住两道，顺序仍旧 2 → 3 → 4。
    app_.clearSpokenKeys();
    ASSERT_TRUE(fireTrigger("trigger_yufeng", kFirst));
    EXPECT_EQ(flag("ch04.yufeng_xue"), 0) << "炉还没开，御风决不该学得上";
    EXPECT_EQ(app_.lastSpokenKey(), "ch04.yufeng.gate");

    const int dayBeforeKailu = s.day;
    const int huangjingBefore = s.itemCount(kHuangjing);
    ASSERT_TRUE(fireTrigger("trigger_kailu", kFirst));
    EXPECT_EQ(flag("ch04.kailu"), 1) << "取第一项：照门中的价";
    EXPECT_EQ(s.day - dayBeforeKailu, kKailuDays);
    EXPECT_TRUE(spoke("ch04.kailu.price1"));
    EXPECT_FALSE(spoke("ch04.kailu.cost1")) << "两条报价只能演一条";
    // 俸禄真的发到手了：只看旗标的话，give() 漏写一行完全看不出来。
    EXPECT_GT(s.itemCount(kHuangjing), huangjingBefore) << "照价那一条该发下黄精草";
    EXPECT_GT(s.itemCount(kZishen), 0) << "照价那一条该发下紫参草";

    // ===================== 节点 4 · 习御风决 =====================
    const int dayBeforeYufeng = s.day;
    ASSERT_TRUE(fireTrigger("trigger_yufeng", kFirst));
    EXPECT_EQ(flag("ch04.yufeng_xue"), 1);
    EXPECT_EQ(s.day - dayBeforeYufeng, kYufengDays);
    EXPECT_TRUE(spoke("ch04.yufeng.learn"));
    ASSERT_EQ(s.learnedMagics.size(), 2u) << "两门都该在清单上";
    // 顺序就是学会的先后（硬约束 1：火弹术在先，御风决在后）。
    EXPECT_EQ(s.learnedMagics[0], kFireMagic);
    EXPECT_EQ(s.learnedMagics[1], kWindMagic);

    // ---- 峰上那一年：开炉卖药 ----
    // 这不是给测试开的后门，它是节点 3 教的那件事本身，而且**非做不可**：
    // 战斗之内吃药是唯一的疗伤手段，本章要连打三场。料只有本章分次发下来的俸禄
    //（kailu.lua 头一个月 ＋ yufeng.lua 那一年），炼掉多少算多少。
    brew_ = brewEverything();
    ASSERT_GT(brew_.pills, 0) << "峰上那座炉子用本章发的料一瓶养精丹也炼不出来";
    EXPECT_EQ(brew_.warmups + brew_.attempts, kFurnacesOnThePriceBranch)
        << "照价那一条本章发的料正好够 " << kFurnacesOnThePriceBranch << " 炉（设计 3.3），"
        << "这一趟开了 " << (brew_.warmups + brew_.attempts) << " 炉：料的来路不对";
    EXPECT_GE(brew_.pills, kPillsBadCase)
        << "本章的料炼出 " << brew_.pills << " 瓶，低于设计 3.3 写的坏情况 " << kPillsBadCase
        << " 瓶——要么发料少了一批，要么这一趟的骰子比设计口径的坏情况还坏";

    // ===================== 节点 5 · 法武并用 =====================
    ASSERT_TRUE(travelTo(kMapYanwuchang, kFirst));
    ASSERT_EQ(s.mapId, kMapYanwuchang) << "御风决学会了，校场那道门该开了";

    // 防跳过：切磋还没打，开战那一节的 guard 不满足——从它上头走过去
    // **什么也不该发生**。这一条是地图给的闸，不是脚本给的。
    const MapObject kaizhan = objectNamed("trigger_kaizhan");
    ASSERT_FALSE(kaizhan.name.empty());
    ASSERT_EQ(kaizhan.property("guard_flag"), std::string("ch04.fawu_bingyong"));
    app_.clearSpokenKeys();
    ASSERT_TRUE(stepOntoWithoutPumping(kaizhan)) << "走不到开战那一格上";
    EXPECT_FALSE(app_.scripts().isRunning()) << "切磋还没打，号角不该响";
    EXPECT_TRUE(app_.lastSpokenKey().empty());

    const std::size_t battlesBeforeQiecuo = battlesFought_.size();
    ASSERT_TRUE(fireTrigger("trigger_qiecuo", kFirst));
    EXPECT_EQ(flag("ch04.fawu_bingyong"), 1) << "取第一项：说实话";
    ASSERT_EQ(battlesFought_.size(), battlesBeforeQiecuo + 1) << "这一节该打一场";
    EXPECT_TRUE(battlesFought_.back()) << "切磋没赢";
    EXPECT_TRUE(spoke("ch04.qiecuo.win")) << "赢了却演了别的落点：battle() 的返回值没接上";
    EXPECT_FALSE(spoke("ch04.qiecuo.lose"));
    EXPECT_FALSE(spoke("ch04.qiecuo.escape"));
    // 硬约束 1：法武并用是他自己摸出来的。凑出来的东西还没有名字。
    EXPECT_TRUE(spoke("ch04.qiecuo.noname"));

    // ===================== 节点 6 · 开战（三选一）=====================
    const int dayBeforeKaizhan = s.day;
    ASSERT_TRUE(nodeKaizhan(kFirst));
    EXPECT_EQ(flag("ch04.kaizhan"), 1);
    EXPECT_EQ(flag("ch04.bushu"), 1) << "取第一项：守辕门";
    EXPECT_EQ(s.day - dayBeforeKaizhan, kKaizhanDays);
    EXPECT_TRUE(spoke("ch04.kaizhan.yuanmen1"));
    EXPECT_FALSE(spoke("ch04.kaizhan.weijian1"));
    EXPECT_FALSE(spoke("ch04.kaizhan.xiadu1"));

    // 防跳过：贾天龙还没露面，殿上那一节的 guard 不满足。
    ASSERT_TRUE(travelTo(kMapGetang, kFirst));
    const MapObject jinguang = objectNamed("trigger_jinguang");
    ASSERT_FALSE(jinguang.name.empty());
    ASSERT_EQ(jinguang.property("guard_flag"), std::string("ch04.jia_tianlong"));
    app_.clearSpokenKeys();
    ASSERT_TRUE(stepOntoWithoutPumping(jinguang)) << "走不到殿门那一格上";
    EXPECT_FALSE(app_.scripts().isRunning()) << "山下那一趟还没走，殿上不该开演";

    // ===================== 节点 7 · 贾天龙现身 =====================
    const std::size_t battlesBeforeJia = battlesFought_.size();
    ASSERT_TRUE(nodeJia(kFirst));
    EXPECT_EQ(flag("ch04.jia_tianlong"), 1) << "取第一项：报假身份";
    ASSERT_EQ(battlesFought_.size(), battlesBeforeJia + 1) << "坊门底下该打一场";
    EXPECT_TRUE(spoke("ch04.jia.lie1"));
    EXPECT_FALSE(spoke("ch04.jia.silent1"));
    // 赢与输两条落点只能演一条。两条都没演 = battle() 的返回值没接上；
    // 两条都演了 = 脚本把 if/else 写成了两段顺序执行。
    EXPECT_NE(spoke("ch04.jia.win1"), spoke("ch04.jia.lose1"))
        << "坊门那一仗的胜负没能传回脚本";

    // ===================== 节点 8 · 金光上人与王绝楚 =====================
    // 这一节末尾是三天备战（复验 N-11）：坊门那一仗之后、攻防战之前本章唯一的日子。
    // 静养一天回上限一成，同伴一起——所以这一节前后，两个人的气血都要**真的**涨回来
    // 那三成（封顶于上限）。只看天数的话，一个把 advance_days 挪回坊门之前的改动
    // 在这里是看不出来的：天数总和不变。
    const int dayBeforeJinguang = s.day;
    const int hpBeforeRest = s.hp;
    const int companionBeforeRest = companionHp();
    ASSERT_TRUE(nodeJinguang(kFirst));
    EXPECT_EQ(s.day - dayBeforeJinguang, kBeizhanDays) << "三天备战该落在殿上那一场之后";
    {
        // 一天一成是契约写死的口径（docs/interfaces-p2.md 第 6 节），这里照抄，不读引擎那个常量。
        constexpr int kRestPercentPerDayFromContract = 10;
        const int perDay = s.maxHp * kRestPercentPerDayFromContract / 100;
        EXPECT_EQ(s.hp, std::min(s.maxHp, hpBeforeRest + perDay * kBeizhanDays))
            << "殿上那一场之后的三天，韩立的伤该缓回三成（一天一成）：" << hpBeforeRest << " → "
            << s.hp;
        const int companionPerDay = companionMax() * kRestPercentPerDayFromContract / 100;
        EXPECT_EQ(companionHp(),
                  std::min(companionMax(), companionBeforeRest + companionPerDay * kBeizhanDays))
            << "曲魂同样养三天：" << companionBeforeRest << " → " << companionHp();
        std::cout << "[ch04 三天备战] 韩立 " << hpBeforeRest << " → " << s.hp << "/" << s.maxHp
                  << "，曲魂 " << companionBeforeRest << " → " << companionHp() << "/"
                  << companionMax() << std::endl;
    }
    EXPECT_EQ(flag("ch04.jinguang_jian"), 1) << "取第一项：接话报小数目";
    EXPECT_TRUE(spoke("ch04.jinguang.answer1"));
    EXPECT_FALSE(spoke("ch04.jinguang.quiet1"));
    // 硬约束：他自始至终没有单独看韩立一眼。这两句是这件事的落点。
    EXPECT_TRUE(spoke("ch04.jinguang.nobody"));
    EXPECT_TRUE(spoke("ch04.jinguang.relief"));
    // 硬约束 4：王绝楚首见于此。
    EXPECT_TRUE(spoke("ch04.jinguang.wang"));

    // ===================== 节点 9 · 门派攻防战 =====================
    // 甬道口外第一格是上校场的必经之路，所以一踏上校场这一场就开演——
    // 与节点 6 的号角同一个道理，跳不过去。
    const std::size_t battlesBeforeSiege = battlesFought_.size();
    std::cout << "[ch04 攻防战开打前] 韩立 " << s.hp << "/" << s.maxHp << " 血 " << s.mp << "/"
              << s.maxMp << " 法力；曲魂 " << companionHp() << "/" << companionMax()
              << "；养精丹 " << s.itemCount(kPill) << std::endl;
    ASSERT_TRUE(nodeGongfang(kFirst));
    ASSERT_EQ(battlesFought_.size(), battlesBeforeSiege + 1) << "攻防战该打一场";
    ASSERT_TRUE(battlesFought_.back())
        << "门派攻防战输了。gongfang.lua 明写赢是唯一一种往下走的落点——"
           "先查本文件那只手（文件头第二段），再查编成";
    ASSERT_FALSE(app_.quitRequested()) << "走到了 game_over 那一条";
    EXPECT_EQ(flag("ch04.gongfang_zhan"), 1);
    EXPECT_TRUE(spoke("ch04.gongfang.plan_yuanmen")) << "ch04.bushu=1 该挑守辕门那一张";
    EXPECT_FALSE(spoke("ch04.gongfang.plan_weijian"));
    EXPECT_FALSE(spoke("ch04.gongfang.plan_xiadu"));
    EXPECT_TRUE(spoke("ch04.gongfang.win1"));
    EXPECT_FALSE(spoke("ch04.gongfang.lost1"));
    // 硬约束 5：门派存续，厉飞雨活着并接任外刃堂堂主。
    EXPECT_TRUE(spoke("ch04.gongfang.feiyu"));

    // ===================== 节点 10 · 战利品 =====================
    // 焦土那一节认的是 ch04.gongfang_zhan。**这道闸的反面在图上也走不出来**：
    // 甬道口是上校场唯一的路，去焦土的路上先把节点 9 打了。所以照前面几处的
    // 办法——人先站到焦土跟前，再把旗标撤掉按一下。
    const MapObject spoils = objectNamed("trigger_zhanlipin");
    ASSERT_FALSE(spoils.name.empty());
    EXPECT_EQ(spoils.property("guard_flag"), std::string("ch04.gongfang_zhan"));
    ASSERT_TRUE(faceObject(spoils, kFirst)) << "走不到焦土跟前";
    s.flags.erase("ch04.gongfang_zhan");
    app_.clearSpokenKeys();
    EXPECT_FALSE(world_.interact(app_)) << "仗还没打，灰里翻不出东西来";
    EXPECT_FALSE(app_.scripts().isRunning());
    s.setFlag("ch04.gongfang_zhan");

    ASSERT_EQ(s.itemCount(kPlaque), 0) << "先验：这之前身上没有那块牌子";
    ASSERT_EQ(s.itemCount(kTalisman), 0) << "先验：这之前身上没有剑符";
    ASSERT_TRUE(nodeZhanlipin(kFirst));
    EXPECT_EQ(flag("ch04.pai_dedao"), 1);
    EXPECT_EQ(flag("ch04.jianfu_dedao"), 1) << "取第一项：摊开试了一次";
    EXPECT_EQ(s.itemCount(kPlaque), 1) << "那块牌子该在背包里";
    EXPECT_EQ(s.itemCount(kTalisman), 1) << "剑符该在背包里";
    EXPECT_TRUE(spoke("ch04.zhanli.try1"));
    EXPECT_FALSE(spoke("ch04.zhanli.keep1"));

    // ===================== 节点 11 · 回村、留药、东去 =====================
    // 防跳过：牌子还没到手之前，村东巷口那一节不该开演——上面已经在
    // 校场验过一次同类的闸，这里验的是**另一张图上的另一个 guard**。
    const int dayBeforeHuicun = s.day;
    ASSERT_TRUE(nodeHuicun(kFirst));
    EXPECT_EQ(flag("ch04.hui_cun"), 1);
    EXPECT_EQ(s.day - dayBeforeHuicun, kHuicunDays) << "五天的路";
    // 章末那个布袋里是碎银和那块没有名字的牌子。**这里绝不点破它。**
    EXPECT_TRUE(spoke("ch04.huicun.end"));

    // 留药：这一趟他在峰上炼了一年药，三场仗打下来还剩得下三瓶，
    // 所以走的该是「摆了三只瓶子」那一条，而且**背包里真的要少三瓶**。
    // 另一条（翻遍箱底也没有）由下面那条同名的用例走。
    const EndBag bag = collectEndBag(kFirst);
    const BagVerdict verdict = chapterEndBag(bag);
    EXPECT_TRUE(verdict.ok) << verdict.why;
    EXPECT_TRUE(bag.saidHave) << "手上有药，却演了「翻遍箱底也没有」";
    EXPECT_EQ(bag.pillsAfter, bag.pillsBefore - 3) << "说留了三瓶就该真的少三瓶";
    EXPECT_EQ(flag("ch04.xin_neirong"), 1) << "取第一项：只写药怎么吃";

    // 东去。章末不给他任何志向（沿用第 3 章 ligu.lua 立下的口径）。
    const int dayBeforeDongqu = s.day;
    ASSERT_TRUE(nodeDongqu(kFirst));
    EXPECT_EQ(flag("ch04.done"), 1) << "本章的终点";
    EXPECT_EQ(s.day - dayBeforeDongqu, kDongquDays);
    EXPECT_TRUE(spoke("ch04.dongqu.last"));
    // 「跟上」——说给一个不会答话的人听的。曲魂还在队里。
    EXPECT_TRUE(spoke("ch04.dongqu.call"));
    ASSERT_EQ(s.party.size(), 1u);
    EXPECT_EQ(s.party[0].roleId, kCompanion);

    // 全章的日子：两处二选一都不改日程，所以这个合计与选哪一项无关。
    EXPECT_EQ(s.day - startDay_, kChapterDays)
        << "全章该走 " << kChapterDays << " 天，实走 " << (s.day - startDay_);
    // 三场仗一场不多一场不少。
    EXPECT_EQ(battlesFought_.size(), 3u) << "本章是切磋、坊门、攻防三场";

    // 交到第 5 章手里的就是这一份（每一处选择都取第一项的那一侧），
    // 契约 docs/interfaces-p3-ch05.md 第 3 节、tests/ChapterFixture.h。
    settleEndingAgainstFixture(fanren::test::kChapterFourEndingFirst);

    std::cout << "\n[ch04 通关实测] 十一个节点走完，" << (s.day - startDay_) << " 天，三场仗，"
              << "章末背包：牌子 " << s.itemCount(kPlaque) << " 剑符 " << s.itemCount(kTalisman)
              << " 养精丹 " << s.itemCount(kPill) << "，已习得法术 " << s.learnedMagics.size()
              << " 门" << std::endl;
}

// ---------------------------------------------------------------------------
// 另一侧：每一处选择都取另一项，照样走到 ch04.done
// ---------------------------------------------------------------------------
// 一条只走一侧的通关测试证明不了分支是活的：另一侧可以写成永远走不到的死路，
// 而那件事在任何单测里都看不出来。
//
// **节点 6 那一处从前是例外**：第二项（放进来围歼）当时是一条必死的分支，
// 于是这条用例改走第三项，并写明「第二项走不走得通不在本文件里假装验过」。
// 那是诚实的写法，但它掩盖的是一个真缺陷——独立校对把它判成 HIGH-2。
// 编成已按实测重调（见 b04_gongfang_weijian.json 的 note），例外随之取消：
// 这里回到「每一处都取第二项」。三项各自走一遍在上面那条 TEST_P 里。
TEST_F(Ch04Walkthrough, TheOtherSideOfEveryChoiceAlsoReachesTheEnd) {
    constexpr int kSecond = 1;
    // 起点也取另一侧：第 3 章「每一处都取第二项」那一趟交过来的终局（五毒水没交、
    // 毒配了双份，背包比第一侧多四样）。
    startFromChapterThreeEnding(fanren::test::kChapterThreeEndingSecond);
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(flag("ch03.yingdui_xuan"), 2) << "先验：读进来的确实是第 3 章另一侧的终局";
    GameState& s = state();

    playUpToTheSpoils(kSecond, kSecond);
    ASSERT_FALSE(HasFatalFailure()) << siegeLine();
    EXPECT_EQ(flag("ch04.liuxia"), 2) << "取第二项：本想推辞，还是留下了";
    EXPECT_EQ(flag("ch04.kailu"), 2) << "取第二项：只收药钱";
    // 只收药钱那一条头一个月多拨了料（kailu.lua），全章够 46 炉（设计 3.3）。
    EXPECT_EQ(brew_.warmups + brew_.attempts, kFurnacesOnTheCostBranch)
        << "只收药钱那一条本章发的料正好够 " << kFurnacesOnTheCostBranch << " 炉";
    EXPECT_GE(brew_.pills, kPillsBadCase);
    EXPECT_EQ(flag("ch04.fawu_bingyong"), 2) << "取第二项：含糊过去";
    EXPECT_EQ(flag("ch04.bushu"), 2) << "取第二项：辕门虚掩，放进空场合围";
    EXPECT_EQ(flag("ch04.jia_tianlong"), 2) << "取第二项：不吭声";
    EXPECT_EQ(flag("ch04.jinguang_jian"), 2) << "取第二项：不吭声";
    EXPECT_EQ(flag("ch04.jianfu_dedao"), 2) << "取第二项：收起来没摊开";
    // 三选一是本章唯一一处**真的换了战斗数据**的分支（gongfang.lua 首部）。
    const auto saidInSiege = [this](const std::string& key) {
        return std::find(siegeKeys_.begin(), siegeKeys_.end(), key) != siegeKeys_.end();
    };
    ASSERT_FALSE(siegeKeys_.empty()) << "先验：攻防战那一节确实演过";
    EXPECT_TRUE(saidInSiege("ch04.gongfang.plan_weijian"));
    EXPECT_FALSE(saidInSiege("ch04.gongfang.plan_yuanmen"));
    EXPECT_FALSE(saidInSiege("ch04.gongfang.plan_xiadu"));

    // 两件战利品与选哪一项无关：分的只是他知道多少。
    EXPECT_EQ(s.itemCount(kPlaque), 1);
    EXPECT_EQ(s.itemCount(kTalisman), 1);

    ASSERT_TRUE(nodeHuicun(kSecond));

    // 留药的另一侧。这一趟他手上其实还剩十几瓶（攻防战用掉八九瓶）；为了走到
    // 「翻遍箱底也没有」那一条，这里把药清掉——那一侧对应的是 xinbie.lua 首部说的
    // 另一种玩家：他这一章没去开炉，或者开了炉没炼成。**两侧都是合法的结局**，
    // 判据管的是「说的和做的一致」，不是他手上该有几瓶。
    while (s.removeItem(kPill, 1)) {
    }
    ASSERT_EQ(s.itemCount(kPill), 0);

    const EndBag bag = collectEndBag(kSecond);
    const BagVerdict verdict = chapterEndBag(bag);
    EXPECT_TRUE(verdict.ok) << verdict.why;
    EXPECT_TRUE(bag.saidNone) << "一瓶也没有，却演了「摆了三只瓶子」";
    EXPECT_EQ(bag.pillsAfter, 0) << "take() 扣不到就一瓶也不许扣";
    EXPECT_EQ(flag("ch04.xin_neirong"), 2) << "取第二项：写一句道别";

    ASSERT_TRUE(nodeDongqu(kSecond));
    EXPECT_EQ(flag("ch04.done"), 1) << "另一侧同样走得到章末";
    // 两处二选一都不改日程，所以两条路的天数必须一样——这一点是 kChapterDays
    // 那个常量的全部意义，只走一侧看不出来。
    EXPECT_EQ(s.day - startDay_, kChapterDays);

    // 另一侧的终局同样交给第 5 章。**这一侧上面那段清药是本用例的手摆**（为了走到
    // 「翻遍箱底也没有」），于是交过去的是 0 瓶养精丹——契约 3.1 写明了，第 5 章照实接。
    settleEndingAgainstFixture(fanren::test::kChapterFourEndingSecond);
}

// ---------------------------------------------------------------------------
// 三选一的**每一项**都走得到章末（独立校对 HIGH-2 的整改判据）
// ---------------------------------------------------------------------------
// 从前只有第 1、3 两项被走过。第 2 项（放进来围歼）是一条玩家点得到、
// 走进去必死的分支——输了 gongfang.lua 直接 game_over，这一章到此为止。
// 而它还处于**零覆盖**：把那张编成三波压成一波，全套测试一条也不红。
//
// **一条点得到却必死的选择不是选择，是陷阱**（设计文档 3.2 节原话）。
// 第 2 项该比第 1 项难——它是贪的那一条——但风险要落在**代价**上
//（掉多少血、耗掉几瓶药）而不是落在**不可能**上。
//
// 这一条是用 TEST_P 写的，为的是**三项各跑一次完整的通关**：
// 只挑一项跑等于又回到「另一侧可以是死路而没人看得见」。
// 起点也是两份：第 3 章两条分支交过来的终局各跑一遍（复验判据 1「两侧各一份」）。
// 药只来自本章分次发下来的俸禄（复验判据 2）；余量不再量在收场气血上——那是量错了轴，
// 真正的余量在药上，由 Ch04HandSweep 扫「少两瓶也不翻」。这里钉的是另一件设计写死的事：
// 打完攻防战，手上还剩得下节点 11 要留的三瓶（设计 3.3 那张表的最后一行）。
class Ch04EverySiegePlan : public Ch04Walkthrough,
                           public ::testing::WithParamInterface<std::tuple<int, int>> {};

TEST_P(Ch04EverySiegePlan, PlaysAllTheWayToTheEndOfTheChapter) {
    constexpr int kFirst = 0;
    const int plan = std::get<0>(GetParam());   // 0/1/2 → ch04.bushu 1/2/3
    const int side = std::get<1>(GetParam());   // 第 3 章哪一侧的终局
    if (side == 1) startFromChapterThreeEnding(fanren::test::kChapterThreeEndingSecond);
    ASSERT_FALSE(HasFatalFailure());
    GameState& s = state();

    playUpToTheSpoils(kFirst, plan);
    ASSERT_FALSE(HasFatalFailure()) << siegeLine();
    EXPECT_EQ(flag("ch04.bushu"), plan + 1) << "挑的不是这一项";
    EXPECT_EQ(brew_.warmups + brew_.attempts, kFurnacesOnThePriceBranch)
        << "照价那一条本章发的料正好够 " << kFurnacesOnThePriceBranch << " 炉：料的来路不对";
    EXPECT_GE(brew_.pills, kPillsBadCase)
        << "本章的料只炼出 " << brew_.pills << " 瓶，低于设计 3.3 的坏情况";
    std::cout << "[ch04 布置 " << (plan + 1) << "·第 3 章第 " << (side + 1) << " 侧] 炼出 "
              << brew_.pills << " 瓶；" << siegeLine() << std::endl;
    EXPECT_TRUE(siege_.won) << "布置 " << (plan + 1) << " 那一张编成打不赢：" << siegeLine();
    EXPECT_GE(siege_.pillsAfter, 3)
        << "打完攻防战手上不够节点 11 留的三瓶：设计 3.3 那张表把这三瓶算在发料里了——"
        << siegeLine();

    ASSERT_TRUE(nodeHuicun(kFirst));
    ASSERT_TRUE(nodeXinbie(kFirst));
    EXPECT_TRUE(spoke("ch04.xinbie.have")) << "手上有三瓶以上，该演「摆了三只瓶子」那一条";
    ASSERT_TRUE(nodeDongqu(kFirst));
    EXPECT_EQ(flag("ch04.done"), 1)
        << "布置 " << (plan + 1) << " 走不到章末";
    // 三项都不改日程：三条路的天数必须一样。
    EXPECT_EQ(s.day - startDay_, kChapterDays);
}

INSTANTIATE_TEST_SUITE_P(ThreeWaysToHoldTheGate, Ch04EverySiegePlan,
                         ::testing::Combine(::testing::Values(0, 1, 2), ::testing::Values(0, 1)),
                         [](const ::testing::TestParamInfo<std::tuple<int, int>>& info) {
                             std::string name;
                             switch (std::get<0>(info.param)) {
                                 case 0: name = "HoldTheGate"; break;
                                 case 1: name = "LetThemInAndSurround"; break;
                                 default: name = "PoisonTheWellFirst"; break;
                             }
                             return name + (std::get<1>(info.param) == 0
                                                ? "_FromChapterThreesFirstSide"
                                                : "_FromChapterThreesSecondSide");
                         });

// ---------------------------------------------------------------------------
// 那只手先证明自己是稳的（复验判据 3，技术债 G-11 / G-18）
// ---------------------------------------------------------------------------
// 拿一只手去量平衡之前，先确认它自己是稳的——否则量到的是它的脾气，不是内容
//（docs/README.md「判据对自己的取舍敏感」那一行）。从前这只手换一个吃药门槛、
// 背包里多一样东西，三张编成的胜负就翻，而 data/battles/ 一个字节没动。
//
// 这一条每次都把那三个方向各扫一遍，**三张编成的胜负一格也不许翻**：
//   · 吃药门槛：40、45、50、55、60、65、70；
//   · 药数：峰上那一趟用本章真发的料炼出来的数 N，以及 N−2、N＋2；
//   · 背包：原样、多一样（最前头塞一份金疮药——它会排在物品那一级的第一行，
//     正是从前那只手会先吃的位置）、少一样（拿掉第 3 章带来的那堆零年黄精——
//     从前那只手在第 3 章终局起点上吃掉的就是它）。
// 三张编成各 7 × 3 × 3 = 63 格，每一格从节点 5 一直打到节点 9（切磋、坊门两场的
// 吃药也在里头），起点是同一份「节点 4 走完、俸禄炼完」的世界。
//
// 另扫一张「最少要几瓶」：门槛 40 / 50 / 60 / 70，身上揣 2–10 瓶各打一遍。
// 设计 3.3 那张炉数表的第一行（kSiegeNeedFromDesign）就是它的最大值——
// 这里量出来揣够那个数还会输，表就过期了。
//
// ---- 这张表量得到什么、量不到什么（二次复验 R-11，别把三个维度都当成在把关）----
// **背包那一维与药数 ±2 那一维，对这只手按构造就翻不了**，三列必然逐字相同：
//   · 背包：这只手只认 pill_yangjing_dan 那一行（BattleHand::feed 按 id 找），别的东西它一概不碰，
//     所以多一份金疮药、少一堆零年黄精，它的每一步都一样。这一维能抓住的只有
//    「有人把手改回按行号取药」这一种回归（那时三列才会分开）——它是那条修法的看门狗，
//     不是平衡的量尺。
//   · 药数：这只手一场攻防战最多吃 11 瓶，21 / 23 / 25 瓶都远在够用那一侧，所以三列相同。
//     「少两瓶也不翻」在这里是**按余量成立**，不是被这张表量出来的；真正量药够不够的，
//     是下面那张「最少要几瓶」表（2–10 瓶逐一打）。
// 真正在量平衡的只有**门槛那一维**与「最少要几瓶」表。而门槛 40–45% 那几格之所以稳，
// 靠的是 BattleHand::tryHeal 里「对面下一轮打得死就吃」那一条：摘掉它，先下药在门槛四成就翻
//（docs/tech-debt.md G-11 的变异）。一个不会算下一轮的玩家落在这张表之外——
// 二次复验第三节扫了 19 种那样的手法，57 格里 18 格揣着满包药也输（登在 docs/tech-debt.md G-22）。
//
// 两张表都打到 stdout，整改报告里贴的就是它。
constexpr int kSweepThresholds[] = {40, 45, 50, 55, 60, 65, 70};
constexpr int kSweepPillDeltas[] = {-2, 0, 2};
constexpr int kNeedThresholds[] = {40, 50, 60, 70};
constexpr int kNeedMinPills = 2;
constexpr int kNeedMaxPills = 10;
constexpr const char* kStrayItem = "pill_jinchuang_yao";   // 多的那一样：坊门那一战掉的就是它

enum class BagVariant { AsIs, OneMore, OneLess };

const char* bagName(BagVariant bag) {
    switch (bag) {
        case BagVariant::AsIs: return "原样";
        case BagVariant::OneMore: return "多一样";
        case BagVariant::OneLess: return "少一样";
    }
    return "?";
}

class Ch04HandSweep : public Ch04Walkthrough, public ::testing::WithParamInterface<int> {
protected:
    struct Cell {
        int threshold = 0;
        int pills = 0;
        BagVariant bag = BagVariant::AsIs;
        SiegeRecord siege;
    };

    // 背包那一维。两条先验：动了就要真的动到（多的那一样确实排第一；少的那一样确实
    // 原来有、现在没有），否则那一列是在空转。
    void perturbBag(BagVariant bag) {
        GameState& s = state();
        if (bag == BagVariant::OneMore) {
            s.bag.insert(s.bag.begin(), fanren::core::BagEntry{kStrayItem, 1, 0});
            ASSERT_EQ(s.bag.front().itemId, kStrayItem);
        } else if (bag == BagVariant::OneLess) {
            ASSERT_GT(s.itemCount(kHuangjing), 0)
                << "先验：第 3 章带来的零年黄精该还在（本章的方子用不上它）";
            s.bag.erase(std::remove_if(s.bag.begin(), s.bag.end(),
                                       [](const fanren::core::BagEntry& e) {
                                           return e.itemId == kHuangjing;
                                       }),
                        s.bag.end());
            ASSERT_EQ(s.itemCount(kHuangjing), 0);
        }
    }

    void setPills(int count) {
        while (state().removeItem(kPill, 1)) {
        }
        if (count > 0) state().addItem(kPill, count, 0);
    }
};

TEST_P(Ch04HandSweep, NoThresholdPillCountOrStrayItemFlipsTheSiege) {
    constexpr int kFirst = 0;
    const int plan = GetParam();   // 0/1/2 → ch04.bushu 1/2/3

    playUpToTheBrewing(kFirst);
    ASSERT_FALSE(HasFatalFailure());
    const GameState afterBrewing = state();
    const int brewed = afterBrewing.itemCount(kPill);
    // 先验：N−2 那一列得是个真的数，不是负数或零（否则「少两瓶也不翻」无从谈起）。
    ASSERT_GE(brewed - 2, 1) << "本章的料只炼出 " << brewed << " 瓶";

    // ---- 表一：门槛 × 药数 × 背包 ----
    std::vector<Cell> cells;
    for (const int threshold : kSweepThresholds) {
        for (const int delta : kSweepPillDeltas) {
            for (const BagVariant bag : {BagVariant::AsIs, BagVariant::OneMore, BagVariant::OneLess}) {
                restoreTo(afterBrewing);
                ASSERT_FALSE(HasFatalFailure());
                setPills(brewed + delta);
                perturbBag(bag);
                ASSERT_FALSE(HasFatalFailure());
                hand_.policy().healAtPercent = threshold;
                playFromTheSparringToTheSiege(kFirst, plan);
                ASSERT_FALSE(HasFatalFailure());
                cells.push_back(Cell{threshold, brewed + delta, bag, siege_});
            }
        }
    }
    ASSERT_EQ(cells.size(), std::size(kSweepThresholds) * std::size(kSweepPillDeltas) * 3u)
        << "先验：每一格都打过了";

    std::cout << "\n[ch04 手的扫描] 布置 " << (plan + 1) << "，本章的料炼出 N=" << brewed
              << " 瓶。每格：胜负 收场气血/剩几瓶（开战前 韩立气血、曲魂气血、药）" << std::endl;
    for (const int threshold : kSweepThresholds) {
        std::cout << "  门槛 " << threshold << "%：";
        for (const Cell& cell : cells) {
            if (cell.threshold != threshold) continue;
            std::cout << " [" << cell.pills << "瓶·" << bagName(cell.bag) << " "
                      << (cell.siege.won ? "胜" : "负") << " " << cell.siege.hpAfter << "/"
                      << cell.siege.pillsAfter << "（" << cell.siege.hpBefore << ","
                      << cell.siege.companionBefore << "," << cell.siege.pillsBefore << "）]";
        }
        std::cout << std::endl;
    }
    for (const Cell& cell : cells) {
        EXPECT_TRUE(cell.siege.won)
            << "布置 " << (plan + 1) << " 在门槛 " << cell.threshold << "%、身上 " << cell.pills
            << " 瓶、背包" << bagName(cell.bag) << "时输了——这只手或这张编成不稳："
            << "攻防战前 韩立 " << cell.siege.hpBefore << "、曲魂 " << cell.siege.companionBefore
            << "、药 " << cell.siege.pillsBefore << "；收场 韩立 " << cell.siege.hpAfter;
    }

    // ---- 表二：最少要几瓶 ----
    std::cout << "[ch04 最少要几瓶] 布置 " << (plan + 1) << "（身上揣 " << kNeedMinPills << "–"
              << kNeedMaxPills << " 瓶各打一遍）" << std::endl;
    for (const int threshold : kNeedThresholds) {
        std::string row;
        int least = -1;
        for (int pills = kNeedMinPills; pills <= kNeedMaxPills; ++pills) {
            restoreTo(afterBrewing);
            ASSERT_FALSE(HasFatalFailure());
            setPills(pills);
            hand_.policy().healAtPercent = threshold;
            playFromTheSparringToTheSiege(kFirst, plan);
            ASSERT_FALSE(HasFatalFailure());
            row += " " + std::to_string(pills) + (siege_.won ? "胜" : "负");
            if (siege_.won && least < 0) least = pills;
            if (pills >= kSiegeNeedFromDesign) {
                EXPECT_TRUE(siege_.won)
                    << "设计 3.3 说攻防战最费的一档揣 " << kSiegeNeedFromDesign << " 瓶就够，"
                    << "布置 " << (plan + 1) << " 在门槛 " << threshold << "% 揣 " << pills
                    << " 瓶却输了——炉数表的第一行过期了，重量三张编成、重算那张表";
            }
        }
        std::cout << "  门槛 " << threshold << "%：" << row << "　→ 最少 " << least << " 瓶"
                  << std::endl;
    }
}

// ---------------------------------------------------------------------------
// 一炉不炼：照目标链走的玩家，在攻防战之前一定被提醒到（二次复验 R-3）
// ---------------------------------------------------------------------------
// 复验时的实情：身上 0 瓶走进攻防战，几乎任何打法都输，输即 game_over；而目标链在「开炉」
// 之后直接跳到「习御风决」「切磋」，全章没有一句话提醒要炼药。
//
// 现在是两道提醒，这一条两道都验：
//   1. 目标行：御风决学完，下一步就是「在丹炉炼好养精丹」，标记落在那座丹炉上；
//   2. 攻防战前：身上 0 瓶，厉飞雨在甬道口问一句，第一项回峰上开炉——**不开打**，
//      挂点留着；炼完回来再踩，这一回不问、仗照打。
// 这一趟每一处选择都取第一项，所以那一问走的是「回峰上开炉」。
TEST_F(Ch04Walkthrough, FollowingTheChainWithoutBrewingIsRemindedBeforeTheSiege) {
    constexpr int kFirst = 0;
    GameState& s = state();

    ASSERT_TRUE(nodeLiuxia(kFirst));
    ASSERT_TRUE(nodeHuodan(kFirst));
    ASSERT_TRUE(nodeKailu(kFirst));
    ASSERT_TRUE(nodeYufeng(kFirst));

    // ---- 第一道：目标行 ----
    const fanren::core::Objective* step = fanren::rules::currentObjective(app_.data().objectives, s);
    ASSERT_NE(step, nullptr) << "御风决学完，目标链不该已经走完";
    EXPECT_EQ(step->id, kBrewStepId) << "御风决之后，目标行该是炼药那一步，实为 " << step->id;
    EXPECT_EQ(step->targetMap, kBrewStepMap) << "炼药那一步该指着落日峰";
    EXPECT_EQ(step->targetObject, kBrewStepObject) << "炼药那一步的标记该落在那座丹炉上";
    const std::string line = app_.data().lookupText(step->textKey);
    EXPECT_FALSE(line.empty());
    EXPECT_NE(line, step->textKey) << "目标行的文案不在：画面上会显示一串 key";

    // ---- 一炉不炼，照样往下走 ----
    ASSERT_EQ(s.itemCount(kPill), 0) << "先验：一瓶养精丹也没炼";
    ASSERT_TRUE(nodeQiecuo(kFirst));
    EXPECT_EQ(flag(kBrewFlag), 0) << "空着手去切磋，炼药那一步不该算做过";
    ASSERT_TRUE(nodeKaizhan(kFirst));
    ASSERT_TRUE(nodeJia(kFirst));
    ASSERT_TRUE(nodeJinguang(kFirst));
    ASSERT_EQ(s.itemCount(kPill), 0) << "先验：走到攻防战跟前，身上仍是 0 瓶";

    // ---- 第二道：攻防战前厉飞雨那一问 ----
    const std::size_t battlesBefore = battlesFought_.size();
    ASSERT_TRUE(nodeGongfang(kFirst));
    EXPECT_TRUE(spoke(kNoPillLine)) << "身上 0 瓶走进攻防战，厉飞雨该问那一句";
    EXPECT_TRUE(spoke(kNoPillBackLine)) << "取第一项：回峰上开炉";
    EXPECT_EQ(battlesFought_.size(), battlesBefore) << "回峰上去了，这一仗不该开打";
    EXPECT_EQ(flag("ch04.gongfang_zhan"), 0) << "没打就不该算打过：挂点得留着，回来再踩";
    EXPECT_FALSE(app_.quitRequested());

    // ---- 回峰上炼完，再来：这一回不问，仗照打 ----
    brew_ = brewEverything();
    ASSERT_GT(brew_.pills, 0) << "本章的料炼不出药";
    ASSERT_TRUE(nodeGongfang(kFirst));
    EXPECT_FALSE(spoke(kNoPillLine)) << "揣着药回来，不该再问";
    ASSERT_EQ(battlesFought_.size(), battlesBefore + 1) << "这一回该开打了";
    EXPECT_TRUE(battlesFought_.back()) << "炼完药回来，这一仗该打得赢：" << siegeLine();
    EXPECT_EQ(flag("ch04.gongfang_zhan"), 1);
    EXPECT_FALSE(app_.quitRequested());
}

// 反例：炼够了药，一句提醒也不出现，炼药那一步在切磋时就算做过。
// 少了这一条，「无论有没有药都问一句」的写法也能让上面那条绿。
TEST_F(Ch04Walkthrough, WithPillsInHandTheSiegeStartsWithoutTheReminder) {
    constexpr int kFirst = 0;
    playUpToTheBrewing(kFirst);
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_GE(state().itemCount(kPill), kPillsBadCase) << "先验：本章的料都炼成了药";

    ASSERT_TRUE(nodeQiecuo(kFirst));
    EXPECT_EQ(flag(kBrewFlag), 1) << "揣着养精丹去切磋，炼药那一步就算做过了";
    const fanren::core::Objective* step =
        fanren::rules::currentObjective(app_.data().objectives, state());
    ASSERT_NE(step, nullptr);
    EXPECT_EQ(step->id, "n6_kaizhan") << "切磋之后目标行该到布置迎敌，实为 " << step->id;

    ASSERT_TRUE(nodeKaizhan(kFirst));
    ASSERT_TRUE(nodeJia(kFirst));
    ASSERT_TRUE(nodeJinguang(kFirst));
    ASSERT_GT(state().itemCount(kPill), 0);
    const std::size_t battlesBefore = battlesFought_.size();
    ASSERT_TRUE(nodeGongfang(kFirst));
    EXPECT_FALSE(spoke(kNoPillLine)) << "身上有药，厉飞雨不该问";
    EXPECT_FALSE(spoke(kNoPillBackLine));
    ASSERT_EQ(battlesFought_.size(), battlesBefore + 1) << "有药就直接开打";
    EXPECT_TRUE(battlesFought_.back());
}

// 那只手按 id 取药，直接验一次：物品那一级第一行摆的是金疮药，后面还有第 3 章带来的
// 零年黄精，它吃下去的仍得是养精丹，别的一样不动（复验判据 3、技术债 G-18）。
// 与上面那张扫描表形状不同：扫描看的是「胜负翻不翻」，这一条看的是「这一口吃的是什么」——
// 从前那只手取「第一条点得动的」，而扫描表上它未必输，这一条它必红。
TEST_F(Ch04Walkthrough, TheHandFeedsTheYangjingPillByIdNotWhateverRowComesFirst) {
    GameState& s = state();
    s.bag.insert(s.bag.begin(), fanren::core::BagEntry{kStrayItem, 2, 0});
    s.addItem(kPill, 3, 0);
    // 这一条验的是「吃下去的是哪一样」，不是「该不该吃」：门槛拉到十成，轮到他就一定吃。
    // 气血留满。从前压到三成（够得上扫描的每一档门槛），横版里坊门那四个人第一回合就都
    // 够得着他、又都比他快，三成血撑不到他自己出手，下面「头一个轮到的是韩立」就落空了。
    s.hp = s.maxHp;
    hand_.policy().healAtPercent = 100;
    const int hjBefore = s.itemCount(kHuangjing);
    ASSERT_GT(hjBefore, 0) << "先验：第 3 章带来的零年黄精在背包里（它也点得动，每株回 6 点）";

    BattleScene fight("b04_jia_tianlong");
    fight.onEnter(app_);
    const int actor = fight.runToAllyTurn();
    ASSERT_GE(actor, 0);
    ASSERT_EQ(fight.battle().units()[static_cast<std::size_t>(actor)].id, "hanli")
        << "先验：头一个轮到的是韩立（身法 5，曲魂 2）";
    // 先验：物品那一级第一行确实**不是**养精丹，否则「按 id 取」与「取第一行」分不出来。
    std::vector<std::string> ids;
    static_cast<void>(BattleScene::buildItemItems(app_.data(), s, fight.battle(), actor, ids));
    ASSERT_FALSE(ids.empty());
    ASSERT_NE(ids.front(), kPill) << "物品那一级第一行就是养精丹，这一条什么也验不出";

    ASSERT_TRUE(hand_.tryHeal(fight, actor)) << "门槛十成、身上有养精丹，这只手却没吃";
    EXPECT_EQ(s.itemCount(kPill), 2) << "吃下去的不是养精丹";
    EXPECT_EQ(s.itemCount(kStrayItem), 2) << "排在第一行的金疮药被当成养精丹吃了";
    EXPECT_EQ(s.itemCount(kHuangjing), hjBefore) << "第 3 章带来的零年黄精被当成养精丹吃了";
}

INSTANTIATE_TEST_SUITE_P(ThreeWaysToHoldTheGate, Ch04HandSweep, ::testing::Values(0, 1, 2),
                         [](const ::testing::TestParamInfo<int>& info) {
                             switch (info.param) {
                                 case 0: return std::string("HoldTheGate");
                                 case 1: return std::string("LetThemInAndSurround");
                                 default: return std::string("PoisonTheWellFirst");
                             }
                         });

// ---------------------------------------------------------------------------
// 引擎前置一 · 法术：他在节点 2 学会的那一门，战斗菜单里真的选得到
// ---------------------------------------------------------------------------
// tests/Ch04MagicTests.cpp 已经把这条链路的规则层逐条钉过，但它用的是 data 里
// 本来就有的**火球术**（写那一批时火弹术还没进 data），战斗也是第 3 章那一场狼。
// 缺的正是玩家实际走的那一段：**火弹术是他在落日峰上学会的，而这一章的仗
// 是用它打的。** 换句话说，那一批证明了「随便哪一门法术都走得通」，
// 这一条证明「这一门、这一章，确实走通了」。
// ---------------------------------------------------------------------------
// 这一章自己把他从第 3 章丢下他的地方接上去
// ---------------------------------------------------------------------------
// 独立校对 CRITICAL-1 的正面判据。从前这条根本不存在：起点是挑出来的，
// 于是「他怎么变强的」这个问题在全章没有任何一处回答得出来。
//
// 三处取值都抄自 docs/ch04-design.md 1.2 节，**不从脚本读**——
// 脚本里那三个 realm.advance 的参数写错了，正该由这一条抓住。
// 形状与通关测试刻意不同：通关测试问「走不走得完」，这一条问「他现在是几层」。
TEST_F(Ch04Walkthrough, TheChapterItselfRaisesHimFromWhereChapterThreeLeftHim) {
    constexpr int kFirst = 0;
    GameState& s = state();

    // 先验：起点真的是上一章交过来的那个数。这一条塌了，下面三条都没有意义。
    ASSERT_EQ(s.realm, kChapterThreeEnding)
        << "起点不是第 3 章章末那个数——这条测试又在自己挑起点了";
    ASSERT_EQ(s.maxMp, fanren::rules::realmMaxMp(kChapterThreeEnding));

    // 三处都不许静默失败（复验 N-5）。契约 6.2：编号非法或目标低于当前会回填 false，
    // 三个脚本都会在那时说出 ch04.jinjie.fail。
    //
    // **每一节演完当场问**，不留到末尾：fireTrigger / enterTrigger 每进一节都清一次台词本，
    // 末尾再问只看得见节点 4——从前那一条就是这样，节点 1 被回绝时它照样绿。
    // 先验那一句（这一节的升境台词确实说过）保证「没说 fail」不是因为这一段压根没演。
    const auto advancedWithoutRefusal = [this](const char* node, const char* jinjieKey) {
        EXPECT_TRUE(spoke(jinjieKey)) << node << "：升境那一段没演到（" << jinjieKey << "）";
        EXPECT_FALSE(spoke("ch04.jinjie.fail"))
            << node << "：realm.advance 被回绝了——多半是脚本里那个境界编号写错";
    };

    ASSERT_TRUE(nodeLiuxia(kFirst));
    advancedWithoutRefusal("节点 1", "ch04.liuxia.jinjie2");
    EXPECT_EQ(s.realm, kRealmAfterLiuxia)
        << "节点 1：药房的钥匙到手，那半年他是随便吃药过来的（原著 ch43）";

    ASSERT_TRUE(nodeHuodan(kFirst));
    advancedWithoutRefusal("节点 2", "ch04.huodan.jinjie");
    EXPECT_EQ(s.realm, kRealmAfterHuodan)
        << "节点 2：ch65「不断的服用灵药……不久后就会进入到第七层境界」";

    ASSERT_TRUE(nodeKailu(kFirst));
    EXPECT_EQ(s.realm, kRealmAfterHuodan) << "节点 3 不涨：开炉那一节是经营，不是修炼";

    ASSERT_TRUE(nodeYufeng(kFirst));
    advancedWithoutRefusal("节点 4", "ch04.yufeng.jinjie");
    EXPECT_EQ(s.realm, kRealmAfterYufeng)
        << "节点 4：ch75「每天把灵药当零食来吃中，悄悄的进入到了第八层」";

    // 上限跟着长，而且是**按境界基准长的**，不是脚本另算一套。
    EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(kChapterEndRealm));
    EXPECT_EQ(s.maxMp, fanren::rules::realmMaxMp(kChapterEndRealm));
    // 本章的尺子：设计 1.2 写死炼气八层法力 80。`ch04.gongfang.count` 那句台词里的
    // 「八十来点力气」与这个数的对应，由 Ch04AcceptanceTests 的
    // TheSiegeLineCountsTheManaTheDesignGivesTheEighthLayer 直接读文案核对（复验 N-6）。
    EXPECT_EQ(s.maxMp, 80) << "设计 1.2：炼气八层法力 80";

    // 「三处都不许静默失败」已在每一节演完时当场问过（上面 advancedWithoutRefusal）。
}

// 章末仍是炼气八层：本章没有第四次上涨，而节点 5 之后的三场仗都是按这个数平衡的。
TEST_F(Ch04Walkthrough, HeLeavesTheChapterAtTheLayerTheNovelSaysHeIsAt) {
    constexpr int kFirst = 0;
    GameState& s = state();
    playUpToTheSpoils(kFirst, kFirst);
    EXPECT_EQ(s.realm, kChapterEndRealm)
        << "原著 ch96 那时他仍是第八层；本章不该在节点 4 之后再涨";
    ASSERT_TRUE(nodeHuicun(kFirst));
    ASSERT_TRUE(nodeXinbie(kFirst));
    ASSERT_TRUE(nodeDongqu(kFirst));
    EXPECT_EQ(s.realm, kChapterEndRealm) << "章末交给第 5 章的也是这个数";
}

// 剧情境界上限（技术债 G-14）：章末带着这一章攒得下的最多修为，自己按突破也按不过八层。
//
// 判据全是字面量：八层出自设计 1.2 与硬约束 8「本章结束时是炼气八层」；365 是通关测试路径上
// 章末修为的最大值：第 3 章带进来的 15（谷外那一仗，交接存档里就是这个数）＋ 切磋 40 ＋ 坊门 90
// ＋ 攻防战最多的那一张 220（围歼），各取自 data/battles/b04_*.json 的 rewards。
//（docs/tech-debt.md G-14 那张表写的 350 是手搭起点时的数，那时起点修为是 0。）
// 上限不是手摆的：它是节点 1 / 2 / 4 那三句 realm.advance 顺带抬上去的。
TEST_F(Ch04Walkthrough, AtTheChapterEndEvenTheMostCultivationCannotPushPastTheEighthLayer) {
    constexpr int kFirst = 0;
    constexpr int kMostCultivationAtTheChapterEnd = 15 + 40 + 90 + 220;
    GameState& s = state();
    ASSERT_EQ(s.realmCap, Realm::QiRefining3) << "先验：起点的上限就是第 3 章交过来的三层";
    playUpToTheSpoils(kFirst, kFirst);
    ASSERT_TRUE(nodeHuicun(kFirst));
    ASSERT_TRUE(nodeXinbie(kFirst));
    ASSERT_TRUE(nodeDongqu(kFirst));
    ASSERT_EQ(s.realm, Realm::QiRefining8) << "先验：章末是八层";
    EXPECT_EQ(s.realmCap, Realm::QiRefining8)
        << "节点 4 的 realm.advance(八层) 该顺带把上限抬到八层";

    s.cultivation = kMostCultivationAtTheChapterEnd;
    ASSERT_GE(s.cultivation, fanren::rules::cultivationNeeded(Realm::QiRefining8))
        << "先验：这份修为确实越过了八层的门槛，否则「按不动」什么也证明不了";

    const auto& words = fanren::game::cultivationLexicon(fanren::game::wordingStage(s));
    const auto items = fanren::game::CultivationScene::buildMainItems(s);
    ASSERT_EQ(items.size(), 3u);
    EXPECT_FALSE(items[1].enabled) << "章末那一行冲关不该点得动";
    EXPECT_EQ(items[1].disabledReason, words.pushAtStoryCap) << "置灰理由该说是卡住了，不是差几点";

    fanren::game::CultivationScene panel;
    const auto attempt = panel.breakthrough(app_);
    EXPECT_EQ(attempt.blocked, fanren::rules::BreakthroughBlock::StoryCap);
    EXPECT_FALSE(attempt.success);
    EXPECT_EQ(s.realm, Realm::QiRefining8) << "章末按出了九层：硬约束 8 与「八十来点力气」都对不上了";
    EXPECT_GE(s.cultivation, kMostCultivationAtTheChapterEnd) << "按不下去不该扣修为";
    EXPECT_EQ(panel.feedback(), words.atStoryCap) << "反馈该是瓶颈那一句，不是运气差";
}

TEST_F(Ch04Walkthrough, TheFireMagicHeLearnedOnThePeakIsOnTheMenuOfThisChaptersFight) {
    constexpr int kFirst = 0;

    // ---- 对照组：还没学之前，施法那一项是灰的，而且说得出为什么 ----
    // 少了这一半，下面的「亮了」可能只是因为这一项一直是亮的。
    {
        BattleScene before("b04_qiecuo_feiyu");
        before.onEnter(app_);
        ASSERT_FALSE(before.battle().hasMagic(kFireMagic))
            << "一门都没学，本场登记表里不该有火弹术";
        const int actor = before.runToAllyTurn();
        ASSERT_GE(actor, 0);
        before.openMenu(app_);
        ASSERT_EQ(before.menuMode(), BattleMenuMode::Root);
        const fanren::ui::ListItem& cast =
            before.menuList().items()[static_cast<std::size_t>(fanren::game::kBattleMenuCast)];
        EXPECT_FALSE(cast.enabled);
        // 先验理由真有内容，再验它说的是哪一件事——只写后一句的话，
        // 理由字符串变成空的时候这条断言照样绿。
        ASSERT_FALSE(cast.disabledReason.empty()) << "点不动却一个字不说";
        EXPECT_NE(cast.disabledReason.find("还没学过"), std::string::npos) << cast.disabledReason;
    }

    // ---- 走到节点 2，让他真的学会 ----
    ASSERT_TRUE(nodeLiuxia(kFirst));
    ASSERT_TRUE(nodeHuodan(kFirst));
    ASSERT_EQ(flag("ch04.huodan_xue"), 1);
    ASSERT_EQ(state().learnedMagics.size(), 1u);
    ASSERT_EQ(state().learnedMagics[0], kFireMagic);

    const fanren::core::Magic* magic = app_.data().findMagic(kFireMagic);
    ASSERT_NE(magic, nullptr) << "data/magics/huodan_shu.json 没读进来";
    ASSERT_FALSE(magic->name.empty());

    // ---- 落盘：存一趟档再读回来，这一门还在 ----
    // Ch04MagicTests 钉过存档往返的每一个字段，那是规则层；这一条钉的是
    // **玩家真的走到那一步之后存的那份档**。
    {
        const fs::path save = fanren::test::uniqueTempPath("fanren_ch04_magic", ".json");
        auto wrote = fanren::io::saveGame(state(), save.string());
        ASSERT_TRUE(wrote.ok) << wrote.error;
        auto read = fanren::io::loadGame(save.string());
        ASSERT_TRUE(read.ok) << read.error;
        std::error_code ec;
        fs::remove(save, ec);
        ASSERT_EQ(read.value.learnedMagics.size(), 1u)
            << "存了一趟档，法术没了——那在游戏里长得就像「这个功能还没做」";
        EXPECT_EQ(read.value.learnedMagics[0], kFireMagic);
        EXPECT_EQ(read.value.flag("ch04.huodan_xue"), 1);
    }

    // ---- 战场：这一章第一场仗（节点 5 的切磋）里，它真的点得出来 ----
    BattleScene fight("b04_qiecuo_feiyu");
    fight.onEnter(app_);
    ASSERT_TRUE(fight.battle().hasMagic(kFireMagic))
        << "学会的法术没进本场登记表，菜单里就永远见不到它";
    ASSERT_FALSE(fight.battle().units().empty());
    ASSERT_EQ(fight.battle().units()[0].magics.size(), 1u) << "韩立是 0 号单位";
    EXPECT_EQ(fight.battle().units()[0].magics[0], kFireMagic);
    EXPECT_TRUE(fight.battle().units()[0].magicsExhaustive)
        << "第 3 章定的口径：声明几条就是几条，不许退回「战场登记的都会」";

    const int mpBefore = fight.battle().units()[0].mp;
    bool cast = false;
    for (int turn = 0; turn < 30 && !cast; ++turn) {
        const int actor = fight.runToAllyTurn();
        ASSERT_GE(actor, 0) << "战斗先结束了，这一条就什么也没测到";
        fight.openMenu(app_);
        ASSERT_EQ(fight.menuMode(), BattleMenuMode::Root);
        const fanren::ui::ListItem& castRow =
            fight.menuList().items()[static_cast<std::size_t>(fanren::game::kBattleMenuCast)];
        ASSERT_TRUE(castRow.enabled) << "学过了，这一项就该亮：" << castRow.disabledReason;
        ASSERT_TRUE(fight.menuChoose(app_, fanren::game::kBattleMenuCast));
        ASSERT_EQ(fight.menuMode(), BattleMenuMode::Magic);

        int row = -1;
        for (int i = 0; i + 1 < fight.menuList().count(); ++i) {
            if (fight.menuList().items()[static_cast<std::size_t>(i)].label == magic->name) {
                row = i;
                break;
            }
        }
        ASSERT_GE(row, 0) << "施法列表里找不到他刚在峰上学会的那一门";
        const fanren::ui::ListItem& magicRow =
            fight.menuList().items()[static_cast<std::size_t>(row)];
        if (!magicRow.enabled) {
            // 够不着。禁用理由照样要有内容（本项目的硬口径），然后凝气等一轮。
            ASSERT_FALSE(magicRow.disabledReason.empty());
            fight.menuBack(app_);
            ASSERT_EQ(fight.menuMode(), BattleMenuMode::Root);
            ASSERT_TRUE(fight.menuChoose(app_, fanren::game::kBattleMenuDefend));
            continue;
        }
        ASSERT_TRUE(fight.menuChoose(app_, row));
        ASSERT_EQ(fight.menuMode(), BattleMenuMode::Target);
        int target = -1;
        for (int i = 0; i < fight.menuList().count(); ++i) {
            const fanren::ui::ListItem& t =
                fight.menuList().items()[static_cast<std::size_t>(i)];
            if (t.enabled && t.label != "返回") {
                target = i;
                break;
            }
        }
        ASSERT_GE(target, 0);
        ASSERT_TRUE(fight.menuChoose(app_, target));
        cast = true;
    }
    ASSERT_TRUE(cast) << "三十个回合都没能把火弹术放出去";
    // 法力真的扣了：只看日志的话，一条「写日志但什么都没做」的实现照样全绿。
    EXPECT_EQ(fight.battle().units()[0].mp, mpBefore - magic->needMp);
    const auto& log = fight.battle().log();
    EXPECT_TRUE(std::any_of(log.begin(), log.end(), [&](const std::string& line) {
        return line.find(magic->name) != std::string::npos;
    })) << "战斗日志上没有这一击";
}

// ---------------------------------------------------------------------------
// 引擎前置二 · 多波次：攻防战第二波在第一波清空之后才入场，而且波间不回血
// ---------------------------------------------------------------------------
// tests/Ch04WaveTests.cpp 已经把推波、不回血、加载器校验逐条钉过，用的是就地
// 搭出来的两波编成。缺的是**玩家走到节点 9 时打的那一场**：编成由节点 6 的
// ch04.bushu 挑，场子是校场那张图，出手走的是动作菜单。
//
// 「不回血」那一条写成**加一行回血就会转红**：先验「转场之前他确实带着伤」
//（分母 > 0），再验转场之后那个数一个字节没变。少了先验那一半，一份把所有人
// 回满血的实现照样能让「hp == hpBefore」在满血局面下全绿。
TEST_F(Ch04Walkthrough, TheSiegeBringsTheNextWaveInOnlyAfterTheOneBeforeIsClearedAndNobodyIsHealed) {
    constexpr int kFirst = 0;
    ASSERT_TRUE(nodeLiuxia(kFirst));
    ASSERT_TRUE(nodeHuodan(kFirst));
    ASSERT_TRUE(nodeKailu(kFirst));
    ASSERT_TRUE(nodeYufeng(kFirst));
    ASSERT_TRUE(nodeQiecuo(kFirst));
    ASSERT_TRUE(nodeKaizhan(kFirst));
    ASSERT_EQ(flag("ch04.bushu"), 1) << "取第一项：守辕门";

    // 节点 6 挑的那一张。脚本里那条映射见 gongfang.lua；这里照它取同一个 id，
    // 而「脚本真的挑了这一张」由上面那条通关测试的 plan_yuanmen 断言钉着。
    BattleScene siege("b04_yelangbang_laifan");
    siege.onEnter(app_);

    const int waves = siege.battle().waveCount();
    ASSERT_GT(waves, 1) << "门派攻防战是多波次的（设计第 2.2 节）；只有一波就什么也没测到";
    ASSERT_EQ(siege.battle().currentWave(), 0) << "开场必须在第一波";
    // 界面上说得出来（契约第 2.3 节）。
    EXPECT_FALSE(fanren::game::waveStatusText(siege.battle()).empty())
        << "多波次战斗界面上必须看得见打到哪一波了";

    // 后面几波此刻不在场上，而且**点不着**——菜单里它们连一行都不该有，
    // 拿动作直接喂过去也要被回绝，且理由是「尚未入场」而不是「已经倒下」。
    // 问之前先把回合推到我方：checkLegal 在轮到谁之前就把话说死了，
    // 那样拿到的理由是「还没轮到你」，测不到入场那一条。
    const int myTurn = siege.runToAllyTurn();
    ASSERT_GE(myTurn, 0);
    int notYet = 0;
    for (std::size_t i = 0; i < siege.battle().units().size(); ++i) {
        const Unit& unit = siege.battle().units()[i];
        if (unit.ally || unit.wave == 0) continue;
        ++notYet;
        EXPECT_FALSE(unit.onField) << unit.name << " 还没到入场的时候，却已经在场上了";
        Action shot;
        shot.kind = ActionKind::Attack;
        shot.actorIndex = myTurn;
        shot.targetIndex = static_cast<int>(i);
        std::string why;
        EXPECT_FALSE(siege.battle().isLegal(shot, &why));
        ASSERT_FALSE(why.empty());
        EXPECT_NE(why.find("尚未入场"), std::string::npos) << why;
    }
    ASSERT_GT(notYet, 0) << "先验：这一场确实有几位还没进场的，不然上面那一圈空转";

    // 打到第一波清空。**不直接把 hp 写成 0**：那样走不到推波那一段。
    int guard = 0;
    while (siege.battle().phase() == BattlePhase::Ongoing &&
           siege.battle().currentWave() == 0 && guard < kMaxBattleSteps) {
        ++guard;
        const int actor = siege.runToAllyTurn();
        if (actor < 0) break;
        ASSERT_TRUE(hand_.act(siege, actor)) << "这一手一件也点不动";
    }

    ASSERT_EQ(siege.battle().phase(), BattlePhase::Ongoing)
        << "第一波清空就算赢了：那这一场根本没有第二波";
    ASSERT_EQ(siege.battle().currentWave(), 1) << "第一波清空之后该进第二波";

    // 第一波的人一个不剩，第二波的人进来了。
    for (const Unit& unit : siege.battle().units()) {
        if (unit.ally) continue;
        if (unit.wave == 0) EXPECT_FALSE(unit.alive()) << unit.name << " 该倒下了";
        if (unit.wave == 1) EXPECT_TRUE(unit.onField) << unit.name << " 该入场了";
    }

    // ---- 波间不回血、不回法力 ----
    //
    // 先验（分母 > 0）：**我方确实有人掉了血或者花了法力**。
    // 不指定是谁——那要看第一波怎么打的，写死「韩立此刻该掉血」就是把一次运行
    // 的样子当成了规格（docs/README.md 那张表的最后一行）。判据只要求
    // 「有一个数低于上限」，而过了波之后每一个数都一个字节没变。
    struct Vital {
        std::string name;
        int hp = 0;
        int mp = 0;
        int maxHp = 0;
        int maxMp = 0;
    };
    std::vector<Vital> before;
    bool somebodySpent = false;
    for (const Unit& unit : siege.battle().units()) {
        if (!unit.ally || !unit.alive()) continue;
        before.push_back(Vital{unit.name, unit.hp, unit.mp, unit.maxHp, unit.maxMp});
        if (unit.hp < unit.maxHp || unit.mp < unit.maxMp) somebodySpent = true;
    }
    ASSERT_FALSE(before.empty()) << "先验：我方还有人站着";
    ASSERT_TRUE(somebodySpent)
        << "先验：第一波打完，我方总得有人掉了血或者花了法力（分母 > 0）——"
           "全员满血满法力的话，下面那一组「一个字节没变」是恒真的";

    // 推波那一刻已经过去了：这些数此刻不该被谁补回来。
    std::size_t at = 0;
    for (const Unit& unit : siege.battle().units()) {
        if (!unit.ally || !unit.alive()) continue;
        ASSERT_LT(at, before.size());
        EXPECT_EQ(unit.hp, before[at].hp)
            << unit.name << " 过了一波就回血：那么「省着打」这件事在攻防战里根本不存在";
        EXPECT_EQ(unit.mp, before[at].mp) << unit.name << " 过了一波就回法力";
        ++at;
    }
    // 界面上那一行跟着动了。
    EXPECT_FALSE(fanren::game::waveStatusText(siege.battle()).empty());

    std::cout << "\n[ch04 波次实测] " << siege.battle().waveCount() << " 波，第一波清空时 "
              << before.front().name << " " << before.front().hp << "/" << before.front().maxHp
              << " 血、" << before.front().mp << "/" << before.front().maxMp
              << " 法力，过波之后一个字节没变" << std::endl;
}

// ---------------------------------------------------------------------------
// 引擎前置三 · 炼丹：节点 3 之后那座炉子能成能败，开不了工时说得清缺什么
// ---------------------------------------------------------------------------
// tests/Ch04AlchemyTests.cpp 已经把加载器、面板条目、成功率、失败策略逐条钉过，
// 用的是金疮药方（难度 5、熟练度门槛 0）。缺的是玩家实际走的那一段：
//   · 节点 3 之前按那座炉子，应声的是剧情；节点 3 之后 once 烧掉、触发器让位，
//     同一下确认键开的才是炼丹面板（kailu.lua 首部那段接线）；
//   · 本章真正要炼的是**养精丹**（节点 11 留给厉飞雨的那一味），
//     它有熟练度门槛 6，而玩家的起点是 0——火候未到那句话在这一章是真的会遇到的。
TEST_F(Ch04Walkthrough, TheFurnaceOnThePeakOnlyAnswersAfterTheSceneAndThenCanSucceedOrBlowUp) {
    constexpr int kFirst = 0;
    ASSERT_TRUE(nodeLiuxia(kFirst));
    ASSERT_TRUE(nodeHuodan(kFirst));

    // ---- 节点 3 之前：同一座炉子应声的是剧情，不是面板 ----
    ASSERT_TRUE(travelTo(kMapLuorifeng, kFirst));
    const MapObject furnace = objectNamed("facility_danlu");
    ASSERT_FALSE(furnace.name.empty());
    ASSERT_EQ(furnace.property("kind"), std::string("alchemy"));
    ASSERT_TRUE(pressAt(furnace, kFirst));
    ASSERT_NE(app_.topScene(), nullptr);
    EXPECT_EQ(app_.topScene()->name(), "Dialogue")
        << "节点 3 还没演，这一下该起剧情，不该直接开面板";
    ASSERT_TRUE(app_.scripts().isRunning());
    pumpScripts(kFirst);
    ASSERT_EQ(flag("ch04.kailu"), 1) << "这一下就是节点 3";

    // ---- 节点 3 之后：once 烧掉，触发器让位，面板接手 ----
    AlchemyScene* panel = openFurnace();
    ASSERT_NE(panel, nullptr) << "触发器烧掉了，这一下该开炼丹面板（kailu.lua 首部的接线）";
    EXPECT_EQ(panel->kind(), CraftKind::Alchemy);
    EXPECT_TRUE(fanren::rules::hasToolOfGrade(panel->toolGrade()))
        << "地图上那座炉子写着 grade=1，面板不该认为身边没有炉鼎";

    const int pillRow = recipeRow(CraftKind::Alchemy, kRecipePill);
    const int easyRow = recipeRow(CraftKind::Alchemy, kRecipeEasy);
    ASSERT_GE(pillRow, 0) << "炼丹面板上没有养精丹方";
    ASSERT_GE(easyRow, 0) << "炼丹面板上没有清灵散方";

    const auto rowsNow = [&] {
        return AlchemyScene::buildRecipeItems(app_.data(), state(), CraftKind::Alchemy,
                                              panel->toolGrade(),
                                              AlchemyScene::visibleRecipes(app_.recipes(),
                                                                           CraftKind::Alchemy));
    };

    // ---- 开不了工要说清楚缺什么：这一刻缺的是火候，不是料 ----
    ASSERT_EQ(state().alchemyProficiency, 0) << "先验：他一炉也没炼过";
    ASSERT_GE(state().itemCount(kHuangjing), 2) << "先验：俸禄发下来的黄精草够用";
    ASSERT_GE(state().itemCount(kZishen), 1) << "先验：俸禄发下来的紫参草够用";
    {
        const fanren::ui::ListItem row = rowsNow()[static_cast<std::size_t>(pillRow)];
        EXPECT_FALSE(row.enabled);
        ASSERT_FALSE(row.disabledReason.empty()) << "点不动却一个字不说，玩家一律当成 bug";
        EXPECT_NE(row.disabledReason.find("火候未到"), std::string::npos)
            << "料是够的，缺的是火候，别赖到材料头上：" << row.disabledReason;
    }
    // 真的点下去也一动不动：「没开工」与「开工失败」是两回事，只有后者该长经验。
    {
        const int profBefore = state().alchemyProficiency;
        const int herbBefore = state().itemCount(kHuangjing);
        EXPECT_FALSE(panel->craftAt(app_, pillRow)) << "开不了工就不能报成开工了";
        EXPECT_EQ(state().alchemyProficiency, profBefore);
        EXPECT_EQ(state().itemCount(kHuangjing), herbBefore);
        ASSERT_FALSE(panel->feedback().empty()) << "静默失败是明令禁止的";
    }

    // ---- 峰上那一年的俸禄到手（节点 4），再回来开炉 ----
    // 料只用本章真发下来的（复验判据 2）：头一个月那一份只够三炉，垫火候都未必够，
    // 所以先把节点 4 走了，让那一年按月抬上来的药材进背包。本文件不往背包里塞药材。
    panel->leave(app_);
    app_.tick(kFrame);
    ASSERT_TRUE(nodeYufeng(kFirst));
    panel = openFurnace();
    ASSERT_NE(panel, nullptr) << "节点 4 回来，那座炉子该还开得了面板";

    // ---- 垫清灵散（熟练度门槛 0），火候就够了 ----
    int furnaces = 0;
    while (state().alchemyProficiency < 6 && furnaces < 20) {
        ++furnaces;
        ASSERT_TRUE(panel->craftAt(app_, easyRow)) << panel->feedback();
    }
    ASSERT_GE(state().alchemyProficiency, 6) << "垫了二十炉还没够养精丹那道门槛";
    {
        const fanren::ui::ListItem row = rowsNow()[static_cast<std::size_t>(pillRow)];
        EXPECT_TRUE(row.enabled) << "火候够了、料也够了，这一行就该亮：" << row.disabledReason;
    }

    // ---- 能成也能炸，两条都要走得到 ----
    // 整条链路是确定的（种子由配方 id、日期、熟练度、第几炉派生），所以这不是
    // 碰运气：同样的起点每次跑出同样的一串结果。
    int successes = 0;
    int failures = 0;
    for (int attempt = 0; attempt < 80 && (successes == 0 || failures == 0); ++attempt) {
        const int huangjingBefore = state().itemCount(kHuangjing);
        const int zishenBefore = state().itemCount(kZishen);
        const int pillsBefore = state().itemCount(kPill);
        const int profBefore = state().alchemyProficiency;

        ASSERT_TRUE(panel->craftAt(app_, pillRow)) << panel->feedback();
        ASSERT_FALSE(panel->feedback().empty()) << "成也好败也好，都要有一句话";

        // 炼丹**失败材料尽毁**（rules::failurePolicyOf 的性格差异），
        // 所以两条路上材料都少同样多——差别只在有没有产物。
        EXPECT_EQ(state().itemCount(kHuangjing), huangjingBefore - 2);
        EXPECT_EQ(state().itemCount(kZishen), zishenBefore - 1);
        if (state().itemCount(kPill) > pillsBefore) {
            ++successes;
            EXPECT_EQ(state().itemCount(kPill), pillsBefore + 1);
        } else {
            ++failures;
        }
        // 失败也长手艺，只是长得慢。
        EXPECT_GT(state().alchemyProficiency, profBefore) << "开了一炉，熟练度一点没涨";
    }
    EXPECT_GT(successes, 0) << "八十炉一次也没成，成功那条路是死的";
    EXPECT_GT(failures, 0) << "八十炉一次也没炸，失败那条路是死的";

    // ---- 料没了，说的就该是料 ----
    // 与上面那条「火候未到」成对：只写一边的话，把整段判定删掉也能全绿。
    while (state().removeItem(kHuangjing, 1)) {
    }
    ASSERT_EQ(state().itemCount(kHuangjing), 0);
    {
        const fanren::ui::ListItem row = rowsNow()[static_cast<std::size_t>(pillRow)];
        EXPECT_FALSE(row.enabled);
        ASSERT_FALSE(row.disabledReason.empty());
        EXPECT_NE(row.disabledReason.find("材料不足"), std::string::npos) << row.disabledReason;
        // 缺什么要报**物品名**而不是 id：规则层不认识物品册，由面板换名。
        const fanren::core::Item* herb = app_.data().findItem(kHuangjing);
        ASSERT_NE(herb, nullptr);
        EXPECT_NE(row.disabledReason.find(herb->name), std::string::npos) << row.disabledReason;
        EXPECT_EQ(row.disabledReason.find(kHuangjing), std::string::npos)
            << "理由里漏出了物品 id：" << row.disabledReason;
    }

    panel->leave(app_);
    app_.tick(kFrame);
    std::cout << "\n[ch04 炼丹实测] 垫 " << furnaces << " 炉清灵散够到火候 6，"
              << "养精丹成 " << successes << " 炉、炸 " << failures << " 炉，"
              << "熟练度 " << state().alchemyProficiency << std::endl;
}

// ---------------------------------------------------------------------------
// 判据自检：证明留药那条账目判据真的有牙
// ---------------------------------------------------------------------------
// 「说留了三瓶就真的少三瓶」在背包整个是空的时候有一半是恒真的。凡这种形状的
// 断言都要另配一条用例，先证明判据本身咬得动人，再拿它去验真实的运行结果。
// 这个项目最惨的一次事故正是校验器的正则被 heredoc 吃掉转义、从此永远报通过
// 而无人发现——因为没人做过负向验证。
EndBag faithfulEndingWithPills() {
    EndBag e;
    e.pillsBefore = 4;
    e.pillsAfter = 1;
    e.saidHave = true;
    e.packedFull = true;
    e.plaque = 1;
    e.talisman = 1;
    e.knowsFire = true;
    e.knowsWind = true;
    return e;
}

EndBag faithfulEndingWithoutPills() {
    EndBag e;
    e.pillsBefore = 0;
    e.pillsAfter = 0;
    e.saidNone = true;
    e.packedBare = true;
    e.plaque = 1;
    e.talisman = 1;
    e.knowsFire = true;
    e.knowsWind = true;
    return e;
}

TEST(Ch04EndState, BothSidesOfTheLetterAreAcceptedBecauseBothAreLegalEndings) {
    // 两侧都合法（xinbie.lua 首部原话）：炼成了就留药，没炼成就留方子。
    // 判据管的是「说的和做的一致」，不是「他手上必须有药」——
    // 把后者写成断言就是把内容形状当成了规格。
    EXPECT_TRUE(chapterEndBag(faithfulEndingWithPills()).ok);
    EXPECT_TRUE(chapterEndBag(faithfulEndingWithoutPills()).ok);
}

TEST(Ch04EndState, TheVerdictCatchesPillsThatWereNeverHandedOver) {
    // 脚本里少写一行 take()：旗标照置、台词照说，而三只瓶子原封不动留在背包里。
    // 这正是第 2 章认定的要害类别——游戏说了句像真的假话。
    EndBag broken = faithfulEndingWithPills();
    broken.pillsAfter = broken.pillsBefore;
    const BagVerdict caught = chapterEndBag(broken);
    EXPECT_FALSE(caught.ok) << "说留了三瓶、一瓶没少，判据却说没事——它没有牙";
    EXPECT_NE(caught.why.find("一瓶没少"), std::string::npos) << caught.why;
}

TEST(Ch04EndState, TheVerdictCatchesPillsBeingTakenOnTheBranchThatSaysThereAreNone) {
    // 另一头：台词说翻遍箱底也没有，背包却少了三瓶。take() 失败时不许动背包。
    EndBag broken = faithfulEndingWithoutPills();
    broken.pillsBefore = 3;
    broken.pillsAfter = 0;
    EXPECT_FALSE(chapterEndBag(broken).ok) << "扣得到却报了扣不到，判据却说没事";
}

TEST(Ch04EndState, TheVerdictRefusesAnEmptyRunInsteadOfWavingItThrough) {
    // 先验判据。一份什么也没发生的证词能同时满足「一瓶也没少」与「说的是没有」
    // ——两条都在空局面上恒真。判据必须先咬住「这一章确实走过」。
    const EndBag nothing;
    const BagVerdict verdict = chapterEndBag(nothing);
    EXPECT_FALSE(verdict.ok) << "什么也没发生也算通关，那这组断言等于没写";
    EXPECT_NE(verdict.why.find("牌子"), std::string::npos)
        << "空局面该栽在「该在的东西不在」这一关：" << verdict.why;
}

TEST(Ch04EndState, TheVerdictCatchesEitherSpoilItemGoingMissing) {
    // 节点 10 那两件是真的发生过的物证。只看旗标的话，give() 漏写一行
    // 完全看不出来。
    EndBag noPlaque = faithfulEndingWithPills();
    noPlaque.plaque = 0;
    EXPECT_FALSE(chapterEndBag(noPlaque).ok) << "那块牌子没了也算通关？";

    EndBag noTalisman = faithfulEndingWithPills();
    noTalisman.talisman = 0;
    EXPECT_FALSE(chapterEndBag(noTalisman).ok) << "剑符没了也算通关？";
}

TEST(Ch04EndState, TheVerdictCatchesEitherMagicGoingMissing) {
    // 两门法术是节点 2、4 的物证，也是「已习得法术真的落盘」的最后一道账。
    EndBag noFire = faithfulEndingWithPills();
    noFire.knowsFire = false;
    EXPECT_FALSE(chapterEndBag(noFire).ok);

    EndBag noWind = faithfulEndingWithPills();
    noWind.knowsWind = false;
    EXPECT_FALSE(chapterEndBag(noWind).ok);
}

TEST(Ch04EndState, TheVerdictCatchesTheTwoBranchesBeingPlayedTogetherOrNeither) {
    // 「有药」与「没药」两条台词只能演一条。两条都演了 = 脚本把 if/else 写成了
    // 两段顺序执行；一条也没演 = take() 的返回值压根没接上。
    EndBag both = faithfulEndingWithPills();
    both.saidNone = true;
    EXPECT_FALSE(chapterEndBag(both).ok);

    EndBag neither = faithfulEndingWithPills();
    neither.saidHave = false;
    neither.packedFull = false;
    EXPECT_FALSE(chapterEndBag(neither).ok);
}

TEST(Ch04EndState, TheVerdictCatchesThePackingLineContradictingTheOneBeforeIt) {
    // 同一个返回值在一条脚本里分了两次岔（有没有药、包什么），两次答案却不一样。
    EndBag mismatched = faithfulEndingWithPills();
    mismatched.packedFull = false;
    mismatched.packedBare = true;
    EXPECT_FALSE(chapterEndBag(mismatched).ok);
}

}  // namespace
