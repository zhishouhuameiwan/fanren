// 第 12 章账（只做静态部分：扫脚本、读物品 / 任务 / 路径行动）。对应 docs/ch12-design.md 第 15 节验收与第 9 节「账要对得上的几条」：
//   第 5 条  realm.advance / cap / demote 本章各 0 处（扫脚本那一半）。
//   第 6 条  party.remove("qu_hun_shadan") 恰 1 处、在 duoshe.lua、看返回值；party.add 0 处；qu_hun_shadan.json 与第 11 章
//            集成版逐字节相同（扫脚本那一半）。
//   第 8 条  lidao.lua 在 advance_days / flag.set("ch12.lidao") / teleport 之前判锦帕且判不过就 return；rudian.lua 0 处判锦帕；
//            全仓 take("story_jinpa_ditu") 0 处（扫脚本那一半）。
//   第 9 条  rongyan.lua 恰一处 flag.set("ch12.binghuo_ri", today())、先于固定那一日；两处抉择常速一支 advance_days(1)、
//            罗烟步 0 日；tihun.lua 读差值、绕路不拨日（全文只有战后一日）。
//   第 10 条 quba.lua 写 quba_xian 1 或 2、两件都 give；n24、n32 灵石三支（低阶 4 / 中阶 1 / advance_days(1)），钱不够不 return。
//   第 11 条 ch12.xinjing 只写位 1、2。
//   第 12 条 神雷 give 2 恰 2 处（战前）、take 余数在 battle 之后、在 `if n > 0 then` 分支里、看返回值（契约第 4 节）；
//            物品 talisman / castMagic / tradeable false / price 0；magic.learn 神雷全仓 0 处；magic.learn 噬金虫群恰 1 处
//            （guairen.lua，看返回值）。
//   第 13 条 0 店；take 灵石只在 quba、canhun；第 9 节 T1–T6（story take 恰 2 处、炼晶、灵草、0 张新配方）。
//   第 18 条 两条支线谓词与 N.3 字面量；第 19 条 路径行动 13 条（16.3）。
// 每条判据都有「在内存里改坏 → 必须报」的负向。切片那一半归测试路 B。
#include <gtest/gtest.h>
#include "Ch12Data.h"

namespace {
using namespace fanren::test::ch12;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

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

// ---- 第 5、6、8、9、10、11、12、13 条：扫本章脚本 ----
Errors scriptErrors(const Sources& ch12, const Sources& everywhere) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    const auto total = [&](const Sources& sources, const std::string& pattern) {
        int n = 0;
        for (const auto& [name, source] : sources) n += count(source, pattern);
        return n;
    };
    const auto onlyIn = [&](const std::string& pattern, const std::set<std::string>& owners, int each) {
        for (const auto& [name, source] : ch12) {
            const int n = count(source, pattern);
            if (owners.count(name) ? n != each : n != 0) out.push_back(pattern + " occurs " + std::to_string(n) + " times in " + name);
        }
    };
    // 第 5 条
    need(total(ch12, R"(\brealm\.(advance|cap|demote)\w*\s*\()") == 0, "realm advance/cap/demote in chapter 12");
    // 第 6 条
    onlyIn(R"(\bparty\.remove\s*\()", {"duoshe"}, 1);
    need(std::regex_search(code(ch12, "duoshe"), std::regex(R"(local\s+(\w+)\s*=\s*party\.remove\("qu_hun_shadan"\)[\s\S]*\bif\s+not\s+\1\b)")),
         "duoshe.lua does not look at party.remove's result");
    need(total(ch12, R"(\bparty\.add\s*\()") == 0, "party.add in chapter 12");
    // 第 8 条：锦帕门闸在拨日、置旗、传送之前，判不过即 return。at() 找不到时返回 size()，所以先断四处都在、
    // 再比先后（测试审查 T-4：删掉后三者任一，只比先后会照样通过）。
    const std::string lidao = code(ch12, "lidao");
    const std::size_t gate = at(lidao, "item.count(\"story_jinpa_ditu\")");
    const std::size_t days = at(lidao, "advance_days(");
    const std::size_t stamp = at(lidao, "flag.set(\"ch12.lidao\")");
    const std::size_t hop = at(lidao, "teleport(");
    need(gate < lidao.size() && days < lidao.size() && stamp < lidao.size() && hop < lidao.size(),
         "lidao.lua lacks the map gate, advance_days, flag.set(\"ch12.lidao\") or teleport");
    need(gate < days && gate < stamp && gate < hop, "lidao.lua does not gate on the map first");
    need(std::regex_search(lidao, std::regex(R"(if\s+item\.count\("story_jinpa_ditu"\)\s*<\s*1\s+then[^\n]*\breturn\b)")),
         "lidao.lua gate does not return when the map is missing");
    need(count(code(ch12, "rudian"), R"(story_jinpa_ditu)") == 0, "rudian.lua checks the map again");
    need(total(everywhere, R"(\btake\s*\(\s*"story_jinpa_ditu")") == 0, "someone takes the map");
    // 第 9 条
    onlyIn(R"(flag\.set\("ch12\.binghuo_ri")", {"rongyan"}, 1);
    need(count(code(ch12, "rongyan"), R"((?:^|\n)flag\.set\("ch12\.binghuo_ri",\s*today\(\)\))") == 1, "binghuo_ri is not today()");
    need(count(code(ch12, "rongyan"), R"(if\s+(\w+)\s*==\s*2\s+then\s*\n(?:\s*talk\([^\n]*\n)*\s*advance_days\(1\))") == 2,
         "rongyan.lua: the two steady-pace branches do not each spend one day");
    need(count(code(ch12, "rongyan"), R"(\bchoice\s*\()") == 2, "rongyan.lua choice count");
    // 3.2 n20：先记 binghuo_ri、再固定拨 1 日（顶格一句）；两处罗烟步 0 日——全文 advance_days 恰 1 ＋ 2 = 3 句。
    {
        const std::string rongyan = code(ch12, "rongyan");
        need(at(rongyan, "flag.set(\"ch12.binghuo_ri\"") < at(rongyan, "advance_days("), "rongyan.lua spends a day before it stamps binghuo_ri");
        need(count(rongyan, R"((?:^|\n)advance_days\(1\))") == 1, "rongyan.lua: the fixed day is not one top-level line");
        need(count(rongyan, R"(\badvance_days\s*\()") == 3, "rongyan.lua: the Luoyan steps are not free of days");
    }
    need(code(ch12, "tihun").find("today() - flag.get(\"ch12.binghuo_ri\")") != std::string::npos, "tihun.lua does not read the elapsed days");
    // 3.2 n21：绕路一支只一句、不拨日；全文只有战后那一句 advance_days(1)。
    {
        const std::string tihun = code(ch12, "tihun");
        need(count(tihun, R"(\badvance_days\s*\()") == 1 && at(tihun, "advance_days(1)") > at(tihun, "battle("),
             "tihun.lua spends days outside the one after battle 2");
    }
    // 第 10 条
    onlyIn(R"(flag\.set\("ch12\.quba_xian",\s*1\))", {"quba"}, 1);
    onlyIn(R"(flag\.set\("ch12\.quba_xian",\s*2\))", {"quba"}, 1);
    need(count(code(ch12, "quba"), R"(\bgive\s*\()") == 2 &&
         count(code(ch12, "quba"), R"((?:^|\n)give\("story_xuese_pifeng", 1\))") == 1 &&
         count(code(ch12, "quba"), R"((?:^|\n)give\("story_wuse_tonghuan", 1\))") == 1, "quba.lua does not give both, unconditionally");
    // 第 10、13 条与第 9 节 T1：两处阵法的灵石三支；钱不够也不 return。
    const std::regex fee(R"((?:^|\n)if take\("material_lingshi", 4\) then\n[\s\S]*?\nelseif take\("material_lingshi_zhong", 1\) then\n[\s\S]*?\nelse\n(?:\s*talk\([^\n]*\n)*\s*advance_days\(1\)\n(?:\s*talk\([^\n]*\n)*end\n)");
    for (const char* name : {"quba", "canhun"}) {
        std::smatch m;
        const std::string text = code(ch12, name);
        need(std::regex_search(text, m, fee), std::string(name) + ".lua lacks the low / mid / one-day fee chain");
        if (!m.empty()) need(m.str().find("return") == std::string::npos, std::string(name) + ".lua returns when short of stones");
    }
    onlyIn(R"(\btake\s*\(\s*"material_lingshi",\s*4\))", {"quba", "canhun"}, 1);
    onlyIn(R"(\btake\s*\(\s*"material_lingshi_zhong",\s*1\))", {"quba", "canhun"}, 1);
    onlyIn(R"(\btake\s*\(\s*"material_lingshi)", {"quba", "canhun"}, 2);
    need(total(ch12, R"(\bgive\s*\(\s*"material_lingshi)") == 0, "chapter 12 gives spirit stones");
    // 第 11 条：ch12.xinjing 只由 huanjing.lua 写、只拼位 1 与位 2。
    onlyIn(R"(flag\.set\("ch12\.xinjing")", {"huanjing"}, 1);
    {
        const std::string text = code(ch12, "huanjing");
        std::smatch m;
        std::set<int> bits;
        if (std::regex_search(text, m, std::regex(R"(flag\.set\("ch12\.xinjing",\s*(\w+)\))"))) {
            const std::string var = m[1].str();
            const std::regex add("\\b" + var + "\\s*=\\s*" + var + "\\s*\\+\\s*(\\d+)");
            for (std::sregex_iterator it(text.begin(), text.end(), add), end; it != end; ++it) bits.insert(std::stoi((*it)[1].str()));
            need(count(text, "\\b" + var + "\\s*=") == 3, "xinjing is assigned outside the two bits");
        }
        need(bits == std::set<int>{1, 2}, "xinjing bits are not exactly 1 and 2");
    }
    // 第 12 条：神雷两处，战前给、战后看余数收。
    onlyIn(R"(\bgive\s*\(\s*"talisman_bixie_shenlei",\s*2\))", {"guiwang", "xuangu"}, 1);
    onlyIn(R"(\btake\s*\(\s*"talisman_bixie_shenlei")", {"guiwang", "xuangu"}, 1);
    for (const char* name : {"guiwang", "xuangu"}) {
        const std::string text = code(ch12, name);
        const std::size_t fight = at(text, "battle(");
        need(at(text, "give(\"talisman_bixie_shenlei\"") < fight, std::string(name) + " gives the thunder after the battle");
        // 契约第 4 节：take(id, 0) 会被引擎当成扣 1，所以 take 必须紧挨在 `if n > 0 then` 的分支里。
        const std::regex rest(R"(local\s+(\w+)\s*=\s*item\.count\("talisman_bixie_shenlei"\)\s*\nif\s+\1\s*>\s*0\s+then\s*\n\s*if\s+take\("talisman_bixie_shenlei",\s*\1\))");
        std::smatch m;
        need(std::regex_search(text, m, rest) && static_cast<std::size_t>(m.position(0)) > fight,
             std::string(name) + " does not take the remainder after the battle, looking at take's result");
    }
    need(total(everywhere, R"(\bmagic\.learn\s*\(\s*"magic_bixie_shenlei")") == 0, "someone learns the thunder");
    need(total(everywhere, R"(\bmagic\.learn\s*\(\s*"magic_shijin_chongqun")") == 1, "shijin learn count across scripts");
    need(std::regex_search(code(ch12, "guairen"), std::regex(R"(local\s+(\w+)\s*=\s*magic\.learn\("magic_shijin_chongqun"\)[\s\S]*\bif\s+\1\b)")),
         "guairen.lua does not learn the swarm and look at the result");
    // 第 9 节 T3、T4、T6
    onlyIn(R"(\btake\s*\(\s*"story_)", {"dengdao", "duoshe"}, 1);
    need(std::regex_search(code(ch12, "dengdao"), std::regex(R"(\bif\s+take\("story_bai_zhenzhu",\s*1\))")), "dengdao.lua pearl take");
    need(std::regex_search(code(ch12, "duoshe"), std::regex(R"(\bif\s+take\("story_zhiyang_huazhou",\s*1\))")), "duoshe.lua scroll take");
    for (const char* kept : {"story_jinpa_ditu", "story_yinhun_zhong", "story_xueyu_zhizhu", "story_shijin_chong"})
        need(total(ch12, std::string(R"(\btake\s*\(\s*")") + kept) == 0, std::string("chapter 12 takes ") + kept);
    onlyIn(R"(\bgive\s*\(\s*"material_lianjing",\s*1\))", {"lianjing"}, 1);
    onlyIn(R"(\bgive\s*\(\s*"herb_)", {"caiyao"}, 1);
    need(count(code(ch12, "caiyao"), R"(\bgive\s*\(\s*"herb_\w+",\s*2\b)") == 1, "caiyao.lua gives two herbs");
    need(total(ch12, R"(\bshop\s*\()") == 0, "chapter 12 opens a shop");
    return out;
}
TEST(Ch12Ledger, TheChapterScriptsKeepTheLedgerTheDesignWrote) {
    const auto found = scriptErrors(scripts(), allScripts());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch12Ledger, MutationsOfTheScriptLedgerAreCaught) {
    const Sources original = scripts();
    const Sources everywhere = allScripts();
    ASSERT_TRUE(scriptErrors(original, everywhere).empty()) << joined(scriptErrors(original, everywhere));
    struct Bad { const char* script; const char* from; const char* to; };   // from 为空 = 末尾追加 to
    const std::vector<Bad> bads{
        {"butian", "", "realm.advance()\n"},
        {"lingquan", "", "party.add(\"qu_hun_shadan\")\n"},
        {"duoshe", "local ok = party.remove(\"qu_hun_shadan\")", "party.remove(\"qu_hun_shadan\")\nlocal ok = true"},
        {"lidao", "if item.count(\"story_jinpa_ditu\") < 1 then", "advance_days(1)\nif item.count(\"story_jinpa_ditu\") < 1 then"},
        {"lidao", "talk(\"\", \"ch12.lidao.no_map\"); return end", "talk(\"\", \"ch12.lidao.no_map\") end"},
        {"lidao", "teleport(\"ch12_dadian\")", ""},
        {"lidao", "advance_days(4)", ""},
        {"lidao", "flag.set(\"ch12.lidao\")", ""},
        {"rudian", "", "if item.count(\"story_jinpa_ditu\") < 1 then return end\n"},
        {"rongyan", "flag.set(\"ch12.binghuo_ri\", today())", "flag.set(\"ch12.binghuo_ri\", 1)"},
        {"rongyan", "flag.set(\"ch12.binghuo_ri\", today())\nadvance_days(1)", "advance_days(1)\nflag.set(\"ch12.binghuo_ri\", today())"},
        {"rongyan", "talk(\"\", \"ch12.rongyan.luo1\")", "talk(\"\", \"ch12.rongyan.luo1\")\n    advance_days(1)"},
        {"tihun", "", "flag.set(\"ch12.binghuo_ri\", today())\n"},
        {"tihun", "talk(\"\", \"ch12.tihun.round\")", "talk(\"\", \"ch12.tihun.round\")\n    advance_days(1)"},
        {"guiwang", "if n > 0 then\n    if take", "if n >= 0 then\n    if take"},
        {"quba", "flag.set(\"ch12.quba_xian\", 2)", "flag.set(\"ch12.quba_xian\", 3)"},
        {"quba", "give(\"story_wuse_tonghuan\", 1)", "if pick == 2 then give(\"story_wuse_tonghuan\", 1) end"},
        {"canhun", "    advance_days(1)", "    return"},
        {"butian", "", "if take(\"material_lingshi\", 4) then end\n"},
        {"huanjing", "bits = bits + 2", "bits = bits + 4"},
        {"guiwang", "give(\"talisman_bixie_shenlei\", 2)", "give(\"talisman_bixie_shenlei\", 3)"},
        {"xuangu", "if take(\"talisman_bixie_shenlei\", n) then", "take(\"talisman_bixie_shenlei\", n)\nif true then"},
        {"guairen", "local learned = magic.learn(\"magic_shijin_chongqun\")", "magic.learn(\"magic_shijin_chongqun\")\nlocal learned = true"},
        {"butian", "", "take(\"story_yinhun_zhong\", 1)\n"},
        {"caiyao", "give(\"herb_huangjing_zhi\", 2, 300)", "give(\"herb_huangjing_zhi\", 3, 300)"},
        {"lingquan", "", "shop(\"ch11_fangshi_zahuo\")\n"},
        {"lingquan", "", "give(\"material_lingshi\", 10)\n"},
    };
    for (const auto& bad : bads) {
        Sources sources = original;
        std::string& text = sources[bad.script];
        if (*bad.from == '\0') {
            text += std::string("\n") + bad.to;
        } else {
            const auto found = text.find(bad.from);
            ASSERT_NE(found, std::string::npos) << bad.script << ": " << bad.from;
            text.replace(found, std::string(bad.from).size(), bad.to);
        }
        EXPECT_FALSE(scriptErrors(sources, everywhere).empty()) << bad.script << " <- " << bad.to;
    }
    Sources elsewhere = everywhere;
    elsewhere["ch13/probe.lua"] = "take(\"story_jinpa_ditu\", 1)\n";
    EXPECT_FALSE(scriptErrors(original, elsewhere).empty()) << "全仓口径：别章 take 锦帕";
    Sources thunder = everywhere;
    thunder["ch12/lingquan.lua"] += "\nmagic.learn(\"magic_bixie_shenlei\")\n";
    EXPECT_FALSE(scriptErrors(original, thunder).empty()) << "全仓口径：学神雷";
    Sources twice = everywhere;
    twice["ch13/probe.lua"] = "magic.learn(\"magic_shijin_chongqun\")\n";
    EXPECT_FALSE(scriptErrors(original, twice).empty()) << "全仓口径：噬金虫群学两处";
}

// ---- 第 6 条后半：曲魂与第 11 章终点逐字节相同 ----
// 第 11 章集成版 data/roles/qu_hun_shadan.json（HEAD 6ec8073）：487 字节，FNV-1a 64 = 0x1542439b30c344d6。
constexpr std::size_t kQuHunBytes = 487;
constexpr std::uint64_t kQuHunHash = 0x1542439b30c344d6ull;

Errors quHunErrors(const std::string& bytes) {
    Errors out;
    if (bytes.size() != kQuHunBytes) out.push_back("qu_hun_shadan.json is " + std::to_string(bytes.size()) + " bytes");
    if (fnv1a(bytes) != kQuHunHash) out.push_back("qu_hun_shadan.json differs from the chapter-11 bytes");
    return out;
}

TEST(Ch12Ledger, TheDepartedCompanionsRoleIsByteForByteChapterEleven) {
    const std::string bytes = read(root() / "data" / "roles" / "qu_hun_shadan.json");
    const auto found = quHunErrors(bytes);
    EXPECT_TRUE(found.empty()) << joined(found);
    // 负向（测试审查 T-5）：改过的字节喂给同一个检查函数——同长改一个字节只报哈希，多一个字节两条都报。
    auto edited = bytes;
    edited[edited.size() / 2] = edited[edited.size() / 2] == '1' ? '2' : '1';
    EXPECT_EQ(quHunErrors(edited), Errors{"qu_hun_shadan.json differs from the chapter-11 bytes"});
    EXPECT_EQ(quHunErrors(bytes + "\n").size(), 2u);
}

// ---- 第 12、13 条：神雷物品与 0 店 ----
// 测试审查 T-2：神雷那门法术的五行恰木（element 2，core::Element 的位）——三首鬼物与玄骨的 killable_by 都含木，
// 改成别的五行，④ 拿神雷收尾就会被锁在 1；施工图 8.2 ①④、第 7 节「神雷两发收尾」。
Errors talismanErrors(const Json& item, const Json& spell, const std::vector<std::string>& shopFiles) {
    Errors out;
    if (item.value("id", "") != "talisman_bixie_shenlei" || item.value("kind", "") != "talisman") out.push_back("thunder kind");
    if (item.value("castMagic", "") != "magic_bixie_shenlei") out.push_back("thunder castMagic");
    if (spell.value("id", "") != "magic_bixie_shenlei" || spell.value("element", -1) != 2) out.push_back("thunder element is not wood");
    if (item.value("tradeable", true) != false || item.value("price", -1) != 0) out.push_back("thunder tradeable/price");
    for (const auto& name : shopFiles)
        if (name.rfind("ch12", 0) == 0) out.push_back("chapter 12 shop file " + name);
    return out;
}

std::vector<std::string> shopFiles() {
    std::vector<std::string> names;
    for (const auto& file : fs::directory_iterator(root() / "data" / "shops")) names.push_back(file.path().filename().string());
    return names;
}

Json thunderSpell() { return json(root() / "data" / "magics" / "bixie_shenlei.json"); }

TEST(Ch12Ledger, TheThunderIsAnUntradeableWoodTalismanAndThereIsNoShop) {
    const Json item = json(root() / "data" / "items" / "talismans" / "bixie_shenlei.json");
    const auto found = talismanErrors(item, thunderSpell(), shopFiles());
    EXPECT_TRUE(found.empty()) << joined(found);
    ASSERT_GT(shopFiles().size(), 10u) << "先验：读得到 data/shops";
}

TEST(Ch12Ledger, MutationsOfTheThunderItemItsSpellAndAShopFileAreCaught) {
    const Json item = json(root() / "data" / "items" / "talismans" / "bixie_shenlei.json");
    const Json spell = thunderSpell();
    const auto shops = shopFiles();
    ASSERT_TRUE(talismanErrors(item, spell, shops).empty());
    for (int variant = 0; variant < 7; ++variant) {
        Json bad = item;
        Json badSpell = spell;
        auto files = shops;
        switch (variant) {
        case 0: bad["kind"] = "consumable"; break;
        case 1: bad["castMagic"] = "magic_qinghuo_lei"; break;
        case 2: bad["tradeable"] = true; break;
        case 3: files.push_back("ch12_xutian_zahuo.json"); break;
        case 4: bad["price"] = 40; break;
        case 5: badSpell["element"] = 8; break;     // 改成火 → 必报
        case 6: badSpell.erase("element"); break;   // 不写五行 → 必报
        }
        EXPECT_FALSE(talismanErrors(bad, badSpell, files).empty()) << variant;
    }
}

// ---- 第 9 节 T6、16.1 第 31 条：本章 0 张新配方（九曲灵参不建配方；炼晶的配方留第 13 章定）----
Errors recipeErrors(const std::vector<Json>& recipes) {
    Errors out;
    const std::set<std::string> ours(kNewItems.begin(), kNewItems.end());
    for (const auto& recipe : recipes) {
        const std::string id = recipe.value("id", "");
        const auto flag = recipe.find("requireFlag");
        if (flag != recipe.end() && flag->is_string() && flag->get<std::string>().rfind("ch12.", 0) == 0)
            out.push_back(id + " unlocks on a chapter-12 flag");
        if (ours.count(recipe.value("productId", ""))) out.push_back(id + " makes a chapter-12 item");
        for (const auto& input : recipe.value("inputs", Json::array()))
            if (ours.count(input.value("itemId", ""))) out.push_back(id + " consumes " + input.value("itemId", ""));
    }
    return out;
}

std::vector<Json> recipeFiles() {
    std::vector<Json> out;
    for (const auto& file : fs::recursive_directory_iterator(root() / "data" / "recipes"))
        if (file.path().extension() == ".json") out.push_back(json(file.path()));
    return out;
}

TEST(Ch12Ledger, NoRecipeUsesOrMakesAChapterTwelveItem) {
    const auto recipes = recipeFiles();
    ASSERT_GT(recipes.size(), 15u) << "先验：读得到 data/recipes";
    const auto found = recipeErrors(recipes);
    EXPECT_TRUE(found.empty()) << joined(found);
    const std::vector<Json> bads{
        {{"id", "recipe_probe_a"}, {"productId", "pill_probe"}, {"inputs", Json::array({{{"itemId", "story_jiuqu_lingshen"}, {"count", 1}}})}},
        {{"id", "recipe_probe_b"}, {"productId", "story_xutian_ding"}, {"inputs", Json::array()}},
        {{"id", "recipe_probe_c"}, {"productId", "pill_probe"}, {"inputs", Json::array()}, {"requireFlag", "ch12.lingshen"}},
    };
    for (const auto& bad : bads) {
        auto more = recipes;
        more.push_back(bad);
        EXPECT_FALSE(recipeErrors(more).empty()) << bad.at("id");
    }
}
// ---- 第 18 条：两条支线（笔记 N.3 字面量）----
Json flagAtLeast(const char* flag, int value) { return Json::array({{{"flag", flag}, {"op", ">="}, {"value", value}}}); }

Errors questErrors(const Json& z1, const Json& z2) {
    Errors out;
    const auto need = [&](bool ok, const char* why) { if (!ok) out.push_back(why); };
    need(z1.value("id", "") == "q12_maigu" && z1.value("chapter", 0) == 12, "z1 id/chapter");
    need(z1.value("accept", Json()) == flagAtLeast("ch12.tupo", 1), "z1 accept");
    need(z1.value("complete", Json()) == flagAtLeast("ch12.z1", 1), "z1 complete");
    need(z1.value("fail", Json()) == flagAtLeast("ch12.guiwang", 1), "z1 fail");
    need(z1.value("origin", "") == "原著", "z1 origin");
    need(z2.value("id", "") == "q12_kuilei_canhai" && z2.value("chapter", 0) == 12, "z2 id/chapter");
    need(z2.value("accept", Json()) == flagAtLeast("ch12.neidian", 1), "z2 accept");
    need(z2.value("complete", Json()) == flagAtLeast("ch12.z2", 1), "z2 complete");
    need(z2.value("fail", Json()) == flagAtLeast("ch12.cengjie", 1), "z2 fail");
    need(z2.value("origin", "") == "原著", "z2 origin");
    return out;
}

TEST(Ch12Ledger, BothSideQuestsMatchTheirPredicates) {
    const auto found = questErrors(json(root() / "data" / "quests" / "q12_maigu.json"),
                                   json(root() / "data" / "quests" / "q12_kuilei_canhai.json"));
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch12Ledger, MutationsOfTheSideQuestsAreCaught) {
    const Json z1 = json(root() / "data" / "quests" / "q12_maigu.json");
    const Json z2 = json(root() / "data" / "quests" / "q12_kuilei_canhai.json");
    ASSERT_TRUE(questErrors(z1, z2).empty());
    for (int variant = 0; variant < 6; ++variant) {
        Json a = z1, b = z2;
        switch (variant) {
        case 0: a.erase("fail"); break;
        case 1: a["origin"] = "改编"; break;
        case 2: a["accept"] = flagAtLeast("ch12.chuansong", 1); break;
        case 3: b["fail"] = flagAtLeast("ch12.juanzhou", 1); break;
        case 4: b["complete"] = flagAtLeast("ch12.z2", 2); break;
        case 5: b["accept"] = flagAtLeast("ch12.huanjing", 1); break;
        }
        EXPECT_FALSE(questErrors(a, b).empty()) << "mutation=" << variant;
    }
}

// ---- 第 19 条：路径行动 13 条（16.3 表；16.1 第 41 条：都不设 reveal）----
struct PathRow {
    const char* id;
    const char* map;
    const char* npc;
    std::vector<const char*> when;
    std::vector<const char*> until;   // 另含 ch12.done
};

const std::vector<PathRow>& pathRows() {
    static const std::vector<PathRow> kRows{
        {"jinqing_yizhi_dating", "ch12_tushan", "npc_jin_qing", {"ch12.dengdao"}, {"ch12.pozhen", "ch12.tuizhu"}},
        {"shidie_hongyue_dating", "ch12_tushan", "npc_shi_die", {"ch12.dengdao"}, {"ch12.tuizhu"}},
        {"huyue_sanxiu_dating", "ch12_tushan", "npc_hu_yue", {"ch12.dengdao"}, {"ch12.tuizhu"}},
        {"wenqiang_kuixing_dating", "ch12_tushan", "npc_wen_qiang", {"ch12.canyuan"}, {"ch12.lidao"}},
        {"xuangu_jinzhi_dating", "ch12_dadian", "npc_xuan_gu", {"ch12.rudian", "ch12.jiumeng"}, {"ch12.chuansong"}},
        {"ziling_canyuan_dating", "ch12_dadian", "npc_zi_ling", {"ch12.rudian"}, {"ch12.chuansong"}},
        {"zhifa_guiju_dating", "ch12_dadian", "npc_zhifa_ci", {"ch12.qunxiang"}, {"ch12.chuansong"}},
        {"ziling_guiwang_dating", "ch12_guiwu", "npc_zi_ling", {"ch12.tupo"}, {"ch12.guiwang"}},
        {"geli_guiwu_dating", "ch12_guiwu", "npc_ge_li", {"ch12.tupo"}, {"ch12.guiwang"}},
        {"xuangu_lingshen_dating", "ch12_xiaojie", "npc_xuan_gu", {"ch12.xiaojie"}, {"ch12.lingshen"}},
        {"qingyi_binghuo_dating", "ch12_xiaojie", "npc_qingyi_xg", {"ch12.baishi"}, {"ch12.xiagu"}},
        {"wuchou_canyuan_dating", "ch12_taqian", "npc_wu_chou_tq", {"ch12.huanjing"}, {"ch12.neidian"}},
        {"qingyi_qianlan_dating", "ch12_hanlitai", "npc_qingyi_hl", {"ch12.cengjie"}, {"ch12.ruyi", "ch12.butian"}},
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
        seen.insert(id);
        const PathRow& r = *row->second;
        if (entry.value("kind", "") != "inquire" || entry.value("realm", -1) != 0) out.push_back(id + " kind/realm");
        if (id.size() < 7 || id.compare(id.size() - 7, 7, "_dating") != 0) out.push_back(id + " id suffix");
        if (entry.contains("reveal")) out.push_back(id + " carries reveal (16.1 #41)");
        if (entry.value("map", "") != r.map || entry.value("npc", "") != r.npc) out.push_back(id + " map/npc");
        if (entry.value("done_flag", "") != "ch12.path." + id) out.push_back(id + " done_flag");
        const Json when = entry.value("when", Json::array()), until = entry.value("until", Json::array());
        if (when.size() != r.when.size()) out.push_back(id + " when size");
        for (const char* flag : r.when) if (!containsFlag(when, flag)) out.push_back(id + " when lacks " + flag);
        if (until.size() != r.until.size() + 1 || !containsFlag(until, "ch12.done")) out.push_back(id + " until size / ch12.done");
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
    return out;
}

std::map<std::string, Json> chapterMaps() {
    std::map<std::string, Json> maps;
    for (const char* id : kMaps) maps[id] = json(root() / "maps" / (std::string(id) + ".tmj"));
    return maps;
}

TEST(Ch12Ledger, TheThirteenInquiriesFollowTheirNpcsAndRevealNothing) {
    const auto found = pathErrors(json(root() / "data" / "pathactions" / "ch12.json"), chapterMaps());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch12Ledger, MutationsOfThePathActionsAreCaught) {
    const Json original = json(root() / "data" / "pathactions" / "ch12.json");
    const auto maps = chapterMaps();
    ASSERT_TRUE(pathErrors(original, maps).empty());
    const auto index = [&](const char* id) {
        for (std::size_t i = 0; i < original.at("entries").size(); ++i)
            if (original.at("entries")[i].at("id") == id) return i;
        return original.at("entries").size();
    };
    for (int variant = 0; variant < 7; ++variant) {
        Json f = original;
        auto& entries = f["entries"];
        switch (variant) {
        case 0: entries[index("ziling_guiwang_dating")]["reveal"] = Json::array({{{"roleId", "gui_yecha"}, {"element", "木"}}}); break;
        case 1: entries[index("geli_guiwu_dating")]["realm"] = 1; break;
        case 2: entries[index("qingyi_qianlan_dating")]["until"].erase(std::size_t{0}); break;
        case 3: entries[index("wuchou_canyuan_dating")]["npc"] = "npc_wu_chou"; break;
        case 4: entries[index("xuangu_jinzhi_dating")]["when"] = flagAtLeast("ch12.rudian", 1); break;
        case 5: entries.erase(index("huyue_sanxiu_dating")); break;
        case 6: entries[index("zhifa_guiju_dating")]["kind"] = "purchase"; break;
        }
        EXPECT_FALSE(pathErrors(f, maps).empty()) << "mutation=" << variant;
    }
    auto moved = maps;
    for (auto& layer : moved["ch12_xiaojie"]["layers"])
        if (layer.value("type", "") == "objectgroup")
            for (auto& o : layer["objects"])
                if (o.at("name") == "npc_qingyi_xg")
                    for (auto& p : o["properties"]) if (p.at("name") == "hidden_flag") p["value"] = "ch12.rongyan";
    EXPECT_FALSE(pathErrors(original, moved).empty()) << "NPC 撤场旗标与 until 对不上";
}

// ---- 第 13 节第 2、4、5、7、9 条里能按字扫的 0 处词（测试审查 T-8）----
// 名号里的「上人」（第 2 条、18.1 第 9 条）；「炉鼎」（第 4 条）；「以身相许」（第 5 条）；n26 / n31 的「碎尸」（第 7 条，
// 「血」「灰烬」扫不得：血色披风、玄骨化灰都合法）；「七十二」「化为乌有」（第 9 条）。全章文案逐条扫值、全章 0 处——
// 比原文的场景口径更严（「上人」不分是不是名号），日后真有合法用法再收窄。Ch12Lexicon 的 zeroWords() 归文案整改路，
// 这里另立一道回归守卫，口径相同（data/text/ch12*.json）。
const std::vector<const char*>& sectionThirteenZeroWords() {
    static const std::vector<const char*> kWords{"上人", "炉鼎", "以身相许", "碎尸", "七十二", "化为乌有"};
    return kWords;
}

using TextEntries = std::vector<std::pair<std::string, std::string>>;

TextEntries chapterTexts(int& files) {
    TextEntries out;
    files = 0;
    for (const auto& file : fs::directory_iterator(root() / "data" / "text")) {
        const std::string name = file.path().filename().string();
        if (file.path().extension() != ".json" || name.rfind("ch12", 0) != 0) continue;
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

TEST(Ch12Ledger, TheSectionThirteenZeroWordsAreSaidNowhereInTheChapter) {
    int files = 0;
    const TextEntries entries = chapterTexts(files);
    ASSERT_GT(entries.size(), 250u) << "先验：读得到第 12 章文案";
    ASSERT_GE(files, 11) << "先验：十一份 ch12*.json";
    const auto found = zeroWordErrors(entries);
    EXPECT_TRUE(found.empty()) << joined(found);
    // 负向：每个词各塞进一条真文案的末尾 → 恰报那一个词、那一条 key。
    for (const char* word : sectionThirteenZeroWords()) {
        TextEntries bad = entries;
        bad.back().second += word;
        EXPECT_EQ(zeroWordErrors(bad), Errors{std::string(word) + " in " + bad.back().first}) << word;
    }
}
}  // namespace
