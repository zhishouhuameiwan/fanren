# 第 3 章校对报告

> 独立校对（不参与创作，除本文件外不修改任何内容文件）· 2026-09-21
> 受检物：`data/text/ch03_main.json`（303 条）、`data/text/ch03.json`（41 条）、
> `scripts/ch03/*.lua`（19）、`maps/ch03_*.tmj`（4）+ `maps/ch02_jusuo.tmj` / `ch02_yaopu.tmj` /
> `ch02_wairentang.tmj` / `ch01_shenshougu.tmj` 的 ch03 挂点、`data/battles/b03_*.json`（3）、
> `data/roles/{yu_zitong,mo_juren,mo_juren_yuanshen,qu_hun}.json`、`data/flags.json` 的 ch03 部分
> 依据：`docs/ch03-design.md` 第 1 节 9 条硬约束 > 第 9 节验收 6 条 >
> `docs/interfaces-p3-ch03.md` 契约（含实现方回填的第 5 节）> 原著全文
> 方法：一切结论由我自己可复跑的脚本或实测得出，判据与实测输出原样贴在下文。
> **不引用交付方任何一个数字作为结论**。

> **快照说明**：复核全程工程树冻结。受检物在复核首尾各取一次 sha256，两次一致（附录 A）。
> 复核期间我为变异注入改过 5 个文件，每次都从我自己的快照还原并 `touch` 推进 mtime；
> 收尾时 `diff -rq` 比对 `src/` `tests/` `maps/` `scripts/` `data/` 全部为空（附录 A）。
> 我自己的构建槽是 `build-ch03rev`，日志 `build-ch03rev.log`，收尾已删除构建目录。

**结论先行：这是三章以来质量最高的一次交付——原创性机器口径与人工口径都过得去（且自报数字第一次全部属实）、
九条硬约束内容全部落地、139,968 条路径穷举零死锁零不可达、五处变异注入五处转红（测试真有牙）、
教学战二的实测数与交付方给的一字不差。但本章最要害的那一处——两种杀的操作感对比——
在关卡层被写反了：该由玩家主动点开的处决挂成了踏入型，该毫无主动性的夺舍反而挂成了交互型，
而钉这一条的测试恰好把错的那一半钉死了（我把它改回脚本自己声明的样子，4 条测试当场转红）。
另有一条「墨居仁死后仍站在密室里说话」。评分 3.5，未达验收线 4，有条件通过。**

---

## 一、三道门禁：我自己跑的一遍

命令与我自己的构建槽：

```
cmd.exe /c "H:\Work\Kys\fanren\build.bat ch03rev > H:\Work\Kys\fanren\build-ch03rev.log 2>&1"
EXIT=0
```

**先核 mtime 再读日志**（本项目因读默认 `build.log` 误诊过一次）：

```
-rw-r--r-- 1 htx-lyh 197609 145051 2026-09-21 10:22:12.477372200 +0000 build-ch03rev.log
Mon, Sep 21, 2026 10:22:19 AM        <- 读日志时刻，距落盘 7 秒，是我这一次的
```

日志里的四条：

```
4:VALIDATE_OK
5:MAPGEN_IN_SYNC：14 张图与生成器一字不差
118:SELFTEST_OK
1524:100% tests passed, 0 tests failed out of 681
warning 命中数：0
```

**三道门禁全过、681/681 全绿、零警告，与声称一致。** 这一条我没有任何异议。

> 说明：这是**还原全部变异之后**重跑的那一次。变异期间的中间构建不作数，
> 每次都在下面各自的小节里单独贴了输出。

---

## 二、硬约束核验（9 条逐条，原著出处我自己查过）

| # | 约束 | 判定 | 证据 |
| --- | --- | --- | --- |
| 1 | 那位大夫真名**墨居仁**，**惊蛟会创始人**，此前未露真面目 | **内容符合，出处有误** | 「墨居仁」确在 ch29（「不愧是我墨居仁看中的人」）。**但「惊蛟会」全书最早出现在 ch104**（`find` 实测 `total_matches=33`，命中章号自 104 起），不在 ch29/ch30。ch30 里他自报的名号是「**鬼手**」（`鬼手` 最早命中章号 30）。设计文档第 1 节把「惊蛟会创始人」的出处写成 ch29/ch30 是错的。详见 MEDIUM-5。 |
| 2 | 以**尸虫丸**控制韩立；后有**解药** | **符合** | `shichong.lua` 节点 4 置 `ch03.shichong_wan`；`jieyao.lua` 节点 9 置 `ch03.jieyao_xuan`。尸虫丸走剧情旗标不走战斗毒，三处代码注释钉着（`core::Item`、`core::battle::Unit`、`qingdu_san.json`），`applyItem` 里 `curesPoison` 只清 `Unit::poison`，确实碰不到旗标。 |
| 3 | 韩立**偷秘籍**、备毒，是**他自己的盘算** | **符合** | `miji.lua` 全程无第二个角色，动机链是节点 1 那句「从看见到扑过来只有三下」。原著 ch35 是厉飞雨把半堂藏书扔给他、两人水潭边打闹——本作改成雨夜偷师父自己的书架，**取材角度完全另起**，且更贴「自己的盘算」。 |
| 4 | 余子童线索经**云翅鸟**浮出 | **符合** | `yunchi.lua`：鸟不睡（三夜灯灭鸟不合眼）+ 手札行间第二只手的落款。原著 ch42 是跟着墨大夫视角、韩立不在场；本作改由韩立自己推出来，是必要的改编（镜头限制），线索载体仍是那只鸟。 |
| 5 | **识海之战发生在梦中**，双方化作光球，体积即实力，靠吞噬取胜 | **符合，逐拍对得上** | 见第三节 1.(c) 的逐拍对照表，ch56 的 11 个节拍一个不少、顺序一致。引擎侧实测：韩立 480 → 吞第一团后 591；第二团 660 → 425，咬下 36%。 |
| 6 | **墨大夫暴毙于元神被吞，不是韩立醒着动手** | **符合** | `chujue.lua` 里 `flag.set("ch03.mo_siwang")` 紧接 `ch03.shenxing.notme`，且在**任何一次 choice 之前**。我自己的变异注入（变异 5）把这一句挪到 choice 之后，`TheTwoKillsComeFromDifferentPlaces` 当场转红——这条判据真有牙。 |
| 7 | 逃走那一个醒后**开口说话**，韩立才主动处决 | **符合** | `ch03.chujue.answer`「然后它开口了，说的是人话」→ `name`/`admit`/choice/`decide`。顺序正确。 |
| 8 | 张铁被炼成无魂躯壳，名**曲魂**，长期随行 | **符合** | `quhun_rudui.lua` 掀帽兜→起名→`party.add("qu_hun")`；`Ch03Walkthrough.TheCompanionSurvivesASaveRoundTrip` 另钉住入队的是 `qu_hun` 而**不是** `qu_hun_huashen`（原著后期身外化身），两条都在 data 里，正因为两条都在，入错才看不出来。 |
| 9 | 「三大铁则」在本章交代 | **符合，三条逐条对得上** | 我自己读了 ch59：①修仙者不可对凡人夺舍，躯体承受不住自行崩溃；②只有法力高的夺法力低的才成，差距越大越安全；③一生只可一次，第二次元神消亡。游戏 `ch03.tiezhe.rule1/2/3` 三条内容一一对应。且 `ch03.tiezhe.realize`「第二条他昨天已经在自己身上验过了」把铁则 2 与节点 10 的反噬接上了，因果正确。 |

**小计：内容 9 条全部符合；#1 的出处标注有误（不是内容错，是施工图的引用错）。**
按验收第 6 条「任何一条不符即打回」，**内容层面无一条不符**。

---

## 三、我自己独立重做的四项

### 1. 原创性

#### (a) 机器口径——我自己跑的，不引用交付方任何数字

基准：`H:/Work/Kys/novel/《凡人修仙传》（校对版全本+番外）作者：忘语.txt`，
按 `tools/novel/chapters.json` 记的 `gb18030` 解码。**交叉验证**：解码后行数 144,887，
与 `chapters.json` 的 `total_lines` 一致。受检：`ch03.json` + `ch03_main.json` **全部 344 条**，非抽样。
方法：剥除一切非字母数字字符后，6 字种子滑窗求最长公共连续片段（对每个种子的**全部**出现位置都做延伸，不是只看第一处）。

```
novel raw chars       : 7613769
decoded lines         : 144887   (chapters.json total_lines = 144887)
novel normalized chars: 6458695
ch28-64 lines 944..2239 normalized chars: 63205
entry count           : 344
game normalized chars : 14456
game raw chars        : 16376

==== BASE A: whole novel ====
longest common substring : 7
>=6 chars entries  : 46
>=8 chars entries  : 0
>=10 chars entries : 0
    7  ch03.npc.mojuren_after_1           他说这话的时候
    7  ch03.npc.mojuren_after_3           是死的人是活的
    7  ch03.tanpai.polite                 是这个意思只是
    7  ch03.shichong.eat1                 什么感觉也没有
    7  ch03.miji.hide1                    不知道有什么用
    7  ch03.tiezhe.search                 的时候他并没有
    7  ch03.ligu.field                    一会儿什么也没
    7  ch03.quhun.nofollow                动韩立站在原地

==== BASE B: ch28-64 ====
longest common substring : 6
>=6 chars entries  : 8
>=8 chars entries  : 0
>=10 chars entries : 0
    6  ch03.npc.guanshi_3                 瞥了韩立一眼
    6  ch03.tanpai.chang                  第四层长春功
    6  ch03.tiezhe.rule2                  只有法力高的
```

**对交付方自报数字的核对：**

- 「对全本最长公共片段 **7 字**」—— **属实**。
- 「对 ch28-64 **6 字**」—— **属实**。
- 「≥10 字 **0 条**」—— **属实**。

**这是三章以来第一次自报数字全部对得上。**（第 2 章连续两轮报错，初审报「最长 7」实测 8、复审报「ch10-27 为 0」实测 7。）
另外结构数字我也逐个重算了，见第五节，也全部属实。这一点值得单独记一笔。

7 字的那几条全是虚词搭配（「他说这话的时候」「什么感觉也没有」），不承载任何内容，机器口径**干净**。

#### (b) 人工口径——上一章正是败在这里，所以四场戏我逐句并排读过

##### 摊牌（对 ch29「冲突起」+ ch30「枭雄末路」）

原著的细节清单与顺序：搭脉不满 → 进屋坐太师椅半坐半躺 → 神秘人戳在椅背后 →
「想看就看，干吗要偷偷摸摸的」→ 脸上黑气生触角张牙舞爪 → 狠厉讥讽 → 韩立退半步抓袖中铁筒 →
「一点小聪明也敢卖弄」→ 幽灵般瞬移点穴 → 韩立强笑恭维 → 捶背轻咳装弱 →
**伸手从袖子里把铁筒搜出来** → 「好！好！好！」→「不愧是我墨居仁看中的人」（自吹时带出名字）
→ ch30：问年龄 → 韩立答六十余 → **墨大夫自报三十七** → 岚州武林／鬼手 → 被亲信下手 →
握拳指甲插进掌心流血 → 得奇书 → 「我活一天相当于普通人活十天」→ 长春功第四层推拿 →
灵根／已找过数百名孩子 → 七玄门王门主 → 下山被仇家认出只能再活一年

本作的细节清单与顺序：两只空茶盏摆得很正、「请坐」→ 坐下时铁筒硌小臂 →
**「你学医四年，我考你一样」** → 六十开外 → **「别看脸，看手，看指甲，看今早搭脉那只手的温度」**
→ **凉手、青指甲、没老茧没裂口** → **「……是耗」（韩立自己推出结论）** → 点头「三十七」→
**咽不下这口气、下巴也压不动，这才发现被点了穴** → 「我叫墨居仁」→ 惊蛟会 → 被身边的人下手／一本书 →
「我活一日，抵旁人十日」→「这和弟子有什么相干」→ 长春功第四层／灵根／几百个孩子 →
回想炼骨崖 → 「方才你进门坐下的时候我就点了」→ **polite（题眼）** → 认不认 → 掌心朝上要铁筒

**判定：真的换了取材角度。** 三处是结构性的改写，不是换词：

1. **框架从「反派变身宣布」换成「大夫问诊」。** 原著靠外观突变（黑气、触角、狞笑、瞬移）
   告诉玩家「他是坏人」；本作一处外观变化也没有，靠一路设问把受害人逼到自己说出病名。
   由受害人亲口推出结论，比由加害人宣布残酷得多。
2. **点穴的发现时机反过来。** 原著是当场察觉（看见手指从胸前拿开），是一次速度展示；
   本作是延迟发现（咽不动、压不动下巴），是一次失控的体验。
3. **铁筒从「被搜」改成「自己交」。** 原著 `伸手从他的袖子里把那只铁筒搜了出来`；
   本作是掌心朝上的手势 + 玩家二选一（交 / 说没有），墨居仁两条都只答一句「也好」。
   不搜身比搜身凉。

原著被**整块丢掉**的有：鬼手名号、岚州武林、握拳流血、七玄门王门主、招徒经过、下山苦战。
这是真的取舍，不是转写。

**保留意见（LOW-4）**：中段那四条（`hui` / `hurt` / `rate` / `chang`）是原著 ch30 那段自述的
同序压缩，`rate`「我活一日，抵旁人十日」与原著「我活一天相当于普通人活十天」几乎同义。
这是全章最接近转写的一段。可辩解——它是情节必需的信息交代——但请知道它在那儿。

##### 偷秘籍（对 ch35）

我自己读了 ch35：那一章是韩立溜出谷和厉飞雨碰头，前半在讲墨大夫为什么放任他自由进出，
后半是运长春功提升耳目触觉。本作的雨夜、没锁的门、走水沟、数架子数两遍、
雨小了一阵整个人贴在架上不敢动——**原著对应处一样也没有**。
机器口径对这一组也干净。**这是四场里改得最彻底的一场。**

##### 识海（对 ch56「光球之战」）

| # | 原著 ch56 | 本作 | 判定 |
| --- | --- | --- | --- |
| 1 | 他是一个**拳头大小**的**绿色**光球 | `dream1` 一团绿的光，**拳头大小** | 同 |
| 2 | 闯进一个**黄色**光球，**拇指那么大**，**小了好几倍** | `dream2` 黄的，小得可笑，**不过是个指头肚** | 同 |
| 3 | 一见就**裂开一张大嘴**去咬 | `dream2` 进来就**直冲着他张开一张嘴** | 同 |
| 4 | 韩立**也变出一张嘴巴**反击 | `won1` 他**也张开了一张嘴** | 同 |
| 5 | **轻易的吞吃掉了**，很轻松就结束 | `won2` 咬下去之后那团黄的就没了 | 同 |
| 6 | 回味**战利品的美味** | `won3` 他只知道**刚才那一口很好** | 同 |
| 7 | 又进来一个绿光球，**大了有一圈有余**，光芒**黯淡虚弱** | `second` 绿的，**比他大一圈**；`flee1` **里头是空的** | 同 |
| 8 | 明显**吃了一惊，停顿了一下，似乎有些犹豫** | `second` **它本来是要退回去的** | 同 |
| 9 | 韩立**哪还肯放过**，**没有考虑实力差别**就直冲上去 | `greedy` **是他先扑上去的** | 同 |
| 10 | **每当被抓住就把被咬的部分脱离开来**，继续逃命 | `flee2` 每追上一回咬住一块，**就把那一块丢下，接着跑** | 同 |
| 11 | 真的让它逃离，**体积小了三分之一** | `flee3` 它到底跑掉了（引擎实测咬下 36%） | 同 |

**判定：这是四场里唯一一处细节的种类、数量、出场顺序与原著一一对应的段落**，
也就是第 2 章被打回时用的那条判据的字面形状。

**但我认为它站得住，理由有两条：**

1. **这 11 个节拍本身就是硬约束 #5**，设计文档第 1.1 节那张表逐条写死了它们。
   一段被硬约束规定了内容的序列，没有「换取材角度」的余地。
2. **语气与落点是反的，而且反得有意义。** 原著韩立「欢欣鼓舞」「快乐无比」，
   全程是他自己；本作加了三句原著没有的：「他想不起来这是哪里，也想不起来自己是谁」、
   「多出来的那一块是暖的」、「久到他开始觉得这样也很好」。原著写的是胜利，
   本作写的是**失去自我**。句子放回原著一眼就看得出不是那本书写的。

**包在它外面的那一层则全是新的**：被提着走过药圃、脸朝天一畦一畦倒着过去、
额头黄纸扫眼皮却眨不了、白灰没干透的新石屋、灯一圈圈摆在地上、
师父的脸一半亮一半暗两半对不上。原著对应处是金针、红光、图案、玉石发亮、
咒语、「你变得好丑啊」——**一样也没留**。`duoshe.blow`（灯灭到只剩五六盏，
是为了另一个怕光的东西）还是一处新埋的因果，到节点 11 `light` 结账。

##### 处决（对 ch57「身醒敌亡」+ ch61）

身醒那一段的节拍顺序跟着 ch57（凉意惊醒 → 眼皮睁不开 → 看见那张脸 → 第一念头先下手 →
发现对方不动没气 → 搭脉确认 → 摸额头符不见了 → 青玉变灰 → 墙角那团绿的 → 认出是它 → 走过去），
**顺序是 1:1 的**。但每一拍的落点都被改了，其中三处是反的：

| 原著 ch57 | 本作 | 判定 |
| --- | --- | --- |
| 「彻底的安下了心……**内心深处一直压迫的巨石终于被丢掉了**」 | `notme`「人是死了，可不是他杀的。他甚至不知道是什么时候死的……**他睡着的时候，事情就已经办完了**」 | **意思完全相反**。原著是松一口气，本作是不安。这一句是全章道德分量的落点 |
| 第一念头「抢先出手，先下手为强」（纯求生本能） | `first` 同一个念头，但加了「**快到他后来想起来会出汗**——十五岁那年他还没有过这种念头」 | 他怕的是自己 |
| 「**单手托起了下巴**，低头沉吟」（冷静盘算） | `end`「他走得很慢，一步一步，**每一步都是他自己迈的**」 | 强调的是主动性，直接服务两种杀的对比 |

处决那一段对 ch61：`ask_why`（「你为什么杀我？为什么？」）与 `answer2`
（「拿爹娘发誓的人，我不跟他共事」← 原著「我从不和以自己的双亲来发毒誓的人合作」）
是同义改写，贴得比较近。但**两处真的换了角度**：

1. **毒誓从「事后讲道理」改成「当场听出复读」。** `oath`「这一回韩立听清了它发誓的口气——
   和石屋里对着师父发的那一回一模一样，连停顿的地方都一样」。原著是韩立砍完了再说那句道理，
   本作是他当场认出这是同一段录音，于是动手。这一改让那句话从格言变成了判断。
2. **跳三尺那一下反过来了。** 原著是他喊「我终于自由了」，被门外的铁奴打断（外部）；
   本作 `jump` 是「喊完他自己吓了一跳，赶紧回头看门外——门外只有日头」（内部）。

另：`opt_giant` 那一支把原著里韩立**忘了问**巨汉来历这个失误（ch61「他竟然犯了一个大错」）
改成了玩家可以选择去问的一件事，是把原著的遗憾还给玩家的一次好改编。

#### 人工口径小结

**四场里三场（摊牌、身醒、处决）真的换了取材角度，且换的方向一致——
把原著的「胜利／解脱」一律改写成「失控／自我惊觉」，这是一以贯之的作者选择，不是逐条躲重复。
识海那一场节拍 1:1，但它本身是硬约束，且语气是反的。**
与第 2 章初审那种「细节的种类、数量、出场顺序一一对应」的转写不是一回事。

**本项判定：通过。** 比第 2 章复审（4/5）还好一些。

---

### 2. 分支可达性——我自己重新枚举，不复用交付方那套自编解释器

#### 方法（以及为什么可信）

交付方用的是自编的 Lua 解释器。我不复用它，**也不自己再写一个**——我把真的 Lua 装起来了：

```
用 vendor/lua-5.4.9/src/lua.c 对着工程构建出的 lua54.lib 编出一个独立解释器
$ luastandalone.exe -v
Lua 5.4.9  Copyright (C) 1994-2026 Lua.org, PUC-Rio
```

于是我的枚举**跑的是真的 `scripts/ch03/*.lua`，用的是真的 `scripts/common/api.lua`**，
只有 host 一侧（命令队列的处理：`talk` / `choice` / `set_flag` / `give_item` / `take_item` /
`battle` / `advance_days` / `party_add` / `game_over`，以及 `__host.*` 那几个同步只读查询）
是我照 `src/script/ScriptHost.cpp` 与 `api.lua` 自己实现的。
地图闸（portal 的 `require_flag`）与触发器闸（`once` + `set_flag`）是我自己从 `maps/*.tmj`
抄出来的一张表，不读工程代码。

这条路的可信处在于：**脚本文本本身没有被我转写过一个字**，
交付方的解释器与我的解释器如果对同一段脚本给出不同结论，那是 host 语义的差异，不是解析的差异。

#### (a) 选择点笛卡尔积

选择点由我自己数（去注释后 `grep -c 'choice\s*{'`）：

```
choice() calls per script:
   beidu.lua 1      chujue.lua 1     jieyao.lua 1    miji.lua 1
   shichong.lua 1   tanpai.lua 2     yingdui.lua 1
  TOTAL choice points: 8
```

8 个选择点，取值 = 各选项 + 取消。另加三场战斗的落点与第 2 章那笔钱的去向：

```
=== 笛卡尔穷举（8 个 choice 点 x 3 场战斗结局 x ch02 钱的去向）===
total runs              : 139968
reached ch03.done       : 69984
game_over (识海败北)    : 69984
blocked-by-map states   : 0

--- 不同末态 ---
  x11664  day=697 money=28 qidu=0 shixin=2 qingdu=1 tugu=1 sword=1 party=1 yaoxia=33
  x11664  day=697 money=28 qidu=0 shixin=2 qingdu=3 tugu=1 sword=1 party=1 yaoxia=33
  x5832   day=697 money=28 qidu=1 shixin=4 qingdu=1 tugu=2 sword=1 party=1 yaoxia=33
  x5832   day=697 money=28 qidu=1 shixin=4 qingdu=3 tugu=2 sword=1 party=1 yaoxia=33
  x11664  day=697 money=5  qidu=0 shixin=2 qingdu=1 tugu=1 sword=0 party=1 yaoxia=33
  x11664  day=697 money=5  qidu=0 shixin=2 qingdu=3 tugu=1 sword=0 party=1 yaoxia=33
  x5832   day=697 money=5  qidu=1 shixin=4 qingdu=1 tugu=2 sword=0 party=1 yaoxia=33
  x5832   day=697 money=5  qidu=1 shixin=4 qingdu=3 tugu=2 sword=0 party=1 yaoxia=33

--- 运行时记号 ---
  TAKE_FAILED:material_lingshix30          x69984
```

三条结论：

1. **8 个末态全部可达，含「有剑」与「没剑」两条。** 这一条我特地重做过一次：
   第一版我把起手钱包写死成 58，于是 `sword=0` 那一支一次也没出现——
   **那正是第 1 章那起「某结局 81 条路径 0 次可达」事故的形状**，只不过这次是我的模型错了。
   把钱包接回 `ch02.qian_quxiang` 之后两条都出来了。`TAKE_FAILED:material_lingshix30 x69984`
   正好是一半，即全部「把钱换成药材」的路线，`beidu.lua` 那一处 `take` 看了返回值，没有钱货两讫。
2. **`blocked-by-map states : 0`** —— 没有任何一条路径会走到「下一个节点所在的图进不去」。
3. **天数恒为 697**，与各脚本 `advance_days` 之和（330+3+1+30+60+45+3+120+99+4+1 = 696，起始第 1 日）逐项对上。
   日历不随选择漂移，`miji` / `jieyao` 两处「验药三天从总数里扣」确实扣住了——
   这让 `ch03.jieyao.day`「还剩一个月」那句断言在所有分支下都成立。

#### (b) 「能不能跳过主线」——每个节点单独验

把**除它自己的前置以外**的所有旗标都置上，看它还挡不挡得住：

```
  elang       missing=ch02.done              -> blocked   first line: ch03.elang.gate
  mogui       missing=ch03.elang_done        -> blocked   first line: ch03.mogui.gate
  tanpai      missing=ch03.mo_gui_gu         -> blocked   first line: ch03.tanpai.gate
  shichong    missing=ch03.tanpai_done       -> blocked   first line: ch03.shichong.gate
  yingdui     missing=ch03.shichong_wan      -> blocked   first line: ch03.yingdui.gate
  miji        missing=ch03.yingdui_xuan      -> blocked   first line: ch03.miji.gate
  beidu       missing=ch03.miji_done         -> blocked   first line: ch03.beidu.gate
  yunchi      missing=ch03.beidu_done        -> blocked   first line: ch03.yunchi.gate
  andao_zhan  missing=ch03.yuzitong_lu       -> blocked   first line: ch03.andao.gate
  jieyao      missing=ch03.andao_zhan        -> blocked   first line: ch03.jieyao.gate
  duoshe      missing=ch03.jieyao_xuan       -> blocked   first line: ch03.duoshe.gate
  chujue      missing=ch03.shihai_done       -> blocked   first line: ch03.shenxing.gate
  tiezhe      missing=ch03.yuzitong_chujue   -> blocked   first line: ch03.tiezhe.gate
  quhun       missing=ch03.tiezhe_zhi        -> blocked   first line: ch03.quhun.gate
  ligu        missing=ch03.quhun_rudui       -> blocked   first line: ch03.done

RESULT: 15/15 nodes refuse to run without their prerequisite, and every refusal says something.
```

**15 个节点全部挡得住，而且没有一条是静默返回**（静默返回＝玩家按了没反应＝一律会被当成 bug）。
闸门构成一条严格全序链，**跳不过去**；又因为 (a) 里 139,968 条路径全部抵达 `ch03.done`（识海败北那一半除外，
那是设计要求的 game over），**也不存在永远满足不了的条件**。

闸门数目我自己数的是：**脚本闸 15 处**（不是交付方说的 8 处，多出来的是好事不是坏事）、
**地图闸 5 道**（`ch02.done` / `ch03.mo_gui_gu` / `ch03.shichong_wan` / `ch03.beidu_done` / `ch03.andao_zhan`，
其中 4 道用 ch03 旗标，与设计第 6 节的「四道」对得上），四道全部配了 `deny_text_key` 且四条文案都存在。

#### (c) 文案可达性——「定义了却永远走不到」的扫描

把 (a)(b) 两轮加上「闸没开」「再走一遍」「NPC 各阶段」三轮的命中 key 取并集，
与 `ch03.json` + `ch03_main.json` 的全部 344 条对差：

- **脚本引用了、文案里没有的 key：0 条。**
- 我的枚举没走到的 30 条，逐条看过，**没有一条是真正的死文案**：
  - 18 条是 `item.desc.*` / `magic.desc.*` / `ch03.map.*.name`——面板与地图名，不走对话层；
  - 4 条 `ch03.block.*`——我逐条回查了 tmj，`andao` / `cangshu` / `mishi` / `guwai`
    **四条全部挂在真实的 portal `deny_text_key` 上**（`guwai` 挂在 `ch01_shenshougu` → `ch03_guwai`）；
  - 1 条 `ch03.battle.andao_shishou.intro`——战斗 json 的 `intro_key`；
  - 7 条是**守卫分支**（`beidu.notugu`、`tanpai.nothing`、`quhun.nofollow`、`chujue.nopoison`、
    `andao.lose_bare` 等），脚本注释里逐条申报过「这是守卫不是分支」，
    且其中 `chujue.nopoison` 与 `andao.lose_bare` 在真机里**是走得到的**（玩家在暗道那一仗里
    把七毒水 / 清毒散用光即可，见下节），只是我的模型不模拟战斗内用物品。

**本项判定：通过。** 这一章的接线是三章里最干净的一次。

---

### 3. 测试有没有牙——我自己注入变异，实测

每次注入后 `touch` 源文件推进 mtime 再 `cmake --build`（源码 mtime 不往前走 ninja 会跳过重编，
看到的红灯会来自上一次的二进制——这一条我每次都做了），跑完从我自己的快照还原。
基线：`BattleShield` 6/6 绿、`Shihai` 12/12 绿、`SaveFile` 9/9 绿、`TwoKills` 1/1 绿。

#### 变异 1：护体罡气**永不失效**（`expireShieldOfCurrentActor` 开头直接 return）

```
4/6 BattleShield.TheShieldStillBlocksForTheRoundItWasRaised ........   Passed
5/6 BattleShield.TheShieldIsGoneByTheTimeTheDefenderActsAgain ......***Failed
6/6 BattleShield.DefendingForeverDoesNotStackIntoInvulnerability ...***Failed
67% tests passed, 2 tests failed out of 6
```

**「失效」那两条转红，「仍然有用」那条正确地保持绿**（罡气不散当然还挡得住）。方向对。

#### 变异 2：护体罡气**失效得太早**（每次轮转把**所有**单位的罡气清零）

```
4/6 BattleShield.TheShieldStillBlocksForTheRoundItWasRaised ........***Failed
    tests\BattleTests.cpp(489): error: Expected equality of these values:
5/6 BattleShield.TheShieldIsGoneByTheTimeTheDefenderActsAgain ......***Failed
    tests\BattleTests.cpp(510): error: Expected: (state.units()[1].shield) > (0), actual: 0 vs 0
6/6 BattleShield.DefendingForeverDoesNotStackIntoInvulnerability ...   Passed
33% tests passed, 4 tests failed out of 6
```

**「仍然有用」这一次转红了。** 两个方向各有一条判据咬得动，**这一对是真的对**，
不是只钉了一个点。

#### 变异 3：识海逃遁阈值改掉（`kFleeBittenDenominator` 3 → 1，敌人永不逃走）

```
The following tests FAILED:
	180 - Ch03Shihai.TheBiggerBallAlwaysGetsAwayNoMatterHowLongYouFight (Failed)
	181 - Ch03Shihai.TheSpoilsMatchTheVolumeActuallyBittenOff (Failed)
	182 - Ch03Shihai.AFledBallIsNoLongerOnTheField (Failed)
	186 - Ch03ShihaiDataTest.PlayingTheRealBattleEndsWithTheEnemyFleeing (Failed)
67% tests passed, 4 tests failed out of 12
```

**「第二场必然逃脱」有牙**，而且是四条一起红（阈值、战果、场上状态、真实战斗四个角度）。

#### 变异 4：把 2→3 的存档迁移**注销掉**

```
[  FAILED  ] SaveFile.RecognisesTheOlderVersionInsteadOfRejectingIt
9/9 SaveFile.RejectsAVersionThatHasNoMigration ....... Passed
89% tests passed, 1 tests failed out of 9
```

**「老存档迁移」有牙**，且负向对照（没登记的版本必须被挡下）仍然绿——两条不是同一条。
顺带记一笔：`kSaveVersion` 现在是 **4** 不是契约 1.2 节写的 3（后来又加了一条 3→4 的境界属性迁移），
`{1,...}` `{2,...}` `{3,...}` 三条都在表里。契约那一句已过时，见 LOW-3。

#### 变异 5：两种杀——把 `flag.set("ch03.mo_siwang")` 挪到玩家被问话**之后**

```
[  FAILED  ] Ch03Walkthrough.TheTwoKillsComeFromDifferentPlaces
0% tests passed, 1 tests failed out of 1
```

**有牙。** 我读过 `twoKillsVerdict` 的实现，它有先验判据（trace 不能为空、整场必须问过玩家），
不会在一场没演过的戏上恒真，这一点写得好。

#### 变异 6（我自己加的一条）：把 `trigger_chujue` 改回它自己的脚本声明的那个 mode

```
 77/104 Ch03Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens ...***Failed
 78/104 Ch03Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd .......***Failed
 79/104 Ch03Walkthrough.TheTwoKillsComeFromDifferentPlaces ...............***Failed
 80/104 Ch03Walkthrough.TheCompanionSurvivesASaveRoundTrip ...............***Failed
96% tests passed, 4 tests failed out of 104
```

**把关卡改成脚本自己说的样子，4 条测试当场转红。** 详见 HIGH-1——这一条不是测试没牙，
是测试把错的那一半钉死了，比没牙更难发现。

**本项判定：通过（5/5 点名项全部转红），但见 HIGH-1。**

---

### 4. 教学战二到底教不教得成——我自己跑的实测

用我自己的构建槽把那条测试单独跑出来（它自己会把实测数打印出来）：

```
$ ctest --test-dir build-ch03rev -R TheBeastThatCannotBeCutDownGoesDownToPoison -V

[ch03 教学二实测] 一刀砍僵兽 5 点（砍笼中兽 14 点），一包蚀心散 3 回合 × 6 点 = 18 点
  甲·只用刀：砍 4 刀进去 20 点（罡气峰值 0），僵兽 30 → 10，韩立 0/60，落点编号 2，第 9 回合止
  乙·毒+补刀：毒扣 29 点、刀补 1 下共 1 点，僵兽 30 → 0（倒下），韩立 18/60，落点编号 1，第 8 回合止
Passed
```

落点编号 2 = `Lost`，1 = `Won`。
**交付方报的「只用刀第 9 回合死、毒+补刀第 8 回合全歼」一字不差，属实。**

另两场我也各跑了一遍：

```
[ch03 教学一实测] 打赢用了 5 回合，韩立余血 52/60，护体罡气 0
[ch03 识海实测] 韩立入梦体积 480 → 吞掉第一团后 591；第二团 660 → 425，咬下 36%
```

识海那一组核对：660 × 2/3 = 440，实测停在 425 < 440，确实是「剩三分之二之前就走了」；
咬下 (660−425)/660 = 35.6% → 36%，落在契约 5.3 声明的 33–45 区间内，
与原著「少了三分之一」同量级。

**这一课确实教得成**：一条「砍不动 → 换毒 → 毒杀不死人 → 最后一下还得是刀」的完整推理，
两次实跑的落点一胜一负，差别就在有没有用那一包毒。**物品与用毒这两样教到了。**

**但「清毒散」这一课教不成，而且比交付方承认的更糟**——见 MEDIUM-1、MEDIUM-2。

---

## 四、用词违约（硬约束 #6 的独立扫描）

对 344 条玩家可见文案 + 19 个脚本（去注释）+ 6 个角色 json 的可见字段 + 4 张 ch03 tmj 全量扫描：

```
===== A. ch03 player-visible text (344 entries) =====
  灵石 : 0 命中
  修仙 x1   magic.desc.magic_huoqiu_shu
  灵根 x1   ch03.tanpai.chang
  法术 x5   magic.desc.{huoqiu,bingjian,feijian,shenshi_chong,haichao}
  法力 x3   ch03.chujue.why1 / ch03.tiezhe.rule2 / ch03.tiezhe.rule3
  元神 x1   ch03.tiezhe.rule3
  夺舍 x1   ch03.chujue.why1
  识海 x1   magic.desc.magic_shenshi_chong
  神识 x2   magic.desc.magic_feijian_shu / magic.desc.magic_shenshi_chong

===== B. scripts/ch03/*.lua, comments stripped =====   (no hits)
===== C. ch03 role json, player-visible fields =====
  元神 x1   mo_juren_yuanshen.name = 「墨居仁元神」
===== D. maps/ch03_*.tmj (raw) =====                   (no hits)
```

**「灵石」玩家可见层 0 处，属实。** 我另外验了它为什么是 0：
货币物品 id 确实叫 `material_lingshi`、`name` 字段确实是「灵石」，但

- 面板与商店走 `game::currencyName(stage)`，`Mortal` 档返回**「碎银」**（`src/game/Wording.h:56`）；
- 商店的卖出列表 `if (entry.itemId == kSpiritStoneItemId) continue;` 把它排除掉（`ShopScene.cpp:98`）；
- 战斗物品菜单只登记 `core::battleUsable` 为真的物品，货币不在其中；
- `src/game/` 下**没有独立的背包面板**（只有 World / Dialogue / Battle / Shop / Field / Cultivation 六个场景）。

所以这个 id 在第 3 章确实浮不到玩家面前。**这一条站得住。**

其余几处逐条判断：

- **`ch03.tanpai.chang` 的「灵根」——站得住。** 原著 ch30 原话是「还要求修炼者必须具有'灵根'体质，
  虽然我不知道什么是'灵根'」。本作写成「还得有一样东西，**书上叫灵根**」——
  说这个词的是反派，而且他明说这是书上的叫法，他自己也只是转述。
  七玄门仍是凡人武馆这一条没有被破：破例的正是墨居仁这个人，那是本章的题目。
- **`tiezhe.rule2/rule3`、`chujue.why1` 的「法力／元神／夺舍」——站得住。**
  硬约束 #9 要求本章交代三大铁则，而 ch59 的三条本来就是讲夺舍的，
  用别的词说不清楚。我对过 ch59 原文，三条内容一一对应。
- **`mo_juren_yuanshen.name`「墨居仁元神」——站得住。** 它是识海那一场的场上单位名，
  而那一场的题目就是元神。
- **7 条 `magic.desc.*` 里的「修仙／法术／神识／识海」——见 LOW-1。**
  这些是第 6–13 章才登场的法术（火球术、冰箭术、飞剑术、风雷翅、海潮诀、护身罡、神识冲）的说明文案，
  却被写进了 **`data/text/ch03.json`** 这个第 3 章的文案文件。
  我查过它们在第 3 章浮不到玩家面前（韩立一条法术也不会，施法列表是空的；
  场上唯一登记的法术是僵兽的尸毒爪，而 `magicsExhaustive` 让玩家放不出来），
  所以**当前不构成违约**，但把后面章节的词摆在本章的文件里是个隐患。

**硬约束 #6 判定：符合。**

---

## 五、结构指标对照设计文档（我自己重算）

| 项 | 设计要求 | 交付自报 | 我实测 | 判定 |
| --- | --- | --- | --- | --- |
| 主线节点 | 12 | 12 | 12（对应 15 个脚本：节点 8 拆 `yunchi`+`andao_zhan`，节点 12 拆 `tiezhe`+`quhun`+`ligu`） | 符合 |
| 分支次数 | ≥6 | 8 | **8**（去注释后数 `choice{`，逐脚本列在第三节 2.(a)） | 符合 |
| 新建地图 | 4 | 4 | 4（`ch03_{mishi,cangshu,andao,guwai}.tmj`） | 符合 |
| 对白字数 | 约 10900 | 15803 | **15803**（全部 `ch03.*` key 跨全部文案文件共 330 条，逐字符计） | **属实**，超目标 45% |
| 每条 30–120 字 | 是 | — | min 30 / max 79 / avg 48.4，**越界 0 条** | 符合 |
| 新建角色 | `yu_zitong`、`mo_juren` | — | 两个都在 `data/roles/` | 符合 |

「对白 15803 字」这个数我一开始算成 16376，差在我多算了住在 `ch03.json` 里但不以 `ch03.` 开头的
`item.desc.*` / `magic.desc.*`。按「全部 `ch03.*` key」这个口径重算，**15803 一字不差**。
**交付方这一章的数字我逐个复算过，没有一个是错的。**

时长那一组我跑了 `tools/audit.py`：全局 A+B 合计 221.1 分钟（覆盖第 1–3 章）。
工具本身不按章拆分，93.2 分钟是按字数占比折算出来的派生值，量级上对得上。
设计文档第 9 节自己写明的两条保留（换算系数未经真人标定、按字数折算忽略了战斗）依然成立，
**三章连续超预算这条工期风险是真的**，与本章质量无关，请主控层单独裁决。

---

## 六、发现的问题

### HIGH（合并前应修）

#### HIGH-1：两种杀的**操作感在关卡层是反的**，而且钉它的测试把错的那一半钉死了

**位置**：`maps/ch02_jusuo.tmj` 的 `trigger_duoshe`（288,160）与 `trigger_chujue`（448,256）；
`scripts/ch03/chujue.lua` 首部注释；`tests/Ch03SliceTests.cpp:43,815,1284-1287`

**实测**：

```
ch02_jusuo.tmj | obj=trigger_duoshe | (288,160)
      mode = 'interact'          <- 夺舍（设计要求：玩家毫无主动性）
      script = 'ch03/duoshe.lua'
ch02_jusuo.tmj | obj=trigger_chujue | (448,256)
      mode = 'enter'             <- 处决（设计要求：玩家自己走过去、自己点开）
      script = 'ch03/chujue.lua'
```

引擎语义（`src/game/WorldScene.cpp:158` 与 `:195`）：
`mode == "enter"` 是**踏上那一格就自动起**；`mode == "interact"` 是**面朝它按确认才起**。

**为什么是问题**：`chujue.lua` 自己的首部把这件事写成了本章的要害，而且**写的是反的**：

> 这一个脚本玩家必须**自己走到墙角、自己点开它**（关卡侧挂的是 interact 触发，不是 touch）

关卡侧挂的是 `enter`。`duoshe.lua` 首部同样写着「mode=touch」，实际是 `interact`。
**两者恰好对调**：该让玩家感到身不由己的那一节，需要他专门走过去按一下确认键；
该让他感到「这一下是我自己按的」的那一节，走路经过就自动开演了。
设计文档第 5 节节点 11 明写「**主动处决**，与上一节的操作感必须不同」，
大纲注解与设计第 0.3 节把这一条列为本章道德分量的全部来源。

**为什么没被发现**：测试把错的行为当成了正确行为。`tests/Ch03SliceTests.cpp:43` 的注释写着
「ch02_jusuo 的 trigger_chujue 同样是踏入型」，:815 用 `enterTrigger` 驱动它，
:1286-1287 更直接断言「踩上去该起脚本」：

```cpp
ASSERT_TRUE(stepOntoWithoutPumping(chujue));
ASSERT_TRUE(app_.scripts().isRunning()) << "踩上去该起脚本";
```

我把 `trigger_chujue` 的 mode 改回脚本自己声明的 `interact`（变异 6），**4 条测试当场转红**：

```
 77/104 Ch03Walkthrough.WalksTheWholeChapterAndEveryGateHoldsThenOpens ...***Failed
 78/104 Ch03Walkthrough.TheOtherSideOfEveryChoiceAlsoReachesTheEnd .......***Failed
 79/104 Ch03Walkthrough.TheTwoKillsComeFromDifferentPlaces ...............***Failed
 80/104 Ch03Walkthrough.TheCompanionSurvivesASaveRoundTrip ...............***Failed
```

**这比「测试没牙」更难办**：没牙的测试改对了照样绿，而这一组是改对了会红。
没有人会主动去改一个全绿的测试，所以这条错会一直留着。

**判据**：设计第 0.3 节与第 5 节节点 11；`chujue.lua` 与 `duoshe.lua` 首部各自写明的 mode；
`WorldScene.cpp:158/195` 的引擎语义。

**建议**：把两个 trigger 的 `mode` 对调（`duoshe` → `enter`，`chujue` → `interact`），
经 `tools/mapgen/genmaps.py` 重新生成以保住 `MAPGEN_IN_SYNC`；同步改 `Ch03SliceTests.cpp`
的 :43 注释、:812/:815 的驱动方式与 :1284-1287 的断言；
**并补一条直接钉住两个 trigger `mode` 属性的测试**——本章唯一的道德设计点，
不该只靠脚本注释和驱动方式间接表达。顺带把两个脚本首部的 `touch` 改成引擎实际认的 `enter`
（`touch` 不是有效取值）。

---

#### HIGH-2：墨居仁死后仍然站在密室里说话

**位置**：`maps/ch03_mishi.tmj` 的 `npc_mo_daifu`（352,288）；`scripts/ch03/mojuren.lua`

**实测**：

```
npc_mo_daifu  type=npc  {'facing': 'down', 'role_id': 'mo_daifu',
                         'script': 'ch03/mojuren.lua', 'wander': False}
```

**没有 `visible_flag`，也没有 `hidden_flag`。**

**为什么是问题**：`ch03.mo_siwang` 在节点 11 置上之后，这个 NPC 照旧在场、照旧占格、照旧答话。
玩家走完节点 11（处决，在 `ch02_jusuo`）之后要去 `ch03_andao` 做节点 12，
而 `ch03_andao` → `ch03_mishi` 这道门**没有任何 `require_flag`**，一步就到。
进去按一下确认键，`mojuren.lua` 走到第三条分支（`ch03.jieyao_xuan > 0` 在节点 9 就已置上），
播的是：

> `ch03.npc.mojuren_again`：他不再说话，低头拨算盘一样地数着什么，数到一半停住，又从头数起。
> 韩立站了一会儿就退出去了。

一个**正在数东西、会停会重来**的活人。而本章的高潮刚刚把他杀掉，
`ch03.ligu.clean` 还写着「师父埋在药圃东头那棵老槐底下，埋得很深」。

对照之下，同一批 NPC 的其它几个**都**做了可见性切换（`npc_li_feiyu` 用 `hidden_flag=ch03.mo_gui_gu`
交棒给 `npc_li_feiyu_ch03` 的 `visible_flag=ch03.mo_gui_gu`，管事同理），
说明关卡侧是懂这套机制的，**唯独漏了这一个**——而漏掉的恰好是全章唯一一个会死的人。

**为什么没被发现**：`tests/Ch03NpcVisibilityTests.cpp` 的 7 条测试全部跑在一张合成夹具图上，
钉的是机制（不在场的不挡路 / 不答话 / 同格两人按章节轮换），
**没有一条碰过 `ch03_mishi` 的 `mo_daifu`**（`grep mo_daifu tests/Ch03NpcVisibilityTests.cpp` → 0 命中）。

**判据**：`ch03.mo_siwang` 的语义（`data/flags.json`：墨居仁已死）；
`ch03.ligu.clean` 明写已下葬；同批其它 NPC 已有的可见性切换范式。

**建议**：给 `npc_mo_daifu` 加 `hidden_flag = "ch03.mo_siwang"`，
并补一条「旗标置上之后这一格上没有人答话」的测试（照 `AnAbsentNpcDoesNotAnswer` 的形状，
但跑在真图上）。若希望玩家看得见尸首，另挂一个 `visible_flag = "ch03.mo_siwang"` 的静物对象。

---

### MEDIUM（应改）

#### MEDIUM-1：清毒散在第 3 章是一件玩家永远用不上、却点得动又会被白白用掉的东西

用户点名要我判断「僵兽的尸毒爪射程 1、AI 永远先挑近战、玩家在那一场中不了毒」
是不是让清毒散变成了玩家永远用不上的东西。**是，而且还多一层。**

**第一层：用不上。** 韩立在节点 7 拿到 3 包清毒散。此后本章只剩两场战斗：
暗道那一仗（玩家中不了毒，交付方自己的测试已如实钉住这一点，
`tests/Ch03TutorialBattleTests.cpp:722-747`）与识海那一场（**一件物品都不登记**）。
所以本章没有任何一个时刻，清毒散能解掉任何东西。设计第 2 节写的
「它的尸毒爪反过来给玩家上毒，清毒散的用处在同一场里教掉」**没有兑现**。

**第二层：点得动，而且点了就没了。** 交付方的测试自己断言它在菜单里是 `enabled` 的：

```cpp
EXPECT_TRUE(rows[static_cast<std::size_t>(row)].enabled)
    << rows[static_cast<std::size_t>(row)].disabledReason;
```

而 `checkItem`（`src/core/battle/BattleAction.cpp:118-131`）对「解毒药用在没中毒的人身上」
**不作任何拦截**；`applyItem`（:337）里 `if (item.curesPoison && target.poison > 0)` 什么也不做，
末尾走 `if (!changed) text += " 并无变化"`；而 `issuePlayerAction`（`src/game/BattleScene.cpp:297-301`）
只要 `result.ok` 就从背包里扣掉一件：

```cpp
core::Result<std::string> result = battle_.apply(action);
if (result.ok && spendsFromBag) {
    static_cast<void>(app.state().removeItem(action.magicId, 1));
}
```

`applyItem` 永远返回成功。**于是玩家在没中毒时点一下清毒散，代价是一件物品 + 一个回合，
收获是一句「并无变化」。** 这与本项目反复确认的那条判据（「玩家掉血却不知道为什么一律会被当成 bug」）
是同一类问题的反面：东西少了却不知道为什么。

**判据**：设计第 2 节教学二「教物品与用毒」、第 3 节「解毒要有明确入口」；
契约 2.3 节「解除：物品（解毒药）」。

**建议**：两件事分开办。
①（玩法）让暗道那一仗真的能给玩家上毒——最省事的是把 `jiang_shou` 的 `magic_shidu_zhua`
射程拉到 2，或给 `decideAi` 一条「目标已中毒则改挑近战、否则优先施法」的口径，
这样设计第 2 节那句话才兑现得了。
②（防呆）在 `checkItem` 里加一条：`curesPoison` 且 `restoreHp/restoreMp` 全为 0 的物品，
目标没中毒时 `deny("他没有中毒")`，让菜单把理由写在条目上——
这与 `buildRootItems` 那段注释自己写的口径（「某件东西为什么用不了，一律写在下一级的条目上」）一致。
两条都要配「故意写坏 → 确实被抓住」的用例。

#### MEDIUM-2：节点 11 与节点 12 的地理对不上，玩家要走出石屋、穿过一道门，才能搜石屋里的尸首

**位置**：`trigger_chujue` 在 `ch02_jusuo`；`trigger_tiezhe`（608,864）与 `trigger_quhun`（320,640）在 **`ch03_andao`**；
而 `tiezhe.lua` / `quhun_rudui.lua` 两个脚本的首部都写着「挂在 **ch03_mishi**」

**为什么是问题**：文案把这三件事写成同一个地点连着发生的：

- `ch03.tiezhe.gate`：「墙角那件事还没了结。没了结之前**他哪儿也不去**，什么也不动，连尸首都不碰。」
- `ch03.tiezhe.search`：「他**回到师父身边**，蹲下去，一寸一寸地摸。」
- `ch03.tiezhe.word`：「这四个字他念了一遍，**屋外头那人**就转过身来。」
- `ch03.quhun.hood`：「他**走出去**，站在那人面前，把帽兜掀了。」

而机制上玩家必须：在 `ch02_jusuo` 处决 → 走到地图边缘 → 过 `ch02_yaopu` → `ch01_shenshougu`
或经 `ch03_mishi` → 进 `ch03_andao`（一条有十一只笼子的暗道）→ 在那里找到师父的尸首。
「他哪儿也不去」这句话与玩家实际要走的路直接打架，
「屋外头那人」在暗道里也没有「屋」。

**判据**：上列四条文案自己断言的空间关系；设计第 5 节节点 11 地图 = 居所、节点 12 = 暗道/谷口
（节点 12 放暗道本身合设计，但**搜尸**这一件应当与身醒/处决同处）。

**建议**：把 `trigger_tiezhe` 与 `trigger_quhun` 移到与 `trigger_chujue` 同一张图
（并按 HIGH-1 一并裁决这三者到底该在 `ch02_jusuo` 还是脚本首部说的 `ch03_mishi`），
只把 `trigger_ligu`（离谷）留在谷口。若坚持放暗道，则 `tiezhe` 那三条文案要改写。

#### MEDIUM-3：`ch03.shihai_yaoxia` 写进去了，但**全作没有任何一处读它**

**位置**：`scripts/ch03/duoshe.lua:75,88`；`data/flags.json:70`

```
$ grep -rn "shihai_yaoxia" scripts/ src/ data/
scripts/ch03/duoshe.lua:75:    flag.set("ch03.shihai_yaoxia", spoils)
scripts/ch03/duoshe.lua:88:    flag.set("ch03.shihai_yaoxia", 0)
src/core/battle/Battle.h:217:    // 脚本要靠它把「咬下了多少」落进旗标 ch03.shihai_yaoxia（原著是三分之一），
src/game/BattleScene.cpp:855:    // 咬下了多少（0-100）。脚本据此写旗标 ch03.shihai_yaoxia，章末余子童的
data/flags.json:70:  "ch03.shihai_yaoxia": "节点 10 第二场咬下了多少，按百分比存。原著是三分之一；
                      章末余子童的状态与台词要对得上它。"
```

**四处都在说它要被用到，没有一处用它。** 契约 3.4 节写的是
「**战果要能被脚本读到**：脚本需要知道咬下了多少……因为章末余子童的状态与台词要对得上」。
从引擎（`CommandResult.value`）到 Lua（`battle()` 第三个返回值）到旗标这一整条链路确实打通了，
也有测试钉着（`TheSpoilsMatchTheVolumeActuallyBittenOff`），**但末端是空的**：
章末余子童的台词一个字也不随这个数变。

不是硬伤——`ch03.shenxing.corner`「缺了一块」与 `ch03.shenxing.know`「连缺的那一块都在原处」
定性上是对的，玩家看不出缺失。但一个「写了从不读」的旗标是典型的「看着做完了其实没做完」，
而且它让 `TheSpoilsMatchTheVolumeActuallyBittenOff` 这条测试守着一个没有下游的数。

**建议**：要么让节点 11 或 12 有一条台词按这个数分档（例如咬下 ≥35% 与 <35% 各一句），
兑现契约 3.4；要么把契约 3.4 那句话改掉并在 `flags.json` 里注明「本章仅记录，留给后续章节」。
**两条都行，但现在这个中间状态请不要留着。**

#### MEDIUM-4：`b03_gu_wai_elang.json` 的 `defeat_is_fatal` 与设计、脚本注释三方打架

**位置**：`data/battles/b03_gu_wai_elang.json`

```json
"can_escape": true,
"defeat_is_fatal": true,
```

设计第 2 节那张表写「可逃可败，**败了只是被打退**」；`elang.lua` 首部写
「b03_gu_wai_elang 可逃可败，败了只是被打退，所以**不得把 won == false 当死局**」，
脚本本身也确实只播 `flee1/flee2` 就往下走。**只有数据写着 `true`。**

当前**无害**，因为引擎根本不消费这个字段——

```
$ grep -rn "defeat_is_fatal\|defeatIsFatal" src/ tools/
src/core/model/Types.h:291:    bool defeatIsFatal = true;
src/io/BattleLoader.cpp:68:    setup.defeatIsFatal = readBool(parsed, "defeat_is_fatal", true);
```

只读进 `setup`，再无第二处引用（`duoshe.lua` 也如实写着「这个字段引擎不自己消费，落地归脚本」）。
但这正是它危险的地方：**哪天有人给这个字段接上落地逻辑，教学第一场就会变成「输了就死」**，
而那是本章之前一场战斗都没有、专门用来教新手的那一场。
对照 `b03_andao_shishou` 写的是 `false`，且 `tests/Ch03TutorialBattleTests.cpp:781`
有 `EXPECT_FALSE(setup->defeatIsFatal)` 钉着——教学一没有这一条。

**建议**：`b03_gu_wai_elang` 改 `"defeat_is_fatal": false`，并照教学二的样子补一条断言。

#### MEDIUM-5：硬约束 #1 的出处标注有误，且本章把原著留到 ch104 的那个名字提前花掉了

**位置**：`docs/ch03-design.md` 第 1 节表格第 1 行；`ch03.tanpai.hui`

设计文档把「墨居仁是**惊蛟会**创始人」的出处标成 ch29/ch30。我自己查了：

```
惊蛟会 : total=33  earliest chapters=[104, 105, 107, 108, 109, 111, 118, 123]
鬼手   : total=13  earliest chapters=[30, 105, 425, 489, 725, 734, 852, 1484]
嘉元城 : total=62  earliest chapters=[100, 101, 103, 104, 105, 106, 107, 116]
```

- **「惊蛟会」全书最早出现在 ch104**（卷二「情报」），出自墨大夫的**遗书**：
  「吾所创帮会惊蛟会……岚州三大霸主之一，**总舵设在嘉元城**」。
- ch30 摊牌时他自报的是**「鬼手」**：「当时岚州有谁不知道我'鬼手'的声威」。

**内容不算错**（他确实是惊蛟会创始人），但两件事值得记：

1. **施工图的出处栏是错的**，而这一章的验收方式就是拿那张表逐条核。一张会被逐条核的表里
   有一条引用错，下一章就没法只看那张表。
2. **本章把 ch104 的揭示提前花掉了。** 原著的设计是：ch30 只给「鬼手」这个江湖名号，
   帮会名、嘉元城、家底要到 ch100-104 才由遗书一次性抖开。
   而本作在摊牌（`ch03.tanpai.hui`）就让他亲口说出「惊蛟会」，
   章末（`ch03.tiezhe.find1`）又把「落款是嘉元城」的家书交到玩家手上当钩子——
   **钩子的谜底在同一章的前半已经说了**。设计文档自己写着这条线通向第 5 章墨府。

**建议**：改设计文档第 1 节的出处为 ch104（或把该行拆成「墨居仁 ch29 / 惊蛟会 ch104」两条）。
文案侧建议把 `ch03.tanpai.hui` 的「叫惊蛟会」换成 ch30 原有的「鬼手」口径
（「岚州道上的人叫我鬼手」），把帮会名留给第 5 章——这样家书那个钩子才有的可揭。
**这一条要主控层裁决，不是编剧能单方面定的。**

#### MEDIUM-6：19 个脚本里 12 个的首部注释把自己的挂点或触发方式写错了

我把每个脚本首部声明的挂点与 `maps/*.tmj` 里的实际对象逐条比对：

| 脚本 | 首部声明 | 实际 | |
| --- | --- | --- | --- |
| `elang.lua` | ch03_guwai, interact | ch03_guwai, **enter** | mode 错 |
| `mogui.lua` | **ch02_yaopu** 畦头, interact | **ch01_shenshougu**, **enter** | 图与 mode 都错 |
| `tanpai.lua` | ch03_mishi, interact | ch03_mishi, **enter** | mode 错 |
| `cangshu_mingdao.lua` | ch03_cangshu, **interact** | ch03_cangshu, **enter** | mode 错 |
| `beidu.lua` | **ch02_yaopu** 第七畦畦背, interact | **ch03_mishi**, interact | 图错 |
| `andao_zhan.lua` | ch03_andao, **interact** | ch03_andao, **enter** | mode 错 |
| `duoshe.lua` | ch02_jusuo, **mode=touch** | ch02_jusuo, **interact** | mode 错（且 `touch` 不是有效取值） |
| `chujue.lua` | **ch03_mishi**, **interact** | **ch02_jusuo**, **enter** | 图与 mode 都错 → **HIGH-1** |
| `tiezhe.lua` | **ch03_mishi** 尸首处, interact | **ch03_andao**, interact | 图错 → MEDIUM-2 |
| `quhun_rudui.lua` | **ch03_mishi** 石屋外, interact | **ch03_andao**, interact | 图错 → MEDIUM-2 |
| `guanshi.lua` | ch02_yaopu npc，**「关卡侧尚未给挂点」** | 已挂 `npc_yaopu_guanshi_ch03`，`visible_flag=ch03.mo_gui_gu` | 申报过时 |
| `lifeiyu.lua` | ch02_wairentang npc，**「关卡侧尚未给挂点」** | 已挂 `npc_li_feiyu_ch03`，`visible_flag=ch03.mo_gui_gu` | 申报过时 |

正确的 7 个：`shichong` / `yingdui` / `miji` / `yunchi` / `jieyao` / `ligu` / `mojuren`。

**为什么是问题**：这些首部注释是编剧写给后人的规格说明，本章的注释质量整体很高
（写法、理由、判据都交待得清楚），**正因为可信度高，错了才更危险**——
HIGH-1 就是从这条缝里掉下去的：`chujue.lua` 用一句关于关卡的错误陈述，
把本章最要害的设计论证建立在了一个不成立的前提上。

**建议**：①把 12 处注释改对；②更要紧的是**让它可验证**——
在 `tools/validate.py` 加一条：脚本首部若写了 `挂在 <map> ... mode=<mode>` 这样的声明，
就拿它去比对 `maps/` 里真实的对象，对不上即报错。
本项目已经有「MAPGEN_IN_SYNC」这种「文档与真相必须一致」的门禁范式，这一条照着做即可。

### LOW（可选）

- **LOW-1**：7 条 `magic.desc.*`（火球术、冰箭术、飞剑术、风雷翅、海潮诀、神识冲，
  以及含「修仙者入门」的那条）写在 `data/text/ch03.json` 里，含「修仙／法术／神识／识海」。
  我验过它们在第 3 章浮不到玩家面前（韩立无法术、施法列表为空），**当前不违约**，
  但第 3 章的文案文件里装着第 6–13 章的词是个隐患。建议挪到 `data/text/magics.json` 一类的公共文件。
- **LOW-2**：`ch03.tanpai.rate`「我活一日，抵旁人十日」与原著 ch30「我活一天相当于普通人活十天」
  几乎同义，是全章最接近转写的一句。中段 `hui`/`hurt`/`rate`/`chang` 四条整体是 ch30 那段自述的
  同序压缩。可辩解（情节必需的信息交代），但若要再改一版，这四条是首选。
- **LOW-3**：契约 `docs/interfaces-p3-ch03.md` 第 1.2 节写「`kSaveVersion` 升到 **3**」，
  实际是 **4**（后来又加了 3→4 的境界属性迁移）。三条迁移都登记了，行为正确，只是契约过时。
- **LOW-4**：`b03_shihai_duoshe.json` 的 `rewards.cultivation = 150` 在识海那一场照常发放，
  而那一场按 5.3 节的口径「不把气血写回存档」。修为写回而气血不写回是不是有意的，
  请实现方确认一句——我没有找到说明。
- **LOW-5**：曲魂在节点 12 入队，而本章此后**再无战斗**，所以「同伴由玩家操作」这件事
  在第 3 章里一次也没真正发生过。符合设计（「入队是章末的事」），测试也覆盖了规则层，
  仅作记录：这条功能要到第 4 章才第一次被玩家实际用到。

### 已知且主控层已接受的，不重复论证

G-3（队伍非战斗时不可见）、G-4（韩立无法术）、G-8（同伴无成长）、G-9（`BattleScene.cpp` 991 行）、
G-10（境界只给气血法力）。另：三章连续超预算的工期风险照第五节记录，不计入本章评分。

---

## 七、裁决

### 评分：**3.5 / 5**（第 2 章初审 2.5、复审 4.0，验收线 4）

分项：

| 项 | 分 | 说明 |
| --- | --- | --- |
| 原创性（机器 + 人工） | 4.5 | 机器口径干净且**自报数字三章以来第一次全部属实**；四场戏三场真的换了取材角度，且换的方向一致 |
| 硬约束落地 | 4.5 | 9 条内容全部符合；#1 出处标注有误 |
| 分支与闸门 | 4.5 | 139,968 条路径零死锁、零不可达、8 个末态全可达、15/15 闸挡得住且无静默返回 |
| 测试有牙 | 4.0 | 5/5 点名项变异全部转红；扣分在 HIGH-1——最要害那条测试钉的是错的一半 |
| 关卡与机制 | 2.0 | HIGH-1 操作感对调、HIGH-2 死人说话、MEDIUM-2 地理错位，三条都出在关卡层 |
| 门禁与工程纪律 | 5.0 | 三道门禁全过、681/681、零警告，我自己跑过 |

**文案与脚本这两层是好的，问题几乎全部集中在关卡层（tmj）**，
而关卡层恰好是唯一没有「文档与真相必须一致」门禁的一层。

### 裁决：**有条件通过**

**不能按现状合并。** 条件是下面两条 HIGH 修完并配上钉得住的测试：

1. **HIGH-1**：对调 `trigger_duoshe` / `trigger_chujue` 的 `mode`，
   同步修正 `Ch03SliceTests.cpp` 中把错误行为钉死的 4 处，
   **并新增一条直接断言两个 trigger `mode` 属性的测试**。
2. **HIGH-2**：给 `ch03_mishi` 的 `npc_mo_daifu` 加 `hidden_flag = "ch03.mo_siwang"`，
   并补一条真图上的「死后无人答话」用例。

两条都是关卡层的有界改动，**不触碰文案**。修完我估这一章到 4.3 左右，过线。

MEDIUM 六条建议排期，其中 **MEDIUM-5（惊蛟会）需要主控层先裁决口径**，
不是编剧能单方面定的；**MEDIUM-1（清毒散）是设计问题**，
它同时否掉了设计第 2 节「清毒散的用处在同一场里教掉」这句话，请一并裁决。

### 给复验的判据，不给结论

复验时请自己跑这几条，不要引用本报告的数字：

1. `cmd.exe /c "build.bat <你的槽>"`，**读日志前先核 mtime**。
2. 把 `trigger_chujue` 的 `mode` 改成 `interact`：**修好之后这 4 条测试应该仍然绿**，
   还绿着就说明 HIGH-1 没修到根上（只改了 tmj、没改测试的驱动方式，或反过来）。
3. 走完节点 11 之后回 `ch03_mishi` 按一下确认键：**不该再有人答话**。
4. 在暗道那一仗里对没中毒的自己用一次清毒散：**不该被白白扣掉一件**。
5. `grep -rn "shihai_yaoxia" scripts/`：**应该有 `flag.get`，不能只有 `flag.set`**。

---

## 附录 A：受检快照与树完整性

复核首尾两次 sha256（前 12 位）一致，`diff` 为空：

```
a74db44ac3b4 data/battles/b03_andao_shishou.json     36b6cccbdd45 data/battles/b03_gu_wai_elang.json
4a1b4833b204 data/battles/b03_shihai_duoshe.json     cae4c1630e1a data/flags.json
d2213b867cc4 data/roles/mo_juren.json                36ad20abde6b data/roles/mo_juren_yuanshen.json
ad70b8a1efaa data/roles/qu_hun.json                  352563160305 data/roles/yu_zitong.json
3cfbd3ba3b8f data/text/ch03.json                     891b53ff27b4 data/text/ch03_main.json
10c24cbc67ef maps/ch03_andao.tmj                     02dcb3bf8f41 maps/ch03_cangshu.tmj
93dd1901da28 maps/ch03_guwai.tmj                     71c6ea66bdbe maps/ch03_mishi.tmj
62d3d1360354 scripts/ch03/andao_zhan.lua             76502441769d scripts/ch03/beidu.lua
6844f3902e94 scripts/ch03/cangshu_mingdao.lua        3b2e41f4279a scripts/ch03/chujue.lua
243795da2aa8 scripts/ch03/duoshe.lua                 fb0a48669e38 scripts/ch03/elang.lua
046e184bd518 scripts/ch03/guanshi.lua                5d6613233456 scripts/ch03/jieyao.lua
f44b9ffdf56d scripts/ch03/lifeiyu.lua                1eb4861616bd scripts/ch03/ligu.lua
a620e571446d scripts/ch03/miji.lua                   7c139e6fd6ce scripts/ch03/mogui.lua
6cd6798ce8ed scripts/ch03/mojuren.lua                a12cefeb7224 scripts/ch03/quhun_rudui.lua
008501be5fa5 scripts/ch03/shichong.lua               8caf01a899d7 scripts/ch03/tanpai.lua
c4694b8ccadd scripts/ch03/tiezhe.lua                 5efa5eadfd00 scripts/ch03/yingdui.lua
131cf907fcfa scripts/ch03/yunchi.lua
```

变异注入共改过 5 个文件（`src/core/battle/BattleState.cpp`、`src/core/battle/Battle.h`、
`src/io/SaveFile.cpp`、`scripts/ch03/chujue.lua`、`maps/ch02_jusuo.tmj`），
每次从我自己的快照还原并 `touch` 推进 mtime。收尾核对：

```
$ diff -rq <backup>/src src && diff -rq <backup>/tests tests && diff -rq <backup>/maps maps \
  && diff -rq <backup>/scripts scripts && diff -rq <backup>/data data
TREE CLEAN
```

我的构建目录 `build-ch03rev/` 已删除。

## 附录 B：一句给下一章的话

**这一章唯一系统性的失守发生在关卡层，而关卡层是这个工程里唯一没有「文档与真相必须一致」门禁的一层。**
文案有禁词扫描、地图几何有 `MAPGEN_IN_SYNC`、校验器有 `SELFTEST_OK`、判据有变异注入——
每一层都被一条会转红的机制盯着，唯独「脚本首部说自己挂在哪儿、怎么触发」这件事全靠人看，
于是 19 个脚本错了 12 个，其中一个错在本章的题眼上。
第 4 章开工前，建议先把 MEDIUM-6 那条校验加上：**它的成本是一个下午，
而它能挡住的正是本章这两条 HIGH。**
