#pragma once
// 地图行走：按格移动、碰撞、与地图对象交互。
//
// 本文件只管规则与「画什么」：谁在场、哪道门开着、哪道门通往目标、哪一格按得动、目标在哪，
// 每帧判定好装进 WorldFrame 交给 game/WorldView.*；「长什么样、按什么顺序画」——烘焙图、精灵、
// 光照、粒子、后处理、HUD——一概在那边（八方旅人化改造，docs/interfaces-octo-world.md）。
//
// **格子语义不许变**：tryStep 一调，GameState::position 立刻变，无头测试靠这一点驱动整章剧情。
// 插值滑动、走路帧、镜头缓动、换图淡入都是 WorldView 里的纯视觉，从不回头去延迟逻辑。
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "core/rules/Objectives.h"
#include "game/Scene.h"
#include "game/WorldView.h"

namespace fanren::game {

class WorldScene : public Scene {
public:
    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;

    [[nodiscard]] std::string name() const override { return "World"; }

    // 供无头 bot 驱动：不经过键盘，直接请求一步移动或一次交互。
    bool tryStep(Application& app, int dx, int dy);
    bool interact(Application& app);

    // ---- 路径行动的钩子（施工图 5.1；P 路规则层落地后由协调者 / P2 接）----
    //
    // 世界层不认识路径行动的数据与规则（那在 core/rules/PathActions.h 与它的加载器里，
    // 游戏层入口还没接），所以这里只留两个口子：
    //   probe  —— 这个 NPC 此刻有没有可用的路径行动、是哪一种：决定头顶浮不浮气泡、浮哪一个；
    //   opener —— 面对这个 NPC 按下 E / Q（Key::Action）时做什么：返回 true 表示受理了。
    // 没接（nullptr）时不浮气泡、E 键不响。进程内全局一份：钩子接的是规则，不是某一个场景实例。
    using PathBubbleProbe = PathBubble (*)(Application& app, const core::MapObject& npc);
    using PathActionOpener = bool (*)(Application& app, const core::MapObject& npc);
    static void setPathActionHooks(PathBubbleProbe probe, PathActionOpener opener);

    // 面朝的那个在场 NPC 身上的路径行动（E 键走的就是这里；公开给无头 bot）。
    // 与 interact 同一道闸：脚本在跑就不受理。没接 opener、面前没有人时返回 false。
    bool pathAction(Application& app);

    // 两个纯判定：只看存档状态与地图对象，不碰渲染与输入。
    // 公开出来是为了能直接单测——它们实现的那几个地图属性此前正是
    // 因为藏在实现文件里没人测，才一路死到内容生产阶段。
    //
    // 曾经还有第三个 markTriggered（触发起了就预先打记号），它是「提前 return 的
    // 脚本会把 once 触发器永久烧掉」那个缺陷的本体，已连同那套时序一并删除：
    // 记号现在由脚本自己的完成旗标兑现，引擎不预写。详见 triggerReady 的注释。
    [[nodiscard]] static bool triggerReady(const core::GameState& state,
                                          const core::MapObject& trigger);
    [[nodiscard]] static bool npcVisible(const core::GameState& state,
                                        const core::MapObject& npc);

    // 这一格上**在场**的那个 npc；一个都没有则 nullptr。
    //
    // 不能直接用 core::TileMap::objectAt(cell, "npc")：它只返回对象表里第一个
    // 矩形命中的，根本不问可见性。于是一个不画、不能谈的 npc 会干两件坏事——
    //
    //   · **当一堵隐形墙**：tryStep 拿 objectAt 挡路，隐身的它照样占着那一格；
    //   · **抢先应声**：同格若还有另一个该出面的 npc，它连命中都轮不到。
    //
    // 后者不是假想。一个 npc 对象只挂得住一个 `script`，所以「同一个位置在不同
    // 章节由不同的人站着」只能靠同格摆两个、各带互补的 visible_flag / hidden_flag，
    // 而这正是 tools/validate.py 规则 17 当场拦下的写法（「被同类靠前的 npc 抢先
    // 命中」）。校验器报的是实情：引擎当时确实会这么干。修的是引擎，校验器跟着改。
    //
    // 口径只加「在不在场」这一条，别的照旧：**没有脚本的 npc 仍然命中**，
    // 于是仍然压住同格排在它后面的那个（objectAt 的老行为，规则 17 同类那条
    // 说的就是它）。这里不顺手把「哑巴 npc 让位」也改了——那是另一件事，
    // 改了就得同时改校验器的另外半条，而本次的账要能一条条对上。
    [[nodiscard]] static const core::MapObject* visibleNpcAt(const core::GameState& state,
                                                            const core::TileMap& map,
                                                            core::Point cell);

    // 地图对象属性里的 facing（up / down / left / right）换成 GameState 那一套
    // 数字朝向。公开是为了能单测：一个拼错的 facing 该退到「朝下」，而不是
    // 让 NPC 面朝一个不存在的方向。
    [[nodiscard]] static int facingFromName(const std::string& name);

    // ---- 跨图指路（第 4 章复验 N-7）----
    //
    // 三个都公开、都是静态的，理由与上面几个一样：画面上「哪道门亮」「左上角写什么」
    // 若只活在 render() 里，就只能靠截图来验。drawObjects 与 drawObjectiveHud 问的
    // 正是这三个函数，一个判据都不自己另写。

    // 目标在别的图上时，这张图上通往它的**下一道门**（rules::nextPortalToward）。
    // 没有目标、目标就在本图、或者此刻走不到时为 nullptr。
    [[nodiscard]] static const rules::PortalLink* guidePortal(Application& app);
    // 这个门对象该不该用目标色描边。从前只认「target_map 就是目标图」的那一道，
    // 第 3 章终局站在谷外时于是一道门都不亮。
    [[nodiscard]] static bool portalLeadsToGoal(Application& app, const core::MapObject& portal);
    // 左上角目标框的第二行：「前往　X」；要先经过别的图时再接「先到　Y」
    //（Y 是下一道门通到的那张图）。目标就在本图时为空串——那时地名是废话。
    [[nodiscard]] static std::string objectiveWhereText(Application& app);

    // 上面两个的「已经算好下一道门」版本。render() 每帧顶上调一次 guidePortal，
    // 再把结果往下传：从前每个门对象各跑一遍全图最短路、目标行再跑一遍。
    // 两个版本是同一个判据——带 app 的那两个就是先算 guidePortal 再转调这两个。
    [[nodiscard]] static bool portalIsGuide(const core::MapObject& portal,
                                            const rules::PortalLink* hop);
    [[nodiscard]] static std::string objectiveWhereText(Application& app,
                                                        const rules::PortalLink* hop);

private:
    // 当前该做的那一步。没有目标链、或全章走完时是 nullptr。
    [[nodiscard]] static const core::Objective* objectiveOf(Application& app);

    // 「画什么」：把这一帧要画的东西按规则判定好，装进 frame_ 交给 view_。
    // 判据全是上面那几个公开的静态函数——画面与走位、对话、指路问的是同一组问题。
    // hop 是本帧的下一道门（guidePortal），render() 顶上算一次传下来。
    void describe(Application& app, const rules::PortalLink* hop);

    // 面朝方向的前一格；交互与触发都看这一格。
    [[nodiscard]] core::Point facingCell(const core::GameState& state) const;

    double stepCooldown_ = 0.0;
    // 存盘之后屏幕上那一句话，以及它还剩多久。
    // 存了却不说话，与没存成在屏幕上长得一模一样。
    std::string saveNotice_;
    double saveNoticeSeconds_ = 0.0;

    WorldView view_;
    WorldFrame frame_;   // 每帧复用
};

}  // namespace fanren::game
