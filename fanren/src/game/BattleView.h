#pragma once
// 战斗的「台」：四层视差背景、后处理、空气里的粒子、台上的人（精灵、待机起伏、脚下柔影、蓄劲气焰、
// 首领的蓄势气焰）、按事件流播的动画（前冲、刀光、受击闪白后仰、伤害数字、破势碎裂与慢镜、倒下溶解）、
// 开战碎屏。
//
// 只管画与动画的钟，不碰规则：BattleScene 播到哪一条事件就调这里对应的 on*，显示用的气血 / 架势
// （ShownUnit）也由它给。无头模式下 BattleScene 根本不建这个对象——表现层一行都不跑，
// runToCompletion 与既有测试的行为于是一个字节也不变。
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Visual.h"
#include "engine/Engine.h"
#include "engine/Particles.h"
#include "engine/PostFx.h"
#include "game/BattleFx.h"
#include "game/SpriteAtlas.h"

namespace fanren::game {

class Application;

// 一个单位在战斗里长什么样：精灵表（人物或非人形）上的战斗帧与落地点。
// 取法（docs/art-sprites.md 第 5 节）：先看 battle_overrides[战斗 id]，再看按章的变体与 roles，
// 再看 enemies；都没有 → file 为空，退回旧的图元画法（施工图 1.2「缺图不崩」）。
struct BattleLook {
    std::string file;           // 相对 art/sprites/
    int frameW = 16;
    int frameH = 24;
    int cols = 3;
    int footY = 23;
    int headY = 0;
    std::map<std::string, std::vector<int>> poses;   // idle / ready / attack / cast / hurt / down / victory / guard
    core::BattleFacing facing = core::BattleFacing::Left;
    bool floating = false;      // 光球：悬空
    bool humanoid = true;       // 人物表（有走路帧的那种）
};
[[nodiscard]] BattleLook battleLookFor(const SpriteIndex& index, const std::string& battleId,
                                       const std::string& roleId, const ChapterDone& chapterDone);
// 这一刻画哪一帧。没有这个动作的（兽没有施法、光球没有出招）退回待机；待机按 idle 那几帧轮播。
[[nodiscard]] int battleFrame(const BattleLook& look, const std::string& pose, float seconds);
// 要不要水平翻转：人物表面向左、非人形面向右、光球不分朝向；站哪一边就得面朝另一边。
[[nodiscard]] bool battleFlip(const BattleLook& look, bool ally);
// 放大倍数：与背景同一像素密度（×4）；首领大一号（×5）。
[[nodiscard]] float battleScale(bool boss);

// HUD 上跟着事件跳一下的那几样（盾被削、盾回满、破绽格亮起）。HUD 读，台上的钟推。
struct HudPulse {
    float shieldHit = 0.f;      // 秒，倒数
    int shieldFrom = -1;        // 被削之前的架势：旧数字往下掉、淡掉，新数字弹出来（「递减」要看得见）
    float shieldRefill = 0.f;
    float reveal = 0.f;
    int revealBits = 0;         // 这一击揭开 / 打中的那几格
};

class BattleView {
public:
    BattleView(Application& app, const core::battle::BattleState& battle, const std::string& battleId,
               const std::string& backdrop);
    ~BattleView();
    BattleView(const BattleView&) = delete;
    BattleView& operator=(const BattleView&) = delete;

    // ---- 钟 ----
    // sceneDt 是这一帧的「戏里时间」（已乘快进与慢镜），realDt 只乘快进：闪白、震屏、慢镜本身、
    // 碎屏按它走——慢镜只该让碎片与刀光变慢，不该让白闪拖成半秒。
    void update(float sceneDt, float realDt);
    // 慢镜时小于 1。BattleScene 拿它乘这一帧的节拍。
    [[nodiscard]] float timeScale() const;

    // ---- 事件（BattleScene::playEvent 播到哪一条调哪一个）----
    void onAct(const core::battle::BattleEvent& e);
    void onBoost(int unit, int level);
    // alreadyBroken：挨这一下之前就已破势（数字红、带残影）；previousToughness：这一下之前显示的架势
    //（比事件里的少了就是被削了：盾跳一下、数字往下递减）。
    void onHit(const core::battle::BattleEvent& e, bool alreadyBroken, int previousToughness);
    void onBreak(int unit);
    void onRecover(int unit);
    void onHeal(int unit, int hp, int mp);
    void onPoisonTick(int unit, int amount);
    void onPoisoned(int unit);
    void onFall(int unit);
    void onFled(int unit);
    void onWaveIn(int wave);
    void onChargeDeclare(int unit);
    // 看破（天眼术）照到这个敌人：金光一闪，bits 那几格破绽图标一起亮（与「打中揭开」同一套画法）。
    void onReveal(int unit, int bits);
    // 这一手演完了：冲出去的人退回原位、施法的收势、这一手的气焰散掉。
    void endAction();
    // 头顶飘一句（「蓄劲 ×2」「中毒」）。
    void say(int unit, const std::string& text, engine::Color color);

    // 这一帧台上的状况（由 BattleScene 给；显示用的局面以它为准）。
    struct Frame {
        const std::vector<ShownUnit>* shown = nullptr;
        int actor = -1;             // 当前行动者（-1 = 没有）
        bool choosing = false;      // 我方正在选命令：这一位往前站半步、身上是正要蓄的气焰
        int boost = 0;
        bool celebrate = false;     // 打赢了：我方摆胜利的架势
    };
    // 台：背景、人、气焰，走后处理；刀光、闪白、碎片压在后处理之上（夜里也亮）。
    void renderStage(Application& app, const Frame& frame);
    // 飘字、伤害数字、「破势」大字、白闪。画在 HUD 之上。
    void renderOverlay(Application& app);

    // ---- 开战碎屏 ----
    // 还没拍世界画面：BattleScene 这一帧还不算不透明（世界层照画），render 开头调 captureIntro。
    [[nodiscard]] bool introPending() const { return intro_ == Intro::Pending; }
    // 碎片还在飞：这期间不开菜单、敌方不出手。
    [[nodiscard]] bool introBusy() const { return intro_ != Intro::Done; }
    void captureIntro(Application& app);
    void renderIntro(Application& app);
    void skipIntro() { intro_ = Intro::Done; }
    // 截图口：碎片定在第 t 秒不动。
    void holdIntroAt(float t);

    // ---- 给 HUD 借用 ----
    [[nodiscard]] const std::vector<StagePoint>& anchors() const { return anchors_; }
    // 头顶的屏幕 y（名牌、目标指针往这上面放），含悬空与进场的位移。
    [[nodiscard]] float headY(int unit) const;
    [[nodiscard]] const HudPulse& pulse(int unit) const;
    // 行动序条上的小像：人物取头肩那一截，兽与光球整帧缩进去。没有图时画一枚纯色的印。
    void drawPortrait(engine::Engine& e, int unit, const engine::RectF& dst, engine::Color tint) const;
    [[nodiscard]] float clock() const { return clock_; }
    // 眼下这一下震屏的震幅（像素）。系统设置里关了战斗震屏就恒为 0（docs/settings.md 第 1 节 #6）。
    [[nodiscard]] float shakeAmplitude() const { return shakeAmp_; }

private:
    struct Actor;
    struct Effect;
    struct Mote;
    struct Floater;
    struct Flare;
    enum class Intro { Pending, Running, Done };

    void loadBackdrop(engine::Engine& e);
    void drawBackdrop(engine::Engine& e, StagePoint shake) const;
    void drawLayer(engine::Engine& e, engine::TextureId tex, float x, float y, bool wrap) const;
    void drawFog(engine::Engine& e, StagePoint shake) const;
    void drawUnit(engine::Engine& e, int unit, const Frame& frame, StagePoint shake, bool flashOnly) const;
    void drawAuras(engine::Engine& e, int unit, const Frame& frame, StagePoint shake, bool emissive) const;
    void drawEffects(engine::Engine& e, StagePoint shake, bool emissive) const;
    void drawMotes(engine::Engine& e, StagePoint shake) const;
    void drawActiveRing(engine::Engine& e, const Frame& frame, StagePoint shake);
    void drawBreakText(engine::Engine& e) const;
    [[nodiscard]] StagePoint unitOffset(int unit, const Frame& frame) const;
    // 前冲时往下让的那一截（lungeReach）：是在地上走，影子与脚下光圈跟着它挪，不跟那一小跳。
    [[nodiscard]] float sidestep(int unit) const;
    [[nodiscard]] std::string poseOf(int unit, const Frame& frame) const;
    [[nodiscard]] engine::RectF bodyRect(int unit, StagePoint at) const;
    [[nodiscard]] StagePoint bodyCenter(int unit) const;
    [[nodiscard]] engine::PostFxSettings postSettings() const;
    void addEffect(FxSheetId sheet, StagePoint at, float scale, engine::Color tint, bool flip, float angle,
                   float life);
    void burst(FxSheetId sheet, StagePoint at, int count, float speedMin, float speedMax, float gravity,
               float lifeMin, float lifeMax, float size, engine::Color tint);
    void startShake(float amplitude, float duration);
    [[nodiscard]] engine::TextureId fxTexture(FxSheetId id) const;
    [[nodiscard]] bool validUnit(int unit) const;

    engine::Engine& eng_;
    const core::battle::BattleState& battle_;
    std::unique_ptr<engine::PostFx> post_;
    engine::ParticleSystem air_;
    engine::ParticleSystem motes2_;        // 识海的第二层星点（萤火那种一明一灭）
    std::string backdrop_;
    BackdropLook look_;
    std::string breakWord_;                // 「破势」
    std::string weakWord_;                 // 伤害数字上那个「破绽」小标
    std::array<engine::TextureId, 4> layers_{};   // sky / far / mid / ground
    std::array<engine::TextureId, static_cast<std::size_t>(FxSheetId::Count)> fx_{};
    std::vector<StagePoint> anchors_;
    std::vector<Actor> actors_;
    std::vector<Effect> effects_;
    std::vector<Mote> motes_;
    std::vector<Floater> floaters_;
    std::vector<Flare> flares_;
    std::vector<engine::Vertex> mesh_;     // 每帧复用：光环、冲击环、碎屏
    std::vector<int> order_;               // 画的次序（按脚底 y 由远到近），每帧复用

    float clock_ = 0.f;                    // 戏里时间
    float realClock_ = 0.f;
    float slowmo_ = 0.f;
    float whiteFlash_ = 0.f;
    float shakeClock_ = 0.f;
    float shakeDuration_ = 0.f;
    float shakeAmp_ = 0.f;
    bool shakeEnabled_ = true;             // 系统设置的「战斗震屏」，开战时读一次（仗打到一半改不了设置）
    int breakUnit_ = -1;                   // 「破势」大字落在谁头上
    float breakAge_ = -1.f;
    float ringAge_ = -1.f;                 // 破势那一圈冲击波
    StagePoint ringAt_{};

    Intro intro_ = Intro::Pending;
    float introClock_ = 0.f;
    bool introHeld_ = false;
    engine::TextureId introShot_ = engine::kInvalidTexture;
    std::vector<ShatterPiece> pieces_;
};

}  // namespace fanren::game
