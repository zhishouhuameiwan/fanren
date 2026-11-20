#include "game/FoePatrol.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <deque>

namespace fanren::game::foepatrol {
namespace {

constexpr core::Point kSteps[4] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};   // 下标即朝向：上、右、下、左

[[nodiscard]] core::Point ahead(core::Point cell, int facing) {
    const core::Point d = kSteps[((facing % 4) + 4) % 4];
    return core::Point{cell.x + d.x, cell.y + d.y};
}

// 从 from 看 to 在哪一向（两格相邻或同一行列时才有意义；相同时保持 fallback）。
[[nodiscard]] int toward(core::Point from, core::Point to, int fallback) {
    if (to.y < from.y) return 0;
    if (to.x > from.x) return 1;
    if (to.y > from.y) return 2;
    if (to.x < from.x) return 3;
    return fallback;
}

[[nodiscard]] bool adjacent(core::Point a, core::Point b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1;
}

// 张望的第 n 下朝哪：1 看前面，2 左转，3 右转。
[[nodiscard]] int lookFacing(int base, int look) {
    if (look == 2) return (base + 3) % 4;
    if (look == 3) return (base + 1) % 4;
    return base;
}

[[nodiscard]] int routeIndexOf(const Walker& walker, core::Point cell) {
    for (std::size_t i = 0; i < walker.route.size(); ++i) {
        if (walker.route[i] == cell) return static_cast<int>(i);
    }
    return -1;
}

[[nodiscard]] int distanceToRoute(const Walker& walker, core::Point cell) {
    int best = -1;
    for (const core::Point& p : walker.route) {
        const int d = std::abs(p.x - cell.x) + std::abs(p.y - cell.y);
        if (best < 0 || d < best) best = d;
    }
    return best < 0 ? 0 : best;
}

std::string trim(const std::string& text) {
    std::size_t begin = 0;
    std::size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) --end;
    return text.substr(begin, end - begin);
}

// 非负整数；读不出返回 -1。
int number(const std::string& text) {
    const std::string t = trim(text);
    if (t.empty() || t.size() > 6) return -1;
    int value = 0;
    for (const char c : t) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) return -1;
        value = value * 10 + (c - '0');
    }
    return value;
}

}  // namespace

std::vector<core::Point> patrolRoute(const core::MapObject& foe) {
    const std::string text = foe.property("patrol");
    if (trim(text).empty()) return {};
    std::vector<core::Point> route{foe.position};
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t semi = text.find(';', start);
        const std::string piece = text.substr(start, semi == std::string::npos ? std::string::npos : semi - start);
        const std::size_t comma = piece.find(',');
        if (comma == std::string::npos) return {};
        const int x = number(piece.substr(0, comma));
        const int y = number(piece.substr(comma + 1));
        if (x < 0 || y < 0) return {};
        const core::Point to{x, y};
        core::Point at = route.back();
        if (at.x != to.x && at.y != to.y) return {};   // 斜着走：写坏了
        while (!(at == to)) {
            at.x += (to.x > at.x) - (to.x < at.x);
            at.y += (to.y > at.y) - (to.y < at.y);
            route.push_back(at);
        }
        if (semi == std::string::npos) break;
        start = semi + 1;
    }
    return route;
}

bool FoePatrol::sameMap(const core::TileMap& map) const {
    if (&map != map_ || map.id != mapId_) return false;
    if (keepOff_.size() != static_cast<std::size_t>(std::max(0, map.width * map.height))) return false;
    for (const Walker& walker : walkers_) {
        if (walker.objectIndex >= map.objects.size()) return false;
        const core::MapObject& object = map.objects[walker.objectIndex];
        if (object.type != worldobj::kFoe || object.property("id") != walker.id) return false;
    }
    return true;
}

void FoePatrol::sync(const core::TileMap* map) {
    if (map == nullptr && map_ == nullptr) return;
    if (map != nullptr && sameMap(*map)) {
        for (Walker& walker : walkers_) walker.object = &map->objects[walker.objectIndex];
        return;
    }
    map_ = map;
    mapId_ = map != nullptr ? map->id : std::string{};
    walkers_.clear();
    poses_.clear();
    keepOff_.clear();
    grace_ = kArrivalGraceSeconds;
    if (map == nullptr) return;

    // 传送点、踏入触发区，连同它们上下左右那一格：明雷不站上去。
    keepOff_.assign(static_cast<std::size_t>(std::max(0, map->width * map->height)), false);
    const auto mark = [&](int x, int y) {
        if (x < 0 || y < 0 || x >= map->width || y >= map->height) return;
        keepOff_[static_cast<std::size_t>(y * map->width + x)] = true;
    };
    for (const core::MapObject& object : map->objects) {
        const bool stepActivated =
            object.type == "portal" || (object.type == "trigger" && object.property("mode") == "enter");
        if (!stepActivated) continue;
        for (int y = object.position.y; y < object.position.y + std::max(1, object.height); ++y) {
            for (int x = object.position.x; x < object.position.x + std::max(1, object.width); ++x) {
                mark(x, y);
                for (const core::Point& d : kSteps) mark(x + d.x, y + d.y);
            }
        }
    }

    for (std::size_t i = 0; i < map->objects.size(); ++i) {
        const core::MapObject& object = map->objects[i];
        if (object.type != worldobj::kFoe) continue;
        std::vector<core::Point> route = patrolRoute(object);
        if (route.empty()) continue;
        Walker walker;
        walker.object = &object;
        walker.objectIndex = i;
        walker.id = object.property("id");
        walker.route = std::move(route);
        reset(walker);
        walkers_.push_back(std::move(walker));
    }
    for (const Walker& walker : walkers_) commit(walker);
}

void FoePatrol::reset(Walker& walker) {
    // 图上那一格、面朝 facing：先看前面，再左右张望，然后上路。
    walker.cell = walker.object->position;
    walker.facing = worldobj::facingOfName(walker.object->property("facing"));
    walker.mode = Mode::Patrol;
    walker.index = 0;
    walker.dir = 1;
    walker.look = 1;
    walker.lookBase = walker.facing;
    walker.timer = kLookSeconds;
    walker.lost = 0.0;
    walker.alert = false;
}

void FoePatrol::commit(const Walker& walker) { poses_[walker.id] = worldobj::FoePose{walker.cell, walker.facing}; }

const worldobj::FoePoses* FoePatrol::posesFor(const core::TileMap* map) const {
    return map != nullptr && map == map_ ? &poses_ : nullptr;
}

const Walker* FoePatrol::find(const core::TileMap* map, const core::MapObject& foe) const {
    if (map == nullptr || map != map_) return nullptr;
    for (const Walker& walker : walkers_) {
        if (walker.object == &foe) return &walker;
    }
    return nullptr;
}

void FoePatrol::onBattle() {
    grace_ = kBattleGraceSeconds;
    for (Walker& walker : walkers_) {
        if (walker.mode == Mode::Chase) giveUp(walker);
    }
}

const core::MapObject* FoePatrol::advance(const Surroundings& around, double dt) {
    if (map_ == nullptr) return nullptr;
    dt = std::max(0.0, dt);
    grace_ = std::max(0.0, grace_ - dt);
    for (Walker& walker : walkers_) {
        if (!around.present(*walker.object)) {
            // 不在场（清掉了、旗标没置）：不走不看。哪天再出现，从图上那一格起。
            if (!(walker.cell == walker.object->position) || walker.mode != Mode::Patrol) {
                reset(walker);
                commit(walker);
            }
            continue;
        }
        step(walker, around, dt);
    }
    if (grace_ > 0.0) return nullptr;
    for (const Walker& walker : walkers_) {
        if (!around.present(*walker.object)) continue;
        if (ahead(walker.cell, walker.facing) == around.hero) return walker.object;
    }
    return nullptr;
}

void FoePatrol::step(Walker& walker, const Surroundings& around, double dt) {
    walker.timer -= dt;
    switch (walker.mode) {
        case Mode::Patrol:
            // 看人每一帧都看（主角一步只要 0.12 秒）；宽限里不看。
            if (grace_ <= 0.0 && sees(walker, around)) {
                walker.mode = Mode::Chase;
                walker.alert = true;
                walker.look = 0;
                walker.lost = 0.0;
                walker.timer = kSpotSeconds;
                return;
            }
            if (walker.timer > 0.0) return;
            patrolAct(walker, around);
            return;
        case Mode::Chase:
            if (walker.timer > 0.0) return;
            chaseAct(walker, around);
            return;
        case Mode::Return:
            if (walker.timer > 0.0) return;
            returnAct(walker, around);
            return;
    }
}

void FoePatrol::patrolAct(Walker& walker, const Surroundings& around) {
    const int last = static_cast<int>(walker.route.size()) - 1;
    if (walker.look > 0) {
        if (last == 0) {
            // 原地张望：前、左、右轮着看。
            walker.look = walker.look % 3 + 1;
            walker.facing = lookFacing(walker.lookBase, walker.look);
            walker.timer += kLookSeconds;
            commit(walker);
            return;
        }
        if (walker.look < 3) {
            ++walker.look;
            walker.facing = lookFacing(walker.lookBase, walker.look);
            walker.timer += kLookSeconds;
            commit(walker);
            return;
        }
        // 看完了：朝下一格转过去（在路的一头就是掉头），下一下再迈步。
        walker.look = 0;
        if (walker.index + walker.dir < 0 || walker.index + walker.dir > last) walker.dir = -walker.dir;
        walker.facing = toward(walker.cell, walker.route[static_cast<std::size_t>(walker.index + walker.dir)],
                               walker.facing);
        walker.timer += kPatrolStepSeconds;
        commit(walker);
        return;
    }
    if (last == 0) {   // 原地张望的不会走到这里；保险起见回到张望
        walker.look = 1;
        walker.timer += kLookSeconds;
        return;
    }
    if (walker.index + walker.dir < 0 || walker.index + walker.dir > last) walker.dir = -walker.dir;
    const core::Point next = walker.route[static_cast<std::size_t>(walker.index + walker.dir)];
    if (blocked(next, around)) {
        // 路上有人（主角、别的明雷、刚好站过来的什么）：面朝那一格等着。主角站在那里就正在它面前——开打由 advance 判。
        walker.facing = toward(walker.cell, next, walker.facing);
        walker.timer = kBlockedRetrySeconds;
        commit(walker);
        return;
    }
    walker.facing = toward(walker.cell, next, walker.facing);
    walker.cell = next;
    walker.index += walker.dir;
    if (walker.index == 0 || walker.index == last) {
        walker.look = 1;
        walker.lookBase = walker.facing;
        walker.timer += kLookSeconds;
    } else {
        walker.facing = toward(walker.cell, walker.route[static_cast<std::size_t>(walker.index + walker.dir)],
                               walker.facing);
        walker.timer += kPatrolStepSeconds;
    }
    commit(walker);
}

void FoePatrol::chaseAct(Walker& walker, const Surroundings& around) {
    const core::Point hero = around.hero;
    if (adjacent(walker.cell, hero)) {
        // 已经贴着：转身面朝他（正在面前就开打，advance 判）。
        walker.facing = toward(walker.cell, hero, walker.facing);
        walker.lost = 0.0;
        walker.timer += kChaseStepSeconds;
        commit(walker);
        return;
    }
    const std::vector<core::Point> path = pathTo(walker, around, [hero](core::Point cell) { return adjacent(cell, hero); });
    if (path.empty() || static_cast<int>(path.size()) > kTrackSteps) {
        walker.lost += kChaseStepSeconds;
        if (walker.lost >= kLoseSeconds) {
            giveUp(walker);
            return;
        }
        walker.timer += kChaseStepSeconds;
        return;
    }
    walker.lost = 0.0;
    const core::Point next = path.front();
    if (distanceToRoute(walker, next) > kLeashCells) {   // 拴绳到头了
        giveUp(walker);
        return;
    }
    walker.facing = toward(walker.cell, next, walker.facing);
    walker.cell = next;
    if (adjacent(walker.cell, hero)) walker.facing = toward(walker.cell, hero, walker.facing);
    walker.timer += kChaseStepSeconds;
    commit(walker);
}

void FoePatrol::returnAct(Walker& walker, const Surroundings& around) {
    if (const int at = routeIndexOf(walker, walker.cell); at >= 0) {
        // 回到路上了：张望一圈，接着巡逻。原地张望的那种转回它本来面朝的方向（守着的东西在那边），
        // 走路线的从此刻的朝向看起、走到头自会掉头。
        walker.mode = Mode::Patrol;
        walker.index = at;
        walker.look = 1;
        if (walker.route.size() == 1) walker.facing = worldobj::facingOfName(walker.object->property("facing"));
        walker.lookBase = walker.facing;
        walker.timer += kLookSeconds;
        commit(walker);
        return;
    }
    const std::vector<core::Point> path =
        pathTo(walker, around, [&walker](core::Point cell) { return routeIndexOf(walker, cell) >= 0; });
    if (path.empty()) {
        walker.timer = kBlockedRetrySeconds;
        return;
    }
    walker.facing = toward(walker.cell, path.front(), walker.facing);
    walker.cell = path.front();
    walker.timer += kPatrolStepSeconds;
    commit(walker);
}

void FoePatrol::giveUp(Walker& walker) {
    walker.mode = Mode::Return;
    walker.alert = false;
    walker.lost = 0.0;
    walker.look = 0;
    walker.timer = kSpotSeconds;
}

bool FoePatrol::blocked(core::Point cell, const Surroundings& around) const {
    if (map_ == nullptr || !map_->inBounds(cell) || !map_->walkable(cell)) return true;
    if (cell == around.hero) return true;
    if (keepOff_[static_cast<std::size_t>(cell.y * map_->width + cell.x)]) return true;
    return around.solid(cell);
}

bool FoePatrol::sees(const Walker& walker, const Surroundings& around) const {
    core::Point cell = walker.cell;
    for (int k = 0; k < kSightCells; ++k) {
        cell = ahead(cell, walker.facing);
        if (!map_->inBounds(cell) || !map_->walkable(cell)) return false;   // 墙挡视线
        if (cell == around.hero) return true;
        if (around.solid(cell)) return false;   // NPC、设施、宝箱、关着的机关、别的明雷挡视线
    }
    return false;
}

std::vector<core::Point> FoePatrol::pathTo(const Walker& walker, const Surroundings& around,
                                           const std::function<bool(core::Point)>& goal) const {
    const int width = map_->width;
    const int height = map_->height;
    if (width <= 0 || height <= 0) return {};
    const auto index = [width](core::Point p) { return static_cast<std::size_t>(p.y * width + p.x); };
    std::vector<int> previous(static_cast<std::size_t>(width * height), -2);   // -2 没到过，-1 起点
    std::deque<core::Point> queue{walker.cell};
    if (!map_->inBounds(walker.cell)) return {};
    previous[index(walker.cell)] = -1;
    while (!queue.empty()) {
        const core::Point here = queue.front();
        queue.pop_front();
        for (const core::Point& d : kSteps) {   // 上、右、下、左：次序固定，平手时走法也固定
            const core::Point next{here.x + d.x, here.y + d.y};
            if (!map_->inBounds(next) || previous[index(next)] != -2) continue;
            if (blocked(next, around)) continue;
            previous[index(next)] = static_cast<int>(index(here));
            if (goal(next)) {
                std::vector<core::Point> path;
                for (int at = static_cast<int>(index(next)); at >= 0 && previous[static_cast<std::size_t>(at)] != -1;
                     at = previous[static_cast<std::size_t>(at)]) {
                    path.push_back(core::Point{at % width, at / width});
                }
                std::reverse(path.begin(), path.end());
                return path;
            }
            queue.push_back(next);
        }
    }
    return {};
}

}  // namespace fanren::game::foepatrol
