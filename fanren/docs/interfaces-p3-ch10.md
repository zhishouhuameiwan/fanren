# P3 第 10 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-09-29 · 依据 `docs/ch10-design.md` 第 10 节（引擎前置）、第 7 节（系统）、第 8 节（编成）、第 11 节（旗标）、第 12 节（禁词）。
> 本批引擎路做 **E1**（重修加速）、**E2**（修炼面板的「旧路」一行与 StoryCap 两句）；2026-10-09 按用户新增授权补 **E3**（普通全体伤害法术，补齐设计 E4 两门法术所需的真实机制）。本文 E3 不替代施工图 E3 的规则 25 登记。
> 门禁：E1、E2 不改 `tools/`；新增 E3 的 `target` 形状及其正反自检归引擎路。规则 25 的 `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER`、C9 来源修复及旧 selftest 探针仍归协调者/门禁路（第 5.2 节），引擎路不覆盖这些片段。
> 可选 O1（Lua 只读 `realm.former()`）、O2（「海面」战斗背景）、O3（打坐设施效率低于 100）、O4（丹药加修为 / 服丹打坐）、O5（目标提示按旗标换版）、O6（分身双控）**都不做**（施工图 10.1）。
> **本章在第 9 章集成之后开工**：第 7 章的 E1（配方 `requireFlag`）、E3（瓶子容量随境界）、E4，第 8 章的 E1（打坐设施按地点给效率），第 9 章的 E1（`formerRealm`、`realm.demote`）、E2（存档 v9）、E3 本章都当成已有的用。

---

## 0. 两件事，各自现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 跌落之后往回修的那一段，打坐快、冲关不卡（原著 ch362：被吸走的只是真元，路他走过一遍；ch368 一月恢复两层；ch374 一年多修到九层）| `meditationEffectiveness(const core::GameState&)`（`src/game/CultivationScene.cpp`）恒返 100——它是「功法、灵根」那一路的合成点；第 8 章 E1 的地点效率乘在它外面（`meditationEffectiveness(state) × sitePercent / 100`，`sitePercent` 先夹到 `[0, kMaxEffectiveness]`）。`breakthroughPillBonus(const core::GameState&)` 恒 0，面板两处冲关（`buildMainItems` 显示把握、`breakthrough` 真按）都把它交给规则层。`GameState::formerRealm`（第 9 章 E1）至今只写不读（第 9 章契约第 3 节把「重修加速」指到了这个合成点）。`rules::meditate` 把效率夹到 `kMaxEffectiveness = 1000`；`breakthroughChance` 把 `pillBonus` 夹到 ±100、结果夹到 [5, 95] |
| E2 | 面板上看得见「旧路」；重修时到了剧情上限，不说「瓶颈」 | `CultivationLexicon`（`src/game/CultivationScene.h`）的 `pushAtStoryCap` / `atStoryCap` 修仙那一套写着「卡在瓶颈上」；第 8 章 E1 追加了 `siteBonusPrefix` / `siteBonusSuffix` 与状态栏那一行；`cultivationPanelStrings(stage)` 两个阶段的串都进 `tests/PanelTests.cpp` 的禁词扫描；`tests/RealmCapTests.cpp` 另有两条专盯 StoryCap 那两句的措辞 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。本批：规则层补纯函数与两个常量；加载器不涉（`formerRealm` 第 9 章 E2 已落盘）；游戏层补面板上的两处入口与用词。

**查过、本章不需要新做的**（`docs/ch10-design.md` 10.1 末尾那张表的依据）：`hero_absent` 编成（第 5 章，`docs/interfaces-p3-ch05.md` 2.1：不建韩立、不带队伍、不登记背包）；`party.add` / `party.remove` 看返回值；`realm.advance` 跨大境界（`RealmAdvance` 直接写目标，炼气九层一步到筑基初期）；`realm.cap` 只升不降；`field.unlock` / `field.planted`；设施 `require_flag`；配方 `requireFlag`；瓶子容量「只补不削」与凝液间隔按境界（`bottleChargeDays`）；布阵模板与位掩码（第 8 章契约第 4 节）；多波次、蓄势、破绽；`defeat_is_fatal false` 时 `battle()` 返回 `won=false`；NPC `visible_flag` / `hidden_flag`；路径行动的 `when` / `until` / `reveal`；`today()`；`bgm(id)` / `bgm("map")`；章节卡按完成旗标自动排、最后一章之后接「未完待续」；横版战斗我方四人以上时的站位（`BattleFx.cpp` 已收紧）。

---

## 1. E1 重修加速

### 1.1 一句话口径

**`rules::reclimbing(realm, formerRealm)` 为真——当前境界的编号小于跌落之前的境界、两者都是修仙境界——叫「重修」：打坐效率 800（×8），冲关加成 ＋100（修为够时夹完恒 95）。追平或越过 `formerRealm` 的那一刻起一切照旧。只看这两个字段：不看章节、不看旗标、不看地点。**

- **为什么是 ×8（算账）**：炼气五层到九层要 `cultivationNeeded(5)` 到 `(8)` 四关之和——602 点（读表，不抄数）；炼气期每百日基准 12、资质 50，×8 时每日约 0.96 点，约 630 日，对得上原著 ch374「一年多」。×8 与第 8 章灵泉的 125 相乘恰是 `kMaxEffectiveness`（1000），不被夹；×10 与任何高于 100 的地点相乘都会被夹住，面板上写的倍数就与实得对不上。
- **为什么冲关 ＋100**：`breakthroughChance` 把 `pillBonus` 夹在 ±100、结果夹在 [5, 95]——修为够时恒 95。剩下那 5% 是规则层自己留的失手（`kBreakthroughMaxChance` 的注释），不去动它。
- **为什么追平就停**：22 以上是新路，瓶颈照旧（ch375 起他停在筑基顶峰，ch386 第一次冲丹失败）。
- 本作眼下触发它的只有一段：第 9 章跌落（ch361）之后，到第 10 章节点 19（`realm.advance` 到筑基后期 23，越过 22）为止。以后若再有跌落（`realm.demote`），同一口径自动生效；口径不合用，先改本文。
- **脚本不读它**（O1 不做）：抬境界、抬上限照旧是脚本的事，快不快是引擎的事（施工图第 17 节第 44 条）。

### 1.2 规则层（`src/core/rules/Cultivation.h` / `Cultivation.cpp`）

```cpp
// 重修（第 10 章契约 docs/interfaces-p3-ch10.md 1.2）：跌落之后往回修、还没追平跌落之前的境界。
// 原著 ch362：被吸走的是真元，路他走过一遍，再修没有瓶颈可卡。
// 任一方不合法、former 是凡人（从没跌过）、current 是凡人 → false；current 编号小于 former → true。
[[nodiscard]] bool reclimbing(Realm current, Realm former) noexcept;

// 重修时的打坐效率（百分数，乘在地点效率之内）与冲关加成（加进 pillBonus 的百分点）。
// 800：炼气五层到九层 602 点修为，资质 50 约 630 日——原著 ch374「一年多」；
//      与第 8 章灵泉的 125 相乘恰是 kMaxEffectiveness，不被夹。
// 100：breakthroughChance 把 pillBonus 夹在 ±100、结果夹在 [5, 95]——修为够时恒 95。
inline constexpr int kReclimbEffectivenessPercent = 800;
inline constexpr int kReclimbBreakthroughBonus = 100;
static_assert(kReclimbEffectivenessPercent <= kMaxEffectiveness);
```

- 判定次序：`current` 或 `former` 不合法 → false；`former` 是凡人 → false；`current` 是凡人 → false；`toValue(current) < toValue(former)` → true；其余 → false。
- `meditate`、`breakthroughChance`、`tryBreakthrough`、`kMaxEffectiveness` **不改**。

### 1.3 游戏层（`src/game/CultivationScene.h` / `CultivationScene.cpp`）

- `meditationEffectiveness(const core::GameState& state)`：**不改签名**；`rules::reclimbing(state.realm, state.formerRealm)` 为真返回 `rules::kReclimbEffectivenessPercent`，否则 100。头文件与实现里那两段注释改成：「功法、灵根将来也在这里合成（乘法）；重修是第一个接进来的（第 10 章契约 1.3）；地点效率是另一路，乘在外面（第 8 章 E1）」。
- 新函数，声明紧挨 `breakthroughPillBonus`：

```cpp
// 重修时冲关的加成（百分点；第 10 章契约 docs/interfaces-p3-ch10.md 1.3）。不是重修就是 0。
// 与 breakthroughPillBonus 分开具名：那一个留给丹药，两样各有来路，测试各钉各的。
[[nodiscard]] int reclimbBreakthroughBonus(const core::GameState& state);
```

- **面板两处冲关用同一个和**：`buildMainItems` 的 `None` 分支交给 `breakthroughChance` 的、`breakthrough(app)` 交给 `tryBreakthrough` 的，都是 `breakthroughPillBonus(state) + reclimbBreakthroughBonus(state)`——建议在本文件里抽一个小函数，两处都调它（G-14 的教训：显示与真按各算一套，就会出现「面板说能按、按下去被回绝」）。
- **地点效率照旧乘在外面**：`bankMeditation(…, sitePercent)` 交给 `settleMeditation` 的效率 = `meditationEffectiveness(state) × sitePercent / 100`；乘积超过 `kMaxEffectiveness` 的由 `rules::meditate` 夹住，游戏层**不另夹**。
- **日课**（`settleDailyPractice`，只在凡人阶段有）走同一个 `meditationEffectiveness`：凡人阶段 `formerRealm` 恒是凡人、`reclimbing` 恒假，结果一个点不变。
- **不动**：`Application`（没有新命令）、`SaveFile`（`formerRealm` 第 9 章已落盘；本章不加存档字段，仍是 v9）、`scripts/common/api.lua`（O1 不做）、`rules::` 其余函数。

### 1.4 判据（`tests/Ch10EngineTests.cpp`，引擎路写）

1. **规则**：`reclimbing` 表驱动——(3, 22) true；(21, 22) true；(13, 21) true；(22, 22) false；(23, 22) false；(3, 凡人) false；(凡人, 22) false；(编号 15, 22) false；(3, 编号 15) false。
2. **效率**：同一份修仙阶段、炼气五层、上限九层、资质 50、修为 0 的状态，`formerRealm` 22 与凡人各一份，同一颗种子各 `bankMeditation(state, 60, seed)`（三参）——设后者进账 x（先验 x > 0）：前者进账落在 `[8x, 8x + 8)` 之内（每日收益对效率线性，只差取整）。60 日两份都碰不到五层那一关的门槛。
3. **追平与越过**：同一份状态把 `realm`、上限都改成 22、`formerRealm` 22 → 与 `formerRealm` 凡人那一份逐点相同；`realm` 23、`formerRealm` 22 → 同样逐点相同。
4. **地点效率乘在外面、夹在规则层**：重修存档 `bankMeditation(state, 60, seed, 125)` 与同一份非重修存档 `bankMeditation(state, 60, seed, 1000)` 逐点相同（都是 1000）；重修存档 `sitePercent 300`（800 × 3 ＝ 2400，被夹到 1000）与之也逐点相同。
5. **显示把握**（真 `CultivationScene`，headless `Application`）：重修、修为恰够、资质 0 的炼气五层存档，主菜单「冲关」那一行的 detail 写的是 95；同一份把 `formerRealm` 改成凡人 → 写的是 `rules::breakthroughChance(realm, cultivation, 0, 0)` 那个数（先验：它小于 95，否则这条比的是两个 95）。
6. **真按**：上一条那份重修存档与它的非重修副本（只差 `formerRealm`；面板的冲关种子取自日子、修为、境界，两份相同），逐日改 `day`、各按一次冲关（每次先复原状态）：**不存在**「非重修成、重修败」的日子；头 200 日里**至少有一日**「重修成、非重修败」（先验：搜得到，否则换资质）。
7. **数从常量来**：重修时 `meditationEffectiveness` 恰等于 `rules::kReclimbEffectivenessPercent`、`reclimbBreakthroughBonus` 恰等于 `rules::kReclimbBreakthroughBonus`；非重修时分别是 100 与 0。
8. **负向自检**（改坏 → 构建 → 确实红 → 还原）：删掉 `meditationEffectiveness` 里重修那一支 → 第 2 条红；`reclimbing` 的 `<` 写成 `<=` → 第 3 条红；只在 `buildMainItems` 加、`breakthrough` 不加 → 第 6 条红；`reclimbing` 放行凡人 `former` → 第 1 条红。

### 1.5 实现记录（引擎路交付时填）

- 2026-10-09，独立 `wt10/fanren`，基线 `f077537226890eaa0df0ecfd1c50229436105e2a`；用户已批准本章推荐并指定构建槽 `resume10eng`。第 9 章依赖由协调者同步，本批不自行补字段。
- `src/core/rules/Cultivation.{h,cpp}` 只追加 `reclimbing(Realm current, Realm former) noexcept`、`kReclimbEffectivenessPercent = 800`、`kReclimbBreakthroughBonus = 100` 与效率上限的 `static_assert`。任一编号非法或任一方为凡人都返回 false；其余严格按 `toValue(current) < toValue(former)`。既有打坐、冲关、上限规则未改。
- `src/game/CultivationScene.{h,cpp}` 已接入效率与具名重修冲关加成；两处冲关共用文件内的 `totalBreakthroughBonus`，地点乘法及规则层封顶照旧。协调者同步第 7 章终版与第 9 章已复验依赖后，已用真实 `GameState::formerRealm` 编译并通过本章全部 15 条（含此前未执行的 10 条游戏层/面板用例）。
- `tests/Ch10EngineTests.cpp` 共 15 条：5 条纯规则、10 条游戏层/面板。纯规则用 `FANREN_CH10_RULES_ONLY` 独立编译；正常 CMake 测试目标不定义该宏，全部 15 条会纳入。RED 为接口未实现时 MSVC 编译退出 2；实现后的纯规则 5/5 通过；与既有 `CultivationTests.cpp` 合并回归 85/85 通过，编译零警告。日志及 XML 均在 `build-resume10eng/`。
- 已实际执行 5 个规则负向改坏：`<` 改为 `<=`、放行凡人 former、移除编号合法性检查、效率 800 改为 100、冲关加成 100 改为 0。各次均编译成功、测试退出 1（每次 2 条红），还原后 5/5 通过；最终规则回归退出 0。
- 60 日未到五层门槛的先验由游戏层用例固定种子 1 钉住；多种子纯规则用例只检查线性比例与封顶，因为额外顿悟可能使其他种子的 60 日收益超过门槛。倍数仍严格为已拍板的 ×8。
- 2026-10-09 依赖同步后的联测负向：游戏层效率分支改为 100（3 条红）、`<` 改成 `<=`（6 条红，含追平收益与真实追平后停止）、真按漏重修加成（1 条红，真实 200 日比较）、显示漏重修加成（1 条红）、放行凡人 former（10 条红）。每类均真实编译退出 0、测试退出 1，无编译警告。
- 联测使用过滤器 `Ch10*:RealmCapWording.*:PanelWording.*`，每次实际 22 条；所有改坏还原后 22/22 通过、退出 0，六个本人源码/测试文件的 SHA256 与变异前相同。各阶段日志及 XML 在 `build-resume10eng/joint-negative-*`、`joint-restored*`；当前源码的 15 个本章测试名与 `initial-ch10.xml` 执行名单逐项一致。

---

## 2. E2 修炼面板：「旧路」一行与 StoryCap 两句

### 2.1 一句话口径

**重修时，修炼面板的状态栏多画一行「旧路」（写明倍数）；到了剧情上限，置灰理由与硬按的那句换成不带「瓶颈」的说法。不重修时面板一个字不变。**

- 这一行是三重补偿里「旧路」唯一看得见的地方（施工图第 0 节第 2 条、第 7 节）；倍数从常量来：`kReclimbEffectivenessPercent / 100`。**冲关不另写数**：把握 95 在「冲关」那一行上已经写着。
- 重修时 StoryCap 照样会出现（本章三段：炼气五层上限五层、炼气九层上限九层、筑基初期上限筑基初期）——「卡在瓶颈上」在那里是错话：这条路他走过，卡住他的是剧情（还没安顿、还没服筑基丹、第一转还没练成），不是瓶颈。新两句与 `notReady`、`breakFail*`、`pushCapped`、「尚差几点」仍要说成不同的事（G-14 的规矩）。

### 2.2 用词表（`CultivationLexicon`，只追加四个字段）

追加在末尾（第 8 章 E1 那两个之后），两张表照 G-14 那次按位置初始化：

```cpp
    // 重修（第 10 章契约 docs/interfaces-p3-ch10.md 第 2 节）。追加在末尾，按位置初始化。
    const char* reclimbPrefix;          // 状态栏那一行：前缀 + 倍数 + 后缀
    const char* reclimbSuffix;
    const char* pushAtStoryCapReclimb;  // 重修时到了剧情上限，「冲关」那一行的置灰理由
    const char* atStoryCapReclimb;      // 重修时到了剧情上限还硬按的那句反馈
```

- **修仙那一套**（例；措辞引擎路定）：「旧路重走，修炼快了 」「 倍」；「这条路走过，眼下还不到往上走的时候」；「路是认得的，只是眼下还不到往上走的时候——得等。」**后两句不许含「瓶颈」**，也不许含 `tests/RealmCapTests.cpp` 那张首见表上的词（「筑基」「灵石」「结丹」……——筑基初期那一段也会出这两句）。
- **凡人那一套**实际不会出现（凡人阶段 `formerRealm` 恒是凡人），但 `cultivationPanelStrings(Mortal)` 也扫它：不许含 `tests/PanelTests.cpp` 禁词表里的任何一个。例：「走过的路再走一遍，快了 」「 倍」；「这条路走过，只是眼下还走不上去」；「路是认得的，眼下却走不上去——得等。」
- 四个字段都加进 `cultivationPanelStrings(stage)`。
- **台词里不许出现倍数**（施工图 13 第 13 条）：「8」只在这一行上。

### 2.3 面板（`src/game/CultivationScene.cpp`）

- `renderStatus`：`rules::reclimbing(state.realm, state.formerRealm)` 为真时多画一行 `reclimbPrefix + std::to_string(rules::kReclimbEffectivenessPercent / 100) + reclimbSuffix`，紧挨第 8 章 E1 那一行（在它之后；两行可以同时出现）。不重修不画。
- `buildMainItems` 的 `StoryCap` 分支：重修时 `disabledReason = pushAtStoryCapReclimb`，否则照旧 `pushAtStoryCap`。
- `breakthrough` 的 `StoryCap` 分支：重修时 `feedback_ = atStoryCapReclimb`，否则照旧 `atStoryCap`。
- 其余分支（`SeriesMax`、`NotEnough`、成功、失败、走火入魔）一个字不改。

### 2.4 判据（`tests/Ch10EngineTests.cpp`；两条既有用例只追加）

1. **状态栏那一行**：真 `CultivationScene`（第 8 章 E1 判据 4 的同一个装置）——重修存档（修仙阶段、炼气五层、`formerRealm` 22）的状态栏文字里有那一行、含「8」；同一份改成非重修 → 整页没有 `reclimbPrefix` 那几个字。
2. **StoryCap 两句**：重修存档到了上限（炼气九层、上限九层、修为很多）：「冲关」那一行 `disabledReason == pushAtStoryCapReclimb`、不含「瓶颈」；硬按 → `feedback == atStoryCapReclimb`、修为一点不扣、境界不动。同一份非重修 → 两处照旧是 `pushAtStoryCap` / `atStoryCap`（修仙那一套含「瓶颈」——钉住原句没被顺手改掉）。
3. **用词表**：`cultivationPanelStrings(Mortal)` 与 `(Immortal)` 都含四个新字段；`tests/PanelTests.cpp` 的禁词扫描因此自动覆盖凡人那一套。
4. **既有两条追加**：`tests/RealmCapTests.cpp` 的 `TheBottleneckIsNeverSaidTheSameWayAsBadLuck` 与 `NeitherLineSaysAWordTheLexiconTableHasNotReleased` 把新两句一并扫进循环（只追加被扫的串，判据不改）。
5. **负向自检**：`renderStatus` 不判重修、总画那一行 → 第 1 条第二小条红；两处 StoryCap 分支不判重修 → 第 2 条红。

### 2.5 实现记录（引擎路交付时填）

- `CultivationLexicon` 末尾追加合同四字段；凡人/修仙两张表均填入新文案，四字段均进入 `cultivationPanelStrings`。原有非重修文案未改。
- 状态栏在地点加成行之后调用 `reclimbLine(state)`，与第 8 章 `siteBonusLine` 相同的具名字符串测试方式；重修时倍数从常量除以 100 得到，追平/越过或无重修记录时返回空串。两行可同时出现。
- 两处 StoryCap 分支已按重修状态择新句；新句不含「瓶颈」，其他冲关分支保持原样。`tests/RealmCapTests.cpp` 仅在合同指定的两条措辞用例中追加新句，判据原样；`tests/PanelTests.cpp` 按 5.3 不改，原禁词扫描会覆盖新字段。
- 对应面板状态行、三处 StoryCap、真实同种子冲关与追平、两阶段字段扫描已在依赖同步后全部实际通过；headless 面板调用真实 `onEnter`、`render`、`breakthrough`，状态行采用合同第 8 章同款的渲染共用字符串入口。此证据不等于逐像素截图或真人验收。
- 已执行负向：去掉 `reclimbLine` 重修判断、让状态行无条件出现（2 条红）；StoryCap 菜单固定用原句（1 条红）；StoryCap 硬按反馈固定用原句（1 条红）；四字段从 `cultivationPanelStrings` 漏掉（1 条红）。各次编译退出 0、测试退出 1；还原后 22/22 通过。与 E1 合计九类联测负向全部抓红。

---

## E3. 普通全体伤害法术（2026-10-09 增补，用户批准）

### E3.1 正式数据字段与边界

- 法术 JSON 可选字符串字段 **`"target": "single" | "all"`**；省略等于 `single`。大小写、空白、空串、未知值及任何非字符串均拒绝。`targetAll` 不作为接口。
- 核心模型追加 `enum class MagicTarget { Single, All };` 与 `Magic::target = MagicTarget::Single`，追加在 Magic 数据成员末尾。旧法术无需改写，旧存档无需加字段或迁移。
- `all` 只用于伤人的法术：`effect` 不得是 `reveal` / `stagger`，并且 `power > 0` 或 `poison > 0`。看破原有全场机制及削架势单体机制保持不变，不能借此扩为全体削架势。
- 内容路将 `data/magics/xuelian_ziyan.json`（`magic_xuelian_ziyan`，火）与 `data/magics/ch10_wulang.json`（`magic_ch10_wulang`，婴鲤兽雾浪的小伤法术）写入 **`"target": "all"`**；各门威力、耗法力、五行和学习/角色登记仍由内容路按设计定，本路不改这两个文件。

### E3.2 规则与行动接口

- 复用 `BattleState::strikeTargets`，普通 `MagicTarget::All` 与已有 `charging && chargeAll` 取并集；不会把普通全体伪装成蓄势，不附赠重招倍率，不新增行动种类或 Lua 命令。
- 目标是行动当时所有 `Unit::alive()` 且 `ally != actor.ally` 的单位，按单位下标顺序；死亡、逃走、未上场的下一波和施法者自己/同伴均不受击。当前行动不越过到下一波。
- `Action::targetIndex` 仍按既有 `charge.all` 要求指定一名合法、存活的敌方作为定位目标。全体菜单自动以首名合法敌方定位，结算仍打全体；不存在敌方、错误定位、未习得、境界不足、法力不足或蓄劲不合法时拒绝，任何 HP/MP/BP/库存均不动。
- MP 按 `needMp` **整次施法只扣一次**；BP 按 `Action::boost` **只扣一次**。`boost=hits` 每个目标承受 `1+boost` 发，`boost=power` 每个目标按同一威力倍率结算；破绽、架势、破势、防御、重招倍率、毒及逐目标 Hit/Break/Down 事件都复用现有路径。
- 道具 `castMagic` 同源继承这门法术的目标范围；仍只扣一件物品，不扣 MP、不吃 BP，不另立第二套全体规则。AI 按既有合法施法入口使用 `all`，不改旧法术决策口径。

### E3.3 游戏选择与反馈

- 法术及符箓列表明确显示「敌方全体」；全体法术的择敌页只有一行「敌方全体」和原有返回项。单体法术列表与择敌行为原样。
- 战斗反馈说明「横扫全场」，逐目标说明实际受击者；所有实际目标各产生原有 Hit 事件，游戏播放/血条/死亡反馈沿用事件流。
- 不增加共享 `data/text/ui.json` 登记，不修改内容路地图、脚本、法术、角色或 flags。

### E3.4 精确新增白名单与验证

仅以下 10 个文件可由本路编辑：

- `src/core/model/Types.h`
- `src/core/battle/Battle.h`
- `src/core/battle/BattleAction.cpp`
- `src/io/DataLoader.cpp`
- `src/game/BattleScene.h`
- `src/game/BattleScene.cpp`
- `tests/Ch10MagicEngineTests.cpp`（新增正式引擎测试，不改或伪造章末存档夹具）
- `tools/validate.py`（仅新 `target` 形状及 focused 入口；旧章手段表与 C9 来源修复不动）
- `tools/validate_selftest.py`（仅新 `target` 正反自检及 focused 入口；旧探针不动）
- `docs/interfaces-p3-ch10.md`（本增补、对应范围和实现记录；既有 E1/E2 全绿/Approve 记录保留）

验证只跑 focused：新核心/加载/真实菜单用例及旧 Battle、加载器、菜单、E1/E2 邻接回归；新门禁形状单独正反自检。C8/C9/内容与共享登记尚未齐备，**不得用本次结果声称 data/full 绿色**。必要变异必须编译成功、断言确实抓红、还原后回绿，日志使用自有 `resume10eng` 槽；UTF-8 无 BOM、LF、apply_patch 手工编辑，不提交、不派代理、不删除 vendor Junction。

### E3.5 实现记录

- 正式字段已接通：`MagicTarget::{Single,All}`、追加的 `Magic::target`，加载器严格解析 `target`，普通全体与 `charge.all` 共用 `strikeTargets`；规则也拒绝直接构造的非法 all/effect 组合。MP/BP 位于逐目标循环之外；符箓共用 `resolveMagic`。AI 经原有施法入口出手；法术/物品列表标示全体，择敌页只有一个全体项，反馈及每名目标的 Hit 事件均真实产生。没有改 Lua、存档、内容或共享登记。
- 本次实际编辑仅 E3.4 的十个文件。`tests/Ch10MagicEngineTests.cpp` 新增正式 15 条：旧单体、存活/阵营/波次过滤、敌方 AI、连发/威力/毒/破勢、蓄势全体并集、非法定位/资源/模型、严格加载、单体/全体菜单、真实菜单施法/符箓及两门内容文件的隔离加载与实打。临时单元输入使用 `TempDir`，未生成或伪造 `ch09-end` 等正式存档夹具。
- RED：接口未实现时 `e3-red-build.log` 的 `MagicTarget` / `Magic::target` 编译错误，实际退出 1。首轮运行 `e3-green-test.log` 实际 14/11 通过、3 条红，原日志/XML保留在原路径及 `e3-initial-14-failed*`；原因是本路临时角色 realm 写成中文描述，以及错误假定防御后的行动顺序。修正推动过程后，整数 realm 的中间 15/13 通过、2 条红也保留为 `e3-intermediate-15-failed*`；最终按加载器规范写 `QiRefining3`，没有放宽全体命中或一次扣费的判据。
- 最终自有 `resume10eng` 仅 focused 编译，`e3-final-build.log` 编译退出 0、警告 0；`e3-final-test.log` / `e3-final.xml` 正式 E3 15/15、进程退出 0；`e3-final-adjacent-test.log` / XML 131/131、进程退出 0（旧 Battle、BattleDataLoading、真实菜单及原 E1/E2 全部纳入）。原 E1/E2 六个源码/测试文件 SHA 保持，不借旧 1682 全量结果称 E3 已通过整章验收。
- 九类 C++ 变异均在本人槽实际编译退出 0、测试退出 1：忽略普通全体（7 条红）、友方受击（8）、提前打下一波（1）、逐目标重复扣 MP/BP（8）、漏规则 effect 拒绝（1）、加载器缺省改 all（2）、吞非法 target 值（1）、择敌漏全体项（3）、反馈仍说单体（2）。日志/XML 为 `e3-negative-*`；逐项恢复后九个源码/测试/工具文件 SHA 与变异前相同，再编译并实际回绿。
- 两类门禁变异：吞非法 target 取值 → focused 形状自检 12 条红、退出 1；从正常形状入口漏接 target 检查 → 1 条红、退出 1。还原后 `python tools/validate_selftest.py --magic-targets` 34/34、退出 0（`e3-final-shape-selftest.log`）；`python tools/validate.py --magic-targets` 仅本形状检查退出 0（`e3-final-target-shape.log`），不等于 data/full 绿色。
- 内容作者已自行给 `xuelian_ziyan.json` / `ch10_wulang.json` 接入 `target: all`，本路正式用例读取实际两文件、复制到隔离目录，经真实加载器与真实规则对两个对手结算。本路没有编辑这两文件；整场四仗平衡和完整内容验收仍归内容/测试/协调者。
- **SOURCE_FROZEN（E3）**：源码/正式测试及本节在 focused/变异还原后冻结，精确十文件 SHA256 清单为 `build-resume10eng/e3-source-frozen.json`。独立初轮 review 的 PRELIMINARY 不是最终 Approve；最终冻结 SHA、独立重编/复跑及裁决交主控。
- C9 工具依赖说明：用户新增授权合入 WT9 的 producer→Quest、MEANS9、N 范围探针移 b10、P 坏文件名移 ch99，但最新通知要求等待 only1 >=2/==2 与 until 恒闭修复；本次未把冻结旧9tools作最终同步或全量绿色依据。待其批准修正版后，仅合这些片段并保留 E3 两个工具的新检查/自检，核心冻结不被此依赖阻塞。C8/C9/shared/content的整包门禁由主控/Nash收口后执行。
- 后续独立 review 已最终 **Approve**：正式/独立/邻接合计 125/125、target 34/34；依据 `docs/ch10-magic-engine-review.md`，七个 C++ 文件仍钉在 `e3-source-frozen.json`。该 review 的工具 SHA 仅有效于 2026-10-09 18:10:20 +08:00 的 E3 快照；以下 5.2 的工具合并及新 SHA 不在那份工具哈希的覆盖范围内。

---

## 3. 第 11 章接口（本批**不实现**，写成约定）

第 11 章施工图与它的契约照这张表接；要改表里任何一行，先改本文。

| 接口 | 本章留下什么 | 第 11 章怎么用 |
| --- | --- | --- |
| **境界** | 章末 23 / 23（原著「筑基顶峰」「假丹」，数据上是筑基后期）；`formerRealm` 22（已越过，重修不再生效）| 结丹怎么上（剧情 `realm.advance` 还是面板冲关，大纲 4.3）由第 11 章定；E1 对它不起作用 |
| **结丹之物** | 降尘丹 5、雪灵水 2（本章不扣，施工图 16.2 第 9 条）；天火液 0、血凝五行丹 0 | 结丹时服不服、要不要接进冲关加成（`breakthroughPillBonus` 是现成的合成点），由第 11 章定 |
| **分身** | `qu_hun_shadan`（结丹初期，在队里）；混元钵、金色小剑符宝、金骷髅头已写进它的法术表（物品从韩立背包扣走了）| 它就是一名队员；数值随章节重填，按那一章的原著 |
| **噬金虫** | `story_shijin_chong`（玉匣里裹成一团，物品）| 要驱使得有灵兽袋（ch387 他自己说的）；什么时候做成手段由后面的章定 |
| **乱星海的丹道** | `story_luanxing_danfang`、`story_dandao_pingjian`、五级妖丹 1、`story_tulijue` | 猎妖的理由（妖丹入药）；配方建不建由第 11 章定 |
| **小瓶与药园** | 容量 6、筑基五日一滴；`field_xiaohuan` 走不回去 | 新的药园另起 field id |
| **开局** | 章末落点：`ch10_xiaohuan` 码头外海边那一格；`ch10.done` 已置；第 10 章是 `data/chapters.json` 的最后一章，「未完待续」自动接 | 照第 9 → 10 的先例二选一：patch `scripts/ch10/zhifadui.lua` 末尾补一行 `teleport`，或在那一格旁挂 `guard_flag = ch10.done` 的开局挂点；`data/chapters.json` 加第 11 章之后，「未完待续」自动挪到第 11 章之后 |
| **语言** | `ch10.yuyan == 2`、`ch10.shizi == 1` | 不再分两版 |
| **规则 25** | `MEANS_LAST_CHAPTER = 10`；`CHAPTER_MEANS[10]` 的「木」取祭青蛟旗 | 按那一章的实际另登；青元剑芒此时已能用（筑基后期不低于筑基初期）|
| **selftest 探针** | 指到第 11 章（`siji_haishou` ＋ `b11_ningcuidao_youyao`）| 第 11 章写到那张编成时，探针再往后挪（第 12 章某个只在一张编成里出现的 role），措辞随之改 |
| **灵石** | 章末低阶 ≈ 起点 ＋ 34（未卖药）、中阶 ≥ 97（估）| 从 `ch10-end` fixture 读，不从施工图抄 |
| **通缉** | 不设任何状态字段；罪名只在执法修士的台词里 | 要不要做成机制由第 11 章定 |
| **Lua 只读** | 不加 `realm.former()` | 第 11 章脚本若要读 `formerRealm`，按 `realm.level` 的写法加 `__host.realm_former` 与 `realm.former()` |

---

## 4. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让几路对同一件事只有一种写法：

- **境界**：`realm.advance` 恰 3 处（`scripts/ch10/muwu.lua` 五层、`zhuji.lua` 筑基初期、`sanzhuan.lua` 筑基后期）、`realm.cap` 恰 1 处（`lifu.lua` 九层），都写 `local ok, code = …`；`ok` 为假时 `talk` 一句占位、`return`（脚本写错了；通关测试会红——**不许静默吞掉**）。`realm.demote` 本章 0 处。**脚本不读 `formerRealm`**。
- **门闸两道**：节点 17 `realm.level() >= 9`；节点 19 `item.count("pill_zhenyuan_dan") >= 2`（兜底见施工图 3.2）。不够都在置旗标之前 `return`。
- **语言两版**：key `ch10.<场景>.<用途>.garbled` / `.clear`；脚本 `if flag.get("ch10.yuyan") >= 1 then … else … end` 择一；`ch10.yuyan` 只由 `xuehua.lua`（置 1）与 `muwu.lua`（置 2）写。
- **学话**：`ch10.xuehua` 位 1、2、4；数位决定下一课；第三课末置 `ch10.yuyan = 1`。
- **分身**：`party.add` / `party.remove` 看返回值；`qu_hun` 本章 0 处 `party.*`。
- **二十余年**：`lifu.lua` 先 `advance_days(1)`、再 `flag.set("ch10.kaifu_ri", today())`；`sanzhuan.lua` 读它补足到开府起 7560 日（至少 360）。
- **布阵**：第 8 章契约第 4 节那个模板函数；两处（小寰山、礁石）。
- **扣东西**：「有则扣」「有几块扣几块」，一律看返回值；每一处都有走得通的一支。
- **数据**：`data/roles/qu_hun_huashen.json` 就地重写（`realm` 筑基后期、兵刃剑、`magics` 含 `magic_xuelian_guangzhu`、note 改写）；`daoyu_husui.json`、`b10_gujia_bidou.json`、`b10_liuliandian_weisha.json` 就地改写；`qu_hun.json`、`wu_chou.json`、`kuilei_shou.json`、`sanji_haishou.json`、`siji_haishou.json` 一个字不动；`data/recipes/alchemy/zhenyuan_dan.json` 新建、`requireFlag = "ch09.xingchen"`。
- **四张必打**：`can_escape false`、`rewards.spirit_stones 0`、`drops` 空；② ④ `hero_absent true`；① `defeat_is_fatal false`，②③④ `true`。
- **teleport**：十六条都写在施工图 3.1 末尾（含第 9 章 `qidong.lua` 补的那一条），测试路登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`。
- **章末**：`zhifadui.lua` 最后置 `ch10.done`，不 `ending()`。
- **静室**：`facility_jingshi` 不写 `effectiveness`（＝100）。

---

## 5. 门禁与既有测试的依赖

### 5.1 引擎路

E1、E2 不改 `tools/`。新增 E3 的法术 `target` 形状归引擎路（E3.4），门禁与 selftest 提供 focused 检查；共享内容不齐时只报告该新形状的证据，不报全量 data 绿色。

### 5.2 协调者：集成时落

- **规则 25 的表**（`tools/validate.py` 的 `CHAPTER_MEANS` 那一段）：`CHAPTER_MEANS[10]` = 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_ji_qingjiao`，ch07——第 9 章契约第 3 节：炼气期的仗里不许把青元剑芒登成必有的「木」）、暗器（`give` `weapon_wuming_sixian`，ch07）；`BATTLE_EXTRA_MEANS`：`b10_liuliandian_weisha` 剑（`qu_hun_huashen` 的兵刃）、火（化身的 `magic_xuelian_guangzhu`、严道友的 `magic_ch10_chunyang`）、水（冯三娘 `magic_ch10_shuiqi`）、木（青算子 `magic_ch10_mufu`），`b10_zhifadui` 剑、火（`magic_xuelian_guangzhu`、`magic_xuelian_ziyan`）、金（`magic_ji_jinjian`）——`hero_absent` 的场按编成里写死的友军算（第 5 章夺帮先例），不继承本尊通表；友军缺省拳照旧计入。`role_magic` / `role_weapon` 来路的第四格是实际友军 role id，须核编成友军、角色法术/兵刃、五行及角色境界能施展；`MEANS_LAST_CHAPTER = 10`。
- **selftest 探针**「第 N 章的敌人不在规则 25 的范围 → 不报」：改指第 11 章——`data/roles/siji_haishou.json` ＋ `b11_ningcuidao_youyao`（`siji_haishou` 只出现在这一张编成里，2026-09-29 核过全仓），措辞改成「第 11 章」。
- **删** `data/text/battles.json` 里的 `ch10.battle.gujia_bidou.intro` 与 `ch10.battle.liuliandian_weisha.intro`：**与内容路的 `data/text/ch10_battle.json` 同一批合进来**（先删后合，免得同一个 key 登记两处）。
- `tests/LexiconTests.cpp` 表一补施工图 12.1 的 93 个词（第 9 章集成之后；补之前按落地的第 8、9 章文案再扫一遍）。
- `data/flags.json`：主线旗标（施工图第 11 节）在内容路派工之前登记，`ch10.kaifu_ri` 的说明写明「整数日期，不是开关」；`ch10.path.*` 等 `data/pathactions/ch10.json` 落地后登记。
- `data/chapters.json` 加第 10 章（`done_flag = ch10.done`）；`data/text/ui.json` 加 `ui.chapter.10.numeral` / `.title`（「第十章」「落海」）。
- `docs/tech-debt.md`：「第 10 章建 `recipe_zhenyuan_dan`」那一笔销账（第 9 章 16.2 第 7 条登的）。
- `docs/handoff.md` 第 4 节那张表：`meditationEffectiveness` 恒 100、`breakthroughPillBonus` 恒 0 两行改成「重修加速已接通（第 10 章 E1）；丹药的冲关加成仍恒 0」。
- 回写大纲与 lore（施工图 16.1 第 1 条）。

#### 5.2 实现记录：2026-10-09 Toolsdelivery

- 主控明确授权本路仅改 WT10 `tools/validate.py`、`tools/validate_selftest.py` 与本文，同批接 C9 批准来源/窗口/自检及本章手段表、范围探针；不改内容、Lexicon、正式 C++ 测试或构建脚本。Nash 已确认 scripts/magic/role/battle/api/mapgen/artgen 输入冻结，本轮没有并发复制或全量门禁。
- WT9 批准输入 SHA256 实际核验匹配：`validate.py` = `4AFE03BC388E42ADEEB3D36E8EE7B46063773713FCC120B0717A8C980E74F5E2`；`validate_selftest.py` = `2D6D57C15D7C310910A1DBD8B7E6CB9583899A4577C4EB9E38902682A8A94AB4`。只取相对 main `f6e89e0` 的指定 C9 producer→Quest、`path_window_can_open` / sources、M2/M3、P 坏文件 ch99 等片段，E3 target 形状与 focused 入口保留；十个 C9 相关函数与批准 WT9 逐字一致。WT10 旧工具范围原停在 7，所以先按 `f6e89e0` 的既有第8章表补齐 8，再接批准 9，最终范围到 10。
- 第10章来源现场核验：第9章 `shudong.lua` 跌到炼气三层，第10章不学回青元剑芒；`ruzhen.lua` 只忘青凝镜/乌龙夺/白蛛飞刀。祭青蛟旗需炼气一层且第7章已习得，故本章「木」取它。两场无本尊编成的角色/法术/兵刃按实际数据接入；本尊通表不借给它们。来源判据会拒绝第10章常驻法术改成筑基门槛、友军删法术/删兵刃/超境界或不在编成，未用文案 note 作为来源证据。
- N 范围探针直接落第11章 `siji_haishou` / `b11_ningcuidao_youyao`；两文件、章号、敌方引用与唯一编成均有先验。阴性对照把同一海兽移入第10章并仅怕毒，必须实际报「没有一样是这一场」，不以空过滤结果称通过；所有临时改坏只在副本并逐字节还原。
- focused 结果：`python tools/validate_selftest.py --magic-targets` **34/34，真实退出 0**（`build-resume10eng/toolsdelivery-target34.log`）；当前 `python tools/validate.py --magic-targets` **退出 0**（`toolsdelivery-target-shape.log`）；新增 `python tools/validate_selftest.py --ch10-means` **23/23，真实退出 0**（`toolsdelivery-means-final.log`），含真实来源正例及逐条改坏/还原、范围阴性。首轮 23 条中一条期望漏算友军默认拳的失败保留为 `toolsdelivery-means-initial-23-one-failed.log`，修的是新自检输入期望，没有改数据或放宽删来源反例。
- 新工具 SHA256：`tools/validate.py` = `9ED97A2453D7B85762707768FA5840BD7DCFD10B0A28E8F64DE30ADC3A77CB60`；`tools/validate_selftest.py` = `0CB33BEA1A4FFF507C6316ACA43C22B7AB36DA0297D6DA5CFEBE89D060208FEF`。这两个新 SHA 代表本次批准依赖合并及第10章表接线，不冒称旧 E3 review 的工具 SHA 已覆盖本合并；交付快照见 `build-resume10eng/toolsdelivery-sha.json`。
- 七个已最终批准的 C++ 文件逐项 SHA 与 `e3-source-frozen.json` 一致；本次未编译/运行 C++，未重复整章构建或完整 selftest。WT9 的 265/265 与 36130 窗口零差异是其独立批准证据，本路未重新跑它们；第10章完整 gates、真实起点重生与正式路由主控/Maxwell继续。

#### 5.2 修复记录：友军法术支付来源 HIGH

- `docs/ch10-gate-review.md` 独立裁决 RequestChanges 的唯一 HIGH 已按用户批准范围修复，是否关闭交 Carson 定向复验；本路未修改独立报告、content、七个 C++ 或正式测试。本次仅两工具与本文，既有批准 C9 producer/窗口/M2/M3 及 E3 target 逻辑保留。
- 已核真实运行时：`BattleScene::unitFromRole` 将友军 `mp=maxMp=role.maxMp`，`RoleTemplate::maxMp` 缺省 0；`Magic::needMp` 缺省 5，`BattleState::checkCast` 在 `u.mp < magic.needMp` 时拒绝。`load_break_roles` 现在保留角色 `maxMp`；`role_means_available` 对 `role_magic` 增加 `needMp <= role.maxMp` 的支付闸门，按上述默认值。不能支付的类别同时从来源与该场 `means` 排除，角色兵刃分支不变。
- 不借本尊 MP/库存、不假设投药，不提高正式角色法力或修改敌人破绽；只核该友军以入场满法力至少能施展一次，不声称完整战斗的法力续航。单体/全体法术原扣 MP、境界压制及核心逻辑均未改。免费 `needMp=0` 的伤人法术在零 MP 下仍合法，不把零 MP 一概说成不能施法。
- 新 focused 自检 `python tools/validate_selftest.py --ch10-ally-mp` 使用真实 `main` 和隔离副本，只在副本里改青算子木法成本/自身 maxMp 与婴鲤兽仅怕木。12 组支付边界逐组核来源、means、main（38 条）：实际 16/340、等额 340/340、独立反例 100000/340、341/340、正成本对零 MP、缺省 5/0/可付 5/不可付 4、免费法术与缺省零 MP；每组后原样逐字节还原，不造正式存档夹具。
- 修前 `ally-mp-before.log`：38 条中 21 条 RED，Python 退出 1；独立反例 100000/340 的木仍进 means、真实 main 退出 0，假绿已由本路亲测确认。修后 `ally-mp-after.log`：**38/38，真实退出 0**；该反例木已排除，来源明确报不可施展、敌人木破绽不可打，真实 main 退出 1；等额/免费零 MP 正例 main 退出 0。
- 邻接 focused：E3 target **34/34、退出 0**（`ally-mp-target34.log`）、当前 `--magic-targets` **退出 0**（`ally-mp-target-shape.log`）、原来源/范围 **23/23、退出 0**（`ally-mp-means23.log`）；C9 M2 **23/23**、M3 **22/22**，focused 合计45条失败0、真实退出0（`ally-mp-c9-m2m3.log`）。十个批准 C9 函数源码逐字一致，严判据未改。
- 支付修复后工具 SHA256：`tools/validate.py` = `E6FFC7566E71C0E4986500213A049D72CFAA0725A6927F04295B32CC7C5F5FA3`；`tools/validate_selftest.py` = `3D74C91ABF7B6C9CD24BB839F947C00171D42ABC21705284008B289F0C6B7DCE`。首次 toolsdelivery SHA 及旧 E3 review 工具 SHA 只保留历史时效，不覆盖本次 HIGH 修复。**SOURCE_FROZEN（支付修复）**：focused 已结束，最终三文件 SHA、七 C++/156 内容不变证据及退出码见 `build-resume10eng/ally-mp-source-frozen.json`；未编译 C++、未改内容/独立报告、未运行完整 selftest 或整章构建，待 Carson 复验此项。

### 5.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/PanelTests.cpp` | 凡人阶段禁词扫描；第 65 行一带用 `meditationEffectiveness(state)` 算期望 | 不改：四个新字段进 `cultivationPanelStrings` 后自动覆盖；那几条的存档都不是重修，期望一个点不变 | 引擎路 |
| `tests/RealmCapTests.cpp` | 两条措辞用例只扫 `pushAtStoryCap` / `atStoryCap`；面板用例断言原句 | 两条措辞用例把新两句追加进被扫的串（2.4 第 4 条）；面板用例的存档不是重修，不动 | 引擎路 |
| `tests/Ch04SliceTests.cpp` 第 1905、1913 行 | 断言 StoryCap 原句 | 不动（第 4 章存档不是重修）| — |
| 第 8 章 E1 的灵泉效率用例 | 存档的 `formerRealm` 是凡人 | 不动；若某条用例的存档取自 `ch09-end` 之后（重修），先把 `formerRealm` 置凡人，判据不改 | 引擎路 |
| `tests/Ch09*`（通关）| 终点断言 `mapId == ch08_lingkuang` 且站在传送阵那一格 | 改成 `ch10_gudao` 石室落点；`ch09-end-{first,second}.sav` 按 patch 之后重生成 | 测试路 |
| `tests/ObjectiveTests.cpp` | `kScriptTransfers` | 加本章十六条（含 `qidong.lua` 补的那一条）；`kOffChainDoorKeys` 本章无 | 测试路 |
| `tests/NpcPresenceTests.cpp` | 一个 role 同一时刻只在一张图上；`kLastChapter` | 曲魂空壳（第 9 章钟乳洞 → 孤岛 → 海船 → 港口 → 木屋 → 小寰岛）、两个顾东主、王长青、黑贵、六连殿六人进时间线；`kLastChapter` 改到 10 | 测试路 |
| `tests/PathActionTests.cpp` | 按章的境界上限表 | 补第 10 章：本章条目全是 `realm 0`；「够得着」按跌落之后重算 | 测试路 |
| `tests/SliceTests.cpp`「每一场迁移来的仗都打得出结果」| 表里有 `b10_gujia_bidou` | 表不改；改写之后照跑、要分得出胜负（第 7 章改 `b07` 同样核过）——红了先找内容路改编成 | 测试路 |
| `tests/Ch03SliceTests.cpp` | 钉「`qu_hun_huashen` 存在、入队的不是它」| 不动（本章只改它的数值）| — |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | 加 `ch10-end` 两份（v9，本章不加存档字段）| 测试路 |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | 不动（本章四张 `drops` 空）| — |
| `tests/Ch06SliceTests.cpp` | 读 `MEANS_LAST_CHAPTER ≥ 6` | 不动 | — |

---

## 6. 白名单（四路按文件分；新增 E3 的 validate/selftest 按片段共享，旧登记仍归协调者）

### 6.1 引擎路

- `src/core/rules/Cultivation.h`、`src/core/rules/Cultivation.cpp`（只追加 `reclimbing` 与两个常量）
- `src/game/CultivationScene.h`、`src/game/CultivationScene.cpp`（`meditationEffectiveness` 的实现与注释、`reclimbBreakthroughBonus`、两处冲关的加成、用词表四个字段、`renderStatus` 那一行、两处 StoryCap 分支）
- `tests/Ch10EngineTests.cpp`（新）；`tests/RealmCapTests.cpp`、`tests/PanelTests.cpp`（只按 5.3）
- `docs/interfaces-p3-ch10.md`（只填 1.5、2.5 与末尾「实现记录」）
- 2026-10-09 新增 E3 以 E3.4 的精确十文件为准；本文新增 E3 合同及范围已获用户授权，规则 25 的既有来源片段不归本路。

### 6.2 内容路（另一份派工单，写在 `docs/ch10-design.md` 第 18 节头部；这里只列边界）

- `maps/ch10_{gudao,haichuan,kuixing,xiaohuan,tiandujie,jinhai,haiyuandao}.tmj`（新 7 张）、`maps/tilesets/terrain_ch10_*.tsj`（生成器要的话，新）
- `tools/mapgen/genmaps_ch10.py`（新）、`tools/mapgen/genmaps.py`（只加登记，排在第 5–9 章之后；本章没有 patch 调用）
- `scripts/ch10/**`（新）；`scripts/ch09/qidong.lua`（**只在末尾补一行 `teleport`**，此前一个字不动）
- `data/text/ch10*.json`（新，按前几章的拆法）
- `data/roles/*.json`（新建 41 个；**就地改写** `qu_hun_huashen.json`、`daoyu_husui.json`；其余已有 role 一律不动）
- `data/items/**`（新建 15 件）、`data/magics/*.json`（新：我方 3 门、敌方约 6 门）
- `data/battles/b10_{gujia_bidou,liuliandian_weisha}.json`（就地改写）、`data/battles/b10_{gu_zhanglao,zhifadui}.json`（新）
- `data/recipes/alchemy/zhenyuan_dan.json`（新）
- `data/quests/q10_{gujia,wunian}.json`、`data/pathactions/ch10.json`、`data/objectives/ch10.json`、`data/shops/ch10_tiandujie_pu.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`tools/artgen/sprites_enemies.py`（只加婴鲤兽一只）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加 7 条登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch10-design.md`（**只追加第 18 节「施工偏差」**，平衡路的实测也写在这里）

### 6.3 协调者

- `tools/validate.py`（**只** `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段）、`tools/validate_selftest.py`（只挪探针）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第十章」「落海」）；`data/text/battles.json`（只删两条）
- `tests/LexiconTests.cpp`（表一补词）
- `docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（回写）

### 6.4 测试路（两路都落地、协调者 5.2 之后另派）

- `tests/Ch10*.cpp`（除 `Ch10EngineTests.cpp`、`Ch10MagicEngineTests.cpp`）：通关、挂点、编成、账、切片、语言、验收
- `tests/Ch09*.cpp`（只改通关终点断言那一处）
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`、`tests/SliceTests.cpp`（只在 5.3 那一格红了的时候）
- `tests/ChapterFixture.h`、`tests/fixtures/ch09-end-{first,second}.sav`（重生成）、`tests/fixtures/ch10-end-{first,second}.sav`（新）、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记主线旗标 → 引擎路（E1、E2）∥ 内容路 → 协调者 5.2 与集成 → 测试路（先改第 9 章通关终点、重生成 `ch09-end`，再写第 10 章通关）→ 独立校对。**本章在第 9 章集成之后开工**（施工图第 2 节全是「估」）。
- **跨路依赖只有一处**：内容路不调任何新接口，两路互不阻塞；但**没有 E1，窗口 A 要五千多日**——通关照样走得通（节点 19 至少还拨 360 日，章末日数比 9212 多出五百来日），施工图验收 2 的日数与验收 6 会红。协调者先合引擎路、再合内容路。
- 每路用自己的构建槽：引擎路 `ch10eng`，内容路 `ch10content`，测试路 `ch10test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 几路共用一棵工作树时，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实，不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- 门禁先于编译跑：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`。
- **旧图不 patch**：本章 0 张；`genmaps_ch05.py`–`genmaps_ch09.py` 一个字不改；旧脚本只 `qidong.lua` 补那一行。

---

## 实现记录

- 本次实际编辑 7 个路径：`src/core/rules/Cultivation.h`、`src/core/rules/Cultivation.cpp`、`src/game/CultivationScene.h`、`src/game/CultivationScene.cpp`、`tests/Ch10EngineTests.cpp`、`tests/RealmCapTests.cpp`、本文（仅 1.5、2.5 与本节）。`PanelTests.cpp`、Types、Save、Application、第 9 章引擎及内容均未编辑。
- 已执行：规则 RED/GREEN、5 次规则负向自检及还原、85 条规则回归；四道门禁均退出 0：`validate.py`（`VALIDATE_OK`）、`genmaps.py --check`（`MAPGEN_IN_SYNC`）、`artgen.py --check`（`ARTGEN_IN_SYNC`）、`validate_selftest.py`（220 条，0 条未如期抓住，`SELFTEST_OK`）。
- 依赖已由协调者同步：真实 `formerRealm`、`realm.demote`、v9 存档、菜单境界禁用均在 WT；依赖文件不编辑。1554 个非本人文件 SHA256 一致是并发内容落地前的联测历史证据。最终复核排除内容作者明确拥有的 `data/roles/daoyu_husui.json`、`data/roles/qu_hun_huashen.json`、`docs/ch10-design.md`，其余 1551 个原有文件 SHA256 仍一致；newC10 namespace 的合法新增文件保留，不按旧路径清单判越界。
- 自有 `resume10eng` 槽：首次 fresh 的中文 MSVC include 前缀出现乱码、Ninja 报头依赖 0；设 `MSVC_CONSOLE_OUTPUT_ENCODING=UTF-8` 并在终端使用 `chcp 65001` 后重新 fresh、clean-first 全编译，退出 0、警告 0，`msvc_deps_prefix` 正确。Ninja 的面板与第 10 章测试对象依赖表均已记录 `../src/core/model/Types.h`（`types-dependency-proof.log`），没有用旧类型对象作绿色证据；仓库构建脚本未改。
- `build_logged.bat resume10eng` 实际于 2026-10-09 15:47:09 +08:00 启动、16:00:05 完成，脚本与调用进程均退出 0；四道门禁通过（selftest 220/220），编译零警告，全量 CTest 1682/1682，通过耗时 266.61 秒。全量中第 7 章 88 条、第 8 章 6 条、第 9 章 33 条、本章 15 条、RealmCap 21 条、Panel 114 条均通过；第 7 章判据未改，历史六条旧红在同步终版后的本次全量中没有复现。最终日志 `build-resume10eng.log` 第 3905 行全量汇总、第 3908 行 `EXITCODE=0`。
- 证据归属：此次构建/门禁基于已同步的第 7 章终版、第 9 章引擎依赖与本章 E1/E2；测试集合包含本章 15 条，不包含第 10 章通关/内容验收。内容作者在运行后段已新增 `newC10` 文件（已观察到 `ch10_main.json`、`muwu.lua` 的 mtime 为 15:58:28），本次绿色不作为新增内容整包验收或当前门禁输入未变的证明。后续内容落地后的整包复核归协调者；本人停止引擎验证，不为半批内容修改数据或判据。
- 无 Git 提交/推送，无子代理；主树/其他 worktree 未编辑，vendor Junction 原样保留。源码与记录按字节核验 UTF-8 无 BOM、LF。
