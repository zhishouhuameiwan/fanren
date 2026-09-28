# 八方旅人化 · 引擎接口契约（渲染能力 / 后处理 / 粒子）

> 2026-09-25 · 引擎路（槽 `engfx`）交付 · 施工图 `docs/octopath-overhaul.md` 1.4 节的落地版。
> 读者：地图渲染（W）、战斗画面（BV）、界面（U）三路。**签名以本文与头文件为准。**
> 头文件：`src/engine/Engine.h`、`src/engine/PostFx.h`、`src/engine/Particles.h`。
> 用法范例：`src/game/FxDemoScene.cpp` 的 `render()`——一帧里该按什么次序调什么，那里按顺序写着。

---

## 0. 一眼看完

| 要做的事 | 用什么 |
| --- | --- |
| 贴像素美术（烘焙地图、人物帧） | `loadTexture(path)`（缺省 `ScaleMode::Pixel`）+ `drawTexture(id, RectF src, RectF dst, DrawOptions)` |
| 人物朝左 | `DrawOptions::flipX = true`（行走图只画朝右的一套） |
| 发光、光斑、刀光 | `DrawOptions::blend = BlendMode::Add` |
| 半透明、淡入淡出 | `DrawOptions::tint.a`（纹理）或 `fillRect(..., Color{.., a}, BlendMode::Alpha)` |
| 面板竖向渐变底、渐隐金条 | `drawGradient` / `drawGradientH` |
| 扇形、光束、椭圆柔影、碎屏三角 | `drawGeometry`（顶点色 + 可选纹理） |
| 名牌、伤害数字要压得住背景 | `drawText(..., TextStyle{.outline = true})` / `shadow` |
| 离屏画布（碎屏、合成） | `createRenderTarget` + `setRenderTarget` + `clear` |
| 运行时生成的图（径向渐变、暗角） | `createTexture(w, h, rgba, mode)` |
| 动画相位 | `elapsedSeconds()` |
| 景深 / 光照 / 辉光 / 暗角 / 调色 | `engine::PostFx`（第 3 节） |
| 尘埃、萤火、落叶、花瓣、雨、雪、火星、薄雾 | `engine::ParticleSystem`（第 4 节） |
| 路径行动键（E / Q） | `Engine::Key::Action` |

**旧接口一个签名都没动**（`drawTexture(id, Rect, Rect)`、`drawRect`、`drawText(5 参)`、`loadTexture(path)`……），
旧画面一个像素都不变（`--map ch01_hanjiacun` 改造前后逐像素比对过，见第 7 节）。

---

## 1. 新增的类型（`Engine.h`）

```cpp
struct RectF { float x, y, w, h; };                  // 新绘制接口一律浮点：插值滑动、亚像素漂移
enum class ScaleMode { Pixel, Linear };              // Pixel = SDL_SCALEMODE_PIXELART；Linear = 双线性
enum class BlendMode { None, Alpha, Add, Mod, Mul };

struct DrawOptions {
    Color tint{255, 255, 255, 255};   // 颜色 × tint.rgb，透明度 × tint.a
    BlendMode blend = BlendMode::Alpha;
    bool flipX = false, flipY = false;
    float angle = 0.f;                // 角度制，顺时针，绕 dst 中心
};

struct Vertex { float x, y; Color color; float u, v; };   // u、v 归一化 [0,1]

struct TextStyle {
    bool shadow = false;  Color shadowColor{0, 0, 0, 160};  int shadowOffset = 2;
    bool outline = false; Color outlineColor{0, 0, 0, 220};
    int outlineWidth = 1;             // 追加字段（契约之外），伤害数字建议 2
};

constexpr std::uint32_t packRgba(const Color&);      // createTexture 的像素：0xRRGGBBAA

class OwnedTexture;                                  // 自有纹理的 RAII 句柄，见 2.3
```

混合公式（src 为画上去的颜色、a 为其 alpha、dst 为目标原色）：

| 模式 | 公式 | 典型用途 |
| --- | --- | --- |
| `None` | `dst = src`（连 alpha 一起拷） | 整张拷贝、降采样 |
| `Alpha` | `dst = src·a + dst·(1−a)` | 一切普通绘制 |
| `Add` | `dst = src·a + dst` | 光斑、刀光、火星、萤火 |
| `Mod` | `dst = src·dst`（**不看 alpha**） | 光照图、整屏调色 |
| `Mul` | `dst = src·dst + dst·(1−a)` | 带透明度的乘色（a=0 时不变） |

---

## 2. `Engine` 新接口

### 2.1 纹理

| 签名 | 语义 |
| --- | --- |
| `TextureId loadTexture(const std::string& path)` | **旧签名**。现等于 `loadTexture(path, ScaleMode::Pixel)`（原先是 NEAREST；游戏里此前没有任何调用点）。 |
| `TextureId loadTexture(const std::string& path, ScaleMode)` | 解析顺序：原样可用 → **资源根下的相对路径**（`"art/fx/glow.png"`，游戏从工程根、测试从 `build-xxx/` 启动都认得）→ 资源根 `textures/`。按「路径 + 取样方式」缓存，同一张图载两次拿到同一个句柄。失败返回 `kInvalidTexture` 并打一行 WARNING（施工图 1.2「缺图不崩」）。 |
| `TextureId createTexture(int w, int h, const std::vector<std::uint32_t>& rgba, ScaleMode)` | 从像素建纹理，`rgba.size()` 必须等于 `w*h`（不等返回无效句柄并记错误日志）。元素格式 `0xRRGGBBAA`（`packRgba`）。**归调用方所有。** |
| `Point textureSize(TextureId) const` | 像素尺寸；无效、已销毁、无头一律 `{0,0}`。 |
| `void setTextureScaleMode(TextureId, ScaleMode)` | 改取样方式（后处理的场景图「降采样时线性、贴屏时像素」两用）。 |
| `void destroyTexture(TextureId)` | 销毁；句柄随即失效，再画是空操作。销毁的若是当前渲染目标，先切回后缓冲。按路径缓存的纹理一并摘掉缓存项（别人手里的同一句柄也失效）。 |
| `std::vector<std::string> findAssets(dir, pattern) const` | 资源根下 `dir` 里匹配通配符（`*` `?`，不分大小写）的文件，**排好序的完整路径**。目录不存在返回空。无头可用。给「美术还在路上、有几张用几张」的场合。 |

### 2.2 渲染目标

| 签名 | 语义 |
| --- | --- |
| `TextureId createRenderTarget(int w, int h, ScaleMode)` | 离屏画布，ARGB8888，**建好时清成全透明**。归调用方所有。 |
| `void setRenderTarget(TextureId)` | 之后的绘制画进它；`kInvalidTexture` = 回到后缓冲。不是渲染目标的（或已销毁的）句柄按后缓冲处理并打警告。 |
| `TextureId renderTarget() const` | 当前目标（后缓冲为 `kInvalidTexture`）。 |
| `void clear(const Color&)` | 整个当前目标清成该色（连 alpha，不混合，不受裁剪影响）。 |
| `TextureId snapshotBackbuffer()` | 把后缓冲**当前**画面拷成新纹理（线性取样），归调用方所有。GPU→CPU 回读：偶尔一次（开战那一帧、开菜单那一帧），**别每帧调**。必须在 `endFrame` 之前。 |

**每个目标各有一套视口、裁剪、坐标系**（SDL3 的规定）：画进一张 w×h 的目标时坐标就是那张图的像素坐标，
不是 1280×720 逻辑坐标；在目标 A 上设的裁剪，切到 B 再切回 A 时还在。

`beginFrame` 会把目标切回后缓冲、取消后缓冲的裁剪再清屏；`endFrame` 先切回后缓冲再 present。
上一帧忘了收尾不会让下一帧整个画进离屏纹理里。

### 2.3 纹理归谁（最容易漏显存的地方）

- `loadTexture` 的纹理**按路径共享**，引擎 shutdown 时统一释放。不用时一般不必管；
  离开一张地图想丢掉它的烘焙图（`below.png` / `above.png` 各几 MB）时可以 `destroyTexture`。
- `createTexture` / `createRenderTarget` / `snapshotBackbuffer` 的纹理**归建它的人**。用 `OwnedTexture` 包：

```cpp
engine::OwnedTexture canvas(engine, engine.createRenderTarget(1280, 720, engine::ScaleMode::Linear));
// ... canvas.get() 当句柄用；析构时自动 destroyTexture。只可移动，不可拷贝。
```

`OwnedTexture` 必须在它引用的 `Engine` 之前析构（`Application::shutdown` 先清场景栈、再关引擎，
场景成员里放它是安全的）；引擎 shutdown 之后再析构是安全的空操作（句柄编号不复用）。
**不要**拿它包 `loadTexture` 的纹理。

### 2.4 绘制

| 签名 | 语义 |
| --- | --- |
| `void drawTexture(TextureId, const RectF& src, const RectF& dst, const DrawOptions&)` | `src` 宽或高非正 = 整张图。旋转绕 `dst` 中心、顺时针。无效/过期句柄不画不崩。 |
| `void fillRect(const RectF&, const Color&, BlendMode)` | 实心矩形，按指定混合。 |
| `void drawGradient(const RectF&, const Color& top, const Color& bottom, BlendMode)` | 竖向渐变。颜色带 alpha，可渐隐。 |
| `void drawGradientH(const RectF&, const Color& left, const Color& right, BlendMode)` | 横向渐变（选中行的渐隐金条）。 |
| `void drawGeometry(TextureId, const std::vector<Vertex>&, const std::vector<int>& indices, BlendMode)` | 三角形列表。`texture` 可为 `kInvalidTexture`（纯顶点色）；`indices` 每三个一组，为空时按顶点顺序每三个一组。**有纹理时只看顶点色**（SDL 规定：`DrawOptions::tint` 这类纹理调制在这里不生效）。下标越界、不是 3 的倍数：SDL 拒画，打警告。 |
| `void drawLine(x1, y1, x2, y2, const Color&)` | 1 像素线，Alpha 混合。 |
| `void setClipRect(const Rect*)` | 当前目标的裁剪；`nullptr` 取消。 |

同一张纹理这次按新接口加了 tint / Add，下次按旧接口画，**旧接口会自己复位**（原色 + 纹理自己的缺省混合），
状态不会漏过去——这是「旧画面不变」的一部分，`SoftwareEngine.TintFlipAndRotationAreHonoured` 钉着。

### 2.5 文字

```cpp
void drawText(const std::string& utf8, int x, int y, int size, const Color&, const TextStyle&);
```

- 正文落在 `(x, y)`，与旧接口同一处：描边往外扩 `outlineWidth` 像素，投影往右下偏 `shadowOffset`。
  **排版（`measureText`）不必为样式改一个数**；只是有墨的范围四周各多出 `outlineWidth`。
- 描边用 SDL_ttf 的真描边（`TTF_CopyFont` + `TTF_SetFontOutline`，每个「字号 × 描边宽」一份字体），
  不是八方向偏移叠画：一行字两次绘制（描边层 + 正文），字形边缘是连续的。
- 开着描边时投影用描边字形（影子跟着整个带边的字走）。
- 缓存与旧接口同一套（按 文本 × 字号 × 颜色 × 描边宽），命中时零分配。**颜色每帧变（闪烁、渐隐）会每帧建一张新纹理**——
  要淡入淡出的文字，别改 `color.a` 逐帧渐变；画到渲染目标里再整体调 `tint.a`，或者接受几十帧的缓存抖动。
- `TextStyle{}`（两样都关）画出来与旧接口逐像素相同。

### 2.6 时钟与按键

- `double elapsedSeconds() const`：init 以来每次 `beginFrame` 的 `deltaSeconds()` 之和（钳位后的 dt，
  断点停十秒动画不会跳十秒）。重新 init 从 0 算。**截图口只 `tick` 不 `beginFrame`**，所以要可复现的画面，
  动画相位用场景自己累加的 `update(dt)`，不要用它（`FxDemoScene` 就是这么做的）。
- `Engine::Key::Action`：E 与 Q。追加在 `Save` 之后、`Count` 之前（`EngineTests` 用 `static_assert` 钉着）。

### 2.7 抓帧

`captureFrame(path)` 语义不变（必须在 `endFrame` 之前、无头如实失败），追加一条：**调用时绑着别的渲染目标，
照样抓后缓冲**，抓完把目标恢复原样。

---

## 3. 后处理 `engine::PostFx`（`PostFx.h`）

### 3.1 一帧的调用次序（照抄这个顺序）

```cpp
// 成员：std::unique_ptr<engine::PostFx> fx_;  在 onEnter 里 make_unique<engine::PostFx>(app.engine())

fx_->beginScene();                    // ① 切到 sceneRT，清成不透明黑
drawWorld(engine);                    //    世界：烘焙图、人物、特效……（全都会经过后处理）
worldParticles.render(engine, camX, camY);

if (fx_->beginEmissive()) {           // ② 可选：发光体另画一份（辉光的素材）
    drawLanternGlows(engine);         //    返回 false（无头 / 建不出目标）时一笔都别画
    fireflies.render(engine, camX, camY);
    fx_->endEmissive();               //    切回 sceneRT，可以接着画世界
}

fx_->addLight({x, y, radius, color, intensity});   // ③ 若干点光源（屏幕像素坐标）

fx_->endScene(settings);              // ④ 光照 → 辉光 → 景深 → 暗角 → 调色，合成回 beginScene 时的目标
drawUi(engine);                       // ⑤ UI：不糊、不压暗、不被调色
```

- `endScene` 合成到 **`beginScene` 那一刻绑着的目标**（通常是后缓冲），并切到那里。
  所以战斗转场可以先 `setRenderTarget(自己的画布)` 再 `beginScene`，结果就落在画布上。
- 一帧里可以多次 `beginEmissive` / `endEmissive`；第一次进来时清屏。**没进过辉光通道的帧不会加辉光**（不吃上一帧的剩饭）。
- 没有 `beginScene` 的帧里调 `endScene` 是空操作。

### 3.2 参数

```cpp
struct PostFxSettings {
    Color ambient{255, 255, 255};   // 环境光。纯白 = 不做光照（光源也就没意义）
    float dof = 0;                  // 景深强度 [0,1]
    float dofFocus = 0.52f;         // 清晰带中心（屏幕高度比例，0 顶 1 底）
    float dofBand = 0.34f;          // 清晰带半宽：量到过渡带正中，该处恰好半糊
    float bloom = 0;                // 辉光强度 [0,1]
    float vignette = 0;             // 暗角强度 [0,1]
    Color gradeMul{255, 255, 255};  // 整屏乘（Mod）
    Color gradeAdd{0, 0, 0};        // 整屏加（Add）
    float dofRamp = 0.15f;          // 追加字段：过渡带全宽（屏幕高度比例）
};
struct Light { float x, y; float radius; Color color; float intensity = 1.f; };
```

- **景深曲线**：`a(y) = smoothstep((|y−focus| − (band − ramp/2)) / ramp)` × `dof`。
  `|y−focus| ≤ band−ramp/2` 全清，`≥ band+ramp/2` 全糊，`focus±band` 处恰好 0.5。缺省值下全清的是
  y∈[0.255, 0.785]（屏幕中间一半多），上缘 0~76 像素、下缘 673~720 像素全糊，其间是过渡。
  想要更重的移轴：`dofBand` 调小、`dofRamp` 调大。
- **光照**：光照图（半尺寸）清成 `ambient`，每盏灯用径向图 `(1−d²)²` 以 Add 叠上 `color`，再 Mod 乘回场景。
  光照图最亮就是 255，所以光源只能把局部**恢复**到原色，不会过曝——「亮到发白」交给辉光。
  `intensity` 大于 1 叠画多遍（上限 4），效果是亮芯更大、衰减更晚。
  **`ambient` 纯白时整步跳过**（光照图处处 ≥ 1，乘回去恒等）。
- **辉光**：emissiveRT → 1/2 → … → **1/64** 线性降采样，再由糊到清逐级放大叠回（每级 ×0.85），最后以 `bloom` 为 alpha Add 回场景。
  施工图写的是降三级；实测三级的光晕只比发光体大十几个像素（每一级只把光推出「一个本级像素」），
  所以加深到六级，光晕能铺出几十个像素。多出的三级合计不到两万像素。
  **辉光通道里画「光晕的形状」**：比场景里的本体更亮、更大、边缘渐隐（`FxDemoScene::drawEmissive`）。
- **暗角**：运行时生成的 320×180 黑色径向图（中心约 42% 对角线内完全透明），拉满全屏，`vignette` 为 alpha。
- **调色**：先 `gradeMul` 以 Mod、后 `gradeAdd` 以 Add；等于缺省值的那层跳过。
- **全部缺省 = 恒等变换**（逐像素相同，`SoftwarePostFx.DefaultSettingsReproduceTheSceneExactly` 钉着）。

参数的出处：地图的时辰预设（`meta.json`）与战斗场景各自一套。`FxDemoScene.cpp` 的 `kPresets`
是四套现成的参考值（夜 / 昼 / 雨 / 雪）。

### 3.3 快照（开战碎屏、菜单背景模糊）

```cpp
TextureId shot = fx_->snapshot();         // 后缓冲当前画面；必须在 endFrame 之前
TextureId soft = fx_->blurredSnapshot();  // 同一帧的模糊版（1/4 尺寸、线性），整屏拉伸着画
```

- 两张图都**归 `PostFx` 所有**：下一次 `snapshot()` 顶掉上一次（旧句柄随之失效），`PostFx` 析构时释放。
  要长期留着就自己 `createRenderTarget` 拷一份。
- 用法：碎屏——开战那一帧世界画完后 `snapshot()`，战斗场景拿 `shot` 当纹理、用 `drawGeometry` 切成三角形飞散；
  菜单——打开菜单那一帧 `snapshot()`，之后每帧画 `blurredSnapshot()` 当底，再画面板。
- 无头返回 `kInvalidTexture`。

### 3.4 纯函数（给要自己画同一种曲线的人）

`dofBlurAlpha`、`dofBandRows`、`lightFalloff`、`vignetteAlpha`、`downsampleSize`、`radialLightPixels`、`vignettePixels`，
语义见头文件注释，`tests/PostFxTests.cpp` 逐条扫着。

---

## 4. 粒子 `engine::ParticleSystem`（`Particles.h`）

```cpp
engine::ParticleSystem leaves(种子);                       // 种子相同 + update 序列相同 → 一颗不差
leaves.configure(engine::ParticleKind::Leaf, 6.f,          // 每秒生 6 片（负数按 0）
                 {0, 0, 1280, 720},                         // area：含义见下表
                 engine::ParticleSpace::Screen);            // 或 World
leaves.prewarm(8.f);                                        // 可选：进场第一帧就是稳态
// 每帧
leaves.update(dt);
leaves.render(engine, cameraX, cameraY);                    // World 空间减去镜头；Screen 空间不理镜头
```

| 种类 | 行为 | `area` 的含义 | 混合 | 美术文件（`assets/art/fx/`，按顺序找，找到的那组全用） | 没有美术时 |
| --- | --- | --- | --- | --- | --- |
| `Dust` 尘埃光点 | 几乎不动、慢浮、微微明灭 | 在区域里随处生灭 | Add | `dust*.png`（像素）→ `glow*.png`（柔图） | 程序光点 |
| `Firefly` 萤火 | 李萨如曲线游走、一明一灭 | 同上 | Add | `firefly*.png`（像素，亮/暗两帧按 3 帧/秒轮播）→ `glow*.png` | 程序光点 |
| `Petal` 花瓣 | 飘落、左右摆、旋转 | 从顶边外进、底边外出 | Alpha | `petal*.png`（像素，每帧一种花瓣形） | 2 种像素小图 |
| `Leaf` 落叶 | 同上，更大更重 | 同上 | Alpha | `leaves*.png` → `leaf*.png`（像素，每帧一种叶形） | 3 种像素小图 |
| `Rain` 雨 | 快、向左斜 | 同上（迎风侧自动多铺一截） | Alpha | `rain*.png`（像素竖丝，转到速度方向） | 画成线 |
| `Snow` 雪 | 慢落、轻摆 | 同上 | Alpha | `snow*.png`（像素，每帧一种大小）→ `softcircle*.png` | 程序柔圆 |
| `Ember` 火星 | 上升、闪烁、越飞越红（柔图还越飞越小） | 从**底边**往上冒，冒出区域后照样飞到寿命用完（区域给火盆口那一窄条即可） | Add | `ember*.png`（像素，**自带火色**，10 帧/秒明灭）→ `spark*.png` → `glow*.png` | 程序光点 |
| `Mist` 薄雾 | 大团（200–360 像素）、很淡、横移 | 在区域里随处生灭 | Alpha | `mist*.png` → `softcircle*.png`（柔图） | 程序柔圆 |

这张表照的是 A2 路 `tools/artgen/fx.py` 的实际产物（`assets/art/fx/index.json`、`docs/art-sprites.md`）：

- **帧**：多帧图**单行横排、方形帧**，帧数 = 宽 ÷ 高（`sheetLayout`；宽不足高的雨丝 4×16 就是一帧）。
  引擎层不读 `index.json`（不解析 JSON 是分层约定），靠这条约定推帧——**A2 若改成多行或非方形帧，这里要跟着改**。
  有 fps 的（火星、萤火）按寿命轮播、各颗错开起点；没有的每颗粒子固定一帧（`particleFrame`）。
- **尺寸**：像素美术（dust / firefly / embers / spark / leaves / petals / snow / rain）按**帧原尺寸 ×3**
  （`kParticlePixelScale`，与地图、人物同一像素密度）画，`ScaleMode::Pixel`；柔图（glow / softcircle）按粒子自身大小拉伸，`ScaleMode::Linear`。
- **上色**：灰度或白色出图的一律乘种类配色（叶的青黄、花瓣的粉、萤火的黄绿……）；只有 `embers.png` 自带火色，不乘。
- **不用的**：`fog.png` 是四边无缝的**平铺**雾纹——当粒子画会露出方形的边。它适合整屏慢速平移的雾层
  （地图路要的话：按 128 的倍数铺满、每帧挪一点偏移、Alpha 画在世界之上），薄雾粒子仍用柔圆。
  `shaft.png`（光柱）、`shadow.png`（脚下柔影）、`slash` / `impact` / `shards` 等是地图与战斗路直接拿去 `drawTexture` 的，不经粒子系统。
- 贴图在第一次 `render` 时解析（要 `Engine` 才找得到文件）；换了种类（`configure`）、换了引擎、
  `setArtDirectory` 换了目录都会重新解析。`usesArt()` 报上一次 render 用的是不是美术贴图。
  美术是后到的：已经在跑的系统不会自己发现新文件，重新进场景即可。
- **上限**：每个系统缺省 512 颗（`setCapacity` 可改，调小时丢最老的）。满了新的就不生，也不攒着以后爆发。
- `configure` 会清空现有粒子（一种新效果）；`clear()` 只清粒子不改配置。
- 发光的粒子（萤火、火星）想要辉光：同一个系统在 `beginEmissive` 里**再 render 一次**（`FxDemoScene` 的 `Layer::emissive`）。
- 粒子系统可移动、不可拷贝（持有程序贴图的 `OwnedTexture`），可以放进 `std::vector`。

纯函数：`lifeEnvelope`、`particleBlend`、`particleScreenPosition`、`proceduralSprite` / `proceduralVariants`。

---

## 5. 性能

- **渲染目标只建一次**：`PostFx` 在第一次 `beginScene` / `snapshot` 时建全部目标（1/1 ×3、1/2 ×2、1/4 ~ 1/64 链、快照模糊），之后每帧复用，
  合计约 14 MB 显存。**一个场景一个 `PostFx`**，不要每帧 new。
- 整条管线全开，每帧约 9 次全屏填充（场景贴回、光照 Mod、辉光 Add、景深模糊放大与景深带、暗角、两层调色）加若干小图——
  1280×720 下每次全屏填充在 GPU 上是零点几毫秒的量级，硬件渲染器毫无压力。
  **硬件渲染器（D3D11）上本机没有实测帧时间**（截图口与测试都是软件渲染器，量硬件就得开一个真窗口）。
  软件渲染器上实测（同一画面、20 帧平均、都含抓帧存 PNG）：不做后处理约 19 ms/帧，整条管线全开约 81 ms/帧，
  后处理本身约 62 ms/帧——那是单线程 CPU 逐像素做十来遍全屏线性取样的代价，只关乎截图口快不快，不代表游戏帧率。
- 每帧不分配：`PostFx` 的光源表、景深几何，`Engine::drawGeometry` 的顶点转换缓冲都复用。
  自己画几何的，照 `FxDemoScene::Mesh` 的样子复用 `std::vector`。
- 同一张纹理连续画（粒子、瓦片）SDL 会自己合批；穿插着换纹理、换混合模式会打断合批。
- `snapshotBackbuffer` / `PostFx::snapshot` 是 GPU→CPU 回读，一次几毫秒，只在「那一帧」调。
- 文字：颜色逐帧变化的文字每帧建纹理（见 2.5）。

---

## 6. 无头契约（CI 与无头 bot）

| 接口 | 无头下 |
| --- | --- |
| `loadTexture`（两个）、`createTexture`、`createRenderTarget`、`snapshotBackbuffer` | 返回 `kInvalidTexture`（哪怕文件真实存在） |
| `textureSize` | `{0,0}` |
| `renderTarget` | 恒为 `kInvalidTexture` |
| `setRenderTarget`、`clear`、`drawTexture`、`fillRect`、`drawGradient(H)`、`drawGeometry`、`drawLine`、`setClipRect`、`drawText`（两个）、`setTextureScaleMode`、`destroyTexture` | 空操作 |
| `captureFrame` | 如实失败 |
| `findAssets`、`measureText`、`elapsedSeconds` | 照常可用 |
| `PostFx` 全部 | 空操作；`beginEmissive` 返回 false；`active()` 为 false；快照为无效句柄 |
| `ParticleSystem::update` / `prewarm` | 照常模拟（它不碰引擎）；`render` 空操作 |

`tests/EngineTests.cpp`（`HeadlessEngine.*`）、`tests/PostFxTests.cpp`（`PostFxHeadless.*`）、
`tests/ParticlesTests.cpp`（`Particles.RenderingHeadlessIsANoOp`）钉着这张表。

---

## 7. 截图与自检

```powershell
$env:SDL_VIDEODRIVER="dummy"   # 不开窗口：dummy 视频驱动 + 软件渲染器
build-<槽名>\fanren.exe --assets . --map ch01_hanjiacun --scene fxdemo --screenshot fx-night.png
#   --scene fxdemo:day / fxdemo:rain / fxdemo:snow
```

- `--scene fxdemo[:预设]` 压一层 `FxDemoScene`：夜景全开（夜间环境光 + 灯笼 ×4 + 火盆 + 月光 + 人物微光 + 辉光 + 景深 + 暗角 + 调色，
  薄雾 / 萤火 / 火星）；昼（尘埃光点 / 花瓣 / 落叶 + 光柱）；雨；雪。布景全是运行时画的，不依赖任何美术产物；
  粒子有美术贴图就用美术的，右下角一行字写明这一张用的是「美术产物」还是「程序生成」。认不出的预设如实报错退出。
- **所有新能力都在软件渲染器上工作**，截图验收靠它；`tests/*` 的 `SoftwareEngine.*` / `SoftwarePostFx.*` 用同一套配法
  （`SDL_HINT_VIDEO_DRIVER=dummy` + `SDL_HINT_RENDER_DRIVER=software`）在 CI 里逐像素验，走的也是 `captureFrame → PNG → 读回`。
- 软件渲染器的两处与硬件不同，写渲染代码时记着：
  1. **带顶点色（不一致）的纹理三角形只做最近邻取样**。拿小图放大着画、又要顶点渐变时，软件渲染器上是一块块马赛克。
     景深带因此改用放大回原尺寸的模糊图、按 1:1 取样；需要同类效果的照此办理。
  2. `ScaleMode::Pixel`（PIXELART）在软件渲染器上等同最近邻；硬件上非整数倍时边缘更干净。截图看不出这点差别。
- 旧画面不变的核对：改造前后各拍一张 `--map ch01_hanjiacun`，逐像素比对（交付报告里有数）。

---

## 8. 已知限制与给下游的提醒

- **透明画布再贴回去会发暗**：渲染目标清成全透明后以 Alpha 画半透明的东西，颜色已经乘过一次 alpha；
  再以 Alpha 贴回去又乘一次，边缘发灰。要么把画布清成不透明底色（场景图、辉光通道都这么做），
  要么整层以 `None` 拷贝。需要「预乘 alpha」混合时找引擎路加（SDL 有 `BLEND_PREMULTIPLIED`，本轮没开）。
- 裁剪、视口按目标各一份（2.2）；在画布上设过裁剪，画完记得 `setClipRect(nullptr)`。
- `snapshotBackbuffer` 在窗口比 1280×720 大（整数倍放大、带黑边）时读的是内容区，尺寸可能是 2560×1440 之类；
  画的时候 `src` 用整张（宽高给 0）即可，别写死 1280×720。
- `ParticleSystem` 的 World 空间只做「减去镜头」，没有视差；远景粒子要视差就按层给不同的镜头系数。
- 粒子的贴图文件名按通配符找（第 4 节表格）。A2 若改了名字或画法约定，改 `Particles.cpp` 的 `artChoicesOf` 一处即可。
