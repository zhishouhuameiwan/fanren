# 模块接口契约 v2（P2 系统层）

> P1 的契约见 `interfaces.md`，其第 0 节的全局约定（命名空间、错误处理、依赖方向、
> 命名风格）**在 P2 继续生效**，本文只列新增部分。
>
> 签名以本文为准。确需变更先报告，不要擅自改。
>
> 2026-09-20 · P2

---

## 0. P2 的分工与文件范围

| 模块 | 负责内容 | 文件范围 |
| --- | --- | --- |
| A 修炼与时间 | 打坐、修为、突破、日历、闭关跳时、掌天瓶经济 | `src/core/rules/Cultivation.*`、`Calendar.*`、`Bottle.*`、`tests/CultivationTests.cpp` |
| B 炼制四艺 | 炼丹、制符、炼器、布阵的配方与判定 | `src/core/rules/Crafting.*`、`data/recipes/**`、`tests/CraftingTests.cpp` |
| C 经济与遭遇 | 商店、交易、拍卖、遭遇表 | `src/core/rules/Economy.*`、`Encounter.*`、`data/shops/**`、`data/encounters/**`、`tests/EconomyTests.cpp` |
| D 界面控件 | 通用控件与各面板 | `src/ui/**`、`tests/UiTests.cpp` |

**core 层的硬约束不变**：不链接任何东西，不含 SDL / JSON / Lua / 文件 IO。
所有规则都在已解析的结构体上操作，随机数种子由调用方传入，保证可复现。

---

## 1. 修炼与时间（模块 A）

```cpp
// src/core/rules/Calendar.h
namespace fanren::rules {

// 游戏内日历。一切周期性玩法（小瓶凝液、灵草生长、坊市补货、拍卖档期、
// 秘境冷却）都按天推进，不按真实时间。
struct Calendar {
    int day = 1;          // 从 1 起算的绝对天数

    [[nodiscard]] int year() const;    // 第几年，从 1 起算
    [[nodiscard]] int monthOfYear() const;
    [[nodiscard]] int dayOfMonth() const;
};

inline constexpr int kDaysPerMonth = 30;
inline constexpr int kMonthsPerYear = 12;

// 推进 days 天，返回跨过的天数（便于调用方批量结算周期事件）。
int advanceDays(Calendar& calendar, int days);

}  // namespace fanren::rules

// src/core/rules/Cultivation.h
namespace fanren::rules {

// 修炼一次的产出。分开返回而不是直接改 GameState，是为了让调用方决定
// 是否接受（例如闭关中途被打断）。
struct CultivationGain {
    int cultivation = 0;    // 修为增量
    int days = 0;           // 消耗天数
    bool insight = false;   // 是否顿悟（额外收益，低概率）
};

// 打坐若干天。effectiveness 由功法、灵根、洞府灵气等外部因素合成后传入，
// 100 为基准；rules 层不关心它怎么来的。
[[nodiscard]] CultivationGain meditate(Realm realm, int aptitude, int effectiveness,
                                       int days, std::uint32_t seed);

// 突破判定。
struct BreakthroughAttempt {
    bool success = false;
    int cultivationSpent = 0;
    int cultivationLost = 0;   // 失败时的倒扣
    bool backlash = false;     // 走火入魔（失败且重伤）
};

// pillBonus 为丹药加成的百分点（筑基丹、结丹辅药等），无丹药传 0。
[[nodiscard]] BreakthroughAttempt attemptBreakthrough(Realm realm, int cultivation, int aptitude,
                                                      int pillBonus, std::uint32_t seed);

}  // namespace fanren::rules

// src/core/rules/Bottle.h
namespace fanren::rules {

// 掌天瓶。全作的经济中枢：凝液 → 催熟灵草 → 炼丹/出售 → 资源 → 修为。
// 原著里瓶子先于催熟能力被发现，两者是独立的解锁节点（见 docs/大纲.md 第 2 章）。
struct Bottle {
    bool owned = false;
    bool matureKnown = false;   // 是否已发现催熟之能
    int drops = 0;              // 当前绿液滴数
    int lastChargeDay = 0;
};

// 每多少天凝一滴。境界越高越快，这里只给基准，倍率由调用方按境界传入。
inline constexpr int kBaseChargeDays = 7;

// 按日历推进补充绿液，返回新增滴数。
int refill(Bottle& bottle, int currentDay, int chargeDays);

// 催熟一株灵草：消耗一滴绿液，年份跃升。
struct MatureResult {
    bool ok = false;
    int newAge = 0;
    std::string reason;   // ok 为假时说明原因（没瓶子/不会用/没绿液/已达上限）
};

[[nodiscard]] MatureResult matureHerb(Bottle& bottle, int currentAge, int maxAge);

// 灵草按年份计价：年份越高越贵，且是超线性的（百年灵草远不止十年的十倍）。
[[nodiscard]] int herbPrice(int basePrice, int age);

}  // namespace fanren::rules
```

灵田种植（同属模块 A）：

```cpp
// src/core/rules/Field.h
namespace fanren::rules {

struct FieldSlot {
    std::string seedId;      // 空表示空闲
    int plantedDay = 0;
    int age = 0;             // 当前年份
    bool ripe = false;
};

struct SpiritField {
    std::string id;
    std::vector<FieldSlot> slots;
};

// 按日历推进全部槽位，返回成熟的槽位下标。
std::vector<int> growField(SpiritField& field, int currentDay, int daysPerYear);

}  // namespace fanren::rules
```

---

## 2. 炼制四艺（模块 B）

```cpp
// src/core/rules/Crafting.h
namespace fanren::rules {

enum class CraftKind { Alchemy, Talisman, Forge, Formation };

struct Ingredient {
    std::string itemId;
    int count = 1;
    int minAge = 0;      // 灵草最低年份要求，非灵草为 0
};

struct Recipe {
    std::string id;
    std::string name;
    CraftKind kind{};
    std::string productId;
    int productCount = 1;
    int difficulty = 0;              // 0-100
    int requiredProficiency = 0;     // 熟练度门槛
    std::vector<Ingredient> inputs;
};

// 炼制一次的结果。失败时材料是否损毁由 kind 决定：
// 炼丹失败材料尽毁，制符失败只废符纸，这是两种手艺的性格差异。
struct CraftResult {
    bool success = false;
    std::string productId;
    int productCount = 0;
    int proficiencyGain = 0;
    std::vector<Ingredient> consumed;   // 实际消耗掉的材料
    std::string log;                    // 可直接显示给玩家的一行说明
};

// 能否开工：材料够不够、年份够不够、熟练度够不够、有没有炉鼎。
// 返回失败原因（可直接显示），成功时 value 为空串。
[[nodiscard]] core::Result<std::string> canCraft(const Recipe& recipe,
                                                 const core::GameState& state,
                                                 int proficiency, bool hasTool);

// 执行炼制。成功率 = 熟练度 + 资质修正 + 炉鼎品阶 − 配方难度，夹在 [5, 95]。
// 永远留 5% 失手与 5% 侥幸：全成功或全失败都会让玩法失去张力。
[[nodiscard]] CraftResult craft(const Recipe& recipe, const core::GameState& state,
                                int proficiency, int aptitude, int toolGrade,
                                std::uint32_t seed);

}  // namespace fanren::rules
```

### 2.1 契约增补（v2.1，2026-09-20 经主控批准）

实现中追加，均为末尾追加或新函数，不改既有签名：

```cpp
struct Ingredient {
    // ... 契约原有的 itemId / count / minAge ...
    int herbAge = 0;        // 实际取用的年份，由扣料时回填，供 consumed 清单如实反映
    bool catalyst = false;  // 是否为「引」：某些手艺失败时保住引，见 FailurePolicy
};

// 四艺各自的失败代价。炼丹材料尽毁、制符只废符纸、炼器部分损耗、布阵阵旗可回收，
// 这个差异是四种手艺的性格，不是数值微调。
struct FailurePolicy {
    int lossPercent = 100;        // 每味材料按需求量损耗的百分比，向上取整
    bool sparesCatalyst = false;  // 失败时是否保住「引」
};

[[nodiscard]] int successChance(int proficiency, int aptitude, int difficulty, int toolGrade) noexcept;
[[nodiscard]] int proficiencyGain(bool success);
[[nodiscard]] FailurePolicy failurePolicyOf(CraftKind kind);
[[nodiscard]] bool hasToolOfGrade(int toolGrade);
void applyConsumption(core::GameState& state, const std::vector<Ingredient>& consumed);
```

`successChance` 单独暴露，是为了让界面显示「有几成把握」，也让数值平衡能脱离随机直接单测。

**调用约定**：`canCraft` 收的是 `bool hasTool`，`craft` 收的是 `int toolGrade`。
调用 `canCraft` 时 **必须** 用 `hasToolOfGrade(toolGrade)` 计算那个布尔值，
否则两处判断会给出不同答案，「判否不执行」这条不变式就破了。

### 2.2 已裁定的语义细节

| 项 | 裁定 | 理由 |
| --- | --- | --- |
| 判否不执行 | `craft` 内部先跑 `canCraft`，判否就在摇骰与扣料之前返回 | 与战斗模块「非法动作不改变任何状态」同一条纪律 |
| 扣灵草的年份策略 | 优先扣刚好够门槛的低年份堆 | 玩家不会喜欢自己的千年灵草被拿去炼金疮药 |
| 同一 itemId 多条原料 | 必须按 itemId 聚合需求后再校验 | 不聚合会让同一批库存被重复计入，材料不够也能开工。分级配方（同材料分年份门槛）是合理设计，不能靠禁止它来回避 |
| 统计类测试的随机源 | 一律固定常量种子，不用 `random_device` 或时间种子 | 统计测试最容易变成偶发红灯的来源 |

---

## 3. 经济与遭遇（模块 C）

```cpp
// src/core/rules/Economy.h
namespace fanren::rules {

struct ShopEntry {
    std::string itemId;
    int price = 0;
    int stock = -1;       // -1 表示无限
    int restockDays = 0;  // 0 表示不补货
};

struct Shop {
    std::string id;
    std::string nameKey;
    double buyRate = 1.0;    // 买入价倍率
    double sellRate = 0.5;   // 卖出价倍率：玩家卖东西天然吃亏，这是货币回收口
    std::vector<ShopEntry> entries;
};

// 按日历补货。
void restock(Shop& shop, int currentDay);

[[nodiscard]] int buyPrice(const Shop& shop, const core::Item& item, int herbAge);
[[nodiscard]] int sellPrice(const Shop& shop, const core::Item& item, int herbAge);

// 拍卖会。原著的标志性场景，也是稀有配方与法器的主要出口。
struct AuctionLot {
    std::string itemId;
    int startPrice = 0;
    int reservePrice = 0;   // 低于此价流拍
    int herbAge = 0;
};

struct AuctionRound {
    int currentBid = 0;
    int rivalBid = 0;
    bool playerLeading = false;
    bool closed = false;
};

// 对手出价。heat 表示这件拍品的抢手程度（0-100）。
AuctionRound bidStep(const AuctionLot& lot, AuctionRound round, int playerBid,
                     int heat, std::uint32_t seed);

}  // namespace fanren::rules

// src/core/rules/Encounter.h
namespace fanren::rules {

struct EncounterEntry {
    std::string battleId;
    int weight = 1;
    Realm minRealm = Realm::Mortal;
    Realm maxRealm = Realm::CoreLate;
};

struct EncounterTable {
    std::string id;
    std::vector<EncounterEntry> entries;
    int stepsMin = 20;
    int stepsMax = 60;
    int dailyCap = 8;      // 每日触发上限，防刷
};

// 遭遇计数器，随存档持久化。
struct EncounterState {
    int stepsSinceLast = 0;
    int stepsUntilNext = 0;
    int triggeredToday = 0;
    int lastDay = 0;
};

// 走一步。返回非空表示触发了遭遇，值为 battleId。
[[nodiscard]] std::string step(const EncounterTable& table, EncounterState& state,
                               Realm realm, int currentDay, std::uint32_t seed);

}  // namespace fanren::rules
```

---

## 4. 界面控件（模块 D）

`ui` 层依赖 `engine`，不依赖 `game`。控件只做「画 + 处理输入 + 报告结果」，
不直接改 `GameState`：状态变更由 `game` 层在拿到控件结果后执行。

**列表摆位的两条硬口径**（违反这两条的症状一模一样：玩家看不见半数选项，
而控件本身每一条单测都是绿的）：

1. 给 `ListView` 的区域高度一律用 `listAreaHeight(rows, theme)` 算，
   不许写 `rows * 行高`——差的那两个 padding 正好吃掉一到两行。
2. `setPageSize` 一律传**这个面板画得下的行数**（`listRowsThatFit`），
   既不是默认值也不是条目数。传默认值 8 会让画得下十七行的面板只显示八行；
   传条目数会让 `count() <= pageSize` 成立，于是长表的翻页键毫无动静。
   各面板把这个数收在自己的 `listPageRows(theme)` 里，`enter*()` 与 `render()`
   共用同一份几何。

```cpp
// src/ui/Widgets.h
namespace fanren::ui {

struct Theme {
    engine::Color panelFill{18, 18, 24, 232};
    engine::Color panelEdge{150, 140, 110, 255};
    engine::Color text{236, 232, 224, 255};
    engine::Color textDim{150, 146, 138, 255};
    engine::Color highlight{255, 226, 150, 255};
    int padding = 24;
    int lineSpacing = 8;
};

// 列表控件。菜单、背包、商店、图鉴都用它，不要各写一份。
struct ListItem {
    std::string label;
    std::string detail;      // 右侧次要信息（数量、价格）
    bool enabled = true;
};

class ListView {
public:
    void setItems(std::vector<ListItem> items);
    void setPageSize(int rows);

    // 返回 true 表示本帧确认了选择。
    bool update(engine::Engine& engine);
    void render(engine::Engine& engine, const engine::Rect& area, const Theme& theme) const;
    // render 的纯计算部分：这块区域这一帧实际画得满几行。
    // 无头 engine 的 drawText 是空操作，漏显示只能靠这个函数断言。
    [[nodiscard]] int visibleRows(const engine::Rect& area, const Theme& theme) const;

    [[nodiscard]] int selection() const;
    [[nodiscard]] bool cancelled() const;
    void reset();
};

// ---- 行高与区域高的换算 ----
// 摆位一律走这三个函数，不要各自拿字号加行距去凑。
// **「区域高 = 行数 x 行高」是错的**：ListView 画之前还要吃掉上下各一个
// padding，按裸行高摆位的话三个选项只画得出一个（P2 对话框就是这么漏的，
// 游戏里 32 处 choice 全部中招，玩家只看得见第一项）。
[[nodiscard]] int listRowHeight(const Theme& theme);
// 要完整显示 rows 行，列表区至少得多高。rows <= 0 返回 0。
[[nodiscard]] int listAreaHeight(int rows, const Theme& theme);
// listAreaHeight 的反函数。面板按自己的高度设 pageSize 时走这里。
[[nodiscard]] int listRowsThatFit(int areaHeight, const Theme& theme);
// 标题带高度。panelContentArea 内部用的就是它。
[[nodiscard]] int panelTitleBandHeight(bool hasTitle, const Theme& theme);

// 面板：带标题与边框的容器，负责摆位，不管内容。
void drawPanel(engine::Engine& engine, const engine::Rect& area, const std::string& title,
               const Theme& theme);

// 多行文本块，内部走 engine::layoutText，自动断行与禁则。
void drawTextBlock(engine::Engine& engine, const std::string& utf8, const engine::Rect& area,
                   int fontSize, const Theme& theme);

// 数值条（气血、法力、修为进度）。
void drawGauge(engine::Engine& engine, const engine::Rect& area, int current, int maximum,
               const engine::Color& fill, const Theme& theme);

}  // namespace fanren::ui
```

---

## 5. 共同要求

1. **每个模块自带单测**，覆盖边界与失败路径，不只测顺利路径。
2. **随机必须可复现**：种子由调用方传入，同种子同输入必得同结果，并为此写一条专门的测试。
3. **失败要能显示给玩家**：`Result::error` 与 `log` 字段都是 UTF-8 中文，直接可上屏。
4. **不修改 P1 已有的公共签名**。需要新字段时追加在结构体末尾，不插在中间（会破坏聚合初始化）。
5. 中文注释，写「为什么」不写「做什么」。UTF-8 无 BOM、LF。
6. 构建与自验：`cmd.exe //c "H:\\Work\\Kys\\fanren\\build_logged.bat"`，日志在 `build.log`
   （混合编码，用 Python 读字节后按 gb18030 解码）。**必须亲自跑通并贴实测输出**。

   > 2026-09-23 补：上面这一条的调用方式已经过时，**会静默返回 0、什么也不做**。
   > 现行做法见 `docs/handoff.md` 第 0 节：只用 PowerShell 调 `build_logged.bat <槽名>`，
   > 日志是 `build-<槽名>.log`。

---

## 6. 战后发放与静养（2026-09-23，第 4 章二次整改·引擎路）

> **已被 docs/octopath-battle.md 取代**（八方旅人化改造，2026-09-25）：战后发放（6.1–6.3）的口径原样沿用；战斗本身的规则以新文档为准。新增一项战后写回：揭开的破绽记进存档（新文档 2.5）。静养（6.4）与战斗无关，不受影响。

> 起因：`docs/ch04-reverify.md` 第六节 N-3（战斗奖励无人发放）与 N-4（静养没有判据）。
> 发放「接上」由用户拍板。本节是这两件事的契约；判据在 `tests/BattleRewardTests.cpp`
> 与 `tests/RestTests.cpp`，每一条都做过「写坏 → 转红」。

### 6.1 什么战果发奖励

| 战果（`BattlePhase`） | 脚本看到的 `how` | 发不发 | 理由 |
| --- | --- | --- | --- |
| `Won` | `""` | **发** | |
| `Lost` | `"lost"` | 不发 | 没有人会把战利品留给输家 |
| `Escaped` | `"escaped"` | 不发 | 自己跑了，场上的东西他没去捡 |
| `EnemyFled` | `"enemy_fled"` | 不发 | 见下 |
| `Ongoing` | `"unfinished"` | 不发 | 没打完就收场只可能是驱动方出错 |

**口径的出处**：`docs/ch03-design.md`、`docs/ch04-design.md` 与第 3、4 章八场仗的 note
里，**没有一处写过败、逃、敌逃算不算**（设计 3.2 只写了「收获」一栏，默认的是打赢）。
按「没写就只认 Won」办。

**直接后果，写在这里免得被当成漏洞**：识海那一场（`b03_shihai_duoshe`）设计上**必然**以
`EnemyFled` 收场（`docs/interfaces-p3-ch03.md` 5.3），所以它**按本口径一样东西也发不下来**。
数据里原先写着修为 150，2026-09-23 协调者定了默认口径：照「只认 Won」不发，**数据改成 0**、
note 里写明为什么——一个永远到不了手的数就是一个会说谎的数。玩家这边零变化。
`docs/ch03-review.md` LOW-4 当时以为它「照常发放」——那时是因为全仓根本没有发放，谁也看不出来。
要让识海也发，得由协调者改口径、改 `battleRewardEarned`、把数据改回去；
`BattleRewardGrant.TheDreamFightEndsInFlightSoItsCultivationIsNeverPaid` 同时钉着「敌逃不发」
与「识海的奖励是空的」，会跟着红，那一下是提醒改的人这件事是有意的。

### 6.2 发什么、怎么发

```cpp
// src/game/BattleScene.h
struct GrantedReward {            // 一场仗发下去的东西 == 存档的增量
    int cultivation = 0;
    int money = 0;                // material_lingshi 的块数
    std::vector<core::BagEntry> drops;
    bool empty() const;
};
bool battleRewardEarned(core::battle::BattlePhase phase);          // 只有 Won 为真
GrantedReward grantBattleReward(core::GameState& state,
                                const core::BattleReward& reward,
                                core::battle::BattlePhase phase);
// BattleScene::grantedReward()：这一场收场时发了什么（测试对账用）
```

- **修为**：加进 `GameState::cultivation`，饱和加法、负数不收。**只加修为，不动境界**：
  突破是打坐面板里玩家自己按的那一下（`CultivationScene::breakthrough`），第 4 章那三次
  升境是脚本做的（`realm.advance`）。规则层（`rules::Cultivation` / `rules::Realm`）本来就
  **没有自动升境**，发放函数也不许加一个（`BattleRewardGrant.CultivationNeverMovesTheRealm`）。
  修为进账之后玩家**自己按突破**能走多远，由第 7 节的剧情境界上限封住（技术债 G-14，已修）。
- **钱**：`spirit_stones` 按商店的记法记——`material_lingshi` 这件物品
  （`game/ShopScene.h` 的 `kSpiritStoneItemId`），`addItem` 进背包，商店怎么扣就怎么加，
  不另开第二本账。字段叫 `spiritStones`、物品册里那件东西的名字叫「灵石」，都**不许上屏**：
  任何要给玩家看钱的地方一律走 `currencyName()` / `currencyAmount()`（`game/Wording.h`），
  凡人篇显示「碎银」。第 1–5 章「灵石」不许出现在玩家看得见的字里（`docs/handoff.md` 第 5 节）。
- **掉落**：`addItem(itemId, count, herbAge)`，count ≤ 0 或 id 为空的跳过。
- **时机**：`BattleScene::finish` 里、`completeCommand` **之前**——脚本 resume 之后第一句
  就可能去查背包。查不到编成（未知 id 掉进兜底遭遇）不发：那一场不是设计过的仗。
  `finished_` 闸保证同一场只发一次。
- **现阶段不上屏**。战斗场景在胜负分出的那一帧就收场，没有地方说一句「收获」；
  另弹一层对话框会改掉脚本与测试都依赖的场景栈次序。要说，文案放进
  `data/text/chNN_*.json`（`LexiconTests` 扫得到的形状），数字从 `GrantedReward` 取。
  登记为 `docs/tech-debt.md` G-15。

### 6.3 掉落的 `rate`：现阶段不读，一律必掉

`io/BattleLoader.cpp` 不读 `rate`，发放也就无从按概率发；全部 25 场的 `rate` 都是 100，
于是「一律必掉」与数据说的是同一件事。**这是规则，不是普查**：谁写一个不是 100 的 `rate`，
`BattleRewardData.EveryDropRateIsOneTheEngineCanHonour` 当场红（缺省 = 必掉，放行）。

真要概率掉落时：加载器读 `rate`，发放函数收一颗种子（战斗已有一颗可注入的种子，
`core::battle::BattleState::setup` 的 `seed`），测试钉住「同种子同结果」（本文件第 5 节第 2 条），
再删掉上面那道闸。**测试里不许出现不确定性。**

### 6.4 静养：每过去一天回上限一成

`Application::restFor(days)`，挂在 `Application::advanceDays` 上——`advance_days` 脚本命令、
打坐面板、剧情跳时全走这一个入口。

- **口径**：每天回上限的 `kRestPercentPerDay`%（= 10，一成），至少 1 点，封顶满值；
  气血、法力、在队同伴一起；同伴 `hp < 0`（「还没打过仗」的记号）不动。
  出处：`docs/handoff.md` 第 2 节 MEDIUM-3 一行、`docs/ch04-reverify.md` 第七节判据 4。
- **常量名实相符**：从前 `kRestPercentPerDay` 被当成**除数**用（`cap / 10`），10 碰巧是一成，
  改成 20 却是「一天半成」。现在算式是 `cap * kRestPercentPerDay / 100`，改成 20 就是两成。
  取值为 10 时两种算法结果逐一相同，所以这一次改名没有动任何一个数。
- **判据**：期望值一律字面量（上限 120 一天回 12），不从常量推；另有两条经 `advanceDays`
  走的用例，并先验「他确实带着伤」。把 `restFor` 从 `advanceDays` 里摘掉、把速率减半、
  照复验的写法改成 20，三种写坏都有测试转红。
- 它对第 3 章的影响（第 3 章两场仗之间也会回血）登记在 `docs/tech-debt.md` G-17。

---

## 7. 剧情境界上限（2026-09-23，第 4 章二次整改·技术债 G-14）

> 用户拍板采用方案 A，协调者确认了各章取值与老档迁移口径。脚本侧（`realm.cap` 命令、
> `realm.advance` 顺带抬上限）见 `docs/interfaces-p3-script.md` 第 7 节；判据在
> `tests/RealmCapTests.cpp`，另有 `Ch04Walkthrough.AtTheChapterEndEvenTheMostCultivationCannotPushPastTheEighthLayer`。

### 7.1 规则

**玩家自己在打坐面板上按的突破，不能越过剧情最近一次给的上限。** 修为照常累积，只是按不过去；
上限一抬，攒下的修为当场就按得动。脚本升境（`realm.advance`）不受上限约束，并顺带把上限抬到目标层。

有了它，章与章之间的境界是确定值：第 3 章终局封在三层，第 4 章封在八层。

### 7.2 唯一入口（`src/core/rules/Cultivation.h`）

```cpp
enum class BreakthroughBlock { None, SeriesMax, StoryCap, NotEnough };
BreakthroughBlock breakthroughBlock(Realm realm, int cultivation, Realm storyCap);
BreakthroughAttempt tryBreakthrough(Realm realm, int cultivation, Realm storyCap,
                                    int aptitude, int pillBonus, std::uint32_t seed);
// BreakthroughAttempt 末尾追加 BreakthroughBlock blocked（不是 None 时一分不扣、骰子不摇）
```

- 判定次序：**本作上限 → 剧情上限 → 修为**。到了剧情上限而修为也不够时报的是上限——那时说
  「尚差几点」是在骗人。「越过」按境界编号比：下一境界的编号大于上限即越过。
- `CultivationScene` 的冲关那一行（置灰理由）与按下去（`breakthrough`）都只问这两个函数。
  `attemptBreakthrough` 降为骰子本身，游戏层**不许直接调**；
  `RealmCapRule.NothingOutsideTheRulesLayerRollsTheBreakthroughDiceDirectly` 扫 `src/` 全树钉着。

### 7.3 存在哪、谁来抬

- `core::GameState::realmCap`（`rules::Realm`，追加在全部数据成员末尾）。**缺省凡人**：
  新开一局时韩立还没拿到口诀。这是安全的一侧——哪一章忘了抬，玩家看到的是一句明明白白的
  「卡住了」，而不是悄悄越过剧情。
- 只由剧情抬、只升不降：`realm.cap(t)` 抬到至少 `t`；`realm.advance(t)` 顺带抬到至少 `t`。

### 7.4 各章取值（只从设计文档推，协调者 2026-09-23 确认）

| 时点 | 上限 | 谁抬 | 出处 |
| --- | --- | --- | --- |
| 新开局 | 凡人 | 缺省值 | `docs/ch01-design.md` 节点 9：授口诀在章末，此前没有口诀 |
| 第 1 章授口诀后 | 炼气一层 | `ch01/shenshougu_koujue.lua` 的 `realm.cap` | 同上 ＋ `docs/ch02-design.md` 第 2 节：段四才到第二层，此前至多第一层 |
| 第 2 章段四 | 二层 | `ch02/ceng2.lua` 的 `realm.cap` | `docs/ch02-design.md` 第 2 节段四 |
| 第 2 章段五 | 三层 | `ch02/ceng3.lua` 的 `realm.cap` | `docs/ch02-design.md` 第 1 节第 5 条、第 2 节段五 |
| 第 3 章 | 不抬（三层） | — | `docs/ch04-design.md` 1.2：第 3 章章末炼气三层 |
| 第 4 章节点 1 / 2 / 4 | 五 / 七 / 八层 | `realm.advance` 顺带 | `docs/ch04-design.md` 1.2、硬约束 8 |

后续各章同理：剧情在哪一节说他到了第几层，就在那一节 `realm.cap` 或 `realm.advance`。

### 7.5 面板上怎么说

两个阶段各两句（`CultivationLexicon::pushAtStoryCap` / `atStoryCap`），与「火候未到」「没冲过去」
**说成完全不同的事**：那两种是「再坐一阵」「运气差」，这一种是「坐多久都没用，得等」。
凡人阶段不说「瓶颈」，说「卡在这一层了」。这四句是 C++ 字面量，不在 `LexiconTests` 扫的
`data/text/chNN*.json` 里，所以另有两道等效检查：`PanelTests` 的凡人禁词扫描（`cultivationPanelStrings`
已把它们列进去），与 `RealmCapWording.NeitherLineSaysAWordTheLexiconTableHasNotReleased`
（`LexiconTests` 首见章号表上的词一个都不许有）。

### 7.6 存档：v5 → v6

- `payload["realmCap"]` = 上限的境界编号。写了却不是整数、不是合法境界编号 → 读档失败（与 `realm` 同一条口径）。
- **老档（v5 及更早）的上限 = max(当前境界, 由已有剧情旗标推出的上限)**（`io::legacyRealmCap`，5→6 迁移的全部内容）：
  - `ch01.koujue_received` 置位 → 一层；
  - `ch02.koujue_ceng` 的值 v（1–13）→ v 层；
  - 第 4 章的升境是脚本直接改的境界，已由「当前境界」覆盖；
  - 什么都没有（新开局的空档）→ 凡人。

  只取当前境界不够：第 2 章段五之后、玩家还没按到三层的老档，上限会被封在二层，一直卡到第 4 章节点 1，
  而剧情早已说过他到了第三层。旗标表是**老档的补救**，不是上限的来源；新档一律明明白白地存着。
- 当前版本的档若缺这个字段（手拼的档、夹具），按同一个口径推一遍，而不是给凡人。
- `saves/` 下两份检查点已用 `tools/mksave` 按新格式重生成：`ch04-start` 上限三层、`ch04-siege` 上限八层；
  与旧文件逐键比对，payload 只多了 `realmCap` 这一项。
