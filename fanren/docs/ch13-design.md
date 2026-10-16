# 第 13 章设计：逆星之乱·外星海

> 对应原著 ch500–542（43 章；开场借 ch499 末的海上相遇）。目标时长 40 min（大纲第 13 章条目）。**起止境界：结丹初期（31）→ 结丹中期（32）**；上限只由剧情抬一处（n27），中期由玩家自己在雾中岛静室坐出来（1.3）。
> **与上一章的衔接**：第 12 章停在 `ch12_lingquan` 的 `quanbian`、置 `ch12.done`、不 teleport，「出了大事」推到本章开场。本章 patch `scripts/ch12/lingquan.lua` 末尾一行 `teleport("ch13_nanming")`，第 12 章其余一个字不动（1.4、3.1 末）。
> 权威依据：研究笔记 `notes/ch13/research.md`（A–O 节；id、旗标、章号、数字一律照抄）；`docs/大纲.md` 第 13 章条目；`fanren/docs/interfaces-p3-ch12.md` 第 3 节。大纲、lore、人物总表、占位数据与原著对不上的三十二处见 1.2。
>
> P3 · 2026-10-13 · **施工图阶段：本文不改任何代码、脚本、数据、地图，也不碰 `data/flags.json`**（旗标交协调者登记）。拍板项全部「已拍板（按推荐，预授权）」。
> **3.2 只写框架件与脚本动作，不摘原著镜头顺序与台词**；每段必须落地的剧情事实与不许碰的原著质感在 13.1，写手与复验共用那一张表。
> **一件事实只在一处写死**（第 12 章审查教训）：节点 → 地图 → 挂点 → 前置 → done_flag 以 3.0 主线节点表为准；mode / once 与 teleport 清单以 3.1 为准；脚本动作与分支以 3.2 为准；`advance_days` 的日数只在 3.3；目标链只在 3.4。别处一律引用，不重抄。
> **起点依赖**：`tests/fixtures/ch12-end-{first,second}.sav` 尚未生成（第 12 章在同一棵工作树里施工），第 2 节资源全部标「估」，并写明哪几项必须拿真档核。

---

## 0. 这一章要做到什么

1. **他回到一座戒严的城。** 半月海路上他听说天星令、逆星令、正魔联手；南明岛的坊市还开着、比天星城略便宜，一夜之间被逆星盟拔成前哨；两名逆星盟追兵是本章唯一一场「人对人」的仗（①）。天星城坊市闭门、传送阵改了条件——**「封锁」是本章的经济底色，不是物价曲线**（1.2 B8）。
2. **他是买路的散修里最不像散修的那个。** 五千一人的黑价、结丹不放、七人一阵——他易容成曲姓假丹混进去，用把柄、妖丹和别人的遗物铺路。**偷渡费有三支付法，最便宜的一支一块灵石不花**（3.2 n12）。
3. **外星海是能出不能进的地方。** 奇渊岛众势力共管、黑石城严禁斗法；寸金阁里他第一次知道千叶露是什么——这条线一直拉到章末（支线 `q13_qianyelu`）。
4. **雾中岛是全作唯一一段可以慢下来的日子。** 立府、虫室、玄阴经、重炼七十二剑、诱妖取丹、炼丹、坐关——**结丹中期由玩家自己在静室里坐出来**，剧情只抬上限；毒蛟、虫魔、兽潮都是远处的雷声，他只看、只听、只躲（判据②③，3.0）。
5. **章末停在一株黑草和一句人声。** 秘市交换会换来文思月与裂风兽巢穴的位置，他蹲守数月、入洞取到黑色伴妖草——身后有人开口，置 `ch13.done`；**那人是谁、叫什么，第 14 章才说**（「风希」首见 ch543，本章 0 处）。

---

## 1. 硬约束（原著核对）

章号全部由研究路用 `tools/novel/novel.py find/read` 核过（2026-10-13，研究笔记 A 表 148 行）；本稿另核 ch510「两颗五级妖丹」与 ch524「七年后」两处。下表按幕归并（「A N」＝研究笔记 A 表行号，便于回查原文定位）；**原文片段不进游戏文案**。

### 1.1 硬约束表

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| | **幕一　南明岛（ch499–505）** | | |
| 1 | 海上所遇是三仙宗修士、奉命赴天星城；他早打算回城即弃府、经传送阵去外星海；披血色披风赶路（A1–A2、A9）| 499–500 | n01；n08 落实；披风只旁白、不做战斗手段 |
| 2 | 星宫发天星令召各宗首领与岛主觐见；正魔联手成立逆星盟，万三谷与六道极圣同任魁首；逆星令鬼牌：响应觐见者即为敌；鬼面与当日六连殿长老所示相同；外星岛已遭攻击（A3–A8）| 500 | n01；K1；两魁首只闻其名、不建 role；鬼面只一句推断（渗透已久）|
| 3 | 南明岛是到天星城前最后一个中转岛，坊市货全、比天星城略便宜；掌柜不识千叶露，玛瑙角是玛瑙兽独角，没听过的多是上古叫法（A10–A13）| 501 | `ch13_nanming`；`ch13_nanming_fangshi` `buyRate 0.9`；n02；`q13_qianyelu` 由此接 |
| 4 | 八级妖兽碰见只能逃（A14）| 501 | 全章 0 处与八级交手 |
| 5 | 逆星盟攻南明岛；苍云龙自报护法、奉王长老之命封岛；两个时辰后出示信物或功法即可离岛；开天门辛明以白水剑为信物（A15–A18）| 501–502 | n03 隐身旁观；n04；K3 |
| 6 | 他以妙音门客卿长老腰牌过关、不易容——理由是他自己的推断：元婴神识扫查之下易容瞒不过、反显心虚（ch503 第 24–31 段）。「两位长老画像早被复制」「煞气只露十之一」是他飞走之后苍、刘二人的私话（ch503 第 2、9 段；第 22 段他已飞离），判据③（A19–A21）| 502–503 | n04 读 `story_miaoyin_yaopai`；旁白只写他自己的推断；画像、煞气、「七煞决」0 处（16.1 第 41 条）|
| 7 | 追兵两人皆逆星盟正道修士、结丹初期；大汉祭白玉大印，儒生的迷幻砚放黑雾怪鸟（A22–A24）| 503–504 | ① 两敌 `CoreEarly`；大汉蓄势、儒生法术；战后两件 story |
| 8 | 五行环锁住儒生真元；此时青竹蜂云剑尚无辟邪神雷（A25–A26）| 504 | ① `magic_ji_wuxinghuan`（金、削架势）；重炼前 0 处剑上神雷 |
| 9 | 凌玉灵自称星宫外事执法；天星城已戒严，愿引他入城（A27–A28）| 505 | n06 → 天星城 |
| | **幕二　天星城（ch505–511）** | | |
| 10 | 赵护法验腰牌、于护法守城门；戒严时段外不准离住处，执法队格杀勿论（A29–A30）| 506 | n07；K5 |
| 11 | 传送阵条件：替星宫完成任务，或大战中当众击杀同阶，先前所杀不算；指环为凭证（A31–A33）| 506 | n07 给指环；K6；① 不抵条件 |
| 12 | 城内坊市全部关门；回洞府装噬金虫、采尽药园，住了近百年（A34–A36）| 507 | `trigger_fangshi_bimen` 只一句；n08，药园玩家自采 |
| 13 | 守阵者开价五千一人、三千不行，结丹给多少灵石也不放（A37–A38；经办人一律「张」，B9）| 507 | n09；n11 易容的理由 |
| 14 | 宅院外小型流水阵；七人议定凑足五千一人（A39–A40）| 508 | n10 |
| 15 | 易容青衫、自称姓曲；传送阵一次最多七人（A41–A42）| 509 | n11；n12；K4 |
| 16 | 他愿出双倍传送费；**两颗**五级妖丹买名额；两柄胡月遗物飞刀；迷魂配神识逼出张姓把柄；石碑刻「奇渊」（A43–A47；本稿复核 ch510 原文「两颗」）| 509–510 | n12 三支付法（游戏要价见 3.2）；n10 把柄；n12 末句 |
| | **幕三　奇渊岛（ch511–517）** | | |
| 17 | 外海传送阵**能出不能进**、传送符被收走（A48；「只出不进」原著查无）| 511 | n13；全章回不去内星海 |
| 18 | 许云来外星海已数年；锦衣大汉自报易敬（A49–A50）| 511 | n13；「易敬」对白自 n13 起 |
| 19 | 黑石城是奇渊岛唯一的城，整座岛众势力共管；除竞斗场外严禁斗法、违者形神被灭；时刻一位元婴坐镇（A51–A53，B4、B22）| 512–513 | `ch13_waihai` 北区；黑石城 0 战斗、0 切磋；K9 |
| 20 | 明珠轩收购海外所得；附近有妖兽海渊；竞宝厅蓝光石起价五百、紫云鹰卵三千三成交；联合大拍卖一年一次（A54–A58）| 513–515 | `ch13_mingzhu_shougou`；n14 观拍只看；K9；K10 |
| 21 | 寸金阁是城内最大原料铺，管事离寻结丹初期，古玉炼气期、专研上古丹方；外来凡人须登记（A59–A62）| 515 | n15；`ch13_cunjin_ge`；K11；炉鼎拍卖不演（B30）|
| 22 | **千叶露即八级妖兽旁伴妖草之汁**；玛瑙角按五千收进、照进价出；他以丹药与符宝换古玉玉简（A63–A65）| 516 | n15；「伴妖草」自此解禁；玛瑙角价 5000（可选）；换简旁白不扣物 |
| 23 | 海图：西边有带小型灵脉、少有人住的岛；北飞十余日后修士渐少（A66–A67）| 517 | n16 |
| | **幕四　雾中岛（ch517–534）** | | |
| 24 | 第四座无人灵脉荒岛，灵脉在一条微型山脉；吞海鱼吐雾罩岛；追四级银翼鸟误入雾海（A68–A71）| 517 | `ch13_wuzhong`；n17；文案多写「雾海」（B17）|
| 25 | 虫室五间；金丝蚕可二次进化为幻焰蛾、奇虫榜第十七，一雌一雄各喂一粒五色圆珠；九曲灵参移入药园禁地；新绿液全用来催养魂木（A72–A76）| 518 | n18（有金丝蚕多一段）；支线 Z3 |
| 26 | 肋骨灌阴气显出是玉简即玄阴经；他已修青元剑诀、不重修玄阴大法；天都尸火须元婴以上；试开虚天鼎半月无果（A77–A80）| 519 | n19；玄阴经只作剧情物品；鼎不开 |
| 27 | 炼晶是熔岩路上所得、炼入飞剑；实炼近一年；杂务由巨猿傀儡打理（A81–A83）| 519–520 | n20；文案「熔岩路上那块炼晶」|
| 28 | 化形雷劫是七级进八级之劫；红鳞毒蛟八级；金霞道士与霍氏兄弟来猎蛟；此战是日后妖修之战的导火索——作者旁白，原文明说他并不知道（ch522 第 14 段）（A84–A87）| 520–522 | n21 旁观不战；末句只写他改主意不迁府、加阵；「导火索」「大祸」0 处（16.1 第 42 条）|
| 29 | 另布两个伪洞府；六颗五色圆珠两月一颗、全部炼化；金丝蚕产卵后**两只双双毙命**（A88–A90，B19）| 522 | n22 收尽 `story_jinsi_can` |
| 30 | 引六级须把霓裳草催到四百年；西飞一月在火红珊瑚礁布四阵；先来三四只五级、继而来青色巨虾（六级，六只碧绿的眼）；聚魂钵抽魂取丹（A92–A96，B32）| 523 | n23；② 两波；「四百年」只在文案；胜后旁白 |
| 31 | 叙事跳到「七年后」；七道青光合成巨剑斩琉璃兽首；四法上人只剩元婴逃生（A97–A99；本稿复核 ch524「七年后」）| 524 | n25；`magic_fengyun_heji` 文案 |
| 32 | 虫魔之名：七八名结丹被灭；碧云门损失近三分之一，太上长老妙鹤真人数度出海；三色甲虫套颈圈逼回三人；琉璃兽皮是炼甲上料（A100–A104）| 525 | n25；妙鹤只闻其名 |
| 33 | 假虫魔驱金银两色飞虫；他当日放十万噬金虫灭了自称碧云门的一队；深海数年存下数百枚六七级妖丹；决意回府闭关二三十年（A105–A108）| 527 | n25（妖丹按比例收成可数的量）|
| 34 | 炼丹先拿五级妖丹练手、一闭三年；霓裳草对三色噬金虫失效；服红光丹修剑诀第八层，十五六年破结丹中期瓶颈；中期后又闭关二十多年（A111–A116，B12）| 528–534 | n26；n27 抬上限 ＋ 静室自冲；n28 |
| 35 | 青灵门弟子被鹰鸢兽追杀；唯一结丹长老死于二十余年前的兽潮，兽潮中两位元婴陨落（A117–A119）| 530–531 | n28 旁白；n29、K12；`ch13.shouchao` |
| 36 | 公孙杏为瘫父求药（父强修青灵玄功第六层）；至元丹三枚用妖丹炼成；他留缠玉决玉简、法器、丹药；动机是像小妹出嫁前的影子（A120–A123）| 531–533 | n29；`recipe_zhiyuan_dan`；留物只旁白（不 `give` 给 NPC）|
| 37 | 中期后可抽土石精气养灵参；换形诀施展后法力至多七成；救黄明礼，南黎岛坊市已毁；秘市须熟人引荐、地点常变（A124–A127）| 534 | n30 旁白；文案不做数值；n31 |
| | **幕五　秘市与深渊边缘（ch535–542）** | | |
| 38 | 五六级妖兽下落找听风阁；秘市一二层都无八级伴妖草（A128–A129）| 535 | K13；n31 |
| 39 | 云天啸自称东主，范夫人挽他手臂十分亲昵；交换会十三位结丹；铜片古文与兽皮书大同小异；有人拿出八级灵骨（A130–A134，B15）| 536–537 | n31；`give("story_tongpian", 1)`；「梵圣真片」0 处 |
| 40 | 裂风兽八级才产子；云天啸燃白骨阴火；十二柄寸许小剑穿透护体火焰；妙音门回内海的传送阵缺幻梦石，自飞需五六年（A135–A139）| 538–539 | n31；一击只作旁白；范夫人之邀他回绝 |
| 41 | 文思月称他「厉先生」；卓右使不知所踪、大权落入范左使与魔道之手；巢穴靠近深渊边缘（A140–A142）| 540 | n32 |
| 42 | 铜片光影三头六臂、三十六式；服五色珠后资质近三灵根；霓裳草对八级无吸引、蹲守数月；八级附近的伴妖草是黑色（A143–A146）| 541 | n33 旁白（不改 `aptitude`）；n34 `give("story_banyao_cao", 1)` |
| 43 | 身后有人开口、自称「风某」；化形第二阶相当九级、居磷火潭（A147–A148）| 542 | n34 末**只写人声**，不写名、不现身；后半句本章 0 处 |

**原著查无、本章不得写的**：「只出不进」（原文「能出不能进」）、「雾阵」、「六眼巨虾」、「困妖」（首见 ch1038）、「物价」作战乱涨价义；**留给后章的**：「梵圣真片」（546）、「风希」（543）、「碧焰酒」（543）、「风雷翅」（545）、「温天仁」（570）、「银月」（615）——全章 0 处。

### 1.2 大纲、lore、人物总表与占位数据里对不上的地方（已核，待回写）

| # | 在哪 | 原说法 → 原著 | 本章以什么为准 |
| --- | --- | --- | --- |
| B1 | lore 5 #49「韩立弃府携虫南下」归 500–502 | ch499 只是计划；ch500–502 海上与南明岛；ch507 才回洞府装虫弃府；「南下」0 命中 | 回写 lore：弃府挪到 #51 之后（507）|
| B2 | lore 把千叶露、玛瑙角列在 516、541–542 | ch501 已出现、问不到；ch516 才查明真身 | 回写 lore #54；词表首见 501 |
| B3 | lore #52「赠飞刀买通守阵二人」| 先以五级妖丹与迷魂得来的把柄逼张姓让名额，再送胡月遗物飞刀；他易容曲姓、压成假丹 | 回写 lore #52 |
| B4 | lore #53 与 4.6 #8「黑石城由多家元婴势力共管」| 是整座奇渊岛众势力联手管，黑石城是岛上唯一的城 | 回写 lore |
| B5、B23 | lore #39 炼晶来路；lore 六「结丹中期 12 柄」| ch466 与 ch519 说的是同一块炼晶；ch524 七道青光合剑、ch538 十二柄小剑 | 都不改；n20 文案「熔岩路上那块炼晶」，两处剑数各照原文 |
| B6 | ch500「万年灵乳」vs ch519–520「万年灵液」| 原文称呼不同，未明说两物 | 回避：文案一律「灵乳」，「万年灵液」0 处 |
| B7 | lore #27、`weapon_qingzhu_fengyun_jian` note「自带辟邪神雷」| ch504 此时无神雷；ch519「刚恢复的一丝」| 回避：① 不给神雷；note 由协调者改「神雷随修炼时间恢复」|
| B8 | 大纲玩法「物价波动、封锁」| ch500–542「物价」0 命中；只有略便宜、闭门、改条件、黑价、能出不能进 | 回写大纲：「战乱封锁」；不做涨价曲线 |
| B9 | 摘要偷渡价与经办人称呼 | ch507 三千被拒、五千一人；ch508 一处「黄道友」、他处皆「张道友」| 采五千一人；经办人一律「张」；「黄道友」`lidao.*` 之前 0 处，之后只许作对黄明礼的称呼（ch535，12.2、16.1 第 56 条）|
| B10 | lore #56「八级毒蛟屠人类修士」| ch521 只与金霞、霍氏交手；ch522 才说是日后导火索 | 回写 lore #56 |
| B11 | lore #58「虫魔之名系碧云门捏造」| ch527 是他的猜测；原文是极阴照齐姓道士之法派弟子冒充 | 回写 lore #58；游戏只写他的猜测与三人所说 |
| B12 | lore #59「距结丹已约十五六年」| 十五六年是修第八层到破中期瓶颈的闭关；此前另有七年猎妖、三年炼丹 | 回写 lore #59 |
| B13 | lore #63 与人物总表风希「ch541–561」「ch543 才出场」| ch541 只有空洞窟；ch542 自称「风某」；全名 ch543 | 回写 lore；本章 0 处「风希」 |
| B14 | lore #62「梵圣真片……以为普通铜片买下」| ch537 因古文近似起意；ch539 以一颗五级妖丹换得；ch541 判为妖修功法 | 回写 lore #62；物品名「铜片」|
| B15 | 人物总表 `yun_tianxiao`「范夫人政敌」| ch536–540 两人当众亲昵同席、同为夺权一方（ch540 第 14 段：大权落入范左使与魔道之手）；但 ch556 范夫人自称已成傀儡、求他除掉云天啸（第 14、16 段），ch558 设局杀之——「政敌」是跨章的事实，不错 | **不改「政敌」**；人物总表只加注「ch536–540 表面亲昵同席，实为被魔道架空的傀儡门主（ch540、ch556）」——第 14 章要用 ch554–560 杀云天啸 |
| B16 | 校对称文思月「早年丧夫」「ch538 才出现」| ch536–541 未见丧夫；ch536 即迎宾陪席 | 回避：文案不写丧夫；n31 她一出场就在 |
| B17 | 大纲与校对「雾中岛」| 原文仅 ch517 章题 1 处，文中一律「雾海」| 地图显示名「雾中岛」；对白旁白多用「雾海」|
| B18 | 大纲第 13 章条目未涉及 | 毒蛟与巨龟、虫魔、公孙杏与青灵门、黄明礼、秘市交换会 | 回写大纲：人物补「青灵门、秘市诸修」，地图补「奇渊岛黑石城、秘市荒岛」|
| B19 | 第 12 章契约第 3 节「金丝蚕」行：「两只活下来」| ch522 两只双双毙命 | 回写第 12 章契约与笔记；n22 收尽 |
| B20 | 第 12 章契约第 3 节「境界」行：ch529 `realm.advance(realm.CORE_MID)` 一处 | 原著十五六年闭关坐出来 | **改契约**：`realm.cap(realm.CORE_MID)` ＋ 静室自冲，`realm.advance` 0 处（1.3）|
| B21 | 人物总表 `wen_tianren` source_ch「ch571」| find 首见 ch570 | 回写人物总表（第 14 章的人）|
| B22 | lore 4.6 #8「城内斗法者灭族」| ch512「形神被灭」，无灭族 | 回写 lore |
| B24、B25 | 占位 `b13_waixinghai_lieyao`（note「立府雾中岛…」、奖励 1400 / 320 / 五级 ×2）与 `wuji_haishou` `weaknesses ["土"]` | 本章 ② 是 ch523 珊瑚礁诱妖；五级被飞剑一击即斩 | 就地改写编成（note、奖励、掉落全换）；`wuji_haishou` 就地补「剑」，全仓只 ② 一张编成含它 |
| B26 | 校对与 ch528「玄阴岛」| 原文 ch489、528 两处；全作通用「极阴岛」| 回避：本章 0 处「玄阴岛」|
| B27 | 大纲地图「偷渡航线」| 原著是星空殿传送阵偷传，不是船 | 回写大纲：「天星城星空殿（偷传）」|
| B28 | 人物总表第 13 章表只 4 人 | 原著具名者另有苍云龙、于护法、易敬、许云、离寻、古玉、公孙杏、黄明礼、文思月等 | 回写人物总表 |
| B29 | 人物总表改编需求「立府向导 / 守卫头目」「商行掌柜」| 原著有明夫人、离寻、巨猿傀儡守府 | 用原著人物顶上，不另造改编人物 |
| B30 | ch514 炉鼎拍卖、ch532「稚女炉鼎」| 原著如此 | 回避 / 改编：拍卖不演；公孙杏之约改她自己提的「入阵为仆」，标改编 |
| B31 | 第 12 章契约「炼晶」行：要必成先改「只许一张」| — | 本章不做配方：重炼走脚本 n20，`guaranteed` 仍全仓一张 |
| B32 | 摘要「黑色条纹」「六眼」等措辞 | ch522「黑斑」「银斑」；巨虾「六只碧绿的眼」| 文案照原文意自写，不用摘要造词 |

### 1.3 境界曲线与上限

- **原著**：ch503 他自认结丹初期 → ch524–529 七年猎妖、三年炼丹（仍初期）→ ch529 闭关十五六年、修成剑诀第八层、破结丹中期瓶颈 → ch534 中期后又闭关二十多年、续修第九层 → ch542 仍结丹中期。
- **游戏**：起点 `realm 31 / realmCap 31 / formerRealm 22`（估，第 2 节）；止 **32 / 32 / 22**。
  - n27（`biguan.lua`）`local ok = realm.cap(realm.CORE_MID)` **全章唯一一处**，看返回值（false 只换一句照走）。
  - 31 → 32 **由玩家在雾中岛静室 `facility_jingshi_wz`（meditate、`effectiveness 150`）按面板冲关**：所需 5000 点，到线成功率 85%（同档基准 60 ＋ 资质 25 ＋ 丹药 0；夹 [5,95]），失败倒扣两成。
  - n28（`chuguan.lua`）开头门闸 `if not realm.at_least(realm.CORE_MID) then … return end`：不到只一句「瓶颈未破」，不置旗、不拨日。静室随时可进，失败只折修为——**不设死局**。
  - `realm.advance` / `realm.demote` 本章各 **0** 处（推翻第 12 章契约第 3 节「境界」行，B20）；`formerRealm` 不读不写（31 ＞ 22，无重修八倍）。
- **为什么由玩家坐**：雾中岛是经营章，最该给玩家的那一下就是亲手按出中期；剧情直接升会把它拿走。
- **坐多久（意图玩家）**：基准每日约 0.9 点、计顿悟期望约 1.02 点；所需 `5000 − C0`（C0 ＝ 进静室时的修为）：C0 ＝ 0 → 约 4900 日，C0 ＝ 1500 → 约 3430 日，C0 ＝ 3000 → 约 1960 日。面板「闭关一年」每次 360 日；**意图玩家按几次以 8.1「C0 修正」为准**（真档前上界约 6 次、M ≲ 2150 日；上面三档 C0 只是刻度，不作次数依据，16.1 第 48 条）。C0 必须拿真档核（O-3）。文案只写「十余年」。
- **数值**（读表，不抄数）：结丹初期 700 / 470、攻 62 防 46；结丹中期 900 / 620、攻 76 防 58（「暂定」）。**本章没有一场仗发生在玩家结丹中期之后**（② 的循环在 n27 收口），结丹中期主角锚点留第 14 章标定。
- **对手**：① 两名追兵结丹初期；② 第 0 波五级海兽（≈ 结丹初期）、首领六级青色巨虾（≈ 结丹中期）。**只露面不打的**：苍云龙、凌玉灵、赵护法、顾乾二人、离寻（结丹初期）；于护法、云天啸、鸠面老者（结丹中期）；易敬、许云、黄明礼（筑基后期 / 假丹）。
- **小瓶**：结丹 3 日一滴、容量 9（第 11 章已抬）；诱妖每轮 `bottle.spend(1)`，三日一轮恰好一滴。

### 1.4 第 12 章留下的线头

| 线头 | 第 12 章留下 | 本章怎么交代 |
| --- | --- | --- |
| 开局 | `ch12_lingquan` `quanbian`、`ch12.done`；不 teleport | **patch `scripts/ch12/lingquan.lua` 末尾一行 `teleport("ch13_nanming")`**（落 default `haishang`）；n01 由三仙宗老者开口 |
| 境界 | 31 / 31，`formerRealm 22`；接口写 `realm.advance(CORE_MID)` 一处 | **改**为 `realm.cap` 一处 ＋ 面板自冲（1.3）；协调者先改第 12 章契约第 3 节「境界」行 |
| 队伍 | 空 | 照接；`party.*` 0 处 |
| 法术 | 11 门 ＋ `magic_shijin_chongqun` | ＋ `magic_ji_wuxinghuan`（n01）＋ `magic_fengyun_heji`（n20）＝ 14 门 |
| 炼晶 | ×1，`tradeable false` | n20 `take`，不做配方 |
| 玄骨肋骨 | story | n19 `take`、换 `story_xuanyin_jing`；「玄阴经」自 n19 起解禁 |
| 血色披风、五色铜环 | story ×2 | 披风只旁白；铜环做法术，物品不动 |
| 啼魂兽、鸣魂珠、乾蓝冰珠、虚天鼎、玉如意、九曲灵参与丹方 | story | 只在 n18、n19 旁白读；全不动（九曲灵参 n30 随身带走，物品不变）|
| 其余 story（灵乳、养魂木根须、皇鳞甲等）| 各 1 | 灵乳只文案；根须进 Z3（可选、`take` 看返回值）；其余不读 |
| 金丝蚕 | `ch12.jincan` 1 时 ×3 | **改**：原著全死——n22 收尽（B19）|
| 规则 25 | `MEANS_LAST_CHAPTER = 12`；探针在 `wuji_haishou` ＋ `b13_waixinghai_lieyao` | 改 13；探针挪 `feng_xi` ＋ `b14_fengxi_xiezhi` |
| 时间 | 单次 ≤ 3650 | 本章单次最大 3600 |
| Lua 命令 | 不加 | 照接：本章 0 个新命令（契约 E1、E2 都不改 `src/`）|
| 洞府 / 灵田 | `field_tianxing_39` 搁置 | `ch13_tianxing` 挂同一块田让玩家自采（契约 E2）|
| 妖丹、妙音门腰牌 | 五级 0、六级 0–1；腰牌不读 | 腰牌 n04 只读不扣 |

---

## 2. 起点资源事实（估）与入口边界

`tests/fixtures/ch12-end-{first,second}.sav` 尚未生成。下表全部按第 12 章施工图章末口径**估**；标「必须」的项落地后逐项拿真档核（O-3），**核前不得据此写死任何断言**。字段形状照 `tests/fixtures/ch10-end-first.sav`（`payload` 下 `bag[{itemId,count,herbAge}]`、`party[]`、`flags{}`、`learnedMagics[]`、`bottle{…}`）；本章**不加存档字段**（仍 v9）。

| 字段 | 估值（first / second）| 来路 | 本章用法 | 拿真档核？ |
| --- | --- | --- | --- | --- |
| `day` | 49560 / 49535 | 第 12 章起点估值 ＋ 第 12 章「恰 182」| 3.3 只写增量 | **必须** |
| `realm / realmCap / formerRealm` | 31 / 31 / 22 | 第 12 章验收 2 终点先验 | n27 抬上限 | **必须** |
| `hp / maxHp`、`mp / maxMp` | 不定 / 700、不定 / 470 | 第 12 章末战之后未必满 | ① 前有 `facility_dazuo_nanming` | **必须** |
| `cultivation`（C0 的底数）| ≥ 2872 / 2811（第 11 章真档 2572 / 2511 ＋ ① 300，第 12 章四场另加；8.1）| 第 11、12 章战斗修为 | 1.3 坐关日数 | **必须**（决定按几次「闭关一年」）|
| `material_lingshi` / `_zhong` | 低阶约 2331 / 2311（第 9 节导语）；中阶 20 / 20 | 第 12 章「恰起点 − 8」/「恰起点」| 偷渡第一支要 10000，意图玩家付不起——第三支兜底；中阶不读 | 低阶**必须**、中阶可选 |
| `material_yaodan_wuji` / `_liuji` | 0 / 0、0–1 | 第 12 章契约第 3 节「妖丹」行 | 偷渡第二支意图玩家没有 | **必须** |
| `party` | 空 | 第 12 章 E1 | `party.*` 0 处 | 必须 |
| `learnedMagics` | 第 10 章 11 门 ＋ `magic_shijin_chongqun` | 第 12 章验收 2 | 规则 25 通表 | **必须** |
| `story_miaoyin_yaopai` | 1 / 1 | 第 11 章 n25 | n04 只读不扣 | **必须** |
| `story_xuangu_leigu`、`material_lianjing` | 各 1 | 第 12 章 | n19、n20 `take` 看返回值 | **必须** |
| `story_jinsi_can` | `ch12.jincan == 1` 时 3，否则 0 | 第 12 章抉择 | n18 多一段、n22 收尽 | **必须** |
| `story_yanghun_mugen`、`story_wannian_lingru`、`story_xuese_pifeng`、`story_wuse_tonghuan`、`story_jiuqu_lingshen`、`story_xutian_ding`、`story_qianlan_zhu`、`story_tihun_shou`、`story_hualan` | 各 1 | 第 12 章验收 2 | 只旁白读；根须进 Z3 | 必须（Z3 门闸）|
| `field_tianxing_39` 六槽 | 第 11 章真档 6 槽全空（契约第 0 节 E2）| 第 11 章 | 玩家经 `facility_yaoyuan_39` 自采 | 可选 |
| `bottle` | capacity 9、drops 若干 | 第 11 章 | 诱妖、Z3 | 可选 |
| `mapId / position` | patch 前 `ch12_lingquan` `quanbian`；patch 后 `ch13_nanming` `haishang` | 第 12 章章末 ＋ 本章 patch | 落点格不挂触发器；n01 是它北面横贯的触发带 | **必须**（重生成后核）|

**入口边界**：

- 本章起点只认「第 12 章真档 ＋ `lingquan.lua` 末尾 patch 后的一次 teleport」。正式 `ch13` 入口档须由测试路从真 `ch12-end` 跑出，**不手改 fixture 的 `mapId / position` 冒称入口已验**；第 12 章通关测试的终点断言随 patch 改（落 `ch13_nanming`）。
- 落地**不触发** enter 触发器（引擎只在走一步时判）：`haishang` 那一格不挂任何对象，n01 挂在它北面横贯礁路的触发带上，通关驱动要先迈一步。
- 意图玩家两侧走同一组选项（3.2 末），章末账两侧之差只来自起点之差，不另开分岔、不拉平。

---

## 3. 主线节点（五幕、34 个节点）

### 3.0 主干怎么定

**判据三条**（与第 8–12 章同一套，先后有序）：

1. **韩立亲身在场、而且后面的章要用到它的结果**——演成可玩场。
2. **韩立在场、但只是一段「事情就是这样发生的」**——进旁白 / 蒙太奇（三五句，交代因果，不演过程）。
3. **韩立不在场、或者只有作者知道**——**不演**。玩家只知道韩立知道的。

**表一　可玩场（22 条）**

| # | 原著事件 | 章 | 节点 | 为什么要演 |
| --- | --- | --- | --- | --- |
| 1 | 海上问话：天星令、逆星令、逆星盟 | 499–500 | n01 | 全章的「乱」从这一问开始；n01 学五行环是**改编**（原著 499–503 没有祭炼铜环的文字，ch504 铜环才再出场），为 ① 铺手段 |
| 2 | 原料铺问千叶露、玛瑙角 | 501 | n02 | 主线支线 `q13_qianyelu` 的起点；战时坊市 |
| 3 | 逆星盟攻港、南明岛成前哨 | 501–502 | n03 | 「封锁」的第一层 |
| 4 | 封岛盘查、腰牌过关 | 502–503 | n04 | 第 11 章腰牌的落点 |
| 5 | 两名追兵、古宝之威 | 503–504 | n05 | 战斗 ①；全作第一次一人对两名同阶 |
| 6 | 凌玉灵谢恩、邀入星宫、引他进城 | 505 | n06 | 他不入星宫的选择 |
| 7 | 戒严入城、指环、传送阵新条件 | 505–506 | n07 | 「封锁」的第二层 |
| 8 | 回洞府、装虫、弃府 | 507 | n08 | 近百年洞府的了结；旧灵田自采 |
| 9 | 探阵、偷听黑价 | 507 | n09 | 偷渡的由头 |
| 10 | 跟踪张姓、七人议事、迷魂得把柄 | 508、510 | n10 | 偷渡三支付法的第三支从这里来 |
| 11 | 易容曲姓、拦七人 | 509 | n11 | 他以假丹身份混入 |
| 12 | 传送大厅：付费、让名额、「奇渊」 | 509–511 | n12 | 偷渡三支；离开内星海 |
| 13 | 石屋落地、能出不能进、黑石城 | 511–512 | n13 | 外星海的规矩 |
| 14 | 明珠轩：收购、观拍 | 513–515 | n14 | 妖丹的去处（经营循环）|
| 15 | 寸金阁：千叶露真身 | 515–516 | n15 | 主线支线第一步 |
| 16 | 雾中岛立府、虫室、玄阴经、重炼 | 517–520 | n17–n20 | 经营章的骨架；蜂云合剑是 ② 的收尾手段 |
| 17 | 炼化五色圆珠、布诱妖阵 | 522–523 | n22、n23 | 诱妖循环的前提 |
| 18 | 珊瑚礁诱妖 | 523 | n24 | 战斗 ②（可重复）|
| 19 | 收手、虫魔之名 | 524–527 | n25 | 「七年」的分量；交换会筹码的来路 |
| 20 | 炼丹三年、闭关冲中期、出关 | 528–530 | n26–n28 | **玩家自己坐出中期** |
| 21 | 公孙杏求药、兽潮旧事 | 530–533 | n29 | 至元丹三支；黑石城已成废墟（ch531 第 5、8 段，只经人口；北区怎么呈现见 7.1）|
| 22 | 离岛、秘市交换会、文思月、铜片、深渊边缘取草 | 533–542 | n30–n34 | 主线收束；第 14 章的三条线头（铜片、伴妖草、人声）|

**表二　旁白 / 蒙太奇（10 条）**

| # | 原著事件 | 章 | 放在哪 | 为什么不演 |
| --- | --- | --- | --- | --- |
| a | 披血色披风赶路、半月海路 | 500 | n01 末段 | 赶路 |
| b | 辛明出示信物 | 502 | n04 | 他在场，但只是盘查的过程（画像、煞气移到表三 κ）|
| c | 空街、神识扫到指环即退 | 507 | n08 开头 | 质感（13.1）|
| d | 以丹药与符宝换古玉玉简；北飞十余日、转西两日；追银翼鸟入雾 | 516–517 | n15；n16 末、n17 开头 | 一笔不扣物的交易；赶路 |
| e | 九曲灵参移入药园禁地；半月炼布阵器具、新噬金虫卵入控神法阵 | 518、522 | n18；n22 | 他做，但没有后果要玩家选 |
| f | 巨龟渡劫、毒蛟与金霞霍氏相斗、血光遁走 | 520–522 | n21 | 八级只能逃（1.1 第 4 条）|
| g | 聚魂钵抽魂取丹；七年：深渊暴动、琉璃兽一剑、深海数年 | 523–527 | ② 胜后；n25 蒙太奇 | 一句交代妖丹来路；七年只剩几件事 |
| h | 三年炼丹、丹术近大师；二十余年、鹰鸢兽被光柱退走 | 528–534 | n26；n28 | 不做「炼丹大师」数值；他一步没出洞府 |
| i | 换形诀；救黄明礼一招 | 534 | n30 | 一招了事，做成仗是灌水 |
| j | 云天啸十二柄小剑一击；铜片光影三十六式；资质近三灵根 | 538–541 | n31、n33 | 一击了事；资质不改数值 |

**表三　不演（韩立不在场、无从得知；10 条）**

| # | 原著事件 | 章 | 玩家怎么知道 |
| --- | --- | --- | --- |
| α | 万三谷、六道极圣本人 | 500 | 只闻其名（n01），0 处上场 |
| β | 逆星盟「少主」是谁 | 502 起 | n04 壮汉当面插一句「少主是圣祖唯一传人」（ch502 第 37 段，判据①）＋ K2 可再问；不出名、不报境界（第 14 章，16.1 第 57 条）|
| γ | 毒蛟此战之后蛟族入局、妖修之战 | 522 | 不知道（ch522 第 14 段明写）——n21 末只写他改主意不迁府、加阵；「导火索」「大祸」0 处（16.1 第 42 条）|
| δ | 极阴照齐姓道士之法派弟子冒充他杀人夺宝 | 527 | 不知道——只写三人所说与他的猜测（碧云门或虚天殿老怪），极阴、齐姓 0 处 |
| ε | 万法门长老漩矶潜来奇渊岛 | 528 | 不知道——漩矶 0 处 |
| ζ | 兽潮本身（海渊万妖涌出、黑石城被破、两位元婴陨落）| 二十余年前 | 只经孙师叔之口（n29、K12）；不演、不建战斗 |
| η | 妙音门惊变的经过 | 虚天殿前后 | 只从文思月口中知道结果，他自己推断时点（n32）|
| θ | 铜片的真名与来历 | 546 | 不知道——「铜片」；「梵圣真片」0 处 |
| ι | 身后那人是谁、化形第几阶、住在哪 | 542–543 | 不知道——只写人声（n34）|
| κ | 两位长老画像早被复制、他的煞气只露十之一 | 503 | 不知道——苍、刘私话时他已飞走（ch503 第 2、9、22 段）；n04 只写他自己不易容的推断（16.1 第 41 条）|

**主线节点表（本章节点事实的唯一权威）**

脚本名 ＝ id 去掉 `nNN_`（如 `scripts/ch13/wenxun.lua`）。「前置」即挂点的 `guard_flag`；NPC 节点的前置由**脚本开头判**（未到只一句 `return`、不置旗），NPC 的 `visible_flag` 以 5.1 为准、可以早于前置（n14、n15 就是，16.1 第 47 条）；`done_flag` 即目标链该步的完成旗。挂点格位与 mode / once 见 3.1，动作见 3.2，日数见 3.3。

| 节点 | id | 幕 | 原著章 | 地图 | 挂点对象 | 前置 | done_flag | 这一节点落成什么 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| n01 | `n01_wenxun` | 一 南明岛 | 499–500 | `ch13_nanming` | `trigger_wenxun` | `ch12.done` | `ch13.wenxun` | 三仙宗老者开口；天下大势；五行环入法术表；半月海路 |
| n02 | `n02_caigou` | 一 | 501 | `ch13_nanming` | NPC `npc_yuanliao_zhanggui` | `ch13.wenxun` | `ch13.caigou` | 问不到千叶露；南明岛店开张 |
| n03 | `n03_gongdao` | 一 | 501–502 | `ch13_nanming` | `trigger_qiuling` | `ch13.caigou` | `ch13.gongdao` | 逆星盟攻港，南明岛成前哨（店自此封）|
| n04 | `n04_panzha` | 一 | 502–503 | `ch13_nanming` | NPC `npc_cang_yunlong` | `ch13.gongdao` | `ch13.panzha` | 腰牌过关 |
| n05 | `n05_zhuibing` | 一 | 503–504 | `ch13_nanming` | `trigger_chugang` | `ch13.panzha` | `ch13.zhuibing` | **battle ①**；两件战利品与灵石 |
| n06 | `n06_yinjian` | 一 | 505 | `ch13_nanming` | NPC `npc_ling_yuling` | `ch13.zhuibing` | `ch13.yinjian` | 不入星宫；随她进城（→ `ch13_tianxing`）|
| n07 | `n07_rucheng` | 二 天星城 | 505–506 | `ch13_tianxing` | `trigger_rucheng` | `ch13.yinjian` | `ch13.rucheng` | 戒严入城；黄色指环；传送阵新条件 |
| n08 | `n08_qifu` | 二 | 507 | `ch13_tianxing` | `trigger_dongfu39` | `ch13.rucheng` | `ch13.qifu` | 装虫、弃府（药园玩家自采）|
| n09 | `n09_tanzhen` | 二 | 507 | `ch13_tianxing` | `trigger_xingkong` | `ch13.qifu` | `ch13.tanzhen` | 探阵；听到黑价与「结丹不放」|
| n10 | `n10_qianru` | 二 | 508、510 | `ch13_tianxing` | `trigger_zhaiyuan` | `ch13.tanzhen` | `ch13.qianru` | 七人议事；张姓的把柄 |
| n11 | `n11_yirong` | 二 | 509 | `ch13_tianxing` | `trigger_lanlu` | `ch13.qianru` | `ch13.yirong` | 易容曲姓假丹；挤进七人 |
| n12 | `n12_chuansong` | 二 | 509–511 | `ch13_tianxing` | NPC `npc_gu_laozhe` | `ch13.yirong` | `ch13.chuansong` | 偷渡三支付法；传往奇渊（→ `ch13_waihai`）|
| n13 | `n13_daoqiyuan` | 三 奇渊岛 | 511–512 | `ch13_waihai` | `trigger_shiwu` | `ch13.chuansong` | `ch13.daoqiyuan` | 能出不能进；黑石城的规矩 |
| n14 | `n14_mingzhu` | 三 | 513–515 | `ch13_waihai` | NPC `npc_ming_furen` | `ch13.daoqiyuan` | `ch13.mingzhu` | 收购行开张；观拍 |
| n15 | `n15_cunjin` | 三 | 515–516 | `ch13_waihai` | NPC `npc_li_xun` | `ch13.mingzhu` | `ch13.cunjin` | 千叶露真身；上古典籍玉简 |
| n16 | `n16_chucheng` | 三 | 517 | `ch13_waihai` | `trigger_chucheng` | `ch13.cunjin` | `ch13.chucheng` | 海图；出城寻岛（→ `ch13_wuzhong`）|
| n17 | `n17_lifu` | 四 雾中岛 | 517 | `ch13_wuzhong` | `trigger_lifu` | `ch13.chucheng` | `ch13.lifu` | 立府；灵田、静室、丹炉、往返自此开 |
| n18 | `n18_chongshi` | 四 | 517–518 | `ch13_wuzhong` | `trigger_chongshi` | `ch13.lifu` | `ch13.chongshi` | 虫室；五色圆珠一匣 |
| n19 | `n19_xuanyin` | 四 | 518–519 | `ch13_wuzhong` | `trigger_shichuang` | `ch13.chongshi` | `ch13.xuanyin` | 肋骨 → 玄阴经；鼎不开 |
| n20 | `n20_chonglian` | 四 | 519–520 | `ch13_wuzhong` | `trigger_lianjianshi` | `ch13.xuanyin` | `ch13.chonglian` | 重炼七十二剑；蜂云合剑入法术表 |
| n21 | `n21_dujie` | 四 | 520–522 | `ch13_wuzhong` | `trigger_dizhen` | `ch13.chonglian` | `ch13.dujie` | 旁观化形雷劫与毒蛟之战 |
| n22 | `n22_wusezhu` | 四 | 522 | `ch13_wuzhong` | `trigger_biguanshi` | `ch13.dujie` | `ch13.wusezhu` | 炼尽五色圆珠；金丝蚕收尽 |
| n23 | `n23_buzhen` | 四 | 522–523 | `ch13_wuzhong` | `trigger_buzhen` | `ch13.wusezhu` | `ch13.buzhen` | 珊瑚礁四阵；诱妖自此可用 |
| n24 | `n24_youyao` | 四 | 523 | `ch13_wuzhong` | `trigger_youyao` | `ch13.buzhen` | `ch13.lieyao`（计数，≥ 1 记一步）| **battle ②**（循环）|
| n25 | `n25_chongmo` | 四 | 524–527 | `ch13_wuzhong` | `trigger_shoushou` | `ch13.buzhen`（脚本另判 `ch13.lieyao >= 2`）| `ch13.chongmo` | 「七年」；虫魔之名；妖丹堆与六七级材料 |
| n26 | `n26_liandan` | 四 | 528–529 | `ch13_wuzhong` | `trigger_danfang` | `ch13.chongmo` | `ch13.liandan` | 炼丹三年；至元丹方解锁 |
| n27 | `n27_biguan` | 四 | 529 | `ch13_wuzhong` | `trigger_jingshimen` | `ch13.liandan` | `ch13.biguan` | **抬上限到 32**；往返自此闭 |
| n28 | `n28_chuguan` | 四 | 529–530 | `ch13_wuzhong` | `trigger_chuguan` | `ch13.biguan` | `ch13.chuguan` | 境界门闸；二十余年；青灵门二人现身 |
| n29 | `n29_gongsun` | 四 | 530–533 | `ch13_wuzhong` | NPC `npc_gongsun_xing` | `ch13.chuguan` | `ch13.gongsun` | 兽潮旧事（`ch13.shouchao`）；至元丹三支；入阵为仆（改编）|
| n30 | `n30_lidao` | 四 | 533–534 | `ch13_wuzhong` | `trigger_lidao` | `ch13.gongsun` | `ch13.lidao` | 弃府离岛（→ `ch13_waihai` 秘市岸）|
| n31 | `n31_mishi` | 五 秘市 | 535–539 | `ch13_waihai` | `trigger_mishi` | `ch13.lidao` | `ch13.jiaohuan` | 交换会：铜片（必得）、换文思月自由 |
| n32 | `n32_wenshi` | 五 | 539–540 | `ch13_waihai` | NPC `npc_wen_siyue` | `ch13.jiaohuan` | `ch13.wenshi` | 妙音门惊变；巢穴位置 |
| n33 | `n33_jianshi` | 五 | 541 | `ch13_waihai` | `trigger_jiaoshi` | `ch13.wenshi` | `ch13.jianshi` | 铜片是妖修功法；深渊边缘蹲守 |
| n34 | `n34_banyao` | 五 | 541–542 | `ch13_waihai` | `trigger_dongkou` | `ch13.jianshi` | `ch13.done` | 黑色伴妖草；身后人声；章末 |

**主动抉择 15 处**：n03、n04、n05、n06、n07、n11、n12（付费三支）、n14、n24（每轮战中可逃）、n29 两处（回话 / 至元丹从哪出）、n31 三处（铜片怎么付 / 换她自由 / 范夫人之邀）、Z1（n27、n30 开头的确认不计，16.1 第 52 条）。**真改状态的 4 类**：`ch13.toudu`、`ch13.zhiyuan`、`ch13.tongpian`、② 逃与不逃；其余只换一句后合流。**必打 2 场**（② 至少胜 2 次才开 n25）。
**主干的一句话**：海上问话 → 南明岛问草 → 攻港 → 盘查 → 追兵 → 入城 → 弃府 → 探阵 → 把柄 → 易容 → 偷传 → 能出不能进 → 收购行 → 千叶露 → 立府 → 玄阴经 → 重炼 → 雷劫 → 诱妖 → 七年 → 三年 → 坐关 → 兽潮旧事 → 秘市 → 文思月 → 深渊边缘 → 黑草与人声。前半章他一路被「封」——岛封、城封、阵封、海封；后半章他第一次有了大把的时间，用来炼剑、养虫、坐关，直到为一株草再次出门。

### 3.1 每一处挂点怎么开始（`mode` / `once`）

分法沿用前十二章：**这件事是他做的（interact），还是发生在他身上的（enter）。** 不用 `auto`（map_spec 的 auto 未实现）。
**挂 NPC 的 8 个节点**（n02、n04、n06、n12、n14、n15、n29、n32）没有 once，脚本开头看 done_flag 判已演过；`once=true` 的触发器 `set_flag` ＝ 主线节点表的 done_flag，**由脚本末尾自己 `flag.set`**（引擎从不替触发器写旗，只拿 `set_flag` 判 once 兑现了没有）。
**每张图的首个 enter 挂点**是 default spawn 前方横贯通道的触发带，不压 spawn（teleport 落地不触发 enter）。**同一格只放一个对象**（同格只取第一个 trigger）。portal 的门闸用 `require_flag ＋ deny_text_key`（`<目标图章号>.block.<目标图短名>`，规则 20；本章 0 个 portal）；**facility 只写 `require_flag`**，拦下时引擎只播 `ui.facility.locked`（4.3 第 1 条、7.1，16.1 第 45 条）。

| # | 挂点 | 地图 | `mode` | `once` | `set_flag` | 战斗 | 格位与为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| NPC | n02 `npc_yuanliao_zhanggui`（小城原料铺）、n04 `npc_cang_yunlong`（码头）、n06 `npc_ling_yuling`（出海口外礁头）、n12 `npc_gu_laozhe`（传送大厅）、n14 `npc_ming_furen`（明珠轩）、n15 `npc_li_xun`（寸金阁）、n29 `npc_gongsun_xing`（东岸林边光幕前）、n32 `npc_wen_siyue`（秘市荒岛东侧绿山小洞府前）| 各图 | NPC | — | — | — | 找人说话的是他；在场由 visible / hidden 管（5.1）；前置由脚本开头判（3.0 表头、3.2）|
| n01 | `trigger_wenxun` | nanming | enter | ✓ | ＝ done | — | 被人拦下问话是落在他身上的。`haishang` 北面一行横贯礁路 |
| n03 | `trigger_qiuling` | nanming | enter | ✓ | ＝ done | — | 攻港是撞上的。港后丘陵坡口 |
| n05 | `trigger_chugang` | nanming | enter | ✓ | ＝ done | ① | 追兵是找上门的。港口东出海口一行横贯带；`facility_dazuo_nanming` 在它之前的路上 |
| n07 | `trigger_rucheng` | tianxing | enter | ✓ | ＝ done | — | 城门盘查是落在他身上的。`chengwai` 北面城门前横贯带 |
| n08 | `trigger_dongfu39` | tianxing | interact | ✓ | ＝ done | — | 推开洞府门的是他。三十九层洞府门一格，与药园、静室设施不同格 |
| n09 | `trigger_xingkong` | tianxing | enter | ✓ | ＝ done | — | 外殿禁制是撞上的。五十层外廊横贯带（在 `trigger_lanlu` 之上）|
| n10 | `trigger_zhaiyuan` | tianxing | interact | ✓ | ＝ done | — | 放神识潜入的是他。第四层宅院院外一格 |
| n11 | `trigger_lanlu` | tianxing | enter | ✓ | ＝ done | — | 在石阶上拦人——七人走到哪里他等在哪里。四十九层石阶横贯带 |
| n13 | `trigger_shiwu` | waihai | enter | ✓ | ＝ done | — | 一出石屋就是外星海。石屋门外横贯带（`shiwu` 在屋内）|
| n16 | `trigger_chucheng` | waihai | enter | ✓ | ＝ done | — | 出城即启程。西门横贯带 |
| n17 | `trigger_lifu` | wuzhong | enter | ✓ | ＝ done | — | 登岛即开府。`andeng` 北面横贯带 |
| n18、n19、n20、n22 | `trigger_chongshi`、`trigger_shichuang`、`trigger_lianjianshi`、`trigger_biguanshi` | wuzhong | interact | ✓ | ＝ done | — | 进哪间屋做哪件事的是他。山脉凹谷洞府六扇门各一格（另两扇见 n26、n27），互不同格 |
| n21 | `trigger_dizhen` | wuzhong | enter | ✓ | ＝ done | — | 地动是落在他身上的。洞府甬道内侧一行 |
| n23 | `trigger_buzhen` | wuzhong | enter | ✓ | ＝ done | — | 登礁即布阵。西海珊瑚礁登礁横贯带 |
| n24 | `trigger_youyao` | wuzhong | interact | ✗ | 空（脚本累加 `ch13.lieyao`）| ② | 放草的是他。阵心礁一格 |
| n25 | `trigger_shoushou` | wuzhong | interact | ✓ | ＝ done | — | 收阵的是他。礁外收阵石，与阵心隔 ≥ 2 格；`lieyao < 2` 在置旗前 return |
| n26 | `trigger_danfang` | wuzhong | interact | ✓ | ＝ done | — | 六扇门之一（与 `facility_danlu_wz` 不同格）|
| n27 | `trigger_jingshimen` | wuzhong | interact | ✓ | ＝ done | — | 六扇门之一（与 `facility_jingshi_wz` 不同格）|
| n28 | `trigger_chuguan` | wuzhong | enter | ✓ | ＝ done | — | 出关一步就撞上门外的事。洞府甬道外侧一行（与 n21 分在两行）；境界不到在置旗前 return |
| n30 | `trigger_lidao` | wuzhong | enter | ✓ | ＝ done | — | 走上码头即离岛。码头入口一行横贯带 |
| n31 | `trigger_mishi` | waihai | interact | ✓ | ＝ done | — | 敲壁的是他。西南秘市荒岛乱石堆石壁 |
| n33 | `trigger_jiaoshi` | waihai | enter | ✓ | ＝ done | — | 登礁即蹲守。东南小岛外礁一行横贯带 |
| n34 | `trigger_dongkou` | waihai | interact | ✓ | ＝ done | — | 入洞的是他。半山巨石后洞口 |
| 链外 | `trigger_fangshi_bimen` | tianxing | interact | ✗ | 空 | — | 圣山脚坊市门；无 guard，只一句 |
| 链外 | `trigger_xuanshang` | waihai | interact | ✗ | 空 | — | 告示墙（Z2）；无 guard，脚本判旗 |
| 链外 | `trigger_yanghun` | wuzhong | interact | ✗ | 空 | — | 药园旁一格（Z3）；无 guard，脚本判旗 |
| 链外 | `trigger_feixu` | waihai | enter | ✗ | 空 | — | 中部礁路进北区的入口一行横贯带；guard `ch13.shouchao`；只一句废墟旁白、不拦路（16.1 第 49 条）|
| 往返 | `trigger_qiyuan_hang` | wuzhong | enter | ✗ | 空 | — | 码头尽头一格，与 n30 的带隔 ≥ 2 格；guard `ch13.lifu` |
| 往返 | `trigger_hui_wuzhong` | waihai | enter | ✗ | 空 | — | 西门外礁头，与 n16 的带错开 ≥ 3 格（#6 落点在两者之间、不压带）；guard `ch13.lifu` |
| 设施 | `facility_cunji_{nanming,tianxing,qiyuan,mishi}`、`facility_cunji_wz` | 四图 | 设施 | — | — | — | 各紧挨落点、在首个触发带的落点一侧 |

- **`once=true` 的挂点每一条都写 `set_flag`**；条件不够的路（n25 胜场不足、n28 境界未到、各抉择取消）都在置旗前 `return`，once 不被兑现，再踏 / 再按照常响。`once=false` 的七处（n24、四处链外、两处往返）不写 `set_flag`。两场必打都挂在目标链的必经挂点上（n05；n24 → n25 的胜场门槛），绕不过去。
- **规则 13**：每张图只从 default spawn 出发 BFS。`ch13_waihai` 三区（北黑石城、西南秘市荒岛、东南小岛）靠礁路连通，未到的挂点靠 guard 惰性，不靠隔断；`ch13_wuzhong` 的西海礁与东岸同图连通。脚本落点不算起点。
- **规则 30**：同图同时在场的 role 两两不同、名牌两两不同；`yi_jing` 两个对象分属 `ch13_tianxing`（`npc_yi_jing_tx`）与 `ch13_waihai`（`npc_yi_jing`）。
- **兽潮之后的北区**：六个 NPC 全撤（`hidden_flag ch13.shouchao`），两家店挂在 NPC 脚本上随之没了；不设阻隔带——进北区的礁路口有一条 `trigger_feixu`（guard `ch13.shouchao`），每次走进只一句：黑石城已成废墟（ch531）。图块不随旗标换，地图本身不改，由这句文案交代（16.1 第 49 条）。

**脚本传送清单**（登 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`；全部只写图 id 或图 id ＋ 坐标，不认 spawn 名；本表是 teleport 的唯一出处）：

*三元（挂 NPC 或首次置旗的传送；`flag.set` 与 `teleport` 写在顶格、早退用顶格 `return`）*

| # | 行 | 落点 | 落地后 |
| --- | --- | --- | --- |
| 1 | `{"ch12.done", "scripts/ch12/lingquan.lua", "ch13_nanming"}`（patch 一行）| `haishang`（default）| 迈一步进 n01 的带 |
| 2 | `{"ch13.yinjian", "scripts/ch13/yinjian.lua", "ch13_tianxing"}`（挂 NPC）| `chengwai`（default）| 迈一步进 n07 的带 |
| 3 | `{"ch13.chuansong", "scripts/ch13/chuansong.lua", "ch13_waihai"}`（挂 NPC）| `shiwu`（default）| 出石屋进 n13 的带 |
| 4 | `{"ch13.chucheng", "scripts/ch13/chucheng.lua", "ch13_wuzhong"}` | `andeng`（default）| 迈一步进 n17 的带 |
| 5 | `{"ch13.lidao", "scripts/ch13/lidao.lua", "ch13_waihai"}`（带坐标）| 秘市岸 (14, 46)（建议，不与任何对象同格）| n31 是 interact，不靠落地触发 |

*五元（往返；触发器 `type trigger`、`mode enter`、`once false`、`guard_flag ch13.lifu`（＝ doneFlag）、`set_flag` 空；脚本 0 处 `flag.set`）*

| # | 行 | 落点 | 脚本开头 |
| --- | --- | --- | --- |
| 6 | `{"ch13.lifu", "scripts/ch13/qiyuan_hang.lua", "ch13_waihai", "trigger_qiyuan_hang", "ch13_wuzhong"}` | 黑石城西门外礁，**写死坐标**（地图路落图时定，同一字面量写进 `qiyuan_hang.lua`、本表与测试；须在 `trigger_hui_wuzhong` 与西门带之间、不与任何对象同格、不压 `trigger_chucheng` 的带）| `ch13.biguan` 已置 → 一句 return |
| 7 | `{"ch13.lifu", "scripts/ch13/hui_wuzhong.lua", "ch13_wuzhong", "trigger_hui_wuzhong", "ch13_waihai"}` | `andeng`（default）| `ch13.lidao` 已置 → 一句 return |

- 五元两条在门闸之后：`choice`「去 / 不去」（取消为 nil 即不去）→ `advance_days`（日数见 3.3）→ teleport。一份脚本拿不到是哪个触发器起的事件，所以两个方向拆两份脚本。**写法**：取消与「不去」都在**顶格** `return`（如 `if go ~= 1 then return end`），`advance_days(12)` 与 `teleport(…)` 也写在顶格——`kScriptTransfers` 对五元行同样要求顶格 teleport（`tests/ObjectiveTests.cpp` `hasTopLevel`），写进 `if` 块里就报「没有顶格的 teleport」（16.1 第 60 条）。
- **落地不触发 enter**：#1–#4 的落点前方都有横贯触发带，通关驱动要先迈一步；#5、#6 落在坐标格，`ObjectiveRoute` 只认目的图、不认坐标（O-11，测试路核）。
- **门钥匙**（`kOffChainDoorKeys`）：本章四张图之间没有一扇门，0 条。

### 3.2 脚本动作与分支

本节只规定状态操作、控制流和门闸，不提供叙事镜头、示例台词或转述顺序；剧情事实与质感禁用项只按 13.1 核验，对白另行原创。挂点属性以 3.1、日数以 3.3 为准（下表 `advance_days` 一律不写数）。**每一处 `take`、`magic.learn`、`realm.cap`、`field.unlock` 都看返回值**；返回 false 的路只换一句，不卡主线。

| 节点 / 脚本 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- |
| n01 wenxun | `magic.learn("magic_ji_wuxinghuan")`；advance_days；置 `ch13.wenxun` | learn false 换一句照走 |
| n02 caigou（NPC）| 首谈：置 `ch13.caigou`；`shop("ch13_nanming_fangshi")` | 再谈：`ch13.gongdao` 未置 → 再开店；已置 → 一句「封市」return |
| n03 gongdao | choice「看下去 / 退回坊市」；置 `ch13.gongdao` | 取消在置旗前 return；两支只换一句 |
| n04 panzha（NPC）| `item.count("story_miaoyin_yaopai") >= 1` 择「出示腰牌」版，否则「报名号」版（不扣；不提画像，16.1 第 41 条）；choice「应下 / 不置可否」；置 `ch13.panzha` | 已置 → 一句 return；取消在置旗前 return |
| n05 zhuibing | choice「随行 / 不随」→ `battle("b13_nanming_zhuibing")`；胜：give `story_mihuan_yan` 1、`story_baiyu_dayin` 1、`material_lingshi` 600；advance_days；置 `ch13.zhuibing` | 两支都打；取消在战前 return；败 game_over |
| n06 yinjian（NPC）| choice「以妙音门之恩婉拒 / 不答」；advance_days；顶格 `flag.set("ch13.yinjian")`、`teleport("ch13_tianxing")` | 两支都不入星宫；取消在置旗前 return |
| n07 rucheng | choice「报名 / 安分」→ 两支都 give `story_huangse_zhihuan` 1；置 `ch13.rucheng` | 取消在给物前 return；① 不抵传送阵条件 |
| n08 qifu | 置 `ch13.qifu` | 药园不由脚本搬作物（玩家经 `facility_yaoyuan_39` 自采）|
| n09 tanzhen | advance_days；置 `ch13.tanzhen` | 无 |
| n10 qianru | advance_days；置 `ch13.qianru` | 无 |
| n11 yirong | choice「直说同去 / 先亮把柄」；advance_days；置 `ch13.yirong` | 取消在置旗前 return |
| n12 chuansong（NPC）| choice 付费，**只列付得起的支**：① `material_lingshi >= 10000` → take 10000；② `material_yaodan_wuji >= 4` → take 4；③ 把柄 ＋ 胡月飞刀两柄（旁白，不扣物，永远在）；`flag.set("ch13.toudu", 1/2/3)`；顶格 `flag.set("ch13.chuansong")`、`teleport("ch13_waihai")` | 取消在扣物前 return；take false 落第三支；意图玩家走 ③ |
| n13 daoqiyuan | 读 `ch13.toudu` 择一句；置 `ch13.daoqiyuan` | 无 |
| n14 mingzhu（NPC）| 开头：`ch13.daoqiyuan` 未置 → 一句 return（不置旗）；首谈：choice「看下去 / 离开」；置 `ch13.mingzhu`；`shop("ch13_mingzhu_shougou")` | **首谈分支在源码里写在再谈分支之前**（`kOffChainFlags` 要 `flag.set("ch13.mingzhu")` 先于 `flag.set("ch13.z1")`，第 11 节末表）；再谈：开收购；Z1 分支：已接单、`ch13.z1` 未置、六级 ≥ 1 → take 六级 1、give `material_lingshi_zhong` 30、置 `ch13.z1` |
| n15 cunjin（NPC）| 开头：`ch13.mingzhu` 未置 → 一句 return（不置旗、不给物，16.1 第 47 条）；首谈：give `story_guyu_yujian` 1；置 `ch13.cunjin` | 再谈：`shop("ch13_cunjin_ge")` |
| n16 chucheng | give `story_qiyuan_haitu` 1；advance_days；顶格 `flag.set("ch13.chucheng")`、`teleport("ch13_wuzhong")` | 无 |
| n17 lifu | advance_days；`field.unlock("field_wuzhong", 6)`；give `herb_nichang_cao` 3；置 `ch13.lifu` | unlock false 换一句 |
| n18 chongshi | give `story_wuse_yuanzhu` 1；advance_days；置 `ch13.chongshi` | `item.count("story_jinsi_can") > 0` 多一段（不扣）|
| n19 xuanyin | take `story_xuangu_leigu` 1；give `story_xuanyin_jing` 1；advance_days；置 `ch13.xuanyin` | take false 换一句照给（不设死局）|
| n20 chonglian | take `material_lianjing` 1；advance_days；`magic.learn("magic_fengyun_heji")`；置 `ch13.chonglian` | take / learn false 各换一句照走 |
| n21 dujie | advance_days；置 `ch13.dujie` | 无 |
| n22 wusezhu | take `story_wuse_yuanzhu` 1；`local n = item.count("story_jinsi_can"); if n > 0 then take("story_jinsi_can", n) end`；advance_days；置 `ch13.wusezhu` | 无 |
| n23 buzhen | advance_days；置 `ch13.buzhen` | 无 |
| n24 youyao | `ch13.biguan` 已置 → 一句 return；草 `herb_nichang_cao < 1`：`material_lingshi >= 10` → 一句（指寸金阁）return，`< 10` → 一句、advance_days、`give("herb_nichang_cao", 1)`（兜底，药园自催一株）return；`bottle.drops < 1` → advance_days（缺液）return；否则 `bottle.spend(1)`、advance_days、`battle("b13_waixinghai_lieyao")` | 胜：take 草 1、`flag.set("ch13.lieyao", flag.get("ch13.lieyao") + 1)`；逃：不扣草；败 game_over；妖丹由编成 `drops` 给 |
| n25 chongmo | advance_days；give `material_yaodan_liuji` 6、`material_yaodan_wuji` 10、`story_liuqi_cailiao` 1；置 `ch13.chongmo` | `ch13.lieyao < 2` → 一句 return（不置旗）|
| n26 liandan | advance_days；置 `ch13.liandan`（解锁 `recipe_zhiyuan_dan`）| 炼不炼由玩家，不设门闸 |
| n27 biguan | choice「就此闭关 / 再等等」（确认，不计入主动抉择）；advance_days；`local ok = realm.cap(realm.CORE_MID)`；置 `ch13.biguan` | 取消在拨日、置旗前 return；cap false 换一句照走；提示静室在旁；确认文案写明此后往返、诱妖、Z1 / Z2 都收口，不点名任何按键（16.1 第 52 条）|
| n28 chuguan | advance_days 两段；置 `ch13.chuguan` | 开头 `not realm.at_least(realm.CORE_MID)` → 一句 return（不置旗、不拨日）|
| n29 gongsun（NPC）| choice 回话；choice 至元丹从哪出（**只列有的**）：① `pill_zhiyuan_dan >= 3` → take 3 记 1；② 六级 ≥ 1 → take 1 记 2；③ advance_days（开炉一月）记 3（永远在）；advance_days；末尾依次置 `ch13.shouchao`、`flag.set("ch13.zhiyuan", n)`、`ch13.gongsun` | 取消在扣物、置旗前 return；take false 落第三支；`shouchao` 必在 `gongsun` 之前 |
| n30 lidao | choice「这就走 / 再留几日」（确认，不计入主动抉择）；advance_days；顶格 `flag.set("ch13.lidao")`、`teleport("ch13_waihai", 14, 46)` | 取消在顶格 `return`（置旗前；要再触发得先走下这条带）；确认文案写明此后回不了雾中岛（Z3 收口），不点名任何按键（16.1 第 52 条）|
| n31 mishi | `shop("ch13_mishi_tan")` 一次；advance_days；铜片三支（只列付得起的）：五级 ≥ 1 → take 1 记 1 / 灵石 ≥ 5000 → take 5000 记 2 / 以桌上材料抵（旁白不扣）记 3；give `story_tongpian` 1；choice「以整堆材料换她自由 / 再看看」；take `story_liuqi_cailiao` 1；choice 范夫人之邀；`flag.set("ch13.tongpian", n)`；置 `ch13.jiaohuan` | 取消在扣物前 return；「再看看」回选单再给一次，第二次只剩「换」；take false 换一句照走；范夫人两支都是回绝 |
| n32 wenshi（NPC）| advance_days；置 `ch13.wenshi` | 已置 → NPC 已撤（`hidden ch13.wenshi`）|
| n33 jianshi | 读 `ch13.tongpian` 择一句；advance_days；置 `ch13.jianshi` | 无 |
| n34 banyao | give `story_banyao_cao` 1；末尾置 `ch13.done` | 不 `ending()`、不 teleport；人声一句之后无动作 |
| Z2 xuanshang | 已接（`ch13.daoqiyuan`）、`ch13.z2` 未置、五级 ≥ 3 → take 3、give `material_lingshi_zhong` 15、`herb_nichang_cao` 3、置 `ch13.z2` | `ch13.shouchao` 已置 → 一句 return；不足一句 |
| Z3 yanghun | 开头：`ch13.buzhen` 未置 → 一句（木未成形）return；`ch13.z3` 已置 → 一句 return；`item.count("story_yanghun_mugen") >= 1` 且 `bottle.drops >= 3` → `bottle.spend(3)`、advance_days、take `story_yanghun_mugen` 1、give `story_yanghun_zhu` 1、置 `ch13.z3` | 根须或绿液不足一句（先判根须、再扣液）；`ch13.lidao` 后不可达（不另判）；门闸以第 6 节为准 |
| 往返 qiyuan_hang / hui_wuzhong | choice 去 / 不去 → advance_days → teleport（3.1 #6、#7）| 门闸见 3.1；脚本 0 处 `flag.set` |

- 只用既有 Lua 接口（本章 0 个新命令）；`realm.advance` / `realm.demote` / `party.*` 0 处；不从脚本发修为。
- **意图玩家**（两侧同一组选项，写死在通关测试）：n02 不买；n03「看下去」；n04「应下」；n05「随行」；n06「婉拒」；n07「报名」；n11「直说」；n12 第三支；n14「看下去」；② 胜 2 次、不逃；0 次往返、0 条可选支线；n27「就此闭关」；静室按「闭关一年」直到冲成；n29 第二支（六级 1）；n30「这就走」；n31 铜片第一支、「以整堆材料换」、回绝范夫人。

### 3.3 日历

**本节是相对日历 spec，不是通关实测**；起点 `day` 从 `ch12-end-{first,second}.sav` 读，不从本表抄（O-3）。本表是 `advance_days` 的**唯一出处**；单次 ≤ 3650（引擎静默夹），本章单次最大 3600。**表里没列的节点（n02–n04、n07、n08、n12–n15、n34）一律 0。**

| 节点 | `advance_days` | 累计（从第 12 章终局起）|
| --- | --- | --- |
| n01 半月海路 | 15 | 15 |
| n05 出港半日、战后 | 1 | 16 |
| n06 随凌玉灵入城 | 2 | 18 |
| n09 次日 | 1 | 19 |
| n10 次日尾随、夜里 | 1 | 20 |
| n11 第三日清晨 | 1 | 21 |
| n16 北飞十余日、转西两日 | 14 | 35 |
| n17 六七日炼阵旗阵盘、开府 | 10 | 45 |
| n18 | 5 | 50 |
| n19 试鼎半月 | 15 | 65 |
| n20 近一年 | 360 | 425 |
| n21 地动、荒岛数日 | 5 | 430 |
| n22 半月 ＋ 六颗两月一颗 | 15 ＋ 360 | 805 |
| n23 西飞一月 | 30 | 835 |
| n24 每轮（意图玩家 2 轮）| 3 × 2 | 841 |
| n25 「七年后」| 2415 | 3256 |
| n26 半月回府 ＋ 三年 | 15 ＋ 1080 | 4351 |
| n27 歇一月 | 30 | 4381 |
| **窗口：静室打坐冲中期（玩家定；M ＝ 360 × N，N 以 8.1 为准，真档前上界约 6、M ≲ 2150）** | 面板 | 4381 ＋ M |
| n28 二十余年（两段）| 3600 ＋ 3600 | 11581 ＋ M |
| n29 两日后 | 3 | 11584 ＋ M |
| n30 | 20 | 11604 ＋ M |
| n31 三日后、交换会 | 5 | 11609 ＋ M |
| n32 | 1 | 11610 ＋ M |
| n33 一月到深渊边缘、蹲守数月 | 180 | **11790 ＋ M** |

**不计入意图玩家的**：n24 缺绿液 5、缺草且灵石不足 30；往返每趟 12；n29 第三支开炉一月 30；Z3 60。
**按幕小计**：一 18（7 min，① 约 3）／ 二 3（8 min）／ 三 14（5 min）／ 四 11569（15 min，② 两轮约 5、打坐面板约 1）／ 五 186（5 min）＝ 11790 ＋ M，**40 min**。全章约 33–39 年（M ≲ 2150 时；原著 ch511→ch542 约 48 年：一年炼剑、七年猎妖、三年炼丹、十五六年坐关、二十余年第九层）；**文案只写相对年数**，不写具体日数。
原著的锚：半月（ch500）、半日（ch503）、次日 / 第三日清晨（ch507–509）、十余日（ch517）、六七日（ch517）、半个月（ch519）、近一年（ch520）、两月一颗（ch522）、一月（ch523）、七年后（ch524，本稿复核）、三年（ch529）、十五六年（ch529，由玩家窗口承担）、二十多年（ch534）、数月（ch541）。

### 3.4 目标链（`data/objectives/ch13.json`）

| # | step id | `done_flag` | `target_map` | `target_object` | R-3 提示 |
| --- | --- | --- | --- | --- | --- |
| 1 | `n01_wenxun` | `ch13.wenxun` | `ch13_nanming` | `trigger_wenxun` | |
| 2 | `n02_caigou` | `ch13.caigou` | `ch13_nanming` | `npc_yuanliao_zhanggui` | **「九曲灵参还缺一味『千叶露』——港口小城有原料铺」** |
| 3 | `n03_gongdao` | `ch13.gongdao` | `ch13_nanming` | `trigger_qiuling` | |
| 4 | `n04_panzha` | `ch13.panzha` | `ch13_nanming` | `npc_cang_yunlong` | **「封岛了——离岛要先过盘查」** |
| 5 | `n05_zhuibing` | `ch13.zhuibing` | `ch13_nanming` | `trigger_chugang` | **「出海口前先把气血法力养足」** |
| 6 | `n06_yinjian` | `ch13.yinjian` | `ch13_nanming` | `npc_ling_yuling` | |
| 7 | `n07_rucheng` | `ch13.rucheng` | `ch13_tianxing` | `trigger_rucheng` | |
| 8 | `n08_qifu` | `ch13.qifu` | `ch13_tianxing` | `trigger_dongfu39` | **「回三十九层——药园里的药自己收」** |
| 9 | `n09_tanzhen` | `ch13.tanzhen` | `ch13_tianxing` | `trigger_xingkong` | |
| 10 | `n10_qianru` | `ch13.qianru` | `ch13_tianxing` | `trigger_zhaiyuan` | |
| 11 | `n11_yirong` | `ch13.yirong` | `ch13_tianxing` | `trigger_lanlu` | |
| 12 | `n12_chuansong` | `ch13.chuansong` | `ch13_tianxing` | `npc_gu_laozhe` | |
| 13 | `n13_daoqiyuan` | `ch13.daoqiyuan` | `ch13_waihai` | `trigger_shiwu` | |
| 14 | `n14_mingzhu` | `ch13.mingzhu` | `ch13_waihai` | `npc_ming_furen` | |
| 15 | `n15_cunjin` | `ch13.cunjin` | `ch13_waihai` | `npc_li_xun` | **「城里最大的原料铺——也许有人知道千叶露」** |
| 16 | `n16_chucheng` | `ch13.chucheng` | `ch13_waihai` | `trigger_chucheng` | |
| 17 | `n17_lifu` | `ch13.lifu` | `ch13_wuzhong` | `trigger_lifu` | |
| 18 | `n18_chongshi` | `ch13.chongshi` | `ch13_wuzhong` | `trigger_chongshi` | |
| 19 | `n19_xuanyin` | `ch13.xuanyin` | `ch13_wuzhong` | `trigger_shichuang` | |
| 20 | `n20_chonglian` | `ch13.chonglian` | `ch13_wuzhong` | `trigger_lianjianshi` | |
| 21 | `n21_dujie` | `ch13.dujie` | `ch13_wuzhong` | `trigger_dizhen` | |
| 22 | `n22_wusezhu` | `ch13.wusezhu` | `ch13_wuzhong` | `trigger_biguanshi` | |
| 23 | `n23_buzhen` | `ch13.buzhen` | `ch13_wuzhong` | `trigger_buzhen` | **「往西去，找一片珊瑚礁布阵」** |
| 24 | `n24_youyao` | `ch13.lieyao` | `ch13_wuzhong` | `trigger_youyao` | **「阵心放一株霓裳草，引妖兽上门」** |
| 25 | `n25_chongmo` | `ch13.chongmo` | `ch13_wuzhong` | `trigger_shoushou` | **「多引几回，攒够了再去收阵」** |
| 26 | `n26_liandan` | `ch13.liandan` | `ch13_wuzhong` | `trigger_danfang` | |
| 27 | `n27_biguan` | `ch13.biguan` | `ch13_wuzhong` | `trigger_jingshimen` | |
| 28 | `n28_chuguan` | `ch13.chuguan` | `ch13_wuzhong` | `trigger_chuguan` | **「静室里坐到结丹中期，再出关」** |
| 29 | `n29_gongsun` | `ch13.gongsun` | `ch13_wuzhong` | `npc_gongsun_xing` | |
| 30 | `n30_lidao` | `ch13.lidao` | `ch13_wuzhong` | `trigger_lidao` | **「去码头——找伴妖草要出海」** |
| 31 | `n31_mishi` | `ch13.jiaohuan` | `ch13_waihai` | `trigger_mishi` | **「乱石堆里的石壁后面就是秘市」** |
| 32 | `n32_wenshi` | `ch13.wenshi` | `ch13_waihai` | `npc_wen_siyue` | |
| 33 | `n33_jianshi` | `ch13.jianshi` | `ch13_waihai` | `trigger_jiaoshi` | |
| 34 | `n34_banyao` | `ch13.done` | `ch13_waihai` | `trigger_dongkou` | |

文案进 `data/text/ch13_objectives.json`（key `ch13.objective.<step id>`）。第 24 步的 done_flag 是计数旗，按 ≥ 1 记一步；第 25 步的提示不写具体次数（门槛 `>= 2` 只在脚本里）。提示文案的禁词与章内先后照 12.2：第 2 步提示（编在 `wenxun`）可写「千叶露」——他进原料铺前就是带着这个名字去问的（ch501 第 14 段），12.2 把千叶露放在 `wenxun` 起合法；「伴妖草」自第 16 步提示（编在 `cunjin`）起合法（16.1 第 55 条）。`ObjectiveRoute` 沿这条链逐步走（四张新图之间全靠脚本 teleport 串，3.1 末）。**本章新增的按键提示：0 条**。

<!-- 第 13 章施工图（逆星之乱）第二部分：第 4–9 节。写手二，2026-10-13。唯一依据 notes/ch13/research.md（下称「笔记」）；笔记没有、另用 novel.py 核过的原著事实注了章号；引擎与门禁现状另用 grep 核过的注了文件与行号。同一件事实只在一处写全：节点与挂点的权威是第 3 节主线节点表，在场旗标的权威是 5.1，门闸的权威是 7 节「门闸一览」，编成的权威是 8.2，数目的权威是第 9 节；别处只引用。 -->

## 4. 地图（新建 4 张，旧图 0 处修补）

| 地图 id | 名称（`display_name_key`）| 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| default spawn | 出入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `ch13_nanming` | 南明岛 | 48×36 | ✓ | `dock` / `day` / 粒子 `mist` / backdrop `dock_river` | `haishang`（西南礁）| 无门；脚本入口 C1（`scripts/ch12/lingquan.lua` 末行 `teleport("ch13_nanming")`，落 `haishang`）；脚本出口 n06 `teleport("ch13_tianxing")` |
| `ch13_tianxing` | 天星城（戒严）| 56×44 | ✓ | `town` / `dusk` / 粒子 `dust` / backdrop `dock_river` | `chengwai`（南端码头）| 无门；脚本入口 n06；脚本出口 n12 `teleport("ch13_waihai")` |
| `ch13_waihai` | 外星海（北区文案称「奇渊岛」「黑石城」）| 72×56 | ✓ | `dock` / `day` / 粒子 `mist` / backdrop `dock_river` | `shiwu`（北区祭坛石屋内）| 无门；脚本入口 n12（落 `shiwu`）、n30（带坐标落秘市岸，建议 (14, 46)）、五元 #6（带坐标，落点见 3.1 #6）；脚本出口 n16 与五元 #7 → `ch13_wuzhong` |
| `ch13_wuzhong` | 雾中岛 | 64×48 | ✓ | `valley` / `day` / 粒子 `mist` / backdrop `herb_valley`；山脉凹谷里的洞府几间石室用室内底图块画在山体里（第 10 章小寰山的画法）| `andeng`（东岸登岸礁）| 无门；脚本入口 n16 与五元 #7（都落 `andeng`）；脚本出口 n30 与五元 #6 → `ch13_waihai`（都带坐标）|

**图与图之间没有一扇门**：跨图全是脚本传送，三元 5 条、五元 2 条（笔记 N.4，登 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`）。外星海那一头原著就是「能出不能进」（A48）：n12 之后回不去内星海，所以天星城与南明岛都不留回程。主题、时辰、背景全是现有组合（笔记 G 表 2026-10-13 逐张列过 `dock/day/dock_river`、`town/dusk/dock_river`、`valley/day/herb_valley`）。全作地图 75 → 79，超大纲上限 4 张（O-6 记账，E 表 3）。

**为什么外星海是一张图三区**（E 表 4）：黑石城、秘市荒岛、裂风兽小岛原著相隔数日到一月的路，三区三图则全章 6 张；秘市单列则 5 张。合成一张、以中部礁路相连（规则 13 只从 default spawn 做 BFS，三区必须连通），区与区之间不设阻隔——三区的挂点靠 guard 旗标惰性（未到时踩了不响），不靠隔断。秘市地下两层与交换会走 n31 脚本场，不建地下图。

**落地不触发 enter**（引擎口径 1）：C1、n06、n12、n16 的落点 `haishang`、`chengwai`、`shiwu`、`andeng` 都不挂触发器，每张图的首个 enter 挂点是落点前方横贯通道的触发带（n01、n07、n13、n17），玩家迈出第一步才响；通关驱动要先迈一步。带坐标的落点（n30、#6）格上同样不放任何对象。

### 4.1 每张图的挂点与在场 NPC

各 NPC 的出场 / 退场旗标以 5.1 为准，这里只列对象与位置；各挂点的 guard 与完成旗标以第 3 节主线节点表为准。

**`ch13_nanming` 南明岛**（第一幕：n01–n06）
- 布局：西南礁（spawn `haishang`；存档点 `facility_cunji_nanming` 紧挨它、在带南侧）→ 北面一行横贯礁路的 n01 触发带 → 港口小城（原料铺）→ 码头 → 港后丘陵坡口 → 港口客栈一角 → 港口东出海口 → 出海口外礁头。
- 挂点：`trigger_wenxun`（n01，enter，横贯带）、`trigger_qiuling`（n03，enter，丘陵坡口）、`trigger_chugang`（n05，enter，出海口一行横贯带）；n02 挂 NPC `npc_yuanliao_zhanggui`、n04 挂 `npc_cang_yunlong`、n06 挂 `npc_ling_yuling`。
- 设施：`facility_cunji_nanming`（存档）、`facility_dazuo_nanming`（meditate，**不写** `effectiveness`〔缺省 100，契约 1.2〕，客栈一角，**在 `trigger_chugang` 之前的路上**——第 12 章 ④ 之后血未必满，① 前要有回血处，E 表 39）。
- NPC：小城 `npc_yuanliao_zhanggui`；码头 `npc_sanxian_laozhe`、`npc_cang_yunlong`、`npc_liu_hufa`、`npc_xin_ming`；出海口外礁头 `npc_ling_yuling`（在出海口带之外，n05 之后才在）。路径行动 K1–K3 挂三仙宗老者、刘护法、辛明。
- 逆星盟攻港（n03）、银衫布阵绿袍搜岛只在脚本里，不摆群众 NPC。

**`ch13_tianxing` 天星城（戒严）**（第二幕：n07–n12）
- 布局（南 → 北，按圣山层数往上）：南端码头（spawn `chengwai`；存档点 `facility_cunji_tianxing` 紧挨它、在带南侧）→ 城门前横贯带 → 城门 → 圣山脚坊市门 → 第四层宅院 → 第三十九层洞府 → 第四十九层石阶 → 第五十层外廊 → 星空殿传送大厅。
- 挂点：`trigger_rucheng`（n07，enter，`chengwai` 北面城门前横贯带）、`trigger_dongfu39`（n08，interact，洞府门）、`trigger_xingkong`（n09，enter，五十层外廊横贯带）、`trigger_zhaiyuan`（n10，interact，宅院院外）、`trigger_lanlu`（n11，enter，四十九层石阶横贯带，在 `xingkong` 之下）、`trigger_fangshi_bimen`（interact、once 否、无旗标，只一句「坊市闭门」，A34）；n12 挂 NPC `npc_gu_laozhe`。
- 设施：`facility_cunji_tianxing`（存档）；三十九层洞府里 `facility_yaoyuan_39`（field、`ref_id field_tianxing_39`、`slots 6`）与 `facility_jingshi_39`（meditate、`effectiveness 125`），门闸见 7 节。
- NPC：城门 `npc_zhao_hufa`、`npc_yu_hufa`；传送大厅 `npc_gu_laozhe`、`npc_qian_xiushi`、`npc_yi_jing_tx`、`npc_zhang_xiushi`。K4–K7 挂乾姓修士、赵护法、于护法、张姓修士。**不摆路人**：戒严，空街、凡人闭户（A30）。
- 走位次序靠 guard：n08（三十九层）→ n09（五十层，途经四十九层时 `trigger_lanlu` 的 guard `ch13.qianru` 未置、踩了不响）→ n10（下到第四层）→ n11（回四十九层）→ n12（大厅）。
- 对象名 `facility_yaoyuan_39`、`facility_jingshi_39` 与 `ch11_dongfu` 上的同名对象重名（`tools/mapgen/genmaps_ch11.py` 第 205–209 行）：对象名按图分，`validate.py` 无跨图重名规则（2026-10-13 grep）；同一个 `ref_id` 跨两图由契约 E2 钉。

**`ch13_waihai` 外星海**（第三幕 n13–n16；第五幕 n31–n34）
- **北区黑石城**：祭坛石屋（spawn `shiwu`；存档点 `facility_cunji_qiyuan` 在石屋内紧挨它）→ 石屋门外横贯带 `trigger_shiwu`（n13，enter）→ 祭坛高台与城中（明珠轩、寸金阁、告示墙）→ 西门横贯带 `trigger_chucheng`（n16，enter）→ 西门外礁头 `trigger_hui_wuzhong`（五元 #7，与西门带错开 ≥ 3 格；#6 的落点在两者之间、坐标写死、不压带，3.1 #6）。告示墙 `trigger_xuanshang`（interact、once 否、无旗标，Z2）。
- **中部礁路**连三区，全程走得通；礁路进北区的入口一行横贯带 `trigger_feixu`（enter、once 否、guard `ch13.shouchao`、无 `set_flag`，只一句废墟旁白、不拦路，16.1 第 49 条）。
- **西南秘市荒岛**：秘市岸（n30 落点，建议 (14, 46)，不与任何对象同格；存档点 `facility_cunji_mishi` 在岸旁）→ 乱石堆石壁 `trigger_mishi`（n31，interact）；东侧绿山小洞府前是文思月（n32）。
- **东南小岛**：外礁一行横贯带 `trigger_jiaoshi`（n33，enter）→ 半山巨石后洞口 `trigger_dongkou`（n34，interact）。
- NPC：北区 `npc_xu_yun`、`npc_yi_jing`（祭坛下）、`npc_ming_furen`、`npc_paimaishi`（明珠轩）、`npc_li_xun`、`npc_gu_yu`（寸金阁）；秘市荒岛 `npc_huang_mingli`（乱石堆旁）、`npc_wen_siyue`（绿山小洞府前）。K8–K11、K13 挂易敬、许云、竞宝厅老者、古玉、黄明礼；n14、n15、n32 挂明夫人、离寻、文思月（挂点 NPC 不挂路径行动，笔记 K 表末）。
- **0 战斗、0 `challenge`**：黑石城除竞斗场外严禁斗法（A52）。
- **兽潮之后的北区**：六个 NPC 全撤（5.1）；明珠轩、寸金阁本就挂在 NPC 脚本上，随人撤；告示墙脚本开头判 `ch13.shouchao` 只一句；玩家幕五走回北区先踩 `trigger_feixu`（一句：黑石城已成废墟，ch531），不设阻隔带（E 表 23、4）。

**`ch13_wuzhong` 雾中岛**（第四幕：n17–n30）
- **东岸**：登岸礁（spawn `andeng`；存档点 `facility_cunji_wz` 紧挨它、在带南侧）→ 北面横贯带 `trigger_lifu`（n17，enter）；码头入口一行横贯带 `trigger_lidao`（n30，enter）、码头尽头一格 `trigger_qiyuan_hang`（五元 #6，与码头带隔 ≥ 2 格）；林边光幕前 `npc_gongsun_xing`（n29 挂点）、`npc_sun_shishu`（K12）。
- **山脉凹谷洞府**：洞府甬道两条横贯带分在两行——内侧 `trigger_dizhen`（n21，enter）、外侧 `trigger_chuguan`（n28，enter）；六扇门各一格：`trigger_chongshi`（n18）、`trigger_shichuang`（n19）、`trigger_lianjianshi`（n20）、`trigger_biguanshi`（n22）、`trigger_danfang`（n26）、`trigger_jingshimen`（n27），都 interact；设施 `facility_jingshi_wz`（meditate、`effectiveness 150`）、`facility_danlu_wz`（alchemy、`grade 4`）、`facility_yaoyuan_wz`（field、`ref_id field_wuzhong`、`slots 6`），**无炼器台**（E 表 17）；药园旁一格 `trigger_yanghun`（interact、once 否、无旗标，Z3）。
- **推荐（本稿拍板）**：静室、丹炉、药园与六扇门都在**内侧带之内**——n27 之后玩家在静室反复坐关不必跨过 `trigger_chuguan`；n28 的「瓶颈未破」只在他往外走时响（`chuguan.lua` 早退不置旗，这条带每次跨过都会再判一次，直到破关）。
- **西海珊瑚礁**（同图，礁路与东岸连通）：登礁横贯带 `trigger_buzhen`（n23，enter）→ 阵心礁 `trigger_youyao`（n24，interact、once 否）→ 礁外收阵石 `trigger_shoushou`（n25，interact，与阵心隔 ≥ 2 格）。「西飞一月」只在 n23 的 `advance_days(30)` 与文案里，图上是一段礁路。
- 巨猿傀儡打理洞府、退鹰鸢兽只在文案（A83、A117），不摆 NPC。

### 4.2 命名、属性与美术

- `chapter=13`；`display_name_key = ch13.map.<短名>.name`（`nanming`、`tianxing`、`waihai`、`wuzhong`）；`region` 由协调者定一个值（笔记无；第 11 章 `luanxinghai_zhongbu`）。显示名「南明岛」（501）、「天星城」（389）、「外星海」（389）、「雾中岛」（517）都 ≤ 542。
- `bgm`：**只用已有曲子**，每图一首由关卡路挑（建议南明岛 `bgm_river`、天星城 `bgm_town`、外星海 `bgm_river`、雾中岛 `bgm_valley`）；`tools/audiogen/catalog.py` 在 `MAP_BGM` 上方的注释表加 4 行 `# ch13_xxx -> bgm_yyy`（第 45–51 行是第 11 章的样子），`docs/audio.md` 同一张表加 4 行。
- `tools/mapgen/genmaps_ch13.py`（4 张）接进 `genmaps.py --check`；`tools/artgen/artgen.py` 烘 4 张并 `--check` 过；**本章不 patch 任何旧图**（C1 是改一行脚本，不是地图）。
- 存档点 5 处（南明岛、天星城、黑石城石屋、秘市岸、雾中岛东岸），打坐设施 3 处（南明岛缺省 100〔不写〕、天星城 125、雾中岛 150）。
- **同一格只取第一个 trigger**（引擎口径 4）：本章没有同格换触发器；`trigger_youyao` 与 `trigger_shoushou`、`trigger_qiyuan_hang` 与 `trigger_lidao`、`trigger_hui_wuzhong` 与 `trigger_chucheng`、`trigger_dizhen` 与 `trigger_chuguan` 都错开；spawn 与带坐标的落点格上不放对象。

### 4.3 地图的整改拍板（与笔记不一致处，推荐做法）

1. **设施不写 `deny_text_key`**：笔记 G 表给 `facility_yaoyuan_39`、`facility_jingshi_39`、雾中岛三设施写了 `deny_text_key ch13.block.yaoyuan / jingshi / dongfu`。引擎拦设施时只播 `ui.facility.locked`（`src/game/Application.cpp` 第 688–692 行），全仓只有 `WorldScene.cpp` 第 169 行的**传送点**读 `deny_text_key`；规则 20（`validate.py` 第 440–470 行）也只查传送点。写了就是三条永远播不出的文案、还要在文案表里登 key（`check_reference_pair` 查 `_text_key` 字段存在）。所以照第 11 章 `ch11_dongfu` 的写法（`genmaps_ch11.py` 第 205–209 行）**只写 `require_flag`**，这三条 key 不建。本章没有传送点，规则 20 无实例。
2. **雾中岛洞府内外两条带的相对位置**（上文「推荐」）：笔记只写「分在两行」；本稿定内侧 `dizhen`、外侧 `chuguan`，静室与丹炉在内侧带之内。

---

## 5. 角色

**新建 30、就地改写 1（`wuji_haishou`）、一个字不动 2（`fan_furen`、`wen_siyue`）**（笔记 H 表）。凡说话、上场的人都有 `data/roles/` 条目；外观进 `data/visual/looks.json`，`sprite_preview.py --coverage` 缺图为 0。境界写 realm 枚举，「只说话」的 role 数值照 `Realm.h` 锚点（笔记 C.2）与第 10–12 章路人写法；名牌就是 role 的 `name`。**万三谷、六道极圣、妙鹤真人、极阴祖师、漩矶本章 0 处上场**（只闻其名或不提，A5、A102、A109–A110）。

### 5.1 剧情角色（地图上有对象的；**在场旗标的权威表**）

| id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 在场（图 `对象`：出场 → 退场）|
| --- | --- | --- | --- | --- | --- |
| `sanxian_laozhe` | 三仙宗老者 | 499 | FoundationLate（估）| 新建 | nanming `npc_sanxian_laozhe`：`wenxun → gongdao` |
| `nanming_zhanggui` | 原料铺掌柜 | 501 | FoundationMid（估）| 新建 | nanming `npc_yuanliao_zhanggui`：`wenxun →`（不撤；封市后只一句）|
| `cang_yunlong` | 苍云龙 | 502 | CoreEarly | 新建 | nanming `npc_cang_yunlong`：`gongdao → panzha` |
| `liu_hufa` | 描述名牌（如「逆星盟壮汉」，不含姓；16.1 第 44 条）| 503 | CoreEarly（估）| 新建 | nanming `npc_liu_hufa`：`gongdao → panzha` |
| `xin_ming` | 辛明 | 502 | FoundationMid（估）| 新建 | nanming `npc_xin_ming`：`gongdao → panzha` |
| `ling_yuling` | 凌玉灵 | 505 | CoreEarly（估；文案不报境界）| 新建 | nanming `npc_ling_yuling`：`zhuibing → yinjian` |
| `zhao_hufa` | 赵护法 | 506 | CoreEarly | 新建 | tianxing `npc_zhao_hufa`：`yinjian → chuansong` |
| `yu_hufa` | 于护法 | 506 | CoreMid | 新建 | tianxing `npc_yu_hufa`：`yinjian → chuansong` |
| `gu_laozhe` | 顾姓老者 | 509 | CoreEarly | 新建 | tianxing `npc_gu_laozhe`：`yirong → chuansong` |
| `qian_xiushi` | 乾姓修士 | 509 | CoreEarly | 新建 | tianxing `npc_qian_xiushi`：`yirong → chuansong` |
| `zhang_xiushi` | 张姓修士 | 507 | FoundationMid | 新建 | tianxing `npc_zhang_xiushi`：`yirong → chuansong` |
| `yi_jing` | 易敬 | 508（易兄）/ 511 | FoundationLate | 新建 | tianxing `npc_yi_jing_tx`：`yirong → chuansong`；waihai `npc_yi_jing`：`chuansong → shouchao` |
| `xu_yun` | 许云 | 511 | FoundationLate | 新建 | waihai `npc_xu_yun`：`chuansong → shouchao` |
| `ming_furen` | 明夫人 | 513 | FoundationMid | 新建 | waihai `npc_ming_furen`：`chuansong → shouchao` |
| `jingbao_paimaishi` | 竞宝厅老者 | 513 | FoundationLate（估）| 新建 | waihai `npc_paimaishi`：`mingzhu → shouchao` |
| `li_xun` | 离寻 | 515 | CoreEarly | 新建 | waihai `npc_li_xun`：`chuansong → shouchao` |
| `gu_yu` | 古玉 | 515 | QiRefining13（估：原文只说炼气期）| 新建 | waihai `npc_gu_yu`：`chuansong → shouchao` |
| `sun_shishu` | 孙师叔 | 529 | FoundationEarly | 新建 | wuzhong `npc_sun_shishu`：`chuguan → gongsun` |
| `gongsun_xing` | 公孙杏 | 531 | QiRefining（估：原文炼气期）| 新建 | wuzhong `npc_gongsun_xing`：`chuguan → gongsun` |
| `huang_mingli` | 黄明礼 | 534 | FoundationLate（假丹）| 新建 | waihai `npc_huang_mingli`：`lidao → jiaohuan` |
| `wen_siyue` | 文思月 | 430 | （第 12 章所建）| **不动** | waihai `npc_wen_siyue`：`jiaohuan → wenshi` |

（旗标一律省 `ch13.` 前缀。）名牌与首见的口径见 16.1 第 44 条：易敬、辛明、凌玉灵、赵护法、明夫人、离寻、古玉、公孙杏等上图早于他听到名字，名字都在他与此人头一回能对话的那个节点里当面出口，记为名牌例外；`liu_hufa` 与 5.2 的闵、宣两人，名字他从没当面听到，用不含姓的描述名牌。`gongsun_xing` 的境界列照笔记写「QiRefining（估）」——realm 枚举没有不带层数的炼气，施工时取 `QiRefining1`–`QiRefining13` 中一个具体层（`Realm.h` 第 17–29 行），由内容路定，不影响任何判据。

### 5.2 只说话的（无地图对象，节点脚本 `talk` 用）

| id | 名牌 | 首见章 | 境界 | 在哪一段说话 |
| --- | --- | --- | --- | --- |
| `liu_furen` | 刘夫人 | 508 | FoundationEarly（估）| n10 七人议事 |
| `qiyuan_kanhu` | 看护修士 | 511 | FoundationEarly（估）| n13 石屋 |
| `qiu_laozhe` | 鸠面老者 | 524 | CoreMid | n25 |
| `min_xiushi` | 描述名牌（不含姓，16.1 第 44 条）| 524 | CoreEarly | n25 |
| `xuan_xiushi` | 描述名牌（不含姓，16.1 第 44 条）| 524 | CoreEarly | n25 |
| `yun_tianxiao` | 云天啸 | 536 | CoreMid | n31（东主；白骨阴火只在文案，不建法术）|
| `sun_heifu` | 孙姓修士 | 537 | CoreMid | n31（铜片主人）|
| `fan_furen` | 范夫人 | 400 | FoundationLate | n31（**不动**）|

### 5.3 只在战斗里的（编成用，无地图对象）

| id | 名牌 | 境界 | 新建 / 改写 | 编成 |
| --- | --- | --- | --- | --- |
| `nixing_rusheng` | 白面儒生 | CoreEarly | 新建（敌）| ① |
| `nixing_dahan` | 黑壮大汉 | CoreEarly | 新建（敌）| ① |
| `wuji_haishou` | 五级海兽 | CoreEarly | **就地改写**（B25：`weaknesses` 补「剑」，note 改写）| ② 第 0 波 |
| `qingse_juxia` | 青色巨虾 | CoreMid | 新建（敌，非人形）| ② 第 1 波 |

破绽、架势、蓄势与 `killable_by` 以 8.2 为准。

### 5.4 不建、外观

- **不建 role、只旁白**（笔记 H 表末）：三仙宗同行者、银衫 / 绿袍逆星盟众、王长老、赵护法手下一队、七人里其余四人、明珠轩带路女子、竞宝厅拍卖师与竞宝诸修、巨龟、毒蛟、金霞道士、霍氏兄弟、鹰鸢兽、巨猿傀儡、青灵门其余三人、锻蓝袍胖老者（原文自称「齐某人」，ch536，与 ch527 极阴手下的齐姓道士是两个人；文案不点他的姓，12.2「齐姓」0 处）、红巾怪人、青夫人、吴姓老者、两名青衣修士、秘市守门老者、章末那个声音（E 表 28：不写名、不现身、不建出场 role）。
- **外观**：人形按名牌身份配；**非人形 2 个**——五级海兽、青色巨虾（三四丈长、六只碧绿的眼，ch523），进 `tools/artgen/sprites_enemies.py`，待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下（第 8、10–12 章同口径）。2026-10-13 grep：`wuji_haishou` 在 `looks.json` 与 `sprites_enemies.py` 里都没有条目（第 11 章 `siji_haishou` 同样没有）——美术路先核 `--coverage` 对只在编成里的 role 怎么算，再定两张都补还是照第 11 章口径。

### 5.5 撤场 / 出场与规则 30（`NpcPresenceTests`、`validate.py` 规则 30）

- **跨图**：`yi_jing` 天星城那个的退场旗标 `ch13.chuansong` 恰是外星海那个的出场旗标，首尾相接；`wen_siyue` 第 12 章 `ch12_tushan` 那个以 `ch12.lidao` 撤场，早于本章 `ch13.jiaohuan`。其余 role 只在一张图上有一个对象。
- **同图同 role**：本章 0 例（规则 30 只放行「一个的 hidden 恰是另一个的 visible」，用不上）。
- **不同 role 同名牌**：同图同时在场的名牌两两不同（逐图核过：南明岛 6 个、天星城 6 个、外星海 8 个、雾中岛 2 个）；「孙师叔」（wuzhong）与「孙姓修士」（只说话）、三个「老者」（三仙宗 / 顾姓 / 竞宝厅）都不同名、不同图或不同时。
- **链外在场旗标**：黑石城六个 NPC 的退场旗标 `ch13.shouchao` 不在目标链上（笔记 J.2）——登 `kOffChainFlags`，`after ch13.chuguan`、`before ch13.gongsun`（`gongsun.lua` 里先置 `shouchao`、后置 `gongsun`）。其余在场旗标全在目标链上。

---

## 6. 支线（5 条：必经 2、可选 3）

| | `q13_qianyelu`「千叶露」| `q13_gongsun_qiuyao`「公孙杏之求」| Z1 `q13_mingzhu_dingdan`「明珠轩订单」| Z2 `q13_haishou_xuanshang`「海兽悬赏」| Z3 `q13_yanghun_zhu`「养魂木珠」|
| --- | --- | --- | --- | --- | --- |
| 必经 / 可选 | **必经** | **必经** | 可选 | 可选 | 可选 |
| 出处 | ch501 掌柜不识千叶露（A12）→ ch516 古玉典籍查明（A63）→ ch540 文思月给巢穴位置（A142）| ch531–533 为瘫父求药（A120–A123）| 改编：大纲「商行订单」；明珠轩收购海外所得（A54）| 改编：黑石城告示墙「悬赏制度」| ch518 新绿液催养魂木（A76）、ch523 已成形 |
| 接取 | `ch13.caigou` | `ch13.chuguan` | `ch13.mingzhu` | `ch13.daoqiyuan` | `ch13.lifu` |
| 步骤与挂点 | s1 `ch13.cunjin` → s2 `ch13.wenshi`；无独立挂点，全在主线 | 无独立挂点，即 n29 | `mingzhu.lua` 再谈分支：六级妖丹 ≥ 1 才列「交货」→ `take` 1、`give("material_lingshi_zhong", 30)`、置 `ch13.z1` | `trigger_xuanshang`：开头判 `ch13.shouchao` 一句 return、判 `ch13.z2` 已置一句 return；五级妖丹 ≥ 3 才列「交」→ `take` 3、`give` 中阶 15 ＋ 霓裳草 3、置 `ch13.z2` | `trigger_yanghun`：`ch13.buzhen` 未置一句（木未成形）return；`ch13.z3` 已置一句 return；根须 ≥ 1 且绿液 ≥ 3 → `bottle.spend(3)`、`advance_days(60)`、`take` 根须 1、`give` 养魂木珠 1、置 `ch13.z3`；不足一句 |
| 完成 | `ch13.done` | `ch13.gongsun` | `ch13.z1` | `ch13.z2` | `ch13.z3` |
| 失败 / 过期 | — | — | `ch13.biguan` | `ch13.biguan` | `ch13.lidao` |
| 奖励 | 不进 rewards | 不进 rewards | 不进 rewards（脚本 `give`）| 同左 | 同左 |
| 文件 / origin | `data/quests/q13_qianyelu.json`，原著 | `q13_gongsun_qiuyao.json`，原著（改编：去炉鼎，E 表 21）| `q13_mingzhu_dingdan.json`，改编 | `q13_haishou_xuanshang.json`，改编 | `q13_yanghun_zhu.json`，原著 |

- **完成压过失败**（`Quests.cpp` 先判 complete）：Z1、Z2 失败于 `ch13.biguan`——n27 起往返黑石城关闭（7 节门闸），`biguan` 之后玩家要到 n30 才回外星海，那时 `ch13.shouchao` 已置、明夫人已撤、告示墙脚本开头就 return，完成旗标不会在过期之后被补上。Z3 失败于 `ch13.lidao`，之后再回不了雾中岛。
- **Z3 的门闸**：笔记 N.3 写接取 `ch13.lifu`、J.1 写「`ch13.buzhen` 后 Z3 可交」、F 表 n23 写「养魂木两年已成形（Z3 自此可接）」——三处合起来的意思是**接得早、交得晚**：任务在立府时出现在任务栏，`yanghun.lua` 开头判 `ch13.buzhen`。本稿照此写，不改接取谓词。
- **意图玩家不做 Z1–Z3**（笔记 N.1 第 4 条）；O-5：Z3 把根须换成珠，第 14 章若要读养魂木，两种 id 都认。
- **配额**（如实，笔记 N.3）：第 12 章后 16 条（明标必经 1）；本章 ＋5（必经 2）→ **21 / 必经 3**；大纲 45 / 14 缺 24 / 11，第 14 章一章补不上——**已定**（16.1 第 51 条）：回写大纲第 5 节支线目标为实际可达值（数由第 14 章研究路按其预算给），本章最终分担 5 条（必经 2），C7 回写清单含此项。**不凑数**：秘市诸摊、竞宝厅、鉴宝厅、听风阁、联合大拍卖都只落在路径行动与 n14、n31 的选项里。
- **不做成支线的**：苍云龙托话给紫灵（n04 一句；紫灵第 14 章以后）；范夫人幻梦石之邀（他回绝，A138）；文思月为妾（原著作罢，E 表 26）；黄明礼之后的去向（原著没写）；虫魔真相（判据③，E 表 35）。

---

## 7. 系统：战乱封锁、偷渡、立府经营、诱妖与炼丹、结丹中期自坐、旧灵田、秘市

**先查了已有的，再定做多少**（笔记 D 表）：`realm.cap` / `realm.at_least` 与面板冲关 `tryBreakthrough`、facility `effectiveness`、`field.unlock` 与 facility `field` 的 `ref_id`、`shop(id)` 与 `Shop{buyRate,sellRate,entries}`、配方 `requireFlag`、`magic.learn` 看返回值、`effect "stagger"`、编成的 `wave` / `charge` / `actions` / `killable_by` / `can_escape`、第 11 章诱妖循环口径（`item.count`、`bottle.drops`、`bottle.spend`、`advance_days`）、`teleport(map_id, x, y)` 与五元 `kScriptTransfers`、NPC `visible_flag` / `hidden_flag`、路径行动 `when` / `until`。**本章不加 Lua 命令、不加存档字段（仍 v9）、不改 `src/`**；契约只有两条判据项：**E1 结丹档面板冲关**、**E2 跨图同一块田**（10.1）。拍卖面板（`AuctionLot` 只有规则层）本章不做（E 表 25）。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **战乱封锁** | **按地点分店**（E 表 14）：南明岛原料铺 `ch13_nanming_fangshi`（`buyRate 0.9`、限量不补）由 n02 掌柜脚本 `shop()` 开，`ch13.gongdao` 之后再谈只一句「封市」；天星城坊市 `trigger_fangshi_bimen` 只一句闭门；传送阵改条件在 n07 于护法口中（任务或大战当众杀同阶、先前不算），「报名 / 安分」两支都领 `story_huangse_zhihuan`、只换一句；外海「能出不能进」在 n13 看护一句，图上不留回程 | 不做物价曲线（`buyRate` 按旗标切换是新引擎项）；不让 ① 抵传送条件（A32「先前不算」）；不做竞价 | 原著 ch500–542「物价」0 命中，只有略便宜、闭门、改条件、黑价、只出（B8）——封锁是这一章的底色，不是一条涨价线 |
| **偷渡三支付法** | n12 `choice` 只列付得起的支：**灵石 10000**（`item.count` 够才列，`take` 看返回值）/ **五级妖丹 4** / **把柄 ＋ 胡月飞刀两柄**（旁白，不扣物，**永远在**）；记 `ch13.toudu` 1 / 2 / 3，n13 只换易敬一句（E 表 12）；「结丹给多少都不放」由 n11 易容压成假丹落地（A38、A41）| 不硬收五千；飞刀不进背包（A45 只旁白）；不做「被识破」分支 | 起点灵石估不过四千（C.3），硬收是死局；「五千一人」不能只剩台词，所以有钱的玩家可以照价付双倍（A43）|
| **易容与两个化名** | 天星城「曲」（n11–n15，见 12.2、16.1 第 43 条）、秘市「厉」（n31–n33）只在他人对白里；换形诀、法力七成只在文案（A125）| 不换名牌、不换主角 role、不做伪装值（E 表 34）| 换名牌要换主角 role，牵动全部战斗 |
| **两样新手段** | **五行环**：n01 `magic.learn("magic_ji_wuxinghuan")` 看返回值（金、`effect stagger`、`stagger 3`、`power 0`）；`story_wuse_tonghuan` 不动（E 表 7）。**蜂云合剑**：n20 `take("material_lianjing", 1)` 看返回值（false 一句照走）→ `advance_days(360)` → `magic.learn("magic_fengyun_heji")` 看返回值（木，七道青光合剑，A98；改编名，图鉴标「改编」）| 剑上辟邪神雷不给（A26：此时无神雷）；血色披风不做战斗手段（A9）；**本章 0 张炼器配方**、重炼不走面板（E 表 8）；五行环不做 `castMagic` 道具 | 古宝不该「用一件扣一件」；炼晶只一块，面板炼器失败折半材料＝死局，再开一张 `guaranteed` 违反「全仓只许一张」（B31）|
| **立府经营** | n17 `field.unlock("field_wuzhong", 6)`、`give("herb_nichang_cao", 3)`；静室（150）、丹炉（grade 4）、药园三设施与码头往返自此开（4.1）；往返黑石城两条五元传送（#6 / #7），每趟 `advance_days(12)`、`choice`「去 / 不去」，取消即不去——**卖妖丹、买霓裳草与黄精、交 Z1 / Z2 都靠它** | 不做建造、年度结算界面、奇虫养成（虫室、幻焰蛾、新噬金虫只旁白，A72–A74、A91）；不给炼器台（E 表 17）| 原著全作唯一一段慢日子（导读第 4 句）；不给往返则 ② 的妖丹无处卖，经营循环断（E 表 24）|
| **诱妖与收手** | n24 `trigger_youyao`（interact、once 否）照第 11 章口径（E 表 18）：`ch13.biguan` 已置 → 一句 return；草 ≥ 1 且绿液 ≥ 1 → `bottle.spend(1)` → `advance_days(3)` → ②；胜扣草 1、`ch13.lieyao` ＋1，逃不扣草（液已扣），败 game over；缺草一句指寸金阁（灵石 < 10 时兜底给 1 株，7.1），缺液 `advance_days(5)` return。n25 `trigger_shoushou` 判 `ch13.lieyao >= 2`，够则七年蒙太奇 `advance_days(2415)`、`give` 六级 6 ＋ 五级 10 ＋ `story_liuqi_cailiao` 1（E 表 19）| 不做「数百枚」真数量（A107 按比例收成）；诱妖阵只叫「法阵」「阵」，「困妖」是禁词（M 表，首见 1038）；不做深海遭遇 | 第 11 章已验过的整套断言不重写；「四百年霓裳草」只在文案（A92），三日一滴恰好一轮 |
| **炼丹与至元丹** | n26 `advance_days(1080)`、置 `ch13.liandan` → 解锁 `recipe_zhiyuan_dan`（五级妖丹 1 ＋ 黄精 1 → 至元丹 3，`guaranteed` 不写，`difficulty` 由平衡路定、意图玩家成功率 ≥ 50%）；n29 至元丹**三支兜底**（E 表 22）：至元丹 ≥ 3 → `take` 3 / 六级妖丹 ≥ 1 → `take` 1 / 再否则 `advance_days(30)` 照走，记 `ch13.zhiyuan`；公孙杏之约改「入阵为仆」（E 表 21）| 不做「炼丹大师」数值；不强制炼几炉；九曲灵参配方不建 | 只认自炼的丹则炼丹失败的玩家卡死；照原著写「稚女炉鼎」越过内容尺度（B30）|
| **结丹中期自坐**（契约 E1）| n27 `local ok = realm.cap(realm.CORE_MID)`（false 一句照走），之后玩家在 `facility_jingshi_wz` 按「闭关一年」自冲；n28 开头 `realm.at_least(realm.CORE_MID)` 不到只一句「瓶颈未破」、不置旗、不拨日（E 表 10、11）。数：基准每日 0.9 点、计顿悟期望约 1.02 点；冲关 5000 点、到线 85%（60 ＋ 资质 25 ＋ 丹 0），失败倒扣 1000（笔记 C.2）| **`realm.advance` / `realm.demote` 本章 0 处**；不改 `aptitude`（五色圆珠只旁白，A144，E 表 38）；红光丹药 0 处成物（A114）；`breakthroughPillBonus` 仍恒 0 | 经营章最该给玩家的那一下是自己坐出来的；推翻第 12 章契约「境界」行（B20，O.1 已写）|
| **旧灵田**（契约 E2）| `ch13_tianxing` 三十九层挂 `facility_yaoyuan_39`（`ref_id field_tianxing_39`），玩家自采第 11 章种下的作物；n08 只旁白装虫、弃府（E 表 16）| 不加「一键采收」Lua 命令；脚本不搬作物；n08 之后不设「采完才能走」 | 不放则第 11 章的药白种；同一 `ref_id` 跨图已有 `shenshougu_yaopu` 先例（契约第 0 节 E2，O-2 结案），E2 只钉旧田不新建、不改尺寸 |
| **三件旧物落地** | n19 `take("story_xuangu_leigu", 1)` 看返回值、`give("story_xuanyin_jing", 1)`；n18 `give("story_wuse_yuanzhu", 1)`、n22 `take` 1；n22 `story_jinsi_can` 余数 > 0 才 `take` 余数（E 表 29、B19）| 玄阴经不给常驻功法与法术、天都尸火不做（A78–A79，E 表 9）；虚天鼎不开（A80）；冰珠不碰 | 他修青元剑诀、不重修玄阴大法；全是「以后的东西」|
| **兽潮只经人口** | n29 孙师叔一句、置 `ch13.shouchao`；K12 是想多问的细节；黑石城六人撤场、两店随人没（5.1）| 不演兽潮、不演黑石城被破（判据③，E 表 23）| 他在雾海闭关二十余年，不在场 |
| **秘市与交换会** | n31 一份脚本场（E 表 25）：引荐 → `shop("ch13_mishi_tan")` → 听风阁一句 → 交换会三处抉择（铜片三支必得 E 表 27 / 以整堆材料换她自由，「再看看」再给一次 / 范夫人之邀）→ 云天啸一击是旁白 → 带文思月离场 | 拍卖、竞价全是选项，不做面板；云天啸不做战斗（E 表 5、O-1）| 原著十二柄小剑一击了事（A137），做成仗是灌水 |
| **章末人声** | n34 入洞 `give("story_banyao_cao", 1)` → 身后一个男子声音（一句）→ 置 `ch13.done`；不 `ending()`、不 teleport（E 表 28）| 不写名、不现身、不建出场 role；「风某」「风希」「九级」「化形第二阶」0 处 | 写到他现身就用掉第 14 章开场（风希 ch543）|

### 7.1 门闸一览（**本章门闸的权威表**）

| 门闸 | 挂在哪 | 怎么拦 | 不到时 | 为什么不会卡死 |
| --- | --- | --- | --- | --- |
| 三十九层药园、静室 | `facility_yaoyuan_39`、`facility_jingshi_39` | `require_flag ch13.rucheng`（不写 deny，4.3）| 引擎播 `ui.facility.locked` | `rucheng` 是城门前的带，进城即置；数据上几乎碰不到 |
| 雾中岛三设施 | `facility_jingshi_wz`、`facility_danlu_wz`、`facility_yaoyuan_wz` | `require_flag ch13.lifu` | 同上 | `lifu` 是登岸礁北面的带 |
| 南明岛原料铺 | `caigou.lua`（NPC）| 脚本判 `ch13.gongdao` | 一句「封市」| 店是便利，不是门槛 |
| 天星城坊市 | `trigger_fangshi_bimen` | 永远一句 | — | 只是封锁的样子 |
| 南明岛盘查 | `panzha.lua`（NPC）| `item.count("story_miaoyin_yaopai")` | 无腰牌 → 报名号，照放（数据上不可达；不提画像，16.1 第 41 条）| 腰牌只读不扣（E 表 15）|
| 偷渡付费 | `chuansong.lua`（NPC）| 选项按 `item.count` 只列付得起的 | — | 第三支永远在（E 表 12）|
| 去黑石城 | `trigger_qiyuan_hang`（五元 #6）| guard `ch13.lifu`；脚本开头判 `ch13.biguan` | 一句「闭关在即」return | 往返是可选的经营，不在意图玩家路上 |
| 回雾中岛 | `trigger_hui_wuzhong`（五元 #7）| guard `ch13.lifu`；脚本开头判 `ch13.lidao` | 一句 return | 已离岛 |
| 诱妖 | `trigger_youyao` | `ch13.biguan` 未置；草 ≥ 1 且绿液 ≥ 1 | `biguan` 已置一句；缺草且灵石 ≥ 10 一句（指寸金阁）；缺草且灵石 < 10 → `advance_days` 后兜底 `give` 霓裳草 1；缺液 `advance_days(5)` return | n17 必给霓裳草 3，但它 `tradeable true`、价 10，玩家可以卖光；寸金阁霓裳草每 5 日补到 5 株、价 10，往返不花钱；全章没有一处强制花灵石——卖光草又花光钱的极端由兜底那一支接住（16.1 第 53 条）|
| 收手 | `trigger_shoushou` | `ch13.lieyao >= 2` | 一句 return、不置旗 | ② 可重复、可逃 |
| 出关 | `trigger_chuguan` | `realm.at_least(realm.CORE_MID)` | 一句「瓶颈未破」、不置旗、不拨日 | 静室随时可进，失败只折修为 |
| 至元丹 | `gongsun.lua`（NPC）| 三支只列有的 | — | 第三支 `advance_days(30)` 永远在（E 表 22）|
| 铜片 | `mishi.lua` | 三支只列付得起的；`story_liuqi_cailiao` 的 `take` 看返回值 | false 只换一句 | 第三支以桌上材料抵（旁白不扣）永远在（E 表 27）|
| 旧物 `take` | n19、n20、n22 | 看返回值 | false 只换一句照走（**推荐**：笔记只给 n20 写明，n19、n22 同口径）| 三件都由第 12 章给、0 处中途扣 |
| Z2 / Z3 | `trigger_xuanshang`、`trigger_yanghun` | 见第 6 节 | 一句 | 可选 |
| 明珠轩 / 寸金阁首谈 | `mingzhu.lua`、`cunjin.lua`（NPC）| 脚本开头判 `ch13.daoqiyuan` / `ch13.mingzhu` | 一句 return，不置旗、不给物 | 两人自 `chuansong` 起就在图上（5.1），不判则可先找离寻、整步跳过 n14（16.1 第 47 条）|
| 闭关 / 离岛 | `biguan.lua`、`lidao.lua` | 开头 choice 确认 | 取消在拨日、置旗前 return | 两处都不可逆（Z1–Z3 由此收口），确认之后才置旗（16.1 第 52 条）|
| 兽潮后的北区 | `trigger_feixu` | guard `ch13.shouchao` | — | 只一句废墟旁白、不拦路；北区六人已撤（16.1 第 49 条）|

区域挂点（外星海三区、雾中岛西海礁）**不设门闸**：未到时 guard 未置、踩了不响（引擎口径 2：portal / facility 不读 visible / hidden，门闸只用 `require_flag` 或脚本开头判旗标）。

### 7.2 起点与章末对照（写测试时读表，不抄数；供第 14 章参考）

| | 起点（ch12-end，估）| ① 时（n05）| ② 时（n24）| n27 之后 | 章末 |
| --- | --- | --- | --- | --- | --- |
| 境界 / 上限 / `formerRealm` | 31 / 31 / 22 | 同左 | 同左 | 31 / **32** / 22 | **32 / 32 / 22**（玩家自冲；n28 已判）|
| 气血 / 法力上限 | 700 / 470 | 同左 | 同左 | 同左 | **900 / 620** |
| 面板冲关回报 | `StoryCap` | 同左 | 同左 | 修为 < 5000 `NotEnough`、≥ 5000 `None`（85%）| `StoryCap`（上限 32）|
| 最近的打坐处 | — | 南明岛 100 | 雾中岛静室 150 | 150 | — |
| 法术 | 12 门（第 10 章 11 门 ＋ 噬金虫群）| 13（＋五行环）| 14（＋蜂云合剑）| 14 | 14 |
| 队伍 | 空 | 空 | 空 | 空 | 空（`party.*` 0 处，E 表 37）|
| 小瓶 | 容量 9、结丹 3 日一滴 | 同左 | 同左 | 同左 | 同左 |
| 旧物 | 腰牌 1、肋骨 1、炼晶 1、金丝蚕 0 或 3（估）| 同左 | 肋骨 0、炼晶 0、玄阴经 1、五色圆珠 0、金丝蚕 0 | 同左 | ＋铜片、伴妖草；腰牌仍 1 |

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** ＝ 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **2 场必打**：① n05 `b13_nanming_zhuibing`（新）、② n24 `b13_waixinghai_lieyao`（循环，可重复，**按一场计**——第 11 章 ① 诱妖先例；n25 要 `ch13.lieyao >= 2`，意图玩家胜 2 次）。两场都是原著的仗。**可选：无**（无遭遇区、黑石城禁斗、路径行动 0 条 `challenge`）。
账（笔记 I 表、E 表 5）：方案全作 52 场，第 1–12 章合计 45 场，**本章 2 场**，第 14 章留 **5 场**。云天啸一击、黄明礼救援原著都是一招了事，不升格；若第 14 章用不满 5 场，预留 id `b13_mishi_yuntianxiao`（O-1，不推荐）。

**这两场就是「古宝之威」与「重炼之剑」的说明书**（第 7 节「两样新手段」）：

| # | 谁上场 | 这一仗教玩家什么 |
| --- | --- | --- |
| ① | 韩立一人，无友军、无队伍 | **五行环第一次上手**：锁住一个（削架势 → 破势），趁窗口收掉，再转向另一个——全作第一次「一人对两名同阶修士」 |
| ② | 韩立一人 | **重炼的回报在最后一刀**：五级随手斩，六级首领只有剑与蜂云合剑（木）要得了命 |

### 8.1 意图玩家

- **共同**：结丹初期（700 / 470，攻 62、防 46——笔记 C.2，`Realm.cpp` 第 257、275 行「暂定」）；本章没有一场仗发生在结丹中期之后（② 在 n27 之前收口），结丹中期锚点不标定（O-9，留第 14 章）。兵刃：剑（`weapon_qingzhu_fengyun_jian`，**无神雷**，A26）、拳、暗器（`weapon_wuming_sixian`）。道具：中阶灵石约 20（估，回法力）、南明岛店的回气丹（可选，意图玩家不买）。**队伍为空**（E 表 37）。法术与旧物以 7.2 为准。
- **①**：法术 13 门（第 10 章 11 门 ＋ 噬金虫群 ＋ n01 学的五行环；蜂云合剑还没有）。进场前无仗（n01–n04 全是演出）；第 12 章 ④ 之后血未必满（C.3），`facility_dazuo_nanming` 在出海口带之前（E 表 39）；存档点在 `haishang`。
- **②**：法术 14 门（＋ n20 学的蜂云合剑）。每轮诱妖先 `advance_days(3)`（每日回一成气血）；洞府静室（150）是兜底。意图玩家胜 2 次、不逃。
- **修为**：① `rewards.cultivation` 建议 300；② 建议 ≤ 60（O-10：可重复的仗给多了，就成了冲结丹中期的捷径）。**C0 修正**（协调者备注 2026-10-13）：第 11 章真档章末修为 2572 / 2511，C0 ≥ 该数 ＋ ① 300（第 12 章四场与 ② 所得另加，按 0 计下界），即两侧 C0 ≥ 2872 / 2811 → 坐到 5000 至多约 2090 / 2150 日（按期望每日 1.02 点），**意图玩家按「闭关一年」约 6 次以内**——**`ch12-end` 真档实测两侧都是 2 次（N = 2、M = 720 日，18.5 第 3 条；2026-10-15 回写）**（**本章坐关次数的唯一权威**；1.3、3.3、第 14 节、验收 2、契约第 0 节只引用这里，16.1 第 48 条）；笔记 C.2 的「6–14 次」是真档之前的上界。次数仍以 `ch12-end` 真档为准（O-3），增量 11790 不变。
- **最低合法起点**：中阶灵石 0、无丹药、气血不满。① 前有打坐设施；② 可逃（逃不扣草）、静室随时可进；两场之前都有存档点——没有「存在仗前、打不过、也回不了血」的死局。平衡路两种都要量。

### 8.2 两张必打编成（**编成的权威表**）

| # | 编成 id | 敌方（按波次）| 我方 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b13_nanming_zhuibing`（新）| **第 0 波** 白面儒生 ×1（`nixing_rusheng`，结丹初期；`toughness 3`；破绽金、剑，`killable_by ["金","剑"]`；`magics ["magic_mihuan_guniao"]`——黑雾怪鸟，水、全体小伤）＋ 黑壮大汉 ×1（`nixing_dahan`，结丹初期；`toughness 4`；破绽金、木、剑，`killable_by ["木","剑"]`；蓄势白玉大印 `charge {every 3, mult <平衡路定>, all false, text_key "ch13.battle.nanming_zhuibing.charge"}`）| 韩立一人 | 否 | **致命** | **先锁一个**：五行环（削架势 3）一下打穿儒生的架势，破势窗口里用剑或祭金蚨收掉；大汉每三回合攒一发单体重击，要在那一回合前把他打破势、或防御硬接；怪鸟是全体小伤，压的是法力。打赢不给灵石（灵石由 n05 脚本从储物袋给）|
| ② | `b13_waixinghai_lieyao`（**就地改写**，E 表 6）| **第 0 波** 五级海兽 ×2（`wuji_haishou`，结丹初期；`toughness 3`；破绽土、剑——就地补「剑」，B25；`killable_by` 不写）；**第 1 波** 青色巨虾 ×1（`qingse_juxia`，结丹中期，首领；`toughness 5`；破绽金、木、剑，`killable_by ["木","剑"]`；`actions 2`；`magics ["magic_haichao_jue"]`；蓄势黑气 `charge {every 3, mult <平衡路定>, all true, text_key "ch13.battle.waixinghai_lieyao.charge"}`）| 韩立一人 | **可**（逃不扣草）| **致命** | **先省后收**：第 0 波原著是飞剑随手斩，玩家用剑、土省法；巨虾一回合动两次、每三回合攒一发全体，靠五行环削势或剑破势打断；**收尾只有剑与蜂云合剑（木）要得了命**——金能削它的势、要不了它的命 |

- **必须守住的**（数值归平衡路，按 `tests/BattleHand.h` 那只手量；本表只写意图与破绽）：两张 `hero_absent false`、无友军、`defeat_is_fatal true`、`terrain sea`、`rewards.spirit_stones 0`；① 一波两敌、`can_escape false`、`drops []`、`rewards.cultivation > 0`；② 两波、`can_escape true`、`drops` 恰六级妖丹 1 ＋ 五级妖丹 2（都 `rate 100`）、`rewards.cultivation` ≤ 60。`killable_by` 一律非空且 ⊆ `weaknesses`（不写即不限）；胜负只看气血归零；`charge` 四键 `every / mult / all / text_key` 都必写——`mult` 缺了或 ≤ 0，加载器整份拒（`src/io/DataLoader.cpp` 第 435–436 行），数值归平衡路（16.1 第 59 条）。
- **② 占位就地改写的几处**（B24）：现文件 `rewards` 是 `cultivation 1400`、`spirit_stones 320`、`drops` 只有五级 2，第 1 波不存在；`name`「外星海·五级海兽群」与 `note`「立府雾中岛…ch512–535」一并改写成 ch523 珊瑚礁诱妖。`wuji_haishou` 的 note「写到那一章时重填」同时改写。
- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，C2；E 表 30）：`CHAPTER_MEANS[13]` ＝ 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`weapon_wuming_sixian`，ch07）、剑（`weapon_lieyang_jian`，ch06）；`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]` ＝ `("木","learn","magic_fengyun_heji","ch13")`（`learn` 种类由 `check_means_sources` 去 `scripts/ch13/` 核 `magic.learn`，`validate.py` 第 2537–2552 行；木在通表里已有，这一条是给重炼留的出处）；`MEANS_LAST_CHAPTER = 13`。**已定**（16.1 第 46 条，推翻笔记 E 表 30 括注里的五行环；本段是规则 25 表的唯一权威，10.1 C2、契约 5.2 只引用）：噬金虫可作补充出处；**五行环不登**：`power 0`、`effect stagger`，削得了势、要不了命，登成必有的「金」会让规则 25 对 `killable_by` 含金的敌人放水（友军那一侧 `role_means_available` 也明确排除带 `effect` 的法术，第 2496 行）。
- **selftest 探针**（笔记 I 表末，C2）：② 写实后探针从 `wuji_haishou` ＋ `b13_waixinghai_lieyao` 挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`，`probe["chapter"] = 13`，`scoped(14)` 应静、`scoped(13)` 应报；`MEANS_LAST_CHAPTER == 12` 的用例改 13。临时写回的是测试副本，还原字节相同。
- **战斗背景**（`data/visual/battles.json`，现有名；2026-10-13 grep 该文件尚无 `b13_*` 条目）：① `dock_river`、② `cliff_top`（第 11 章红瑚岛同组）。每条都要指得出读它的那一行代码、有走真实入口的测试（handoff 第 6 节第 5 条）。
- **开场白与文案**（`data/text/ch13_battle.json`）：`ch13.battle.<短名>.intro` / `.charge`；① 开场白写两名银衫修士要他随行验容貌（不抄原著台词），蓄势写大印；② 开场白写霓裳草香与礁外法阵（叫「法阵」，不叫「困妖」）、蓄势写巨虾裹着的黑气。**旧占位 `ch13.battle.waixinghai_lieyao.intro` 在 `data/text/battles.json` 第 8 行**，由协调者删（重复 key 让 `loadGameData` 起不来，第 11、12 章同一个坑）。胜利文案：① 两人俱亡、他收两只储物袋；② 聚魂钵抽魂取丹（A96）。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 追兵 | ch503–504：两名逆星盟结丹初期正道修士借故要他随行验容貌；交手中大印压垮他的青色巨剑，六道青虹被符文护罩挡住（剑无神雷，A26）；他以花篮卷走怪鸟与大印，五行环锁住儒生真元、剑气洞穿；大汉弃宝遁走，五行环再锁、一道青光斩之（ch504）| 一场、一波两敌；「随行 / 不随」两支都打；五行环＝削架势 3；花篮只在胜利文案 | 原著就是他一人对两人、无援手；「锁真元、功法施展不得」落成破势窗口（A25）。花篮（第 12 章 `story_hualan`）不做手段：做成要再登一类，而原著胜负在五行环与剑上 |
| ② 诱妖 | ch523：霓裳草诱来三四只五级妖兽，飞剑即斩；继而六级青色巨虾裹黑气而来，被阵盘黄光罩住、五行环锁死、两道青虹斩成数截；聚魂钵抽魂取丹（ch523）| 两波；可逃、可重复；胜后旁白抽魂取丹，妖丹由编成 `drops` 给 | 第 11 章 ① 是五级千目妖、有曲魂同行；本场独战、首领六级。黄光罩是「法阵」，只在开场白；**可逃、可重复是游戏的让步**——原著七年猎妖（A97）落成循环 ＋ n25 蒙太奇 |
| 逆星盟攻港 | ch501–502：上千人攻南明岛，他隐匿旁观（丘陵上藏的是七八里外另一名结丹中期，他以神识看见，ch501 第 42 段）| 演出（n03）；挂点名 `trigger_qiuling` 只是地点，文案不写他藏在丘陵上 | 他没有出手（A15）|
| 毒蛟与巨龟 | ch520–522：化形雷劫、八级毒蛟与金霞、霍氏交手 | 演出（n21），他远看后遁走 | 八级碰见只能逃（A14、A85）|
| 琉璃兽 | ch524：七道青光合剑，一剑斩首 | 蒙太奇（n25）| 一剑之事；同一招就是 ② 里的蜂云合剑 |
| 鸠面老者三人 | ch525–527：三色甲虫凝矛、颈圈逼回 | 演出（n25），他否认虫魔、放人 | 没有交手（A103）|
| 碧云门一队 | ch527：十万噬金虫灭敌 | 回想（n25）| 七年里的事，判据② |
| 鹰鸢兽 | ch530：傀儡与光幕退兽 | 旁白（n28）| 他隔阵问话、没出手（A117）|
| 黄明礼之围 | ch534：救下黄明礼 | 旁白一招（n30）| 一招（A126）|
| 云天啸 | ch538：十二柄寸许小剑穿透护体火焰 | 旁白一击（n31）| 一击（A137，E 表 25）|

### 8.4 野外遭遇

**本章不放遭遇区**（笔记 D 表）：黑石城除竞斗场外严禁斗法（A52）；雾中岛是他自己布了幻阵的岛（A70、n17）；南明岛与天星城一路戒严、被盘查。外海猎妖原著的收获是妖丹，而遭遇表的掉落只许灵草——做遭遇区只会给玩家一堆打了没妖丹的海兽；猎妖的循环就是 ②。修为的来路是静室（第 7 节），不是遭遇。

---

## 9. 经济账（**数目的权威表**）

**四本账：低阶灵石（`material_lingshi`）、中阶灵石（`material_lingshi_zhong`）、妖丹（`material_yaodan_wuji` / `material_yaodan_liuji`）、诱妖资源（`herb_nichang_cao`、`herb_huangjing_cao`〔数据里显示名「黄精」〕、小瓶绿液）**，另记 story 物品。起点全部是「估」（`tests/fixtures/ch12-end-*.sav` 尚未生成，笔记 C.3）：协调者备注给出第 11 章真档章末低阶 2339 / 2319、中阶 20 / 20，第 12 章账面低阶 −8、中阶 0，**本章起点估低阶约 2331 / 2311、中阶 20 / 20**（比笔记 C.3「数百到四千」窄；仍须拿真档核）。下面只写本章的进出与恒等式，不先宣称终局数。

**来源**

| 项 | 低阶灵石 | 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| 起点（ch12-end，**估**）| 约 2331 / 2311 | 20 / 20 | 0 / 0–1 | 霓裳草余数、小瓶（容量 9）、腰牌、肋骨、炼晶、金丝蚕 0 或 3、养魂木根须、灵乳等（**必须拿真档核**）|
| n05 储物袋 | ＋600 | | | 迷幻砚、白玉大印 |
| n07 / n15 / n16 | | | | 黄色指环 / 上古典籍玉简 / 奇渊海图 |
| n17 立府 | | | | 霓裳草 ＋3；灵田 `field_wuzhong` 6 槽 |
| n18 / n19 | | | | 五色圆珠一匣 / 玄阴经 |
| n24 缺草兜底（草 0 且灵石 < 10；意图玩家碰不到）| | | | 霓裳草 ＋1 / 次 |
| ② 每胜（编成 `drops`）| | | ＋2 / ＋1 | |
| n25 收手 | | | ＋10 / ＋6 | 六七级妖兽材料 1 |
| 丹炉（玩家）| | | | 至元丹 ＋3 / 炉 |
| Z1 / Z2 / Z3 | | ＋30 / ＋15 / — | | Z2 霓裳草 ＋3；Z3 养魂木珠 |
| n31 / n34 | | | | 铜片 / 伴妖草 |
| 卖（玩家，可选）| 明珠轩 `sellRate 0.8`：五级 760、六级 4800；寸金阁 0.5；秘市 0.6（收至元丹、妖丹）| | | 南明岛原料铺 0.5 |

**去向**

| 项 | 低阶灵石 | 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| 南明岛原料铺（可选，`buyRate 0.9`）| 照价 × 0.9（霓裳草 9、回气丹 450）| | | 限量不补货 |
| n12 偷渡 | 第一支 −10000 | | 第二支 −4 / — | 第三支 0（把柄 ＋ 飞刀，旁白）|
| 寸金阁 / 秘市（可选）| 霓裳草 −10、黄精 −5、玛瑙角 −5000 | | | |
| 丹炉每炉（玩家）| | | −1 / — | 黄精 −1 |
| n24 每轮 | | | | 绿液 −1（战前扣，逃也扣）；胜则霓裳草 −1 |
| n19 / n20 / n22 | | | | 肋骨 −1 / 炼晶 −1 / 五色圆珠 −1、金丝蚕 −余数 |
| n29 至元丹 | | | 第二支 — / −1 | 第一支至元丹 −3；第三支 `advance_days(30)` |
| n31 交换会 | 铜片第二支 −5000 | | 铜片第一支 −1 / — | 换她自由：六七级材料 −1（看返回值）|
| Z1 / Z2 / Z3 | | | Z1 — / −1；Z2 −3 / — | Z3 绿液 −3、根须 −1 |
| 往返黑石城（可选）| | | | 每趟 `advance_days(12)`，不花钱 |

**账要对得上的几条**（交测试路，T 前缀；直接读 `scripts/ch13/*.lua` 与 `data/battles/b13_*.json`）：

1. **`give("material_lingshi")` 恰 1 处**（`zhuibing.lua`，600）；**`take("material_lingshi"` 恰 2 处**（`chuansong.lua` 10000、`mishi.lua` 5000），都只在 `item.count` 够时才列出那一支、`take` 看返回值。**没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训）：偷渡、至元丹、铜片三处付出都有零成本一支（纪律 70）。
2. **中阶灵石**：`give` 恰 2 处（`mingzhu.lua` 30、`xuanshang.lua` 15）；`take` 0 处。
3. **妖丹**：脚本 `give` 恰 2 处、都在 `chongmo.lua`（五级 10、六级 6）；`take` 恰 5 处——`chuansong.lua` 五级 4、`gongsun.lua` 六级 1、`mishi.lua` 五级 1、`mingzhu.lua` 六级 1（Z1）、`xuanshang.lua` 五级 3（Z2），每一处都在「够才列」的支里；编成来源只有 ② 的 `drops`。
4. **story**：本章新 story 各 `give` 1 处（迷幻砚、白玉大印、黄色指环、上古典籍玉简、奇渊海图、五色圆珠、玄阴经、六七级妖兽材料、铜片、伴妖草、养魂木珠）；`take` 恰 7 处（肋骨 n19、炼晶 n20、五色圆珠 n22、金丝蚕余数 n22、六七级材料 n31、养魂木根须 Z3、至元丹 3 于 n29 第一支），全部看返回值；`story_miaoyin_yaopai` 0 处 `take`；`story_wannian_lingru` 0 处读写（文案「灵乳」）。
5. **小瓶**：`bottle.spend(1)` 恰 1 处（`youyao.lua`）、`bottle.spend(3)` 恰 1 处（`yanghun.lua`）；缺液的 `advance_days(5)` 不计入意图玩家。
6. **编成**：① `spirit_stones 0`、`drops []`；② `spirit_stones 0`、`drops` 恰六级 1 ＋ 五级 2。
7. **店与配方**：`data/shops/ch13_*` 恰 4 份；脚本 `shop(` 恰 4 处（`caigou.lua`、`mingzhu.lua`、`cunjin.lua`、`mishi.lua`）；新配方恰 1 张（`recipe_zhiyuan_dan`，`requireFlag ch13.liandan`，不写 `guaranteed`）；炼器配方 0；全仓 `guaranteed:true` 仍恰 1 张。
8. **意图玩家章末账**（两侧同，笔记 N.1 第 5 条）：低阶 ＝ 起点 ＋ 600、中阶 ＝ 起点；六级 ＝ 起点 ＋ 2 ＋ 6 − 1 ＝ 起点 ＋ 7；五级 ＝ 起点 ＋ 4 ＋ 10 − 1 ＝ 起点 ＋ 13；霓裳草 ＝ 起点 ＋ 3 − 2 ＝ 起点 ＋ 1；六七级材料 0；`story_wuse_yuanzhu`、`story_xuangu_leigu`、`material_lianjing`、`story_jinsi_can` 都是 0；本章新 story 各 1（迷幻砚、白玉大印、黄色指环、上古典籍玉简、奇渊海图、玄阴经、铜片、伴妖草）。
9. **两侧同账**：通关两侧走同一组选项（笔记 N.1 第 4 条：n12 第三支、② 胜 2 次不逃、0 次往返、0 条可选支线、n29 第二支、n31 铜片第一支 ＋「以整堆材料换」＋ 回绝）；章末 `day` − 起点 `day` ＝ **11790 ＋ 360 × N**，N 是该侧按「闭关一年」的次数，由该侧真档 C0 与冲关种子定（两侧 C0 差约 61 点，通常同 N；不同则各写各的，不抹平）；两侧低阶灵石差 ＝ 起点差、中阶同。
10. **退路**：本章门闸全在 7.1；没有一处会因物品或钱不够而卡死。

**对照：原著这一段的量级**。偷渡是五千一人的黑价（A37），而他一块灵石没花——两颗五级妖丹、一个把柄、两柄死人的飞刀（A44–A46）；玛瑙角一只五千（A64）；七年深海存下数百枚六七级妖丹（A107），本作收成 16 颗。所以本章的钱在**妖丹**不在灵石：意图玩家章末多出的六级 7、五级 13，按明珠轩价折算约 4.3 万低阶，却几乎无处可花（兽潮后黑石城空了，秘市只开一次）——这是他「外海一趟的身家」，留给第 14 章（笔记 O.2）。

<!-- 第 13 章施工图·第三段（第 10–17 节；写手三，2026-10-13）。唯一事实来源：notes/ch13/research.md；原著补核处注明章号（novel.py read 499–542）。 -->

## 10. 引擎前置

### 10.1 清单

**编号对照**（全文只此一套）：**契约 E1–E2** 指研究笔记 D 表末两条（E1 结丹档面板冲关、E2 跨图同一块田；契约文件落地后以契约为准），由引擎路做、判据写进 `tests/Ch13EngineTests.cpp`，**都不改 `src/`**；本表其余项按谁做分前缀：**C** 内容路 / 协调者、**T** 测试路、**B** 平衡路；「不做」的几项不编号。本文别处写「E 表 N」的，一律指研究笔记 E 表（16.4 原表）的拍板编号；写「A N」「B 表 N」的指 1.1 / 1.2 表；写「O-N」的指笔记 O.3 开放问题——都不是本表。

| # | 项 | 必须 / 可选 | 谁做 | 说明 |
| --- | --- | --- | --- | --- |
| 契约 E1 | **结丹档面板冲关**：起点 `realm 31 / cap 31` 时 `breakthroughBlock` 报 `StoryCap`；脚本 `realm.cap(realm.CORE_MID)` 之后修为 < 5000 报 `NotEnough`、≥ 5000 报 `None`，`breakthroughChance` 到线恰 85；冲成后 `realm 32`、`maxHp 900`、`maxMp 620`，再按报 `StoryCap`（上限 32）而不是 `NotEnough`；静室 `effectiveness 150` 坐 360 日的进账与 `meditate(CoreEarly, 50, 150, …)` 同种子逐点相等（**前提：余数账 `cultivationRemainder` 先清零**；不清零时是差分，见契约 1.3 第 6 条）| **必须** | 引擎路 | 判据项，不改代码：规则层与入口齐备，但**结丹档面板冲关从没有一章跑过**（第 8 章筑基档、第 10 章重修八倍）；任一处不符才改 `src/`，且先改研究笔记 |
| 契约 E2 | **跨图同一块田**：存档里已有带作物的 `field_tianxing_39` 时，从 `ch13_tianxing` 的 `facility_yaoyuan_39` 进 `FieldScene`，读到的是原作物（不新建、不清空），采收进背包、年份不丢；`validate.py` 与地图测试对两张图（`ch11_dongfu`、`ch13_tianxing`）挂同一个 `ref_id` 不报错 | **必须** | 引擎路 | 判据项；O-2 已结案（`shenshougu_yaopu` 先例，契约第 0 节），不动 `src/`、`tools/` |
| C1 | **开局接法**：patch `scripts/ch12/lingquan.lua` 末尾——`flag.set("ch12.done")` 之后补一行 `teleport("ch13_nanming")`（只传图 id，落 default spawn `haishang`；此前一个字不动）；`data/chapters.json` 加 `{"number": 13, "numeral_key": "ui.chapter.13.numeral", "title_key": "ui.chapter.13.title", "done_flag": "ch13.done"}`；`ui.json` 加「第十三章」「逆星之乱」 | **必须** | 内容路（一行）＋ 协调者（两份数据）| E 表 1、2；与 9→10、10→11、11→12 先例一致；第 12 章的图一个字不动 |
| T1 | `tests/fixtures/ch12-end-{first,second}.sav` 按 C1 patch 之后重生成：`mapId` 变 `ch13_nanming`、站在 `haishang`（落点不触发 n01，第 13 章通关驱动要先迈一步踩进 `trigger_wenxun` 那条横贯带）；第 12 章验收里「第十二章　终」接「未完待续」的断言改成接「第十三章　逆星之乱」；第 12 章验收 20 与纪律 61 那条扫脚本断言（`lingquan.lua` 不 teleport）改成「`lingquan.lua` 仅末行一处 `teleport("ch13_nanming")`」（落点改动，不算推翻第 12 章拍板，16.1 第 58 条）| **必须** | 测试路 | 照第 12 章 T1 做法；第 2 节标「必须」的各项在这两份上逐项核（O-3）|
| C2 | 规则 25：`CHAPTER_MEANS[13]` 照 8.2「规则 25」段（金：祭金蚨，噬金虫可补；**五行环不登**，16.1 第 46 条）；`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]` ＝ 木（`magic_fengyun_heji`，n20 学）；`MEANS_LAST_CHAPTER = 13`；**selftest 探针挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`**（`outside_role` → `data/roles/feng_xi.json`、`outside_battle` → `b14_fengxi_xiezhi`，先验改「`chapter == 14` 且敌方含 `feng_xi`」，`probe["chapter"] = 13`，措辞改「第 14 章」；`future_id` 同法替换）；`MEANS_LAST_CHAPTER == 12` 的用例与名「末章12」改 13 | **必须** | 协调者（集成时）| E 表 30；不挪则「第 13 章唯一含 `wuji_haishou` 的编成」那条红（② 已写实，它成了范围内的敌人）|
| C3 | `tests/LexiconTests.cpp` 表一补词（12.1：82 个 ＋ 禁词 2）| **必须** | 协调者 | 放在第 12 章集成之后；`{13, 542}` 早在第 441 行 |
| T2 | `tests/ObjectiveTests.cpp` 的 `kScriptTransfers` 登 teleport 清单 7 条（三元 5、五元 2；笔记 N.4，施工图 3.1 末表）；`tests/NpcPresenceTests.cpp` 的 `kOffChainFlags` 登第 11 节链外 7 面 | **必须** | 测试路 | O-11：#5（`lidao.lua`）与五元两条都带坐标、落在非 spawn 格 |
| T4 | 照第 12 章 T4 顺延一章的四处：`UiStyleTests` 的 `ChapterTableData` / `ChapterCards`（表长 13；「`ch13.done` 不排卡」的断言改成 `ch14.done`）、`PathActionTests` 的 `realmCapOfChapter()` 加 `{13, Realm::CoreMid}`、`NpcPresenceTests` 的 `kLastChapter` 改 13、`tests/ChapterFixture.h` 加 `ch13-end` 两份 | **必须** | 测试路 | 本章上限止于 32（1.3）|
| C4 | 数据：角色新建 30、就地改写 1（`wuji_haishou` 补「剑」，B 表 25）、一个字不动 2（`fan_furen`、`wen_siyue`）；编成新建 1（①）＋ 就地改写 1（② `b13_waixinghai_lieyao`，B 表 24）；法术新建 3；物品新建 13；配方 1（`recipe_zhiyuan_dan`）、炼器配方 0；商店 4；任务 5；路径行动 13；地图 4（旧图 0 张修补）| **必须** | 内容路 | 清单在第 4、5、6、8、9 节与 16.3；物品描述受第 12 节约束 |
| C5 | `data/visual/maps.json` 四张图（只用现有组合：`dock/day/dock_river`、`town/dusk/dock_river`、`valley/day/herb_valley`）、`data/visual/battles.json` 两场背景（① `dock_river`、② `cliff_top`）；开场白、蓄势、胜利文案写 `data/text/ch13_battle.json`；**删占位**：`data/text/battles.json` 里旧的 `ch13.battle.waixinghai_lieyao.intro`（若在）归协调者，与 `ch13_battle.json` 同一批落 | **必须** | 内容路 ＋ 协调者（占位）| 重复 key 让 `loadGameData` 起不来（第 11、12 章同一个坑）|
| C6 | `data/flags.json` 登第 11 节 41 面（不含 `ch13.path.*`）| **必须** | 协调者 | 施工方不碰 |
| C7 | 回写：大纲（含第 5 节地图上限「65–75」→「80–85」、支线目标改为实际可达值，16.1 第 3、51 条）、lore、人物总表（B15 只加注、不改「政敌」）、第 12 章契约第 3 节与笔记 O.2 的「境界」「金丝蚕」两行、第 12 章施工图验收 20 与纪律 61（16.1 第 58 条）、`weapon_qingzhu_fengyun_jian` 的 note（笔记 O.4 全部）| **必须** | 协调者 | B 表 1–32 ＋ O-6、O-7 |
| C8 | 门禁放行「同一 `ref_id` 跨图」| **不做** | — | O-2 已结案：门禁本来不查 `ref_id` 跨图，`shenshougu_yaopu` 同挂两图一向是绿的（契约第 0 节 E2，16.1 第 50 条）|
| B1 | **量法标定**：先跑 `BattleHand` 自检（同一起点、同一手法连跑多次逐字段一致），再用 `tests/BattleHand.h` 量两种：「结丹初期单人对两名同阶、无友军」（①）与「结丹初期单人对五级两只 ＋ 六级首领」（②）；起点为意图玩家两侧与**最低合法起点**（中阶灵石 0、无丹药、每场之前在最近的打坐设施静养到满）；② 的 `rewards.cultivation` 定在 ≤ 60（O-10）；`recipe_zhiyuan_dan` 的 `difficulty` 定到意图玩家熟练度下成功率 ≥ 50%；`Realm.cpp` 锚点一个不动 | **必须** | 平衡路（内容路收尾之后串行接手）| 1.3；结丹中期锚点留第 14 章标定（O-9）|
| T3 | 通关测试 `Ch13Walkthrough`：起点 T1 两份、终点写 `tests/fixtures/ch13-end-{first,second}.sav`（v9，不加存档字段）；`Ch13TriggerModeTests` / `Ch13BattleDataTests` / `Ch13LedgerTests` / `Ch13SliceTests`（**不含 `Ch13EngineTests`**，归引擎路）| **必须** | 测试路（**另一个代理**）| 第 15 节 |
| 不做 | 拍卖 / 竞价面板 | 可选 | — | **不做**：竞宝厅、交换会只看、只做选项（E 表 25）|
| 不做 | 战时物价曲线（`buyRate` 按旗标切换）| 可选 | — | **不做**：原著无涨价（B 表 8）；按地点分店（E 表 14）|
| 不做 | Lua 改 `aptitude`（五色圆珠加资质）| 可选 | — | **不做**：是 `src/` 项，收益只有一成打坐（E 表 38）|
| 不做 | 重炼七十二剑做成面板配方 | 可选 | — | **不做**：走脚本 n20，`guaranteed` 全仓仍一张（E 表 8）|
| 不做 | 云天啸一击 / 黄明礼救援升格为仗 | 可选 | — | **不做**：原著都是一招了事；预留 id `b13_mishi_yuntianxiao`（O-1）|
| — | 遭遇区、队伍（`party.*`）、换形诀数值、天都尸火、剑上辟邪神雷、困敌阵做机制 | **不做** | — | E 表 37、9；A26、A79 |

**查过、本章不需要新做的**：`realm.cap` 只升不降、看返回值；`realm.at_least`；`breakthroughBlock` / `tryBreakthrough`（面板冲关唯一入口）、失败倒扣两成、走火四成；facility `meditate` 的 `effectiveness`、`field` 的 `ref_id`、`alchemy` 的 `grade`、`shop`；`field.unlock`；脚本 `shop(id)`；配方 `requireFlag`、`successChance` 夹 [5, 95]、`guaranteed` 全仓一张；`effect "stagger"`（第 7 章先例）；多波次、`can_escape true` 时 `battle()` 返回逃走；`charge{every,all,text_key}` 与 `actions`；`killable_by` 非空（加载器与规则 24 只拒空数组；**⊆ `weaknesses` 两处都不查**，靠第 15 节验收 4）；诱妖口径（`item.count`、`bottle.drops`、`bottle.spend(1)`、`advance_days(3)`，第 11 章 `youyao.lua`）；结丹后小瓶三日一滴、容量 9；`teleport(map_id[, x, y])`（不认 spawn 名，(0, 0) 当没给坐标）；`advance_days` 单次 ≤ 3650（本章最大 3600）；NPC `visible_flag / hidden_flag`（portal / facility 不读）；`choice` 的选项由脚本按条件拼、取消为 nil；路径行动 `when / until`；章节卡与「未完待续」；存档 v9。

### 10.2 判据的要点

- **契约 E1**（以契约为准，这里只是摘要）：夹具 `tests/fixtures/ch11-end-first.sav`（`realm 31 / cap 31`，判据不依赖章节进度；修为由用例直接写）。四态：上限 31 → `StoryCap`；探针脚本 `realm.cap` 返回 true 后，修为 4999 → `NotEnough`、5000 → `None` 且把握 85；同一颗种子冲成 → `realm 32`、`maxHp == rules::realmMaxHp(32)`、`maxMp == rules::realmMaxMp(32)`、再按 → `StoryCap`；冲败 → 修为倒扣两成（走火四成）、`realm` 不变。静室 150 坐 360 日与规则层 `meditate` 同种子逐点相等（余数账清零时；契约 1.3 第 6 条）。**负向**：去掉 `realm.cap` 那一步 → 面板永远 `StoryCap`；设施去掉 `effectiveness` → 进账回到 100 那条，用例红。
- **契约 E2**（以契约为准）：同一夹具，用例先确保 `field_tianxing_39` 至少一槽有作物（真档若空，用例自己种一株、记下年份）；从 `ch13_tianxing` 的设施进 → 槽位、作物、年份与存档逐字段相同；采收 → 背包多一株、`herbAge` 不丢；**负向**：把设施 `ref_id` 改成新 id → 得到空田（证明上一条读的是旧田而非新建）。真数据跑 `validate.py`：两张图同一 `ref_id` 零报错（先例 `shenshougu_yaopu`；C8 不做）。
- **C1**：第 12 章集成版 `lingquan.lua` 与 patch 后的 diff 只有末尾一行；headless `Application` 演完它，`mapId == ch13_nanming`、站在 `haishang`、`ch12.done == 1`、章节卡队列「第十二章　终」接「第十三章　逆星之乱」；演完 `banyao.lua` 则是「第十三章　终」接「未完待续」。**负向**：把那一行挪到 `flag.set` 之前 → 第 12 章章末断言红。
- **C2**：`validate.py` 对两张编成「至少一样破绽是本章我方手段打得到的」全过（① 金 / 剑、② 剑 / 木 / 金）；探针用例把 `feng_xi.json` 临时写坏，`probe["chapter"]` 为 13 时不报、改 14 时报，还原后字节相同。**负向**：`MEANS_LAST_CHAPTER` 留 12 → `b13_*` 两张全红。
- **T2**：`kScriptTransfers` 7 条逐条跑脚本到那一行，`mapId` 等于落点图；五元两条的触发器 `type trigger / mode enter / once false / guard_flag == "ch13.lifu" / set_flag` 空、`qiyuan_hang.lua` 与 `hui_wuzhong.lua` 各 0 处 `flag.set`；`kOffChainFlags` 7 面都不在 `data/objectives/ch13.json` 的 `done_flag` 序列里、却都在 `data/flags.json` 里，窗口照第 11 节末表。
- **B1**：两侧意图起点——① 五行环削势 3 能把儒生打进破势、大汉的白玉大印蓄势要么被破势打断要么防御硬接得住、儒生的怪鸟只压法力；② 第 0 波剑与土省法、第 1 波巨虾 `actions 2` 下蓄势能被五行环或剑打断、**只有剑与木要得了它的命**。最低合法起点——两场都打得过。数值改动落在内容路的 role / battle 文件里，测量记录写进编成的 `note`，由协调者并入第 18 节。

---

## 11. 旗标清单（交协调者登记；施工方不碰 `data/flags.json`）

**「谁置」一律是脚本自己 `flag.set`**：引擎从不替触发器写 `set_flag`，只拿它判 once 兑现了没有；下表「（＝触发器 set_flag）」只说明那个触发器的 `set_flag` 字段填的就是这面旗。挂 NPC 的节点没有 once，脚本开头看 done_flag 判已演过。整数旗标用 `flag.set(name, n)`，开关旗标 `flag.set(name)`。

| 旗标 | 值 | 谁置 | 谁读 |
| --- | --- | --- | --- |
| `ch13.wenxun` | 1 | n01 `wenxun.lua` 末尾（＝ `trigger_wenxun` 的 set_flag）| n02 前置；`npc_sanxian_laozhe`、`npc_yuanliao_zhanggui` visible；K1 `when` |
| `ch13.caigou` | 1 | n02 `caigou.lua`（NPC）| n03 guard；`q13_qianyelu` accept |
| `ch13.gongdao` | 1 | n03 `gongdao.lua`（＝触发器 set_flag）| n04 前置；苍云龙、刘护法、辛明 visible；三仙宗老者 hidden；掌柜脚本「封市」分支；K1 `until`；K2、K3 `when` |
| `ch13.panzha` | 1 | n04 `panzha.lua`（NPC）| n05 guard；苍云龙、刘护法、辛明 hidden；K2、K3 `until` |
| `ch13.zhuibing` | 1 | n05 `zhuibing.lua`（＝触发器 set_flag；① 胜后）| 凌玉灵 visible；n06 前置 |
| `ch13.yinjian` | 1 | n06 `yinjian.lua`（NPC；置旗与 teleport 顶格）| n07 guard；凌玉灵 hidden；赵、于护法 visible；K5、K6 `when`；`kScriptTransfers` |
| `ch13.rucheng` | 1 | n07 `rucheng.lua`（＝触发器 set_flag）| n08 guard；`facility_yaoyuan_39`、`facility_jingshi_39` 的 require_flag；K6 `when` |
| `ch13.qifu` | 1 | n08 `qifu.lua`（＝触发器 set_flag）| n09 guard |
| `ch13.tanzhen` | 1 | n09 `tanzhen.lua`（＝触发器 set_flag）| n10 guard |
| `ch13.qianru` | 1 | n10 `qianru.lua`（＝触发器 set_flag）| n11 guard |
| `ch13.yirong` | 1 | n11 `yirong.lua`（＝触发器 set_flag）| 传送大厅四人 visible；n12 前置；K4、K7 `when` |
| `ch13.chuansong` | 1 | n12 `chuansong.lua`（NPC；置旗与 teleport 顶格）| n13 guard；天星城六个 NPC hidden；黑石城五个 NPC visible；K4–K7 `until`；K8–K11 `when`；`kScriptTransfers` |
| `ch13.daoqiyuan` | 1 | n13 `daoqiyuan.lua`（＝触发器 set_flag）| n14 前置（`mingzhu.lua` 开头判）；K9 `when`；`q13_haishou_xuanshang` accept |
| `ch13.mingzhu` | 1 | n14 `mingzhu.lua`（NPC）| n15 前置（`cunjin.lua` 开头判）；竞宝厅老者 visible；K10 `when`；`q13_mingzhu_dingdan` accept |
| `ch13.cunjin` | 1 | n15 `cunjin.lua`（NPC）| n16 guard；K11 `when`；`q13_qianyelu` s1 |
| `ch13.chucheng` | 1 | n16 `chucheng.lua`（置旗与 teleport 顶格）| n17 guard；`kScriptTransfers` |
| `ch13.lifu` | 1 | n17 `lifu.lua`（＝触发器 set_flag）| n18 guard；雾中岛三设施 require_flag；往返五元两条 guard（＝ doneFlag）；`q13_yanghun_zhu` accept |
| `ch13.chongshi` | 1 | n18 `chongshi.lua` | n19 guard |
| `ch13.xuanyin` | 1 | n19 `xuanyin.lua` | n20 guard |
| `ch13.chonglian` | 1 | n20 `chonglian.lua` | n21 guard |
| `ch13.dujie` | 1 | n21 `dujie.lua` | n22 guard |
| `ch13.wusezhu` | 1 | n22 `wusezhu.lua` | n23 guard |
| `ch13.buzhen` | 1 | n23 `buzhen.lua` | `trigger_youyao`、`trigger_shoushou` guard；Z3 可交 |
| `ch13.lieyao` | 计数（诱妖胜场）| n24 `youyao.lua` 每胜一次 `flag.set("ch13.lieyao", flag.get("ch13.lieyao") + 1)`；逃与败不写 | n25 判 `>= 2`；目标链按 `>= 1` 记一步 |
| `ch13.chongmo` | 1 | n25 `chongmo.lua`（过 `lieyao >= 2` 之后）| n26 guard |
| `ch13.liandan` | 1 | n26 `liandan.lua` | n27 guard；`recipe_zhiyuan_dan.requireFlag` |
| `ch13.biguan` | 1 | n27 `biguan.lua` | n28 guard；`qiyuan_hang.lua`「闭关在即」分支；`youyao.lua` 开头门闸；Z1、Z2 fail |
| `ch13.chuguan` | 1 | n28 `chuguan.lua`（过 `realm.at_least` 门闸之后）| 青灵门二人 visible；n29 前置；K12 `when`；`q13_gongsun_qiuyao` accept |
| `ch13.gongsun` | 1 | n29 `gongsun.lua`（NPC）| n30 guard；青灵门二人 hidden；K12 `until`；`q13_gongsun_qiuyao` complete |
| `ch13.lidao` | 1 | n30 `lidao.lua`（置旗与 teleport 顶格）| `trigger_mishi` guard；黄明礼 visible；`hui_wuzhong.lua` 分支；K13 `when`；Z3 fail；`kScriptTransfers` |
| `ch13.jiaohuan` | 1 | n31 `mishi.lua` | 黄明礼 hidden；文思月 visible；n32 前置；K13 `until` |
| `ch13.wenshi` | 1 | n32 `wenshi.lua`（NPC）| `trigger_jiaoshi` guard；文思月 hidden；`q13_qianyelu` s2 |
| `ch13.jianshi` | 1 | n33 `jianshi.lua` | `trigger_dongkou` guard |
| `ch13.done` | 1 | n34 `banyao.lua` 末尾 | 章节卡「第十三章　终」与「未完待续」；全部路径行动 `until`；`q13_qianyelu` complete；第 14 章入口 |
| `ch13.shouchao`（链外）| 1 | n29 `gongsun.lua`（`ch13.chuguan` 之后、`ch13.gongsun` 之前）| 黑石城六个 NPC hidden；`trigger_xuanshang` 分支；`trigger_feixu` guard；K8–K11 `until` |
| `ch13.toudu`（链外）| 1 灵石 / 2 妖丹 / 3 把柄与飞刀 | n12 `chuansong.lua` | n13 一句（易敬看他的眼神）|
| `ch13.zhiyuan`（链外）| 1 自炼丹 / 2 六级妖丹化开 / 3 开炉一月 | n29 `gongsun.lua` | n29 后段一句 |
| `ch13.tongpian`（链外）| 1 五级妖丹 / 2 灵石 / 3 以材料抵 | n31 `mishi.lua` | n33 一句 |
| `ch13.z1`（链外）| 1 | Z1（`mingzhu.lua` 分支）| `q13_mingzhu_dingdan` complete |
| `ch13.z2`（链外）| 1 | Z2 `xuanshang.lua` | `q13_haishou_xuanshang` complete |
| `ch13.z3`（链外）| 1 | Z3 `yanghun.lua` | `q13_yanghun_zhu` complete |
| `ch13.path.*` | | 路径行动 | 只由路径行动置（13 面，16.3；**等 `data/pathactions/ch13.json` 落地后登记**）|

链上 34 ＋ 链外 7 ＝ **41**；链外 7 面登 `kOffChainFlags`（T2），窗口如下（`{flag, after, before, script}`；`offChainProblems` 要两步在目标链上紧挨着，同一脚本里也置了窗口两头的，源码次序要对得上；16.1 第 54 条）：

| flag | after | before | script | 源码次序要求 |
| --- | --- | --- | --- | --- |
| `ch13.toudu` | `ch13.yirong` | `ch13.chuansong` | `scripts/ch13/chuansong.lua` | `flag.set("ch13.toudu", n)` 写在顶格 `flag.set("ch13.chuansong")` 之前 |
| `ch13.z2` | `ch13.daoqiyuan` | `ch13.mingzhu` | `scripts/ch13/xuanshang.lua` | 无（该脚本不置链上旗）|
| `ch13.z1` | `ch13.mingzhu` | `ch13.cunjin` | `scripts/ch13/mingzhu.lua` | **首谈分支（置 `ch13.mingzhu`）写在再谈 / Z1 分支之前**；反过来就是第 11 章 `ch11.jiguan` 那个坑（`NpcPresenceTests.cpp` 第 124–127 行）|
| `ch13.z3` | `ch13.buzhen` | `ch13.lieyao` | `scripts/ch13/yanghun.lua` | 无 |
| `ch13.shouchao` | `ch13.chuguan` | `ch13.gongsun` | `scripts/ch13/gongsun.lua` | 先于 `flag.set("ch13.gongsun")` |
| `ch13.zhiyuan` | `ch13.chuguan` | `ch13.gongsun` | `scripts/ch13/gongsun.lua` | 先于 `flag.set("ch13.gongsun")` |
| `ch13.tongpian` | `ch13.lidao` | `ch13.jiaohuan` | `scripts/ch13/mishi.lua` | 先于 `flag.set("ch13.jiaohuan")` |

Z 三面插在 12.2 编号所用的那一步（Z1 `mingzhu`、Z2 `daoqiyuan`、Z3 `buzhen`），与第 11 章 `ch11.z1` 插在接单步之后同法；七面里只有 `shouchao` 是在场旗标，其余六面是全量登记，不牵动覆盖先验。不新增 `story.*`。登记说明：`ch13.lieyao` 写「计数，诱妖胜场；只由 `youyao.lua` 写」；`toudu / zhiyuan / tongpian` 写明三个值各指哪一支。**done_flag 与脚本名不同的只有三处**：`ch13.lieyao` → `youyao`、`ch13.jiaohuan` → `mishi`、`ch13.done` → `banyao`（12.2 的场景编号用）。

---

## 12. 禁词与首见章号

### 12.1 交协调者：`tests/LexiconTests.cpp` 表一要补的词

笔记 M 表逐个 `novel.py find` 取最早命中章（2026-10-13）。**已在表里的不重登**：逆星盟、万三谷 500，南明岛 501，凌玉灵 505，奇渊岛 511，黑石城 512，明珠轩 513，雾中岛 517，裂风兽 538，外星海、妖海 389，玄阴经、聚魂钵 330，天都尸火 406，六道极圣 435，星空殿、传送符 393，妙音门 399，范夫人 400，文思月 430，补天丹 435，虚天鼎 434，乾蓝冰珠 494；第 12 章 M 表的（集成后在表）金丝蚕 435、啼魂兽 437、铁火蚁 462、炼晶 466、五色铜环 469、五行环 492、万年灵乳 497、养魂木 497、血色披风 499、玄阴环 456。2026-10-13 扫过 `data/text/*.json`：下表 82 词 ＋ 两个禁词在现有文案里只命中「奇虫榜」3 处（`ch08_main.json` 2、`ch10_haiwai.json` 1；首见 345 ≤ 第 8 章覆盖的 345，合法），其余 0 命中——补表不会让前章变红。

| 首见 | 词（82 个） |
| --- | --- |
| ch499–505（17）| 三仙宗（499）；天星令、逆星令（500）；千叶露、玛瑙角、玛瑙兽（501）；苍云龙、王长老、辛明、开天门、白水剑（502）；刘护法、七煞决（503）；迷幻砚、白玉大印（504）；外事执法、天香索（505）|
| ch506–517（21）| 于护法、赵护法（506）；黄色指环（507）；流水阵、刘夫人（508）；易敬、许云（511）；竞斗场（512）；明夫人、鉴宝厅、竞宝厅、妖兽海渊、蓝光石（513）；紫云鹰（514）；联合大拍卖、寸金阁、离寻、古玉（515）；伴妖草（516）；吞海鱼、银翼鸟（517）|
| ch518–529（16）| 幻焰蛾、五色圆珠（518）；化形雷劫（520）；毒蛟、金霞、霍氏（521）；妖修、伪洞府（522）；巨虾（523）；琉璃兽、三色藻、四法上人（524）；虫魔、碧云门、妙鹤真人（525）；三色噬金虫（527）|
| ch530–541（25）| 青灵门、鹰鸢兽、兽潮（530）；公孙杏、青灵玄功（531）；至元丹（532）；金真功、缠玉决（533）；换形诀、黄明礼、南黎岛、秘市（534）；听风阁、交换会（535）；云天啸、盘黎蚓、卧虎鲨、化海兽、天灵子（536）；青夫人、灵骨（537）；白骨阴火（538）；幻梦石（539）；厉先生（540，「厉」单字不登）；三头六臂（541）|
| 早于 499（3）| 圣祖（427）；玄阴大法（407）；奇虫榜（345；表里若已有则不重登）|
| **禁词（2）** | **梵圣真片（546）**：本章物品名用「铜片」；**困妖（1038）**：诱妖的阵只叫「法阵」「阵」 |

**故意不进表的**（泛称或单字，进表会误伤）：曲、厉（化名单字）、少主、海渊、雾海、灵乳、铜片、肋骨、巨猿傀儡、银衫、绿袍、红霞大阵。它们由 12.2 按场景管。

### 12.2 规则放行、本章另有专项断言的

**断言的写法**同第 10–12 章：把 3.4 目标链的次序当场景次序，给每个文案 key 的场景前缀（`ch13.<场景>.*`，场景名 ＝ 节点脚本名）编号；「X 之前不含」＝编号小于 X 的前缀下 0 处。往返两份（`qiyuan_hang.*`、`hui_wuzhong.*`）编在 `lifu`；Z1 编在 `mingzhu`、Z2（`xuanshang.*`）编在 `daoqiyuan`、Z3（`yanghun.*`）编在 `buzhen`；`feixu.*` 编在 `gongsun`（`shouchao` 在那一步置）；路径行动文案按它 `when` 里最晚的链上旗标编号；物品描述按 `give` 它的脚本编号；战斗开场白、蓄势与胜利文案按调用它的脚本编号（① `zhuibing`、② `youyao`）；地图名按第一次 teleport 进去的脚本编号（`ch13_nanming` 编在第 12 章 `lingquan`，即本章第 0 号；`ch13_tianxing` → `yinjian`、`ch13_waihai` → `chuansong`、`ch13_wuzhong` → `chucheng`）；目标链 R-3 提示按它所属那一步的上一步编号。**名牌跟 role 走、不算文案 key**：NPC 上图到他自报之间名牌已是本名的几处，见 16.1 第 44 条。

| 词 | 首见 | 为什么规则管不住 | 本章的断言 |
| --- | --- | --- | --- |
| 千叶露 | 501 | 章内先后；他进原料铺前就带着这个名字（ch501 第 14 段），第 2 步提示编在 `wenxun` | `wenxun.*` 之前不含（即第 2 步提示起合法，16.1 第 55 条）|
| 玛瑙角、玛瑙兽 | 501 | 章内先后 | `caigou.*` 之前不含 |
| 苍云龙、王长老、辛明、开天门、白水剑、少主、圣祖 | 502 / 427 | 章内先后 | `gongdao.*` 之前不含（封岛通告与辛明出示信物都在攻岛之后）|
| 七煞决、刘护法（作文案）、厌瑙 | 503 / 511 | **他不在场时出口的名字**（ch503 苍、刘二人私话时他已飞走；ch511 顾、乾二人灭口时他已传走）| **全章文案 0 处**；`liu_hufa` 用不含姓的描述名牌（16.1 第 44 条）|
| 迷幻砚 | 504 | 只在儒生自己心里叫出 | ① 的开场白、蓄势、胜利文案 0 处；物品名自 `zhuibing` 的 `give` 起 |
| 辟邪神雷 | 424 | 规则放行；剑上此时无神雷（A26）| `xuanyin.*` 之前不含（ch519 试鼎那句「刚恢复的一丝」起）；① 全部文案 0 处 |
| 外事执法、天香索 | 505 | 章内先后 | `yinjian.*` 之前不含 |
| 赵护法、于护法、定颜丹、黄色指环 | 506–507（定颜丹全书首见 155）| 章内先后 | `rucheng.*` 之前不含 |
| 流水阵、刘夫人、易兄 | 508 | 章内先后 | `qianru.*` 之前不含 |
| 曲道友、曲兄、曲某（化名）| 509（化名义；同名他人 ch376 已有）| 化名只在他人口中 | `yirong.*` 之前不含；`cunjin.*` 之后 0 处（ch512–516 许云、易敬、离寻仍称他「曲」，E 表 34 写的「n11–n12」偏窄，本表以原著为准放到 `cunjin`）|
| 奇渊（碑名）| 510 | 章内先后 | `chuansong.*` 之前不含 |
| 易敬、许云 | 511 | 章内先后 | 对白与旁白 `daoqiyuan.*` 之前不含（n10–n12 只称「易兄」「易道友」「锦衣大汉」）|
| 黑石城、奇渊岛、竞斗场、联合大拍卖 | 511–515 | 章内先后 | `daoqiyuan.*` 之前不含（K9 是许云自己说的，编在 `daoqiyuan`）|
| 明夫人、鉴宝厅、竞宝厅、蓝光石、紫云鹰、妖兽海渊、寸金阁 | 513–515 | 章内先后 | `mingzhu.*` 之前不含 |
| 离寻、古玉、伴妖草 | 515–516 | 章内先后 | `cunjin.*` 之前不含（之前只叫「千叶露」）|
| 雾中岛、雾海、吞海鱼、银翼鸟 | 517 | 章内先后 | `chucheng.*` 之前不含 |
| 幻焰蛾、五色圆珠 | 518 | 章内先后 | `chongshi.*` 之前不含（n18 之前那一匣只叫「白玉匣」）|
| 玄阴经、玄阴大法、天都尸火 | 330 / 407 / 406 | 章内先后 | `xuanyin.*` 之前不含（之前只叫「肋骨」）|
| 化形雷劫、毒蛟、金霞、霍氏、伪洞府 | 520–522 | 章内先后 | `dujie.*` 之前不含 |
| 妖修 | 522 | ch522 那一处是作者旁白 | `jianshi.*` 之前不含（ch541 他自己判铜片是妖修功法）；「妖修之战」全章 0 处 |
| 巨虾 | 523 | 章内先后 | `buzhen.*` 之前不含；② 开场白在第 1 波现身之前不点名 |
| 琉璃兽、三色藻、四法上人、虫魔、碧云门、三色噬金虫 | 524–527 | 章内先后 | `chongmo.*` 之前不含（n22 孵出的新虫只写样子、不叫「三色」）|
| 妙鹤真人、妙鹤 | 525 | 只闻其名 | 只许出现在 `chongmo.*`、`mishi.*`（ch539 范夫人提起）、`wenshi.*` |
| 万三谷、六道极圣 | 500 | 只闻其名 | 只许出现在 `wenxun.*` |
| 至元丹 | 532 | 配方名 | `liandan.*` 之前不含（配方自 n26 列在丹炉上，E 表 20）|
| 青灵门、鹰鸢兽、兽潮 | 530 | 章内先后 | `chuguan.*` 之前不含 |
| 公孙杏、青灵玄功、金真功、缠玉决 | 531–533 | 章内先后 | `gongsun.*` 之前不含 |
| 换形诀、黄明礼、南黎岛、秘市、听风阁、厉（作称呼）| 534–535 | 章内先后 | `lidao.*` 之前不含 |
| 交换会、云天啸、盘黎蚓、卧虎鲨、化海兽、天灵子、青夫人、灵骨、白骨阴火、幻梦石、裂风兽 | 535–539 | 章内先后 | `mishi.*` 之前不含 |
| 厉先生 | 540 | 章内先后 | `wenshi.*` 之前不含 |
| 三头六臂 | 541 | 章内先后 | `jianshi.*` 之前不含 |
| 风希、风某、风雷翅、碧焰酒、梵圣真片、温天仁、银月、紫罗极火、磷火潭、九级、化形第二阶 | 542–731（「九级」作泛称 516、521 已有，这里禁的是章末那人的「九级」）| 第 14 章及以后 | **全章 0 处**（章末只写人声，E 表 28）|
| 极阴、齐姓、漩矶、玄阴岛 | 382 / 527–528 | 暗场（判据③，B 表 11、26）| **全章 0 处** |
| 云啸天、历道友、历某 | 538 | 原著笔误（ch538 云天啸作「云啸天」、厉作「历」）| **全章 0 处**（ch538 两处为本稿补核）|
| 黄道友 | 508 / 535 | ch508 一处笔误（张作「黄」）；ch535 起韩立称黄明礼「黄道友」是正当称呼 | `lidao.*` 之前不含（张姓那几段 0 处，B 表 9）；`lidao.*` 起只许指黄明礼（16.1 第 56 条）|
| 万年灵液、只出不进、雾阵、六眼巨虾、物价、灭族、丧夫、炉鼎、双修 | — | 摘要造词 / 改编尺度（B 表 6、8、16、22、30，1.1 末段）| **全章 0 处** |

---

## 13. 文案要求

与前十二章同一标准：对 ch499–542 最长公共片段 ≤ 6 字、对全本 ≤ 7 字、≥ 10 字 0 条（**汉字剥离、逐条精确最长公共片段、带正负对照**）；换词不算重写；key 命名 `ch13.<场景>.<用途>`；台词 18–60 字为主、不超 80，选项 5–20 字。笔记 A 表的「原文片段」只作出处定位，**一条都不进文案**。

**转写高风险点**：

1. **n01 老者那三段话**（天星令、逆星盟与两魁首、外星岛战况）：条目是事实，**讲法与次序全换**——做成韩立一句句问出来的；老者赌咒发誓、赔笑那一套不用。
2. **n03–n04 封岛与盘查**：通告的内容（两个时辰、出示信物或功法即放）是事实，通告的原话、众修面面相觑、辛明出示信物那一句不用；苍云龙「认得一切信物」只写结果。
3. **n05 追兵**：「追剿残党、验容貌」是由头；「随行」那一支**不许照搬原著「假意应下、随即出手」那一手的写法**，两支的开打各自写。
4. **n07 于护法**：两条路与传送阵的新条件是事实；他那段「只讲一遍」的口吻与「先前不算」的原句不用。
5. **n09–n12 偷渡**：开价的数字是事实，两人开价、还价的原话不用；拦路那一番威胁（「我一喊星宫就来」）意思保留、说法自写；石碑「奇渊」二字可用（碑名）。
6. **n13–n15 黑石城**：许云的导览是条目——黑石城格局、众势力共管、严禁斗法、元婴坐镇、海渊、兽卵——**讲法全换**，不按他边走边指的次序。城里的风月一律不写（13.1 第 13、14 段）。
7. **n21 巨龟与毒蛟**：形象全部自出；**不写「日后妖修之战的导火索」**——那是原著作者在旁白里告诉读者的，原文明说他并不知道、很快抛到脑后（ch522 第 14 段；16.1 第 42 条）。
8. **n25 虫魔**：只写三人当面对他说的与他自己的猜测；三人现身前的私语、极阴与齐姓道士、假虫魔杀人——0 处（判据③）。
9. **n29 公孙杏（改编尺度最严）**：她在原著是十五六岁的少女（ch529）。「入阵为仆」是唯一的约；双修、炉鼎、处子、稚女、夜里召她、摸她头发、她独处时的惶恐——**一个字都不许写到**；留书的意思（孝心可嘉、余物相赠、勿示人以免招祸）保留，原文四句不用。
10. **n31 交换会**：东主、范夫人、十三位结丹、铜片、灵骨、他只换巢穴、文思月开口、白骨阴火、十二柄小剑一击——事实；在座女修作陪只写一句「各人身边都有年轻女修陪坐」，**他与文思月之间的身体接触 0 处**。
11. **名场面台词不许复刻**：老者那句「据实相告」；苍云龙的封岛通告；壮汉说少主的那句；大汉「你不能走」；儒生临死那句；于护法「这个不算」；瘦削汉子开五千的那句；他自称「姓曲、一介散修」；「我一喊星宫就来」；乾姓「一次最多七人」那段；看护「能出不能进」那句；许云「形神被灭」那句；离寻「东主之谊」；他喝破鸠面三人那句；孙师叔说兽潮那段；公孙杏的誓言；黄明礼呼救；「我姓厉」；云天啸自报东主；文思月提条件那句；「血洗了这里」；「叫我厉先生」；章末那句人声——**意思保留，说法自己写**；3.2、13.1 里为说明意图写的转述同样**不许照抄进文案**。
12. **死亡只写倒下**：① 两名追兵（不写洞穿、腰斩）、巨虾（不写斩成数截与绿液）、琉璃兽（不写斩首与碧血）、鹰鸢兽（不写洞穿）；云天啸**重伤未死**，只写他倒地起不来。
13. **韩立不知道的不写**：苍、刘私话（煞气、七煞决、画像、对紫灵的图谋）、顾乾灭口与厌瑙、寸金阁画中白衣人、鸠面三人的私语、极阴与齐姓道士、公孙杏独自在洞府里看到的东西、交换会之后范夫人的布置——情报、旁白、路径行动、物品描述都不许写到。
14. **章末停在人声**：`ch13.banyao.*` 的最后一条是身后那个男子的声音；不写他的样子、名字、境界，不写韩立回身之后的任何事；「未完待续」是章节卡的事，文案里 0 处。

### 13.1 每段框架件清单（写手与复验共用的判据）

**判据**（沿用第 5 章 13.1）：**剧情事实**＝删了戏就塌的东西（事实、硬约束、谁在场、落点），下表逐段列，动作与旗标以 3.2 为准；专名不算质感。表外的一切——人数、岁数、长相、手势、道具、次要人物的反应、次要的一句话、谁先谁后——**一律自出**。**质感边界**一栏列的是最容易被搬进来的原著质感项（只点名、不复述）：不许出现，换词也不行，反着写也算留着；它不是全部。复验：逐段做「原著质感项 → 现文」的并排表，剩下的对应项必须全在剧情事实里。

| 段 | 原著 | 剧情事实（删了就塌）| 质感边界（不许出现；只点名不复述）| 在场 | 落点 |
| --- | --- | --- | --- | --- | --- |
| 1 问讯 | 499–500 | ①海上拦下一队三仙宗修士问话，他以妙音门客卿长老自居；②天星令（觐见，否则剿灭）、逆星盟（正魔联手，两魁首只闻其名）、逆星令（响应觐见即为敌）、外星岛已被攻；③逆星令的鬼面与当年六连殿长老所示同源——他推断渗透已久；④大战利他藏鼎、但进城盘查必严；原本就打算回城弃府、经传送阵去外星海；⑤（**改编**：原著 499–503 无此文字，ch504 铜环才再出场）他把五色铜环祭成可用之宝——为 ① 铺手段；⑥以灵乳续力、披风赶路半月 | 老者赌咒与赔笑；雪笛仙子与紫灵的交情；他哑然失笑；复制令牌图样的那枚玉简；「抛尸荒野、全体灭门」那串；他对着海面踌躇、仰看天色；结丹修士误认他是元婴、纷纷避让 | 韩立、三仙宗老者 | `ch13.wenxun` |
| 2 采购 | 501 | ①港口小城的坊市货全、比天星城略便宜；②原料铺掌柜不识千叶露；玛瑙角是玛瑙兽的独角、要去拍卖行碰；③掌柜提示：多半是上古叫法，得查典籍；④他补齐外海要用的东西 | 掌柜「干了百余年」的自夸；他手指敲椅柄；掌柜送到门口才想起的那一下；三道结丹神识扫他、一道跟了很久 | 韩立、原料铺掌柜 | `ch13.caigou` |
| 3 攻岛 | 501–502 | ①傍晚逆星盟上千修士攻港，红霞大阵挡住；②他隐身、神识放出——观战者百余、结丹五六人；③灰白邪气加入、大阵撕开、星宫修士四散；④银衫的拆阵布阵、绿袍的搜岛——南明岛成了前哨；⑤苍云龙通告：两个时辰后再离岛 | 霞光与轰鸣的写法；阵的残片回光返照般乱飞；七八里外丘陵上那名结丹中期；他关于元婴多困在初期的那段念头；「训练有素」的感慨与他自嘲散修 | 韩立、苍云龙（通告）| `ch13.gongdao` |
| 4 盘查 | 502–503 | ①苍云龙带三名结丹回来：出示信物或功法即放；②辛明先出示白水剑、获放；③他出示妙音门客卿长老腰牌（无腰牌则报名号）；④苍云龙问起韩、曲两位长老，托话给紫灵；⑤壮汉插话：少主是圣祖唯一传人；⑥飞离时一道元婴神识扫过——他早料到，所以从不易容（易容瞒不过神识强的人）| **他走后苍、刘二人的私话整段**（煞气、七煞决、画像、对紫灵的图谋——0 处，16.1 第 41 条）；老者认得一切信物那段称奇；辛明的相貌与白光小剑；苍云龙脸上那丝古怪 | 韩立、苍云龙、刘护法、辛明 | `ch13.panzha` |
| 5 追兵 | 503–504 | ①出港半日，两名银衫结丹初期追上；②借「追剿星宫残党、验你容貌」要他随行；③（两支都打）战斗①：五行环锁住一人真元；另一人弃宝而逃，也被锁住；④收两人储物袋、大印与砚台；⑤点破海里藏着的人 | 「你不能走」；他展颜应下随即出手；六道青光幻成十二道、六道合成的巨剑被大印压断；砚台的墨香与半尺长的红光怪鸟；花篮卷鸟；儒生临死那句；洞穿、腰斩 | 韩立、白面儒生、黑壮大汉 | `ch13.zhuibing`；① |
| 6 引荐 | 505 | ①海里藏着的是星宫外事执法凌玉灵，谢救命；②他辨不出对方男女（至多一句）；③对方点出他身怀古宝、邀入星宫，他以妙音门之恩婉拒；④天星城已戒严，对方愿引他进城 | 五官与额带；喉结；媚笑与眼波；他想起田不缺；黄脸修士献丹、对方转嗔为喜那一场 | 韩立、凌玉灵 | `ch13.yinjian` |
| 7 入城 | 505–506 | ①赵护法验腰牌与洞府令牌、随行女修对画像；②城门白衣修士中唯一坐着的是结丹中期的于护法；③于护法问他驻颜，他答服过定颜丹；④两条路：协助星宫 / 安分待在住处——戒严时段外执法队格杀勿论；⑤传送阵改了条件：完成任务或在大战中当众击杀同阶，先前所杀不算；⑥黄色指环为凭 | 暗哨的波数；城墙的霞光；竹椅、山羊胡、闭目养神、有气无力；「凌道友是男是女」的问答；于护法嘴角的讥讽；**「这个不算」那一句** | 韩立、赵护法、于护法 | `ch13.rucheng` |
| 8 弃府 | 507 | ①街上罕有人影、凡人闭户，坊市全关；②圣山十几道神识扫到指环即退（其中有结丹后期）；③三十九层洞府：噬金虫装袋，药园在旁；④住了近百年，弃府；⑤两点顾虑：鼎的消息、星宫赖账——先去探阵 | 「一个鬼影也没有」；他在禁制外呆站长叹；躺床上昏昏睡去 | 韩立 | `ch13.qifu` |
| 9 探阵 | 507 | ①次日白天有小半日自由；②神识探星空殿外殿，遇青黄两色禁制即撤；③偷听两名筑基：三千不行、五千一人，结丹给多少都不放；④瘦削那人说大战或延数年乃至数十年 | 两人的相貌；「守财奴」的比方；**两句开价的原话** | 韩立、张姓修士（及同伴，不建 role）| `ch13.tanzhen` |
| 10 潜入 | 508、510 | ①一缕神识缠上张姓，跟到第四层宅院（院外小流水阵）；②隐匿入院、神识罩住厢房：七人议事——易拍板付灵石、不够的先借、后天必走；③次日尾随张去星空殿偏门，出来时面带喜色、进门前换成愁容——他起疑；④当夜迷魂，问出张与守阵二人合谋抬价 | 竹林与院落样式；锦衣大汉的鹰钩鼻；妇人那句「黄道友带话」（含笔误）；夫妇与刘夫人拌嘴；他摸鼻子、托下巴；「半睡半醒」的写法 | 韩立、易敬（易兄）、刘夫人、张姓修士 | `ch13.qianru` |
| 11 易容 | 509 | ①第三日清晨刚解戒严；②他易容青衫、压成筑基后期假丹，自称姓曲；③在四十九层拦下七人、点破易的姓；④真动手他一喊星宫就来——易咬牙应下引荐；⑤众人把他夹在队中前行 | 青衫人的病容；易的手势与按住储物袋的青筋；**「到时曲某大喊一声」那句**；黑脸修士的敌意 | 韩立、易敬、张姓修士 | `ch13.yirong` |
| 12 传送 | 509–511 | ①殿内禁制已关；顾、乾两名结丹点人——多一人；②阵一次最多七人、只开一座；③付费三支（E 表 12）；④张被把柄逼着让出名额；⑤白玉牌撤阵禁；⑥阵前石碑刻「奇渊」 | 秃顶老者倨傲点头；**乾姓「一次最多七人」那段原话**；玉匣里的红光与两把飞刀（第三支只一句旁白）；「侥幸」的心声与假符的盘算；**他走后顾乾灭口、厌瑙那一段（0 处）** | 韩立、顾姓老者、乾姓修士、易敬、张姓修士 | `ch13.chuansong` |
| 13 奇渊 | 511–512 | ①破旧石屋，看护：如今能出不能进、传送符被收；②许云来此数年、正想回内海；易敬以答换情报；③出屋：二十余里的黑石小城、石墙外光罩、清一色平房，凡人与修士杂处；④奇渊岛众势力共管、星宫也不愿得罪；除竞斗场外严禁斗法、违者形神俱灭；时刻一位元婴坐镇 | 许云要回去给儿子炼丹的沮丧；**街上艳装的凡人女子、搂着凡女走路的男修、门口迎客的妖艳女子（0 处）**；「资料是伪造的」那段 | 韩立、看护修士、许云、易敬 | `ch13.daoqiyuan` |
| 14 明珠轩 | 513–515 | ①明珠轩收购海外所得、可鉴定后参拍；②明夫人的媚术，他不为所动；③海渊：深不可测、高阶妖兽频出，修士也死得多；④竞宝厅观拍：蓝光石、兽卵——兽卵过不了传送阵，所以这里便宜；⑤（抉择）看下去 / 离开 | **粉红屏风与飞天仕女**；明夫人的嗓音与「少一位主顾」；标牌、排队按玉牌进小屋；拍卖报价那串数字；**炉鼎拍卖整段（0 处，B 表 30）** | 韩立、明夫人、竞宝厅老者、许云、易敬 | `ch13.mingzhu` |
| 15 寸金阁 | 515–516 | ①凡人须登记、无人雇用即离城（一句）；②他与易敬等人分手；③放出一丝结丹修为、被请进贵宾室；④管事离寻（结丹初期）片刻就查出他今日才到；⑤古玉的上古典籍：千叶露即八级妖兽旁伴妖草之汁；⑥他以一瓶丹药、一张符宝换下玉简；⑦玛瑙角店里有；⑧离寻劝他结伴出海，他婉拒 | 他一眼寒芒吓出许、易一身冷汗；清秀少年引路；传讯符在离寻手上烧起；古玉的长髯与「出尘」；黄色小剑符宝；**画中走出的白衣人与离寻的私语（他不在场，0 处）** | 韩立、离寻、古玉 | `ch13.cunjin` |
| 16 出城 | 517 | ①杂货铺买海图与妖兽出没情报；②海渊在南、修士云集——他不去；③北飞十余日修士渐少，再转西两日 | 危险标记之多；海渊「纵横千万里」 | 韩立 | `ch13.chucheng` |
| 17 立府 | 517 | ①西边第四座无人灵脉岛；②他是追一只四级银翼鸟误入雾海；③雾由岛底成千上万的吞海鱼吐出；④灵脉在长十余里、高三四十丈的小山脉；⑤半日开府，幻阵把山脉变成一片林子；⑥六七日炼十几套阵旗阵盘、埋遍全岛放雾 | 荒地、树林、草原的排比；手指粗的孔洞与银鱼进出；「名字好笑」；「破财消灾」 | 韩立 | `ch13.lifu` |
| 18 虫室 | 517–518 | ①古玉玉简里一幅图：五色光团与十来颗小珠——补天丹？（只是他的猜想）；②白玉匣里的五色圆珠；③虫室开五间（两批噬金虫、血玉蜘蛛、啼魂兽、金丝蚕）；④有金丝蚕时：奇虫榜第十七的幻焰蛾一段，挑一雌一雄各喂一粒；⑤九曲灵参本体移入药园禁地、白兔归体；⑥新绿液全给养魂木 | 血凝五行丹的联想；「半死不活」的蚕与饲灵丸；隔着禁法窗口探头；白光从匣里窜进灵参 | 韩立 | `ch13.chongshi` |
| 19 玄阴经 | 518–519 | ①肋骨不是骨；②聚魂钵残阴灌入，发黑、嗡鸣、放黑芒，神识得入——是一枚玉简；③前半玄阴大法（不修：已修青元剑诀），后半秘术默记两三种；④天都尸火须玄阴大法为基、元婴以上；他看出它的弱点；⑤试鼎半月无果（连剑上刚恢复的一丝神雷也试过）；乾蓝冰珠不碰 | 「斗大的古文」；他想着下次让极阴吃一惊的冷笑；火烧水淹的试法清单 | 韩立 | `ch13.xuanyin` |
| 20 重炼 | 519–520 | ①决意把熔岩路上那块炼晶炼进七十二柄飞剑；②原估大半年，实炼近一年，以灵乳续力；③洞府杂务由巨猿傀儡打理 | 「这一界」的夸口；中途法力不继的后果那段 | 韩立 | `ch13.chonglian` |
| 21 化形之劫 | 520–522 | ①地动，黄旗定住洞府；②岛的小半被巨浪淹成沼泽；③雾海外近千丈的巨龟渡化形雷劫；④百丈毒蛟破浪而出、化作人形；⑤金霞道士（元婴初期）与霍氏兄弟（结丹后期）赶到，三人一妖交手；⑥他披血色披风遁走、不往雾海方向；荒岛躲数日；⑦回看：海底有龟尾与半截拐杖，他扔回海里；⑧改主意不迁府：加幻阵、布两个伪洞府 | **巨龟与毒蛟的形貌**；万里雷云、银色雷球；毒蛟瞥向他藏身处那一眼；道士的拂尘、八卦袍、背刀；霍氏的僵尸脸、红葫芦、鬼头拐杖、破天神砂；「骏马嘶鸣」；**「妖修之战导火索」那句（0 处）** | 韩立 | `ch13.dujie` |
| 22 五色珠 | 522 | ①半月炼困敌布阵器具；②噬金虫卵带斑，搬进控神法阵滴血认主；③丹火两月炼一颗五色圆珠，年许六颗炼尽；④金丝蚕喂霓裳草后产卵上百、全是死卵，那一对也毙命；⑤新噬金虫孵出、样子有变；留小半守府 | 珠子入口变软、微苦、丹田发暖；「冷水浇头」「祸福相依」；**卵上的黑斑、新虫壳上的银斑与黑纹**（B 表 32 只记「黑斑」「银斑」，ch522 原文孵出的虫是黑纹——两样都算质感，16.2 第 17 条）| 韩立 | `ch13.wusezhu` |
| 23 布阵 | 522–523 | ①养魂木两年已成形（雕珠归 Z3）；②不去深渊（修士云集，不宜放绿液）；③西飞一月，避开近处珊瑚岛，在一片火红珊瑚礁布四阵、草移阵心 | 「天雷竹」的类比 | 韩立 | `ch13.buzhen` |
| 24 诱妖 | 523 | ①前三次展叶引来三四只五级，飞剑斩之；②第四叶、诱妖之气最浓；③漩涡、黑气裹着一只六级扑来；④阵起光罩困住、五行环锁住——是六只碧眼的青色巨虾；⑤剑斩；⑥聚魂钵抽魂、取淡蓝内丹 | 「三百年草错引六七级」的旧事；黑云风雨与十来丈的漩涡；铁青巨钳捣罩；绿色汁液；切下两只虾螯、火球化灰 | 韩立、五级海兽、青色巨虾 | `ch13.lieyao`；② |
| 25 虫魔 | 524–527 | ①「七年后」；两年前深渊妖兽暴动，四法上人只剩元婴逃生；②琉璃兽被困，七道青光合剑、一剑斩落；③三名结丹窥伺；他喝破，三人分头逃，三色甲虫凝矛追回、套上颈圈；④三人当他是「虫魔」：七八名结丹被灭、一人逃脱，碧云门折了近三分之一；⑤他否认、解圈放人；假虫魔驱的是金银两色飞虫；⑥回想：放十万噬金虫灭了碧云门一队，躲进深海数年、存下数百枚六七级妖丹；⑦猜是碧云门栽赃，或虚天殿老怪逼他现身；⑧决意回府、二三十年不出 | 三人在他现身前的私语；琉璃兽的乳白长丝与晶球；剑「模糊一下再飞出一把」；**他喝破三人的那句**；切下琉璃兽皮；「强者才有说话的权利」那段感慨；**极阴与齐姓道士的密谈、假虫魔杀人那一幕（0 处）** | 韩立、鸠面老者、闵姓修士、宣姓修士 | `ch13.chongmo` |
| 26 炼丹 | 528–529 | ①半月回府，洞府无恙，整理妖丹与材料；②炼丹到了瓶颈，先拿五级妖丹练手；③一闭三年 | 「近乎炼丹大师」的自评；鼎盖自行挪开 | 韩立 | `ch13.liandan` |
| 27 闭关 | 529 | ①歇一月；②霓裳草对三色噬金虫失效——用一只少一只，只好喂未进化的那批；③服龙眼大小的红光丹药、诵第八层剑诀 | 「枯燥无味」；四季轮转与洞府积尘 | 韩立 | `ch13.biguan` |
| 28 出关 | 529–530 | ①十五六年破结丹中期瓶颈、修成第八层（面板冲成）；②续修第九层，又二十余年；③某日清晨五人逃进雾海——鹰鸢兽追来；④幻阵、光幕、巨猿傀儡的光柱灭鸟；⑤他隔阵问话：来者是青灵门弟子（筑基初期的孙师叔带四名炼气）| 清晨海鸟；少女的长相；鹰鸢兽的赤冠与青光；「洞穿无数空洞」；光柱的粗细 | 韩立、孙师叔、公孙杏 | `ch13.chuguan` |
| 29 公孙杏 | 530–533 | ①兽潮：二十多年前海渊妖兽涌出，黑石城成废墟、两位元婴陨落、外海村镇被扫荡；青灵门唯一的结丹长老死于兽潮；②公孙杏为瘫父求药（父为她强修青灵玄功第六层），他一语点破拔苗助长；③她跪求、愿终身为奴婢——**改编：他只要她「入阵为仆」**（E 表 21）；④至元丹三粒（三支兜底）由孙师叔带回；⑤两日后他出关，以青年面目见她、问附近哪里能买卖，她给海图玉简；⑥他留书赠宝（缠玉决玉简、法器、丹药）而去；⑦动机：孝心，像小妹出嫁前的影子 | **双修、炉鼎、处子、稚女、夜召、摸头发、她坐在床沿发怕、脸红——一概 0 处**；她独自翻看玉简与法宝那一段（他在闭关）；粗壮青年的不忿；**留书那四句原文**；他想起未在父母跟前尽孝那段 | 韩立、公孙杏、孙师叔 | `ch13.gongsun` |
| 30 离岛 | 533–534 | ①近后期遇瓶颈，出去找伴妖草；②结丹中期起能抽土石精气养灵参，带灵参走、弃府；③途中施换形诀（法力至多七成）；④救下被几只三四阶妖兽追的黄明礼（一招）；⑤南黎岛坊市已毁，黄明礼引他去秘市 | 「贼眉鼠目」；他对人性的叹息；黄明礼肢解妖兽尸体 | 韩立、黄明礼 | `ch13.lidao` |
| 31 秘市 | 535–539 | ①他自称姓厉；②乱石堆敲壁、守门老者、地下两层；③一二层都无八级伴妖草；④三日后白衣女子来请——他认出文思月（她认不出他）；⑤东主云天啸（结丹中期），范夫人与他亲昵同来；十三位结丹；⑥铜片古文像兽皮书；有人拿出八级灵骨；⑦他用一堆六七级材料只换八级巢穴的消息；⑧文思月开口：知道一处巢穴，条件是带她走（侍妾一说到 n32 作罢）；⑨云天啸燃白骨阴火、罚她面壁；⑩他出手：十二柄寸许小剑穿透护体火焰，云天啸重伤；⑪铜片三支必得（E 表 27），材料作换她自由的代价；⑫范夫人邀他为回内海的传送阵出力（缺幻梦石），他回绝；带她离开 | **女修偎依、他揽腰、手落在她胸前那一段（0 处）**；胖老者搂人与亮宝名目；红巾怪人的竹竿身形；青夫人那袋灵石；**「血洗了这里」那句**；刺向云天啸的「白色精芒」；众修以为他是元婴老怪 | 韩立、黄明礼、守门老者、文思月、云天啸、范夫人、孙姓修士 | `ch13.jiaohuan` |
| 32 文思月 | 539–540 | ①两日后一座灵气微薄的荒岛，他开小洞府安置她；②「厉先生」；③妙音门惊变：少门主去虚天殿后，范左使与外来魔道夺权、迁到外星海，卓右使不知所踪；④他推断叛乱在他困于虚天殿时，妙鹤或有牵连；⑤侍妾之说他作罢，只要巢穴；⑥她警告巢穴已近深渊边缘 | 她「花容绽开」与他心起涟漪；他揉鼻子；「女色圈套」那段顾虑 | 韩立、文思月 | `ch13.wenshi` |
| 33 监视 | 541 | ①研铜片：背面三头六臂的怪物，注入法力投出光影，人形妖兽摆了三十六个姿势——妖修的功法，他不修、自觉上当；②服五色珠后资质近三灵根（一句）；③留丹药与法器给闭关的她，独自离开；④一月到深渊边缘那座小岛；⑤礁上小幻阵、敛息蹲守数月，霓裳草引不动八级 | 独角鳞身的形貌；「哭笑不得」；一路灭妖 | 韩立 | `ch13.jianshi` |
| 34 伴妖草 | 541–542 | ①凌晨敛气、从半山巨石后的洞口入洞；②百丈洞窟、淡绿壁光、热气腾腾的碧蓝深潭；③潭边几株乌黑小草——八级附近的伴妖草是黑的；④玉盒收草；⑤正要走，身后一个男子声音 | **那句人声的原话**；说话者的样子（银冠、麻鞋、碧眼、尖鼻）；他吞冰珠、祭剑、灵兽袋被夺；潭底与珍珠通道（全是第 14 章）| 韩立、（那个声音，不建 role）| `ch13.done` |
| Z3 养魂木珠 | 523 | ①两年绿液催出成形之木，过了万年再不生长；②雕成一串乌黑圆珠挂颈 | 「天雷竹」的类比；珠子的大小与金丝 | 韩立 | `ch13.z3` |

Z1、Z2 是改编（原著无对应场），框架件只有第 6 节那两行，其余一律自出。**名牌**：本章没有一处换名牌；化名「曲」「厉」只在他人对白里，韩立名牌不变（E 表 34）。

---

## 14. 时长估算

按幕（笔记 N.2），目标 40 min：

| 桶 | 幕 | 项 | 估 |
| --- | --- | --- | --- |
| A | 一·南明岛 | n01–n06：问讯、采购、攻岛（一处抉择）、盘查、① 约 3 min、引荐；`advance_days` 合 18 | 7 min |
| A | 二·天星城 | n07–n12：入城、弃府、探阵、潜入、易容、传送（付费三支）；合 3 日 | 8 min |
| A | 三·奇渊岛 | n13–n16：黑石城、明珠轩观拍、寸金阁、出城；合 14 日 | 5 min |
| A | 四·雾中岛 | n17–n30：立府与洞府五段、化形之劫、② 两轮约 5 min、虫魔、炼丹、闭关面板约 1 min、出关、公孙杏、离岛；合 11569 日 ＋ 玩家打坐 M | 15 min |
| A | 五·秘市 | n31–n34：秘市与交换会、文思月、监视、伴妖草；合 186 日 | 5 min |
| **A** | | | **≈ 40 min**（意图玩家日历 11790 ＋ M 日，M ＝ 360 × N，N 以 8.1 为准（真档前上界约 6）；`tools/audit.py` 落地后重算）|
| C | 可选 | Z1–Z3、路径行动 13 条、往返黑石城、② 多打几轮、四家店、旧药园自采 | 4–6 min（估）|

**真要压，先压这三处**（都不动 13.1 的剧情事实）：**n10 潜入**（院中议事、尾随偏门、夜里迷魂三段并成两段——议事只留「易拍板、后天走」两句，迷魂只留问出的结果）；**n25 虫魔**（深渊暴动与四法上人并成一句，碧云门一队与深海数年的回想并成一段）；**n31 交换会**（众修亮宝那一串压成一句，只留铜片与灵骨两样）。玩家多打几轮 ②、在静室一年一年地按——那是玩家自己挑的节奏，记在 C 桶。

---

## 15. 验收

每一条都配一条**不经过驱动、直接读数据、判据写死成本文原文**的断言；每一条新检查都要配「故意写坏 → 确实被抓住」的用例。**切片一律写明起点存档**；没写的，起点是 `tests/fixtures/ch12-end-first.sav`（**T1 重生成版**），由切片自己推到所需旗标（只置目标链上该步之前的旗标、按 3.2 补齐该步之前应得的物品）。

| # | 验收条目 | 直接读的断言 |
| --- | --- | --- |
| 1 | 四道门禁全过（validate / selftest / mapgen / artgen），零新增警告 | 门禁本身 |
| 2 | **一条自动化通关测试**，起点 `tests/fixtures/ch12-end-{first,second}.sav`（T1 重生成的两份），落在 `ch13.done`；终点写 `ch13-end-{first,second}.sav`（v9）| `SetUp` 先验 fixture：`ch12.done == 1`、`realm == realmCap == 31`、`formerRealm == 22`、`party` 空、`learnedMagics` 恰第 12 章终点那 12 门（第 10 章 11 门 ＋ `magic_shijin_chongqun`）、`story_miaoyin_yaopai == 1`、`story_xuangu_leigu == 1`、`material_lianjing == 1`、`weapon_qingzhu_fengyun_jian == 1`、`mapId == ch13_nanming` 且站在 `haishang`；`day`、`hp / mp`、`cultivation`、两种灵石、两种妖丹、`story_jinsi_can` 按真档写死（第 2 节标「必须」的各项，O-3）；**意图玩家写死**（两侧同一组选项，第 9 节）。终点先验：`ch13.done == 1`、`realm == realmCap == 32`、`formerRealm == 22`、`maxHp == rules::realmMaxHp(32)`、`maxMp == rules::realmMaxMp(32)`、`learnedMagics` 恰起点 12 门 ＋ `magic_ji_wuxinghuan`、`magic_fengyun_heji`、`party` 空、`ch13.lieyao == 2`、`ch13.toudu == 3`、`ch13.zhiyuan == 2`、`ch13.tongpian == 1`；第 9 节章末账逐项（低阶 ＝ 起点 ＋ 600、中阶 ＝ 起点、六级妖丹 ＝ 起点 ＋ 7、五级妖丹 ＝ 起点 ＋ 13、霓裳草 ＝ 起点 ＋ 1、本章新 story 八件各 1，`story_wuse_yuanzhu`、`story_xuangu_leigu`、`material_lianjing`、`story_jinsi_can`、`story_liuqi_cailiao` 各 0）；**`day` − 起点 `day` == 11790 ＋ 360 × N**——N 是每侧写死的**字面量**（先拿真档 C0 与固定冲关种子算定、写进测试；两侧不同就各写各的），并断言 N ≤ 6（8.1 上界）；驱动自己数的按键次数只拿来与这个字面量对照，不进断言右边；`mapId == ch13_waihai` 且 `position` 等于通关驱动按 interact 的那一格（洞口格位由地图路落图时定，字面量写进测试）|
| 3 | 挂点 `mode` / `once` / guard / set_flag 与 3.1 表一致 | `Ch13TriggerModeTests` 读四张新图逐条对表：`once=true` 的每一个都有 `set_flag`（＝ done_flag）；NPC 挂点（n02、n04、n06、n12、n14、n15、n29、n32）无 once、脚本开头判 done_flag；`trigger_youyao` once 否、无 `set_flag`；`trigger_fangshi_bimen`、`trigger_xuanshang`、`trigger_yanghun` once 否、无旗标；`trigger_feixu` once 否、无 `set_flag`、guard `ch13.shouchao`；无 `auto`；同格 0 处两个对象；**没有一个 enter 触发器覆盖任何 spawn 格**；`trigger_wenxun`、`trigger_rucheng`、`trigger_shiwu`、`trigger_lifu` 是各自 spawn 前方的横贯带（从 spawn 出发不踩它就走不到该图其它任何挂点）；`trigger_hui_wuzhong` 与 `trigger_chucheng` 隔 ≥ 3 格、`trigger_qiyuan_hang` 与 `trigger_lidao` 隔 ≥ 2 格；#5、#6 两个带坐标的落点格上无任何对象，#6 不在 `trigger_chucheng` 的带上；`trigger_dizhen` 与 `trigger_chuguan` 不同行；五处 `facility_cunji_*` 与落点相邻、不与触发器同格 |
| 4 | 两张编成与第 8 节一致 | `Ch13BattleDataTests`：① 一波、`nixing_rusheng` 与 `nixing_dahan` 各 1、`killable_by` 恰 `["金","剑"]` / `["木","剑"]`、大汉 `charge.every == 3` 且 `all == false`、无友军、`can_escape false`、`drops` 空、`rewards.cultivation > 0`；② 两波（第 0 波 `wuji_haishou` ×2、第 1 波 `qingse_juxia` ×1）、巨虾 `killable_by` 恰 `["木","剑"]`、`actions == 2`、`charge.every == 3` 且 `all == true`、`can_escape true`、`drops` 恰六级妖丹 1 ＋ 五级妖丹 2（`rate 100`）、`0 < rewards.cultivation <= 60`；两张 `hero_absent false`、`defeat_is_fatal true`、`spirit_stones == 0`、`killable_by ⊆ weaknesses`、无空数组；`wuji_haishou.weaknesses` 含「土」「剑」且全仓只 ② 一张编成含它；`magic_ji_wuxinghuan` 金、`effect stagger`、`stagger 3`；**最低合法起点两场都打得过**（B1 实测，记在施工偏差）|
| 5 | **境界**：一处上限、玩家自冲、出关门闸 | 扫 `scripts/ch13/`：`realm.cap` 恰 1 处（`biguan.lua`，看返回值）、`realm.advance` / `realm.demote` 0 处；`chuguan.lua` 第一条可执行语句是 `realm.at_least(realm.CORE_MID)` 的判断；**切片**（推到 `ch13.biguan`、修为写成 4000）：踩 `trigger_chuguan` → 一句、`ch13.chuguan == 0`、`day` 不变、挂点还响；**真实入口**（同一切片、修为写成 5000、固定一颗冲得成的种子）：`openFacility("facility_jingshi_wz")` → `CultivationScene` 冲关 → `realm 32`、面板下一关报 `StoryCap`；再踩 → `ch13.chuguan == 1`（契约 E1 的真实入口版）|
| 6 | **跨图旧田** | 切片（推到 `ch13.rucheng`）：从 `facility_yaoyuan_39` 进去是 `field_tianxing_39` 原样、采收进包、年份不丢；`ch13.rucheng` 未置时 `facility_yaoyuan_39`、`facility_jingshi_39` 拦人：栈顶不是 `Field` / `Cultivation`、播的是 `ui.facility.locked`；读地图：两处设施 0 个 `deny_text_key`、`data/text/ch13*.json` 0 个 `ch13.block.*` key（契约 E2 的真实入口版，16.1 第 45 条）|
| 7 | **偷渡三支** | `chuansong.lua`：支一 `take("material_lingshi", 10000)`、支二 `take("material_yaodan_wuji", 4)` 都在 `if` 里看返回值，支三 0 处 `take`；选项只列付得起的；**切片**（推到 `ch13.yirong`）：灵石 0、五级 0 → 只列支三、`ch13.toudu == 3`、背包不变；灵石 10000、五级 4 → 三支都列，选支一 → 恰扣 10000、`toudu == 1` |
| 8 | **诱妖循环与收手门槛** | `youyao.lua`：`ch13.biguan` 已置 → 一句、`day` 不变；草 0、灵石 ≥ 10 → 一句、`day` 不变；**草 0、灵石 0** → 一句、`day` 增 3.3 那一数、霓裳草 1（兜底）；草 ≥ 1 且 `bottle.drops == 0` → `advance_days(5)` 后 `return`；胜 → 草 −1、`lieyao` ＋1、`drops` 进包；逃 → 草与 `lieyao` 都不变；切片（推到 `ch13.buzhen`）逐支；`ch13.lieyao == 1` 时踩 `trigger_shoushou` → 一句、`ch13.chongmo == 0`；`== 2` → `give` 六级 6、五级 10、`story_liuqi_cailiao` 1、`day + 2415` |
| 9 | **重炼、玄阴经、五色珠、金丝蚕** | `take("material_lianjing", 1)`、`take("story_xuangu_leigu", 1)`、`take("story_wuse_yuanzhu", 1)` 各恰 1 处、都看返回值；`magic.learn("magic_ji_wuxinghuan")`（`wenxun.lua`）与 `magic.learn("magic_fengyun_heji")`（`chonglian.lua`）各恰 1 处、看返回值；`story_jinsi_can` 只在 `wusezhu.lua` 按余数 `take`；**切片**（起点删炼晶、推到 `ch13.xuanyin`）：n20 只换一句、照样学会、`ch13.chonglian == 1`；起点 `story_jinsi_can == 0` → n18 无幻焰蛾那一段的 key、n22 0 次 `take` |
| 10 | **至元丹兜底** | `recipe_zhiyuan_dan`：`requireFlag == "ch13.liandan"`、`productCount 3`、`guaranteed` 不写；`gongsun.lua` 三支按「丹 ≥ 3 → 六级 ≥ 1 → 开炉一月」判；**切片**三份（推到 `ch13.chuguan`）：丹 3 → 扣 3、`zhiyuan == 1`；丹 0、六级 1 → 扣 1、`== 2`；全无 → `day + 30`、`== 3`；三份都 `ch13.gongsun == 1` |
| 11 | **铜片必得** | `mishi.lua` 三支（五级 1 / 灵石 5000 / 以材料抵），三支之后都 `give("story_tongpian", 1)`；`take("story_liuqi_cailiao", 1)` 看返回值；**切片**（推到 `ch13.lidao`、五级 0、灵石 0）：只列支三、`story_tongpian == 1`、`ch13.tongpian == 3` |
| 12 | **兽潮只经人口、店随人撤** | `flag.set("ch13.shouchao")` 全仓恰 1 处（`gongsun.lua`）；黑石城六个 NPC 的 `hidden_flag == "ch13.shouchao"`；四张新图上 `shop` 类 facility 0 个，四家店各只被一个脚本 `shop()`；**切片**（推到 `ch13.gongsun`）：黑石城六个 NPC 都不在、`trigger_xuanshang` 只一句、`trigger_feixu` 响一句且不拦路 |
| 13 | **往返黑石城** | 两个触发器属性见 10.2 T2；两份脚本各恰 1 处 `advance_days(12)`、0 处 `flag.set`；**切片**：`ch13.lifu` 置、选「去」→ `day + 12`、到对岸那一格；`ch13.biguan` 置后踩 `trigger_qiyuan_hang` → 一句、`mapId` 不变；`ch13.lidao` 置后踩 `trigger_hui_wuzhong` → 一句、`mapId` 不变 |
| 14 | **经济账**（第 9 节）| `Ch13LedgerTests`：四家店 `buyRate / sellRate` 与条目照第 9 节；两场 `spirit_stones 0`；`take("material_lingshi"…)` 只在 `chuansong.lua`、`mishi.lua`；两侧走同一组选项，章末两侧低阶差、中阶差都等于起点差 |
| 15 | **必打 2 场绕不过去** | `battle(` 全章只在 `zhuibing.lua`、`youyao.lua`；`ch13.zhuibing` 是 n06 前置；`ch13.chongmo` 只在 `lieyao >= 2` 之后置；通关两侧各记打到的编成恰是这 2 个（② 2 次）；遭遇区 0 处 |
| 16 | **无门、只经脚本** | 四张新图 `portal` 0 个；第 12 章及以前的图 0 处修补；`kScriptTransfers` 7 条（T2）|
| 17 | **NPC 在场链** | 第 5 节每个对象「前一段的 hidden 恰是后一段的 visible」；同图可能同时在场的名牌两两不同；`yi_jing` 两个对象分属 `ch13_tianxing` / `ch13_waihai`；`npc_paimaishi` 出场于 `ch13.mingzhu`（规则 30 ＋ `NpcPresenceTests` ＋ 读地图）|
| 18 | **路径行动** | 13 条字面量与 16.3 相符；全 `inquire`、`realm 0`、无 `reveal / give / set_flags`；每条 `when` 含该对象 visible_flag、`until` 含 `ch13.done` 与该对象 hidden_flag；八个节点 NPC 挂点 0 条（`PathActionTests` ＋ 读数据）|
| 19 | **支线 5 条** | 谓词与第 6 节字面量相符；必经两条的 complete 在目标链上；可选三条的 fail 是 `ch13.biguan`（Z1、Z2）/ `ch13.lidao`（Z3）（读 `data/quests/q13_*.json`）|
| 20 | **章末** | `banyao.lua` 最后一条可执行语句是 `flag.set("ch13.done")`，`give("story_banyao_cao", 1)` 在它之前，0 处 `ending()` / `teleport`；headless `Application` 演完：章节卡队列「第十三章　终」接「未完待续」（扫脚本 ＋ 真实入口）|
| 21 | **开局 patch** | 10.2 C1 全部（扫脚本 ＋ 真实入口 ＋ 复验看 diff）|
| 22 | **禁词** | 表一含 12.1 的 82 词与两个禁词；12.2 的章内先后按 key 前缀逐条断言；分母先验 > 250 条（`LexiconTests` ＋ 专项）|
| 23 | **暗场与尺度** | 13 第 9、10、12、13 条逐 key 核；12.2 末四行的词 0 处（独立校对 ＋ 12.2 专项）|
| 24 | 独立校对 | 1.1 的 43 条（对应笔记 A 表 148 行）逐条核验；13.1 逐段并排；原创性人工复核（13 第 11 条那二十三处）|
| 25 | 主线节点 34、主动抉择 15、新地图 4、必打 2 | `tools/audit.py` |
| 26 | **引擎** | 契约 E1、E2 的判据全部落地（`tests/Ch13EngineTests.cpp`）；`tests/fixtures/*.sav` 与 `saves/*.sav` 在 v9 下全部读得进来 |
| 27 | **NPC 节点前置与两处确认** | 切片（推到 `ch13.chuansong`、`daoqiyuan` 未置）：与 `npc_ming_furen` 对话 → 一句、`ch13.mingzhu == 0`；与 `npc_li_xun` 对话 → 一句、`ch13.cunjin == 0`、背包无 `story_guyu_yujian`；推到 `ch13.daoqiyuan`、仍先找离寻 → 同上（证明 n14 跳不过去）；n27、n30 选取消 → 旗标不置、`day` 不变（n27 `realmCap` 仍 31、n30 `mapId` 不变）；扫脚本：`mingzhu.lua`、`cunjin.lua` 第一个可执行语句块判前置旗标（16.1 第 47、52 条）|

---

## 16. 拍板记录与开放问题

### 16.1 已拍板（按推荐，预授权——第 1–40 条照笔记 E 表按推荐落地；第 41–60 条是 2026-10-13 四路审查之后的整改新定，同属按推荐、预授权；**与前 40 条冲突处以后者为准**）

| # | 拍板 | 本稿怎么落 |
| --- | --- | --- |
| 1 | **开局**：patch `scripts/ch12/lingquan.lua` 末尾补一行 `teleport("ch13_nanming")`，第 12 章其余一个字不动 | 10.1 C1、T1；验收 21 |
| 2 | **章节卡**「第十三章」「逆星之乱」 | 10.1 C1 |
| 3 | **新图 4 张**、旧图 0 处 patch（全作 79 张，超大纲上限）；**O-6 定**：回写大纲第 5 节地图上限「65–75」→「80–85」，进 C7 | 第 4 节；10.1 C7；验收 16 |
| 4 | `ch13_waihai` **一图三区**（黑石城、秘市荒岛、裂风兽小岛），秘市内部与交换会走脚本场 | 第 4 节 |
| 5 | **战斗 2 场**：① 新建、② 可重复按一场计；第 14 章留 5 场 | 第 8 节；验收 15 |
| 6 | **占位 ② 就地改写**：第 0 波 `wuji_haishou` ×2、第 1 波 `qingse_juxia`；`wuji_haishou` 补「剑」| 第 8 节；10.1 C4；验收 4 |
| 7 | **五行环做法术** `magic_ji_wuxinghuan`（金、削架势 3），物品不动；血色披风只旁白 | 3.2 n01；验收 9 |
| 8 | **重炼七十二剑走脚本 n20**，学 `magic_fengyun_heji`；本章 0 张炼器配方 | 3.2 n20；验收 9 |
| 9 | **玄阴经是剧情物品**；秘术只在文案，不给功法、不给法术 | 3.2 n19 |
| 10 | **结丹中期：剧情抬上限、玩家自坐**；`realm.advance` 0 处 | 1.3；契约 E1；验收 5 |
| 11 | **出关门闸** `realm.at_least(CORE_MID)`，不到只一句 | 3.2 n28；验收 5 |
| 12 | **偷渡三支**（灵石 10000 / 五级妖丹 4 / 把柄与飞刀），只列付得起的 | 3.2 n12；第 11 节 `ch13.toudu`；验收 7 |
| 13 | **星宫的正路**：报名、安分两支都领指环；当众杀同阶做不成 | 3.2 n07 |
| 14 | **按地点分店、不做物价曲线** | 7 节「战乱封锁」行；第 9 节；10.1「不做」|
| 15 | **南明岛盘查**只读腰牌、不扣；无腰牌报名号照放 | 3.2 n04 |
| 16 | **弃府只旁白**；旧灵田挂在天星城让玩家自采 | 第 4 节；契约 E2；验收 6 |
| 17 | **雾中岛四设施**（静室 150、丹炉 grade 4、药园 6 槽、存档），无炼器台 | 第 4 节 |
| 18 | **诱妖循环**照第 11 章口径 | 3.2 n24；验收 8 |
| 19 | **收手门槛** `ch13.lieyao >= 2`；七年蒙太奇按比例收成 | 3.2 n25；验收 8 |
| 20 | **炼丹三年**、置 `ch13.liandan` 解锁至元丹方 | 3.2 n26 |
| 21 | **公孙杏之约改编**：去炉鼎、双修，改「入阵为仆」 | 3.2 n29；13 第 9 条；13.1 第 29 段 |
| 22 | **至元丹三支兜底** | 验收 10 |
| 23 | **兽潮不演**，只经孙师叔之口；黑石城的店挂在 NPC 脚本上，随人撤 | 第 5 节；验收 12 |
| 24 | **黑石城往返**两条五元传送，每趟 12 日 | 3.1 末表；验收 13 |
| 25 | **秘市与交换会**是脚本场，拍卖与竞价都是选项 | 3.2 n31 |
| 26 | **文思月**：带她离开，侍妾之说作罢；`wen_siyue.json` 一个字不动 | 3.2 n32；13.1 第 31、32 段 |
| 27 | **铜片必得**，物品名「铜片」 | 验收 11 |
| 28 | **章末停在人声**，不 `ending()`、不 teleport | 3.2 n34；13 第 14 条；验收 20 |
| 29 | **金丝蚕收尽**（原著全死）| 3.2 n22；验收 9 |
| 30 | **规则 25 登第 13 章、探针挪到 `feng_xi`**（「金」不登五行环，以第 46 条为准）| 8.2；10.1 C2 |
| 31 | **路径行动 13 条全 `inquire`**、`realm 0`、0 条 reveal | 16.3；验收 18 |
| 32 | **支线 5 条**：必经 2、可选 3 | 第 6 节；验收 19 |
| 33 | 文案一律「灵乳」，「万年灵液」0 处 | 12.2 |
| 34 | **两个化名**「曲」「厉」只在他人对白，名牌不变 | 12.2（「曲」的范围以第 43 条为准）；13.1 末 |
| 35 | **虫魔**只写三人所说与他的猜测 | 13 第 8 条 |
| 36 | **毒蛟与巨龟**旁观、不战、不建 role（笔记原拍的末一句「大祸起头」已删，以第 42 条为准）| 5.4；8.3；13.1 第 21 段 |
| 37 | **队伍** `party.*` 0 处 | 10.1 |
| 38 | **五色圆珠与资质**只旁白 | 7 节「结丹中期自坐」行；10.1「不做」；13.1 第 33 段 |
| 39 | **南明岛回血点**在 ① 之前；天星城 39 层静室 125 | 第 4 节 |
| 40 | **名字首见与禁词**按 M 表；名牌跟 role 走 | 第 12 节 |
| 41 | **n04 只写他自己不易容的推断**（元婴神识扫查、易容瞒不过反显心虚，ch503 第 24–31 段）；「画像早被复制」「煞气只露十之一」「七煞决」0 处（判据③：ch503 苍、刘私话，第 22 段他已飞离）；无腰牌版也不提画像（原 16.2 第 13 条）| 1.1 第 6 条；3.0 表二 b、表三 κ；3.2 n04；7.1；12.2；13.1 第 4 段 |
| 42 | **n21 末句只写改主意不迁府、加阵**；「导火索」「大祸」0 处（ch522 第 14 段：他并不知道）；推翻 E 表 36 与本表第 36 条的末句（原 16.2 第 14 条）| 1.1 第 28 条；3.0 表三 γ；13 第 7 条；13.1 第 21 段 |
| 43 | **化名「曲」的范围**：`yirong`–`cunjin`（n11–n15；ch512–516 许云、易敬、离寻仍称他「曲」）；修正 E 表 34 与本表第 34 条的「n11–n12」（原 16.2 第 16 条）| 7 节「易容」行；12.2 |
| 44 | **名牌口径**：名牌跟 role 走、全章不换。名字在他与此人头一回能对话的那个节点里当面出口的，上图即用本名，记为名牌例外（易敬、辛明、凌玉灵、赵护法、明夫人、离寻、古玉、公孙杏等）；名字韩立从没当面听到的——`liu_hufa`（「刘护法」只在 ch503 私话）、`min_xiushi`、`xuan_xiushi`（姓只在三人私语与传音，ch524–526）——用不含姓的描述名牌，role id 不动（原 16.2 第 15 条，扩到审查补出的五个）| 5.1；5.2；12.2 首段与「七煞决」行 |
| 45 | **facility 门闸只写 `require_flag`**，不写 `deny_text_key`，拦下时引擎播 `ui.facility.locked`；`ch13.block.yaoyuan / jingshi / dongfu` 三条 key 不建（推翻笔记 G 表与契约旧稿；引擎只有 portal 读 deny）| 3.1 首段；4.3 第 1 条；7.1；验收 6；契约 1.2、2.2、2.3 第 5 条、第 4 节 |
| 46 | **规则 25「金」不登五行环**（`power 0`、`effect stagger`，削得了势、要不了命）；噬金虫可作补充出处；推翻 E 表 30 括注里的五行环 | 8.2（权威）；10.1 C2；契约 5.2 |
| 47 | **n14、n15 脚本开头判前置**：`mingzhu.lua` 判 `ch13.daoqiyuan`、`cunjin.lua` 判 `ch13.mingzhu`，未到一句 return；两人的 `visible_flag` 仍是 `chuansong`（5.1 不动）| 3.0 表头；3.1；3.2；7.1；验收 27；契约第 4 节 |
| 48 | **意图玩家坐关次数的权威在 8.1**（真档前上界约 6 次、M ≲ 2150 日）；1.3、3.3、第 14 节、验收 2、契约第 0 节只引用 | 8.1 |
| 49 | **兽潮后黑石城是废墟**（ch531 第 5、8 段）：北区入口加 `trigger_feixu`（enter、once 否、guard `ch13.shouchao`、无 `set_flag`），每次一句废墟旁白、不拦路；地图图块不改 | 3.0 表一第 21 条；3.1；4.1；7.1；第 11 节；验收 3、12；契约第 3 节 |
| 50 | **O-2 结案**：同一 `ref_id` 跨图已有 `shenshougu_yaopu` 先例，门禁不拒；C8 不做 | 7 节「旧灵田」行；10.1；10.2；16.2 第 4 条 |
| 51 | **O-7 支线配额**：回写大纲第 5 节支线目标为实际可达值（数由第 14 章研究路给）；本章最终分担 5 条（必经 2），全作 21 / 必经 3 | 第 6 节；10.1 C7 |
| 52 | **n27、n30 开头加确认 choice**（「就此闭关 / 再等等」「这就走 / 再留几日」），取消在拨日、置旗前 return；不计入主动抉择；文案不点名按键 | 3.0；3.2；7.1；验收 27 |
| 53 | **诱妖缺草兜底**：草 0 且灵石 < 10 → `advance_days` 后 `give` 霓裳草 1；草 0、灵石 ≥ 10 仍只一句指寸金阁 | 3.2 n24；3.3；7.1；第 9 节；验收 8 |
| 54 | **`kOffChainFlags` 七面的窗口**照第 11 节末表；`mingzhu.lua` 首谈分支写在再谈 / Z1 分支之前 | 第 11 节；3.2 n14；契约 5.3 |
| 55 | **「千叶露」自第 2 步提示（编在 `wenxun`）起合法**；「伴妖草」自第 16 步提示（编在 `cunjin`）起 | 3.4；12.2 |
| 56 | **「黄道友」**：ch508 笔误那一处 0 处（`lidao.*` 之前不含）；`lidao.*` 起只许指黄明礼 | 1.2 B9；12.2；契约第 4 节 |
| 57 | **「少主是圣祖唯一传人」**：n04 壮汉当面一句（ch502 第 37 段）＋ K2 可再问；0 处出名 | 3.0 表三 β；13.1 第 4 段；16.3 K2；契约第 3 节 |
| 58 | **开局 patch 连带第 12 章验收 20 与纪律 61**：那条扫脚本断言改成「`lingquan.lua` 仅末行一处 `teleport("ch13_nanming")`」，是落点改动、不算推翻第 12 章拍板 | 10.1 T1、C7；契约 5.3；16.2 第 23 条 |
| 59 | **编成 `charge` 必写 `mult`**（加载器缺了整份拒），数值由平衡路定 | 8.2；契约第 4 节 |
| 60 | **往返 #6 落点写死坐标**，`trigger_hui_wuzhong` 与西门带隔 ≥ 3 格、落点在两者之间且不压带；五元两份脚本的取消在顶格 `return`，`advance_days` 与 `teleport` 顶格 | 3.1；4.1；验收 3；契约第 4 节 |

### 16.2 与旧拍板的冲突、开放问题（拿不准、需协调者核的）

1. **推翻第 12 章契约「境界」行**（B 表 20）：第 12 章接口写的是 ch529 `realm.advance(CORE_MID)` 一处，本章改为 `realm.cap` ＋ 面板自冲。这不是用户的板，是第 12 章留下的接口；协调者先改第 12 章契约第 3 节与笔记 O.2 那一行，再集成本章。
2. **推翻第 12 章「金丝蚕」行**（B 表 19）：第 12 章写「服圆珠的两只活下来」，原著 ch522 那一对产卵后双双毙命；同第 1 条的改法。
3. **O-1 战斗场数**：本章 2 场；第 14 章用不满 5 场时可把云天啸一击或黄明礼救援升格（预留 `b13_mishi_yuntianxiao`），不推荐。
4. **O-2 跨图同一块田**：**已结案**（16.1 第 50 条）——契约第 0 节 E2 核到 `shenshougu_yaopu` 同挂 `ch01_shenshougu` 与 `ch02_yaopu`，门禁与地图测试一向是绿的；C8 不做。
5. **O-3 ch12-end 真档**：第 2 节标「必须」的各项落地后逐项核；意图玩家按几次「闭关一年」由真档 C0 定（协调者备注：第 11 章章末修为只有 2572 / 2511），次数口径以 8.1 为准（16.1 第 48 条），增量 11790 不变。
6. **O-4 探针末章**：第 14 章写实 `b14_fengxi_xiezhi` 后无处可挪，改为 selftest 在测试副本里临时写一张 `chapter 15` 的合成编成；第 14 章研究路定。
7. **O-5 养魂木两种 id**：Z3 把根须换成木珠；第 14 章还魂术若要它，两种 id 都认，或协调者决定 Z3 不 `take` 根须。
8. **O-6 地图超预算**：**已定**（16.1 第 3 条）——回写大纲第 5 节「65–75」为「80–85」，进 C7。
9. **O-7 支线配额**：**已定**（16.1 第 51 条）——现 21 / 必经 3，距 45 / 14 缺 24 / 11；回写大纲支线目标为实际可达值，本章分担 5（必经 2），不虚报。
10. **O-8 公孙杏的改编尺度**：本稿 13 第 9 条比笔记收得更严——她在原著是十五六岁的少女（ch529 原文），夜召、摸头发、她的惶恐也 0 处；派写手前请校对确认。
11. **O-9 结丹中期锚点**：本章无一场仗在结丹中期，`Realm.cpp` 那一档「暂定」不动，留第 14 章标定。
12. **O-10 ② 的修为奖励** ≤ 60，平衡路量过再定（验收 4 已按 `<= 60` 写死）。
13. **n04 的「煞气、画像」是判据③**：笔记 A20、A21 与 F 表 n04 写「煞气感应、画像在案（旁白）」，但这两件出自 ch503 苍云龙与刘护法在韩立飞走之后的私话（novel.py read 503 第 1–21 段），他不在场、此后也没有得知。本稿按判据③改为 0 处（12.2、13.1 第 4 段），只留他自己的推断（元婴神识扫过、易容瞒不过神识强的人，ch503 第 24–31 段）。已定为 16.1 第 41 条；笔记 A20、A21、E 表 15 与 F 表 n04 已同步改（2026-10-13 整改）。
14. **n21「大祸起头」是作者旁白**：笔记 A87、E 表 36、F 表 n21 要 n21 末一句「后来想起，这是一场大祸的起头」；ch522 原文是作者对读者说此战是日后妖修之战的导火索，并明说他并不知道、不久就抛到脑后。写进文案就是「玩家知道他不知道的事」（与 E 表 35 同理）。本稿推荐末句只写他改主意不迁府、加阵，**「导火索」0 处**；第 14 章的钩子由 n28–n29 的兽潮承担。已定为 16.1 第 42 条（推翻 E 表 36 末句）；笔记 A87、E 表 36、F 表 n21 已同步改。
15. **名牌先于他知道的名字**：`liu_hufa` 名牌「刘护法」——全章这个称呼只出现在 ch503 苍、刘私话里（第 6 段），韩立从未听到；`min_xiushi`「闵姓修士」、`xuan_xiushi`「宣姓修士」的姓只在三人私下交谈与传音里（ch524–526）。推荐：这三个 role 的名牌改成描述（如「逆星盟壮汉」「淡金脸修士」「凶相修士」），role id 不动，「刘护法」仍登表一防前章误写；`yi_jing` 名牌「易敬」在 n11 上图时他只知道「易」（ch509），全名 ch511 才出口——维持笔记 M 表「名牌例外」。已定为 16.1 第 44 条，并按审查补出明夫人、离寻、古玉、公孙杏、赵护法（首见 506）同属名牌例外；笔记 H 表名牌列已同步改。
16. **化名「曲」的范围**：E 表 34 写「n11–n12」，原著 ch512–516 许云、易敬、离寻仍称他「曲」；本稿 12.2 放到 `cunjin`；已定为 16.1 第 43 条，笔记 E 表 34 已同步。
17. **笔记 B 表 32 不全**：ch522 原文虫卵上是「黑色的斑点」，孵出的新噬金虫是金壳、银斑之外「黑色的条纹」——摘要的「黑色条纹」不是造词。处置不变（文案自写），13.1 第 22 段把两样都列为质感。
18. **原著笔误**：ch538 通章把云天啸写作「云啸天」、把「厉」写作「历」（novel.py read 538）；本稿 12.2 列为全章 0 处，lore 回写时加注。
19. **n25 的 2415 日**：笔记 N.2 写 n25 `advance_days(2415)`（「七年后」），推法未写明（七年按 360 日计是 2520）；数字照抄进验收 8 与第 14 节，请协调者核一句来历。
20. **A 表与 K 表对 K8 / K9 的分派不一致**：A53、A55 的「本章怎么用」写 K8（元婴坐镇、海渊），K 表把元婴坐镇放在 K9（许云）、K8（易敬）不提海渊；本稿以 K 表为准（16.3），12.2 里「妖兽海渊」编在 `mingzhu`。
21. **O-11 带坐标的三元传送**：`lidao.lua`（秘市岸，建议 (14, 46)）与五元两条都落在非 spawn 格；测试路核 `ObjectiveRoute` 只认目的图、不认坐标（第 12 章走偏先例）。
22. **O-12 南明岛店的霓裳草**：原著 ch501 只写「结丹期丹方辅料」，这一条是为外海备饵的便利；协调者觉得多余可删，不牵动别处。
23. **开局 patch 牵动第 12 章两条断言**（16.1 第 58 条）：`docs/ch12-design.md` 验收 20（「`lingquan.lua` … 不 teleport」）与纪律 61（「`lingquan.lua` 不 teleport」）。本章 patch 之后都改成「仅末行一处 `teleport("ch13_nanming")`」；与第 12 章 16.2 第 1 条同类，是落点改动、不算推翻拍板。协调者集成时改第 12 章施工图那两行（C7），测试路改对应的扫脚本断言（10.1 T1、契约 5.3）。
24. **13.1 质感边界的覆盖面（教训 13）**：13.1 的质感项来自研究路与写手三的阅读，**没有留下逐章逐段读过的清单**，复验无从判断全不全。2026-10-13 整改时逐段读过的只有 502 第 33–40 段、503 第 0–31 段、531 第 3–9 段，其余是 `novel.py find` 定点核。**派写手之前**，协调者（或另派一路只读核对）须把 499–542 共 44 章逐章逐段读一遍、对 13.1 各段补全质感项，并把读过的章号列在本条下面——判据先于写作给全。
25. **北区废墟挂点是整改新加的对象**（16.1 第 49 条）：`trigger_feixu`、`scripts/ch13/feixu.lua`、文案 `ch13.feixu.*` 都落在内容路 A / C 原有白名单的文件范围（新图、`scripts/ch13/**`、`data/text/ch13*.json`）里，不新增文件类别；`feixu.lua` 0 处 `flag.set`、0 处 teleport，不进 `kScriptTransfers`。

### 16.3 路径行动条目（施工用；`ch13.path.*` 旗标**等 `data/pathactions/ch13.json` 落地后由协调者登记**）

全部是「打探」（`kind: inquire`）、**一律 `realm 0`**、**0 条 `reveal`**（reveal 提示会拼出 role 名牌，而本章敌人全不在图上）；`purchase` / `challenge` 0 条（黑石城严禁斗法，买卖走商店）。**K 号以下表首列为准**，第 3、4、11 节与契约引用的 K N 都对这一列。id 一律 `<npc>_<话题>_dating`（`PATH_ID_RE`）；`when` 都含那个 NPC 对象的 visible_flag、一律写 `>= 1`；`until` 都含 `ch13.done` 与该对象的 hidden_flag；同一 NPC 对象只挂一条，无时段重叠。**八个节点挂点 NPC**（`npc_yuanliao_zhanggui`、`npc_cang_yunlong`、`npc_ling_yuling`、`npc_gu_laozhe`、`npc_ming_furen`、`npc_li_xun`、`npc_gongsun_xing`、`npc_wen_siyue`）0 条，免得 E 键与对话抢同一个 NPC。文案 `data/text/ch13_path.json`，key `ch13.path.<id>.text`。

| K | id | 图 / NPC 对象 | when | until | 效果（文案要点）|
| --- | --- | --- | --- | --- | --- |
| K1 | `sanxian_tianxingling_dating` | nanming / `npc_sanxian_laozhe` | `ch13.wenxun` | `ch13.gongdao`、`ch13.done` | 天星令要宗主岛主觐见；逆星令说响应觐见的就是敌人；小宗派只能观战再选边（A3、A6）|
| K2 | `liuhufa_shaozhu_dating` | nanming / `npc_liu_hufa` | `ch13.gongdao` | `ch13.panzha`、`ch13.done` | 逆星盟那位「少主」：**这条只在盘查之前问得到**（NPC 对象 `gongdao → panzha`），壮汉还没在 n04 当面说过什么——站在盘查之前说，不提「方才」；只说「少主」、不出名、不报境界（第 14 章钩子，16.1 第 57 条）。**2026-10-15 整改改写**（`docs/ch13-review.md` C-3、C-10）|
| K3 | `xinming_xinwu_dating` | nanming / `npc_xin_ming` | `ch13.gongdao` | `ch13.panzha`、`ch13.done` | **站在盘查之前**：辛明在等、在猜——听说要验信物或功法，他身上只有一柄白水剑，不知够不够（规矩 n04 才宣布，原先写的过去时是时序错；2026-10-15 整改，C-3、C-10）|
| K4 | `qianxiushi_chuansong_dating` | tianxing / `npc_qian_xiushi` | `ch13.yirong` | `ch13.chuansong`、`ch13.done` | 一次最多七人、只开一座阵（A42）。**要点可用，次序与讲法自出**——不走原段「一人一符 → 七人 → 一座 → 多传就查」的推理链（第 13 节第 11 条；2026-10-15 整改，C-1、C-10）|
| K5 | `zhaohufa_jieyan_dating` | tianxing / `npc_zhao_hufa` | `ch13.yinjian` | `ch13.chuansong`、`ch13.done` | 星宫存着各宗长老的画像，入城一律对照腰牌与画像验过才放（ch506 第 7 段，A29）；戒严时段不准离住处那条规矩是于护法讲的（ch506 第 29 段），不挂在他身上 |
| K6 | `yuhufa_chuansong_dating` | tianxing / `npc_yu_hufa` | `ch13.yinjian` ∧ `ch13.rucheng` | `ch13.chuansong`、`ch13.done` | 传送阵如今要替星宫办一件事、或在大战里当众杀一名同阶，之前杀的不算（A31–A32）|
| K7 | `zhangxiushi_jiama_dating` | tianxing / `npc_zhang_xiushi` | `ch13.yirong` | `ch13.chuansong`、`ch13.done` | 他咬定五千一人是「上头」定的，三千连门都摸不着（A37；他在抬价，玩家已知把柄，只换一句冷笑）|
| K8 | `yijing_waihai_dating` | waihai / `npc_yi_jing` | `ch13.chuansong` | `ch13.shouchao`、`ch13.done` | 筑基修士要结伙才在外海活得下去；杀几头高阶妖兽就能把灵石补回来（ch508）|
| K9 | `xuyun_qiyuan_dating` | waihai / `npc_xu_yun` | `ch13.chuansong` ∧ `ch13.daoqiyuan` | `ch13.shouchao`、`ch13.done` | 奇渊岛众势力联手管、星宫也不愿得罪；城里时刻一位元婴坐镇；联合大拍卖一年一次（A51–A53、A58）|
| K10 | `paimaishi_shouluan_dating` | waihai / `npc_paimaishi` | `ch13.chuansong` ∧ `ch13.mingzhu` | `ch13.shouchao`、`ch13.done` | 兽卵过不了传送阵、带着走只会成死卵，所以高阶修士专程来这边买卵孵养（A57）|
| K11 | `guyu_shanggu_dating` | waihai / `npc_gu_yu` | `ch13.chuansong` ∧ `ch13.cunjin` | `ch13.shouchao`、`ch13.done` | 他炼气期修为、专研上古丹方与灵药；那本提到千叶露的上古典籍是前两年才得的，已复制进玉简（ch515 第 39 段、ch516 第 2 段，A61–A63）|
| K12 | `sunshishu_shouchao_dating` | wuzhong / `npc_sun_shishu` | `ch13.chuguan` | `ch13.gongsun`、`ch13.done` | 兽潮的事实 n29 主线已由孙师叔交代，**这条不再重说**，改讲如今出海采药的难处、不提兽潮——这条只在 n29 之前问得到，兽潮还没从孙师叔嘴里说出来（第 13 节第 11 条；2026-10-15 整改，C-1、C-3、C-8、C-10）|
| K13 | `huangmingli_mishi_dating` | waihai / `npc_huang_mingli` | `ch13.lidao` | `ch13.jiaohuan`、`ch13.done` | 秘市要熟人引荐、地点常换、不设阵法免得妖兽察觉；五六级妖兽的下落问听风阁（A127–A128）|

`sunshishu_shouchao_dating` 与 n29 不重复：n29 里孙师叔只在公孙杏开口之前一句交代兽潮，这一条是玩家想多问时的细节，`until` 有 `ch13.gongsun`，n29 演完即收。`liuhufa_shaozhu_dating` 挂的对象名牌按 16.1 第 44 条改成描述名牌，id 不变。

### 16.4 拍板前的原表（保留，供追溯；照笔记 E 表）

| # | 问题 | 推荐 | 不选推荐会怎样 |
| --- | --- | --- | --- |
| 1 | 开局怎么接 | **patch `scripts/ch12/lingquan.lua` 末尾**补一行 `teleport("ch13_nanming")`（只传图 id，`haishang` 标 default）；第 12 章其余一个字不动 | 在 `ch12_lingquan` 挂开局挂点违反「不在上一章图上挂开局」；第 12 章那张图要 patch |
| 2 | 章节卡 | 「第十三章」「逆星之乱」；`chapters.json` 加第 13 章 | 「外星海」与第 11 章已用词撞味；「雾中岛」剧透立府；「逆星之乱」是开场第一句就出的事 |
| 3 | 新图几张 | **4 张**；旧图 0 处 patch；全作 75 → 79（O-6）| 5 张（秘市单列）多一张美术与 `--check`；3 张要把两座岛画成一张 |
| 4 | 外星海怎么并图 | 一张图三区，礁路相连；秘市内部与交换会走脚本场 | 秘市单建图超预算；三区分三图则 6 张 |
| 5 | 战斗几场 | **2 场**（① 新、② 可重复按一场计）；第 14 章留 5 | 3 场则第 14 章只剩 4，而云天啸、黄明礼原著都是一招了事 |
| 6 | 占位 `b13_waixinghai_lieyao` | **就地改写**：五级 ×2 → 六级巨虾；`wuji_haishou` 补破绽 | 另建则占位悬空，selftest 探针挪来挪去 |
| 7 | 五色铜环做不做手段 | **做成法术**（金、`stagger 3`），物品不动；披风只旁白 | 不做则 ① 少了原著唯一解法；做成道具会「用一件扣一件」 |
| 8 | 重炼七十二剑 | **脚本节点 n20**：`take` 炼晶 → 360 日 → 学合剑；0 张炼器配方 | 面板配方要么概率失败成死局，要么再开一张 `guaranteed` |
| 9 | 玄阴经在机制上是什么 | **剧情物品**；秘术只在文案 | 做成功法要接引擎项；做成法术与原著「元婴以上」相反 |
| 10 | 结丹中期怎么上 | **剧情抬上限、玩家自坐**（静室 150，按「闭关一年」6–14 次——已由 8.1 修正为约 6 次以内，真档实测 2 次，16.1 第 48 条、18.5 第 3 条）| 剧情直接升：经营章最该给玩家的那一下被拿走 |
| 11 | 未破关怎么办 | n28 开头判境界，不到只一句、不置旗、不拨日 | 不设门闸则章末仍结丹初期，第 14 章接不上 |
| 12 | 偷渡怎么付 | 三支：灵石 10000 / 五级妖丹 4 / 把柄 ＋ 飞刀（旁白）；第三支永远在 | 硬收五千是死局；只有剧情一支则「五千一人」只剩台词 |
| 13 | 星宫的正路 | 报名 / 安分都领指环；当众杀同阶做不成 | 让 ① 抵条件与原著「先前不算」相反 |
| 14 | 战时坊市 | **按地点分商店，不做物价曲线** | 涨价要新引擎项，原著也没有 |
| 15 | 南明岛盘查 | 读腰牌；无则报名号照放；不扣 | 硬门闸则掉了腰牌的玩家卡死在岛上 |
| 16 | 弃府与旧灵田 | 只旁白；天星城挂旧田让玩家自采（契约 E2）| 不放则第 11 章种的药永远拿不回；「一键采收」是引擎项 |
| 17 | 雾中岛设施 | 静室 150、丹炉 grade 4、药园 6 槽、存档；**无炼器台** | 给炼器台则玩家期待能炼，而本章 0 张炼器配方 |
| 18 | 诱妖循环 | 照第 11 章口径 | 换口径要重写第 11 章已验过的整套断言 |
| 19 | 收手（七年）| `ch13.lieyao >= 2`；蒙太奇按比例收成 | 不设门槛则 ② 可以一次不打；「数百枚」真数量则经济账炸 |
| 20 | 炼丹三年 | 置 `ch13.liandan`，炼不炼由玩家 | 强制炼几炉要门闸，失败概率会制造死局 |
| 21 | 公孙杏之约 | **改编**：去炉鼎、双修，改「入阵为仆」 | 照原著写越过内容尺度 |
| 22 | 至元丹兜底 | 三支：自炼丹 3 / 六级妖丹 1 / 开炉一月 | 只认自炼的丹则炼丹失败的玩家卡死 |
| 23 | 兽潮 | **不演**；店挂 NPC 脚本，随人撤 | 店做成 facility 则兽潮后照开 |
| 24 | 黑石城往返 | 两条五元传送，每趟 12 日 | 不给往返则 ② 的妖丹无处卖，经营循环断 |
| 25 | 秘市与交换会 | 脚本场；拍卖、竞价全是选项 | 拍卖面板是引擎大项；云天啸战是灌水 |
| 26 | 文思月 | 带她离开、他婉拒侍妾；role 不动 | 写侍妾则与第 14 章她的处境冲突 |
| 27 | 铜片 | **必得**，三支付法 | 可选则第 14 章「铜片是海族圣物」无物可依 |
| 28 | 章末停在哪 | 取草 → 身后一个男子声音 → 置 `ch13.done` | 写到风某现身就要建 role、写化形第二阶，提前用掉第 14 章开场 |
| 29 | 金丝蚕 | n18 有蚕多一段；n22 余数 > 0 才收尽 | 照第 12 章接口「两只活下来」与原著相反 |
| 30 | 规则 25 与探针 | 登第 13 章；探针挪 `feng_xi` | 不挪则「第 13 章唯一含 `wuji_haishou`」那条红 |
| 31 | 路径行动 | 13 条全 `inquire`、`realm 0`、0 条 reveal | reveal 拼出 role 名牌；黑石城禁斗法 |
| 32 | 支线 | 5 条：必经 2 ＋ 可选 3 | 再多 40 分钟装不下；再少全作配额缺口更大 |
| 33 | 万年灵乳 / 灵液 | 文案一律「灵乳」 | 两名并用像两件东西 |
| 34 | 两个化名 | 只在他人对白里称呼；名牌不变 | 换名牌要换主角 role，牵动全部战斗 |
| 35 | 虫魔 | 只写三人所说与他的猜测 | 写真相＝玩家知道他不知道的事 |
| 36 | 毒蛟与巨龟 | 旁观、不战、不建 role；一句「大祸起头」（末句已由 16.1 第 42 条推翻）| 做仗与原著「八级只能逃」相反 |
| 37 | 队伍 | `party.*` 0 处 | `party.add` 要配对 `remove`，弄脏第 14 章接口 |
| 38 | 五色圆珠与资质 | 只旁白；不改 `aptitude` | 加命令是 `src/` 项，收益只有一成打坐 |
| 39 | 南明岛的回血 | ① 前放打坐设施；天星城静室 125 | ① 前无回血点可能死局 |
| 40 | 名字首见与禁词 | M 表；名牌跟 role 走 | — |

---

## 17. 施工纪律（沿用前十二章，外加本章的十条）

沿用 `docs/ch04-design.md` 第 8 节、`docs/ch05-design.md`–`docs/ch11-design.md` 各第 17 节与 `docs/ch12-design.md` 第 17 节（第 59–68 条）。本章另加：

69. **境界只经一处上限**：`realm.cap(realm.CORE_MID)` 只在 `biguan.lua`、看返回值；`realm.advance` / `realm.demote` 本章 0 处；`chuguan.lua` 开头判 `realm.at_least`，不到只一句、不置旗、不拨日。
70. **三处付出都有零成本一支**：偷渡、至元丹、铜片的选项只列付得起的；每一处 `take` 看返回值；全章没有一处靠灵石过关。
71. **兽潮只经人口、店挂脚本**：`ch13.shouchao` 只在 `gongsun.lua` 置；四家店全由 NPC 或挂点脚本 `shop()` 开，四张新图不放 `shop` 类 facility。
72. **炼器 0 配方、重炼走脚本**：`guaranteed` 全仓仍一张；`recipe_zhiyuan_dan` 不写 `guaranteed`。
73. **章末停在人声**：`banyao.lua` 最后置 `ch13.done`，不 `ending()`、不 teleport；「风某」「风希」「九级」「化形第二阶」本章文案 0 处；那句人声不写名、不写样子。
74. **旧图不 patch、旧脚本只补一行**：`scripts/ch12/lingquan.lua` 只末尾一行；`wen_siyue.json`、`fan_furen.json` 一个字不动。
75. **韩立不知道的不写**：苍、刘私话，顾乾灭口，寸金阁画中人，鸠面三人的私语，极阴与齐姓道士，「妖修之战导火索」，公孙杏独处时的所见——情报、旁白、路径行动、物品描述都不许写到（13 第 13 条；16.1 第 41、42 条）。
76. **尺度**：炉鼎、双修与一切风月场面 0 处；公孙杏之约只是「入阵为仆」；交换会里他与文思月之间没有身体接触（13 第 9、10 条）。
77. **同格不叠、once 必有旗标、enter 挂点不压 spawn**：四张图的首个 enter 挂点是落点前方的横贯带，落点、存档点、打坐设施在带的落点一侧；`trigger_qiyuan_hang` 与码头带隔 ≥ 2 格、`trigger_hui_wuzhong` 与西门带隔 ≥ 3 格、#6 落点写死坐标且不压带；`trigger_dizhen` 与 `trigger_chuguan` 分两行；NPC 挂点不设 once、脚本开头判 done_flag。
78. **可重复的仗少给修为**：② 每胜 `rewards.cultivation` ≤ 60，结丹中期只从静室坐出来；② 逃不扣草、败即 game over；诱妖照第 11 章口径，不另起一套。

## 18. 施工偏差（施工阶段填）

各路原稿在 `notes/ch13/deviations-{A,B,C}.md`；这里按路去重收录，交接给别路、已在集成时消掉的缺项不再列。几路共有的两条只记在 18.1：**文案归属**（18.1 第 11 条；B 路同记）——以派工为准，`item.desc.*`、`magic.desc.*`、`ch13.path.*`、`ch13.quest.*`、`ch13.shop.name.*`、`objective.ch13.*` 只在 B 的 `ch13_{items,path,quests,objectives}.json`，四个地名 key 只在 A1 的 `ch13_maps.json`，`ch13.battle.*` 四条只在 C 的 `ch13_battle.json`（重复 key 会让 `loadGameData` 整个起不来）；**② 开场白与旧占位重名**（18.1 第 12 条；A2、B 同记）——`b13_waixinghai_lieyao` 的 `intro_key` 字面不变、出处换到 `ch13_battle.json`，旧占位集成时已删（18.4）。

### 18.1 内容路 C（脚本与主线文案，2026-10-14）

落地：`scripts/ch13/*.lua` 40 份（主线 34 ＋ `fangshi_bimen`、`xuanshang`、`yanghun`、`feixu`、`qiyuan_hang`、`hui_wuzhong`）；`scripts/ch12/lingquan.lua` 只在 `flag.set("ch12.done")` 之后补一行 `teleport("ch13_nanming")`，头注释「不 teleport」半句改成现状；文案 `data/text/ch13_{nanming,tianxing,qiyuan,wuzhong,mishi,battle}.json`。与本稿不一致处如下（其余照 3.1、3.2、3.3、12.2、13.1）：

1. **n17 `field.unlock` 无返回值**：`scripts/common/api.lua` 的 `field.unlock` 只发命令、不回传结果，3.2「unlock false 换一句」无从判；照常调用、不分支。
2. **n13 / n14 / n15 的在场**：许云、易敬的对象自 `chuansong` 起站在祭坛下（`genmaps_ch13.py`），为守「一人一时一地」，他们只在 n13（祭坛下那条带）说话；n14 明珠轩里他独自一人，许云导览的条目拆开讲——共管、禁斗、元婴坐镇在 n13 由许云说，海渊在 n14 由明夫人说，兽卵在竞宝厅由竞宝厅老者说；n15 第 ② 条「与易敬等人分手」只一句旁白（他们往别处去了）。13.1 第 14 段「在场」一栏的许云、易敬因此不入场。
3. **n32 的洞府开在秘市荒岛东侧**：文思月的对象在秘市荒岛东侧绿山小洞府前（4.1），文案就把洞府开在这座灵气稀薄的岛上，不写「两日后另一座荒岛」。
4. **n31 的次序与取消**：三处抉择照 3.2 的次序（铜片三支 → 换她自由 → 范夫人），叙事相应调成他先买下铜片、文思月再开口（原著是一击之后才买）。`advance_days(5)` 挪到铜片抉择之后（取消再按不重复拨日）；第一次扣物之后的两处抉择取消按默认支走（「换」/ 回绝），免得已给的铜片随重演再给一次。
5. **「只列付得起的支」只剩零成本一支时仍出一个单选项**（n12、n29、n31），保住「取消在扣物前 return」。
6. **n29「回话」两支**由本路定：「不要奴婢，只要她入阵为仆」/「先问她可曾想清楚」（后者多一句后合流）；唯一的约是「入阵为仆」。
7. **n05「随行」支**：随行途中对方要他交出储物袋，他不交，对方先动手；「不随」支是他转身要走、被砚台拦路。两支各自开打，不写「假意应下、随即出手」。
8. **n01 的舞台**：落点在南明岛礁上，文案从海上问话一路说到脚下的礁石；三仙宗老者自报「先去南明岛歇脚」，以接上码头那个 `npc_sanxian_laozhe`。
9. **n24 胜后 `take` 草返回 false** 仍记一胜（换一句），照 3.2「返回 false 的路只换一句，不卡主线」；第 11 章同处是 return 不记。Z3 `take` 根须 false 同样照给木珠（数据上不可达）。
10. **n31 段落超长**：交换会一份脚本承载 12 条事实，主路 16 句，超出「每段 4–12 句」；其余各段主路都在 12 句以内（n29 走「先问」支 13 句）。
11. **文案归属**：契约 6.4 写 C 管 `data/text/ch13*.json` 全部，协调者派工把 `ch13_{items,path,quests,objectives}` 归 B、`ch13_maps` 归地图路；本路照派工，没有写这几份。
12. **② 开场白与旧占位重名**：`ch13.battle.waixinghai_lieyao.intro` 在 `data/text/battles.json` 仍有旧占位，validate 报重复 key 一条，待协调者照契约 5.2 删旧占位。

### 18.2 内容路 A（A1 地图 2026-10-13、A2 美术与音频登记 2026-10-14）

A1 交付 `tools/mapgen/genmaps_ch13.py`（四图各一函数，确定性、不用随机数）与四张 `maps/ch13_*.tmj`；A2 交付 `data/visual/{maps,battles,looks}.json`、`sprites_enemies.py` 两只非人形、`assets/art/**` 重烘、BGM 注释表。

1. **`region` 自定 `luanxinghai_nixing`**：4.2「由协调者定一个值」未给，沿 `luanxinghai_*` 前缀，四张同值；要改只动生成器 `REGION` 一处、重跑。
2. **`trigger_fangshi_bimen` 的脚本名**按「对象名去 `trigger_`」定为 `fangshi_bimen.lua`（C 同名落地）。
3. **带坐标的两处落点**（生成器常量；格上无对象、可走、可达）：#5 `lidao.lua` → `("ch13_waihai", 14, 46)` 秘市岸（存档点 `facility_cunji_mishi` 在 (13,46)）；#6 `qiyuan_hang.lua` → `("ch13_waihai", 7, 12)` 黑石城西门外礁（`trigger_hui_wuzhong` 在礁头尖 (3,12)，`trigger_chucheng` 是 x=10、y 11–13 的竖带，落点不压任何一条）。default spawn：nanming `haishang` (6,29) 朝上、tianxing `chengwai` (27,36) 朝上、waihai `shiwu` (34,7) 朝下、wuzhong `andeng` (59,37) 朝上。
4. **横贯带的形状**：`trigger_wenxun` (5,28)、`trigger_rucheng` (26,35)、`trigger_lifu` (58,36) 都是 3×1、紧贴 spawn 北面一格；外星海 `shiwu` 在屋内，`trigger_shiwu` (33,9) 3×1 是门 (34,8) 外台阶第一行。东西走向的通道用 1×3 竖带：`trigger_chugang` (42,13)、`trigger_chucheng` (10,11)、`trigger_lidao` (55,15)、`trigger_buzhen` (12,30)。
5. **interact 挂点画成实心格**（building gid 8，站旁边按，第 11、12 章同写法）：天星城三十九层洞府门 `trigger_dongfu39` (10,12) 是实心门，旧灵田 `facility_yaoyuan_39` (16,11) 与静室 `facility_jingshi_39` (21,12) 在门外同一层的院内台地上，只靠 `require_flag ch13.rucheng` 拦（施工图只要求与门不同格）；第四层宅院 `trigger_zhaiyuan` (36,18) 画实心院墙、不画院内；雾中岛六扇门在凹谷北壁与东壁，门后石室不画可走地面。
6. **雾中岛「山脉凹谷洞府」**：山体中凿凹谷院（x 20–43、y 4–12），三格宽甬道（x 30–32、y 13–21）出谷；`trigger_dizhen` 在 y=15、`trigger_chuguan` 在 y=19；静室 `facility_jingshi_wz`、丹炉、药园与 `trigger_yanghun` 全在院内。公孙杏 (35,24)、孙师叔 (35,26) 在甬道口东侧林边。
7. **外星海三区**：黑石城（西门、南门）→ 南门外礁路主干 → 西南、东南各一支；`trigger_feixu` 是北区与礁路之间唯一一行；西门外礁头是死路、只通 `trigger_hui_wuzhong`；东南小岛只有 `trigger_jiaoshi` 一个口，洞口 `trigger_dongkou` (61,44)、章末站 (60,44)。南明岛港后丘陵是死路，坡口只留 `trigger_qiuling` 三格；打坐 `facility_dazuo_nanming` (40,12)。
8. **预设照第 4 节表**：nanming `dock/day/mist/dock_river`、tianxing `town/dusk/dust/dock_river`、waihai `dock/day/mist/dock_river`、wuzhong `valley/day/mist/herb_valley`；① `dock_river`、② `cliff_top`；BGM nanming / waihai `bgm_river`、tianxing `bgm_town`、wuzhong `bgm_valley`（`catalog.py` 注释表与 `docs/audio.md` 同一张表各 4 行，元组不动；`audio.md` 各曲「用在」一栏不归本章）。
9. **人工指认 `structures` / `zones`**：A1 的实心块在 natural 主题里会被 mapinfer 认成屋顶，四张图都写了岩体 / 城墙 / 高台的指认；礁路改 gravel、石阶 steps、外星海祭坛石屋 rock ＋ stone_floor ＋ 门外 steps（`hall_stone` 读着像屋顶，不用）、雾中岛院子 cave_floor ＋ 甬道 cave_path（第 10 章小寰山的画法），每条带 `why`。
10. **A1 的「树」认不成树**（南明岛坡上 4 株、雾中岛谷外 6 处；mapinfer 要树干格本身带 front）：A2 在 `maps.json` 里逐个指认成 `rockery`（一堆山石），协调者按 LOW 接受、不改生成器。
11. **雾中岛海面荷叶**：`mapwater.paint_water` 对 valley / manor 主题的所有水面撒荷叶，四面是海的雾中岛整片海成了荷塘。协调者拍板主题仍 valley、走单图开关：`tools/artgen/mapwater.py` 加 `lotus_of`（读 `maps.json` 的 `lotus`，缺省 true，非布尔当场 `ValueError`），ch13_wuzhong 写 `"lotus": false` 与 `lotus_why`；重烘只变 `maps/ch13_wuzhong/below.png`，其余产物（含别章 valley / manor 图）逐字节不变。
12. **外观 29 条**按名牌、role note 与文案写明的外貌类型化（名牌里的「淡金脸」取 `tan`），同图同时在场的两两分得开；**非人形 2 张**：五级海兽 40×32、青色巨虾 48×40（六只碧绿的眼、一缕缕黑气），待机×2＋出招＋受击＋倒下。第 11 章 `siji_haishou` 仍无图，不在本章范围。

### 18.3 内容路 B（数据，2026-10-14）

1. **目标链文案 key** 取 `objective.ch13.<step id>`（契约 5.2 与第 1–12 章口径），不用施工图 3.4 的 `ch13.objective.*`。
2. **公孙杏那条支线**：任务文案（标题、摘要、步骤）0 处「公孙杏」（12.2：`gongsun` 之前不得出现她的名字），标题写「雾海来客」；文件 `name` 字段仍是「公孙杏之求」（`name` 不进 `data/text`）。
3. **`q13_qianyelu` 不写 target**：接取在南明岛（`caigou`），指向外海的对象会让告示板提前报出外海地名（地名编在 `chuansong`）；s1 / s2 只写到「千叶露」。
4. **商店名** `ch13.shop.name.<短名>` 写在 `ch13_quests.json`（第 11 章同放 `ch11_quests.json` 的先例），`nameKey` 名为本路自定。
5. **`wuji_haishou` 就地改写不止补「剑」**：旧值是「写到那一章时重填」的占位，给了内容初值——气血 720→320、法力 430→54、攻 66→34、防 50→36、身法 30→24；`magics` 去掉冰箭术（对结丹防御只打 1 点，G-34 下会被反复放），只留海潮诀三发；`toughness 3`、`element 4` 不动，不写 `killable_by`。`b13_waixinghai_lieyao` 去掉 `rewritten_by_lore`（`src` / `tests` / `tools` 0 处读它），`name`「珊瑚礁诱妖」。
6. **数字**（施工图只写意图，以下都是内容初值，归平衡路）。**下表已作废**：平衡路调过 3 个 role 的 5 个数，现值见 18.5 第 2 条，照下表改数会把平衡改回去（测试质量审查 T-1、门禁审查 G-3）：

| role | 境界 | 气血 / 法力 | 攻 / 防 / 速 | 五行 | 架势 / 破绽 / killable_by | 其他 |
| --- | --- | --- | --- | --- | --- | --- |
| `nixing_rusheng` 白面儒生 | CoreEarly | 480 / 96 | 34 / 38 / 24 | 木 | 3 / 金剑 / 金剑 | 黑雾怪鸟四发 |
| `nixing_dahan` 黑壮大汉 | CoreEarly | 600 / 0 | 38 / 44 / 18 | 土 | 4 / 金木剑 / 木剑 | 蓄势 every 3 ×2.0 单体 |
| `wuji_haishou` 五级海兽 | CoreEarly | 320 / 54 | 34 / 36 / 24 | 水 | 3 / 土剑 / — | 海潮诀三发 |
| `qingse_juxia` 青色巨虾 | CoreMid | 820 / 54 | 36 / 48 / 26 | 水 | 5 / 金木剑 / 木剑 | 行动 2；蓄势 every 3 ×1.8 全体；海潮诀三发 |

   - 理由：韩立结丹初期 700 / 攻 62 / 防 46，剑一刀约 160；② 两波串打、第 0 波不回血，第 0 波压得很轻；巨虾攻防远低于结丹中期锚点 76 / 58。**G-34**：配了法术的敌人一发与普攻相当、法力只够三四发，放完落回拳脚；大汉法力 0。`rewards.cultivation` ① 300、② 40（≤ 60，O-10）。
   - 法术：`magic_ji_wuxinghuan` 金、stagger 3、needMp 30、needRealm CoreEarly（不进规则 25）；`magic_fengyun_heji` 木、power 64、needMp 40、needRealm CoreEarly、boost power（`BATTLE_EXTRA_MEANS` 的 learn 来路，守第 13 章起点 CoreEarly / 470）；`magic_mihuan_guniao` 水、power 24、needMp 24、target all。
   - 配方 `recipe_zhiyuan_dan`：difficulty 60、requiredProficiency 40，按 ch12-end 炼丹熟练 100、资质 50、丹炉 grade 4 算成功率 60%（第 7 节要 ≥ 50%）；`pill_zhiyuan_dan` grade 4、price 1500、restoreHp 300（笔记 L.1 估值）。story 一律 price 0、不可交易，唯 `story_manao_jiao` price 5000（买价）。
   - 商店：南明岛 buyRate 0.9 / sellRate 0.5、限量不补；明珠轩 1.0 / 0.8、entries 空；寸金阁 1.0 / 0.5（霓裳草、黄精各 5 株、每 5 日补 3、上限 5；玛瑙角 1 件不补；回气丹 3 颗、每 10 日补 1、上限 3）；秘市 1.0 / 0.6、限量不补。
   - 路人境界：`gongsun_xing` QiRefining10、`gu_yu` QiRefining13（原文只说炼气期）；名牌 `liu_hufa`「逆星盟壮汉」、`min_xiushi`「淡金脸修士」、`xuan_xiushi`「凶相修士」（16.2 第 15 条建议原样取）。
7. **文案编号**：目标链 R-3 提示 11 条照抄 3.4 原句，其余 23 条自写、按「上一步的场景」编号；路径行动 13 条 59–88 个汉字，按「when 里最晚的链上旗标」编号（K5 编在 `yinjian`，赵护法不自报名号、不提于护法；K8 编在 `chuansong`，不点易敬 / 黑石城、不叫「曲」；K2 只说「少主」与「圣祖唯一一脉」）；物品描述按 give 它的脚本编号（玛瑙角没有 give，按最早合法的 `caigou` 写、不点寸金阁；`guyu_yujian` 不提五色珠图）。

### 18.4 集成（协调者，2026-10-14，构建槽 `integ13`）

1. **契约 5.2 落地**：`data/text/battles.json` 删旧占位 `ch13.battle.waixinghai_lieyao.intro`（18.1 第 12 条消掉）；规则 25 登 `CHAPTER_MEANS[13]`（第 12 章九条照旧，五行环不登）、`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"] = [("木","learn","magic_fengyun_heji","ch13")]`（编成无友军；蜂云合剑木、needRealm CoreEarly、needMp 40；`chonglian` 在 ② 的门闸 `buzhen` 之前）、`MEANS_LAST_CHAPTER = 13`、`CHAPTER_START[13] = ("CoreEarly", 470)`；selftest 两处探针挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`（含第 12 章整改加的两处必报判据），对照：只把末章改回 12 → 69 条里 3 条 FAIL；`data/flags.json` 登 `ch13.path.*` 13 面（597 → 610，原条目逐字节不变）；`tests/Ch13EngineTests.cpp` 5 处 `GTEST_SKIP` 改 `FAIL()`，`Ch13*` 27 / 27 绿、0 跳过。
2. **合缝修补 0 处**：四路交付文件之间的 key、对象名、role id、脚本路径、字段形状没有一处对不上（validate 0 错 0 警告；全仓 `data/text` 0 个重复 key；`scripts/ch13` 的七处 teleport 与契约 5.3 `kScriptTransfers` 一致）。
3. **门禁**：`VALIDATE_OK`、`MAPGEN_IN_SYNC`（79 张）、`SELFTEST_OK`（413 条 ＝ 411 ＋ `CHAPTER_START[13]` 对 `ch12-end` 两侧 2 条，未如期抓住 0）、`ARTGEN_IN_SYNC`（721 个）。
4. **终证** `build-integ13.log`（01:57）：编译 0 警告（同槽全量编译 `build-integ13-compile.log` 236 步 0 警告），ctest 1948 / 1973，`EXITCODE=1`。25 红全属测试路的登记类（契约 5.3）：`Ch07BattleData` 放行表 1、`Ch12*` 16（`lingquan.lua` 补 teleport 后的通关终点 / `ch12-end` 重生成 / 「不 teleport」断言）、`NpcPresenceShipped` 1（`kOffChainFlags` / `kLastChapter`）、`ObjectiveRoute` 3（`kScriptTransfers`）、`PathActionData` 1（`realmCapOfChapter`）、`UiStyleTests` 3（章节表 / 卡）。

### 18.5 测试路与平衡路（测试路 A、B，2026-10-14；并入施工图 2026-10-15，测试质量审查 T-1、门禁审查 G-3）

1. **测试路 A**（`build-ch13test`，2008 / 2008）：`ch12-end` 两份按 `lingquan.lua` 末行 patch 重生成，只差 `mapId`（`ch12_lingquan` → `ch13_nanming`）、坐标（(12,11) → (6,29)）、计时与校验和；登记类（章节表 13 项、`kScriptTransfers` 7 条、`kOffChainFlags` 7 条、`realmCapOfChapter {13, CoreMid}`、第 7 章放行表）。措辞口径：商店「`shop(` 恰 4 处」实为 4 个脚本、6 处调用；12.2 未编号类的补编口径见 `tests/Ch13LexiconTests.cpp` 文件头；`caigou.lua` 首句判 `ch13.wenxun` 是死代码（落点之前必已置）。
2. **平衡路（测试路 B）**：初值下 ① 险胜（韩立余 75）、② 第二轮必败，调了 3 个敌方 role 的 5 个数，破绽、`killable_by`、架势、行动、蓄势、编成、波次、`rewards` 一处没动：

   | role | 气血 | 法力 | 攻 | 防 | 说明 |
   | --- | --- | --- | --- | --- | --- |
   | `nixing_rusheng` | 480 → **360** | 96 | 34 | 38 | 意图手蓄满的祭小刀符宝一发 391 收得掉；**气血过 391 就收不掉**，日后动 ① 先看这条线 |
   | `nixing_dahan` | 600 | 0 | 38 | 44 | 没改 |
   | `wuji_haishou` | 320 | 54 → **18** | 34 → **30** | 36 | 海潮诀只剩一发（G-34）|
   | `qingse_juxia` | 820 → **700** | 54 | 36 → **30** | 48 | actions 2、every 3、all、×1.8 不动 |

   调后两侧相同：① 胜 16 回合 700→312、大汉蓄势被打断 1 次；② 第一轮胜 13 回合 700→343；② 第二轮（三日回三成）胜 10 回合 553→283。敌方法力 ≥ 法术耗蓝**不归规则 25 管**（规则 25 只核友军），是人工核过、由 `Ch13BattleData` 钉住的（门禁审查 G-2；`notes/ch13/balance.md` 第 1 节末句「规则 25 守得住」是误述）。
3. **坐关次数**：两侧 C0 4452 / 4391，「闭关一年」两次后 5235 / 5228，第一次冲关即成，**N = 2**（`Ch13Driver.h` 的 `kFirst` / `kSecond` 字面量，`static_assert` 钉 N ≤ 6），章末 day − 起点 day ＝ 11790 ＋ 360 × 2 ＝ 12510。8.1 已回写。
4. **判据**（测试质量审查 T-2，协调者拍板）：回合窗取记录版 ① 10–22、② 6–18（`Ch13AcceptanceTests.cpp::kTargets`）；「不险胜」直接写成判据——每场胜后韩立余血 ≥ 150（`kHeroHpFloor`），负向含「余血跌破下限」与「把三个 role 改回调前值重打，① 19 回合余 75 必判不达标」；① 另断「蓄势至少被打断一次」。
5. **意图手的取舍**（`Ch13Driver.h::intentPolicy`）：不吃药、**不按五行环**——`BattleHand` 只列 power > 0 或带毒的法术，五行环 power 0、effect stagger 永远不按，所以两场胜负一律不靠五行环（技术债 G-37）。补了切片 `Ch13Slice.FourTheRingFromTheMenu…`：① 开场经真菜单放五行环，类别金、儒生架势 3 → 0 当场破势、不伤人、法力 −30，对照组祭金蚨只削 1（测试质量审查 T-3）。② 巨虾的蓄势两侧四场都没被打断（平衡目标不要求），没有运行时证据。
6. **验收 5 的种子**：面板冲关种子是 `breakthroughSeed`（day、修为、境界的哈希），不可设；切片 5 在 day 53941 冲不成，先在静室坐 1 日到 53942 再写 5000 才成，两版都写进用例。「面板下一关报 StoryCap」断的是 `disabledReason` 与返回值，不是面板上的字（第 11、12 章同况）。
7. **G-31 被钉成了现状**：`Ch13AcceptanceTests.cpp` 断言读档后朝下（`facing == 2`），章末人其实朝东；将来修 G-31 这一行会红，届时一并改。
8. **其余口径**：n29「回话」通关驱动取第一支 `opt_terms`；验收 6、12、13、27 的切片起点按图上实际可达改过；`Ch13TriggerMode` 的 `layoutErrors` 现也查 facility（测试质量审查 T-7）；`Ch13Ledger` 两处先后比较先断两头都在（T-5）；逐节点账 n28 取 `side.retreats`、不取驱动自数的按键次数（T-4）。

### 18.6 内容整改（2026-10-15，按 `docs/ch13-review.md` 初审 3.0 的问题清单）

1. **C-1 逐槽换词 11 个 key 全部重写**：开价两句改成「截到半句牢骚」＋「先诉担风险、再报数」；喊人那句改成「响动传上去、灵石白攒」；自报「姓曲」那句改成「自掏灵石、多一双手」；乾姓七人那段（K4）改从「阵石每日上报」切入；看护那句改成指空符匣；公孙杏的誓言改成「命归前辈使唤、只求一味药」，病症挪给孙师叔在 `kneel` 里交代；孙师叔说兽潮改成先说青灵门剩几个人、再倒推兽潮；留书不写原文，只写「东西收好、别外露」，孝心挪到 `why`；文思月的条件改成先说门里换主、留下是死路，再拿巢穴换「带我走」；章末人声改成一句冷淡的招呼，不提守了多久、不点草。
2. **C-2 n01 次序全换**：从老者腰间的鬼脸牌切入，他一句句倒问出逆星令 → 逆星盟 → 天星令与外星岛；「前辈不知么」「不敬」「去天星城的人多了」三处去掉。其余 42 个 key 与 6 处字眼按表逐条改。
3. **C-3**：K2、K3 改成站在盘查之前说（16.3 已回写）。**C-4**：妙鹤真人在 `chongmo.fake`（碧云门亲自出海搜过）与 `mishi.fan`（连妙鹤前辈都应下了）各铺一句，`wenshi.infer` 的推断有了来路。**C-5 / C-8**：盘查两支去掉「姓韩」；灵乳、「…惯了」、「一概不作数」、「香气顺着海风」各只留一处；K9、K10、K12、K13 不再重说主线。**C-6**：「四百年」进 `youyao.drip`，「裂风兽八级才产子」进 `wenshi.warn`，「奉王长老之命」进 `gongdao.notice`；`gongsun.note` 改「后来离岛那天」，`chuguan.not_yet` 不替玩家移动，`wusezhu.eggs` 去掉没头的「互相吞噬」。**C-7**：13 条路径行动压到 40–70 汉字、≤ 78 字（`PathActionData` 另要 ≥ 40 汉字）。**C-9**：n02 目标改「两味」、引号统一；11 条 R-3 提示补句号；养魂木任务摘要改成全程都对；「旧得掉渣」去掉。
4. **脚本只动一处**：`yirong.lua` 的 `yirong.threat` 改由旁白说（易敬不接话、众人散开半围），其余 key、旗标、门闸、数据、地图不动。
5. **原创性机器口径没跑**：本机（macOS）没有原著全文，`lcs_check.py` 跑不了；改写按初审给的「骨架」逐条避开，复验时须在有原著的机器上补跑一次。
6. **独立复验（2026-10-15，未对原著、只凭初审骨架）3.8，按它的清单再改 16 个 key**：`mishi.fire` 改成侍从「应声上前」、人还在席上（后面 `wait` / `swap` / `out` 接得上）；`mishi.siyue` 补「端茶递水」一句，`wenshi.release` 对着它作罢（侍妾一说的来处）；`yirong.direct` 句首点出「易道友」（13.1 段 11 第 3 条）；`yinjian.escort` 改「先替道友打个招呼」，`rucheng.gate` 补「凌护法已传过话来」；王长老从苍云龙的通告挪到 `gongdao.wait` 的旁人议论里；`liandan.three` 不写积灰；K3、K4、K6、K10、K12 各自去掉时序错、重复与逻辑打架；`banyao.dawn`、`qianru.decide` 理顺时序。复验人的结论是这几处改完即可通过、不必再整体复验；**`lcs_check.py` 仍须在有原著的机器上补跑**，并人工核 `gongdao.*` 与 K4。
