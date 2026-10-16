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
| 6 | 他以妙音门客卿长老腰牌过关、不易容（两位长老画像早被复制）；苍云龙感应他煞气所露不过十之一（A19–A21）| 502–503 | n04 读 `story_miaoyin_yaopai`；旁白（可不点「七煞决」）|
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
| 19 | 黑石城是奇渊岛唯一的城，整座岛众势力共管；除竞斗场外严禁斗法、违者形神被灭；时刻一位元婴坐镇（A51–A53，B4、B22）| 512 | `ch13_waihai` 北区；黑石城 0 战斗、0 切磋；K9 |
| 20 | 明珠轩收购海外所得；附近有妖兽海渊；竞宝厅蓝光石起价五百、紫云鹰卵三千三成交；联合大拍卖一年一次（A54–A58）| 513–515 | `ch13_mingzhu_shougou`；n14 观拍只看；K9；K10 |
| 21 | 寸金阁是城内最大原料铺，管事离寻结丹初期，古玉炼气期、专研上古丹方；外来凡人须登记（A59–A62）| 515 | n15；`ch13_cunjin_ge`；K11；炉鼎拍卖不演（B30）|
| 22 | **千叶露即八级妖兽旁伴妖草之汁**；玛瑙角按五千收进、照进价出；他以丹药与符宝换古玉玉简（A63–A65）| 516 | n15；「伴妖草」自此解禁；玛瑙角价 5000（可选）；换简旁白不扣物 |
| 23 | 海图：西边有带小型灵脉、少有人住的岛；北飞十余日后修士渐少（A66–A67）| 517 | n16 |
| | **幕四　雾中岛（ch517–534）** | | |
| 24 | 第四座无人灵脉荒岛，灵脉在一条微型山脉；吞海鱼吐雾罩岛；追四级银翼鸟误入雾海（A68–A71）| 517 | `ch13_wuzhong`；n17；文案多写「雾海」（B17）|
| 25 | 虫室五间；金丝蚕可二次进化为幻焰蛾、奇虫榜第十七，一雌一雄各喂一粒五色圆珠；九曲灵参移入药园禁地；新绿液全用来催养魂木（A72–A76）| 518 | n18（有金丝蚕多一段）；支线 Z3 |
| 26 | 肋骨灌阴气显出是玉简即玄阴经；他已修青元剑诀、不重修玄阴大法；天都尸火须元婴以上；试开虚天鼎半月无果（A77–A80）| 519 | n19；玄阴经只作剧情物品；鼎不开 |
| 27 | 炼晶是熔岩路上所得、炼入飞剑；实炼近一年；杂务由巨猿傀儡打理（A81–A83）| 519–520 | n20；文案「熔岩路上那块炼晶」|
| 28 | 化形雷劫是七级进八级之劫；红鳞毒蛟八级；金霞道士与霍氏兄弟来猎蛟；此战是日后妖修之战的导火索（A84–A87）| 520–522 | n21 旁观不战；末句他只「隐约觉得」|
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
| B9 | 摘要偷渡价与经办人称呼 | ch507 三千被拒、五千一人；ch508 一处「黄道友」、他处皆「张道友」| 采五千一人；经办人一律「张」，「黄道友」0 处 |
| B10 | lore #56「八级毒蛟屠人类修士」| ch521 只与金霞、霍氏交手；ch522 才说是日后导火索 | 回写 lore #56 |
| B11 | lore #58「虫魔之名系碧云门捏造」| ch527 是他的猜测；原文是极阴照齐姓道士之法派弟子冒充 | 回写 lore #58；游戏只写他的猜测与三人所说 |
| B12 | lore #59「距结丹已约十五六年」| 十五六年是修第八层到破中期瓶颈的闭关；此前另有七年猎妖、三年炼丹 | 回写 lore #59 |
| B13 | lore #63 与人物总表风希「ch541–561」「ch543 才出场」| ch541 只有空洞窟；ch542 自称「风某」；全名 ch543 | 回写 lore；本章 0 处「风希」 |
| B14 | lore #62「梵圣真片……以为普通铜片买下」| ch537 因古文近似起意；ch539 以一颗五级妖丹换得；ch541 判为妖修功法 | 回写 lore #62；物品名「铜片」|
| B15 | 人物总表 `yun_tianxiao`「范夫人政敌」| ch536–540 两人亲昵同席、同为夺权一方 | 回写人物总表 |
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

- **原著**：ch503 他自认结丹初期 → ch524–528 七年猎妖、三年炼丹（仍初期）→ ch529 闭关十五六年、修成剑诀第八层、破结丹中期瓶颈 → ch534 中期后又闭关二十多年、续修第九层 → ch542 仍结丹中期。
- **游戏**：起点 `realm 31 / realmCap 31 / formerRealm 22`（估，第 2 节）；止 **32 / 32 / 22**。
  - n27（`biguan.lua`）`local ok = realm.cap(realm.CORE_MID)` **全章唯一一处**，看返回值（false 只换一句照走）。
  - 31 → 32 **由玩家在雾中岛静室 `facility_jingshi_wz`（meditate、`effectiveness 150`）按面板冲关**：所需 5000 点，到线成功率 85%（同档基准 60 ＋ 资质 25 ＋ 丹药 0；夹 [5,95]），失败倒扣两成。
  - n28（`chuguan.lua`）开头门闸 `if not realm.at_least(realm.CORE_MID) then … return end`：不到只一句「瓶颈未破」，不置旗、不拨日。静室随时可进，失败只折修为——**不设死局**。
  - `realm.advance` / `realm.demote` 本章各 **0** 处（推翻第 12 章契约第 3 节「境界」行，B20）；`formerRealm` 不读不写（31 ＞ 22，无重修八倍）。
- **为什么由玩家坐**：雾中岛是经营章，最该给玩家的那一下就是亲手按出中期；剧情直接升会把它拿走。
- **坐多久（意图玩家）**：基准每日约 0.9 点、计顿悟期望约 1.02 点；所需 `5000 − C0`（C0 ＝ 进静室时的修为）：C0 ＝ 0 → 约 4900 日，C0 ＝ 1500 → 约 3430 日，C0 ＝ 3000 → 约 1960 日。面板「闭关一年」每次 360 日，**意图玩家按 6–14 次**。C0 必须拿真档核（O-3）。文案只写「十余年」。
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
| `cultivation`（C0 的底数）| 未知（数百到三千）| 第 11、12 章战斗修为 | 1.3 坐关日数 | **必须**（决定按几次「闭关一年」）|
| `material_lingshi` / `_zhong` | 低阶数百到四千、两侧差 20；中阶 20 / 20 | 第 12 章「恰起点 − 8」/「恰起点」| 偷渡第一支要 10000，意图玩家付不起——第三支兜底；中阶不读 | 低阶**必须**、中阶可选 |
| `material_yaodan_wuji` / `_liuji` | 0 / 0、0–1 | 第 12 章契约第 3 节「妖丹」行 | 偷渡第二支意图玩家没有 | **必须** |
| `party` | 空 | 第 12 章 E1 | `party.*` 0 处 | 必须 |
| `learnedMagics` | 第 10 章 11 门 ＋ `magic_shijin_chongqun` | 第 12 章验收 2 | 规则 25 通表 | **必须** |
| `story_miaoyin_yaopai` | 1 / 1 | 第 11 章 n25 | n04 只读不扣 | **必须** |
| `story_xuangu_leigu`、`material_lianjing` | 各 1 | 第 12 章 | n19、n20 `take` 看返回值 | **必须** |
| `story_jinsi_can` | `ch12.jincan == 1` 时 3，否则 0 | 第 12 章抉择 | n18 多一段、n22 收尽 | **必须** |
| `story_yanghun_mugen`、`story_wannian_lingru`、`story_xuese_pifeng`、`story_wuse_tonghuan`、`story_jiuqu_lingshen`、`story_xutian_ding`、`story_qianlan_zhu`、`story_tihun_shou`、`story_hualan` | 各 1 | 第 12 章验收 2 | 只旁白读；根须进 Z3 | 必须（Z3 门闸）|
| `field_tianxing_39` 六槽 | 第 11 章章末作物（空到满）| 第 11 章 | 玩家经 `facility_yaoyuan_39` 自采 | 可选 |
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
| 1 | 海上问话：天星令、逆星令、逆星盟；五行环祭熟 | 499–500 | n01 | 全章的「乱」从这一问开始；五行环是 ① 的唯一解法 |
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
| 21 | 公孙杏求药、兽潮旧事 | 530–533 | n29 | 至元丹三支；黑石城人去城空 |
| 22 | 离岛、秘市交换会、文思月、铜片、深渊边缘取草 | 533–542 | n30–n34 | 主线收束；第 14 章的三条线头（铜片、伴妖草、人声）|

**表二　旁白 / 蒙太奇（10 条）**

| # | 原著事件 | 章 | 放在哪 | 为什么不演 |
| --- | --- | --- | --- | --- |
| a | 披血色披风赶路、半月海路 | 500 | n01 末段 | 赶路 |
| b | 辛明出示信物；画像在案、煞气感应 | 502–503 | n04 | 他在场，但只是盘查的过程 |
| c | 空街、神识扫到指环即退 | 507 | n08 开头 | 质感（13.1）|
| d | 以丹药与符宝换古玉玉简；北飞十余日、转西两日；追银翼鸟入雾 | 516–517 | n15；n16 末、n17 开头 | 一笔不扣物的交易；赶路 |
| e | 九曲灵参移入药园禁地；半月炼布阵器具、新噬金虫卵入控神法阵 | 518、522 | n18；n22 | 他做，但没有后果要玩家选 |
| f | 巨龟渡劫、毒蛟与金霞霍氏相斗、血光遁走 | 520–522 | n21 | 八级只能逃（1.1 第 4 条）|
| g | 聚魂钵抽魂取丹；七年：深渊暴动、琉璃兽一剑、深海数年 | 523–527 | ② 胜后；n25 蒙太奇 | 一句交代妖丹来路；七年只剩几件事 |
| h | 三年炼丹、丹术近大师；二十余年、鹰鸢兽被光柱退走 | 528–534 | n26；n28 | 不做「炼丹大师」数值；他一步没出洞府 |
| i | 换形诀；救黄明礼一招 | 534 | n30 | 一招了事，做成仗是灌水 |
| j | 云天啸十二柄小剑一击；铜片光影三十六式；资质近三灵根 | 538–541 | n31、n33 | 一击了事；资质不改数值 |

**表三　不演（韩立不在场、无从得知；9 条）**

| # | 原著事件 | 章 | 玩家怎么知道 |
| --- | --- | --- | --- |
| α | 万三谷、六道极圣本人 | 500 | 只闻其名（n01），0 处上场 |
| β | 逆星盟「少主」是谁 | 502 起 | 只从 K2 听到「圣祖唯一传人」，不出名、不报境界（第 14 章）|
| γ | 毒蛟此战之后蛟族入局、妖修之战 | 522 | 不知道——n21 末他只「隐约觉得」是大祸起头 |
| δ | 极阴照齐姓道士之法派弟子冒充他杀人夺宝 | 527 | 不知道——只写三人所说与他的猜测（碧云门或虚天殿老怪），极阴、齐姓 0 处 |
| ε | 万法门长老漩矶潜来奇渊岛 | 528 | 不知道——漩矶 0 处 |
| ζ | 兽潮本身（海渊万妖涌出、黑石城被破、两位元婴陨落）| 二十余年前 | 只经孙师叔之口（n29、K12）；不演、不建战斗 |
| η | 妙音门惊变的经过 | 虚天殿前后 | 只从文思月口中知道结果，他自己推断时点（n32）|
| θ | 铜片的真名与来历 | 546 | 不知道——「铜片」；「梵圣真片」0 处 |
| ι | 身后那人是谁、化形第几阶、住在哪 | 542–543 | 不知道——只写人声（n34）|

**主线节点表（本章节点事实的唯一权威）**

脚本名 ＝ id 去掉 `nNN_`（如 `scripts/ch13/wenxun.lua`）。「前置」即挂点的 `guard_flag`（NPC 节点为 NPC 的 `visible_flag`，脚本开头另判）；`done_flag` 即目标链该步的完成旗。挂点格位与 mode / once 见 3.1，动作见 3.2，日数见 3.3。

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
| n18 | `n18_chongshi` | 四 | 518 | `ch13_wuzhong` | `trigger_chongshi` | `ch13.lifu` | `ch13.chongshi` | 虫室；五色圆珠一匣 |
| n19 | `n19_xuanyin` | 四 | 518–519 | `ch13_wuzhong` | `trigger_shichuang` | `ch13.chongshi` | `ch13.xuanyin` | 肋骨 → 玄阴经；鼎不开 |
| n20 | `n20_chonglian` | 四 | 519–520 | `ch13_wuzhong` | `trigger_lianjianshi` | `ch13.xuanyin` | `ch13.chonglian` | 重炼七十二剑；蜂云合剑入法术表 |
| n21 | `n21_dujie` | 四 | 520–522 | `ch13_wuzhong` | `trigger_dizhen` | `ch13.chonglian` | `ch13.dujie` | 旁观化形雷劫与毒蛟之战 |
| n22 | `n22_wusezhu` | 四 | 522 | `ch13_wuzhong` | `trigger_biguanshi` | `ch13.dujie` | `ch13.wusezhu` | 炼尽五色圆珠；金丝蚕收尽 |
| n23 | `n23_buzhen` | 四 | 523 | `ch13_wuzhong` | `trigger_buzhen` | `ch13.wusezhu` | `ch13.buzhen` | 珊瑚礁四阵；诱妖自此可用 |
| n24 | `n24_youyao` | 四 | 523 | `ch13_wuzhong` | `trigger_youyao` | `ch13.buzhen` | `ch13.lieyao`（计数，≥ 1 记一步）| **battle ②**（循环）|
| n25 | `n25_chongmo` | 四 | 524–527 | `ch13_wuzhong` | `trigger_shoushou` | `ch13.buzhen`（脚本另判 `ch13.lieyao >= 2`）| `ch13.chongmo` | 「七年」；虫魔之名；妖丹堆与六七级材料 |
| n26 | `n26_liandan` | 四 | 528–529 | `ch13_wuzhong` | `trigger_danfang` | `ch13.chongmo` | `ch13.liandan` | 炼丹三年；至元丹方解锁 |
| n27 | `n27_biguan` | 四 | 529 | `ch13_wuzhong` | `trigger_jingshimen` | `ch13.liandan` | `ch13.biguan` | **抬上限到 32**；往返自此闭 |
| n28 | `n28_chuguan` | 四 | 529–530 | `ch13_wuzhong` | `trigger_chuguan` | `ch13.biguan` | `ch13.chuguan` | 境界门闸；二十余年；青灵门二人现身 |
| n29 | `n29_gongsun` | 四 | 530–533 | `ch13_wuzhong` | NPC `npc_gongsun_xing` | `ch13.chuguan` | `ch13.gongsun` | 兽潮旧事（`ch13.shouchao`）；至元丹三支；入阵为仆（改编）|
| n30 | `n30_lidao` | 四 | 533–534 | `ch13_wuzhong` | `trigger_lidao` | `ch13.gongsun` | `ch13.lidao` | 弃府离岛（→ `ch13_waihai` 秘市岸）|
| n31 | `n31_mishi` | 五 秘市 | 535–539 | `ch13_waihai` | `trigger_mishi` | `ch13.lidao` | `ch13.jiaohuan` | 交换会：铜片（必得）、换文思月自由 |
| n32 | `n32_wenshi` | 五 | 540 | `ch13_waihai` | NPC `npc_wen_siyue` | `ch13.jiaohuan` | `ch13.wenshi` | 妙音门惊变；巢穴位置 |
| n33 | `n33_jianshi` | 五 | 541 | `ch13_waihai` | `trigger_jiaoshi` | `ch13.wenshi` | `ch13.jianshi` | 铜片是妖修功法；深渊边缘蹲守 |
| n34 | `n34_banyao` | 五 | 541–542 | `ch13_waihai` | `trigger_dongkou` | `ch13.jianshi` | `ch13.done` | 黑色伴妖草；身后人声；章末 |

**主动抉择 15 处**：n03、n04、n05、n06、n07、n11、n12（付费三支）、n14、n24（每轮战中可逃）、n29 两处（回话 / 至元丹从哪出）、n31 三处（铜片怎么付 / 换她自由 / 范夫人之邀）、Z1。**真改状态的 4 类**：`ch13.toudu`、`ch13.zhiyuan`、`ch13.tongpian`、② 逃与不逃；其余只换一句后合流。**必打 2 场**（② 至少胜 2 次才开 n25）。
**主干的一句话**：海上问话 → 南明岛问草 → 攻港 → 盘查 → 追兵 → 入城 → 弃府 → 探阵 → 把柄 → 易容 → 偷传 → 能出不能进 → 收购行 → 千叶露 → 立府 → 玄阴经 → 重炼 → 雷劫 → 诱妖 → 七年 → 三年 → 坐关 → 兽潮旧事 → 秘市 → 文思月 → 深渊边缘 → 黑草与人声。前半章他一路被「封」——岛封、城封、阵封、海封；后半章他第一次有了大把的时间，用来炼剑、养虫、坐关，直到为一株草再次出门。

### 3.1 每一处挂点怎么开始（`mode` / `once`）

分法沿用前十二章：**这件事是他做的（interact），还是发生在他身上的（enter）。** 不用 `auto`（map_spec 的 auto 未实现）。
**挂 NPC 的 8 个节点**（n02、n04、n06、n12、n14、n15、n29、n32）没有 once，脚本开头看 done_flag 判已演过；`once=true` 的触发器 `set_flag` ＝ 主线节点表的 done_flag，**由脚本末尾自己 `flag.set`**（引擎从不替触发器写旗，只拿 `set_flag` 判 once 兑现了没有）。
**每张图的首个 enter 挂点**是 default spawn 前方横贯通道的触发带，不压 spawn（teleport 落地不触发 enter）。**同一格只放一个对象**（同格只取第一个 trigger）。portal / facility 的门闸只用 `require_flag ＋ deny_text_key`（`ch13.block.<短名>`）。

| # | 挂点 | 地图 | `mode` | `once` | `set_flag` | 战斗 | 格位与为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| NPC | n02 `npc_yuanliao_zhanggui`（小城原料铺）、n04 `npc_cang_yunlong`（码头）、n06 `npc_ling_yuling`（出海口外礁头）、n12 `npc_gu_laozhe`（传送大厅）、n14 `npc_ming_furen`（明珠轩）、n15 `npc_li_xun`（寸金阁）、n29 `npc_gongsun_xing`（东岸林边光幕前）、n32 `npc_wen_siyue`（秘市荒岛东侧绿山小洞府前）| 各图 | NPC | — | — | — | 找人说话的是他；在场由 visible / hidden 管（3.0 主表「前置」即 visible_flag）|
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
| 往返 | `trigger_qiyuan_hang` | wuzhong | enter | ✗ | 空 | — | 码头尽头一格，与 n30 的带隔 ≥ 2 格；guard `ch13.lifu` |
| 往返 | `trigger_hui_wuzhong` | waihai | enter | ✗ | 空 | — | 西门外礁头，与 n16 的带错开 ≥ 2 格；guard `ch13.lifu` |
| 设施 | `facility_cunji_{nanming,tianxing,qiyuan,mishi}`、`facility_cunji_wz` | 四图 | 设施 | — | — | — | 各紧挨落点、在首个触发带的落点一侧 |

- **`once=true` 的挂点每一条都写 `set_flag`**；条件不够的路（n25 胜场不足、n28 境界未到、各抉择取消）都在置旗前 `return`，once 不被兑现，再踏 / 再按照常响。`once=false` 的六处（n24、三处链外、两处往返）不写 `set_flag`。两场必打都挂在目标链的必经挂点上（n05；n24 → n25 的胜场门槛），绕不过去。
- **规则 13**：每张图只从 default spawn 出发 BFS。`ch13_waihai` 三区（北黑石城、西南秘市荒岛、东南小岛）靠礁路连通，未到的挂点靠 guard 惰性，不靠隔断；`ch13_wuzhong` 的西海礁与东岸同图连通。脚本落点不算起点。
- **规则 30**：同图同时在场的 role 两两不同、名牌两两不同；`yi_jing` 两个对象分属 `ch13_tianxing`（`npc_yi_jing_tx`）与 `ch13_waihai`（`npc_yi_jing`）。
- **兽潮之后的北区**：六个 NPC 全撤（`hidden_flag ch13.shouchao`），两家店挂在 NPC 脚本上随之没了；不设阻隔带。

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
| 6 | `{"ch13.lifu", "scripts/ch13/qiyuan_hang.lua", "ch13_waihai", "trigger_qiyuan_hang", "ch13_wuzhong"}` | 黑石城西门外礁、`trigger_hui_wuzhong` 东侧两格（坐标）| `ch13.biguan` 已置 → 一句 return |
| 7 | `{"ch13.lifu", "scripts/ch13/hui_wuzhong.lua", "ch13_wuzhong", "trigger_hui_wuzhong", "ch13_waihai"}` | `andeng`（default）| `ch13.lidao` 已置 → 一句 return |

- 五元两条在门闸之后：`choice`「去 / 不去」（取消为 nil 即不去）→ `advance_days`（日数见 3.3）→ teleport。一份脚本拿不到是哪个触发器起的事件，所以两个方向拆两份脚本。
- **落地不触发 enter**：#1–#4 的落点前方都有横贯触发带，通关驱动要先迈一步；#5、#6 落在坐标格，`ObjectiveRoute` 只认目的图、不认坐标（O-11，测试路核）。
- **门钥匙**（`kOffChainDoorKeys`）：本章四张图之间没有一扇门，0 条。

### 3.2 脚本动作与分支

本节只规定状态操作、控制流和门闸，不提供叙事镜头、示例台词或转述顺序；剧情事实与质感禁用项只按 13.1 核验，对白另行原创。挂点属性以 3.1、日数以 3.3 为准（下表 `advance_days` 一律不写数）。**每一处 `take`、`magic.learn`、`realm.cap`、`field.unlock` 都看返回值**；返回 false 的路只换一句，不卡主线。

| 节点 / 脚本 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- |
| n01 wenxun | `magic.learn("magic_ji_wuxinghuan")`；advance_days；置 `ch13.wenxun` | learn false 换一句照走 |
| n02 caigou（NPC）| 首谈：置 `ch13.caigou`；`shop("ch13_nanming_fangshi")` | 再谈：`ch13.gongdao` 未置 → 再开店；已置 → 一句「封市」return |
| n03 gongdao | choice「看下去 / 退回坊市」；置 `ch13.gongdao` | 取消在置旗前 return；两支只换一句 |
| n04 panzha（NPC）| `item.count("story_miaoyin_yaopai") >= 1` 择「出示腰牌」版，否则「报名号、画像对得上」版（不扣）；choice「应下 / 不置可否」；置 `ch13.panzha` | 已置 → 一句 return；取消在置旗前 return |
| n05 zhuibing | choice「随行 / 不随」→ `battle("b13_nanming_zhuibing")`；胜：give `story_mihuan_yan` 1、`story_baiyu_dayin` 1、`material_lingshi` 600；advance_days；置 `ch13.zhuibing` | 两支都打；取消在战前 return；败 game_over |
| n06 yinjian（NPC）| choice「以妙音门之恩婉拒 / 不答」；advance_days；顶格 `flag.set("ch13.yinjian")`、`teleport("ch13_tianxing")` | 两支都不入星宫；取消在置旗前 return |
| n07 rucheng | choice「报名 / 安分」→ 两支都 give `story_huangse_zhihuan` 1；置 `ch13.rucheng` | 取消在给物前 return；① 不抵传送阵条件 |
| n08 qifu | 置 `ch13.qifu` | 药园不由脚本搬作物（玩家经 `facility_yaoyuan_39` 自采）|
| n09 tanzhen | advance_days；置 `ch13.tanzhen` | 无 |
| n10 qianru | advance_days；置 `ch13.qianru` | 无 |
| n11 yirong | choice「直说同去 / 先亮把柄」；advance_days；置 `ch13.yirong` | 取消在置旗前 return |
| n12 chuansong（NPC）| choice 付费，**只列付得起的支**：① `material_lingshi >= 10000` → take 10000；② `material_yaodan_wuji >= 4` → take 4；③ 把柄 ＋ 胡月飞刀两柄（旁白，不扣物，永远在）；`flag.set("ch13.toudu", 1/2/3)`；顶格 `flag.set("ch13.chuansong")`、`teleport("ch13_waihai")` | 取消在扣物前 return；take false 落第三支；意图玩家走 ③ |
| n13 daoqiyuan | 读 `ch13.toudu` 择一句；置 `ch13.daoqiyuan` | 无 |
| n14 mingzhu（NPC）| 首谈：choice「看下去 / 离开」；置 `ch13.mingzhu`；`shop("ch13_mingzhu_shougou")` | 再谈：开收购；Z1 分支：已接单、`ch13.z1` 未置、六级 ≥ 1 → take 六级 1、give `material_lingshi_zhong` 30、置 `ch13.z1` |
| n15 cunjin（NPC）| 首谈：give `story_guyu_yujian` 1；置 `ch13.cunjin` | 再谈：`shop("ch13_cunjin_ge")` |
| n16 chucheng | give `story_qiyuan_haitu` 1；advance_days；顶格 `flag.set("ch13.chucheng")`、`teleport("ch13_wuzhong")` | 无 |
| n17 lifu | advance_days；`field.unlock("field_wuzhong", 6)`；give `herb_nichang_cao` 3；置 `ch13.lifu` | unlock false 换一句 |
| n18 chongshi | give `story_wuse_yuanzhu` 1；advance_days；置 `ch13.chongshi` | `item.count("story_jinsi_can") > 0` 多一段（不扣）|
| n19 xuanyin | take `story_xuangu_leigu` 1；give `story_xuanyin_jing` 1；advance_days；置 `ch13.xuanyin` | take false 换一句照给（不设死局）|
| n20 chonglian | take `material_lianjing` 1；advance_days；`magic.learn("magic_fengyun_heji")`；置 `ch13.chonglian` | take / learn false 各换一句照走 |
| n21 dujie | advance_days；置 `ch13.dujie` | 无 |
| n22 wusezhu | take `story_wuse_yuanzhu` 1；`local n = item.count("story_jinsi_can"); if n > 0 then take("story_jinsi_can", n) end`；advance_days；置 `ch13.wusezhu` | 无 |
| n23 buzhen | advance_days；置 `ch13.buzhen` | 无 |
| n24 youyao | `ch13.biguan` 已置 → 一句 return；草 `herb_nichang_cao < 1` → 一句（指寸金阁）return；`bottle.drops < 1` → advance_days（缺液）return；否则 `bottle.spend(1)`、advance_days、`battle("b13_waixinghai_lieyao")` | 胜：take 草 1、`flag.set("ch13.lieyao", flag.get("ch13.lieyao") + 1)`；逃：不扣草；败 game_over；妖丹由编成 `drops` 给 |
| n25 chongmo | advance_days；give `material_yaodan_liuji` 6、`material_yaodan_wuji` 10、`story_liuqi_cailiao` 1；置 `ch13.chongmo` | `ch13.lieyao < 2` → 一句 return（不置旗）|
| n26 liandan | advance_days；置 `ch13.liandan`（解锁 `recipe_zhiyuan_dan`）| 炼不炼由玩家，不设门闸 |
| n27 biguan | advance_days；`local ok = realm.cap(realm.CORE_MID)`；置 `ch13.biguan` | cap false 换一句照走；提示静室在旁 |
| n28 chuguan | advance_days 两段；置 `ch13.chuguan` | 开头 `not realm.at_least(realm.CORE_MID)` → 一句 return（不置旗、不拨日）|
| n29 gongsun（NPC）| choice 回话；choice 至元丹从哪出（**只列有的**）：① `pill_zhiyuan_dan >= 3` → take 3 记 1；② 六级 ≥ 1 → take 1 记 2；③ advance_days（开炉一月）记 3（永远在）；advance_days；末尾依次置 `ch13.shouchao`、`flag.set("ch13.zhiyuan", n)`、`ch13.gongsun` | 取消在扣物、置旗前 return；take false 落第三支；`shouchao` 必在 `gongsun` 之前 |
| n30 lidao | advance_days；顶格 `flag.set("ch13.lidao")`、`teleport("ch13_waihai", 14, 46)` | 无 |
| n31 mishi | `shop("ch13_mishi_tan")` 一次；advance_days；铜片三支（只列付得起的）：五级 ≥ 1 → take 1 记 1 / 灵石 ≥ 5000 → take 5000 记 2 / 以桌上材料抵（旁白不扣）记 3；give `story_tongpian` 1；choice「以整堆材料换她自由 / 再看看」；take `story_liuqi_cailiao` 1；choice 范夫人之邀；`flag.set("ch13.tongpian", n)`；置 `ch13.jiaohuan` | 取消在扣物前 return；「再看看」回选单再给一次，第二次只剩「换」；take false 换一句照走；范夫人两支都是回绝 |
| n32 wenshi（NPC）| advance_days；置 `ch13.wenshi` | 已置 → NPC 已撤（`hidden ch13.wenshi`）|
| n33 jianshi | 读 `ch13.tongpian` 择一句；advance_days；置 `ch13.jianshi` | 无 |
| n34 banyao | give `story_banyao_cao` 1；末尾置 `ch13.done` | 不 `ending()`、不 teleport；人声一句之后无动作 |
| Z2 xuanshang | 已接（`ch13.daoqiyuan`）、`ch13.z2` 未置、五级 ≥ 3 → take 3、give `material_lingshi_zhong` 15、`herb_nichang_cao` 3、置 `ch13.z2` | `ch13.shouchao` 已置 → 一句 return；不足一句 |
| Z3 yanghun | 已接（`ch13.lifu`）、`ch13.z3` 未置、`bottle.drops >= 3` → `bottle.spend(3)`、advance_days、take `story_yanghun_mugen` 1、give `story_yanghun_zhu` 1、置 `ch13.z3` | 绿液不足一句；`ch13.lidao` 后不可达（不另判）|
| 往返 qiyuan_hang / hui_wuzhong | choice 去 / 不去 → advance_days → teleport（3.1 #6、#7）| 门闸见 3.1；脚本 0 处 `flag.set` |

- 只用既有 Lua 接口（本章 0 个新命令）；`realm.advance` / `realm.demote` / `party.*` 0 处；不从脚本发修为。
- **意图玩家**（两侧同一组选项，写死在通关测试）：n02 不买；n03「看下去」；n04「应下」；n05「随行」；n06「婉拒」；n07「报名」；n11「直说」；n12 第三支；n14「看下去」；② 胜 2 次、不逃；0 次往返、0 条可选支线；静室按「闭关一年」直到冲成；n29 第二支（六级 1）；n31 铜片第一支、「以整堆材料换」、回绝范夫人。

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
| **窗口：静室打坐冲中期（玩家定，意图玩家 M ≈ 1960–4900）** | 面板 | 4381 ＋ M |
| n28 二十余年（两段）| 3600 ＋ 3600 | 11581 ＋ M |
| n29 两日后 | 3 | 11584 ＋ M |
| n30 | 20 | 11604 ＋ M |
| n31 三日后、交换会 | 5 | 11609 ＋ M |
| n32 | 1 | 11610 ＋ M |
| n33 一月到深渊边缘、蹲守数月 | 180 | **11790 ＋ M** |

**不计入意图玩家的**：n24 缺绿液 5；往返每趟 12；n29 第三支开炉一月 30；Z3 60。
**按幕小计**：一 18（7 min，① 约 3）／ 二 3（8 min）／ 三 14（5 min）／ 四 11569（15 min，② 两轮约 5、打坐面板约 1）／ 五 186（5 min）＝ 11790 ＋ M，**40 min**。全章约 33–46 年（原著 ch511→ch542 约 48 年：一年炼剑、七年猎妖、三年炼丹、十五六年坐关、二十余年第九层）；**文案只写相对年数**，不写具体日数。
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

文案进 `data/text/ch13_objectives.json`（key `ch13.objective.<step id>`）。第 24 步的 done_flag 是计数旗，按 ≥ 1 记一步；第 25 步的提示不写具体次数（门槛 `>= 2` 只在脚本里）。提示文案的禁词与章内先后照 M 表：第 2 步只许「千叶露」、不许「伴妖草」（`ch13.cunjin` 前）；第 30 步起「伴妖草」合法。`ObjectiveRoute` 沿这条链逐步走（四张新图之间全靠脚本 teleport 串，3.1 末）。**本章新增的按键提示：0 条**。

