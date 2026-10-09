#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include "ChapterFixture.h"
#include "io/DataLoader.h"

namespace {
using namespace fanren;
std::string code(const std::filesystem::path& file) {
    std::ifstream in(file, std::ios::binary);
    std::string out, line;
    while (std::getline(in, line)) out += line.substr(0, line.find("--")) + '\n';
    return out;
}
std::multiset<std::string> callers(const std::string& root, const std::string& expression) {
    const std::regex pattern(expression);
    std::multiset<std::string> found;
    for (const auto& e : std::filesystem::directory_iterator(root + "/scripts/ch08")) {
        if (e.path().extension() != ".lua") continue;
        const std::string source = code(e.path());
        for (auto i = std::sregex_iterator(source.begin(), source.end(), pattern); i != std::sregex_iterator(); ++i)
            found.insert(e.path().stem().string());
    }
    return found;
}
TEST(Ch08Ledger, CurrencyAndPillMutationsOccurOnlyAtTheDesignNodes) {
    const auto root = test::chapterEightAssetRoot();
    const std::string give = R"re(\bgive\("material_lingshi")re";
    ASSERT_TRUE(std::regex_search(std::string("give(\"material_lingshi\", 5)"), std::regex(give)));
    EXPECT_EQ(callers(root, give), (std::multiset<std::string>{"dating", "chuansongzhen", "jiaoyisuo", "liesha"}));
    EXPECT_EQ(callers(root, R"re(\btake\("material_lingshi")re"),
              (std::multiset<std::string>{"dengji", "kaifu", "caihuan", "yindong"}));
    EXPECT_EQ(callers(root, R"re(\btake\("pill_zhuji_dan")re"), (std::multiset<std::string>{"biguan"}));
    EXPECT_TRUE(callers(root, R"re(\bgive\("pill_zhuji_dan")re").empty());
    EXPECT_EQ(callers(root, R"re(\brealm\.advance\(22\))re"), (std::multiset<std::string>{"quanyan"}));
    EXPECT_TRUE(callers(root, R"re(\brealm\.cap\()re").empty());
    EXPECT_EQ(callers(root, R"re(\bmagic\.learn\()re"),
              (std::multiset<std::string>{"shixiong", "quqi", "xifeng", "xifeng", "chuansongzhen"}));
    EXPECT_EQ(callers(root, R"re(\bmagic\.forget\("magic_ji_qingchi"\))re"),
              (std::multiset<std::string>{"zhulinxin"}));
    EXPECT_EQ(callers(root, R"re(\bparty\.add\("kuilei_shou"\))re"),
              (std::multiset<std::string>{"jingshi", "shanding"}));
    EXPECT_EQ(callers(root, R"re(\bparty\.remove\("kuilei_shou"\))re"),
              (std::multiset<std::string>{"lenggong"}));
    const std::multiset<std::string> first = {"zhenwei_jin", "zhenwei_mu", "zhenwei_shui", "zhenwei_huo"};
    EXPECT_EQ(callers(root, R"re(\bplace\("ch08\.zhenqi1")re"), first);
    EXPECT_EQ(callers(root, R"re(\bplace\("ch08\.zhenqi2")re"),
              (std::multiset<std::string>{"zhulin_1", "zhulin_2", "zhulin_3", "zhulin_4"}));
    EXPECT_EQ(callers(root, R"re(\bplace\("ch08\.zhenqi3")re"),
              (std::multiset<std::string>{"milin_1", "milin_2", "milin_3", "milin_4"}));
    // Design 18: generated wrappers bind the common mask writer's flag_name parameter.
    ASSERT_TRUE(std::regex_search(std::string("place(\"ch08.zhenqi1\", 1)"),
                                  std::regex(R"re(\bplace\("ch08\.zhenqi1")re")));
    EXPECT_TRUE(callers(root, R"re(\bflag\.set\("ch08\.zhenqi[123]")re").empty());
    EXPECT_EQ(callers(root, R"re(\bflag\.set\(flag_name,\s*mask\s*\|\s*bit\))re").size(), 12u);
}
TEST(Ch08Ledger, StoryResourcesCannotBeSoldToFundAnArtificialWalkthrough) {
    const auto data = io::loadGameData(test::chapterEightAssetRoot() + "/data");
    ASSERT_TRUE(data.ok) << data.error;
    for (const char* id : {"herb_zigui_hua", "pill_lianqi_san", "pill_dingyan_dan", "herb_huangjing_zhi",
         "herb_yusui_zhi", "herb_zihou_hua", "herb_tianling_guo", "material_zhuji_yaofen", "pill_zhuji_dan"}) {
        ASSERT_TRUE(data.value.items.count(id)) << id;
        EXPECT_FALSE(data.value.items.at(id).tradeable) << id;
    }
}
}  // namespace
