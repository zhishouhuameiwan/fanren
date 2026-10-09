// 第 7 章每一处挂点是怎么开始的——直接读 tmj，不经过走位、不经过脚本（docs/ch07-design.md 验收 3、19）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 3、4 章的教训，handoff-2026-09-23-ch04 第 8 节第一条）
// ---------------------------------------------------------------------------
// 门禁拦得住「只改地图不改脚本」（@hook 一致性），拦不住「两边一起改」——判据三角缺的是「设计文档 ↔ 地图」
// 那一条边。本文件就是那条边：
//   · 不走位、不起脚本、不看旗标，直接读 maps/ch07_*.tmj 与第 6 章两张修补过的图上的属性；
//   · 判据**逐格抄自 docs/ch07-design.md 3.1 那张表的原文**（mode / once / guard_flag / set_flag），
//     旁边附施工图给的理由，转红时原样打出来；
//   · 形状与 Ch07AcceptanceTests 的通关测试不同——同一套驱动写两遍，只是把同一个错误钉两遍。
//
// 施工偏差（docs/ch07-design.md 第 18 节，以它为准）：
//   · 18.1：两处修补也进了第 6 章的生成（本文件只看落在图上的对象，与由谁生成无关）；
//   · 18.2：节点 6 挂在黄枫谷东口 (46,26)(46,27)；
//   · 18.3：节点 16 的报名案在王师叔身边 (25,12)；
//   · 18.4：出口不摆掩月宗的人、南宫婉、向之礼（第 5 节的出口名单按落地的判）；
//   · 18.13：8 / 15 的挂点在吴风南边 (40,10)，35a 的挂点在丑汉面前 (43,5)——都不压在 NPC 身上。
// 外加 3.1 表下那几条纪律：全章不用 auto、同一格只挂一个对象、两处 once=false 不写 set_flag、once=true 的每一条都写
// set_flag 且挂着的脚本真的会置它；以及第 4 节的地图表（尺寸、室内外、区域、曲子、视觉预设）、五道闸门、只进不出。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
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
using fanren::core::TileMap;
using fanren::game::Application;

constexpr const char* kByy = "ch06_baiyaoyuan";
constexpr const char* kHfg = "ch06_huangfenggu";
constexpr const char* kYuelu = "ch07_yuelu_dian";
constexpr const char* kDihuo = "ch07_dihuo";
constexpr const char* kFangshi = "ch07_fangshi";
constexpr const char* kShandong = "ch07_shandong";
constexpr const char* kWai = "ch07_jindi_wai";
constexpr const char* kWaiwei = "ch07_jindi_waiwei";
constexpr const char* kZhongxin = "ch07_jindi_zhongxin";
constexpr const char* kHxs = "ch07_huanxingshan";
constexpr const char* kZhaoze = "ch07_dixia_zhaoze";

// 施工图第 4 节：新建的九张图。
constexpr const char* kNewMaps[] = {kYuelu, kDihuo, kFangshi, kShandong, kWai, kWaiwei, kZhongxin, kHxs, kZhaoze};

// ---------------------------------------------------------------------------
// 判据。逐行抄自 docs/ch07-design.md 3.1 那张表（"" 表示那一格是「—」）
// ---------------------------------------------------------------------------
struct HookSpec {
    const char* row;       // 表里 # 那一列
    const char* object;
    const char* mapId;
    const char* mode;
    bool once;
    const char* guard;
    const char* setFlag;
    const char* battle;    // 「战斗」一列（空 = 没有；可选那一场也写上）
    const char* why;
};

const std::vector<HookSpec>& designSaysSo() {
    static const std::vector<HookSpec> kSpecs = {
        {"1", "trigger_chaibao", kByy, "interact", true, "ch06.done", "ch07.chaibao", "", "拆叶师叔那一包是他做的"},
        {"2/3/Z1", "trigger_yaolou", kByy, "interact", false, "ch07.chaibao", "", "",
         "把药摆进药篓是他做的；一处药篓管三次交药，once=false、脚本看旗标"},
        {"4a", "trigger_yuelu_ru", kYuelu, "enter", true, "ch07.danbao", "ch07.yuelu", "", "传送阵把他送进大厅"},
        {"4b", "trigger_cangshi", kYuelu, "interact", true, "ch07.yuelu", "ch07.cangshi", "", "在藏室书桌前翻玉筒是他"},
        {"5a", "trigger_dihuo_wen", kYuelu, "interact", true, "ch07.cangshi", "ch07.yinsi", "", "回头问许老地火、挑丹炉是他"},
        {"5b", "trigger_shimen", kYuelu, "enter", true, "ch07.yinsi", "ch07.chouhan", "", "丑汉的冷脸是撞上的"},
        {"6", "trigger_murong", kHfg, "enter", true, "ch07.chouhan", "ch07.murong", "", "雷声是传过来的（施工偏差 18.2：东口）"},
        {"7", "trigger_dufang", kByy, "interact", true, "ch07.murong", "ch07.dufang", "", "回屋逐字读方是他"},
        {"8/15", "trigger_chuangong", kHfg, "interact", false, "ch07.dufang", "", "",
         "去传功阁请教吴风是他；一处管两场，理由同药篓"},
        {"9", "trigger_yuanjiao", kByy, "interact", true, "ch07.lianqi", "ch07.qiannian", "", "园角两株千年灵草收成，是他"},
        {"10", "trigger_chuling", kHfg, "interact", true, "ch07.qiannian", "ch07.chuling", "", "向于执事领令牌是他"},
        {"11a", "trigger_fangshi_ru", kFangshi, "enter", true, "ch07.chuling", "ch07.fangshi", "", "进坊市北口，规矩是看出来的"},
        {"11b", "trigger_wanbaolou", kFangshi, "interact", true, "ch07.fangshi", "ch07.wanbaolou", "",
         "挑最气派的一家推门进去是他"},
        {"12", "trigger_fangshi_chu", kFangshi, "enter", true, "ch07.wanbaolou", "ch07.likai", "", "出南口就走"},
        {"13", "trigger_yeyu", kShandong, "interact", true, "ch07.likai", "ch07.yeyu", "b07_lu_shixiong",
         "靠石壁歇下是他；洞外的人声是后来的"},
        {"14", "trigger_fanhui", kShandong, "interact", true, "ch07.yeyu", "ch07.fanhui", "", "收拾残局、带她走是他"},
        {"16", "trigger_baoming", kHfg, "interact", true, "ch07.fudan_fa", "ch07.baoming", "", "去报名是他"},
        {"17", "trigger_ma_songyao", kByy, "enter", true, "ch07.baoming", "ch07.ma_songyao", "", "马师伯在园门等他"},
        {"18", "trigger_jihe", kHfg, "enter", true, "ch07.ma_songyao", "ch07.jihe", "", "信符召集、踏进大殿"},
        {"19a", "trigger_xiang", kWai, "enter", true, "ch07.jihe", "ch07.xiang", "", "向之礼找上他"},
        {"19b", "trigger_liedui", kWai, "interact", true, "ch07.xiang", "ch07.qipai", "", "次日上午走进队列是他"},
        {"20", "trigger_pojin", kWai, "enter", true, "ch07.qipai", "ch07.pojin", "", "七件法宝轰开的通道"},
        {"21a", "trigger_wulongtan", kWaiwei, "enter", true, "ch07.pojin", "ch07.wulongtan", "", "潭边的事是撞上的"},
        {"21b", "trigger_liangshi", kWaiwei, "interact", true, "ch07.wulongtan", "ch07.sixian", "", "翻两具尸首是他"},
        {"22", "trigger_yixiantian", kWaiwei, "enter", true, "ch07.sixian", "ch07.yixiantian", "b07_yixiantian",
         "一出路口就被堵"},
        {"23", "trigger_shulin", kZhongxin, "interact", true, "ch07.yixiantian", "ch07.fengyue", "b07_fengyue",
         "爬上大树歇息是他；后面的事找上门"},
        {"24", "trigger_tongmen", kZhongxin, "enter", true, "ch07.fengyue", "ch07.zhongwu", "",
         "铜门前的三具尸与门内的飞蛇都是撞上的"},
        {"25", "trigger_yueyang", kZhongxin, "enter", true, "ch07.zhongwu", "ch07.yueyang", "", "光雨是看见的"},
        {"26a", "trigger_kuitan", kHxs, "enter", true, "ch07.yueyang", "ch07.kuitan", "", "拐角探头看见巨蜈蚣"},
        {"26b", "trigger_charen", kHxs, "interact", true, "ch07.kuitan", "ch07.mairen", "", "退回洞道倒插子刃是他"},
        {"26c", "trigger_youdi", kHxs, "enter", true, "ch07.mairen", "ch07.zihouhua", "", "大摇大摆走进石厅挑衅"},
        {"27", "trigger_sichu", kHxs, "interact", true, "ch07.zihouhua", "ch07.sichu", "", "照资料把另外四处跑一遍是他"},
        {"28", "trigger_xiaoshidian", kHxs, "enter", true, "ch07.sichu", "ch07.wuyou", "b07_zhongxinqu_duoyao",
         "打斗声是传过来的"},
        {"29", "trigger_qingshidian", kHxs, "interact", true, "ch07.wuyou", "ch07.didao", "", "拿金刃试殿门是他"},
        {"30", "trigger_guanzhan", kZhaoze, "enter", true, "ch07.didao", "ch07.mojiao", "b07_zhaoze_shouyao",
         "伏在黑土堆后，外面的事发生在他眼前"},
        {"31", "trigger_jiaoshi", kZhaoze, "interact", true, "ch07.mojiao", "ch07.nangong", "", "拿银剑剖蛟尸是他"},
        {"32a", "trigger_xiashan", kHxs, "enter", true, "ch07.nangong", "ch07.xiashan", "", "往山下奔"},
        {"32b", "trigger_chukou", kWai, "enter", true, "ch07.xiashan", "ch07.chujindi", "", "出了通道，一派一派的人站着"},
        {"33", "trigger_xieshili", kByy, "enter", true, "ch07.chujindi", "ch07.xieshili", "", "马师伯在屋里等他"},
        {"34", "trigger_sannian", kByy, "interact", true, "ch07.xieshili", "ch07.sannian", "",
         "睡足一觉、坐下排三年的计划是他"},
        {"35a", "trigger_chouhan", kYuelu, "interact", true, "ch07.sannian", "ch07.dihuo", "", "拿铃铛摇醒丑汉是他"},
        {"35b", "trigger_shijiu", kDihuo, "enter", true, "ch07.dihuo", "ch07.feidan", "", "推门进十九号，玉牌一贴石门合上"},
        {"35c", "trigger_banian", kDihuo, "interact", true, "ch07.feidan", "ch07.chengdan", "",
         "在鼎旁盘点、决定接着炼半年是他"},
        {"36", "trigger_zhuji", kDihuo, "interact", true, "ch07.chengdan", "ch07.done", "", "就地服丹是他"},
        {"Z2a", "trigger_yujian_dongpo", kHxs, "interact", true, "ch07.yujian_qiu", "ch07.yujian_a", "be07_tiebi_yuan",
         "玉简上东坡的石屋；门口两只铁臂猿（可逃）"},
        {"Z2b", "trigger_yujian_hantan", kHxs, "interact", true, "ch07.yujian_qiu", "ch07.yujian_b", "", "玉简上北坡的寒潭"},
    };
    return kSpecs;
}

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch07_dihuo.tmj") && fs::exists(root / "scripts" / "ch07" / "zhuji.lua")) {
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

// 一个对象属于本章：它挂的脚本在 scripts/ch07/，或者它的哪一个属性写着 ch07. 开头的旗标 / 文案 key。
bool belongsToChapterSeven(const MapObject& object) {
    if (object.property("script").rfind("ch07/", 0) == 0) return true;
    for (const char* key : {"guard_flag", "set_flag", "require_flag", "visible_flag", "hidden_flag", "deny_text_key"}) {
        if (object.property(key).rfind("ch07.", 0) == 0) return true;
    }
    const std::string target = object.property("target_map");
    return target.rfind("ch07_", 0) == 0;
}

class Ch07TriggerMode : public ::testing::Test {
protected:
    void SetUp() override {
        root_ = assetRoot();
        auto ready = app_.init(root_, /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    const TileMap* mapOf(const std::string& mapId) {
        auto loaded = app_.loadMap(mapId, std::string{});
        if (!loaded.ok) return nullptr;
        return app_.currentMap();
    }
    std::vector<MapObject> objectsOn(const std::string& mapId) {
        const TileMap* map = mapOf(mapId);
        return map == nullptr ? std::vector<MapObject>{} : map->objects;
    }
    MapObject objectOn(const std::string& mapId, const std::string& name) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.name == name) return object;
        }
        return MapObject{};
    }
    std::string scriptCode(const std::string& path) { return codeOnly(readFile(fs::path(root_) / "scripts" / path)); }

    Application app_;
    std::string root_;
};

// ---------------------------------------------------------------------------
// 先验：表里每一处挂点确实在图上，是 trigger，而且 mode / once / script 真的写了
// ---------------------------------------------------------------------------
TEST_F(Ch07TriggerMode, EveryHookTheDesignTableNamesIsReallyOnItsMap) {
    ASSERT_EQ(designSaysSo().size(), 46u) << "判据表的行数变了。改它之前先改 docs/ch07-design.md 3.1——那张表是上游";
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.row << "：" << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.type, "trigger") << spec.object;
        EXPECT_FALSE(object.property("script").empty()) << spec.object << " 没写 script";
        EXPECT_FALSE(object.property("mode").empty()) << spec.object << " 没写 mode——下面会变成比两个空串";
        EXPECT_FALSE(object.property("once").empty()) << spec.object << " 没写 once";
    }
}

// ---------------------------------------------------------------------------
// 正题：mode / once / guard_flag / set_flag 逐格对 3.1 表；挂的脚本在 scripts/ch07/，打的仗是表里那一场
// ---------------------------------------------------------------------------
TEST_F(Ch07TriggerMode, EveryHookStartsTheWayTheDesignTableSays) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.property("mode"), spec.mode)
            << spec.row << " " << spec.object << " 的触发方式与施工图 3.1 不符：" << spec.why;
        EXPECT_EQ(object.property("once") == "true", spec.once)
            << spec.row << " " << spec.object << " 的 once 与施工图 3.1 不符（读到 \"" << object.property("once") << "\"）";
        EXPECT_EQ(object.property("guard_flag"), spec.guard) << spec.row << " " << spec.object << " 的 guard_flag 与施工图 3.1 不符";
        EXPECT_EQ(object.property("set_flag"), spec.setFlag) << spec.row << " " << spec.object << " 的 set_flag 与施工图 3.1 不符";
        const std::string script = object.property("script");
        EXPECT_EQ(script.rfind("ch07/", 0), 0u) << spec.object << " 挂的不是本章的脚本：" << script;
        const std::string code = scriptCode(script);
        ASSERT_GT(code.size(), 40u) << "先验：读得到 " << script;
        const std::string battle = spec.battle;
        std::set<std::string> battles;
        static const std::regex kBattle("battle\\(\"([a-z0-9_]+)\"\\)");
        for (auto it = std::sregex_iterator(code.begin(), code.end(), kBattle); it != std::sregex_iterator(); ++it) {
            battles.insert((*it)[1]);
        }
        if (battle.empty()) {
            EXPECT_TRUE(battles.empty()) << spec.row << " " << spec.object << " 不该开战（3.1「战斗」一列是「—」）";
        } else {
            EXPECT_EQ(battles, std::set<std::string>{battle}) << spec.row << " " << spec.object << " 打的不是 3.1 表那一场";
        }
    }
}

// 3.1 表下第一条：once=true 的每一条都写 set_flag，挂着的脚本**真的会置它**（否则 once 永远烧不掉，那一幕可以无限重演，
// advance_days 跟着把日历刷到任意一天）；两处 once=false（药篓、传功阁）不写 set_flag（地图规范 4.4），
// 靠脚本自己的旗标判「已演过」——那几面旗脚本也真的会置。
TEST_F(Ch07TriggerMode, EveryOnceHookIsBurntByTheScriptItStartsAndTheTwoRepeatersKeepTheirOwnBooks) {
    int repeaters = 0;
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.object;
        const std::string code = scriptCode(object.property("script"));
        if (!spec.once) {
            ++repeaters;
            EXPECT_EQ(object.property("set_flag"), "") << spec.row << "：once=false 不写 set_flag（地图规范 4.4）";
            continue;
        }
        EXPECT_NE(code.find(std::string("flag.set(\"") + spec.setFlag + "\""), std::string::npos)
            << spec.row << " " << spec.object << " 挂的脚本 " << object.property("script") << " 一处也不置 " << spec.setFlag
            << "：once 永远烧不掉";
    }
    EXPECT_EQ(repeaters, 2) << "施工图 3.1 表下：两处 once=false（药篓、传功阁）";
    // 药篓：节点 2 置 ch07.jiaoyao1、节点 3 置 ch07.danbao 与 ch07.baigong_qiu、Z1 置 ch07.baigong；
    // 传功阁：节点 8 置 ch07.lianqi、节点 15 置 ch07.fudan_fa（3.1 表那两行的 set_flag 一列）。
    const std::string yaolou = scriptCode("ch07/yaolou.lua");
    for (const char* f : {"ch07.jiaoyao1", "ch07.danbao", "ch07.baigong_qiu", "ch07.baigong"}) {
        EXPECT_NE(yaolou.find(std::string("flag.set(\"") + f + "\""), std::string::npos) << "药篓脚本不置 " << f;
    }
    const std::string chuangong = scriptCode("ch07/chuangong.lua");
    for (const char* f : {"ch07.lianqi", "ch07.fudan_fa"}) {
        EXPECT_NE(chuangong.find(std::string("flag.set(\"") + f + "\""), std::string::npos) << "传功阁脚本不置 " << f;
    }
}

// 3.1 表下第二条：同一格只挂一个对象（九张新图与两张修补图，修补图连第 6 章原有的对象一起数）；本章不用 auto；
// 九张新图上的 trigger 都是表里的，两张修补图上本章加的 trigger 也都是表里的。
TEST_F(Ch07TriggerMode, NoAutoOneObjectPerCellAndNothingOutsideTheTable) {
    std::set<std::string> listed;
    for (const HookSpec& spec : designSaysSo()) listed.insert(std::string(spec.mapId) + "/" + spec.object);
    int triggersSeen = 0;
    std::vector<std::string> maps(std::begin(kNewMaps), std::end(kNewMaps));
    maps.push_back(kByy);
    maps.push_back(kHfg);
    for (const std::string& mapId : maps) {
        std::map<std::pair<int, int>, std::string> owner;
        const bool patched = mapId.rfind("ch06_", 0) == 0;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "spawn" || object.type == "encounter") continue;   // 出生点与遭遇区不占格
            if (object.type == "trigger" && (!patched || belongsToChapterSeven(object))) {
                ++triggersSeen;
                EXPECT_NE(object.property("mode"), "auto") << mapId << " / " << object.name << " 用了 auto（本章不用 auto）";
                EXPECT_TRUE(listed.count(mapId + "/" + object.name))
                    << mapId << " / " << object.name << " 不在施工图 3.1 的表里";
            }
            for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                    const auto cell = std::make_pair(object.position.x + dx, object.position.y + dy);
                    const auto [it, fresh] = owner.emplace(cell, object.name);
                    EXPECT_TRUE(fresh) << mapId << " (" << cell.first << "," << cell.second << ") 上挂了两个对象："
                                       << it->second << " 与 " << object.name << "（施工图 3.1：同一格只挂一个对象）";
                }
            }
        }
    }
    EXPECT_EQ(triggersSeen, 46) << "先验：十一张图上一共 46 处本章的 trigger（3.1 表 46 行）";
}

// 3.1 表末几行：园角两畦灵田、北街收药摊、十九号地火、禁地三处打坐点、九张新图各有存档点。
TEST_F(Ch07TriggerMode, TheFacilitiesOfTableThreeOneAndSectionFourAreOnTheirMaps) {
    const MapObject corner = objectOn(kByy, "facility_field_jiao");
    ASSERT_FALSE(corner.name.empty()) << "施工图 3.1：ch06_baiyaoyuan 上的 facility_field_jiao";
    EXPECT_EQ(corner.type, "facility");
    EXPECT_EQ(corner.property("kind"), "field");
    EXPECT_EQ(corner.property("ref_id"), "field_baiyaoyuan_jiao") << "3.1：ref_id=field_baiyaoyuan_jiao";
    EXPECT_EQ(corner.property("slots"), "2") << "3.1：slots=2";
    EXPECT_EQ(corner.property("require_flag"), "ch07.chaibao") << "3.1：require_flag=ch07.chaibao";
    const MapObject stall = objectOn(kFangshi, "facility_yaotan");
    ASSERT_FALSE(stall.name.empty()) << "施工图 3.1：ch07_fangshi 上的 facility_yaotan";
    EXPECT_EQ(stall.property("kind"), "shop");
    EXPECT_EQ(stall.property("ref_id"), "ch07_fangshi_yaotan") << "3.1：ref_id=ch07_fangshi_yaotan";
    const MapObject furnace = objectOn(kDihuo, "facility_dihuo");
    ASSERT_FALSE(furnace.name.empty()) << "施工图 3.1：ch07_dihuo 上的 facility_dihuo";
    EXPECT_EQ(furnace.property("kind"), "alchemy") << "3.1：kind=alchemy";
    EXPECT_EQ(furnace.property("grade"), "4") << "3.1 / 验收 14：grade=4（地火）";
    EXPECT_EQ(furnace.property("require_flag"), "ch07.feidan") << "3.1 / 验收 14：require_flag=ch07.feidan";
    // 禁地三处打坐点（3.1 表倒数第二行）：中心区外树冠、树洞，环形山山顶石洞；地火屋「facility_dazuo 不放」（第 4 节）。
    for (const auto& [mapId, name] : std::vector<std::pair<const char*, const char*>>{
             {kZhongxin, "facility_dazuo_shuguan"}, {kZhongxin, "facility_dazuo_shudong"}, {kHxs, "facility_dazuo_shidong"}}) {
        const MapObject cushion = objectOn(mapId, name);
        ASSERT_FALSE(cushion.name.empty()) << mapId << " 上没有 " << name << "（3.1 表：禁地打坐点）";
        EXPECT_EQ(cushion.property("kind"), "meditate") << name;
    }
    int meditateOnDihuo = 0;
    for (const MapObject& object : objectsOn(kDihuo)) {
        if (object.type == "facility" && object.property("kind") == "meditate") ++meditateOnDihuo;
    }
    EXPECT_EQ(meditateOnDihuo, 0) << "施工图第 4 节：地火屋 facility_dazuo 不放（此图的日子只由脚本拨）";
    // 3.1 表：「树冠打坐点在林边大树旁」——与节点 23 那棵树相邻。
    const MapObject tree = objectOn(kZhongxin, "trigger_shulin");
    const MapObject crown = objectOn(kZhongxin, "facility_dazuo_shuguan");
    EXPECT_EQ(std::abs(tree.position.x - crown.position.x) + std::abs(tree.position.y - crown.position.y), 1)
        << "3.1 / 第 4 节：打坐点在林边大树旁";
    // 各图存档点（3.1 表末行、第 4 节「每场必打之前一个」；环形山两处：上山路口、小石殿前）。
    for (const char* mapId : kNewMaps) {
        int saves = 0;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "facility" && object.property("kind") == "save") ++saves;
        }
        EXPECT_GE(saves, 1) << mapId << "：第 4 节写了存档点";
        if (std::string(mapId) == kHxs) EXPECT_EQ(saves, 2) << "第 4 节：环形山存档点两处（上山路口、小石殿前）";
    }
    // 遭遇区（8.4）：只在环形山一块，require_flag=ch07.yueyang；外围、中心区外不放，别的新图也不放。
    int zones = 0;
    for (const char* mapId : kNewMaps) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type != "encounter") continue;
            ++zones;
            EXPECT_EQ(std::string(mapId), kHxs) << mapId << "：施工图 8.4「外围、中心区外不放」";
            EXPECT_EQ(object.property("require_flag"), "ch07.yueyang") << "8.4：区 require_flag=ch07.yueyang";
            EXPECT_EQ(object.property("table_id"), "encounter_ch07_huanxingshan") << "8.4：表 ch07_huanxingshan";
        }
    }
    EXPECT_EQ(zones, 1) << "施工图 8.4：环形山一块区";
}

// ---------------------------------------------------------------------------
// 施工图第 4 节那张地图表：尺寸、室内外、区域、曲子、地名 key、章号、视觉预设
// ---------------------------------------------------------------------------
struct MapSpec {
    const char* id;
    int width, height;
    bool outdoor;
    const char* bgm;
    const char* shortName;
    const char* theme;
    const char* time;
    std::vector<const char*> particles;
    const char* backdrop;
};

const std::vector<MapSpec>& mapTable() {
    static const std::vector<MapSpec> kMaps = {
        {kYuelu, 48, 36, false, "bgm_indoor", "yuelu_dian", "indoor_stone", "indoor", {"dust"}, "secret_room"},
        {kDihuo, 32, 24, false, "bgm_cave", "dihuo", "indoor_stone", "indoor", {"ember"}, "secret_room"},
        {kFangshi, 48, 30, true, "bgm_town", "fangshi", "town", "day", {"dust"}, "town_street"},
        {kShandong, 32, 24, true, "bgm_night", "shandong", "wild", "night", {"firefly"}, "mountain_forest_dusk"},
        {kWai, 48, 36, true, "bgm_cliff", "jindi_wai", "cliff", "day", {"dust"}, "cliff_top"},
        {kWaiwei, 48, 36, true, "bgm_wild", "jindi_waiwei", "wild", "day", {"mist"}, "mountain_forest_dusk"},
        {kZhongxin, 48, 36, true, "bgm_wild", "jindi_zhongxin", "valley", "day", {"petal"}, "herb_valley"},
        {kHxs, 48, 40, true, "bgm_mountain_path", "huanxingshan", "mountain_path", "day", {"mist"}, "mountain_forest"},
        {kZhaoze, 36, 28, false, "bgm_cave", "dixia_zhaoze", "cave", "indoor", {"mist", "ember"}, "cave_tunnel"},
    };
    return kMaps;
}

TEST_F(Ch07TriggerMode, TheNineMapsAreTheOnesTheMapTableDraws) {
    ASSERT_EQ(mapTable().size(), 9u) << "施工图第 4 节：新建 9 张";
    for (const MapSpec& spec : mapTable()) {
        const TileMap* map = mapOf(spec.id);
        ASSERT_NE(map, nullptr) << spec.id << " 载不进来";
        EXPECT_EQ(map->width, spec.width) << spec.id << "：施工图第 4 节的尺寸";
        EXPECT_EQ(map->height, spec.height) << spec.id << "：施工图第 4 节的尺寸";
        EXPECT_EQ(map->outdoor, spec.outdoor) << spec.id << "：施工图第 4 节的室外一列";
        EXPECT_EQ(map->region, "jianzhou") << spec.id << "：第 4 节「region 一律 jianzhou」";
        EXPECT_EQ(map->bgm, spec.bgm) << spec.id << "：第 4 节的 bgm";
        EXPECT_EQ(map->chapter, 7) << spec.id << "：chapter=7";
        EXPECT_EQ(map->displayNameKey, std::string("ch07.map.") + spec.shortName + ".name") << spec.id << "：规则 21";
        EXPECT_NE(app_.data().lookupText(map->displayNameKey), map->displayNameKey) << spec.id << " 的地名查不到";
    }
}

// 视觉预设（第 4 节那一列：主题 / 时段 / 粒子 / 背景）直接读 data/visual/maps.json 的字。
// 测试目标拿不到 nlohmann，这里按那一张图的那一段原文找（段 = 从 "图 id": { 到下一张图的 id 之前）。
TEST_F(Ch07TriggerMode, TheNineMapsWearTheVisualPresetsOfSectionFour) {
    const std::string visual = readFile(fs::path(root_) / "data" / "visual" / "maps.json");
    ASSERT_GT(visual.size(), 1000u) << "先验：读得到 data/visual/maps.json";
    for (const MapSpec& spec : mapTable()) {
        const std::string head = std::string("\"") + spec.id + "\": {";
        const std::size_t at = visual.find(head);
        ASSERT_NE(at, std::string::npos) << spec.id << " 不在 data/visual/maps.json（施工图 E11：maps.json 9 条）";
        std::size_t end = visual.size();
        static const std::regex kNextMap("\n    \"[a-z0-9_]+\": \\{");
        std::smatch m;
        const std::string rest = visual.substr(at + head.size());
        if (std::regex_search(rest, m, kNextMap)) end = at + head.size() + static_cast<std::size_t>(m.position(0));
        const std::string block = visual.substr(at, end - at);
        EXPECT_NE(block.find(std::string("\"theme\": \"") + spec.theme + "\""), std::string::npos) << spec.id << "：主题 " << spec.theme;
        EXPECT_NE(block.find(std::string("\"time\": \"") + spec.time + "\""), std::string::npos) << spec.id << "：时段 " << spec.time;
        EXPECT_NE(block.find(std::string("\"backdrop\": \"") + spec.backdrop + "\""), std::string::npos)
            << spec.id << "：背景 " << spec.backdrop;
        for (const char* particle : spec.particles) {
            EXPECT_NE(block.find(std::string("\"kind\": \"") + particle + "\""), std::string::npos)
                << spec.id << "：粒子 " << particle;
        }
    }
}

// ---------------------------------------------------------------------------
// 施工图第 4 节「连通」：五道闸门各指一张不同的图（规则 20）、deny_text_key 按目标图命名、不许有第六道；
// 禁地四张图只进不出（外围没有回禁地外的门，中心区没有回外围的门，环形山没有回中心区的门，沼泽没有门）；
// 禁地外、山洞只靠 teleport 进出。
// ---------------------------------------------------------------------------
TEST_F(Ch07TriggerMode, TheFiveGatesAreTheOnesSectionFourDrawsAndTheForbiddenGroundOnlyLetsYouIn) {
    struct Gate {
        const char* mapId;
        const char* portal;
        const char* target;
        const char* requireFlag;
        const char* denyKey;
    };
    const std::vector<Gate> kGates = {
        {kHfg, "portal_to_fangshi", kFangshi, "ch07.chuling", "ch07.block.fangshi"},
        {kHfg, "portal_to_yuelu_dian", kYuelu, "ch07.danbao", "ch07.block.yuelu_dian"},
        {kYuelu, "portal_to_dihuo", kDihuo, "ch07.dihuo", "ch07.block.dihuo"},
        {kWaiwei, "portal_to_jindi_zhongxin", kZhongxin, "ch07.yixiantian", "ch07.block.jindi_zhongxin"},
        {kZhongxin, "portal_to_huanxingshan", kHxs, "ch07.yueyang", "ch07.block.huanxingshan"},
    };
    std::set<std::string> targets;
    for (const Gate& gate : kGates) {
        const MapObject portal = objectOn(gate.mapId, gate.portal);
        ASSERT_FALSE(portal.name.empty()) << gate.mapId << " 上没有 " << gate.portal;
        EXPECT_EQ(portal.type, "portal");
        EXPECT_EQ(portal.property("target_map"), gate.target) << gate.portal;
        EXPECT_EQ(portal.property("require_flag"), gate.requireFlag) << gate.portal;
        EXPECT_EQ(portal.property("deny_text_key"), gate.denyKey) << gate.portal << "（规则 20：按目标图命名）";
        EXPECT_NE(app_.data().lookupText(gate.denyKey), gate.denyKey) << gate.denyKey << " 查不到";
        targets.insert(gate.target);
    }
    EXPECT_EQ(targets.size(), 5u) << "第 4 节：五道闸门各指一张不同的图（规则 20）";
    // 连通图：九张新图上的门，与两张修补图上本章加的门，只有这几道（第 4 节「连通」那张图）。
    std::multiset<std::pair<std::string, std::string>> links;
    int gated = 0;
    std::vector<std::string> maps(std::begin(kNewMaps), std::end(kNewMaps));
    maps.push_back(kByy);
    maps.push_back(kHfg);
    for (const std::string& mapId : maps) {
        const bool patched = mapId.rfind("ch06_", 0) == 0;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type != "portal" || (patched && !belongsToChapterSeven(object))) continue;
            links.insert({mapId, object.property("target_map")});
            if (!object.property("require_flag").empty()) ++gated;
        }
    }
    EXPECT_EQ(gated, 5) << "第 4 节：本章的闸门只有五道";
    const std::multiset<std::pair<std::string, std::string>> design = {
        {kHfg, kFangshi},       {kFangshi, kHfg},        {kHfg, kYuelu},  {kYuelu, kHfg},
        {kYuelu, kDihuo},       {kDihuo, kYuelu},        {kWaiwei, kZhongxin}, {kZhongxin, kHxs}};
    EXPECT_EQ(links, design) << "施工图第 4 节：坊市、岳麓殿、地火屋各有回程的门；禁地四张图只进不出、山洞与禁地外只靠 teleport";
}

// ---------------------------------------------------------------------------
// 施工图第 5 节「撤场 / 出场旗标」与验收 19：谁在哪张图、认哪一个旗标来去
// ---------------------------------------------------------------------------
TEST_F(Ch07TriggerMode, ThePeopleComeAndGoOnTheFlagsSectionFiveNames) {
    // 山头七人（施工偏差 18.2：东口）：visible ch07.chouhan / hidden ch07.murong，一进一撤。
    for (const char* npc : {"npc_murong_xiong", "npc_murong_di", "npc_lu_shixiong", "npc_chen_shimei", "npc_lanyi_nvzi",
                            "npc_cuai_qingnian", "npc_huangfeng_weiguan"}) {
        const MapObject object = objectOn(kHfg, npc);
        ASSERT_FALSE(object.name.empty()) << kHfg << " 上没有 " << npc << "（第 5 节：山头七人）";
        EXPECT_EQ(object.property("visible_flag"), "ch07.chouhan") << npc;
        EXPECT_EQ(object.property("hidden_flag"), "ch07.murong") << npc;
    }
    // 大殿李师祖：visible ch07.ma_songyao / hidden ch07.jihe。
    const MapObject li = objectOn(kHfg, "npc_li_huayuan");
    ASSERT_FALSE(li.name.empty()) << "第 5 节：大殿李师祖";
    EXPECT_EQ(li.property("visible_flag"), "ch07.ma_songyao");
    EXPECT_EQ(li.property("hidden_flag"), "ch07.jihe");
    // 马师伯本章不摆 NPC（3.1 开头）：两张修补图上本章没有 ma_shibo 的对象。
    for (const char* mapId : {kByy, kHfg}) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type != "npc" || !belongsToChapterSeven(object)) continue;
            EXPECT_NE(object.property("role_id"), "ma_shibo") << mapId << " / " << object.name << "：马师伯本章不摆 NPC";
        }
    }
    // 禁地外：同 role 在同一张图上几段在场，一律「前一段的 hidden 恰是后一段的 visible」（第 5 节、验收 19）；
    // 头一段从 ch07.jihe（到荒山）或 ch07.qipai / ch07.pojin（到黄土坡）起，最后一段不晚于 ch07.chujindi 撤场。
    const std::vector<std::string> order = {"ch07.jihe", "ch07.qipai", "ch07.pojin", "ch07.chujindi"};
    const auto rank = [&order](const std::string& flag) {
        const auto it = std::find(order.begin(), order.end(), flag);
        return it == order.end() ? -1 : static_cast<int>(it - order.begin());
    };
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> segments;
    for (const MapObject& object : objectsOn(kWai)) {
        if (object.type != "npc") continue;
        EXPECT_GE(rank(object.property("visible_flag")), 0) << object.name << "：禁地外的人都按链上的旗标来去";
        EXPECT_GE(rank(object.property("hidden_flag")), 0) << object.name << "：禁地外的人都按链上的旗标撤场";
        segments[object.property("role_id")].emplace_back(object.property("visible_flag"), object.property("hidden_flag"));
    }
    ASSERT_GE(segments.size(), 10u) << "先验：禁地外摆了七派的人";
    for (auto& [role, list] : segments) {
        std::sort(list.begin(), list.end(),
                  [&rank](const auto& a, const auto& b) { return rank(a.first) < rank(b.first); });
        for (std::size_t i = 0; i + 1 < list.size(); ++i) {
            EXPECT_EQ(list[i].second, list[i + 1].first) << role << "：前一段的 hidden 该恰是后一段的 visible（验收 19）";
        }
        EXPECT_LE(rank(list.back().second), rank("ch07.chujindi")) << role << "：出禁地时一批撤场";
    }
    // 验收 18：本章之后任何地图上 nangong_wan / baiyi_shaonv 的 NPC 都以 ch07.chujindi 或更早的旗标撤场。
    // 施工偏差 18.4：出口不摆南宫婉；第 4 节末段：白衣少女不摆 NPC——所以全仓一个也不该有。
    int she = 0;
    for (const auto& entry : fs::directory_iterator(fs::path(root_) / "maps")) {
        if (entry.path().extension() != ".tmj") continue;
        for (const MapObject& object : objectsOn(entry.path().stem().string())) {
            if (object.type != "npc") continue;
            const std::string role = object.property("role_id");
            if (role != "nangong_wan" && role != "baiyi_shaonv") continue;
            ++she;
            EXPECT_GE(rank(object.property("hidden_flag")), 0) << object.name << "：验收 18，要以 ch07.chujindi 或更早撤场";
            EXPECT_LE(rank(object.property("hidden_flag")), rank("ch07.chujindi")) << object.name;
        }
    }
    EXPECT_EQ(she, 0) << "施工偏差 18.4 / 第 4 节末段：南宫婉、白衣少女都不摆 NPC";
    // 禁地四张图上不摆 NPC（第 4 节末段：「禁地里没有能『走过去聊聊』的人」）。
    for (const char* mapId : {kWaiwei, kZhongxin, kHxs, kZhaoze}) {
        for (const MapObject& object : objectsOn(mapId)) {
            EXPECT_NE(object.type, "npc") << mapId << " / " << object.name << "：禁地里不摆 NPC";
        }
    }
}

}  // namespace
