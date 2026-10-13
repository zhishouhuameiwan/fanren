// 第 11 章账与配方（只做静态部分：商店、物品、配方、任务、路径行动、脚本扫描）。对应 docs/ch11-design.md 第 15 节验收：
//   第 12 条 炼剑走面板：facility_lianqi_39 forge / grade 3 / require ch11.liugen；recipe_qingzhu_fengyun_jian 的形状
//            （productCount 1、金雷竹 6 ＋ 辅料 1、无灵石、difficulty 0、requiredProficiency 0、requireFlag ch11.liugen、
//            guaranteed，并经真加载器读出 guaranteed）；本章脚本 0 处 give 青竹蜂云剑；chujian.lua 先判剑在身上。
//   第 14 条 分身：本章 party.add / remove 0 处；qu_hun_shadan / wu_chou / zi_ling.json 与第 10 章集成版逐字节相同。
//   第 15 条 经济账（读 data/shops、data/items 那一半）：两家只收店的 sellRate 与空 entries；两家杂货的霓裳草补货四项；
//            坊市杂货的设施 require_flag；五级 / 六级妖丹在收购行卖 760 / 4800（真数据 × rules::sellPrice）；
//            金雷竹 tradeable false。
//   第 19 条 支线 Z1、Z2 的谓词与 N.3 字面量。
//   第 20 条 路径行动 14 条（id 按 18.2 第 1 条落地名）。
// 每条判据都有「在内存里改坏 → 必须报」的负向。
#include <gtest/gtest.h>
#include "Ch11Data.h"
#include "core/rules/Economy.h"
#include "io/DataLoader.h"
#include "io/RecipeLoader.h"
#include "io/ShopLoader.h"

namespace {
using namespace fanren::test::ch11;
using Errors = std::vector<std::string>;
using Sources = std::map<std::string, std::string>;

std::string joined(const Errors& errors) {
    std::string out;
    for (const auto& e : errors) out += "\n  " + e;
    return out;
}

Json facility(const char* map, const char* name) { return properties(objects(json(root() / "maps" / (std::string(map) + ".tmj"))).at(name)); }

// ---- 第 12 条 ----
Errors forgeErrors(const Json& recipe, const Json& forge, const Sources& sources) {
    Errors out;
    const auto need = [&](bool ok, const char* why) { if (!ok) out.push_back(why); };
    need(forge.value("kind", "") == "forge" && forge.value("grade", 0) == 3, "forge kind/grade");
    need(forge.value("require_flag", "") == "ch11.liugen", "forge require_flag");
    need(recipe.at("id") == "recipe_qingzhu_fengyun_jian" && recipe.at("kind") == "forge", "recipe id/kind");
    need(recipe.at("productId") == "weapon_qingzhu_fengyun_jian" && recipe.at("productCount") == 1, "recipe product");
    need(recipe.at("inputs") == Json::array({{{"itemId", "material_jinleizhu"}, {"count", 6}},
                                             {{"itemId", "material_lianjian_fuliao"}, {"count", 1}}}), "recipe inputs");
    need(recipe.at("difficulty") == 0 && recipe.at("requiredProficiency") == 0, "recipe difficulty/proficiency");
    need(recipe.at("requireFlag") == "ch11.liugen", "recipe requireFlag");
    need(recipe.value("guaranteed", false) == true, "recipe guaranteed");
    const std::regex grant(R"lua(\bgive\s*\(\s*"weapon_qingzhu_fengyun_jian")lua");
    for (const auto& [name, source] : sources)
        if (countMatches(executableLines(source), grant)) out.push_back(name + " gives the sword directly");
    const std::regex gate(R"lua((?:^|\n)if\s+item\.count\("weapon_qingzhu_fengyun_jian"\)\s*<\s*1\s+then\s+return\s+end)lua");
    const std::string chujian = executableLines(sources.count("chujian") ? sources.at("chujian") : "");
    need(std::regex_search(chujian, gate), "chujian.lua does not gate on the sword first");
    return out;
}

TEST(Ch11Ledger, TheSwordIsForgedThroughThePanelOnly) {
    const Json recipe = json(root() / "data" / "recipes" / "forge" / "qingzhu_fengyun_jian.json");
    const auto found = forgeErrors(recipe, facility("ch11_dongfu", "facility_lianqi_39"), scripts());
    EXPECT_TRUE(found.empty()) << joined(found);
    // 真加载器读出来的也是必成、有门闸（判据不只看 JSON 字面）。
    const auto loaded = fanren::io::loadRecipe((root() / "data" / "recipes" / "forge" / "qingzhu_fengyun_jian.json").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_TRUE(loaded.value.guaranteed);
    EXPECT_EQ(loaded.value.requireFlag, "ch11.liugen");
    ASSERT_EQ(loaded.value.inputs.size(), 2u);
    for (const auto& input : loaded.value.inputs) {
        EXPECT_NE(input.itemId, "material_lingshi");
        EXPECT_NE(input.itemId, "material_lingshi_zhong");
    }
}

TEST(Ch11Ledger, MutationsOfTheForgeRecipeAndSwordGrantsAreCaught) {
    const Json recipe = json(root() / "data" / "recipes" / "forge" / "qingzhu_fengyun_jian.json");
    const Json forge = facility("ch11_dongfu", "facility_lianqi_39");
    const Sources sources = scripts();
    ASSERT_TRUE(forgeErrors(recipe, forge, sources).empty());
    for (int variant = 0; variant < 8; ++variant) {
        Json r = recipe, f = forge;
        Sources s = sources;
        switch (variant) {
        case 0: r["guaranteed"] = false; break;
        case 1: r["inputs"].push_back({{"itemId", "material_lingshi"}, {"count", 50}}); break;
        case 2: r["inputs"][0]["count"] = 7; break;
        case 3: r["difficulty"] = 40; break;
        case 4: r["requireFlag"] = "ch11.jinlei"; break;
        case 5: f["require_flag"] = "ch11.anju"; break;
        case 6: s["chujian"] += "\ngive(\"weapon_qingzhu_fengyun_jian\", 1)\n"; break;
        case 7: {
            const auto at = s["chujian"].find("item.count(\"weapon_qingzhu_fengyun_jian\") < 1");
            ASSERT_NE(at, std::string::npos);
            s["chujian"].replace(at, 0, "false and ");
            break;
        }
        }
        EXPECT_FALSE(forgeErrors(r, f, s).empty()) << "mutation=" << variant;
    }
}

// ---- 第 14 条 ----
struct Pinned { const char* file; std::size_t bytes; std::uint64_t hash; };
// 第 10 章集成版（HEAD 68aa3f7）原文件的长度与 FNV-1a 64；第 11 章「一个字不动」。
constexpr Pinned kPinnedRoles[] = {
    {"qu_hun_shadan.json", 487, 0x1542439b30c344d6ull},
    {"wu_chou.json", 726, 0x55a3df0583774db6ull},
    {"zi_ling.json", 344, 0x5fe3163fc5adc262ull}};

Errors avatarErrors(const Sources& sources, const std::map<std::string, std::string>& roleBytes) {
    Errors out;
    const std::regex party(R"lua(\bparty\.(add|remove)\s*\()lua");
    for (const auto& [name, source] : sources)
        if (countMatches(executableLines(source), party)) out.push_back(name + " changes the party");
    for (const auto& pin : kPinnedRoles) {
        const std::string& bytes = roleBytes.at(pin.file);
        if (bytes.size() != pin.bytes || fnv1a(bytes) != pin.hash) out.push_back(std::string(pin.file) + " differs from chapter 10");
    }
    return out;
}

std::map<std::string, std::string> pinnedRoleBytes() {
    std::map<std::string, std::string> bytes;
    for (const auto& pin : kPinnedRoles) bytes[pin.file] = read(root() / "data" / "roles" / pin.file);
    return bytes;
}

TEST(Ch11Ledger, TheAvatarStaysOutsideThePartyCodeAndItsRolesAreByteForByteChapterTen) {
    const auto found = avatarErrors(scripts(), pinnedRoleBytes());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch11Ledger, APartyCallOrAOneByteRoleEditIsCaught) {
    const Sources sources = scripts();
    const auto bytes = pinnedRoleBytes();
    ASSERT_TRUE(avatarErrors(sources, bytes).empty());
    auto withParty = sources;
    withParty["daoge"] += "\nparty.remove(\"qu_hun_shadan\")\n";
    EXPECT_FALSE(avatarErrors(withParty, bytes).empty());
    auto commented = sources;
    commented["daoge"] += "\n-- party.add(\"qu_hun_shadan\")\n";
    EXPECT_TRUE(avatarErrors(commented, bytes).empty()) << "注释里的不算";
    for (const auto& pin : kPinnedRoles) {
        auto edited = bytes;
        edited[pin.file][edited[pin.file].size() / 2] ^= 0x01;
        EXPECT_FALSE(avatarErrors(sources, edited).empty()) << pin.file;
        auto longer = bytes;
        longer[pin.file] += "\n";
        EXPECT_FALSE(avatarErrors(sources, longer).empty()) << pin.file;
    }
}

// ---- 第 15 条 ----
using Shops = std::map<std::string, fanren::rules::Shop>;
using Items = std::map<std::string, fanren::core::Item>;

Shops shippedShops() {
    Shops shops;
    for (const char* id : {"ch11_fangshi_zahuo", "ch11_ningcui_zahuo", "ch11_ningcui_shougou", "ch11_qingzhu_xiaoxuan"}) {
        const auto loaded = fanren::io::loadShop((root() / "data" / "shops" / (std::string(id) + ".json")).string());
        if (!loaded.ok) throw std::runtime_error(std::string(id) + ": " + loaded.error);
        shops[id] = loaded.value;
    }
    return shops;
}

Errors shopErrors(const Shops& shops, const Json& zahuoFacility, const Items& items) {
    Errors out;
    const auto need = [&](bool ok, const std::string& why) { if (!ok) out.push_back(why); };
    const auto& buyer = shops.at("ch11_ningcui_shougou");
    need(buyer.sellRate == 0.8 && buyer.entries.empty(), "shougou sellRate 0.8 / no entries");
    const auto& own = shops.at("ch11_qingzhu_xiaoxuan");
    need(own.sellRate == 0.6 && own.entries.empty(), "xiaoxuan sellRate 0.6 / no entries");
    struct Herb { const char* shop; int stock, days, amount, cap; };
    for (const auto& herb : {Herb{"ch11_fangshi_zahuo", 10, 5, 5, 10}, Herb{"ch11_ningcui_zahuo", 5, 5, 3, 5}}) {
        int rows = 0;
        for (const auto& e : shops.at(herb.shop).entries) {
            if (e.itemId != "herb_nichang_cao") continue;
            ++rows;
            need(e.stock == herb.stock && e.restockDays == herb.days && e.restockAmount == herb.amount && e.maxStock == herb.cap,
                 std::string(herb.shop) + " nichang restock");
        }
        need(rows == 1, std::string(herb.shop) + " nichang rows " + std::to_string(rows));
    }
    need(zahuoFacility.value("kind", "") == "shop" && zahuoFacility.value("ref_id", "") == "ch11_fangshi_zahuo" &&
         zahuoFacility.value("require_flag", "") == "ch11.nichang", "facility_zahuo gate");
    // 契约 4.2 第 2 条字面量：真数据的妖丹在收购行卖 760 / 4800。
    need(fanren::rules::sellPrice(buyer, items.at("material_yaodan_wuji"), 0) == 760, "yaodan_wuji sells 760");
    need(fanren::rules::sellPrice(buyer, items.at("material_yaodan_liuji"), 0) == 4800, "yaodan_liuji sells 4800");
    need(!items.at("material_jinleizhu").tradeable, "jinleizhu tradeable");
    return out;
}

Items shippedItems() {
    const auto data = fanren::io::loadGameData((root() / "data").string());
    if (!data.ok) throw std::runtime_error(data.error);
    return data.value.items;
}

TEST(Ch11Ledger, TheShopsAndPricesAreTheDesignLiterals) {
    const auto found = shopErrors(shippedShops(), facility("ch11_fangshi", "facility_zahuo"), shippedItems());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch11Ledger, MutationsOfShopsFacilityPricesAndTradeabilityAreCaught) {
    const Shops shops = shippedShops();
    const Json zahuo = facility("ch11_fangshi", "facility_zahuo");
    const Items items = shippedItems();
    ASSERT_TRUE(shopErrors(shops, zahuo, items).empty());
    for (int variant = 0; variant < 9; ++variant) {
        Shops s = shops;
        Json f = zahuo;
        Items i = items;
        switch (variant) {
        case 0: s["ch11_ningcui_shougou"].sellRate = 0.7; break;
        case 1: s["ch11_qingzhu_xiaoxuan"].entries.push_back(s["ch11_fangshi_zahuo"].entries.front()); break;
        case 2: for (auto& e : s["ch11_fangshi_zahuo"].entries) if (e.itemId == "herb_nichang_cao") e.restockAmount = 3; break;
        case 3: for (auto& e : s["ch11_ningcui_zahuo"].entries) if (e.itemId == "herb_nichang_cao") e.maxStock = 10; break;
        case 4: s["ch11_ningcui_zahuo"].entries.clear(); break;
        case 5: f["require_flag"] = "ch11.ziliao"; break;
        case 6: i["material_yaodan_wuji"].price = 1000; break;
        case 7: i["material_yaodan_liuji"].price = 5000; break;
        case 8: i["material_jinleizhu"].tradeable = true; break;
        }
        EXPECT_FALSE(shopErrors(s, f, i).empty()) << "mutation=" << variant;
    }
}

// ---- 第 19 条 ----
Json flagAtLeast(const char* flag, int value) { return Json::array({{{"flag", flag}, {"op", ">="}, {"value", value}}}); }

Errors questErrors(const Json& z1, const Json& z2, const Sources& sources) {
    Errors out;
    const auto need = [&](bool ok, const char* why) { if (!ok) out.push_back(why); };
    need(z1.at("accept") == flagAtLeast("ch11.ningcui", 1), "z1 accept");
    need(z1.value("fail", Json()) == flagAtLeast("ch11.huicheng", 1), "z1 fail");
    need(z1.at("complete") == flagAtLeast("ch11.z1", 2), "z1 complete");
    need(z1.at("origin") == "改编", "z1 origin");
    const std::string order = executableLines(sources.at("shougou_dingdan"));
    need(order.find("if not take(\"material_yaodan_wuji\", 3) then") != std::string::npos, "z1 takes 3 wuji checking the return");
    need(order.find("give(\"material_lingshi_zhong\", 10)") != std::string::npos, "z1 pays 10 mid stones");
    need(order.find("give(\"material_lianqi_yao\", 2)") != std::string::npos, "z1 pays 2 lianqi_yao");
    need(z2.at("accept") == flagAtLeast("ch11.zhaolan", 1), "z2 accept");
    need(z2.at("complete") == flagAtLeast("ch11.z2", 1), "z2 complete");
    const std::string swap = executableLines(sources.at("linju_z2"));
    need(swap.find("if not take(\"pill_dingyan_dan\", 1) then") != std::string::npos, "z2 checks the take");
    return out;
}

TEST(Ch11Ledger, BothSideQuestsMatchTheirPredicatesAndScripts) {
    const Json z1 = json(root() / "data" / "quests" / "q11_shougou_dingdan.json");
    const Json z2 = json(root() / "data" / "quests" / "q11_linju_dingyan.json");
    const auto found = questErrors(z1, z2, scripts());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch11Ledger, MutationsOfTheSideQuestsAreCaught) {
    const Json z1 = json(root() / "data" / "quests" / "q11_shougou_dingdan.json");
    const Json z2 = json(root() / "data" / "quests" / "q11_linju_dingyan.json");
    const Sources sources = scripts();
    ASSERT_TRUE(questErrors(z1, z2, sources).empty());
    for (int variant = 0; variant < 6; ++variant) {
        Json a = z1, b = z2;
        Sources s = sources;
        switch (variant) {
        case 0: a.erase("fail"); break;
        case 1: a["origin"] = "原著"; break;
        case 2: b["accept"] = flagAtLeast("ch11.chuguan", 1); break;
        case 3: { auto& t = s["shougou_dingdan"]; t.replace(t.find("\"material_yaodan_wuji\", 3)"), 26, "\"material_yaodan_wuji\", 2)"); break; }
        case 4: { auto& t = s["linju_z2"]; t.replace(t.find("if not take("), 12, "take("); break; }
        case 5: { auto& t = s["shougou_dingdan"]; t.replace(t.find("material_lingshi_zhong\", 10"), 27, "material_lingshi_zhong\", 12"); break; }
        }
        EXPECT_FALSE(questErrors(a, b, s).empty()) << "mutation=" << variant;
    }
}

// ---- 第 20 条：路径行动 14 条（施工图 18.2 第 1 条落地 id）----
bool containsFlag(const Json& conditions, const char* flag) {
    for (const auto& c : conditions)
        if (c.value("flag", "") == flag && c.value("op", "") == ">=" && c.value("value", 0) == 1) return true;
    return false;
}

Errors pathErrors(const Json& file, const std::map<std::string, Json>& roles) {
    Errors out;
    const std::set<std::string> ids{
        "baiyi_guiju_dating", "luer_shengshan_dating", "fengle_mulu_dating", "chuwu_dai_dating",
        "baimen_zhaixing_dating", "haoyuantian_waihai_dating", "haoyuantian_qianmu_dating", "fanxiaozi_jiage_dating",
        "zahuo_haiyu_dating", "zaohong_jiguan_dating", "linju_miaoyin_dating", "linju_dingyan_dating",
        "hexianshi_fanren_dating", "jinqing_luanxing_dating"};
    // 两条 reveal：郝远天说千目妖怕土、枣红老者说鸡冠蛟怕金。
    const std::map<std::string, std::pair<std::string, std::string>> reveals{
        {"haoyuantian_qianmu_dating", {"qianmu_yao", "土"}}, {"zaohong_jiguan_dating", {"jiguan_jiao", "金"}}};
    std::set<std::string> seen;
    for (const auto& entry : file.at("entries")) {
        const std::string id = entry.at("id");
        seen.insert(id);
        if (!ids.count(id)) out.push_back("unexpected path action " + id);
        if (entry.at("kind") != "inquire") out.push_back(id + " kind");
        if (entry.at("realm") != 0) out.push_back(id + " realm");
        if (!containsFlag(entry.at("until"), "ch11.done")) out.push_back(id + " until lacks ch11.done");
        if (entry.at("done_flag") != "ch11.path." + id) out.push_back(id + " done_flag");
        const auto reveal = reveals.find(id);
        const Json shown = entry.value("reveal", Json::array());
        if (reveal == reveals.end()) {
            if (!shown.empty()) out.push_back(id + " reveals something");
            continue;
        }
        if (shown != Json::array({{{"role", reveal->second.first}, {"category", reveal->second.second}}}))
            out.push_back(id + " reveal");
        const auto weak = roles.at(reveal->second.first).at("weaknesses").get<std::set<std::string>>();
        if (!weak.count(reveal->second.second)) out.push_back(id + " reveals a category the role does not have");
    }
    if (seen != ids) out.push_back("path action ids differ from the fourteen landed ids");
    for (const auto& entry : file.at("entries"))
        if (entry.at("id") == "zaohong_jiguan_dating" && !containsFlag(entry.at("until"), "ch11.chuansong"))
            out.push_back("zaohong_jiguan_dating is not missable at ch11.chuansong");
    return out;
}

std::map<std::string, Json> revealRoles() {
    std::map<std::string, Json> roles;
    for (const char* id : {"qianmu_yao", "jiguan_jiao"}) roles[id] = json(root() / "data" / "roles" / (std::string(id) + ".json"));
    return roles;
}

TEST(Ch11Ledger, TheFourteenInquiriesAreUngatedAndRevealOnlyRealWeaknesses) {
    const auto found = pathErrors(json(root() / "data" / "pathactions" / "ch11.json"), revealRoles());
    EXPECT_TRUE(found.empty()) << joined(found);
}

TEST(Ch11Ledger, MutationsOfThePathActionsAreCaught) {
    const Json file = json(root() / "data" / "pathactions" / "ch11.json");
    const auto roles = revealRoles();
    ASSERT_TRUE(pathErrors(file, roles).empty());
    const auto index = [&](const char* id) {
        for (std::size_t i = 0; i < file.at("entries").size(); ++i) if (file["entries"][i]["id"] == id) return i;
        throw std::runtime_error(id);
    };
    for (int variant = 0; variant < 7; ++variant) {
        Json f = file;
        auto r = roles;
        switch (variant) {
        case 0: f["entries"][index("fengle_mulu_dating")]["realm"] = 21; break;
        case 1: f["entries"][index("luer_shengshan_dating")]["kind"] = "purchase"; break;
        case 2: f["entries"][index("jinqing_luanxing_dating")]["until"] = Json::array(); break;
        case 3: f["entries"][index("zaohong_jiguan_dating")]["until"].erase(std::size_t{0}); break;
        case 4: r["qianmu_yao"]["weaknesses"] = Json::array({"剑"}); break;
        case 5: f["entries"].erase(index("chuwu_dai_dating")); break;
        case 6: f["entries"][index("baiyi_guiju_dating")]["reveal"] = Json::array({{{"role", "qianmu_yao"}, {"category", "土"}}}); break;
        }
        EXPECT_FALSE(pathErrors(f, r).empty()) << "mutation=" << variant;
    }
}
} // namespace
