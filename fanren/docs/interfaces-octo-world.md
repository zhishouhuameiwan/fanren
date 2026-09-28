# 八方旅人化 · 世界画面接口契约（行走画面 HD-2D）

> 2026-09-26 · 世界画面路（W，槽 `worldfx`）交付 · 施工图 `docs/octopath-overhaul.md` 第 1.1–1.3 节与第 3 节的落地版。
> 读者：P2（路径行动入口、野外遭遇）、U（主菜单、标题流程）、A1（地图烘焙）、A2（精灵）、BV（战斗画面，可借用本文的纯函数）。
> 头文件：`src/game/WorldView.h`（渲染器与纯函数）、`WorldHud.h`（HUD）、`SpriteAtlas.h`（index.json 里挑外观、算帧）、
> `MapVisual.h`（meta.json → 画面参数）、`WorldScene.h`（规则与钩子）。
> 两份描述文件的**读取**在 io 层：`src/io/VisualLoader.h`（`loadSpriteIndex` / `loadMapMeta`，nlohmann），
> 读出来的纯结构体在 `src/core/model/Visual.h`（`core::SpriteIndex`、`core::MapMeta`）。
> 原先的 `game::VisualJson` 手写读取器已退役（C1 收口：JSON 只在 io 层读）。

---

## 0. 分工与不变量

| 谁 | 管什么 |
| --- | --- |
| `WorldScene` | **规则与「画什么」**：谁在场、哪道门开着、哪道门通往目标、哪一格按得动、目标在哪。每帧装进 `WorldFrame` |
| `WorldView` | **怎么画**：烘焙图 / 旧图元、精灵与走路帧、镜头、插值、光照、粒子、后处理、HUD、换图淡入 |
| `WorldHud` | 后处理之后那一层：地名横幅、目标框、目标菱形 / 边缘箭头、名牌、存盘提示 |

**格子语义不变**：`tryStep` 一调 `GameState::position` 立刻变，`interact` 照旧。插值滑动（0.12 秒）、走路帧、
镜头缓动、换图淡入（0.3 秒）全是纯视觉，从不回头延迟状态（`WorldViewHeadless.TheLogicMovesAtOnceWhileThePictureSlides` 钉着）。
无头模式：`WorldView::advance` 只推几个浮点数，不碰文件与纹理；`render` 由主循环在无头时根本不调。

## 1. 像素尺度与坐标

- `kArtTilePx = 16`、`kArtScale = 3`、`kWorldTilePx = 48`（屏幕像素 / 格）。逻辑格仍是 `engine::kTileSize = 32`，
  48 只在渲染层出现。
- 世界像素 = 格 × 48；屏幕像素 = 世界像素 − 镜头（镜头取整到屏幕像素，三层图严丝合缝）。
- 人物精灵 16×24 ×3 = 48×72：帧内 `(frame_w/2, foot_y+1)` 对准所站那一格的底边中点。
- 物件：`anchor`（帧内像素）×3 对准对象矩形的底边中点（平铺标记对准每一格的底边中点）。

## 2. 一帧的渲染次序

```
PostFx::beginScene
  ① 下层  烘焙 below.png（缺图 → MapArt 旧图元 ground/overlay/building，按 48px 画）
  ② 地面  传送箭头 + 门光（加色）、交互闪光（只画 ready 的 interact 触发）、目标光圈、水面波光
  ③ 柔影  fx/shadow.png（缺图 → 几何椭圆）
  ④ 排序  设施精灵、NPC、主角：按脚底 y，再按 x，再按登记次序（drawsBefore）
  ⑤ 上层  烘焙 above.png，主角周围 ±3 格窗口逐顶点算透明度（canopyAlpha：1.25 格内 0.3，2.75 格外 1）
           （缺图 → front 层旧图元，同一条透明度曲线逐格算）
  ⑥ 粒子  落下的（花瓣/叶/雨/雪）屏幕坐标；浮着的（尘埃/萤火/雾/火星）与定点发射器世界坐标
  ⑦ 辉光  灯的光晕（fx/glow.png 按灯色）、设施发光帧（objects_emissive.png）、开着的门的箭头、闪光、目标光圈、萤火
  ⑧ 光源  meta.lights（带闪烁）+ 主角随行微光（夜 / 黄昏 / 室内）
PostFx::endScene(postFxFor(meta, 主角的屏幕高度))
UI：名牌（theme.titleStyle 描边字，无底条）、路径行动气泡、目标金菱形或屏幕边缘柔光箭头（带「N 格」）、
    地名横幅、目标框、存盘提示、换图淡入的黑幕（盖住以上全部）
```

性能：只画视口里的格与对象；烘焙图一张图载一次、离开时 `destroyTexture`；人物表、物件图集、柔影、光晕各载一次；
横幅的字在地名变时画进一张离屏画布一次，之后每帧只调 `tint.a`（不逐帧新建文字纹理）；几何与排序缓冲每帧复用。

## 3. 镜头、插值、时钟

| 纯函数 | 语义 |
| --- | --- |
| `cameraAxis(focus, mapExtent, viewExtent)` | 地图比视口小 → 居中（负数）；否则 focus 居中并夹在 [0, 地图−视口] |
| `cameraTarget(w, h, focusCell)` | 镜头左上角（48px 格）；focus 取主角画面位置 + 0.5 |
| `approach(cur, target, dt, rate)` | 指数趋近，rate = 10/秒，与帧率无关、不越过 |
| `StepSlide` | `moveTo` 从画面当前位置匀速滑 0.12 秒；`snap` 瞬移 |

- 相邻一格 → 滑；换图（地图 id 变）→ 人与镜头就位 + 从黑 0.3 秒淡入 + 横幅在黑幕淡一半后淡入；
  同图不相邻的跳（脚本挪人）→ 人与镜头一起跳，不滑不淡。
- **开场**（`WorldScene::onEnter` → `WorldView::reset`）：不淡入、横幅直接在——外层（标题 / 章节卡）的转场自己会淡。
- 时钟：`WorldScene::update` 每帧 `advance(dt)`；对话框等压在上面时世界层轮不到 update，`render` 用
  `Engine::deltaSeconds()` 补这一步（树叶照落、那一步照样滑完，世界不冻住）。
- 走路帧：`walk_cycle` × `walk_fps`（index.json），起步直接从「迈甲」开始；停下 0.06 秒后回站立帧
  （按住方向键时两步之间不到一帧的空当不闪回站姿）。`kStepCooldownSeconds == kStepSlideSeconds` 由 `static_assert` 钉着。

## 4. 精灵选择（`assets/art/sprites/index.json`）

- 角色 → 外观：`role_variants`（第一条落进 `[min_chapter, max_chapter]` 的）→ `roles` → 没有（旧画法 + 一行 WARNING）。
- **判据 = 「第 n 章演完没有」**：`Application::chapterTable().chapterDone(state, n)`（`data/chapters.json` 的 done_flag），
  `max_chapter: M` 在第 M 章没演完时成立、`min_chapter: m` 要第 m−1 章已演完——主菜单小像走的也是
  这一条（`lookForRole` / `sheetForRole`，同一张表只读一份）。不用 `GameState::chapter`：它从未被任何脚本推进。
  韩立、张铁第 1–2 章少年版，`ch02.done` 置位后换成年版
  （`RealSpriteIndex.HanliAndZhangTieAreBoysUntilChapterTwoIsDone`、`WorldViewHeadless.HanliGrowsUpWhenChapterTwoIsDoneThroughTheRealChapterTable`）。
- 主角按角色 id `hanli` 查。NPC 按对象的 `role_id` 与 `facing`（站立帧）。
- 设施 kind → `facilities[kind]` → 物件帧（带 fps 的轮播；丹炉 / 器炉 / 阵盘 / 香炉的发光帧另进辉光通道）。
- 传送点：`portal.arrow_<方向>` 每格一枚（加色；开着冷白、通往目标金色、关着暗红几乎只剩影），
  方向 = `portalArrowFacing`：那一侧挡路或出了地图、对面是走得通的来路；贴边优先；都不合就朝下。
  箭头朝上且上面不是地图边的（墙上的门）再加 `portal.door` 门光。
- 交互闪光 `mark.sparkle`：每 2.2 秒闪一轮（四帧），各对象错开相位。
- 物件图集缺失 → 设施 / 传送点 / 闪光退回改造前的底板 + 字 / 描边框画法（`MapArt::facilityColor/facilityGlyph/drawGlyph`）。

## 5. 地图视觉（`assets/art/maps/<id>/{below.png, above.png, meta.json}`）

- 烘焙图可用 ⇔ 载得进来且尺寸恰为 `地图格数 × 16`（`bakedLayerUsable`）；两层各自判，缺哪层哪层退回旧图元，打一行 WARNING。
- meta 字段见 `MapVisual.h` 文件头。缺字段按时辰缺省（与 `tools/artgen/maplights.py` 的 `TIME_DEFAULTS` 逐项相同），
  时辰缺了按主题推（`indoor_wood / indoor_stone / cave` → 室内，其余 → 昼）。
  **认不出的时辰、粒子种类、形状不对的数组一律报错**（整份退回缺省 + WARNING）——A1 若新增粒子种类，先在 io 层加
  （`io/VisualLoader.cpp` 的 `particleFromName` 与 `core::AmbientParticle`），再在 `MapVisual.cpp` 的 `particleKindOf` 接到引擎。
- 读（`io::loadMapMeta`）只留文件里写了的；按时辰补缺省、换成引擎的颜色与粒子种类在 `mapVisualFrom`。
  meta 里的 `backdrop`（这张图上开打用的战斗背景 `assets/art/battle/<backdrop>/`）世界画面不用，留给战斗画面取
  （`core::MapMeta::backdrop`）。
- meta 整个缺失：室外按昼、室内按室内，不点灯、不下粒子。
- 粒子 `rate` 是 0..1 的**浓淡**，不是每秒几颗：每秒生成 = 满浓度率 × rate × 发射区面积（屏幕数）
  （满浓度率 / 屏：尘埃 24、萤火 10、花瓣 2.4、落叶 2、雨 700、雪 12、火星 12、薄雾 1.6——落下来的活二三十秒，所以低一个量级）。
- 定点发射器：火星 30×10 像素一窄条、每秒 7；薄雾 144×72、每秒 0.3；萤火 144×96、每秒 0.7；
  树冠下的落叶 / 花瓣 96×120、每秒 0.35（从树冠落到地上）。整张图的火星按 4 格一条横带铺（火星从发射区底边往上冒）。
- 后处理：`ambient / dof / bloom / vignette` 取 meta；景深对焦在主角身上（屏幕高度比例夹在 0.3–0.7，带宽 0.30、过渡 0.22）；
  调色按时辰（昼微暖、暮橙、夜青、室内烛黄，偏移很小）。
- 水面波光：meta.water 的格子，每格两粒，只在正弦波顶上一闪，落在美术像素网格上。

## 6. 给 P2 的接口：路径行动气泡与 E 键

```cpp
// WorldScene.h
using PathBubbleProbe  = PathBubble (*)(Application& app, const core::MapObject& npc);
using PathActionOpener = bool (*)(Application& app, const core::MapObject& npc);
static void WorldScene::setPathActionHooks(PathBubbleProbe probe, PathActionOpener opener);
bool WorldScene::pathAction(Application& app);   // E 键走这里；公开给无头 bot
```

- `probe`：每帧对每个**在场**的 NPC 问一次，返回 `PathBubble::{None, Generic, Inquire, Purchase, Challenge}`，
  决定头顶浮不浮气泡、浮哪一个（`bubble.path / path_inquire / path_buy / path_duel`）。建议实现：
  `rules::pathActionsAt(app.data().pathActions, app.state(), map.id, npc.name)`，空 → None；只有一种 → 那一种；多种 → Generic。
- `opener`：面对在场 NPC 按 E / Q（`Key::Action`）时调；返回 true 表示受理。与 `interact` 同一道闸（脚本在跑不受理），
  找人同 `visibleNpcAt`（不在场的人不应声）。
- 两个钩子进程内全局一份，没接时为 nullptr：不浮气泡、E 键不响。接的地方建议在 `Application::init` 之后（协调者定）。
- 野外危险星级：`WorldFrame::dangerStars`（地名横幅右侧一排小金菱形）已留好，`WorldScene::describe` 眼下恒填 0。

## 7. 给 U 的接口：主菜单

- `WorldScene::update`：Tab（`Key::Menu`）或 Esc / X（`Key::Cancel`）→ `app.openMainMenu()`，在「脚本在跑就不受理」那道早退之后、
  存盘（F5）之后、确认键之前。**已落地**（U 的 `Application::openMainMenu` 已在树里）。
- HUD 颜色与字样全部取 `app.theme()` 的墨金 token（`ink / inkLow / inkDeep / gold / goldDim / goldBright / paper`、
  `titleStyle / bodyStyle`），`WorldHud` 里没有字面量颜色。

## 8. 截图口与已知限制

- `--map <id> --screenshot <png>`：截图口只 `tick` 三帧（约 0.033 秒）就抓帧——开场不淡入、横幅直接在，
  所以一上来的画面就是完整的；换图淡入、横幅淡入淡出、走路帧只能靠单测与真窗口看。
- 名牌、气泡、目标标记画在后处理之后：夜里不压暗、景深带里不糊（读得清优先于「在世界里」）。
- 世界坐标的全图粒子（尘埃、萤火、雾）铺满整张图，离屏那些也会提交绘制（上限 512 颗 / 层）；
  定点发射器按范围裁剪。
- 设施的站立物件由运行时按精灵画；A1 烘焙里的「地面部分」（地火口、存档石、蒲团）在下层，两者叠在同一处。
  若 A1 在烘焙里也画了立着的物件（告示板），会与精灵叠成两份——见交付报告，由 A1 / 协调者定谁让。
