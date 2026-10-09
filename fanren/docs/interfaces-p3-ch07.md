# P3 第 7 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-09-29 · 依据 `docs/ch07-design.md` 第 10 节（引擎前置）、第 7 节（系统解锁）、第 8 节（编成）、第 11 节（旗标）、第 12 节（禁词）。
> 本批引擎路做 E1（配方按旗标列出）、E2（道具施法与「削架势」——第 6 章拍板放到本章的「符箓进战斗」）、E3（瓶子容量随境界）、E4（脚本按年份下限计数与扣物）、E7（陆师兄改写牵动的曲线锚点与注释）。
> 门禁（`tools/validate.py`、`tools/validate_selftest.py`）**整份归协调者**：形状检查在派工之前先落（第 5.1 节），规则 25 的表在集成时落（第 5.2 节）。引擎路不碰 `tools/`。
> 可选 O1（修炼面板「师承」一行）**推荐不做**，本批不做；O2（踏云靴身法）、O3（禁地专用主题与战斗背景）不做。

---

## 0. 五件事，各自现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 定颜丹方、筑基丹方在藏室那一节之前不出现在炼制面板上；第 4–6 章面板上提前露面的「结丹灵药方」与四张制符方一并收起来 | `rules::Recipe`（`src/core/rules/Crafting.h`）没有任何「已得」的字段；`AlchemyScene::visibleRecipes(all, kind)`（`src/game/AlchemyScene.cpp`）按门类把**全部**方子列出——第 4 章丹房起面板上就有「定颜丹方」「结丹灵药方」，第 6 章制符桌上护身符方的缺料理由里写着「一级妖丹」（妖丹首见 ch344）。`docs/interfaces-p3-ch04.md` 3.2「配不出来的列出来但点不动」管的是**已得而配不齐**，本契约不动它 |
| E2 | 天雷子、土牢符、火球符、定神符在战斗里用得出去；土牢、定神是「削架势」而不是「伤血」 | `core::Item`（`src/core/model/Types.h`）的战斗效果只有回血、回法、下毒、解毒四样（`battleUsable`）；`MagicEffect` 只有 `None / Reveal`（第 6 章 E1）；`BattleState::checkItem` 把一切不带毒的物品挡在「丹药只能给自己人用」；符箓至今只能卖 |
| E3 | 筑基之后瓶子装得下 6 滴、结丹 9 滴 | `rules::Bottle::capacity` 缺省 3；`Bottle.h` 注释写明「由 game 层按境界写进」，并给了具名基准 `kBottleCapacityPerTier = 3`——**全仓没有一处写它**（只有 `SaveFile` 读写）；凝液间隔倒是早已按境界分档（`Application.cpp` 的 `bottleChargeDays`） |
| E4 | 脚本能问「年份不低于 N 的有几株」，能「从够格的里扣 N 株」 | `item.count(id)` 数的是所有年份的总和；`take(id, n)` 走 `GameState::removeItem`——**先扣年份最低的**；`take(id, n, age)` 走 `removeItemOfAge`——只扣**恰好**那个年份（`Application.cpp` 的 `CommandKind::TakeItem`）。两样都答不了「交三株四十四年以上的黄精」 |
| E7 | 陆师兄改成十二层之后，曲线锚点与注释表跟上 | `tests/RealmTests.cpp` 锚点 `{Realm::QiRefining11, 155, "lu_shixiong"}`（字面量，不读数据）；`src/core/rules/Realm.h` 注释表「炼气十一层　lu_shixiong 155」与「lu_shixiong(十一层 155≈156)」 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。E1–E4 三段都在本批里落。

---

## 1. E1 配方按旗标列出

### 1.1 一句话口径

**配方可以带一面 `requireFlag`：旗标未置 = 这张方子「还没得到」，炼制面板不列、也开不了工；已得的方子照 `interfaces-p3-ch04.md` 3.2 办——配不齐也列出来、置灰、写明缺什么。**

### 1.2 数据

- 新字段 **`requireFlag`**（配方 JSON，驼峰，与 `requiredProficiency` 同一写法）：字符串；缺省 = 没有门闸（现有方子不写，行为零变化）。写了就必须非空；旗标是否已登记由门禁既有的 `*_flag` 检查管（`check_data_references`，字段名归一化后以 `_flag` 结尾），不另立规则。
- 引擎路改的 5 份（6.1，**只加这一个字段**）：`data/recipes/alchemy/jiedan_lingyao.json`、`data/recipes/talisman/{huoqiu_fu,hushen_fu,jinci_fu,leiming_fu}.json` → `"requireFlag": "story.recipe_later"`。`dingshen_fu.json`（第 6 章的定神符方）**不加**。
- 内容路改写的 2 份（6.2）：`zhuji_dan.json`、`dingyan_dan.json` → `"requireFlag": "ch07.cangshi"`，输入与难度见 `docs/ch07-design.md` 第 7 节。
- `story.recipe_later`：**永不置**的占位旗标（协调者登记，描述写「写到那一章时换成那一章的旗标」）。只许配方用（门禁 5.1 第 5 条）。

### 1.3 规则层（`src/core/rules/Crafting.h/.cpp`）

- `Recipe` 末尾追加 `std::string requireFlag;`（按字段顺序的聚合初始化不受影响）。
- 新谓词 **`recipeKnown(const Recipe&, const core::GameState&)`**：`requireFlag` 为空，或 `state.flag(requireFlag) != 0`。面板与开工只问它这一个函数。
- `canCraft` 在「配方完整」之后、「炉鼎」之前加一步：`!recipeKnown` → 失败，理由「手边还没有这张方子」。注释里的顺序承诺改成「配方 → 已得 → 炉鼎 → 熟练度 → 材料 → 年份」；既有的顺序用例不受影响（它们用的方子都没有门闸）。`craft` 走 `canCraft`，自然跟上。

### 1.4 加载器（`src/io/RecipeLoader.cpp`）

- 读 `requireFlag`（可缺省）；非字符串或空串**加载期报错**，报错点名文件与字段（照 `requiredProficiency` 越界那一条的写法）。

### 1.5 游戏层（`src/game/AlchemyScene.*`）

- `visibleRecipes(all, kind, state)`：多收一个 `const core::GameState&`，只列 `recipeKnown` 的；行序仍按 id 字典序。**旧的两参签名删掉**（留着就会有人绕过门闸）。
- `enterList` 用新签名；`recipeIds_` 与列表同源（现状已是）；`craftAt` 按 id 重查之后照旧问 `canCraft`（门闸在里头）。
- 一门手艺一张已得的方子也没有：列表只剩「离开」，状态栏照旧，不另加话。

### 1.6 判据（`tests/Ch07EngineTests.cpp`，引擎路写）

1. 数据：`requireFlag` 读得进；空串、数字各报错且点名文件与字段；五份门闸方子读出来都是 `story.recipe_later`，定神符方没有门闸。
2. 面板：一个 `story.recipe_later` 未置的存档（用 `tests/fixtures/ch06-end-first.sav`；第 6 章夹具若尚未集成，用空存档并在实现记录里写明），炼丹面板里没有结丹灵药方、制符面板里没有那四张（定神符方在）；置 `story.recipe_later` 之后都在；两态的行数都与 `visibleRecipes` 同源。
3. `canCraft`：未得的方子理由是「手边还没有这张方子」，而且排在炉鼎之前（无炉 ＋ 未得 → 报未得）。
4. 行号：同一个面板，旗标 0 / 1 两态下，`craftAt(i)` 开的都是第 i 行写的那张方子。
5. 既有用例：`tests/Ch04AlchemyTests.cpp` 的 `rows()`、`indexOfRecipe()`、`EveryRecipeOfTheCraftIsListedEvenWhenItCannotBeMade` 改用三参签名；`TooLowAProficiencySaysSoInsteadOfBlamingTheMaterials`（结丹灵药方）与 `ATalismanFailureSparesTheCatalystWhileAlchemyBurnsEverything`（护身符方）在取行之前先置 `story.recipe_later`——**判据一个字不改**。`tests/Ch04SliceTests.cpp` 两处（约 980、2195 行）只改签名。
6. **负向自检**（改坏 → 构建 → 确实红 → 还原）：`visibleRecipes` 去掉 `recipeKnown` 过滤 → 第 2 条红；`canCraft` 去掉那一步 → 第 3 条红。

### 1.7 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch07eng`。

- **规则层**：`Recipe` 末尾加 `requireFlag`；`rules::recipeKnown(recipe, state)`（空串或旗标非 0）；`canCraft` 在「配方残缺」之后、「炉鼎」之前加一步，理由写成 **「手边还没有这张方子。」**（带句号，与同一层「这张方子残缺不全，无从下手。」等几句一致）；`Crafting.h` 的顺序承诺改成「配方 → 已得 → 炉鼎 → 熟练度 → 材料 → 年份」。
- **加载器**：`RecipeLoader` 读 `requireFlag`；写了而不是非空字符串（空串、数字）报「配方 X 的 requireFlag 必须是非空字符串：路径」。
- **游戏层**：`AlchemyScene::visibleRecipes(all, kind, state)` 只列 `recipeKnown` 的，两参签名删掉；`enterList` 用三参，`recipeIds_` 与列表同源；`craftAt` 照旧按 id 重查后问 `canCraft`。一门手艺一张都没到手时列表只剩「离开」，没加话。
- **数据**：五份方子在 `requiredProficiency` 下一行加 `"requireFlag": "story.recipe_later"`，别的一字未动。
- **既有用例**：`Ch04AlchemyTests` 的 `rows()`、`indexOfRecipe()`、`EveryRecipeOfTheCraftIsListedEvenWhenItCannotBeMade` 改三参；结丹灵药方、护身符方两条在开头先置 `story.recipe_later`（护身符那条必须在 `scene.onEnter` 之前——面板的行表是进面板那一刻建的），判据未改。`Ch04SliceTests` 两处改三参；其中 `recipeRow` 去掉了 `const`（三参要拿存档，`Application::state()` 没有 const 版；两个调用方都不是 const）。
- **判据**：`tests/Ch07EngineTests.cpp` 的 `Ch07RecipeGateData`（1 条）、`Ch07RecipeGateRule`（1）、`Ch07RecipePanel`（3 条：五张门闸 / 第 6 章终局存档两态 / 两态行号）。存档用的就是 `tests/fixtures/ch06-end-first.sav`（第 6 章夹具已在 WT 里）；行号那条的判据是开炉那一句里的「《方子名》」——成与不成都带着它。
- **负向自检**：见末尾「实现记录」。

---

## 2. E2 道具施法与「削架势」

### 2.1 一句话口径

**物品可以带一门 `castMagic`：在战斗里用它 = 以那门法术施展一次，不耗法力、不吃蓄劲、扣一件。新效果 `stagger` 让一门法术不伤血、只削目标 N 点架势——削到 0 就破势，走的是「打中破绽削到 0」那一条既有的路。**

### 2.2 数据

- 物品新字段 **`castMagic`**（`data/items/**`，驼峰）：法术 id，缺省没有。写了就：
  - 必须指向存在的法术，且那门法术 `castableMagic`（伤人或带效果；护身罡这类 `power 0`、无效果的不行）；
  - 与 `restoreHp` / `restoreMp` / `poison` / `curesPoison` **互斥**：一件东西要么是药、要么是符，同写报错。
- 法术 `effect` 增一个取值 **`"stagger"`**，配新字段 **`stagger`**：整数 1–9，缺省 1；只许与 `effect: "stagger"` 同写；带 `stagger` 效果的法术与 `power > 0` / `poison > 0` 同写报错（与 `reveal` 同一条口径）。
- 引擎路建的数据（6.1）——数值是初值，归平衡路：

```json
{
  "id": "magic_tulao_shu",
  "name": "土牢术",
  "descKey": "magic.desc.magic_tulao_shu",
  "element": 16,
  "needMp": 0,
  "power": 0,
  "needRealm": "QiRefining1",
  "effect": "stagger",
  "stagger": 3,
  "origin": "改编",
  "note": "只供土牢符施放，不学（全仓 magic.learn 0 处）。原著 ch183 高阶土牢符困住络腮胡子：游戏里做成一下削掉三点架势。"
}
```

  - `magic_dingshen_shu` 定神术：`element 0`、`effect "stagger"`、`stagger 1`、`needMp 0`、`power 0`（第 6 章设计 10.1 E2 早就写好的口径：定神符削 1 点架势）。
  - `magic_tianleizi` 天雷子：`element 8`（火）、`power` 初值 60、`needMp 0`、`boost "power"`，无 `effect`（它是伤人的）；note 写明只供道具、筑基硬抗也化灰（ch163）、③ 那一仗的原著解法。
  - 物品 `talisman_tianleizi`（天雷子，`kind` 与现有符箓同、`tradeable false`、`castMagic "magic_tianleizi"`）、`talisman_tulao_fu`（土牢符，`tradeable false`、`castMagic "magic_tulao_shu"`）。
  - 既有 `talisman_huoqiu_fu` 加 `"castMagic": "magic_huoqiu_shu"`；既有 `talisman_dingshen_fu` 加 `"castMagic": "magic_dingshen_shu"`——**只加这一个字段**。
  - 文案 `data/text/ch07_e2.json`（新）：`item.desc.talisman_tianleizi`、`item.desc.talisman_tulao_fu`、`magic.desc.magic_tianleizi`、`magic.desc.magic_tulao_shu`、`magic.desc.magic_dingshen_shu` 五条；措辞守 `docs/ch07-design.md` 第 12、13 节（天雷子首见 ch163，`LexiconTests` 按文件名认章，文件名以 `ch07` 开头即归第 7 章）。

### 2.3 规则层（`src/core/model/Types.h`、`src/core/battle/`）

- `Item` 末尾追加 `std::string castMagic;`；`battleUsable` 加一条 `!item.castMagic.empty()`，注释里「五个字段」那句跟着改。
- `MagicEffect` 追加 `Stagger`（`None, Reveal, Stagger`，追加在末尾）；`Magic` 末尾追加 `int stagger = 1;`。`castableMagic` 不用改（`effect != None` 已放行）。
- **用一件带 `castMagic` 的物品**（`ActionKind::Item`）：
  - 选目标按那门法术走：伤人的与 `stagger` 选敌人（照 `checkEnemyTarget`）；`reveal` 不挑目标（照第 6 章 1.6）。`checkItem` 里「丹药只能给自己人用」那一条对它不适用。
  - 结算与 `ActionKind::Cast` 施展那门法术**逐项相同**（伤害公式、五行相克、境界压制按使用者算、破绽判定、削架势、破势、打断蓄势、揭开破绽），**只差三处**：① 不扣法力、不查法力；② 不吃蓄劲（`checkBoost` 对 `Item` 本来就回绝非 0 的劲——保持）；③ 事件流里 `Act` 那一行 `value = Item`，日志「韩立 使用 天雷子——……」。
  - `actionCategories` 对它返回那门法术的 `magicCategories`（打得中破绽）；`hitCount` 恒 1（`boost: hits` 的法术也只打一发——不吃劲）；`estimateHitDamage` 按 `Cast`、劲 0 算（给界面预估用）。
  - 背包扣减照旧由 game 层做（`BattleScene` 里 `removeItem(action.magicId, 1)` 那一处）；**被回绝的动作不扣**（既有口径）。
- **`stagger` 效果**（法术施展与道具施法走同一条）：
  - 目标：一个敌方单位。目标**没有架势**（`maxToughness == 0`）或**已经破势** → **回绝**（`checkCast` / `checkItem`，理由「X 没有架势可削」「X 已经破势」），不扣法力、不扣物品——与「解毒药用在没中毒的人身上」同一口径。
  - 效果：目标架势减 `stagger`（下限 0）；到 0 即破势——走「打中破绽削到 0」那一条既有路径（同样的破势回合数、同样打断蓄势、同样的 `Break` 事件）。**不判破绽、不揭开破绽、不造成伤害、不出伤害数字**。
  - 蓄劲：对它无效（蓄了只当没蓄，`checkBoost` 放行、不扣劲——与看破同一口径）。
  - 事件：`Act` ＋ 一条新的 **`BattleEventKind::Stagger`**（追加在枚举末尾；actor → target，`value` = 削掉的点数，`toughness` = 削后的架势）；削到 0 时紧接既有的 `Break`。
- AI：敌方 AI 拿不到（敌人不带物品，法术表里本章不写 `stagger`）；我方 AI（无头 `runToCompletion`）**不用**带 `castMagic` 的物品、也不施 `stagger` 法术（候选循环照旧先问 `offensiveMagic`）——既有无头测试的走向一步不变。

### 2.4 加载器（`src/io/DataLoader.cpp`）

- 物品：读 `castMagic`；非字符串 / 空串报错；与四样药效同写报错。引用不存在的法术、或指向不可施展的法术，放在物品与法术**都读完之后**的汇总处报错。每条报错点名文件与字段。
- 法术：`effect` 认 `"reveal"`、`"stagger"`，别的（含大小写不同、带空格、空串）报错；`stagger` 字段只许与 `effect: "stagger"` 同写、须为 1–9 的整数；`effect: "stagger"` 而不写 `stagger` → 取 1。

### 2.5 游戏层（`src/game/BattleScene.cpp` / `BattleView` / `BattleHud`）

- 物品菜单：`battleUsable` 已放行；选中带 `castMagic` 的，下一级按那门法术选目标（敌方；`reveal` 不选）；行尾写那门法术的名字与类别（例如「天雷子　火」「土牢符　削架势 3」），不写法力。
- 播放：伤人的复用那门法术的特效；`Stagger` 事件播一段「架势格被削」（复用打中破绽时架势减少的画法），不新画资源。

### 2.6 判据（`tests/Ch07EngineTests.cpp`）

1. 数据：四件符的 `castMagic` 与 2.2 一致；`castMagic` 拼错 / 指向护身罡 / 与 `restoreHp` 同写，各报错且点名文件与字段；`stagger` 写 0、10、`"3"`、与 `power 12` 同写，各报错；`effect: "Stagger"` 报错。
2. 规则：一场一个敌人（架势 3、破绽「火」），韩立法力 50——用天雷子 → 敌人掉血、「火」揭开、架势 −1，法力仍 50；再用土牢符 → 架势 2 → 0、破势（`Break` 事件在），法力仍 50。
3. 回绝：对已破势的敌人用土牢符 → 回绝、理由含「已经破势」、背包件数不变（走 `BattleScene` 真入口）；对没有架势的单位同理。
4. 蓄劲：蓄 2 点劲用天雷子 → 回绝（原句「只有攻击与法术蓄得了劲」）；蓄 2 点劲施一门 `stagger` 法术（测试里就地造一门）→ 效果与不蓄相同，劲不扣。
5. 真入口：`b07_*` 编成还没落地，**用既有编成**（例如 `b04_qiecuo_maliu`），背包放天雷子一件、火球符一件，走真菜单用掉 → 背包各少一件、法力一点不少、日志有「使用 天雷子」。
6. AI：`runToCompletion` 跑一场背包里有天雷子与土牢符的仗，两件一件不少。
7. **负向自检**：`battleUsable` 去掉 `castMagic` 那一条 → 第 5 条红；道具施法照扣法力 → 第 2 条红；`stagger` 不查「已破势」→ 第 3 条红；我方 AI 去掉过滤 → 第 6 条红（若第 6 条咬不住，照第 6 章 1.6 的写法在实现记录里如实写「没有能咬住的用例」）。

### 2.7 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch07eng`。

- **事件种类 `BattleEventKind::Stagger`**（追加在 `Reveal` 之后）：一手削架势 = `Act`（actor → target；value = `Cast` 或 `Item`；`category` = 那门法术的五行，只给画面挑施法光的颜色，这一手不判破绽）＋ `Stagger`（actor → target；`value` = 实削点数；`toughness` = 削后的架势；`hp` = 目标气血）＋ 削到 0 时 `Break`（蓄势中的再接 `ChargeInterrupt`）——与 `emitStrike` 共用新拆出来的 `emitBreak`，次序、`Break.value`（破势回合数）一模一样。没有 `Hit`、没有伤害数字、没有 `BoostSpent`。
- **`stagger` 的数值口径**：削 `min(stagger, 当前架势)` 点（下限 0），事件里记实削的；到 0 走 `breakUnit`（与打中破绽削到 0 同一条：同样的恢复回合、同样打断蓄势）。不判破绽、不揭开、不记 `tested`。蓄劲：`checkBoost` 对 effect ≠ None 的施法本来就放行（看破那一条），`applyStagger` 不花劲、不置 `boosted`。没有架势（`maxToughness <= 0`）/ 已经破势 → `checkMagicTarget` 回绝「X 没有架势可削」「X 已经破势」，法力、物品都不扣。
- **道具施法**：`checkItem` 对带 `castMagic` 的物品改走 `checkMagicTarget`（与 `checkCast` 共用：看破不挑目标；伤人的与削架势的挑一个敌人）；**不查法力、不查习得、不查 `needRealm`**（符的力气在纸上；本章三门都是 `QiRefining1`，查不查结果一样）。结算 `applyItem` → `resolveMagic(a, magic, &item)`——就是施法用的那一个函数（`applyCast` 也只剩一句调它），只差契约那三处：不扣法力；不花劲（`checkBoost` 对带劲的物品照旧回绝「只有攻击与法术蓄得了劲」，所以 `a.boost` 恒 0、`hitCount` 恒 1）；`Act.value = Item`、说法「韩立 使用 天雷子——击中 X，造成 N 点伤害，破绽「火」」。装着看破的符同理（「韩立 使用 X——Y 的破绽「…」尽收眼底」，本章没有这样的数据，有一条用例钉着）。`hitCategories` / `estimateHitDamage` 对它按那门法术算（`castMagicOf`）。
- **AI**：代码没动，只改注释。我方候选循环照旧先问 `offensiveMagic`（削架势的不进候选），物品从来不在我方 AI 的候选里（「不吃药」），敌方不带物品。
- **加载器**：物品 `castMagic` 非字符串 / 空串报「物品 "id" 的 castMagic 必须是非空的法术 id: 路径」；与四样药效（数值 > 0、`curesPoison` 为 true）同写报；指向不存在 / 施展不了的法术在 `loadGameDataImpl` 读完法术之后的汇总处报，点名物品文件。法术 `effect` 只认 `"reveal"` / `"stagger"`（报错句改成「effect 只许 "reveal" 或 "stagger"」，第 6 章那几条用例照旧绿）；`stagger` 只许与 effect stagger 同写、1–9 整数，否则报；不写取 1。
- **游戏层**：`registerBagItems` 把 `castMagic` 那门法术随物品一起 `addMagic`（各单位 `magicsExhaustive`，谁也没因此学会它；法术列表 = 自己那份清单 ∩ 登记表，天雷子不会出现在法术栏）。物品列表行尾「余 N · 火」「余 N · 削架势 3」——照契约的例子只写类别 / 削几点，不重复法术名（与物品名重复）、不写法力。带 `castMagic` 的物品择敌只列敌人，`refuseOnAnyone` 往敌人那边试。**顺手修了一处必须修的**：`menuChoose` 从前对 effect ≠ None 的法术一律不择敌、直接发——削架势的法术（内容路的祭青凝镜）会被当成看破、无目标发出去被回绝；改成只认 `Reveal`，道具里的看破同样直接发。法术列表行尾对削架势的法术写「削架势 N」而不写五行、不写「蓄劲加威」。`BattleView::onStagger`（复用 `onHit` 里架势减少那一段：`shieldHit` ＋ `shieldFrom` 让旧数字往下掉，一把金色火花，`hit_weak` 音）；`onAct` 对带类别的 `Item` 复用施法光与施法声；`BattleScene::playEvent` 加 `Stagger` 分支（更新显示的架势）。`BattleHud` 没改（`shieldFrom` 本来就按任意差值画）。
- **数据**：`data/magics/{tulao_shu,dingshen_shu,tianleizi}.json`、`data/items/talismans/{tianleizi,tulao_fu}.json`（天雷子 grade 3、price 0；土牢符 grade 1、price 0；两件都 `tradeable false`）、火球符 / 定神符只加 `castMagic`、`data/text/ch07_e2.json` 五条（对全本最长公共片段 ≤ 5 字，本地核过）。原著事实本地核过：天雷子首见 ch163、ch189 封岳；土牢术 ch183–184 络腮胡子；定神术 ch131 / ch138。
- **测试**：`Ch07ItemCastData` ×2、`Ch07ItemCast` ×4、`Ch07Stagger` ×2、`Ch07ItemCastScene` ×5（天雷子 ＋ 火球符走真菜单、土牢符对破势敌人回绝、定神符削 1 点、法术栏里的削架势要择敌、无头 AI 不掏符）、`Ch07RecipePanel.TheFourTalismansCastWhatTheContractSays`。规则层那两门法术在用例里就地造、**法力写成非 0**（数据里是 0，扣不扣看不出来）；真入口用 `b04_qiecuo_maliu`。
- **已知**：`talisman_dingshen_fu` 的 note 末句「本作不给它战斗用途（E2 不做）」现在不对了——本路只许加 `castMagic` 一个字段，没改，交协调者。

---

## 3. E3 瓶子容量随境界

### 3.1 一句话口径

**瓶子容量的下限随大境界走：凡人与炼气 3、筑基 6、结丹 9（`kBottleCapacityPerTier` × 档位）；只补不削——已经更大的（将来剧情道具给的加成）一滴也不动。抬上限不送液。**

### 3.2 规则层（`src/core/rules/Bottle.h`）

- 新纯函数 **`bottleCapacityFloor(RealmTier)`**：`Mortal` / `QiRefining` → 3，`Foundation` → 6，`Core` → 9，更高档按 9（本作用不上，注释写明）。`Bottle.h` 那段「rules 层不写死」的注释不改意思：这是具名基准，写进 `capacity` 的仍是 game / io 层。

### 3.3 三处（同一个函数，注释互相指）

1. `Application` 的 `CommandKind::RealmAdvance`（脚本升境，`liftToFloor` 那一段旁边）；
2. `CultivationScene::applyRealmAttributes`（面板突破）；
3. `SaveFile` 读档（与气血法力的 `liftToFloor` 同一段：先读出 `capacity`，再按境界抬）。

写法一律 `capacity = max(capacity, bottleCapacityFloor(tierOf(realm)))`。境界往回拨不削；`drops` 不动。

### 3.4 判据（`tests/Ch07EngineTests.cpp`）

1. 纯函数：四档各一条。
2. 脚本升境：炼气十三层、容量 3、滴数 3 → `realm.advance(21)` → 容量 6、滴数仍 3。
3. 面板突破：十二层 → 十三层，容量仍 3（同档不变）。
4. 读档：存档里筑基初期、容量 3（老档）→ 读进来 6；存档里容量 8 → 读进来仍 8。
5. **负向自检**：三处任删一处 → 第 2 / 3 / 4 条里对应的那一条红（第 3 条换成跨档的「十三层 → 筑基初期」再验一次，才咬得住面板那一处）。

### 3.5 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch07eng`。

- **纯函数**：`rules::bottleCapacityFloor(RealmTier)`（`Bottle.h`，`constexpr`）：凡人 / 炼气 3、筑基 6、结丹 9（`kBottleCapacityPerTier` × 档位），越界的档按 9；`Bottle.h` 因此 include `Realm.h`（后者不依赖前者，没有环）。「rules 层不写死」那段注释的意思不变，新注释写明写进 `capacity` 的是哪三处。
- **三处**（写法一律 `capacity = max(capacity, bottleCapacityFloor(tierOf(realm)))`，注释互相指）：① `Application` 的 `RealmAdvance`，放在气血法力 `liftToFloor` 之后、只在真升上去的那一支（原地不动、往回拨在前面已返回）；② `CultivationScene.cpp` 的 `applyRealmAttributes`（面板突破成功才调它）；③ `SaveFile.cpp` 的 `fromJson`，读完 `bottle` 块之后（存档里有没有 `bottle` 块都抬）。`drops` 三处都不动。
- **与契约的一处偏差**：契约写「与气血法力的 `liftToFloor` 同一段」。`SaveFile` 里那段 `liftToFloor` 是 v3→v4 **迁移**，只对老档跑一次；判据 4 用的是现行版本的档，放进迁移就咬不住，所以放在 `fromJson` 读完容量的地方、每次读档都抬。没升存档版本：只补不削，往返恒等只在「容量低于境界下限」这种本来就不一致的档上被打破（`IoTests` 的往返用例是炼气、容量 6，不受影响）。
- **测试**：`Ch07BottleFloor.EachTierHasItsFloor`、`Ch07BottleFloor.ALoadedFoundationSaveGetsSixAndABiggerBottleKeepsItsOwn`（3 → 6、8 → 8，滴数不动）、`Ch07BottlePanel.ABreakthroughWithinQiRefiningKeepsThreeAndIntoFoundationMakesSix`（打坐面板真突破：十二 → 十三仍 3；十三 → 筑基初期 6）；脚本升境那条是 `ScriptApiTest.AScriptedStepIntoFoundationWidensTheBottleWithoutFillingIt`（放在 `tests/ScriptApiTests.cpp`：跑真 api.lua 的装置在那里）。

---

## 4. E4 脚本按年份下限计数与扣物

### 4.1 一句话口径

**`item.count_aged(id, min_age)` 数年份 ≥ min_age 的件数；`take_aged(id, count, min_age)` 从年份 ≥ min_age 的堆里扣 count 件，先扣够格里年份最低的；不够就一件不扣、返回 false。**

### 4.2 脚本 API（`scripts/common/api.lua`，只追加）

```lua
-- 年份不低于 min_age 的有几件（第 7 章契约 4.1）。
item.count_aged = function(id, min_age)
    return __host.item_count_aged(id or "", min_age or 0)
end

-- 从年份不低于 min_age 的堆里扣 count 件，够格里年份低的先扣；不够就一件不扣。
function take_aged(item_id, count, min_age)
    local result = emit{ kind = "take_item_aged", a = item_id or "", x = count or 1, y = min_age or 0 }
    return result.ok == true
end
```

- `min_age <= 0` 等于「哪一堆都行」（与 `take` 省略年份同义，次序仍是从低到高）；`count <= 0` 按 1（与 `take` 一致）。
- `take` 与 `take(id, n, age)` 一字不动：三种扣法各有用处，注释写清分工（「哪一堆都行」/「恰好这一堆」/「够格的里挑」）。

### 4.3 宿主与命令

- `__host.item_count_aged(id, min_age)`（`src/script/ScriptHost.cpp`，只读，不占命令位）→ `GameState` 新方法 **`itemCountAtLeastAge(id, minAge)`**。
- 新命令 **`CommandKind::TakeItemAged`**：**追加在枚举末尾**（`PlayBgm` 之后；理由见 `src/script/Command.h` 那段注释——存档与测试按序号比对）；`ScriptHost` 把 `"take_item_aged"` 翻成它；`Application` 执行 `GameState` 新方法 **`removeItemAtLeastAge(id, count, minAge)`**：全有或全无，从够格的堆里按年份升序扣。
- 两个新方法与 `removeItem` / `removeItemOfAge` 同处（`src/core/model/Types.h/.cpp`）。

### 4.4 判据（`tests/Ch07EngineTests.cpp` 或 `tests/ScriptApiTests.cpp` 追加）

1. 背包「黄精 44 年 ×2 ＋ 11 年 ×3 ＋ 0 年 ×1」：`count_aged(…, 44) == 2`；`count_aged(…, 0) == 6`；`count_aged(…, 45) == 0`。
2. `take_aged(…, 3, 44)` → false，背包一件不动；`take_aged(…, 2, 20)` → true，扣的是两株 44 年（11 年、0 年原样）；`take_aged(…, 2, 5)` → 扣两株 11 年（够格里最低的）。
3. 走真脚本（`tests/scripts/` 下新增一段测试脚本，或 `ScriptApiTests` 既有的装置）调用两个函数，结果与第 1、2 条一致。
4. 命令序号：`TakeItemAged` 的序号等于 `PlayBgm` 的序号 ＋ 1（钉住「追加在末尾」）。
5. **负向自检**：扣的次序改成从高到低 → 第 2 条第三小条红；「全有或全无」改成能扣几件扣几件 → 第 2 条第一小条红。

### 4.5 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch07eng`。

- **`GameState::itemCountAtLeastAge` / `removeItemAtLeastAge`**（`Types.h/.cpp`，紧跟 `removeItemOfAge`）：`minAge <= 0` 即全部年份；扣之前先问够不够（全有或全无）；只把够格那几堆的下标按年份 `stable_sort` 后从低到高扣，**不重排背包**（`removeItem` 会整包 `sort`，这条没照抄）；`count <= 0` 视作无事发生返回 true（与 `removeItemOfAge` 一致），「按 1」在命令那一层做。
- **命令与宿主**：`CommandKind::TakeItemAged` 追加在 `PlayBgm` 之后（`Command.h` 的槽位表同步加一行：a 物品 id、x 数量、y 年份下限）；`ScriptHost` 的 kindTable 加 `"take_item_aged"`，`__host.item_count_aged(id, min_age)` 只读、不占命令位；`Application::dispatch` 执行 `removeItemAtLeastAge`（`x <= 0` 按 1）。
- **api.lua**：末尾照 4.2 原文追加 `item.count_aged` 与 `take_aged`，前面加一段注释写清三种扣法的分工（「哪一堆都行」/「恰好这一堆」/「够格的里挑」）；`take` 一字未动。
- **测试**：`Ch07AgedItems` ×3（计数三条、扣物三小条、命令序号）；走真脚本的 `ScriptApiTest.TheScriptCountsOnlyThePilesOldEnough`、`ScriptApiTest.TheScriptTakesAllOrNothingFromTheYoungestPileOldEnough`，脚本是新增的 `tests/scripts/ch07_take_aged.lua`（先数、再扣、再数，三样记进旗标）。

---

## 5. 门禁与既有测试的依赖

### 5.1 引擎路：形状检查（纯增补，不依赖本章新数据）

> **2026-09-29 协调者改派**：这一批原写「协调者派工前落」，改归引擎路，与第 6 章的分法一致（引擎路改规则 24 形状、协调者集成时改规则 25 的表）。引擎路只动下面 1–6 条涉及的函数与表；`CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段不动（5.2，协调者）。

这一批只加检查、不加本章的表，落完 `validate.py` 与 `validate_selftest.py` 须照旧全绿：

1. 规则 24 法术那一半：`effect` 认 `"reveal"`、`"stagger"`；`stagger` 为 1–9 的整数、只许与 `effect: "stagger"` 同写；带效果的与 `power > 0` / `poison > 0` 同写报错。
2. 规则 24 物品那一半：`castMagic` 进 `REFERENCE_FIELDS`（`'castMagic': 'magic'` 与归一化后的 `'cast_magic': 'magic'`），引用不存在的法术即报；与 `restoreHp` / `restoreMp` / `poison` / `curesPoison` 同写报错。
3. 脚本物品引用（`SCRIPT_ITEM_CALL`）认 `take_aged(` 与 `item.count_aged(`：拼错的药名同样被抓。那条正则上方的注释说过转义被 heredoc 吃掉的事故——用编辑工具改。
4. 规则 25 的来路核对（`check_means_sources`）：`give` 的类别是兵刃类（剑、刀、暗器）时，核物品的 `weapon` 字段等于那一类（与「毒」核 `poison` 同一个位置）。
5. `story.recipe_later` 只许出现在 `data/recipes/**` 的 `requireFlag` 里；任何脚本 `flag.set("story.recipe_later")` 报错。
6. `validate_selftest.py` 每一条配负例（写坏 → 必报 → 还原）；第 3 条另配一条「拼对了不报」的正例。按字段名登记的清单（第 6 章记录里的 `BREAK_RULE_WORDS`）加 `stagger`、`castMagic`。

旗标：`story.recipe_later` 协调者已先登记（引擎路的 5 份门闸方子引用它）；本章主线旗标（`docs/ch07-design.md` 第 11 节）在内容路派工前由协调者登记。

### 5.2 协调者：集成时落（依赖内容路的脚本）

- `CHAPTER_MEANS[7]`、`BATTLE_EXTRA_MEANS`（②③④⑤ 与 `be07_tuishan_shou`）、`MEANS_LAST_CHAPTER = 7`——表见 `docs/ch07-design.md` 8.2；来路核对会去 `scripts/ch07/` 找 `magic.learn` 与 `give`，所以只能在内容路落地之后加。
- `validate_selftest.py` 约 1518 行那条「第 7 章的敌人不在规则 25 的范围 → 不报」：改指一场第 8 章的编成（例如 `b08_jinguyuan_juezhan` 与它的一个敌人），措辞改成「第 8 章」。
- 删 `data/text/battles.json` 里三条旧的 `ch07.battle.*.intro`（内容路的新 key 在 `data/text/ch07_battle.json`）。
- `tests/LexiconTests.cpp` 表一补 `docs/ch07-design.md` 12.1 的 27 个词（放在第 6 章集成之后，16.2 第 4 条）。

### 5.3 E7 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/RealmTests.cpp` 曲线锚点 | `{Realm::QiRefining11, 155, "lu_shixiong"}` | `{Realm::QiRefining12, 180, "lu_shixiong"}`（`realmMaxHp(12) = 168`，差 7%，在一成半以内）；判据不改 | 引擎路 |
| `src/core/rules/Realm.h` 注释表 | 「炼气十一层　lu_shixiong 155　156」「lu_shixiong(十一层 155≈156)」 | 挪到十二层那一行（与 `qingxumen_gaotu 178` 并列）；十一层那一行写「—」 | 引擎路 |
| `data/roles/lu_shixiong.json` | 十一层、火、火球术 | 十二层、木、`maxHp 180`；法术表**留一门伤人法术且法力够放**（`tests/Ch03BattleKitTests.cpp` 的 `AnEnemyReallyCastsInARealBattle` 拿它验「敌人真的施法」） | 内容路 |
| `tests/Ch03BattleKitTests.cpp` 约 504 行注释 | 「陆师兄会火球术」 | 改成「陆师兄会青弧斩」；用例本身不改 | 测试路 |
| `tests/SliceTests.cpp` 那张「迁移战斗逐场跑完」的表 | 含 `b07_zhaoze_shouyao` | 内容路改写它时须保证**自动跑得出结果**（墨蛟 `killable_by 土`：我方打不死它，就得被它打死，不许僵住）；做不到时由测试路把那一格换成 `b07_yixiantian` | 内容路（数据）／测试路（表）|
| `tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp` | 数第 6 章图上的挂点与门（「不在 3.1 表里」「24 处 trigger」「不许有第四道」）| 第 7 章经 patch 加到 `ch06_huangfenggu`、`ch06_baiyaoyuan` 的对象排除在计数之外（认 `ch07.` 开头的旗标属性）；判据不改。**内容路落地到测试路改完之间这几条是红的**——已知，不是内容路的错（第 6 章南城东门的先例） | 测试路 |

---

## 6. 白名单（四路按文件分，**一个文件只归一路**）

### 6.1 引擎路

- `src/**`
- `scripts/common/api.lua`（只追加 4.2 那两个封装）
- `data/recipes/alchemy/jiedan_lingyao.json`、`data/recipes/talisman/{huoqiu_fu,hushen_fu,jinci_fu,leiming_fu}.json`（只加 `requireFlag` 一个字段）
- `data/magics/{tulao_shu,dingshen_shu,tianleizi}.json`（新）
- `data/items/talismans/{tianleizi,tulao_fu}.json`（新）、`data/items/talismans/{huoqiu_fu,dingshen_fu}.json`（只加 `castMagic` 一个字段）
- `data/text/ch07_e2.json`（新；只放 2.2 那五条描述）
- `tests/Ch07EngineTests.cpp`（新）；`tests/Ch04AlchemyTests.cpp`、`tests/Ch04SliceTests.cpp`（只按 1.6 第 5 条改）；`tests/RealmTests.cpp`（只改锚点那一行）；`tests/BattleTests.cpp`、`tests/ScriptApiTests.cpp`、`tests/GameWiringTests.cpp`（只追加）；`tests/scripts/**`（只新增 E4 的测试脚本）
- `tools/validate.py`（**只** 5.1 第 1–5 条涉及的检查；`CHAPTER_MEANS` 那一段不动）、`tools/validate_selftest.py`（只补 5.1 第 6 条的负例与正例）
- `docs/interfaces-p3-ch07.md`（只填 1.7、2.7、3.5、4.5 与末尾「实现记录」）、`docs/octopath-battle.md`（5.2 法术表加 `effect: stagger` 与 `stagger` 两行、5.3 物品表加 `castMagic` 一行、2.7 物品细则加一句）

### 6.2 内容路（另一份派工单，写在 `docs/ch07-design.md` 第 18 节头部；这里只列边界）

- `maps/ch07_*.tmj`（新 9 张）、`maps/tilesets/terrain_{yuelu_dian,dihuo,fangshi,shandong,jindi_wai,jindi_waiwei,jindi_zhongxin,huanxingshan,dixia_zhaoze}.tsj`（新）
- `maps/ch06_huangfenggu.tmj`、`maps/ch06_baiyaoyuan.tmj`（**只经** `genmaps_ch07.patch_*()` 改；不动第 6 章已有的对象）
- `tools/mapgen/genmaps_ch07.py`（新）、`tools/mapgen/genmaps.py`（只加登记与两处 patch 调用）
- `scripts/ch07/**`
- `data/text/ch07*.json`（新；**不含** `ch07_e2.json`）、`data/text/shops.json`（只加一条 nameKey）
- `data/roles/*.json`（新建 43 个；改写 `lu_shixiong.json`，守 5.3 那两条）
- `data/items/**`（新建 29 件；`pills/zhuji_dan.json` 只改 `tradeable`）——**不含** 6.1 的四份
- `data/magics/*.json`（新：祭器法术 4 门、敌方法术 6 门；`ji_jianfu.json` 只改 note）——**不含** 6.1 的三份
- `data/recipes/alchemy/{zhuji_dan,dingyan_dan}.json`（改写）
- `data/battles/b07_*.json`（新 2、改写 3）、`data/battles/be07_*.json`（新 3）、`data/encounters/ch07_huanxingshan.json`、`data/shops/ch07_fangshi_yaotan.json`、`data/quests/q07_*.json`、`data/pathactions/ch07.json`、`data/objectives/ch07.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`tools/artgen/sprites_enemies.py`（只加 6 个非人形敌人）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加 9 条登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch07-design.md`（**只追加第 18 节「施工偏差」**）

### 6.3 协调者

- `tools/validate.py`、`tools/validate_selftest.py`（**只** 5.2 集成时那几处；5.1 归引擎路）
- `data/flags.json`（主线旗标与 `story.recipe_later` 派工之前登记；`ch07.path.*` 等内容路的 `data/pathactions/ch07.json` 落地后登记）
- `data/chapters.json`、`data/text/ui.json`（「第七章」「血色试炼」）、`data/text/battles.json`（只删三条旧 intro）
- `tests/LexiconTests.cpp`（表一补词）
- `docs/handoff.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（回写，`docs/ch07-design.md` 16.1 第 1 条）

### 6.4 测试路（两路都落地、协调者 5.2 之后另派）

- `tests/Ch07*.cpp`（除 `Ch07EngineTests.cpp`）：通关、挂点、编成、账、切片、验收
- `tests/ObjectiveTests.cpp`（`kScriptTransfers` 十条，见 `docs/ch07-design.md` 3.1 末）
- `tests/NpcPresenceTests.cpp`（第 7 章进时间线；本章不加群像）
- `tests/PathActionTests.cpp`（境界上限表补第 7 章：本章脚本抬到筑基初期；钱袋表补第 7 章：第 6 章终局第二侧 82 块，本章求购 12 块付得起）
- `tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp`、`tests/Ch03BattleKitTests.cpp`、`tests/SliceTests.cpp`（只按 5.3 那几行改）
- `tests/ChapterFixture.h`、`tests/fixtures/ch07-end-*.sav`、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者 5.1 ＋ 主线旗标登记 → 引擎路 ∥ 内容路 → 协调者 5.2 与集成 → 测试路 → 独立校对。本章在第 6 章集成之后开工。
- 每路用自己的构建槽：引擎路 `ch07eng`，内容路 `ch07content`，测试路 `ch07test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 几路共用一棵工作树：验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实，不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- 门禁先于编译跑：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`。

---

## 实现记录

引擎路 · 2026-09-29 · 构建槽 `ch07eng` · WT 基线 `a2a11f3`。E1–E4 各自的细节在 1.7、2.7、3.5、4.5。

**改了的文件**

- `src/`：`core/model/Types.h/.cpp`、`core/battle/Battle.h`、`BattleAction.cpp`、`BattleAi.cpp`（只改注释）、`core/rules/Crafting.h/.cpp`、`Bottle.h`、`Realm.h`（E7 注释表：十一层写「—」，陆师兄挪到十二层与 qingxumen_gaotu 并列，直线那句改成「十二层 180，差 7%」）、`io/DataLoader.cpp`、`RecipeLoader.cpp`、`SaveFile.cpp`、`game/AlchemyScene.h/.cpp`、`Application.cpp`、`BattleScene.cpp`、`BattleView.h/.cpp`、`CultivationScene.cpp`、`script/Command.h`、`ScriptHost.cpp`。
- `scripts/common/api.lua`（末尾追加 4.2 两段）；`data/` 见 1.7 / 2.7；`docs/octopath-battle.md`（2.7 一条、5.2 两行、5.3 一行）。
- `tests/Ch07EngineTests.cpp`（新，25 条）、`tests/ScriptApiTests.cpp`（追加 3 条）、`tests/scripts/ch07_take_aged.lua`（新）、`tests/Ch04AlchemyTests.cpp` / `tests/Ch04SliceTests.cpp`（1.6 第 5 条）、`tests/RealmTests.cpp`（锚点那一行）。
- `tools/validate.py`、`tools/validate_selftest.py`（5.1，下面）。

**门禁（5.1）**

1. 规则 24 法术：`effect` 认 `"reveal"` / `"stagger"`；`stagger` 只许与 effect stagger 同写、1–9 整数。
2. 规则 24 物品：`REFERENCE_FIELDS` 加 `castMagic` / `cast_magic` → magic；`castMagic` 与四样药效（数值 > 0、`curesPoison` true）同写报。
3. `SCRIPT_ITEM_CALL` 认 `take_aged(` 与 `item.count_aged(`（编辑工具改，按字节核过五个反斜杠都在）。
4. `check_means_sources`：give 的类别是剑 / 刀 / 暗器时核那件东西的 `weapon`（`GIVEN_WEAPON_CATEGORIES`）。`CHAPTER_MEANS` 那一段没动——它上方的注释「give = give() 给的毒药」该补「或兵器」，留给协调者集成时顺手改。
5. 新 `check_recipe_only_flag`（全量模式接进 `main`）：`data/**`（除 `flags.json`、`text/`）与 `maps/*.tmj` 里任何一个值等于 `story.recipe_later` 的，除 `data/recipes/**` 的 `requireFlag` 外一律报；`scripts/**`（除 `common/`）里 `flag.set` 它报。
6. selftest 193 → **213** 条：G4–G6（两种新写法拼错必报 ×2、拼对全量干净、形近写法 `my_item.count_aged` / `itemXcount_aged` 不误报）、N 节 stagger ×5 ＋ castMagic ×3 ＋ 兵刃来路一对（临时在表上登第 6 章 give 的冷月刀：weapon 是刀不报、改成剑必报，问完拿掉）、新 T 节 5 条（五张方子不报 ＋ 脚本置 / 物品引 / 配方写在别的字段 / 地图属性引 各必报）。`BREAK_RULE_WORDS` 加 `stagger`、`castMagic`。

**负向自检**（C++ 改坏在隔离副本 `scratchpad/mut07` 里做——WT 与内容路共用，不在那里改坏源码；改坏 → 编译 → 跑对应用例 → 确实红 → 还原）

- A 组一次编译：`visibleRecipes` 去掉 `recipeKnown` → `Ch07RecipePanel.TheChapterSixSaveSeesNoneOfTheFiveAndTheFlagBringsThemAllBack`、`…EveryRowOpensTheRecipeItNamesWithTheGateShutAndOpen` 红；道具施法照扣法力 → `Ch07ItemCast.TianleiziThenTulaoBreaksTheFoeAndTheManaNeverMoves`（法力 38、32）、`Ch07ItemCastScene.BothTalismansGoThroughTheRealMenuAndOnlyTheBagPays`、`Ch07ItemCast.ARevealTalismanLaysTheFieldBareWithoutMana` 红；`stagger` 不查已破势 → `Ch07ItemCastScene.TulaoOnABrokenFoeIsRefusedAndTheBagKeepsIt`（第二张照样用了出去、背包 0）与 `Ch07ItemCast.AStaggerOnABrokenFoeOrOneWithoutStanceIsRefusedFirst` 红；我方 AI 去掉 `offensiveMagic` 过滤 → `Ch07Stagger.TheAllyAiNeverReachesForAStaggerSpell` 红（出的是 Cast）；`RealmAdvance` 不抬容量 → `ScriptApiTest.AScriptedStepIntoFoundationWidensTheBottleWithoutFillingIt` 红；扣的次序改成从高到低 → `Ch07AgedItems.TakingAgedIsAllOrNothingAndTakesTheYoungestPileOldEnough` 第三小条、`ScriptApiTest.TheScriptTakesAllOrNothingFromTheYoungestPileOldEnough` 红。
- B 组：`canCraft` 去掉已得那一步 → `Ch07RecipeGateRule.AnUnknownRecipeSaysSoBeforeTheFurnace` 红；`battleUsable` 去掉 `castMagic` → 真入口那几条（`BothTalismans…` 的 hasItem、`TulaoOnABroken…`、`ADingshenTalismanTakesOnePointOfStanceAndNoBlood`、`TheHeadlessAllyNeverSpendsATalisman`）与 `TheFourTalismansCastWhatTheContractSays` 红；`applyRealmAttributes` 不抬 → `Ch07BottlePanel.ABreakthroughWithinQiRefiningKeepsThreeAndIntoFoundationMakesSix` 跨档那半红；读档不抬 → `Ch07BottleFloor.ALoadedFoundationSaveGetsSixAndABiggerBottleKeepsItsOwn` 红；「全有或全无」改成能扣几件扣几件 → `TakingAged…` 第一小条、`TheScriptTakes…` 红。
- C：菜单改回「effect ≠ None 的法术一律直接发」→ `Ch07ItemCastScene.AStaggerSpellFromTheSpellMenuAsksForATarget` 红。
- **没有能咬住的用例**：「我方 AI 去掉过滤」对契约 2.6 第 6 条（`TheHeadlessAllyNeverSpendsATalisman`）咬不住——物品本来就不进我方 AI 的候选，天雷子 / 土牢术也不在韩立的已习得表里，去掉过滤行为不变。咬住那条过滤的是另写的 `TheAllyAiNeverReachesForAStaggerSpell`（拳已试过、只剩「火」没试，而会的那门削架势恰好是火）。
- 门禁：副本里把新加的检查一起关掉（stagger 取值与同写、castMagic 引用、castMagic 互斥、正则不认两种新写法、兵刃来路、占位旗标那条不接进 `main`）→ selftest 那 14 条负例全 `[FAIL]`、`SELFTEST_FAIL`；另把 `item\.count_aged` 的点改成不转义 → G6 两条 `[FAIL]`。还原后 213 条全过。

**全量**：WT 槽 `ch07eng`，**1557 条（基线 1529 ＋ 本路 28）、1556 过、1 条红**——`Ch06Slice.TheTalismanDeskOnlyOpensAfterNineA`，已知、非本路（引擎不看设施的 `require_flag`，归第 6 章整改路；开工时 WT 基线就是这 1 条红）。零编译警告。另：16:33 那次 `build_logged.bat ch07eng`（四道门禁全过）是 1556 / 1555 过 / 同一条红（那时还没加菜单择敌那一条）；之后内容路的地图还在半路（`genmaps.py --check` 报 11 张漂移，`build.bat` 停在第二道门），最后两次全量是在同一个槽上照 `build.bat` 去掉门禁那几步跑的（vcvars → cmake → ctest），结果如上。`validate.py` 交付时 VALIDATE_OK。

**没做的 / 已知**

- O1–O3 不做（契约）。道具施法不查 `needRealm`（2.7）。
- `data/items/talismans/dingshen_fu.json` 的 note 末句「本作不给它战斗用途（E2 不做）」已过时——只许加 `castMagic`，没改，交协调者。
- `tools/validate.py` 规则 25 表上方「give = give() 给的毒药」一句待补「或兵器」（`CHAPTER_MEANS` 段归协调者）。
- 契约 3.3 第 3 处的位置偏差见 3.5（放在 `fromJson` 而不是 v3→v4 迁移里）。

**协调者裁决（2026-09-29，按推荐）**：道具施法不查 `needRealm`——本章三门都是炼气一层，查与不查结果一样；符箓在设定上本就是「不看施术者修为」的东西。将来若有高阶符箓要门槛，再在物品上加字段，不在施法路径上加判断。
