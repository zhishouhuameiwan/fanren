# 系统设置（施工契约）

> 2026-09-27 用户拍板：首批 7 项 + 画面特效档位 + Alt+Enter 切全屏 + 改键 + 垂直同步开关。
> 分两期：**一期** = 除改键以外的全部（含设置文件里 `keys` 字段的读写往返）；**二期** = 改键（引擎键位表、改键面板、提示文案跟着键位走）。
> 本文是派工契约：词表、默认值、文件格式、接口名在这里写死一种，施工照抄，要改先改这里。

---

## 1. 设置项

| # | 项 | id（文件里的键） | 取值 | 默认（= 改造前的行为） | 落点 |
|---|---|---|---|---|---|
| 0 | 音乐音量 | `bgm_volume` | 0–10 | 10 | BGM 轨道增益 |
| 1 | 音效音量 | `sfx_volume` | 0–10 | 10 | 音效轨道池的增益（见 4.1） |
| 2 | 显示模式 | `fullscreen` | 窗口 / 全屏 | 窗口 | `SDL_SetWindowFullscreen`，无边框桌面全屏 |
| 3 | 画面缩放 | `scale` | 整数倍 `integer` / 铺满 `fit` | 整数倍 | 逻辑呈现 `INTEGER_SCALE` / `LETTERBOX` |
| 4 | 垂直同步 | `vsync` | 开 / 关 | 开 | `SDL_SetRenderVSync`；关时帧率封顶 240（见 4.2） |
| 5 | 画面特效 | `effects` | 完整 `full` / 精简 `lite` | 完整 | 精简 = 关辉光、关景深（见 4.3） |
| 6 | 战斗震屏 | `screen_shake` | 开 / 关 | 开 | BattleView 触发震屏的那一处 |
| 7 | 文字速度 | `text_speed` | 慢 `slow` 30 / 标准 `normal` 60 / 快 `fast` 120 / 瞬显 `instant`（字/秒） | 标准 | DialogueScene 的逐字速度 |
| 8 | 手柄震动 | `pad_rumble` | 开 / 关 | 开（有意的例外，见下） | `Engine::rumble` 那一道闸（`docs/gamepad.md` 第 8 节） |
| 9 | 按键设置 | `keys` | 见第 6 节 | 见第 6 节 | 二期 |
| 10 | 恢复默认 | — | — | — | 全部（含键位、手柄震动）回到默认 |

- 音量档位 → 线性增益：`gain = (level / 10)²`。0 = 静音，10 = 1.0（改造前的混音响度，`docs/audio.md` 的响度口径不动）。平方曲线是听感上「每档差不多一样大」的近似。纯函数，单测钉住 0→0、10→1、单调。
- **默认值一律等于改造前的行为**：不开设置面板的玩家，画面、声音、手感与今天逐像素 / 逐采样相同。
  唯一的例外是 #8 手柄震动（2026-09-28 加，`docs/gamepad.md` 第 8 节）：改造前根本不认手柄，没有「以前」可对，缺省开；
  键盘玩家永远感觉不到它（最后一下按的不是手柄就不震）。它插在「文字速度」之后，「按键设置」「恢复默认」各往后挪了一行。

## 2. 存盘

- 路径：`<资产根>/saves/settings.json`（与 `quick.sav` 同目录）。**与存档分开**：设置属于这台机器，不属于某一局；存档格式仍是 v8，一个字节不动。
- 格式（UTF-8 JSON，nlohmann）：

```json
{
  "version": 1,
  "bgm_volume": 10,
  "sfx_volume": 10,
  "fullscreen": false,
  "scale": "integer",
  "vsync": true,
  "effects": "full",
  "screen_shake": true,
  "text_speed": "normal",
  "pad_rumble": true,
  "keys": { "up": [26, 0], "confirm": [29, 44], "action": [8, 20] }
}
```

- `pad_rumble`（2026-09-28，`docs/gamepad.md` 第 8 节）：缺字段 → 开；类型不对 → 开 + 警告（与别的布尔字段同一个写法）；`version` 仍是 1。

- `keys`：动作 id（第 6 节）→ 两格自定义键位的 **SDL 扫描码数值**，0 = 空格位。**存数不存名**：`SDL_GetScancodeName` 的名字按 SDL 文档跨平台不稳定、还有重名（RETURN 与 RETURN2 都叫 "Return"），不能拿来做双向映射；扫描码数值就是 USB HID usage，稳定。缺省某个动作 = 那个动作用默认。
- **读盘宽松、写盘完整**：
  - 文件不存在 → 全默认，不算错（第一次启动）。
  - JSON 坏了 → 全默认，返回失败原因（调用方打一行警告），**不挡启动、不覆盖那个坏文件**——直到玩家在面板里改了什么才写。
  - 缺字段 → 那一项默认；数值越界 → 夹紧；枚举认不出 → 那一项默认 + 警告；多出来的字段 → 忽略。
  - `keys` 整张表校验不过（固定键 / 保留键 / 重复 / 某个动作一个键都不剩 / 扫描码越界）→ **键位整张回默认** + 警告。只回滚一个动作会造出别的冲突，整张回滚的结果才可预期。
  - 写盘永远写全部字段。
- **谁读、谁写**（与 `setEncountersEnabled` 同一个套路——缺省关，启动器打开）：
  - `Application` 缺省**不碰文件**：`settings()` 是默认值，`saveSettings()` 是成功的空操作。几百条无头测试因此永远拿默认值，也永远不会写仓库里的 `saves/settings.json`。
  - `main.cpp`：正常游玩（标题流程、`--load`、`--map`）调 `enableSettingsFile(路径)`，路径 = `--settings <文件>`，没给就是默认路径。**`--headless` 永远不读**；**`--screenshot` 只在显式给了 `--settings` 时读**（拍出来的图不受开发机上的偏好左右，要拍某种设置下的画面就显式指一份）。
  - 写：设置面板关闭时（有改动才写）；Alt+Enter 切了全屏时。写失败要说出来（面板上那一行），不静默。

## 3. Application 接口

```cpp
// ---- 系统设置（docs/settings.md）----
[[nodiscard]] const io::Settings& settings() const;
// 存进来并立刻生效：音量、全屏、缩放、垂直同步、特效档位、键位推给引擎；
// 震屏、文字速度由 BattleView / DialogueScene 读 settings()。
void setSettings(const io::Settings& settings);
// 记住路径并读盘（规则见第 2 节）。坏文件：已按默认生效，返回失败原因给调用方打警告。
core::Result<bool> enableSettingsFile(const std::string& path);
// 写到 enable 过的路径；没 enable 过 = 成功的空操作（value 为 false）。
core::Result<bool> saveSettings();
[[nodiscard]] std::string defaultSettingsPath() const;   // <资产根>/saves/settings.json
```

- `io::Settings`（`src/io/SettingsFile.h`）与读写函数放 io 层：io 不依赖 engine，所以文件里的 `keys` 就是「动作 id → 两个 int」，由 Application 经引擎的词表（`Engine::keyId` / `keyFromId`）换成 `Engine::Key`。
- `systemSceneActive()` 把设置面板、改键面板算进去：开着它们的时间记 `playSecondsSystem`，与主菜单同口径。
- Alt+Enter 是引擎自己切的：`Application::tick` 每帧比一次 `engine.fullscreen()` 与 `settings().fullscreen`，不一致就以引擎（窗口的真实状态）为准改 settings 并写盘。

## 4. 引擎接口（`src/engine/Engine.h`，SDL 类型仍然一个都不出现在头文件里）

### 4.1 音频
- `setBgmVolume(float gain)` / `setSfxVolume(float gain)`，读回 `bgmVolume()` / `sfxVolume()`。**无头也存值**（测试读回）。
- BGM：`MIX_SetTrackGain(bgmTrack, gain)`。
- 音效：`MIX_PlayAudio` 是「发完即忘」，SDL3_mixer 3.2.4 的文档写明它**不接受任何混音参数**，调不了音量。改成 init 时建一个 **12 条轨道的池**；`playSfx` 取一条空闲的（`!MIX_TrackPlaying`），全忙就抢最早开始的那条；每条轨道的增益 = 音效音量。12 条够：连击 1+N 最多 6 下命中音，加上界面音效仍有余量。

### 4.2 显示
- `setFullscreen(bool)` / `fullscreen()`：后者读窗口的**真实**状态（`SDL_GetWindowFlags`）；无头读存的值。
- `setIntegerScale(bool)`：`SDL_SetRenderLogicalPresentation(1280, 720, on ? INTEGER_SCALE : LETTERBOX)`。铺满时回读后缓冲（`snapshotBackbuffer`、`captureFrame`）拿到的内容区不再是 1280×720 的整数倍——`readBackbufferContent` 已经按呈现矩形裁；**主菜单的模糊底图、开战碎屏在铺满 + 全屏下要实拍确认没错位**。
- `setVSync(bool)`：`SDL_SetRenderVSync(renderer, on ? 1 : 0)`，失败记一行日志。**关的时候 `endFrame` 里补睡到 1/240 秒**：关垂直同步是为了少一帧延迟，不是为了让一个核空转、笔记本发烫。
- **Alt+Enter**（主键盘 Enter 与小键盘 Enter 都认）：`pollEvents` 里切全屏，并**吞掉这次按键**——不记 `scancodeDown`、不触发 Confirm（否则长按 250ms 后还会自动重复出 Confirm）。

### 4.3 画面特效档位
- `enum class EffectsLevel { Full, Lite };`，`Engine::setEffectsLevel` / `effectsLevel()`。放在 Engine 上而不是 PostFx 的静态量：每个 Application 各一份，测试之间不串。
- `PostFx` 读 `engine_.effectsLevel()`，**单点**在 PostFx 里：Lite 时 `beginEmissive()` 返回 false（各场景本来就按「false 就别画发光体」处理）、`endScene` 跳过辉光与景深；光照、暗角、调色照做（便宜，而且夜景的昼夜感靠光照）。
- 这个开关只在退回软件渲染器时才有意义（后处理全开那时 60–80 ms/帧）；硬件渲染器下帧率未实测，默认完整。

### 4.4 输入与键位（二期）
- 扫描码对外是 `using ScanCode = int;`（SDL 扫描码数值）。
- 词表：`static const char* keyId(Key)`、`static std::optional<Key> keyFromId(std::string_view)`（id 见第 6 节）。
- `static std::vector<ScanCode> fixedKeys(Key)`、`static std::array<ScanCode, 2> defaultCustomKeys(Key)`、`std::array<ScanCode, 2> customKeys(Key) const`。
- `core::Result<bool> setCustomKeys(const KeyTable&)`：整张表一起设，校验不过**不改**、返回原因。
- 改一格的规则（第 6 节）做成**不碰 SDL 事件的纯函数**，单测直接喂表。
- 抓键：`beginKeyCapture()`；抓键期间下一次物理按下（非重复）**不映射成任何逻辑键**，由 `takeCapturedKey()` 取走。
  手柄（`docs/gamepad.md` 第 7 节）：抓键时手柄 B = 作罢（`takeCapturedKey()` 交出 `kEscapeCode`），别的手柄键不理、抓键继续。
- `static std::string keyLabel(ScanCode)`：显示名（第 6 节）。
- **连发只给方向键**（2026-09-28，`docs/gamepad.md` 第 5 节）：上下左右按住 250ms 后每 60ms 再出一次 `keyPressed`；确认、取消、菜单、
  快进、存盘、路径行动一次物理按下只出一次，按住不连发（`keyDown` 不变，快进靠它）。从前十个键一视同仁地连发，按住 Tab / Esc
  主菜单每 60ms 开合一次、按住 F5 每 60ms 存一次盘。改在「记一次刚按下」的那一处（`Engine::Impl::press`，键盘与手柄共用）。
- 手柄（Xbox 优先）的键位、摇杆、热插拔、提示跟设备走、震动见 `docs/gamepad.md`；键盘与手柄换算成同一套 10 个逻辑键，
  场景代码不分设备。

## 5. 界面

### 5.1 入口
- 标题画面：新的旅程 / 继续旅程 / **设置** / 离开（下标 `kNewJourney 0 / kContinue 1 / kSettings 2 / kQuit 3`）。
- 主菜单左栏：状态 · 物品 · 法门/法术 · 记事 · 存盘 · **设置** · 返回（`Page::Settings` 插在 `Save` 与 `Back` 之间）。光标停在「设置」时右栏写一句说明 + 当前几项的摘要；确认 → 压 `SettingsScene`，菜单照「记事」的做法只画模糊底图（`journalOpen_` 那一套），面板一收再成为顶层。
- 同一个 `SettingsScene` 两处共用；改键面板 `KeyConfigScene` 从设置面板的「按键设置」进。

### 5.2 设置面板 `SettingsScene`
```
┌──────────── 设置 ────────────┐
│ 声音                         │
│   音乐音量   ■■■■■■■■■■  10  │
│   音效音量   ■■■■■■■■□□   8  │
│ 画面                         │
│   显示模式     〈 窗口 〉      │
│   画面缩放     〈 整数倍 〉    │
│   垂直同步     〈 开 〉        │
│   画面特效     〈 完整 〉      │
│   战斗震屏     〈 开 〉        │
│ 游玩                         │
│   文字速度     〈 标准 〉      │
│   手柄震动     〈 开 〉        │
│   按键设置     ›              │
│ ───────────────────────────  │
│   恢复默认                    │
│ 〔光标这一行的说明〕            │
│ ↑↓ 选择　←→ 调整　Esc 返回    │
└──────────────────────────────┘
```
- 行的次序即第 1 节的 # 次序（测试按下标驱动）：`0 音乐音量 · 1 音效音量 · 2 显示模式 · 3 画面缩放 · 4 垂直同步 · 5 画面特效 ·
  6 战斗震屏 · 7 文字速度 · 8 手柄震动 · 9 按键设置 · 10 恢复默认`（`SettingsScene::kPadRumble = 8 / kKeys = 9 / kRestore = 10 /
  kRowCount = 11`；「手柄震动」2026-09-28 插进来，面板高 592 → 622、仍上下居中）。
  墨金主题，照 `docs/interfaces-octo-ui.md` 的面板画法；先压一层半透明墨色，底下（标题画面或主菜单模糊底图）就退成背景。
- 最后一下按的是手柄时，最下面那行提示换成手柄版「↑↓ 选择　←→ 调整　B 返回」（第 6 节 `.pad` 提示）。
- ←→ 调值，**立刻生效**（`app.setSettings`）；移动光标 `ui_cursor`，调值 `ui_cursor`，调音效音量时在新增益下响一声 `ui_confirm` 试听；音乐音量直接听正在放的曲子（标题画面放着 `bgm_title`）。
- 确认：「按键设置」→ 压改键面板；「恢复默认」→ 全部回默认，说明行写「已恢复默认」。
- Esc / X / 菜单键 → 关面板；有改动就 `saveSettings()`，**失败时留在面板上把原因写在说明行**，再按一次才关（这一局照样按改过的跑）。
- 已知取舍：只有栈顶场景会 update，从标题画面进来时标题背景的漂移会停住；压着的那层墨色让它读起来像「退成背景」，不另做。

### 5.3 改键面板 `KeyConfigScene`（二期）
- 十行动作（第 6 节的次序）+ 最后一行「恢复默认键位」。四列：固定（灰，不可改）/ 键位一 / 键位二 / **手柄**（灰、只读、光标选不到，
  2026-09-28 加：手柄键位固定，`docs/gamepad.md` 第 7 节）。手柄列每行写那个动作的手柄键显示名，多个用「 / 」隔开
  （`十字↑ / 摇杆↑` …… `A`、`B`、`Y / ≡`、`RT / RB`、`⧉`、`X`），「恢复默认键位」那一行空着。面板 840 → 980 宽、仍居中，原有各列离面板左边的距离不变。
- ↑↓ 选动作，←→ 选格，确认 → 说明行变成「按下新键（Esc 作罢，Backspace 清空）」，引擎进抓键；抓到后按第 6 节的规则落格，结果（成功 / 挪过来 / 拒绝的原因）写在说明行。
  用手柄进的抓键，说明行是手柄版「在键盘上按下新键（B 作罢）」；抓键时手柄 B = 作罢，别的手柄键不理——只拿手柄的玩家按 A 进了「按下新键」也出得来。
- 立刻生效（`app.setSettings`），关面板时随设置一起写盘。

### 5.4 文案
全部进 `data/text/ui.json`，前缀 `ui.settings.*`（设置面板、两个入口的字）与 `ui.keys.*`（改键面板、动作名）。面板上能出现的固有字由各自的 `static std::vector<std::string> …Strings(const core::GameData&)` 列出，`tests/UiStyleTests.cpp` 拿禁词表扫（与 `MenuScene::menuStrings` 同一套）。

### 5.5 截图口
`--scene settings`、`--scene settings:keys`（压在世界层上），`--scene menu:settings`（主菜单光标停在「设置」）。配合 `--settings <文件>` 拍某种设置下的画面。

## 6. 键位（二期；一期只做文件往返）

| id | Key | 面板上的名字 | 固定键（不可改、不可删） | 默认 键位一 / 键位二 |
|---|---|---|---|---|
| `up` | Up | 向上 | ↑ | W / — |
| `down` | Down | 向下 | ↓ | S / — |
| `left` | Left | 向左 | ← | A / — |
| `right` | Right | 向右 | → | D / — |
| `confirm` | Confirm | 确认 | Enter、小键盘 Enter | Z / 空格 |
| `cancel` | Cancel | 取消 | Esc | X / — |
| `menu` | Menu | 菜单 | — | Tab / — |
| `skip` | Skip | 快进 | — | Ctrl（左）/ — |
| `save` | Save | 存盘 | — | F5 / — |
| `action` | Action | 路径行动 | — | E / Q |

- 默认表与改造前的 `kBindings` 一一对应（19 条），`EngineTests.MapsScancodesToLogicalKeys` 不改一个字照样绿。
- **固定键**保证玩家永远不会把自己锁在菜单外：方向键、Enter、Esc 始终可用。
- **保留键**（不许绑）：左右 Alt（Alt+Enter）、左右 Win、Application 菜单键、Backspace 与 Delete（抓键时的「清空」）。
- **改一格**（把键 K 放到动作 A 的第 s 格，那格原来是 P，可能为空）：
  1. K 是任何动作的固定键或保留键 → 拒绝，说明是谁的固定键 / 留作他用。
  2. K 就在 (A, s) → 无事。
  3. K 在 A 的另一格 → 两格互换。
  4. K 在别的动作 B 的某一格 → 那一格换成 P（**互换**）；若 P 为空且 B 因此一个键都不剩 → 拒绝「「B」只剩这一个键，先给它另配一个」。
  5. 否则 (A, s) = K。
  - 清空一格：若 A 因此一个键都不剩 → 拒绝。
  - 不变式：**每个动作至少一个键**（固定或自定义）；两个动作永远不共用一个键。
- **显示名** `keyLabel`：字母、数字、F1–F12 照写；空格 → `空格`；Return → `Enter`；小键盘 Enter → `小键盘Enter`；Esc、Tab 照写；方向 → `↑↓←→`；左 Ctrl → `Ctrl`、右 Ctrl → `右Ctrl`；左 Shift → `Shift`、右 Shift → `右Shift`；小键盘数字 → `小键盘0`…；其余用 `SDL_GetScancodeName`，空名 → `键#<数值>`。
- **设置文件只存与默认不同的动作**（`keys` 里缺省的动作 = 那个动作用默认）。这是有意的取舍：「全部默认」就是空表，
  改了又改回去不会凭空多出一份文件，将来调整默认键位也能自动带给没改过那个动作的玩家。代价是：**将来改默认键位时，
  若新的默认键撞上某位玩家已存的自定义键**（例如把「快进」的默认从左 Ctrl 改成 J，而这位玩家已把 J 配给了「菜单」），
  叠出来的整表就有重复，读盘时整表校验不过 → **这位玩家的键位整张回默认**（打一行警告，设置文件不被覆盖，
  直到他在面板里改了什么）。改默认键位的那一次要在交付说明里点出这一条；要避免，就得换成「存全表」并带版本迁移，眼下不值当。
- **提示文案跟着键位走**：含可改键名的提示改成占位符 `{key.<id>}`，展开成那个动作当前第一个自定义键（没有自定义键就取第一个固定键）的显示名。展开的单点是 `Application::text()`。**并非全部文案都经它**：告示板（`BoardScene`）、路径行动面板（`PathActionScene`）、章节卡（`ChapterCardScene`）、设置面板与改键面板的固有字表直接调 `GameData::lookupText`，占位在那里不会展开——所以占位**只许出现在经 `text()` 显示的下面四条上**，`KeyPrompts.PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText` 扫 `data/text/**` 钉着白名单（id 认得、括号闭合）。默认键位下展开结果与改造前的字面**逐字节相同**（测试钉）。要改的四处：
  - `ui.menu.keys`：`{key.menu} 关闭　Esc 返回`
  - `ui.battle.hint.menu`：`↑↓ 选择 · ←→ 蓄劲 · 回车 确定 · Esc / {key.menu} 返回 · 按住 {key.skip} 加速`
  - `ui.battle.hint.watch`：`按住 {key.skip} 加速`
  - `DialogueScene.cpp` 里写死的 `"Tab 回看　长按 Ctrl 快进"` → 新文案 `ui.dialogue.keys`：`{key.menu} 回看　长按 {key.skip} 快进`
  - 只含固定键（↑↓、Enter、回车、Esc）的提示不动。
- **提示也跟着设备走**（2026-09-28，`docs/gamepad.md` 第 6 节，那边是这一条的契约）：最后一下按的是手柄、且文案表里有
  `<id>.pad`，`Application::text()` 就取那一条；手柄版里的占位是 `{pad.<id>}`，展开成那个动作第一个手柄键的显示名
  （手柄键位固定：确认 `A`、取消 `B`、菜单 `Y`、快进 `RT`……）。有 `.pad` 版的是这 11 条：`ui.title.keys`、`ui.menu.keys`、
  `ui.dialogue.keys`、`ui.path.keys`、`ui.settings.hint`、`ui.keys.hint`、`ui.keys.capture`、`ui.keys.desc`、`ui.keys.desc.restore`、
  `ui.battle.hint.menu`、`ui.battle.hint.watch`（全文与默认展开见 `docs/gamepad.md` 第 6 节那张表）。**键盘模式下不取 `.pad`**：
  展开结果与从前逐字节相同（`SettingsWiring.DefaultKeyPromptsAreByteForByteTheOldText` 一字没改照样绿）。
  `.pad` 里只许有 `{pad.*}`、不许有 `{key.*}`；`{pad.` 只许出现在那 11 条的 `.pad` 版里，每条 `.pad` 都有键盘版本体——
  `KeyPrompts.PadPlaceholdersOnlyLiveInTheElevenPadPrompts` 扫 `data/text/**` 钉着（与上面那条 `{key.` 的白名单同一个做法）。

## 7. 测试（最低清单；照上一轮教训，**至少一条走玩家碰得到的真实入口**）

1. `SettingsFile`：缺文件 = 默认且成功；往返逐字段相等；坏 JSON = 默认 + 失败；越界夹紧；认不出的枚举回默认；`keys` 往返；非法 `keys` 整张回默认。
2. 纯函数：音量曲线（0→0、10→1、单调）；文字速度 → 字/秒（瞬显单独判）。
3. 引擎（无头，`SDL_PushEvent` 真事件，照 `EngineTests` 的 `pushKey`）：音量 / 全屏 / 缩放 / 垂直同步 / 特效档位的存取；Alt+Enter 切全屏且**不出 Confirm**（含按住 300ms 后也不自动重复出 Confirm）；二期：自定义键驱动 `keyDown/keyPressed`、固定键删不掉、抓键期间不出逻辑键、改一格的五条规则与两条拒绝。
4. 接线（`GameWiringTests` 那一类，真 `Application`）：标题画面第 3 项确认 → 栈顶是 `SettingsScene`；主菜单「设置」确认 → 同上；在设置面板里按 → 调音乐音量 → `app.settings().bgmVolume` 与 `engine.bgmVolume()` 都变（C++ 字段照工程习惯用驼峰，文件里的键用第 1 节的下划线 id）；关面板 → 写到 **enable 过的临时路径**、读回相等；没 enable 时关面板不产生任何文件；文字速度改了 → DialogueScene 实际用的速率变（DialogueScene 在无头下直接全显，所以把「用哪个速率」抽成它 update 里调的那个函数来测，不另写一份）；震屏关 → BattleView 的震幅为 0；二期：改键后 `app.text("ui.menu.keys")` 跟着变、默认键位下四处提示与旧字面逐字节相同。
5. `systemSceneActive` 把两个面板算进去。
6. `UiStyleTests`：两个面板与两处入口的字过禁词表。
7. 旧测试一条都不许放宽；菜单项数、下标这类断言改成新的真值，并在提交说明里列出改了哪几条、为什么。

## 8. 验收（协调者亲自做，不信施工方的自报）

- 独占构建槽完整构建：全部测试绿（交接时 1322 条 + 新增），四道门禁（`VALIDATE_OK` / `SELFTEST_OK` / `MAPGEN_IN_SYNC` / `ARTGEN_IN_SYNC`）全过，零编译警告。
- 跑完测试后仓库里**没有** `saves/settings.json`。
- 截图：两个面板；主菜单「设置」那一页；`--settings` 指一份 `effects: lite` 拍 `fxdemo` 与一张世界图，对比完整档；全屏 + 铺满下拍一张世界图与主菜单（模糊底图不错位）。
- 默认设置下截几张旧截图口（标题、世界、战斗），与改造前逐像素相同。

## 9. 施工纪律

- 构建槽 `settings`（`& "H:\Work\Kys\fanren\build_logged.bat" settings`，**PowerShell 调**；日志 `build-settings.log` 读字节按 gb18030 解码，读前核 mtime）。
- 不派子代理；不整份重读大文件；最小改动足迹，不顺手重构；汇报精炼（改了哪些文件、测试数、门禁、截图路径、已知问题）。
- 不写仓库里的 `saves/settings.json`（测试写临时目录）。
- 新文件放进已有目录，CMake 按 glob 收，不用改 CMakeLists。

---

## 实现记录

（施工方在这里补：与契约不同之处及理由、测试清单、截图路径。）

### 一期（2026-09-27，构建槽 `settings`）

**落地文件**
- 新增：`src/io/SettingsFile.{h,cpp}`、`src/game/SettingsScene.{h,cpp}`、`tests/SettingsFileTests.cpp`、`tests/SettingsWiringTests.cpp`。
- 改动：`src/engine/Engine.{h,cpp}`、`src/engine/PostFx.{h,cpp}`、`src/game/Application.{h,cpp}`、`src/game/TitleScene.{h,cpp}`、
  `src/game/MenuScene.{h,cpp}`、`src/game/DialogueScene.{h,cpp}`、`src/game/BattleView.{h,cpp}`、`src/main.cpp`、
  `data/text/ui.json`（+45 条：`ui.title.settings`、`ui.menu.settings`、`ui.menu.settings.hint`、`ui.settings.*`）、
  `tests/EngineTests.cpp`、`tests/PostFxTests.cpp`、`tests/UiStyleTests.cpp`。存档格式（v8）没动。

**契约没写死、施工时定下的（二期照这个接）**
1. io 层的名字：`io::Settings`（字段 `bgmVolume / sfxVolume / fullscreen / scale / vsync / effects / screenShake / textSpeed / keys`）；
   枚举 `io::DisplayScale{Integer, Fit}`、`io::EffectsLevel{Full, Lite}`、`io::TextSpeed{Slow, Normal, Fast, Instant}`（次序即由慢到快）；
   `io::KeySlots = std::array<int, 2>`；读写 `io::parseSettings / loadSettings / saveSettings`，读的结果
   `io::SettingsRead{settings, found, error, warnings}`；音量曲线 `io::volumeGain(level)`。
2. **动作 id 词表 `io::kKeyActionIds` 一期放在 io**（`keys` 的形状检查要用，而 io 不依赖 engine）。二期引擎的
   `Engine::keyId / keyFromId` 必须用同一组词，并加一条测试把两边钉在一起（共享词表只许有一种写法）。
3. 读盘比第 2 节再宽一点：文件开头的 UTF-8 BOM 照认（这份文件给人手改，记事本可能存出 BOM；生成器写的数据文件仍拒 BOM）；
   `version` 不是 1 只警告、认得的字段照读。夹紧、不是整数、类型不对也各留一条 warning（第 2 节只点名了枚举）。
4. 软警告（夹紧、认不出的词、键位整张回默认）由 `Application` 自己逐条打到标准错误（前缀 `[settings]`）；
   硬失败（坏 JSON、顶层不是对象、读不了）照第 3 节返回给调用方，`main` 打一行。成功时 `value` = 文件在不在。
   `saveSettings()` 失败时返回一句放得进面板说明行的短话（「写不进 settings.json」），带整条路径的完整原因打到标准错误（审查 A3）。
5. 引擎多了两个读回 `integerScale()`、`vsync()`（第 7 节第 3 条要测「存取」）。`setFullscreen` 之后调 `SDL_SyncWindow`：
   异步的窗口系统上等它落定，免得 Application 每帧对账时把刚设的全屏当成「被 Alt+Enter 切回去了」（Windows 上是空操作）。
   `setIntegerScale` 设的是后缓冲那一份逻辑呈现（SDL3 每个渲染目标各一份）。**`Engine::init` 按已存的全屏、缩放、垂直同步、音量开窗**
   （口径统一：init 之前设过的一律生效；缺省与改造前相同），`Application::init` 末尾再推一次 settings（审查 A8）。
   音量增益夹在 [0, 1]（NaN 当 0）。音效轨道池建不齐照样跑（少几条就是同时响的少几个），init 打一行警告。
   关垂直同步的补睡用 `SDL_DelayNS`（`SDL_DelayPrecise` 按头文件原文最后一截是忙等，与封顶的本意相反；审查 A4）。
6. Alt 的判定以事件自带的修饰键（`SDL_KMOD_ALT`）为准（审查 A6：一期另记过左右 Alt 的按下状态，那是为测试加的生产分支，删了；
   测试塞真事件时带上 `SDL_KMOD_LALT`）。Alt 在窗口拿到焦点之前就按下了也认，右 Alt 也认。
7. 面板交互里第 5.2 节没写死的：确认键只在「恢复默认」「按键设置」上起作用，值那几行只吃左右键；**一律有方向、到头就停**：
   音量、文字速度一格一档不回绕；两档的 ← 取第 1 节表里写在前面的那一档、→ 取后面那一档（审查 A1 HIGH：一期是「左右都翻一下」，
   而左右键带自动重复，按住会让全屏与窗口每秒闪切十几次，有光敏风险）；到头那一边的括号变暗（问的就是 `adjusted`，不另写判断）。
   调值响 `ui_cursor`，调音效音量只响 `ui_confirm`（新增益下试听）；关面板 `ui_close`，写盘失败 `ui_error`。
   「有改动」= 与**打开那一刻**的设置不同（开关一下面板不会凭空多出文件）；写盘失败后又改了东西，下次关面板重新试着写；
   写盘失败时说明行先写「再按一次…」，再写短原因（说明行只有两行，放不下的是原因的尾巴，不是该怎么办；审查 A3）；
   「已恢复默认」光标一动就撤，写盘失败的原因留到又改了什么或关掉面板。
7a. 审查 A2（MEDIUM）：面板里改了设置、没关面板就点窗口 X / Alt+F4，改动从前会丢。现在 Application 记一份「已落盘的设置」
   （enable 时生效的那一份、之后每次写盘成功写下的那一份），`shutdown` 时 enable 过且与它不同就补写；坏文件而又没人改过时两者相等，不覆盖。
7b. 审查 A5：Alt+Enter 之后那次写盘失败，设置面板在栈顶就写在它的说明行上（`SettingsScene::showSaveFailure`）；**面板不在时只进标准错误**
   ——这是与第 2 节「写失败要说出来（面板上那一行）」的偏差：行走、战斗画面上没有一块现成的地方说这句话，为它另开一个提示不值当；
   这一局照样按全屏跑，退出时（7a）再试着写一次。
7c. 审查 A9：`--screenshot` 显式给了 `--settings` 时**只读不写**（不 enable，只读来 `setSettings`）：全屏被窗口系统驳回时，每帧对账会把
   `fullscreen` 改回 false，enable 了的话就写回截图夹具里去了。
8. 标题画面：`TitleScene::kSettings = 2 / kQuit = 3 / kItemCount = 4`；固有字 `TitleScene::menuStrings(data)`（菜单四项、置灰理由、读档失败前缀、
   按键提示）。**书名「凡人修仙传」不进禁词扫描**：那是书名，不是界面用词，本来就带「修仙」。
9. 主菜单：`Page::Settings` 插在 `Save` 与 `Back` 之间，左栏命令块高 252 → 282（七行）；右栏「设置」页 = 一句说明 + 八项摘要，
   行名与值都问 `SettingsScene::rowLabel / valueText`，不另写一份。
10. 文字速度：「用哪个速率」是 `DialogueScene::revealRate(app)`（update 与 onEnter 调的就是它），纯映射 `DialogueScene::graphemesPerSecond(TextSpeed)`；
    瞬显返回 `std::nullopt`，onEnter 当场全显。标准档乘的仍是原来那个 60.0，默认下逐字节奏与改造前逐帧相同。
11. 震屏：`BattleView` 开战时读一次 `settings().screenShake`（仗打到一半改不了设置），`startShake` 一处拦；加了读回 `shakeAmplitude()`。
12. 截图口：`--scene settings`、`--scene menu:settings` 落地；`settings:keys` 二期落地（见下）。

**验证时发现的（不在本期改动范围）**
- 协调者 scratchpad 里的 `pixdiff.py` 对 RGBA 图调 `ImageChops.difference(...).getbbox()`：Pillow 10 起 `getbbox()` 对带 alpha 的图缺省
  `alpha_only=True`，两张不透明截图的差图 alpha 处处为 0，**RGB 再不同也报 IDENTICAL**（本机 Pillow 12.1.1 实测：标题、主菜单明明变了，
  它两张都报 IDENTICAL）。按 RGB 比的版本放在同目录 `pixdiff_rgb.py`（用法相同）。
- 基准的 `menu` 截图本身不确定：协调者先后拍的 `base/`、`base2/` 两份在 (614,262)–(689,303) 有 77 个像素不同（主菜单模糊底图里有东西随墙钟动），
  与本期无关。
- 本机是 4K 屏，全屏铺满恰好 3 倍，实拍试不出非整数倍；非整数倍另用软件渲染器测了一条（窗口 1500×1000、1.171875 倍，见下）。
  顺带看到：SDL 3.4 软件渲染器上 `SDL_RenderReadPixels(nullptr)` 在逻辑呈现下读回的就是内容区（不含黑边），`readBackbufferContent` 的裁剪那时是空操作。

**测试**（1322 → 1352，旧测试一条没改：没有哪条断言过菜单项数或下标）
- `tests/SettingsFileTests.cpp`：`SettingsDefaults.AreTheBehaviourBeforeTheSettingsExisted`、`VolumeCurve.ZeroIsSilenceTenIsTheOldLoudnessAndItOnlyGoesUp`、
  `TextSpeedRate.SlowNormalFastAreThirtySixtyAHundredTwentyAndInstantHasNoRate`、`SettingsFile.` × 8（缺文件、全字段往返、写全部字段与词、坏 JSON、
  越界夹紧、认不出的词与错类型、`keys` 八种坏形状整张回默认、写失败如实返回）。
- `tests/SettingsWiringTests.cpp`（真 Application + SDL 真事件，临时资产根）：`SettingsWiring.` × 11——标题画面第 3 项确认、主菜单「设置」确认 → 栈顶是面板；
  面板里 ←→ 调音乐音量 → `settings().bgmVolume` 与 `engine().bgmVolume()` 同变、关面板写到 enable 过的路径且读回相等；没 enable 关面板不产生文件；
  没改动不写 + 写盘失败留在面板上说原因、再按一次才关；按键设置行（二期改成「打开改键面板」）+ 恢复默认；第 1–6 行经面板到引擎；
  Alt+Enter → 设置跟着窗口走并写盘；文字速度 → `DialogueScene::revealRate`；震屏关 → `BattleView::shakeAmplitude()` 为 0（配对：开着不为 0）；
  开着面板记 `playSecondsSystem`。
- `tests/EngineTests.cpp`：`HeadlessEngine.SettingsDefaultToTheBehaviourBeforeTheyExisted`、`HeadlessEngine.SettingsAreStoredAndReadBackWithoutAWindow`、
  `HeadlessEngine.AltEnterTogglesFullscreenAndIsNeverAConfirm`（含按住 300ms 不出 Confirm、小键盘 Enter、配对的负向）、
  `HeadlessEngine.RightAltEnterCountsTooEvenWithoutSeeingTheAltGoDown`、`SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`。
- `tests/PostFxTests.cpp`：`SoftwarePostFx.LiteEffectsDropBloomAndDepthOfFieldButKeepTheLighting`、
  `SoftwarePostFx.TheMenuBackdropStaysPutWhenFitScalesByANonIntegerFactor`（铺满 + 非整数倍下主菜单模糊底图不错位）。
- `tests/UiStyleTests.cpp`：`SettingsWording.ThePanelAndBothEntrancesPassTheForbiddenWordScan`（面板 41 条固有字 + 标题菜单 + 主菜单，过禁词表；行名逐字对第 1 节）。

**截图**（`build-settings\shots\`，工作目录 = 仓库根，`--assets H:\Work\Kys\fanren`）
- 默认设置下的旧截图口：`default\{title,world,menu,battle,talk,fx}.png`。按 RGB 与基准比：world / battle / talk / fx 逐像素相同；
  title 只在 (613,523)–(669,599) 变（菜单第 3、4 行：「设置」「离开」）；menu 只在 (44,59)–(351,334) 变（左栏命令块长了一行），
  另有 (614,262)–(689,303) 那 77 个像素是上面说的基准噪声（与 `base2/` 比右半边逐像素相同）。
- 新画面：`settings.png`（`--scene settings`）、`menu_settings.png`（`--scene menu:settings`）。
- 精简档（`--settings` 指 `{"effects": "lite"}`）：`fx_lite.png` 对 `default\fx.png`、`world_lite.png` 对 `default\world.png`——
  灯笼没有辉光、上下两条不再移轴糊（嘉元城南城上下 100 行的边缘能量约翻倍，中段不变），夜里的光照照旧。
- 全屏 + 铺满（`{"fullscreen": true, "scale": "fit"}`，3840×2160）：`menu_fullfit.png`、`world_fullfit.png`、`battle_intro_fullfit.png`
  （对照窗口版 `battle_intro.png`）。缩回 1280×720 与窗口版比，最佳对齐偏移都是 (0, 0)：模糊底图与开战碎屏都不错位。
  两份设置文件的副本放在 `shots\lite.json`、`shots\fullfit.json`。

### 一期整改（审查 1 HIGH、1 MEDIUM、若干 LOW；2026-09-27）

逐条的做法写进了上面第 4–7c 条。新增 / 改动的用例（1352 → 1358）：
- A1 `SettingsWiring.HoldingRightOnATwoStateRowFlipsItOnceNotOnEveryRepeat`（真事件按住 → 400ms，先验「确实自动重复了」，全屏只切一次；← 同样）。
- A2 `SettingsWiring.ChangesSurviveClosingTheWindowWithThePanelStillOpen`、`SettingsWiring.ShuttingDownWithoutChangesLeavesABrokenFileAlone`。
- A3 `SettingsWiring.AnUnchangedPanelDoesNotWriteAndAFailedWriteKeepsThePanelOpen` 追加断言：说明行以「再按一次…」开头、点名 `blocked_settings.json`、
  不含整条路径。
- A5 `SettingsWiring.AnAltEnterWriteFailureShowsOnTheOpenPanel`。
- A6 `HeadlessEngine.AltEnterTogglesFullscreenAndIsNeverAConfirm` 的真事件带上 `SDL_KMOD_LALT`；`HeadlessEngine.AltEnterIsAlsoReadFromTheEventsOwnModifiers`
  改名 `HeadlessEngine.RightAltEnterCountsTooEvenWithoutSeeingTheAltGoDown`（自记 Alt 那一支删了，「also」不成立了）。
- A10 `SettingsWiring.AltEnterFlipsFullscreenAndTheSettingsFollowTheWindow` 不再拿「栈顶仍是 World」当没出 Confirm 的证据，改成直接断言
  `keyPressed(Confirm)`、`keyDown(Confirm)` 为假；`SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond` 加上界（十帧 < 120ms）；
  `SettingsFile.AByteOrderMarkIsAcceptedBecausePeopleEditThisFileByHand`、`SettingsFile.AnotherVersionIsOnlyAWarningAndTheKnownFieldsAreStillRead`。

### 二期：改键（2026-09-27）

**落地文件**
- 新增：`src/game/KeyConfigScene.{h,cpp}`、`tests/KeyBindingTests.cpp`。
- 改动：`src/engine/Engine.{h,cpp}`（`kBindings` 拆成固定键 `kFixedKeys` + 默认自定义 `kDefaultKeys`，`keyHeld` / `pollEvents` 查新表）、
  `src/game/Application.{h,cpp}`、`src/game/SettingsScene.{h,cpp}`、`src/game/DialogueScene.cpp`、`src/main.cpp`、
  `data/text/ui.json`（`ui.menu.keys` 改占位、新增 `ui.dialogue.keys` 与 `ui.keys.*` 32 条、删 `ui.settings.keys.later`、
  `ui.settings.desc.keys` 与 `ui.menu.settings.hint` 补上按键）、
  `data/text/ui_battle.json`（两条提示改占位）、`tests/SettingsWiringTests.cpp`、`tests/UiStyleTests.cpp`；
  文档 `docs/interfaces-octo-ui.md`（第 3、6、7、9 节）、`saves/README.md`（按键一行 + 「系统设置」一节）。

**契约没写死、施工时定下的**
1. 引擎接口照第 4.4 节，另加：`Engine::KeySlots / KeyTable`（按 Key 次序的 `std::array`）、`defaultKeyTable()`、`fixedKeyOwner()`、`reservedKey()`、
   `validateKeyTable()`（`setCustomKeys` 用的就是它，纯函数）、`cancelKeyCapture()`（面板中途被关掉时把引擎放回常态，不然之后一个逻辑键都不出）、
   `capturingKey()`、抓键用的三个扫描码常量 `kEscapeCode / kBackspaceCode / kDeleteCode`（game 层不碰 SDL 头；Engine.cpp 里 `static_assert` 对账）。
   扫描码上限 `engine::kScanCodeLimit`：Engine.cpp 里 `static_assert` 等于 `SDL_SCANCODE_COUNT`，Application.cpp 里等于 `io::kScancodeLimit`。
2. 改一格的纯函数是 `Engine::assignKey / clearKey`，结果 `KeyChangeResult{table, change, other}`；`change` 按实际发生的事分：
   Assigned / Unchanged / SwappedSlots（本动作两格互换）/ MovedSlot（这一格原来空着：从另一格挪过来）/ TradedWith（与动作 B 互换）/
   TookFrom（这一格原来空着：从 B 挪过来，B 那一格空出来）/ Cleared / AlreadyEmpty（清一个本来就空的格）/ RejectedFixed / RejectedReserved /
   RejectedLastKey / RejectedInvalid；`other` 是另一方、固定键的主人或会被掏空的那一方（终审 L5：说明行不再把「挪过来」说成「互换」、
   把「本来就空」说成「就是这个键」）。
3. 抓键期间：被抓的那一下不记按下（记了的话它刚被配给哪个动作，那个动作就成了「按住」）；`beginKeyCapture` 当场作废本帧的「刚按下」并把重复计时清零，
   抓键期间不出任何逻辑键、也不自动重复；Alt+Enter 照样切全屏、不被抓；纯修饰的保留键（左右 Alt、左右 Win）照常记按下、**不被抓、不结束抓键**
   ——它们多半是 Alt+Enter、Alt+Tab、Win 组合键的前一半（终审 M2）。菜单键、Backspace、Delete 照抓。
   扫描码 0（SDL_SCANCODE_UNKNOWN：没映射的媒体键、Fn 组合、厂商宏键）任何时候都不算一个键：在 `pollEvents` 最前面与越界一起挡掉，
   `pressScancode` 也只认有效扫描码（终审 H1：从前它与默认表里八个空格位一起命中，同一帧八个逻辑键全按下，世界层先判存盘、悄悄覆盖 quick.sav）。
4. `io::Settings::keys` 只存**与默认不同**的动作（`game::keySettingsOf`）；往引擎推时默认表打底、列出的动作盖上去（`game::keyTableOf`）。
   于是「全部默认」就是空表，与 `io::Settings{}` 相等——改了又改回去不会凭空多出文件。整表校验不过（含**重复**）→ 键位整张回默认 + 标准错误一行
   （`Application::applySettings`，`setSettings` 与 `enableSettingsFile` 都走它）；`enableSettingsFile` 之后「已落盘」随之是清空后的那份，退出时不覆盖那个文件。
   引擎校验的失败原因用动作 id 与键的显示名（给日志看，不进面板）。
5. 占位符展开的单点是 `Application::text()`（原来的内联一行挪进 .cpp）：`{key.<id>}` → 那个动作第一个自定义键（没有就第一个固定键）的显示名；
   认不出的 id 原样留着。`MenuScene::menuStrings` 等禁词扫描读的是 `lookupText` 原文（带占位），不影响扫描。
6. 改键面板：←→ 直接选第一 / 第二格（不回绕），↑↓ 在 11 行里回绕；确认 → 抓键；Esc / 菜单键关面板（与设置面板同一个口径——所以把「菜单」改成 M 之后，
   在改键面板里按 M 就是关面板，用例钉着）；抓到 Esc = 作罢、Backspace / Delete = 清空这一格；结果写说明行（成功绿、拒绝红，拒绝响 `ui_error`）。
   设置面板开着改键面板时一笔不画（照主菜单「记事」的做法），改键面板自己压纱。写盘随设置面板关闭时一起（那边比打开时的整份设置，含键位）；
   面板开着就关窗口，也由上面 7a 的退出补写兜住。
7. 设置面板「按键设置」一行接通：右栏「›」，确认压改键面板；`SettingsScene(bool openKeyConfig)` 给截图口 `settings:keys` 用（第一次 update 里压，
   与主菜单「直接翻开记事」同一个理由）。设置面板让开的判据是「改键面板开过 **且顶上不是我**」：改键面板收起的那一帧本面板的 update 还没跑，
   只看旗子的话那一帧两块面板都不画、底下的画面亮一下。（主菜单的「记事」「设置」沿用 `journalOpen_` 的旧做法，收起那一帧只剩模糊底图，
   是已有的一帧空档，没动。）
8. 改键面板在等按键、引擎那边的抓键却被收回了（`cancelKeyCapture`）：面板下一帧回到常态，不卡在「等按键」上。
9. 终审 M1：设置面板记的是「写盘失败那一刻的设置」（`failedAt_`），不是一个布尔：关面板时有改动、且与失败那一刻不同才重新写。
   失败后转去改键再回来按 Esc，会重新写、又失败就留下说原因；原样不动再按一次才关。进改键面板不再清掉写盘失败的原因（回来还看得见）。
10. 终审 L1：Alt+Enter 那次写盘失败，在整个场景栈里找设置面板写给它（不限栈顶）——改键面板压在上面时也写，收起就看得见。
11. 终审 L3（文档）：「只存与默认不同的动作」的代价写进了契约第 6 节：将来改默认键位撞上玩家已存的自定义键，那位玩家的键位整张回默认。
12. 终审 L4：契约第 6 节「全部文案都经 `text()`」不成立，改成实情；占位只许出现在四条提示上，数据级用例钉白名单。
13. 复查 N1（M2 引入）：抓键中按着 Alt / Win 按下去的键（Alt+F4、Alt+空格、Win+D……）一律不抓、不结束抓键——组合键本来也配不了；
    从前 Alt 放过之后，Alt+F4 的 F4 被抓走、配进当前格，退出时还补写进盘。纯修饰键放过那条保留。
14. 复查 N2：写盘失败之后在本面板调值或恢复默认，`failedAt_` 直接清掉（说明也撤了）：调走再调回、恰好回到失败那一刻的值，关面板也要再写再报，
    不能屏上什么都没有就关了；改键面板那条路仍靠比值。

**测试**（1358 → 1382；终审整改 1382 → 1386；复查整改 1386 → 1389：`HeadlessKeys.AltOrWinCombosAreNeverTakenAsTheNewKey`（N1）、
`SettingsWiring.AfterAFailedWriteAdjustingAwayAndBackStillRetriesOnClose`（N2）、`KeyResultText.EachOutcomeSaysWhatHappenedWordForWord`
（N3：结果类别 → 说明行那句话，期望串写死在测试里，不从 ui.json 读））
- `tests/KeyBindingTests.cpp`（18 条）：`KeyVocabulary.TheEngineAndTheSettingsFileUseTheSameWords`（`Engine::keyId` 与 `io::kKeyActionIds` 钉在一起）、
  `KeyDefaults.FixedPlusDefaultCustomKeysAreExactlyTheOldNineteenBindings`（旧 19 条原样写成字面量来比）、`KeyLabels.FollowTheContractsTable`、
  `KeyTableValidation.EachRuleOfTheWholeTableCheckRefusesOnItsOwn`、`KeyRules.` × 9（五条规则、两条拒绝、清空、胡乱输入、四千步随机改动后不变式仍成立）、
  `HeadlessKeys.` × 5（自定义键驱动 keyDown / keyPressed、固定键删不掉也挪不走、整表换不上就一格不动、抓键期间不出逻辑键且下一下被取走、
  抓键期间 Alt+Enter 照切 + cancelKeyCapture）。
- `tests/SettingsWiringTests.cpp`：`SettingsWiring.DefaultKeyPromptsAreByteForByteTheOldText`（四处与旧字面逐字节相同）、
  `SettingsWiring.RebindingThroughTheKeyPanelMovesTheKeyThePromptsAndTheFile`（设置面板 → 改键面板 →「菜单」改成 M：引擎、三处提示、写盘都跟着变）、
  `SettingsWiring.InTheKeyPanelEscCancelsBackspaceClearsAndTheLastKeyStays`、`SettingsWiring.AnInvalidKeyTableInTheFileResetsEveryKeyAndLeavesTheFileAlone`、
  `SettingsWiring.TimeSpentInTheKeyPanelIsSystemTimeToo`、`SettingsWiring.AKeyPanelWhoseCaptureWasWithdrawnStopsWaiting`；一期的「按键设置行被跳过」改成新的真值：
  `SettingsWiring.TheKeysRowOpensTheKeyPanelAndRestoreBringsEveryDefaultBack`（「恢复默认」连键位一起回去也在这条里）。
- `tests/UiStyleTests.cpp`：`SettingsWording.ThePanelAndBothEntrancesPassTheForbiddenWordScan` 把改键面板的 35 条固有字也扫进去（设置面板少了「稍后开放」，41 → 40）。
- `EngineTests.MapsScancodesToLogicalKeys` 一字没改，照样绿。
- 终审整改新增：`HeadlessKeys.AnUnmappedKeyWithScancodeZeroIsNoKeyAtAll`（H1：扫描码 0 的真事件，十个 Key 都不 pressed、不 down，按住 300ms 也一样；
  抓键中按到它抓键继续）、`KeyPrompts.PlaceholdersOnlyLiveInTheFourPromptsThatGoThroughText`（L4：扫 `data/text/**`，另按原始字节数一遍对账）、
  `SettingsWiring.AfterAFailedWriteAKeyChangeMakesTheNextCloseTryAgain`（M1）、`SettingsWiring.AnAltEnterWriteFailureUnderTheKeyPanelLandsOnTheSettingsPanel`（L1）。
  改动：`HeadlessKeys.AltEnterStillTogglesFullscreenWhileCapturingAndIsNotTaken` 先塞左 Alt 的按下再塞 Enter（M2；从前只塞了带修饰键位的 Enter，
  给的是假保证），并补右 Alt、左右 Win 不结束抓键、菜单键照抓；`KeyRules.NoSequenceOfChangesBreaksTheInvariants` 的判据不再借被测的
  `validateKeyTable`，改在测试里用字面量集合独立判（L2，另配三条阳性对照）；`KeyRules.` 里互换 / 挪过来 / 本来就空按新的结果类别断言（L5）。

**截图**（`build-settings\shots\`）
- 默认键位下的旧截图口与一期产物 `scratchpad\v1\` 逐像素比（协调者新的 `pixdiff.py`，带阳性自检）：title / world / menu / battle / talk / fx 全部 IDENTICAL。
  （世界层在韩三叔一带约 (614,262)–(696,331) 有一小块随墙钟变的像素，就是一期记下的那块基准噪声；改键面板两张图之间也碰得到它，比对时扣掉。）
- `settings_keys.png`（`--scene settings:keys`，默认键位）；`settings_keys_custom.png`、`talk_keys.png`、`menu_keys.png`
  （`--settings` 指 `shots\keys.json`：菜单 → M、快进 → 右Shift；对照 `settings_keys.png`、`default\talk.png`、`default\menu.png`）：
  改键面板上是新键名，对白框右上那块牌写「M 回看　长按 右Shift 快进」，主菜单右下写「M 关闭　Esc 返回」。
  战斗提示没拍：`battle:<编成>` 各个时刻在截图口的默认状态下都停在战败卡上（与基准那张相同，是截图口的旧状况），看不到提示那一行；
  它的展开由 `SettingsWiring.RebindingThroughTheKeyPanelMovesTheKeyThePromptsAndTheFile` 断言（「Esc / M 返回」）。
- `settings.png`：默认设置下五个两档行左括号是暗金（到头）、右括号是金色；文字速度两边都亮。
