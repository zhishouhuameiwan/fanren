// 第 7 章同道录 2a 集成：给用户试玩的存档（docs/ch07-tongdaolu-integration.md 第 7 节，saves/README.md「检查点」表）。
//
// 这不是验收用例，是造试玩档的辅助：只有设了环境变量 FANREN_TONGDAO_PLAYSAVES=<目录> 才写，没设就跳过（全量 ctest 里它是 Skipped）。
// 都从「刚出禁地」那份检查点（tests/fixtures/companions/ch07-chujindi-first.sav，第 7 章通关测试沿途比着的那一份）起，
// 不改任何交接档与检查点：
//   甲「刚出禁地」     不另存：就是那份检查点原样（默契照主线的账，只有南宫婉 37），这里只核一遍读写回来逐字节相同。
//   ch07-tongdao-t3.sav  乙「三位都在三档门口」：演过谢师礼、站在百药园茅屋里蒲团前，三位默契各 42（合意赠礼 +5 或同修 +4 就到 45「三档」），
//                        二档的叙话都还没演；背包里各人喜欢的礼物、打切磋用的丹药。调试档。
//   ch07-tongdao-top.sav 丙「最高档门口」：陈巧倩 93、菡云芝 93（差一步到五档），南宫婉 67（差一步到四档，她第 7 章封在四档）；
//                        前面各档的叙话、请教都记过账（菡云芝二档那 3 张火球符照账发了），三条在册加成已生效。调试档。
// 另设 FANREN_TONGDAO_SHOTSAVE=<文件> 时多写一份截图用的（丙推到各人最高档 95 / 94 / 95、三位都在队伍里），不入库。
// 重生成（入库的两份）：FANREN_TONGDAO_PLAYSAVES=saves，在 fanren/ 下跑 build-<槽>/fanren_tests.exe --gtest_filter=TongdaoPlaySaves.*（命令见 saves/README.md）
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "ChapterFixture.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::GameState;
using fanren::game::Application;
namespace rules = fanren::rules;

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "companions" / "han_yunzhi.json")) return candidate;
    }
    return ".";
}

std::string envValue(const char* name) {
#if defined(_MSC_VER)
    char* value = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) return {};
    std::string out = value;
    std::free(value);
    return out;
#else
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
#endif
}

std::string readBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

class TongdaoPlaySaves : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = envValue("FANREN_TONGDAO_PLAYSAVES");
        if (dir_.empty()) GTEST_SKIP() << "造试玩档的辅助：设 FANREN_TONGDAO_PLAYSAVES=<目录> 才写";
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override {
        if (!dir_.empty()) app_.shutdown();
    }

    GameState& state() { return app_.state(); }

    void startFromChujindi() {
        const auto loaded = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), "companions/ch07-chujindi-first.sav").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state() = loaded.value;
        ASSERT_EQ(state().flag("ch07.chujindi"), 1);
        ASSERT_EQ(state().flag("ch07.xieshili"), 0);
        ASSERT_EQ(state().mapId, "ch06_baiyaoyuan");
    }

    // 跑一段剧情脚本到底（对白一路按确认），与 TongdaoContentTests 的 runEvent 同一个办法。
    void runEvent(const std::string& path) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << "：" << started.error;
        for (int frame = 0; frame < 4000; ++frame) {
            app_.tick(1.0 / 60.0);
            if (app_.awaitingCommand()) {
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
                if (app_.topScene() != nullptr && app_.topScene()->name() == "Dialogue") app_.popScene();
                continue;
            }
            if (!app_.scripts().isRunning()) break;
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path;
        ASSERT_TRUE(app_.scripts().lastError().empty()) << app_.scripts().lastError();
    }

    // 谢师礼演过、站到茅屋里蒲团（facility_dazuo，23,4）正下方一格、面朝它：一进游戏就能打坐 / 同修，右边两格是历练石碑。
    void settleInTheHut() {
        runEvent("ch07/xieshili.lua");
        if (HasFatalFailure()) return;
        ASSERT_EQ(state().flag("ch07.xieshili"), 1);
        state().mapId = "ch06_baiyaoyuan";
        state().position = {23, 5};
        state().facing = 0;
    }

    void giveLikesAndPills() {
        for (const auto& [id, companion] : app_.data().companions) {
            for (const std::string& like : companion.giftsLike) state().addItem(like, 3);
        }
        state().addItem("pill_yangjing_dan", 8);
        state().addItem("pill_jinchuang_yao", 6);
        state().addItem("pill_huiqi_dan", 3);
    }

    void write(const char* fileName) {
        fs::create_directories(dir_);
        const auto wrote = fanren::io::saveGame(state(), (fs::path(dir_) / fileName).string());
        ASSERT_TRUE(wrote.ok) << wrote.error;
    }

    std::string dir_;
    Application app_;
};

TEST_F(TongdaoPlaySaves, WriteTheTier3AndTopGateSaves) {
    // 甲：就是检查点原样，不另存——核一遍读进来再写出去逐字节相同（说明里直接指向那份检查点）。
    startFromChujindi();
    {
        const fs::path checkpoint = fanren::test::chapterFixturePath(assetRoot(), "companions/ch07-chujindi-first.sav");
        const fs::path probe = fs::temp_directory_path() / "fanren_tongdao_jia_probe.sav";
        ASSERT_TRUE(fanren::io::saveGame(state(), probe.string()).ok);
        EXPECT_EQ(readBytes(probe), readBytes(checkpoint)) << "甲与检查点不同：要另存一份 ch07-tongdao-start.sav";
        std::error_code ignored;
        fs::remove(probe, ignored);
    }

    // 乙：三位都在三档门口。
    startFromChujindi();
    settleInTheHut();
    ASSERT_FALSE(HasFatalFailure());
    for (const char* id : {"chen_qiaoqian", "nangong_wan", "han_yunzhi"}) {
        ASSERT_TRUE(rules::inRoster(app_.data().companions.at(id), state())) << id;
        state().bonds[id].value = 42;
    }
    giveLikesAndPills();
    write("ch07-tongdao-t3.sav");

    // 丙：最高档门口。
    startFromChujindi();
    settleInTheHut();
    ASSERT_FALSE(HasFatalFailure());
    struct Gate {
        const char* id;
        int bond;
        int talkedTier;
    };
    for (const Gate& g : {Gate{"chen_qiaoqian", 93, 4}, Gate{"nangong_wan", 67, 3}, Gate{"han_yunzhi", 93, 4}}) {
        state().bonds[g.id].value = g.bond;
        state().setFlag(rules::xuhuaFlag(g.id), g.talkedTier);
        state().setFlag(rules::qingjiaoFlag(g.id), 1);
        state().setFlag(rules::sparPrizeFlag(g.id), g.talkedTier);
    }
    state().addItem("talisman_huoqiu_fu", 3);   // 菡云芝二档叙话照账发过的那 3 张
    giveLikesAndPills();
    const auto active = rules::activeRosterBonuses(app_.data().companions, state());
    EXPECT_EQ(active.size(), 3u) << "三条在册加成都生效";
    write("ch07-tongdao-top.sav");

    // 截图用（不入库）：丙再往上推到各人最高档、三位都在队伍里，截图口 battle:<编成> 拍按档的法术、合招与辅助法术的演法。
    const std::string shotSave = envValue("FANREN_TONGDAO_SHOTSAVE");
    if (shotSave.empty()) return;
    state().bonds["chen_qiaoqian"].value = 95;
    state().bonds["nangong_wan"].value = 94;
    state().bonds["han_yunzhi"].value = 95;
    for (const char* role : {"chen_shimei", "nangong_wan_ch07", "han_yunzhi"}) ASSERT_TRUE(state().partyAdd(role)) << role;
    const auto wrote = fanren::io::saveGame(state(), shotSave);
    ASSERT_TRUE(wrote.ok) << wrote.error;
}

}  // namespace
