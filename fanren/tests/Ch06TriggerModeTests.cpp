// 第 6 章每一处挂点是怎么开始的——直接读 tmj，不经过走位、不经过脚本（docs/ch06-design.md 验收 3）。
//
// ---------------------------------------------------------------------------
// 为什么要有这一个文件（第 3、4 章的教训，handoff-2026-09-23-ch04 第 8 节第一条）
// ---------------------------------------------------------------------------
// 第 3 章两种杀的触发方式装反，四条测试对它满格敏感却一条没发现：驱动是照着 tmj 写的。
// 门禁拦得住「只改地图不改脚本」（@hook 一致性），拦不住「两边一起改」——判据三角缺的是
// 「设计文档 ↔ 地图」那一条边。本文件就是那条边：
//   · 不走位、不起脚本、不看旗标，直接读 maps/ch06_*.tmj 上的属性；
//   · 判据**逐格抄自 docs/ch06-design.md 3.1 那张表的原文**（mode / once / guard_flag / set_flag），
//     旁边附施工图给的理由，转红时原样打出来；
//   · 形状与 Ch06AcceptanceTests 的通关测试不同——同一套驱动写两遍，只是把同一个错误钉两遍。
//
// 施工偏差（docs/ch06-design.md 第 18 节，以它为准）：
//   · 18.3 第 1 条：5b、8b 挂在 NPC 上，NPC 对象没有 mode / once / guard_flag / set_flag，由脚本自己判旗标——
//     这两行改判「NPC 挂着那个脚本、脚本开头认 3.1 的 guard、脚本置 3.1 的 set_flag」；
//   · 18.3 第 2 条：13 挂在迎宾楼房门上、不 teleport（3.1 的 mode / once / guard / set_flag 照旧）；
//   · 18.3 第 3 条：万小山只摆在山脚（visible ch06.wan_met / hidden ch06.rugu）。
//
// 外加 3.1 表下那几条纪律：全章不用 auto、同一格只挂一个对象（规则 17，本章特意写了「一格一个对象」）、
// 每一条都写 set_flag 且挂着的脚本真的会置它；以及第 4 节的地图表（尺寸、室内外、区域、曲子）与三道闸门。
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
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
using fanren::core::TileMap;
using fanren::game::Application;

// ---------------------------------------------------------------------------
// 判据。逐行抄自 docs/ch06-design.md 3.1 那张表（"" 表示那一格是「—」）
// ---------------------------------------------------------------------------
struct HookSpec {
    const char* row;       // 表里 # 那一列
    const char* object;
    const char* mapId;
    const char* mode;      // "npc" = 施工偏差 18.3 第 1 条，挂在 NPC 上
    bool once;
    const char* guard;
    const char* setFlag;
    const char* script;    // 挂的脚本（施工图第 17 节验收里点名了几个文件名，其余照 3.1 的挂点）
    const char* why;
};

const std::vector<HookSpec>& designSaysSo() {
    static const std::vector<HookSpec> kSpecs = {
        {"1a", "trigger_chudu", "ch06_tainan_cun", "enter", true, "", "ch06.chudu", "ch06/chudu.lua",
         "出生点就在树林里，一睁眼就是除毒最后一天；毒是自己散尽的"},
        {"1b", "trigger_cunkou", "ch06_tainan_cun", "enter", true, "ch06.chudu", "ch06.wan_met", "ch06/cunkou.lua",
         "万小山是撞见的（ch126「刚一进借住的小村」）。放在林子通村口的唯一路口"},
        {"2", "trigger_guaipo", "ch06_tainan_cun", "interact", true, "ch06.wan_met", "ch06.rugu", "ch06/guaipo.lua",
         "领他去怪坡、让他放通音符，是韩立定的（ch126「我带你去」）"},
        {"3", "trigger_qingyan", "ch06_tainan_gu", "enter", true, "ch06.rugu", "ch06.qingyan", "ch06/qingyan.lua",
         "是万小山在远处叫他（ch129）。放在谷口通广场的唯一口子"},
        {"4", "trigger_ruhuo", "ch06_tainan_gu", "enter", true, "ch06.qingyan", "ch06.ruhuo", "ch06/ruhuo.lua",
         "青纹道士是从背后找上来的（ch130）"},
        {"5a", "trigger_lingshi", "ch06_tainan_gu", "enter", true, "ch06.ruhuo", "ch06.lingshi", "ch06/lingshi.lua",
         "第一次走进摊位区，灵石与灵符的行情是看出来的"},
        {"5b", "npc_caomao_qingnian", "ch06_tainan_gu", "npc", true, "ch06.lingshi", "ch06.feixingfu", "ch06/caomao.lua",
         "拿丹药去换飞行符是他主动的（ch131 末）。挂在 NPC 上，撤场旗标 ch06.feixingfu"},
        {"6", "trigger_ye_xunxin", "ch06_tainan_gu", "enter", true, "ch06.feixingfu", "ch06.xunxin", "ch06/xunxin.lua",
         "叶豹带人回来找草帽青年的麻烦，是撞上的"},
        {"7", "trigger_yishi", "ch06_sanxiu_lou", "interact", true, "ch06.xunxin", "ch06.yishi", "ch06/yishi.lua",
         "上二楼推门是他（ch132）"},
        {"8a", "trigger_shuangshou", "ch06_tainan_gu", "enter", true, "ch06.yishi", "ch06.shuangshou",
         "ch06/shuangshou.lua", "双首鹜从头顶飞过（ch136）。放在小楼通广场的路上"},
        {"8b", "npc_maifu_shaonv", "ch06_tainan_gu", "npc", true, "ch06.shuangshou", "ch06.jinzhubi", "ch06/shaonv.lua",
         "停在她摊前是他（ch136 末）。挂在 NPC 上；换完她不撤场"},
        {"9a", "trigger_zhifu", "ch06_sanxiu_lou", "interact", true, "ch06.jinzhubi", "ch06.zhifu", "ch06/zhifu.lua",
         "摆开符纸丹砂是他（ch138）。制符桌是设施，挂点摆在桌前一格"},
        {"9b", "trigger_kuxiu", "ch06_sanxiu_lou", "interact", true, "ch06.zhifu", "ch06.jiuceng", "ch06/kuxiu.lua",
         "蒲团。与 facility kind=meditate 不同格"},
        {"10a", "trigger_canpian", "ch06_tainan_gu", "enter", true, "ch06.jiuceng", "ch06.canpian", "ch06/canpian.lua",
         "争吵声是传过来的（ch139）。放在广场中段"},
        {"10b", "trigger_bilu", "ch06_sanxiu_lou", "interact", true, "ch06.canpian", "ch06.shengxianling",
         "ch06/bilu.lua", "躺在床头翻书是他（ch141）。床那一格"},
        {"11", "trigger_sanhui", "ch06_tainan_gu", "enter", true, "ch06.shengxianling", "ch06.chugu", "ch06/sanhui.lua",
         "青纹一行人是找上他的（ch142）。放在谷口"},
        {"12", "trigger_xisha", "ch06_shanqiu", "enter", true, "ch06.chugu", "ch06.xisha", "ch06/xisha.lua",
         "冰锥是破土而出的。放在凹地"},
        {"13", "trigger_ce_linggen", "ch06_huangfenggu", "enter", true, "ch06.xisha", "ch06.linggen", "ch06/linggen.lua",
         "王师弟安排他住下、当场测了属性，是发生在他身上的"},
        {"14", "trigger_ye_maidan", "ch06_huangfenggu", "interact", true, "ch06.linggen", "ch06.rangdan",
         "ch06/maidan.lua", "开门放他们进来是他。房间里床那一格"},
        {"15", "trigger_dadian", "ch06_huangfenggu", "interact", true, "ch06.rangdan", "ch06.rumen", "ch06/dadian.lua",
         "走进大殿三道门是他"},
        {"16a", "trigger_lin_lingqu", "ch06_huangfenggu", "interact", true, "ch06.rumen", "ch06.chuwudai",
         "ch06/lingqu.lua", "领东西"},
        {"16b", "trigger_wufeng", "ch06_huangfenggu", "interact", true, "ch06.chuwudai", "ch06.wufeng", "ch06/wufeng.lua",
         "传功阁里去请教吴风、应他一句「切磋领悟」，是他"},
        {"17a", "trigger_zawu", "ch06_huangfenggu", "interact", true, "ch06.wufeng", "ch06.zawu", "ch06/zawu.lua",
         "一个月熟悉期都不等、当天就来领活，是他（ch149）。百机堂柜前"},
        {"17b", "trigger_juanzong", "ch06_huangfenggu", "interact", true, "ch06.zawu", "ch06.huinuo", "ch06/juanzong.lua",
         "跟叶堂主进内殿翻卷宗、听他悔诺。内殿里一格"},
        {"18a", "trigger_jinzhi", "ch06_baiyaoyuan", "enter", true, "ch06.huinuo", "ch06.renyao", "ch06/jinzhi.lua",
         "禁制拦住他、马师伯的声音传出来。园门外一格"},
        {"18b", "trigger_maiping", "ch06_baiyaoyuan", "interact", true, "ch06.renyao", "ch06.done", "ch06/maiping.lua",
         "夜里把小瓶埋进药田角落、盖上残片，是他。药田角落一格"},
    };
    return kSpecs;
}

// 施工图第 4 节：新建的六张图。
constexpr const char* kChapterMaps[] = {"ch06_tainan_cun", "ch06_tainan_gu", "ch06_sanxiu_lou",
                                         "ch06_shanqiu", "ch06_huangfenggu", "ch06_baiyaoyuan"};

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        const fs::path root(candidate);
        if (fs::exists(root / "maps" / "ch06_tainan_gu.tmj") && fs::exists(root / "scripts" / "ch06" / "maiping.lua")) {
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

// 复验整改 16.5（MEDIUM-N1 ②）：卖符少女的脚本多了一段还账 `local function repay()`（顶格 `end` 收尾），
// 它只在「8b 已换完、ch06.qianyao > 0」那一支里被叫到。下面「guard 没满足时在买卖的 take 之前 return」
// 那几条判的是买卖，认的是去掉这一段之后的代码；还账那段自己的判据另写（见用到它的地方）。
// 找不到这一段就原样返回。
std::string withoutRepay(const std::string& code) {
    const std::size_t start = code.find("local function repay(");
    if (start == std::string::npos) return code;
    const std::size_t close = code.find("\nend\n", start);
    if (close == std::string::npos) return code;
    return code.substr(0, start) + code.substr(close + 5);
}

// 第 7 章经 genmaps_ch07.patch_*() 加到 ch06_huangfenggu、ch06_baiyaoyuan 上的对象（docs/interfaces-p3-ch07.md 5.3）：
// 认旗标属性里 ch07. 开头的那些（guard_flag / set_flag / require_flag / visible_flag / hidden_flag）。它们归
// Ch07TriggerModeTests 管；本文件数第 6 章的挂点与门时把它们排除在外，判据不改。一格一个对象（规则 17）仍对全图。
bool patchedInByChapterSeven(const MapObject& object) {
    if (object.name == "portal_to_nancheng" && object.property("target_map") == "ch05_nancheng") return true;
    for (const char* key : {"guard_flag", "set_flag", "require_flag", "visible_flag", "hidden_flag"}) {
        if (object.property(key).rfind("ch07.", 0) == 0 || object.property(key).rfind("ch08.", 0) == 0 ||
            object.property(key).rfind("ch09.", 0) == 0) return true;
    }
    return false;
}

class Ch06TriggerMode : public ::testing::Test {
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
// 先验：表里每一处挂点确实在图上，而且 mode / once / script 真的写了
// ---------------------------------------------------------------------------
TEST_F(Ch06TriggerMode, EveryHookTheDesignTableNamesIsReallyOnItsMap) {
    ASSERT_EQ(designSaysSo().size(), 26u) << "判据表的行数变了。改它之前先改 docs/ch06-design.md 3.1——那张表是上游";
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.row << "：" << spec.mapId << " 上没有 " << spec.object;
        EXPECT_FALSE(object.property("script").empty()) << spec.object << " 没写 script";
        if (std::string(spec.mode) == "npc") {
            EXPECT_EQ(object.type, "npc") << spec.row << "：施工偏差 18.3 第 1 条，挂在 NPC 上";
            continue;
        }
        EXPECT_EQ(object.type, "trigger") << spec.object;
        EXPECT_FALSE(object.property("mode").empty()) << spec.object << " 没写 mode——下面会变成比两个空串";
        EXPECT_FALSE(object.property("once").empty()) << spec.object << " 没写 once";
    }
}

// ---------------------------------------------------------------------------
// 正题：mode / once / guard_flag / set_flag 逐格对 3.1 表
// ---------------------------------------------------------------------------
TEST_F(Ch06TriggerMode, EveryHookStartsTheWayTheDesignTableSays) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.mapId << " 上没有 " << spec.object;
        EXPECT_EQ(object.property("script"), spec.script) << spec.row << " " << spec.object << " 挂的脚本不对";
        if (std::string(spec.mode) == "npc") continue;   // 下一条专判
        EXPECT_EQ(object.property("mode"), spec.mode)
            << spec.row << " " << spec.object << " 的触发方式与施工图 3.1 不符：" << spec.why;
        EXPECT_EQ(object.property("once") == "true", spec.once)
            << spec.row << " " << spec.object << " 的 once 与施工图 3.1 不符（读到 \"" << object.property("once") << "\"）";
        EXPECT_EQ(object.property("guard_flag"), spec.guard) << spec.row << " " << spec.object << " 的 guard_flag 与施工图 3.1 不符";
        EXPECT_EQ(object.property("set_flag"), spec.setFlag) << spec.row << " " << spec.object << " 的 set_flag 与施工图 3.1 不符";
    }
}

// 施工偏差 18.3 第 1 条：5b、8b 挂在 NPC 上，由脚本自己判——3.1 表那一行的 guard 与 set_flag 挪进了脚本：
//   · 脚本里第一处 flag.get(...) == 0 就 return 的，正是 3.1 的 guard（前置没到只说一句闲话）；
//   · 脚本置 3.1 的 set_flag，且那一句排在所有 take( 之后（换成了才算换了）；
//   · once：换完之后再按，脚本不再走买卖——5b 靠 hidden_flag 撤场（3.1「撤场旗标 ch06.feixingfu」），
//     8b「换完她不撤场」，脚本开头先认 set_flag、已置就只说一句 return。
TEST_F(Ch06TriggerMode, TheTwoHooksOnPeopleGuardAndBurnInsideTheirScripts) {
    for (const HookSpec& spec : designSaysSo()) {
        if (std::string(spec.mode) != "npc") continue;
        const MapObject npc = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(npc.name.empty()) << spec.object;
        // 判的是买卖：还账那段（只有少女有）先摘掉（复验整改 16.5）。
        const std::string code = withoutRepay(scriptCode(npc.property("script")));
        ASSERT_GT(code.size(), 100u) << "先验：读得到 " << npc.property("script");
        const std::string guardCheck = std::string("flag.get(\"") + spec.guard + "\") == 0";
        const std::size_t guardAt = code.find(guardCheck);
        ASSERT_NE(guardAt, std::string::npos) << spec.row << "：脚本里没有认 3.1 的 guard「" << spec.guard << "」";
        const std::size_t returnAt = code.find("return", guardAt);
        const std::size_t firstTake = code.find("take(");
        ASSERT_NE(returnAt, std::string::npos);
        ASSERT_NE(firstTake, std::string::npos) << spec.row << "：以物易物是脚本（第 17 节第 15 条），脚本里该有 take";
        EXPECT_LT(returnAt, firstTake) << spec.row << "：guard 没满足时要在任何买卖之前 return";
        const std::string setter = std::string("flag.set(\"") + spec.setFlag + "\"";
        const std::size_t setAt = code.rfind(setter);
        ASSERT_NE(setAt, std::string::npos) << spec.row << "：脚本不置 3.1 的 set_flag「" << spec.setFlag << "」";
        EXPECT_GT(setAt, code.rfind("take(")) << spec.row << "：set_flag 要排在最后一处 take 之后";
    }
    // 5b 撤场、8b 不撤场（施工图 3.1 / 第 5 节）。
    const MapObject caomao = objectOn("ch06_tainan_gu", "npc_caomao_qingnian");
    EXPECT_EQ(caomao.property("hidden_flag"), "ch06.feixingfu") << "施工图 3.1 5b：撤场旗标 ch06.feixingfu";
    const MapObject girl = objectOn("ch06_tainan_gu", "npc_maifu_shaonv");
    // 换完不撤场（3.1 8b）；出谷才撤——第 7 章施工图要她在第 6 章之后离开这张图（校对整改 16.4 第二阶段）。
    EXPECT_EQ(girl.property("hidden_flag"), "ch06.chugu") << "施工图 3.1 8b：换完她不撤场，出谷才撤";
    const std::string whole = scriptCode(girl.property("script"));
    const std::string shaonv = withoutRepay(whole);
    ASSERT_NE(shaonv, whole) << "先验：少女的脚本里有还账那段 repay()（复验整改 16.5 MEDIUM-N1 ②）";
    const std::size_t burnt = shaonv.find("flag.get(\"ch06.jinzhubi\") ~= 0");
    ASSERT_NE(burnt, std::string::npos) << "8b 换完之后再按，脚本得先认出「已经换过」";
    EXPECT_LT(shaonv.find("return", burnt), shaonv.find("take(")) << "已经换过就在任何买卖之前 return";
    // 还账只在「已经换过、又欠着」那一支里：repay() 被叫到的那一处排在认出已换过之后、认欠账之后，
    // 而且排在双首鹜那道 guard 之前（没换过的人走不到还账）。
    const std::size_t owes = shaonv.find("flag.get(\"ch06.qianyao\") > 0", burnt);
    const std::size_t called = shaonv.find("repay()", burnt);
    ASSERT_NE(owes, std::string::npos) << "已经换过的那一支得先认欠账（ch06.qianyao）";
    ASSERT_NE(called, std::string::npos) << "已经换过、又欠着账：该叫还账那段";
    EXPECT_LT(owes, called);
    EXPECT_LT(called, shaonv.find("flag.get(\"ch06.shuangshou\") == 0")) << "还账不在买卖那条路上";
    EXPECT_EQ(scriptCode(caomao.property("script")).find("ch06.qianyao"), std::string::npos)
        << "草帽青年那一处补不齐是免了差额，不记账（复验整改 16.5 MEDIUM-N1 ①）";
}

// 3.1 表下第一条：once 全为真，每一条都写 set_flag——而且挂着的脚本**真的会置它**
//（否则 once 永远烧不掉，那一幕可以无限重演，advance_days 跟着把日历刷到任意一天）。
TEST_F(Ch06TriggerMode, EveryOnceHookIsBurntByTheScriptItStarts) {
    for (const HookSpec& spec : designSaysSo()) {
        const MapObject object = objectOn(spec.mapId, spec.object);
        ASSERT_FALSE(object.name.empty()) << spec.object;
        const std::string code = scriptCode(object.property("script"));
        ASSERT_GT(code.size(), 40u) << "先验：读得到 " << object.property("script");
        EXPECT_TRUE(spec.once) << spec.row << "：施工图 3.1「once 全为真」";
        EXPECT_NE(code.find(std::string("flag.set(\"") + spec.setFlag + "\""), std::string::npos)
            << spec.row << " " << spec.object << " 挂的脚本 " << object.property("script") << " 一处也不置 " << spec.setFlag
            << "：once 永远烧不掉";
    }
}

// 3.1 表下第二条：同一格只挂一个对象（规则 17；本章特意写了「小楼二楼 7、9a、9b、10b 四场戏加制符桌与蒲团
// 六样东西，各占一格」）。本章不用 auto。六张图上的挂点都是表里的挂点，没有表外的。
TEST_F(Ch06TriggerMode, NoAutoOneObjectPerCellAndNothingOutsideTheTable) {
    std::set<std::string> listed;
    for (const HookSpec& spec : designSaysSo()) listed.insert(std::string(spec.mapId) + "/" + spec.object);
    int triggersSeen = 0;
    for (const char* mapId : kChapterMaps) {
        std::map<std::pair<int, int>, std::string> owner;
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "spawn" || object.type == "encounter") continue;   // 出生点与遭遇区不占格
            if (object.type == "trigger" && !patchedInByChapterSeven(object)) {
                ++triggersSeen;
                EXPECT_NE(object.property("mode"), "auto") << mapId << " / " << object.name << " 用了 auto（本章不用 auto）";
                EXPECT_TRUE(listed.count(std::string(mapId) + "/" + object.name))
                    << mapId << " / " << object.name << " 不在施工图 3.1 的表里";
            }
            for (int dy = 0; dy < std::max(1, object.height); ++dy) {
                for (int dx = 0; dx < std::max(1, object.width); ++dx) {
                    const auto cell = std::make_pair(object.position.x + dx, object.position.y + dy);
                    const auto [it, fresh] = owner.emplace(cell, object.name);
                    EXPECT_TRUE(fresh) << mapId << " (" << cell.first << "," << cell.second << ") 上挂了两个对象："
                                       << it->second << " 与 " << object.name << "（施工图 3.1：一格一个对象）";
                }
            }
        }
    }
    EXPECT_EQ(triggersSeen, 24) << "先验：六张图上一共 24 处 trigger（3.1 表 26 行减去挂在 NPC 上的 5b、8b）";
}

// 3.1 表最后两行：制符桌是设施（talisman，grade 2——金竺笔是「上好符笔」，require_flag ch06.zhifu），
// 9a 的挂点「摆在桌前一格」；蒲团（meditate）在小楼与百药园各一张，与 9b 的挂点不同格。
TEST_F(Ch06TriggerMode, TheTalismanDeskAndTheCushionsAreFacilitiesBesideTheirHooks) {
    const MapObject desk = objectOn("ch06_sanxiu_lou", "facility_zhifu");
    ASSERT_FALSE(desk.name.empty()) << "施工图 3.1：ch06_sanxiu_lou 上的 facility_zhifu";
    EXPECT_EQ(desk.type, "facility");
    EXPECT_EQ(desk.property("kind"), "talisman") << "施工图第 7 节：facility kind=talisman";
    EXPECT_EQ(desk.property("grade"), "2") << "施工图第 7 节：金竺笔是「上好符笔」，grade 2";
    EXPECT_EQ(desk.property("require_flag"), "ch06.zhifu") << "施工图 3.1：9a 之后玩家自己练";
    const MapObject zhifu = objectOn("ch06_sanxiu_lou", "trigger_zhifu");
    ASSERT_FALSE(zhifu.name.empty());
    EXPECT_EQ(std::abs(zhifu.position.x - desk.position.x) + std::abs(zhifu.position.y - desk.position.y), 1)
        << "施工图 3.1 9a：挂点摆在桌前一格";
    int cushions = 0;
    for (const char* mapId : {"ch06_sanxiu_lou", "ch06_baiyaoyuan"}) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "facility" && object.property("kind") == "meditate") ++cushions;
        }
    }
    EXPECT_EQ(cushions, 2) << "施工图 3.1：facility_dazuo 在小楼与百药园各一张";
    const MapObject kuxiu = objectOn("ch06_sanxiu_lou", "trigger_kuxiu");
    const MapObject dazuo = objectOn("ch06_sanxiu_lou", "facility_dazuo");
    ASSERT_FALSE(kuxiu.name.empty());
    ASSERT_FALSE(dazuo.name.empty());
    EXPECT_FALSE(kuxiu.position == dazuo.position) << "施工图 3.1 9b：与 facility kind=meditate 不同格";
}

// ---------------------------------------------------------------------------
// 施工图第 5 节「撤场 / 出场旗标」与施工偏差 18.3 第 3 条：谁在哪张图、认哪一个旗标来去
// ---------------------------------------------------------------------------
TEST_F(Ch06TriggerMode, ThePeopleComeAndGoOnTheFlagsSectionFiveNames) {
    struct Presence {
        const char* mapId;
        const char* npc;
        const char* visible;
        const char* hidden;
        const char* why;
    };
    const std::vector<Presence> kPresence = {
        {"ch06_tainan_cun", "npc_wan_xiaoshan", "ch06.wan_met", "ch06.rugu", "18.3 第 3 条：万小山只摆在山脚"},
        {"ch06_tainan_gu", "npc_caomao_qingnian", "", "ch06.feixingfu", "3.1 5b：换完他收摊"},
        {"ch06_tainan_gu", "npc_maifu_shaonv", "", "ch06.chugu", "3.1 8b：换完她不撤场，出谷才撤（第 7 章施工图）"},
        {"ch06_sanxiu_lou", "npc_wu_jiuzhi", "ch06.yishi", "", "第 5 节：吴九指小楼（7 起）"},
        {"ch06_sanxiu_lou", "npc_huang_xiaotian", "ch06.yishi", "", "第 5 节：黄孝天小楼（7）"},
        // 复验整改 16.5（N-L1、N-L7）：① 之前一伙人都在广场上，楼里只有苦桑；① 那天傍晚青纹在议事屋门口等，
        // 其余的人在屋里（门关着）；议事之后众人才在一楼。
        {"ch06_sanxiu_lou", "npc_kusang", "", "", "复验 N-L1：苦桑全程在一楼"},
        {"ch06_sanxiu_lou", "npc_qingwen_daoshi", "ch06.xunxin", "", "复验 N-L7：青纹 ① 那天才站到议事屋门口"},
        {"ch06_sanxiu_lou", "npc_hu_pinggu", "ch06.yishi", "", "复验 N-L1：议事前众人在二楼屋里"},
        {"ch06_sanxiu_lou", "npc_xiong_dali", "ch06.yishi", "", "复验 N-L1：议事前众人在二楼屋里"},
        {"ch06_sanxiu_lou", "npc_hei_mu", "ch06.yishi", "", "复验 N-L1：议事前众人在二楼屋里"},
        {"ch06_sanxiu_lou", "npc_hei_jin", "ch06.yishi", "", "复验 N-L1：议事前众人在二楼屋里"},
        {"ch06_sanxiu_lou", "npc_honglian", "ch06.yishi", "", "复验 N-L1：议事前众人在二楼屋里"},
        {"ch06_huangfenggu", "npc_ye_shishu", "ch06.rangdan", "", "18.3 第 3 条：叶师叔只摆在百机堂内殿"},
        // 14-16b 的脚本里王师叔全程陪着韩立，送回石屋群之后才站到大殿前（校对整改 16.4，LOW-7）。
        {"ch06_huangfenggu", "npc_wang_shishu", "ch06.wufeng", "", "校对 LOW-7：王师叔 16b 之后才在大殿前"},
        {"ch06_baiyaoyuan", "npc_ma_shibo", "", "ch06.renyao", "第 5 节：马师伯 hidden ch06.renyao"},
    };
    for (const Presence& p : kPresence) {
        const MapObject npc = objectOn(p.mapId, p.npc);
        ASSERT_FALSE(npc.name.empty()) << p.mapId << " 上没有 " << p.npc;
        EXPECT_EQ(npc.type, "npc");
        EXPECT_EQ(npc.property("visible_flag"), p.visible) << p.npc << "：" << p.why;
        EXPECT_EQ(npc.property("hidden_flag"), p.hidden) << p.npc << "：" << p.why;
    }
    // 只在脚本里、不落地图的人（18.3 第 3 条：黑汉、圆脸青年、叶豹、钟灵道、少女的兄长；第 5 节：黄衣人、土甲大汉只在战斗里）；
    // 万小山只在山脚；青纹一行只摆在小楼（施工图第 17 节第 17 条）。
    std::map<std::string, std::set<std::string>> whereRoles;
    for (const char* mapId : kChapterMaps) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "npc") whereRoles[object.property("role_id")].insert(mapId);
        }
    }
    for (const char* role : {"hei_han", "yuanlian_qingnian", "ye_bao", "zhong_lingdao", "shaonv_xiong", "huangyi_ren",
                             "tujia_dahan", "yejia_dizi"}) {
        EXPECT_EQ(whereRoles.count(role), 0u) << role << " 不该落在地图上（施工偏差 18.3 第 3 条 / 第 5 节）";
    }
    EXPECT_EQ(whereRoles["wan_xiaoshan"], (std::set<std::string>{"ch06_tainan_cun"})) << "万小山只摆在山脚";
    EXPECT_EQ(whereRoles["qingwen_daoshi"], (std::set<std::string>{"ch06_sanxiu_lou"})) << "施工图第 17 节第 17 条：青纹一行只摆在小楼";
}

// ---------------------------------------------------------------------------
// 施工图第 4 节那张地图表：尺寸、室内外、区域、曲子、地名 key、章号
// ---------------------------------------------------------------------------
TEST_F(Ch06TriggerMode, TheSixMapsAreTheOnesTheMapTableDraws) {
    struct MapSpec {
        const char* id;
        int width, height;
        bool outdoor;
        const char* region;
        const char* bgm;
        const char* shortName;
    };
    const std::vector<MapSpec> kMaps = {
        {"ch06_tainan_cun", 40, 30, true, "lanzhou", "bgm_mountain_path", "tainan_cun"},
        {"ch06_tainan_gu", 48, 36, true, "lanzhou", "bgm_valley", "tainan_gu"},
        {"ch06_sanxiu_lou", 32, 24, false, "lanzhou", "bgm_inn", "sanxiu_lou"},
        {"ch06_shanqiu", 40, 30, true, "lanzhou", "bgm_wild", "shanqiu"},
        {"ch06_huangfenggu", 48, 36, true, "jianzhou", "bgm_sect", "huangfenggu"},
        {"ch06_baiyaoyuan", 32, 24, true, "jianzhou", "bgm_valley", "baiyaoyuan"},
    };
    for (const MapSpec& spec : kMaps) {
        const TileMap* map = mapOf(spec.id);
        ASSERT_NE(map, nullptr) << spec.id << " 载不进来";
        EXPECT_EQ(map->width, spec.width) << spec.id << "：施工图第 4 节的尺寸";
        EXPECT_EQ(map->height, spec.height) << spec.id << "：施工图第 4 节的尺寸";
        EXPECT_EQ(map->outdoor, spec.outdoor) << spec.id << "：施工图第 4 节的室外一列";
        EXPECT_EQ(map->region, spec.region) << spec.id << "：施工图第 4 节「region（山脚、谷、小楼、山丘：lanzhou；黄枫谷、百药园：jianzhou）」";
        EXPECT_EQ(map->bgm, spec.bgm) << spec.id << "：施工图第 4 节的 bgm";
        EXPECT_EQ(map->chapter, 6) << spec.id << "：chapter=6";
        EXPECT_EQ(map->displayNameKey, std::string("ch06.map.") + spec.shortName + ".name") << spec.id << "：规则 21";
        EXPECT_NE(app_.data().lookupText(map->displayNameKey), map->displayNameKey) << spec.id << " 的地名查不到";
    }
}

// 施工图第 4 节「要点」一列点名的设施：存档点（除小楼外各图的要点里都写了，小楼写「存档点在一楼」）、
// 坊市（谷，shop ch06_tainan_fangshi）、药田（百药园，field ref_id=field_baiyaoyuan slots=6）、遭遇区（荒丘，require ch06.chugu）。
TEST_F(Ch06TriggerMode, TheFacilitiesTheMapTableNamesAreOnTheirMaps) {
    const auto has = [&](const char* mapId, const std::string& kind, const std::string& key, const std::string& value) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type == "facility" && object.property("kind") == kind && (key.empty() || object.property(key) == value)) {
                return true;
            }
        }
        return false;
    };
    for (const char* mapId : kChapterMaps) EXPECT_TRUE(has(mapId, "save", "", "")) << mapId << "：施工图第 4 节写了存档点";
    EXPECT_TRUE(has("ch06_tainan_gu", "shop", "ref_id", "ch06_tainan_fangshi")) << "施工图第 7 节：坊市挂在谷广场";
    EXPECT_TRUE(has("ch06_baiyaoyuan", "field", "ref_id", "field_baiyaoyuan")) << "施工图第 4 节：药田";
    EXPECT_TRUE(has("ch06_baiyaoyuan", "field", "slots", "6")) << "施工图第 4 节：slots=6";
    bool zone = false;
    for (const MapObject& object : objectsOn("ch06_shanqiu")) {
        if (object.type != "encounter") continue;
        zone = true;
        EXPECT_EQ(object.property("require_flag"), "ch06.chugu") << "施工图 8.4：区 require_flag=ch06.chugu（落地就算）";
    }
    EXPECT_TRUE(zone) << "施工图 8.4：荒丘一块遭遇区";
    for (const char* mapId : {"ch06_tainan_cun", "ch06_tainan_gu", "ch06_sanxiu_lou", "ch06_huangfenggu", "ch06_baiyaoyuan"}) {
        for (const MapObject& object : objectsOn(mapId)) {
            EXPECT_NE(object.type, "encounter") << mapId << "：施工图 8.4「太南谷、黄枫谷、百药园是人群与门派禁地，不放」";
        }
    }
}

// ---------------------------------------------------------------------------
// 施工图第 4 节的三道闸门：各指一张不同的图，deny_text_key 按目标图命名，不许有第四道；
// 四处只靠 teleport 进（山脚没有回南城的门、谷没有回山脚的门、山丘没有任何门、黄枫谷没有回山丘的门）。
// ---------------------------------------------------------------------------
TEST_F(Ch06TriggerMode, TheThreeGatesAreTheOnesSectionFourDrawsAndThereIsNoWayBack) {
    struct Gate {
        const char* mapId;
        const char* portal;
        const char* target;
        const char* requireFlag;
        const char* denyKey;
    };
    const std::vector<Gate> kGates = {
        {"ch05_nancheng", "portal_to_tainan_cun", "ch06_tainan_cun", "ch05.done", "ch06.block.tainan_cun"},
        {"ch06_tainan_gu", "portal_to_sanxiu_lou", "ch06_sanxiu_lou", "ch06.ruhuo", "ch06.block.sanxiu_lou"},
        {"ch06_huangfenggu", "portal_to_baiyaoyuan", "ch06_baiyaoyuan", "ch06.huinuo", "ch06.block.baiyaoyuan"},
    };
    for (const Gate& gate : kGates) {
        const MapObject portal = objectOn(gate.mapId, gate.portal);
        ASSERT_FALSE(portal.name.empty()) << gate.mapId << " 上没有 " << gate.portal;
        EXPECT_EQ(portal.type, "portal");
        EXPECT_EQ(portal.property("target_map"), gate.target) << gate.portal;
        EXPECT_EQ(portal.property("require_flag"), gate.requireFlag) << gate.portal;
        EXPECT_EQ(portal.property("deny_text_key"), gate.denyKey) << gate.portal << "（规则 20：按目标图命名）";
        EXPECT_NE(app_.data().lookupText(gate.denyKey), gate.denyKey) << gate.denyKey << " 查不到";
    }
    // 不许有第四道：六张图上带 require_flag 的门只有两道（第三道在南城）。
    int gated = 0;
    std::multiset<std::pair<std::string, std::string>> links;
    for (const char* mapId : kChapterMaps) {
        for (const MapObject& object : objectsOn(mapId)) {
            if (object.type != "portal" || patchedInByChapterSeven(object)) continue;
            links.insert({mapId, object.property("target_map")});
            if (!object.property("require_flag").empty()) ++gated;
        }
    }
    EXPECT_EQ(gated, 2) << "施工图第 4 节：闸门只有三道（迎宾楼房门不做成门，施工偏差 18.3 第 2 条）";
    // 连通图（施工图第 4 节）：门只有这四道——两道闸门与它们的回程。
    const std::multiset<std::pair<std::string, std::string>> design = {
        {"ch06_tainan_gu", "ch06_sanxiu_lou"}, {"ch06_sanxiu_lou", "ch06_tainan_gu"},
        {"ch06_huangfenggu", "ch06_baiyaoyuan"}, {"ch06_baiyaoyuan", "ch06_huangfenggu"}};
    EXPECT_EQ(links, design) << "施工图第 4 节：山脚没有回南城的门、谷没有回山脚的门、山丘没有任何门、黄枫谷没有回山丘的门";
}

}  // namespace
