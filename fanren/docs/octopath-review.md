# 八方旅人化改造 · 终审报告（第 1–5 章）

> 独立终审方（R-A）。不参与创作；除本文件外未改动工作树（变异注入全在 scratchpad 的拷贝里做，做完已删，见附录 D）。
> 受检树 `ROLLUP=bfb7bf89…ee90b5749`（2285 个文件，不含 build*/saves/日志）。收尾时这 2285 个文件逐一相同，
> 只多出一份 R-B 的 `docs/ch05-reverify.md`（附录 D）。
> 构建槽 `finalrev` · 机器日期 2026-09-26（协调者文档写的是 09-27，时钟对不上，见 L-7）。
> 截图目录：`C:\Users\htx-lyh\AppData\Local\Temp\claude\H--Work-Kys\72d03c6b-2b5f-46cb-ae43-1ef00b7360da\scratchpad\finalrev\`（下文只写文件名）。

---

## 结论先行

**评分：4.0 / 5（验收线 4.0）。裁决：通过，附一条收尾前必修（HIGH-1）。**
HIGH-1 是一行接线加一条游戏层测试的活，修完按第八节给的判据复核三张截图即可，不必再开一轮全量终审；
三条 MEDIUM 进技术债。

这一轮改得又大又扎实。战棋换成了横版「破势与蓄劲」：规则、AI、事件流、HUD、存档 v7/v8、路径行动 42 条、
野外遭遇三张图，全都从正常启动的游戏里够得着（标题 → 新的旅程 → 章节卡 → 韩家村；Tab/Esc 主菜单；E 键与头顶气泡；
遭遇在 `main.cpp` 打开）。战斗画面是整轮最像八方旅人的一块：视差背景、行动序条、敌人脚下的架势盾加「？」破绽格、
劲珠、气焰、破势碎玉慢镜、胜利结算，和改造前那张灰格子（`before/battle.png`）比已经是另一个游戏。
四道门禁我自己跑了，全过；1312/1312；全新目录 177 步编译零警告；同一场战斗两次截图逐像素相同。
我做了六处数据变异、两处门禁负向，**有设计原文判据的地方全都转红**：第 3 章「砍不动就下毒」、
第 5 章斩杀线、欧阳飞天「只有剑符能取首级」。

压着分数的是下面这些：

1. **HIGH-1：`data/visual/battles.json` 在游戏里从来读不到。** 协调者在第 8 节写了「让 battles.json 在运行时生效，加 3 条测试」，
   可运行时是按引擎的资产根（`<仓库>/assets`）去找 `data/visual/battles.json`，那个路径不存在，指派表永远是空的。
   按表里自己登记的所在图推算，**31 场里有 14 场背景是错的**：第 4 章的夜袭攻防战（三张）画成了晴天，
   墨府地窖的尸傀打在月下院子里，潇湘院打在江边码头。那 3 条测试只测了加载器，没测游戏里的入口。
   我把同一份表放到代码真正去找的位置，夜景当场出来了（`g_backdrop_proof.png`）。
2. **MEDIUM：第 5 章那只手在「先吃药还是先打」这件事上不稳（G-11）。** 「先吃金疮药」那一只在 ⑦ 墨府尸傀
   四条路里有三条 **game_over**，揣 6 瓶也救不回来；只看意图玩家那一只全绿。第 4 章攻防战只钉了上界
   （揣够 8 瓶必赢），没钉下界：我把攻防三张编成里所有敌人的攻击减半，只有章末存档漂移那一条转红。
   「升阶」「开门」两种音效已经生成了，但游戏里一处都没播。
3. **画面**：世界地图是很精致的 16 位俯视像素画，加了光照和粒子，但景深带很弱（dof 0.3），也没有立体透视，
   离 HD-2D 的「立体场景 + 移轴」还有一段；有几张图放大到新尺度后显得空（演武场），有两处地砖读起来像墙
   （客栈、墨府院子）。

---

## 一、门禁与构建（全部我自己跑，不引用任何人的数字）

`& "H:\Work\Kys\fanren\build_logged.bat" finalrev`，2026-09-27 03:14:19 → 03:20:39（+08:00）。
日志 `build-finalrev.log` 的 mtime 是 19:20:39Z，和我这次构建的结束时间对得上。日志是 UTF-8，不是 gb18030。

| 项 | 结果 |
| --- | --- |
| `validate.py` | `VALIDATE_OK`：数据条目 233、文案 1818、旗标 169，错误 0、警告 0 |
| `mapgen --check` | `MAPGEN_IN_SYNC`：24 张图与生成器一字不差 |
| `validate_selftest.py` | `SELFTEST_OK`：用例 177 条，未如期抓住 0 条 |
| `artgen --check` | `ARTGEN_IN_SYNC`：257 个文件逐像素一致，**18.1 s**（施工图目标 < 30 s） |
| 编译 | 全新目录，ninja 177 步，`/W4 /permissive-`，warning / error / LNK 行都是 **0** |
| ctest | **100% tests passed, 0 tests failed out of 1312**，133.27 s，`EXITCODE=0` |
| 直接跑 `fanren_tests.exe --gtest_brief=1` | 1312 tests / 215 suites 全 PASSED，**0 SKIPPED**，39.2 s |

改造前基线是 1062 条，这一轮净增 250 条。
编译的源码是最新的：协调者最后一次改源码在 19:05:43Z，早于我开编的 19:14:19Z。
之后只有 `docs/octopath-overhaul.md` 在 19:14:26Z 改过一次（补第 8 节的记录），是纯文档，不影响构建。

---

## 二、施工图逐节核对

✓ = 做了且我实测过；△ = 做了但有偏差（有的已登记，有的没有）；✗ = 没做，或者做了但在游戏里不生效。

| 节 | 条目 | 状态 | 在哪 / 证据 |
| --- | --- | --- | --- |
| 1.1 | 16px 美术 ×3 上屏、逻辑格仍是 32 | ✓ | `WorldView.h:49` 的 `kWorldTilePx`；`tryStep` 的语义没动（改动前的地图与对象测试照旧全绿） |
| 1.1 | 像素图用 PIXELART、模糊用 LINEAR、文字按 1280×720 原生画 | ✓ | `Engine.cpp:156`、`:1028`；截图里的字都清楚 |
| 1.2 | 资产目录、生成物进仓库、`--check` 门禁 | ✓ | 257 个文件；我改一个像素，门禁 rc=1，报出了那张图（附录 B） |
| 1.2 | 缺图不崩、退回旧画法并打 WARNING | ✓ | 在拷贝里删掉 `below.png` 和 `index.json`：SHOT_OK，两行 `[world] WARNING`（`missing_art.png`） |
| 1.3 | 烘焙 below/above/meta | ✓ | 24 张图都有；`MapArtTests` 的语义表照旧钉着 |
| 1.4 | 引擎新接口与 PostFx 管线，无头时是空操作 | ✓ | `Engine.h`、`PostFx.*`、`Particles.*`；`HeadlessEngine.*` 测试；`--headless` 冒烟跑两次输出相同 |
| 1.5 | Tab/Esc/X 开主菜单，E/Q 路径行动，战斗左右键调蓄劲，Ctrl 加速 | △ | `Engine.cpp:65-75`、`WorldScene.cpp:247-261`、`BattleScene.cpp:860-861`、`:1150`。**战斗里按 Tab 不能返回**（只认 Cancel，`:869`），见 L-3 |
| 1.6 | 墨金主题 token | △ | `ui/Widgets.h:27-30` 与施工图的值一致（`InkGoldTheme.*` 钉着）；BattleHud/BattleScene 还剩几处语义色写死在代码里（L-4） |
| 2.1 | 横版、行动序（本回合 + 下回合） | ✓ | 截图 41/50 顶端的序条；`BattleTurnOrder.*` |
| 2.2 | 架势、十类破绽、初始显示「？」、打中即揭、打探可提前揭 | △ | 规则按设计原文钉死（`BattleBreak.*`）。**架势数值下调**：杂兵 2 / 头目 3 / 首领 4–8（施工图写的是 2–4 / 5–8 / 10–20），理由写在 `octopath-battle.md` 7.1，属于已登记的偏差 |
| 2.3 | 开场 1 点劲、每回合 +1、上限 5、蓄过的下回合不加、蓄 0–3 | ✓ | `Battle.h:32-34`；`BattleBp.*` 用的是设计原文 |
| 2.4 | 防御减半并下回合先手；破势 ×2；伤害公式沿用 | ✓ | `Damage.cpp/.h` 与快照逐字节相同；`BattleBreak.ABrokenFoeTakesDoubleDamageFromAnything`、`BattleGuard.*` |
| 2.5 | 首领多次行动、蓄势（暗红气焰 + 预告句），破势可打断 | ✓ | 截图 74（欧阳飞天的预告横幅加气焰）；`BattleBoss.*` |
| 3 | 烘焙图、按 y 排序、柔影、插值滑步 0.12 s、镜头缓动 | ✓ | `WorldView.h:53`、`WorldView.cpp:34,437` |
| 3 | 时辰光照、夜里的光源、主角夜里自带微光、粒子 | ✓ | 截图 19、20（夜）、13、22（黄昏）、12、14、21（室内） |
| 3 | 景深移轴 | △ | 有，但很弱：dof 0.3，我放大裁图才看得出上下带有一点虚（`z_dof_qingniu.png`） |
| 3 | 传送点淡入淡出、脚本 fade/wait、开战碎屏 | ✓ | 截图 54、66 是碎屏；`FadeLevels.*` |
| 3（战斗背景） | 按 `battles.json` 给每场指派背景 | **✗** | **HIGH-1**：`BattleView.cpp:204` 的路径找不到，指派表恒为空 |
| 4 | 标题画面 | ✓ | 截图 01；菜单写的是「继续旅程」，不是施工图的「读取存档」，无伤大雅 |
| 4 | 章节卡 | △ | 截图 03、04、05。用完成旗标自动排卡，**没有做** Lua `chapter.begin/finish`，第 8 节已登记 |
| 4 | 地名横幅，野外图带星级 | ✓ | 截图 15（★☆☆☆☆）、20（★★☆☆☆） |
| 4 | 对话框：名签、逐字、▼ | ✓ | 截图 35 |
| 4 | 主菜单：状态 / 物品 / 法门 / 记事 / 存盘 / 返回 | ✓ | 截图 30–34；第 1–5 章显示「口诀第几层」「碎银」 |
| 4 | 胜利结算（还掉 G-15） | ✓ | 截图 43（修为 +15）、79（修为 +60、碎银 30）、67（修为 +3、黄精 ×1） |
| 4 | BGM | ✓ | 18 首；24 张图都挂了曲子；战斗、首领、识海、标题都会切（`BattleScene.cpp:291,979-986`、`TitleScene.cpp:126`） |
| 4 | 音效：光标、确定、取消、打击、破绽、破势、蓄劲、升阶、开门 | △ | 九种里七种接上了。**`realm_up`、`door_open`（还有 `item_get`）生成了但全仓零调用**，脚本 API 里的 `sfx()` 也没有一处在用（M-3） |
| 5.1 | 路径行动三种：打探 34、求购 4、切磋 4，E 键 + 气泡 | ✓ | 截图 36–38；`PathActionFlow.*`、`SparFlowTests`；阅历门槛 3 条，回绝时不点名境界 |
| 5.1 | 附带「隐藏物件出现」「商店折扣」 | △ | 只有 `set_flags` 这套机制，没有一条内容用到（契约第 5 节已写明现状） |
| 5.2 | 野外遭遇：`main.cpp` 打开，`Application` 默认关 | ✓ | `main.cpp:179`；`EncounterFlow.*`（负了留 1 点血、每天有上限、星级、v7→v8） |
| 5.2 | 放在哪几张图 | △ | 实际三张（谷外、渡口、独霸山庄），施工图列了五处；落日峰没有野地，第 8 节已登记 |
| 0 | 不改的：剧情节点、正文、存档语义 | ✓ | 按 key 对快照比：第 1–4 章正文 **0 条被改**，只有新增；第 5 章的改动是 C5F 整改授权范围内的。存档 6→7→8 都有迁移 |
| 7 | 通关测试全绿，且对自己的取舍稳定 | △ | 第 4 章稳；第 5 章换「先吃金疮药」就 game_over（M-1） |

---

## 三、玩家够得着（按 handoff 第 4 节「三样齐备」逐项核）

| 新东西 | 正常启动时的入口 | 证据 | 结论 |
| --- | --- | --- | --- |
| 标题 → 新的旅程 → 第一章卡 → 韩家村 | 不带参数启动，走 `titleFlow`（`main.cpp:203-205`）→ `TitleScene::update` → `startNewJourney`（`Application.cpp:374-388`） | 截图 01、03；韩家村没有「一进图就跑」的触发器，卡片后面不会有戏插进来 | 够得着。**没有自动化测试覆盖这条链**（`startNewJourney` 零测试），我只读了代码 |
| 继续旅程 | 读 `saves/quick.sav`；F5 与主菜单「存盘」都写这一份 | `TitleScene.cpp:113-121,160-167` | 够得着（见 L-2、L-8） |
| Tab/Esc 主菜单 | `WorldScene.cpp:247-249` | 截图 30–34 | 够得着 |
| E 键路径行动与头顶气泡 | 钩子在 `Application.cpp:204` 接上；E 键在 `WorldScene.cpp:258` | `PathActionFlow.TheWorldSeesTheBubbleAndTheActionKeyOpensTheMenu` 用的是真的 Application；截图 36、37 | 够得着 |
| 切磋开战 / 揭破绽 | `applyPathEffects` → `startBattle` 回调（`Application.cpp:601-648,751-770`） | `SparFlowTests`（四场都「先输后赢」） | 够得着 |
| 章末旗标 → 章节卡 | 脚本置 done 旗标 → `cardsForFlagChange`（`Application.cpp:832`）→ 回到世界层再播 | `ChapterCards.*`、`HeadlessChapterCards.*`；截图 04、05 | 够得着 |
| 野外遭遇 | `main.cpp:179` 打开；`WorldScene::tryStep` → `stepEncounters`（`Application.cpp:688-707`） | `EncounterFlow.*`；截图 15、20 的星级横幅 | 够得着 |
| 战斗：左右键蓄劲、揭破绽、破势、胜利结算 | `BattleScene.cpp:860-861`；事件流由 BattleView 播 | 截图 41（蓄劲 1/1 加气焰）、51（破势）、43、79 | 够得着 |
| 背景按图、按战斗、按脚本切 BGM | `Application.cpp:278-280,866-875`；`BattleScene.cpp:291,979-986` | `EncounterFlow.ScriptBgmSurvivesATeleportAndMapRestoresTheMapTrack` | 够得着（我没有用耳朵听过，见第九节） |
| **战斗背景指派表 `battles.json`** | `BattleScene.cpp:279-288` → `battleBackdropOverride`（`BattleView.cpp:201-213`） | 规则层 ✓、加载器 ✓，**游戏层入口找错了目录** | **够不着**（HIGH-1） |
| 升阶、开门、得物音效 | 无 | 全仓 grep 零调用 | **够不着**（M-3） |

---

## 四、画面（逐张看过；和改造前截图对照）

**和改造前比**：`before/w_hanjiacun.png` 是色块瓦片、一格一个小方人、满屏「闭」字；
现在的 `10_map_hanjiacun.png` 有茅草屋、土路、花瓣、描边名牌、金菱目标标记，墨金的地名横幅和目标框。
`before/battle.png` 是 15×11 的灰格子加一栏字；现在的 `41_b03_elang_target.png` 已经是八方旅人那套战斗画面的格局。

| 类 | 截图 | 观感 | 问题 |
| --- | --- | --- | --- |
| 标题 | 01 | 云海、远山、飞檐、光点，四层视差，字清楚，像样 | 菜单压在亮云上，靠描边才读得清 |
| 章节卡 | 03、04、05 | 黑底金线，很克制 | 比较素：没有画面，也没有人物或地名的图案 |
| 昼 | 10、11、18 | 16 位像素画手艺很好，屋顶、摊子、墙门分得清 | 纯俯视，景深几乎看不出来，更像精修的 SNES 游戏，不是 HD-2D 的立体布景 |
| 黄昏 | 13、15、17、22 | 暖色调得好，落叶、树冠、崖面层次清楚 | 洛日峰的石坪和几根立柱显得空 |
| 夜 | 19、20 | 最接近 HD-2D 的两张：蓝调、萤火、点光源、丹炉发光、主角身上一团光 | 墨府院子铺砖的方向读起来像一面墙 |
| 室内 | 12、14、21 | 灯光暖，暗道的火把带火星，很出彩 | 客栈满地砖纹，地板和墙读不清 |
| 空旷 | 16 | — | 演武场放大到 ×3 后是一大片空沙地，旧地图是按格子密度设计的 |
| 菜单 / 对话 / 路径 | 30–39 | 墨金主题统一，名签和「▼」都有，气泡清楚 | 路径和对话面板固定贴在底部中间；镜头贴着地图下边时会盖住韩立和 NPC（38；P2a 自己也提过） |
| 战斗 | 41、42、43、51、74、77、79 | 视差背景、气焰、破绽格、劲珠、破势碎玉、火弹连发、胜利卡，节奏齐全 | ① 择敌那一行把「破绽…」截掉了（41、64：`BattleScene.cpp:131-134` 拼出来的串超出 330 宽）；② 前冲的人从同伴身上穿过去画在上面（42、78）；③ 曲魂在战斗尺寸下像一块墓碑（有意为之，见 LOW-6）；④ **背景错配**（HIGH-1：70 是晴天下的夜袭，77 在月下院子而不是地窖，79 在码头而不是潇湘院里头） |
| 识海 | 45、46 | 紫色星海，和其他战斗一眼分得开 | — |
| 转场 | 54、66 | 碎屏崩片有力 | — |

另外：截图口 `battle:<id>:charge|victory`，如果要的时刻一直没来（比如用错存档，打到输），会一直推到战败卡。
这是开发工具的行为，不算玩家问题（52、53、63 就是这么来的；换对存档之后拍 74、79 正常）。

---

## 五、玩法手感与平衡

**那只手自己的稳定性（G-11）**

- 第 4 章 `Ch04HandSweep`：三张编成 × 门槛七档 × 药数 N−2 / N / N+2 × 背包三种变体，189 格全胜；
  背包那一维三列逐字相同。「最少要几瓶」的最大值是 8，正好等于 `kSiegeNeedFromDesign`（`Ch04SliceTests.cpp:221`，设计 3.3 原文）。稳。
- 第 5 章 `Ch05HandSweep`：意图玩家那一只（门槛 40 / 50 / 60、P0±2、两侧、两条路）一格不 game_over。
  **但「先吃金疮药 50」在 ⑦ 墨府尸傀：第一侧都不备、第二侧照常、第二侧都不备这三条路都是 game_over**
  （`gtest_direct.txt` 第 304、311、318 行），「最少要几瓶」给的是「⑦>6」。
  这一只只打印、不判（`Ch05SliceTests.cpp:2041`）。B2 在 `octopath-battle.md` 第 10 节写了「那是另一种玩家」，
  可按 G-11 的定义，「先吃哪一瓶」正是一种同样合理的取舍，而它把 ⑦ 的结论从「0 瓶也赢」翻成了「怎么都输」。

**设计意图守住了没有**（下表全是我在拷贝里改数据、用同一个二进制跑全量得到的，附录 B 有原始记录）

| 意图 | 变异 | 结果 | 判定 |
| --- | --- | --- | --- |
| 欧阳飞天只能用剑符取首级 | 删掉 `killable_by` | 4 条红，其中 `Ch05BattleData...:253` 是写死的设计原文 | 有牙 |
| 欧阳飞天的破绽只有「金」 | 破绽加上「火」 | 1 条红（`:252`，设计原文） | 有牙 |
| 第 5 章斩杀线（单轮不过三成） | 尸傀攻击 13 → 20 | 25 条红，含 `NoRoundCanTakeMoreThanThreeTenthsOfTheTargetsLife` 和设计表 8.3 | 有牙 |
| 第 3 章暗道「砍不动就下毒」 | 僵兽防御 10 → 2 | 3 条红，含 `TheBeastThatCannotBeCutDownGoesDownToPoison` | 有牙 |
| 第 4 章揣够设计的药必赢 | 贾天龙攻击 19 → 26 | 扫描照旧全胜，只有章末存档漂移红 | 这一改没有越过上界，结果合理 |
| **第 4 章攻防战是一道药关（下界）** | 攻防三张的敌人攻击全部减半 | **只有章末存档漂移红，扫描和最少几瓶都绿** | **下界没钉住（M-2）** |

最后一行正是 handoff 第 8 节第一条说的情形：存档漂移只是「变化探测器」，而它自己的报错文字就在教人「若是有意改的就重生成」。
重生成之后，一场一瓶药都不用的攻防战也能全绿。

**手感**（数据加截图，没有用手柄玩过）：第 5 章的欧阳飞天在两侧都是「剑符一招取首级」，韩立一滴血不掉。这和原著 ch126
「祭起了剑符，一下就削掉了其脑袋」对得上，但作为章末首领战很短，没练符那一侧也只是「一开打就走」。
这是有意的，我记在这里，不算问题。第 3、4 章的节奏（第 4 回合收狼、破势后蓄满劲打）截图上看是顺的。

---

## 六、禁词与原著

- 我自己写了扫描（`lexscan.py`）：`data/text/*.json` 共 1818 条。有章号的文件按表一（`LexiconTests.cpp:65-89`）加 `chapterCoversUpTo` 查；
  第 1–2 章另加 ch02 硬约束 8 那一组（修仙 / 灵根 / 法术 / 灵气 / 炼气 / 筑基 / 真元 / 走火入魔……）；
  没有章号的 `ui*.json`、`items.json` 按第 1 章最严的口径查。
  **这一轮新写的文案（`chNN_path.json`、`ch04/05_battle.json`、`ui.json` 的凡人一侧、`ui_battle.json`）命中 0 条。**
  `ui.json` 里命中的三处都不算违约：「凡人修仙传」是游戏标题；另两处是 `ui.menu.magic.immortal`（「法术」）
  和 `…empty.immortal`（「尚未习得法术」），属于修仙阶段的备用词，第 1–5 章没人置 `story.xiuxian_known`（G-13），不会显示。
  `items.json` 的命中全在第 6 章以后才拿得到的物品上，而且这份文件这一轮一个字没改。
- 负向（附录 B）：往 `ch03_path.json` 注入「灵石」→ `Lexicon.NoChapterSaysAWordItHasNotEarnedYet` 红；
  往 `ch01_path.json` 注入「修仙」→ `PathActionData.ChaptersOneAndTwoNeverSpeakOfImmortals` 红。
  **`validate.py` 对这两处都放行**：禁词闸门只长在 C++ 测试里，要等全量编译完才会报。
- 措辞前后不一（不违约）：主菜单第 1–5 章说「法门 / 气力」，战斗菜单写的是「耗法力 N」（`BattleScene.cpp:601`）、
  「法力不足」（`BattleAction.cpp:123`）、「法术」。第 3–5 章表一不禁这几个词，但同一样东西在两块界面上叫法不同（L-9）。
- 原著抽查（`novel.py find`）：三叔在七玄门的酒楼、是外门（ch1）✓；吴剑鸣连败十六名情敌（ch106）✓；
  欧阳飞天练「霸王甲」刀枪不入（ch126）✓；剑符削首（ch126）✓。路径行动 note 里写的出处都对得上。

---

## 七、工程质量

- **分层**：nlohmann 只出现在 `src/io/`；`src/core/` 里的「SDL」都是注释；SDL 头只进了 engine 和 `main.cpp`（SDL_main）。
  C1 收口把 ui/game 里的三份 JSON 读取合回了 io，核实属实。
- **存档**：`kSaveVersion = 8`，迁移表里有 6→7、7→8 两条（`SaveFile.cpp:412-525`）。夹具都是 v7，每次读都会走一遍迁移。
  `EncounterSave.*` 和 `BattleKnowledge.*` 覆盖了新旧档和坏档。
- **确定性**：同一场 `b04_gongfang_weijian:mid` 拍两次，逐像素相同，粒子也一样；`--headless` 跑两次输出相同。
- **门禁有牙**：selftest 的 177 条负向用例全部如期抓住；我自己又改了 `below.png` 的一个像素 → `ARTGEN` rc=1，报出了那张图。
- **测试覆盖的洞**：`startNewJourney` 没有测试；`battleBackdropOverride` 没有测试（HIGH-1 就是这么漏过去的）。
  `QuickSaveTests` 每跑一次都会在**真仓库**的 `saves/` 里写入再删除 `quick.sav`（`QuickSaveTests.cpp:51-56`）；
  现在标题画面的「继续旅程」读的正是这一份，开发者每跑一次测试就丢一次手测存档（L-8）。
- **文档陈旧**：见 L-7。

---

## 八、问题清单

### CRITICAL
无。

### HIGH

**HIGH-1　战斗背景指派表在游戏里永远读不到，14/31 场背景错（夜袭画成白天）**
- 位置：`src/game/BattleView.cpp:204` 用 `e.findAssets("data/visual", "battles.json")` 去找。
  引擎的根由 `findAssetRoot()` 定，指向 `<仓库>/assets`（`src/engine/Engine.cpp:85-101`），
  实际查的是 `assets/data/visual/battles.json`，这个文件不存在，于是 `BattleScene.cpp:288` 一律退回「所在地图 meta」。
- 影响：按 `battles.json` 自己登记的 `map` 推算，错配的是 `b04_gongfang_weijian / xiadu / yelangbang_laifan`（夜 → 昼）、
  `b05_mofu_shigui`（地窖 → 月下院子）、`b05_xiaoxiangyuan`（厅堂 → 码头）、`b05_duobang`、`b05_heishuixiang`、`b05_matou_zhuishao`、
  `b05_qiecuo_huyuan`、`b05_qiecuo_lingban`、`b05_wu_jianming`、`b05_yange_qiecuo`、`b05_yesu_yelang`，共 14 场。
  协调者在第 8 节写的「让 battles.json 在运行时生效」**并不成立**。那 3 条测试（`VisualLoaderTests.cpp:316-356`）
  只用 `repoRoot()/data/visual/battles.json` 测了加载器本身。
- 复现：`--assets . --load tests\fixtures\ch04-end-first.sav --map ch04_yanwuchang --scene battle:b04_gongfang_weijian --screenshot x.png` → 晴天（`70_b04_siege_night_mid.png`、`bd_before_fix.png`）。
  在拷贝里把同一份表复制到 `assets/data/visual/` 再拍 → 月夜（`bd_after_fix.png`）。
- 修改建议：在 `Application::init` 里和其他数据一起，用 `assetRoot_ + "/data/visual/battles.json"` 调 `io::loadBattleBackdrops` 读一次，
  再传给 BattleScene，不再经过 `Engine::findAssets`。补一条**游戏层**测试：`BattleScene` 在 `ch04_yanwuchang` 上开
  `b04_gongfang_weijian`，解析出来的背景必须是 `drill_ground_night`，判据照抄 art-maps.md 第 8 节。
  改完拍 70、77、79 三张复核。同时改 `octopath-battle.md` 6.6 里「待接」那句，和第 8 节的那句自述。

### MEDIUM

**M-1　第 5 章那只手对「先吃哪一瓶」敏感：⑦ 墨府尸傀在「先吃金疮药」下 game_over（G-11）**
- 位置：`tests/Ch05SliceTests.cpp:2041`（其余几只手只打印、不判）；数据 `data/roles/mofu_shigui.json`（一回合动两次、攻 13）。
- 复现：直接跑 `fanren_tests.exe`，看 `[ch05 扫描] … 先吃金疮药50` 那几行：三条路 game_over；「最少要几瓶」⑦>6。
- 建议：二选一。要么让 ⑦ 在「先吃金疮药」下也不 game_over（比如把金疮药回血提到够扛一轮，或者尸傀第一回合只动一次），
  并把「扫描里任何一只手都不许 game_over」写成断言；要么由协调者明确拍板「这是有意的陷阱」，在战前加一句提示，登记进 tech-debt。
  施工图第 7 节要求「对自己的取舍稳定」，现在只满足了一半。

**M-2　第 4 章攻防战的「药关」只有上界，没有下界**
- 位置：`tests/Ch04SliceTests.cpp:1665`（只断言 ≥ 8 瓶必赢）；「2 瓶全负」那一列只打印不判。
- 复现：在拷贝里把 `yelangbang_mazei / yelangbang_toumu / jinguang_shangren / jia_tianlong` 的攻击都减半，跑全量：
  只有 `Ch04Walkthrough` 的两条存档漂移红（附录 B · M8）。
- 建议：按设计原文补一条下界，比如「门槛六成、身上 2 瓶，三张编成都输」，或者「围歼那一张最少要几瓶 ≥ N」，N 取设计 3.3 那张表。
  不要从扫描的输出里反推这个数。

**M-3　「升阶」「开门」音效生成了，但没接进游戏**
- 位置：`assets/sfx/realm_up.ogg`、`door_open.ogg`、`item_get.ogg`，全仓零引用；`scripts/common/api.lua:83` 的 `sfx()` 也没有调用方。
- 建议：换图时（`WorldScene::tryStep` 走传送点那一支）播 `door_open`；`realm.advance` 那条命令里播 `realm_up`；Give 效果和获得物品时播 `item_get`。

### LOW

1. **择敌行把破绽信息截掉了**：`BattleScene.cpp:131-134` 拼出的串超出 330 宽的菜单（截图 41、64）。建议把「破绽」挪到第二行，或者菜单开宽一些。
2. **存盘提示是给开发者看的**：`WorldScene.cpp:239` 写着「重开时加 --load 读回」。标题画面已经有「继续旅程」，这句该改掉，并收进 `data/text`（现在它在禁词闸门之外）。
3. **战斗里 Tab 不能返回**（施工图 1.5）：`BattleScene.cpp:869` 只认 Cancel。
4. **主题色没有收全**：`BattleHud.cpp:241,246,253,297,337`、`BattleScene.cpp:50-53` 还写着语义色（破势红、毒紫、蓄势暗红）。
5. **战斗站位**：前冲的人从同伴身上穿过去（截图 42、78）；曲魂在战斗尺寸下像墓碑（有意为之，但读起来确实像）。
6. **面板盖住主角**：路径和对话面板固定在底部中间（`PathActionScene.cpp:45-46`），镜头贴着地图下边时会盖住韩立和 NPC（截图 38）。
7. **文档陈旧**：`octopath-overhaul.md` 第 331-335 行「第二批（仍待派）」列的全是已经做完的活；第 8 节的日期写 09-27，比机器日期早了一天；
   `octopath-battle.md` 6.6 仍写「battles.json 待接」。
8. **测试会删掉真仓库里的 `saves/quick.sav`**：`QuickSaveTests.cpp:51-56`。这是老问题，但标题画面的「继续旅程」上线后变严重了。
   建议 QuickSave 这一组用临时资产根，或者先备份再还原。
9. **叫法不一**：主菜单是「法门 / 气力」，战斗里是「法术 / 耗法力 / 法力不足」（第六节）。
10. **地图观感**：演武场、洛日峰放大后太空；客栈和墨府院子的砖纹读起来像墙；世界图的景深带太弱，移轴感不足。
    建议 dof 提到 0.5 以上，再给大空地加一些点缀物件。
11. **已登记的偏差，列在这里备查**：架势数值下调；章节卡没有做 Lua 接口；打探的「隐藏物件 / 折扣」没有内容；遭遇只放了三张图。

---

## 九、未覆盖面（我没量到的，以及风险评估）

| 面 | 为什么没量 | 风险 |
| --- | --- | --- |
| 硬件渲染器的帧率与画面 | 全程用的是 `SDL_VIDEODRIVER=dummy`（软件渲染）。PostFx 每帧有多张渲染目标、三级降采样、Mod/Mul 混合，没在 D3D/Vulkan 上跑过 | **中**：集显上的帧率，以及各后端 Mod/Mul 混合是否一致，都不知道。建议真机跑一次 60 帧计时，拍一张夜图对照 |
| 音频 | 没有用耳朵听；BGM 循环接缝、响度（自述 −16 LUFS）、音效混音都没核 | 低到中 |
| 真实按键手感 | 没有注入输入的口子。滑步 0.12 s、镜头、连按节奏、Tab/Esc/E 只读了代码、看了单测 | 中：「好不好玩」这一块没人亲手摸过 |
| 标题 → 新的旅程这条链 | 没有自动化测试，我只读了代码 | 低到中 |
| 真人从第 1 章走到第 5 章 | 只有无头通关测试 | 中：比如遭遇 18–40 步一场、每天 3 场的疏密手感，没人试过 |
| 截图覆盖 | 我拍了 13/24 张图、约 20 场战斗。夜战、首领胜利卡因为截图口的 AI 不会用剑符，拍不到 | 低 |
| 窗口缩放、全屏、HiDPI，以及换一台机器的字体 | 没测 | 低 |

---

## 附录 A　跑过的命令与输出摘要

```
& "H:\Work\Kys\fanren\build_logged.bat" finalrev        → EXITCODE=0（03:14:19 → 03:20:39 +08:00）
  VALIDATE_OK / MAPGEN_IN_SYNC：24 / SELFTEST_OK（177，未抓 0）/ ARTGEN_IN_SYNC：257 个文件（18.1s）
  [176/177] Linking fanren_tests.exe；warning 行 0
  100% tests passed, 0 tests failed out of 1312（133.27 s）
build-finalrev\fanren_tests.exe --gtest_brief=1          → 1312 tests / 215 suites，PASSED 1312，SKIPPED 0
shoot.ps1 / shoot2.ps1（49 + 12 次截图，全部 rc=0 SHOT_OK）
fanren.exe --headless --load tests\fixtures\ch05-end-first.sav   ×2 → HEADLESS_OK map=ch05_mofu pos=32,10（两次相同）
b04_gongfang_weijian:mid ×2 → 逐像素相同（sha256 前缀 d596cfb286e3）
python tools/artgen/artgen.py --check（拷贝里改 1 个像素）→ rc=1「1 个文件与生成器不一致 … maps/ch01_hanjiacun/below.png」
python tools/validate.py（拷贝里注入「灵石」「修仙」）→ rc=0（validate 不查禁词）
novel.py find 酒楼/外门(1–3)、十六(104–110)、霸王甲/剑符(100–126) → 与 note 所述一致
```

## 附录 B　变异注入（拷贝：scratchpad/finalrev/mut，用同一个二进制跑全量；基线 1312/1312）

| 变异 | 红几条 | 代表 |
| --- | --- | --- |
| M1 欧阳飞天删掉 `killable_by` | 4 | `Ch05BattleData.FieldEscapeDefeatAndHeroAbsenceAreWhatTheTableSays`（`:253`）、`Ch05EveryPrep…FirstSide_Nothing` |
| M2 欧阳飞天破绽加「火」 | 1 | 同上（`:252`） |
| M3 尸傀攻击 13→20 | 25 | `NoRoundCanTakeMoreThanThreeTenthsOfTheTargetsLife`、`EveryRoleCarriesTheNumbersOfSectionEightThree`、通关各条 |
| M4 贾天龙攻击 19→26 | 1 | `Ch04Walkthrough.WalksTheWholeChapter…`（存档漂移） |
| M5 僵兽防御 10→2 | 3 | `TheBeastThatCannotBeCutDownGoesDownToPoison`、第 3 章通关两条 |
| M8 攻防四类敌人攻击减半 | 2 | 只有存档漂移两条（M-2） |
| 负向：`below.png` 改 1 像素 | 门禁 rc=1 | `ARTGEN` |
| 负向：`ch03_path` 注入「灵石」、`ch01_path` 注入「修仙」 | 2 | `Lexicon.NoChapterSaysAWordItHasNotEarnedYet`、`PathActionData.ChaptersOneAndTwoNeverSpeakOfImmortals` |
| 根因证明：`battles.json` 复制到 `assets/data/visual/` | — | 攻防战背景由晴天变成月夜（`g_backdrop_proof.png`） |

每一处改完都还原了；拷贝的 `data/`、`assets/` 与工作树 `diff -rq` 无差异之后，整个拷贝已删除。

## 附录 C　截图清单（finalrev/）

标题与卡：`01_title_boot` `03_card_ch1` `04_card_ch1_end` `05_card_ch5_end`
地图：`10_map_hanjiacun`（昼） `11_map_qingniuzhen` `12_map_jusuo`（室内） `13_map_yabi`（黄昏） `14_map_andao`（洞） `15_map_guwai`（黄昏、星级）
`16_map_yanwuchang` `17_map_luorifeng` `18_map_nancheng` `19_map_mofu`（夜） `20_map_dubashanzhuang`（夜、星级） `21_map_kezhan` `22_map_dukou`
界面：`30`–`34`（主菜单） `35_talk_sanshu` `36_path_sanshu` `37_path_bubble` `38_path_purchase` `39_board`
战斗：`41_b03_elang_target` `42_b03_elang_mid` `43_b03_elang_vic` `45/46_b03_shihai*` `51_b04_siege_mid`（破势） `54_b04_intro` `66_be05_dukou_intro`（碎屏）
`70_b04_siege_night_mid`（HIGH-1） `74_b05_ouyang_charge` `77_b05_mofu_shigui_mid` `78_b03_andao_mid` `79_b05_xiaoxiang_vic` `67_be03_yezhu_vic`
对照与证明：`g_backdrop_proof.png`（`bd_before_fix` / `bd_after_fix`） `missing_art.png` `det_1/2.png` `z_dof_qingniu.png`；拼图 `g_*.png`
（52、53、63 是截图口在错存档下打到战败的样子，保留作 L 条旁证。）

## 附录 D　树的完整性

- 开审时 `rollup.py`：`FILES 2285 ROLLUP bfb7bf89fccdccf34e9ccf9aa49a130204cb0eb41ac7e48114426c7ee90b5749`，
  中途（变异做完、拷贝删除之后）重算一次，逐文件相同。收尾时重算（不含本文件）：`FILES 2286`，
  和开审清单逐行 diff，唯一的差别是新增的 `docs/ch05-reverify.md`（R-B 的产物）。
- 我在树里的足迹只有三样：`build-finalrev/`、`build-finalrev.log`，以及本文件。
  `saves/` 目录的 mtime 被 `QuickSaveTests` 碰过（写入再删除，里面的文件没变，见 L-8）。
- 上面那份 `docs/ch05-reverify.md` 是另一名校对（R-B）约定要交的，不是我写的，也不在本文件的核对范围里。
