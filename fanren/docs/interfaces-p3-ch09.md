# P3 第 9 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-09-29 · 依据 `docs/ch09-design.md` 第 10 节（引擎前置）、第 7 节（系统）、第 8 节（编成）、第 11 节（旗标）、第 12 节（禁词）。
> 本批引擎路做 **E1**（境界跌落 `realm.demote`）、**E2**（存档 v9 与读档交叉校验——技术债 G-20）、**E3**（菜单法术页写明「境界不足」）。
> 门禁：本批引擎路**不碰 `tools/`**（没有新的数据字段、没有新的调用形状要门禁查形状）；规则 25 的表与 selftest 探针在集成时由协调者改（第 6.2 节）。
> 可选 O1（「撑过 N 回合」的胜负条件）、O2（队员在场不参战开关）、O3（Lua 只读 `realm.former()`）、O4（重修加速与面板措辞）**都不做**；O4 是第 10 章的事，本文第 3 节把接口写成约定。
> **本章在第 8 章集成之后开工**：第 7 章的 E1（配方 `requireFlag`）、E3（瓶子容量随境界）、E4（`count_aged` / `take_aged`，命令 `TakeItemAged`），第 8 章的 E1（打坐设施按地点给效率）本章都当成已有的用。

---

## 0. 三件事，各自现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 剧情让他从筑基中期跌到炼气三层（ch361）：境界、上限、气血法力、修为一起掉，法术与瓶子不动；记下他跌之前在哪一层，第 10 章「重修加速」要用 | `rules::demote(realm, levels)`（`src/core/rules/Realm.h`）按**级数**逐级回落、下限炼气一层，**零调用方**（只有 `tests/RealmTests.cpp` 四条）；脚本命令 `RealmAdvance` 目标低于当前回 `not_higher`（`src/game/Application.cpp`，注释写着「跌落是第 9 章 `rules::Realm::demote` 的事」）；`RealmCap` 只升不降；`GameState` 没有任何「跌落之前」的记号；`scripts/common/api.lua` 里 `realm.advance` 的注释写着「跌落是 Realm::demote 的事，第 9 章才用得上」；`Realm.h` 里 `liftToFloor` 的注释写着「掉境界要不要跟着削气血是那一章的设计决定」 |
| E2 | 跌落是第二条写入境界与上限的路径——读档要能拒「上限低于境界」；新字段要落盘 | `io/SaveFile.cpp` 的 `fromJson` 对 `realmCap` 只查「是整数、是合法编号」，**不查它不低于 `realm`**（`docs/tech-debt.md` G-20「有意不修」，写明「哪天出现第二条写入境界或上限的路径……在那条路径落地的同一批里补这道校验」）；存档 v8（`kSaveVersion = 8`）|
| E3 | 跌落之后，玩家打开菜单就看得出哪几门法术「会，但使不出来」 | 战斗菜单（`BattleScene::buildMagicItems`）早已用 `refuseOnAnyone` 问 `BattleState::checkCast`，`needRealm` 不够时那一行的理由是「境界不足，施展不了【X】」；**菜单法术页**（`MenuScene` 建 `magicRows` 那一段）每一行只写法力数 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器（本章是存档）→ 游戏层入口。E1 补规则层的判定与游戏层入口、E2 补存档，三段在本批一起落。

**查过、本章不需要新做的**（`docs/ch09-design.md` 10.1 末尾那张表的依据）：战斗里 `needRealm` 不够点不动（`BattleState::checkCast`）；瓶子容量「境界往回拨不削」（第 7 章 E3 三处都是 `max`）；凝液间隔按境界（`bottleChargeDays`）；静养按新上限回（`Application::restFor` 读 `maxHp` / `maxMp`）；攻防按当前境界算（`BattleScene` 开战时 `realmAttack` / `realmDefence`）；状态页按当前境界显示（`MenuScene`）；`field.ripe` / `field.planted`；`item.count`、`take` 看返回值；第 8 章的布阵模板；多波次、蓄势、破绽；NPC `visible_flag` / `hidden_flag`；章节卡按完成旗标自动排。

---

## 1. E1 境界跌落

### 1.1 一句话口径

**`realm.demote(t)` 把境界落到 t（低于当前、不低于炼气一层）：上限一并落到 t；气血法力上限按 t 的基准重算——当前气血夹到新上限、法力清零；修为与余数清零；记下跌落之前的境界（`formerRealm`，取大）。法术、瓶子、队伍、背包、旗标一概不动。**

它是全作**唯一**会压低境界上限的路径：`realm.cap` 仍只升不降（`docs/interfaces-p3-script.md` 7.1 的口径不变），`realm.advance` 仍只升不降。
本作眼下确定用它的只有一处：第 9 章 ch361（筑基中期 → 炼气三层，真元被吸）。三转重元功本身要散三次功（每次到筑基后期散回初期），但原著 ch374 把 ch361 这一次**算作了第一转的散功**（他在中期就意外散了，只是压进去的真元少些），ch375 第一转练成、法力到了筑基顶峰；后两次散功在第 10 章的原著范围（ch364-388）里没有发生——**第 10 章用不上本命令**。日后哪一章演后两转，照本节的口径用它；口径不合用，先改本文。

### 1.2 规则层（`src/core/rules/Realm.h` / `Realm.cpp`）

```cpp
// 剧情跌落到 target 行不行（RealmDemote 命令的判定，第 9 章契约 docs/interfaces-p3-ch09.md 1.2）。
// 与 rules::demote（按级数）分开：剧情入口按目标境界说话，不按级数——
// 「落到炼气三层」比「往下掉十二级」可靠，也不必算筑基与炼气之间那一跳。
enum class DemoteCheck : std::int32_t {
    Ok = 0,
    AlreadyThere,   // target == current：结果已经成立
    NotLower,       // target 高于 current（按编号比）
    NoRealm,        // target 不合法，或是凡人（跌落不回凡人——与 demote 的下限同一口径）；current 不合法也归这里
};
[[nodiscard]] DemoteCheck checkDemote(Realm current, Realm target) noexcept;
```

- 判定次序：`current` 或 `target` 不合法 → `NoRealm`；`target` 是凡人 → `NoRealm`；相等 → `AlreadyThere`；`target` 编号大于 `current` → `NotLower`；其余 → `Ok`。
- `demote(realm, levels)` **不删不改**（四条单测不动）；它上面的注释加一句「剧情入口按目标境界走，见 `checkDemote` 与脚本命令 `RealmDemote`」。
- `liftToFloor` 那段注释里「掉境界要不要跟着削气血是那一章的设计决定」改成：**「第 9 章定了：跌落那一条路径（`RealmDemote`）按新境界的基准重算上限、法力清零；读档这一条仍只补不削。」**

### 1.3 状态（`src/core/model/Types.h`）

- `GameState` **追加在所有数据成员的末尾**（与 `party`、`learnedMagics`、`realmCap`、`knownWeaknesses`、`encounter` 同一个规矩）：

```cpp
    // 跌落之前到过的境界（第 9 章契约 docs/interfaces-p3-ch09.md 1.3）。
    // 缺省凡人＝从没跌过。只由 RealmDemote 写：每次跌落取「原值」与「跌落前的境界」里编号大的那个——
    // 连跌两次，记住的是最高那一层，不是中间那一层。
    // 本章只写不读（存档与测试除外）；第 10 章的「重修加速」读它（契约第 3 节）。
    rules::Realm formerRealm = rules::Realm::Mortal;
```

### 1.4 命令与宿主（`src/script/Command.h`、`src/script/ScriptHost.cpp`）

- 新命令 **`CommandKind::RealmDemote`**：**追加在枚举末尾**（第 7 章 E4 的 `TakeItemAged` 之后；理由见 `Command.h` 那段注释——存档与测试按序号比对）。字段占用表加一行：`RealmDemote   -   -   目标境界编号   -   -`。
- `ScriptHost` 把 `"realm_demote"` 翻成它。

### 1.5 Lua 侧封装（`scripts/common/api.lua`，只追加、只改一处注释）

在 `realm` 表里 `cap` 之后追加：

```lua
    -- 境界跌落（第 9 章契约 docs/interfaces-p3-ch09.md 第 1 节）。目标是境界编号，与 advance 同一套坐标。
    -- 跌到目标之后：上限一并落到目标；气血法力上限按新境界重算（气血夹到新上限、法力清零）；
    -- 修为清零；记下跌落之前的境界。法术、瓶子、队伍、背包一概不动。
    --
    -- 原因码：
    --   "no_realm"   不是合法的境界编号，或是凡人（跌落不回凡人）
    --   "not_lower"  目标高于当前
    -- 目标等于当前：什么也不做，返回 true（结果已经成立，读档重跑同一段不算错）。
    --
    -- **凡调请看返回值**，与 realm.advance 同一条规矩。
    demote = function(value)
        assert(type(value) == "number", "realm.demote() 需要境界编号")
        local result = emit{ kind = "realm_demote", x = value }
        return result.ok == true, result.code or ""
    end,
```

`realm.advance` 注释里「目标低于当前（跌落是 Realm::demote 的事，第 9 章才用得上）」改成「目标低于当前（跌落走 realm.demote）」。

### 1.6 游戏层（`src/game/Application.cpp` 的 `dispatch`）

新分支 `case CommandKind::RealmDemote`：

1. `target = rules::fromValue(command.x)`；按 `rules::checkDemote(state_.realm, target)`：
   - `NoRealm` → `ok = false`，`code = "no_realm"`，什么也不动；
   - `NotLower` → `ok = false`，`code = "not_lower"`，什么也不动；
   - `AlreadyThere` → `ok = true`，什么也不动（**连上限也不动**）；
   - `Ok` → 往下。
2. `formerRealm` = 原值与 `state_.realm` 里编号大的那个（先记，再改境界）。
3. `realm = target`；`realmCap = target`。
4. `maxHp = rules::realmMaxHp(target)`，`hp = min(hp, maxHp)`；`maxMp = rules::realmMaxMp(target)`，**`mp = 0`**。
   - 为什么重算而不是 `liftToFloor`：跌落是真元没了，上限里凡是境界给的那一截都没了；本作至今没有境界之外的上限加成，将来有了再议（改本文）。
   - 为什么法力清零、气血只夹：原著 ch361 被吸走的是法力连同苦修来的真元——法力是真元的流量，气血是身子。
5. `cultivation = 0`，`cultivationRemainder = 0`：修为是冲下一关的积累，被吸走的就是它；不清的话第 10 章剧情一抬上限就连跳数层。
6. **不动**：`learnedMagics`、`bottle`（`owned` / `matureKnown` / `drops` / `lastChargeDay` / `capacity`）、`party`、`bag`、`flags`、`fields`、`knownWeaknesses`、`encounter`、`aptitude`、`day`、`lastPracticeDay`。
7. **不响音效**（跌落的演出归脚本：`fade` 等）。`ok = true`。

`RealmAdvance` 那段注释「只许升不许降。跌落是第 9 章 rules::Realm::demote 的事……」改成指向本分支（行为不变）。

### 1.7 判据（`tests/Ch09EngineTests.cpp`，引擎路写）

1. **规则**：`checkDemote` 表驱动——(22, 3) `Ok`；(3, 3) `AlreadyThere`；(3, 22) `NotLower`；(22, 0) `NoRealm`；(22, 编号 15) `NoRealm`；(编号 15, 3) `NoRealm`；(3, 1) `Ok`。
2. **真脚本**（`tests/ScriptApiTests.cpp` 那套装置，跑**真的** `api.lua`）：一份筑基中期的状态——`hp 380/380`、`mp 250/250`、`cultivation 1500`、`cultivationRemainder 7`、`realmCap 22`、瓶子「容量 6 / 滴数 4 / lastChargeDay d」、12 门法术、队伍里有傀儡兽、背包若干、旗标若干 → `realm.demote(realm.QI_REFINING_3)`：返回 `true, ""`；`realm 3`、`realmCap 3`、`maxHp 60`、`hp 60`、`maxMp 30`、`mp 0`、`cultivation 0`、`cultivationRemainder 0`、`formerRealm 22`；**瓶子五项、法术表、队伍、背包、旗标逐字段等于跌落前**。
3. **带伤**：跌落前 `hp 50` → 跌后 `50`（夹、不升）；`hp 200` → `60`。
4. **回绝**：当前 3 时 `demote(22)` → `false, "not_lower"`，状态逐字节不变；`demote(0)`、`demote(15)` → `false, "no_realm"`，不变；当前 3 时再 `demote(3)` → `true, ""`，不变（上限、`formerRealm` 都不动）。
5. **连跌取大**：22 → 13 → 3：`formerRealm` 是 22（不被第二次写成 13）。
6. **跌后升**：跌到 3 之后 `realm.advance(5)`：`realm 5`、`realmCap 5`、`maxHp 84`（`liftToFloor` 抬的）、`formerRealm` 仍是 22；`rules::breakthroughBlock(5, 足够的修为, 5)` 报 `StoryCap`。
7. **战斗侧**（走 `BattleScene` 真入口）：跌到 3 之后开一场既有编成，我方单位攻防等于 `realmAttack(3)` / `realmDefence(3)`；学会的一门 `needRealm` 筑基初期的法术，法术菜单那一行的理由含「境界不足」。
8. **命令序号**：`RealmDemote` 的序号等于 `TakeItemAged` 的序号 ＋ 1（钉住「追加在末尾」）。
9. **负向自检**（改坏 → 构建 → 确实红 → 还原）：删掉 `realmCap = target` → 第 2 条红；删掉修为清零 → 第 2 条红；`formerRealm` 改成直接覆盖 → 第 5 条红；`mp = 0` 改成夹 → 第 2 条红；`checkDemote` 放行凡人 → 第 1、4 条红。

### 1.8 实现记录（引擎路交付时填）

2026-10-09，在持久 wt09、基线 `f077537` 重新实现（旧20条暂停改动无可靠快照，不沿用旧报告）。

- `checkDemote(current, target)` 按非法/凡人、相等、高于、低于的契约次序判定；按级数的 `demote` 未改。
- `GameState::formerRealm` 追加在数据成员末尾；`RealmDemote` 追加在 `TakeItemAged` 之后。宿主解析 `realm_demote`，真 `api.lua` 提供 `realm.demote(目标)`。
- 游戏入口先取 `max(formerRealm, realm)`，再同步跌落境界和上限；重算气血法力上限、气血夹值、法力及修为/余数清零，不响跌落音效。
- `tests/Ch09EngineTests.cpp` 的 E1 共13条：规则表14组、命令序号与新状态默认值、真Lua全部字段对照、带伤5组、回绝/幂等6组、非法当前境界、凡人相等、连续跌落、已有更高旧境界、跌后升、真战斗入口、Lua类型断言。单独验证全部通过。
- 独立审查P2整改后E1共18条：增加真Lua的大整数反例、其他非法数值11组、合法整数表示3组、原始命令类型4组和实际Cast回绝。宿主只对RealmDemote在读取x的缩窄前检查数值类型、有限值、整数性和int范围，非法目标交给既有no_realm路径；其他旧命令读取保持原样。
- 原完整跌落用例追加保留字段逐项断言：瓶子5项、法术、队伍每成员、背包每堆、旗标、灵田及槽位、破绽、遭遇4项、资质、日历/日课水位、四艺、地图/位置/朝向、章节及两类计时器，不只依赖共用序列化器相等。实际Cast回绝验证全场HP/MP、当前行动者和GameState不变。
- 6项负向自检均实际编译成功、测试退出1：漏降上限、漏清修为、formerRealm直接覆盖、法力仅夹值、放行凡人、漏清余数。对应 `mut01`–`mut06` 日志及XML已记录，每项已还原。

---

## 2. E2 存档 v9 与读档交叉校验（技术债 G-20）

### 2.1 一句话口径

**`formerRealm` 落盘；v8 及更早的档读进来是凡人（从没跌过）；读档时「上限低于境界」与「`formerRealm` 不是合法境界编号」都判失败，不自动修正。**

### 2.2 v8 → v9（`src/io/SaveFile.h` / `SaveFile.cpp`）

- `kSaveVersion = 9`；`SaveFile.h` 的版本注释加第 9 条：**加入跌落之前的境界（`GameState::formerRealm`，第 9 章 E1）。v8 档里没有这一项，读不到就是凡人——旧档＝从没跌过，那时还没有跌落这回事。8→9 迁移是空操作，照规矩登记。**
- `toJson`：`payload["formerRealm"]` = 境界编号（整数）。
- `fromJson`：写了却不是整数、不是合法境界编号 → 读档失败（与 `realm` 同一条口径）；当前版本的档缺这个字段（手拼的档、夹具）→ 凡人。

### 2.3 交叉校验

- `fromJson` 在 `realm` 与 `realmCap` 都读完（含 5→6 迁移补上的上限）之后：**`realmCap` 的编号小于 `realm` → 读档失败**，报错点名两个数（「境界上限（3）低于境界（8）」这一类）。5→6 迁移补的上限是 `legacyRealmCap = max(当前境界, 旗标推出的上限)`，本就不低于境界，不受影响。
- **不做自动修正**：这种档只有手改一条来路（G-20 原文），读不进来比悄悄改掉好查。
- `formerRealm` 与 `realm` **不交叉校验**：重修越过了原来的境界，`formerRealm` 低于 `realm` 是正常的。

### 2.4 判据（`tests/Ch09EngineTests.cpp`，或存档用例所在的文件——只追加）

1. **往返**：`formerRealm 22`、`realm 3`、`realmCap 3` 的状态存了再读，逐字段相等。
2. **老档**：一份 v8 夹具（第 8 章落地后用 `tests/fixtures/ch08-end-first.sav`；之前用任一份 v8 夹具）读进来 `formerRealm` 是凡人，其余字段与读档前相同。
3. **写坏**：手拼一份 v9 档「`realm 8`、`realmCap 3`」→ 读档失败、错误信息含「3」与「8」；「`formerRealm` 写成 15」→ 读档失败；「`formerRealm` 写成 `"22"`（字符串）」→ 读档失败。
4. **既有存档都读得进来**：`tests/fixtures/*.sav` 与 `saves/*.sav` 全部照旧读得进来（先验：至少读到 10 份，否则用例在空转）。
5. **负向自检**：删掉交叉校验那一句 → 第 3 条第一小条红。

### 2.5 实现记录（引擎路交付时填）

- 当前版本9，显式登记8→9空迁移；`formerRealm` 写入整数，缺字段默认凡人，类型/编号非法拒绝。大整数在缩窄前检查范围，避免截断成合法编号。
- `realmCap < realm` 在上限读取/迁移后拒绝，错误点名两个编号；不比较formerRealm与当前境界。
- E2共10条通过：完整跌落状态往返、版本号、v8其他字段保持、真实v8夹具再存、v1–v8迁移、上限倒挂拒绝、formerRealm非法值11组、当前版本缺字段、重修越过旧境界、12份既有存档扫描（先验至少10份）。
- 独立审查P2整改后E2共11条：扫描只对原始payload缺formerRealm的档断言凡人，含字段的档按原始JSON整数验证读回值；使用已有vendor/json.hpp解析原文，不改CMake或vendor。追加合法v9临时档（realm=3、realmCap=3、formerRealm=22）通过同一个扫描检查；原至少10份、12份旧档扫描和v1–v8迁移证据保留。
- 交叉校验移除后1条失败（`mut07-cap-check`）；8→9登记移除后2条失败（`mut08-migration-retry`）。两次有效自检均编译成功、测试退出1，已还原。
- 必要的既有存档测试更新按6.3处理：同步版本断言，并给手设境界的保存夹具显式设合法上限；完整范围说明见末尾实现记录。原行为断言全部保留，没有修改既有存档文件。

---

## 3. 第 10 章接口（本批**不实现**，写成约定）

第 10 章施工图与它的契约照这张表接；要改表里任何一行，先改本文。

| 接口 | 本章留下什么 | 第 10 章怎么用 |
| --- | --- | --- |
| **重修加速的锚** | `GameState::formerRealm`（第 9 章章末 = 筑基中期，编号 22）| `meditationEffectiveness(const GameState&)`（`src/game/CultivationScene.cpp`，注释里说的「功法、灵根」那一路的合成点）在 `realm < formerRealm` 时乘一个系数——数由第 10 章定（原著 ch362 南宫屏说他只伤了真元，再修一遍没有瓶颈可卡；ch374 的节奏：一年多修到炼气九层，连服三颗筑基丹、再一年回到筑基）；第 8 章 E1 的地点效率乘在它外面，不动 |
| **面板措辞** | — | `realm < formerRealm` 时，`CultivationLexicon` 里 StoryCap 那两句的「瓶颈」二字不贴切；换不换、换成什么由第 10 章定（两套词都要进 `cultivationPanelStrings`）|
| **掌天瓶** | 容量 6（第 7 章 E3「只补不削」）| 现成：炼气期的下限是 3，他手里是 6；凝液间隔按炼气是七日 |
| **分身** | 曲魂随行、**不在队里**；那把银剑已交给它（`story_yinhui_jian` 章末为 0）| `qu_hun_huashen`（已有）何时、怎样入队由第 10 章定（ch364 它先拿银剑砍阵角，ch373 起祭炼）|
| **真元丹方** | 物品 `story_zhenyuan_danfang`；旗标 `ch09.xingchen`（1 或 2，非 0 即已得）| 第 10 章建 `data/recipes/alchemy/zhenyuan_dan.json`，`requireFlag = "ch09.xingchen"`（第 7 章 E1 的字段）|
| **三转重元功** | 本章这次跌落（`ch09.dieluo`、`formerRealm`）| 原著 ch374 把 ch361 被吸走的那一次算作第一转的散功——第 10 章**不再跌一次**：他从炼气往上重修、服筑基丹回到筑基，再把第一转练成（ch375 筑基顶峰）。后两转的散功不在 ch364-388 里；哪一章要演，用 `realm.demote`、口径照 1.6 |
| **规则 25 的「木」** | `magic_qingyuan_jianmang` 的 `needRealm` 是筑基初期（内容路改的）| 第 10 章在炼气期的仗里**不许**把青元剑芒登成必有的「木」；木取祭青蛟旗（第 7 章），或按那一章的实际另登 |
| **开局** | 第 9 章章末落点：`ch08_lingkuang` 钟乳洞传送阵心那一格；`ch09.done` 已置 | 第 10 章的开局挂点挂在那一格旁（`guard_flag = ch09.done`），或 patch `scripts/ch09/qidong.lua` 末尾补一行 `teleport` 到第 10 章的落地图——二选一由第 10 章施工图定 |
| **Lua 只读** | 不加 | 第 10 章脚本若要读 `formerRealm`，按 `realm.level` 的写法加 `__host.realm_former` 与 `realm.former()` |

---

## 4. E3 菜单法术页写明「境界不足」

### 4.1 一句话口径

**菜单法术页（`MenuScene` 建 `magicRows` 那一段）：修仙阶段、`needRealm` 高于当前境界的那一行，行尾写「境界不足」、不写法力数；够的照旧写法力数。只改文字，不改能不能选中。**

- 与战斗菜单同一个说法（`BattleState::checkCast` 那一句是「境界不足，施展不了【X】」）；菜单这一页只是看，不写「施展不了」后半句。
- 凡人阶段（`wordingStage == Mortal`）不走这一支：凡人阶段的用词不许出现「境界」二字（`tests/PanelTests.cpp` 的禁词口径）；而凡人阶段他学会的法术 `needRealm` 都是炼气一层、他又已在炼气期，实际不会出现这种行。

### 4.2 判据（`tests/Ch09EngineTests.cpp`）

1. 炼气三层、修仙阶段（`story.xiuxian_known` 置位）的状态，学会 `magic_qingyuan_jianmang`（测试里可就地造一门 `needRealm` 筑基初期的法术）与一门 `needRealm` 炼气一层的法术，**真 `MenuScene`** 翻到法术页：前者那一行 detail 是「境界不足」，后者是法力数。
2. 同一份状态把境界改成筑基中期：两行都写法力数。
3. 凡人阶段的状态（第 4 章夹具）：法术页每一行都是法力数，全页没有「境界」二字。
4. **负向**：删掉那一处判断 → 第 1 条红。

### 4.3 实现记录（引擎路交付时填）

- 只改 `MenuScene::rebuildLists` 的magicRows段：修仙阶段且needRealm高于当前时detail为“境界不足”，其余保留法力/气力数，不改变可选状态。
- 真MenuScene共4条通过：不足/足够两种状态、恰好达到门槛与未知法术行、第4章夹具凡人用词及禁词扫描。测试通过显式实例化取得真实列表，不修改生产头文件或复制菜单逻辑。
- 实现前3条对照里不足境界用例确实红；实现后移除判断再次红（`mut09-menu-retry`，编译成功、测试退出1），已还原。

---

## 5. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口或本批接口，列在这里是为了让两路对同一件事只有一种写法：

- **跌落**：`realm.demote` 全仓恰 1 处——`scripts/ch09/shudong.lua`：`local ok, code = realm.demote(realm.QI_REFINING_3)`；`ok` 为假时 `talk` 一句占位、`return`（脚本写错了；通关测试会红——**不许静默吞掉**）。别处不许削气血、清修为、改上限。
- **布阵**：第 8 章契约第 4 节那个模板函数；四阵位 `once=false` 写 `ch09.zhenqi` 的位（1 / 2 / 4 / 8），阵眼 `== 15` 才往下。
- **清点四块田**：`field.ripe(id)` 与 `field.planted(id)` 按 `field_dongfu`、`field_dongfu_nei`、`field_baiyaoyuan`、`field_baiyaoyuan_jiao` 各问一次；**不用 `field.ripe("")`**（那会把第 2 章七玄门那几块早已走不回去的田也数进来）。
- **扣东西**：「有则扣」「有几块扣几块」「不够就……」三种写法，施工图 3.2 逐处写明，一律看返回值；**每一处都有走得通的一支**。
- **数据**：`data/magics/qingyuan_jianmang.json` **只改 `needRealm`**（`"FoundationEarly"`）；`data/items/pills/dingyan_dan.json` **只改 note**。
- **三张必打**（`b09_tuwei`、`b09_fujia`、`b09_dong_xuaner`）：`can_escape false`、`defeat_is_fatal true`、`rewards.spirit_stones 0`、`drops` 空。
- **teleport**：十六条都写在 `docs/ch09-design.md` 3.1 末尾，测试路登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`。

---

## 6. 门禁与既有测试的依赖

### 6.1 引擎路

本批**不碰 `tools/`**：E1–E3 没有新的数据字段、没有新的脚本调用形状要门禁查（`realm.demote` 的调用次数由施工图验收 5 的扫脚本用例钉）。`validate.py` 与 `validate_selftest.py` 须照旧全绿。

### 6.2 协调者：集成时落

- **规则 25 的表**：`CHAPTER_MEANS[9]` = 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08——本章三场都在跌落之前）、暗器（`give` `weapon_wuming_sixian`，ch07）；`BATTLE_EXTRA_MEANS` 本章不加；`MEANS_LAST_CHAPTER = 9`。
- **selftest 探针**「第 N 章的敌人不在规则 25 的范围 → 不报」：改指第 10 章——`data/roles/daoyu_husui.json` ＋ `b10_gujia_bidou`（`daoyu_husui` 只出现在这一张编成里，改它的破绽不牵动第 1–9 章），措辞改成「第 10 章」。第 8 章集成时若把它指到了 `b09_guzhen_zhuibing`，先挪走再删那张编成。
- **删** `data/battles/b09_guzhen_zhuibing.json` 与 `data/text/battles.json` 里的 `ch09.battle.guzhen_zhuibing.intro`（施工图 1.2、16.1 第 3 条）；删之前核一遍全仓没有别处引用。
- `tests/LexiconTests.cpp` 表一补施工图 12.1 的 36 个词（放在第 8 章集成之后）；第 8 章 12.1 那 11 个若集成时漏了，一并补。
- `data/flags.json`：主线旗标（施工图第 11 节）在内容路派工之前登记；`ch09.path.*` 等 `data/pathactions/ch09.json` 落地后登记。
- `data/chapters.json` 加第 9 章；`data/text/ui.json` 加 `ui.chapter.09.numeral` / `.title`（「第九章」「遁走」）。
- `docs/tech-debt.md`：**G-20 销账**（写明哪一批、哪一条用例钉着）；登一笔「第 10 章建 `recipe_zhenyuan_dan`，`requireFlag = ch09.xingchen`」；「零调用方」那张表里 `rules::Realm::demote` 一行改成与 handoff 同一句（跌落走 `checkDemote` ＋ `RealmDemote`，按级数的 `demote` 保留作纯函数）。
- `docs/handoff.md` 第 4 节那张表：「`rules::Realm::demote` 零调用方，第 9 章要用」一行改成「境界跌落已接通（`RealmDemote`，按目标境界）；按级数的 `rules::demote` 仍无调用方，保留作纯函数」。
- 回写大纲与 lore（施工图 16.1 第 1 条）。
- `tools/validate_selftest.py` 的路径行动文件探针借用了 `ch09` 这个文件名（`write_path_probe` 会删副本里的 `ch09.json`）：`data/pathactions/ch09.json` 落地后实跑 selftest 核一遍；换成 `ch99` 一类永远不会有的章号更稳（施工图 16.2 第 18 条）。

### 6.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/RealmTests.cpp` | `RealmDemotion` 四条（按级数）| 四条不动；`checkDemote` 的表驱动用例放这里或 `Ch09EngineTests` 均可 | 引擎路 |
| `tests/ScriptApiTests.cpp` | `RealmAdvanceRefusesToWalkBackDown` 的注释提到 demote | 既有用例不动；demote 的真脚本用例若放这里，只追加 | 引擎路 |
| 存档用例（`SaveFile` 相关的那个文件）| 断言版本 8 | 版本号断言改到 9；追加 2.4 的往返、老档、写坏 | 引擎路 |
| `tests/RealmCapTests.cpp` | 「上限只升不降」针对 `realm.cap` 与面板 | 不动；若有用例断言「全仓没有任何写入会降低 `realmCap`」，改成排除 `RealmDemote`，判据不改 | 引擎路 |
| `tests/PanelTests.cpp` | 凡人阶段禁词扫描 | E3 的字符串只在修仙阶段出现；若扫描范围含菜单法术页，确认凡人阶段不出现「境界」| 引擎路 |
| 钉着「菜单法术页每一行都是法力数」的既有用例（若有）| — | 只对「境界够的」成立，改成按境界分；判据不改 | 引擎路 |
| `tests/PathActionTests.cpp` | 按章的境界上限表（到第 8 章）、「第 N 章够得着 ＝ 第 1..N 章脚本抬到过的最高」| 补第 9 章：表里记筑基中期（跌落之前）；**另加一条**：第 9 章里 `when` 晚于 `ch09.dieluo` 的条目，门槛不得高于炼气三层（本章 0 条——全部 `realm 0`）；第 10 章起「够得着」要按跌落之后重算（第 10 章的事）| 测试路 |
| `tests/NpcPresenceTests.cpp` | 一个 role 同一时刻只在一张图上 | 曲魂（洞府 → 钟乳洞）、辛如音（金马城 → 无名小山）、齐云霄（金马城撤场）进时间线 | 测试路 |
| `tests/ObjectiveTests.cpp` | `kScriptTransfers` | 加本章十六条；`kOffChainDoorKeys` 本章无 | 测试路 |
| `tests/Ch08*.cpp`、`tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp` 等按章计数、按对象属性比对的用例 | 数第 6、8 章图上的挂点、门、NPC | 本章经 patch 加到 `ch06_baiyaoyuan`、`ch06_huangfenggu`、`ch08_dongfu`、`ch08_tianxing_fangshi`、`ch08_lingkuang`、`ch08_jinmacheng` 的对象（认 `ch09.` 开头的旗标属性，或名字在施工图 3.1 表里）排除在计数之外；三个 NPC 补的 `hidden_flag`（`npc_qu_hun` → `ch09.fengfu`，`npc_qi_yunxiao` / `npc_xin_ruyin` → `ch08.yueding`）若被「旧对象属性不许变」一类用例盯着，改成允许这一个属性；判据不改。**内容路落地到测试路改完之间这几条是红的**——已知，不是内容路的错（第 6–8 章的先例）| 测试路 |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | `ch09-end` 两份写成 v9；`ch08-end`（v8）读进来 `formerRealm` 是凡人 | 测试路 |
| 第 8 章 E1 的灵泉效率用例 | 读 `ch08_dongfu` 的 `facility_lingquan` | **不动**：本章毁泉之后那口泉的设施仍在图上（设施没有在场旗标）；玩家走不回去靠的是施工图第 4 节「封府一段脚本一路演到离开」与坊市门神 | — |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | 不动（本章三张 `drops` 空）| — |

---

## 7. 白名单（四路按文件分，**一个文件只归一路**）

### 7.1 引擎路

- `src/core/rules/Realm.h`、`src/core/rules/Realm.cpp`（`checkDemote`；`demote` 与 `liftToFloor` 两段注释）
- `src/core/model/Types.h`（只追加 `GameState::formerRealm`）
- `src/script/Command.h`、`src/script/ScriptHost.cpp`
- `src/game/Application.cpp`（只加 `RealmDemote` 分支、改 `RealmAdvance` 那段注释）
- `src/game/MenuScene.cpp`（只改法术页 `magicRows` 那一段）
- `src/io/SaveFile.h`、`src/io/SaveFile.cpp`
- `scripts/common/api.lua`（只追加 `realm.demote`、改 `realm.advance` 注释一句）
- `tests/Ch09EngineTests.cpp`（新）；`tests/RealmTests.cpp`、`tests/ScriptApiTests.cpp`、存档用例文件、`tests/RealmCapTests.cpp`、`tests/PanelTests.cpp`（只按 6.3）
- `tools/mksave/mksave.cpp`（**只在**它手拼存档字段、需要 `formerRealm` 时动；走 `toJson` 的话不动）
- `docs/interfaces-p3-script.md`（第 7.1 节补一句「压低上限的是 `realm.demote`，见 `docs/interfaces-p3-ch09.md`」）、`docs/interfaces-p3-ch09.md`（只填 1.8、2.5、4.3 与末尾「实现记录」）

### 7.2 内容路（另一份派工单，写在 `docs/ch09-design.md` 第 18 节头部；这里只列边界）

- `maps/ch09_{huangshan,yuanwu,wumingshan,milin}.tmj`（新 4 张）、`maps/tilesets/terrain_{huangshan,yuanwu,wumingshan,milin}.tsj`（新）
- `maps/ch06_baiyaoyuan.tmj`、`maps/ch06_huangfenggu.tmj`、`maps/ch08_dongfu.tmj`、`maps/ch08_tianxing_fangshi.tmj`、`maps/ch08_lingkuang.tmj`、`maps/ch08_jinmacheng.tmj`（**只经** `genmaps_ch09.patch_*()` 改：只加对象、只给三个已有 NPC 补 `hidden_flag`）
- `tools/mapgen/genmaps_ch09.py`（新）、`tools/mapgen/genmaps.py`（只加登记与六处 patch 调用，排在第 5–8 章之后）
- `scripts/ch09/**`
- `data/text/ch09*.json`（新）
- `data/roles/*.json`（新建 17 个；**不改任何已有 role**——`wang_chan`、`dong_xuaner`、`guiling_shaozhu`、`yanli_nanzi`、`guilingmen_zhanglao`、`xin_ruyin`、`qu_hun` 一律不动）
- `data/items/**`（新建 10 件；`pills/dingyan_dan.json` **只改 note**）
- `data/magics/*.json`（新：我方 2 门、敌方约 4 门；`qingyuan_jianmang.json` **只改 `needRealm`**）
- `data/battles/b09_{tuwei,fujia,dong_xuaner}.json`（新 3）
- `data/quests/q09_baichi.json`、`data/pathactions/ch09.json`、`data/objectives/ch09.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加 4 条登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch09-design.md`（**只追加第 18 节「施工偏差」**，平衡路的实测也写在这里）

### 7.3 协调者

- `tools/validate.py`、`tools/validate_selftest.py`（**只** 6.2 那几处）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第九章」「遁走」）；`data/text/battles.json`（只删一条）；`data/battles/b09_guzhen_zhuibing.json`（删）
- `tests/LexiconTests.cpp`（表一补词）
- `docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（回写）

### 7.4 测试路（两路都落地、协调者 6.2 之后另派）

- `tests/Ch09*.cpp`（除 `Ch09EngineTests.cpp`）：通关、挂点、编成、账、切片、验收
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`
- `tests/Ch08*.cpp`、`tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp`（只按 6.3 那几行改）
- `tests/ChapterFixture.h`、`tests/fixtures/ch09-end-{first,second}.sav`、`saves/README.md`

---

## 8. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记主线旗标 → 引擎路（E1–E3）∥ 内容路 → 协调者 6.2 与集成 → 测试路 → 独立校对。**本章在第 8 章集成之后开工**（起点 fixture `ch08-end-*.sav` 要先存在，施工图第 2 节全是「估」）。
- **唯一一处跨路依赖**：内容路 `scripts/ch09/shudong.lua` 里的 `realm.demote` 在引擎路 E1 合进来之前跑不通——内容路可以先写，节点 15 之后的手测与门禁以外的检查等 E1 落地。协调者先合引擎路、再合内容路。
- 每路用自己的构建槽：引擎路 `ch09eng`，内容路 `ch09content`，测试路 `ch09test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 几路共用一棵工作树时，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实，不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- 门禁先于编译跑：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`。
- **旧图只经 patch**：六张旧图只加对象、只补那三个 `hidden_flag`；`genmaps_ch05.py`–`genmaps_ch08.py` 一个字不改。

---

## 实现记录

2026-10-09 引擎路交付，唯一施工目录：`C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/wt09/fanren`。字段/参数最终名为 `formerRealm`、`realm.demote(value)`、命令kind `realm_demote`、`RealmDemote`、规则 `checkDemote(current, target)`，存档v9。

改动路径共21个（相对此目录）：

- `src/core/rules/Realm.h`
- `src/core/rules/Realm.cpp`
- `src/core/model/Types.h`
- `src/script/Command.h`
- `src/script/ScriptHost.cpp`
- `src/game/Application.cpp`
- `src/game/MenuScene.cpp`
- `src/io/SaveFile.h`
- `src/io/SaveFile.cpp`
- `scripts/common/api.lua`
- `tests/Ch09EngineTests.cpp`（新增）
- `tests/BattleTests.cpp`
- `tests/Ch03PartyTests.cpp`
- `tests/Ch04MagicTests.cpp`
- `tests/EncounterTests.cpp`
- `tests/Ch06EngineTests.cpp`
- `tests/Ch07EngineTests.cpp`
- `tests/IoTests.cpp`
- `tests/QuickSaveTests.cpp`
- `docs/interfaces-p3-script.md`
- `docs/interfaces-p3-ch09.md`

**7.1白名单范围补注（契约6.3的存档用例）**：既有保存用例必要修改如下，不删除用例、不改往返/行为/章节断言，历史测试名保留。

| 文件 | 必要修改及原因 |
| --- | --- |
| `tests/BattleTests.cpp` | 文件内当前版本断言8→9；保存夹具的境界已为3，上限补为3，才能继续验证破绽名往返 |
| `tests/Ch03PartyTests.cpp` | 当前版本常量断言8→9；其余不动 |
| `tests/Ch04MagicTests.cpp` | 当前版本常量断言8→9；法术往返夹具的境界已为3，上限补为3 |
| `tests/EncounterTests.cpp` | 当前版本常量断言8→9；遭遇计数器判据不动 |
| `tests/Ch06EngineTests.cpp` | 天眼术“存档→读档→下场已知破绽”用例保存前补合法上限1行，破绽/战斗判据不动 |
| `tests/Ch07EngineTests.cpp` | 瓶子读档用例手设筑基初期，补相同上限1行，容量/滴数判据不动 |
| `tests/IoTests.cpp` | 保存样本手设炼气三层，补相同上限1行，其余字段保持原值，往返比较不动 |
| `tests/QuickSaveTests.cpp` | 两条快存用例三次手设境界，逐次补相同上限共3行，保存身份/覆盖判据不动 |

协调者依据用户已批准的连续施工授权确认必要存档断言同步。后4个文件均是6.3所述SaveFile相关存档用例；全量测试实际暴露上限默认0的夹具被E2拒绝，因此只补合法测试前置状态，不在生产读档路径自动修正，也不把失败改成预期成功。

验证记录：

- 自有构建槽 `resume9eng`，PowerShell直接执行 `build_logged.bat resume9eng`。
- 基线实际1624/1630通过、6条第7章失败；四道门禁通过，selftest220/220，编译警告0。日志 `build-resume9eng/baseline.log`。
- 新增27条独立测试全部通过，9类有效负向自检均编译成功并捕获错误，代码已还原。详细退出码/失败用例见 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/work-notes/eng09` 的XML及阶段记录；日志在自有构建目录。
- 本轮PowerShell `build_logged` 的四道门禁与编译通过。第一次全量出现16条失败：6条基线第7章问题、10条旧保存夹具的上限默认0。夹具按上表修正后，56条相关回归全部通过、编译警告0；门禁输入未变，最终同槽全量CTest为1651/1657通过、6条失败，实际退出8。与基线失败用例逐名比较完全相同，新增失败0。最终日志 `build-resume9eng/final-ctest.log`；原16条失败日志保留为 `build-resume9eng/full-before-fixture-caps.log`。
- 保留的基线红项：`Ch07Walkthrough.FirstSideWalksTheWholeChapterAndEveryGateHoldsThenOpens`、`Ch07Walkthrough.SecondSideTakesEveryOtherChoice`、`Ch07Acceptance.No6_TheChapterCardAndTheTwoGatesSeenInChapterSixSayNothingOfChapterSeven`、`Ch07IntentHand.TheIntentPlayerWinsEachOfTheFiveFromAFullBar`、`Ch07IntentHand.FengYueFallsWithoutTheThunderPearl`、`Ch07Ledger.TheMarketRoadNeverPaysMoreForOneHerbThanTheWholeChapterCap`。没有修改这些用例或其内容数据。
- 本机中文MSVC诊断曾使Ninja依赖前缀乱码，头文件变化后混入旧目标文件；在自己槽内设UTF8诊断输出、fresh并完整重编译后解决，Ninja已记录Types.h等头文件依赖。没有改仓库构建脚本。

独立审查整改记录（2026-10-09）：

- 依据 `docs/ch09-engine-review.md` 的两个P2进行修复，审查文件保留原文。仅续改 `src/script/ScriptHost.cpp`、`tests/Ch09EngineTests.cpp`、本契约实现记录及外部阶段笔记；没有扩大旧命令的解析重构。
- 修复前实际编译成功，两个新增反例均失败、测试退出1：`realm.demote(4294967299)` 错误落到3；合法v9档的扫描错误要求formerRealm=凡人。日志 `build-resume9eng/p2-before.log`、报告 `work-notes/eng09/p2-before.xml`。
- 修复后本章33/33通过（原27条加6条），编译警告0。日志 `build-resume9eng/p2-final-subset.log`；最终同槽全量CTest为1657/1663通过、6条失败，实际退出8，失败集合与上列基线6条逐名完全相同，新增失败0。日志 `build-resume9eng/p2-final-ctest.log`，耗时65.74秒。这个结果保留已知红项，不称作全量全部通过。
- 本轮门禁输入未变，沿用上一轮实际通过的四门禁证据；本轮实际执行了增量编译、两个修复前反例、33条本章测试及1663条全量CTest。独立审查原文件SHA256未变，后续审查结论由协调者处理。

未做：可选O1–O4、第8/9章内容与数据、验证工具、角色/地图/视觉、CultivationScene、第10章、技术债/集成文档、主树/wt08、提交推送。`mksave` 已调用 `saveGame`，无需手拼新字段，未改。vendor Junction只使用现有依赖，保留worktree。

Quest 路径行动旗标来源修补（2026-10-09）：

- 本补仅改 `tools/validate.py`、`tools/validate_selftest.py` 及本段实现记录。前述引擎交付记录保留；未改 src、content、C++ tests、既有 CHAPTER_MEANS/范围探针，未做 Git 写操作或派代理。
- 真实 producer 已核对：`src/core/rules/PathActions.cpp` 的 `pathActionEffects` 在首次打探时生成 `set_flags` 与 `done_flag` 的 SetFlag，求购成交生成 done_flag；`challengeResultEffects` 只在胜利时生成 done_flag。`pathActionShown` 排除 pending 切磋；`Application::applyPathEffects` 逐项调用 `applyPathEffect`，其中 SetFlag 分支调用 `state_.setFlag(effect.flag)`，战斗回调接入胜负效果。
- `check_path_actions` 返回通过文件/条目形状、旗标登记、引用和 NPC 在场检查且非 pending 的 producer 来源，按脚本已有来源与合法路径行动的非零 when 前置旗收敛，排除 pending/非法 producer 的下游及无起点的自依赖。`check_path_entry` 补 done_flag 登记检查；`check_path_cross` 的在场检查移到逐条校验处，其他旧负向规则保留。`main` 先取得该集合，再由 `check_quests` 合并脚本来源；`check_quest_condition` 在来源为空时仍报无人产生。没有 ch09 特例、假 whitelist 或手塞脚本旗标。
- 新增 `selftest_m_path_sources`，由 selftest.main 接入，实际运行 validate.main：5项正例为打探/求购/胜利切磋完成旗、打探附带旗及合法前置链；16项负例覆盖仅登记、来源全空、done_flag/set_flags 未登记、非法字段/完成旗形状/顶层 id、缺地图/NPC/文案/物品/战斗、pending，以及前置旗仅来自 pending/非法条目或自身。另1项验证负例后逐字节还原及合法任务重新通过。专项实跑22/22，退出0。
- 实际 q09_baichi 正反门禁：同一 main 临时以脚本来源接线时 steps 三项与 complete 三项共6错、退出1；恢复真实路径来源后该任务0错，当前全量 validate.py 为576条数据、4115条文案、394个旗标，0错0警告、退出0。接线仅在验证进程中临时切换并恢复，没有修改正式内容。初始8错中的另2项 C8 path until 在并行内容施工中消失，本补没有触碰或认领该修复；此结果不表示第9章全部交付。
- 冻结前追加授权的6.2集成项同批落在两工具：补 `CHAPTER_MEANS[9]` 的拳、火、土、水、金、木、暗器七种常驻手段及真实前章来源，`MEANS_LAST_CHAPTER=9`；`selftest_n_battle_break` 的范围外探针迁到 WT9 内现有 `data/roles/daoyu_husui.json` / `b10_gujia_bidou`，`write_path_probe` 与 `selftest_p_path_actions` 的坏文件名探针从 ch09 迁到 ch99，阴性断言保留，并加真实 ch09 路径文件字节不变的正向检查。旧 b09 编成/intro 删除和 Lexicon 由内容路负责；本批未触 WT10 或 E3 形状规则。
- 追加配置前完整 selftest 实跑242/242（既有220项加来源22项），退出0、SELFTEST_OK。追加配置后 validate 实跑0错0警告、退出0；按最新串行约定停止尚未完成的完整 selftest，两工具冻结，最终完整复验等待明确 CONTENT_FROZEN，再按实际总数/退出码补记。中止运行不计作最终通过。

独立门禁审查返修（2026-10-09）：

- 按 `docs/ch09-gate-review.md` 的2项HIGH与1项MEDIUM返修，仅续改两工具及本段记录。Euclid已明确交回停止后的selftest窗口，CONTENT_FROZEN保持；未改独立审查原文、正式内容、src/C++ tests、主树或WT10，也没有Git写操作或派代理。前述242/242只保留为历史证据，不用于关闭本次发现。
- `check_quest_condition` 对路径旗标保留确定的置1语义，拒绝 `>=2` / `==2`；新增 `path_window_can_open`，复用 `path_implies` / `path_contradicts` 与章节公理，再对单个旗标/物品的when整数范围及until取反范围求交，路径旗限0/1，无来源旗只限0。`check_path_entry` 拒绝可证明永闭的窗口；`check_path_actions` 在每轮来源收敛时用同一窗口检查，故非法完成/附带旗及其下游不进sources。脚本旗保留既有来源分析口径，合法1/2与变量写旗前提均通过，不展开Lua控制流、不引入全局SAT。
- `selftest_m_path_sources` 的胜利切磋正例改为完整的 `done_flag == 1` 谓词，保留原22项及真实ch09字节保留项。新 `selftest_m_path_values` 通过真实C9行动/Quest与validate.main新增22项：9项负例覆盖前置/Quest的>=2和==2、when蕴含until、互斥when、置1后恒闭、永闭附带旗/下游、多until覆盖全部取值；12项正例覆盖原样C9、>=1/==1、首次==0、until==0后开启、B→A→C链、脚本1/2/变量及存在取值2的合法窗口；1项逐字节还原并原样再验。新M3修前22项中9失败、退出1；修后22/22、退出0，临时样本均还原。
- 已重跑独立量具：真实PathActions/Quest C++量具退出0、RUNTIME_PROBE_OK，确认86条运行时行动、首次打探与胜战置1、重看不累加、败战不置完成旗、pending隐藏；M2正例现在与实测胜战值1一致。独立counterexamples入口中三类旧误放行修前均退出0；修后取值2前置/恒闭窗口/任务取值2分别退出1并报4/3/1错，原自环/两节点环仍拒绝，干净内容退出0。boundaries退出0、BOUNDARIES_OK，原第3章狼/第9章破绽拒错、第10章范围外与非法形状拒错、手段来源及ch99旧诊断/C9字节保留均维持。
- 冻结内容实跑validate为575数据、4117文案、394旗标，0错0警告、退出0；冻结清单中127项内容/正式C8起点SHA256核对0差异。完整selftest已在交回的串行窗口重跑，最终总数与退出码待本次结束后追加；不以专项或中止结果代替完整验收。
- 最终串行 `python -B -X utf8 -u tools/validate_selftest.py` 实跑265/265、0失败、退出0、SELFTEST_OK：既有220项、M2原22项加真实ch09保留项共23项、新M3共22项。旧负向判据全部保留且实跑；Python AST、diff空白和UTF-8无BOM/LF检查通过。两工具冻结SHA256：validate.py `4afe03bc388e42adeeb3d36e8ee7b46063773713fcc120b0717a8c980e74f5e2`，validate_selftest.py `2d6d57c15d7c310910a1dbd8b7e6cb9583899a4577c4eb9e38902682a8a94ab4`。独立审查结论仍由审查路复验更新；本记录不冒称整章通关或运行时购买/战斗验收已完成。
