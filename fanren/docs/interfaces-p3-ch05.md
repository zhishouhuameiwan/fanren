# P3 第 5 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
> 若你认为契约本身有问题，先在报告里说清楚再动手。
>
> P3 · 2026-09-23 · 依据 `docs/ch05-design.md` 第 10 节（引擎前置）、3.4 节（目标链）、第 6 节（支线）、第 12 节（禁词）。
> 本批只做 E1（任务系统最低版）、E2（韩立不在场的战斗）、E3（第 4 章终局交接存档）、E5 的表一部分。
> 可选项 O1–O5 本轮一概不做。

---

## 0. 四件事，各自现状

| # | 要的东西 | 现状（开工前） |
| --- | --- | --- |
| E1 | 支线任务：数据、状态、告示板 | `data/quests/` 空目录，**规则层、加载器、入口三段全无**（`docs/handoff.md` 第 4 节那张表） |
| E1·Q8 | 目标链收编成主线任务 | 目标链是另一套东西：`data/objectives/chNN.json` → `GameData::objectives` → `rules::currentObjective` → HUD 一行 + 告示板 |
| E2 | 编成里一位开关，开了就不建韩立 | `BattleScene::buildFromSetup` **无条件**建韩立并放在 0 号；`finish()` 按「0 号是韩立」把气血写回存档 |
| E3 | `tests/fixtures/ch04-end-{first,second}.sav` | 不存在，只有第 3 章那两份 |
| E5 | `LexiconTests` 表一补 11 个词 | 表一只有 8 个词 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：**规则层 `src/core/rules/` → 加载器 `src/io/` → 游戏层入口 `src/game/`**。E1 三段都在本批里落。

---

## 1. E1 任务系统（最低版）

### 1.1 一句话口径

**任务只是一张「怎么从旗标和背包读出进度」的表。** 它不存任何东西、不发任何东西、不改存档（仍是 v6）。
接任务、推进、交任务、过期，一律是剧情脚本置旗标、`give` / `take` 物品这些**本来就要做的事**；
任务系统只负责把它们读成「未接 / 进行中 / 已了结 / 已过期」四种状态之一，摆到告示板上。

### 1.2 数据：`data/quests/<quest_id>.json`（一任务一文件，只放支线）

```json
{
  "id": "q05_fengwu_yishu",
  "name": "墨凤舞的医书",
  "chapter": 5,
  "kind": "side",
  "title_key": "ch05.quest.fengwu.title",
  "summary_key": "ch05.quest.fengwu.summary",
  "accept": [
    { "flag": "ch05.fengwu_qiu", "op": ">=", "value": 1 }
  ],
  "steps": [
    {
      "id": "s1_chaoxie",
      "text_key": "ch05.quest.fengwu.s1",
      "done": [ { "flag": "ch05.fengwu_chao", "op": ">=", "value": 1 } ],
      "target_map": "ch05_kezhan",
      "target_object": "trigger_chaoxie"
    },
    {
      "id": "s2_jiaofu",
      "text_key": "ch05.quest.fengwu.s2",
      "done": [ { "flag": "ch05.fengwu_yigao", "op": ">=", "value": 1 } ],
      "target_map": "ch05_mofu",
      "target_object": "npc_mo_fengwu"
    }
  ],
  "complete": [ { "flag": "ch05.fengwu_yigao", "op": ">=", "value": 1 } ],
  "fail":     [ { "flag": "ch05.done", "op": ">=", "value": 1 } ],
  "fail_text_key": "ch05.quest.fengwu.fail",
  "rewards": [
    { "item_id": "herb_huangjing_cao", "count": 6, "herb_age": 40 },
    { "item_id": "herb_zishen_cao", "count": 3, "herb_age": 40 }
  ],
  "origin": "原著 ch124；交付为改编",
  "note": "……"
}
```

> 上面是**示例**（照设计第 6 节 Z1 的字面量写的），不是规定：`data/quests/` 归内容路，id、文案 key、步骤怎么切由内容路定。
> 规定的是下面三张表里的字段、类型与约束。

**顶层字段**（表外的字段一律报错——拼错一个 `fail` 会让任务永远不会过期，而那种错不会自己暴露）：

| 字段 | 类型 | 必填 | 约束 / 语义 |
| --- | --- | --- | --- |
| `id` | string | ✓ | `[a-z0-9_]+`，全 `data/` 唯一（与物品、角色、目标链同一个 id 空间）；**文件名必须是 `<id>.json`**。分工：任务之间、任务与目标链之间撞车，加载器与门禁都拦；与物品、角色等别的类别撞车**只由门禁**（`validate.py` 的 `collect_data`）拦 |
| `name` | string | ✓ | 给维护者看的名字，**不上屏**（`validate.py` 要求每个数据文件都有 `name`） |
| `chapter` | int | ✓ | 1–14 |
| `kind` | string | ✓ | **只许 `"side"`**。主线只从 `data/objectives/` 来（1.5 节），`data/quests/` 里写 `"main"` 报错——两处都能写主线，就得定「两处都写了算谁的」 |
| `title_key` | string | ✓ | 任务名，文案 key |
| `summary_key` | string | ✓ | 一句话说这是什么事；当前没有可显示的步骤时告示板显示它 |
| `accept` | 谓词表 | ✓ 非空 | 全部成立 = 已接（1.3 节）。**可以引用主线旗标**（Q6：前置就写在这里） |
| `steps` | 步骤表 | ✓ 非空 | 见下表；数组顺序就是推进顺序 |
| `complete` | 谓词表 | ✓ 非空 | 全部成立 = 已了结 |
| `fail` | 谓词表 | 否 | 缺省或空表 = **这条任务不会过期**（注意：不是「空表恒真 = 恒失败」，见 1.3） |
| `fail_text_key` | string | `fail` 非空时 ✓ | 过期的原因，告示板写在「已过期」那一行（Q7：不许静默消失）。`fail` 为空时**不许写**（一句永远说不出口的话） |
| `rewards` | 奖励表 | 否 | 缺省 = 空。**任务系统不发**（Q4）：只给测试拿去对账脚本里的 `give`；本轮不上屏 |
| `origin` / `note` | string | 否 | 出处与说明，不上屏 |

**步骤**（`steps[]` 的每一项，表外字段报错）：

| 字段 | 类型 | 必填 | 约束 / 语义 |
| --- | --- | --- | --- |
| `id` | string | ✓ | 本任务内唯一 |
| `text_key` | string | ✓ | 这一步要做什么，文案 key |
| `done` | 谓词表 | ✓ 非空 | 全部成立 = 这一步做完了 |
| `target_map` / `target_object` | string | 否，**成对** | Q9：指这一步在哪儿。要么都写、要么都不写；写了就得真实存在（门禁查）。**本章 HUD 不给支线指路**；告示板只在那一行末尾写上地名 |

**谓词**（`accept` / `complete` / `fail` / `done` 里的每一项，表外字段报错）——恰好三种（Q3）：

| 写法 | 语义 | 约束 |
| --- | --- | --- |
| `{"flag": "<旗标>", "op": ">=", "value": v}` | `flag(旗标) >= v` | `v` 为整数且 **≥ 1**（`>= 0` 恒真，是一句空话） |
| `{"flag": "<旗标>", "op": "==", "value": v}` | `flag(旗标) == v` | `v` 为整数且 ≥ 0（`== 0` 即「没置过」） |
| `{"item": "<物品 id>", "op": ">=", "value": n}` | 背包里这件东西**各年份合计** ≥ n（`GameState::itemCount`） | `n` 为整数且 ≥ 1；物品**不支持 `==`** |

- 一项里 `flag` 与 `item` **恰好出现一个**；`op` 只认上表两种字面量；`value` 必须是整数（`true`、`1.0`、`"1"` 都报错）。
- 一张谓词表是 **AND**：全部成立才算成立。没有 OR——设计第 6 节三条支线用不到；真要 OR，拆成两条步骤或另议。

**奖励**（`rewards[]` 的每一项，表外字段报错）：`{"item_id": "<物品 id>", "count": n, "herb_age": a}`，`count` ≥ 1，`herb_age` 可缺省（0），≥ 0。
写法与编成 `rewards.drops` 同一套字段名，与脚本 `give(item_id, count, herb_age)` 一一对应。
修为、碎银以外的回报（例如 Z2「12d 多一招」）写进 `note`，不进这张表。碎银就是物品 `material_lingshi`。

### 1.3 状态怎么推出来（Q2，纯函数）

```
未接      NotAccepted : accept 不全成立
已了结    Completed   : accept 成立，且 complete 成立
已过期    Failed      : accept 成立，complete 不成立，fail 非空且全成立
进行中    Active      : 其余
```

- **判定次序：已接 → 了结 → 过期。** 了结压过过期：Z1 的 `fail` 是 `ch05.done`，交过医书的人章末照样置 `ch05.done`，他的任务必须是「已了结」而不是「已过期」。
- **没接的任务一律不显示**，哪怕它的 `complete` / `fail` 已经成立：玩家从没听说过的事，谈不上「消失」。
- **空谓词表**：规则层的 AND 按数学口径，空表为真；所以 `accept` / `complete` / `steps[].done` 在加载器里**不许为空**（空的就是「一开局就接了」「什么都不做就算完」，必是数据漏写）；
  `fail` 为空则**另判为「不会过期」**，不走 AND——这是唯一一处例外，写在这里免得有人「统一」掉。
- **当前步骤**（只在 `Active` 时有）= **走得最远的那一条已做完的步骤之后那一条**，与目标链同一个口径（`Objectives.cpp` 的 `progressIndex`）：
  物品谓词会回落（药吃掉了），按「第一条没做完的」算，步骤会倒退回去；按「走得最远的」算不会。
  全部步骤都做完了而 `complete` 还不成立时，当前步骤为空，告示板改显示 `summary_key`。

### 1.4 规则层（`src/core/rules/Quests.h`，core 层：不含 JSON / 文件 / SDL）

```cpp
namespace fanren::core {   // src/core/model/Types.h
enum class QuestKind { Main, Side };
struct QuestCondition {
    enum class Op { FlagAtLeast, FlagEquals, ItemAtLeast };
    Op op = Op::FlagAtLeast;
    std::string subject;   // 旗标名或物品 id
    int value = 1;
};
struct QuestStep {
    std::string id, textKey;
    std::vector<QuestCondition> done;   // kind=main 时为空：主线的完成只认目标链判据，见 1.5
    std::string targetMap, targetObject;
};
struct Quest {
    std::string id, name;
    int chapter = 0;
    QuestKind kind = QuestKind::Side;
    std::string titleKey, summaryKey;
    std::vector<QuestStep> steps;
    std::vector<QuestCondition> accept, complete, fail;
    std::string failTextKey;
    std::vector<BagEntry> rewards;
    std::string origin, note;
};
// GameData 末尾追加：std::vector<Quest> quests;  主线（按章）在前，支线（按章、id）在后
}

namespace fanren::rules {
enum class QuestStatus { NotAccepted, Active, Completed, Failed };
bool conditionHolds(const core::QuestCondition&, const core::GameState&);
bool allConditionsHold(const std::vector<core::QuestCondition>&, const core::GameState&);  // 空表为真
QuestStatus questStatus(const core::Quest&, const std::vector<core::Objective>& mainChain,
                        const core::GameState&);
const core::QuestStep* currentQuestStep(const core::Quest&, const std::vector<core::Objective>& mainChain,
                                        const core::GameState&);   // 非 Active 返回 nullptr
struct QuestEntry { const core::Quest* quest; QuestStatus status; const core::QuestStep* step; };
std::vector<QuestEntry> sideQuestJournal(const std::vector<core::Quest>&, const core::GameState&);
}
```

- 全部取 `const core::GameState&`：**任务系统从签名上就改不了存档**（Q4 的结构保证，不靠纪律）。
- `sideQuestJournal`：只收支线、只收 `NotAccepted` 以外的；排序为**进行中 → 已过期 → 已了结**，组内按章号**倒序**（新的在上）、同章按 id。告示板照它的顺序摆。

### 1.5 主线：目标链原地收编（Q8）

- **`data/objectives/chNN.json` 原地读，不移动、不改名、不改文件内容。** 加载器在读目标链的同一趟里，每个文件另产出一条 `kind = Main` 的 `Quest`：
  `id` = 文件顶层的 `id`（如 `objectives_ch01`）、`name` = 文件的 `name`、`chapter` = 文件的 `chapter`；
  `steps` 与目标链的步骤一一对应（`id`、`text_key` → `textKey`、`target_map`、`target_object`），`done` 留空；
  `accept` / `complete` / `fail` / `title_key` / `summary_key` 一概留空。
- **`GameData::objectives` 一个字节不变**，仍是 HUD 与告示板上半截的唯一来源（Q5：HUD 那一行仍只给主线）。
- **主线任务的状态不另算，照目标链判据**（`rules::currentObjective`，「走得最远的那一步」之后那一步）：
  - 这一章的步骤在整条链上占 `[first, last]`，当前进度下标为 `p`（走完为链长）；
  - `p > last` → 已了结；`first ≤ p ≤ last` → 进行中，当前步骤 = 链上第 `p` 步对应的那一条；`p < first` → 未接。
  - 这就保证了「任意时刻至多一章主线在进行中，且就是 HUD 上那一步所在的那一章」。
- **一章一条主线**：两个目标链文件同属一章，加载失败、门禁报错。
- 目标链文件的顶层 `id` 从此在加载器里也是必填（门禁本来就要求）。
- **第 5 章的 `data/objectives/ch05.json` 由内容路写**，放进目录即被读成第 5 章主线，引擎这边不需要任何改动。

### 1.6 加载器（`src/io/QuestLoader.{h,cpp}`，由 `io::loadGameData` 调）

```cpp
core::Result<core::Quest> loadQuestFile(const std::string& path);                 // 单个支线文件
core::Result<std::vector<core::Quest>> loadQuests(const std::string& questsDir);  // 整个目录，递归，按路径排序
```

加载器只查**形状**，失败一律写明文件与字段；引用是否存在（文案、旗标、物品、地图对象）归门禁（1.8）。
拒收：缺必填字段、类型不对、表外字段、`kind` 不是 `side`、`chapter` 越界、`accept` / `complete` / `steps` / 某步 `done` 为空、
`fail` 非空而无 `fail_text_key`（或反之）、谓词三种写法之外的任何写法、步骤 id 重复、`target_map` / `target_object` 落单、
奖励 `count` < 1 或 `herb_age` < 0、支线 id 重复、与主线 id 撞车、同章两条主线。目录不存在 = 0 条支线，不算错。

### 1.7 游戏层入口：告示板（`BoardScene`）

- 上半截「眼下」**不变**：当前主线目标与地点。
- 下半截的列表：**先列支线**（`sideQuestJournal` 的顺序），**再列做过的主线步骤**（与现在逐行相同，倒序）。
- 支线一行 = 左栏正文 + 右栏状态字：

  | 状态 | 左栏 | 右栏 |
  | --- | --- | --- |
  | 进行中 | `任务名：当前步骤`，步骤带 target 时末尾加 `（地名）`；没有当前步骤时 `任务名：summary` | `进行中` |
  | 已过期 | `任务名：<fail_text_key 那句原因>` | `已过期` |
  | 已了结 | `任务名` | `已了结` |

  **文案长度建议**（一行要放得下）：任务名 ≤ 6 字；步骤、原因、summary ≤ 16 字。
- 行的内容由 `BoardScene::buildRows(const core::GameData&, const core::GameState&)`（静态、纯）给出，测试直接读它。
- **HUD 那一行、目标框、指路箭头一律不看支线**（Q5、Q9）。

### 1.8 门禁（`tools/validate.py` 规则 23，`validate_selftest.py` M 节逐条负例）

对 `data/quests/**/*.json` 逐个查（Q10）：

1. 文件名 == `<id>.json`；
2. `kind == "side"`；`chapter` ∈ 1..14；
3. 1.2 那三张表之外的字段报错（顶层、步骤、谓词、奖励四层都查）；
4. `title_key` / `summary_key` / 每步 `text_key` / `fail_text_key` 必填（按 1.2 的条件）且**文案存在**；
5. `accept` / `complete` / `steps` / 每步 `done` 非空；步骤 id 本任务内唯一；
6. `fail` 与 `fail_text_key` 成对（有一个就得有另一个）；
7. 谓词写法恰为三种之一（`flag` 与 `item` 恰一个、`op`、`value` 的类型与下限按 1.2）；
8. 谓词里的旗标**已在 `data/flags.json` 登记**，且**有脚本会置它**（`flag.set` 出现在某个脚本里）——没人置的旗标会让任务永远接不到、永远了结不了、或永远不会过期；
9. 谓词与奖励里的物品 id 存在；奖励 `count` ≥ 1、`herb_age` ≥ 0；
10. 步骤的 `target_map` / `target_object` 成对，且地图与对象真实存在；
11. **一章一条主线**：`data/objectives/` 下两个文件同一个 `chapter` 报错。

文案进 `data/text/chNN_quests.json`（Q11）。**key 建议 `chNN.quest.<短名>.<用途>`**（`title` / `summary` / `s1`… / `fail`）：
带上 `chNN.` 前缀，设计 12.2 那几条「`ch05.` 文案不含某词」的专项断言就一并扫到任务文案；文件名带 `chNN`，`LexiconTests` 的首见章号闸门也扫得到。

### 1.9 测试（`tests/QuestTests.cpp`）

- 谓词三种各一条「成立 / 不成立」，并卡在边界上（`>= v` 在 `v−1` 与 `v`；`== v` 在 `v±1`；物品按各年份合计）；
- 状态四种与判定次序（未接压过一切、了结压过过期、`fail` 空 = 不会过期）；
- 当前步骤「走得最远」：物品回落不倒退；全做完未了结时为空；
- 主线：对真链的每一个前缀，主线任务的状态与当前步骤与 `currentObjective` 一致，且**任意时刻至多一条主线进行中**；
- 加载器：好文件读得进；1.6 那张拒收表逐项一条负例；主线任务由目标链原地生成、与 `GameData::objectives` 逐步对应；
- 告示板：进行中 / 已过期 / 已了结三种行的样子（**先验那句原因确实有内容**，不写成「找不到某个词就算过」）；未接不显示；
  **第 1–4 章零变化**：沿第 1–4 章真链的每一个前缀，告示板的行与改动前的算法逐行相同，HUD 那一步不变；
- 任务系统不发奖励：推出状态、开告示板前后存档逐字段相同。

每一种谓词、每一条状态规则都有「写坏 → 转红」的变异证据（落在第 5 节）。

---

## 2. E2 韩立不在场的战斗

> **已被 docs/octopath-battle.md 取代**（八方旅人化改造，2026-09-25）：开关与口径不变；韩立不在场的仗同样不往存档写揭开的破绽（新文档 2.5）。

### 2.1 数据：`data/battles/<id>.json` 顶层一位

```json
{ "id": "b05_duobang", "hero_absent": true, "units": [
    { "role_id": "qu_hun", "x": 3, "y": 5, "faction": "ally" },
    { "role_id": "matou_dashou", "x": 3, "y": 6, "faction": "ally" },
    { "role_id": "matou_dashou", "x": 12, "y": 4, "faction": "enemy" } ] }
```

| 字段 | 类型 | 缺省 | 约束 |
| --- | --- | --- | --- |
| `hero_absent` | bool | `false` | 写了却不是布尔（`"true"`、`1`）→ **加载期报错**，不按缺省悄悄收下 |

- 缺省为假：第 3、4 章与第 6–14 章现有编成一个字不改，行为零变化。
- 为真时 **`units` 里第 0 波必须至少有一个 `"faction": "ally"`**，否则加载期报错：
  一个友军都没有（或友军全排在后面几波）的仗，开场即判负，那不是一场仗。
- 为真时 `player_spawn` **不读**（没有人站在那儿）；写了不报错。
- `core::BattleSetup` 末尾追加 `bool heroAbsent = false;`。

### 2.2 开了开关，战斗里是什么样（`BattleScene`）

| 项 | 口径 | 理由 |
| --- | --- | --- |
| 韩立 | **不建这个单位**：单位表里没有 `id == "hanli"` | 他在客栈睡觉（ch122） |
| 同伴 | **不带 `GameState::party` 里的任何人**，我方就是编成里写死的 `ally` | 他的同伴在他身边，不在这一场；设计 10.1：「我方全是编成里写死的 `faction: ally`」 |
| 操纵 | 友军照旧由玩家操纵（与第 8 章编成同一套） | — |
| 胜负 | 敌方全倒 = 胜；**友军全倒 = 败**（core 本来就只按阵营判，不认人名） | 设计 10.2 |
| 物品 | **不登记背包里的东西**，物品一栏为空 | 药囊在他身上；设计 8.4：「⑤ 不动背包」 |
| 法术 | 照旧：场上各单位 `data/roles` 声明的并集 | — |
| 逃跑 | 照 `can_escape`；任一友军逃成即 `Escaped` | 与现行口径一致 |
| 奖励 | 照 `docs/interfaces-p2.md` 第 6 节：只有 `Won` 发（本章 ⑤ 写 0） | — |
| 战后 | **`GameState` 的气血、法力、同伴气血一个字节不动**；除第 6 节那份奖励外存档零变化 | 他在睡觉 |
| 脚本 | `battle(id)` 的返回照旧（胜否、`code`），**没有新 API** | — |

`finish()` 从此不再假定「0 号是韩立」：只有本场真的建了韩立才写回他的气血与同伴气血。

### 2.3 测试（`tests/Ch05HeroAbsentTests.cpp`）

- 加载器：`hero_absent` 读得进；缺省为假；非布尔报错；为真而无友军报错；为真而友军全在第 1 波以后报错；
- **真仓库全部现有编成**的 `heroAbsent` 都是假（除开关为真的那几场外，开场 0 号仍是韩立）；
- 开关为真：单位表里没有 `hanli`、没有队伍里的同伴；物品登记为空；友军全倒判负、敌方全倒判胜；
- 战后 `hp` / `mp` / `party[].hp` 与开战前逐一相同（**先验他确实带着伤、同伴确实不满血**，满血局面下「没变」是空话）；
- 奖励照发（胜）/ 不发（负）。

每条都做「写坏 → 转红」（第 5 节）。

---

## 3. E3 第 4 章终局交接存档

### 3.1 两侧是哪两侧（照 `tests/Ch04SliceTests.cpp` 的实际分支）

| 文件 | 由哪条用例写出 | 起点 | 分支 |
| --- | --- | --- | --- |
| `tests/fixtures/ch04-end-first.sav` | `Ch04Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens` | `ch03-end-first.sav` | 每一处选择取**第一项**（守辕门）；节点 11 手上有药，留了三瓶 |
| `tests/fixtures/ch04-end-second.sav` | `Ch04Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd` | `ch03-end-second.sav` | 每一处选择取**第二项**（放进来围歼）；**节点 11 之前测试把养精丹全部清掉**，走「翻遍箱底也没有」那一条 |

> **第二侧那一处清药是第 4 章通关测试里的手摆**（`while (s.removeItem(kPill, 1))`，为了走到另一条台词），不是玩家的动作。
> 本批按「照实际分支定」原样导出，**不改第 4 章的走法**；于是第二侧交给第 5 章的是**养精丹 0 瓶**的终局。
> 这对设计 8.5 的复量（P0−2 / P0 / P0＋2）是一个要协调者知道的事实：第二侧的 P0 = 0。实测数见 3.4。

`Ch04EverySiegePlan` 那六条 TEST_P 不导出：它们是同一侧的三种布置，不是交接的两侧。

### 3.2 生成、比对、重写开关

照第 3 章那一套（`tests/ChapterFixture.h`、`Ch03Walkthrough::settleEndingAgainstFixture`）：

- 两条用例在 `ch04.done` 那一刻把终局与对应文件**逐字段**比（`comparableSaveLines`：除 `checksum` 与两个计时器外全比），差一个字段就红；
- **重写开关** `FANREN_WRITE_CH04_FIXTURES=1`：置了它，这两条改为写出文件而不是比对。
  ```
  set FANREN_WRITE_CH04_FIXTURES=1
  build-<槽>\fanren_tests.exe --gtest_filter=Ch04Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens:Ch04Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd
  ```
  写完**先跑一遍不带这个变量的全套**。
- 常量进 `tests/ChapterFixture.h`：`kChapterFourEndingFirst` / `kChapterFourEndingSecond` / `kWriteChapterFourFixturesEnv`。

### 3.3 第 5 章那一头

第 5 章通关测试（测试路写）直接 `io::loadGame` 读这两份起步，与第 4 章读第 3 章那两份同一个办法；
与存档不同的字段每一个写一行来路注释（设计第 2 节）。本批不写第 5 章通关测试。

### 3.4 实测终局

见第 5.3 节（两侧逐项，外加与设计第 2 节「硬 / 估」对不上的几处）。

---

## 4. E5 `LexiconTests` 表一补词

只补表一（原著首见章号），11 个，章号照设计 12.1：

| 词 | 首见 | 效果（按现有规则推） |
| --- | --- | --- |
| 灵气 | 126 | 第 1–5 章禁 |
| 炼气 | 127 | 第 1–5 章禁 |
| 筑基期 | 127 | 第 1–5 章禁（「筑基」ch58 那条不动） |
| 真元 | 164 | 第 1–6 章禁 |
| 黄枫谷 | 127 | 第 1–5 章禁 |
| 太南小会 | 128 | 第 1–5 章禁 |
| 万小山 | 126 | 第 1–5 章禁 |
| 灵符 | 131 | 第 1–5 章禁 |
| 坊市 | 146 | 第 1–5 章禁 |
| 储物袋 | 148 | 第 1–5 章禁 |
| 定颜丹 | 155 | 第 1–6 章禁 |

补之前先扫第 1–4 章全部 `data/text/ch0[1-4]*.json`，证明 11 个词命中为 0（扫描结果落在第 5 节）。
第 5 章专项的章内先后断言（寒毒、惊蛟会、升仙、天眼……）**不归本批**，内容路写。

---

## 5. 落地记录（2026-09-23，引擎路）

### 5.1 落在哪

| 契约条目 | 落在哪 |
| --- | --- |
| 1.4 数据结构 | `src/core/model/Types.h`：`QuestKind` / `QuestCondition` / `QuestStep` / `Quest`，`GameData::quests`（追加在末尾） |
| 1.3 / 1.4 规则层 | `src/core/rules/Quests.{h,cpp}`（新）：`conditionHolds` / `allConditionsHold` / `questStatus` / `currentQuestStep` / `sideQuestJournal` |
| 1.5 主线原地收编 | `src/io/DataLoader.cpp`：`loadOneObjectiveFile` 同一趟产出主线任务；一章一条主线、支线与目标链 id 撞车都判失败；目标链顶层 `id` 从此必填 |
| 1.6 加载器 | `src/io/QuestLoader.{h,cpp}`（新）：`loadQuestFile` / `loadQuests`，由 `io::loadGameData` 调 |
| 1.7 告示板 | `src/game/BoardScene.{h,cpp}`：`BoardScene::buildRows`（静态、纯），右栏三个字 `kActiveTag` / `kFailedTag` / `kCompletedTag` |
| 1.8 门禁 | `tools/validate.py` `check_quests`（规则 23）；`tools/validate_selftest.py` M 节 31 条 |
| 2.1 编成开关 | `core::BattleSetup::heroAbsent`（追加在末尾）；`src/io/BattleLoader.cpp` 两道加载期闸（非布尔、第 0 波无 ally） |
| 2.2 战斗里 | `src/game/BattleScene.cpp`：韩立的建法抽成 `makeHero`；`heroPresent_` 决定建不建韩立、带不带同伴、登不登记背包、`finish()` 写不写回 |
| 3 交接存档 | `tests/ChapterFixture.h`：`kChapterFourEnding{First,Second}`、`kWriteChapterFourFixturesEnv`、通用的 `settleAgainstFixture`；`tests/Ch04SliceTests.cpp` 两条通关用例末尾各一句；`tests/fixtures/ch04-end-{first,second}.sav` |
| 4 词表 | `tests/LexiconTests.cpp` 表一 +11 行（只动表一） |
| 测试 | `tests/QuestTests.cpp`（30 条）、`tests/Ch05HeroAbsentTests.cpp`（10 条）；全套 926 → 966 |

**`GameData::objectives`、`rules::Objectives`、`WorldScene` 一行未改**：HUD 与指路仍只读目标链。

### 5.2 第 1–4 章零变化的证据

- 现有 `ObjectiveTests` 全部 21 条、四章通关测试（`Ch02SliceTests` / `Ch03SliceTests` / `Ch04SliceTests` 含 `Ch04EverySiegePlan` 六条与 `Ch04HandSweep`）全绿，未改一行断言；
- `ShippedQuests.ChapterOneToFourBoardRowsAreExactlyWhatTheyWereBeforeQuestsExisted`：沿第 1–4 章真链 53 个前缀，告示板的行与改动前的算法（抄在用例里）逐行相同；
- `Ch05HeroAbsentShipped.EveryShippedFightWithoutTheSwitchStillOpensWithHanLiAsUnitZero`：真仓库每一场开关为假的编成，开场 0 号仍是韩立；
- `QuestInGame.AnActiveSideQuestDoesNotMoveTheHudOrTheLitDoor`：一条进行中的支线指着别的图，HUD 那一步、亮的门、左上角第二行都不变。

### 5.3 两份交接存档实测（`tests/fixtures/ch04-end-*.sav`，由 `FANREN_WRITE_CH04_FIXTURES=1` 写出）

| 字段 | first | second | 设计第 2 节 | 对得上吗 |
| --- | --- | --- | --- | --- |
| `mapId` / `position` | `ch01_hanjiacun` (36,17) | 同左 | 村东老树下（硬） | ✓ |
| `day` | 1521 | 1521 | 约 1521（估） | ✓ |
| `realm` / `realmCap` | 8 / 8 | 8 / 8 | 八层 / 八层（硬） | ✓ |
| `hp` / `maxHp`、`mp` / `maxMp` | 120/120、80/80 | 同左 | 满（估）；120 / 80（硬） | ✓ |
| `learnedMagics` | 火弹术、御风决 | 同左 | 硬 | ✓ |
| `party` | `qu_hun`，**hp 31** | 同左 | `[qu_hun]`，**满血 55（硬）** | **✗ 31 / 55**：第 4 章章末回村 5 天、东去 1 天的静养没把曲魂养满。第 5 章 `SetUp` 若照设计写「满血 55」会当场红——以 fixture 为准，设计这一格要改 |
| `talisman_jianfu` / `material_heipai` | 1 / 1 | 1 / 1 | 硬 | ✓ |
| `story_mo_jiashu` / `story_mo_shouzha` | 1 / 1 | 1 / 1 | 硬 | ✓ |
| `weapon_yudai_duanjian` | 1 | 1 | 0 或 1（分支） | **两侧都是 1**：节点 3a「没有软剑」那种写法从交接存档起步走不到 |
| `pill_yangjing_dan` | 12 | **0** | 首侧约 12（估） | 首侧 ✓；次侧是第 4 章测试清药的结果（3.1、`docs/tech-debt.md` G-24） |
| `pill_jinchuang_yao` | 5 | 6 | 首侧约 5（估） | ✓ |
| `pill_qingling_san` | 2 | 2 | 约 4–8（估） | 偏少，以 fixture 为准 |
| `material_lingshi`（碎银） | 112 | 148 | 首侧约 112（估） | ✓ |
| 黄精 / 紫参 | 零年 7 / 2 | 零年 7、二十四年 2 / 零年 2 | 零年 7 / 2（估） | ✓（次侧多两株二十四年黄精） |
| `bottle` | owned、matureKnown、3/3 滴 | 同左 | 估 | ✓ |
| `alchemyProficiency` | 100 | 100 | 约 20（估） | 偏高，以 fixture 为准（客栈药炉成功率会比设计估的高） |
| `ch03.jieyao_xuan` | 1 | 2 | 1 / 2（分支） | ✓ 两侧各一种 |
| `ch04.jianfu_dedao` | 1 | 2 | 1 / 2（分支） | ✓ |
| `ch04.done` | 1 | 1 | 硬 | ✓ |
| `story.xiuxian_known` | 未置 | 未置 | 0（硬） | ✓ |

两侧另都继承 `docs/tech-debt.md` G-23（日子从第 1 天算、第 1、2 章旗标不全）。

### 5.4 词表补词前的扫描

补之前用 Python 按字节读 `data/text/ch0[1-4]*.json` 共 16 个文件（186,697 字节）：11 个词命中 **0** 条。
当时 `data/text/` 里还没有 `ch05*` 文件；内容路的第 5 章文案落地后由 `LexiconTests` 按首见章号自动扫到。

### 5.5 变异证据（写坏 → 转红，还原后全绿）

C++ 侧六批，每批只改被测物、各打不同的用例，经 `build_logged.bat c5eng` 全量跑，红的用例与预期逐一对上：

| 批 | 改坏了什么 | 转红的用例 |
| --- | --- | --- |
| A | `FlagEquals` 的 `==` 改成 `>=` | `QuestRule.FlagEqualsHoldsOnlyOnTheValue` |
| A | 加载器顶层表外字段不查 | `QuestLoading.EveryShapeTheContractRefusesIsRefusedAndSaysWhere`（「顶层字段拼错」那一条） |
| A | `hero_absent` 退回 `readBool`（非布尔按缺省收） | `Ch05HeroAbsentLoading.ASwitchThatIsNotABooleanIsRefused` |
| A | 过期那一行不写原因 | `QuestBoard.ActiveFailedAndCompletedSideQuestsEachShowTheirOwnLineAboveTheMainSteps` |
| A | 一章两条主线不查 | `QuestLoading.TwoChainsForOneChapterAreRefused` |
| B | 物品谓词只数零年那一堆 | `QuestRule.ItemAtLeastCountsEveryAgeTogether` |
| B | `fail` 为空也走 AND | `QuestStatusRule.AnEmptyFailListMeansTheQuestNeverExpires` |
| B | 开了开关不查第 0 波 ally | `Ch05HeroAbsentLoading.ASwitchedOnFightWithNoAllyIsRefusedAtLoadTime`、`…AlliesThatOnlyArriveInALaterWaveDoNotCount` |
| B | 开了开关照样带同伴 | `Ch05HeroAbsentFight.NoHanLiNoCompanionAndNothingFromTheBagIsOnTheField` |
| B | 手改 `ch04-end-first.sav` 的 `day` | `Ch04Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens`（校验和不符，读不回来） |
| C | 了结与过期判定次序颠倒 | `QuestStatusRule.CompletingBeatsExpiring` |
| C | 当前步骤改成「第一条没做完的」 | `QuestStepRule.TheCurrentStepFollowsTheFurthestDoneStep`、`…ASkippedStepDoesNotHoldTheQuestBack` |
| C | `finish()` 不看 `heroPresent_` 照写回 | `Ch05HeroAbsentFight` 的胜 / 负 / 逃三条（「韩立的气血变了」） |
| C | 开了开关照样登记背包 | `Ch05HeroAbsentFight.NoHanLiNoCompanionAndNothingFromTheBagIsOnTheField` |
| C | 主线那几行不再倒序 | `ShippedQuests.ChapterOneToFourBoardRowsAreExactlyWhatTheyWereBeforeQuestsExisted` |
| D | 支线不看 `accept` | `QuestStatusRule.NotAcceptedHidesEvenAFinishedOrExpiredQuest`、`…AnEmptyAcceptListIsNeverTakenAsAccepted`、`QuestJournal.*`、`QuestBoard.*` |
| D | 主线「进行中」的下界 `>=` 改 `>` | `QuestMain.AMainQuestFollowsTheChainIncludingACheckpointThatSkippedSteps`、`ShippedQuests.WalkingTheRealChainKeepsExactlyTheHudChapterActive` |
| D | 开了开关照样建韩立 | `Ch05HeroAbsentFight` 的三条（场上出现 `hanli`） |
| D | 开告示板时把奖励发下去 | `QuestInGame.OpeningTheBoardOnACompletedQuestGivesNothing` |
| D | 加载器放行 `kind = "main"` | `QuestLoading.EveryShapeTheContractRefusesIsRefusedAndSaysWhere`（「kind 写成 main 却被收下了」） |
| D | 第 0 波 ally 的判据去掉 `wave == 0` | `Ch05HeroAbsentLoading.AlliesThatOnlyArriveInALaterWaveDoNotCount` |
| D | 第二侧交出去之前 `day += 1` | `Ch04Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd`（「第 102 行（字段 day）」逐字段比出来） |
| E | core 判负改成「韩立倒下即负」 | `Ch05HeroAbsentFight.TheAlliesWinItAndHisStateIsUntouchedButTheRewardIsPaid`（胜负只看友军）；另有 50 余条 core 战斗测试一并红，属预期噪声 |
| F | `BattleSetup::heroAbsent` 缺省改成 `true` | `Ch05HeroAbsentLoading.TheSwitchIsOffUnlessWritten`；全仓 321 条红（缺省一翻，全部老编成都成了无友军的仗） |

门禁侧 14 条，在私有副本上逐条改坏 `validate.py` 再跑 `validate_selftest.py`，每条都让 M 节对应的那一条 FAIL：
有人置的检查、顶层表外字段、kind、一章一条主线、target 成对、文件名、`>= 0`、fail 缺原因、谓词物品、旗标登记、非空、步骤 id 重复、value 整数，
以及把 `check_quests` 从 `main` 里摘掉（M 节 30 条同时 FAIL）。

词表侧不改仓库文案：在临时目录复制一份 `data/text/`、往某章塞一个词，以它为工作目录跑 `Lexicon.*`：

| 往哪一章塞了什么 | 期望 | 实测 |
| --- | --- | --- |
| 第 4 章塞「灵气」（首见 ch126） | 红 | 红，报「ch04_main.json 里出现了「灵气」」 |
| 第 4 章塞「炼气」（ch127） | 红 | 红 |
| 第 5 章塞「储物袋」（ch148） | 红 | 红 |
| 第 6 章塞「定颜丹」（ch155，第 1–6 章禁） | 红 | 红 |
| 第 6 章塞「真元」（ch164，第 1–6 章禁） | 红 | 红 |
| 对照：第 6 章塞「灵气」（ch126 落在第 6 章里） | 绿 | 绿 |
| 对照：第 6 章塞「坊市」（ch146 落在第 6 章里） | 绿 | 绿 |
| 对照：原样不塞 | 绿 | 绿 |

最后一次构建：槽 `c5eng`，日志 `build-c5eng.log`（mtime 2026-09-24 11:44:07 本机时间），三道门禁全过、966 / 966 全绿、零编译告警。

### 5.6 没做的与交出去的

- 可选 O1–O5 一概没做；`scripts/**`、`maps/**`、`data/**` 一个字节没碰。
- `docs/tech-debt.md` 新登 G-24（第二侧 0 瓶）、G-25（支线奖励不上屏）。
- 独立审查（cpp-reviewer）无 CRITICAL / HIGH；两条 MEDIUM 一条已在本文 1.2 的 id 一栏写明分工，另一条（`BattleLoader` 的波次上限与 `loadBattles` 调用处无 try/catch）是第 4 章留下的既有代码，不在本批改动里，原样转交。
