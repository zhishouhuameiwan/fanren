# 八方旅人化 · 界面路契约（墨金主题 / 对话框 / 章节卡 / 主菜单 / 标题画面）

> 2026-09-26 · 界面路（槽 `uistyle`）交付 · 施工图 `docs/octopath-overhaul.md` 1.5、1.6、第 4 节的落地版。
> 读者：世界画面（W）、战斗画面（BV）、路径行动入口（P2）。**签名以本文与头文件为准。**
> 头文件：`src/ui/Widgets.h`、`src/game/ChapterCardScene.h`、
> `src/game/FadeScene.h`、`src/game/MenuScene.h`、`src/game/TitleScene.h`、`src/game/Application.h`。
> 读表在 io 层：章节表 `src/io/ChapterLoader.h`，精灵索引 `src/io/VisualLoader.h`（挑外观 `src/game/SpriteAtlas.h`）。

---

## 1. 主题 token（`ui::Theme`）

施工图 1.6 那张表原样进了 `ui::Theme`（缺省值出自 `ui::palette` 同一组常量，`tests/UiStyleTests.cpp` 按施工图原文钉着）：

| token | 值 | 用途 |
| --- | --- | --- |
| `ink` / `inkLow` | `#0E1119` α .86 / α .72 | 面板底（竖向渐变上端 / 下端） |
| `inkDeep` | `#07090E` | 名牌底、槽、投影、黑幕、卡片底 |
| `gold` / `goldDim` | `#C9A45C` / 暗金 | 外框、角花、选中条 / 内缩线、分隔线 |
| `goldBright` | `#F2D98B` | 标题、当前项、名签字 |
| `paper` / `paperDim` | `#F1EAD8` / `#9C9480` | 正文 / 次要字、禁用项 |
| `cinnabar` | `#B8412F` | 危险、失败（存盘失败、读档失败） |
| `jade` / `azure` | `#5FAE8C` / `#5B8FD6` | 气血条 / 法力（气力）条 |
| `sheen` / `scrim` | 高光 / 纱 | 数值条顶上的高光 / 模态面板底下压的纱 |
| `bodyStyle` / `titleStyle` | `TextStyle` | 正文投影 / 标题描边 + 投影 |
| `breakRed` | `#FF7860` | 战斗：破势那一句、首领「蓄势」标签 |
| `poison` / `poisonDim` | `#C492EC` / `#BA96E2` | 战斗：中毒（脚下状态字、飘字）/ 队伍面板里「中毒 · 守势」那行小字 |
| `floatCinnabar` / `floatDim` / `floatJade` / `floatGuard` | `#E86048` / `#C4BEB0` / `#8CE8AC` / `#96C8FA` | 战斗飘字：蓄势 / 遁走、脱身 / 毒解 / 守势 |
| `chargeBandTop` / `chargeBandBottom` | `#340C0A` α .9 / `#180606` α .9 | 首领蓄势预告横幅的底（上 / 下） |

- 契约里按用途命名的旧五个（`panelFill/panelEdge/text/textDim/highlight`）**保留**，值就是 `ink/gold/paper/paperDim/goldBright`，
  给还没改造的调用方（`BattleScene` 等）用。**新代码一律用 token**；BV 改造战斗画面时顺手换掉，届时可删旧字段。
- 摆位几何一个数没动（`padding 24 / lineSpacing 8 / bodyFontSize 22 / titleFontSize 24`）：各面板可显示几行、对话框不出屏
  （`ListLayoutTests`）照旧成立。
- 战斗那几色（`breakRed` 起的九个，2026-09-26 追加在末尾）是终审 LOW-4 从 `BattleHud` / `BattleScene` 收上来的字面量，
  数值原样；比面板色亮一档，因为压在战斗背景上。追加在末尾，按契约字段顺序的聚合初始化不受影响。

## 2. 装饰件（`src/ui/Widgets.h`）

| 函数 | 画什么 |
| --- | --- |
| `drawPanel` | 竖向渐变底 + 1px 金框 + 内缩 3px 暗金线 + 四角菱形；有标题时标题下一道向右渐隐的金线 |
| `drawSelection(row)` | 选中行：左端金色菱形指针 + 向右渐隐的金条（`ListView` 用它） |
| `drawGauge` | 深槽 + 竖向渐变填充 + 顶部高光 + 暗金边 |
| `drawOrnament(cx, y, half)` | 饰线：中间菱形、两侧金线向外渐隐（章节卡、标题画面） |
| `drawPlate(rect)` | 小牌：深底金边、两端小菱形（对话框名签；**W 做地名横幅可以直接用**） |
| `drawFrame / drawDiamond / drawRuleH / drawRuleV` | 1px 框、菱形、两头渐隐的暗金分隔线 |
| `drawScrim` | 整屏压一层纱：模态面板（修炼/炼制/商店/灵田/告示板）打开时先画它 |
| `drawSpacedText / spacedTextWidth` | 拉开字距的大字（章节卡、标题） |
| `mixColor / withAlpha` | 颜色运算（不另起字面量颜色） |

`ListView` 追加 `renderPreview`（同一张表不画光标，主菜单右栏预览用）；右栏（数量、价格、禁用理由）与名字撞上时自动退一号字、
再不够就截尾补「…」，不再整句压在名字上。

## 3. 对话框（`DialogueScene`）

- 底部墨金框；说话人是框上沿左侧一块**独立名签**（`nameTagArea`，旁白不画）；右上沿一块小牌写「Tab 回看　长按 Ctrl 快进」
  （2026-09-27 起是文案 `ui.dialogue.keys`，键名跟着键位走，见 `docs/settings.md` 第 6 节；默认键位下逐字不变）。
- 正文逐字（60 字/秒不变）；读完、没有选项时右下角一个缓慢起伏的「▼」。
- 选项：正文与选项之间一道暗金线，选中行同 `ListView`。**脚本的 `choice{}` 不带问句**，框里现在留着刚说完的那一句（连名签，直接全显）。
- 回看面板（Tab）同一套 `drawPanel`。
- **躲开主角**（终审 LOW-6）：框压着主角（`Application::heroScreenRect`：主角那一格连同四周一格——说话的 NPC 就在相邻一格；只在世界层露着时有）就整个上移，
  底边抬到那一圈的上沿（最高到离上沿 48），名签、键位牌跟着框走；尺寸与内容一点不变（`keepHeroInSight`）。只挪必要的那一截、
  不直接翻到顶边：顶边左上角是地名与目标提示，半透明的框压上去两层字叠在一起。只在镜头贴着地图下沿、人站在屏幕下半时才会发生。
  路径行动面板（`PathActionScene`）同一条规矩。
- F5 存盘的提示是说给玩家听的：`ui.world.save.ok`「已存盘。下回从标题画面选『继续旅程』…」，不提命令行（终审 LOW-2）。

## 4. 章节卡（`ChapterCardScene`）与触发规则

- 数据：`data/chapters.json`（章号、`numeral_key` / `title_key`、`done_flag`；`closing_key`「终」、`to_be_continued_key`「未完待续」），
  文案在 `data/text/ui.json`。门禁照常查（`*_key` 必须有文案、`*_flag` 必须登记）。
- **触发由引擎做，剧情脚本一句不改**：`Application::dispatch` 的 SetFlag 把某章 `done_flag` 由 0 置成非 0 时，
  `cardsForFlagChange` 排「第 N 章　终」接「第 N+1 章」开篇（最后一章之后接「未完待续」），记进 `pendingCards()`；
  `flushChapterCards` 等三件事都成立才播：脚本演完且没有命令在等、这一帧栈上没有要压要弹的、栈顶是 `WorldScene`。
  读档（旗标本来就是 1）、脚本重复置、清回 0 都不排。
- 新开一局（标题画面「新的旅程」）先播第一章开篇卡。
- 画面：黑底淡入 0.9 秒、停 2.5 秒、淡出 0.9 秒；确认键从当前亮度接着淡出；每张卡播一次 `chapter_card`。
- **无头下不压场景**（`presentCards` 只记 `cardLog()`），`ChapterCardScene` 自己在无头下也第一帧退场。`HeadlessChapterCards` 那条用例逐帧看栈顶。
- 脚本 `ending(title, body)` 也用这个场景（`ChapterCardScene::ending`）：标题 + 饰线 + 正文，完全显出后等确认，淡出后回填命令。

## 5. 淡入淡出与等待（`FadeScene`）

- `fade.out(ms)` / `fade.in(ms)` / `wait(ms)` 做成真的：黑幕浓度在 `Application::screenFade()`，`FadeScene` 按毫秒推、推完回填。
- 黑幕由 `Application::drawScenes` **紧贴世界层**画（压住地图与 HUD、压不住对话框；世界层被战斗、卡片等不透明场景盖住时不画）。
- 无头下当场到终值、当场回填，与改造前一样不多等一帧。
- 注意：`fade.out` 之后一直黑到 `fade.in`，脚本要成对写（引擎不替脚本自动复位）。

## 6. 主菜单（`MenuScene`）与入口

- 入口：`Application::openMainMenu()`。**按键钩子在 `WorldScene::update`（W 路的文件）**，要加的几行见第 8 节补丁。
- 左栏七项：状态 · 物品 · 法术 · 记事 · 存盘 · 设置 · 返回（「法术」凡人阶段叫「法门」，见下；「设置」2026-09-27 插在存盘与返回之间）；
  左下：盘缠 / 日期 / 所在。右栏随光标预览；
  物品、法术按确认进右栏列表看说明；记事直接压 `BoardScene`；存盘走 `Application::quickSave()`（与 F5 同一条路），成败都说一句；
  设置页右栏写一句说明 + 当前八项的摘要，确认压系统设置面板 `SettingsScene`（与标题画面共用；菜单照「记事」的做法只画模糊底图，
  面板一收再成为顶层）。设置面板与改键面板见 `docs/settings.md` 第 5 节。
- 状态页：韩立 + 在队同伴，小像取 `assets/art/sprites/index.json`（`io::loadSpriteIndex` 读、`game::sheetForRole` 挑，
  与世界画面同一张表、同一条规则：`roles` + `role_variants`，
  变体判据是**章节表里那一章的 `done_flag`**——韩立 `max_chapter: 2` → `ch02.done` 未置时用少年版），×4 画；没有精灵时画一方刻着姓的「印章」。
  气血 / 气力条、境界（`realmText`，第 1–5 章说「口诀　第几层」）、攻防（`realmAttack/realmDefence`，与战斗同源）、速度（`data/roles/hanli.json`）。
- 用词走 Wording：`menuStrings(stage)` 列出菜单上全部固有字，`MenuWording` 用例拿禁词表扫。钱（背包里的 `material_lingshi`）不进物品页。
- 键：Tab 在哪一级都合上；Esc 退一级（左栏时合上）。音效：`ui_open / ui_close / ui_cursor / ui_confirm / ui_cancel`（另：存盘成功 `save`、不可用 `ui_error`）。
- 底图：打开那一帧拍 `PostFx::snapshot()`，之后只画 `blurredSnapshot()`（`opaque()` 随之为真，世界层不再重画）。
- 菜单与标题画面开着的时间记进 `playSecondsSystem`，不计入玩法时长（方案 2.2 的防刷口径）。

## 7. 标题画面（`TitleScene`）

- 不带 `--load` / `--map` 启动时的第一个场景（无头与带 `--scene` 的截图口不走它）。
- 背景：`assets/art/title/{sky,far,mid,near}.png` 四层 ×4，sky/far/mid 按 3/6/12 px/s 漂移首尾相接，near 不动；
  **缺任何一张**就整套换成程序画法（渐变晨天、日晕、两重远山、云海、近峰）。上面薄雾 + 光点粒子，PostFx 做景深、辉光、暗角、调色。
- 菜单四项（下标 `kNewJourney 0 / kContinue 1 / kSettings 2 / kQuit 3`）：新的旅程（淡入黑 → `startNewJourney` → 第一章开篇卡 → 韩家村）/
  继续旅程（`saves/quick.sav` 在才可选；`continueJourney` 失败如实写在菜单下，不静默新开局）/
  设置（压系统设置面板 `SettingsScene`，面板开着时标题背景的漂移停住，见 `docs/settings.md` 5.2）/ 离开。BGM `bgm_title`。
- `--load <档>`（不带 `--map`）与「继续旅程」走同一个 `Application::continueJourney`。

## 8. 给协调者落地的补丁：`WorldScene::update` 接主菜单

插在 F5 存盘那一段之后、确认键交互之前（脚本在跑时上面那道早退已经挡着）：

```cpp
    // 主菜单（施工图 1.5：行走时 Tab / Esc 都打开）。与存盘一样只在世界层受理；
    // 按哪个键、开哪个面板由 Application 决定，世界层不认识菜单类。
    if (eng.keyPressed(engine::Engine::Key::Menu) || eng.keyPressed(engine::Engine::Key::Cancel)) {
        app.openMainMenu();
        return true;
    }
```

## 9. 截图口（`--scene`）

`title`、`chapter:<n>[:end]`、`menu[:status|items|magic|board|settings]`、`settings[:keys]`、`talk:<key>[:<说话人角色 id>]`、
`choice:<选项key,选项key…>[:<上一句 key>[:<说话人>]]`、`ending:<标题 key>:<正文 key>`、`cultivate`、`shop:<店 id>`。
标题画面空转 150 帧、章节卡与结局卡 90 帧、对白 240 帧（等淡入、逐字走完），其余 3 帧。

## 10. JSON（已收回 io 层）

界面路交付时为读章节表与精灵索引在 `src/ui/Json.*` 手写过一个严格 JSON 读取器（nlohmann 只 PRIVATE 链给 `fanren_io`，
当时 io 不在界面路白名单里），另有 `ui::SpriteIndex` 读人物表。C1 收口时统一到 io 层，两者均已删除：
- 章节表 → `io::loadChapterFile`（`src/io/ChapterLoader.h`），`game::loadChapterTable` 只把文案 key 换成正文；
- 精灵索引 → `io::loadSpriteIndex`（`src/io/VisualLoader.h`，结构体 `core::SpriteIndex` 在 `src/core/model/Visual.h`），
  选外观 `game::lookForRole / sheetForRole`（`src/game/SpriteAtlas.h`）。
严格度不降：语法错带行列号、拒 BOM、限嵌套深度（`src/io/StrictJson.h`）。**上层不再自写 JSON 读取器。**
