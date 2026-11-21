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

---

## 9. 试玩返修·不挡路（2026-11-20，用户试玩地基包之后）

用户原话：「像是打坐的蒲团，或者7章开始任务提示点 类似这种 都不应该阻挡走路，这样才符常理」。
主会话定的规则一句话：**画面上是实心东西才挡路；贴地的、看不见的不挡。**

### 9.1 改了什么

| 处 | 改动 |
| --- | --- |
| `WorldScene::walksOnto` | 新的具名判定：设施且 `kind=meditate`（蒲团；第 1 章的平石、第 8 章的养息处、灵泉都是这一种）→ 能走上去。其余设施（存记点、历练晶石、药田、摊位、丹炉、告示牌、制符、锻造）照旧占格 |
| `WorldScene::tryStep` | 设施占格改问 `blockingFacilityAt`（跳过蒲团）；NPC、宝箱、采集点、明雷、关着的机关不变 |
| `WorldScene::interact` | 次序不动，最后加一条「脚下的蒲团」，排在「脚下的按键型任务点」之后——同一格脚下两样都有时任务点优先（第 5 章客栈 (11,7) 正是蒲团与 `trigger_neishi` 同格） |
| `WorldScene::interactHint` | 面朝蒲团时提示词由「使用」改「打坐」；坐在蒲团上时提示的那一格是主角自己那格，`WorldView` 原有的分支把它画在主角头顶。新文案 `ui.world.interact.meditate`（`data/text/ui.json`） |
| 明雷巡逻 | `advanceFoes` 的 `around.solid` 不变：设施一律当墙，**明雷照旧不踩蒲团**（只补了一句注释） |
| `tools/mapgen/genmaps.py` | `build_everything()` 最后加一道 `clear_flat_cells`：全仓每张图上蒲团那几格、`FLAT_TRIGGERS` 所列任务点那一格，碰撞清零。用 `genmaps.py --sync` 写盘，写了 12 张图，每张只差这几格碰撞（逐层逐格比过：别的图层、对象表一处没动）。**building 层照留**，理由见下 |
| `ShotScenes` 的 `walk:` | 截图口多认小写 `u d l r`：只转身不迈步（拍「在蒲团旁面朝它」要用） |
| `depthOrder`（`WorldView.cpp`） | 蒲团按矩形**上沿**排进深度序：按底边排时与坐在上面的人同一个 footY，再比 footX，两格宽的蒲团就画在人腿上（第一张截图里看出来的）。别的设施照旧按底边排 |

**building 层为什么不清**：各章生成器早年用 `solid_rect(…, 8)` 把蒲团画成 gid 8「次体」，烘焙底图（`tools/artgen/mapinfer.py`）
正是靠这个 gid 认出「这里摆着一件东西」画出蒲团。清掉 building，下次重烘蒲团就从画面上没了。于是这几格是「画了、不挡路」，
与水面「不画墙、只挡路」反过来——是 `genmaps.py` 文件头那条不变量（building 有瓦片处 collision 非 0）仅有的例外，代码注释里写明了。
`mapinfer.py` 用 `collision==0` 找房间与豁口，所以**将来重烘这 12 张图里的非高清图**（第 5、6、8 章那几张）时，屋里多出一格可走，
房间与豁口的推断可能略有不同；`mapbake.py` 没有 `--check`，不会因此报漂移。高清底图（`maps_hd`）与碰撞无关，`hd_maps.py --check` 不受影响。

清掉碰撞的格（12 张图、20 格）：

| 图 | 格 | 是什么 |
| --- | --- | --- |
| ch05_kezhan | (11,7)(12,7) | 蒲团（(11,7) 同格还有 `trigger_neishi`） |
| ch06_baiyaoyuan | (23,4)(24,4) | 居室蒲团 |
| ch06_baiyaoyuan | (22,3) | `trigger_chaibao` 拆包点 |
| ch06_sanxiu_lou | (22,2) | 小楼二楼蒲团 |
| ch07_jindi_zhongxin | (8,6) | 树洞里的打坐处 `facility_dazuo_shudong` |
| ch07_jindi_zhongxin | (30,25)(31,25) | `trigger_shulin` 与树下打坐处 `facility_dazuo_shuguan` |
| ch08_dongfu | (21,17) | 灵泉 `facility_lingquan` |
| ch08 其余七张 | 各一两格 | 养息处 `facility_yangxi*`（共 9 处） |

其余 22 张图的蒲团碰撞本来就是 0（只靠引擎的「设施占格」挡着），这次只是引擎不再挡。全游戏 `kind=meditate` 共 35 处，裁图总览
`build-int/shots/r01_meditate_cells_overview.png`。

### 9.2 第 7 章会走到的图上的按键型任务点（逐格判定）

范围：`ch06_baiyaoyuan`、`ch06_huangfenggu`、`ch07_*`，`trigger` 且 `mode=interact`，有一格落在碰撞格上的。
**一共 27 处，不是 24**：返修单给的分项（4、9、2、1、1、1、3、1、1、2、2）加起来就是 27。清 2 处，留 25 处。
画面取的是玩家实际看到的那张：有高清底图的用 `assets/art/maps_hd/<图>/`（below + above），黄枫谷没有高清图，用烘焙图 `assets/art/maps/`。
裁图总览 `build-int/shots/r00_trigger_cells_overview.png`（每处周围 5×5 格，放大，标格线，红框是任务点那一格）；单张 `cell_NN_<名>.png`。

「⚠」= 脚本说的与画面上画的对不上（没修图，只标出来）。

| # | 图 | 格 | 脚本说的 | 画面上画的 | 判定 | 理由 |
| --- | --- | --- | --- | --- | --- | --- |
| 01 | ch06_baiyaoyuan | (22,3) | 居室里放包袱的小桌 | 空地板（墙角下一格，没有桌、没有包袱） | **清** | 空地。⚠ 脚本说小桌 |
| 02 | ch06_baiyaoyuan | (23,8) | 茅屋前的药篓 | 石灯 | 留 | 实心石灯。⚠ 脚本说药篓 |
| 03 | ch06_baiyaoyuan | (26,4) | 居室的书桌 | 书桌桌面 | 留 | 桌 |
| 04 | ch06_baiyaoyuan | (26,5) | 书桌的另一格 | 书桌带抽屉的柜身 | 留 | 柜 |
| 05 | ch06_huangfenggu | (4,23)(5,23) | 迎宾楼房里的床 | 蓝被褥的床 | 留 | 床 |
| 06 | ch06_huangfenggu | (23,8)(24,8) | 议事大殿的殿门 | 两扇黑漆殿门 | 留 | 门 |
| 07 | ch06_huangfenggu | (38,28)(39,28) | 林师弟屋前的案 | 深色木案，摆着东西 | 留 | 案 |
| 08 | ch06_huangfenggu | (38,9)(39,9) | 传功阁前的案 | 木案，一张摊着纸 | 留 | 案 |
| 09 | ch06_huangfenggu | (6,12) | 百机堂的柜台 | 柜台 | 留 | 柜台 |
| 10 | ch06_huangfenggu | (4,3)(5,3) | 内殿的卷宗架 | 书架 | 留 | 架子 |
| 11 | ch06_huangfenggu | (7,12) | 柜台、于执事面前那一格 | 柜台 | 留 | 柜台 |
| 12 | ch06_huangfenggu | (25,12) | 王师叔身边那张案 | 竖条木架（像兵器架） | 留 | 实心木架。⚠ 脚本说案（轻微） |
| 13 | ch06_huangfenggu | (26,8) | 第 8 章「登记」，脚本没写挂在什么上 | 麒麟阁白墙（窗与门之间） | 留 | 墙 |
| 14 | ch07_dihuo | (24,2) | 十九号圆墩旁的石凳 | 深色木凳 / 矮几 | 留 | 凳。⚠ 石凳画成木色（轻微） |
| 15 | ch07_dihuo | (28,1) | **十九号墙角的蒲团** | **一只摆着发红物件的木箱**（左边一盆火） | 留 | 照画面是实心木箱。⚠⚠ **要你拍板**，见 9.4 第 1 条 |
| 16 | ch07_dixia_shiku | (21,2) | 北头岩层最薄处（脚本写明是实体格，人站在正下方面朝上按） | 岩块 / 石阶顶 | 留 | 岩层，设计上就不让站 |
| 17 | ch07_dixia_zhaoze | (3,0) | 石阶口最上面那一格的岩壁 | 图上沿的石墙、石柱 | 留 | 岩壁 |
| 18 | ch07_fangshi | (28,19)(28,20) | 万宝楼的门 | 朝西的两扇门扇与下面的门槛格栅 | 留 | 门 |
| 19 | ch07_huanxingshan | (19,28) | 洞外的岔路石 | 树桩 | 留 | 实心树桩。⚠ 脚本说岔路石 |
| 20 | ch07_huanxingshan | (37,31) | 青石殿的门 | 殿墙（门画在东邻 (38,31)） | 留 | 墙。⚠ 门画偏了一格 |
| 21 | ch07_huanxingshan | (42,21) | 东坡石屋的门 | 门 | 留 | 门 |
| 22 | ch07_jindi_wai | (26,6) | 队列前的旗杆 | 一根短石柱 / 木桩（旗杆座） | 留 | 柱 |
| 23 | ch07_jindi_zhongxin | (30,25) | 林边那棵大树 | 树冠下的草地（两根树干画在上一行） | **清** | 空地。⚠ 脚本说大树，那一格是树下草地 |
| 24 | ch07_shandong | (7,5) | 洞里的石壁 | 崖壁上洞口的黑影（整行 y=5 都是崖壁） | 留 | 崖壁。洞口黑影看着像能进，但那一整行生成器都是岩体，单清一格会在墙上凿出一个缺口 |
| 25 | ch07_shandong | (20,10) | 洞外空地那块大石 | 大石 | 留 | 石 |
| 26 | ch07_yuelu_dian | (8,5)(9,5) | 藏室的书桌 | 书桌 | 留 | 桌 |
| 27 | ch07_yuelu_dian | (8,15) | 许老的柜台 | 柜台 | 留 | 柜台 |

### 9.3 查的三样

**（a）蒲团格、清掉的任务点格上有没有出生点、传送点、踏入型触发区**：没有。全游戏 45 格（35 处蒲团 + 两处任务点）逐格对过对象表，
同格的只有一处：第 5 章客栈 (11,7) 蒲团与按键型的 `trigger_neishi`（内视）。两样都是按键型，按 9.1 的次序任务点优先，
站在上面按、面朝它按都先演内视，演完（或不满足条件时）再按就是打坐——与改动前面朝它按的次序一样。

**（b）脚本的移动或传送落点有没有靠「这一格走不上去」**：没有。脚本里能动主角位置的只有 `teleport(图, x, y)`，
全部常量落点没有一个落在这 45 格上；贴着蒲团落地的两处（第 7 章 `poding` 落 (24,15) 挨着石窟蒲团 (25,15)、第 9 章 `fuji` 落 (29,26)
挨着黄山脚下的养息处 (28,26)）落点本身不变，只是落地后往蒲团那边多一格能走。不带坐标的 `teleport("图")` 落在出生点，
第 12 章 `zoupian_*` 的 `FORK_X/FORK_Y` 在 `ch12_guiwu`，那张图没有蒲团。

**（c）目标指路**：`routeToward` 是图与图之间的门路，与格内占不占格无关；「N 格」是到目标对象中心的曼哈顿距离，菱形、地上那圈金光
钉在目标对象那一格，都不问能不能站上去，改动前后一样。新情形只有一种：**人能站到目标格上了**（拆包点、林边大树下）。这时金光落在脚下
（`drawObjectiveRing` 本来就为「人站在目标格上」写过），菱形悬在人头顶，「[Z] 调查」在更上面——见截图 r02。
第 7 章 `n46_zhuji`「去墙角的蒲团坐下」指的是 `trigger_zhuji` (28,1)，这一格判「留」（画面是木箱），指路与改动前完全一样：
菱形在木箱上，人走到旁边面朝它按确认。它**不是**设施，所以 `walksOnto` 不管它，也不会弹打坐面板。

教学卡「打坐回气」与目标文案里没有「面朝蒲团」一类说法（只有「蒲团上打坐」「去墙角的蒲团坐下」「去蒲团上坐一阵」），没改。

### 9.4 拿不准、交回的

1. **第 7 章最后一步的「蒲团」画成了木箱**（表中 15）：`trigger_zhuji` 的脚本、目标「去墙角的蒲团坐下」都说是蒲团，用户试玩的抱怨也是蒲团；
   但规则 B 让按画面判，那一格画的是实心木箱，我照规则留着挡路、没放宽。可选两条路：照画面留（现在这样），
   或认它是蒲团、加进 `FLAT_TRIGGERS` 清掉（一行），顺手把那一格的高清底图改成蒲团（美术活，本次不动）。
2. **第 8 章的九处养息处、一处灵泉也算蒲团**（`kind=meditate`），照规则 A 一并能走上去；画面上它们是石头、矮案、木台
   （裁图总览 r01 第 2、3 行）。主会话若觉得石头不该能站，要另定一条，本次没区分。
3. 门禁 `tools/validate.py` 的 `occupied_cells` 注释写着「facility 没有可见性属性，一律占格」，与引擎现在不一样了（它更严：把蒲团当墙）。
   可达性一类检查只会更保守、不会误报通过，所以没改；门禁照用户的规矩本次没跑，留给下次碰门禁的人顺手对齐。
4. 第 10–13 章测试驱动（`tests/Ch1xDriver.h`）寻路时照旧把一切设施当墙，绕着蒲团走，走得通，没改。

### 9.5 测试

- 新文件 `tests/FlatCellTests.cpp`，12 条：判定函数只认蒲团；能走上蒲团（两格宽的另一半也能）；坐在上面按确认开打坐面板；
  在旁边面朝它也开；坐在上面时提示的格就是主角那格、字是「[Z] 打坐」，在旁边时提示在蒲团上方；明雷来回巡逻 12 秒不踩蒲团、
  停在蒲团跟前（探针图），主角照样能走上那一格；拆包点站上去按、面朝它按，各只演一次；书桌两格、石灯照旧走不上去；
  27 处任务点逐个钉住清 / 留；全游戏 35 处蒲团没有一格还是墙。
  坐在蒲团上的人、上一行的人都画在蒲团之上（深度序）。
- 改的旧测试 **1 条**：`PanelTest.FacilitiesOccupyTheirCellSoTheyCanBeFaced`（`tests/PanelTests.cpp`）。它原本拿蒲团来断言「设施占格」，
  与新规则正面相反；改成拿同一张图（神手谷）上的药田来问，断言一字未改（撞不过去、站在原地、脸转过去）。这不是「走位变了」，
  是这条用例问的那件东西换了；按返修单的口径单列出来。
- 别章测试改了**走位 3 处**（只改寻路，断言一条没动）：`tests/Ch02SliceTests.cpp`、`tests/Ch05SliceTests.cpp`、
  `tests/Ch07AcceptanceTests.cpp` 三个机器人的 `enterable()` 各一行——「设施一律进不去」改问 `WorldScene::blockingFacilityAt`，与引擎一致。
  原因：它们面朝一格进不去的东西时，是「站到旁边、朝它按一下方向键，人不动、脸转过去」；蒲团能走了，这一下人就走上了蒲团，
  脸朝着蒲团的另一头，于是「走不到 facility_dazuo 跟前」「走不到 trigger_neishi 跟前」「居室的蒲团坐不上去」，连带 10 条通关 / 验收用例红
  （第 2 章 2 条、第 5 章 2 条、第 7 章 6 条）。改成认得蒲团进得去之后，机器人走它们原有的另一条路：从蒲团对面那一格迈一步落到旁边、面朝它。
  第 7 章验收的断言一字未改；第 8–13 章的机器人照旧把一切设施当墙、绕着蒲团走，走得通，没改。
- 全量 ctest：见 9.6。

### 9.6 结果与截图

- 编译通过；全量 ctest **2406 / 2406 通过**（`build-int`，`-j 4`，428 秒，`build-int/ctest_flat3.log`；比集成时多 12 条，即 `FlatCellTests`）。
  中间一轮是 2395 / 2405（9.5 说的那 10 条），改了机器人的走位之后全过。
- `build-int/fanren.exe` 已重编，三个启动脚本双击就是新版。
- 截图（`build-int/shots/`，脚本 `build-int/shots_flat.bat`，都从 `ch07-start.sav` 起，人在 (22,4)）：
  `r02_on_chaibao.png`（`walk:U`，站在拆包点上，「[Z] 调查」在头顶）、`r03_on_cushion.png`（`walk:DRU`，坐在蒲团上，「[Z] 打坐」在头顶）、
  `r04_beside_cushion.png`（`walk:DRu`，在蒲团下方面朝它，「[Z] 打坐」浮在蒲团上方）；裁图总览 `r00`（27 处任务点）、`r01`（35 处蒲团）。
- 截图里看到、没动的两处：人站在目标格上时（r02），目标菱形悬在人脸上、「[Z] 调查」框在它上面，三样挤在一起——
  这是「脚下的调查点」原有的画法，不是这次引进的，只是现在站上目标格的机会多了；「[Z] …」框的下沿与背影的发顶贴着（r03），同一个原因。
