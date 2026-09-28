#pragma once
// 战斗画面的纯计算：站位、按类别挑特效、几条动画曲线、BGM 规则、背景的时辰与粒子、行动序条排版、
// 开战碎屏的几何。一律不碰引擎的绘制（只借 Color / RectF 这几个值类型），好让
// tests/BattleViewTests.cpp 不开窗口就钉住「画面该是什么样」——这些数一旦只活在 render() 里，
// 就只剩截图能验，而截图验不出「第四个敌人站到了日志框里」这种事。
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Visual.h"
#include "engine/Engine.h"
#include "engine/Particles.h"

namespace fanren::game {

// 画面上显示的那一份局面：事件播到哪一条，它就更新到哪一条（BattleScene::playEvent），
// 于是血条在那一刀落下的时候才掉。HUD 与台上的人读的都是它，不是规则层的现值。
struct ShownUnit {
    int hp = 0;
    int toughness = 0;
    int revealed = 0;
    bool broken = false;
    bool visible = true;
};

struct StagePoint {
    float x = 0.f;
    float y = 0.f;
};

// ---- 站位（docs/octopath-battle.md 6.1）----
//
// 锚点是脚底中点。敌左我右，都站在背景的地面带里（背景 320×180 ×4，地平线在 y≈384，
// 美术约定站位带是画面高度的 55%–85%）。rank 是同一边里的第几个（敌人按同一波数），count 是共几个。
[[nodiscard]] StagePoint standPoint(bool ally, int rank, int count);
// 整张表：下标与 units 对齐。敌人按「同一波里的第几个」排：下一波进场时上一波已经倒光，
// 新的一波从头占位；已经站着的人不会因为身边有人倒下而挪位置。
[[nodiscard]] std::vector<StagePoint> standPoints(const std::vector<core::battle::Unit>& units);

// 台上一个人占的地方（像素，已乘放大倍数）：宽取整帧宽，高取头顶到脚底（精灵索引的 headY..footY）。
struct StageBody {
    float w = 0.f;
    float h = 0.f;
};

// 前冲冲到位时相对站位的位移（下标与 units、stand、bodies 对齐）。终审 LOW-5：我方站成一条斜线，后一位比
// 前一位靠后 64、靠下 70（挤时 56），直着冲 70 正好冲进前一位身上、还画在他上面（截图 42、78）。
// 所以前面挡着同伴时先往镜头这边（下方）让，让到自己的头顶低过他的脚底再冲；往下让不开（再低就踩进
// 右下角的队伍面板）就少冲一截，停在他身后。只管我方：敌方没人说过穿人，队形也不动它。
[[nodiscard]] StagePoint lungeReach(const std::vector<core::battle::Unit>& units, const std::vector<StagePoint>& stand,
                                    const std::vector<StageBody>& bodies, std::size_t who);

// 站位不许压到的几块 HUD：左下日志、右下队伍面板（脚下还要留出敌人的架势行）。
inline constexpr float kStageFloorY = 640.f;        // 敌人脚下那一行的底边不得低于它
inline constexpr engine::RectF kPartyPanelRect{848.f, 588.f, 416.f, 120.f};
inline constexpr engine::RectF kLogRect{16.f, 652.f, 780.f, 56.f};

// ---- 特效贴图（assets/art/fx，由 tools/artgen/fx.py 生成）----
//
// 帧布局抄自 assets/art/fx/index.json。game 层不解析 JSON（分层约定），而气焰是 32×40 的
// 三行三列，按「宽 ÷ 高 = 帧数」推不出来，所以这里写死一张表；A2 改了图的布局要跟着改这里。
enum class FxSheetId {
    Slash,
    Impact,
    FireBurst,
    Wind,
    Poison,
    Heal,
    Spark,
    Glow,
    Shards,
    BoostAura,
    BossAura,
    Shadow,
    Fog,
    Count,
};
struct FxSheet {
    const char* file = "";     // 相对 art/fx/
    int frameW = 0;
    int frameH = 0;
    int frames = 1;            // 每行几帧
    int rows = 1;
    float fps = 0.f;
    bool additive = false;
};
[[nodiscard]] const FxSheet& fxSheet(FxSheetId id);
// 第 row 行（0 起）、这一刻第几帧的像素矩形。fps 为 0 时恒为第 0 帧；loop 为假时停在最后一帧。
[[nodiscard]] engine::RectF fxFrameRect(const FxSheet& sheet, int row, float seconds, bool loop);

// ---- 按攻击类别挑特效 ----
//
// 剑刀 → 刀光；拳 → 钝击；火 → 火爆；金 → 金光斩（刀光染金）；毒 → 毒雾；木（风）→ 风旋；
// 水 → 钝击染冰蓝；土 → 钝击染赭；暗器 → 细刀光；没有类别的法术（神识冲）→ 风旋。
// 一击带几种类别时（带毒的法术）按「元素 > 毒 > 兵刃」挑最醒目的那一种。
struct StrikeLook {
    FxSheetId sheet = FxSheetId::Impact;
    engine::Color tint{255, 255, 255, 255};
    float scale = 3.f;          // 48×48 的帧放大几倍
    const char* hitSfx = "hit_blunt";
};
[[nodiscard]] StrikeLook strikeLook(int category);
// 施法那一下（Act）的音效：火 / 金 / 毒各有一声，其余一律风声。
[[nodiscard]] const char* castSfx(int category);

// ---- 曲线 ----

// 伤害数字的弹跳：出现后 t 秒离基线多高（像素，向上为正）。先蹿上去、落回、再小弹一下、停住。
[[nodiscard]] float numberLift(float t);
// 数字弹出那一下的字号倍数：1.45 → 1.0。
[[nodiscard]] float numberPop(float t);
// 飘字的透明度（0–1）：最后 0.25 秒淡出。
[[nodiscard]] float floaterAlpha(float t, float life);
// 屏幕震动的包络：t = 0 时为 1，单调不增，duration 起恒为 0。
[[nodiscard]] float shakeEnvelope(float t, float duration);
// 这一刻的震动位移（像素）。两条不同频的正弦叠出来的，不耗随机数：同一时刻永远同一个位移，截图可复现。
[[nodiscard]] StagePoint shakeOffset(float t, float duration, float amplitude);
[[nodiscard]] float easeOutCubic(float t);
[[nodiscard]] float smoothStep(float edge0, float edge1, float x);

// 首领：画得大一号。判据是数据里给它的东西（多次行动、会蓄势、架势八点以上），不认名字。
[[nodiscard]] bool looksLikeBoss(const core::battle::Unit& unit);

// ---- BGM ----
//
// 识海 → bgm_soulsea；编成里有首领（任一敌人一回合动不止一次、或会蓄势，哪一波的都算）→ bgm_boss；
// 其余 → bgm_battle。判据是数据里给它的东西，不认名字。
[[nodiscard]] std::string battleBgm(const std::vector<core::battle::Unit>& units, bool mind);

// ---- 背景 ----

// 这一场用哪一套背景（assets/art/battle/<名>/）。先后：编成自己写的 backdrop → data/visual/battles.json
// 的覆写 → 识海一律 soulsea → 所在地图 meta 的 backdrop → 按地形兜底。
[[nodiscard]] std::string resolveBackdrop(const std::string& setupBackdrop, const std::string& terrain,
                                          const std::string& visualOverride,
                                          const std::string& mapBackdrop);

// 一套背景的时辰与空气里飘的东西（docs/art-maps.md 第 8 节那张表）。
struct BackdropLook {
    core::TimeOfDay time = core::TimeOfDay::Day;
    bool soulsea = false;       // 识海：自有一套调色，星点满天
    engine::ParticleKind particle = engine::ParticleKind::Dust;
    float rate = 6.f;           // 每秒生几颗
    bool fog = false;           // 远景上压一层慢慢漂的雾
};
[[nodiscard]] BackdropLook backdropLook(const std::string& backdrop);

// ---- 行动序条（顶端）----
//
// 本回合还没动的人（第一个是当前行动者，大一号）→ 一道分隔 → 下回合的预览（小一号）。
// 放不下的从尾巴上截掉，不换行、不缩。
struct OrderChip {
    int unit = -1;
    float x = 0.f;
    float y = 0.f;
    float size = 0.f;
    bool current = false;
    bool nextRound = false;
};
struct OrderStrip {
    std::vector<OrderChip> chips;
    float dividerX = -1.f;      // 分隔的中线；本回合一个都没有时也照画（「本回合」已经走完）
};
inline constexpr float kOrderChipCurrent = 48.f;
inline constexpr float kOrderChip = 40.f;
inline constexpr float kOrderChipNext = 32.f;
inline constexpr float kOrderGap = 6.f;
inline constexpr float kOrderDivider = 30.f;
inline constexpr float kOrderBaseline = 64.f;   // 芯片底边对齐到这条线
[[nodiscard]] OrderStrip layoutOrderStrip(const std::vector<int>& thisRound,
                                          const std::vector<int>& nextRound, float left, float right);

// ---- 开战碎屏 ----
//
// 把开战那一帧的世界画面切成 cols×rows 格、每格沿一条随机对角线劈成两片三角，从画面中心往外崩飞。
// 同一个种子切出来的一模一样（截图口要可复现）。坐标是 1280×720 的逻辑像素；uv 归一化到 [0,1]，
// 快照是多大的纹理都照贴（窗口放大时快照可能是 2560×1440）。
struct ShatterPiece {
    std::array<StagePoint, 3> corner{};
    std::array<StagePoint, 3> uv{};
    StagePoint velocity{};      // 像素 / 秒
    float spin = 0.f;           // 度 / 秒
    float delay = 0.f;          // 离中心越远崩得越晚
};
inline constexpr float kShatterDuration = 1.1f;   // 这之后一片都不剩
[[nodiscard]] std::vector<ShatterPiece> shatterPieces(int cols, int rows, std::uint32_t seed);
// t 秒时这一片三个角在哪（绕自己的重心转、带重力往下掉）。
[[nodiscard]] std::array<StagePoint, 3> shatterPose(const ShatterPiece& piece, float t);
[[nodiscard]] float shatterAlpha(const ShatterPiece& piece, float t);

}  // namespace fanren::game
