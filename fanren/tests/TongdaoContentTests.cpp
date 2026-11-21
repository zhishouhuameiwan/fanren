// 第 7 章同道录 2a·路 N（内容与美术）的验收：docs/ch07-tongdaolu-n.md，施工图 docs/ch07-tongdaolu.md 3.1–3.6。
//
//   * 三位的伙伴表读得进来：菡云芝入册（met_flag 借 ch07.wuyou）、各档带的法术与被动、合招、在册加成、切磋；
//   * 数值护栏：单门威力不超过韩立同阶的法术、合招不超过环火合击、追击加成不超过三成、回春符一场的回血有上限；
//   * 切磋编成读得进来，她的角色带架势与破绽；从「刚出禁地」那份检查点起，二档时韩立一个人打得完、也打得赢；
//   * 11 段叙话、3 段请教在无头模式下各跑得通：每句文案都有、四个表情都用上、不超过 8 句、记对了账；
//   * 事件图的表与图对得上（形状、sha256、2560×1440），陈巧倩五档那一段真的铺了图又撤了。
// 走的都是真数据（data/、scripts/、assets/），不另造探针。
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../vendor/json.hpp"

#include "BattleHand.h"
#include "ChapterFixture.h"
#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "core/rules/Companions.h"
#include "core/rules/Roster.h"
#include "core/rules/Tongdao.h"
#include "game/Application.h"
#include "game/BattleScene.h"
#include "io/SaveFile.h"
#include "script/Command.h"

namespace {

namespace fs = std::filesystem;
using fanren::core::BondLine;
using fanren::core::Companion;
using fanren::core::GameState;
using fanren::core::Magic;
using fanren::core::MagicEffect;
using fanren::core::PerkKind;
using fanren::core::battle::BattlePhase;
using fanren::game::Application;
using fanren::game::BattleScene;
namespace rules = fanren::rules;
using json = nlohmann::json;

constexpr const char* kChen = "chen_qiaoqian";
constexpr const char* kWan = "nangong_wan";
constexpr const char* kHan = "han_yunzhi";
constexpr std::array<const char*, 3> kThree{kChen, kWan, kHan};

std::string assetRoot() {
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "companions" / "han_yunzhi.json")) return candidate;
    }
    return ".";
}

std::string readFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}

// ---- sha256（只给事件图对账用；与 tools 里 hashlib 的结果逐字节相同）----
std::string sha256Hex(const std::string& data) {
    static constexpr std::array<std::uint32_t, 64> k{
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    std::array<std::uint32_t, 8> h{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    std::string msg = data;
    const std::uint64_t bits = static_cast<std::uint64_t>(data.size()) * 8u;
    msg.push_back(static_cast<char>(0x80));
    while (msg.size() % 64 != 56) msg.push_back('\0');
    for (int i = 7; i >= 0; --i) msg.push_back(static_cast<char>((bits >> (i * 8)) & 0xffu));
    const auto rotr = [](std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
    for (std::size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        std::array<std::uint32_t, 64> w{};
        for (int i = 0; i < 16; ++i) {
            const auto byte = [&](int j) {
                return static_cast<std::uint32_t>(static_cast<unsigned char>(msg[chunk + static_cast<std::size_t>(i * 4 + j)]));
            };
            w[static_cast<std::size_t>(i)] = (byte(0) << 24) | (byte(1) << 16) | (byte(2) << 8) | byte(3);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        auto v = h;
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr(v[4], 6) ^ rotr(v[4], 11) ^ rotr(v[4], 25);
            const std::uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
            const std::uint32_t t1 = v[7] + s1 + ch + k[i] + w[i];
            const std::uint32_t s0 = rotr(v[0], 2) ^ rotr(v[0], 13) ^ rotr(v[0], 22);
            const std::uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            const std::uint32_t t2 = s0 + maj;
            v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
            v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
        }
        for (std::size_t i = 0; i < 8; ++i) h[i] += v[i];
    }
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    for (const std::uint32_t word : h) {
        for (int i = 28; i >= 0; i -= 4) out.push_back(kHex[(word >> i) & 0xfu]);
    }
    return out;
}

TEST(TongdaoContentSha, TheLocalSha256MatchesKnownVectors) {
    EXPECT_EQ(sha256Hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

class TongdaoContent : public ::testing::Test {
protected:
    void SetUp() override {
        auto ready = app_.init(assetRoot(), /*headless=*/true);
        ASSERT_TRUE(ready.ok) << ready.error;
    }
    void TearDown() override { app_.shutdown(); }

    GameState& state() { return app_.state(); }
    const std::map<std::string, Companion>& companions() { return app_.data().companions; }
    const Companion& companion(const std::string& id) { return companions().at(id); }
    void tick(int frames = 1) {
        for (int i = 0; i < frames; ++i) app_.tick(1.0 / 60.0);
    }
    // 「刚出禁地」那份检查点（默契照主线的账：只有南宫婉 37），当作这一局。
    void startFromChujindi() {
        const auto loaded = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), "companions/ch07-chujindi-first.sav").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        state() = loaded.value;
        ASSERT_TRUE(rules::rosterEffectsActive(state())) << "先验：第 7 章里";
    }
    void setBond(const std::string& id, int value) { state().bonds[id].value = value; }

    // 跑一段事件脚本到底：对白一路按确认，选择取第一项。返回说过的 key；途中铺过的事件图记进 cgSeen_。
    std::vector<std::string> runEvent(const std::string& path) {
        app_.clearSpokenKeys();
        logBefore_ = app_.dialogueLog().size();
        cgSeen_.clear();
        const auto started = app_.startEvent(path);
        EXPECT_TRUE(started.ok) << path << "：" << started.error;
        std::vector<std::string> spoken;
        for (int frame = 0; frame < 4000; ++frame) {
            tick();
            if (!app_.currentCg().empty()) cgSeen_.insert(app_.currentCg());
            if (app_.awaitingCommand()) {
                spoken.insert(spoken.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
                app_.clearSpokenKeys();
                fanren::script::CommandResult result;
                result.ok = true;
                result.choiceIndex = 0;
                app_.completeCommand(result);
                if (app_.topScene() != nullptr && app_.topScene()->name() == "Dialogue") app_.popScene();
                continue;
            }
            if (!app_.scripts().isRunning()) break;
        }
        EXPECT_FALSE(app_.scripts().isRunning()) << path << "：脚本没能跑到结束";
        EXPECT_TRUE(app_.scripts().lastError().empty()) << path << "：" << app_.scripts().lastError();
        spoken.insert(spoken.end(), app_.spokenKeys().begin(), app_.spokenKeys().end());
        return spoken;
    }
    // 这一段里她（说话人是她那个角色）用过的表情。
    std::set<std::string> facesOf(const std::string& roleId) const {
        std::set<std::string> out;
        const auto& log = app_.dialogueLog();
        for (std::size_t i = logBefore_; i < log.size(); ++i) {
            if (log[i].roleId == roleId) out.insert(log[i].face.empty() ? "normal" : log[i].face);
        }
        return out;
    }

    Application app_;
    std::size_t logBefore_ = 0;
    std::set<std::string> cgSeen_;
};

// ---------------------------------------------------------------------------
// 伙伴表
// ---------------------------------------------------------------------------

TEST_F(TongdaoContent, HanYunzhiJoinsTheRosterOnTheFlagSetAfterTheStoneHallOnEitherChoice) {
    ASSERT_EQ(companions().count(kHan), 1u) << "data/companions/han_yunzhi.json";
    const Companion& han = companion(kHan);
    EXPECT_EQ(han.kind, fanren::core::CompanionKind::Person);
    EXPECT_EQ(han.line, BondLine::Moni) << "施工图 3.2：只写朋友之谊，最高一档「莫逆」";
    EXPECT_EQ(han.metFlag, "ch07.wuyou") << "借小石殿末尾本来就置的旗，不新置旗";
    ASSERT_EQ(han.roles.size(), 1u);
    EXPECT_EQ(han.roles[0].roleId, "han_yunzhi");
    EXPECT_EQ(rules::currentRole(companions(), kHan, state()), "han_yunzhi");

    // 小石殿二选一：1 让她忘了这半日、2 先让她立誓（取消按 1）。两条都非零，都算入册；之前不算。
    EXPECT_FALSE(rules::inRoster(han, state()));
    for (const int pick : {1, 2}) {
        state().setFlag("ch07.wuyou", pick);
        EXPECT_TRUE(rules::inRoster(han, state())) << "ch07.wuyou = " << pick;
    }
    // 交接档两侧（第一侧忘、第二侧立誓）里它都非零：第 8 章起读档同道录里也有她。
    for (const char* side : {"first", "second"}) {
        const auto loaded = fanren::io::loadGame(
            fanren::test::chapterFixturePath(assetRoot(), std::string("ch07-end-") + side + ".sav").string());
        ASSERT_TRUE(loaded.ok) << loaded.error;
        EXPECT_NE(loaded.value.flag("ch07.wuyou"), 0) << side;
    }
    // 喜好都是现成的物品。
    for (const auto* list : {&han.giftsLike, &han.giftsDislike}) {
        for (const std::string& item : *list) EXPECT_NE(app_.data().findItem(item), nullptr) << item;
    }
}

TEST_F(TongdaoContent, TheThreeCarryTheirTierSpellsPassivesAndDuosOnTheirChapterSevenRoles) {
    struct Want {
        const char* id;
        const char* role;
        const char* tier3;
        const char* tier4;
        PerkKind passive;
        std::vector<int> values;
        const char* duo;   // 写在这一段上的合招；空 = 用顶层的
    };
    const std::vector<Want> wants{
        {kChen, "chen_shimei", "magic_shuangjian", "magic_jianmu", PerkKind::ChasePower, {20, 30}, "magic_duo_jianren"},
        {kWan, "nangong_wan_ch07", "magic_liuhuo", "magic_huanhu", PerkKind::OpeningBp, {1}, ""},
        {kHan, "han_yunzhi", "magic_fu_huichun", "magic_fu_dingshen", PerkKind::VictoryHeal, {10, 15},
         "magic_duo_lianzhu"},
    };
    for (const Want& w : wants) {
        const auto* entry = rules::companionRoleEntry(companion(w.id), w.role);
        ASSERT_NE(entry, nullptr) << w.id << " / " << w.role;
        ASSERT_EQ(entry->tierMagics.size(), 2u) << w.id;
        EXPECT_EQ(entry->tierMagics[0].tier, 3);
        EXPECT_EQ(entry->tierMagics[0].magicId, w.tier3);
        EXPECT_EQ(entry->tierMagics[1].tier, 4);
        EXPECT_EQ(entry->tierMagics[1].magicId, w.tier4);
        EXPECT_EQ(entry->passive.kind, w.passive) << w.id;
        EXPECT_EQ(entry->passive.value, w.values.front()) << w.id;
        EXPECT_EQ(entry->passive.value5, w.values.back()) << w.id;
        EXPECT_EQ(entry->duo, w.duo) << w.id;

        // 按档：二档什么都不多，三档一门 + 被动，四档两门 + 合招，五档被动加强一级（写了第二级的话）。
        const auto at = [&](int bond) { return rules::companionKit(companion(w.id), w.role, bond, true); };
        EXPECT_TRUE(at(20).extraMagics.empty()) << w.id;
        EXPECT_EQ(at(20).passive.kind, PerkKind::None) << w.id;
        EXPECT_EQ(at(45).extraMagics, std::vector<std::string>{w.tier3}) << w.id;
        EXPECT_EQ(at(45).passive.kind, w.passive) << w.id;
        EXPECT_EQ(at(45).passive.value, w.values.front()) << w.id;
        EXPECT_EQ(at(70).extraMagics, (std::vector<std::string>{w.tier3, w.tier4})) << w.id;
        EXPECT_EQ(at(95).passive.value, w.values.back()) << w.id;
        // 判据不成立（第 7 章以外）一样都不带。
        EXPECT_TRUE(rules::companionKit(companion(w.id), w.role, 95, false).extraMagics.empty()) << w.id;
    }
    // 南宫婉问名之前那一段（主线墨蛟与地下几拍）一个字都没加：主线的仗行为不变。
    const auto* veil = rules::companionRoleEntry(companion(kWan), "baiyi_shaonv");
    ASSERT_NE(veil, nullptr);
    EXPECT_TRUE(veil->tierMagics.empty());
    EXPECT_EQ(veil->passive.kind, PerkKind::None);
    EXPECT_TRUE(veil->duo.empty());
    EXPECT_EQ(companion(kWan).duo, "magic_duo_huanhuo") << "合招照旧挂在顶层";
    // 第 8 章起的南宫婉一段不写（路 K 记录 1.1）。
    const auto* later = rules::companionRoleEntry(companion(kWan), "nangong_wan");
    ASSERT_NE(later, nullptr);
    EXPECT_TRUE(later->tierMagics.empty());
    EXPECT_EQ(later->passive.kind, PerkKind::None);
    // 菡云芝一档的符写在角色表里。
    const auto* role = app_.data().findRole("han_yunzhi");
    ASSERT_NE(role, nullptr);
    EXPECT_EQ(role->magics, std::vector<std::string>{"magic_fu_hanbing"});
}

TEST_F(TongdaoContent, RosterBonusesAreTheThreeOfSectionThreeSix) {
    EXPECT_EQ(companion(kChen).rosterBonus.kind, PerkKind::AmbushBp);
    EXPECT_EQ(companion(kChen).rosterBonus.value, 1);
    EXPECT_EQ(companion(kWan).rosterBonus.kind, PerkKind::ElementStagger);
    EXPECT_EQ(companion(kWan).rosterBonus.value, 1);
    EXPECT_EQ(companion(kWan).rosterBonus.element, fanren::core::kElementFire);
    EXPECT_EQ(companion(kHan).rosterBonus.kind, PerkKind::TalismanPower);
    EXPECT_EQ(companion(kHan).rosterBonus.value, 20);
}

// 数值护栏（施工图 3.3 末）：单门威力不超过主角同阶的法术；合招不超过现有的环火合击；追击加成至多三成。
TEST_F(TongdaoContent, NewSpellsStayUnderTheHerosSameStageSpells) {
    // 韩立出禁地时（检查点 ch07-chujindi-first）会的伤人法术里最重的一门。
    startFromChujindi();
    int heroBest = 0;
    for (const std::string& id : state().learnedMagics) {
        const Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        if (m->effect == MagicEffect::None) heroBest = std::max(heroBest, m->power);
    }
    ASSERT_GE(heroBest, 20) << "先验：青蛟旗 22";
    const Magic* ring = app_.data().findMagic("magic_duo_huanhuo");
    ASSERT_NE(ring, nullptr);
    for (const char* id : {"magic_shuangjian", "magic_liuhuo", "magic_fu_hanbing"}) {
        const Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        EXPECT_LE(m->power, heroBest) << id;
        EXPECT_GT(m->needMp, 0) << id;
    }
    for (const char* id : {"magic_duo_jianren", "magic_duo_lianzhu"}) {
        const Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        EXPECT_LT(m->power, ring->power) << id << "：不超过现有的合招";
    }
    for (const char* id : {"magic_jianmu", "magic_huanhu", "magic_fu_huichun", "magic_fu_dingshen"}) {
        const Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        EXPECT_EQ(m->power, 0) << id;
        EXPECT_NE(m->effect, MagicEffect::None) << id;
    }
    EXPECT_EQ(app_.data().findMagic("magic_shuangjian")->hits, 2);
    EXPECT_LE(rules::companionRoleEntry(companion(kChen), "chen_shimei")->passive.value5, 30) << "追击加成至多三成";
}

// 回春符不许把必打战变成磨不死（施工图 3.3）：她一场能回的气血有上限——法力只靠背包里的药补，
// 战斗里不会自己回；整场上限不到韩立出禁地时气血上限的一倍二。验算的其余部分在施工记录 2.4。
TEST_F(TongdaoContent, TheHealingTalismanHasAPerBattleCeiling) {
    startFromChujindi();
    const Magic* heal = app_.data().findMagic("magic_fu_huichun");
    ASSERT_NE(heal, nullptr);
    EXPECT_EQ(heal->effect, MagicEffect::Heal);
    const auto* role = app_.data().findRole("han_yunzhi");
    ASSERT_NE(role, nullptr);
    const int casts = role->maxMp / heal->needMp;
    EXPECT_EQ(casts, 5);
    EXPECT_LE(casts * heal->amount, state().maxHp * 6 / 5) << "整场回血上限";
    EXPECT_LT(heal->amount * 4, state().maxHp) << "一张回不到韩立气血的四分之一";
}

// ---------------------------------------------------------------------------
// 切磋
// ---------------------------------------------------------------------------

TEST_F(TongdaoContent, TheThreeSparFormationsLoadAndPutHerAcrossWithAStanceToBreak) {
    const std::map<std::string, std::string> roleOf{
        {kChen, "chen_shimei"}, {kWan, "nangong_wan_ch07"}, {kHan, "han_yunzhi"}};
    for (const char* id : kThree) {
        const Companion& c = companion(id);
        EXPECT_EQ(c.sparBattle, std::string("spar_") + id);
        const auto found = app_.battles().find(c.sparBattle);
        ASSERT_NE(found, app_.battles().end()) << c.sparBattle;
        const auto* setup = &found->second;
        EXPECT_TRUE(setup->spar);
        ASSERT_EQ(setup->units.size(), 1u);
        EXPECT_EQ(setup->units[0].roleId, roleOf.at(id));
        const auto* role = app_.data().findRole(roleOf.at(id));
        ASSERT_NE(role, nullptr);
        EXPECT_GT(role->toughness, 0) << id << "：没有架势就打不出破势（路 K 记录 1.4）";
        EXPECT_NE(role->weaknesses, 0) << id;
        // 首胜奖品只许现成的丹药、材料、符。
        ASSERT_FALSE(c.sparPrizes.empty()) << id;
        for (const auto& prize : c.sparPrizes) {
            for (const auto& entry : prize.items) {
                const auto* item = app_.data().findItem(entry.itemId);
                ASSERT_NE(item, nullptr) << entry.itemId;
                EXPECT_TRUE(item->kind == fanren::core::ItemKind::Pill ||
                            item->kind == fanren::core::ItemKind::Material ||
                            item->kind == fanren::core::ItemKind::Talisman)
                    << entry.itemId;
            }
        }
    }
    // 南宫婉第 7 章封在四档：首胜奖品到四档为止。
    EXPECT_EQ(companion(kWan).sparPrizes.back().tier, 4);
}

// 二档时：从刚出禁地那份检查点起（炼气十一层，主线的药与符），那只手（会挑破绽、蓄劲、吃药）一个人打得赢；
// 陈巧倩、南宫婉只会普攻的打不赢——要动脑子（找到土 / 水，蓄劲一击）。菡云芝是辅助、炼气十层，
// 三位里最温和的一场：普攻也打得赢（施工记录 3.3 记着，拿不准，交主会话定）。
// 打完气血法力放回开打前的数（路 K 的口径）。实际的回合与伤害：FANREN_TONGDAO_SPAR_LOG=1 打出战报。
TEST_F(TongdaoContent, AtTierTwoTheHeroAloneWinsEachSparButNotByJustSwinging) {
    for (const char* id : kThree) {
        for (const bool smart : {true, false}) {
            startFromChujindi();
            ASSERT_FALSE(HasFatalFailure());
            setBond(id, 20);
            const int hp = state().hp;
            const int mp = state().mp;
            bool done = false;
            int prizeTier = -1;
            const auto check = app_.startSpar(id, [&](const rules::SparOutcome& o, const fanren::script::CommandResult&) {
                done = true;
                prizeTier = o.prizeTier;
            });
            ASSERT_EQ(check, rules::TongdaoCheck::Ok) << id;
            tick();
            auto* scene = dynamic_cast<BattleScene*>(app_.topScene());
            ASSERT_NE(scene, nullptr) << id;
            ASSERT_TRUE(scene->battle().spar());
            BattlePhase phase = BattlePhase::Ongoing;
            if (smart) {
                fanren::test::HandPolicy policy;
                policy.healAtPercent = 50;
                policy.pills = {"pill_yangjing_dan", "pill_jinchuang_yao"};
                // 动脑子的第一步：火修的她不吃火，火弹不往她身上扔。
                if (std::string(id) == kWan) policy.withheldMagics = {"magic_huodan_shu"};
                fanren::test::BattleHand hand(app_, policy);
                phase = hand.play(*scene);
            } else {
                for (int step = 0; step < 400 && scene->battle().phase() == BattlePhase::Ongoing; ++step) {
                    const int actor = scene->runToAllyTurn();
                    if (actor < 0) break;
                    fanren::core::battle::Action a;
                    a.kind = fanren::core::battle::ActionKind::Attack;
                    a.actorIndex = actor;
                    for (std::size_t i = 0; i < scene->battle().units().size(); ++i) {
                        const auto& u = scene->battle().units()[i];
                        if (!u.ally && u.alive()) a.targetIndex = static_cast<int>(i);
                    }
                    ASSERT_TRUE(scene->issuePlayerAction(app_, a).ok) << id;
                }
                phase = scene->battle().phase();
            }
            ASSERT_NE(phase, BattlePhase::Ongoing) << id << "：打得完";
            const int rounds = scene->battle().round();
            if (fanren::test::environmentFlagSet("FANREN_TONGDAO_SPAR_LOG")) {
                std::cout << "---- " << id << (smart ? " 那只手" : " 只会普攻") << " ----" << std::endl;
                for (const std::string& line : scene->battle().log()) std::cout << "  " << line << std::endl;
            }
            for (int i = 0; i < 32 && dynamic_cast<BattleScene*>(app_.topScene()) != nullptr; ++i) tick();
            EXPECT_TRUE(done) << id;
            if (smart) {
                EXPECT_EQ(phase, BattlePhase::Won) << id << "：二档时一个人打得赢（" << rounds << " 回合）";
                EXPECT_GE(rounds, std::string(id) == kHan ? 2 : 3) << id << "：不是一下就完";
                EXPECT_EQ(prizeTier, 2) << id;
                EXPECT_EQ(state().bondOf(id), 24) << id;
            } else if (std::string(id) == kHan) {
                EXPECT_EQ(phase, BattlePhase::Won) << id << "：她这一场最温和（见上）";
            } else {
                EXPECT_EQ(phase, BattlePhase::Lost) << id << "：只会普攻打不赢";
                EXPECT_EQ(state().bondOf(id), 22) << id << "：输了 +2";
            }
            EXPECT_EQ(state().hp, hp) << id << "：打完放回开打前的数";
            EXPECT_EQ(state().mp, mp) << id;
        }
    }
}

// ---------------------------------------------------------------------------
// 叙话与请教
// ---------------------------------------------------------------------------

struct Scene {
    const char* companion;
    const char* role;
    int tier;   // 0 = 请教
};

const std::vector<Scene>& scenes() {
    static const std::vector<Scene> kScenes{
        {kChen, "chen_shimei", 2},      {kChen, "chen_shimei", 3},      {kChen, "chen_shimei", 4},
        {kChen, "chen_shimei", 5},      {kWan, "nangong_wan_ch07", 2},  {kWan, "nangong_wan_ch07", 3},
        {kWan, "nangong_wan_ch07", 4},  {kHan, "han_yunzhi", 2},        {kHan, "han_yunzhi", 3},
        {kHan, "han_yunzhi", 4},        {kHan, "han_yunzhi", 5},        {kChen, "chen_shimei", 0},
        {kWan, "nangong_wan_ch07", 0},  {kHan, "han_yunzhi", 0},
    };
    return kScenes;
}

std::string scriptOf(const Scene& s) {
    return std::string("ch07/tongdao_") + s.companion + (s.tier == 0 ? std::string("_qingjiao") : "_t" + std::to_string(s.tier)) +
           ".lua";
}

TEST_F(TongdaoContent, ElevenTalksAndThreeLessonsRunHeadlessAndKeepTheirBooks) {
    ASSERT_EQ(scenes().size(), 14u) << "11 段叙话 + 3 段请教";
    const json texts = json::parse(readFile(fs::path(assetRoot()) / "data" / "text" / "tongdao.json"));
    std::set<std::string> everyKey;
    for (const Scene& s : scenes()) {
        const std::string path = scriptOf(s);
        ASSERT_TRUE(fs::exists(fs::path(assetRoot()) / "scripts" / path)) << path;
        // 叙话第 N 档：默契恰在第 N 档门口，前一档演过了（二档没有前一档）。请教：三档。
        startFromChujindi();
        const int threshold[] = {0, 0, 20, 45, 70, 95};
        setBond(s.companion, s.tier == 0 ? 45 : threshold[s.tier]);
        if (s.tier > 2) state().setFlag(rules::xuhuaFlag(s.companion), s.tier - 1);
        if (s.tier > 0) ASSERT_EQ(rules::xuhuaPending(companions(), s.companion, state()), s.tier) << path;
        const int fireBalls = state().itemCount("talisman_huoqiu_fu");

        const std::vector<std::string> spoken = runEvent(path);
        ASSERT_FALSE(HasFatalFailure());
        ASSERT_FALSE(spoken.empty()) << path;
        EXPECT_LE(spoken.size(), 8u) << path << "：每段不超过 8 句（施工图 2.2）";
        for (const std::string& key : spoken) {
            EXPECT_EQ(key.rfind(std::string("tongdao.") + s.companion + ".", 0), 0u) << path << "：" << key;
            EXPECT_TRUE(texts.contains(key)) << path << "：缺文案 " << key;
            EXPECT_NE(app_.text(key), key) << key;
            everyKey.insert(key);
        }
        if (s.tier > 0) {
            EXPECT_EQ(state().flag(rules::xuhuaFlag(s.companion)), s.tier) << path << "：记对了账";
            EXPECT_EQ(rules::xuhuaPending(companions(), s.companion, state()), 0) << path;
            EXPECT_EQ(facesOf(s.role), (std::set<std::string>{"normal", "smile", "angry", "hurt"}))
                << path << "：四个表情都用上";
        } else {
            EXPECT_TRUE(rules::canQingjiao(companions(), s.companion, state()) == rules::TongdaoCheck::AlreadyDone) << path;
            EXPECT_EQ(state().flag(rules::qingjiaoFlag(s.companion)), 1) << path;
            EXPECT_TRUE(rules::rosterBonusUnlocked(companion(s.companion), state())) << path << "：在册加成从此生效";
        }
        // 菡云芝二档给三张火球符（只给一次）；别的段不给东西。
        const int given = state().itemCount("talisman_huoqiu_fu") - fireBalls;
        EXPECT_EQ(given, (s.companion == std::string(kHan) && s.tier == 2) ? 3 : 0) << path;
        // 陈巧倩五档铺了事件图、演完撤了；别的段不碰。
        if (s.companion == std::string(kChen) && s.tier == 5) {
            EXPECT_EQ(cgSeen_, std::set<std::string>{"chen_qiaoqian_hongyan"}) << path;
        } else {
            EXPECT_TRUE(cgSeen_.empty()) << path;
        }
        EXPECT_TRUE(app_.currentCg().empty()) << path;
    }
    // 再演一遍菡云芝二档：账已记过，东西不再给。
    startFromChujindi();
    setBond(kHan, 20);
    state().setFlag(rules::xuhuaFlag(kHan), 2);
    const int before = state().itemCount("talisman_huoqiu_fu");
    runEvent("ch07/tongdao_han_yunzhi_t2.lua");
    EXPECT_EQ(state().itemCount("talisman_huoqiu_fu"), before);

    // 叙话、请教的每一句都在 tongdao.json 里，没有一句漏进 ch07*.json 的登记表（键前缀不是 ch07.）。
    for (const auto& [key, value] : texts.items()) {
        EXPECT_NE(key.rfind("ch07.", 0), 0u) << key;
        if (key.find(".t") != std::string::npos || key.find(".qingjiao.") != std::string::npos) {
            EXPECT_TRUE(everyKey.count(key)) << "没有脚本说这一句：" << key;
        }
    }
    // 路 U 的页面用的两类文字：三位都有。
    for (const char* id : kThree) {
        EXPECT_TRUE(texts.contains(std::string("tongdao.") + id + ".hint")) << id;
        EXPECT_TRUE(texts.contains(std::string("tongdao.") + id + ".intro")) << id;
    }
    // 剪影下的提示不点名字。
    EXPECT_EQ(texts["tongdao.chen_qiaoqian.hint"].get<std::string>().find("陈"), std::string::npos);
    EXPECT_EQ(texts["tongdao.nangong_wan.hint"].get<std::string>().find("南宫"), std::string::npos);
    EXPECT_EQ(texts["tongdao.han_yunzhi.hint"].get<std::string>().find("菡"), std::string::npos);
    // 新法术的描述都有。
    for (const char* id : {"magic_shuangjian", "magic_jianmu", "magic_duo_jianren", "magic_liuhuo", "magic_huanhu",
                           "magic_fu_hanbing", "magic_fu_huichun", "magic_fu_dingshen", "magic_duo_lianzhu"}) {
        const Magic* m = app_.data().findMagic(id);
        ASSERT_NE(m, nullptr) << id;
        EXPECT_NE(app_.text(m->descKey), m->descKey) << id;
    }
}

// 文案底线里能机器查的那几条：南宫婉、菡云芝的段里没有一句带「红颜」「道侣」「心意」这种词，
// 三位的段里都没有「拥抱」「吻」「握住」这一类身体接触的字眼（菡云芝「爱脸红」是她既定的性子，见 cards.json，不算）。真正的复审是主会话逐句看。
TEST_F(TongdaoContent, TheLinesStayInsideTheLinesOfSectionTwoTwo) {
    const json texts = json::parse(readFile(fs::path(assetRoot()) / "data" / "text" / "tongdao.json"));
    for (const auto& [key, value] : texts.items()) {
        const std::string line = value.get<std::string>();
        for (const char* word : {"拥抱", "抱住", "抱起", "吻", "握住", "搂", "依偎", "牵着", "肌肤", "衣带"}) {
            EXPECT_EQ(line.find(word), std::string::npos) << key << " 含「" << word << "」";
        }
        if (key.rfind("tongdao.nangong_wan.", 0) == 0 || key.rfind("tongdao.han_yunzhi.", 0) == 0) {
            for (const char* word : {"红颜", "道侣", "心意", "喜欢你", "倾心"}) {
                EXPECT_EQ(line.find(word), std::string::npos) << key << " 含「" << word << "」";
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 事件图
// ---------------------------------------------------------------------------

TEST(TongdaoCg, TheTableMatchesThePicture) {
    const fs::path root = assetRoot();
    const json table = json::parse(readFile(root / "data" / "visual" / "cg.json"));
    ASSERT_TRUE(table.contains("_doc") && table["_doc"].is_string());
    ASSERT_TRUE(table.contains("cg") && table["cg"].is_object());
    ASSERT_TRUE(table.contains("files") && table["files"].is_object());
    EXPECT_EQ(table.size(), 3u) << "形状定死：_doc、cg、files";
    ASSERT_TRUE(table["cg"].contains("chen_qiaoqian_hongyan"));
    std::set<std::string> referenced;
    for (const auto& [id, entry] : table["cg"].items()) {
        ASSERT_TRUE(entry.is_object() && entry.size() == 1u && entry["file"].is_string()) << id;
        const std::string file = entry["file"].get<std::string>();
        referenced.insert(file);
        ASSERT_TRUE(table["files"].contains(file)) << id;
        const std::string bytes = readFile(root / "assets" / "art" / "cg" / file);
        ASSERT_GT(bytes.size(), 24u) << file;
        EXPECT_EQ(sha256Hex(bytes), table["files"][file].get<std::string>()) << file;
        // PNG 头：IHDR 的宽高（大端）。
        ASSERT_EQ(bytes.substr(1, 3), "PNG") << file;
        const auto be32 = [&](std::size_t at) {
            std::uint32_t v = 0;
            for (std::size_t i = 0; i < 4; ++i) v = (v << 8) | static_cast<unsigned char>(bytes[at + i]);
            return v;
        };
        EXPECT_EQ(be32(16), 2560u) << file;
        EXPECT_EQ(be32(20), 1440u) << file;
    }
    for (const auto& [file, sha] : table["files"].items()) EXPECT_TRUE(referenced.count(file)) << "没人用的图：" << file;
    // 脚本里 show_cg 的每一个 id 都在表里。
    const std::string script = readFile(root / "scripts" / "ch07" / "tongdao_chen_qiaoqian_t5.lua");
    EXPECT_NE(script.find("show_cg(\"chen_qiaoqian_hongyan\")"), std::string::npos);
}

}  // namespace
