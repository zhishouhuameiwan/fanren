# 野外遭遇接口契约（八方旅人化改造 · P2b）

施工图 `docs/octopath-overhaul.md` 5.2。规则层 `rules::Encounter`（`src/core/rules/Encounter.*`）早就有，
本棒补上加载器、世界层入口、开战回填、第 3–5 章的内容与地名横幅的危险度。

## 0. 一眼看完

- **开关**：`Application::setEncountersEnabled`，**缺省关**；`src/main.cpp` 启动时打开。几百条无头测试不受影响。
- **数据**：`data/encounters/*.json`（表：遇上谁）＋ 地图上的 `encounter` 对象（区：多密、从什么时候起）。
- **入口**：`WorldScene::tryStep` 每迈成一步（没换图、没踩响剧情）调一次 `Application::stepEncounters()`。
- **开战**：「以回调开战」`Application::startBattle(battleId, onFinish)`；BattleScene 一行没改。
- **收场**：胜 → 编成奖励（BattleScene 发）；逃 → 什么也没有；负 → **不 game over**，气血 1、就地不动、旁白一句。
- **内容**：第 3 章谷外荒坡（第 4 章回来也遇得上）、第 5 章渡口河滩、第 5 章独霸山庄外林子。第 1–2 章不放，落日峰没有野地、不放。
- **存档**：计数器进存档，`kSaveVersion` 7 → 8（7→8 空迁移）。

## 1. 文件

| 东西 | 在哪 |
| --- | --- |
| 加载器 | `src/io/EncounterLoader.*`（`loadEncounterTables(dir, battles)`，battleId 当场对账） |
| 入口、回调开战、危险度 | `src/game/Application.*`（`stepEncounters` / `startBattle` / `dangerStars` / `encounterDangerStars` / `battleMenace` / `menaceStars`） |
| 世界层两处钩子 | `src/game/WorldScene.cpp`：`tryStep` 末尾一行、`describe` 填 `frame_.dangerStars` 一行 |
| 横幅那一行 | `src/game/WorldHud.*`：`PlaceBanner::setDangerLabel` / `dangerLine`，地名金线下一行 |
| 计数器 | `GameState::encounter`（`rules::EncounterState`，`Types.h` 末尾追加） |
| 门禁 | `tools/validate.py` 规则 27（表、区、编成）、规则 28（脚本 bgm） |
| 测试 | `tests/EncounterTests.cpp`；自检 `tools/validate_selftest.py` 的 Q 组 |

## 2. 遭遇表 `data/encounters/<名>.json`

```json
{ "id": "encounter_ch03_guwai", "name": "谷外荒坡", "dailyCap": 3,
  "entries": [ { "battleId": "be03_guwai_gulang", "weight": 3, "minRealm": "Mortal", "maxRealm": "QiRefining4" } ] }
```

- 字段都照 `rules::EncounterTable`；`stepsMin` / `stepsMax` / `dailyCap` 可选（区上写了以区为准，见第 3 节）。
- 加载器口径「写了就得写对」：`entries` 非空；`battleId` 在 `data/battles` 里；`weight` ≥ 1；
  境界写枚举标识符（与 `data/roles` 的 `realm` 同一套，对照表只在 `io::parseRealmName` 一处）、下不高于上。
- 早年的三张（`mountain_road` / `secret_realm` / `village_wilds`）照旧加载、**没挂在任何图上**：它们引用的是剧情战。
  哪天有人把它们挂上图，门禁规则 27 的编成那一截当场会报（id 不以 `be` 打头、输了会 game over）。

## 3. 遭遇区（地图 `encounter` 对象，map_spec 4.5）与触发规则

| 属性 | 必填 | 说明 |
| --- | --- | --- |
| `table_id` | 是 | 表的 `id` |
| `steps_min` / `steps_max` | 是 | 触发步数区间（**覆盖表上的**：区定疏密，表定遇上谁） |
| `daily_cap` | 否 | 覆盖表上的每日上限 |
| `require_flag` | 否 | **本作补的**：这面旗置起来之前区不数步、横幅不画危险度（谷外要等教学那一仗打完） |

`stepEncounters` 的步骤：开关关着、这一格不在开着的区里 → 什么也不做（**连步都不数**）；否则拿表拷一份、
填上区的步数与上限，调 `rules::step(table, state.encounter, realm, day, seed)`；摇中了放 `encounter` 音效、以回调开战。

- **计数器全局一份**，不按区分：每日上限防的是「这一天刷了几场」，按区分的话在两块区之间来回走就能翻倍。
- **种子**：`day × 今天第几场 × 这一段走了几步 × 所在格` 揉出来——同一份存档走同一条路必遇同一场。
- **只数迈成的那一步**：撞墙、踩响 enter 触发器、进门换图的那一步不数。
- 境界过滤照规则层：这个境界遇不上的条目不抽（第 5 章的韩立回谷外，一场也遇不上）。

## 4. 开战与回填（与切磋共用）

`Application::startBattle(battleId, onFinish)`：登记回调、压 `BattleScene`。BattleScene 收场照旧调 `completeCommand`；
`completeCommand` 看到「登记着回调、而且栈顶是一场仗」就改走回调（只用一次），不去惊动脚本。
认栈顶而不是只认登记：压在那一仗上面的邀战对话框收起时也会调到 `completeCommand`。
`replaceScene`（整个栈换掉）顺手把登记作废，免得下一场脚本开的仗被认成那一场。

回调拿到的 `CommandResult` 与脚本 `battle()` 拿到的是同一张（`battleWon`、`code` = "" / lost / escaped / enemy_fled）。
气血至少 1 放回、揭开的破绽记账、编成奖励，都在 `BattleScene::finish` 之前就做完了（战斗画面路的 `settle`）。

遭遇的回调（`finishEncounter`）：`code == "lost"` → 旁白 `ui.encounter.lost`；其余什么也不做。
**负了为什么就地不动、不挪回出生点**：挪过去的那一格可能正压在 enter 触发器上，替玩家踩响一段剧情；
计数器刚清零，离下一场至少还有 `steps_min` 步，够他走出野地或歇口气。

## 5. 各章放在哪

凶 = 敌方每个单位 气血 × 攻 之和（第 6 节）。「本章最轻的剧情战」由门禁现算（规则 27）。

| 图 / 区 | 表 | 开关 | 步数 | 编成（境界窗） | 凶 | 奖励（修为 / 碎银 / 药材） |
| --- | --- | --- | --- | --- | --- | --- |
| `ch03_guwai` / `encounter_huangpo`（荒坡 1,1 22×25，不含夹道与 `trigger_elang`） | `encounter_ch03_guwai` | `ch03.elang_done` | 18–40 | `be03_guwai_gulang` 恶狼 ×1（凡人–炼气四层） | 72 | 3 / 1 / — |
| | | | | `be03_guwai_yezhu` 野猪 ×1（同上） | 130 | 3 / 0 / 黄精 |
| | | | | `be04_guwai_langqun` 恶狼 ×3（炼气四–七层） | 216 | 4 / 2 / — |
| | | | | `be04_guwai_yezhu` 野猪 ×2（同上） | 260 | 4 / 1 / 黄精 |
| `ch05_dukou` / `encounter_hetan`（峡口以东河滩 16,5 16×20） | `encounter_ch05_dukou` | `ch05.kaipian` | 20–45 | `be05_dukou_yezhu` 野猪 ×2（炼气五–十层） | 260 | 5 / 1 / 清风草 |
| | | | | `be05_dukou_jianjing` 剪径贼 ×2（同上） | 392 | 5 / 3 / — |
| `ch05_dubashanzhuang` / `encounter_linzi`（庄外林子 1,1 18×28，不含墙根） | `encounter_ch05_linzi` | — | 18–40 | `be05_linzi_langqun` 恶狼 ×4（炼气五–十层） | 288 | 5 / 2 / — |
| | | | | `be05_linzi_yezhu` 野猪 ×3（同上） | 390 | 5 / 1 / 清风草 |

- 三张表的每日上限都是 3。本章最轻的剧情战：第 3 章 144（教学两头狼）、第 4 章 716（马六切磋）、第 5 章 432（夜宿遇狼）。
- 新角色：`ye_zhu` 野猪、`jianjing_zei` 剪径贼（带 `weapons` / `toughness` / `weaknesses`，破绽都含「拳」，门禁规则 25）。狼沿用 `wild_wolf`，
  于是教学那一仗、药圃管事打探揭开的「拳」在野外照样亮着。
- **不放的**：第 1–2 章（韩立还是孩子，门禁拦第 3 章以前的图）；落日峰（他自己那座偏峰，四面是崖、只有一条峰道，没有野地）；
  嘉元城郊（没有这张图）；渡口峡口以西（①号仗夜宿遇狼得是这一章头一仗）。
- **不许进任何野外表的角色**（门禁点名）：墨府尸傀（地窖里的东西）、独霸山庄庄丁与巡头（第 5 章校对 8.4：刺探那几天「一点波澜也没起」）。
- 开场文案：`ch03.battle.guwai_*.intro`、`ch04.battle.guwai_*.intro`、`ch05.dongqu.encounter_*`（节点 1）、`ch05.tancha.encounter_*`（节点 12）。
  第 5 章的 key 借已登记的场景前缀，`tests/Ch05LexiconTests.cpp` 的节点表不用改。

## 6. 危险度

- `battleMenace(battle)` = 敌方每个单位 `maxHp × attack` 之和（全部波次）：前者是要砍多久，后者是这段时间里挨多重。
- `menaceStars(凶)`：每多一颗，凶一倍——`< 200` 一颗；200–399 两颗；400–799 三颗；800–1599 四颗；≥ 1600 五颗（上限 `hud::PlaceBanner::kMaxStars`）。
- `Application::dangerStars()`：这张图上**此刻开着**的区里，**按当前境界遇得上**的条目中最凶的那一场折成星；
  没有区、区没开、一场也遇不上 = 0。第 3 章谷外一颗、第 4 章回谷外两颗、第 5 章两处野地各两颗。
- 横幅：地名金线下一行「凶险　★★☆☆☆」（界面词 `ui.world.danger`，实星 + 空星补足五颗），0 颗不画。
  那一行也画进离屏画布、随横幅同淡同现；字号 15，落在金线与目标框（`kBannerBottom`）之间，目标框不挪位置。

## 7. 存档

`GameState::encounter`（四个整数）写进 payload 的 `encounter` 对象。v7 → v8 空迁移（旧档 = 从没遇过）；
写了却写坏了（不是对象、缺字段、不是整数）拒读。**为什么要存**：不存的话读一次档就把当天的上限清零。

## 8. 顺带：脚本点播 BGM

- Lua：`bgm("bgm_night")` 点播（换图不撤）；`bgm("map")` 撤掉点播、放回这张图的曲子（地图写 `none` 就静音）。
  不许省参数（`bgm()` 看不出是要静音还是恢复）。命令 `CommandKind::PlayBgm`（追加在末尾），`kMapBgm = "map"`。
- `Application::worldBgm()`：此刻世界层该放的曲子（点播优先，其次地图曲）；`loadMap` 也按它放。点播**不进存档**。
- 第 5 章：`yeru.lua`（6a 三更翻墙）点上 `bgm_night`、不撤（teleport 进后园还在潜入）；`toutin.lua`（6b）开头再点一次
  （给在后园存档读回来的那一路）、听完撤回；`tancha.lua`（12c 刺探）寒毒那一关之后点上、演完撤回。`dengmen.lua`（7a 敲门报名）不点：不再是潜入。
- 门禁规则 28：每一处 `bgm("id")` 的曲子都在 `assets/bgm/` 里（或是 `"map"`）。

## 9. 门禁与测试

- 规则 27：表的形状；区的 `table_id` 指得到表、只许第 3 章起的图、`steps_min` ≥ 1、`daily_cap` 正整数；
  挂在图上的表里的编成：id 以 `be` 打头、`defeat_is_fatal` 假、`can_escape` 真、只有敌方、没有禁用角色、
  修为 ≤ 5、碎银 ≤ 5、掉落只许灵草每样一件、凶低于同章剧情战（不以 `be` 打头、不是识海）里最轻的那一场。
- 规则 28：脚本 bgm 的曲子存在。
- 自检 Q 组 13 条（一条对照 ＋ 12 条改坏一处必报）。
- `tests/EncounterTests.cpp`：缺省关、打开后区里一步摇中开战、关着一步不数、区没开不数、每日上限与过天恢复、
  负了不 game over（气血 1、就地、旁白）、胜了只发编成那一小份、危险度随境界与区开关、计数器存档往返、v7 老档、坏档拒读、
  脚本 bgm 换图不撤与撤回。

## 10. 未覆盖

- 遭遇战的**画面**（野猪、剪径贼的战斗精灵、`data/visual/battles.json` 的背景指派）归战斗画面路 / 美术路：
  编成里没写背景，运行时按所在地图退回；两个新角色没有专门的精灵。
- 战斗打完回到世界层时续哪一首曲子：`Application::worldBgm()` 已给出答案，战斗画面路换战斗曲后接回时请问它
  （否则夜探里打完巡庄那一仗会放回地图曲，直到脚本 `bgm("map")`）。
- 遭遇战前的转场（碎屏）归战斗画面路；本路只放了 `encounter` 音效（`docs/audio.md`）。
