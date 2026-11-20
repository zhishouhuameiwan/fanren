#pragma once
// 主循环与场景栈，兼作脚本命令的派发中枢。
//
// 脚本在 Lua 侧 yield 出命令对象，这里每帧取出、交给对应场景执行，
// 场景完成后把结果回填并 resume 协程。script 层因此不必知道 engine / game 的存在
// （方案 3.2 的依赖方向）。
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "core/model/Exploration.h"
#include "core/model/Lilian.h"
#include "core/model/Types.h"
#include "core/rules/Crafting.h"
#include "core/rules/Economy.h"
#include "core/rules/Encounter.h"
#include "core/rules/Objectives.h"
#include "core/rules/PathActions.h"
#include "core/rules/Tutorials.h"
#include "engine/Engine.h"
#include "game/ChapterCardScene.h"
#include "game/DialogueExtras.h"
#include "game/ItemToasts.h"
#include "game/LoreNotices.h"
#include "game/Portraits.h"
#include "game/WorldHud.h"
#include "game/Scene.h"
#include "io/SettingsFile.h"
#include "script/Command.h"
#include "script/ScriptHost.h"
#include "ui/Widgets.h"

namespace fanren::game {

// 新开一局站在哪张图上。标题画面的「新的旅程」、无头冒烟与不带 --map 的截图口都从这里开，
// 一处定义，免得三处各写一个字面量。
inline constexpr const char* kNewGameMap = "ch01_hanjiacun";

// 掌天瓶每多少天凝一滴绿液：炼气 7 天、筑基 5 天、结丹 3 天。
//
// 写成具名函数而不是在 advanceDays 里就地 switch，是因为这条映射将来还会被
// 洞府灵脉、剧情道具改写，而「瓶子多久凝一滴」是玩家能直接感知的数值——
// 它必须只有一个定义处，也必须能不开窗口直接单测。
[[nodiscard]] int bottleChargeDays(rules::Realm realm) noexcept;

// 灵田缺省槽位数。地图上的 facility 用 slots 属性覆盖它。
inline constexpr int kDefaultFieldSlots = 4;

// 脚本一次 advance_days 的上限。
//
// 不是平衡数值而是防手误：脚本里多打一个零，灵田结算就要空转几十万次，
// 而现象是「过场卡住不动」，查起来会一路查到渲染层去。夹在十年上，
// 需要更长跨度的剧情写两条 advance_days 即可，代价只是一行脚本。
inline constexpr int kMaxScriptAdvanceDays = 3650;

// 脚本一次 field_unlock 能开出的最大槽位数。同上，防的是写错的数量级——
// 槽位是 vector，一个笔误就是一次几十万元素的分配。
inline constexpr int kMaxFieldSlots = 64;

// 静养：每过去一天回上限的**百分之几**。10 = 一成，十天回满。
// 取值的论证在 Application.cpp 的 restFor() 上头。
//
// 名字与用法从前是对不上的：它叫 PercentPerDay，却被当作**除数**用
//（`cap / kRestPercentPerDay`），于是 10 碰巧等于一成，改成 20 却是「一天半成」——
// 下一个照名字去调它的人会调反。第 4 章复验（N-4）点过这一条，现在算式按百分数写，
// 名实相符：改成 20 就是一天两成。
//
// 设计口径（「一天回上限一成」，docs/handoff.md 第 2 节 MEDIUM-3 一行、
// docs/ch04-reverify.md 第七节判据 4）由 tests/RestTests.cpp 以字面量钉住，
// 不从这个常量推——从被测物推出来的判据，改了口径它跟着改，等于没有判据。
inline constexpr int kRestPercentPerDay = 10;

// 读 mapsRoot 下全部 .tmj 的门（Application::portalLinks 的实现，拎出来是为了能不开
// 窗口、不建整个 Application 就测它的降级路径）。
//
// **不抛异常。** 目录读不了（不存在、扫到一半出错）→ 返回空表并在 problems 里留一句：
// 跨图指路整个停用，不拿半张图去算。某一张图读不进来 → 只跳过那一张，同样留一句。
[[nodiscard]] std::vector<rules::PortalLink> loadPortalLinks(const std::string& mapsRoot,
                                                             std::vector<std::string>& problems);

// 危险度（docs/interfaces-octo-encounters.md 第 6 节）。一场遭遇的「凶」= 敌方每个单位
// 气血 × 攻击 之和（全部波次）：前者是要砍多久，后者是这段时间里挨多重，两样相乘才是
// 「这一仗要付出多少」。星数按对数走——每多一颗，凶一倍：
//   凶 < 200 → 1 颗；200–399 → 2；400–799 → 3；800–1599 → 4；≥ 1600 → 5。
// 取的是按 realm 遇得上的那几条里最凶的一场（「最坏会撞上什么」）；一条也遇不上返回 0。
// 星数的上限是横幅画得下的那几颗（hud::PlaceBanner::kMaxStars），只有那一处定义。
inline constexpr int kDangerUnit = 100;
[[nodiscard]] int battleMenace(const core::BattleSetup& battle, const core::GameData& data);
[[nodiscard]] int menaceStars(int menace);
[[nodiscard]] int encounterDangerStars(const rules::EncounterTable& table, rules::Realm realm,
                                       const std::map<std::string, core::BattleSetup>& battles,
                                       const core::GameData& data);

// 贴底的面板（对话框、路径行动面板）该摆在哪（终审 LOW-6）：压不着主角就留在底边；
// 压着了（镜头贴着地图下沿、人站在屏幕下半）就整体上移到主角那一圈的上沿之上，最高到离上沿 topGap。
// 只挪不缩：面板的尺寸与内容一点不变，玩家看到的只是它往上让了一截。
[[nodiscard]] engine::Rect keepHeroInSight(const engine::Rect& bottomPanel, const engine::Rect& hero, int topGap);

// 开战参数（伙伴改版，docs/interfaces-companions.md 3.4 与第 6 节）。开一仗带着它走，战斗场景开场
// 从 Application::battleStart() 读。缺省值就是改版前的开法：脚本 battle()、遭遇、切磋都不带先手。
struct BattleStart {
    std::string battleId;
    // 偷袭明雷得手：我方全员开场多 1 点劲，第一回合我方全员排在所有敌人之前（规则归战斗路实现）。
    bool preemptive = false;
};

// 进一趟历练（docs/interfaces-companions.md 3.12）。组队面板定好人之后交给 Application::enterLilian。
struct LilianEntry {
    std::string siteId;                // 历练地 id（data/lilian/<site>.json）
    std::string mapId;                 // 入口图
    std::string spawn;                 // 入口图上的出生点；空 = 缺省出生点
    std::vector<std::string> party;    // 这一趟的队员：currentRole 给的角色 id，至多 3 位
};

class Application {
public:
    Application();
    ~Application();

    // headless 为真时不开窗口，供无头 bot 与 CI 使用。
    core::Result<bool> init(const std::string& assetRoot, bool headless);
    int run();
    void shutdown();

    // ---- 场景栈 ----
    void pushScene(ScenePtr scene);
    void popScene();
    void replaceScene(ScenePtr scene);
    [[nodiscard]] Scene* topScene();

    // ---- 子系统访问 ----
    [[nodiscard]] engine::Engine& engine() { return *engine_; }
    [[nodiscard]] script::ScriptHost& scripts() { return *scripts_; }
    [[nodiscard]] core::GameData& data() { return data_; }
    [[nodiscard]] core::GameState& state() { return state_; }
    [[nodiscard]] const core::TileMap* currentMap() const { return map_.get(); }

    // 战斗编成。查不到返回 nullptr，由调用方决定是回退还是报错——
    // 这里不替它做主，因为剧情战打不开和遭遇战打不开该有不同处理。
    [[nodiscard]] const core::BattleSetup* battleSetup(const std::string& id) const;

    // 这一场在 data/visual/battles.json 里指派的背景（docs/art-maps.md 第 8 节）；没指派 = 空串，
    // 由 BattleScene 退回所在地图 meta 的背景。表与 data/、maps/ 同一个资产根，init 时读一次。
    [[nodiscard]] std::string battleBackdrop(const std::string& battleId) const;

    // 店铺。查不到返回 nullptr（脚本或地图写错了 id），由调用方决定怎么说。
    //
    // 返回**可改**的指针是有意的：库存与补货计时都记在 rules::ShopEntry 上，
    // 卖光一件就得真的少一件，否则「限量珍品」这件事根本不存在。这份状态
    // 目前只活在本局进程里（存档不含商店，见 io/SaveFile.cpp）——读档后货架
    // 回到 data 里的初始量，这是眼下的已知取舍，不是漏写。
    [[nodiscard]] rules::Shop* shop(const std::string& id);

    // 全部配方（data/recipes/**，四艺混在一张表里，按 id 排）。
    //
    // 返回**只读**引用：配方是 data，运行期不变。与商店那份可改的库存正好相反，
    // 理由也正好相反——库存要真的少一件，方子不会因为炼过一炉就变。
    [[nodiscard]] const std::map<std::string, rules::Recipe>& recipes() const { return recipes_; }

    // 场景显示文案走它。提示里的键名跟着键位走（docs/settings.md 第 6 节）：文案里写 {key.<动作 id>}，
    // 这里展开成那个动作眼下第一个自定义键（没有自定义键就取第一个固定键）的显示名。认不出的占位原样留着
    // （画面上一眼看得出）。默认键位下展开结果与改造前写死的字面逐字节相同。
    // 提示也跟着设备走（docs/gamepad.md 第 6 节）：最后一下按的是手柄、且文案表里有 key + ".pad"，就取那一条；
    // {pad.<动作 id>} 展开成那个动作第一个手柄键的显示名。键盘模式下不取 .pad，展开结果与从前逐字节相同。
    // **不是全部文案都经它**：告示板、路径行动面板、章节卡、设置面板与改键面板的固有字表直接调
    // GameData::lookupText。所以占位只许出现在经 text() 显示的那几条上（KeyPrompts 用例扫 data/text/** 钉着白名单）。
    [[nodiscard]] std::string text(const std::string& key) const;

    // 无头模式下场景应跳过纯观赏性的动画（逐字显示等），
    // 否则 bot 要空跑几十帧等文字滚完。engine 不暴露此标志，由这里持有。
    [[nodiscard]] bool headless() const { return headless_; }

    // 全局唯一的一份配色与间距。各处各自默认构造一份等于没有集中点，
    // 换皮时又要一处处改回去。
    [[nodiscard]] const ui::Theme& theme() const { return theme_; }

    // 世界层被对白、菜单压着时补一步用的时钟步长（秒）。0（缺省）= 按引擎的真实帧间隔走；截图口设成定数，
    // 不然同一屏拍两次灯的明灭不一样（game/ShotScenes.h 的 kShotWorldClockSeconds）。
    void setFixedWorldClock(double seconds) { fixedWorldClock_ = seconds; }
    [[nodiscard]] double fixedWorldClock() const { return fixedWorldClock_; }

    // 对白头像（伙伴改版 3.9）：表与图都从 init 收的游戏根取（game/Portraits.h 的 PortraitBook）。
    [[nodiscard]] PortraitBook& portraits() { return portraits_; }

    // 角色 id 到显示名。查不到就回显 id，画面上一眼看出谁没进 data。
    [[nodiscard]] std::string speakerName(const std::string& roleId) const;

    // 主角此刻在屏幕上占的地方（逻辑像素），连同四周一格——跟他说话的 NPC 就站在相邻那一格上。
    // 只有世界层在底下露着时才有；战斗、标题画面上返回空矩形，那时对话框没有谁要躲。
    // 镜头取它要收住的那一点（cameraTarget）：面板打开时人已站定，镜头也已收住。
    [[nodiscard]] engine::Rect heroScreenRect() const;

    // 发了东西（剧情给物、路径行动给物）：记一笔，帧末响一声 item_get。
    // 同一帧里连发几样只响一声——一次发放响一次，不是每件都响（终审 M-3）。
    void noteItemsGiven() { itemsGiven_ = true; }

    // ---- 得物卡片与集齐奖励（docs/ch07-playtest-ui.md）----
    //
    // 拿到一样东西：推一张得物卡片（game/ItemToasts.h：不挡路、不进场景栈，几秒后自己淡出），并记一笔
    // noteItemsGiven（帧末响一声 item_get）。开箱、采集、脚本 give()、路径行动给物、集齐奖励都走这里。
    // 只管「让玩家看见」：东西进不进背包由调用方先办好。
    void announceItem(const std::string& itemId, int count);
    // 卡片的那一份：世界层露着时随 tick 走时钟（被战斗、菜单整个盖住时停着，回来再接着放），
    // 画在世界层之上、对话框之下。世界层按确认（面前没有别的能按的）收起它们（WorldScene::update）。
    [[nodiscard]] ItemToasts& itemToasts() { return itemToasts_; }
    [[nodiscard]] const ItemToasts& itemToasts() const { return itemToasts_; }
    // 这张图「找齐」的奖励（data/exploration/*.json，init 时读，game/Exploration.h 发）；没配为 nullptr。
    [[nodiscard]] const core::ExplorationReward* explorationReward(const std::string& mapId) const;

    // ---- 时间 ----
    // 推进游戏内日历若干天，并结算所有按天走的系统。
    // 打坐、闭关、等待、剧情跳时都走这一个入口——散在各处必然有人漏掉一项，
    // 而漏掉的那项要到很久以后才会被发现。
    // days <= 0 一律无副作用（日历层本身也拒绝倒退时间）。
    void advanceDays(int days);

    // 快速存档。写到 <资产根>/saves/quick.sav，成功与失败都如实返回，
    // 由调用方负责把话说给玩家听——**静默失败在这里是明令禁止的**。
    //
    // 只存 GameState，不存场景栈：schema 里没有场景栈，也不该有。
    // 所以调用方要保证只在世界层（走格子）存，不在对话 / 战斗 / 面板里存。
    // 主菜单的「存盘」也走这一条：菜单只从世界层打开，存下去读回来就是站在原地。
    [[nodiscard]] core::Result<std::string> quickSave();

    // 快速存档的路径。存（quickSave）与读（标题画面的「继续旅程」）问的是同一个地方。
    [[nodiscard]] std::string quickSavePath() const;

    // ---- 系统设置（docs/settings.md）----
    //
    // 与野外遭遇同一个套路：**缺省不碰文件**——settings() 是默认值，saveSettings() 是成功的空操作。
    // 几百条无头测试因此永远拿默认值，也永远不会写仓库里的 saves/settings.json。
    // 启动器（main.cpp）在正常游玩时 enableSettingsFile；截图口只在显式给了 --settings 时才读。
    [[nodiscard]] const io::Settings& settings() const { return settings_; }
    // 存进来并立刻生效：音量、全屏、缩放、垂直同步、特效档位、键位推给引擎；
    // 震屏、文字速度由 BattleView / DialogueScene 读 settings()。音量档位越界先夹到 0–10。
    // 键位：默认表上叠 keys 里列出的动作，整张交给引擎校验（固定键 / 保留键 / 重复 / 某动作一个键都不剩 /
    // 越界）；不过 → 键位整张回默认（settings().keys 清空）+ 标准错误一行警告。
    void setSettings(const io::Settings& settings);
    // 记住路径并读盘（规则见 io/SettingsFile.h）。读成什么就按什么生效；坏文件已按默认生效，
    // 返回失败原因给调用方打警告（不覆盖那个坏文件：直到玩家在面板里改了什么才写）。
    // 读进来但有几项没照文件办（夹紧、认不出的词、键位整张回默认）的，逐条打到标准错误。
    // 成功时 value = 文件在不在（false = 第一次启动，全默认）。
    core::Result<bool> enableSettingsFile(const std::string& path);
    // 写到 enable 过的路径；没 enable 过 = 成功的空操作（value 为 false）。写失败如实返回：
    // error 是一句放得进面板说明行的短话（「写不进 settings.json」），带整条路径的完整原因打到标准错误。
    // shutdown 时若 enable 过、而设置与最后一次落盘的不同（面板开着就关了窗口），会再写一次。
    core::Result<bool> saveSettings();
    // <资产根>/saves/settings.json（与 quick.sav 同目录，但与存档分开：设置属于这台机器）。
    [[nodiscard]] std::string defaultSettingsPath() const;

    // ---- 八方旅人化：主菜单、标题流程、章节卡、淡入淡出（docs/interfaces-octo-ui.md）----

    // 打开主菜单（状态 · 物品 · 法术 · 记事 · 存盘 · 设置 · 返回首页 · 返回）。行走时按 Tab / Esc 调它——
    // 按键的钩子在 WorldScene::update（地图画面那一路的文件），那几行见上面那份契约文档。
    // 只该在世界层、脚本没在跑的时候调：WorldScene 的那道「脚本在跑就不受理输入」已经挡着。
    void openMainMenu();

    // 放弃本局未存盘的进度，清理运行期状态，在帧末回到首页；不写入存档。
    void returnToTitle();

    // 标题画面的「新的旅程」：全新的存档状态，站到 kNewGameMap 的出生点，压世界层，
    // 再压第一章的开篇卡。载图失败如实返回（标题画面把它说出来），不压任何场景。
    core::Result<bool> startNewJourney();

    // 标题画面的「继续旅程」，也是命令行 --load 的同一条路：读档 → 载存档里那张图 →
    // 站回存档那一格 → 压世界层。读档失败如实返回，**不静默退回新开局**（saves/README.md）。
    core::Result<bool> continueJourney(const std::string& savePath);

    // 章节表（data/chapters.json，init 时读）。没有这份文件时为空表：不排任何卡。
    [[nodiscard]] const ChapterTable& chapterTable() const { return chapters_; }

    // 播一串卡。无头模式下不压场景（章节卡不停留），只记进 cardLog——与 spokenKeys 同一个
    // 理由：被测的那条路必须就是上线的那条路，所以记账无条件、只有「画不画」看模式。
    void presentCards(const std::vector<CardRequest>& cards);

    // 至今播过（或无头下「播过」）的卡，按次序。测试用它断言「排了哪几张」，
    // 否则「场景栈上没有卡片」可能只是因为根本没排——那条判据就空转了。
    [[nodiscard]] const std::vector<CardRequest>& cardLog() const { return cardLog_; }

    // 还没播、等着脚本演完的卡。
    [[nodiscard]] const std::vector<CardRequest>& pendingCards() const { return pendingCards_; }

    // 脚本 fade.out / fade.in 的黑幕浓度（0 透明、1 全黑）。由 FadeScene 推，由 drawScenes
    // 画在世界层之上、其余场景之下——黑场里的旁白照样读得见。
    [[nodiscard]] float screenFade() const { return screenFade_; }
    void setScreenFade(float level);

    // 静养若干天：按 kRestPercentPerDay 回气血与法力，同伴一起。
    // 公开而不私有，是为了能单独测它——只能经 advanceDays 间接触发的话，
    // 判据就得绕过日历、灵田与掌天瓶三样才碰得到它。
    void restFor(int days);

    // ---- 地图 ----
    core::Result<bool> loadMap(const std::string& mapId, const std::string& spawnId);

    // 全部地图上的门（rules::PortalLink），供跨图指路用（第 4 章复验 N-7）。
    //
    // 引擎同一时刻只载一张图，而「通往目标的下一道门」要看全部的图才答得出来。
    // **第一次问到时才读盘**，之后缓存：不开世界层的几百条测试一张图也不必多读；
    // 地图在运行期不变，读一次就够。读不进来的那张图记一行日志后跳过——
    // 它照样会在门禁与 loadMap 那里响亮地失败，这里不必替它把整个指路弄瞎。
    [[nodiscard]] const std::vector<rules::PortalLink>& portalLinks();

    // 面朝地图设施按下确认键时的入口。按 kind 打开对应面板；灵田的 ref_id
    // 就是灵田 id。放在 Application 而不是 WorldScene，是为了让世界层不必
    // 认识每一个面板类——设施每多一种，耦合就多一条。
    void openFacility(const core::MapObject& facility);

    // ---- 路径行动（契约 docs/interfaces-octo-pathactions.md 第 5 节）----
    //
    // 世界层经 WorldScene::setPathActionHooks 的两个钩子找到这里（init 里接上）：
    // probe 问 pathActionsFor 决定头顶浮不浮气泡、浮哪一个，opener 就是 openPathActions。
    // 世界层于是不认识路径行动的数据与规则，与 openFacility 不让它认识面板类同一个理由。

    // 当前地图上某个 NPC 此刻挂着的路径行动（rules::pathActionsAt），按 打探 → 求购 → 切磋 排好。
    // 指针指进 data().pathActions。头顶气泡与「按 E 有没有反应」问的都是它。
    [[nodiscard]] std::vector<const core::PathAction*> pathActionsFor(const std::string& npcName) const;

    // 面对这个 NPC 按下 E / Q：压路径行动菜单（PathActionScene）。此刻一条都没挂着就什么也不压，
    // 返回 false——头顶没有气泡的人按了不该有反应。在不在场、脚本在不在跑由世界层那道闸管。
    bool openPathActions(const std::string& npcName);

    // 照单施加 action 这一条的一张效果清单（契约 5.4），说话人是 speakerRole。次序照清单，一条都不跳：
    //   Say → sayAs；GiveItem / TakeItem / SetFlag / GainCultivation → 就地改存档；
    //   RevealWeakness → 写进 GameState::knownWeaknesses，旁白补一句「记下了某某的破绽」；
    //   StartBattle → startBattle 开这一场，收场按战果再施加 challengeResultEffects(action, won)
    //                （逃也算负），说话人照旧。
    // 这一批压的几层（对话、旁白、那一仗）按清单次序演：场景栈后进先出，所以落地前倒过来排——
    // 邀战说完才开打，情报说完才报「记下了破绽」。
    // TakeItem 扣不成就停在那一步、返回 false：调用方没先问 pathActionVerdict 才会这样，
    // 而「记做过」永远排在清单最后，停下来这一条就不算做过。
    // action 必须指进 data().pathActions：收场回调拿着它，那张表开机读一次、运行期不变。
    bool applyPathEffects(const core::PathAction& action, const std::vector<rules::PathEffect>& effects,
                          const std::string& speakerRole);

    // ---- 以回调开战（契约 docs/interfaces-octo-pathactions.md 5.4、docs/interfaces-octo-encounters.md 第 4 节）----
    //
    // 不经脚本开的仗（切磋、野外遭遇）要一条自己的回填路：压 BattleScene(battleId)，并登记 onFinish。
    // BattleScene 收场照旧调 completeCommand（它不必知道自己是谁开的）；completeCommand 认出「此刻
    // 在收场的是一场仗、而且登记着回调」，就改走回调、不去惊动脚本。回调只用一次。
    // 战场上那几件事——气血至少 1 放回、揭开的破绽记账、编成奖励——BattleScene::finish 已经做完，
    // 回调里拿到的战果与脚本 battle() 拿到的是同一张 CommandResult。
    using BattleOutcome = std::function<void(const script::CommandResult&)>;
    void startBattle(const std::string& battleId, BattleOutcome onFinish);
    // 带开战参数的那一条（明雷）。上面那条就是 BattleStart{battleId} 的简写。
    void startBattle(const BattleStart& start, BattleOutcome onFinish);
    // 最近一次开的那一仗的开战参数（脚本 battle() 开的也记）。战斗场景开场读它定先手。
    [[nodiscard]] const BattleStart& battleStart() const { return battleStart_; }

    // ---- 历练（伙伴改版，docs/interfaces-companions.md 3.12）----
    //
    // 进：把此刻的图、位置、朝向、队伍记在内存里，队伍换成 entry.party（满血），传到入口图。
    // 出：队伍、图、位置、朝向恢复成进去之前的，日历加一天。拿到的东西、加的默契照留。
    // 两样都不进存档：历练里不能存档（quickSave 拒绝），读档就是回到进去之前那一份。
    // 已在历练里再进、不在历练里出，都返回失败、什么也不动。
    core::Result<bool> enterLilian(const LilianEntry& entry);
    core::Result<bool> leaveLilian();
    [[nodiscard]] bool inLilian() const;
    // 四种地图对象的状态此刻记在哪一份：历练里是「这一趟」那份（出来就扔），否则是存档里那份。
    // 宝箱、采集点、明雷、机关的逻辑一律经它读写，别直接碰 GameState::worldObjects。
    // **每次现取，不许跨调用持有**：enterLilian、leaveLilian、读档、新开局、返回首页之后，
    // 先前拿到的引用要么已经失效（那一趟扔了），要么指向的是另一份。
    [[nodiscard]] core::WorldObjectState& worldObjects();
    //
    // 进去之前先读这处历练地用到的每张图（只读文件、不动状态），做两件事：
    //   · 带 chance 的 foe / chest 按 rules::lilianSeed(日历日, 历练地) 各抽一次；没抽中的直接记进
    //     「这一趟」——明雷记成已清、宝箱记成已开（看上去是被人先开过的）——不另设状态；
    //   · 按此刻的档核对每个 foe 的编成（foe.battle + 后缀）都在。
    // 图读不进来、缺一场编成，都不进去，什么也不动。历练地没登记（data/lilian 里没有这个 id）时
    // 只读入口图，也不分档。入口图载不进来时换回内存里的原图（不从磁盘重读）。
    //
    // 在历练里开的仗（startBattle）编成 id 拼上档的后缀（_t1 / _t2 / _t3，rules::lilianTier）；
    // 世界逻辑只管把 foe.battle 原样递进来。battleStart().battleId 记的是拼过之后真开的那一场。
    // 拼出来的编成不存在时这一仗不开（不压战斗场景、不调回调），只打一行标准错误。
    //
    // 出口：历练里面朝任何一处 kind = lilian 的设施按确认，就是出去（openFacility → leaveLilian）。

    // 历练地的定义（data/lilian/*.json，init 时读）。查不到返回 nullptr。
    [[nodiscard]] const core::LilianSite* lilianSite(const std::string& siteId) const;
    [[nodiscard]] const std::map<std::string, core::LilianSite>& lilianSites() const { return lilianSites_; }

    // ---- 野外遭遇（docs/interfaces-octo-encounters.md）----
    //
    // **缺省关闭**：几百条无头测试在野地里走来走去，不该冷不丁被拖进一场仗。启动器（main.cpp）
    // 打开它；遭遇自己的测试也打开它。
    void setEncountersEnabled(bool enabled) { encountersEnabled_ = enabled; }
    [[nodiscard]] bool encountersEnabled() const { return encountersEnabled_; }
    // 世界层每迈成一步调一次（WorldScene::tryStep）：站在遭遇区里、区的 require_flag 已置，
    // 就照区上的步数区间与每日上限摇一次 rules::step；摇中了开战，返回 true。
    // 胜：编成奖励由 BattleScene 发；逃：什么也不发；负：**不 game over**——气血已按至少 1
    // 放回，就地不动，旁白说一句（ui.encounter.lost）。
    bool stepEncounters();
    // 地名横幅的危险度：这张图上此刻开着的遭遇区里，按当前境界遇得上的编成里最凶的那一场，
    // 折成 0–hud::PlaceBanner::kMaxStars 颗星（encounterDangerStars）。没有遭遇区、区还没开、或这个境界
    // 一场也遇不上 = 0（横幅不画那一行）。
    [[nodiscard]] int dangerStars() const;
    [[nodiscard]] const std::map<std::string, rules::EncounterTable>& encounterTables() const {
        return encounterTables_;
    }

    // ---- 教学卡与见闻提示（第一期地基包，docs/ch07-foundation.md 2.3、2.5）----
    //
    // 教学卡与野外遭遇同一个套路：**缺省关闭**。几百条无头测试与通关验收不该冷不丁被一张卡拦住、
    // 也不该多出 tutorial.seen.* 旗标；启动器（main.cpp）打开它，教学卡自己的测试打开它（判据：给测试一个关掉它的办法）。
    // 关着时 tutorialEvent / requestTutorial 什么也不做。
    void setTutorialsEnabled(bool enabled) { tutorialsEnabled_ = enabled; }
    [[nodiscard]] bool tutorialsEnabled() const { return tutorialsEnabled_; }
    // 引擎事件（data/tutorials 的 trigger）：此刻有一张能弹的卡就排进队里。事件名的清单见 docs/ch07-foundation-g.md。
    void tutorialEvent(const std::string& event);
    // 脚本 tutorial("id")：按 id 排一张。卡不存在返回 "no_card"，看过了或不在这一章返回 "skipped"，排上了返回空串。
    [[nodiscard]] std::string requestTutorial(const std::string& cardId);
    // 排着、还没弹的卡。弹的时机（flushTutorials）：战斗里轮到我方时；不在战斗里时脚本演完、顶上是走图或设施面板。
    // 对白进行中不弹。
    [[nodiscard]] const std::vector<std::string>& pendingTutorials() const { return pendingTutorials_; }
    // 至今弹过（或设置里关了、只记了看过）的卡，按次序。测试用它断言「弹了哪几张」。
    [[nodiscard]] const std::vector<std::string>& tutorialLog() const { return tutorialLog_; }
    // 开战时记一笔「这一场之前看过哪几张卡」：BattleScene::onEnter 调。轮到我方时对其中每一张发「battle.after.<id>」，
    // 于是「防御换先手」挂 battle.after.boost，就落在蓄劲讲过之后的**下一场**仗第 1 回合，不与蓄劲挤在同一场。
    void noteBattleStart();
    [[nodiscard]] const std::vector<std::string>& tutorialsSeenAtBattleStart() const { return battleStartSeen_; }
    // 记事「指南」页重看一张：直接压卡片，不看开关、不改旗标。
    void replayTutorial(const std::string& cardId);
    // 全部战斗编成（见闻页的「掉落」从这里查）。
    [[nodiscard]] const std::map<std::string, core::BattleSetup>& battles() const { return battles_; }
    // 「见闻：XXX」提示：不进场景栈，画在世界层之上（与得物卡片同一个做法）。
    [[nodiscard]] LoreNotices& loreNotices() { return loreNotices_; }

    // ---- BGM ----
    // 此刻世界层该放的曲子：脚本 bgm("id") 点播的那一首，没点播就是地图属性上的。
    // 战斗打完回到世界层时该续哪一首，问它（BattleScene 自己换战斗曲，收场后由画面路接回来）。
    [[nodiscard]] std::string worldBgm() const;
    // 脚本点播的曲子；空串 = 放地图曲。换图时它仍然作数（夜探翻墙进了墨府，还是那一首）。
    [[nodiscard]] const std::string& bgmOverride() const { return bgmOverride_; }

    // 以某个角色的口吻说一句：记进 spokenKeys 与对话回看，压对话框。脚本的 talk(role, key)
    // 也走这里——路径行动的话与剧情的话必须是同一个口径（契约 5.4）。roleId 为空是旁白。
    // face 是表情（normal / smile / angry / hurt，伙伴改版 3.9），原样送到对话框（DialogueScene::setPortrait）。
    void sayAs(const std::string& roleId, const std::string& textKey,
               const std::string& face = script::kDefaultFace);

    // ---- 脚本 ----
    core::Result<bool> startEvent(const std::string& scriptPath);
    // 脚本挂起期间禁止存档（方案 3.5 规则 4）。
    [[nodiscard]] bool canSave() const;

    // 单帧推进。无头 bot 直接调它，不经过 run() 的窗口循环。
    void tick(double deltaSeconds);

    // 把场景栈画一遍。**只画，不 beginFrame / endFrame**。
    //
    // 从 run() 里抽出来是为了截图自检：抓帧必须插在「画完」与「present」之间
    // （present 之后后台缓冲的内容按 SDL 的规定是未定义的），而那一刀切不进
    // 一个把三件事焊死在一起的循环体里。
    void drawScenes();

    void requestQuit() { quitRequested_ = true; }
    [[nodiscard]] bool quitRequested() const { return quitRequested_; }

    // 供场景回填命令结果；场景完成一条命令后调用。
    void completeCommand(const script::CommandResult& result);

    // 弹一句提示，不牵扯脚本协程。地图层拦住玩家时要说明原因，
    // 否则玩家撞到的就是一堵没有反馈的墙。
    void showMessage(const std::string& textKey);
    [[nodiscard]] bool awaitingCommand() const { return commandPending_; }

    // 最近播出的若干条文案 key，供测试断言「播的是哪一句」。
    //
    // 为什么需要它：对话只有文案 key 的差别时，测试无从分辨。第 2 章复审
    // 查出的那条「手上有药却被告知空着手来的」，两个分支的唯一区别就是
    // key——任何只看钱和背包的断言都抓不住它。这类「游戏说了句像真的假话」
    // 的缺陷，从第 1 章起一直是盲区。
    //
    // 无条件记录而不是只在 headless 下记录：被测的那条路必须就是上线的那条路。
    // 环形上限 kSpokenLogCap 条，长流程不会把内存吃掉。
    [[nodiscard]] const std::vector<std::string>& spokenKeys() const { return spokenKeys_; }
    void clearSpokenKeys() { spokenKeys_.clear(); }

    // 最近一次播出的文案 key；没播过任何一句时返回空串。
    [[nodiscard]] std::string lastSpokenKey() const {
        return spokenKeys_.empty() ? std::string{} : spokenKeys_.back();
    }

    // 玩家回看用的对话记录：画面上真正出现过的那一行。
    //
    // 与上面那份 spokenKeys_ 是两件事，不合并：那一份记 key，是给测试与校验用的
    // 机器口径；这一份记「说话人 + 正文」，是给人看的。合成一份的话回看面板就得
    // 在运行期把 key 再翻译回正文，而翻译不回来的那几条（data 里缺了这条文案）
    // 恰恰是最该被看见的——它们在对话框里显示成 key 本身，回看里却会不见。
    struct SpokenLine {
        std::string speaker;   // 空串是旁白
        std::string body;
        // 说话人的角色 id 与表情（伙伴改版 3.9）：选项框留着上一句时，头像跟着一起留。
        std::string roleId;
        std::string face;
    };
    [[nodiscard]] const std::vector<SpokenLine>& dialogueLog() const { return dialogueLog_; }
    void pushDialogueLine(std::string speaker, std::string body, std::string roleId = {}, std::string face = {});

    // ---- 对白减负（第 7 章地基包路 D，docs/ch07-foundation.md 4.2–4.4）----
    // 自动播放开 / 关：改设置、当场写盘（对话框里按路径行动键走这里）。
    void toggleAutoAdvance();
    // 此刻能不能整段跳过：有脚本在跑、正等着一句 talk 演完（路径行动、提示那种不经脚本的框不跳）。
    [[nodiscard]] bool canSkipDialogue() const;
    // 开始整段跳过：从下一条命令起，talk 不再逐框显示，直接记进回看（spokenKeys 照记），
    // 直到下一条要玩家动手的命令（选择、战斗、商店、结局卡）或脚本结束。途中 give、set_flag、teleport 照常，
    // wait 与淡入淡出压成当场完成。选择永远不自动选：跳到选择就停。
    void beginDialogueSkip();
    [[nodiscard]] bool skippingDialogue() const { return dialogueSkipping_; }
    // 跳完之后那一条「已跳过 N 句 · 按 Tab 回看」。停着的时候世界层上的菜单键翻开回看面板。
    [[nodiscard]] const SkipNotice& skipNotice() const { return skipNotice_; }
    // 插话气泡（bubble 命令）。
    [[nodiscard]] const SpeechBubbles& speechBubbles() const { return speechBubbles_; }
    // 浮一只气泡（不记回看；bubble 命令自己先记）。跟随者按 roleId 每帧重找，找不到用 fallbackCell。截图口也调它。
    void pushSpeechBubble(const std::string& roleId, const std::string& textKey, float seconds,
                          std::optional<Vec2f> fallbackCell);
    // 弹「已跳过 N 句 · 按 Tab 回看」（endDialogueSkip 与截图口共用）。
    void noteDialogueSkipped(int lines);
    // 这个角色此刻在画面上的哪一格（世界格，带滑步的小数）：跟随者、主角按画面上的位置，NPC 按图上那一格。
    // 不在画面上（没有世界层、人不在这张图上、出了镜头）返回空——bubble 据此退回成普通对白。
    [[nodiscard]] std::optional<Vec2f> bubbleCell(const std::string& roleId);
    // 一格在屏幕上的位置（逻辑像素，格的左上角）；没有世界层时返回空。
    [[nodiscard]] std::optional<engine::Point> cellToScreen(Vec2f cell) const;

private:
    // 取出脚本命令并派发。能就地完成的（给物品、置旗标）直接执行并 resume；
    // 需要玩家参与的（对话、选择、战斗）压入场景，由场景回填。
    void pumpScriptCommands();
    // 返回 true 表示命令已就地完成，可立刻回填；false 表示已压入场景，等场景回填。
    // outcome 带回执行结果：扣物品失败等情况要让脚本能分支，不能悄悄当成功。
    bool dispatch(const script::Command& command, script::CommandResult& outcome);
    // 把剧情境界上限抬到 target（已经不低于就不动）。realm.cap 与 realm.advance 共用，
    // 「只升不降」只有这一个定义处。
    void raiseRealmCap(rules::Realm target);
    // 等着的章节卡：脚本演完、场景栈回到行走、这一帧栈上也没有别的变动时才播。
    // 不在脚本还在跑时插进去——那会把一句台词或一场战斗压在卡片底下。
    void flushChapterCards();
    // applyPathEffects 的一条；TakeItem 扣不成返回 false。
    bool applyPathEffect(const core::PathAction& action, const rules::PathEffect& effect,
                         const std::string& speakerRole);
    void finishEncounter(const script::CommandResult& result);
    // 栈上有没有「系统」场景（主菜单、标题画面、设置面板、改键面板）：那段时间记进 playSecondsSystem。
    [[nodiscard]] bool systemSceneActive() const;
    // 把 settings_ 推给引擎（setSettings 与 enableSettingsFile 共用）。
    void applySettings();
    // Alt+Enter 是引擎自己切的全屏：每帧对一次账，不一致就以窗口的真实状态为准改设置并写盘。
    void syncFullscreenFromEngine();

    std::unique_ptr<engine::Engine> engine_;
    std::unique_ptr<script::ScriptHost> scripts_;
    std::unique_ptr<core::TileMap> map_;

    core::GameData data_;
    std::map<std::string, core::BattleSetup> battles_;
    std::map<std::string, std::string> battleBackdrops_;   // battle_id → 背景 id（battleBackdrop）
    std::map<std::string, rules::Shop> shops_;
    std::map<std::string, rules::Shop> initialShops_;
    std::map<std::string, rules::Recipe> recipes_;
    // portalLinks() 的缓存。loaded 与内容分开记：一个一道门都没有的资产根
    // 也是「读过了」，不该每帧重读一遍。
    std::vector<rules::PortalLink> portalLinks_;
    bool portalLinksLoaded_ = false;
    core::GameState state_;
    std::vector<ScenePtr> scenes_;
    std::vector<ScenePtr> pendingPush_;   // 延迟到帧末再改栈，避免在遍历中失效

    ui::Theme theme_;
    std::string assetRoot_;
    bool headless_ = false;
    bool quitRequested_ = false;
    bool commandPending_ = false;
    bool itemsGiven_ = false;   // noteItemsGiven：这一帧发过东西，帧末响一声
    static constexpr std::size_t kSpokenLogCap = 128;
    std::vector<std::string> spokenKeys_;
    // 回看用的对话记录。上限比 key 那一份小：回看是「刚才说了什么」，
    // 不是通关实录，六十行已经比任何一场戏都长（最长的一场 39 句）。
    // 地基包路 D：整段跳过把一整段直接塞进来，跳过之前那一段、跳过的这一段都要翻得全——调到 120。
    static constexpr std::size_t kDialogueLogCap = 120;
    std::vector<SpokenLine> dialogueLog_;
    // 地基包路 D：整段跳过、跳完的提示、插话气泡。
    bool dialogueSkipping_ = false;
    int skippedLines_ = 0;
    SkipNotice skipNotice_;
    SpeechBubbles speechBubbles_;
    void endDialogueSkip();
    void resetDialogueExtras();
    // 记一句话（spokenKeys + 回看），不压对话框。sayAs、整段跳过、气泡共用。
    void recordSpoken(const std::string& roleId, const std::string& textKey, const std::string& face);
    void drawSpeechBubbles();
    int pendingPops_ = 0;

    ChapterTable chapters_;
    std::vector<CardRequest> pendingCards_;
    std::vector<CardRequest> cardLog_;
    float screenFade_ = 0.f;

    // 以回调开的那一仗的收场回调（startBattle）；空 = 此刻没有这样的仗。
    BattleOutcome battleOutcome_;
    BattleStart battleStart_;   // battleStart()
    // 进行中的那一趟历练（enterLilian）；空 = 不在历练里。
    struct LilianTrip {
        std::string siteId;
        std::string mapId;               // 进去之前站的图、格、朝向、队伍
        core::Point position;
        int facing = 2;
        std::vector<core::PartyMember> party;
        core::WorldObjectState objects;   // 这一趟里四种地图对象的状态
    };
    std::optional<LilianTrip> lilian_;
    std::map<std::string, core::LilianSite> lilianSites_;   // lilianSite()
    // 设施 kind = lilian：历练外弹组队面板，历练里就是出口（openFacility 的一支）。
    void openLilianFacility(const core::MapObject& facility);
    bool encountersEnabled_ = false;
    std::map<std::string, rules::EncounterTable> encounterTables_;
    std::string bgmOverride_;

    PortraitBook portraits_;   // portraits()
    double fixedWorldClock_ = 0.0;   // fixedWorldClock()

    // 得物卡片（itemToasts）与集齐奖励表（explorationReward）。图标图集画的时候才载，无头从不碰。
    ItemToasts itemToasts_;
    std::map<std::string, core::ExplorationReward> explorationRewards_;
    engine::TextureId toastIcons_ = engine::kInvalidTexture;
    bool toastIconsTried_ = false;
    hud::Geometry toastGeometry_;
    // 从栈顶往下第一层不透明的是不是世界层（玩家此刻眼里的底图是不是地图）。卡片只在这时走时钟。
    [[nodiscard]] bool worldShown() const;
    // ---- 教学卡与见闻提示（第一期地基包）----
    // 等着弹的卡：战斗里轮到我方、或脚本演完回到走图 / 设施面板时，一帧弹一张（tick 末尾）。
    void flushTutorials();
    // 教学卡判「在不在这一章」用的：章节表的 done_flag（与对白头像同一个判据）。
    [[nodiscard]] rules::ChapterDoneFn chapterDoneFn() const;
    // 每帧看一眼：见闻有没有新解锁（有就推一条提示）、主线目标换没换、名册与默契、气血——后几样折成教学卡的引擎事件。
    void watchGuide();
    // 新开局、读档、返回首页：重记一遍「已经知道的」，读进来的档不该把早就解锁的见闻当成新的再报一遍。
    void resetGuideWatch();
    bool tutorialsEnabled_ = false;
    std::vector<std::string> pendingTutorials_;
    std::vector<std::string> tutorialLog_;
    std::vector<std::string> battleStartSeen_;   // noteBattleStart
    bool guideBaselined_ = false;
    std::set<std::string> loreKnown_;
    std::string guideObjective_;
    LoreNotices loreNotices_;
    hud::Geometry loreGeometry_;
    io::Settings settings_;
    // 「已落盘」的那一份：enable 时读进来生效的、之后每次写盘成功写下的。shutdown 时与 settings_ 不同就补写。
    io::Settings savedSettings_;
    std::string settingsPath_;   // enableSettingsFile 记下的路径；空 = 不碰文件
};

}  // namespace fanren::game
