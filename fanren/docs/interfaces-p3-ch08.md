# P3 第 8 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-09-29 · 依据 `docs/ch08-design.md` 第 10 节（引擎前置）、第 7 节（系统解锁）、第 8 节（编成）、第 11 节（旗标）、第 12 节（禁词）。
> 本批引擎路做 **E1**（打坐设施按地点给效率——灵眼之泉）与 **E3①**（E1 的门禁形状检查）；**E2**（筑基档攻防标定）是「量了再定」：平衡路拿 ①–⑥ 量完给数，要改就由引擎路在本批里改，不改也要落一行注释（第 2 节）。
> 门禁分两半：形状检查（第 5.1 节）归引擎路、派工即做（与第 7 章契约 5.1 的改派一致）；规则 25 的表（`CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER`）与 selftest 探针在集成时由协调者改（第 5.2 节）。
> 可选 O1（傀儡兽「分神三里」「命令延迟」做成战斗机制）、O2（阵法系统）、O3（队员在场开关 `party.set_active`）**都不做**。
> **本章在第 7 章集成之后开工**：第 7 章的 E1（配方 `requireFlag`）、E2（道具施法与 `stagger`）、E3（瓶子容量随境界）、E4（`count_aged` / `take_aged`）本章都当成已有的用。

---

## 0. 三件事，各自现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 在洞府的灵眼之泉打坐，同样的日子多得二成半修为（ch220：上品的灵眼之物能让修炼快两三成）；别处照旧 | `rules::meditate(realm, aptitude, effectiveness, days, seed)`（`src/core/rules/Cultivation.h`）早就收 `effectiveness`、上限 `kMaxEffectiveness = 1000`，每日收益对它是线性的；但游戏层只有一个入口 `meditationEffectiveness(const core::GameState&)`（`src/game/CultivationScene.cpp`）**恒返回 100**；`Application::openFacility` 的 `meditate` 分支是 `pushScene(std::make_unique<CultivationScene>())`，**不读设施的任何属性**；`CultivationScene::bankMeditation(state, days, seed)` 在里面自己取 `meditationEffectiveness(state)`；地图规范 4.6（仓库根 `docs/map_spec.md`）的设施属性表里没有这一项；`tools/validate.py` 对设施只要求 `kind`（`OBJECT_REQUIRED_PROPS["facility"]`），别的属性一概不查 |
| E2 | 筑基初期、中期的攻防是平衡路量过的数，而不是暂定值 | `src/core/rules/Realm.h` 的筑基档攻防注明暂定（初期攻 32 防 20、中期攻 37 防 26）；`tests/RealmTests.cpp` 的 `RealmCombat.PinsTheProvisionalFoundationAndCoreNumbers` 钉着它们；曲线锚点表里筑基只有后期一条（`{Realm::FoundationLate, 470, "wang_chan"}`）|
| E3① | `effectiveness` 写错了门禁抓得住 | 见 E1 一行末尾：门禁不查设施的其他属性 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。E1 的规则层早已齐备、地图加载器把任意属性原样收进 `MapObject::property`，**本批只补游戏层入口与门禁**。

**查过、本章不需要新做的**（`docs/ch08-design.md` 10.1 末尾那张表的依据）：多波次（第 4 章 `wave`、`BattleState::deployNextWave`）；友军单位（第 5 章）；首领蓄势与多次行动；道具施法与 `stagger`（第 7 章 E2）；配方 `requireFlag`（第 7 章 E1）；`count_aged` / `take_aged`（第 7 章 E4）；瓶子容量随境界、筑基五日一滴（第 7 章 E3、`bottleChargeDays`）；`realm.advance(22)`（`tryNext` 已通 21 → 22）；`party.add` / `party.remove`（`scripts/common/api.lua`，看返回值）；多块灵田（`field.unlock` 按 `ref_id` 分开，田按 `kDaysPerYear` 生长）；设施 `require_flag`（`Application::openFacility` 已判，未置时说 `ui.facility.locked`）；NPC `visible_flag` / `hidden_flag`（`WorldScene::npcVisible`，在场即占格，规则 13 把带旗标的 NPC 当可走）；输了不死（编成 `defeat_is_fatal: false`，`battle()` 返回 `won=false, how="lost"`）；遭遇区 `require_flag`；商店、路径行动、任务、目标链。

---

## 1. E1 打坐设施按地点给效率

### 1.1 一句话口径

**地图上 `kind=meditate` 的设施可以带一个整数属性 `effectiveness`（百分比，缺省 100）：在那一处打坐，交给 `rules::meditate` 的效率乘上它；没写的蒲团、凡人阶段的日课，一切照旧。**

### 1.2 数据

- 新的设施属性 **`effectiveness`**（Tiled 自定义属性，`int`）：只许写在 `kind=meditate` 的设施上；取值 100–300；缺省（不写）= 100。门禁见 5.1。
- 本章只用一处：`maps/ch08_dongfu.tmj` 的 `facility_lingquan`，`effectiveness = 125`（内容路写，经 `tools/mapgen/genmaps_ch08.py` 生成）。
- 仓库根 `docs/map_spec.md` 4.6 的设施属性表加一行（引擎路）：`effectiveness` / int / 否 / 「`kind=meditate` 时有意义：在此打坐的修为效率（百分比，100–300，缺省 100）。灵眼之泉这类洞府灵脉写它；功法、灵根的加成不写在地图上」。

### 1.3 规则层

**不动**。`rules::meditate` 的每日收益本来就是 `baseRate × (50 + aptitude) × effectiveness / 100`，对效率线性；`kMaxEffectiveness`、`kMaxMeditateDays` 不改。

### 1.4 游戏层（`src/game/CultivationScene.*`、`src/game/Application.cpp`）

- `CultivationScene` 加一个构造参数：`explicit CultivationScene(int sitePercent = 100);`，存成成员（名字引擎路定，下文叫 `sitePercent_`）。缺省 100，现有的 `std::make_unique<CultivationScene>()` 调用一处不用改。
- `bankMeditation` 加一个缺省参数：`static MeditateOutcome bankMeditation(core::GameState& state, int days, std::uint32_t seed, int sitePercent = 100);`。函数里交给 `settleMeditation` 的效率 = `meditationEffectiveness(state) × sitePercent / 100`（整数运算；`sitePercent` 先夹到 `[0, rules::kMaxEffectiveness]`）。**缺省参数保证 `tests/PanelTests.cpp` 那一串三参调用的结果一个点都不变。**
- `meditateFor(app, days)` 调 `bankMeditation(state, days, meditationSeed(state), sitePercent_)`。**日课**（`settleDailyPractice`，只在凡人阶段有）照旧走三参版本——灵脉加成只给「在那一处坐下」的人。
- `Application::openFacility` 的 `meditate` 分支：`propertyInt(facility.property("effectiveness"), 100)` 读出来，非正数当 100（门禁已拦，这里只是不崩），`pushScene(std::make_unique<CultivationScene>(value))`。`require_flag` 那道闸照旧在前面。
- **面板上看得见**：`CultivationLexicon`（`src/game/CultivationScene.h`）加两个字段 `siteBonusPrefix` / `siteBonusSuffix`；`sitePercent_ != 100` 时状态栏多画一行 `前缀 + (sitePercent_ − 100) + 后缀`（例：「此地灵气充沛，修为多得 」「%」）。**两套词都要写**：修仙阶段可以说灵气；凡人阶段那一套不许带「灵气」「修为」之类的禁词（例：「此处静坐格外见效，多得 」「%」）——凡人阶段实际不会出现这一行，但 `cultivationPanelStrings` 两个阶段都扫。两个字段都加进 `cultivationPanelStrings(stage)`，`tests/PanelTests.cpp` 的禁词扫描因此自动覆盖。
- `meditationEffectiveness(const GameState&)` **不改签名、不改返回值**（它是「功法、灵根」那一路将来的合成点；地点加成是另一路，乘在它外面）。

### 1.5 判据（`tests/Ch08EngineTests.cpp`，引擎路写）

1. **比例**：同一个 `GameState`（`realm = FoundationEarly`、资质 50、修为离门槛尚远）复制两份、同一颗种子，各 `bankMeditation(…, 120, seed, 100)` 与 `bankMeditation(…, 120, seed, 125)`：后者的 `cultivation × 100` 与前者的 `× 125` 相差不超过 125（即取整误差不超过 1 点）；两份的 `insight` 相同。
2. **缺省不变**：三参调用与显式传 100 的结果逐字段相等（`cultivation`、`days`、`insight`、`cultivationRemainder`）。
3. **真实入口**：headless `Application` 加载一张带 `effectiveness=125` 的 meditate 设施的测试图（或直接拿 `ch08_dongfu` 的 `facility_lingquan`，内容路落地之后），走 `openFacility` → 场景栈顶是 `CultivationScene` → `meditateFor(app, 90)`；与同一存档在 `ch06_baiyaoyuan` 的 `facility_dazuo` 坐 90 天比，比例同第 1 条。
4. **面板那一行**：`sitePercent = 125` 的场景，状态栏文字里含「25」；`sitePercent = 100` 的不含那一行。`cultivationPanelStrings(Mortal)` 与 `(Immortal)` 都含两个新字段。
5. **日课不沾光**：凡人阶段存档，`settleDailyPractice` 的结果与改动前逐字相等（用改动前的数字写死）。
6. **负向**：把 `openFacility` 里读属性那一句删掉（或 `meditateFor` 改回三参），第 3 条红；把 `sitePercent` 的乘法删掉，第 1 条红。

### 1.6 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch08eng` · WT 基线 `21e3bd8`。

- **名字**：缺省值具名为 `kDefaultSitePercent = 100`（`CultivationScene.h`，与 `kDefaultCraftToolGrade` 同一个做法；构造、`bankMeditation`、`openFacility`、`siteBonusLine` 四处都用它）。`explicit CultivationScene(int sitePercent = kDefaultSitePercent)`，成员 `sitePercent_`；`bankMeditation(state, days, seed, int sitePercent = kDefaultSitePercent)`。
- **游戏层**：交给 `settleMeditation` 的效率 = `meditationEffectiveness(state) × clamp(sitePercent, 0, kMaxEffectiveness) / 100`；`meditateFor` 传 `sitePercent_`，`settleDailyPractice` 照旧三参。`meditationEffectiveness` 签名、返回值未动，只把注释里「洞府灵气并进这里」改成「乘在外面」。余数账记的是天数、差分按此刻的效率取：换一处坐时已进账的不回算，只有零头按新效率重折（差不到一点），注释写在 `bankMeditation`。`openFacility` 的 meditate 分支 `propertyInt(facility.property("effectiveness"), kDefaultSitePercent)`，非正数当缺省；`require_flag` 那道闸照旧在前。
- **面板**：`CultivationLexicon` 末尾加 `siteBonusPrefix` / `siteBonusSuffix`——修仙「此地灵气充沛，修为多得 」「%」，凡人「此处静坐格外见效，火候多得 」「%」；两个都进 `cultivationPanelStrings`。新公开 `siteBonusLine(const GameState&) const`（`sitePercent_ ≠ 100` 才有，否则空串），`renderStatus` 把它画在修为条下面——与 `spiritRootLine` 同一个做法（无头绘制是空操作，测试问的是它）。
- **规范**：仓库根 `docs/map_spec.md` 4.6 表加 `effectiveness` 一行（1.2 原文）。
- **测试**（`tests/Ch08EngineTests.cpp`，6 条）：第 1 条 `Ch08SiteMeditation.TheSpringPaysFiveQuartersOfTheCushionWithTheSameInsight`（种子扫 1–32，有顿悟、无顿悟的都真出现过）；第 2 条 `…TheThreeArgumentCallIsExactlyAnExplicitHundred`（带余数账连坐 1 / 10 / 120 / 360 日）；第 3 条 `Ch08SiteEntry.ASpringOfOneHundredTwentyFiveOnTheMapPaysFiveQuartersOfTheCushion`——125 侧是测试图（`ch06_baiyaoyuan.tmj` 的副本，`facility_dazuo` 加一条 Tiled int 属性 `effectiveness=125`，`io::loadTileMap` 读进来交给 `openFacility`），100 侧是真图同一张蒲团、`WorldScene::interact`；各坐 90 日：**灵泉 45、蒲团 36**；第 4 条 `Ch08SitePanel.OnlyASpotAboveTheCushionAddsTheLineAndBothStagesListItsWords`（另问凡人那一句不带「灵气」「修为」：「修为」不在 `PanelWording` 的禁词表里）；第 5 条 `Ch08SiteMeditation.DailyPracticeIsExactlyWhatItWasBeforeTheSpring` ＋ `Ch08SiteEntry.TheDailyPracticeOwedBeforeSittingAtTheSpringTakesNoShareOfIt`——改动前的数（炼气二层、资质 40、欠一百日：进账 9、余数账 60、水位 101）是隔离副本里**原样的源码**加一段探针跑出来的；第二条在一块 125 的面板上打坐，先补的那笔日课仍是 9。内容路的 `ch08_dongfu` 还没落地，第 3 条没拿 `facility_lingquan` 比——测试路可照同一个比法再比一遍真图。`tests/PanelTests.cpp` 没改：两个新字段进了 `cultivationPanelStrings`，禁词扫描自动覆盖。
- **负向自检**：见末尾「实现记录」。

---

## 2. E2 筑基档攻防标定（量了再定）

### 2.1 一句话口径

**筑基初期、中期的攻防由平衡路拿本章六场必打（`tests/BattleHand.h` 那只手）量过之后定：要改，就在本批里连同 `RealmTests` 一起改；不改，就在 `Realm.h` 那几行旁边写明「第 8 章标定：不改」。两种结果都算交付，不许留着「暂定」两个字不动。**

### 2.2 改的话改哪里

| 依赖 | 现状 | 改法 |
| --- | --- | --- |
| `src/core/rules/Realm.h`（及 `Realm.cpp`，若数在表里）| 筑基初期攻 32 防 20、中期攻 37 防 26（暂定）| 换成平衡路给的数；注释写明依据（哪几场、哪只手）|
| `tests/RealmTests.cpp` `RealmCombat.PinsTheProvisionalFoundationAndCoreNumbers` | 钉着暂定值 | 同一次改动里改成新数；用例名可以改成去掉 Provisional 的说法 |
| `tests/RealmTests.cpp` 曲线锚点表 | 筑基只有后期（`wang_chan 470`）| **推荐**加一条筑基中期：`{Realm::FoundationMid, <guiling_shaozhu 的 maxHp>, "guiling_shaozhu"}`，判据照表里其余各条的 ±15%；数由内容路在 `data/roles/guiling_shaozhu.json` 定，引擎路只抄字面量 |
| `Realm.h` 注释表 | 按境界列数据点 | 加「筑基中期　guiling_shaozhu」一行（同上）|

气血、法力（`realmMaxHp` / `realmMaxMp`：筑基初期 260 / 180、中期 380 / 250）**不在标定范围内**：第 7 章终点断言（`maxHp ≥ 260`、`maxMp ≥ 180`）与本章终点断言（`≥ 380`、`≥ 250`）都读它们。

### 2.3 判据

- 改了：`RealmTests` 全绿，且新数与 `docs/ch08-design.md` 第 18 节（施工偏差）里平衡路写的表一致。
- 不改：`Realm.h` 那几行旁边有「第 8 章标定：不改」与依据；`RealmTests` 不动。
- 锚点那一条（若加）：`guiling_shaozhu` 的 `maxHp` 落在 `realmMaxHp(FoundationMid)` 的 ±15% 内。

### 2.4 实现记录（引擎路交付时填）

引擎路 · 2026-09-29。

- **没改数**（派工单：「量了再定」按契约写注释、不擅自改值）。交付时六张必打编成、平衡路的量法都还没落地，`docs/ch08-design.md` 也还没有第 18 节，没有数可依。
- `src/core/rules/Realm.h` 攻防表下面加了一段注释：这两行由平衡路拿 ①–⑥ 量过之后定，改就连同 `RealmTests` 的 `PinsTheProvisionalFoundationAndCoreNumbers` 一起改，不改就把这一段换成「第 8 章标定：不改」并写依据。`Realm.cpp` 那几行「// 暂定」与 `tests/RealmTests.cpp` 都没动——**「暂定」两个字还在，本节尚未交付完**，等平衡路给数后在同一批里落（交协调者）。
- 锚点（推荐项）没加：`data/roles/guiling_shaozhu.json` 还没落地，引擎路只抄字面量、无数可抄；与上一条一起落。

**2026-10-09 收口：第 8 章标定不改。** 测试路从正式第 7 章两侧章末存档，经真实 Application 与 BattleHand 走完六战：6/4/8/6/3/3 回合均胜，③无雏角与⑥无血灵钻分支也胜。筑基初期 32/20、中期 37/26 保留，气血法力基准不变，`RealmTests` 严格断言不动；Realm.h 注释已注明依据。没有把外置 mock 胜利或历史扰动扫描当本轮证据，未新增非必需敌方气血锚点。

---

## 3. （本章没有第三件引擎功能）

第 7 节的「洞府经营」「阵法布防」「傀儡」「情报板」全部用已有的东西拼：灵田、丹房、制符桌、`once=false` 挂点、位掩码旗标、`party.add`、编成数据（`docs/ch08-design.md` 第 7 节「不做什么」一栏）。**引擎路不为它们加任何代码**；内容路发现哪一样拼不出来，先停下来改本文，不在脚本里绕。

---

## 4. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让两路对同一件事只有一种写法：

- **布阵**：三组阵位挂点（`once=false`，不写 `set_flag`）各自 `flag.set("ch08.zhenqiN", flag.get("ch08.zhenqiN") | bit)`（位 1 / 2 / 4 / 8，Lua 5.4 的整数按位或）；阵眼挂点（`once=true`）先判 `flag.get("ch08.zhenqiN") == 15`，不齐就说一句、`return`（**不置完成旗标**）。三处共用 `scripts/ch08/` 里一个模板函数。
- **年份**：交药、换宝一律 `item.count_aged` / `take_aged`（第 7 章 E4），不用 `take(id, n, 恰好年份)`。
- **E2 道具**：`talisman_mojiao_chujiao`、`talisman_xuelingzuan` 走第 7 章 E2 的 `castMagic`（`magic_chujiao_zibao`、`magic_xuelingzuan`，`effect: "stagger"`、`stagger: 4`、`power: 0`）。
- **输了不死**：④ `b08_jinguyuan_juezhan` 与三张可选编成 `defeat_is_fatal: false`；脚本 `local won = battle(id)`，`won == false` 时拨日子、不置旗标、`return`。
- **teleport**：二十条都写在 `docs/ch08-design.md` 3.1 末尾，测试路登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`。

---

## 5. 门禁与既有测试的依赖

### 5.1 引擎路：形状检查（纯增补，不依赖本章新数据）

这一批只加检查、不加本章的表，落完 `validate.py` 与 `validate_selftest.py` 须照旧全绿：

1. 地图设施属性 `effectiveness`：只许出现在 `kind=meditate` 的设施上；必须是整数（Tiled `int` 属性，不是字符串）；取值 100–300。三条各报各的错。
2. `validate_selftest.py` 每一条配负例（写坏 → 必报 → 还原）：写在 `kind=field` 的设施上、写成字符串 `"125"`、写成 `90`、写成 `400`；另配一条正例（`meditate` 上写 `125` 不报）。

`CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段**不动**（5.2，协调者）。

### 5.2 协调者：集成时落（依赖内容路的数据与脚本）

- `CHAPTER_MEANS[8]`：拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`give` `weapon_wuming_sixian`，ch07）；`BATTLE_EXTRA_MEANS` 本章不加；`MEANS_LAST_CHAPTER = 8`。来路核对要去 `scripts/ch08/` 找 `magic.learn("magic_qingyuan_jianmang")`，所以只能在内容路落地之后加。
- `validate_selftest.py` 那条「第 N 章的敌人不在规则 25 的范围 → 不报」：改指第 9 章——`data/roles/wang_chan.json` ＋ `b09_guzhen_zhuibing`（`wang_chan` 只在那一张编成里出现，改它的破绽不会牵动第 8 章），措辞改成「第 9 章」。第 7 章集成时若已把它挪到某张 b08 编成上，这里再挪一次。
- 删 `data/battles/b08_jinguyuan_weigong.json`（`docs/ch08-design.md` 8.2）；删 `data/text/battles.json` 里三条旧的 `ch08.battle.*.intro`（内容路的新 key 在 `data/text/ch08_battle.json`）。
- `tests/LexiconTests.cpp` 表一补 `docs/ch08-design.md` 12.1 的 112 个词（**放在第 7 章集成之后**，并先对第 7 章的文案重扫一遍，16.2 第 10 条）。
- `data/flags.json`：主线旗标（第 11 节）在内容路派工之前登记；`ch08.path.*` 等 `data/pathactions/ch08.json` 落地后登记。
- `data/chapters.json` 加第 8 章；`data/text/ui.json` 加 `ui.chapter.08.numeral` / `.title`（「第八章」「魔道入侵」）。

### 5.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/PanelTests.cpp` 面板禁词扫描 | 扫 `cultivationPanelStrings(Mortal / Immortal)` | E1 的两个新字段进这张表，扫描自动覆盖；凡人阶段那一套词不许带禁词（1.4）| 引擎路 |
| `tests/RealmTests.cpp` | 暂定值、筑基锚点只有后期 | 第 2 节 | 引擎路 |
| `tests/Ch05*.cpp`、`tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp` 等按章计数、按对象属性比对的用例 | 数第 5、6 章图上的挂点、门、NPC | 本章经 patch 加到 `ch05_nancheng`、`ch05_mofu`、`ch06_huangfenggu`、`ch06_baiyaoyuan`、`ch06_tainan_cun` 的对象（认 `ch08.` 开头的旗标属性，或名字在本章 3.1 表里）排除在计数之外；五个 NPC 补的 `hidden_flag`（`npc_lin_shidi` → `ch08.midian`，`npc_huyuan_a`–`_d` → `ch08.junling`）若被「旧对象属性不许变」一类用例盯着，改成允许这一个属性取 `ch08.*` 的值；判据不改。**内容路落地到测试路改完之间这几条是红的**——已知，不是内容路的错（第 6、7 章的先例）| 测试路 |
| `tests/NpcPresenceTests.cpp` | 一个 role 同一时刻只在一张图上 | 宋蒙（金鼓原 → 越京）、墨凤舞（墨府 → 越京）两段进时间线；林师弟、南城护院的撤场进时间线 | 测试路 |
| `tests/ObjectiveTests.cpp` | `kScriptTransfers` | 加本章二十条；`kOffChainDoorKeys` 本章无 | 测试路 |
| `tests/PathActionTests.cpp` | 按章的境界上限表、钱袋表 | 补第 8 章：本章脚本抬到筑基中期；第 7 章终局第二侧的灵石付得起本章的求购 | 测试路 |
| `tests/SliceTests.cpp`「迁移战斗逐场跑完」 | 不含 b08 | 不动；删掉的 `b08_jinguyuan_weigong` 不在表里 | — |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落率 | 不动；本章六张必打 `drops` 空，可选编成若有掉落一律 `rate` 缺省或 100 | 内容路（数据）|

---

## 6. 白名单（四路按文件分，**一个文件只归一路**）

### 6.1 引擎路

- `src/game/CultivationScene.h`、`src/game/CultivationScene.cpp`、`src/game/Application.cpp`（只改 `openFacility` 的 `meditate` 分支）
- `src/core/rules/Realm.h`、`src/core/rules/Realm.cpp`（只在 E2 要改数时动；不改就只加一行注释）
- `tests/Ch08EngineTests.cpp`（新）；`tests/RealmTests.cpp`（只按第 2 节）；`tests/PanelTests.cpp`（只在扫描需要时追加断言）
- `tools/validate.py`（**只** 5.1 第 1 条）、`tools/validate_selftest.py`（只补 5.1 第 2 条）——协调者在集成时（引擎路交付之后，不与引擎路并行）再按 5.2 改这两份
- 仓库根 `docs/map_spec.md`（4.6 表加 `effectiveness` 一行）
- `docs/interfaces-p3-ch08.md`（只填 1.6、2.4 与末尾「实现记录」）

### 6.2 内容路（另一份派工单，写在 `docs/ch08-design.md` 第 18 节头部；这里只列边界）

- `maps/ch08_{dongfu,tianxing_fangshi,yanlingbao,lingkuang,jinguyuan,jinmacheng,yuejing,jiayuan_shanlin}.tmj`（新 8 张）、`maps/tilesets/terrain_{dongfu,tianxing_fangshi,yanlingbao,lingkuang,jinguyuan,jinmacheng,yuejing,jiayuan_shanlin}.tsj`（新）
- `maps/ch05_nancheng.tmj`、`maps/ch05_mofu.tmj`、`maps/ch06_huangfenggu.tmj`、`maps/ch06_baiyaoyuan.tmj`、`maps/ch06_tainan_cun.tmj`（**只经** `genmaps_ch08.patch_*()` 改：只加对象、只给五个已有 NPC 补 `hidden_flag`）
- `tools/mapgen/genmaps_ch08.py`（新）、`tools/mapgen/genmaps.py`（只加登记与五处 patch 调用，排在第 5、6、7 章之后）
- `scripts/ch08/**`
- `data/text/ch08*.json`（新）、`data/text/shops.json`（只加四条 nameKey）
- `data/roles/*.json`（新建 63 个；**不改任何已有 role**——`wang_chan`、`guilingmen_zhanglao`、`hehuanzong_xiushi`、`heishajiao_shigui`、`li_huayuan`、`chen_shimei`、`ma_shibo`、`lin_shidi` 一律不动）
- `data/items/**`（新建 37 件；`pills/dingyan_dan.json` 只改 `tradeable`）
- `data/magics/*.json`（新：我方 5 门、道具专用 2 门、敌方与友军约 10 门）
- `data/recipes/alchemy/{lianqi_san,juling_dan}.json`（新）
- `data/battles/b08_{guiling_shaozhu,lingkuang_shouzhen,zhongrudong,yuehuang,quhun,songmeng_qiecuo}.json`（新 6）、`data/battles/b08_{jinguyuan_juezhan,jinguyuan_qianfeng}.json`（改写 2）、`data/battles/be08_huangyuan_xiyi.json`（新）
- `data/encounters/ch08_lingkuang.json`、`data/shops/ch08_{tianxing_fangshi,tianxing_yaopu,yanlingbao,jinguyuan_jiaoyisuo}.json`、`data/quests/q08_{liesha,xiaojia}.json`、`data/pathactions/ch08.json`、`data/objectives/ch08.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`tools/artgen/sprites_enemies.py`（只加 5 个非人形：双瞳鼠、血鬼、血蜘蛛、傀儡兽、荒原蜥蜴）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加 8 条登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch08-design.md`（**只追加第 18 节「施工偏差」**，平衡路的标定表也写在这里）

### 6.3 协调者

- `tools/validate.py`、`tools/validate_selftest.py`（**只** 5.2 那几处，引擎路交付之后）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第八章」「魔道入侵」）；`data/text/battles.json`（只删三条旧 intro）；`data/battles/b08_jinguyuan_weigong.json`（删）
- `tests/LexiconTests.cpp`（表一补词）
- `docs/handoff.md`、`docs/tech-debt.md`（登「制符方换旗标改到第一次拿到妖丹那一章」一笔，16.2 第 2 条）；仓库根 `docs/大纲.md`、`docs/lore/**`（回写，`docs/ch08-design.md` 16.1 第 1 条）

### 6.4 测试路（两路都落地、协调者 5.2 之后另派）

- `tests/Ch08*.cpp`（除 `Ch08EngineTests.cpp`）：通关、挂点、编成、账、切片、验收
- `tests/ObjectiveTests.cpp`（`kScriptTransfers` 二十条）
- `tests/NpcPresenceTests.cpp`
- `tests/PathActionTests.cpp`
- `tests/Ch05*.cpp`、`tests/Ch06TriggerModeTests.cpp`、`tests/Ch06AcceptanceTests.cpp`（只按 5.3 那几行改）
- `tests/ChapterFixture.h`、`tests/fixtures/ch08-end-{first,second}.sav`、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记主线旗标 → 引擎路（E1 ＋ 5.1，E2 等平衡路的数）∥ 内容路 → 协调者 5.2 与集成 → 测试路 → 独立校对。**本章在第 7 章集成之后开工**（起点 fixture `ch07-end-*.sav` 要先存在，`docs/ch08-design.md` 第 2 节全是「估」）。
- 每路用自己的构建槽：引擎路 `ch08eng`，内容路 `ch08content`，测试路 `ch08test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 几路共用一棵工作树时，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实，不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- 门禁先于编译跑：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`。
- **旧图只经 patch**：第 5、6 章的五张图，本章只加对象、只补那五个 `hidden_flag`；`genmaps_ch05.py`、`genmaps_ch06.py` 一个字不改（`docs/ch08-design.md` 16.2 第 4 条若被协调者改判，照改判办）。

---

## 实现记录

引擎路 · 2026-09-29 · 构建槽 `ch08eng` · WT 基线 `21e3bd8`（47c36c0 ＋ 第 7 章合并后的快照）。E1 的细节在 1.6，E2 在 2.4。

**改了的文件**

- `src/game/CultivationScene.h/.cpp`；`src/game/Application.cpp`（只 `openFacility` 的 meditate 分支）；`src/core/rules/Realm.h`（只加 E2 那段注释）。
- `tests/Ch08EngineTests.cpp`（新，6 条）。
- `tools/validate.py`、`tools/validate_selftest.py`（5.1，下面）。
- 仓库根 `docs/map_spec.md`（4.6 表一行）；本文件 1.6、2.4 与这一节。

**门禁（5.1）**

1. `validate.py` 新 `check_facility_effectiveness`，在 `check_map` 的对象循环里调（`--maps` 这一档就查）：带 `effectiveness` 的对象不是 `kind=meditate` 的设施 → 报「只许写在 kind=meditate 的设施上」；Tiled 原始属性不是 `"type": "int"` 的整数 → 报「必须是整数」（类型只能看原始属性：`object_properties` 把值一律转成字符串，"125" 与 125 在那里分不出来）；不在 100–300 → 报「超出 100–300」。三条各报各的；类型不对时不再判取值。常量 `EFFECTIVENESS_PROP`、`EFFECTIVENESS_RANGE`。
2. `validate_selftest.py` 新 U 节（接在 T 之后，文档串同步）：设施现找（副本里第一处 meditate、第一处 field，不写死图名），每条都是写坏 → 跑 `validate.main()` → 还原，并问「报的恰好是这一条」：正例 int 125 / 100 / 300 不报（后两个钉住闭区间）；负例写在 `kind=field` 上只报位置、字符串 "125" 只报类型、90 与 400 只报取值。selftest **213 → 220** 条。`CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 与「第 N 章不在规则 25 范围」那条探针没动（5.2）。

**负向自检**（C++ 改坏在隔离副本 `scratchpad/mut08` 里做——WT 与内容路共用，不在那里改坏源码；驱动 `scratchpad/eng08/mutate08.py`：改坏 → 编 `fanren_tests` → 跑 `Ch08*` → 还原，并核对红的恰是预期那几条、其余全绿）

- 对照：不改动的副本 6 条全绿。
- A（一次编译）：`openFacility` 不读 `effectiveness`（M1）→ 第 3 条红（「灵泉 36 点，蒲团 36 点」）；`cultivationPanelStrings` 漏两个新字段（M5）→ 第 4 条红。
- B：`meditateFor` 改回三参（M2）→ 第 3 条红；`settleDailyPractice` 按 125 结（M6）→ 第 5 条两条都红（12 ≠ 9）。
- C：`bankMeditation` 去掉 `sitePercent` 的乘法（M3）→ 第 1 条红（32 颗种子灵泉都等于蒲团）、第 3 条红。
- D：`siteBonusLine` 恒返回空串（M4）→ 第 4 条与第 3 条（那一行）红。
- E：`bankMeditation` 的缺省参数改成 125（M8）→ 第 2 条红，第 5 条两条也红（日课走缺省）。
- 门禁（驱动 `scratchpad/eng08/gatemut08.py`，改的是副本的 `validate.py`）：关掉位置那条 → U 的 field 那一条 `[FAIL]`；类型改成按字符串解析（照 `object_properties` 的口径）→ "125" 那一条 `[FAIL]`；关掉取值 → 90、400 两条 `[FAIL]`；`check_map` 不接这条检查 → 四条负例全 `[FAIL]`，全量 selftest「220 条，未如期抓住 4 条」`SELFTEST_FAIL`；取值改成开区间 → 正例 100、300 两条 `[FAIL]`。还原后全量 220 条全过。

**全量**：WT 槽 `ch08eng`，`build_logged.bat` 四道门禁全过（VALIDATE_OK / MAPGEN_IN_SYNC / SELFTEST_OK / ARTGEN_IN_SYNC），零编译警告；**1630 条（基线 1624 ＋ 本路 6），1624 过、6 条红**——`Ch07Walkthrough` ×2、`Ch07Acceptance.No6`、`Ch07IntentHand` ×2、`Ch07Ledger.TheMarketRoadNeverPaysMoreForOneHerbThanTheWholeChapterCap`，已知、非本路（开工时同一槽的基线就是这 6 条：1624 条、1618 过）。

**没做的 / 已知**

- E2 没交付完：数没给，注释写了、「暂定」还在；锚点同（2.4）。交协调者。
- 第 3 条用的是测试图，不是 `ch08_dongfu` 的 `facility_lingquan`（内容路还没落地）。
- 面板那一行只在 `renderStatus` 里画，测试问的是 `siteBonusLine`：无头绘制是空操作，「`renderStatus` 漏画」这一种坏法没有用例咬得住（与 `spiritRootLine` 同一个缺口）。
- O1–O3 不做（契约）。
