#include "game/TitleScene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <system_error>
#include <utility>

#include "game/Application.h"
#include "game/SettingsScene.h"

namespace fanren::game {
namespace {

using engine::BlendMode;
using engine::Color;
using engine::RectF;

constexpr float kW = static_cast<float>(engine::kLogicalWidth);
constexpr float kH = static_cast<float>(engine::kLogicalHeight);
constexpr float kCenterX = kW / 2.f;
constexpr float kTwoPi = 2.f * std::numbers::pi_v<float>;

// 美术层 320×180，×4 铺满 1280×720（施工图 1.1：战斗背景同一尺度）。
constexpr float kArtScale = 4.f;
constexpr std::array<const char*, 4> kArtFiles{"art/title/sky.png", "art/title/far.png",
                                              "art/title/mid.png", "art/title/near.png"};
// 各层的漂移速度（屏幕像素/秒）。天最慢、云海最快，near 那一角亭台不动（title.py 的约定）。
constexpr std::array<float, 4> kLayerSpeed{3.f, 6.f, 12.f, 0.f};

// 程序画法的配色：晨色天、远山、云海、近峰。这是场景美术，不是界面，所以不进 ui::Theme。
constexpr Color kSkyTop{18, 22, 50, 255};
constexpr Color kSkyMid{84, 64, 112, 255};
constexpr Color kSkyLow{222, 158, 128, 255};
constexpr Color kSunCore{255, 226, 170, 150};
constexpr Color kFarTop{120, 104, 150, 255};
constexpr Color kFarBottom{176, 132, 150, 255};
constexpr Color kNearRidgeTop{70, 62, 104, 255};
constexpr Color kNearRidgeBottom{126, 104, 138, 255};
constexpr Color kCloudTop{246, 232, 206, 255};
constexpr Color kCloudBottom{236, 214, 190, 255};
constexpr Color kPeakTop{28, 44, 50, 255};
constexpr Color kPeakBottom{58, 86, 84, 255};

constexpr float kHorizonY = 420.f;
constexpr float kCloudY = 468.f;

// 大字与菜单的位置。
constexpr int kTitleY = 150;
constexpr int kTitleSize = 84;
constexpr int kTitleSpacing = 20;
constexpr int kSubtitleY = 268;
constexpr int kMenuTop = 430;
constexpr int kMenuRow = 46;
constexpr int kMenuSize = 26;

// 标题画面的 BGM 与音效（docs/audio.md）。
constexpr const char* kBgm = "bgm_title";

// 山脊线。频率一律取 2π/1280 的整数倍：一个屏宽正好一个周期，漂移时首尾相接没有断口。
constexpr float kRidgeK = kTwoPi / kW;

float farRidge(float x) {
    return 330.f - 42.f * std::sin(x * kRidgeK + 0.8f) - 20.f * std::sin(x * kRidgeK * 3.f + 2.1f) -
           8.f * std::sin(x * kRidgeK * 9.f);
}

float nearRidge(float x) {
    return 382.f - 30.f * std::sin(x * kRidgeK * 2.f + 2.4f) -
           12.f * std::sin(x * kRidgeK * 5.f + 0.3f);
}

engine::Vertex vtx(float x, float y, const Color& c) {
    return engine::Vertex{x, y, c, 0.f, 0.f};
}

}  // namespace

TitleScene::TitleScene() : mist_(11), motes_(12) {}

TitleScene::~TitleScene() = default;

float TitleScene::layerOffset(float speed, double t, float period) {
    if (period <= 0.f) return 0.f;
    const double shifted = std::fmod(static_cast<double>(speed) * t, static_cast<double>(period));
    const double wrapped = shifted < 0.0 ? shifted + period : shifted;
    return static_cast<float>(wrapped);
}

std::vector<std::string> TitleScene::menuStrings(const core::GameData& data) {
    std::vector<std::string> out;
    for (const char* key : {"ui.title.new", "ui.title.continue", "ui.title.settings", "ui.title.quit",
                            "ui.title.no_save", "ui.title.load_failed", "ui.title.keys"}) {
        out.push_back(data.lookupText(key));
    }
    return out;
}

void TitleScene::onEnter(Application& app) {
    engine::Engine& e = app.engine();
    fx_ = std::make_unique<engine::PostFx>(e);
    time_ = 0.0;
    leaving_ = -1.0;
    error_.clear();

    artReady_ = true;
    for (std::size_t i = 0; i < kArtFiles.size(); ++i) {
        art_[i] = e.loadTexture(kArtFiles[i]);
        artReady_ = artReady_ && art_[i] != engine::kInvalidTexture;
    }

    // 薄雾压在云海那一带；光点散在天上，一明一灭。先空跑一阵：第一帧就是「已经飘了一会儿」。
    mist_.configure(engine::ParticleKind::Mist, 0.7f, {-200.f, 380.f, 1680.f, 220.f},
                    engine::ParticleSpace::Screen);
    mist_.prewarm(30.f);
    motes_.configure(engine::ParticleKind::Dust, 7.f, {160.f, 90.f, 960.f, 420.f},
                     engine::ParticleSpace::Screen);
    motes_.prewarm(20.f);

    // 继续旅程：存档在才可选。不在时照样列出来、置灰并说明为什么——少一行，玩家会以为
    // 这个游戏就没有读档。
    std::error_code ec;
    const bool hasSave = std::filesystem::exists(app.quickSavePath(), ec);
    std::vector<ui::ListItem> items(kItemCount);
    items[kNewJourney].label = app.text("ui.title.new");
    items[kContinue].label = app.text("ui.title.continue");
    items[kContinue].enabled = hasSave;
    items[kContinue].disabledReason = app.text("ui.title.no_save");
    items[kSettings].label = app.text("ui.title.settings");
    items[kQuit].label = app.text("ui.title.quit");
    menu_.reset();
    menu_.setPageSize(kItemCount);
    menu_.setItems(std::move(items));

    e.playBgm(kBgm);
}

bool TitleScene::update(Application& app, double deltaSeconds) {
    engine::Engine& e = app.engine();
    time_ += deltaSeconds;
    mist_.update(static_cast<float>(deltaSeconds));
    motes_.update(static_cast<float>(deltaSeconds));

    if (leaving_ >= 0.0) {
        leaving_ += deltaSeconds;
        if (leaving_ < kLeaveSeconds) return true;
        // 黑透了再换场景：换图、压世界层、压开篇卡都在这一帧末尾生效，玩家看到的是
        // 黑 → 章节卡，而不是标题画面一闪变成韩家村。
        auto started = app.startNewJourney();
        if (!started) {
            error_ = started.error;
            leaving_ = -1.0;
            e.playSfx("ui_error");
        }
        return true;
    }
    if (time_ < kFadeInSeconds * 0.5) return true;   // 淡入的前半截不收键，免得启动时的回车余势误选

    const int before = menu_.selection();
    const bool confirmed = menu_.update(e);
    if (menu_.selection() != before) e.playSfx("ui_cursor");
    if (!confirmed) return true;

    switch (menu_.selection()) {
        case kNewJourney:
            e.playSfx("ui_confirm");
            leaving_ = 0.0;
            break;
        case kContinue: {
            e.playSfx("ui_confirm");
            auto resumed = app.continueJourney(app.quickSavePath());
            if (!resumed) {
                error_ = app.text("ui.title.load_failed") + resumed.error;
                e.playSfx("ui_error");
            }
            break;
        }
        case kSettings:
            // 压在标题画面上：面板一收，标题画面接着转（docs/settings.md 5.1）。
            e.playSfx("ui_confirm");
            app.pushScene(std::make_unique<SettingsScene>());
            break;
        case kQuit:
        default:
            app.requestQuit();
            break;
    }
    return true;
}

void TitleScene::fan(float cx, float cy, float rx, float ry, const Color& center, const Color& rim) {
    constexpr int kSegments = 32;
    const int base = static_cast<int>(vertices_.size());
    vertices_.push_back(vtx(cx, cy, center));
    for (int i = 0; i < kSegments; ++i) {
        const float a = kTwoPi * static_cast<float>(i) / static_cast<float>(kSegments);
        vertices_.push_back(vtx(cx + rx * std::cos(a), cy + ry * std::sin(a), rim));
    }
    for (int i = 0; i < kSegments; ++i) {
        indices_.insert(indices_.end(), {base, base + 1 + i, base + 1 + (i + 1) % kSegments});
    }
}

void TitleScene::flush(engine::Engine& e, BlendMode blend) {
    if (!vertices_.empty()) e.drawGeometry(engine::kInvalidTexture, vertices_, indices_, blend);
    vertices_.clear();
    indices_.clear();
}

void TitleScene::drawArtLayers(engine::Engine& e) {
    const float period = kW;   // 320 × 4
    for (std::size_t i = 0; i < art_.size(); ++i) {
        const float offset = layerOffset(kLayerSpeed[i], time_, period);
        e.drawTexture(art_[i], RectF{}, RectF{-offset, 0.f, period, kH}, engine::DrawOptions{});
        if (kLayerSpeed[i] > 0.f) {
            e.drawTexture(art_[i], RectF{}, RectF{period - offset, 0.f, period, kH},
                          engine::DrawOptions{});
        }
    }
}

void TitleScene::drawProceduralLayers(engine::Engine& e) {
    // 天：深靛到紫、紫到晨光，两段竖向渐变；地平线上方一团日晕。
    e.drawGradient(RectF{0.f, 0.f, kW, 260.f}, kSkyTop, kSkyMid, BlendMode::Alpha);
    e.drawGradient(RectF{0.f, 260.f, kW, kHorizonY - 250.f}, kSkyMid, kSkyLow, BlendMode::Alpha);
    fan(944.f, 400.f, 300.f, 170.f, kSunCore, ui::withAlpha(kSunCore, 0));
    flush(e, BlendMode::Add);

    // 两重远山：顶点色从山脊到山脚渐变，按各自的速度往左漂，首尾相接。
    const auto ridge = [&](float (*height)(float), float speed, const Color& top,
                           const Color& bottom) {
        const float offset = layerOffset(speed, time_, kW);
        for (float x = 0.f; x < kW; x += 32.f) {
            const float sx = x - offset;
            for (const float dx : {0.f, kW}) {
                const float x0 = sx + dx;
                if (x0 > kW || x0 + 32.f < 0.f) continue;
                const int base = static_cast<int>(vertices_.size());
                vertices_.insert(vertices_.end(),
                                 {vtx(x0, height(x), top), vtx(x0 + 32.f, height(x + 32.f), top),
                                  vtx(x0 + 32.f, kCloudY + 20.f, bottom),
                                  vtx(x0, kCloudY + 20.f, bottom)});
                indices_.insert(indices_.end(),
                                {base, base + 1, base + 2, base, base + 2, base + 3});
            }
        }
        flush(e, BlendMode::Alpha);
    };
    ridge(farRidge, 6.f, kFarTop, kFarBottom);
    ridge(nearRidge, 10.f, kNearRidgeTop, kNearRidgeBottom);

    // 云海：一片浅金的底，上沿是一排缓慢漂移的圆团。
    e.drawGradient(RectF{0.f, kCloudY, kW, kH - kCloudY}, kCloudTop, kCloudBottom,
                   BlendMode::Alpha);
    const float drift = layerOffset(14.f, time_, 96.f);
    for (float x = -96.f; x < kW + 96.f; x += 48.f) {
        const float wobble = 10.f * std::sin((x + 17.f) * 0.07f);
        fan(x - drift + 48.f, kCloudY + 4.f + wobble, 44.f, 20.f, kCloudTop, kCloudTop);
    }
    flush(e, BlendMode::Alpha);

    // 近峰：三座从云里探出来的墨绿尖峰，不漂（近景漂得快反而晃眼）。
    for (const auto& [px, h, w] : {std::array<float, 3>{140.f, 190.f, 120.f},
                                   std::array<float, 3>{330.f, 120.f, 80.f},
                                   std::array<float, 3>{1140.f, 220.f, 150.f}}) {
        const int base = static_cast<int>(vertices_.size());
        vertices_.insert(vertices_.end(), {vtx(px, kCloudY + 40.f - h, kPeakTop),
                                           vtx(px + w / 2.f, kCloudY + 60.f, kPeakBottom),
                                           vtx(px - w / 2.f, kCloudY + 60.f, kPeakBottom)});
        indices_.insert(indices_.end(), {base, base + 1, base + 2});
    }
    flush(e, BlendMode::Alpha);
}

void TitleScene::drawEmissive(engine::Engine& e) {
    // 辉光通道：标题四个字背后一团暖金的光（「微光」）。经辉光管线化成光晕加回去。
    fan(kCenterX, static_cast<float>(kTitleY + kTitleSize / 2), 380.f, 80.f, {200, 150, 80, 150},
        {200, 150, 80, 0});
    // 程序画法的日晕也发一点光；美术层自己画了日晕，再叠一团就烧成一片白。
    if (!artReady_) fan(944.f, 400.f, 120.f, 70.f, {255, 220, 160, 200}, {255, 220, 160, 0});
    flush(e, BlendMode::Alpha);
    motes_.render(e, 0.f, 0.f);
}

void TitleScene::render(Application& app) {
    engine::Engine& e = app.engine();

    fx_->beginScene();
    if (artReady_) {
        drawArtLayers(e);
    } else {
        drawProceduralLayers(e);
    }
    mist_.render(e, 0.f, 0.f);
    motes_.render(e, 0.f, 0.f);
    if (fx_->beginEmissive()) {
        drawEmissive(e);
        fx_->endEmissive();
    }
    engine::PostFxSettings post;
    post.dof = 0.8f;
    post.dofFocus = 0.50f;
    post.dofBand = 0.36f;
    post.dofRamp = 0.18f;
    post.bloom = 0.55f;
    post.vignette = 0.55f;
    post.gradeMul = {255, 246, 236, 255};
    post.gradeAdd = {6, 3, 0, 255};
    fx_->endScene(post);

    drawMenu(app);

    // 开场从黑里淡出来；选了「新的旅程」再淡回黑。一层黑纱，字不逐帧改色。
    const ui::Theme& theme = app.theme();
    double veil = std::max(0.0, 1.0 - time_ / kFadeInSeconds);
    if (leaving_ >= 0.0) veil = std::max(veil, std::min(1.0, leaving_ / kLeaveSeconds));
    const auto alpha = static_cast<std::uint8_t>(std::lround(veil * 255.0));
    if (alpha > 0) {
        e.fillRect(RectF{0.f, 0.f, kW, kH}, ui::withAlpha(theme.inkDeep, alpha), BlendMode::Alpha);
    }
}

void TitleScene::drawMenu(Application& app) {
    engine::Engine& e = app.engine();
    const ui::Theme& theme = app.theme();

    // 菜单底下一团墨色的柔光：云海是全画面最亮的一块，菜单正好压在上面，没有它
    // 浅色的字（尤其置灰的那一项）就化在云里了。椭圆、边缘淡到零，不画成一块板子。
    fan(kCenterX, static_cast<float>(kMenuTop + kMenuRow + 40), 440.f, 190.f,
        ui::withAlpha(theme.inkDeep, 175), ui::withAlpha(theme.inkDeep, 0));
    flush(e, BlendMode::Alpha);

    ui::drawOrnament(e, kCenterX, static_cast<float>(kTitleY - 22), 150.f, theme);
    ui::drawSpacedText(e, app.text("ui.title.name"), kCenterX, kTitleY, kTitleSize, theme.paper,
                       theme.titleStyle, kTitleSpacing);
    ui::drawSpacedText(e, app.text("ui.title.subtitle"), kCenterX, kSubtitleY, 22, theme.paper,
                       theme.titleStyle, 8);
    ui::drawOrnament(e, kCenterX, static_cast<float>(kSubtitleY + 44), 220.f, theme);

    // 菜单：居中的四行。选中那一行两侧各一颗菱形、底下一道两头渐隐的金条。
    for (int i = 0; i < menu_.count(); ++i) {
        const ui::ListItem& item = menu_.items()[static_cast<std::size_t>(i)];
        const bool selected = i == menu_.selection();
        const int y = kMenuTop + i * kMenuRow;
        const int w = e.measureText(item.label, kMenuSize).x;
        if (selected) {
            const float half = static_cast<float>(w) / 2.f + 70.f;
            const RectF left{kCenterX - half, static_cast<float>(y - 6), half, 38.f};
            const RectF right{kCenterX, static_cast<float>(y - 6), half, 38.f};
            e.drawGradientH(left, ui::withAlpha(theme.gold, 0),
                            ui::withAlpha(theme.gold, theme.cursorAlpha), BlendMode::Alpha);
            e.drawGradientH(right, ui::withAlpha(theme.gold, theme.cursorAlpha),
                            ui::withAlpha(theme.gold, 0), BlendMode::Alpha);
            const float dy = static_cast<float>(y + kMenuSize / 2);
            ui::drawDiamond(e, kCenterX - static_cast<float>(w) / 2.f - 24.f, dy, 5.f,
                            theme.goldBright);
            ui::drawDiamond(e, kCenterX + static_cast<float>(w) / 2.f + 24.f, dy, 5.f,
                            theme.goldBright);
        }
        const Color color = !item.enabled ? theme.paperDim : selected ? theme.goldBright : theme.paper;
        e.drawText(item.label, static_cast<int>(kCenterX) - w / 2, y, kMenuSize, color,
                   theme.titleStyle);
        if (!item.enabled && !item.disabledReason.empty()) {
            e.drawText("（" + item.disabledReason + "）", static_cast<int>(kCenterX) + w / 2 + 12,
                       y + 6, 16, theme.paperDim, theme.bodyStyle);
        }
    }

    if (!error_.empty()) {
        const int w = e.measureText(error_, 18).x;
        e.drawText(error_, static_cast<int>(kCenterX) - w / 2, kMenuTop + menu_.count() * kMenuRow + 18, 18,
                   theme.cinnabar, theme.bodyStyle);
    }
    const std::string keys = app.text("ui.title.keys");
    const int keysW = e.measureText(keys, 16).x;
    e.drawText(keys, static_cast<int>(kCenterX) - keysW / 2, 676, 16, theme.paperDim,
               theme.bodyStyle);
}

}  // namespace fanren::game
