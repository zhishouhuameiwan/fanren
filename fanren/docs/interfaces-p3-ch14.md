# P3 第 14 章引擎增补契约

> 本文是**契约**：引擎方照它写判据，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-10-13 · 依据第 14 章研究笔记（`notes/ch14/research.md`）C 表（境界锚点）、D 表（引擎现状）、E 表（拍板）、F 表（节点）、G 表（地图与在场窗口）、I 表（编成）、J 表（旗标）、L 表（物品 / 法术）、N.4（teleport）、O 表（收束约定与开放问题）；格式沿 `docs/interfaces-p3-ch13.md`。**全作末章**：没有第 15 章。
> **本章 0 个代码项**：笔记 D 表逐行核过（2026-10-13 grep），本章要的引擎行为全部已有，`src/` 一行不改。**E1–E3 是验证项**：把此前没有一章跑过的三种局面——结丹后期即本作上限、主角缺席而友军顶着「韩立」名牌的封法局、结局卡 ＋ 末章收束 ＋ 通关读档——钉在 `tests/Ch14EngineTests.cpp`，红了先改笔记、再改本文、最后才动 `src/`。
> **编号对照**：本文 E1 ＝ 笔记 D 表「境界推进」「冲关面板」两行，E2 ＝「编成」「法术形状」两行，E3 ＝「结局卡与章节卡」「标题继续旅程」「目标链」三行（笔记 D 表末称契约 E1–E3）；笔记的拍板号写作「E 表 N」、开放问题写作「O-N」，与本文 E 项不是一套；施工图 10.1 其余各项用 C（协调者 / 内容路一行）、T（测试路）、B（平衡路）前缀（笔记 O.5），不进本文编号。
> 门禁：引擎路不改 `tools/`。规则 25 的 `CHAPTER_MEANS[14]` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER = 14`、selftest 合成第 15 章探针归协调者（第 6.2 节）。
> **不做**（笔记 E 表已拍板）：「封法」字段（E 表 9：④⑤ 走 `hero_absent`）；「顶峰」存档字段或新境界档（E 表 4、C.1：33 即顶峰）；拍卖 / 秘市（E 表 22）；降灵符配方（E 表 23，`guaranteed` 全仓仍一张）；风雷翅先手 / 位移（E 表 10：战斗里只削架势，世界里只走脚本捷径）；按 spawn 名落点；`party.*`（E 表 30）；遭遇区；新 Lua 命令；存档字段（仍 v9）；`breakthroughPillBonus`（仍恒 0）。
> **本章在第 12、13 章集成之后开工**：第 12 章的 `magic_bixie_shenlei`、`yuan_yao_jiedan`、`story_*` 旧物，第 13 章的 `magic_ji_wuxinghuan`、`yun_tianxiao`、`story_tongpian`、`story_wuse_tonghuan` 等（2026-10-13 工作树里尚未落地）都当成已有的用。`ch13-end-*.sav` 尚未生成——本文判据的夹具一律用内存状态，或 `tests/fixtures/ch11-end-first.sav`（`realm 31 / realmCap 31 / formerRealm 22`）经探针脚本 `realm.advance(realm.CORE_MID)` 推到 32，判据不依赖章节进度。

---

## 0. 各引擎项现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | n03 一句 `realm.advance(realm.CORE_LATE)` 把 32 → 33（`scripts/ch14/houqi.lua` 恰 1 处）；章末「结丹后期顶峰」＝ 33 ＋ 面板满条，不另立数据（笔记 C.1、E 表 4） | `RealmAdvance`（`src/game/Application.cpp` 第 1271 行起）：目标 < 当前 → `ok false`、`code "not_higher"`；**目标 == 当前 → `ok true`、只 `raiseRealmCap`、别的不动**（第 1284–1308 行注释，与 `PartyAdd` 同口径）；否则抬上限、改 `realm`、`liftToFloor` 按差额抬气血法力、小瓶容量取 `max(capacity, bottleCapacityFloor)`。`cultivationNeeded`（`Realm.cpp` 第 142–149 行）`CoreMid 7000`、`CoreLate` 走 default 返回 `−1`（「结丹后期为本作上限」）；气血 / 法力 `CoreLate 1300 / 860`（第 219、236 行）；攻 / 防 `102 / 82`（第 259、277 行，「暂定」）。`BreakthroughBlock::SeriesMax`（`Cultivation.h` 第 55 行）先于 `StoryCap` 判；`CultivationScene.cpp` 第 427–431 行冲关行 `enabled false`、理由 `words.pushCapped`（修仙词表「已至本作境界之极」），第 713–719 行修为条 `capped` 画满并接 `cappedNote`（「（已至境界之极）」）。`api.lua` 第 218 行 `CORE_LATE = 33`。**全作还没有一张存档到过 33** |
| E2 | ④ `b14_yinming_tihun`、⑤ `b14_baofeng_feiyao` 两场 `hero_absent true`；⑤ 友军 `hanli_jueling` 名牌「韩立」、`magics []`、`weapons ["剑"]`；银猿 `magic_tihun_yinguang` `target "all"`；④ `defeat_is_fatal false`、`can_escape true`（笔记 E 表 9、I 表） | `BattleLoader.cpp` 第 84–91 行 `hero_absent` 非布尔即拒；第 5 章契约 2.1–2.2（已并入 `docs/octopath-battle.md`，开关与口径不变）：为真时不建 `hanli`、不带队员、物品栏为空、第 0 波须有友军、只按阵营判胜负、战后 `GameState` 气血法力一字节不动；`BattleScene.cpp` 第 334 行只在建了主角时 `hero.name = app.speakerName("hanli")`；`DataLoader.cpp` 第 403–412 行 `target "all"` 只许伤人法术、不与 `reveal / stagger` 同写；`battle()` 回 `won, code`（`api.lua` 第 54–57 行；`BattleScene.cpp` 第 1031 行 `Lost → "lost"`）；规则 25 对 `hero_absent` 只认友军兵刃与 `BATTLE_EXTRA_MEANS` 里核得到的 `role_magic`，不继承韩立通表（`validate.py` 第 2578–2582 行）。**「友军名牌与主角同名」「`hero_absent` 局里友军放全体法术」都没有一章跑过**（笔记 O-2） |
| E3 | n29 `guixiang.lua` 先 `ending("ui.ending.ch14.title", "ch14.ending.body")`、最后一条可执行语句 `flag.set("ch14.done")`；通关后留在 `ch14_minzhou` 行走；标题「继续旅程」读通关档（笔记 E 表 28–29） | `Application.cpp` 第 1080–1083 行 `Ending` → `pushScene(ChapterCardScene::ending(...))` 并 `return false`（脚本停到玩家确认）；`ChapterCardScene.cpp` 第 87–103 行 `cardsForFlagChange` 只认 0 → 1，表中末章排 `{Closing, n}` ＋ `{ToBeContinued}`，否则 `{Closing, n}` ＋ `{Opening, n+1}`；`ui.ending.continue_hint` 已在 `data/text/ui.json` 第 20 行；`TitleScene.cpp` 第 173 行 `continueJourney(quickSavePath())`；`Objectives.h` 第 24 行「全部完成时返回 nullptr」。**全仓 `scripts/` 里 0 处调过 `ending(`**（2026-10-13 grep；第 4、6、7、11 章脚本都明写「不 ending()」）——结局卡与章节卡在同一脚本里先后出现、末章通关档读回，都没跑过 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。本批三段**都不改**；E1–E3 各自只有判据。

**查过、本章不需要新做的**（笔记 D 表「缺什么：无」那几行）：`advance_days` 单次 ≤ 3650（`kMaxScriptAdvanceDays`，超了静默夹；本章每句 ≤ 360）、每日回一成气血；小瓶结丹档 3 日一滴、容量 9（本章 0 处 `bottle.*`）；`magic.learn / forget / knows`（`api.lua` 第 333–352 行；`forget` 先例 `scripts/ch05/shangyue.lua` 第 42 行）；`teleport(map_id, x, y)` 落 default spawn、`x`、`y` 不同时为 0 才改坐标、落地**不判**脚下触发器（只有 `WorldScene::tryStep` 迈步时判；同图带坐标先例 `scripts/ch05/jianmian.lua` 第 69 行）；`effect "stagger"`（第 7 章先例）；多波次、`charge`、`actions`、`killable_by` 非空、`can_escape`、`drops`、`backdrop`、`intro_key`；`item.count` 与 `take` 看返回值；NPC `visible_flag / hidden_flag`；facility 只读 `require_flag`；路径行动 `when / until`；Lua 只读查询 `magic_knows / realm_value / item_count / flag_get`。

**不是引擎的事、写在这里免得找错人**：结丹后期攻防锚点 102 / 82 用 ①②③ 标定、④⑤ 友军数值与 ④「输了重来」的手感归平衡路（施工图 B1、B2，用 `tests/BattleHand.h` 量）——**平衡路只改编成与 role 的数值，`Realm.cpp` 的数字不动**（E1 判据以字面量钉它们；真要改，先改本文 1.3）；`Realm.h` 第 175–186 行（含第 177 行「结丹中期、后期仍待对应章节标定」那句与其后的样本表）与 `Realm.cpp` 第 258–259、276–277 行的「暂定」注释改「结丹中期未经战斗标定（全作 0 场）、结丹后期第 14 章标定」归协调者，**只改注释**（笔记 O-5、C7）。`feng_xi.json` 只改 note、数值不动（笔记 B19）。

---

## 1. E1 结丹后期档（验证项，不改代码）

### 1.1 一句话口径

**本章的境界只经一处：n03 一句 `realm.advance(realm.CORE_LATE)` 把 `realm` 32 → 33、`realmCap` 顺带抬到 33，气血 / 法力上限到 1300 / 860（按差额上抬、不补满），`formerRealm` 仍 22，小瓶容量仍 9、液滴不变；此后面板「冲关」报 `SeriesMax`（不是 `StoryCap`、不是 `NotEnough`），不掷骰、修为一分不扣，修为条画满并接「已至境界之极」那句注。同一句再执行一次返回真、状态不变（目标 == 当前「结果已成立」）；目标写小（`CORE_MID`）返回假、`code "not_higher"`。「顶峰」不另立任何数据字段，只在结局卡文案里写。** 任一处与此不符，才改 `src/`（先改笔记 C.1、再改本文）。

- 规则层（`rules::cultivationNeeded / breakthroughBlock / tryBreakthrough`、`Realm.cpp` 各档数值）、加载器（存档 v9，无新键）、游戏层（`Application` 的 `RealmAdvance`、`CultivationScene`）：**都不改**。
- **与笔记的出入（按引擎收口；笔记已于 2026-10-13 整改时回写）**：笔记 D 表契约 E1 末句旧稿「再 `realm.advance(realm.CORE_LATE)` 一次返回假」不成立——引擎对「目标 == 当前」回 `ok true`（第 1286–1290 行注释写明是有意的，剧情回放、读档重跑不走失败分支）。本文改成「返回真、状态不变」，负向改用 `CORE_MID`。笔记 C.1「面板写『已至上限』」以词表原话为准：修仙词表 `pushCapped`「已至本作境界之极」、`cappedNote`「（已至境界之极）」；判据比词表常量，不抄字面。
- 内容路要知道的：`realm.advance` 看返回值，`false` 只会是编号写错——`talk` 一句、照走，**不许静默吞掉**；`realm.cap`、`realm.demote` 全章 0 处；`magic.learn("magic_jianying_fenguang")`（`needRealm CoreLate`）写在 `realm.advance` **之后**——**这只是写法约束、让剧情次序对得上**：`MagicLearn` 只查 id 存不存在、不看 `needRealm`（`Application.cpp` 第 1239–1258 行，已会也回真），境界只在战斗施法时由 `checkCast` 判（`BattleAction.cpp` 第 131 行），先学后升引擎照样收下。learn 回假只会是 id 拼错（`no_magic`）。

### 1.2 数据形状（内容路 C 写；只是形状，文案 key 由内容路 C 定）

```lua
-- scripts/ch14/houqi.lua（n03），本章唯一一处境界调用
local ok = realm.advance(realm.CORE_LATE)
if not ok then talk("", "<文案 key>") end
-- 之后才 magic.learn 剑影分光、辟邪神雷（各看返回值，false 只换一句照走）
```

### 1.3 判据（`tests/Ch14EngineTests.cpp`，引擎路写；每条直接读 `GameState` / `rules::` 返回值 / 场景菜单，不经 `Driver`）

夹具 `Ch14Fixture` 照 `Ch13Fixture`：headless `Application` 载入 `tests/fixtures/ch11-end-first.sav`，探针脚本先跑 `realm.advance(realm.CORE_MID)`，先验 `realm 32 / realmCap 32 / formerRealm 22 / maxHp 900 / maxMp 620 / bottle.capacity 9`；探针脚本写法照第 11 章契约 2.3 的偏差记录（`TempDir/scripts/` 重挂 scriptRoot、`startEvent` 后泵帧）。

1. **剧情升境**：先把 `hp` 砍到 450、`mp` 砍到 310，探针脚本 `local ok, code = realm.advance(realm.CORE_LATE)`，把 `ok` 写进探针旗标 → 旗标 1、`realm == 33`、`realmCap == 33`、`formerRealm == 22`、`maxHp == 1300`、`maxMp == 860`、`hp == 850`、`mp == 550`（差额 400 / 240，不是补满）、`bottle.capacity == 9`、`bottle.drops` 不变；`rules::realmAttack(Realm::CoreLate) == 102`、`rules::realmDefence(Realm::CoreLate) == 82`（字面量写死，不从实现推导）。
2. **重跑与写小**：同一状态再跑一次 `realm.advance(realm.CORE_LATE)` → 探针旗标 1、`GameState` 上述各字段逐一不变。**负向**：`realm.advance(realm.CORE_MID)` → 旗标 0、`code == "not_higher"`（第二个返回值写进另一探针旗标或经 `talk` 捕获，二选一写明）、各字段不变。
3. **上限即系列上限**：`rules::cultivationNeeded(Realm::CoreLate) == -1`、`(Realm::CoreMid) == 7000`；`realm 33 / realmCap 33` 时 `cultivation` 取 0、7000、99999 三值，`breakthroughBlock` 都是 `SeriesMax`；`tryBreakthrough(…, 任意种子)` 的 `blocked == SeriesMax`、`success false`、`cultivationLost == 0`，`cultivation` 不变。**负向**：`realm 32 / realmCap 33 / cultivation 7000` → `None`（证明挡它的是系列上限，不是修为或剧情上限）；`realm 32 / realmCap 32 / cultivation 7000` → `StoryCap`（两种「挡」分得开）。
4. **面板那一行与修为条**：真 `CultivationScene` 栈顶，`realm 33`：`buildMainItems(state)` 的「冲关」行 `enabled false`、`disabledReason == ` 修仙词表的 `pushCapped`。修为条那一行是 `renderStatus` 里的局部字符串、没有公开取值口（headless 也不记录画出的文字），**不改 `src/` 就不断它的文本**，改断它的分支条件：`rules::cultivationNeeded(Realm::CoreLate) < 0`（即 `CultivationScene.cpp` 第 713–719 行 `capped` 分支的条件），并把修仙词表的 `cappedNote` 与「（已至境界之极）」做常量比对。**对照**：`realm 32 / realmCap 33` → 「冲关」行 `enabled true`（修为 7000）、`cultivationNeeded(Realm::CoreMid) == 7000`。两态成对。（真要断画出来的那一行，另开代码项公开 `cultivationLine(state)`，先改笔记再改本文。）
5. **真数据（内容路落地后）**：`scripts/ch14/**` 里 `realm.advance(` 恰 1 处、在 `houqi.lua`、参数 `realm.CORE_LATE`；`realm.cap(`、`realm.demote(` 各 0 处；`houqi.lua` 里 `magic.learn("magic_jianying_fenguang")` 在 `realm.advance(` 之后（**写法约束**，不是引擎会拒，1.1）。**负向**：同一扫描函数喂一段内存字符串（两处 `realm.advance(`，或 `learn` 写在 `advance` 之前）→ 分别报出。目录不存在 `GTEST_SKIP`。
6. **负向自检**（改坏 → 增量构建 → 确实红 → 还原，日志 `build-ch14eng-neg-*.log`）：`breakthroughBlock` 里把 `SeriesMax` 那一判挪到 `StoryCap` 之后 → 第 3 条红（`tryNext` 失败、`next` 留在缺省 `Realm{}`，0 不大于 33、`cultivationNeeded` 回 −1，33 / 33 最后回 **`None`**，不是 `StoryCap`）；`RealmAdvance` 的「目标 == 当前」改回 `ok false` → 第 2 条红。

### 1.4 实现记录（引擎路交付时填）

（引擎路交付时填）

---

## 2. E2 封法局：主角缺席、友军顶「韩立」名牌（验证项，不改代码）

### 2.1 一句话口径

**阴冥之地的两场（④ `b14_yinming_tihun`、⑤ `b14_baofeng_feiyao`）`hero_absent true`：单位表里没有 id `hanli`，物品栏为空，我方只有编成里写死的友军（④ 啼魂兽一只；⑤ `hanli_jueling`、银猿、紫灵、梅凝四人）；`hanli_jueling` 是一个普通友军 role，名牌「韩立」只从它来，战斗里「韩立」恰一个单位、一行；友军的 `target "all"` 法术打全体站着的敌人；只按阵营判胜负；战后 `GameState` 的气血法力与开战前逐一相同；输了 `battle()` 回 `false, "lost"`、脚本照走下一句——**游戏结不结束只看脚本调不调 `game_over()`**：运行期没有代码读 `defeatIsFatal`（只有 `BattleLoader.cpp` 第 78 行读进来、`PathActionLoader.cpp` 第 773 行拿它拦切磋），④ `defeat_is_fatal false` 是给门禁与切磋看的，④ 不结束是因为 `lieshou.lua` 不调 `game_over`；①②③⑤ 的「致命」是脚本 `battle()` 后顶格 `if not won then game_over(); return end`（先例 `scripts/ch11/daoge.lua` 第 9–10 行，第 5 节）。** 名牌那一条红了，按笔记 O-2 的退路把 `hanli_jueling` 名牌改「韩立（无法力）」（先改笔记 H 表、再改本文），不动 `src/`；其余红了才议引擎。

- 规则层（`BattleState` 按阵营判胜负、`strike`）、加载器（`hero_absent`、`target`）、游戏层（`BattleScene` 建单位与 `settle()` 写回，`finish()` 调它）：**都不改**。
- 内容路要知道的：`hanli_jueling` `magics []`、`weapons ["剑"]`、`actions 2`；友军兵刃由规则 25 的 `hero_absent` 分支自动算进手段、不登表，啼魂两门 `role_magic` 要登（第 6.2 节）。`target "all"` 只许伤人法术（`power > 0`、不写 `effect`）；风雷翅 `effect "stagger"`，**不许**同写 `target "all"`（加载器拒）。④ 第 0 波必有友军 `tihun_shou_yin`；⑤ 四个友军都在第 0 波（`wave` 缺省）。阴冥段脚本 0 处 `magic.learn`（笔记施工纪律 80）。

### 2.2 数据形状（内容路 B 写；只是形状，role 与数量以笔记 I 表为准）

```json
{ "id": "b14_baofeng_feiyao", "name": "<内容路定>", "chapter": 14, "hero_absent": true,
  "can_escape": false, "defeat_is_fatal": true, "intro_key": "ch14.battle.baofeng_feiyao.intro",
  "units": [ {"role_id": "hanli_jueling", "faction": "ally"}, {"role_id": "tihun_yinyuan", "faction": "ally"},
             {"role_id": "zi_ling_jueling", "faction": "ally"}, {"role_id": "mei_ning", "faction": "ally"},
             {"role_id": "feixing_yinshou", "faction": "enemy"},
             {"role_id": "sanshou_yinshou", "faction": "enemy", "wave": 1} ],
  "rewards": {"cultivation": 0, "spirit_stones": 0, "drops": []} }
```

`rewards.cultivation` 由平衡路定（④⑤ 照发，第 5 章口径）；④ 的 `drops` 是 `story_yinming_shoujing` ×1、`rate 100`，形状照既有带掉落的编成。

### 2.3 判据（`tests/Ch14EngineTests.cpp`；临时编成与临时 role 写进 `TempDir`，直接读单位表 / `BattleState` / `GameState`，不经 `Driver`）

1. **单位表**：一张 `hero_absent true` 的临时编成（友军 `probe_hanli`：名牌「韩立」、`magics []`、`weapons ["剑"]`；友军 `probe_yuan`：带一门 `target "all"`、水类伤人法术；敌方两只）开真 `BattleScene`：没有 `id == "hanli"` 的单位；名字是「韩立」的单位恰 1 个且 `faction ally`；物品栏为空。**负向**：同一编成改 `hero_absent false` → 有 `hanli` 单位、名字是「韩立」的单位恰 2 个（证明第一句来自开关；同名双行正是 O-2 要防的样子，只出现在韩立在场的仗里）。
2. **全体法术**：`probe_yuan` 对敌放那门法术 → 每个站着的敌人 `hp` 都减少；先打倒一只再放 → 只打剩下那只、已倒的不再受击。
3. **胜负按阵营**：敌方全倒 → `Won`；友军全倒 → `Lost`（两份状态分开跑）。
4. **战后不动主角**：开战前把 `GameState` 的 `hp / mp` 砍到非满（先验不等于上限），`Won` 与 `Lost` 两种收尾之后 `hp / mp / maxHp / maxMp` 与开战前逐一相同；背包不变（`drops` 空时）。
5. **输了不结束（因为脚本没调 `game_over`）**：探针脚本 `local won, code = battle("<临时编成 id>")`，之后 `flag.set("probe.after", 1)`，**0 处 `game_over`**；让友军全倒 → `won` 为假、`code == "lost"`（经探针旗标读出）、`probe.after == 1`、`Application` 未请求退出、栈顶回 `"World"`。这条证明的是「`battle()` 输了照样回填、脚本照走」，**不证明 `defeat_is_fatal` 字段**（字段填什么这条都绿）。**对照**：同一编成、探针脚本改成 `if not won then game_over(); return end` → `Application` 请求退出、`probe.after == 0`（证明致命由脚本落地）。
6. **胜利卡与战报**：第 1 条那一场打赢后，胜利卡与战斗日志里以「韩立」为来源的行数 == 该友军实际行动的次数，没有一行来自 `speakerName("hanli")`（同名不合并、不重复）；实现若拿不到卡上的行，改钉「`BattleScene` 未调用主角名牌这一支」的等价事实，偏差写进 2.4。
7. **真数据（内容路落地后）**：`b14_yinming_tihun`：`hero_absent true`、`defeat_is_fatal false`、`can_escape true`、第 0 波友军恰 `tihun_shou_yin`、`drops` 恰 `story_yinming_shoujing` ×1；`b14_baofeng_feiyao`：`hero_absent true`、`defeat_is_fatal true`、`can_escape false`、第 0 波友军恰 I 表四个；`hanli_jueling` 名牌「韩立」、`magics` 空、`weapons` 恰 `["剑"]`；`magic_tihun_yinguang` `target "all"` 且 `power > 0`。任一文件不存在 `GTEST_SKIP`。
8. **负向自检**（改坏 → 构建 → 红 → 还原）：`BattleScene::settle()`（`finish()` 调它，第 995 行）去掉 `heroPresent_` 条件 → 第 4 条红；建单位时不看 `heroAbsent` → 第 1 条红。

### 2.4 实现记录（引擎路交付时填）

（引擎路交付时填）

---

## 3. E3 终章收束：结局卡、末章卡与通关读档（验证项，不改代码）

### 3.1 一句话口径

**全作最后一份脚本 `guixiang.lua` 先 `ending(title_key, body_key)`：结局卡压栈、脚本停住，玩家确认后淡出、脚本接着走；最后一条可执行语句 `flag.set("ch14.done")`：0 → 1 触发 `cardsForFlagChange`，第 14 章是 `data/chapters.json` 末项，排「第十四章　终」接「未完待续」，各一张、不重；之后回到行走、站在原格。存盘再经标题「继续旅程」读回：旗标本来就是 1，不排任何卡；目标链全部完成，`currentObjective` 返回 nullptr、HUD 无目标行；`ch14_minzhou` 上 `visible_flag ch14.done` 的紫灵、梅凝在场；主菜单「返回首页」照常。** 任一处不符，才改 `src/`（先改笔记 E 表 28–29、再改本文）。

- 规则层（`cardsForFlagChange`、`currentObjective`）、加载器（`chapters.json`、存档 v9）、游戏层（`Application` 的 `Ending`、`ChapterCardScene`、`TitleScene::continueJourney`）：**都不改**。
- 内容路要知道的：`ending` 的两个参数都是**文案 key**（引擎经 `text()` 取字），不是正文；`ui.ending.ch14.title`（「乱星海篇·终」）在 `ui.json`（协调者），`ch14.ending.body` 在 `data/text/ch14*.json`（内容路 C），正文三四句、不写下一篇的任何名字（笔记 O.2）。`flag.set("ch14.done")` 之后不许再有 `talk / teleport / advance_days / battle`——`flag.set` 只把章节卡记进 `pendingCards_`（`Application.cpp` 第 1016–1021 行），要等脚本跑完、栈顶回到行走才由 `flushChapterCards` 播出（第 590–601 行），所以它之后的语句会**播在章节卡之前**，「第十四章　终」就不再是全作最后一样东西。

### 3.2 数据形状

```lua
-- scripts/ch14/guixiang.lua（n29）末尾；全仓唯一一处 ending(
ending("ui.ending.ch14.title", "ch14.ending.body")
flag.set("ch14.done")   -- 全作最后一条可执行语句
```

`data/chapters.json` 末项（协调者）：`{"number":14,"numeral_key":"ui.chapter.14.numeral","title_key":"ui.chapter.14.title","done_flag":"ch14.done"}`；`ui.json`：`ui.chapter.14.numeral`「第十四章」、`ui.chapter.14.title`「磷火潭」（施工图 16.1 第 33 条；笔记 E 表 2 的「鬼雾」剧透第三幕转折，以施工图为准）、`ui.ending.ch14.title`「乱星海篇·终」。

### 3.3 判据（`tests/Ch14EngineTests.cpp`；直接读卡片请求 / 栈顶场景名 / `GameState`，不经 `Driver`）

1. **排卡规则**（纯函数）：内存里拼一张 14 章的 `ChapterTable`（第 1–13 章照 `data/chapters.json`、末项照 3.2）：`cardsForFlagChange(table, "ch14.done", 0, 1)` 恰 `{Closing, 14}`、`{ToBeContinued, 0}` 两项；`"ch13.done"` 0 → 1 恰 `{Closing, 13}`、`{Opening, 14}`；`"ch14.done"` 1 → 1 与 0 → 0 都空。**负向**：同一张表去掉末项 → `"ch14.done"` 空、`"ch13.done"` 排 `ToBeContinued`（证明「末章」按表位置判，不是写死章号）。
2. **同一脚本里的次序**（引擎口径：`flag.set` 只把卡记进 `pendingCards_`，脚本跑完、栈顶回到 `World` 才由 `flushChapterCards` 经 `presentCards` 播出；`presentCards` 把 Closing 与 ToBeContinued **装进同一个** `ChapterCardScene`、只压栈 1 次，headless 下直接 return、**不压栈**，只记 `cardLog`；`cardLog` **不记结局卡**——结局卡由 `Ending` 分支直接 `pushScene`，`waitsForConfirm` 的卡在 headless 下也停住等确认。`Application.cpp` 第 579–601、1016–1021、1080–1082 行；`Application.h` 第 236、239 行）：先把表末章的 done_flag 清 0（集成前 `data/chapters.json` 末项是第 13 章、之后是 14，判据读表、不写死章号）；headless 探针脚本 `ending(<任两条既有 ui.* key>)` 后 `flag.set(<该 done_flag>)`。泵帧至脚本停住 → 栈顶是结局卡（标题、正文与两个 key 的文本相同）、**旗标仍 0**、`cardLog` 与开头相同；模拟确认 → 结局卡出栈、脚本跑完 → `cardLog` 恰追加 `{Closing, 末章}`、`{ToBeContinued}` 两项、`pendingCards()` 为空 → 栈顶 `"World"`、`mapId / position` 与开头相同。**负向**：两句调换（先 `flag.set` 后 `ending`）→ 结局卡在栈顶时旗标**已是 1**、`pendingCards()` 非空（卡被押到结局卡之后），用例红——证明 n29 的先后由脚本决定、内容路必须照 3.2 写。
3. **通关读档**：上一条之后存盘（路径换成 `TempDir`）→ 新的 headless `Application` 经 `continueJourney` 读回：栈顶 `"World"`，泵 60 帧后 `cardLog` 与 `pendingCards()` **都为空**；旗标仍 1；`mapId / position` 与存盘时相同。**负向**：读回后把旗标改 0 再 `flag.set` 一次 → `cardLog` 追加那两项（证明「读回不排」来自 0 → 1 规则，不是读档把卡吞了）。
4. **目标链收尾**：内存里一条三步链，三步 done_flag 全置 → `currentObjective` 为 nullptr；差最后一步 → 指向它。行走 HUD 取目标的那一处在 nullptr 时不画目标行（以 HUD 实际调用的函数判，写明是哪个）。
5. **主菜单**：第 3 条读回后打开主菜单，「返回首页」一行 `enabled true`。
6. **真数据（内容路 / 协调者落地后）**：`data/chapters.json` 末项恰 3.2 那一行；`ui.json` 有 3.2 那三条 key；`data/text/ch14*.json` 有 `ch14.ending.body`；`maps/ch14_minzhou.tmj` 0 个 portal，`npc_zi_ling_minzhou` / `npc_mei_ning_minzhou` 的 `visible_flag == "ch14.done"`、不写 `hidden_flag`，载入该图时旗标 1 两人在场、0 两人都不在；`data/objectives/ch14.json` 末步 done_flag 是 `ch14.done`，把第 1–14 章链上旗标全置后合链 `currentObjective` 为 nullptr；`scripts/**` 里 `ending(` 恰 1 处、在 `guixiang.lua`、参数恰 3.2 两个 key，该文件最后一条可执行语句是 `flag.set("ch14.done")`；`scripts/**` 与 `data/**` 里 `ch15.` 0 处。**负向**：同一扫描函数喂内存字符串（`flag.set("ch14.done")` 之后还有一句 `talk`）→ 报出。任一文件不存在 `GTEST_SKIP`。
7. **负向自检**（改坏 → 构建 → 红 → 还原）：`cardsForFlagChange` 末章分支改成不排 `ToBeContinued` → 第 1、2 条红；`Ending` 分支改 `return true`（不等确认）→ 第 2 条红。

### 3.4 实现记录（引擎路交付时填）

（引擎路交付时填）

---

## 4. 下一章接口约定（**全作末章：没有第 15 章**；本批不实现，写成收束约定；笔记 O.2 逐行照抄）

协调者、测试路与将来的续作照这张表接；要改表里任何一行，先改本文。第 13 章契约第 3 节（「第 14 章接口」）里「境界」「伴妖草、铜片」「养魂木」三行的结论（笔记 B21–B23：本章 0 场在结丹中期、风希不索草、养魂木 0 处读）由协调者在那一份里补，本文不重复。

| 接口 | 本章留下什么 | 怎么用 |
| --- | --- | --- |
| **章节表** | `data/chapters.json` 末项 `{"number":14,…,"done_flag":"ch14.done"}`；`ui.json`「第十四章」「磷火潭」（施工图 16.1 第 33 条）| `ch14.done` 0 → 1 排「第十四章　终」接「未完待续」（E3）；`UiStyleTests` 表长 14、末章断 `{Closing,14}` ＋ `ToBeContinued`，删掉「下一章不排卡」那条断言（全仓不留 `ch15.` 字样） |
| **结局卡** | `ending("ui.ending.ch14.title", "ch14.ending.body")`：标题「乱星海篇·终」，正文写离乡百余年、结丹后期顶峰、风雷翅冲出鬼雾、携紫灵与梅凝落脚溪国闵州（三四句，不写下一篇的任何名字） | E3；`ui.ending.continue_hint` 已有 |
| **境界** | 33 / 33 / `formerRealm 22`（E1）；面板 `SeriesMax` | 「顶峰」0 个数据字段；续作若开元婴档，自己开引擎项（`cultivationNeeded(CoreLate)` 现为 −1） |
| **规则 25 探针** | `MEANS_LAST_CHAPTER = 14`；`b14_*` 全部进范围 | selftest 在测试副本里**临时写一张合成编成** `data/battles/b15_fanwei_tanzhen.json`（`chapter 15`）**连同一个合成 role** `data/roles/probe_fanwei_shou.json`（`weaknesses ["毒"]`、`killable_by ["毒"]`，只被合成编成引用）——**不碰任何真 role**（施工图 16.1 第 48 条：`sanshou_yinshou` 是 ⑤ 的真敌人，改它期间 ⑤ 也会报，「角色只在指定一场」的先验不成立）：先验「写入前 `probe_fanwei_shou` 0 处被引用」；`chapter 15` → 不报；改 `chapter 14` → 必报「没有一样是这一场」（报错按 `b15_fanwei_tanzhen` 过滤）；`finally` 删两个文件，再验目录清单与改前相同；`future_id` 那段同法；「末章12 / 13」用例名与 `== 13` 断言改 14（C2） |
| **终档** | `tests/fixtures/ch14-end-{first,second}.sav`（v9） | 只作通关档回归：标题「继续旅程」读它（E3 第 3 条的真档版）；`ChapterFixture.h` 登记 |
| **通关后** | 站在 `ch14_minzhou`；紫灵、梅凝在场；客栈可打坐；0 portal | 不开放旧图；将来续作从这张图接（落云宗 ch597 起） |
| **未用的线头** | `story_jiaohun_ping`（可能 0）、`story_tianfu_guhe`（可能 0）、`story_yaowen_zhujie`（可能 0）、`story_jiuqu_lingshen`、`story_ningying_xinde`、`story_xutian_ding`、`story_bamen_jinjing`、妖丹 | 只留在包里，不做任何结算；全仓不得有读它们的第 15 章脚本 |
| **队伍 / Lua / 存档** | `party` 空、0 个新命令、v9 | 照旧 |
| **交接** | `docs/handoff.md` 写成**全作完成版**（14 章、52 场、84 图、支线 24 / 必经 4） | 归协调者（C7） |

---

## 5. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让几路对同一件事只有一种写法（id、旗标、数字一律以笔记为准；同一件事实的权威在施工图主表——节点 F 表、挂点与在场窗口 G.1、编成 I 表、旗标 J 表、日数 N.2、teleport N.4——本节只引用）：

- **开局**：`scripts/ch13/banyao.lua` **只在末尾 `flag.set("ch13.done")` 之后补一行 `teleport("ch14_linhuotan")`**（只传图 id，落 default spawn `tongdao`；此前一个字不动）；n01 挂在 `trigger_tongdao`（spawn 北两格的横贯带），不压 spawn。
- **境界**：见 1.1、1.2——`realm.advance(realm.CORE_LATE)` 恰 1 处（`houqi.lua`）、看返回值；`realm.cap`、`realm.demote` 0 处；脚本不读 `formerRealm`。
- **法术**：`magic.learn("magic_jianying_fenguang")`、`magic.learn("magic_bixie_shenlei")`（`houqi.lua`，都在 `realm.advance` 之后）、`magic.learn("magic_fenglei_chi")`（`taoni.lua`，n07）、`magic.forget("magic_ji_wuxinghuan")` ＋ `take("story_wuse_tonghuan", 1)`（`hufa.lua`，n15 战后）各恰 1 处、都看返回值（false 只换一句照走）。`magic_fenglei_chi` **就地改写**（`data/magics/fenglei_chi.json`）：`element 2`（木）、`effect "stagger"`、`stagger 2`、`power 0`、`needRealm "CoreLate"`、note 改「雷遁绕身削架势」，不写 `target`；`descKey` 不改——它的描述在 `data/text/ch03.json` 第 34 行，原句与新用法不冲突，旧文件不碰。`magic_fengyun_heji` 一字不动（笔记 E 表 11）。**新建 6 门**（施工图 16.2 第 9 条；笔记 L.2 表头写 5、表内实列 6）：`magic_jianying_fenguang`、`magic_dujiao_xueguang`、`magic_wen_jinzhen`、`magic_bamen_jinguang`、`magic_tihun_xihun`、`magic_tihun_yinguang`，类别 / 形状照笔记 L.2。`magic_bixie_shenlei`（第 12 章为符箓 `castMagic` 所建，道具施法不扣法力）本章第一次当常驻法术学，它的 `needMp` / `power` 合不合用由平衡路 B1 点名量；不合用则 `bixie_shenlei.json` 进内容路 B 的就地改写（先改 7.3）。
- **teleport**：只写图 id 或图 id ＋ 整数坐标，不传 spawn 名，坐标不许 `(0, 0)`；笔记 N.4 共 9 条（三元 7、五元 2）。挂 NPC 的（`biyanjiu.lua`）与首次置旗的（`taoni.lua`、`shefu.lua`、`chuansong.lua`、`hufa.lua`、`tuoli.lua`）走三元：`flag.set` 与 `teleport` 写在顶格、早退用顶格 `return`。落地**不触发**脚下 enter——每个落点都在下一个剧情带之前一两格（N.4 #6 落点不压 `trigger_yuta`、与 `trigger_mishi_men` 隔 ≥ 2 格；#7 落在 `trigger_zhenqian` 南侧 ≥ 2 格）。
- **雷遁捷径（五元，同图）**：`trigger_leidun_dong` / `trigger_leidun_xi` 一律 `type trigger`、`mode enter`、`once false`、`guard_flag ch14.taoni`、`set_flag` 空，分别挂 `leidun_dong.lua` / `leidun_xi.lua`，每份 0 处 `flag.set(`；开头 `if not magic.knows("magic_fenglei_chi") then talk(…) return end`，`teleport("ch14_shuangfeng", x, y)` 顶格。两岸本有绕行路（规则 13：捷径只省路、不挡路）。一份脚本拿不到来处，所以按方向拆两份。
- **触发器与门闸**：`once true` 的必有 `set_flag`（＝ done_flag），脚本末尾也 `flag.set`；挂 NPC 的节点（n02、n09、n18、n22）脚本开头判 done_flag 已置 → 一句 `return`、前置未置 → 一句 `return`。同一格只取对象表里第一个 trigger，挂点错开格子。**本章 0 个 portal**；facility 只写 `require_flag`（`facility_dazuo_gukou` 写 `require_flag ch14.fatan`、不写 `deny_text_key`），其余打坐设施不写 `effectiveness`（＝ 100）；`ch14_yinming` 0 个打坐设施。规则 13：每张图只从 default spawn BFS，密室、西岛大厅、山巅都要有路连着，门闸靠 guard 惰性、不靠隔断。规则 30：**同图同 role 至多两个对象**、两者互补（规则两两比对、不传递；施工图 5.5、16.1 第 38 条——炼器室不放风希对象，梅凝阴冥两段 `rucun → chucun`、`chucun → tuoli`）；墓穴岛上的元瑶用新 role `yuan_yao_hufa`（`yuan_yao_jiedan` 在第 12 章灵泉不撤，16.1 第 36 条）。
- **日历**：每句 `advance_days` ≤ 360，日数只照笔记 N.2（全章固定增量 680）；阴冥段 0 处打坐。
- **扣东西**：`take` 一律看返回值；n06 `take("story_xishi_lingye", 1)`、n12 回赠三支（只列付得起的，第三支「只道谢」永远在）、n15 `take("story_wuse_tonghuan", 1)`；全章 0 处扣灵石；`take(id, 0)` 在引擎里会被当成扣 1，余数写法照第 13 章契约第 4 节。
- **整数旗标**：`ch14.jiaohun`（1 / 2）、`ch14.huizeng`（1 / 2 / 3）、`ch14.z_guhe`（1 / 2）用 `flag.set(name, n)`；开关旗标 `flag.set(name)`（笔记 J 表）。
- **致命由脚本落地**：①②③⑤ 的脚本（`lianqi.lua`、`chuansong.lua`、`hufa.lua`、`tuoli.lua`）在 `battle()` 那一行之后紧跟顶格 `if not won then game_over(); return end`，在战后的一切置旗、give / take、forget、advance_days、teleport 之前；④ `lieshou.lua` 判 `won`（败、逃同支）、只说一句 `return`，0 处 `game_over`（施工图 3.2 表后第 4 条、验收 15）。
- **编成**：五场照笔记 I 表；① **就地改写** `b14_fengxi_xiezhi`（id 不变、`name`「炼器室·噬蛟」、`backdrop secret_room`、敌方换 `du_jiao` 两段、`feng_xi` 不进任何编成）；全部 `rewards.spirit_stones 0`；`killable_by` 非空且 ⊆ `weaknesses`；`charge` 四键 `every / mult / all / text_key` 都必写；开场白、蓄势、胜利文案在 `data/text/ch14_battle.json`，key `ch14.battle.<短名>.intro / .charge / .win`；③ 胜利文案只写到「精血与金角耗尽、骑虎难下」（O-8）。
- **配方 / 商店 / 遭遇 / 队伍**：0 / 0 / 0 / `party.*` 0 处；任务 3 份（`q14_xunxiong` 必经、`q14_yaowen`、`q14_guhe`）。
- **章末**：见 3.1、3.2——`ending(…)` 后最后一条可执行语句 `flag.set("ch14.done")`。
- **文案**：`data/text/ch14*.json` 0 处 `{key.` 与 `{pad.`，**也不点名任何可改的键**（菜单、确认 / E、存盘、WASD、Z / X）——真要点名就用 `{key.<id>}` ＋ `.pad` 版并进 KeyPrompts 白名单（6.2；施工图 13 第 17 条）；笔记 M.2 那些词全章 0 处（「结婴」最容易误写）；章内先后照笔记 M 表末段（「风希」「梅凝」「封天极」「紫灵」「结丹第一人」「溪国 / 闵州 / 封日城 / 馨牙兽」各自的起点），施工图若另立专项以施工图为准。

---

## 6. 门禁与既有测试的依赖

### 6.1 引擎路

本章不改 `src/`、不改 `tools/`。共享内容不齐时只报告 `tests/Ch14EngineTests.cpp` 的证据，不报全量 data 绿色；内容路未落地的那几条（1.3 第 5 条、2.3 第 7 条、3.3 第 6 条）按 `GTEST_SKIP` 交付，集成后由协调者重跑一次看实跑未跳过。

### 6.2 协调者：集成时落

- **规则 25 的表**（`tools/validate.py` 第 2289 行起；`MEANS_LAST_CHAPTER` 第 2385 行，2026-10-13 仍是 11，第 12、13 章集成后 13）：`CHAPTER_MEANS[14]` ＝ 第 13 章那 8 类原样（拳 `hand`；火 `learn magic_huodan_shu ch04`；土 `learn magic_liusha_shu ch06`；水 `learn magic_bingdong_shu ch06`；金 `learn magic_ji_jinfu ch07` ＋ `("金","learn","magic_shijin_chongqun","ch12")`；木 `learn magic_qingyuan_jianmang ch08`；暗器 `give weapon_wuming_sixian ch07`；剑 `give weapon_lieyang_jian ch06`），木外加 `("木","learn","magic_jianying_fenguang","ch14")`、`("木","learn","magic_bixie_shenlei","ch14")`（n03，① 之前）；**五行环不登**（n15 忘掉）；`BATTLE_EXTRA_MEANS["b14_yinming_tihun"] = [("水","role_magic","magic_tihun_xihun","tihun_shou_yin")]`、`["b14_baofeng_feiyao"] = [("水","role_magic","magic_tihun_yinguang","tihun_yinyuan")]`（友军兵刃由 `hero_absent` 分支自动算，不登）；`MEANS_LAST_CHAPTER = 14`。
- **selftest 探针**（`tools/validate_selftest.py` 现第 1801–1830、2465–2466、2503–2526 行，以第 13 章集成后为准）：照第 4 节「规则 25 探针」一行改成合成第 15 章编成；**必须与 ① 就地改写同一批落**——改写后 `feng_xi` 不在任何编成里，第 13 章挪过去的探针先验「敌方含 `feng_xi`」会红（笔记 O-1）。
- `tests/LexiconTests.cpp` 表一**照施工图 12.1**：73 词 ＋ 禁词「结婴」「落云宗」两个；**巨猿傀儡**（首见 427，不是笔记 M.1 写的 567）、**狻猊**（第 10 章定为不进表）、**结丹大圆满**（原著 0 命中、表一每行要写出处，只走施工图 12.2 专项）不进表一。笔记 M.1 / H 表里巨猿傀儡的「567」由协调者一并回写。表二已有 `{14, 596}`（2026-10-13 在第 542 行），不动。
- `data/flags.json`：笔记 J.1 ＋ J.2 共 **33 面**在内容路派工之前登记（`ch14.jiaohun` 写「1 收瓶 / 2 散去」、`ch14.huizeng` 写「1 / 2 / 3 三支」、`ch14.z_guhe` 写「1 接下 / 2 婉拒」）；J.3 的 14 条 `ch14.path.<id>` 等 `data/pathactions/ch14.json` 落地后登记。
- `data/chapters.json` 末项与 `data/text/ui.json` 三条 key（3.2）。
- `data/text/battles.json`：**删**旧占位 `ch14.battle.fengxi_xiezhi.intro`（2026-10-13 在第 9 行）——① 的开场白由内容路 C 写进 `ch14_battle.json`，不删就是重复 key，`loadGameData` 整个起不来（第 11–13 章同一个坑）；**在内容路 C 落文案之前删**。
- **KeyPrompts 白名单**（`tests/KeyBindingTests.cpp` 第 556、605 行两组 `allowed`）：本章文案 0 处占位则不动；若某条 `objective.ch14.*` 要带 `{key.*}`，两组各加一条、`.pad` 版同写、先验计数随之加。
- `src/core/rules/Realm.h` 第 175–186 行、`Realm.cpp` 第 258–259、276–277 行：**只改注释**（第 0 节末段；笔记 O-5）。
- 回写（笔记 O.4）：大纲第 14 章条目（B1–B4）与第 5 节（地图 84、支线 24 / 必经 4、剧情战 52、法术数 O-12）；lore（B5–B8、B24）；`势力与地理.md`（B12、B13）；人物总表（B9、B10、B14、B15）；第 13 章两份文档里写「`banyao.lua` 不 teleport / 最后置 `ch13.done`」的**八处**都改成「`flag.set("ch13.done")` 之后仅末行一处 `teleport("ch14_linhuotan")`」——`docs/ch13-design.md` 第 402、663、1095（验收 20，连「最后一条可执行语句是 `flag.set`」）、1139（16.1 第 28 条，只改落点）、1278 行（纪律 73），`docs/interfaces-p3-ch13.md` 第 107、142、235 行（施工图 10.1 C7、16.2 第 1 条；行号 2026-10-13，以 grep「不 teleport」为准）；`docs/interfaces-p3-ch13.md` 第 3 节三行（第 4 节开头）；`docs/handoff.md` 写成全作完成版、`docs/tech-debt.md` 按惯例。平衡路写在编成 `note` 里的实测由协调者摘进 `handoff.md`，**不进** `ch14-design.md`（那一份归内容路 C；施工图 16.1 第 50 条同口径）。

### 6.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/Ch13*`（第 13 章测试路写的通关 / 验收）| 与 `ch13-end-{first,second}.sav` 逐字段比；章节卡断「未完待续」；验收 20 扫 `banyao.lua` 不 teleport | `banyao.lua` 末尾补 `teleport("ch14_linhuotan")` 之后重生成 `ch13-end-*`（`mapId ch14_linhuotan`、站 `tongdao`）；章节卡断言改 `Opening(14)`；扫描断言改「仅末行一处 `teleport("ch14_linhuotan")`」（施工图 T1） | 测试路 |
| `tests/UiStyleTests.cpp` `ChapterTableData` / `ChapterCards` | 2026-10-13 钉到第 11 章；第 13 章集成后钉到 13、断 `ch14.done` 不排卡 | 表长 14、末项「第十四章」「磷火潭」`ch14.done`；末章 `ch14.done` → `{Closing,14}` ＋ `ToBeContinued`，`headline`「第十四章　终」、`kicker`「磷火潭」；**删掉**「下一章不排卡」那条（全仓不留 `ch15.`） | 测试路 |
| `tests/ObjectiveTests.cpp` `kScriptTransfers` | 第 594 行起；第 13 章集成后到 `ch13.*` | 三元 7 条：`{"ch13.done", "scripts/ch13/banyao.lua", "ch14_linhuotan"}`、`{"ch14.yinjiu", "scripts/ch14/biyanjiu.lua", "ch14_linhuotan"}`、`{"ch14.taoni", "scripts/ch14/taoni.lua", "ch14_shuangfeng"}`、`{"ch14.shefu", "scripts/ch14/shefu.lua", "ch14_shuangfeng"}`、`{"ch14.jieyao", "scripts/ch14/chuansong.lua", "ch14_yaomu"}`、`{"ch14.guiwu", "scripts/ch14/hufa.lua", "ch14_yinming"}`、`{"ch14.tuoli", "scripts/ch14/tuoli.lua", "ch14_minzhou"}`；五元 2 条 `{"ch14.taoni", "scripts/ch14/leidun_dong.lua", "ch14_shuangfeng", "trigger_leidun_dong", "ch14_shuangfeng"}`、`{"ch14.taoni", "scripts/ch14/leidun_xi.lua", "ch14_shuangfeng", "trigger_leidun_xi", "ch14_shuangfeng"}`（笔记 N.4）。**O-4**：五元两条源图 ＝ 目的图，第 12、13 章的五元都跨图——先核 `kScriptTransfers` 与 `ObjectiveRoute` 对同图条目不误判；`biyanjiu.lua`、`shefu.lua` 是同图带坐标的三元，同核 | 测试路 |
| `tests/ObjectiveTests.cpp` `kOffChainDoorKeys` | 只有 `ch01.done` | 不动：本章 0 个 portal；唯一的设施门闸 `ch14.fatan` 在链上 | — |
| `tests/NpcPresenceTests.cpp` `kOffChainFlags` / `kLastChapter` | 第 133 行 `kLastChapter = 11`（第 12、13 章集成后 13） | `kLastChapter` 改 14；**`kOffChainFlags` 本章 0 条**（施工图 16.1 第 39 条）：笔记 J.2 四面都不挂在 NPC 在场旗标上（那张表只管这一类），且 `z_yaowen` 在第 20–23 步之间任意时刻都可能置、`offChainProblems` 要求窗口两头在链上紧挨着，登了必红；照 `ch11.jiguan` 在表尾写一行注释说明不登 | 测试路 |
| `tests/PathActionTests.cpp` `realmCapOfChapter()` | 第 901–903 行到 `{11, Realm::CoreEarly}`（集成后到 `{13, Realm::CoreMid}`） | 加 `{14, Realm::CoreLate}`（`houqi.lua` 的升境在 `ch14.done` 之前）；本章 14 条全 `realm 0` | 测试路 |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | 加 `ch14-end` 两份（v9） | 测试路 |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | ④ 的 `drops` 是 story 物（剧情战，不是遭遇）；红了按遭遇 / 剧情战分开判，不改编成 | 测试路 |
| `tests/Ch11EngineTests.cpp`、`Ch13EngineTests.cpp`、`RealmCapTests.cpp`、`PanelTests.cpp` | 钉 31 / 32 档与 `StoryCap` | 不动：E1 用自己的夹具；`tests/` 下 0 处引用 `b14_*`、`feng_xi`、`fenglei_chi`（2026-10-13 grep），就地改写不牵动既有用例 | — |

---

## 7. 白名单（按文件分；两两不重叠，共享文件归唯一一路）

### 7.1 引擎路

- `tests/Ch14EngineTests.cpp`（新；E1–E3 全部判据）
- `docs/interfaces-p3-ch14.md`（只填 1.4、2.4、3.4 与末尾「实现记录」）
- `src/**`、`tools/**`：**不在白名单**。判据红了先报协调者、改笔记与本文，再另派。

### 7.2 内容路 A（地图美术）

- `maps/ch14_{linhuotan,shuangfeng,yaomu,yinming,minzhou}.tmj`（新 5 张）、`maps/tilesets/terrain_ch14_*.tsj`（生成器要的话，新）
- `tools/mapgen/genmaps_ch14.py`（新）、`tools/mapgen/genmaps.py`（只加登记，排在第 13 章之后）
- `data/visual/maps.json`、`data/visual/battles.json`、`data/visual/looks.json`（只加本章条目；主题 / 时辰 / 背景只用笔记 G 表与 I 表列的现有名）
- `tools/artgen/sprites.py`、`tools/artgen/sprites_enemies.py`（只加本章 role 的立绘登记；非人形见笔记 H 表末段，`sprite_preview.py --coverage` 缺图 0）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 上方的注释表加 5 行，元组不动，同第 12、13 章）、`docs/audio.md`（同一张表）

### 7.3 内容路 B（数据）

- `data/roles/*.json`（新建 41 个：笔记 H 表 40 ＋ `yuan_yao_hufa`〔施工图 16.1 第 36 条〕；**就地改写** `feng_xi.json` 只改 note；`fan_furen.json`、`yun_tianxiao.json`、`yuan_yao_jiedan.json`、`zi_ling.json` 一个字不动）
- `data/items/**`（只新建 L.1 那 18 件；已有物品一件不改）、`data/magics/*.json`（新建 6 门，id 见第 5 节「法术」；**就地改写** `fenglei_chi.json`，第 5 节；`bixie_shenlei.json` 只在平衡路 B1 判它不合用、先改本文之后才进）
- `data/battles/b14_fengxi_xiezhi.json`（就地改写）、`data/battles/b14_{qingyang_zhifashi,hufa_wentianren,yinming_tihun,baofeng_feiyao}.json`（新）
- `data/quests/q14_{xunxiong,yaowen,guhe}.json`、`data/pathactions/ch14.json`、`data/objectives/ch14.json`
- **平衡路**不另开白名单、不与 B / C 并行：B、C 收尾之后串行接手，只改上面这些 role / battle 文件的数值，测量记录写进编成的 `note`；`Realm.cpp` 的数字不动（第 0 节末段）。

### 7.4 内容路 C（脚本文案）

- `scripts/ch14/**`（新）；`scripts/ch13/banyao.lua`（**只在末尾 `flag.set("ch13.done")` 之后补一行 `teleport("ch14_linhuotan")`**，此前一个字不动）
- `data/text/ch14*.json`（新，按前几章的拆法：正文、`ch14_battle.json`、`ch14_path.json` 等，含 `ch14.ending.body`；0 处 `{key.` / `{pad.`）
- `docs/ch14-design.md`（**只追加「施工偏差」一节**）

### 7.5 协调者

- `tools/validate.py`（**只** `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段）、`tools/validate_selftest.py`（只换合成探针与 `MEANS_LAST_CHAPTER` 那一处断言）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（3.2 那三条）；`data/text/battles.json`（**只删**那一条旧占位）
- `tests/LexiconTests.cpp`（**只**表一补词；章内先后专项归测试路 7.6）；`tests/KeyBindingTests.cpp`（只在 6.2 那一条触发时）
- `src/core/rules/Realm.h`、`src/core/rules/Realm.cpp`（**只改注释**，6.2）
- `docs/interfaces-p3-ch13.md`（只补第 3 节三行，与第 107、142、235 行「不 teleport」三处）；`docs/ch13-design.md`（只改第 402、663、1095、1139、1278 行这五处「不 teleport / 最后置 `ch13.done`」，6.2 回写条）；`docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（含 `人物总表.md`、`势力与地理.md`，回写）

### 7.6 测试路（引擎路与内容路都落地、协调者 6.2 之后另派）

- `tests/Ch14*.cpp`（除 `Ch14EngineTests.cpp`）：通关 `Ch14Walkthrough`、挂点、编成、账、切片、验收、章内先后词表专项（施工图 T3）
- `tests/Ch13*.cpp`（除 `Ch13EngineTests.cpp`；只改通关终点、章节卡与 `banyao.lua` 扫描那三处）、`tests/UiStyleTests.cpp`（只改 `ChapterTableData` / `ChapterCards`）
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`、`tests/BattleRewardTests.cpp`（只按 6.3）
- `tests/ChapterFixture.h`、`tests/fixtures/ch13-end-{first,second}.sav`（重生成）、`tests/fixtures/ch14-end-{first,second}.sav`（新）、`saves/README.md`

---

## 8. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记 J 表旗标、删 `battles.json` 占位 → 引擎路（E1–E3 判据）∥ 内容路 A / B / C → 平衡路（B、C 收尾之后）→ 协调者 6.2 与集成（规则 25 表与合成探针同 ① 改写一批）→ 测试路（先改第 13 章通关终点、重生成 `ch13-end`，再写第 14 章通关、生成 `ch14-end`）→ 独立校对。**本章在第 12、13 章集成之后开工**；前面的章节仍在同一棵工作树里施工 / 审查时，各路只碰自己白名单里的文件，别的一律只读。
- **跨路依赖**：引擎路与内容路之间没有字段依赖（本章 0 个代码项）；1.3 第 5 条、2.3 第 7 条、3.3 第 6 条要等内容路 A / B / C 与协调者落地，之前 `GTEST_SKIP`。
- 每路用自己的构建槽：引擎路 `ch14eng`、内容路 `ch14content`、测试路 `ch14test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。几路共用一棵工作树，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- **门禁先于编译跑**：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`；引擎路交付前跑 `python tools/validate_selftest.py`（仓库本身不净时它连锁 FAIL，照第 11 章实现记录写明与本路无关的条目）。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- **旧图不 patch**：本章 0 张；`genmaps_ch05.py`–`genmaps_ch13.py` 一个字不改。**旧脚本只** `scripts/ch13/banyao.lua` 末尾补那一行 `teleport`；旧文案文件（含 `data/text/ch03.json` 里风雷翅的描述）不碰。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- 原著文本只在本地核对事实（`tools/novel/novel.py`），不复制、不进仓库；对白字句自己写，换词不算重写。
- **不派子代理**，不整份重读大文件，报告精炼。**不碰 git**：各路不 add / commit / stash / checkout，提交由协调者按 `docs/handoff.md` 第 3 节的流程做；不动 `fanren/build*` 目录。

---

## 实现记录

（引擎路交付时填）

