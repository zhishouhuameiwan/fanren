# 第 12 章整改路·文案与美术（2026-10-13，构建槽 ch12fix）

照 `fanren/docs/ch12-review.md`（初审 3.0）逐条整改。审查报告未改。原著只用 `tools/novel/novel.py read/find` 读（每个改动 key 改前都读了对应原文，行号见审查报告）；下面的「改前 / 改后」都是概述，不引原文。

改动范围：`data/text/ch12_{yizhi,guiwu,dadian,binghuo,neidian,battle,items,objectives,path}.json`（82 个 key 改写、1 个新增，554 → 555 条）；`scripts/ch12/guiwang.lua`（加一句 talk）、`scripts/ch12/yihai.lua`（一句改旁白）；`tools/artgen/{mapmat,mapwater,maplights}.py`；`data/visual/maps.json` 的 `ch12_binghuo` 条目；`assets/art/maps/ch12_binghuo/{below.png,meta.json}`。旗标、门闸、advance_days、teleport、battle 调用一处未动。

## C-1 HIGH（6 个 key）

| key | 改前 | 改后 |
| --- | --- | --- |
| `duoshe.want` | 挑他的身子、自称、叫另两人滚 | 怪影只说自己在柱里关了多年、身子早烂了、今天有人送上门；无自称、无赶人 |
| `duoshe.burn` / `duoshe.scroll_gone`（为承接上条，补事实 ③「放另两人走」） | 画轴烧尽 / 散尽 | 同上，再加一句旁白：影子让开台阶，眼睛不离他 |
| `tupo.legend` | 上回开殿→一队在雾里→撞鬼王→只回来一个 | 葛笠先说雾里有鬼王，再说一个老相识是那一队里唯一活着出来的、从此不再进殿 |
| `path.ziling_guiwang_dating.text` | 同上四拍 ＋ 鬼物畏雷 | 删掉那一队的事：紫灵先说自己帮不上忙、只转述听来的，再讲鬼物见雷就躲、雷留到要紧时（**与 16.3 K8「效果」一栏的 A47 不再重合，A47 由 `tupo.legend` 承担**）。第一版只有 34 个汉字，`PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters` 红（下限 40），已加到 53 字 |
| `yuanyao.deny` | 反问「元」→认错人→姓阮 | 停顿后只报一个「阮」字，让他称阮道友 |
| `baishi.accept` | 好→记名→回岛行礼→给环→门下人手一只 | 只留记名、给环，末句换成「日后肯用心，少不了好处」 |
| `baishi.xg_warn` | 当年我赐的阴阳环→改了名→被阳环克 | 只说是阴阳环拆开的一半，另一半在极阴身上、正克着它 |

## C-2 HIGH（31 个 key，逐段）

| 段 | key | 改前 → 改后（概述） |
| --- | --- | --- |
| n01 | `dengdao.condition` | 「丑话」起句、阵归我、我先挑 → 直接说只要一件、得由她先挑 |
| n02 | `pozhen.split` | 站位、扑灭漏网的流程 → 只把水属法器一人一件分下去 |
| n04 | `qixialian.watch` | 看骸骨的手 → 盯着花茎：池水不动它却在颤 |
| n04 | `qixialian.jinqing` | 「圈套」 → 金青只说画轴撑不了多久（换了意思） |
| n05 | `duoshe.want`、`duoshe.boast` | 两处「本座」全删（want 见 C-1；boast 改为「往后归我使唤」） |
| n06 | `xiaocha.age/name/judge/jade/truth/verdict/go` | 次序全换（脚本 talk 次序不动，内容按槽重排）：先要玉简→报名号→他趁对方看简时估修为→只问「跟谁学的」→答→认不是一脉，**六百年与散婴挪到 verdict 末尾**；truth 只说得自一个死人、照着自练，来历不提；go 删「徒弟」一层，只说先走一步。注意 key 名与内容不再一一对应（`age` 槽里是要玉简） |
| n07 | `path.wenqiang_kuixing_dating.text` | 师傅坐化→熬几十年→筑基→离岛→进妙音门 → 先说如今在妙音门跑外事，筑基一句带过，师傅那时已不在 |
| n10 | `qunxiang.enter`、`qunxiang.guess`（入殿次序随之）、`qunxiang.days`、`qunxiang.man` | 外貌全删（苍白、老道、老农、紫袍、黄须、震地）；入殿次序改为：前头那人看不透→柱上有人低声叫出「极阴祖师」、众人挪眼→他才认出后头是乌丑→玄骨杀意；三人入殿只写满厅收声、有人改了坐姿（「看不透」留给 enter 与 wan 两处）；蛮胡子由满厅变色的旁人念出名号 |
| n13 | `guiwang.lost` | 被哭声带着起舞 → 只写脚步慢、眼神发直、脚下发虚 |
| n13 | `guiwang.soul` | 三色珠→一剑劈开→三缕黑气→烧尽 → 主魂想借雾遁走，被火追上烧尽 |
| n14 | `yuanyao.garden` | 「七八座」 → 「几座」 |
| n15 | `xiaojie.rule`、`xiaojie.voice` | 冰火道→采药→回程 → 「冰火道」挪进 voice 由玄骨点出；rule 改为：限一整天、过时永困→一个月后回程阵→这一天采什么各凭眼力 |
| n16 | `lingshen.see` | 「凭你的神识看不见」 → 玄骨只指方位（山腹东侧、离地几丈） |
| n17 | `fanlian.lvying` | 电弧→晶珠→捏粉 → 只写绿影被金色电弧击碎 |
| n19 | `xiagu.lend2` | 取鼎之前别死 → 只是先借、出来再还（为 n23 转赠铺一笔） |
| n19 | `item.desc.story_huanglin_jia` | 后半「说白了是保他活到取鼎」 → 删 |
| n20 | `rongyan.luo1` | 「法力去了不少」 → 删 |
| n21 | `tihun.call` | 唤道友→自报元瑶→求救→重谢 → 只喊一声「韩道友——这边」 |
| n21 | `magic.desc.magic_qinghuo_lei` | 魔道秘制、炼一枚极费工夫 → 只写战斗里的样子（绿芒落地炸开青焰、贴地卷） |
| n25 | `huanjing.girl` | 低头→抬头→怯唤 → 旁白：白影唤他四哥、自称家里的小妹 |
| n25 | `huanjing.strike` | 化一缕黑烟 → 像撕破的纸从中裂开、飘两飘没了 |
| n25 | `huanjing.after` | 不看烟去向（反写） → 改成婆罗珠的热意退下去 |
| n30 | `butian.ice` | 乌丑被冻、极阴救 → 删后半 |
| n32 | `canhun.mark` | 虫爬满全身→某处骚动 → 只写虫群找出暗记、他抹去 |
| Z1 | `yihai.take`、`item.desc.story_bihuo_baoyi` | 收骨 / 掩埋尸骨的「酬劳」 → 「衣裳我带走，坑我替你挖，两清」；物品描述改为「替主人挖了坑，衣裳便归了他」 |

另：`baishi.ring` 原写的指环颜色是 13.1 n18 点名的质感项「指环的样子」（审查未列），顺手删成「那只指环到了他手里」。若协调者认为越界，可回退这一条。

## C-3 MEDIUM（只动文案）

- `tupo.plan`：他先走一程探路，二位留坡上等他探明再跟上；`tupo.agree`：紫灵答在坡上候着。
- 新增 `ch12.guiwang.catchup`（旁白）：紫灵和葛笠循着他探出的路从后头赶上；`guiwang.lua` 在 `ge_li` 那句之前加一行 `talk("", "ch12.guiwang.catchup")`（战斗之前；无旗标 / 门闸变化）。
- `yihai.ziling` 改旁白（`yihai.lua` 该句 speaker 由 `zi_ling_xutian` 改成 `""`）：葛笠在坡上提过玉真人、离结丹中期一步、身上衣裳鬼雾近不得，多半就是他。**13.1 Z1「在场」一栏的紫灵随之不在场**。K8、K9 仍挂在坡上二人身上，与叙述一致。

## C-4 LOW

- `lingquan.again`：泉边闭目调息 → 「灵乳与根须都已到手，两人之间再没有别的话要说」（与位置无关、不与海面分手矛盾）。NPC、地图未动。

## C-5 LOW

- `lingshen.hold` 末补一句：远远望去，山上钻土的影子不止玄骨一个（给 `fanlian.call` 前因）。
- `fanlian.red`：「旗本就是玄骨的东西」 → 「旗是玄骨留的后手」。

## C-6 LOW

- `battle.guiwu_guiwang.intro`：删鬼音半句；「法力先压着不动」 → 「他抢先迎了上去」。

## C-7 LOW

- `battle.hanlitai_xuangu.intro`、`xuangu.pull`：玄骨一律点名，「他」只指主角；pull 末句改为「鼎飞出洞口，被他隔着残焰收进了袖里」。
- `duoshe.seized`：「先一步」 → 「冷不防，身侧」；末句写明是他留在曲魂身上的分神。

## C-8 LOW

- `quba.paid_none`、`canhun.paid_none`：「推演了半日」 → 「一整日」（脚本拨 1 日，脚本未动）。
- `objective.ch12.n10_qunxiang`：「到柱子底下找个位置」 → 「在柱顶坐着，留意陆续进殿的人」。
- `path.wuchou_canyuan_dating.text`：「往上走不得」 → 「再往上的，不是我们拿得动的」（70 字，仍 ≤ 80）。
- `xuangu.pearl`：「拳头大」删。理由：1.1 硬约束 A111–A114 一行只有「冰焰凝成乾蓝冰珠」，没有大小，不是硬约束允许保留的数字，按审查改。
- `dengdao.greet`（审查说可不改）：删「在此候了多日」，不再与原著相反；没有改写成原著的说法。

## C-9 LOW（18 个选项补到 5–20 字）

`tihun.opt_cross` 直穿这片沙漠；`shidian.opt_east/south/west/north` 推开东 / 南 / 西 / 北边那扇门；`huanjing.opt_go` 上前去扶她起来；`jiumeng.opt_think` 先不应，再想一想；`qunxiang.opt_yield` 起身把柱子让给他、`opt_stay` 坐在柱上不动；`yihai.opt_bury` 衣裳不动，只葬了他；`guiwang.opt_go` 上前替黑袍人解围；`yuanyao.opt_no` 当场回绝她；`xiagu.opt_xuanjing/rongyan` 从玄晶道 / 熔岩路进去；`cengjie.opt_leave` 不去管那堆碎块；`ruyi.opt_skip` 不去管那几条蚕；`guairen.opt_now` 立刻放出虫群；`qixialian.opt_wait` 先不动，再等一等。全章 56 个选项，5–20 字以外 0 个。

## C-10 LOW（只在本轮已经在改的段里顺手压）

- 「倒下了」7 → 3：`guairen.flee`（不会再起来了）、`qixialian.shidie`（拖倒在阶前）、`tihun.yellow_fall`（那人已经不动了）、`xuangu.strike`（倒在了冰面上）。留下的 3 处在 `qixialian.bone`、`duoshe.jin_fall`、`neidian.cut`，未点名的 n26 不重开。
- 旁白以「他」起头 112 → 102 句（按脚本 `talk("", …)` 的 key 切句计，基线与审查同为 112）；「一道」24 → 21。

## C-11 LOW（冰火道熔岩被画成水）

- **运行时只读 meta.json**：`src/io/VisualLoader.cpp` 的 `readMapMeta` 读 `assets/art/maps/<id>/meta.json`（water 列表只喂 `WorldView::drawWaterGlints`，蓝白色 `kGlint` 叠加）；`data/visual/maps.json` 在 `src/` 里只出现在 `FxDemoScene.cpp` 第 25 行注释，不被运行时读。读 maps.json 的只有 artgen、genmaps 与测试 `tests/Ch07TriggerModeTests.cpp`（读文本找第 7 章条目）。
- 开关：`data/visual/maps.json` 的 `ch12_binghuo` 加 `"liquid": "lava"` 与 `liquid_why`；缺省 `water`，写错名字 artgen 当场报错（`mapwater.liquid_of`）。
- `mapmat.lava()`：7 级渐层，冷壳近黑红 → 暗红 → 红 → 橙 → 橙黄 → 亮黄白，全经 `palette.c/mix`。
- `mapwater.paint_lava()`：岸用水岸同一套圆角算法、画成冷凝黑岩壳（`rock("cave")`）；面上横向拉长、被低频噪声扭过的条带，离岸越远越亮；漂着硬壳、壳缝一线亮黄；短横流光；贴岸暗边；岸外两圈烤焦。水面那一支代码与种子原样不动。
- `maplights`：熔岩格不进 `water`（88 → 0，免得运行时打蓝白波光）；原先挂在这些格上的 mist / firefly 发射点（16 个）没了，改放 ember 7 个与 kind `lava` 的光 10 盏（运行时 `glowCells` 对未知 kind 取 1.0 格，光色暗红、闪 0.15）。
- 生成后看过产物图：熔岩夹道两条、沼泽两处泥潭都是暗红底、橙色流纹、黄亮壳缝、黑岩边，读得出是熔岩。
- 顺带看到但**没动**：草地与沼泽地面（overlay 6）仍是绿草，文案写的是「红草」——不在 C-11 范围，交协调者定。
- 产物比对（sha256 逐文件，`assets/art` 全部 678 个文件）：先对一份把 `ch12_guiwu/below.png` 改了一个字节的副本跑，报 `DIFF maps/ch12_guiwu/below.png`、`NOT_SAME (1)`（判据有牙）；整改前后实比：只有 `maps/ch12_binghuo/below.png`、`maps/ch12_binghuo/meta.json` 两个变（`above.png` 未变），其余 676 个 `SAME_BYTES`。

## C-12 LOW

- 按协调者裁定保留 4 条 `.win`、4 条 `.lose`，未动（第 11 章先例）。

## 自检

- 原创性：`lcs_check.py --from 416 --to 499 data/text/ch12_*.json` → `SELF_CHECK_OK probe=12`，`checked=555 over=0`。
- 12.2 禁词与首见：照 `tests/Ch12LexiconTests.cpp`（节点表、24 行「X 之前不含」、定点词、全章 0 处词）与 `tests/LexiconTests.cpp` 表一（首见 > 499 的 21 个词）写了 Python 扫描，0 违例；对一份故意塞进「玄骨」（duoshe）与「银月」的副本跑，三条都报。
- 全章选项 56 个都在 5–20 字；最长条目 70 字（`path.wuchou_canyuan_dating`，R2-4 改后 69 字）；13 条打探都在 40–150 汉字。
- 原文对照：每个改动 key 改前都用 `novel.py read` 读了审查报告点到的那几行，改后逐条对过不同骨。
- 终证：`build_logged.bat ch12fix` → `build-ch12fix.log`（22:17:15），VALIDATE_OK、MAPGEN_IN_SYNC、SELFTEST_OK、ARTGEN_IN_SYNC（678 个文件），`100% tests passed, 0 tests failed out of 1946`，`EXITCODE=0`；`PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters` 转绿。上一轮（21:45，EXITCODE=1）唯一一红就是这条（K8 34 字），已归因并修好。

## 协调者收尾（R2-1…R2-6，第 2 轮复验 PASS 4.0 之后）

R2-7（施工图回写）归协调者，本路没碰 docs/。脚本一行未动。

| 编号 | key / 文件 | 改前 → 改后（概述） |
| --- | --- | --- |
| R2-1 | `xiaocha.age`、`xiaocha.judge` | age 删去「看完了，我再告诉你我是谁」，改成「要东西的是谁，你也该知道」，接下一句 name 报名号；judge 开头补一拍「玉简抛了过去」，再写少年看简的工夫他估修为。talk 次序不变 |
| R2-2 | `huanjing.opt_go` | 「上前去扶她起来」 → 「上前去扶她」（5 字） |
| R2-3 | `qunxiang.days`、`qunxiang.man` | 四处「众人反应」只留 guess（挪开眼）与 wan（极阴闭嘴）。days 只交代时间（五六日后门口才又有动静，先后进来三人）；man 只写动作（进门不停、扫一圈、落到他那根柱上），名号由 `claim` 那句的名牌「蛮胡子」带出 |
| R2-4 | `path.wuchou_canyuan_dating.text` | 「只拿得到第一层的东西」 → 从一图一门说起（残图在塔里只开得动一扇门、开过便不灵），往上几层是元婴们争的、伸手就没人救；「第一层」留给 `cengjie.rule`。62 个汉字（40–150） |
| R2-5 | `rongyan.wall`、`rongyan.grass` | 先用 `novel.py` 读了 ch459 第 21548–21570 行。wall：穿红光→热浪→一切皆红 → 光壁在身后合拢、鞋底隔着皮子发烫、吸进的气带焦味（下一句 `bihuo` 的「挡去一层热浪」仍接得上）；grass：利器比方 → 齐膝高的红草、叶上浮着热气、不敢落脚 |
| R2-6 | `tools/artgen/{mapmat,mapground,mapbake}.py`、`data/visual/maps.json` | 见下 |

R2-6（冰火道红草）：
- 开关：`ch12_binghuo` 条目加 `"grass_tint": "fire"` 与 `grass_tint_why`（与 `liquid` 同一套写法：缺省不做，写错名字当场报错）。
- `mapmat.grass_fire()`：6 级，烤红的草茎到焦黄的草尖，与 `grass()` 同级数、同明度用法。
- `mapground.tint_grass(sc, below, above)`：在 `mapbake.render` 全部画完、出 meta 之前调用，把用了本图草地渐层（`grass(season)`）的像素整体换成 `grass_fire`，明度级不动——地面、路沿草叶、草丛、崖顶散草一起换；没写开关的图直接 return、不登记任何渐层。
- 结果：中段大草地、沼泽周边、最东一块都成了暗红褐底、橙黄草簇（Read 看过 below.png）。`above.png`、`meta.json` 这次没变。
- 顺带看到但**没动**：崖顶外沿的灌木仍是绿色（foliage 渐层，不在草地铺层里），在走不到的崖顶上；要不要一起烤红由协调者定。
- 比对：先对 `ch12_taqian/below.png` 改一字节的副本跑，报 `DIFF`、`NOT_SAME (1)`；本轮前后实比只有 `maps/ch12_binghuo/below.png` 变，其余 677 个 `SAME_BYTES`；与本路开工前比，也只有 binghuo 的 `below.png`、`meta.json` 两个变。

本轮检查：`lcs_check` → `SELF_CHECK_OK probe=12`、`checked=555 over=0`；禁词扫描 0 违例；`validate.py` VALIDATE_OK；`genmaps.py --check` MAPGEN_IN_SYNC；`artgen.py --check` ARTGEN_IN_SYNC（678）；`build-ch12fix/fanren_tests.exe --gtest_filter=Ch12*:PathAction*:Lexicon*:Ch07TriggerMode*`（数据运行时读、未重编）153 个用例全过。没跑全量构建与 selftest（协调者终验）。
