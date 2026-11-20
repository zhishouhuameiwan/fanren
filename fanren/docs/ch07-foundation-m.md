# 第 7 章可玩性改版·第一期「地基包」路 M：地图与说明（2026-11-20）

> 施工图：`docs/ch07-foundation.md` 第 3 节（主检出里的上位文档，本工作树里没有）。同一期另外两路并行：路 G（指引）、
> 路 D（对白减负），各在自己的工作树里；三支由集成会话合并。
> 分支 `claude/ch07-foundation-m-377c44`，构建目录 `fanren/build-m`（`run.bat` / `test.bat`，照抄 `build-s7` 的两份改了路径）。

---

## 1. 用户原话与本路的三件事

用户 2026-11-20 第二轮试玩第 7 章：

> 还是感觉可玩性不高，各种信息指引不够，物品说明也不够，对话又很多

本路管其中两样：**我在哪、门在哪、还有什么没拿**（小地图、区域地图），**这东西有什么用、哪儿来的**（说明四行式）。

| 件 | 落点 |
| --- | --- |
| 小地图 | `game/Minimap.*`（换算、取景、标记、底图）、`WorldView::drawMinimap`、`WorldFrame::minimap / objects / heroFacing / minimapKey` |
| 区域地图 | `game/AreaMapScene.*`；M 键 / 手柄 LB（`Engine::Key::Map`）、主菜单「地图」 |
| 说明四行式 | `core/rules/ItemInfo.*`（纯规则）、`game/ItemInfoView.*`（扫图建来源索引、画说明块）；五处界面换上 |

---

## 2. 小地图（施工图 3.1）

| 项 | 做法 |
| --- | --- |
| 位置 | 右上角 200×150（`minimap::panelRect`：x 1064、y 12）。墨金面板，图缩进 5 像素。右下角一小行「M 地图」（`ui.map.minimap.key`，跟着键位与设备走：手柄是「LB 地图」） |
| 底图 | 有 `assets/art/maps_hd/<图>/below.png` 且尺寸 = 格数 × 24 的：用渲染目标逐级减半缩成 12 / 6 / 3 像素一格（线性取样，一次缩一半就是 2×2 取平均，不起摩尔纹），按要画的大小挑一档。没有的：通行格画剪影（一格一像素，走得通旧纸色 `0xC8BC98E6`、走不通墨色 `0x262A32D2`，按像素放大）。换图才重建；无头时建不出渲染目标，什么也不画 |
| 取景 | 整张缩进来一格不小于 3 像素就整张居中；否则一格 3 像素、跟着主角开窗，夹住不越过图边（`minimap::fit`）。第 7 章最大的图 48×40，整张缩进来一格约 3.8 像素，都是整张 |
| 标记 | 主角：带朝向的白三角，最后画、压在最上面。主线目标：金菱形，目标在本图就是那个对象，在别的图就标在通往它的那道门上；出了取景贴边。门：青色方块；通往目标的那一道金色；`require_flag` 没置的灰色。存档点：蓝菱形。其余设施（药田、打坐、炼丹、制符、炼器、商店、告示、历练入口）：青绿方块。宝箱：橙色方块，**没开的只在主角 8 格以内画**、开过的一直画成灰的。采集点：绿点，同上（「采过」看 `gatheredDay` 里有没有它，与收集进度同一个口径）。明雷：红点，只画 8 格以内的 |
| 判据 | 一个都不另写：门开没开、哪道门通往目标、宝箱开没开、明雷站在哪（巡逻中的此刻那一格），全取 `WorldScene::describe` 判好的 `WorldFrame`，与画面、走位同一个答案。距离按切比雪夫（横竖斜都算一步） |
| 何时不画 | 设置「小地图」关着；世界层不在栈顶（对白、选项、主菜单、商店、区域地图、路径行动面板压着）；脚本在跑（过场）。盖住主角时淡成 0.35 |
| 让位 | 存盘提示挪到它下面（`minimap::kSaveNoticeTop` = 168；小地图关着时照旧 16）。得物卡片从 132 起改为 172 起、间隔 8 → 6，六张排到 562 仍压不到对话框（`ExplorationTests` 那条「六张也压不到底下的对话框」原样过）。地名横幅、目标框在左上，不重叠 |
| 存档 | 不做战争迷雾、不记「去过哪儿」：**不加任何存档字段**，`kSaveVersion` 仍是 10 |
| 设置 | `io::Settings::minimap`（文件键 `minimap`，缺省开），设置面板「游玩」组「手柄震动」之后插一行「小地图」：`SettingsScene::kMinimap = 9`，「按键设置」「恢复默认」各往后挪一行（`kKeys 10 / kRestore 11 / kRowCount 12`），面板高 622 → 652、仍上下居中 |

---

## 3. 区域地图（施工图 3.2）

| 项 | 做法 |
| --- | --- |
| 打开 | 世界层按 **M**（手柄 **LB**），与主菜单同一个口径：只在世界层、脚本没在跑时受理。主菜单左栏「记事」之后加一项「地图」（`MenuScene::Page::Map`，追加在枚举末尾不挪别的值）：光标停在上面时右栏写一句说明（含地图键），确认 → 菜单合上、区域地图打开 |
| 关上 | M / LB、Esc / B、Tab / Y、确认，都关 |
| 画面 | 全屏一块面板，标题是地名。左边 860×560 的大图：底图与标记同小地图（显示规矩也相同），存档点与设施旁边写名字（存档、打坐、商店、药田……）；一格至少 8、至多 32 像素，放不下时开窗，方向键一下平移 3 格。右栏：凶险星级（同地名横幅那一行，没有遭遇的图写「凶险　无」）、本图收集进度（同目标框下那一行；没有可数的写「这里没有宝箱和采集点」，历练里写「历练里不计收集」）、图例（12 种标记）、一句「没开的宝箱、没采的采集点和妖兽，走近八步之内才标出来」、出口列表（每道门通到哪张图，同一张图并成一行；通往主线目标的那一道金字打头「◆」；走不通的灰字接「（还走不通）」） |
| 数据 | 全从世界层这一帧拿（`Application::worldScene()->describeFrame`），出口表是纯函数 `AreaMapScene::exitsOf` |
| 不做 | 世界地图、传送（施工图 3.2、第 6 节） |

### 键位

| 项 | 改动 |
| --- | --- |
| 引擎 | `Engine::Key` 在 `Action` 之后、`Count` 之前追加 `Map`（Save、Action 当年的规矩：已有键的编号一个不动）。默认自定义键位 M / 空；手柄 LB（`kPadKeys` 16 → 17 条；LB 原先空着）。动作 id `map` |
| 设置文件 | `io::kKeyActionIds` 10 → 11 个词，与 `Engine::keyId` 同一组 |
| 改键面板 | 自动多一行「地图」（`KeyConfigScene::kActionRows` 跟着 `Key::Count` 走），手柄列写「LB」；面板高 580 → 610、仍上下居中 |
| 提示 | `ui.map.hint`、`ui.map.hint.pan`、`ui.map.minimap.key`、`ui.menu.map.desc` 带 `{key.map}`，各有 `.pad` 版带 `{pad.map}`；`KeyPrompts` 两张白名单各加这四条 |
| 文档 | `docs/settings.md` 第 1 节表、第 6 节表与说明；`docs/gamepad.md` 第 2 节表、第 6 节表 |

路 D 也要挑空闲键（自动播放、整段跳过）。本路占用的键写在共享笔记 `route-m-keys.md`：键盘 M、手柄 LB；路 D 若也往 `Key` 枚举追加，排在 `Map` 之后。

---

## 4. 物品与法术说明四行式（施工图 3.3）

### 4.1 规则（`core/rules/ItemInfo.*`，纯函数）

`rules::describeItem(data, sources, state, words, itemId, herbAge)` → 四类行，**全由数据生成**：

| 行 | 内容 |
| --- | --- |
| 效果 | 回复气血 N、回复法力 N、解毒、中毒 N 回合每回合掉血 M；符箓「施展「火弹术」 · 火 · 威力 12」（看破、削架势 N 点、打敌方全体照写）与「不耗法力，不能蓄劲」；兵刃「普攻多一样「剑」」；灵草「这一株 44 年，至多长到 44 年」（不知道哪一堆时只写上限） |
| 用途 | 「入「回气丹」要 1 份（炼丹），须 20 年以上」，按配方逐张列（只列已得到的方子：`requireFlag` 已置或没有门闸）；「战斗里用得上」（`battleUsable`）；名册上的伙伴谁喜欢、谁不喜（`Companion::giftsLike / giftsDislike`，只列已结识的） |
| 来源 | 「「北街收药摊」有售」「照「回气丹方」炼得」「「恶狼」掉落」「「禁地外围」可采」「「地下沼泽」、「环形山」、「禁地外围」等 6 处的宝箱」「「X」寻遍的酬劳」（集齐奖励）「「支线名」的酬谢」（`Quest::rewards`）；都没有写「剧情所得」 |
| 品阶 | 「良品（2 阶） · 基准价 500 灵石」；`tradeable` 为假写「不可买卖」。品阶名五档与得物卡片的颜色档同一个分法：0–1 凡品、2–3 良品、4–5 上品、6–7 珍品、8 以上极品 |

- **剧透闸**：给了 `state` 时，来源里的商店（按店 id 的 `chNN_` 前缀）、编成、宝箱采集所在图、支线只列章号不超过当前章的；全被闸掉也写「剧情所得」。单测遍历时不给 `state`，全列。
  「当前章」**不能用 `GameState::chapter`**：它不跟剧情走，第 7 章的两份检查点存档里还是 1（截图时商店来源被误闸掉才发现）。
  game 层按章节表推：第一章 `done_flag` 没置的那一章，与此刻所在地图的章号取大的（`game::currentChapter`），填进 `ItemInfoSources::chapter`。
- **用词跟阶段**：`rules::InfoWords`（`game::infoWords(state)` 取 `Wording`）：凡人篇写「回复气力」「基准价 N 碎银」。
- 字样全在 `data/text/ui_iteminfo.json`（`ui.iteminfo.*`），C++ 只引 key、用 `rules::fillTemplate` 填 `{n}` 之类的占位。
- 法术：`rules::describeMagic` → 类别（五行与毒；看破、削架势、辅助、无属性各有说法）、耗法力、效用（威力与打一人 / 全体；下毒；看破「揭开本场所有敌人的破绽，不伤人」；削架势「一个敌人的架势减 N 点，减到 0 就破势」；蓄劲「蓄劲加威，每点劲多一倍，至多 ×4」/「蓄劲连发，每点劲多一发，至多 4 发」——4 = 1 + `battle::kBoostMax`；不吃蓄劲的写「不吃蓄劲」；护身罡这类写「战斗里施展不了」）。

### 4.2 宝箱与采集点的索引（`game/ItemInfoView.cpp::buildItemSpots`）

扫 `maps/*.tmj` 的 `chest`（`loot`）与 `gather`（`item`），用 `worldobj::parseItemList` 与引擎同一种写法，记「物品 → 哪些图」；
集齐奖励（`data/exploration`）记成另一种。`Application::itemSpots()` 头一次要用时建（88 张图约 0.15 秒），建好留着。
说明块每样东西算一次约几毫秒（要翻全部商店、编成、配方），所以界面用 `DetailCache`：光标那一样不变就不重算。

### 4.3 五处界面

| 处 | 改动 |
| --- | --- |
| 主菜单物品页 | 左 44% 列表（**按品阶上色**：`ui::ListItem::labelColor`，凡品照旧正文色）、右边说明块：名称（品阶色，灵草带年份）、效果、用途、来源、品阶、一条分隔线、原来的风味文字（暗一档）、可服用的接「按确认使用。」。列表占满页高（原来让出底下 150 高的说明区） |
| 主菜单法术页 | 同一个分栏：名称、类别、消耗、效用、分隔线、原来的说明 |
| 商店买卖页 | 左栏日期、钱、上一笔反馈之下，一条分隔线，接光标那一样的说明块（17 号字） |
| 战斗法术 / 物品选单 | 说明区写光标那一门 / 那一件的效果行（`magicSummary` / `effectSummary`，过 `stageWords`），金字；上一句反馈照旧在前。说明区只有 72 高，通用文字块（内边距 24）只排得下一行，这两种选单改成自己排：15 号字、内边距 12、至多三行 |
| 得物卡片副行 | 有效果行就用效果行的第一条（「回复气血 6」「施展「火弹术」 · 火 · 威力…」），没有才用说明的首句 |

原有 `descKey` 文案**一个字没改**（`ItemInfo.DetailRowsKeepTheOrderAndTheFlavourTextUntouched` 钉着）。

---

## 5. 验收判据（施工图 3.5）

| # | 判据 | 结果 |
| --- | --- | --- |
| 1 | 第 7 章 12 张主线图和 6 张历练图上，小地图和区域地图都画得出；标记位置与对象的实际格子一致，换算有单测 | **满足**。12 张主线图 = 11 张 `ch07_*` + 第 7 章开场所在的 `ch06_baiyaoyuan`（施工图说 12 张，`maps/` 里 `ch07_*` 只有 11 张；百药园是第 7 章的图、也在 `maps_hd` 里，按它算）。18 张都有尺寸对得上的高清烘焙图；`MinimapMaps.EveryChapter7MapHasAHdBaseAndEveryMarkLandsOnItsOwnCells` 对每张图上每个门、设施、宝箱、采集点、明雷（共一百多个）在小地图与区域地图两种取景下换算再换回，落在它自己那几格上。截图见第 7 节 |
| 2 | 没有高清烘焙图的章（第 3、6 章各一张）剪影画得出 | **满足**。`ch03_guwai`、`ch06_huangfenggu`：单测核剪影逐格对上通行格，截图两张图各拍小地图与区域地图 |
| 3 | 单测遍历 `data/items` 全部物品：每一样都生成得出来源行；有回复、施法、兵刃字段的都有效果行 | **满足**。`ItemInfo.EveryItemHasASourceAndAValueLine`、`EveryItemWithAnEffectFieldHasAnEffectLine`（另加解毒、下毒字段） |
| 4 | 单测遍历 `data/magics`：每一门都生成得出类别和耗法力行 | **满足**。`ItemInfo.EveryMagicHasACategoryAndACostLine`、`MagicLinesFollowTheBattleRules` |
| 5 | 物品页、法术页、商店页、战斗选单、得物卡片五处都换成新说明 | **满足**。截图见第 7 节 |
| 6 | 全量 ctest 通过 | **满足**：2338 / 2338 |

---

## 6. 测试

新增：

- `tests/ItemInfoTests.cpp`（17 条）：遍历全部物品（每样都有来源行与品阶行、带效果字段的都有效果行、店里卖的都写得出那家店、没有漏填的占位与缺的文案）；遍历全部法术（类别与耗法力、蓄劲与看破削架势的说法与引擎规则一致）；回气丹、黄精、火球符、玉带短剑、不可买卖、剧情所得、剧透闸（含存档章号不跟剧情时以 `sources.chapter` 为准）、凡人篇用词、伙伴喜恶逐字对上；`fillTemplate`、`shopChapter`；说明块的次序与风味文字原样。
- `tests/MinimapTests.cpp`（11 条）：取景与换算（整张居中、开窗夹边、贴边、平移、反向换算）、标记规矩（8 格、灰、目标或通往目标的门、主角最后画）、第 7 章 18 张图、第 3、6 章剪影、小地图设置项读写；真入口（M 开关、手柄 LB、Esc 关、主菜单「地图」、设置关掉小地图就不画、地图键在键表里、提示跟着键位）。
- `tests/UiStyleTests.cpp` 加 `MenuLexicon.TheAreaMapAndTheItemInfoWordsAreSafeForMortalChapters`：区域地图的固有字与 `ui.iteminfo.*` 全部字样过凡人篇禁词表（区域地图与说明是通用界面，第 1 章就看得见；钱与法力走占位，按阶段换词）。

改了的旧测试（都是施工图要的行为变了，断言跟着改，不是放水）：

| 测试 | 原因 | 改法 |
| --- | --- | --- |
| `ExplorationWorld.GatheringPopsACardToo` | 得物卡片副行改用效果行（3.3） | 黄精卡片副行期望从说明首句改为「回复气血 6」 |
| `EngineTests.cpp` 键序 `static_assert` | `Key` 追加 `Map` | 改成 Action → Map → Count |
| `SettingsWiringTests.cpp` 行下标 `static_assert` | 设置面板插「小地图」 | `kMinimap 9 / kKeys 10 / kRestore 11 / kRowCount 12` |
| `SettingsWiring.TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack` | 同上 | 「手柄震动」之后多按一下到「小地图」再到「按键设置」 |
| `SettingsWiring.RebindingThroughTheKeyPanelMovesTheKeyThePromptsAndTheFile` | M 成了地图的默认键，再把 M 配给「菜单」就是和「地图」互换 | 示例键改用空闲的 J（提示期望「J 关闭」等随之改） |
| `SettingsWiring.InTheKeyPanelEscCancelsBackspaceClearsAndTheLastKeyStays`、`AfterAFailedWriteAKeyChangeMakesTheNextCloseTryAgain` | 改键面板「路径行动」与「恢复默认键位」之间多了「地图」 | 往上 / 往下多按一格 |
| `SettingsWiring` 恢复默认那一条里的自定义「菜单」键 | 同 M 冲突 | 改用 J |
| `UiStyle` 设置面板行名 | 插「小地图」 | 行名表加「小地图」 |
| `KeyDefaults.FixedPlusDefaultCustomKeysAreExactlyTheOldNineteenBindings` | 默认表多一条 M → 地图 | 改造前那 19 条一条不动，另加一条，计数 19 → 20 |
| `KeyTableValidation.EachRuleOfTheWholeTableCheckRefusesOnItsOwn`、`KeyRules.RuleFiveAFreeKeyGoesStraightIn`、`HeadlessKeys.CustomKeysDriveKeyDownAndKeyPressed` | 拿 M 当「没人用的键」，M 现在有人用了 | 改用 J |
| `KeyPrompts.PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText`、`PadPlaceholdersOnlyLiveInTheElevenPadPrompts` | 新提示带 `{key.map}` / `{pad.map}` | 两张白名单各加 `ui.map.hint`、`ui.map.hint.pan`、`ui.map.minimap.key`、`ui.menu.map.desc`（及 `.pad`）；`{pad.` 占位计数 21 → 27 |
| `PadTable.EachActionsPadButtonsAreTheContractsTableInDisplayOrder`、`EveryActionHasAPadButtonNoButtonServesTwoAndLbLtServeNone` | LB 配给了地图 | 表加 `Map → LB`；「LB 不配」改为「LB 是地图键、只有 LT 不配」，17 个手柄键有动作 |
| `HeadlessPad.LbTheGuideAndTheStickClicksPressNothingAndLeaveTheDeviceAlone`、`TheDeviceFollowsTheLastPressThatMeantSomething`、`PadWiring.PromptsFollowTheDeviceThatWasPressedLast` | 同上：拿 LB 当「没配动作的键」 | 改用左摇杆按下（仍没配动作） |
| `MenuFlow.HomeIsReachableFromTheWorldMenuAndDefaultsToCancel`、`MenuD.AnEmptyRosterLeavesTheEightOldPagesAndAMetCompanionAddsPartyAndRoster` | 主菜单「记事」后面加了「地图」 | 页表与按键次数各加一项（用例名没改） |

第 1–6、8–13 章的通关验收、对账、触发器测试**一条都没改**。

---

## 7. 截图

全在 `fanren/build-m/shots/`（截图脚本 `build-m/shots.bat`、`shots2.bat`；`pad_*.png` 四张是 `GamepadTests` 跑测试时自己写出来的，不是本路拍的）。

| 图 | 内容 |
| --- | --- |
| `minimap_ch07_jindi_waiwei.png` | 第 7 章野外：小地图（整张、门、存档点、目标菱形、主角三角、右下角「M 地图」），目标的边缘箭头停在小地图左边 |
| `minimap_ch07_dixia_shiku_indoor.png` | 第 7 章室内（地底石窟） |
| `minimap_lilian_jindi_zhongxin.png` | 历练图 |
| `minimap_ch03_guwai_silhouette.png`、`minimap_ch06_huangfenggu_silhouette.png` | 没有高清图的剪影 |
| `minimap_off.png` | 设置关掉小地图（`--settings build-m/nominimap.json`） |
| `areamap_ch07_jindi_waiwei.png`、`areamap_ch07_dixia_shiku_indoor.png` | 区域地图：第 7 章野外、室内（设施旁写名字、图例、出口、凶险、收集进度） |
| `areamap_ch03_guwai_silhouette.png`、`areamap_ch06_huangfenggu_silhouette.png` | 区域地图剪影；出口里通往目标的那一道金字打头「◆」 |
| `menu_items.png`、`menu_items_pill.png`、`menu_items_talisman.png`、`menu_items_weapon.png`、`menu_items_herb.png` | 物品页：剧情物件（只有来源、品阶）、金疮药（四行齐全）、火球符、玉带短剑、灵草 |
| `menu_magic.png`、`menu_magic_reveal.png` | 法术页：火弹术、天眼术 |
| `menu_map.png` | 主菜单停在「地图」 |
| `shop_buy.png` | 商店买货页（第 7 章北街收药摊） |
| `battle_items.png`、`battle_magic.png` | 战斗物品、法术选单的说明区 |
| `item_toasts.png` | 得物卡片：回气丹「回复法力 90」、火球符「施展「火球术」…」、黄精「回复气血 6」、火龙草（没有效果，照旧说明首句） |
| `settings_panel.png`、`keys_panel.png` | 设置面板「小地图」一行、改键面板「地图」一行（手柄列 LB） |

---

## 8. 交付

- 分支 `claude/ch07-foundation-m-377c44`（工作树 `E:anren\.claude\worktrees\ch07-foundation-m-377c44`），未推送、未合并。
- 全量 ctest（`build-m/test.bat`，`-j 4`）：**2338 / 2338 通过**，用时 480 秒。
  中途第二轮有一条 `SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`（关垂直同步时一帧至少 1/240 秒）没过，
  单跑三次都过、第三轮全量也过：是机器负载下的计时抖动，与本路无关。
- 没跑门禁（`validate.py`、`validate_selftest.py`、各种 `--check`），照施工图 1.5。

---

## 9. 没做的、拿不准的、留给集成的

1. **「12 张主线图」**：`maps/` 里 `ch07_*` 只有 11 张，按 `ch06_baiyaoyuan`（第 7 章开场所在、有高清图）补成 12 张。若主会话指的是别的口径，请指正。
2. **共享文件的冲突点**（集成时要合）：`Application.h/.cpp`（末尾一块：`itemSpots()`、`worldScene()`、`explorationRewards()`、`assetRoot()`、`shops()`、`battles()`，成员 `itemSpots_`）；`io/SettingsFile.*`（`minimap`）；`SettingsScene.*`（`kMinimap = 9`，路 G「教学提示」、路 D「自动播放」也要插行，下标要一起排）；`WorldHud.*`（`drawSaveNotice` 加了 `top` 参数，缺省 16，路 G 动目标框不冲突）；`Engine.h/.cpp` 的 `Key` 枚举与键表（路 D 若也追加键，排在 `Map` 之后；本路占 M / LB，共享笔记 `route-m-keys.md` 写着）；`KeyBindingTests` 两张白名单与占位计数、`EngineTests` 键序断言、`GamepadTests` 手柄表（路 D 加键时这几处会再改一次）。
3. **主菜单 `kCommandPanel`**：多了「地图」后左栏九项（名册有人时十一项），面板按行数往下长，与左下角信息面板仍有间隔；没有逐像素对过名册有人时的十一项（伙伴菜单测试只断言页表）。
4. **存盘提示与得物卡片**：存盘提示挪到小地图下面（168 起）后，与第一张得物卡片（172 起）在同时出现时会叠一点（卡片画在上面）。两样同时出现很少（F5 存盘的那三秒里正好拿到东西），没再挪。
5. **边缘箭头**：只做了「不钻进小地图那一块」。箭头三角本身的朝向是改版前的画法，没动。
6. **门禁规则（不跑门禁，只记下将来要补的）**：`ui_map.json`、`ui_iteminfo.json` 的 key 前缀；`ui.map.facility.<kind>` 要覆盖地图上出现的全部设施种类（现在九种全有）；`{key.map}`/`{pad.map}` 的白名单与 `KeyPrompts` 用例同步。
7. **性能**：说明块一样东西约几毫秒（翻全部商店、编成、配方），界面上按选中项缓存；`buildItemSpots` 头一次用时扫 88 张图约 0.15 秒（开主菜单物品页、商店那一下）。没有做成启动时预建。
8. **没加存档字段**，`kSaveVersion` 仍是 10；第 7 章章末状态不受影响（本路不碰脚本与剧情数据）。
