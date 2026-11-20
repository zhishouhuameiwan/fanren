#pragma once
// 明雷的巡逻与追击（2026-11-20 用户试玩第 7 章：「敌人不动的确很呆」。设计、数字与用户原话见
// docs/ch07-playtest-foes.md；契约 docs/interfaces-companions.md 2.4、3.4）。
//
// 只管写了 patrol 的明雷：
//   巡逻 —— 沿 patrol 那一小段路来回走（比主角慢）；走到头停一停，先看前面、再左右转身张望，然后掉头。
//            patrol 只写自己那一格的是原地张望（守宝箱的那种）。
//   看见 —— 朝向直线 kSightCells 格以内、中间没被挡路格与占格对象挡住，就看见了主角：头顶冒「!」，愣一下扑过来。
//   追   —— 按网格最短路朝主角走（比巡逻快），走到主角旁边就转身面朝他。不穿挡路格、宝箱、NPC、别的明雷，
//            也不踩传送点、踏入型触发区——连它们旁边那一格都不站：明雷面前那一格压着它们，主角迈进去就被这一仗
//            吞掉那一步（门禁规则 32 的道理），追人的明雷也就不能把主角逼进剧情。
//   回去 —— 离巡逻路线超过 kLeashCells 格、或闻不到主角（最短路超过 kTrackSteps 步、走不过去）满 kLoseSeconds 秒，
//            就放弃，按最短路走回路线，接着巡逻。回去的路上不看人。
//   开打 —— 主角正站在它面前那一格（它走过来的、转身转过来的都算）就开打，不算先手——与「迈进它面前那一格」
//            同一判据。侧后面朝它按确认的偷袭照旧由 WorldScene::interact 判，追着的时候它总是转过来面朝主角，
//            也就偷袭不成了。
//   宽限 —— 开打之后（逃跑回来、敌人跑了）kBattleGraceSeconds 秒内谁也不看人、不因走到主角面前开打，在追的一律
//            放弃回去；进图（换图、读档）kArrivalGraceSeconds 秒同理——落点恰在它眼前，也给玩家一个走开的工夫。
//
// 没写 patrol（或写坏了）的明雷与改版前一样：站着不动、不看不追，只在主角迈进它面前那一格时开打。
//
// 运行时状态，不进存档：读档（新的 WorldScene）、换图（TileMap 换了一张）都回到图上那一格。
// 按时间推进：WorldScene::update 每帧调 advance。脚本在跑、对话框 / 菜单 / 战斗压在世界层上时 update 轮不到它，
// 明雷一律停住。无头测试按固定步长 tick，结果是确定的。判定一律按主角的逻辑格（GameState::position）。
#include <functional>
#include <string>
#include <vector>

#include "core/model/Types.h"
#include "game/WorldObjects.h"

namespace fanren::game::foepatrol {

// ---- 数字（初值；手感由用户试玩定，改了回写 docs/ch07-playtest-foes.md）----
inline constexpr int kSightCells = 3;                 // 视线：朝向直线几格
inline constexpr int kLeashCells = 6;                 // 拴绳：离巡逻路线（最近那一格，曼哈顿距离）至多几格
inline constexpr int kTrackSteps = 8;                 // 追着时闻得到多远：到主角身边的最短路至多几步
inline constexpr double kPatrolStepSeconds = 0.5;     // 巡逻、回去：一格走多久（主角 0.12）
inline constexpr double kChaseStepSeconds = 0.25;     // 追：一格走多久（比巡逻快，仍比主角慢一倍——跑得掉）
inline constexpr double kLookSeconds = 0.8;           // 张望：每个朝向看多久
inline constexpr double kSpotSeconds = 0.4;           // 冒「!」之后愣多久才扑过来；放弃时也愣这么久再回去
inline constexpr double kLoseSeconds = 2.0;           // 闻不到主角多久就放弃
inline constexpr double kBlockedRetrySeconds = 0.25;  // 路被挡着：多久再试一次
inline constexpr double kBattleGraceSeconds = 3.0;    // 开打之后的宽限
inline constexpr double kArrivalGraceSeconds = 1.0;   // 进图之后的宽限

// patrol 属性：途经格，写成 `x,y;x,y`——从图上那一格起依次走到最后一格，再原路走回来，来回往复。
// 每一段要横平竖直（同一行或同一列），中间的格子自动补上。只写自己那一格 = 原地张望。
// 返回整条路的每一格，第 0 格是图上那一格；没写、写坏了（读不出数、斜着走）返回空：当没写，站着不动
// （门禁 validate.py 规则 32 拦写坏的，这里只是不崩）。
[[nodiscard]] std::vector<core::Point> patrolRoute(const core::MapObject& foe);

// 一帧要知道的「世界」。格子挡不挡由 WorldScene 回答（与主角走位同一组判据：在场的 NPC、设施、宝箱、采集点、
// 关着的机关、**此刻站位上的**明雷），墙、地图边、主角那一格、传送点与踏入触发区那一圈由这里自己管。
struct Surroundings {
    core::Point hero;
    std::function<bool(core::Point)> solid;
    // 这只明雷此刻在不在场（worldobj::foePresent）：不在的不走、不看、不占格，再出现时从图上那一格起。
    std::function<bool(const core::MapObject&)> present;
};

enum class Mode {
    Patrol,   // 沿路来回走、到头张望
    Chase,    // 看见了，追
    Return,   // 放弃了，走回路线
};

// 一只会走的明雷此刻的样子（公开给测试与「画什么」）。
struct Walker {
    const core::MapObject* object = nullptr;   // 每次 sync 都从当前那张图重新取（objectIndex），不跨图留着
    std::size_t objectIndex = 0;               // 在 TileMap::objects 里的下标
    std::string id;
    std::vector<core::Point> route;   // 第 0 格是图上那一格
    core::Point cell;
    int facing = 2;                   // 同 GameState::facing
    Mode mode = Mode::Patrol;
    int index = 0;                    // 巡逻时站在 route 的第几格
    int dir = 1;                      // 沿 route 往哪头走：+1 / -1
    int look = 0;                     // 0 = 在走；1–3 = 张望（前、左、右）
    int lookBase = 2;                 // 张望从哪个朝向起算
    double timer = 0.0;               // 离下一个动作还有几秒
    double lost = 0.0;                // 追着时闻不到主角多久了
    bool alert = false;               // 在追：头顶冒「!」
};

class FoePatrol {
public:
    // 认图：图换了全体回原位，并给一段进场宽限。nullptr = 没有图，全清。
    // 「换了」看指针，再核图名与每只明雷在对象表里的那一项：旧图早已释放，新图恰好落在同一个地址上
    // （两次换图之间世界层一帧都没走，比如脚本连着传送两回）也认得出来，不拿旧图的对象去用。
    void sync(const core::TileMap* map);

    // 推进 dt 秒。返回此刻要开打的那只：主角正站在它面前那一格、不在宽限里；没有为 nullptr。
    // 只有会走的明雷在这里开打；不巡逻的照旧只在 tryStep「迈进面前那一格」时开打。
    const core::MapObject* advance(const Surroundings& around, double dt);

    // 开打了（谁开的都算：迈进面前格、偷袭、追上）：在追的全部放弃回去，宽限 kBattleGraceSeconds 秒。
    // 宽限只在 advance 里倒数——仗打着的时候世界层不推进，回到世界层才开始算。
    void onBattle();

    // 站位表（交给 worldobj 的判定）。map 不是 sync 过的那一张时返回 nullptr——表还是上一张图的，
    // 而 nullptr 的口径（都在图上那一格）正是换图之后的样子。
    [[nodiscard]] const worldobj::FoePoses* posesFor(const core::TileMap* map) const;
    // 这只明雷会不会走、此刻什么样；不会走（没写 patrol）、或 map 不是 sync 过的那一张时为 nullptr。
    [[nodiscard]] const Walker* find(const core::TileMap* map, const core::MapObject& foe) const;
    [[nodiscard]] const std::vector<Walker>& walkers() const { return walkers_; }
    [[nodiscard]] double grace() const { return grace_; }

private:
    void reset(Walker& walker);
    void commit(const Walker& walker);
    void step(Walker& walker, const Surroundings& around, double dt);
    void patrolAct(Walker& walker, const Surroundings& around);
    void chaseAct(Walker& walker, const Surroundings& around);
    void returnAct(Walker& walker, const Surroundings& around);
    void giveUp(Walker& walker);
    [[nodiscard]] bool blocked(core::Point cell, const Surroundings& around) const;
    [[nodiscard]] bool sees(const Walker& walker, const Surroundings& around) const;
    // 从 walker 那一格起，按 blocked 判能不能走，找最近一格满足 goal 的路（不含起点）；找不到为空。
    [[nodiscard]] std::vector<core::Point> pathTo(const Walker& walker, const Surroundings& around,
                                                  const std::function<bool(core::Point)>& goal) const;

    [[nodiscard]] bool sameMap(const core::TileMap& map) const;

    const core::TileMap* map_ = nullptr;
    std::string mapId_;
    std::vector<Walker> walkers_;
    worldobj::FoePoses poses_;
    std::vector<bool> keepOff_;   // 传送点、踏入触发区与它们旁边一格（width × height）
    double grace_ = 0.0;
};

}  // namespace fanren::game::foepatrol
