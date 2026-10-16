// 第 13 章账（只做静态部分：扫脚本、读商店 / 配方 / 任务 / 路径行动）。对应 docs/ch13-design.md 第 15 节验收与第 9 节
//「账要对得上的几条」：
//   第 5 条  realm.cap 恰 1 处（biguan.lua，看返回值）、realm.advance / realm.demote 0 处；party.* 0 处（E 表 37）。
//   第 7 条  chuansong.lua：支一 take 灵石 10000、支二 take 五级妖丹 4 都看返回值，支三 0 处 take；选项只列付得起的、第三支永远在；
//            ch13.toudu 先于 ch13.chuansong 置。
//   第 8 条  youyao.lua：biguan 门闸在先；缺草且灵石 >= 10 一句 return、< 10 拨日给一株 return；缺液 advance_days(5) return；
//            bottle.spend(1) → advance_days(3) → battle；逃在扣草之前 return；胜扣草、lieyao ＋1。chongmo.lua 给六级 6、五级 10、
//            六七级材料 1、advance_days(2415)。
//   第 9 条  炼晶、肋骨、五色圆珠的 take 各恰 1 处、看返回值；magic.learn 五行环（wenxun）与蜂云合剑（chonglian）全仓各恰 1 处、
//            看返回值；金丝蚕只在 wusezhu.lua 按余数 take。
//   第 10 条 recipe_zhiyuan_dan：requireFlag ch13.liandan、productCount 3、不写 guaranteed；gongsun.lua 三支按「丹 >= 3 →
//            六级 >= 1 → 开炉一月」判，开炉那一支 advance_days(30)；shouchao、zhiyuan 先于 gongsun 置。
//   第 11 条 mishi.lua 三支（五级 1 / 灵石 5000 / 以材料抵），三支之后都给铜片；take 六七级材料看返回值；shop 一次。
//   第 12 条 flag.set("ch13.shouchao") 全仓恰 1 处（gongsun.lua）；四家店各只被一个脚本 shop()。
//   第 14 条 四家店 buyRate / sellRate 与条目照第 9 节；take 灵石只在 chuansong、mishi；第 9 节 T1–T7。
//   第 18 条 路径行动 13 条（16.3 字面量）；第 19 条 支线 5 条（第 6 节字面量）。
//   第 23 条 第 13 节第 9、10 条里能按字扫的 0 处词（全章文案）。
// 每条判据都有「在内存里改坏 → 必须报」的负向。切片那一半归测试路 B。
#include <gtest/gtest.h>
#include <tuple>
#include "Ch13Data.h"

namespace {
using namespace fanren::test::ch13;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

// 全仓 scripts/**（键 = 相对 scripts/ 的路径），全仓口径的那几条用它。
Sources allScripts() {
    Sources out;
    const fs::path base = root() / "scripts";
    for (const auto& file : fs::recursive_directory_iterator(base))
        if (file.path().extension() == ".lua") out[fs::relative(file.path(), base).generic_string()] = read(file.path());
    return out;
}

int count(const std::string& source, const std::string& pattern) {
    return countMatches(executableLines(source), std::regex(pattern));
}

std::string code(const Sources& sources, const char* name) {
    const auto found = sources.find(name);
    return found == sources.end() ? std::string() : executableLines(found->second);
}

std::size_t at(const std::string& text, const std::string& needle) {
    const auto found = text.find(needle);
    return found == std::string::npos ? text.size() : found;
}

// 「只列付得起的支」：每一个列了付费支的 choice 都同时列出那条零成本的支，且有一个 choice 只剩它（3.2、7.1、18.1 第 5 条）。
bool freeBranchAlwaysOffered(const std::string& text, const std::vector<std::string>& paid, const std::string& free) {
    const std::regex call(R"lua(choice\(\{([^}]*)\}\))lua");
    bool alone = false, ok = true;
    for (std::sregex_iterator it(text.begin(), text.end(), call), end; it != end; ++it) {
        const std::string options = (*it)[1].str();
        const bool lists = [&] {
            for (const auto& p : paid) if (options.find("\"" + p + "\"") != std::string::npos) return true;
            return false;
        }();
        const bool hasFree = options.find("\"" + free + "\"") != std::string::npos;
        if (lists && !hasFree) ok = false;
        if (options == "\"" + free + "\"") alone = true;
    }
    return ok && alone;
}

// 3.2 的 give：本章新 story 各恰 1 处，在给它的那一个节点脚本里（第 9 节 T4）。
const std::vector<std::pair<const char*, const char*>>& storyGives() {
    static const std::vector<std::pair<const char*, const char*>> kGives{
        {"story_mihuan_yan", "zhuibing"}, {"story_baiyu_dayin", "zhuibing"}, {"story_huangse_zhihuan", "rucheng"},
        {"story_guyu_yujian", "cunjin"}, {"story_qiyuan_haitu", "chucheng"}, {"story_wuse_yuanzhu", "chongshi"},
        {"story_xuanyin_jing", "xuanyin"}, {"story_liuqi_cailiao", "chongmo"}, {"story_tongpian", "mishi"},
        {"story_banyao_cao", "banyao"}, {"story_yanghun_zhu", "yanghun"}};
    return kGives;
}

// ---- 第 5、7、8、9、10、11、12、14 条：扫本章脚本 ----
Errors scriptErrors(const Sources& ch13, const Sources& everywhere) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    const auto total = [&](const Sources& sources, const std::string& pattern) {
        int n = 0;
        for (const auto& [name, source] : sources) n += count(source, pattern);
        return n;
    };
    const auto onlyIn = [&](const std::string& pattern, const std::set<std::string>& owners, int each) {
        for (const auto& [name, source] : ch13) {
            const int n = count(source, pattern);
            if (owners.count(name) ? n != each : n != 0) out.push_back(pattern + " occurs " + std::to_string(n) + " times in " + name);
        }
        for (const auto& owner : owners)
            if (!ch13.count(owner)) out.push_back(owner + ".lua missing");
    };
    const auto has = [&](const char* name, const std::string& pattern) { return std::regex_search(code(ch13, name), std::regex(pattern)); };
    // 第 5 条
    onlyIn(R"(\brealm\.cap\s*\()", {"biguan"}, 1);
    need(has("biguan", R"(local\s+(\w+)\s*=\s*realm\.cap\(realm\.CORE_MID\)[\s\S]*\bif\s+\1\b)"), "biguan.lua does not look at realm.cap's result");
    need(total(ch13, R"(\brealm\.(advance|demote)\w*\s*\()") == 0, "realm.advance / realm.demote in chapter 13");
    need(total(ch13, R"(\bparty\.\w+\s*\()") == 0, "party.* in chapter 13");
    // 每一处 take 都看返回值（3.2 首段）：前面紧跟 if 或 not。
    need(total(ch13, R"(\btake\s*\()") == total(ch13, R"(\b(?:if|not)\s+take\s*\()"), "a take( whose result is ignored");
    // 第 7 条
    const std::string chuansong = code(ch13, "chuansong");
    need(chuansong.find("item.count(\"material_lingshi\") >= 10000") != std::string::npos &&
         chuansong.find("item.count(\"material_yaodan_wuji\") >= 4") != std::string::npos, "chuansong.lua does not list only what he can pay");
    onlyIn(R"(\btake\s*\(\s*"material_lingshi",\s*10000\))", {"chuansong"}, 1);
    onlyIn(R"(\btake\s*\(\s*"material_yaodan_wuji",\s*4\))", {"chuansong"}, 1);
    need(freeBranchAlwaysOffered(chuansong, {"ch13.chuansong.opt_stone", "ch13.chuansong.opt_dan"}, "ch13.chuansong.opt_blade"),
         "chuansong.lua: the third way is not always offered");
    onlyIn(R"(flag\.set\("ch13\.toudu",\s*\w+\))", {"chuansong"}, 1);
    {
        // 先断言两头都在、再比先后（at() 找不到返回 size()，缺后一项时「先后」恒真；测试审查 T-5）。
        const std::size_t toudu = at(chuansong, "flag.set(\"ch13.toudu\""), sent = at(chuansong, "flag.set(\"ch13.chuansong\")");
        need(toudu < chuansong.size() && sent < chuansong.size(), "chuansong.lua lacks flag.set(\"ch13.toudu\") or flag.set(\"ch13.chuansong\")");
        need(toudu < sent, "toudu is not set before chuansong");
    }
    // 第 8 条：诱妖循环（3.2 n24、16.1 第 53 条）。
    const std::string youyao = code(ch13, "youyao");
    const auto first = liveLines(ch13.count("youyao") ? ch13.at("youyao") : std::string());
    need(!first.empty() && first[0].rfind("if flag.get(\"ch13.biguan\") ~= 0 then", 0) == 0 && first[0].find("return") != std::string::npos,
         "youyao.lua does not close first on ch13.biguan");
    need(has("youyao", R"(if item\.count\("herb_nichang_cao"\) < 1 then\n\s*if item\.count\("material_lingshi"\) >= 10 then[^\n]*\breturn end\n\s*advance_days\(30\)\n\s*give\("herb_nichang_cao", 1\)\n(?:\s*talk\([^\n]*\n)*\s*return\nend)"),
         "youyao.lua: the out-of-grass branches differ from 3.2 / 16.1 #53");
    need(has("youyao", R"(if bottle\.drops\(\) < 1 then[^\n]*advance_days\(5\);\s*return end)"), "youyao.lua: no drop does not cost five days and return");
    {
        const std::size_t spend = at(youyao, "bottle.spend(1)"), days = at(youyao, "advance_days(3)"), fight = at(youyao, "battle(");
        const std::size_t herb = at(youyao, "take(\"herb_nichang_cao\", 1)"), win = at(youyao, "flag.set(\"ch13.lieyao\", flag.get(\"ch13.lieyao\") + 1)");
        need(spend < days && days < fight && fight < herb && herb < win && win < youyao.size(), "youyao.lua order is not drop → days → battle → grass → count");
        const std::string after = fight < herb ? youyao.substr(fight, herb - fight) : std::string();
        need(after.find("\"escaped\"") != std::string::npos && after.find("return") != std::string::npos, "youyao.lua takes the grass after an escape");
    }
    onlyIn(R"(\bbottle\.spend\(1\))", {"youyao"}, 1);
    onlyIn(R"(\bbottle\.spend\(3\))", {"yanghun"}, 1);
    need(total(ch13, R"(\bbottle\.spend\s*\()") == 2, "bottle.spend count");
    onlyIn(R"(flag\.set\("ch13\.lieyao")", {"youyao"}, 1);
    // n25 收手（3.2、3.3、第 9 节）：七年一笔拨日、六级 6 ＋ 五级 10 ＋ 六七级材料 1，都顶格。
    for (const char* line : {R"((?:^|\n)advance_days\(2415\))", R"((?:^|\n)give\("material_yaodan_liuji", 6\))",
                             R"((?:^|\n)give\("material_yaodan_wuji", 10\))", R"((?:^|\n)give\("story_liuqi_cailiao", 1\))"})
        need(countMatches(code(ch13, "chongmo"), std::regex(line)) == 1, std::string("chongmo.lua lacks the top-level ") + line);
    // 第 9 条
    onlyIn(R"(\btake\s*\(\s*"material_lianjing",\s*1\))", {"chonglian"}, 1);
    onlyIn(R"(\btake\s*\(\s*"story_xuangu_leigu",\s*1\))", {"xuanyin"}, 1);
    onlyIn(R"(\btake\s*\(\s*"story_wuse_yuanzhu",\s*1\))", {"wusezhu"}, 1);
    need(total(everywhere, R"(\bmagic\.learn\s*\(\s*"magic_ji_wuxinghuan")") == 1 &&
         has("wenxun", R"(local\s+(\w+)\s*=\s*magic\.learn\("magic_ji_wuxinghuan"\)[\s\S]*\bif\s+\1\b)"), "the ring is not learned once in wenxun.lua");
    need(total(everywhere, R"(\bmagic\.learn\s*\(\s*"magic_fengyun_heji")") == 1 &&
         has("chonglian", R"(local\s+(\w+)\s*=\s*magic\.learn\("magic_fengyun_heji"\)[\s\S]*\bif\s+\1\b)"), "the sword is not learned once in chonglian.lua");
    need(total(ch13, R"(\bmagic\.learn\s*\()") == 2, "chapter 13 learns something else");
    onlyIn(R"(\btake\s*\(\s*"story_jinsi_can")", {"wusezhu"}, 1);
    need(has("wusezhu", R"(local\s+(\w+)\s*=\s*item\.count\("story_jinsi_can"\)\s*\nif\s+\1\s*>\s*0\s+then\s*\n(?:\s*talk\([^\n]*\n)*\s*if\s+take\("story_jinsi_can",\s*\1\))"),
         "wusezhu.lua does not take the remaining silk silkworms by count");
    // 第 10 条：至元丹三支（丹 >= 3 → 六级 >= 1 → 开炉一月，永远在）；开炉那一支 30 日在分支里、不顶格。
    const std::string gongsun = code(ch13, "gongsun");
    need(gongsun.find("item.count(\"pill_zhiyuan_dan\") >= 3") != std::string::npos &&
         gongsun.find("item.count(\"material_yaodan_liuji\") >= 1") != std::string::npos, "gongsun.lua does not list only what he has");
    onlyIn(R"(\btake\s*\(\s*"pill_zhiyuan_dan",\s*3\))", {"gongsun"}, 1);
    need(freeBranchAlwaysOffered(gongsun, {"ch13.gongsun.opt_pill", "ch13.gongsun.opt_dan"}, "ch13.gongsun.opt_furnace"),
         "gongsun.lua: the furnace month is not always offered");
    need(countMatches(gongsun, std::regex(R"(\n[ \t]+advance_days\(30\))")) == 1 && countMatches(gongsun, std::regex(R"(\badvance_days\(30\))")) == 1,
         "gongsun.lua: the furnace month is not one branch-only advance_days(30)");
    onlyIn(R"(flag\.set\("ch13\.zhiyuan",\s*\w+\))", {"gongsun"}, 1);
    need(at(gongsun, "flag.set(\"ch13.shouchao\")") < at(gongsun, "flag.set(\"ch13.gongsun\")") &&
         at(gongsun, "flag.set(\"ch13.zhiyuan\"") < at(gongsun, "flag.set(\"ch13.gongsun\")") &&
         at(gongsun, "flag.set(\"ch13.gongsun\")") < gongsun.size(), "gongsun.lua sets shouchao / zhiyuan after gongsun");
    // 第 12 条：兽潮只经人口。
    need(total(everywhere, R"(flag\.set\("ch13\.shouchao")") == 1, "ch13.shouchao is set outside gongsun.lua somewhere in scripts/");
    onlyIn(R"(flag\.set\("ch13\.shouchao"\))", {"gongsun"}, 1);
    // 第 11 条：铜片必得。
    const std::string mishi = code(ch13, "mishi");
    need(freeBranchAlwaysOffered(mishi, {"ch13.mishi.opt_dan", "ch13.mishi.opt_stone"}, "ch13.mishi.opt_pile"),
         "mishi.lua: paying with the pile is not always offered");
    need(mishi.find("item.count(\"material_yaodan_wuji\") >= 1") != std::string::npos &&
         mishi.find("item.count(\"material_lingshi\") >= 5000") != std::string::npos, "mishi.lua does not list only what he can pay");
    onlyIn(R"(\btake\s*\(\s*"material_lingshi",\s*5000\))", {"mishi"}, 1);
    need(countMatches(mishi, std::regex(R"((?:^|\n)give\("story_tongpian", 1\))")) == 1 &&
         at(mishi, "give(\"story_tongpian\", 1)") > at(mishi, "take(\"material_lingshi\", 5000)"), "mishi.lua does not give the bronze slip after all three ways");
    onlyIn(R"(\btake\s*\(\s*"story_liuqi_cailiao",\s*1\))", {"mishi"}, 1);
    onlyIn(R"(flag\.set\("ch13\.tongpian",\s*\w+\))", {"mishi"}, 1);
    {
        // 同上：两头都在才比先后（T-5）。
        const std::size_t tongpian = at(mishi, "flag.set(\"ch13.tongpian\""), traded = at(mishi, "flag.set(\"ch13.jiaohuan\")");
        need(tongpian < mishi.size() && traded < mishi.size(), "mishi.lua lacks flag.set(\"ch13.tongpian\") or flag.set(\"ch13.jiaohuan\")");
        need(tongpian < traded, "tongpian is not set before jiaohuan");
    }
    // 第 9 节 T1：低阶灵石只进一处（① 战后储物袋 600）、只出两处（偷渡、铜片），都在「够才列」的支里。
    onlyIn(R"(\bgive\s*\(\s*"material_lingshi",)", {"zhuibing"}, 1);
    need(has("zhuibing", R"((?:^|\n)give\("material_lingshi", 600\))"), "zhuibing.lua does not give 600 stones");
    onlyIn(R"(\btake\s*\(\s*"material_lingshi",)", {"chuansong", "mishi"}, 1);
    // T2：中阶 give 两处（Z1 30、Z2 15）、take 0 处。
    onlyIn(R"(\bgive\s*\(\s*"material_lingshi_zhong",)", {"mingzhu", "xuanshang"}, 1);
    need(has("mingzhu", R"(give\("material_lingshi_zhong", 30\))") && has("xuanshang", R"(give\("material_lingshi_zhong", 15\))"), "mid-stone rewards");
    need(total(ch13, R"(\btake\s*\(\s*"material_lingshi_zhong")") == 0, "chapter 13 takes mid stones");
    // T3：妖丹 give 只在 chongmo.lua（两处），take 五处、各在「够才列」的支里。
    onlyIn(R"(\bgive\s*\(\s*"material_yaodan_)", {"chongmo"}, 2);
    onlyIn(R"(\btake\s*\(\s*"material_yaodan_)", {"chuansong", "gongsun", "mishi", "mingzhu", "xuanshang"}, 1);
    need(has("gongsun", R"(take\("material_yaodan_liuji", 1\))") && has("mishi", R"(take\("material_yaodan_wuji", 1\))") &&
         has("mingzhu", R"(take\("material_yaodan_liuji", 1\))") && has("xuanshang", R"(take\("material_yaodan_wuji", 3\))"), "yaodan take amounts");
    // T4：新 story 各 give 1 处；腰牌 0 处 take、灵乳 0 处读写。
    for (const auto& [item, owner] : storyGives()) {
        onlyIn(std::string(R"(\bgive\s*\(\s*")") + item + "\"", {owner}, 1);
        need(count(ch13.count(owner) ? ch13.at(owner) : "", std::string(R"(\bgive\s*\(\s*")") + item + R"(",\s*1\))") == 1,
             std::string(owner) + ".lua does not give one " + item);
    }
    need(total(everywhere, R"(\btake\s*\(\s*"story_miaoyin_yaopai")") + total(ch13, R"(\btake\s*\(\s*"story_miaoyin_yaopai")") == 0,
         "someone takes the Miaoyin waist tablet");
    need(total(ch13, R"(story_wannian_lingru)") == 0, "chapter 13 touches the spirit milk");
    onlyIn(R"(\bgive\s*\(\s*"herb_nichang_cao",)", {"lifu", "youyao", "xuanshang"}, 1);
    onlyIn(R"(\bfield\.unlock\("field_wuzhong", 6\))", {"lifu"}, 1);
    // T7 与验收 12：四家店各只被自己那一个脚本 shop()；秘市一次（3.2 n31）。
    const std::map<std::string, std::string> shops{{"ch13_nanming_fangshi", "caigou"}, {"ch13_mingzhu_shougou", "mingzhu"},
                                                   {"ch13_cunjin_ge", "cunjin"}, {"ch13_mishi_tan", "mishi"}};
    for (const auto& [shop, owner] : shops)
        for (const auto& [name, source] : ch13) {
            const int n = count(source, R"(\bshop\s*\(\s*")" + shop + "\"");
            if (name == owner ? n < 1 : n != 0) out.push_back(shop + " opened " + std::to_string(n) + " times in " + name);
        }
    need(total(ch13, R"(\bshop\s*\()") == total(ch13, R"re(\bshop\s*\(\s*"ch13_(?:nanming_fangshi|mingzhu_shougou|cunjin_ge|mishi_tan)")re"),
         "chapter 13 opens a shop outside the four");
    need(count(ch13.count("mishi") ? ch13.at("mishi") : "", R"(\bshop\s*\()") == 1, "mishi.lua opens the stalls more than once");
    return out;
}

TEST(Ch13Ledger, TheChapterScriptsKeepTheLedgerTheDesignWrote) {
    const auto found = scriptErrors(scripts(), allScripts());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch13Ledger, MutationsOfTheScriptLedgerAreCaught) {
    const Sources original = scripts();
    const Sources everywhere = allScripts();
    ASSERT_TRUE(scriptErrors(original, everywhere).empty()) << joined(scriptErrors(original, everywhere));
    struct Bad { const char* script; const char* from; const char* to; };   // from 为空 = 末尾追加 to
    const std::vector<Bad> bads{
        {"chuguan", "", "realm.advance()\n"},
        {"biguan", "local ok = realm.cap(realm.CORE_MID)", "realm.cap(realm.CORE_MID)\nlocal ok = true"},
        {"lidao", "", "party.add(\"qu_hun_shadan\")\n"},
        {"chuansong", "not take(\"material_lingshi\", 10000)", "false"},                        // 支一不再扣
        {"chuansong", "local p = choice({\"ch13.chuansong.opt_blade\"})", "local p = choice({})"},
        {"chuansong", "\"ch13.chuansong.opt_dan\", \"ch13.chuansong.opt_blade\"})", "\"ch13.chuansong.opt_dan\"})"},
        {"chuansong", "flag.set(\"ch13.toudu\", way)\nflag.set(\"ch13.chuansong\")", "flag.set(\"ch13.chuansong\")\nflag.set(\"ch13.toudu\", way)"},
        {"youyao", "if flag.get(\"ch13.biguan\") ~= 0", "if flag.get(\"ch13.chuguan\") ~= 0"},
        {"youyao", "advance_days(30)", "advance_days(5)"},
        {"youyao", "advance_days(3)", "advance_days(2)"},
        {"youyao", "if not won and how == \"escaped\" then talk(\"\", \"ch13.youyao.escaped\"); return end", ""},
        {"youyao", "flag.set(\"ch13.lieyao\", flag.get(\"ch13.lieyao\") + 1)", "flag.set(\"ch13.lieyao\", flag.get(\"ch13.lieyao\") + 2)"},
        {"youyao", "bottle.spend(1)", "bottle.spend(2)"},
        {"chongmo", "advance_days(2415)", "advance_days(3650)"},
        {"chongmo", "give(\"material_yaodan_liuji\", 6)", "give(\"material_yaodan_liuji\", 7)"},
        {"chonglian", "if take(\"material_lianjing\", 1) then", "take(\"material_lianjing\", 1)\nif true then"},
        {"wenxun", "local learned = magic.learn(\"magic_ji_wuxinghuan\")", "magic.learn(\"magic_ji_wuxinghuan\")\nlocal learned = true"},
        {"chongshi", "", "take(\"story_jinsi_can\", 3)\n"},
        {"wusezhu", "if n > 0 then", "if n >= 0 then"},
        {"gongsun", "local p = choice({\"ch13.gongsun.opt_furnace\"})", "local p = choice({})"},
        {"gongsun", "    advance_days(30)", "    advance_days(20)"},
        {"gongsun", "flag.set(\"ch13.shouchao\")\n", ""},
        {"feixu", "", "flag.set(\"ch13.shouchao\")\n"},
        {"mishi", "give(\"story_tongpian\", 1)", "if way ~= 3 then give(\"story_tongpian\", 1) end"},
        {"mishi", "if take(\"story_liuqi_cailiao\", 1) then", "take(\"story_liuqi_cailiao\", 1)\nif true then"},
        {"mishi", "shop(\"ch13_mishi_tan\")", "shop(\"ch13_mishi_tan\")\nshop(\"ch13_mishi_tan\")"},
        {"zhuibing", "give(\"material_lingshi\", 600)", "give(\"material_lingshi\", 900)"},
        {"rucheng", "", "if take(\"material_lingshi\", 50) then end\n"},                       // 进城收钱
        {"lidao", "", "if take(\"material_lingshi_zhong\", 1) then end\n"},
        {"jianshi", "", "give(\"material_yaodan_wuji\", 1)\n"},
        {"banyao", "", "give(\"story_tongpian\", 1)\n"},
        {"panzha", "", "if take(\"story_miaoyin_yaopai\", 1) then end\n"},                       // 腰牌只读不扣
        {"caigou", "shop(\"ch13_nanming_fangshi\")\n    return", "shop(\"ch13_cunjin_ge\")\n    return"},
        {"lifu", "field.unlock(\"field_wuzhong\", 6)", "field.unlock(\"field_wuzhong\", 4)"},
    };
    for (const auto& bad : bads) {
        Sources sources = original;
        std::string& text = sources[bad.script];
        if (*bad.from == '\0') {
            text += std::string("\n") + bad.to;
        } else {
            const auto found = findInCode(text, bad.from);
            ASSERT_NE(found, std::string::npos) << bad.script << ": " << bad.from;
            text.replace(found, std::string(bad.from).size(), bad.to);
        }
        EXPECT_FALSE(scriptErrors(sources, everywhere).empty()) << bad.script << " <- " << bad.to;
    }
    // T-5：删掉先后比较里的后一项（chuansong / jiaohuan 的置旗）→ 恰报「缺置旗」那一条（改前这两种删法一条不报）。
    for (const auto& [script, line, why] : {std::tuple{"chuansong", "flag.set(\"ch13.chuansong\")", "chuansong.lua lacks flag.set"},
                                            std::tuple{"mishi", "flag.set(\"ch13.jiaohuan\")", "mishi.lua lacks flag.set"}}) {
        Sources sources = original;
        std::string& text = sources[script];
        const auto found = findInCode(text, line);
        ASSERT_NE(found, std::string::npos) << script << ": " << line;
        text.erase(found, std::string(line).size());
        const auto errors = scriptErrors(sources, everywhere);
        EXPECT_EQ(errors.size(), 1u) << script << joined(errors);
        EXPECT_NE(joined(errors).find(why), std::string::npos) << script << joined(errors);
    }
    Sources twice = everywhere;
    twice["ch14/probe.lua"] = "magic.learn(\"magic_fengyun_heji\")\n";
    EXPECT_FALSE(scriptErrors(original, twice).empty()) << "全仓口径：蜂云合剑学两处";
    Sources tablet = everywhere;
    tablet["ch14/probe.lua"] = "if take(\"story_miaoyin_yaopai\", 1) then end\n";
    EXPECT_FALSE(scriptErrors(original, tablet).empty()) << "全仓口径：别章扣腰牌";
    Sources tide = everywhere;
    tide["ch14/probe.lua"] = "flag.set(\"ch13.shouchao\")\n";
    EXPECT_FALSE(scriptErrors(original, tide).empty()) << "全仓口径：别章再置兽潮";
}

// ---- 第 14 条：四家店（第 9 节「来源 / 去向」与 7 节「战乱封锁」：按地点分店、不做物价曲线）----
struct Stock { const char* item; int stock; int restockDays; int restockAmount; int maxStock; };   // stock < 0 = 只要求在架上
struct ShopRow { const char* id; double buyRate; double sellRate; bool exact; bool noRestock; std::vector<Stock> entries; };

// 买价 ＝ 物品价 × buyRate（第 9 节「去向」：南明岛 0.9「霓裳草 9、回气丹 450」；寸金阁 / 秘市霓裳草 −10、黄精 −5、玛瑙角 −5000
// → 1.0）；卖价第 9 节「来源」末行：明珠轩 0.8、寸金阁 0.5、秘市 0.6、南明岛 0.5。明珠轩只收不卖（entries 空，买价不起作用，
// 照 18.3 第 6 条记 1.0）。寸金阁的条目照 18.3 第 6 条逐件钉死；南明岛、秘市「限量不补」（第 9 节、18.3 第 6 条），
// 南明岛只钉施工图点名的两样，秘市不钉货单。
const std::vector<ShopRow>& shopRows() {
    static const std::vector<ShopRow> kRows{
        {"ch13_nanming_fangshi", 0.9, 0.5, false, true, {{"herb_nichang_cao", -1, 0, 0, 0}, {"pill_huiqi_dan", -1, 0, 0, 0}}},
        {"ch13_mingzhu_shougou", 1.0, 0.8, true, true, {}},
        {"ch13_cunjin_ge", 1.0, 0.5, true, false, {{"herb_nichang_cao", 5, 5, 3, 5}, {"herb_huangjing_cao", 5, 5, 3, 5},
                                                   {"story_manao_jiao", 1, 0, 0, 1}, {"pill_huiqi_dan", 3, 10, 1, 3}}},
        {"ch13_mishi_tan", 1.0, 0.6, false, true, {}},
    };
    return kRows;
}

Errors shopErrors(const std::map<std::string, Json>& files) {
    Errors out;
    std::set<std::string> designed;
    for (const auto& row : shopRows()) {
        designed.insert(row.id);
        const auto found = files.find(row.id);
        if (found == files.end()) { out.push_back(std::string("missing shop ") + row.id); continue; }
        const Json& shop = found->second;
        if (shop.value("buyRate", -1.0) != row.buyRate || shop.value("sellRate", -1.0) != row.sellRate) out.push_back(std::string(row.id) + " rates");
        if (shop.value("nameKey", "") != "ch13.shop.name." + std::string(row.id).substr(5)) out.push_back(std::string(row.id) + " nameKey");
        const Json entries = shop.value("entries", Json::array());
        if (row.exact && entries.size() != row.entries.size()) out.push_back(std::string(row.id) + " entry count");
        if (row.noRestock)
            for (const auto& e : entries)
                if (e.value("restockDays", 0) != 0 || e.value("restockAmount", 0) != 0) out.push_back(std::string(row.id) + " restocks " + e.value("itemId", ""));
        for (const Stock& s : row.entries) {
            const auto e = std::find_if(entries.begin(), entries.end(), [&](const Json& x) { return x.value("itemId", "") == s.item; });
            if (e == entries.end()) { out.push_back(std::string(row.id) + " lacks " + s.item); continue; }
            if (s.stock >= 0 && (e->value("stock", -1) != s.stock || e->value("restockDays", -1) != s.restockDays ||
                                 e->value("restockAmount", -1) != s.restockAmount || e->value("maxStock", -1) != s.maxStock))
                out.push_back(std::string(row.id) + " entry " + s.item);
        }
    }
    for (const auto& [id, shop] : files)
        if (id.rfind("ch13_", 0) == 0 && !designed.count(id)) out.push_back("undesigned shop " + id);
    return out;
}

std::map<std::string, Json> shopFiles() {
    std::map<std::string, Json> out;
    for (const auto& file : fs::directory_iterator(root() / "data" / "shops"))
        if (file.path().extension() == ".json") out[file.path().stem().string()] = json(file.path());
    return out;
}

TEST(Ch13Ledger, TheFourShopsAreTheOnesOfSectionNine) {
    const auto files = shopFiles();
    ASSERT_GT(files.size(), 10u) << "先验：读得到 data/shops";
    const auto found = shopErrors(files);
    EXPECT_TRUE(found.empty()) << joined(found);
    for (int variant = 0; variant < 6; ++variant) {
        auto bad = files;
        switch (variant) {
        case 0: bad["ch13_nanming_fangshi"]["buyRate"] = 1.0; break;              // 物价没有更便宜
        case 1: bad["ch13_mingzhu_shougou"]["sellRate"] = 0.5; break;
        case 2: bad["ch13_mingzhu_shougou"]["entries"].push_back({{"itemId", "pill_huiqi_dan"}}); break;
        case 3: bad["ch13_cunjin_ge"]["entries"][0]["restockDays"] = 0; break;   // 霓裳草不再补货
        case 4: bad["ch13_mishi_tan"]["entries"][0]["restockAmount"] = 3; break; // 秘市限量不补
        case 5: bad["ch13_heishi_zahuo"] = Json::object(); break;                // 第五家店
        }
        EXPECT_FALSE(shopErrors(bad).empty()) << "mutation=" << variant;
    }
}

// ---- 第 10 条与第 9 节 T7：配方 ----
Errors recipeErrors(const std::vector<Json>& recipes) {
    Errors out;
    int guaranteed = 0, ours = 0;
    for (const auto& recipe : recipes) {
        const std::string id = recipe.value("id", "");
        if (recipe.value("guaranteed", false)) ++guaranteed;
        const auto flag = recipe.find("requireFlag");
        const bool chapter13 = flag != recipe.end() && flag->is_string() && flag->get<std::string>().rfind("ch13.", 0) == 0;
        if (!chapter13 && id != "recipe_zhiyuan_dan") continue;
        ++ours;
        if (id != "recipe_zhiyuan_dan") { out.push_back(id + " is a second chapter-13 recipe"); continue; }
        if (recipe.value("kind", "") != "alchemy" || recipe.value("productId", "") != "pill_zhiyuan_dan" || recipe.value("productCount", 0) != 3)
            out.push_back("recipe_zhiyuan_dan kind/product/count");
        if (recipe.value("requireFlag", "") != "ch13.liandan") out.push_back("recipe_zhiyuan_dan requireFlag");
        if (recipe.contains("guaranteed")) out.push_back("recipe_zhiyuan_dan writes guaranteed");
        std::multiset<std::pair<std::string, int>> inputs;
        for (const auto& input : recipe.value("inputs", Json::array())) inputs.insert({input.value("itemId", ""), input.value("count", 0)});
        if (inputs != std::multiset<std::pair<std::string, int>>{{"material_yaodan_wuji", 1}, {"herb_huangjing_cao", 1}})
            out.push_back("recipe_zhiyuan_dan inputs");
    }
    if (ours != 1) out.push_back("chapter-13 recipes: " + std::to_string(ours));
    if (guaranteed != 1) out.push_back("guaranteed recipes across the repo: " + std::to_string(guaranteed));
    return out;
}

std::vector<Json> recipeFiles() {
    std::vector<Json> out;
    for (const auto& file : fs::recursive_directory_iterator(root() / "data" / "recipes"))
        if (file.path().extension() == ".json") out.push_back(json(file.path()));
    return out;
}

TEST(Ch13Ledger, OneNewRecipeTheZhiyuanPillAndStillOnlyOneGuaranteedRecipe) {
    const auto recipes = recipeFiles();
    ASSERT_GT(recipes.size(), 15u) << "先验：读得到 data/recipes";
    const auto found = recipeErrors(recipes);
    EXPECT_TRUE(found.empty()) << joined(found);
    const auto index = [&recipes] {
        for (std::size_t i = 0; i < recipes.size(); ++i)
            if (recipes[i].value("id", "") == "recipe_zhiyuan_dan") return i;
        return recipes.size();
    }();
    ASSERT_LT(index, recipes.size());
    for (int variant = 0; variant < 6; ++variant) {
        auto bad = recipes;
        switch (variant) {
        case 0: bad[index]["guaranteed"] = true; break;
        case 1: bad[index]["requireFlag"] = "ch13.chongmo"; break;
        case 2: bad[index]["productCount"] = 2; break;
        case 3: bad[index]["inputs"][0]["count"] = 2; break;
        case 4: bad.push_back({{"id", "recipe_wujian_chonglian"}, {"kind", "forge"}, {"requireFlag", "ch13.xuanyin"}}); break;  // 重炼不走面板
        case 5: bad.erase(bad.begin() + static_cast<std::ptrdiff_t>(index)); break;
        }
        EXPECT_FALSE(recipeErrors(bad).empty()) << "mutation=" << variant;
    }
}

// ---- 第 19 条：支线 5 条（第 6 节字面量；完成压过失败，Quests.cpp 先判 complete）----
Json flagAtLeast(const char* flag, int value = 1) { return Json::array({{{"flag", flag}, {"op", ">="}, {"value", value}}}); }

struct QuestRow { const char* id; const char* accept; std::vector<const char*> steps; const char* complete; const char* fail; const char* origin; };

const std::vector<QuestRow>& questRows() {
    static const std::vector<QuestRow> kRows{
        {"q13_qianyelu", "ch13.caigou", {"ch13.cunjin", "ch13.wenshi"}, "ch13.done", "", "原著"},
        {"q13_gongsun_qiuyao", "ch13.chuguan", {"ch13.gongsun"}, "ch13.gongsun", "", "原著"},
        {"q13_mingzhu_dingdan", "ch13.mingzhu", {"ch13.z1"}, "ch13.z1", "ch13.biguan", "改编"},
        {"q13_haishou_xuanshang", "ch13.daoqiyuan", {"ch13.z2"}, "ch13.z2", "ch13.biguan", "改编"},
        {"q13_yanghun_zhu", "ch13.lifu", {"ch13.z3"}, "ch13.z3", "ch13.lidao", "原著"},
    };
    return kRows;
}

Errors questErrors(const std::map<std::string, Json>& quests) {
    Errors out;
    const std::set<std::string> chain(kChain.begin(), kChain.end());
    for (const auto& row : questRows()) {
        const auto found = quests.find(row.id);
        if (found == quests.end()) { out.push_back(std::string("missing quest ") + row.id); continue; }
        const Json& q = found->second;
        const std::string id = row.id;
        if (q.value("chapter", 0) != 13) out.push_back(id + " chapter");
        if (q.value("accept", Json()) != flagAtLeast(row.accept)) out.push_back(id + " accept");
        if (q.value("complete", Json()) != flagAtLeast(row.complete)) out.push_back(id + " complete");
        if (*row.fail == '\0' ? q.contains("fail") : q.value("fail", Json()) != flagAtLeast(row.fail)) out.push_back(id + " fail");
        if (q.value("origin", "").rfind(row.origin, 0) != 0) out.push_back(id + " origin");
        if (!q.value("rewards", Json::array()).empty()) out.push_back(id + " has rewards (第 6 节：不进 rewards，脚本 give)");
        const Json steps = q.value("steps", Json::array());
        if (steps.size() != row.steps.size()) { out.push_back(id + " step count"); continue; }
        for (std::size_t i = 0; i < steps.size(); ++i)
            if (steps[i].value("done", Json()) != flagAtLeast(row.steps[i])) out.push_back(id + " step " + std::to_string(i + 1));
        // 必经两条的 complete 在目标链上（验收 19）。
        if (*row.fail == '\0' && !chain.count(row.complete)) out.push_back(id + " completes off the chain");
    }
    for (const auto& [id, quest] : quests)
        if (id.rfind("q13_", 0) == 0 && std::none_of(questRows().begin(), questRows().end(), [&](const QuestRow& r) { return id == r.id; }))
            out.push_back("undesigned quest " + id);
    return out;
}

std::map<std::string, Json> questFiles() {
    std::map<std::string, Json> out;
    for (const auto& file : fs::directory_iterator(root() / "data" / "quests"))
        if (file.path().extension() == ".json") out[file.path().stem().string()] = json(file.path());
    return out;
}

TEST(Ch13Ledger, TheFiveSideQuestsMatchTheirPredicates) {
    const auto quests = questFiles();
    const auto found = questErrors(quests);
    EXPECT_TRUE(found.empty()) << joined(found);
    for (int variant = 0; variant < 8; ++variant) {
        auto bad = quests;
        switch (variant) {
        case 0: bad["q13_mingzhu_dingdan"].erase("fail"); break;
        case 1: bad["q13_yanghun_zhu"]["fail"] = flagAtLeast("ch13.biguan"); break;      // Z3 失败于 lidao
        case 2: bad["q13_yanghun_zhu"]["accept"] = flagAtLeast("ch13.buzhen"); break;    // 接得早、交得晚（第 6 节）
        case 3: bad["q13_qianyelu"]["steps"][1]["done"] = flagAtLeast("ch13.jiaohuan"); break;
        case 4: bad["q13_gongsun_qiuyao"]["fail"] = flagAtLeast("ch13.lidao"); break;
        case 5: bad["q13_haishou_xuanshang"]["origin"] = "原著"; break;
        case 6: bad["q13_haishou_xuanshang"]["rewards"] = Json::array({{{"item_id", "material_lingshi_zhong"}, {"count", 15}}}); break;
        case 7: bad["q13_yuntianxiao"] = {{"chapter", 13}}; break;
        }
        EXPECT_FALSE(questErrors(bad).empty()) << "mutation=" << variant;
    }
}

// ---- 第 18 条：路径行动 13 条（16.3 表；全 inquire、realm 0、0 条 reveal、无 give / set_flags）----
struct PathRow {
    const char* id;
    const char* map;
    const char* npc;
    std::vector<const char*> when;
    std::vector<const char*> until;   // 另含 ch13.done
};

const std::vector<PathRow>& pathRows() {
    static const std::vector<PathRow> kRows{
        {"sanxian_tianxingling_dating", "ch13_nanming", "npc_sanxian_laozhe", {"ch13.wenxun"}, {"ch13.gongdao"}},
        {"liuhufa_shaozhu_dating", "ch13_nanming", "npc_liu_hufa", {"ch13.gongdao"}, {"ch13.panzha"}},
        {"xinming_xinwu_dating", "ch13_nanming", "npc_xin_ming", {"ch13.gongdao"}, {"ch13.panzha"}},
        {"qianxiushi_chuansong_dating", "ch13_tianxing", "npc_qian_xiushi", {"ch13.yirong"}, {"ch13.chuansong"}},
        {"zhaohufa_jieyan_dating", "ch13_tianxing", "npc_zhao_hufa", {"ch13.yinjian"}, {"ch13.chuansong"}},
        {"yuhufa_chuansong_dating", "ch13_tianxing", "npc_yu_hufa", {"ch13.yinjian", "ch13.rucheng"}, {"ch13.chuansong"}},
        {"zhangxiushi_jiama_dating", "ch13_tianxing", "npc_zhang_xiushi", {"ch13.yirong"}, {"ch13.chuansong"}},
        {"yijing_waihai_dating", "ch13_waihai", "npc_yi_jing", {"ch13.chuansong"}, {"ch13.shouchao"}},
        {"xuyun_qiyuan_dating", "ch13_waihai", "npc_xu_yun", {"ch13.chuansong", "ch13.daoqiyuan"}, {"ch13.shouchao"}},
        {"paimaishi_shouluan_dating", "ch13_waihai", "npc_paimaishi", {"ch13.chuansong", "ch13.mingzhu"}, {"ch13.shouchao"}},
        {"guyu_shanggu_dating", "ch13_waihai", "npc_gu_yu", {"ch13.chuansong", "ch13.cunjin"}, {"ch13.shouchao"}},
        {"sunshishu_shouchao_dating", "ch13_wuzhong", "npc_sun_shishu", {"ch13.chuguan"}, {"ch13.gongsun"}},
        {"huangmingli_mishi_dating", "ch13_waihai", "npc_huang_mingli", {"ch13.lidao"}, {"ch13.jiaohuan"}},
    };
    return kRows;
}

bool containsFlag(const Json& conditions, const std::string& flag) {
    for (const auto& c : conditions)
        if (c.value("flag", "") == flag && c.value("op", "") == ">=" && c.value("value", 0) == 1) return true;
    return false;
}

Errors pathErrors(const Json& file, const std::map<std::string, Json>& maps) {
    Errors out;
    std::map<std::string, const PathRow*> rows;
    for (const auto& row : pathRows()) rows[row.id] = &row;
    std::set<std::string> seen;
    for (const auto& entry : file.at("entries")) {
        const std::string id = entry.value("id", "");
        const auto row = rows.find(id);
        if (row == rows.end()) { out.push_back("unexpected path action " + id); continue; }
        if (!seen.insert(id).second) out.push_back("duplicate path action " + id);
        const PathRow& r = *row->second;
        if (entry.value("kind", "") != "inquire" || entry.value("realm", -1) != 0) out.push_back(id + " kind/realm");
        for (const char* key : {"reveal", "give", "set_flags"})
            if (entry.contains(key)) out.push_back(id + " carries " + key);
        if (entry.value("map", "") != r.map || entry.value("npc", "") != r.npc) out.push_back(id + " map/npc");
        if (entry.value("done_flag", "") != "ch13.path." + id || entry.value("text_key", "") != "ch13.path." + id + ".text")
            out.push_back(id + " done_flag/text_key");
        const Json when = entry.value("when", Json::array()), until = entry.value("until", Json::array());
        if (when.size() != r.when.size()) out.push_back(id + " when size");
        for (const char* flag : r.when) if (!containsFlag(when, flag)) out.push_back(id + " when lacks " + flag);
        if (until.size() != r.until.size() + 1 || !containsFlag(until, "ch13.done")) out.push_back(id + " until size / ch13.done");
        for (const char* flag : r.until) if (!containsFlag(until, flag)) out.push_back(id + " until lacks " + flag);
        // when 含该 NPC 的 visible_flag、until 含它的 hidden_flag（读地图核，不读表）。
        const auto map = maps.find(r.map);
        const auto all = map == maps.end() ? std::map<std::string, Json>{} : objects(map->second);
        const auto npc = all.find(r.npc);
        if (npc == all.end()) { out.push_back(id + " npc missing on map"); continue; }
        const Json p = properties(npc->second);
        if (!containsFlag(when, p.value("visible_flag", ""))) out.push_back(id + " when lacks the npc visible_flag");
        if (!containsFlag(until, p.value("hidden_flag", ""))) out.push_back(id + " until lacks the npc hidden_flag");
    }
    if (seen.size() != 13) out.push_back("path actions " + std::to_string(seen.size()) + " of 13");
    // 八个节点挂点 NPC 0 条（16.3 首段：免得 E 键与对话抢同一个 NPC）。
    for (const auto& hook : kNpcHooks)
        for (const auto& entry : file.at("entries"))
            if (entry.value("npc", "") == hook.object) out.push_back(std::string(hook.object) + " is a node NPC and carries a path action");
    return out;
}

TEST(Ch13Ledger, TheThirteenInquiriesFollowTheirNpcsAndRevealNothing) {
    const auto found = pathErrors(json(root() / "data" / "pathactions" / "ch13.json"), shippedMaps());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch13Ledger, MutationsOfThePathActionsAreCaught) {
    const Json original = json(root() / "data" / "pathactions" / "ch13.json");
    const auto maps = shippedMaps();
    ASSERT_TRUE(pathErrors(original, maps).empty());
    const auto index = [&](const char* id) {
        for (std::size_t i = 0; i < original.at("entries").size(); ++i)
            if (original.at("entries")[i].at("id") == id) return i;
        return original.at("entries").size();
    };
    for (int variant = 0; variant < 9; ++variant) {
        Json f = original;
        auto& entries = f["entries"];
        switch (variant) {
        case 0: entries[index("liuhufa_shaozhu_dating")]["reveal"] = Json::array({{{"roleId", "nixing_dahan"}, {"element", "木"}}}); break;
        case 1: entries[index("xinming_xinwu_dating")]["realm"] = 1; break;
        case 2: entries[index("guyu_shanggu_dating")]["until"].erase(std::size_t{0}); break;
        case 3: entries[index("yijing_waihai_dating")]["npc"] = "npc_yi_jing_tx"; break;
        case 4: entries[index("yuhufa_chuansong_dating")]["when"] = flagAtLeast("ch13.yinjian"); break;
        case 5: entries.erase(index("zhangxiushi_jiama_dating")); break;
        case 6: entries[index("huangmingli_mishi_dating")]["kind"] = "challenge"; break;
        case 7: entries[index("sunshishu_shouchao_dating")]["give"] = Json::array({{{"item", "material_lingshi"}, {"count", 1}}}); break;
        case 8: entries[index("xuyun_qiyuan_dating")]["npc"] = "npc_ming_furen"; break;    // 挂到节点 NPC 上
        }
        EXPECT_FALSE(pathErrors(f, maps).empty()) << "mutation=" << variant;
    }
    auto moved = maps;
    for (auto& layer : moved["ch13_waihai"]["layers"])
        if (layer.value("type", "") == "objectgroup")
            for (auto& o : layer["objects"])
                if (o.at("name") == "npc_gu_yu")
                    for (auto& p : o["properties"]) if (p.at("name") == "hidden_flag") p["value"] = "ch13.gongsun";
    EXPECT_FALSE(pathErrors(original, moved).empty()) << "NPC 撤场旗标与 until 对不上";
}

// ---- 第 23 条：第 13 节第 9、10 条与 12.2 末行里能按字扫的 0 处词（全章文案逐条扫值）----
// n29 改编尺度（第 9 条）：双修、炉鼎、处子、稚女；交换会（第 10 条）与 12.2 末行：灭族、丧夫、物价、困妖。比原文的场景口径更严
//（不分是哪一段），日后真有合法用法再收窄。Ch13Lexicon 另有 12.2 全表；这里是第 13 节的回归守卫，口径相同（data/text/ch13*.json）。
const std::vector<const char*>& sectionThirteenZeroWords() {
    static const std::vector<const char*> kWords{"双修", "炉鼎", "处子", "稚女", "灭族", "丧夫", "物价", "困妖"};
    return kWords;
}

using TextEntries = std::vector<std::pair<std::string, std::string>>;

TextEntries chapterTexts(int& files) {
    TextEntries out;
    files = 0;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text")) {
        const std::string name = file.path().filename().string();
        if (file.path().extension() != ".json" || name.rfind("ch13", 0) != 0) continue;
        ++files;
        const Json text = json(file.path());
        for (auto it = text.begin(); it != text.end(); ++it)
            out.emplace_back(it.key(), it.value().is_string() ? it.value().get<std::string>() : it.value().dump());
    }
    return out;
}

Errors zeroWordErrors(const TextEntries& entries) {
    Errors out;
    for (const auto& [key, text] : entries)
        for (const char* word : sectionThirteenZeroWords())
            if (text.find(word) != std::string::npos) out.push_back(std::string(word) + " in " + key);
    return out;
}

TEST(Ch13Ledger, TheSectionThirteenZeroWordsAreSaidNowhereInTheChapter) {
    int files = 0;
    const TextEntries entries = chapterTexts(files);
    ASSERT_GT(entries.size(), 250u) << "先验：读得到第 13 章文案";
    ASSERT_GE(files, 11) << "先验：十一份 ch13*.json";
    const auto found = zeroWordErrors(entries);
    EXPECT_TRUE(found.empty()) << joined(found);
    for (const char* word : sectionThirteenZeroWords()) {
        TextEntries bad = entries;
        bad.back().second += word;
        EXPECT_EQ(zeroWordErrors(bad), Errors{std::string(word) + " in " + bad.back().first}) << word;
    }
}
}  // namespace
