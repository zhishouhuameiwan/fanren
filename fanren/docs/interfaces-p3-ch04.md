# P3 第 4 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧与关卡照它写调用，双方不得私自改动。
> 需要改签名时改本文并知会另一方，不在代码里单方面偏移。
> **若你认为契约本身有问题，先在报告里说清楚再动手**——第 2、3 章的实现方都这么做过，
> 其中两次是对的，一次纠正了我写错的地方。
>
> P3 · 2026-09-21 · 依据 `docs/ch04-design.md`

---

## 0. 三样前置，全都不具备

| # | 要的东西 | 现状 |
| --- | --- | --- |
| 1 | 已习得法术能落盘 | `magics` 只在 `RoleTemplate`（data）上，`GameState` 里没有。**技术债 G-4** |
| 2 | 多波次战斗 | `BattleSetup` 一份编成就是一组单位，没有波次概念 |
| 3 | 炼制能从游戏层调到 | `src/core/rules/Crafting.h` 造好了、`data/recipes/` 有四类，但 `src/game/` 与 `src/io/` **一次调用都没有** |

> **第 3 样是同一个形状的第三次**：P2 的规则模块在第 2 章前够不着、
> 战斗的法术与物品在第 3 章前够不着、炼制在这一章前够不着。
> **动手前请先跑一遍全仓盘点**：`src/core/rules/` 下还有哪些模块在 `src/game/` 与 `src/io/`
> 里没有任何调用方？把清单写进报告——我要在第 5 章开工前一次看清，不要再一章撞一个。

---

## 1. 已习得法术（解掉 G-4）

### 1.1 状态

`GameState` **末尾**追加（不得插在中间，会破坏聚合初始化）：

```cpp
std::vector<std::string> learnedMagics;   // 已习得法术 id，对应 data/magics/
```

### 1.2 存档

- `kSaveVersion` 4 → **5**，登记 **4→5** 迁移。
- v4 本就没有这一项，所以迁移是空操作——**但必须登记**。
  第 2 章已经证明「空操作」与「忘了登记」在行为上天差地别。
- 往返测试覆盖：空、一条、多条、重复 id（应当去重还是原样存？由你定并论证）。

#### 1.2.1 重复 id 的裁决（2026-09-21 落地）：**去重，保留第一次出现的位置**

`learnedMagics` 的语义是**集合**，不是流水账。两条理由，第一条是硬的：

1. **战斗菜单逐条遍历这份清单建行。** `BattleScene::buildMagicItems` 对清单里的每一个
   id 建一行，重复的 id 就是同一门法术在菜单里出现两行——两行一模一样、点哪行都一样。
   这不是洁癖问题，是屏幕上看得见的缺陷。
2. `magic.count()` 要回答的是「他会几门法术」，不是「`learn` 被调了几次」。

去重发生在**写入侧**（`GameState::learnMagic` 幂等），所以 `toJson` 永远写不出重复项；
**读入侧另有一道**（走的是同一个 `learnMagic`，规则只有一个定义处），挡的是手改过的存档。

代价写明：这让「手写一份带重复项的 payload 再读回来」不再是恒等变换，而往返恒等是
io 层最硬的那条断言。代价可接受，因为**带重复项的 payload 经由 API 写不出来**——
`tests/Ch04MagicTests.cpp` 有一条专门钉住这个边界
（`ARoundTripIsExactlyTheIdentityForAnyStateReachableThroughTheApi`）。

顺序是**习得的先后**，刻意不排序：菜单的行序因此等于他习得的次序，火弹术在前、
御风决在后，那正是本章两个节点的先后（设计第 1 节约束 1）。排序看着整齐，
代价是那个先后从此由拼音决定。

坏数据的三种态度也一并记下：数组里混着**非字符串** → 判失败（与 `flags` 里
「值不是整数就报错」同一口径）；**空串** → 跳过；**重复** → 去重。

#### 1.3.1 `magic.forget` 的原因码

契约只给了 `learn` 的 `no_magic`。`forget` 照 `party.remove` / `take()` 的口径：
真忘掉了返回 `true`；本来就不会返回 `false`，原因码 **`not_learned`**。
`forget` **不查 data**——忘掉一门 data 里已经被删掉的法术仍然该成功，
那是把存档里的悬空 id 清出去的唯一办法。

### 1.3 脚本 API

```lua
magic.learn(id)     -- 学会一门。已会则什么都不做（幂等），仍回填 ok = true
magic.forget(id)    -- 忘掉一门（第 9 章境界跌落可能用得上；本章不用，但一并给）
magic.knows(id)     -- 只读，bool
magic.count()       -- 只读，int
```

写走命令队列，读走 `__host`。命令 kind 追加在 `CommandKind` **末尾**。

**学一门 `data/magics/` 里不存在的 id 要回填 `ok = false`**（`code = "no_magic"`），
不要静默收下——拼错的法术 id 会让玩家在战斗里看着一个空菜单，而那时已经很难追回是谁写错的。

### 1.4 战斗里怎么用

> **已被 docs/octopath-battle.md 取代**（八方旅人化改造，2026-09-25）：已习得法术的口径不变；法术多了蓄劲方式 `boost`（新文档 2.3、5.2）。

`BattleScene::buildFromSetup` 里韩立那一位的 `magics` **取自 `GameState::learnedMagics`**，
不再是空表。`magicsExhaustive` 仍然置 `true`（那是第 3 章定的口径：声明几条就是几条）。

**这一条落地之后，技术债 G-4 即可销账。**

---

## 2. 多波次战斗

> **已被 docs/octopath-battle.md 取代**（八方旅人化改造，2026-09-25）：多波次原样沿用；推波不再有落位与「压在有人的格子上就挪开」（没有格子），波间照旧不回血不重置。

### 2.1 要表达什么

大纲写明门派攻防战是**多波次**的。最小可用口径：

- 一场战斗可以有若干波，**打完当前波之后下一波进场**。
- **波与波之间不回血、不重置状态**（毒、罡气、位置都留着）。那是攻防战的全部张力。
- 最后一波打完才算 `Won`；中途全灭即 `Lost`。

### 2.2 数据怎么写

由你定并写进本文。一个建议（不强制）：`BattleSetup.units` 里给每个单位一个波次号，
波次号 0 的开场就在场上，之后按号入场。

**不要为此改 `BattleSetup` 已有字段的含义**——老的战斗数据（14 张图上现有的十几场）
必须一字不改地继续按单波跑。这一条要有测试。

#### 2.2.1 实现方的裁决（2026-09-21 落地）

**照建议做：波次号挂在单位上。** `data/battles/<id>.json` 的 `units` 条目增一个可选整数：

```json
{ "role_id": "yelangbang_mazei", "x": 10, "y": 3, "faction": "enemy", "wave": 1 }
```

| 项 | 口径 |
| --- | --- |
| 缺省 | `0`，即**开场就在场上**。老数据一个字不改，全员 0，仍然只有一波 |
| 取值 | 非负整数。负数与非整数**加载时报错**，不夹到 0 悄悄收下 |
| 连续性 | 出现过的波次号必须构成 `0..N` 无空档，且第 0 波至少有一个单位；否则加载报错 |
| 入场时机 | **当前波的敌人全部倒下**时下一波入场；最后一波打完才 `Won`；中途我方全灭即 `Lost` |
| 行动序 | 中途入场的单位**下一回合**才轮到行动（行动序是回合开始时排的，他们刚走上战场） |
| 站位 | 编成写的那一格若被占，引擎就近挪到空格（否则两个单位叠一格，`unitIndexAt` 只认得出一个） |

为什么不另开 `waves: [[...], [...]]` 一张表：老数据不改是硬要求，另开一张表就意味着
`units` 与 `waves` 两处都能写单位，加载器要判「两处都写了算谁的」，而那种规则没人记得住。
波次只是「什么时候上场」，与 `x` / `y` / `faction` 同一层次，就该和它们并排。
战场尺寸、地形、能否逃跑仍是整场共用的——真让每波各带一套，「第二波把战场变大了」
这种写法就成立了，而那不是本章要的东西。

引擎侧（`core::battle::Unit`）对应两位新字段：`wave`（编成，只读的意图）与
`onField`（局面，此刻在不在场）。`BattleState::setup` 一律按 `onField = (wave <= 0)`
推出后者，不信调用方传进来的那一位。`Unit::alive()` 把 `onField` 一并算进去，
与 `fled` 同一个办法——行动序、选中、可否被打、胜负判定都只问这一个谓词。

**「波间不回血、不重置」靠结构守住，不靠纪律**：推波只把下一波的 `onField` 置真、
`currentWave_` 加一，此外一个字节也不动；`setup()` 不会被再调一次，既有单位的
气血 / 毒 / 罡气 / 位置 / 回合数没有任何代码路径碰得到。要破坏它得专门去写一段回血。

### 2.3 玩家要看得见

**进入下一波必须在界面上说得出来**（例如战斗日志上一行「第二波从西侧压上来」）。
玩家正打着突然多出六个敌人而没有任何提示，是本项目明令禁止的那类观感。

---

## 3. 炼制接线

### 3.1 范围

`Crafting.h` 已有：四类（炼丹 / 炼器 / 制符 / 阵法）、成功率、熟练度增长、失败策略、
工具品阶。`GameState` 已有四个熟练度字段。**规则层一个字不要重写，只做接线。**

本章只需要**炼丹**这一类真正可用；其余三类的入口可以留着但不必接内容。

### 3.2 面板

照 `src/game/FieldScene.*` / `ShopScene.*` / `BattleScene` 的动作菜单那一套写：

- 列出**当前配得出来的**方子；配不出来的**列出来但点不动，并写明缺什么**
  （缺料 / 缺炉 / 火候不够）。**静默失败是明令禁止的。**
- 成功与失败都要有明确反馈；失败按 `failurePolicyOf` 决定扣不扣料。
- 熟练度按 `proficiencyGain` 涨，落在 `GameState::alchemyProficiency` 上。

### 3.3 入口

地图上的 `facility` `kind=alchemy` 规范里早就留好了（`map_spec.md` §4.6）。
`Application::openFacility` 加这一支。

#### 3.3.1 落地情况与**一条要请主控改规范的属性**（2026-09-21）

`openFacility` 认四个 kind（`alchemy` / `talisman` / `forge` / `formation`），
四种走同一块面板（`src/game/AlchemyScene.*`）：判定与失败策略全在 `rules::Crafting`
里按 `CraftKind` 分流，game 层再照着分一次就成了两处真源。本章只有 `alchemy`
真的有内容（`data/recipes/alchemy` 下六张方子），其余三类列得出来、也配得出来，
只是本章地图上没有那三种炉子。

**炉鼎品阶从哪来：`facility` 的 `grade` 属性。** 它直接进成功率
（`rules::successChance`，每品阶 5 点，量程 0-5），是玩家能感知的数值。

> **`map_spec.md` §4.6 的属性表里还没有这一行。** 规范归主控与地图方，
> 实现方不单方面去改别人的规范（G-6 的教训），所以这里只写出要加的那一行：
>
> | 属性 | 类型 | 必填 | 说明 |
> | --- | --- | --- | --- |
> | `grade` | int | 否 | 炉鼎 / 符笔 / 地火 / 阵盘的品阶 0-5，缺省 1；`kind` 为炼制四艺之一时有意义 |
>
> 在它落地之前，**缺字段按 1 收**（`game::kDefaultCraftToolGrade`），老地图一个字
> 不改也能开炉。取 1 而不是 0：0 在规则层的意思是「没有炉鼎」，于是一处忘了写
> `grade` 的丹房会变成一间点不动任何东西的屋子，而玩家站在丹炉前看着
> 「身边没有丹炉，无从起火」只会当成 bug。

**一句提示语的翻译**：`rules::canCraft` 的拒绝理由里嵌的是**物品 id**
（「材料不足：herb_qingfeng_cao 需 2，现有 0」），因为规则层不认识物品册。
面板用 `game::humanizeReason` 只把**这张方子里真的出现过的** id 换成显示名，
除此之外一个字不改——**判据仍然只有 `canCraft` 一个**，换掉的只是名字。
有一条测试钉住「面板画的那句话 == humanizeReason(canCraft 给的那句话)」。

### 3.4 配方数据

`data/recipes/alchemy/` 下已有内容，**先看它们能不能加载**——
`src/io/` 里可能压根没有配方加载器。没有就写一个，校验要能拒绝坏数据
（材料 id 不存在、产出 id 不存在、成功率越界）。

---

## 4. 测试要求

**每一条新行为都要有测试，且要有负向用例。** 最低要求：

1. `magic.learn` 幂等；学不存在的 id **回填失败**（负向）。
2. 已习得法术存档往返；**4→5 迁移已登记**（负向对照：未登记的版本要在版本检查这一步被挡下）。
3. 韩立学会火弹术之后，战斗菜单里**真的能选到它**（走 `issuePlayerAction`，不是只看数据）。
4. 多波次：第二波确实在第一波清空后入场；**波间不回血**（负向：加上回血会让它转红）；
   **老的单波战斗一字不改仍然只有一波**。
5. 炼丹能成能败；失败按策略扣或不扣料；**缺料时点不动且写明缺什么**
   （断言那串字**确实有内容**，不要写成「找不到某个词就算过」）。
6. 配方加载器拒绝坏数据，各类坏法各一条。

**本项目最惨的一次事故是校验器的正则被吃掉转义、从此永远报通过而无人发现，
因为没人做过负向验证。只报告「测试通过」而没跑过负向用例的，视为没做完。**

已知的判据空转法见 `docs/README.md` 那张表（正则被吃、字符串变空、禁词表清空、
比值分母塌成 0、只钉一个点、陈旧二进制、读错日志、**判据是从被测物推导出来的**），
写断言前逐条对照。**最后那一条是第 3 章刚栽出来的，请特别注意**：
凡设计文档写死的取值，要有一条**不经过驱动、直接读数据、判据写死成设计原文**的断言。

---

## 5. 落地清单（2026-09-21，引擎批）

| 契约条目 | 落在哪 |
| --- | --- |
| 1.1 `GameState::learnedMagics` | `src/core/model/Types.h`（末尾追加，另有 `knowsMagic` / `learnMagic` / `forgetMagic`） |
| 1.2 存档 4 → 5 与迁移 | `src/io/SaveFile.h`（`kSaveVersion = 5`）、`src/io/SaveFile.cpp`（`{4, …}` 已登记，空操作） |
| 1.3 脚本 API | `scripts/common/api.lua` 的 `magic` 表；命令 `MagicLearn` / `MagicForget` 追加在 `CommandKind` 末尾；`__host.magic_knows` / `magic_count` |
| 1.4 战斗里取自存档 | `src/game/BattleScene.cpp` 的 `buildFromSetup`（兜底遭遇 `buildProbeEncounter` 一并接上） |
| 2 多波次 | `core::BattleUnitSpec::wave`、`core::battle::Unit::{wave,onField}`、`BattleState::{currentWave,waveCount,deployNextWave,placeOnFreeCell}`、`io/BattleLoader` 的三道闸 |
| 2.3 界面上说得出来 | 日志一行（`deployNextWave`）+ 常驻一行（`game::waveStatusText`，单波返回空串） |
| 3 炼制接线 | `src/io/RecipeLoader.*`（新建）、`src/game/AlchemyScene.*`（新建）、`Application::{init,recipes,openFacility}` |
| 4 测试 | `tests/Ch04MagicTests.cpp`、`tests/Ch04WaveTests.cpp`、`tests/Ch04AlchemyTests.cpp`，共 58 条 |

**规则层一个字没改**：`src/core/rules/Crafting.{h,cpp}` 在本批里一行未动，只是接线。

### 5.1 一条要请主控裁决的跨界冲突

`kSaveVersion` 4 → 5 是本契约 1.2 节的硬要求，而 `tests/Ch03PartyTests.cpp` 里有两处
把 4 钉死，**该文件在本批的禁改名单里**（另有一人同时在 `tests/Ch03*.cpp` 作业）。
实现方按 G-6 的先例不单方面去改别人的文件，把要改的两处连原文写在交付报告里。

---

## 6. 境界：攻防曲线与提升命令（2026-09-22 增补，付清 G-10 / CRITICAL-1）

本节是事后增补，起因是复审的 CRITICAL-1：这一章从第 3 章交过来的存档打不通，
而通关测试用一个游戏里到不了的境界（炼气十三层）把这件事盖住了。
病根有两条，本节各给一条契约。

### 6.1 境界给攻防（规则层）

`realmAttack(Realm)` / `realmDefence(Realm)`，与既有的 `realmMaxHp` / `realmMaxMp`
并排、同一个体例（`src/core/rules/Realm.h`）。

```
炼气期： 攻 = max(6, round(0.6 + 1.8 × 层数))
         防 = max(3, 层数)
凡人：   一律 6 / 3
非法：   返回凡人值
```

逐层取值：

| 层 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 攻 | 6 | 6 | **6** | 8 | 10 | 11 | 13 | 15 | 17 | 19 | 20 | 22 | 24 |
| 防 | 3 | 3 | **3** | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 |

曲线从 `data/roles` 反解（攻 ≈ 3 + 1.8×层、防 ≈ 4 + 层），**整条锚在
「炼气三层 = 攻 6 / 防 3」上**。锚定是硬要求：`data/roles/jiang_shou.json` 与
`data/battles/b03_andao_shishou.json` 的说明写死了「韩立平砍 6*2-9=3」
「本章的尺度是韩立攻 6」，不锚定会当场打坏第 3 章的两场教学战。
锚定之后**第 1-3 章一个数不动**，涨的部分全落在还没施工的章节里。

筑基以上按 `realmMaxHp` 那种显式 switch 写，取值**暂定**（筑基初 32/20、
筑基中 37/26、筑基后 44/30、结丹初 62/46、结丹中 76/58、结丹后 102/82），
**到那一章再标定**：那几档一场仗都还没编，没有任何一场战斗能验证它们。

战斗侧：`BattleScene::buildFromSetup` 与 `buildProbeEncounter` 里韩立那一位的
`attack` / `defence` **取自这两条函数**，不再是字面量 6/3。

**识海那一场（`terrain = "mind"`）不另开攻防口径**，这是判断，不是遗漏：
体积（`maxHp`）特判是因为它**真的被 `devourBite` 读**，而原著那三条体积关系
是剧情硬约束；攻防在吞噬模式下压根不参与
（`BattleAction.cpp` 的 `devour_ ? devourBite(...) : physicalDamage(...)`），
再写一条 `mind ? 6 : ...` 只会造出一段永远不被读、却长得像规则的死代码。

### 6.2 脚本命令 `realm_advance`

命令 kind **追加在 `CommandKind` 末尾**（与 `MagicLearn` 同一条铁律）。
字段占用：`x` = 目标境界编号，其余不用。

```lua
-- api.lua 的糖衣（见 6.4：这一行还没落地，scripts/** 不在本批白名单里）
realm.advance(realm.QI_REFINING_7)   -- 提升到目标境界
realm.level()                        -- 只读，当前境界编号（__host.realm_value）
realm.at_least(value)                -- 既有，只读，bool
```

写走命令队列，读走 `__host`，与既有分界一致。

**口径（与 `PartyAdd` / `MagicLearn` 逐条对齐）：**

| 情形 | `ok` | `code` | 做了什么 |
| --- | --- | --- | --- |
| 目标境界编号非法（落在 enum 空段） | `false` | `no_realm` | 一个字节都不动 |
| 目标 **低于** 当前 | `false` | `not_higher` | 一个字节都不动 |
| 目标 **等于** 当前 | `true` | — | 什么都不做（幂等） |
| 目标高于当前 | `true` | — | 换境界，并补齐 `maxHp` / `maxMp` |

`ok` 回答的是**「这条命令要的那个结果成不成立」**，不是「有没有改动过东西」——
这正是 `PartyAdd` 那条「已在队里时 `partyAdd` 返回 false，但 ok 保持 true」的口径。
于是「目标 == 当前」判 `true`（结果已经成立，剧情回放与读档重跑都不该走进失败分支），
而「目标 < 当前」判 `false`（结果不成立，而这条命令也不打算让它成立）。
**不跟着 `PartyAdd` 把降级也判 true**：那会让「脚本把境界写小了」与
「他本来就更高」在脚本侧长得一模一样，而前者是个 bug。

**只许升不许降。** 跌落是第 9 章 `rules::Realm::demote` 的事：那一章要决定掉境界
跟不跟着削气血，而这条命令什么也决定不了，只会留下一个上限远高于境界基准的主角。

**升上去之后 `maxHp` / `maxMp` 必须跟着长**（`realmMaxHp` / `realmMaxMp`）。
**当前值按差额上抬**（`rules::liftToFloor`），**不补满**：

1. 突破在打坐面板里已经有一套口径（`CultivationScene::applyRealmAttributes`
   走的就是 `liftToFloor`），同一件事经脚本发生与经面板发生必须是同一个结果；
   界面各算一套是数值失控最常见的来源。
2. 补满会让这条命令顺带成为一次免费全恢复；而 `BattleScene::finish` 会把气血
   写回存档，这个差别玩家看得见。
3. 玩家那句直觉「突破之后是满的」照样成立：突破的常态是打坐时，那时本来就是满的，
   `liftToFloor` 把当前值抬同样的量，满血进满血出。只有带伤突破的人会带着那道伤
   出来——**突破不是疗伤**。

### 6.3 `__host.realm_value()`

返回**当前境界编号**，与 api.lua 里 `realm.QI_REFINING_7 = 7` 那组常量、
与 `realm_at_least` 收的那个数同一套坐标。炼气期的编号恰好就是层数（1-13），
这正是 enum 当初这样编号的原因；筑基以上是 21/22/23、31/32/33。

**刻意不另开一个只数层数的查询**：那个数在炼气期是 1-13、在筑基期是 1-3，
同一个变量在不同大境界里意思不同，写闸门时几乎必然出错。要分档请配 `realm_at_least`。

### 6.4 落地清单与**一处还没接上的线**

| 契约条目 | 落在哪 |
| --- | --- |
| 6.1 两条曲线 | `src/core/rules/Realm.{h,cpp}`（`realmAttack` / `realmDefence`） |
| 6.1 战斗侧取用 | `src/game/BattleScene.cpp`（`buildFromSetup` 与 `buildProbeEncounter`） |
| 6.2 命令 | `src/script/Command.h`（`RealmAdvance` 追加在末尾 + 字段占用表）、`src/script/ScriptHost.cpp`（`realm_advance`）、`src/game/Application.cpp` 的 `dispatch` |
| 6.3 只读查询 | `src/script/ScriptHost.cpp`（`__host.realm_value`） |
| 测试 | `tests/RealmTests.cpp`（6 条，逐层钉死）、`tests/ScriptApiTests.cpp`（9 条） |

**还差 api.lua 的糖衣。** `scripts/common/api.lua` 不在本批的文件白名单内
（第 4 章的编剧正同时在改 `scripts/**`），所以 `realm.advance` / `realm.level`
这两行还没有落地。眼下脚本只能写成命令表原形
（`coroutine.yield{ kind = "realm_advance", x = 7 }`），这不该是剧情脚本的写法。
**接手 `scripts/**` 的人请把下面这一段并进 `api.lua` 的 `realm` 表**，
形状照 `party.add` / `magic.learn`（凡调请看返回值）：

```lua
    -- 提升境界。只许升不许降：目标低于当前会**返回 false**（原因码 "not_higher"），
    -- 目标等于当前什么都不做但返回 true（结果已经成立，与 party.add 同一口径）。
    -- 升上去之后气血 / 法力上限会跟着长，当前值按差额上抬（突破不是疗伤）。
    --
    -- 原因码：
    --   "no_realm"    不是一个合法的境界编号
    --   "not_higher"  目标低于当前（跌落是 Realm::demote 的事，不是这条命令的事）
    advance = function(value)
        assert(type(value) == "number", "realm.advance() 需要境界编号")
        local result = emit{ kind = "realm_advance", x = value }
        return result.ok == true, result.code or ""
    end,

    -- 当前境界编号。炼气期的编号就是层数（1-13）；筑基以上是 21/22/23、31/32/33，
    -- 要分档请用 at_least，不要拿它当「第几层」去算。
    level = function() return __host.realm_value() end,
```

在这一段落地之前，**这条命令在剧情脚本里事实上还用不上**——
那正是 `docs/README.md` 那张「一个模块可用需要三样齐备」表里的第 3 段。
