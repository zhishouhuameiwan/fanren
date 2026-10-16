# 第 12 章设计：虚天殿

> 对应原著 ch416–499（84 章）。目标时长 **55 min**（大纲第 12 章条目；A 主线 ＋ B 必经口径，按幕分摊见 3.3 与第 14 节）。**起止境界都是结丹初期（31 / 31）**：本章 `realm.advance`、`realm.cap`、`realm.demote` 各 0 处，结丹中期留给第 13 章（ch529）。
> 与第 11 章的衔接：第 11 章章末停在 `ch11_dongfu` 门外、置 `ch11.done`；本章 **patch `scripts/ch11/zhuzhong.lua` 末尾补一行 `teleport("ch12_tushan")`**（落 `andeng`，标 default），第 11 章的图与其余脚本一个字不动（拍板 1）。曲魂 `qu_hun_shadan` 随队进本章，在 n05 离队，此后队伍为空（契约 E1）。
> 权威依据：`notes/ch12/research.md`（研究路 2026-10-12，id、旗标、章号、数字只许照抄）；`H:/Work/Kys/docs/大纲.md` 第 12 章条目；大纲与原著冲突之处以原著为准，待回写（对不上的二十处见 1.2）。引擎增补只有两条判据项，另见契约 `docs/interfaces-p3-ch12.md`（E1 空队伍、E2 同一 role 先友后敌）。
>
> P3 · **2026-10-12** · **施工图阶段：本文不改任何代码、脚本、数据、地图，也不碰 `data/flags.json`**（旗标清单见第 11 节，交协调者登记）。第 11 章正在同一棵工作树里集成，**`tests/fixtures/ch11-end-*.sav` 尚未生成**——第 2 节起点资源全部标「估」，并写明哪几项必须拿真档核。
> 文中「拍板 N」＝ 16.1 第 N 条（研究笔记 E 表同号）；「O-N」＝ 16.2 开放问题；「A N」＝ 1.1 表行号；「B N」＝ 1.2 表行号；「契约 E1 / E2」是引擎判据，与拍板号不是一套。
> **3.2 只写框架件与脚本动作，不摘原著镜头顺序与台词**；每段必须落地的剧情事实与不许碰的原著质感在 13.1，写手与复验共用那一张表。

---

## 0. 这一章要做到什么

1. **一张旧锦帕忽然醒了。** 金青邀他去破一处古修士遗址（ch416），同行四人一个个倒在里面，曲魂被一个散尽元婴的老怪夺了舍（ch426–427）；那人从自己肋骨里取出一块残图，他第八章得来的锦帕随之亮起、恒指西北（ch429）。**残图不是在遗址「取」的，是在遗址「醒」的**（B1）——游戏不发新残图，入殿门闸读第 8 章的 `story_jinpa_ditu`。
2. **他第一次一个人。** 曲魂离队（n05 `party.remove`）之后队伍为空，直到第 13 章；临时同行的人只以写死友军进编成（拍板 6）。这是全作第一次在空队伍下跑地图与战斗（契约 E1）。
3. **三百年一现的空中玉殿。** 持图者注灵入图、穿罩入殿（ch431）；正魔元婴云集、星宫执法长老只管外殿（ch432–436）。玩家第一次被一群打不过的人围住，只能结盟、忍让、藏拙——大厅 0 战斗、0 店。
4. **三关不是三场仗。** 鬼冤之地是**迷路**（三处岔口，走偏被鬼雾推回）＋鬼王一战；冰火道是**限时**（五日、禁飞，日历计时、永不超时）＋铁火蚁耐久战；宝光阁是**取舍**（每人一样，他多取一件）＋极妙幻境的**心境**（两段抉择、不失败）。大纲「解谜、耐久战、夺宝竞速」改为「迷路解谜、限时耐久、取舍与心境」（B2）。
5. **他拜了一个想杀他的人为师。** 极阴看中的是血玉蜘蛛（ch454–455），玄骨劝他拜、因为九曲灵参的丹方只在玄骨手里（ch456）——不是「为自保投靠」（B5）。
6. **夺鼎是别人的大战。** 元婴混战他只能退后（ch482）；他抢到的是一只收服不了的狼形器灵（ch485）和一口打不开的鼎（ch491、ch499）；乌丑死于玄骨之手（ch489），玄骨强融修罗圣火失控身亡（ch493）。**章末停在灵泉密室出殿之后、鼎盖打不开**，置 `ch12.done`，不 `ending()`、不 teleport、不演逆星盟（拍板 24）。

---

## 1. 硬约束（原著核对）

章号照研究笔记 A 表（研究路 2026-10-12 逐条用 `tools/novel/novel.py find/read` 核过）；本节把 A 表一百二十六条按幕合并成行，**行号保留 A 号**，供 3.0、3.2、13.1 引用。原文片段只作出处定位，不进游戏文案，本表不再抄。

### 1.1 硬约束表

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| **幕一** | **遗址（ch416–431，n01–n07）** | | |
| A1–A3 | 金青来邀，他与曲魂随行出城；两日答复、两个月后到荒岛；两颗珍珠是金青与胡月一同发现的 | 416 | n01；`advance_days(62)`；曲魂此时仍在队；`take("story_bai_zhenzhu", 1)` 看返回值，0 也照走（拍板 4）|
| A4–A7 | 他以北斗两仪阵破最外层幻阵（研究阵法才二十余年、靠辛如音的心得），用了三个月；翅恶是火系妖灵、须水属法器克制 | 417–418 | n02 阵势三选一、旁白不夸大他的阵法；`advance_days(90)`；第二段布水旗，翅恶不进编成 |
| A8–A13 | 看守怪人的花篮、双剑是洪荒古宝；噬金虫数万首次当众实战；战利品有花篮与红光画轴；石柱是封灵柱；他自承有七十二口青竹蜂云剑；曲魂加巨力符推倒石柱 | 419–422 | n03：`magic.learn("magic_shijin_chongqun")`、`give` 花篮与画轴、`portal_dixue` 开；看守怪人不打（拍板 8）；一件 `weapon_qingzhu_fengyun_jian` 代表七十二口 |
| A14–A17 | 池中七霞莲、千年石乳，七霞莲是妖冠蛇的诱饵；脂阳鸟封在画轴里、是阴物克星；石蝶被鬼爪抓走 | 422–425 | n04 抉择「展画轴」；妖冠蛇不进编成；死亡只写倒下（拍板 22）|
| A18–A22 | 金青奔过白骨时被射杀；怪影问他是极阴还是极炫门下；**九柄是他同时操控飞剑的上限**；曲魂被夺舍；引魂钟对夺舍后的分身无效 | 426–427 | n05：画轴燃尽 `take`、`party.remove("qu_hun_shadan")`、「敲钟」必落空（钟不扣）；「极炫」只此一句；全章「九柄」口径，不写七十二口同出 |
| A23–A26 | 曲魂躯体被炼成眉清目秀的少年；对方散尽元婴、顶多相当结丹后期（他也这样估）；未修玄魂大法时已活六百余岁 | 427–428 | NPC `xuangu_shaonian`（名牌「少年」）外观；`xuan_gu` 名牌 CoreLate（拍板 9）；n06 台词可用数字 |
| A27–A29 | 玄骨从肋骨里取出残图；他的锦帕现金剑、恒指西北；玄骨化血光遁走 | 429 | n06；B1；n07 起「循图」；NPC 撤场 |
| A30 | 途中遇文樯父女，文樯是魁星岛旧识 | 430 | n07（地点改编到荒岛东岸，拍板 33）；文思月首见 |
| **幕二** | **入殿（ch431–436，n08–n11）** | | |
| A31–A33 | 虚天殿悬于约千丈高空、金罩罩住；持图者注灵入图、白光护体穿罩；别的修士手持同样的锦帕 | 431 | n08 门闸 `item.count("story_jinpa_ditu") >= 1`；「一人一图」口径（拍板 3）|
| A34–A38 | 玄骨传音：你也有虚天残图，合作；三百年一次；极阴自称「乌某」；蛮胡子的残图是别人孝敬的；九曲灵参能让凝婴机会倍增（玄骨说）| 432–433 | n09 结盟的饵；n10 台词；名牌「极阴祖师」|
| A39–A42 | 殿内禁制只容漂浮术一类小法术；他猜万天明是元婴中期、万法门门主；星宫两名执法长老只管外殿 | 433–435 | 大厅 0 战斗、0 条 `challenge`；名牌只到 CoreLate（O-9）；内殿不管（n30 化身符的由头）|
| A43–A45 | 传送阵过去就是外殿；紫灵自称假丹期，高价买了一块残图来碰运气 | 436 | n11 → `ch12_guiwu`；新 role `zi_ling_xutian`（FoundationLate，B8）；路径行动 `ziling_canyuan_dating` |
| **幕三** | **鬼雾与小空间（ch436–459，n12–n19）** | | |
| A46–A49 | 鬼冤之地结丹修士多被众鬼吞噬，曾有一队遇神智大开的鬼王全灭；葛笠只想摘寿元果、原路返回；连快结丹中期的修士都陨落于此 | 437–439 | n12；① 的由头；n13 开头葛笠退场；Z1 玉真人遗骸 |
| A50–A52 | 勾魂鬼音制住紫灵；鬼夜叉有肉身、白日可行；四柄飞剑分化为八柄、半路四柄化为乌有 | 440–444 | n13 抉择「喝醒」；① 第 0 波；**不是损剑**，散去的是分化之影（B9）|
| A53–A55 | 他对元瑶说同为结丹初期、提起百余年前与「曲师叔」见过她；绿袍年轻修士（玄骨）从鬼雾毫发无伤走出 | 445 | `yuan_yao_jiedan` CoreEarly；n14 台词（曲魂已不在）；花园 `npc_xuan_gu` 外观同 A23 |
| A56–A57 | 采药一整天，一个月后有阵传回大厅；峡谷里分玄晶道与熔岩路 | 447–448 | n15、`trigger_caiyao`；n19 二选一 |
| A58–A61 | 九曲灵参只有纯金之物捉得住；四象玄武阵旗是玄骨所给；灵参本体长约半尺、土黄；极阴来袭时他锁定九只飞剑 | 449–453 | n16 金网、抉择「换旗」；`story_jiuqu_lingshen` 描述；与 A20 同口径 |
| A62–A66 | 极阴抛出血玉蜘蛛的蛛丝，蜘蛛是取鼎最佳灵兽之一；他拜极阴为记名弟子；玄骨：丹方只他一人知道；紫灵与李姓青年不闯冰火道 | 454–456 | n18 依附的真实缘由（B5）、`give("story_xuanyin_huan", 1)`；紫灵在 n15 之后退场（B6）|
| A67–A68 | 极阴借他白犀佩（防火）；两条窄道各立三四尺高石碑 | 458 | n19 `give("story_baixi_pei", 1)`；`ch12_binghuo` 入口道具 |
| **幕四** | **冰火道与宝光阁（ch459–474，n20–n26）** | | |
| A69–A72 | 冰火道上所有修士无法腾空飞行，他施展罗烟步疾奔；必须五日之内通过、否则传送阵关闭；黑沙漠太大，绕路不可能 | 459–460 | 全图不做飞行；n20 抉择「罗烟步 / 常速」；`ch12.binghuo_ri` 计时（拍板 12）；n21 绕路一支被时限挡回 |
| A73–A75 | 铁火蚁唯冰水法术方能伤；他向元瑶要啼魂兽为护送代价，元瑶先开价青火雷 | 462–464 | ② 破绽水、金（噬金虫）；n21 战后 `give` 啼魂兽、鸣魂珠；青火雷不给他 |
| A76–A79 | 铁火蚁后反吐的炼晶；石殿五六十丈、是冰火道出口；四条通道三通阁楼，每人只取一样；蛮胡子坚持把寒冰珠送他 | 466–467 | n22 `give("material_lianjing", 1)`；n23；n24 取舍（拍板 14）；寒冰珠 n19 借、n23 转赠，同一件 |
| A80–A84 | 宝光阁高三十余丈、分两层；星宫长老口中他是「结丹初期的小子」；二层光罩里数十件古宝；噬金虫结成黑甲多取一件，手上是披风与铜环 | 468–470 | `ch12_neidian` 西区；**全章 realm 31 的出处**；n24 两件都到手：`story_xuese_pifeng`、`story_wuse_tonghuan` |
| A85–A89 | 极妙幻境长廊白云仙音；幻象自称小妹、唤他「四哥」；婆罗珠在幻象靠近时灼热；内殿是五层青石巨塔；蛮胡子当众把一名结丹修士斩成数截 | 471–474 | n25 色段、惧段两处抉择，婆罗珠是提示；n26 只写倒下 |
| **幕五** | **内殿夺鼎（ch475–499，n27–n33）** | | |
| A90–A94 | 每人只能凭残图开一面石室门；狼首傀儡被蛮胡子赤手砸碎；台阶是寒骊台唯一出入口；台背青石是空心夹层，内有陈旧发黄的卷轴 | 475–478 | n27 台词（乌丑）；Z2 收残骸、不进编成；东区布局；n28 `give("story_neidian_juanzhou", 1)` |
| A95–A98 | 金丝蚕服天青花撼鼎失败；鼎外是至阴至寒的乾蓝冰焰；血玉蜘蛛四级顶峰；蛮胡子对蜘蛛施狂暴之术 | 479–481 | n29 抉择「讨要废蚕」与旁白；`story_xueyu_zhizhu` 不改 |
| A99–A101 | 鼎中古宝化双头火狼破鼎而出；他手持黄红两色玉如意；五枚连成一串的铜环套住银狼 | 483–485 | ③ 第 0 波；胜后 `give("story_yu_ruyi", 1)`；③ 破绽「金」的来路 |
| A102–A105 | 星宫长老以化身符现身；一只血玉蜘蛛被斩成两截；皇鳞甲救他一命；补天丹落入蛮胡子手中 | 486–488 | n30 旁白；蜘蛛物品不扣；**韩立未得补天丹** |
| A106–A110 | 乌丑死于玄骨鬼爪；血玉蜘蛛一窝只两卵活；补天丹配九曲灵参可使凝婴达三四成；玄骨交出丹方玉简；虚天鼎扁圆两耳三足、高约四尺 | 489–491 | n31 旁白与台词，乌丑不是他杀的（B4）；`give` 丹方、虚天鼎 |
| A111–A114 | 修罗圣火彻底失控，玄骨在火光中化为灰烬；冰焰凝成乾蓝冰珠；拾得一截未被炼化的肋骨 | 493–494 | ④ 胜利文案「圣火反噬」（B3）；敌方可写灰烬（拍板 22）；`story_qianlan_zhu`、`story_xuangu_leigu`（第 13 章用）|
| A115–A119 | 噬金虫找出极阴留下的追踪暗记；卷轴是内殿示意图、红线通密室；灵眼之泉被迁入内殿；万年灵乳一小口回复全部法力；他只要养魂木的根 | 495–498 | n32 → 灵泉密室；n33 `give` 灵乳、根须 |
| A120–A123 | 出口传送到海面；鼎盖如生铸、打不开；银色巨狼他无法控制；他打算弃府经星城传送阵去外星海 | 498–499 | n33 旁白与章末心念一句；玉如意只作 story（拍板 20）|
| A124–A126 | 结丹中期在 ch529 前后；逆星盟首见 ch500；「银月」首见 ch615、「紫罗极火」首见 ch731 | 529 / 500 / 615 / 731 | 本章不升境；三词全章 0 处；器灵只叫「狼形器灵」|

**口径**：上表里「本章演出来的」与「只在旁白里交代的」分别标在 3.0 表一、表二；**不演的**（韩立不在场、或只有作者知道的）不在表里，列在 3.0 表三。

### 1.2 大纲、lore、人物总表与占位数据里对不上的地方（已核，待回写）

| # | 在哪 / 原说法 | 复核（原著）| 本章以什么为准 |
| --- | --- | --- | --- |
| B1 | 大纲「古遗址取**虚天残图**」；人物总表 `jin_qing`「引出虚天残图异变线索」| 他的残图是 ch331 得的锦帕（第 8 章已给）；遗址里是玄骨取出他自己那块，他的锦帕随之异变（ch429）| 回写大纲「古遗址：残图苏醒」；不新给残图 |
| B2 | 大纲「三关闯殿」、玩法「解谜、耐久战、夺宝竞速」；lore 4.6 序列 | 实为大厅 → 鬼冤之地 → 花园 → 小空间（ch447）→ 冰火道 → 石殿四通道（ch467）→ 宝光阁 → 极妙幻境（ch471–473）→ 内殿五层塔 | 回写大纲与 lore 4.6；「夺宝竞速」删 |
| B3 | 大纲「玄骨**自焚**」；lore #46 | 强融冰焰为修罗圣火、被逼急时失控（ch493），不是主动自焚 | 回写「强融圣火失控而亡」；④ 文案照此 |
| B4 | `data/roles/wu_chou.json` note「不以其死亡收场」；占位 `b12_neidian_duoding` 敌方乌丑 | 乌丑死于玄骨（ch489），韩立在旁未出手 | `wu_chou.json` **一个字不动**（第 11 章验收 14）；③ 里乌丑改友军（O-11）|
| B5 | 大纲「为丹方与自保依附极阴岛」；lore 5.1 | 极阴图的是血玉蜘蛛（ch454–455）；玄骨以丹方劝他拜（ch456）| 回写大纲 |
| B6 | lore 5 #33「紫灵此后原路退出」| 与他一同走出鬼雾到花园（ch446），之后不闯冰火道（ch456）| 回写 lore；`zi_ling_xutian` 退场旗标 `ch12.xiaojie` |
| B7 | 大纲人物缺金青、葛笠、蛮胡子、万天明；人物总表把蛮胡子 / 万天明 / 天悟子并成一行 `manhuzi_deng` | 四人各有戏份（ch432–488）| 本章分建 role（第 5 节）；回写人物总表拆行 |
| B8 | `data/roles/zi_ling.json` CoreEarly | ch436 自称假丹期 | `zi_ling` 不动；新建 `zi_ling_xutian`（FoundationLate）|
| B9 | 摘要「八柄回收四柄化为乌有」「九只」| ch442 是分化之影散去；九是同时操控的上限（ch426）| 全章只写「九柄」，不写损剑 |
| B10 | `data/roles/yuan_yao.json` QiRefining11 | ch445 已是结丹初期，对外自称姓阮 | 旧 role 不动；新建 `yuan_yao_heipao`、`yuan_yao_jiedan` |
| B11 | `data/roles/wen_qiang.json` QiRefining5 | ch430 两鬓斑白、筑基中期 | 旧 role 不动；新建 `wen_qiang_zhuji`（名牌「文樯」）|
| B12 | 人物总表 `wen_siyue`「韩立以长老身份顺手解围」| 他杀了追堵的三名毒龙会修士；她被同门逼婚一事他**没插手**（ch430–431）| 回写；n07 照原著 |
| B13 | 人物总表 `xuan_gu`「元婴期」「临时之盟合谋取鼎」| 约相当结丹后期（ch428）；盟约是「九曲灵参换除极阴」（ch433），合谋取鼎只在 ch490 | 回写 |
| B14 | 人物总表 `yin_yue` 行、「银月（收服后定名）」| 「银月」首见 ch615 | role id 用 `langxing_qiling`（名牌「狼形器灵」）；回写 |
| B15 | 占位 `b12_xutian_hudian_kuilei`、role `xutian_hudian_kuilei`、`duoding_xiushi` | 原著三关无守卫傀儡；内殿傀儡由蛮胡子砸碎（ch475–476）| **删**（协调者，连同 `looks.json`、`data/text/battles.json` 两条旧 intro；O-7）|
| B16 | 第 11 章文案 `ch11.wuchou.map`「那**半幅**虚天残图」| ch405 原文没有「半幅」| 第 11 章集成后删两字（O-12），本章不碰 |
| B17 | 大纲「乾蓝冰焰 / 乾蓝冰珠」| 冰焰 ch480；凝珠 ch494 章题「乾蓝珠」、文中亦称「乾蓝冰珠」| 物品名「乾蓝冰珠」|
| B18 | lore 六「宝光阁二层规则：限取 1 件（469–471）」| 规则在石殿处讲、对三座阁楼都成立（ch467）| 回写出处改 467 |
| B19 | 第 11 章接口「残图疑在隐煞门孙姓门主手」| ch431 他用的是自己的锦帕；孙姓门主那份本章不再提 | 照接；孙姓门主线不演 |
| B20 | 摘要「披风」| ch470–471 只叫「披风」，「血色披风」首见 ch499 | 物品名「血色披风」；ch470 段文案写「暗红披风」|

### 1.3 境界曲线与上限

- **原著**：ch416 起全程结丹初期——ch445 他自称「同为结丹初期」、ch468 星宫长老称他「结丹初期的小子」；结丹中期在 ch529（外星海，第 13 章）。
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
| 极阴岛：乌丑嫁祸、极阴附身、残图疑在孙姓门主手 | ch432 极阴本尊入殿 | n10 `jiyin_zushi` 现身；孙姓门主线不演（B19）|
| 妖丹（五级 0、六级 0–1）| — | 不用、不设门闸 |
| 洞府 `ch11_dongfu` 可回 | ch499 他才起意弃府 | 本章**回不去**（殿内单向）；第 13 章开局由第 13 章定 |
| 时间：单次 ≤ 3650 | — | 本章单次 ≤ 90（3.3）|
| 灵石：从 `ch11-end` 读 | ch469、ch496 两处「装几块灵石」| 两处 `take(..., 4)` 都有兜底（第 9 节）|
| 规则 25：`MEANS_LAST_CHAPTER = 11`；selftest 探针在 `wu_chou` ＋ `b12_neidian_duoding` | — | C2：改 12；探针挪 `wuji_haishou` ＋ `b13_waixinghai_lieyao`、`probe["chapter"] = 12`（拍板 25）|
| 词表 | — | C3：补 81 词 ＋ 禁词「紫罗极火」（第 12 节）|
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
| `story_jinpa_ditu` | 1 / 1 | `scripts/ch08/shadan.lua` 第 8 行给；第 10 章两份真档实读 1；第 9–11 章 0 处 `take` | n08 入殿门闸 | **必须** |
| `story_bai_zhenzhu` | 1 / 1 | `scripts/ch11/zhuzhong.lua` `give` | n01 `take` | 必须 |
| `weapon_qingzhu_fengyun_jian` | 1 / 1 | 第 11 章 n30 必成配方 | 规则 25「剑」的来路之一 | **必须** |
| `story_xueyu_zhizhu` / `story_shijin_chong` / `story_yinhun_zhong` | 1 / 1 / 1 | 第 8、10 章给，第 11 章不扣 | n18、n05 旁白 | 必须 |
| 其余（`story_lvhuang_jian`、`story_miaoyin_yaopai`、`story_jinzhi_lingpai` 各 1；`material_yaodan_liuji` 0 / 0；`bottle` capacity 9；`knownWeaknesses` 第 10 章 36 / 35 个 ＋ 第 11 章新揭）| 见左 | 第 10–11 章；第 11 章验收 2、n16 | 都不读、不用；本章新 role 全未揭 | 可选 |
| `mapId / position` | patch 前 `ch11_dongfu` 门外；patch 后 `ch12_tushan` 的 `andeng` | 第 11 章 n31 ＋ C1 | n01 挂点就在这一格 | **必须**（T1 重生后核）|

**C12 入口边界**：

- **入口版 ＝ C1 patch 之后由测试路从真 `ch11-end` 重跑生成的档**（T1）；生成后逐项核上表九个「必须」，并记 SHA 与字节数。任何一项与本表不同，先查真驱动证据，再改本节与 3.3 的绝对日，**不改 182 的增量**（O-3），不手改 fixture。
- 入口版的 `mapId` 必须是 `ch12_tushan`、`position` 是 `andeng` 那一格；`ch11.done = 1`、`ch12.*` 全 0。
- 两侧差异来自真实历程（灵石差 20 等），本章不据此另开分岔、也不拉平；意图玩家两侧走同一组选项（第 8 节）。
- 本章**没有一处资源门闸**：入殿读的是第 8 章必给的锦帕；灵石、珍珠、画轴、神雷的 `take` 全看返回值、不足照走（第 9 节）。

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
| 4 | 少年自报姓名、残图醒 | 428–429 | n06 | 入殿的钥匙（B1）；玄骨线起点 |
| 5 | 文氏父女 | 429–431 | n07 | 文思月首见；他不插手的分寸（B12）|
| 6 | 循图入殿、玄骨结盟 | 431–433 | n08、n09 | 入殿门闸；「九曲灵参换除极阴」的盟约（B13）|
| 7 | 群像入殿、星宫规矩、传送阵 | 432–436 | n10、n11 | 正魔元婴群像；殿内规矩是此后一切的前提 |
| 8 | 土坡三人、鬼雾迷路、鬼王 | 436–444 | n12、n13 | 迷路解谜；战斗 ①、神雷首用 |
| 9 | 黑袍人现为元瑶、一日一月 | 444–447 | n14、n15 | 第 10 章一面之缘的下文；进小空间 |
| 10 | 捕九曲灵参、反脸 | 448–453 | n16、n17 | 「换旗」的伏笔；凝婴之物到手 |
| 11 | 极阴逼收记名弟子 | 453–456 | n18 | 拜师抉择（B5）；玄阴环 |
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
| k | 乌丑之死 | 489 | n31 | 玄骨所为（B4）|
| l | 破阵两三日、海面分手 | 498 | n33 | 过场 |

**表三　不演（韩立不在场、无从得知；10 条）**

| # | 原著事件 | 章 | 玩家怎么知道 |
| --- | --- | --- | --- |
| α | 卜绪与艳女闯关 | 459–460 | 不知道——「卜绪」0 处 |
| β | 星宫二人密谈 | 465、486 | 不知道 |
| γ | 万天明一行商议 | 458、477 | 不知道 |
| δ | 玄晶道上的事 | 465 | 他走的是熔岩路——「银光鼠」「血冰林」0 处 |
| ε | 元瑶对妍姐姐的独白 | 498 | 不知道 |
| ζ | 元婴们出殿后搜塔 | 498 | 不知道（他已离开）|
| η | 孙姓门主那份残图的下落 | — | 原著本章不提（B19）|
| θ | 玄骨肋骨里的功法 | 494 | 第 13 章破出才知道——只许「肋骨」，「玄阴经」0 处 |
| ι | 天星城出了大事 | 499 | 第 13 章开场才说 |
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
| n07 | 离岛 | 土山荒岛（东岸）| NPC；teleport | ch429–431 | 4 |
| **幕二** | **入殿** | | | | |
| n08 | 入殿 | 外殿大厅 | **门闸（锦帕）** | ch431 | 8 |
| n09 | 旧盟 | 外殿大厅 | NPC；二选一循环 | ch432–433 | 0 |
| n10 | 群像 | 外殿大厅 | 二选一 | ch432–435 | 8 |
| n11 | 传送阵 | 外殿大厅 | teleport | ch436 | 3 |
| **幕三** | **鬼雾与小空间** | | | | |
| n12 | 土坡 | 鬼冤之地 | 叙述 | ch436–438 | 0 |
| — | 走偏（三处）| 鬼冤之地 | **迷路**：teleport 回岔口（同图）| ch437–438 | 0 |
| n13 | 鬼王 | 鬼冤之地 | 二选一循环；**战斗 ①** | ch439–444 | 0 |
| n14 | 元瑶 | 鬼冤之地（花园）| 二选一 | ch444–446 | 0 |
| n15 | 一日一月 | 鬼冤之地（花园）| teleport | ch446–447 | 1 |
| n16 | 灵参 | 小空间 | 二选一（记旗）；领物 | ch448–451 | 0 |
| n17 | 反脸 | 小空间 | 叙述 | ch451–453 | 0 |
| n18 | 拜师 | 小空间 | NPC；**三选一循环**；领物 | ch453–456 | 0 |
| n19 | 峡谷口 | 小空间 | 二选一循环；领物；teleport | ch456–459 | 0 |
| **幕四** | **冰火道与宝光阁** | | | | |
| n20 | 熔岩路 | 冰火道 | **限时**：两处二选一（常速 ＋1 日）| ch459–460 | 1 ＋ 0–2 |
| n21 | 啼魂 | 冰火道 | 二选一循环；**战斗 ②**；领物 | ch460–465 | 1 |
| n22 | 炼晶 | 冰火道 | 领物 | ch466 | 0 |
| n23 | 石殿 | 冰火道 | 领物；四选一（合流）；teleport | ch466–468 | 0 |
| n24 | 取宝 | 宝光阁 | 付灵石（兜底）；**二选一取舍**；teleport（同图）| ch468–470 | 0 |
| n25 | 幻境 | 极妙幻境 | **两段心境抉择** | ch471–473 | 1 |
| n26 | 塔前 | 内殿 | 叙述 | ch473–474 | 0 |
| **幕五** | **内殿夺鼎** | | | | |
| n27 | 层阶 | 内殿 | 二选一（Z2）；teleport（同图）| ch475–477 | 0 |
| n28 | 卷轴 | 寒骊台 | 领物 | ch477–478 | 0 |
| n29 | 如意 | 寒骊台 | 二选一；**战斗 ③**；领物 | ch478–485 | 0 |
| n30 | 补天丹 | 寒骊台 | 二选一（合流）| ch485–488 | 0 |
| n31 | 玄骨 | 寒骊台 | 领物；**战斗 ④**；领物 | ch489–494 | 0 |
| n32 | 残魂 | 寒骊台 | 二选一；付灵石（兜底）；teleport（同图）| ch494–495 | 0 |
| n33 | 灵泉 | 灵泉密室 | NPC；领物；**章末** | ch496–499 | 3 |

**主动抉择 25 处**（n01、n02、n03、n04×2、n05、n06、n09、n10、n13、n14、n16、n18、n19、n20×2、n21、n23、n24、n25×2、n27、n29、n30、n32），其中回选单的循环 9 处（n02、n04 第二处、n09、n13、n18、n19、n21、n25×2），真改状态的 6 类（`ch12.huanqi`、`ch12.quba_xian`、`ch12.xinjing`、`ch12.jincan`、`ch12.z2`、常速拨日），其余只换一句后合流；**必打战斗 4 场**（第 8 节）；支线 2 条，都可选（第 6 节）。

### 3.1 每一处挂点怎么开始（`mode` / `once`）

分法沿用前十一章：**这件事是他做的（interact），还是发生在他身上的（enter）。** 不用 `auto`。
**挂 NPC 的五个节点**（n06、n07、n09、n18、n33）没有 once：脚本开头看自己的 done_flag，已置则一句 `return`；`flag.set` 写在脚本里。NPC 的出场靠 visible / hidden 旗标——**portal 与 facility 不读这两个旗标**，门闸只用 `require_flag ＋ deny_text_key`。
**`once=true` 的触发器每一条都写 `set_flag`（＝ done_flag）**；循环抉择、门闸不满足、取消，都在置旗标之前 `return`（与第 10 章 27 同口径）。

| # | 挂点 | 地图 | `mode` | `once` | `guard_flag` | `set_flag` | 战斗 | 为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| n01 | `trigger_dengdao` | `ch12_tushan` | `enter` | ✓ | `ch11.done` | `ch12.dengdao` | — | 船一靠岸就有人迎出来。落在 `andeng`（default spawn，C1 的落点）|
| n02 | `trigger_huangwu` | `ch12_tushan` | `interact` | ✓ | `ch12.dengdao` | `ch12.pozhen` | — | 走到黄雾前布阵的是他。坡上黄雾一格 |
| n03 | `trigger_guaizhao` | `ch12_tushan` | `enter` | ✓ | `ch12.pozhen` | `ch12.tuizhu` | — | 怪人是自己落下来的。光罩前一格（与黄雾错开）|
| n04 | `trigger_shuichi` | `ch12_dixue` | `interact` | ✓ | `ch12.tuizhu` | `ch12.qixialian` | — | 凑到池边看莲的是他。池边一格 |
| n05 | `trigger_shizhu` | `ch12_dixue` | `enter` | ✓ | `ch12.qixialian` | `ch12.duoshe` | — | 怪影从柱里扑出来。北壁石柱前一格（离池 ≥ 3 格）|
| n06 | NPC `npc_shaonian`（`xuangu_shaonian`）| `ch12_dixue` | NPC | — | —（visible `ch12.duoshe`）| —（脚本置 `ch12.canyuan`）| — | 走过去问话的是他。hidden `ch12.canyuan` |
| n07 | NPC `npc_wen_siyue`（`wen_siyue`）| `ch12_tushan` | NPC | — | —（visible `ch12.canyuan`）| —（脚本置 `ch12.lidao`）| — | 上前搭话的是他。东岸；hidden `ch12.lidao` |
| n08 | `trigger_rudian` | `ch12_dadian` | `enter` | ✓ | `ch12.lidao` | `ch12.rudian` | — | 循图而至。落在 `tongdao`（default spawn）|
| n09 | NPC `npc_xuan_gu`（`xuan_gu`）| `ch12_dadian` | NPC | — | —（visible `ch12.rudian`）| —（脚本置 `ch12.jiumeng`）| — | 传音是对方先发，但走过去应的是他。hidden `ch12.chuansong` |
| n10 | `trigger_qunxiang` | `ch12_dadian` | `enter` | ✓ | `ch12.jiumeng` | `ch12.qunxiang` | — | 群修入殿是落在他眼前的事。大厅中段柱下 |
| n11 | `trigger_chuansongzhen` | `ch12_dadian` | `interact` | ✓ | `ch12.qunxiang` | `ch12.chuansong` | — | 踏上传送阵的是他。北端尽头 |
| n12 | `trigger_tupo` | `ch12_guiwu` | `enter` | ✓ | `ch12.chuansong` | `ch12.tupo` | — | 一落地就见土坡三人。落在 `tupo`（default spawn）|
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
| n20 | `trigger_guangbi` | `ch12_binghuo` | `enter` | ✓ | `ch12.xiagu` | `ch12.rongyan` | — | 穿壁即入禁制。落在 `guangbi`（default spawn）|
| n21 | `trigger_shamo` | `ch12_binghuo` | `enter` | ✓ | `ch12.rongyan` | `ch12.tihun` | ② | 沙漠是撞上的。黑沙漠西缘 |
| n22 | `trigger_jingzhu` | `ch12_binghuo` | `interact` | ✓ | `ch12.tihun` | `ch12.lianjing` | — | 放虫吞柱的是他。沙漠中心晶柱 |
| n23 | `trigger_shidian` | `ch12_binghuo` | `enter` | ✓ | `ch12.lianjing` | `ch12.shidian` | — | 一进石殿就被星宫长老叫住。东端石殿门内 |
| n24 | `trigger_yuqiao` | `ch12_neidian` | `enter` | ✓ | `ch12.shidian` | `ch12.quba` | — | 玉桥匾额落在他眼前。落在 `yuqiao`（default spawn）|
| — | `trigger_yutai` | `ch12_neidian` | `interact` | ✗ | — | — | — | 一层玉台只给描述、不给物（拍板 14）|
| n25 | `trigger_changlang` | `ch12_neidian` | `enter` | ✓ | `ch12.quba` | `ch12.huanjing` | — | 幻境是踏进去的。石室出口 |
| n26 | `trigger_taqian` | `ch12_neidian` | `enter` | ✓ | `ch12.huanjing` | `ch12.neidian` | — | 塔前众人已在。塔前 |
| n27 | `trigger_tamen` | `ch12_neidian` | `interact` | ✓ | `ch12.neidian` | `ch12.cengjie` | — | 以残图开门的是他。塔门（与塔前错开 ≥ 2 格）|
| n28 | `trigger_qingshi` | `ch12_neidian` | `interact` | ✓ | `ch12.cengjie` | `ch12.juanzhou` | — | 敲青石的是他。台背 |
| n29 | `trigger_taijie` | `ch12_neidian` | `enter` | ✓ | `ch12.juanzhou` | `ch12.ruyi` | ③ | 一上台就卷进撼鼎。台阶顶 |
| n30 | `trigger_jitan` | `ch12_neidian` | `enter` | ✓ | `ch12.ruyi` | `ch12.butian` | — | 冰封与白虹都是落在他身上的。祭坛前 |
| n31 | `trigger_dongkou` | `ch12_neidian` | `interact` | ✓ | `ch12.butian` | `ch12.xuangu` | ④ | 走去洞口应玄骨的是他。洞口旁 |
| n32 | `trigger_xiaojian` | `ch12_neidian` | `interact` | ✓ | `ch12.xuangu` | `ch12.canhun` | — | 拾起小箭的是他。台面另一角 |
| n33 | NPC `npc_yuan_yao_lq`（`yuan_yao_jiedan`）| `ch12_neidian` | NPC | — | —（visible `ch12.canhun`）| —（脚本置 `ch12.done`）| — | 泉边人是先到的，开口谈的是他。灵泉；章末不撤 |
| — | `portal_dixue` / `portal_tushan` | 荒岛 / 地洞 | 门 | — | `require_flag ch12.tuizhu`（只北向那扇）| — | — | 本章唯一一对门；`deny_text_key ch12.block.dixue`（规则 20）|
| — | `facility_dazuo_dadian`；`facility_cunji_*` ×7 | 大厅；七张新图 | 设施 | — | — | — | — | 打坐（meditate、`effectiveness 100`）让等候的日子有事做；每图一处存档点，与 spawn 相邻、不压挂点 |

- **同一格只挂一个对象**（引擎同格只取对象表第一个 trigger、不分 `mode`；规则 16 报同格第二个 `enter`「踏不响」）：五处落在 default spawn 上的 `enter` 挂点（`andeng`、`tongdao`、`tupo`、`guangbi`、`yuqiao`，第 11 章 n01 先例）那一格不许再放别的对象，存档点放相邻格；荒岛上黄雾与光罩、地洞里池边与石柱（≥ 3 格）、内殿塔前与塔门（≥ 2 格）、寒骊台五处（台背、台阶顶、祭坛前、洞口旁、台面另一角）互不同格；**四处脚本落点格**（`ch12_neidian` 的石室、寒骊台下、灵泉，`ch12_guiwu` 的三处岔口）不与任何触发器同格。
- **四场必打全挂在目标链的必经挂点上**（n13、n21、n29、n31），每一处的 `set_flag` 都是下一节点的 `guard_flag`——绕不过去。
- **规则 30**：同图同 role 的对象旗标首尾相接、不重叠——`ch12_guiwu` 的 `npc_zi_ling`（`tupo` → `guiwang`）与 `npc_zi_ling_hy`（`yuanyao` → `xiaojie`）；`ch12_xiaojie` 石山组（`fanlian` → `baishi`）与峡谷口组 `_xg`（`baishi` → `xiagu`）；`ch12_neidian` 塔前组 `_tq`（`huanjing` → `neidian`）与寒骊台组 `_hl`（`cengjie` → `butian`；乌丑、玄骨 → `xuangu`），`npc_yuan_yao_tq` 与 `npc_yuan_yao_lq`（`canhun` 起）。逐个对象见第 5 节。
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
| 6 | `{"ch12.shidian", "scripts/ch12/shidian.lua", "ch12_neidian"}` | `trigger_shidian` | `yuqiao` |
| 7 | `{"ch12.quba", "scripts/ch12/quba.lua", "ch12_neidian"}` | `trigger_yuqiao` | 石室坐标（同图）|
| 8 | `{"ch12.cengjie", "scripts/ch12/cengjie.lua", "ch12_neidian"}` | `trigger_tamen` | 寒骊台下坐标（台阶脚南一格，同图）|
| 9 | `{"ch12.canhun", "scripts/ch12/canhun.lua", "ch12_neidian"}` | `trigger_xiaojian` | 灵泉坐标（池边，同图）|

*五元 `{doneFlag, script, toMap, reusableObject, hostMap}`（3 条；触发器 `trigger` / `enter` / `once false` / `guard_flag == doneFlag` / `set_flag` 空，脚本 0 处 `flag.set`）*

| # | 五元 | 落点 |
| --- | --- | --- |
| 10 | `{"ch12.tupo", "scripts/ch12/zoupian.lua", "ch12_guiwu", "trigger_zoupian_a", "ch12_guiwu"}` | 第一岔口坐标 |
| 11 | `{"ch12.tupo", "scripts/ch12/zoupian.lua", "ch12_guiwu", "trigger_zoupian_b", "ch12_guiwu"}` | 第二岔口坐标 |
| 12 | `{"ch12.tupo", "scripts/ch12/zoupian.lua", "ch12_guiwu", "trigger_zoupian_c", "ch12_guiwu"}` | 第三岔口坐标 |

- 五元写法此前只用于「回城阵」那种可重复的跨图挂点；同图带坐标回岔口是第一次——测试路核 `ObjectiveRoute` 不把它当链上一步（O-5）。
- **待定（新开放问题，交协调者）**：`scripts/common/api.lua` 没有读玩家坐标的命令（2026-10-12 列过全部导出函数），一份 `zoupian.lua` 分不出是从哪一条错路进来的，「各回各的岔口」写不出来。两条路二选一：三处都推回同一个点（第一岔口前），或拆成 `zoupian_a/b/c.lua` 三份（上表 10–12 的 script 随之改）。研究笔记改定前，本稿照笔记写一份脚本。
- `lingquan.lua`（n33）**不 teleport**——第 13 章 patch 它（拍板 24）。
- 门钥匙（`kOffChainDoorKeys`）：`portal_dixue` 的 `ch12.tuizhu` 在链上，不必另登；链外门钥匙：无。

### 3.2 脚本动作与分支

本节仅规定状态操作、控制流和挂点，不提供叙事镜头、示例台词或转述顺序。剧情事实与质感禁用项只按 13.1 核验；对白另行原创。挂点属性以 3.1、日期以 3.3 为准。所有 `take`、`party.remove`、`magic.learn` 都看返回值：失败只记一句、照走（第 9 节退路）。

| 节点 / 脚本 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- |
| n01 dengdao | `take("story_bai_zhenzhu", 1)`；`advance_days(62)`；choice；置 `ch12.dengdao` | 两支只换一句后合流；取消 return |
| n02 pozhen | choice 循环；`advance_days(90)`；置 `ch12.pozhen` | 阵势三选一，错选一句后回选单；取消 return |
| n03 guairen | choice；`magic.learn("magic_shijin_chongqun")`；`give` `story_hualan` 1、`story_zhiyang_huazhou` 1；置 `ch12.tuizhu` | 两支都以噬金虫收场、合流；置旗后 `portal_dixue` 可过 |
| n04 qixialian | choice；choice 循环；置 `ch12.qixialian` | 第一处只换一句；第二处「再等」一句后回选单，直到选展画轴 |
| n05 duoshe | `take("story_zhiyang_huazhou", 1)`；`party.remove("qu_hun_shadan")`；choice；置 `ch12.duoshe` | 敲钟一支必落空、钟不扣；两支合流 |
| n06 xiaocha | choice；`flag.set("ch12.canyuan")` | 开头 `canyuan` 已置 → 一句 return；两支合流（据实也不泄底）|
| n07 lidao | `advance_days(4)`；顶格 `flag.set("ch12.lidao")` 与 `teleport("ch12_dadian")` | 开头 `lidao` 已置 → return；不插手、不打 |
| n08 rudian | `advance_days(8)`；置 `ch12.rudian` | 开头 `item.count("story_jinpa_ditu") >= 1`，不足一句、return（数据上不可达）|
| n09 jiumeng | choice 循环；`flag.set("ch12.jiumeng")` | 开头 `jiumeng` 已置 → return；「再想」一句后回选单 |
| n10 qunxiang | `advance_days(8)`；choice；置 `ch12.qunxiang` | 「不让」一句后照让；合流 |
| n11 chuansong | `advance_days(3)`；置 `ch12.chuansong`；`teleport("ch12_guiwu")` | 无 |
| n12 tupo | 置 `ch12.tupo` | 无 |
| 走偏 zoupian | 旁白；`teleport("ch12_guiwu", x, y)` | 开头 `ch12.guiwang >= 1` → return；0 处 `flag.set` |
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
| n21 tihun | 读 `today() − binghuo_ri`；choice 循环；battle ②；`give` `story_tihun_shou` 1、`story_minghun_zhu` 1；`advance_days(1)`；置 `ch12.tihun` | 「绕路」一句后回选单（时限挡回）；败 game_over |
| n22 lianjing | `give("material_lianjing", 1)`；置 `ch12.lianjing` | 无 |
| n23 shidian | `give` `story_poluo_zhu`、`story_qingming_zhen`、`story_huanglin_jia` 各 1；choice；置 `ch12.shidian`；`teleport("ch12_neidian")` | 四门合流（皆通宝光阁）；寒冰珠转赠只改文案、不 `give` |
| n24 quba | 灵石启阵；choice；`flag.set("ch12.quba_xian", 1 / 2)`；`give` `story_xuese_pifeng` 1、`story_wuse_tonghuan` 1；置 `ch12.quba`；`teleport("ch12_neidian", 石室)` | 灵石：`take("material_lingshi", 4)` 不成则 `take("material_lingshi_zhong", 1)`，再不成 `advance_days(1)` 照走；两件都到手，先后只换文案 |
| 玉台 yutai | 只描述 | 可反复看，不给物 |
| n25 huanjing | 两段 choice；`ch12.xinjing` 按位写（1 色、2 惧）；`advance_days(1)`；置 `ch12.huanjing` | 错选一句（婆罗珠灼热）后回选单；一次就对才置该位；不失败（拍板 15）|
| n26 neidian | 置 `ch12.neidian` | 无 |
| n27 cengjie | choice（Z2）；收支 `give("story_kuilei_canhai", 1)`、`flag.set("ch12.z2")`；置 `ch12.cengjie`；`teleport("ch12_neidian", 寒骊台下)` | 收 / 不收合流 |
| n28 juanzhou | `give("story_neidian_juanzhou", 1)`；置 `ch12.juanzhou` | 无 |
| n29 ruyi | choice；讨要支 `give("story_jinsi_can", 3)`、`flag.set("ch12.jincan", 1)`；battle ③；`give("story_yu_ruyi", 1)`；置 `ch12.ruyi` | 败 game_over |
| n30 butian | choice；置 `ch12.butian` | 「溜向台阶」被叫回，合流 |
| n31 xuangu | `give` `story_jiuqu_danfang`、`story_xutian_ding` 各 1；`give("talisman_bixie_shenlei", 2)`；battle ④；`take` 神雷余数；`give` `story_qianlan_zhu`、`story_xuangu_leigu`、`story_jinlei_xiaojian` 各 1；置 `ch12.xuangu` | 败 game_over |
| n32 canhun | choice；灵石启阵（同 n24 三档）；置 `ch12.canhun`；`teleport("ch12_neidian", 灵泉)` | 两支结果同 |
| n33 lingquan | `give` `story_wannian_lingru`、`story_yanghun_mugen` 各 1；`advance_days(3)`；`flag.set("ch12.done")` | 开头 `done` 已置 → return；`ch12.xinjing` 各位多一句；**不 `ending()`、不 teleport** |

- 只使用既有脚本接口；本章 `realm.*` 写命令、`party.add`、`bottle.*`、`shop`、`ending` 各 0 处，不加任何 Lua 命令。
- 神雷「`take` 余数」＝ 战后 `item.count("talisman_bixie_shenlei")` 有几张扣几张，章末净增 0（拍板 19）。
- 普通资源不足的支路照常可走；全章唯一的物品门闸是 n08 的锦帕（数据上必有）。

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

- **范围**：182 ＋ 常速 0–2 ＋ 灵石兜底 0–2 ＝ 182–186。研究笔记 N.2 只记了常速那 0–2；**兜底那一日 N.2 没计**——意图玩家（中阶灵石 0 块花销、低阶 −8）两处都付得起，不影响 182，但测试路的切片断言要按「常速 / 兜底」各自加日（第 15 节）。
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
| 24 | `n24_quba` | `ch12.quba` | `ch12_neidian` | `trigger_yuqiao` | **「阁里的东西每人只能拿一样」** |
| 25 | `n25_huanjing` | `ch12.huanjing` | `ch12_neidian` | `trigger_changlang` | |
| 26 | `n26_neidian` | `ch12.neidian` | `ch12_neidian` | `trigger_taqian` | |
| 27 | `n27_cengjie` | `ch12.cengjie` | `ch12_neidian` | `trigger_tamen` | |
| 28 | `n28_juanzhou` | `ch12.juanzhou` | `ch12_neidian` | `trigger_qingshi` | **「台背那块青石的声音不对」** |
| 29 | `n29_ruyi` | `ch12.ruyi` | `ch12_neidian` | `trigger_taijie` | |
| 30 | `n30_butian` | `ch12.butian` | `ch12_neidian` | `trigger_jitan` | |
| 31 | `n31_xuangu` | `ch12.xuangu` | `ch12_neidian` | `trigger_dongkou` | |
| 32 | `n32_canhun` | `ch12.canhun` | `ch12_neidian` | `trigger_xiaojian` | |
| 33 | `n33_lingquan` | `ch12.done` | `ch12_neidian` | `npc_yuan_yao_lq` | |

文案进 `data/text/ch12_objectives.json`（R-3 提示是原创短句，不摘原著）。`ObjectiveRoute` 那三条沿这条链逐步走：荒岛 → 地洞靠 `portal_dixue`（`ch12.tuizhu` 后开），地洞 → 荒岛走回 `portal_tushan`；进图靠第 11 章 patch 那一行，此后五次换图、三次同图换区全靠 3.1 清单里的脚本 teleport（九条三元）；三条五元（走偏）不是链上一步（O-5）。**本章新增的按键提示：0 条**（问路、打坐都用既有的键，不点名物理键）。

