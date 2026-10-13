# P3 第 11 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-10-12 · 依据第 11 章研究笔记（`notes/ch11/research.md`）D 表（引擎现状）、E 表（拍板）、F 表（节点）、I 表（战斗）、L 表（物品/配方/商店）、N 表（teleport）、O 表（第 12 章接口与开放问题）；格式沿 `docs/interfaces-p3-ch10.md`。
> 本批引擎路只做 **E1**（必成配方）——它是本章唯一要动 `src/` 的事：笔记 E16「配方 `difficulty 0 / requiredProficiency 0`，面板必成」在现有规则层**不成立**（`successChance` 夹在 [5, 95]，炼器失败折损一半材料，六根金雷竹折掉三根就是死局），要补一个字段。**E2–E5 是验证项**：查过、不新做代码，只在 `tests/Ch11EngineTests.cpp` 把内容路要倚仗的引擎行为钉住，红了再议。
> 门禁：引擎路不改 `tools/`。规则 25 的 `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER`、selftest 探针搬家归协调者（第 7.2 节）。
> **不做**（笔记 E 表与 O 表已拍板）：拍卖面板（E9：数据目录 + `io/AuctionLoader` + `game/AuctionScene` + Lua `auction(id)` 一整套，第 12/13 章再议）；`breakthroughPillBonus` 接降尘丹/雪灵水（E4：本章不走面板冲关，接了没人读）；抬 `kMaxScriptAdvanceDays`（E5：脚本分段）；Lua 加熟练度命令（E16）；名牌「？」（O-9：文案说「看不透」）；`realm.former()`（沿第 10 章 O1）；路径行动 `give` 看返回值（O-3：K12 改挂 interact 脚本）；分身双控、遭遇区（E15 不设）。
> **本章在第 10 章集成之后开工**：第 7 章 E1（配方 `requireFlag`）、第 8 章 E1（打坐设施按地点给效率）、第 9 章 E1/E2（`formerRealm`、存档 v9）、第 10 章 E1–E3 都当成已有的用。本章不加存档字段，仍是 v9。

---

## 0. 各引擎项现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 洞府密室炼器台上选 `recipe_qingzhu_fengyun_jian` 一次必成（笔记 E16、B14） | `rules::successChance`（`src/core/rules/Crafting.cpp` 第 231–240 行）= 底数 40 + 熟练/2 + 资质/5 + 炉鼎品阶×5 − 难度，夹在 `[kMinSuccessChance 5, kMaxSuccessChance 95]`（`Crafting.h` 第 84–86 行「永远留 5% 失手」）；起点炼器熟练 0、资质 50、炉鼎 3 品 → 65。`craft`（第 306–331 行）掷 d100。`failurePolicyOf(Forge)` = `{50, false}`（第 223 行）：失败折损每味一半、向上取整。`Recipe`（`Crafting.h` 第 45–60 行）末字段是 `requireFlag`，没有「必成」一说。`io::RecipeLoader` 校验 productCount / difficulty / requiredProficiency / inputs。现有配方文件 `data/recipes/forge/qingzhu_fengyun_jian.json`：`productCount 12`、`difficulty 90`、`requiredProficiency 80`、金雷竹 ×4 + 灵石 ×20、无 `requireFlag` |
| E2 | 六十年闭关、二十余年开店（笔记 E5、F-n16、F-n29） | `kMaxScriptAdvanceDays = 3650`（`src/game/Application.h` 第 49 行）；`AdvanceDays` 命令 `advanceDays(std::min(command.x, kMaxScriptAdvanceDays))`（`Application.cpp` 第 1114 行）——**超出静默夹到 3650，不报错**；`Application::advanceDays`（第 618–635 行）逐项结算灵田 `growField`、小瓶 `refill(bottle, day, bottleChargeDays(realm))`、`restFor(crossed)`；`bottleChargeDays`（第 130–141 行）结丹 3、筑基 5、其余 `kBaseChargeDays 7`；`today()` 经 `flag.set(name, n)` 存整数 |
| E3 | 战斗 ④ 妖尸「困而不杀」（笔记 I 表、O-1） | 角色 `killable_by` 是类别位掩码（`core/model/Types.h` 第 218–219 行；`Battle.h` 第 139 行 `Unit::killableBy`，0 = 什么都要得了命）；`BattleState::strike`（`BattleAction.cpp` 第 561–562 行）`lethal = killableBy == 0 || (category & killableBy) != 0`，不 lethal 的那一击最多压到 1（第 649 行注释）；`DataLoader.cpp` 第 531–534 行与 `validate.py` 第 2435–2439 行都拒绝空数组。胜负只看气血，没有「架势被破两次即胜」 |
| E4 | 凝翠岛收购行（`entries` 空、`sellRate 0.8`）与青竹小轩（`entries` 空、`sellRate 0.6`）（笔记 E8、L.4） | `io/ShopLoader.cpp` 第 60 行 `entries` 可缺省；第 49–56 行只拒 `buyRate <= 0`、`sellRate < 0`、`sellRate >= buyRate`；`ShopScene.cpp` 第 241–243 行 `entries.empty()` 时「买入」禁用、理由「货架上空空如也」；第 100 行卖出页只列 `tradeable` 的；`rules::sellPrice`（`Economy.h` 第 54 行）= `item.price × sellRate`（灵草另走 `herbPrice`） |
| E5 | 结丹初期的几处口径：剧情 `realm.advance(31)`、小瓶 3 日一滴、面板不许过结丹中期、三十九层静室 125、炼器台 forge 3 品挂 `require_flag`（笔记 C.1–C.3、E11、E16） | `RealmAdvance`（`Application.cpp` 第 1271–1330 行）只升不降、`raiseRealmCap(target)` 顺带抬上限、当前值 `liftToFloor` 差额上抬；`breakthroughBlock(realm, cultivation, storyCap)`（`Cultivation.h` 第 114 行）按编号比；设施 `effectiveness` 属性经 `openFacility`（`Application.cpp` 第 697–700 行）交给 `CultivationScene(sitePercent)`，门禁只放 100–300（`validate.py` 第 234–242 行）；`forge` 设施经 `io::craftKindFromString` 开 `AlchemyScene(Forge, grade)`（`AlchemyScene.cpp` 第 91–92 行把品阶夹到 `[0, kMaxToolGrade 5]`） |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。本批：规则层补一个字段与一个常量；加载器补一个可选键与一条不变量；游戏层**不改**（`AlchemyScene` 已从 `successChance` 读数，字段生效后面板那一行自动写 100）。

**查过、本章不需要新做的**（笔记 D 表「缺什么：无」那几行）：`realm.advance` 跨大境界（筑基后期 → 结丹初期一步到位）；`realm.cap` 只升不降；`field.unlock(id, slots)`（≤ `kMaxFieldSlots 64`）；设施 `require_flag`；配方 `requireFlag`；瓶子 `bottle.spend / mature / drops` 与「只补不削」；`party.add / remove / has` 看返回值；任务板 `board` 设施与 `data/quests`；路径行动 `when / until / reveal`；NPC `visible_flag / hidden_flag`；触发器 `once=false` 的可重复 interact；`defeat_is_fatal false` 时 `battle()` 返回 `won=false`、`can_escape`；多波次、蓄势、破绽；`teleport`；章节卡按完成旗标自动排、最后一章之后接「未完待续」；`playSecondsGameplay` 只按真实秒累计（`advance_days` 不碰它，「跳过的岁月不计游玩时长」天然成立）。

**不是引擎的事、写在这里免得找错人**：结丹初期攻防锚点（`Realm.h` 第 175 行「筑基后期与结丹档仍待对应章节标定」）由平衡路用 `tests/BattleHand.h` 量 I 表四场后改注释；`siji_haishou` 的 `weaknesses` 就地改写归内容路；`wu_chou.json` 一个字不动。

---

## 1. E1 必成配方

### 1.1 一句话口径

**`Recipe::guaranteed` 为真的方子，`rules::successChance` 恒返回 `kGuaranteedSuccessChance = 100`，不落进 [5, 95]；`craft` 掷出的 d100 ∈ [1, 100] 必 ≤ 100，所以 `craft` 一字不改、失败分支不可达。必成的方子必须有 `requireFlag` 门闸（加载器拒绝没有的），全仓眼下只许这一张。熟练度门槛、材料门闸、炉鼎有无（`canCraft`）照旧生效——必成是「开得了工就必成」，不是「什么都不要」。**

- **为什么不用 `difficulty 0`**：难度 0、熟练 0、资质 50、炉鼎 3 品是 65；熟练 100、资质 100、炉鼎 5 品也只到 95——「永远留 5% 失手」是规则层写死的不变量，数据够不到 100。
- **为什么不改失败策略**：`Forge` 失败折损一半是整条法宝线的口径（`Crafting.cpp` 第 221–223 行注释），为这一张方子改它会牵动第 12 章以后所有炼器。
- **为什么不剧情 `give`**：笔记 E16——炼器面板全作到此没用过一次；原著 ch413–415 他是一次炼成、无材料再试，「必成」就是原著。
- **为什么要 `requireFlag` 作前提**：没有门闸的必成方子会在面板上从第 7 章起就列出来（`recipeKnown` 只看 `requireFlag`），等于一张白送的产物；门闸把「必成」限定在剧情给它的那一刻。

### 1.2 规则层（`src/core/rules/Crafting.h` / `Crafting.cpp`）

```cpp
// Recipe 末尾追加（第 11 章契约 docs/interfaces-p3-ch11.md 1.2）。按字段顺序写的聚合初始化不受影响。
// 必成：successChance 恒 100，不落进 [kMinSuccessChance, kMaxSuccessChance]。原著 ch413–415 七十二口青竹蜂云剑
// 一次炼成、无材料再试；炼器失败折损一半（failurePolicyOf(Forge)），六根金雷竹折掉三根就是死局。
// 加载器要求它配 requireFlag（io/RecipeLoader）：没有门闸的必成方子会从第 7 章起就列在面板上。
bool guaranteed = false;

// 必成方子的成功率。写成常量不写字面量：测试钉的是「恰等于它」，不是「大于 95」。
inline constexpr int kGuaranteedSuccessChance = 100;
static_assert(kGuaranteedSuccessChance > kMaxSuccessChance);
```

- `successChance`：**第一句**加 `if (recipe.guaranteed) return kGuaranteedSuccessChance;`，放在四个入参夹量程之前；其余一行不动。
- `craft`、`canCraft`、`proficiencyGain`（难度 0 成功 +3）、`failurePolicyOf`、`planConsumption`、`applyConsumption`、`kMinSuccessChance` / `kMaxSuccessChance` / `kBaseSuccessChance` **不改**。
- `Crafting.h` 第 84 行「永远留 5% 失手与 5% 侥幸」那段注释补一句：「例外只有 `guaranteed`（第 11 章契约 1.1）」。

### 1.3 加载器（`src/io/RecipeLoader.h` / `RecipeLoader.cpp`）

- 读可选键 `"guaranteed"`：缺省 `false`；存在但不是 bool → `Out::failure("配方 <id> 的 guaranteed 必须是 true/false：<path>")`（与第 40 行「"difficulty": "很难" 该当场喊出来」同一口径）。
- 不变量：`recipe.guaranteed && recipe.requireFlag.empty()` → `Out::failure("配方 <id> 写了 guaranteed 却没有 requireFlag 门闸（必成的方子必须由剧情给）：<path>")`。
- `RecipeLoader.h` 第 32–33 行那段「结构性校验」清单补 `guaranteed`。
- **不改**：`tools/validate.py`（它对配方只查 `story.recipe_later`，第 3220 行，而且它是 Python、不调 C++ 加载器——**门禁不查 `guaranteed`**）。两条校验只在 C++ 跑到 `io::loadRecipes` 时生效：游戏启动（`Application.cpp` 第 232 行 `io::loadRecipes(assetRoot_ + "/data/recipes", data_)`，配方不在 `GameData` 里、`loadGameData` 不读它）与 1.5 第 5 条那条用例。

### 1.4 游戏层（`src/game/AlchemyScene.cpp`）

- **不改**。`buildRecipeItems`（第 111–135 行）的 detail 已写 `rules::successChance(...)`，必成方子自动显示 100；`craftOnce`（第 188–211 行）走 `rules::craft`，结果必成。
- 数据形状（内容路写，笔记 B14 / L.3；**这一段只是形状，数字以笔记为准**）：

```json
{ "id": "recipe_qingzhu_fengyun_jian", "name": "青竹蜂云剑图", "kind": "forge",
  "productId": "weapon_qingzhu_fengyun_jian", "productCount": 1,
  "difficulty": 0, "requiredProficiency": 0, "requireFlag": "ch11.liugen", "guaranteed": true,
  "inputs": [ {"itemId": "material_jinleizhu", "count": 6},
              {"itemId": "material_lianjian_fuliao", "count": 1} ] }
```

- `inputs` **不要灵石**（原文件的 `material_lingshi ×20` 去掉）：炼剑是主线必经门闸，收店（`ch11.liugen`）后店已盘出、凝翠岛到不了、中阶 `tradeable false` 换不成低阶，玩家把低阶买光就是死局（施工图 9 节第 5 条）。

- 地图侧：`ch11_dongfu` 的 `facility_lianqi_39`（kind `forge`、`grade 3`、`require_flag ch11.liugen`）——`grade 3` 只影响面板上「炉鼎品阶」那一行与非必成方子的成功率，对这张方子无关紧要，但 `grade 0` 会被 `hasToolOfGrade` 挡成「未曾引来地火」，**不许写 0**。

### 1.5 判据（`tests/Ch11EngineTests.cpp`，引擎路写；每条都直接读 `Recipe` / `CraftResult` / `GameState`，不经驱动）

1. **规则**：一张 `guaranteed true`、`difficulty 100`、`requireFlag "ch11.liugen"` 的方子，`successChance(recipe, 0, 0, 1)` 恰等于 `rules::kGuaranteedSuccessChance`；同一张改 `guaranteed false` → 恰 `kMinSuccessChance`（5）。
2. **掷骰不可达失败**：背包恰有金雷竹 6、辅料 1，`ch11.liugen` 已置，`craft(recipe, state, 0, 50, 3, seed)` 对 `seed` 0..999 逐一：`success` 全真、`productId == "weapon_qingzhu_fengyun_jian"`、`productCount == 1`、`consumed` 按 `inputs` 原顺序恰是 6 / 1、`proficiencyGain == 3`。
3. **对照组证明字段在起作用**：同一份状态、同一张方子只把 `guaranteed` 改假（此时 `successChance` ≤ 95，先验断言）：`seed` 0..999 里**至少一次** `success == false`，且那一次 `consumed` 的金雷竹恰 3（`ceil(6 × 50 / 100)`）、辅料恰 1——这就是 1.1 说的死局，钉住它免得日后有人把必成删了还全绿。
4. **加载器三条**（写临时目录，`io::loadRecipe` 或等价入口）：`guaranteed true` 无 `requireFlag` → `ok == false` 且 `error` 含「requireFlag」；`"guaranteed": "yes"` → `ok == false` 且 `error` 含「guaranteed」；不写 `guaranteed` → `ok == true` 且 `recipe.guaranteed == false`。
5. **真数据**：`auto data = io::loadGameData("data").value; auto recipes = io::loadRecipes("data/recipes", data);`（配方不在 `GameData` 里，`loadGameData` 不读它——`Application.cpp` 第 232 行就是这么另调的）后，`guaranteed` 为真的配方集合 ⊆ `{recipe_qingzhu_fengyun_jian}`，且其中每一张 `requireFlag` 非空、`kind == Forge`。内容路落地前集合为空也绿（本条钉的是「只许这一张」，不是「必须有这一张」）；集成之后由测试路在 `tests/Ch11*` 的编成/账用例里补「恰等于」与「`inputs` 恰金雷竹 6 ＋ 辅料 1、无灵石」。
6. **面板**（真 `AlchemyScene(rules::CraftKind::Forge, 3)`，headless `Application`，状态同第 2 条、炼器熟练 0）：列表里那一行的 detail 含 `"100"`；按一次炼制 → 背包 `weapon_qingzhu_fengyun_jian` 恰 1、`material_jinleizhu` 恰 0、`material_lianjian_fuliao` 恰 0、`material_lingshi` 不变、炼器熟练 3。
7. **数从常量来**：第 1 条用 `kGuaranteedSuccessChance` 比，不写 100 字面量；`static_assert(kGuaranteedSuccessChance > kMaxSuccessChance)` 在头文件里。
8. **负向自检**（改坏 → 构建 → 确实红 → 还原）：删 `successChance` 的早退 → 第 1、2、6 条红；早退挪到 `std::clamp` 之后（被夹成 95）→ 第 1 条红；加载器放行无 `requireFlag` 的必成方子 → 第 4 条红；加载器把 `guaranteed` 读成 `readBool(..., true)` 缺省真 → 第 3、4 条红。

### 1.6 实现记录（引擎路交付时填）

- **规则层**（2026-10-12，引擎路）：`src/core/rules/Crafting.h` +12/−1——`Recipe` 末尾追加 `bool guaranteed = false;`（带 1.2 那段注释）；`kMaxSuccessChance` 之后追加 `kGuaranteedSuccessChance = 100` 与 `static_assert(kGuaranteedSuccessChance > kMaxSuccessChance)`；第 84 行注释补「例外只有 `guaranteed`」，`successChance` 的声明注释补一句。`Crafting.cpp` +3/−0——`successChance` 第一句 `if (recipe.guaranteed) return kGuaranteedSuccessChance;`，在四个 `std::clamp` 之前；`craft` / `canCraft` / `proficiencyGain` / `failurePolicyOf` / `planConsumption` / `applyConsumption` 一字未动。
- **加载器**：`src/io/RecipeLoader.cpp` +13/−0——`requireFlag` 之后、`inputs` 之前：`guaranteed` 存在但非 bool → `failure("配方 <id> 的 guaranteed 必须是 true/false：<path>")`；`readBool(root, "guaranteed", false)`；`guaranteed && requireFlag.empty()` → `failure("配方 <id> 写了 guaranteed 却没有 requireFlag 门闸（必成的方子必须由剧情给）：<path>")`。`RecipeLoader.h` +3/−2——第 32–35 行的结构性校验清单补 `requireFlag / guaranteed`。
- **游戏层**：未改。`tools/`：未改。
- **判据**（`tests/Ch11EngineTests.cpp`，E1 部分 6 条用例）：1.5 第 1、7 条 → `Ch11GuaranteedRule.AGuaranteedRecipe…`（比的是 `kGuaranteedSuccessChance`，外加三组极端入参证明早退在夹量程之前）；第 2 条 → `…EveryOneOfAThousandSeeds…`（seed 0..999，`consumed` 逐项恰 6 / 1，`proficiencyGain == 3`）；第 3 条 → `…TheSameRecipeWithoutGuaranteedFails…`（先验 ≤ 95，至少一次失败且那次金雷竹恰 3、辅料恰 1）；第 4 条 → `Ch11GuaranteedData.TheLoaderRequires…`（契约三条 + 负向 `1` / `null` + `"guaranteed": false` 无门闸放行 + 正向形状）；第 5 条 → `…TheWholeRepositoryHasAtMost…`（真 `loadGameData` + `loadRecipes`，集合 ⊆ {`recipe_qingzhu_fengyun_jian`}，每张 `requireFlag` 非空、`kind == Forge`）；第 6 条 → `Ch11ForgePanel.TheListShowsOneHundred…`（真 `AlchemyScene(Forge, 3)`，detail 含 "100"，按一次：剑 1、金雷竹 0、辅料 0、灵石 20 不变、炼器熟练 3；配方未带 `guaranteed` 时 `GTEST_SKIP`——内容路已落地，实跑未跳过）。
- **1.5 第 8 条负向自检**（改坏 → 增量构建 → 跑 `--gtest_filter=Ch11*` → 还原，日志 `build-ch11eng-neg-{A,B,C,D}.log`）：A 删早退 → 第 1、2、6 条红（3 红 / 25 绿）；B 早退挪到 clamp 之后（被夹成 95）→ 第 1 条红，第 2、6 条也红（95% 下一千颗种子必有失败、面板写 95）；C 加载器放行无 `requireFlag` 的必成方子 → 第 4 条红（1 红 / 27 绿）；D `readBool(..., true)` → 第 4、5 条红，且全仓每张无门闸方子都被 C 那条不变量拒收、`Application::init` 失败，连带 E2/E4/E5 的面板用例红（14 红 / 14 绿）。四处均已还原，`git diff` 核过无残留标记。
- 偏差：无。配方 JSON 由内容路改写，交付时已是 1.4 形状（`guaranteed true`、`requireFlag ch11.liugen`、金雷竹 6 ＋ 辅料 1、无灵石）。

---

## 2. E2 六十年与二十年：`advance_days` 的分段与结算（验证项，不改代码）

### 2.1 一句话口径

**单次 `advance_days(n)` 最多推进 `kMaxScriptAdvanceDays = 3650` 日，超出的部分静默丢掉；六十年写六句 `advance_days(3600)`（n16）；二十余年不是「两句 3600」——n29 每根 `advance_days(1080)` 共 5 根（第一根在 n26 催成、不拨日），第六根再按 `today() − ch11.kaidian_ri` 补足到 7200（至少 360，补足值超 3650 时分两句），施工图 3.1 / 3.3 / 验收 11 同此（笔记 E5）。推进的每一段都逐项结算灵田、小瓶与静养；跳过的日子不计游玩时长。**

- 规则层 / 加载器 / 游戏层：**都不改**。本节存在是因为笔记 O-5 要引擎路「跑一遍」：六次 3600 合计 21600 日，`day` 从 16003 走到 37603，再加本章其余推进（≈ 180 + 720 + 180 + 3×N〔诱妖每轮 3，缺绿液另 ＋5〕+ 1800 + 720 + 15 + 15 + 30 + 720 + 7200 + 120 + 60）落在五万以内，`int` 存得下、`flag.set(name, today())` 存得下。
- 内容路要知道的一句：**写 `advance_days(21600)` 不会报错，只会悄悄少走六十年**——通关测试的日数断言会红，但脚本本身不会。

### 2.2 判据（`tests/Ch11EngineTests.cpp`）

1. **夹法**：从 `tests/fixtures/ch10-end-first.sav` 载入（`day == 16003` 先验），headless `Application` 跑一个**临时 `.lua` 文件**（`ScriptHost` 只有 `startEvent(scriptPath)`，没有执行字符串的入口；写进 `TempDir`、`startEvent` 后按帧泵到 `isRunning() == false`，照 `tests/Ch02BottleTests.cpp` 第 66–75 行）：内容 `advance_days(21600)` → `day == 16003 + 3650`；内容六句 `advance_days(3600)` → `day == 16003 + 21600`。前者钉的是「分段是内容路的事」，后者钉的是六段加起来没有丢日子。「不经驱动」指不走 `Driver`，脚本文件与泵帧仍是要的。
2. **灵田**：`field.unlock("field_tianxing_39", 6)` 后在一槽种一株一年份灵草（`rules::plant`，`kFieldDaysPerYear = rules::kDaysPerYear`），六句 `advance_days(3600)` 之后该槽 `ripe == true`；其余五槽仍空。
3. **小瓶按境界**（两种改法容量不同，写明用哪一种）：**直接改 `state.realm = CoreEarly`（31）、不走 `RealmAdvance`**，`bottle.capacity` 仍是 fixture 的 6、`bottle.drops` 清零，`advance_days(30)` → `drops == min(6, 30 / 3)` 即 6（容量 6 封顶）；若改成先执行 `realm.advance(realm.CORE_EARLY)`，容量会被 `max(capacity, bottleCapacityFloor(Core))` 抬到 9（`Application.cpp` 第 1337 行），30 日就凝到 9——5.2 第 1 条钉这一点。`realm` 保持 `FoundationLate`（23）、同样清零、`advance_days(14)` → `drops == 2`（5 日一滴）。两条合起来钉 `bottleChargeDays` 的 3 / 5。
4. **静养**：`hp` 砍到 1 后 `advance_days(3600)` → `hp == maxHp`。
5. **整数旗标**：`flag.set("ch11.kaidian_ri", today())` 后 `flag.get("ch11.kaidian_ri") == day`；再 `advance_days(3600)` ×2 后 `today() - flag.get(...) == 7200`（n29 补足算法读的就是这个差）。
6. **耗时**：第 1 条六句合计用 `std::chrono` 量，断言 < 2 秒（宽松上限，只防 `growField` / `refill` 退化成逐日循环）。

### 2.3 实现记录（引擎路交付时填）

- 代码未改。判据 `tests/Ch11EngineTests.cpp` 夹具 `Ch11Fixture`（`io::loadGame(tests/fixtures/ch10-end-first.sav)` → `app_.state()`，先验 `day 16003 / realm 23 / realmCap 23 / formerRealm 22 / bottle.capacity 6`）共 5 条用例、6 条判据：
  - 第 1、6 条 → `OneCallClampsToTheMaximum…`：`advance_days(21600)` → `day == 16003 + 3650`；六句 `advance_days(3600)` → `16003 + 21600`，六句合计 `std::chrono` < 2 s（实测约 120 ms 含泵帧）。
  - 第 2 条 → `AFieldOfSixSlotsRipens…`：`field.unlock("field_tianxing_39", 6)` → `rules::plant` 一株黄精草 → 六句 3600 → 槽 0 `ripe`、`age ≥ kRipeAge`，槽 1–5 空且未熟。
  - 第 3 条 → `TheBottleChargesEveryThreeDays…`：**直接改 `state.realm = CoreEarly`**（不走 `RealmAdvance`，容量仍 6）、`drops 0`、`lastChargeDay = day` → `advance_days(30)` → `drops == min(6, 30/3) == 6`；改回 `FoundationLate` 同样清零 → `advance_days(14)` → `drops == 2`。
  - 第 4 条 → `SixtyYearsOfRestHealToFull`：`hp 1` → `advance_days(3600)` → `hp == maxHp`。
  - 第 5 条 → `TodayStoredInAFlag…`：`flag.set("ch11.kaidian_ri", today())` 后 `flag == day`；两句 3600 后 `day − flag == 7200`；再让脚本自己算 `today() - flag.get(...)` 写进探针旗标也是 7200。
- **写法偏差（与 2.2 第 1 条「写进 TempDir」的字面不同）**：`ScriptHost::startEvent` 只收 scriptRoot 下的相对路径（`isSafeRelativePath`，绝对路径被拒），而 `scripts/` 不在引擎路白名单里。夹具于是把真 `scripts/common/api.lua` 复制进 `TempDir/scripts/common/`，调公开的 `app_.scripts().init(TempDir/scripts, &app_.state())` 重挂 scriptRoot（同一个宿主对象、同一个 `GameState` 指针，`Application` 的轮询与 `dispatch` 不受影响），探针脚本写成 `TempDir/scripts/probe_N.lua`、`startEvent("probe_N.lua")`。泵帧与 `completeCommand` 照 `Ch02BottleTests` 第 66–75 行。

---

## 3. E3 战斗 ④ 「困而不杀」的数据表达（验证项，不改代码）

### 3.1 一句话口径

**`tiandu_shi` 写 `killable_by ["水", "火"]`（不是空数组——加载器与门禁都拒空），`weaknesses` 也含「水」「火」；只有水、火那一击能把它压到 0，其余类别最多压到 1。「困住」「它不会死」是文案与图鉴注，胜负在引擎里仍是气血归零。** 笔记 O-1 的答案：引擎没有「架势被破两次即胜」，也不加；`toughness` 低只是让玩家容易破势吃 ×2，不是胜利条件。

- 四友军的手段：`xueyu_zhizhu_ally` 的「吐网」写成水类法术 **`magic_zhizhu_tuwang`**、`kuilei_yuan` 的「紫火」写成火类法术 **`magic_kuilei_zihuo`**（内容路建 `data/magics` 这两张，敌方的刺球光罩 / 变身 / 自愈全走 `charge`、不建法术，我方 0 门；规则 25 的 `BATTLE_EXTRA_MEANS` 登 `role_magic`，第 8.2 节）；韩立自己有冰冻术（水）与火弹术（火）。
- 「每回合自愈」引擎没有回血机制：用 `charge`（蓄势）写它的威胁、用高 `hp` 写它的「打不死」，平衡路量。

### 3.2 判据（`tests/Ch11EngineTests.cpp`，直接构 `BattleState`，不开 `BattleScene`）

1. **压到 1**：敌方一单位 `hp 10`、`killableBy = 水|火` 位掩码（用 `io` 的类别表换算，不写数字），我方用「剑」类别 `strike` 基础伤害 100 → `hp == 1`、未阵亡、战斗未结束。
2. **水火要得了命**：同一单位用「水」类别同样一击 → `hp == 0`、阵亡；全部敌方阵亡后 `BattleState` 判我方胜。
3. **0 = 谁都杀得死**：`killableBy == 0` 的对照单位，「剑」一击 → `hp == 0`。
4. **真数据（内容路落地后）**：`data/roles/tiandu_shi.json` 的 `killable_by` 集合恰 `{水, 火}` 且 ⊆ `weaknesses`；`data/battles/b11_tiandu_shi.json` `chapter == 11`、`defeat_is_fatal true`、`can_escape false`、友军含 `xueyu_zhizhu_ally` ×2 与 `kuilei_yuan` ×2。文件不存在时 `GTEST_SKIP`。

### 3.3 实现记录（引擎路交付时填）

- 代码未改。`tests/Ch11EngineTests.cpp` 直接构 `core::battle::BattleState`（不开 `BattleScene`），4 条用例：
  - 第 1 条 → `Ch11TrappedCorpse.ASwordStrikeStopsAtOneHitPoint…`：敌方 `hp 10`、`killableBy = categoryFromName("水") | categoryFromName("火")`（用 `core/model/AttackCategory.h` 的名字表换算，不写数字）、`weaknesses` 同；我方 `weapons = 剑|水`、`attack 60`、`defence 0` 对 `defence 0`，先验 `estimateHitDamage ≥ 10`；`Action{Attack, category 剑}` → `hp == 1`、`alive()`、`phase() == Ongoing`。
  - 第 2 条 → `…AWaterStrikeKillsAndTheBattleIsWon`：同一编成、「水」一击 → `hp == 0`、`!alive()`、`phase() == Won`（`apply` 内部 `refreshPhase`，不必 `endTurn`）。
  - 第 3 条 → `…AUnitWithoutKillableByDiesToTheSword`：`killableBy 0` 对照 → 剑一击 `hp == 0`、Won。
  - 第 4 条 → `…TheRealRoleIsKillableByWaterAndFireOnly…`：`data/roles/tiandu_shi.json` 的 `killableBy` 恰 水|火 且 ⊆ `weaknesses`；`io::loadBattle(data/battles/b11_tiandu_shi.json)` `chapter 11`、`defeatIsFatal true`、`canEscape false`、`ally` 里 `xueyu_zhizhu_ally` ×2、`kuilei_yuan` ×2。两文件任一不存在 `GTEST_SKIP`——交付时内容路已落地，实跑绿、未跳过。

---

## 4. E4 只收的商店与玩家自己的店（验证项，不改代码）

### 4.1 一句话口径

**`entries` 为空的商店是合法的：买入页禁用（「货架上空空如也」）、卖出页照开、只列 `tradeable` 的物品、价钱是 `item.price × sellRate` 取整。凝翠岛收购行 `sellRate 0.8`（五级妖丹 760、六级 4800）、青竹小轩 `sellRate 0.6`，都低于各自的 `buyRate 1.0`，加载器放行。** 笔记 O-6（玩家卖符给自己的店怪不怪）是文案问题，不在本节。

- 规则层 / 加载器 / 游戏层：**都不改**。四张商店（`ch11_fangshi_zahuo`、`ch11_ningcui_zahuo`、`ch11_ningcui_shougou`、`ch11_qingzhu_xiaoxuan`）归内容路，形状照 `data/shops/ch10_tiandujie_pu.json`。
- 内容路要知道的：story 物品（`story_*`）与 `tradeable false` 的东西（`material_tianleizhu`、`material_lianjian_fuliao`、`weapon_qingzhu_fengyun_jian`，以及**`material_jinleizhu`——现文件是 `tradeable true`、`price 20000`，内容路回写 B13 时顺手改 `false`**，否则青竹小轩卖出页按 `price × 0.6` 列它、一根 12000 低阶、六根 72000，收购行 0.8 一根 16000，章末「低阶数百到四千」与「只卖符与药材」都不成立）**不会出现在卖出页**，不必另设门闸；`material_yaodan_liuji` 要写 `tradeable true` 才卖得掉。

### 4.2 判据（`tests/Ch11EngineTests.cpp`）

1. **加载器**：临时目录三张：无 `entries`、`sellRate 0.8` → `ok` 且 `entries.empty()`；`sellRate 1.0`、`buyRate 1.0` → `ok == false` 且 `error` 含「sellRate」；`entries: []` → `ok` 且空（两种空写法等价）。
2. **价钱**（契约定取整，字面量写死——判据不从被测物推导，`docs/handoff.md` 第 6 节第 5、18 条）：`rules::sellPrice(shop{sellRate 0.8}, item{price 950}, 0)` **恰 760**；`item{price 6000}` **恰 4800**；`sellRate 0.6`、`price 10` **恰 6**（三个都是整除，不涉及取整方向）；**取整方向由本文定为四舍五入**（`Economy.cpp` 第 43–48 行 `saturateToInt` 用 `std::llround`，本文照它写死，日后实现若改向下取整要先改本文）：再加一条非整除钉方向——`sellRate 0.8`、`price 957` **恰 766**（765.6 四舍五入）；`price 951` **恰 761**（760.8）。字面量写进用例，不读实现算。
3. **补货不动**：`rules::restock(shop, day)` 对空 `entries` 不崩、仍空。
4. **面板**（真 `ShopScene("<临时空店 id>")`，headless）：主菜单「买入」`enabled == false`、`disabledReason == "货架上空空如也"`；「卖出」可进；背包放五级妖丹 1、`story_waihai_tu` 1、`material_tianleizhu` 1 → 卖出页恰 1 行。
5. **真数据（内容路落地后）**：四张店都加载成功；`ch11_ningcui_shougou` 与 `ch11_qingzhu_xiaoxuan` 的 `entries` 空、`sellRate` 分别 0.8 / 0.6；`ch11_fangshi_zahuo` 含 `herb_nichang_cao`。文件不存在时 `GTEST_SKIP`。

### 4.3 实现记录（引擎路交付时填）

- 代码未改。`tests/Ch11EngineTests.cpp` 5 条用例：
  - 第 1 条 → `Ch11BuyOnlyShopData.EmptyEntriesLoadEitherWay…`：临时目录三张照 4.2 写（无 `entries` / `sellRate == buyRate` 报错含「sellRate」/ `entries: []`），另加负向 `"entries": 3` 必须被拒且报「entries」。
  - 第 2 条 → `Ch11BuyOnlyShopRule.SellPricesAreTheContractLiterals…`：`sellRate 0.8` × `price 950 / 6000 / 957 / 951` 恰 760 / 4800 / 766 / 761；`0.6 × 10` 恰 6。字面量写死，四舍五入方向由本文定。
  - 第 3 条 → `…RestockLeavesAnEmptyShelfEmpty`：`restock` 两次不崩、仍空。
  - 第 4 条 → `Ch11ForgePanel.AnEmptyShopHasNoBuyRows…`：手构 `Shop{sellRate 0.8, entries 空}`，背包五级妖丹 1 ＋ `story_waihai_tu` 1 ＋ `material_tianleizhu` 1（先验后者 `tradeable false`）：`buildBuyItems` 恰 1 行（只剩「返回」）；`buildSellStacks` 恰 1 堆且是妖丹、单价 == `rules::sellPrice`；`buildSellItems` 恰 2 行（1 ＋ 返回）。
  - 第 5 条 → `Ch11BuyOnlyShopData.TheFourRealShopsLoad…`：四张真店 `loadShop` 全 ok；收购行 / 青竹小轩 `entries` 空、`sellRate` 0.8 / 0.6；方市杂货含 `herb_nichang_cao`。任一文件不存在 `GTEST_SKIP`——交付时已落地，实跑绿。
- **未钉到的一句**：4.2 第 4 条「主菜单『买入』`enabled == false`、`disabledReason == "货架上空空如也"`」——`ShopScene::enterMain` 与 `main_`（`ui::ListView`）都是 private，无头测试拿不到那一行；`src/game/ShopScene.h` 不在引擎路白名单，没加访问器。第 4 条改钉「买入页只剩返回」这一等价事实（`enterMain` 判空的是同一个 `shop->entries.empty()`）；要钉那一句原话需另给 `ShopScene` 一个只读的 `mainItems()`（一行），由协调者定。

---

## 5. E5 结丹初期的几处引擎口径（验证项，不改代码）

### 5.1 一句话口径

**n16 一句 `realm.advance(realm.CORE_EARLY)` 把 23 → 31，上限顺带抬到 31，气血法力按差额上抬；此后面板上「结丹中期」被 `StoryCap` 挡住（本章 `realm.cap` 0 处）；小瓶自此 3 日一滴；三十九层静室 `effectiveness 125`、炼器台 `forge` 3 品挂 `require_flag ch11.liugen`。** 这些全是既有机制，本节把它们对本章的具体数值钉死。

- `storyCap` 在 n16 之前保持 23：面板上的下一关「结丹初期」被挡，脚本不替玩家冲（笔记 C.3）；n16 之后下一关是「结丹中期」、所需修为 5000（`Realm.cpp` 第 143–146 行），仍被挡。
- 降尘丹 5、雪灵水 2 在 n16「有则扣」只是 `take`，**不接 `breakthroughPillBonus`**（它仍恒 0，第 10 章契约 1.3 的那一行注释照旧）。

### 5.2 判据（`tests/Ch11EngineTests.cpp`）

1. **剧情升境**：从 `ch10-end-first.sav` 载入（`realm 23 / realmCap 23 / formerRealm 22 / bottle.capacity 6` 先验），脚本 `local ok = realm.advance(realm.CORE_EARLY)` → `ok` 真、`realm == 31`、`realmCap == 31`、`formerRealm` 仍 22、`maxHp` 抬到结丹初期基准、`hp` 抬同样的差额（`liftToFloor`，不是补满——先把 `hp` 砍到一半再执行，钉这一点）、**`bottle.capacity == 9`**（`Application.cpp` 第 1337 行 `max(capacity, bottleCapacityFloor(tierOf(31)))`，`Bottle.h` 结丹 9；`drops` 不变——抬上限不送液）。
2. **不许过结丹中期**：上一条之后 `cultivation` 置 5000，`rules::breakthroughBlock(realm, cultivation, realmCap) == BreakthroughBlock::StoryCap`；真 `CultivationScene` 主菜单「冲关」那一行 `enabled == false`。
3. **n16 之前也挡**：原始存档 `cultivation` 置 3500，`breakthroughBlock(23, 3500, 23) == StoryCap`。
4. **小瓶**：`bottleChargeDays(Realm::CoreEarly) == 3`、`(FoundationLate) == 5`（第 2.2 节第 3 条已经由推进证过，这一条钉函数本身）。
5. **静室**：headless `Application` 载入一张临时图（或内容路落地后的 `ch11_dongfu`），`facility_jingshi_39` 的 `effectiveness` 属性读出 125，`openFacility` 之后栈顶 `CultivationScene::sitePercent() == 125`；`bankMeditation(state, 60, seed, 125)` 的进账 y 与同种子 `sitePercent 100` 的进账 x 满足 `y ∈ [x × 5 / 4 − 1, x × 5 / 4 + 1]`（取整误差）。
6. **炼器台门闸**：`facility_lianqi_39`（`forge`、`grade 3`、`require_flag ch11.liugen`）在 `ch11.liugen` 未置时 `openFacility` 不开面板——**栈顶是 `DialogueScene`**（`Application.cpp` 第 682–694 行 `showMessage("ui.facility.locked")` → 第 965–971 行 `pushScene(DialogueScene)`），不是 `AlchemyScene`，也不是行走；已置时栈顶是 `AlchemyScene` 且 `kind == Forge`、`toolGrade == 3`。
7. **负向**（只做一条，其余都是读既有机制）：把第 1 条脚本改成 `realm.advance(realm.CORE_MID)`（32）→ `realm == 32`——这条**必须绿**，证明引擎不替脚本把关、「本章 `realm.advance` 恰 1 处且目标是 31」要由 `tests/Ch11*` 的脚本扫描钉（测试路）。

### 5.3 实现记录（引擎路交付时填）

- 代码未改。`tests/Ch11EngineTests.cpp` 8 条用例（7 条判据 + 真图一条）：
  - 第 1 条 → `Ch11Fixture.StoryAdvanceToCoreEarly…`：先把 `hp` 砍到一半，脚本 `local ok = realm.advance(realm.CORE_EARLY)` → ok 真、`realm 31`、`realmCap 31`、`formerRealm` 仍 22、`maxHp == realmMaxHp(CoreEarly)`、`hp == 半血 + 差额`（不是补满）、`bottle.capacity == 9`、`drops` 不变。
  - 第 2 条 → `…CoreMidIsBlockedByStoryCap…`：升境后 `cultivation 5000` → `breakthroughBlock == StoryCap`；`CultivationScene::buildMainItems` 第 2 行 label 是修仙表的「冲关」、`enabled false`、`disabledReason` 非空。
  - 第 3 条 → `Ch11CoreEarlyRule.FoundationLateAtItsCap…`：`breakthroughBlock(23, 3500, 23) == StoryCap`。
  - 第 4 条 → `…BottleChargeDaysAreThree…`：`game::bottleChargeDays(CoreEarly) == 3`、`(FoundationLate) == 5`。
  - 第 5 条 → `Ch11Fixture.TheOneHundredTwentyFiveRoom…`：手构 `MapObject{kind meditate, effectiveness "125"}` → `openFacility` → 栈顶 `"Cultivation"`；`bankMeditation(…, 60, seed, 125)` 对同种子 `100` 的进账，seed 1..8 都满足 `|4y − 5x| ≤ 4`。真图另一条 `Ch11CoreEarlyData.TheRealDongfuMap…`：`io::loadTileMap(maps/ch11_dongfu.tmj)` 的 `facility_jingshi_39` `effectiveness == "125"`、`facility_lianqi_39` `kind forge / grade "3" / require_flag ch11.liugen`（图不存在 `GTEST_SKIP`，交付时已落地）。
  - 第 6 条 → `…TheForgeShowsTheLockedMessageUntilLiugen…`：手构 `MapObject{forge, grade 3, require_flag ch11.liugen}`，旗标 0 → 栈顶 `"Dialogue"`；置旗 → 栈顶 `"Alchemy"`、`kind() == Forge`、`toolGrade() == 3`。
  - 第 7 条 → `…TheEngineLetsAScriptAdvanceToCoreMid…`：`realm.advance(realm.CORE_MID)` → `realm == 32`，绿。
- **偏差**：5.2 第 5 条写的 `CultivationScene::sitePercent() == 125`——该类没有这个访问器（`sitePercent_` 私有），用例改断 `siteBonusLine(state)` 含 "25"（与 `Ch08EngineTests` 同一比法），加成本身由 `bankMeditation` 那一条钉死。

---

## 6. 第 12 章接口（本批**不实现**，写成约定；笔记 O.1 逐行照抄）

第 12 章施工图与它的契约照这张表接；要改表里任何一行，先改本文。

| 接口 | 本章留下什么 | 第 12 章怎么用 |
| --- | --- | --- |
| **境界** | 章末 31 / 31（结丹初期），`formerRealm 22` | 结丹中期（大纲 4.3 第 12–13 章）怎么上由第 12 章定；本章 `realm.cap` 不抬到 32 |
| **本命法宝** | `weapon_qingzhu_fengyun_jian` ×1（兵刃「剑」，`tradeable false`） | 法术化（剑阵、分光）由第 12 章定；规则 25 的「剑」本章起多一来路 |
| **必成配方** | `Recipe::guaranteed`（E1）；全仓只 `recipe_qingzhu_fengyun_jian` 一张为真 | 第 12 章以后要再用必成，先改本文 1.1「只许这一张」那一句 |
| **分身** | `qu_hun_shadan` 在队，数值、名牌不动 | 同上；它仍是一名队员 |
| **妖丹** | 五级 0（n26 扣光）、六级 0–1（`material_yaodan_liuji`，② 可选战利品） | 结丹中期要六级妖丹入药（笔记 A73）——第 12 章若要门闸，别依赖那颗可选的 |
| **妙音门** | `story_miaoyin_yaopai`；紫灵、范、卓三人已迁天星城（n25 台词） | 虚天殿同行（ch431 起）由腰牌引出 |
| **极阴岛** | 韩立知道：乌丑劫货嫁祸、极阴附身乌丑（ch405 原文称「极阴老祖」）、虚天残图**疑在**隐煞门孙姓门主（乌丑的孙师叔）手——乌丑只问「有没有带在身上」、未得确认（施工图 A56 同此措辞）、虚天殿三百年一现（n23 一句） | 第 12 章开场的钩子；`wu_chou`（结丹中期）数值是第 12 章的 |
| **金青** | `story_bai_zhenzhu`；金青 NPC 在 `ch11.done` 后离场（`hidden_flag ch11.done`，2026-10-13 按内容复验 C-16 改；第 12 章的金青是新图上的另一个对象） | 古修士遗址（ch416 起）第 12 章开场 |
| **噬金虫** | 仍是 `story_shijin_chong`；旁白里已「认主、数万」 | 做成手段（灵兽袋、战斗单位）由第 12 章定 |
| **洞府** | `ch11_dongfu` 可回；`field_tianxing_39` 6 槽；静室 125；炼器台 forge 3 品 | 第 12 章开局可从这里接（照第 10 → 11 先例 patch `zhuzhong.lua` 末尾补一行 `teleport`） |
| **出海** | 星空殿在 `ch11.huicheng` 后只说一句「本章不再出海」 | 第 13 章外星海另起图 |
| **开局** | 章末落点：`ch11_dongfu` 门外（n31）；`ch11.done` 已置；第 11 章是 `data/chapters.json` 最后一章，「未完待续」自动接 | `data/chapters.json` 加第 12 章之后，「未完待续」自动挪 |
| **时间** | `advance_days` 单次 3650 不变（E2）；`day` ≈ 五万以内 | 第 12 章若要更长的跳跃仍分段 |
| **灵石** | 从 `ch11-end-{first,second}.sav` 读（中阶 ≈ 20、低阶数百到四千） | 不从施工图抄 |
| **规则 25** | `MEANS_LAST_CHAPTER = 11`；探针在 `wu_chou` + `b12_neidian_duoding` | 第 12 章写实那张编成时探针再挪（第 13 章 `wuji_haishou` + `b13_waixinghai_lieyao` 也是唯一） |
| **词表** | M 表已登；`tests/LexiconTests.cpp` 表二 `{11, 415}` 早已在 | 文思月 ch430、明珠轩 ch513、交换会 ch535 仍禁 |
| **Lua 只读** | 不加 `realm.former()` | 同第 10 章契约第 3 节末行 |

---

## 7. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让几路对同一件事只有一种写法（id、旗标、数字一律以笔记为准）：

- **境界**：`realm.advance` 恰 1 处（`scripts/ch11/biguan.lua`，`realm.CORE_EARLY`），写 `local ok, code = realm.advance(realm.CORE_EARLY)`；`ok` 为假时 `talk` 一句占位、`return`（**不许静默吞掉**）。`realm.cap` 0 处、`realm.demote` 0 处（散功只写旁白，笔记 A42）。**脚本不读 `formerRealm`**。
- **六十年 / 二十年**：`biguan.lua` 六句 `advance_days(3600)`，三段之间各一句 `talk`，不加画面（笔记 E5）；`peizhu.lua` 收店那一支用 `today() - flag.get("ch11.kaidian_ri")` 补足到 7200（至少 360，照 `scripts/ch10/sanzhuan.lua` 的写法），补足值超过 3650 时分两句。任何一句 `advance_days` 的参数 ≤ 3650。
- **必成配方**：`data/recipes/forge/qingzhu_fengyun_jian.json` 就地改写成 1.4 的形状（`inputs` 只有金雷竹 6 ＋ 辅料 1，无灵石；`guaranteed true`）；`facility_lianqi_39` `grade 3`（不许 0）、`require_flag ch11.liugen`、`deny_text_key` 按规则 20 成对。炼剑**走面板**，脚本里 0 处 `give("weapon_qingzhu_fengyun_jian")`；`trigger_chujian` 用 `item.count("weapon_qingzhu_fengyun_jian") >= 1` 判。
- **困而不杀**：`data/roles/tiandu_shi.json` `killable_by ["水","火"]`，`weaknesses` 含这两样；编成 `b11_tiandu_shi` 胜利文案写「困住」不写「杀死」（笔记 E25）。
- **商店**：四张（笔记 L.4）；收购行与青竹小轩 `entries` 不写或写 `[]`；`sellRate` 0.8 / 0.6；`material_yaodan_liuji` 新建时 `tradeable true`；四样不可卖的东西 `tradeable false`（`material_tianleizhu`、`material_lianjian_fuliao`、`weapon_qingzhu_fengyun_jian`，以及现有 `material_jinleizhu` 改 false，4.1）。青竹小轩的卖出口不是 shop facility，是 `trigger_guitai` ＋ `guitai.lua`（开头判 `ch11.liugen` 则一句 return，否则 `shop("ch11_qingzhu_xiaoxuan")`，`api.lua` 第 95 行既有命令）——portal / facility 不读 `hidden_flag`，收店后关不掉。
- **小瓶**：诱妖与培竹都是 `bottle.spend(1)` 看返回值（不成则「等几日」`advance_days(5)` 再试）；天雷竹**不走** `bottle.mature`（笔记 D 表）。
- **灵田**：`anju.lua` 一句 `field.unlock("field_tianxing_39", 6)`；旧田 id 一个不碰。
- **静室**：`facility_jingshi_39` 写 `effectiveness 125`（门禁 100–300 之内）；其余打坐设施不写（= 100）。
- **扣东西**：「有则扣」「有几块扣几块」一律看返回值；每一处都有走得通的一支（笔记 N.1 退路列）。`take("material_lingshi_zhong", 全部)` 写成先 `item.count` 再 `take`。
- **整数旗标**：`ch11.lieyao`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.jiesuan`、`ch11.dan_fu`、`ch11.z1`、`ch11.z2` 用 `flag.set(name, n)`；`ch11.kaidian_ri`、`ch11.jiesuan_ri` 存 `today()`（笔记 J 表）；开关旗标 `flag.set(name)`。
- **分身**：`qu_hun_shadan` 全章在队，`party.*` 0 处（笔记 E18）。
- **四张必打 / 可逃**：① `defeat_is_fatal true`、`can_escape true`、`drops` 五级妖丹 1；② `defeat_is_fatal false`、`can_escape true`、`drops` 六级妖丹 1；③④ `defeat_is_fatal true`、`can_escape false`、`drops` 空、`spirit_stones 0`（笔记 I 表附注）。四张 `hero_absent false`。
- **teleport**：脚本 `teleport` **只传图 id**——`scripts/common/api.lua` 第 63 行是 `teleport(map_id, x, y)`、x/y 是整数坐标，引擎 `Application.cpp` 第 1005 行 `loadMap(command.a, std::string{})` 恒落 default spawn，不认 spawn 名（传 `"chengmen"` 会被 `ScriptHost.cpp` 第 178 行 `get_or("x", 0)` 当 0 或抛错）；每张图把脚本落点那个 spawn 标 `default=True`（`genmaps.py` 第 163 行），门走 portal 的 `target_spawn`。按名落点要加引擎（Teleport 命令加 b 字段 → `loadMap(a, b)`），本批不做。七条写在笔记 N.4（含 `scripts/ch10/zhifadui.lua` 末尾补的那一条 `teleport("ch11_fangshi")`），测试路登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`（第 8.3 节）。
- **章末**：`zhuzhong.lua` 最后置 `ch11.done`，不 `ending()`，不写去向。
- **文案**：`data/text/ch11*.json` 里 0 处 `{key.` 与 `{pad.`（第 8.2 节 KeyPrompts）；`ui.chapter.11.*` 归协调者。

---

## 8. 门禁与既有测试的依赖

### 8.1 引擎路

E1 不改 `tools/`：`validate.py` 对配方不查字段（第 3220 行只查 `story.recipe_later`），`guaranteed` 的两条校验在加载器。E2–E5 不改任何 `src/`。共享内容不齐时只报告 `tests/Ch11EngineTests.cpp` 的证据，不报全量 data 绿色。

### 8.2 协调者：集成时落

- **规则 25 的表**（`tools/validate.py` 第 2289–2368 行）：`CHAPTER_MEANS[11]` = 拳（`hand`）、火（`learn magic_huodan_shu ch04`）、土（`learn magic_liusha_shu ch06`）、水（`learn magic_bingdong_shu ch06`）、金（`learn magic_ji_jinfu ch07`）、木（`learn magic_qingyuan_jianmang ch08`——筑基后期与结丹初期都不低于它的 `needRealm`，第 10 章跌落的例外本章不再适用）、暗器（`give weapon_wuming_sixian ch07`）、剑（`give weapon_lieyang_jian ch06`，`scripts/ch06/lingqu.lua` 第 20 行）；`BATTLE_EXTRA_MEANS` 四张各登 `qu_hun_shadan` 的 `("剑","role_weapon","剑","qu_hun_shadan")`、`("火","role_magic","magic_xuelian_guangzhu","qu_hun_shadan")`、`("火","role_magic","magic_xuelian_ziyan","qu_hun_shadan")`、`("金","role_magic","magic_ji_jinjian","qu_hun_shadan")`（照 `b10_zhifadui` 写法）；`b11_tiandu_shi` 再加友军 `("水","role_magic","magic_zhizhu_tuwang","xueyu_zhizhu_ally")`、`("火","role_magic","magic_kuilei_zihuo","kuilei_yuan")`——这两张 `data/magics` 由内容路建（施工图 10.1 C4），登之前核编成友军、角色法术、五行与境界能施展；`MEANS_LAST_CHAPTER = 11`。
- **selftest 探针**（`tools/validate_selftest.py` 第 1801–1823 行）：`outside_role` 改 `data/roles/wu_chou.json`、`outside_battle` 改 `b12_neidian_duoding`（全仓唯一含 `wu_chou` 的编成，2026-10-12 grep 过），`chapter == 12`、敌方含 `wu_chou`，三处措辞改「第 12 章」；**第 1815 行 `probe["chapter"] = 10` 改 11**（「移入本章范围」才对照的是新表）；临时写回 `wu_chou.json` 的是测试副本、还原字节相同（笔记 O-2：「一个字不动」指提交的数据）。`validate_selftest.py` 第 2465 行 `MEANS_LAST_CHAPTER == 10` 的断言改 11。
- `tests/LexiconTests.cpp` 表一补笔记 M 表（形如 `{"词", 首见章, "出处"}`；施工图 12.1 已去掉赵长老、白玉蜘蛛、隆兴客栈，「黄龙」若登须放行「黄龙丹」）；表二 `{11, 415}` 已在，不动。
- `data/flags.json`：施工图第 11 节 **39 条**旗标（含 `kaidian_ri`、`jiesuan_ri`、`z1`、`z2`、`lianjian_qian`；不含 path）在内容路派工之前登记，`ch11.kaidian_ri` / `ch11.jiesuan_ri` 说明写「整数日期，不是开关」；`ch11.path.<id>` 14 条等 `data/pathactions/ch11.json` 落地后登记。现有 449 条、`ch11.*` 零撞名（已核）。
- `data/text/battles.json`：删旧占位 `ch11.battle.ningcuidao_youyao.intro`（第 6 行）——四场开场白由内容路写在 `data/text/ch11_battle.json`，这个文件只有协调者动（9.3）。
- `data/chapters.json` 加 `{"number":11,"numeral_key":"ui.chapter.11.numeral","title_key":"ui.chapter.11.title","done_flag":"ch11.done"}`；`data/text/ui.json` 加 `ui.chapter.11.numeral`「第十一章」、`ui.chapter.11.title`「结丹」（卡上不写「天星城」，笔记 E2）。
- **KeyPrompts 白名单**（`tests/KeyBindingTests.cpp` 第 556–557、605–609 行两组 `allowed` 与第 586 行的占位计数先验）：本章文案 0 处占位则不动；若 `objective.ch11.*` 某条要带 `{key.*}` 提示（照 `objective.ch09.n20_huicheng` 先例），两组各加一条、`.pad` 版同时写、先验计数随之加。
- `docs/handoff.md` 第 4 节那张「仍然只有规则层的」表：`rules::Economy` 的拍卖那一半那一行改成「第 11 章不做面板（契约 `docs/interfaces-p3-ch11.md` 头部不做清单）；第 12/13 章再议」；`rules::Realm::demote` 那一行照旧。
- 回写（笔记 E30）：大纲删「文樯」「木龙真人」（B1、B2）；人物总表 `fan_furen` summary（B3）、极阴祖师首见 ch382（B5）、赵长老/符长老加注（B6）；`material_jinleizhu` note「带根一节」（B13）。

### 8.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/Ch10*`（通关）| `Ch10AcceptanceTests` 用 `verifyRun(driver, kChapterTenEndingFirst)` 与夹具逐字段比（第 51 行），没有单独的 `mapId` 断言（**更正，2026-10-13 测试路 A 实测**：终点断言在 `tests/Ch10Driver.h` 第 520–521 行，`mapId == "ch10_xiaohuan"`、`position {50,39}`，已改成 `ch11_fangshi` / `Point{28,38}`——城门 default spawn `chengmen`；测试质量复验 R-2）；第 25 行断 `cards.back() == ToBeContinued` | `ch10-end-{first,second}.sav` 按 `zhifadui.lua` patch 之后重生成、夹具逐字段比自然带上 `ch11_fangshi` / `chengmen`（笔记 O-10）；**章节卡断言改 `Opening(11)`**——第 11 章进 `chapters.json` 后 `cardsForFlagChange` 对 `ch10.done` 排的是 Closing(10)＋Opening(11)，只有表里最后一章才排「未完待续」（`ChapterCardScene.cpp` 第 88–99 行） | 测试路 |
| `tests/ObjectiveTests.cpp` `kScriptTransfers` | 第 668 行 `{"ch10.done", "scripts/ch10/zhifadui.lua", "ch10_xiaohuan"}` | 那一行 `toMap` 改 `ch11_fangshi`；加三元行五条：`ch11.zufu → ch11_dongfu`、**`ch11.chuansong → ch11_ningcui_jie`（`xingkong.lua`，三元——它挂在 NPC 上、首次置旗，过不了五元那张表；脚本把 `flag.set("ch11.chuansong")` 与 `teleport("ch11_ningcui_jie"` 写在顶格、早退用顶格 `return`）**、`ch11.huicheng → ch11_dongfu`、`ch11.zhuzhan → ch11_yinsha_dao`、`ch11.yaoshi → ch11_dongfu`；回城传送一条五元行（照第 652 行 `suidao_hui`）：`{"ch11.chuansong", "scripts/ch11/huicheng_chuansong.lua", "ch11_xingkong", "trigger_huicheng_chuansong", "ch11_ningcui_jie"}`——该对象 `type trigger`、`mode enter`、`once false`、`guard_flag ch11.chuansong`、`set_flag` 空，脚本 0 处 `flag.set(`（第 862–873 行的用例就这么查） | 测试路 |
| `tests/ObjectiveTests.cpp` `kOffChainDoorKeys` | 只有 `ch01.done` | 目标链第 27 步 done_flag 直接是 `ch11.liugen`（施工图 3.4），本章无链外门钥匙；`portal_shangshan` 的 `ch11.zufu` 也在链上 | 测试路 |
| `tests/NpcPresenceTests.cpp` `kOffChainFlags` | 只到第 3 章 | 登八条（施工图已拍板目标链 29 步，`ch11.jiguan`、`ch11.jiesuan` 不在链上）：`ch11.jiguan`（`youyao.lua`，`ch11.lieyao` → `ch11.huicheng` 之间）、`ch11.dan_fu`（`biguan.lua`）、`ch11.jiesuan`（`jiesuan.lua`，`ch11.kaidian` → `ch11.liugen`）、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`（`peizhu.lua`）、`ch11.lianjian_qian`（`lianjian.lua`，`ch11.liugen` → `ch11.lianjian`）——只登在场旗标真用到的那几条，用例红了再补 | 测试路 |
| `tests/NpcPresenceTests.cpp` `kLastChapter` | **9**（第 10 章契约 5.3 写的「改到 10」未落） | 改到 11（第 10 章目标链 `data/objectives/ch10.json` 已在，一并覆盖） | 测试路 |
| `tests/PathActionTests.cpp` `realmCapOfChapter()` | 第 896–901 行到 `{10, FoundationLate}` | 加 `{11, Realm::CoreEarly}`（`biguan.lua` 的 `realm.advance(realm.CORE_EARLY)` 与 `ch11.done` 不在同一脚本，本章就按 31 算）；本章 14 条全 `realm 0` | 测试路 |
| `tests/Ch03BattleKitTests.cpp` 第 52 行注释 | 提到 `b11_ningcuidao_youyao`「三级海兽一条法术没声明」 | 只是注释引用，不是用例表（`tests/SliceTests.cpp` 里没有「迁移来的仗」那张表，`b11_ningcuidao_youyao` 在 tests/ 下仅此一处）；编成改写后注释那一句随手改成「已在第 11 章写实」，不改用例 | 测试路 |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | 加 `ch11-end` 两份（v9，本章不加存档字段） | 测试路 |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | ①② 的 `drops` 是妖丹（不是灵草）——若该用例只放行灵草，按遭遇/剧情战分开判；红了测试路改用例，不改编成 | 测试路 |
| `tests/Ch10EngineTests.cpp`、`RealmCapTests.cpp`、`PanelTests.cpp` | 钉重修与 StoryCap 措辞 | 不动（本章存档 `formerRealm 22` 已越过，重修恒假） | — |
| 第 7 章配方用例（`tests/Ch07*` 读 `data/recipes/**`）| 若钉「每张配方 `difficulty > 0`」或「熟练门槛单调」 | 先跑；红了把 `recipe_qingzhu_fengyun_jian` 按 `guaranteed` 放行，判据不改 | 引擎路 |
| `tests/Ch06SliceTests.cpp` | 读 `MEANS_LAST_CHAPTER ≥ 6` | 不动 | — |

---

## 9. 白名单（四路按文件分；两两不重叠，共享文件归唯一一路）

### 9.1 引擎路

- `src/core/rules/Crafting.h`、`src/core/rules/Crafting.cpp`（只追加 `Recipe::guaranteed`、`kGuaranteedSuccessChance`、`static_assert` 与 `successChance` 的早退一句）
- `src/io/RecipeLoader.h`、`src/io/RecipeLoader.cpp`（只读 `guaranteed` 与那条 `requireFlag` 不变量）
- `tests/Ch11EngineTests.cpp`（新；E1–E5 全部判据）；`tests/Ch07*` 配方用例（只在 8.3 那一格红了的时候）
- `docs/interfaces-p3-ch11.md`（只填 1.6、2.3、3.3、4.3、5.3 与末尾「实现记录」）

### 9.2 内容路（另一份派工单写在 `docs/ch11-design.md` 施工图头部；这里只列边界）

- `maps/ch11_{fangshi,dongfu,xingkong,ningcui_jie,honghu,yinsha_dao,xiaoxuan}.tmj`（新 7 张）、`maps/tilesets/terrain_ch11_*.tsj`（生成器要的话，新）
- `tools/mapgen/genmaps_ch11.py`（新）、`tools/mapgen/genmaps.py`（只加登记，排在第 10 章之后；本章没有 patch 调用）
- `scripts/ch11/**`（新）；`scripts/ch10/zhifadui.lua`（**只在末尾补一行 `teleport("ch11_fangshi")`**——只传图 id、落 default spawn `chengmen`，第 7 节「teleport」；此前一个字不动）
- `data/text/ch11*.json`（新，按前几章的拆法；0 处 `{key.` / `{pad.`）
- `data/roles/*.json`（新建笔记 H 表那些；**就地改写** `siji_haishou.json` 的 `weaknesses`；`wu_chou.json`、`zi_ling.json`、`qu_hun_shadan.json` 一个字不动）
- `data/items/**`（新建笔记 L.1 那 **13 件**：`material_yaodan_liuji`、`herb_nichang_cao`、`material_tianleizhu`、`material_lianjian_fuliao` ＋ 9 件 story；就地改写 `material_jinleizhu.json` 的 `tradeable` 与 note）、`data/magics/*.json`（新建 2 张友军法术 `magic_zhizhu_tuwang`〔水〕、`magic_kuilei_zihuo`〔火〕；敌方全走 `charge`；我方 0 门）
- `data/recipes/forge/qingzhu_fengyun_jian.json`（就地改写成 1.4 形状）
- `data/battles/b11_ningcuidao_youyao.json`（就地改写）、`data/battles/b11_{honghu_jiguanjiao,yinsha_shayao,tiandu_shi}.json`（新）
- `data/quests/q11_{shougou_dingdan,linju_dingyan}.json`、`data/pathactions/ch11.json`、`data/objectives/ch11.json`、`data/shops/ch11_{fangshi_zahuo,ningcui_zahuo,ningcui_shougou,qingzhu_xiaoxuan}.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`tools/artgen/sprites_enemies.py`（只加千目妖、鸡冠蛟、煞妖变身态、妖尸、血玉蜘蛛友军）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加 7 条登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch11-design.md`（**只追加「施工偏差」一节**，平衡路的实测与 `Realm.h` 第 175 行锚点注释的改法也写在这里；`src/core/rules/Realm.h` 那一段注释由平衡路改，算内容路边界）

### 9.3 协调者

- `tools/validate.py`（**只** `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段）、`tools/validate_selftest.py`（只挪探针与第 2465 行那一处断言）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第十一章」「结丹」）；`data/text/battles.json`（**只删**旧占位 `ch11.battle.ningcuidao_youyao.intro`，8.2）
- `tests/LexiconTests.cpp`（表一补词）；`tests/KeyBindingTests.cpp`（只在 8.2 那一条触发时）
- `docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`、`docs/lore/人物总表.md`（回写）

### 9.4 测试路（引擎路与内容路都落地、协调者 8.2 之后另派）

- `tests/Ch11*.cpp`（除 `Ch11EngineTests.cpp`）：通关、挂点、编成、账、切片、验收
- `tests/Ch10*.cpp`（只改通关终点断言那一处）
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`、`tests/SliceTests.cpp`、`tests/BattleRewardTests.cpp`（只按 8.3）
- `tests/ChapterFixture.h`、`tests/fixtures/ch10-end-{first,second}.sav`（重生成）、`tests/fixtures/ch11-end-{first,second}.sav`（新）、`saves/README.md`

---

## 10. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记主线旗标（J 表）→ 引擎路（E1 + E2–E5 判据）∥ 内容路 → 协调者 8.2 与集成 → 测试路（先改第 10 章通关终点、重生成 `ch10-end`，再写第 11 章通关）→ 独立校对。**本章在第 10 章集成之后开工**。
- **跨路依赖只有一处**：内容路的配方 JSON 多了 `guaranteed` 键。`io::RecipeLoader` 按键名读、不遍历对象（`RecipeLoader.cpp` 第 25–43 行的 `readInt / readBool / readString`），E1 落地之前这个键被**静默忽略**，两路互不阻塞、落地顺序无所谓。没有 E1，通关照样走得通（面板 65% 成功率，多按几次），但炼器失败会折掉三根金雷竹、第二次就开不了工——验收「炼剑一次必成」与 `tests/Ch11*` 的账用例会红。
- 每路用自己的构建槽：引擎路 `ch11eng`，内容路 `ch11content`，测试路 `ch11test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 几路共用一棵工作树时，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实（`tools/novel/novel.py`），不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- **门禁先于编译跑**：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`；引擎路交付前跑 `python tools/validate_selftest.py`。
- **旧图不 patch**：本章 0 张；`genmaps_ch05.py`–`genmaps_ch10.py` 一个字不改；旧脚本只 `scripts/ch10/zhifadui.lua` 末尾补那一行 `teleport`。
- **不碰 git**：各路不 add / commit / stash / checkout，提交由协调者按 `docs/handoff.md` 第 3 节的流程做；不动 `fanren/build*` 目录。

---

## 实现记录

**引擎路 · 2026-10-12 · 构建槽 `ch11eng`**

- 改动：`src/core/rules/Crafting.h` +12/−1、`Crafting.cpp` +3/−0、`src/io/RecipeLoader.cpp` +13/−0、`RecipeLoader.h` +3/−2（`git diff --numstat`）；新文件 `tests/Ch11EngineTests.cpp` 817 行、28 条用例（E1 6、E2 5、E3 4、E4 5、E5 8），其中负向 7 处（加载器 5 种坏形状、商店 `entries: 3`、`sellRate == buyRate`），`GTEST_SKIP` 守卫 6 处（E1 第 6 条配方未带 `guaranteed`；E3 第 4 条 role / battle 文件；E4 第 4 条三件物品、第 5 条四张店；E5 真图 `ch11_dongfu.tmj`）——交付时内容路都已落地，实跑 **0 处跳过**。本文只填 1.6、2.3、3.3、4.3、5.3 与本节。`tests/Ch07*` 未动（8.3 那一格没红：`Ch07SliceTests` 只钉两张丹方的数值，不扫「每张 `difficulty > 0`」）。
- 门禁与构建：`build_logged.bat ch11eng` 在 `tools/validate.py` 这一步就停（`build-ch11eng.log`，EXITCODE=1，36 错 0 警告——全是内容路的 `data/pathactions/ch11.json` 旗标未登记、`data/text/ch11_battle.json` 与 `battles.json` 的 `ch11.battle.ningcuidao_youyao.intro` 重复〔8.2 协调者删占位〕、`ch11_dongfu.tmj` 规则 30 的 `jiedan_linju` 两个 NPC；引擎路白名单内零条）。编译与测试于是在 PowerShell 里按 `build.bat` 同样的步骤分开跑：`cmake -S . -B build-ch11eng -G Ninja -DCMAKE_BUILD_TYPE=Release` + `cmake --build build-ch11eng --parallel` → `build-ch11eng-compile.log` EXITCODE=0，**0 警告**（/W4）；`ctest --test-dir build-ch11eng` → `build-ch11eng-ctest.log`（见下）。
- `tools/validate_selftest.py`：**SELFTEST_FAIL**，RC=1，「363 条探针漏抓 26 条」——第一条就是「未改动的副本 0 错误」：自检先拿一份原样副本跑 `validate.py`，期望 0 错，而眼下仓库本身就是上面那 36 错（`ch11_dongfu.tmj` 规则 30 的 `jiedan_linju` 等），于是所有「全量校验应净」「改坏后应只多出这一条」类探针连锁 FAIL。自检不读 `src/` 与 `tests/`，26 条里零条与引擎路文件相关；内容路把 validate 清零后它自然回绿（协调者 8.2 挪探针那一段另算）。
- `Ch11EngineTests` 本身：在仓库根直接跑 `--gtest_filter=Ch11*` 时 14 条红，根因只有一个——`io::loadGameData` 因上面那条重复文案 key 失败，`Application::init` 起不来；去掉那一条重复（临时根：`data/` 副本 + 其余目录 junction，一个字节不动仓库）后 **28 / 28 绿**。
- 全量（`build-ch11eng-ctest.log`，仓库根，22:10–22:2x）：1808 条，**1723 绿 / 85 红**，EXITCODE=8；**`Ch11EngineTests` 28 / 28 绿**（协调者 22:11 已删 `battles.json` 的重复占位，仓库根也绿了）。85 红里 70 条是瞬态：`BattleMenuTest` ×18、`BattleKitTest` ×13、`BattleRewardGrant` ×9、`NpcVisibilityTest` ×8、`Ch02ExploitTest` ×7、`Ch02BottleTest` ×5 等，全落在 #117–#182 那一段（22:10–22:11，`data/flags.json` / `data/text/battles.json` 正被协调者改写），`***Failed 0.09 sec` 无任何输出；`ctest --rerun-failed`（`build-ch11eng-ctest-rerun.log`）把它们 **70 条全跑绿**，剩 **15 条稳定红**，没有一条涉及 Crafting / Recipe / Alchemy / Shop / Ch07 / Ch11：
  - 8.3 测试路那几格：`Ch10Walkthrough` ×2、`Ch10Acceptance.BothActualInheritedPurses…`、`Ch10Slice.TheOtherLateChoice…`（`zhifadui.lua` 末尾 `teleport("ch11_fangshi")` 之后 `ch10-end-*.sav` 待重生成）；`ChapterCards` ×2、`ChapterTableData`（`chapters.json` 已登第 11 章，用例仍钉「第 10 章最后、接未完待续」）；`ObjectiveRoute` ×3（`kScriptTransfers` 待登五条三元行与回城五元行）；`NpcPresenceShipped`（`fan_furen` 同时站 `ch11_fangshi` / `ch11_yinsha_dao`，`kOffChainFlags` / `kLastChapter` 待改）。
  - 内容路 / 门禁同一批：`PathActionData` ×2（`data/pathactions/ch11.json` 的 `ch11.path.*` 14 条旗标未登记 → `real.ok == false`，与 validate 那 36 错同源）；`MapLinkWalk`（`ch11_fangshi` 的 `portal_chaguan` / `portal_chaguan_chu` 按住方向键会弹回）；`MapArt.TheShippedMapsUseGid3…`（第 11 章某张图 building 层压在 gid 3 上）。
- 全量（临时根，`data/` 副本只去掉那一条重复 key，其余目录 junction）：1803 跑 / 1767 绿 / 36 红——15 条同上，另 21 条是临时根的伪红（`IoTests` / `RealmCapTests` 找仓库根要 `CMakeLists.txt` / `src/`，junction 根没放），**不是产品问题**。

