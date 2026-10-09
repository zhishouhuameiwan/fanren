// 第 6 章的自动化通关测试与几条整章验收（docs/ch06-design.md 第 15 节验收第 2、6、11、15、18 条），
// 外加「那只手先证明自己稳」（同一存档跑三遍）与「先吃哪一瓶、门槛多低」的扫描，以及交给第 7 章的终局存档。
//
// 驱动的是玩家真会碰的那一整套：真的 Application（无头）、真的 maps/ch06_*.tmj 与嘉元城南城、
// 真的 WorldScene 走位规则、真的 scripts/ch06/*.lua、真的 BattleScene、ShopScene。
//
// ---------------------------------------------------------------------------
// 判据从哪来（handoff 第 6 节第 5 条、handoff-2026-09-23-ch04 第 8 节）
// ---------------------------------------------------------------------------
// **判据只从施工图推，不从被测的脚本和数据里抄。** 下面每一个 kXxx 常量、每一张表旁边都写着它抄自
// 施工图哪一节。脚本里的文案 key 只拿来**认出**「演的是哪一条」（与第 4、5 章同一个办法），
// 不拿来决定「该演哪一条」。认药三题与竹简那两处要知道「哪一项是对的」：选项的 key 从脚本的 choice{}
// 读，哪一项算对只看施工图原文（18.4 第 1 条的三味草名、3.2 节点 17 的「只有百药园走得下去」）。
// 施工图第 18 节的施工偏差，本文件认了这几条（改判据的地方旁边写着「施工偏差 18.x」）：
//   · 18.2 第 5 条：支线 Z2 的了结看 ch06.zhang_done（出谷前数一遍），不看「灵石 ≥ 10 且未出谷」；
//   · 18.3 第 1 条：5b、8b 挂在 NPC 上，由脚本自己判旗标——这里照样按 3.1 的次序去按那两个人；
//   · 18.3 第 2 条：13 不再 teleport，12 的传送落在迎宾楼的厅里、13 挂在房门上；
//   · 18.4 第 1 条：认药三题出子夜花、白鹤芝、望月草，选错（取消同）同一题重来；
//   · 18.4 第 3 条：1a 黄精、紫参不论年份全部用掉，土骨花不动；
//   · 18.4 第 5 条：17a 取消就卷起竹简、不置旗标（本文件不走取消，那一条在 Ch06SliceTests）。
//
// ---------------------------------------------------------------------------
// 起点不许自己挑（handoff-2026-09-23-ch04 第 8 节第二条、施工图第 2 节）
// ---------------------------------------------------------------------------
// 起点**直接读** tests/fixtures/ch05-end-first.sav / ch05-end-second.sav，由第 5 章两条通关用例写出、
// 逐字段看着。本文件在起点上**一个字段也不改**：读进来、载入存档里那张图（墨府）、把人放回存档里那一格
//（loadMap 会把人挪到出生点，放回去不是改字段，是撤销这一挪）。SetUp 先验施工图第 2 节标「硬」的每一格。
//
// ---------------------------------------------------------------------------
// 替玩家出手的那只手：意图玩家（施工图 8.1 / 8.2）
// ---------------------------------------------------------------------------
// 第 3、4、5 章共用的那只（tests/BattleHand.h）。取舍沿用第 5 章的意图玩家（血掉到一半才吃药、不算对面
// 下一轮打不打得死自己、没药能逃就逃——本章三场必打都不许逃，这一条用不上），只改一处：**吃药的名单是
// 养精丹在前、金疮药在后**。施工图 8.2 ② 原话「养精丹只有一瓶——打得赢，但要吃金疮药」：意图玩家会吃金疮药。
// 这只手自己稳不稳：TheHandIsSteadyThreeRunsFromTheSameSaveEndAlike（同一存档整章跑三遍）；
// 对取舍敏不敏感：TheSweepOfWhichPillFirstAndHowLowToWait（先吃金疮药、门槛四成 / 六成、第 4 章那只）。
//
// ---------------------------------------------------------------------------
// 一条走位规则，与第 3、4、5 章同源（WorldScene::interact 看面朝的前一格）
// ---------------------------------------------------------------------------
// 要站在 S 上脸朝着相邻的 F：F 进不去时站到 S 朝 F 按一下方向键；F 进得去时从 F 的另一侧那一格朝 F 迈一步
// 落到 S。路上不许顺手把剧情演了：standable() 避开此刻还备着的踏入型触发（WorldScene::triggerReady）。
//
// ---------------------------------------------------------------------------
// 选择怎么答
// ---------------------------------------------------------------------------
// 脚本的 talk 与 choice 都压一个对话框、都等一条回填；分得出是哪一种，靠的是 talk 会往 spokenKeys 里记一条
// key 而 choice 不记（Application::sayAs / dispatch）。每回填一条就把 spokenKeys 读走并清空，于是「这一回
// 等着的命令之前有没有新说一句」就是「这是 talk 还是 choice」。
//
// ---------------------------------------------------------------------------
// 这个文件看不见什么
// ---------------------------------------------------------------------------
//   · 画面（对话框的样子、破绽图标）：只读 spokenKeys 与状态；
//   · 手按方向键的节奏、长按快进；
//   · 战斗的 RNG：BattleScene 的种子由编成 id 派生，每一场都是**确定的**——「打得赢」只是这一个种子上的
//     一次抽样，不是概率上的保证。
#include <gtest/gtest.h>
#include "TextKeys.h"

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/BoardScene.h"
#include "game/CultivationScene.h"
#include "game/Scene.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "game/WorldScene.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::core::MapObject;
using fanren::core::Point;
using fanren::core::TileMap;
using fanren::core::battle::BattlePhase;
using fanren::core::battle::Unit;
using fanren::game::Application;
using fanren::game::BattleScene;
using fanren::game::BoardScene;
using fanren::game::ShopScene;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// ---- 本章踏过的八张图（施工图第 4 节：新建 6 张 ＋ 南城东门修补；起点在墨府）----
constexpr const char* kMapMofu = "ch05_mofu";
constexpr const char* kMapNancheng = "ch05_nancheng";
constexpr const char* kMapCun = "ch06_tainan_cun";
constexpr const char* kMapGu = "ch06_tainan_gu";
constexpr const char* kMapLou = "ch06_sanxiu_lou";
constexpr const char* kMapShanqiu = "ch06_shanqiu";
constexpr const char* kMapHfg = "ch06_huangfenggu";
constexpr const char* kMapByy = "ch06_baiyaoyuan";
constexpr const char* kChapterMaps[] = {kMapCun, kMapGu, kMapLou, kMapShanqiu, kMapHfg, kMapByy};

// ---- 本章动到的东西（施工图第 2 节、E9、第 9 节）----
constexpr const char* kPill = "pill_yangjing_dan";
constexpr const char* kSalve = "pill_jinchuang_yao";
constexpr const char* kShixin = "pill_shixin_san";     // 那只手许撒的毒（第二侧身上有两包）
constexpr const char* kQingdu = "pill_qingdu_san";     // 两侧各 3 瓶，坊市卖它攒支线 Z2 的钱
constexpr const char* kLingshi = "material_lingshi";
constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZishen = "herb_zishen_cao";
constexpr const char* kTugu = "herb_tugu_hua";
constexpr const char* kPlaque = "material_heipai";
constexpr const char* kJianfuItem = "talisman_jianfu";
constexpr const char* kBaoyu = "story_nuanyang_baoyu";
constexpr const char* kSoftSword = "weapon_yudai_duanjian";
constexpr const char* kJinfaFu = "story_jinfa_fu";
constexpr const char* kFeixingFu = "talisman_feixing_fu";
constexpr const char* kFuzhi = "material_fuzhi";
constexpr const char* kDansha = "material_dansha";
constexpr const char* kJinzhuBi = "story_jinzhu_bi";
constexpr const char* kSeed = "material_qixingcao_zhongzi";
constexpr const char* kCanpian = "story_fabao_canpian";
constexpr const char* kBilu = "story_qingxi_bilu";
constexpr const char* kShengxianling = "story_shengxianling";
constexpr const char* kQingye = "story_qingye_faqi";
constexpr const char* kLieyang = "weapon_lieyang_jian";
constexpr const char* kLengyue = "weapon_lengyue_dao";
constexpr const char* kChuwudai = "story_chuwudai";
constexpr const char* kYupai = "story_yaoyuan_yupai";
constexpr const char* kMupai = "story_mupai";
constexpr const char* kDingshenFu = "talisman_dingshen_fu";
constexpr const char* kHuoqiuFu = "talisman_huoqiu_fu";
constexpr const char* kHushenFu = "talisman_hushen_fu";
constexpr const char* kJinciFu = "talisman_jinci_fu";
constexpr const char* kFireMagic = "magic_huodan_shu";
constexpr const char* kWindMagic = "magic_yufeng_jue";
constexpr const char* kTianyan = "magic_tianyan_shu";
constexpr const char* kLiusha = "magic_liusha_shu";
constexpr const char* kBingdong = "magic_bingdong_shu";
constexpr const char* kJijianfu = "magic_ji_jianfu";
constexpr const char* kField = "field_baiyaoyuan";

// ---- 施工图 3.2 节点 1 / 第 9 节 / 验收 11 ----
constexpr int kPillsAfterChudu = 15;      // 「补足到 15 瓶」
// 以物易物（第 9 节第 2 条的字面量）：5b 五瓶、8b 两瓶加七瓶。
constexpr int kBarterFeixing = 5;
constexpr int kBarterBook = 2;
constexpr int kBarterBrush = 7;
constexpr int kFuzhiFromCaomao = 12;      // 5b「一打符纸」
constexpr int kDanshaFromStall = 6;       // 8b give("material_dansha", 6)
constexpr int kFuzhiWasted = 12;          // 9a 十二张全废
constexpr int kDanshaWasted = 3;          // 9a take 丹砂 3
constexpr int kSpoilsLingshi = 50;        // 12 搜身「五十块低阶灵石」
constexpr int kSpoilsHuoqiu = 2;          // 12 火球符 2
constexpr int kSpoilsHushen = 1;          // 12 护身符 1
constexpr int kFieldSlots = 6;            // 18b field.unlock("field_baiyaoyuan", 6)
// 18b 叶师叔送物（施工图 3.2 节点 18b 原文）：huinuo == 1 → 灵石 60、火球符 2、护身符 1、金刺符 1；
// huinuo == 2 → 灵石 30、火球符 1；rangdan == 2 → 在前两者基础上灵石各减 10。
struct Delivery {
    int lingshi, huoqiu, hushen, jinci;
};
Delivery designDelivery(int huinuo, int rangdan) {
    Delivery d = huinuo == 2 ? Delivery{30, 1, 0, 0} : Delivery{60, 2, 1, 1};
    if (rangdan == 2) d.lingshi -= 10;
    return d;
}

// ---- 施工图 1.3：九层气血 24 + 12 × 9 = 132、法力 90 ----
constexpr int kNinthLayerHp = 132;
constexpr int kNinthLayerMp = 90;

// ---- 施工图 8.0 / 8.2：三场必打（按发生顺序）----
constexpr const char* kMustFight[] = {"b06_yejia_xunxin", "b06_shanqiu_xisha", "b06_wufeng_qiecuo"};
constexpr const char* kMustFightMark[] = {"①", "②", "③"};
// 编成认人时的候选：三场必打 ＋ 两场可选切磋（施工图 8.2 那张表的五行）。
constexpr const char* kChapterBattles[] = {"b06_yejia_xunxin", "b06_shanqiu_xisha", "b06_wufeng_qiecuo",
                                           "b06_qiecuo_wujiuzhi", "b06_huangfenggu_qiecuo"};

// ---- 施工图 3.4 与验收 17 的 R-3 提示 ----
constexpr const char* kHintStepFeixing = "n07_feixingfu";
constexpr const char* kHintStepJinzhubi = "n11_jinzhubi";
constexpr const char* kHintStepSanhui = "n16_sanhui";

// ---- 施工图 18.4 第 1 条：认药三题的正确答案（按出题次序）；3.2 节点 17：竹简上只有百药园走得下去 ----
constexpr const char* kQuizHerbs[] = {"子夜花", "白鹤芝", "望月草"};
constexpr const char* kSlipThatWorks = "百药园";

// ---- 施工图 E3 / 10.1：修炼面板的灵根一行 ----
constexpr const char* kSpiritRootText = "四属性缺金·伪灵根";

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr double kFrame = 1.0 / 60.0;
constexpr int kMaxScriptFrames = 8000;
constexpr int kMaxAttempts = 3;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch06" / "maiping.lua") &&
            fs::exists(root / "maps" / "ch06_tainan_gu.tmj")) {
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

std::vector<std::string> linesOf(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        out.push_back(line);
    }
    return out;
}

// 一行去掉 -- 行注释之后的正文。本章脚本只有行注释；字符串里没有 --。
std::string codeOf(const std::string& line) {
    const std::size_t dash = line.find("--");
    return dash == std::string::npos ? line : line.substr(0, dash);
}

std::string codeOnly(const std::string& text) {
    std::string out;
    for (const std::string& line : linesOf(text)) out += codeOf(line) + "\n";
    return out;
}

int countOf(const std::string& haystack, const std::string& needle) {
    int n = 0;
    for (std::size_t at = haystack.find(needle); at != std::string::npos; at = haystack.find(needle, at + 1)) ++n;
    return n;
}

// 一个脚本里每一处 choice{ … } 的选项 key（按出现次序）。
std::vector<std::vector<std::string>> choiceBlocks(const std::string& source) {
    std::vector<std::vector<std::string>> out;
    const std::string code = codeOnly(source);
    static const std::regex kKey("\"([a-z0-9_.]+)\"");
    for (std::size_t at = code.find("choice{"); at != std::string::npos; at = code.find("choice{", at + 1)) {
        const std::size_t close = code.find('}', at);
        if (close == std::string::npos) break;
        const std::string body = code.substr(at, close - at);
        std::vector<std::string> keys;
        for (auto it = std::sregex_iterator(body.begin(), body.end(), kKey); it != std::sregex_iterator(); ++it) {
            keys.push_back((*it)[1]);
        }
        out.push_back(keys);
    }
    return out;
}

int facingOf(Point direction) {
    if (direction.x > 0) return 1;
    if (direction.x < 0) return 3;
    if (direction.y < 0) return 0;
    return 2;
}

std::map<std::string, int> bagCounts(const GameState& state) {
    std::map<std::string, int> out;
    for (const auto& entry : state.bag) {
        if (entry.count != 0) out[entry.itemId] += entry.count;
    }
    return out;
}

std::map<std::string, int> diffOf(const std::map<std::string, int>& before, const std::map<std::string, int>& after) {
    std::map<std::string, int> out;
    for (const auto& [id, n] : after) {
        const auto it = before.find(id);
        const int was = it == before.end() ? 0 : it->second;
        if (n != was) out[id] = n - was;
    }
    for (const auto& [id, n] : before) {
        if (!after.count(id) && n != 0) out[id] = -n;
    }
    return out;
}

std::string showBag(const std::map<std::string, int>& bag) {
    std::ostringstream out;
    out << "{";
    bool first = true;
    for (const auto& [id, n] : bag) {
        out << (first ? "" : ", ") << id << (n > 0 ? " +" : " ") << n;
        first = false;
    }
    out << "}";
    return out.str();
}

// ---------------------------------------------------------------------------
// 七张图之间的门（不写任何 require_flag：闸开没开由引擎说了算）
// ---------------------------------------------------------------------------
// 山脚 → 谷、谷 → 荒丘、荒丘 → 黄枫谷都是脚本 teleport，不在这张表里（施工图第 4 节「连通」）。
struct MapLink {
    const char* from;
    const char* to;
    const char* portal;
};

constexpr MapLink kMapLinks[] = {
    {kMapMofu, kMapNancheng, "portal_to_nancheng"},
    {kMapNancheng, kMapCun, "portal_to_tainan_cun"},
    {kMapGu, kMapLou, "portal_to_sanxiu_lou"},
    {kMapLou, kMapGu, "portal_to_tainan_gu"},
    {kMapHfg, kMapByy, "portal_to_baiyaoyuan"},
    {kMapByy, kMapHfg, "portal_to_huangfenggu"},
};

// ---------------------------------------------------------------------------
// 一章的步骤（施工图 3.1 表的挂点，按 3.4 目标链的次序）＋ 坊市买卖（玩家自己的动作）
// ---------------------------------------------------------------------------
enum class Step {
    Chudu, Cunkou, Guaipo, Qingyan, Ruhuo, Lingshi, Feixingfu, Xunxin, Yishi, Shuangshou, Jinzhubi,
    Zhifu, Kuxiu, Canpian, Bilu, Fangshi, Sanhui, Xisha, Linggen, Maidan, Dadian, Lingqu, Wufeng, Zawu,
    Juanzong, Jinzhi, Maiping,
};

struct StepInfo {
    Step step;
    const char* node;       // 施工图 3.1 表的 # 列
    const char* name;
    const char* doneFlag;   // 3.1 表的 set_flag 列（坊市为空）
    const char* mapId;
    const char* object;
    bool enter;             // 3.1 表 mode 列：enter；否则 interact（5b、8b 是 NPC，施工偏差 18.3 第 1 条）
    int days;               // 3.3 日历：这一步该走掉的日子
};

const std::vector<StepInfo>& stepTable() {
    static const std::vector<StepInfo> kTable = {
        {Step::Chudu, "1a", "除毒", "ch06.chudu", kMapCun, "trigger_chudu", true, 15},
        {Step::Cunkou, "1b", "村口", "ch06.wan_met", kMapCun, "trigger_cunkou", true, 0},
        {Step::Guaipo, "2", "怪坡", "ch06.rugu", kMapCun, "trigger_guaipo", false, 0},
        {Step::Qingyan, "3", "青颜真人", "ch06.qingyan", kMapGu, "trigger_qingyan", true, 0},
        {Step::Ruhuo, "4", "入伙", "ch06.ruhuo", kMapGu, "trigger_ruhuo", true, 0},
        {Step::Lingshi, "5a", "灵石与灵符", "ch06.lingshi", kMapGu, "trigger_lingshi", true, 0},
        {Step::Feixingfu, "5b", "飞行符", "ch06.feixingfu", kMapGu, "npc_caomao_qingnian", false, 0},
        // 校对整改 16.4（LOW-8）：① 开头拨 1 天——「隔了一夜」是换完飞行符的次日一早，3.3 补了这一行。
        {Step::Xunxin, "6", "叶家寻衅", "ch06.xunxin", kMapGu, "trigger_ye_xunxin", true, 1},
        {Step::Yishi, "7", "小楼议事", "ch06.yishi", kMapLou, "trigger_yishi", false, 1},
        {Step::Shuangshou, "8a", "双首鹜", "ch06.shuangshou", kMapGu, "trigger_shuangshou", true, 0},
        {Step::Jinzhubi, "8b", "长春功与金竺笔", "ch06.jinzhubi", kMapGu, "npc_maifu_shaonv", false, 0},
        {Step::Zhifu, "9a", "制符", "ch06.zhifu", kMapLou, "trigger_zhifu", false, 0},
        {Step::Kuxiu, "9b", "苦修", "ch06.jiuceng", kMapLou, "trigger_kuxiu", false, 12},
        {Step::Canpian, "10a", "法宝残片", "ch06.canpian", kMapGu, "trigger_canpian", true, 2},
        {Step::Bilu, "10b", "青溪笔录", "ch06.shengxianling", kMapLou, "trigger_bilu", false, 0},
        {Step::Fangshi, "—", "坊市买卖", "", kMapGu, "facility_fangshi", false, 0},
        // 复验整改 16.5（N-L6）：11 开头再拨 1 天——10a 说「只剩最后两天」、当夜 10b，散会是第二天；「又住一夜」再 1 天。
        {Step::Sanhui, "11", "散会", "ch06.chugu", kMapGu, "trigger_sanhui", true, 2},
        {Step::Xisha, "12", "袭杀", "ch06.xisha", kMapShanqiu, "trigger_xisha", true, 15},
        {Step::Linggen, "13", "测灵根", "ch06.linggen", kMapHfg, "trigger_ce_linggen", true, 4},
        {Step::Maidan, "14", "叶师叔买丹", "ch06.rangdan", kMapHfg, "trigger_ye_maidan", false, 0},
        {Step::Dadian, "15", "掌门殿", "ch06.rumen", kMapHfg, "trigger_dadian", false, 0},
        {Step::Lingqu, "16a", "领装备", "ch06.chuwudai", kMapHfg, "trigger_lin_lingqu", false, 0},
        {Step::Wufeng, "16b", "吴风切磋", "ch06.wufeng", kMapHfg, "trigger_wufeng", false, 0},
        {Step::Zawu, "17a", "杂务", "ch06.zawu", kMapHfg, "trigger_zawu", false, 0},
        {Step::Juanzong, "17b", "卷宗与悔诺", "ch06.huinuo", kMapHfg, "trigger_juanzong", false, 0},
        {Step::Jinzhi, "18a", "认药", "ch06.renyao", kMapByy, "trigger_jinzhi", true, 0},
        {Step::Maiping, "18b", "埋瓶", "ch06.done", kMapByy, "trigger_maiping", false, 4},
    };
    return kTable;
}

const StepInfo& infoOf(Step step) {
    for (const StepInfo& info : stepTable()) {
        if (info.step == step) return info;
    }
    return stepTable().front();
}

// 3.3 表的合计：从第 5 章终局起算，章末 56 天（校对整改 16.4 LOW-8：① 那一天补进来之前是 54；
// 复验整改 16.5 N-L6：11 开头那一天补进来之前是 55）。
constexpr int kChapterDays = 56;

// 一条路线：二选一取第几项 ＋ 多次作答的那几处的答案次序 ＋ 坊市买卖。
struct Route {
    std::string name;
    int fallback = 0;                          // 二选一取第几项（0 起算，Choice 命令的 choiceIndex）
    std::map<Step, std::vector<int>> answers;  // 按次序作答；答完了从头轮（同一题重来时轮到下一项）
    int sellQingdu = 0;                        // 坊市卖几瓶清毒散
    int buyDingshen = 0;                       // 坊市买几张定神符
    [[nodiscard]] bool shops() const { return sellQingdu > 0 || buyDingshen > 0; }
};

// 一场仗的记录。
struct BattleRecord {
    std::string id;
    Step step{};
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    int hpBefore = 0, hpAfter = 0, maxHp = 0;
    int mpBefore = 0, mpAfter = 0;   // mpAfter：收场那一刻他在场上还剩的法力
    int pillsBefore = 0, pillsAfter = 0;
    int salvesBefore = 0, salvesAfter = 0;
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
    bool finished = false;
    std::map<std::string, int> bagBefore, bagAfter;
    std::vector<std::string> magicsBefore, magicsAfter;
    Realm realmBefore = Realm::Mortal, realmAfter = Realm::Mortal;
    std::vector<std::string> keys;       // 这一步里说过的文案 key
    std::vector<int> answers;            // 这一步里答过的选择（0 起算）
    std::vector<BattlePhase> phases;     // 这一步里打的仗各是什么落点
};

// 两侧的起点（施工图第 2 节那张表标「硬」的、两侧不同的几格）。
struct HandOver {
    const char* file;
    int lingshi;          // 碎银 122 / 168
    int pills;            // 养精丹 12 / 0
    int salves;           // 金疮药 5 / 6
    int day;              // 1643 / 1645
    int huangjingZero;    // 黄精零年 7
    int huangjingAged;    // 首侧 40 年 6 / 次侧 24 年 2
    int huangjingAge;
    int zishenZero;       // 紫参零年 2
    int zishenAged;       // 首侧 40 年 3 / 次侧无
    int tugu;             // 土骨花 1 / 2
};

constexpr HandOver kFirstSide = {fanren::test::kChapterFiveEndingFirst, 122, 12, 5, 1643, 7, 6, 40, 2, 3, 1};
constexpr HandOver kSecondSide = {fanren::test::kChapterFiveEndingSecond, 168, 0, 6, 1645, 7, 2, 24, 2, 0, 2};

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch06Walkthrough : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        startFromChapterFiveEnding(kFirstSide);
    }

    void TearDown() override { app_.shutdown(); }

    // 第 5 章交到本章手里的那份存档，**原样读进来**（施工图第 2 节、验收 2）。
    // 与存档不同的字段：**一个也没有**。
    void startFromChapterFiveEnding(const HandOver& side) {
        const fs::path fixture = fanren::test::chapterFixturePath(assetRoot(), side.file);
        auto handedOver = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(handedOver.ok) << "第 5 章的交接存档读不进来（" << fixture.string() << "）：" << handedOver.error;
        const GameState& s = handedOver.value;
        // 验收 2 的 SetUp 先验（施工图第 15 节第 2 行）＋ 第 2 节标「硬」的每一格。
        ASSERT_EQ(s.flag("ch05.done"), 1) << "交接存档不是第 5 章的终局";
        ASSERT_EQ(s.realm, Realm::QiRefining8) << "施工图第 2 节：炼气八层（硬）";
        ASSERT_EQ(s.realmCap, Realm::QiRefining8) << "施工图第 2 节：上限八层（硬）";
        ASSERT_EQ(s.maxHp, 120) << "施工图第 2 节：气血 120（硬）";
        ASSERT_EQ(s.maxMp, 80) << "施工图第 2 节：法力 80（硬）";
        ASSERT_EQ(s.hp, s.maxHp) << "施工图第 2 节：满（硬）";
        ASSERT_EQ(s.mp, s.maxMp) << "施工图第 2 节：满（硬）";
        ASSERT_TRUE(s.party.empty()) << "施工图第 2 节：party 空（硬）";
        const std::vector<std::string> twoMagics{kFireMagic, kWindMagic};
        ASSERT_EQ(s.learnedMagics, twoMagics) << "施工图第 2 节：火弹术、御风决两门（硬）";
        ASSERT_EQ(s.itemCount(kJianfuItem), 1) << "施工图第 2 节：剑符 1（硬）";
        ASSERT_EQ(s.itemCount(kPlaque), 1) << "施工图第 2 节：那块牌子 1（硬）";
        ASSERT_EQ(s.itemCount(kBaoyu), 1) << "施工图第 2 节：暖阳宝玉 1（硬）";
        ASSERT_EQ(s.itemCount(kSoftSword), 1) << "施工图第 2 节：软剑 1（硬）";
        ASSERT_EQ(s.itemCount(kLingshi), side.lingshi) << "施工图第 2 节：碎银（硬，两侧不同）";
        ASSERT_EQ(s.itemCount(kPill), side.pills) << "施工图第 2 节：养精丹（硬，两侧不同）";
        ASSERT_EQ(s.itemCount(kSalve), side.salves) << "施工图第 2 节：金疮药（硬）";
        ASSERT_EQ(s.itemCount(kQingdu), 3) << "施工图第 2 节：清毒散 3（硬）";
        ASSERT_EQ(s.itemCountOfAge(kHuangjing, 0), side.huangjingZero) << "施工图第 2 节：黄精零年 7（硬）";
        ASSERT_EQ(s.itemCountOfAge(kHuangjing, side.huangjingAge), side.huangjingAged) << "施工图第 2 节：年份黄精（硬）";
        ASSERT_EQ(s.itemCountOfAge(kZishen, 0), side.zishenZero) << "施工图第 2 节：紫参零年 2（硬）";
        ASSERT_EQ(s.itemCountOfAge(kZishen, 40), side.zishenAged) << "施工图第 2 节：40 年紫参（硬）";
        ASSERT_EQ(s.itemCount(kTugu), side.tugu) << "施工图第 2 节：土骨花（硬）";
        ASSERT_TRUE(s.bottle.owned) << "施工图第 2 节：掌天瓶 owned（硬）";
        ASSERT_TRUE(s.bottle.matureKnown) << "施工图第 2 节：matureKnown（硬）";
        ASSERT_EQ(s.bottle.drops, 3) << "施工图第 2 节：3/3 滴（硬）";
        ASSERT_EQ(s.bottle.capacity, 3) << "施工图第 2 节：3/3 滴（硬）";
        ASSERT_EQ(s.alchemyProficiency, 100) << "施工图第 2 节：炼丹熟练度 100（硬）";
        ASSERT_EQ(s.talismanProficiency, 0) << "施工图第 2 节：制符熟练度 0（硬）";
        ASSERT_EQ(s.aptitude, 50) << "施工图第 2 节：资质 50（硬）";
        ASSERT_EQ(s.knownWeaknesses.size(), 15u) << "施工图第 2 节：15 个第 3-5 章的敌人（硬）";
        ASSERT_EQ(s.day, side.day) << "施工图第 2 节：日子（硬）";
        ASSERT_EQ(s.mapId, kMapMofu) << "施工图第 2 节：墨府（硬）";
        ASSERT_EQ(s.position.x, 32) << "施工图第 2 节：(32, 10)（硬）";
        ASSERT_EQ(s.position.y, 10) << "施工图第 2 节：(32, 10)（硬）";
        ASSERT_EQ(s.flag("story.xiuxian_known"), 0) << "施工图第 2 节：还没进修仙界（硬）";
        for (const StepInfo& info : stepTable()) {
            if (info.doneFlag[0] != '\0') ASSERT_EQ(s.flag(info.doneFlag), 0) << "第 5 章终局不该已经置了 " << info.doneFlag;
        }

        const Point where = s.position;
        const int facing = s.facing;
        state() = s;
        auto loaded = app_.loadMap(s.mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        // loadMap 把人挪到了出生点；放回存档里那一格——这是撤销那一挪，不是改字段。
        state().position = where;
        state().facing = facing;
        startDay_ = state().day;
        side_ = &side;
        battles_.clear();
        steps_.clear();
        chapterLog_.clear();
        stuckAt_.clear();
        answered_.clear();
        shopped_ = false;
        throwFights_.clear();
        app_.clearSpokenKeys();
    }

    void settleEndingAgainstFixture(const char* fileName) {
        const fanren::test::FixtureVerdict verdict = fanren::test::settleAgainstFixture(
            state(), assetRoot(), fileName, fanren::test::kWriteChapterSixFixturesEnv, "第 7 章");
        if (verdict.wrote) {
            std::cout << "[ch06 交接存档] 已写出 " << fanren::test::chapterFixturePath(assetRoot(), fileName).string()
                      << std::endl;
        }
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
    }

    GameState& state() { return app_.state(); }
    int flag(const std::string& name) { return state().flag(name); }

    void complain(const std::string& why) {
        if (!tolerateBlocked_) ADD_FAILURE() << why;
    }

    // -----------------------------------------------------------------------
    // 文案：读走 spokenKeys 并清空（见文件头「选择怎么答」）。返回这一回读到的条数。
    // -----------------------------------------------------------------------
    std::size_t drainKeys() {
        const std::vector<std::string> keys = app_.spokenKeys();
        for (const std::string& key : keys) {
            chapterLog_.emplace_back(currentStep_, key);
            if (!steps_.empty()) steps_.back().keys.push_back(key);
        }
        app_.clearSpokenKeys();
        return keys.size();
    }
    bool spokeIn(Step step, const std::string& key) const {
        for (const StepRecord& record : steps_) {
            if (record.step != step) continue;
            if (std::find(record.keys.begin(), record.keys.end(), key) != record.keys.end()) return true;
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
    const StepRecord* recordOf(Step step) const {
        for (const StepRecord& record : steps_) {
            if (record.step == step && record.finished) return &record;
        }
        return nullptr;
    }

    // 这一回等着的是一个选择：按路线作答。
    int answerChoice() {
        const int nth = answered_[currentStep_]++;
        int pick = route_ != nullptr ? route_->fallback : 0;
        if (route_ != nullptr) {
            const auto it = route_->answers.find(currentStep_);
            if (it != route_->answers.end() && !it->second.empty()) {
                pick = it->second[static_cast<std::size_t>(nth) % it->second.size()];
            }
        }
        if (!steps_.empty()) steps_.back().answers.push_back(pick);
        return pick;
    }

    // -----------------------------------------------------------------------
    // 战斗：意图玩家（文件头第三段）
    // -----------------------------------------------------------------------
    static fanren::test::HandPolicy intentPolicy() {
        fanren::test::HandPolicy p;
        p.healAtPercent = 50;          // 沿用第 5 章意图玩家：血掉到一半才吃药
        p.healWhenDoomed = false;      // 不算对面下一轮打不打得死自己
        p.fleeWhenSpent = true;        // 没药能逃就逃（本章三场必打都不许逃）
        p.pills = {kPill, kSalve};     // 施工图 8.2 ②「打得赢，但要吃金疮药」
        return p;
    }

    // 这一场是哪一张编成：按「敌方每个人的角色与波次」对施工图 8.2 那五张认。认不出来记成「?」。
    std::string identifyBattle(const BattleScene& scene) {
        std::vector<std::pair<std::string, int>> have;
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally) have.emplace_back(u.id, u.wave);
        }
        std::sort(have.begin(), have.end());
        for (const char* id : kChapterBattles) {
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

    // 「这一场故意输」：我方每一个人每一手都按防御，一招不出，直到分出胜负。只给「输了之后路怎么走」
    // 那几条用例用（施工图 8.2：①③ 败不致命、② 败即 game over），不是意图玩家。
    BattlePhase playPassively(BattleScene& scene) {
        for (int step = 0; step < 8000; ++step) {
            if (scene.battle().phase() != BattlePhase::Ongoing) break;
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            scene.openMenu(app_);
            if (!scene.menuChoose(app_, fanren::game::kBattleMenuDefend)) {
                scene.closeMenu();
                break;
            }
        }
        return scene.battle().phase();
    }

    // -----------------------------------------------------------------------
    // 脚本
    // -----------------------------------------------------------------------
    void pumpScripts() {
        int pendingFill = -1;
        for (int frame = 0; frame < kMaxScriptFrames && app_.scripts().isRunning(); ++frame) {
            app_.tick(kFrame);
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
                    if (u.ally) record.allyIds.push_back(u.id);
                }
                if (throwFights_.count(record.id)) {
                    record.phase = playPassively(*fight);
                } else {
                    fanren::test::HandPolicy policy = policy_;
                    // 复验整改 16.5（N-L5）：吴风切磋里剑符「能不露就不露」，露不露由玩家定。两侧拆开（抽查 N3-L4）：
                    // 第一侧照这句话捂着剑符（③ 输，wufeng = 2），第二侧照放（③ 赢，wufeng = 1）——两种结局都有通关走到，
                    // 第 7 章也拿得到两种交接存档。
                    // 空手拳：候选里有（BattleScene::heroWeapons「空手永远算一样」，拳也是吴风的破绽），但这只手的次序是
                    // 「打已知破绽」排在「拿没试过的类别去探」之前——火弹先揭出「火」之后它一直打火，拳这一样破绽始终
                    // 没去探（第一侧终局 knownWeaknesses 里吴风只有「火」为证）。这是几章共用那只手的次序，这里照实记下，不改它。
                    if (record.id == "b06_wufeng_qiecuo" && side_ == &kFirstSide) policy.withheldMagics.push_back(kJijianfu);
                    fanren::test::BattleHand hand(app_, policy);
                    record.phase = hand.play(*fight);
                }
                record.rounds = fight->battle().round();
                // 法力不写回存档（BattleScene::settle 只带出气血），这一场花了多少要在场上读。
                for (const Unit& u : fight->battle().units()) {
                    if (u.ally && u.id == "hanli") record.mpAfter = u.mp;
                }
                battles_.push_back(record);
                if (!steps_.empty()) steps_.back().phases.push_back(record.phase);
                pendingFill = static_cast<int>(battles_.size()) - 1;
                continue;
            }
            if (!app_.awaitingCommand()) continue;
            const bool talked = drainKeys() > 0;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = talked ? 0 : answerChoice();
            app_.completeCommand(result);
            app_.popScene();
        }
        if (app_.scripts().isRunning()) complain("脚本没能跑到结束");
        app_.tick(kFrame);
        drainKeys();
        if (pendingFill >= 0) fillAfter(static_cast<std::size_t>(pendingFill));
    }

    void fillAfter(std::size_t index) {
        BattleRecord& record = battles_[index];
        record.hpAfter = state().hp;
        record.pillsAfter = state().itemCount(kPill);
        record.salvesAfter = state().itemCount(kSalve);
    }

    // -----------------------------------------------------------------------
    // 地图与走位（与第 3、4、5 章同一套）
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

    bool step(Point direction) {
        const bool moved = world_.tryStep(app_, direction.x, direction.y);
        if (app_.scripts().isRunning()) pumpScripts();
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
                if (next.x < 0 || next.y < 0 || next.x >= map().width || next.y >= map().height) continue;
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

    bool walkTo(Point goal, bool strict = false) {
        if (app_.state().position == goal) return true;
        std::vector<Point> path = routeTo(goal, /*avoidTriggers=*/true);
        if (path.empty() && !strict) path = routeTo(goal, /*avoidTriggers=*/false);
        if (path.empty()) return false;
        const std::string mapBefore = app_.state().mapId;
        for (const Point& cell : path) {
            const Point here = app_.state().position;
            if (!step(Point{cell.x - here.x, cell.y - here.y})) return false;
            if (app_.state().mapId != mapBefore) return false;
        }
        return app_.state().position == goal;
    }

    bool faceCell(Point target) {
        for (const Point& direction : kDirections) {
            const Point stand{target.x - direction.x, target.y - direction.y};
            if (!standable(stand, /*avoidTriggers=*/true)) continue;
            if (!enterable(target)) {
                if (!walkTo(stand)) continue;
                step(direction);
            } else {
                const Point approach{stand.x - direction.x, stand.y - direction.y};
                if (!standable(approach, /*avoidTriggers=*/true)) continue;
                if (!walkTo(approach)) continue;
                if (!step(direction)) continue;
            }
            if (app_.state().position == stand && app_.state().facing == facingOf(direction)) return true;
        }
        return false;
    }

    bool faceObject(const MapObject& object) {
        for (int dy = 0; dy < std::max(1, object.height); ++dy) {
            for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                if (faceCell(Point{object.position.x + dx, object.position.y + dy})) return true;
            }
        }
        return false;
    }

    bool stepOnto(const MapObject& object) {
        for (const bool strict : {true, false}) {
            for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                    const Point target{object.position.x + dx, object.position.y + dy};
                    if (!map().walkable(target)) continue;
                    for (const Point& direction : kDirections) {
                        const Point stand{target.x - direction.x, target.y - direction.y};
                        if (!standable(stand, /*avoidTriggers=*/true)) continue;
                        if (!walkTo(stand, strict)) continue;
                        step(direction);
                        return true;
                    }
                }
            }
        }
        return false;
    }

    // 面朝一处 interact 触发器 / NPC 按确认并把脚本演完。返回「脚本真的起来了」。
    bool fireTrigger(const std::string& name) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        if (!faceObject(trigger)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        drainKeys();
        if (!world_.interact(app_)) {
            complain(name + " 点不着");
            return false;
        }
        if (!app_.scripts().isRunning()) {
            complain(name + " 点着的不是脚本");
            return false;
        }
        pumpScripts();
        return true;
    }

    // 踩上踏入型触发。返回「踩上去了」（guard 未满足时踩上去什么也不发生）。
    bool enterTrigger(const std::string& name) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        drainKeys();
        if (!stepOnto(trigger)) {
            complain("走不到 " + name + " 上");
            return false;
        }
        return true;
    }

    bool pressAt(const MapObject& object) {
        if (!faceObject(object)) return false;
        drainKeys();
        const bool handled = world_.interact(app_);
        app_.tick(kFrame);
        return handled;
    }

    bool usePortal(const std::string& name) {
        const MapObject portal = objectNamed(name);
        if (portal.name.empty()) {
            complain(app_.state().mapId + " 上没有 " + name);
            return false;
        }
        const std::string before = app_.state().mapId;
        if (!stepOnto(portal)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        return app_.state().mapId != before;
    }

    void closePanel() {
        app_.popScene();
        app_.tick(kFrame);
    }

    bool travelTo(const std::string& target) {
        for (int hop = 0; hop < 8; ++hop) {
            if (app_.state().mapId == target) return true;
            const std::string portal = nextPortalToward(app_.state().mapId, target);
            if (portal.empty()) {
                complain("从 " + app_.state().mapId + " 没有门通到 " + target);
                return false;
            }
            if (!usePortal(portal)) {
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
                if (here != link.from || cameFrom.count(link.to)) continue;
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

    // 被闸门拦下时屏幕上那一句：栈顶是对话框，最后一行是 deny_text_key 的正文。
    void expectDeniedWith(const std::string& denyKey, const std::string& why) {
        app_.tick(kFrame);
        ASSERT_NE(app_.topScene(), nullptr) << why;
        EXPECT_EQ(app_.topScene()->name(), "Dialogue") << why << "：拦住玩家就要说明还差什么";
        ASSERT_FALSE(app_.dialogueLog().empty()) << why;
        EXPECT_EQ(app_.dialogueLog().back().body, app_.data().lookupText(denyKey)) << why << "：拦下时说的不是 " << denyKey;
        closePanel();
    }

    // -----------------------------------------------------------------------
    // 坊市：真的走进太南坊市卖、买（Z1 / Z2 的钱与符）
    // -----------------------------------------------------------------------
    bool shopAtFangshi(const Route& route) {
        if (!travelTo(kMapGu)) return false;
        const MapObject shop = objectNamed("facility_fangshi");
        if (shop.name.empty() || !pressAt(shop)) {
            complain("太南坊市开不了门");
            return false;
        }
        auto* panel = dynamic_cast<ShopScene*>(app_.topScene());
        if (panel == nullptr) {
            complain("按坊市开出来的不是商店面板");
            return false;
        }
        fanren::rules::Shop* shopState = app_.shop(panel->shopId());
        bool ok = shopState != nullptr;
        for (int i = 0; ok && i < route.sellQingdu; ++i) {
            const auto stacks = ShopScene::buildSellStacks(app_.data(), state(), *shopState);
            int row = -1;
            for (std::size_t k = 0; k < stacks.size(); ++k) {
                if (stacks[k].itemId == kQingdu) row = static_cast<int>(k);
            }
            ok = row >= 0 && panel->sellAt(app_, row);
        }
        for (int i = 0; ok && i < route.buyDingshen; ++i) {
            int entry = -1;
            for (std::size_t k = 0; k < shopState->entries.size(); ++k) {
                if (shopState->entries[k].itemId == kDingshenFu) entry = static_cast<int>(k);
            }
            ok = entry >= 0 && panel->buyAt(app_, entry);
        }
        panel->leave(app_);
        app_.tick(kFrame);
        if (!ok) complain("坊市里的买卖没做成：" + panel->feedback());
        shopped_ = ok;
        return ok;
    }

    // -----------------------------------------------------------------------
    // 一步：走过去、演一趟
    // -----------------------------------------------------------------------
    bool attemptStep(Step s) {
        if (s == Step::Fangshi) return shopAtFangshi(*route_);
        const StepInfo& info = infoOf(s);
        if (!travelTo(info.mapId)) return false;
        return info.enter ? enterTrigger(info.object) : fireTrigger(info.object);
    }

    bool isDone(Step s) {
        if (s == Step::Fangshi) return shopped_;
        return flag(infoOf(s).doneFlag) != 0;
    }

    bool runStep(Step s) {
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            beginStep(s);
            const bool started = attemptStep(s);
            endStep();
            if (HasFatalFailure() || app_.quitRequested()) return false;
            if (isDone(s)) {
                steps_.back().finished = true;
                return true;
            }
            if (!started) return false;
        }
        return isDone(s);
    }

    void beginStep(Step s) {
        currentStep_ = s;
        StepRecord record;
        record.step = s;
        record.dayBefore = state().day;
        record.bagBefore = bagCounts(state());
        record.magicsBefore = state().learnedMagics;
        record.realmBefore = state().realm;
        steps_.push_back(record);
    }
    void endStep() {
        drainKeys();
        if (steps_.empty()) return;
        StepRecord& record = steps_.back();
        record.dayAfter = state().day;
        record.bagAfter = bagCounts(state());
        record.magicsAfter = state().learnedMagics;
        record.realmAfter = state().realm;
    }

    std::vector<Step> stepsFor(const Route& r) const {
        std::vector<Step> out;
        for (const StepInfo& info : stepTable()) {
            if (info.step == Step::Fangshi && !r.shops()) continue;
            out.push_back(info.step);
        }
        return out;
    }

    // 整章走一遍。before / after 是用例插断言的钩子。返回「走到了 ch06.done」。
    bool playChapter(const Route& route, const std::function<void(Step)>& before = {},
                     const std::function<void(Step)>& after = {}) {
        route_ = &route;
        for (Step s : stepsFor(route)) {
            if (before) before(s);
            if (HasFatalFailure()) return false;
            const bool done = runStep(s);
            if (after) after(s);
            if (HasFatalFailure() || app_.quitRequested()) return false;
            if (!done) {
                stuckAt_ = std::string(infoOf(s).node) + " " + infoOf(s).name;
                return false;
            }
        }
        return flag("ch06.done") == 1;
    }

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

    std::string battleLine() const {
        std::ostringstream out;
        for (const BattleRecord& b : battles_) {
            out << "\n    " << b.id << " " << phaseName(b.phase) << " " << b.rounds << " 回合  韩立 " << b.hpBefore
                << "→" << b.hpAfter << "/" << b.maxHp << "  法力 " << b.mpBefore << "→" << b.mpAfter << "  养精丹 "
                << b.pillsBefore << "→" << b.pillsAfter << "  金疮药 " << b.salvesBefore << "→" << b.salvesAfter;
        }
        return out.str();
    }

    // -----------------------------------------------------------------------
    // 整章走完之后的判卷
    // -----------------------------------------------------------------------
    // 每一步的天数都对得上施工图 3.3（逐步，不只看总数）。
    void expectEveryStepTookTheDesignDays() {
        for (const StepRecord& record : steps_) {
            if (!record.finished) continue;
            const int took = record.dayAfter - record.dayBefore;
            EXPECT_EQ(took, infoOf(record.step).days)
                << infoOf(record.step).node << " " << infoOf(record.step).name << " 走了 " << took << " 天，施工图 3.3 该是 "
                << infoOf(record.step).days << " 天";
        }
        EXPECT_EQ(state().day - startDay_, kChapterDays) << "施工图 3.3：从第 5 章终局起算，章末累计 56 天";
    }

    // 每一步背包的进出与施工图 3.2 / 第 9 节一格不差。仗里吃掉的药（养精丹、金疮药）与撒掉的毒另算：
    // 只许少不许多。坊市那一步是玩家自己的买卖，不判。
    std::map<std::string, int> designBagDiff(const StepRecord& r) {
        std::map<std::string, int> d;
        const auto had = [&](const char* id) {
            const auto it = r.bagBefore.find(id);
            return it == r.bagBefore.end() ? 0 : it->second;
        };
        switch (r.step) {
            case Step::Chudu:
                // 碎银整包留给借住的那户人家；养精丹补足到 15；黄精紫参不论年份全部用掉（施工偏差 18.4 第 3 条）。
                if (had(kLingshi) > 0) d[kLingshi] = -had(kLingshi);
                if (had(kPill) < kPillsAfterChudu) d[kPill] = kPillsAfterChudu - had(kPill);
                if (had(kHuangjing) > 0) d[kHuangjing] = -had(kHuangjing);
                if (had(kZishen) > 0) d[kZishen] = -had(kZishen);
                break;
            case Step::Ruhuo: d[kJinfaFu] = 1; break;   // 3.2 节点 4：给禁法符
            case Step::Feixingfu:
                d[kPill] = -kBarterFeixing;
                d[kFeixingFu] = 1;
                d[kFuzhi] = kFuzhiFromCaomao;
                break;
            case Step::Jinzhubi:
                d[kPill] = -(kBarterBook + kBarterBrush);
                d[kJinzhuBi] = 1;
                d[kDansha] = kDanshaFromStall;
                if (flag("ch06.zhongzi") == 1) d[kSeed] = 1;
                if (had(kSalve) > 0) d[kSalve] = -1;   // 丹砂拿一瓶金疮药换（有则扣；校对 LOW-2 删了「最后」）
                break;
            case Step::Zhifu:
                d[kFuzhi] = -kFuzhiWasted;
                d[kDansha] = -kDanshaWasted;
                break;
            case Step::Canpian:
                d[kFeixingFu] = -1;
                d[kCanpian] = 1;
                d[kBilu] = 1;
                break;
            case Step::Bilu:
                d[kPlaque] = -1;
                d[kShengxianling] = 1;
                break;
            case Step::Xisha:
                d[kLingshi] = kSpoilsLingshi;
                d[kHuoqiuFu] = kSpoilsHuoqiu;
                d[kHushenFu] = kSpoilsHushen;
                break;
            case Step::Lingqu:
                d[kQingye] = 1;
                d[kLieyang] = 1;
                d[kLengyue] = 1;
                d[kChuwudai] = 1;
                break;
            case Step::Juanzong: d[kYupai] = 1; break;
            case Step::Jinzhi: d[kMupai] = 1; break;
            case Step::Maiping: {
                const Delivery g = designDelivery(flag("ch06.huinuo"), flag("ch06.rangdan"));
                d[kLingshi] = g.lingshi;
                d[kHuoqiuFu] = g.huoqiu;
                if (g.hushen > 0) d[kHushenFu] = g.hushen;
                if (g.jinci > 0) d[kJinciFu] = g.jinci;
                break;
            }
            default: break;
        }
        return d;
    }

    void expectEveryStepMovedTheBagTheDesignSays() {
        for (const StepRecord& r : steps_) {
            if (!r.finished || r.step == Step::Fangshi) continue;
            std::map<std::string, int> got = diffOf(r.bagBefore, r.bagAfter);
            const bool fight = !r.phases.empty();
            std::map<std::string, int> spent;
            if (fight) {
                for (const char* id : {kPill, kSalve, kShixin}) {
                    const auto it = got.find(id);
                    if (it == got.end()) continue;
                    // 药在编成之外另有进出（8b 那一步没有仗）：仗里只许少。
                    if (!designBagDiff(r).count(id)) {
                        spent[id] = it->second;
                        got.erase(it);
                    }
                }
                for (const auto& [id, n] : spent) {
                    EXPECT_LT(n, 0) << infoOf(r.step).node << " 那一仗之后 " << id << " 反倒多了 " << n;
                }
            }
            EXPECT_EQ(showBag(got), showBag(designBagDiff(r)))
                << infoOf(r.step).node << " " << infoOf(r.step).name << " 背包的进出与施工图 3.2 / 第 9 节不符";
        }
    }

    // 学法术、升境只在施工图写的那几步（1b 天眼术；9b 流沙、冰冻、祭剑符与九层）。
    void expectMagicsAndRealmMoveOnlyWhereTheDesignSays() {
        for (const StepRecord& r : steps_) {
            if (!r.finished) continue;
            std::vector<std::string> learned;
            for (const std::string& id : r.magicsAfter) {
                if (std::find(r.magicsBefore.begin(), r.magicsBefore.end(), id) == r.magicsBefore.end()) {
                    learned.push_back(id);
                }
            }
            std::sort(learned.begin(), learned.end());
            std::vector<std::string> want;
            if (r.step == Step::Cunkou) want = {kTianyan};
            if (r.step == Step::Kuxiu) want = {kJijianfu, kBingdong, kLiusha};
            std::sort(want.begin(), want.end());
            EXPECT_EQ(learned, want) << infoOf(r.step).node << " 学会的法术与施工图不符（1b 天眼术；9b 流沙、冰冻、祭剑符）";
            EXPECT_GE(r.magicsAfter.size(), r.magicsBefore.size())
                << infoOf(r.step).node << " 忘掉了法术（本章 magic.forget 0 处）";
            const Realm want9 = r.step == Step::Kuxiu ? Realm::QiRefining9 : r.realmBefore;
            EXPECT_EQ(r.realmAfter, want9) << infoOf(r.step).node << " 的境界变化与施工图 1.3 不符（只在 9b 升一层）";
        }
    }

    // 三场必打一场不少（验收 15 走一遍那一半）；每一场认得出。
    void expectTheThreeMustFightsAllHappened() {
        const std::vector<std::string> ids = battleIds();
        for (std::size_t i = 0; i < std::size(kMustFight); ++i) {
            EXPECT_EQ(std::count(ids.begin(), ids.end(), kMustFight[i]), 1)
                << kMustFightMark[i] << " " << kMustFight[i] << " 这一趟该打恰好一次（施工图 8.0）：" << battleLine();
        }
        for (const std::string& id : ids) EXPECT_NE(id, "?") << "有一场认不出是哪一张编成：" << battleLine();
        // ① 我方：韩立 ＋ 友军三人（施工图 8.2 ① 友军一列：青纹道士、黑木、熊大力——校对整改 16.4 HIGH-4，
        // 原为吴九指：他节点 7 才头一回见面）。
        if (const BattleRecord* first = lastBattle(kMustFight[0])) {
            std::multiset<std::string> allies(first->allyIds.begin(), first->allyIds.end());
            const std::multiset<std::string> design = {"hanli", "qingwen_daoshi", "hei_mu", "xiong_dali"};
            EXPECT_EQ(allies, design) << "① 我方该是韩立与青纹道士、黑木、熊大力";
        }
        // ② 生死仗：只许赢（输了就 game over，走不到这里）。
        if (const BattleRecord* second = lastBattle(kMustFight[1])) {
            EXPECT_EQ(second->phase, BattlePhase::Won) << battleLine();
            EXPECT_EQ(second->allyIds, std::vector<std::string>{"hanli"}) << "② 全章无同伴，只有他一个";
        }
    }

    // 验收 2 的终点先验（施工图第 15 节第 2 行，逐条抄）＋ 第 2 节、1.3 写死的几格。
    void expectTheChapterEndState() {
        GameState& s = state();
        EXPECT_EQ(s.flag("ch06.done"), 1);
        EXPECT_EQ(s.realm, Realm::QiRefining9) << "验收 2：realm == 9";
        EXPECT_EQ(s.realmCap, Realm::QiRefining9) << "验收 2：realmCap == 9";
        EXPECT_EQ(s.maxHp, kNinthLayerHp) << "施工图 1.3：九层气血 132";
        EXPECT_EQ(s.maxMp, kNinthLayerMp) << "施工图 1.3：九层法力 90";
        const std::set<std::string> six = {kFireMagic, kWindMagic, kTianyan, kLiusha, kBingdong, kJijianfu};
        EXPECT_EQ(s.learnedMagics.size(), 6u) << "验收 2：learnedMagics 恰 6 门";
        EXPECT_EQ(std::set<std::string>(s.learnedMagics.begin(), s.learnedMagics.end()), six)
            << "验收 2：原两门 + 天眼、流沙、冰冻、祭剑符";
        EXPECT_EQ(s.itemCount(kPlaque), 0) << "验收 2：material_heipai 0";
        EXPECT_EQ(s.itemCount(kShengxianling), 1) << "验收 2：story_shengxianling 1";
        EXPECT_EQ(s.itemCount(kCanpian), 1) << "验收 2：story_fabao_canpian 1";
        EXPECT_EQ(s.itemCount(kChuwudai), 1) << "验收 2：story_chuwudai 1";
        const fanren::rules::SpiritField* field = s.findField(kField);
        ASSERT_NE(field, nullptr) << "验收 2：fields 里有 field_baiyaoyuan";
        EXPECT_EQ(field->slots.size(), static_cast<std::size_t>(kFieldSlots)) << "验收 2：6 槽";
        EXPECT_EQ(s.flag("story.xiuxian_known"), 1) << "验收 2：story.xiuxian_known == 1";
        // 灵石（校对整改 16.4 / LOW-11 (b)，施工图验收第 2 条与第 9 节第 6 条）：章末 = 50（②）＋ 送物 ＋ 卖出 − 买入 − 补差；
        // 原写「≥ 80」不是全路径不变量。出谷那一刻之后只有搜身与送物两笔进账，所以按 11 之前的余数往后推。
        int stonesBeforeSanhui = -1;
        for (const StepRecord& r : steps_) {
            if (r.step != Step::Sanhui || !r.finished) continue;
            const auto it = r.bagBefore.find(kLingshi);
            stonesBeforeSanhui = it == r.bagBefore.end() ? 0 : it->second;
        }
        ASSERT_GE(stonesBeforeSanhui, 0) << "先验：这一趟走过了 11";
        const Delivery delivered = designDelivery(s.flag("ch06.huinuo"), s.flag("ch06.rangdan"));
        EXPECT_EQ(s.itemCount(kLingshi), stonesBeforeSanhui + kSpoilsLingshi + delivered.lingshi)
            << "验收 2：灵石 = 出谷前的余数 ＋ 搜身 50 ＋ 送物";
        // 第 2 节「本章怎么用」一列：全章无同伴；剑符物品本身不动；暖阳宝玉本章不收回；小瓶埋着但还是他的。
        EXPECT_TRUE(s.party.empty()) << "施工图第 2 节：全章无同伴";
        EXPECT_EQ(s.itemCount(kJianfuItem), 1) << "施工图第 2 节：剑符物品本身不动";
        EXPECT_EQ(s.itemCount(kBaoyu), 1) << "施工图第 2 节：暖阳宝玉本章不收回";
        EXPECT_TRUE(s.bottle.owned) << "施工图 3.2 节点 18b：bottle 状态不动（还是他的，只是埋着）";
        EXPECT_EQ(s.itemCount(kFeixingFu), 0) << "10a 飞行符换了残片";
        EXPECT_EQ(s.itemCount(kJinzhuBi), 1);
        EXPECT_EQ(s.itemCount(kBilu), 1);
        EXPECT_EQ(s.itemCount(kJinfaFu), 1);
        EXPECT_EQ(s.itemCount(kQingye), 1);
        EXPECT_EQ(s.itemCount(kLieyang), 1);
        EXPECT_EQ(s.itemCount(kLengyue), 1);
        EXPECT_EQ(s.itemCount(kYupai), 1);
        EXPECT_EQ(s.itemCount(kMupai), 1);
        EXPECT_EQ(s.itemCount(kSeed), s.flag("ch06.zhongzi") == 1 ? 1 : 0) << "8b 要了种子才有种子";
        EXPECT_EQ(s.day - startDay_, kChapterDays);
        EXPECT_NE(fanren::game::CultivationScene::spiritRootLine(s).find(kSpiritRootText), std::string::npos)
            << "E3：测过灵根，面板有那一行";
    }

    // 施工图 12.2 的章内先后，**按玩家实际读到的顺序**判：每个词头一回被说出来在哪一节。
    std::map<std::string, std::string> firstNodeOfEachGatedWord() {
        std::map<std::string, std::string> first;
        for (const auto& [step, key] : chapterLog_) {
            const std::string text = app_.data().lookupText(key);
            for (const char* word : {"升仙令", "灵石", "燕", "松纹", "太南小会", "升仙大会", "天眼", "中阶灵石"}) {
                if (text.find(word) != std::string::npos && first.count(word) == 0) first[word] = infoOf(step).node;
            }
        }
        return first;
    }

    void expectGatedWordsArriveWhereTheDesignSays() {
        ASSERT_GT(chapterLog_.size(), 300u) << "先验：整章说过的句子够多，下面的「没有」才有分量";
        std::map<std::string, std::string> first = firstNodeOfEachGatedWord();
        EXPECT_EQ(first["升仙令"], "10b") << "施工图 12.2 / 17 第 14 条：「升仙令」三个字在节点 10b 之前一个不许出现";
        EXPECT_EQ(first["灵石"], "5a") << "施工图 12.2：节点 1-4 不含「灵石」（5a 才讲）";
        EXPECT_EQ(first["太南小会"], "2") << "施工图 3.2 节点 2：太南小会的名字在这里第一次出现";
        EXPECT_EQ(first["升仙大会"], "2") << "施工图 3.2 节点 2：升仙大会的名字在这里第一次出现";
        EXPECT_EQ(first["天眼"], "1b") << "施工图 1.1 第 2 条：天眼术第一次真正施展是看万小山";
        EXPECT_EQ(first.count("松纹"), 0u) << "施工图 12.2：全章只用「青纹」";
        EXPECT_EQ(first.count("中阶灵石"), 0u) << "施工图第 7 节：文案里不出现「中阶灵石」";
        // 「燕」只在 8a 那一幕（施工图 12.2）。
        for (const auto& [step, key] : chapterLog_) {
            if (app_.data().lookupText(key).find("燕") == std::string::npos) continue;
            EXPECT_EQ(step, Step::Shuangshou) << key << "（节点 " << infoOf(step).node << "）说了「燕」：燕家到 8a 为止";
        }
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
    fanren::ui::ListItem boardRowOf(const std::string& questId) {
        const fanren::core::Quest* quest = questOf(questId);
        if (quest == nullptr) return {};
        const std::string title = app_.data().lookupText(quest->titleKey);
        for (const fanren::ui::ListItem& row : BoardScene::buildRows(app_.data(), state())) {
            if (row.label.rfind(title, 0) == 0) return row;
        }
        return {};
    }

    // 认药三题的正确答案（0 起算，按出题次序）：选项从 jinzhi.lua 的 choice{} 读，哪一项对只看施工图
    //（18.4 第 1 条：子夜花、白鹤芝、望月草）。竹简那一处同理（3.2 节点 17：只有百药园走得下去）。
    std::vector<int> designQuizAnswers() {
        std::vector<int> out;
        const auto blocks = choiceBlocks(readFile(fs::path(assetRoot()) / "scripts" / "ch06" / "jinzhi.lua"));
        for (std::size_t q = 0; q < blocks.size() && q < std::size(kQuizHerbs); ++q) {
            int right = -1;
            for (std::size_t i = 0; i < blocks[q].size(); ++i) {
                if (app_.data().lookupText(blocks[q][i]) == kQuizHerbs[q]) right = static_cast<int>(i);
            }
            out.push_back(right);
        }
        return out;
    }
    int designSlipAnswer() {
        const auto blocks = choiceBlocks(readFile(fs::path(assetRoot()) / "scripts" / "ch06" / "zawu.lua"));
        if (blocks.size() != 1u) return -1;
        for (std::size_t i = 0; i < blocks[0].size(); ++i) {
            if (app_.data().lookupText(blocks[0][i]).find(kSlipThatWorks) != std::string::npos) return static_cast<int>(i);
        }
        return -1;
    }

    Route firstSideRoute() {
        Route r;
        r.name = "第一侧";
        r.fallback = 0;
        const std::vector<int> quiz = designQuizAnswers();
        EXPECT_EQ(quiz.size(), 3u) << "施工图 18.4 第 1 条：认药三题";
        for (const int q : quiz) EXPECT_GE(q, 0) << "认药有一题的选项里没有施工图点名的那一味";
        r.answers[Step::Jinzhi] = quiz;
        const int slip = designSlipAnswer();
        EXPECT_GE(slip, 0) << "竹简上没有「百药园」那一条（施工图 3.2 节点 17）";
        r.answers[Step::Zawu] = {slip};
        r.sellQingdu = 1;     // 卖一瓶清毒散，
        r.buyDingshen = 1;    // 买一张定神符：Z1 了结，Z2 攒不够十块、出谷即过期
        return r;
    }
    static Route secondSideRoute() {
        Route r;
        r.name = "第二侧";
        r.fallback = 1;
        r.answers[Step::Jinzhi] = {0, 1, 2, 3};   // 四个选项挨个点：选错同一题重来（验收 19）
        r.answers[Step::Zawu] = {0, 1, 2, 3};     // 竹简四条挨个点：前三条各回一句再回到竹简
        r.sellQingdu = 3;     // 三瓶清毒散攒够十块（Z2 了结）；不买定神符（Z1 出谷即过期）
        return r;
    }

    Application app_;
    WorldScene world_;
    fanren::test::HandPolicy policy_ = intentPolicy();
    const Route* route_ = nullptr;
    const HandOver* side_ = nullptr;
    std::map<Step, int> answered_;
    int startDay_ = 1;
    bool tolerateBlocked_ = false;
    bool shopped_ = false;
    Step currentStep_ = Step::Chudu;
    std::vector<BattleRecord> battles_;
    std::vector<StepRecord> steps_;
    std::vector<std::pair<Step, std::string>> chapterLog_;
    std::string stuckAt_;
    std::set<std::string> throwFights_;   // 这几张编成一开打就只防御、故意输（playPassively）
};

// ---------------------------------------------------------------------------
// 起点：第 5 章终局原样读入，一个字段也不改
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, StartsFromTheChapterFiveHandOverWithNoFieldChanged) {
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        startFromChapterFiveEnding(*side);
        ASSERT_FALSE(HasFatalFailure());
        const auto fixture = fanren::io::loadGame(fanren::test::chapterFixturePath(assetRoot(), side->file).string());
        ASSERT_TRUE(fixture.ok);
        const auto expected = fanren::test::comparableSaveLines(fixture.value);
        const auto actual = fanren::test::comparableSaveLines(state());
        ASSERT_GT(expected.size(), 20u) << "先验：存档写得出来";
        const auto diff = fanren::test::saveLineDifferences(expected, actual);
        EXPECT_TRUE(diff.empty()) << side->file << " 读进来之后有字段被改了——施工图第 2 节：与第 5 章终局不同的字段"
                                  << "一个也不应有：" << fanren::test::joinLines(diff);
    }
}

// ---------------------------------------------------------------------------
// 第一侧：每一处二选一取第一项；认药一次认对、竹简直指百药园；坊市卖一瓶清毒散、买一张定神符。
// 沿途的闸门验两次（拦得住、开得了）。
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    const Route route = firstSideRoute();
    ASSERT_FALSE(HasFailure()) << "先验：认药与竹简的答案从施工图读得出来";
    GameState& s = state();
    const int tuguAtStart = s.itemCount(kTugu);
    std::vector<std::string> hints;

    const auto before = [&](Step step) {
        switch (step) {
            case Step::Chudu: {
                // 本章入口：南城东门认 ch05.done（施工图第 4 节修补）。撤掉验一次拦得住，放回来验开得了。
                ASSERT_TRUE(travelTo(kMapNancheng));
                const MapObject gate = objectNamed("portal_to_tainan_cun");
                ASSERT_FALSE(gate.name.empty()) << "施工图第 4 节：南城加 portal_to_tainan_cun";
                EXPECT_EQ(gate.property("require_flag"), "ch05.done");
                s.flags.erase("ch05.done");
                EXPECT_FALSE(usePortal("portal_to_tainan_cun")) << "第 5 章没完，东门不该放行";
                EXPECT_EQ(s.mapId, kMapNancheng);
                expectDeniedWith("ch06.block.tainan_cun", "南城东门");
                s.setFlag("ch05.done");
                break;
            }
            case Step::Guaipo:
                // 用词切换之前：面板还说碎银、法门（施工图第 7 节）。
                EXPECT_EQ(std::string(fanren::game::currencyName(s)), "碎银");
                EXPECT_EQ(std::string(fanren::game::magicWord(fanren::game::wordingStage(s))), "法门");
                break;
            case Step::Feixingfu:
            case Step::Jinzhubi:
            case Step::Sanhui: {
                // R-3 提示在前（施工图 3.4、验收 17）：走进去之前，目标行已经说出来了。
                const auto* obj = fanren::rules::currentObjective(app_.data().objectives, s);
                ASSERT_NE(obj, nullptr);
                const char* wantId = step == Step::Feixingfu ? kHintStepFeixing
                                     : step == Step::Jinzhubi ? kHintStepJinzhubi
                                                              : kHintStepSanhui;
                const char* word = step == Step::Feixingfu ? "丹药" : step == Step::Jinzhubi ? "数一遍" : "练熟";
                EXPECT_EQ(obj->id, wantId);
                EXPECT_NE(app_.data().lookupText(obj->textKey).find(word), std::string::npos)
                    << obj->id << " 的提示该含「" << word << "」：" << app_.data().lookupText(obj->textKey);
                hints.push_back(obj->id);
                break;
            }
            case Step::Linggen:
                EXPECT_EQ(fanren::game::CultivationScene::spiritRootLine(s), "") << "E3：还没测过灵根，面板没有那一行";
                break;
            case Step::Maidan:
                ASSERT_TRUE(travelTo(kMapHfg));
                EXPECT_FALSE(WorldScene::npcVisible(s, objectNamed("npc_ye_shishu"))) << "叶师叔在 14 之前不在百机堂";
                break;
            case Step::Lingqu:
                EXPECT_EQ(std::string(fanren::game::bagWord(s)), "物品") << "O1：还没领储物袋";
                break;
            case Step::Juanzong: {
                // 通百药园的口认 ch06.huinuo：此刻还没悔诺，从黄枫谷走过去必须被拦下（局面本身就是反面）。
                ASSERT_TRUE(travelTo(kMapHfg));
                const MapObject gate = objectNamed("portal_to_baiyaoyuan");
                ASSERT_FALSE(gate.name.empty());
                EXPECT_EQ(gate.property("require_flag"), "ch06.huinuo");
                EXPECT_FALSE(usePortal("portal_to_baiyaoyuan")) << "还没接下百药园，那道口不该放行";
                EXPECT_EQ(s.mapId, kMapHfg);
                expectDeniedWith("ch06.block.baiyaoyuan", "通百药园的口");
                break;
            }
            default: break;
        }
    };
    const auto after = [&](Step step) {
        switch (step) {
            case Step::Chudu:
                // 验收 11：两侧同账——1a 之后养精丹 15、灵石 0。
                EXPECT_EQ(s.itemCount(kPill), kPillsAfterChudu) << "验收 11：1a 之后养精丹 15";
                EXPECT_EQ(s.itemCount(kLingshi), 0) << "验收 11：1a 之后灵石 0";
                EXPECT_EQ(s.itemCount(kHuangjing), 0) << "施工偏差 18.4 第 3 条：黄精不论年份全部用掉";
                EXPECT_EQ(s.itemCount(kZishen), 0) << "施工偏差 18.4 第 3 条：紫参不论年份全部用掉";
                EXPECT_EQ(s.itemCount(kTugu), tuguAtStart) << "施工偏差 18.4 第 3 条：土骨花不动";
                EXPECT_EQ(s.bottle.drops, 0) << "施工图 3.2 节点 1：bottle.spend(3)，三滴全用";
                break;
            case Step::Cunkou:
                EXPECT_TRUE(WorldScene::npcVisible(s, objectNamed("npc_wan_xiaoshan")))
                    << "施工偏差 18.3 第 3 条：万小山 visible ch06.wan_met，撞见之后站在村口";
                break;
            case Step::Yishi:
                // 施工图第 5 节：吴九指、黄孝天从节点 7 起在小楼。
                EXPECT_TRUE(WorldScene::npcVisible(s, objectNamed("npc_wu_jiuzhi"))) << "吴九指（7 起）";
                EXPECT_TRUE(WorldScene::npcVisible(s, objectNamed("npc_huang_xiaotian"))) << "黄孝天（7）";
                break;
            case Step::Maidan:
                ASSERT_TRUE(travelTo(kMapHfg));
                EXPECT_TRUE(WorldScene::npcVisible(s, objectNamed("npc_ye_shishu")))
                    << "施工偏差 18.3 第 3 条：叶师叔 visible ch06.rangdan，摆在百机堂内殿";
                break;
            case Step::Guaipo:
                // 验收 12：置后 currencyName == 灵石、magicWord == 法术。
                EXPECT_EQ(s.flag("story.xiuxian_known"), 1);
                EXPECT_EQ(std::string(fanren::game::currencyName(s)), "灵石");
                EXPECT_EQ(std::string(fanren::game::magicWord(fanren::game::wordingStage(s))), "法术");
                EXPECT_EQ(s.mapId, kMapGu) << "施工图 3.1 节点 2：脚本末尾 teleport 太南谷";
                break;
            case Step::Ruhuo: {
                // 谷 → 小楼那道门认 ch06.ruhuo：撤掉验一次拦得住，放回来。
                const MapObject gate = objectNamed("portal_to_sanxiu_lou");
                ASSERT_FALSE(gate.name.empty());
                EXPECT_EQ(gate.property("require_flag"), "ch06.ruhuo");
                const int ruhuo = s.flag("ch06.ruhuo");
                s.flags.erase("ch06.ruhuo");
                tolerateBlocked_ = true;
                EXPECT_FALSE(usePortal("portal_to_sanxiu_lou")) << "还没入伙，小楼不该放行";
                tolerateBlocked_ = false;
                EXPECT_EQ(s.mapId, kMapGu);
                expectDeniedWith("ch06.block.sanxiu_lou", "散修小楼的门");
                s.setFlag("ch06.ruhuo", ruhuo);
                break;
            }
            case Step::Lingshi: {
                // 双首鹜飞过之前，卖符少女那一摊只说一句闲话：不扣、不给、不置旗标（施工偏差 18.3 第 1 条）。
                const auto bag = bagCounts(s);
                const auto flags = s.flags;
                ASSERT_TRUE(fireTrigger("npc_maifu_shaonv"));
                EXPECT_EQ(bagCounts(s), bag) << "8a 之前找少女，不该有买卖";
                EXPECT_EQ(s.flags, flags) << "8a 之前找少女，不该置旗标";
                break;
            }
            case Step::Feixingfu: {
                EXPECT_FALSE(WorldScene::npcVisible(s, objectNamed("npc_caomao_qingnian")))
                    << "施工图 3.1 5b：换完他收摊（hidden_flag ch06.feixingfu）";
                EXPECT_EQ(s.flag("ch06.zhoushu"), 1) << "施工图 3.2 节点 5：咒书是旗标不是物品";
                break;
            }
            case Step::Jinzhubi: {
                // 施工图 3.1 8b：换完她不撤场，再找她只说一句（打探在路径行动里）。
                const MapObject girl = objectNamed("npc_maifu_shaonv");
                EXPECT_TRUE(WorldScene::npcVisible(s, girl)) << "施工图第 5 节：少女不撤场";
                EXPECT_EQ(s.flag("ch06.changchungong"), 1) << "施工图 3.2 节点 8：长春功是旗标不是物品";
                const auto bag = bagCounts(s);
                const auto flags = s.flags;
                ASSERT_TRUE(fireTrigger("npc_maifu_shaonv"));
                EXPECT_EQ(bagCounts(s), bag) << "换过之后再找少女，不该再有买卖";
                EXPECT_EQ(s.flags, flags);
                break;
            }
            case Step::Kuxiu:
                EXPECT_EQ(s.realm, Realm::QiRefining9) << "施工图 3.2 节点 9：realm.advance(9)";
                EXPECT_EQ(s.realmCap, Realm::QiRefining9) << "顺带抬上限到九层";
                EXPECT_EQ(s.maxHp, kNinthLayerHp) << "施工图 1.3：九层气血 132";
                EXPECT_EQ(s.maxMp, kNinthLayerMp) << "施工图 1.3：九层法力 90";
                break;
            case Step::Sanhui:
                EXPECT_EQ(s.mapId, kMapShanqiu) << "施工图 3.1 节点 11：脚本末尾 teleport 山丘";
                break;
            case Step::Xisha:
                EXPECT_EQ(s.mapId, kMapHfg) << "施工图 3.1 节点 12：胜后 teleport 黄枫谷";
                EXPECT_TRUE(s.position == (Point{8, 30})) << "施工偏差 18.3 第 2 条：12 的传送落在迎宾楼的厅里 (8,30)";
                break;
            case Step::Linggen: {
                const std::string line = fanren::game::CultivationScene::spiritRootLine(s);
                EXPECT_NE(line.find(kSpiritRootText), std::string::npos) << "E3 / 验收 13：测过之后面板多一行：" << line;
                break;
            }
            case Step::Lingqu:
                EXPECT_EQ(std::string(fanren::game::bagWord(s)), "储物袋") << "O1：领了储物袋，背包就叫储物袋";
                break;
            case Step::Jinzhi:
                EXPECT_FALSE(WorldScene::npcVisible(s, objectNamed("npc_ma_shibo")))
                    << "施工图第 5 节：马师伯 hidden ch06.renyao";
                break;
            default: break;
        }
    };

    ASSERT_TRUE(playChapter(route, before, after)) << "走不到 ch06.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << "走到了 game_over 那一条" << battleLine();

    // ---- 每一处二选一都取了第一项（施工图第 11 节的取值）----
    EXPECT_EQ(flag("ch06.ruhuo"), 1) << "4 第一项：问清楚再入";
    EXPECT_EQ(flag("ch06.zhongzi"), 1) << "8b 第一项：要种子";
    EXPECT_EQ(flag("ch06.canpian"), 1) << "10a 第一项：先试再换";
    EXPECT_EQ(flag("ch06.jujue"), 1) << "11 第一项：婉拒";
    EXPECT_EQ(flag("ch06.rangdan"), 1) << "14 第一项：要杂务任选";
    EXPECT_EQ(flag("ch06.huinuo"), 1) << "17b 第一项：忍";
    EXPECT_EQ(hints.size(), 3u) << "R-3 三处提示都在走进去之前看到了";

    // ---- ③ 捂着剑符、输（复验整改 16.5 N-L5「能不露就不露」；抽查 N3-L4：第二侧照放、赢）----
    {
        const BattleRecord* wufeng = lastBattle(kMustFight[2]);
        ASSERT_NE(wufeng, nullptr);
        EXPECT_EQ(wufeng->phase, BattlePhase::Lost) << "第一侧 ③ 捂着剑符，这只手打不下吴风" << battleLine();
        EXPECT_EQ(flag("ch06.wufeng"), 2) << "施工图 3.2 节点 16b：输 → 2";
        EXPECT_TRUE(spokeIn(Step::Wufeng, "ch06.wufeng.lost")) << "吴风输的那一句";
        EXPECT_FALSE(spokeIn(Step::Wufeng, "ch06.wufeng.won"));
    }

    // ---- 分支之后的那一句（施工图 3.2 原文里「多一句」的几处）----
    EXPECT_FALSE(spokeIn(Step::Yishi, "ch06.yishi.easy")) << "3.2 节点 7：ruhuo == 2 时吴九指才多一句";
    EXPECT_TRUE(spokeIn(Step::Maiping, "ch06.maiping.tested")) << "3.2 节点 18b：canpian == 1 多一句「他试过那块布」";
    EXPECT_TRUE(spokeIn(Step::Xisha, "ch06.xisha.scrap")) << "3.2 节点 12：jujue == 1 多一句「烧剩的符纸角」";
    EXPECT_TRUE(spokeIn(Step::Zawu, "ch06.zawu.pick")) << "3.2 节点 17a：rangdan == 1「让韩师侄任意挑选」";
    EXPECT_FALSE(spokeIn(Step::Maiping, "ch06.maiping.less")) << "rangdan == 1：不再减那十块";
    // 认药一次认对、竹简直指百药园：三题各答一次，竹简只点一下。
    ASSERT_NE(recordOf(Step::Jinzhi), nullptr);
    EXPECT_EQ(recordOf(Step::Jinzhi)->answers.size(), 3u) << "三题各认一次";
    ASSERT_NE(recordOf(Step::Zawu), nullptr);
    EXPECT_EQ(recordOf(Step::Zawu)->answers.size(), 1u) << "竹简直接点百药园";

    // ---- 战斗 ----
    expectTheThreeMustFightsAllHappened();
    const BattleRecord* xunxin = lastBattle(kMustFight[0]);
    ASSERT_NE(xunxin, nullptr);
    EXPECT_TRUE(spokeIn(Step::Xunxin, xunxin->phase == BattlePhase::Won ? "ch06.xunxin.won" : "ch06.xunxin.lost"))
        << "① 的落点台词按胜负分";
    const BattleRecord* wufeng = lastBattle(kMustFight[2]);
    ASSERT_NE(wufeng, nullptr);
    EXPECT_EQ(flag("ch06.wufeng"), wufeng->phase == BattlePhase::Won ? 1 : 2) << "施工图 3.2 节点 16b：赢 1 / 输 2";

    // ---- 支线：Z1 了结（坊市买来的定神符也算，任务 note 写明）、Z2 攒不够过期（施工图第 6 节）----
    EXPECT_EQ(statusOf("q06_dingshen"), fanren::rules::QuestStatus::Completed) << "Z1：身上有一张定神符";
    EXPECT_EQ(statusOf("q06_zhang"), fanren::rules::QuestStatus::Failed) << "Z2：出谷时不到十块";
    EXPECT_EQ(flag("ch06.zhang_done"), 0);
    EXPECT_FALSE(spokeIn(Step::Xisha, "ch06.xisha.zhang")) << "Z2 没了结，12 不该多那一句";
    EXPECT_EQ(boardRowOf("q06_zhang").detail, BoardScene::kFailedTag);
    EXPECT_EQ(boardRowOf("q06_dingshen").detail, BoardScene::kCompletedTag);

    // ---- 账 ----
    expectEveryStepTookTheDesignDays();
    expectEveryStepMovedTheBagTheDesignSays();
    expectMagicsAndRealmMoveOnlyWhereTheDesignSays();
    expectTheChapterEndState();
    expectGatedWordsArriveWhereTheDesignSays();

    std::cout << "\n[ch06 第一侧通关] " << (s.day - startDay_) << " 天，灵石 " << s.itemCount(kLingshi) << "，"
              << battles_.size() << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterSixEndingFirst);
}

// ---------------------------------------------------------------------------
// 第二侧：每一处取第二项；认药、竹简挨个选项点过去；坊市卖三瓶清毒散攒够十块。
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, SecondSideTakesEveryOtherChoice) {
    startFromChapterFiveEnding(kSecondSide);
    ASSERT_FALSE(HasFatalFailure());
    ASSERT_EQ(state().itemCount(kPill), 0) << "先验：第二侧 0 瓶养精丹（技术债 G-24）";
    const Route route = secondSideRoute();
    GameState& s = state();
    int lingshiBeforeSanhui = -1;
    const auto before = [&](Step step) {
        if (step == Step::Sanhui) lingshiBeforeSanhui = s.itemCount(kLingshi);
    };
    const auto after = [&](Step step) {
        if (step == Step::Chudu) {
            EXPECT_EQ(s.itemCount(kPill), kPillsAfterChudu) << "验收 11：1a 之后养精丹 15（第二侧从 0 补足）";
            EXPECT_EQ(s.itemCount(kLingshi), 0) << "验收 11：1a 之后灵石 0";
        }
    };

    ASSERT_TRUE(playChapter(route, before, after)) << "走不到 ch06.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << "走到了 game_over 那一条" << battleLine();

    // ---- 每一处二选一都取了第二项 ----
    EXPECT_EQ(flag("ch06.ruhuo"), 2) << "4 第二项：立刻入";
    EXPECT_EQ(flag("ch06.zhongzi"), 0) << "8b 第二项：不要种子";
    EXPECT_EQ(flag("ch06.canpian"), 2) << "10a 第二项：直接换";
    EXPECT_EQ(flag("ch06.jujue"), 2) << "11 第二项：直说";
    EXPECT_EQ(flag("ch06.rangdan"), 2) << "14 第二项：只要东西";
    EXPECT_EQ(flag("ch06.huinuo"), 2) << "17b 第二项：追问";

    // ---- ③ 照放剑符、打赢（复验整改 16.5 / 抽查 N3-L4：第一侧捂着剑符输，这一侧赢，两支都有通关走到）----
    {
        const BattleRecord* wufeng = lastBattle(kMustFight[2]);
        ASSERT_NE(wufeng, nullptr);
        EXPECT_EQ(wufeng->phase, BattlePhase::Won) << "第二侧 ③ 照放剑符，该赢" << battleLine();
        EXPECT_EQ(flag("ch06.wufeng"), 1) << "施工图 3.2 节点 16b：赢 → 1";
        EXPECT_TRUE(spokeIn(Step::Wufeng, "ch06.wufeng.won")) << "吴风赢的那一句";
        EXPECT_FALSE(spokeIn(Step::Wufeng, "ch06.wufeng.lost"));
    }

    // ---- 分支之后的那一句 ----
    EXPECT_TRUE(spokeIn(Step::Yishi, "ch06.yishi.easy")) << "3.2 节点 7：ruhuo == 2 时吴九指多一句";
    EXPECT_FALSE(spokeIn(Step::Maiping, "ch06.maiping.tested")) << "canpian == 2：不说「他试过那块布」";
    EXPECT_FALSE(spokeIn(Step::Xisha, "ch06.xisha.scrap")) << "jujue == 2：没有符纸角那一句";
    EXPECT_TRUE(spokeIn(Step::Zawu, "ch06.zawu.lottery")) << "3.2 节点 17a：rangdan == 2「还是抓签」";
    EXPECT_TRUE(spokeIn(Step::Maiping, "ch06.maiping.less")) << "3.2 节点 18b：rangdan == 2 再减十块";
    EXPECT_TRUE(spokeIn(Step::Maiping, "ch06.maiping.cold")) << "3.2 节点 18b：huinuo == 2 送得更少";

    // ---- 认药：选错同一题重来，不置旗标、不扣东西（验收 19 在通关里走一遍）；竹简四条挨个点 ----
    const StepRecord* jinzhi = recordOf(Step::Jinzhi);
    ASSERT_NE(jinzhi, nullptr);
    const std::vector<int> right = designQuizAnswers();
    ASSERT_EQ(right.size(), 3u);
    // 挨个点：每题从上一题停下的下一项接着点，点到对的那一项为止。
    std::vector<int> expectAnswers;
    int cursor = 0;
    for (const int answer : right) {
        for (;;) {
            const int pick = cursor % 4;
            ++cursor;
            expectAnswers.push_back(pick);
            if (pick == answer) break;
        }
    }
    EXPECT_EQ(jinzhi->answers, expectAnswers) << "选错了就同一题重来，答对才出下一题";
    int wrong = 0;
    for (const std::string& key : jinzhi->keys) wrong += key.rfind("ch06.jinzhi.wrong", 0) == 0 ? 1 : 0;
    EXPECT_EQ(wrong, static_cast<int>(expectAnswers.size()) - 3) << "每选错一次，马师伯冷笑一声";
    const StepRecord* zawu = recordOf(Step::Zawu);
    ASSERT_NE(zawu, nullptr);
    EXPECT_EQ(zawu->answers.size(), static_cast<std::size_t>(designSlipAnswer() + 1))
        << "竹简挨个点：前几条各回一句，点到百药园才走得下去";
    for (const char* key : {"ch06.zawu.no_tree", "ch06.zawu.no_shen", "ch06.zawu.no_mei"}) {
        EXPECT_TRUE(spokeIn(Step::Zawu, key)) << "施工图 3.2 节点 17a：前三个于执事各回一句（" << key << "）";
    }

    // ---- 战斗 ----
    expectTheThreeMustFightsAllHappened();
    const BattleRecord* wufeng = lastBattle(kMustFight[2]);
    ASSERT_NE(wufeng, nullptr);
    EXPECT_EQ(flag("ch06.wufeng"), wufeng->phase == BattlePhase::Won ? 1 : 2);

    // ---- 支线：Z2 了结（施工偏差 18.2 第 5 条：出谷前数一遍）、Z1 过期 ----
    ASSERT_GE(lingshiBeforeSanhui, 10) << "先验：三瓶清毒散卖出去够十块";
    EXPECT_EQ(flag("ch06.zhang_done"), 1);
    EXPECT_TRUE(spokeIn(Step::Xisha, "ch06.xisha.zhang")) << "施工图第 6 节 Z2：12 搜身时多一句";
    EXPECT_EQ(statusOf("q06_zhang"), fanren::rules::QuestStatus::Completed);
    EXPECT_EQ(statusOf("q06_dingshen"), fanren::rules::QuestStatus::Failed) << "Z1：出了谷，桌子留在楼里";
    EXPECT_EQ(boardRowOf("q06_dingshen").detail, BoardScene::kFailedTag);
    // 章末灵石 = 出谷前攒的 ＋ 搜身 50 ＋ 送物（huinuo 2、rangdan 2：30 − 10）。
    const Delivery g = designDelivery(2, 2);
    EXPECT_EQ(s.itemCount(kLingshi), lingshiBeforeSanhui + kSpoilsLingshi + g.lingshi);

    expectEveryStepTookTheDesignDays();
    expectEveryStepMovedTheBagTheDesignSays();
    expectMagicsAndRealmMoveOnlyWhereTheDesignSays();
    expectTheChapterEndState();
    expectGatedWordsArriveWhereTheDesignSays();

    std::cout << "\n[ch06 第二侧通关] " << (s.day - startDay_) << " 天，灵石 " << s.itemCount(kLingshi) << "，"
              << battles_.size() << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterSixEndingSecond);
}

// ---------------------------------------------------------------------------
// 三场必打一场也绕不过去（验收 15 走一遍的另一半），每一场按它自己的形状问：
//   ① 踏入型、挂在摊位区的口子上（施工图 3.1：「换完飞行符出摊位区必踩」）——人在摊位区里，
//     不踩那两格就走不到小楼的门；
//   ② 踏入型、荒丘没有门（施工图第 4 节：只靠 teleport 进出）——不踩它哪儿也去不了；
//   ③ 按确认的（传功阁）——不打它，下一节点（百机堂柜台）按下去不响，也不在别处把仗补上。
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, NoneOfTheThreeMustFightsCanBeWalkedAround) {
    const auto walkUpTo = [&](Step stop) {
        startFromChapterFiveEnding(kFirstSide);
        ASSERT_FALSE(HasFatalFailure());
        static Route route;
        route = firstSideRoute();
        route_ = &route;
        for (Step s : stepsFor(route)) {
            if (s == stop) break;
            ASSERT_TRUE(runStep(s)) << "走到 " << infoOf(stop).node << " 之前就卡在 " << infoOf(s).node << battleLine();
        }
    };

    // ①
    walkUpTo(Step::Xunxin);
    ASSERT_FALSE(HasFatalFailure());
    {
        ASSERT_EQ(state().mapId, kMapGu);
        const MapObject hook = objectNamed("trigger_ye_xunxin");
        ASSERT_FALSE(hook.name.empty());
        ASSERT_TRUE(WorldScene::triggerReady(state(), hook)) << "先验：换完飞行符，① 的挂点备着";
        const MapObject door = objectNamed("portal_to_sanxiu_lou");
        ASSERT_FALSE(door.name.empty());
        std::vector<Point> besideDoor;
        for (int dx = 0; dx < std::max(1, door.width); ++dx) {
            for (const Point& d : kDirections) {
                const Point p{door.position.x + dx + d.x, door.position.y + d.y};
                if (standable(p, /*avoidTriggers=*/false)) besideDoor.push_back(p);
            }
        }
        ASSERT_FALSE(besideDoor.empty()) << "先验：小楼门口有站得住的格子";
        bool anyRoute = false;
        for (const Point& p : besideDoor) {
            EXPECT_TRUE(routeTo(p, /*avoidTriggers=*/true).empty())
                << "人在摊位区里，不踩 ① 的挂点也走得到小楼门口 (" << p.x << "," << p.y << ")——这一场绕得过去";
            anyRoute = anyRoute || !routeTo(p, /*avoidTriggers=*/false).empty();
        }
        EXPECT_TRUE(anyRoute) << "反面：踩着挂点走是通的（上一条不是因为门口本来就走不到）";
    }

    // ②
    walkUpTo(Step::Xisha);
    ASSERT_FALSE(HasFatalFailure());
    EXPECT_EQ(state().mapId, kMapShanqiu);
    for (const MapObject& object : map().objects) {
        EXPECT_NE(object.type, "portal") << "荒丘上出现了门 " << object.name << "：施工图第 4 节只用 teleport 进出";
    }

    // ③
    walkUpTo(Step::Wufeng);
    ASSERT_FALSE(HasFatalFailure());
    const std::size_t fought = battles_.size();
    tolerateBlocked_ = true;
    beginStep(Step::Zawu);
    static_cast<void>(attemptStep(Step::Zawu));
    endStep();
    tolerateBlocked_ = false;
    EXPECT_EQ(flag("ch06.zawu"), 0) << "没和吴风切磋就在百机堂领到了活：③ 绕得过去";
    EXPECT_EQ(battles_.size(), fought) << "跳过的那一步不该在别处把仗补上";
}

// ---------------------------------------------------------------------------
// 输了之后路怎么走（施工图 8.2「败」一列、3.2 节点 6 / 12 / 16b）：
//   ①「打输了是青颜真人到场叫停」、③「赢 / 输只改 ch06.wufeng = 1 / 2」——都往下走；
//   ②「败：game over（原著这一仗输了就是死）」。
// 意图玩家 ① ② 都打赢；③ 第一侧捂着剑符输、第二侧照放赢（上面两条通关，复验整改 16.5 / 抽查 N3-L4）。
// ① 输的那一支（连同 ③）另换一只「一招不出、只防御」的手走到。
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, TheTwoFightsThatSpareHimGoOnWhenHeLoses) {
    startFromChapterFiveEnding(kSecondSide);
    ASSERT_FALSE(HasFatalFailure());
    throwFights_ = {kMustFight[0], kMustFight[2]};
    const Route route = secondSideRoute();
    ASSERT_TRUE(playChapter(route)) << "① ③ 输了也该走得到 ch06.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app_.quitRequested()) << battleLine();
    const BattleRecord* xunxin = lastBattle(kMustFight[0]);
    const BattleRecord* wufeng = lastBattle(kMustFight[2]);
    ASSERT_NE(xunxin, nullptr);
    ASSERT_NE(wufeng, nullptr);
    EXPECT_EQ(xunxin->phase, BattlePhase::Lost) << "先验：① 这一回是输的" << battleLine();
    EXPECT_EQ(wufeng->phase, BattlePhase::Lost) << "先验：③ 这一回是输的" << battleLine();
    EXPECT_TRUE(spokeIn(Step::Xunxin, "ch06.xunxin.lost")) << "3.2 节点 6：输了是青颜真人叫停";
    EXPECT_FALSE(spokeIn(Step::Xunxin, "ch06.xunxin.won"));
    EXPECT_TRUE(spokeIn(Step::Xunxin, "ch06.xunxin.stop")) << "3.2 节点 6：输赢都是青颜真人叫停";
    EXPECT_EQ(flag("ch06.xunxin"), 1) << "施工图 3.2 节点 6：两条结局都不置分岔旗标，只置完成旗标";
    EXPECT_EQ(flag("ch06.wufeng"), 2) << "施工图 3.2 节点 16b：输 → 2";
    EXPECT_TRUE(spokeIn(Step::Wufeng, "ch06.wufeng.lost")) << "吴风输的那一句";
    EXPECT_FALSE(spokeIn(Step::Wufeng, "ch06.wufeng.won"));
    EXPECT_GE(wufeng->hpAfter, 1) << "败不致命：气血至少留 1";
    expectTheThreeMustFightsAllHappened();
    expectEveryStepTookTheDesignDays();
    expectEveryStepMovedTheBagTheDesignSays();
}

TEST_F(Ch06Walkthrough, LosingTheAmbushOnTheHillsEndsTheGame) {
    startFromChapterFiveEnding(kFirstSide);
    ASSERT_FALSE(HasFatalFailure());
    throwFights_ = {kMustFight[1]};
    const Route route = firstSideRoute();
    tolerateBlocked_ = true;
    EXPECT_FALSE(playChapter(route)) << "② 输了不该还走得到章末";
    tolerateBlocked_ = false;
    const BattleRecord* xisha = lastBattle(kMustFight[1]);
    ASSERT_NE(xisha, nullptr) << battleLine();
    EXPECT_EQ(xisha->phase, BattlePhase::Lost) << "先验：② 这一回是输的" << battleLine();
    EXPECT_TRUE(app_.quitRequested()) << "施工图 8.2 ②：败即 game over";
    EXPECT_EQ(flag("ch06.xisha"), 0) << "输了不置完成旗标";
    EXPECT_EQ(state().mapId, kMapShanqiu) << "输了不该被送去黄枫谷";
    EXPECT_TRUE(spokeIn(Step::Xisha, "ch06.xisha.lost"));
}

// ---------------------------------------------------------------------------
// 那只手先证明自己稳：同一存档整章跑三遍，每一场的落点、血、法力、药与终局存档都一字不差
// ---------------------------------------------------------------------------
TEST_F(Ch06Walkthrough, TheHandIsSteadyThreeRunsFromTheSameSaveEndAlike) {
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        std::vector<std::string> firstLines;
        std::string firstBattles;
        for (int run = 0; run < 3; ++run) {
            startFromChapterFiveEnding(*side);
            ASSERT_FALSE(HasFatalFailure());
            const Route route = side == &kFirstSide ? firstSideRoute() : secondSideRoute();
            ASSERT_TRUE(playChapter(route)) << side->file << " 第 " << (run + 1) << " 遍卡在「" << stuckAt_ << "」"
                                            << battleLine();
            const std::vector<std::string> lines = fanren::test::comparableSaveLines(state());
            ASSERT_GT(lines.size(), 20u);
            if (run == 0) {
                firstLines = lines;
                firstBattles = battleLine();
                continue;
            }
            EXPECT_EQ(battleLine(), firstBattles) << side->file << " 第 " << (run + 1) << " 遍的仗与第 1 遍不同";
            const auto diff = fanren::test::saveLineDifferences(firstLines, lines);
            EXPECT_TRUE(diff.empty()) << side->file << " 第 " << (run + 1) << " 遍的终局与第 1 遍不同："
                                      << fanren::test::joinLines(diff);
        }
        std::cout << "[ch06 那只手] " << side->file << " 三遍一致：" << firstBattles << std::endl;
    }
}

// ---------------------------------------------------------------------------
// 先吃哪一瓶、门槛多低（技术债 G-26 / G-11 那一类敏感）：换五只手各走两侧，打印每一场的落点与用药
// ---------------------------------------------------------------------------
// 判的只有施工图写死的那两件：② 是生死仗，**任何一只手**都不许在这里 game over 之外的地方卡住，
// 且意图玩家两侧都走得完（上面两条通关已判）。其余全部打印，结论写进测试路的回复。
struct SweepHand {
    const char* name;
    int healAt;
    bool doomed;
    bool salveFirst;
};

constexpr SweepHand kSweepHands[] = {
    {"意图玩家（养精丹在前、五成）", 50, false, false},
    {"先吃金疮药（五成）", 50, false, true},
    {"门槛四成", 40, false, false},
    {"门槛六成", 60, false, false},
    {"第 4 章那只（六成＋再挨一轮就死）", 60, true, false},
};

TEST_F(Ch06Walkthrough, TheSweepOfWhichPillFirstAndHowLowToWait) {
    std::ostringstream table;
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        for (const SweepHand& h : kSweepHands) {
            startFromChapterFiveEnding(*side);
            ASSERT_FALSE(HasFatalFailure());
            fanren::test::HandPolicy p = intentPolicy();
            p.healAtPercent = h.healAt;
            p.healWhenDoomed = h.doomed;
            if (h.salveFirst) p.pills = {kSalve, kPill};
            policy_ = p;
            const Route route = side == &kFirstSide ? firstSideRoute() : secondSideRoute();
            tolerateBlocked_ = true;
            const bool done = playChapter(route);
            tolerateBlocked_ = false;
            const bool dead = app_.quitRequested();
            table << "\n  " << (side == &kFirstSide ? "第一侧" : "第二侧") << " · " << h.name << "："
                  << (done ? "走完" : dead ? "game over" : "卡在「" + stuckAt_ + "」") << battleLine();
            if (dead) {
                // game over 只许出在 ② 输了那一处（施工图 8.2：生死仗）。
                ASSERT_FALSE(battles_.empty());
                EXPECT_EQ(battles_.back().id, kMustFight[1]) << h.name << "：game over 不在 ② 上";
                // 这一侧的 Application 已经 requestQuit，换一个新的接着扫。
                app_.shutdown();
                auto ready = app_.init(assetRoot(), /*headless=*/true);
                ASSERT_TRUE(ready.ok) << ready.error;
            }
        }
    }
    policy_ = intentPolicy();
    std::cout << "\n[ch06 那只手的扫描]" << table.str() << std::endl;
}

// ===========================================================================
// 下面几条不驱动：直接读数据与脚本，判据写死成施工图原文
// ===========================================================================
std::map<std::string, std::string> scriptsUnder(const std::string& root, const std::string& sub) {
    std::map<std::string, std::string> out;
    const fs::path base = fs::path(root) / "scripts";
    const fs::path dir = base / sub;
    if (!fs::exists(dir)) return out;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file() || entry.path().extension() != ".lua") continue;
        out[fs::relative(entry.path(), base).generic_string()] = readFile(entry.path());
    }
    return out;
}

class Ch06Acceptance : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        chapterScripts_ = scriptsUnder(root_, "ch06");
        allScripts_ = scriptsUnder(root_, "");
        ASSERT_GE(chapterScripts_.size(), 26u) << "先验：scripts/ch06/ 读得到（26 处挂点，分母不能塌）";
    }
    void TearDown() override { app_.shutdown(); }

    MapObject objectOn(const std::string& mapId, const std::string& name) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok || app_.currentMap() == nullptr) return MapObject{};
        for (const MapObject& object : app_.currentMap()->objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    // 所有 key 以 ch06. 开头、或文件名以 ch06 开头的文案（key → 正文）。
    std::map<std::string, std::string> chapterTexts() {
        std::map<std::string, std::string> out;
        for (const auto& entry : fs::directory_iterator(fs::path(root_) / "data" / "text")) {
            if (entry.path().extension() != ".json") continue;
            const std::string file = entry.path().filename().string();
            const std::string body = readFile(entry.path());
            const bool chapterFile = file.rfind("ch06", 0) == 0;
            for (const auto& [key,value] : fanren::test::textKeyValues(body)) {
                if (chapterFile || key.rfind("ch06.", 0) == 0) out[key]=value;
            }
        }
        return out;
    }

    Application app_;
    std::string root_;
    std::map<std::string, std::string> chapterScripts_;
    std::map<std::string, std::string> allScripts_;
};

// ===========================================================================
// 验收 6：禁词（施工图 12.1 / 12.2）
// ===========================================================================
// 12.1：LexiconTests 表一含这十个词，章号照 12.1。读 tests/LexiconTests.cpp 的字——它是协调者的那张表，
// 本条只看它有没有照 12.1 补齐，不改它。
TEST_F(Ch06Acceptance, No6_TheLexiconTableCarriesTheTenWordsOfSectionTwelveOne) {
    const std::string lexicon = readFile(fs::path(root_) / "tests" / "LexiconTests.cpp");
    ASSERT_GT(lexicon.size(), 1000u) << "先验：读得到 tests/LexiconTests.cpp";
    const std::vector<std::pair<const char*, int>> kWords = {
        {"岳麓殿", 153}, {"地火屋", 215}, {"血色试炼", 161}, {"血禁", 160}, {"南宫婉", 209},
        {"李化元", 212}, {"青元剑诀", 213}, {"陈巧倩", 273}, {"大衍诀", 230}, {"妖丹", 344}};
    for (const auto& [word, chapter] : kWords) {
        const std::string row = std::string("{\"") + word + "\", " + std::to_string(chapter) + ",";
        EXPECT_NE(lexicon.find(row), std::string::npos)
            << "LexiconTests 表一没有「" << word << "」首见 ch" << chapter << " 这一行（施工图 12.1）";
    }
}

// 12.2：章内先后按 key 前缀逐条断言。文案 key 的场景段（ch06.<场景>.<用途>）记的就是「这一句在哪一节读到」；
// 下面这张表把每一种前缀钉到施工图 3.1 / 3.4 的次序上（目标链第 N 步，1a = 1 … 18b = 26）。
// **表里查不到的 key 直接判红**——新加一条文案而不给它登记节点，等于绕开了这一层检查。
//   · NPC 闲话、路径行动：这个人**最早**在场、说得出这句话的那一步（照 3.1 / 16.2 的 when）；
//   · 目标链第 N 步的文案在第 N−1 步做完之后上屏：按 N−1 算（第 1 步按 0：第 5 章刚结束）；
//   · 物品描述：给出它的那一步；地名：进那张图的那一步；拦路话：走到那道门就看得见的那一步。
struct KeyOrder {
    const char* prefix;
    int order;
};

const std::vector<KeyOrder>& keyOrders() {
    static const std::vector<KeyOrder> kTable = {
        // 地名（进那张图时横幅上那一行）与拦路话（走到那道门就看得见；施工图第 4 节）
        {"ch06.map.tainan_cun.", 0},   {"ch06.map.tainan_gu.", 3},      {"ch06.map.sanxiu_lou.", 5},
        {"ch06.map.shanqiu.", 16},     {"ch06.map.huangfenggu.", 17},   {"ch06.map.baiyaoyuan.", 24},
        {"ch06.block.tainan_cun", 0},  {"ch06.block.sanxiu_lou", 4},    {"ch06.block.baiyaoyuan", 17},
        // 坊市在广场上，入伙之后就走得到（它卖的丹砂、定神符，描述在货架上就看得见）
        {"ch06.shop.", 5},
        // 主线挂点（3.1 表，按 3.4 目标链的次序）
        {"ch06.chudu.", 1},   {"ch06.cunkou.", 2},   {"ch06.guaipo.", 3},    {"ch06.qingyan.", 4},
        {"ch06.ruhuo.", 5},   {"ch06.lingshi.", 6},  {"ch06.feixingfu.", 7}, {"ch06.xunxin.", 8},
        {"ch06.battle.yejia_xunxin.", 8},            {"ch06.yishi.", 9},     {"ch06.shuangshou.", 10},
        {"ch06.jinzhubi.", 11}, {"ch06.zhifu.", 12}, {"ch06.kuxiu.", 13},    {"ch06.canpian.", 14},
        {"ch06.bilu.", 15},   {"ch06.sanhui.", 16},  {"ch06.xisha.", 17},    {"ch06.battle.shanqiu_xisha.", 17},
        {"ch06.battle.huangyi.", 17},                {"ch06.battle.tujia.", 17},
        {"ch06.linggen.", 18}, {"ch06.maidan.", 19}, {"ch06.dadian.", 20},   {"ch06.lingqu.", 21},
        {"ch06.wufeng.", 22}, {"ch06.battle.wufeng_qiecuo.", 22},            {"ch06.zawu.", 23},
        {"ch06.juanzong.", 24}, {"ch06.jinzhi.", 25}, {"ch06.maiping.", 26},
        // NPC 闲话（3.1 表下「NPC 挂点不在此表」那一串）：这个人在场、脚本走得到这一句的最早那一步
        {"ch06.npc.laonong.", 1},          {"ch06.npc.wan.", 2},
        {"ch06.npc.qingyan.cool", 4},      {"ch06.npc.qingyan.after", 8},
        {"ch06.npc.caomao.", 5},           {"ch06.npc.shaonv.idle", 6},      {"ch06.npc.shaonv.after", 11},
        // 复验整改 16.5（N-L1、N-L7）：① 之前一伙人都在广场上，楼里只有苦桑（kusang.out）；青纹 ① 那天才站到议事屋门口。
        {"ch06.npc.qingwen.wait", 8},      {"ch06.npc.qingwen.kind", 9},
        {"ch06.npc.kusang.out", 5},        {"ch06.npc.kusang.upstairs", 8},  {"ch06.npc.kusang.chant", 9},
        {"ch06.npc.kusang.fu", 12},
        // 王师叔 16b（ch06.wufeng）之后才站到大殿前，「等传唤」那句随之删掉（校对整改 16.4 LOW-7）
        {"ch06.npc.wang.go", 22},
        {"ch06.npc.ye.busy", 19},          {"ch06.npc.ye.", 24},
        // 路径行动（施工图 16.2 的 when）
        {"ch06.path.cunmin_dating.", 1}, {"ch06.path.wan_dating.", 2},     {"ch06.path.dansha_qiugou.", 6},
        {"ch06.path.shaonv_dating.", 11}, {"ch06.path.kusang_dating.", 12}, {"ch06.path.hu_dating.", 9},
        {"ch06.path.heimu_dating.", 9},  {"ch06.path.wujiuzhi_qiecuo.", 9}, {"ch06.battle.qiecuo_wujiuzhi.", 9},
        // wang_dating 的 when 随王师叔在场旗标改成 ch06.wufeng（校对整改 16.4 LOW-7 连带；原 20）
        {"ch06.path.wang_dating.", 22},  {"ch06.path.yu_dating.", 23},     {"ch06.path.waimen_qiecuo.", 21},
        {"ch06.battle.huangfenggu_qiecuo.", 21},
        // 支线（第 6 节：Z2 在 5a 挂起、Z1 在 9a）
        // Z2 改到 8b 之后接（校对整改 16.4 HIGH-3：接取谓词 ch06.jinzhubi），它的字从 8b 之后才上屏。
        {"ch06.quest.zhang.", 11}, {"ch06.quest.dingshen.", 12},
        // 物品与法术描述：给出它的那一步（E9、10.1）
        {"magic.desc.magic_tianyan_shu", 2},     {"magic.desc.magic_liusha_shu", 13},
        {"magic.desc.magic_bingdong_shu", 13},   {"item.desc.story_jinfa_fu", 5},
        {"item.desc.talisman_feixing_fu", 7},    {"item.desc.material_dansha", 5},
        {"item.desc.talisman_dingshen_fu", 5},   {"item.desc.story_jinzhu_bi", 11},
        {"item.desc.material_qixingcao_zhongzi", 11}, {"item.desc.story_fabao_canpian", 14},
        {"item.desc.story_qingxi_bilu", 14},     {"item.desc.story_shengxianling", 15},
        {"item.desc.story_qingye_faqi", 21},     {"item.desc.weapon_lieyang_jian", 21},
        {"item.desc.weapon_lengyue_dao", 21},    {"item.desc.story_chuwudai", 21},
        {"item.desc.story_yaoyuan_yupai", 24},   {"item.desc.story_mupai", 25},
    };
    return kTable;
}

int orderOfKey(const std::string& key) {
    // 目标链：第 N 步的文案在第 N−1 步做完之后上屏。
    static const std::regex kObjective("^objective\\.ch06\\.n([0-9]+)_");
    std::smatch m;
    if (std::regex_search(key, m, kObjective)) return std::stoi(m[1]) - 1;
    int best = -1;
    std::size_t bestLength = 0;
    for (const KeyOrder& k : keyOrders()) {
        const std::string prefix = k.prefix;
        if (key.rfind(prefix, 0) == 0 && prefix.size() > bestLength) {
            best = k.order;
            bestLength = prefix.size();
        }
    }
    return best;
}

TEST_F(Ch06Acceptance, No6_EachGatedWordStaysOutOfTheKeysBeforeItsNode) {
    const std::map<std::string, std::string> texts = chapterTexts();
    ASSERT_GT(texts.size(), 200u) << "先验：施工图验收 6「分母先验 > 200 条」";
    // 表的自检：同一个口径对「5a 的一句提到灵石」「2 的一句提到灵石」看得出差别。
    ASSERT_EQ(orderOfKey("ch06.lingshi.two"), 6);
    ASSERT_EQ(orderOfKey("ch06.guaipo.walk"), 3);
    ASSERT_EQ(orderOfKey("objective.ch06.n07_feixingfu"), 6);
    for (const auto& [key, value] : texts) {
        const int order = orderOfKey(key);
        EXPECT_GE(order, 0) << key << " 不在节点表里：新加的文案要登记它在哪一节读到";
        if (order < 0) continue;
        // 「升仙令」：节点 1-10a（次序 ≤ 14）不含；「升仙大会」「升仙会」不受此限。
        if (order <= 14) {
            EXPECT_EQ(value.find("升仙令"), std::string::npos) << key << "（节点次序 " << order << "）在 10b 之前说了「升仙令」：" << value;
        }
        // 「灵石」：节点 1-4（次序 ≤ 5）不含。
        if (order <= 5) {
            EXPECT_EQ(value.find("灵石"), std::string::npos) << key << "（节点次序 " << order << "）在 5a 之前说了「灵石」：" << value;
        }
        // 「燕」只在 ch06.shuangshou.* 里出现（少女的文案不含「燕」）。
        if (key.rfind("ch06.shuangshou.", 0) != 0) {
            EXPECT_EQ(value.find("燕"), std::string::npos) << key << " 说了「燕」：燕家只留在 8a 那一幕";
        }
        // 「松纹」0 处；「中阶灵石」按第 7 节不说；「真元」「定颜丹」「血色试炼」「岳麓殿」表一拦着，这里再钉一遍。
        for (const char* word : {"松纹", "中阶灵石", "真元", "定颜丹", "血色试炼", "岳麓殿", "血禁", "李化元", "妖丹"}) {
            EXPECT_EQ(value.find(word), std::string::npos) << key << " 里有「" << word << "」（施工图 12.1 / 12.2 / 第 7 节）";
        }
    }
    // 正向：这几个词本章确实说了，而且头一回就说在施工图写的那一节。
    std::map<std::string, int> first;
    for (const auto& [key, value] : texts) {
        const int order = orderOfKey(key);
        for (const char* word : {"升仙令", "灵石"}) {
            if (value.find(word) == std::string::npos) continue;
            if (!first.count(word) || order < first[word]) first[word] = order;
        }
    }
    EXPECT_EQ(first["升仙令"], 15) << "「升仙令」头一回该在 10b";
    EXPECT_EQ(first["灵石"], 6) << "「灵石」头一回该在 5a";
}

// 16.4 MEDIUM-1（校对之后的裁决）：「12.2 补一条断言：ui.chapter.06.* 不含『升仙』」——章节卡在第 6 章一开场就上屏，
// 比 10b 早得多；它在 data/text/ui.json 里、key 不以 ch06. 开头，上一条的扫描范围够不着它。
TEST_F(Ch06Acceptance, No6_TheChapterCardSaysNoShengxianBeforeTheTokenHasAName) {
    const std::string ui = readFile(fs::path(root_) / "data" / "text" / "ui.json");
    int keys = 0;
    for (const auto& [key,value] : fanren::test::textKeyValues(ui)) {
        if (key.rfind("ui.chapter.06.",0)!=0) continue;
        ++keys;
        EXPECT_EQ(value.find("升仙"), std::string::npos) << key << "：章节卡在 10b 之前就说了「升仙」：" << value;
    }
    EXPECT_GE(keys, 2) << "先验：ui.chapter.06.numeral / .title 读得到（施工图 E8）";
    EXPECT_NE(app_.data().lookupText("ui.chapter.06.title"), "ui.chapter.06.title") << "章节卡的章名查不到";
}

// 12.2「天眼术：本章 magic.learn("magic_tianyan_shu") 恰 1 处（1b）」在 Ch06SliceTests 验收 8；
// 「黄枫谷：节点 10b 之前不与升仙令同句」由上一条的「10b 之前一个升仙令也没有」蕴含。

// 校对整改 16.4（HIGH-4）新增，校对给复验的判据 4：吴九指节点 7 才头一回见面（议事那一场的戏全建立在「生人」上），
// 所以 ① 的友军里没有他、换成节点 4 已引见的熊大力；节点 7 之前任何文案（含 ① 的战斗开场）都不点他的名。
TEST_F(Ch06Acceptance, No6_WuJiuzhiIsNotInTheFirstFightAndNotNamedBeforeSeven) {
    const fanren::core::BattleSetup* setup = app_.battleSetup("b06_yejia_xunxin");
    ASSERT_NE(setup, nullptr) << "先验：读得到 ① 的编成";
    std::multiset<std::string> allies;
    for (const fanren::core::BattleUnitSpec& unit : setup->units) {
        if (unit.ally) allies.insert(unit.roleId);
    }
    EXPECT_EQ(allies, (std::multiset<std::string>{"qingwen_daoshi", "hei_mu", "xiong_dali"}))
        << "施工图 8.2 ①（校对整改 16.4）：友军青纹道士、黑木、熊大力";
    EXPECT_EQ(allies.count("wu_jiuzhi"), 0u) << "① 的友军里不该有吴九指：他节点 7 才头一回见面";
    const std::map<std::string, std::string> texts = chapterTexts();
    // 友军都是节点 4 已经引见过的人：入伙那几句里点过他们的名。
    std::string ruhuo;
    for (const auto& [key, value] : texts) {
        if (key.rfind("ch06.ruhuo.", 0) == 0) ruhuo += value;
    }
    for (const char* name : {"青纹", "黑木", "熊大力"}) {
        EXPECT_NE(ruhuo.find(name), std::string::npos) << name << "：① 的友军得是节点 4 引见过的人";
    }
    // 节点 7 之前（目标链次序 < 9）一个「吴九指」也没有；节点 7 那一场确实点了他的名（上面的「没有」才有分量）。
    bool namedAtSeven = false;
    for (const auto& [key, value] : texts) {
        const int order = orderOfKey(key);
        if (order < 0) continue;
        if (order < 9) {
            EXPECT_EQ(value.find("吴九指"), std::string::npos)
                << key << "（节点次序 " << order << "）在节点 7 之前点了吴九指的名：" << value;
        }
        if (key.rfind("ch06.yishi.", 0) == 0 && value.find("吴九指") != std::string::npos) namedAtSeven = true;
    }
    EXPECT_TRUE(namedAtSeven) << "先验：节点 7 的文案里点了吴九指的名";
}

// ===========================================================================
// 验收 15：必打 3 场绕不过去——直接读数据
// ===========================================================================
// 判据原文：「三个编成 id 各只被一个脚本调用、挂点在 3.1 表、set_flag 在目标链 done_flag 序列里且是下一步的 guard」。
// 另加两条这张判据背后的意思：那场仗在脚本里是**无条件**的一句（顶格写），置完成旗标的每一句都排在它后面；
// 完成旗标只有这一个脚本置。以及施工图第 4 节「每一处挂战斗的格子前面都有存档点」。
struct MustFightHook {
    const char* mark;
    const char* battleId;
    const char* mapId;
    const char* hook;
    const char* nextMap;
    const char* nextHook;
};

const std::vector<MustFightHook>& mustFightHooks() {
    // 施工图 3.1 表「战斗」一列 ＋ 3.4 目标链的次序（下一节点）。
    static const std::vector<MustFightHook> kHooks = {
        {"①", "b06_yejia_xunxin", kMapGu, "trigger_ye_xunxin", kMapLou, "trigger_yishi"},
        {"②", "b06_shanqiu_xisha", kMapShanqiu, "trigger_xisha", kMapHfg, "trigger_ce_linggen"},
        {"③", "b06_wufeng_qiecuo", kMapHfg, "trigger_wufeng", kMapHfg, "trigger_zawu"},
    };
    return kHooks;
}

TEST_F(Ch06Acceptance, No15_EachOfTheThreeMustFightsIsCalledOnceFromItsHookAndGuardsTheNextNode) {
    std::vector<std::string> doneFlags;
    std::vector<std::string> targets;
    for (const fanren::core::Objective& o : app_.data().objectives) {
        if (o.chapter != 6) continue;
        doneFlags.push_back(o.doneFlag);
        targets.push_back(o.targetObject);
    }
    ASSERT_EQ(doneFlags.size(), 26u) << "先验：第 6 章目标链 26 步（施工图 3.4）";
    for (const MustFightHook& h : mustFightHooks()) {
        std::vector<std::string> callers;
        const std::string call = std::string("battle(\"") + h.battleId + "\")";
        for (const auto& [path, source] : allScripts_) {
            if (codeOnly(source).find(call) != std::string::npos) callers.push_back(path);
        }
        ASSERT_EQ(callers.size(), 1u) << h.mark << " " << h.battleId << " 该只被一个脚本调用";
        const MapObject hook = objectOn(h.mapId, h.hook);
        ASSERT_FALSE(hook.name.empty()) << h.mapId << " 上没有 " << h.hook;
        EXPECT_EQ(hook.property("script"), callers[0]) << h.mark << "：" << h.battleId << " 不是从 " << h.hook << " 打的";
        const std::string setFlag = hook.property("set_flag");
        ASSERT_FALSE(setFlag.empty()) << h.hook << " 没写 set_flag";
        const auto at = std::find(doneFlags.begin(), doneFlags.end(), setFlag);
        ASSERT_NE(at, doneFlags.end()) << h.mark << "：" << setFlag << " 不在目标链的 done_flag 序列里";
        const std::size_t index = static_cast<std::size_t>(at - doneFlags.begin());
        EXPECT_EQ(targets[index], h.hook) << h.mark << "：目标链指着别的挂点";
        ASSERT_LT(index + 1, targets.size()) << h.mark << " 后面没有下一步了";
        EXPECT_EQ(targets[index + 1], h.nextHook) << h.mark << "：目标链的下一步不是施工图 3.4 的下一节点";
        const MapObject next = objectOn(h.nextMap, h.nextHook);
        ASSERT_FALSE(next.name.empty()) << h.nextMap << " 上没有 " << h.nextHook;
        EXPECT_EQ(next.property("guard_flag"), setFlag)
            << h.mark << "：下一节点 " << h.nextHook << " 的 guard 不认 " << setFlag << "——这一场绕得过去";

        const std::vector<std::string> lines = linesOf(allScripts_[callers[0]]);
        int battleLine = -1;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const std::string code = codeOf(lines[i]);
            if (code.find(call) == std::string::npos) continue;
            battleLine = static_cast<int>(i);
            EXPECT_TRUE(!code.empty() && code[0] != ' ' && code[0] != '\t')
                << callers[0] << " 第 " << (i + 1) << " 行：那一场不是顶格的一句——套在某个分支里，就有一条路不打它";
        }
        ASSERT_GE(battleLine, 0);
        const std::string setter = "flag.set(\"" + setFlag + "\"";
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (codeOf(lines[i]).find(setter) == std::string::npos) continue;
            EXPECT_GT(static_cast<int>(i), battleLine) << callers[0] << " 第 " << (i + 1) << " 行：在打 " << h.battleId
                                                       << " 之前就置了 " << setFlag;
        }
        for (const auto& [path, source] : allScripts_) {
            if (path == callers[0]) continue;
            EXPECT_EQ(codeOnly(source).find(setter), std::string::npos)
                << path << " 也置 " << setFlag << "：" << h.mark << " 那一节能被它跳过去";
        }
    }
    // 扫描器自检：同一个扫法对第 5 章那十场是看得见的（分母不塌）。
    int chapterFive = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch05")) chapterFive += countOf(codeOnly(source), "battle(\"b05_");
    EXPECT_GE(chapterFive, 10) << "扫描器自检：第 5 章的 battle( 该数得出来";
}

// 施工图第 4 节：「每一处挂战斗的格子前面都有存档点：① 广场、② 山丘落点、③ 石屋群」。
// 口径与第 5 章同：从那张图的默认出生点出发，不踩本图任何一处必打战斗的踏入型挂点，走得到一个存档设施的旁边。
// 静止的 NPC（不带 visible / hidden 旗标的）挡路；设施与门不能踩。
TEST_F(Ch06Acceptance, No15_EveryMustFightHookHasASavePointReachableBeforeIt) {
    std::set<std::string> fightMaps;
    for (const MustFightHook& h : mustFightHooks()) fightMaps.insert(h.mapId);
    ASSERT_EQ(fightMaps.size(), 3u);
    for (const std::string& mapId : fightMaps) {
        auto loaded = app_.loadMap(mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        const TileMap& map = *app_.currentMap();
        std::set<std::pair<int, int>> blocked;
        Point spawn{-1, -1};
        std::vector<const MapObject*> saves;
        for (const MapObject& object : map.objects) {
            const auto cells = [&](const std::function<void(int, int)>& f) {
                for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                    for (int dx = 0; dx < std::max(1, object.width); ++dx) f(object.position.x + dx, object.position.y + dy);
                }
            };
            if (object.type == "spawn" && object.property("default") == "true") spawn = object.position;
            if (object.type == "facility" || object.type == "portal") cells([&](int x, int y) { blocked.insert({x, y}); });
            if (object.type == "npc" && object.property("visible_flag").empty() && object.property("hidden_flag").empty()) {
                cells([&](int x, int y) { blocked.insert({x, y}); });
            }
            if (object.type == "facility" && object.property("kind") == "save") saves.push_back(&object);
            for (const MustFightHook& h : mustFightHooks()) {
                if (mapId != h.mapId || object.name != h.hook) continue;
                if (object.property("mode") == "enter") cells([&](int x, int y) { blocked.insert({x, y}); });
            }
        }
        ASSERT_GE(spawn.x, 0) << mapId << " 没有默认出生点";
        ASSERT_FALSE(saves.empty()) << mapId << " 挂着必打战斗，图上却没有存档点（施工图第 4 节）";
        std::set<std::pair<int, int>> seen{{spawn.x, spawn.y}};
        std::deque<Point> queue{spawn};
        while (!queue.empty()) {
            const Point p = queue.front();
            queue.pop_front();
            for (const Point d : kDirections) {
                const Point n{p.x + d.x, p.y + d.y};
                if (!map.walkable(n) || blocked.count({n.x, n.y}) || seen.count({n.x, n.y})) continue;
                seen.insert({n.x, n.y});
                queue.push_back(n);
            }
        }
        bool reachable = false;
        for (const MapObject* save : saves) {
            for (const Point d : kDirections) reachable = reachable || seen.count({save->position.x + d.x, save->position.y + d.y}) > 0;
        }
        EXPECT_TRUE(reachable) << mapId << "：从进图那一格出发，不踩必打的挂点就走不到存档点（施工图第 4 节）";
    }
}

// ===========================================================================
// 验收 18：主线节点、分支、新地图、字数（与 tools/audit.py 的数对照写在测试路的回复里）
// ===========================================================================
TEST_F(Ch06Acceptance, No18_BranchesMapsAndWordsAreWhereSectionFifteenPutsThem) {
    // 主动分支 9 处（4、8、10、11、14、17×2、认药）：3.1 表里这八处挂点的脚本各有 choice{。
    int choices = 0;
    for (const auto& [path, source] : chapterScripts_) choices += countOf(codeOnly(source), "choice{");
    EXPECT_GE(choices, 9) << "施工图验收 18：分支 ≥ 9 处";
    const std::vector<std::pair<const char*, const char*>> kWithChoice = {
        {kMapGu, "trigger_ruhuo"},        {kMapGu, "npc_maifu_shaonv"},     {kMapGu, "trigger_canpian"},
        {kMapGu, "trigger_sanhui"},       {kMapHfg, "trigger_ye_maidan"},   {kMapHfg, "trigger_zawu"},
        {kMapHfg, "trigger_juanzong"},    {kMapByy, "trigger_jinzhi"}};
    for (const auto& [mapId, hook] : kWithChoice) {
        const MapObject object = objectOn(mapId, hook);
        const auto it = allScripts_.find(object.property("script"));
        ASSERT_NE(it, allScripts_.end()) << hook << " 挂的脚本读不到";
        EXPECT_GT(countOf(codeOnly(it->second), "choice{"), 0) << hook << "：施工图 3.1 的主动分支在这里";
    }
    // 认药三题（施工偏差 18.4 第 1 条）：jinzhi 里三处 choice，各四个选项。
    const auto blocks = choiceBlocks(allScripts_["ch06/jinzhi.lua"]);
    ASSERT_EQ(blocks.size(), 3u) << "施工偏差 18.4 第 1 条：认药三题";
    for (const auto& options : blocks) EXPECT_EQ(options.size(), 4u) << "施工图 3.2 节点 18：四个选项里认出那一味";
    // 新地图 6 张。
    int maps = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "maps")) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("ch06_", 0) == 0 && entry.path().extension() == ".tmj") ++maps;
    }
    EXPECT_EQ(maps, 6) << "施工图第 4 节 / 验收 18：新地图 6 张";
    // 主线对白（key 以 ch06. 开头、在 ch06* 文件里的文案，只数汉字）。施工图第 3 节「约 14800」、
    // 施工偏差 18.4 第 4 条「实为约 17500（ch06_main.json）」。「约」没有给容差，这里只防塌：少于 14800 的八成就报。
    int chars = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "data" / "text")) {
        if (entry.path().filename().string() != "ch06_main.json") continue;
        const std::string body = readFile(entry.path());
        for (const auto& [key,value] : fanren::test::textKeyValues(body)) {
            if (key.rfind("ch06.",0)!=0) continue;
            for (std::size_t i = 0; i < value.size();) {
                const unsigned char c = static_cast<unsigned char>(value[i]);
                if (c >= 0xE0 && c < 0xF0 && i + 2 < value.size()) {
                    const unsigned cp = ((c & 0x0Fu) << 12) | ((static_cast<unsigned char>(value[i + 1]) & 0x3Fu) << 6) |
                                        (static_cast<unsigned char>(value[i + 2]) & 0x3Fu);
                    if ((cp >= 0x3400 && cp <= 0x9FFF) || (cp >= 0xF900 && cp <= 0xFAFF)) ++chars;
                    i += 3;
                } else {
                    i += c < 0x80 ? 1 : (c < 0xE0 ? 2 : 4);
                }
            }
        }
    }
    std::cout << "[ch06 字数] ch06_main.json 主线文案汉字 " << chars << "（施工图第 3 节约 14800；施工偏差 18.4 第 4 条约 17500）"
              << std::endl;
    EXPECT_GE(chars, 14800 * 8 / 10) << "主线对白比施工图的「约 14800 字」少了两成以上";
    // 主线节点 18：目标链 26 步落在 3.1 表的 18 个节点上（1a/1b、5a/5b、8a/8b、9a/9b、10a/10b、16a/16b、17a/17b、18a/18b）。
    std::set<std::string> nodes;
    for (const StepInfo& info : stepTable()) {
        if (info.doneFlag[0] == '\0') continue;
        std::string node = info.node;
        while (!node.empty() && (node.back() == 'a' || node.back() == 'b')) node.pop_back();
        nodes.insert(node);
    }
    EXPECT_EQ(nodes.size(), 18u) << "施工图第 3 节：主线节点 18 个";
}

// ---------------------------------------------------------------------------
// 验收第 15 节各条的落点（不在本文件的那几条）
// ---------------------------------------------------------------------------
//   1 门禁：build.bat 的四道门禁本身。不写 C++ 断言。
//   3 挂点：Ch06TriggerModeTests。
//   4 编成：Ch06BattleDataTests。
//   5、7、8、9、12、13、14、16、17、19：Ch06SliceTests。
//   10 经济账、11 两侧同账（读数据那一半）：Ch06LedgerTests；11 的「走一遍」在上面两条通关的 1a 之后。
//   20 独立校对：人工，不是测试。

}  // namespace
