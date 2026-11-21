# 第 7 章第二期「同道录」2a·路 U（界面）施工记录

> 2026-11-21。施工图：`docs/ch07-tongdaolu.md`（第 0–4 节）；接口：`docs/ch07-tongdaolu-k.md`（路 K）。
> 分支 `claude/musing-darwin-bc16f7`（从 main 切出，先快进合入路 K 的 `4d8f7c1` 与复审补丁 `995b02f`），不推送、不合 main。构建目录 `build-u`。

---

## 0. 做了什么（一句话一样）

| 施工要求 | 落在哪 |
| --- | --- |
| 「同道录」页顶替「名册」页（`Page::Roster` 这个值不变，字换成「同道录」） | 新 `game/TongdaoPage.*`（页本身）；`MenuScene` 只托管（预览、进出、截图口入口） |
| 卡片：遇到过的立绘头肩 / 像素小人，没遇到的剪影 +「？？？」+ 提示；三列两行、翻页；「可叙话」角标 | `TongdaoPage::drawCard`、`tongdaoEntries` |
| 详情：档名（按线）、五格档位（封顶写「机缘未到」）、默契进度、已有 / 下一档（从伙伴表、角色表、法术表生成）、喜好（二档起全列）、在册加成（种类 + 数值生成说明、生效没有、名额） | `TongdaoPage::drawInfo`、`tierGains`、`perkText` |
| 相处四个动作：问规则层置灰写理由；地点只在百药园（`ch06_baiyaoyuan`），别处「回百药园后可相处」；页面标「外传」 | `tongdaoActionRows` |
| 赠礼（`rules::giveGiftTo`，二档起列表标合意 / 不喜，送完报默契加多少） | `TongdaoPage::giveSelected`；队伍页原有的赠与没动 |
| 切磋（`Application::startSpar`，打完报输赢、默契、首胜奖品） | `TongdaoPage::activate` |
| 请教、叙话：演路 N 的事件脚本；脚本不在置灰写「（内容未到）」 | `tongdaoXuhuaScript` / `tongdaoQingjiaoScript` / `tongdaoScriptExists` |
| 蒲团「同修」 | 新 `game/TongxiuScene.*`；`Application::openFacility` 的打坐分支一处 |
| 升档提示、入册提示（运行时比较，不立旗，无头不出） | `Application::watchTongdao`（推「见闻」那一条提示的样式） |
| 事件图（0.3 秒淡入淡出，垫在对话框下，表不在 / 图缺各警告一次，无头不读） | 新 `io/CgLoader.*`（读表）、新 `game/EventCg.*`（淡入淡出、载图、画）、`Application::drawScenes` |
| 五种新战斗事件的演法、持续标记；我方择目标菜单顺一遍 | `BattleView::onWard` 等、`BattleView::drawStatusMarks`、`BattleScene::playEvent`、`BattleHud` 状态字 |
| 教学卡四张（同道录、相处、同修、切磋），只在第 7 章、第一次用到时弹 | `data/tutorials/ch07.json` 追加四张；`Application::tongdaoTutorial` |
| 截图口与截图 | `ShotScenes.cpp` 末尾成块追加 `pushTongdaoShot`；`BattleShot` 追加五种 |
| 文案 | 新 `data/text/ui_tongdao.json`（界面、战斗演出、教学卡的字全在这里） |

存档版本没动（10）。没有新加任何「状态」旗标；只新登记了四面教学卡的「看过」旗（`tutorial.seen.tongdao*`，与第一期那十六面同一个口径，见第 8 节）。

## 1. 版面的数（1280×720）

主菜单右栏 `kPagePanel {372, 48, 860, 624}`，标题带 40，正文区 `{372, 88, 860, 584}`。标题带右端一块朱红框的「外传」。

**卡片层**

| 项 | 数 |
| --- | --- |
| 上面一行 | 左：「确认看详情，方向键挑人。」18 号；右：「第 n/m 页」（只有一页时不写） |
| 卡片 | 3 列 × 2 行，每张 260×248，间距 16；整格居中（左右各 24）；首行顶在正文区顶 +36 |
| 卡里 | 上半 252×168 是画（立绘裁头肩：格子左右各裁 15%、从格高 3% 处起按框的比例取；没有立绘画像素小人；都没有写姓）；下面名字 22 号、右端档名 18 号、一根 6 高的默契条、一行简介 16 号（放不下截断加「…」） |
| 剪影 | 同一幅画乘 `{14,16,24,235}`，只剩轮廓；画上居中「？？？」26 号；名字「？？？」；简介位写提示 |
| 角标 | 「可叙话」：卡右上一块小牌子（升了档、该演的那一段脚本在） |
| 底下一行 | 「在册加成　名额 a/b」17 号，卡片下沿 +10 |
| 九位 | 一页 6 张，第 7 位起翻到第 2 页；光标走过页边就翻 |

**详情层**

| 项 | 数 |
| --- | --- |
| 左栏 | 宽 300：立绘整身（`bustRosterSource` / `bustRosterArea`，与队伍页同一种裁法，按栏宽缩）；没有立绘画像素小人（×2）。竖分隔线 |
| 右栏 | 从 x = 684 起、宽 536 |
| 名字行 | 28 号名字 + 20 号档名，正文区顶 +8 |
| 简介 | +46，17 号（退回的「默契 N」不写，下一行已经有） |
| 五格档位 | +74，每格（宽 −4×6）/5 ≈ 103 × 32；够到的填暗金，此刻那一格亮框；封顶以上的格写「机缘未到」15 号 |
| 进度 | +114：「默契 50　离「知己」还差 20」+ 右边一根 8 高的条；最高一档写「默契已满」（满 100）或「已到最高一档」；封顶写「机缘未到」 |
| 四块说明 | +146 起每块 50 高（标签宽 92，正文 18 号、行高 23、至多两行，多的截断加「…」）：已有、下一档、喜好、在册加成 |
| 动作列表 | 五行（`listAreaHeight(5)` = 198），贴正文区底：赠礼、切磋、请教、叙话、带上在册加成；灰掉的行尾写理由 |
| 赠礼层 | 四块说明的位置换成「赠礼：挑一样送给某某」+ 列表（正文区顶 +174 起到底，12 行） |
| 左右键 | 详情里左右换上一位 / 下一位遇到过的人 |

测试 `CardsAndDetailBlocksStayInsideThePageWithoutOverlapping` 钉着：六张卡都在正文区里、两两不重叠；详情的左栏、右栏、动作列表、赠礼列表都在正文区里，动作列表在四块说明之下。最长的名字用截断兜住（卡片名字给档名让位，详情名字留 120 给档名）。

**蒲团的面板**：屏幕正中、宽 560；「打坐 / 同修」两行；选人那一层多一行说明（17 号），至多 6 行，多了滚。

## 2. 四个动作怎么判

一律先判地点（不在百药园：四项都写「回百药园后可相处」），再判是不是人物（灵兽：「灵兽不做这件事」），再问规则层：

| 动作 | 问谁 | 理由（`data/text/ui_tongdao.json`） |
| --- | --- | --- |
| 赠礼 | 今天送过没有（`BondRecord::giftDay`）、身上有没有送得出手的（与队伍页同一道过滤：钱、剧情物不算） | 今天已经送过了 / 身上没有送得出手的东西 |
| 切磋 | `rules::canSpar` | 她不与人切磋（`NoSpar`，规则层先判这一条）/ 到「相熟」才行 / 今天做过了 |
| 请教 | `rules::canQingjiao`，再看脚本在不在 | 到「信任」才行（档名按线取）/ 已经请教过了 /（内容未到） |
| 叙话 | 二档起；`rules::xuhuaPending`；那一档的脚本在不在 | 到「相熟」才行 / 眼下没有要叙的话 /（内容未到） |
| 带上在册加成 | 写了加成没有、`rules::rosterBonusUnlocked`；**哪里都能勾**（不是相处，是排兵） | 没有在册加成 / 还没生效；行尾写「已带上 / 未带上」 |

确认时再问一遍（列表是打开时拼的）；不成立就响错、重拼。

**流程**：赠礼在菜单里办完（说一句「某某收下了，很合心意。默契 +5。」），菜单停在详情。切磋、请教、叙话关掉菜单再演：页面返回 `CloseMenu`，菜单那一帧出栈，压上去的仗 / 事件接着来。
切磋打完的那一句由 `startSpar` 的回调说：「切磋胜了某某。默契 +4。首胜之礼：回气丹 ×2。」（奖品的得物卡片由路 K 的 `announceItem` 照常推）。

**脚本约定**（路 N 照同一条写）：叙话第 N 档 `scripts/ch07/tongdao_<伙伴 id>_t<N>.lua`（N = 2–5），请教 `scripts/ch07/tongdao_<伙伴 id>_qingjiao.lua`；脚本末尾自己调 `tongdao.xuhua_mark` / `tongdao.qingjiao`。
界面判「在不在」看游戏根下 `scripts/` 里有没有这个文件，起脚本走 `Application::startEvent`。测试用测试树里的桩脚本，没建路 N 的文件。

**档位给什么**（`tierGains`）：一档「可同行、赠礼、同修」+ 角色表的一档法术；二档追击（`chase`）、可切磋（有编成）、喜好全知；
三档：`roles[i].magics` 里三档的、被动（`perkText`，三档的数）、可请教；四档：四档的法术、合招（这一段的 `duo`，没有用顶层的）；
五档：五档的法术、被动加强（五档的数大于三档时才写）；二到五档那一档的叙话脚本在就加「叙话一段」。按档的法术与被动只在 `rosterEffectsActive` 时写（第 7 章以外不作数，不说）。
「已有」只列本事（法术、追击、被动、合招），不列「可切磋」这类门槛。

被动 / 在册加成六种的说明（`ui.tongdao.perk.*`）：追击伤害 +N% / 她在场时全队开场多 N 点劲 / 打赢后全队回 N% 气血 / 偷袭得手时全队开场再多 N 点劲 / 你的某属法术打中破绽多削 N 点架势 / 用符的威力 +N%。

## 3. 蒲团

`Application::openFacility` 的打坐分支加一处：**开关开着**（`setCushionChoiceEnabled`）、人在百药园、同道录里有人物，就压 `TongxiuScene`；否则照旧直接压修炼面板。
- 「打坐」→ 换成原来的修炼面板（同一个灵脉效率）；「同修」→ 选人（名字、档名；今天同修过的置灰）→ `Application::tongxiu` → 说一句「与某某同修一日。默契 +4，明天第一场战斗全队多 1 点劲。」。
- **开关缺省关着**，`main.cpp` 打开（正常游玩与截图都开）——与野外遭遇、教学卡同一个套路。理由：无头的 `Ch07AcceptanceTests` 在百药园蒲团上直接找修炼面板（`openCushion` 期待 `CultivationScene`），那时陈巧倩已经结识；开关开着它就挂。这条红线不能碰，所以照现有套路关着。
- 「走上去或面朝它都能用」在世界层（`WorldScene` 的交互），没动；别处的蒲团、同道录里没有人物时与改版前一模一样（测试 `OnlyBaiyaoyuanWithSomeoneInTheRosterAndTheSwitchOnOffersTongxiu`）。

## 4. 升档 / 入册提示

`Application::watchTongdao`（每帧，排在 `watchGuide` 后面）：比前后两份「谁在同道录里、在第几档」，多出来的推进「见闻」那一条提示（`loreNotices`，屏幕上沿正中、几秒淡掉、不挡路、排队）：
「默契渐深：陈巧倩 · 信任」「已记入同道录：南宫婉」。不立旗标。只在第 7 章（`rosterEffectsActive`）报；新开局、读档、回首页（`resetGuideWatch`）只记不报；**无头时整个跳过**（不记基线、不推、不改状态）。
「可叙话」角标不是这里推的：卡片每次从状态算（`xuhuaPending` > 0 且那段脚本在）。

## 5. 事件图

- 表 `data/visual/cg.json`：`{"_doc": "…", "cg": {"<id>": {"file": "<文件名>.png"}}, "files": {"<文件名>.png": "<sha256>"}}`。读取器 `io/CgLoader.*` 严格：缺 `cg`、认不出的键、`file` 不是 `.png` 或带目录、`files` 的值不是 64 位小写十六进制，整份报错（以「cg.json」起头）。`files` 不核对图（门禁的事）。
- 图 `assets/art/cg/<file>`，线性缩放载入，按图的实际尺寸铺满 1280×720。
- `CgStage`：每帧按 `currentCg()` 推，0.3 秒淡入；换图交叉；收起后淡完才清；淡出一半又要回来的从那一刻的透明度接着淡入。
- 画在最底下那一层（走图）之上、对话框之下；最底下就是对话框（没有世界层）时画在它前面。
- 表不存在、读坏了、id 没登记、图载不进来：这一幅不画，各警告一次（`[cg] …`）；无头不读表、不载图、不警告。
- 截图口与测试用 `Application::setCurrentCg`（游戏里只由脚本 `show_cg` / `hide_cg` 改）。

## 6. 战斗：五种事件的演法与持续标记

全用现成的特效贴图与粒子（`assets/art/fx`），没出新图：

| 事件 | 演法 | 持续标记 |
| --- | --- | --- |
| `Ward` 挂上护持 | 钢青的光（Glow + 回春光）落到他身上、几点光往上飘、一盏小灯；头上飘「护持」；日志「某某为某某挂上护持，可挡下一击」 | 脚下一圈压扁的青光（一明一暗）、头顶一颗青菱；我方面板那一行小字「护持」（只有它时用守势那种青色） |
| `WardBlock` 挡下 | 碎玉片崩开（Shards）、闪一下白、不往后仰；飘「挡下」 | 那一层护持散掉 |
| `Bulwark` 剑幕 | 施法者这一边每个站着的人身前一道风旋（Wind）+ 一圈淡光；飘「剑幕」；日志写减几成 | 脚下淡一点的光圈；面板「剑幕」；下一回合开头散掉 |
| `Bind` 定身 | 金光压下、金星悬着慢慢落、身子一僵；飘「定身」 | 三颗金菱绕身子转（前半圈亮后半圈暗）；脚下牌子下面一行「定身」 |
| `BoundSkip` 动弹不得 | 身上一颤、金星散开；飘「动弹不得」 | 定身散掉 |

显示副本 `ShownUnit` 多了 `warded / wardUntil / bound / bulwark`，事件播到哪一条改到哪一条；一手演完对规则层同步一次（`syncShown`）。护持过了它那一回合没用上就散（规则层不发事件，`RoundStart` 时照回合号收）。

**我方择目标的菜单**（路 K 的最小适配，看了一遍，补了三样）：标题回血 / 护持时叫「择人」不叫「择敌」；说明区写指着的那位的气血与身上的护持 / 剑幕；
光标一进来先指着气血比例最低、还能选的那一位。没改的：已挂着护持的人照路 K 的规矩灰着写理由；剑幕选中就出。

## 7. 截图

真程序（`build-u/fanren.exe`、仓库数据、`saves/ch07-jindi.sav`）拍的在 `build-u/shots/`，重拍：在 `build-u` 里 `bash shots-u.sh`。
立绘是真的（陈巧倩 `chen_shimei`、南宫婉 `nangong_wan_ch07`）；数值、法术、脚本、事件图都还是路 N 没填之前的样子。

| 图 | 内容 |
| --- | --- |
| `u01_cards_two_met.png` | 卡片层：两位都遇到过（立绘头肩、档名、默契条） |
| `u02_cards_one_met_rest_silhouette.png` | 只遇到南宫婉：陈巧倩是剪影 +「？？？」+「尚未结识。」 |
| `u03_detail_chen_baiyaoyuan.png` | 陈巧倩详情（三档「信任」，百药园） |
| `u04_detail_nangong_capped.png` | 南宫婉详情（四档，五档那一格「机缘未到」） |
| `u05_detail_chen_tier1.png` | 陈巧倩一档：喜好写「再熟些才知道她喜欢什么」 |
| `u06_detail_chen_away.png` | 不在百药园：四项灰着写「回百药园后可相处」 |
| `u07_gift_chen_tier2.png` | 赠礼列表（二档：合意 / 不喜标在行尾） |
| `u08_cushion_choose.png`、`u09_cushion_pick.png` | 蒲团的「打坐 / 同修」、选人 |
| `u10_tierup_notice.png` | 升档提示「默契渐深：陈巧倩 · 信任」 |
| `u11_joined_notice.png` | 入册提示「已记入同道录：南宫婉」 |

测试夹具拍的（`FANREN_TONGDAO_SHOTS=<目录>` 时像素用例另存一份；这一批在 `build-u/shots-test/`）：

| 图 | 内容 |
| --- | --- |
| `u_cards.png` | 九位、两页：七位剪影（测试图）、遇到的两位 |
| `u_detail_probe_a.png`、`u_gift_probe_a.png`、`u_cushion_pick.png` | 详情（按档的法术、被动、合招都是从表生成的）、赠礼、同修选人 |
| `u_cg_half.png`、`u_cg_under_dialogue.png` | 事件图淡入一半、铺满后垫在对话框下（测试用纯色小图） |
| `u_battle_ward.png`、`u_battle_wardblock.png`、`u_battle_bulwark.png`、`u_battle_bind.png`、`u_battle_boundskip.png` | 五种新战斗事件各一张（测试用的三门辅助法术与一位会它们的帮手） |

**拍不了的两样**：事件图垫在对话框下、五种战斗事件——仓库里还没有路 N 的 `cg.json`、图与会这几门法术的伙伴数据，所以只有测试夹具拍的。路 N 的数据进来之后截图口照样能拍：
`--scene tongdao:cg=<事件图 id>:<文案 key>[:<角色 id>[:<表情>]]`；`--scene battle:<编成 id>:ward|wardblock|bulwark|bind|boundskip`（我方有人会那一门时，轮到他就替他放，推到那一条事件定格）。
「全是剪影」那一屏：同道录里一个人都没有时主菜单根本没有这一页（改版前「名册空着时左栏就是那几项」的规矩没动，`CompanionsMenuTests` 钉着），所以拍的是「只遇到一位、其余剪影」。

截图口全表（`ShotScenes.h`）：`tongdao:cards|detail=<id>|gift=<id>|cushion|cushion.pick|tierup=<id>|joined=<id>|cg=…`，记号 `@met.<id>,unmet.<id>,bond.<id>=<n>,flag.<旗>[=<n>],item.<物品>=<n>`。

## 8. 教学卡

`data/tutorials/ch07.json` 末尾追加四张，都 `min_chapter = max_chapter = 7`：

| 卡 | 触发 | 什么时候 |
| --- | --- | --- |
| `tongdao`「同道录」 | `tongdao.open` | 头一回进同道录页 |
| `tongdao_xiangchu`「相处」 | `tongdao.together` | 头一回在百药园看一位的详情 |
| `tongdao_tongxiu`「同修」 | `tongdao.tongxiu` | 头一回看到蒲团的两项 |
| `tongdao_spar`「切磋」 | `battle.spar` | 切磋头一回轮到下令（排在战斗规则那几张前面） |

前三张不能排队等 `flushTutorials`（它只在走图 / 设施面板时弹，菜单合上才轮得到，就不是「第一次用到时」了）：`Application::tongdaoTutorial` 当场压，开关、看过的旗、设置里关了只记不弹，与 `flushTutorials` 同一个口径。切磋那张走现成的战斗里那条路。
「看过」旗 `tutorial.seen.tongdao*` 四面登记进 `data/flags.json`（与第一期那十六面同一个写法）。教学卡缺省关着，验收路线上不会多出它们。

## 9. 改过的旧测试

只动了一处，是一张白名单：

| 测试 | 改了什么 | 为什么 |
| --- | --- | --- |
| `tests/GuideTests.cpp` 的 `knownEvents()`（`GuideTutorials.ChapterSevenHasAtLeastFourteenShortCardsEachWiredToAnEventThatFires` 用） | 名单末尾追加四个事件名：`tongdao.open`、`tongdao.together`、`tongdao.tongxiu`、`battle.spar` | 这条用例要每张教学卡挂的事件都在「引擎会发的事件」名单里；四张新卡的事件是这一路新发的（菜单、蒲团、切磋），名单是手写的，不加就红。断言本身没改 |

同一个文件的 `TheWordsInQuotesAreTheOnesTheChapterSevenScreensShow`（卡里「」括起来的词必须是第 7 章屏幕上的菜单字）一开始也红了：我把卡里的「同修」去了括号，没动测试。
另一条「卡的每行不超过 30 字」也是改卡的字（切磋那张第二行 32 字 → 28 字），没动测试。

其余旧测试的断言一条没改。名册页换成同道录后，`CompanionsMenuTests` 的 `TheRosterPageListsMetCompanionsWithTheirBondTier` 照样过：
`MenuScene::rosterRows` 留着（档名改成按线取，通用线就是旧的 `ui.bond.tier<N>`）；它最后那段「光标停到名册页、确认，菜单还开着、什么也没改」——确认现在是进卡片层，菜单照样开着、状态一个字节没动。
`CompanionsShotTests` 的 `menu:roster` 截图口照样拍得成（拍的是同道录的卡片层）。`UiStyleTests` 的禁词扫描：同道录的字只放进修仙阶段那一套（凡人阶段的菜单没有这一页）。

改了的旧代码（不在我名下、动了的）：`BattleHud.*`（面板状态字、敌人脚下「定身」，`HudFrame` 末尾追加三个字段）、`BattleView.*`（五个 `on*` 与持续标记）、`main.cpp`（打开蒲团开关一行）、`data/flags.json`（登记四面教学卡旗）。都是成块追加。

## 10. 拿不准的、施工图里不合理的地方

1. **蒲团开关缺省关着**（第 3 节）：不这样 `Ch07AcceptanceTests` 必挂。代价：任何不经 `main.cpp` 起的 `Application` 都没有「同修」两项（截图口经 `main.cpp`，开着）。
2. **「机缘未到」写在封顶以上的每一格**：施工图说「最高那一格」。南宫婉封在四档，两种说法是同一格；将来若有封在三档的人，我这里四、五两格都写。
3. **「全是剪影」的一屏不存在**（第 7 节末）。要的话得让同道录页在没人时也出现——那就改了「名册空着时左栏不变」这条旧规矩与它的测试，没做。
4. **事件图里有人开口时立绘照样画**（立绘是对话框的一部分，压在事件图上）。施工图没说事件图时要不要收立绘；陈巧倩五档那幅里她若开口，图上会再叠一张她的立绘。要收的话在 `DialogueScene::enterBust` 里看 `currentCg()` 非空就不交立绘，一行的事，等路 N 的脚本定了再说。
5. **升档提示借「见闻」那一条的样式与队列**：两样都在屏幕上沿正中、排队一条条放。施工图说「用现成的提示 / 卡片样式」，没有另做。
6. **在册加成勾选不限地点**：施工图只说相处四样限百药园；勾选是排兵，我放在哪里都能勾。
7. **剑幕的持续标记**：施工图只要求「被定住的敌人、身上有护持的我方」，剑幕只管一回合，我也给了一圈淡光与面板字，下一回合开头收。
8. **切磋的教学卡在无头测试里没验**：无头的战斗一上来就自动打完、不开菜单，`battle.spar` 发不出来；这一张只靠读代码（与第一期那几张战斗卡同一条路）。
9. **择目标菜单的三处改动没有自动测试**（标题、说明区、光标落点），截图里也没有那一屏（得有会回血 / 护持的伙伴数据）。

## 11. 留给集成的注意事项

- 路 N 的 `data/visual/cg.json` 照第 5 节的形状写；`assets/art/cg/` 的图 2560×1440。脚本 `show_cg` 之后先说话再 `hide_cg`，图会在 0.3 秒里淡入。
- 路 N 的 `tongdao.<id>.hint`（剪影下的一句）、`tongdao.<id>.intro`（卡片上的一行）缺了退回缺省（「尚未结识。」/「默契 N」）。菡云芝进来之后要看一眼她的卡（立绘表里有 `han_yunzhi`）。
- 叙话 / 请教脚本的文件名见第 2 节；脚本在 `scripts/ch07/` 下才认。
- 菡云芝的 `met_flag` 定了之后，入册提示自动出（不用另接）。
- 2b 的灵兽（`kind: beast`）：卡片与详情都能画，四个相处动作写「灵兽不做这件事」；喂养、随从的入口 2b 再接（赠礼那一行可以改成喂养）。
- 一页 6 张，2b 九位时两页；更多也只是多翻几页。
- 共享文件里我加的：`Application.h` 两块（公开那块在 `currentCg()` 后面，私有那块在类末尾）、`Application.cpp` 末尾一块 + `init` / `tick` / `drawScenes` / `openFacility` / `resetGuideWatch` 里各一两行钩子；`scripts/common/api.lua` 没动。
- 测试树要的四样（桩脚本、测试立绘、测试事件图、三门辅助法术）都在 `tests/TongdaoUiTests.cpp` 里自己写进临时目录，不依赖路 N。

## 12. 测试

新文件 `tests/TongdaoUiTests.cpp`，26 条：
- 页面三条（每一位都在、剪影与提示与缺省、卡片层进出与翻页；档名按线与封顶；档位给什么从表生成、第 7 章以外不写、加成说明）；
- 四个动作四条（别处全灰；百药园逐项问规则层、两种「（内容未到）」；赠礼走真菜单报加了多少、一档不露喜好；切磋走真菜单报输赢与奖品；叙话 / 请教起脚本、脚本自己记账；勾选只写自己那面旗）；
- 蒲团两条（只在百药园、有人物、开关开着；打坐回原面板、同修过一天）；教学卡一条（当场弹、第 7 章以外不弹）；
- 提示两条（无头不记不推不立旗；有窗口时升档 / 入册各推一条、不立旗、第 7 章以外不报）；
- 事件图五条（表的形状、淡入淡出与交叉、无头不读表、软件渲染比像素：铺满 / 淡入一半 / 对话框压在上面 / 收起淡完、缺图只警告一次）；
- 像素与版面三条（剪影比像素、卡片与详情各块不出框不重叠、详情左栏是立绘）；截图口一条；战斗两条（护持的标记；五种事件的截图口各推到那一刻，挡下 / 轮过之后标记散掉）。

全量 ctest（`build-u`，`-j 4`）：2526 条（路 K 交来时 2500 + 这一路 26）。第一遍 4 条红：上面两条 `GuideTutorials`（改了，见第 9 节），
加 `SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`（十帧要在 120 毫秒内，`-j 4` 压着时超了）与 `KeyRepeat.OnTheKeyboardOnlyTheFourArrowsRepeat`（按住计时）——
这两条是引擎的计时用例、与这一路无关，单跑各 5 遍全过。改完第二遍：**2526 条全过（`CTEST_EXIT=0`）**。门禁没跑（用户定的规矩）。

---

# 复审补丁（2026-11-21，主会话复审路 U 之后的三处）

第 10 节九条的裁定：1、2、3、5、6、7 认可；4 见下面第 1 处；8、9（切磋教学卡、择目标菜单没有自动测试）留给集成，等路 N 的数据进来用截图口拍、试玩时看。

1. **事件图铺着时不画立绘。** `DialogueScene::enterBust`：`currentCg()` 非空就不交立绘；名牌、对话框、小头像照常（与没有立绘的人一样）。
   测试 `WhileACgIsUpTheSpeakerKeepsHerNameButNotHerBust`：先验没有事件图时她带立绘；铺着事件图时同一句不带、名牌照常、立绘那一块露着的是事件图。截图 `shots-test/u_cg_no_bust.png`。
2. **法术说明写出新效果**（`rules::describeMagic`，`src/core/rules/ItemInfo.cpp`；字在 `data/text/ui_tongdao.json` 的 `ui.tongdao.magic.*`，数取法术表）：
   回血「回一名队友 N 点气血」、护持「替一名队友挡下一击」、剑幕「这一回合我方全员受伤减 N%」、定身「令一名敌人跳过下一次行动；首领与不能逃的仗里无效」、
   `hits ≥ 2`「连出 N 段，每段单独判破绽」。四种辅助效果那一行「不吃蓄劲」照旧留着。战斗里择法术时的说明走的是同一个函数（`magicEffectSummary` → `magicSummary(describeMagic)`），跟着对了。
   测试写在 `tests/TongdaoUiTests.cpp`（`MagicDescriptionsNameTheNewEffectsWithNumbersFromTheTable`），没动 `tests/ItemInfoTests.cpp`。
3. **喜好整段换行**：`TongdaoPage::likesLines`——一行放得下写一行；放不下把「不喜：…」整段挪到第二行；某一段自己还放不下才截断加「…」。
   文案 `ui.tongdao.likes.line` 拆成 `ui.tongdao.likes.like` / `ui.tongdao.likes.dislike`。测试 `TongdaoLikes.TheDislikePartMovesWholeToTheSecondLine`。截图 `u04_detail_nangong_capped.png` 重拍（「不喜：兽材」整段在第二行）。

新加测试 3 条，`tests/TongdaoUiTests.cpp` 共 29 条。改过的截图：`shots/` 全部重拍（只有 `u04` 有变化），`shots-test/` 全部重拍（加 `u_cg_no_bust.png`；`u_cards` 剪影卡不再叠两层「？」、战斗五张的光调暗了，是第一个提交里就改了、这一轮才重拍）。

全量 ctest（`build-u`，`-j 4`，这一轮）：**2529 条全过（`CTEST_EXIT=0`）**。门禁没跑。
