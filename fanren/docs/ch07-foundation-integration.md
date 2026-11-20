# 第 7 章可玩性改版·第一期地基包·集成记录（2026-11-20）

> 三路（G 指引、D 对白减负、M 地图与说明）合到一个集成分支：解冲突、补三路彼此看不见的接缝、编译、跑全量 ctest、
> 做试玩启动脚本。上位文档：`docs/ch07-foundation.md`（施工图，第 1 节总则、第 5 节集成流程、第 7 节审核记录）、
> `docs/gameplay-overhaul.md`（方案与拍板），两份随本分支一并入库，与主检出里的逐字节相同。

---

## 0. 一览

| 项 | 内容 |
| --- | --- |
| 分支 | `claude/happy-maxwell-8427c8`（从 main 切出；未推送、未合进 main） |
| 工作树 | `E:\fanren\.claude\worktrees\happy-maxwell-8427c8` |
| 合并次序 | 路 G `claude/ch07-foundation-g-96ce1c` → 路 D `claude/ch07-foundation-d-cd9304` → 路 M `claude/ch07-foundation-m-377c44`，都是 `--no-ff` |
| 全量 ctest | **2394 / 2394 通过**（`build-int`，`-j 4`，376 秒，`build-int/ctest_full1.log`）；三条墙钟用例也在其中、都过 |
| 换行符 | `git diff --stat main...HEAD` 与加 `--ignore-cr-at-eol` 的结果逐数相同（每次提交后复核；代码部分 157 个文件，加上三份文档 160 个） |
| 存档版本 | `kSaveVersion` 仍是 10；没有新存档字段 |
| 试玩 | `fanren/build-int/` 与主检出 `E:\fanren\fanren\build-s7\` 各三个启动脚本（第 7 节） |

提交（`git log --first-parent main..HEAD`）：

| 提交 | 内容 |
| --- | --- |
| `merge: 第 7 章地基包·路 G——…` | 路 G 合入（与 main 无冲突） |
| `merge: 第 7 章地基包·路 D——…` | 路 D 合入，14 个文件冲突（第 1.1 节） |
| `merge: 第 7 章地基包·路 M——…` | 路 M 合入，13 个文件冲突（第 1.2 节）；同时修了路 G/D 合并留下的一处漏 `break` |
| `fix: 第 7 章地基包集成——屏幕上沿同屏不叠、…` | 接缝的修补与单测、截图口（第 2、3 节） |
| `docs: …` | 两份上位文档入库、本记录 |

---

## 1. 冲突怎么解的

总原则照主会话定的：三路都是成块追加或小段插入，**三块都留**，不为省事丢任何一路的行为。
合并时 Git 把有冲突的文件按 CRLF 写了出来（本机全局 `core.autocrlf=true`，冲突文件不受 `-text` 管），
解完逐个按 HEAD 上那一份的换行符写回（只有 `KeyBindingTests.cpp`、`WorldScene.cpp` 是 CRLF，其余 LF）。

### 1.1 合路 D（在路 G 之上）

| 文件 | 解法 |
| --- | --- |
| `src/game/SettingsScene.h/.cpp` | 中间态：`kAutoAdvance = 9`、`kTutorialCards = 10`；最终次序在合路 M 时定（1.2） |
| `src/io/SettingsFile.h/.cpp` | `tutorial_cards`、`auto_advance` 两个键都留（读、写、字段） |
| `src/script/Command.h` | `CommandKind` 末尾依次 `Tutorial`、`Bubble`；参数表两行都留 |
| `src/script/ScriptHost.cpp` | 命令名表 `tutorial`、`bubble` 都留 |
| `scripts/common/api.lua` | 末尾 `tutorial()` 一段在前、`bubble()` 一段在后，补回两段之间的 `end` 与分隔线 |
| `src/game/Application.cpp` | 三处重置 `resetGuideWatch()` 与 `resetDialogueExtras()` 都调；`tick` 里路 D 的「跳过提示停着时菜单键开回看」与路 G 的 `watchGuide()` / `flushTutorials()` 都在；时钟三样（见闻提示、跳过提示、气泡）都走；`drawScenes` 里见闻提示、气泡、跳过提示都画（补回自动合并掉的一个 `}`） |
| `src/game/ShotScenes.cpp` | `guide:` 与 `dlg:` 两个入口都在派发里；`pushGuideShot` 函数体原样 |
| `data/lore/ch07.json` | 顶层取路 G 的（带 `name`）；`entries` 先路 G 的 5 条妖兽、再路 D 的 23 条，共 28 条。路 D 的 id 去掉 `ch07_` 前缀（`ch07_yuelu_ru_menjin` → `yuelu_ru_menjin` 等），文案 key 本来就是 `lore.ch07.<去前缀的 id>.*`，不用动；28 个 id 无重复。测试里没有按 id 引用路 D 条目的地方（`keyOrders()` 按 key 前缀登记），不用改 |
| `data/text/ch07_lore.json` | 两份文案并成一份（10 + 46 = 56 个 key，无重叠） |
| `tests/SettingsWiringTests.cpp`、`UiStyleTests.cpp` | 中间态行号、行名；最终在 1.2 |
| `tests/KeyBindingTests.cpp` | 两张白名单取并集；`{pad.` 占位按数据数（这一步是 26，测试照数过） |
| `tests/Ch07AcceptanceTests.cpp` | 自动合并：路 G 的 `tutorial.`、五条妖兽与路 D 的 23 条见闻都在 `keyOrders()` 里；路 D 改的两条新真值（带名牌的句数 > 60、主线字数 9000–10000）照路 D 的 |

### 1.2 合路 M（在 G + D 之上）

| 文件 | 解法 |
| --- | --- |
| `src/game/SettingsScene.h` | 0–8 行不动；`kAutoAdvance = 9`、`kTutorialCards = 10`、`kMinimap = 11`、`kKeys = 12`、`kRestore = 13`、`kRowCount = 14` |
| `src/game/SettingsScene.cpp` | 行表、`valueText`、`adjusted` 三处三行都在，「游玩」组 6 行（文字速度、手柄震动、自动播放、教学提示、小地图）；面板见下 |
| `src/io/SettingsFile.h/.cpp` | `tutorial_cards`、`auto_advance`、`minimap` 三键都留 |
| `src/game/Application.h` | 路 D 一块、路 M 一块都留；**`battles()` 路 G、路 M 各加了一个同样的访问器**，去掉路 M 那一个（注释写明） |
| `src/game/Application.cpp` | 文件末尾路 G、路 D、路 M 三块依次都在（路 M 那块补回丢掉的分隔线） |
| `src/game/ShotScenes.h/.cpp` | 路 M 的 `menu:items/magic`、`areamap`、`toast`、`shopbuy` 派发在前，`guide:`、`dlg:` 在后；头文件说明三段都留 |
| `src/game/WorldScene.cpp` | `describe` 末尾路 M 的小地图几行在前，路 G 的 `markHeads` / `noteGuideEvents` 在后，其余路 G 的函数原样 |
| `src/game/WorldView.h` | `drawQuestTargets`（G）、`drawMinimap`（M）两个声明都留；`drawHud` 自动合并，目标框两行式（G）与小地图、存盘提示挪位（M）都在 |
| `tests/KeyBindingTests.cpp` | 两张白名单三路并集；`{pad.` 占位**从数据里重新数**：32（原 21 + 路 G 1 + 路 D 4 + 路 M 6），断言与说明照改 |
| `tests/SettingsWiringTests.cpp` | `static_assert` 照最终行号；「按键设置」那一条从手柄震动往下依次走自动播放、教学提示、小地图；路 M 别的改动（J 键、改键面板多一格）自动合并 |
| `tests/UiStyleTests.cpp` | 行名表照最终次序 14 行 |
| `tests/EngineTests.cpp`、`GamepadTests.cpp` | 只有路 M 动过，照路 M 的 |
| `docs/tech-debt.md` | G-25 已由路 G 标成收掉，自动合并带进来 |

**设置面板**：三行都进来按原样加是 622 → 712，贴着 720 的屏幕边。改成高 692、`y = 14`：上下各留 14、仍上下居中。
多出的 20 像素是从「说明区与底下按键提示之间」那段空白里收的（render 不用改：按键提示钉在面板底边）。
截图 `i19_settings_longest_desc.png` 光标停在说明最长的「自动播放」：两行说明放得下，按键提示与说明之间还隔着一段，不叠。

**合并时我自己引进又修掉的一处错**：合路 D 时 `SettingsScene::adjusted` 的 switch 里，`kAutoAdvance` 与 `kTutorialCards` 两个 case
之间漏了 `break`（两边的块都以「赋值」结尾，`break` 在冲突区外面只剩一个），按「自动播放」左右键会连带改掉「教学提示」。
那一步跑的 395 条相关测试没有一条盖住它；合路 M 时看代码发现，补上 `break`，并加单测
`SettingsFoundationRows.EachOnOffRowTogglesOnlyItsOwnSetting` 钉住三行各动各的。

---

## 2. 三路彼此看不见的接缝（施工交代的六条）

| # | 接缝 | 结论 | 证据 |
| --- | --- | --- | --- |
| 1 | 屏幕上沿同屏：地名横幅与目标框（左上）、小地图（右上）、存盘提示与得物卡片、「见闻：XXX」（上沿正中 18 起）、「已跳过 N 句」（120 起正中）、边缘箭头 | **修了三处**，见下 | 截图 `i21`–`i25`；单测 `FoundationSeam.TheSkipNoticeStepsRightOfAWideObjectiveColumn`、`FoundationSeamLayout.ItemToastsStartWhereTheyAreToldAndDefaultBelowTheMinimap` |
| 2 | 整段跳过途中解锁的见闻、排的教学卡 | **没有缺口**：见闻提示每帧比对解锁表，跳过途中 `flag.set` 照办，跳完照报；教学卡排在队里，`flushTutorials` 等脚本结束才弹 | 单测 `FoundationSeam.LoreUnlockedDuringASkipIsStillAnnouncedAfterIt`、`FoundationSeam.ATutorialQueuedDuringASkipPopsOnceTheScriptEnds` |
| 3 | 记事「见闻」页装 28 条 | **修了一处**：多过一页时列表底下的「13/28」刻度画在最后一行上、与它叠字 → 多过一页时少排一行让给刻度（主线页做过的步多时同样受益）。四类都有、右栏最长 103 字的「坊市的规矩」三行放得下 | 截图 `i06`、`i07`（光标在最长那条）、`i08`（翻到最后一条）；单测 `FoundationSeam.TheLorePageHoldsAllTwentyEightAndLeavesARowForTheCounter` |
| 4 | 主菜单「地图」与教学卡「区域地图也在主菜单」 | **成立**：路 M 的 `MapWiring.TheMainMenuHasAMapEntry` 钉着第 7 章主菜单有「地图」；路 G 的 `GuideTutorials.TheWordsInQuotesAreTheOnesTheChapterSevenScreensShow` 合并后照过。它那张主菜单页表是手写的，补上 `Page::Map`，以后卡片写「主菜单「地图」」也认得 | 两条单测；截图 `i12_menu_map.png` |
| 5 | 说明四行式在合并后的数据上 | **没问题**：`ItemInfo.*` 21 条全过 | 全量 ctest |
| 6 | 第 7 章通关验收与别章 | **全过**：`Ch07Acceptance*`、`Ch07Walkthrough*`、`Ch07Ledger*`、`Ch07Slice*`、`Ch07Companions*` 全过（章末状态由这些验收逐项核）；第 1–6、8–13 章的通关验收**一条断言没改** | 全量 ctest；`git diff main...HEAD --stat -- fanren/tests` 里别章的测试文件一个都没有 |

**接缝 1 修的三处**（`i22_seam_long_objective.png` 第一次拍时卡片压在存盘提示上；`i24_seam_no_minimap.png` 第一次拍时存盘提示顶着「见闻」那条、盖住边缘箭头）：

1. **得物卡片排在存盘提示下面**：存盘提示亮着时，卡片的上沿从 172 挪到提示底边 + 6（`WorldView::saveNoticeRect()` → `drawItemToasts(…, top)`）。路 M 记录里写过这处会叠（10.4），当时没挪。
2. **存盘提示一律排在小地图那一块之下**（168 起），小地图关着时也不再贴顶：贴顶时它那五百来像素宽的一条与上沿正中的「见闻：XXX」、目标的边缘箭头会叠。
3. **「已跳过 N 句」给宽目标框让位**：第 7 章最长的一行目标（25 字的要点，`i21` 里那一句）框也只宽到 404 像素，与正中的提示（约 512 起）不碰；但别章有 35 字的目标（第 11 章），框宽到 568，
   会与它叠。现在与目标框那一列（框连同收集进度）上下有交叠、左右又够得着时，提示往右让到框右边 12 像素处。

两条居中的提示（「见闻」18–50、「已跳过」120–154）上下本就错开，不会互相压。边缘箭头只做了「让开小地图」（路 M 做的），没有为别的提示让位：
它跟着目标方向贴边走、位置不定，与 3 秒的存盘提示偶尔同处时箭头在下面——见第 6 节。

---

## 3. 集成时动的代码（冲突之外）

| 文件 | 改了什么 |
| --- | --- |
| `SettingsScene.cpp` | 面板 `{330, 14, 620, 692}`；`adjusted` 补 `break`（1.2） |
| `SettingsScene.h` | 截图口用的构造 `SettingsScene(bool, int startRow)` |
| `BoardScene.h/.cpp` | 多过一页少排一行给刻度；截图口用的构造 `BoardScene(Page, int startRow)` |
| `WorldHud.h/.cpp` | `drawSaveNotice` 返回画出来的矩形（原来是 `void`） |
| `WorldView.h/.cpp` | 记下存盘提示与目标框那一列的矩形；存盘提示一律在 `minimap::kSaveNoticeTop` |
| `ItemToasts.h/.cpp` | `kItemToastTop`（172）挪到头文件；`itemToastRect` / `drawItemToasts` 收一个 `top` |
| `DialogueExtras.h/.cpp` | `skipNoticeArea` / `drawSkipNotice` 收一个 `avoid` 矩形 |
| `Application.cpp` | `drawScenes` 记下这一帧画出来的世界层，拿它的 HUD 矩形给卡片与跳过提示让位 |
| `WorldScene.h` | 截图口用的 `showSaveNotice` |
| `ShotScenes.h/.cpp`、`main.cpp` | 新截图口：`seam:<lore,skip,save,toast>[@记号]`（同屏情形）、`guide:<页>#<n>`、`settings:row<n>`（光标停在第 n 行）；去掉一行重复的 `#include <string>` |

## 4. 改过的旧测试及原因

| 测试 | 改了什么 | 为什么 |
| --- | --- | --- |
| `SettingsWiringTests.cpp` 顶上的 `static_assert` | 最终行号（9 / 10 / 11 / 12 / 13 / 14） | 三路各插一行，集成后的新真值 |
| `SettingsWiring.TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack` | 从手柄震动往下依次过自动播放、教学提示、小地图 | 同上 |
| `UiStyleTests.cpp` 设置面板行名表 | 14 行，照最终次序 | 同上 |
| `KeyPrompts.PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText`、`PadPlaceholdersOnlyLiveInTheElevenPadPrompts` | 白名单三路并集；`{pad.` 占位 32 | 三路各加了带占位的提示；数是从数据里数的 |
| `GuideTutorials.TheWordsInQuotesAreTheOnesTheChapterSevenScreensShow` | 主菜单页表补 `Page::Map` | 路 M 加了「地图」页，表要与第 7 章主菜单一致 |

三路各自改过的旧测试见各自施工记录（G 第 7 节、M 第 6 节、D 第 8 节），合并时原样带进来。第 1–6、8–13 章的通关验收、对账、触发器测试一条没改。

新增 `tests/FoundationSeamTests.cpp`（5 条，即第 2 节表里列的那几条），另在 `SettingsWiringTests.cpp` 末尾加
`SettingsFoundationRows.EachOnOffRowTogglesOnlyItsOwnSetting`（1.2 那处漏 `break`）。

## 5. 测试

- 每合一支编译一次、跑相关测试：合 G 后 184 / 184；合 D 后 395 / 395；合 M 后 549 / 549（正则覆盖设置、键位、手柄、地图、说明、菜单、引擎、教学卡、对白、见闻、记事、第 7 章）。
- 全量（接缝修补之后，`build-int/test.bat`，`-j 4`，机器上没有别的负载）：**2394 / 2394 通过，0 失败**，376 秒。
  三条墙钟用例都过：`PadWiring.HoldingTabForASecondOpensTheMainMenuOnceAndItStaysOpen`（4.15 秒）、
  `PadWiring.HoldingEscForASecondOpensTheMainMenuOnceAndItStaysOpen`（4.39 秒）、`SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`（0.20 秒）。
- 没跑门禁（`validate.py`、`validate_selftest.py`、`genmaps.py --check`、`hd_* --check`），照用户定的规矩。

## 6. 截图（`fanren/build-int/shots/`，脚本 `build-int/shots.bat`）

| 图 | 内容 |
| --- | --- |
| `i01_world_minimap_objective.png` | 章首走图：小地图、两行式目标框（行动句 + 「0/3」+ 要点） |
| `i02_tutorial_card.png` | 教学卡「蓄劲」 |
| `i03_board_main.png`、`i04_board_side.png`、`i05_board_guide.png` | 记事·主线、支线、指南 |
| `i06_board_lore_top.png` | 记事·见闻：28 条全解锁，四类按妖兽、灵草、人物、所闻排，路 D 的条目在列，「1/28」刻度单占一行 |
| `i07_board_lore_longest.png` | 光标在最长的「坊市的规矩」（103 字），右栏三行放得下 |
| `i08_board_lore_last.png` | 翻到最后一条「青元剑诀」，「28/28」 |
| `i09_areamap.png` | 区域地图（禁地外围） |
| `i10_menu_items.png`、`i11_menu_magic.png` | 主菜单物品页（金疮药四行式）、法术页 |
| `i12_menu_map.png` | 主菜单停在「地图」 |
| `i13_shop.png` | 商店买货页（北街收药摊） |
| `i14_battle_items.png` | 战斗物品选单的说明区 |
| `i15_dialogue_auto.png` | 对白框：「自动」小标与键位牌 |
| `i16_skip_ask.png` | 跳过确认框 |
| `i17_bubble.png` | 白衣少女头顶的插话气泡 |
| `i18_settings.png`、`i19_settings_longest_desc.png` | 设置面板（14 行，上下各留 14）；光标在说明最长的「自动播放」 |
| `i20_keys_panel.png` | 改键面板（「地图」一行，手柄 LB） |
| `i21_seam_start.png` | 同屏：章首，目标框两行式 + 小地图 + 见闻提示 + 跳过提示 + 存盘提示 + 两张得物卡片 |
| `i22_seam_long_objective.png` | 同屏：黄枫谷，三行目标框（带「前往」）+ 其余同上 |
| `i23_seam_jindi.png` | 同屏：禁地外围，目标框 + 收集进度 + 边缘箭头 + 小地图 + 其余同上 |
| `i24_seam_no_minimap.png` | 同屏：小地图关着（`--settings build-int/nominimap.json`），存盘提示不再贴顶 |
| `i25_skipped_notice.png` | 「已跳过 12 句 · 按 Tab 回看」单独一张 |
| `i26_load_ch07-start.png`、`i26_load_ch07-jindi.png` | 两个检查点存档读进来的第一屏 |

`sheet0.png`、`sheet1.png`（在 `build-int/` 下）是前 25 张的缩略总览。

## 7. 试玩启动脚本

| 位置 | 文件 |
| --- | --- |
| 工作树 `E:\fanren\.claude\worktrees\happy-maxwell-8427c8\fanren\build-int\` | `玩第7章地基包-从开头.bat`、`玩第7章地基包-从禁地外围.bat`、`玩第7章地基包-标题画面.bat` |
| 主检出 `E:\fanren\fanren\build-s7\`（被 `.gitignore` 忽略的构建目录） | 同名三个，逐字节相同 |

写法照 `build-s7\玩第7章-从开头.bat`：`cd /d <工作树>\fanren`，再 `start "" "<工作树>\fanren\build-int\fanren.exe" --assets . [--load saves\ch07-start.sav | saves\ch07-jindi.sav]`，
路径全是绝对路径。试玩时的快速存档、设置文件落在工作树的 `fanren/saves/` 里，不碰主检出的存档。

验过：两个检查点存档无头读档（`HEADLESS_OK map=ch06_baiyaoyuan` / `map=ch07_jindi_waiwei`）、截图读档（`i26`）、
三个脚本各经 `cmd /c` 启动一次（进程起来、8 秒后仍在跑，之后由我关掉）；另把 `fanren.exe` 以交互模式带同样参数直接跑 8 秒，
标准错误一行没有——读档失败时那里会先打出原因再弹框。
没有在窗口里亲手玩一段，那是用户试玩的事。

## 8. 没做成的、拿不准的、留给门禁的

1. **边缘箭头不为存盘提示、见闻提示让位**：箭头贴边跟着目标方向走，位置不定；与只亮 3 秒的存盘提示同处时箭头在下面。没做。
2. **存盘提示挪位是集成时定的**：小地图关着时也排在 168 起（原来贴顶 16）。若主会话觉得关小地图的玩家该看到贴顶的提示，可改回，但要另想办法避开「见闻」那一条。
3. **「已跳过 N 句」让位只在第 7 章以外才会真发生**（第 7 章目标框最宽 404，碰不到正中），由单测钉着，没有在别章找一处实景截图。
4. **头顶「支」与目标菱形同处时不避让**（路 G 10.4 第 6 条），集成没动。
5. 路 D 交回的三条「无着落」（过了时辰另算、墙上有价目、水罩符持续抽法力）照主会话审定不补。
6. **门禁将来要补的规则**（本期不跑门禁）：三路记录里各列了一份（G 10.4 第 7 条、M 第 10 节第 6 条、D 第 11 节第 6 条）；集成另加两条：
   `data/lore/*.json` 合并后 **id 全局唯一、不带章节前缀**（文案 key 照 `lore.ch07.<id>.*`）；
   `{key.*}` / `{pad.*}` 占位白名单与 `KeyPrompts` 用例的计数要随数据同步（眼下 `{pad.` 32 处）。
7. **环境上的坑**（给后来的人）：本机全局 `core.autocrlf=true`，合并冲突的文件会被写成 CRLF，解完要按 HEAD 的换行符写回；
   这台机器的 Bash 工具会把命令里的 `\\` 收成 `\`（heredoc 里也一样），写带反斜杠的脚本要用文件而不是内联；
   `grep -c $'\r$'` 在这里数的是行数、不是 CR 数，查换行符用 Python 按字节数。
