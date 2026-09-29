# 手柄支持（施工契约）

> 2026-09-28 用户拍板：**支持手柄，Windows 下的 Xbox 手柄优先**。范围 = 基础方案 + 手柄震动；键位用下面第 2 节那套（推荐方案）；
> 顺带修「按住菜单 / 取消键时主菜单每秒开关八次」——**只让方向键连发**。
> 不做：手柄改键、PS / Switch 显示各自的键名、右摇杆、触摸板、陀螺仪、任天堂手柄的 A/B 语义对调（SDL 按位置对上，照样能玩）。
> 本文是派工契约：词表、默认值、阈值、文件格式、接口名、文案原文在这里写死一种，施工照抄，要改先改这里。
> 与系统设置契约 `docs/settings.md` 配合读：键位、设置文件、两块面板、提示占位符的来龙去脉都在那里。

---

## 0. 摸底时核实过的事实（协调者 2026-09-28，本机）

- 所有场景只认 10 个逻辑键（`Engine::Key`），物理键 → 逻辑键只在 `Engine::pollEvents` 一处换算。手柄接进这一处，**场景代码一行不用改**。
- 探针（Python ctypes 直接调 `vendor/SDL3-3.4.16/lib/x64/SDL3.dll`，脚本不在仓库里）：
  - `SDL_Init(SDL_INIT_GAMEPAD)` 在本机正常；**本机眼下没接手柄**（`SDL_GetGamepads` 数到 0 个）。
  - **虚拟手柄**（`SDL_AttachVirtualJoystick`，type = GAMEPAD，15 键 6 轴）能走完 SDL 真实的手柄链：
    `SDL_IsGamepad` 为真、类型 STANDARD；接上就来 `SDL_EVENT_GAMEPAD_ADDED`；按键出 `BUTTON_DOWN/UP`；
    设轴出 `AXIS_MOTION`（**第一次设轴时 SDL 会把六个轴的初值各补发一遍，而且同一个值连发两次**——处理要幂等）；
    拔出（detach）时**先补发松开**（轴归零、键抬起）**再报 `REMOVED`**。没有窗口也照样收得到事件。
  - **先插着、后开手柄子系统**（只开 `SDL_INIT_JOYSTICK` 时挂上虚拟手柄，再 `SDL_InitSubSystem(SDL_INIT_GAMEPAD)`）→ SDL 补发 `GAMEPAD_ADDED`；
    对照组（先开子系统后挂）同样收到。所以游戏启动前就插着的手柄不必另外枚举。
- 画面上告诉玩家按哪个键的提示全部经 `Application::text()` 一个出口（`ui.title.keys`、`ui.menu.keys`、`ui.dialogue.keys`、
  `ui.path.keys`、`ui.settings.hint`、`ui.keys.*`、`ui.battle.hint.*`）。
- 字体 `assets/fonts/LXGWWenKai-Regular.ttf` 有 `≡`（U+2261）、`⧉`（U+29C9）、`↑↓←→` 的字形（fontTools 查 cmap，带阳性 / 阴性对照）。
- 按住任何键超过 250ms 都会自动连发（`kKeyRepeatDelayMs` / `kKeyRepeatIntervalMs`，对全部 10 个键一视同仁）。于是：
  世界层见 Menu / Cancel 的「刚按下」就开主菜单，主菜单见 Menu / Cancel 的「刚按下」就合上——**按住 Tab / Esc / X 超过 0.25 秒，
  主菜单每 60ms 开合一次**；按住 F5 每 60ms 存一次盘；按住确认键对白每 60ms 翻一页、翻完又和同一个人重谈。
  （读代码推断，未实测；第 5 节要求先写出失败的用例再修。）

## 1. 范围

1. 引擎：打开 SDL 手柄子系统、热插拔、多个手柄同时认；十字键 / 左摇杆 / 按键 / 扳机 → 同一套 10 个逻辑键，与键盘同一套「刚按下 / 按住 / 连发」语义。
2. 连发规则改成**只有方向键连发**（键盘、手柄一样）。
3. 提示跟设备走：最后一下按的是手柄，就显示手柄键名；按一下键盘就切回。键盘文案一个字不动。
4. 改键面板加一列只读的「手柄」；抓键盘键时按手柄 B = 作罢。
5. 手柄震动：破势、我方挨重击时震一下；设置面板加「手柄震动」开关，设置文件加 `pad_rumble`。

## 2. 手柄键位（固定，不可改）

按**位置**认键（SDL 的 SOUTH / EAST / WEST / NORTH），叫法用 Xbox 的：A 下、B 右、X 左、Y 上。PS、Switch 手柄按同一位置对上。

| 动作 id | Key | 手柄键（按显示优先的次序） |
|---|---|---|
| `up` | Up | 十字键↑、左摇杆↑ |
| `down` | Down | 十字键↓、左摇杆↓ |
| `left` | Left | 十字键←、左摇杆← |
| `right` | Right | 十字键→、左摇杆→ |
| `confirm` | Confirm | A |
| `cancel` | Cancel | B（世界层与键盘的 Esc 一样也开主菜单） |
| `menu` | Menu | Y、≡（Start / 菜单键） |
| `skip` | Skip | RT、RB（按住快进 / 加速） |
| `save` | Save | ⧉（Back / 视图键） |
| `action` | Action | X |

- 不配任何动作：LB、LT、左右摇杆按下、右摇杆、Xbox 键（Guide，Windows 的 Game Bar 要用它）、分享键、背键、触摸板。
- 一个手柄键只属于一个动作；每个动作至少一个手柄键（测试钉）。
- SDL 对应：A = `SDL_GAMEPAD_BUTTON_SOUTH`、B = `EAST`、X = `WEST`、Y = `NORTH`、⧉ = `BACK`、≡ = `START`、LB / RB = `LEFT/RIGHT_SHOULDER`、
  十字键 = `DPAD_*`；LT / RT = `SDL_GAMEPAD_AXIS_LEFT/RIGHT_TRIGGER`；左摇杆 = `SDL_GAMEPAD_AXIS_LEFTX/LEFTY`。
- **显示名** `Engine::padLabel`：A、B、X、Y、LB、RB、LT、RT 照写；Back → `⧉`；Start → `≡`；
  十字键 → `十字↑` `十字↓` `十字←` `十字→`；左摇杆 → `摇杆↑` `摇杆↓` `摇杆←` `摇杆→`。

## 3. 摇杆与扳机的数字化（纯函数，单测直接喂）

轴值归一：摇杆 `v / 32767`，再夹到 [-1, 1]（-32768 → -1）；扳机 `v / 32767`，夹到 [0, 1]。SDL 的 y 轴**向下为正**（y < 0 是上）。

**左摇杆 → 四向**：`static std::optional<Key> stickDirection(float x, float y, std::optional<Key> held)`
- r = √(x² + y²)，大于 1 当 1。没按住（held 为空，或不是四个方向键之一）时 r ≥ **0.5** 才算推到位；按住时 r ≥ **0.35** 仍算（迟滞，防抖）。不到位 → 空。
- 方向取最近的轴：|x| > |y| → 左 / 右；|y| > |x| → 上 / 下；**恰在对角线（|x| = |y|）取左 / 右**（与世界层「左右优先」同口径）。
- 按住某个方向时，摇杆偏离那条轴**不超过 55°**（对角线再过去 10°）就保持原方向；超过才换成最近轴的方向。
- **换向算一次新的推**（整改轮 2026-09-28，审查 HIGH-1 的契约层补丁）：偏出 55° 要换到新方向时，新方向与从零推起一样要 r ≥ **0.5**；
  不够 0.5 → 空（当松开）。0.35 的迟滞只管「原方向继续按住」，不给新方向。
- 用例至少覆盖：中心为空；(0.49, 0) 空、(0.5, 0) 右；四个正方向；按住右时 (0.36, 0) 仍右、(0.34, 0) 空；
  对角线 (0.6, 0.6)、(0.6, -0.6) 右，(-0.6, 0.6) 左；按住右时偏 50° 仍右、偏 60° 换下；按住上时 (0.55, -0.6) 仍上；(1, 1) 不越界；
  按住右时 (0.1, 0.45)（偏约 77°、r≈0.46）→ 空，(0.1, 0.6) → 下。

**扳机**：`static bool triggerDown(float value, bool wasDown)`：没按住时 value ≥ **0.5** 算按下；按住时 value ≥ **0.3** 仍按住。

## 4. 设备、热插拔、事件

- **初始化**：非无头时在建好窗口、渲染器之后 `SDL_InitSubSystem(SDL_INIT_GAMEPAD)`；失败只打一行警告（「手柄子系统起不来，只能用键盘：…」），
  照样启动——与音频起不来的处理同一个口径。**无头不开**（几百条无头测试与 `--headless` 机器人因此不受开发机上插着的手柄影响）；
  但 `pollEvents` 对手柄事件的处理**不看是谁开的子系统**：测试自己开子系统挂虚拟手柄、或用 `SDL_PushEvent` 塞手柄事件，走的都是同一段代码。
  **不许为测试另开生产分支**（settings.md 审查 A6 的教训）。
- `SDL_EVENT_GAMEPAD_ADDED` → `SDL_OpenGamepad(which)`，失败打一行警告跳过。已经插着的手柄在子系统启动时 SDL 会补发 ADDED
  （第 0 节探针实测），**不另外枚举**（两处都开会把同一个手柄开两次）。
- `SDL_EVENT_GAMEPAD_REMOVED` → 关句柄、删掉那个手柄的状态（它按着的一切随之松开）。拔掉的是最后一个手柄、且眼下的设备是手柄 → 设备回到键盘。
- 按键 / 轴事件的 `which` 不在表里（`SDL_PushEvent` 塞的测试事件）→ 按 `which` 现建一份状态（句柄为空）。句柄为空的不算「接着的手柄」，也不能震。
- 每个手柄各记一份状态：18 个 `PadButton` 各自按没按（摇杆四向与两个扳机由第 3 节的纯函数写进这张表）、左摇杆两轴的最新值。
  **逻辑键「按住」= 键盘按住 或 任一手柄上它的任一手柄键按住**；某个手柄键从没按到按下 → 它那个动作记一次「刚按下」（与键盘一个物理键按下同一个口径）。
  同一个值的轴事件重复来（SDL 补发初值时就这样）不产生新的按下。
- **摇杆按「一份完整的报告」判方向，不按单个轴事件判**（整改轮 2026-09-28，审查 HIGH-1）。SDL 一份报告里先发 LEFTX、再发 LEFTY、
  最后发 `SDL_EVENT_GAMEPAD_UPDATE_COMPLETE`（3.4.16 缺省发，审查方实测）；逐个轴事件判的话，两个事件之间是「新 X + 旧 Y」的半截状态——
  斜推后松手，X 先归零那一刻是 (0, 0.5)，偏离右轴 90°，凭空出一次「下」的刚按下（设置面板光标跳一行、战斗换了指令、世界多走一步）。所以：
  - `AXIS_MOTION`（LEFTX / LEFTY）只存值、记「这个手柄的摇杆有新值」；
  - `UPDATE_COMPLETE`（`event.gdevice.which`）→ 这个手柄有新值就按第 3 节判一次，清掉标记；
  - 一帧的事件处理完之后，仍有新值没判的手柄再判一次（`SDL_PushEvent` 塞的测试事件没有报告末尾的标记，也走这里——同一个函数，不是测试专用分支）；
  - `REMOVED` → 状态整份删掉，没判的新值随之作废（拔线时 SDL 逐轴补发回中，不许判出方向）。
  - 扳机是单轴，收到就判（不存在半截状态）。
- 窗口失焦：沿用 SDL 缺省（`SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS` = "0"，头文件原话「应用在后台时关掉手柄输入事件」）。
  失焦时松开 / 回中的事件是否照发，头文件没写；协调者记得 SDL 源码是照发（防卡键），**未实测**——列进第 12 节手测单第 6 条。**不另加失焦处理**。

## 5. 连发：只有方向键

- 上下左右四个键（键盘、手柄一样）照旧：按下出一次，按住 250ms 后每 60ms 再出一次。
- 确认、取消、菜单、快进、存盘、路径行动：**一次物理按下 = 一次 `keyPressed`，按住不连发**。`keyDown` 不变（快进靠它）。
- 改在「记一次刚按下」的那一处（单点）：只有方向键设重复计时。`Engine.h` 里 `keyPressed` 的注释跟着改。
- 先 RED 后 GREEN：先写接线用例——世界层按住 Tab 1 秒（逐帧 `tick`），主菜单开了之后**一直开着**、只开过一次；按住 Esc 同样；
  按住手柄 Y、B 同样。这几条在改之前必须是红的（施工记录里写明红过）。
- **旧用例的判别力**：`HeadlessKeys.WhileCapturingNoLogicalKeyFiresAndTheNextPressIsTaken` 拿确认键验「抓键期间按住的键不自动重复」，
  改完之后确认键本来就不连发，这句断言**失去判别力**（抓键不拦连发它也照样过）——补一个按住方向键的版本，保住判据；
  `HeadlessEngine.AltEnterTogglesFullscreenAndIsNeverAConfirm` 里「按住 300ms 不出 Confirm」同理，确认它仍有 `keyDown(Confirm)` 为假那一句兜着。
  逐条查全部旧用例，失去判别力的在实现记录里列出、各自怎么补的。**旧测试一条都不许放宽**；真值变了的（依赖非方向键连发的）改成新真值并列出。

## 6. 提示跟设备走

- `Engine::lastInputDevice()`，缺省 `Keyboard`。**切换点就是「记一次刚按下」的那一处**（与第 5 节同一处）：
  键盘上一个配给了动作的键（固定键或自定义键）按下 → `Keyboard`；手柄上一个配给了动作的输入（按键、十字键、摇杆推到位、扳机过半）变成按下 → `Gamepad`，
  并记下是哪个手柄（震动找它）。没配动作的键（Alt、Win、媒体键、LB、LT、Xbox 键……）不切。
  抓键时：被抓走的那一下键盘键 → `Keyboard`；手柄 B 作罢 → `Gamepad`。最后一个手柄拔掉 → `Keyboard`（第 4 节）。
- `Application::text(key)`：设备是手柄、且文案表里有 `key + ".pad"` → 取那一条；然后展开占位：`{key.<id>}` 照旧（键盘键名），
  `{pad.<id>}` → 那个动作第一个手柄键的显示名（第 2 节表的第一个）。**键盘模式下不取 `.pad`，展开结果与今天逐字节相同**
  （`SettingsWiring.DefaultKeyPromptsAreByteForByteTheOldText` 一字不改照样绿）。
- `.pad` 文案（照抄；进 `data/text/ui.json` / `ui_battle.json`，与键盘版同一个文件）：

| id | 键盘版（不动） | `.pad` 版 | 默认展开成 |
|---|---|---|---|
| `ui.title.keys` | ↑↓ 选择　Enter 确认 | ↑↓ 选择　{pad.confirm} 确认 | ↑↓ 选择　A 确认 |
| `ui.menu.keys` | {key.menu} 关闭　Esc 返回 | {pad.menu} 关闭　{pad.cancel} 返回 | Y 关闭　B 返回 |
| `ui.dialogue.keys` | {key.menu} 回看　长按 {key.skip} 快进 | {pad.menu} 回看　长按 {pad.skip} 快进 | Y 回看　长按 RT 快进 |
| `ui.path.keys` | 确认 选定　Esc 作罢 | {pad.confirm} 选定　{pad.cancel} 作罢 | A 选定　B 作罢 |
| `ui.settings.hint` | ↑↓ 选择　←→ 调整　Esc 返回 | ↑↓ 选择　←→ 调整　{pad.cancel} 返回 | ↑↓ 选择　←→ 调整　B 返回 |
| `ui.keys.hint` | ↑↓ 选择　←→ 选格　Enter 改键　Esc 返回 | ↑↓ 选择　←→ 选格　{pad.confirm} 改键　{pad.cancel} 返回 | ↑↓ 选择　←→ 选格　A 改键　B 返回 |
| `ui.keys.capture` | 按下新键（Esc 作罢，Backspace 清空） | 在键盘上按下新键（{pad.cancel} 作罢） | 在键盘上按下新键（B 作罢） |
| `ui.keys.desc` | 选好一格按 Enter，再按下要配的键。方向键、Enter、Esc 是固定键，始终可用。 | 这里改的是键盘键位，手柄键位固定（见最右一列）。选好一格按 {pad.confirm}，再在键盘上按下要配的键。 | …按 A… |
| `ui.keys.desc.restore` | 全部动作回到默认键位。按 Enter 执行。 | 全部动作回到默认键位。按 {pad.confirm} 执行。 | …按 A 执行。 |
| `ui.battle.hint.menu` | ↑↓ 选择 · ←→ 蓄劲 · 回车 确定 · Esc / {key.menu} 返回 · 按住 {key.skip} 加速 | ↑↓ 选择 · ←→ 蓄劲 · {pad.confirm} 确定 · {pad.cancel} / {pad.menu} 返回 · 按住 {pad.skip} 加速 | ↑↓ 选择 · ←→ 蓄劲 · A 确定 · B / Y 返回 · 按住 RT 加速 |
| `ui.battle.hint.watch` | 按住 {key.skip} 加速 | 按住 {pad.skip} 加速 | 按住 RT 加速 |

- 上表每一条的显示点都必须经 `app.text()`（今天不经它的改成经它——`.pad` 只在那里生效）；`.pad` 里只许有 `{pad.*}`、不许有 `{key.*}`。
  `KeyPrompts` 那一组用例加一条钉住：`{pad.` 只出现在上表 11 条的 `.pad` 版里、每条 `.pad` 都有键盘版本体、id 认得（`Engine::keyFromId`）、括号闭合。
- 「按确认键继续」这类不点名物理键的话、只有键盘才有的功能（Alt+Enter）不出 `.pad` 版。

## 7. 改键面板

- 键位二右边加一列只读的「手柄」（文案 `ui.keys.col.gamepad` = `手柄`），画法同「固定」那一列（灰，光标选不到）。面板加宽到放得下，仍居中、不出 1280×720。
  （初版契约写的是 `ui.keys.col.pad`，与第 6 节「`.pad` 后缀 = 手柄版文案」撞名，整改轮改名；**文案 id 一律不许以 `.pad` 结尾，除非它就是某条提示的手柄版**。）
- 每行写那个动作的手柄键显示名，多个用 ` / ` 隔开：`十字↑ / 摇杆↑` …… `A`、`B`、`Y / ≡`、`RT / RB`、`⧉`、`X`；「恢复默认键位」那一行这一列空着。
- 抓键（等玩家在键盘上按新键）时：手柄 B = 作罢（`takeCapturedKey()` 交出 `kEscapeCode`，面板照现有的「作罢」那条路走）；
  手柄上别的键一律不理、抓键继续。手柄键按下的物理状态照记（松开时对得上），只是不出逻辑键。
  否则只拿手柄的玩家按 A 进了「按下新键」就出不来。
- `KeyConfigScene::keyConfigStrings` 把新列头与 `.pad` 文案也列进去（禁词扫描）。

## 8. 手柄震动

- **设置项**：`pad_rumble`（C++ 字段 `padRumble`），开 / 关，**缺省开**。这是对 settings.md「默认值一律等于改造前的行为」的有意例外：
  改造前根本不认手柄，没有「以前」可对；键盘玩家永远感觉不到它（第 6 节：最后一下按的不是手柄就不震）。
  设置文件样例在 `text_speed` 后面加 `"pad_rumble": true`；缺字段 → 默认；类型不对 → 默认 + 警告（照已有字段的写法）。`version` 仍是 1。
- **设置面板**：「游玩」组里、「文字速度」之后加一行「手柄震动　〈 开 〉」。行号变为
  `0 音乐音量 · 1 音效音量 · 2 显示模式 · 3 画面缩放 · 4 垂直同步 · 5 画面特效 · 6 战斗震屏 · 7 文字速度 · 8 手柄震动 · 9 按键设置 · 10 恢复默认`
  （`kPadRumble = 8`、`kKeys = 9`、`kRestore = 10`、`kRowCount = 11`）。两档行规矩照旧：← 取「开」、→ 取「关」，到头就停。
  文案：`ui.settings.pad_rumble` = `手柄震动`；`ui.settings.desc.pad_rumble` = `破势、挨重击时手柄震一下。用键盘时不震。`；值沿用 `ui.settings.value.on/off`。
  主菜单「设置」页的摘要多一项（九项）；「恢复默认」连它一起回到开。
- **引擎**：`setRumbleEnabled(bool)` / `rumbleEnabled()`（无头也存值，读回的就是存进去的）；`Application::applySettings` 推 `settings.padRumble`。
  `rumble(float low, float high, int durationMs)` 是**唯一的闸**：开关开着、眼下的设备是手柄、最后按键的那个手柄还接着（句柄非空）
  → `SDL_RumbleGamepad(句柄, low·0xFFFF, high·0xFFFF, ms)`（强度夹到 [0, 1]，时长夹到 [0, 1000]）；否则什么也不做。
  `SDL_RumbleGamepad` 的返回值**有意不看**：不带马达的手柄是正常情况，每一击都打一行日志只是刷屏（在调用处写一句注释说明）。
- **战斗里什么时候震**（`BattleView`，常量具名写在 BattleView.cpp 顶上）：
  - `onBreak`（谁被破势都算）→ `rumble(0.8, 0.6, 280)`；
  - `onHit`：出手的是敌人、挨打的是我方，且这一下伤害 ≥ 挨打那人气血上限的 1/4（`value * 4 >= maxHp`）或这一下把人打倒（事件里的 `hp == 0`）
    → `rumble(0.5, 0.35, 180)`；
  - 别处一律不震（普通命中、世界层都不震）。**与「战斗震屏」开关互不相干**。
- 为什么不在 `startShake` 里一起震：震屏每一击都有（3–5 像素），手柄每一击都震就成了噪音；只在上面两处。

## 9. 引擎接口（`src/engine/Engine.h` 追加；SDL 类型仍然一个都不出现在头文件里）

```cpp
// ---- 手柄（docs/gamepad.md）----
// 按位置命名（Xbox 的叫法）：A 下、B 右、X 左、Y 上。Stick* 是左摇杆数字化后的四向，LT / RT 是扳机数字化后的按下。
enum class PadButton {
    A, B, X, Y, Back, Start, LB, RB, LT, RT,
    DpadUp, DpadDown, DpadLeft, DpadRight,
    StickUp, StickDown, StickLeft, StickRight,
    Count
};
enum class InputDevice { Keyboard, Gamepad };

[[nodiscard]] static std::vector<PadButton> padButtons(Key key);   // 第 2 节那张表，按显示优先次序
[[nodiscard]] static std::string padLabel(PadButton button);       // 第 2 节「显示名」
[[nodiscard]] static std::optional<Key> stickDirection(float x, float y, std::optional<Key> held);   // 第 3 节
[[nodiscard]] static bool triggerDown(float value, bool wasDown);                                    // 第 3 节
[[nodiscard]] InputDevice lastInputDevice() const;                 // 第 6 节
[[nodiscard]] int gamepadCount() const;                            // 眼下开着的手柄数（句柄非空的）
void setRumbleEnabled(bool on);
[[nodiscard]] bool rumbleEnabled() const;
void rumble(float low, float high, int durationMs);                // 第 8 节
```

`PadButton`、`InputDevice` 放在 `class Engine` 里面（与 `Key` 同处）。`shutdown` 关掉全部手柄句柄、清空手柄状态、设备回到键盘。

## 10. 测试（最低清单；照前几轮的教训，**至少一条走玩家碰得到的真实入口**）

新文件 `tests/GamepadTests.cpp`（放已有目录，CMake 按 glob 收）。

1. **表与纯函数**：`padButtons` 与第 2 节的表逐项相等（期望写成字面量，不从被测物推）；每个动作至少一个手柄键、没有手柄键属于两个动作；
   `padLabel` 逐个对第 2 节；第 3 节 `stickDirection`、`triggerDown` 的全部用例。
2. **无头引擎 + `SDL_PushEvent` 塞真手柄事件**（照 `EngineTests` 的 `pushKey` 写一个 `pushPadButton` / `pushPadAxis`）：
   A → Confirm 刚按下且按住、松开；B / X / Y / Start / Back / RB → 各自的动作；RT、LT 过半 / 回落（LT 不出任何键）；十字键↓ 按住 → 250ms 后连发；
   A 按住 400ms → 只出一次；左摇杆推到位 → 方向刚按下，回中 → 松开；摇杆换向不经中心 → 新方向刚按下、旧方向松开；同值轴事件连来两次不出第二次按下；
   两个手柄：两边都按着 A、松开一边仍按住；`REMOVED` → 它按着的立刻松开；LB、Guide 不出任何键、不切设备；
   `lastInputDevice`：按键盘映射键 → Keyboard，按手柄映射键 → Gamepad，按没配动作的键两边都不切；抓键期间手柄键不出逻辑键、B → 交出 `kEscapeCode`、别的键抓键继续。
3. **虚拟手柄走 SDL 全链路**（无头 Engine；测试自己 `SDL_InitSubSystem(SDL_INIT_GAMEPAD)`，开之前用 hint 关掉真手柄驱动：
   `SDL_HINT_JOYSTICK_RAWINPUT`、`SDL_HINT_JOYSTICK_WGI`、`SDL_HINT_XINPUT_ENABLED`、`SDL_HINT_JOYSTICK_DIRECTINPUT`、`SDL_HINT_JOYSTICK_HIDAPI`、
   `SDL_HINT_JOYSTICK_GAMEINPUT` 置 "0"，测试末尾复原）：接上 → `gamepadCount() == 1`；虚拟 A → Confirm；虚拟左摇杆 → 方向；虚拟 RT → Skip 按住；
   detach → 全部松开、`gamepadCount() == 0`、设备回键盘；**先挂虚拟手柄、后开手柄子系统**（只开 `SDL_INIT_JOYSTICK` 时挂上）→ 照样收到 ADDED、被打开；
   **震动到得了手柄**：虚拟手柄的 `Rumble` 回调记下收到的强度——最后按的是这个手柄且开关开 → 收到 `(0.8·0xFFFF, 0.6·0xFFFF)` 量级的值；
   开关关 → 收不到；最后按的是键盘 → 收不到。
4. **连发（第 5 节）**：RED 先行的接线用例（按住 Tab / Esc / 手柄 Y / 手柄 B 1 秒，主菜单只开一次、一直开着）；引擎层：六个非方向键按住 400ms 各只出一次，四个方向键照旧连发。
5. **接线（真 `Application`，`GameWiringTests` / `SettingsWiringTests` 那一类）**：世界层手柄 Y → 主菜单、B → 合上；对白里手柄 A 翻页；
   `app.text()` 的 11 条：键盘模式与今天逐字节相同，按过手柄之后等于第 6 节表的最后一列（期望串写死在测试里，不从 ui.json 读）；
   改键面板：手柄 A 在键位格上 → 进抓键、说明行是 `.pad` 版，手柄 B → 作罢；设置面板第 8 行 ←→ → `settings().padRumble` 与 `engine.rumbleEnabled()` 同变、关面板写盘读回相等；
   战斗：挂虚拟手柄并按过它，打出一次破势 → 虚拟手柄收到震动（走 `BattleView` 的真实事件流，不是直接调 `rumble`）。
6. **设置文件**：`pad_rumble` 往返、缺省为开、类型不对回默认 + 警告；`SettingsDefaults` 那条补上新字段。
7. **禁词**：`UiStyleTests` 把新行名、新说明、新列头、全部 `.pad` 文案扫进去；设置面板行名逐字对第 8 节。
8. 旧测试一条都不许放宽；行号、项数这类断言改成新真值（第 8 节），在实现记录里列出改了哪几条、为什么。
9. **整改轮（2026-09-28 独立审查）追加**：
   - HIGH-1 回归（先 RED 后 GREEN）：同一帧里塞 LEFTX、LEFTY（按真实顺序再跟一个 `UPDATE_COMPLETE`，另一条不跟）——从右偏下 30°、45°、右偏上、
     左偏下斜推后一份报告回中，都不许出上 / 下的刚按下、也不许留一帧按住；20° 与 (1, 0) 两条对照照旧；虚拟手柄一次更新回中不出；
     斜推着拔线不出；**设置面板接线**：光标第 0 行，摇杆 (0.87, 0.5) 推一下、一份报告松手 → 光标仍在第 0 行（整改前实测跳到第 1 行）。
   - 关真手柄驱动的 hint 用 `SDL_SetHintWithPriority(…, SDL_HINT_OVERRIDE)` 并断言返回真（环境变量设了 `SDL_JOYSTICK_HIDAPI=1` 之类时，
     普通优先级设不上——审查方实测）；复原照旧 `SDL_ResetHint`。
   - 截图用例：拍到 `TempDir` 里、在那份上做断言；再尽力拷一份到构建目录的 `shots\` 给人看，拷不成不判红（不写固定文件名再读回来断言）。
   - 破势震动那条：照 `PadWiring` 的做法拷一份临时资产根，写一场专用的测试战斗（敌人架势 1、怕拳、气血厚），不再绑第 3 章野猪的平衡；
     「手柄 A → 战斗菜单 → `BattleView::onBreak` → 虚拟手柄收到震动」这条真实入口保留。

## 11. 验收（协调者亲自做，不信施工方的自报）

- 独占构建槽完整构建：全部测试绿（交接时 1406 条 + 新增），四道门禁（`VALIDATE_OK` / `SELFTEST_OK` / `MAPGEN_IN_SYNC` / `ARTGEN_IN_SYNC`）全过，零编译警告。
- 跑完测试后 `saves/settings.json` 不被碰：本机那份是 2026-09-27 手玩时存的（git 忽略；sha256 `e4b47f66…`），测试前后哈希与 mtime 都不变。
- 截图（按 RGB 逐像素比、比对脚本自带阳性对照）：默认设置下 `title / world / menu / battle / talk / fx` 与改动前逐像素相同
  （主菜单模糊底图里那块随墙钟变的 77 像素照 settings.md 的口径扣掉）；`settings`、`menu:settings`、`settings:keys` 三张是新真值，人工看一遍。
- 手柄提示的画面：施工方用软件渲染器拍对白框与主菜单在「按过手柄之后」的样子（不许为此改 `main.cpp` 或加命令行开关——那个文件另一条线正在改；
  用测试里现成的软件渲染器路子拍到构建目录下），路径写进实现记录。
- **真 Xbox 手柄手测**：本机眼下没接手柄，列进交接的「未验证」，交用户接上之后按第 12 节的手测单试。

## 12. 手测单（给接上真手柄的人）

1. 标题画面：十字键 / 左摇杆上下挪光标，A 进「新的旅程」；提示行变成「↑↓ 选择　A 确认」。
2. 世界：左摇杆斜推走得顺（不左右乱跳）；A 交谈；X 路径行动；Y、≡、B 都开主菜单；按住 B 不闪；⧉ 存盘；按一下键盘，提示换回键盘键名。轻推（没推过一半）转向时会先停一下、推深才换方向——换向算一次新的推，要推过一半（第 3 节），属设计。
3. 对白：A 翻页；按住 RT 快进；Y 回看。
4. 战斗：←→ 调蓄劲，A 确定，B / Y 返回，按住 RT 加速；打出破势时手柄震一下；设置里关掉「手柄震动」后不震。
5. 拔掉手柄再插上：接着能用；拔掉时正按着的方向不会「卡住一直走」。
6. 窗口切到后台时按手柄：游戏不动；推着摇杆切到后台、在后台松手、再切回来：人不会自己一直走。

## 13. 施工纪律

- 构建槽 `gamepad`（`& "H:\Work\Kys\fanren\build_logged.bat" gamepad`，**PowerShell 调**；日志 `build-gamepad.log` 读字节按 gb18030 解码，读前核 mtime）。
- **另一条线正在同一棵树里改「双击 exe 就能进」**：`src/main.cpp`、`src/io/GameRoot.*`、`tests/GameRootTests.cpp`、`docs/handoff.md`、`saves/README.md`、
  仓库根 `README.md`。**这些文件一律不碰**（交接与按键说明由协调者最后统一改）。构建红了先看报错文件的 mtime 是不是落在你构建期间（别人改到一半），再动手。
- 不派子代理；不整份重读大文件；最小改动足迹，不顺手重构；汇报精炼（改了哪些文件、测试数、门禁、截图路径、已知问题）。
- 不写仓库里的 `saves/settings.json`（测试写临时目录）。新文件放进已有目录，CMake 按 glob 收，不改 CMakeLists。
- 同步改文档：`docs/settings.md`（设置项表加一行、行号、文件样例、改键面板的手柄列、4.4 节连发规则、第 6 节的 `.pad` 提示，并指向本文）、
  `docs/interfaces-octo-ui.md`（提到按键提示的地方）、`docs/README.md` 索引加本文一行。
- **不提交、不推送**。

---

## 实现记录

（施工方在这里补：与契约不同之处及理由、RED 的证据、失去判别力的旧断言与补法、测试清单、截图路径。）

### 2026-09-28，构建槽 `gamepad`

**落地文件**
- 新增：`tests/GamepadTests.cpp`。
- 改动：`src/engine/Engine.{h,cpp}`（第 9 节的接口；手柄状态、事件、`Impl::press` 单点；init 开手柄子系统、shutdown 关句柄）、
  `src/game/Application.{h,cpp}`（`text()` 的 `.pad` 与 `{pad.}`；`applySettings` 推 `padRumble`）、`src/game/SettingsScene.{h,cpp}`（第 8 行）、
  `src/game/MenuScene.cpp`（摘要九项）、`src/game/KeyConfigScene.{h,cpp}`（手柄列、固有字表）、`src/game/BattleView.cpp`（两处震）、
  `src/io/SettingsFile.{h,cpp}`（`pad_rumble`）、`data/text/ui.json`（11 条里的 9 条 `.pad` + `ui.settings.pad_rumble` / `ui.settings.desc.pad_rumble` /
  列头 `ui.keys.col.gamepad`——初版叫 `ui.keys.col.pad`，整改轮改名）、`data/text/ui_battle.json`（2 条 `.pad`）；测试 `tests/{KeyBindingTests,SettingsWiringTests,SettingsFileTests,UiStyleTests}.cpp`；
  文档 `docs/settings.md`、`docs/interfaces-octo-ui.md`、`docs/README.md`、本文。`main.cpp`、`GameRoot.*`、`handoff.md`、`saves/README.md`、根 `README.md` 没碰。
- 新文案全是从本文第 6–8 节的表里按字节取出来写进 JSON 的（脚本取，不手抄），测试里写死的 22 个期望串另用脚本与本文的表逐字节对过。

**与契约不同之处 / 契约没写死、施工时定下的**
1. `ui.keys.col.pad`（初版第 7 节写死的列头 id）也以 `.pad` 结尾，长得像某一条的手柄版，其实不是。初版照抄、在 `KeyPrompts` 里开了个特例；
   **整改轮已按改过的契约改名 `ui.keys.col.gamepad`，特例删掉**，改成钉「以 `.pad` 结尾的 id 恰是那 11 条手柄版」这条规矩（见下「整改轮」）。
2. 摆位：改键面板 840 → 980 宽（x 220 → 150，仍居中），手柄列从面板左边 800 起、左对齐（最宽的「十字↑ / 摇杆↑」18 号字 126 像素，右边还空 54），
   原有各列离面板左边的距离不变；设置面板高 592 → 622、y 64 → 49（仍上下居中），说明行与按键提示的间距不变。
3. 为了钉住手柄列的字，`KeyConfigScene` 多了一个静态 `padKeysText(Key)`（面板画的就是它）。
4. 契约没说的几处，按「最小惊讶」定：抓键期间被「不理」的手柄键（B 以外）不切设备；手柄上没有 PadButton 对应的键（Xbox 键、摇杆按下、
   分享键、背键、触摸板）连状态都不记；初版里右摇杆的轴事件对不在表里的 `which` 也会现建一份空状态（整改轮 S1 已改：右摇杆、越界的轴不建状态）；
   「拔掉的是最后一个手柄」按 `gamepadCount() == 0`（只数句柄非空的）判。引擎的震动开关缺省开，与 `io::Settings` 同。
5. 第 10 节第 5 条「打出一次破势 → 虚拟手柄收到震动」：原想借截图口 `battle:<编成>:mid`（AI 替两边打到头一次破势）省事，实测不行——
   新开局的韩立（气血 10）第一回合就倒；载第 3、4、5 章末的存档，AI 也是一路打到输或把敌人直接打死，没有破势
   （用本槽的 `fanren.exe` 拍了 8 张 `battle:<编成>:mid`，看了其中 6 张，全是打输或最后一击的定格）。
   改成**真用手柄打**：软件渲染器的真 `Application` + 虚拟手柄，谷外野猪 `be03_guwai_yezhu`（气血 26、防 4、架势 2、破绽有「拳」），韩立气血改成 500，
   轮到我方就按虚拟手柄 A（「攻击」→ 第一个敌人），第二拳时野猪剩 10 点、架势见底 → 破势 → `BattleScene` 把事件演给 `BattleView::onBreak` → 震。
   「谷外饿狼」不行：狼 18 点气血，第二拳直接打倒，打倒的那一下不削架势。
   （整改轮 LOW-3：不再借野猪，改成临时资产根里的专用测试战斗，见下。）
6. 手柄提示的截图由 `PadScreen.AfterAPadPressTheDialogueAndTheMainMenuNameThePadsButtons` 拍（拍法与截图口相同），落在测试程序所在目录的 `shots\`
   （`SDL_GetBasePath()`，即 `build-<槽>\shots\`）——每次跑测试都会重拍，协调者自己的槽里也会有一份。
   （整改轮 LOW-2：改成拍进用例自己的 `TempDir`、断言读那一份，再尽力拷一份到 `shots\`，拷不成不判红。）
7. `SettingsScene::settingsStrings` 加了 `ui.settings.hint.pad`，`KeyConfigScene::keyConfigStrings` 加了列头与四条 `.pad`；标题、主菜单、对白、
   路径行动、战斗那几条 `.pad` 没往各自场景的固有字表里加（最小足迹），由 `SettingsWording` 那条用例按 id 字面量逐条扫。
8. 对白截图里的名签写「han_li」：沿用协调者旧截图口 `talk:ui.ending.continue_hint:han_li` 的同一句（角色 id 是 `hanli`，那是旧口径，与本改动无关）。

**RED 的证据**（日志副本在协调者 scratchpad 的 `gamepad-agent\red1.log`、`red2.log`）
- RED-1（纯函数与引擎全是桩、连发规则未改）：1447 条里 33 条红——第 3 节纯函数全部、引擎层手柄用例全部，以及
  `PadWiring.HoldingTabForASecond…`：**主菜单开了 7 次、合了 6 次**；`…HoldingEsc…`：开 7 次、合 7 次、一秒末是合着的；
  `KeyRepeat.OnTheKeyboardOnlyTheFourArrowsRepeat`：确认、取消、菜单、快进、存盘、路径行动按住 400ms **各出 4 次**。
- RED-2（手柄全接好、连发规则仍是旧的）：7 条红——Tab / Esc 同上；`…HoldingPadY…`：**开 7 次、合 6 次**；`…HoldingPadB…`：开 7、合 7、末了合着；
  `KeyRepeat.OnAPadOnlyTheFourDirectionsRepeat`：A、B、Y、⧉、X、RT 按住 400ms 各出 4 次（另 1 条是我自己用例写错：RT 已把设备切成手柄，
  又断言 LT 之前还是键盘——改成先按一下键盘再验 LT）。
- GREEN：`Impl::press` 里只给方向键设重复计时之后，上面全绿（按住一秒：开 1 次、合 0 次、一直开着）。

**失去判别力的旧断言与补法**（逐条查了全部带「按住」/ 真时间的旧用例：`EngineTests`、`KeyBindingTests`、`SettingsWiringTests`、`UiTests` 里用
`SDL_Delay` / `SDL_GetTicks` 或跨帧按着键的那些；别处的测试都是按下、松开成对，碰不到连发）
- `HeadlessKeys.WhileCapturingNoLogicalKeyFiresAndTheNextPressIsTaken`「抓键期间按住的键不自动重复」：拿的是确认键，改完之后确认键本来就不连发，
  **失去判别力**。补 `HeadlessKeys.WhileCapturingAHeldArrowDoesNotRepeatEither`：同一件事换成本来就连发的 ↓，并带阳性对照（不抓键时按住 ↓ 300ms 确实连发）。
  旧用例一字没改。
- `HeadlessEngine.AltEnterTogglesFullscreenAndIsNeverAConfirm`「按住 300ms 之后冒出了 Confirm」：同理失去判别力；紧跟着那句
  `EXPECT_FALSE(engine.keyDown(Confirm))` 还兜着（Alt+Enter 若没被吞掉，Enter 会记成按住）。核过，没改。
- `HeadlessKeys.AnUnmappedKeyWithScancodeZeroIsNoKeyAtAll`「按住过了首次重复延迟」：H1 那个 bug 会命中的 8 个空格位里有 4 个是方向键（照旧连发），
  另 4 个有同一行的 `keyDown` 断言兜着——判别力还在，没改。
- `SettingsWiring.HoldingRightOnATwoStateRowFlipsItOnceNotOnEveryRepeat`、`HeadlessEngine.ThrottlesKeyRepeatAfterTheFirstTrigger`：用的是 → / ↓，照旧连发，
  各自的先验（`presses ≥ 3` / 350ms 后再出一次）照旧成立，没改。

**真值变了的旧用例**（都是收紧或换成新真值，没有一条放宽）
- `SettingsWiringTests.cpp` 顶上的 `static_assert`：`kKeys 8 → 9`、`kRestore 9 → 10`、`kRowCount 10 → 11`，并加 `kPadRumble == 8`（第 8 节的行号）。
- `SettingsWiring.TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack`：从「文字速度」到「按键设置」要按两下 ↓（中间隔着「手柄震动」），
  多断言一次停在 `kPadRumble`。
- `SettingsWording.ThePanelAndBothEntrancesPassTheForbiddenWordScan`：行名表 10 → 11 个（第 8 行「手柄震动」）并加表长断言；用词表下限 40 → 43、
  改键面板 35 → 40；扫描加上全部 11 条 `.pad`；钉新说明与列头原文。
- `SettingsFileTests.cpp`：`expectFactoryDefaults` 加 `padRumble` 为真（`SettingsDefaults.AreTheBehaviourBeforeTheSettingsExisted` 等三条用它）；
  `everythingChanged` 把 `padRumble` 也改掉（往返那条因此连它一起验）；`SettingsFile.TheFileNamesEveryFieldWithTheContractsWords` 加 `"pad_rumble"` 两种写法与「排在 text_speed 后面」。
- `SettingsWiring.DefaultKeyPromptsAreByteForByteTheOldText`、`EngineTests.MapsScancodesToLogicalKeys` 一字没改，照样绿。

**测试**（1413 → 1458，+45；1413 = 交接时的 1406 + 另一条线的 `GameRootTests` 7 条）
- `tests/GamepadTests.cpp`（42 条）：`PadTable.` × 2、`PadLabels.FollowTheContractsTable`、`StickDirection.` × 6、`TriggerDown.HalfwayPressesAndThirtyPercentKeepsItPressed`、
  `HeadlessPad.` × 13（SDL_PushEvent：A 按下按住松开、每个键各管各的、扳机过半 / 回落与 LT、十字↓ 连发、左摇杆四向、换向不经中心、同值两次、两个手柄、
  REMOVED、LB / Xbox 键 / 摇杆按下不出键不切设备、设备跟最后一下有意义的按键走、抓键时 B 作罢别的不理、震动开关无头存值）、
  `VirtualPad.` × 4（接上被打开、按键摇杆扳机到得了逻辑键、拔掉全松开且回键盘、先插后开子系统照样打开、震动只在「最后按它 + 开关开」时到得了）、
  `KeyRepeat.` × 2（键盘 / 手柄：六个非方向键按住 400ms 各一次，方向键照旧连发）、`PadWiring.` × 9（按住 Tab / Esc / Y / B 一秒的四条 RED 用例；
  Y 开主菜单、B 合上、≡ 也开；手柄 A 翻对白；11 条提示键盘模式逐字节是旧的、按过手柄是第 6 节最后一列、按键盘又换回；改键面板手柄 A 进抓键说明行是 .pad 版、
  B 作罢；设置面板第 8 行 ←→ 同变、关面板写盘读回相等、恢复默认回到开）、`KeyPanelPadColumn.EachRowNamesThatActionsPadButtonsSeparatedBySlashes`、
  `PadRumbleWiring.OnlyABreakOrAHeavyEnemyHitOnOurSideRumbles`（普通命中、我方打敌人不震；敌人一下 ≥ 1/4 或打倒、破势震；震屏关照样震；开关关不震）、
  `PadScreen.ABreakInARealFightRumblesThePadThatWasPressedLast`（见上第 5 条）、`PadScreen.AfterAPadPressTheDialogueAndTheMainMenuNameThePadsButtons`（截图 + 右下角提示那一块
  有像素变、左下角对照块一个像素不变）。
- 别处 3 条：`HeadlessKeys.WhileCapturingAHeldArrowDoesNotRepeatEither`、`KeyPrompts.PadPlaceholdersOnlyLiveInTheElevenPadPrompts`（原始字节数 18 处 `{pad.` 对账）、
  `SettingsFile.PadRumbleDefaultsOnRoundTripsAndAWrongTypeFallsBackWithAWarning`。

**截图**（`build-gamepad\shots\`）
- 软件渲染器拍的（`PadScreen` 那条用例）：`pad_talk.png`（对白框右上「Y 回看　长按 RT 快进」）、`pad_talk_keyboard.png`（同一帧键盘版）、
  `pad_menu.png`（主菜单右下「Y 关闭　B 返回」）、`pad_menu_keyboard.png`。
- `fanren.exe` 截图口拍的三张新真值：`settings.png`（`--scene settings`，11 行，「手柄震动 〈 开 〉」在文字速度之后、左括号到头变暗）、
  `menu_settings.png`（`--scene menu:settings`，摘要九项）、`settings_keys.png`（`--scene settings:keys`，最右「手柄」一列）。看过：字没出框、没重叠，新行 / 新列在该在的位置。
- 施工方自己先比了一遍（协调者的 `shots.ps1` 拍全套、`pixdiff.py` 按 RGB 比协调者的 `base1\`，自检过）：title、world、world_night、menu、battle、
  battle_intro、talk、fx 八张 IDENTICAL；只有三张新真值不同——menu_settings 只在 (396,456)–(566,476)（多出来的「手柄震动　开」那一行），
  settings、settings_keys 是面板本身换了尺寸。

**已知问题 / 未验证**
- 真 Xbox 手柄没测（本机没接）：第 12 节手测单整张交给用户。尤其第 6 条「切到后台时松手」：SDL 缺省在后台不发手柄事件，失焦时补不补发松开 / 回中，
  没实测（第 4 节原话）。
- SDL 对「与正在震的强度相同」的请求不再转给手柄、只把时长续上（探针实测）：280ms 内连着两次破势，手柄上是一段连着的震，不是两下。
- 协调者 scratchpad 里原来的 `padprobe.py` 被施工方误覆盖（同名），原内容没了；施工方的探针挪到了 `scratchpad\gamepad-agent\` 下。

### 整改轮（2026-09-28，独立审查 1 HIGH、4 LOW；契约第 3、4、7 节与第 10 节第 9 条、第 11 节先由协调者改好）

**RED 的证据**（先只加回归用例、引擎没动就构建：`scratchpad\gamepad-agent\red3_high1.log`，1463 条里恰好这 5 条红）
- `HeadlessPad.LettingGoOfADiagonalInOneReportPressesNothingNew`：7 条轨迹 × 带 / 不带 `UPDATE_COMPLETE` 两种，5 条轨迹两种都红——
  右偏下 30° 一份回中、两份回中、右偏下 45° 两份回中（还**留了一帧按住「下」**）、左偏下 30° 各出一次「下」，右偏上 30° 出一次「上」；
  两条对照 (1, 0)、右偏下 20° 照旧绿。
- `VirtualPad.LettingGoOfADiagonalInOneUpdatePressesNothingNew`：SDL 真链路一次更新回中，出「下」。
- `VirtualPad.UnpluggingWhileHoldingADiagonalPressesNothing`：斜推着拔线那一帧出「下」。
- `PadWiring.ADiagonalStickPushAndLetGoLeavesTheSettingsCursorWhereItWas`：设置面板光标 **0 → 1**（与审查方实测相同）。
- `StickDirection.SwitchingToANewWayIsAFreshPushThatNeedsHalfway`：按住右时 (0.1, 0.45) 旧函数给「下」，契约要空。
- GREEN：引擎按下面改完，1463 条全绿；另在环境变量 `SDL_JOYSTICK_HIDAPI=1` 下单跑 `VirtualPad.* / PadRumbleWiring.* / PadScreen.*` 共 9 条，全绿
  （对照：审查方的 `hint_env.py` 实测普通优先级 `SDL_SetHint` 在这个环境变量下返回 false、值仍是 1）。

**改了什么**
- HIGH-1（`src/engine/Engine.cpp`）：`Impl::padAxis` 的 LEFTX / LEFTY 只存值、记 `stickFresh`；新增 `Impl::settleStick`（有新值才按第 3 节判一次、清标记），
  `SDL_EVENT_GAMEPAD_UPDATE_COMPLETE` 调它一次，`pollEvents` 事件循环之后对每个手柄再调一次（排在抓键那道闸之前：抓键期间摇杆的物理状态照记）；
  `REMOVED` 整份删状态，没判的新值随之作废。扳机照旧收到就判。
- 第 3 节新规矩（`Engine::stickDirection`）：原方向继续按住要「偏离 ≤ 55° 且 r ≥ 0.35」；否则一律按「从零推起」判，要 r ≥ 0.5（换向算一次新的推）。
  `Engine.h` 那条注释跟着改。
- S1：`padAxis` 只在用到状态的分支里建状态，右摇杆、越界的轴不建。
- LOW-1：`TestPad::disableRealDrivers` 改用 `SDL_SetHintWithPriority(…, SDL_HINT_OVERRIDE)` 并 `ASSERT_TRUE`，三个夹具用 `ASSERT_NO_FATAL_FAILURE` 接；复原照旧 `SDL_ResetHint`。
- LOW-2：`PadScreen::shoot` 拍进用例自己的 `TempDir`、断言读那一份；再尽力拷到 `SDL_GetBasePath()\shots\`（取不到基路径就不拷，拷失败不判红）。
- LOW-3：`PadScreen` 改用一份临时资产根（照 `PadWiring` 拷 data/scripts/maps），再写进一个测试角色 `pad_test_dummy`（架势 1、怕「拳」、气血 999、攻 1、速 1）
  与一场测试战斗 `pad_test_break`；破势那条照旧是「手柄 A → 战斗菜单 → `BattleView::onBreak` → 虚拟手柄收到震动」。仓库的 data/ 一个字节没动。
- 改名：`ui.keys.col.gamepad`（`data/text/ui.json`、`KeyConfigScene.cpp` 两处、`UiStyleTests`）；`KeyPrompts` 里为旧名开的特例删掉，改成扫全部文案 id：
  以 `.pad` 结尾的必须在那 11 条白名单里、集合恰是白名单，另钉列头在新名字上。
- LOW-4 不改，记一句：改键面板透出底下世界层的任务框只出现在截图口（`--scene settings:keys` 把面板直接压在世界层上）；游戏里只从标题画面
  （不透明）或主菜单（底图就绪后不透明、只画模糊底图）进，玩家看不到。

**新增 / 改动的用例**（1458 → 1463，+5）
- 新增：`StickDirection.SwitchingToANewWayIsAFreshPushThatNeedsHalfway`、`HeadlessPad.LettingGoOfADiagonalInOneReportPressesNothingNew`、
  `VirtualPad.LettingGoOfADiagonalInOneUpdatePressesNothingNew`、`VirtualPad.UnpluggingWhileHoldingADiagonalPressesNothing`、
  `PadWiring.ADiagonalStickPushAndLetGoLeavesTheSettingsCursorWhereItWas`（带配对：同一根摇杆往下推一下光标确实下移）。
- 改动：`PadScreen.ABreakInARealFightRumblesThePadThatWasPressedLast`（改用测试战斗、先验测试战斗读进来了）、
  `PadScreen.AfterAPadPressTheDialogueAndTheMainMenuNameThePadsButtons`（经 `shoot` 改拍临时目录）、`KeyPrompts.PadPlaceholdersOnlyLiveInTheElevenPadPrompts`（上面那条规矩）、
  `SettingsWording.ThePanelAndBothEntrancesPassTheForbiddenWordScan`（列头 id 改名）；三个带虚拟手柄的夹具（`VirtualPad`、`PadRumbleWiring`、`PadScreen`）换了关驱动的写法。
  旧用例没有一条放宽；`HeadlessPad.SwingingTheStickRound…` 照旧绿（它把两轴分两帧塞，本来就测不到半截状态——这回由上面几条补上）。

**收尾验证**：删掉 `build-gamepad` 从零构建（176 个编译单元），1463 条全绿、四道门禁全过、零编译警告；`saves/settings.json` 哈希
（`e4b47f66…`）与 mtime（2026-09-27 10:50:27）不变。`shots.ps1` 全套与第一轮逐像素相同（列头改名不改字），与协调者 `base1\` 比仍是
八张 IDENTICAL、三张新真值不同。

### 收尾小轮（2026-09-28，复审放行后的两条可选 LOW）

- LOW-A：新增 `HeadlessPad.AStickPushedDuringACaptureDoesNotFireWhenAKeyEndsIt`，钉住「一帧末尾补判摇杆排在抓键闸之前」这个次序：抓键期间把摇杆推到位
  （带 / 不带 `UPDATE_COMPLETE` 各一遍，每一遍先按一下键盘、从「设备是键盘」起），用键盘键结束抓键、摇杆还推着——结束那一帧与之后 320ms 不出「右」、
  不连发，设备仍是键盘。带 `UPDATE_COMPLETE` 那一遍在报告末尾当场就判了，与那个次序无关，只是一并钉住真实次序下的行为；判别力在不带的那一遍。
  **变异验证：挪到闸后 → 红**（只在构建槽里临时挪、重编跑这一条：不带 `UPDATE_COMPLETE` 那一遍结束抓键那一帧出了「右」、设备切成手柄、
  之后 320ms 连发 2 次；输出在 `scratchpad\gamepad-agent\mutation_lowA.txt`）。挪回去（`Engine.cpp` 用备份按字节复原，sha256 与挪之前相同）重编 → 绿。
- LOW-B：第 12 节手测单第 2 条末尾补了「轻推转向会先停一下、推深才换方向」那一句（属设计）。
- 用例 1463 → 1464。
