// 第 11 章验收第 5–11、13 条：「扫脚本」那一半（读 scripts/ch11/*.lua 的活代码，判据写死成施工图原文）
// 与「切片」那一半（每条写明起点存档，从真实第 10 章终档走 Ch11Driver 到点再扰动）。
// 每条扫描都配「在内存里改坏 → 必须报」；每条切片都配一组对照（扰动前后、两版分支）。
#include <regex>
#include <utility>
#include "Ch11Driver.h"
#include "game/DialogueScene.h"
#include "script/Command.h"

namespace {
using namespace fanren;
using namespace fanren::test::ch11;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

int count(const Sources& s, const std::string& name, const std::string& pattern) {
    const auto found = s.find(name);
    return found == s.end() ? 0 : countMatches(executableLines(found->second), std::regex(pattern));
}
int countAll(const Sources& s, const std::string& pattern) {
    int total = 0;
    for (const auto& entry : s) total += count(s, entry.first, pattern);
    return total;
}
std::size_t at(const Sources& s, const std::string& name, const std::string& literal) {
    const auto found = s.find(name);
    return found == s.end() ? std::string::npos : executableLines(found->second).find(literal);
}
std::size_t lastAt(const Sources& s, const std::string& name, const std::string& literal) {
    const auto found = s.find(name);
    return found == s.end() ? std::string::npos : executableLines(found->second).rfind(literal);
}
// 先后判据：两头都必须找得到（npos 最大，B 被删掉时 a < npos 恒真——测试审查 T-4）。
bool ordered(std::size_t a, std::size_t b) { return a != std::string::npos && b != std::string::npos && a < b; }

// 验收 5–11、13 的「扫」：每一项写成「这句话在这份脚本里恰几处」。
Errors scanErrors(const Sources& s) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    // 5：realm.advance 恰 1 处、在 biguan、目标 CORE_EARLY、看返回值；realm.cap / realm.demote 0 处。
    need(countAll(s, R"(realm\.advance\s*\()") == 1, "5 realm.advance count");
    need(count(s, "biguan", R"(local\s+ok\b[^\n]*=\s*realm\.advance\(realm\.CORE_EARLY\))") == 1, "5 biguan advance");
    need(count(s, "biguan", R"(if\s+not\s+ok\s+then)") == 1, "5 advance result checked");
    need(countAll(s, R"(realm\.(cap|demote)\s*\()") == 0, "5 realm.cap/demote");
    // 6：降尘丹 5、雪灵水 2 各恰 1 处 take，都在 biguan；dan_fu 只在 biguan 写。
    need(countAll(s, R"(take\("pill_jiangchen_dan",\s*5\))") == 1 && count(s, "biguan", R"(take\("pill_jiangchen_dan",\s*5\))") == 1, "6 jiangchen");
    need(countAll(s, R"(take\("material_xuelingshui",\s*2\))") == 1 && count(s, "biguan", R"(take\("material_xuelingshui",\s*2\))") == 1, "6 xuelingshui");
    need(countAll(s, R"(flag\.set\("ch11\.dan_fu")") == 1 && count(s, "biguan", R"(flag\.set\("ch11\.dan_fu")") == 1, "6 dan_fu writer");
    need(count(s, "biguan", R"(dan_fu\s*=\s*dan_fu\s*\+\s*1\b)") == 1 && count(s, "biguan", R"(dan_fu\s*=\s*dan_fu\s*\+\s*2\b)") == 1, "6 dan_fu bits");
    // 7：biguan 里 advance_days(3600) 恰 6 处，别的 advance_days 0 处。
    need(count(s, "biguan", R"(advance_days\(3600\))") == 6 && count(s, "biguan", R"(advance_days\()") == 6, "7 sixty years");
    // 8：youyao 开头判草、再判绿液；扣草在 battle 返回之后；② 由同一脚本调；nichang 恰一处给 3 株。
    need(ordered(at(s, "youyao", "item.count(\"herb_nichang_cao\")"), at(s, "youyao", "bottle.drops()")), "8 herb gate first");
    need(count(s, "youyao", R"(bottle\.spend\(1\))") == 1 && count(s, "youyao", R"(advance_days\(3\))") == 1, "8 drop and three days");
    need(count(s, "youyao", R"(advance_days\(5\))") == 2, "8 dry bottle waits five days");
    need(ordered(at(s, "youyao", "battle(\"b11_ningcuidao_youyao\")"), at(s, "youyao", "local lieyao")), "8 count after the battle");
    need(ordered(at(s, "youyao", "battle(\"b11_ningcuidao_youyao\")"), lastAt(s, "youyao", "take(\"herb_nichang_cao\", 1)")), "8 herb taken after battle");
    // 扣草恰两处：② 胜那一支、① 胜那一支（在 game_over 之后，逃与败都到不了）。
    need(count(s, "youyao", R"(take\("herb_nichang_cao",\s*1\))") == 2, "8 herb take count");
    need(ordered(at(s, "youyao", "if not won then game_over()"), lastAt(s, "youyao", "take(\"herb_nichang_cao\", 1)")), "8 herb only on a win");
    need(count(s, "youyao", R"(flag\.get\("ch11\.lieyao"\)\s*==\s*4\s+and\s+flag\.get\("ch11\.jiguan"\)\s*==\s*0)") == 1, "8 jiguan gate");
    need(ordered(at(s, "youyao", "battle(\"b11_honghu_jiguanjiao\")"), at(s, "youyao", "flag.set(\"ch11.jiguan\")")), "8 jiguan flag after battle");
    need(countAll(s, R"(give\("herb_nichang_cao",\s*3\))") == 1 && count(s, "nichang", R"(give\("herb_nichang_cao",\s*3\))") == 1, "8 three herbs");
    // 9：首次 2、此后 7，两处都有不足那一支（else），都落到同一句 teleport；huicheng 之后 return 在 teleport 之前。
    need(count(s, "xingkong", R"(if\s+take\("material_lingshi_zhong",\s*2\)\s+then[\s\S]*?else)") == 1, "9 first fare");
    need(count(s, "xingkong", R"(if\s+take\("material_lingshi_zhong",\s*7\)\s+then[\s\S]*?else)") == 1, "9 later fare");
    need(count(s, "xingkong", R"((^|\n)teleport\("ch11_ningcui_jie"\))") == 1, "9 top-level teleport");
    need(count(s, "xingkong", R"(flag\.get\("ch11\.huicheng"\)\s*>=\s*1\s+then[^\n]*return\s+end)") == 1, "9 closed after huicheng");
    // 10：七份脚本里的每一处 take 都看返回值：take( 落在同一行 if / elseif 与 then 之间（是条件的一部分）。
    // 「if x then take(...) end」不算——那是在 then 后面、返回值没人看（测试审查 T-4）。
    for (const char* name : {"zhihuan", "zufu", "ziliao", "ningcui", "zhaolan", "peizhu", "kaidian"}) {
        const auto found = s.find(name);
        if (found == s.end()) { out.push_back(std::string("10 missing ") + name); continue; }
        std::istringstream lines(executableLines(found->second));
        for (std::string line; std::getline(lines, line);)
            if (line.find("take(") != std::string::npos && !std::regex_search(line, std::regex(R"(\b(if|elseif)\s((?!\bthen\b).)*\btake\()")))
                out.push_back(std::string("10 unchecked take in ") + name);
    }
    // 11：第一根的计数在 n26 支、n29 每轮 1080 日 ＋ 一根、第六根补足到开店起 7200（至少 360）、收店给中阶 20。
    need(count(s, "peizhu", R"(flag\.set\("ch11\.zhu_gen",\s*1\))") == 1, "11 first root");
    need(count(s, "peizhu", R"(advance_days\(1080\))") == 1 && count(s, "peizhu", R"(give\("material_jinleizhu",\s*1\))") == 2, "11 roots");
    need(count(s, "peizhu", R"(math\.max\(360,\s*7200\s*-\s*\(today\(\)\s*-\s*flag\.get\("ch11\.kaidian_ri"\)\)\))") == 1, "11 top-up");
    need(count(s, "peizhu", R"(give\("material_lingshi_zhong",\s*20\))") == 1, "11 twenty mid stones");
    // 审查 C-7：n29 那一支先判铺子盘没盘下——没盘下一句 return，不扣绿液、不拨日。
    need(count(s, "peizhu", R"(if\s+flag\.get\("ch11\.kaidian"\)\s*==\s*0\s+then[^\n]*return\s+end)") == 1, "11 shop first");
    need(ordered(at(s, "peizhu", "flag.get(\"ch11.kaidian\") == 0"), lastAt(s, "peizhu", "bottle.spend(1)")) &&
         ordered(at(s, "peizhu", "flag.get(\"ch11.kaidian\") == 0"), at(s, "peizhu", "advance_days(1080)")), "11 shop gate before the drop");
    // 13：每满 30 日 15、一次封顶 240 个月、写回结算日。
    need(count(s, "jiesuan", R"(math\.floor\(\(today\(\)\s*-\s*flag\.get\("ch11\.jiesuan_ri"\)\)\s*/\s*30\))") == 1, "13 months");
    need(count(s, "jiesuan", R"(if\s+n\s*>\s*240\s+then\s+n\s*=\s*240)") == 1, "13 cap");
    need(count(s, "jiesuan", R"(give\("material_lingshi",\s*15\s*\*\s*n\))") == 1, "13 fifteen per month");
    need(count(s, "jiesuan", R"(flag\.set\("ch11\.jiesuan_ri",\s*today\(\)\))") == 1, "13 settle day");
    return out;
}

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

TEST(Ch11SliceScan, TheShippedScriptsSayExactlyWhatAcceptanceFiveToThirteenSays) {
    const auto errors = scanErrors(scripts());
    EXPECT_TRUE(errors.empty()) << joined(errors);
    // 全仓剧本 realm.demote 仍恰 1 处（第 9 章 shudong.lua；scripts/common 是接口定义，不算剧本）。
    int demotes = 0;
    for (const auto& dir : fs::directory_iterator(root() / "scripts"))
        if (dir.is_directory() && dir.path().filename() != "common")
            demotes += countAll(scripts(dir.path().filename().string()), R"(realm\.demote\s*\()");
    EXPECT_EQ(demotes, 1);
}

TEST(Ch11SliceScan, EachMutationOfTheScannedLinesIsCaught) {
    const Sources real = scripts();
    struct Bad { const char* script; const char* from; const char* to; };
    const Bad bads[] = {
        {"biguan", "realm.advance(realm.CORE_EARLY)", "realm.advance(realm.CORE_MID)"},
        {"liandan", "advance_days(15)", "advance_days(15)\nrealm.demote(22)"},
        {"zhuzhong", "flag.set(\"ch11.done\")", "realm.cap(32)\nflag.set(\"ch11.done\")"},
        {"biguan", "take(\"pill_jiangchen_dan\", 5)", "take(\"pill_jiangchen_dan\", 4)"},
        {"biguan", "take(\"material_xuelingshui\", 2)", "take(\"material_xuelingshui\", 1)"},
        {"biguan", "dan_fu = dan_fu + 2", "dan_fu = dan_fu + 1"},
        {"biguan", "advance_days(3600)\n", "advance_days(3650)\n"},
        {"biguan", "fade.out()", "fade.out()\nadvance_days(1)"},
        {"youyao", "if not bottle.spend(1)", "advance_days(3)\nif not bottle.spend(1)"},
        {"youyao", "if not won and how == \"escaped\"", "take(\"herb_nichang_cao\", 1)\nif not won and how == \"escaped\""},
        {"youyao", "== 4 and", "== 3 and"},
        {"nichang", "give(\"herb_nichang_cao\", 3)", "give(\"herb_nichang_cao\", 2)"},
        {"xingkong", "take(\"material_lingshi_zhong\", 7)", "take(\"material_lingshi_zhong\", 6)"},
        {"xingkong", "\nteleport(\"ch11_ningcui_jie\")", "\nif first then teleport(\"ch11_ningcui_jie\") end"},
        {"zufu", "if n > 0 and not take(", "take("},
        {"peizhu", "flag.set(\"ch11.zhu_gen\", 1)", "flag.set(\"ch11.zhu_gen\", 0)"},
        {"peizhu", "math.max(360,", "math.max(300,"},
        {"peizhu", "give(\"material_lingshi_zhong\", 20)", "give(\"material_lingshi_zhong\", 30)"},
        {"jiesuan", "if n > 240 then n = 240", "if n > 300 then n = 300"},
        {"jiesuan", "15 * n", "16 * n"},
        // T-4：把先后判据的后一头删掉，必须报（不能因 npos 最大而放过）；take 挪到 then 之后，必须报。
        {"youyao", "local lieyao", "local tally"},
        {"youyao", "flag.set(\"ch11.jiguan\")", "flag.set(\"ch11.jiguan_seen\")"},
        {"youyao", "bottle.drops()", "bottle.level()"},
        {"kaidian", "if n > 0 and not take(\"material_lingshi\", n) then talk(\"\", \"ch11.error.take\"); return end",
                    "if n > 0 then take(\"material_lingshi\", n) end"},
        // C-7：删掉「先盘店」那一句，或把它挪到扣绿液之后。
        {"peizhu", "if flag.get(\"ch11.kaidian\") == 0 then talk(\"\", \"ch11.peizhu.shop_first\"); return end\n", ""},
        {"peizhu",
         "if flag.get(\"ch11.kaidian\") == 0 then talk(\"\", \"ch11.peizhu.shop_first\"); return end\n"
         "if not bottle.spend(1) then talk(\"\", \"ch11.peizhu.wait\"); return end\n",
         "if not bottle.spend(1) then talk(\"\", \"ch11.peizhu.wait\"); return end\n"
         "if flag.get(\"ch11.kaidian\") == 0 then talk(\"\", \"ch11.peizhu.shop_first\"); return end\n"},
    };
    for (const auto& bad : bads) {
        Sources broken = real;
        auto& text = broken.at(bad.script);
        const auto where = text.find(bad.from);
        ASSERT_NE(where, std::string::npos) << bad.script << ": " << bad.from;
        text.replace(where, std::string(bad.from).size(), bad.to);
        EXPECT_FALSE(scanErrors(broken).empty()) << bad.script << ": " << bad.from << " -> " << bad.to;
    }
}

// ---- 切片 ----
void dropAll(GameState& state, const char* id) {
    const int have = state.itemCount(id);
    if (have > 0) ASSERT_TRUE(state.removeItem(id, have)) << id;
}

void setCount(GameState& state, const char* id, int wanted) {
    const int have = state.itemCount(id);
    if (have > wanted) ASSERT_TRUE(state.removeItem(id, have - wanted)) << id;
    if (have < wanted) state.addItem(id, wanted - have);
}

// 验收 5。起点 ch10-end-first.sav，走到 ch11.sangong。
TEST(Ch11Slice, FiveTheRealmIsHeldAtTwentyThreeBeforeTheSixtyYearsAndAtThirtyOneAfter) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(15)) << driver.problem();
    const auto& words = game::cultivationLexicon(game::PanelStage::Immortal);
    for (const int realm : {23, 31}) {
        SCOPED_TRACE(realm);
        // n16 之后密室唯一的出口格就是 n17 的 enter 触发器，先走出关（不动境界）再去静室。
        if (realm == 31) ASSERT_TRUE(driver.runTo(17)) << driver.problem();
        EXPECT_EQ(rules::toValue(driver.state().realm), realm);
        EXPECT_EQ(rules::toValue(driver.state().realmCap), realm);
        ASSERT_TRUE(driver.open("facility_jingshi_39")) << driver.problem();
        auto* panel = dynamic_cast<CultivationScene*>(driver.app().topScene());
        ASSERT_NE(panel, nullptr);
        const auto rows = CultivationScene::buildMainItems(driver.state());
        ASSERT_GE(rows.size(), 2u);
        EXPECT_FALSE(rows[1].enabled);
        EXPECT_EQ(rows[1].disabledReason, std::string(words.pushAtStoryCap));
        const auto before = driver.state();
        EXPECT_EQ(panel->breakthrough(driver.app()).blocked, rules::BreakthroughBlock::StoryCap);
        EXPECT_EQ(driver.state().cultivation, before.cultivation);
        EXPECT_EQ(driver.state().realm, before.realm);
        driver.close();
    }
    // 下一关「结丹中期」所需 5000（契约 5.2），修为攒够了也照样被 StoryCap 挡。
    EXPECT_EQ(rules::cultivationNeeded(rules::Realm::CoreEarly), 5000);
    EXPECT_EQ(rules::breakthroughBlock(rules::Realm::CoreEarly, 5000, driver.state().realmCap), rules::BreakthroughBlock::StoryCap);
    // 对照：上限若被抬到 32（本章 realm.cap 0 处），同样的修为就不再被 StoryCap 挡——上面那条不是恒真。
    EXPECT_NE(rules::breakthroughBlock(rules::Realm::CoreEarly, 5000, rules::Realm::CoreMid), rules::BreakthroughBlock::StoryCap);
}

// 验收 6、7。起点 ch10-end-first.sav 走到 ch11.sangong，三版背包（扰动只改两样丹物的数）。
TEST(Ch11Slice, SixAndSevenPillsAreTakenIfPresentAndSixtyYearsCostNoPlayTime) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(15)) << driver.problem();
    const GameState reached = driver.state();
    struct Version { int pills; int drops; int danFu; const char* key; };
    for (const Version v : {Version{5, 2, 3, "ch11.biguan.both"}, Version{0, 0, 0, "ch11.biguan.none"},
                            Version{4, 2, 2, "ch11.biguan.part"}}) {
        SCOPED_TRACE(v.key);
        ASSERT_TRUE(driver.resumeReached(reached, 16)) << driver.problem();
        setCount(driver.state(), "pill_jiangchen_dan", v.pills);
        setCount(driver.state(), "material_xuelingshui", v.drops);
        EXPECT_EQ(game::breakthroughPillBonus(driver.state()), 0);
        const int day = driver.state().day;
        const double played = driver.state().playSecondsGameplay;
        ASSERT_TRUE(driver.press("trigger_biguan")) << driver.problem();
        EXPECT_EQ(driver.state().flag("ch11.dan_fu"), v.danFu);
        EXPECT_EQ(driver.state().flag("ch11.jiedan"), 1);
        EXPECT_EQ(rules::toValue(driver.state().realm), 31);
        for (const char* key : {"ch11.biguan.both", "ch11.biguan.none", "ch11.biguan.part"})
            EXPECT_EQ(driver.spoke(key), std::string(key) == v.key) << key;
        EXPECT_EQ(driver.state().itemCount("pill_jiangchen_dan"), v.pills >= 5 ? v.pills - 5 : v.pills);
        EXPECT_EQ(driver.state().itemCount("material_xuelingshui"), v.drops >= 2 ? v.drops - 2 : v.drops);
        EXPECT_EQ(game::breakthroughPillBonus(driver.state()), 0);
        // 验收 7：六十年 ＝ day ＋21600；游玩时长只按真实帧累计（> 0 证明真在计，< 5 秒证明岁月不计）。
        EXPECT_EQ(driver.state().day - day, 21600);
        EXPECT_GT(driver.state().playSecondsGameplay - played, 0.0);
        EXPECT_LT(driver.state().playSecondsGameplay - played, 5.0);
    }
}

// 验收 8。起点 ch10-end-second.sav，走到 ch11.buzhen（意图玩家此刻手上 5 株草、5 滴绿液）。
TEST(Ch11Slice, EightTheLureLoopGatesOnHerbsCountsOnlyWinsAndTheFifthRoundIsTheCrestedDragon) {
    Driver driver;
    ASSERT_TRUE(driver.load(kSecond)) << driver.problem();
    ASSERT_TRUE(driver.runTo(11)) << driver.problem();
    const GameState reached = driver.state();
    ASSERT_EQ(reached.itemCount("herb_nichang_cao"), 5);
    {   // 草 0：不扣不拨日、不打、lieyao 不动，旁白指杂货。
        ASSERT_TRUE(driver.resumeReached(reached, 12)) << driver.problem();
        dropAll(driver.state(), "herb_nichang_cao");
        const GameState before = driver.state();
        ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        EXPECT_TRUE(driver.fights().empty());
        EXPECT_EQ(driver.state().flag("ch11.lieyao"), 0);
        EXPECT_EQ(driver.state().day, before.day);
        EXPECT_EQ(driver.state().bottle.drops, before.bottle.drops);
        EXPECT_TRUE(driver.spoke("ch11.youyao.no_herb"));
    }
    {   // ① 逃：草数不变、lieyao 不加、绿液已扣、day ＋3。
        ASSERT_TRUE(driver.resumeReached(reached, 12)) << driver.problem();
        driver.nextModes = {FightMode::Flee};
        const GameState before = driver.state();
        ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        ASSERT_EQ(driver.fights().size(), 1u);
        EXPECT_EQ(driver.fights()[0].id, kBattles[0]);
        EXPECT_EQ(driver.fights()[0].phase, BattlePhase::Escaped);
        EXPECT_EQ(driver.state().itemCount("herb_nichang_cao"), 5);
        EXPECT_EQ(driver.state().flag("ch11.lieyao"), 0);
        EXPECT_EQ(driver.state().bottle.drops, before.bottle.drops - 1);
        EXPECT_EQ(driver.state().day, before.day + 3);
        EXPECT_TRUE(driver.spoke("ch11.youyao.escaped"));
    }
    {   // 连打五轮：前四轮 ① 胜、各扣一株；第五轮是 ②，② 逃 → jiguan 置、六级妖丹 0、草不扣。
        ASSERT_TRUE(driver.resumeReached(reached, 12)) << driver.problem();
        for (int round = 0; round < 5; ++round) ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        ASSERT_EQ(driver.fights().size(), 5u);
        for (std::size_t i = 0; i < 4; ++i) EXPECT_EQ(driver.fights()[i].id, kBattles[0]);
        EXPECT_EQ(driver.fights()[4].id, kBattles[1]);
        EXPECT_EQ(driver.fights()[4].phase, BattlePhase::Escaped);
        EXPECT_EQ(driver.state().flag("ch11.lieyao"), 4);
        EXPECT_EQ(driver.state().flag("ch11.jiguan"), 1);
        EXPECT_EQ(driver.state().itemCount("material_yaodan_liuji"), 0);
        EXPECT_EQ(driver.state().itemCount("herb_nichang_cao"), 1);
        EXPECT_EQ(driver.state().itemCount("material_yaodan_wuji"), reached.itemCount("material_yaodan_wuji") + 4);
        EXPECT_EQ(driver.state().day, reached.day + 15);
        // 第六轮又回到 ①（jiguan 已置），且 lieyao 照加。
        ASSERT_TRUE(driver.press("trigger_youyao")) << driver.problem();
        EXPECT_EQ(driver.fights().back().id, kBattles[0]);
        EXPECT_EQ(driver.state().flag("ch11.lieyao"), 5);
    }
}

// 验收 9。起点 ch10-end-first.sav：走到 ch11.nichang 后把中阶清零；另一趟走到 ch11.huicheng。
TEST(Ch11Slice, NineTheTransferAlwaysWorksWithZeroMidStonesAndClosesAfterTheReturn) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(8)) << driver.problem();
    dropAll(driver.state(), "material_lingshi_zhong");
    ASSERT_TRUE(driver.travel("ch11_xingkong")) << driver.problem();
    ASSERT_TRUE(driver.press("npc_xingkong_xiushi")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch11_ningcui_jie");
    EXPECT_EQ(driver.state().flag("ch11.chuansong"), 1);
    EXPECT_TRUE(driver.spoke("ch11.xingkong.covered"));
    // 回程阵（enter、不置旗）→ 星空殿；第二次出海仍是 0 块：搭阵，照样落在凝翠岛。
    ASSERT_TRUE(driver.enter("trigger_huicheng_chuansong")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch11_xingkong");
    ASSERT_TRUE(driver.press("npc_xingkong_xiushi")) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch11_ningcui_jie");
    EXPECT_TRUE(driver.spoke("ch11.xingkong.ride"));
    EXPECT_EQ(driver.state().itemCount("material_lingshi_zhong"), 0);
    // huicheng 之后：只说一句，不 teleport、不扣钱、不拨日。
    Driver home;
    ASSERT_TRUE(home.load(kFirst)) << home.problem();
    ASSERT_TRUE(home.runTo(14)) << home.problem();
    ASSERT_TRUE(home.travel("ch11_xingkong")) << home.problem();
    const GameState before = home.state();
    ASSERT_TRUE(home.press("npc_xingkong_xiushi")) << home.problem();
    EXPECT_EQ(home.state().mapId, "ch11_xingkong");
    EXPECT_EQ(home.state().day, before.day);
    EXPECT_EQ(home.state().itemCount("material_lingshi_zhong"), before.itemCount("material_lingshi_zhong"));
    EXPECT_TRUE(home.spoke("ch11.xingkong.closed"));
}

// 验收 10。起点 ch10-end-first.sav 把中阶改 0；走到 n18、n26 之前再各清零一次（其间 n15 / n20 / Z1 会给）。
TEST(Ch11Slice, TenZeroMidStonesStillPayWhatTheyHaveAndEveryNodeGoesThrough) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    dropAll(driver.state(), "material_lingshi_zhong");
    const int low = driver.state().itemCount("material_lingshi");
    ASSERT_TRUE(driver.runTo(2)) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch11.zhihuan"), 2);
    EXPECT_EQ(driver.state().itemCount("material_lingshi"), low - 3);
    ASSERT_TRUE(driver.runTo(4)) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch11.zufu"), 1);
    EXPECT_EQ(driver.state().itemCount("story_jinzhi_lingpai"), 1);
    EXPECT_TRUE(driver.spoke("ch11.zufu.credit"));
    EXPECT_FALSE(driver.spoke("ch11.zufu.paid"));
    ASSERT_TRUE(driver.runTo(17)) << driver.problem();
    dropAll(driver.state(), "material_lingshi_zhong");
    ASSERT_TRUE(driver.runTo(18)) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("story_kuilei_yuan_qun"), 1);
    EXPECT_TRUE(driver.spoke("ch11.zhaolan.empty"));
    ASSERT_TRUE(driver.runTo(25)) << driver.problem();
    dropAll(driver.state(), "material_lingshi_zhong");
    ASSERT_TRUE(driver.runTo(26)) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("material_lianjian_fuliao"), 1);
    EXPECT_EQ(driver.state().flag("ch11.jinlei"), 1);
    // 对照：真实起点（中阶 117）那一趟是红环、租金照付（通关测试的 checkEnding 钉 zhihuan == 1）。
    Driver paid;
    ASSERT_TRUE(paid.load(kFirst)) << paid.problem();
    ASSERT_TRUE(paid.runTo(4)) << paid.problem();
    EXPECT_EQ(paid.state().flag("ch11.zhihuan"), 1);
    EXPECT_TRUE(paid.spoke("ch11.zufu.paid"));
    EXPECT_FALSE(paid.spoke("ch11.zufu.credit"));
}

// 验收 11。起点 ch10-end-first.sav 走到 ch11.kaidian；三版：照常、开店后多坐 1000 日、多坐 2000 日。
TEST(Ch11Slice, ElevenSixRootsCloseTheShopTwentyYearsAfterOpeningAndOnlyThenOpenTheForge) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(27)) << driver.problem();
    const GameState reached = driver.state();
    // 收店之前炼器台不开（require_flag ch11.liugen）。
    ASSERT_TRUE(driver.travel("ch11_dongfu")) << driver.problem();
    ASSERT_TRUE(driver.open("facility_lianqi_39")) << driver.problem();
    EXPECT_EQ(dynamic_cast<game::AlchemyScene*>(driver.app().topScene()), nullptr);
    driver.close();
    int closingDay = 0;
    for (const int extra : {0, 1000, 2000}) {
        SCOPED_TRACE(extra);
        ASSERT_TRUE(driver.resumeReached(reached, 28)) << driver.problem();
        if (extra) ASSERT_TRUE(driver.travel("ch11_dongfu") && driver.sit(extra)) << driver.problem();
        ASSERT_TRUE(driver.runTo(29)) << driver.problem();
        const int open = driver.state().flag("ch11.kaidian_ri");
        // 3.2 n29：开店到第六根之间过了 5400 ＋ extra 日；补足到 7200，不足 360 按 360。
        EXPECT_EQ(driver.state().day - open, extra == 2000 ? 5400 + 2000 + 360 : 7200);
        if (extra == 0) closingDay = driver.state().day;
        if (extra == 1000) EXPECT_EQ(driver.state().day, closingDay);
        EXPECT_EQ(driver.state().itemCount("material_jinleizhu"), 6);
        EXPECT_EQ(driver.state().flag("ch11.zhu_gen"), 6);
        EXPECT_EQ(driver.state().flag("ch11.liugen"), 1);
        ASSERT_TRUE(driver.stepIntoSecretRoom() && driver.enter("trigger_lianjian")) << driver.problem();
        ASSERT_TRUE(driver.open("facility_lianqi_39")) << driver.problem();
        auto* forge = dynamic_cast<game::AlchemyScene*>(driver.app().topScene());
        ASSERT_NE(forge, nullptr);
        forge->leave(driver.app());
        driver.app().tick(1.0 / 60.0);
    }
}

// 验收 13。起点 ch10-end-first.sav 走到 ch11.kaidian；开店当日、拨 90 日、拨 9000 日各按一次柜台后的结算格。
TEST(Ch11Slice, ThirteenTheShopPaysFifteenAMonthCappedAtTwoHundredFortyMonths) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(27)) << driver.problem();
    // 开店当日：0 个月 → 不给、不计次（3.2 n28「n == 0 → 一句 return」；验收 13 原写「jiesuan == 1」，施工图 18.5 裁定按 3.2）。
    int low = driver.state().itemCount("material_lingshi");
    ASSERT_TRUE(driver.visitShop()) << driver.problem();
    EXPECT_EQ(driver.state().itemCount("material_lingshi"), low);
    EXPECT_EQ(driver.state().flag("ch11.jiesuan"), 0);
    EXPECT_TRUE(driver.spoke("ch11.jiesuan.none"));
    struct Visit { int days; int paid; int times; const char* key; };
    for (const Visit v : {Visit{90, 45, 1, "ch11.jiesuan.first"}, Visit{9000, 3600, 2, "ch11.jiesuan.sum"}}) {
        SCOPED_TRACE(v.days);
        ASSERT_TRUE(driver.travel("ch11_dongfu") && driver.sit(v.days)) << driver.problem();
        driver.clearKeys();
        low = driver.state().itemCount("material_lingshi");
        ASSERT_TRUE(driver.visitShop()) << driver.problem();
        EXPECT_EQ(driver.state().itemCount("material_lingshi"), low + v.paid);
        EXPECT_EQ(driver.state().flag("ch11.jiesuan"), v.times);
        EXPECT_EQ(driver.state().flag("ch11.jiesuan_ri"), driver.state().day);
        EXPECT_TRUE(driver.spoke(v.key));
        EXPECT_EQ(driver.spoke("ch11.jiesuan.first"), v.times == 1);
    }
    // 9000 日 = 300 个月：不封顶会是 4500，上面那一条 3600 才咬得住封顶；封顶改坏由扫描负向
    // 「n > 300」抓（EachMutationOfTheScannedLinesIsCaught），这里不再放常量比较。
}

// ---- 手动推一个挂点：不经 pump，好在每一句对白还在栈顶时看它 ----
void settle(Driver& driver) {
    for (int frame = 0; frame < 600 && driver.app().scripts().isRunning() && !driver.app().awaitingCommand(); ++frame)
        driver.app().tick(1.0 / 60.0);
}
void answer(Driver& driver) {
    script::CommandResult reply;
    reply.ok = true;
    reply.choiceIndex = 0;
    driver.app().completeCommand(reply);
    driver.app().popScene();
    settle(driver);
}
bool dialogueOnTop(Driver& driver) { return dynamic_cast<game::DialogueScene*>(driver.app().topScene()) != nullptr; }
bool shopOnTop(Driver& driver) { return dynamic_cast<game::ShopScene*>(driver.app().topScene()) != nullptr; }

// 验收 17（测试审查 T-2 之一）。起点 ch10-end-first.sav 走到 ch11.luer（ch11.zufu 未置）：
// 真 tryStep 踩坊市北口 portal_shangshan → 不换图、说 ch11.block.dongfu；对照：租府之后同一扇门放行。
TEST(Ch11Slice, SeventeenTheMountainGateRefusesBeforeTheLeaseAndOpensAfterIt) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(3)) << driver.problem();
    ASSERT_EQ(driver.state().flag("ch11.zufu"), 0);
    const auto gate = driver.object("portal_shangshan");
    ASSERT_FALSE(gate.name.empty());
    ASSERT_TRUE(driver.walk(gate.position, true)) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch11_fangshi");
    ASSERT_TRUE(dialogueOnTop(driver));
    ASSERT_FALSE(driver.app().dialogueLog().empty());
    EXPECT_EQ(driver.app().dialogueLog().back().body, driver.app().text("ch11.block.dongfu"));
    driver.app().popScene();
    driver.app().tick(1.0 / 60.0);
    // 对照：n04 租下洞府（teleport 进洞府），再下山走同一扇门，就上得去。
    ASSERT_TRUE(driver.runTo(4)) << driver.problem();
    ASSERT_TRUE(driver.travel("ch11_fangshi")) << driver.problem();
    ASSERT_TRUE(driver.walk(driver.object("portal_shangshan").position, true)) << driver.problem();
    EXPECT_EQ(driver.state().mapId, "ch11_dongfu");
}

// 验收 17（测试审查 T-2 之二）。起点 ch10-end-first.sav：走到 ch11.kaidian 是对照（柜台开 ShopScene），
// 再走到 ch11.liugen：柜台与结算格各只说一句（栈顶 DialogueScene、不开 ShopScene、低阶不变）。
TEST(Ch11Slice, SeventeenAfterTheShopIsSoldTheCounterAndTheLedgerSayOneLineEach) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(27)) << driver.problem();
    ASSERT_TRUE(driver.travel("ch11_xiaoxuan")) << driver.problem();
    driver.clearKeys();
    ASSERT_TRUE(driver.open("trigger_guitai")) << driver.problem();
    settle(driver);
    ASSERT_TRUE(dialogueOnTop(driver));
    EXPECT_EQ(driver.app().spokenKeys(), std::vector<std::string>{"ch11.guitai.open"});
    answer(driver);
    auto* shop = dynamic_cast<game::ShopScene*>(driver.app().topScene());
    ASSERT_NE(shop, nullptr) << "对照：盘店之后、收店之前柜台要开店面";
    shop->leave(driver.app());
    settle(driver);
    driver.app().tick(1.0 / 60.0);
    ASSERT_FALSE(driver.app().scripts().isRunning());

    ASSERT_TRUE(driver.runTo(29)) << driver.problem();
    ASSERT_EQ(driver.state().flag("ch11.liugen"), 1);
    ASSERT_TRUE(driver.travel("ch11_xiaoxuan")) << driver.problem();
    for (const auto& [hook, key] : {std::pair{"trigger_guitai", "ch11.guitai.closed"},
                                    std::pair{"trigger_jiesuan", "ch11.jiesuan.closed"}}) {
        SCOPED_TRACE(hook);
        const GameState before = driver.state();
        driver.clearKeys();
        ASSERT_TRUE(driver.open(hook)) << driver.problem();
        settle(driver);
        ASSERT_TRUE(dialogueOnTop(driver));
        EXPECT_EQ(driver.app().spokenKeys(), std::vector<std::string>{key});
        answer(driver);
        driver.app().tick(1.0 / 60.0);
        EXPECT_FALSE(driver.app().scripts().isRunning());
        EXPECT_FALSE(shopOnTop(driver));
        EXPECT_EQ(driver.app().spokenKeys(), std::vector<std::string>{key});
        EXPECT_EQ(driver.state().itemCount("material_lingshi"), before.itemCount("material_lingshi"));
        EXPECT_EQ(driver.state().flag("ch11.jiesuan"), before.flag("ch11.jiesuan"));
        EXPECT_EQ(driver.state().day, before.day);
    }
}

// 审查 C-7。起点 ch10-end-first.sav 走到 ch11.jinlei（铺子还没盘下）：按培竹 → 只说一句「先盘店」，
// 第二根不长、日子不动、绿液不扣；对照：盘店之后同一处按下去，第二根长出来、拨 1080 日。
TEST(Ch11Slice, ElevenNoSecondRootGrowsBeforeTheShopIsBought) {
    Driver driver;
    ASSERT_TRUE(driver.load(kFirst)) << driver.problem();
    ASSERT_TRUE(driver.runTo(26)) << driver.problem();
    ASSERT_EQ(driver.state().flag("ch11.kaidian"), 0);
    const GameState before = driver.state();
    driver.clearKeys();
    ASSERT_TRUE(driver.peizhu()) << driver.problem();
    EXPECT_TRUE(driver.spoke("ch11.peizhu.shop_first"));
    EXPECT_EQ(driver.state().flag("ch11.zhu_gen"), 1);
    EXPECT_EQ(driver.state().day, before.day);
    EXPECT_EQ(driver.state().bottle.drops, before.bottle.drops);
    EXPECT_EQ(driver.state().itemCount("material_jinleizhu"), before.itemCount("material_jinleizhu"));
    EXPECT_EQ(driver.state().flag("ch11.liugen"), 0);
    // 对照：n27 盘店、n28 再按（驱动的第 28 节先按培竹再回访）。
    driver.clearKeys();
    ASSERT_TRUE(driver.runTo(28)) << driver.problem();
    EXPECT_EQ(driver.state().flag("ch11.zhu_gen"), 2);
    EXPECT_FALSE(driver.spoke("ch11.peizhu.shop_first"));
}
} // namespace

