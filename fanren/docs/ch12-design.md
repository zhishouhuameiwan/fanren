# 第 12 章设计：虚天殿

> 对应原著 ch416–499（84 章）。目标时长 **55 min**（大纲第 12 章条目；A 主线 ＋ B 必经口径，按幕分摊见 3.3 与第 14 节）。**起止境界都是结丹初期（31 / 31）**：本章 `realm.advance`、`realm.cap`、`realm.demote` 各 0 处，结丹中期留给第 13 章（ch529）。
> 与第 11 章的衔接：第 11 章章末停在 `ch11_dongfu` 门外、置 `ch11.done`；本章 **patch `scripts/ch11/zhuzhong.lua` 末尾补一行 `teleport("ch12_tushan")`**（落 `andeng`，标 default），第 11 章的图与其余脚本一个字不动（拍板 1）。曲魂 `qu_hun_shadan` 随队进本章，在 n05 离队，此后队伍为空（契约 E1）。**2026-10-13 四路审查后的整改拍板是 16.1 第 36–48 条**（内殿拆四张图、enter 挂点不压 spawn、走偏拆三份脚本、锦帕门闸前移到 n07、玄骨只带魔火等），正文已按它们改过。
> 权威依据：`notes/ch12/research.md`（研究路 2026-10-12，id、旗标、章号、数字只许照抄）；`H:/Work/Kys/docs/大纲.md` 第 12 章条目；大纲与原著冲突之处以原著为准，待回写（对不上的二十处见 1.2）。引擎增补只有两条判据项，另见契约 `docs/interfaces-p3-ch12.md`（E1 空队伍、E2 同一 role 先友后敌）。
>
> P3 · **2026-10-12** · **施工图阶段：本文不改任何代码、脚本、数据、地图，也不碰 `data/flags.json`**（旗标清单见第 11 节，交协调者登记）。第 11 章正在同一棵工作树里集成，**`tests/fixtures/ch11-end-*.sav` 尚未生成**——第 2 节起点资源全部标「估」，并写明哪几项必须拿真档核。
> 文中「拍板 N」＝ 16.1 第 N 条（研究笔记 E 表同号）；「O-N」＝ 16.2 开放问题；「A N」＝ 1.1 表行号；「B 表 N」＝ 1.2 表行号（10.1 的 B1 是平衡路项，不是它）；「契约 E1 / E2」是引擎判据，与拍板号不是一套。
> **3.2 只写框架件与脚本动作，不摘原著镜头顺序与台词**；每段必须落地的剧情事实与不许碰的原著质感在 13.1，写手与复验共用那一张表。

---

## 0. 这一章要做到什么

1. **一张旧锦帕忽然醒了。** 金青邀他去破一处古修士遗址（ch416），同行四人一个个倒在里面，曲魂被一个散尽元婴的老怪夺了舍（ch426–427）；那人从自己肋骨里取出一块残图，他第八章得来的锦帕随之亮起、恒指西北（ch429）。**残图不是在遗址「取」的，是在遗址「醒」的**（B 表 1）——游戏不发新残图，入殿门闸读第 8 章的 `story_jinpa_ditu`。
2. **他第一次一个人。** 曲魂离队（n05 `party.remove`）之后队伍为空，直到第 13 章；临时同行的人只以写死友军进编成（拍板 6）。空队伍本身第 5–7 章已跑过（`ch05`–`ch07-end` 的 `party` 都是 `[]`）；本章新的是「曾有队员、章中途离队，接着进带写死友军的仗」（契约 E1）。
3. **三百年一现的空中玉殿。** 持图者注灵入图、穿罩入殿（ch431）；正魔元婴云集、星宫执法长老只管外殿（ch432–436）。玩家第一次被一群打不过的人围住，只能结盟、忍让、藏拙——大厅 0 战斗、0 店。
4. **三关不是三场仗。** 第一关鬼冤之地是**迷路**（三处岔口，走偏被鬼雾推回——推回是游戏改编）＋鬼王一战；第二关冰火道是**限时**（五日、禁飞，日历计时、永不超时）＋铁火蚁耐久战；过关后石殿四条通道里的阁楼宝光阁是**取舍**（每人一样，他多取一件，宝光阁本身不算一关，ch467）；第三关极妙幻境是**心境**（两段抉择、不失败，ch460、ch467）。大纲「解谜、耐久战、夺宝竞速」改为「迷路解谜、限时耐久、取舍与心境」（B 表 2）。
5. **他拜了一个想杀他的人为师。** 极阴看中的是血玉蜘蛛（ch454–455），玄骨劝他拜、因为九曲灵参的丹方只在玄骨手里（ch456）——不是「为自保投靠」（B 表 5）。
6. **夺鼎是别人的大战。** 元婴混战他只能退后（ch482）；他抢到的是一只收服不了的狼形器灵（ch485）和一口打不开的鼎（ch491、ch499）；乌丑死于玄骨之手（ch489）；玄骨强融修罗圣火失控，他十指连弹剑芒封住去路，玄骨被圣火焚为灰烬（ch493），藏在金雷竹小箭里的残魂又被他令啼魂兽吞灭（ch495）——乌丑不是他杀的，玄骨之死他推了一把。**章末停在灵泉密室出殿之后、鼎盖打不开**，置 `ch12.done`，不 `ending()`、不 teleport、不演逆星盟（拍板 24）。

---

## 1. 硬约束（原著核对）

章号照研究笔记 A 表（研究路 2026-10-12 逐条用 `tools/novel/novel.py find/read` 核过）；本节把 A 表一百二十六条按幕合并成行，**行号保留 A 号**，供 3.0、3.2、13.1 引用。原文片段只作出处定位，不进游戏文案，本表不再抄。

### 1.1 硬约束表

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| **幕一** | **遗址（ch416–431，n01–n07）** | | |
| A1–A3 | 金青来邀，他与曲魂随行出城；两日答复、两个月后到荒岛；两颗珍珠是金青与胡月一同发现的 | 416 | n01；`advance_days(62)`；曲魂此时仍在队；`take("story_bai_zhenzhu", 1)` 看返回值，0 也照走（拍板 4）|
| A4–A7 | 他以北斗两仪阵破最外层幻阵（研究阵法才二十余年、靠辛如音的心得），用了三个月；翅恶是火系妖灵、须水属法器克制 | 417–418 | n02 阵势三选一、旁白不夸大他的阵法；`advance_days(90)`；第二段布水旗，翅恶不进编成 |
| A8–A13 | 看守怪人的花篮是洪荒古宝（ch419；简姓修士的三杆黑幡也是，ch420）；噬金虫数万首次当众实战；战利品有花篮与红光画轴；石柱是封灵柱；他自承有七十二口青竹蜂云剑；曲魂加巨力符推倒石柱 | 419–422 | n03：`magic.learn("magic_shijin_chongqun")`、`give` 花篮与画轴、`portal_dixue` 开；看守怪人不打（拍板 8）；一件 `weapon_qingzhu_fengyun_jian` 代表七十二口 |
| A14–A17 | 池中七霞莲、千年石乳，七霞莲是妖冠蛇的诱饵；脂阳鸟封在画轴里、是阴物克星；石蝶被鬼爪抓走 | 422–425 | n04 抉择「展画轴」；妖冠蛇不进编成；死亡只写倒下（拍板 22）|
| A18–A22 | 金青奔过白骨时被射杀；怪影问他是极阴还是极炫门下；**九柄是他同时操控飞剑的上限**；曲魂被夺舍；引魂钟对夺舍后的分身无效 | 426–427 | n05：画轴燃尽 `take`、`party.remove("qu_hun_shadan")`、「敲钟」必落空（钟不扣）；「极炫」只此一句；全章「九柄」口径，不写七十二口同出 |
| A23–A26 | 曲魂躯体被炼成眉清目秀的少年；对方散尽元婴、顶多相当结丹后期（他也这样估）；未修玄魂大法时已活六百余岁 | 427–428 | NPC `xuangu_shaonian`（名牌「少年」）外观；`xuan_gu` 名牌 CoreLate（拍板 9）；n06 台词可用数字 |
| A27–A29 | 玄骨从肋骨里取出残图；他的锦帕现金剑、恒指西北；玄骨化血光遁走 | 429 | n06；B 表 1；n07 起「循图」；NPC 撤场 |
| A30 | 途中遇文樯父女，文樯是魁星岛旧识 | 430 | n07（地点改编到荒岛东岸，拍板 33）；文思月首见 |
| **幕二** | **入殿（ch431–436，n08–n11）** | | |
| A31–A33 | 虚天殿悬于约千丈高空、金罩罩住；持图者注灵入图、白光护体穿罩；别的修士手持同样的锦帕 | 431 | n08 门闸 `item.count("story_jinpa_ditu") >= 1`；「一人一图」口径（拍板 3）|
| A34–A38 | 玄骨传音：你也有虚天残图，合作；三百年一次；极阴自称「乌某」；蛮胡子的残图是别人孝敬的；九曲灵参能让凝婴机会倍增（玄骨说）| 432–433 | n09 结盟的饵；n10 台词；名牌「极阴祖师」|
| A39–A42 | 殿内禁制只容漂浮术一类小法术；他猜万天明是元婴中期、万法门门主；星宫两名执法长老只管外殿 | 433–435 | 大厅 0 战斗、0 条 `challenge`；名牌只到 CoreLate（O-9）；内殿不管（n30 化身符的由头）|
| A43–A45 | 传送阵过去就是外殿；紫灵自称假丹期，高价买了一块残图来碰运气 | 436 | n11 → `ch12_guiwu`；新 role `zi_ling_xutian`（FoundationLate，B 表 8）；路径行动 `ziling_canyuan_dating` |
| **幕三** | **鬼雾与小空间（ch436–459，n12–n19）** | | |
| A46–A49 | 鬼冤之地结丹修士多被众鬼吞噬，曾有一队遇神智大开的鬼王、整队泯灭仅一人逃出；葛笠只想摘寿元果、原路返回；连快结丹中期的修士都陨落于此 | 437–439 | n12；① 的由头；n13 开头葛笠退场；Z1 玉真人遗骸 |
| A50–A52 | 勾魂鬼音制住紫灵；鬼夜叉有肉身、白日可行；四柄飞剑分化为八柄（ch442）、回收时半路四柄化为乌有（ch444）| 440–444 | n13 抉择「喝醒」；① 第 0 波；**不是损剑**，散去的是分化之影（B 表 9）|
| A53–A55 | 二人同为结丹初期（ch445 旁白，不是他说的）；他提起百余年前与「曲师叔」见过她；绿袍年轻修士（玄骨）从鬼雾毫发无伤走出 | 445 | `yuan_yao_jiedan` CoreEarly；n14 台词（曲魂已不在）；花园 `npc_xuan_gu` 外观同 A23 |
| A56–A57 | 采药一整天，一个月后有阵传回大厅；峡谷里分玄晶道与熔岩路 | 447–448 | n15、`trigger_caiyao`；n19 二选一 |
| A58–A61 | 九曲灵参只有纯金之物捉得住；四象玄武阵旗是玄骨所给；灵参本体长约半尺、土黄；极阴来袭时他锁定九只飞剑 | 449–453 | n16 金网、抉择「换旗」；`story_jiuqu_lingshen` 描述；与 A20 同口径 |
| A62–A66 | 极阴抛出血玉蜘蛛的蛛丝，蜘蛛是取鼎最佳灵兽之一；他拜极阴为记名弟子；玄骨：丹方只他一人知道；紫灵与李姓青年不闯冰火道 | 454–456 | n18 依附的真实缘由（B 表 5）、`give("story_xuanyin_huan", 1)`；紫灵在 n15 之后退场（B 表 6）|
| A67–A68 | 极阴借他白犀佩（防火）；两条窄道各立三四尺高石碑 | 458 | n19 `give("story_baixi_pei", 1)`；`ch12_binghuo` 入口道具 |
| **幕四** | **冰火道与宝光阁（ch459–474，n20–n26）** | | |
| A69–A72 | 冰火道上所有修士无法腾空飞行，他施展罗烟步疾奔；必须五日之内通过、否则传送阵关闭；黑沙漠太大，绕路不可能 | 459–460 | 全图不做飞行；n20 抉择「罗烟步 / 常速」；`ch12.binghuo_ri` 计时（拍板 12）；n21 绕路一支被时限挡回 |
| A73–A75 | 铁火蚁不惧大部分法术，水、冰属性中阶以上法术（及其它属性的个别法术）可伤，噬金虫能吞；他向元瑶要啼魂兽为护送代价，元瑶先开价青火雷，谈妥当场交兽吐珠（ch464）；万只级大群是他遇元瑶之前独自以噬金虫打的（ch463），同行后只遇小群（ch466）| 462–466 | ② 破绽水、金（噬金虫）；n21 谈妥即 `give` 啼魂兽、鸣魂珠（**在 ② 之前**）；青火雷不给他；② 里元瑶并肩打大群是**改编**（原著大群一战无友军）|
| A76–A79 | 铁火蚁后反吐的炼晶；石殿五六十丈、是冰火道出口；四条通道三通阁楼，每人只取一样；蛮胡子坚持把寒冰珠送他 | 466–467 | n22 `give("material_lianjing", 1)`；n23；n24 取舍（拍板 14）；寒冰珠 n19 借、n23 转赠，同一件 |
| A80–A84 | 宝光阁高三十余丈、分两层；星宫长老口中他是「结丹初期的小子」；二层光罩里数十件古宝；噬金虫结成黑甲多取一件，手上是披风与铜环 | 468–470 | `ch12_baoguang`；**全章 realm 31 的出处**；n24 两件都到手：`story_xuese_pifeng`、`story_wuse_tonghuan` |
| A85–A89 | 极妙幻境长廊白云仙音；幻象自称小妹、唤他「四哥」；婆罗珠在幻象靠近时灼热；内殿是五层青石巨塔；蛮胡子当众把一名结丹修士斩成数截 | 471–474 | n25 色段、惧段两处抉择，婆罗珠是提示；n26 只写倒下 |
| **幕五** | **内殿夺鼎（ch475–499，n27–n33）** | | |
| A90–A94 | 每人只能凭残图开一面石室门；狼首傀儡被蛮胡子赤手砸碎；台阶是寒骊台唯一出入口；台背青石是空心夹层，内有陈旧发黄的卷轴 | 475–478 | n27 台词（乌丑）；Z2 收残骸、不进编成；`ch12_hanlitai` 布局；n28 `give("story_neidian_juanzhou", 1)` |
| A95–A98 | 金丝蚕服天青花撼鼎失败；鼎外是至阴至寒的乾蓝冰焰；血玉蜘蛛四级顶峰；蛮胡子对蜘蛛施狂暴之术 | 479–481 | n29 抉择「讨要废蚕」与旁白；`story_xueyu_zhizhu` 不改 |
| A99–A101 | 鼎中古宝化双头火狼破鼎而出；他手持黄红两色玉如意；五枚连成一串的铜环套住银狼 | 483–485 | ③ 第 0 波；胜后 `give("story_yu_ruyi", 1)`；③ 破绽「金」的来路 |
| A102–A105 | 星宫长老以化身符现身；一只血玉蜘蛛被斩成两截；皇鳞甲救他一命；补天丹落入蛮胡子手中 | 486–488 | n30 旁白；蜘蛛物品不扣；**韩立未得补天丹** |
| A106–A110 | 乌丑死于玄骨鬼爪；血玉蜘蛛一窝只两卵活；补天丹配九曲灵参可使凝婴达三四成；玄骨交出丹方玉简；虚天鼎扁圆两耳三足、高约四尺 | 489–491 | n31 旁白与台词，乌丑不是他杀的（B 表 4）；`give` 丹方、虚天鼎 |
| A111–A114 | 修罗圣火彻底失控，他十指连弹青元剑芒封住玄骨去路，玄骨退回火中、化为灰烬（ch493）；残魂藏在金雷竹小箭里，他决意灭之、令啼魂兽吞尽（ch495）；冰焰凝成乾蓝冰珠；拾得一截未被炼化的肋骨 | 493–495 | ④ 胜利文案「圣火反噬」（B 表 3）；13.1 n31 写他封路、n32 写他下令吞魂；敌方可写灰烬（拍板 22）；`story_qianlan_zhu`、`story_xuangu_leigu`（第 13 章用）|
| A115–A119 | 噬金虫找出极阴留下的追踪暗记；卷轴是内殿示意图、红线通密室；灵眼之泉被迁入内殿；万年灵乳一小口回复全部法力；他只要养魂木的根 | 495–498 | n32 → 灵泉密室；n33 `give` 灵乳、根须 |
| A120–A123 | 出口传送到海面；鼎盖如生铸、打不开；银色巨狼他无法控制；他打算弃府经星城传送阵去外星海 | 498–499 | n33 旁白与章末心念一句；玉如意只作 story（拍板 20）|
| A124–A126 | 结丹中期在 ch529 前后；逆星盟首见 ch500；「银月」首见 ch615、「紫罗极火」首见 ch731 | 529 / 500 / 615 / 731 | 本章不升境；三词全章 0 处；器灵只叫「狼形器灵」|

**口径**：上表里「本章演出来的」与「只在旁白里交代的」分别标在 3.0 表一、表二；**不演的**（韩立不在场、或只有作者知道的）不在表里，列在 3.0 表三。

### 1.2 大纲、lore、人物总表与占位数据里对不上的地方（已核，待回写）

| # | 在哪 / 原说法 | 复核（原著）| 本章以什么为准 |
| --- | --- | --- | --- |
| B 表 1 | 大纲「古遗址取**虚天残图**」；人物总表 `jin_qing`「引出虚天残图异变线索」| 他的残图是 ch331 得的锦帕（第 8 章已给）；遗址里是玄骨取出他自己那块，他的锦帕随之异变（ch429）| 回写大纲「古遗址：残图苏醒」；不新给残图 |
| B 表 2 | 大纲「三关闯殿」、玩法「解谜、耐久战、夺宝竞速」；lore 4.6 序列 | 实为大厅 → 鬼冤之地 → 花园 → 小空间（ch447）→ 冰火道 → 石殿四通道（ch467）→ 宝光阁 → 极妙幻境（ch471–473）→ 内殿五层塔 | 回写大纲与 lore 4.6；「夺宝竞速」删 |
| B 表 3 | 大纲「玄骨**自焚**」；lore #46 | 强融冰焰为修罗圣火、被逼急时失控（ch493），不是主动自焚；他以剑芒封住去路、玄骨被圣火焚尽，残魂 ch495 被他令啼魂兽吞灭——他推了一把，但不是一剑杀之 | 回写「强融圣火失控、被他封路焚尽」；④ 文案照此 |
| B 表 4 | `data/roles/wu_chou.json` note「不以其死亡收场」；占位 `b12_neidian_duoding` 敌方乌丑 | 乌丑死于玄骨（ch489），韩立在旁未出手 | `wu_chou.json` **一个字不动**（第 11 章验收 14）；③ 里乌丑改友军（O-11）|
| B 表 5 | 大纲「为丹方与自保依附极阴岛」；lore 5.1 | 极阴图的是血玉蜘蛛（ch454–455）；玄骨以丹方劝他拜（ch456）| 回写大纲 |
| B 表 6 | lore 5 #33「紫灵此后原路退出」| 与他一同走出鬼雾到花园（ch446），之后不闯冰火道（ch456）| 回写 lore；`zi_ling_xutian` 退场旗标 `ch12.xiaojie` |
| B 表 7 | 大纲人物缺金青、葛笠、蛮胡子、万天明；人物总表把蛮胡子 / 万天明 / 天悟子并成一行 `manhuzi_deng` | 四人各有戏份（ch432–488）| 本章分建 role（第 5 节）；回写人物总表拆行 |
| B 表 8 | `data/roles/zi_ling.json` CoreEarly | ch436 自称假丹期 | `zi_ling` 不动；新建 `zi_ling_xutian`（FoundationLate）|
| B 表 9 | 摘要「八柄回收四柄化为乌有」「九只」| ch442 四柄分化为八柄、ch444 回收时四柄（分化之影）化为乌有；九是同时操控的上限（ch426）| 全章只写「九柄」，不写损剑 |
| B 表 10 | `data/roles/yuan_yao.json` QiRefining11 | ch445 已是结丹初期，对外自称姓阮 | 旧 role 不动；新建 `yuan_yao_heipao`、`yuan_yao_jiedan` |
| B 表 11 | `data/roles/wen_qiang.json` QiRefining5 | ch430 两鬓斑白、筑基中期 | 旧 role 不动；新建 `wen_qiang_zhuji`（名牌「文樯」）|
| B 表 12 | 人物总表 `wen_siyue`「韩立以长老身份顺手解围」| 他杀了追堵的三名毒龙会修士；她拒了数名同门结双修的要求、得罪妙音门高层，被派危险的护送差事——此事他听出来了、**没插手**（ch430–431）| 回写；n07 照原著 |
| B 表 13 | 人物总表 `xuan_gu`「元婴期」「临时之盟合谋取鼎」| 约相当结丹后期（ch428）；盟约是「九曲灵参换除极阴」（ch433），结盟时玄骨另附一条：此行他有样东西势在必得、到时要韩立出手相助（未点明是鼎，ch433）；合谋取鼎点明在 ch490 | 回写 |
| B 表 14 | 人物总表 `yin_yue` 行、「银月（收服后定名）」| 「银月」首见 ch615 | role id 用 `langxing_qiling`（名牌「狼形器灵」）；回写 |
| B 表 15 | 占位 `b12_xutian_hudian_kuilei`、role `xutian_hudian_kuilei`、`duoding_xiushi` | 原著三关无守卫傀儡；内殿傀儡由蛮胡子砸碎（ch475–476）| **删**（协调者，连同 `looks.json`、`data/text/battles.json` 两条旧 intro；O-7）|
| B 表 16 | 第 11 章文案 `ch11.wuchou.map`「那**半幅**虚天残图」| ch405 原文没有「半幅」| 第 11 章集成后删两字（O-12），本章不碰 |
| B 表 17 | 大纲「乾蓝冰焰 / 乾蓝冰珠」| 冰焰 ch480；凝珠 ch494 章题「乾蓝珠」、文中亦称「乾蓝冰珠」| 物品名「乾蓝冰珠」|
| B 表 18 | lore 六「宝光阁二层规则：限取 1 件（469–471）」| 规则在石殿处讲、对三座阁楼都成立（ch467）| 回写出处改 467 |
| B 表 19 | 第 11 章接口「残图疑在隐煞门孙姓门主手」| ch431 他用的是自己的锦帕；孙姓门主那份本章不再提 | 照接；孙姓门主线不演 |
| B 表 20 | 摘要「披风」| ch470–471 只叫「披风」，「血色披风」首见 ch499 | 物品名「血色披风」；ch470 段文案写「暗红披风」|

### 1.3 境界曲线与上限

- **原著**：ch416 起全程结丹初期——ch445 旁白「同为结丹初期」、ch468 星宫长老称他「结丹初期的小子」；结丹中期在 ch529（外星海，第 13 章）。
- **游戏**：起点 **31 / 31**（结丹初期，`Realm::CoreEarly`，`Realm.h` 第 35 行），`formerRealm` 22（估，第 2 节）；**章末仍 31 / 31**，`formerRealm` 不读不写。`realm.advance`、`realm.cap`、`realm.demote` 各 **0** 处（拍板 7）。大纲 4.3「12–13 章结丹初期 → 结丹中期」那一步落在第 13 章。
- **面板**：结丹中期所需修为 5000（`Realm.cpp` 第 146 行），`BreakthroughBlock` 照第 11 章口径回绝——本章不靠面板，四场仗的 `rewards.cultivation` 只是攒着。
- **小瓶**：结丹后 3 日一滴、容量 9（第 11 章已抬）；本章**不用**（殿内无灵田、无催熟，`bottle.*` 0 处）。
- **锚点**（`Realm.cpp`；写测试时读表，不抄数）：筑基后期 23 → 480 / — / 44 / 30；**结丹初期 31 → 700 / 470 / 62 / 46**（第 217、234、257、275 行，注释「暂定」）；结丹中期 32 → 900 / 620 / 76 / 58；结丹后期 33 → 1300 / 860 / 102 / 82。本章**不再标定**结丹初期锚点，只标定「结丹初期单人 ＋ 临时友军对结丹后期首领」那一种量法（平衡路，B 前缀，第 8、10 节）。
- **谁在哪一档**：筑基后期——紫灵（假丹）、炜吾；结丹初期——韩立、元瑶、鬼夜叉、铁火蚁群；结丹中期——乌丑（③ 友军）、灰影、双头怪虎、狼形器灵；结丹后期——三首鬼物、玄骨。**元婴诸人**（极阴、青易、温夫人、万天明、天悟子、黑瘦老者、蛮胡子、两名执法长老）名牌一律 CoreLate，文案说「看不透」（拍板 9、O-9）。

### 1.4 第 11 章留下的线头

逐行照接第 11 章接口（`docs/interfaces-p3-ch11.md` 第 6 节）；「C / T」是 10.1 的协调者 / 测试路编号。

| 线头 | 原著本章有没有下文 | 本章怎么交代 |
| --- | --- | --- |
| 章末落点（`ch11_dongfu` 门外，`ch11.done`）| ch416 金青来邀、出城 | **C1：patch `scripts/ch11/zhuzhong.lua` 末尾一行 `teleport("ch12_tushan")`**（拍板 1；「patch 只此一行」的 diff 判据照第 11 章验收 21）；入口 fixture 由 T1 重生 |
| 章节卡 | — | C1：`data/chapters.json` 加 `{"number": 12, "numeral_key": "ui.chapter.12.numeral", "title_key": "ui.chapter.12.title", "done_flag": "ch12.done"}`；`ui.json` 加「第十二章」「虚天殿」（拍板 2）；「未完待续」自动挪到第 12 章之后 |
| 境界 31 / 31，`formerRealm` 22 | 全程结丹初期 | 照接、不抬（1.3）|
| 本命法宝 `weapon_qingzhu_fengyun_jian` ×1 | ch422 七十二口；ch426 九柄 | 照接；剑阵、分光**不做法术**，「九柄」只在文案；神雷走道具（拍板 19）|
| 必成配方（全仓只一张）| — | 照接；本章 0 配方 |
| 曲魂 `qu_hun_shadan` 在队 | ch426–427 被夺舍 | **n05 `party.remove`**；`qu_hun_shadan.json` 一个字不动（拍板 5）；此后空队伍（契约 E1）|
| 金青 NPC、`story_bai_zhenzhu` | ch416 二珠合图 | n01 `take` 珍珠看返回值（拍板 4）；`jin_qing` 不动 |
| 噬金虫 `story_shijin_chong`（旁白已认主数万）| ch421 首次当众实战 | 新法术 `magic_shijin_chongqun`，n03 `magic.learn` 看返回值（拍板 21）；物品不动 |
| 血玉蜘蛛、引魂钟、锦帕地图 | ch454 / 486 / 489；ch427；ch429 / 431 | 三件都**不扣**：一件代表一对蜘蛛；n05 敲钟落空；n08 门闸只读锦帕 |
| 妙音门腰牌；紫灵已迁天星城 | ch436 紫灵入殿 | 紫灵以 `zi_ling_xutian` 在大厅与鬼雾同行；腰牌不读 |
| 极阴岛：乌丑嫁祸、极阴附身、残图疑在孙姓门主手 | ch432 极阴本尊入殿 | n10 `jiyin_zushi` 现身；孙姓门主线不演（B 表 19）|
| 妖丹（五级 0、六级 0–1）| — | 不用、不设门闸 |
| 洞府 `ch11_dongfu` 可回 | ch499 他才起意弃府 | 本章**回不去**（殿内单向）；`field_tianxing_39` 的作物与静室、炼器台随之搁置；第 13 章开局是否回府由第 13 章定（契约第 3 节「洞府 / 灵田」行）|
| 时间：单次 ≤ 3650 | — | 本章单次 ≤ 90（3.3）|
| 灵石：从 `ch11-end` 读 | ch469、ch496 两处「装几块灵石」| 两处 `take(..., 4)` 都有兜底（第 9 节）|
| 规则 25：`MEANS_LAST_CHAPTER = 11`；selftest 探针在 `wu_chou` ＋ `b12_neidian_duoding` | — | C2：改 12；探针挪 `wuji_haishou` ＋ `b13_waixinghai_lieyao`、`probe["chapter"] = 12`（拍板 25）|
| 词表 | — | C3：补 83 词 ＋ 禁词「紫罗极火」（第 12 节）|
| Lua 只读（不加 `realm.former()`）| — | 照接；本章不加任何 Lua 命令 |
| `wu_chou.json` note 与原著相反；文案「半幅」| ch489；ch405 | 本章都不碰（O-11、O-12）|

---

## 2. 起点：C11 档的资源事实（估）与 C12 入口边界

`tests/fixtures/ch11-end-{first,second}.sav` **尚未生成**（第 11 章正在集成）。下表按第 11 章施工图章末口径**估**；字段形状照 `tests/fixtures/ch10-end-first.sav`（研究路 2026-10-12 用 Python 读：`payload` 下 `bag[{itemId,count,herbAge}]`、`party[{roleId,active,hp}]`、`flags{}`、`learnedMagics[]`、`bottle{capacity,drops,lastChargeDay,matureKnown,owned}`）。本章**不加存档字段**（仍 v9）。

| 字段 | 估值（first / second）| 来路 | 本章用法 | 必须拿真档核？ |
| --- | --- | --- | --- | --- |
| `day` | 49378 / 49353 | 第 10 章真档 16003 / 15978 ＋ 第 11 章 3.3 固定 33375 | 3.3 只写增量 182 | **必须**（起点是否被 C1 patch 重生成改动）|
| `realm / realmCap / formerRealm` | 31 / 31 / 22 | 第 11 章第 7 节章末 spec | 全章不动 | 必须 |
| `hp / maxHp`、`mp / maxMp` | 700 / 700、470 / 470 | 同上 | 是否满状态开局 | 必须 |
| `cultivation` | 未知（数百到两千）| 第 11 章四场 `rewards.cultivation` | 不走面板 | 可选 |
| `material_lingshi_zhong` | 20 / 20 | 第 11 章验收 2「两侧恰 20」| n24、n32 兜底第二档 | 可选 |
| `material_lingshi`（低阶）| 数百到四千，两侧差 20 | 第 11 章接口「灵石」行 | n24、n32 各 `take(..., 4)` | **必须**（兜底分支按真值写切片）|
| `party` | `[qu_hun_shadan]` | 第 11 章验收 14：`party.add / remove` 0 处 | n05 `party.remove` 返回值要为真 | **必须** |
| `learnedMagics` | 恰第 10 章那 11 门 | 第 11 章 E17 不新学 | 规则 25 的「木」靠青元剑芒；章末 ＋ `magic_shijin_chongqun` | 必须 |
| `story_jinpa_ditu` | 1 / 1 | `scripts/ch08/shadan.lua` 第 8 行给；第 10 章两份真档实读 1；第 9–11 章 0 处 `take` | n07 门闸（`lidao.lua` teleport 之前判）| **必须** |
| `story_bai_zhenzhu` | 1 / 1 | `scripts/ch11/zhuzhong.lua` `give` | n01 `take` | 必须 |
| `weapon_qingzhu_fengyun_jian` | 1 / 1 | 第 11 章 n30 必成配方 | 规则 25「剑」的来路之一 | **必须** |
| `story_xueyu_zhizhu` / `story_shijin_chong` / `story_yinhun_zhong` | 1 / 1 / 1 | 第 8、10 章给，第 11 章不扣 | n18、n05 旁白 | 必须 |
| 其余（`story_lvhuang_jian`、`story_miaoyin_yaopai`、`story_jinzhi_lingpai` 各 1；`material_yaodan_liuji` 0 / 0；`bottle` capacity 9；`knownWeaknesses` 第 10 章 36 / 35 个 ＋ 第 11 章新揭）| 见左 | 第 10–11 章；第 11 章验收 2、n16 | 都不读、不用；本章新 role 全未揭 | 可选 |
| `mapId / position` | patch 前 `ch11_dongfu` 门外；patch 后 `ch12_tushan` 的 `andeng` | 第 11 章 n31 ＋ C1 | 落点格本身不挂触发器；n01 是它北面横贯岸路的触发带，入口版要先迈一步才演（16.1 第 36 条）| **必须**（T1 重生后核）|

**C12 入口边界**：

- **入口版 ＝ C1 patch 之后由测试路从真 `ch11-end` 重跑生成的档**（T1）；生成后逐项核上表九个「必须」，并记 SHA 与字节数。任何一项与本表不同，先查真驱动证据，再改本节与 3.3 的绝对日，**不改 182 的增量**（O-3），不手改 fixture。
- 入口版的 `mapId` 必须是 `ch12_tushan`、`position` 是 `andeng` 那一格；`ch11.done = 1`、`ch12.*` 全 0。
- 两侧差异来自真实历程（灵石差 20 等），本章不据此另开分岔、也不拉平；意图玩家两侧走同一组选项（第 8 节）。
- 本章**没有一处资源门闸**：n07 送人入殿前读的是第 8 章必给的锦帕；灵石、珍珠、画轴、神雷的 `take` 全看返回值、不足照走（第 9 节）。

---

## 3. 主线节点（五幕、33 个节点）

### 3.0 主干怎么定

**判据三条**（与第 8–11 章同一套，先后有序）：

1. **韩立亲身在场、而且后面的章要用到它的结果**——演成可玩场。
2. **韩立在场、但只是一段「事情就是这样发生的」**——进旁白（三五句，交代因果，不演过程）。
3. **韩立不在场、或者只有作者知道**——**不演**。玩家只知道韩立知道的（拍板 23）。

**表一　可玩场（33 个节点按戏份并成 18 行 ＋ 2 条支线）**

| # | 原著事件 | 章 | 节点 | 为什么要演 |
| --- | --- | --- | --- | --- |
| 1 | 邀约登岛、破幻阵 | 416–418 | n01、n02 | 曲魂最后一段在队；第一处解谜（阵势），他的阵法是辛如音给的 |
| 2 | 看守怪人、噬金虫、推柱 | 419–422 | n03 | 噬金虫成为法术；花篮、画轴的来路 |
| 3 | 七霞莲、鬼雾、怪影、曲魂被夺 | 422–427 | n04、n05 | 同行人接连倒下；**队伍变空**（契约 E1）|
| 4 | 少年自报姓名、残图醒 | 428–429 | n06 | 入殿的钥匙（B 表 1）；玄骨线起点 |
| 5 | 文氏父女 | 429–431 | n07 | 文思月首见；他不插手的分寸（B 表 12）|
| 6 | 循图入殿、玄骨结盟 | 431–433 | n08、n09 | 锦帕注灵入殿（门闸在 n07 送人之前判）；「九曲灵参换除极阴」的盟约（B 表 13）|
| 7 | 群像入殿、星宫规矩、传送阵 | 432–436 | n10、n11 | 正魔元婴群像；殿内规矩是此后一切的前提 |
| 8 | 土坡三人、鬼雾迷路、鬼王 | 436–444 | n12、n13 | 迷路解谜；战斗 ①、神雷首用 |
| 9 | 黑袍人现为元瑶、一日一月 | 444–447 | n14、n15 | 第 10 章一面之缘的下文；进小空间 |
| 10 | 捕九曲灵参、反脸 | 448–453 | n16、n17 | 「换旗」的伏笔；凝婴之物到手 |
| 11 | 极阴逼收记名弟子 | 453–456 | n18 | 拜师抉择（B 表 5）；玄阴环 |
| 12 | 峡谷口、熔岩路 | 456–460 | n19、n20 | 借物；**限时**机制 |
| 13 | 铁火蚁、救元瑶、炼晶 | 460–466 | n21、n22 | 战斗 ②；啼魂兽；第 13 章重炼飞剑的材料 |
| 14 | 石殿、宝光阁 | 466–470 | n23、n24 | 「只取一样」的规矩；**取舍** |
| 15 | 极妙幻境、塔前 | 471–474 | n25、n26 | **心境**；内殿规矩 |
| 16 | 层阶、卷轴、撼鼎、狼形器灵 | 475–485 | n27–n29 | 「结丹只取一层」；出路（拍板 34）；战斗 ③ |
| 17 | 补天丹之争、收鼎、玄骨 | 485–494 | n30、n31 | 补天丹不归他；战斗 ④；虚天鼎、乾蓝冰珠、肋骨 |
| 18 | 残魂、灵泉、出殿 | 494–499 | n32、n33 | 极阴追踪的了结；章末 |
| Z1 / Z2 | 玉真人遗骸；傀儡残骸 | 438–439；475–476 | `trigger_yihai`；n27 内抉择 | 都可选：辟火宝衣、第 13 章傀儡研究（第 6 节）|

**表二　旁白（12 条）**

| # | 原著事件 | 章 | 放在哪 | 为什么不演 |
| --- | --- | --- | --- | --- |
| a | 两日答复与赶路；三个月破阵；等候七八日 | 416、418、431 | n01、n02、n08 | 赶路与等 |
| b | 看守怪人一战与追击 | 419–421 | n03 | 噬金虫一句收场；做仗与 ① 重复（拍板 8；O-1）|
| c | 妖冠蛇、白骨掏丹 | 424–425 | n04 | 他在旁、救不及 |
| d | 毒龙会三人 | 430 | n07 | 剑芒三道，一瞬间的事 |
| e | 寒暄与五六日 | 432–435 | n10 | 群像压成几段（第 14 节可再并）|
| f | 葛笠原路返回；鬼物主魂烧尽 | 439、444 | n13 | 过场 |
| g | 小石山绿影 | 452 | n17 | 电弧一击（拍板 8）|
| h | 炎灵兽；焦黄男修探路殒命 | 459–465 | n20、n21 | 过场；他隐身旁观 |
| i | 塔内三四层 | 475–477 | n27 | 过场 |
| j | 元婴混战；化身符、蛮胡子夺丹 | 482–488 | n29、n30 | 那是别人的仗，他只挨了一下 |
| k | 乌丑之死 | 489 | n31 | 玄骨所为（B 表 4）|
| l | 破阵两三日、海面分手 | 498 | n33 | 过场 |

**表三　不演（韩立不在场、无从得知；10 条，ι 例外见行内）**

| # | 原著事件 | 章 | 玩家怎么知道 |
| --- | --- | --- | --- |
| α | 卜绪与艳女闯关 | 459–460 | 不知道——「卜绪」0 处 |
| β | 星宫二人密谈 | 465、486 | 不知道 |
| γ | 万天明一行商议 | 458、477 | 不知道 |
| δ | 玄晶道上的事 | 465 | 他走的是熔岩路——「银光鼠」「血冰林」0 处 |
| ε | 元瑶对妍姐姐的独白 | 498 | 不知道 |
| ζ | 元婴们出殿后搜塔 | 498 | 不知道（他已离开）|
| η | 孙姓门主那份残图的下落 | — | 原著本章不提（B 表 19）|
| θ | 玄骨肋骨里的功法 | 494 | 第 13 章破出才知道——只许「肋骨」，「玄阴经」0 处 |
| ι | 天星城出了大事 | 499 | **他在场听到**（ch499 三仙宗修士当面告知），按拍板推到第 13 章开场再说——不是「无从得知」 |
| κ | 逆星盟 | 500 | 不在本章——0 处 |

**主干的一句话**：邀约 → 破阵 → 噬金虫 → 同伴倒下 → 曲魂被夺 → 残图醒 → 循图 → 入殿 → 结盟 → 群像 → 传送 → 鬼雾迷路 → 鬼王 → 元瑶 → 小空间 → 灵参 → 反脸 → 拜师 → 熔岩路 → 铁火蚁 → 炼晶 → 取宝 → 幻境 → 塔前 → 层阶 → 卷轴 → 器灵 → 补天丹 → 玄骨 → 残魂 → 灵泉 → 打不开的鼎。前半章他在一群人里一个个失去同行者，后半章他在一群更强的人里只求带着东西活着出去。

### 主线节点表

脚本名 ＝ 节点 id 去掉 `nNN_`（如 `scripts/ch12/dengdao.lua`）。「走掉的日子」照 3.3（本列是 3.3 的摘要，冲突以 3.3 为准）；每节字数在第 13、14 节按幕分钟数配。

| # | 节点 | 地图 | 玩法 | 出处 | 走掉的日子 |
| --- | --- | --- | --- | --- | --- |
| **幕一** | **遗址** | | | | |
| n01 | 登岛 | 土山荒岛 | 二选一（只换一句）；`take` 珍珠 | ch416 | 62 |
| n02 | 破阵 | 土山荒岛 | **阵势三选一循环** | ch417–418 | 90 |
| n03 | 怪人；推柱 | 土山荒岛 | 二选一；`magic.learn`；领物；开门 | ch419–422 | 0 |
| n04 | 七霞莲 | 封灵柱地洞 | 二选一；二选一循环 | ch422–425 | 0 |
| n05 | 夺舍 | 封灵柱地洞 | `take` 画轴；**`party.remove`**；二选一 | ch425–427 | 0 |
| n06 | 萧诧；残图醒 | 封灵柱地洞 | NPC；二选一 | ch428–429 | 0 |
| n07 | 离岛 | 土山荒岛（东岸）| NPC；**门闸（锦帕）**；teleport | ch429–431 | 4 |
| **幕二** | **入殿** | | | | |
| n08 | 入殿 | 外殿大厅 | 叙述（锦帕注灵穿罩）| ch431 | 8 |
| n09 | 旧盟 | 外殿大厅 | NPC；二选一循环 | ch432–433 | 0 |
| n10 | 群像 | 外殿大厅 | 二选一 | ch432–435 | 8 |
| n11 | 传送阵 | 外殿大厅 | teleport | ch436 | 3 |
| **幕三** | **鬼雾与小空间** | | | | |
| n12 | 土坡 | 鬼雾 | 叙述 | ch436–438 | 0 |
| — | 走偏（三处）| 鬼雾 | **迷路**：teleport 回岔口（同图；改编）| ch438 | 0 |
| n13 | 鬼王 | 鬼雾 | 二选一循环；**战斗 ①** | ch439–444 | 0 |
| n14 | 元瑶 | 鬼雾（花园）| 二选一 | ch444–446 | 0 |
| n15 | 一日一月 | 鬼雾（花园）| teleport | ch446–447 | 1 |
| n16 | 灵参 | 小空间 | 二选一（记旗）；领物 | ch448–451 | 0 |
| n17 | 反脸 | 小空间 | 叙述 | ch451–453 | 0 |
| n18 | 拜师 | 小空间 | NPC；**三选一循环**；领物 | ch453–456 | 0 |
| n19 | 峡谷口 | 小空间 | 二选一循环；领物；teleport | ch456–459 | 0 |
| **幕四** | **冰火道与宝光阁** | | | | |
| n20 | 熔岩路 | 冰火道 | **限时**：两处二选一（常速 ＋1 日）| ch459–460 | 1 ＋ 0–2 |
| n21 | 啼魂 | 冰火道 | 二选一循环；领物；**战斗 ②** | ch460–465 | 1 |
| n22 | 炼晶 | 冰火道 | 领物 | ch466 | 0 |
| n23 | 石殿 | 冰火道 | 领物；四选一（合流）；teleport | ch466–468 | 0 |
| n24 | 取宝 | 宝光阁 | 付灵石（兜底）；**二选一取舍**；teleport | ch468–470 | 0 |
| n25 | 幻境 | 内殿塔前（石室出口）| **两段心境抉择** | ch471–473 | 1 |
| n26 | 塔前 | 内殿塔前 | 叙述 | ch473–474 | 0 |
| **幕五** | **内殿夺鼎** | | | | |
| n27 | 层阶 | 内殿塔前 | 二选一（Z2）；teleport | ch475–477 | 0 |
| n28 | 卷轴 | 寒骊台 | 领物 | ch477–478 | 0 |
| n29 | 如意 | 寒骊台 | 二选一；**战斗 ③**；领物 | ch478–485 | 0 |
| n30 | 补天丹 | 寒骊台 | 二选一（合流）| ch485–488 | 0 |
| n31 | 玄骨 | 寒骊台 | 领物；**战斗 ④**；领物 | ch489–494 | 0 |
| n32 | 残魂 | 寒骊台 | 二选一；付灵石（兜底）；teleport | ch494–495 | 0 |
| n33 | 灵泉 | 灵泉密室 | NPC；领物；**章末** | ch496–499 | 3 |

**主动抉择 25 处**（n01、n02、n03、n04×2、n05、n06、n09、n10、n13、n14、n16、n18、n19、n20×2、n21、n23、n24、n25×2、n27、n29、n30、n32），其中回选单的循环 9 处（n02、n04 第二处、n09、n13、n18、n19、n21、n25×2），真改状态的 6 类（`ch12.huanqi`、`ch12.quba_xian`、`ch12.xinjing`、`ch12.jincan`、`ch12.z2`、常速拨日），其余只换一句后合流；**必打战斗 4 场**（第 8 节）；支线 2 条，都可选（第 6 节）。

### 3.1 每一处挂点怎么开始（`mode` / `once`）

分法沿用前十一章：**这件事是他做的（interact），还是发生在他身上的（enter）。** 不用 `auto`（src 与 `validate.py` 里都没有实现）。**enter 只在玩家迈步踏上那一格时判**（`WorldScene.cpp` 第 151–157 行，`tryStep` 里 `state.position = target` 之后）；`teleport` / `loadMap` 只改 position（`Application.cpp` 第 1004–1008 行），落在触发格上什么也不发生——所以本章**没有一个 enter 挂点压在 spawn 上**（16.1 第 36 条）。
**挂 NPC 的五个节点**（n06、n07、n09、n18、n33）没有 once：脚本开头看自己的 done_flag，已置则一句 `return`；`flag.set` 写在脚本里。NPC 的出场靠 visible / hidden 旗标——**portal 与 facility 不读这两个旗标**，门闸只用 `require_flag ＋ deny_text_key`。
**`once=true` 的触发器每一条都写 `set_flag`（＝ done_flag）**；循环抉择、门闸不满足、取消，都在置旗标之前 `return`（与第 10 章 27 同口径）。

| # | 挂点 | 地图 | `mode` | `once` | `guard_flag` | `set_flag` | 战斗 | 为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| n01 | `trigger_dengdao` | `ch12_tushan` | `enter` | ✓ | `ch11.done` | `ch12.dengdao` | — | 船一靠岸就有人迎出来。`andeng`（default spawn，C1 落点）北面一行横贯岸路的触发带（照第 11 章 `trigger_chudao`），落点格本身空着 |
| n02 | `trigger_huangwu` | `ch12_tushan` | `interact` | ✓ | `ch12.dengdao` | `ch12.pozhen` | — | 走到黄雾前布阵的是他。坡上黄雾一格 |
| n03 | `trigger_guaizhao` | `ch12_tushan` | `enter` | ✓ | `ch12.pozhen` | `ch12.tuizhu` | — | 怪人是自己落下来的。光罩前一格（与黄雾错开）|
| n04 | `trigger_shuichi` | `ch12_dixue` | `interact` | ✓ | `ch12.tuizhu` | `ch12.qixialian` | — | 凑到池边看莲的是他。池边一格 |
| n05 | `trigger_shizhu` | `ch12_dixue` | `enter` | ✓ | `ch12.qixialian` | `ch12.duoshe` | — | 怪影从柱里扑出来。北壁石柱前一格（离池 ≥ 3 格）|
| n06 | NPC `npc_shaonian`（`xuangu_shaonian`）| `ch12_dixue` | NPC | — | —（visible `ch12.duoshe`）| —（脚本置 `ch12.canyuan`）| — | 走过去问话的是他。hidden `ch12.canyuan` |
| n07 | NPC `npc_wen_siyue`（`wen_siyue`）| `ch12_tushan` | NPC | — | —（visible `ch12.canyuan`）| —（脚本置 `ch12.lidao`）| — | 上前搭话的是他。东岸；hidden `ch12.lidao` |
| n08 | `trigger_rudian` | `ch12_dadian` | `enter` | ✓ | `ch12.lidao` | `ch12.rudian` | — | 循图而至。`tongdao`（default spawn）北面一行横贯通道的触发带 |
| n09 | NPC `npc_xuan_gu`（`xuan_gu`）| `ch12_dadian` | NPC | — | —（visible `ch12.rudian`）| —（脚本置 `ch12.jiumeng`）| — | 传音是对方先发，但走过去应的是他。hidden `ch12.chuansong` |
| n10 | `trigger_qunxiang` | `ch12_dadian` | `enter` | ✓ | `ch12.jiumeng` | `ch12.qunxiang` | — | 群修入殿是落在他眼前的事。大厅中段柱下 |
| n11 | `trigger_chuansongzhen` | `ch12_dadian` | `interact` | ✓ | `ch12.qunxiang` | `ch12.chuansong` | — | 踏上传送阵的是他。北端尽头 |
| n12 | `trigger_tupo` | `ch12_guiwu` | `enter` | ✓ | `ch12.chuansong` | `ch12.tupo` | — | 一落地就见土坡三人。`tupo`（default spawn）北面一行横贯坡道的触发带 |
| — | `trigger_zoupian_a` / `_b` / `_c` | `ch12_guiwu` | `enter` | ✗ | `ch12.tupo` | — | — | 鬼雾是扑上来的。三条错路的尽头各一格（五元传送，见下）|
| Z1 | `trigger_yihai` | `ch12_guiwu` | `interact` | ✓ | `ch12.tupo` | `ch12.z1` | — | 俯身看遗骸的是他。正路旁死角；**不在目标链上** |
| n13 | `trigger_guiwang` | `ch12_guiwu` | `enter` | ✓ | `ch12.tupo` | `ch12.guiwang` | ① | 鬼音是找上门的。第三岔之后的空地 |
| n14 | `trigger_huayuan` | `ch12_guiwu` | `enter` | ✓ | `ch12.guiwang` | `ch12.yuanyao` | — | 出雾就撞上黑袍人。北端花园入口 |
| n15 | `trigger_tingzi` | `ch12_guiwu` | `interact` | ✓ | `ch12.yuanyao` | `ch12.xiaojie` | — | 走进亭子的是他。花园中央亭 |
| — | `trigger_caiyao` | `ch12_xiaojie` | `interact` | ✓ | `ch12.xiaojie` | `ch12.caiyao` | — | 蹲下采药的是他。草原灵药丛；**不在目标链上** |
| n16 | `trigger_shishan` | `ch12_xiaojie` | `interact` | ✓ | `ch12.xiaojie` | `ch12.lingshen` | — | 布网插旗的是他。东南小石山巨石 |
| n17 | `trigger_fanlian` | `ch12_xiaojie` | `enter` | ✓ | `ch12.lingshen` | `ch12.fanlian` | — | 玄骨是回头来翻脸的。石山下树林边（与巨石错开）|
| n18 | NPC `npc_jiyin`（`jiyin_zushi`）| `ch12_xiaojie` | NPC | — | —（visible `ch12.fanlian`）| —（脚本置 `ch12.baishi`）| — | 应对的是他。石山；hidden `ch12.baishi` |
| n19 | `trigger_xiagu` | `ch12_xiaojie` | `enter` | ✓ | `ch12.baishi` | `ch12.xiagu` | — | 走到谷口就被众人围看。东北峡谷口两碑之间 |
| n20 | `trigger_guangbi` | `ch12_binghuo` | `enter` | ✓ | `ch12.xiagu` | `ch12.rongyan` | — | 穿壁即入禁制。`guangbi`（default spawn）东面一列横贯长道的触发带（存档点、打坐设施在带的西侧）|
| n21 | `trigger_shamo` | `ch12_binghuo` | `enter` | ✓ | `ch12.rongyan` | `ch12.tihun` | ② | 沙漠是撞上的。黑沙漠西缘 |
| n22 | `trigger_jingzhu` | `ch12_binghuo` | `interact` | ✓ | `ch12.tihun` | `ch12.lianjing` | — | 放虫吞柱的是他。沙漠中心晶柱 |
| n23 | `trigger_shidian` | `ch12_binghuo` | `enter` | ✓ | `ch12.lianjing` | `ch12.shidian` | — | 一进石殿就被星宫长老叫住。东端石殿门内 |
| n24 | `trigger_yuqiao` | `ch12_baoguang` | `enter` | ✓ | `ch12.shidian` | `ch12.quba` | — | 过玉桥、看过一层，走到去二层的简易传送阵前才演：传送阵前一格；spawn `yuqiao` 空着（16.1 第 38 条）|
| — | `trigger_yutai` | `ch12_baoguang` | `interact` | ✗ | — | — | — | 一层玉台只给描述、不给物（拍板 14）；在 `yuqiao` 与 `trigger_yuqiao` 之间 |
| n25 | `trigger_changlang` | `ch12_taqian` | `enter` | ✓ | `ch12.quba` | `ch12.huanjing` | — | 幻境是踏进去的。石室出口 |
| n26 | `trigger_taqian` | `ch12_taqian` | `enter` | ✓ | `ch12.huanjing` | `ch12.neidian` | — | 塔前众人已在。塔前 |
| n27 | `trigger_tamen` | `ch12_taqian` | `interact` | ✓ | `ch12.neidian` | `ch12.cengjie` | — | 以残图开门的是他。塔门（与塔前错开 ≥ 2 格）|
| n28 | `trigger_qingshi` | `ch12_hanlitai` | `interact` | ✓ | `ch12.cengjie` | `ch12.juanzhou` | — | 敲青石的是他。台背 |
| n29 | `trigger_taijie` | `ch12_hanlitai` | `enter` | ✓ | `ch12.juanzhou` | `ch12.ruyi` | ③ | 一上台就卷进撼鼎。台阶顶 |
| n30 | `trigger_jitan` | `ch12_hanlitai` | `enter` | ✓ | `ch12.ruyi` | `ch12.butian` | — | 冰封与白虹都是落在他身上的。祭坛前 |
| n31 | `trigger_dongkou` | `ch12_hanlitai` | `interact` | ✓ | `ch12.butian` | `ch12.xuangu` | ④ | 走去洞口应玄骨的是他。洞口旁 |
| n32 | `trigger_xiaojian` | `ch12_hanlitai` | `interact` | ✓ | `ch12.xuangu` | `ch12.canhun` | — | 拾起小箭的是他。台面另一角 |
| n33 | NPC `npc_yuan_yao_lq`（`yuan_yao_jiedan`）| `ch12_lingquan` | NPC | — | —（visible `ch12.canhun`）| —（脚本置 `ch12.done`）| — | 泉边人是先到的，开口谈的是他。灵泉；章末不撤 |
| — | `portal_dixue` / `portal_tushan` | 荒岛 / 地洞 | 门 | — | `require_flag ch12.tuizhu`（只北向那扇）| — | — | 本章唯一一对门；`deny_text_key ch12.block.dixue`（规则 20）|
| — | `facility_dazuo_{dadian,binghuo,taqian,hanlitai}` ×4；`facility_cunji_*` ×10 | 大厅、冰火道、塔前、寒骊台；十张新图 | 设施 | — | — | — | — | 打坐（meditate、`effectiveness 100`）：大厅那处让等候的日子有事做，另三处是 ②③④ 之前的回血兜底（16.1 第 45 条；殿内单向，回血只有 `advance_days` 一成一日与打坐）；每图一处存档点，与 spawn 相邻、在触发带的落点一侧、不压挂点 |

- **同一格只挂一个对象**（引擎同格只取对象表第一个 trigger、不分 `mode`；规则 16 报同格第二个 `enter`「踏不响」）。**enter 挂点不压 spawn**：n01、n08、n12、n20 的挂点是 spawn 前方、玩家离开落点必经的一条横贯触发带（第 11 章 `trigger_chudao` 先例：5 格宽、在 spawn `chengmen` 前 3 格，`maps/ch11_fangshi.tmj`；全仓没有一个 enter 触发器压在 spawn 上），spawn 格与存档点、打坐设施放在带的落点一侧；n24 在简易传送阵前一格。荒岛上黄雾与光罩、地洞里池边与石柱（≥ 3 格）、塔前空地与塔门（≥ 2 格）、寒骊台五处（台背、台阶顶、祭坛前、洞口旁、台面另一角）互不同格；**三处脚本落点格**（`ch12_guiwu` 的三处岔口）不与任何触发器同格。
- **四场必打全挂在目标链的必经挂点上**（n13、n21、n29、n31），每一处的 `set_flag` 都是下一节点的 `guard_flag`——绕不过去。
- **规则 30**：同图同 role 的对象只放行「一个的 hidden 恰等于另一个的 visible」（`validate.py` 第 3132–3142 行）——`ch12_guiwu` 的 `npc_zi_ling`（`tupo` → `guiwang`）与 `npc_zi_ling_hy`（`guiwang` → `xiaojie`）；`ch12_xiaojie` 石山组（`fanlian` → `baishi`）与峡谷口组 `_xg`（`baishi` → `xiagu`）。塔前组 `_tq` 与寒骊台组 `_hl`、`npc_yuan_yao_tq` 与 `npc_yuan_yao_lq` 已分在 `ch12_taqian` / `ch12_hanlitai` / `ch12_lingquan` 三张图，不再同图。不同 role 同名牌：两名执法长老分名牌（5.5）。逐个对象见第 5 节。
- **门**：全章只有荒岛 ↔ 地洞一对门（规则 29 方向见第 4 节）；其余图之间全是脚本单向 teleport——殿内走不回头（ch447、ch467）。

**脚本 teleport 清单**（登 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`，共 12 条；只写图 id 或图 id ＋ 坐标，`teleport` 不认 spawn 名，落点 spawn 一律标 default）：

*三元 `{doneFlag, script, toMap}`（9 条；挂 NPC 或首次置旗，`flag.set` 与 `teleport` 写在顶格）*

| # | 三元 | 挂点 | 落点 |
| --- | --- | --- | --- |
| 1 | `{"ch11.done", "scripts/ch11/zhuzhong.lua", "ch12_tushan"}` | NPC `npc_jin_qing`（`ch11_dongfu`）；**patch 末尾一行**（C1）| `andeng` |
| 2 | `{"ch12.lidao", "scripts/ch12/lidao.lua", "ch12_dadian"}` | NPC `npc_wen_siyue` | `tongdao` |
| 3 | `{"ch12.chuansong", "scripts/ch12/chuansong.lua", "ch12_guiwu"}` | `trigger_chuansongzhen` | `tupo` |
| 4 | `{"ch12.xiaojie", "scripts/ch12/xiaojie.lua", "ch12_xiaojie"}` | `trigger_tingzi` | `caoyuan` |
| 5 | `{"ch12.xiagu", "scripts/ch12/xiagu.lua", "ch12_binghuo"}` | `trigger_xiagu` | `guangbi` |
| 6 | `{"ch12.shidian", "scripts/ch12/shidian.lua", "ch12_baoguang"}` | `trigger_shidian` | `yuqiao` |
| 7 | `{"ch12.quba", "scripts/ch12/quba.lua", "ch12_taqian"}` | `trigger_yuqiao` | `shishi`（石室）|
| 8 | `{"ch12.cengjie", "scripts/ch12/cengjie.lua", "ch12_hanlitai"}` | `trigger_tamen` | `taijiao`（台阶脚南一格）|
| 9 | `{"ch12.canhun", "scripts/ch12/canhun.lua", "ch12_lingquan"}` | `trigger_xiaojian` | `quanbian`（池边）|

*五元 `{doneFlag, script, toMap, reusableObject, hostMap}`（3 条；触发器 `trigger` / `enter` / `once false` / `guard_flag == doneFlag` / `set_flag` 空，脚本 0 处 `flag.set`）*

| # | 五元 | 落点 |
| --- | --- | --- |
| 10 | `{"ch12.tupo", "scripts/ch12/zoupian_a.lua", "ch12_guiwu", "trigger_zoupian_a", "ch12_guiwu"}` | 第一岔口坐标 |
| 11 | `{"ch12.tupo", "scripts/ch12/zoupian_b.lua", "ch12_guiwu", "trigger_zoupian_b", "ch12_guiwu"}` | 第二岔口坐标 |
| 12 | `{"ch12.tupo", "scripts/ch12/zoupian_c.lua", "ch12_guiwu", "trigger_zoupian_c", "ch12_guiwu"}` | 第三岔口坐标 |

- 五元写法此前只用于「回城阵」那种可重复的跨图挂点；同图带坐标回岔口是第一次——测试路核 `ObjectiveRoute` 不把它当链上一步（O-5）。
- **已拍板（16.1 第 39 条）：走偏拆成 `zoupian_a.lua` / `zoupian_b.lua` / `zoupian_c.lua` 三份**，各写死自己那一处岔口的坐标、各 0 处 `flag.set`。理由：Lua 侧拿不到是哪个触发器起的事件、也读不到玩家坐标（`ScriptHost.cpp` 注册的 host 函数只有旗标、物品、境界、日历、小瓶、灵田、队伍、法术查询；`Application::startEvent` 只传脚本路径），一份脚本只能把三条错路送回同一点。`tableRowProblems` 按「一行一个触发器、脚本名对上」核（`ObjectiveTests.cpp` 第 872–886 行），三份各登一行能过。
- `lingquan.lua`（n33）**不 teleport**——第 13 章 patch 它（拍板 24）。
- 门钥匙（`kOffChainDoorKeys`）：`portal_dixue` 的 `ch12.tuizhu` 在链上，不必另登；链外门钥匙：无。

### 3.2 脚本动作与分支

本节仅规定状态操作、控制流和挂点，不提供叙事镜头、示例台词或转述顺序。剧情事实与质感禁用项只按 13.1 核验；对白另行原创。挂点属性以 3.1、日期以 3.3 为准。所有 `take`、`party.remove`、`magic.learn` 都看返回值：失败只记一句、照走（第 9 节退路）。

| 节点 / 脚本 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- |
| n01 dengdao | `take("story_bai_zhenzhu", 1)`；`advance_days(62)`；choice；置 `ch12.dengdao` | 两支只换一句后合流；取消 return |
| n02 pozhen | choice 循环；`advance_days(90)`；置 `ch12.pozhen` | 阵势三选一，错选一句后回选单；取消 return |
| n03 guairen | choice；`magic.learn("magic_shijin_chongqun")`；`give` `story_hualan` 1、`story_zhiyang_huazhou` 1（显示名「红光画轴」，16.1 第 42 条）；置 `ch12.tuizhu` | 两支都以噬金虫收场、合流；置旗后 `portal_dixue` 可过 |
| n04 qixialian | choice；choice 循环；置 `ch12.qixialian` | 第一处只换一句；第二处「再等」一句后回选单，直到选展画轴 |
| n05 duoshe | `take("story_zhiyang_huazhou", 1)`；`party.remove("qu_hun_shadan")`；choice；置 `ch12.duoshe` | 敲钟一支必落空、钟不扣；两支合流 |
| n06 xiaocha | choice；`flag.set("ch12.canyuan")` | 开头 `canyuan` 已置 → 一句 return；两支合流（据实也不泄底）|
| n07 lidao | `advance_days(4)`；顶格 `flag.set("ch12.lidao")` 与 `teleport("ch12_dadian")` | 开头 `lidao` 已置 → return；**锦帕门闸**：顶格 `if item.count("story_jinpa_ditu") < 1 then` 一句 `return end`，写在 `advance_days`、`flag.set`、`teleport` 之前——不足就不送、留在荒岛（数据上不可达，16.1 第 44 条）；不插手、不打 |
| n08 rudian | `advance_days(8)`；置 `ch12.rudian` | 无门闸（落点是无门的大厅，判不足等于把人关死；门闸已前移到 n07）；锦帕只在旁白里注灵穿罩 |
| n09 jiumeng | choice 循环；`flag.set("ch12.jiumeng")` | 开头 `jiumeng` 已置 → return；「再想」一句后回选单 |
| n10 qunxiang | `advance_days(8)`；choice；置 `ch12.qunxiang` | 「不让」一句后照让；合流 |
| n11 chuansong | `advance_days(3)`；置 `ch12.chuansong`；`teleport("ch12_guiwu")` | 无 |
| n12 tupo | 置 `ch12.tupo` | 无 |
| 走偏 zoupian_a / _b / _c | 旁白；`teleport("ch12_guiwu", x, y)`（三份各写死自己那一处岔口的坐标）| 开头 `ch12.guiwang >= 1` → return；各 0 处 `flag.set` |
| Z1 yihai | choice；取衣支 `give("story_bihuo_baoyi", 1)`；置 `ch12.z1` | `ch12.guiwang` 已置 → 一句、return（支线已 fail）；取消 return |
| n13 guiwang | choice 循环；`give("talisman_bixie_shenlei", 2)`；battle ①；`take` 神雷余数；置 `ch12.guiwang` | 「先自守」一句后回选单；败 game_over |
| n14 yuanyao | choice；置 `ch12.yuanyao` | 「问来意」多一段后仍拒；合流 |
| n15 xiaojie | `advance_days(1)`；置 `ch12.xiaojie`；`teleport("ch12_xiaojie")` | 无 |
| 采药 caiyao | `give`（现有灵草一种，id 由协调者选，O-13）2；置 `ch12.caiyao` | 无 |
| n16 lingshen | choice；`flag.set("ch12.huanqi", 1 / 2)`；`give("story_jiuqu_lingshen", 1)`；置 `ch12.lingshen` | 取消 return |
| n17 fanlian | 置 `ch12.fanlian` | `ch12.huanqi == 1` 多一句 |
| n18 baishi | choice 循环；`give("story_xuanyin_huan", 1)`；`flag.set("ch12.baishi")` | 开头 `baishi` 已置 → return；拜青易 / 都不拜各一句传音后回选单，只有拜极阴往下（拍板 16）；蜘蛛不扣 |
| n19 xiagu | choice 循环；`give` `story_baixi_pei` 1、`story_hanbing_zhu` 1；置 `ch12.xiagu`；`teleport("ch12_binghuo")` | 「玄晶道」一句后回选单 |
| n20 rongyan | 先 `flag.set("ch12.binghuo_ri", today())`；`advance_days(1)`；两处 choice；置 `ch12.rongyan` | 每处「常速」`advance_days(1)`，「罗烟步」不拨日 |
| n21 tihun | 读 `today() − binghuo_ri`；choice 循环；谈妥即 `give` `story_tihun_shou` 1、`story_minghun_zhu` 1（ch464 当场交）；battle ②；`advance_days(1)`；置 `ch12.tihun` | 「绕路」一句后回选单（时限挡回）；已用日数分档只换台词，≥ 4 日（玩家在打坐设施静养过）一律走「时限将尽」那一档，不设失败；败 game_over |
| n22 lianjing | `give("material_lianjing", 1)`；置 `ch12.lianjing` | 无 |
| n23 shidian | `give` `story_poluo_zhu`、`story_qingming_zhen`、`story_huanglin_jia` 各 1；choice；置 `ch12.shidian`；`teleport("ch12_baoguang")` | 四门合流（皆通宝光阁）；寒冰珠转赠只改文案、不 `give` |
| n24 quba | 灵石启阵；choice；`flag.set("ch12.quba_xian", 1 / 2)`；`give` `story_xuese_pifeng` 1、`story_wuse_tonghuan` 1；置 `ch12.quba`；`teleport("ch12_taqian")` | 灵石：`take("material_lingshi", 4)` 不成则 `take("material_lingshi_zhong", 1)`，再不成 `advance_days(1)` 照走；两件都到手，先后只换文案 |
| 玉台 yutai | 只描述 | 可反复看，不给物 |
| n25 huanjing | 两段 choice；`ch12.xinjing` 按位写（1 色、2 惧）；`advance_days(1)`；置 `ch12.huanjing` | 错选一句（婆罗珠灼热）后回选单；一次就对才置该位；不失败（拍板 15）|
| n26 neidian | 置 `ch12.neidian` | 无 |
| n27 cengjie | choice（Z2）；收支 `give("story_kuilei_canhai", 1)`、`flag.set("ch12.z2")`；置 `ch12.cengjie`；`teleport("ch12_hanlitai")` | 收 / 不收合流 |
| n28 juanzhou | `give("story_neidian_juanzhou", 1)`；置 `ch12.juanzhou` | 无 |
| n29 ruyi | choice；讨要支 `give("story_jinsi_can", 3)`、`flag.set("ch12.jincan", 1)`；battle ③；`give("story_yu_ruyi", 1)`；置 `ch12.ruyi` | 败 game_over |
| n30 butian | choice；置 `ch12.butian` | 「溜向台阶」被叫回，合流 |
| n31 xuangu | `give` `story_jiuqu_danfang`、`story_xutian_ding` 各 1；`give("talisman_bixie_shenlei", 2)`；battle ④；`take` 神雷余数；`give` `story_qianlan_zhu`、`story_xuangu_leigu`、`story_jinlei_xiaojian` 各 1；置 `ch12.xuangu` | 败 game_over |
| n32 canhun | choice；灵石启阵（同 n24 三档）；置 `ch12.canhun`；`teleport("ch12_lingquan")` | 两支结果同（他都决意灭之、令啼魂兽吞尽，A111）|
| n33 lingquan | `give` `story_wannian_lingru`、`story_yanghun_mugen` 各 1；`advance_days(3)`；`flag.set("ch12.done")` | 开头 `done` 已置 → return；`ch12.xinjing` 各位多一句；**不 `ending()`、不 teleport** |

- 只使用既有脚本接口；本章 `realm.*` 写命令、`party.add`、`bottle.*`、`shop`、`ending` 各 0 处，不加任何 Lua 命令。
- 神雷「`take` 余数」＝ 战后 `item.count("talisman_bixie_shenlei")` 有几张扣几张，章末净增 0（拍板 19）。
- 普通资源不足的支路照常可走；全章唯一的物品门闸是 n07 送人入殿前判的锦帕（数据上必有）。

### 3.3 日历

**本节是相对日历 spec，不是通关实测。** 起点 `day` 估 49378 / 49353（第 2 节，待入口版实读）；本章只钉**增量**：意图玩家章末 `day` − 起点 `day` ＝ **182**（字面量写死，第 8 节通关测试两侧同一组选项）。单次 `advance_days` 最大 90，远低于 `kMaxScriptAdvanceDays = 3650`，不必分段。

| 节点 | `advance_days` | 累计（从第 11 章终局起，意图玩家）|
| --- | --- | --- |
| n01 两日答复；两个月赶路 | 62 | 62 |
| n02 三个月破阵 | 90 | 152 |
| n07 途中数日（地点改编，拍板 33）| 4 | 156 |
| n08 循图、等候七八日 | 8 | 164 |
| n10 五六日寒暄与等候 | 8 | 172 |
| n11 第四日石门落 | 3 | 175 |
| n15 采药一整天 | 1 | 176 |
| n20 熔岩路（先记 `binghuo_ri`）| 1 ＋ 常速 0–2（意图玩家两处罗烟步：0）| 177 |
| n21 黑沙漠 | 1 | 178 |
| n22–n24 | 0（n24 灵石不足再兜底一档 ＋1）| 178 |
| n25 幻境 | 1 | 179 |
| n26–n32 | 0（n32 灵石兜底同 n24）| 179 |
| n33 破阵两三日 | 3 | **182** |

- **范围**：182 ＋ 常速 0–2 ＋ 灵石兜底 0–2 ＝ 182–186（契约第 3 节「时间」行与笔记 N.2 已照此改）；四处打坐设施静养另拨日，玩家自选、意图玩家 0。研究笔记 N.2 只记了常速那 0–2；**兜底那一日 N.2 没计**——意图玩家（中阶灵石 0 块花销、低阶 −8）两处都付得起，不影响 182，但测试路的切片断言要按「常速 / 兜底」各自加日（第 15 节）。
- **冰火道五日永不超时**（拍板 12）：`binghuo_ri` 在 n20 拨日**之前**记；到 n21 开头已用 1–3 日，n21 再 ＋1，到石殿至多 4 日。n21 开头读出的日数只决定台词（「还剩几日」），不设失败分支；绕路一支一句挡回。
- 原著的锚：两日 / 两个月（ch416）、三个月（ch418）、数日后（ch430）、七八日（ch431）、五六日（ch432–435）、第四日（ch436）、一整天 / 一个月（ch447，那一个月是给不闯关的人的，本章不拨）、五日（ch460）、两三日（ch498）。
- **3.2 里没写 `advance_days` 的挂点，按本表拨**（本表是 `advance_days` 的唯一出处）；表里未列的节点、走偏、Z1、采药一律 0 日。

### 3.4 目标链（`data/objectives/ch12.json`）

33 步全进（n01–n33 的 done_flag 依次）；`ch12.z1`、`ch12.z2`、`ch12.caiyao`、走偏都不在链上（若 O-8 要本章出一条必经，Z1 进链、链变 34 步）。

| # | step id | `done_flag` | `target_map` | `target_object` | R-3 提示 |
| --- | --- | --- | --- | --- | --- |
| 1 | `n01_dengdao` | `ch12.dengdao` | `ch12_tushan` | `trigger_dengdao` | |
| 2 | `n02_pozhen` | `ch12.pozhen` | `ch12_tushan` | `trigger_huangwu` | **「坡上那团黄雾是阵——先认出是哪一种」** |
| 3 | `n03_guairen` | `ch12.tuizhu` | `ch12_tushan` | `trigger_guaizhao` | |
| 4 | `n04_qixialian` | `ch12.qixialian` | `ch12_dixue` | `trigger_shuichi` | |
| 5 | `n05_duoshe` | `ch12.duoshe` | `ch12_dixue` | `trigger_shizhu` | |
| 6 | `n06_xiaocha` | `ch12.canyuan` | `ch12_dixue` | `npc_shaonian` | |
| 7 | `n07_lidao` | `ch12.lidao` | `ch12_tushan` | `npc_wen_siyue` | **「锦帕指着西北——离岛之前，东岸像是有人」** |
| 8 | `n08_rudian` | `ch12.rudian` | `ch12_dadian` | `trigger_rudian` | |
| 9 | `n09_jiumeng` | `ch12.jiumeng` | `ch12_dadian` | `npc_xuan_gu` | |
| 10 | `n10_qunxiang` | `ch12.qunxiang` | `ch12_dadian` | `trigger_qunxiang` | |
| 11 | `n11_chuansong` | `ch12.chuansong` | `ch12_dadian` | `trigger_chuansongzhen` | **「殿门迟早要开——等的日子可以在柱下打坐」** |
| 12 | `n12_tupo` | `ch12.tupo` | `ch12_guiwu` | `trigger_tupo` | |
| 13 | `n13_guiwang` | `ch12.guiwang` | `ch12_guiwu` | `trigger_guiwang` | **「跟着啼魂兽开出的路走；雾色发淡处多半走得通」** |
| 14 | `n14_yuanyao` | `ch12.yuanyao` | `ch12_guiwu` | `trigger_huayuan` | |
| 15 | `n15_xiaojie` | `ch12.xiaojie` | `ch12_guiwu` | `trigger_tingzi` | |
| 16 | `n16_lingshen` | `ch12.lingshen` | `ch12_xiaojie` | `trigger_shishan` | **「小石山下布网——这东西只认纯金」** |
| 17 | `n17_fanlian` | `ch12.fanlian` | `ch12_xiaojie` | `trigger_fanlian` | |
| 18 | `n18_baishi` | `ch12.baishi` | `ch12_xiaojie` | `npc_jiyin` | |
| 19 | `n19_xiagu` | `ch12.xiagu` | `ch12_xiaojie` | `trigger_xiagu` | |
| 20 | `n20_rongyan` | `ch12.rongyan` | `ch12_binghuo` | `trigger_guangbi` | **「五日之内走出冰火道——这里飞不起来」** |
| 21 | `n21_tihun` | `ch12.tihun` | `ch12_binghuo` | `trigger_shamo` | |
| 22 | `n22_lianjing` | `ch12.lianjing` | `ch12_binghuo` | `trigger_jingzhu` | |
| 23 | `n23_shidian` | `ch12.shidian` | `ch12_binghuo` | `trigger_shidian` | |
| 24 | `n24_quba` | `ch12.quba` | `ch12_baoguang` | `trigger_yuqiao` | **「阁里的东西每人只能拿一样」** |
| 25 | `n25_huanjing` | `ch12.huanjing` | `ch12_taqian` | `trigger_changlang` | |
| 26 | `n26_neidian` | `ch12.neidian` | `ch12_taqian` | `trigger_taqian` | |
| 27 | `n27_cengjie` | `ch12.cengjie` | `ch12_taqian` | `trigger_tamen` | |
| 28 | `n28_juanzhou` | `ch12.juanzhou` | `ch12_hanlitai` | `trigger_qingshi` | **「台背那块青石的声音不对」** |
| 29 | `n29_ruyi` | `ch12.ruyi` | `ch12_hanlitai` | `trigger_taijie` | |
| 30 | `n30_butian` | `ch12.butian` | `ch12_hanlitai` | `trigger_jitan` | |
| 31 | `n31_xuangu` | `ch12.xuangu` | `ch12_hanlitai` | `trigger_dongkou` | |
| 32 | `n32_canhun` | `ch12.canhun` | `ch12_hanlitai` | `trigger_xiaojian` | |
| 33 | `n33_lingquan` | `ch12.done` | `ch12_lingquan` | `npc_yuan_yao_lq` | |

文案进 `data/text/ch12_objectives.json`（R-3 提示是原创短句，不摘原著）。`ObjectiveRoute` 那三条沿这条链逐步走：荒岛 → 地洞靠 `portal_dixue`（`ch12.tuizhu` 后开），地洞 → 荒岛走回 `portal_tushan`；进图靠第 11 章 patch 那一行，此后八次换图全靠 3.1 清单里的脚本 teleport（九条三元，都只传图 id）；三条五元（走偏）不是链上一步（O-5）。R-3 提示文案（`objective.ch12.*`）按它所属那一步的**上一步**编号进 12.2 的章内先后（n13 的提示含「啼魂兽」，编在 `tupo`）。**本章新增的按键提示：0 条**（问路、打坐都用既有的键，不点名物理键）。

> 写手二，2026-10-12。唯一事实来源：`notes/ch12/research.md`（下称「笔记」）；格式照 `fanren/docs/ch10-design.md` 第 4–9 节。
> 引用约定：「16.1 第 N 条」＝笔记 E 表第 N 条（拍板）；「16.2 O-N」＝笔记 O.3；「16.3 KN」＝笔记 K 表第 N 行（路径行动）；「3.2 nNN」＝主线节点；A、B 编号＝笔记 A、B 表。
> 本部分凡与笔记不一致处都**不擅改**，集中列在各节末「与笔记不一致、待协调者拍板」里，给出推荐改法；协调者改笔记后再定稿。
> NPC 在场写作「出场旗标 → 退场旗标」，**旗标一律省去 `ch12.` 前缀**（`tupo` ＝ `ch12.tupo`）；表外正文写全名。

## 4. 地图（新建 10 张，旧图 0 处修补）

| 地图 id | 名称（`display_name_key`）| 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| default spawn（另有）| 出入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `ch12_tushan` | 土山荒岛 | 48×36 | ✓ | `wild` / `dusk` / backdrop `mountain_forest_dusk` | `andeng`（南岸）；另 `dongkou`（`portal_dixue` 正南一格）| 北坡 `portal_dixue` → `ch12_dixue` `taijie`（往北出、进门朝北），`require_flag ch12.tuizhu`、`deny_text_key ch12.block.dixue`；脚本出口 n07 `teleport("ch12_dadian")` |
| `ch12_dixue` | 封灵柱地洞 | 30×24 | — | `cave` / `indoor` / backdrop `cave_tunnel` | `taijie`（南端台阶脚）| 南端 `portal_tushan` → `ch12_tushan` `dongkou`（往南出、进门朝南），不设门闸 |
| `ch12_dadian` | 虚天殿外殿大厅 | 44×40 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `tongdao`（南端通道口）| 无门；脚本出口 n11 `teleport("ch12_guiwu")` |
| `ch12_guiwu` | 鬼雾 | 56×48 | ✓ | `mountain_path` / `night` / backdrop `mountain_forest`；粒子 `mist`（`ch09_milin` 同一组合）| `tupo`（南端土坡）| 无门；脚本出口 n15 `teleport("ch12_xiaojie")`；走偏三处同图带坐标回岔口 |
| `ch12_xiaojie` | 小空间 | 64×40 | ✓ | `valley` / `day` / backdrop `herb_valley` | `caoyuan`（西端草原）| 无门；脚本出口 n19 `teleport("ch12_binghuo")` |
| `ch12_binghuo` | 冰火道·熔岩路 | 80×28 | ✓ | `cliff` / `dusk` / backdrop `cliff_top_dusk` | `guangbi`（西端光壁）| 无门；脚本出口 n23 `teleport("ch12_baoguang")` |
| `ch12_baoguang` | 宝光阁 | 36×28 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `yuqiao`（南端玉桥头）| 无门；脚本出口 n24 `teleport("ch12_taqian")` |
| `ch12_taqian` | 内殿塔前 | 44×32 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `shishi`（西端石室）| 无门；脚本出口 n27 `teleport("ch12_hanlitai")` |
| `ch12_hanlitai` | 寒骊台 | 40×36 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `taijiao`（台阶脚南一格）| 无门；脚本出口 n32 `teleport("ch12_lingquan")` |
| `ch12_lingquan` | 灵泉密室 | 24×20 | — | `indoor_stone` / `indoor` / backdrop `secret_room` | `quanbian`（池边）| 无门；章末不 teleport（第 13 章 patch）|

**图与图之间只有一对门**（荒岛 ↔ 地洞）；其余全是脚本单向 teleport——殿内走不回头（原著 ch447 一月后才有阵传回大厅、ch467 石殿规矩，A56、A78）。脚本出口共 9 处（C1、n07、n11、n15、n19、n23、n24、n27、n32），都只写图 id、落该图 default spawn；带坐标的同图落点只有 `ch12_guiwu` 三处岔口（坐标由关卡路定、照抄进 `zoupian_a/b/c.lua` 与 `kScriptTransfers` 注释），**落点格与任何触发器都不同格**。

**为什么内殿拆成四张**（16.1 第 37 条）：规则 13 只从 default spawn 出发做 BFS（`validate.py` 第 656 行起 `check_reachability`，取 `defaults[0]` 作起点），脚本落点不算起点；一张图四区以墙隔开，中、东、南三区的存档点、触发器与 NPC 全部会报「从出生点无法到达」（第 11 章 `genmaps_ch11.py` 第 12–14 行同一个坑）。拆开后每张图的脚本落点就是它的 default spawn，n24、n27、n32 只传图 id；塔前与寒骊台、两个元瑶也不再同图（规则 30）；寒骊台自有存档点（③④ 前有档）。

### 4.1 每张图的挂点与在场 NPC

**`ch12_tushan` 土山荒岛**（第一幕：n01–n03、n07）
- 布局：南岸登岛处（spawn `andeng`，存档点 `facility_cunji_tushan` 紧挨它）→ 横贯岸路的 n01 触发带 → 石屋 → 北坡黄雾（`trigger_huangwu`，interact）→ 光罩（`trigger_guaizhao`，enter，光罩前一格，与黄雾错开）→ 北坡地洞口；东岸一片礁滩（n07 文氏父女，改编地点，16.1 第 33 条）。
- 挂点：`trigger_dengdao`（enter，`andeng` 北面一行横贯岸路的触发带——C1 patch 的 `teleport("ch12_tushan")` 落在带南侧，玩家往北离开岸边必踩这条带，踩上就演 n01）、`trigger_huangwu`（n02）、`trigger_guaizhao`（n03）；n07 挂 NPC `npc_wen_siyue`。
- NPC：石屋前 `npc_jin_qing`、`npc_hu_yue`、`npc_jian_xiushi`、`npc_shi_die`（`dengdao → tuizhu`）；东岸 `npc_wen_siyue`、`npc_wen_qiang`（`canyuan → lidao`）。路径行动 K1–K4 挂在金青、石蝶、胡月、文樯上（16.3）。
- 「岛上无灵脉、草木不生」（A3、K1）：底图少植被，碎石与枯土为主；`wild` ＋ 黄昏够用。

**`ch12_dixue` 封灵柱地洞**（第一幕：n04–n06）
- 布局：南端台阶脚（spawn `taijie`，存档点 `facility_cunji_dixue` 在台阶旁）→ 中部水池（七霞莲、千年石乳，`trigger_shuichi`，interact，池边）→ 北壁封灵柱（`trigger_shizhu`，enter，**离池 ≥ 3 格**）。
- NPC：`npc_jin_qing`（`tuizhu → duoshe`）、`npc_hu_yue`、`npc_jian_xiushi`、`npc_shi_die`（`tuizhu → qixialian`）；`npc_shaonian`（role `xuangu_shaonian`，`duoshe → canyuan`，n06 挂点）。
- 四同伴「只写倒下」（16.1 第 22 条）：退场靠 hidden 旗标，地图上不留尸体装饰。

**`ch12_dadian` 虚天殿外殿大厅**（第二幕：n08–n11）
- 布局：南端通道口（spawn `tongdao`，存档点紧挨它）→ 横贯通道的 n08 触发带 → 长厅两排巨柱（每人占一根柱，原著抢柱一段，n10）→ 北端尽头石门后的传送阵（`trigger_chuansongzhen`，interact）。存档点 `facility_cunji_dadian`；打坐 `facility_dazuo_dadian`（meditate，`effectiveness 100`，在一根空柱下）——让 n08、n10 的等候日子有事做（N.2 第二幕）。
- 挂点：`trigger_rudian`（enter，`tongdao` 北面一行横贯通道的触发带；锦帕门闸已前移到 n07）、`trigger_qunxiang`（enter，大厅中段柱下）、`trigger_chuansongzhen`（n11）；n09 挂 NPC `npc_xuan_gu`。
- NPC：`npc_xuan_gu`、`npc_zi_ling`（role `zi_ling_xutian`）、`npc_miao_zhanglao`（`rudian → chuansong`）；`npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_wen_furen`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_man_huzi`、`npc_zhifa_ci`、`npc_zhifa_leng`（`qunxiang → chuansong`）。
- **0 战斗、0 `challenge`**：殿内禁制只容小法术（A39）。

**`ch12_guiwu` 鬼雾**（第三幕前半：n12–n15；显示名用泛称，文案从 `tupo.*` 起才叫「鬼冤之地」）
- 布局：南端土坡（spawn `tupo`，存档点 `facility_cunji_guiwu` 在土坡白光内、紧挨它）→ 横贯坡道的 n12 触发带 → 鬼雾区：一条正路、**三处岔口**，每处一条错路，错路尽头各一格 `trigger_zoupian_a` / `_b` / `_c` → 第三岔之后一块空地（`trigger_guiwang`，enter；**空地做成去花园必经的一格宽咽喉**，绕不过）→ 北端花园入口（`trigger_huayuan`，enter）→ 花园中央亭（`trigger_tingzi`，interact）。正路旁一处死角放玉真人遗骸（`trigger_yihai`，interact，Z1）。
- 正路的标记（16.1 第 10 条）：沿正路摆白骨堆与「雾色发淡」的地砖装饰（啼魂兽开出的通道）；错路两侧雾浓、无白骨。K9 葛笠给这条提示。
- 挂点：`trigger_tupo`（enter，`tupo` 北面一行横贯坡道的触发带）、`trigger_zoupian_a/b/c`（enter、once 否、guard `ch12.tupo`、无 set_flag）、`trigger_yihai`、`trigger_guiwang`、`trigger_huayuan`、`trigger_tingzi`，**各占一格**；三处岔口落点也各占一格。
- NPC：土坡 `npc_zi_ling`（`tupo → guiwang`）、`npc_ge_li`（`tupo → guiwang`）；花园 `npc_zi_ling_hy`（`guiwang → xiaojie`，与土坡那个首尾恰接，规则 30）；`npc_yuan_yao`（role `yuan_yao_jiedan`）、`npc_xuan_gu`、`npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_wan_tianming`、`npc_man_huzi`、`npc_zhifa_ci`、`npc_zhifa_leng`（`yuanyao → xiaojie`）。
- ① 前的存档点：土坡那一处；走到 `trigger_guiwang` 要先过三处岔口，存档点在全部挂点之前。

**`ch12_xiaojie` 小空间**（第三幕后半：n16–n19）
- 布局：西端草原（spawn `caoyuan`，存档点 `facility_cunji_xiaojie` 紧挨它；草原一丛灵药 `trigger_caiyao`，interact、once、set `ch12.caiyao`）→ 东南小石山（巨石 `trigger_shishan`，interact）与山下树林边（`trigger_fanlian`，enter，与巨石错开）→ 东北峡谷口，两块三四尺高石碑（A68）之间 `trigger_xiagu`（enter）。
- NPC：石山 `npc_xuan_gu`（`xiaojie → lingshen`）；`npc_jiyin`（n18 挂点）、`npc_wu_chou`、`npc_qingyi`（`fanlian → baishi`）；峡谷口 `npc_jiyin_xg`、`npc_wu_chou_xg`、`npc_qingyi_xg`、`npc_man_huzi`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_yuan_yao`（`baishi → xiagu`）。石山与峡谷口两组极阴 / 乌丑 / 青易**首尾恰接**（`baishi`），规则 30 放行。
- 「一整天」（A56）：采药只此一格；其余灵药丛是装饰。

**`ch12_binghuo` 冰火道·熔岩路**（第四幕前半：n20–n23）
- 布局（西 → 东一条长道，**全程不能飞**，A69）：光壁（spawn `guangbi`）→ 熔岩夹道 → 草地 → 沼泽（n20 两处「罗烟步 / 常速」的地景，挂点只有 n20 一格）→ 黑沙漠西缘（`trigger_shamo`，enter）→ 沙漠中心晶柱（`trigger_jingzhu`，interact）→ 东端石殿（门内一格 `trigger_shidian`，enter）。
- 挂点：`trigger_guangbi`（enter，`guangbi` 东面一列横贯长道的触发带）、`trigger_shamo`、`trigger_jingzhu`、`trigger_shidian`。
- 存档点 `facility_cunji_binghuo` 与打坐设施 `facility_dazuo_binghuo`（meditate、`effectiveness 100`）：都在光壁内侧、紧挨 spawn、n20 触发带的西侧（笔记 G 表已改）——② 之前有档、有回血兜底（本节末第 2 条、16.1 第 45 条）。
- NPC：石殿 `npc_jiyin`、`npc_wu_chou`、`npc_qingyi`、`npc_man_huzi`、`npc_wan_tianming`、`npc_tianwuzi`、`npc_heishou`、`npc_xuan_gu`、`npc_zhifa_ci`、`npc_zhifa_leng`、`npc_yuan_yao`（`lianjing → shidian`）。

**`ch12_baoguang` 宝光阁**（第四幕：n24）
- 布局：玉桥头（spawn `yuqiao`，存档点 `facility_cunji_baoguang` 紧挨它）→ 玉桥、匾额 → 一层玉台（`trigger_yutai`，interact、once 否、无旗标，可反复看）→ 一层通二层的简易传送阵，阵前一格 `trigger_yuqiao`（enter，n24；灵石兜底在脚本里）→ 二层光罩只在 n24 文案里，不画可走区。玩家从玉桥走过一层、看过玉台才踩到 n24（16.1 第 38 条）。
- NPC：无。

**`ch12_taqian` 内殿塔前**（第四幕末：n25–n27）
- 布局：石室（spawn `shishi`，存档点 `facility_cunji_taqian`、打坐 `facility_dazuo_taqian` 紧挨它）→ 石室出口 `trigger_changlang`（enter，n25 长廊与黑殿两段都在脚本里）→ 塔前空地 `trigger_taqian`（enter）→ 五层青石塔门 `trigger_tamen`（interact，与塔前**错开 ≥ 2 格**）。
- NPC：`npc_jiyin_tq`、`npc_wu_chou_tq`、`npc_qingyi_tq`、`npc_man_huzi_tq`、`npc_wan_tianming_tq`、`npc_tianwuzi_tq`、`npc_heishou_tq`、`npc_xuan_gu_tq`、`npc_yuan_yao_tq`（`huanjing → neidian`）。

**`ch12_hanlitai` 寒骊台**（第五幕：n28–n32）
- 布局：台阶脚南一格（spawn `taijiao`，存档点 `facility_cunji_hanlitai`、打坐 `facility_dazuo_hanlitai` 紧挨它）→ 台背青石 `trigger_qingshi`（interact）→ 台阶顶 `trigger_taijie`（enter）→ 祭坛前 `trigger_jitan`（enter）→ 洞口旁 `trigger_dongkou`（interact）→ 台面另一角 `trigger_xiaojian`（interact）。台阶是唯一出入口（A92），台下一圈不留别的缺口。
- NPC：`npc_jiyin_hl`、`npc_qingyi_hl`、`npc_man_huzi_hl`、`npc_wan_tianming_hl`、`npc_tianwuzi_hl`、`npc_heishou_hl`（`cengjie → butian`）、`npc_wu_chou_hl`、`npc_xuan_gu_hl`（`cengjie → xuangu`）。
- ③④ 之前的档：本图存档点；意图玩家 ③ 打完不回血直接打 ④（平衡路按此量），台下打坐设施是兜底——n30 演完、n31 之前玩家可走回台下静养（16.1 第 45 条）。

**`ch12_lingquan` 灵泉密室**（章末：n33）
- 布局：池边（spawn `quanbian`，存档点 `facility_cunji_lingquan` 紧挨它）；`npc_yuan_yao_lq`（`canhun →` 不撤，n33 挂点）。

### 4.2 命名、属性与美术

- `chapter=12`；`display_name_key = ch12.map.<短名>.name`；`region` 沿第 10–11 章 `luanxinghai_*` 前缀，由协调者定一个值（笔记无）。
- `bgm`：**只用已有曲子**（`bgm_cliff`、`bgm_wild`、`bgm_valley`、`bgm_town`、`bgm_river`、`bgm_night` 等），每图一首由关卡路挑；`tools/audiogen/catalog.py` 在 `MAP_BGM` **上方的注释表**加 10 行 `# ch12_xxx -> bgm_yyy`（`MAP_BGM` 是曲子 id 元组、第 58 行断言每项都在 `BGM` 里，全用已有曲子时元组不动），`docs/audio.md` 同一张表加 10 行。
- **主题、时辰、背景全用现有组合**（16.1 第 29 条）；不加新主题、不做新背景。战斗背景见 8.2。
- `tools/mapgen/genmaps_ch12.py`（10 张）接进 `genmaps.py --check`；`tools/artgen/artgen.py` 烘 10 张并 `--check` 过；**本章不 patch 任何旧图**（C1 是改 `scripts/ch11/zhuzhong.lua` 一行，不是地图）。
- 存档点 10 处，每图一处，全部在主线挂点之外、紧挨 spawn；打坐设施 4 处（大厅、冰火道、塔前、寒骊台）。
- **同一格只取第一个 trigger**：本章没有同格换触发器；三处走偏、`trigger_yutai`、`trigger_caiyao`、`trigger_yihai` 各占一格，与主线挂点、spawn、落点都错开（规则 16）。

### 4.3 地图的整改拍板（原「与笔记不一致」四条，已落）

1. **`trigger_yuqiao` 挪到简易传送阵前一格**（16.1 第 38 条）：原写法落在 spawn 上——且落在 spawn 上本来就踩不响（3.1 首段）；就算踩得响，n23 一送进来就演完 n24、直接送走，`trigger_yutai`（拍板 14）永远走不到。挪后 spawn `yuqiao` 空着，玩家走过一层、看玉台、踩到传送阵前才演 n24。触发器的完成旗标由脚本自己置（`WorldScene.cpp` 第 32–65 行），挪格子不影响 once 契约；3.1 清单第 7 条（`quba.lua`）只改目标图为 `ch12_taqian`。
2. **`facility_cunji_binghuo` 摆在光壁内侧、紧挨 spawn**：笔记原写「石殿内」在 ② 之后，② `defeat_is_fatal true`，败了要从小空间重来；笔记 G 表已改。
3. **原 `ch12_neidian` 东区无存档点**：已随拆图解决——`ch12_hanlitai` 自有存档点与打坐设施（16.1 第 37、45 条），存档点总数 10。
4. **规则 30**：花园紫灵改 `guiwang` 出场、两名执法长老分名牌（5.5、16.1 第 43 条）；塔前与寒骊台、两个元瑶随拆图不再同图。
5. **内殿拆四张图**（16.1 第 37 条）：理由见第 4 节表下。

---

## 5. 角色

**新建 32、就地改写 0、删 2、一个字不动 4**（笔记 H 表）。凡说话、上场的人都有 `data/roles/` 条目；外观进 `data/visual/looks.json`，`sprite_preview.py --coverage` 缺图为 0。境界写 realm 枚举，「只说话」的 role 数值照 `Realm.h` 锚点（笔记 C.2）与第 10–11 章路人写法。**元婴诸人名牌一律 `CoreLate`**（本作上限，天眼术显示「结丹后期」，文案说「看不透」，16.1 第 9 条、16.2 O-9）。

### 5.1 剧情角色（地图上有对象的）

| id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 在场（图 `对象`：出场 → 退场）|
| --- | --- | --- | --- | --- | --- |
| `qu_hun_shadan` | 曲魂（身外化身）| 386 | CoreEarly | **不动** | 队伍（起点在队）→ n05 `party.remove` |
| `jin_qing` | 金青 | 415 | CoreEarly | 不动 | tushan `npc_jin_qing`：`dengdao → tuizhu`；dixue：`tuizhu → duoshe` |
| `hu_yue` | 胡月 | 416 | CoreEarly | 新建 | tushan：`dengdao → tuizhu`；dixue：`tuizhu → qixialian` |
| `jian_xiushi` | 简姓修士 | 416 | CoreMid | 新建 | 同胡月 |
| `shi_die` | 石蝶 | 416 | FoundationMid | 新建 | 同胡月 |
| `xuangu_shaonian` | 少年 | 427 | CoreLate | 新建 | dixue `npc_shaonian`：`duoshe → canyuan`（n06 挂点）|
| `wen_siyue` | 文思月 | 430 | FoundationEarly | 新建 | tushan `npc_wen_siyue`：`canyuan → lidao`（n07 挂点）|
| `wen_qiang_zhuji` | 文樯 | 369 | FoundationMid | 新建（旧 `wen_qiang` 不动，B 表 11）| tushan `npc_wen_qiang`：`canyuan → lidao` |
| `xuan_gu` | 玄骨 | 428 | CoreLate | 新建（③ 友军、④ 敌人）| dadian `rudian → chuansong`（n09 挂点）；guiwu 花园 `yuanyao → xiaojie`；xiaojie 石山 `xiaojie → lingshen`；binghuo `lianjing → shidian`；taqian `huanjing → neidian`；hanlitai `cengjie → xuangu` |
| `zi_ling_xutian` | 紫灵 | 403 | FoundationLate | 新建（B 表 8；① 友军）| dadian `npc_zi_ling` `rudian → chuansong`；guiwu 土坡 `npc_zi_ling` `tupo → guiwang`、花园 `npc_zi_ling_hy` `guiwang → xiaojie`（规则 30 恰接）|
| `miao_zhanglao` | 苗长老 | 379 | CoreEarly | 不动 | dadian `rudian → chuansong` |
| `jiyin_zushi` | 极阴祖师 | 382 | CoreLate | 新建（`jiyin_fushen` 不动）| dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；xiaojie 石山 `fanlian → baishi`（n18 挂点）、峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；taqian `huanjing → neidian`；hanlitai `cengjie → butian` |
| `wu_chou` | 乌丑 | 382 | CoreMid | 不动（③ 友军；B 表 4）| 同极阴，唯寒骊台 `cengjie → xuangu` |
| `qingyi_jushi` | 青易居士 | 435 | CoreLate | 新建 | 同极阴 |
| `wen_furen` | 温夫人 | 432 | CoreLate | 新建 | dadian `qunxiang → chuansong` |
| `wan_tianming` | 万天明 | 434 | CoreLate | 新建 | dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；taqian `huanjing → neidian`；hanlitai `cengjie → butian` |
| `man_huzi` | 蛮胡子 | 432 | CoreLate | 新建 | 同万天明 |
| `tianwuzi` | 天悟子 | 434 | CoreLate | 新建 | 同万天明，**guiwu 不在** |
| `heishou_laozhe` | 黑瘦老者 | 434 | CoreLate | 新建 | 同天悟子 |
| `xinggong_zhifa_ci` | 慈眉长老 | 435 | CoreLate | 新建 | dadian `qunxiang → chuansong`；guiwu `yuanyao → xiaojie`；binghuo `lianjing → shidian` |
| `xinggong_zhifa_leng` | 冷脸长老 | 435 | CoreLate | 新建 | 同上（ch435「冷脸老者」；两块名牌必须不同，台词里仍称执法长老，5.5）|
| `ge_li` | 葛笠 | 437 | CoreEarly | 新建 | guiwu 土坡 `tupo → guiwang` |
| `yuan_yao_jiedan` | 元瑶 | 375 | CoreEarly | 新建（② 友军；B 表 10）| guiwu 花园 `yuanyao → xiaojie`；峡谷 `baishi → xiagu`；binghuo `lianjing → shidian`；taqian `huanjing → neidian`；lingquan `npc_yuan_yao_lq` `canhun →` 章末不撤（n33 挂点）|

- **玄骨三段**：少年（`xuangu_shaonian`，n05 成形到 n06 自报）→ 玄骨（`xuan_gu`，n06 起）；`xuangu_shaonian` 与 `xuan_gu` 外观同一张「眉清目秀少年」（A23），`xuan_gu` 换绿袍（A55）。**名牌不出现「萧诧」**（他自报一次，在台词里）。
- **元瑶两段**：鬼雾里是「黑袍人」（`yuan_yao_heipao`，只在 ① 编成里，无地图对象），n14 现为女修之后是 `yuan_yao_jiedan`；旧 `yuan_yao`（第 10 章炼气）不动。文案「元瑶」二字只许在 key ≥ `ch12.yuanyao` 的段（笔记 M 表章内先后）。
- **紫灵**：本章是假丹（A44），旧 `zi_ling`（第 11 章在用）不动；过第一关后不闯冰火道，在 n15 之后退场（B 表 6）。
- **乌丑**：`wu_chou.json` 一个字不动（第 11 章验收要它逐字节同第 10 章，16.2 O-11）；本章他只做 ③ 友军与群像，死于玄骨之手只写倒下（A106、16.1 第 22 条）。

### 5.2 只说话的（无地图对象，n 脚本 `talk` 用）

| id | 名牌 | 首见章 | 境界 | 用在 |
| --- | --- | --- | --- | --- |
| `dao_guairen` | 怪人 | 419 | CoreMid | n03（噬金虫收场，不打）|
| `dixue_guiying` | 怪影 | 425 | CoreLate | n05（问师门）|
| `xuangu_lvying` | 绿影 | 446 | CoreLate | n17（被电弧击碎，旁白）|

### 5.3 只在战斗里的（编成用，无地图对象）

| id | 名牌 | 首见章 | 境界 | 阵营 · 场次 |
| --- | --- | --- | --- | --- |
| `yuan_yao_heipao` | 黑袍人 | 436 | CoreEarly | 友军 · ① |
| `weiwu_hanggui` | 炜吾 | 441 | FoundationLate | 敌 · ① 第 0 波 |
| `gui_yecha` | 鬼夜叉 | 440 | CoreEarly | 敌 · ① 第 0 波 |
| `hui_ying` | 灰影 | 441 | CoreMid | 敌 · ① 第 1 波 |
| `shuangtou_guihu` | 双头怪虎 | 442 | CoreMid | 敌 · ① 第 1 波 |
| `sanshou_guiwu` | 三首鬼物 | 443 | CoreLate | 敌（首领）· ① 第 2 波 |
| `tiehuo_yiqun` | 铁火蚁群 | 462 | CoreEarly | 敌 · ② 第 0、1 波 |
| `tiehuo_yiqun_da` | 铁火蚁大群 | 463 | CoreMid | 敌（首领）· ② 第 2 波 |
| `langxing_qiling` | 狼形器灵 | 483 | CoreMid | 敌 · ③ 第 0 波（双头火形）|
| `langxing_qiling_yin` | 狼形器灵 | 485 | CoreMid | 敌（首领）· ③ 第 1 波（独角银形）|

另：`zi_ling_xutian`（① 友军）、`yuan_yao_jiedan`（② 友军）、`wu_chou` 与 `xuan_gu`（③ 友军）、`xuan_gu`（④ 敌）见 5.1。**器灵永远叫「狼形器灵」**，「银月」全作禁用（A126、B 表 14）。

### 5.4 不动、删除、外观

- **不动**：`qu_hun_shadan`、`jin_qing`、`wu_chou`、`miao_zhanglao`（本章用），以及 `qu_hun`、`zi_ling`、`yuan_yao`、`wen_qiang`、`jiyin_fushen` 等旧 role（本章不用）。
- **删**（16.1 第 18 条、B 表 15）：role `xutian_hudian_kuilei`、`duoding_xiushi`、编成 `b12_xutian_hudian_kuilei` 与 `data/text/battles.json` 两条旧 intro 由**协调者**删；`looks.json` 两条由**内容路 A** 删（契约 6.2 白名单）；同一批落、在第 11 章集成**之后**（16.2 O-7）。
- **外观**：人形 23 个按名牌身份配（少年与玄骨同一张脸）；**非人形 9 个**（炜吾、鬼夜叉、灰影、双头怪虎、三首鬼物、两种蚁群、两形狼）进 `tools/artgen/sprites_enemies.py`，待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下（第 8、10 章同口径）。两形狼一张火色双头、一张银色独角（A99、A101）。

### 5.5 撤场 / 出场与规则 30（`NpcPresenceTests`、`validate.py` 规则 30）

- **跨图**（任一时刻一个 role 只在一张图）：上表每个 role 前一张图的退场旗标都在后一张图的出场旗标之前（目标链序，笔记 J.1），全部过得去；第 11 章 `ch11_yinsha_dao` 的 `npc_wu_chou` 以 `ch11.jiyin` 撤场、早于本章。**本章 NPC 在场旗标全在目标链上**，链外 8 个旗标（J.2）没有一个挂在 NPC 上。
- **同图同 role**：规则 30 只放行「一个的 `hidden_flag` **恰是**另一个的 `visible_flag`」（`validate.py` 第 3132–3142 行），「首尾不重叠」不够。逐对核过（已落，16.1 第 43 条）：
  - xiaojie 石山 / 峡谷两组（极阴、乌丑、青易）：`baishi` ＝ `baishi`，**过**。
  - guiwu 两个紫灵：花园 `npc_zi_ling_hy` 的 visible 定为 `ch12.guiwang`（＝土坡那个的 hidden），**过**——鬼王一死她就跟他出雾；花园在 `trigger_guiwang` 咽喉格之后，提早出场玩家也看不见。
  - 原 neidian 塔前 / 寒骊台八个 role、两个元瑶：随拆图分在 `ch12_taqian`、`ch12_hanlitai`、`ch12_lingquan` 三张图，**不再同图**，规则 30 不管；跨图仍按目标链序先退后出（`neidian` 先于 `cengjie`、`canhun`）。`lingquan.lua` 不必另判 `ch12.canhun`（NPC 在 `canhun` 之前不在场）。
- **不同 role 同名牌**：`xinggong_zhifa_ci` 与 `xinggong_zhifa_leng` 在 dadian、guiwu 花园、binghuo 石殿三处同框——同名必红。名牌定为「慈眉长老」「冷脸长老」（ch435：两名白衣老者，一位慈眉善目、一位冷脸，`novel.py find` 核过），台词里仍称「星宫执法长老」。名牌是 role 的 `name`，归内容路 B（笔记 H 表已改）。
- 以上只改旗标、名牌与图的归属，不改 id；H 表、J.1「谁读」列、K 表已一并回写笔记。

---

## 6. 支线（2 条，都可选）

| | Z1 埋骨（可选）| Z2 傀儡残骸（可选）|
| --- | --- | --- |
| 出处 | ch438–439：鬼雾里一具快到结丹中期的修士遗骸（玉真人，A49），他取了辟火宝衣、把骨埋了，说是埋骨的报酬 | ch475–476：内殿狼首傀儡、蛇卫被蛮胡子砸碎，他把残骸一一收进储物袋（A91）|
| 接取 | `ch12.tupo`（进了鬼冤之地）| `ch12.neidian`（进了塔）|
| 步骤 | 一步：正路旁死角 `trigger_yihai`（interact、once、guard `ch12.tupo`、set_flag `ch12.z1`）；抉择「取衣而后埋骨 / 只埋骨」，前者 `give("story_bihuo_baoyi", 1)`，两支都置 `ch12.z1` | 一步：**无独立挂点**，在 n27 `cengjie.lua` 里一处抉择「收残骸 / 不收」；收则 `give("story_kuilei_canhai", 1)`、`flag.set("ch12.z2")` |
| 完成 | `ch12.z1 >= 1` | `ch12.z2 >= 1` |
| 失败 / 过期 | `ch12.guiwang`（鬼王一战之后）| `ch12.cengjie`（n27 演完）|
| 奖励 | 不进 rewards（`story_bihuo_baoyi` 由脚本 `give`；n20 旁白多一句「宝衣挡了热浪」）| 不进 rewards（第 13 章傀儡研究用）|
| 文件 | `data/quests/q12_maigu.json`，`origin "原著"` | `data/quests/q12_kuilei_canhai.json`，`origin "原著"` |

- **完成压过失败**（`Quests.cpp` 第 106–113 行先判 complete）。所以：Z1 的 `yihai.lua` **开头判 `ch12.guiwang` 已置 → 一句旁白（鬼王既灭、雾势已变，那具遗骸寻不着了）return、不置 `ch12.z1`**（触发器完成旗标由脚本置，return 不烧格子，`WorldScene.cpp` 第 32–65 行），否则玩家打完鬼王回头埋骨，任务会从「已过期」翻成「完成」；Z2 在 `cengjie.lua` 里 **`flag.set("ch12.z2")` 写在 `flag.set("ch12.cengjie")` 之前**——`kOffChainFlags` 的窗口是「`ch12.neidian` 之后、`ch12.cengjie` 之前」，`ch12.z1` 的窗口是「`ch12.tupo` 之后、`ch12.guiwang` 之前」。
- **为什么只有这两条**：本章他亲身在场、有前因后果、又不在主线上的旁枝只这两件；其余可问可看的都落在路径行动（16.3，13 条 inquire）与 `trigger_caiyao`（采药一次，不立任务）。**不凑数**。
- **不做成支线的**：寿元果（葛笠的事，他原路返回，A48）；文思月拒双修得罪高层、被派险差（他听出来了、没插手，B 表 12）；紫灵买图（只闯第一关，K6）；温夫人、万天明一行（他与之无交集）；补天丹（他没得，A105）。
- **配额**（当前账面，2026-10-13 读 `data/quests/`）：第 5–11 章共 14 份，`kind` 全是 `side`；各章施工图第 6 节里明标「必经」的只有第 10 章 Z2 `q10_wunian` 一条，第 5–9 章未标、第 11 章 0 条。本章 2 可选、0 必经，到第 12 章末共 16 条（明标必经 1）。大纲第 237 行目标「45（必经 14）」：第 13–14 章还要补 29 条、其中必经 13 条——缺口太大，不是 Z1 进链（O-8，链变 34 步、只补 1 条必经）能填的。**处置**：本章不改 Z1；协调者在派第 13 章研究路之前，按现有 16 条重核大纲第 237 行的目标（或回写大纲、或把补量分到第 13–14 章施工图），结论写进 handoff。

---

## 7. 系统：残图门闸、三关玩法、依附极阴岛、曲魂离队、虚天鼎与乾蓝冰珠、多方混战

**先查了已有的，再定做多少**：物品门闸 `item.count` 与 `take` 返回值、`choice` 循环、`party.remove`（`api.lua` 第 292–321 行）、`today()` 与 `advance_days`（单次 ≤ 3650，本章单次 ≤ 90）、`teleport(map_id, x, y)`、`kScriptTransfers` 的五元写法（第 11 章回城阵先例）、道具 `castMagic`（`talisman_tianleizi` 先例）、编成的 `wave` / `faction "ally"` / `killable_by` / `charge` / `actions`、NPC `visible_flag` / `hidden_flag`、路径行动的 `when` / `until` / `reveal`。**本章不加任何 Lua 命令、不加存档字段（仍 v9）、不改 `src/`**；契约只有两条判据项：**E1 空队伍**、**E2 同一 role 先友后敌**（10.1）。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **残图门闸** | n07 `lidao.lua` 在 `flag.set` / `teleport` 之前判 `item.count("story_jinpa_ditu") >= 1`；不足一支照写（一句旁白后 return、不送、留在荒岛），**数据上不可达**——第 8 章 `shadan.lua` 给、第 9–11 章脚本 0 处 `take`、本章 0 处 `take`。文案称它「虚天残图」（物品名不改）。「醒」只在 n06：玄骨肋骨里取出同样一块，他的锦帕现金剑、恒指西北（A27–A28），置 `ch12.canyuan`；n07 起「循图」 | 不做「收集残图」、不新给残图、不 `take` 残图；不做 portal 门闸；**不把门闸放在单向落点上**（大厅无门无出口，n08 判不足等于把人关死，16.1 第 44 条）| 原著一人一图（A33）、各凭手中那一块入殿（A32）；大纲「遗址取残图」与原著相反（B 表 1，回写大纲）|
| **第一关·鬼冤之地＝迷路** | 三处岔口、三条错路；错路尽头 `trigger_zoupian_a/b/c`（enter、once 否、guard `ch12.tupo`、无 set_flag）→ 各自的 `zoupian_a/b/c.lua`：开头判 `ch12.guiwang` 已置则 return，否则一句旁白（鬼雾把他推回）＋ `teleport("ch12_guiwu", x, y)` 回**那一处**岔口（三份脚本各写死一组坐标，各 0 处 `flag.set`）；正路由白骨与淡雾标出，K9 葛笠给提示；第三岔之后 ①（16.1 第 10、11 条）| 不做随机迷宫、不计步数、不做雾中遭遇、走错不扣血不扣日 | 原著是跟着啼魂兽开出的、比四周雾色淡得多的丈许宽通道走（ch438）；「偏了被鬼雾推回」是**游戏改编**，原著没有这一机制；做成战斗则与 ① 重复，做成纯对白则地图无用 |
| **第二关·冰火道＝限时** | n20 `flag.set("ch12.binghuo_ri", today())`；草地、沼泽两处「罗烟步（不耗日）/ 常速（＋1 日）」；n21 开头算 `today() − ch12.binghuo_ri`（只决定台词档；≥ 4 日——玩家在打坐设施静养过——一律走「时限将尽」档），绕路一支说「至少多耽误两日、五日内过不去」挡回（A72）；固定段 2 日 ＋ 选择至多 2 日 ＝ **至多 4 日，永不超时**；禁飞只在旁白（A69）；② 是「耐久」：三波、波间不回血不回法（16.1 第 12、13 条）| 不做超时致死、不做屏幕计时器、不做「法力当燃料」的走路消耗 | 「五日」（A71）要落在玩家手上——两处抉择让他看见日子在走；真做超时就是死局。法力的紧，落在 ② 波间不回 |
| **石殿阁楼（宝光阁）＝取舍**（宝光阁不算一关：它是第二关后石殿四条通道里三座阁楼之一，取宝后才被传去第三关，ch467）| 一层玉台 `trigger_yutai` 只给描述、不给物；去二层的简易传送阵 `take("material_lingshi", 4)`，不成扣中阶 1，再不成「以阵法心得推演半日」`advance_days(1)` 照走；二层先取一件（血色披风 / 五色铜环二选一，记 `ch12.quba_xian` 1 / 2），再选「噬金虫结甲」取另一件——**两件都到手**（A83–A84，16.1 第 14 条）| 不做「真的只许一件」、不做背包上限、不做抢先竞速 | 只给一件会让 ③（铜环套狼，A101）或第 13 章（披风逃命）断线；「取舍」的分量在规矩（每人一样，A78）与他破规矩的那一下 |
| **第三关·极妙幻境＝心境** | n25 两段抉择（色：长廊白云仙音；惧：黑殿里自称小妹的幻象，A85–A86）；错选只换一句旁白 ＋ 婆罗珠灼热提示（A87）、再选一次，**不失败**；一次就选对的段置 `ch12.xinjing` 对应位（位 1 色、位 2 惧），只供 n33 章末一句（16.1 第 15 条）| 不做失败分支、不做回档点、不做心魔数值 | 55 分钟装不下第二条路；心境是「他怎么走过去」的质感，不是关卡 |
| **依附极阴岛** | n18（挂 NPC `npc_jiyin`）：黑云、蛛丝（A62）、放出蜘蛛（`story_xueyu_zhizhu` **不扣**）、青易点破「取鼎最佳灵兽」（A63）；三方传音——三选一「拜极阴 / 拜青易 / 两边都不拜」，后两项各接对方一句传音再回到同一选择，只有拜极阴往下走；玄骨那条传音劝他拜，理由是九曲灵参的丹方只在玄骨手里（A65）；`give("story_xuanyin_huan", 1)`；此后极阴一行在 binghuo、neidian 群像里，路径行动 0 条挂他（16.1 第 16 条）| 不做阵营声望、不做「拜青易」的另一条线、记名弟子不给任何特权 | 极阴图的是蜘蛛、不是他（B 表 5）；拜师是被三方夹着点的头。n32 噬金虫除去追踪暗记（A115），第 13 章不必再演追杀 |
| **曲魂离队与队伍变化** | n05 `party.remove("qu_hun_shadan")` 看返回值，false 只记一句照走；`qu_hun_shadan.json` 一个字不动；夺舍后的躯体是新 role `xuangu_shaonian` → `xuan_gu`；抉择「敲引魂钟」必落空、钟不扣（A22）；**此后队伍为空到章末**；临时同伴只写死在编成里（① 紫灵 ＋ 黑袍人、② 元瑶、③ 乌丑 ＋ 玄骨）；本章 `party.add` 0 处（16.1 第 5、6 条）| 不做临时入队、不做分身复活或重炼、不在菜单队伍页留空位提示 | 曲魂之躯在 ch493 随玄骨化灰，只许旁白提；`party.add` 临时同伴要配对 `remove`，存档留人会弄脏第 13 章接口。空队伍本身第 5–7 章已跑过（`ch05`–`ch07-end` 的 `party` 都是 `[]`，`scripts/ch05/dingji.lua` 第 59 行）；本章新的是「曾有队员、章中途离队，接着进带写死友军的仗」，判据收窄见契约 E1 |
| **噬金虫与辟邪神雷**（两样新手段）| n03 `magic.learn("magic_shijin_chongqun")` 看返回值（金，`boost hits`，A9）；`story_shijin_chong` 不动。神雷做道具施法：`talisman_bixie_shenlei`（kind `talisman`、`castMagic magic_bixie_shenlei`〔木〕、`tradeable false`、`price 0`），n13、n31 战前各 `give` 2、战后 `take` 余数（ch444 耗尽、ch494 荡然无存）；全仓 `magic.learn("magic_bixie_shenlei")` 0 处（16.1 第 19、21 条）| 神雷不做常驻法术；不做剑阵 / 分光法术，「九柄」只在文案（A20）| 原著 ①②④ 的主手段都是噬金虫与神雷；神雷是剑上攒的那一点，打完就没了 |
| **虚天鼎与乾蓝冰珠** | 全是 story：n31 `give` 丹方玉简、虚天鼎（A109–A110）；④ 后 `give` 乾蓝冰珠、玄骨肋骨、金雷竹小箭（A113–A114）；n29 ③ 后 `give` 狼首玉如意；n33 `give` 万年灵乳、养魂木根须；章末一句「鼎盖如生铸、打不开」（A121）、玉如意里的器灵不受控（A122）| **不开鼎**（ch641 才清点）；冰珠不做药效（第 14 章 ch544 破结丹后期用）；灵乳不做回法；玉如意不给 weapon / castMagic；九曲灵参不建配方（16.1 第 20、31 条）；**补天丹不给**（落入蛮胡子手中，A105）；青火雷不给（元瑶留着自用，A75）| 本章拿到的全是「以后的东西」：结丹初期打不开、用不上；做成可用道具会被第 13–14 章当手段重复消耗 |
| **多方混战的编成** | **不加第三阵营**：③ 敌方只有狼形器灵两波，友军写死乌丑、玄骨——他们是**争夺者**，伤害照算、各抢各的；第 1 波银形 `killable_by ["金"]`，**两名友军不配金类法术**，所以只有玩家的金（五色铜环的文案、祭金蚨、噬金虫）要得了命＝「抢在他们之前收服」；元婴大战只在 ③ 开场白、蓄势文案与 n29 / n30 旁白（不点名谁胜）；胜利文案「收服」，战后两人各自恼怒（16.1 第 17 条）| 不做三方阵营、不做 AI 互殴、不做「抢夺计分」；元婴们不进任何编成 | 引擎只有 ally / enemy（`BattleLoader.cpp` 第 106 行），加第三阵营是 `BattleState` 与 AI 的大项；原著他在元婴大战里只能退后（ch482），他能抢的就是逃下台的那只狼。`xuan_gu` 在 ③ 是友军、在 ④ 是敌人，`wu_chou` 在 ③ 是友军——友军是否忽略 `toughness / weaknesses / killable_by / charge` 没有用例钉着，要判据（契约 E2）|

**队伍与手段对照**（写测试时读表，不抄数；供第 13 章参考）：

| | 起点（ch11-end，估，笔记 C.3）| n05 之后 | ①④ 战中 | 章末 |
| --- | --- | --- | --- | --- |
| 境界 / 上限 / `formerRealm` | 31 / 31 / 22 | 同左 | 同左 | 31 / 31 / 22（`realm.*` 0 处）|
| 队伍 | `[qu_hun_shadan]` | 空 | 空（友军写死在编成）| **空** |
| 法术 | 第 10 章那 11 门 | 12 门（n03 已学噬金虫）| 12 门 ＋ 神雷（道具）| 12 门 |
| `talisman_bixie_shenlei` | 0 | 0 | 2（战前给）| 0（战后收尽）|
| 入殿凭据 | `story_jinpa_ditu` ×1 | 同左 | 同左 | 同左（不扣）|

**走偏脚本（已拍板，16.1 第 39 条）**：拆成 `zoupian_a.lua` / `zoupian_b.lua` / `zoupian_c.lua` 三份（内容只差坐标），`kScriptTransfers` 三条五元各写各的脚本名——Lua 侧拿不到是哪个触发器起的事件、也拿不到玩家坐标（`ScriptHost.cpp` 第 250–329 行），一份脚本只能把三条错路送回同一点。笔记 N.4、E 表 10 与契约第 4、5.3 节已照此改；旗标与挂点不变。

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** ＝ 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **4 场必打**（① n13 鬼王、② n21 铁火蚁、③ n29 狼形器灵、④ n31 玄骨），四场都是原著的仗；**可选：无**（无遭遇区、殿内禁斗、无切磋）。
账：方案全作 52 场，第 1–11 章合计 41 场，**本章 4 场**，余 **7 场**给第 13 章（低强度，约 2）与第 14 章（约 5）。遗址看守怪人（ch419–421）做旁白；若第 14 章用不满 5 场，可升格为本章第 5 场，id 预留 `b12_tushan_guairen`（16.2 O-1）。

**这四场就是「独行」的说明书**（曲魂 n05 离队之后，队伍为空，第 7 节）：

| # | 谁上场 | 他与谁并肩 |
| --- | --- | --- |
| ① | 韩立 ＋ 紫灵 ＋ 黑袍人（写死友军）| **临时同伴**：雾里三人议定同行，各出其力 |
| ② | 韩立 ＋ 元瑶（写死友军）| **受托护送**：她在光团里求救，他开价啼魂兽 |
| ③ | 韩立 ＋ 乌丑 ＋ 玄骨（写死友军）| **争夺者**：同一只狼三个人抢，元婴在头顶打 |
| ④ | 韩立一人 | **独战**：第 1–11 章首领战都有队友或友军，这是他第一次真正一个人 |

**友军由玩家操纵**（`BattleScene.cpp` 第 974 行起 `runToAllyTurn`：轮到 ally 就停给玩家）。所以 ③ 的「争夺」只在文案；机制上靠「友军不配金类法术」把收服留给韩立自己（7 节末行、8.2 ③）。

### 8.1 意图玩家

- **共同**：结丹初期（700 / 470，攻 62、防 46——笔记 C.2，`Realm.cpp` 注释「暂定」）；全章 31 / 31，修为给了也冲不了关（`BreakthroughBlock` 照第 11 章口径回绝）。法术 12 门：第 10 章章末那 11 门（火弹术、流沙术、冰冻术、天眼术、御风决、祭金蚨子母刃、祭青蛟旗、青元剑芒、祭青火瘴、祭小刀符宝、祭红线遁光针——`ch10-design.md` 8.1 ③；**以 ch11-end 真档为准**，笔记 C.3 标「必须核」）＋ n03 学的噬金虫群（金）。兵刃：剑（`weapon_qingzhu_fengyun_jian`，一件代表「九柄」）、拳、暗器（`weapon_wuming_sixian`）。道具：中阶灵石约 20（估，回法力）、①④ 战前各给辟邪神雷 2 枚（木）。**队伍为空**。
- **①**：刚过 n12–n13 的迷路，满状态进场（存档点在土坡）。友军紫灵（筑基后期，剑）、黑袍人（结丹初期，火）。
- **②**：n20 之后、n21 一段旁白（探路者殒命、绕路被挡）之后进场；**三波之间不回血不回法**（引擎既有）。友军元瑶（结丹初期，火：青火雷）。
- **③**：n27–n29 一路演出、无战斗；`ch12_hanlitai` 台下有存档点与打坐设施。友军乌丑（结丹中期，`wu_chou` 900 / 78 / 62，水：冰箭术）、玄骨（结丹后期档，火：魔火 `magic_moyan_huo`，16.1 第 40 条）。
- **④**：紧接 n30 演出（他被斩飞、皇鳞甲救命，A104）与 n31（乌丑倒下、交出丹方、收鼎），**意图玩家 ③ 之后不回血**——平衡路按「刚打完 ③」量 ④（第 11 章 ③④ 同口径）；台下打坐设施是兜底（n30 之后可走回静养）。无友军。
- **最低合法起点**（流程第 5 条「任何合法消耗之后仍走得下去」）：中阶灵石 0、无丹药、上一场打完剩多少就带多少；回血只有 `advance_days` 一成一日与四处打坐设施（殿内单向、无店）。②③④ 之前各有一处打坐设施（冰火道光壁、塔前石室、寒骊台下），所以不会有「存档在 ② 前、法力气血都不够、重读也过不去」的死局；平衡路 B1 必须把这一种也量过（10.2）。

### 8.2 四张必打编成

| # | 编成 id | 敌方（按波次）| 我方 / 友军 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b12_guiwu_guiwang`（新）| **第 0 波** 炜吾 ×2（`weiwu_hanggui`，筑基后期；破绽火、剑）、鬼夜叉 ×1（`gui_yecha`，结丹初期；破绽木、火）；**第 1 波** 灰影 ×1（`hui_ying`，结丹中期；金、火）、双头怪虎 ×1（`shuangtou_guihu`，结丹中期；剑、木）；**第 2 波** 三首鬼物 ×1（`sanshou_guiwu`，结丹后期，首领；破绽木、火，`killable_by ["木","火"]`；蓄势「怪笑」`all true`）| 韩立 ＋ `zi_ling_xutian`、`yuan_yao_heipao` | 否 | **致命** | **越打越大**：第一波交给友军、自己省法；第二波噬金虫咬灰影、剑与剑芒对怪虎；第三波首领攒怪笑（全体）——**破势打断蓄势**，神雷两发收尾。破绽全靠战中试探（K8、K9 不设 reveal：reveal 提示会拼出 role 名牌，早于 `guiwang.*`，16.1 第 41 条）|
| ② | `b12_heishamo_tiehuoyi`（新）| **第 0 波** 铁火蚁群 ×2（`tiehuo_yiqun`，结丹初期；破绽水、金，`killable_by ["水","金"]`）；**第 1 波** 铁火蚁群 ×3；**第 2 波** 铁火蚁大群 ×1（`tiehuo_yiqun_da`，结丹中期，首领；同上破绽与 killable_by；蓄势「黑炎」`all true`、`actions 2`）| 韩立 ＋ `yuan_yao_jiedan` | 否 | **致命** | **耐久**：剑、拳、火最多把蚁群压到 1，要命的只有冰冻术（水）与噬金虫（金）；三波不回法，玩家得把法力留到第二、三波；元瑶的火打不到破绽（蚁群破绽只有水、金），只分担伤害、把蚁群压到 1。原著「水、冰属性中阶以上法术可伤」（A73）就落在 `killable_by` 上；她并肩打大群是改编（A73–A75）|
| ③ | `b12_neidian_duoding`（**就地改写**）| **第 0 波** 狼形器灵 ×1（`langxing_qiling`，结丹中期，双头火形；破绽水、金，`killable_by` 不写）；**第 1 波** 狼形器灵 ×1（`langxing_qiling_yin`，结丹中期，独角银形，首领；破绽金，`killable_by ["金"]`；蓄势「银光弹」`all true`）| 韩立 ＋ `wu_chou`、`xuan_gu`（争夺者）| 否 | **致命** | **多方混战的缩影**：第 0 波归零＝火狼被缚、化银（文案）；第 1 波只有金要得了命（祭金蚨、噬金虫；文案写五色铜环套住）——**两名友军都不配金类法术**，收服只能是他。乌丑的水能削火形的架势，玄骨的火打不到破绽 |
| ④ | `b12_hanlitai_xuangu`（新）| 玄骨 ×1（`xuan_gu`，结丹后期档；破绽木，`killable_by ["木"]`；`magics` 只有魔火；蓄势「修罗圣火」只在 `charge.text_key`、`every 3`、`actions 2`）| 韩立一人 | 否 | **致命** | **独战首领**：他一回合动两次、每三回合攒一发圣火；剑与噬金虫只能压到 1，**只有木**（青元剑芒、神雷）要得了命；神雷用早了，圣火压上来就没有收尾的了。五色铜环套不住玄骨（旁门修士、修为远超祭出者，ch492），**不进破绽** |

- **必须守住的**（数值归平衡路，按 `tests/BattleHand.h` 那只手量；本表只写意图与破绽）：四张 `hero_absent false`、`can_escape false`、`defeat_is_fatal true`、`rewards.spirit_stones 0`、`drops []`（物品一律由脚本 `give`）、`rewards.cultivation > 0`；① 三波、友军恰两名、首领 `killable_by` 恰「木、火」；② 三波、友军恰 `yuan_yao_jiedan`、两种蚁群 `killable_by` 恰「水、金」、大群 `actions 2`；③ 两波、友军恰 `wu_chou` 与 `xuan_gu`、**两名友军的 `magics` 里没有金类**、银形 `killable_by` 恰「金」；④ 无友军、`killable_by` 恰「木」、`charge.every 3`、`actions 2`。`killable_by` 一律非空且 ⊆ `weaknesses`；胜负只看气血归零。
- **E2 判据**（契约）：`xuan_gu` 在 ③ 做 ally、在 ④ 做 enemy，`wu_chou` 在 ③ 做 ally——ally 不吃 `toughness / weaknesses / killable_by / charge`、可被敌方打倒不报错；enemy 照常生效（16.2 O-4）。
- **E1 判据**（契约）：空队伍下四场进场都只有韩立 ＋ 编成里写死的友军；战后 `units[0]` 回写韩立气血（`BattleScene.cpp` 第 996 行）不越界。
- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，C2）：`CHAPTER_MEANS[12]` ＝ 拳（hand）、火（`magic_huodan_shu`）、土（`magic_liusha_shu`）、水（`magic_bingdong_shu`）、金（`magic_ji_jinfu`；外加 n03 学的 `magic_shijin_chongqun`）、木（`magic_qingyuan_jianmang`）、暗器（`weapon_wuming_sixian`）、剑（`weapon_lieyang_jian`，第 6 章）；`BATTLE_EXTRA_MEANS`：① `("木","give","talisman_bixie_shenlei","ch12")`、`("剑","role_weapon","剑","zi_ling_xutian")`、`("火","role_magic","magic_moyan_huo","yuan_yao_heipao")`；② `("火","role_magic","magic_qinghuo_lei","yuan_yao_jiedan")`；③ `("水","role_magic","magic_bingjian_shu","wu_chou")`、`("火","role_magic","magic_moyan_huo","xuan_gu")`；④ `("木","give","talisman_bixie_shenlei","ch12")`；`MEANS_LAST_CHAPTER = 12`。登之前核 `role_means_available`（`validate.py` 第 2475–2498 行）：`zi_ling_xutian.weapons` 含「剑」；两门 `role_magic` 都在该 role 的 `magics` 里、`element` 对得上、`power > 0`、无 `effect`、`needRealm` ≤ 该 role 境界、`needMp` ≤ 该 role 的 `maxMp`（魔火：火、12 / 12、`FoundationEarly`、无 effect，三名友军都满足）；selftest 探针挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`、`probe["chapter"] = 12`（16.1 第 25 条）。
- **战斗背景**（`data/visual/battles.json`，都是现有名）：① `mountain_forest_dusk`、② `cliff_top`、③④ `secret_room`；每条都要指得出读它的那一行代码、有走真实入口的测试（handoff 第 6 节第 5 条；验收 26）。
- **开场白与文案**（`data/text/ch12_battle.json`）：`ch12.battle.<短名>.intro`、蓄势 `ch12.battle.<短名>.charge`；① 蓄势文案用「附灵术」（M 表 442）；③ 开场白写头顶元婴混战、**不点名谁胜**；四场胜利文案：① 鬼物飞灰、② 蚁群退散、③ 「收服」、④ 「圣火反噬」（**不写「杀死玄骨」**：肉身是圣火焚尽、他只封了去路，残魂是 n32 他令啼魂兽吞灭——四字会把这两步并成一句他杀，B 表 3）。旧的两条 intro 与 `b12_xutian_hudian_kuilei` 由协调者删（16.2 O-7）。
- **修罗圣火（已拍板，16.1 第 40 条）**：原著到 ch491–492 才成（`novel.py find 修罗` 核过；ch492 是他逼急了才强行合成），③ 是 ch483–485——`xuan_gu` 的 `magics` 若放这门法术，玩家操纵 ③ 的玄骨时技能栏就先露了它。所以 **`xuan_gu` 的 `magics` 只放 `magic_moyan_huo`（魔火，现有）**，④ 的修罗圣火只走 `charge.text_key`（蓄势文案点名，编在 `xuangu.*`）；`magic_xiuluo_shenghuo` **不建**（L.2 新法术 4 → 3）；`BATTLE_EXTRA_MEANS` ③ 那一条是魔火。契约 2.1、2.2、5.2 与笔记 I、L.2 已照此改；16.2 原第 4 条的「开例外」方案作废。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 鬼王 | ch437：曾有一队遇神智大开的鬼王、整队泯灭仅一人逃出（A47）；ch440–444：勾魂鬼音、鬼夜叉、炜吾、双头怪虎接连扑来，最后三首鬼物现身，神雷耗尽、鬼物化灰 | 三波：悍鬼与鬼夜叉 → 灰影与双头怪虎 → 三首鬼物；神雷战前给、战后收 | 第 11 章 ④ 妖尸是「困而不杀」，本场鬼物可杀；前十一章没有鬼物首领。飞剑「四化八、散去四」是分化之影（B 表 9），**不写损剑** |
| ② 铁火蚁 | ch462–466：铁火蚁不惧大部分法术，水、冰属性中阶以上法术伤得了；万只级大群是他遇元瑶之前独自以噬金虫打的（ch463）；他解光团之围、开价要啼魂兽，她当场交兽吐珠（ch464），同行后只遇小群（ch466）| 一场、三波、不可逃；谈妥即 `give` 啼魂兽、鸣魂珠（**战前**）；元瑶并肩打大群是改编 | 第 11 章 ① 诱妖可重复、可逃；本场一次、不可逃；「近万」用三波群体单位表达 |
| ③ 狼形器灵 | ch483–485：鼎中古宝化双头火狼破鼎、逃向结丹修士处；他以花篮与五枚连串的铜环收服化银的独角狼，得玉如意；头顶元婴混战 | 两波；友军是同在场的两个争夺者；胜利＝收服 | 前十一章没有「收服」战；本作第一场抢东西的仗。元婴大战他在原著里只能退后（ch482），**不进编成** |
| ④ 玄骨 | ch491–493：玄骨融冰焰为修罗圣火要他死，他以剑芒封路、以神雷克火，圣火失控反噬，玄骨化为灰烬 | 独战；破绽只有木；胜利文案「圣火反噬」| 原著玄骨是**自己的火失控**、被他封住去路焚尽（B 表 3）——「气血归零」在文案里落成反噬；不写成他一剑杀了玄骨（残魂在 n32 才被吞灭）；元婴级敌人可写灰烬（16.1 第 22 条）|
| 看守怪人 | ch419–421：噬金虫数万当众吞食 | 旁白（n03，抉择两支都以噬金虫收场）| 一边倒，做成仗只剩按一下；见 O-1 |
| 小石山绿影 | ch452：扑来被金色电弧击碎 | 旁白（n17）| 一击之事 |
| 毒龙会三人 | ch430：三道青元剑芒 | 旁白（n07）| 一瞬间 |
| 内殿傀儡 | ch475–476：蛮胡子赤手砸碎 | 旁白 ＋ Z2 | 他没出手 |
| 乌丑之死 | ch489：死于玄骨鬼爪 | 旁白（n31），只写倒下 | 他在旁未出手（B 表 4）|
| 翅恶、妖冠蛇 | ch418、ch424 | n02 布水旗、n04 旁白 | 解谜与演出，不进编成 |

**不做的仗**：正魔元婴大战（他退后）、卜绪与艳女闯关、星宫二人密谈（他不在场，16.1 第 23 条）。

### 8.4 野外遭遇

**本章不放遭遇区**：荒岛无灵脉、草木不生（A3），地洞与遗址里能动的东西都在主线演出里；进殿之后一路是禁制、关卡与正魔元婴，殿内禁斗（A39），关卡里的敌人（鬼物、铁火蚁、器灵）都是剧情的。遭遇表的掉落又只许灵草——殿内采药只有小空间一天（`trigger_caiyao`）。修为的来路是四场战斗的 `rewards.cultivation`，本章冲不了关，不需要刷。

---

## 9. 经济账

**四本账：灵石（`material_lingshi`、`material_lingshi_zhong`）、辟邪神雷（`talisman_bixie_shenlei`）、story 物品、材料与灵草**。本章 **0 店、0 配方、0 门闸要钱**（殿内不卖不买，原著无坊市；16.1 第 28 条）。起点全部是「估」（笔记 C.3：`tests/fixtures/ch11-end-*.sav` 尚未生成），下面只写本章的去向与恒等式，不先宣称终局数。

**来源**

| 项 | 低阶灵石 | 中阶灵石 | 神雷 | 其他 |
| --- | --- | --- | --- | --- |
| 起点（ch11-end，**估**）| 数百到四千，两侧差 20 | 20 / 20 | 0 | 残图、白珍珠、噬金虫、血玉蜘蛛、引魂钟、青竹蜂云剑各 1（**必须拿真档核**）|
| n03 怪人 | | | | 花篮古宝、红光画轴（`story_zhiyang_huazhou`）|
| n13 鬼王（战前）| | | ＋2 | |
| Z1 埋骨（取衣一支）| | | | 辟火宝衣 |
| n16 捕参 | | | | 九曲灵参 |
| n18 拜师 | | | | 玄阴环 |
| n19 峡谷口 | | | | 白犀佩、寒冰珠（n23 转赠只改文案）|
| `trigger_caiyao` | | | | 现有灵草一种 ×2（id 由协调者从 `data/items/herbs/` 选，16.2 O-13）|
| n21 铁火蚁（战前，谈妥即交）| | | | 啼魂兽、鸣魂珠 |
| n22 晶柱 | | | | 炼晶 ×1（`tradeable false`）|
| n23 石殿 | | | | 婆罗珠、青冥针符宝、皇鳞甲 |
| n24 宝光阁 | | | | 血色披风、五色铜环 |
| n27 Z2（收）| | | | 傀儡残骸 |
| n28 台背 | | | | 内殿卷轴 |
| n29 夺鼎 | | | | 金丝蚕 ×3（讨要一支）、狼首玉如意（③ 后）|
| n31 玄骨 | | | ＋2（战前）| 丹方玉简、虚天鼎；④ 后乾蓝冰珠、玄骨肋骨、金雷竹小箭 |
| n33 灵泉 | | | | 万年灵乳、养魂木根须 |
| 四场战斗 | 0 | 0 | | 只给修为（`rewards.cultivation > 0`）|

**去向**

| 项 | 低阶灵石 | 中阶灵石 | 神雷 | 其他 |
| --- | --- | --- | --- | --- |
| n01 登岛 | | | | 白珍珠 −1（看返回值，0 也照走）|
| n05 夺舍 | | | | 红光画轴 −1（看返回值）；引魂钟**不扣**；曲魂离队 |
| n13 战后 | | | −余数 | |
| n24 简易传送阵 | −4（`take` 成则止）| −1（只在低阶不足时）| | 再不成：`advance_days(1)` 照走 |
| n30 化身符 | | | | 一只血玉蜘蛛两截——**不扣**（一件代表一对，A103、A107）|
| n31 战后 | | | −余数 | |
| n32 密室传送阵 | −4（同 n24）| −1（同 n24）| | 同 n24 |

**账要对得上的几条**（交测试路，T 前缀；直接读 `scripts/ch12/*.lua` 与 `data/battles/b12_*.json`）：

1. **本章 `give("material_lingshi")` 0 处、`take("material_lingshi", 4)` 恰 2 处**（`quba.lua`、`canhun.lua`）；**`take("material_lingshi_zhong", 1)` 恰 2 处**，只在同一脚本里前一个 `take` 返回 false 时走；再不成一句「以阵法心得推演半日」＋ `advance_days(1)`。**没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训）。原著两处都是「往阵里装几块灵石」（ch469、ch496），数目原著没给，取 4。
2. **辟邪神雷**：`give("talisman_bixie_shenlei", 2)` 恰 2 处（`guiwang.lua`、`xuangu.lua`，都在 `battle()` 之前）；`take` 恰 2 处、数量取 `item.count("talisman_bixie_shenlei")` 的余数、看返回值（都在 `battle()` 之后）；**章末 0**；物品 `tradeable false`、`price 0`。全仓 `magic.learn("magic_bixie_shenlei")` 0 处。
3. **story 的 `take`** 恰 2 处：`story_bai_zhenzhu`（n01）、`story_zhiyang_huazhou`（n05）；`story_jinpa_ditu`、`story_yinhun_zhong`、`story_xueyu_zhizhu`、`story_shijin_chong` 全章 0 处 `take`。
4. **材料与灵草**：`give("material_lianjing", 1)` 恰 1 处（n22）；灵草 `give` 恰 1 处、数量 2（`trigger_caiyao`，once）；遭遇区 0（8.4）。
5. 四张必打编成 `rewards.spirit_stones` 都是 0、`drops` 都空、`rewards.cultivation` 都大于 0。
6. **本章 `shop(` 0 处**、新配方 0 张（九曲灵参丹方是 story 玉简，16.1 第 31 条）、`data/shops/ch12_*` 0 份。
7. **章末净增**（意图玩家：Z1 取衣、Z2 收、n29 讨要）：story 24 种（金丝蚕 3 件，其余各 1；画轴给了又收不计）、炼晶 1、灵草 2、白珍珠 −1、神雷 0；低阶灵石 **−8**、中阶 **0**。不讨要金丝蚕、不做 Z1 / Z2 时分别少对应一种，账照样闭合。
8. **两侧同账**：通关两侧走同一组选项（笔记 N.1 第 5 条那一串：n01 应允、n02 一次选对……n32 吞；Z1 取衣埋骨、Z2 收、采药 1 次）；章末 `day` − 起点 `day` ＝ **182**（字面量写死；笔记 N.2；兜底那一日不在意图玩家路上）；两侧低阶灵石差 ＝ 起点差、中阶同。起点若与估值不同（16.2 O-3），增量不变、起点按真档改。
9. **退路**：本章所有门闸都是旗标或必有物品（残图第 8 章给、0 处 `take`）；`party.remove` 返回 false 只记一句照走；白珍珠、画轴、两次神雷余数的 `take` 全看返回值——**没有一处会因物品或钱不够而卡死**。

**对照：原著这一段的量级**。虚天殿一行他没有赚一块灵石，花出去的也只是两次启阵的几块；他带出来的是一口打不开的鼎、一颗要到结丹后期才用得上的冰珠、一株凝婴用的灵参和它的丹方、一截藏着功法的肋骨——**全是以后的东西**。所以本章的账在物品不在灵石：24 种 story 本章一种都卖不掉（殿内无店），第 13–14 章按需取用（笔记 O.2）。

<!-- 第 12 章施工图（虚天殿）第三部分：第 10–17 节。写手三，2026-10-12。唯一依据 notes/ch12/research.md；笔记没有、另用 novel.py 核过的原著事实都注了章号。 -->

## 10. 引擎前置

### 10.1 清单

**编号对照**（全文只此一套）：**契约 E1–E2** 指研究笔记 D 表末两条（E1 空队伍、E2 同一 role 先友后敌；契约文件落地后以契约为准），由引擎路做、判据写进 `tests/Ch12EngineTests.cpp`，**都不改 `src/`**；本表其余项按谁做分前缀：**C** 内容路 / 协调者、**T** 测试路、**B** 平衡路；「不做」的几项不编号。本文别处写「E 表 N」的，一律指研究笔记 E 表（16.4 原表）的拍板编号；写「B 表 N」的指 1.2 表，都不是本表。

| # | 项 | 必须 / 可选 | 谁做 | 说明 |
| --- | --- | --- | --- | --- |
| 契约 E1 | **空队伍**：`party.remove("qu_hun_shadan")` 返回 true 之后 `party.size() == 0`；空队伍下地图行走、① 编成进场（只有韩立 ＋ 两名写死友军）、存档→读档、菜单队伍页都不崩 | **必须** | 引擎路 | 判据项，不改代码；任一处崩才改 `src/`，且先改研究笔记（O-2） |
| 契约 E2 | **同一 role 先友后敌**：`xuan_gu` 带 `toughness / weaknesses / killable_by / charge`，在 ③ 做 ally 时这四样不生效、可被敌方打到 0 不报错，在 ④ 做 enemy 时照常生效；`wu_chou` 在 ③ 做 ally 同此 | **必须** | 引擎路 | 判据项（O-4）；`unitFromRole(state, role, spec.ally)` 现有，只缺用例钉住 |
| C1 | **开局接法**：patch `scripts/ch11/zhuzhong.lua` 末尾——`flag.set("ch11.done")` 之后补一行 `teleport("ch12_tushan")`（只传图 id，落 default spawn `andeng`；此前一个字不动）；`data/chapters.json` 加 `{"number": 12, "numeral_key": "ui.chapter.12.numeral", "title_key": "ui.chapter.12.title", "done_flag": "ch12.done"}`；`ui.json` 加「第十二章」「虚天殿」 | **必须** | 内容路（一行）＋ 协调者（两份数据） | E 表 1、2；与 9→10、10→11 先例一致；第 11 章的图一个字不动 |
| T1 | `tests/fixtures/ch11-end-{first,second}.sav` 按 C1 patch 之后重生成：`mapId` 变 `ch12_tushan`、站在 `andeng` 那一格（落点不触发 n01，第 12 章通关驱动要先迈一步踩进 n01 触发带）；第 11 章验收 21 的章节卡断言「第十一章　终」接「未完待续」改成接「第十二章　虚天殿」 | **必须** | 测试路 | 照第 11 章 T1 做法；C.3 标「必须」的各项在这两份上逐项核（O-3） |
| C2 | 规则 25：`CHAPTER_MEANS[12]` ＝ 拳、火、土、水、金、木、暗器、剑；`BATTLE_EXTRA_MEANS` 照 8.2 末（① 神雷木 ＋ 紫灵剑 `role_weapon` ＋ 黑袍人魔火、② 元瑶青火雷、③ 乌丑冰箭术与玄骨魔火、④ 神雷木）；`MEANS_LAST_CHAPTER = 12`；**selftest 探针挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`**、`probe["chapter"] = 12` | **必须** | 协调者（集成时） | E 表 25；不挪则「同一乌丑移入第 11 章」那条红（乌丑已是友军） |
| C3 | `tests/LexiconTests.cpp` 表一补词（12.1） | **必须** | 协调者 | 放在第 11 章集成之后；`{12, 499}` 已在第 441 行 |
| T2 | `tests/ObjectiveTests.cpp` 的 `kScriptTransfers` 登 3.1 末表 12 条（三元 9、五元 3，五元三条各用 `zoupian_a/b/c.lua`）；`tests/NpcPresenceTests.cpp` 的 `kOffChainFlags` 登第 11 节链外 8 面 | **必须** | 测试路 | O-5：走偏五元是第一次「同图带坐标回岔口」 |
| T4 | 契约 5.3 的另外四处：`UiStyleTests` 的 `ChapterTableData` / `ChapterCards`（表长 12；第 232 行「`ch12.done` 不排卡」的断言改成 `ch13.done`）、`PathActionTests` 的 `realmCapOfChapter()` 加 `{12, Realm::CoreEarly}`、`NpcPresenceTests` 的 `kLastChapter`（第 133 行，现为 11）改 12、`tests/ChapterFixture.h` 加 `ch12-end` 两份 | **必须** | 测试路 | 契约 5.3 逐行 |
| C4 | 数据：角色新建 32、删 2（`xutian_hudian_kuilei`、`duoding_xiushi`）、一个字不动 4（`qu_hun_shadan`、`jin_qing`、`wu_chou`、`miao_zhanglao`）；编成 `b12_neidian_duoding` 就地改写成 ③ ＋ 新建 3；法术新建 3（不建 `magic_xiuluo_shenghuo`）；物品新建 27；任务 2；路径行动 13；地图 10（旧图 0 张修补）；配方 0、商店 0 | **必须** | 内容路 | 清单在笔记 H、I、L、G、K、N.3；物品描述受第 12 节约束 |
| C5 | `data/visual/maps.json` 十张图（只用现有组合）、`data/visual/battles.json` 四场背景（① `mountain_forest_dusk`、② `cliff_top`、③④ `secret_room`）；开场白、蓄势、胜利文案写 `data/text/ch12_battle.json`；**删占位**：`looks.json` 两条归内容路 A，`b12_xutian_hudian_kuilei`、两个 role、`data/text/battles.json` 两条旧 intro 归协调者，同一批落 | **必须** | 内容路 A（`looks.json`）＋ 协调者（其余占位） | E 表 18；O-7：删在第 11 章集成之后，免得撞车 |
| C6 | `data/flags.json` 登第 11 节 41 面（不含 `ch12.path.*`） | **必须** | 协调者 | 施工方不碰 |
| C7 | 回写：大纲、lore、人物总表（笔记 O.4 全部）；`wu_chou.json` 的 note（O-11）与第 11 章文案「半幅」（O-12）在两章都集成后由协调者处理 | **必须** | 协调者 | B 表 1–20 |
| B1 | **量法标定**：先跑 `BattleHand` 自检（同一起点多次结果一致，与第 11 章同口径），再用 `tests/BattleHand.h` 量「结丹初期单人 ＋ 临时友军对结丹后期首领」四场，两种起点：意图玩家（两侧）与**最低合法起点**（8.1 末条）；`Realm.cpp` 锚点数一个不动 | **必须** | 平衡路（B、C 两路都收尾之后串行接手，契约 6.3）| C.2；本章不再标定结丹初期 |
| T3 | 通关测试 `Ch12Walkthrough`：起点 T1 两份、终点写 `tests/fixtures/ch12-end-{first,second}.sav`（v9，不加存档字段）；`Ch12TriggerModeTests` / `Ch12BattleDataTests` / `Ch12LedgerTests` / `Ch12SliceTests`（**不含 `Ch12EngineTests`**，归引擎路） | **必须** | 测试路（**另一个代理**） | 第 15 节 |
| 不做 | 第三阵营（多方混战） | 可选 | — | **不做**：要动 `BattleState` 与 AI；争夺者当友军（E 表 17） |
| 不做 | 冰火道超时致死 / 飞行与禁飞机制 | 可选 | — | **不做**：日历计时永远不超时（E 表 12）；禁飞只在旁白（A69） |
| 不做 | 看守怪人升格第 5 场 | 可选 | — | **不做**：预留 id `b12_tushan_guairen`（O-1） |
| 不做 | 玉如意做兵刃 / 器灵受控 | 可选 | — | **不做**：只作 story（E 表 20） |
| 不做 | 剑阵、分光一类新法术；九曲灵参配方 | 可选 | — | **不做**：「九柄」只在文案（A20）；配方无辅药可定（E 表 31） |
| — | 残图收集、殿内商店、遭遇区、小瓶、面板冲结丹中期、名牌 realm 显示「？」 | **不做** | — | E 表 3、28、7；O-9 |

**查过、本章不需要新做的**：`party.add / remove / size` 看返回值；`teleport(map_id, x, y)`（不认 spawn 名，`api.lua` 第 63 行）；`advance_days` 单次 ≤ 3650（本章单次 ≤ 90）；`choice` 的选项数组由脚本按条件拼、取消为 nil；`item.count` 门闸、`take` 看返回值；NPC `visible_flag / hidden_flag`；portal / facility 的 `require_flag ＋ deny_text_key`（它们不读 visible / hidden）；多波次与波间不回血不回法；`charge{every,mult,all,text_key}` 与 `actions`；`killable_by` 非空（加载器 `DataLoader.cpp` 第 531–535 行与规则 24 都拒空数组；**⊆ `weaknesses` 两处都不查**，只靠本章专项断言——契约 E2 第 4 条）；道具施法 `castMagic`（`talisman_tianleizi` 先例）；`flag.set(name, today())` 存整数日期（`ch10.kaifu_ri` 先例）；路径行动 `when / until / reveal`；章节卡与「未完待续」。

### 10.2 判据的要点

- **契约 E1**（以契约 1.3 为准，这里只是摘要）：夹具 `tests/fixtures/ch10-end-first.sav`、判据不依赖章节进度；离队（探针脚本 remove 一次 ok＝1、size＝0，再 remove 一次 ok＝0 不崩）；空队伍借 `b11_tiandu_shi` 建场 ally 恰 5 名、放回曲魂恰 6 名；存读档字节相同（带 1 名的负向对照）；菜单队伍页不崩（含 `no_such_role` 负向）；内容路落地后四张真编成各一条。空队伍本身第 5–7 章已跑过，E1 的新增价值在建场人数与菜单两条（契约 0 节）。真实入口版（演到 n05、空队伍打 ①）在验收 6 的切片里。
- **契约 E2**（以契约 2.3 为准）：`xuan_le` 先例两场对照（ally 那场 `maxToughness / weaknesses / revealed / chargeEvery / killableBy` 全 0、`actions == role.actions`；enemy 那场照抄）；直接构 `BattleState` 证友军打得死（负向：给友军 `killableBy` 后被锁在 1）；破绽记账只记敌方；真数据 `xuan_gu` / `wu_chou` / 银形狼与四张编成全部敌方 role 的 `killable_by ⊆ weaknesses`；③ 友军不配金。负向自检：去掉 `unitFromRole` 的 `if (!ally)` 守卫 → 第 1、3、4 条红。
- **C1**：第 11 章集成版 `zhuzhong.lua` 与 patch 后的 diff 只有末尾一行；headless `Application` 演完它，`mapId == ch12_tushan`、站在 `andeng`、`ch11.done == 1`、章节卡队列「第十一章　终」接「第十二章　虚天殿」；演完 `lingquan.lua` 则是「第十二章　终」接「未完待续」。**负向**：把那一行挪到 `flag.set` 之前 → 第 11 章验收 21 红。
- **C2**：`validate.py` 对四张编成「至少一样破绽是本章我方手段打得到的」全过（① 木 / 火、② 水 / 金、③ 金、④ 木）；探针用例把 `wuji_haishou.json` 临时写坏、`probe["chapter"]` 改 12 时报、改回不报。**负向**：`MEANS_LAST_CHAPTER` 留 11 → `b12_*` 四张全红。
- **T2**：`kScriptTransfers` 12 条逐条跑脚本到那一行，`mapId` 等于落点图；五元三条的触发器 `type trigger / mode enter / once false / guard_flag == "ch12.tupo" / set_flag` 空、`zoupian_a/b/c.lua` 各 0 处 `flag.set`；`kOffChainFlags` 8 面都不在 `data/objectives/ch12.json` 的 `done_flag` 序列里、却都在 `data/flags.json` 里。
- **B1**：先跑 `BattleHand` 自检（同一起点、同一手法连跑多次结果逐字段一致，与第 11 章同口径），再量四场：两侧意图起点——① 三波打得完、第三波怪笑能被破势打断；② 三波之间不回血不回法、剑拳火只压到 1；③ 第 1 波只有金要得了命、两名友军打不死它；④ 只有木要命、神雷 2 发 ＋ 青元剑芒收得了尾。**最低合法起点**（中阶灵石 0、无丹药、上一场剩余量；每场之前在最近的打坐设施静养到满）——②③④ 都打得过。数值改动落在内容路 B 的 role / battle 文件里，测量记录写进编成的 `note` 字段，由协调者并入第 18 节施工偏差（契约 6.3）。

---

## 11. 旗标清单（交协调者登记；施工方不碰 `data/flags.json`）

**「谁置」一律是脚本自己 `flag.set`**：引擎从不替触发器写 `set_flag`，它只拿来判 once 兑现了没有（`WorldScene.cpp` 第 32–65 行 `triggerReady` 注释）；下表「（＝触发器 set_flag）」只说明那个触发器的 `set_flag` 字段填的就是这面旗。脚本漏写 `flag.set`，once 触发器会反复响、目标链也走不下去。

| 旗标 | 值 | 谁置 | 谁读 |
| --- | --- | --- | --- |
| `ch12.dengdao` | 1 | n01 `dengdao.lua` 末尾 `flag.set`（＝ `trigger_dengdao` 的 set_flag）| n02 guard；荒岛四人 visible；K1–K3 `when` |
| `ch12.pozhen` | 1 | n02 `pozhen.lua` 末尾 `flag.set`（＝触发器 set_flag）| n03 guard；K1 `until` |
| `ch12.tuizhu` | 1 | n03 `guairen.lua` 末尾 `flag.set`（＝触发器 set_flag）| `portal_dixue` require_flag；n04 guard；荒岛四人 hidden、地洞四人 visible；K1–K3 `until` |
| `ch12.qixialian` | 1 | n04 `qixialian.lua` 末尾 `flag.set`（＝触发器 set_flag）| n05 guard；胡月、简姓修士、石蝶 hidden |
| `ch12.duoshe` | 1 | n05 `duoshe.lua` 末尾 `flag.set`（＝触发器 set_flag）| `npc_shaonian` visible；金青 hidden；n06 前置 |
| `ch12.canyuan` | 1 | n06 `xiaocha.lua` | `npc_shaonian` hidden；文氏父女 visible；n07 前置；K4 `when` |
| `ch12.lidao` | 1 | n07 `lidao.lua`（与 teleport 同在顶格） | 文氏 hidden；n08 guard；K4 `until`（锦帕门闸在它之前判，n07）|
| `ch12.rudian` | 1 | n08 `rudian.lua` 末尾 `flag.set`（＝触发器 set_flag）| 大厅玄骨、紫灵、苗长老 visible；n09 前置；K5、K6 `when` |
| `ch12.jiumeng` | 1 | n09 `jiumeng.lua` | n10 guard；K5 `when` |
| `ch12.qunxiang` | 1 | n10 `qunxiang.lua` 末尾 `flag.set`（＝触发器 set_flag）| 大厅群像 visible；n11 guard；K7 `when` |
| `ch12.chuansong` | 1 | n11 `chuansong.lua` 末尾 `flag.set`（＝触发器 set_flag）| 大厅全员 hidden；n12 guard；K5–K7 `until` |
| `ch12.tupo` | 1 | n12 `tupo.lua` 末尾 `flag.set`（＝触发器 set_flag）| 土坡紫灵、葛笠 visible；三处走偏 guard；`trigger_yihai`、n13 guard；K8、K9 `when` |
| `ch12.guiwang` | 1 | n13 `guiwang.lua` 末尾 `flag.set`（＝触发器 set_flag）| 土坡二人 hidden、花园紫灵 `npc_zi_ling_hy` visible；`zoupian_a/b/c.lua` 开头判；n14 guard；K8、K9 `until`；Z1 `fail` |
| `ch12.yuanyao` | 1 | n14 `yuanyao.lua` 末尾 `flag.set`（＝触发器 set_flag）| 花园群像（紫灵除外）visible；n15 guard |
| `ch12.xiaojie` | 1 | n15 `xiaojie.lua` 末尾 `flag.set`（＝触发器 set_flag）| 花园 hidden；小空间玄骨 visible；n16、`trigger_caiyao` guard；K10 `when` |
| `ch12.lingshen` | 1 | n16 `lingshen.lua` 末尾 `flag.set`（＝触发器 set_flag）| 玄骨 hidden；n17 guard；K10 `until` |
| `ch12.fanlian` | 1 | n17 `fanlian.lua` 末尾 `flag.set`（＝触发器 set_flag）| 石山极阴三人 visible；n18 前置 |
| `ch12.baishi` | 1 | n18 `baishi.lua` | 石山三人 hidden、峡谷口群像 visible；n19 guard；K11 `when` |
| `ch12.xiagu` | 1 | n19 `xiagu.lua` 末尾 `flag.set`（＝触发器 set_flag）| 峡谷口 hidden；n20 guard；K11 `until` |
| `ch12.rongyan` | 1 | n20 `rongyan.lua` 末尾 `flag.set`（＝触发器 set_flag）| n21 guard |
| `ch12.tihun` | 1 | n21 `tihun.lua` 末尾 `flag.set`（＝触发器 set_flag）| n22 guard |
| `ch12.lianjing` | 1 | n22 `lianjing.lua` 末尾 `flag.set`（＝触发器 set_flag）| 石殿群像 visible；n23 guard |
| `ch12.shidian` | 1 | n23 `shidian.lua` 末尾 `flag.set`（＝触发器 set_flag）| 石殿 hidden；n24 guard |
| `ch12.quba` | 1 | n24 `quba.lua` 末尾 `flag.set`（＝触发器 set_flag）| n25 guard |
| `ch12.huanjing` | 1 | n25 `huanjing.lua` 末尾 `flag.set`（＝触发器 set_flag）| 塔前群像 visible（`ch12_taqian`）；n26 guard；K12 `when` |
| `ch12.neidian` | 1 | n26 `neidian.lua` 末尾 `flag.set`（＝触发器 set_flag）| 塔前 hidden；n27 guard；K12 `until`；Z2 `accept` |
| `ch12.cengjie` | 1 | n27 `cengjie.lua` 末尾 `flag.set`（＝触发器 set_flag）| 寒骊台群像 visible（`ch12_hanlitai`）；n28 guard；K13 `when`；Z2 `fail` |
| `ch12.juanzhou` | 1 | n28 `juanzhou.lua` 末尾 `flag.set`（＝触发器 set_flag）| n29 guard |
| `ch12.ruyi` | 1 | n29 `ruyi.lua` 末尾 `flag.set`（＝触发器 set_flag）| n30 guard；K13 `until` |
| `ch12.butian` | 1 | n30 `butian.lua` 末尾 `flag.set`（＝触发器 set_flag）| 寒骊台元婴诸人 hidden；n31 guard；K13 `until` |
| `ch12.xuangu` | 1 | n31 `xuangu.lua` 末尾 `flag.set`（＝触发器 set_flag）| 乌丑、玄骨 hidden；n32 guard |
| `ch12.canhun` | 1 | n32 `canhun.lua` 末尾 `flag.set`（＝触发器 set_flag）| `npc_yuan_yao_lq` visible（`ch12_lingquan`）；n33 前置 |
| `ch12.done` | 1 | n33 `lingquan.lua` | 章节卡；全部路径行动 `until`；第 13 章开局 |
| `ch12.huanqi` | 1 用他的旗 / 2 换自备的旗 | n16 `lingshen.lua` | n17 `fanlian.lua` 多一句（`== 1`） |
| `ch12.binghuo_ri` | 整数日期（`today()` 的值） | n20 `rongyan.lua` | n21 `tihun.lua` 算已用日数 |
| `ch12.quba_xian` | 1 披风 / 2 铜环 | n24 `quba.lua` | n24 第二件的文案 |
| `ch12.xinjing` | 0–3（位掩码：色 1、惧 2；一次就对才置） | n25 `huanjing.lua` | n33 章末一句 |
| `ch12.jincan` | 0 / 1 | n29 `ruyi.lua` | 第 13 章（ch522 旁白） |
| `ch12.caiyao` | 1 | `caiyao.lua` 末尾 `flag.set`（＝ `trigger_caiyao` 的 set_flag）| 自身 once |
| `ch12.z1` | 1 | `yihai.lua` 末尾 `flag.set`（＝ `trigger_yihai` 的 set_flag）| `q12_maigu` complete |
| `ch12.z2` | 1 | n27 `cengjie.lua`（抉择「收」那一支） | `q12_kuilei_canhai` complete |
| `ch12.path.*` | | 路径行动 | 只由路径行动置（13 面，16.3；**等 `data/pathactions/ch12.json` 落地后登记**） |

链上 33 ＋ 链外 8 ＝ **41**；链外 8 面登 `kOffChainFlags`（T2）。不新增 `story.*`。**`ch12.binghuo_ri` 存的是日期**——登记说明写明「整数日期，不是开关；只由 `rongyan.lua` 写、`tihun.lua` 读」。`ch12.xinjing` 是位掩码，登记说明写明两位的含义。

---

## 12. 禁词与首见章号

### 12.1 交协调者：`tests/LexiconTests.cpp` 表一要补的词

笔记 M 表逐个 `novel.py find` 取最早命中章（2026-10-12）。已在表里的不重登（虚天殿、虚天残图 405；虚天鼎 434；九曲灵参 433；补天丹、六道极圣 435；啼魂 437；乾蓝冰焰 480；乾蓝冰珠 494；辟邪神雷 424；玄骨 428；蛮胡子 432；元瑶 375；文思月 430；极阴祖师、乌丑 382；青阳门 464；妖尸 408；紫灵仙子 403；金青、古修士遗址 415 等）。全部首见 ≤ 499，第 1–11 章禁、本章放行（规则按 `{12, 499}` 自动判）。

| 首见 | 词（83 个） |
| --- | --- |
| ch416-436（26） | 胡月、石蝶（416，名牌；2026-10-13 补登，`novel.py find` 首见 416，表一原无）；北斗两仪阵（417）；翅恶（418）；洪荒古宝（419）；封灵柱（421）；七霞莲、千年石乳（422）；妖冠蛇（424）；脂阳鸟（425）；极炫（426）；玄魂炼妖（427）；萧诧（428）；毒龙会（429）；南鹤岛、白壁山、温夫人、寿元果（432）；万天明、天悟子、天罗真功、托天诀（434）；金丝蚕、离火岛、火蟒、青易居士（435） |
| ch437-457（19） | 鬼冤之地、鬼王、啼魂兽、葛笠（437）；玉真人、辟火宝衣（438）；勾魂鬼音、鬼夜叉（440）；炜吾（441）；附灵术（442）；冰火道（447）；玄晶道、熔岩路、呲唬（448）；四象玄武阵（449）；麝兰兽（450）；托天魔功（455）；玄阴环、阴阳环（456） |
| ch458-478（26） | 白犀佩、天青花（458）；寒冰珠（459）；黑沙漠、极妙幻境（460）；灵犀佩、铁火蚁、天晶蚁（462）；鸣魂珠、青火雷（464）；银光鼠、血冰林（465）；炼晶、玄黄镜（466）；宝光阁、婆罗珠、青冥针、皇鳞甲（468）；五色铜环（469）；狼首傀儡（475）；蛇卫（476）；寒骊台、天罡罩（477）；寒蛟、离龟、青棘鸟（478） |
| ch479-499（12） | 狂暴之术（480）；化石兽（481）；三目豹（482）；天罗真雷、玉如意（484）；化身符、贯日剑（486）；修罗圣火（491）；五行环（492）；万年灵乳、养魂木（497）；血色披风（499） |
| **补登** | **紫罗极火（731）**：全 14 章禁用（大纲第 12 章注意） |

**笔记未列、本稿另核**（2026-10-12 `novel.py find` 取最早章；协调者定是否登表一）：

| 词 | 首见 | 为什么提 |
| --- | --- | --- |
| 红月岛 | 416 | **在用**：K2 `shidie_hongyue_dating` 的效果就是它 |
| 炎灵兽 | 460 | **在用**：n20 旁白（笔记 F 表） |
| 金雷竹小箭 | 485 | **在用**：物品 `story_jinlei_xiaojian` 的名字（n31 给） |
| 石真人、驱灵术、玄骨上人、天台岛、勾魂飞灵、疯婆子、天缘子 | 416 / 425 / 428 / 437 / 447 / 465 / 478 | 他在场时出口的名字；登表只为章级兜底，本章用不用见 12.2 |
| 三阳老魔、天都妖尸、藏魂匣、妍姐姐、三仙宗 | 474 / 482 / 498 / 498 / 499 | 本章 0 处（12.2 末行）；登表防第 1–11 章误写 |

**故意不进表的**（泛称，进表会误伤）：少年、怪影、怪人、黑袍人、绿影、狼形器灵、小空间、花园、石殿、灵泉、白兔、金网、小妹。它们由 12.2 按场景管。

### 12.2 规则放行、本章另有专项断言的

**断言的写法**同第 10、11 章：把 3.4 目标链的次序当场景次序，给每个文案 key 的场景前缀（`ch12.<场景>.*`，场景名 ＝ 节点脚本名，如 `xiaocha`）编号；「X 之前不含」＝编号小于 X 的前缀下 0 处。走偏（`zoupian.*`）与 Z1（`trigger_yihai` 的文案）编在 `tupo` 与 `guiwang` 之间；采药（`caiyao.*`）编在 `xiaojie` 与 `lingshen` 之间；路径行动文案按它 `when` 里最晚的链上旗标编号；物品描述按 `give` 它的脚本编号；战斗开场白、蓄势与胜利文案按调用它的脚本编号；地图名按第一次 teleport 进去的脚本编号，经 portal 首入的图按开门的脚本编号（`ch12_dixue` 编在 `guairen`）；目标链 R-3 提示（`objective.ch12.*`）按它所属那一步的上一步编号。**旗标 → 场景前缀**：done_flag 与脚本名不同的只有三处——`ch12.tuizhu` → `guairen`、`ch12.canyuan` → `xiaocha`、`ch12.done` → `lingquan`；其余 done_flag 去掉 `ch12.` 就是场景名。

| 词 | 首见 | 为什么规则管不住 | 本章的断言 |
| --- | --- | --- | --- |
| 北斗两仪阵、翅恶 | 417-418 | 章内先后 | `pozhen.*` 之前不含 |
| 洪荒古宝、封灵柱 | 419-421 | 章内先后 | `guairen.*` 之前不含 |
| 七霞莲、千年石乳、妖冠蛇、脂阳鸟 | 422-425 | 章内先后 | `qixialian.*` 之前不含——物品 `story_zhiyang_huazhou`（n03 `give`）的显示名因此定为「红光画轴」（ch421 原著叫法，16.1 第 42 条），n04 展开后由旁白点出脂阳鸟 |
| 极炫 | 426 | 只此一句 | 全章恰 1 处、在 `duoshe.*` |
| 玄魂炼妖 | 427 | 章内先后 | `duoshe.*` 之前不含 |
| 玄骨 | 428 | 名字（之前是「怪影」「少年」） | `xiaocha.*` 之前不含；`dixue_guiying`、`xuangu_shaonian` 的 `name` 不含「玄」 |
| 萧诧 | 428 | 只此一次、不上名牌 | 全章恰 1 处、在 `xiaocha.*`；任何 role 的 `name` 0 处 |
| 毒龙会、文思月、文樯 | 429 / 430 / 369 | 章内先后 | `xiaocha.*` 之前不含（K4 编在 `xiaocha`） |
| 九曲灵参 | 433 | 章内先后 | `jiumeng.*` 之前不含 |
| 南鹤岛、白壁山、温夫人、寿元果、万天明、天悟子、天罗真功、托天诀、金丝蚕、离火岛、火蟒、青易居士、**补天丹、虚天鼎** | 432-435 | 章内先后 | `qunxiang.*` 之前不含（青易居士此前只称「青兄」） |
| 鬼冤之地、鬼王、啼魂兽、葛笠 | 437 | 章内先后 | `tupo.*` 之前不含——`ch12_guiwu` 首次由 n11 `chuansong.lua` 送入，所以地图显示名用泛称「鬼雾」（16.1 第 46 条）|
| 玉真人、辟火宝衣 | 438 | 只在支线 | 只许在 Z1 的文案与 `rongyan.*` 那一句 |
| 勾魂鬼音、鬼夜叉、炜吾、附灵术 | 440-442 | 章内先后 | `guiwang.*` 之前不含；**K8、K9 的文案不含「鬼夜叉」「三首」，也不设 reveal**——引擎的 reveal 提示会拼出 role 名牌（`Application.cpp` 第 816–817 行 `speakerName(effect.reveal.roleId)`），在 `tupo` 阶段就显示「鬼夜叉」「三首鬼物」，`LexiconTests` 按 key 扫抓不到这句拼出来的字（16.1 第 41 条） |
| 元瑶 | 375 | 名字（之前是「黑袍人」） | `yuanyao.*` 之前不含；`yuan_yao_heipao` 的 `name` 不含「元」 |
| 冰火道 | 447 | 章内先后 | `xiaojie.*` 之前不含 |
| 呲唬、四象玄武阵、麝兰兽 | 448-450 | 章内先后 | `lingshen.*` 之前不含 |
| 玄阴环、阴阳环 | 456 | 章内先后 | `baishi.*` 之前不含 |
| 托天魔功、玄晶道、熔岩路、白犀佩、寒冰珠 | 448-459 | 章内先后 | `xiagu.*` 之前不含（`qunxiang.*` 只许「托天诀」）；「灵犀佩」全章 0 处（只用白犀佩） |
| 黑沙漠、铁火蚁、天晶蚁、鸣魂珠、青火雷 | 460-464 | 章内先后 | `tihun.*`（含 ② 开场白）之前不含 |
| 炼晶、玄黄镜 | 466 | 章内先后 | `lianjing.*` 之前不含 |
| 宝光阁、婆罗珠、青冥针、皇鳞甲、极妙幻境 | 460-468 | 章内先后 | `shidian.*` 之前不含 |
| 五色铜环 | 469 | 章内先后 | `quba.*` 之前不含 |
| 血色披风 | 499 | 章内先后；物品名放行 | 文案只许在 `lingquan.*`；`quba.*`、`huanjing.*` 写「暗红披风」；物品名 `story_xuese_pifeng` 例外（B 表 20） |
| 狼首傀儡、蛇卫、寒骊台 | 475-477 | 章内先后 | `cengjie.*` 之前不含 |
| 天罡罩 | 477 | 章内先后 | `juanzhou.*` 之前不含 |
| 天青花、寒蛟、离龟、青棘鸟、狂暴之术、化石兽、三目豹、玉如意、狼形器灵 | 458-484 | 章内先后 | `ruyi.*`（含 ③ 开场白）之前不含 |
| 天罗真雷、化身符、贯日剑 | 484-486 | 章内先后 | `butian.*` 之前不含 |
| 修罗圣火、五行环、乾蓝冰珠 | 491-494 | 章内先后 | `xuangu.*`（含 ④）之前不含——③ 友军玄骨只带魔火（16.1 第 40 条），③ 战斗界面不出这四字 |
| 万年灵乳、养魂木 | 497 | 章内先后 | `lingquan.*` 之前不含 |
| 自焚、杀死玄骨 | — | B 表 3：他不是自焚，肉身是圣火失控焚尽（他以剑芒封了去路），残魂是 n32 他令啼魂兽吞灭——「杀死玄骨」会把这两步并成一句他杀 | 全章 0 处（④ 胜利文案是「圣火反噬」；封路写在 `xuangu.*`、下令吞魂写在 `canhun.*`） |
| 银月、紫罗极火、逆星盟、万三谷、凌玉灵、玄阴经、元磁神光、银光鼠、血冰林、卜绪、掌天瓶 | 615 / 731 / 500 / 500 / 505 / 330 / 408 / 465 / 465 / 459 / 2425 | 表一或暗场 | **全章 0 处**（玄阴经只许「肋骨」不许点名；小瓶写「小瓶」） |
| 三阳老魔、天都妖尸、藏魂匣、妍姐姐、三仙宗、天星城出了大事 | 474-499 | 他不知道 / 不演 / 第 13 章 | **全章 0 处**（本稿补，12.1 另核表） |

---

## 13. 文案要求

与前十一章同一标准：对 ch416-499 最长公共片段 ≤ 6 字、对全本 ≤ 7 字、≥ 10 字 0 条（汉字剥离、逐条精确最长公共片段、带正负对照）；换词不算重写；key 命名 `ch12.<场景>.<用途>`；台词 18-60 字为主、不超 80，选项 5-20 字。

**转写高风险点**：

1. **n04-n05 同伴接连倒下**：谁倒下、在哪一步倒下是框架件；**倒下的方式只写「倒下」**（E 表 22）——腹穿、干尸、白骨穿身一概不写；胡月临终的回想是他心里的事（他不在场），0 处。
2. **n06 玄骨自报**：六百余岁、散尽元婴、转修玄魂之体、他估「顶多结丹后期」是事实；两人互问来历、互不相让的过程**讲法与先后全换**；对方报名号那一句只许一次、名号里的「上人」不用。
3. **n10 群像**：寒暄里的事实（三百年一次、蛮胡子得了别人孝敬的残图、为寿元果而来、星宫只管外殿）做成他旁听的几句；元婴诸人的外貌一样都不用，名牌之外只写「看不透」（O-9）。
4. **n18 三方传音**：框架件只有三条——青易许衣钵、极阴许势、玄骨拿丹方逼他（A65）；**极阴开出的具体条件一样都不用**，尤其拿紫灵作炉鼎那一句——0 处，换说法也不行。
5. **n21 元瑶求护**：她开价青火雷、他要啼魂兽、她交得太痛快他起疑——事实；她的媚态、「以身相许」一类的话、他的轻薄回嘴——0 处。
6. **n25 极妙幻境**：色段只许「声色之诱、他不为所动」的意思，**原著那一整段描写一样都不用**；惧段的框架件是「幻象自称小妹、唤他四哥」与婆罗珠灼热（A86、A87），黑殿的光、白影的衣着与眉眼自出。
7. **n26、n31 两次杀人**：蛮胡子斩结丹修士、玄骨杀乌丑——都只写倒下；碎尸、血、灰烬 0 处；玄骨自己死于圣火可写灰烬（A112）。
8. **n33 灵泉**：交易条款（灵乳换他放弃、他只要根、合力破阵两三日、海面分手）是事实；**他进门时撞见的那一幕、她的恼与羞**——0 处；分手后她的独白、元婴们搜塔——不演（E 表 23）。
9. **「九柄」**：全章只写「九柄」（A20）；七十二同出、剑分化、「化为乌有」都不写（B 表 9）。
10. **名场面台词不许复刻**：怪影对他躯体的那句挑选、金青临走的那句道歉、他回怪影「不必问」的那句、玄骨挡剑芒时的那句轻蔑、玄骨报名号那一句、他试探对方修为的那一句、极阴自称「乌某」的那句客套、星宫长老宣规矩那一段、葛笠讲鬼王那一队只逃出一人的那一段、紫灵讲啼魂兽来历那一段、他驳「此兽现世必乱」的那句冷笑、元瑶「我姓阮」那一句、极阴收记名弟子与赠环那一段、玄骨讥阴阳环那一句、他向元瑶要啼魂兽那一句、星宫长老「请罪」那一段、四门规矩那一段、小妹的两声、蛮胡子杀人后那句、万天明讥「真假」那一句、玄骨讲洗炼灵根那一段、残魂求饶那一长段、元瑶报出「万年灵乳」那一句、他「只要根」那一句——**意思保留，说法自己写**；3.2 与 13.1 里为说明意图写的转述同样不许照抄进文案。
11. **死亡口径**（E 表 22）：胡月、简姓修士、石蝶、金青、被斩的结丹修士、乌丑只写倒下；鬼物、狼形器灵、玄骨可写飞灰 / 灰烬。
12. **韩立不知道的不写**：极阴登岛发怒（ch429）、玄骨与体内旧识的密谋（ch445、449）、极阴的盘算与和青易的密约（ch455）、万天明一行议天青花（ch458）、卜绪与艳女（ch459-460）、星宫二人密谈（ch465、474、486）、元瑶对妍姐姐（ch498）、元婴搜塔（ch498）——情报、旁白、路径行动、物品描述一个字都不写到。
13. **章末停在鼎盖**：最后几条是鼎盖打不开、银狼不受控、弃府去外星海的心念一句；不写「天星城出了大事」（他 ch499 当面听到，按拍板推到第 13 章开场）、不 `ending()`。
14. **按键占位**（handoff 第 123–124 行）：文案提到菜单、存盘、路径行动、WASD 等可改键的操作时写 `{key.<id>}`，并登 `tests/KeyBindingTests.cpp` 两组 KeyPrompts 白名单；点名物理键的提示同时写一条 `<id>.pad` 版；别的 key 不许以 `.pad` 结尾。本章预计 **0 条**（3.4 末）；真要有，按契约 5.2 末条登记、先验计数随之加。

### 13.1 每段框架件清单（写手与复验共用的判据）

**判据**（沿用第 5 章 13.1，第 11 章两栏写法）：

- **剧情事实**＝框架件＝删了戏就塌的东西——事实、硬约束、谁在场、落点。下表逐段列全；与原著对得上是应当的。专名不算质感。
- **质感边界**＝表外的一切——人数、岁数、长相、手势、道具、次要人物的反应、次要的一句话、谁先谁后——**一律自出**。原著该场的质感项不许出现，**换了词也不行**；把原著的一个动作反过来写也算留着。本栏点名的是最容易被搬进来的那几样，不是全部，**只点名、不复述**。
- 设计语气卡点名的意象可以留，每段至多两处，写法要换。
- 复验：逐段做「原著质感项 → 现文」的并排表，剩下的对应项必须全在「剧情事实」里。

| 段 | 原著 | 剧情事实（删了就塌） | 质感边界（不许出现，换词也不行） | 在场 | 落点 |
| --- | --- | --- | --- | --- | --- |
| n01 登岛 | 416 | ①金青相邀，他两日后答应，曲魂同行；②两月赶路到荒岛：极大、无灵脉、草木不生；③土山脚下石屋，胡月、简姓修士、石蝶迎出；④两颗珍珠合图（白珍珠交出）；⑤他对外称曲魂是师兄（ch416）；⑥石蝶要先挑一件，金青因她父亲旧恩应下 | 他心里对遗址收获的盘算；近山时的风沙与护体灵光；石屋的成色；胡月的保养与眼角细纹；金青狡黠的神情；石蝶开条件时那句开场 | 韩立、曲魂、金青、胡月、简姓修士、石蝶 | `ch12.dengdao` |
| n02 破阵 | 417-418 | ①土坡上一团黄雾罩着阵，石蝶看出风土双属性；②他以北斗两仪阵的阵旗破最外层幻阵；③他研阵才二十余年、全靠辛如音的心得（不夸大）；④破阵三个月；⑤翅恶现，分水属法器布阵 | 石蝶接连试法器出丑与她用的那几件；光柱汇聚、雾如水沸；众人被风吹得站不稳；石蝶前倨后恭的那一礼 | 同上 | `ch12.pozhen` |
| n03 怪人 | 419-422 | ①背剑挎篮的怪人落下，自封岛主，结丹中期；②花篮是洪荒古宝，困住众人法宝；③他藏拙 / 放虫，两支都以数万噬金虫收场；④怪人遁走、他追上，怪人倒下（旁白）；⑤得花篮、红光画轴；⑥最后禁制破、翅恶尽灭；封灵柱；曲魂加巨力符推倒石柱，地洞门开 | 怪人的相貌与皮衣；胸前活骷髅头与咬断手指；黑血珠自爆；简姓修士心疼飞叉；黑幡鬼哭；虫云压顶的比方；合剑载人追击；骷髅头收玉匣；封灵柱的价钱 | 同上 ＋ 怪人 | `ch12.tuizhu` |
| n04 七霞莲 | 422-425 | ①池中七霞莲、千年石乳；②简姓修士急取落池；③七霞莲是妖冠蛇的诱饵；④白骨暴起、胡月倒下；⑤鬼雾自壁中涌出——展画轴，脂阳鸟分身护住众人；⑥石蝶独奔台阶、被鬼爪拖住、倒下 | 胡月临终回想（整段）；第三把飞刀；火燕的大小；石蝶的宝珠；遗体被扔到他脚边；鬼啼的形容；金青一喝退雾 | 韩立、曲魂、金青、胡月、简姓修士、石蝶 | `ch12.qixialian` |
| n05 夺舍 | 425-427 | ①怪影出柱；②画轴燃尽；③怪影要他的躯体、放另两人走；金青离去，奔过白骨时被射中、倒下；④怪影问他是极阴还是极炫门下；⑤偷袭，九柄剑与神雷逼退；⑥血灵钻穿他胸口、曲魂被夺，他收回分神；⑦引魂钟无效；⑧白骨融入，躯体炼成眉清目秀的少年 | 怪影的自称与戏弄；墨蟒与火鸟相斗；一拳击散火团、吞下晶珠；金青羞愧抱拳；怪影「挫骨扬灰」那一句；他止血的符；傀儡与合剑的阵仗；三色光华与骨节爆响 | 韩立、曲魂、金青、怪影 | `ch12.duoshe`；曲魂离队 |
| n06 萧诧 | 428-429 | ①少年自报萧诧、玄骨：未修玄魂之法已活六百余岁，散尽元婴转修玄魂之体；②他估对方顶多结丹后期；③对方要看玉简、问来历（据实 / 搪塞都不泄底），认了他的功法不是那一脉；④肋骨里取出锦帕，似他的残图；⑤对方化血光遁走；⑥他收遗物（旁白）；⑦出洞后他的锦帕现金剑、恒指西北 | 金雷竹箭与合剑的雷球对撞；他想起那箭插在白骨上的后怕；他心里给对方起的外号；黑蛇叼玉简；剑芒洞穿石壁；黑虫鸣叫、肋骨捏粉；火化遗体；锦帕上的红线；**极阴登岛那一段（不演）** | 韩立、少年 | `ch12.canyuan` |
| n07 离岛 | 429-431 | ①途中遇文氏父女被毒龙会三人追堵（地点改编为荒岛东岸，E 表 33）；②文思月认出他是妙音门名义长老；文樯是魁星岛旧识，两鬓斑白、筑基中期；③青元剑芒三道了结三人（旁白）；④文樯说离岛后的数十年（K4）；⑤她拒了数名同门结双修的要求、得罪妙音门高层、被派了这趟险差，他听出来了，只答应见到紫灵时提一句，不插手（ch430–431）；⑥告辞、循图 | 文思月的容貌与香气；供奉堂挂画像；文樯又羡又愧的神情；三人化荧光；他对故人老去的唏嘘；父女事后的对话（不演） | 韩立、文樯、文思月 | `ch12.lidao` |
| n08 入殿 | 431 | ①约千丈高空悬着的玉殿、金罩罩住；②他隐身等了七八日，见他人持同样的锦帕入殿；③注灵入图、白光护体穿罩；④殿门上「虚天殿」三字；⑤又窄又高的玉石通道，神识被禁制弹回；⑥大厅玉柱林立，修士各占一柱 | 宫殿的高度与美玉；红发老者与红云；他倒吸一口气；看字眼痛；通道走了多久；柱上浮雕 | 韩立 | `ch12.rudian` |
| n09 旧盟 | 432-433 | ①玄骨在大厅一角认出他，传音：你也有残图，合作；②九曲灵参能让凝婴机会倍增；③盟约：灵参换他助除极阴；④殿内禁制只容漂浮术一类小法术；⑤他认出苗长老的怒目 | 紫灵与青衫男子低语；他认出两名元婴的那段外貌；他心里「扮猪吃老虎」的盘算；他听到「取宝」二字的一喜 | 韩立、玄骨、紫灵、苗长老 | `ch12.jiumeng` |
| n10 群像 | 432-435 | ①极阴（自称乌某）携乌丑入殿，玄骨杀意一闪即收，极阴见他面露喜色；②南鹤岛青兄、白壁山温夫人寒暄：三百年一次、蛮胡子得了别人孝敬的残图、为寿元果而来；③五六日后天悟子、万天明（万法门门主，他猜元婴中期）、黑瘦老者、蛮胡子入殿；④抢柱（让 / 不让，不让则威压后照让）；⑤魔道几人议取鼎，他隔得远只听到片言；⑥星宫两执法长老宣规矩：只管外殿、内殿不管、殿内凶险不救 | 极阴冷眼盯住一名黄脸修士（**孙姓门主线不演**，B 表 19）；元婴诸人的外貌；分宝条件（传音）；温夫人之夫；长老对万天明、蛮胡子的嘀咕；玄骨讲星宫打压正魔那一段 | 韩立与大厅群像 | `ch12.qunxiang` |
| n11 传送 | 436 | ①第四日石门落下封死大厅；②尽头地面现小传送阵，两长老查验后先行；③万天明一行、玄骨先走；④极阴意味深长看他一眼；⑤他上阵 | 石门上的白光；众修士先慌后定；极阴与乌丑的问答（他不在旁）；一月后再现之说 | 韩立与大厅群像 | `ch12.chuansong` |
| n12 土坡 | 436-438 | ①荒凉小土坡，白光挡住鬼雾；②紫灵在此：假丹期、高价购图、只闯第一关；③葛笠：多有结丹修士被吞，上次一队遇鬼王、整队泯灭仅一人逃出；④黑袍人拒同行，放出啼魂兽独入鬼雾；⑤三人议定跟着通道走 | 紫灵那一笑与他的失态；葛笠对她艳名的狐疑；啼魂兽的长相；它的祭炼来历与「现世必乱」之说 | 韩立、紫灵、葛笠、黑袍人 | `ch12.tupo` |
| n13 鬼王 | 439-444 | ①葛笠只想摘寿元果、原路返回；②勾魂鬼音制住紫灵，他一声喝醒；③黑袍人被鬼物缠住，他出手；④① 三波：悍鬼与鬼夜叉 → 灰影与双头怪虎 → 三首鬼物（怪笑、附灵）；⑤神雷两发收尾，鬼物飞灰、主魂烧尽 | 紫灵被迫起舞；他喝的那个字；鬼夜叉的毛色与骨叉；黑袍人的飞锤；紫灵讲鬼夜叉那段；噬金虫被笑声震落；虎爪空手抓剑；女面虎头吞灰影长出第三首；剑分化（B 表 9） | 韩立、紫灵、黑袍人、葛笠 | `ch12.guiwang`；① |
| n14 元瑶 | 444-446 | ①黑袍人现为女修；②他唤「元姑娘」，她改口姓阮、戒备；他提百余年前魁星岛坊市外与「曲师叔」一面，她想起；③同为结丹初期；④出雾到花园：玉亭、二三十名修士、元婴闭目、两长老坐当中；⑤绿袍年轻修士毫发无伤走出鬼雾（玄骨）；⑥她求同行——拒 / 问来意，仍拒 | 她手按储物袋；紫灵靠他半步、似笑非笑；她脸红与「普通」那番自嘲；他猜她的奇遇；玄骨体内另有声音（不演）；极阴睁眼又闭 | 韩立、紫灵、元瑶；花园群像 | `ch12.yuanyao` |
| n15 小界 | 446-447 | ①玄骨传音再约：先抓灵参、再除极阴；②一整天后亭中阵现，星宫长老宣：采药一整天、过时永困，一个月后有阵传回大厅；③传到碧空草原；④玄骨说是古修士开辟的小空间 | 元瑶跺脚离去；紫灵与青年男修；他与玄骨互探（鬼雾里遇到什么）；极阴的虎视与他心里那股狠劲；草原景色的铺陈 | 韩立、玄骨；花园群像 | `ch12.xiaojie` |
| n16 灵参 | 448-451 | ①小石山，玄骨以神识看清灵之气；②金网：只有纯金之物擒得住九曲灵参；③四象玄武阵旗交他布——用他的旗 / 换自备的旗；④布饵诱参（呲唬、麝兰兽见于此段）；⑤白兔化身入网；⑥得九曲灵参（长约半尺、土黄） | **玄骨与体内旧识的密谋（不演）**；金丝球拉成网；阵旗上层叠的符文；他用大衍诀也看见青霞；灵参顶上的花与香 | 韩立、玄骨 | `ch12.lingshen` |
| n17 反脸 | 451-453 | ①玄骨点破、阴魂丝暗算；②噬金虫咬断；③绿影扑来被金色电弧击碎（旁白）；④他开条件，玄骨交出灵参本体（他以神雷验过金盒），说极阴已在他身上做了手脚；⑤黑云压来，玄骨先遁；⑥（`huanqi == 1`）阵罩曾被他催红 | 白兔被击晕扔进盒；他心里骂玄骨；黑光柱一击破罩；傀儡与灵兽袋列阵 | 韩立、玄骨 | `ch12.fanlian` |
| n18 拜师 | 453-456 | ①黑云中极阴与乌丑，极阴抛出血玉蜘蛛的蛛丝——他图的是蜘蛛；②他放出蜘蛛，以「灵兽认主」相抵；③青易现身，点破血玉蜘蛛是取鼎最佳灵兽之一；④三方传音（13 第 4 条）——三选一，只有拜极阴往下走；⑤收为记名弟子、赐玄阴环；玄骨传音：那本是阴阳环、被阳环所克；⑥极阴路上指点他修炼谬误；⑦乌丑悻悻 | 极阴的具体许诺（0 处）；乌丑的狠话；青易「闲逛」的托词；把蜘蛛炼成妖尸的威胁；极阴与青易的密约；指环的样子 | 韩立、极阴、乌丑、青易（玄骨传音） | `ch12.baishi` |
| n19 峡谷 | 456-459 | ①峡谷口三十余人，元瑶见他与魔道同行而惊避；②山体分罩红蓝两色光；③蛮胡子试他、「要他」一句，青易传音说破，三人议定分宝（他在旁）；④两碑：玄晶道 / 熔岩路——选玄晶道则极阴一句无防寒物、回到抉择；⑤极阴借白犀佩、蛮胡子借寒冰珠；⑥入通道即被禁制分散 | 分宝讨价的份数；万天明一行议天青花（不演）；炙风与寒风；碑上古字 | 韩立、极阴、乌丑、青易、蛮胡子、元瑶、万天明一行 | `ch12.xiagu` |
| n20 熔岩 | 459-460 | ①穿光壁入熔岩路，全程不能飞、五日为限；②大草地、熔岩沼泽两处（罗烟步 / 常速），沼泽暗坑靠寒冰珠护身；③怪树林遇炎灵兽（旁白） | 卜绪、艳女、玄晶道诸人、极阴漫步（**全部不演**）；闪电劈死飞行者；罗烟步的负担；沼泽里烫伤的形容 | 韩立 | `ch12.rongyan` |
| n21 啼魂 | 460-465 | ①黑沙漠无边，绕路至少多误两日——被时限挡回；②焦黄男修探路殒命（他隐身旁观）；③认出铁火蚁：水、冰属性中阶以上法术方能伤；④一日数群；⑤光团里元瑶呼救；⑥她开价青火雷，他要啼魂兽；她当场交兽、吐出鸣魂珠，交得太痛快，他起疑；⑦同行遇蚁群 → ②（并肩打大群是改编：原著大群他是独自以噬金虫打的，ch463）| 她的媚态与「以身相许」那番、他的戏言（0 处）；「只能救一时」那句冷话；青火雷与元阳之火的比；她眼里的窃喜 | 韩立、元瑶 | `ch12.tihun`；② |
| n22 炼晶 | 466 | ①一路数群飞蚁、未见蚁后；②沙丘后的水晶柱裹着飞蚁聚成的巨人、蚁后为眼；③元瑶说是万法门玄黄镜所为；④噬金虫连柱带蚁吞尽，剩炼晶；⑤炼晶是蚁后吞矿反吐之物、加强法宝的辅料 | 巨人的秃头与怒目；元瑶张口结舌；柱高几十丈；他从哪本古书读到 | 韩立、元瑶 | `ch12.lianjing` |
| n23 石殿 | 466-468 | ①五六十丈石殿、四面青石门、中间传送阵；②众人因冰火道异变暗骂星宫；③星宫长老「请罪」，无人敢问；④他与元瑶最后到，极阴、青易、蛮胡子围上来；⑤寒冰珠要还，蛮胡子挥手转赠；⑥四门规矩：三通阁楼一通直路、每人只取一样、取后传往极妙幻境、不闯的留殿自出；⑦三人借他婆罗珠、青冥针符宝、皇鳞甲 | 极阴等怕他与蜘蛛同死的那段心思；玄骨心里的动摇（不演）；长老「面壁百余年」；石桌石凳的数目 | 韩立、元瑶、石殿群像 | `ch12.shidian` |
| n24 取宝 | 468-470 | ①玉桥、匾额，阁高三十余丈、分两层；②一层玉台只看；③灵石启简易传送阵（兜底）；④二层光罩里数十件古宝，他看中金黄镜、五色铜环、暗红披风；⑤先取一件（披风 / 铜环）；⑥噬金虫结成黑甲、短暂隔断禁制，多取另一件；⑦传入石室 | 披风的两层构造与羽毛；他引辛如音空间禁制之说；结甲时的虫鸣与黑斑比方；蓝光压甲；他怕样子诡异被人看见 | 韩立 | `ch12.quba` |
| n25 幻境 | 471-473 | ①试铜环、披风（旁白）；②白云长廊、仙音——色段抉择；③黑殿——惧段：白影自称小妹、唤他四哥，婆罗珠灼热；④出殿见五层青石巨塔 | **色段原著的全部描写**；黑殿吞光（火球、月光石）；白影的衣着；小妹的眉眼与离家时那一眼 | 韩立 | `ch12.huanjing` |
| n26 塔前 | 473-474 | ①塔前众人，借物两人都不提；②玄骨传音「取宝之后」；③万天明约先进去、各凭本事，石门升起；④蛮胡子斩一名结丹修士（只写倒下）、元瑶化光遁入塔中；⑤乌丑改口叫他「韩师弟」 | 碎尸与血；蛮胡子杀人后那句；天悟子质问；元瑶身份与三阳老魔的议论（0 处）；星宫二人从传送阵现身（不演）；他猜极阴许了乌丑什么 | 韩立、塔前群像 | `ch12.neidian` |
| n27 层阶 | 475-477 | ①迷宫般的青石通道、石门朝向无规律；②乌丑：每人只能凭残图开一室、结丹只取一层；③狼首傀儡被蛮胡子赤手砸碎——Z2 收 / 不收残骸；④蛇卫同样被砸；⑤三四层（旁白）；⑥寒骊台：台阶是唯一出入口 | 蛮胡子满脸金鳞冲他一笑；那颗阴寒的碧绿宝石；玄骨说材料炼不成法宝；他想起傀儡真经与巨虎傀儡 | 韩立、极阴、青易、蛮胡子、乌丑、玄骨 | `ch12.cengjie`；Z2 |
| n28 卷轴 | 477-478 | ①台背青石是空心夹层；②剑芒划孔、取出陈旧发黄的卷轴、孔洞复原；③极阴传音催回；④卷轴收起不看 | 他靠石遮孔装歇息；卷轴的手感 | 韩立 | `ch12.juanzhou` |
| n29 如意 | 478-485 | ①蛮胡子砸开光罩，台上奇寒（白犀佩防寒）；②祭坛上金丝蚕吐丝拉鼎，正道借来寒蛟、离龟；③金丝蚕服天青花撼鼎失败、崩断（讨要 / 不要）；④鼎外是乾蓝冰焰；火蟒、血玉蜘蛛、青棘鸟上阵，蛮胡子对蜘蛛施狂暴之术；⑤正魔大战起，他退后；⑥鼎中古宝化双头火狼破鼎、逃向结丹修士处 → ③；⑦乌丑、玄骨争抢，他以花篮与五色铜环收服，得黄红两色狼首玉如意 | 寒蛟、离龟主人的名字（他听得见，本章不用）；乌丑的臂环、玄骨的珠串；化石兽、三目豹的打法；万天明以假雷骗人那段；他怕花篮露馅那段心思；残余三蚕被扯进洞、重物落地；蛮胡子捧腹大笑与「领教天罗真功」的挑衅；青易留青棘鸟、极阴留两具妖尸守入口；儒衫老者要灭废蚕、他「对奇虫有兴趣」的托词、老者错以为他穷；极阴「为师绝不亏待」那句与他背上的恶寒；洞深与蓝焰里的黑影、看两眼就眩晕；火蟒吞黑丸、尾插地面化作绳索；极阴喷两口精气；蛮胡子口吐血光成雾、蜘蛛通体变红；蛛丝泛蓝；青易放大宝光护住他；化石兽留守监视与「两个选择」；三目豹化宝为石与变异灵兽那段议论；万天明化紫焰火人与分身；天悟子敲龟壳、离龟吐寒浪；黑瘦老者柳枝化绿海缠妖尸；蛮胡子身涨数丈、金鳞银带、尖刺铁手套；青易分神所化青光大手被火龙一咬；他觉得元婴们像比武较技；乌丑黑气巨蟒缠狼、玄骨口喷绿光丝网（2026-10-13 补读 479–483）| 韩立、元婴诸人、乌丑、玄骨 | `ch12.ruyi`；③ |
| n30 补天 | 485-488 | ①冰封；②蛮胡子喝破隐身者——星宫长老；③另一长老白虹斩来：他被斩飞、皇鳞甲救命，一只血玉蜘蛛被斩两截；④两长老原是化身符，化作星光、只剩金符；⑤补天丹自鼎中飞出、元婴出窍争抢，最终落入蛮胡子手中；⑥他想溜向台阶被玄骨叫回；⑦众元婴追出 | 星宫二人在五层角落的对话（不演）；贯日剑的光与啸；他那一念「太快」；虎口震裂；金符的颜色 | 韩立、元婴诸人、乌丑、玄骨 | `ch12.butian` |
| n31 玄骨 | 489-494 | ①乌丑逼他交如意，玄骨出手，乌丑倒下；②玄骨点破第二只蜘蛛：一窝只两卵活；③补天丹真相：洗炼灵根，配九曲灵参凝婴三四成；④交出丹方玉简（两三种辅药他从未听说）；⑤炼尸接回死蛛、两蛛拉鼎，隔焰收鼎；⑥玄骨融冰焰为修罗圣火、要他死 → ④；⑦圣火核心爆裂失控，他十指连弹剑芒封住玄骨去路，火花落肩，玄骨化为灰烬；⑧冰焰凝珠、一截肋骨、金雷竹小箭 | 乌丑化灰、妖尸被绿莲包住、黑剑被鬼爪捏住；极炫与五色骸骨之说；电网缩放、双掌喷雷；玄骨咬舌自醒与贪念；他看玄骨的那种眼神；玄骨伸手入怀；鼎身的虫鱼山水雕纹与蛮荒气息；祭坛被冰封成一块蓝水晶；玄骨鬼气旋风化黑绿火柱、火柱化巨莲裹走冰焰；剑气化青索缠住两耳、鼎缩到掌上；他疑鼎是假；玄骨捧光球狂笑那句（三物合一的原话）与「糊涂鬼」那句；五环套颈与四肢、冰霜令环落地、「五行环」那番讥讽；九剑合一巨剑与冰龙相抵；剑影分光；血红披风自动披上；双掌拉出雷球、雷球化电网护罩；灰白火花铺天盖地；玄骨心里对金雷竹数量的惊疑（他的心思，不演）（2026-10-13 补读 491–492）| 韩立、乌丑、玄骨 | `ch12.xuangu`；④ |
| n32 残魂 | 494-495 | ①小箭里的残魂被啼魂兽的霞光缠住；②残魂求饶，许功法、秘府、说出追踪暗记——吞 / 听他说完，结果同：他决意灭之、令啼魂兽吞尽（ch495）；③噬金虫找出暗记、除去；④剑划破罩出台；⑤展卷轴：内殿示意图，红线通高墙后的传送阵；⑥灵石启阵（兜底） | 残魂幻化虫鱼鸟兽与老者鬼脸；啼魂兽饱嗝拍肚；他事后那句自语；暗记在身上哪里；傀儡探路 | 韩立 | `ch12.canhun` |
| n33 灵泉 | 496-499 | ①传到内殿二层密室：灵眼之泉，元瑶先在；②残图开不了来路的门；③她以万年灵乳换他放弃阵后之宝，他逼问出是养魂木、只要根；④合力破阵两三日；⑤传到海面，二人分手；⑥鼎盖如生铸，打不开；⑦玉如意召得出两只小狼，银狼不受控；⑧他想弃府、经星城传送阵去外星海（心念一句） | **他进门撞见的那一幕**与她的恼羞；她烘干头发；他泡脚回气；三大神木之说；她手按储物袋；分手后她的独白（0 处）；他在海上骂殿主；五色小珠与极炫；「天星城出了大事」（第 13 章） | 韩立、元瑶 | `ch12.done` |
| Z1 埋骨 | 438-439 | ①鬼雾路旁一具快到结丹中期的修士遗骸（玉真人）；②取衣而后埋骨 / 只埋骨；③辟火宝衣；④取衣是「埋骨的报酬」（意思保留、说法自写） | 青衫裹骨与断成数截、灵性犹存的残剑；葛笠弹火试衫、火灭；冰蚕丝炼衣、冰属性异灵根之说；他单手一抓收衣、剑光击坑、衣袖拂骨入坑；他「同为修道之人、盼有人为我拾骨」那番话；紫灵欲言又止、望新坟轻叹（2026-10-13 补读 438–439）| 韩立（整改后紫灵、葛笠都留在土坡上等他探路，`yihai.ziling` 改旁白——他想起葛笠在坡上提过玉真人；18.7）| `ch12.z1` |
| 走偏、采药 | 改编 | 走偏：鬼雾把人推回岔口；采药：一整天、两株灵草 | 原著无对应，**一律自出**；不许把 n15 草原的铺陈挪进来 | 韩立 | —、`ch12.caiyao` |

**名牌对照**（名牌跟着 role 走，换名牌是换 role）：怪影（`dixue_guiying`）→ 少年（`xuangu_shaonian`）→ 玄骨（`xuan_gu`，节点 6 之后）；黑袍人（`yuan_yao_heipao`）→ 元瑶（`yuan_yao_jiedan`，节点 14）；文樯另建 `wen_qiang_zhuji`、紫灵另建 `zi_ling_xutian`；两名星宫执法长老两个 role、名牌分「慈眉长老」「冷脸长老」（同图同名牌规则 30 必红，5.5）；「萧诧」「银月」不上任何名牌。

---

## 14. 时长估算

按幕（笔记 N.2），目标 55 min：

| 桶 | 幕 | 项 | 估 |
| --- | --- | --- | --- |
| A | 一·遗址 | n01–n07：两处解谜（阵势三选一、画轴两步）、四同伴倒下与曲魂离队压在 n04–n05、文氏父女一段；`advance_days` 62 ＋ 90 ＋ 4 | 13 min |
| A | 二·入殿 | n08–n11：群像多、对话多；`facility_dazuo_dadian` 让等候的日子有事做；8 ＋ 8 ＋ 3 日 | 7 min |
| A | 三·鬼雾与小空间 | n12–n19：迷路 3、① 5、花园与采药 2、捕参与反脸 3、拜师 1；1 日 | 14 min |
| A | 四·冰火道与宝光阁 | n20–n26：熔岩 2、② 4、取宝 2、幻境 2、塔前 1；1 ＋（抉择 0–2）＋ 1 ＋ 1 日 | 11 min |
| A | 五·内殿夺鼎 | n27–n33：③ 3、④ 4、其余旁白与抉择 3；3 日 | 10 min |
| **A** | | | **≈ 55 min**（意图玩家日历 182 日；`tools/audit.py` 落地后重算） |
| C | 可选 | Z1、Z2、路径行动 13 条、采药、玉台反复看、走偏 | 3-5 min（估） |

四场仗（① 5、② 4、③ 3、④ 4 min）已含在幕内。**真要压，先压这三处**（都不动框架件）：n10 群像（入殿两批并成两段，寒暄条目交给 K5–K7 承担一部分）、n27 傀儡（狼首、蛇卫并成一句，Z2 抉择保留）、n33 尾声（鼎盖、玉如意、心念三句并成两句）。玩家在迷路里多走几趟、在玉台前多看几次——那是玩家自己挑的节奏，记在 C 桶。

---

## 15. 验收

每一条都配一条**不经过驱动、直接读数据、判据写死成本文原文**的断言；每一条新检查都要配「故意写坏 → 确实被抓住」的用例。**切片一律写明起点存档**；没写的，起点是 `tests/fixtures/ch11-end-first.sav`（**T1 重生成版**），由切片自己推到所需旗标。

| # | 验收条目 | 直接读的断言 |
| --- | --- | --- |
| 1 | 四道门禁全过（validate / selftest / mapgen / artgen），零新增警告 | 门禁本身 |
| 2 | **一条自动化通关测试**，起点 `tests/fixtures/ch11-end-{first,second}.sav`（T1 重生成的两份），落在 `ch12.done`；终点写 `ch12-end-{first,second}.sav`（v9） | `SetUp` 先验 fixture：`ch11.done == 1`、`realm == realmCap == 31`、`formerRealm == 22`、`party` 恰 `[qu_hun_shadan]`、`learnedMagics` 恰第 10 章那 11 门、`story_jinpa_ditu == 1`、`story_bai_zhenzhu == 1`、`weapon_qingzhu_fengyun_jian == 1`、`story_xueyu_zhizhu == 1`、`story_shijin_chong == 1`、`story_yinhun_zhong == 1`、`mapId == ch12_tushan` 且站在 `andeng`；`day`、`hp / mp`、两种灵石按真档写死（C.3 标「必须」的各项，O-3）；**意图玩家写死**（两侧同一组选项，笔记 N.1 第 5 条）；终点先验：`realm == realmCap == 31`、`formerRealm == 22`、**`party` 为空**、`learnedMagics` ＝ 起点 11 门 ＋ `magic_shijin_chongqun`、`story_bai_zhenzhu == 0`、`story_zhiyang_huazhou == 0`、`talisman_bixie_shenlei == 0`、`material_lianjing == 1`、`story_jinsi_can == 3`、下列 23 种 story 各恰 1：花篮、辟火宝衣、九曲灵参、玄阴环、白犀佩、寒冰珠、啼魂兽、鸣魂珠、婆罗珠、青冥针、皇鳞甲、血色披风、五色铜环、傀儡残骸、内殿卷轴、玉如意、丹方、虚天鼎、乾蓝冰珠、玄骨肋骨、金雷竹小箭、万年灵乳、养魂木根须；`story_jinpa_ditu`、`story_yinhun_zhong`、`story_xueyu_zhizhu`、`story_shijin_chong` 与起点相同；采药那种灵草比起点多 2；`material_lingshi` 恰起点 − 8、`material_lingshi_zhong` 恰起点；`ch12.huanqi == 2`、`ch12.quba_xian == 1`、`ch12.xinjing == 3`、`ch12.jincan == 1`、`ch12.z1 == 1`、`ch12.z2 == 1`、`ch12.caiyao == 1`；**`day` − 起点 `day` 两侧恰 182**（字面量写死，不从驱动推导）；`mapId == ch12_lingquan` 且站在 `quanbian` |
| 3 | 挂点 `mode` / `once` / guard / set_flag 与 3.1 表一致；`once=true` 的每一个都有 `set_flag`；NPC 挂点（n06、n07、n09、n18、n33）无 once、脚本开头判 done_flag；`trigger_yutai` once 否、无旗标、不 `give`；无 `auto`；无同格两个对象；**没有一个 enter 触发器覆盖任何 spawn 格**（n01、n08、n12、n20 是 spawn 前方横贯通道的触发带：从 spawn 出发不踩它就走不到该图任何其它挂点）；`trigger_yutai` 从 `yuqiao` 走得到且不必经过 `trigger_yuqiao`；十处 `facility_cunji_*` 与 spawn 相邻、不与触发器同格；四处 `facility_dazuo_*`（`meditate`、`effectiveness 100`）在 dadian、binghuo、taqian、hanlitai；三处落点格（鬼雾岔口）不与任何触发器同格 | `Ch12TriggerModeTests` 读十张新图逐条对表 |
| 4 | 四张编成与 8.2 一致：① 三波、`weaknesses` 照表、`sanshou_guiwu` 的 `killable_by` 恰 `["木","火"]`、`charge.all == true`、ally 恰 `zi_ling_xutian`、`yuan_yao_heipao`；② 三波、蚁群与大群 `killable_by` 恰 `["水","金"]`、ally 恰 `yuan_yao_jiedan`；③ 两波、`langxing_qiling_yin` 的 `killable_by` 恰 `["金"]`、ally 恰 `wu_chou`、`xuan_gu`，**两名友军的 `magics` 不含金类**；④ `xuan_gu` ×1、`killable_by` 恰 `["木"]`、`charge.every == 3`、无友军；四张 `hero_absent false`、`defeat_is_fatal true`、`can_escape false`、`spirit_stones == 0`、`drops` 空、`cultivation > 0`；`killable_by` ⊆ `weaknesses`、无空数组；`b12_xutian_hudian_kuilei` 不存在 | `Ch12BattleDataTests` ＋ `BattleHand`（10.2 B1） |
| 5 | **境界**：`realm.advance / cap / demote` 本章各 0 处；切片：本章任一点面板下一关写「结丹中期」所需 5000、硬按被 `BreakthroughBlock` 回绝 | 扫 `scripts/` ＋ 切片 |
| 6 | **曲魂离队与空队伍**：`party.remove("qu_hun_shadan")` 恰 1 处（`duoshe.lua`，看返回值）、`party.add` 本章 0 处；`qu_hun_shadan.json` 与第 11 章终点逐字节相同；切片（起点把 `party` 改空）：演完 n05 → 旁白换 key、`ch12.duoshe == 1`；空队伍下走到 n13 打完 ①（契约 E1 的真实入口版） | 扫脚本 ＋ 切片 |
| 7 | **同一 role 先友后敌**：`xuan_gu.json` 只一份，③ 里 `faction ally`、④ 里敌方；`wu_chou.json` 与第 11 章集成版逐字节相同；`xutian_hudian_kuilei`、`duoding_xiushi` 不存在 | 读数据（契约 E2 引擎用例之外） |
| 8 | **锦帕门闸**：`lidao.lua` 在 `advance_days`、`flag.set("ch12.lidao")`、`teleport` 之前判 `item.count("story_jinpa_ditu") >= 1`；`rudian.lua` 0 处 `item.count("story_jinpa_ditu")`；全仓 `take("story_jinpa_ditu")` 0 处；`trigger_rudian` `once true`、`set_flag ch12.rudian`；切片（起点删锦帕、推到 `ch12.canyuan`）：与文思月对话 → 只一句、`return`，`mapId` 仍 `ch12_tushan`、`ch12.lidao == 0`、`day` 不变（数据上不可达，只验分支存在且不把人关死） | 扫脚本 ＋ 切片 |
| 9 | **冰火道计时**：`rongyan.lua` 恰一处 `flag.set("ch12.binghuo_ri", today())`；两处抉择罗烟步 0 日、常速 `advance_days(1)`；`tihun.lua` 读 `today() − binghuo_ri`，绕路一支只一句、不拨日；切片：两处都选常速 → `day` 比意图玩家多 2、照样过关、永不超时 | 扫脚本 ＋ 切片 |
| 10 | **宝光阁取舍**：`quba.lua` 先取写 `quba_xian` 1 或 2、两件都 `give`、金黄镜 0 处 `give`；灵石三支（低阶 4 / 中阶 1 / `advance_days(1)`）；n32 同此；切片（起点把两种灵石改 0）：n24、n32 各 `day` ＋1、两件照得、`ch12.canhun == 1` | 扫脚本 ＋ 切片 |
| 11 | **幻境与拜师不失败**：`ch12.xinjing` 只写位 1、2；切片：两段都先选错 → `xinjing == 0`、`ch12.huanjing == 1`；n18 先选青易、再选两边不拜 → 回到同一抉择、最终 `story_xuanyin_huan == 1`、`ch12.baishi == 1` | 扫脚本 ＋ 切片 |
| 12 | **神雷与噬金虫**：`give("talisman_bixie_shenlei", 2)` 恰 2 处（`guiwang.lua`、`xuangu.lua`），各自的 `take(余数)` 在 `battle` 返回之后、看返回值；物品 `kind talisman`、`castMagic magic_bixie_shenlei`、`tradeable false`、`price 0`；`magic.learn("magic_bixie_shenlei")` 全仓 0 处；`magic.learn("magic_shijin_chongqun")` 恰 1 处（`guairen.lua`，看返回值） | 读数据 ＋ 扫脚本 |
| 13 | **经济账**：本章 0 店、`data/shops/` 无本章文件；四场 `spirit_stones 0`；`take("material_lingshi"…)` 只在 `quba.lua`、`canhun.lua`；两侧走同一组选项，章末两侧低阶差等于起点差、中阶不变 | `Ch12LedgerTests` |
| 14 | **必打 4 场绕不过去**：四编成各只被一个脚本调用（`guiwang.lua`、`tihun.lua`、`ruyi.lua`、`xuangu.lua`），`set_flag` 在目标链里且是下一步 guard；通关两侧各记实际打到的编成，恰是这 4 个；本章没有别的 `battle(`；遭遇区 0 处 | 读数据 ＋ 扫脚本 |
| 15 | **走不回去的路**：十张新图只有荒岛 ↔ 地洞一对门；`portal_dixue` `require_flag ch12.tuizhu`、`deny_text_key ch12.block.dixue`；通第 11 章任何图的门 0 扇；切片：`ch12.tuizhu` 未置时 `portal_dixue` 拦人、拒绝语是那个 key | 读地图 ＋ 切片 |
| 16 | **走偏**：三个触发器 `enter`、once false、guard `ch12.tupo`、无 `set_flag`；`zoupian_a/b/c.lua` 各 0 处 `flag.set`、开头判 `ch12.guiwang` 已置则 `return`、三份的 `teleport` 坐标两两不同；`kScriptTransfers` 五元三条各用自己的脚本名；切片：走偏后 `mapId` 仍 `ch12_guiwu`、站在那条岔口的坐标；`guiwang` 置后再踩 → 位置不变 | 读地图 ＋ 扫脚本 ＋ 切片 |
| 17 | **NPC 在场链**：H 表每个对象「前一段的 hidden 恰是后一段的 visible」；同图同 role 的对象（鬼雾两个紫灵恰接于 `guiwang`、小空间两组恰接于 `baishi`）一个的 hidden 恰等于另一个的 visible；塔前与寒骊台两组、两个元瑶分属 `ch12_taqian` / `ch12_hanlitai` / `ch12_lingquan`；两名执法长老名牌不同；寒骊台乌丑、玄骨退场为 `ch12.xuangu` | 规则 30 ＋ `NpcPresenceTests` ＋ 读地图 |
| 18 | **支线 Z1、Z2** 谓词与笔记 N.3 字面量相符：`q12_maigu` `accept ch12.tupo / complete ch12.z1 / fail ch12.guiwang`；`q12_kuilei_canhai` `accept ch12.neidian / complete ch12.z2 / fail ch12.cengjie`；两条 `origin "原著"` | 读 `data/quests/q12_*.json` |
| 19 | **路径行动 13 条**：全部 `inquire`、`realm 0`、id 以 `_dating` 结尾、`when` 含该 NPC 的 visible_flag、`until` 含 `ch12.done` 与该对象的 hidden_flag（K1、K13 另含更早收的话题旗标）；13 条都不设 `reveal`（16.1 第 41 条） | `PathActionTests` ＋ 读 `data/pathactions/ch12.json` |
| 20 | **章末与开局**：`lingquan.lua` 最后置 `ch12.done`、不 `ending()`、不 teleport；`zhuzhong.lua` 与第 11 章集成版 diff 只末尾一行；章节卡两段队列见 10.2 C1 | 扫脚本 ＋ 真实入口 |
| 21 | **禁词**：表一含 12.1 的 83 词与「紫罗极火」；12.2 的章内先后按 key 前缀逐条断言；分母先验 > 250 条 | `LexiconTests` ＋ 专项 |
| 22 | **暗场与死亡口径**（13 第 7、11、12 条）逐 key 核；12.2 末两行的词 0 处 | 独立校对 ＋ 12.2 专项 |
| 23 | 独立校对：笔记 A 表 126 条硬约束逐条核；13.1 逐段并排；原创性人工复核（13 第 10 条那二十余处） | 校对报告 |
| 24 | 主线节点 33、**目标链 33 步**、抉择点 ≥ 20、新地图 10、战斗 4、物品新建 27、旗标 41 | `tools/audit.py` |
| 25 | **引擎**：契约 E1、E2 的判据落地（`tests/Ch12EngineTests.cpp`）；`tests/fixtures/*.sav` 与 `saves/*.sav` 在 v9 下全部读得进来 | 第 10 节 |
| 26 | **视觉数据走真实入口**（handoff 第 6 节第 5 条）：`data/visual/battles.json` 四场背景、`data/visual/maps.json` 十张图的主题 / 时辰 / backdrop 各指得出读它的那一行代码；经真 `BattleScene` / `WorldScene` 入口建场，断言四场背景与十张图的预设生效（负向：临时改坏一条 → 断言红） | `GameWiringTests` |

---

## 16. 拍板记录与开放问题

### 16.1 已拍板（按推荐，预授权——第 1–35 条照笔记 E 表；第 36–48 条是 2026-10-13 四路审查后的整改拍板，与前 35 条冲突处以后者为准）

| # | 拍板 | 本稿怎么落 |
| --- | --- | --- |
| 1 | **开局 patch `zhuzhong.lua` 末尾一行** `teleport("ch12_tushan")`，`andeng` 标 default | 10.1 C1、T1；验收 20 |
| 2 | **章节卡「第十二章」「虚天殿」** | 10.1 C1；10.2 C1 的两段队列 |
| 3 | **残图不收集**：入殿门闸读 `story_jinpa_ditu`；「醒」是 n06 旁白与 `ch12.canyuan` | 13.1 n06、n08；验收 8；大纲回写（C7） |
| 4 | **白珍珠** n01 `take` 看返回值，0 也照走 | 13.1 n01；验收 2 |
| 5 | **曲魂离队** n05 `party.remove`；`qu_hun_shadan.json` 不动；夺舍后用 `xuangu_shaonian` → `xuan_gu` 两个 role | 契约 E1；13.1 名牌对照；验收 6 |
| 6 | **韩立单人**，临时同伴只以写死友军进编成；`party.add` 0 处 | 8.2；第 17 节第 59 条；验收 6 |
| 7 | **境界全章 31 / 31**，`realm.*` 0 处 | 验收 5；第 17 节第 60 条 |
| 8 | **必打 4 场**；看守怪人、绿影、内殿傀儡做旁白 | 13.1 n03、n17、n27；10.1 O3；验收 14 |
| 9 | **元婴与玄骨名牌 `CoreLate`**，文案「看不透」 | 13 第 3 条；第 17 节第 60 条 |
| 10 | **鬼冤之地是迷路**：三处走偏同图回岔口，五元登表 | 10.1 T2；验收 16 |
| 11 | **① 三波**，友军紫灵、黑袍人 | 8.2；验收 4 |
| 12 | **冰火道日历计时**，固定 2 日 ＋ 至多 2 日，永不超时 | 第 11 节 `binghuo_ri`；验收 9；10.1 O2 |
| 13 | **② 耐久**：三波、波间不回血、不可逃 | 8.2；10.2 B1 |
| 14 | **宝光阁两件都到手**，金黄镜只描述；传送阵灵石三支兜底 | 13.1 n24；验收 10 |
| 15 | **极妙幻境不失败**，`xinjing` 只供章末一句 | 13 第 6 条；验收 11 |
| 16 | **拜师三选一**，只有拜极阴往下走 | 13 第 4 条；验收 11 |
| 17 | **不加第三阵营**：争夺者当友军，元婴大战只在开场白与旁白 | 契约 E2；10.1 O1 |
| 18 | **占位**：`b12_neidian_duoding` 就地改写成 ③，另一张编成与两个 role 删 | 10.1 C4、C5；验收 4、7 |
| 19 | **辟邪神雷走道具施法**，n13、n31 各给 2、战后收余数 | 验收 12 |
| 20 | **玉如意只作 story**，器灵不受控 | 10.1 O4；13.1 n33 |
| 21 | **噬金虫做法术** `magic_shijin_chongqun`，物品不动 | 验收 12 |
| 22 | **死亡写法**：同行者只写倒下，鬼物、狼形器灵、玄骨可写灰烬 | 13 第 1、7、11 条 |
| 23 | **不演的场**（判据③） | 13 第 12 条；13.1 各行「不演」 |
| 24 | **章末停在灵泉**：置 `ch12.done`，不 `ending()`、不 teleport | 13 第 13 条；验收 20 |
| 25 | **规则 25 登第 12 章、探针挪第 13 章** | 10.1 C2；10.2 C2 |
| 26 | **路径行动 13 条**，全 `inquire`、`realm 0`；~~两条 reveal~~ → **0 条**（第 41 条）| 16.3；验收 19 |
| 27 | **支线 2 条可选、0 条必经** | 13.1 Z1、n27；验收 18 |
| 28 | **0 店**，四场不给灵石，灵石只两处付出 | 验收 13（位置见 16.2 第 2 条） |
| 29 | **新图 ~~7~~ 10 张**（第 37 条）、旧图 0 处修补、主题全用现有 | 10.1 C5；验收 15、24 |
| 30 | **借来的宝物全做 story** | 验收 2 |
| 31 | **九曲灵参不建配方** | 10.1 O5 |
| 32 | **小空间采药**：`trigger_caiyao` 给现有灵草 2 | 第 11 节；13.1 末行；验收 2 |
| 33 | **文氏父女挪到荒岛东岸**（改编） | 13.1 n07 |
| 34 | **卷轴主线必经** | 3.4 第 28 步；13.1 n28、n32；验收 2、24 |
| 35 | **废金丝蚕讨要 / 不要**，记 `ch12.jincan` | 第 11 节；验收 2 |
| 36 | **enter 挂点不压 spawn**：n01、n08、n12、n20 改成 spawn 前方、离开落点必经的横贯触发带（第 11 章 `trigger_chudao` 先例）；落点格、存档点、打坐设施在带的落点一侧。理由：enter 只在 `tryStep` 迈步时判，teleport 落在触发格上什么也不发生 | 3.1；4.1；验收 3；T1 通关驱动要先迈一步 |
| 37 | **`ch12_neidian` 拆成四张图**：`ch12_baoguang`（`yuqiao`）、`ch12_taqian`（`shishi`）、`ch12_hanlitai`（`taijiao`）、`ch12_lingquan`（`quanbian`），各自的脚本落点就是 default spawn；新图 7 → 10（改第 29 条）。理由：规则 13 只从 default spawn 做 BFS，四区以墙隔开就有三区报不可达 | 第 4 节；3.1；3.4；验收 2、3、24；契约 3、4、5.3、6.2 |
| 38 | **`trigger_yuqiao` 挪到简易传送阵前一格**，spawn `yuqiao` 空着 | 3.1；4.1；4.3 第 1 条；验收 3 |
| 39 | **走偏拆三份脚本** `zoupian_a/b/c.lua`，各写死自己那一处岔口坐标、各 0 处 `flag.set` | 3.1；3.2；第 7 节；验收 16；T2；契约 4、5.3 |
| 40 | **玄骨只带魔火**：`xuan_gu.magics` ＝ `["magic_moyan_huo"]`，修罗圣火只走 ④ 的 `charge.text_key`；`magic_xiuluo_shenghuo` 不建（新法术 4 → 3）；规则 25 ③ 登魔火。作废 16.2 原第 4 条的「开例外」 | 8.2；12.2；10.1 C2、C4；契约 2.1、2.2、5.2 |
| 41 | **13 条路径行动都不设 reveal**（改第 26 条）：reveal 提示拼 role 名牌，K8、K9 会在 `tupo` 阶段露出「三首鬼物」「鬼夜叉」 | 16.3；12.2；8.2 ①；验收 19 |
| 42 | **`story_zhiyang_huazhou` 显示名「红光画轴」**（id 不动；ch421 原著叫法） | 12.2；第 9 节；笔记 L.1 |
| 43 | **规则 30**：花园 `npc_zi_ling_hy` visible ＝ `ch12.guiwang`；两名执法长老名牌「慈眉长老」「冷脸长老」 | 5.1；5.5；4.1；第 11 节；验收 17 |
| 44 | **锦帕门闸前移到 n07** `lidao.lua` 的 teleport 之前；n08 不设门闸（单向落点上判不足等于把人关在无门的大厅） | 3.2；第 7 节；验收 8 |
| 45 | **回血兜底**：冰火道光壁、塔前石室、寒骊台下各一处打坐设施（meditate、`effectiveness 100`），连同大厅共 4 处；平衡路加「最低合法起点」用例 | 3.1；4.1；8.1；10.1 B1；10.2 B1；验收 3 |
| 46 | **`ch12_guiwu` 显示名「鬼雾」**：首次由 `chuansong.lua` 送入，早于 `tupo.*` 点名「鬼冤之地」 | 第 4 节；12.2 |
| 47 | **啼魂兽、鸣魂珠在 ② 之前给**（ch464 谈妥当场交）；② 里元瑶并肩打大群标改编 | 1.1 A73–A75；3.2 n21；8.3；13.1 n21 |
| 48 | **① 友军来路登记**：`("剑","role_weapon","剑","zi_ling_xutian")`、`("火","role_magic","magic_moyan_huo","yuan_yao_heipao")` | 8.2；10.1 C2；契约 5.2 |

### 16.2 与旧拍板的冲突、开放问题（拿不准、需协调者核的）

1. **第 11 章两条验收的落点改动**：验收 21「第十一章　终」接「未完待续」随 T1 改成接「第十二章　虚天殿」——这是一处落点改动，不是推翻那条板；验收 14「`wu_chou.json` 与第 10 章逐字节相同」照守（B 表 4），它的 note 由协调者在两章都集成后回写（O-11）。
2. **灵石兜底的位置**（已解）：笔记 E 表 28 与 C.3 原写「n24、n33」，F 表 n32 与 N.1 第 2 条写「n24、n32」；按 F 表与 N.1 落在 n32（`canhun.lua`，高墙后密室的传送阵在 n32 启动、n33 已在灵泉），笔记 E 表 28 与 C.3 已改一致。
3. **「脂阳鸟画轴」早于「脂阳鸟」**（已拍板，16.1 第 42 条）：物品显示名改「红光画轴」（id 不动），n04 展开后由旁白点出脂阳鸟；笔记 L.1 已改。
4. **「修罗圣火」早于 ch491**（已拍板，16.1 第 40 条；原推荐作废）：原推荐「12.2 对 ③ 战斗界面开例外」与 8.2 末条「只放一门现有火法」相反。定为后者——`xuan_gu.magics` 只放魔火，③④ 仍是同一份 `xuan_gu.json`，不另建 role；修罗圣火只在 ④ 蓄势文案。契约 2.1、2.2、5.2 与笔记 I、L.2 已改。
5. **12.1 另核表**：红月岛（K2 在用）、炎灵兽（n20 在用）、金雷竹小箭（物品名）是笔记 M 表漏登的；其余几项与 12.2 末行（三阳老魔等）是本稿加的防线，协调者不同意可整行删，不牵动别处。
6. **契约 E1**（O-2）：空队伍本身第 5–7 章已跑过；本章新的是「曾有队员、章中途离队后接着进带写死友军的仗」，判据收窄到建场人数与菜单两条（契约 0 节）；若有崩点，引擎路先报、先改笔记再动 `src/`。
7. **ch11-end 真档**（O-3）：C.3 标「必须」的各项落地后逐项核；起点 `day` 若与笔记估值不同，增量 182 不变、起点按真档改。
8. **走偏五元**（O-5）：同图带坐标回岔口是五元写法第一次这样用——测试路核 `ObjectiveRoute` 不把它当链上一步。
9. **神雷的 kind**（O-6）用 `talisman`，图鉴文案注明是剑上积攒的神雷；**删占位的时机**（O-7）在第 11 章集成之后；**采药灵草 id**（O-13）由协调者选。
10. **支线配额**（O-8）：本章 2 可选 0 必经；若全作账要本章出一条必经，把 Z1 改必经（去 `fail`、`trigger_yihai` 进目标链，链变 34 步，验收 2、24 随改）。
11. **文氏父女的地点**（O-10）：若校对认为应留海上，n07 改挂 `ch12_dadian` 殿外一角（大厅图南端加一块露天区）。
12. **13.1 质感边界的覆盖面**：本稿逐段读过原著 416、417、419、421、425-432、435-437、440、443、445、447、449、453、455、456、458、460、464、466、467、470、472、474、475、478、484、486、489、490、493、495、497-499 各章；2026-10-13 补读 438-439（Z1）、479-483（n29）、491-492（n31）并已填进 13.1。**派写手之前**，协调者还须补读其余各章（418、420、422-424、433-434、441-442、444、446、448、450-452、454、457、459、461-463、465、468-469、471、473、476-477、485、487-488、494、496），把质感项填进对应行——判据要先于写作给全（handoff 教训 13），不留给复验。
14. **推翻第 11 章 16.1 第 19 条**（探针挪到 `wu_chou` ＋ `b12_neidian_duoding`）：本章 ③ 写实后乌丑是友军，「全仓唯一含 `wu_chou` 敌方的编成」不再成立，探针改挪 `wuji_haishou` ＋ `b13_waixinghai_lieyao`（16.1 第 25 条）。照第 11 章 16.2 第 11 条的写法记在这里；后果见 16.4 第 25 条。
15. **第 11 章 16.1 第 6 条「星宫长老不建 role、不上图」只管第 11 章**：ch435 起两名执法长老亲自在大厅宣规矩、在花园与石殿坐镇，本章新建 `xinggong_zhifa_ci` / `_leng` 两个 role 上图，不算推翻。
13. **C 桶 3-5 min** 与四场仗各几分钟是本稿按笔记 N.2 拆的估值，`tools/audit.py` 落地后重算。

### 16.3 路径行动条目（施工用；`ch12.path.*` 旗标**等 `data/pathactions/ch12.json` 落地后由协调者登记**）

全部是「打探」（`kind: inquire`）、**一律 `realm 0`**：殿内禁斗法、无店，`challenge` / `purchase` 0 条。id 一律 `<npc>_<话题>_dating`（`PATH_ID_RE`）；`when` 都含那个 NPC 对象的 visible_flag，`until` 都含 `ch12.done` 与该对象的 hidden_flag（K1、K13 另含更早收的话题旗标 `pozhen`、`ruyi`）；同一 NPC 对象只挂一条，无时段重叠；**13 条都不设 reveal**（16.1 第 41 条）。文案 `data/text/ch12_path.json`，key `ch12.path.<id>.text`。

| id | 图 / NPC 对象 | when | until | 效果 |
| --- | --- | --- | --- | --- |
| `jinqing_yizhi_dating` | 荒岛 / `npc_jin_qing` | `ch12.dengdao` | `ch12.pozhen`、`ch12.tuizhu` | 两颗珍珠合成全图；岛上无灵脉、草木不生（A3） |
| `shidie_hongyue_dating` | 荒岛 / `npc_shi_die` | `ch12.dengdao` | `ch12.tuizhu` | 红月岛阵法世家；其父对金青有恩，所以金青先应了她 |
| `huyue_sanxiu_dating` | 荒岛 / `npc_hu_yue` | `ch12.dengdao` | `ch12.tuizhu` | 散修能结丹的少、比同阶穷，所以要搏一搏 |
| `wenqiang_kuixing_dating` | 荒岛 / `npc_wen_qiang` | `ch12.canyuan` | `ch12.lidao` | 师傅坐化后，他在魁星岛熬了数十年才勉强筑基，之后离岛游历、入妙音门做杂事（ch430） |
| `xuangu_jinzhi_dating` | 大厅 / `npc_xuan_gu` | `ch12.rudian` ∧ `ch12.jiumeng` | `ch12.chuansong` | 殿内禁制只容漂浮术一类小法术，法宝祭不出（A39） |
| `ziling_canyuan_dating` | 大厅 / `npc_zi_ling` | `ch12.rudian` | `ch12.chuansong` | 她资质平平，高价买图碰运气，只打算闯第一关（A45、A66） |
| `zhifa_guiju_dating` | 大厅 / `npc_zhifa_ci` | `ch12.qunxiang` | `ch12.chuansong` | 星宫只管外殿的恃强凌弱，殿内凶险不救（A42） |
| `ziling_guiwang_dating` | 鬼雾 / `npc_zi_ling` | `ch12.tupo` | `ch12.guiwang` | 鬼物畏雷（改编；不设 reveal，16.1 第 41 条）。A47（一队遇鬼王、仅一人逃出）整改后改由 `tupo.legend` 承担，本条不再说（18.7） |
| `geli_guiwu_dating` | 鬼雾 / `npc_ge_li` | `ch12.tupo` | `ch12.guiwang` | 雾里认路：前人白骨所在、雾色发淡处多半走得通（迷路提示，改编；不设 reveal） |
| `xuangu_lingshen_dating` | 小空间 / `npc_xuan_gu` | `ch12.xiaojie` | `ch12.lingshen` | 九曲灵参化形、钻土入木，只有纯金之物擒得住（A58） |
| `qingyi_binghuo_dating` | 小空间 / `npc_qingyi_xg` | `ch12.baishi` | `ch12.xiagu` | 冰火道一踏进通道就被禁制传到峡谷各处、全程不能飞、五日为限（A69、A71；**改编**：原著禁飞是紫灵告诉他的，ch459，被传散是他自己知道的，ch458，借青易之口说） |
| `wuchou_canyuan_dating` | 塔前 / `npc_wu_chou_tq` | `ch12.huanjing` | `ch12.neidian` | 每人只能凭残图开一室、结丹只取一层（A90） |
| `qingyi_qianlan_dating` | 寒骊台 / `npc_qingyi_hl` | `ch12.cengjie` | `ch12.ruyi`、`ch12.butian` | 鼎外是乾蓝冰焰，只有金丝、蛛丝、火蟒烧不毁（A96；**改编**：原话是极阴说的，ch480，借青易之口） |

`when` 一律写「`>= 1`」；表中 `until` 另含 `ch12.done`（门禁要求）。四场的破绽全在战中试探，不设 reveal——引擎的 reveal 提示会拼出 role 名牌（`Application.cpp` 第 816–817 行），K8、K9 若揭破绽就在 `tupo` 阶段露出「三首鬼物」「鬼夜叉」。K8、K9 的文案按 12.2 不含「鬼夜叉」「三首」。

### 16.4 拍板前的原表（保留，供追溯；照笔记 E 表）

| # | 问题 | 推荐 | 不选推荐会怎样 |
| --- | --- | --- | --- |
| 1 | 开局怎么接 | **patch `zhuzhong.lua` 末尾补一行 teleport** | 在 `ch11_dongfu` 挂开局挂点违反「不在上一章图上挂开局」，金青那一路要改 |
| 2 | 章节卡 | **「第十二章」「虚天殿」** | 改「残图」会与第 8 章物品名撞；「虚天殿」第 11 章已出，不算剧透 |
| 3 | 残图收集 | **不收集，门闸读锦帕** | 「收集几块」与原著一人一图相反 |
| 4 | 白珍珠 | **n01 `take` 看返回值** | 留在包里成死物 |
| 5 | 曲魂离队 | **`party.remove`，夺舍躯体另建 role** | 改 `qu_hun_shadan` 名牌推翻第 10 章「三段三 role」 |
| 6 | 之后的同伴 | **只以写死友军进编成** | `party.add` 要配对 `remove`、存档留人，第 13 章接口变脏 |
| 7 | 境界 | **全章 31 / 31** | 升 32 与原著「结丹初期的小子」相反，第 13 章没了升境节点 |
| 8 | 战斗几场 | **4 场** | 5 场则第 13–14 章只剩 6 场 |
| 9 | 元婴名牌 | **一律 `CoreLate`** | 更高的枚举没有 |
| 10 | 鬼冤之地的解谜 | **迷路、走偏回岔口** | 纯对白谜题地图无用；做成战斗与 ① 重复 |
| 11 | 鬼王战 | **三波** | 一波显不出越打越大 |
| 12 | 冰火道限时 | **日历计时、永不超时** | 真超时致死是死局；不计时「五日」只剩台词 |
| 13 | 铁火蚁耐久 | **三波、波间不回、不可逃** | 一波打完，耐久无从谈起 |
| 14 | 宝光阁取舍 | **两件都到手** | 只许一件，③ 或第 13 章逃命断线 |
| 15 | 极妙幻境 | **两段抉择、不失败** | 失败分支要回档点与第二条路，55 分钟装不下 |
| 16 | 拜师 | **三选一，只拜极阴往下** | 真拜青易要改写 ch458 以后整条线 |
| 17 | 多方混战 | **不加第三阵营** | 要动 `BattleState` 与 AI，是引擎大项 |
| 18 | 占位数据 | **就地改写 ③、删另一张与两个 role** | 留着会被规则 25 当真编成查、selftest 误用 |
| 19 | 辟邪神雷 | **道具施法，战后收余数** | 常驻法术没有「耗尽」；不做则 ①④ 少了原著解法 |
| 20 | 玉如意 | **只作 story** | 做成兵刃会被第 13–14 章当手段 |
| 21 | 噬金虫 | **新法术** | 只在旁白则 ①②④ 缺主手段 |
| 22 | 死亡写法 | **同行者只写倒下** | 与第 11 章 16.1 第 25 条口径不一 |
| 23 | 不演的场 | **判据③** | 玩家知道他不知道的事 |
| 24 | 章末 | **停在灵泉，不 teleport** | 本章自己回天星城，替第 13 章定了开局 |
| 25 | 规则 25 与探针 | **登第 12 章、探针挪第 13 章** | selftest「同一乌丑移入第 11 章」那条红 |
| 26 | 路径行动 | **13 条全 inquire** | 设 `purchase` / `challenge` 就违反殿内禁斗、无店（A39）；不设则殿内的规矩与情报只剩旁白 |
| 27 | 支线 | **2 可选 0 必经** | 全作必经账若要本章出一条，见 16.2 第 10 条 |
| 28 | 经济 | **0 店、灵石两处兜底** | 殿内开店违背原著；硬门闸要钱即死局 |
| 29 | 地图 | **新 7 张、旧图 0 处**（后改 10 张，16.1 第 37 条） | patch 旧图违反纪律 56（旧图一个字不动）；新图多于必需则美术与 `--check` 成本白涨 |
| 30 | 借来的宝物 | **全做 story** | 做成战斗道具会在后两章被重复消耗 |
| 31 | 九曲灵参 | **不建配方** | 要定辅药，原著没有 |
| 32 | 小空间采药 | **一次、现有灵草 2** | 「一整天」只剩台词 |
| 33 | 文氏父女的地点 | **挪到荒岛东岸** | 另建海面图要多画一张 |
| 34 | 卷轴 | **主线必经** | 做成可选可能卡死在寒骊台 |
| 35 | 废金丝蚕 | **讨要 / 不要** | 不给讨要，第 13 章 ch522 的旁白（服五色圆珠的两只活下来）无物可依；硬给则违背「他讨来的」这一笔 |

---

## 17. 施工纪律（沿用前十一章，外加本章的十条）

沿用 `docs/ch04-design.md` 第 8 节、`docs/ch05-design.md`–`docs/ch10-design.md` 各第 17 节与 `docs/ch11-design.md` 第 17 节（第 50–58 条，含 53a）。本章另加：

59. **曲魂只离一次、队伍留空**：`party.remove("qu_hun_shadan")` 只在 `duoshe.lua`、看返回值；`party.add` 本章 0 处；临时同伴只以写死友军进编成；`qu_hun_shadan.json`、`wu_chou.json`、`jin_qing.json`、`miao_zhanglao.json` 一个字不动。
60. **境界不动、名牌封顶**：`realm.advance / cap / demote` 本章 0 处；元婴与玄骨名牌只到 `CoreLate`，文案写「看不透」；他对谁境界的判断只许写成他的猜测（如 A40 他猜万天明是元婴中期）。
61. **脚本 `teleport` 只传图 id 或图 id ＋ 坐标**：跨图落 default spawn（每张图脚本落点那个 spawn 标 default）；只有鬼雾三处岔口用坐标，坐标照抄进 `zoupian_a/b/c.lua` 与 `kScriptTransfers` 注释；不写 spawn 名；`lingquan.lua` 不 teleport。
62. **门闸只读旗标与必给物品、不花钱、不放在单向落点上**：锦帕门闸在 n07 送人之前；portal / facility 只用 `require_flag ＋ deny_text_key`（key 按规则 20 命名 `ch12.block.<短名>`）；每一处 `take` 看返回值、零块走得通（n24、n32 三支）。
63. **不制造死局**：冰火道永不超时；幻境、拜师、玄晶道错选只换一句再选；宝光阁两件都到手；走偏只回岔口。
64. **同格不叠、once 必有旗标**：走偏三处、`trigger_yutai`、`trigger_caiyao`、`trigger_yihai` 各占一格，与主线挂点、spawn、落点错开；`once=true` 的每一个都有 `set_flag`；NPC 挂点不设 once、脚本开头判 done_flag。
65. **战斗口径**：四场 `hero_absent false`、友军写死、`killable_by` 非空且 ⊆ `weaknesses`、③ 友军不配金类；胜负只看气血归零；`drops` 空，物品由脚本 `give`；本章 0 遭遇区。
66. **名字按节点解禁、同行者只写倒下**：12.2 就是清单；名牌跟着 role 走（怪影 → 少年 → 玄骨、黑袍人 → 元瑶是换 role）；「萧诧」全章一次、「极炫」一句、「九柄」不写七十二同出。
67. **韩立不知道的不写、章末停在鼎盖**：13 第 12 条那些场——情报、旁白、路径行动、物品描述都不许写到；`ch12.lingquan.*` 的最后几条是鼎盖、银狼、弃府的心念；不 `ending()`；「未完待续」是章节卡的事，文案里 0 处。
68. **enter 挂点不压 spawn**：teleport 落地不触发；跨图首个 enter 挂点一律是落点前方横贯通道的触发带，落点、存档点、打坐设施在带的落点一侧（16.1 第 36 条）。

## 18. 施工偏差（施工阶段填）

各路原稿在 `notes/ch12/deviations-{A,B,C}.md`；这里按路去重收录，交接给别路的缺项已在集成时消掉的不再列。**白名单口径**（三路共有的一条，只记一次）：契约 6.4 把 `data/text/ch12*.json` 整体划给内容路 C，派工另把 `ch12_maps.json` 给了 A1、把 `ch12_{items,path,quests,objectives,battle}.json` 五份给了 B，以派工为准；`ch12.battle.*`、`item.desc.*`、`magic.desc.*`、`ch12.path.*`、`ch12.quest.*`、`objective.ch12.*` 与十个地名 key、`ch12.block.dixue` 只在各自那一份里出现（重复 key 会让 `loadGameData` 整个起不来）。

### 18.1 内容路 C（脚本与主线文案，2026-10-13）

1. 胜利旁白落在各场景前缀（`ch12.guiwang/tihun/ruyi/xuangu.win`），不 talk `ch12_battle.json` 的 `.win` / `.lose`（派工把该文件划给别路；那八条目前无读者，与第 11 章同况）。要改用那八条，只换四个脚本里 `.win` 那一句（`xuangu.lua` 的 `ash` 一句随之删，免得重复「化灰」）。
2. 取消：抉择之前无状态改动的，取消即 `return`（n01、n10 把拨日 / 扣珠挪到抉择之后，动作不增不减）；抉择落在状态改动之后的 n05、n20 两处、n23、n24，取消按某一支照走、不 `return`，免得重踩重复拨日、领物、离队。
3. 「长约半尺、土黄」的灵参描述落在 n17 开金盒那一句（n16 只到化身落网），`give("story_jiuqu_lingshen", 1)` 仍在 n16。
4. n33 完成后再对话：`return` 之前补一句旁白（元瑶章末不撤）。
5. `caiyao.lua` 灵草用 `herb_huangjing_zhi` ×2、年份 300（现有灵草、年份上限 2000）；O-13 派工前没给 id，集成时协调者认定就用它，不再改（文案不点名灵草，日后要换只动 `give` 那一行）。
6. 走偏落点照抄 `genmaps_ch12.py` 的 `FORK_A/B/C`：(27,33)、(18,24)、(30,16)；三格都不与任何对象同格。脚本里 `teleport` 之后还有一句 `talk`（同图回岔口，演完再说「已站回岔口」）。
7. `give` / `advance_days` 在 `api.lua` 里只 emit、没有返回值，无从「看返回值」；`take`、`party.remove`、`magic.learn`、`battle` 全部看了（神雷余数在 `n > 0` 分支里才 `take`）。没有用 `repeat … until pick`：无头驱动不回填时 `choice` 恒为 nil，会死循环。
8. n18 两条回选单的传音：「拜青易」接极阴一句，「都不拜」接玄骨一句（第 7 节「各接对方一句传音」的落法）。
9. n05 少年自称「本座」、n06 自报名号：名号句只出现一次（「萧诧」恰 1 处），不带「上人」。
10. n29 讨要废蚕：13.1 质感边界禁用「儒衫老者要灭废蚕、他的托词、老者错以为他穷」那一套，改成他向名义师尊极阴讨下、极阴一句「随你」。

### 18.2 内容路 A（A1 地图、A2 美术与音频登记，2026-10-13）

1. **`region` ＝ `luanxinghai_xutian`**：4.2 写「由协调者定一个值」而协调者备注没给，沿 `luanxinghai_*` 前缀自定；引擎与 validate 只要非空字符串，要改只动生成器 `REGION` 一处、重跑。
2. **四条横贯触发带在 spawn 前方第 3 格**（照第 11 章 `trigger_chudao` 先例），不是紧贴的下一行：tushan `trigger_dengdao` (22,28) 5×1、dadian `trigger_rudian` (19,34) 5×1、guiwu `trigger_tupo` (24,42) 7×1、binghuo `trigger_guangbi` (6,10) **1×8 竖带**（光壁在西、往东走）。核过：把带当墙，从 default spawn 出发够不到该图任何别的挂点 / NPC / 门。
3. **为让单格 enter 挂点必经而收窄的地形**（施工图只说位置）：tushan 光罩 `trigger_guaizhao` (24,10) 在洞口前一格宽坡道南口；binghuo 黑沙漠西缘沙丘留一格沙口放 `trigger_shamo` (55,13)，石殿门一格宽、`trigger_shidian` (71,13) 在门内；taqian 塔前一格宽石径，`trigger_taqian` (30,10) 与塔门 `trigger_tamen` (30,7) 隔 3 格；寒骊台 `trigger_taijie` 在台壁唯一缺口 (20,24)（＝台阶顶）。
4. **interact 挂点画成实心石栏 / 石台**（building gid 8，站旁边按，第 11 章生成器同写法）；寒骊台「洞口」画成岩隙（实心 gid 5），不画成水面。
5. **唯一一对门**：`portal_dixue` 在 tushan (24,6)，只能从南进，落 dixue `taijie` 朝上，`require ch12.tuizhu`、`deny_text_key ch12.block.dixue`；`portal_tushan` 在 dixue 南沿 (15,23)，落 tushan `dongkou` 朝下、不设闸。两侧落地按住方向键都不弹回。
6. **BGM**：tushan→`bgm_wild`、dixue→`bgm_cave`、dadian→`bgm_indoor`、guiwu→`bgm_night`、xiaojie→`bgm_valley`、binghuo→`bgm_cliff`、baoguang→`bgm_indoor`、taqian→`bgm_cave`、hanlitai→`bgm_cliff`、lingquan→`bgm_cave`；全是 `MAP_BGM` 已有曲子，元组不动，只加注释表与 `docs/audio.md` 第 12 节。
7. **两个 role 不另做 look**：`zi_ling_xutian` → `ch11_zi_ling_zhenrong`、`yuan_yao_jiedan` → `ch10_yuan_yao`（同一个人换张脸反而认不出）；人形 23 个 role、新 look 21 条。
8. **玄骨两段同一张脸**：`ch12_xuan_gu` 以 `ch12_xuangu_shaonian` 为 base（A23），只换松绿袍（A55）；少年版穿黑袍茶缘（曲魂躯壳的斗篷色）。黑袍人 `ch12_yuan_yao_heipao` 以 `ch10_yuan_yao` 为 base，黑斗篷 ＋ 低兜帽盖住眉眼，名牌之外不露身份。
9. **熔岩画成了水**：冰火道两条熔岩是 gid 3 挡路格，烘焙器一律画成水面、`zones` 不覆写水格；现有主题没有熔岩地面、16.1 第 29 条不许加新主题，只把夹道两侧与黑沙漠铺 `scorched`、粒子加 `ember`。真要熔岩得加主题或水面变体（待办）。
10. **荒岛北坡岩顶是林冠**：`wild` 主题岩顶固定 `forest`，`structures.style` 只有 cave / valley 能改、都不对；地面已用 `zones` 铺成枯土 / 礁滩 / 沙滩（A3「草木不生」），北坡岩顶的绿留着。
11. **推断成房子的岩体改指认为 rock**：tushan 北坡（3,3–44,10）、南岸两道岩坎、东岸三块礁石；xiaojie 东南小石山两块；hanlitai 台面洞口岩隙（11,8–12,9）。tushan 中部石屋保留为房子。宝光阁玉桥用 `zones.road = flagstone` 改成石桥（路格挨水原被画成栈桥木板）。
12. **战斗背景的 `map` 字段**：② 填 `ch12_binghuo`；③ `b12_neidian_duoding`、④ `b12_hanlitai_xuangu` 都填 `ch12_hanlitai`（n29、n31 都在寒骊台）。

### 18.3 内容路 B（数据，2026-10-13）

数值原是内容初值、未经 BattleHand 量，归平衡路（契约 6.3 末条：测量记录写进编成 `note`）。**平衡路已调过 10 个敌方 role**（18.5）：下面第 2、3 条已改成 `data/roles/*.json` 的现值（整改第 1 轮，2026-10-13）；调前的数只留在 18.5 第 3 条的「调前 → 调后」表里，**照这里改数就会把平衡改回去，改之前先看 18.5**。

1. **四场共同**：`hero_absent false`、`can_escape false`、`defeat_is_fatal true`、`spirit_stones 0`、`drops []`，友军写死、不写 `wave`。`rewards.cultivation`（施工图只要 > 0）：① 360、② 360、③ 300、④ 480。`terrain`：① `forest`、② `field`、③④ `secret`；③ 旧名「内殿夺鼎混战」换成「器灵出鼎」，四场名字里的词都不早于各自场景。
2. **三个境界最高的敌人（现值，平衡后）**：`xuan_gu` CoreLate 气血 / 法力 450 / 96、攻防速 36 / 40 / 34、`actions 2`，③ 做友军时同一份数；`sanshou_guiwu` CoreLate 800、攻防速 60 / 54 / 28；`tiehuo_yiqun_da` CoreMid 500、40 / 42 / 22、`actions 2`。十个敌方 role 都远低于境界锚点（结丹后期锚点 1300 / 102 / 82），原因见 18.5 第 4 条；`xuan_gu` 法力 96 的来由见 18.5 第 2 条。（内容路初值是玄骨 1400 / 860、88 / 64，三首 1500、88 / 62，大群 1200、72 / 58——已作废。）
3. **蓄势与架势**：① 三首每 3 回合 ×1.8 全体；② 大群 ×1.6 全体；③ 银形 ×1.8 全体；④ 玄骨 ×2.0 单体（倍率、`every`、`actions` 平衡路都没动）。架势（现值）：炜吾 3、鬼夜叉 4、灰影 5、怪虎 5、三首 6、蚁群 3、大群 3、火形 5、银形 6、玄骨 4（初值三首 8、蚁群 4、大群 7、玄骨 8，已作废）。
4. **五行**：`langxing_qiling_yin` 的 `element` 取 0 不取金——取金则火克金，③ 里玄骨的魔火会对它加成，与「只有玩家的金收得了」相背；`langxing_qiling` 与两种蚁群取火（水克火，冰冻术有加成）。元婴群像的 `element` 随手配，不参战。`zi_ling_xutian.magics` 留空（规则 25 只要她的剑）。
5. **三门法术**：`magic_shijin_chongqun` 金、power 22、needMp 16、needRealm FoundationLate、`boost hits`；`magic_bixie_shenlei` 木、power 70、needMp 0（只经道具 `talisman_bixie_shenlei`）；`magic_qinghuo_lei` 火、power 30、needMp 24、needRealm CoreEarly。不建 `magic_xiuluo_shenghuo`。
6. **物品**：`material_lianjing` price 6000、`tradeable false`（同 `material_jinleizhu` 写法）；story 一律 price 0、`tradeable false`。描述按 13.1 质感边界去掉几样：辟火宝衣不写冰蚕丝，披风不写两层，虚天鼎不写缩到掌上，金雷竹小箭不写插在白骨上、不写残魂（n31 给时他还不知道）。
7. **`ch12_battle.json` 另写了 `.win` / `.lose`**（照 `ch11_battle.json` 的拆法，引擎不读，见 18.1 第 1 条）；③ 写「收服」、④ 写「圣火反噬」，0 处「杀死玄骨」「自焚」（编成 `note` 里原有一处也已改说法）。
8. **路径行动打探文案 43–58 字**：`tests/PathActionTests.cpp` 第 1123 行要求打探 40–150 个汉字（第 13 节没写这条），初稿四条偏短已加长。K12 `wuchou_canyuan_dating` 不让乌丑叫「韩师弟」：改口是 n26 的事，K12 的窗口在 `huanjing → neidian`，早于它。
9. **目标链 R-3 提示**：七条照抄 3.4 原句，其余 26 条自写；n17 不写「玄骨还在那里」（`npc_xuan_gu` 在 `lingshen` 撤场），n30 不写「鼎破了」（狼是破鼎而出，鼎还在）。Z2 任务文案不点名狼首傀儡、蛇卫（接取在 `neidian`、早于 `cengjie`）。

### 18.4 集成（协调者，2026-10-13）

1. **规则 25 表**照契约 5.2 登记，`MEANS_LAST_CHAPTER = 12`；五条友军来路登记前逐条过 `role_means_available`（编成里是 ally、法术在 `magics`、五行 / `power` / 境界 / `needMp` ≤ `maxMp` 都对得上，`zi_ling_xutian.weapons` 含剑）。
2. **删占位**：`battles.json` 的 `ch12.battle.xutian_hudian_kuilei.intro`（`neidian_duoding.intro` 集成前已删）与三份占位文件（两 role、一编成）同一批删。
3. **selftest 探针**挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`，第 13 章应静、移入第 12 章必报，阳性 / 阴性对照都留着。
4. **O-13** 认定 `herb_huangjing_zhi`（见 18.1 第 5 条）。
5. **契约 5.2 漏列的一处**：`tests/Ch07BattleDataTests.cpp` 第 319 行断言 `MEANS_LAST_CHAPTER` 在 7–11 之间，改成 12 后红；归测试路（放行表加 12）。

### 18.5 平衡路与测试路（测试路 B，2026-10-13；整改第 1 轮并入）

原稿 `notes/ch12/balance.md` 第 0、3 节与 `notes/ch12/tests-summary.md`「施工图要改」三条；`docs/ch12-test-review.md` T-1 第 1–6 条、`docs/ch12-gate-review.md` G-6 要求登记，判据以此为准。量尺是 `tests/BattleHand.h` 那只手（没改），意图手见 `tests/Ch12Driver.h::intentPolicy`；起点一律是驱动从 `tests/fixtures/ch11-end-{first,second}.sav` 真走到的那一刻，不手搭。

1. **四场的目标、实测与回合窗**（调后两侧相同）：

   | # | 编成 | 目标（第 8 节 / 10.2 B1） | 调前（意图手） | 调后 |
   | --- | --- | --- | --- | --- |
   | ① | `b12_guiwu_guiwang` | 三波打完、第三波怪笑被破势打断、韩立不死 | 败 15 回合，敌余 1854 | 胜 16 回合，700→580，怪笑被打断 |
   | ② | `b12_heishamo_tiehuoyi` | 三波打完、剑拳火只压到 1、韩立不死 | 败 9 回合，敌余 3231 | 胜 10 回合，700→479 |
   | ③ | `b12_neidian_duoding` | 第 1 波只有金要得了命、两名友军打不死它 | 胜 5 回合，345→129 | 胜 10 回合，619→553 |
   | ④ | `b12_hanlitai_xuangu` | 只有木要命、③ 后不回血、韩立不死 | 胜 29 回合，129→79，玄骨每手 1–2 点 | 胜 8 回合，553→404 |

   **回合窗**（`tests/Ch12AcceptanceTests.cpp::kTargets`）：① 12–20、② 7–14、③ 6–14、④ 5–12。第 8 节、10.2 B1 都没给回合数，窗按调后实测（16 / 10 / 10 / 8）各放宽约四成定；窗有牙——调前 ①② 败、③ 5 回合、④ 29 回合都落在窗外。共同判据：胜、敌方清场、韩立站着；① 另要出手记录里有「蓄势被打断」（四场只有三首带蓄势，指的就是怪笑）。章内血线（意图玩家不静养）：① 700→580，两日 → ② 700→479，两日 → ③ 619→553，不回 → ④ 553→404；章末 614 / 700（n33 拨 3 日）。

2. **调了 10 个敌方 role（三名新友军一个没动）**：只动气血 / 攻 / 防 / 架势与 `xuan_gu.maxMp`；破绽、`killable_by`、编成人数、友军构成、波次、`actions`、`charge.every / mult` 一处没改，`src/core/rules/Realm.h` 没动。

   | role | 场次 | 气血 | 法力 | 攻 | 防 | 架势 |
   | --- | --- | --- | --- | --- | --- | --- |
   | `weiwu_hanggui` | ① 第 0 波 | 420 | 0 | 44→**40** | 28 | 3 |
   | `gui_yecha` | ① 第 0 波 | 640→**520** | 0 | 60→**62** | 40 | 4 |
   | `hui_ying` | ① 第 1 波 | 720→**600** | 0 | 70→**44** | 48→**44** | 5 |
   | `shuangtou_guihu` | ① 第 1 波 | 840→**600** | 0 | 76→**49** | 52→**40** | 5 |
   | `sanshou_guiwu` | ① 首领（×1.8 不动） | 1500→**800** | 0 | 88→**60** | 62→**54** | 8→**6** |
   | `tiehuo_yiqun` | ② ×5 | 560→**220** | 0 | 58→**36** | 54→**36** | 4→**3** |
   | `tiehuo_yiqun_da` | ② 首领（`actions 2`、×1.6 不动） | 1200→**500** | 0 | 72→**40** | 58→**42** | 7→**3** |
   | `langxing_qiling` | ③ 第 0 波 | 900 | 0 | 74→**50** | 54 | 5 |
   | `langxing_qiling_yin` | ③ 首领（×1.8 不动） | 1000 | 0 | 76→**58** | 58 | 6 |
   | `xuan_gu` | ③ 友军 / ④ 首领（`actions 2`、`every 3`、×2.0 不动） | 1400→**450** | 860→**96** | 88→**36** | 64→**40** | 8→**4** |

   **`xuan_gu` 法力 860→96，协调者已裁定接受。** 理由：`BattleState::enemyDecision`（`src/core/battle/BattleAi.cpp` 第 65–89 行）只要有一门伤人法术合法、法力够，就只在法术里挑、不跟普攻比；`xuan_gu.magics` 只有魔火（power 12），对韩立防 46 一手 1–2 点，860 法力够放 71 手，④ 成了打不疼的仗（调前 29 回合 129→79）。压到 96，放完八手魔火才落到拳脚。不低于魔火 `needMp` 12：规则 25 `role_magic` 要 `needMp ≤ maxMp`（门禁审查 V5 / V5b：改 11 报、12 不报）。这是绕引擎，记技术债 G-34；**契约 2.2 的 `maxMp` 由平衡路定，须 ≥ 12**。

3. **意图手不吃药**（`intentPolicy()` 的 `pills = {}`；8.1 原无此条）：第 10、11 章的意图手是「五成吃药、先养精丹后金疮药」；本章起点能回血的只有养精丹（55）与金疮药（15），对 700 气血不值一手，照旧的话韩立一过半血就连吃十来回合、一下不打（第一侧 3 + 7 颗、第二侧 1 + 8 颗），初值下 ② 就是这样输的。其余（五成门槛、不算下一轮）照旧；第 11 章四场意图手实测也一颗没吃。神雷不在这只手的物品单上（`BattleHand` 不会用单体 `castMagic` 道具，技术债 G-36），四场胜负一律不靠神雷；神雷是不是木、收不收得了尾，由切片经真菜单放（18.6 T-2）。

4. **数值远低于锚点**：玄骨 450 / 36 / 40 对结丹后期锚点 1300 / 102 / 82，三首 800 / 60 / 54，蚁群 220 / 36 / 36。原因是那只手先拿法术逐类试探破绽（每个新 role 四五手），且四场之间意图玩家不静养、血线串起来；真人知道破绽会快得多。**混沌**：定稿那组做过一一扰动（上表 18 个数各 ±1 档，36 组两侧八场全胜），其中 7 组 ① 的怪笑没被打断（鬼夜叉攻 ±2、灰影攻 +2、怪虎攻 ±2、三首架势 ±1）——日后谁动 ① 的数，先看「怪笑被破势打断」这一条。

5. **法力不是 ② 的压力**：所有战后存档的法力恒为 470（各场开打即满，`settle` 只写回气血、不写回法力，技术债 G-35），8.2 ② 的「把法力留到第二、三波」这只手量不出来。

6. **最低合法起点**（8.1 末条、10.2 B1）：中阶灵石 0、背包里一样 `pill_` 也没有（蚀心散一并拿走）、上一场剩多少带多少。②③④ 两侧各打两版，全胜：
   - **静养版**（战前在最近的打坐设施静养十日，一日回一成）：② 光壁旁 10 回合 700→479；③ 寒骊台台下 10 回合 700→634；④ 台下 8 回合 700→551。证的是「②③④ 之前的打坐兜底走得通」。
   - **压力版**（不静养，带意图玩家一路的余血；整改第 1 轮补）：② 700→479、③ 619→553、④ 553→404。数字与意图手那一趟相同——那只手本来就不吃药、不花中阶灵石——它证的是「拿走丹药与中阶灵石、余血不回也打得过」。
   - **③ 的兜底打坐在寒骊台台下，不是 8.1 写的「塔前石室」**：n27 单向传送之后回不去塔前，按 10.2 B1「最近的打坐设施」就是寒骊台；静养时机在 n27 落地后、上台背之前（上去之后台阶顶是 n29 的触发格，下不来）。

7. **验收两处措辞的落法**：
   - 第 2 条「站在 quanbian」：n33 挂在元瑶 (15,9) 身上，搭话必站在她身旁；驱动说完话自己走回 (12,11)、朝上（`Ch12Driver.h::settleAtSpring`），所以终点断言 `position == (12,11)` 证的是「说完话站回 quanbian」。(12,11) 就是 `ch12_lingquan` 的 default spawn `quanbian`。
   - 第 5 条「面板写『结丹中期』所需 5000」：Slice Five 断言的是 `rules::cultivationNeeded()`、`game::realmText()` 两个函数的返回值与 `CultivationScene::buildMainItems` 那一行的 `disabledReason`（恰是 `pushAtStoryCap`）、硬按被 `StoryCap` 回绝，不是面板上画出来的字（同第 11 章 18.5 第 14 条）。

8. **反证（10.2 B1 的「只有……要得了命」）**：② 捂住冰冻术与四门金类法术 → 败 27 回合、敌余 1162，出手记录里有「却伤不到……的要害」；③ 韩立只防、乌丑玄骨照打 → 败 40 回合、敌余恰 1（银形压在 1）；④ 捂住青元剑芒、祭青蛟旗 → 败 15 回合、敌余恰 1（玄骨压在 1）；④ 人人只防 → 败。对照：同一份存档意图手照常打，三场全胜。手稳：四场各从两侧真实存档打三遍，结局指纹（胜负、回合、出手记录、战后存档逐行）一字不差，且与通关那一趟同一场一字不差。

### 18.6 整改记录（第 1 轮）

对应 `docs/ch12-test-review.md`（T-1–T-8）与 `docs/ch12-gate-review.md`（G-1–G-7）。两份报告本身不改。

**测试与门禁路（2026-10-13，构建槽 `ch12tfix`）**——`src/`、`data/`、`scripts/`、fixtures 零改动：

1. **T-1 / G-6**：18.5 并入平衡记录（回合窗及来由、10 个 role 调数、`xuan_gu` 法力 96 的裁定与理由、意图手不吃药、quanbian 一格、Slice Five 断言的是函数返回值、③ 兜底打坐在寒骊台）；18.3 第 2、3 条改成 `data/roles` 现值，初值标作废。契约 2.2 补「`maxMp` ≥ 12」。
2. **T-2**：新切片 `Ch12Slice.TwelveTheWoodThunderThrownFromTheMenuFinishesXuanGuWhereTheSwordOnlyHoldsHimAtOne`（`Ch12Driver` 加 `FightMode::Custom` / `customFight`）：起点 n30 走完，④ 里只捂住木、照那只手打到「轮到韩立时玄骨恰 1」，三版——剑（经菜单「攻击 → 剑」）这一击类别剑、玄骨仍 1；神雷（经菜单「物品 → 辟邪神雷」）这一击类别木、玄骨 0、胜；两版在压到 1 那一刻的回合、余血、出手记录一字不差。「用掉 1 发」「用掉 2 发」（开打先放一发）各把 n31 走完：战后余数 1 / 0，都说 `ch12.xuangu.spent`、不说 `.keep`，背包神雷 0（`.keep` 只在 take 余数失败时出现，余数恰是 `item.count` 时运行期走不到，只留静态扫描）。`Ch12Ledger` 补 `magic_bixie_shenlei.element == 2`，负向「改成火」「不写五行」各必报。
3. **T-3**：元素反证 ③④ 断敌余恰 1；②③④ 断出手记录里有「却伤不到」（引擎 `BattleAction.cpp` 的半句，不是文案）。
4. **T-4**：`lidao.lua` 先断门闸、`advance_days(`、`flag.set("ch12.lidao")`、`teleport(` 四处都在再比先后；负向补「删掉 teleport / advance_days / flag.set」三条。
5. **T-5**：指纹的恒真负向改成把改过回合 / 胜负 / 出手记录 / 战后气血的 `Fight` 喂给 `fightFingerprint`（配原样复制的对照）；曲魂逐字节那条改成 `quHunErrors(bytes)` 检查函数，同长改一字节恰报哈希、多一字节两条都报。Slice Five 的纯规则对照照第 11 章 T-5 口径保留。
6. **T-6**：逐节点账的战斗收支直接断 0（`kAccounts` 每一样、每一场），负向「① 里偷偷多给 3 块低阶、节点账也多 3 块」从前一条不报、现在恰报 2 条；最低合法起点加压力版（18.5 第 6 条）。
7. **T-7 / G-1**：③ 真场景 `continue` 改 `ASSERT_NE`，补友军 `actions == role.actions`、玄骨恰 2；阴性对照 E 记在契约 2.4 末条（scratch 复制树里做，1 红 / 83 绿；还原后哈希与 `git HEAD` 相同、84 / 84 绿）。
8. **T-8**：`Ch12Ledger.TheSectionThirteenZeroWordsAreSaidNowhereInTheChapter` 扫 `data/text/ch12*.json` 的值：「上人」「炉鼎」「以身相许」「碎尸」「七十二」「化为乌有」全章 0 处（「上人」不分是不是名号，比原文严），分母先验 > 250 条、≥ 11 份，负向逐词各塞一条必报。`Ch12LexiconTests.cpp` 的 `zeroWords()` 归文案整改路，没碰。
9. **G-2**：`tools/validate.py::check_given_spell`——`give` 来路登记五行类时，核符上 `castMagic` 那门法术存在、五行含这一类、`power > 0`、无 `effect`。selftest 新段 `selftest_ch12_means`：神雷 `castMagic` 改火弹术、`magic_bixie_shenlei` 五行改火 / `power` 0 / 挂 effect、去掉 `castMagic` → 都必报。
10. **G-3（技术债 G-29，已解决）**：`CHAPTER_START`（第 4–12 章起点境界与法力上限，取自上一章终档；第 3 章不登）与 `check_learned_castable`，原第 10 章特判并进去；前面各章现数据 0 错。selftest：两门金术 `needRealm` CoreMid、`needMp` 999，青元剑芒 `needRealm` CoreLate → 第 12 章都必报，噬金虫群 470 / CoreEarly 恰边界不报、471 必报。
11. **G-4**：`Ch12EngineTests.cpp` 文件头与两段注释、契约 1.3 / 1.4 / 2.3 / 2.4 / 5.1 / 第 7 节的「落地前 GTEST_SKIP」改成现状（集成时 11 处已改 `FAIL()`），契约实现记录补一句。
12. **G-5**：selftest 两处「必报」判据各加 `"wuji_haishou" in error`。
13. **G-7**：技术债新记 G-34（`enemyDecision` 有法术合法就不比普攻）、G-35（`settle` 不写回法力）、G-36（`BattleHand` 不会用单体 `castMagic` 道具）。

终证 `build-ch12tfix.log`（21:43）：`VALIDATE_OK`、`MAPGEN_IN_SYNC`（75 张）、`SELFTEST_OK`（391 条、未如期抓住 0）、`ARTGEN_IN_SYNC`（678 个），0 个编译警告；ctest 1945 / 1946，`EXITCODE=1`。唯一一红 `PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters`：`ch12.path.ziling_guiwang_dating.text` 只有 34 个汉字（下限 40，18.3 第 8 条），`data/text/ch12_path.json` 21:02 被文案整改路改写，不归本路，已交协调者。`Ch12*` 84 条全绿。


### 18.7 整改记录（第 1 轮·文案与美术，2026-10-13，构建槽 `ch12fix`）

对应 `docs/ch12-review.md` 初审（3.0，C-1–C-12）。报告本身不改；逐 key 的改前 / 改后概述留在持久笔记 `notes/ch12/fix-content.md`（不入库）。

1. **范围**：`data/text/ch12_{yizhi,guiwu,dadian,binghuo,neidian,battle,items,objectives,path}.json` 改写 82 个 key、新增 `ch12.guiwang.catchup`（554 → 555 条）；`scripts/ch12/guiwang.lua` 战前加一句 talk、`yihai.lua` 一句改旁白——旗标、门闸、`advance_days`、`teleport`、`battle` 调用零改动。
2. **C-1（HIGH）**：第 13 节第 10 条点名不许复刻的五句（夺舍挑身子、鬼王一队只逃一人、「我姓阮」、收记名赠环、讥阴阳环）按骨架拆开重写。A47（一队遇鬼王仅一人逃出）从 K8 `ziling_guiwang_dating` 挪到 `tupo.legend`，K8 只讲鬼物畏雷（16.3 已回写）。
3. **C-2（HIGH）**：31 个 key 去掉照原著镜头留下的质感项；n06 互问来历的先后全换（talk 次序不动，内容在槽间重排）；n10 外貌全删、改由旁人叫出「极阴祖师」「蛮胡子」；另顺手删了 `baishi.ring` 指环颜色（13.1 n18 质感项）。
4. **C-3（MEDIUM，协调者裁定只动文案）**：紫灵、葛笠留在土坡等他探路，`guiwang.catchup` 写二人循路赶上；`yihai.ziling` 改旁白，13.1 Z1「在场」只剩韩立（已回写）。
5. **C-4–C-10（LOW）**：`lingquan.again` 改成与位置无关；`fanlian` 补前因、改「旗是玄骨留的后手」；① 开场白删鬼音；玄骨一律点名；「半日」改「一整日」对齐脚本拨 1 日；「拳头大」删（1.1 A111–A114 没写大小）；18 个四字选项补到 5–20 字；「倒下了」7 → 3、「他」起头旁白 112 → 102、「一道」24 → 21。
6. **C-11（LOW）熔岩**：`data/visual/maps.json` 的 `ch12_binghuo` 加 `"liquid": "lava"`；artgen 新增 `mapmat.lava()` 与 `mapwater.paint_lava()`，熔岩格不进 meta.json 的 water 列表（运行时只读 meta.json，免得打蓝白波光），改放 ember 7 个与 kind `lava` 的光 10 盏。其余 676 个产物逐字节不变（sha256 比对，先对改一字节的副本报 `NOT_SAME (1)`）。
7. **C-12（LOW，协调者裁定保留）**：4 条 `.win`、4 条 `.lose` 不动，第 11 章先例。
8. **自检**：`lcs_check --from 416 --to 499` 555 条 over 0；禁词与首见扫描 0 违例；终证 `build-ch12fix.log`（22:17）1946 / 1946、四门全过、`EXITCODE=0`（第一次构建那一红是 K8 改写后只剩 34 字，加长到 53 字后转绿）。

复验第 2 轮 **PASS 4.0**（`docs/ch12-review.md` 末节）。

### 18.8 协调者收尾（2026-10-13）

1. **内容复验剩余 LOW（R2-1–R2-6，交原整改代理收）**：n06 槽间重排后先后打架（`xiaocha.age` 删「看完再告诉你」、`judge` 补抛玉简一拍）；`huanjing.opt_go` 改「上前去扶她」；`qunxiang` 四处众人反应留两处；K12 `wuchou_canyuan_dating` 改从「残图只开得动一扇门」说起，「第一层」留给 `cengjie.rule`；`rongyan.wall` / `rongyan.grass` 两句自出写法；冰火道草地改红草——`maps.json` 加 `"grass_tint": "fire"`，artgen 新增 `mapmat.grass_fire()` 与 `mapground.tint_grass()`，只作用于 `ch12_binghuo`（崖顶外沿的灌木玩家走不到，仍绿，不改）。lcs 555 条 over 0；其余 677 个产物逐字节不变。
2. **R2-7（施工图回写）**：16.3 K8「效果」栏去掉 A47；13.1 Z1「在场」只剩韩立。
3. **门禁 G-8**：`check_learned_castable` 遇到有 learn 来路、`CHAPTER_START` 却没登的章直接报错；selftest CH12 段补「删掉第 12 章起点必报」与第 4–12 章起点逐章对终档（18 条＋1 条对照），selftest 391 → 411。
4. **测试 R-1**：`Ch12EngineTests.cpp` 友军存在性由缺项即过改成 `ASSERT_NE`。
5. **第 13 章接口**：第 12 章真档 `ch12-end-{first,second}` 已对第 13 章施工图第 2 节逐项核过（修为 4072 / 4011 比估值高约 1200，坐关约 2 次；妖丹 0 / 0；其余与估值一致），记在第 13 章协调者备注，施工时以它为准。
