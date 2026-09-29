// 改键（docs/settings.md 4.4、第 6 节、第 7 节第 3 条二期那几条）：引擎的键位表、整表校验、改一格的五条规则
// 与两条拒绝、抓键、显示名、词表。
//
// 判据抄契约原文：固定键 ↑↓←→ / Enter、小键盘 Enter / Esc；默认自定义 W S A D / Z 空格 / X / Tab / 左 Ctrl / F5 / E Q；
// 保留键 左右 Alt、左右 Win、菜单键、Backspace、Delete；显示名那一行逐项照抄。默认表与改造前 kBindings 那 19 条一一对应，
// 这里把那 19 条原样写成字面量来比——不从被测的表里推。
// 引擎那几条走无头引擎 + SDL 真事件（SDL_PushEvent，与 EngineTests 的 pushKey 同一个做法）。
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "engine/Engine.h"
#include "game/KeyConfigScene.h"
#include "io/DataLoader.h"
#include "io/SettingsFile.h"

namespace {

using fanren::engine::Engine;
using fanren::engine::ScanCode;
using Key = Engine::Key;
using KeyChange = Engine::KeyChange;
using KeyTable = Engine::KeyTable;

void pushKey(SDL_Scancode scancode, bool down, SDL_Keymod mod = SDL_KMOD_NONE) {
    SDL_Event event{};
    event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
    event.key.scancode = scancode;
    event.key.mod = mod;
    event.key.down = down;
    event.key.repeat = false;
    SDL_PushEvent(&event);
}

class HeadlessKeys : public ::testing::Test {
protected:
    void SetUp() override {
        const auto result = engine.init("fanren-tests", true);
        ASSERT_TRUE(result.ok) << result.error;
    }
    void TearDown() override { engine.shutdown(); }

    // 按一下：按下、收一帧、松开、再收一帧。返回按下那一帧 key 有没有「刚按下」。
    bool tap(SDL_Scancode scancode, Key key) {
        pushKey(scancode, true);
        engine.pollEvents();
        const bool fired = engine.keyPressed(key);
        pushKey(scancode, false);
        engine.pollEvents();
        return fired;
    }

    Engine engine;
};

std::size_t at(Key key) {
    return static_cast<std::size_t>(key);
}

KeyTable defaults() {
    return Engine::defaultKeyTable();
}

}  // namespace

// ---------------------------------------------------------------------------
// 词表、默认表、显示名
// ---------------------------------------------------------------------------

// 设置文件的 keys 用的词（io::kKeyActionIds，一期放在 io）与引擎的词表必须是同一组、同一个次序。
TEST(KeyVocabulary, TheEngineAndTheSettingsFileUseTheSameWords) {
    ASSERT_EQ(fanren::io::kKeyActionIds.size(), Engine::kKeyCount);
    for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
        const auto key = static_cast<Key>(k);
        EXPECT_EQ(std::string(Engine::keyId(key)), std::string(fanren::io::kKeyActionIds[k]));
        EXPECT_EQ(Engine::keyFromId(fanren::io::kKeyActionIds[k]), std::optional<Key>(key));
    }
    EXPECT_FALSE(Engine::keyFromId("jump").has_value());
    EXPECT_FALSE(Engine::keyFromId("").has_value());
    EXPECT_EQ(fanren::engine::kScanCodeLimit, fanren::io::kScancodeLimit);
}

TEST(KeyDefaults, FixedPlusDefaultCustomKeysAreExactlyTheOldNineteenBindings) {
    // 改造前 src/engine/Engine.cpp 的 kBindings，原样抄过来。
    const std::set<std::pair<int, int>> old{
        {SDL_SCANCODE_UP, 0},     {SDL_SCANCODE_W, 0},      {SDL_SCANCODE_DOWN, 1},   {SDL_SCANCODE_S, 1},
        {SDL_SCANCODE_LEFT, 2},   {SDL_SCANCODE_A, 2},      {SDL_SCANCODE_RIGHT, 3},  {SDL_SCANCODE_D, 3},
        {SDL_SCANCODE_RETURN, 4}, {SDL_SCANCODE_KP_ENTER, 4}, {SDL_SCANCODE_Z, 4},    {SDL_SCANCODE_SPACE, 4},
        {SDL_SCANCODE_ESCAPE, 5}, {SDL_SCANCODE_X, 5},      {SDL_SCANCODE_TAB, 6},    {SDL_SCANCODE_LCTRL, 7},
        {SDL_SCANCODE_F5, 8},     {SDL_SCANCODE_E, 9},      {SDL_SCANCODE_Q, 9},
    };
    ASSERT_EQ(old.size(), 19u);
    std::set<std::pair<int, int>> now;
    std::size_t count = 0;
    for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
        const auto key = static_cast<Key>(k);
        for (const ScanCode code : Engine::fixedKeys(key)) {
            now.insert({code, static_cast<int>(k)});
            ++count;
        }
        for (const ScanCode code : Engine::defaultCustomKeys(key)) {
            if (code == 0) continue;
            now.insert({code, static_cast<int>(k)});
            ++count;
        }
    }
    EXPECT_EQ(count, 19u) << "默认表里有重复的键";
    EXPECT_EQ(now, old);
    EXPECT_TRUE(Engine::validateKeyTable(defaults()).ok);
    // 固定键的分布照契约第 6 节那张表：方向各一个，确认两个（Enter、小键盘 Enter），取消一个（Esc），其余没有。
    EXPECT_EQ(Engine::fixedKeys(Key::Confirm), (std::vector<ScanCode>{SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER}));
    EXPECT_EQ(Engine::fixedKeys(Key::Cancel), (std::vector<ScanCode>{SDL_SCANCODE_ESCAPE}));
    for (const Key key : {Key::Menu, Key::Skip, Key::Save, Key::Action}) {
        EXPECT_TRUE(Engine::fixedKeys(key).empty()) << Engine::keyId(key);
    }
}

TEST(KeyLabels, FollowTheContractsTable) {
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_A), "A");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_M), "M");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_Z), "Z");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_1), "1");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_9), "9");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_0), "0");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_F1), "F1");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_F5), "F5");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_F12), "F12");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_SPACE), "空格");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_RETURN), "Enter");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_KP_ENTER), "小键盘Enter");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_ESCAPE), "Esc");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_TAB), "Tab");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_UP), "↑");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_DOWN), "↓");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_LEFT), "←");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_RIGHT), "→");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_LCTRL), "Ctrl");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_RCTRL), "右Ctrl");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_LSHIFT), "Shift");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_RSHIFT), "右Shift");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_KP_0), "小键盘0");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_KP_1), "小键盘1");
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_KP_9), "小键盘9");
    // 其余用 SDL 的名字；SDL 也叫不出名字的，写「键#数值」。
    EXPECT_EQ(Engine::keyLabel(SDL_SCANCODE_HOME), std::string(SDL_GetScancodeName(SDL_SCANCODE_HOME)));
    EXPECT_FALSE(Engine::keyLabel(SDL_SCANCODE_HOME).empty());
    EXPECT_EQ(Engine::keyLabel(450), "键#450");
    EXPECT_EQ(Engine::keyLabel(0), "键#0");
}

// ---------------------------------------------------------------------------
// 整表校验
// ---------------------------------------------------------------------------

TEST(KeyTableValidation, EachRuleOfTheWholeTableCheckRefusesOnItsOwn) {
    const auto withMenu = [](ScanCode a, ScanCode b) {
        KeyTable table = defaults();
        table[at(Key::Menu)] = {a, b};
        return table;
    };
    EXPECT_TRUE(Engine::validateKeyTable(withMenu(SDL_SCANCODE_M, 0)).ok) << "先验：换一个没人用的键是可以的";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(SDL_SCANCODE_W, 0)).ok) << "重复：W 是「向上」的";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(SDL_SCANCODE_M, SDL_SCANCODE_M)).ok) << "同一个动作两格重复";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(SDL_SCANCODE_UP, 0)).ok) << "固定键";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(SDL_SCANCODE_LALT, 0)).ok) << "保留键";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(0, 0)).ok) << "「菜单」没有固定键：一个键都不剩";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(600, 0)).ok) << "越界";
    EXPECT_FALSE(Engine::validateKeyTable(withMenu(-3, 0)).ok) << "负数";
    // 有固定键的动作两格都空着是可以的：「向上」还有 ↑。
    KeyTable upless = defaults();
    upless[at(Key::Up)] = {0, 0};
    EXPECT_TRUE(Engine::validateKeyTable(upless).ok);
    // 失败时说得出是哪一条（给日志看）。
    const auto dup = Engine::validateKeyTable(withMenu(SDL_SCANCODE_W, 0));
    EXPECT_NE(dup.error.find("menu"), std::string::npos) << dup.error;
    EXPECT_NE(dup.error.find("up"), std::string::npos) << dup.error;
}

// ---------------------------------------------------------------------------
// 改一格：五条规则 + 两条拒绝（纯函数，直接喂表）
// ---------------------------------------------------------------------------

TEST(KeyRules, RuleOneFixedAndReservedKeysAreRefusedAndSayWhose) {
    const KeyTable table = defaults();
    const auto fixed = Engine::assignKey(table, Key::Menu, 0, SDL_SCANCODE_UP);
    EXPECT_EQ(fixed.change, KeyChange::RejectedFixed);
    EXPECT_EQ(fixed.other, Key::Up) << "说得出是谁的固定键";
    EXPECT_EQ(fixed.table, table) << "拒绝时原表不动";
    EXPECT_EQ(Engine::assignKey(table, Key::Skip, 1, SDL_SCANCODE_KP_ENTER).other, Key::Confirm);
    for (const SDL_Scancode reserved : {SDL_SCANCODE_LALT, SDL_SCANCODE_RALT, SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI,
                                        SDL_SCANCODE_APPLICATION, SDL_SCANCODE_BACKSPACE, SDL_SCANCODE_DELETE}) {
        const auto result = Engine::assignKey(table, Key::Menu, 1, reserved);
        EXPECT_EQ(result.change, KeyChange::RejectedReserved) << static_cast<int>(reserved);
        EXPECT_EQ(result.table, table);
    }
}

TEST(KeyRules, RuleTwoTheKeyAlreadyInThatSlotChangesNothing) {
    const auto result = Engine::assignKey(defaults(), Key::Up, 0, SDL_SCANCODE_W);
    EXPECT_EQ(result.change, KeyChange::Unchanged);
    EXPECT_EQ(result.table, defaults());
}

TEST(KeyRules, RuleThreeTheOtherSlotOfTheSameActionSwaps) {
    const auto result = Engine::assignKey(defaults(), Key::Confirm, 0, SDL_SCANCODE_SPACE);
    EXPECT_EQ(result.change, KeyChange::SwappedSlots);
    EXPECT_EQ(result.table[at(Key::Confirm)], (Engine::KeySlots{SDL_SCANCODE_SPACE, SDL_SCANCODE_Z}));
    // 这一格原来空着时就是「挪过去」、另一格空出来：键没多也没少，而且说的是挪、不是互换。
    const auto moved = Engine::assignKey(defaults(), Key::Up, 1, SDL_SCANCODE_W);
    EXPECT_EQ(moved.change, KeyChange::MovedSlot);
    EXPECT_EQ(moved.table[at(Key::Up)], (Engine::KeySlots{0, SDL_SCANCODE_W}));
}

TEST(KeyRules, RuleFourAKeyOfAnotherActionIsSwappedOver) {
    const auto result = Engine::assignKey(defaults(), Key::Menu, 0, SDL_SCANCODE_W);
    EXPECT_EQ(result.change, KeyChange::TradedWith);
    EXPECT_EQ(result.other, Key::Up);
    EXPECT_EQ(result.table[at(Key::Menu)], (Engine::KeySlots{SDL_SCANCODE_W, 0}));
    EXPECT_EQ(result.table[at(Key::Up)], (Engine::KeySlots{SDL_SCANCODE_TAB, 0})) << "那一格换成这一格原来的键";

    // 这一格原来空着：B 那一格就空出来（说的是「挪过来」，不是「互换」）——B 还有别的键（「取消」有固定的 Esc）就可以。
    const auto emptied = Engine::assignKey(defaults(), Key::Menu, 1, SDL_SCANCODE_X);
    EXPECT_EQ(emptied.change, KeyChange::TookFrom);
    EXPECT_EQ(emptied.other, Key::Cancel);
    EXPECT_EQ(emptied.table[at(Key::Menu)], (Engine::KeySlots{SDL_SCANCODE_TAB, SDL_SCANCODE_X}));
    EXPECT_EQ(emptied.table[at(Key::Cancel)], (Engine::KeySlots{0, 0}));
}

TEST(KeyRules, RuleFourRefusesToStripAnActionOfItsLastKey) {
    // 「菜单」只有 Tab、没有固定键：把 Tab 挪到「向上」空着的第二格，菜单就一个键都不剩了。
    const auto result = Engine::assignKey(defaults(), Key::Up, 1, SDL_SCANCODE_TAB);
    EXPECT_EQ(result.change, KeyChange::RejectedLastKey);
    EXPECT_EQ(result.other, Key::Menu) << "说的是会被掏空的那个动作";
    EXPECT_EQ(result.table, defaults());
    // [配对] 挪到「向上」有键的第一格就是互换，菜单换得 W，不算掏空。
    EXPECT_EQ(Engine::assignKey(defaults(), Key::Up, 0, SDL_SCANCODE_TAB).change, KeyChange::TradedWith);
}

TEST(KeyRules, RuleFiveAFreeKeyGoesStraightIn) {
    const auto result = Engine::assignKey(defaults(), Key::Menu, 1, SDL_SCANCODE_M);
    EXPECT_EQ(result.change, KeyChange::Assigned);
    EXPECT_EQ(result.table[at(Key::Menu)], (Engine::KeySlots{SDL_SCANCODE_TAB, SDL_SCANCODE_M}));
    KeyTable expected = defaults();
    expected[at(Key::Menu)][1] = SDL_SCANCODE_M;
    EXPECT_EQ(result.table, expected) << "别的动作一格都没动";
}

TEST(KeyRules, ClearingTheLastKeyIsRefusedClearingASpareIsNot) {
    const auto last = Engine::clearKey(defaults(), Key::Menu, 0);
    EXPECT_EQ(last.change, KeyChange::RejectedLastKey);
    EXPECT_EQ(last.other, Key::Menu);
    EXPECT_EQ(last.table, defaults());

    const auto spare = Engine::clearKey(defaults(), Key::Action, 1);
    EXPECT_EQ(spare.change, KeyChange::Cleared);
    EXPECT_EQ(spare.table[at(Key::Action)], (Engine::KeySlots{SDL_SCANCODE_E, 0}));

    // 「向上」两格都清得掉：还有固定的 ↑。
    const auto first = Engine::clearKey(defaults(), Key::Up, 0);
    EXPECT_EQ(first.change, KeyChange::Cleared);
    const auto again = Engine::clearKey(first.table, Key::Up, 1);
    EXPECT_EQ(again.change, KeyChange::AlreadyEmpty) << "清一个本来就空的格：说「本来就空」，不说「就是这个键」";
    EXPECT_EQ(again.table, first.table);
}

TEST(KeyRules, NonsenseInputIsRefusedWithoutTouchingTheTable) {
    for (const auto& result : {Engine::assignKey(defaults(), Key::Menu, 2, SDL_SCANCODE_M),
                               Engine::assignKey(defaults(), Key::Menu, 0, 0),
                               Engine::assignKey(defaults(), Key::Menu, 0, 600),
                               Engine::assignKey(defaults(), Key::Count, 0, SDL_SCANCODE_M),
                               Engine::clearKey(defaults(), Key::Menu, -1)}) {
        EXPECT_EQ(result.change, KeyChange::RejectedInvalid);
        EXPECT_EQ(result.table, defaults());
    }
}

// 不变式：每个动作至少一个键；两个动作永远不共用一个键。随便怎么改、怎么清，表始终过得了整表校验。
TEST(KeyRules, NoSequenceOfChangesBreaksTheInvariants) {
    const std::vector<ScanCode> pool{
        SDL_SCANCODE_W,  SDL_SCANCODE_S,     SDL_SCANCODE_A,      SDL_SCANCODE_D,    SDL_SCANCODE_Z,
        SDL_SCANCODE_X,  SDL_SCANCODE_TAB,   SDL_SCANCODE_LCTRL,  SDL_SCANCODE_F5,   SDL_SCANCODE_E,
        SDL_SCANCODE_Q,  SDL_SCANCODE_SPACE, SDL_SCANCODE_M,      SDL_SCANCODE_UP,   SDL_SCANCODE_LALT,
        SDL_SCANCODE_J,  SDL_SCANCODE_K,     SDL_SCANCODE_RETURN, SDL_SCANCODE_ESCAPE,
    };
    KeyTable table = defaults();
    std::uint32_t seed = 20260927u;
    const auto next = [&seed](std::uint32_t n) {
        seed = seed * 1664525u + 1013904223u;
        return (seed >> 8) % n;
    };
    // 判据自己写，不借被测的 validateKeyTable：固定键、保留键照契约第 6 节写成字面量集合；全表的扫描码去重；
    // 没有固定键的四个动作（菜单、快进、存盘、路径行动）各至少一个自定义键——有固定键的那六个本来就不会一个都不剩。
    const std::set<ScanCode> fixedLiteral{SDL_SCANCODE_UP,     SDL_SCANCODE_DOWN,     SDL_SCANCODE_LEFT,  SDL_SCANCODE_RIGHT,
                                          SDL_SCANCODE_RETURN, SDL_SCANCODE_KP_ENTER, SDL_SCANCODE_ESCAPE};
    const std::set<ScanCode> reservedLiteral{SDL_SCANCODE_LALT,        SDL_SCANCODE_RALT,      SDL_SCANCODE_LGUI,
                                             SDL_SCANCODE_RGUI,        SDL_SCANCODE_APPLICATION, SDL_SCANCODE_BACKSPACE,
                                             SDL_SCANCODE_DELETE};
    const auto invariantBroken = [&](const KeyTable& t) -> std::string {
        std::set<ScanCode> seen;
        for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
            for (const ScanCode code : t[k]) {
                if (code == 0) continue;
                if (code < 0 || code >= 512) return "扫描码越界 " + std::to_string(code);
                if (fixedLiteral.count(code) != 0) return "配了固定键 " + std::to_string(code);
                if (reservedLiteral.count(code) != 0) return "配了保留键 " + std::to_string(code);
                if (!seen.insert(code).second) return "两处配了同一个键 " + std::to_string(code);
            }
        }
        for (const Key key : {Key::Menu, Key::Skip, Key::Save, Key::Action}) {
            if (t[at(key)][0] == 0 && t[at(key)][1] == 0) return std::string("一个键都不剩：") + Engine::keyId(key);
        }
        return {};
    };
    ASSERT_TRUE(invariantBroken(table).empty()) << "先验：默认表本身成立";
    int applied = 0;
    for (int step = 0; step < 4000; ++step) {
        const auto action = static_cast<Key>(next(static_cast<std::uint32_t>(Engine::kKeyCount)));
        const int slot = static_cast<int>(next(2));
        const bool clear = next(4) == 0;
        const auto result = clear ? Engine::clearKey(table, action, slot)
                                  : Engine::assignKey(table, action, slot, pool[next(static_cast<std::uint32_t>(pool.size()))]);
        const std::string broken = invariantBroken(result.table);
        ASSERT_TRUE(broken.empty()) << "第 " << step << " 步之后：" << broken;
        if (result.table != table) ++applied;
        table = result.table;
    }
    EXPECT_GT(applied, 500) << "先验：真的改动过很多次，不是一路都被拒绝";
    // [配对] 自写的判据有牙：把「菜单」清空、把 W 配两处、把 ↑ 配出去，它都认得出来。
    KeyTable bad = defaults();
    bad[at(Key::Menu)] = {0, 0};
    EXPECT_FALSE(invariantBroken(bad).empty());
    bad = defaults();
    bad[at(Key::Menu)] = {SDL_SCANCODE_W, 0};
    EXPECT_FALSE(invariantBroken(bad).empty());
    bad = defaults();
    bad[at(Key::Save)] = {SDL_SCANCODE_UP, 0};
    EXPECT_FALSE(invariantBroken(bad).empty());
}

// ---------------------------------------------------------------------------
// 引擎：自定义键驱动 keyDown / keyPressed；固定键删不掉；整表换不上就一格不动；抓键
// ---------------------------------------------------------------------------

TEST_F(HeadlessKeys, CustomKeysDriveKeyDownAndKeyPressed) {
    KeyTable table = defaults();
    table[at(Key::Menu)] = {SDL_SCANCODE_M, 0};
    ASSERT_TRUE(engine.setCustomKeys(table).ok);
    EXPECT_EQ(engine.customKeys(Key::Menu), (Engine::KeySlots{SDL_SCANCODE_M, 0}));

    pushKey(SDL_SCANCODE_M, true);
    engine.pollEvents();
    EXPECT_TRUE(engine.keyPressed(Key::Menu));
    EXPECT_TRUE(engine.keyDown(Key::Menu));
    pushKey(SDL_SCANCODE_M, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyDown(Key::Menu));
    EXPECT_FALSE(tap(SDL_SCANCODE_TAB, Key::Menu)) << "Tab 让出来了：不再是菜单";
}

TEST_F(HeadlessKeys, FixedKeysKeepWorkingWhenTheCustomSlotsAreEmpty) {
    KeyTable table = defaults();
    table[at(Key::Up)] = {0, 0};
    ASSERT_TRUE(engine.setCustomKeys(table).ok);
    EXPECT_TRUE(tap(SDL_SCANCODE_UP, Key::Up)) << "固定键删不掉";
    EXPECT_FALSE(tap(SDL_SCANCODE_W, Key::Up)) << "W 清掉了";
    // 固定键也挪不走：把 ↑ 配给「菜单」的表整张换不上。
    KeyTable stolen = defaults();
    stolen[at(Key::Menu)] = {SDL_SCANCODE_UP, 0};
    EXPECT_FALSE(engine.setCustomKeys(stolen).ok);
    EXPECT_TRUE(tap(SDL_SCANCODE_UP, Key::Up));
    EXPECT_FALSE(tap(SDL_SCANCODE_UP, Key::Menu));
}

TEST_F(HeadlessKeys, ARejectedTableChangesNothing) {
    KeyTable bad = defaults();
    bad[at(Key::Menu)] = {SDL_SCANCODE_W, 0};   // W 已经是「向上」的
    const auto result = engine.setCustomKeys(bad);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.error.empty());
    EXPECT_EQ(engine.customKeys(Key::Menu), (Engine::KeySlots{SDL_SCANCODE_TAB, 0})) << "校验不过不改";
    EXPECT_TRUE(tap(SDL_SCANCODE_TAB, Key::Menu));
    EXPECT_TRUE(tap(SDL_SCANCODE_W, Key::Up));
}

TEST_F(HeadlessKeys, WhileCapturingNoLogicalKeyFiresAndTheNextPressIsTaken) {
    // 按住 Enter 开始抓键（改键面板就是这么进的）：从这一刻起不出 Confirm，按住 300ms 也不自动重复。
    pushKey(SDL_SCANCODE_RETURN, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Confirm));
    engine.beginKeyCapture();
    EXPECT_TRUE(engine.capturingKey());
    EXPECT_FALSE(engine.keyPressed(Key::Confirm)) << "本帧已经记下的「刚按下」作废";
    SDL_Delay(300);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Confirm)) << "抓键期间按住的键不自动重复";
    pushKey(SDL_SCANCODE_RETURN, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.takeCapturedKey().has_value()) << "松开不算按下";

    // 下一次物理按下：抓走，不映射成任何逻辑键（Z 本来是确认）。
    pushKey(SDL_SCANCODE_Z, true);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Confirm));
    EXPECT_FALSE(engine.keyDown(Key::Confirm)) << "抓走的那一下也不算按住";
    EXPECT_FALSE(engine.capturingKey());
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<ScanCode>(SDL_SCANCODE_Z));
    EXPECT_FALSE(engine.takeCapturedKey().has_value()) << "取走一次就没了";
    pushKey(SDL_SCANCODE_Z, false);
    engine.pollEvents();

    // 抓完回到常态。
    EXPECT_TRUE(tap(SDL_SCANCODE_X, Key::Cancel));
    EXPECT_TRUE(tap(SDL_SCANCODE_Z, Key::Confirm));
}

// docs/gamepad.md 第 5 节：只有方向键连发之后，上面那条拿确认键验「抓键期间按住的键不自动重复」失去了判别力——
// 确认键本来就不连发，抓键不拦连发它也照样过。换一个本来就连发的方向键，这条判据才还在。
TEST_F(HeadlessKeys, WhileCapturingAHeldArrowDoesNotRepeatEither) {
    // [先验] 不抓键时同样按住 ↓ 300ms，确实连发：下面那个「不连发」才判得出东西。
    pushKey(SDL_SCANCODE_DOWN, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Down));
    SDL_Delay(300);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Down)) << "先验：不抓键时按住 ↓ 过了 250ms 就连发";
    pushKey(SDL_SCANCODE_DOWN, false);
    engine.pollEvents();

    // 按住 ↓ 开始抓键：从这一刻起不出 Down，按住 300ms 也不连发。
    pushKey(SDL_SCANCODE_DOWN, true);
    engine.pollEvents();
    ASSERT_TRUE(engine.keyPressed(Key::Down));
    engine.beginKeyCapture();
    EXPECT_FALSE(engine.keyPressed(Key::Down)) << "本帧已经记下的「刚按下」作废";
    SDL_Delay(300);
    engine.pollEvents();
    EXPECT_FALSE(engine.keyPressed(Key::Down)) << "抓键期间按住的方向键不连发";
    pushKey(SDL_SCANCODE_DOWN, false);
    engine.pollEvents();
    EXPECT_FALSE(engine.takeCapturedKey().has_value()) << "松开不算按下";
    engine.cancelKeyCapture();
}

TEST_F(HeadlessKeys, AltEnterStillTogglesFullscreenWhileCapturingAndIsNotTaken) {
    engine.beginKeyCapture();
    // 真键盘上是先到一下左 Alt 的按下，再到 Enter：那一下 Alt 不许被抓走（抓走了就是「留作他用」、抓键结束）。
    pushKey(SDL_SCANCODE_LALT, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.capturingKey()) << "先到的左 Alt 不是被抓的那一下";
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushKey(SDL_SCANCODE_RETURN, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.fullscreen());
    EXPECT_TRUE(engine.capturingKey()) << "Alt+Enter 也不是被抓的那一下";
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushKey(SDL_SCANCODE_RETURN, false, SDL_KMOD_LALT);
    pushKey(SDL_SCANCODE_LALT, false);
    engine.pollEvents();

    // Alt+Tab、Win 组合键的前一半同理：右 Alt、左右 Win 按下去都不结束抓键。
    for (const SDL_Scancode modifier : {SDL_SCANCODE_RALT, SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI}) {
        pushKey(modifier, true);
        engine.pollEvents();
        EXPECT_TRUE(engine.capturingKey()) << static_cast<int>(modifier);
        EXPECT_FALSE(engine.takeCapturedKey().has_value()) << static_cast<int>(modifier);
        pushKey(modifier, false);
        engine.pollEvents();
    }
    // [配对] 修饰键之后按下的真键照样被抓；菜单键虽也是保留键，却是实打实的一个键，也照抓（由面板去说「留作他用」）。
    pushKey(SDL_SCANCODE_J, true);
    engine.pollEvents();
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<ScanCode>(SDL_SCANCODE_J));
    pushKey(SDL_SCANCODE_J, false);
    engine.pollEvents();
    engine.beginKeyCapture();
    pushKey(SDL_SCANCODE_APPLICATION, true);
    engine.pollEvents();
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<ScanCode>(SDL_SCANCODE_APPLICATION));
    pushKey(SDL_SCANCODE_APPLICATION, false);
    engine.pollEvents();

    // 面板中途被关掉：收回抓键，之后照常出逻辑键。
    engine.beginKeyCapture();
    engine.cancelKeyCapture();
    EXPECT_FALSE(engine.capturingKey());
    EXPECT_TRUE(tap(SDL_SCANCODE_X, Key::Cancel));
}

// 审查 H1：SDL3 给没映射的键（媒体键、启动键、笔记本 Fn 组合、厂商宏键）发扫描码 0、raw ≠ 0 的真事件。
// 0 是键位表的「空格位」：从前它与默认表里八个空格位一起命中，同一帧八个逻辑键全按下（世界层先判存盘，悄悄覆盖 quick.sav）。
TEST_F(HeadlessKeys, AnUnmappedKeyWithScancodeZeroIsNoKeyAtAll) {
    const auto pushUnknown = [](bool down) {
        SDL_Event event{};
        event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.scancode = SDL_SCANCODE_UNKNOWN;
        event.key.raw = 0xE0;   // 平台给的原始码不是 0：SDL 认得「有一个键」，只是叫不出它是哪个
        event.key.down = down;
        event.key.repeat = false;
        SDL_PushEvent(&event);
    };
    const auto expectNoKey = [this](const char* when) {
        for (std::size_t k = 0; k < Engine::kKeyCount; ++k) {
            const auto key = static_cast<Key>(k);
            EXPECT_FALSE(engine.keyPressed(key)) << when << "：" << Engine::keyId(key);
            EXPECT_FALSE(engine.keyDown(key)) << when << "：" << Engine::keyId(key);
        }
    };
    pushUnknown(true);
    engine.pollEvents();
    expectNoKey("按下那一帧");
    SDL_Delay(300);
    engine.pollEvents();
    expectNoKey("按住过了首次重复延迟");
    pushUnknown(false);
    engine.pollEvents();

    // 抓键中按到它：不算被抓的那一下，抓键继续。
    engine.beginKeyCapture();
    pushUnknown(true);
    engine.pollEvents();
    EXPECT_TRUE(engine.capturingKey());
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushUnknown(false);
    engine.pollEvents();
    engine.cancelKeyCapture();

    // [配对] 真键照常：同一台引擎上按 F5 就是存盘。
    EXPECT_TRUE(tap(SDL_SCANCODE_F5, Key::Save));
}

// 审查 L4：提示文案里的占位 {key.<id>} 只由 Application::text() 展开，而不是全部文案都经它（告示板、路径行动面板、
// 章节卡、设置面板与改键面板的固有字表直接调 lookupText）。所以数据里的占位只许出现在经 text() 显示的四条上，
// id 必须认得、括号必须闭合。白名单写字面量，不从被测的东西推。
TEST(KeyPrompts, PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText) {
    namespace fs = std::filesystem;
    fs::path root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
            root = candidate;
            break;
        }
    }
    const auto loaded = fanren::io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const std::set<std::string> allowed{"ui.menu.keys", "ui.battle.hint.menu", "ui.battle.hint.watch", "ui.dialogue.keys"};

    constexpr std::string_view kOpen = "{key.";
    std::set<std::string> carriers;
    std::size_t viaData = 0;
    for (const auto& [key, line] : loaded.value.text) {
        for (std::size_t at = line.find(kOpen); at != std::string::npos; at = line.find(kOpen, at + 1)) {
            ++viaData;
            carriers.insert(key);
            const std::size_t close = line.find('}', at);
            ASSERT_NE(close, std::string::npos) << key << "：占位没有闭合：" << line;
            const std::string id = line.substr(at + kOpen.size(), close - at - kOpen.size());
            EXPECT_TRUE(Engine::keyFromId(id).has_value()) << key << "：认不出的动作「" << id << "」";
            EXPECT_EQ(id.find('{'), std::string::npos) << key << "：占位里又套了一个括号：" << line;
        }
        EXPECT_TRUE(allowed.count(key) != 0 || line.find(kOpen) == std::string::npos)
            << key << " 带了占位，可它不在经 Application::text() 显示的那四条里：" << line;
    }
    EXPECT_EQ(carriers, allowed) << "四条提示都该带占位（默认键位下展开成改造前的字面）";

    // 读盘那一层（io::loadGameData）没漏掉哪个文件：直接数 data/text/** 的原始字节里有几处占位，与上面数到的对上。
    std::size_t viaBytes = 0;
    for (const auto& entry : fs::recursive_directory_iterator(root / "data" / "text")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path(), std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (std::size_t at = bytes.find(kOpen); at != std::string::npos; at = bytes.find(kOpen, at + 1)) ++viaBytes;
    }
    EXPECT_EQ(viaBytes, viaData);
    EXPECT_GE(viaData, 6u) << "先验：四条提示里一共有六处占位";
}

// docs/gamepad.md 第 6 节：手柄版提示的占位 {pad.<id>} 同样只由 Application::text() 展开，所以只许出现在那 11 条提示的
// .pad 版里；每条 .pad 都得有键盘版本体（text() 是在本体的 id 后面加 ".pad" 去找的）；.pad 里只许有 {pad.*}、
// 不许有 {key.*}；id 认得、括号闭合。白名单写字面量，不从被测的东西推。
// 第 7 节的规矩一并钉住：**文案 id 一律不许以 .pad 结尾，除非它就是那 11 条之一的手柄版**——扫全部文案 id，
// 以 .pad 结尾的集合必须恰是白名单（整改轮：改键面板列头从前叫 ui.keys.col.pad，撞了这个后缀，已改名 ui.keys.col.gamepad）。
TEST(KeyPrompts, PadPlaceholdersOnlyLiveInTheElevenPadPrompts) {
    namespace fs = std::filesystem;
    fs::path root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
            root = candidate;
            break;
        }
    }
    const auto loaded = fanren::io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const std::set<std::string> allowed{
        "ui.title.keys.pad",   "ui.menu.keys.pad",   "ui.dialogue.keys.pad", "ui.path.keys.pad",
        "ui.settings.hint.pad", "ui.keys.hint.pad",  "ui.keys.capture.pad",  "ui.keys.desc.pad",
        "ui.keys.desc.restore.pad", "ui.battle.hint.menu.pad", "ui.battle.hint.watch.pad",
    };
    constexpr std::string_view kOpen = "{pad.";
    constexpr std::string_view kPadSuffix = ".pad";
    std::set<std::string> carriers;
    std::set<std::string> padIds;
    std::size_t viaData = 0;
    for (const auto& [key, line] : loaded.value.text) {
        if (key.size() > kPadSuffix.size() && std::string_view(key).substr(key.size() - kPadSuffix.size()) == kPadSuffix) {
            padIds.insert(key);
            EXPECT_TRUE(allowed.count(key) != 0) << key << " 以 .pad 结尾，却不是那 11 条提示之一的手柄版（第 7 节的规矩）";
            EXPECT_TRUE(loaded.value.text.count(key.substr(0, key.size() - kPadSuffix.size())) != 0)
                << key << " 没有键盘版本体";
            EXPECT_EQ(line.find("{key."), std::string::npos) << key << "：.pad 里不许有 {key.*}：" << line;
        }
        for (std::size_t at = line.find(kOpen); at != std::string::npos; at = line.find(kOpen, at + 1)) {
            ++viaData;
            carriers.insert(key);
            const std::size_t close = line.find('}', at);
            ASSERT_NE(close, std::string::npos) << key << "：占位没有闭合：" << line;
            const std::string id = line.substr(at + kOpen.size(), close - at - kOpen.size());
            EXPECT_TRUE(Engine::keyFromId(id).has_value()) << key << "：认不出的动作「" << id << "」";
            EXPECT_EQ(id.find('{'), std::string::npos) << key << "：占位里又套了一个括号：" << line;
        }
        EXPECT_TRUE(allowed.count(key) != 0 || line.find(kOpen) == std::string::npos)
            << key << " 带了 {pad.，可它不是那 11 条提示的 .pad 版：" << line;
    }
    EXPECT_EQ(carriers, allowed) << "11 条 .pad 都带占位";
    EXPECT_EQ(padIds, allowed) << "文案表里以 .pad 结尾的 id 恰是那 11 条";
    EXPECT_EQ(loaded.value.text.count("ui.keys.col.gamepad"), 1u) << "改键面板的列头在改过的名字上（第 7 节）";

    // 读盘那一层没漏掉哪个文件：直接数 data/text/** 的原始字节里有几处 {pad.，与上面数到的对上。
    std::size_t viaBytes = 0;
    for (const auto& entry : fs::recursive_directory_iterator(root / "data" / "text")) {
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        std::ifstream in(entry.path(), std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        for (std::size_t at = bytes.find(kOpen); at != std::string::npos; at = bytes.find(kOpen, at + 1)) ++viaBytes;
    }
    EXPECT_EQ(viaBytes, viaData);
    EXPECT_EQ(viaData, 18u) << "先验：11 条 .pad 里一共有 18 处占位（数法见 docs/gamepad.md 第 6 节那张表）";
}

// 复查 N1：抓键中按 Alt+F4 想退出——Alt 放过之后，同一下 SDL 还发 F4 的按下（带 Alt 修饰）。从前 F4 被抓走、配进当前格，
// 退出时还补写进盘。按着 Alt / Win 按下去的键一律不抓、不结束抓键（组合键本来也配不了）。
TEST_F(HeadlessKeys, AltOrWinCombosAreNeverTakenAsTheNewKey) {
    engine.beginKeyCapture();
    pushKey(SDL_SCANCODE_LALT, true, SDL_KMOD_LALT);
    engine.pollEvents();
    pushKey(SDL_SCANCODE_F4, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.capturingKey()) << "Alt+F4 的 F4 不是被抓的那一下";
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushKey(SDL_SCANCODE_F4, false, SDL_KMOD_LALT);
    // Alt+空格（系统菜单）同理：空格不许被挪走。
    pushKey(SDL_SCANCODE_SPACE, true, SDL_KMOD_LALT);
    engine.pollEvents();
    EXPECT_TRUE(engine.capturingKey());
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushKey(SDL_SCANCODE_SPACE, false, SDL_KMOD_LALT);
    pushKey(SDL_SCANCODE_LALT, false);
    engine.pollEvents();

    // Win 组合键（Win+D 回桌面）：带 LGUI 修饰的字母键也不抓。
    pushKey(SDL_SCANCODE_LGUI, true, SDL_KMOD_LGUI);
    engine.pollEvents();
    pushKey(SDL_SCANCODE_D, true, SDL_KMOD_LGUI);
    engine.pollEvents();
    EXPECT_TRUE(engine.capturingKey()) << "Win+D 的 D 不是被抓的那一下";
    EXPECT_FALSE(engine.takeCapturedKey().has_value());
    pushKey(SDL_SCANCODE_D, false, SDL_KMOD_LGUI);
    pushKey(SDL_SCANCODE_LGUI, false);
    engine.pollEvents();

    // [配对] 修饰键松开之后，同一个 F4 单按就照抓：挡的是组合键，不是这个键。
    pushKey(SDL_SCANCODE_F4, true);
    engine.pollEvents();
    EXPECT_FALSE(engine.capturingKey());
    EXPECT_EQ(engine.takeCapturedKey(), std::optional<ScanCode>(SDL_SCANCODE_F4));
    pushKey(SDL_SCANCODE_F4, false);
    engine.pollEvents();
}

// 复查 N3：结果类别 → 说明行那句话，逐字比对。期望串写死在这里，不从 ui.json 读——文案里 traded 与 took 对调、
// took 那句两处 {action} 只换了一处，照 ui.json 推判据的话都照样全绿。
TEST(KeyResultText, EachOutcomeSaysWhatHappenedWordForWord) {
    namespace fs = std::filesystem;
    fs::path root = ".";
    for (const char* candidate : {".", "..", "../..", "../../.."}) {
        if (fs::exists(fs::path(candidate) / "data" / "text" / "ch01_main.json")) {
            root = candidate;
            break;
        }
    }
    const auto loaded = fanren::io::loadGameData((root / "data").string());
    ASSERT_TRUE(loaded.ok) << loaded.error;
    const auto say = [&](KeyChange change, Key other) {
        return fanren::game::KeyConfigScene::resultText(loaded.value, Engine::KeyChangeResult{defaults(), change, other});
    };
    EXPECT_EQ(say(KeyChange::TookFrom, Key::Up), "这个键原先是「向上」的，挪了过来，「向上」那一格空了出来。");
    EXPECT_EQ(say(KeyChange::TradedWith, Key::Up), "这个键原先是「向上」的，两边互换了。");
    EXPECT_EQ(say(KeyChange::MovedSlot, Key::Count), "从这个动作的另一格挪到了这一格，那一格空了出来。");
    EXPECT_EQ(say(KeyChange::AlreadyEmpty, Key::Count), "这一格本来就空着。");
    // 与它们容易混的那几句也钉住：本动作两格互换、就是这个键、清空、只剩一个键。
    EXPECT_EQ(say(KeyChange::SwappedSlots, Key::Count), "与这个动作的另一格互换了。");
    EXPECT_EQ(say(KeyChange::Unchanged, Key::Count), "就是这个键，没有改动。");
    EXPECT_EQ(say(KeyChange::Cleared, Key::Count), "这一格清空了。");
    EXPECT_EQ(say(KeyChange::RejectedLastKey, Key::Action), "「路径行动」只剩这一个键，先给它另配一个。");
}
