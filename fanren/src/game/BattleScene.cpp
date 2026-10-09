#include "game/BattleScene.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "core/rules/Realm.h"
#include "game/Application.h"
#include "game/BattleHud.h"
#include "game/BattleView.h"
#include "game/ShopScene.h"
#include "io/VisualLoader.h"

namespace fanren::game {
namespace {

using core::battle::Action;
using core::battle::ActionKind;
using core::battle::BattleEvent;
using core::battle::BattleEventKind;
using core::battle::BattlePhase;
using core::battle::BattleState;
using core::battle::Unit;

// ---- 节拍 ----
// 每条事件在画面上停多久（戏里秒；慢镜时戏里的一秒更长）。按住 Ctrl 四倍速。数值只管「看得清」：
// 一刀 0.32 秒看得清刀光落下、数字跳出来；破势 0.8 秒——前 0.3 实秒是慢镜，碎片崩开要看得见；
// 蓄势的预告句要读完一整句，给 0.9 秒。表见 docs/octopath-battle.md 6.4。
constexpr double kFastForward = 4.0;
constexpr double kAiThinkSeconds = 0.25;   // 敌方出手前的停顿，否则看不出轮到谁了
constexpr double kBannerLife = 2.4;
constexpr double kIntroShotAt = 0.32;      // 截图口拍碎屏：定在碎片刚崩开的那一刻
constexpr double kShotSettle = 0.35;       // 截图口拍破势：那一下之后再走这么久（实秒），碎片与大字展开
constexpr int kShotPrerollFrames = 60 * 40;

// 动作菜单的面板：摆在我方与敌阵之间，竖直方向跟着当前我方角色（「贴在当前角色旁」），
// 横向固定——右边缘紧贴我方最前面那一位的左肩，不压人、也不压敌人第一列。
constexpr int kMenuX = 536;
constexpr int kMenuW = 330;
constexpr int kMenuH = 360;
constexpr int kMenuFeedbackH = 72;

[[nodiscard]] bool validUnit(const BattleState& battle, int index) {
    return index >= 0 && static_cast<std::size_t>(index) < battle.units().size();
}

// 行动者「会」的法术 ∩ 本场登记表。会而没登记的（识海那一场）不进列表：checkCast 对它
// 给的理由是「没有这门法术：<id>」，那是说给开发者听的，不是说给玩家听的。
[[nodiscard]] std::vector<std::string> knownMagicIds(const BattleState& battle, int actorIndex) {
    if (!validUnit(battle, actorIndex)) return {};
    const Unit& actor = battle.units()[static_cast<std::size_t>(actorIndex)];
    if (!actor.magicsExhaustive && actor.magics.empty()) return battle.registeredMagicIds();
    std::vector<std::string> ids;
    for (const std::string& id : actor.magics) {
        if (battle.hasMagic(id)) ids.push_back(id);
    }
    return ids;
}

// 一行条目：reason 非空即置灰并写明原因。只把字变灰而不写原因，玩家一律会当成 bug。
[[nodiscard]] ui::ListItem row(std::string label, std::string detail, std::string reason) {
    ui::ListItem item;
    item.label = std::move(label);
    item.detail = std::move(detail);
    if (!reason.empty()) {
        item.enabled = false;
        item.disabledReason = std::move(reason);
    }
    return item;
}

[[nodiscard]] ui::ListItem backRow() { return row("返回", {}, {}); }

// 一手动作对「站在 wantAlly 那一边的任何一个人」做不做得成：做得成返回空串，
// 做不成返回第一个人身上的理由。
//
// 从前（战棋）探的是「离得最近的那一个」——射程是最后一道判定，最近的都够不着别人只会更远。
// 格子没了之后判据只剩「这个人能不能挨这一下」，而解毒药那一条是按人判的：自己没中毒、
// 同伴中着毒，这包药就是用得上的。所以逐个试，自己排第一个（丹药多半是给自己吃的）。
[[nodiscard]] std::string refuseOnAnyone(const core::GameState& state, const BattleState& battle,
                                         Action probe, bool wantAlly) {
    if (!validUnit(battle, probe.actorIndex)) return "此刻无人可以行动";
    const std::vector<Unit>& units = battle.units();
    const bool actorAlly = units[static_cast<std::size_t>(probe.actorIndex)].ally;
    std::vector<int> candidates;
    if (wantAlly) candidates.push_back(probe.actorIndex);
    for (std::size_t i = 0; i < units.size(); ++i) {
        const int index = static_cast<int>(i);
        if (index == probe.actorIndex || !units[i].alive()) continue;
        if ((units[i].ally == actorAlly) == wantAlly) candidates.push_back(index);
    }
    if (candidates.empty()) return wantAlly ? "身边没有可以用的人" : "场上没有可以攻击的对象";
    std::string first;
    for (const int target : candidates) {
        probe.targetIndex = target;
        std::string why = refusePlayerAction(state, battle, probe);
        if (why.empty()) return {};
        if (first.empty()) first = std::move(why);
    }
    return first;
}

[[nodiscard]] std::string menuTitle(BattleMenuMode mode, PanelStage stage) {
    switch (mode) {
        case BattleMenuMode::Magic:  return magicWord(stage);
        case BattleMenuMode::Item:   return "物品";
        case BattleMenuMode::Target: return "择敌";
        case BattleMenuMode::Weapon: return "兵刃";
        case BattleMenuMode::Root:
        case BattleMenuMode::Closed: break;
    }
    return "行动";
}

// 择敌那一行的右栏：只放破绽。行宽就是菜单那么宽，名字长一点（「四平帮头目」）右栏就只剩一半；
// 从前把气血、架势、破绽拼成一串，被截掉的总是最后那截破绽——而择敌要看的恰恰是它（终审 LOW-1）。
// 气血与架势整句写在菜单下方的说明区（foeStatusText），那里按宽度折行，一个字也不截。
[[nodiscard]] std::string foeRowDetail(const Unit& unit) {
    std::string slots;
    for (const std::string& slot : weaknessSlots(unit)) slots += slot;
    return slots.empty() ? std::string{} : "破绽 " + slots;
}

// 一门法术这一手打什么，写在菜单行尾（法术列表与物品列表的符箓共用）：削架势的写「削架势 N」
//——它不判破绽，写五行只会让人以为打得中哪一样；别的写它的类别（五行，带毒加「毒」），没有就空着。
[[nodiscard]] std::string spellEffectLabel(const core::Magic& magic) {
    if (magic.effect == core::MagicEffect::Stagger) return "削架势 " + std::to_string(magic.stagger);
    return core::categoryNames(core::magicCategories(magic));
}

}  // namespace

std::string foeStatusText(const Unit& unit) {
    std::string text = "气血 " + std::to_string(std::max(0, unit.hp)) + "/" + std::to_string(unit.maxHp);
    if (unit.maxToughness > 0) {
        text += " · 架势 " + std::to_string(unit.toughness);
        std::string slots;
        for (const std::string& slot : weaknessSlots(unit)) slots += slot;
        if (!slots.empty()) text += " · 破绽 " + slots;
    }
    return text;
}

// ---------------------------------------------------------------------------
// 自由函数
// ---------------------------------------------------------------------------

std::string refusePlayerAction(const core::GameState& state, const BattleState& battle,
                               const Action& action) {
    const std::vector<Unit>& units = battle.units();
    const bool byOurSide = validUnit(battle, action.actorIndex) &&
                           units[static_cast<std::size_t>(action.actorIndex)].ally;
    // 只有我方掏的是玩家的背包，而战场的登记表是开战那一刻的快照：用光最后一颗
    // 之后它还留在表里。这一闸排在 core 的一切判定之前，与 issuePlayerAction 同序。
    if (action.kind == ActionKind::Item && byOurSide && state.itemCount(action.magicId) <= 0) {
        return "背包里已经没有这件东西了";
    }
    std::string why;
    if (battle.isLegal(action, &why)) return {};
    // deny() 每一条都写了理由，走到这里 why 不会是空的；真空了也不能返回空串
    // ——那会被上层读成「这一步做得了」。规则层的句子是修仙说法，按阶段换词再上屏（Wording.h）。
    return why.empty() ? std::string("这一步做不了") : stageWords(wordingStage(state), std::move(why));
}

std::string poisonStatusText(const Unit& unit) {
    if (unit.poison <= 0) return {};
    // 两个数都要写出来：还要掉几回合、一回合掉多少（契约第 2.2 节）。
    return "中毒 " + std::to_string(unit.poison) + " 回合（每回合 " +
           std::to_string(unit.poisonPower) + " 点）";
}

std::string waveStatusText(const BattleState& battle) {
    if (battle.waveCount() <= 1) return {};
    return "第 " + std::to_string(battle.currentWave() + 1) + " 波 / 共 " +
           std::to_string(battle.waveCount()) + " 波";
}

std::vector<std::string> weaknessSlots(const Unit& unit) {
    std::vector<std::string> slots;
    if (unit.maxToughness <= 0) return slots;
    for (const int bit : core::categoryBits(unit.weaknesses)) {
        slots.emplace_back((unit.revealed & bit) != 0 ? core::categoryName(bit) : "？");
    }
    return slots;
}

std::string breakStatusText(const BattleState& battle, const Unit& unit) {
    if (!unit.broken()) return {};
    return "破势 · 余 " + std::to_string(unit.breakRecoverRound - battle.round()) + " 回合";
}

std::string backdropOf(const core::BattleSetup& setup) {
    if (!setup.backdrop.empty()) return setup.backdrop;
    return setup.terrain.empty() ? std::string("field") : setup.terrain;
}

std::string battleBackdropFor(Application& app, const std::string& battleId) {
    // 地图 meta 是美术产物，在引擎的资产根（assets/art/maps/）下；指派表是数据，在 Application 的资产根下。
    // 两个根不是同一个——HIGH-1 就是拿前一个根去找后一张表。
    std::string mapBackdrop;
    if (const std::vector<std::string> meta = app.engine().findAssets("art/maps/" + app.state().mapId, "meta.json");
        !meta.empty()) {
        if (core::Result<core::MapMeta> loaded = io::loadMapMeta(meta.front()); loaded) {
            mapBackdrop = loaded.value.backdrop;
        }
    }
    // 编成查不到（兜底遭遇）时地形按 field 算，与 BattleScene::buildEncounter 同一口径。
    const core::BattleSetup* setup = app.battleSetup(battleId);
    return resolveBackdrop(setup != nullptr ? setup->backdrop : std::string{},
                           setup != nullptr ? setup->terrain : std::string("field"), app.battleBackdrop(battleId),
                           mapBackdrop);
}

int heroWeapons(const core::GameData& data, const core::GameState& state) {
    int weapons = core::kCategoryFist;   // 空手永远算一样
    for (const core::BagEntry& entry : state.bag) {
        if (entry.count <= 0) continue;
        if (const core::Item* item = data.findItem(entry.itemId)) weapons |= item->weapon;
    }
    return weapons;
}

bool battleRewardEarned(BattlePhase phase) {
    // **不要加 default:**：新增一个战果时让 /W4 逼人回来想一想「这一种发不发」。
    switch (phase) {
        case BattlePhase::Won:
            return true;
        case BattlePhase::Lost:        // 输了：没有人会把战利品留给输家
        case BattlePhase::Escaped:     // 自己跑了：场上的东西他没去捡
        case BattlePhase::EnemyFled:   // 对方跑了：设计文档里没有一处说这算赢到了东西
        case BattlePhase::Ongoing:     // 没打完就收场只可能是驱动方出错，别拿它发东西
            return false;
    }
    return false;
}

GrantedReward grantBattleReward(core::GameState& state, const core::BattleReward& reward,
                                BattlePhase phase) {
    GrantedReward granted;
    if (!battleRewardEarned(phase)) return granted;

    // 负数一律不收：「打赢了反倒掉修为」是一条玩家当场就会报的 bug。
    if (reward.cultivation > 0) {
        // 饱和加法：修为是 int，一份写错量级的数据不该把它绕成负数。
        const long long sum = static_cast<long long>(state.cultivation) + reward.cultivation;
        const int total = static_cast<int>(std::min<long long>(sum, std::numeric_limits<int>::max()));
        granted.cultivation = total - state.cultivation;
        state.cultivation = total;
    }
    if (reward.spiritStones > 0) {
        // 商店卖出时怎么加钱，这里就怎么加（ShopScene.cpp 的 sell 那一句）。
        state.addItem(kSpiritStoneItemId, reward.spiritStones, 0);
        granted.money = reward.spiritStones;
    }
    for (const core::BagEntry& drop : reward.drops) {
        if (drop.itemId.empty() || drop.count <= 0) continue;
        state.addItem(drop.itemId, drop.count, drop.herbAge);
        granted.drops.push_back(drop);
    }
    return granted;
}

// ---------------------------------------------------------------------------
// 建场
// ---------------------------------------------------------------------------

BattleScene::BattleScene(std::string battleId, BattleShot shot)
    : battleId_(std::move(battleId)), shot_(shot) {}

BattleScene::~BattleScene() = default;

bool BattleScene::opaque() const { return view_ == nullptr || !view_->introPending(); }

void BattleScene::onEnter(Application& app) {
    settled_ = false;
    finished_ = false;
    granted_ = GrantedReward{};
    closeMenu();
    feedback_.clear();
    boost_ = 0;
    playback_.clear();
    banner_.clear();
    beat_ = 0.0;
    aiDelay_ = 0.0;
    card_.reset();
    frozen_ = false;
    shotHold_ = -1.0;
    shotFallback_ = false;
    stageActor_ = -1;
    lastPlayed_.reset();
    buildEncounter(app);
    // 开场那几条事件（第一回合、开场毒发）不必演：局面就是开场的样子。
    eventCursor_ = battle_.events().size();
    syncShown();
    if (app.headless()) return;

    // ---- 画面：只在有人看的时候建 ----
    engine::Engine& e = app.engine();
    view_ = std::make_unique<BattleView>(app, battle_, battleId_, battleBackdropFor(app, battleId_));
    icons_ = e.loadTexture("art/ui/icons.png");
    e.playBgm(battleBgm(battle_.units(), battle_.devourMode()));
    if (shot_ == BattleShot::None) {
        e.playSfx("encounter");
        return;
    }
    prepareShot(app);
}

void BattleScene::buildEncounter(Application& app) {
    if (const core::BattleSetup* setup = app.battleSetup(battleId_)) {
        buildFromSetup(app, *setup);
        return;
    }
    // 查不到就退回内置遭遇：未知 id 多半是脚本笔误，给一场能打的杂兵战
    // 好过把玩家丢进空战场。日志由上层记录。
    buildProbeEncounter(app);
}

namespace {

// 韩立这个单位。只在他在场时建；在场时他永远是 0 号。
Unit makeHero(Application& app, bool mind) {
    Unit hero;
    hero.id = "hanli";
    hero.name = app.speakerName("hanli");
    // 识海里比的是元神的体积，与肉身气血无关（kDevourPlayerVolume 的注释）。
    hero.maxHp = mind ? core::battle::kDevourPlayerVolume : std::max(1, app.state().maxHp);
    hero.hp = mind ? hero.maxHp : std::max(1, app.state().hp);
    hero.maxMp = app.state().maxMp;
    hero.mp = app.state().mp;
    // 攻防取自境界（技术债 G-10）。识海那一场不另开口径：吞噬模式下攻防一概不参与。
    hero.attack = rules::realmAttack(app.state().realm);
    hero.defence = rules::realmDefence(app.state().realm);
    hero.speed = 5;
    hero.realm = app.state().realm;
    hero.ally = true;
    // 已习得法术取自存档（技术债 G-4）；清单是穷尽的，学过几条就是几条。
    hero.magics = app.state().learnedMagics;
    hero.magicsExhaustive = true;
    // 兵刃：空手一样，背包里每件兵器再加一样（heroWeapons）。
    hero.weapons = heroWeapons(app.data(), app.state());
    return hero;
}

// 一个按 data/roles 模板建的单位。敌我共用：码头打手这种敌友两用的角色，站哪一边
// 由编成说了算，而架势、破绽、蓄势只对敌人有意义——我方不会破势，也不会蓄势。
Unit unitFromRole(const core::GameState& state, const core::RoleTemplate& role, bool ally) {
    Unit unit;
    unit.id = role.id;
    unit.name = role.name;
    unit.hp = unit.maxHp = std::max(1, role.maxHp);
    unit.mp = unit.maxMp = role.maxMp;
    unit.attack = role.attack;
    unit.defence = role.defence;
    unit.speed = role.speed;
    unit.realm = role.realm;
    unit.element = role.element;
    unit.magics = role.magics;
    // data/roles 里那份 magics 是穷尽的：声明了几条就是几条（Unit::magicsExhaustive）。
    unit.magicsExhaustive = true;
    unit.ally = ally;
    unit.weapons = role.weapons;
    unit.actions = role.actions;
    if (!ally) {
        unit.maxToughness = role.toughness;
        unit.weaknesses = role.weaknesses;
        // 上一场揭开过的破绽，这一场一开场就是亮的（GameState::knownWeaknesses）。
        unit.revealed = state.knownWeaknessesOf(role.id) & role.weaknesses;
        unit.chargeEvery = role.charge.every;
        unit.chargeMult = role.charge.mult;
        unit.chargeAll = role.charge.all;
        unit.chargeTextKey = role.charge.textKey;
        unit.killableBy = role.killableBy;
    }
    return unit;
}

// 种子由战斗 id 派生而非取当前时间：同一场战斗在 bot 回归里必须可复现，
// 不同战斗之间又不该共用同一串随机数。
std::uint32_t seedFor(const std::string& battleId) {
    std::uint32_t seed = 2166136261u;
    for (const char ch : battleId) seed = (seed ^ static_cast<std::uint8_t>(ch)) * 16777619u;
    return seed;
}

}  // namespace

void BattleScene::buildFromSetup(Application& app, const core::BattleSetup& setup) {
    std::vector<Unit> units;
    units.reserve(setup.units.size() + app.state().party.size() + 1);
    const bool mind = setup.terrain == kMindTerrain;

    // ---- 韩立不在场（契约 docs/interfaces-p3-ch05.md 第 2 节）----
    // 不建他，也不带他的同伴、不登记他背包里的东西，我方就是编成里写死的 ally。
    // 他在场时永远是 0 号：战斗结束后按下标把气血写回存档。
    heroPresent_ = !setup.heroAbsent;
    if (heroPresent_) units.push_back(makeHero(app, mind));

    for (const core::BattleUnitSpec& spec : setup.units) {
        const core::RoleTemplate* role = app.data().findRole(spec.roleId);
        if (role == nullptr) continue;   // 内容门禁会拦下悬空引用，运行期跳过即可
        Unit unit = unitFromRole(app.state(), *role, spec.ally);
        // 波次：缺省 0 = 开场就在场上。onField 由 BattleState::setup 按 wave 推出来。
        unit.wave = spec.wave;
        units.push_back(std::move(unit));
    }

    // 同伴跟着他走：他不在场，他们也不在（契约 2.2）。
    if (heroPresent_) addPartyUnits(app, units);

    battle_.setup(std::move(units), seedFor(setup.id));
    battle_.setCanEscape(setup.canEscape);
    // 吞噬模式挂在地形上。梦里没有法术、没有物品、没有架势与劲、不能逃。
    battle_.setDevourMode(mind);

    // ---- 登记法术与物品（必须在 setup 之后：setup 会清空这两张表）----
    // **识海那一场一件都不登记，这是一条明写的例外**（契约 3.5 节）。
    // 韩立不在场的仗不登记背包：药囊在他身上。
    if (!mind) {
        registerFieldMagics(app);
        if (heroPresent_) registerBagItems(app);
    }
    if (!setup.introKey.empty()) lastLog_ = app.text(setup.introKey);
}

void BattleScene::registerFieldMagics(Application& app) {
    // 登记范围 = 场上各单位在 data/roles 里声明的那些，取并集。真正把每个单位钉在
    // 自己那份清单上的是 Unit::magicsExhaustive，两件事缺一不可。
    for (const Unit& unit : battle_.units()) {
        for (const std::string& id : unit.magics) {
            if (const core::Magic* magic = app.data().findMagic(id)) battle_.addMagic(*magic);
        }
    }
}

void BattleScene::registerBagItems(Application& app) {
    // 登记范围 = 玩家背包里战斗中真有效果的那些，判据在 core::battleUsable。
    // 带 castMagic 的符箓连它施展的那门法术一起登记：结算按那门法术走（契约
    // docs/interfaces-p3-ch07.md 2.3）。登记表里多一门法术不会让谁学会它——各单位只施展得了
    // 自己那份清单上的（Unit::magicsExhaustive，game 层建场一律置位）。
    for (const core::BagEntry& entry : app.state().bag) {
        if (entry.count <= 0) continue;
        const core::Item* item = app.data().findItem(entry.itemId);
        if (item == nullptr || !core::battleUsable(*item)) continue;
        battle_.addItem(*item);
        if (item->castMagic.empty()) continue;
        if (const core::Magic* magic = app.data().findMagic(item->castMagic)) battle_.addMagic(*magic);
    }
}

void BattleScene::addPartyUnits(Application& app, std::vector<Unit>& units) const {
    for (const core::PartyMember& member : app.state().party) {
        if (!member.active) continue;   // 留着位置但不上场的同伴
        // 同一个角色不得同时出现两次（契约第 1.4 节），编成里已写死的 ally 优先。
        const bool already = std::any_of(units.begin(), units.end(), [&member](const Unit& u) {
            return u.id == member.roleId;
        });
        if (already) continue;
        const core::RoleTemplate* role = app.data().findRole(member.roleId);
        if (role == nullptr) continue;
        Unit unit = unitFromRole(app.state(), *role, /*ally=*/true);
        // hp = -1 表示「按角色模板满血」；存下来的具体血量夹在 [1, maxHp]。
        unit.hp = member.hp < 0 ? unit.maxHp : std::clamp(member.hp, 1, unit.maxHp);
        // 从前（战棋）这里要从主角出生点向外找一格空地；横版里站位由界面按顺序排，
        // 「站不下就不上场」这一条随之消失。
        units.push_back(std::move(unit));
    }
}

void BattleScene::buildProbeEncounter(Application& app) {
    heroPresent_ = true;   // 兜底遭遇永远有他
    std::vector<Unit> units;
    units.push_back(makeHero(app, /*mind=*/false));

    Unit beast;
    beast.id = "wild_dog";
    beast.name = "野狗";
    beast.hp = beast.maxHp = 18;
    beast.attack = 4;
    beast.defence = 1;
    beast.speed = 4;
    beast.realm = rules::Realm::Mortal;
    beast.ally = false;
    // 兜底这一场也有架势与破绽：玩家在这里学不会破势的话，它就比正经战斗少一半玩法。
    beast.maxToughness = 2;
    beast.weaknesses = core::kCategoryFist | core::kCategorySword;
    units.push_back(std::move(beast));

    battle_.setup(std::move(units), 20260920u);
    battle_.setCanEscape(true);
    // 兜底遭遇也让玩家用得上背包里的药与自己学过的法术：这一场是脚本写错战斗 id
    // 时玩家真会打到的那一场，不该比正经战斗少一半操作。
    registerFieldMagics(app);
    registerBagItems(app);
}

// ---------------------------------------------------------------------------
// 发起动作
// ---------------------------------------------------------------------------

core::Result<std::string> BattleScene::issuePlayerAction(Application& app, const Action& action) {
    // 能不能做，问的是与菜单同一个函数——判据分成两套的那一刻，「面板说能点、
    // 点下去被回绝」就会出现。
    if (std::string why = refusePlayerAction(app.state(), battle_, action); !why.empty()) {
        return core::Result<std::string>::failure(std::move(why));
    }
    const auto& units = battle_.units();
    const bool byOurSide = validUnit(battle_, action.actorIndex) &&
                           units[static_cast<std::size_t>(action.actorIndex)].ally;
    // 只有我方掏的是玩家的背包。
    const bool spendsFromBag = action.kind == ActionKind::Item && byOurSide;

    core::Result<std::string> result = battle_.apply(action);
    // 规则层的战报与回绝都是修仙说法（「回复 5 点法力」），与菜单同一道换词。
    result.value = stageWords(wordingStage(app.state()), std::move(result.value));
    result.error = stageWords(wordingStage(app.state()), std::move(result.error));
    if (!result.ok) return result;
    if (spendsFromBag) {
        // 刚问过数量，这一步不会失败；removeItem 是 [[nodiscard]] 的，显式丢弃。
        static_cast<void>(app.state().removeItem(action.magicId, 1));
    }
    lastLog_ = result.value;
    boost_ = 0;
    // 横版里每一手都是这一位的整个回合：出了手就轮到下一位。
    battle_.endTurn();
    return result;
}

// ---------------------------------------------------------------------------
// 动作菜单
// ---------------------------------------------------------------------------

std::vector<ui::ListItem> BattleScene::buildRootItems(const core::GameData& data,
                                                      const core::GameState& state,
                                                      const BattleState& battle, int actorIndex) {
    std::vector<ui::ListItem> items;
    items.reserve(kBattleMenuCount);

    // 攻击 / 法术 / 物品三项只是**导航**：点不点得动只看下一级列表空不空。
    // 某条法术为什么放不出来、某件东西为什么用不了，一律写在下一级的条目上。
    Action attack;
    attack.kind = ActionKind::Attack;
    attack.actorIndex = actorIndex;
    std::string weapons;
    if (validUnit(battle, actorIndex) && !battle.devourMode()) {
        weapons = core::categoryNames(battle.units()[static_cast<std::size_t>(actorIndex)].weapons, " / ");
    }
    items.push_back(row("攻击", weapons, refuseOnAnyone(state, battle, attack, /*wantAlly=*/false)));

    std::vector<std::string> ids;
    static_cast<void>(buildMagicItems(data, state, battle, actorIndex, ids));
    items.push_back(row(magicWord(wordingStage(state)), {},
                        ids.empty() ? emptyMagicReason(wordingStage(state), battle, actorIndex) : std::string{}));

    static_cast<void>(buildItemItems(data, state, battle, actorIndex, ids));
    items.push_back(row("物品", {}, ids.empty() ? emptyItemReason(data, state) : std::string{}));

    // 防御与逃跑没有下一级，直接拿真动作去问，理由原样转述 core 那一句。
    Action defend;
    defend.kind = ActionKind::Defend;
    defend.actorIndex = actorIndex;
    items.push_back(row("防御", "受伤减半 · 下回合先手", refusePlayerAction(state, battle, defend)));

    Action escape;
    escape.kind = ActionKind::Escape;
    escape.actorIndex = actorIndex;
    items.push_back(row("逃跑", {}, refusePlayerAction(state, battle, escape)));
    return items;
}

std::vector<ui::ListItem> BattleScene::buildWeaponItems(const BattleState& battle, int actorIndex,
                                                        std::vector<int>& categories) {
    categories.clear();
    std::vector<ui::ListItem> items;
    if (validUnit(battle, actorIndex)) {
        for (const int bit : core::categoryBits(battle.units()[static_cast<std::size_t>(actorIndex)].weapons)) {
            categories.push_back(bit);
            items.push_back(row(core::categoryName(bit), {}, {}));
        }
    }
    items.push_back(backRow());
    return items;
}

std::string BattleScene::emptyMagicReason(PanelStage stage, const BattleState& battle, int actorIndex) {
    if (!validUnit(battle, actorIndex)) return "此刻无人可以施法";
    const Unit& actor = battle.units()[static_cast<std::size_t>(actorIndex)];
    if (actor.magicsExhaustive && actor.magics.empty()) return actor.name + " 还没学过任何" + magicWord(stage);
    return actor.name + " 会的" + magicWord(stage) + "在这一场施展不出来";
}

std::string BattleScene::emptyItemReason(const core::GameData& data, const core::GameState& state) {
    // 背包里确实有战斗中用得上的东西，却一件都没登记上 —— 那是这一场的例外（识海）。
    for (const core::BagEntry& entry : state.bag) {
        if (entry.count <= 0) continue;
        const core::Item* item = data.findItem(entry.itemId);
        if (item != nullptr && core::battleUsable(*item)) return "这一场用不上随身之物";
    }
    return "身上没有战斗中用得上的东西";
}

std::vector<ui::ListItem> BattleScene::buildMagicItems(const core::GameData& data,
                                                       const core::GameState& state,
                                                       const BattleState& battle, int actorIndex,
                                                       std::vector<std::string>& magicIds) {
    magicIds.clear();
    std::vector<ui::ListItem> items;
    for (const std::string& id : knownMagicIds(battle, actorIndex)) {
        const core::Magic* magic = data.findMagic(id);
        if (magic == nullptr) continue;   // 内容问题，门禁拦在合入前
        Action probe;
        probe.kind = ActionKind::Cast;
        probe.actorIndex = actorIndex;
        probe.magicId = id;
        magicIds.push_back(id);
        std::string detail = std::string("耗") + mpWord(wordingStage(state)) + " " + std::to_string(magic->needMp);
        if (const std::string what = spellEffectLabel(*magic); !what.empty()) detail += " · " + what;
        // 看破（天眼术）与削架势不吃蓄劲，不写「蓄劲加威」骗人。
        if (magic->effect == core::MagicEffect::None) {
            detail += magic->boost == core::MagicBoost::Hits ? " · 蓄劲连发" : " · 蓄劲加威";
        }
        items.push_back(row(magic->name, detail, refuseOnAnyone(state, battle, probe, /*wantAlly=*/false)));
    }
    if (magicIds.empty()) items.push_back(row("（无）", {}, emptyMagicReason(wordingStage(state), battle, actorIndex)));
    items.push_back(backRow());
    return items;
}

std::vector<ui::ListItem> BattleScene::buildItemItems(const core::GameData& data,
                                                      const core::GameState& state,
                                                      const BattleState& battle, int actorIndex,
                                                      std::vector<std::string>& itemIds) {
    itemIds.clear();
    std::vector<ui::ListItem> items;
    for (const std::string& id : battle.registeredItemIds()) {
        const core::Item* item = data.findItem(id);
        if (item == nullptr) continue;
        Action probe;
        probe.kind = ActionKind::Item;
        probe.actorIndex = actorIndex;
        probe.magicId = id;   // 契约的 Action 没有 itemId，物品借用这个槽位
        itemIds.push_back(id);
        std::string detail = "余 " + std::to_string(state.itemCount(id));
        // 符箓行尾写它施展的那门法术打什么（「火」「削架势 3」），不写法力：用符不耗法力
        //（契约 docs/interfaces-p3-ch07.md 2.5）。
        if (const core::Magic* spell = item->castMagic.empty() ? nullptr : data.findMagic(item->castMagic)) {
            if (const std::string what = spellEffectLabel(*spell); !what.empty()) detail += " · " + what;
        }
        // 毒药往对面使、丹药给自己人用，按**数据**分流（与 checkItem 同一条口径）；
        // 带 castMagic 的符箓照那门法术挑目标，打的是敌人。
        const bool wantAlly = item->castMagic.empty() && item->poison <= 0;
        items.push_back(row(item->name, detail, refuseOnAnyone(state, battle, probe, wantAlly)));
    }
    if (itemIds.empty()) items.push_back(row("（无）", {}, emptyItemReason(data, state)));
    items.push_back(backRow());
    return items;
}

std::vector<ui::ListItem> BattleScene::buildTargetItems(const core::GameState& state,
                                                        const BattleState& battle, const Action& shape,
                                                        std::vector<int>& targetIndices) {
    targetIndices.clear();
    std::vector<ui::ListItem> items;
    if (!validUnit(battle, shape.actorIndex)) {
        items.push_back(backRow());
        return items;
    }
    const std::vector<Unit>& units = battle.units();
    const Unit& actor = units[static_cast<std::size_t>(shape.actorIndex)];
    // 带 castMagic 的符箓按那门法术挑目标（契约 docs/interfaces-p3-ch07.md 2.5）：同施法一样只列敌人。
    const core::Item* item = shape.kind == ActionKind::Item ? battle.findItem(shape.magicId) : nullptr;
    const bool enemiesOnly = shape.kind != ActionKind::Item || (item != nullptr && !item->castMagic.empty());
    for (std::size_t i = 0; i < units.size(); ++i) {
        const Unit& unit = units[i];
        if (!unit.alive()) continue;
        // 攻击与施法只列敌人；物品两边都列——往哪一边使是数据决定的，
        // 让玩家看见「毒药不能用在自己人身上」比从列表里删掉说得清楚。
        if (enemiesOnly && unit.ally == actor.ally) continue;
        Action probe = shape;
        probe.targetIndex = static_cast<int>(i);
        targetIndices.push_back(static_cast<int>(i));
        const std::string detail =
            unit.ally ? "气血 " + std::to_string(std::max(0, unit.hp)) + "/" + std::to_string(unit.maxHp)
                      : foeRowDetail(unit);
        items.push_back(row(unit.name, detail, refusePlayerAction(state, battle, probe)));
    }
    if (targetIndices.empty()) items.push_back(row("（无）", {}, "场上没有可选的目标"));
    items.push_back(backRow());
    return items;
}

void BattleScene::enterRoot(Application& app) {
    menu_ = BattleMenuMode::Root;
    shape_ = Action{};
    menuIds_.clear();
    menuCategories_.clear();
    menuTargets_.clear();
    menuList_.reset();
    menuList_.setPageSize(listPageRows(app.theme()));
    menuList_.setItems(buildRootItems(app.data(), app.state(), battle_, battle_.currentActor()));
}

void BattleScene::enterWeapon(Application& app) {
    menu_ = BattleMenuMode::Weapon;
    menuList_.reset();
    menuList_.setPageSize(listPageRows(app.theme()));
    menuList_.setItems(buildWeaponItems(battle_, battle_.currentActor(), menuCategories_));
}

void BattleScene::enterMagic(Application& app) {
    menu_ = BattleMenuMode::Magic;
    menuList_.reset();
    menuList_.setPageSize(listPageRows(app.theme()));
    menuList_.setItems(buildMagicItems(app.data(), app.state(), battle_, battle_.currentActor(), menuIds_));
}

void BattleScene::enterItem(Application& app) {
    menu_ = BattleMenuMode::Item;
    menuList_.reset();
    menuList_.setPageSize(listPageRows(app.theme()));
    menuList_.setItems(buildItemItems(app.data(), app.state(), battle_, battle_.currentActor(), menuIds_));
}

void BattleScene::enterTarget(Application& app) {
    menu_ = BattleMenuMode::Target;
    // 行动者一律在这一刻取，不缓存：缓存下来就有「菜单开着、回合已经过去了」的可能。
    shape_.actorIndex = battle_.currentActor();
    shape_.targetIndex = -1;
    // 蓄劲只跟攻击与法术走（物品、防御、逃跑蓄不了）。
    shape_.boost = shape_.kind == ActionKind::Attack || shape_.kind == ActionKind::Cast ? boost_ : 0;
    menuList_.reset();
    menuList_.setPageSize(listPageRows(app.theme()));
    menuList_.setItems(buildTargetItems(app.state(), battle_, shape_, menuTargets_));
}

void BattleScene::openMenu(Application& app) {
    if (battle_.phase() != BattlePhase::Ongoing) return;
    const int actor = battle_.currentActor();
    if (!validUnit(battle_, actor)) return;
    if (!battle_.units()[static_cast<std::size_t>(actor)].ally) return;
    setBoost(boost_);   // 换了人：夹回这一位的劲以内
    enterRoot(app);
}

void BattleScene::closeMenu() {
    menu_ = BattleMenuMode::Closed;
    menuList_.reset();
    menuList_.setItems({});
    menuIds_.clear();
    menuCategories_.clear();
    menuTargets_.clear();
    shape_ = Action{};
}

void BattleScene::menuBack(Application& app) {
    // 不加 default:，好让新增的一级菜单漏了处理时由编译器（/W4 C4061）喊出来。
    switch (menu_) {
        case BattleMenuMode::Closed: return;
        case BattleMenuMode::Root:   closeMenu(); return;
        case BattleMenuMode::Weapon:
        case BattleMenuMode::Magic:
        case BattleMenuMode::Item:   enterRoot(app); return;
        case BattleMenuMode::Target:
            if (shape_.kind == ActionKind::Cast) {
                enterMagic(app);
            } else if (shape_.kind == ActionKind::Item) {
                enterItem(app);
            } else if (menuCategories_.size() > 1) {
                enterWeapon(app);
            } else {
                enterRoot(app);
            }
            return;
    }
}

bool BattleScene::issueFromMenu(Application& app, const Action& action) {
    // **菜单唯一的出口。** 这里只调 issuePlayerAction：另起一条直接喂 BattleState::apply
    // 的路，扣背包这件事就会漏在新路径上（技术债 G-5）。
    core::Result<std::string> result = issuePlayerAction(app, action);
    if (!result.ok) {
        // 菜单只让点得动的项被点中，走到这里说明局面在菜单开着的时候变了。
        feedback_ = result.error;
        enterRoot(app);
        return false;
    }
    feedback_ = result.value;
    closeMenu();
    return true;
}

bool BattleScene::menuChoose(Application& app, int index) {
    if (menu_ == BattleMenuMode::Closed) return false;
    if (index < 0 || index >= menuList_.count()) return false;
    // 禁用项点下去确实什么也不发生 —— 但**不是没有反馈**：理由就写在那一行上。
    if (!menuList_.items()[static_cast<std::size_t>(index)].enabled) return false;

    switch (menu_) {
        case BattleMenuMode::Closed:
            return false;
        case BattleMenuMode::Root:
            switch (index) {
                case kBattleMenuAttack: {
                    shape_ = Action{};
                    shape_.kind = ActionKind::Attack;
                    // 兵刃不止一样才多一级；一样就直接出那一样（识海里一样也没有，出 0）。
                    std::vector<int> categories;
                    static_cast<void>(buildWeaponItems(battle_, battle_.currentActor(), categories));
                    if (categories.size() > 1 && !battle_.devourMode()) {
                        enterWeapon(app);
                        return true;
                    }
                    menuCategories_ = categories;
                    shape_.category =
                        categories.size() == 1 && !battle_.devourMode() ? categories.front() : 0;
                    enterTarget(app);
                    return true;
                }
                case kBattleMenuCast:
                    shape_ = Action{};
                    shape_.kind = ActionKind::Cast;
                    enterMagic(app);
                    return true;
                case kBattleMenuItem:
                    shape_ = Action{};
                    shape_.kind = ActionKind::Item;
                    enterItem(app);
                    return true;
                case kBattleMenuDefend:
                case kBattleMenuEscape: {
                    Action action;
                    action.kind = index == kBattleMenuDefend ? ActionKind::Defend : ActionKind::Escape;
                    action.actorIndex = battle_.currentActor();
                    return issueFromMenu(app, action);
                }
                default:
                    return false;
            }
        case BattleMenuMode::Weapon:
            if (static_cast<std::size_t>(index) >= menuCategories_.size()) {
                enterRoot(app);
                return true;
            }
            shape_.category = menuCategories_[static_cast<std::size_t>(index)];
            enterTarget(app);
            return true;
        case BattleMenuMode::Magic:
        case BattleMenuMode::Item:
            // 末项是「返回」，落在 menuIds_ 之外。
            if (static_cast<std::size_t>(index) >= menuIds_.size()) {
                enterRoot(app);
                return true;
            }
            shape_.kind = menu_ == BattleMenuMode::Magic ? ActionKind::Cast : ActionKind::Item;
            shape_.magicId = menuIds_[static_cast<std::size_t>(index)];
            {
                // 看破照的是整个场子，不挑目标：选中就施展（契约 docs/interfaces-p3-ch06.md 1.5）；
                // 装着看破的符箓同理（契约 docs/interfaces-p3-ch07.md 2.5）。削架势要挑一个敌人，照常择敌。
                // 蓄劲按 0 发——它不吃劲，与防御同一个口径。
                const core::Item* item =
                    menu_ == BattleMenuMode::Item ? app.data().findItem(shape_.magicId) : nullptr;
                const std::string spellId = menu_ == BattleMenuMode::Magic ? shape_.magicId
                                            : item != nullptr               ? item->castMagic
                                                                            : std::string{};
                const core::Magic* magic = app.data().findMagic(spellId);
                if (magic != nullptr && magic->effect == core::MagicEffect::Reveal) {
                    shape_.actorIndex = battle_.currentActor();
                    shape_.targetIndex = -1;
                    shape_.boost = 0;
                    return issueFromMenu(app, shape_);
                }
            }
            enterTarget(app);
            return true;
        case BattleMenuMode::Target:
            if (static_cast<std::size_t>(index) >= menuTargets_.size()) {
                menuBack(app);
                return true;
            }
            shape_.actorIndex = battle_.currentActor();
            shape_.targetIndex = menuTargets_[static_cast<std::size_t>(index)];
            shape_.boost = shape_.kind == ActionKind::Attack || shape_.kind == ActionKind::Cast ? boost_ : 0;
            return issueFromMenu(app, shape_);
    }
    return false;
}

int BattleScene::maxBoost() const {
    const int actor = battle_.currentActor();
    if (!validUnit(battle_, actor) || battle_.devourMode()) return 0;
    const Unit& unit = battle_.units()[static_cast<std::size_t>(actor)];
    return unit.ally ? std::min(core::battle::kBoostMax, unit.bp) : 0;
}

void BattleScene::setBoost(int boost) { boost_ = std::clamp(boost, 0, maxBoost()); }

void BattleScene::updateMenu(Application& app) {
    engine::Engine& eng = app.engine();
    using Key = engine::Engine::Key;
    // 左右键是蓄劲，不是翻页（ListView::update 会把它们当翻页，所以这里不走它）。
    // 加一档响一声 boost_N：气焰与劲珠同一刻变，耳朵也跟上。
    const int before = boost_;
    if (eng.keyPressed(Key::Left)) setBoost(boost_ - 1);
    if (eng.keyPressed(Key::Right)) setBoost(boost_ + 1);
    if (boost_ > before) eng.playSfx("boost_" + std::to_string(boost_));
    if (boost_ < before) eng.playSfx("ui_cursor");
    const int row = menuList_.selection();
    if (eng.keyPressed(Key::Up)) menuList_.moveUp();
    if (eng.keyPressed(Key::Down)) menuList_.moveDown();
    if (menuList_.selection() != row) eng.playSfx("ui_cursor");
    // 取消优先于确认：同一帧两个键都下来时，退一步比误出一手安全。
    // Tab 与 Esc / X 同义（施工图 1.5）：世界层按 Tab 开菜单，进了战斗手还放在 Tab 上。
    if (eng.keyPressed(Key::Cancel) || eng.keyPressed(Key::Menu)) {
        eng.playSfx("ui_cancel");
        menuBack(app);
        return;
    }
    if (eng.keyPressed(Key::Confirm) && menuList_.confirm()) {
        eng.playSfx(menuChoose(app, menuList_.selection()) ? "ui_confirm" : "ui_error");
    }
}

// ---------------------------------------------------------------------------
// 回合推进
// ---------------------------------------------------------------------------

void BattleScene::stepAi() {
    const int actor = battle_.currentActor();
    if (actor < 0) {
        battle_.endTurn();
        return;
    }
    const Action action = battle_.decideAi(actor);
    if (action.actorIndex >= 0) {
        auto result = battle_.apply(action);
        if (result.ok) lastLog_ = result.value;
    }
    // AI 一次只做一个动作；行动完就把回合推给下一位。
    battle_.endTurn();
}

bool BattleScene::runToCompletion(int maxRounds) {
    // maxRounds 数的是「从此刻起再打几回合」，不是回合号：测试里逐回合调 runToCompletion(1)
    // 盯着局面，按回合号数的话第二次调用就一步也不走了。每一步不是出一手就是推进行动序，
    // 一回合的位数有限，所以这个循环必然走到下一回合，不需要另设步数上限。
    const int lastRound = battle_.round() + maxRounds;
    while (battle_.phase() == BattlePhase::Ongoing && battle_.round() < lastRound) {
        // 我方也交给 decideAi（我方那一套：探破绽、打破绽、破势时蓄满劲；不吃药）。
        stepAi();
    }
    return battle_.phase() == BattlePhase::Won;
}

int BattleScene::runToAllyTurn(int maxSteps) {
    for (int guard = 0; guard < maxSteps; ++guard) {
        if (battle_.phase() != BattlePhase::Ongoing) return -1;
        const int actor = battle_.currentActor();
        if (actor < 0) {
            battle_.endTurn();
            continue;
        }
        if (battle_.units()[static_cast<std::size_t>(actor)].ally) return actor;
        stepAi();
    }
    return -1;
}

void BattleScene::settle(Application& app) {
    if (settled_) return;
    settled_ = true;

    const auto& units = battle_.units();
    // 识海之战不写回气血：那一场打的是元神，肉身正躺在床上。
    // 韩立不在场的仗一个字节也不写：他在客栈睡觉，他的同伴也没上场。
    if (!battle_.devourMode() && heroPresent_) {
        if (!units.empty() && units[0].ally) app.state().hp = std::max(1, units[0].hp);
        // 同伴的气血也带出战场，否则队伍里那一位永远是满血的。
        for (core::PartyMember& member : app.state().party) {
            for (const Unit& unit : units) {
                if (unit.id != member.roleId || !unit.ally) continue;
                member.hp = std::max(1, unit.hp);
                break;
            }
        }
        // 这一场揭开的破绽记进存档：下一场再遇上同一种敌人，那几格一开场就是亮的
        //（GameState::knownWeaknesses）。输了、逃了也记——看见了就是看见了。
        for (const Unit& unit : units) {
            if (!unit.ally) app.state().learnWeaknesses(unit.id, unit.revealed);
        }
    }

    // 战后发放（契约 docs/interfaces-p2.md 第 6 节）。**排在回填之前**：脚本 resume
    // 之后的第一句就可能去查背包。查不到编成（兜底遭遇）就什么也不发。
    // 也排在战果卡之前：卡上那几行说的就是这里真发下去的东西（技术债 G-15），不另算一遍。
    if (const core::BattleSetup* setup = app.battleSetup(battleId_)) {
        granted_ = grantBattleReward(app.state(), setup->reward, battle_.phase());
    }
}

void BattleScene::finish(Application& app) {
    if (finished_) return;
    settle(app);
    finished_ = true;

    script::CommandResult result;
    result.ok = true;
    result.battleWon = battle_.phase() == BattlePhase::Won;
    // 战果与结局的机器码。**不要加 default:**。
    switch (battle_.phase()) {
        case BattlePhase::Won:       result.code = ""; break;
        case BattlePhase::Lost:      result.code = "lost"; break;
        case BattlePhase::Escaped:   result.code = "escaped"; break;
        case BattlePhase::EnemyFled: result.code = "enemy_fled"; break;
        case BattlePhase::Ongoing:
            // 打到一半就收场只可能是驱动方出错。如实报一个机器码，别伪装成一场败仗。
            result.code = "unfinished";
            break;
    }
    // 咬下了多少（0-100）。脚本据此写旗标 ch03.shihai_yaoxia。非吞噬战恒为 0。
    result.value = battle_.devourSpoilsPercent();
    app.completeCommand(result);

    // 回到世界层的曲子（脚本点播的那一首优先，没点播就是地图的；Engine::playBgm 同一首不重头放）；
    // 世界层本来就不放曲子，就停。
    if (view_) {
        const std::string bgm = app.worldBgm();
        if (!bgm.empty() && bgm != "none") {
            app.engine().playBgm(bgm);
        } else {
            app.engine().stopBgm();
        }
    }
}

// ---------------------------------------------------------------------------
// 逐条播放
// ---------------------------------------------------------------------------

void BattleScene::pullEvents() {
    const std::vector<BattleEvent>& events = battle_.events();
    while (eventCursor_ < events.size()) playback_.push_back(events[eventCursor_++]);
}

void BattleScene::syncShown() {
    const std::vector<Unit>& units = battle_.units();
    shown_.resize(units.size());
    for (std::size_t i = 0; i < units.size(); ++i) {
        shown_[i] = ShownUnit{units[i].hp, units[i].toughness, units[i].revealed, units[i].broken(),
                              units[i].alive()};
    }
}

double BattleScene::playEvent(Application& app, const BattleEvent& e) {
    const std::vector<Unit>& units = battle_.units();
    BattleView& view = *view_;
    const auto at = [this](int index) -> ShownUnit* {
        return index >= 0 && static_cast<std::size_t>(index) < shown_.size()
                   ? &shown_[static_cast<std::size_t>(index)]
                   : nullptr;
    };
    const auto nameOf = [&units](int index) -> std::string {
        return index >= 0 && static_cast<std::size_t>(index) < units.size()
                   ? units[static_cast<std::size_t>(index)].name
                   : std::string{};
    };
    lastPlayed_ = e.kind;
    // 不加 default:：新的事件种类漏了演法，由 /W4 喊出来。
    switch (e.kind) {
        case BattleEventKind::RoundStart:
            // 上一手的尾巴（回合在它之后翻篇）：出手的人先收势，光环不再挂在他身上。
            view.endAction();
            stageActor_ = -1;
            lastLog_ = "—— 第 " + std::to_string(e.value) + " 回合 ——";
            return 0.2;
        case BattleEventKind::Act:
            lastLog_ = stageWords(wordingStage(app.state()), e.text);
            stageActor_ = e.actor;
            view.onAct(e);
            // 施法多给一拍：剑指、光起，再落下去。
            return static_cast<core::battle::ActionKind>(e.value) == core::battle::ActionKind::Cast ? 0.4 : 0.3;
        case BattleEventKind::BoostSpent:
            stageActor_ = e.actor;
            view.say(e.actor, app.text("ui.battle.boost") + " ×" + std::to_string(e.value), app.theme().goldBright);
            view.onBoost(e.actor, e.value);
            return 0.3;
        case BattleEventKind::Hit: {
            ShownUnit* s = at(e.target);
            const bool wasBroken = s != nullptr && s->broken;
            const int before = s != nullptr ? s->toughness : e.toughness;
            if (s != nullptr) {
                s->hp = e.hp;
                s->toughness = e.toughness;
            }
            // 揭开的破绽同 id 的一起亮（规则层也是这么记的：知道的是「恶狼怕什么」）。
            if (e.revealed != 0 && validUnit(battle_, e.target)) {
                const std::string& id = units[static_cast<std::size_t>(e.target)].id;
                for (std::size_t i = 0; i < units.size() && i < shown_.size(); ++i) {
                    if (units[i].id == id) shown_[i].revealed |= e.revealed & units[i].weaknesses;
                }
            }
            view.onHit(e, wasBroken, before);
            return 0.32;
        }
        case BattleEventKind::Break:
            if (ShownUnit* s = at(e.target)) {
                s->broken = true;
                s->toughness = 0;
            }
            view.onBreak(e.target);
            lastLog_ = nameOf(e.target) + " " + app.text("ui.battle.break") + "！";
            return 0.8;
        case BattleEventKind::ChargeInterrupt:
            view.say(e.target, app.text("ui.battle.charge_interrupt"), app.theme().goldBright);
            banner_.clear();
            return 0.4;
        case BattleEventKind::Recover:
            if (ShownUnit* s = at(e.target)) {
                s->broken = false;
                s->toughness = e.toughness;
            }
            view.say(e.target, app.text("ui.battle.recover"), app.theme().goldBright);
            view.onRecover(e.target);
            return 0.45;
        case BattleEventKind::Poisoned:
            view.say(e.target, "中毒", app.theme().poison);
            view.onPoisoned(e.target);
            return 0.25;
        case BattleEventKind::PoisonTick:
            if (ShownUnit* s = at(e.target)) s->hp = e.hp;
            view.onPoisonTick(e.target, e.value);
            return 0.3;
        case BattleEventKind::Cured:
            view.say(e.target, "毒解", app.theme().floatJade);
            return 0.2;
        case BattleEventKind::Heal:
            if (ShownUnit* s = at(e.target)) s->hp = e.hp;
            view.onHeal(e.target, e.value, e.value2);
            return 0.35;
        case BattleEventKind::Guard:
            view.say(e.actor, "守势", app.theme().floatGuard);
            return 0.25;
        case BattleEventKind::GuardEnd:
            return 0.05;
        case BattleEventKind::Fall:
            if (ShownUnit* s = at(e.target)) s->visible = false;
            view.onFall(e.target);
            return 0.55;
        case BattleEventKind::Fled:
            if (ShownUnit* s = at(e.target)) s->visible = false;
            view.say(e.target, "遁走", app.theme().floatDim);
            view.onFled(e.target);
            return 0.5;
        case BattleEventKind::Escape:
            view.say(e.actor, e.value != 0 ? "脱身" : "脱身不得", app.theme().floatDim);
            return 0.35;
        case BattleEventKind::WaveIn:
            // 新进场的一波：显示的那一份按规则层对齐一下在不在场（他们刚走上战场）。
            for (std::size_t i = 0; i < units.size() && i < shown_.size(); ++i) {
                if (units[i].wave == e.value && units[i].onField) {
                    shown_[i] = ShownUnit{units[i].hp, units[i].toughness, units[i].revealed, false, true};
                }
            }
            view.onWaveIn(e.value);
            lastLog_ = "第 " + std::to_string(e.value + 1) + " 波杀到";
            return 0.6;
        case BattleEventKind::ChargeDeclare:
            // 预告句是叙述，本身就以角色名起头（「欧阳飞天沉肩坐马……」），不再前缀「名：」——
            // 前缀出来就是「欧阳飞天：欧阳飞天沉肩坐马」。
            banner_ = app.text(e.text);
            bannerAge_ = 0.0;
            stageActor_ = e.actor;
            view.say(e.actor, app.text("ui.battle.charge"), app.theme().floatCinnabar);
            view.onChargeDeclare(e.actor);
            return 0.9;
        case BattleEventKind::End:
            return 0.5;
        case BattleEventKind::Reveal:
            // 看破：这个敌人（与同 id 的）破绽一格格全亮，用的是「打中揭开」的同一套画法。
            if (validUnit(battle_, e.target)) {
                const std::string& id = units[static_cast<std::size_t>(e.target)].id;
                for (std::size_t i = 0; i < units.size() && i < shown_.size(); ++i) {
                    if (units[i].id == id && units[i].ally == units[static_cast<std::size_t>(e.target)].ally) {
                        shown_[i].revealed |= e.value & units[i].weaknesses;
                    }
                }
            }
            view.onReveal(e.target, e.value);
            return 0.25;
        case BattleEventKind::Stagger: {
            // 削架势（契约 docs/interfaces-p3-ch07.md 2.5）：架势格被削，复用「打中破绽架势减少」的
            // 那套画法；削到 0 的破势由紧跟着的 Break 那一条演。
            ShownUnit* s = at(e.target);
            const int before = s != nullptr ? s->toughness : e.toughness + e.value;
            if (s != nullptr) s->toughness = e.toughness;
            view.onStagger(e.target, before);
            return 0.35;
        }
    }
    return 0.1;
}

bool BattleScene::update(Application& app, double deltaSeconds) {
    if (finished_) return false;

    // 无头模式下没有人按键也没有人看，直接把整场跑完，免得 bot 空转。
    if (app.headless()) {
        if (battle_.phase() == BattlePhase::Ongoing) runToCompletion();
        finish(app);
        return false;
    }
    if (frozen_) return true;

    engine::Engine& eng = app.engine();
    using Key = engine::Engine::Key;
    const double speed = eng.keyDown(Key::Skip) ? kFastForward : 1.0;
    const double real = deltaSeconds * speed;
    // 慢镜只拉长「戏里的时间」：节拍、刀光、碎片都按它走。
    const double step = real * static_cast<double>(view_->timeScale());
    view_->update(static_cast<float>(step), static_cast<float>(real));
    bannerAge_ += step;
    if (shotHold_ >= 0.0) {
        shotHold_ -= real;
        if (shotHold_ <= 0.0) frozen_ = true;
    }
    // 开战碎屏还没碎完：不开菜单、敌方不出手。
    if (view_->introBusy()) return true;

    // ---- 战果卡：等确认键 ----
    if (card_) {
        const float settle = resultCardSettleTime(*card_);
        const bool landing = card_->age < settle;
        card_->age += static_cast<float>(real);
        // 收获里有东西（掉落或碎银）：最后一行落定的那一刻响一声得物（终审 M-3）。
        // 胜利曲在开卡时已经响过，两声错开；一场仗不论掉几样都只响这一声。
        if (landing && card_->age >= settle && (!granted_.drops.empty() || granted_.money > 0)) {
            eng.playSfx("item_get");
        }
        if (eng.keyPressed(Key::Confirm) && card_->age >= resultCardSettleTime(*card_)) {
            eng.playSfx("ui_confirm");
            finish(app);
            return false;
        }
        return true;
    }

    // ---- 先把刚才发生的事一条一条演完 ----
    pullEvents();
    if (beat_ > 0.0) {
        beat_ -= step;
        if (beat_ > 0.0) return true;
    }
    if (!playback_.empty()) {
        beat_ = playEvent(app, playback_.front());
        playback_.pop_front();
        return true;
    }
    beat_ = 0.0;
    if (stageActor_ >= 0) {
        // 这一手演完了：冲出去的人退回原位，光环回到下一位身上。
        view_->endAction();
        stageActor_ = -1;
    }
    syncShown();

    if (battle_.phase() != BattlePhase::Ongoing) {
        settle(app);
        buildResultCard(app);
        return true;
    }

    const int actor = battle_.currentActor();
    if (actor < 0) {
        battle_.endTurn();
        return true;
    }
    const bool playerTurn = battle_.units()[static_cast<std::size_t>(actor)].ally;
    if (!playerTurn) {
        // 回合已经转到敌方，菜单不该还开着：上面那几条禁用理由算的是上一位的局面。
        if (menu_ != BattleMenuMode::Closed) closeMenu();
        aiDelay_ += step;
        if (aiDelay_ >= kAiThinkSeconds) {
            aiDelay_ = 0.0;
            stepAi();
        }
        return true;
    }

    // 轮到我方：菜单自己打开（横版里没有「先挪一步」，这一位能做的事全在菜单里）。
    // 截图口预演时不读键：那几帧不 pollEvents，键盘状态是进场前那一帧的，读了会替人按下去。
    if (menu_ == BattleMenuMode::Closed) openMenu(app);
    if (shot_ == BattleShot::None) updateMenu(app);
    return true;
}

// ---------------------------------------------------------------------------
// 战果卡
// ---------------------------------------------------------------------------

void BattleScene::buildResultCard(Application& app) {
    card_ = std::make_unique<ResultCard>();
    card_->phase = battle_.phase();
    card_->hint = app.text("ui.ending.continue_hint");
    // 不加 default:：新的战果漏了收场的话，由 /W4 喊出来。
    switch (battle_.phase()) {
        case BattlePhase::Won:
            card_->title = app.text("ui.battle.victory");
            card_->lines = rewardLines(granted_, app.data(), app.state(), app.text("ui.battle.result.cultivation"));
            card_->subtitle = app.text(card_->lines.empty() ? "ui.battle.result.nothing" : "ui.battle.victory.sub");
            app.engine().playSfx("victory");
            break;
        case BattlePhase::Lost:
            card_->title = app.text("ui.battle.result.lost");
            card_->subtitle = app.text("ui.battle.result.lost.sub");
            app.engine().playSfx("defeat");
            break;
        case BattlePhase::Escaped:
            card_->title = app.text("ui.battle.result.escaped");
            card_->subtitle = app.text("ui.battle.result.escaped.sub");
            break;
        case BattlePhase::EnemyFled:
            card_->title = app.text("ui.battle.result.enemy_fled");
            if (battle_.devourMode()) {
                card_->subtitle = app.text("ui.battle.result.devour") + " " +
                                  std::to_string(battle_.devourSpoilsPercent()) + "%";
            }
            break;
        case BattlePhase::Ongoing:
            break;
    }
}

// ---------------------------------------------------------------------------
// 截图口
// ---------------------------------------------------------------------------

bool parseBattleShot(const std::string& spec, std::string& battleId, BattleShot& shot) {
    const std::size_t colon = spec.find(':');
    battleId = spec.substr(0, colon);
    if (battleId.empty()) return false;
    if (colon == std::string::npos) {
        shot = BattleShot::Opening;
        return true;
    }
    const std::string moment = spec.substr(colon + 1);
    if (moment == "intro") shot = BattleShot::Intro;
    else if (moment == "mid") shot = BattleShot::Mid;
    else if (moment == "charge") shot = BattleShot::Charge;
    else if (moment == "victory") shot = BattleShot::Victory;
    else if (moment == "target") shot = BattleShot::Target;
    else return false;
    return true;
}

bool BattleScene::rewindToAction(BattleEventKind kind, BattleEventKind fallback) {
    std::vector<ShownUnit> fallbackShown;
    std::size_t fallbackCursor = 0;
    bool haveFallback = false;
    for (int guard = 0; guard < 4096 && battle_.phase() == BattlePhase::Ongoing; ++guard) {
        syncShown();
        const std::size_t before = battle_.events().size();
        if (battle_.currentActor() < 0) {
            battle_.endTurn();
            continue;
        }
        stepAi();
        const std::vector<BattleEvent>& events = battle_.events();
        const auto has = [&events, before](BattleEventKind k) {
            return std::any_of(events.begin() + static_cast<std::ptrdiff_t>(before), events.end(),
                               [k](const BattleEvent& ev) { return ev.kind == k; });
        };
        if (has(kind)) {
            eventCursor_ = before;
            return true;
        }
        if (has(fallback)) {
            fallbackShown = shown_;
            fallbackCursor = before;
            haveFallback = true;
        }
    }
    if (haveFallback) {
        shown_ = std::move(fallbackShown);
        eventCursor_ = fallbackCursor;
    } else {
        eventCursor_ = battle_.events().size();
        syncShown();
    }
    return false;
}

bool BattleScene::shotReached() const {
    // 不加 default:
    switch (shot_) {
        case BattleShot::None:
        case BattleShot::Intro:
            return false;
        case BattleShot::Opening:
        case BattleShot::Target:
            return menu_ != BattleMenuMode::Closed;
        case BattleShot::Mid:
            return lastPlayed_ == BattleEventKind::Break || (shotFallback_ && lastPlayed_ == BattleEventKind::Hit);
        case BattleShot::Charge:
            return lastPlayed_ == BattleEventKind::ChargeDeclare;
        case BattleShot::Victory:
            return card_ != nullptr;
    }
    return false;
}

void BattleScene::prepareShot(Application& app) {
    double hold = 0.0;
    // 不加 default:
    switch (shot_) {
        case BattleShot::None:
            return;
        case BattleShot::Intro:
            // 碎片定在刚崩开的那一刻；世界画面在第一次 render 时拍（这一帧世界层照画）。
            view_->holdIntroAt(static_cast<float>(kIntroShotAt));
            frozen_ = true;
            return;
        case BattleShot::Opening:
            break;
        case BattleShot::Target:
            hold = 0.3;   // 让气焰与指针走起来
            break;
        case BattleShot::Mid:
            shotFallback_ = !rewindToAction(BattleEventKind::Break, BattleEventKind::Hit);
            hold = kShotSettle;
            break;
        case BattleShot::Charge:
            shotFallback_ = !rewindToAction(BattleEventKind::ChargeDeclare, BattleEventKind::ChargeDeclare);
            hold = 0.7;
            break;
        case BattleShot::Victory:
            runToCompletion();
            eventCursor_ = battle_.events().size();
            syncShown();
            break;
    }
    view_->skipIntro();
    // 截图口只空转三帧：在这里按 1/60 秒一帧把戏推到那一刻，再多走 hold 秒让特效展开，然后定格。
    for (int frame = 0; frame < kShotPrerollFrames && !frozen_; ++frame) {
        static_cast<void>(update(app, 1.0 / 60.0));
        // !frozen_：定格的那一帧 shotHold_ 刚减到负数，不拦的话这一块会再跑一遍（拍择敌时就是再点一下「攻击」）。
        if (!frozen_ && shotHold_ < 0.0 && shotReached()) {
            if (shot_ == BattleShot::Target) {
                // 蓄满劲、点「攻击」（兵刃不止一样时取第一样），停在择敌那一级。
                setBoost(maxBoost());
                static_cast<void>(menuChoose(app, kBattleMenuAttack));
                if (menu_ == BattleMenuMode::Weapon) static_cast<void>(menuChoose(app, 0));
            }
            if (card_) hold = static_cast<double>(resultCardSettleTime(*card_)) + 0.3;
            shotHold_ = hold;
            if (hold <= 0.0) frozen_ = true;
        }
    }
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------

void BattleScene::renderMenu(Application& app) const {
    if (menu_ == BattleMenuMode::Closed) return;
    engine::Engine& eng = app.engine();
    const ui::Theme& theme = app.theme();
    const int actor = battle_.currentActor();
    const float feetY = validUnit(battle_, actor) ? view_->anchors()[static_cast<std::size_t>(actor)].y : 490.f;
    // 竖直方向跟着当前这一位；底边不压日志。
    const int top = std::clamp(static_cast<int>(feetY) - 250, 96,
                               static_cast<int>(kLogRect.y) - kMenuH - 2);
    const engine::Rect panel{kMenuX, top, kMenuW, kMenuH};
    std::string title = menuTitle(menu_, wordingStage(app.state()));
    if (maxBoost() > 0 || boost_ > 0) {
        title += " · " + app.text("ui.battle.boost") + " " + std::to_string(boost_) + "/" + std::to_string(maxBoost());
    }
    ui::drawPanel(eng, panel, title, theme);
    // 右缘一颗金菱指向这一位：菜单是「他的」菜单。
    ui::drawDiamond(eng, static_cast<float>(panel.x + panel.w), std::clamp(feetY - 60.f, static_cast<float>(top) + 20.f,
                                                                           static_cast<float>(top + kMenuH) - 20.f),
                    6.f, theme.goldBright);
    const engine::Rect content = ui::panelContentArea(panel, title, theme);
    const int listH = std::max(0, content.h - kMenuFeedbackH);
    menuList_.render(eng, engine::Rect{content.x, content.y, content.w, listH}, theme);
    // 择敌时说明区写指着的那个敌人的整句（行里只放得下破绽，见 foeRowDetail）；其余时候写上一句反馈。
    std::string note = feedback_;
    if (const int pick = menuList_.selection(); menu_ == BattleMenuMode::Target && pick >= 0 &&
                                                static_cast<std::size_t>(pick) < menuTargets_.size()) {
        const Unit& target = battle_.units()[static_cast<std::size_t>(menuTargets_[static_cast<std::size_t>(pick)])];
        if (!target.ally) note = foeStatusText(target);
    }
    if (!note.empty()) {
        ui::drawTextBlock(eng, note, engine::Rect{content.x, content.y + listH, content.w, kMenuFeedbackH}, 16, theme);
    }
}

void BattleScene::render(Application& app) {
    if (!view_) return;
    engine::Engine& eng = app.engine();
    // 开战那一帧：世界层刚画完（这一帧本场景不算不透明），先拍下来，再往上画战斗。
    if (view_->introPending()) view_->captureIntro(app);

    const bool choosing = menu_ != BattleMenuMode::Closed;
    BattleView::Frame frame;
    frame.shown = &shown_;
    frame.actor = (playing() || card_ || stageActor_ >= 0) ? stageActor_ : battle_.currentActor();
    frame.choosing = choosing;
    frame.boost = boost_;
    frame.celebrate = card_ != nullptr && card_->phase == BattlePhase::Won;
    view_->renderStage(app, frame);

    if (!card_) {
        int target = -1;
        if (menu_ == BattleMenuMode::Target && menuList_.selection() >= 0 &&
            static_cast<std::size_t>(menuList_.selection()) < menuTargets_.size()) {
            target = menuTargets_[static_cast<std::size_t>(menuList_.selection())];
        }
        const HudFrame hud{battle_, shown_, *view_, app.theme(), icons_, frame.actor, boost_, target, choosing};
        drawOrderStrip(eng, hud, app.text("ui.battle.order.now"), app.text("ui.battle.order.next"));
        drawRoundInfo(eng, hud, "第 " + std::to_string(battle_.round()) + " 回合", waveStatusText(battle_));
        drawEnemyPlates(eng, hud, app.text("ui.battle.charge"));
        drawPartyPanel(eng, hud);
        drawChargeBanner(eng, app.theme(), banner_, static_cast<float>(bannerAge_), static_cast<float>(kBannerLife));
        drawBattleLog(eng, app.theme(), lastLog_, app.text(choosing ? "ui.battle.hint.menu" : "ui.battle.hint.watch"));
        renderMenu(app);
        drawTargetPointer(eng, hud);
    }
    view_->renderOverlay(app);
    if (card_) drawResultCard(eng, app.theme(), icons_, *card_);
    view_->renderIntro(app);
}

int BattleScene::listPageRows(const ui::Theme& theme) {
    // 与 renderMenu() 的 listH 同一份高度：面板高 - 标题带 - 反馈带。
    return ui::listRowsThatFit(kMenuH - ui::panelTitleBandHeight(true, theme) - kMenuFeedbackH, theme);
}

}  // namespace fanren::game
