# 模块接口契约 v1（P1 垂直切片）

> 四个模块并行开发，本文是它们之间唯一的约定。**签名以本文为准**，实现方不得擅自更改
> 公共 API 的名称、参数顺序或语义；确需变更先改本文并通知主控。
>
> 2026-09-20 · P1

---

## 0. 全局约定

| 项 | 约定 |
| --- | --- |
| 命名空间 | `fanren::core` / `fanren::io` / `fanren::script` / `fanren::engine` / `fanren::ui` / `fanren::game` |
| 字符串 | 一律 `std::string`，UTF-8 编码。源文件保存为 UTF-8 无 BOM |
| 错误处理 | **不跨模块边界抛异常**。失败用返回值表达：`bool` + 出参，或下面的 `Result<T>` |
| 头文件包含 | 一律从 `src/` 起算：`#include "core/rules/Realm.h"` |
| 命名 | 类型 `PascalCase`，函数与变量 `camelCase`，成员 `trailing_`，常量 `kPascalCase` |
| 所有权 | 优先值语义；需要多态时用 `std::unique_ptr`，不用裸 `new` |
| 依赖方向 | `game → {ui, engine, script, io, core}`，`ui → engine`，`script → core`，`io → core`。**core 不得链接任何东西** |

```cpp
// src/core/Result.h  —— 由 core 模块提供，所有层可用
namespace fanren::core {
template <typename T>
struct Result {
    bool ok = false;
    T value{};
    std::string error;          // ok == false 时非空，UTF-8，可直接显示给玩家

    static Result success(T v) { return {true, std::move(v), {}}; }
    static Result failure(std::string e) { return {false, {}, std::move(e)}; }
    explicit operator bool() const { return ok; }
};
}
```

---

## 1. engine 模块

负责 SDL3 平台层。**SDL 类型不得出现在本模块的公共头文件里**，game/ui 通过下列句柄操作。

```cpp
// src/engine/Engine.h
namespace fanren::engine {

struct Color { std::uint8_t r{}, g{}, b{}, a{255}; };
struct Rect  { int x{}, y{}, w{}, h{}; };
struct Point { int x{}, y{}; };

// 逻辑分辨率固定 1280x720，整数缩放到窗口（见 docs/map_spec.md 第 1 节）
inline constexpr int kLogicalWidth  = 1280;
inline constexpr int kLogicalHeight = 720;
inline constexpr int kTileSize      = 32;

using TextureId = std::uint32_t;   // 0 表示无效
inline constexpr TextureId kInvalidTexture = 0;

class Engine {
public:
    Engine();
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // headless == true 时不开窗口、不建渲染器：供无头 bot 与 CI 使用。
    // 此模式下所有绘制调用都是合法的空操作，尺寸测量仍然可用。
    core::Result<bool> init(const std::string& title, bool headless);
    void shutdown();

    // 每帧：pollEvents 收集输入 → beginFrame → 绘制 → endFrame
    void pollEvents();
    bool shouldQuit() const;
    void beginFrame();
    void endFrame();
    double deltaSeconds() const;

    // ---- 纹理 ----
    TextureId loadTexture(const std::string& path);   // 失败返回 kInvalidTexture
    void drawTexture(TextureId id, const Rect& src, const Rect& dst);
    void drawRect(const Rect& rect, const Color& color, bool filled);

    // ---- 文本 ----
    // size 为像素字号。字体在 init 时从 assets/fonts/ 载入，全项目一套。
    void drawText(const std::string& utf8, int x, int y, int size, const Color& color);
    Point measureText(const std::string& utf8, int size) const;

    // ---- 输入 ----
    // Key 是逻辑键位，不是物理扫描码；映射表在 engine 内部维护。
    enum class Key { Up, Down, Left, Right, Confirm, Cancel, Menu, Skip, Count };
    bool keyDown(Key key) const;      // 当前是否按住
    bool keyPressed(Key key) const;   // 本帧是否刚按下（带重复节流）

    // ---- 音频 ----
    void playBgm(const std::string& id);
    void stopBgm();
    void playSfx(const std::string& id);
};

}  // namespace fanren::engine
```

### 1.1 中文排版（engine 内独立模块，可单测）

```cpp
// src/engine/TextLayout.h
namespace fanren::engine {

struct LayoutLine { std::string text; int width{}; };

// 按像素宽度断行，遵守中文标点禁则：
//   行首禁止：。，、；：？！）》」』】%
//   行尾禁止：（《「『【
// 西文单词不从中间断开；连续 ASCII 视为一个不可分单元。
// measure 为字符串宽度测量函数，便于脱离 SDL 单测。
std::vector<LayoutLine> layoutText(
    const std::string& utf8,
    int maxWidthPx,
    const std::function<int(const std::string&)>& measure);

// UTF-8 安全的字符切分：按「字符」而非字节前进，供逐字显示用。
std::vector<std::string> splitGraphemes(const std::string& utf8);

}  // namespace fanren::engine
```

**排版模块必须带单测**，覆盖：行首禁则、行尾禁则、纯中文、中英混排、超长无空格串、空串、宽度不足一字。

---

## 2. io 模块

```cpp
// src/io/DataLoader.h
namespace fanren::io {

// 加载 data/ 下的 JSON。目录结构见 docs/map_spec.md 与方案 3.4。
core::Result<core::GameData> loadGameData(const std::string& dataRoot);

// 加载 Tiled .tmj。校验规则见 docs/map_spec.md 第 7 节；
// 缺图层、缺地图属性、对象缺必填属性一律判失败并在 error 里写明哪张图哪一项。
core::Result<core::TileMap> loadTileMap(const std::string& tmjPath);

}  // namespace fanren::io

// src/io/SaveFile.h
namespace fanren::io {

inline constexpr int kSaveVersion = 1;

// JSON 文本存档 + version + 校验和。校验和不匹配时判失败，不静默接受。
core::Result<bool> saveGame(const core::GameState& state, const std::string& path);
core::Result<core::GameState> loadGame(const std::string& path);

}  // namespace fanren::io
```

---

## 3. script 模块

实现方案 3.5 的并发模型。**关键约束：yield 发生在 Lua 侧，不跨 C 调用边界。**

```cpp
// src/script/Command.h
namespace fanren::script {

enum class CommandKind {
    Talk, Choice, Battle, Teleport, FadeOut, FadeIn,
    GiveItem, TakeItem, SetFlag, Shop, Wait, PlaySfx, GameOver, Ending,
};

struct Command {
    CommandKind kind{};
    std::string a;                     // 主参数：文案 key / 地图 id / 物品 id …
    std::string b;                     // 次参数
    int x{}, y{};                      // 坐标类参数
    std::vector<std::string> options;  // Choice 用
};

// 命令执行结果，由 game 层回填后 resume 协程。
struct CommandResult {
    bool ok = true;
    int choiceIndex = -1;   // Choice：选中项，0 起算；取消为 -1
    bool battleWon = false; // Battle
};

}  // namespace fanren::script

// src/script/ScriptHost.h
namespace fanren::script {

class ScriptHost {
public:
    ScriptHost();
    ~ScriptHost();

    core::Result<bool> init(const std::string& scriptRoot, core::GameState* state);

    // 启动一个事件脚本。scriptPath 相对 scriptRoot，如 "ch01/sanshu.lua"。
    core::Result<bool> startEvent(const std::string& scriptPath);

    bool isRunning() const;

    // 取出当前待执行命令。协程未挂起或已结束时返回 false。
    bool pollCommand(Command& out);

    // game 层执行完命令后回填结果，协程继续跑到下一个 yield。
    void resumeWith(const CommandResult& result);

    // 协程挂起期间禁止存档（方案 3.5 规则 4）。
    bool canSave() const { return !isRunning(); }
};

}  // namespace fanren::script
```

Lua 侧的事件 API（在 `scripts/common/api.lua` 中用纯 Lua 实现，内部 `coroutine.yield` 命令表）：

```lua
talk(speaker_id, text_key)            -- 返回 nil
choice(option_keys)                   -- 返回选中序号（1 起算），取消返回 nil
battle(battle_id)                     -- 返回 true/false
teleport(map_id, x, y)
fade.out() / fade.in()
give(item_id, count) / take(item_id, count)
flag.set(name) / flag.get(name)
wait(ms) / sfx(id)
game_over() / ending(title_key, text_key)
```

---

## 4. core 战斗模块

> **已被 docs/octopath-battle.md 取代**（八方旅人化改造，2026-09-25）：格子、移动、射程、护体罡气与下面这份 `Battle.h` 摘录一律作废，规则、接口与数字以新文档为准；合法性单入口、种子唯一来源两条原则照旧。

```cpp
// src/core/battle/Battle.h
namespace fanren::core::battle {

struct Unit {
    std::string id, name;
    int x{}, y{};
    int hp{}, maxHp{}, mp{}, maxMp{};
    int attack{}, defence{}, speed{};
    rules::Realm realm{};
    int element{};          // 五行 bitmask
    bool ally = false;
    bool acted = false, moved = false;
    int shield = 0, stun = 0;
};

enum class ActionKind { Move, Attack, Cast, Item, Defend, Escape };

struct Action {
    ActionKind kind{};
    int actorIndex = -1;
    int targetIndex = -1;
    Point target{};          // Move 用
    std::string magicId;     // Cast 用
};

enum class BattlePhase { Ongoing, Won, Lost, Escaped };

class BattleState {
public:
    void setup(int width, int height, std::vector<Unit> units, std::uint32_t seed);

    int width() const;  int height() const;
    const std::vector<Unit>& units() const;
    int currentActor() const;         // 行动序里当前该谁动；-1 表示回合结束
    int round() const;
    BattlePhase phase() const;

    bool isLegal(const Action& action) const;
    // 非法动作返回 failure 且不改变任何状态（"非法动作不消耗任何资源"）
    core::Result<std::string> apply(const Action& action);   // value 为战斗日志行

    void endTurn();                   // 推进到下一个行动者/回合
    Action decideAi(int actorIndex) const;   // 敌方 AI 选择动作，纯函数式
};

}  // namespace fanren::core::battle
```

### 4.1 契约增补（v1.1，2026-09-20 经主控批准）

实现方按「先报告再改」的要求提出，契约原文未覆盖地形、法术表与物品表，故正式纳入：

```cpp
// 布置阶段（setup 之后、开打之前调用）
void setBlocked(Point cell, bool blocked);   // 不可通行格
void addMagic(const Magic& magic);           // 本场可用法术表
void addItem(const Item& item);              // 本场可用物品表
void setCanEscape(bool allowed);             // 本战是否允许逃跑

// 只读访问（供 UI 与 AI，不改变状态）
const std::vector<std::string>& log() const;
int moveRange(int unitIndex) const;
bool isBlocked(Point cell) const;
bool inBounds(Point cell) const;
int unitIndexAt(Point cell) const;           // 无单位返回 -1
```

`Unit` 末尾追加 `std::vector<std::string> magics`（留空表示登记的法术都会）与 `bool alive() const`。
追加在末尾，按契约字段顺序的聚合初始化不受影响。
`Action` 不新增字段：`ActionKind::Item` 复用 `magicId` 传物品 id。

### 4.2 已裁定的语义细节

| 项 | 裁定 | 理由 |
| --- | --- | --- |
| 伤害随机浮动 | **不要**。伤害是 `f(攻击, 防御, 境界压制, 五行)` 的纯函数 | 无头 bot 回归与战斗回放要可预测；浮动只增噪声不增博弈深度。随机数在本模块只剩逃跑判定一处，「同种子可复现」因此是结构性成立 |
| 距离度量 | 全线**曼哈顿**（移动、近战、施法一致） | 旧原型移动用曼哈顿、攻击用切比雪夫，导致斜对角敌人「能打但走不到」 |
| 眩晕 | 判 `>0` → 递减 → 本回合出局，`stun = N` 正好跳 N 个回合 | 先减后判会让 `stun = 1` 等于没眩晕 |
| 原地不动 | 判**非法** | 否则确认脚下格子会白白吃掉一次移动机会 |
| 敌方逃跑 | 本期不支持（`BattlePhase::Escaped` 只有一个落点） | P2 需要时再扩 phase |
| 护盾档位 | 用 `rules::tierOf()` 而非境界编号算 | 境界编号不连续（筑基 21、结丹 31），按编号算会让结丹凭空多出一截护盾 |

战斗规则要点（从最早原型 fanren-kys 的 `src/BattleScene.cpp` 迁移，但必须与绘制解耦；那份归档 `_archive/fanren-kys/` 已于 2026-09-29 删除）：
行动序按身法、五行相克系数、境界压制（用 `rules::suppressionFactor`）、护体罡气、眩晕、逃跑。

---

## 5. game 层（主控整合，不委托）

场景状态机：`Title / World / Dialogue / Battle / Menu`。每帧 `update(dt)` + `render()`，
**不使用嵌套阻塞 run()**。脚本命令由 `ScriptHost::pollCommand` 取出后派发给对应场景，
场景完成时调用 `resumeWith`。

---

## 6. 垂直切片验收内容

| 项 | 内容 |
| --- | --- |
| 地图 | `ch01_hanjiacun.tmj`（韩家村，改编地名）一张，含 spawn / npc / trigger / portal 各至少一个 |
| 对话 | 三叔引荐，带一次二选一分支，文案走 `data/text/ch01.json` 的 key |
| 战斗 | 一场技术验证战（山路野狗，标注改编），3 对 1，含移动、攻击、逃跑 |
| 存读档 | 任意时刻存档，重启后读回，位置、旗标、背包一致 |
| 无头 bot | `tests/` 下的 bot 在 headless 模式跑完上述流程并断言状态 |
| 校验 | `tools/validate.py` 通过；`ctest` 全绿 |
