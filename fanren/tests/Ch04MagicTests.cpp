// P3 第 4 章前置之一：已习得法术（契约 docs/interfaces-p3-ch04.md 第 1 节，技术债 G-4）。
//
// 在这一批之前，「韩立学会了火弹术」这件事**无处存放**：magics 只在 RoleTemplate
//（data，运行期只读）上，GameState 里没有这一项。本文件从四个高度钉这条链路：
//
//   一、GameState 上的集合语义（幂等、去重、顺序、忘掉）；
//   二、存档：往返、版本号、4→5 迁移已登记、手改过的坏数据；
//   三、脚本 API：跑的是真的 api.lua，不是复刻品；
//   四、战斗：学会之后菜单里**真的点得出来**，而且走的是 issuePlayerAction。
//
// 写断言前逐条对照 docs/README.md 那张「判据会说谎」表，本文件对上了这几条：
//   · 「找不到某个词就算过」—— 每一条禁用理由先 ASSERT_FALSE(empty())，再验内容；
//   · 「只钉一个数据点」—— 版本号那一条既钉住字面值 5（设计原文），又扫一遍
//      1..kSaveVersion-1 每一版都读得回来；
//   · 「判据是从被测物推导出来的」—— 契约第 1.2 节白纸黑字写着「kSaveVersion
//      4 → 5」，所以那条断言写死成 5，不写成 kSaveVersion（那是恒真）。
#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#include "TempDir.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/SaveFile.h"
#include "ui/Widgets.h"

namespace {

namespace fs = std::filesystem;

using fanren::core::GameState;
using fanren::game::Application;
using fanren::game::BattleMenuMode;
using fanren::game::BattleScene;
using fanren::game::kBattleMenuCast;
using fanren::io::kSaveVersion;
using fanren::rules::Realm;

// 第 4 章韩立要学的第一门法术是火弹术（原著 ch67），但 data/magics/ 眼下没有这一条，
// 而 data/magics/** 不在本批的白名单里（见交付报告）。测试因此用 data 里**真的有**的
// 一条低阶火系法术：火球术（QiRefining1 / 耗法力 8）。
//
// 引擎侧一个法术 id 都不认识（learn 只查 data 里有没有），所以换成哪一条都一样；
// 编剧把火弹术加进 data 之后，脚本写 magic.learn("magic_huodan_shu") 即可，
// 这里一个字都不必改。
constexpr const char* kKnownMagic = "magic_huoqiu_shu";
constexpr const char* kKnownMagicName = "火球术";
constexpr const char* kOtherMagic = "magic_hushen_gang";
// data/magics/ 里必然不存在的一条。负向用例用它。
constexpr const char* kBogusMagic = "magic_bu_cun_zai";

void writeText(const fs::path& path, const std::string& text) {
    fs::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// ---------------------------------------------------------------------------
// 一、GameState 上的集合语义
// ---------------------------------------------------------------------------

TEST(Ch04LearnedMagics, LearnIsIdempotentAndKeepsTheOrderTheyWereLearnedIn) {
    GameState s;
    EXPECT_EQ(s.learnedMagics.size(), 0u);
    EXPECT_FALSE(s.knowsMagic(kKnownMagic));

    EXPECT_TRUE(s.learnMagic(kKnownMagic)) << "第一次学会，确实发生了变化";
    EXPECT_FALSE(s.learnMagic(kKnownMagic)) << "再学一次什么都不该发生";
    EXPECT_TRUE(s.learnMagic(kOtherMagic));

    // 顺序就是习得的先后——火弹术在前、御风决在后是本章的剧情（设计第 1 节约束 1），
    // 而战斗菜单的行序照着这份清单走。排序一下看着整齐，代价是那个先后由拼音决定。
    ASSERT_EQ(s.learnedMagics.size(), 2u);
    EXPECT_EQ(s.learnedMagics[0], kKnownMagic);
    EXPECT_EQ(s.learnedMagics[1], kOtherMagic);
    EXPECT_TRUE(s.knowsMagic(kKnownMagic));
    EXPECT_TRUE(s.knowsMagic(kOtherMagic));
}

TEST(Ch04LearnedMagics, AnEmptyIdIsNeverAccepted) {
    // 负向。空串进了清单，战斗菜单里会多出一条查不到名字的行。
    GameState s;
    EXPECT_FALSE(s.learnMagic(""));
    EXPECT_EQ(s.learnedMagics.size(), 0u);
    EXPECT_FALSE(s.knowsMagic(""));
}

TEST(Ch04LearnedMagics, ForgetReportsWhetherAnythingWasActuallyForgotten) {
    GameState s;
    ASSERT_TRUE(s.learnMagic(kKnownMagic));

    EXPECT_FALSE(s.forgetMagic(kOtherMagic)) << "本来就不会的，不该报成「忘了」";
    EXPECT_EQ(s.learnedMagics.size(), 1u) << "失败的一次不该动清单";

    EXPECT_TRUE(s.forgetMagic(kKnownMagic));
    EXPECT_FALSE(s.knowsMagic(kKnownMagic));
    EXPECT_EQ(s.learnedMagics.size(), 0u);
    EXPECT_FALSE(s.forgetMagic(kKnownMagic)) << "忘过一次之后再忘就该是 false";
}

// ---------------------------------------------------------------------------
// 二、存档
// ---------------------------------------------------------------------------

// 一份最小可读的 GameState。fromJson 对 mapId / position / facing 等等一律要求
// 存在且类型正确，所以往返用例不能拿默认构造的 GameState 直接存。
GameState roundTripState() {
    GameState s;
    s.mapId = "ch04_luorifeng";
    s.position = fanren::core::Point{3, 4};
    s.facing = 1;
    s.realm = Realm::QiRefining3;
    s.realmCap = s.realm;  // v9 的合法保存夹具；原往返断言不变。
    s.cultivation = 12;
    s.hp = 40;
    s.maxHp = 60;
    s.mp = 20;
    s.maxMp = 30;
    s.day = 900;
    s.chapter = 4;
    return s;
}

GameState roundTrip(const GameState& before, const std::string& tag) {
    const fs::path path = fanren::test::uniqueTempPath(tag, ".json");
    const auto saved = fanren::io::saveGame(before, path.string());
    EXPECT_TRUE(saved.ok) << saved.error;
    const auto loaded = fanren::io::loadGame(path.string());
    EXPECT_TRUE(loaded.ok) << loaded.error;
    std::error_code ec;
    fs::remove(path, ec);
    return loaded.ok ? loaded.value : GameState{};
}

TEST(Ch04MagicSave, AnEmptyListStaysEmpty) {
    const GameState after = roundTrip(roundTripState(), "fanren_ch04_magic_empty");
    EXPECT_TRUE(after.learnedMagics.empty());
    // 先验：这份夹具确实往返得回来，否则上面那条空表可能只是因为读档整个失败了。
    EXPECT_EQ(after.mapId, "ch04_luorifeng");
    EXPECT_EQ(after.chapter, 4);
}

TEST(Ch04MagicSave, OneAndManyBothComeBackInOrder) {
    GameState before = roundTripState();
    ASSERT_TRUE(before.learnMagic(kKnownMagic));
    const GameState one = roundTrip(before, "fanren_ch04_magic_one");
    ASSERT_EQ(one.learnedMagics.size(), 1u);
    EXPECT_EQ(one.learnedMagics[0], kKnownMagic);

    ASSERT_TRUE(before.learnMagic(kOtherMagic));
    ASSERT_TRUE(before.learnMagic("magic_bingjian_shu"));
    const GameState many = roundTrip(before, "fanren_ch04_magic_many");
    ASSERT_EQ(many.learnedMagics.size(), 3u);
    // **顺序也要原样回来**：只比个数的话，一份把清单排了序的实现照样全绿，
    // 而那正好把「火弹术在前、御风决在后」这条剧情先后抹掉。
    EXPECT_EQ(many.learnedMagics, before.learnedMagics);
}

TEST(Ch04MagicSave, ARoundTripIsExactlyTheIdentityForAnyStateReachableThroughTheApi) {
    // 去重那条规则让「手写一份带重复项的 payload 再读回来」不是恒等变换，
    // 而往返恒等是这一层最硬的断言。这一条钉的是代价的边界：**经由 API 到达的
    // 任何状态，往返仍然恒等**——因为 learnMagic 幂等，重复项根本写不出来。
    GameState before = roundTripState();
    for (int i = 0; i < 3; ++i) {
        static_cast<void>(before.learnMagic(kKnownMagic));   // 学三遍
        static_cast<void>(before.learnMagic(kOtherMagic));
    }
    ASSERT_EQ(before.learnedMagics.size(), 2u) << "先验：写入侧已经去过重了";

    const GameState after = roundTrip(before, "fanren_ch04_magic_identity");
    EXPECT_EQ(after.learnedMagics, before.learnedMagics);
}

// ---- 手拼存档：校验和自算，与 tests/IoTests.cpp 同一个办法 ----
//
// checksum 把版本号一起绑了进去，所以没法靠文本替换伪造出「合法校验和 + 想要的
// 版本号」。这里复刻一份 FNV-1a，模拟的是真实场景（文件没被篡改，只是来自另一个
// 版本的游戏），而不是去复用生产代码的私有实现。
std::uint64_t testFnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string testChecksum(int version, const std::string& payloadDump) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0')
        << testFnv1a64(std::to_string(version) + "|" + payloadDump);
    return oss.str();
}

std::string saveFileText(int version, const std::string& payloadDump) {
    return "{\"save_version\":" + std::to_string(version) + ",\"checksum\":\"" +
           testChecksum(version, payloadDump) + "\",\"payload\":" + payloadDump + "}";
}

TEST(Ch04MagicSave, TheSaveVersionIsFive) {
    // **不经过驱动、直接读常量、判据写死成设计原文。**
    // 契约 docs/interfaces-p3-ch04.md 第 1.2 节：「kSaveVersion 4 → 5」。
    // 写成 EXPECT_EQ(kSaveVersion, kSaveVersion) 一类的恒真式，就是
    // docs/README.md 那条「判据是从被测物推导出来的」——它只发现得了「变了」，
    // 发现不了「与规格不一致」。
    //
    // 此后又推过两次：第 4 章二次整改的境界上限推到 6，八方旅人化改造的「已揭开的破绽」
    // 推到 7（docs/octopath-battle.md 2.5、io/SaveFile.h 第 7 条）。判据照旧写死成当下
    // 那份设计原文的数。再推到 8：野外遭遇的计数器（docs/interfaces-octo-encounters.md、
    // io/SaveFile.h 第 8 条）。
    // 第 9 章 E1/E2 的 formerRealm 将当前版本推到 9。
    EXPECT_EQ(kSaveVersion, 9);
}

TEST(Ch04MagicSave, TheFourToFiveMigrationIsRegistered) {
    // 空 payload 必然通过校验和与版本检查，再必然在字段解析阶段失败。看的是**失败
    // 在哪一步**：4→5 没登记的话，while 循环在 migrations().find(4) 上落空，
    // 报的是「无法识别的存档版本号 v4」，压根走不到解析；登记了才会跑完迁移、
    // 进入解析、报「缺少 mapId」。与 tests/IoTests.cpp 那条 v1 用例同一个办法。
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_v4", ".json");
    writeText(path, saveFileText(4, "{}"));

    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_FALSE(loaded) << "空 payload 不该被当成合法存档";
    EXPECT_NE(loaded.error.find("mapId"), std::string::npos)
        << "期望走到字段解析才失败（说明 4→5 登记了），实际错误是：" << loaded.error;
    EXPECT_EQ(loaded.error.find("无法识别"), std::string::npos) << loaded.error;

    std::error_code ec;
    fs::remove(path, ec);
}

TEST(Ch04MagicSave, AVersionWithNoMigrationIsStoppedAtTheVersionCheck) {
    // 上一条的负向对照：证明「失败在哪一步」这个判据真的分得开两种情形。
    // 没有这一条，上面那句 EXPECT_NE(...find("mapId")) 有可能对任何版本号都成立。
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_v0", ".json");
    writeText(path, saveFileText(0, "{}"));

    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_FALSE(loaded);
    EXPECT_EQ(loaded.error.find("mapId"), std::string::npos)
        << "不该走到字段解析：" << loaded.error;

    std::error_code ec;
    fs::remove(path, ec);
}

// 一份合法的 v5 payload，learnedMagics 那一项由调用方填。
//
// 校验和覆盖 "版本号|payload.dump()"，而 dump() 输出的是**按 key 字节序排好的
// 紧凑形**，所以下面这串必须自己就写成那个样子。只写 fromJson 真正要求存在的
// 那几个字段：其余（bottle / fields / 四艺熟练度 / party……）都是 value() 读的
// 可选项，写进来只是多几处会拼错的地方。键序与写法照
// tests/Ch03PartyTests.cpp 的 canonicalPayload——那一份已经被验过。
std::string payloadWithMagics(const std::string& magicsArray) {
    std::string p = "{";
    p += R"("bag":[],)";
    p += R"("chapter":4,)";
    p += R"("cultivation":12,)";
    p += R"("day":40,)";
    p += R"("facing":2,)";
    p += R"("flags":{},)";
    p += R"("hp":30,)";
    // hp < learnedMagics < mapId，按字节序正好落在这里。
    p += R"("learnedMagics":)" + magicsArray + ",";
    p += R"("mapId":"ch04_luorifeng",)";
    p += R"("maxHp":42,)";
    p += R"("maxMp":8,)";
    p += R"("mp":5,)";
    p += R"("playSecondsGameplay":0.0,)";
    p += R"("playSecondsSystem":0.0,)";
    p += R"("position":{"x":3,"y":4},)";
    p += R"("realm":4)";
    p += "}";
    return p;
}

TEST(Ch04MagicSave, TheHandBuiltPayloadFixtureIsItselfValid) {
    // **先验判据。** 这一条要是红的，下面两条的红绿一个都不作数——它们分不清
    // 「去重生效了 / 非字符串被拒了」与「我这份手拼的 JSON 本来就读不进来」。
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_hand_ok", ".json");
    writeText(path, saveFileText(kSaveVersion, payloadWithMagics(R"(["magic_a","magic_b"])")));

    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    EXPECT_EQ(loaded.value.mapId, "ch04_luorifeng");
    ASSERT_EQ(loaded.value.learnedMagics.size(), 2u);
    EXPECT_EQ(loaded.value.learnedMagics[0], "magic_a");
    EXPECT_EQ(loaded.value.learnedMagics[1], "magic_b");

    std::error_code ec;
    fs::remove(path, ec);
}

TEST(Ch04MagicSave, DuplicatesInAHandEditedSaveAreFoldedKeepingTheFirst) {
    // 口径（契约第 1.2 节让实现方定并论证）：**去重，保留第一次出现的位置。**
    // 理由写在 core/model/Types.h：这份清单的语义是集合，而战斗菜单逐条遍历它
    // 建行——重复的 id 会让同一门法术在菜单里出现两行。
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_dup", ".json");
    writeText(path,
              saveFileText(kSaveVersion, payloadWithMagics(R"(["magic_a","magic_b","magic_a"])")));

    const auto loaded = fanren::io::loadGame(path.string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    ASSERT_EQ(loaded.value.learnedMagics.size(), 2u);
    EXPECT_EQ(loaded.value.learnedMagics[0], "magic_a") << "保留的是第一次出现的位置";
    EXPECT_EQ(loaded.value.learnedMagics[1], "magic_b");

    std::error_code ec;
    fs::remove(path, ec);
}

TEST(Ch04MagicSave, ANonStringEntryIsRefusedInsteadOfSilentlyDropped) {
    // 负向。一个数字根本不是法术 id；静默丢掉只会把一份坏存档的症状推迟到
    // 玩家打开战斗菜单那一刻。与 flags 里「值不是整数就报错」同一条口径。
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_badmagic", ".json");
    writeText(path, saveFileText(kSaveVersion, payloadWithMagics(R"(["magic_a",42])")));

    const auto loaded = fanren::io::loadGame(path.string());
    EXPECT_FALSE(loaded.ok) << "非字符串条目必须当场报错";
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("learnedMagics"), std::string::npos) << loaded.error;

    std::error_code ec;
    fs::remove(path, ec);
}

TEST(Ch04MagicSave, ANonArrayLearnedMagicsIsRefused) {
    const auto path = fanren::test::uniqueTempPath("fanren_ch04_notarray", ".json");
    writeText(path, saveFileText(kSaveVersion, payloadWithMagics(R"("magic_a")")));

    const auto loaded = fanren::io::loadGame(path.string());
    EXPECT_FALSE(loaded.ok);
    ASSERT_FALSE(loaded.error.empty());
    EXPECT_NE(loaded.error.find("learnedMagics"), std::string::npos) << loaded.error;

    std::error_code ec;
    fs::remove(path, ec);
}

// ---------------------------------------------------------------------------
// 三、脚本 API：跑的是真的 api.lua
// ---------------------------------------------------------------------------

fs::path uniqueTempRoot() {
    static std::atomic<std::uint64_t> counter{0};
    const auto stamp = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return fs::temp_directory_path() /
           ("fanren_ch04_magic_root_" + std::to_string(fanren::test::currentProcessId()) + "_" +
            std::to_string(stamp) + "_" + std::to_string(counter.fetch_add(1)));
}

fs::path findProjectRoot() {
    fs::path dir = fs::current_path();
    for (int depth = 0; depth < 8; ++depth) {
        if (fs::exists(dir / "scripts" / "common" / "api.lua") && fs::is_directory(dir / "data")) {
            return dir;
        }
        const fs::path parent = dir.parent_path();
        if (parent.empty() || parent == dir) break;
        dir = parent;
    }
    return {};
}

class Ch04MagicScriptTest : public ::testing::Test {
protected:
    static fs::path& root() {
        static fs::path path;
        return path;
    }

    // 资源根只搭一次：data/ 有上百个文件。全程只读，用例之间不会互相弄脏。
    //
    // 夹具脚本写在这里而不是 tests/scripts/ 下：本批的文件白名单只给了
    // tests/Ch04*.cpp，而这几段 lua 只有本文件用得上。
    static void SetUpTestSuite() {
        const fs::path source = findProjectRoot();
        ASSERT_FALSE(source.empty()) << "找不到工程根（应含 scripts/common/api.lua 与 data/）";

        root() = uniqueTempRoot();
        std::error_code ec;
        fs::create_directories(root() / "scripts" / "common", ec);
        fs::copy(source / "data", root() / "data", fs::copy_options::recursive, ec);
        ASSERT_FALSE(ec) << "复制 data/ 失败: " << ec.message();
        // **跑的是真的 api.lua，不是复刻品**：封装写错了要在这里红，不是等上线。
        fs::copy_file(source / "scripts" / "common" / "api.lua",
                      root() / "scripts" / "common" / "api.lua",
                      fs::copy_options::overwrite_existing, ec);
        ASSERT_FALSE(ec) << "复制 api.lua 失败: " << ec.message();

        // 学一门真法术，并把返回值写进旗标——脚本能不能看见成败，靠的就是它。
        writeText(root() / "scripts" / "t" / "learn_ok.lua",
                  std::string("local ok, code = magic.learn(\"") + kKnownMagic + "\")\n" +
                      "flag.set(\"t.ok\", ok and 1 or 0)\n" +
                      "flag.set(\"t.has_code\", (code ~= \"\") and 1 or 0)\n" +
                      "flag.set(\"t.knows\", magic.knows(\"" + kKnownMagic + "\") and 1 or 0)\n" +
                      "flag.set(\"t.count\", magic.count())\n");
        // 学两遍：幂等，且第二遍仍然 ok。
        writeText(root() / "scripts" / "t" / "learn_twice.lua",
                  std::string("magic.learn(\"") + kKnownMagic + "\")\n" +
                      "local ok = magic.learn(\"" + kKnownMagic + "\")\n" +
                      "flag.set(\"t.ok\", ok and 1 or 0)\n" +
                      "flag.set(\"t.count\", magic.count())\n");
        // 学一门不存在的：必须回填失败，且清单一动不动。
        writeText(root() / "scripts" / "t" / "learn_bogus.lua",
                  std::string("local ok, code = magic.learn(\"") + kBogusMagic + "\")\n" +
                      "flag.set(\"t.ok\", ok and 1 or 0)\n" +
                      "flag.set(\"t.no_magic\", (code == \"no_magic\") and 1 or 0)\n" +
                      "flag.set(\"t.count\", magic.count())\n");
        writeText(root() / "scripts" / "t" / "forget.lua",
                  std::string("magic.learn(\"") + kKnownMagic + "\")\n" +
                      "local ok = magic.forget(\"" + kKnownMagic + "\")\n" +
                      "local again, code = magic.forget(\"" + kKnownMagic + "\")\n" +
                      "flag.set(\"t.first\", ok and 1 or 0)\n" +
                      "flag.set(\"t.again\", again and 1 or 0)\n" +
                      "flag.set(\"t.not_learned\", (code == \"not_learned\") and 1 or 0)\n" +
                      "flag.set(\"t.count\", magic.count())\n");
    }

    static void TearDownTestSuite() {
        std::error_code ec;
        fs::remove_all(root(), ec);
    }

    void SetUp() override {
        auto ready = app_.init(root().string(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    void runScript(const std::string& path) {
        const auto started = app_.startEvent(path);
        ASSERT_TRUE(started.ok) << path << ": " << started.error;
        for (int frame = 0; frame < 64 && app_.scripts().isRunning(); ++frame) {
            app_.tick(1.0 / 60.0);
        }
        ASSERT_FALSE(app_.scripts().isRunning()) << path << " 没能跑到结束";
    }

    GameState& state() { return app_.state(); }

    Application app_;
};

TEST_F(Ch04MagicScriptTest, LearnPutsItOnTheSheetAndSaysSo) {
    runScript("t/learn_ok.lua");
    EXPECT_EQ(state().flag("t.ok"), 1) << "learn 成功时要回填 ok = true";
    EXPECT_EQ(state().flag("t.has_code"), 0) << "成功时原因码要是空串";
    EXPECT_EQ(state().flag("t.knows"), 1) << "__host.magic_knows 要看得见刚学的那一门";
    EXPECT_EQ(state().flag("t.count"), 1);
    EXPECT_TRUE(state().knowsMagic(kKnownMagic)) << "落到 GameState 上了才算数";
}

TEST_F(Ch04MagicScriptTest, LearningTheSameOneTwiceIsIdempotentAndStillReportsSuccess) {
    runScript("t/learn_twice.lua");
    EXPECT_EQ(state().flag("t.ok"), 1) << "契约第 1.3 节：已会则什么都不做，仍回填 ok = true";
    EXPECT_EQ(state().flag("t.count"), 1) << "会的还是一门，不是两门";
    EXPECT_EQ(state().learnedMagics.size(), 1u);
}

TEST_F(Ch04MagicScriptTest, LearningAMagicThatIsNotInDataIsRefusedOutRight) {
    // 契约第 1.3 节点名要的负向用例：**不要静默收下**。拼错的法术 id 会让玩家
    // 在战斗里看着一个空菜单，而那时已经很难追回是谁写错的。
    //
    // 先验：这个 id 确实不在 data 里，否则这条测试证明的是别的事。
    ASSERT_EQ(app_.data().findMagic(kBogusMagic), nullptr);

    runScript("t/learn_bogus.lua");
    EXPECT_EQ(state().flag("t.ok"), 0) << "learn 一个不存在的 id 必须回填 ok = false";
    EXPECT_EQ(state().flag("t.no_magic"), 1) << "原因码要是 no_magic";
    EXPECT_EQ(state().flag("t.count"), 0);
    EXPECT_TRUE(state().learnedMagics.empty()) << "被拒的一次不该在清单上留下任何痕迹";
}

TEST_F(Ch04MagicScriptTest, ForgetDistinguishesForgettingFromNeverHavingKnown) {
    runScript("t/forget.lua");
    EXPECT_EQ(state().flag("t.first"), 1) << "真忘掉了一门";
    EXPECT_EQ(state().flag("t.again"), 0) << "本来就不会的，不该报成「忘了」";
    EXPECT_EQ(state().flag("t.not_learned"), 1) << "原因码要是 not_learned";
    EXPECT_EQ(state().flag("t.count"), 0);
}

// ---------------------------------------------------------------------------
// 四、战斗：学会之后菜单里真的点得出来
//
// 契约第 4 节第 3 条：「走 issuePlayerAction，不是只看数据」。所以下面走的是
// 玩家真会走的那条路——开菜单 → 施法 → 挑法术 → 挑目标 → 确认，
// 而 menuChoose 的每一条路最终都收在 issueFromMenu → issuePlayerAction。
// ---------------------------------------------------------------------------

class Ch04MagicBattleTest : public ::testing::Test {
protected:
    static std::string assetRoot() {
        for (const char* candidate : {".", "..", "../..", "../../.."}) {
            if (fs::exists(fs::path(candidate) / "data" / "battles" / "b03_gu_wai_elang.json")) {
                return candidate;
            }
        }
        return ".";
    }

    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
        // 开局的 10 点气血一个照面就会被放倒，那样测的就成了「战斗早结束了」。
        // 境界也要给够：火球术 needRealm 是炼气一层，凡人施展不出来。
        app_.state().maxHp = 240;
        app_.state().hp = 240;
        app_.state().maxMp = 60;
        app_.state().mp = 60;
        app_.state().realm = Realm::QiRefining3;
    }
    void TearDown() override { app_.shutdown(); }

    static int indexOfLabel(const fanren::ui::ListView& list, const std::string& label) {
        for (int i = 0; i < list.count(); ++i) {
            if (list.items()[static_cast<std::size_t>(i)].label == label) return i;
        }
        return -1;
    }

    static bool logHas(const fanren::core::battle::BattleState& battle, const std::string& needle) {
        return std::any_of(
            battle.log().begin(), battle.log().end(),
            [&needle](const std::string& line) { return line.find(needle) != std::string::npos; });
    }

    Application app_;
};

TEST_F(Ch04MagicBattleTest, WithNothingLearnedTheCastRowIsGreyedOutAndSaysWhy) {
    // 先验 / 对照组。没有它，下一条的「亮了」可能只是因为这一项一直是亮的。
    ASSERT_TRUE(app_.state().learnedMagics.empty());

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    scene.openMenu(app_);
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Root);

    const fanren::ui::ListItem& cast =
        scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)];
    EXPECT_FALSE(cast.enabled);
    // 先验理由真有内容，再验它说的是哪一件事——只写后一句的话，理由字符串
    // 变成空的时候这条断言照样绿（README 那张表的第二行）。
    ASSERT_FALSE(cast.disabledReason.empty());
    EXPECT_NE(cast.disabledReason.find("还没学过"), std::string::npos) << cast.disabledReason;
}

TEST_F(Ch04MagicBattleTest, TheLearnedMagicReachesTheBattlefieldAndCanActuallyBeCast) {
    ASSERT_TRUE(app_.state().learnMagic(kKnownMagic));

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);

    // 登记上了：韩立那份清单进了本场的法术表（登记范围 = 场上各单位声明的并集）。
    ASSERT_TRUE(scene.battle().hasMagic(kKnownMagic))
        << "学会的法术没进本场登记表，菜单里就永远见不到它";
    ASSERT_EQ(scene.battle().units()[0].magics.size(), 1u) << "韩立是 0 号单位";
    EXPECT_EQ(scene.battle().units()[0].magics[0], kKnownMagic);
    EXPECT_TRUE(scene.battle().units()[0].magicsExhaustive)
        << "第 3 章定的口径：声明几条就是几条，不许退回「战场登记的都会」";

    const int mpBefore = scene.battle().units()[0].mp;
    // 走真实行动序：狼比韩立快（身法 9 对 5），先让它们出手。横版没有射程，
    // 第一次轮到他就放得出来。
    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0) << "战斗先结束了，这一条就什么也没测到";
    scene.openMenu(app_);
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Root);

    const fanren::ui::ListItem& castRow =
        scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)];
    ASSERT_TRUE(castRow.enabled) << "学过了，这一项就该亮：" << castRow.disabledReason;
    ASSERT_TRUE(scene.menuChoose(app_, kBattleMenuCast));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Magic);

    const int row = indexOfLabel(scene.menuList(), kKnownMagicName);
    ASSERT_GE(row, 0) << "施法列表里找不到刚学会的那一门";
    const fanren::ui::ListItem& magicRow =
        scene.menuList().items()[static_cast<std::size_t>(row)];
    ASSERT_TRUE(magicRow.enabled) << "横版没有射程，轮到他就该放得出来：" << magicRow.disabledReason;

    ASSERT_TRUE(scene.menuChoose(app_, row));
    ASSERT_EQ(scene.menuMode(), BattleMenuMode::Target);
    int target = -1;
    for (int i = 0; i < scene.menuList().count(); ++i) {
        const fanren::ui::ListItem& item = scene.menuList().items()[static_cast<std::size_t>(i)];
        if (item.enabled && item.label != "返回") {
            target = i;
            break;
        }
    }
    ASSERT_GE(target, 0);
    ASSERT_TRUE(scene.menuChoose(app_, target));

    EXPECT_TRUE(logHas(scene.battle(), kKnownMagicName)) << "战斗日志上没有这一击";
    const fanren::core::Magic* magic = app_.data().findMagic(kKnownMagic);
    ASSERT_NE(magic, nullptr);
    // 法力真的扣了：只看日志的话，一条「写日志但什么都没做」的实现照样全绿。
    EXPECT_EQ(scene.battle().units()[0].mp, mpBefore - magic->needMp);
}

TEST_F(Ch04MagicBattleTest, ForgettingItTakesItBackOffTheBattlefield) {
    // 对照的另一半：learn 之后能用，forget 之后就不能。
    // 两边都验，才排得掉「菜单其实与这份清单无关」这种可能。
    ASSERT_TRUE(app_.state().learnMagic(kKnownMagic));
    ASSERT_TRUE(app_.state().forgetMagic(kKnownMagic));

    BattleScene scene("b03_gu_wai_elang");
    scene.onEnter(app_);
    EXPECT_FALSE(scene.battle().hasMagic(kKnownMagic));

    const int actor = scene.runToAllyTurn();
    ASSERT_GE(actor, 0);
    scene.openMenu(app_);
    const fanren::ui::ListItem& cast =
        scene.menuList().items()[static_cast<std::size_t>(kBattleMenuCast)];
    EXPECT_FALSE(cast.enabled);
    ASSERT_FALSE(cast.disabledReason.empty());
}

}  // namespace
