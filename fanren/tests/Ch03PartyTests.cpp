// P3 第 3 章：队伍与存档 v3（契约 docs/interfaces-p3-ch03.md 第 1.1 / 1.2 节）。
//
// 本文件只碰 core 与 io，不开 Application：队伍的状态与落盘是两件能单独钉住的事，
// 脚本 API 与战斗编成各有自己的测试文件。
//
// 老存档那几条用**手拼的 JSON**，不借 saveGame() 生成：saveGame 永远写当前版本，
// 拿它是造不出一份 v2 档的，而「v2 档还读不读得回来」正是这一批最要紧的判据。
// 手拼就要自己算校验和，于是第一条用例先验证「手拼 + 自算校验和」这套夹具在
// **当前版本**上确实能被接受——夹具本身若是坏的，后面每一条都会红，不会静悄悄
// 地全绿（判据空转的第一种长相：先验判据本身没被验过）。
#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "TempDir.h"
#include "core/model/Types.h"
#include "core/rules/Realm.h"
#include "io/SaveFile.h"

namespace {

using fanren::core::GameState;
using fanren::core::PartyMember;
using fanren::io::kSaveVersion;

// ---- 手拼存档的夹具 ----
//
// 校验和覆盖 "版本号|payload.dump()"，而 dump() 输出的是**按 key 字节序排好的
// 紧凑形**。所以下面的 payload 必须自己就写成那个样子，算出来的校验和才对得上
// 读档时重算的那一个。键序错一个字，这份档就会被判成「可能被手动修改过」。
std::uint64_t fnv1a64(const std::string& data) {
    std::uint64_t hash = 0xcbf29ce484222325ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

std::string toHex16(std::uint64_t v) {
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << v;
    return oss.str();
}

// 一份 payload 里与境界、气血、法力有关的那几个数。v3→v4 的迁移动的正是它们，
// 所以要能逐条摆布；其余字段对那条迁移毫无影响，留在 canonicalPayload 里写死。
struct PayloadVitals {
    int realm = 4;      // 炼气四层
    int hp = 30;
    int maxHp = 42;
    int mp = 5;
    int maxMp = 8;
};

// partyJson 为空表示这份 payload 根本没有 party 字段——v2 及更早的档就是这样。
std::string canonicalPayload(const std::string& partyJson, PayloadVitals v = {}) {
    std::string p = "{";
    p += R"("bag":[],)";
    p += R"("chapter":3,)";
    p += R"("cultivation":12,)";
    p += R"("day":40,)";
    p += R"("facing":2,)";
    p += R"("flags":{},)";
    p += R"("hp":)" + std::to_string(v.hp) + ",";
    p += R"("mapId":"ch03_mishi",)";
    p += R"("maxHp":)" + std::to_string(v.maxHp) + ",";
    p += R"("maxMp":)" + std::to_string(v.maxMp) + ",";
    p += R"("mp":)" + std::to_string(v.mp) + ",";
    if (!partyJson.empty()) p += R"("party":)" + partyJson + ",";
    p += R"("playSecondsGameplay":0.0,)";
    p += R"("playSecondsSystem":0.0,)";
    p += R"("position":{"x":3,"y":4},)";
    p += R"("realm":)" + std::to_string(v.realm);
    p += "}";
    return p;
}

std::string saveFileText(int version, const std::string& payload) {
    const std::string checksum = toHex16(fnv1a64(std::to_string(version) + "|" + payload));
    return R"({"save_version":)" + std::to_string(version) + R"(,"checksum":")" + checksum +
           R"(","payload":)" + payload + "}";
}

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << text;
}

// ---- 队伍本身 ----

TEST(Ch03Party, AddIsIdempotentAndKeepsTheFirstEntry) {
    GameState state;
    EXPECT_TRUE(state.partyAdd("qu_hun"));
    ASSERT_EQ(state.party.size(), 1u);
    state.party[0].hp = 120;   // 打过一场，带伤

    EXPECT_FALSE(state.partyAdd("qu_hun")) << "已在队里就什么都不做";
    ASSERT_EQ(state.party.size(), 1u) << "重复入队不该多出一个人";
    EXPECT_EQ(state.party[0].hp, 120) << "幂等是「什么都不做」，不是「重置成满血」";
    EXPECT_TRUE(state.partyHas("qu_hun"));
    EXPECT_FALSE(state.partyHas("zhang_tie"));
}

TEST(Ch03Party, RemoveReportsWhetherAnyoneActuallyLeft) {
    GameState state;
    ASSERT_TRUE(state.partyAdd("qu_hun"));

    EXPECT_TRUE(state.partyRemove("qu_hun"));
    EXPECT_EQ(state.party.size(), 0u);
    // 负向：本来就不在队里的人「离队」必须报 false，否则脚本分不出
    // 「他走了」与「他压根没来过」。
    EXPECT_FALSE(state.partyRemove("qu_hun"));
    EXPECT_FALSE(state.partyRemove("zhang_tie"));
}

TEST(Ch03Party, AnEmptyRoleIdIsRefused) {
    GameState state;
    EXPECT_FALSE(state.partyAdd(""));
    EXPECT_EQ(state.party.size(), 0u);
}

// ---- 存档往返 ----

TEST(Ch03PartySave, RoundTripsEveryFieldOfEveryMember) {
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_party_save", ".json");

    GameState saved;
    saved.mapId = "ch03_andao";
    saved.hp = 30;
    saved.maxHp = 42;
    // 四种取值各占一格：hp = -1（按模板满血）、具体血量、active 的两种。
    saved.party.push_back(PartyMember{"qu_hun", -1, true});
    saved.party.push_back(PartyMember{"zhang_tie", 17, false});

    auto wrote = fanren::io::saveGame(saved, path.string());
    ASSERT_TRUE(wrote.ok) << wrote.error;

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    const GameState& back = read.value;

    ASSERT_EQ(back.party.size(), 2u);
    EXPECT_EQ(back.party[0].roleId, "qu_hun");
    EXPECT_EQ(back.party[0].hp, -1) << "-1 是「按模板满血」，不能被读成 0 或满血的具体数";
    EXPECT_TRUE(back.party[0].active);
    EXPECT_EQ(back.party[1].roleId, "zhang_tie");
    EXPECT_EQ(back.party[1].hp, 17);
    EXPECT_FALSE(back.party[1].active) << "留着位置不上场的同伴不能被读成参战";

    std::filesystem::remove(path);
}

TEST(Ch03PartySave, AnEmptyPartyStaysEmpty) {
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_party_empty", ".json");

    GameState saved;
    saved.mapId = "ch03_mishi";
    auto wrote = fanren::io::saveGame(saved, path.string());
    ASSERT_TRUE(wrote.ok) << wrote.error;

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    EXPECT_TRUE(read.value.party.empty());

    std::filesystem::remove(path);
}

TEST(Ch03PartySave, AMemberWithoutARoleIdIsDroppedInsteadOfLoadedAsABlank) {
    // 负向：一份手改坏的存档。没有 roleId 的队员在战斗里是个找不到模板的空位，
    // 静默收下只会把症状推迟到开打那一刻。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_party_blank", ".json");
    const std::string payload =
        canonicalPayload(R"([{"active":true,"hp":-1,"roleId":""},)"
                         R"({"active":true,"hp":-1,"roleId":"qu_hun"}])");
    writeText(path, saveFileText(kSaveVersion, payload));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    ASSERT_EQ(read.value.party.size(), 1u) << "空 roleId 的那一条应当被丢掉";
    EXPECT_EQ(read.value.party[0].roleId, "qu_hun");

    std::filesystem::remove(path);
}

// ---- 版本与迁移 ----

TEST(Ch03SaveVersion, IsFive) {
    // 契约第 1.2 节把版本推到 3（队伍进存档）。
    //
    // **改成 4**：maxHp / maxMp 改由境界给出基准（core/rules/Realm.h）。字段一个也
    // 没增减，变的是同一个 maxHp 在同一个境界下该是多少——一份炼气三层的 v3 档里
    // 它是 10，到了 v4 该是 60。那同样是不兼容变化，照 SaveFile.h 开头写的规矩升
    // 版本、补迁移。这也是本作第一条真的做事的迁移（1→2、2→3 都是空操作）。
    // **再推到 5**：已习得法术进存档
    // （GameState::learnedMagics，第 4 章契约 1.2 节）。
    // v4 本就没有这一项，所以 4→5 是空操作——
    // 但仍然必须登记，理由见下一条用例。
    // **再推到 6**：剧情给的境界上限；**再推到 7**：已揭开的破绽（八方旅人化改造，
    // io/SaveFile.h 第 7 条）。6→7 同样是空操作、同样登记。**再推到 8**：野外遭遇的计数器
    //（io/SaveFile.h 第 8 条），7→8 也是空操作。
    // 第 9 章 E1/E2 的 formerRealm 将当前版本推到 9，8→9 仍登记空迁移。
    EXPECT_EQ(kSaveVersion, 9);
}

TEST(Ch03SaveVersion, HandBuiltSaveAtTheCurrentVersionIsAccepted) {
    // 先验判据：证明「手拼 JSON + 自算校验和」这套夹具是对的。
    // 这一条要是红的，下面几条的绿灯一个都不作数。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_ver_now", ".json");
    writeText(path, saveFileText(kSaveVersion, canonicalPayload(
                                                   R"([{"active":true,"hp":-1,"roleId":"qu_hun"}])")));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    EXPECT_EQ(read.value.mapId, "ch03_mishi");
    ASSERT_EQ(read.value.party.size(), 1u);
    EXPECT_EQ(read.value.party[0].roleId, "qu_hun");

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, EveryOlderVersionStillHasAMigrationRegistered) {
    // 契约第 1.2 节要求登记 2→3。这里不是只钉住 2 这一个点，而是扫一片：
    // 从 v1 到 v(当前-1) 每一个版本都必须能读回来。谁日后把 kSaveVersion 加到 4
    // 却忘了登记 3→4，这条用例立刻转红——而「忘了登记」的症状在真机上是
    // 老玩家的存档一份也读不回来，那时才发现就晚了。
    ASSERT_GT(kSaveVersion, 1);
    for (int version = 1; version < kSaveVersion; ++version) {
        const auto path = fanren::test::uniqueTempPath("fanren_ch03_ver_old", ".json");
        // 老档里根本没有 party 字段：v2→v3 的迁移之所以是空操作，正因为如此。
        writeText(path, saveFileText(version, canonicalPayload({})));

        auto read = fanren::io::loadGame(path.string());
        EXPECT_TRUE(read.ok) << "v" << version << " 的存档读不回来了: " << read.error;
        if (read.ok) {
            EXPECT_TRUE(read.value.party.empty()) << "老档迁上来应当是空队伍，不是凭空多个人";
            EXPECT_EQ(read.value.chapter, 3) << "迁移不该动与它无关的字段";
            EXPECT_EQ(read.value.mapId, "ch03_mishi");
            EXPECT_EQ(read.value.cultivation, 12);
            // 气血与法力**不再**属于「迁移不该动」的那一类：3→4 的迁移就是为它们
            // 而存在的（这份 payload 是炼气四层，maxHp 却还是老口径的 42）。
            // 原来这里写的是 EXPECT_EQ(read.value.hp, 30) <<「迁移不该动既有字段」，
            // 那句话在 1→2、2→3 全是空操作的年代成立，现在必须让位：让它留着，
            // 等于把「老档读进来仍然停在 10 点气血」这个缺陷钉成正确行为。
            // 补齐气血法力的是 3→4 那一步，它只对 **v3 及更早** 跑。
            // v4 及以后的档本就带着自己的数，不再被抬一次，
            // 所以这两句到 v4 就该停。不加这道守卫的话，
            // 它会逐步变成「每一条新迁移都得顺手再补一次气血」，
            // 而那是为了迱就测试去写代码。
            if (version < 4) {
                EXPECT_EQ(read.value.maxHp,
                          fanren::rules::realmMaxHp(fanren::rules::Realm::QiRefining4));
                EXPECT_EQ(read.value.maxMp,
                          fanren::rules::realmMaxMp(fanren::rules::Realm::QiRefining4));
            }
        }
        std::filesystem::remove(path);
    }
}

// ---- v3 → v4：老存档拿得到境界该给的属性 ----
//
// 缺陷 B 的存档那一半。真实的老存档长什么样：玩家从第 1 章练到第 3 章的炼气三层，
// 而 maxHp 从头到尾没人动过，所以还是 GameState 的默认值 10。这几条钉的就是
// 「这样一份档读进来会拿到什么」。

// 一份炼气三层、气血还停在 10 点的老档——第 3 章结束时每一位老玩家手里那一份。
PayloadVitals oldSaveAtQiRefiningThree() {
    PayloadVitals v;
    v.realm = fanren::rules::toValue(fanren::rules::Realm::QiRefining3);
    v.hp = 10;
    v.maxHp = 10;   // GameState 的默认值，全作没有任何一处动过它
    v.mp = 0;
    v.maxMp = 0;
    return v;
}

TEST(Ch03SaveVersion, ASaveAlreadyAtTheCurrentVersionIsNotTouched) {
    // **先验 / 对照组。** 同一份 payload，写成当前版本：不走任何迁移，于是那 10 点
    // 原样读回来。两件事一起证明了：
    //   1. 手拼的这份 payload 与校验和是对的（否则这里就红在「校验和不匹配」上），
    //   2. 干活的是 3→4 那条迁移，不是 fromJson 里某处顺手做的补齐 —— 下一条用例
    //      的绿灯因此有了来处。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_v4_intact", ".json");
    writeText(path, saveFileText(kSaveVersion, canonicalPayload({}, oldSaveAtQiRefiningThree())));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    EXPECT_EQ(read.value.realm, fanren::rules::Realm::QiRefining3);
    EXPECT_EQ(read.value.maxHp, 10) << "v4 档不该被再迁一次";
    EXPECT_EQ(read.value.maxMp, 0);

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, AnOldSaveArrivesWithTheAttributesItsRealmOwesIt) {
    // 正题：同一份 payload，只把版本号写成 3（并按 v3 重算校验和），就必须被
    // 3→4 那条迁移补齐。数值写成对 rules 层的调用而不是字面量 60/30：曲线日后
    // 调平衡时，该红的是 tests/RealmTests.cpp 里钉着曲线的那一组，不是这一条。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_v3_old", ".json");
    writeText(path, saveFileText(3, canonicalPayload({}, oldSaveAtQiRefiningThree())));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    const int owedHp = fanren::rules::realmMaxHp(fanren::rules::Realm::QiRefining3);
    const int owedMp = fanren::rules::realmMaxMp(fanren::rules::Realm::QiRefining3);
    ASSERT_GT(owedHp, 10) << "先验：炼气三层该给的气血必须真的多于那 10 点，"
                             "否则下面几条断言在任何实现下都成立";
    ASSERT_GT(owedMp, 0);

    EXPECT_EQ(read.value.maxHp, owedHp) << "炼气三层的老档读进来还停在 " << read.value.maxHp
                                        << " 点气血 —— 修为就还只是一道剧情闸门";
    EXPECT_EQ(read.value.maxMp, owedMp);
    // 补上去的那一截是根基不是伤势：存档里他是满血的，读回来也该是满血的。
    EXPECT_EQ(read.value.hp, owedHp) << "只抬上限的话他会带着 10/" << owedHp
                                     << " 醒过来，一进战斗就死";
    EXPECT_EQ(read.value.mp, owedMp);
    // 迁移只管这四个数，别的一律照旧。
    EXPECT_EQ(read.value.cultivation, 12);
    EXPECT_EQ(read.value.day, 40);
    EXPECT_EQ(read.value.mapId, "ch03_mishi");

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, TheMigrationOnlyEverLiftsAndNeverCuts) {
    // 负向那一侧：上限已经高过境界基准的档，一个字节也不许被削。
    // 「读档时按境界重算」会在这里把多出来的那一截抹掉，而那种损失在存档里
    // 看不出来、也找不回来（日后的丹药、功法、装备都要从这条路上过）。
    PayloadVitals rich = oldSaveAtQiRefiningThree();
    rich.maxHp = 500;
    rich.hp = 320;
    rich.maxMp = 400;
    rich.mp = 111;

    const auto path = fanren::test::uniqueTempPath("fanren_ch03_v3_rich", ".json");
    writeText(path, saveFileText(3, canonicalPayload({}, rich)));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    ASSERT_GT(rich.maxHp, fanren::rules::realmMaxHp(fanren::rules::Realm::QiRefining3))
        << "先验：这份档的上限真的高过基准，否则这条用例什么也没测";
    EXPECT_EQ(read.value.maxHp, 500);
    EXPECT_EQ(read.value.hp, 320);
    EXPECT_EQ(read.value.maxMp, 400);
    EXPECT_EQ(read.value.mp, 111);

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, AMortalOldSaveIsLeftExactlyWhereItWas) {
    // 第 1、2 章的存档：凡人，10 点气血。境界基准在凡人一档正是 10，所以这条
    // 迁移对前两章的老档是不折不扣的空操作 —— 那两章一场战斗都没有，不该被动。
    PayloadVitals mortal = oldSaveAtQiRefiningThree();
    mortal.realm = fanren::rules::toValue(fanren::rules::Realm::Mortal);
    mortal.hp = 7;

    const auto path = fanren::test::uniqueTempPath("fanren_ch03_v3_mortal", ".json");
    writeText(path, saveFileText(3, canonicalPayload({}, mortal)));

    auto read = fanren::io::loadGame(path.string());
    ASSERT_TRUE(read.ok) << read.error;
    EXPECT_EQ(read.value.maxHp, 10);
    EXPECT_EQ(read.value.hp, 7) << "凡人档一个字节都不该被动";
    EXPECT_EQ(read.value.maxMp, 0);

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, AVersionAboveTheCurrentOneIsRefused) {
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_ver_future", ".json");
    writeText(path, saveFileText(kSaveVersion + 1, canonicalPayload({})));

    auto read = fanren::io::loadGame(path.string());
    EXPECT_FALSE(read.ok) << "比当前新的存档必须挡下，不能半懂不懂地读进来";

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, SwappingTheVersionNumberAloneIsRefused) {
    // 负向：校验和把版本号一起绑了进去，所以「拿一份合法的 v3 档改个版本号
    // 来绕过迁移」行不通。没有这一条，上面那条 v1..v2 全绿只能说明迁移表非空，
    // 说明不了版本号本身是被保护的。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_ver_swap", ".json");
    const std::string payload = canonicalPayload({});
    std::string text = saveFileText(kSaveVersion, payload);
    const std::string from = R"("save_version":)" + std::to_string(kSaveVersion);
    const std::string to = R"("save_version":)" + std::to_string(kSaveVersion - 1);
    const std::size_t at = text.find(from);
    ASSERT_NE(at, std::string::npos);
    text.replace(at, from.size(), to);
    writeText(path, text);

    auto read = fanren::io::loadGame(path.string());
    EXPECT_FALSE(read.ok) << "改了版本号却没改校验和，必须被判成手改存档";

    std::filesystem::remove(path);
}

TEST(Ch03SaveVersion, ATamperedPayloadIsRefused) {
    // 负向对照，防的是「校验和这道闸其实什么都没查」：同一套夹具，只把气血
    // 从 30 改成 99 而不重算校验和，必须被挡下。它与上一条正向用例只差这一处。
    const auto path = fanren::test::uniqueTempPath("fanren_ch03_ver_tamper", ".json");
    std::string text = saveFileText(kSaveVersion, canonicalPayload({}));
    const std::size_t at = text.find(R"("hp":30)");
    ASSERT_NE(at, std::string::npos);
    text.replace(at, std::string(R"("hp":30)").size(), R"("hp":99)");
    writeText(path, text);

    auto read = fanren::io::loadGame(path.string());
    EXPECT_FALSE(read.ok) << "改过的 payload 必须被校验和抓住";

    std::filesystem::remove(path);
}

}  // namespace
