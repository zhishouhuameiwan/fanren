// 第 7 章的自动化通关测试与几条整章验收（docs/ch07-design.md 第 15 节验收第 2、6、11、15、17、18、22 条），
// 外加「那只手先证明自己稳」（同一存档跑三遍）与「五只手」的扫描，以及交给第 8 章的终局存档。
//
// 驱动的是玩家真会碰的那一整套：真的 Application（无头）、真的 maps/ch07_*.tmj 与第 6 章两张修补过的图、
// 真的 WorldScene 走位规则、真的 scripts/ch07/*.lua、真的 BattleScene、FieldScene（下种 / 催熟 / 采收）、
// CultivationScene（蒲团打坐推日子）、ShopScene（坊市收药摊）、AlchemyScene（地火屋亲手开炉）。
//
// ---------------------------------------------------------------------------
// 判据从哪来（handoff 第 6 节第 5 条、handoff-2026-09-23-ch04 第 8 节）
// ---------------------------------------------------------------------------
// **判据只从施工图推，不从被测的脚本和数据里抄。** 下面每一个 kXxx 常量、每一张表旁边都写着它抄自
// 施工图哪一节。脚本里的文案 key 只拿来**认出**「演的是哪一条」，不拿来决定「该演哪一条」。
// 施工图第 18 节的施工偏差，本文件认了这几条（改判据的地方旁边写着「施工偏差 18.x」）：
//   · 18.2：节点 6 挂在黄枫谷东口 (46,26)，七个人站路两边——照样按 3.1 的次序去踩它；
//   · 18.3：节点 16 的报名案摆在王师叔身边 (25,12)；
//   · 18.5：节点 26b 插刃是一个选项按八次（choice 循环），中途取消 = 收手——本文件每一回都按「插」；
//   · 18.6：节点 32a 拨一天（xiashan.lua），3.3 的日历表在那一步记 1 天；
//   · 18.7：节点 35a 的钱一律「有则扣」（没钱那几条路在 Ch07LedgerTests 的切片里走）；
//   · 18.13：8 / 15 的挂点在吴风南边 (40,10)；
//   · 18.14：卖符少女的欠账在 19b 了结（ch06.qianyao > 0 才说那两句）。
//
// ---------------------------------------------------------------------------
// 起点不许自己挑（handoff-2026-09-23-ch04 第 8 节第二条、施工图第 2 节）
// ---------------------------------------------------------------------------
// 起点**直接读** tests/fixtures/ch06-end-first.sav / ch06-end-second.sav，由第 6 章两条通关用例写出、
// 逐字段看着。本文件在起点上**一个字段也不改**：读进来、载入存档里那张图（百药园）、把人放回存档里那一格。
// SetUp 只先验施工图第 2 节里**本章靠得住**的那几格（境界、上限、六门法术、剑符、瓶子、灵田、熟练度、资质、
// 第 6 章完成旗标）；日子、灵石、丹药、符箓、站位、第 6 章各分岔的旗标**不写死**——第 6 章还在整改，
// 合回主树后交接存档会有小变化（协调者 2026-09-29：日子 +2 左右、ch06.qianyao、站位）。
// 账一律按「起点读出来的数 + 施工图的进出」算，不抄起点的绝对值。
//
// ---------------------------------------------------------------------------
// 替玩家出手的那只手：意图玩家（施工图 8.1 / 8.2）
// ---------------------------------------------------------------------------
// 第 3–6 章共用的那只（tests/BattleHand.h），取舍沿用第 5、6 章的意图玩家：血掉到一半才吃药、不算对面
// 下一轮打不打得死自己、没药能逃就逃（本章五场必打都不许逃，这一条只管 Z2 东坡那一场）；吃药的名单是
// 养精丹在前、金疮药在后（施工图 8.1「养精丹、金疮药（马师伯的两瓶）」）。那只手不会用带 castMagic 的符
//（BattleHand 只撒 policy.poisons 里的毒），所以第一侧在 ② 开场亲手甩一张土牢符给络腮胡子、在 ③ 开场
// 亲手掷天雷子——施工图 8.2 ②③ 写明的原著解法，走的是真菜单（BattleHand::issue）；第二侧一张符也不用，
// ③ 是「没用天雷子」的那一路（验收 4）。
// 这只手自己稳不稳：TheHandIsSteadyThreeRunsFromTheSameSaveEndAlike；对取舍敏不敏感：TheSweepOfFiveHands。
//
// ---------------------------------------------------------------------------
// 药园经营（施工图第 7 节、3.2 节点 2 / 3 / 9、第 6 节 Z1）
// ---------------------------------------------------------------------------
// 意图玩家照目标行的 R-3 提示办（3.4）：拆包当天把八株苗全种下（药田六畦：黄精 3、紫参 2、血红芝 1；
// 园角两畦：黄精芝 2——「角上阴凉，留给娇贵的」）；在居室蒲团上「闭关一年」等苗足年；此后「瓶子一满就去浇」：
// 打坐一日一日地坐到瓶满，浇进当下要交的那一株，浇到年份再采。天数是玩家自己推的（3.3「玩家推进的三段」），
// 不判；剧情挂点自己拨的日子逐步判。
// 禁地里的伤一路带着走（施工图 16.2 第 5 条：每天只回一成、存档点不回血）：意图玩家在 ③、④、Z2 东坡、下地道
// 之前，到施工图 3.1 表末行那三处打坐点（树冠、山顶石洞）坐到满血——那三处就是为这个放的。这几天也记「玩家推的」。
//
// 想看每一场仗的出手记录：set FANREN_CH07_BATTLE_LOG=1 再跑（平衡路量数用；不设就不打印）。
//
// ---------------------------------------------------------------------------
// 这个文件看不见什么
// ---------------------------------------------------------------------------
//   · 画面（对话框、破绽图标、章节卡）：只读 spokenKeys 与状态；
//   · 战斗与炼丹的 RNG：种子由编成 id / 方子与日期派生，每一场、每一炉都是**确定的**——「打得赢」「几炉成一炉」
//     只是这一个种子上的一次抽样，不是概率上的保证。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
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
#include "core/rules/Crafting.h"
#include "core/rules/Objectives.h"
#include "core/rules/Quests.h"
#include "core/rules/Realm.h"
#include "game/AlchemyScene.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "game/BoardScene.h"
#include "game/CultivationScene.h"
#include "game/FieldScene.h"
#include "game/Scene.h"
#include "game/ShopScene.h"
#include "game/Wording.h"
#include "game/WorldScene.h"
#include "io/DataLoader.h"
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
using fanren::game::BattleScene;
using fanren::game::BoardScene;
using fanren::game::CultivationScene;
using fanren::game::FieldScene;
using fanren::game::ShopScene;
using fanren::game::WorldScene;
using fanren::rules::Realm;

// ---- 本章踏过的十一张图（施工图第 4 节：新建 9 张 ＋ 第 6 章两张修补；起点在百药园）----
constexpr const char* kMapByy = "ch06_baiyaoyuan";
constexpr const char* kMapHfg = "ch06_huangfenggu";
constexpr const char* kMapYuelu = "ch07_yuelu_dian";
constexpr const char* kMapDihuo = "ch07_dihuo";
constexpr const char* kMapFangshi = "ch07_fangshi";
constexpr const char* kMapShandong = "ch07_shandong";
constexpr const char* kMapWai = "ch07_jindi_wai";
constexpr const char* kMapWaiwei = "ch07_jindi_waiwei";
constexpr const char* kMapZhongxin = "ch07_jindi_zhongxin";
constexpr const char* kMapHxs = "ch07_huanxingshan";
constexpr const char* kMapZhaoze = "ch07_dixia_zhaoze";

// ---- 本章动到的东西（施工图第 2 节、3.2、第 9 节）----
constexpr const char* kLingshi = "material_lingshi";
constexpr const char* kZhong = "material_lingshi_zhong";
constexpr const char* kPill = "pill_yangjing_dan";
constexpr const char* kSalve = "pill_jinchuang_yao";
constexpr const char* kQingling = "pill_qingling_san";
constexpr const char* kHuiqi = "pill_huiqi_dan";
constexpr const char* kZhuji = "pill_zhuji_dan";
constexpr const char* kYaofen = "material_zhuji_yaofen";
constexpr const char* kHuangjing = "herb_huangjing_cao";
constexpr const char* kZishen = "herb_zishen_cao";
constexpr const char* kXuehong = "herb_xuehong_zhi";
constexpr const char* kHuangjingZhi = "herb_huangjing_zhi";
constexpr const char* kYusui = "herb_yusui_zhi";
constexpr const char* kZihou = "herb_zihou_hua";
constexpr const char* kTianling = "herb_tianling_guo";
constexpr const char* kJianfuItem = "talisman_jianfu";
constexpr const char* kJinguangzhuan = "talisman_jinguangzhuan";
constexpr const char* kTianleizi = "talisman_tianleizi";
constexpr const char* kTulaoFu = "talisman_tulao_fu";
constexpr const char* kHuoqiuFu = "talisman_huoqiu_fu";
constexpr const char* kHushenFu = "talisman_hushen_fu";
constexpr const char* kDingshenFu = "talisman_dingshen_fu";
constexpr const char* kXiaodaoFubao = "talisman_xiaodao_fubao";
constexpr const char* kSixian = "weapon_wuming_sixian";
constexpr const char* kBiguang = "weapon_biguang_dao";
constexpr const char* kTayun = "story_tayun_xue";
constexpr const char* kJianjue = "story_qingyuan_jianjue";
constexpr const char* kJinggangHuan = "story_jinggang_huan";
constexpr const char* kYinGou = "story_yin_gou";
constexpr const char* kQingSuo = "story_qing_suo";
constexpr const char* kMagicHuodan = "magic_huodan_shu";
constexpr const char* kMagicYufeng = "magic_yufeng_jue";
constexpr const char* kMagicTianyan = "magic_tianyan_shu";
constexpr const char* kMagicLiusha = "magic_liusha_shu";
constexpr const char* kMagicBingdong = "magic_bingdong_shu";
constexpr const char* kMagicJianfu = "magic_ji_jianfu";
constexpr const char* kMagicJinfu = "magic_ji_jinfu";
constexpr const char* kMagicJinguangzhuan = "magic_ji_jinguangzhuan";
constexpr const char* kMagicQingjiao = "magic_ji_qingjiao";
constexpr const char* kMagicQingning = "magic_ji_qingning";
constexpr const char* kField = "field_baiyaoyuan";
constexpr const char* kFieldJiao = "field_baiyaoyuan_jiao";

// ---- 施工图的数（旁边写着出处）----
constexpr int kHuangjingAge = 44;     // 3.2 节点 2：take_aged("herb_huangjing_cao", 3, 44)
constexpr int kHuangjingDue = 3;
constexpr int kZishenAge = 100;       // 3.2 节点 3：take_aged("herb_zishen_cao", 2, 100)
constexpr int kZishenDue = 2;
constexpr int kXuehongAge = 176;      // 第 6 节 Z1：take_aged("herb_xuehong_zhi", 1, 176)
constexpr int kQiannianAge = 1000;    // 3.2 节点 9 / 11：count_aged(..., 1000) >= 2
constexpr int kQiannianDue = 2;
constexpr int kSniffAge = 100;        // 3.2 节点 32b：嗅灵兽闻得出百年以上
constexpr int kPayYearOne = 24;       // 3.2 节点 2：一年月例
constexpr int kPayYearTwo = 60;       // 3.2 节点 3：月例涨到五块
constexpr int kCangshiFee = 1 + 1 + 20;   // 3.2 节点 4b：两个时辰 ＋ 复制两份
constexpr int kDingPrice = 32;        // 3.2 节点 5a：银丝鼎
constexpr int kLuLingshi = 20;        // 3.2 节点 13 搜身
constexpr int kLuZhuji = 2;
constexpr int kLuHuoqiu = 4;
constexpr int kLuHushen = 2;
constexpr int kZ2Lingshi = 50;        // 第 6 节 Z2 加赏
constexpr int kZ2Zhong = 1;
constexpr int kRewardZhuji = 1;       // 3.2 节点 34 奖赏
constexpr int kTallyZhong = 10;       // 3.2 节点 34 清点
constexpr int kTallyLingshi = 300;
constexpr int kYeZhong = 5;           // 3.2 节点 34 叶师叔补还
constexpr int kYeHuiqi = 2;
constexpr int kYaofenGiven = 40;      // 3.2 节点 34：药粉一律 40 份
constexpr int kYaofenWasted = 22;     // 3.2 节点 35b：二十几炉废丹
constexpr int kZhujiTopUp = 25;       // 3.2 节点 35c：筑基丹补足到 25
constexpr int kZhujiEaten = 8;        // 3.2 节点 36
constexpr int kZhujiAtEnd = 17;       // 第 9 节第 2 条 / 验收 2
constexpr int kLingshiDelta = 349;    // 第 9 节第 6 条：章末灵石 ≈ 起点 + 349（不含 Z2 与收药摊）
constexpr int kZhongDelta = 17;       // 第 9 节第 6 条：中阶 ≈ 17 − k

// ---- 施工图 8.0 / 8.2：五场必打（按发生顺序）与本章可能碰上的编成 ----
constexpr const char* kMustFight[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                                      "b07_zhaoze_shouyao"};
constexpr const char* kMustFightMark[] = {"①", "②", "③", "④", "⑤"};
constexpr const char* kChapterBattles[] = {"b07_lu_shixiong", "b07_yixiantian", "b07_fengyue", "b07_zhongxinqu_duoyao",
                                           "b07_zhaoze_shouyao", "be07_tuishan_shou", "be07_tiebi_yuan",
                                           "be07_huoyan_shu"};

constexpr Point kDirections[] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
constexpr double kFrame = 1.0 / 60.0;
constexpr int kMaxScriptFrames = 12000;
constexpr int kMaxAttempts = 3;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "scripts" / "ch07" / "zhuji.lua") && fs::exists(root / "maps" / "ch07_dihuo.tmj")) {
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

// scripts/<sub> 下所有 .lua（相对 scripts/ 的路径 → 全文）。
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

// ---------------------------------------------------------------------------
// 图与图之间的门（不写任何 require_flag：闸开没开由引擎说了算）
// ---------------------------------------------------------------------------
// 坊市 → 山洞、山洞 → 百药园、黄枫谷 → 禁地外、禁地外 → 外围、环形山 ⇄ 沼泽、环形山 → 禁地外、禁地外 → 百药园、
// 地火屋 → 百药园都是脚本 teleport，不在这张表里（施工图第 4 节「连通」）。
struct MapLink {
    const char* from;
    const char* to;
    const char* portal;
};

constexpr MapLink kMapLinks[] = {
    {kMapByy, kMapHfg, "portal_to_huangfenggu"},        {kMapHfg, kMapByy, "portal_to_baiyaoyuan"},
    {kMapHfg, kMapFangshi, "portal_to_fangshi"},        {kMapFangshi, kMapHfg, "portal_to_huangfenggu"},
    {kMapHfg, kMapYuelu, "portal_to_yuelu_dian"},       {kMapYuelu, kMapHfg, "portal_to_huangfenggu"},
    {kMapYuelu, kMapDihuo, "portal_to_dihuo"},          {kMapDihuo, kMapYuelu, "portal_to_yuelu_dian"},
    {kMapWaiwei, kMapZhongxin, "portal_to_jindi_zhongxin"}, {kMapZhongxin, kMapHxs, "portal_to_huanxingshan"},
};

// ---------------------------------------------------------------------------
// 一章的步骤：施工图 3.1 表的挂点，按 3.4 目标链的次序；中间夹着玩家自己的动作（种药、催熟、买、炼）
// ---------------------------------------------------------------------------
enum class Step {
    Chaibao, Xiazhong, CuiHuangjing, Jiaoyao, CuiZishen, Danbao, CuiXuehong, Baigong,
    Yuelu, Cangshi, DihuoWen, Shimen, Murong, Dufang, Lianqi, CuiZhi, Qiannian, Chuling,
    FangshiRu, Wanbaolou, Yaotan, FangshiChu, Yeyu, Fanhui, Fudan, Baoming, MaSongyao, Jihe,
    Xiang, Liedui, Pojin, Wulongtan, Liangshi, Yixiantian, RestShulin, Shulin, Tongmen, Yueyang,
    Kuitan, Charen, Youdi, Sichu, RestYujian, YujianA, YujianB, RestXiaoshidian, Xiaoshidian, RestDidao, Qingshidian,
    Guanzhan, Jiaoshi,
    Xiashan, Chukou, Xieshili, Sannian, Chouhan, Shijiu, Liandan, Banian, Zhuji,
};

// 这一步是谁的动作：剧情挂点（踩 / 按），还是玩家自己（种药催熟、坊市买卖、亲手开炉）。
enum class Kind { Enter, Interact, Farm, Shop, Craft, Rest };

struct StepInfo {
    Step step;
    const char* node;       // 施工图 3.1 表的 # 列（玩家自己的动作写「—」）
    const char* name;
    const char* doneFlag;   // 3.1 表的 set_flag 列（药篓、传功阁两处 once=false 写脚本自己置的那面旗）
    const char* mapId;
    const char* object;
    Kind kind;
    int days;               // 3.3 日历：这一步该走掉的日子；-1 = 玩家自己推的（3.3「玩家推进的三段」），不判
};

const std::vector<StepInfo>& stepTable() {
    static const std::vector<StepInfo> kTable = {
        {Step::Chaibao, "1", "拆包", "ch07.chaibao", kMapByy, "trigger_chaibao", Kind::Interact, 0},
        {Step::Xiazhong, "—", "下种（药田六畦、园角两畦）", "", kMapByy, "facility_yaotian", Kind::Farm, -1},
        {Step::CuiHuangjing, "—", "催黄精到四十四年", "", kMapByy, "facility_yaotian", Kind::Farm, -1},
        {Step::Jiaoyao, "2", "头一年交药", "ch07.jiaoyao1", kMapByy, "trigger_yaolou", Kind::Interact, 0},
        {Step::CuiZishen, "—", "催紫参到百年", "", kMapByy, "facility_yaotian", Kind::Farm, -1},
        {Step::Danbao, "3", "第二年交药·担保", "ch07.danbao", kMapByy, "trigger_yaolou", Kind::Interact, 0},
        {Step::CuiXuehong, "—", "催血红芝（Z1）", "", kMapByy, "facility_yaotian", Kind::Farm, -1},
        {Step::Baigong, "Z1", "白看一年园子", "ch07.baigong", kMapByy, "trigger_yaolou", Kind::Interact, 0},
        {Step::Yuelu, "4a", "传送阵", "ch07.yuelu", kMapYuelu, "trigger_yuelu_ru", Kind::Enter, 0},
        {Step::Cangshi, "4b", "许老与藏室", "ch07.cangshi", kMapYuelu, "trigger_cangshi", Kind::Interact, 0},
        {Step::DihuoWen, "5a", "地火与银丝鼎", "ch07.yinsi", kMapYuelu, "trigger_dihuo_wen", Kind::Interact, 0},
        {Step::Shimen, "5b", "丑汉与石门", "ch07.chouhan", kMapYuelu, "trigger_shimen", Kind::Enter, 0},
        {Step::Murong, "6", "东口：慕容兄弟与陆师兄", "ch07.murong", kMapHfg, "trigger_murong", Kind::Enter, 0},
        {Step::Dufang, "7", "读方", "ch07.dufang", kMapByy, "trigger_dufang", Kind::Interact, 6},
        {Step::Lianqi, "8", "吴风：禁地与敛气术", "ch07.lianqi", kMapHfg, "trigger_chuangong", Kind::Interact, 3},
        {Step::CuiZhi, "—", "园角催两株千年黄精芝", "", kMapByy, "facility_field_jiao", Kind::Farm, -1},
        {Step::Qiannian, "9", "园角千年灵草", "ch07.qiannian", kMapByy, "trigger_yuanjiao", Kind::Interact, 0},
        {Step::Chuling, "10", "外出令牌", "ch07.chuling", kMapHfg, "trigger_chuling", Kind::Interact, 0},
        {Step::FangshiRu, "11a", "坊市北口", "ch07.fangshi", kMapFangshi, "trigger_fangshi_ru", Kind::Enter, 0},
        {Step::Wanbaolou, "11b", "万宝楼", "ch07.wanbaolou", kMapFangshi, "trigger_wanbaolou", Kind::Interact, 0},
        {Step::Yaotan, "—", "收药摊买卖", "", kMapFangshi, "facility_yaotan", Kind::Shop, 0},
        {Step::FangshiChu, "12", "绕路", "ch07.likai", kMapFangshi, "trigger_fangshi_chu", Kind::Enter, 6},
        {Step::Yeyu, "13", "夜遇陆师兄", "ch07.yeyu", kMapShandong, "trigger_yeyu", Kind::Interact, 0},
        {Step::Fanhui, "14", "返回", "ch07.fanhui", kMapShandong, "trigger_fanhui", Kind::Interact, 1 + 3 + 30},
        {Step::Fudan, "15", "服丹之法；封禁", "ch07.fudan_fa", kMapHfg, "trigger_chuangong", Kind::Interact, 14},
        {Step::Baoming, "16", "报名", "ch07.baoming", kMapHfg, "trigger_baoming", Kind::Interact, 0},
        {Step::MaSongyao, "17", "马师伯的两瓶丹药", "ch07.ma_songyao", kMapByy, "trigger_ma_songyao", Kind::Enter, 20},
        {Step::Jihe, "18", "议事大殿", "ch07.jihe", kMapHfg, "trigger_jihe", Kind::Enter, 2},
        {Step::Xiang, "19a", "向之礼", "ch07.xiang", kMapWai, "trigger_xiang", Kind::Enter, 0},
        {Step::Liedui, "19b", "七派到齐", "ch07.qipai", kMapWai, "trigger_liedui", Kind::Interact, 1},
        {Step::Pojin, "20", "破禁", "ch07.pojin", kMapWai, "trigger_pojin", Kind::Enter, 0},
        {Step::Wulongtan, "21a", "乌龙潭", "ch07.wulongtan", kMapWaiwei, "trigger_wulongtan", Kind::Enter, 0},
        {Step::Liangshi, "21b", "山崖两尸", "ch07.sixian", kMapWaiwei, "trigger_liangshi", Kind::Interact, 0},
        {Step::Yixiantian, "22", "一线天", "ch07.yixiantian", kMapWaiwei, "trigger_yixiantian", Kind::Enter, 0},
        // 禁地里的伤一路带着走（施工图 16.2 第 5 条）：原著他歇过的三处打坐点，意图玩家在必打之前坐到满血。
        {Step::RestShulin, "—", "树冠打坐", "", kMapZhongxin, "facility_dazuo_shuguan", Kind::Rest, -1},
        {Step::Shulin, "23", "林中一夜；封岳", "ch07.fengyue", kMapZhongxin, "trigger_shulin", Kind::Interact, 1},
        {Step::Tongmen, "24", "铜门；钟吾", "ch07.zhongwu", kMapZhongxin, "trigger_tongmen", Kind::Enter, 1},
        {Step::Yueyang, "25", "月阳宝珠", "ch07.yueyang", kMapZhongxin, "trigger_yueyang", Kind::Enter, 0},
        {Step::Kuitan, "26a", "紫猴花洞：拐角", "ch07.kuitan", kMapHxs, "trigger_kuitan", Kind::Enter, 0},
        {Step::Charen, "26b", "紫猴花洞：插刃", "ch07.mairen", kMapHxs, "trigger_charen", Kind::Interact, 0},
        {Step::Youdi, "26c", "紫猴花洞：诱敌", "ch07.zihouhua", kMapHxs, "trigger_youdi", Kind::Enter, 0},
        {Step::Sichu, "27", "四处灵药", "ch07.sichu", kMapHxs, "trigger_sichu", Kind::Interact, 0},
        {Step::RestYujian, "—", "去东坡之前打坐", "", kMapHxs, "facility_dazuo_shidong", Kind::Rest, -1},
        {Step::YujianA, "Z2a", "东坡石屋", "ch07.yujian_a", kMapHxs, "trigger_yujian_dongpo", Kind::Interact, 0},
        {Step::YujianB, "Z2b", "北坡寒潭", "ch07.yujian_b", kMapHxs, "trigger_yujian_hantan", Kind::Interact, 0},
        {Step::RestXiaoshidian, "—", "山顶石洞打坐", "", kMapHxs, "facility_dazuo_shidong", Kind::Rest, -1},
        {Step::Xiaoshidian, "28", "小石殿", "ch07.wuyou", kMapHxs, "trigger_xiaoshidian", Kind::Enter, 1},
        // 地下沼泽没有打坐点（施工图第 4 节）：下地道之前在山顶石洞坐满。
        {Step::RestDidao, "—", "下地道之前打坐", "", kMapHxs, "facility_dazuo_shidong", Kind::Rest, -1},
        {Step::Qingshidian, "29", "青石殿与地道", "ch07.didao", kMapHxs, "trigger_qingshidian", Kind::Interact, 0},
        {Step::Guanzhan, "30", "观战；墨蛟", "ch07.mojiao", kMapZhaoze, "trigger_guanzhan", Kind::Enter, 0},
        {Step::Jiaoshi, "31", "金箱；破土；南宫婉", "ch07.nangong", kMapZhaoze, "trigger_jiaoshi", Kind::Interact, 0},
        // 施工偏差 18.6：3.3 在节点 28 之后不再拨日子，出禁地是第五日下午——xiashan.lua 拨一天。
        {Step::Xiashan, "32a", "下山", "ch07.xiashan", kMapHxs, "trigger_xiashan", Kind::Enter, 1},
        {Step::Chukou, "32b", "出禁地；记名弟子", "ch07.chujindi", kMapWai, "trigger_chukou", Kind::Enter, 4},
        {Step::Xieshili, "33", "谢师礼", "ch07.xieshili", kMapByy, "trigger_xieshili", Kind::Enter, 0},
        {Step::Sannian, "34", "三年", "ch07.sannian", kMapByy, "trigger_sannian", Kind::Interact, 4 + 1080},
        {Step::Chouhan, "35a", "丑汉与石门", "ch07.dihuo", kMapYuelu, "trigger_chouhan", Kind::Interact, 0},
        {Step::Shijiu, "35b", "十九号", "ch07.feidan", kMapDihuo, "trigger_shijiu", Kind::Enter, 0},
        {Step::Liandan, "—", "地火亲手开炉", "", kMapDihuo, "facility_dihuo", Kind::Craft, 0},
        {Step::Banian, "35c", "半年", "ch07.chengdan", kMapDihuo, "trigger_banian", Kind::Interact, 180},
        {Step::Zhuji, "36", "服丹筑基", "ch07.done", kMapDihuo, "trigger_zhuji", Kind::Interact, 150},
    };
    return kTable;
}

const StepInfo& infoOf(Step step) {
    for (const StepInfo& info : stepTable()) {
        if (info.step == step) return info;
    }
    return stepTable().front();
}

// 3.3 表逐项相加（加上施工偏差 18.6 那一天）：剧情脚本自己拨的日子，整章 1508 天。
constexpr int kScriptedDays = 6 + 3 + 6 + (1 + 3 + 30) + 14 + 20 + 2 + 1 + 1 + 1 + 1 + 1 + 4 + (4 + 1080) + 180 + 150;

// 战斗开场亲手出的那一招（施工图 8.2 ②③ 的原著解法）：物品 id 用在哪个角色身上。
struct Opener {
    const char* itemId;
    const char* roleId;
};

// 一条路线。
struct Route {
    std::string name;
    int fallback = 0;                          // 二选一取第几项（0 起算）
    std::map<Step, std::vector<int>> answers;  // 按次序作答；答完了从头轮
    bool z1 = false;                           // 支线 Z1：药篓交血红芝
    bool z2 = false;                           // 支线 Z2：东坡、寒潭两处都去
    int buyQingling = 0;                       // 坊市收药摊买几瓶清灵散
    std::map<std::string, Opener> openers;     // 编成 id → 开场那一招
    [[nodiscard]] bool shops() const { return buyQingling > 0; }
};

struct BattleRecord {
    std::string id;
    Step step{};
    BattlePhase phase = BattlePhase::Ongoing;
    int rounds = 0;
    int hpBefore = 0, hpAfter = 0, maxHp = 0;
    int mpBefore = 0, mpAfter = 0;
    int pillsBefore = 0, pillsAfter = 0;
    int salvesBefore = 0, salvesAfter = 0;
    bool opened = false;                     // 开场那一招出手了
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

struct StepRecord {
    Step step{};
    int dayBefore = 0, dayAfter = 0;
    bool finished = false;
    std::map<std::string, int> bagBefore, bagAfter;
    std::map<std::string, int> matureBefore;   // 禁地三药百年以上的株数（嗅灵兽那一步按它算）
    std::vector<std::string> magicsBefore, magicsAfter;
    Realm realmBefore = Realm::Mortal, realmAfter = Realm::Mortal;
    int yaofenCrafted = 0;
    std::vector<std::string> keys;
    std::vector<int> answers;
    std::vector<BattlePhase> phases;
    std::vector<std::string> usedInBattle;     // 开场那一招用掉的物品
};

// 两侧交接存档的文件名。别的一格也不写：起点的数一律现读（文件头「起点不许自己挑」）。
struct HandOver {
    const char* file;
    const char* label;
};
constexpr HandOver kFirstSide = {fanren::test::kChapterSixEndingFirst, "第一侧"};
constexpr HandOver kSecondSide = {fanren::test::kChapterSixEndingSecond, "第二侧"};

// ---------------------------------------------------------------------------
// 夹具
// ---------------------------------------------------------------------------
class Ch07Walkthrough : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app().init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        startFromChapterSixEnding(kFirstSide);
    }

    void TearDown() override { app().shutdown(); }

    // 第 6 章交到本章手里的那份存档，**原样读进来**（施工图第 2 节、验收 2）。
    void startFromChapterSixEnding(const HandOver& side) {
        const fs::path fixture = fanren::test::chapterFixturePath(assetRoot(), side.file);
        auto handedOver = fanren::io::loadGame(fixture.string());
        ASSERT_TRUE(handedOver.ok) << "第 6 章的交接存档读不进来（" << fixture.string() << "）：" << handedOver.error;
        const GameState& s = handedOver.value;
        // 验收 2 的 SetUp 先验（施工图第 15 节第 2 行原文）。
        ASSERT_EQ(s.flag("ch06.done"), 1) << "交接存档不是第 6 章的终局";
        ASSERT_EQ(s.realm, Realm::QiRefining9) << "验收 2：realm == 9";
        ASSERT_EQ(s.realmCap, Realm::QiRefining9) << "验收 2：realmCap == 9";
        ASSERT_EQ(s.itemCount(kJianfuItem), 1) << "验收 2：talisman_jianfu == 1";
        const fanren::rules::SpiritField* field = handedOver.value.fields.empty() ? nullptr : &handedOver.value.fields.front();
        for (const auto& f : s.fields) {
            if (f.id == kField) field = &f;
        }
        ASSERT_NE(field, nullptr) << "验收 2：field_baiyaoyuan 存在";
        ASSERT_EQ(field->id, kField);
        // 施工图第 2 节里本章靠得住的那几格（这一章的账、配方成算、瓶子、法术表都从它们起算）。
        ASSERT_EQ(field->slots.size(), 6u) << "施工图第 2 节：field_baiyaoyuan 6 槽";
        for (const auto& slot : field->slots) ASSERT_TRUE(slot.seedId.empty()) << "施工图第 2 节：6 槽全空";
        for (const auto& f : s.fields) ASSERT_NE(f.id, kFieldJiao) << "园角那两畦是节点 1 才开的";
        ASSERT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining9)) << "施工图第 2 节：九层 132";
        ASSERT_EQ(s.maxMp, fanren::rules::realmMaxMp(Realm::QiRefining9)) << "施工图第 2 节：九层 90";
        const std::set<std::string> six = {kMagicHuodan, kMagicYufeng, kMagicTianyan, kMagicLiusha, kMagicBingdong,
                                           kMagicJianfu};
        ASSERT_EQ(std::set<std::string>(s.learnedMagics.begin(), s.learnedMagics.end()), six)
            << "施工图第 2 节：火弹术、御风决、天眼术、流沙术、冰冻术、祭剑符（6 门）";
        ASSERT_EQ(s.learnedMagics.size(), 6u);
        ASSERT_TRUE(s.party.empty()) << "施工图第 2 节：party 空";
        ASSERT_TRUE(s.bottle.owned) << "施工图第 2 节：bottle owned";
        ASSERT_TRUE(s.bottle.matureKnown) << "施工图第 2 节：matureKnown";
        ASSERT_EQ(s.bottle.capacity, 3) << "施工图第 2 节：capacity 3";
        ASSERT_EQ(s.alchemyProficiency, 100) << "施工图第 2 节：炼丹熟练度 100（筑基丹方的成算按它定）";
        ASSERT_EQ(s.aptitude, 50) << "施工图第 2 节：资质 50";
        ASSERT_EQ(s.flag("story.xiuxian_known"), 1) << "施工图第 2 节：story.xiuxian_known";
        ASSERT_EQ(s.mapId, kMapByy) << "施工图第 2 节：百药园";
        for (const StepInfo& info : stepTable()) {
            if (info.doneFlag[0] != '\0') ASSERT_EQ(s.flag(info.doneFlag), 0) << "第 6 章终局不该已经置了 " << info.doneFlag;
        }

        const Point where = s.position;
        const int facing = s.facing;
        state() = s;
        auto loaded = app().loadMap(s.mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
        // loadMap 把人挪到了出生点；放回存档里那一格——这是撤销那一挪，不是改字段。
        state().position = where;
        state().facing = facing;
        startDay_ = state().day;
        startLingshi_ = state().itemCount(kLingshi);
        startZhong_ = state().itemCount(kZhong);
        startQianyao_ = state().flag("ch06.qianyao");
        side_ = &side;
        battles_.clear();
        steps_.clear();
        chapterLog_.clear();
        stuckAt_.clear();
        answered_.clear();
        shopped_ = false;
        planted_ = false;
        throwFights_.clear();
        lingshiAfterDanbao_ = -1;
        crafts_ = 0;
        strictWalk_ = false;
        hints_.clear();
        app().clearSpokenKeys();
    }

    void settleEndingAgainstFixture(const char* fileName) {
        const fanren::test::FixtureVerdict verdict = fanren::test::settleAgainstFixture(
            state(), assetRoot(), fileName, fanren::test::kWriteChapterSevenFixturesEnv, "第 8 章");
        if (verdict.wrote) {
            std::cout << "[ch07 交接存档] 已写出 " << fanren::test::chapterFixturePath(assetRoot(), fileName).string()
                      << std::endl;
        }
        EXPECT_TRUE(verdict.problem.empty()) << verdict.problem;
    }

    Application& app() { return *appHolder_; }
    const Application& app() const { return *appHolder_; }
    void renewApp() {
        appHolder_->shutdown();
        appHolder_ = std::make_unique<Application>();
        auto ready = appHolder_->init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }

    GameState& state() { return app().state(); }
    int flag(const std::string& name) { return state().flag(name); }

    void complain(const std::string& why) {
        if (!tolerateBlocked_) ADD_FAILURE() << why;
    }

    // -----------------------------------------------------------------------
    // 文案：读走 spokenKeys 并清空。返回这一回读到的条数（talk 记 key、choice 不记——「选择怎么答」）。
    // -----------------------------------------------------------------------
    std::size_t drainKeys() {
        const std::vector<std::string> keys = app().spokenKeys();
        for (const std::string& key : keys) {
            chapterLog_.emplace_back(currentStep_, key);
            if (!steps_.empty()) steps_.back().keys.push_back(key);
        }
        app().clearSpokenKeys();
        return keys.size();
    }
    bool spokeIn(Step step, const std::string& key) const {
        for (const StepRecord& record : steps_) {
            if (record.step != step) continue;
            if (std::find(record.keys.begin(), record.keys.end(), key) != record.keys.end()) return true;
        }
        return false;
    }
    bool spokeAnywhere(const std::string& key) const {
        return std::any_of(chapterLog_.begin(), chapterLog_.end(),
                           [&key](const auto& entry) { return entry.second == key; });
    }
    const StepRecord* recordOf(Step step) const {
        for (const StepRecord& record : steps_) {
            if (record.step == step && record.finished) return &record;
        }
        return nullptr;
    }

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
        p.healAtPercent = 50;          // 沿用第 5、6 章意图玩家：血掉到一半才吃药
        p.healWhenDoomed = false;      // 不算对面下一轮打不打得死自己
        p.fleeWhenSpent = true;        // 没药能逃就逃（五场必打都不许逃）
        p.pills = {kPill, kSalve};     // 施工图 8.1：养精丹、金疮药（马师伯的两瓶）
        return p;
    }

    std::string identifyBattle(const BattleScene& scene) {
        std::vector<std::pair<std::string, int>> have;
        for (const Unit& u : scene.battle().units()) {
            if (!u.ally) have.emplace_back(u.id, u.wave);
        }
        std::sort(have.begin(), have.end());
        for (const char* id : kChapterBattles) {
            const fanren::core::BattleSetup* setup = app().battleSetup(id);
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

    // 「这一场故意输」：一招不出，只按防御——「输了之后路怎么走」那几条用例用，不是意图玩家。
    BattlePhase playPassively(BattleScene& scene) {
        for (int step = 0; step < 8000; ++step) {
            if (scene.battle().phase() != BattlePhase::Ongoing) break;
            const int actor = scene.runToAllyTurn();
            if (actor < 0) break;
            scene.openMenu(app());
            if (!scene.menuChoose(app(), fanren::game::kBattleMenuDefend)) {
                scene.closeMenu();
                break;
            }
        }
        return scene.battle().phase();
    }

    // 开场那一招：轮到韩立的头一手，走真菜单把那件东西用在那个人身上。用成了返回 true。
    bool openWith(BattleScene& scene, fanren::test::BattleHand& hand, const Opener& opener) {
        const int actor = scene.runToAllyTurn();
        if (actor < 0) return false;
        int target = -1;
        for (std::size_t i = 0; i < scene.battle().units().size(); ++i) {
            const Unit& u = scene.battle().units()[i];
            if (!u.ally && u.alive() && u.id == opener.roleId) target = static_cast<int>(i);
        }
        if (target < 0 || state().itemCount(opener.itemId) <= 0) return false;
        Action a;
        a.kind = ActionKind::Item;
        a.actorIndex = actor;
        a.targetIndex = target;
        a.magicId = opener.itemId;
        return hand.issue(scene, a);
    }

    // -----------------------------------------------------------------------
    // 脚本
    // -----------------------------------------------------------------------
    void pumpScripts() {
        int pendingFill = -1;
        for (int frame = 0; frame < kMaxScriptFrames && app().scripts().isRunning(); ++frame) {
            app().tick(kFrame);
            if (pendingFill >= 0 && dynamic_cast<BattleScene*>(app().topScene()) == nullptr) {
                fillAfter(static_cast<std::size_t>(pendingFill));
                pendingFill = -1;
            }
            if (auto* fight = dynamic_cast<BattleScene*>(app().topScene()); fight != nullptr) {
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
                    fanren::test::BattleHand hand(app(), policy_);
                    if (route_ != nullptr) {
                        const auto opener = route_->openers.find(record.id);
                        if (opener != route_->openers.end()) {
                            record.opened = openWith(*fight, hand, opener->second);
                            if (record.opened && !steps_.empty()) steps_.back().usedInBattle.push_back(opener->second.itemId);
                        }
                    }
                    record.phase = hand.play(*fight);
                }
                record.rounds = fight->battle().round();
                for (const Unit& u : fight->battle().units()) {
                    if (u.ally && u.id == "hanli") record.mpAfter = u.mp;
                }
                if (fanren::test::environmentFlagSet("FANREN_CH07_BATTLE_LOG")) {
                    std::cout << "---- " << record.id << " ----" << std::endl;
                    for (const std::string& line : fight->battle().log()) std::cout << "  " << line << std::endl;
                }
                battles_.push_back(record);
                if (!steps_.empty()) steps_.back().phases.push_back(record.phase);
                pendingFill = static_cast<int>(battles_.size()) - 1;
                continue;
            }
            if (!app().awaitingCommand()) continue;
            const bool talked = drainKeys() > 0;
            fanren::script::CommandResult result;
            result.ok = true;
            result.choiceIndex = talked ? 0 : answerChoice();
            app().completeCommand(result);
            app().popScene();
        }
        if (app().scripts().isRunning()) complain("脚本没能跑到结束");
        app().tick(kFrame);
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
    // 地图与走位（与第 3–6 章同一套：WorldScene::interact 看面朝的前一格；路上不许顺手把剧情演了）
    // -----------------------------------------------------------------------
    const TileMap& map() const { return *app().currentMap(); }

    MapObject objectNamed(const std::string& name) const {
        for (const MapObject& object : map().objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    // 另一张图上的对象（不换当前图）：看撤场 / 出场旗标用。
    MapObject objectOnMap(const std::string& mapId, const std::string& name) {
        auto it = otherMaps_.find(mapId);
        if (it == otherMaps_.end()) {
            auto loaded = fanren::io::loadTileMap((fs::path(assetRoot()) / "maps" / (mapId + ".tmj")).string());
            if (!loaded.ok) return MapObject{};
            it = otherMaps_.emplace(mapId, loaded.value).first;
        }
        for (const MapObject& object : it->second.objects) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }

    bool enterable(Point p) {
        if (!map().walkable(p)) return false;
        if (WorldScene::visibleNpcAt(app().state(), map(), p) != nullptr) return false;
        if (map().objectAt(p, "facility") != nullptr) return false;
        return true;
    }

    bool freeOfLiveEnterTrigger(Point p) {
        const MapObject* trigger = map().objectAt(p, "trigger");
        if (trigger == nullptr) return true;
        if (trigger->property("mode") != "enter") return true;
        return !WorldScene::triggerReady(app().state(), *trigger);
    }

    bool standable(Point p, bool avoidTriggers) {
        if (!enterable(p)) return false;
        if (map().objectAt(p, "portal") != nullptr) return false;
        return !avoidTriggers || freeOfLiveEnterTrigger(p);
    }

    bool step(Point direction) {
        const bool moved = world_.tryStep(app(), direction.x, direction.y);
        if (app().scripts().isRunning()) pumpScripts();
        return moved;
    }

    std::vector<Point> routeTo(Point goal, bool avoidTriggers) {
        const int width = map().width;
        const auto index = [width](Point p) { return p.y * width + p.x; };
        const Point from = app().state().position;
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
        if (app().state().position == goal) return true;
        std::vector<Point> path = routeTo(goal, /*avoidTriggers=*/true);
        if (path.empty() && !strict && !strictWalk_) path = routeTo(goal, /*avoidTriggers=*/false);
        if (path.empty()) return false;
        const std::string mapBefore = app().state().mapId;
        for (const Point& cell : path) {
            const Point here = app().state().position;
            if (!step(Point{cell.x - here.x, cell.y - here.y})) return false;
            if (app().state().mapId != mapBefore) return false;
        }
        return app().state().position == goal;
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
            if (app().state().position == stand && app().state().facing == facingOf(direction)) return true;
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
            if (!strict && strictWalk_) break;
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

    bool fireTrigger(const std::string& name) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app().state().mapId + " 上没有 " + name);
            return false;
        }
        if (!faceObject(trigger)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        drainKeys();
        if (!world_.interact(app())) {
            complain(name + " 点不着");
            return false;
        }
        if (!app().scripts().isRunning()) {
            complain(name + " 点着的不是脚本");
            return false;
        }
        pumpScripts();
        return true;
    }

    bool enterTrigger(const std::string& name) {
        const MapObject trigger = objectNamed(name);
        if (trigger.name.empty()) {
            complain(app().state().mapId + " 上没有 " + name);
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
        const bool handled = world_.interact(app());
        app().tick(kFrame);
        return handled;
    }

    bool usePortal(const std::string& name) {
        const MapObject portal = objectNamed(name);
        if (portal.name.empty()) {
            complain(app().state().mapId + " 上没有 " + name);
            return false;
        }
        const std::string before = app().state().mapId;
        if (!stepOnto(portal)) {
            complain("走不到 " + name + " 跟前");
            return false;
        }
        return app().state().mapId != before;
    }

    void closePanel() {
        app().popScene();
        app().tick(kFrame);
    }

    bool travelTo(const std::string& target) {
        for (int hop = 0; hop < 8; ++hop) {
            if (app().state().mapId == target) return true;
            const std::string portal = nextPortalToward(app().state().mapId, target);
            if (portal.empty()) {
                complain("从 " + app().state().mapId + " 没有门通到 " + target);
                return false;
            }
            if (!usePortal(portal)) {
                complain("从 " + app().state().mapId + " 走 " + portal + " 没过去");
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

    void expectDeniedWith(const std::string& denyKey, const std::string& why) {
        app().tick(kFrame);
        ASSERT_NE(app().topScene(), nullptr) << why;
        EXPECT_EQ(app().topScene()->name(), "Dialogue") << why << "：拦住玩家就要说明还差什么";
        ASSERT_FALSE(app().dialogueLog().empty()) << why;
        EXPECT_EQ(app().dialogueLog().back().body, app().data().lookupText(denyKey)) << why << "：拦下时说的不是 " << denyKey;
        closePanel();
    }

    // 一道闸门在此刻关着：走上去被拦下、说的是它自己的拦路话、人还在原图（施工图第 4 节五道闸）。
    void expectGateShut(const std::string& mapId, const std::string& portal, const std::string& requireFlag,
                        const std::string& denyKey) {
        ASSERT_TRUE(travelTo(mapId));
        const MapObject gate = objectNamed(portal);
        ASSERT_FALSE(gate.name.empty()) << mapId << " 上没有 " << portal;
        EXPECT_EQ(gate.property("require_flag"), requireFlag) << portal;
        ASSERT_EQ(flag(requireFlag), 0) << "先验：" << requireFlag << " 此刻还没置";
        tolerateBlocked_ = true;
        EXPECT_FALSE(usePortal(portal)) << portal << " 在 " << requireFlag << " 之前不该放行";
        tolerateBlocked_ = false;
        EXPECT_EQ(state().mapId, mapId) << "被拦下就该还在 " << mapId;
        expectDeniedWith(denyKey, portal);
    }

    // -----------------------------------------------------------------------
    // 药园：真的走到田边、开面板、挑畦、挑种 / 浇 / 采；真的走到蒲团打坐推日子
    // -----------------------------------------------------------------------
    FieldScene* openField(const std::string& facility) {
        if (!travelTo(kMapByy)) return nullptr;
        const MapObject object = objectNamed(facility);
        if (object.name.empty() || !pressAt(object)) {
            complain("百药园的 " + facility + " 开不了");
            return nullptr;
        }
        auto* panel = dynamic_cast<FieldScene*>(app().topScene());
        if (panel == nullptr) complain(facility + " 开出来的不是灵田面板");
        return panel;
    }

    CultivationScene* openCushion() {
        if (!travelTo(kMapByy)) return nullptr;
        const MapObject object = objectNamed("facility_dazuo");
        if (object.name.empty() || !pressAt(object)) {
            complain("居室的蒲团坐不上去");
            return nullptr;
        }
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (panel == nullptr) complain("蒲团开出来的不是修炼面板");
        return panel;
    }

    // 面板上列着的打坐档位（施工图 3.3「玩家推进」只用面板上有的那几档）：最短、最长各一。
    int shortestSit() {
        const auto& options = CultivationScene::meditateOptions(fanren::game::wordingStage(app().state()));
        int best = 1 << 30;
        for (const auto& option : options) best = std::min(best, option.days);
        return best;
    }
    int longestSit() {
        const auto& options = CultivationScene::meditateOptions(fanren::game::wordingStage(app().state()));
        int best = 0;
        for (const auto& option : options) best = std::max(best, option.days);
        return best;
    }

    // 在蒲团上一日一日地坐，坐到 until() 成立（R-3 提示：「瓶子一满就去浇」）。
    bool sitUntil(const std::function<bool()>& until, int guardDays = 4000) {
        if (until()) return true;
        CultivationScene* panel = openCushion();
        if (panel == nullptr) return false;
        const int oneSit = shortestSit();
        for (int sat = 0; sat < guardDays && !until(); sat += oneSit) panel->meditateFor(app(), oneSit);
        closePanel();
        return until();
    }

    const fanren::rules::FieldSlot* slotOf(const std::string& fieldId, int index) {
        const fanren::rules::SpiritField* field = state().findField(fieldId);
        if (field == nullptr || index < 0 || static_cast<std::size_t>(index) >= field->slots.size()) return nullptr;
        return &field->slots[static_cast<std::size_t>(index)];
    }

    // 拆包那一天把八株苗全种下（文件头「药园经营」）。
    bool plantEverything() {
        FieldScene* main = openField("facility_yaotian");
        if (main == nullptr) return false;
        bool ok = true;
        const std::vector<std::pair<int, const char*>> mainPlan = {
            {0, kHuangjing}, {1, kHuangjing}, {2, kHuangjing}, {3, kZishen}, {4, kZishen}, {5, kXuehong}};
        for (const auto& [slot, herb] : mainPlan) {
            if (!main->plantAt(app(), slot, herb)) {
                complain(std::string("药田第 ") + std::to_string(slot + 1) + " 畦种不下 " + herb + "：" + main->feedback());
                ok = false;
            }
        }
        closePanel();
        FieldScene* corner = openField("facility_field_jiao");
        if (corner == nullptr) return false;
        for (int slot = 0; slot < 2; ++slot) {
            if (!corner->plantAt(app(), slot, kHuangjingZhi)) {
                complain("园角第 " + std::to_string(slot + 1) + " 畦种不下黄精芝：" + corner->feedback());
                ok = false;
            }
        }
        closePanel();
        planted_ = ok;
        return ok;
    }

    // 苗还没足年：在蒲团上闭关一年（面板上最长的那一档）。
    bool waitUntilRipe(const std::string& fieldId, int slot) {
        const fanren::rules::FieldSlot* s = slotOf(fieldId, slot);
        if (s == nullptr) return false;
        if (s->ripe) return true;
        CultivationScene* panel = openCushion();
        if (panel == nullptr) return false;
        for (int guard = 0; guard < 4; ++guard) {
            s = slotOf(fieldId, slot);
            if (s == nullptr || s->ripe) break;
            panel->meditateFor(app(), longestSit());
        }
        closePanel();
        s = slotOf(fieldId, slot);
        return s != nullptr && s->ripe;
    }

    // 把 facility 那块田第 slot 畦浇到 targetAge 以上：瓶里没液就去坐到瓶满，再回来浇。
    bool pourUntil(const std::string& facility, const std::string& fieldId, int slot, int targetAge) {
        if (!waitUntilRipe(fieldId, slot)) {
            complain(fieldId + " 第 " + std::to_string(slot + 1) + " 畦等不到足年");
            return false;
        }
        for (int guard = 0; guard < 64; ++guard) {
            const fanren::rules::FieldSlot* s = slotOf(fieldId, slot);
            if (s == nullptr) return false;
            if (s->age >= targetAge) return true;
            if (state().bottle.drops <= 0) {
                if (!sitUntil([this] { return state().bottle.drops >= state().bottle.capacity; })) {
                    complain("坐到瓶满也没凝出绿液");
                    return false;
                }
            }
            FieldScene* panel = openField(facility);
            if (panel == nullptr) return false;
            bool poured = true;
            while (state().bottle.drops > 0 && slotOf(fieldId, slot)->age < targetAge) {
                if (!panel->matureAt(app(), slot)) {
                    complain(fieldId + " 第 " + std::to_string(slot + 1) + " 畦浇不进去：" + panel->feedback());
                    poured = false;
                    break;
                }
            }
            closePanel();
            if (!poured) return false;
        }
        return false;
    }

    bool harvest(const std::string& facility, const std::string& fieldId, int slot) {
        FieldScene* panel = openField(facility);
        if (panel == nullptr) return false;
        const bool ok = panel->harvestAt(app(), slot);
        if (!ok) complain(fieldId + " 第 " + std::to_string(slot + 1) + " 畦采不下来：" + panel->feedback());
        closePanel();
        return ok;
    }

    // 一味药按畦找出来（下种那一步定的畦位由 seedId 认，不写死畦号）。
    std::vector<int> slotsGrowing(const std::string& fieldId, const std::string& herb) {
        std::vector<int> out;
        const fanren::rules::SpiritField* field = state().findField(fieldId);
        if (field == nullptr) return out;
        for (std::size_t i = 0; i < field->slots.size(); ++i) {
            if (field->slots[i].seedId == herb) out.push_back(static_cast<int>(i));
        }
        return out;
    }

    bool growAndHarvest(const std::string& facility, const std::string& fieldId, const std::string& herb, int count,
                        int minAge) {
        std::vector<int> slots = slotsGrowing(fieldId, herb);
        if (static_cast<int>(slots.size()) < count) {
            complain(fieldId + " 里种着的 " + herb + " 不够 " + std::to_string(count) + " 畦");
            return false;
        }
        slots.resize(static_cast<std::size_t>(count));
        for (const int slot : slots) {
            if (!pourUntil(facility, fieldId, slot, minAge)) return false;
        }
        for (const int slot : slots) {
            if (!harvest(facility, fieldId, slot)) return false;
        }
        return state().itemCountAtLeastAge(herb, minAge) >= count;
    }

    // -----------------------------------------------------------------------
    // 坊市收药摊：真的走进去买（第一侧买两瓶清灵散：14、26c 两处「有则扣」各走一次「有」）
    // -----------------------------------------------------------------------
    bool shopAtYaotan(const Route& route) {
        if (!travelTo(kMapFangshi)) return false;
        const MapObject shop = objectNamed("facility_yaotan");
        if (shop.name.empty() || !pressAt(shop)) {
            complain("收药摊开不了");
            return false;
        }
        auto* panel = dynamic_cast<ShopScene*>(app().topScene());
        if (panel == nullptr) {
            complain("收药摊开出来的不是商店面板");
            return false;
        }
        fanren::rules::Shop* shopState = app().shop(panel->shopId());
        bool ok = shopState != nullptr;
        for (int i = 0; ok && i < route.buyQingling; ++i) {
            int entry = -1;
            for (std::size_t k = 0; k < shopState->entries.size(); ++k) {
                if (shopState->entries[k].itemId == kQingling) entry = static_cast<int>(k);
            }
            ok = entry >= 0 && panel->buyAt(app(), entry);
        }
        panel->leave(app());
        app().tick(kFrame);
        if (!ok) complain("收药摊的买卖没做成：" + panel->feedback());
        shopped_ = ok;
        return ok;
    }

    // -----------------------------------------------------------------------
    // 地火屋十九号：亲手开炉，炼到第一颗筑基丹成（施工图 3.2 节点 35b 末句、35c「亲手炼出至少一颗」）
    // -----------------------------------------------------------------------
    bool craftFirstPill() {
        if (!travelTo(kMapDihuo)) return false;
        const MapObject furnace = objectNamed("facility_dihuo");
        if (furnace.name.empty() || !pressAt(furnace)) {
            complain("十九号的圆墩开不了炉");
            return false;
        }
        auto* panel = dynamic_cast<AlchemyScene*>(app().topScene());
        if (panel == nullptr) {
            complain("圆墩开出来的不是炼制面板");
            return false;
        }
        const auto& ids = panel->recipeIds();
        const auto row = std::find(ids.begin(), ids.end(), "recipe_zhuji_dan");
        if (row == ids.end()) {
            complain("地火屋的炼制面板上没有筑基丹方（E1：ch07.cangshi 置过就该列出来）");
            panel->leave(app());
            app().tick(kFrame);
            return false;
        }
        const int index = static_cast<int>(row - ids.begin());
        const int before = state().itemCount(kZhuji);
        for (int i = 0; i < 200 && state().itemCount(kZhuji) <= before && state().itemCount(kYaofen) > 0; ++i) {
            if (!panel->craftAt(app(), index)) {
                complain("筑基丹方开不了工：" + panel->feedback());
                break;
            }
            ++crafts_;
            if (!steps_.empty()) ++steps_.back().yaofenCrafted;
        }
        panel->leave(app());
        app().tick(kFrame);
        return state().itemCount(kZhuji) > before;
    }

    // -----------------------------------------------------------------------
    // 禁地里的打坐点：一日一日地坐到满血（施工图 3.1 表末行：没有这三处，必打之前的存档可能是死档）
    // -----------------------------------------------------------------------
    bool restToFull(const std::string& mapId, const std::string& facility) {
        if (state().hp >= state().maxHp) return true;
        if (!travelTo(mapId)) return false;
        const MapObject object = objectNamed(facility);
        if (object.name.empty() || !pressAt(object)) {
            complain(mapId + " 的 " + facility + " 坐不上去");
            return false;
        }
        auto* panel = dynamic_cast<CultivationScene*>(app().topScene());
        if (panel == nullptr) {
            complain(facility + " 开出来的不是修炼面板");
            return false;
        }
        const int oneSit = shortestSit();
        for (int sat = 0; sat < 60 && state().hp < state().maxHp; sat += oneSit) panel->meditateFor(app(), oneSit);
        closePanel();
        return state().hp >= state().maxHp;
    }

    // -----------------------------------------------------------------------
    // 一步：走过去、演一趟
    // -----------------------------------------------------------------------
    bool doFarm(Step s) {
        switch (s) {
            case Step::Xiazhong: return plantEverything();
            case Step::CuiHuangjing:
                return growAndHarvest("facility_yaotian", kField, kHuangjing, kHuangjingDue, kHuangjingAge);
            case Step::CuiZishen: return growAndHarvest("facility_yaotian", kField, kZishen, kZishenDue, kZishenAge);
            case Step::CuiXuehong: return growAndHarvest("facility_yaotian", kField, kXuehong, 1, kXuehongAge);
            case Step::CuiZhi:
                return growAndHarvest("facility_field_jiao", kFieldJiao, kHuangjingZhi, kQiannianDue, kQiannianAge);
            default: return false;
        }
    }

    bool attemptStep(Step s) {
        const StepInfo& info = infoOf(s);
        switch (info.kind) {
            case Kind::Farm: return doFarm(s);
            case Kind::Shop: return shopAtYaotan(*route_);
            case Kind::Craft: return craftFirstPill();
            case Kind::Rest: return restToFull(info.mapId, info.object);
            case Kind::Enter:
            case Kind::Interact:
                break;
        }
        if (!travelTo(info.mapId)) return false;
        return info.kind == Kind::Enter ? enterTrigger(info.object) : fireTrigger(info.object);
    }

    bool isDone(Step s) {
        switch (s) {
            case Step::Xiazhong: return planted_;
            case Step::CuiHuangjing:
                return flag("ch07.jiaoyao1") != 0 || state().itemCountAtLeastAge(kHuangjing, kHuangjingAge) >= kHuangjingDue;
            case Step::CuiZishen:
                return flag("ch07.danbao") != 0 || state().itemCountAtLeastAge(kZishen, kZishenAge) >= kZishenDue;
            case Step::CuiXuehong: return flag("ch07.baigong") != 0 || state().itemCountAtLeastAge(kXuehong, kXuehongAge) >= 1;
            case Step::CuiZhi: return state().itemCountAtLeastAge(kHuangjingZhi, kQiannianAge) >= kQiannianDue;
            case Step::Yaotan: return shopped_;
            case Step::RestShulin:
            case Step::RestYujian:
            case Step::RestXiaoshidian:
            case Step::RestDidao: return state().hp >= state().maxHp;
            case Step::Liandan: return flag("ch07.chengdan") != 0 || state().itemCount(kZhuji) >= kLuZhuji + kRewardZhuji + 1;
            default: return flag(infoOf(s).doneFlag) != 0;
        }
    }

    bool runStep(Step s) {
        for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
            beginStep(s);
            const bool started = attemptStep(s);
            endStep();
            if (HasFatalFailure() || app().quitRequested()) return false;
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
        for (const char* herb : {kYusui, kZihou, kTianling}) record.matureBefore[herb] = state().itemCountAtLeastAge(herb, kSniffAge);
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
            if ((info.step == Step::CuiXuehong || info.step == Step::Baigong) && !r.z1) continue;
            if ((info.step == Step::RestYujian || info.step == Step::YujianA || info.step == Step::YujianB) && !r.z2) {
                continue;
            }
            if (info.step == Step::Yaotan && !r.shops()) continue;
            out.push_back(info.step);
        }
        return out;
    }

    // 整章走一遍。before / after 是用例插断言的钩子。返回「走到了 ch07.done」。
    bool playChapter(const Route& route, const std::function<void(Step)>& before = {},
                     const std::function<void(Step)>& after = {}) {
        route_ = &route;
        for (Step s : stepsFor(route)) {
            if (before) before(s);
            if (HasFatalFailure()) return false;
            const bool done = runStep(s);
            if (s == Step::Danbao && done) lingshiAfterDanbao_ = state().itemCount(kLingshi);
            if (after) after(s);
            if (HasFatalFailure()) return false;
            if (app().quitRequested()) {
                stuckAt_ = std::string(infoOf(s).node) + " " + infoOf(s).name + "（game over）";
                return false;
            }
            if (!done) {
                stuckAt_ = std::string(infoOf(s).node) + " " + infoOf(s).name;
                return false;
            }
        }
        return flag("ch07.done") == 1;
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
                << b.pillsBefore << "→" << b.pillsAfter << "  金疮药 " << b.salvesBefore << "→" << b.salvesAfter
                << (b.opened ? "  （开场出了那一招）" : "");
        }
        return out.str();
    }

    int playerDays() const {
        int total = 0;
        for (const StepRecord& r : steps_) {
            if (r.finished && infoOf(r.step).days < 0) total += r.dayAfter - r.dayBefore;
        }
        return total;
    }

    // -----------------------------------------------------------------------
    // 整章走完之后的判卷
    // -----------------------------------------------------------------------
    // 剧情挂点自己拨的日子逐步对 3.3（施工偏差 18.6 那一天在 32a）；玩家自己推的日子另记、不判。
    void expectEveryStoryStepTookTheDesignDays() {
        int scripted = 0;
        for (const StepRecord& record : steps_) {
            if (!record.finished) continue;
            const int want = infoOf(record.step).days;
            const int took = record.dayAfter - record.dayBefore;
            if (want < 0) continue;
            scripted += took;
            EXPECT_EQ(took, want) << infoOf(record.step).node << " " << infoOf(record.step).name << " 走了 " << took
                                  << " 天，施工图 3.3 该是 " << want << " 天";
        }
        EXPECT_EQ(scripted, kScriptedDays) << "施工图 3.3：剧情脚本拨的日子合计（含施工偏差 18.6）";
        EXPECT_EQ(state().day - startDay_, kScriptedDays + playerDays()) << "日子只有两个来处：剧情拨的、玩家坐的";
    }

    // 每一步背包的进出与施工图 3.2 / 第 9 节一格不差。仗里吃掉的药与开场用掉的符另算：只许少不许多。
    std::map<std::string, int> designBagDiff(const StepRecord& r) {
        std::map<std::string, int> d;
        const auto had = [&](const char* id) {
            const auto it = r.bagBefore.find(id);
            return it == r.bagBefore.end() ? 0 : it->second;
        };
        const auto add = [&d](const char* id, int n) {
            if (n != 0) d[id] += n;
        };
        switch (r.step) {
            case Step::Chaibao:
                add(kHuangjing, 3); add(kZishen, 2); add(kXuehong, 1); add(kHuangjingZhi, 2);
                add(kZhong, 2); add(kJinggangHuan, 1); add("story_heiwu_qi", 1); add("story_huangtong_ping", 1);
                add(kTulaoFu, 2);
                break;
            case Step::Jiaoyao: add(kHuangjing, -kHuangjingDue); add(kLingshi, kPayYearOne); break;
            case Step::Danbao: add(kZishen, -kZishenDue); add(kLingshi, kPayYearTwo); break;
            case Step::Baigong: add(kXuehong, -1); break;
            case Step::Cangshi: add(kLingshi, -kCangshiFee); break;
            case Step::DihuoWen: add(kLingshi, -kDingPrice); add("story_yinsi_ding", 1); break;
            case Step::Wanbaolou:
                add(kHuangjingZhi, -kQiannianDue); add("story_jinfu_zimuren", 1); add("story_xuantie_dun", 1);
                add(kTianleizi, 1); add(kJinguangzhuan, 1);
                break;
            case Step::Yeyu:
                add(kJianfuItem, -1); add(kJinggangHuan, -1); add(kZhuji, kLuZhuji); add("story_qingjiao_qi", 1);
                add(kQingSuo, 1); add(kYinGou, 1); add(kHuoqiuFu, kLuHuoqiu); add(kHushenFu, kLuHushen);
                add(kLingshi, kLuLingshi);
                if (had(kDingshenFu) > 0) add(kDingshenFu, -1);
                break;
            case Step::Fanhui:
                if (had(kQingling) > 0) add(kQingling, -1);
                break;
            case Step::MaSongyao: {
                const int n = flag("ch07.baigong") != 0 ? 5 : 3;   // 第 6 节 Z1：做了各 5，没做各 3
                add(kPill, n); add(kSalve, n);
                break;
            }
            // 校对整改 16.5（LOW-3）：点破名字之前给的是「布包」「小圆镜」「黑靴」，点破那一节换成正名
            // （18 → 34 傀儡弓手、23 → 28 青凝镜、23 → 24 踏云靴；与 maifu_shaonv → han_yunzhi 同一个办法）。
            case Step::Jihe: add(kZhong, 1); add("story_bubao_faqi", 1); break;
            case Step::Liangshi: add(kSixian, 1); break;
            case Step::Shulin:
                if (had(kYinGou) > 0) add(kYinGou, -1);
                if (had(kQingSuo) > 0) add(kQingSuo, -1);
                add("story_xiao_yuanjing", 1); add("story_shuijing_qiu", 1); add(kXiaodaoFubao, 1); add("story_hei_xue", 1);
                break;
            case Step::Tongmen: add("story_hei_xue", -1); add(kTayun, 1); break;
            case Step::Youdi:
                add(kZihou, 4); add("material_wugong_ke", 3);
                if (had(kQingling) > 0) add(kQingling, -1);
                break;
            case Step::Sichu: add(kYusui, 4); add(kTianling, 2); add(kZihou, 2); break;
            case Step::YujianA: add(kYusui, 2); break;
            case Step::YujianB: add(kTianling, 2); break;
            case Step::Xiaoshidian:
                add("story_xiao_yuanjing", -1); add("story_qingning_jing", 1);
                add("story_yinhui_jian", 1); add(kTianling, 4);
                break;
            case Step::Jiaoshi:
                add(kYusui, 8 + 2); add(kZihou, 7); add(kTianling, 7); add("material_mojiao_cailiao", 1);
                add(kJinguangzhuan, -1);
                break;
            case Step::Chukou: {
                for (const char* herb : {kYusui, kZihou, kTianling}) add(herb, -r.matureBefore.at(herb));
                add(kBiguang, 1);
                if (flag("ch07.yujian_a") != 0 && flag("ch07.yujian_b") != 0) {
                    add(kLingshi, kZ2Lingshi);
                    add(kZhong, kZ2Zhong);
                }
                break;
            }
            case Step::Sannian:
                add(kZhuji, kRewardZhuji); add(kZhong, kTallyZhong + kYeZhong); add(kLingshi, kTallyLingshi);
                add("story_yinse_shuye", 1); add(kYaofen, kYaofenGiven); add(kHuiqi, kYeHuiqi); add(kJianjue, 1);
                add("story_bubao_faqi", -1); add("story_kuilei_gongshou", 1);
                for (const char* herb : {kYusui, kZihou, kTianling}) add(herb, -had(herb));
                break;
            case Step::Chouhan: {
                int lingshi = had(kLingshi);
                if (lingshi >= 1) {
                    add(kLingshi, -1);
                    --lingshi;
                }
                if (had(kZhong) > 0) {
                    add(kZhong, -1);
                } else if (lingshi >= 100) {
                    add(kLingshi, -100);
                }
                break;
            }
            case Step::Shijiu:
                if (had(kYaofen) >= kYaofenWasted) {
                    add(kYaofen, -kYaofenWasted);
                    add("story_feidan_yuhe", 1);
                }
                break;
            case Step::Banian:
                add(kYaofen, -had(kYaofen));
                if (had(kZhuji) < kZhujiTopUp) add(kZhuji, kZhujiTopUp - had(kZhuji));
                break;
            case Step::Zhuji: add(kZhuji, -kZhujiEaten); break;
            default: break;
        }
        return d;
    }

    void expectEveryStoryStepMovedTheBagTheDesignSays() {
        for (const StepRecord& r : steps_) {
            const Kind kind = infoOf(r.step).kind;
            if (!r.finished || kind == Kind::Farm || kind == Kind::Shop || kind == Kind::Craft || kind == Kind::Rest) continue;
            std::map<std::string, int> got = diffOf(r.bagBefore, r.bagAfter);
            if (!r.phases.empty()) {
                std::set<std::string> spendable = {kPill, kSalve};
                spendable.insert(r.usedInBattle.begin(), r.usedInBattle.end());
                const std::map<std::string, int> design = designBagDiff(r);
                for (const std::string& id : spendable) {
                    const auto it = got.find(id);
                    if (it == got.end()) continue;
                    const int designed = design.count(id) ? design.at(id) : 0;
                    const int spent = it->second - designed;
                    EXPECT_LE(spent, 0) << infoOf(r.step).node << " 那一仗之后 " << id << " 反倒多了 " << spent;
                    if (designed == 0) {
                        got.erase(it);
                    } else {
                        it->second = designed;
                    }
                }
            }
            EXPECT_EQ(showBag(got), showBag(designBagDiff(r)))
                << infoOf(r.step).node << " " << infoOf(r.step).name << " 背包的进出与施工图 3.2 / 第 9 节不符";
        }
    }

    // 学、忘法术，升境，只在施工图写的那几步（验收 5、7）。
    void expectMagicsAndRealmMoveOnlyWhereTheDesignSays() {
        for (const StepRecord& r : steps_) {
            if (!r.finished) continue;
            std::vector<std::string> learned;
            std::vector<std::string> forgot;
            for (const std::string& id : r.magicsAfter) {
                if (std::find(r.magicsBefore.begin(), r.magicsBefore.end(), id) == r.magicsBefore.end()) learned.push_back(id);
            }
            for (const std::string& id : r.magicsBefore) {
                if (std::find(r.magicsAfter.begin(), r.magicsAfter.end(), id) == r.magicsAfter.end()) forgot.push_back(id);
            }
            std::sort(learned.begin(), learned.end());
            std::sort(forgot.begin(), forgot.end());
            std::vector<std::string> wantLearned;
            std::vector<std::string> wantForgot;
            if (r.step == Step::Wanbaolou) wantLearned = {kMagicJinfu, kMagicJinguangzhuan};
            if (r.step == Step::Yeyu) {
                wantLearned = {kMagicQingjiao};
                wantForgot = {kMagicJianfu};
            }
            if (r.step == Step::Shulin) wantLearned = {kMagicQingning};
            if (r.step == Step::Jiaoshi) wantForgot = {kMagicJinguangzhuan};
            std::sort(wantLearned.begin(), wantLearned.end());
            EXPECT_EQ(learned, wantLearned) << infoOf(r.step).node << " 学会的法术与验收 7 不符";
            EXPECT_EQ(forgot, wantForgot) << infoOf(r.step).node << " 忘掉的法术与验收 7 不符（剑符 13、金光砖 31）";
            Realm want = r.realmBefore;
            if (r.step == Step::Danbao) want = Realm::QiRefining11;
            if (r.step == Step::Zhuji) want = Realm::FoundationEarly;
            EXPECT_EQ(r.realmAfter, want) << infoOf(r.step).node << " 的境界变化与施工图 1.3 不符（节点 3 到十一层、36 到筑基）";
        }
    }

    // 五场必打一场不少（验收 15 走一遍那一半）；每一场认得出；⑤ 我方有白衣少女。
    void expectTheFiveMustFightsAllHappened() {
        const std::vector<std::string> ids = battleIds();
        for (std::size_t i = 0; i < std::size(kMustFight); ++i) {
            EXPECT_EQ(std::count(ids.begin(), ids.end(), kMustFight[i]), 1)
                << kMustFightMark[i] << " " << kMustFight[i] << " 这一趟该打恰好一次（施工图 8.0）：" << battleLine();
            if (const BattleRecord* b = lastBattle(kMustFight[i])) {
                EXPECT_EQ(b->phase, BattlePhase::Won) << kMustFightMark[i] << " 是生死仗（8.2「败」一列：致命）" << battleLine();
                const std::multiset<std::string> allies(b->allyIds.begin(), b->allyIds.end());
                const std::multiset<std::string> design = i == 4 ? std::multiset<std::string>{"hanli", "baiyi_shaonv"}
                                                                  : std::multiset<std::string>{"hanli"};
                EXPECT_EQ(allies, design) << kMustFightMark[i] << "：全章无同伴，⑤ 的白衣少女是编成友军（8.2）";
            }
        }
        for (const std::string& id : ids) EXPECT_NE(id, "?") << "有一场认不出是哪一张编成：" << battleLine();
    }

    // 验收 2 的终点先验（施工图第 15 节第 2 行，逐条抄）。
    void expectTheChapterEndState() {
        GameState& s = state();
        EXPECT_EQ(s.flag("ch07.done"), 1);
        EXPECT_EQ(s.realm, Realm::FoundationEarly) << "验收 2：realm == 21";
        EXPECT_EQ(s.realmCap, Realm::FoundationEarly) << "验收 2：realmCap == 21";
        EXPECT_GE(s.maxHp, 260) << "验收 2：maxHp ≥ 260";
        EXPECT_GE(s.maxMp, 180) << "验收 2：maxMp ≥ 180";
        EXPECT_EQ(s.bottle.capacity, 6) << "验收 2 / E3：bottle.capacity == 6";
        const std::set<std::string> eight = {kMagicHuodan, kMagicYufeng, kMagicTianyan, kMagicLiusha, kMagicBingdong,
                                             kMagicJinfu,  kMagicQingjiao, kMagicQingning};
        EXPECT_EQ(s.learnedMagics.size(), 8u) << "验收 2：learnedMagics 恰 8 门";
        EXPECT_EQ(std::set<std::string>(s.learnedMagics.begin(), s.learnedMagics.end()), eight)
            << "验收 2：火弹、御风决、天眼、流沙、冰冻、祭金蚨子母刃、祭青蛟旗、祭青凝镜";
        EXPECT_EQ(s.itemCount(kZhuji), kZhujiAtEnd) << "验收 2：pill_zhuji_dan == 17";
        EXPECT_EQ(s.itemCount(kJianfuItem), 0) << "验收 2：talisman_jianfu == 0";
        EXPECT_EQ(s.itemCount(kJinguangzhuan), 0) << "验收 2：talisman_jinguangzhuan == 0";
        EXPECT_EQ(s.itemCount(kSixian), 1) << "验收 2：weapon_wuming_sixian == 1";
        EXPECT_EQ(s.itemCount(kTayun), 1) << "验收 2：story_tayun_xue == 1";
        EXPECT_EQ(s.itemCount(kJianjue), 1) << "验收 2：story_qingyuan_jianjue == 1";
        EXPECT_EQ(s.itemCount(kBiguang), 1) << "验收 2：weapon_biguang_dao == 1";
        const fanren::rules::SpiritField* main = s.findField(kField);
        const fanren::rules::SpiritField* corner = s.findField(kFieldJiao);
        ASSERT_NE(main, nullptr) << "验收 2：fields 含 field_baiyaoyuan";
        ASSERT_NE(corner, nullptr) << "验收 2：fields 含 field_baiyaoyuan_jiao";
        EXPECT_EQ(main->slots.size(), 6u) << "验收 2：field_baiyaoyuan（6）";
        EXPECT_EQ(corner->slots.size(), 2u) << "验收 2：field_baiyaoyuan_jiao（2）";
        for (const char* herb : {kYusui, kZihou, kTianling}) {
            EXPECT_EQ(s.itemCountAtLeastAge(herb, kSniffAge), 0) << "验收 2：" << herb << " count_aged(…, 100) == 0";
        }
        EXPECT_TRUE(s.party.empty()) << "施工图第 2 节：全章无同伴";
        EXPECT_EQ(s.mapId, kMapByy) << "施工图 3.1 节点 36：末尾 teleport 百药园";
    }

    // 施工图 12.2 的章内先后，**按玩家实际读到的顺序**判：每个词头一回被说出来在哪一节。
    std::map<std::string, std::string> firstNodeOfEachGatedWord() {
        std::map<std::string, std::string> first;
        for (const auto& [step, key] : chapterLog_) {
            const std::string text = app().data().lookupText(key);
            for (const char* word : {"岳麓殿", "定颜丹", "血色试炼", "师祖", "菡云芝", "南宫", "李化元", "中阶灵石"}) {
                if (text.find(word) != std::string::npos && first.count(word) == 0) first[word] = infoOf(step).node;
            }
        }
        return first;
    }

    void expectGatedWordsArriveWhereTheDesignSays() {
        ASSERT_GT(chapterLog_.size(), 300u) << "先验：整章说过的句子够多，下面的「没有」才有分量";
        std::map<std::string, std::string> first = firstNodeOfEachGatedWord();
        EXPECT_EQ(first["岳麓殿"], "3") << "施工图 3.2 节点 3：「岳麓殿」三个字在这里第一次出现（12.2：节点 1、2 不含）";
        EXPECT_EQ(first["定颜丹"], "4b") << "施工图 12.2：节点 1-4a 不含「定颜丹」，4b 红玉筒是定颜丹方";
        EXPECT_EQ(first["血色试炼"], "8") << "施工图 3.2 节点 8：「血色试炼」四个字在这里第一次出现";
        EXPECT_EQ(first["师祖"], "18") << "施工图 12.2：节点 1-17 不含「师祖」，18 李师祖入殿";
        EXPECT_EQ(first["菡云芝"], "28") << "施工图 3.2 节点 28：她报出自己叫菡云芝";
        EXPECT_EQ(first["南宫"], "31") << "施工图 12.2：南宫二字从节点 31 问名那一句起";
        EXPECT_EQ(first["李化元"], "33") << "施工图 3.2 节点 33：「李化元」首次出口（马师伯说的）";
        EXPECT_EQ(first["中阶灵石"], "1") << "施工图 12.2：中阶灵石本章起说（节点 1 起）";
        for (const auto& [step, key] : chapterLog_) {
            const std::string text = app().data().lookupText(key);
            for (const char* word : {"血禁", "陈巧倩", "淫囊", "合欢", "董家", "红拂", "马云龙", "寒天涯", "萧二",
                                     "封闭前最后一次"}) {
                EXPECT_EQ(text.find(word), std::string::npos)
                    << key << "（节点 " << infoOf(step).node << "）说了「" << word << "」（施工图 12.2 / 验收 17：本章 0 处）";
            }
        }
    }

    const fanren::core::Quest* questOf(const std::string& questId) {
        for (const fanren::core::Quest& q : app().data().quests) {
            if (q.id == questId) return &q;
        }
        return nullptr;
    }
    fanren::rules::QuestStatus statusOf(const std::string& questId) {
        const fanren::core::Quest* quest = questOf(questId);
        if (quest == nullptr) return fanren::rules::QuestStatus::NotAccepted;
        return fanren::rules::questStatus(*quest, app().data().objectives, state());
    }

    static Route firstSideRoute() {
        Route r;
        r.name = "第一侧";
        r.fallback = 0;
        r.answers[Step::Charen] = {0};   // 插刃：一个选项按八次（施工偏差 18.5）
        r.z1 = true;
        r.z2 = true;
        r.buyQingling = 2;
        r.openers["b07_yixiantian"] = Opener{kTulaoFu, "luosai_huzi"};   // 8.2 ②：土牢符困住胡子
        r.openers["b07_fengyue"] = Opener{kTianleizi, "feng_yue"};       // 8.2 ③：天雷子一掷定局
        return r;
    }
    static Route secondSideRoute() {
        Route r;
        r.name = "第二侧";
        r.fallback = 1;
        r.answers[Step::Charen] = {0};
        return r;
    }

    // 一局 game over 之后 Application 就一直是「要退出」的（quitRequested 没有复位的口）：
    // 要接着再走一趟，就换一个新的（renewApp）。
    std::unique_ptr<Application> appHolder_ = std::make_unique<Application>();
    WorldScene world_;
    fanren::test::HandPolicy policy_ = intentPolicy();
    const Route* route_ = nullptr;
    const HandOver* side_ = nullptr;
    std::map<Step, int> answered_;
    std::map<std::string, TileMap> otherMaps_;
    int startDay_ = 1;
    int startLingshi_ = 0;
    int startZhong_ = 0;
    int startQianyao_ = 0;
    int lingshiAfterDanbao_ = -1;
    int crafts_ = 0;
    bool tolerateBlocked_ = false;
    bool shopped_ = false;
    bool planted_ = false;
    bool strictWalk_ = false;   // 只走不踩备着的踏入型挂点的路（绕不过去那一条用）
    Step currentStep_ = Step::Chaibao;
    std::vector<BattleRecord> battles_;
    std::vector<StepRecord> steps_;
    std::vector<std::pair<Step, std::string>> chapterLog_;
    std::string stuckAt_;
    std::set<std::string> throwFights_;

    // ---- 两条通关共用的几样判卷 ----

    // 某一步里说过的、正文同时含这几个词的那一句（验收 17、18 用：判据是施工图写的词，key 只拿来认出是哪一句）。
    std::string spokenWith(Step step, const std::vector<std::string>& words) {
        for (const StepRecord& record : steps_) {
            if (record.step != step) continue;
            for (const std::string& key : record.keys) {
                const std::string text = app().data().lookupText(key);
                const bool all = std::all_of(words.begin(), words.end(),
                                             [&text](const std::string& w) { return text.find(w) != std::string::npos; });
                if (all) return key;
            }
        }
        return {};
    }

    // 目标行此刻是这一步、而且说的就是 3.4 那一格 R-3 提示的原文（验收 17 在第 6 章的口径）。
    void expectHint(const std::string& stepId, const std::string& phrase) {
        const auto* obj = fanren::rules::currentObjective(app().data().objectives, state());
        ASSERT_NE(obj, nullptr) << "目标行空了，该是 " << stepId;
        EXPECT_EQ(obj->id, stepId);
        EXPECT_NE(app().data().lookupText(obj->textKey).find(phrase), std::string::npos)
            << obj->id << " 的目标行该含 3.4 的 R-3 提示「" << phrase << "」：" << app().data().lookupText(obj->textKey);
        hints_.push_back(obj->id);
    }

    bool visibleOn(const std::string& mapId, const std::string& npc) {
        const MapObject object = objectOnMap(mapId, npc);
        EXPECT_FALSE(object.name.empty()) << mapId << " 上没有 " << npc;
        return WorldScene::npcVisible(state(), object);
    }

    std::vector<std::string> alchemyRecipesListed() {
        std::vector<std::string> ids;
        for (const auto* recipe :
             AlchemyScene::visibleRecipes(app().recipes(), fanren::rules::CraftKind::Alchemy, state())) {
            ids.push_back(recipe->id);
        }
        return ids;
    }

    // 两侧共有的节点检查（before / after 钩子里调）。返回的是「这一步之前 / 之后」该看的东西。
    void commonBefore(Step step) {
        switch (step) {
            case Step::Cangshi: {
                // E1 / 验收 8：藏室之前炼制面板上没有这两张方子（名字不许提前露面）。
                const auto listed = alchemyRecipesListed();
                for (const char* id : {"recipe_zhuji_dan", "recipe_dingyan_dan"}) {
                    EXPECT_EQ(std::count(listed.begin(), listed.end(), id), 0) << "ch07.cangshi 之前面板上就有 " << id;
                }
                break;
            }
            case Step::Murong:
                for (const char* npc : {"npc_murong_xiong", "npc_murong_di", "npc_lu_shixiong", "npc_chen_shimei",
                                        "npc_lanyi_nvzi", "npc_cuai_qingnian", "npc_huangfeng_weiguan"}) {
                    EXPECT_TRUE(visibleOn(kMapHfg, npc)) << npc << "：施工图第 5 节「山头七人 visible ch07.chouhan」";
                }
                break;
            case Step::Jihe:
                EXPECT_TRUE(visibleOn(kMapHfg, "npc_li_huayuan")) << "施工图第 5 节：大殿李师祖 visible ch07.ma_songyao";
                break;
            case Step::Chukou:
                seedlingsBeforeChukou_.clear();
                for (const char* herb : {kYusui, kZihou, kTianling}) {
                    seedlingsBeforeChukou_[herb] = state().itemCount(herb) - state().itemCountAtLeastAge(herb, kSniffAge);
                }
                break;
            default: break;
        }
    }

    void commonAfter(Step step) {
        GameState& s = state();
        switch (step) {
            case Step::Chaibao: {
                const fanren::rules::SpiritField* corner = s.findField(kFieldJiao);
                ASSERT_NE(corner, nullptr) << "施工图 3.2 节点 1：field.unlock(\"field_baiyaoyuan_jiao\", 2)";
                EXPECT_EQ(corner->slots.size(), 2u);
                break;
            }
            case Step::Danbao:
                EXPECT_EQ(s.realm, Realm::QiRefining11) << "施工图 3.2 节点 3：realm.advance(11)";
                EXPECT_EQ(s.realmCap, Realm::QiRefining11) << "上限随之到十一层（1.3）";
                EXPECT_EQ(s.maxHp, fanren::rules::realmMaxHp(Realm::QiRefining11)) << "1.3：读表";
                EXPECT_EQ(s.maxMp, fanren::rules::realmMaxMp(Realm::QiRefining11)) << "1.3：读表";
                // 验收 11 / 第 9 节第 1 条：两次交药之后灵石 = 起点 + 24 + 60，两侧同型（差只剩起点那一笔差）。
                EXPECT_EQ(s.itemCount(kLingshi), startLingshi_ + kPayYearOne + kPayYearTwo)
                    << "第 9 节第 1 条：节点 4b 之前灵石只进不出，两次交药进 24、60";
                break;
            case Step::Cangshi: {
                const auto listed = alchemyRecipesListed();
                for (const char* id : {"recipe_zhuji_dan", "recipe_dingyan_dan"}) {
                    EXPECT_EQ(std::count(listed.begin(), listed.end(), id), 1) << "E1：置 ch07.cangshi 即列出 " << id;
                }
                break;
            }
            case Step::Murong:
                for (const char* npc : {"npc_murong_xiong", "npc_murong_di", "npc_lu_shixiong", "npc_chen_shimei",
                                        "npc_lanyi_nvzi", "npc_cuai_qingnian", "npc_huangfeng_weiguan"}) {
                    EXPECT_FALSE(visibleOn(kMapHfg, npc)) << npc << "：戏一演完全部撤场（hidden ch07.murong）";
                }
                break;
            case Step::Jihe:
                EXPECT_FALSE(visibleOn(kMapHfg, "npc_li_huayuan")) << "施工图第 5 节：大殿李师祖 hidden ch07.jihe";
                EXPECT_EQ(s.mapId, kMapWai) << "施工图 3.1 节点 18：末尾 teleport 禁地外";
                break;
            case Step::Liedui:
                // 施工偏差 18.14：ch06.qianyao > 0 才有那两句，说完就了结；== 0 一个字不提。
                EXPECT_EQ(spokeIn(Step::Liedui, "ch07.liedui.qian"), startQianyao_ > 0)
                    << "卖符少女的欠账（起点 ch06.qianyao = " << startQianyao_ << "）";
                EXPECT_EQ(s.flag("ch06.qianyao"), 0) << "施工偏差 18.14：说完就了结";
                break;
            case Step::Pojin: EXPECT_EQ(s.mapId, kMapWaiwei) << "施工图 3.1 节点 20：末尾 teleport 外围"; break;
            case Step::Qingshidian: EXPECT_EQ(s.mapId, kMapZhaoze) << "施工图 3.1 节点 29：末尾 teleport 沼泽"; break;
            case Step::Jiaoshi: EXPECT_EQ(s.mapId, kMapHxs) << "施工图 3.1 节点 31：末尾 teleport 环形山"; break;
            case Step::Xiashan: EXPECT_EQ(s.mapId, kMapWai) << "施工图 3.1 节点 32a：末尾 teleport 禁地外"; break;
            case Step::Chukou:
                // 验收 13 / 第 9 节第 5 条：百年以上一株不留，幼苗一株不少。
                for (const char* herb : {kYusui, kZihou, kTianling}) {
                    EXPECT_EQ(s.itemCountAtLeastAge(herb, kSniffAge), 0) << herb << "：嗅灵兽之后百年以上都是 0";
                    EXPECT_EQ(s.itemCount(herb), seedlingsBeforeChukou_[herb]) << herb << "：幼苗（< 100 年）一株不少";
                    EXPECT_GT(seedlingsBeforeChukou_[herb], 0) << "先验：" << herb << " 真有幼苗可留";
                }
                EXPECT_EQ(s.mapId, kMapByy) << "施工图 3.1 节点 32b：末尾 teleport 百药园";
                for (const char* npc : {"npc_li_huayuan_chukou", "npc_han_yunzhi_chukou", "npc_zhong_wu_chukou"}) {
                    EXPECT_FALSE(visibleOn(kMapWai, npc)) << npc << "：出口一批 hidden ch07.chujindi";
                }
                break;
            case Step::Sannian:
                EXPECT_EQ(s.itemCount(kYaofen), kYaofenGiven) << "施工图 3.2 节点 34：药粉一律 40 份";
                for (const char* herb : {kYusui, kZihou, kTianling}) EXPECT_EQ(s.itemCount(herb), 0) << herb << " 余量全 take";
                break;
            case Step::Zhuji:
                EXPECT_EQ(s.bottle.capacity, 6) << "E3：筑基之后瓶子 6 滴";
                break;
            default: break;
        }
    }

    // 两侧同一套的章末判卷。
    void expectTheCommonEnding() {
        expectTheFiveMustFightsAllHappened();
        expectEveryStoryStepTookTheDesignDays();
        expectEveryStoryStepMovedTheBagTheDesignSays();
        expectMagicsAndRealmMoveOnlyWhereTheDesignSays();
        expectTheChapterEndState();
        expectGatedWordsArriveWhereTheDesignSays();
        // 验收 17：交代紧迫感的那一句真的说出来了。
        EXPECT_FALSE(spokenWith(Step::Fudan, {"五年", "六十年", "最后"}).empty())
            << "验收 17：节点 15 该说出「五年后那一届是封山前最后一回」那一句（同时含五年、六十年、最后）";
        // 验收 18：南宫婉登舟那一句（含「没」「看」）真的上了屏。
        EXPECT_FALSE(spokenWith(Step::Chukou, {"南宫婉", "没", "看"}).empty())
            << "验收 18：节点 32b 南宫婉上神舟、自始至终没看他一眼";
        // 第 1.4 节：吴风开口第一句按 ch06.wufeng 分。
        const bool lost = state().flag("ch06.wufeng") == 2;
        EXPECT_EQ(spokeIn(Step::Lianqi, "ch07.chuangong.open_lost"), lost) << "1.4：ch06.wufeng == 2 说输的那一句";
        EXPECT_EQ(spokeIn(Step::Lianqi, "ch07.chuangong.open_won"), !lost) << "1.4：ch06.wufeng == 1 说赢的那一句";
        // 1.4：叶师叔补还按 ch06.huinuo / ch06.rangdan 两面各有一句，东西一样。
        const bool cold = state().flag("ch06.huinuo") == 2 || state().flag("ch06.rangdan") == 2;
        EXPECT_EQ(spokeIn(Step::Sannian, "ch07.sannian.ye_cold"), cold);
        EXPECT_EQ(spokeIn(Step::Sannian, "ch07.sannian.ye_warm"), !cold);
        // 3.2 节点 35c：地火屋亲手开炉，至少一颗成了才盘点（Liandan 那一步）。
        ASSERT_NE(recordOf(Step::Liandan), nullptr);
        EXPECT_GE(crafts_, 1) << "地火是玩家亲手开的炉";
    }

    // 切片的起点：第一侧的交接存档，把目标链上 step 之前各步的完成旗标置上（施工图 3.1 的次序；
    // 药园、坊市、打坐那几步不是旗标，跳过），境界按节点 3 之后的十一层、满血满法力，站在 step 那张图的出生点。
    void standJustBefore(Step step) {
        startFromChapterSixEnding(kFirstSide);
        if (HasFatalFailure()) return;
        GameState& s = state();
        for (const StepInfo& info : stepTable()) {
            if (info.step == step) break;
            if (info.doneFlag[0] != '\0') s.setFlag(info.doneFlag, 1);
        }
        s.realm = s.realmCap = Realm::QiRefining11;
        s.hp = s.maxHp = fanren::rules::realmMaxHp(Realm::QiRefining11);
        s.mp = s.maxMp = fanren::rules::realmMaxMp(Realm::QiRefining11);
        auto loaded = app().loadMap(infoOf(step).mapId, std::string{});
        ASSERT_TRUE(loaded.ok) << loaded.error;
    }

    std::vector<std::string> hints_;
    std::map<std::string, int> seedlingsBeforeChukou_;
};

// ---------------------------------------------------------------------------
// 起点：第 6 章终局原样读入，一个字段也不改
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, StartsFromTheChapterSixHandOverWithNoFieldChanged) {
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        startFromChapterSixEnding(*side);
        ASSERT_FALSE(HasFatalFailure());
        const auto fixture = fanren::io::loadGame(fanren::test::chapterFixturePath(assetRoot(), side->file).string());
        ASSERT_TRUE(fixture.ok);
        const auto expected = fanren::test::comparableSaveLines(fixture.value);
        const auto actual = fanren::test::comparableSaveLines(state());
        ASSERT_GT(expected.size(), 20u) << "先验：存档写得出来";
        const auto diff = fanren::test::saveLineDifferences(expected, actual);
        EXPECT_TRUE(diff.empty()) << side->file << " 读进来之后有字段被改了——施工图第 2 节：与第 6 章终局不同的字段"
                                  << "一个也不应有：" << fanren::test::joinLines(diff);
    }
}

// ---------------------------------------------------------------------------
// 第一侧：每一处二选一取第一项；Z1、Z2 都做；收药摊买两瓶清灵散；② 甩土牢符、③ 掷天雷子。
// 沿途五道闸门各验两次（拦得住、开得了），R-3 提示在走进去之前看到。
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens) {
    const Route route = firstSideRoute();
    GameState& s = state();
    int dingshenBeforeYeyu = -1;
    int tianleiziBeforeFengyue = -1;
    const auto before = [&](Step step) {
        commonBefore(step);
        switch (step) {
            case Step::Xiazhong:
                expectHint("n02_jiaoyao", "黄精要四十四年——一株浇满一瓶才到顶；绿液不够就去蒲团上坐一阵");
                break;
            case Step::CuiZishen: expectHint("n03_danbao", "紫参要百年，一株五滴"); break;
            case Step::Danbao:
                // 通岳麓殿的门认 ch07.danbao：此刻还没担保，从黄枫谷北缘走过去必须被拦下（局面本身就是反面）。
                expectGateShut(kMapHfg, "portal_to_yuelu_dian", "ch07.danbao", "ch07.block.yuelu_dian");
                break;
            case Step::Cangshi: expectHint("n05_cangshi", "藏室按时辰收灵石，复制另算——身上的灵石先数一遍"); break;
            case Step::CuiZhi: expectHint("n11_qiannian", "千年要八滴一株——瓶子一满就去园角浇，别让它空满着"); break;
            case Step::Chuling:
                expectGateShut(kMapHfg, "portal_to_fangshi", "ch07.chuling", "ch07.block.fangshi");
                break;
            case Step::FangshiChu: expectHint("n15_likai", "要买的符、要卖的药，都在坊市办了"); break;   // 校对整改 16.5（LOW-7）：门只看 ch07.chuling，不说「不回头」
            case Step::Yeyu: dingshenBeforeYeyu = s.itemCount(kDingshenFu); break;
            case Step::Jihe: expectHint("n21_jihe", "这一去五日，回不了头——药与符都带足"); break;
            case Step::Liangshi:
                // 外围通中心区的口认 ch07.yixiantian：一线天那两个人还没打，口子关着（此刻一线天的挂点还没备着，走过去不起戏）。
                expectGateShut(kMapWaiwei, "portal_to_jindi_zhongxin", "ch07.yixiantian", "ch07.block.jindi_zhongxin");
                break;
            case Step::Shulin:
                tianleiziBeforeFengyue = s.itemCount(kTianleizi);
                // 中心区通环形山的口认 ch07.yueyang：雾还没散。
                expectGateShut(kMapZhongxin, "portal_to_huanxingshan", "ch07.yueyang", "ch07.block.huanxingshan");
                break;
            case Step::Charen: expectHint("n32_charen", "毒血会毁了苗——洞道里能做点手脚"); break;   // 校对整改 16.5（§4.2(e) 第 45 行）
            case Step::Chouhan:
                // 岳麓殿通地火屋的石门认 ch07.dihuo：丑汉还没开门。
                expectGateShut(kMapYuelu, "portal_to_dihuo", "ch07.dihuo", "ch07.block.dihuo");
                break;
            case Step::Liandan: expectHint("n45_chengdan", "一炉不成就再开一炉——药粉还多"); break;
            default: break;
        }
    };
    const auto after = [&](Step step) {
        commonAfter(step);
        switch (step) {
            case Step::Baigong:
                EXPECT_EQ(statusOf("q07_baigong"), fanren::rules::QuestStatus::Completed) << "Z1：药篓交了血红芝";
                break;
            default: break;
        }
    };

    ASSERT_TRUE(playChapter(route, before, after)) << "走不到 ch07.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app().quitRequested()) << "走到了 game_over 那一条" << battleLine();

    // ---- 每一处二选一都取了第一项（施工图第 11 节的取值）----
    for (const char* f : {"ch07.wanbaolou", "ch07.yeyu", "ch07.baoming", "ch07.xiang", "ch07.fengyue", "ch07.zhongwu",
                          "ch07.wuyou", "ch07.mojiao", "ch07.baishi"}) {
        EXPECT_EQ(flag(f), 1) << f << "：第一项";
    }
    EXPECT_EQ(hints_.size(), 8u) << "3.4 那几格 R-3 提示都在走进去之前看到了";

    // ---- 分支之后的那一句（施工图 3.2 原文里「多一句」的几处）----
    EXPECT_TRUE(spokeIn(Step::Wanbaolou, "ch07.wanbaolou.ask1")) << "11b 先亮一株：田掌柜那句说得不急";
    EXPECT_FALSE(spokeIn(Step::Wanbaolou, "ch07.wanbaolou.ask2"));
    EXPECT_TRUE(spokeIn(Step::Chukou, "ch07.chukou.xiang_old")) << "3.2 节点 32b：ch07.xiang == 1 多一句那老滑头竟也活着";
    EXPECT_TRUE(spokeIn(Step::Chukou, "ch07.chukou.wang_fruit")) << "32b：王师叔一句按 ch07.baoming 分";
    EXPECT_FALSE(spokeIn(Step::Guanzhan, "ch07.guanzhan.cold")) << "30：再等等就没有那句冷言";
    EXPECT_TRUE(spokeIn(Step::Zhuji, "ch07.zhuji.here"));
    // 有则扣的几处，这一侧都有（坊市买了两瓶清灵散；天雷子在 ③ 开场掷了）。
    EXPECT_EQ(spokeIn(Step::Yeyu, "ch07.yeyu.freeze_fu"), dingshenBeforeYeyu > 0) << "13：有定神符就用符";
    EXPECT_EQ(spokeIn(Step::Yeyu, "ch07.yeyu.freeze_shu"), dingshenBeforeYeyu <= 0) << "13：没有就改用定神术";
    EXPECT_TRUE(spokeIn(Step::Fanhui, "ch07.fanhui.qingling")) << "14：清灵散有则扣（坊市买的那一瓶）";
    EXPECT_TRUE(spokeIn(Step::Youdi, "ch07.youdi.qingling")) << "26c：清灵散有则扣（第二瓶）";
    ASSERT_GE(tianleiziBeforeFengyue, 1) << "先验：③ 之前身上有天雷子";
    EXPECT_TRUE(spokeIn(Step::Shulin, "ch07.shulin.after_lei")) << "23：天雷子一起化了灰";

    // ---- 战斗：②③ 开场那一招真的出了手（施工图 8.2 的原著解法，E2 在真仗里用得出来）----
    const BattleRecord* yixiantian = lastBattle("b07_yixiantian");
    const BattleRecord* fengyue = lastBattle("b07_fengyue");
    ASSERT_NE(yixiantian, nullptr);
    ASSERT_NE(fengyue, nullptr);
    EXPECT_TRUE(yixiantian->opened) << "② 开场土牢符没用出去";
    EXPECT_TRUE(fengyue->opened) << "③ 开场天雷子没掷出去";
    EXPECT_EQ(s.itemCount(kTianleizi), 0);

    // ---- 支线：Z1、Z2 了结（施工图第 6 节）----
    EXPECT_EQ(statusOf("q07_baigong"), fanren::rules::QuestStatus::Completed);
    EXPECT_EQ(statusOf("q07_yujian"), fanren::rules::QuestStatus::Completed);
    EXPECT_TRUE(spokeIn(Step::MaSongyao, "ch07.ma_songyao.baigong")) << "第 6 节 Z1：多一句这一年的药一株没少";
    EXPECT_TRUE(spokeIn(Step::Chukou, "ch07.chukou.yujian")) << "第 6 节 Z2：加赏";

    // ---- 账（第 9 节第 6 条：章末灵石 = 起点 + 349 ＋ Z2 50 − 收药摊花的；中阶 = 起点 + 17 ＋ Z2 1，这只手不吃中阶）----
    const StepRecord* yaotan = recordOf(Step::Yaotan);
    ASSERT_NE(yaotan, nullptr);
    const int spent = diffOf(yaotan->bagBefore, yaotan->bagAfter)[kLingshi];
    EXPECT_LT(spent, 0) << "先验：收药摊真花了钱";
    EXPECT_EQ(s.itemCount(kLingshi), startLingshi_ + kLingshiDelta + kZ2Lingshi + spent) << "第 9 节第 6 条";
    EXPECT_EQ(s.itemCount(kZhong), startZhong_ + kZhongDelta + kZ2Zhong) << "第 9 节第 6 条（k = 0）";

    expectTheCommonEnding();

    std::cout << "\n[ch07 第一侧通关] " << (s.day - startDay_) << " 天（剧情 " << kScriptedDays << "、玩家推 " << playerDays()
              << "），灵石 " << startLingshi_ << "→" << s.itemCount(kLingshi) << "，中阶 " << s.itemCount(kZhong)
              << "，筑基丹 " << s.itemCount(kZhuji) << "，开炉 " << crafts_ << " 次，" << battles_.size()
              << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterSevenEndingFirst);
}

// ---------------------------------------------------------------------------
// 第二侧：每一处取第二项；Z1、Z2 都不做；不买、一张符也不用（③ 是没用天雷子的那一路）。
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, SecondSideTakesEveryOtherChoice) {
    startFromChapterSixEnding(kSecondSide);
    ASSERT_FALSE(HasFatalFailure());
    const Route route = secondSideRoute();
    GameState& s = state();
    int dingshenBeforeYeyu = -1;
    const auto before = [&](Step step) {
        commonBefore(step);
        if (step == Step::Yeyu) dingshenBeforeYeyu = s.itemCount(kDingshenFu);
    };
    const auto after = [&](Step step) { commonAfter(step); };

    ASSERT_TRUE(playChapter(route, before, after)) << "走不到 ch07.done，卡在「" << stuckAt_ << "」" << battleLine();
    ASSERT_FALSE(app().quitRequested()) << "走到了 game_over 那一条" << battleLine();

    for (const char* f : {"ch07.wanbaolou", "ch07.yeyu", "ch07.baoming", "ch07.xiang", "ch07.fengyue", "ch07.zhongwu",
                          "ch07.wuyou", "ch07.mojiao", "ch07.baishi"}) {
        EXPECT_EQ(flag(f), 2) << f << "：第二项";
    }
    EXPECT_TRUE(spokeIn(Step::Wanbaolou, "ch07.wanbaolou.ask2")) << "11b 两株一起亮：田掌柜那句说得更急";
    EXPECT_FALSE(spokeIn(Step::Chukou, "ch07.chukou.xiang_old")) << "ch07.xiang == 2：没有那一句";
    EXPECT_TRUE(spokeIn(Step::Chukou, "ch07.chukou.wang_plain")) << "32b：王师叔一句按 ch07.baoming 分";
    EXPECT_TRUE(spokeIn(Step::Guanzhan, "ch07.guanzhan.cold")) << "3.2 节点 30：这就出手，战后多一句冷言";
    EXPECT_TRUE(spokeIn(Step::Zhuji, "ch07.zhuji.home_no")) << "3.2 节点 36：回药园再说——想了想还是就地";
    EXPECT_EQ(spokeIn(Step::Yeyu, "ch07.yeyu.freeze_fu"), dingshenBeforeYeyu > 0);
    EXPECT_EQ(spokeIn(Step::Yeyu, "ch07.yeyu.freeze_shu"), dingshenBeforeYeyu <= 0);
    EXPECT_TRUE(spokeIn(Step::Fanhui, "ch07.fanhui.noqingling")) << "14：没有清灵散换一句，不设死局";
    EXPECT_TRUE(spokeIn(Step::Youdi, "ch07.youdi.noqingling")) << "26c：同上";
    EXPECT_TRUE(spokeIn(Step::Shulin, "ch07.shulin.after_plain")) << "23：没用天雷子的那一路";
    EXPECT_EQ(s.itemCount(kTianleizi), 1) << "这一侧天雷子一直揣着（③ 靠别的破势）";
    const BattleRecord* fengyue = lastBattle("b07_fengyue");
    ASSERT_NE(fengyue, nullptr);
    EXPECT_EQ(fengyue->phase, BattlePhase::Won) << "验收 4：③ 不用天雷子那只手也赢得了" << battleLine();

    // ---- 支线：Z1、Z2 过期（施工图第 6 节）----
    EXPECT_EQ(statusOf("q07_baigong"), fanren::rules::QuestStatus::Failed) << "Z1：马师伯送药时还没交";
    EXPECT_EQ(statusOf("q07_yujian"), fanren::rules::QuestStatus::Failed) << "Z2：掉进地道就回不来了";
    EXPECT_FALSE(spokeIn(Step::MaSongyao, "ch07.ma_songyao.baigong"));
    EXPECT_FALSE(spokeIn(Step::Chukou, "ch07.chukou.yujian"));

    // ---- 账：章末灵石 = 起点 + 349（第 9 节第 6 条，不含 Z2 与收药摊）；中阶 = 起点 + 17 ----
    EXPECT_EQ(s.itemCount(kLingshi), startLingshi_ + kLingshiDelta) << "第 9 节第 6 条";
    EXPECT_EQ(s.itemCount(kZhong), startZhong_ + kZhongDelta) << "第 9 节第 6 条（k = 0）";

    expectTheCommonEnding();

    std::cout << "\n[ch07 第二侧通关] " << (s.day - startDay_) << " 天（剧情 " << kScriptedDays << "、玩家推 " << playerDays()
              << "），灵石 " << startLingshi_ << "→" << s.itemCount(kLingshi) << "，中阶 " << s.itemCount(kZhong)
              << "，筑基丹 " << s.itemCount(kZhuji) << "，开炉 " << crafts_ << " 次，" << battles_.size()
              << " 场仗：" << battleLine() << std::endl;
    settleEndingAgainstFixture(fanren::test::kChapterSevenEndingSecond);
}

// ---------------------------------------------------------------------------
// 五场必打一场也绕不过去（验收 15 走一遍的另一半）：不打它，下一节点按下去 / 踩上去不响，也不在别处把仗补上；
// ② 另有闸门，⑤ 的沼泽没有门。
// 起点是**切片**：第一侧的交接存档，把目标链上这一仗之前各步的完成旗标置上（施工图 3.1 的次序），
// 站在这一仗那张图的出生点——不靠通关走过去，免得前面哪一仗的数值把这一条也拖红（那是另一件事）。
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, NoneOfTheFiveMustFightsCanBeWalkedAround) {
    struct Skip {
        Step fight;
        Step next;
        const char* mark;
    };
    const std::vector<Skip> kSkips = {{Step::Yeyu, Step::Fanhui, "①"},       {Step::Yixiantian, Step::Shulin, "②"},
                                      {Step::Shulin, Step::Tongmen, "③"},    {Step::Xiaoshidian, Step::Qingshidian, "④"},
                                      {Step::Guanzhan, Step::Jiaoshi, "⑤"}};
    for (const Skip& skip : kSkips) {
        standJustBefore(skip.fight);
        ASSERT_FALSE(HasFatalFailure());
        static Route route;
        route = secondSideRoute();
        route_ = &route;
        ASSERT_TRUE(WorldScene::triggerReady(state(), objectNamed(infoOf(skip.fight).object)))
            << skip.mark << "：先验，这一仗的挂点此刻备着";
        const std::size_t fought = battles_.size();
        const std::string doneFlag = infoOf(skip.next).doneFlag;
        // 只走不踩备着的踏入型挂点的路：走不到就是走不到，不许「顺路」把这一仗踩出来再算绕过去了。
        strictWalk_ = true;
        tolerateBlocked_ = true;
        if (skip.fight == Step::Yixiantian) {
            // ② 的下一节点在另一张图上：外围通中心区的口认 ch07.yixiantian，不打这一仗就过不去。
            const MapObject gate = objectNamed("portal_to_jindi_zhongxin");
            EXPECT_EQ(gate.property("require_flag"), "ch07.yixiantian") << "②：通中心区的口认一线天的完成旗标";
            EXPECT_FALSE(usePortal("portal_to_jindi_zhongxin")) << "②：没打一线天，通中心区的口不该放行";
            EXPECT_EQ(state().mapId, kMapWaiwei);
        } else {
            beginStep(skip.next);
            static_cast<void>(attemptStep(skip.next));
            endStep();
        }
        tolerateBlocked_ = false;
        strictWalk_ = false;
        EXPECT_EQ(flag(doneFlag), 0) << skip.mark << "：没打这一仗就把 " << infoOf(skip.next).node << " 演了——绕得过去";
        EXPECT_EQ(battles_.size(), fought) << skip.mark << "：跳过的那一步不该在别处把仗补上";
        if (skip.fight == Step::Guanzhan) {
            for (const MapObject& object : map().objects) {
                EXPECT_NE(object.type, "portal") << "沼泽上出现了门 " << object.name << "：施工图第 4 节「沼泽没有门」";
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 输了之后：五场都是生死仗（施工图 8.2「败」一列：致命；3.2 节点 13「败：game over」）。
// 切片：同上一条的起点，起那一仗挂的脚本，我方一招不出、只按防御（playPassively）。
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, LosingAnyOfTheFiveEndsTheGame) {
    struct Loss {
        const char* battle;
        Step step;
        const char* lostKey;   // 输了那一支的旁白（只拿来认出演的是哪一支）
    };
    const std::vector<Loss> kLosses = {{"b07_lu_shixiong", Step::Yeyu, "ch07.yeyu.lost"},
                                       {"b07_yixiantian", Step::Yixiantian, "ch07.yixiantian.lost"},
                                       {"b07_fengyue", Step::Shulin, "ch07.shulin.lost"},
                                       {"b07_zhongxinqu_duoyao", Step::Xiaoshidian, "ch07.xiaoshidian.lost"},
                                       {"b07_zhaoze_shouyao", Step::Guanzhan, "ch07.guanzhan.lost"}};
    for (const Loss& loss : kLosses) {
        renewApp();
        ASSERT_FALSE(HasFatalFailure());
        standJustBefore(loss.step);
        ASSERT_FALSE(HasFatalFailure());
        static Route route;
        route = firstSideRoute();
        route_ = &route;
        throwFights_ = {loss.battle};
        const MapObject hook = objectNamed(infoOf(loss.step).object);
        ASSERT_FALSE(hook.name.empty());
        beginStep(loss.step);
        const auto started = app().startEvent(hook.property("script"));
        ASSERT_TRUE(started.ok) << started.error;
        pumpScripts();
        endStep();
        const BattleRecord* record = lastBattle(loss.battle);
        ASSERT_NE(record, nullptr) << loss.battle << "：这一场没开起来" << battleLine();
        EXPECT_EQ(record->phase, BattlePhase::Lost) << "先验：" << loss.battle << " 这一回是输的" << battleLine();
        EXPECT_TRUE(app().quitRequested()) << loss.battle << "：施工图 8.2 败即 game over";
        EXPECT_EQ(flag(infoOf(loss.step).doneFlag), 0) << loss.battle << "：输了不置完成旗标";
        EXPECT_TRUE(spokeIn(loss.step, loss.lostKey)) << loss.battle << "：输了那一支的旁白";
    }
}

// ---------------------------------------------------------------------------
// 那只手先证明自己稳：同一存档整章跑三遍，每一场的落点、血、法力、药、走到哪一步与终局存档都一字不差。
// 判的是「稳」，不是「走得完」：走不完（数值上打不过）的时候，三遍也得死在同一场、同一回合、同一格血上。
// ---------------------------------------------------------------------------
TEST_F(Ch07Walkthrough, TheHandIsSteadyThreeRunsFromTheSameSaveEndAlike) {
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        std::vector<std::string> firstLines;
        std::string firstBattles;
        std::string firstOutcome;
        for (int run = 0; run < 3; ++run) {
            renewApp();
            ASSERT_FALSE(HasFatalFailure());
            startFromChapterSixEnding(*side);
            ASSERT_FALSE(HasFatalFailure());
            const Route route = side == &kFirstSide ? firstSideRoute() : secondSideRoute();
            tolerateBlocked_ = true;
            const bool done = playChapter(route);
            tolerateBlocked_ = false;
            const std::string outcome = done ? std::string("走完")
                                             : app().quitRequested() ? std::string("game over")
                                                                    : "卡在「" + stuckAt_ + "」";
            const std::vector<std::string> lines = fanren::test::comparableSaveLines(state());
            ASSERT_GT(lines.size(), 20u);
            ASSERT_FALSE(battles_.empty()) << "先验：这一趟至少打了一仗，下面比的不是两份空表（" << outcome << "）";
            if (run == 0) {
                firstLines = lines;
                firstBattles = battleLine();
                firstOutcome = outcome;
                continue;
            }
            EXPECT_EQ(outcome, firstOutcome) << side->file << " 第 " << (run + 1) << " 遍的去向与第 1 遍不同";
            EXPECT_EQ(battleLine(), firstBattles) << side->file << " 第 " << (run + 1) << " 遍的仗与第 1 遍不同";
            const auto diff = fanren::test::saveLineDifferences(firstLines, lines);
            EXPECT_TRUE(diff.empty()) << side->file << " 第 " << (run + 1) << " 遍的终局与第 1 遍不同："
                                      << fanren::test::joinLines(diff);
        }
        std::cout << "[ch07 那只手] " << side->file << " 三遍一致（" << firstOutcome << "）：" << firstBattles << std::endl;
    }
}

// ---------------------------------------------------------------------------
// 五只手：先吃哪一瓶、门槛多低（技术债 G-26 / G-11 那一类敏感）。换五只手各走两侧，打印每一场的落点与用药。
// ---------------------------------------------------------------------------
// 判的只有施工图写死的那一件：五场都是生死仗，**任何一只手**要么走完，要么 game over 在五场必打之一上——
// 不许卡在别处。其余全部打印，结论写进测试路的回复。
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

TEST_F(Ch07Walkthrough, TheSweepOfFiveHands) {
    std::ostringstream table;
    int finished = 0;
    for (const HandOver* side : {&kFirstSide, &kSecondSide}) {
        for (const SweepHand& h : kSweepHands) {
            renewApp();
            ASSERT_FALSE(HasFatalFailure());
            startFromChapterSixEnding(*side);
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
            const bool dead = app().quitRequested();
            finished += done ? 1 : 0;
            table << "\n  " << side->label << " · " << h.name << "："
                  << (done ? "走完" : dead ? "game over" : "卡在「" + stuckAt_ + "」") << battleLine();
            if (!done) {
                ASSERT_TRUE(dead) << h.name << "：没走完也没死，卡在「" << stuckAt_ << "」" << battleLine();
                ASSERT_FALSE(battles_.empty());
                const std::string last = battles_.back().id;
                EXPECT_TRUE(std::find(std::begin(kMustFight), std::end(kMustFight), last) != std::end(kMustFight))
                    << h.name << "：game over 不在五场必打上（" << last << "）";
            }
        }
    }
    policy_ = intentPolicy();
    std::cout << "\n[ch07 五只手的扫描] 走完 " << finished << " / 10" << table.str() << std::endl;
}

// ===========================================================================
// 下面几条不驱动：直接读数据与脚本，判据写死成施工图原文
// ===========================================================================
class Ch07Acceptance : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        chapterScripts_ = scriptsUnder(root_, "ch07");
        allScripts_ = scriptsUnder(root_, "");
        ASSERT_GE(chapterScripts_.size(), 46u) << "先验：scripts/ch07/ 读得到（3.1 表 46 行挂点，分母不能塌）";
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

    // 本章的文案：key 以 ch07. / objective.ch07. 开头的，或者在 ch07*.json 里的（key → 正文）。
    std::map<std::string, std::string> chapterTexts() {
        std::map<std::string, std::string> out;
        static const std::regex kPair("\"([^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
        for (const auto& entry : fs::directory_iterator(fs::path(root_) / "data" / "text")) {
            if (entry.path().extension() != ".json") continue;
            const std::string file = entry.path().filename().string();
            const std::string body = readFile(entry.path());
            const bool chapterFile = file.rfind("ch07", 0) == 0;
            for (auto it = std::sregex_iterator(body.begin(), body.end(), kPair); it != std::sregex_iterator(); ++it) {
                const std::string key = (*it)[1];
                if (chapterFile || key.rfind("ch07.", 0) == 0 || key.rfind("objective.ch07.", 0) == 0) out[key] = (*it)[2];
            }
        }
        return out;
    }

    // 脚本里 talk("说话人", "key") 的说话人（旁白为空）。一句 key 只该有一个说话人。
    std::map<std::string, std::string> speakers() {
        std::map<std::string, std::string> out;
        static const std::regex kTalk("talk\\(\"([a-z0-9_]*)\",\\s*\"([a-z0-9_.]+)\"\\)");
        for (const auto& [path, source] : chapterScripts_) {
            const std::string code = codeOnly(source);
            for (auto it = std::sregex_iterator(code.begin(), code.end(), kTalk); it != std::sregex_iterator(); ++it) {
                out[(*it)[2]] = (*it)[1];
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
// 12.1：LexiconTests 表一含这 27 个词，章号照 12.1。读 tests/LexiconTests.cpp 的字——它是协调者的那张表，
// 本条只看它有没有照 12.1 补齐，不改它。
TEST_F(Ch07Acceptance, No6_TheLexiconTableCarriesTheTwentySevenWordsOfSectionTwelveOne) {
    const std::string lexicon = readFile(fs::path(root_) / "tests" / "LexiconTests.cpp");
    ASSERT_GT(lexicon.size(), 1000u) << "先验：读得到 tests/LexiconTests.cpp";
    const std::vector<std::pair<const char*, int>> kWords = {
        {"先天真火", 155}, {"地火", 156},     {"地肺之火", 156}, {"玉髓芝", 158},    {"紫猴花", 158},
        {"天灵果", 158},   {"敛气术", 161},   {"万宝楼", 162},   {"天雷子", 163},    {"金光砖", 165},
        {"封岳", 187},     {"钟吾", 193},     {"月阳宝珠", 194}, {"穹老怪", 196},    {"青凝镜", 202},
        {"菡云芝", 203},   {"墨蛟", 205},     {"素女轮回功", 208}, {"剑影分光术", 217}, {"灵眼之泉", 220},
        {"天罗国", 221},   {"神风舟", 228},   {"青竹蜂云剑", 241}, {"董萱儿", 245},   {"鬼灵门", 249},
        {"聂盈", 708},     {"掌天瓶", 2425}};
    ASSERT_EQ(kWords.size(), 27u) << "施工图 12.1：27 个词";
    for (const auto& [word, chapter] : kWords) {
        const std::string row = std::string("{\"") + word + "\", " + std::to_string(chapter) + ",";
        EXPECT_NE(lexicon.find(row), std::string::npos)
            << "LexiconTests 表一没有「" << word << "」首见 ch" << chapter << " 这一行（施工图 12.1）";
    }
}

// 12.2：章内先后按 key 前缀逐条断言。文案 key 的场景段（ch07.<场景>.<用途>）记的就是「这一句在哪一节读到」；
// 下面这张表把每一种前缀钉到施工图 3.4 目标链的步序上（n01 = 1 … n46 = 46）。
// **表里查不到的 key 直接判红**——新加一条文案而不给它登记节点，等于绕开了这一层检查。
//   · 挂点脚本的文案：那一步的步序（药篓、传功阁两处 once=false 按句子前缀再细分到它演的那一回）；
//   · 目标链第 N 步的文案在第 N−1 步做完之后上屏：按 N−1 算；
//   · 路径行动：条目 when 里那一步；支线：接取的那一步；物品 / 法术描述：给出它的那一步（敌方法术：那一仗）；
//   · 地名：进那张图的那一步；拦路话：走到那道门就看得见的那一步（施工偏差 18.1：通坊市、岳麓殿的两道门
//     第 6 章里就在，记 0）。
struct KeyOrder {
    const char* prefix;
    int order;
};

const std::vector<KeyOrder>& keyOrders() {
    static const std::vector<KeyOrder> kTable = {
        // 地名与拦路话
        {"ch07.map.yuelu_dian.", 4},        {"ch07.map.fangshi.", 13},        {"ch07.map.shandong.", 15},
        {"ch07.map.jindi_wai.", 21},        {"ch07.map.jindi_waiwei.", 24},   {"ch07.map.jindi_zhongxin.", 28},
        {"ch07.map.huanxingshan.", 31},     {"ch07.map.dixia_zhaoze.", 36},   {"ch07.map.dihuo.", 44},
        {"ch07.block.fangshi", 0},          {"ch07.block.yuelu_dian", 0},     {"ch07.block.dihuo", 4},
        {"ch07.block.jindi_zhongxin", 24},  {"ch07.block.huanxingshan", 28},
        // 坊市收药摊的店名
        {"ch07.shop.", 13},
        // 主线挂点（3.1 表，按 3.4 目标链的步序）
        {"ch07.chaibao.", 1},
        {"ch07.yaolou.hj_less", 2},         {"ch07.yaolou.y1_", 2},           {"ch07.yaolou.zs_less", 3},
        {"ch07.yaolou.y2_", 3},             {"ch07.yaolou.z1", 3},            {"ch07.yaolou.done", 3},
        {"ch07.yuelu.", 4},                 {"ch07.cangshi.", 5},             {"ch07.dihuo_wen.", 6},
        {"ch07.shimen.", 7},                {"ch07.murong.", 8},              {"ch07.dufang.", 9},
        {"ch07.chuangong.", 10},            {"ch07.yuanjiao.", 11},           {"ch07.chuling.", 12},
        {"ch07.fangshi.", 13},              {"ch07.wanbaolou.", 14},          {"ch07.fangshi_chu.", 15},
        {"ch07.yeyu.", 16},                 {"ch07.battle.lu.", 16},          {"ch07.fanhui.", 17},
        {"ch07.fudan.", 18},                {"ch07.baoming.", 19},            {"ch07.ma_songyao.", 20},
        {"ch07.jihe.", 21},                 {"ch07.xiang.", 22},              {"ch07.liedui.", 23},
        {"ch07.pojin.", 24},                {"ch07.wulongtan.", 25},          {"ch07.liangshi.", 26},
        {"ch07.yixiantian.", 27},           {"ch07.battle.yixiantian.", 27},  {"ch07.shulin.", 28},
        {"ch07.battle.fengyue.", 28},       {"ch07.tongmen.", 29},            {"ch07.yueyang.", 30},
        {"ch07.kuitan.", 31},               {"ch07.charen.", 32},             {"ch07.youdi.", 33},
        {"ch07.sichu.", 34},                {"ch07.xiaoshidian.", 35},        {"ch07.battle.xiaoshidian.", 35},
        {"ch07.battle.dahan.", 35},         {"ch07.qingshidian.", 36},        {"ch07.guanzhan.", 37},
        {"ch07.battle.mojiao.", 37},        {"ch07.jiaoshi.", 38},            {"ch07.xiashan.", 39},
        {"ch07.chukou.", 40},               {"ch07.xieshili.", 41},           {"ch07.sannian.", 42},
        {"ch07.chouhan.", 43},              {"ch07.shijiu.", 44},             {"ch07.banian.", 45},
        {"ch07.zhuji.", 46},
        // 支线（第 6 节：Z1 在节点 3 接、Z2 在节点 24 接；Z2 的两处要进了环形山才走得到）
        {"ch07.quest.baigong.", 3},         {"ch07.quest.yujian.", 29},
        {"ch07.yujian_dongpo.", 31},        {"ch07.yujian_hantan.", 31},
        // 路径行动（落地的 when，施工偏差 18.10）
        {"ch07.path.wufeng_dating.", 1},    {"ch07.path.yu_dating.", 1},      {"ch07.path.ye_dating.", 2},
        {"ch07.path.wang_dating.", 19},     {"ch07.path.xulao_dating.", 5},   {"ch07.path.chouhan_dating.", 7},
        {"ch07.path.chouhan_dating2.", 43}, {"ch07.path.tanzhu_dating.", 13}, {"ch07.path.yuanwu_dating.", 13},
        {"ch07.path.zhishi_qiugou.", 13},   {"ch07.path.xiang_dating.", 23},  {"ch07.path.chen_dating.", 21},
        {"ch07.path.lingshou_dating.", 23}, {"ch07.path.jujian_dating.", 23},
        // 物品与法术描述：给出它的那一步（3.2）
        {"item.desc.material_lingshi_zhong", 1},   {"item.desc.story_jinggang_huan", 1},
        {"item.desc.story_heiwu_qi", 1},           {"item.desc.story_huangtong_ping", 1},
        {"item.desc.herb_huangjing_zhi", 1},       {"item.desc.talisman_tulao_fu", 1},
        {"item.desc.story_yinsi_ding", 6},         {"item.desc.story_jinfu_zimuren", 14},
        {"item.desc.story_xuantie_dun", 14},       {"item.desc.talisman_jinguangzhuan", 14},
        {"item.desc.talisman_tianleizi", 14},      {"item.desc.story_qingjiao_qi", 16},
        {"item.desc.story_qing_suo", 16},          {"item.desc.story_yin_gou", 16},
        // 校对整改 16.5（LOW-3）：点破名字之前给的是布包、小圆镜、黑靴，点破那一步才换成正名。
        {"item.desc.story_bubao_faqi", 21},        {"item.desc.story_kuilei_gongshou", 42},
        {"item.desc.weapon_wuming_sixian", 26},
        {"item.desc.story_xiao_yuanjing", 28},     {"item.desc.story_qingning_jing", 35},
        {"item.desc.story_shuijing_qiu", 28},
        {"item.desc.talisman_xiaodao_fubao", 28},  {"item.desc.story_hei_xue", 28},
        {"item.desc.story_tayun_xue", 29},
        {"item.desc.herb_zihou_hua", 33},          {"item.desc.material_wugong_ke", 33},
        {"item.desc.herb_yusui_zhi", 34},          {"item.desc.herb_tianling_guo", 34},
        {"item.desc.story_yinhui_jian", 35},       {"item.desc.material_mojiao_cailiao", 38},
        {"item.desc.weapon_biguang_dao", 40},      {"item.desc.story_yinse_shuye", 42},
        {"item.desc.story_qingyuan_jianjue", 42},  {"item.desc.material_zhuji_yaofen", 42},
        {"item.desc.story_feidan_yuhe", 44},
        {"magic.desc.magic_tulao_shu", 1},         {"magic.desc.magic_dingshen_shu", 1},
        {"magic.desc.magic_ji_jinfu", 14},         {"magic.desc.magic_ji_jinguangzhuan", 14},
        {"magic.desc.magic_tianleizi", 14},        {"magic.desc.magic_ji_qingjiao", 16},
        {"magic.desc.magic_qinghu_zhan", 16},      {"magic.desc.magic_xiaodao_fubao", 28},
        {"magic.desc.magic_ji_qingning", 28},      {"magic.desc.magic_yinhui_jian", 35},
        {"magic.desc.magic_heishui", 37},          {"magic.desc.magic_ziye", 37},
        {"magic.desc.magic_zhuque_huan", 37},
    };
    return kTable;
}

int orderOfKey(const std::string& key) {
    static const std::regex kObjective("^objective\\.ch07\\.n([0-9]+)_");
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

// 12.2 那张表的每一行：词、从第几步起才许出现（步序）、以及同一步里从哪一句前缀起才许。
struct Gate {
    const char* word;
    int allowedFrom;          // 步序 < allowedFrom 的 key 不许含
    const char* sameStepFrom; // 步序 == allowedFrom 时，只有这个前缀起的 key 许含（空 = 整步都许）
    const char* why;
};

const std::vector<Gate>& gates() {
    static const std::vector<Gate> kGates = {
        {"定颜丹", 5, "", "12.2：节点 1-4a 的 key 不含「定颜丹」"},
        {"岳麓殿", 3, "", "12.2：节点 1、2 的 key 不含（节点 3 马师伯第一次说出口）"},
        {"血色试炼", 10, "", "12.2：节点 1-7 的 key 不含「血色试炼」"},
        {"南宫", 38, "ch07.jiaoshi.name", "12.2：ch07.jiaoshi.name* 起才许"},
        {"李化元", 41, "", "12.2：节点 1-32 的 key 不含「李化元」"},
        {"菡云芝", 35, "ch07.xiaoshidian.after", "12.2：ch07.xiaoshidian.after* 起才许"},
        {"师祖", 21, "", "12.2：节点 1-17 的 key 不含「师祖」"},
        {"巫钧山", 3, "", "12.2：节点 1、2 的 key 与第 6 章可见的拦门话不含（ch153 首见，同岳麓殿；校对整改 16.5 M2）"},
    };
    return kGates;
}

bool gateBreaks(const Gate& gate, int order, const std::string& key, const std::string& text) {
    if (text.find(gate.word) == std::string::npos) return false;
    if (order < gate.allowedFrom) return true;
    if (order == gate.allowedFrom && gate.sameStepFrom[0] != '\0') return key.rfind(gate.sameStepFrom, 0) != 0;
    return false;
}

TEST_F(Ch07Acceptance, No6_EachGatedWordStaysOutOfTheKeysBeforeItsNode) {
    const std::map<std::string, std::string> texts = chapterTexts();
    ASSERT_GT(texts.size(), 300u) << "先验：施工图验收 6「分母先验 > 300 条」";
    // 表的自检：同一个口径分得清「节点 3 的一句提到岳麓殿」与「节点 2 的一句提到岳麓殿」。
    ASSERT_EQ(orderOfKey("ch07.yaolou.y2_ma_where"), 3);
    ASSERT_EQ(orderOfKey("ch07.yaolou.y1_basket"), 2);
    ASSERT_EQ(orderOfKey("ch07.fangshi_chu.go"), 15) << "最长前缀：fangshi_chu 不是 fangshi";
    ASSERT_EQ(orderOfKey("objective.ch07.n05_cangshi"), 4);
    ASSERT_EQ(orderOfKey("ch07.path.chouhan_dating2.text"), 43);
    ASSERT_TRUE(gateBreaks(gates()[3], 38, "ch07.jiaoshi.dream", "南宫")) << "同一步里问名之前也不许";
    ASSERT_FALSE(gateBreaks(gates()[3], 38, "ch07.jiaoshi.name_say", "南宫"));
    int unregistered = 0;
    for (const auto& [key, value] : texts) {
        const int order = orderOfKey(key);
        EXPECT_GE(order, 0) << key << " 不在节点表里：新加的文案要登记它在哪一节读到";
        if (order < 0) {
            ++unregistered;
            continue;
        }
        for (const Gate& gate : gates()) {
            EXPECT_FALSE(gateBreaks(gate, order, key, value))
                << key << "（步序 " << order << "）说了「" << gate.word << "」——施工图 " << gate.why << "：" << value;
        }
        // 12.2 / 验收 17：全章 0 处的几样。
        for (const char* word : {"血禁", "陈巧倩", "淫囊", "合欢", "董家", "红拂", "马云龙", "寒天涯", "萧二",
                                 "封闭前最后一次"}) {
            EXPECT_EQ(value.find(word), std::string::npos) << key << " 里有「" << word << "」（施工图 12.2 / 验收 17：0 处）";
        }
    }
    EXPECT_EQ(unregistered, 0);
    // 正向：这几个词本章确实说了，而且头一回就说在施工图写的那一步。
    std::map<std::string, int> first;
    for (const auto& [key, value] : texts) {
        const int order = orderOfKey(key);
        if (order < 0) continue;
        for (const Gate& gate : gates()) {
            if (value.find(gate.word) == std::string::npos) continue;
            if (!first.count(gate.word) || order < first[gate.word]) first[gate.word] = order;
        }
    }
    for (const Gate& gate : gates()) {
        EXPECT_EQ(first[gate.word], gate.allowedFrom) << "「" << gate.word << "」头一回该在步序 " << gate.allowedFrom;
    }
}

// 12.2「血色禁地」只许出现在对白里、≤ 2 处：对白 = 脚本里 talk 的说话人不空的那一句。
TEST_F(Ch07Acceptance, No6_TheBloodForbiddenGroundIsOnlySaidAloudAndAtMostTwice) {
    const std::map<std::string, std::string> texts = chapterTexts();
    const std::map<std::string, std::string> who = speakers();
    ASSERT_GT(who.size(), 300u) << "先验：脚本里的 talk 读得出来";
    int count = 0;
    for (const auto& [key, value] : texts) {
        if (value.find("血色禁地") == std::string::npos) continue;
        ++count;
        const auto it = who.find(key);
        ASSERT_NE(it, who.end()) << key << " 说了「血色禁地」，却不是哪个脚本 talk 出来的一句";
        EXPECT_FALSE(it->second.empty()) << key << "：「血色禁地」只许出现在对白里，这一句是旁白";
    }
    EXPECT_LE(count, 2) << "施工图 12.2：「血色禁地」≤ 2 处";
}

// 名牌也是上屏的字：说话人的名字按那一句的步序同样过一遍 12.2（白衣少女在问名之前不许挂「南宫婉」的名牌，
// 卖符少女在报名之前不许挂「菡云芝」的名牌）。
TEST_F(Ch07Acceptance, No6_TheNamePlatesFollowTheSameOrder) {
    const std::map<std::string, std::string> who = speakers();
    ASSERT_GT(who.size(), 300u);
    int checked = 0;
    for (const auto& [key, role] : who) {
        if (role.empty()) continue;
        const fanren::core::RoleTemplate* r = app_.data().findRole(role);
        ASSERT_NE(r, nullptr) << key << " 的说话人 " << role << " 不在 data/roles";
        const int order = orderOfKey(key);
        if (order < 0) continue;   // 上一条已经判红
        ++checked;
        for (const Gate& gate : gates()) {
            EXPECT_FALSE(gateBreaks(gate, order, key, r->name))
                << key << "（步序 " << order << "）的名牌「" << r->name << "」含「" << gate.word << "」——施工图 " << gate.why;
        }
    }
    EXPECT_GT(checked, 100);
}

// 12.2 与第 5 节写死的名牌。
TEST_F(Ch07Acceptance, No6_TheRolesCarryTheNamesSectionsFiveAndTwelveTwoGive) {
    const auto nameOf = [&](const char* id) {
        const fanren::core::RoleTemplate* r = app_.data().findRole(id);
        EXPECT_NE(r, nullptr) << id;
        return r == nullptr ? std::string{} : r->name;
    };
    EXPECT_EQ(nameOf("li_huayuan"), "李师祖") << "12.2：role li_huayuan 的 name 是「李师祖」";
    EXPECT_EQ(nameOf("chen_shimei").find("巧倩"), std::string::npos) << "12.2：陈师妹的 name 不含「巧倩」";
    EXPECT_EQ(nameOf("baiyi_shaonv").find("南宫"), std::string::npos) << "12.2：白衣少女的 name 不含「南宫」";
    EXPECT_EQ(nameOf("baiyi_shaonv"), "白衣少女") << "第 5 节：名牌「白衣少女」";
    EXPECT_EQ(nameOf("maifu_shaonv"), "卖符少女") << "12.2：maifu_shaonv 的 name 不变";
    EXPECT_EQ(nameOf("han_yunzhi"), "菡云芝") << "第 5 节";
    EXPECT_EQ(nameOf("nangong_wan"), "南宫婉") << "第 5 节：名牌南宫婉";
    EXPECT_EQ(nameOf("chen_shimei"), "陈师妹") << "第 5 节：本章只称陈师妹";
}

// 章节卡在第 6 章一收尾就上屏，通坊市、岳麓殿的两道门第 6 章里就在（施工偏差 18.1：「两句都写成不靠第 7 章剧情
// 也读得通（不含『岳麓殿』、不点第 7 章的人名）」）。这几句的步序是 0：本章的章内禁词一个也不许有。
// 「巫钧山」与「岳麓殿」同在 ch153 首见（原著本地核过），同一个理由。
// 章节卡的「血色试炼」是施工图 12.2 登记的例外（校对整改 16.5 M1：它是本章大事件的名字、不是悬念，又是大纲章名），
// 改判为「卡片例外」：只放行 ui.chapter.07.title 这一个 key 的这一个词，卡片两个 key 的值仍逐字钉死。
TEST_F(Ch07Acceptance, No6_TheChapterCardAndTheTwoGatesSeenInChapterSixSayNothingOfChapterSeven) {
    const std::map<std::string, std::string> texts = chapterTexts();
    const std::string ui = readFile(fs::path(root_) / "data" / "text" / "ui.json");
    static const std::regex kPair("\"(ui\\.chapter\\.07\\.[^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
    std::map<std::string, std::string> early;
    for (auto it = std::sregex_iterator(ui.begin(), ui.end(), kPair); it != std::sregex_iterator(); ++it) {
        early[(*it)[1]] = (*it)[2];
    }
    ASSERT_EQ(early.size(), 2u) << "先验：ui.chapter.07.numeral / .title（施工图 E9）";
    EXPECT_EQ(early["ui.chapter.07.numeral"], "第七章") << "E9";
    EXPECT_EQ(early["ui.chapter.07.title"], "血色试炼")
        << "E9：章名「血色试炼」——在节点 8 之前就上屏，12.2 登记为卡片例外（校对整改 16.5 M1）";
    for (const char* key : {"ch07.block.fangshi", "ch07.block.yuelu_dian"}) {
        ASSERT_TRUE(texts.count(key)) << key;
        early[key] = texts.at(key);
    }
    std::vector<std::string> names = {"岳麓殿", "巫钧山"};
    for (const char* role : {"xu_lao", "chou_han", "murong_xiong", "murong_di", "chen_shimei", "lanyi_nvzi", "li_huayuan",
                             "xiang_zhili", "fuyunzi", "qiong_qianbei", "nichang_xianzi", "luosai_huzi", "feng_yue",
                             "zhong_wu", "han_yunzhi", "tian_buli", "ding_lao"}) {
        const fanren::core::RoleTemplate* r = app_.data().findRole(role);
        ASSERT_NE(r, nullptr) << role;
        names.push_back(r->name);
    }
    for (const auto& [key, value] : early) {
        for (const Gate& gate : gates()) {
            // 卡片例外（12.2，校对整改 16.5 M1）：只此一个 key、一个词
            if (key == "ui.chapter.07.title" && std::string(gate.word) == "血色试炼") continue;
            EXPECT_EQ(value.find(gate.word), std::string::npos) << key << " 在第 7 章开篇之前就上屏，却含「" << gate.word << "」";
        }
        if (key.rfind("ch07.block.", 0) != 0) continue;
        for (const std::string& name : names) {
            EXPECT_EQ(value.find(name), std::string::npos)
                << key << " 第 6 章里就看得见（施工偏差 18.1），却点了第 7 章才有的「" << name << "」：" << value;
        }
    }
}

// ===========================================================================
// 验收 15：必打 5 场绕不过去——直接读数据
// ===========================================================================
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
        {"①", "b07_lu_shixiong", kMapShandong, "trigger_yeyu", kMapShandong, "trigger_fanhui"},
        {"②", "b07_yixiantian", kMapWaiwei, "trigger_yixiantian", kMapZhongxin, "trigger_shulin"},
        {"③", "b07_fengyue", kMapZhongxin, "trigger_shulin", kMapZhongxin, "trigger_tongmen"},
        {"④", "b07_zhongxinqu_duoyao", kMapHxs, "trigger_xiaoshidian", kMapHxs, "trigger_qingshidian"},
        {"⑤", "b07_zhaoze_shouyao", kMapZhaoze, "trigger_guanzhan", kMapZhaoze, "trigger_jiaoshi"},
    };
    return kHooks;
}

TEST_F(Ch07Acceptance, No15_EachOfTheFiveMustFightsIsCalledOnceFromItsHookAndGuardsTheNextNode) {
    std::vector<std::string> doneFlags;
    std::vector<std::string> targets;
    for (const fanren::core::Objective& o : app_.data().objectives) {
        if (o.chapter != 7) continue;
        doneFlags.push_back(o.doneFlag);
        targets.push_back(o.targetObject);
    }
    ASSERT_EQ(doneFlags.size(), 46u) << "先验：第 7 章目标链 46 步（施工图 3.4）";
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

        // 那一场在脚本里是顶格的一句（套在分支里就有一条路不打它），置完成旗标的每一句都排在它后面，
        // 输了（not won）就 game_over 并 return——输了的那一支走不到置旗标。
        const std::vector<std::string> lines = linesOf(allScripts_[callers[0]]);
        int battleLine = -1;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const std::string code = codeOf(lines[i]);
            if (code.find(call) == std::string::npos) continue;
            battleLine = static_cast<int>(i);
            EXPECT_TRUE(!code.empty() && code[0] != ' ' && code[0] != '\t')
                << callers[0] << " 第 " << (i + 1) << " 行：那一场不是顶格的一句";
        }
        ASSERT_GE(battleLine, 0);
        const std::string setter = "flag.set(\"" + setFlag + "\"";
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (codeOf(lines[i]).find(setter) == std::string::npos) continue;
            EXPECT_GT(static_cast<int>(i), battleLine) << callers[0] << " 第 " << (i + 1) << " 行：在打 " << h.battleId
                                                       << " 之前就置了 " << setFlag;
        }
        const std::string afterBattle = codeOnly(allScripts_[callers[0]]).substr(codeOnly(allScripts_[callers[0]]).find(call));
        EXPECT_LT(afterBattle.find("game_over()"), afterBattle.find(setter))
            << callers[0] << "：施工图 8.2「败：致命」——输了要先 game_over，走不到置旗标";
        for (const auto& [path, source] : allScripts_) {
            if (path == callers[0]) continue;
            EXPECT_EQ(codeOnly(source).find(setter), std::string::npos)
                << path << " 也置 " << setFlag << "：" << h.mark << " 那一节能被它跳过去";
        }
    }
    // 扫描器自检：同一个扫法对第 6 章那三场是看得见的（分母不塌）。
    int chapterSix = 0;
    for (const auto& [path, source] : scriptsUnder(root_, "ch06")) chapterSix += countOf(codeOnly(source), "battle(\"b06_");
    EXPECT_GE(chapterSix, 3) << "扫描器自检：第 6 章的 battle( 该数得出来";
}

// 施工图第 4 节：「每一处挂战斗的格子前面都有存档点：① 山洞洞内、② 外围落点、③ 中心区外树林入口、
// ④ 环形山小石殿前、⑤ 沼泽石阶出口」。口径与第 5、6 章同：从那张图的默认出生点（本章五张图都是剧情传送的落点）
// 出发，不踩本图任何一处必打战斗的踏入型挂点，走得到一个存档设施的旁边。静止的 NPC 挡路；设施与门不能踩。
TEST_F(Ch07Acceptance, No15_EveryMustFightHookHasASavePointReachableBeforeIt) {
    std::set<std::string> fightMaps;
    for (const MustFightHook& h : mustFightHooks()) fightMaps.insert(h.mapId);
    ASSERT_EQ(fightMaps.size(), 5u);
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
        EXPECT_TRUE(reachable) << mapId << "：从落点出发，不踩必打的挂点就走不到存档点（施工图第 4 节）";
    }
}

// ===========================================================================
// 验收 22：主线节点 36、分支 ≥ 10、新地图 9、主线对白约 17300 字（与 tools/audit.py 的数对照写在测试路的回复里）
// ===========================================================================
TEST_F(Ch07Acceptance, No22_BranchesMapsNodesAndWordsAreWhereSectionFifteenPutsThem) {
    // 主动分支 10 处（3 节表下：11、13、16、19a、23、24、28、30、32b、36）：这十处挂点的脚本各有一个二选一。
    const std::vector<std::pair<const char*, const char*>> kWithChoice = {
        {kMapFangshi, "trigger_wanbaolou"}, {kMapShandong, "trigger_yeyu"},      {kMapHfg, "trigger_baoming"},
        {kMapWai, "trigger_xiang"},         {kMapZhongxin, "trigger_shulin"},    {kMapZhongxin, "trigger_tongmen"},
        {kMapHxs, "trigger_xiaoshidian"},   {kMapZhaoze, "trigger_guanzhan"},    {kMapWai, "trigger_chukou"},
        {kMapDihuo, "trigger_zhuji"}};
    ASSERT_EQ(kWithChoice.size(), 10u);
    for (const auto& [mapId, hook] : kWithChoice) {
        const MapObject object = objectOn(mapId, hook);
        const auto it = allScripts_.find(object.property("script"));
        ASSERT_NE(it, allScripts_.end()) << hook << " 挂的脚本读不到";
        const auto blocks = choiceBlocks(it->second);
        ASSERT_EQ(blocks.size(), 1u) << hook << "：施工图第 3 节的主动分支在这里（一处二选一）";
        EXPECT_EQ(blocks[0].size(), 2u) << hook << "：二选一";
    }
    int choices = 0;
    for (const auto& [path, source] : chapterScripts_) choices += countOf(codeOnly(source), "choice{");
    EXPECT_GE(choices, 10) << "施工图验收 22：分支 ≥ 10";
    // 新地图 9 张。
    int maps = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "maps")) {
        const std::string name = entry.path().filename().string();
        if (name.rfind("ch07_", 0) == 0 && entry.path().extension() == ".tmj") ++maps;
    }
    EXPECT_EQ(maps, 9) << "施工图第 4 节 / 验收 22：新地图 9 张";
    // 主线节点 36：3.1 表的挂点落在 36 个节点上（1、2…36；a/b/c 与 Z 不另算）。
    std::set<std::string> nodes;
    for (const StepInfo& info : stepTable()) {
        std::string node = info.node;
        if (node == "—" || node.rfind("Z", 0) == 0) continue;
        while (!node.empty() && (node.back() == 'a' || node.back() == 'b' || node.back() == 'c')) node.pop_back();
        nodes.insert(node);
    }
    EXPECT_EQ(nodes.size(), 36u) << "施工图第 3 节：主线节点 36 个";
    // 主线对白（三份主线文案里 ch07. 开头的，只数汉字）。施工图第 3 节「约 17300」：「约」没有给容差，只防塌。
    int chars = 0;
    for (const char* file : {"ch07_main.json", "ch07_jindi.json", "ch07_dihuo.json"}) {
        const std::string body = readFile(fs::path(root_) / "data" / "text" / file);
        ASSERT_FALSE(body.empty()) << file;
        static const std::regex kPair("\"(ch07\\.[^\"]+)\"\\s*:\\s*\"((?:[^\"\\\\]|\\\\.)*)\"");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), kPair); it != std::sregex_iterator(); ++it) {
            const std::string value = (*it)[2];
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
    std::cout << "[ch07 字数] 三份主线文案汉字 " << chars << "（施工图第 3 节约 17300）" << std::endl;
    EXPECT_GE(chars, 17300 * 8 / 10) << "主线对白比施工图的「约 17300 字」少了两成以上";
}

// ---------------------------------------------------------------------------
// 验收第 15 节各条的落点（不在本文件的那几条）
// ---------------------------------------------------------------------------
//   1 门禁：build.bat 的四道门禁本身。不写 C++ 断言。
//   3 挂点、19 在场链：Ch07TriggerModeTests。
//   4 编成（含 ③ 没有天雷子那只手也赢得了）：Ch07BattleDataTests。
//   5、7、8、9、12、13、14、16、17、18（文案那一半）：Ch07SliceTests。
//   10 经济账、11 两侧同账（读数据那一半）与死局退路：Ch07LedgerTests；11 的「走一遍」在上面两条通关里。
//   20 瓶子与年份接口：Ch07EngineTests（引擎路）。
//   21 独立校对：人工，不是测试。

}  // namespace
