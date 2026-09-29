# P3 第 6 章引擎增补契约

> 本文是**契约**：引擎方照它实现，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-09-29 · 依据 `docs/ch06-design.md` 第 10 节（引擎前置）、第 7 节（系统解锁）、第 11 节（旗标）、第 12 节（禁词）。
> 本批引擎路做 E1（天眼术「看破」）、E3（修炼面板灵根一行）、E4 的代码注释部分、O1（背包标题「储物袋」）、以及 E1 需要的门禁形状检查。
> E2（符箓进战斗）**不做**（用户拍板，放第 7 章）。

---

## 0. 四件事，各自现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | 一门施展后「揭开本场所有敌人破绽」的辅助法术，菜单上点得动 | `core::Magic` 只有 `power / poison / boost`；`offensiveMagic()`（`src/core/model/Types.h`）把 `power 0` 的法术挡在菜单外，理由「不是伤人的法术」；破绽的「已揭开」记在 `GameState::knownWeaknesses`（`docs/octopath-battle.md` 2.5） |
| E3 | 修炼面板一行「灵根　四属性缺金·伪灵根」 | `CultivationLexicon`（`src/game/CultivationScene.h`）有 `aptitudeLabel`「资质」一行，没有灵根；`state.aptitude` 恒 50 |
| E4 | `story.xiuxian_known` 改由第 6 章置 | 数据描述已改（协调者）；`src/game/Wording.h` 与 `src/game/MenuScene.h` 的注释还写着「第 1–5 章都不置（G-13）」这类过时的话 |
| O1 | 背包面板标题按 `ch06.chuwudai` 换「储物袋」 | 主菜单「物品」那一栏与背包面板标题都是写死的 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。E1 三段都在本批里落。

---

## 1. E1 天眼术「看破」

### 1.1 一句话口径

**天眼术是一门 `effect: "reveal"` 的法术：施展一次，本场所有敌方单位的破绽全部变成「已揭开」，并写进 `knownWeaknesses`（跨战斗记住）；它不造成伤害、不削架势、不吃蓄劲。**

### 1.2 数据：`data/magics/tianyan_shu.json`（引擎路建，白名单内）

```json
{
  "id": "magic_tianyan_shu",
  "name": "天眼术",
  "descKey": "magic.desc.magic_tianyan_shu",
  "element": 0,
  "needMp": 5,
  "power": 0,
  "needRealm": "QiRefining1",
  "effect": "reveal",
  "origin": "原著",
  "note": "……"
}
```

- 新字段 **`effect`**：字符串，只认 `"reveal"`；缺省 = 没有效果（现有全部法术不写，行为零变化）。写了别的值（含空串）**加载期报错**，报错点名文件与字段（照 `boost` 那一条的写法，`src/io/DataLoader.cpp`）。
- `effect` 与 `power > 0` / `poison > 0` **不许同时出现**：一门法术要么伤人要么看破，写了两样加载期报错（免得有人给火弹术挂个 reveal 变成「打一下顺便全看穿」）。
- `descKey` 的文案 `magic.desc.magic_tianyan_shu` 进 `data/text/ch06_items.json`（内容路写；引擎路的测试里不要求它存在——门禁的通用 key 检查会在内容落地后管着）。
- `note` 里写清：ch67 录、ch68 / 88 / 96 施展过、第 4、5 章按拍板没演、ch126 第一次真正派上用场（看万小山）。

### 1.3 规则层（`src/core/model/Types.h`、`src/core/battle/`）

- `Magic` 追加在末尾：`enum class MagicEffect { None, Reveal }; MagicEffect effect = MagicEffect::None;`。
- 新判据 **`castableMagic(const Magic&)`** = `offensiveMagic(m) || m.effect != MagicEffect::None`。**菜单「能不能点」改用它**；`offensiveMagic` 原样保留，凡是「伤害 / 下毒」的分支照旧问它。
  护身罡（`power 0`、无 effect）照旧点不动，理由那一句「不是伤人的法术」不变。
- 施展 `Reveal`：
  - 扣法力 `needMp`；
  - 对本场**每一个存活的敌方单位**：其 `weaknesses` 全部标为已揭开（与「打中破绽揭开」是同一份状态，`docs/octopath-battle.md` 2.5 那条路），并写入 `GameState::knownWeaknesses[role_id]`；
  - 不算一次攻击：不判破绽、不削架势、不触发首领蓄势被打断、不产生伤害数字；
  - 事件流里出一条能让界面播「金光一闪、破绽图标全亮」的事件（`BattleState::events()`，事件种类由引擎路定，写进本文 1.6）；
  - **蓄劲对它无效**：蓄了劲也只当没蓄（劲不扣、不退还——与「蓄劲后防御」同一口径，若现有规则是蓄劲后防御会退劲，则跟它一致；以现有规则为准并在 1.6 写明）。
- AI：`BattleAi.cpp` 的我方 AI（无头 `runToCompletion` 用）**不用它**；敌方 AI 本来就不会拿到它（敌人法术表里不写）。

### 1.4 加载器（`src/io/DataLoader.cpp`）

- 读 `effect`，口径见 1.2；两条负向（拼错、与 power 同写）各一条用例。

### 1.5 游戏层（`src/game/BattleScene.cpp` / `BattleView` / `BattleHud`）

- 命令菜单：`castableMagic` 放行；置灰理由不变。
- 播放：一条短特效（复用现有「刀光 / 气焰」那一层的任一效果即可，不新画资源）＋ 所有敌方破绽图标当场全亮（现有「已揭开」的画法）。
- 截图口不改。

### 1.6 实现记录（引擎路交付时填）

引擎路 · 2026-09-29 · 构建槽 `ch06eng`。

- **事件种类 `BattleEventKind::Reveal`**（追加在枚举末尾）：一手看破 = 一条 `Act`（actor = 施法者，target = -1，value = `Cast`，category 0，hits 1）＋ 本场每一个**站着的敌人**各一条 `Reveal`（actor → target，`revealed` = 这一下新揭开的，`value` = 揭开之后它的全部已揭开 = 它的全部破绽），按单位下标序。没有 `Hit` / `Break` / `ChargeInterrupt` / `BoostSpent`。界面播法：`Act` 照旧（施法光）；每条 `Reveal` 在那个敌人身上一团金光（`FxSheetId::Glow` + `Spark`，染 `kGoldGlow`）＋ 破绽图标用「打中揭开」那一套闪亮（`hud.reveal` / `revealBits`），显示用的 `shown_` 同 id 同边一起亮。
- **不挑目标**：`checkCast` 对带 effect 的法术不查 `targetIndex`；菜单上点中天眼术**直接施展**，不经过选目标那一级（`BattleScene::menuChoose`）。
- **蓄劲口径**：现有规则里「蓄了劲再防御」是界面按 0 发（`checkBoost` 对防御的非 0 蓄劲是回绝的，不存在「退劲」）。看破跟它一致而更宽：规则层 `checkBoost` 对看破的非 0 蓄劲**放行不拦**，`applyReveal` **不扣劲、不置 `boosted`**（下一回合照常加劲）；界面点天眼术时也按 0 发。即「蓄了也只当没蓄，劲不扣、不退还」。
- **`knownWeaknesses`**：施展时改的是 `Unit::revealed`（与「打中揭开」同一份，同 id 同边一起，含还没进场的下一波同 id 单位；不同 id 的后续波不管），收场时由既有的 `BattleScene::finish` 并进 `GameState::knownWeaknesses`（输了、逃了也记；韩立不在场的仗不写）。没有在施展那一刻另写一份存档状态。
- **日志**：「韩立 施展【天眼术】，X 的破绽「剑、火」尽收眼底；Y 的……」；一样破绽都没有时「，场上没有看得出的破绽」。
- **AI**：`BattleAi.cpp` 敌我两个候选循环都先问 `offensiveMagic`，不伤人的法术不进候选。
- **菜单细节**：法术列表那一行对带 effect 的法术不写「蓄劲加威 / 蓄劲连发」。
- **加载器**：`effect` 只认 `"reveal"`，别的（含空串、非字符串）报「法术 "id" 的 effect 只许 "reveal": 路径」；与 `power > 0` / `poison > 0` 同写报「…effect 不能与 power > 0 或 poison > 0 同写…: 路径」。`power` 缺省是 10，所以看破的法术必须明写 `"power": 0`。
- **改的文件**：`src/core/model/Types.h`（`MagicEffect`、`Magic::effect`、`castableMagic`）、`src/core/battle/Battle.h` / `BattleAction.cpp`（`Reveal` 事件、`applyReveal`、`checkCast` / `checkBoost`）、`BattleAi.cpp`、`src/io/DataLoader.cpp`、`src/game/BattleScene.cpp`、`BattleView.h/.cpp`（`onReveal`）；`data/magics/tianyan_shu.json`（新）；`tools/validate.py`（规则 24 法术那一半）、`tools/validate_selftest.py`（N 节两条负例，`BREAK_RULE_WORDS` 加 `effect`）；`docs/octopath-battle.md` 5.2。E3 / E4 / O1 见各节与下方。
- **测试**：`tests/Ch06EngineTests.cpp`（新，12 条）：`Ch06RevealData` ×3（读得进 / 缺省 None；`"reveal "` `"Reveal"` `""` 各报错且点名文件与字段；与 power 12、与 poison 同写报错）、`Ch06Reveal` ×5（两敌四样全揭开、法力 30→25、气血架势不动、事件序列；下一波同 id 一起亮；菜单三行：天眼术可点、护身罡不可点且理由原句、火弹术照旧；蓄 3 点劲与不蓄相同、劲不扣、不置 boosted；敌方 AI 不拿它）、`Ch06RevealScene` ×2（`b04_qiecuo_maliu` 走真菜单点天眼术 → 两个师兄破绽全亮、法力少 5 → 收场 `knownWeaknesses` 两条各两样 → 存档读档 → 同一场再开开局全亮；数据文件与 1.2 一致）、`Ch06SpiritRoot` ×2；`tests/GameWiringTests.cpp` 追加 `TheBagIsCalledTheStorageBagOnceHeHasOne`。
- **负向自检**（改坏 → 构建 → 确实红 → 还原）：A 组一次构建同时改坏六处——`castableMagic` 改回 `offensiveMagic`（1.7 第 6 条：菜单那条红，理由「【天眼术】不是伤人的法术」）、加载器不查拼写、不查同写、灵根不看 `ch06.linggen`、凡人表灵根标签填「灵根　」（`PanelWording` 凡人扫描红）、`bagWord` 恒为「物品」，各自对应的用例全红；B 组——`applyReveal` 扣劲、敌方 AI 去掉 `offensiveMagic` 过滤、只揭开最低一位，对应用例全红；门禁：把 `validate.py` 的 effect 检查关掉，selftest 那两条 `[FAIL]`。还原后重新构建复核全绿。
- **没做的 / 已知**：我方 AI 那一处过滤没有能咬住的用例（我方 AI 的第 4 步「普攻优先」本来就轮不到一门伤害 1 的法术，去掉过滤行为也不变）；`descKey` 的文案 `magic.desc.magic_tianyan_shu` 归内容路，落地之前 `validate.py` 报「文案 key 不存在」一条、selftest 三条「不动的副本」对照跟着红（`build.bat` 因此停在门禁；把本文件挪开时 VALIDATE_OK、SELFTEST_OK 193 条），这是 1.2 定下的次序；看破的「揭开」在施展那一刻只进 `Unit::revealed`，中途强退游戏的话这一场的看破不进存档（与「打中揭开」一样）。
- **E3**：`CultivationLexicon` 末尾加 `spiritRootLabel` / `spiritRootValue`（凡人表两个空串，修仙表「灵根　」「四属性缺金·伪灵根」），两条都进了 `cultivationPanelStrings`（`PanelWording` 照扫）；`CultivationScene::spiritRootLine(state)`（公开静态，`renderStatus` 画的就是它）只在修仙阶段且 `ch06.linggen` 已置时非空，画在「资质」下一行；旗标名只在 `CultivationScene.cpp` 的 `kSpiritRootFlag`。
- **E4**：`Wording.h`、`MenuScene.h` 两处注释照第 3 节改，行为未动。
- **O1**：见 4.1。

### 1.7 判据（`tests/`，引擎路写；文件名建议 `tests/Ch06EngineTests.cpp`，不动 `tests/BattleTests.cpp` 之外的既有文件）

1. 数据：`effect: "reveal"` 读得进来；`"reveal "`（带空格）、`"Reveal"`、`""`、与 `power 12` 同写——四条各报错、报错文本点名文件与字段。
2. 规则：一场两个敌人各两样破绽、开局零揭开；施展天眼术后四样全揭开、`knownWeaknesses` 两条各两样；法力少 5；两个敌人架势与气血一点不动。
3. 菜单：天眼术可点；护身罡不可点且理由是原来那一句；火弹术照旧。
4. 蓄劲：蓄 3 点劲再放天眼术，效果与不蓄相同（1.3 定的口径）。
5. 跨战斗：一场里放过天眼术，存档 → 读档 → 下一场同一个 role 的破绽开局就是已揭开（2.5 那条路已有的用例照抄一份）。
6. **负向自检**：把 `castableMagic` 的实现临时改回 `offensiveMagic`，第 3 条必红。

---

## 2. E3 修炼面板「灵根」一行

- `CultivationLexicon` 追加在末尾（两张表按位置初始化，照 G-14 那次的做法）：`spiritRootLabel`（修仙表「灵根　」；凡人表空串）与 `spiritRootValue`（修仙表「四属性缺金·伪灵根」；凡人表空串）。
- 显示条件：`wordingStage == Immortal` **且** `state.flag("ch06.linggen") != 0`。两者缺一不显示这一行（第 6 章前半段他已进修仙界但还没测过）。
- 位置：状态栏「资质」那一行的下一行。**「资质」那一行不动**（`aptitude` 仍是 50，它是修炼速度的输入，与灵根是两件事，`Cultivation.cpp` 的注释早写了伪灵根是这个数的来路）。
- 旗标名写死在 `CultivationScene.cpp` 一处常量 `kSpiritRootFlag = "ch06.linggen"`，注释指向本文。
- 判据（`tests/PanelTests.cpp` 追加，或放进 `Ch06EngineTests.cpp`）：凡人阶段不含「灵根」；修仙阶段、旗标 0 不含；修仙阶段、旗标 1 含「四属性缺金·伪灵根」；面板禁词用例（`PanelWording`）对新加的两条词照扫（「灵根」是凡人篇禁词，凡人表必须是空串）。

---

## 3. E4 `story.xiuxian_known`：只改注释

- `src/game/Wording.h` 顶部那段「第 1–5 章都不置（技术债 G-13）」改成「第 1–5 章不置，第 6 章太南谷听完灵根之说时置（`scripts/ch06/guaipo.lua`；G-13 已按第 6 章拍板收口）」；`MenuScene.h` 同。
- **不改任何行为。** `data/flags.json` 的描述协调者已改。

---

## 4. O1 背包标题「储物袋」（可选，做了要写实现记录）

- 主菜单「物品」一栏与背包面板标题：`state.flag("ch06.chuwudai") != 0` 时显示「储物袋」，否则照旧「物品」。走 `Wording.h` 加一条 `bagWord(const GameState&)`，旗标名只在这一处。
- 判据：`GameWiringTests` 或 `UiTests` 加一条：旗标 0 / 1 两种字面。
- 文案若走 `ui.json`（`ui.menu.items`），需要加一条 `ui.menu.items.bag`——**`data/text/ui.json` 是协调者的文件**，要加的 key 与文案写进本文 4.1 交协调者落地。

### 4.1 交协调者的文案

**无需改 `ui.json`。** O1 做了，没走 `ui.json`：照 `magicWord` 的先例，「储物袋」写在 `src/game/Wording.h` 的 `bagWord(const GameState&)`（旗标名 `kChuwudaiFlag = "ch06.chuwudai"` 只在这一处；未置时返回「物品」，与 `ui.menu.items` 同词，`GameWiringTests` 的 `TheBagIsCalledTheStorageBagOnceHeHasOne` 钉着）。主菜单左栏与右栏页眉都改取新的 `MenuScene::pageLabel(data, state, page)`，物品那一栏走 `bagWord`，其余栏照旧取 `ui.json`；按阶段取词的 `pageLabel(data, stage, page)` 与 `menuStrings` 未动。
若协调者更愿意把文案收进 `ui.json`：加 `"ui.menu.items.bag": "储物袋"`，并把 `bagWord` 换成取这个 key——两处改动，测试不变。

---

## 5. 门禁（`tools/validate.py` 规则 24 的法术那一半，引擎路改）

- `effect` 写了就得是 `"reveal"`；与 `power > 0` / `poison > 0` 同写报错。`tools/validate_selftest.py` 补两条负例（写坏 → 必报 → 还原）。
- **`CHAPTER_MEANS[6]` 与 `MEANS_LAST_CHAPTER = 6` 不在本批**：那两行要等内容路的脚本落地才不会红（规则 25 的来路检查会去 `scripts/ch06/` 找 `magic.learn`），由协调者在集成时加。引擎路**不要动** `CHAPTER_MEANS` 那一段。

---

## 6. 白名单

### 6.1 引擎路

- `src/**`（**不含** `src/game/AlchemyScene.*`、`ShopScene.*`、`FieldScene.*`——本章不动它们）
- `tests/Ch06EngineTests.cpp`（新）、`tests/PanelTests.cpp`、`tests/BattleTests.cpp`、`tests/GameWiringTests.cpp`、`tests/UiTests.cpp`（只追加）
- `data/magics/tianyan_shu.json`（新）
- `tools/validate.py`（**只**规则 24 法术那一半）、`tools/validate_selftest.py`（只补对应负例）
- `docs/interfaces-p3-ch06.md`（只填 1.6、4.1 与末尾「实现记录」）、`docs/octopath-battle.md`（5.2 那张表加 `effect` 一行）

### 6.2 内容路（另一份派工单，写在 `docs/ch06-design.md` 第 18 节头部；这里只列边界）

- `maps/ch06_*.tmj`、`maps/tilesets/terrain_{tainan_cun,tainan_gu,sanxiu_lou,shanqiu,huangfenggu,baiyaoyuan}.tsj`、`maps/ch05_nancheng.tmj`（只补东门）
- `tools/mapgen/genmaps_ch06.py`（新）、`tools/mapgen/genmaps.py`（只加登记两处）
- `scripts/ch06/**`
- `data/text/ch06*.json`（新）、`data/text/shops.json`（只加一条 nameKey）
- `data/roles/*.json`（新建；已有的只改 `huangfenggu_waimen_dizi.json` 与 `jiexiu.json`——后者若不再有人引用，删）
- `data/items/**`（新建）、`data/magics/liusha_shu.json`、`data/magics/bingdong_shu.json`（新）、`data/magics/ji_jianfu.json`（只改 note）
- `data/battles/b06_*.json`、`data/encounters/ch06_shanqiu.json`、`data/shops/ch06_tainan_fangshi.json`、`data/recipes/talisman/dingshen_fu.json`、`data/quests/q06_*.json`、`data/pathactions/ch06.json`、`data/objectives/ch06.json`
- `data/visual/maps.json`、`data/visual/looks.json`、`data/visual/battles.json`（只加条目）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` 加登记；本章不新做曲子）、`docs/audio.md`（同一张表）
- `docs/ch06-design.md`（**只追加第 18 节「施工偏差」**）

### 6.3 协调者

- `data/flags.json`（`ch06.path.*` 十一条待内容路的 `data/pathactions/ch06.json` 落地后登记；主线旗标已登记）、`data/chapters.json`、`data/text/ui.json`、`tests/LexiconTests.cpp`、`tools/validate.py` 的 `CHAPTER_MEANS` / `MEANS_LAST_CHAPTER`、`docs/handoff.md`、大纲与 lore。

### 6.4 测试路（两路都落地之后另派）

- `tests/Ch06*.cpp`（除 `Ch06EngineTests.cpp`）、`tests/ObjectiveTests.cpp`（`kScriptTransfers` 三条）、`tests/NpcPresenceTests.cpp`（群像白名单两条）、`tests/ChapterFixture.h`、`tests/fixtures/ch06-end-*.sav`、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- 每路用自己的构建槽：引擎路 `ch06eng`，内容路 `ch06content`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- **不派子代理**，不整份重读大文件，报告精炼。
- 原著文本只在本地核对事实，不复制、不进仓库；对白字句自己写，换词不算重写。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- 门禁先于编译跑：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`。
