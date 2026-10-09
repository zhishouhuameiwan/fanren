#include "game/BattleView.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <utility>

#include "core/model/AttackCategory.h"
#include "game/Application.h"
#include "game/MapArt.h"
#include "game/MapVisual.h"
#include "io/VisualLoader.h"
#include "ui/Widgets.h"

namespace fanren::game {

using core::battle::ActionKind;
using core::battle::BattleEvent;
using core::battle::Unit;
using engine::BlendMode;
using engine::Color;
using engine::DrawOptions;
using engine::RectF;

namespace {

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);
constexpr float kPi = std::numbers::pi_v<float>;

// ---- 动作的时长（戏里秒）----
// 前冲 0.14 秒到位：Act 的节拍是 0.3 秒，刀光落下时人已经站在前面了。退回慢一点，看得出是收势。
constexpr float kLungeOut = 0.14f;
constexpr float kLungeBack = 0.2f;
constexpr float kHurt = 0.28f;
constexpr float kHitFlash = 0.12f;
constexpr float kKnock = 0.22f;
constexpr float kKnockDistance = 14.f;
constexpr float kDissolve = 0.6f;
constexpr float kFlee = 0.5f;
constexpr float kEnter = 0.5f;
constexpr float kNumberLife = 1.0f;
constexpr float kLabelLife = 0.9f;
// ---- 破势（实秒，只乘快进）----
// 慢镜 0.3 秒、时间流速 0.3：碎片崩开的那一下拉长到看得清；白闪 0.16 秒；震屏半秒内衰减完。
constexpr float kSlowmo = 0.3f;
constexpr float kSlowmoScale = 0.3f;
constexpr float kSlowmoRamp = 0.08f;
constexpr float kWhiteFlash = 0.16f;
constexpr float kBreakText = 0.95f;       // 戏里秒
constexpr float kBreakRing = 0.4f;

// ---- 手柄震动（docs/gamepad.md 第 8 节）----
// 只在两处震：破势（谁被破都算），我方挨了敌人的重击（一下掉气血上限的四分之一以上，或被这一下打倒）。
// 不跟震屏一起震：震屏每一击都有（3–5 像素），手柄每一击都震就成了噪音。与「战斗震屏」开关互不相干——
// 震不震只看 Engine::rumble 那一道闸（开关、设备是不是手柄）。
constexpr float kBreakRumbleLow = 0.8f;
constexpr float kBreakRumbleHigh = 0.6f;
constexpr int kBreakRumbleMs = 280;
constexpr float kHeavyHitRumbleLow = 0.5f;
constexpr float kHeavyHitRumbleHigh = 0.35f;
constexpr int kHeavyHitRumbleMs = 180;

// ---- 颜色 ----
constexpr Color kWhite{255, 255, 255, 255};
constexpr Color kNumber{246, 240, 226, 255};
constexpr Color kNumberWeak{255, 214, 90, 255};     // 打中破绽：黄
constexpr Color kNumberBroken{242, 88, 62, 255};    // 破势中：红
constexpr Color kNumberEcho{120, 20, 10, 255};
constexpr Color kNumberHeal{140, 232, 172, 255};
constexpr Color kNumberPoison{196, 146, 236, 255};
constexpr Color kShardTint{191, 227, 210, 255};     // #BFE3D2 碎玉
constexpr Color kGoldGlow{255, 214, 120, 255};
constexpr Color kHealTint{159, 224, 184, 255};      // #9FE0B8
constexpr Color kPoisonTint{138, 111, 168, 255};
constexpr Color kFireLight{255, 150, 70, 255};
constexpr Color kCinnabarGlow{220, 70, 50, 255};
constexpr Color kBrokenTint{196, 196, 214, 255};
constexpr Color kDownTint{150, 150, 162, 255};

[[nodiscard]] Color alphaOf(Color c, float alpha) {
    // 透明度按 1/16 取整：逐帧变色的字每一档都要新建一张纹理（引擎契约 2.5），取整之后一条字最多十几张。
    const float q = std::round(std::clamp(alpha, 0.f, 1.f) * 16.f) / 16.f;
    c.a = static_cast<std::uint8_t>(std::lround(static_cast<float>(c.a) * q));
    return c;
}

[[nodiscard]] float approach(float value, float target, float step) {
    return value < target ? std::min(target, value + step) : std::max(target, value - step);
}

[[nodiscard]] std::uint32_t seedOf(const std::string& text) {
    std::uint32_t seed = 2166136261u;
    for (const char ch : text) seed = (seed ^ static_cast<std::uint8_t>(ch)) * 16777619u;
    return seed;
}

// 一圈柔边的椭圆环：中线最亮、内外两侧淡到透明。顶点直接追加进 mesh（三个一组，不用下标）。
void appendRing(std::vector<engine::Vertex>& mesh, float cx, float cy, float rx, float ry, float width,
                Color color) {
    constexpr int kSegments = 40;
    const Color clear{color.r, color.g, color.b, 0};
    const float half = width / 2.f;
    const float aspect = ry / std::max(1.f, rx);
    const auto point = [&](int k, float r) {
        const float a = 2.f * kPi * static_cast<float>(k) / static_cast<float>(kSegments);
        return StagePoint{cx + std::cos(a) * r, cy + std::sin(a) * r * aspect};
    };
    const std::array<std::pair<float, float>, 2> bands{{{rx - half, rx}, {rx, rx + half}}};
    for (std::size_t b = 0; b < bands.size(); ++b) {
        const float r0 = bands[b].first;
        const float r1 = bands[b].second;
        const Color c0 = b == 0 ? clear : color;
        const Color c1 = b == 0 ? color : clear;
        for (int k = 0; k < kSegments; ++k) {
            const StagePoint a0 = point(k, r0);
            const StagePoint a1 = point(k + 1, r0);
            const StagePoint b0 = point(k, r1);
            const StagePoint b1 = point(k + 1, r1);
            mesh.push_back({a0.x, a0.y, c0, 0.f, 0.f});
            mesh.push_back({b0.x, b0.y, c1, 0.f, 0.f});
            mesh.push_back({b1.x, b1.y, c1, 0.f, 0.f});
            mesh.push_back({a0.x, a0.y, c0, 0.f, 0.f});
            mesh.push_back({b1.x, b1.y, c1, 0.f, 0.f});
            mesh.push_back({a1.x, a1.y, c0, 0.f, 0.f});
        }
    }
}

[[nodiscard]] RectF lookFrameRect(const BattleLook& look, int frame) {
    const int cols = std::max(1, look.cols);
    return RectF{static_cast<float>((frame % cols) * look.frameW), static_cast<float>((frame / cols) * look.frameH),
                 static_cast<float>(look.frameW), static_cast<float>(look.frameH)};
}

}  // namespace

// ---------------------------------------------------------------------------
// 外观
// ---------------------------------------------------------------------------

BattleLook battleLookFor(const SpriteIndex& index, const std::string& battleId, const std::string& roleId,
                         const ChapterDone& chapterDone) {
    BattleLook look;
    const auto fromSheet = [&look](const CharacterSheet& sheet) {
        look.file = sheet.file;
        look.frameW = sheet.frameW;
        look.frameH = sheet.frameH;
        look.cols = sheet.cols;
        look.footY = sheet.footY;
        look.headY = sheet.headY;
        look.poses = sheet.battle;
        look.facing = sheet.battleFacing;
        look.floating = false;
        look.humanoid = true;
    };
    const auto fromEnemy = [&look](const core::EnemySheet& sheet) {
        look.file = sheet.file;
        look.frameW = sheet.frameW;
        look.frameH = sheet.frameH;
        look.cols = sheet.cols;
        look.footY = sheet.footY;
        look.headY = sheet.headY;
        look.poses = sheet.battle;
        look.facing = sheet.battleFacing;
        look.floating = sheet.floating;
        look.humanoid = false;
    };
    // 按战斗覆写的外观（识海之战里韩立是光球）：外观 id 可能在人物表里，也可能在非人形表里。
    if (const auto battle = index.battleOverrides.find(battleId); battle != index.battleOverrides.end()) {
        if (const auto role = battle->second.find(roleId); role != battle->second.end()) {
            if (const auto sheet = index.sheets.find(role->second); sheet != index.sheets.end()) {
                fromSheet(sheet->second);
                return look;
            }
            if (const auto enemy = index.enemies.find(role->second); enemy != index.enemies.end()) {
                fromEnemy(enemy->second);
                return look;
            }
        }
    }
    if (const CharacterSheet* sheet = sheetForRole(index, roleId, chapterDone); sheet != nullptr) {
        fromSheet(*sheet);
        return look;
    }
    if (const auto enemy = index.enemies.find(roleId); enemy != index.enemies.end()) fromEnemy(enemy->second);
    return look;
}

int battleFrame(const BattleLook& look, const std::string& pose, float seconds) {
    auto found = look.poses.find(pose);
    if (found == look.poses.end() || found->second.empty()) found = look.poses.find("idle");
    if (found == look.poses.end() || found->second.empty()) return 0;
    const std::vector<int>& frames = found->second;
    if (frames.size() == 1) return frames.front();
    // 待机：人与兽两帧一口气（2.5 帧 / 秒），光球四帧一脉动（6 帧 / 秒）。
    const float fps = look.floating ? 6.f : 2.5f;
    const auto i = static_cast<std::size_t>(std::floor(std::max(0.f, seconds) * fps));
    return frames[i % frames.size()];
}

bool battleFlip(const BattleLook& look, bool ally) {
    if (look.facing == core::BattleFacing::None) return false;
    const core::BattleFacing wanted = ally ? core::BattleFacing::Left : core::BattleFacing::Right;
    return look.facing != wanted;
}

float battleScale(bool boss) { return boss ? 5.f : 4.f; }

// ---------------------------------------------------------------------------
// 内部结构
// ---------------------------------------------------------------------------

struct BattleView::Actor {
    BattleLook look;
    engine::TextureId tex = engine::kInvalidTexture;
    float scale = 4.f;
    bool flip = false;
    bool ally = false;
    float phase = 0.f;          // 待机错开的相位：一排人不齐刷刷地一起喘气
    float lunge = 0.f;          // 0 在原位，1 冲到位
    StagePoint reach{};         // 冲到位时相对站位的位移（lungeReach：前面挡着同伴就先往下让）
    bool lungeOut = false;
    bool casting = false;
    float hurt = 0.f;
    float flash = 0.f;
    float knock = 0.f;
    float dissolve = -1.f;      // ≥ 0：正在倒下溶解（秒）
    float flee = -1.f;
    float enter = -1.f;
    int aura = 0;               // 这一手蓄了几点（气焰档位）
    float auraFlare = 0.f;
    float chargeFlare = 0.f;
    HudPulse hud;
};

struct BattleView::Effect {
    FxSheetId sheet = FxSheetId::Impact;
    StagePoint at{};
    float scale = 3.f;
    Color tint{255, 255, 255, 255};
    bool flip = false;
    float angle = 0.f;
    float age = 0.f;
    float life = 0.3f;
};

struct BattleView::Mote {
    FxSheetId sheet = FxSheetId::Spark;
    StagePoint at{};
    StagePoint velocity{};
    float angle = 0.f;
    float spin = 0.f;
    float gravity = 0.f;
    float age = 0.f;
    float life = 0.5f;
    float size = 16.f;
    int frame = 0;
    Color tint{255, 255, 255, 255};
};

struct BattleView::Floater {
    int unit = -1;
    std::string text;
    Color color{};
    float age = 0.f;
    bool number = false;        // 伤害 / 回复数字：弹跳；否则是飘字：慢慢上浮
    bool weak = false;
    bool broken = false;
    float dx = 0.f;
    float dy = 0.f;
};

struct BattleView::Flare {
    StagePoint at{};
    float radius = 300.f;
    Color color{};
    float age = 0.f;
    float life = 0.4f;
};

// ---------------------------------------------------------------------------
// 建台
// ---------------------------------------------------------------------------

BattleView::BattleView(Application& app, const core::battle::BattleState& battle, const std::string& battleId,
                       const std::string& backdrop)
    : eng_(app.engine()),
      battle_(battle),
      post_(std::make_unique<engine::PostFx>(app.engine())),
      air_(seedOf(battleId)),
      motes2_(seedOf(battleId) ^ 0x5bd1e995u),
      backdrop_(backdrop),
      look_(backdropLook(backdrop)),
      breakWord_(app.text("ui.battle.break")),
      weakWord_(app.text("ui.battle.weak")),
      shakeEnabled_(app.settings().screenShake) {
    engine::Engine& e = eng_;
    loadBackdrop(e);
    for (std::size_t i = 0; i < fx_.size(); ++i) {
        const auto id = static_cast<FxSheetId>(i);
        // 柔图（光斑、雾、柔影）要线性取样，像素图保持像素。
        const bool soft = id == FxSheetId::Glow || id == FxSheetId::Fog || id == FxSheetId::Shadow;
        fx_[i] = e.loadTexture(std::string("art/fx/") + fxSheet(id).file,
                               soft ? engine::ScaleMode::Linear : engine::ScaleMode::Pixel);
    }

    // 精灵索引：与世界画面、主菜单读的是同一张表、同一条选外观的规则（game/SpriteAtlas.h）。
    SpriteIndex index;
    if (const std::vector<std::string> found = e.findAssets("art/sprites", "index.json"); !found.empty()) {
        if (core::Result<SpriteIndex> loaded = io::loadSpriteIndex(found.front()); loaded) {
            index = std::move(loaded.value);
        } else {
            std::fprintf(stderr, "[battle] 精灵索引读不了，台上的人退回旧画法：%s\n", loaded.error.c_str());
        }
    }
    const ChapterTable& chapters = app.chapterTable();
    const core::GameState& state = app.state();
    const ChapterDone chapterDone = [&chapters, &state](int n) { return chapters.chapterDone(state, n); };

    const std::vector<Unit>& units = battle_.units();
    anchors_ = standPoints(units);
    actors_.resize(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        Actor& a = actors_[i];
        a.look = battleLookFor(index, battleId, units[i].id, chapterDone);
        a.ally = units[i].ally;
        a.scale = battleScale(!units[i].ally && looksLikeBoss(units[i]));
        a.flip = battleFlip(a.look, units[i].ally);
        a.phase = static_cast<float>(i) * 0.37f;
        if (!a.look.file.empty()) a.tex = e.loadTexture("art/sprites/" + a.look.file);
        if (a.tex == engine::kInvalidTexture) {
            std::fprintf(stderr, "[battle] %s 没有战斗图，退回图元画法\n", units[i].id.c_str());
        }
    }

    // 前冲的落点要知道每个人有多大（曲魂比人高一截），所以等外形都定了再算。
    std::vector<StageBody> bodies(actors_.size());
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        const BattleLook& look = actors_[i].look;
        bodies[i] = StageBody{static_cast<float>(look.frameW) * actors_[i].scale,
                              static_cast<float>(look.footY - look.headY + 1) * actors_[i].scale};
    }
    for (std::size_t i = 0; i < actors_.size(); ++i) actors_[i].reach = lungeReach(units, anchors_, bodies, i);

    // 空气里的东西：尘、萤火、火星、识海的星点。区域的含义见引擎契约第 4 节那张表。
    RectF area{0.f, 140.f, kW, 480.f};
    switch (look_.particle) {
        case engine::ParticleKind::Firefly: area = RectF{0.f, 250.f, kW, 380.f}; break;
        case engine::ParticleKind::Ember: area = RectF{0.f, 420.f, kW, 300.f}; break;
        case engine::ParticleKind::Leaf:
        case engine::ParticleKind::Petal:
        case engine::ParticleKind::Rain:
        case engine::ParticleKind::Snow: area = RectF{0.f, 0.f, kW, kH}; break;
        case engine::ParticleKind::Mist: area = RectF{0.f, 300.f, kW, 300.f}; break;
        case engine::ParticleKind::Dust: break;
    }
    air_.configure(look_.particle, look_.rate, area, engine::ParticleSpace::Screen);
    air_.prewarm(6.f);
    if (look_.soulsea) {
        motes2_.configure(engine::ParticleKind::Firefly, 7.f, RectF{0.f, 120.f, kW, 520.f},
                          engine::ParticleSpace::Screen);
        motes2_.prewarm(6.f);
    }
    pieces_ = shatterPieces(10, 6, seedOf(battleId));
}

BattleView::~BattleView() = default;

void BattleView::loadBackdrop(engine::Engine& e) {
    static constexpr std::array<const char*, 4> kLayers{"sky", "far", "mid", "ground"};
    for (std::size_t i = 0; i < kLayers.size(); ++i) {
        layers_[i] = e.loadTexture("art/battle/" + backdrop_ + "/" + kLayers[i] + ".png");
    }
    if (layers_[0] == engine::kInvalidTexture) {
        std::fprintf(stderr, "[battle] 战斗背景 %s 缺图，退回两色底\n", backdrop_.c_str());
    }
}

bool BattleView::validUnit(int unit) const {
    return unit >= 0 && static_cast<std::size_t>(unit) < actors_.size();
}

engine::TextureId BattleView::fxTexture(FxSheetId id) const { return fx_[static_cast<std::size_t>(id)]; }

const HudPulse& BattleView::pulse(int unit) const { return actors_[static_cast<std::size_t>(unit)].hud; }

// ---------------------------------------------------------------------------
// 钟
// ---------------------------------------------------------------------------

float BattleView::timeScale() const {
    if (slowmo_ <= 0.f) return 1.f;
    if (slowmo_ >= kSlowmoRamp) return kSlowmoScale;
    // 最后一小截缓回常速：慢镜「松开」而不是「断开」。
    return kSlowmoScale + (1.f - kSlowmoScale) * (1.f - slowmo_ / kSlowmoRamp);
}

void BattleView::update(float dt, float real) {
    clock_ += dt;
    realClock_ += real;
    slowmo_ = std::max(0.f, slowmo_ - real);
    whiteFlash_ = std::max(0.f, whiteFlash_ - real);
    shakeClock_ += real;
    if (intro_ == Intro::Running && !introHeld_) {
        introClock_ += real;
        if (introClock_ >= kShatterDuration) intro_ = Intro::Done;
    }
    for (Actor& a : actors_) {
        a.lunge = a.lungeOut ? std::min(1.f, a.lunge + dt / kLungeOut) : std::max(0.f, a.lunge - dt / kLungeBack);
        a.hurt = std::max(0.f, a.hurt - dt);
        a.flash = std::max(0.f, a.flash - dt);
        a.knock = std::max(0.f, a.knock - dt);
        a.auraFlare = std::max(0.f, a.auraFlare - dt);
        a.chargeFlare = std::max(0.f, a.chargeFlare - dt);
        if (a.dissolve >= 0.f) a.dissolve += dt;
        if (a.flee >= 0.f) a.flee += dt;
        if (a.enter >= 0.f) {
            a.enter += dt;
            if (a.enter >= kEnter) a.enter = -1.f;
        }
        a.hud.shieldHit = std::max(0.f, a.hud.shieldHit - dt);
        a.hud.shieldRefill = std::max(0.f, a.hud.shieldRefill - dt);
        a.hud.reveal = std::max(0.f, a.hud.reveal - dt);
    }
    for (Effect& fx : effects_) fx.age += dt;
    std::erase_if(effects_, [](const Effect& fx) { return fx.age >= fx.life; });
    for (Mote& m : motes_) {
        m.age += dt;
        m.velocity.y += m.gravity * dt;
        m.at.x += m.velocity.x * dt;
        m.at.y += m.velocity.y * dt;
        m.angle += m.spin * dt;
    }
    std::erase_if(motes_, [](const Mote& m) { return m.age >= m.life; });
    for (Floater& f : floaters_) f.age += dt;
    std::erase_if(floaters_, [](const Floater& f) { return f.age >= (f.number ? kNumberLife : kLabelLife); });
    for (Flare& f : flares_) f.age += dt;
    std::erase_if(flares_, [](const Flare& f) { return f.age >= f.life; });
    if (breakAge_ >= 0.f) {
        breakAge_ += dt;
        if (breakAge_ >= kBreakText) breakAge_ = -1.f;
    }
    if (ringAge_ >= 0.f) {
        ringAge_ += dt;
        if (ringAge_ >= kBreakRing) ringAge_ = -1.f;
    }
    air_.update(dt);
    if (look_.soulsea) motes2_.update(dt);
}

// ---------------------------------------------------------------------------
// 事件
// ---------------------------------------------------------------------------

void BattleView::addEffect(FxSheetId sheet, StagePoint at, float scale, Color tint, bool flip, float angle,
                           float life) {
    effects_.push_back(Effect{sheet, at, scale, tint, flip, angle, 0.f, life});
}

void BattleView::burst(FxSheetId sheet, StagePoint at, int count, float speedMin, float speedMax, float gravity,
                       float lifeMin, float lifeMax, float size, Color tint) {
    const FxSheet& info = fxSheet(sheet);
    // 方向与快慢用黄金角摊开，不耗随机数：同一刀的碎片每次都一样，截图可复现。
    const float base = static_cast<float>(motes_.size()) * 0.61803f;
    for (int i = 0; i < count; ++i) {
        const float k = static_cast<float>(i) / static_cast<float>(std::max(1, count));
        const float angle = 2.f * kPi * (k + base) + 0.35f * std::sin(static_cast<float>(i) * 12.9898f);
        const float mix = 0.5f + 0.5f * std::sin(static_cast<float>(i) * 78.233f + base);
        const float speed = speedMin + (speedMax - speedMin) * mix;
        Mote m;
        m.sheet = sheet;
        m.at = at;
        // 略往上偏：碎片是被「崩」起来的，不是四面平均地散。
        m.velocity = StagePoint{std::cos(angle) * speed, std::sin(angle) * speed - speed * 0.35f};
        m.angle = 360.f * mix;
        m.spin = (mix - 0.5f) * 900.f;
        m.gravity = gravity;
        m.life = lifeMin + (lifeMax - lifeMin) * (1.f - mix);
        m.size = size;
        m.frame = i % std::max(1, info.frames);
        m.tint = tint;
        motes_.push_back(m);
    }
}

void BattleView::startShake(float amplitude, float duration) {
    // 系统设置关了战斗震屏：全工程只在这一处拦（docs/settings.md 第 1 节 #6），震幅留在 0，
    // 画面上所有吃 shakeOffset 的东西都不动。
    if (!shakeEnabled_) return;
    // 正在震着的更大一下不被小的一下冲掉。
    const float current = shakeAmp_ * shakeEnvelope(shakeClock_, shakeDuration_);
    if (amplitude < current) return;
    shakeAmp_ = amplitude;
    shakeDuration_ = duration;
    shakeClock_ = 0.f;
}

void BattleView::say(int unit, const std::string& text, Color color) {
    if (!validUnit(unit)) return;
    floaters_.push_back(Floater{unit, text, color, 0.f, false, false, false, 0.f, 0.f});
}

void BattleView::endAction() {
    for (Actor& a : actors_) {
        a.lungeOut = false;
        a.casting = false;
        a.aura = 0;
    }
}

void BattleView::onAct(const BattleEvent& e) {
    // 上一手的人先收势（正常情况下队列播空时早就收了）；这一手的气焰是 BoostSpent 刚点上的，留着。
    for (std::size_t i = 0; i < actors_.size(); ++i) {
        if (static_cast<int>(i) == e.actor) continue;
        actors_[i].lungeOut = false;
        actors_[i].casting = false;
        actors_[i].aura = 0;
    }
    if (!validUnit(e.actor)) return;
    Actor& a = actors_[static_cast<std::size_t>(e.actor)];
    switch (static_cast<ActionKind>(e.value)) {
        case ActionKind::Attack:
            a.lungeOut = true;
            break;
        case ActionKind::Cast: {
            a.casting = true;
            const StrikeLook look = strikeLook(e.category);
            addEffect(FxSheetId::Glow, bodyCenter(e.actor), 2.6f, look.tint, false, 0.f, 0.45f);
            eng_.playSfx(castSfx(e.category));
            break;
        }
        case ActionKind::Item:
            a.casting = true;
            // 带 castMagic 的符箓：复用那门法术的施法光与声（契约 docs/interfaces-p3-ch07.md 2.5）。
            // 丹药、毒药那一手的 Act 不带类别，照旧只抬手。
            if (e.category != 0) {
                const StrikeLook look = strikeLook(e.category);
                addEffect(FxSheetId::Glow, bodyCenter(e.actor), 2.6f, look.tint, false, 0.f, 0.45f);
                eng_.playSfx(castSfx(e.category));
            }
            break;
        case ActionKind::Charge:
            a.chargeFlare = 0.6f;
            break;
        case ActionKind::Defend:
        case ActionKind::Escape:
            break;
    }
}

void BattleView::onBoost(int unit, int level) {
    if (!validUnit(unit)) return;
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    a.aura = std::clamp(level, 0, 3);
    a.auraFlare = 0.35f;
    burst(FxSheetId::Spark, bodyCenter(unit), 5 + 3 * a.aura, 90.f, 220.f, -160.f, 0.35f, 0.6f, 18.f, kGoldGlow);
    eng_.playSfx("boost_" + std::to_string(std::max(1, a.aura)));
}

void BattleView::onHit(const BattleEvent& e, bool alreadyBroken, int previousToughness) {
    if (!validUnit(e.target)) return;
    const auto t = static_cast<std::size_t>(e.target);
    Actor& target = actors_[t];
    const std::vector<Unit>& units = battle_.units();
    const bool fromEnemy = validUnit(e.actor) && !units[static_cast<std::size_t>(e.actor)].ally;
    const StrikeLook look = strikeLook(e.category);
    // 连击每一击错开一点位置与角度：分得出是几刀。
    const float n = static_cast<float>(std::max(0, e.hit - 1));
    StagePoint at = bodyCenter(e.target);
    at.x += std::sin(n * 2.1f) * 16.f;
    at.y += std::cos(n * 1.7f) * 12.f;
    const FxSheet& sheet = fxSheet(look.sheet);
    const float life = static_cast<float>(sheet.frames) / std::max(1.f, sheet.fps) + 0.06f;
    const float size = look.scale * (target.scale / 4.f);
    addEffect(look.sheet, at, size, look.tint, fromEnemy, std::sin(n * 3.3f) * 22.f, life);
    const bool elemental = (e.category & core::kElementCategories) != 0;
    if (elemental) addEffect(FxSheetId::Glow, at, 3.6f, look.tint, false, 0.f, 0.3f);
    if ((e.category & core::kCategoryFire) != 0) flares_.push_back(Flare{at, 360.f, kFireLight, 0.f, 0.35f});
    burst(FxSheetId::Spark, at, e.weakness ? 9 : 6, 160.f, 380.f, 700.f, 0.22f, 0.45f, 16.f,
          e.weakness ? kNumberWeak : kWhite);

    target.hurt = kHurt;
    target.flash = kHitFlash;
    target.knock = kKnock;
    if (e.value > 0) {
        Floater f;
        f.unit = e.target;
        f.text = std::to_string(e.value);
        f.color = alreadyBroken ? kNumberBroken : e.weakness ? kNumberWeak : kNumber;
        f.number = true;
        f.weak = e.weakness;
        f.broken = alreadyBroken;
        f.dx = n * 24.f;
        f.dy = -n * 14.f;
        floaters_.push_back(std::move(f));
    }
    if (e.revealed != 0 || e.weakness) {
        target.hud.reveal = 0.55f;
        target.hud.revealBits = e.revealed | (e.weakness ? e.category & units[t].weaknesses : 0);
    }
    if (e.toughness < previousToughness) {
        target.hud.shieldHit = 0.3f;
        target.hud.shieldFrom = previousToughness;
    }
    eng_.playSfx(look.hitSfx);
    if (e.weakness) eng_.playSfx("hit_weak");
    startShake(e.weakness ? 5.f : 3.f, e.weakness ? 0.16f : 0.12f);
    // 我方挨了敌人的重击：这一下 ≥ 气血上限的四分之一，或这一下把人打倒。
    const Unit& victim = units[t];
    if (fromEnemy && victim.ally && (e.value * 4 >= victim.maxHp || e.hp == 0)) {
        eng_.rumble(kHeavyHitRumbleLow, kHeavyHitRumbleHigh, kHeavyHitRumbleMs);
    }
}

void BattleView::onBreak(int unit) {
    if (!validUnit(unit)) return;
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    const StagePoint at = bodyCenter(unit);
    // 碎玉：十八片崩开，带重力落下；再一把金色火花、一团白光、一圈冲击波。
    burst(FxSheetId::Shards, at, 18, 260.f, 640.f, 1100.f, 0.75f, 1.15f, 36.f, kShardTint);
    burst(FxSheetId::Spark, at, 12, 220.f, 520.f, 500.f, 0.3f, 0.6f, 22.f, kGoldGlow);
    // 白光别开太大：七倍时把挨打的人整个吞进去，破势那一刻反而看不见是谁碎了。
    addEffect(FxSheetId::Glow, at, 4.2f, Color{255, 246, 226, 170}, false, 0.f, 0.35f);
    flares_.push_back(Flare{at, 520.f, kWhite, 0.f, 0.5f});
    ringAt_ = at;
    ringAge_ = 0.f;
    breakUnit_ = unit;
    breakAge_ = 0.f;
    slowmo_ = kSlowmo;
    whiteFlash_ = kWhiteFlash;
    startShake(13.f, 0.5f);
    a.hurt = kHurt * 2.f;
    a.flash = kHitFlash;
    a.hud.shieldHit = 0.5f;
    eng_.playSfx("break");
    eng_.rumble(kBreakRumbleLow, kBreakRumbleHigh, kBreakRumbleMs);
}

void BattleView::onRecover(int unit) {
    if (!validUnit(unit)) return;
    actors_[static_cast<std::size_t>(unit)].hud.shieldRefill = 0.6f;
    const StagePoint feet = anchors_[static_cast<std::size_t>(unit)];
    addEffect(FxSheetId::Glow, StagePoint{feet.x, feet.y - 20.f}, 2.8f, kGoldGlow, false, 0.f, 0.45f);
}

void BattleView::onHeal(int unit, int hp, int mp) {
    if (!validUnit(unit)) return;
    addEffect(FxSheetId::Heal, bodyCenter(unit), 3.f, kHealTint, false, 0.f, 0.5f);
    burst(FxSheetId::Spark, bodyCenter(unit), 6, 40.f, 110.f, -220.f, 0.5f, 0.8f, 16.f, kHealTint);
    if (hp > 0) {
        floaters_.push_back(Floater{unit, "+" + std::to_string(hp), kNumberHeal, 0.f, true, false, false, 0.f, 0.f});
    }
    if (mp > 0) say(unit, "+" + std::to_string(mp), Color{150, 190, 250, 255});
    eng_.playSfx("heal");
}

void BattleView::onPoisonTick(int unit, int amount) {
    if (!validUnit(unit)) return;
    addEffect(FxSheetId::Poison, bodyCenter(unit), 2.4f, kPoisonTint, false, 0.f, 0.5f);
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    a.hurt = kHurt * 0.6f;
    floaters_.push_back(
        Floater{unit, "-" + std::to_string(amount), kNumberPoison, 0.f, true, false, false, 0.f, 0.f});
}

void BattleView::onPoisoned(int unit) {
    if (!validUnit(unit)) return;
    addEffect(FxSheetId::Poison, bodyCenter(unit), 3.f, kPoisonTint, false, 0.f, 0.5f);
    eng_.playSfx("poison");
}

void BattleView::onFall(int unit) {
    if (!validUnit(unit)) return;
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    a.lungeOut = false;
    a.casting = false;
    if (a.ally) {
        // 我方倒下是跪倒（战斗帧「倒地」），人还在台上：原作也不让同伴当场化掉。
        eng_.playSfx("ally_down");
        return;
    }
    a.dissolve = 0.f;
    burst(FxSheetId::Spark, bodyCenter(unit), 16, 30.f, 150.f, -260.f, 0.6f, 1.0f, 18.f, Color{255, 236, 220, 255});
    eng_.playSfx("enemy_down");
}

void BattleView::onFled(int unit) {
    if (!validUnit(unit)) return;
    actors_[static_cast<std::size_t>(unit)].flee = 0.f;
    eng_.playSfx("miss");
}

void BattleView::onWaveIn(int wave) {
    const std::vector<Unit>& units = battle_.units();
    for (std::size_t i = 0; i < units.size() && i < actors_.size(); ++i) {
        if (!units[i].ally && units[i].wave == wave) actors_[i].enter = 0.f;
    }
    eng_.playSfx("encounter");
}

void BattleView::onChargeDeclare(int unit) {
    if (!validUnit(unit)) return;
    actors_[static_cast<std::size_t>(unit)].chargeFlare = 0.9f;
    addEffect(FxSheetId::Glow, bodyCenter(unit), 5.f, kCinnabarGlow, false, 0.f, 0.6f);
    flares_.push_back(Flare{bodyCenter(unit), 420.f, kCinnabarGlow, 0.f, 0.8f});
    startShake(4.f, 0.3f);
    eng_.playSfx("boost_3");
}

void BattleView::onReveal(int unit, int bits) {
    if (!validUnit(unit)) return;
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    // 复用现有的金光（Glow）与火花，不新画资源（契约 docs/interfaces-p3-ch06.md 1.5）。
    addEffect(FxSheetId::Glow, bodyCenter(unit), 3.4f, kGoldGlow, false, 0.f, 0.45f);
    burst(FxSheetId::Spark, bodyCenter(unit), 6, 60.f, 160.f, -120.f, 0.3f, 0.55f, 16.f, kGoldGlow);
    if (bits != 0) {
        a.hud.reveal = 0.55f;
        a.hud.revealBits = bits;
    }
}

void BattleView::onStagger(int unit, int previousToughness) {
    if (!validUnit(unit)) return;
    Actor& a = actors_[static_cast<std::size_t>(unit)];
    // 复用 onHit 里「架势减少」那一段（盾跳一下、旧数字往下掉）与现成的金光火花，不新画资源
    //（契约 docs/interfaces-p3-ch07.md 2.5）。不出伤害数字：这一下没伤人。
    burst(FxSheetId::Spark, bodyCenter(unit), 8, 80.f, 220.f, 300.f, 0.25f, 0.5f, 16.f, kGoldGlow);
    a.hud.shieldHit = 0.3f;
    a.hud.shieldFrom = previousToughness;
    eng_.playSfx("hit_weak");
}

// ---------------------------------------------------------------------------
// 位置与外形
// ---------------------------------------------------------------------------

float BattleView::sidestep(int unit) const {
    // 先让后冲：往下让的那一截在前半程走完，冲到同伴身边时人已经在他前面（靠镜头那一侧）。
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    return a.reach.y * smoothStep(0.f, 0.5f, a.lunge);
}

StagePoint BattleView::unitOffset(int unit, const Frame& frame) const {
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    const float toward = a.ally ? -1.f : 1.f;   // 朝对面的方向
    StagePoint off{};
    const float lunge = smoothStep(0.f, 1.f, a.lunge);
    off.x += a.reach.x * lunge;
    off.y += sidestep(unit);
    off.y -= std::sin(kPi * std::clamp(a.lunge, 0.f, 1.f)) * 10.f;
    if (unit == frame.actor) off.x += toward * (frame.choosing ? 12.f : 8.f);
    off.x -= toward * kKnockDistance * (a.knock / kKnock);
    if (a.enter >= 0.f) off.x -= toward * 160.f * (1.f - easeOutCubic(a.enter / kEnter));
    if (a.flee >= 0.f) off.x -= toward * 420.f * std::min(1.f, a.flee / kFlee) * std::min(1.f, a.flee / kFlee);
    if (a.look.floating) off.y += -18.f + std::sin(clock_ * 2.1f + a.phase * 3.f) * 6.f;
    return off;
}

RectF BattleView::bodyRect(int unit, StagePoint at) const {
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    if (a.tex == engine::kInvalidTexture) {
        const float size = a.scale * 18.f;
        return RectF{at.x - size / 2.f, at.y - size, size, size};
    }
    const float w = static_cast<float>(a.look.frameW) * a.scale;
    const float h = static_cast<float>(a.look.frameH) * a.scale;
    return RectF{at.x - w / 2.f, at.y - static_cast<float>(a.look.footY + 1) * a.scale, w, h};
}

StagePoint BattleView::bodyCenter(int unit) const {
    const StagePoint feet = anchors_[static_cast<std::size_t>(unit)];
    const StagePoint off = unitOffset(unit, Frame{});
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    const float top = a.tex == engine::kInvalidTexture
                          ? a.scale * 18.f
                          : static_cast<float>(a.look.footY + 1 - a.look.headY) * a.scale;
    return StagePoint{feet.x + off.x, feet.y + off.y - top * 0.5f};
}

float BattleView::headY(int unit) const {
    const StagePoint feet = anchors_[static_cast<std::size_t>(unit)];
    const StagePoint off = unitOffset(unit, Frame{});
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    const float top = a.tex == engine::kInvalidTexture
                          ? a.scale * 18.f
                          : static_cast<float>(a.look.footY + 1 - a.look.headY) * a.scale;
    return feet.y + off.y - top;
}

std::string BattleView::poseOf(int unit, const Frame& frame) const {
    const auto i = static_cast<std::size_t>(unit);
    const Actor& a = actors_[i];
    const Unit& u = battle_.units()[i];
    const ShownUnit& s = (*frame.shown)[i];
    if (a.dissolve >= 0.f || a.flee >= 0.f) return "hurt";
    if (a.ally && s.hp <= 0) return "down";
    if (a.hurt > 0.f) return "hurt";
    if (a.lunge > 0.05f) return "attack";
    if (a.casting) return "cast";
    if (frame.celebrate && a.ally) return "victory";
    if (s.broken) return "hurt";
    if (u.guarding) return "guard";
    if (u.charging || (unit == frame.actor && frame.choosing && frame.boost > 0)) return "ready";
    return "idle";
}

// ---------------------------------------------------------------------------
// 画：背景
// ---------------------------------------------------------------------------

void BattleView::drawLayer(engine::Engine& e, engine::TextureId tex, float x, float y, bool wrap) const {
    if (tex == engine::kInvalidTexture) return;
    const engine::Point size = e.textureSize(tex);
    const auto sw = static_cast<float>(size.x);
    const auto sh = static_cast<float>(size.y);
    const DrawOptions plain{};
    e.drawTexture(tex, RectF{}, RectF{x, y, kW, kH}, plain);
    // 水平：远景三层首尾相接（美术约定），平移出来的缝用另一份补上；地面有透视不能接，把边上那一列拉宽补缝。
    if (wrap) {
        if (x > 0.f) e.drawTexture(tex, RectF{}, RectF{x - kW, y, kW, kH}, plain);
        if (x < 0.f) e.drawTexture(tex, RectF{}, RectF{x + kW, y, kW, kH}, plain);
    } else {
        if (x > 0.f) e.drawTexture(tex, RectF{0.f, 0.f, 1.f, sh}, RectF{0.f, y, x + 1.f, kH}, plain);
        if (x < 0.f) e.drawTexture(tex, RectF{sw - 1.f, 0.f, 1.f, sh}, RectF{x + kW - 1.f, y, 1.f - x, kH}, plain);
    }
    // 竖直：震屏最多十几像素，拉宽顶上 / 底下那一行补上。
    if (y > 0.f) e.drawTexture(tex, RectF{0.f, 0.f, sw, 1.f}, RectF{x, 0.f, kW, y + 1.f}, plain);
    if (y < 0.f) e.drawTexture(tex, RectF{0.f, sh - 1.f, sw, 1.f}, RectF{x, y + kH - 1.f, kW, 1.f - y}, plain);
}

void BattleView::drawBackdrop(engine::Engine& e, StagePoint shake) const {
    if (layers_[0] == engine::kInvalidTexture) {
        // 缺图：两色底 + 地平线，好让人站在「地上」。
        e.drawGradient(RectF{0.f, 0.f, kW, 384.f}, Color{30, 34, 52, 255}, Color{58, 60, 84, 255}, BlendMode::None);
        e.drawGradient(RectF{0.f, 384.f, kW, kH - 384.f}, Color{52, 58, 48, 255}, Color{34, 38, 32, 255},
                       BlendMode::None);
        return;
    }
    // 镜头以十来秒一个来回轻轻摆：远近几层挪得不一样多，这就是视差。天上的云另外自己慢慢漂。
    const float sway = std::sin(clock_ * 0.21f) * 12.f;
    const float cloud = -std::fmod(clock_ * 5.f, kW);
    drawLayer(e, layers_[0], cloud + sway * 0.15f + shake.x, shake.y, true);
    drawLayer(e, layers_[1], std::fmod(sway * 0.35f, kW) + shake.x, shake.y, true);
    if (look_.fog) drawFog(e, shake);
    drawLayer(e, layers_[2], std::fmod(sway * 0.6f, kW) + shake.x, shake.y, true);
    drawLayer(e, layers_[3], shake.x, shake.y, false);
}

void BattleView::drawFog(engine::Engine& e, StagePoint shake) const {
    const engine::TextureId fog = fxTexture(FxSheetId::Fog);
    if (fog == engine::kInvalidTexture) return;
    // 雾纹四边无缝，按 256 一块铺一条带子，压在远景上慢慢往左漂。
    constexpr float kTile = 256.f;
    Color tint{255, 255, 255, 34};
    switch (look_.time) {
        case core::TimeOfDay::Dusk: tint = Color{255, 214, 196, 38}; break;
        case core::TimeOfDay::Night: tint = Color{176, 190, 255, 40}; break;
        case core::TimeOfDay::Indoor: tint = Color{210, 196, 176, 30}; break;
        case core::TimeOfDay::Day: break;
    }
    if (look_.soulsea) tint = Color{150, 176, 255, 56};
    const float drift = -std::fmod(clock_ * 9.f, kTile);
    DrawOptions opt{};
    opt.tint = tint;
    for (float x = drift - kTile; x < kW; x += kTile) {
        for (float y = 220.f; y < 440.f; y += kTile * 0.5f) {
            e.drawTexture(fog, RectF{}, RectF{x + shake.x, y + shake.y, kTile, kTile * 0.5f}, opt);
        }
    }
}

// ---------------------------------------------------------------------------
// 画：人
// ---------------------------------------------------------------------------

void BattleView::drawAuras(engine::Engine& e, int unit, const Frame& frame, StagePoint shake, bool emissive) const {
    const auto i = static_cast<std::size_t>(unit);
    const Actor& a = actors_[i];
    const Unit& u = battle_.units()[i];
    const StagePoint off = unitOffset(unit, frame);
    const StagePoint feet{anchors_[i].x + off.x + shake.x, anchors_[i].y + off.y + shake.y};
    // 蓄劲的气焰：菜单里正在选的那几点，或这一手已经蓄下的那几点。
    const int level = (frame.choosing && unit == frame.actor && a.ally) ? frame.boost : a.aura;
    const engine::TextureId boost = fxTexture(FxSheetId::BoostAura);
    if (level > 0 && boost != engine::kInvalidTexture) {
        const FxSheet& sheet = fxSheet(FxSheetId::BoostAura);
        const float s = a.scale;
        DrawOptions opt{};
        opt.blend = BlendMode::Add;
        const float strength = 0.6f + 0.13f * static_cast<float>(level) + a.auraFlare;
        opt.tint = alphaOf(kWhite, emissive ? strength * 0.7f : strength);
        e.drawTexture(boost, fxFrameRect(sheet, level - 1, clock_ + a.phase, true),
                      RectF{feet.x - 16.f * s, feet.y - 39.f * s, 32.f * s, 40.f * s}, opt);
    }
    // 首领宣告蓄势之后：暗红气焰，大体型按体型放大。
    const engine::TextureId boss = fxTexture(FxSheetId::BossAura);
    if ((u.charging || a.chargeFlare > 0.f) && boss != engine::kInvalidTexture && (*frame.shown)[i].visible) {
        const FxSheet& sheet = fxSheet(FxSheetId::BossAura);
        const float s = a.scale * std::max(1.f, static_cast<float>(a.look.frameH) / 32.f);
        DrawOptions opt{};
        opt.blend = emissive ? BlendMode::Add : BlendMode::Alpha;
        opt.tint = alphaOf(emissive ? kCinnabarGlow : kWhite, emissive ? 0.5f : 0.9f);
        e.drawTexture(boss, fxFrameRect(sheet, 0, clock_ + a.phase, true),
                      RectF{feet.x - 16.f * s, feet.y - 39.f * s, 32.f * s, 40.f * s}, opt);
    }
}

void BattleView::drawUnit(engine::Engine& e, int unit, const Frame& frame, StagePoint shake, bool flashOnly) const {
    const auto i = static_cast<std::size_t>(unit);
    const Actor& a = actors_[i];
    const Unit& u = battle_.units()[i];
    const ShownUnit& s = (*frame.shown)[i];
    const bool dissolving = a.dissolve >= 0.f && a.dissolve < kDissolve;
    const bool fleeing = a.flee >= 0.f && a.flee < kFlee;
    const bool downAlly = a.ally && s.hp <= 0 && !u.fled && u.onField;
    if (!s.visible && !dissolving && !fleeing && !downAlly) return;

    float alpha = 1.f;
    if (a.enter >= 0.f) alpha = smoothStep(0.f, kEnter * 0.6f, a.enter);
    if (fleeing) alpha = 1.f - a.flee / kFlee;
    if (dissolving) alpha = 1.f - a.dissolve / kDissolve;
    const StagePoint off = unitOffset(unit, frame);
    const StagePoint feet{anchors_[i].x + off.x + shake.x, anchors_[i].y + off.y + shake.y};

    if (flashOnly) {
        // 受击闪白与溶解时的白：同一个剪影以加色再画一遍，压在后处理之上，夜里也是白的。
        const float white = std::max(a.flash / kHitFlash, dissolving ? 1.f - a.dissolve / kDissolve : 0.f);
        if (white <= 0.f || a.tex == engine::kInvalidTexture) return;
        DrawOptions opt{};
        opt.blend = BlendMode::Add;
        opt.flipX = a.flip;
        opt.tint = alphaOf(kWhite, std::min(1.f, white) * alpha);
        const int f = battleFrame(a.look, poseOf(unit, frame), clock_ + a.phase);
        e.drawTexture(a.tex, lookFrameRect(a.look, f), bodyRect(unit, feet), opt);
        return;
    }

    // 脚下柔影：跟着人走，但不跟着跳（影子贴在地上）；悬空的光球影子小而淡。
    if (const engine::TextureId shadow = fxTexture(FxSheetId::Shadow); shadow != engine::kInvalidTexture) {
        const float bodyW = a.tex == engine::kInvalidTexture ? a.scale * 18.f
                                                             : static_cast<float>(a.look.frameW) * a.scale;
        const float w = std::clamp(bodyW * (a.look.floating ? 0.5f : 0.8f), 44.f, 190.f);
        const float h = w * 0.28f;
        DrawOptions opt{};
        opt.tint = Color{0, 0, 0, static_cast<std::uint8_t>((a.look.floating ? 90.f : 140.f) * alpha)};
        const float groundY = anchors_[i].y + sidestep(unit) + shake.y;
        e.drawTexture(shadow, RectF{}, RectF{feet.x - w / 2.f, groundY - h / 2.f, w, h}, opt);
    }

    Color tint = kWhite;
    if (s.broken) tint = kBrokenTint;
    if (downAlly) tint = kDownTint;
    if (a.tex == engine::kInvalidTexture) {
        // 没有图：旧的图元画法（施工图 1.2「缺图不崩」）。
        const RectF body = bodyRect(unit, feet);
        const FigureStyle style = a.ally ? FigureStyle{Color{110, 170, 220, 255}, Color{234, 208, 180, 255},
                                                       Color{188, 224, 255, 255}}
                                         : FigureStyle{Color{206, 106, 96, 255}, Color{206, 176, 148, 255},
                                                       Color{250, 186, 160, 255}};
        if (alpha > 0.4f) {
            drawFigure(e,
                       engine::Rect{static_cast<int>(body.x), static_cast<int>(body.y), static_cast<int>(body.w),
                                    static_cast<int>(body.h)},
                       style, a.ally ? 3 : 1);
        }
        return;
    }
    DrawOptions opt{};
    opt.flipX = a.flip;
    opt.tint = alphaOf(tint, alpha);
    const int f = battleFrame(a.look, poseOf(unit, frame), clock_ + a.phase);
    e.drawTexture(a.tex, lookFrameRect(a.look, f), bodyRect(unit, feet), opt);
}

// ---------------------------------------------------------------------------
// 画：特效
// ---------------------------------------------------------------------------

void BattleView::drawEffects(engine::Engine& e, StagePoint shake, bool emissive) const {
    for (const Effect& fx : effects_) {
        const FxSheet& sheet = fxSheet(fx.sheet);
        const engine::TextureId tex = fxTexture(fx.sheet);
        if (tex == engine::kInvalidTexture) continue;
        if (emissive && !sheet.additive && fx.sheet != FxSheetId::FireBurst) continue;
        const float fade = 1.f - smoothStep(fx.life * 0.6f, fx.life, fx.age);
        // 光斑一冒就开到最大再慢慢收；帧动画按帧播完就停在最后一帧。
        float scale = fx.scale;
        if (fx.sheet == FxSheetId::Glow) scale *= 0.7f + 0.3f * easeOutCubic(fx.age / std::max(0.01f, fx.life * 0.4f));
        const float w = static_cast<float>(sheet.frameW) * scale;
        const float h = static_cast<float>(sheet.frameH) * scale;
        DrawOptions opt{};
        opt.blend = (sheet.additive || emissive) ? BlendMode::Add : BlendMode::Alpha;
        opt.flipX = fx.flip;
        opt.angle = fx.angle;
        opt.tint = alphaOf(fx.tint, fade * (emissive ? 0.8f : 1.f));
        const bool loop = fx.sheet == FxSheetId::Heal || fx.sheet == FxSheetId::Poison;
        e.drawTexture(tex, fxFrameRect(sheet, 0, fx.age, loop),
                      RectF{fx.at.x + shake.x - w / 2.f, fx.at.y + shake.y - h / 2.f, w, h}, opt);
    }
}

void BattleView::drawMotes(engine::Engine& e, StagePoint shake) const {
    for (const Mote& m : motes_) {
        const FxSheet& sheet = fxSheet(m.sheet);
        const engine::TextureId tex = fxTexture(m.sheet);
        if (tex == engine::kInvalidTexture) continue;
        const float fade = 1.f - smoothStep(m.life * 0.55f, m.life, m.age);
        DrawOptions opt{};
        opt.blend = sheet.additive ? BlendMode::Add : BlendMode::Alpha;
        opt.angle = m.angle;
        opt.tint = alphaOf(m.tint, fade);
        const RectF src{static_cast<float>(m.frame * sheet.frameW), 0.f, static_cast<float>(sheet.frameW),
                        static_cast<float>(sheet.frameH)};
        e.drawTexture(tex, src, RectF{m.at.x + shake.x - m.size / 2.f, m.at.y + shake.y - m.size / 2.f, m.size, m.size},
                      opt);
    }
}

void BattleView::drawActiveRing(engine::Engine& e, const Frame& frame, StagePoint shake) {
    mesh_.clear();
    if (validUnit(frame.actor) && (*frame.shown)[static_cast<std::size_t>(frame.actor)].visible) {
        const auto i = static_cast<std::size_t>(frame.actor);
        const Actor& a = actors_[i];
        const StagePoint off = unitOffset(frame.actor, frame);
        const float bodyW = a.tex == engine::kInvalidTexture ? a.scale * 18.f
                                                             : static_cast<float>(a.look.frameW) * a.scale;
        const float rx = std::clamp(bodyW * 0.42f, 34.f, 92.f);
        const float pulse = 0.72f + 0.28f * std::sin(realClock_ * 5.f);
        const float groundY = anchors_[i].y + sidestep(frame.actor) + shake.y;
        appendRing(mesh_, anchors_[i].x + off.x + shake.x, groundY, rx, rx * 0.3f, 7.f,
                   alphaOf(Color{242, 217, 139, 255}, pulse));
        appendRing(mesh_, anchors_[i].x + off.x + shake.x, groundY, rx + 8.f, (rx + 8.f) * 0.3f,
                   16.f, alphaOf(Color{242, 190, 90, 255}, 0.35f * pulse));
    }
    if (ringAge_ >= 0.f) {
        // 破势的冲击波：一圈白金往外推，越推越淡。
        const float t = ringAge_ / kBreakRing;
        const float r = 40.f + 280.f * easeOutCubic(t);
        appendRing(mesh_, ringAt_.x + shake.x, ringAt_.y + shake.y, r, r * 0.55f, 26.f * (1.f - t) + 6.f,
                   alphaOf(Color{255, 240, 200, 255}, 1.f - t));
    }
    if (!mesh_.empty()) e.drawGeometry(engine::kInvalidTexture, mesh_, {}, BlendMode::Add);
}

engine::PostFxSettings BattleView::postSettings() const {
    // 时辰的缺省（环境光、景深、辉光、暗角、调色）与世界画面同一张表（MapVisual）；焦点对在两阵的人身上：
    // 清晰带 y≈0.32–0.94，天与远景糊掉一截，移轴感就出来了。
    engine::PostFxSettings s = postFxFor(visualDefaults(look_.time), 0.63f);
    s.dofFocus = 0.63f;
    s.dofBand = 0.31f;
    s.dofRamp = 0.2f;
    if (look_.soulsea) {
        s.ambient = Color{150, 160, 226, 255};
        s.dof = 0.3f;
        s.bloom = 0.75f;
        s.vignette = 0.6f;
        s.gradeMul = Color{226, 232, 255, 255};
        s.gradeAdd = Color{6, 4, 18, 255};
    }
    return s;
}

void BattleView::renderStage(Application& app, const Frame& frame) {
    engine::Engine& e = app.engine();
    const StagePoint shake = shakeOffset(shakeClock_, shakeDuration_, shakeAmp_);
    // 由远到近画：脚底 y 小的先画。
    order_.resize(actors_.size());
    for (std::size_t i = 0; i < order_.size(); ++i) order_[i] = static_cast<int>(i);
    std::sort(order_.begin(), order_.end(), [this](int a, int b) {
        const StagePoint& pa = anchors_[static_cast<std::size_t>(a)];
        const StagePoint& pb = anchors_[static_cast<std::size_t>(b)];
        return pa.y != pb.y ? pa.y < pb.y : pa.x < pb.x;
    });
    const bool glowing = look_.particle == engine::ParticleKind::Firefly ||
                         look_.particle == engine::ParticleKind::Ember || look_.particle == engine::ParticleKind::Dust;

    post_->beginScene();
    drawBackdrop(e, shake);
    air_.render(e, 0.f, 0.f);
    if (look_.soulsea) motes2_.render(e, 0.f, 0.f);
    for (const int unit : order_) {
        drawAuras(e, unit, frame, shake, false);
        drawUnit(e, unit, frame, shake, false);
    }
    if (post_->beginEmissive()) {
        if (glowing) air_.render(e, 0.f, 0.f);
        if (look_.soulsea) motes2_.render(e, 0.f, 0.f);
        for (const int unit : order_) drawAuras(e, unit, frame, shake, true);
        drawEffects(e, shake, true);
        post_->endEmissive();
    }
    if (look_.time != core::TimeOfDay::Day || look_.soulsea) {
        // 夜里、室内：两阵头顶各一盏大而软的灯，人才看得清；打出来的火光、破势的白光另外叠。
        post_->addLight({1000.f + shake.x, 470.f + shake.y, 440.f, Color{255, 228, 190, 255}, 1.3f});
        post_->addLight({380.f + shake.x, 480.f + shake.y, 480.f, Color{228, 228, 255, 255}, 1.1f});
    }
    for (const Flare& f : flares_) {
        const float k = 1.f - f.age / f.life;
        post_->addLight({f.at.x + shake.x, f.at.y + shake.y, f.radius, f.color, 1.f + 2.f * k});
    }
    post_->endScene(postSettings());

    // 压在后处理之上：光环、刀光、闪白、碎片。
    drawActiveRing(e, frame, shake);
    drawEffects(e, shake, false);
    for (const int unit : order_) drawUnit(e, unit, frame, shake, true);
    drawMotes(e, shake);
}

// ---------------------------------------------------------------------------
// 画：飘字、破势大字、白闪
// ---------------------------------------------------------------------------

void BattleView::drawBreakText(engine::Engine& e) const {
    if (breakAge_ < 0.f || !validUnit(breakUnit_)) return;
    const float t = breakAge_;
    const float pop = 1.f + 0.6f * (1.f - easeOutCubic(t / 0.12f));
    const float alpha = 1.f - smoothStep(kBreakText * 0.72f, kBreakText, t);
    const int size = static_cast<int>(std::lround(64.f * pop / 2.f)) * 2;
    const StagePoint feet = anchors_[static_cast<std::size_t>(breakUnit_)];
    const float cx = std::clamp(feet.x, 200.f, kW - 200.f);
    // 压在身子正中（原作的 BREAK 也是）：头顶留给伤害数字与「破绽」小标，两样不叠在一起。
    const float cy = std::clamp(bodyCenter(breakUnit_).y - static_cast<float>(size) / 2.f, 90.f, kH - 200.f);
    // 字后一道朱红的刀痕：两头淡掉的横条，比字宽出一截。
    const float streak = 420.f * easeOutCubic(t / 0.16f);
    const Color cinnabar = alphaOf(Color{184, 40, 26, 230}, alpha);
    const Color clear{184, 40, 26, 0};
    e.drawGradientH(RectF{cx - streak / 2.f, cy + static_cast<float>(size) * 0.42f, streak / 2.f, 12.f}, clear,
                    cinnabar, BlendMode::Alpha);
    e.drawGradientH(RectF{cx, cy + static_cast<float>(size) * 0.42f, streak / 2.f, 12.f}, cinnabar, clear,
                    BlendMode::Alpha);
    engine::TextStyle style{};
    style.shadow = true;
    style.shadowColor = alphaOf(Color{7, 9, 14, 200}, alpha);
    style.shadowOffset = 4;
    style.outline = true;
    style.outlineColor = alphaOf(Color{110, 18, 10, 255}, alpha);
    style.outlineWidth = 3;
    ui::drawSpacedText(e, breakWord_, cx, static_cast<int>(cy), size, alphaOf(Color{255, 236, 186, 255}, alpha),
                       style, size / 6);
}

void BattleView::renderOverlay(Application& app) {
    engine::Engine& e = app.engine();
    std::vector<int> stack(actors_.size(), 0);
    for (const Floater& f : floaters_) {
        if (!validUnit(f.unit)) continue;
        const StagePoint feet = anchors_[static_cast<std::size_t>(f.unit)];
        const float head = headY(f.unit);
        if (f.number) {
            const float pop = numberPop(f.age);
            const float grow = f.broken ? 1.3f : 1.f;
            const int size = static_cast<int>(std::lround(30.f * pop * grow / 2.f)) * 2;
            const float alpha = floaterAlpha(f.age, kNumberLife);
            const engine::Point measure = e.measureText(f.text, size);
            const float x = feet.x + f.dx - static_cast<float>(measure.x) / 2.f;
            const float y = std::max(70.f, head - 14.f + f.dy - numberLift(f.age) - static_cast<float>(size));
            engine::TextStyle style{};
            style.outline = true;
            style.outlineColor = alphaOf(Color{7, 9, 14, 235}, alpha);
            style.outlineWidth = f.broken ? 3 : 2;
            if (f.broken) {
                // 破势中挨的这一下：身后多一道暗红的残影，「翻倍」要看得出来。
                engine::TextStyle echo = style;
                echo.outline = false;
                e.drawText(f.text, static_cast<int>(x) + 4, static_cast<int>(y) + 4, size, alphaOf(kNumberEcho, alpha * 0.8f),
                           echo);
            }
            e.drawText(f.text, static_cast<int>(x), static_cast<int>(y), size, alphaOf(f.color, alpha), style);
            if (f.weak) {
                engine::TextStyle tag{};
                tag.outline = true;
                tag.outlineColor = alphaOf(Color{60, 30, 0, 230}, alpha);
                const engine::Point m = e.measureText(weakWord_, 16);
                e.drawText(weakWord_, static_cast<int>(feet.x + f.dx - static_cast<float>(m.x) / 2.f),
                           static_cast<int>(y) - 16, 16, alphaOf(kNumberWeak, alpha), tag);
            }
            continue;
        }
        const float alpha = floaterAlpha(f.age, kLabelLife);
        const int slot = stack[static_cast<std::size_t>(f.unit)]++;
        const engine::Point measure = e.measureText(f.text, 18);
        const float y = std::max(70.f, head - 40.f - f.age * 28.f - static_cast<float>(slot) * 22.f);
        engine::TextStyle style{};
        style.outline = true;
        style.outlineColor = alphaOf(Color{7, 9, 14, 230}, alpha);
        e.drawText(f.text, static_cast<int>(feet.x - static_cast<float>(measure.x) / 2.f), static_cast<int>(y), 18,
                   alphaOf(f.color, alpha), style);
    }
    drawBreakText(e);
    if (whiteFlash_ > 0.f) {
        e.fillRect(RectF{0.f, 0.f, kW, kH}, alphaOf(kWhite, 0.7f * whiteFlash_ / kWhiteFlash), BlendMode::Add);
    }
}

// ---------------------------------------------------------------------------
// 开战碎屏
// ---------------------------------------------------------------------------

void BattleView::captureIntro(Application& app) {
    static_cast<void>(app);
    introShot_ = post_->snapshot();
    intro_ = introShot_ == engine::kInvalidTexture ? Intro::Done : Intro::Running;
    if (!introHeld_) introClock_ = 0.f;
}

void BattleView::holdIntroAt(float t) {
    introHeld_ = true;
    introClock_ = t;
}

void BattleView::renderIntro(Application& app) {
    if (intro_ != Intro::Running) return;
    engine::Engine& e = app.engine();
    const float t = introClock_;
    // 战斗画面从黑里淡出来，世界画面碎成三角崩开，两件事同时发生。
    const float veil = 1.f - smoothStep(0.12f, 0.75f, t);
    if (veil > 0.f) e.fillRect(RectF{0.f, 0.f, kW, kH}, alphaOf(Color{7, 9, 14, 255}, veil), BlendMode::Alpha);
    mesh_.clear();
    for (const ShatterPiece& piece : pieces_) {
        const float alpha = shatterAlpha(piece, t);
        if (alpha <= 0.f) continue;
        const std::array<StagePoint, 3> pose = shatterPose(piece, t);
        const Color c = alphaOf(kWhite, alpha);
        for (std::size_t k = 0; k < 3; ++k) mesh_.push_back({pose[k].x, pose[k].y, c, piece.uv[k].x, piece.uv[k].y});
    }
    if (!mesh_.empty()) e.drawGeometry(introShot_, mesh_, {}, BlendMode::Alpha);
    // 开头那一下：裂纹一闪。
    if (t < 0.14f) {
        const Color crack = alphaOf(Color{255, 250, 235, 255}, 1.f - t / 0.14f);
        for (const ShatterPiece& piece : pieces_) {
            const std::array<StagePoint, 3> pose = shatterPose(piece, t);
            for (std::size_t k = 0; k < 3; ++k) {
                const StagePoint& p = pose[k];
                const StagePoint& q = pose[(k + 1) % 3];
                e.drawLine(p.x, p.y, q.x, q.y, crack);
            }
        }
        e.fillRect(RectF{0.f, 0.f, kW, kH}, alphaOf(kWhite, 0.35f * (1.f - t / 0.14f)), BlendMode::Add);
    }
}

// ---------------------------------------------------------------------------
// 小像
// ---------------------------------------------------------------------------

void BattleView::drawPortrait(engine::Engine& e, int unit, const RectF& dst, Color tint) const {
    if (!validUnit(unit)) return;
    const Actor& a = actors_[static_cast<std::size_t>(unit)];
    if (a.tex == engine::kInvalidTexture) {
        e.fillRect(RectF{dst.x + 4.f, dst.y + 4.f, dst.w - 8.f, dst.h - 8.f},
                   a.ally ? Color{91, 143, 214, tint.a} : Color{184, 65, 47, tint.a}, BlendMode::Alpha);
        return;
    }
    const RectF frame = lookFrameRect(a.look, battleFrame(a.look, "idle", 0.f));
    RectF src = frame;
    if (a.look.humanoid) {
        // 人：取头肩那一截（帧宽见方，从最高的不透明行往上留一行）。
        const float top = static_cast<float>(std::max(0, a.look.headY - 1));
        src = RectF{frame.x, frame.y + top, frame.w, std::min(frame.w, frame.h - top)};
    } else {
        const float side = std::min(frame.w, frame.h);
        src = RectF{frame.x + (frame.w - side) / 2.f, frame.y + (frame.h - side) / 2.f, side, side};
    }
    DrawOptions opt{};
    opt.flipX = a.flip;
    opt.tint = tint;
    e.drawTexture(a.tex, src, dst, opt);
}

}  // namespace fanren::game
