# 路径行动接口契约（八方旅人化改造 · P 路）

> 施工图：`docs/octopath-overhaul.md` 5.1（玩法）、1.5（按键）、2.2（破绽类别）。
> 本契约由 P 路（规则层、加载器、门禁、第 1–5 章数据与文案）写定；**游戏层入口不在本路**，
> 由后续的 P2 按第 5 节接；切磋的编成由战斗路按第 6 节补建；破绽写进战斗由战斗路按第 7 节接。

---

## 0. 一眼看完

| 行动 | id 尾巴 | 开放 | 做成一次之后 | 要钱 | 效果 |
| --- | --- | --- | --- | --- | --- |
| 打探 | `_dating` | 第 1 章起 | **仍挂着**，再按是重看情报，效果不重发 | 否 | 说情报；头一回可附带揭破绽、给物件、置本路旗标 |
| 求购 | `_qiugou` | 第 2 章起 | 收起（每条只买一次） | 碎银 | 付钱、得物件 |
| 切磋 | `_qiecuo` | 第 3 章起 | 赢过收起；**输了不记、可以再来** | 否 | 开战；赢了发奖励（只发一次） |

不变量（门禁与测试都钉着）：

- 一条条目挂在（地图，NPC 对象名）上；什么时候挂着由 `when`（AND）/ `until`（OR）两张谓词表定，谓词写法与任务系统同一套。
- 能不能做：先看「阅历」（`GameState::realm` ≥ 条目门槛），再看钱（只有求购）。回绝的话**不点名境界**。
- 做成一次记在 `done_flag = chNN.path.<id>` 上；**路径行动只置 `chNN.path.*`，从不置剧情旗标**；剧情脚本也从不置 `chNN.path.*`。
- 每条都在本章收尾时收起（`until` 里必有 `chNN.done >= 1`）。
- 切磋输了不 game over；奖励只从条目发，编成自己的 `rewards` 必须全 0。
- 规则层全是纯函数：只回答「此刻挂着什么、做不做得成、做了会怎样」，改存档的是游戏层。

---

## 1. 文件与命名

| 东西 | 位置 / 形状 |
| --- | --- |
| 条目 | `data/pathactions/chNN.json`，**一章一个文件**，顶层 `id` 必须是 `pathactions_chNN`，`chapter` 与文件名的 NN 相同 |
| 文案 | `data/text/chNN_path.json`（`chNN` 前缀，禁词闸门自动覆盖） |
| 条目 id | `<npc 短名>_<dating｜qiugou｜qiecuo>[序号]`，小写字母、数字、下划线；尾巴必须与 `kind` 一致；本章内唯一 |
| done_flag | 恒为 `chNN.path.<id>`（加载器按 id 推出来比对，写错即拒） |
| 本路旗标 | `chNN.path.*`，全部登记在 `data/flags.json`（说明里写明「只由路径行动置，剧情脚本不碰」） |
| 文案 key（约定） | `chNN.path.<id>.text` / `.refuse` / `.deal` / `.poor` / `.win` / `.lose`。第 5 章的词表节点表按 `ch05.path.<id>.` 前缀登记（第 9 节），所以这个约定在第 5 章是硬的 |

---

## 2. 数据格式

### 2.1 文件顶层

```json
{
  "id": "pathactions_ch04",
  "name": "第 4 章路径行动",
  "chapter": 4,
  "note": "（可选）本章的口径：钱袋、门槛、跟哪几段剧情错开……",
  "entries": [ … ]
}
```

只认 `id / name / chapter / entries / note`；`entries` 非空。

### 2.2 条目：公共字段

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `id` | 是 | 见第 1 节 |
| `kind` | 是 | `inquire` / `purchase` / `challenge` |
| `map` / `npc` | 是 | 地图 id 与那张图上 `type = npc` 的对象名。说话人取那个对象的 `role_id` |
| `when` | 是，非空 | 谓词表，**全部**成立才挂出来。必须「锚在本章」：至少一条要求非零的 `chNN.*` 旗标，或 `ch(NN-1).done >= 1` |
| `until` | 是，非空 | 谓词表，**任一**成立就收起。必须含 `{"flag": "chNN.done", "op": ">=", "value": 1}` |
| `realm` | 否 | 阅历门槛，整数，与存档同一个数：凡人 0、炼气 N 层即 N、筑基 21–23、结丹 31–33。缺省 0 |
| `refuse_key` | `realm > 0` 时必填，否则不许写 | 阅历不足时对方怎么回绝（不许点名境界） |
| `done_flag` | 是 | 恒为 `chNN.path.<id>` |
| `text_key` | 是 | 打探：情报本身；求购：开价那一句（确认之前说）；切磋：邀战那一句（开战之前说） |
| `origin` / `note` | 否 | 原著出处与改编理由（给人看的，程序不读） |

谓词（与 `io/QuestLoader.cpp` 同进同退，`tests/PathActionTests.cpp` 逐条对拍）：

```json
{"flag": "ch04.liuxia", "op": ">=", "value": 1}
{"flag": "ch05.path.zhi_fengwu_yao", "op": "==", "value": 0}
{"item": "material_lingshi", "op": ">=", "value": 20}
```

`flag` 与 `item` 恰好一个；`op` 只认 `>=` / `==`；`value` 是整数；旗标 `>=` 至少 1、`==` 不许为负；物品只认 `>=` 且至少 1。

### 2.3 条目：各行动自己的字段

| 行动 | 字段 | 说明 |
| --- | --- | --- |
| 打探 | `reveal` | 可选。`[{"role": "<data/roles id>", "category": "<类别>"}]`，类别见第 7 节十种 |
| | `give` | 可选。`[{"item_id", "count" ≥ 1, "herb_age" ≥ 0}]`，头一回给 |
| | `set_flags` | 可选。只许本章的 `chNN.path.*`，不许是自己或别的条目的 `done_flag`，且必须有条目的 `when` / `until` 读它 |
| 求购 | `item_id`、`count` ≥ 1、`herb_age` ≥ 0（可选） | 卖的东西 |
| | `price` ≥ 1 | 碎银块数（物品 `material_lingshi`）。**文案里不写钱数**，价钱只活在这一个字段里 |
| | `deal_key` / `poor_key` | 成交那一句 / 钱不够那一句 |
| 切磋 | `battle` | 编成 id（`data/battles/`） |
| | `pending` | 可选布尔。`true` = 编成还没建（见第 6 节） |
| | `win_key` / `lose_key` | 赢了 / 输了（逃也算输）说的那一句 |
| | `reward` | `{"cultivation": ≥ 0, "items": [物件…]}`，至少给一样 |

表外字段一律拒收（拼错一个 `until` 会让条目挂到天荒地老，拼错一个 `refuse_key` 会让回绝变成一句 key 本身）。

### 2.4 例子

```json
{
  "id": "fengwu_qiugou", "kind": "purchase", "map": "ch05_mofu", "npc": "npc_mo_fengwu",
  "when": [{"flag": "ch05.dengmen", "op": ">=", "value": 1},
           {"flag": "ch05.path.zhi_fengwu_yao", "op": ">=", "value": 1}],
  "until": [{"flag": "ch05.done", "op": ">=", "value": 1}],
  "done_flag": "ch05.path.fengwu_qiugou",
  "text_key": "ch05.path.fengwu_qiugou.text",
  "item_id": "herb_zishen_cao", "count": 2, "herb_age": 20, "price": 40,
  "deal_key": "ch05.path.fengwu_qiugou.deal", "poor_key": "ch05.path.fengwu_qiugou.poor"
}
```

这一条由同一个人的打探 `fengwu_dating` 的 `set_flags: ["ch05.path.zhi_fengwu_yao"]` 解锁——施工图 5.1 说的「打探让一件隐藏物件出现」就是这个写法。

---

## 3. 规则层（`src/core/rules/PathActions.h`）

全部取 `const core::GameState&`，不改存档；core 层，不含 SDL / JSON / 文件 IO。

| 函数 | 回答什么 |
| --- | --- |
| `pathWindowOpen(a, s)` | 时段开没开：`when` 全成立且 `until` 无一成立。**空 `when` 永不开**（按 AND 恒真，等于一开局就挂着，必是漏写）；空 `until` 就是不收起 |
| `pathActionShown(a, s)` | 此刻挂不挂出来：时段开着；求购买过、切磋赢过的收起；`pending` 的切磋永不挂；打探做过仍挂着 |
| `pathActionsAt(all, s, mapId, npc)` | 某个 NPC 身上此刻挂着的，按 打探 → 求购 → 切磋、同类按 id 排。**返回指向 `all` 的指针，`all` 一变就失效，即取即用** |
| `pathActionVerdict(a, s)` | 能不能做：`RealmTooLow`（`reasonKey = refuseKey`）→ `NotEnoughMoney`（只有求购，`reasonKey = poorKey`）→ `None`。不看时段 |
| `pathActionEffects(a, s)` | 做这一条的效果清单（`PathEffect`，纯数据），次序就是施加次序，见下表 |
| `challengeResultEffects(a, won)` | 切磋收场：赢 → 说 `win_key`、加修为、给物件、记赢过；输（逃也算）→ 只说 `lose_key` |
| `pathRevealedWeaknesses(all, s)` | 已经做过的打探揭开了哪些破绽：按（roleId，category）排好、去重。**从存档就推得出来** |
| `pendingChallenges(all)` | 还标着 `pending` 的切磋，按（章号，id）排 |
| `kPathCurrencyItemId` | `"material_lingshi"`，与 `game::kSpiritStoneItemId` 相等（测试钉着） |

效果清单：

| 情形 | 清单 |
| --- | --- |
| 打探，头一回 | `Say(text)` → `RevealWeakness`… → `GiveItem`… → `SetFlag(set_flags)`… → `SetFlag(done_flag)` |
| 打探，做过了 | `Say(text)`（只重说，不重发） |
| 求购 | `Say(deal)` → `TakeItem(material_lingshi × price)` → `GiveItem(货)` → `SetFlag(done_flag)` |
| 切磋 | `Say(text)` → `StartBattle(battle)`；收场另见 `challengeResultEffects` |

**记做过永远在最后**：前面哪一步没施加成，这一条就不算做过。

---

## 4. 加载器（`src/io/PathActionLoader.h`）

### 4.1 三层

| 函数 | 查什么 | 谁用 |
| --- | --- | --- |
| `loadPathActionFile(path)` | 一个文件的形状：字段表、类型、表外字段、各行动的必填、谓词、done_flag 的名字、`until` 有本章收尾、`when` 锚在本章、文件名与顶层 id | 下两层 |
| `loadPathActionDir(dir)` | 整个目录的形状，外加一章一个文件；目录不存在 = 0 条；按章号排 | `loadGameData`（4.3 的补丁） |
| `loadPathActions(dataRoot, data)` | 目录的形状，再加引用：文案、物品、角色（`GameData`）、旗标已登记（`dataRoot/flags.json`）、地图与 NPC 对象存在（`dataRoot/../maps/`）、切磋编成存在且输了不死、编成不发奖励（`dataRoot/battles/`）；`pending` 反过来要求编成**还不存在** | `tests/PathActionTests.cpp` 在真树上跑；游戏层若想开机时多查一道也可以用 |

为什么 `loadGameData` 只走形状那一层：它只拿得到 `data/`，而 `ScriptApiTests`、`Ch03ScriptTests` 等好几组测试只把 `data/` 抄进临时根目录、不抄 `maps/`——在那里查地图，它们会全体红。引用由门禁（每次构建）与真树上的测试兜住，与任务系统「形状归加载器、引用归门禁」同一个分工。

### 4.2 门禁另查的（`tools/validate.py` 规则 26）

C++ 加载器查得到的那些门禁也查一遍（两边的字段表是同一张，改要一起改），另外只有门禁查得了的：

- 谓词里的旗标有人会置（剧情脚本 `flag.set`、各条目的 `done_flag`、`set_flags`）；`set_flags` 必须有条目读；登记了却没人用的 `chNN.path.*` 报错；剧情脚本置 `chNN.path.*` 报错；
- 同一 NPC 同一种行动的两条条目，时段必须**证明得了**不重叠（蕴含 / 矛盾推理，外加「`chM.*` 非零 ⟹ `chN.done ≥ 1`（N < M）」的章节公理）。公理的前一半前提有机器查（`check_path_axiom_premise`：`scripts/chNN/` 不置以后的章的旗标，`scripts/common/` 不置带章号的旗标）；后一半（第 M 章的脚本只在 `ch(M-1).done` 之后跑得到）要做可达性推演，**没有机器查**，靠各章挂点都锚在本章；
- 条目时段落在 NPC 在场的时段里：NPC 的 `visible_flag` 必须被 `when` 蕴含，`hidden_flag` 必须蕴含某条 `until`；
- 揭破绽的角色真的会上阵（在某场编成里，或是某条 pending 切磋的那个 NPC 的角色）；角色一旦有了 `weaknesses` 字段，揭的类别必须在里面；还没有的汇总成一条警告；
- `pending` 的切磋每条一条警告（「第 5 章收尾前必须清零」）。

负例在 `tools/validate_selftest.py` 的 P 节（39 例，每例都核「是因为它要测的那个原因才报的」）。

### 4.3 接进 `GameData`（补丁，交协调者落地）

本轮 `Types.h` 只许追加、`DataLoader.cpp` 不在本路白名单。下面两段已在 scratch 里对当前共享树编译过（`/W4`，零警告，含 `GameData` 的拷贝与移动）。

`src/core/model/Types.h`，`struct GameData {` 之前：

```cpp
// 路径行动（契约 docs/interfaces-octo-pathactions.md 第 4.3 节）。定义在本文件末尾那一段追加里；
// 这里先声明一句，GameData 才放得下它的 vector（C++17 起 vector 容许元素类型此刻还不完整，
// 只要用到成员之前补全——本文件读完时它就完整了）。
struct PathAction;
```

`GameData` 里 `std::vector<Quest> quests;` 之后：

```cpp
    // 路径行动，按章号排好（io/PathActionLoader.h 的 loadPathActionDir，只查形状；
    // 引用由门禁与 tests/PathActionTests.cpp 在真树上查）。
    std::vector<PathAction> pathActions;
```

`src/io/DataLoader.cpp`：`#include "io/JsonUtil.h"` 之后加 `#include "io/PathActionLoader.h"`；`loadGameDataImpl` 末尾 `return core::Result<GameData>::success(std::move(data));` 之前：

```cpp
    // 路径行动（契约 docs/interfaces-octo-pathactions.md 第 4.3 节）。只查形状：这里只拿得到 data/，
    // 好几组测试只把 data/ 抄进临时根目录；地图、编成、旗标登记的引用归门禁与 PathActionTests。
    core::Result<std::vector<core::PathAction>> pathActions =
        loadPathActionDir((dataRoot / "pathactions").string());
    if (!pathActions) return core::Result<GameData>::failure(pathActions.error);
    data.pathActions = std::move(pathActions.value);
```

另一个做法是不进 `GameData`，由 `Application` 在 `loadGameData` 之后自己调 `loadPathActions` 存一份——那样开机就多一道引用检查，代价是 `Application` 多一个成员、测试里造 `Application` 的地方都要有 `maps/`。推荐上面这个。

---

## 5. 游戏层怎么接（给 P2）

### 5.1 按键与入口

- `Key::Action`（E / Q）引擎已经接好（`src/engine/Engine.cpp` 的键位表），世界层还没用它。
- 在 `WorldScene::update` 里与 `Key::Confirm` 并排受理，**同一道闸**：脚本在跑、或在等命令回填时不受理。
- 面前那个人用 `WorldScene::visibleNpcAt(state, map, facingCell(state))` 取——不在场的 NPC 不该接住这一下。
- `rules::pathActionsAt(data.pathActions, state, map.id, npc->name)` 为空就什么也不做（头顶没有图标，玩家不会按）。

### 5.2 头顶图标

每个可见 NPC：`!pathActionsAt(...).empty()` 就浮一个小图标。全量四十几条，每帧算也不贵；要缓存就在旗标或背包变了之后重算。图标不区分三种行动也可以；要区分，看列表里有哪几种。

### 5.3 菜单流程

```
按 E → 列出 pathActionsAt（至多三项：同一个人同一种行动的时段门禁保证不重叠）
  选一项 → v = pathActionVerdict(a, state)
    v.block == RealmTooLow → 说 v.reasonKey（refuse），结束
    打探：施加 pathActionEffects(a, state)
    求购：说 text_key（开价）→ 选「买 / 不买」（界面上写物品名、件数、价钱）
          → 买：再取一次 verdict；NotEnoughMoney → 说 poor_key；None → 施加 pathActionEffects
    切磋：施加 pathActionEffects（先说邀战，再开战）；收场施加 challengeResultEffects(a, won)
```

- 菜单项的字：「打探」「求购」「切磋」；求购可以带上「物品名 × 件数 · N 块」——**价钱由界面从 `price` 取**，文案里不写（门禁与测试钉着）。
- 界面上任何地方都不出现境界名；阅历不足只有那一句 `refuse_key`。

### 5.4 效果怎么施加

| 效果 | 施加 |
| --- | --- |
| `Say(textKey)` | 说话人 = 那个 NPC 对象的 `role_id`（与脚本 `talk(role, key)` 同一个口径，文案都是第三人称转述），正文 `data.lookupText(textKey)`，记进对话回看 |
| `RevealWeakness(reveal)` | **此刻就写**：`state.learnWeaknesses(roleId, core::categoryFromName(category))`，旁白补一句「记下了某某的破绽：X」（`ui.path.reveal.lead` ＋ 角色名 ＋ `ui.path.reveal.tail` ＋ 类别名）。`pathRevealedWeaknesses(all, state)` 仍能从 `done_flag` 推回同一个答案，**存档不必加字段** |
| `GiveItem(item)` | `state.addItem(itemId, count, herbAge)` |
| `TakeItem(item)` | `state.removeItem(itemId, count)`（verdict 已保证够；返回 false 就是调用方没先问 verdict） |
| `SetFlag(flag)` | `state.setFlag(flag)`（置 1） |
| `StartBattle(battleId)` | `Application::startBattle(battleId, 回调)`（以回调开战，`docs/interfaces-octo-encounters.md` 第 4 节）；收场 `won` = `CommandResult::battleWon`（只认 `BattlePhase::Won`，逃也算负），回调里施加 `challengeResultEffects(a, won)`，说话人照旧 |
| `GainCultivation(amount)` | 与 `grantBattleReward` 同口径：**只加修为、不动境界**，饱和加法 |

顺序照清单，一条都不跳。同一批压的几层（对话、旁白、那一仗）**按清单次序演**：场景栈后进先出，`applyPathEffects` 落地前把这一批倒过来——邀战说完才开打，情报说完才报「记下了破绽」。

### 5.5 存档

不加字段：做没做过在 `chNN.path.*` 旗标上，钱与物件在背包里，揭开的破绽由旗标推得出来。

### 5.6 与任务 / 目标链的关系

- 路径行动**不推进**主线目标链、不置剧情旗标；目标链与主线任务不许以 `chNN.path.*` 为完成条件（它们是可选内容，漏做不该卡主线）。
- 支线任务可以读 `chNN.path.*`（例如「向某人打探过」作为某一步的完成条件），写法照任务系统的谓词。
- 施工图 5.1 的「让隐藏物件出现」「给商店折扣旗标」都用 `set_flags` 置一面 `chNN.path.*`，由读它的那一方（条目的 `when`、地图对象的 `visible_flag`、商店的条件）去用。**现状**：门禁「`set_flags` 必须有人读」只认路径条目自己的 `when` / `until`；第一次有地图对象或商店去读它时，把那一方加进这条规则的读者里。

### 5.7 落地状态（P2a 交付时；P2b 收尾见本节末）

**已落地**（切磋以外的全部）：

| 东西 | 在哪 |
| --- | --- |
| 世界层两个钩子 | W 路留的 `WorldScene::setPathActionHooks(probe, opener)`（`docs/interfaces-octo-world.md` 第 6 节），在 `Application::init` 末尾接上；WorldScene 一行没改 |
| 查询 | `Application::pathActionsFor(npcName)` = `rules::pathActionsAt(data.pathActions, state, state.mapId, npcName)` |
| 气泡 | `game::pathBubbleFor(list)`（`PathActionScene.h`）：空 → `None`；只有一种 → 那一种；几种都有 → `Generic`。probe 就是 `pathBubbleFor(app.pathActionsFor(npc.name))` |
| 入口 | `Application::openPathActions(npcName)`：一条都没挂着返回 false、什么也不压；否则压 `PathActionScene`，说话人取那个 npc 对象的 `role_id` |
| 菜单 | `src/game/PathActionScene.*`：第 5.3 节的流程；逻辑动作 `choose(app, row)` / `answer(app, buy)` 与按键解耦（无头测试直接调） |
| 效果 | `Application::applyPathEffects(effects, speakerRole)`：Say / GiveItem / TakeItem / SetFlag / GainCultivation 照 5.4；TakeItem 扣不成就停、返回 false |
| 说话 | `Application::sayAs(roleId, key)`：记 `spokenKeys`、进对话回看、压对话框。脚本 `talk` 的派发也改走它，两边一个口径 |
| 用词 | `data/text/ui.json` 的 `ui.path.*`（打探 / 求购 / 切磋 / 买 / 不买 / 按键提示）；确认面板的盘缠借 `ui.menu.money` 与 `Wording::currencyAmount` |
| 截图口 | `--scene path:<npc 对象名>[:<旗标>,…]`：先把列出的旗标置 1 再按 E 那条路开菜单（第 1、2 章没有停在条目时段里的存档）；对象名空着（`path::<旗标>…`）只置旗标、拍行走画面上的气泡 |
| 测试 | `tests/PathActionFlowTests.cpp`（6 条，真数据第 2 章三条：打探给一次可重看、求购扣钱给货收起、钱不够与不买什么都不动、阅历不足回绝且不点名境界、界面固有字与五章菜单行不含境界名、钩子接通：气泡与 E 键） |

P2a 自己定的（契约没写到的）：

- 做完一件（说完情报、成交、钱不够、回绝、不买）菜单就收起，不回到列表；想做下一件再按一次 E。
- 「买 / 不买」里按取消 = 不买；不买一句不说。
- 确认面板上除了价码还写一行「盘缠　碎银 N 块」（按阶段说碎银 / 灵石）；价码那一行只写「N 块」，不写钱名。
- 菜单与确认面板压全屏纱（与其它模态面板一样），横向居中、底边贴屏幕下沿；对方说话时这一层不画。

~~留给战斗路合回之后~~ 的两项，**P2b 已落地（2026-09-27）**：

1. `RevealWeakness`：打探那一刻写进 `GameState::knownWeaknesses`（契约 5.4 两条路里取「此刻就写」：玩家打探完回头就去打，
   开战时才并进去的话，中间看状态、存档都查不到这一笔），旁白一句「记下了某某的破绽：X」，情报先说、旁白后说。
2. `StartBattle` 与切磋收场：**以回调开战**——`Application::startBattle(battleId, onFinish)` 登记回调、压 `BattleScene`；
   BattleScene 收场照旧调 `completeCommand`，`completeCommand` 认出「登记着回调、栈顶是一场仗」就改走回调，不惊动脚本。
   **BattleScene 一行没改**。回调里施加 `challengeResultEffects(a, won)`：胜 → 说胜、条目奖励、记赢过、条目收起；
   负（逃也算）→ 只说负、不记、可以再来。输了不 game over：没有脚本去读 `"lost"`，气血由 BattleScene 按至少 1 放回。

P2b 自己定的：

- `applyPathEffects` 改签名，多带条目本身（`applyPathEffects(action, effects, speakerRole)`）：收场回调要拿它算胜负那一套。
  指针指进 `data().pathActions`（开机读一次、运行期不变）。
- 同一批压的场景倒过来落地（见 5.4 末）。没有这一条，邀战那一句会在仗打完之后才冒出来。
- 揭破绽那一句是旁白、不进 `spokenKeys`（那是 NPC 说的话的账），进对话回看。
- 测试 `tests/SparFlowTests.cpp`：四场切磋各「先输一次、再赢一次」（邀战先于开战上屏；输：不 game over、气血 ≥ 1、什么也不发、
  条目还在；赢：修为增量恰好是条目那一份、物件到手、记做过、条目收起）；揭破绽写进已知破绽、旁白次序、存档往返。

---

## 6. 切磋与编成

### 6.1 编成的要求

- `defeat_is_fatal: false`（输了不 game over，`BattleScene` 按 hp 至少 1 放回来）；
- `rewards` 全 0（修为、碎银、掉落都是 0）：奖励只由条目发一次，编成若也发，赢一场拿两份；
- 建议 `can_escape: true`；逃走按输算，不记、可以再来；
- 我方照章节当时的队伍（`GameState::party`，曲魂随不随队由剧情定）。

### 6.2 pending

编成要按战斗路的横版格式建（施工图第 2 节），本路不新建 `data/battles` 与敌方 `data/roles`，所以这几条标着 `"pending": true`：

- 规则层：`pending` 的切磋**永不挂出来**；
- 加载器与门禁：`pending` 时编成必须**不存在**（建好了还挂着 pending，这条就永远不上屏）；每条一条警告；
- 测试 `PathActionData.PendingChallengesMustBeBuiltOnceTheSideScrollBattlesLandBack`：每次把清单念出来；**一旦 `data/battles` 或 `data/roles` 里出现 `weaknesses` / `toughness` 字段（横版格式合回来了），还挂着 pending 就转红**；
- 第 5 章收尾前必须清零。

建好一场：在 `data/battles/` 建编成 → 条目里删掉 `"pending": true` → 门禁与测试自动改查 6.1 的三条。

### 6.3 待建的四场

| 编成 | 条目 | 地图 / NPC | 对手 | 条目发的奖励 | 说明 |
| --- | --- | --- | --- | --- | --- |
| `b04_qiecuo_maliu` | ch04 `maliu_qiecuo` | `ch02_wairentang` / `npc_tongmen_maliu` | 马六 `tongmen_maliu` ＋ 卢春 `tongmen_luchun` | 修为 25、`weapon_zhishi_peidao` × 1 | 阅历门槛炼气五层。两人现有数值是闲聊 NPC 的（气血 30、攻 5），要按「炼气五层的韩立打两个外刃堂弟子」重调；文案是缠布木刀，攻击类别宜为刀 |
| `b04_qiecuo_feiyu2` | ch04 `feiyu_qiecuo` | `ch04_getang` / `npc_li_feiyu` | 厉飞雨 `li_feiyu` 一人 | 修为 30 | 节点 5 演武台那一场（`b04_qiecuo_feiyu`，40）之后的复赛，数值照那一场 |
| `b05_qiecuo_huyuan` | ch05 `huyuan_b_qiecuo` | `ch05_nancheng` / `npc_huyuan_b` | 墨府护院 `mofu_huyuan` × 4（齐眉棍） | 修为 20、`pill_jinchuang_yao` × 1 | 燕歌讨教（`b05_yange_qiecuo`，30）是本章切磋的上限 |
| `b05_qiecuo_lingban` | ch05 `huyuan_a_qiecuo` | `ch05_nancheng` / `npc_huyuan_a` | **新角色**护院领班 `mofu_huyuan_lingban`（要新建 `data/roles`） | 修为 25、碎银 20 块 | 节点 9 之后才挂；强度介于护院与燕歌之间 |

测试 `PathActionData.NoSparPaysMoreThanItsChaptersStorySpar` 钉着：每场切磋的修为不超过同章剧情切磋（第 4 章 40、第 5 章 30）。

---

## 7. 破绽（交战斗路对账）

类别（施工图 2.2）：数据里写**中文名**——剑、刀、拳、暗器、毒、金、木、水、火、土，与战斗数据、存档同一张表（`src/core/model/AttackCategory.h`；门禁 `PATH_WEAKNESS_CATEGORIES` 是它的镜像）。本路最初用的是英文标识（sword/fist/fire……），战斗合回时（2026-09-26）统一成了中文名；四场切磋编成同时建好，`pending` 已全部去掉。

本路打探揭的破绽（角色的 `weaknesses` 字段落地时，这些类别必须在里面，否则门禁报错）：

| 章 | 条目 | 角色 | 类别 | 情报的说法（大意） |
| --- | --- | --- | --- | --- |
| 3 | `guanshi_dating` | `wild_wolf` | 拳 | 狼的头硬、腰软，棍子石头砸在腰上就趴下 |
| 4 | `feiyu_dating` | `yelangbang_mazei` | 剑 | 马贼的刀只会劈，劈下来腋下全空，软剑往那儿递 |
| 4 | `wangjuechu_dating` | `yelangbang_mazei` | 毒 | 夜里爬到东坡的人头一件找水，那口井供着半山 |
| 4 | `maliu_dating` | `yelangbang_toumu` | 拳 | 油浸皮甲让刀剑打滑，拳头棍棒隔着甲照样伤人 |
| 4 | `yaofan_dating` | `jinguang_shangren` | 火 | 金光挡得住刀枪，人却怕热 |
| 5 | `chuanjia_dating` | `matou_dashou` | 拳 | 码头汉子凭蛮力，挨揍的本事不大 |
| 5 | `jiefang_dating` | `tiequanhui_quanshi` | 剑 | 铁拳会拳头硬，碰上带刃的就缩手 |
| 5 | `huyuan_a_dating` | `wu_jianming` | 火 | 吴公子的剑一递空就收势慢，怕隔着距离耗 |
| 5 | `huyuan_d_dating` | `mofu_shigui` | 火 | 地底下封着的东西都怕火 |
| 5 | `yange_dating` | `yan_ge` | 火 | 近身谁也不怕，隔着两丈只能干挨 |
| 5 | `luren_dating` | `shen_zhongshan` | 毒 | 沈帮主什么都防，只喝院里自家的酒 |

- **欧阳飞天（`ouyang_feitian`）一条也不揭**，文案也不提前说「刀剑难伤」「霸王甲」（协调者拍板，第 5 章校对 8.2 / 8.4：他只能以剑符取首级，破绽由战斗路定为只含「金」）。测试 `PathActionData.ChapterFiveLeavesTheManorAndItsMasterToTheStory` 钉着。
- 上表是本路按原著与文案定的提议；战斗路配平衡时若要改某个角色的破绽，改这边的条目（`reveal`）与文案，别只改一边——门禁会在两边对不上时报错。

---

## 8. 门禁与测试一览

| 位置 | 管什么 |
| --- | --- |
| `tools/validate.py` 规则 26（`check_path_actions`） | 第 4.2 节全部；每次构建最先跑 |
| `tools/validate_selftest.py` P 节 | 39 例：干净探针不误报、每条规则一条负例、文件级三例 |
| `tests/PathActionTests.cpp` | 规则层 16 条；加载器与引用 14 条；真数据的设计意图 16 条（见下） |
| `tests/Ch05LexiconTests.cpp` 节点表 | 第 5 章路径行动文案按节点登记（第 9 节） |

真数据的设计意图（数字写死，出处写在用例旁边）：

- 五章都读得到、每条文案查得到；三种行动按章开放；
- 阅历门槛 ≤ 韩立那一章够得着的境界：第 1 章凡人（本章打探一律不设门槛）、第 2、3 章炼气三层、第 4、5 章炼气八层；这张表的来处另有一条从剧情脚本（`realm.cap` / `realm.advance`）再推一遍的漂移检查；
- 价钱 ≤ 那一章最省钱的主线路上、条目挂出来之后手里至少有的碎银：第 2 章 6 块（且必须挂在 `ch02.duan2_start` 之后）、第 4 章 28 块、第 5 章 87 块；另有一条漂移检查核它们的来处（`scripts/ch02/maiyao.lua` 给的钱、`tests/fixtures/ch03-end-*.sav` ≥ 28、`ch04-end-*.sav` ≥ 112）；
- 切磋奖励不压过本章剧情切磋；打探文案 40–150 字；回绝不点名境界；文案不写钱数；第 1、2 章不提修仙、灵根、法术；
- 第 5 章：章内先后按 `when` 推出的节点判（寒毒、五色门、独霸山庄第 9 节起，惊蛟会第 4 节起，太南谷、太南山第 11 节起），并与节点表逐行对账；欧阳飞天、燕歌、吴剑鸣、清灵散、养精丹、庄丁的拍板；
- 第 1–4 章不给、不卖、不赏养精丹与黄精、紫参（第 4 章药账）；哪一章都不碰那块牌子；
- 揭破绽的角色真的会上阵；pending 的待办闸门（第 6.2 节）。

---

## 9. 第 5 章的词表登记

`tests/Ch05LexiconTests.cpp` 要求每条 `ch05.` 文案都登记到一个节点。第 5 章每加一条路径行动，在它的节点表里加一行：

```cpp
{"ch05.path.<id>.", <节点>},
```

节点 = 条目 `when` 里剧情旗标所属节点的最大值（`docs/ch05-design.md` 3.1 各挂点的 `set_flag`；`ch05.path.*` 旗标不算，它由更早挂出来的那一条置）。尾巴上的点是有意的：没有它，`jiefang_dating` 那一行会悄悄认领 `jiefang_dating2` 的文案。`PathActionData.ChapterFiveSaysNoWordBeforeTheNodeItsWhenOpensAt` 按数据推一遍节点，与这张表逐行对账：多一行、少一行、节点不对都红。

---

## 10. 各章内容（本棒交付时）

| 章 | 打探 | 求购 | 切磋 | 文案 | 汉字 | 阅历门槛 | 钱袋 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 9 | — | — | 9 | 816 | 全部不设 | — |
| 2 | 8 | 1（金疮药，6 块） | — | 12 | 885 | 1 条（炼气二层） | 6 |
| 3 | 3 | — | — | 3 | 261 | 不设 | — |
| 4 | 6 | 1（青风草，24 块） | 2（pending） | 17 | 1112 | 2 条（炼气五层、七层） | 28 |
| 5 | 8 | 2（紫参 40 块、金疮药 30 块） | 2（pending） | 20 | 1386 | 不设 | 87 |

每条条目的 `origin` / `note` 写着原著出处与改编理由。

---

## 11. 未覆盖（交接）

- 游戏层入口（第 5 节）：**全部落地**。P2a 落了切磋以外的全部；P2b（2026-09-27）落了揭破绽写入与切磋的以回调开战、收场回填（5.7 末）。
- 破绽写进战斗、`weaknesses` 字段对账：战斗路已合回（`GameState::knownWeaknesses`，存档 v7）；第 7 节是当时的提议表。
- 四场切磋编成与新角色 `mofu_huyuan_lingban`：已建好、`pending` 已去掉（6.2 清零）。
- `GameData` 接入：协调者已按第 4.3 节落补丁。
- `Types.h` 末尾的追加与 `validate.py` 规则 26：已于 2026-09-26 与副本三方合并。
- 以回调开战同样给野外遭遇用：`docs/interfaces-octo-encounters.md`（规则 27、28 也在那一份里）。
- 切磋战的画面（战斗背景指派 `data/visual/battles.json`、马六等人的战斗精灵）归战斗画面路 / 美术路，本路没碰。
