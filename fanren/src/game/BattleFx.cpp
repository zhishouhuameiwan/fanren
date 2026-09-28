#include "game/BattleFx.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>

#include "core/model/AttackCategory.h"

namespace fanren::game {

using core::battle::Unit;

namespace {

// ---- 站位的数 ----
// 我方：自上而下往右斜，三个人时脚底在 y = 420 / 490 / 560，最下面那位仍在队伍面板之上。
constexpr float kAllyX = 910.f;   // 最前面那位往前站半步（12）之后，左肩仍在命令菜单右缘之外
constexpr float kAllyStepX = 64.f;
constexpr float kAllyMidY = 490.f;
constexpr float kAllyStepY = 70.f;
constexpr float kAllyStepYCrowded = 56.f;   // 四个人以上时收紧，最下面那位别踩进面板
// 敌人：前后两排交错（后排 y 445、前排 y 572），前排插在后排两人之间的空当里。
// 为什么不是三人一列的斜队：每人脚下还压着一行架势 / 破绽（约 50 像素），三排一列时上一排那一行
// 正好盖住下一排的上半身（截图里看得清清楚楚）；两排交错时它落在前排两人的肩膀之间。
constexpr float kEnemyX = 450.f;               // 最靠我方的那一位；再往右就钻进命令菜单底下了
constexpr float kEnemyBackY = 445.f;
constexpr float kEnemyFrontY = 572.f;
constexpr float kEnemySpacing = 190.f;         // 同一排相邻两人
constexpr float kEnemySpacingCrowded = 150.f;  // 五个以上时收紧，最左边那位别出画
constexpr StagePoint kEnemyAlone{400.f, 520.f};

// ---- 前冲（lungeReach）----
constexpr float kLungeDistance = 70.f;   // 往对面冲多远：刀光落下时人已经站在前面了
constexpr float kLungeFloorY = 600.f;    // 往下让最多让到这里：队伍面板上沿 588，脚尖压进去一点点只在冲的那一下

// ---- 特效贴图表（assets/art/fx/index.json）----
constexpr std::array<FxSheet, static_cast<std::size_t>(FxSheetId::Count)> kFxSheets{{
    {"slash.png", 48, 48, 4, 1, 16.f, true},
    {"impact.png", 48, 48, 3, 1, 14.f, true},
    {"fireburst.png", 48, 48, 4, 1, 12.f, false},
    {"wind.png", 48, 48, 3, 1, 12.f, true},
    {"poison.png", 48, 48, 3, 1, 6.f, false},
    {"heal.png", 48, 48, 3, 1, 8.f, true},
    {"spark.png", 8, 8, 3, 1, 12.f, true},
    {"glow.png", 64, 64, 1, 1, 0.f, true},
    {"shards.png", 12, 12, 6, 1, 0.f, false},
    {"boost_aura.png", 32, 40, 3, 3, 10.f, true},
    {"boss_aura.png", 32, 40, 3, 1, 8.f, false},
    {"shadow.png", 24, 8, 1, 1, 0.f, false},
    {"fog.png", 128, 128, 1, 1, 0.f, false},
}};

// index.json 里给白 / 灰度图的建议乘色。
constexpr engine::Color kWhite{255, 255, 255, 255};
constexpr engine::Color kGoldLight{255, 222, 140, 255};
constexpr engine::Color kPoisonTint{138, 111, 168, 255};   // #8A6FA8
constexpr engine::Color kWindTint{207, 232, 224, 255};     // #CFE8E0
constexpr engine::Color kWoodTint{168, 226, 150, 255};
constexpr engine::Color kIceTint{170, 212, 255, 255};
constexpr engine::Color kEarthTint{226, 184, 128, 255};
constexpr engine::Color kSteelTint{210, 220, 232, 255};

[[nodiscard]] float clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

// 碎屏用的一个确定性的小哈希（splitmix 的尾巴）：不耗任何人的随机数。
[[nodiscard]] float hash01(std::uint32_t seed, std::uint32_t a, std::uint32_t b) {
    std::uint32_t x = seed ^ (a * 0x9E3779B9u) ^ (b * 0x85EBCA6Bu);
    x ^= x >> 16U;
    x *= 0x7FEB352Du;
    x ^= x >> 15U;
    x *= 0x846CA68Bu;
    x ^= x >> 16U;
    return static_cast<float>(x & 0xFFFFFFu) / static_cast<float>(0x1000000u);
}

}  // namespace

// ---------------------------------------------------------------------------
// 站位
// ---------------------------------------------------------------------------

StagePoint standPoint(bool ally, int rank, int count) {
    const int n = std::max(1, count);
    if (ally) {
        const float step = n >= 4 ? kAllyStepYCrowded : kAllyStepY;
        return StagePoint{kAllyX + static_cast<float>(rank) * kAllyStepX,
                          kAllyMidY - static_cast<float>(n - 1) * step / 2.f + static_cast<float>(rank) * step};
    }
    if (n == 1) return kEnemyAlone;
    const float spacing = n >= 5 ? kEnemySpacingCrowded : kEnemySpacing;
    const int column = rank / 2;
    const bool front = rank % 2 == 1;
    return StagePoint{kEnemyX - static_cast<float>(column) * spacing - (front ? spacing / 2.f : 0.f),
                      front ? kEnemyFrontY : kEnemyBackY};
}

std::vector<StagePoint> standPoints(const std::vector<Unit>& units) {
    std::vector<StagePoint> out(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        const Unit& me = units[i];
        int rank = 0;
        int count = 0;
        for (std::size_t j = 0; j < units.size(); ++j) {
            const Unit& other = units[j];
            if (other.ally != me.ally || (!me.ally && other.wave != me.wave)) continue;
            if (j < i) ++rank;
            ++count;
        }
        out[i] = standPoint(me.ally, rank, count);
    }
    return out;
}

StagePoint lungeReach(const std::vector<Unit>& units, const std::vector<StagePoint>& stand,
                      const std::vector<StageBody>& bodies, std::size_t who) {
    const Unit& me = units[who];
    if (!me.ally) return StagePoint{kLungeDistance, 0.f};
    const StagePoint at = stand[who];
    const StageBody body = bodies[who];
    float side = 0.f;                 // 往下让多少
    float stop = kLungeDistance;      // 让不开时最多冲多远（冲到挡路那位身侧为止）
    for (std::size_t j = 0; j < units.size(); ++j) {
        if (j == who || !units[j].ally) continue;
        const float ahead = at.x - stand[j].x;                    // 他在我前面（左边）多远
        const float rise = at.y - stand[j].y;                     // 他比我靠上（离镜头远）多少
        const float apart = (body.w + bodies[j].w) / 2.f;         // 两人横向差这么多身子才不叠
        // 只躲前面、冲得到、身子叠得上的人。比我靠下的（离镜头近）从他身后过：他画在我上面，读起来是「走在他后头」。
        if (ahead <= 0.f || ahead >= kLungeDistance + apart || rise < 0.f || rise >= body.h) continue;
        side = std::max(side, body.h - rise);   // 让到我的头顶低过他的脚底
        stop = std::min(stop, ahead - apart);
    }
    const float room = std::max(0.f, kLungeFloorY - at.y);
    if (side <= room) return StagePoint{-kLungeDistance, side};
    return StagePoint{-std::max(0.f, stop), room};
}

// ---------------------------------------------------------------------------
// 特效贴图
// ---------------------------------------------------------------------------

const FxSheet& fxSheet(FxSheetId id) { return kFxSheets[static_cast<std::size_t>(id)]; }

engine::RectF fxFrameRect(const FxSheet& sheet, int row, float seconds, bool loop) {
    int frame = 0;
    if (sheet.fps > 0.f && sheet.frames > 1) {
        frame = static_cast<int>(std::floor(std::max(0.f, seconds) * sheet.fps));
        frame = loop ? frame % sheet.frames : std::min(frame, sheet.frames - 1);
    }
    const int r = std::clamp(row, 0, sheet.rows - 1);
    return engine::RectF{static_cast<float>(frame * sheet.frameW), static_cast<float>(r * sheet.frameH),
                         static_cast<float>(sheet.frameW), static_cast<float>(sheet.frameH)};
}

// ---------------------------------------------------------------------------
// 按类别挑特效
// ---------------------------------------------------------------------------

StrikeLook strikeLook(int category) {
    using namespace core;
    // 次序即优先级：元素的光效最醒目，其次是毒，兵刃最后。
    if ((category & kCategoryFire) != 0) return {FxSheetId::FireBurst, kWhite, 3.4f, "hit_blunt"};
    if ((category & kCategoryMetal) != 0) return {FxSheetId::Slash, kGoldLight, 3.8f, "hit_slash"};
    if ((category & kCategoryPoison) != 0) return {FxSheetId::Poison, kPoisonTint, 3.f, "poison"};
    if ((category & kCategoryWood) != 0) return {FxSheetId::Wind, kWoodTint, 3.2f, "hit_blunt"};
    if ((category & kCategoryWater) != 0) return {FxSheetId::Impact, kIceTint, 3.2f, "hit_blunt"};
    if ((category & kCategoryEarth) != 0) return {FxSheetId::Impact, kEarthTint, 3.4f, "hit_blunt"};
    if ((category & (kCategorySword | kCategoryBlade)) != 0) return {FxSheetId::Slash, kWhite, 3.2f, "hit_slash"};
    if ((category & kCategoryHidden) != 0) return {FxSheetId::Slash, kSteelTint, 2.4f, "hit_slash"};
    if ((category & kCategoryFist) != 0) return {FxSheetId::Impact, kWhite, 3.f, "hit_blunt"};
    return {FxSheetId::Wind, kWindTint, 3.2f, "hit_blunt"};
}

const char* castSfx(int category) {
    using namespace core;
    if ((category & kCategoryFire) != 0) return "fire_cast";
    if ((category & kCategoryMetal) != 0) return "metal_cast";
    if ((category & kCategoryPoison) != 0) return "poison";
    return "wind_cast";
}

// ---------------------------------------------------------------------------
// 曲线
// ---------------------------------------------------------------------------

float easeOutCubic(float t) {
    const float u = 1.f - clamp01(t);
    return 1.f - u * u * u;
}

float smoothStep(float edge0, float edge1, float x) {
    const float t = clamp01((x - edge0) / (edge1 - edge0));
    return t * t * (3.f - 2.f * t);
}

float numberLift(float t) {
    // 蹿 0.14 秒、落 0.16 秒、小弹 0.10 秒。数只管「一眼看得出是弹了一下」：蹿得太高会压到头顶的飘字，
    // 小弹太大就像是又挨了一下。
    constexpr float kRise = 0.14f;
    constexpr float kFall = 0.16f;
    constexpr float kBounce = 0.10f;
    constexpr float kPeak = 38.f;
    constexpr float kBouncePeak = 9.f;
    if (t <= 0.f) return 0.f;
    if (t < kRise) return kPeak * easeOutCubic(t / kRise);
    if (t < kRise + kFall) {
        const float u = (t - kRise) / kFall;
        return kPeak * (1.f - u * u);
    }
    if (t < kRise + kFall + kBounce) {
        return kBouncePeak * std::sin(std::numbers::pi_v<float> * (t - kRise - kFall) / kBounce);
    }
    return 0.f;
}

float numberPop(float t) {
    constexpr float kPop = 0.10f;
    if (t >= kPop) return 1.f;
    return 1.45f - 0.45f * easeOutCubic(std::max(0.f, t) / kPop);
}

float floaterAlpha(float t, float life) {
    constexpr float kFade = 0.25f;
    if (t >= life) return 0.f;
    return clamp01((life - t) / kFade);
}

float shakeEnvelope(float t, float duration) {
    if (t <= 0.f) return 1.f;
    if (t >= duration) return 0.f;
    const float u = 1.f - t / duration;
    return u * u;
}

StagePoint shakeOffset(float t, float duration, float amplitude) {
    const float k = amplitude * shakeEnvelope(t, duration);
    return StagePoint{k * 0.5f * (std::sin(t * 71.f) + std::sin(t * 113.f + 1.3f)),
                      k * 0.5f * (std::cos(t * 89.f + 0.7f) + std::sin(t * 131.f))};
}

// ---------------------------------------------------------------------------
// BGM
// ---------------------------------------------------------------------------

bool looksLikeBoss(const Unit& unit) {
    return unit.actions > 1 || unit.chargeEvery > 0 || unit.maxToughness >= 8;
}

std::string battleBgm(const std::vector<Unit>& units, bool mind) {
    if (mind) return "bgm_soulsea";
    const bool boss = std::any_of(units.begin(), units.end(), [](const Unit& u) {
        return !u.ally && (u.actions >= 2 || u.chargeEvery > 0);
    });
    return boss ? "bgm_boss" : "bgm_battle";
}

// ---------------------------------------------------------------------------
// 背景
// ---------------------------------------------------------------------------

std::string resolveBackdrop(const std::string& setupBackdrop, const std::string& terrain,
                            const std::string& visualOverride, const std::string& mapBackdrop) {
    if (!setupBackdrop.empty()) return setupBackdrop;
    if (!visualOverride.empty()) return visualOverride;
    if (terrain == "mind") return "soulsea";
    if (!mapBackdrop.empty()) return mapBackdrop;
    // 兜底：地形名 → 最像的那一套。连地图都认不出的（兜底遭遇、截图口没给 --map）才走到这里。
    if (terrain == "cave") return "cave_tunnel";
    if (terrain == "indoor") return "inn_hall";
    if (terrain == "secret") return "secret_room";
    if (terrain == "battlefield") return "drill_ground";
    return "mountain_forest";
}

BackdropLook backdropLook(const std::string& backdrop) {
    using core::TimeOfDay;
    using engine::ParticleKind;
    const auto endsWith = [&backdrop](const char* suffix) {
        const std::string s(suffix);
        return backdrop.size() >= s.size() && backdrop.compare(backdrop.size() - s.size(), s.size(), s) == 0;
    };
    if (backdrop == "soulsea") return {TimeOfDay::Night, true, ParticleKind::Dust, 26.f, true};
    if (backdrop == "cave_tunnel") return {TimeOfDay::Indoor, false, ParticleKind::Ember, 5.f, true};
    if (backdrop == "inn_hall" || backdrop == "secret_room") {
        return {TimeOfDay::Indoor, false, ParticleKind::Dust, 8.f, false};
    }
    if (backdrop == "wild_manor" || backdrop == "manor_night") {
        return {TimeOfDay::Night, false, ParticleKind::Firefly, 5.f, true};
    }
    if (backdrop == "drill_ground_night") return {TimeOfDay::Night, false, ParticleKind::Ember, 6.f, true};
    if (backdrop == "dock_river") return {TimeOfDay::Dusk, false, ParticleKind::Mist, 1.2f, true};
    if (endsWith("_dusk")) {
        const bool forest = backdrop.rfind("mountain_forest", 0) == 0;
        return {TimeOfDay::Dusk, false, forest ? ParticleKind::Firefly : ParticleKind::Dust, forest ? 5.f : 6.f,
                true};
    }
    if (backdrop == "mountain_forest") return {TimeOfDay::Day, false, ParticleKind::Leaf, 3.f, false};
    if (backdrop == "sect_courtyard" || backdrop == "manor_court" || backdrop == "village_road") {
        return {TimeOfDay::Day, false, ParticleKind::Petal, 2.5f, false};
    }
    return {TimeOfDay::Day, false, ParticleKind::Dust, 6.f, false};
}

// ---------------------------------------------------------------------------
// 行动序条
// ---------------------------------------------------------------------------

OrderStrip layoutOrderStrip(const std::vector<int>& thisRound, const std::vector<int>& nextRound, float left,
                            float right) {
    OrderStrip strip;
    float x = left;
    bool full = false;
    const auto place = [&](int unit, float size, bool current, bool next) {
        if (full || x + size > right) {
            full = true;
            return;
        }
        strip.chips.push_back(OrderChip{unit, x, kOrderBaseline - size, size, current, next});
        x += size + kOrderGap;
    };
    for (std::size_t i = 0; i < thisRound.size(); ++i) {
        place(thisRound[i], i == 0 ? kOrderChipCurrent : kOrderChip, i == 0, false);
    }
    if (full || x + kOrderDivider > right) return strip;
    strip.dividerX = x + kOrderDivider / 2.f - kOrderGap / 2.f;
    x += kOrderDivider;
    for (const int unit : nextRound) place(unit, kOrderChipNext, false, true);
    return strip;
}

// ---------------------------------------------------------------------------
// 开战碎屏
// ---------------------------------------------------------------------------

std::vector<ShatterPiece> shatterPieces(int cols, int rows, std::uint32_t seed) {
    const float w = static_cast<float>(engine::kLogicalWidth);
    const float h = static_cast<float>(engine::kLogicalHeight);
    const int c = std::max(1, cols);
    const int r = std::max(1, rows);
    // 网格点：边上的点只沿边挪（碎片必须严丝合缝地铺满整屏），里面的点两向都挪。
    std::vector<StagePoint> grid(static_cast<std::size_t>((c + 1) * (r + 1)));
    const float cellW = w / static_cast<float>(c);
    const float cellH = h / static_cast<float>(r);
    for (int gy = 0; gy <= r; ++gy) {
        for (int gx = 0; gx <= c; ++gx) {
            float x = static_cast<float>(gx) * cellW;
            float y = static_cast<float>(gy) * cellH;
            const auto ux = static_cast<std::uint32_t>(gx);
            const auto uy = static_cast<std::uint32_t>(gy);
            // 挪动不超过格宽的 ±0.2：超过 0.25 时一个角能被推进对面三角里，四边形凹下去，
            // 沿「错的那条」对角线劈开的两片就会叠在一起（单测按面积和钉着）。
            if (gx > 0 && gx < c) x += (hash01(seed, ux, uy * 2U) - 0.5f) * cellW * 0.4f;
            if (gy > 0 && gy < r) y += (hash01(seed, ux, uy * 2U + 1U) - 0.5f) * cellH * 0.4f;
            grid[static_cast<std::size_t>(gy * (c + 1) + gx)] = StagePoint{x, y};
        }
    }
    const StagePoint center{w / 2.f, h / 2.f};
    std::vector<ShatterPiece> pieces;
    pieces.reserve(static_cast<std::size_t>(c * r * 2));
    const auto at = [&](int gx, int gy) { return grid[static_cast<std::size_t>(gy * (c + 1) + gx)]; };
    for (int gy = 0; gy < r; ++gy) {
        for (int gx = 0; gx < c; ++gx) {
            const StagePoint a = at(gx, gy);
            const StagePoint b = at(gx + 1, gy);
            const StagePoint cc = at(gx + 1, gy + 1);
            const StagePoint d = at(gx, gy + 1);
            const auto ux = static_cast<std::uint32_t>(gx);
            const auto uy = static_cast<std::uint32_t>(gy);
            const bool flip = hash01(seed ^ 0xA5A5u, ux, uy) < 0.5f;
            const std::array<std::array<StagePoint, 3>, 2> tris =
                flip ? std::array<std::array<StagePoint, 3>, 2>{{{a, b, cc}, {a, cc, d}}}
                     : std::array<std::array<StagePoint, 3>, 2>{{{a, b, d}, {b, cc, d}}};
            for (std::size_t t = 0; t < tris.size(); ++t) {
                ShatterPiece piece;
                piece.corner = tris[t];
                StagePoint mid{};
                for (std::size_t k = 0; k < 3; ++k) {
                    piece.uv[k] = StagePoint{piece.corner[k].x / w, piece.corner[k].y / h};
                    mid.x += piece.corner[k].x / 3.f;
                    mid.y += piece.corner[k].y / 3.f;
                }
                const float dx = mid.x - center.x;
                const float dy = mid.y - center.y;
                const float dist = std::max(1.f, std::sqrt(dx * dx + dy * dy));
                const auto salt = static_cast<std::uint32_t>(t) + 7U;
                const float speed = 320.f + 420.f * hash01(seed, ux * 31U + salt, uy);
                piece.velocity = StagePoint{dx / dist * speed, dy / dist * speed - 160.f};
                piece.spin = (hash01(seed, uy * 17U + salt, ux) - 0.5f) * 720.f;
                piece.delay = dist * 0.00032f;
                pieces.push_back(piece);
            }
        }
    }
    return pieces;
}

std::array<StagePoint, 3> shatterPose(const ShatterPiece& piece, float t) {
    constexpr float kGravity = 1400.f;
    const float u = std::max(0.f, t - piece.delay);
    StagePoint mid{};
    for (const StagePoint& p : piece.corner) {
        mid.x += p.x / 3.f;
        mid.y += p.y / 3.f;
    }
    const float moveX = piece.velocity.x * u;
    const float moveY = piece.velocity.y * u + 0.5f * kGravity * u * u;
    const float angle = piece.spin * u * std::numbers::pi_v<float> / 180.f;
    const float cs = std::cos(angle);
    const float sn = std::sin(angle);
    // 崩开时往里缩一点：三角之间露出缝，一眼看得出是「碎了」而不是整张在平移。
    const float shrink = 1.f - 0.25f * smoothStep(0.f, 0.5f, u);
    std::array<StagePoint, 3> out{};
    for (std::size_t k = 0; k < 3; ++k) {
        const float rx = (piece.corner[k].x - mid.x) * shrink;
        const float ry = (piece.corner[k].y - mid.y) * shrink;
        out[k] = StagePoint{mid.x + moveX + rx * cs - ry * sn, mid.y + moveY + rx * sn + ry * cs};
    }
    return out;
}

float shatterAlpha(const ShatterPiece& piece, float t) {
    const float u = t - piece.delay;
    return 1.f - smoothStep(0.35f, 0.7f, u);
}

}  // namespace fanren::game
