#include "game/WorldProps.h"

#include <cmath>
#include <cstdio>
#include <set>

#include "game/WorldView.h"

namespace fanren::game {
namespace {

[[nodiscard]] float fl(int v) { return static_cast<float>(v); }

// 对齐到屏幕像素，与 WorldView 里的 snap 同一个口径：像素美术的边不落在半像素上。
[[nodiscard]] float snap(float v) { return std::round(v); }

// 占位底板的颜色。木箱是暗木色、开过的更暗；采集点是草绿、采过的发灰；机关是石青；明雷标记是血红 / 精英暗金。
constexpr engine::Color kChestPlate{164, 112, 64, 255};
constexpr engine::Color kChestOpenPlate{104, 76, 52, 255};
constexpr engine::Color kGatherPlate{104, 168, 84, 255};
constexpr engine::Color kGatherSpentPlate{112, 116, 100, 255};
constexpr engine::Color kGatePlate{92, 112, 136, 255};
constexpr engine::Color kGateOpenEdge{150, 170, 190, 90};
constexpr engine::Color kFoeMarkPlate{196, 48, 40, 255};
constexpr engine::Color kFoeMarkElitePlate{214, 160, 40, 255};
constexpr engine::Color kFoeAlertPlate{236, 200, 64, 255};

std::set<std::string>& worldWarnings() {
    static std::set<std::string> warned;
    return warned;
}

}  // namespace

bool warnWorldOnce(const std::string& key, const std::string& message) {
    if (!worldWarnings().insert(key).second) return false;
    std::fprintf(stderr, "[world] WARNING: %s\n", message.c_str());
    return true;
}

void forgetWorldWarnings() { worldWarnings().clear(); }

const char* chestSprite(bool opened) { return opened ? kChestOpenSprite : kChestClosedSprite; }
const char* gatherSprite(bool depleted) { return depleted ? kGatherSpentSprite : kGatherReadySprite; }
const char* gateSprite(bool open) { return open ? kGateOpenSprite : kGateClosedSprite; }
const char* foeMarkSprite(bool elite) { return elite ? kFoeMarkEliteSprite : kFoeMarkSprite; }

bool gateStands(bool open) { return !open; }

PropPlaceholder chestPlaceholder(bool opened) {
    return opened ? PropPlaceholder{kChestOpenPlate, "空", true} : PropPlaceholder{kChestPlate, "箱", true};
}

PropPlaceholder gatherPlaceholder(bool depleted) {
    return depleted ? PropPlaceholder{kGatherSpentPlate, "枯", true} : PropPlaceholder{kGatherPlate, "采", true};
}

PropPlaceholder gatePlaceholder(bool open) {
    return open ? PropPlaceholder{kGateOpenEdge, "", false} : PropPlaceholder{kGatePlate, "关", true};
}

PropPlaceholder foeMarkPlaceholder(bool elite) {
    return elite ? PropPlaceholder{kFoeMarkElitePlate, "凶", true} : PropPlaceholder{kFoeMarkPlate, "敌", true};
}

PropPlaceholder foeAlertPlaceholder() { return PropPlaceholder{kFoeAlertPlate, "！", true}; }

engine::RectF characterDest(const CharacterSheet& sheet, float footX, float footY) {
    const float scale = fl(sheet.scale);
    const float w = fl(sheet.frameW) * scale;
    const float h = fl(sheet.frameH) * scale;
    return engine::RectF{snap(footX - w / 2.f), snap(footY - fl(sheet.footY + 1) * scale), w, h};
}

float characterHeadTop(const CharacterSheet& sheet, float footY) {
    return footY - fl(sheet.footY + 1 - sheet.headY) * fl(sheet.scale);
}

FoeBody foeBodyFor(const SpriteIndex& index, const std::string& look, const ChapterDone& chapterDone) {
    FoeBody body;
    if (look.empty()) return body;
    if (const CharacterSheet* sheet = sheetForRole(index, look, chapterDone)) {
        body.sheet = sheet;
        return body;
    }
    if (const auto it = index.sheets.find(look); it != index.sheets.end()) {
        body.sheet = &it->second;
        return body;
    }
    if (const auto it = index.enemies.find(look); it != index.enemies.end()) body.enemy = &it->second;
    return body;
}

int enemyStandFrame(const core::EnemySheet& sheet) {
    if (const auto it = sheet.battle.find("idle"); it != sheet.battle.end() && !it->second.empty()) {
        return it->second.front();
    }
    return 0;
}

engine::RectF enemyFrameRect(const core::EnemySheet& sheet, int frame) {
    const int cols = sheet.cols > 0 ? sheet.cols : 1;
    return engine::RectF{fl((frame % cols) * sheet.frameW), fl((frame / cols) * sheet.frameH), fl(sheet.frameW),
                         fl(sheet.frameH)};
}

bool enemyFlipped(const core::EnemySheet& sheet, int facing) {
    const int dir = ((facing % 4) + 4) % 4;
    if (sheet.battleFacing == core::BattleFacing::None) return false;
    if (dir == 1) return sheet.battleFacing == core::BattleFacing::Left;
    if (dir == 3) return sheet.battleFacing == core::BattleFacing::Right;
    return false;
}

engine::RectF enemyDest(const core::EnemySheet& sheet, float footX, float footY) {
    const float scale = fl(sheet.scale);
    const float w = fl(sheet.frameW) * scale;
    const float h = fl(sheet.frameH) * scale;
    return engine::RectF{snap(footX - w / 2.f), snap(footY - fl(sheet.footY + 1) * scale), w, h};
}

float enemyHeadTop(const core::EnemySheet& sheet, float footY) {
    return footY - fl(sheet.footY + 1 - sheet.headY) * fl(sheet.scale);
}

}  // namespace fanren::game
