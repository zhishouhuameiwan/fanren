// 第 5 章的自动化通关测试（docs/ch05-design.md 第 15 节验收第 2、10、13、14、15、17 条），
// 外加「意图玩家」的扫描（施工图 8.1 / 8.5）、寒毒倒计时的插桩用例、支线的接取与过期、
// 十场必打战斗「绕不绕得过去」的走位用例，以及交给第 6 章的终局存档。
//
// 驱动的是玩家真会碰的那一整套：真的 Application（无头）、真的 maps/ch05_*.tmj 与韩家村、
// 真的 WorldScene 走位规则、真的 scripts/ch05/*.lua、真的 BattleScene、ShopScene、告示板。
//
// ---------------------------------------------------------------------------
// 判据从哪来（写每一条断言之前对照过 docs/README.md「判据自己会说谎」那张表）
// ---------------------------------------------------------------------------
// **判据只从用户批准的施工图推，不从被测的脚本和数据里抄。** 下面每一个 kXxx 常量
// 旁边都写着它抄自施工图哪一节。脚本里的 key 名只拿来**认出**「演的是哪一条」
//（与第 4 章同一个办法），不拿来决定「该演哪一条」。
// 施工图第 18 节的施工偏差，本文件认了其中两条会改变判据的：
//   · 第 6 条：回城那十天从 12e 挪到 12d 得手之后（总天数不变，12e 看到的 d 不变）；
//   · 第 7 条：寒毒检查点在脚本开头、拨日子之前（施工图 3.3「检查点：…的脚本开头」照字面）。
// 两条都合理，判据照它们改写，旁边各有一行说明。
//
// ---------------------------------------------------------------------------
// 起点不许自己挑（第 4 章复验判据 1、handoff 第 8 节）
// ---------------------------------------------------------------------------
// 起点**直接读** tests/fixtures/ch04-end-first.sav / ch04-end-second.sav，由第 4 章两条
// 通关用例写出、逐字段看着。本文件在起点上**一个字段也不改**：读进来、载入存档里那张图、
// 把人放回存档里那一格（loadMap 会把人挪到出生点，放回去不是改字段，是撤销这一挪）。
// 与施工图第 2 节「硬」不符的两处照 fixture 办（施工偏差 18.4 已登记）：
// 曲魂 31/55 不是满血（六十天的路静养回满）；第二侧 0 瓶养精丹。
//
// 两处**测试里的手摆**，都是扫描与插桩用的量尺，都不在通关用例里：
//   · Ch05HandSweep 的「P0−2 / P0＋2」一列与「最少要几瓶」表改身上的养精丹数
//    （施工图 8.5 自己就把药数列为一维）；
//   · Ch05ColdPoison 把起表那一天往前拨，让 d 落在 66 / 80 / 90 上（施工图验收 10 原话
//    「插桩用例：拨到 d = 66」）。
// 「只练剑符」「都不备」两种备法要身上没有清灵散，而第 4 章终局两侧各有 2 份：
// 这里**真的走进南城药铺卖掉**（ShopScene::sellAt），不是手删——施工偏差 18.4 第 3 行写明了
// 「要走到条件战得先把清灵散花掉或卖掉」。
//
// ---------------------------------------------------------------------------
// 替玩家出手的那只手：意图玩家（施工图 8.1）
// ---------------------------------------------------------------------------
//   · 血掉到一半（≤ 60 / 120）才吃药；**不算**对面下一轮打不打得死自己；
//   · 按物品 id 先吃养精丹，不碰金疮药；不把药喂给同伴；
//   · 背包从第 4 章终局 fixture 原样接过来，不补料。
// 施工图没写「什么时候逃」，而 8.4 / 8.5 又要求 ⑧⑩ 与潇湘院「输，可逃」、「不许 game_over」。
// 这里补的是**最不精明**的那一种读法：**半血以下、身上没有养精丹、这一场能逃，就逃**。
// 它不看对面还剩几滴血，也不看这一轮逃不逃得掉（逃跑成功率由身法算，BattleState::escapeChance）。
// 出手交给第 3、4、5 章共用的那只手（tests/BattleHand.h）：蓄劲、挑破绽、破势时蓄满打、
// 首领蓄势时抢着破势或防御；「手上没有一样东西还伤得了对面」时也走（这一条本章如今用不上：
// ⑩ 没练剑符的人到了亭子不照面、不开战，退回林子练符——复验 MEDIUM-B 方案 1，施工图 8.4）。
// 意图玩家与第 4 章那只手的差别只在 HandPolicy 的三项：门槛五成、不算下一轮、没药能逃就逃。
//
// 扫描（Ch05HandSweep）另换四只手：门槛 40 / 60、先吃金疮药、喂同伴（复验二第八节第 1 条）。
// **哪几维按构造翻不了**写在那一组用例的注释里。
//
// ---------------------------------------------------------------------------
// 一条走位规则，与第 3、4 章同源（WorldScene::interact 看面朝的前一格）
// ---------------------------------------------------------------------------
// 要站在 S 上脸朝着相邻的 F：F 进不去时站到 S 朝 F 按一下方向键；F 进得去时从 F 的另一侧
// 那一格朝 F 迈一步落到 S。faceCell 两条都实现了。
// 路上不许顺手把剧情演了：standable() 避开此刻还备着的踏入型触发（WorldScene::triggerReady）。
//
// ---------------------------------------------------------------------------
// 这个文件看不见什么
// ---------------------------------------------------------------------------
//   · 画面（对白框、告示板的样子）：只读 BoardScene::buildRows 与 spokenKeys；
//   · 手按方向键的节奏、长按快进；
//   · 战斗的 RNG：BattleScene 的种子由编成 id 派生，所以每一场的逃跑成败都是**确定的**——
//     「逃得掉」只是这一个种子上的一次抽样，不是概率上的保证（扫描表的注释里有这一条的后果）。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/battle/Damage.h"
#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/BoardScene.h"
#include "game/Scene.h"
#include "game/ShopScene.h"
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
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::BoardScene;
using fanren::game::ShopScene;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// ---- 本章踏过的七张图（施工图第 4 节：新建 6 张 ＋ 韩家村东口修补）----
constexpr const char* kMapHanjiacun = "ch01_hanjiacun";
constexpr const char* kMapDukou = "ch05_dukou";
constexpr const char* kMapXicheng = "ch05_xicheng";
constexpr const char* kMapNancheng = "ch05_nancheng";
constexpr const char* kMapKezhan = "ch05_kezhan";
constexpr const char* kMapMofu = "ch05_mofu";
constexpr const char* kMapShanzhuang = "ch05_dubashanzhuang";

// ---- 本章动到的东西（施工图第 2 节、E6、第 9 节）----
constexpr const char* kHero = "hanli";
constexpr const char* kCompanion = "qu_hun";
constexpr const char* kPill = "pill_yangjing_dan";
constexpr const char* kSalve = "pill_jinchuang_yao";
constexpr const char* kQingling = "pill_qingling_san";
constexpr const char* kTalisman = "talisman_jianfu";
constexpr const char* kPlaque = "material_heipai";
constexpr const char* kYishu = "story_mo_yishu";
constexpr const char* kQinbixin = "story_mo_qinbixin";
constexpr const char* kWenlong = "story_wenlong_jie";
constexpr const char* kBaoyu = "story_nuanyang_baoyu";
constexpr const char* kChaoben = "story_yigao_chaoben";
constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZishen = "herb_zishen_cao";
constexpr const char* kFireMagic = "magic_huodan_shu";
constexpr const char* kWindMagic = "magic_yufeng_jue";
constexpr const char* kJianfuMagic = "magic_ji_jianfu";

// ---- 施工图 3.3 日历（判据抄自那张表，不从脚本读）----
constexpr int kDaysRoad = 60;            // 1a：前两个月的路
constexpr int kDaysRiver = 29;           // 1b：水路
constexpr int kDaysQingbao = 1 + 1;      // 4a：「1，末尾再 1」
constexpr int kDaysJieren = 2;           // 4b：「一番打听」
constexpr int kDaysJianmian = 1;         // 7b：宿墨府
constexpr int kDaysDuobang = 1;          // 10c：夺帮一夜（输一回再 ＋1）
constexpr int kDaysChuzheng = 1 + 10;    // 12b：出发 1 ＋ 骑马 10
constexpr int kDaysTancha = 3;           // 12c：刺探
constexpr int kDaysTanchaAlert = 6;      // 12c：被盯上（8 报了师门）
// 12e 回城 10 天——施工偏差 18.3 第 6 条挪进 12d 得手之后、teleport 之前（总天数不变）。
constexpr int kDaysHomeward = 10;
constexpr int kDaysChaoxie = 1;          // Z1 誊抄
constexpr int kDaysLianfu = 2;           // Z2 练符
constexpr int kDaysXiaoxiangRetreat = 1; // 10b 逃 ＋1/次
// 表末那一格：12e 回城时发作起第几日，主线最快 59，被盯上 62。
constexpr int kDayAtTheEndFastest = 59;
constexpr int kDayAtTheEndAlert = 62;

// ---- 施工图 3.3 寒毒三段 ----
constexpr int kHanduStageTwo = 65;
constexpr int kHanduStageThree = 80;
constexpr int kHanduDeadline = 90;

// ---- 施工图 8.0 / 8.2：十场必打（按发生顺序）与一场条件战 ----
constexpr const char* kMustFight[] = {
    "b05_yesu_yelang",       // ① 1a
    "b05_heishuixiang",      // ② 3a
    "b05_matou_zhuishao",    // ③ 3b
    "b05_tiequanhui_jieren", // ④ 4b
    "b05_duobang",           // ⑤ 10c
    "b05_yange_qiecuo",      // ⑥ 11b
    "b05_mofu_shigui",       // ⑦ 12a
    "b05_wu_jianming",       // ⑧ 12b
    "b05_xunzhuang",         // ⑨ 12c
    "b05_ouyang_feitian",    // ⑩ 12d
};
constexpr const char* kConditionalFight = "b05_xiaoxiangyuan";
constexpr const char* kMustFightMark[] = {"①", "②", "③", "④", "⑤", "⑥", "⑦", "⑧", "⑨", "⑩"};

// 施工图 8.4「身上 0 瓶养精丹时」一列写「赢」的那几场：①②③④⑥⑦⑨（「无必败分支」）。
// ⑤ 不动背包（不适用）；⑧⑩ 写的是「输，可逃」。
bool winsWithNoPillPerDesign(const std::string& id) {
    for (const char* ok : {"b05_yesu_yelang", "b05_heishuixiang", "b05_matou_zhuishao",
                           "b05_tiequanhui_jieren", "b05_yange_qiecuo", "b05_mofu_shigui",
                           "b05_xunzhuang"}) {
        if (id == ok) return true;
    }
    return false;
}

// ---- 施工图 3.4 与 8.4 的 R-3 提示（「凡不做 X 就必败的分支，提示至少两处，且都在走进去之前」）----
// 提示一：目标链那一步（3.4 表第 16 / 21 / 23 步，验收 14 的字面量「清灵散」「药」「剑符」）。
// 提示二：8.4 表里那一句——两句施工图给了原话（10a、11c），两句是转述（11a「刀剑难伤」、
// 12c「看见刀从他胳膊上弹开」）。转述的那两句按施工图里的**词**认：施工图第 13 节要求台词
// 自己写，所以不许要求逐字相同，只要求那几个词都在。
constexpr const char* kHintStepXiaoxiang = "n16_xiaoxiang";
constexpr const char* kHintStepMajiu = "n21_majiu";
constexpr const char* kHintStepTancha = "n23_tancha";
constexpr const char* kHintStepCisha = "n24_cisha";     // 复验 MEDIUM-B：没练成剑符也看得见的那一句
constexpr const char* kHintDingjiQuote = "要毒人，自己先得有一颗";           // 8.4：10a 他自己说
constexpr const char* kHintAnpaiQuote = "公子此去山高路远，药可带足了？";   // 8.4：11c 孙二狗

// ---- 意图玩家（施工图 8.1）----
constexpr int kIntentHealPercent = 50;   // 「血掉到一半（≤ 60 / 120）才吃药」

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr double kFrame = 1.0 / 60.0;
constexpr int kMaxScriptFrames = 8000;
// 同一个节点最多重来几次（①逃、⑤输、潇湘院逃、⑩没练符不照面）。到了还过不去就停下来报出来。
constexpr int kMaxAttempts = 4;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch05" / "huanyu.lua") &&
            fs::exists(root / "maps" / "ch05_mofu.tmj")) {
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
// 七张图之间的门（不写任何 require_flag：闸开没开由引擎说了算）
// ---------------------------------------------------------------------------
// 渡口 → 西城、南城 → 墨府后园（夜）、墨府 → 山庄、山庄 → 墨府前院都是脚本 teleport，
// 不在这张表里（施工图第 4 节「连通」）。
struct MapLink {
    const char* from;
    const char* to;
    const char* portal;
};

constexpr MapLink kMapLinks[] = {
    {kMapHanjiacun, kMapDukou, "portal_to_dukou"},
    {kMapDukou, kMapHanjiacun, "portal_to_hanjiacun"},
    {kMapXicheng, kMapNancheng, "portal_to_nancheng"},
    {kMapNancheng, kMapXicheng, "portal_to_xicheng"},
    {kMapNancheng, kMapMofu, "portal_to_mofu"},
    {kMapMofu, kMapNancheng, "portal_to_nancheng"},
    {kMapNancheng, kMapKezhan, "portal_to_kezhan"},
    {kMapKezhan, kMapNancheng, "portal_to_nancheng"},
};

// ---------------------------------------------------------------------------
// 一章的步骤（施工图 3.1 / 3.2 的挂点 ＋ 支线三处 ＋ 卖清灵散）
// ---------------------------------------------------------------------------
enum class Step {
    DongquLu, Shangchuan, Matou, Heishui, Zhuishao, SellQingling, Qingbao, Jieren, Jiulou,
    Yeru, Toutin, Dengmen, Jianmian, Huayuan, Duizhi, Dingji, Xiaoxiang, Shuijiao,
    Jiaoyi, Yange, Chaoxie, LianJianfu, Anpai, FengwuDeliver, Majiu, Zhuwu, Tancha,
    LianJianfuLin, Shangyue, Huanyu,
};

struct StepInfo {
    Step step;
    const char* name;
    int node;              // 施工图 3.1 表 # 列的节点号（1a → 1 … 12e → 12）；支线按挂起那一节
    const char* doneFlag;  // 这一步做完的记号（施工图 3.1 的 set_flag 列 / 第 6 节）
};

const std::vector<StepInfo>& stepTable() {
    static const std::vector<StepInfo> kTable = {
        {Step::DongquLu, "1a 东去路上", 1, "ch05.kaipian"},
        {Step::Shangchuan, "1b 上船", 1, "ch05.shangchuan"},
        {Step::Matou, "2 码头", 2, "ch05.matou"},
        {Step::Heishui, "3a 黑水巷", 3, "ch05.shoufu"},
        {Step::Zhuishao, "3b 码头盯梢", 3, "ch05.zhuishao"},
        {Step::SellQingling, "卖清灵散", 3, ""},
        {Step::Qingbao, "4a 情报", 4, "ch05.qingbao"},
        {Step::Jieren, "4b 截人", 4, "ch05.jieren"},
        {Step::Jiulou, "5 酒楼", 5, "ch05.jiulou"},
        {Step::Yeru, "6a 夜入", 6, "ch05.yeru"},
        {Step::Toutin, "6b 偷听", 6, "ch05.toutin"},
        {Step::Dengmen, "7a 登门", 7, "ch05.dengmen"},
        {Step::Jianmian, "7b 见面礼", 7, "ch05.jianmianli"},
        {Step::Huayuan, "8 花园", 8, "ch05.huayuan"},
        {Step::Duizhi, "9 对质", 9, "ch05.duizhi"},
        {Step::Dingji, "10a 定计", 10, "ch05.dingji"},
        {Step::Xiaoxiang, "10b 潇湘院", 10, "ch05.xiaoxiang"},
        {Step::Shuijiao, "10c 夺帮", 10, "ch05.duobang"},
        {Step::Jiaoyi, "11a 交易", 11, "ch05.jiaoyi"},
        {Step::Yange, "11b 燕歌", 11, "ch05.yange"},
        {Step::Chaoxie, "Z1 誊抄", 11, "ch05.fengwu_chao"},
        {Step::LianJianfu, "Z2 练符", 11, "ch05.lian_jianfu"},
        {Step::Anpai, "11c 安排", 11, "ch05.anpai"},
        {Step::FengwuDeliver, "Z1 交付", 11, "ch05.fengwu_yigao"},
        {Step::Majiu, "12a 尸傀", 12, "ch05.shigui"},
        {Step::Zhuwu, "12b 吴剑鸣", 12, "ch05.chuzheng"},
        {Step::Tancha, "12c 巡庄", 12, "ch05.tancha"},
        {Step::LianJianfuLin, "Z2′ 林中练符", 12, "ch05.lian_jianfu"},
        {Step::Shangyue, "12d 欧阳飞天", 12, "ch05.cisha"},
        {Step::Huanyu, "12e 换玉", 12, "ch05.done"},
    };
    return kTable;
}

const StepInfo& infoOf(Step step) {
    for (const StepInfo& info : stepTable()) {
        if (info.step == step) return info;
    }
    return stepTable().front();
}

// 一条路线：每一处主动选择取第几项（0 起算，Choice 命令的 choiceIndex）＋ 备法 ＋ 支线做不做。
struct Route {
    std::string name;
    std::map<Step, int> choices;   // 没写的一律取第 0 项
    int fallback = 0;              // 没写进 choices 的步骤取这一项
    bool keepQingling = true;      // 身上的清灵散留着（否则去南城药铺卖掉）
    bool trainJianfu = true;       // 出发去山庄之前在客栈后院练剑符（Z2）
    bool fengwuBook = false;       // 支线 Z1 两步都做
    [[nodiscard]] int choiceFor(Step step) const {
        const auto it = choices.find(step);
        return it == choices.end() ? fallback : it->second;
    }
};

// 施工图 8.5「路：全备 / 只备清灵散 / 只练剑符 / 都不备」
enum class Prep { Full, QinglingOnly, JianfuOnly, Nothing };

const char* prepName(Prep prep) {
    switch (prep) {
        case Prep::Full: return "全备";
        case Prep::QinglingOnly: return "只备清灵散";
        case Prep::JianfuOnly: return "只练剑符";
        case Prep::Nothing: return "都不备";
    }
    return "?";
}

Route routeFor(int side, Prep prep) {
    Route r;
    r.name = std::string(side == 0 ? "第一侧" : "第二侧") + "·" + prepName(prep);
    r.fallback = side == 0 ? 0 : 1;
    r.keepQingling = prep == Prep::Full || prep == Prep::QinglingOnly;
    r.trainJianfu = prep == Prep::Full || prep == Prep::JianfuOnly;
    return r;
}

// 一场仗的记录。
struct BattleRecord {
    std::string id;
    Step step{};
    BattlePhase phase = BattlePhase::Ongoing;
    int hpBefore = 0, hpAfter = 0, maxHp = 0;
    int mpBefore = 0, mpAfter = 0;
    int pillsBefore = 0, pillsAfter = 0;
    int salvesBefore = 0, salvesAfter = 0;
    bool heroOnField = false;
    std::vector<std::string> allyIds;
};

const char* phaseName(BattlePhase phase) {
    switch (phase) {
        case BattlePhase::Won: return "胜";
        case BattlePhase::Lost: return "负";
        case BattlePhase::Escaped: return "逃";
        case BattlePhase::EnemyFled: return "敌逃";
        case BattlePhase::Ongoing: return "未完";
    }
    return "?";
}

// 一步的记录。
struct StepRecord {
    Step step{};
    int dayBefore = 0, dayAfter = 0;
    int pillsBefore = 0, pillsAfter = 0;
    std::vector<std::string> keys;       // 这一步里说过的文案 key
    std::vector<BattlePhase> phases;     // 这一步里打的仗各是什么落点
};

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch05Walkthrough : public ::testing::Test {
protected:
    struct HandPolicy {
        int healAtPercent = kIntentHealPercent;
        bool salveFirst = false;      // 先吃金疮药（扫描用）
        bool feedCompanion = false;   // 喂同伴（扫描用）
    };

    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        startFromChapterFourEnding(fanren::test::kChapterFourEndingFirst);
    }

    void TearDown() override { app_.shutdown(); }

    // 第 4 章交到本章手里的那份存档，**原样读进来**（施工图第 2 节、验收 2）。
    // 与存档不同的字段：**一个也没有**。
    void startFromChapterFourEnding(const char* fileName) {
        const fs::path fixture = fanren::test::chapterFixturePath(assetRoot(), fileName);
        auto handedOver = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(handedOver.ok) << "第 4 章的交接存档读不进来（" << fixture.string()
                                   << "）：" << handedOver.error;
        const GameState& s = handedOver.value;
        // 验收 2 的 SetUp 先验，逐条抄自施工图第 15 节第 2 行。
        ASSERT_EQ(s.flag("ch04.done"), 1) << "交接存档不是第 4 章的终局";
        ASSERT_EQ(s.realm, Realm::QiRefining8) << "施工图第 2 节：炼气八层（硬）";
        ASSERT_EQ(s.realmCap, Realm::QiRefining8) << "施工图第 2 节：上限八层（硬）";
        ASSERT_EQ(s.party.size(), 1u) << "施工图第 2 节：party == [qu_hun]";
        ASSERT_EQ(s.party[0].roleId, kCompanion);
        const std::vector<std::string> twoMagics{kFireMagic, kWindMagic};
        ASSERT_EQ(s.learnedMagics, twoMagics) << "施工图第 2 节：火弹术、御风决两门（硬）";
        ASSERT_EQ(s.itemCount(kTalisman), 1) << "施工图第 2 节：剑符 1（硬）";
        ASSERT_EQ(s.itemCount(kPlaque), 1) << "施工图第 2 节：那块牌子 1（硬）";
        ASSERT_EQ(s.flag("story.xiuxian_known"), 0) << "施工图第 2 节：本章不置（硬）";
        ASSERT_EQ(s.mapId, kMapHanjiacun) << "施工图第 2 节：韩家村（硬）";

        const Point where = s.position;
        const int facing = s.facing;
        state() = s;
        auto loaded = app_.loadMap(s.mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        // loadMap 把人挪到了出生点；放回存档里那一格——这是撤销那一挪，不是改字段。
        state().position = where;
        state().facing = facing;
        startDay_ = state().day;
        startPills_ = state().itemCount(kPill);
        startSnapshot_ = state();
        battles_.clear();
        steps_.clear();
        chapterLog_.clear();
    }

    void settleEndingAgainstFixture(const char* fileName) {
        const fanren::test::FixtureVerdict verdict = fanren::test::settleAgainstFixture(
            state(), assetRoot(), fileName, fanren::test::kWriteChapterFiveFixturesEnv, "第 6 章");
        if (verdict.wrote) {
            std::cout << "[ch05 交接存档] 已写出 "
                      << fanren::test::chapterFixturePath(assetRoot(), fileName).string() << std::endl;
        }
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
    }

    GameState& state() { return app_.state(); }
    int flag(const std::string& name) { return state().flag(name); }
    int dayCount() { return state().day - flag("ch05.yindu_qi"); }

    // 不许报错的时候（「绕得过去吗」那一组用例专门去撞闸门）把本文件自己的 ADD_FAILURE 关掉。
    void complain(const std::string& why) {
        if (!tolerateBlocked_) ADD_FAILURE() << why;
    }

    // -----------------------------------------------------------------------
    // 文案记录：整章按顺序记下每一句（spokenKeys 每节清一次，上限 128，不够用）
    // -----------------------------------------------------------------------
    void clearKeys() {
        app_.clearSpokenKeys();
        keysSeen_ = 0;
    }
    void syncKeys() {
        const std::vector<std::string>& keys = app_.spokenKeys();
        if (keys.size() < keysSeen_) keysSeen_ = 0;
        for (std::size_t i = keysSeen_; i < keys.size(); ++i) {
            chapterLog_.emplace_back(currentStep_, keys[i]);
            if (!steps_.empty()) steps_.back().keys.push_back(keys[i]);
        }
        keysSeen_ = keys.size();
    }
    bool spokeIn(Step step, const std::string& key) const {
        for (const StepRecord& record : steps_) {
            if (record.step != step) continue;
            if (std::find(record.keys.begin(), record.keys.end(), key) != record.keys.end()) {
                return true;
            }
        }
        return false;
    }
    bool spokeAnywhere(const std::string& key) const {
        for (const auto& entry : chapterLog_) {
            if (entry.second == key) return true;
        }
        return false;
    }
    std::vector<std::string> textsIn(Step step) {
        std::vector<std::string> out;
        for (const StepRecord& record : steps_) {
            if (record.step != step) continue;
            for (const std::string& key : record.keys) out.push_back(app_.data().lookupText(key));
        }
        return out;
    }
    bool stepDone(Step step) const {
        for (const StepRecord& record : steps_) {
            if (record.step == step) return true;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // 战斗：意图玩家（文件头第三段），手是 tests/BattleHand.h 那一只
    // -----------------------------------------------------------------------
    // 本文件的取舍（HandPolicy，扫描换它）翻成那只手的取舍。
    static fanren::test::HandPolicy intentPolicy(const HandPolicy& h) {
        fanren::test::HandPolicy p;
        p.healAtPercent = h.healAtPercent;
        p.healWhenDoomed = false;   // 施工图 8.1：不算对面下一轮打不打得死自己
        p.fleeWhenSpent = true;     // 半血以下、没药可吃、这一场能逃就逃
        p.feedCompanion = h.feedCompanion;
        if (h.salveFirst) p.pills.insert(p.pills.begin(), kSalve);
        return p;
    }

    BattlePhase playBattle(BattleScene& scene) {
        fanren::test::BattleHand hand(app_, intentPolicy(hand_));
        hand.fleeAtOnce(fleeingThisFight_);
        return hand.play(scene);
    }

    // 这一场是哪一张编成。BattleScene 不公开编成 id，只好按「敌方每个人的角色与波次」
    // 对 data 里本章的十一张认——十一张两两不同，认得出来；认不出来记成「?」，用例会红。
    //（从前还比宽高；横版没有格子，那一半删了，只凭人也认得出来。）
    std::string identifyBattle(const BattleScene& scene) {
        std::vector<std::pair<std::string, int>> have;
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally) have.emplace_back(u.id, u.wave);
        }
        std::sort(have.begin(), have.end());
        std::vector<const char*> candidates(std::begin(kMustFight), std::end(kMustFight));
        candidates.push_back(kConditionalFight);
        for (const char* id : candidates) {
            const fanren::core::BattleSetup* setup = app_.battleSetup(id);
            if (setup == nullptr) continue;
            std::vector<std::pair<std::string, int>> want;
            for (const fanren::core::BattleUnitSpec& spec : setup->units) {
                if (!spec.ally) want.emplace_back(spec.roleId, spec.wave);
            }
            std::sort(want.begin(), want.end());
            if (want == have) return id;
        }
        return "?";
    }

    // -----------------------------------------------------------------------
    // 脚本
    // -----------------------------------------------------------------------
    void pumpScripts(int choiceIndex) {
        int pendingFill = -1;
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
            syncKeys();
            if (pendingFill >= 0 && dynamic_cast<BattleScene*>(app_.topScene()) == nullptr) {
                fillAfter(static_cast<std::size_t>(pendingFill));
                pendingFill = -1;
            }
            if (auto* fight = dynamic_cast<BattleScene*>(app_.topScene()); fight != nullptr) {
                if (pendingFill >= 0) continue;   // 胜负已分、还没收场的那一帧
                BattleRecord record;
                record.id = identifyBattle(*fight);
                record.step = currentStep_;
                record.hpBefore = state().hp;
                record.maxHp = state().maxHp;
                record.mpBefore = state().mp;
                record.pillsBefore = state().itemCount(kPill);
                record.salvesBefore = state().itemCount(kSalve);
                for (const Unit& u : fight->battle().units()) {
                    if (u.id == kHero) record.heroOnField = true;
                    if (u.ally) record.allyIds.push_back(u.id);
                }
                // 「这一场一开打就逃」的预算（只给「输了 / 逃了之后还往下走吗」那一条用例用）。
                fleeingThisFight_ = fleeBudget_ > 0;
                if (fleeingThisFight_) --fleeBudget_;
                record.phase = playBattle(*fight);
                fleeingThisFight_ = false;
                battles_.push_back(record);
                if (!steps_.empty()) steps_.back().phases.push_back(record.phase);
                pendingFill = static_cast<int>(battles_.size()) - 1;
                continue;
            }
            if (!app_.awaitingCommand()) continue;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = choiceIndex;
            app_.completeCommand(result);
            app_.popScene();
        }
        if (app_.scripts().isRunning()) complain("脚本没能跑到结束");
        app_.tick(kFrame);
        syncKeys();
        if (pendingFill >= 0) fillAfter(static_cast<std::size_t>(pendingFill));
    }

    void fillAfter(std::size_t index) {
        BattleRecord& record = battles_[index];
        record.hpAfter = state().hp;
        record.mpAfter = state().mp;
        record.pillsAfter = state().itemCount(kPill);
        record.salvesAfter = state().itemCount(kSalve);
    }

    // -----------------------------------------------------------------------
    // 地图与走位（与第 4 章同一套）
    // -----------------------------------------------------------------------
    const TileMap& map() const { return *app_.currentMap(); }

    MapObject objectNamed(const std::string& name) const {
        for (const MapObject& object : map().objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    bool enterable(Point p) {
        if (!map().walkable(p)) return false;
        if (WorldScene::visibleNpcAt(app_.state(), map(), p) != nullptr) return false;
        if (map().objectAt(p, "facility") != nullptr) return false;
        return true;
    }

    bool freeOfLiveEnterTrigger(Point p) {
        const MapObject* trigger = map().objectAt(p, "trigger");
        if (trigger == nullptr) return true;
        if (trigger->property("mode") != "enter") return true;
        return !WorldScene::triggerReady(app_.state(), *trigger);
    }

    bool standable(Point p, bool avoidTriggers) {
        if (!enterable(p)) return false;
        if (map().objectAt(p, "portal") != nullptr) return false;
        return !avoidTriggers || freeOfLiveEnterTrigger(p);
    }

    bool step(Point direction, int choiceIndex) {
        const bool moved = world_.tryStep(app_, direction.x, direction.y);
        if (app_.scripts().isRunning()) pumpScripts(choiceIndex);
        return moved;
    }

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

    bool walkTo(Point goal, int choiceIndex = 0, bool strict = false) {
        if (app_.state().position == goal) return true;
        std::vector<Point> path = routeTo(goal, /*avoidTriggers=*/true);
        if (path.empty() && !strict) path = routeTo(goal, /*avoidTriggers=*/false);
        if (path.empty()) return false;
        const std::string mapBefore = app_.state().mapId;
        for (const Point& cell : path) {
            const Point here = app_.state().position;
            if (!step(Point{cell.x - here.x, cell.y - here.y}, choiceIndex)) return false;
            if (app_.state().mapId != mapBefore) return false;
        }
        return app_.state().position == goal;
    }

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

    // 面朝一处 interact 触发器 / NPC 按确认并把脚本演完。返回「脚本真的起来了」。
    bool fireTrigger(const std::string& name, int choiceIndex = 0) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        if (!faceObject(trigger, choiceIndex)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        clearKeys();
        if (!world_.interact(app_)) {
            complain(name + " 点不着");
            return false;
        }
        if (!app_.scripts().isRunning()) {
            complain(name + " 点着的不是脚本");
            return false;
        }
        pumpScripts(choiceIndex);
        return true;
    }

    // 踩上踏入型触发。返回「踩上去了」（踩上去不一定开演：guard 未满足就什么也不发生）。
    bool enterTrigger(const std::string& name, int choiceIndex = 0) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        clearKeys();
        if (!stepOnto(trigger, choiceIndex)) {
            complain("走不到 " + name + " 上");
            return false;
        }
        return true;
    }

    bool pressAt(const MapObject& object, int choiceIndex = 0) {
        if (!faceObject(object, choiceIndex)) return false;
        clearKeys();
        const bool handled = world_.interact(app_);
        app_.tick(kFrame);
        return handled;
    }

    bool usePortal(const std::string& name, int choiceIndex = 0) {
        const MapObject portal = objectNamed(name);
        if (portal.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        const std::string before = app_.state().mapId;
        if (!stepOnto(portal, choiceIndex)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        return app_.state().mapId != before;
    }

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    bool travelTo(const std::string& target, int choiceIndex = 0) {
        for (int hop = 0; hop < 8; ++hop) {
            if (app_.state().mapId == target) return true;
            const std::string portal = nextPortalToward(app_.state().mapId, target);
            if (portal.empty()) {
                complain("从 " + app_.state().mapId + " 没有门通到 " + target);
                return false;
            }
            if (!usePortal(portal, choiceIndex)) {
                complain("从 " + app_.state().mapId + " 走 " + portal + " 没过去");
                return false;
            }
        }
        complain("地图之间绕了八跳还没到 " + target);
        return false;
    }

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
    // 卖清灵散：真的走进南城药铺卖（施工偏差 18.4 第 3 行）
    // -----------------------------------------------------------------------
    bool sellAllQingling() {
        if (!travelTo(kMapNancheng)) return false;
        const MapObject shop = objectNamed("facility_yaopu");
        if (shop.name.empty() || !pressAt(shop)) {
            complain("南城药铺开不了门");
            return false;
        }
        auto* panel = dynamic_cast<ShopScene*>(app_.topScene());
        if (panel == nullptr) {
            complain("按药铺开出来的不是商店面板");
            return false;
        }
        fanren::rules::Shop* shopState = app_.shop(panel->shopId());
        for (int guard = 0; guard < 16 && shopState != nullptr && state().itemCount(kQingling) > 0;
             ++guard) {
            const auto stacks = ShopScene::buildSellStacks(app_.data(), state(), *shopState);
            int row = -1;
            for (std::size_t i = 0; i < stacks.size(); ++i) {
                if (stacks[i].itemId == kQingling) row = static_cast<int>(i);
            }
            if (row < 0 || !panel->sellAt(app_, row)) break;
        }
        panel->leave(app_);
        app_.tick(kFrame);
        return state().itemCount(kQingling) == 0;
    }

    // -----------------------------------------------------------------------
    // 一步：走过去、演一趟（不重来）
    // -----------------------------------------------------------------------
    bool attemptStep(Step s, int c) {
        switch (s) {
            case Step::DongquLu: return travelTo(kMapDukou) && enterTrigger("trigger_dongqu_lu", c);
            case Step::Shangchuan: return travelTo(kMapDukou) && fireTrigger("trigger_shangchuan", c);
            case Step::Matou: return travelTo(kMapXicheng) && enterTrigger("trigger_matou", c);
            case Step::Heishui: return travelTo(kMapXicheng) && enterTrigger("trigger_heishuixiang", c);
            case Step::Zhuishao: return travelTo(kMapXicheng) && enterTrigger("trigger_zhuishao", c);
            case Step::SellQingling: return sellAllQingling();
            case Step::Qingbao: return travelTo(kMapKezhan) && fireTrigger("trigger_qingbao", c);
            case Step::Jieren: return travelTo(kMapXicheng) && fireTrigger("trigger_jieren", c);
            case Step::Jiulou: return travelTo(kMapNancheng) && fireTrigger("trigger_jiulou", c);
            case Step::Yeru: return travelTo(kMapNancheng) && fireTrigger("trigger_yeru", c);
            case Step::Toutin: return travelTo(kMapMofu) && fireTrigger("trigger_toutin", c);
            case Step::Dengmen: return travelTo(kMapMofu) && fireTrigger("trigger_dengmen", c);
            case Step::Jianmian: return travelTo(kMapMofu) && enterTrigger("trigger_jianmianli", c);
            case Step::Huayuan: return travelTo(kMapMofu) && enterTrigger("trigger_huayuan", c);
            case Step::Duizhi: return travelTo(kMapMofu) && fireTrigger("trigger_duizhi", c);
            case Step::Dingji: return travelTo(kMapKezhan) && fireTrigger("trigger_dingji", c);
            case Step::Xiaoxiang:
                return travelTo(kMapXicheng) && fireTrigger("trigger_xiaoxiangyuan", c);
            case Step::Shuijiao: return travelTo(kMapKezhan) && fireTrigger("trigger_shuijiao", c);
            case Step::Jiaoyi: return travelTo(kMapMofu) && fireTrigger("trigger_jiaoyi", c);
            case Step::Yange: return travelTo(kMapMofu) && enterTrigger("trigger_yange", c);
            case Step::Chaoxie: return travelTo(kMapKezhan) && fireTrigger("trigger_chaoxie", c);
            case Step::LianJianfu: return travelTo(kMapKezhan) && fireTrigger("trigger_lian_jianfu", c);
            case Step::Anpai: return travelTo(kMapKezhan) && fireTrigger("trigger_anpai", c);
            case Step::FengwuDeliver: return travelTo(kMapMofu) && fireTrigger("npc_mo_fengwu", c);
            case Step::Majiu: return travelTo(kMapMofu) && fireTrigger("trigger_majiu", c);
            case Step::Zhuwu: return travelTo(kMapMofu) && enterTrigger("trigger_zhuwu", c);
            case Step::Tancha: return travelTo(kMapShanzhuang) && fireTrigger("trigger_tancha", c);
            case Step::LianJianfuLin:
                return travelTo(kMapShanzhuang) && fireTrigger("trigger_lian_jianfu_lin", c);
            case Step::Shangyue: return travelTo(kMapShanzhuang) && fireTrigger("trigger_shangyue", c);
            case Step::Huanyu: return travelTo(kMapMofu) && fireTrigger("trigger_huanyu", c);
        }
        return false;
    }

    bool isDone(Step s) {
        if (s == Step::SellQingling) return state().itemCount(kQingling) == 0;
        return flag(infoOf(s).doneFlag) != 0;
    }

    // 一步，含重来（①逃、⑤输、潇湘院逃——施工图 3.2 各自写明「回头再来」）。
    // ⑩ 没练剑符：到了亭子不照面、没打就退（复验 MEDIUM-B 方案 1）——先去林子里那一处练符（Z2′），再来。
    bool runStep(Step s, const Route& route, int maxAttempts = kMaxAttempts) {
        const int c = route.choiceFor(s);
        for (int attempt = 0; attempt < maxAttempts; ++attempt) {
            beginStep(s);
            const bool started = attemptStep(s, c);
            endStep();
            if (HasFatalFailure() || app_.quitRequested()) return false;
            if (isDone(s)) return true;
            if (!started) return false;
            if (s == Step::Shangyue && flag("ch05.lian_jianfu") == 0) {
                beginStep(Step::LianJianfuLin);
                attemptStep(Step::LianJianfuLin, c);
                endStep();
            }
        }
        return isDone(s);
    }

    void beginStep(Step s) {
        currentStep_ = s;
        StepRecord record;
        record.step = s;
        record.dayBefore = state().day;
        record.pillsBefore = state().itemCount(kPill);
        steps_.push_back(record);
    }
    void endStep() {
        syncKeys();
        if (steps_.empty()) return;
        steps_.back().dayAfter = state().day;
        steps_.back().pillsAfter = state().itemCount(kPill);
    }

    std::vector<Step> stepsFor(const Route& r) const {
        std::vector<Step> out = {Step::DongquLu, Step::Shangchuan, Step::Matou, Step::Heishui,
                                 Step::Zhuishao};
        if (!r.keepQingling) out.push_back(Step::SellQingling);
        for (Step s : {Step::Qingbao, Step::Jieren, Step::Jiulou, Step::Yeru, Step::Toutin,
                       Step::Dengmen, Step::Jianmian, Step::Huayuan, Step::Duizhi, Step::Dingji,
                       Step::Xiaoxiang, Step::Shuijiao, Step::Jiaoyi, Step::Yange}) {
            out.push_back(s);
        }
        if (r.fengwuBook) out.push_back(Step::Chaoxie);
        if (r.trainJianfu) out.push_back(Step::LianJianfu);
        out.push_back(Step::Anpai);
        if (r.fengwuBook) out.push_back(Step::FengwuDeliver);
        for (Step s : {Step::Majiu, Step::Zhuwu, Step::Tancha, Step::Shangyue, Step::Huanyu}) {
            out.push_back(s);
        }
        return out;
    }

    // 整章走一遍。before / after 是用例插断言的钩子（闸门验两次、提示在前）。
    // 返回「走到了 ch05.done」。
    bool playChapter(const Route& route, const std::function<void(Step)>& before = {},
                     const std::function<void(Step)>& after = {}) {
        for (Step s : stepsFor(route)) {
            if (before) before(s);
            if (HasFatalFailure()) return false;
            const bool done = runStep(s, route);
            if (after) after(s);
            if (HasFatalFailure() || app_.quitRequested()) return false;
            if (!done) {
                stuckAt_ = infoOf(s).name;
                return false;
            }
        }
        return flag("ch05.done") == 1;
    }

    // 实际打到的编成 id（按发生顺序，重打的算多次）。
    std::vector<std::string> battleIds() const {
        std::vector<std::string> ids;
        for (const BattleRecord& b : battles_) ids.push_back(b.id);
        return ids;
    }
    const BattleRecord* lastBattle(const std::string& id) const {
        for (auto it = battles_.rbegin(); it != battles_.rend(); ++it) {
            if (it->id == id) return &*it;
        }
        return nullptr;
    }
    bool everLost() const {
        for (const BattleRecord& b : battles_) {
            if (b.phase == BattlePhase::Lost && b.id != "b05_duobang") return true;
        }
        return false;
    }

    std::string battleLine() const {
        std::ostringstream out;
        for (const BattleRecord& b : battles_) {
            out << "\n    " << b.id << " " << phaseName(b.phase) << "  韩立 " << b.hpBefore << "→"
                << b.hpAfter << "/" << b.maxHp << "  养精丹 " << b.pillsBefore << "→" << b.pillsAfter
                << "  金疮药 " << b.salvesBefore << "→" << b.salvesAfter;
        }
        return out.str();
    }

    // 从起点到现在每一步走了几天，按施工图 3.3 算出来的应走天数。
    int designDaysFor(const StepRecord& record) {
        switch (record.step) {
            case Step::DongquLu: {
                // 只有第一趟走六十天的路；逃了回头再踩不再走（施工偏差 18.3 第 8 条）。
                int earlier = 0;
                for (const StepRecord& r : steps_) {
                    if (&r == &record) break;
                    if (r.step == Step::DongquLu) ++earlier;
                }
                return earlier == 0 ? kDaysRoad : 0;
            }
            case Step::Shangchuan: return kDaysRiver;
            case Step::Qingbao: return kDaysQingbao;
            case Step::Jieren: return kDaysJieren;
            case Step::Jianmian: return kDaysJianmian;
            case Step::Shuijiao: return kDaysDuobang;
            case Step::Zhuwu: return kDaysChuzheng;
            case Step::Tancha:
                // 只剩「节点 8 报了师门」那一支会让山庄警觉：吴剑鸣总会被擒（校对 MEDIUM-2）。
                return flag("ch05.huayuan") == 2 ? kDaysTanchaAlert : kDaysTancha;
            case Step::Chaoxie: return kDaysChaoxie;
            case Step::LianJianfu:
            case Step::LianJianfuLin: return kDaysLianfu;
            case Step::Xiaoxiang: {
                // 有清灵散：毒杀，不拨日子；条件战逃了：次日再来 ＋1；打赢：不拨。
                const bool fled = std::find(record.phases.begin(), record.phases.end(),
                                            BattlePhase::Escaped) != record.phases.end();
                return fled ? kDaysXiaoxiangRetreat : 0;
            }
            case Step::Shangyue: {
                // 得手：回城十天（偏差 18.3 第 6 条挪到这里）。没练成剑符那一趟不照面、当夜退回林子，
                // 不拨日子（复验 MEDIUM-B 方案 1；练符那两天记在 Z2′ 名下）。⑩ 不许逃（can_escape 假）。
                const bool won = std::find(record.phases.begin(), record.phases.end(),
                                           BattlePhase::Won) != record.phases.end();
                return won ? kDaysHomeward : 0;
            }
            default: return 0;
        }
    }

    // 每一步的天数都对得上施工图 3.3（逐步，不只看总数）。
    void expectEveryStepTookTheDesignDays() {
        for (const StepRecord& record : steps_) {
            if (record.dayAfter == 0) continue;
            const int took = record.dayAfter - record.dayBefore;
            const int want = designDaysFor(record);
            EXPECT_EQ(took, want) << infoOf(record.step).name << " 走了 " << took
                                  << " 天，施工图 3.3 该是 " << want << " 天";
        }
    }

    // 十场必打一场不少、潇湘院只在身上没有清灵散的那一侧出现（验收 15 ②）。
    void expectTheTenMustFightsAllHappened(bool expectConditional) {
        const std::vector<std::string> ids = battleIds();
        for (const char* id : kMustFight) {
            EXPECT_NE(std::find(ids.begin(), ids.end(), id), ids.end())
                << "必打的 " << id << " 这一趟没有打到（施工图 8.0：十场一场也不许绕过去）："
                << battleLine();
        }
        const bool sawConditional = std::find(ids.begin(), ids.end(), kConditionalFight) != ids.end();
        EXPECT_EQ(sawConditional, expectConditional)
            << "潇湘院条件战" << (expectConditional ? "该打而没打" : "不该打却打了")
            << "（施工图 8.2：没有清灵散才打）：" << battleLine();
        for (const std::string& id : ids) {
            EXPECT_NE(id, "?") << "有一场认不出是哪一张编成：" << battleLine();
        }
    }

    // ⑧ 的落点台词按 phase 分三句（复验 LOW-b）；另两句一句也不许说。
    void expectZhuwuLineMatches(BattlePhase phase) {
        const char* want = phase == BattlePhase::Won       ? "ch05.zhuwu.won"
                           : phase == BattlePhase::Escaped ? "ch05.zhuwu.blocked"
                                                           : "ch05.zhuwu.felled";
        EXPECT_TRUE(spokeIn(Step::Zhuwu, want)) << "⑧ 的落点台词没对上：该是 " << want << battleLine();
        for (const char* other : {"ch05.zhuwu.won", "ch05.zhuwu.blocked", "ch05.zhuwu.felled"}) {
            if (std::string(other) == want) continue;
            EXPECT_FALSE(spokeIn(Step::Zhuwu, other)) << "⑧ 落点是 " << want << "，却说了 " << other;
        }
    }

    // 没练成剑符到了亭子（复验 MEDIUM-B，协调者拍板方案 1；施工图 3.2 12d、16.1 第 16 条）：
    // 头一趟不开战、不拨日子，说的是「不照面」那三句、开战那一夜的句子一句不说；
    // 之后去林子里那一处练符（Z2′）；再来那一趟才开打，只打这一次，一招取首级。
    void expectTheUnpreparedNightIsOnlyWatched() {
        std::vector<std::size_t> visits;
        std::size_t lin = steps_.size();
        for (std::size_t i = 0; i < steps_.size(); ++i) {
            if (steps_[i].step == Step::Shangyue) visits.push_back(i);
            if (steps_[i].step == Step::LianJianfuLin && lin == steps_.size()) lin = i;
        }
        ASSERT_EQ(visits.size(), 2u) << "没练符：先来一趟不照面，练成了再来一趟" << battleLine();
        const StepRecord& first = steps_[visits[0]];
        const StepRecord& second = steps_[visits[1]];
        const auto said = [](const StepRecord& r, const char* key) {
            return std::find(r.keys.begin(), r.keys.end(), key) != r.keys.end();
        };
        EXPECT_TRUE(first.phases.empty()) << "没练符那一趟不许开战（不照面）" << battleLine();
        EXPECT_EQ(first.dayAfter, first.dayBefore) << "当夜就退，不拨日子";
        for (const char* key : {"ch05.shangyue.nofu", "ch05.shangyue.repelled", "ch05.shangyue.repelled_hint"}) {
            EXPECT_TRUE(said(first, key)) << "不照面那一趟该说 " << key;
        }
        for (const char* key : {"ch05.shangyue.night", "ch05.shangyue.fu", "ch05.shangyue.won_fu"}) {
            EXPECT_FALSE(said(first, key)) << "不照面那一趟不该说 " << key;
        }
        EXPECT_TRUE(visits[0] < lin && lin < visits[1]) << "林子里练符（Z2′）该夹在两趟之间";
        EXPECT_EQ(second.phases, std::vector<BattlePhase>{BattlePhase::Won}) << "练成了符回来，一招取首级"
                                                                            << battleLine();
        EXPECT_TRUE(said(second, "ch05.shangyue.fu"));
        EXPECT_FALSE(said(second, "ch05.shangyue.nofu"));
    }

    // 验收 2 的终点先验（施工图第 15 节第 2 行，逐条抄）。
    void expectTheChapterEndState() {
        GameState& s = state();
        EXPECT_EQ(s.flag("ch05.done"), 1);
        EXPECT_TRUE(s.party.empty()) << "施工图验收 2：终点 party 为空（曲魂长久留在四平帮）";
        EXPECT_EQ(s.itemCount(kBaoyu), 1) << "施工图验收 2：story_nuanyang_baoyu == 1";
        EXPECT_EQ(s.itemCount(kQinbixin), 0) << "施工图验收 2：亲笔信 0（登门时呈给了严氏）";
        EXPECT_EQ(s.itemCount(kWenlong), 0) << "施工图验收 2：纹龙戒 0（留在严氏手里）";
        EXPECT_EQ(s.itemCount(kTalisman), 1) << "施工图验收 2：剑符 1";
        EXPECT_EQ(s.itemCount(kPlaque), 1) << "施工图验收 2：牌子 1（本章任何脚本不碰）";
        const std::vector<std::string> twoMagics{kFireMagic, kWindMagic};
        EXPECT_EQ(s.learnedMagics, twoMagics)
            << "施工图验收 2：learnedMagics 仍是那两门（「祭剑符」已还回去）";
        EXPECT_EQ(s.realm, Realm::QiRefining8) << "施工图验收 5：本章不升境";
        EXPECT_EQ(s.realmCap, Realm::QiRefining8) << "施工图验收 5：本章不抬上限";
        EXPECT_EQ(s.flag("story.xiuxian_known"), 0) << "施工图第 11 节：本章不置";
    }

    // 施工图 12.2 的章内先后，**按玩家实际读到的顺序**判（与 Ch05LexiconTests 按 key 前缀判的
    // 形状不同：那一份信 key 的场景段，看不见「挂在 A 节名下的文案被 B 节的脚本说了」）。
    // 返回每个词头一回被说出来的节点号（没说过为 0）。
    std::map<std::string, int> firstNodeOfEachGatedWord() {
        std::map<std::string, int> first;
        for (const auto& entry : chapterLog_) {
            const std::string text = app_.data().lookupText(entry.second);
            const int node = infoOf(entry.first).node;
            for (const char* word : {"寒毒", "惊蛟会", "五色门", "独霸山庄", "太南谷", "太南山", "升仙",
                                     "仙令", "天眼"}) {
                if (text.find(word) != std::string::npos && first[word] == 0) first[word] = node;
            }
        }
        return first;
    }

    void expectGatedWordsArriveWhereTheDesignSays() {
        ASSERT_GT(chapterLog_.size(), 150u) << "先验：整章说过的句子够多，下面的「没有」才有分量";
        std::map<std::string, int> first = firstNodeOfEachGatedWord();
        // 施工图 12.2：「升仙」「仙令」「天眼」一条也不许有。
        EXPECT_EQ(first["升仙"], 0) << "「升仙」在第 " << first["升仙"] << " 节说出来了";
        EXPECT_EQ(first["仙令"], 0) << "「仙令」在第 " << first["仙令"] << " 节说出来了";
        EXPECT_EQ(first["天眼"], 0) << "「天眼」在第 " << first["天眼"] << " 节说出来了";
        // 施工图 12.2 / 3.2：头一回出现在哪一节（既不许早，也不许一次都不说）。
        EXPECT_EQ(first["惊蛟会"], 4) << "施工图 4a：「惊蛟会」三个字在本节第一次出现";
        EXPECT_EQ(first["寒毒"], 9) << "施工图 9：「寒毒」一词在这里第一次出现";
        EXPECT_EQ(first["五色门"], 9) << "施工图 9：「五色门」首次出现";
        EXPECT_EQ(first["独霸山庄"], 9) << "施工图 9：「独霸山庄」首次出现";
        EXPECT_EQ(first["太南谷"], 11) << "施工图 11c：太南谷";
        EXPECT_EQ(first["太南山"], 11) << "施工图 11c：太南山";
    }

    // 告示板上某条支线那一行（按任务名找）。没有返回空行。
    fanren::ui::ListItem boardRowOf(const std::string& questId) {
        const fanren::core::Quest* quest = nullptr;
        for (const fanren::core::Quest& q : app_.data().quests) {
            if (q.id == questId) quest = &q;
        }
        if (quest == nullptr) return {};
        const std::string title = app_.data().lookupText(quest->titleKey);
        for (const fanren::ui::ListItem& row : BoardScene::buildRows(app_.data(), state())) {
            if (row.label.rfind(title, 0) == 0) return row;
        }
        return {};
    }
    const fanren::core::Quest* questOf(const std::string& questId) {
        for (const fanren::core::Quest& q : app_.data().quests) {
            if (q.id == questId) return &q;
        }
        return nullptr;
    }
    fanren::rules::QuestStatus statusOf(const std::string& questId) {
        const fanren::core::Quest* quest = questOf(questId);
        if (quest == nullptr) return fanren::rules::QuestStatus::NotAccepted;
        return fanren::rules::questStatus(*quest, app_.data().objectives, state());
    }
    // 过期那一行：「任务名：原因」，右栏「已过期」（契约 1.7）。先验原因确实有内容。
    void expectExpiredRow(const std::string& questId, const std::string& why) {
        const fanren::core::Quest* quest = questOf(questId);
        ASSERT_NE(quest, nullptr) << questId;
        const std::string title = app_.data().lookupText(quest->titleKey);
        const std::string reason = app_.data().lookupText(quest->failTextKey);
        ASSERT_FALSE(reason.empty());
        ASSERT_NE(reason, quest->failTextKey) << "先验：过期原因那句文案查得到";
        EXPECT_EQ(statusOf(questId), fanren::rules::QuestStatus::Failed) << why;
        const fanren::ui::ListItem row = boardRowOf(questId);
        EXPECT_EQ(row.label, title + "：" + reason) << why << "——过期要写原因，不许静默消失（契约 1.7、Q7）";
        EXPECT_EQ(row.detail, BoardScene::kFailedTag) << why;
    }
    void expectCompletedRow(const std::string& questId, const std::string& why) {
        const fanren::core::Quest* quest = questOf(questId);
        ASSERT_NE(quest, nullptr) << questId;
        EXPECT_EQ(statusOf(questId), fanren::rules::QuestStatus::Completed) << why;
        const fanren::ui::ListItem row = boardRowOf(questId);
        EXPECT_EQ(row.label, app_.data().lookupText(quest->titleKey)) << why;
        EXPECT_EQ(row.detail, BoardScene::kCompletedTag) << why;
    }

    // 世界拨回某一刻（扫描与插桩用）。整个世界就是这一份 GameState。
    void restoreTo(const GameState& snapshot) {
        state() = snapshot;
        auto loaded = app_.loadMap(snapshot.mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state().position = snapshot.position;
        state().facing = snapshot.facing;
        battles_.clear();
        steps_.clear();
        chapterLog_.clear();
        stuckAt_.clear();
    }

    void setPills(int count) {
        while (state().removeItem(kPill, 1)) {
        }
        if (count > 0) state().addItem(kPill, count, 0);
    }

    Application app_;
    WorldScene world_;
    HandPolicy hand_;
    int startDay_ = 1;
    int startPills_ = 0;
    GameState startSnapshot_;
    bool tolerateBlocked_ = false;
    int fleeBudget_ = 0;            // 接下来几场一开打就逃（能逃的话）
    bool fleeingThisFight_ = false;
    Step currentStep_ = Step::DongquLu;
    std::size_t keysSeen_ = 0;
    std::vector<BattleRecord> battles_;
    std::vector<StepRecord> steps_;
    std::vector<std::pair<Step, std::string>> chapterLog_;
    std::string stuckAt_;
};

// ---------------------------------------------------------------------------
// 起点：第 4 章终局原样读入，一个字段也不改
// ---------------------------------------------------------------------------
TEST_F(Ch05Walkthrough, StartsFromTheChapterFourHandOverWithNoFieldChanged) {
    for (const char* side : {fanren::test::kChapterFourEndingFirst,
                             fanren::test::kChapterFourEndingSecond}) {
        startFromChapterFourEnding(side);
        ASSERT_FALSE(HasFatalFailure());
        const auto fixture = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), side).string());
        ASSERT_TRUE(fixture.ok);
        const auto expected = fanren::test::comparableSaveLines(fixture.value);
        const auto actual = fanren::test::comparableSaveLines(state());
        ASSERT_GT(expected.size(), 20u) << "先验：存档写得出来";
        const auto diff = fanren::test::saveLineDifferences(expected, actual);
        EXPECT_TRUE(diff.empty()) << side << " 读进来之后有字段被改了——施工图第 2 节："
                                  << "与第 4 章终局不同的字段一个也不应有：" << fanren::test::joinLines(diff);
    }
}

// ---------------------------------------------------------------------------
// 第一侧：每一处取第一项，全备（清灵散留着、练剑符、医书做完），沿途的闸门验两次
// ---------------------------------------------------------------------------
TEST_F(Ch05Walkthrough, FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    Route route = routeFor(0, Prep::Full);
    route.fengwuBook = true;
    GameState& s = state();
    const int qinglingAtStart = s.itemCount(kQingling);
    ASSERT_GE(qinglingAtStart, 2) << "先验：第 4 章终局带着两份清灵散（施工偏差 18.4）";
    int qinglingBeforeDuizhi = 0;
    int qinglingBeforeXiaoxiang = 0;
    int herbsBeforeDelivery = 0;
    int hpBeforeDuobang = 0;
    int mpBeforeDuobang = 0;

    const auto before = [&](Step step) {
        switch (step) {
            case Step::DongquLu: {
                // 本章入口：韩家村东口那道门认 ch04.done。撤掉验一次拦得住，放回来验开得了。
                ASSERT_EQ(s.mapId, kMapHanjiacun);
                const MapObject gate = objectNamed("portal_to_dukou");
                ASSERT_FALSE(gate.name.empty()) << "施工图第 4 节：韩家村东口加 portal_to_dukou";
                EXPECT_EQ(gate.property("require_flag"), "ch04.done");
                s.flags.erase("ch04.done");
                EXPECT_FALSE(usePortal("portal_to_dukou")) << "ch04 没完，东口不该放行";
                app_.tick(kFrame);
                ASSERT_NE(app_.topScene(), nullptr);
                EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
                closePanel();
                s.setFlag("ch04.done");
                break;
            }
            case Step::Jiulou: {
                // 墨府正门认 ch05.dengmen：此刻还没登门，从南城走过去必须被拦下（不必撤旗标，
                // 局面本身就是反面）。
                ASSERT_TRUE(travelTo(kMapNancheng));
                const MapObject gate = objectNamed("portal_to_mofu");
                ASSERT_FALSE(gate.name.empty());
                EXPECT_EQ(gate.property("require_flag"), "ch05.dengmen");
                EXPECT_FALSE(usePortal("portal_to_mofu")) << "还没登门，墨府正门不该放行";
                EXPECT_EQ(s.mapId, kMapNancheng);
                app_.tick(kFrame);
                ASSERT_NE(app_.topScene(), nullptr);
                EXPECT_EQ(app_.topScene()->name(), "Dialogue") << "拦住玩家就要说明还差什么";
                closePanel();
                break;
            }
            case Step::Duizhi: qinglingBeforeDuizhi = s.itemCount(kQingling); break;
            case Step::Xiaoxiang: {
                qinglingBeforeXiaoxiang = s.itemCount(kQingling);
                // R-3 提示一：定计之后目标行就是潇湘院那一步，写着清灵散。
                const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
                ASSERT_NE(obj, nullptr);
                EXPECT_EQ(obj->id, kHintStepXiaoxiang);
                EXPECT_NE(app_.data().lookupText(obj->textKey).find("清灵散"), std::string::npos);
                break;
            }
            case Step::Shuijiao:
                hpBeforeDuobang = s.hp;
                mpBeforeDuobang = s.mp;
                break;
            case Step::FengwuDeliver:
                herbsBeforeDelivery = s.itemCountOfAge(kHuangjing, 40) * 100 + s.itemCountOfAge(kZishen, 40);
                break;
            default: break;
        }
    };
    const auto after = [&](Step step) {
        if (step == Step::Zhuishao) {
            // 西城去南城那道门认 ch05.zhuishao。站到门里侧，撤掉旗标验一次拦得住。
            const MapObject gate = objectNamed("portal_to_nancheng");
            ASSERT_FALSE(gate.name.empty());
            EXPECT_EQ(gate.property("require_flag"), "ch05.zhuishao");
            ASSERT_TRUE(walkTo(Point{gate.position.x, gate.position.y - 2}));
            s.flags.erase("ch05.zhuishao");
            EXPECT_FALSE(usePortal("portal_to_nancheng")) << "盯梢那一仗没打，南城不该放行";
            app_.tick(kFrame);
            ASSERT_NE(app_.topScene(), nullptr);
            EXPECT_EQ(app_.topScene()->name(), "Dialogue");
            closePanel();
            s.setFlag("ch05.zhuishao");
        }
        if (step == Step::Jiaoyi) {
            // 支线并行（施工图第 6 节）：Z1、Z2 同时挂起，Z3 已了结，主线照走。
            EXPECT_EQ(statusOf("q05_fengwu_yishu"), fanren::rules::QuestStatus::Active);
            EXPECT_EQ(statusOf("q05_jianfu"), fanren::rules::QuestStatus::Active);
            EXPECT_EQ(boardRowOf("q05_fengwu_yishu").detail, BoardScene::kActiveTag);
            EXPECT_EQ(boardRowOf("q05_jianfu").detail, BoardScene::kActiveTag);
            // 进行中那一行：「任务名：当前步骤（地名）」（契约 1.7）。
            const auto* quest = questOf("q05_fengwu_yishu");
            ASSERT_NE(quest, nullptr);
            const auto* current = fanren::rules::currentQuestStep(*quest, app_.data().objectives, s);
            ASSERT_NE(current, nullptr);
            const std::string place =
                app_.data().lookupText(fanren::core::mapDisplayNameKey(current->targetMap));
            ASSERT_FALSE(place.empty());
            EXPECT_EQ(boardRowOf("q05_fengwu_yishu").label,
                      app_.data().lookupText(quest->titleKey) + "：" + app_.data().lookupText(current->textKey) +
                          "（" + place + "）");
            // 主线照走：HUD 那一步是 11b（燕歌），不被支线拽走（契约 Q5）。
            const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
            ASSERT_NE(obj, nullptr);
            EXPECT_EQ(obj->id, "n19_yange");
        }
        if (step == Step::Yeru) {
            // 施工偏差 18.1 第 1 条：翻进来、还没登门的这一夜，走不到正门（暗哨站在月洞门里）——
            // 否则出了正门就再也进不来（正门要 ch05.dengmen）。
            ASSERT_EQ(s.mapId, kMapMofu);
            const MapObject gate = objectNamed("portal_to_nancheng");
            ASSERT_FALSE(gate.name.empty());
            EXPECT_TRUE(routeTo(gate.position, /*avoidTriggers=*/false).empty())
                << "夜里翻进墨府后园，还没登门就走得到正门——出去了就再也进不来（施工偏差 18.1）";
        }
        if (step == Step::Dengmen) {
            // 登门之后暗哨退场，正门走得通（反面：上面那一条不是因为正门本来就走不到）。
            const MapObject gate = objectNamed("portal_to_nancheng");
            ASSERT_FALSE(gate.name.empty());
            EXPECT_FALSE(routeTo(gate.position, /*avoidTriggers=*/false).empty())
                << "登门之后正门该走得通";
        }
        if (step == Step::Dingji) {
            // Z3 在 10a 挂起；身上已有清灵散，第一步挂起那一刻就是做完的，当前步骤落在第二步。
            const auto* quest = questOf("q05_qingling");
            ASSERT_NE(quest, nullptr);
            EXPECT_EQ(statusOf("q05_qingling"), fanren::rules::QuestStatus::Active);
            const auto* current = fanren::rules::currentQuestStep(*quest, app_.data().objectives, s);
            ASSERT_NE(current, nullptr);
            EXPECT_EQ(current->done.size(), 1u);
            EXPECT_EQ(current->done[0].subject, "ch05.xiaoxiang") << "身上有清灵散，该落在送酒那一步";
        }
    };

    ASSERT_TRUE(playChapter(route, before, after)) << "走不到 ch05.done，卡在「" << stuckAt_ << "」"
                                                   << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << "走到了 game_over 那一条" << battleLine();

    // ---- 每一处主动选择都取了第一项（施工图第 11 节的取值）----
    EXPECT_EQ(flag("ch05.shoufu"), 1) << "3a 第一项：腐心丸 ＋ 一袋碎银";
    EXPECT_EQ(flag("ch05.qingbao"), 1) << "4a 第一项：再赏";
    EXPECT_EQ(flag("ch05.yeru"), 1) << "6a 第一项：后墙";
    EXPECT_EQ(flag("ch05.toutin"), 1) << "6b 第一项：听完";
    EXPECT_EQ(flag("ch05.dengmen"), 1) << "7a 第一项：运功抬头";
    EXPECT_EQ(flag("ch05.jianmianli"), 1) << "7b 第一项：萦香丸";
    EXPECT_EQ(flag("ch05.huayuan"), 1) << "8 第一项：堂侄";
    EXPECT_EQ(flag("ch05.duizhi"), 1) << "9 第一项：狠";
    EXPECT_EQ(flag("ch05.jiaoyi"), 1) << "11a 第一项：请严氏吞药";
    EXPECT_EQ(flag("ch05.anpai"), 1) << "11c 第一项：解毒丹";

    // ---- 节点 1：前提补种（验收 8）；第一侧 ch03.jieyao_xuan == 1 → 黑丸那一句 ----
    ASSERT_EQ(flag("ch03.jieyao_xuan"), 1) << "先验：第一侧是「吞下」那一侧";
    EXPECT_TRUE(spokeIn(Step::DongquLu, "ch05.dongqu.hook_eat"));
    EXPECT_FALSE(spokeIn(Step::DongquLu, "ch05.dongqu.hook_test")) << "两条钩子只能演一条";
    EXPECT_EQ(s.itemCount(kYishu), 1) << "施工图 3.2 节点 1：遗书给了";
    EXPECT_EQ(flag("ch05.yindu_qi"), startDay_ + kDaysRoad) << "施工图 3.3：六十天的路之后那一夜起表";

    // ---- 节点 6a 取后墙：暗舵那一段听得到（施工图 6a 第二项才错过它）----
    bool heardAnshe = false;
    for (const std::string& t : textsIn(Step::Toutin)) heardAnshe = heardAnshe || t.find("暗舵") != std::string::npos;
    EXPECT_TRUE(heardAnshe) << "走后墙准时到了小楼，暗舵那一段该听得到";

    // ---- 节点 9、10b：清灵散自动核（施工图 3.2）----
    EXPECT_EQ(flag("ch05.qianrenzui"), 1) << "身上有清灵散，千人醉防住了";
    EXPECT_EQ(qinglingBeforeDuizhi, qinglingAtStart) << "节点 9 之前没有别处碰清灵散";
    EXPECT_EQ(flag("ch05.xiaoxiang"), 1) << "身上有清灵散，潇湘院照原著毒杀";
    EXPECT_EQ(qinglingBeforeXiaoxiang, 1) << "节点 9 核掉了一份，剩一份进潇湘院";
    EXPECT_EQ(s.itemCount(kQingling), 0) << "两份各核一份";

    // ---- 节点 10c：韩立不在场（验收 16）----
    const BattleRecord* duobang = lastBattle("b05_duobang");
    ASSERT_NE(duobang, nullptr);
    EXPECT_FALSE(duobang->heroOnField) << "夺帮那一夜韩立在客栈睡觉，单位表里不该有 hanli";
    EXPECT_NE(std::find(duobang->allyIds.begin(), duobang->allyIds.end(), kCompanion),
              duobang->allyIds.end()) << "曲魂以友军出场（施工图 8.2 ⑤）";
    EXPECT_EQ(duobang->hpAfter, duobang->hpBefore) << "战后气血一点不动（施工图 10.2）";
    EXPECT_EQ(duobang->mpAfter, duobang->mpBefore) << "战后法力一点不动（施工图 10.2）";
    EXPECT_EQ(duobang->pillsAfter, duobang->pillsBefore) << "施工图 8.4 ⑤：不动背包";
    static_cast<void>(hpBeforeDuobang);
    static_cast<void>(mpBeforeDuobang);

    // ---- 12d 练成了剑符：借一招、第一回合取首级、战后还回去（施工图 10.3）----
    EXPECT_EQ(flag("ch05.lian_jianfu"), 1);
    EXPECT_TRUE(spokeIn(Step::Shangyue, "ch05.shangyue.fu"));
    EXPECT_FALSE(spokeIn(Step::Shangyue, "ch05.shangyue.nofu")) << "出发前练成了，不该有不照面那一趟";
    const BattleRecord* ouyang = lastBattle("b05_ouyang_feitian");
    ASSERT_NE(ouyang, nullptr);
    EXPECT_EQ(ouyang->phase, BattlePhase::Won);
    EXPECT_EQ(ouyang->pillsAfter, ouyang->pillsBefore) << "施工图 8.4 ⑩：剑符那条路 0 瓶";

    // ---- Z1 交付：药圃的年份药材（施工图第 6 节 Z1 奖励：黄精 6、紫参 3，40 年）----
    EXPECT_EQ(flag("ch05.fengwu_yigao"), 1);
    EXPECT_EQ(s.itemCountOfAge(kHuangjing, 40) * 100 + s.itemCountOfAge(kZishen, 40) - herbsBeforeDelivery,
              6 * 100 + 3) << "Z1 交付该发黄精 6、紫参 3（40 年）";
    EXPECT_EQ(s.itemCount(kChaoben), 0) << "抄本交出去了";
    // 了结压过过期：交过医书的人章末照样置 ch05.done（契约 1.3）。
    expectCompletedRow("q05_fengwu_yishu", "Z1 交了医书");
    expectCompletedRow("q05_jianfu", "Z2 练成了剑符");
    expectCompletedRow("q05_qingling", "Z3 潇湘院毒杀");

    // ---- 12e：请过严氏吞药 → 给解药（施工图 3.2 12e）----
    EXPECT_TRUE(spokeIn(Step::Huanyu, "ch05.huanyu.antidote"));

    // ---- 日历（施工图 3.3，逐步）----
    expectEveryStepTookTheDesignDays();
    EXPECT_EQ(dayCount(), kDayAtTheEndFastest + kDaysChaoxie + kDaysLianfu)
        << "主线最快 59，外加 Z1 一天、Z2 两天";
    EXPECT_EQ(flag("ch05.handu_fa"), 0) << "d 没到 65，寒毒一次也不该发作";

    expectTheTenMustFightsAllHappened(/*expectConditional=*/false);
    EXPECT_FALSE(everLost()) << battleLine();
    expectTheChapterEndState();
    expectGatedWordsArriveWhereTheDesignSays();

    std::cout << "\n[ch05 第一侧通关] " << (s.day - startDay_) << " 天（d=" << dayCount() << "），"
              << battles_.size() << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterFiveEndingFirst);

    // 宝玉到手即停表（施工图 3.3）：章末之后回客栈内视，说的是「不长了」，一样也不扣。
    // （放在交接存档比对之后：走回客栈会改位置。）
    const int pillsAtTheEnd = s.itemCount(kPill);
    ASSERT_TRUE(travelTo(kMapKezhan));
    ASSERT_TRUE(fireTrigger("trigger_neishi"));
    EXPECT_EQ(app_.lastSpokenKey(), "ch05.neishi.stop") << "宝玉到手，内视该说停表那一句";
    EXPECT_EQ(s.itemCount(kPill), pillsAtTheEnd);
}

// ---------------------------------------------------------------------------
// 第二侧：每一处取第二项，什么都不备（0 瓶养精丹、不练剑符、不做医书）
// ---------------------------------------------------------------------------
// 协调者定：第二侧就是「什么都不备」那一档——不炼药、不练剑符、不做医书、身上 0 瓶养精丹。
// 清灵散是第 4 章带来的两份，节点 9、10b 自动核掉；这一侧**没有去卖**（玩家什么也没做，
// 身上本来就有）。按施工图 8.5「路」那一维的字面，这一条落在「只备清灵散」那一格；
// 真把清灵散卖掉的「只练剑符」「都不备」两格在 Ch05EveryPrep 里。
// 这一侧要核对的是施工图 8.4 与 R-3：**必败的那几仗之前，提示真的出现了**。
TEST_F(Ch05Walkthrough, SecondSideTakesEveryOtherChoiceWithNothingPrepared) {
    startFromChapterFourEnding(fanren::test::kChapterFourEndingSecond);
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(flag("ch03.jieyao_xuan"), 2) << "先验：第二侧是「先验」那一侧";
    ASSERT_EQ(state().itemCount(kPill), 0) << "先验：第二侧 0 瓶养精丹（契约 3.1、施工偏差 18.4）";
    Route route = routeFor(1, Prep::Nothing);
    route.name = "第二侧·什么都不备";
    route.keepQingling = true;   // 没去卖：第 4 章带来的两份原样留着
    route.trainJianfu = false;
    route.fengwuBook = false;
    GameState& s = state();

    std::vector<std::string> hintsSeen;
    const auto before = [&](Step step) {
        if (step == Step::Majiu) {
            const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
            ASSERT_NE(obj, nullptr);
            EXPECT_EQ(obj->id, kHintStepMajiu) << "进马厩之前目标行该是第 21 步";
            EXPECT_NE(app_.data().lookupText(obj->textKey).find("药"), std::string::npos)
                << "第 21 步的提示该提到药";
            if (obj->id == std::string(kHintStepMajiu)) hintsSeen.push_back("第21步");
        }
        if (step == Step::Tancha) {
            const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
            ASSERT_NE(obj, nullptr);
            EXPECT_EQ(obj->id, kHintStepTancha) << "到了山庄外，目标行该是第 23 步";
            EXPECT_NE(app_.data().lookupText(obj->textKey).find("剑符"), std::string::npos);
            if (obj->id == std::string(kHintStepTancha)) hintsSeen.push_back("第23步");
        }
        if (step == Step::Shangyue) {
            // 没练成剑符走到亭子之前，目标行就该说出「剑符」（复验 MEDIUM-B 方案 1 的目标提示）。
            const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
            ASSERT_NE(obj, nullptr);
            EXPECT_EQ(obj->id, kHintStepCisha) << "上亭子之前目标行该是第 24 步";
            EXPECT_NE(app_.data().lookupText(obj->textKey).find("剑符"), std::string::npos)
                << "第 24 步该提醒：剑符催得动了再下手";
            if (obj->id == std::string(kHintStepCisha)) hintsSeen.push_back("第24步");
        }
    };

    ASSERT_TRUE(playChapter(route, before)) << "走不到 ch05.done，卡在「" << stuckAt_ << "」"
                                            << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << "走到了 game_over 那一条" << battleLine();

    // ---- 每一处主动选择都取了第二项 ----
    EXPECT_EQ(flag("ch05.shoufu"), 2) << "3a 第二项：曲魂的手";
    EXPECT_EQ(flag("ch05.qingbao"), 2) << "4a 第二项：不赏";
    EXPECT_EQ(flag("ch05.yeru"), 2) << "6a 第二项：屋顶";
    EXPECT_EQ(flag("ch05.toutin"), 2) << "6b 第二项：早现身";
    EXPECT_EQ(flag("ch05.dengmen"), 2) << "7a 第二项：低头";
    EXPECT_EQ(flag("ch05.jianmianli"), 2) << "7b 第二项：碎银（身上够 20 块，这一项列得出来）";
    EXPECT_EQ(flag("ch05.huayuan"), 2) << "8 第二项：报师门";
    EXPECT_EQ(flag("ch05.duizhi"), 2) << "9 第二项：那就谈";
    EXPECT_EQ(flag("ch05.jiaoyi"), 2) << "11a 第二项：不必";
    EXPECT_EQ(flag("ch05.anpai"), 2) << "11c 第二项：按月";

    // ---- 节点 1：第二侧 ch03.jieyao_xuan == 2 → 「原来在这里」那一句（验收 8）----
    EXPECT_TRUE(spokeIn(Step::DongquLu, "ch05.dongqu.hook_test"));
    EXPECT_FALSE(spokeIn(Step::DongquLu, "ch05.dongqu.hook_eat"));

    // ---- 6a 屋顶：到小楼时暗舵那一段已经说完了（施工图 6a 第二项）----
    for (const std::string& t : textsIn(Step::Toutin)) {
        EXPECT_EQ(t.find("暗舵"), std::string::npos) << "走屋顶来晚了，不该还听得到暗舵那一段：" << t;
    }

    // ---- R-3：必败的那几仗之前，两处提示都出现过（施工图 8.4）----
    // ⑧ 吴剑鸣：提示一 第 21 步（上面钩子里验了）；提示二 11c 孙二狗那一句。
    bool anpaiQuote = false;
    for (const std::string& t : textsIn(Step::Anpai)) {
        anpaiQuote = anpaiQuote || t.find(kHintAnpaiQuote) != std::string::npos;
    }
    EXPECT_TRUE(anpaiQuote) << "施工图 8.4：11c 孙二狗递来一句「" << kHintAnpaiQuote << "」";
    // ⑩ 欧阳飞天：提示一 第 23 步；提示二 11a「刀剑难伤」＋ 12c 看见刀从他胳膊上弹开。
    bool jiaoyiHard = false;
    for (const std::string& t : textsIn(Step::Jiaoyi)) {
        jiaoyiHard = jiaoyiHard || (t.find("刀剑") != std::string::npos && t.find("难伤") != std::string::npos);
    }
    EXPECT_TRUE(jiaoyiHard) << "施工图 11a：严氏交代欧阳飞天「刀剑难伤」";
    bool tanchaBounce = false;
    for (const std::string& t : textsIn(Step::Tancha)) {
        tanchaBounce = tanchaBounce || (t.find("胳膊") != std::string::npos && t.find("弹") != std::string::npos);
    }
    EXPECT_TRUE(tanchaBounce) << "施工图 12c：看见刀从他胳膊上弹开";
    EXPECT_EQ(hintsSeen.size(), 3u);

    // ---- 0 瓶养精丹时各仗的落点（施工图 8.4「身上 0 瓶养精丹时」一列）----
    for (const BattleRecord& b : battles_) {
        EXPECT_EQ(b.pillsBefore, 0) << "第二侧一瓶也没有，也炼不出来（这一侧不做医书）";
        if (winsWithNoPillPerDesign(b.id)) {
            EXPECT_EQ(b.phase, BattlePhase::Won)
                << b.id << "：施工图 8.4 写「0 瓶也赢，无必败分支」" << battleLine();
        }
        if (b.id == "b05_wu_jianming") continue;   // 输了也被擒（下面那一条）
        EXPECT_NE(b.phase, BattlePhase::Lost) << b.id << "：输了（除夺帮、吴剑鸣外输即 game_over）";
    }
    // ⑧ 吴剑鸣：他总会被擒（校对 MEDIUM-2 的拍板）——打赢是亲手按倒，输了、逃了是护院堵回侧门。
    const BattleRecord* wu = lastBattle("b05_wu_jianming");
    ASSERT_NE(wu, nullptr);
    EXPECT_EQ(flag("ch05.qinwu"), 1) << "吴剑鸣没被擒";
    // 台词按实际落点分三句（复验 LOW-b）：赢 → 亲手按倒；逃 → 他抽身退开、护院在侧门把吴剑鸣围住；
    // 输 → 他跪倒、护院合围。玩家的逃不许叙述成对方的逃。
    expectZhuwuLineMatches(wu->phase);
    // ⑨ 巡庄：迷倒不杀（16.1 第 14 条；复验 LOW-c 建议在通关里钉住这一句）。
    EXPECT_TRUE(spokeIn(Step::Tancha, "ch05.tancha.drugged")) << "⑨ 赢了该是一包药放倒、不杀";
    EXPECT_TRUE(spokeIn(Step::Tancha, "ch05.tancha.drugged2"));
    // ⑩ 欧阳飞天：没练剑符就不照面（复验 MEDIUM-B，协调者拍板方案 1）——头一趟伏在墙头看一夜、
    // 悄悄退回林子，不开战、不拨日子；在林子里那一处练符（Z2′）；再来只打一次，一招取首级。
    int ouyangFights = 0;
    for (const BattleRecord& b : battles_) ouyangFights += b.id == "b05_ouyang_feitian" ? 1 : 0;
    EXPECT_EQ(ouyangFights, 1) << "⑩ 只打持符的那一次：没练符那一趟不照面（施工图 3.2、16.1 第 16 条）"
                               << battleLine();
    expectTheUnpreparedNightIsOnlyWatched();
    EXPECT_EQ(flag("ch05.lian_jianfu"), 1);
    expectCompletedRow("q05_jianfu", "林子里练成了剑符");
    expectExpiredRow("q05_fengwu_yishu", "Z1 没做，ch05.done 置位时过期（施工图第 6 节）");
    expectCompletedRow("q05_qingling", "清灵散是第 4 章带来的，潇湘院照原著毒杀");

    // ---- 寒毒：d 过了 65，头一个检查点发作一次，身上没药只有台词（施工图 3.3）----
    if (dayCount() >= kHanduStageTwo) {
        EXPECT_TRUE(spokeAnywhere("ch05.handu.attack"));
        EXPECT_TRUE(spokeAnywhere("ch05.handu.nopill")) << "0 颗养精丹：只是一段台词";
        EXPECT_FALSE(spokeAnywhere("ch05.handu.pill"));
    }

    expectEveryStepTookTheDesignDays();
    expectTheTenMustFightsAllHappened(/*expectConditional=*/false);
    expectTheChapterEndState();
    expectGatedWordsArriveWhereTheDesignSays();

    std::cout << "\n[ch05 第二侧通关] " << (s.day - startDay_) << " 天（d=" << dayCount() << "），"
              << battles_.size() << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterFiveEndingSecond);
}

// ---------------------------------------------------------------------------
// 三选一的第三项（3a 只给腐心丸、7b 什么也不给）：另一条完整通关
// ---------------------------------------------------------------------------
// 前两条走了每一处的第一、第二项；两处三选一的第三项靠这一条覆盖（「各节点的主动分支
// 至少各有一条路线覆盖到」）。顺带它是**主线最快**的那一条：不做支线、不被盯上，
// 施工图 3.3 那张表末格的 59 在这里量（再加林子里练符的两天）。起点第一侧，只备清灵散：
// 没练剑符，⑩ 打不下来（校对 MEDIUM-1），到了亭子不照面、退回林子练成了再来（复验 MEDIUM-B 方案 1）。
TEST_F(Ch05Walkthrough, TheThirdOptionOfBothThreeWayChoicesAlsoReachesTheEnd) {
    Route route = routeFor(0, Prep::QinglingOnly);
    route.name = "第一侧·三选一第三项";
    route.choices[Step::Heishui] = 2;
    route.choices[Step::Jianmian] = 2;
    ASSERT_TRUE(playChapter(route)) << "走不到 ch05.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << battleLine();
    EXPECT_EQ(flag("ch05.shoufu"), 3) << "3a 第三项：只给腐心丸";
    EXPECT_EQ(flag("ch05.jianmianli"), 3) << "7b 第三项：什么也不给";
    EXPECT_FALSE(stepDone(Step::LianJianfu)) << "这一条出发前不练剑符";
    // 没练剑符的那一侧（校对 MEDIUM-1 的拍板）：火弹要不了欧阳飞天的命。揣着第一侧的药也一样——
    // 手里没有要得了命的东西就不照面（复验 MEDIUM-B 方案 1）：退回林子练符（Z2′），再来一招取首级。
    expectTheUnpreparedNightIsOnlyWatched();
    EXPECT_EQ(flag("ch05.lian_jianfu"), 1);
    expectCompletedRow("q05_jianfu", "林子里练成了剑符");
    expectExpiredRow("q05_fengwu_yishu", "Z1 没做");
    expectEveryStepTookTheDesignDays();
    const bool alert = flag("ch05.huayuan") == 2;
    EXPECT_EQ(dayCount(), (alert ? kDayAtTheEndAlert : kDayAtTheEndFastest) + kDaysLianfu)
        << "施工图 3.3：12e 回城时 d = 59（被盯上 62），再加林子里练符的两天（不照面那一夜不拨日子）";
    expectTheTenMustFightsAllHappened(false);
    expectTheChapterEndState();
    std::cout << "\n[ch05 三选一第三项] d=" << dayCount() << "：" << battleLine() << std::endl;
}

// ---------------------------------------------------------------------------
// 没打赢的那几种落点，路照样往下走（施工图 3.1「凡逃出战斗、输了再来的路都在置旗标之前 return，
// 触发器留着」；3.2 的 ①、10b、11b；3.3「10b 逃 ＋1/次」）
// ---------------------------------------------------------------------------
// 这几条落点意图玩家一次也没走到（它打得赢），所以另换一只「一开打就逃」的手，只在指定的那几场用。
// 施工图 3 节开头：「2 处按战斗结局分岔（11b 燕歌胜负、12b 吴剑鸣擒下或走脱）」——12b 如今两条落点都是擒下
//（校对 MEDIUM-2），台词分「按倒」与「堵回」，由第二侧通关走到；11b「输或走」由这一条走到。起点第一侧，清灵散卖掉（潇湘院要开打）。
TEST_F(Ch05Walkthrough, FightsThatAreFledAreComeBackToOrCarriedOnFrom) {
    Route route = routeFor(0, Prep::JianfuOnly);
    route.name = "第一侧·①、潇湘院、⑥各逃一回";
    const auto before = [&](Step step) {
        if (step == Step::DongquLu || step == Step::Xiaoxiang || step == Step::Yange) fleeBudget_ = 1;
    };
    const auto after = [&](Step) { fleeBudget_ = 0; };
    ASSERT_TRUE(playChapter(route, before, after)) << "卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << battleLine();

    // ① 逃了：置 ch05.kaipian 之前 return，挂点留着；回头再踩，那六十天的路不再走（施工偏差 18.3 第 8 条）。
    std::vector<const StepRecord*> dongqu;
    for (const StepRecord& r : steps_) {
        if (r.step == Step::DongquLu) dongqu.push_back(&r);
    }
    ASSERT_EQ(dongqu.size(), 2u) << "① 逃了一回，该回头再踩一回";
    EXPECT_EQ(dongqu[0]->phases, std::vector<BattlePhase>{BattlePhase::Escaped});
    EXPECT_EQ(dongqu[0]->dayAfter - dongqu[0]->dayBefore, kDaysRoad);
    EXPECT_EQ(dongqu[1]->phases, std::vector<BattlePhase>{BattlePhase::Won});
    EXPECT_EQ(dongqu[1]->dayAfter - dongqu[1]->dayBefore, 0) << "回头再来不再走那六十天";
    EXPECT_EQ(flag("ch05.yindu_qi"), startDay_ + kDaysRoad) << "起表只起一回";

    // 10b 逃了：次日再来（＋1），挂点留着；再来打下来 → ch05.xiaoxiang = 2。
    std::vector<const StepRecord*> xiaoxiang;
    for (const StepRecord& r : steps_) {
        if (r.step == Step::Xiaoxiang) xiaoxiang.push_back(&r);
    }
    ASSERT_EQ(xiaoxiang.size(), 2u) << "潇湘院逃了一回，该次日再来";
    EXPECT_EQ(xiaoxiang[0]->phases, std::vector<BattlePhase>{BattlePhase::Escaped});
    EXPECT_EQ(xiaoxiang[0]->dayAfter - xiaoxiang[0]->dayBefore, kDaysXiaoxiangRetreat);
    EXPECT_EQ(xiaoxiang[1]->phases, std::vector<BattlePhase>{BattlePhase::Won});
    EXPECT_EQ(flag("ch05.xiaoxiang"), 2) << "施工图 3.2 10b：赢 → 2";

    // 11b 走了：切磋输了不死（defeat_is_fatal 假），记 2，路往下走；12e 燕歌不在门口送。
    const BattleRecord* yange = lastBattle("b05_yange_qiecuo");
    ASSERT_NE(yange, nullptr);
    EXPECT_EQ(yange->phase, BattlePhase::Escaped);
    EXPECT_EQ(flag("ch05.yange"), 2) << "施工图 3.2 11b：2 输或走";
    EXPECT_TRUE(spokeIn(Step::Huanyu, "ch05.huanyu.yan_none")) << "施工图第 7 节：12e 燕歌送不送看 ch05.yange";
    EXPECT_FALSE(spokeIn(Step::Huanyu, "ch05.huanyu.yan_door"));

    expectEveryStepTookTheDesignDays();
    expectTheTenMustFightsAllHappened(/*expectConditional=*/true);
    expectTheChapterEndState();
}

// ---------------------------------------------------------------------------
// 两侧 × 四种备法（施工图 8.5「路」、验收 15 ②）
// ---------------------------------------------------------------------------
// 每一条都记下实际打到的编成 id：十场必打一场不少；潇湘院只在身上没有清灵散的那几条出现。
// 「只练剑符」「都不备」要身上没有清灵散：第 4 章终局两侧各有 2 份，这里真的去南城药铺卖掉
//（施工偏差 18.4 第 3 行）。每一处选择取该侧的那一项（第一侧取第一项，第二侧取第二项）。
class Ch05EveryPrep : public Ch05Walkthrough,
                      public ::testing::WithParamInterface<std::tuple<int, Prep>> {};

TEST_P(Ch05EveryPrep, TheTenMustFightsAreAllFoughtAndOnlyTheUnpreparedMeetTheBrothel) {
    const int side = std::get<0>(GetParam());
    const Prep prep = std::get<1>(GetParam());
    if (side == 1) startFromChapterFourEnding(fanren::test::kChapterFourEndingSecond);
    ASSERT_FALSE(HasFatalFailure());
    const Route route = routeFor(side, prep);

    bool xiaoxiangHintSeen = false;
    const auto before = [&](Step step) {
        if (step == Step::Xiaoxiang) {
            const auto* obj = fanren::rules::currentObjective(app_.data().objectives, state());
            xiaoxiangHintSeen = obj != nullptr && obj->id == std::string(kHintStepXiaoxiang) &&
                                app_.data().lookupText(obj->textKey).find("清灵散") != std::string::npos;
        }
    };
    const bool reached = playChapter(route, before);
    std::cout << "\n[ch05 备法] " << route.name << "：" << (reached ? "到章末" : "没到章末，卡在「" + stuckAt_ + "」")
              << "，d=" << dayCount() << "；实际打到的编成：";
    for (const std::string& id : battleIds()) std::cout << " " << id;
    std::cout << battleLine() << std::endl;

    EXPECT_TRUE(reached) << route.name << " 走不到 ch05.done，卡在「" << stuckAt_ << "」" << battleLine();
    EXPECT_FALSE(app_.quitRequested()) << route.name << " 走到了 game_over" << battleLine();
    expectTheTenMustFightsAllHappened(/*expectConditional=*/!route.keepQingling);

    // 潇湘院必败分支的两处提示（施工图 8.4）：目标行第 16 步 ＋ 10a 他自己那一句。
    if (!route.keepQingling) {
        EXPECT_TRUE(xiaoxiangHintSeen) << "进潇湘院之前目标行该写着清灵散";
        bool quote = false;
        for (const std::string& t : textsIn(Step::Dingji)) quote = quote || t.find(kHintDingjiQuote) != std::string::npos;
        EXPECT_TRUE(quote) << "施工图 8.4：10a 他自己说「" << kHintDingjiQuote << "」";
    }
    if (!reached) return;
    // 支线的落点（施工图第 6 节）。
    if (route.keepQingling) {
        EXPECT_EQ(flag("ch05.xiaoxiang"), 1);
        expectCompletedRow("q05_qingling", route.name);
    } else {
        EXPECT_EQ(flag("ch05.xiaoxiang"), 2);
        expectExpiredRow("q05_qingling", route.name + "：潇湘院是硬打下来的（Z3 过期）");
    }
    // 不论出发前练没练：能拿着首级回来的都练成了剑符（校对 MEDIUM-1），这一条支线不会过期。
    expectCompletedRow("q05_jianfu", route.name);
    expectExpiredRow("q05_fengwu_yishu", route.name + "：医书没交（Z1 过期）");
    expectTheChapterEndState();
}

INSTANTIATE_TEST_SUITE_P(
    BothSidesFourWays, Ch05EveryPrep,
    ::testing::Combine(::testing::Values(0, 1),
                       ::testing::Values(Prep::Full, Prep::QinglingOnly, Prep::JianfuOnly, Prep::Nothing)),
    [](const ::testing::TestParamInfo<std::tuple<int, Prep>>& info) {
        std::string name = std::get<0>(info.param) == 0 ? "FirstSide_" : "SecondSide_";
        switch (std::get<1>(info.param)) {
            case Prep::Full: name += "Full"; break;
            case Prep::QinglingOnly: name += "QinglingOnly"; break;
            case Prep::JianfuOnly: name += "JianfuOnly"; break;
            case Prep::Nothing: name += "Nothing"; break;
        }
        return name;
    });

// ---------------------------------------------------------------------------
// 十场必打一场都绕不过去（验收 15 ②：「试着绕过去，必须过不去」）
// ---------------------------------------------------------------------------
// 做法：照第一侧的路走到每一场必打之前拍一张快照；从快照出发**跳过那一场**，直接去做下一步。
// 判据只有一条：**下一步的完成旗标不许在没打那一场的情况下被置上**。
// 允许的两种结局：被闸门拦下（下一步的 guard 不认、门不放行、这张图根本出不去），或者走过去的
// 路上被逼着打了那一场（踏入型挂点压在唯一的路上）。两种都记进打印里。
// 数据侧的证明（挂点、guard 链、存档点）在 Ch05AcceptanceTests 里，形状不同。
TEST_F(Ch05Walkthrough, NoneOfTheTenMustFightsCanBeWalkedAround) {
    const Route route = routeFor(0, Prep::Full);
    // 每一场必打 → 它后面那一步（施工图 3.1 的顺序）。
    const std::vector<std::pair<Step, Step>> kPairs = {
        {Step::DongquLu, Step::Shangchuan}, {Step::Heishui, Step::Zhuishao},
        {Step::Zhuishao, Step::Qingbao},    {Step::Jieren, Step::Jiulou},
        {Step::Shuijiao, Step::Jiaoyi},     {Step::Yange, Step::Anpai},
        {Step::Majiu, Step::Zhuwu},         {Step::Zhuwu, Step::Tancha},
        {Step::Tancha, Step::Shangyue},     {Step::Shangyue, Step::Huanyu},
    };
    std::map<Step, GameState> snapshots;
    const auto before = [&](Step step) {
        for (const auto& pair : kPairs) {
            if (pair.first == step) snapshots[step] = state();
        }
    };
    ASSERT_TRUE(playChapter(route, before)) << "先验：这条路本身走得通，卡在「" << stuckAt_ << "」";
    ASSERT_EQ(snapshots.size(), kPairs.size()) << "先验：每一场之前都拍到了快照";

    for (std::size_t i = 0; i < kPairs.size(); ++i) {
        const Step fight = kPairs[i].first;
        const Step next = kPairs[i].second;
        restoreTo(snapshots[fight]);
        ASSERT_FALSE(HasFatalFailure());
        ASSERT_EQ(flag(infoOf(fight).doneFlag), 0) << "先验：快照里 " << infoOf(fight).name << " 还没做";
        tolerateBlocked_ = true;
        beginStep(next);
        attemptStep(next, 0);
        endStep();
        tolerateBlocked_ = false;
        const bool fought = [&] {
            for (const BattleRecord& b : battles_) {
                if (b.id == kMustFight[i]) return true;
            }
            return false;
        }();
        const bool nextDone = flag(infoOf(next).doneFlag) != 0;
        std::cout << "[ch05 绕行] 跳过 " << kMustFightMark[i] << " " << infoOf(fight).name << " 直接去 "
                  << infoOf(next).name << "：" << (fought ? "路上被逼着打了这一场" : (nextDone ? "绕过去了！" : "被拦下"))
                  << std::endl;
        EXPECT_TRUE(!nextDone || fought)
            << kMustFightMark[i] << " " << kMustFight[i] << " 绕得过去：没打它，"
            << infoOf(next).name << " 的完成旗标 " << infoOf(next).doneFlag << " 照样置上了（施工图验收 15）";
    }
}

// ---------------------------------------------------------------------------
// 寒毒倒计时的插桩用例（验收 10：「拨到 d = 66，下一个检查点养精丹少一颗且只少一次；
// 0 颗时照样走得过去」；3.3「超期不 game over」）
// ---------------------------------------------------------------------------
// 插桩：把起表那一天（ch05.yindu_qi）往前拨，让下一个检查点开头看到的 d 落在指定的数上。
// 其余一律照第一侧的路走（第一侧有 12 瓶养精丹，扣没扣看得出来）。
class Ch05ColdPoison : public Ch05Walkthrough {
protected:
    // 走到 stopBefore 之前（不做它），返回快照。
    GameState walkUpTo(const Route& route, Step stopBefore) {
        for (Step s : stepsFor(route)) {
            if (s == stopBefore) break;
            EXPECT_TRUE(runStep(s, route)) << infoOf(s).name << " 没走通" << battleLine();
            if (HasFailure()) break;
        }
        return state();
    }
    void pinDayCount(int d) { state().setFlag("ch05.yindu_qi", state().day - d); }
    // 只看这一步的检查点：做完这一步之前与之后的养精丹差、说了什么。
    int pillsTakenBy(Step s, const Route& route) {
        const int before = state().itemCount(kPill);
        const std::size_t battlesBefore = battles_.size();
        EXPECT_TRUE(runStep(s, route, 1)) << infoOf(s).name << battleLine();
        int eatenInFights = 0;
        for (std::size_t i = battlesBefore; i < battles_.size(); ++i) {
            eatenInFights += battles_[i].pillsBefore - battles_[i].pillsAfter;
        }
        return before - state().itemCount(kPill) - eatenInFights;
    }
};

TEST_F(Ch05ColdPoison, AtDaySixtySixTheNextCheckpointTakesExactlyOnePillAndOnlyOnce) {
    const Route route = routeFor(0, Prep::Full);
    walkUpTo(route, Step::Dingji);
    ASSERT_FALSE(HasFailure());
    ASSERT_GT(state().itemCount(kPill), 3) << "先验：第一侧身上有药，扣没扣看得出来";
    pinDayCount(66);
    EXPECT_EQ(pillsTakenBy(Step::Dingji, route), 1) << "d = 66：进第二段后的第一个检查点发作一次，扣一颗";
    EXPECT_TRUE(spokeIn(Step::Dingji, "ch05.handu.attack"));
    EXPECT_TRUE(spokeIn(Step::Dingji, "ch05.handu.pill"));
    EXPECT_EQ(flag("ch05.handu_fa"), 2);
    // 第二段只发作一次：下一个检查点（11c）d 仍在 65-79，不再扣。
    for (Step s : {Step::Xiaoxiang, Step::Shuijiao, Step::Jiaoyi, Step::Yange, Step::LianJianfu}) {
        ASSERT_TRUE(runStep(s, route)) << infoOf(s).name;
    }
    ASSERT_LT(dayCount(), kHanduStageThree) << "先验：11c 时 d 仍在第二段";
    EXPECT_EQ(pillsTakenBy(Step::Anpai, route), 0) << "第二段只发作一次（施工图 3.3）";
    EXPECT_FALSE(spokeIn(Step::Anpai, "ch05.handu.attack"));
}

TEST_F(Ch05ColdPoison, InTheThirdStageEveryCheckpointTakesOneAndPastTheDeadlineNothingEndsTheGame) {
    const Route route = routeFor(0, Prep::Full);
    walkUpTo(route, Step::Zhuwu);
    ASSERT_FALSE(HasFailure());
    ASSERT_GE(state().itemCount(kPill), 3) << "先验：身上的药够扣三回";
    pinDayCount(kHanduStageThree);
    EXPECT_EQ(pillsTakenBy(Step::Zhuwu, route), 1) << "第三段：12b 扣一颗";
    EXPECT_EQ(flag("ch05.handu_fa"), 3);
    EXPECT_FALSE(spokeIn(Step::Zhuwu, "ch05.handu.over")) << "d = 80 还没过期限";
    EXPECT_EQ(pillsTakenBy(Step::Tancha, route), 1) << "第三段：12c 再扣一颗（每个检查点各一次）";
    ASSERT_TRUE(runStep(Step::Shangyue, route));
    // 过期限：d ≥ 90 照第三段办，外加一句，**不设任何失败出口**（用户拍板）。
    pinDayCount(kHanduDeadline + 5);
    EXPECT_EQ(pillsTakenBy(Step::Huanyu, route), 1) << "12e 照第三段扣一颗";
    EXPECT_TRUE(spokeIn(Step::Huanyu, "ch05.handu.over")) << "超期多一句「不知道还能撑几天」";
    EXPECT_FALSE(app_.quitRequested()) << "超期不许 game over（施工图 3.3、用户拍板）";
    EXPECT_EQ(flag("ch05.done"), 1) << "超期照样走到章末";
}

TEST_F(Ch05ColdPoison, WithNoPillTheAttackIsOnlyALineAndTheCheckpointStillGoesOn) {
    startFromChapterFourEnding(fanren::test::kChapterFourEndingSecond);
    ASSERT_FALSE(HasFatalFailure());
    const Route route = routeFor(1, Prep::Nothing);
    walkUpTo(route, Step::Dingji);
    ASSERT_FALSE(HasFailure());
    ASSERT_EQ(state().itemCount(kPill), 0) << "先验：第二侧 0 瓶";
    pinDayCount(kHanduDeadline);
    ASSERT_TRUE(runStep(Step::Dingji, route, 1));
    EXPECT_TRUE(spokeIn(Step::Dingji, "ch05.handu.attack"));
    EXPECT_TRUE(spokeIn(Step::Dingji, "ch05.handu.nopill")) << "0 颗：只是一段台词";
    EXPECT_TRUE(spokeIn(Step::Dingji, "ch05.handu.over"));
    EXPECT_EQ(flag("ch05.dingji"), 1) << "发作之后这一节照样演完";
    EXPECT_FALSE(app_.quitRequested());
}

// 内视：只说不扣，按 d 说哪一段（施工图 3.3 三段的界；宝玉到手即停表）。
TEST_F(Ch05ColdPoison, LookingInwardNamesTheStageAndNeverTakesAnything) {
    const Route route = routeFor(0, Prep::Full);
    walkUpTo(route, Step::Qingbao);
    ASSERT_FALSE(HasFailure());
    ASSERT_TRUE(travelTo(kMapKezhan));
    struct Probe {
        int d;
        const char* stageKey;
        bool over;
    };
    for (const Probe& probe : {Probe{kHanduStageTwo - 1, "ch05.neishi.d1", false},
                               Probe{kHanduStageTwo, "ch05.neishi.d2", false},
                               Probe{kHanduStageThree - 1, "ch05.neishi.d2", false},
                               Probe{kHanduStageThree, "ch05.neishi.d3", false},
                               Probe{kHanduDeadline, "ch05.neishi.d3", true}}) {
        pinDayCount(probe.d);
        const GameState before = state();
        ASSERT_TRUE(fireTrigger("trigger_neishi"));
        EXPECT_EQ(app_.spokenKeys().empty() ? std::string{} : app_.spokenKeys().front(),
                  std::string(probe.stageKey)) << "d = " << probe.d;
        EXPECT_EQ(std::find(app_.spokenKeys().begin(), app_.spokenKeys().end(), "ch05.neishi.over") !=
                      app_.spokenKeys().end(), probe.over) << "d = " << probe.d;
        EXPECT_EQ(state().itemCount(kPill), before.itemCount(kPill)) << "内视只说不扣";
        EXPECT_EQ(state().flags, before.flags) << "内视不写旗标（施工图 3.1：不写 set_flag）";
        EXPECT_EQ(state().day, before.day);
    }
}

// ---------------------------------------------------------------------------
// 意图玩家的扫描（施工图 8.5、复验二第八节第 1 条）
// ---------------------------------------------------------------------------
// 维度：手 × 起点两侧 × 路（两种）× 药数。
//   · 手：意图 40 / 50 / 60、先吃金疮药 50、喂同伴 50（意图 50 另跑 P0−2、P0＋2 两档药数）；
//   · 起点：ch04-end-first.sav（P0 = 12）、ch04-end-second.sav（P0 = 0）；
//   · 路：「照常」——第一侧取第一项、全备；第二侧取第二项、什么都不备（清灵散是第 4 章带来的，
//     与两条通关用例同一条路）。「都不备（卖掉）」——同一侧的选择，清灵散在南城药铺卖掉、不练剑符：
//     潇湘院要硬打、欧阳飞天没练剑符打不下来（先上亭子看一眼不照面、在林子里练成了再来），药的那一维只有在这条路上才量得到东西。
// 每一格一条用例（每条用例一个新的 Application：game_over 置下的退出请求不会漏到下一格）。
// 「最少要几瓶」另一组：从意图 50 那一趟每一场必打之前的快照出发，逐一试 0..6 瓶。
//
// ---- 这张表量得到什么、量不到什么（第 4 章 R-11 的教训，别把每一维都当成在把关）----
// **按构造翻不了的几维**：
//   · 第二侧的「P0−2」：第二侧 P0 = 0，减两瓶还是 0——与 P0 那一格逐字相同，它不量任何东西；
//   · ⑤ 夺帮：韩立不在场、不登记背包（施工图 10.2），五只手、所有药数下它必然逐字相同；
//   · 「喂同伴」在 10a 之后：曲魂 10a 就借给了孙二狗，此后无同伴可喂，从 ⑥ 起与意图 50 逐字相同；
//   · 「照常」那条路上第一侧的 ⑩（练成了剑符）：一招取首级、不吃药，手与药数都碰不到它；
//     「照常」那条路上的潇湘院：身上有清灵散，不开打；
//   · 战斗的 RNG 按编成 id 定种子：「逃得掉」是这一个种子上的一次抽样，不量逃跑的概率。
// **按余量翻不了、实测也没翻的**（不是构造保证，是这一批内容上量出来的）：
//   · 第一侧的药数那一维：意图 50 整章一瓶不吃，P0−2 / P0 / P0＋2 三格逐字相同——
//     这一维在第一侧**什么也没量出来**，只说明第一侧的药远远用不完。
// 真正在量东西的是：门槛那一维、「先吃金疮药」那一只（它有药可吃就不逃）、「都不备（卖掉）」那条路、
// 与「最少要几瓶」表。
struct SweepHand {
    const char* name;
    int healAtPercent;
    bool salveFirst;
    bool feedCompanion;
    int pillDelta;
};

constexpr SweepHand kSweepHands[] = {
    {"意图40", 40, false, false, 0},
    {"意图50", 50, false, false, 0},
    {"意图60", 60, false, false, 0},
    {"意图50·P0−2", 50, false, false, -2},
    {"意图50·P0＋2", 50, false, false, 2},
    {"先吃金疮药50", 50, true, false, 0},
    {"喂同伴50", 50, false, true, 0},
};
constexpr int kNeedMaxPills = 6;
// 用例名里的手（gtest 的名字只许字母数字下划线），与 kSweepHands 一一对应。
constexpr const char* kSweepHandIds[] = {"Intent40",        "Intent50",     "Intent60",       "Intent50MinusTwo",
                                         "Intent50PlusTwo", "SalveFirst50", "FeedCompanion50"};
static_assert(std::size(kSweepHandIds) == std::size(kSweepHands));

// 路：0 = 照常，1 = 都不备（卖掉清灵散、不练剑符）。
Route sweepRoute(int side, int way) {
    Route r = routeFor(side, Prep::Nothing);
    if (way == 0) {
        r.name = side == 0 ? "第一侧·照常（全备）" : "第二侧·照常（什么都不备）";
        r.keepQingling = true;   // 两侧照常都不卖：第 4 章带来的两份原样留着
        r.trainJianfu = side == 0;
    } else {
        r.name = side == 0 ? "第一侧·都不备（卖掉清灵散）" : "第二侧·都不备（卖掉清灵散）";
    }
    return r;
}

const char* chapterFourEndingOf(int side) {
    return side == 0 ? fanren::test::kChapterFourEndingFirst : fanren::test::kChapterFourEndingSecond;
}

class Ch05HandSweep : public Ch05Walkthrough,
                      public ::testing::WithParamInterface<std::tuple<int, int, int>> {};

TEST_P(Ch05HandSweep, TheIntentPlayerNeverDiesWithinTwoPillsOfTheHandOver) {
    const int side = std::get<0>(GetParam());
    const int way = std::get<1>(GetParam());
    const SweepHand& h = kSweepHands[std::get<2>(GetParam())];
    startFromChapterFourEnding(chapterFourEndingOf(side));
    ASSERT_FALSE(HasFatalFailure());
    const Route route = sweepRoute(side, way);
    const int pills = std::max(0, startPills_ + h.pillDelta);
    setPills(pills);   // 扫描的量尺（施工图 8.5 把药数列为一维），通关用例里没有这一行
    hand_ = HandPolicy{h.healAtPercent, h.salveFirst, h.feedCompanion};
    const bool reached = playChapter(route);

    std::ostringstream row;
    row << "[ch05 扫描] " << route.name << " · " << h.name << "（起步 " << pills << " 瓶，P0 = "
        << startPills_ << "）：";
    for (std::size_t i = 0; i < std::size(kMustFight); ++i) {
        std::string cell = "—";
        int used = 0;
        for (const BattleRecord& b : battles_) {
            if (b.id != kMustFight[i]) continue;
            cell = cell == "—" ? std::string(phaseName(b.phase)) : cell + "/" + phaseName(b.phase);
            used += b.pillsBefore - b.pillsAfter;
        }
        row << " " << kMustFightMark[i] << cell << used;
    }
    if (const BattleRecord* x = lastBattle(kConditionalFight)) {
        row << " 潇湘院" << phaseName(x->phase) << (x->pillsBefore - x->pillsAfter);
    }
    row << " → " << (reached ? "到章末" : (app_.quitRequested() ? "game_over" : "卡在「" + stuckAt_ + "」"))
        << "，剩养精丹 " << state().itemCount(kPill) << "、金疮药 " << state().itemCount(kSalve);
    std::cout << row.str() << std::endl;

    // 判据（施工图 8.5）：意图玩家那一档（门槛 50、按 id 吃养精丹）在 P0−2 之内，
    // 十场必打一格不输（⑧⑩ 允许走「逃」，但不许 game_over）。其余几只手只打印，不判。
    const bool intent = h.healAtPercent == kIntentHealPercent && !h.salveFirst && !h.feedCompanion;
    if (!intent) return;
    EXPECT_FALSE(app_.quitRequested()) << h.name << " 走到了 game_over：" << battleLine();
    EXPECT_TRUE(reached) << h.name << " 没到章末，卡在「" << stuckAt_ << "」" << battleLine();
    for (const BattleRecord& b : battles_) {
        if (b.id == "b05_duobang") continue;   // 输了第二夜再来（施工图 3.2 10c，defeat_is_fatal 假）
        if (b.id == "b05_wu_jianming") continue;   // 输了也被擒（校对 MEDIUM-2，defeat_is_fatal 假）
        EXPECT_NE(b.phase, BattlePhase::Lost) << h.name << " 在 " << b.id << " 输了" << battleLine();
        if (winsWithNoPillPerDesign(b.id)) {
            EXPECT_EQ(b.phase, BattlePhase::Won)
                << h.name << "：" << b.id << " 施工图 8.4 写「0 瓶也赢」，这里却" << phaseName(b.phase);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    HandsSidesWays, Ch05HandSweep,
    ::testing::Combine(::testing::Values(0, 1), ::testing::Values(0, 1),
                       ::testing::Range(0, static_cast<int>(std::size(kSweepHands)))),
    [](const ::testing::TestParamInfo<std::tuple<int, int, int>>& info) {
        return std::string(std::get<0>(info.param) == 0 ? "FirstSide_" : "SecondSide_") +
               (std::get<1>(info.param) == 0 ? "AsUsual_" : "NothingSold_") +
               kSweepHandIds[std::get<2>(info.param)];
    });

// 「最少要几瓶」：意图 50 照一条路走一趟，拍下每一场之前的世界；每一场、每只手，逐一试 0..6 瓶。
class Ch05LeastPills : public Ch05Walkthrough,
                       public ::testing::WithParamInterface<std::tuple<int, int>> {};

TEST_P(Ch05LeastPills, TheFightsTheDesignCallsSafeAreWonWithNoPillAtAll) {
    const int side = std::get<0>(GetParam());
    const int way = std::get<1>(GetParam());
    startFromChapterFourEnding(chapterFourEndingOf(side));
    ASSERT_FALSE(HasFatalFailure());
    const Route route = sweepRoute(side, way);
    const std::vector<std::pair<Step, const char*>> fights = {
        {Step::DongquLu, "①"}, {Step::Heishui, "②"}, {Step::Zhuishao, "③"}, {Step::Jieren, "④"},
        {Step::Xiaoxiang, "潇湘院"}, {Step::Yange, "⑥"}, {Step::Majiu, "⑦"}, {Step::Zhuwu, "⑧"},
        {Step::Tancha, "⑨"}, {Step::Shangyue, "⑩"}};
    std::map<Step, GameState> snapshots;
    const auto before = [&](Step s) {
        for (const auto& fight : fights) {
            if (fight.first == s && !snapshots.count(s)) snapshots[s] = state();
        }
    };
    ASSERT_TRUE(playChapter(route, before)) << "先验：意图 50 走得通这条路，卡在「" << stuckAt_ << "」"
                                            << battleLine();
    ASSERT_EQ(snapshots.size(), fights.size()) << "先验：每一场之前都拍到了快照";

    std::cout << "[ch05 最少要几瓶] " << route.name
              << "（每一场之前的快照；「不开打」＝那一节没有仗；「>6」＝揣 6 瓶也赢不下来）" << std::endl;
    for (const SweepHand& h : kSweepHands) {
        if (h.pillDelta != 0) continue;   // 药数就是这张表的横轴
        std::ostringstream row;
        row << "  " << h.name << "：";
        for (const auto& fight : fights) {
            int least = -1;
            bool anyFight = false;
            for (int pills = 0; pills <= kNeedMaxPills && least < 0; ++pills) {
                restoreTo(snapshots[fight.first]);
                ASSERT_FALSE(HasFatalFailure());
                setPills(pills);   // 量尺
                hand_ = HandPolicy{h.healAtPercent, h.salveFirst, h.feedCompanion};
                runStep(fight.first, route, 1);
                bool won = false;
                for (const BattleRecord& b : battles_) {
                    anyFight = true;
                    won = won || b.phase == BattlePhase::Won;
                }
                if (!anyFight) break;
                if (won) least = pills;
            }
            row << " " << fight.second
                << (!anyFight ? std::string("不开打") : (least < 0 ? std::string(">6") : std::to_string(least)));
        }
        std::cout << row.str() << std::endl;
    }

    // 判据（施工图 8.4「身上 0 瓶养精丹时」一列与 R-3 表末两行）：①②③④⑥⑦⑨ 0 瓶也赢。
    hand_ = HandPolicy{};
    for (const auto& fight : fights) {
        restoreTo(snapshots[fight.first]);
        ASSERT_FALSE(HasFatalFailure());
        setPills(0);
        runStep(fight.first, route, 1);
        for (const BattleRecord& b : battles_) {
            if (!winsWithNoPillPerDesign(b.id)) continue;
            EXPECT_EQ(b.phase, BattlePhase::Won)
                << route.name << "：" << b.id << " 身上 0 瓶，施工图 8.4 写「赢」，实测" << phaseName(b.phase)
                << "（开打前韩立 " << b.hpBefore << "/" << b.maxHp << "，收场 " << b.hpAfter << "）";
        }
    }
}

INSTANTIATE_TEST_SUITE_P(SidesWays, Ch05LeastPills,
                         ::testing::Combine(::testing::Values(0, 1), ::testing::Values(0, 1)),
                         [](const ::testing::TestParamInfo<std::tuple<int, int>>& info) {
                             return std::string(std::get<0>(info.param) == 0 ? "FirstSide_" : "SecondSide_") +
                                    (std::get<1>(info.param) == 0 ? "AsUsual" : "NothingSold");
                         });

}  // namespace
