#pragma once
// 横版回合制战斗界面（「破势与蓄劲」，docs/octopath-battle.md 第 6 节）。
// 规则全在 core::battle::BattleState 里，本场景只负责输入、按事件流逐条播放、
// 把结果回填给脚本协程；画面交给 BattleView（台：背景、人、特效）与 BattleHud（墨金 HUD）。
// 无头模式下那两样一概不建：runToCompletion 与既有测试走的路一个字节也不变。
#include <cstddef>
#include <deque>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/battle/Battle.h"
#include "core/model/Types.h"
#include "game/BattleFx.h"
#include "game/Scene.h"
#include "game/Wording.h"
#include "ui/Widgets.h"

namespace fanren::game {

class BattleView;
struct ResultCard;

// 截图口（--scene battle:<编成 id>[:<时刻>]）要拍的那一刻。None 是正常开战（碎屏转场、等玩家）；
// 其余几种都在 onEnter 里自己把戏推到那一刻、然后定格——截图口只空转三帧，等不到任何动画。
enum class BattleShot {
    None,
    Opening,   // 开局：第一次轮到我方、菜单打开的那一刻（不带碎屏）
    Intro,     // 开战碎屏的中途
    Mid,       // 打到中途头一次破势的那一刻（没有破势就是最后一刀）
    Charge,    // 首领宣告蓄势、横幅亮出的那一刻
    Victory,   // 战果卡（收获全部飞到位）
    Target,    // 开局那一手：蓄满劲、点「攻击」，停在择敌（指针、气焰、劲珠一起看）
};

// 「b03_gu_wai_elang」→ Opening；「b03_gu_wai_elang:mid」→ Mid。认不出的时刻返回 false。
[[nodiscard]] bool parseBattleShot(const std::string& spec, std::string& battleId, BattleShot& shot);

// 识海之战在战斗数据里的地形名。吞噬模式就挂在它上面，不新开数据 schema。
inline constexpr const char* kMindTerrain = "mind";

// 中毒在界面上的那半句话；没中毒时返回空串。单独拎成自由函数是为了能被测试直接调用：
// 中毒**必须在界面上看得出来**（契约第 2.2 节）。
[[nodiscard]] std::string poisonStatusText(const core::battle::Unit& unit);

// 波次在界面上的那一行；单波战斗返回空串。进入下一波必须在界面上说得出来
//（契约第 2.3 节）：日志上那一行「第 N 波杀到」是「刚刚发生了什么」，
// 这一行常驻的「第 N 波 / 共 M 波」是「现在打到哪了」。
[[nodiscard]] std::string waveStatusText(const core::battle::BattleState& battle);

// 一个敌人脚下那一排破绽格：已揭开的写类别字，没揭开的写「？」，按位序排。
// 与 poisonStatusText 同一个理由单独拎出来：「破绽看得见、揭开了会变」是这套玩法
// 的一半，只活在 render() 里就只能靠截图验。没有架势的（我方、识海）返回空表。
[[nodiscard]] std::vector<std::string> weaknessSlots(const core::battle::Unit& unit);

// 破势在界面上那一句：「破势 · 余 2 回合」；没破势返回空串。
[[nodiscard]] std::string breakStatusText(const core::battle::BattleState& battle,
                                          const core::battle::Unit& unit);

// 择敌时菜单下方说明区那一句：「气血 12/18 · 架势 2 · 破绽 剑？？」（没有架势的只写气血）。
// 择敌那一行只放得下破绽（终审 LOW-1），整句在这里；与 poisonStatusText 同一个理由拎成自由函数。
[[nodiscard]] std::string foeStatusText(const core::battle::Unit& unit);

// 战斗背景名：编成写了 backdrop 就用它，没写按地形推（画面路按这个名字找图）。
[[nodiscard]] std::string backdropOf(const core::BattleSetup& setup);

// 这一场在此刻所在地图上开打，画哪张背景：编成写的 → data/visual/battles.json 的指派
//（Application::battleBackdrop）→ 识海 → 所在地图 meta → 地形兜底（resolveBackdrop 的次序）。
// 开战画面与测试问的是这同一个函数（终审 HIGH-1：从前那一档在游戏里恒为空，测试只测了加载器）。
[[nodiscard]] std::string battleBackdropFor(Application& app, const std::string& battleId);

// 韩立手上的兵刃：空手（拳）永远算一样，背包里每件兵器再加它那一样
//（docs/octopath-battle.md 2.2；与 scripts/ch05/heishui.lua「有软剑就抽剑，没有就换一掌」
// 同一个口径）。他不是 role 模板驱动的，所以这条规则写在这里，不写在 data/roles。
[[nodiscard]] int heroWeapons(const core::GameData& data, const core::GameState& state);

// ---- 战后发放（第 4 章复验 N-3，契约 docs/interfaces-p2.md 第 6 节）----
//
// 一场仗打完，存档里真正多出来的东西。发放函数的返回值就是存档的增量——
// 测试拿两者对账，日后界面要说一句「收获」也从这里取，不再另算一遍。
struct GrantedReward {
    int cultivation = 0;
    int money = 0;                        // 钱：material_lingshi，界面一律走 currencyName
    std::vector<core::BagEntry> drops;

    [[nodiscard]] bool empty() const {
        return cultivation == 0 && money == 0 && drops.empty();
    }
};

// 这个战果发不发奖励。**只有 Won 发**；败、逃、敌逃、没打完一律不发（契约 6.1）。
[[nodiscard]] bool battleRewardEarned(core::battle::BattlePhase phase);

// 按战果把 reward 发进存档，返回实际发下的东西。战果不是 Won 时什么也不动。
// **只加修为，不动境界**；钱按商店的记法记（material_lingshi）；掉落的 rate 现阶段不读。
GrantedReward grantBattleReward(core::GameState& state, const core::BattleReward& reward,
                                core::battle::BattlePhase phase);

// ---- 动作菜单 ----
//
// 层级：根菜单 →（攻击：兵刃不止一样时先选兵刃 / 法术 / 物品）选一条 → 选目标 → 发起。
// Weapon 追加在末尾：switch (menu) 不加 default，漏处理由 /W4 喊出来。
enum class BattleMenuMode { Closed, Root, Magic, Item, Target, Weapon };

// 根菜单五项，顺序固定，下标写死在 menuChoose 里。「这一项现在不能用」一律靠置灰
// 表达，不靠删行，否则删一条要改好几处。
inline constexpr int kBattleMenuAttack = 0;
inline constexpr int kBattleMenuCast = 1;
inline constexpr int kBattleMenuItem = 2;
inline constexpr int kBattleMenuDefend = 3;
inline constexpr int kBattleMenuEscape = 4;
inline constexpr int kBattleMenuCount = 5;

// 玩家发起这个动作会不会被回绝，以及为什么；返回空串表示做得了。
//
// **菜单画禁用理由与 issuePlayerAction 真的发起，问的都是这一个函数。**
// 两道闸的顺序也与 issuePlayerAction 一致：game 层的背包闸在前（战场的物品登记表是
// 开战那一刻的快照，用光最后一颗之后它还留在表里），core 的 checkLegal 在后。
[[nodiscard]] std::string refusePlayerAction(const core::GameState& state,
                                             const core::battle::BattleState& battle,
                                             const core::battle::Action& action);

class BattleScene : public Scene {
public:
    explicit BattleScene(std::string battleId, BattleShot shot = BattleShot::None);
    ~BattleScene() override;
    BattleScene(const BattleScene&) = delete;
    BattleScene& operator=(const BattleScene&) = delete;

    void onEnter(Application& app) override;
    bool update(Application& app, double deltaSeconds) override;
    void render(Application& app) override;
    // 开战那一帧还没拍下世界画面：这一帧不算不透明，让世界层照画，好拍下来碎给玩家看。
    [[nodiscard]] bool opaque() const override;

    // 动作菜单的页大小：菜单面板让出标题带与反馈带之后画得满几行。
    // 必须按区域算而不是按条目数算，理由见 tests/ListLayoutTests.cpp 的战斗菜单那一条。
    [[nodiscard]] static int listPageRows(const ui::Theme& theme);

    [[nodiscard]] std::string name() const override { return "Battle"; }

    // 供无头 bot 驱动：两边都交给 BattleState::decideAi，跑到分出胜负，返回是否我方获胜。
    // 不经过键盘，也不依赖帧率。maxRounds 是「从此刻起最多再打几回合」，到了就停在
    // 那一回合的开头（胜负未分时返回 false）。
    bool runToCompletion(int maxRounds = 200);

    // 把回合推进到某个我方单位可以行动为止，中途的敌方由同一套 AI 走完；
    // 返回那个我方单位的下标，战斗先结束了则返回 -1。玩家的那一步必须落在
    // **真实的行动序**上，所以测试一律经它走到自己的回合。
    int runToAllyTurn(int maxSteps = 256);

    // 玩家（或替玩家操作的驱动方）发起一个动作。**所有会动到存档的动作都必须走这一条**：
    // core 只认战斗里的效果，不持有玩家的物品栏。成功时就此结束这一位的回合（endTurn）——
    // 横版里没有「挪一步再出手」，每一手都是这一位的整个回合。
    core::Result<std::string> issuePlayerAction(Application& app,
                                                const core::battle::Action& action);

    // **只读**：拿得到可改的 BattleState，就等于多了一条绕开 issuePlayerAction 的路。
    [[nodiscard]] const core::battle::BattleState& battle() const { return battle_; }

    // 这一场收场时真的发下去的东西；还没收场、或者这一场什么也不发时为空。
    [[nodiscard]] const GrantedReward& grantedReward() const { return granted_; }

    // ---- 动作菜单：纯逻辑，公开供无头测试直接驱动 ----

    [[nodiscard]] BattleMenuMode menuMode() const { return menu_; }
    // 当前这一级菜单的条目，含禁用理由。渲染与测试读的是同一份。
    [[nodiscard]] const ui::ListView& menuList() const { return menuList_; }
    // 面板上那句话：刚发生了什么，或者刚才为什么没成。
    [[nodiscard]] const std::string& feedback() const { return feedback_; }

    // 打开根菜单。轮不到我方就什么也不做。
    void openMenu(Application& app);
    void closeMenu();
    // 选中当前这一级的第 index 项并确认。返回 false 表示这一项点不动（越界或禁用）。
    bool menuChoose(Application& app, int index);
    // 退回上一级；已在根菜单则关掉菜单。
    void menuBack(Application& app);

    // ---- 蓄劲（左右键）----
    // 当前要蓄几点：夹在 [0, min(3, 行动者现有的劲)]。只作用在攻击与法术上，
    // 物品、防御、逃跑发起时一律按 0 发。发起过一手之后归零。
    [[nodiscard]] int boost() const { return boost_; }
    void setBoost(int boost);
    void boostUp() { setBoost(boost_ + 1); }
    void boostDown() { setBoost(boost_ - 1); }
    // 行动者此刻最多能蓄几点（识海里恒为 0）。
    [[nodiscard]] int maxBoost() const;

    // ---- 逐条播放事件 ----
    // 还有没播完的事件：这期间不收输入、敌方不出手，画面一条一条把刚才发生的事演出来。
    [[nodiscard]] bool playing() const { return !playback_.empty() || eventCursor_ < battle_.events().size(); }

    // ---- 各级列表的内容 ----
    // 一律静态：测试可以就地搭一个局面问它，不必先凑出一整场真战斗。每一条的可否与
    // 理由都来自 refusePlayerAction，这里一个判据都不自己写。

    [[nodiscard]] static std::vector<ui::ListItem> buildRootItems(
        const core::GameData& data, const core::GameState& state,
        const core::battle::BattleState& battle, int actorIndex);

    // 兵刃列表（攻击那一项的下一级；只有一样兵刃时不经过这一级）。categories 与行同序。
    // 末项固定是「返回」。
    [[nodiscard]] static std::vector<ui::ListItem> buildWeaponItems(
        const core::battle::BattleState& battle, int actorIndex, std::vector<int>& categories);

    // 法术列表 = 行动者自己那份清单 ∩ 本场登记表。ids 与列表同序。末项固定是「返回」。
    [[nodiscard]] static std::vector<ui::ListItem> buildMagicItems(
        const core::GameData& data, const core::GameState& state,
        const core::battle::BattleState& battle, int actorIndex,
        std::vector<std::string>& magicIds);

    // 物品列表 = 本场登记表（开战那一刻从背包拍下的快照）。用光的那件照样列出，
    // 只是写明「背包里已经没有了」。
    [[nodiscard]] static std::vector<ui::ListItem> buildItemItems(
        const core::GameData& data, const core::GameState& state,
        const core::battle::BattleState& battle, int actorIndex,
        std::vector<std::string>& itemIds);

    // 目标列表。shape 带着 kind、magicId（物品复用该字段）、category 与 boost，
    // target all spells (including castMagic items) offer one group row anchored to the first live foe.
    // 本函数只填目标。攻击与施法只列敌人；物品两边都列。每个敌人的条目上写着
    // 气血、架势与已知的破绽——挑谁打，看的就是这一行。
    [[nodiscard]] static std::vector<ui::ListItem> buildTargetItems(
        const core::GameState& state, const core::battle::BattleState& battle,
        const core::battle::Action& shape, std::vector<int>& targetIndices);

    // 施法列表为空时那句话：「他还没学过」与「这一场施展不出来」要靠完全不同的方式去解决。
    // 那一栏叫什么按阶段取（Wording.h 的 magicWord：凡人阶段「法门」）。
    [[nodiscard]] static std::string emptyMagicReason(PanelStage stage, const core::battle::BattleState& battle,
                                                      int actorIndex);
    // 物品列表为空时那句话。识海那一场（一件都不登记）与「身上确实没有」是两回事。
    [[nodiscard]] static std::string emptyItemReason(const core::GameData& data,
                                                     const core::GameState& state);

private:
    void buildEncounter(Application& app);
    // 编成开了 hero_absent 时：不建韩立、不调 addPartyUnits、不调 registerBagItems。
    void buildFromSetup(Application& app, const core::BattleSetup& setup);
    // 把 GameState::party 里 active 的成员放上场（契约第 1.4 节）。同一个角色不得同时
    // 出现两次，编成里已写死的 ally 单位优先。
    void addPartyUnits(Application& app, std::vector<core::battle::Unit>& units) const;
    // 本场能用的法术 = 场上各单位在 data/roles 里声明的那些，取并集。
    void registerFieldMagics(Application& app);
    // 本场能用的物品 = 玩家背包里战斗中真的有效果的那些（core::battleUsable）。
    void registerBagItems(Application& app);
    // 未知战斗 id 时的兜底遭遇。
    void buildProbeEncounter(Application& app);
    void stepAi();
    // 收场分两步：settle 把气血、破绽写回存档并发下奖励（胜负一分就做，战果卡要拿它的结果说话）；
    // finish 回填脚本协程。无头时两步连着走；有画面时中间隔着一张等确认键的战果卡。
    void settle(Application& app);
    void finish(Application& app);

    // ---- 菜单内部 ----
    void enterRoot(Application& app);
    void enterWeapon(Application& app);
    void enterMagic(Application& app);
    void enterItem(Application& app);
    void enterTarget(Application& app);
    // 唯一的发起处：菜单的每一条路最终都收在这里，而这里只调 issuePlayerAction。
    bool issueFromMenu(Application& app, const core::battle::Action& action);
    // 键盘驱动菜单的那一支（含左右键蓄劲）。
    void updateMenu(Application& app);

    // ---- 逐条播放 ----
    // 把规则层新产生的事件接进播放队列。
    void pullEvents();
    // 播一条：更新画面上显示的那一份局面、飘字。返回这一条要停留多久（秒）。
    double playEvent(Application& app, const core::battle::BattleEvent& event);
    // 画面上显示的那一份局面与规则层对齐（队列播空之后）。
    void syncShown();

    // ---- 截图口 ----
    // 用 AI 把仗往前推，停在「这一手里有 kind 这种事件」的那一手之前：显示的局面对齐到那一手之前，
    // 事件游标指回那一手的开头，好让它在画面上重演一遍。找不到返回 false（局面照样推过去了）。
    // 一次都没有 kind 时退而求其次，停在最后一手有 fallback 的那一手之前。
    bool rewindToAction(core::battle::BattleEventKind kind, core::battle::BattleEventKind fallback);
    // 把戏推到 shot_ 要拍的那一刻，然后定格。
    void prepareShot(Application& app);
    [[nodiscard]] bool shotReached() const;

    // ---- 绘制 ----
    void renderMenu(Application& app) const;
    void buildResultCard(Application& app);

    core::battle::BattleState battle_;
    std::string battleId_;
    std::string lastLog_;
    bool settled_ = false;
    bool finished_ = false;
    GrantedReward granted_;
    // 这一场建没建韩立（契约 docs/interfaces-p3-ch05.md 第 2 节）。编成开了 hero_absent
    // 就是假：finish() 据此一个字节也不往存档里写（他在睡觉，0 号不是他）。
    bool heroPresent_ = true;

    // 菜单状态。几级共用一个 ListView：战斗菜单每一级都是选完就走。
    BattleMenuMode menu_ = BattleMenuMode::Closed;
    ui::ListView menuList_;
    // 正在拼的那个动作：kind / magicId / category 在上几级填上，targetIndex 在选目标那一级
    // 填上。actorIndex 一律在发起时取 currentActor()，不缓存。
    core::battle::Action shape_;
    std::vector<std::string> menuIds_;    // 与法术/物品列表同序
    std::vector<int> menuCategories_;     // 与兵刃列表同序
    std::vector<int> menuTargets_;        // 与目标列表同序
    std::string feedback_;
    int boost_ = 0;

    // ---- 逐条播放 ----
    // 画面上显示的那一份：气血、架势、破势、已揭开的破绽、在不在场。事件播到哪一条，
    // 它就更新到哪一条，于是血条在那一刀落下的时候才掉——规则层早已算完，画面按拍子追。
    std::vector<ShownUnit> shown_;
    std::deque<core::battle::BattleEvent> playback_;
    std::size_t eventCursor_ = 0;
    double beat_ = 0.0;          // 当前这一条还要停多久
    double aiDelay_ = 0.0;       // 敌方出手前的一点停顿
    std::string banner_;         // 首领蓄势的预告句（播到那一条时亮出来）
    double bannerAge_ = 0.0;
    // 台上正在演的是谁的那一手：规则层出完手就 endTurn 了，currentActor() 早已是下一位，
    // 光环得跟着正在演的人走。播空之后回到 currentActor()。
    int stageActor_ = -1;
    std::optional<core::battle::BattleEventKind> lastPlayed_;   // 截图口认「那一刻」用

    // ---- 画面（无头时一概为空）----
    std::unique_ptr<BattleView> view_;
    engine::TextureId icons_ = engine::kInvalidTexture;
    std::unique_ptr<ResultCard> card_;   // 胜负已分、等确认键收场的那张卡
    BattleShot shot_ = BattleShot::None;
    bool frozen_ = false;                // 截图口：到了要拍的那一刻，钟停住
    double shotHold_ = -1.0;             // 到了那一刻之后再走多久才定格（让特效展开）
    bool shotFallback_ = false;          // 要拍的那种事一次都没发生：退而求其次拍最后一刀
};

}  // namespace fanren::game
