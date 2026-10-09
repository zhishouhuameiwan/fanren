# 第 7 章设计：血色试炼

> 对应原著 ch152-216。目标时长 70 min（A 主线 + B 必经口径）。
> 权威依据：`H:/Work/Kys/docs/大纲.md` 第 7 章条目；大纲与原著冲突之处以原著为准，待回写（本稿查出四处，见 1.2）。
> 本文是编剧、关卡、引擎三路的共同施工图；引擎增补的字段、接口、判据另见 `docs/interfaces-p3-ch07.md`。
>
> P3 · 2026-09-29 · **施工图阶段：本文不改任何代码、脚本、数据、地图，也不碰 `data/flags.json`**（旗标清单见第 11 节，交协调者登记）。
> 第 16 节的拍板项**已按推荐定下**（用户 2026-09-29 预授权：一路推进到第 10 章、遇事按推荐走）；与前几章已拍的板冲突的，一律维持旧板，冲突写在 16.2。
> **起点依赖第 6 章**：第 6 章正在施工；`tests/fixtures/ch06-end-*.sav` 是第 6 章测试路刚写出、尚未集成的版本。第 2 节起点表从这一版读出、与第 6 章设计的推算对过；以 ch06-end fixture 为准，fixture 落地后协调者逐字段核。本章在第 6 章集成之后开工。

---

## 0. 这一章要做到什么

1. **他第一次有钱，却买不到筑基。** 两年药园是掌天瓶经济的收获期：上交的药一夜催熟、月例一涨再涨，园角两株千年灵草在万宝楼换回一整套顶级法器（第 9 节）。
   可筑基丹要的三味主药只长在禁地里，钱在这一章第一次**不够用**——这是全章的动力。
2. **修仙界的规矩只有一条：弱肉强食。** 陆师兄为一粒筑基丹要杀自己的女伴；禁地里人人杀人夺宝；结丹师祖拿弟子的命下注；师傅收徒是为了抽徒弟一半灵药。
   韩立在这一章里也杀人灭口、也藏、也骗——但他始终没对女人下死手（ch171、ch203）。**生还率不足四分之一**，气氛要压得住（3.2 幕三的节奏写法）。
3. **他赢在一个只有小瓶才成立的算计：采别人不要的幼苗**（ch196）。别人拼命抢成熟灵药，他专挑刚被采过、只剩幼苗的地方；嗅灵兽只闻得出百年以上的药（ch210）——幼苗留在他手里，三年后催成四五百年，炼出二十几颗筑基丹。
4. **筑基不在禁地。** 出禁地之后是三年备药、地火屋里半年炼丹、五个月服丹冲关（ch213-216）。章末境界跳到**筑基初期**。
5. **南宫婉在本章起、在本章收。** 茅草地里那一眼是暗场（ch182），地下沼泽里是一场意外（ch205-208），出禁地时她自始至终没看他一眼（ch211）。本章之后她不在场（第 5 节）。
6. **借来的威能用完就没了。** 剑符在斗陆师兄时耗尽自燃（ch170），金光砖在打穿地面时耗尽（ch209）——两门「祭 X」都在当场 `magic.forget`（第 10 节 E5、验收 7）。

---

## 1. 硬约束（原著核对）

章号由我用 `tools/novel/novel.py` 亲自核过（2026-09-29）：**ch152-216 逐章通读**（65 章，约 17.9 万字；逐章摘要在会话草稿 `scratchpad/ch07/digest.md`，不进仓库）；
后续人物下场抽读 ch217-218、242、244-245、262、265-269、273-274、314-316、338、345、356、359-363；专名首见逐个 `find`（第 12 节那张表的每一行都是这样来的）。

### 1.1 硬约束表

| # | 约束 | 出处 |
| --- | --- | --- |
| 1 | **开篇两年多**：叶师叔送来的东西里有两块中阶灵石（火、土）与三件法器——精钢指环（驱动后飞出锁敌、可放大）、黑色三角小旗（挥出黑雾遮目掩踪）、**黄铜瓶**（保灵力不失；装绿液只从一刻钟延到一整天，过时照样消失）| ch152 |
| 2 | 两年里：绿液配黄龙丹、金髓丸当糖豆吃（游戏里就是养精丹，第 6 章口径）；上交的药一夜催熟；马师伯**每数月**来收一次药，月例从**每月 2 块涨到 5 块**（普通弟子平均 3 块）；常去吴风处学初级法术口诀 | ch152 |
| 3 | 十年一次大开山门：慕容兄弟（雷灵根）、十一二岁九层顶峰的李姓少年、七八岁有玄阴之眼的王家童子；掩月宗收了个天灵根小姑娘。韩立被忘得干净。叶师叔的侄孙服了筑基丹没成 | ch152 |
| 4 | **两年连破两层**：九层 → **十一层**；此时黄龙丹、金髓丸彻底无效——要新丹方 | ch152 |
| 5 | **岳麓殿**在巫钧山：未筑基者要担保人信物；担保人马师伯（守关的两名筑基红衣弟子称他马师兄、痴迷炼丹），代价是**白看一年药园**；传送阵每趟收低阶灵石一块，头一回免 | ch153 |
| 6 | **许老**（筑基，不许人叫师伯——好明着要钱）：藏室二楼一个时辰一块、复制一份十块；楼上只有世俗医书、几捆同档丹方与两枚玉筒——**筑基丹方（须先天真火，筑基后才有：死循环）、定颜丹方（只驻颜；不要真火）**；两份复制 20 块；大半丹方随天材地宝绝迹而失传 | ch154-155 |
| 7 | **地肺之火**：门派的玄阳火地早就替代了真火，**岳麓殿那条无标记通道通往火地**，交灵石借用；买最小的**银丝鼎** 32 块（储物袋只装得下它）；在许老处共花五十几块；石门旁疙瘩脸的**丑汉**当面爱答不理、背后骂他穷鬼 | ch156 |
| 8 | 回程山头：慕容兄弟演雷法；**陆师兄**（风灵根，原本低阶弟子里唯一的异灵根，心眼小、记仇）与女伴**陈师妹**；陆的青弧斩追人，一少年扑向韩立，他闪开；另一少年躲到粗矮青年身后；蓝衣女子以火焰鸟叫停，临走**传音责他独善其身** | ch157-158 |
| 9 | 筑基丹方：辅药三十一味园里都有、要数百年火候；**主药玉髓芝、天灵果与紫猴花**园里一株也没有；马师伯：天生自长、**没有种子**、幼苗离了原地难活，不肯说出处 | ch158-159 |
| 10 | 定颜丹：药材常见，但动辄**千年**以上药性，所以少有人知；他打算筑基丹之后再试（**本章没炼**，ch243 才炼七八颗）| ch159、243 |
| 11 | 吴风（服过筑基丹仍在炼气顶峰；对谁都倾囊相授）讲禁地：风属性古禁**每五年有五天衰弱期**，数名结丹合力可开；**只有炼气期进得去**；近百年活着出来不足三分之一，重赏之后**不足四分之一**；报名先赐中阶灵石一块、灵器一件，带出灵药按量按质重赏，最高可换筑基丹；**下一次在半年后** | ch160 |
| 12 | 决心；**敛气术**（中阶辅助，对抗天眼术：不被肉眼看见即可敛气；对筑基无用）苦修**四个多月**；同时在园角偏僻处（错开马师伯来的日子）催出**两株千年灵草**，只卖外来修仙者；向于执事领外出令牌（每年一次）| ch161 |
| 13 | **坊市**在太岳山脉东北缘，元武国修士常来，五里内禁飞；一条南北街：南段店铺（七巧阁、引风斋、天工楼、万宝楼、五行书店、酒楼客栈），北段临时摊位（交一块低阶灵石摆一天、受保护）；他换灰布衫、戴青斗篷、化名「厉飞雨」| ch161-162 |
| 14 | **万宝楼**：掌柜田卜离（凡人）；锦盒四件——金蚨子母刃（母一子八）、玄铁飞天盾（放大后自动绕身防御）、天雷子（筑基硬抗也化灰）、金光砖符宝；鉴定人丁老断为新出土的千年黄精芝；**两株千年灵草换下全部四件**；田掌柜求以后优先卖他——他因此认定这种买卖少做 | ch162-165 |
| 15 | **符宝**：结丹以上把法宝至多十分之一的威能封进符纸；谁都能用，筑基前只发挥一两成；用一次耗一次，耗尽作废 | ch164 |
| 16 | 绕路三四天防跟踪，三日后傍晚宿太岳山脉外围一个石洞；**陆师兄（十二层中阶）**制住陈师妹要灭口夺丹（她是陈家家主独生女；他为攀高枝、也为她那粒筑基丹）；陆早察觉了他 | ch165-166 |
| 17 | **恶斗**：他一言不发先出手；陆的高阶风墙符挡住钢环、黑球、火球；他摆三层防御（钢环、飞天盾、水罩）驱动剑符；青蛟旗风刃击碎钢环、再**化蛟**；剑符化两三丈巨剑与蛟相持——比法力：中阶灵石、生嚼年份草药、撤掉水罩省法力（符箓激发后仍有灵线耗法力，吴风说过）；陆临死一记风刃，罗烟步避开，**陆被斩成两半** | ch167-169 |
| 18 | **剑符返回时自燃成灰——报废**；钢环粉碎；得青蛟旗、青色绳索、银钩、数十张低中阶符箓、低阶灵石二十多块、**两粒筑基丹**（装进黄铜瓶）| ch170-171 |
| 19 | 陈师妹服了迷药、神志不清、记不住他的脸；他用定神符制住她、没碰她（马师伯说过元阳之体筑基机会大）；焚尸、刮乱痕迹，带她往西百余里放下，喂清灵散解药毒；回园闭关三日，元气大伤约一月 | ch170-171 |
| 20 | 筑基丹直接吞服，但须**闭关三个月**化药——与参加试炼不可兼得；上层宣布**禁地于五年之后封闭六十年**（三四百年一次的临时圈封）→ **五年后那一届必然精英尽出、极度血腥**，他改赶这一届；六十年后又过了筑基的好年纪 | ch171-172 |
| 21 | 报名的是**王师叔**：九层到十一层太快，拉到一边再测，仍是伪灵根；他编了个幼时误食「龙鳞果」的说法，王半信半疑 | ch172-173 |
| 22 | 卸下药园差事时**马师伯丢给他两瓶丹药**（一瓶内服、一瓶外敷），面冷心热 | ch173 |
| 23 | 本次黄枫谷满额 **25 人**：十三层顶峰五六个，多为十二层（陈师妹十二层中阶），**十一层只三人**（他、白发老者、少年）；陈师妹变得冷艳、认不出他；陆师兄按失踪了事 | ch173 |
| 24 | 议事大殿：中阶灵石任取一块（他取水属性）、从隔绝神识的袋里摸一件上品灵器；**李师祖**（结丹，方脸虎目、性子刚烈）带队，王师叔等五名管事同行；**银甲角蟒**两天两夜飞到建州北一座荒山 | ch174 |
| 25 | 荒山：向之礼拉十一层的结伙，他拒；清虚门乘雪虹绫到，**浮云子**与李师祖赌（先比灵药多寡、再比质量、最后比活着出来的人数）；**穹老怪**插手：两家合计胜过掩月宗就算他输；李师祖当众讲**正与邪**、七派中立的来历，许诺贡献最大者筑基后收入门下；掩月宗乘天月神舟到（**霓裳仙子**领队，一对对年轻男女、没有老人）；巨剑门、灵兽山、化刀坞、天阙堡 | ch174-178 |
| 26 | 卖金竺笔的少女入了灵兽山、在队中，被一个十三层的**络腮胡子**呵斥；他冲胡子做鬼脸 | ch179 |
| 27 | **破禁**：飞数个时辰到一片黄土坡；巨剑门高人掷石剑试禁（无边风刃之墙），每隔约一个时辰一次，第四次明显减弱；七件法宝合力三四个时辰打出丈许通道；入内即被挪移阵随机送走 | ch179-180 |
| 28 | **乌龙潭**（禁地东北角）：寒烟草、一级下阶寒冰蟾；天阙堡弟子被灵兽山二人借寒冰蟾偷袭杀死，他敛气旁观；山崖下巨剑门与黄枫谷弟子同归于尽，他拾得**透明丝线法器**（茅草地里其实藏着白衣少女——他不知道）| ch180-182 |
| 29 | **一线天**：禁地里御器飞行等于当靶子；络腮胡子与天阙堡严姓用高阶**融灵符**同落一处、专杀人夺宝；**土牢符**困住胡子、**丝线**隔空取严的首级、**金光砖**（先吸走他三分之一法力）拍死胡子 | ch183-184 |
| 30 | 第一日后禁地里只剩**七十多人**；禁地之夜亮如白昼、天灰濛濛；第二日早：多宝女（**掩月双娇**之一，祖母是掩月宗结丹长老）追杀黄衫师姐，他出手；**封岳**（天阙堡狂人，**踏云靴**）以小刀符宝杀多宝女、杀师姐；飞天盾硬扛，**天雷子**藏在混元珠后炸得封岳化灰；得踏云靴、青凝镜、水晶球、小刀符宝 | ch185-190 |
| 31 | 中心区：四扇青铜门；墙上钉着三具尸（化刀坞寒天涯所为）；灵兽山**钟吾**飞蛇偷袭、认出踏云靴；互换玉简资料；中心区三层（花园、终年浓雾的**环形山**、百丈巨塔）；**月阳宝珠**七派轮掌，本次天阙堡执掌，**第三日早上**驱雾 | ch191-195 |
| 32 | 他的计划：**只采幼苗**，出去用绿液催熟（主药四五百年即可入药；幼苗离地能活一两年）；紫猴花洞以倒插的子刃暗算上阶**巨蜈蚣**；此后四处都没有守护兽 | ch196-200 |
| 33 | 小石殿：巨剑门赤脚大汉（武痴，**银辉剑**）逼灵兽山十层的少女让出烈阳花；他救人、斗剑，**青凝镜**定住银剑，十丈之内丝线取首级；青凝镜是掩月双娇之物、泄露必遭长老追杀 → **无忧针法＋忘尘丸**抹去她半日记忆；她自报名字**菡云芝** | ch200-203 |
| 34 | 第四日：青石殿 → 地道 → **地下沼泽**（金箱、数十株灵药）；掩月宗众人与「师祖」（结丹期的白衣少女，法力只剩炼气顶峰，法宝朱雀环）斗**墨蛟**；墨蛟蜕皮进二阶（可比筑基中阶）、紫液化人；赵姓女弟子的**小五行符**封死通道（暗场）；他出手，金光砖击杀墨蛟；她收蛟元神 | ch204-207 |
| 35 | 墨蛟淫囊的催情雾（**游戏暗场**）；醒后她外貌成十八九岁：修**素女轮回功**，此事只当一场梦、外泄即杀；金箱归她、灵药归他；她暂传法力，两人打穿地面——**金光砖耗尽**、她损二三十年功力；他问来她的名字「**南宫婉**」 | ch207-209 |
| 36 | **第五日下午**出禁地：黄枫谷活着五人（别派多是三四人，巨剑门只剩两人）；掩月宗最后一刻钟十余人整队出来；向之礼最后一个爬出、通道随即碎灭（没按时出来的从未再出现过）；**嗅灵兽**三丈内闻得出百年以上灵药；他交出二十几株；掩月宗赢赌局；**南宫婉自始至终没看他一眼**；菡云芝离开时对他一笑 | ch209-211 |
| 37 | 李师祖问名、问入谷几年（**近三年**）；收他为**记名弟子**，赐**碧光刀**（顶级法器），筑基成了再收正式弟子；**谢师礼**：师傅可抽徒弟上交的一半，他的奖赏只剩一枚筑基丹；马师伯改口让他叫师兄，他坚持没筑基仍叫师伯 | ch211-212 |
| 38 | **三年**：备齐辅药；陈师妹服奖赏的筑基丹、一年后筑基；她大哥第二枚仍失败、回家族；叶姓老者把拖欠的一股脑送来还多出；李师祖派人送来亲笔誊写、署名的**《青元剑诀》**，此后三年无音讯 | ch213 |
| 39 | **地火屋**：丑汉认得李师祖笔迹；中阶灵石作定金（公认一百低阶兑一块，但没人愿换）；坞石通道、三十六间地火间，**十九号**；八龙首喷紫火、四葫芦火星砂；二十几炉凝丹不成，其后约三次成一次、取丹过半；**半年得二十几颗**；辟谷丹一粒撑一月 | ch213-215 |
| 40 | **就地服丹**：第一粒到十二层、第二粒到十三层、第三到第七粒真元化液、**第八粒昏死中筑基**；残余药力要功法吸纳——手上只有《青元剑诀》（九层；前三层掌发剑芒、中三层护体剑盾）；在屋里共**十一个月**；出来时丑汉改口称师叔 | ch215-217 |

### 1.2 大纲与 lore 里对不上的地方（已核，待回写）

| 在哪 | 原说法 | 复核 | 本章以什么为准 |
| --- | --- | --- | --- |
| 大纲第 7 章「紧迫感」 | 这回之后禁地封六十年，韩立赶上的是「**封闭前最后一次**」 | **错**。ch172 宣布的是**五年之后**起封闭六十年：封闭前最后一次是五年后的下一届；韩立怕下一届精英尽出，才改赶**这一届**（倒数第二次）| 3.2 节点 15 照原著交代：「五年后封、那一届会更惨、这一届是他唯一的机会」。**回写大纲**（16.1 第 1 条）|
| 大纲地图「闭关洞府」 | 列为本章地图 | 原著本章**没有洞府**（ch219 起才置办）；筑基就在地火屋十九号 | 不做洞府图；回写 |
| 大纲主线「地火屋半年炼丹、近一年闭关」 | 读作半年＋近一年 | 原著：在地火屋里**共十一个月**＝约半年炼丹＋约五个月服丹冲关（ch215-216）| 3.3 日历按 180 + 150；回写 |
| 大纲人物表 | 南宫婉、李化元、陆师兄、七派同行弟子 | 漏了本章戏份很重的：陈师妹（即陈巧倩，**本章只称陈师妹**，点名在 ch273）、马师伯（担保人）、许老、吴风、王师叔、菡云芝（ch203 具名）、钟吾、封岳、浮云子、霓裳仙子、穹老怪、向之礼 | 第 5 节全列；回写 |
| lore 人物「钟吾」 | 「结局章 ch199（与人混战）」 | **错**：ch209 活着出禁地，第 8 章 ch265-269 再出场并称韩立前辈 | 本章出禁地时在场 |
| lore 时间线第 20 条 | 百药园两年「十层上下」 | ch152 两年里已连破两层到**十一层** | 1.3 |
| lore 功法表「女轮回功 ch208」（`人物总表.md` 南宫婉一行同）| 名称 | 原文全名**素女轮回功** | 文案用全名 |
| lore 数字设定矛盾清单第 1 条 | 五年一开 / 六十年 / 三四百年三处「互不一致」 | **不矛盾**，是同一机制的三层：常规每五年五天衰弱期（ch160）；每三四百年临时圈封一次（ch172）；本次宣布五年后起封六十年（ch172）| 回写数字设定表 |
| `docs/ch05-design.md` 10.3、`ch06-design.md` 3.2 节点 9b | 「第 7 章 **ch171** 剑符报废时收回」 | 报废是 **ch170**（返回时自燃成灰），ch171 是清点损失 | 收回写在节点 13 战后（ch170）|
| `docs/ch06-design.md` 10.1 E2、16.3 第 9 条 | E2 放第 7 章，理由写的是 **ch242**「他真正学制符」 | ch242 在第 8 章区间；但用户拍的是「放第 7 章」 | **维持旧拍板**：E2 在本章做（第 10 节），冲突记 16.2 第 1 条 |
| `docs/ch06-design.md` 第 5 节 | 卖符少女「原著无名」 | 太南会时无名；**ch203 她自报菡云芝** | 本章 ch203 之前沿用 `maifu_shaonv`，之后换 `han_yunzhi`（第 5 节）|
| `data/roles/lu_shixiong.json` | 炼气十一层、火属性、火球术 | 原著**十二层中阶**（ch166）、**风灵根**（ch157）| 改写（第 8 节 ①；RealmTests 锚点同步，10.1 E7）|
| `data/battles/b07_zhaoze_shouyao.json` | 「沼泽妖兽 ×3 … 月阳宝珠驱瘴、与南宫婉同逃出地下沼泽」| 地下沼泽里是**墨蛟**（ch205-207）；月阳宝珠驱的是环形山的雾（ch195），不在沼泽 | 改写成 ⑤ |
| `data/battles/b07_zhongxinqu_duoyao.json` | 清虚门高徒＋化刀坞弟子 | 原著没有这一场（清虚门与化刀坞那一架是韩立不在场的暗场，ch182）| 改写成 ④（小石殿赤脚大汉）|
| `data/recipes/alchemy/zhuji_dan.json`、`dingyan_dan.json` | 筑基丹方要五十年黄精；定颜丹方要六十年清风草 | 黄精、清风草上限 44 年——**两张方子现在根本配不出来**；原料也与原著不合 | 按原著重写（第 7 节）|

**本章不回写、只提醒的**：大纲第 8 章条目「修青元剑诀」起点接本章章末（ch217 马师兄讲剑诀缺陷），第 8 章设计请从 ch217 起算。

### 1.3 境界曲线与上限

- **原著**：进本章时九层初期（ch147）；两年连破两层到十一层（ch152）；此后十一层无丹药不可能再破（ch161），一路十一层进出禁地（ch173、183）；地火屋里第一粒筑基丹到十二层、第二粒十三层、第八粒筑基（ch216）。
- **游戏**：起点**九层 / 上限九层**（第 6 章章末，fixture 读出）。
  - 节点 3（第二年交药之后）`realm.advance(11)`，上限随之到十一层——此后面板冲不过十一层，与 ch161 对得上。
  - 节点 36（地火屋服丹）依次 `realm.advance(12)`、`realm.advance(13)`、`realm.advance(21)`；**章末筑基初期 / 上限筑基初期**。`realm.cap` 本章 0 处。
- 数值（`rules::realmMaxHp` 等，写测试时读表，不抄数）：十一层气血 24 + 12 × 11 = **156**、法力 **110**、攻 20、防 11；筑基初期 **260 / 180**、攻 32、防 20（**筑基档是 Realm.h 标明的暂定值，本章没有筑基期的仗，标定留给第 8 章第一场**）。
- 对手：陆师兄十二层、络腮胡子十三层、严姓十二层顶峰、封岳十三层顶峰、赤脚大汉十三层（估：原著只说不在封岳之下）、墨蛟二阶（叙事上可比筑基中阶，**数据写炼气十三层**——跨大境界压制系数 0.7 / 1.6 会让 ⑤ 失控，第 8 节）。
  **本章打赢靠法器与算计，不靠境界**：他比每个对手都低一到两层。

### 1.4 第 6 章留下的线头

| 线头 | 原著本章有没有下文 | 本章怎么交代 |
| --- | --- | --- |
| 七星草种子（`material_qixingcao_zhongzi`，`ch06.zhongzi`）| **全书只在 ch137-138 出现**，此后再无一字 | 不用、不收回；物品 note 写明（16.1 第 14 条）|
| 法宝残片盖着小瓶（ch151）| ch140 之后全书不再提残片；小瓶照旧每夜凝液 | 节点 1 旁白一句「残片还盖在瓶上，夜里取液、白天盖回」；不做「瓶不在身上」的机制（第 6 章已定）|
| 叶师叔悔诺（`ch06.huinuo`、`ch06.rangdan`）| ch152 叶的侄孙没筑基成；ch213 韩立拜在李师祖座下（记名）之后，叶把拖欠的**一股脑送来还多出** | 节点 3 旁白说侄孙的事；节点 34 叶补还（按两面旗标各有一句，东西一样：中阶灵石 5、回气丹 2）|
| 吴风（`ch06.wufeng` 赢 / 输）| ch152、159-161、170-171 都在；**ch172 以后原著不再出场** | 节点 8、15 两场；开口第一句按 `ch06.wufeng` 分（赢：「那天的路数，我还记着」；输：「底子补上来没有」）；他的 NPC 留在传功阁 |
| 马师伯 = 马师兄（`ma_shibo`）| ch153 守关人称马师兄；ch212 他让韩立改口 | 沿用 `ma_shibo`，名牌「马师伯」不改（韩立没筑基前一直这么叫）；**本章不摆他的 NPC**（第 4 节「马师伯只用挂点」）|
| 卖符少女（`maifu_shaonv`）| ch179 入了灵兽山；ch203 自报菡云芝 | 第 5 节 |
| 祭剑符常驻（`magic_ji_jianfu`、`talisman_jianfu`）| ch168-169 用、ch170 报废 | 节点 13 战后 `take` + `magic.forget`（验收 7）|
| 灵田 `field_baiyaoyuan`（6 槽）| 百药园经营 | 本章第一块用；节点 1 另开园角两槽 |
| 升仙令、储物袋、青叶法器、烈阳剑、冷月刀、玉牌、木牌 | 升仙令 ch152 提一句；烈阳剑冷月刀 ch161 说是最低级法器 | 不动；飞行一律叫「御器」|

---

## 2. 起点：从第 6 章终局接过来

**规矩**（`docs/ch04-review.md` 附录 D）：本章通关测试的起点只能是 `tests/fixtures/ch06-end-{first,second}.sav`。**两份是第 6 章测试路 2026-09-29 07:05 刚写出的版本，第 6 章还没集成**；下表是我从这一版逐字段读出来的（标「读」），与 `docs/ch06-design.md` 的推算对过、没有冲突。**以 ch06-end fixture 为准，fixture 落地（第 6 章集成）后协调者逐字段核**（核出差异改本表与第 9 节第 1 条、不改规则）。

| 字段 | first / second | 读 / 估 | 本章怎么用 |
| --- | --- | --- | --- |
| `realm` / `realmCap` | 9 / 9（两侧同）| 读 | 节点 3 → 11 / 11；节点 36 → 21 / 21 |
| `maxHp` / `maxMp` | 132 / 90，满（两侧同）| 读 | 十一层 156 / 110；筑基 260 / 180 |
| `learnedMagics` | 火弹术、御风决、天眼术、流沙术、冰冻术、祭剑符（6 门，两侧同）| 读 | 章末恰 8 门（验收 2）|
| `party` | 空 | 读 | 全章无同伴（⑤ 的白衣少女是编成友军，不入队）|
| `material_lingshi` | 112 / 82 | 读 | **不拉平**：本章第一笔花销（许老 54）之前还有两笔月例，最低也有 166（第 9 节）|
| `pill_yangjing_dan` / `pill_jinchuang_yao` | 1 / 1；4 / 5 | 读 | 节点 17 马师伯补 |
| 符箓 | 火球符 4 / 3、护身符 2 / 1、金刺符 1 / 0、定神符 1 / 0 | 读 | 火球符、定神符本章可进战斗（E2）；节点 13 的定神符有则扣（second 侧没有，改用定神术的说法）|
| `talisman_jianfu` | 1 / 1 | 读 | 节点 13 收回 |
| `bottle` | owned、matureKnown、3 / 3 滴、capacity 3 | 读 | 药园经营；节点 36 容量 → 6（E3）|
| `fields` | `field_baiyaoyuan` 6 槽，全空 | 读 | 节点 1 另开 `field_baiyaoyuan_jiao` 2 槽 |
| 年份药材 | 零年土骨花 1 / 2；**没有黄精、紫参** | 读 | 不依赖；节点 1 发苗 |
| 清灵散 | 0 / 0（first 有清毒散 2；second 有失心散 2 与两样毒水）| 读 | 节点 14、26 的清灵散一律「有则扣」|
| `alchemyProficiency` / `talismanProficiency` | 100 / 0（两侧同）| 读 | 筑基丹方难度按 100 定（第 7 节）|
| `aptitude` | 50 | 读 | 同上 |
| `knownWeaknesses` | 第 3–6 章敌人 | 读 | 本章敌人全新 |
| `day` | 1697 / 1699 | 读 | 3.3 从此起算 |
| `mapId` / `position` | `ch06_baiyaoyuan` (10,19)，药田角落（`trigger_maiping` 旁）| 读 | 节点 1 就在园里 |
| 旗标 | `ch06.done`；`ch06.huinuo` 1 / 2、`ch06.rangdan` 1 / 2、`ch06.wufeng` 1 / 1、`ch06.zhongzi` 1 / 无；`story.xiuxian_known` | 读 | 本章读 huinuo、rangdan、wufeng（wufeng 两侧都是 1，「输」那一句只在手玩时出现）|
| 七星草种子 | 1 / 0 | 读 | 不用（1.4）|

两侧不同的字段（灵石、符箓、毒药、`ch06.*` 分支）**本章不许成为分岔的原因**：只影响台词与一笔补还的说法（1.4），账在节点 3 之后两侧同型（第 9 节）。

---

## 3. 主线节点（四幕、36 个节点）

| # | 节点 | 地图 | 玩法 | 出处 | 走掉的日子 | 字数 |
| --- | --- | --- | --- | --- | --- | --- |
| **幕一** | **百药园两年** | | | | | |
| 1 | 拆包：叶师叔那一包、黄铜瓶试液、马师伯的苗 | 百药园 | 领物；开园角两槽 | ch152（承 ch151）| 0 | 500 |
| 2 | 头一年：马师伯来收药 | 百药园 | **药园经营**：种、打坐等绿液、催熟、交药（四十四年黄精 ×3）| ch152 | 玩家定（≈ 400）| 400 |
| 3 | 第二年：月例五块；十一层；问方；担保 | 百药园 | 交药（百年紫参 ×2）；`realm.advance(11)` | ch152-153 | 玩家定（≈ 300）| 500 |
| 4 | 岳麓殿：传送阵；许老；藏室两枚玉筒 | 岳麓殿 | 付灵石；**配方解锁**（定颜丹方、筑基丹方）| ch153-155 | 0 | 950 |
| 5 | 地肺之火；银丝鼎；丑汉 | 岳麓殿 | 买鼎；石门前问规矩 | ch155-156 | 0 | 450 |
| 6 | 山头：慕容兄弟与陆师兄 | 黄枫谷 | 观看；蓝衣女子传音 | ch157-158 | 0 | 400 |
| 7 | 读方；马师伯：没有种子 | 百药园 | 叙述 | ch158-159 | 6 | 400 |
| 8 | 吴风：禁地与血色试炼；几夜未眠；敛气术 | 黄枫谷·传功阁 | 叙述；学敛气术 | ch159-161 | 3 | 650 |
| **幕二** | **备战与坊市** | | | | | |
| 9 | 园角两株千年灵草 | 百药园 | **药园经营**（黄精芝推到千年 ×2）；支线 Z1 | ch161 | 玩家定（≈ 130）| 250 |
| 10 | 外出令牌 | 黄枫谷·百机堂 | 叙述 | ch161 | 0 | 150 |
| 11 | 坊市：北口、摊位；万宝楼 | 坊市 | 逛摊、收药摊；**以物易物**（两株千年灵草 → 四件）；二选一 | ch161-165 | 0.5 | 1150 |
| 12 | 绕路；山洞过夜 | 坊市 → 山洞 | teleport | ch165 | 6 | 100 |
| 13 | 夜遇：陆师兄 | 山洞 | 二选一；**战斗①**；剑符报废 | ch165-170 | 1 | 650 |
| 14 | 返回：带她西去、焚尸、闭关三日 | 山洞 → 百药园 | 叙述；teleport | ch170-171 | 1 + 3 + 30 | 350 |
| 15 | 服丹之法；五年后封禁六十年 | 黄枫谷·传功阁 | 叙述；决定 | ch171-172 | 14 | 400 |
| 16 | 报名：王师叔、龙鳞果 | 黄枫谷·百机堂 | 二选一 | ch172-173 | 0 | 350 |
| 17 | 马师伯的两瓶丹药 | 百药园 | 领物（Z1 结算）| ch173 | 20 | 150 |
| **幕三** | **血色试炼** | | | | | |
| 18 | 议事大殿：灵石、摸灵器、李师祖、银甲角蟒 | 黄枫谷 → 禁地外 | 领物；teleport | ch173-174 | 2 | 550 |
| 19 | 荒山：向之礼；七派到齐；赌约；正与邪 | 禁地外（荒山）| 二选一（向之礼）；叙述 | ch174-179 | 1 | 1000 |
| 20 | 黄土坡：破禁 | 禁地外 → 外围 | 叙述；teleport | ch179-180 | 0 | 350 |
| 21 | 乌龙潭；山崖两尸；丝线 | 外围 | 潜伏；拾丝线 | ch180-182 | 0 | 350 |
| 22 | 一线天 | 外围 | **战斗②** | ch183-184 | 0 | 300 |
| 23 | 林中一夜；多宝女；封岳 | 中心区外 | 二选一（出手 / 想躲）；**战斗③**；踏云靴 | ch185-190 | 1 | 700 |
| 24 | 铜门；钟吾；中心区三层；树洞 | 中心区外 | 二选一（认 / 含糊）；交换玉简；支线 Z2 | ch191-194 | 1 | 500 |
| 25 | 月阳宝珠 | 中心区外 | 叙述 | ch195-196 | 0 | 250 |
| 26 | 紫猴花洞：设伏巨蜈蚣 | 环形山 | **设伏**（窥探 → 插刃 → 诱敌）；采苗 | ch196-198 | 0 | 400 |
| 27 | 四处灵药 | 环形山 | 蒙太奇 | ch200 | 0 | 120 |
| 28 | 小石殿：菡云芝与赤脚大汉 | 环形山 | **战斗④**；二选一（让她忘 / 让她立誓）；无忧针 | ch200-203 | 1 | 700 |
| 29 | 青石殿与地道 | 环形山 → 沼泽 | 躲；teleport | ch203-204 | 0 | 250 |
| 30 | 观战；墨蛟 | 沼泽 | 二选一（再等等 / 这就出手）；**战斗⑤**（友军白衣少女）| ch204-207 | 0 | 650 |
| 31 | 醒来；金箱；灵药；破土；南宫婉 | 沼泽 → 环形山 | 暗场；领药；teleport | ch207-209 | 0 | 450 |
| 32 | 下山；出禁地；赌局；嗅灵兽；记名弟子 | 环形山 → 禁地外 → 百药园 | 上交；二选一；teleport ×2 | ch209-212 | 1 + 4 | 900 |
| **幕四** | **地火屋** | | | | | |
| 33 | 谢师礼 | 百药园 | 叙述 | ch212 | 0 | 300 |
| 34 | 三年 | 百药园 | 蒙太奇（奖赏、清点、催熟、配药粉、消息）| ch213 | 4 + 1080 | 450 |
| 35 | 丑汉与石门；十九号；第一颗；半年 | 岳麓殿 → 地火屋 | 付定金；**炼丹**（地火，亲手开炉）| ch213-215 | 180 | 700 |
| 36 | 服丹筑基；出关 | 地火屋 → 百药园 | 叙述；`realm.advance` ×3；章末 | ch215-217 | 150 | 550 |

合计约 **17300 字**（本章原著 65 章，是第 6 章的两倍半；讲解性长段——许老、吴风、符宝之秘、正与邪、中心区秘闻、服丹——每段压到 8 句以内）。
**主动分支 10 处**（11、13、16、19a、23、24、28、30、32b、36）＋药园三时节的种什么、浇哪株；**必打战斗 5 场**（8.0），可选：环形山野外遭遇、支线 Z2 东坡石屋那一场。

### 3.1 每一处挂点怎么开始（`mode` / `once`）——判据的上游

分法沿用前六章：**这件事是他做的（interact），还是发生在他身上的（enter）。** 不用 `auto`。
**马师伯本章不摆 NPC**：第 6 章的 `npc_ma_shibo` 以 `ch06.renyao` 撤场，本章再摆同一个 role 的 NPC 必须 `visible_flag == ch06.renyao` 才过规则 30——那会让他在第 6 章 18a 之后一直站在园里。他本章的来去全靠挂点与台词（第 4 节）。

| # | 挂点 | 地图 | `mode` | `once` | `guard_flag` | `set_flag` | 战斗 | 为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | `trigger_chaibao` | `ch06_baiyaoyuan` | `interact` | ✓ | `ch06.done` | `ch07.chaibao` | — | 拆叶师叔那一包是他做的。居室桌前一格（与 `facility_dazuo` 不同格）|
| 2 / 3 / Z1 | `trigger_yaolou` | `ch06_baiyaoyuan` | `interact` | ✗ | `ch07.chaibao` | —（脚本按旗标置 `ch07.jiaoyao1` / `ch07.danbao` / `ch07.baigong`）| — | 把药摆进茅屋前的药篓、等马师伯来取，是他做的。**一处药篓管三次交药**：`once=false`，脚本看旗标决定这一回收什么；都交过了就一句「这一季的药已经交了」。R-3 提示在目标链上 |
| 4a | `trigger_yuelu_ru` | `ch07_yuelu_dian` | `enter` | ✓ | `ch07.danbao` | `ch07.yuelu` | — | 传送阵把他送进大厅，是发生在他身上的。传送阵出口第一格；石台验玉符、红衣弟子那一段在这里追叙 |
| 4b | `trigger_cangshi` | `ch07_yuelu_dian` | `interact` | ✓ | `ch07.yuelu` | `ch07.cangshi` | — | 在藏室书桌前翻玉筒是他。**置 `ch07.cangshi` 即两张配方可见**（E1）|
| 5a | `trigger_dihuo_wen` | `ch07_yuelu_dian` | `interact` | ✓ | `ch07.cangshi` | `ch07.yinsi` | — | 回头问许老地火、挑丹炉是他。许老柜台前一格 |
| 5b | `trigger_shimen` | `ch07_yuelu_dian` | `enter` | ✓ | `ch07.yinsi` | `ch07.chouhan` | — | 丑汉的冷脸是撞上的。无标记通道尽头、石门前一格 |
| 6 | `trigger_murong` | `ch06_huangfenggu` | `enter` | ✓ | `ch07.chouhan` | `ch07.murong` | — | 坡上的热闹是撞上的。岳麓殿那道门回园子的必经路上（18 #2：东口）|
| 7 | `trigger_dufang` | `ch06_baiyaoyuan` | `interact` | ✓ | `ch07.murong` | `ch07.dufang` | — | 回屋读方是他。书桌一格（与 1 不同格）。**脚本里 `advance_days(6)`，马师伯来** |
| 8 / 15 | `trigger_chuangong` | `ch06_huangfenggu` | `interact` | ✗ | `ch07.dufang` | —（脚本置 `ch07.lianqi` / `ch07.fudan_fa`）| — | 去传功阁请教吴风是他。吴风面前一格（与第 6 章 `trigger_wufeng` 不同格）。**一处管两场**，理由同药篓 |
| 9 | `trigger_yuanjiao` | `ch06_baiyaoyuan` | `interact` | ✓ | `ch07.lianqi` | `ch07.qiannian` | — | 园角两株千年灵草收成，是他。园角田（`facility_field_jiao`）旁一格；**脚本 `item.count_aged` 数到两株 ≥ 1000 年才置旗标（药不扣，节点 11 才交出去），不够就说还差几年** |
| 10 | `trigger_chuling` | `ch06_huangfenggu` | `interact` | ✓ | `ch07.qiannian` | `ch07.chuling` | — | 向于执事领令牌是他。百机堂于执事面前（与 `trigger_zawu` 不同格）|
| 11a | `trigger_fangshi_ru` | `ch07_fangshi` | `enter` | ✓ | `ch07.chuling` | `ch07.fangshi` | — | 进坊市北口，规矩是看出来的。北口第一格 |
| 11b | `trigger_wanbaolou` | `ch07_fangshi` | `interact` | ✓ | `ch07.fangshi` | `ch07.wanbaolou` | — | 挑最气派的一家推门进去是他。万宝楼门口一格 |
| 12 | `trigger_fangshi_chu` | `ch07_fangshi` | `enter` | ✓ | `ch07.wanbaolou` | `ch07.likai` | — | 出南口就走。**`advance_days(6)`、脚本末尾 teleport 山洞** |
| 13 | `trigger_yeyu` | `ch07_shandong` | `interact` | ✓ | `ch07.likai` | `ch07.yeyu` | ① | 在洞里歇下是他；洞外的人声是后来的。洞里石壁前一格 |
| 14 | `trigger_fanhui` | `ch07_shandong` | `interact` | ✓ | `ch07.yeyu` | `ch07.fanhui` | — | 收拾残局、带她走是他。洞外空地一格。**脚本末尾 teleport 百药园** |
| 16 | `trigger_baoming` | `ch06_huangfenggu` | `interact` | ✓ | `ch07.fudan_fa` | `ch07.baoming` | — | 去报名是他。百机堂另一格（王师叔今天坐这里收报名）|
| 17 | `trigger_ma_songyao` | `ch06_baiyaoyuan` | `enter` | ✓ | `ch07.baoming` | `ch07.ma_songyao` | — | 马师伯在园门等他。园门内一格（与 `trigger_jinzhi` 不同格）|
| 18 | `trigger_jihe` | `ch06_huangfenggu` | `enter` | ✓ | `ch07.ma_songyao` | `ch07.jihe` | — | 信符召集、踏进大殿。大殿门前一格（与 `trigger_dadian` 不同格）。**脚本末尾 teleport 禁地外（荒山）** |
| 19a | `trigger_xiang` | `ch07_jindi_wai` | `enter` | ✓ | `ch07.jihe` | `ch07.xiang` | — | 向之礼找上他。荒山角落一格 |
| 19b | `trigger_liedui` | `ch07_jindi_wai` | `interact` | ✓ | `ch07.xiang` | `ch07.qipai` | — | 次日上午走进队列是他。队列那一格。**`advance_days(1)` 在开头；末尾 teleport 黄土坡（同图）** |
| 20 | `trigger_pojin` | `ch07_jindi_wai` | `enter` | ✓ | `ch07.qipai` | `ch07.pojin` | — | 七件法宝轰开的通道。通道口一格。**末尾 teleport 外围** |
| 21a | `trigger_wulongtan` | `ch07_jindi_waiwei` | `enter` | ✓ | `ch07.pojin` | `ch07.wulongtan` | — | 潭边的事是撞上的。潭边矮树后一格 |
| 21b | `trigger_liangshi` | `ch07_jindi_waiwei` | `interact` | ✓ | `ch07.wulongtan` | `ch07.sixian` | — | 翻两具尸首是他。山崖下一格。`give("weapon_wuming_sixian")` |
| 22 | `trigger_yixiantian` | `ch07_jindi_waiwei` | `enter` | ✓ | `ch07.sixian` | `ch07.yixiantian` | ② | 一出路口就被堵。一线天出口一格 |
| 23 | `trigger_shulin` | `ch07_jindi_zhongxin` | `interact` | ✓ | `ch07.yixiantian` | `ch07.fengyue` | ③ | 爬上大树歇息是他；后面的事找上门。林边大树一格。**`advance_days(1)` 在第一夜之后、第二日早之前**（校对 LOW-12）|
| 24 | `trigger_tongmen` | `ch07_jindi_zhongxin` | `enter` | ✓ | `ch07.fengyue` | `ch07.zhongwu` | — | 铜门前的三具尸与门内的飞蛇都是撞上的。铜门内第一格。**末尾 `advance_days(1)`（树洞睡到第三日凌晨）** |
| 25 | `trigger_yueyang` | `ch07_jindi_zhongxin` | `enter` | ✓ | `ch07.zhongwu` | `ch07.yueyang` | — | 光雨是看见的。上山小路口的树后一格；置后通环形山的门开 |
| 26a | `trigger_kuitan` | `ch07_huanxingshan` | `enter` | ✓ | `ch07.yueyang` | `ch07.kuitan` | — | 在拐角处看见巨蜈蚣。洞内拐角一格 |
| 26b | `trigger_charen` | `ch07_huanxingshan` | `interact` | ✓ | `ch07.kuitan` | `ch07.mairen` | — | 退回洞道埋子刃是他。洞道中段一格 |
| 26c | `trigger_youdi` | `ch07_huanxingshan` | `enter` | ✓ | `ch07.mairen` | `ch07.zihouhua` | — | 去石厅口把它引出来是他（引法自出，13.1 第 26 段）。石厅口一格 |
| 27 | `trigger_sichu` | `ch07_huanxingshan` | `interact` | ✓ | `ch07.zihouhua` | `ch07.sichu` | — | 照资料把另外四处跑一遍是他。洞外资料所记的岔路石一格 |
| 28 | `trigger_xiaoshidian` | `ch07_huanxingshan` | `enter` | ✓ | `ch07.sichu` | `ch07.wuyou` | ④ | 打斗声是传过来的。小石殿前空地边一格。**末尾 `advance_days(1)`（第三日过去）** |
| 29 | `trigger_qingshidian` | `ch07_huanxingshan` | `interact` | ✓ | `ch07.wuyou` | `ch07.didao` | — | 拿金刃试殿门是他。青石殿门前一格。**末尾 teleport 沼泽** |
| 30 | `trigger_guanzhan` | `ch07_dixia_zhaoze` | `enter` | ✓ | `ch07.didao` | `ch07.mojiao` | ⑤ | 伏在黑土堆后，外面的事发生在他眼前。土堆后一格 |
| 31 | `trigger_jiaoshi` | `ch07_dixia_zhaoze` | `interact` | ✓ | `ch07.mojiao` | `ch07.nangong` | — | 拿银剑剖蛟尸是他。蛟尸旁一格。**末尾 teleport 环形山（地面破洞旁）** |
| 32a | `trigger_xiashan` | `ch07_huanxingshan` | `enter` | ✓ | `ch07.nangong` | `ch07.xiashan` | — | 往山下奔。破洞外一格。**末尾 teleport 禁地外（黄土坡出口）** |
| 32b | `trigger_chukou` | `ch07_jindi_wai` | `enter` | ✓ | `ch07.xiashan` | `ch07.chujindi` | — | 出了通道，一派一派的人站着。出口第一格。**脚本里置 `ch07.baishi`；末尾 `advance_days(4)`、teleport 百药园** |
| 33 | `trigger_xieshili` | `ch06_baiyaoyuan` | `enter` | ✓ | `ch07.chujindi` | `ch07.xieshili` | — | 马师伯在屋里等他。茅屋门内一格 |
| 34 | `trigger_sannian` | `ch06_baiyaoyuan` | `interact` | ✓ | `ch07.xieshili` | `ch07.sannian` | — | 坐下排三年的计划是他。书桌旁一格（与 1、7 不同格）|
| 35a | `trigger_chouhan` | `ch07_yuelu_dian` | `interact` | ✓ | `ch07.sannian` | `ch07.dihuo` | — | 去石屋借地火是他（怎么弄醒丑汉自出）。丑汉石屋门口一格；置后石门开（`portal_to_dihuo`）|
| 35b | `trigger_shijiu` | `ch07_dihuo` | `enter` | ✓ | `ch07.dihuo` | `ch07.feidan` | — | 推门进十九号是他。门内第一格；置后地火可用 |
| 35c | `trigger_banian` | `ch07_dihuo` | `interact` | ✓ | `ch07.feidan` | `ch07.chengdan` | — | 在鼎旁盘点、决定接着炼半年是他。圆墩旁一格。**亲手炼出至少一颗才置旗标**（第 7 节）|
| 36 | `trigger_zhuji` | `ch07_dihuo` | `interact` | ✓ | `ch07.chengdan` | `ch07.done` | — | 就地服丹是他。墙角蒲团一格。**`realm.advance` 三次、`advance_days(150)`、末尾 teleport 百药园、章末** |
| Z2a | `trigger_yujian_dongpo` | `ch07_huanxingshan` | `interact` | ✓ | `ch07.yujian_qiu` | `ch07.yujian_a` | 可选 | 玉简上东坡的石屋；门口两只铁臂猿（`be07_tiebi_yuan`，可逃，逃了不置旗标）|
| Z2b | `trigger_yujian_hantan` | `ch07_huanxingshan` | `interact` | ✓ | `ch07.yujian_qiu` | `ch07.yujian_b` | — | 玉简上北坡的寒潭 |
| — | `facility_field_jiao` | `ch06_baiyaoyuan` | 设施 | — | `require_flag=ch07.chaibao` | — | — | 园角两槽灵田（`ref_id=field_baiyaoyuan_jiao`、`slots=2`）；药田角落埋瓶那一格旁 |
| — | `facility_yaotan` | `ch07_fangshi` | 设施 | — | — | — | — | 北段收药摊（`kind=shop`、`ref_id=ch07_fangshi_yaotan`）|
| — | `facility_dihuo` | `ch07_dihuo` | 设施 | — | `require_flag=ch07.feidan` | — | — | 十九号圆墩：`kind=alchemy`、`grade=4`（地火）|
| — | 禁地打坐点 `facility_dazuo_shuguan` / `facility_dazuo_shudong` / `facility_dazuo_shidong` | `ch07_jindi_zhongxin`（林边大树旁、树洞）/ `ch07_huanxingshan`（山顶石洞）| 设施 | — | — | — | — | `kind=meditate`。原著他在树冠（ch185）、树洞（ch195）打坐回过法力，第三日夜里藏身山顶一带（ch203）；禁地里的伤一路带着走、存档点不回血（16.2 第 5 条），没有这三处，必打之前的存档可能是死档 |
| — | 各图存档点 `facility_cunji` | 九张新图 | 设施 | — | — | — | — | 每场必打之前一个（第 4 节）|

- **`once=true` 的挂点每一条都写 `set_flag`**；逃出战斗、输了再来、条件不够（药不够、千年不够）的路都在置旗标之前 `return`。两处 `once=false`（药篓、传功阁）不写 `set_flag`（地图规范 `docs/map_spec.md` 4.4），靠脚本自己的旗标判「已演过」。
- **同一格只挂一个对象**：百药园居室里拆包、读方、三年三场戏加蒲团四样东西，各占一格；百机堂里第 6 章的 `trigger_zawu`、于执事 NPC 与本章的令牌、报名四样各占一格；十九号里圆墩（设施）、盘点、蒲团三样各占一格。
- 五场必打全部挂在目标链的必经挂点上（13、22、23、28、30），每一处的 `set_flag` 都是下一节点的 `guard_flag`——绕不过去。
- **脚本 teleport 登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`**：12（坊市 → 山洞）、14（山洞 → 百药园）、18（黄枫谷 → 禁地外）、19b（禁地外荒山 → 黄土坡，同图）、20（禁地外 → 外围）、29（环形山 → 沼泽）、31（沼泽 → 环形山）、32a（环形山 → 禁地外）、32b（禁地外 → 百药园）、36（地火屋 → 百药园）。不在目标链上的门钥匙（`kOffChainDoorKeys`）：无。

### 3.2 各节点要点

> **校对整改（2026-09-29，裁决 16.5 HIGH-1）**：本节原先逐节转述原著的镜头、动作与台词，13.1 虽宣布作废、正文却一句没删，写手照着抄进了文案（校对 §4.2(d)：80 处残留里至少 32 处能在这里找到同一个镜头）。**现已整节重写：只留脚本调用、分支结构与数据、死局兜底这类机制要求。每段写什么、不许写什么，一律以 13.1「每段框架件清单」为准；镜头、动作、台词、次序、比方一律自出。**
> 下文「框架件：13.1 第 N 段」即指该段那一行；凡本节没写到的文字内容，都不是施工要求。

**幕一　百药园两年**

**节点 1　拆包**（`trigger_chaibao`）
- 脚本：`give` 苗（黄精 3、紫参 2、血红芝 1、黄精芝 2，年份 0）；`field.unlock("field_baiyaoyuan_jiao", 2)`；`give("material_lingshi_zhong", 2)`、`story_jinggang_huan`、`story_heiwu_qi`、`story_huangtong_ping`、`talisman_tulao_fu` ×2；`bottle.spend(1)` 看返回值（瓶里没液就换一句，不设死局）；`flag.set("ch07.chaibao")`。
- 衔接第 6 章：第 6 章章末送物时数过的是桌上那一堆，这一包当时没拆（校对 LOW-11）。
- 框架件：13.1 第 1 段。目标行给 R-3 提示（3.4）。

**节点 2　头一年**（`trigger_yaolou`，第一回）
- 脚本：`item.count_aged("herb_huangjing_cao", 44) >= 3` 才往下，`take_aged("herb_huangjing_cao", 3, 44)`（E4）；不够按手上够格的株数各给一句、`return`；`give("material_lingshi", 24)`；`flag.set("ch07.jiaoyao1")`。
- 苗要先下到药田、满一年才浇得进；黄精一株三滴到顶（第 2 章的规矩）。
- 框架件：13.1 第 2 段。

**节点 3　第二年**（`trigger_yaolou`，第二回）
- 脚本：`take_aged("herb_zishen_cao", 2, 100)`；`give("material_lingshi", 60)`；`realm.advance(11)` 看返回值；养精丹**脚本不扣**；`flag.set("ch07.danbao")`（岳麓殿那道门的钥匙）、`flag.set("ch07.baigong_qiu")`（Z1）。
- 「岳麓殿」「巫钧山」从这一节起可以出口（12.2）。
- 框架件：13.1 第 3 段。

**节点 4　岳麓殿**
- 4a `trigger_yuelu_ru`：`flag.set("ch07.yuelu")`。框架件：13.1 第 4a 段。
- 4b `trigger_cangshi`：先数够 22 块再动手（`item.count < 22` → `return`，一块不扣）；`take("material_lingshi", 1)` ×2（两个时辰）、`take(…, 20)`（复制两份），每一处看返回值；`flag.set("ch07.cangshi")` → 两张方子在炼制面板上出现（E1）。「定颜丹」从这一节起可以出口（12.2）。框架件：13.1 第 4b 段。

**节点 5　地肺之火；丑汉**
- 5a `trigger_dihuo_wen`：`take("material_lingshi", 32)` 看返回值，不够就 `return`；`give("story_yinsi_ding")`；`flag.set("ch07.yinsi")`。框架件：13.1 第 5a 段。
- 5b `trigger_shimen`：`flag.set("ch07.chouhan")`；只写他听见了、不发作，**不写杀心**（第 8 章之前他并没有报复）。框架件：13.1 第 5b 段。

**节点 6　东口坡上**（`trigger_murong`）
- 脚本：`flag.set("ch07.murong")`。玩家不操作。
- NPC：本节点七个人 `visible_flag=ch07.chouhan`、`hidden_flag=ch07.murong`（第 5 节）；蓝衣女子不点名（12.1：聂盈）。
- 框架件：13.1 第 6 段。

**节点 7　读方；马师伯**（`trigger_dufang`）
- 脚本：`advance_days(6)`；`flag.set("ch07.dufang")`。辅药不点药名。
- 框架件：13.1 第 7 段。

**节点 8　吴风：禁地；敛气术**（`trigger_chuangong`，第一回）
- 脚本：开口第一句按 `ch06.wufeng` 分（1 / 2，1.4）；`advance_days(3)`；`flag.set("ch07.lianqi")`——**敛气术不进法术表**（16.1 第 13 条），此后脚本里凡「他敛了气」都读这面旗。
- 讲禁地压成 8 句以内，次序按「韩立想知道什么」自排（13 第 2 条）；「血色试炼」四个字从这一节起可以出口（12.2；章节卡是登记的例外）。
- 框架件：13.1 第 8 段。

**幕二　备战与坊市**

**节点 9　园角千年灵草**（`trigger_yuanjiao`）
- 脚本：`item.count_aged("herb_huangjing_zhi", 1000) >= 2` 才置 `ch07.qiannian`（药不扣，节点 11 才交出去）；不够就一句、`return`。黄精芝五阶，千年要八滴一株（R-3 提示）。支线 Z1 那一季的药也在这段日子里交（第 6 节）。
- 框架件：13.1 第 9 段。

**节点 10　外出令牌**（`trigger_chuling`）：`flag.set("ch07.chuling")` 开坊市那道门。框架件：13.1 第 10 段。

**节点 11　坊市与万宝楼**（`ch07_fangshi`）
- 11a `trigger_fangshi_ru`：`flag.set("ch07.fangshi")`。收药摊（`facility_yaotan`）收不了千年黄精芝、禁地三药与血红芝（`tradeable=false`，第 9 节；血红芝见 16.5 M4）。框架件：13.1 第 11a 段。
- 11b `trigger_wanbaolou`：进门前 `count_aged(…, 1000) >= 2`；**二选一**「先只亮出一株」（`ch07.wanbaolou = 1`）/「两株一起亮」（`= 2`，只改掌柜那两句）；`take_aged("herb_huangjing_zhi", 2, 1000)` 看返回值 → `give` 金蚨子母刃、玄铁飞天盾、天雷子、金光砖，`magic.learn` 金蚨、金光砖。符宝讲法压成 5 句以内、不用「伪法宝」（13 第 3 条）。框架件：13.1 第 11b 段。
- 不演掌柜事后派不派人跟；他出楼即走。

**节点 12　绕路**（`trigger_fangshi_chu`）：`advance_days(6)`；`flag.set("ch07.likai")`；teleport `ch07_shandong` 洞内。框架件：13.1 第 12 段。

**节点 13　夜遇**（`trigger_yeyu`，战斗①）
- **暗场**（16.1 第 3 条）：只写「被捆」「神情迷乱」「扑来」「被定住」；董家、红拂不点名。
- **二选一**「等他露出破绽」（`= 1`）/「现在就动手」（`= 2`）——两条都接同一场偷袭、同一张编成。
- 胜：`take("talisman_jianfu", 1)` ＋ `magic.forget("magic_ji_jianfu")`；`take("story_jinggang_huan", 1)`；`give("pill_zhuji_dan", 2)`、`story_qingjiao_qi` ＋ `magic.learn("magic_ji_qingjiao")`、`story_qing_suo`、`story_yin_gou`、火球符 4、护身符 2、灵石 20；定神符**有则扣**，没有就改说定神术（不设死局）；`flag.set("ch07.yeyu", pick)`。
- 败：game over。
- 框架件：13.1 第 13 段；打法意图见 8.2 ①。

**节点 14　返回**（`trigger_fanhui`）
- 脚本：清灵散**有则扣**（没有就换一句）；`advance_days(1)`；teleport `ch06_baiyaoyuan`；`advance_days(3)`；`advance_days(30)`；`flag.set("ch07.fanhui")`。只保留「他不碰她」这层意思，不写欲念。
- 框架件：13.1 第 14 段。

**节点 15　服丹之法；封禁**（`trigger_chuangong`，第二回）
- 脚本：`advance_days(14)`；`flag.set("ch07.fudan_fa")`。
- 日子：此时离出发只剩一个来月（3.3：≈880 → ≈916），两难说成「闭关三个月就赶不上这一趟」（校对 MEDIUM-3）。
- 「紧迫感」那一句（验收 17）：同时含「五年」「六十年」「最后」，交代五年后那一届是封山前最后一回、这一届是他仅有的机会；「封闭前最后一次」这个错误说法 0 处。
- 框架件：13.1 第 15 段。

**节点 16　报名**（`trigger_baoming`）：**二选一**「幼时误食过一枚异果」（`= 1`）/「弟子只是苦修」（`= 2`）——两条都报上名；节点 32 王师叔有一句按它分。框架件：13.1 第 16 段。

**节点 17　两瓶丹药**（`trigger_ma_songyao`）：Z1 做了 `give` 养精丹、金疮药各 5，没做各 3（第 6 节）；`advance_days(20)`；`flag.set("ch07.ma_songyao")`。框架件：13.1 第 17 段。

**幕三　血色试炼**

> **幕三的节奏写法**：① 韩立的内心一律是算计与怕，不写豪言；② 每进一处先交代这里刚死过人，尸体只写姿态、不写血腥细节；③ 每一日开头一行地名＋「第几日」横幅（`advance_days` 之后）；④ 五场必打收场只播结算卡、不放凯旋；⑤ 出禁地时把人数一派一派报出来，这是这一幕唯一的「数字」。

**节点 18　议事大殿**（`trigger_jihe`）
- 脚本：`give("material_lingshi_zhong", 1)`（水属性）；`give("story_bubao_faqi")`——**本章到节点 34 才说它是什么**，那时换成 `story_kuilei_gongshou`（校对 LOW-3）；`advance_days(2)`；teleport `ch07_jindi_wai` 荒山；`flag.set("ch07.jihe")`。
- 「师祖」从这一节起可以出口（12.2）；大殿前李师祖的 NPC 与文案对得上（他一直站在殿门口，第 5 节）。
- 框架件：13.1 第 18 段。

**节点 19　荒山：七派到齐**（`ch07_jindi_wai`）
- 19a `trigger_xiang`：**二选一**「客客气气地谢绝」（`= 1`）/「进去以后再说」（`= 2`）；节点 32 向之礼最后一个爬出来时 `= 1` 多一句。框架件：13.1 第 19a 段。
- 19b `trigger_liedui`：`advance_days(1)` 在开头；`ch06.qianyao > 0` 时两句了结欠账、`flag.set("ch06.qianyao", 0)`（18 #14）；teleport 黄土坡（同图）；`flag.set("ch07.qipai")`。正与邪压成 4 句以内、**句式全换**（13 第 4 条）；穹前辈的赌注要交代输赢两头（校对 LOW-10）；「穹老怪」不出口（ch196 首见）。框架件：13.1 第 19b 段。

**节点 20　破禁**（`trigger_pojin`）：`flag.set("ch07.pojin")`；teleport `ch07_jindi_waiwei`。框架件：13.1 第 20 段。

**节点 21　乌龙潭；山崖**
- 21a `trigger_wulongtan`：`flag.set("ch07.wulongtan")`；寒烟草一株也拿不到。地貌自出。框架件：13.1 第 21a 段。
- 21b `trigger_liangshi`：`give("weapon_wuming_sixian")`（暗器，8.2）；`flag.set("ch07.sixian")`；茅草地里的人是暗场（16.1 第 9 条）。框架件：13.1 第 21b 段。

**节点 22　一线天**（`trigger_yixiantian`，战斗②）：`battle("b07_yixiantian")`，败 game over；胜后储物袋在节点 34 清点、毁尸一句；`flag.set("ch07.yixiantian")`。框架件：13.1 第 22 段；意图 8.2 ②。

**节点 23　林中；多宝女；封岳**（`trigger_shulin`，战斗③）
- 脚本：第一夜说完才 `advance_days(1)`、再打「第二日」横幅（校对 LOW-12）；**二选一**「跳下去救她」（`= 1`）/「躲在树上不出声」（`= 2`，她那一路也照样找到他）；多宝女**做成演出、不开战**（8.3）；银钩、青索各 `take` 一件（看返回值）；`battle("b07_fengyue")`，败 game over；胜：`give("story_xiao_yuanjing")` ＋ `magic.learn("magic_ji_qingning")`（法术名「祭镜」）、`story_shuijing_qiu`、`talisman_xiaodao_fubao`、`give("story_hei_xue")`——镜子、靴子的正名要到节点 28、24 才点破（校对 LOW-3）；`flag.set("ch07.fengyue", pick)`。
- 封岳那句问话与他的回答是名场面，说法自拟（13 第 7 条）；萧二、寒天涯、钟吾杀人的几幕不演。
- 框架件：13.1 第 23 段；意图 8.2 ③。

**节点 24　铜门；钟吾；秘闻**（`trigger_tongmen`）
- 脚本：钟吾认出靴子那一句之后 `take("story_hei_xue")` → `give("story_tayun_xue")`；**二选一**「照实说封岳死了」（`= 1`）/「说靴子是捡来的」（`= 2`）；`flag.set("ch07.yujian_qiu")`（Z2）；`advance_days(1)`；`flag.set("ch07.zhongwu", pick)`。
- 墙上三具尸，动手的人不点名；中心区秘闻压成 5 句以内，不用「果皮果肉果核」（13 第 5 条）。
- 框架件：13.1 第 24 段。

**节点 25　月阳宝珠**（`trigger_yueyang`）：`flag.set("ch07.yueyang")` 开通环形山的门。写羡慕。框架件：13.1 第 25 段。

**节点 26　紫猴花洞**（`ch07_huanxingshan`）
- 26a `trigger_kuitan`：他的算计（幼苗）在这里说出来，**不提嗅灵兽**；`flag.set("ch07.kuitan")`。
- 26b `trigger_charen`：埋八柄子刃做成 `choice{}` 循环（中途取消 = 收手、不置旗标、回来重埋），不点名按键；`flag.set("ch07.mairen")`。
- 26c `trigger_youdi`：**不是战斗**（8.3）；`give("herb_zihou_hua", 4, 1)`、`give("material_wugong_ke", 3)`；清灵散有则扣；`flag.set("ch07.zihouhua")`。
- 框架件：13.1 第 26 段（引它出来的法子、埋刃之后怎么藏，都自出）。

**节点 27　四处灵药**（`trigger_sichu`）：`give` 玉髓芝 4、天灵果 2、紫猴花 2（幼苗，年份 1）；`flag.set("ch07.sichu")`。框架件：13.1 第 27 段。

**节点 28　小石殿**（`trigger_xiaoshidian`，战斗④）
- 脚本：`battle("b07_zhongxinqu_duoyao")`，败 game over；大汉认出镜子那一句之后 `take("story_xiao_yuanjing")` → `give("story_qingning_jing")`；`give("story_yinhui_jian")`；**二选一**「让她忘了这半日」（`= 1`）/「先让她立誓」（`= 2`）——两条都打晕、施针；`give("herb_tianling_guo", 4, 1)`；`advance_days(1)`；`flag.set("ch07.wuyou", pick)`。
- 「菡云芝」从战后那几句起可以出口（`ch07.xiaoshidian.after*`，12.2），名牌同时换 `han_yunzhi`；原著那一吻不演，只写打晕（13 第 8 条）。
- 框架件：13.1 第 28 段；意图 8.2 ④。

**节点 29　青石殿与地道**（`trigger_qingshidian`）：`flag.set("ch07.didao")`（Z2 过期）；teleport `ch07_dixia_zhaoze`。框架件：13.1 第 29 段。

**节点 30　观战；墨蛟**（`trigger_guanzhan`，战斗⑤）
- 脚本：**二选一**「再等她耗一阵」（`= 1`）/「现在就出手」（`= 2`，战后多一句冷言）——**编成同一张**；`battle("b07_zhaoze_shouyao")`，败 game over；`flag.set("ch07.mojiao", pick)`。
- 小五行符那一段在外面、是暗场；这一节不出现「南宫」二字（12.2）；她质问他为何不早出手、他的回答，是名场面，说法自拟（13 第 7 条）。
- 框架件：13.1 第 30 段；意图 8.2 ⑤。

**节点 31　醒来；金箱；破土**（`trigger_jiaoshi`）
- **暗场**（16.1 第 3 条）：只写「两个人都失去了知觉」与醒后她的外貌和那几句话；「淫囊」「合欢」不出现。
- 脚本：`give` 成熟灵药（玉髓芝 8@400、紫猴花 7@400、天灵果 7@300，共二十二株）与幼苗（玉髓芝 2@1）、`material_mojiao_cailiao`；`take("talisman_jinguangzhuan", 1)` ＋ `magic.forget("magic_ji_jinguangzhuan")`；teleport `ch07_huanxingshan` 破洞旁；`flag.set("ch07.nangong")`。面板境界不动。
- 「南宫」从问名那一句起可以出口（`ch07.jiaoshi.name*`，12.2）。
- 框架件：13.1 第 31 段。

**节点 32　下山；出禁地；记名弟子**
- 32a `trigger_xiashan`：`advance_days(1)`（第五日，18 #6）；teleport 出口；`flag.set("ch07.xiashan")`。不开战。框架件：13.1 第 32a 段。
- 32b `trigger_chukou`：禁地三药各 `n = item.count_aged(id, 100)`、`take_aged(id, n, 100)`（幼苗 < 100 年留在他手里）；Z2 两处都去过加赏灵石 50、中阶 1；**二选一**「迟疑片刻再应」（`= 1`）/「当场跪下叩拜」（`= 2`）——两条都拜；`give("weapon_biguang_dao")`；王师叔一句按 `ch07.baoming` 分；`advance_days(4)`；teleport `ch06_baiyaoyuan`；`flag.set("ch07.baishi", pick)`、`flag.set("ch07.chujindi")`。
- 南宫婉登舟那一句含「没」「看」（验收 18）；「李化元」这一节仍不出口（12.2）；李师祖应承的铁精是给穹前辈的（校对 LOW-10）。
- 框架件：13.1 第 32b 段（他怎么交药、编的来历、李师祖怎么问，都自出）。

**幕四　地火屋**

**节点 33　谢师礼**（`trigger_xieshili`）：`flag.set("ch07.xieshili")`。「李化元」首次出口（马师伯说）。框架件：13.1 第 33 段。

**节点 34　三年**（`trigger_sannian`）
- 脚本：`advance_days(4)`；`give("pill_zhuji_dan", 1)`；`give` 中阶 10、灵石 300；打开布包那一句之后 `take("story_bubao_faqi")` → `give("story_kuilei_gongshou")`（校对 LOW-3）；`give("story_yinse_shuye")`；三药全部余量 `take`；`advance_days(1080)`；`give("material_zhuji_yaofen", 40)`——**不看余量多少，药粉一律 40 份**（两侧同账）；叶师叔补还按 `ch06.huinuo` / `ch06.rangdan` 两面各一句、东西一样（中阶 5、回气丹 2）；`give("story_qingyuan_jianjue")`；`flag.set("ch07.sannian")`。
- 框架件：13.1 第 34 段。

**节点 35　丑汉与石门；十九号；半年**
- 35a `trigger_chouhan`：传送阵一块、定金一块中阶都**有则扣**（没有灵石记账放行；没有中阶按兑率收一百块低阶；都没有先欠着，18 #7）；`flag.set("ch07.dihuo")`（`portal_to_dihuo` 开）。怎么把丑汉弄醒自出。框架件：13.1 第 35a 段。
- 35b `trigger_shijiu`：`take("material_zhuji_yaofen", 22)` 看返回值 → `give("story_feidan_yuhe")`；`flag.set("ch07.feidan")`——**地火这才交到玩家手上**（`facility_dihuo`，成算约三成，第 7 节）。屋里的陈设、失败的样子自出。框架件：13.1 第 35b 段。
- 35c `trigger_banian`：`item.count("pill_zhuji_dan") >= 4` 才往下；药粉用尽而一颗未成就 `give("material_zhuji_yaofen", 10)`、`return`（**不设死局**）；`advance_days(180)`；药粉余量 `take` 掉，筑基丹**补足到 25**；`flag.set("ch07.chengdan")`。框架件：13.1 第 35c 段。

**节点 36　服丹筑基**（`trigger_zhuji`）
- 脚本：**二选一**「就在这里闭关」/「回药园再说」——两条都就地、不另置旗标；`realm.advance(12)`、`realm.advance(13)`、`realm.advance(21)` 依次各看返回值；`take("pill_zhuji_dan", 8)`（章末剩 17）；`advance_days(150)`；`flag.set("ch07.done")`；teleport `ch06_baiyaoyuan`；不 `ending()`。
- 服丹写痛不写奇，痛法自出（13 第 6 条）；《青元剑诀》后三层的神通名不说（ch217，第 8 章）；章末落在他自己的感觉上，不给志向、不用面板式说法。
- 框架件：13.1 第 36 段。

### 3.3 日历

| 节点 | `advance_days` | 累计（从第 6 章终局起，估）|
| --- | --- | --- |
| 2 头一年（玩家打坐推进）| — | ≈ 400 |
| 3 第二年（同上）| — | ≈ 700 |
| 7 马师伯来 | 6 | ≈ 706 |
| 8 几夜未眠 | 3 | ≈ 709 |
| 9 四个多月（玩家推进）| — | ≈ 840 |
| 11a 坊市 | 0.5（不拨，归入 12）| |
| 12 绕路 | 6 | ≈ 846 |
| 13 夜遇 | 0（当夜）| |
| 14 返回、闭关、复原 | 1 + 3 + 30 | ≈ 880 |
| 15 近半月 | 14 | ≈ 894 |
| 17 到出发前三天 | 20 | ≈ 914 |
| 18 骑蟒 | 2 | ≈ 916 |
| 19b 次日 | 1 | ≈ 917 |
| 23 第一夜 | 1 | ≈ 918（禁地第二日）|
| 24 树洞 | 1 | ≈ 919（第三日）|
| 28 第三日过去 | 1 | ≈ 920（第四日）|
| 32b 回谷数日 | 4 | ≈ 924（禁地第五日下午出，途中四日）|
| 34 奖赏＋三年 | 4 + 1080 | ≈ 2008 |
| 35c 半年 | 180 | ≈ 2188 |
| 36 服丹五个月 | 150 | ≈ 2338 |

原著的锚：两年多（ch152）、六七日后（ch159）、下一届在半年后（ch160）、四个多月（ch161）、三四天＋三日（ch165）、三天三夜＋个把月（ch171）、近半月（ch172）、出发前三天（ch173）、两天两夜（ch174）、禁地五日（ch160、209）、数日（ch212）、四天后（ch213）、三年（ch213）、六个月＋十一个月（ch215）。
**玩家推进的三段**（2、3、9）靠居室蒲团打坐；本章**没有倒计时**，日子只用来对得上这几句话。禁地的五日由脚本拨；三处打坐点（3.1）是防死档用的，玩家在那里多坐的日子不改横幅（横幅照剧情写，不读 `today()`）。

### 3.4 目标链（`data/objectives/ch07.json`）

| # | step id | `done_flag` | `target_map` | `target_object` | R-3 提示 |
| --- | --- | --- | --- | --- | --- |
| 1 | `n01_chaibao` | `ch07.chaibao` | `ch06_baiyaoyuan` | `trigger_chaibao` | |
| 2 | `n02_jiaoyao` | `ch07.jiaoyao1` | `ch06_baiyaoyuan` | `trigger_yaolou` | **「黄精要四十四年——一株浇满一瓶才到顶；绿液不够就去蒲团上坐一阵」**（整改后前面补「苗先下到药田，满一年才浇得进」、后面补「瓶满了就不再凝」，校对 LOW-6、LOW-14）|
| 3 | `n03_danbao` | `ch07.danbao` | `ch06_baiyaoyuan` | `trigger_yaolou` | **「紫参要百年，一株五滴」** |
| 4 | `n04_yuelu` | `ch07.yuelu` | `ch07_yuelu_dian` | `trigger_yuelu_ru` | |
| 5 | `n05_cangshi` | `ch07.cangshi` | `ch07_yuelu_dian` | `trigger_cangshi` | **「藏室按时辰收灵石，复制另算——身上的灵石先数一遍」** |
| 6 | `n06_dihuo_wen` | `ch07.yinsi` | `ch07_yuelu_dian` | `trigger_dihuo_wen` | |
| 7 | `n07_shimen` | `ch07.chouhan` | `ch07_yuelu_dian` | `trigger_shimen` | |
| 8 | `n08_murong` | `ch07.murong` | `ch06_huangfenggu` | `trigger_murong` | |
| 9 | `n09_dufang` | `ch07.dufang` | `ch06_baiyaoyuan` | `trigger_dufang` | |
| 10 | `n10_jindi` | `ch07.lianqi` | `ch06_huangfenggu` | `trigger_chuangong` | |
| 11 | `n11_qiannian` | `ch07.qiannian` | `ch06_baiyaoyuan` | `trigger_yuanjiao` | **「千年要八滴一株——瓶子一满就去园角浇，别让它空满着」** |
| 12 | `n12_chuling` | `ch07.chuling` | `ch06_huangfenggu` | `trigger_chuling` | |
| 13 | `n13_fangshi` | `ch07.fangshi` | `ch07_fangshi` | `trigger_fangshi_ru` | |
| 14 | `n14_wanbaolou` | `ch07.wanbaolou` | `ch07_fangshi` | `trigger_wanbaolou` | |
| 15 | `n15_likai` | `ch07.likai` | `ch07_fangshi` | `trigger_fangshi_chu` | **「要买的符、要卖的药，都在坊市办了」**（校对 LOW-7：门只看 `ch07.chuling`，随时回得去，不说「不回头」）|
| 16 | `n16_yeyu` | `ch07.yeyu` | `ch07_shandong` | `trigger_yeyu` | |
| 17 | `n17_fanhui` | `ch07.fanhui` | `ch07_shandong` | `trigger_fanhui` | |
| 18 | `n18_fudan` | `ch07.fudan_fa` | `ch06_huangfenggu` | `trigger_chuangong` | |
| 19 | `n19_baoming` | `ch07.baoming` | `ch06_huangfenggu` | `trigger_baoming` | |
| 20 | `n20_songyao` | `ch07.ma_songyao` | `ch06_baiyaoyuan` | `trigger_ma_songyao` | |
| 21 | `n21_jihe` | `ch07.jihe` | `ch06_huangfenggu` | `trigger_jihe` | **「这一去五日，回不了头——药与符都带足」** |
| 22 | `n22_xiang` | `ch07.xiang` | `ch07_jindi_wai` | `trigger_xiang` | |
| 23 | `n23_qipai` | `ch07.qipai` | `ch07_jindi_wai` | `trigger_liedui` | |
| 24 | `n24_pojin` | `ch07.pojin` | `ch07_jindi_wai` | `trigger_pojin` | |
| 25 | `n25_wulongtan` | `ch07.wulongtan` | `ch07_jindi_waiwei` | `trigger_wulongtan` | |
| 26 | `n26_liangshi` | `ch07.sixian` | `ch07_jindi_waiwei` | `trigger_liangshi` | |
| 27 | `n27_yixiantian` | `ch07.yixiantian` | `ch07_jindi_waiwei` | `trigger_yixiantian` | |
| 28 | `n28_shulin` | `ch07.fengyue` | `ch07_jindi_zhongxin` | `trigger_shulin` | |
| 29 | `n29_tongmen` | `ch07.zhongwu` | `ch07_jindi_zhongxin` | `trigger_tongmen` | |
| 30 | `n30_yueyang` | `ch07.yueyang` | `ch07_jindi_zhongxin` | `trigger_yueyang` | |
| 31 | `n31_kuitan` | `ch07.kuitan` | `ch07_huanxingshan` | `trigger_kuitan` | |
| 32 | `n32_charen` | `ch07.mairen` | `ch07_huanxingshan` | `trigger_charen` | **「在石厅里硬打，毒血会毁了苗——洞道里能做点手脚」**（原先那一句是原著的算计，随 §4.2(e) 第 45 行一起换掉）|
| 33 | `n33_youdi` | `ch07.zihouhua` | `ch07_huanxingshan` | `trigger_youdi` | |
| 34 | `n34_sichu` | `ch07.sichu` | `ch07_huanxingshan` | `trigger_sichu` | |
| 35 | `n35_xiaoshidian` | `ch07.wuyou` | `ch07_huanxingshan` | `trigger_xiaoshidian` | |
| 36 | `n36_qingshidian` | `ch07.didao` | `ch07_huanxingshan` | `trigger_qingshidian` | |
| 37 | `n37_guanzhan` | `ch07.mojiao` | `ch07_dixia_zhaoze` | `trigger_guanzhan` | |
| 38 | `n38_jiaoshi` | `ch07.nangong` | `ch07_dixia_zhaoze` | `trigger_jiaoshi` | |
| 39 | `n39_xiashan` | `ch07.xiashan` | `ch07_huanxingshan` | `trigger_xiashan` | |
| 40 | `n40_chukou` | `ch07.chujindi` | `ch07_jindi_wai` | `trigger_chukou` | |
| 41 | `n41_xieshili` | `ch07.xieshili` | `ch06_baiyaoyuan` | `trigger_xieshili` | |
| 42 | `n42_sannian` | `ch07.sannian` | `ch06_baiyaoyuan` | `trigger_sannian` | |
| 43 | `n43_chouhan` | `ch07.dihuo` | `ch07_yuelu_dian` | `trigger_chouhan` | |
| 44 | `n44_shijiu` | `ch07.feidan` | `ch07_dihuo` | `trigger_shijiu` | |
| 45 | `n45_chengdan` | `ch07.chengdan` | `ch07_dihuo` | `trigger_banian` | **「一炉不成就再开一炉——药粉还多」** |
| 46 | `n46_zhuji` | `ch07.done` | `ch07_dihuo` | `trigger_zhuji` | |

文案进 `data/text/ch07_objectives.json`。`ObjectiveRoute` 那三条会沿这条链逐步走门（五道闸门都由链上的旗标开，3.1）；脚本 teleport 十处登 `kScriptTransfers`。

---

## 4. 地图（新建 9 张 ＋ 就地修补 2 张）

| 地图 id | 名称 | 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| 要点 |
| --- | --- | --- | --- | --- | --- |
| `ch07_yuelu_dian` | 岳麓殿 | 48×36 | ✗ | `indoor_stone` / `indoor` / `dust` / backdrop `secret_room`；ambient 冷 | 传送阵大厅（落点，4a）、「器」道（装饰，不通）、「丹」道 → 许老的屋（NPC 许老、柜台前 5a）→ 楼梯 → 藏室（书架、书桌 4b）、无标记通道 → 丑汉石屋（NPC 丑汉、门口 35a）＋ 五彩石门（`portal_to_dihuo`，`require_flag=ch07.dihuo`）＋ 石门前一格（5b）、回黄枫谷的门（传送阵旁）、存档点在大厅 |
| `ch07_dihuo` | 地火屋 | 32×24 | ✗ | `indoor_stone` / `indoor` / `ember` 粒子 / backdrop `secret_room`；ambient 暖红 | 坞石通道（落点，`portal_to_yuelu_dian`）→ 圆形大厅（三十六扇石门，装饰）→ 十九号（门内 35b、圆墩 `facility_dihuo` grade 4、鼎旁 35c、墙角蒲团 36、`facility_dazuo` **不放**：此图的日子只由脚本拨）、存档点在大厅 |
| `ch07_fangshi` | 黄枫谷坊市 | 48×30 | ✓ | `town` / `day` / `dust` / backdrop `town_street` | 北口（落点、11a、回黄枫谷的门）、北段摊位（收药摊 `facility_yaotan` ＋ 摊主 NPC、打探 NPC ×2、执事弟子）、南段店铺（七巧阁、引风斋、天工楼、五行书店、酒楼客栈为装饰；万宝楼门口 11b ＋ 侍从 NPC）、南口（12）、存档点在北口 |
| `ch07_shandong` | 太岳山脉外围·山洞 | 32×24 | ✓ | `wild` / `night` / `firefly` / backdrop `mountain_forest_dusk` | 半山腰乱石挡着的洞（洞内落点、石壁前 13、存档点在洞内）、洞外空地（14）|
| `ch07_jindi_wai` | 禁地外（荒山·黄土坡）| 48×36 | ✓ | `cliff` / `day` / `dust` / backdrop `cliff_top` | 北半荒山（落点、角落 19a、队列 19b、七派人群）；南半黄土坡（石堆、寸草不生；通道口 20、出口 32b、结丹与出来的弟子）；两半之间一条长坡相连（规则 13：落点走得到每个挂点）——剧情里那段数个时辰的飞行用脚本 teleport，不让玩家走；存档点在荒山 |
| `ch07_jindi_waiwei` | 禁地外围 | 48×36 | ✓ | `wild` / `day` / `mist` / backdrop `mountain_forest_dusk`；ambient 灰偏红（腐土、血红小草）| 落点、乌龙潭（东北角、潭边结霜、21a）、蛇蜒树林、山崖与茅草地（21b）、一线天（两侧峭壁夹一条小路，出口 22）、通中心区的口（`require_flag=ch07.yixiantian`）、存档点在落点 |
| `ch07_jindi_zhongxin` | 禁地中心区外 | 48×36 | ✓ | `valley` / `day` / `petal` / backdrop `herb_valley`；ambient 灰 | 树林（落点、林边大树 23）、石墙＋青铜门（24；墙上三具尸烘在底图里）、第一层花园（银菊、血树、紫草、黄竹）、树洞、上山小路口（25）、通环形山的口（`require_flag=ch07.yueyang`）、打坐点两处（林边大树旁、树洞）、存档点在树林入口 |
| `ch07_huanxingshan` | 环形山 | 48×40 | ✓ | `mountain_path` / `day` / `mist` / backdrop `mountain_forest` | 上山小路（落点、遭遇区 `require_flag=ch07.yueyang`）、紫猴花洞（洞内拐角 26a、洞道 26b、石厅口 26c，洞内地面暗）、岔路石（27）、东坡石屋与北坡寒潭（Z2）、近山顶小石殿（28）、山顶石洞（打坐点）、小盆地青石殿（29）、地面破洞（31 的落点、32a）、存档点两处（上山路口、小石殿前）|
| `ch07_dixia_zhaoze` | 地下沼泽 | 36×28 | ✗ | `cave` / `indoor` / `mist`＋`ember` / backdrop `cave_tunnel`；ambient 暗青、湿热 | 石阶出口（落点）、黑土堆后（30）、沼泽（水面）、白玉亭与金箱（装饰）、边上的奇花灵草（装饰）、蛟尸处（31）、存档点在石阶出口 |
| 修补 `ch06_huangfenggu` | 黄枫谷外门 | — | — | — | 加 `portal_to_fangshi`（`require_flag=ch07.chuling`）、`portal_to_yuelu_dian`（`require_flag=ch07.danbao`）；传功阁 `trigger_chuangong`；百机堂 `trigger_chuling`、`trigger_baoming`；山头 `trigger_murong` ＋ 七个 NPC；大殿门前 `trigger_jihe` ＋ 李师祖 NPC |
| 修补 `ch06_baiyaoyuan` | 百药园 | — | — | — | 加 `facility_field_jiao`；居室 `trigger_chaibao`、`trigger_dufang`、`trigger_sannian`；茅屋前 `trigger_yaolou`；园角 `trigger_yuanjiao`；园门内 `trigger_ma_songyao`；茅屋门内 `trigger_xieshili` |

**连通**：

```
ch06_baiyaoyuan ◀──▶ ch06_huangfenggu ──(ch07.chuling)──▶ ch07_fangshi ──teleport(12)──▶ ch07_shandong ──teleport(14)──▶ ch06_baiyaoyuan
ch06_huangfenggu ──(ch07.danbao)──▶ ch07_yuelu_dian ──(ch07.dihuo)──▶ ch07_dihuo ──teleport(36)──▶ ch06_baiyaoyuan
ch06_huangfenggu ──teleport(18)──▶ ch07_jindi_wai[荒山] ──teleport(19b)──▶ [黄土坡] ──teleport(20)──▶ ch07_jindi_waiwei
ch07_jindi_waiwei ──(ch07.yixiantian)──▶ ch07_jindi_zhongxin ──(ch07.yueyang)──▶ ch07_huanxingshan ──teleport(29)──▶ ch07_dixia_zhaoze
ch07_dixia_zhaoze ──teleport(31)──▶ ch07_huanxingshan ──teleport(32a)──▶ ch07_jindi_wai[出口] ──teleport(32b)──▶ ch06_baiyaoyuan
```

- 坊市、岳麓殿、地火屋各有回程的门（不设闸）；**禁地四张图只进不出**：外围没有回禁地外的门，中心区没有回外围的门，环形山没有回中心区的门，沼泽没有门——一律靠剧情 teleport，规则 18 把 teleport 算作边。
- **五道闸门各指一张不同的图**（规则 20）：`ch07.block.fangshi`（出令牌之前：「没有令牌，出不得山门」）/ `ch07.block.yuelu_dian`（没有担保玉符）/ `ch07.block.dihuo`（丑汉还没开门）/ `ch07.block.jindi_zhongxin`（一线天那两个人还堵着）/ `ch07.block.huanxingshan`（雾还没散）。
- 出入口按「往哪边走进门，落点就面朝哪边」摆（规则 29）：**坊市落点在北口、面朝下（南）**，所以黄枫谷通坊市的门摆在南缘、往下走进去；**岳麓殿落点在大厅南端、面朝上（北）**，黄枫谷通岳麓殿的门摆在北缘、往上走进去；回程两道门反过来。关卡路可按第 6 章地图的实际布局调门的位置，调了改本节这一句。
- **每一处挂战斗的格子前面都有存档点**：① 山洞洞内、② 外围落点、③ 中心区外树林入口、④ 环形山小石殿前、⑤ 沼泽石阶出口。
- 命名与地图属性：`chapter=7`、`region` 一律 `jianzhou`（与第 6 章两张图同；禁地在建州以北，ch174）、`display_name_key=ch07.map.<短名>.name`（规则 21）；`bgm`：岳麓殿 `bgm_indoor`、地火屋 `bgm_cave`、坊市 `bgm_town`、山洞 `bgm_night`、禁地外 `bgm_cliff`、外围与中心区外 `bgm_wild`、环形山 `bgm_mountain_path`、沼泽 `bgm_cave`（都是已有的曲子；`tools/audiogen` 的 `MAP_BGM` 登记表跟着改，`docs/audio.md`）。
- `tools/mapgen/genmaps_ch07.py` 接进 `genmaps.py --check`；两处修补照第 5、6 章的做法写成 `genmaps_ch07.patch_huangfenggu()` / `patch_baiyaoyuan()`，由 `genmaps.py` 在第 6 章建完这两张图之后调用；美术 `tools/artgen/artgen.py` 烘焙 9 张新图并 `--check` 过。
- **马师伯只用挂点**（3.1 开头）；**白衣少女、南宫婉、钟吾、封岳、赤脚大汉、菡云芝在禁地四张图上不摆 NPC**（都在挂点脚本里说话）——禁地里没有能「走过去聊聊」的人。

---

## 5. 角色

**新建**（凡说话的人都要有 `data/roles/` 条目；外观进 `data/visual/looks.json`，`sprite_preview.py --coverage` 缺图为 0；非人形敌人进 `tools/artgen/sprites_enemies.py`）：

| id | 名字（名牌）| 身份 | 出处 | 战斗 | 在场 |
| --- | --- | --- | --- | --- | --- |
| `xu_lao` | 许老 | 岳麓殿丹房掌柜，筑基，贪财 | ch154-156 | 否 | 岳麓殿，全程 |
| `chou_han` | 丑汉 | 地火石门看守，炼气顶层，钟掌门近亲 | ch156、213-215 | 否 | 岳麓殿石屋，全程 |
| `murong_xiong` / `murong_di` | 慕容兄 / 慕容弟 | 雷灵根同胞，十一二岁 | ch157-158 | 否 | 黄枫谷山头（`visible ch07.chouhan` / `hidden ch07.murong`）|
| `chen_shimei` | 陈师妹 | 陈家家主独生女，十二层中阶（**不叫陈巧倩**）| ch157-213 | 否 | 山头（同上）；禁地外荒山（`visible ch07.jihe` / `hidden ch07.pojin`）、出口（`visible ch07.pojin` / `hidden ch07.chujindi`）|
| `lanyi_nvzi` | 蓝衣女子 | 慕容兄弟的师姐（**聂盈首见 ch708，不点名**）| ch158 | 否 | 山头（同上）|
| `cuai_qingnian` | 粗矮青年 | 背怪木拐、土墙术 | ch158 | 否 | 山头（同上）|
| `huangfeng_weiguan` | 围观弟子 | 看热闹的外门弟子（**只摆一个**，其余烘进底图）| ch157 | 否 | 山头（同上）|
| `li_huayuan` | 李师祖 | 黄枫谷结丹，带队；**名牌写「李师祖」，全名只在台词里（节点 33 起）** | ch174-213 | 否 | 黄枫谷大殿（`visible ch07.ma_songyao` / `hidden ch07.jihe`）；禁地外荒山 / 出口（同陈师妹两段）|
| `xiang_zhili` | 向之礼 | 十一层老滑头 | ch174-210 | 否 | 禁地外荒山 / 出口 |
| `xiang_shaonian` | 黄毛少年 | 跟着向之礼的十一层少年（死在禁地）| ch174-175 | 否 | 荒山 |
| `huangshan_shijie` | 黄衫师姐 | 陈师妹的同伴，十二层（死在禁地）| ch175、186-188 | 否 | 荒山 |
| `chen_dage` | 陈师兄 | 陈家大公子，金书银笔 | ch199、209、213 | 否 | 出口 |
| `huangfeng_shilian` | 黄衫弟子 | 同行弟子的代表（**只摆一个**，其余烘进底图）| ch174 | 否 | 荒山、出口各一（两段轮流在场；出口那个是活着的第五人）|
| `fuyunzi` | 浮云子 | 清虚门结丹 | ch175-211 | 否 | 荒山 / 出口 |
| `qiong_qianbei` | 穹前辈 | 掩月宗，结丹顶峰、半只脚元婴（「穹老怪」只在旁白里）| ch176-211 | 否 | 出口（`visible ch07.pojin` / `hidden ch07.chujindi`；荒山那一段他隐身来去，不摆）|
| `nichang_xianzi` | 霓裳仙子 | 掩月宗领队 | ch177-211 | 否 | 荒山 / 出口 |
| `jujianmen_gaoren` | 巨剑门高人 | 巨剑门结丹（破禁试剑）| ch179 | 否 | 荒山 / 出口 |
| `qingxumen_daoshi` / `yanyue_dizi` / `jujianmen_dizi` / `lingshoushan_dizi` / `tianquebao_dizi` | 清虚门道士 / 掩月宗弟子 / 巨剑门弟子 / 灵兽山弟子 / 天阙堡弟子 | 各派一个代表（**不摆一排同名 NPC**：规则 30 的群像白名单 `CROWD_ROLES` 与 `kCrowdRoles` 都不归施工路，第 6 章 18.2 第 6 条的教训；人多的样子烘进底图）| ch175-178 | 否 | 荒山、出口各一（两段轮流在场）；化刀坞用已有 `huadaowu_dizi` |
| `luosai_huzi` | 络腮胡子 | 灵兽山，十三层 | ch179、182-184 | 是（②）| 荒山 |
| `yan_xiongdi` | 天阙堡严姓 | 十二层顶峰，胡子的酒肉朋友 | ch183-184 | 是（②）| 只在战斗里 |
| `zichi_bishe` / `chuanshan_shou` | 紫翅碧蛇 / 穿山甲兽 | 胡子的灵兽（**非人形**）| ch184 | 是（②）| 只在战斗里 |
| `duobao_nv` | 白衣女子 | 掩月双娇之一（演出，不打）| ch186-187 | 否 | 只在脚本里说话 |
| `feng_yue` | 封岳 | 天阙堡狂人，十三层顶峰 | ch187-189 | 是（③）| 只在战斗里 |
| `zhong_wu` | 钟吾 | 灵兽山，驱蛇驱蜂 | ch192-209 | 否 | 出口 |
| `yan_wuchi` | 赤脚大汉 | 巨剑门武痴（原著自称言某，**不点姓**）| ch200-202 | 是（④）| 只在战斗里 |
| `han_yunzhi` | 菡云芝 | 灵兽山十层少女（即卖符少女）| ch203-211 | 否 | 出口（`visible ch07.pojin` / `hidden ch07.chujindi`）|
| `baiyi_shaonv` | 白衣少女 | 掩月宗「师祖」（南宫婉，**名字未出口之前**）| ch204-209 | 是（⑤ 友军）| 只在脚本里说话 |
| `mo_jiao` | 墨蛟 | 二阶墨蛟（**非人形**）| ch205-207 | 是（⑤）| 只在战斗里 |
| `tuishan_shou` / `tiebi_yuan` / `huoyan_shu` | 推山兽 / 铁臂猿 / 火焰鼠 | 环形山下阶妖兽（**非人形**）| ch194、196、200 | 是（遭遇、Z2）| 只在战斗里 |
| `fangshi_tanzhu` / `yuanwu_xiushi` / `fangshi_zhishi` / `wanbaolou_shicong` | 收药摊主 / 元武国修士 / 执事弟子 / 万宝楼侍从 | 坊市路人（一人一个 role）| ch161-162 | 否 | 坊市，全程 |
| `tian_buli` / `ding_lao` | 田掌柜 / 丁老 | 万宝楼掌柜与鉴定人（凡人）| ch162-165 | 否 | 只在脚本里说话 |

**沿用**：`ma_shibo`（只说话）、`wu_feng`、`yu_zhishi`、`wang_shishu`（黄枫谷上第 6 章那个 NPC 不动；**禁地外只在脚本里说话**——`NpcPresenceTests` 钉着一个 role 同一时刻只在一张图上，而第 6 章那个没有撤场旗标）、`zhong_lingdao`（只说话）、`lu_shixiong`（**改写**：十二层、木；第 8 节 ①；山头 NPC，同上）、`maifu_shaonv`（**荒山那一段只在脚本里**：太南谷第 6 章那个 NPC 没有撤场旗标，再摆一个她就同时站在两张图上——16.2 第 8 条）、`nangong_wan`（**名牌南宫婉**，出口 NPC，`visible ch07.pojin` / `hidden ch07.chujindi`；数值是第 8 章 b08 两张编成用的，本章不动）。
**不新建**：慕容兄弟以外的大开山门新秀（只在台词里）、萧二与寒天涯（暗场）、乌龙潭的天阙堡蓝衣人与灵兽山二人（演出，只写旁白）、董家那位与红拂（不点名）、马云龙（暗场）。
**不再用**：`zhaoze_yaoshou`、`qingxumen_gaotu`（第 7 章占位编成改写后没人引用；**不删**：`src/core/rules/Realm.h` 注释表把它们当炼气八层、十二层的数据点）。

**撤场 / 出场旗标**（`NpcPresenceTests`）：山头七人 `visible ch07.chouhan` / `hidden ch07.murong`；大殿李师祖 `visible ch07.ma_songyao` / `hidden ch07.jihe`；禁地外荒山一批 `visible ch07.jihe` / `hidden ch07.pojin`、出口一批 `visible ch07.pojin` / `hidden ch07.chujindi`——**同 role 在同一张图上两段在场，一律「前一段的 hidden 恰是后一段的 visible」**（李师祖、陈师妹、向之礼、浮云子、霓裳仙子、巨剑门高人、各派代表与黄枫谷同行弟子）。结丹们在破禁之后就站到了出口（原著他们守在外面五日），玩家那段时间不在这张图上。**目标链之外的在场旗标：无**（全在链上）。

---

## 6. 支线（2 条）

原著本章线头多数是主线的一部分；两条支线都从原著的一句话里长出来，都不改主线的账。

| | Z1 白看一年园子 | Z2 钟吾的玉简 |
| --- | --- | --- |
| 出处 | ch153 担保的代价是替马师伯白看一年园子；ch173 马师伯临别丢下两瓶丹药（面冷心热）| ch193 互换玉简；ch203 他后来去的正是玉简上记的地方，还剩好几处没去；ch160 带出灵药越多赏越重 |
| 触发 | 节点 3 得担保 → `ch07.baigong_qiu = 1` | 节点 24 交换玉简 → `ch07.yujian_qiu = 1` |
| 步骤 | 那一年里照样交药：药篓交一株 **176 年以上的血红芝**（三阶药，一株五滴；节点 9 那四个月里做）| 玉简上环形山的两处各去一次：东坡石屋（门口两只铁臂猿，`be07_tiebi_yuan`，可逃——逃了不置旗标）、北坡寒潭 |
| 完成 | `ch07.baigong = 1`（药篓脚本 `take_aged("herb_xuehong_zhi", 1, 176)` 成功后置）| `ch07.yujian_a == 1` 且 `ch07.yujian_b == 1` |
| 失败 / 过期 | `ch07.ma_songyao` 置位（马师伯送药时还没交）| `ch07.didao` 置位（掉进地道就回不来了）|
| 奖励 | 不进 rewards；节点 17 两瓶各 5（未完成各 3），多一句台词 | 每处 `give` 成熟灵药（东坡：玉髓芝 300 年 ×2；寒潭：天灵果 300 年 ×2）；上交时多交的这几株，节点 32 门派**加赏** `material_lingshi` 50 ＋ `material_lingshi_zhong` 1（脚本读两面旗）|

不做成支线的：定颜丹（原著本章没炼，ch243 才炼七八颗；方子本章可见，玩家若多种了两株千年黄精芝，可在地火屋顺手炼——不设目标、不记账，16.2）；慕容兄弟、蓝衣女子（原著本章之后不再有交集）；菡云芝（原著 ch242、338 只在韩立回忆里）；田掌柜「以后优先卖本楼」（他的决定是少做）。

---

## 7. 系统解锁：药园经营、中阶灵石、法器即法术、配方与地火、秘境、筑基

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **药园经营**（暴富循环）| 三个「时节」（节点 2、3、9），推进条件是**马师伯按年份收药**（44 年黄精 ×3 → 月例 24；百年紫参 ×2 → 月例 60、十一层、担保；园角两株千年黄精芝）；日子靠居室蒲团打坐自己推；园角另开两槽（`field_baiyaoyuan_jiao`）专养千年药；坊市收药摊收多余的药 | **不做农场**：不做浇水松土、不做季节作物、不做每月的收药倒计时；马师伯不催、不罚 | 第 2 章「五个时节」的做法已被验证（`docs/ch02-design.md` 第 2 节）；原著这两年的钱就是月例＋一次卖药，玩家的手感来自「这株浇几滴、先保哪一季」|
| **千年灵草** | 新药材**黄精芝**（五阶，上限 2000 年；`tradeable=false`：店里不收）| 不让千年药进商店 | `herbPrice` 的平方曲线下，一株 1536 年的药按 0.3 收购也值上万灵石——万宝楼的以物易物（脚本）才是它唯一的出口，这也是原著的意思（ch165「这种买卖少做」）|
| **中阶灵石** | 新物品 `material_lingshi_zhong`：战斗里当回法力的药用（ch169 他握着中阶灵石补法力）、地火定金（节点 35）；**不当钱**：商店不以它计价 | 不做兑换、不做「一百低阶兑一块」的柜台 | 原著 ch214：公认兑率有，但没人肯换——游戏里就不给这个按钮 |
| **法器即法术** | 法器驱动靠法力，所以本作把法器做成「祭 X」法术：金蚨子母刃（金、连发）、金光砖（土、一击、贵）、青蛟旗（木）、青凝镜（困敌：削架势）；透明丝线做成**暗器兵刃**（揣在身上普攻多一类）；天雷子、土牢符、火球符、定神符走 E2（道具施法）| 不做装备系统、不做法器耐久；玄铁飞天盾、踏云靴、黑雾小旗只作物品与旁白 | 沿用第 5、6 章剑符「借用 / 常驻」的口径：一门法术＝一样东西；耗尽的当场 `forget` |
| **配方与地火** | 藏室之后定颜丹方、筑基丹方**才出现在炼制面板上**（E1）；筑基丹只在地火屋十九号开得了（`grade 4`，成算约三成）| 不做「真火」这一路；不做炉鼎品质对成丹数的影响；银丝鼎只作物品 | 原著死循环（真火）靠地火解开；面板上提前列出没学到的方子是**名字泄漏**（第 4 章丹房已漏，16.2）|
| **秘境探索** | 禁地五日：四张只进不出的图、每日一横幅；环形山野外遭遇（`ch07_huanxingshan`）；支线 Z2 的两处；出禁地时**嗅灵兽上交**（≥ 100 年的禁地灵药一株不留，幼苗留下）| 不做倒计时、不做随机挪移、不做可重复秘境（大纲 4.4 的「可重复秘境」不在本章）| 五日的节奏由剧情拨；「生还率」写在叙事里，不做成随机死亡 |
| **炼丹（修仙丹药）** | 第一次用地火开炉：先演二十几炉废丹、再把地火交给玩家亲手开炉、半年蒙太奇归一 | 不给筑基丹方加熟练度门槛以外的门 | 熟练度第 4 章起就是 100（世俗丹药），成算靠难度 87 压到约三成——对上 ch215 的「约三次成一次」|
| **筑基** | 脚本服丹：十二、十三、筑基；瓶子容量随境界到 6 滴、凝液五日一滴（E3、既有规则）| 面板冲关不给筑基丹加成（`tryBreakthrough` 的 `pillBonus` 照旧 0）| 原著筑基是八粒丹药硬推的，一次性剧情事件；面板冲关留给第 8 章以后的小境界 |

**配方数据**（内容路按此写，E1 要求的门闸字段见契约）：

| 配方 | 输入 | 难度 | 熟练度门槛 | `requireFlag` | 说明 |
| --- | --- | --- | --- | --- | --- |
| `recipe_zhuji_dan` 筑基丹方 | `material_zhuji_yaofen` ×1（**一炉份的筑基丹药粉**，三主药＋三十一味辅药按方研好，ch214 他按份量装进白玉瓶）| **87** | 30 | `ch07.cangshi` | 熟练 100、资质 50、地火 grade 4：40 + 50 + 10 + 20 − 87 = **33%**。失败按炼丹口径尽毁（一份药粉）|
| `recipe_dingyan_dan` 定颜丹方 | `herb_huangjing_zhi` ×2，`minAge 1000` | 60 | 30 | `ch07.cangshi` | 地火下约六成；本章只可见、不必炼 |
| `recipe_jiedan_lingyao` 结丹灵药方 | 不动 | 不动 | 不动 | `story.recipe_later` | **堵泄漏**（第 4 章丹房起就把「结丹灵药方」列在面板上）；写到那一章换成那一章的旗标 |
| `recipe_huoqiu_fu` / `recipe_hushen_fu` / `recipe_jinci_fu` / `recipe_leiming_fu` | 不动 | 不动 | 不动 | `story.recipe_later` | **堵泄漏**（第 6 章制符桌上熟练度一过门槛，拒绝理由里就会出「一级妖丹」——妖丹首见 ch344）；ch242 制符那一章换旗标 |

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** = 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **5 场必打**（① 节点 13 陆师兄、② 节点 22 一线天、③ 节点 23 封岳、④ 节点 28 赤脚大汉、⑤ 节点 30 墨蛟），**全部是原著的仗**，没有改编仗；可选：环形山野外遭遇、Z2 东坡石屋。
第 1-6 章合计 19 场，**第 1-7 章合计 24 场**；方案 P3 的 52 场余 28 场给第 8-14 章（每章约 4 场）。本章原著真打的恰是这五场（外加巨蜈蚣、推山兽、灵兽山伏击者三次一击了事的——做成设伏、遭遇、脚本，8.3）。

### 8.1 意图玩家

十一层、156 血 / 110 法力；法术：火弹术（8）、流沙术（12）、冰冻术（10）、天眼术（5）、祭剑符（40，① 之后没了）、祭金蚨子母刃（≈ 12，连发）、祭金光砖（≈ 40，⑤ 之后没了）、祭青蛟旗（≈ 20，① 之后）、祭镜（≈ 15，③ 之后；原名「祭青凝镜」，校对 LOW-3 改名，节点 28 之前不点破镜子的名字）；兵刃：拳、剑（软剑、烈阳剑）、刀（冷月刀）、暗器（丝线，② 之前拿到）。

**道具按脚本推出来的实际背包写**（校对 LOW-8：原先这里写着「年份草药（黄精回血、紫参回法力）」，可黄精、紫参在节点 2、3 就交给了马师伯，① 时背包里一株年份草药也没有——没做 Z1 的那株血红芝除外）。「必有」是每个存档都有的，「可能有」是玩家自己挣或自己留的；数值与打法归平衡路（`docs/ch07-balance.md`）。

| 场 | 必有 | 可能有 |
| --- | --- | --- |
| ① 陆师兄 | 中阶灵石 2（节点 1）、土牢符 2（节点 1，E2）、天雷子 1（节点 11，E2）；第 6 章带来的火球符、护身符、金疮药（两侧数目不同，第 2 节） | 第 6 章剩下的养精丹、定神符；坊市收药摊买的回气丹、清灵散；执事那两张火球符；没交 Z1 的那株血红芝 |
| ② 一线天 | 上一行 ＋ 陆师兄身上的火球符 4、护身符 2（节点 13）；马师伯的养精丹、金疮药各 3 或 5（节点 17）；中阶灵石再 ＋1（节点 18） | 同上 |
| ③ 封岳 | 同上（② 里用掉的不回来） | 天雷子：没在 ①② 扔掉才有——③ 必须在没有它时也打得赢（验收 4） |
| ④ 赤脚大汉、⑤ 墨蛟 | 同上 ＋ 祭镜（③ 之后） | 同上 |

### 8.2 五张必打编成 ＋ 可选

| # | 编成 id | 敌方 | 友军 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b07_lu_shixiong`（改写占位）| 陆师兄（十二层，**木**＝风灵根（巽属木）；架势 5；破绽**金、火**；法术青弧斩（木）；蓄势「化蛟」每 3 回合 ×2.0 单体；**maxHp 钉 180**——它是 `tests/RealmTests.cpp` 的曲线锚点，十二层 168 的 ±15% 内，10.1 E7）| — | 否 | **致命** | **法力之战**——实测是**两回合、剑符收场、花掉四成多法力**（`docs/ch07-balance.md` 第 3 节 ①：第 1 回合火弹探出「火」，第 2 回合劲攒到 2 点、剑符一发收场；挨两记青弧斩，法力 110 → 62，不吃药）。原先这里写「法力撑不到第三轮、要吃中阶灵石才耗得过他」，量不出来：那只手不吃回法力的东西，剑符在手、劲一攒够，第 2 回合就收场（平衡路第 8 节第 3 条，协调者转，18.17）。破绽金火都好打。天雷子若在这里扔掉，③ 要换打法（8.3）|
| ② | `b07_yixiantian`（新）| 络腮胡子（十三层，木＝绿罩；架势 3；破绽**土、金**；行动 1）、天阙堡严姓（十二层顶峰，土；架势 2；破绽**暗器、火**；**气血低**——没放护罩）、紫翅碧蛇（木；架势 2；破绽火）、穿山甲兽（土；架势 2；破绽木、水）| — | 否 | **致命** | **先除没罩子的、再困住有罩子的**：丝线（暗器）一两下收掉严；土牢符（E2，削 3 点架势）让胡子当场破势——破势窗口里金光砖（土，×2）一砖；两只灵兽是分心的。四个敌人一齐上是本章最乱的一仗，单轮伤害按 `docs/octopath-battle.md` 7.3 的斩杀线口径、由平衡路拿 `BattleHand` 量|
| ③ | `b07_fengyue`（新）| 封岳（十三层顶峰，土；架势 6；破绽**火、木**；**防极高**＝黄罗伞；行动 2＝踏云靴；蓄势「小刀符宝」每 2 回合 ×1.8 单体）| — | 否 | **致命** | **天雷子（E2，火，威力归平衡路）一掷定局**——原著的解法；没有天雷子（已在别处扔了）：青蛟旗（木）或火弹（火）打破绽破势，破势窗口里金光砖；**不用 `killable_by`**：天雷子是消耗品，玩家可能在前两场就扔了；拿 `killable_by` 逼它只会逼出死局 |
| ④ | `b07_zhongxinqu_duoyao`（改写占位）| 赤脚大汉（十三层，**金**＝银辉剑；架势 5；破绽**暗器、火**；**不放护罩**；蓄势「银盘」每 3 回合 ×2.0 单体）| — | 否 | **致命** | **定住他的剑，贴身一刀**：金蚨子母刃（金对金）打不动；青凝镜（削 2 点架势）＋火弹打破绽破势，破势时丝线（暗器）近身——原著「十丈之内、没放护罩」。武痴不逃不求饶 |
| ⑤ | `b07_zhaoze_shouyao`（改写占位）| 墨蛟（二阶，**水**；数据写炼气十三层——**叙事上可比筑基中阶，数值靠血防**，跨大境界压制 0.7 / 1.6 会让这一仗失控；架势 6；破绽**土、火**；**`killable_by: ["土"]`**；行动 2；蓄势「紫液」每 3 回合 ×1.8 **全体**）| **白衣少女**（`baiyi_shaonv`，法力只剩炼气顶峰、法宝朱雀环＝火攻；按模板满血上场。`ch07.mojiao` 的两个选项只改台词，**编成只有一张**）| 否 | **致命** | **她烧、你砸**：朱雀环（火）打破绽破势，破势窗口里金光砖（土，×2）；只有「土」要得了它的命（原著：金刃砍上只留白痕，只有金光砖破得了它的防）；紫液全体重招——在它出手之前破势打断，或者防御扛。友军照既有 AI 出手|
| 可选 | `be07_tuishan_shou` / `be07_tiebi_yuan` / `be07_huoyan_shu`（新）| 推山兽（土，破绽木、暗器）/ 铁臂猿 ×2（破绽火、剑）/ 火焰鼠 ×3（破绽水）| — | 是 | 不致命 | 环形山遭遇（`dailyCap 2`，区 `require_flag=ch07.yueyang`）；`be07_tiebi_yuan` 兼作 Z2 东坡那一场。**低于同章剧情战**（契约口径）|

- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，10.1 E6）：`CHAPTER_MEANS[7]` = 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07：节点 11 万宝楼必经，在五场之前）；
  `BATTLE_EXTRA_MEANS`：②③④⑤ 与环形山遭遇 `be07_tuishan_shou` 各加 木（`magic_ji_qingjiao`，ch07：节点 13 战后）与 暗器（`give("weapon_wuming_sixian")`，ch07：节点 21b）——五场都在节点 22 之后（遭遇区要 `ch07.yueyang`）；① 不加（那时两样都还没有）。`MEANS_LAST_CHAPTER = 7`。规则 25 按编成的 `chapter` 字段认章，遭遇编成也写 7、也在它的范围里。
  **金光砖（土）不登**：它在 ⑤ 之后被 forget，但五场都在那之前——登不登都成立；不登是因为流沙术已经给了「土」，少一条会漂的来路。
- **数值归战斗路按 `tests/BattleHand.h` 那只手量**；本表只写意图与破绽。必须守住的数：陆师兄 maxHp 180（锚点）、⑤ 的 `killable_by`、各场 `defeat_is_fatal`。**实测**（五场的回合、血、法力、吃药，天雷子与土牢符省不省力，斩杀线，五只手扫描，两侧通关的终局）见 `docs/ch07-balance.md` 第 0、3–5、7 节，本表不抄。
- 战斗背景（`data/visual/battles.json`）：① `mountain_forest_dusk`、② `cliff_top`、③ `mountain_forest`、④ `mountain_forest`、⑤ `cave_tunnel`、遭遇 `mountain_forest`。**每一条都要能指出读它的那一行代码并有走真实入口的测试**（handoff 第 6 节第 5 条）。
- **结算卡**：五场的 `rewards.spirit_stones` 都是 0、`drops` 空（东西全由脚本搜身给，第 9 节）；修为照给（数归平衡路）。
- 占位编成的旧 `intro_key`（`ch07.battle.*.intro`，在 `data/text/battles.json` 里）**请协调者删**，新 key 进 `data/text/ch07_battle.json`（第 5 章 18.2 第 4 条的先例）。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 陆师兄 | ch167-169 全写了 | 照原著；「三层防御」「撤水罩省法力」做成旁白与意图 | — |
| ② 一线天 | ch183-184：胡子被困、严被隔空取首、胡子放出两只灵兽 | 四个敌人同时在场（原著灵兽是陆续放出的；引擎没有「召唤」）| 顺序与结局照原著；只是把「放出来」前移到开场 |
| ③ 封岳 | ch186-189：多宝女那一段他是在打的，打到一半被封岳截胡 | **多宝女那一段做成演出**（她的青凝镜、水晶球、他的青索银钩尽失都在旁白里），只打封岳 | 多宝女死于封岳之手是原著；让玩家先把她打到一半再被截胡，等于逼玩家白打一场，还要处理「玩家先把她打死了怎么办」|
| ④ 赤脚大汉 | ch201-202 全写了 | 照原著 | — |
| ⑤ 墨蛟 | ch205-207：掩月宗众人先斗黑墨蛟、蜕皮后他出手 | 前半是**观战演出**，只打蜕皮之后的；友军白衣少女 | 原著他就是在她法力将尽时加入的 |
| 巨蜈蚣 | ch197-198：倒插子刃暗算，没有正面交手 | **设伏**：三个挂点（窥探、插刃、诱敌），不开战斗 | 这是全章最「韩立」的一场——做成一仗反而把算计变成了数值 |
| 推山兽 | ch196：丝线一挥 | 环形山遭遇 | — |
| 灵兽山伏击者 | ch209：身形一闪 | 脚本一行 | 他那时有南宫婉渡来的十三层法力 |

**天雷子与 ③ 的关系**：天雷子是 E2 道具，玩家可以在 ①② 就扔掉。**③ 必须在没有天雷子时也打得赢**（封岳破绽火、木；破势后金光砖）——平衡路要量「没有天雷子」这一路，那只手要证明它赢得了（验收 4）。
**不做的仗**：慕容兄弟对陆师兄（他在旁看）、乌龙潭（他敛气不动）、多宝女（演出）、钟吾的飞蛇（兜圈、说话，不开打）、掩月宗对黑墨蛟（观战）、破土而出（脚本）。

### 8.4 野外遭遇（`ch07_huanxingshan` 一块区）

按 `docs/interfaces-octo-encounters.md`：表 `data/encounters/ch07_huanxingshan.json`，条目 `be07_tuishan_shou`、`be07_tiebi_yuan`、`be07_huoyan_shu`（原著环形山里一级下阶的火焰鼠之类，ch194），`dailyCap 2`；区 `require_flag=ch07.yueyang`；外围、中心区外不放（原著那两处的死都是人杀的）。遭遇的每日上限按实际日子算；玩家在打坐点多坐几天，遭遇也就多几轮——都是可逃的小仗，不影响账。

---

## 9. 经济账

**两本账：灵石（低阶，`material_lingshi`）与三样硬货（中阶灵石、筑基丹、禁地灵药）**。

**来源**

| 项 | 灵石 | 中阶灵石 | 筑基丹 | 其他 |
| --- | --- | --- | --- | --- |
| 第 6 章带来 | 112 / 82 | 0 | 0 | 定神符 1 / 0 |
| 1 拆包 | | ＋2 | | 精钢环、黑雾小旗、黄铜瓶、土牢符 2；苗 8 株 |
| 2 交药一 | ＋24 | | | |
| 3 交药二 | ＋60 | | | |
| 11 万宝楼 | | | | 两株千年黄精芝 → 金蚨子母刃、飞天盾、天雷子、金光砖 |
| 13 陆师兄 | ＋20 | | ＋2 | 青蛟旗、青索、银钩、火球符 4、护身符 2 |
| 17 马师伯 | | | | 养精丹 3/5、金疮药 3/5 |
| 18 大殿 | | ＋1 | | 布包（节点 34 打开时换成傀儡弓手，校对 LOW-3）|
| 21–31 禁地 | | | | 丝线、黑靴（节点 24 换成踏云靴）、小圆镜（节点 28 换成青凝镜）、水晶球、小刀符宝、银辉剑、蜈蚣壳 3、墨蛟材料；禁地三药：成熟 22 株（＋Z2 4 株）、幼苗 18 株 |
| 32 门派赏 | Z2 ＋50 | Z2 ＋1 | | 碧光刀 |
| 34 奖赏、清点、叶补还 | ＋300 | ＋10 ＋5 | ＋1 | 回气丹 2、银色书页、青元剑诀；幼苗 → 药粉 40 份 |
| 35 自炼 | | | ＋22（归一）| 一盒废丹 |
| 可选：坊市收药摊（sellRate 0.3）| ＋0～≈1300（收得了的只有黄精、紫参——血红芝 `tradeable=false`，16.5 M4；坊市的门只看 `ch07.chuling`，节点 10 之后随时回得去，校对 LOW-7）| | | |

**去向**

| 项 | 灵石 | 中阶灵石 | 筑基丹 | 其他 |
| --- | --- | --- | --- | --- |
| 2、3、Z1 交药 | | | | 44 年黄精 3、百年紫参 2、（176 年血红芝 1）|
| 4b 藏室 | −1 −1 −20 | | | |
| 5a 银丝鼎 | −32 | | | |
| 11 万宝楼 | | | | 千年黄精芝 −2 |
| 13 陆师兄 | | −k（战斗里用掉的）| | 剑符 −1、精钢环 −1、定神符 −1（有则）|
| 23 多宝女那一段 | | | | 银钩 −1、青索 −1 |
| 31 破土 | | | | 金光砖 −1 |
| 32 嗅灵兽 | | | | 禁地三药 ≥ 100 年全数（22，Z2 另 4）|
| 34 三年 | | | | 禁地三药幼苗全数 → 药粉 |
| 35a 传送阵、定金 | −1 | −1 | | |
| 35b 废丹 | | | | 药粉 −22 |
| 35c 半年 | | | | 药粉余量全数 |
| 36 服丹 | | | −8 | |

**账要对得上的几条**（`Ch07LedgerTests` 直接读 `scripts/ch07/*.lua` 与 `data/battles/b07_*.json`）：

1. **节点 4b 之前灵石只进不出**：两次交药之后最低 82 + 24 + 60 = **166** ≥ 许老与银丝鼎的 54——藏室那几处 `take` 在意图玩家身上一定成功（两侧都成立）；脚本仍看返回值、不够就 `return`（不设死局）。
2. 本章脚本 `give("pill_zhuji_dan")` 只有三处：节点 13（2）、节点 34（1）、节点 35c（**差额补足到 25**，差额 ≤ 0 不给）；`take("pill_zhuji_dan")` 只有一处：节点 36（8）。**章末恰 17**（两侧同账）。
3. 五张必打编成 `rewards.spirit_stones` 都是 0、`drops` 都空；灵石的 `give` 只在节点 2（24）、3（60）、13（20）、32（Z2 的 50）、34（300）。
4. `herb_huangjing_zhi`、`herb_yusui_zhi`、`herb_zihou_hua`、`herb_tianling_guo`、`pill_zhuji_dan`、`material_zhuji_yaofen` 全部 `tradeable=false`（店里卖不掉：千年药与禁地药卖了会把第 8 章的钱撑爆，筑基丹卖了会断第 10 章「连服三颗」的账）。
5. 嗅灵兽之后（`ch07.chujindi`）背包里 `item.count_aged(三药, 100)` 都是 0、幼苗（< 100 年）一株不少：**两侧同账**。
6. **章末灵石**意图玩家 ≈ 起点 + 349（first 461 / second 431，不含 Z2 与收药摊），中阶 ≈ 17 − k；第 8 章设计从 `ch07-end` fixture 读，不从本表抄。
7. **本章可卖年份药的上限**（校对 MEDIUM-4 建议补的断言）：收药摊收得了的年份药只有节点 1 的黄精、紫参，每一味推到顶卖一株都不超过封顶约一千三；血红芝 `tradeable=false`（16.5 M4：推到 300 年一株 3,460）。`Ch07LedgerTests.TheMarketRoadNeverPaysMoreForOneHerbThanTheWholeChapterCap` 钉住。

**对照：原著这一段的量级**。ch163 他去万宝楼时身上是中阶 2、低阶近百；ch213 禁地清点中阶十余、低阶数百——本表去万宝楼时低阶 112–142、章末低阶 430–460，同一个量级。坊市收药摊那一路是玩家自己挣的「暴富」，封顶约一千三（校对时若远超这个数，说明哪味药的 `tradeable` 或上限漏了——校对果然查出血红芝漏了，见第 7 条）。

---

## 10. 引擎前置

### 10.1 清单

| # | 项 | 必须 / 可选 | 谁做 | 说明 |
| --- | --- | --- | --- | --- |
| E1 | **配方按旗标列出**：`rules::Recipe` 追加 `requireFlag`（配方 JSON 同名驼峰 `"requireFlag"`，与 `requiredProficiency` 一个写法；缺省无）；`AlchemyScene::visibleRecipes` 收 `state`、旗标未置的方子**不列**；`requireFlag` 的登记由既有的 `*_flag` 检查兜住（字段名归一化后以 `_flag` 结尾）（`check_data_references`），不另立规则 | **必须** | 引擎路 | 定颜丹方、筑基丹方在节点 4b 之前不许有名字（ch155）；顺带堵第 4–6 章面板上「定颜丹方」「结丹灵药方」「一级妖丹」的名字泄漏（16.2 第 2 条）。判据：同一个存档、旗标 0 / 1 两态的配方列表；负向：把过滤去掉，面板用例红 |
| E2 | **道具施法**（第 6 章拍板放到本章的「符箓进战斗」）：`Item` 追加 `castMagic`；`battleUsable` 认它；用一件 = 以那门法术施展一次、**不耗法力**、扣一件；新效果 **`stagger`**（削 N 点架势，不看破绽、不伤血）| **必须** | 引擎路 | 天雷子（③ 的原著解法）、土牢符（② 的原著解法）、火球符、定神符。判据与负向见契约第 2 节 |
| E3 | **瓶子容量随境界**：炼气 3、筑基 6、结丹 9，只补不削；升境（脚本、面板）与读档三处 | **必须** | 引擎路 | 本章是全作第一次跨大境界（节点 36）；Bottle.h 早写明「由 game 层按境界写进」，至今没人写——不在这一章补，`ch07-end` fixture 就会带着「筑基初期、瓶子三滴」的不一致交给第 8 章 |
| E4 | **脚本按年份下限计数与扣物**：`item.count_aged(id, min_age)`、`take_aged(id, count, min_age)` | **必须** | 引擎路 | 马师伯收药（≥ 44 / 100 / 176 年）、万宝楼（≥ 1000 年）、嗅灵兽（≥ 100 年全数）都要按年份下限；现有 `take(id, n, age)` 只认**恰好**那个年份（千年药浇到 1536 年就对不上），省略年份时又**先扣最嫩的**（`GameState::removeItem`）——交药会把刚种下的苗交上去 |
| E5 | 数据：祭器法术 4 门（`magic_ji_jinfu` 金 / 连发、`magic_ji_jinguangzhuan` 土 / 一击、`magic_ji_qingjiao` 木、`magic_ji_qingning` effect stagger 2）＋ 敌方法术 6 门（青弧斩、小刀符宝、银辉剑、黑水、紫液、朱雀环）；`magic_ji_jianfu` 只改 note。**道具专用的 3 门**（`magic_tianleizi` 火、`magic_tulao_shu` stagger 3、`magic_dingshen_shu` stagger 1）连同天雷子、土牢符两件物品与火球符、定神符的 `castMagic` 字段，**归引擎路**（E2 的判据要在真战斗里用它们；契约 6.1）| 必须 | 内容路（E2 那几份：引擎路）| 数值归平衡路 |
| E6 | `CHAPTER_MEANS[7]`、`BATTLE_EXTRA_MEANS`（8.2）、`MEANS_LAST_CHAPTER = 7`；`validate_selftest.py` 里「第 7 章的敌人不在规则 25 范围 → 不报」那一条改指 `b08_*`（MEANS 改 7 之后它就会报）| **必须** | 协调者（集成时）| `tools/validate.py` 与 `validate_selftest.py` **整份归协调者**（契约第 5、6 节）：E1–E4 要的形状检查、规则 25 的「give 的兵器类别要核 `weapon` 字段」，在派工之前先落一批；这一行的表在集成时落 |
| E7 | **测试依赖**：`tests/RealmTests.cpp` 锚点 `{QiRefining11, 155, "lu_shixiong"}` → `{QiRefining12, 180, "lu_shixiong"}`（`Realm.h` 注释表同步）；`tests/Ch03BattleKitTests.cpp` 用 `b07_lu_shixiong` 验「敌人真的施法」——陆师兄的法术表里必须留一门伤人法术且法力够放 | 必须 | 引擎路（RealmTests、Realm.h 注释）／内容路（守住陆师兄的数）| 数据先落、测试没改会当场红；两件事写在同一张契约里 |
| E8 | `LexiconTests` 表一补词（12.1）| **必须** | 协调者 | |
| E9 | `data/chapters.json` 加第 7 章；`ui.json` 加 `ui.chapter.07.numeral` / `.title`（「第七章」「血色试炼」）| 必须 | 协调者 | 章节卡按完成旗标自动排 |
| E10 | 新物品 31 件（天雷子、土牢符两件归引擎路，其余 29 件内容路；清单在契约第 6 节）＋ 改 1 件（筑基丹 `tradeable`）、配方改写 2（筑基丹方、定颜丹方；另外 5 张的门闸归引擎路）、角色 43 新建 ＋ 1 改写、编成 2 新 ＋ 3 改写、遭遇表 1、遭遇编成 3、商店 1、任务 2、路径行动约 14 条 | 必须 | 内容路 | 物品描述受第 12 节禁词约束 |
| E11 | `tools/mapgen/genmaps_ch07.py`（9 张 ＋ 两处 patch）接门禁；`maps.json` 9 条；`looks.json` 约 40 条（非人形 6 个进 `sprites_enemies.py`）；`battles.json` 8 条；`MAP_BGM` 9 条；artgen 烘焙 | 必须 | 关卡 / 美术路（内容路）| |
| E12 | 通关测试 `Ch07Walkthrough`：起点 `ch06-end-*.sav` 两侧，终点写 `tests/fixtures/ch07-end-{first,second}.sav`；`Ch07TriggerModeTests` / `Ch07BattleDataTests` / `Ch07LedgerTests` / `Ch07SliceTests`；`kScriptTransfers` 十条；第 6 章几张按章计数的用例排除第 7 章的 patch（契约 5.3）| 必须 | 测试路（**另一个代理**）| |
| O1 | 修炼面板「师承　李师祖（记名）」一行（照第 6 章灵根一行的做法）| 可选 | 引擎路 | **推荐不做**：不影响本章任何一条验收 |
| O2 | 踏云靴给身法（我方行动序）| 可选 | — | **不做**：没有装备系统；原著的快写在旁白里 |
| O3 | 禁地专用地图主题与战斗背景 | 可选 | — | **不做**：现有 `wild` / `valley` / `cave` 加 ambient 覆写够用；做要改 `tools/artgen/` |
| — | 拍卖、`Realm::demote`、洞府、阵法傀儡、分神 | **不做** | — | 第 8、9 章 |

**查过、本章不需要新做的**：友军单位（⑤，第 5 章夺帮已有）；首领蓄势与多次行动（①–⑤）；`killable_by`（⑤）；天眼术看破（第 6 章 E1）；`realm.advance` 跨大境界（`tryNext` 已通 13 → 21，`liftToFloor` 已按 `realmMaxHp(21)` 抬）；多块灵田（`field.unlock` 与面板按 `ref_id` 分开）；商店卖出（收药摊）；凝液随境界加快（`bottleChargeDays` 筑基 5 日，已有）；路径行动、遭遇、任务、目标链。

### 10.2 判据的要点（细节在契约）

- E1：旗标未置 → 列表里没有这一行、行号往前挪；置了 → 有、置灰理由照旧（缺料 / 火候）；**行号 → 方子 id 那张表（`AlchemyScene::recipeIds_`）与列表同源**（行号不许错位到别的方子上）。
- E2：天雷子一件、火球符一件在同一场各用一次 → 背包各少一件、法力一点不少；土牢符对架势 3 的敌人 → 当场破势；对已破势的 / 没有架势的 → **拒绝在先**（理由说清楚），道具与法力都不扣——与「解毒药用在没中毒的人身上」同一口径。
- E3：炼气十三层 → `realm.advance(21)` → 容量 6；再读档仍 6；手改存档容量 8 → 读档后仍 8（只补不削）。
- E4：`count_aged` 与 `take_aged` 对「44 年 2 株 ＋ 11 年 3 株」这样的背包：`count_aged(44) == 2`；`take_aged(3, 44)` 失败且一株不动；`take_aged(2, 20)` 扣的是两株 44 年（只有它们够格）。

---

## 11. 旗标清单（交协调者登记；施工方不碰 `data/flags.json`）

| 旗标 | 值 | 谁置 | 谁读 |
| --- | --- | --- | --- |
| `ch07.chaibao` | 1 | 1 | 园角田 `require_flag`；药篓 guard |
| `ch07.jiaoyao1` | 1 | 2（药篓）| 药篓分支；目标链 |
| `ch07.danbao` | 1 | 3（药篓）| 岳麓殿那道门；4a guard |
| `ch07.baigong_qiu` | 1 | 3 | Z1 接取 |
| `ch07.baigong` | 1 | Z1（药篓）| Z1 完成；17 两瓶的数 |
| `ch07.yuelu` | 1 | 4a | 4b guard |
| `ch07.cangshi` | 1 | 4b | **定颜丹方、筑基丹方的 `requireFlag`**；5a guard |
| `ch07.yinsi` | 1 | 5a | 5b guard |
| `ch07.chouhan` | 1 | 5b | 山头七人出场；6 guard |
| `ch07.murong` | 1 | 6 | 山头七人撤场；7 guard |
| `ch07.dufang` | 1 | 7 | 传功阁 guard |
| `ch07.lianqi` | 1 | 8（传功阁）| 9 guard；脚本里「敛气」的前提 |
| `ch07.qiannian` | 1 | 9 | 10 guard |
| `ch07.chuling` | 1 | 10 | 坊市那道门；11a guard |
| `ch07.fangshi` | 1 | 11a | 11b guard |
| `ch07.wanbaolou` | 1 先亮一株 / 2 两株齐亮 | 11b | 12 guard；田掌柜一句 |
| `ch07.likai` | 1 | 12 | 13 guard |
| `ch07.yeyu` | 1 等破绽 / 2 这就动手 | 13 | 14 guard |
| `ch07.fanhui` | 1 | 14 | 传功阁第二回的前提 |
| `ch07.fudan_fa` | 1 | 15（传功阁）| 16 guard |
| `ch07.baoming` | 1 龙鳞果 / 2 不解释 | 16 | 17 guard；32b 王师叔一句 |
| `ch07.ma_songyao` | 1 | 17 | 大殿李师祖出场；18 guard；Z1 过期 |
| `ch07.jihe` | 1 | 18 | 大殿李师祖撤场；荒山一批出场；19a guard |
| `ch07.xiang` | 1 回绝 / 2 敷衍 | 19a | 19b guard；32b 一句 |
| `ch07.qipai` | 1 | 19b | 20 guard |
| `ch07.pojin` | 1 | 20 | 荒山一批撤场、出口一批出场；21a guard |
| `ch07.wulongtan` | 1 | 21a | 21b guard |
| `ch07.sixian` | 1 | 21b | 22 guard |
| `ch07.yixiantian` | 1 | 22 | 通中心区的门；23 guard |
| `ch07.fengyue` | 1 出手 / 2 想躲 | 23 | 24 guard |
| `ch07.zhongwu` | 1 认 / 2 含糊 | 24 | 25 guard |
| `ch07.yujian_qiu` | 1 | 24 | Z2 接取；Z2 两处 guard |
| `ch07.yujian_a` / `ch07.yujian_b` | 1 | Z2 两处 | Z2 完成；32b 加赏 |
| `ch07.yueyang` | 1 | 25 | 通环形山的门；遭遇区；26a guard |
| `ch07.kuitan` | 1 | 26a | 26b guard |
| `ch07.mairen` | 1 | 26b | 26c guard |
| `ch07.zihouhua` | 1 | 26c | 27 guard |
| `ch07.sichu` | 1 | 27 | 28 guard |
| `ch07.wuyou` | 1 让她忘 / 2 让她立誓 | 28 | 29 guard |
| `ch07.didao` | 1 | 29 | 30 guard；Z2 过期 |
| `ch07.mojiao` | 1 再等等 / 2 这就出手 | 30 | 31 guard；战后她那一句冷言 |
| `ch07.nangong` | 1 | 31 | 32a guard |
| `ch07.xiashan` | 1 | 32a | 32b guard |
| `ch07.chujindi` | 1 | 32b | 出口一批撤场；33 guard |
| `ch07.baishi` | 1 迟疑再应 / 2 当场叩拜 | 32b | 第 8 章（李化元那边的态度）|
| `ch07.xieshili` | 1 | 33 | 34 guard |
| `ch07.sannian` | 1 | 34 | 35a guard |
| `ch07.dihuo` | 1 | 35a | 地火那道门；35b guard |
| `ch07.feidan` | 1 | 35b | 地火设施 `require_flag`；35c guard |
| `ch07.chengdan` | 1 | 35c | 36 guard |
| `ch07.done` | 1 | 36 | 章末；第 8 章入口 |
| `story.recipe_later` | —（**永不置**）| 无 | 结丹灵药方与四张制符方的 `requireFlag`（占位；描述写明「写到那一章时换成那一章的旗标」）|
| `ch07.path.*` | | 路径行动 | 只由路径行动置（**等 `data/pathactions/ch07.json` 落地后登记**，门禁规则 26）|

不新增别的 `story.*`。**目标链之外的在场旗标：无**。`story.recipe_later` 是全表唯一一面「登记了却没人置」的旗：门禁只对路径旗标、目标链、任务谓词查「有没有人置」，配方引用它不触发任何一条；契约第 1 节写明它只许配方用。

---

## 12. 禁词与首见章号

### 12.1 交协调者：`tests/LexiconTests.cpp` 表一要补的词

逐个 `novel.py find` 查过首见（2026-09-29）。补之前扫过 `data/text/ch0[1-6]*.json`（34 个文件，含第 6 章正在写的 7 个，2026-09-29）：下列词**命中 0 条**，补进去不会让已有章节转红。

| 词 | 原著首见 | 证据 | 效果 |
| --- | --- | --- | --- |
| 先天真火 | ch155 | 筑基丹方的死循环 | 第 1-6 章禁 |
| 地火 | ch156 | 许老推销丹炉那一句 | 第 1-6 章禁（「地火屋」ch215 已在表里）|
| 地肺之火 | ch156 | 「地火」管不住它（中间隔着两个字）| 第 1-6 章禁 |
| 玉髓芝 | ch158 | 筑基丹三主药之一 | 第 1-6 章禁 |
| 紫猴花 | ch158 | 同上 | 第 1-6 章禁 |
| 天灵果 | ch158 | 同上 | 第 1-6 章禁 |
| 敛气术 | ch161 | | 第 1-6 章禁 |
| 万宝楼 | ch162 | 章题 | 第 1-6 章禁 |
| 天雷子 | ch163 | | 第 1-6 章禁 |
| 金光砖 | ch165 | ch164 只说是一块金色的长砖 | 第 1-6 章禁 |
| 封岳 | ch187 | 章题「黄雀封岳」| 第 1-6 章禁 |
| 钟吾 | ch193 | | 第 1-6 章禁 |
| 月阳宝珠 | ch194 | | 第 1-6 章禁 |
| 穹老怪 | ch196 | ch176 起只叫「穹前辈」| 第 1-6 章禁 |
| 青凝镜 | ch202 | | 第 1-6 章禁 |
| 菡云芝 | ch203 | 太南会时无名（第 6 章叫卖符少女）| 第 1-6 章禁 |
| 墨蛟 | ch205 | | 第 1-6 章禁 |
| 素女轮回功 | ch208 | lore 误作「女轮回功」| 第 1-6 章禁 |
| 剑影分光术 | ch217 | 青元剑诀后三层的神通（第 8 章开篇）| 第 1-7 章禁 |
| 灵眼之泉 | ch220 | | 第 1-7 章禁 |
| 天罗国 | ch221 | | 第 1-7 章禁 |
| 神风舟 | ch228 | 墨蛟鳍尾炼的——本章只给「墨蛟材料」| 第 1-7 章禁 |
| 青竹蜂云剑 | ch241 | 本作物品早就存在（`weapon_qingzhu_fengyun_jian`，名字不在扫描范围）| 第 1-7 章禁 |
| 董萱儿 | ch245 | 陆师兄口里的「董家那丫头」本章不点名 | 第 1-7 章禁 |
| 鬼灵门 | ch249 | | 第 1-7 章禁 |
| 聂盈 | ch708 | 蓝衣女子 | 全 14 章禁（与「银月」同理）|
| 掌天瓶 | ch2425 | 人间篇只叫小瓶 / 绿瓶 | 全 14 章禁 |

**本章解禁的**（首见落在 ch152-216 内，规则自动放行；列出来是给写手看的）：岳麓殿、定颜丹、血禁、血色试炼、真元、地火屋、南宫婉、李化元、青元剑诀，以及上表前 18 个。**仍禁**：陈巧倩（ch273）、大衍诀（ch230）、妖丹（ch344）与上表后 9 个。

### 12.2 规则放行、本章另有专项断言的

| 词 | 首见 | 为什么规则管不住 | 本章的断言 |
| --- | --- | --- | --- |
| 定颜丹、筑基丹方 | ch155 | 章内先后 | 节点 1-4a 的文案 key（`ch07.chaibao.*` … `ch07.yuelu.*`）不含「定颜丹」；**面板**：`ch07.cangshi` 置之前炼制面板列表里没有这两张方子（E1 用例）|
| 岳麓殿 | ch153 | 章内先后 | 节点 1、2 的 key 不含（节点 3 马师伯第一次说出口）|
| 血色试炼 | ch161（ch160「血禁试炼」）| 章内先后；两种写法 | 节点 1-7 的 key 不含「血色试炼」；全章「血禁」0 处（大纲命名裁决：事件名统一用血色试炼）；「血色禁地」只许出现在对白里、≤ 2 处 |
| 血色试炼（章节卡）| ch161 | 章节卡 `ui.chapter.07.title` 在 `ch06.done` 那一刻就上屏，早于节点 8 | **登记例外**（校对整改 16.5 M1）：第 6 章改卡名是因为「升仙令」会剧透那块牌子的身份；「血色试炼」是本章大事件的名字、不是悬念，又是大纲章名，不改。例外只放行这一个 key 的这一个词：`Ch07Acceptance.No6_TheChapterCardAndTheTwoGatesSeenInChapterSixSayNothingOfChapterSeven` 钉住卡片两个 key 的值（「第七章」「血色试炼」），其余章内禁词照判 |
| 巫钧山 | ch153 | 不在表一；通岳麓殿的北门第 6 章里就在，拦门话第 6 章与节点 1、2 都读得到 | 节点 1、2 的 key 与两句第 6 章可见的拦门话（`ch07.block.fangshi` / `ch07.block.yuelu_dian`）不含「巫钧山」（`No6_TheChapterCard…` 已把它列进名单断言）、不提担保（校对 MEDIUM-2）。是否补进 `LexiconTests` 表一（153）由协调者定（16.5 M2）|
| 陈巧倩 | ch273 | 表一已拦 | 本章人名一律「陈师妹」；role `chen_shimei` 的 `name` 不含「巧倩」|
| 南宫、南宫婉 | ch206 / ch209 | ch206 那句在暗场，规则放行整章 | 节点 1-30 的 key 与节点 31 问名之前的 key 不含「南宫」（**按 key 前缀断言：`ch07.jiaoshi.name*` 起才许**，其后 `ch07.xiashan.*`、`ch07.chukou.*` 照常）；`baiyi_shaonv` 的 `name` 不含「南宫」|
| 李化元 | ch212 | 章内先后 | 节点 1-32 的 key 不含「李化元」；role `li_huayuan` 的 `name` 是「李师祖」|
| 菡云芝 | ch203 | 表一放行本章 | 节点 1-28 战前的 key 不含（按 key 前缀：`ch07.xiaoshidian.after*` 起）；`maifu_shaonv` 的 `name` 不变 |
| 李师祖、师祖 | ch174 | 章内先后 | 节点 1-17 的 key 不含「师祖」|
| 符宝 | ch143 | 第 6 章已放行 | 允许；原著拿它比作法宝的那个说法不用（13 第 3 条）|
| 董家、红拂、马云龙、寒天涯、萧二 | ch244 / ch144 / ch199 / ch185 / ch185 | 前一个表一拦；后四个都在暗场 | 本章文案 0 处（暗场不搬上来）|
| 淫囊、合欢 | ch208 / ch165 | 不是专名 | **0 处**（暗场口径，16.1 第 3 条）|
| 中阶灵石 | ch131 | 第 6 章放行但不说 | 本章起说（节点 1 起）|

---

## 13. 文案要求

与前六章同一标准：对 ch152-216 最长公共片段 ≤ 6 字、对全本 ≤ 7 字、≥ 10 字 0 条；换词不算重写；key 命名 `ch07.<场景>.<用途>`；台词 18-60 字为主、不超 80，选项 5-20 字。

**转写高风险点**（本章讲解性长段多、原著的名场面多）：

1. ch155 许老讲丹方失传、ch156 讲地火：因果是事实（天材地宝有限 → 原料绝迹 → 方子没人重视），比方与语气全换。
2. ch160 吴风讲禁地：数字（五年、五天、三分之一、四分之一、中阶灵石一块、灵器一件）是事实；讲法全换，**不照原著的叙述顺序**。
3. ch164 符宝之秘：六条性质是事实；「伪法宝」这类说法不用。
4. ch177 李师祖讲正与邪：结论是事实（弱肉强食、七派中立的来历、尊敬只在同阶之间），**句式全换，不许出现原著那几句总结的骨架**。（初稿没做到，校对整改已按原意重做：18.16 第 2 条。）
5. ch194 中心区秘闻：三层结构是事实；「果皮果肉果核」的比方换掉。
6. ch212 谢师礼、ch214 地火屋陈设、ch216 服丹过程：数字与步骤是事实，描写全换。（初稿地火屋与服丹没做到，校对整改已补做：18.16 第 2 条。）
7. 名场面台词**不许复刻**：封岳问他想怎样时他那句带笑的狠话（ch187）、她质问为何不早出手时他那句大实话（ch206）、她要他把这事当梦一场的那句（ch208）——这三句是本章最容易被原样搬进来的。意思保留，说法自己写；第 3.2 节里为说明意图写的转述同样**不许照抄进文案**。
8. **暗场口径**（16.1 第 3 条）：陈师妹那一夜只写「被捆」「神情迷乱」「扑来」「被定住」；墨蛟之后只写「失去知觉」与醒后她的外貌和三句话；菡云芝那一掌只写「打晕」。任何形容身体、衣着剥落、亲吻的句子**一句也不写**；验收 21 由独立校对逐 key 过。
9. 点名可改的键（菜单、快进、存盘、路径行动）一律 `{key.<id>}`，并加进 `KeyPrompts` 白名单；点名物理键的提示同时写 `.pad` 版（`docs/gamepad.md` 第 6、7 节）。本章预计新增的按键提示：插刃那一段「按 {key.confirm} 插下一柄」一条（及 `.pad` 版）。

### 13.1 每段框架件清单（2026-09-29 补；内容路施工第 1 步；写手与复验共用的判据）

**判据**（照 `docs/ch05-design.md` 13.1；第 6 章初审 HIGH-1 的上游原因正是施工图没有这一节、3.2 逐节列的就是原著镜头序列）：

- **框架件**＝删了戏就塌的东西——剧情事实、1.1 的硬约束、谁在场、落点。下表逐段列全；它们与原著对得上是应当的。专名（人名、派名、地名、物名、法术名、药名）与数字不算质感。
- 表外的一切——样貌、年纪、衣着、手势、道具、次要人物的反应、次要的一句话、谁先开口、谁先谁后、比方——**一律自出**。原著该场的质感项不许出现，**换了词也不行**；把原著的一个动作反过来写（他没回头、她没哭）也算留着，一并删。
- **3.2 里凡是原著的镜头、动作与台词转述一律作废**（例：节点 2 马师伯「闻、掂」，节点 4a 红衣弟子「诧异马师兄也肯作保」，节点 6 陈师妹「挽着胳膊」、罚兄弟「面壁到九层」，节点 15「一拳砸在桌角」，节点 19b 陈师妹「站他右手边」、穹前辈「凭空伸手」，节点 23「一张大火球符烧掉树冠」「一掌穿胸」，节点 24 花园四样花木、钟吾「往布袋里扔肉块」，节点 26「抹黑泥」「火球挑衅」，节点 28 地上「红狼、白雕」、她那块「黄丝帕」，节点 32b 那个「鹬蚌相争」的说法与「赤脚大汉与疤脸」的故事，节点 33「像见了鬼」「孺子可教」，节点 35a「铃铛」，节点 36「烈火、刀绞、奇痒、灰色杂质」）。**3.2 只剩脚本调用（give / take / flag / battle / advance_days / teleport / realm）与分支结构有效**；文案以本表为准。
- **次序自定**：下表编号只为清点，不是讲述次序。讲解性长段（许老、吴风、符宝、正与邪、秘闻、服丹）按「韩立想知道什么 → 先问哪一句」重排，不按原著叙述的先后。
- **观察者**：这一章的韩立是算账的人（灵石、法力、株数、日子，处处在数）、药师（看成色、闻火候、估年份）、怕露底的人（先看退路、先看对方的手、先想这句话会不会被人记住）。质感只从这三只眼睛里出。
- **名场面三句**（13 第 7 条）与**暗场**（13 第 8 条）照原条；下表只列事实，不列说法。
- 复验：逐段做「原著质感项 → 现文」的并排表，剩下的对应项必须全在本表里；机器口径见本节末。

| 段 | 原著 | 框架件（删了就塌） | 在场 | 落点 |
| --- | --- | --- | --- | --- |
| 1 拆包 | ch152（承 ch151）| ①叶师叔那一包：中阶灵石两块（火、土）、精钢指环（驱动后飞出锁敌、可放大）、黑色三角小旗（挥出黑雾遮目）、黄铜瓶（保灵力不失）、高阶土牢符两张；②黄铜瓶试液：绿液在里面从一刻钟撑到一整天，过时照样没了；③残片仍盖在瓶上；④马师伯交给他打理的苗；园角另开两槽（不说破为什么）；⑤「中阶灵石」从这里起可说 | 韩立 | `ch07.chaibao` |
| 2 头一年 | ch152 | ①交三株四十四年以上的黄精；②月例每月两块（一年二十四）；③那一年谷里大开山门，新人里有一对雷灵根的慕容兄弟（节点 6 的伏笔）；④谷里没几个人还记得他（可省） | 韩立、马师伯 | `ch07.jiaoyao1` |
| 3 第二年 | ch152-153 | ①交两株百年紫参；②月例涨到每月五块；③两年连破两层到十一层；黄龙丹（本作养精丹）从此无效；④叶师叔的侄孙服了筑基丹没成；⑤问丹方：马师伯不肯给，叫他自己去岳麓殿（「岳麓殿」第一次出口）；⑥担保的代价是白看一年园子，换来担保玉符 | 韩立、马师伯 | `ch07.danbao`、`ch07.baigong_qiu` |
| 4a 传送阵 | ch153 | ①巫钧山；未筑基者须担保人信物；②两名筑基红衣弟子验玉符，担保人是马师兄——马师伯即马师兄在这里点破；③传送阵每趟一块，头一回免；④大厅三条通道：器、丹、无标记 | 韩立（红衣弟子只在旁白）| `ch07.yuelu` |
| 4b 许老与藏室 | ch154-155 | ①许老：筑基，不许人叫师伯（好明着要钱）；②藏室一个时辰一块、复制一份十块；③楼上多是世俗医书与几捆同档丹方（一句带过，不点书名）；④两枚玉筒：筑基丹方须先天真火（筑基后才有：死循环）；定颜丹方只驻颜、不要真火；⑤复制两份共二十块；⑥收了钱才说：大半丹方随天材地宝绝迹而失传，筑基丹是留下来的一种 | 韩立、许老 | `ch07.cangshi` |
| 5a 地火与银丝鼎 | ch156 | ①地肺之火可以替真火；玄阳火地在无标记通道尽头，交灵石借用；②他买最小的银丝鼎（储物袋只装得下它），三十二块；③在许老这里前后花了五十几块 | 韩立、许老 | `ch07.yinsi` |
| 5b 丑汉与石门 | ch156 | ①五彩禁光的石门，门边石屋里的丑汉（炼气顶层）；②问清借地火的规矩；③丑汉瞧不起他、背后讥他穷——他听见了，不发作 | 韩立、丑汉 | `ch07.chouhan` |
| 6 山头 | ch157-158 | ①慕容兄弟（雷灵根）当众演雷法；②陆师兄（风灵根，原本低阶弟子里唯一的异灵根）不服、夸口硬接；③陆的女伴陈师妹在场；④陆以护罩扛下，反手青弧斩追人；⑤一个少年往他身后躲，他闪开；⑥另一少年躲到粗矮青年身后，土墙挨了一刀；⑦蓝衣女子以火焰鸟收场，罚了兄弟、谢了粗矮青年，临走传音说他只顾自己——他不以为然；⑧陆师兄看清了他的脸（节点 13 的由头） | 韩立、慕容兄弟、陆师兄、陈师妹、蓝衣女子、粗矮青年、围观弟子 | `ch07.murong` |
| 7 读方与马师伯 | ch158-159 | ①辅药三十一味，园里都有、要数百年火候（他有瓶）——不点药名；②主药玉髓芝、天灵果、紫猴花，园里一株也没有；③定颜丹：药常见、要千年，往后再说（本章不炼）；④六七日后马师伯来：三味主药天生自长、没有种子、幼苗离了原地难活；不肯说出处，只说去了等于送死 | 韩立、马师伯 | `ch07.dufang` |
| 8 吴风：禁地；敛气术 | ch159-161 | ①吴风服过筑基丹仍在炼气顶峰，对谁都肯教（开口按 `ch06.wufeng` 分，1.4）；②禁地：风属性古禁、每五年五天衰弱期、数名结丹合力开路；只有炼气期进得去；各派在里头为药互相下手；③近百年活着出来的不足三分之一，报名重赏之后不足四分之一；④报名先赐中阶灵石一块、灵器一件；带出灵药按量按质给赏，最高可换筑基丹；⑤下一次在半年后；⑥「血色试炼」四个字第一次出现（弟子们起的名）；⑦几夜不眠：不去就百余年后化作白骨，去就是四分之三的死数——写他怕；⑧挑中敛气术（中阶辅助：只要不被人亲眼看见，气息就藏得住；对筑基无用），理由：交手时来不及念中阶法术，藏得住才活得长 | 韩立、吴风 | `ch07.lianqi` |
| 9 园角千年灵草 | ch161 | ①四个多月：白天练敛气术，夜里在园角催两株黄精芝，避开马师伯来的日子；②灵石留着保命不动，拿草去换；③只卖给外来修仙者，不与本门交易 | 韩立 | `ch07.qiannian` |
| 10 外出令牌 | ch161 | ①向于执事领外出令牌，每年一次，少有人申请 | 韩立、于执事 | `ch07.chuling` |
| 11a 坊市 | ch161-162 | ①坊市在太岳山脉东北缘，元武国修士常来，两边修仙界不敌对；②五里内禁飞；③换装遮脸（穿什么自出）；④一条南北街：南段店铺是黄枫谷的产业，北段临时摊位（一块灵石摆一天，有人护着）——店名不列 | 韩立、坊市路人 | `ch07.fangshi` |
| 11b 万宝楼 | ch162-165 | ①挑最气派的一家；掌柜田卜离（凡人）；化名「厉飞雨」；②楼里有人暗中护着（写法自出）；③四件：金蚨子母刃（母一子八）、玄铁飞天盾（放大后自动绕身防御）、天雷子（筑基硬抗也化灰）、金光砖符宝；④鉴定人丁老断为新出土的千年黄精芝；⑤符宝五条：结丹以上把法宝至多十分之一的威能封进符纸、谁都能用、筑基前只发挥一两成、用一次耗一次、耗尽作废；⑥两株换四件；⑦田掌柜求以后优先卖他——他因此定下这种买卖少做 | 韩立、田卜离、丁老 | `ch07.wanbaolou` |
| 12 绕路 | ch165 | ①出坊市就走；怕被跟，绕了三四天；②傍晚歇在太岳山脉外围半山腰的一个石洞 | 韩立 | `ch07.likai` |
| 13 夜遇 | ch165-170 | ①下半夜陆师兄带着被制住的陈师妹落在洞外；②陆要杀她：有人许他攀高枝、条件是与她断绝；她是陈家家主独生女，留着会报复；她身上藏着一粒筑基丹（红木盒）——董家、红拂不点名；③她被法术捆着、说不出话，药力让她神志渐乱（暗场口径）；④他敛气旁观；陆早察觉了他，偷袭被水罩符挡住；陆认出他是山头见过的那个；⑤二选一；⑥**战斗①**：陆十二层、风系、青蛟旗化蛟；剑符化巨剑相持、比法力——中阶灵石、生嚼年份草药、撤掉水罩省法力（符激发后仍连着一丝灵线耗法力，吴风说过）；⑦陆临死一击，他避开，陆被斩成两半；⑧剑符回来时自燃成灰；精钢环碎了；⑨搜得两粒筑基丹、青蛟旗、青色绳索、银钩、一叠符、二十多块灵石；⑩生嚼草药的杂质淤在丹田；⑪她药力发作扑来，他以定神符（或定神术）把她定住 | 韩立、陆师兄、陈师妹 | `ch07.yeyu` |
| 14 返回 | ch170-171 | ①她事后记不清他的脸；②他想过灭口，没下手；马师伯说过元阳之体筑基机会大，他不碰她（不写欲念）；③焚尸、抹掉打斗痕迹；④带她往西百余里放下，喂清灵散解药毒；⑤回园闭关三日驱尽杂质；伤了元气，一个来月才复原；⑥两粒筑基丹装进黄铜瓶，原来的盒子瓶子毁掉；⑦陈师妹那一粒不还 | 韩立（陈师妹昏沉）| `ch07.fanhui` |
| 15 服丹之法；封禁 | ch171-172 | ①借学法术的由头，从吴风那里套出服法：直接吞，不要药引；可服了须闭关三个月化药；②与试炼不可兼得；③近半月的思量；④告示：禁地五年之后起封闭六十年、七派共派人看守；三四百年一次的圈封（开得太勤灵气流失）；⑤他的判断：五年后那一届是封山前最后一回，七派精锐会尽出，那才是死地；这一届是他唯一的机会；六十年后又过了筑基的好年纪 | 韩立、吴风 | `ch07.fudan_fa` |
| 16 报名 | ch172-173 | ①王师叔收报名；新人来送死、九层到十一层太快——拉到一边再测：仍是伪灵根；②二选一：误食异果（龙鳞果，王师叔信了一半）/ 不解释；③王师叔交代注意事项 | 韩立、王师叔 | `ch07.baoming` |
| 17 两瓶丹药 | ch173 | ①他来卸下药园差事；②马师伯看他像看一个要去送死的人；③临走丢下两瓶：一瓶内服、一瓶外敷 | 韩立、马师伯 | `ch07.ma_songyao` |
| 18 议事大殿 | ch173-174 | ①出发前三天信符召集；②陈师妹也在，变得冷，认不出他；陆师兄按失踪了事，没人再提；③本届满额二十五人：十三层顶峰五六个、多为十二层、十一层只三人（他、一个白发老者、一个少年）；④中阶灵石任取一块（他取水属性）；从一只隔绝神识的袋子里摸一件灵器（不说是什么）；⑤李师祖（结丹）带队，王师叔等五名管事同行；⑥银甲角蟒，两天两夜飞到建州北的荒山 | 韩立、李师祖、陈师妹、王师叔（台词）、同门 | `ch07.jihe` |
| 19a 向之礼 | ch174-175 | ①向之礼带着一个少年来找他结伙（法力低的抱团）；②他拒绝（或敷衍） | 韩立、向之礼、少年 | `ch07.xiang` |
| 19b 七派到齐 | ch175-179 | ①次日上午等了数个时辰；②清虚门乘雪虹绫到，领队浮云子（结丹）；③李、浮云二人打赌：先比灵药多寡、再比质量、最后比活着出来的人数（血线蛟内丹对两块铁精）；④穹前辈插进来：黄枫、清虚两家合计胜过掩月宗就算他输，押三枚无形针符宝；⑤李师祖对弟子讲正与邪——结论：修仙界弱肉强食；七派原是两头倒的小派，正邪两败之后联手拔掉双方，所以中立；尊敬只在实力相近者之间；许诺这一趟功劳最大的人筑基后收作弟子（他不打这个主意：秘密太多）；⑥掩月宗乘天月神舟到，霓裳仙子领队，一对对年轻男女、没有老人；⑦巨剑门、灵兽山、化刀坞、天阙堡到齐；七派每派至多二十五人；⑧卖符少女在灵兽山队里，被络腮胡子呵斥，他冲胡子做了个鬼脸（节点 22 的由头） | 韩立、李师祖、浮云子、霓裳仙子、穹前辈（台词）、各派、陈师妹、向之礼 | `ch07.qipai` |
| 20 破禁 | ch179-180 | ①飞了几个时辰，到一片寸草不生的黄土坡；②巨剑门的结丹掷剑试禁，风刃之墙现形；约一个时辰一次，第四次明显弱了；③七位结丹各放法宝，三四个时辰打出丈许通道；④弟子穿插进去；一进去同门也不可信；⑤出口的挪移阵把人随机送走 | 韩立、各派 | `ch07.pojin` |
| 21a 乌龙潭 | ch180-181 | ①落点在禁地外围（地貌自出）；②资料：挪移阵随机送人；乌龙潭在东北角、潭边长寒烟草，潭里的寒冰蟾温顺不伤人；③一个天阙堡弟子去采寒烟草，被灵兽山二人借寒冰蟾偷袭杀死；二人搜走东西、毁尸离去，不肯守在潭边；④他敛气不动，以一敌二不去想；寒烟草一株也拿不到 | 韩立（三人只在旁白）| `ch07.wulongtan` |
| 21b 山崖两尸 | ch181-182 | ①山崖下，巨剑门弟子与黄枫谷弟子同归于尽；②两人的储物袋都不见了——还有第四个人；③他试探一下、没有动静，走人（茅草地里的人是暗场）；④捡走那团透明丝线（十余丈、几乎看不见、锋利） | 韩立 | `ch07.sixian` |
| 22 一线天 | ch182-184 | ①两侧峭壁一条小路；禁地里御器飞行等于当靶子；②络腮胡子（十三层）与天阙堡严姓（十二层顶峰）前后堵住——两人借融灵符落在一处、专杀人夺宝；③胡子记恨禁地外那个鬼脸；④**战斗②**；⑤毁尸，带走五六个储物袋 | 韩立、络腮胡子、严姓 | `ch07.yixiantian` |
| 23 林中一夜；多宝女；封岳 | ch185-190 | ①他在树上打坐回法力；禁地的夜亮如白昼、天灰蒙蒙；②头一天的清洗：弱的多半死在头一天（讲法自出）；③第二日早，同门黄衫师姐逃到树下求救（牵引之术让同门互相感应，十日有效）；追来的是掩月宗的白衣女子（十二层）；④二选一；⑤多宝女（掩月宗长老的后人，法器多）：小镜子定住他的金刃，粉红水晶球锈坏了银钩；他用青索捆住她；⑥一道黄芒穿透青索、护罩与她本人：天阙堡的狂人封岳（十三层顶峰，小刀符宝）；⑦封岳拿走她的储物袋，想收镜子与水晶球——他一枚火球打断；⑧黄衫师姐想逃，被封岳杀了；⑨封岳问他想怎么个死法——他回了一句要他命的话（**说法自拟**）；⑩**战斗③**；⑪储物袋被天雷子一起化灰；捡回小镜子（青凝镜）、水晶球、小刀符宝；靴子是踏云靴；⑫草草埋了师姐；⑬第一天过后禁地里只剩七十多人 | 韩立、黄衫师姐、多宝女、封岳 | `ch07.fengyue` |
| 24 铜门；钟吾；秘闻；树洞 | ch191-194 | ①中心区：石墙、四扇青铜门之一，门开着；墙上钉着三具尸——**动手的人不点名**（12.2）；②门内第一层是花园（花木自出）；③灵兽山钟吾放飞蛇偷袭，认出踏云靴；④二选一；钟吾退开、改口称兄弟（不为封岳报仇）；⑤交换玉简资料（各派明令不许）；⑥秘闻：中心区占禁地三分之一多，由外向里三层——花园、终年浓雾的环形山（妖兽多，灵药长在洞谷石殿里）、百丈巨塔（谁也进不去）；月阳宝珠驱雾，七派轮掌，本次约定第三日早上；（原著「果皮果肉果核」的比方不用）；⑦他在一个树洞里睡到第三日凌晨 | 韩立、钟吾 | `ch07.zhongwu`、`ch07.yujian_qiu` |
| 25 月阳宝珠 | ch195-196 | ①上山只走资料里几条安全的小路；他在一处路口等；②一道白光冲天、化作光雨落进雾里，雾散了；本次执掌的是天阙堡；③他第一次见识法宝的威力（写羡慕）；④环形山现形，有人从各处进山；他最后一个进山 | 韩立 | `ch07.yueyang` |
| 26 紫猴花洞 | ch196-198 | ①他的算计：别人抢熟的，他专找刚被采过、只剩幼苗的地方；幼苗离了地还能活一两年，瓶子催几轮就够；主药四五百年即可入药（不提嗅灵兽）；②资料里一处出紫猴花的洞，百余年前被采过；③洞里一只巨蜈蚣守着几株紫猴花幼苗；④硬拼不划算：退回洞道，把八柄子刃刃口朝上埋在地里；⑤把它引出来，它追进洞道，腹部一路被剖开；⑥采苗；割几块背壳；毒气入鼻——服清灵散 | 韩立 | `ch07.kuitan` → `ch07.mairen` → `ch07.zihouhua` |
| 27 四处灵药 | ch200 | ①此后几处没有守护兽，一处接一处；②第三日将尽，最后一站：近山顶的小石殿（天灵果在里头） | 韩立 | `ch07.sichu` |
| 28 小石殿 | ch200-203 | ①巨剑门的赤脚大汉（银色巨剑）逼卖符少女让出烈阳花，她护身的法器快撑不住（她带进来的灵兽已经死了）；②法器被击碎的一刻，他的金刃救下她——这一回他没守自己的规矩；③大汉是武痴，只认他的顶级法器：打赢了，人和药都归他；④**战斗④**；⑤战后大汉认出青凝镜：掩月双娇的护身法器，她们的祖母是掩月宗结丹长老——传出去必遭追杀；⑥少女提议大家都守口如瓶；两个男人都明白了；大汉拼命——十丈之内、没放护罩，丝线取了他的首级；⑦少女问他是不是也要灭她的口；⑧他烧尸、收银剑与金刃；互报姓名：她叫菡云芝；她只要烈阳花；⑨二选一；她去采花时一掌打晕（只写打晕）；扫空石殿灵药，烈阳花留给她；抱到山顶石洞，无忧针法＋忘尘丸，她只会忘掉半日；⑩他躲在树上看她捧着花茫然下山 | 韩立、赤脚大汉、卖符少女 / 菡云芝 | `ch07.wuyou` |
| 29 青石殿与地道 | ch203-204 | ①第四日不顺：守护兽多、资料有误，法力体力都吃紧；药量勉强够；②钟吾的玉简上近处还有一处：小盆地里的青石殿；③殿门弹开他的金刃；钟吾自己为什么不来——他起疑；④一大批人逼近，他闪身进殿；⑤殿中央的地道口往外吹潮热的风，他钻了下去 | 韩立 | `ch07.didao` |
| 30 观战；墨蛟 | ch204-207 | ①地下百余丈：一大片沼泽；边上长着数十株灵药（他要的三味都在）；中央亭子里悬着一口金箱；②他藏起来（敛气＋匿形）；③掩月宗一群人到，领头的是个白衣少女，弟子叫她「师祖」——结丹才当得起的称呼，她的法力却只有炼气顶峰；④一对对男女合击（阴阳牵引术）；她祭出法宝朱雀环；⑤沼泽里出来的是墨蛟（她认出来）；套住、焚烧、打出血洞；⑥墨蛟蜕皮：雪白、生角生爪，进了二阶（可比筑基中阶）；紫液化掉几名弟子（只写地上多了坑）；⑦她叫众人撤、自己拖住；要走时通道被一道禁法封死（小五行须弥禁法；外面的事是暗场）；⑧她靠符箓撑了几个时辰，法力将尽；⑨二选一；他出手的算计：通道没了，只有她认得这道禁法；⑩她问他为何不早出手——他答怕她事后杀人夺宝（**说法自拟**）；她叫他小家伙；⑪**战斗⑤** | 韩立、白衣少女、掩月宗弟子 | `ch07.mojiao` |
| 31 醒来；金箱；破土；南宫婉 | ch207-209 | ①他用赤脚大汉的银剑剖蛟尸；她说这剑里掺了银精、外人驱使不了；②他从蛟腹里掏出一团东西，她一碰炸开粉红烟雾——**暗场**：两个人都失去了知觉；③醒来她的外貌长大了好几岁；她冷冷交代就当没发生过、传出去就杀他（**说法自拟**），他应下；④她修素女轮回功，损了几年法力（只说损）；⑤金箱归她；灵药她不要，他全收了；⑥破禁要两人合力：她把被禁的法力暂传给他（他一时有了十三层的法力，面板不动），朱雀环与金光砖轮番打穿地面；⑦金光砖耗尽成了废纸；她损了二三十年功力；⑧分别时他问她的名字：南宫婉；⑨他清楚两人不会再有交集 | 韩立、白衣少女 / 南宫婉 | `ch07.nangong` |
| 32a 下山 | ch209 | ①他在林梢飞奔下山；一个灵兽山弟子伏击，他一闪就取了那人首级；②第五日下午赶到出口 | 韩立 | `ch07.xiashan` |
| 32b 出禁地；赌局；记名弟子 | ch209-212 | ①七位结丹、管事与穹前辈等在出口；②一派一派出来，人数报出来：黄枫谷活着五人，别派多是三四人，巨剑门只剩两人；③他坐到陈氏兄妹旁边——他们以为他躲了一路；④掩月宗迟迟不出，他替她担心；菡云芝在灵兽山那边，平安；⑤最后一刻钟掩月宗十余人整队出来，为首的是南宫婉（霓裳仙子迎上去；别派认不出这位常年遮面的仙子）；⑥向之礼最后一个爬出来，通道随即碎灭；⑦嗅灵兽（三丈内闻得出百年以上的灵药，储物袋也藏不住）；他交出二十几株；幼苗它闻不出；⑧掩月宗赢了赌局；浮云子交出内丹，李师祖应承送穹前辈铁精；⑨南宫婉上神舟，自始至终没看他一眼；菡云芝离开时对他一笑；⑩李师祖问他名字、入谷几年（近三年）、药从哪来——他编了一段自己捡了漏的来历（故事自出）；⑪收记名弟子（二选一），赐碧光刀；筑基之后收为正式弟子；⑫骑蟒数日回谷 | 韩立、李师祖、南宫婉、菡云芝、钟吾、陈氏兄妹、向之礼、浮云子、霓裳仙子、穹前辈、各派 | `ch07.chujindi`、`ch07.baishi` |
| 33 谢师礼 | ch212 | ①马师伯没想到他活着回来、还成了李师叔的记名弟子；②马要他改口叫师兄，他坚持没筑基还叫师伯；③马点破谢师礼：师傅可抽徒弟上交的一半（只一次）；约十株换一粒，他本该两粒、如今一粒；④「李化元」首次出口（马说的）；李师叔护短；⑤他反倒放了心：图的是药，一粒筑基丹对要自己开炉的他不算什么 | 韩立、马师伯 | `ch07.xieshili` |
| 34 三年 | ch213 | ①奖赏只一粒筑基丹，说辞是弟子应尽的孝敬（王师叔送来）；②清点禁地所得：中阶十余、低阶数百、傀儡弓手（要「分神」秘术、筑基以上才修得）、银色书页（参不透）；③三年：催主药、备辅药；④陈师妹服了奖赏的丹、一年后筑基；她大哥第二枚仍败、回了家族；⑤他在谷里有了点名气；⑥叶师叔把拖欠的一股脑送来、还多出；⑦李师祖派人送来亲笔誊写、署名的《青元剑诀》，此外三年无音讯 | 韩立、王师叔 | `ch07.sannian` |
| 35a 石门 | ch213-214 | ①再进岳麓殿（传送阵一块）；②丑汉在睡，他把人弄醒（怎么弄醒自出）；③报出李师祖门下，丑汉不信——亮出署名的剑诀，丑汉认得笔迹，立刻换了脸色；④要一间火力稳的；一块中阶灵石作定金（公认一百低阶兑一块，却没人肯换）；⑤令箭开石门 | 韩立、丑汉 | `ch07.dihuo` |
| 35b 十九号 | ch214-215 | ①圆厅、三十六间地火间；给他十九号；②屋里的陈设能控火（写法自出）；门一关，几名结丹合力才进得来；③他悬鼎、试火；④二十几炉连凝丹都过不去；几乎要放弃、想回去正经拜师学炼丹；⑤临走前又不甘心开一炉——地火交到玩家手上 | 韩立 | `ch07.feidan` |
| 35c 半年 | ch215 | ①亲手成了第一颗；②此后约三次成一次、取丹过半；③辟谷丹一粒撑一月；④丑汉那边从高兴到不安；⑤半年得二十几颗 | 韩立 | `ch07.chengdan` |
| 36 服丹筑基 | ch215-217 | ①就地服丹；②第一粒洗髓、排出一身杂质 → 十二层；③十几日后第二粒 → 十三层；④第三到第七粒：洗髓渐弱、真元由气化液、周身灼热；⑤第八粒药力爆发、昏死，醒来已是筑基；⑥残余药力要功法吸纳，手上只有《青元剑诀》（九层；前三层掌发剑芒、中三层护体剑盾；后三层不说）；⑦在屋里共十一个月；⑧出来时丑汉改口称师叔；他没为难丑汉 | 韩立、丑汉 | `ch07.done` |

**不在表里的段**（支线 Z1、Z2，药篓「这一季的药已经交了」，坊市、禁地外、出口的路人与打探，路径行动）原著没有对应场面：全部自出，**也不许借用原著别处的现成段落**（第 5 章 13 第 9 条的口径）。

**机器口径**（写进来，免得各方各量各的；第 6 章校对 HIGH-2 的建议）：本章全部玩家可见文案（`data/text/ch07*.json` ＋ 共享文件里本章的 key）逐条剥成只剩汉字，对原著 ch152-216 与全本各求**精确**最长公共连续片段；量具先自证——从原著取 12 个汉字嵌进一句自编的话必须报 12、一句原著不可能有的话必须报 ≤ 3，自证不过不出结论。判线：对 ch152-216 ≤ 6、对全本 ≤ 7、≥ 10 字 0 条。

---

## 14. 时长估算

| 桶 | 项 | 估 |
| --- | --- | --- |
| A | 主线对白约 17300 字 ÷ 500 字/分（校对整改后实测 **17,501 字**：46 个脚本调用的全部文案、含各分支与选项，与校对 §六同一口径；整改前 19,143）| 35 min |
| A | 必打 5 场 × 4.5 min | 22.5 min |
| A | 9 张新图首次探索 × 0.75 ＋ 两处修补 | 7 min |
| B | 药园三时节（种、坐、浇、交）| 7 min（校对按「约 40 滴、14 轮坐—走—浇」估 7–10 分；目标行 n02 补了「瓶满了就不再凝」，LOW-14）|
| B | 万宝楼以物易物、设伏插刃、嗅灵兽上交、地火亲手开炉 | 3 min |
| **A + B** | | **≈ 75 min**（目标 70；超 5 分钟，与第 5、6 章同一处理：先记着，`tools/audit.py` 落地后重算）|
| C | 坊市买卖、收药摊、支线 2、环形山遭遇、多种千年药、定颜丹、路径行动 | 12-18 min |

超出的 4 分钟几乎全在对白：原著 65 章是第 6 章的两倍半，本稿每段讲解已压到 8 句以内；再压就只能删名场面，不划算。

---

## 15. 验收

每一条都配一条**不经过驱动、直接读数据、判据写死成本文原文**的断言；每一条新检查都要配「故意写坏 → 确实被抓住」的用例。

| # | 验收条目 | 直接读的断言 |
| --- | --- | --- |
| 1 | 四道门禁全过（validate / selftest / mapgen / artgen），零新增警告 | 门禁本身 |
| 2 | **一条自动化通关测试**，起点 `ch06-end-*.sav` 两侧，落在 `ch07.done`；终点写 `ch07-end-*.sav` | `SetUp` 先验 fixture：`ch06.done == 1`、`realm == realmCap == 9`、`talisman_jianfu == 1`、`field_baiyaoyuan` 存在；终点先验：`realm == realmCap == 21`、`maxHp ≥ 260`、`maxMp ≥ 180`、`bottle.capacity == 6`、`learnedMagics` 恰是 8 门（火弹、御风决、天眼、流沙、冰冻、祭金蚨子母刃、祭青蛟旗、祭青凝镜）、`pill_zhuji_dan == 17`、`talisman_jianfu == 0`、`talisman_jinguangzhuan == 0`、`weapon_wuming_sixian == 1`、`story_tayun_xue == 1`、`story_qingyuan_jianjue == 1`、`weapon_biguang_dao == 1`、`fields` 含 `field_baiyaoyuan`（6）与 `field_baiyaoyuan_jiao`（2）、三味禁地药 `count_aged(…, 100) == 0` |
| 3 | 挂点 `mode` / `once` / guard / set_flag 与 3.1 一致；无 `auto`；无同格两个对象；两处 `once=false` 无 `set_flag` | `Ch07TriggerModeTests` 读九张新图与两张修补图逐条对表 |
| 4 | 五张必打编成与 8.2 一致：① 陆师兄 `maxHp == 180`、realm 十二层、法术表里有伤人法术；⑤ `killable_by == ["土"]`、友军恰一人；五张 `can_escape false`、`defeat_is_fatal true`、`spirit_stones == 0`、`drops` 空；**③ 在背包没有天雷子时那只手也赢得了**（平衡路实测） | `Ch07BattleDataTests` ＋ `BattleHand` |
| 5 | **境界**：`realm.advance` 恰 4 处（`yaolou.lua` 一处目标 11；`zhuji.lua` 三处目标 12、13、21，次序也钉）；`realm.cap` 0 处 | 扫 `scripts/ch07/` |
| 6 | **禁词**：表一含 12.1 的 27 个词；12.2 的章内先后按 key 前缀逐条断言；分母先验 > 300 条 | `LexiconTests` ＋ 专项 |
| 7 | **借来的威能用完就收**：`magic.forget("magic_ji_jianfu")` 与 `take("talisman_jianfu")` 恰各 1 处、同在 `yeyu.lua`；`magic.forget("magic_ji_jinguangzhuan")` 与 `take("talisman_jinguangzhuan")` 恰各 1 处、同在 `jiaoshi.lua`；`magic.learn` 恰 4 处（金蚨、金光砖在 `wanbaolou.lua`，青蛟在 `yeyu.lua`，青凝在 `shulin.lua`，另无）| 扫脚本 |
| 8 | **配方**：`recipe_zhuji_dan` 与 `recipe_dingyan_dan` 的 `requireFlag == "ch07.cangshi"`、输入与难度同第 7 节表；结丹灵药方与四张制符方 `requireFlag == "story.recipe_later"`；从 `ch06-end` 起点读档，炼丹面板里没有这三张、制符面板里没有那四张，置 `ch07.cangshi` 后炼丹面板多出两张 | 读数据 ＋ `AlchemyScene` 用例 |
| 9 | **道具施法**：`talisman_tianleizi` / `talisman_tulao_fu` / `talisman_huoqiu_fu` / `talisman_dingshen_fu` 的 `castMagic` 与 E5 一致；在真战斗里用一件天雷子 → 背包少一件、法力不变；土牢符把架势 3 的敌人打到破势 | 读数据 ＋ `Ch07EngineTests` |
| 10 | **经济账**（第 9 节六条）| `Ch07LedgerTests` |
| 11 | **两侧同账**：节点 3 之后两侧灵石差与起点差相同（不因本章分岔扩大）；章末筑基丹都是 17；嗅灵兽之后两侧三药百年以上都是 0 | 通关测试断言 |
| 12 | **药园**：`field.unlock("field_baiyaoyuan_jiao", 2)` 恰 1 处（`chaibao.lua`）；药篓的 `take_aged` 年份字面量是 44、100、176；园角是 1000；万宝楼是 1000 | 扫脚本 |
| 13 | **嗅灵兽留幼苗**：`chukou.lua` 对三味药用 `count_aged(…, 100)` ＋ `take_aged(…, n, 100)`，不用 `take(id, item.count(id))` | 扫脚本 ＋ 切片测试（幼苗数不变）|
| 14 | **地火**：`facility_dihuo` 的 `grade == 4`、`require_flag == ch07.feidan`；意图存档下筑基丹方成算恰 33%；药粉用尽而一颗未成时 35c 补药粉、不置旗标 | 读地图 ＋ `successChance` 字面量 ＋ 切片测试 |
| 15 | **必打 5 场绕不过去**：五个编成 id 各只被一个脚本调用、挂点在 3.1 表、`set_flag` 在目标链 `done_flag` 序列里且是下一步的 guard；通关两侧各记实际打到的编成，都含这 5 个 | 读数据 ＋ 走一遍 |
| 16 | **支线 2 条**谓词与第 6 节字面量相符 | 读 `data/quests/q07_*.json` |
| 17 | **紧迫感的那一句**：`ch07.fudan.*` 里有一条同时含「五年」「六十年」「最后」三个词（交代封山前最后一回是下一届）；全章文案里「封闭前最后一次」这一错误说法 0 处 | 读文案 |
| 18 | **南宫婉收住**：`chukou.lua` 里南宫婉登舟那一段的旁白 key 含「没」「看」（没看他一眼）；本章之后任何地图上 `nangong_wan` / `baiyi_shaonv` 的 NPC 都以 `ch07.chujindi` 或更早的旗标撤场 | 读文案 ＋ `NpcPresenceTests` |
| 19 | NPC 在场链：禁地外同 role 两段一律「前 hidden == 后 visible」；山头七人一进一撤 | 规则 30 ＋ `NpcPresenceTests` |
| 20 | **瓶子与年份接口**（E3、E4）| `Ch07EngineTests` |
| 21 | 独立校对：1.1 四十条硬约束逐条核验；**暗场口径逐 key 核**（13 第 8 条）；原创性人工复核（13 第 7 条那三句）| 校对报告 |
| 22 | 主线节点 36、分支 ≥ 10、新地图 9、主线对白约 17300 字 | `tools/audit.py` |

---

## 16. 拍板记录与开放问题

### 16.1 已拍板（按推荐，用户 2026-09-29 预授权——下表 22 条全部按推荐落地）

| # | 拍板 | 本稿怎么落 |
| --- | --- | --- |
| 1 | 回写大纲与 lore（1.2 前四行与 lore 四行）| 协调者回写 `docs/大纲.md` 第 7 章条目、`docs/lore/人间篇-校对.md`（钟吾、时间线第 20 条、素女轮回功）、`docs/lore/数字设定.md` 矛盾清单第 1 条 |
| 2 | 必打 5 场，全部原著；巨蜈蚣做设伏、推山兽做遭遇、灵兽山伏击者做脚本 | 8.0–8.3 |
| 3 | **成人情节一律暗场**：陈师妹那一夜、墨蛟之后、菡云芝那一吻都不演；只保留剧情必需的事实 | 3.2 节点 13、14、28、31；13 第 8 条；验收 21 |
| 4 | 百药园两年做成三个时节，推进条件是马师伯按年份收药；不做农场 | 第 7 节 |
| 5 | 千年灵草用新药材黄精芝（五阶、`tradeable=false`）| 第 7、9 节 |
| 6 | 筑基丹方、定颜丹方按原著重写；筑基丹以「一炉份药粉」为料、难度 87（约三成）| 第 7 节配方表 |
| 7 | 地火屋先演废丹、再让玩家亲手开炉、半年蒙太奇把自炼数补足到 22 | 3.2 节点 35 |
| 8 | 筑基走脚本：12 → 13 → 21，扣 8 颗，章末剩 17 | 3.2 节点 36；验收 2、5 |
| 9 | 茅草地里的白衣少女、清虚门对化刀坞、萧二、寒天涯、赵师妹贴符、掩月宗弟子对她的那声称呼，**全部暗场** | `docs/ch06-design.md` 第 17 节第 16 条（沿用） |
| 10 | 南宫婉在名字出口前用 `baiyi_shaonv`「白衣少女」；卖符少女在自报姓名前沿用 `maifu_shaonv` | 第 5 节；12.2 |
| 11 | 剑符 ch170 报废当场收回；金光砖 ch209 耗尽当场收回 | 验收 7 |
| 12 | E2 道具施法本章做（维持第 6 章拍板），带 `stagger` | 10.1 E2；16.2 第 1 条 |
| 13 | 敛气术只作旗标，不进法术表 | 3.2 节点 8 |
| 14 | 七星草种子：原著全书无下文，本章不用、不收回 | 1.4 |
| 15 | 陈师妹的筑基丹不还（原著）| 3.2 节点 14 |
| 16 | 瓶子容量随境界本章补（E3）| 10.1 |
| 17 | 中阶灵石做独立物品，战斗里回法力、不当钱 | 第 7 节 |
| 18 | 踏云靴、飞天盾、黑雾小旗只作物品与旁白（不做装备）| 第 7 节；O2 |
| 19 | 墨蛟数据写炼气十三层，叙事上说可比筑基中阶| 1.3；8.2 ⑤ |
| 20 | 马师伯本章不摆 NPC，全靠挂点 | 3.1；第 4 节 |
| 21 | 配方门闸顺带堵第 4–6 章的名字泄漏（`story.recipe_later`）| 第 7 节；10.1 E1；16.2 第 2 条 |
| 22 | 陆师兄改十二层、木，maxHp 钉 180 作曲线锚点 | 8.2 ①；10.1 E7 |

### 16.2 与旧拍板的冲突、开放问题（拿不准、需协调者核的）

1. **E2 与旧拍板的理由对不上（旧板不动）**：第 6 章 16.1 第 9 条拍的是「E2 本章不做」、落点写第 7 章；可它的推荐理由是「ch242 他真正学制符」——**ch242 在第 8 章区间**。本稿**照旧板**在本章做 E2，理由换成本章的原著解法（土牢符困人 ch183、天雷子炸封岳 ch189）；若协调者认为旧板的本意是「跟着 ch242 走」，E2 可整块挪到第 8 章，代价是 ②③ 的原著解法要改成借用法术（第 5 章剑符的做法）。
2. **配方门闸会改第 4–6 章的面板**：结丹灵药方与四张制符方加 `story.recipe_later` 后，第 4 章丹房、第 6 章制符桌上就不再列它们。`tests/Ch04AlchemyTests.cpp` 有两条用例正是拿结丹灵药方与护身符方验「火候不够」「引保得住」——契约让引擎路在那两条里先置 `story.recipe_later` 再测（判据不变）。**第 6 章正在施工，制符桌的用例若数了行数，会跟着变**：请协调者在第 6 章集成时一并看。
3. **起点读的是第 6 章测试路 07:05 版 fixture**（第 2 节）：第 6 章集成时它可能重写；核出差异改第 2 节与第 9 节第 1 条，不改规则。
4. **LexiconTests 补词与第 6 章并行**：12.1 的扫描含第 6 章正在写的 7 个文件（2026-09-29 的快照）；第 6 章写手若之后用了「地火」「万宝楼」之类（本不该用），补词会让第 6 章转红——那是第 6 章的错，但请协调者把补词放在第 6 章集成之后。
5. **禁地里的伤一路带着走**：气血、法力跨战斗保留，`advance_days` 每天只回一成（`Application::restFor`），存档点不回血（`openFacility` 的 save 分支只存盘）、存档只有一格。本稿在禁地放了三处打坐点（3.1、第 4 节）防死档——代价是玩家多坐的日子会让实际天数多于剧情的五日（横幅照剧情写，不读 `today()`）。协调者若宁要紧张感，替代做法是每场必打的挂点脚本开头多拨一夜（`advance_days(1)`），但「五日」就得跟着改。
6. **坊市、岳麓殿两道门的位置**（第 4 节）按规则 29 分摆在黄枫谷南、北两缘；关卡路若因第 6 章地图的实际布局要挪，改第 4 节那一句即可。
7. **定颜丹**：方子本章可见；玩家若自己多养了两株千年黄精芝，在地火屋能炼出来——原著 ch243 才炼。本稿不拦（不影响任何一条账），第 8 章设计若要原著那一炉七八颗，从玩家背包实际数起。
8. **太南谷的卖符少女一直站在那里**：`maps/ch06_tainan_gu.tmj` 的 `npc_maifu_shaonv` 没有撤场旗标，而本章起她已入灵兽山（ch179），本章也就不能在别处再摆她（`NpcPresenceTests`）。本稿不动第 6 章的对象（第 17 节第 25 条）；建议第 6 章集成时给她补 `hidden_flag`（例如 `ch06.done`：太南会散了），补了之后本章荒山那一段可以改摆 NPC。

### 16.3 路径行动条目（施工用；`ch07.path.*` 旗标**等 `data/pathactions/ch07.json` 落地后由协调者登记**）

| id | kind | 图 / NPC | when | 效果 |
| --- | --- | --- | --- | --- |
| `wufeng_dating` | 打探 | 黄枫谷 / 吴风 | `ch07.chaibao` | 中阶法术难在哪（「念咒的工夫，对手早到了跟前」）|
| `yu_dating` | 打探 | 黄枫谷 / 于执事 | `ch07.chaibao` | 外出令牌一年一次、没人申请 |
| `ye_dating` | 打探 | 黄枫谷 / 叶师叔 | `ch07.jiaoyao1` | 侄孙闭关的消息（不说结果）|
| `wang_dating` | 打探 | 黄枫谷 / 王师叔 | `ch07.baoming` | 禁地资料是历届生还者攒的 |
| `xulao_dating` | 打探 | 岳麓殿 / 许老 | `ch07.cangshi` | 丹炉扛得住地火的才叫丹炉 |
| `chouhan_dating` | 打探 | 岳麓殿 / 丑汉 | `ch07.chouhan` | 地火间按房算、按日收（`ch07.dihuo` 之后换一句恭敬的）|
| `tanzhu_dating` | 打探 | 坊市 / 收药摊主 | `ch07.fangshi` | 百年以上的药有价无市 |
| `yuanwu_dating` | 打探 | 坊市 / 元武国修士 | `ch07.fangshi` | 元武国与越国修仙界不敌对 |
| `zhishi_qiugou` | 求购 | 坊市 / 执事弟子 | `ch07.fangshi` | 火球符 2 张，12 块灵石 |
| `xiang_dating` | 打探 | 禁地外 / 向之礼 | `ch07.xiang` | 他又去找了别人结伙 |
| `chen_dating` | 打探 | 禁地外 / 陈师妹 | `ch07.jihe` | 冷冷一眼，一个字也没有（`poor_key` 之外唯一一条「无情报」的打探——写成动作）|
| `lingshou_dating` | 打探 | 禁地外 / 灵兽山弟子 | `ch07.jihe` | 驱兽的口袋里有活物在动；**`reveal`：`zichi_bishe` 火** |
| `jujian_dating` | 打探 | 禁地外 / 巨剑门弟子 | `ch07.jihe` | 巨剑门的剑一人一口，不用第二件法器；**`reveal`：`yan_wuchi` 火** |
| `hanyunzhi_dating` | 打探 | 禁地外出口 / 菡云芝 | `ch07.pojin` | 她只记得在沙地里躲过一劫（**不许提半日之事**）|

### 16.4 拍板前的原表（保留，供追溯）

| # | 问题 | 推荐 | 不选推荐会怎样 |
| --- | --- | --- | --- |
| 1 | 大纲「封闭前最后一次」等四处与原著不合 | **回写** | 照大纲写：玩家会以为错过这一届就再等一甲子，而原著明写下一届还在、只是更惨——第 8 章 ch316 陈巧倩追问旧事的时间线也会乱 |
| 2 | 必打几场 | **5 场，全原著** | 做 3 场（照第 6 章）：② 或 ④ 砍掉，一线天或小石殿就成了纯旁白，丢了本章最硬的两场算计 |
| 3 | 成人情节 | **暗场** | 照原著：游戏分级与商店上架都过不去；只删不暗：南宫婉此后「长期不理他」没有来由 |
| 4 | 药园两年 | **三时节、马师伯按年份收药** | 纯跳时：大纲的「暴富循环」没有手感；做成月月收药：两年里交二十几回，时长超预算 |
| 5 | 千年灵草 | **新药材黄精芝，店里不收** | 用现有药：一阶二阶上限 44 / 100，到不了千年；让店收：一株上万灵石 |
| 6 | 筑基丹方的料 | **一炉份药粉** | 三主药＋辅药逐味入方：每一炉要四样带年份的药，玩家手上的药来自蒙太奇，逐味入方只是把同一件事拆成四个字段 |
| 7 | 地火屋炼丹 | **演废丹 ＋ 亲手第一颗 ＋ 蒙太奇归一** | 全脚本：「炼丹正式解锁」名不副实；全手动：约三成的成算要按上百次确认，且章末筑基丹数随种子漂 |
| 8 | 筑基怎么发生 | **脚本服八颗** | 面板冲关：要给筑基丹做加成、给面板开「服丹」入口，而原著是一次性的剧情 |
| 9 | 暗场范围 | **韩立不在场的一律不演** | 演茅草地：玩家比韩立先知道白衣少女，沼泽那一场的「她是谁」就没了 |
| 10 | 名字随时间 | **两个 role** | 一个 role：名牌提前写出「南宫婉」「菡云芝」 |
| 11 | 剑符、金光砖 | **当场收回** | 留着：金光砖一招土系重击会一路带进第 8 章 |
| 12 | E2 | **本章做** | 挪到第 8 章：见 16.2 第 1 条 |
| 13 | 敛气术 | **只作旗标** | 进法术表：战斗里多一行点不动的法术 |
| 14 | 七星草种子 | **不用** | 硬给它一个用处：原著没有 |
| 15 | 陈师妹的筑基丹 | **不还** | — |
| 16 | 瓶子容量 | **本章补** | 留给第 8 章：章末 fixture 带着不一致 |
| 17 | 中阶灵石 | **独立物品** | 算作一百块低阶：与原著「没人肯换」相反，还会让地火定金变成随手一扣 |
| 18 | 踏云靴等 | **只作物品** | 做装备：本作没有装备系统 |
| 19 | 墨蛟的境界 | **写十三层** | 写筑基中期：压制系数让这一仗打不过或只能靠友军 |
| 20 | 马师伯 | **只用挂点** | 摆 NPC：过不了规则 30 或让他在园里站成雕像 |
| 21 | 配方门闸范围 | **连已有的一起堵** | 只管本章两张：第 4–6 章面板上的「定颜丹方」「结丹灵药方」「一级妖丹」照漏 |
| 22 | 陆师兄 | **十二层、木、180** | 保持十一层火属性：与原著不合，且「风墙」「青弧」「化蛟」全成了火 |

---

### 16.5 校对（`docs/ch07-review.md`，3.0 / 5）与测试路之后的裁决（2026-09-29，协调者按推荐；用户预授权）

| 条目 | 裁决 | 谁做 |
| --- | --- | --- |
| HIGH-1 换词不换骨（14 段、80 处残留，§4.2(e)） | **先把 3.2 里转述原著镜头的正文真删掉**（13.1 宣布作废而正文没删，是这 80 处里至少 32 处的上游），3.1 表 26c「大摇大摆」一类同改；再照校对 §4.2(e) 逐行改；第 13 节第 4、6 条补做 | 整改路 |
| M1 章节卡「血色试炼」在 `ch06.done` 就上屏 | **登记例外、不改名**：第 6 章改卡名是因为「升仙令」会剧透牌子的身份；「血色试炼」是本章大事件的名字、不是悬念，又是大纲章名。12.2 补一行例外，`Ch07Acceptance.No6` 的章节卡那一条改判「卡片例外」 | 整改路 |
| M2 北门拦门话带出「巫钧山」与担保信物 | 照校对建议改文案；「巫钧山」补进 `LexiconTests` 表一（协调者）或确认 12.2 章内先后断言管得住 | 整改路 + 协调者 |
| M3 `fudan.clash` 时间说反 | 照校对建议改 | 整改路 |
| M4 血红芝 300 年卖 3460 灵石 | 三种改法取最窄的一刀：**`herb_xuehong_zhi` 改 `tradeable: false`**，收药摊与年份上限不动 | 整改路 |
| LOW 16 条 | 照校对第九节建议改；对白压回约 17300 字（施工图第 14 节） | 整改路 |
| 测试路 CRITICAL / HIGH：② 一线天、③ 封岳（不带天雷子）打不过；①④⑤ 一回合打完 | **平衡路**按 `tests/BattleHand.h` 那只手量：意图玩家（施工图 8.1）五场都打得赢、② ③ 要吃药、①④⑤ 不许一回合；③ 不带天雷子也打得过（天雷子是省力，不是唯一解）。只动第 7 章敌人的角色数值与编成；记录写 `docs/ch07-balance.md` | 平衡路 |

## 17. 施工纪律（沿用前六章，外加本章的七条）

沿用 `docs/ch04-design.md` 第 8 节、`docs/ch05-design.md` 第 17 节与 `docs/ch06-design.md` 第 17 节。本章另加：

19. **暗场就是暗场**：13 第 8 条那几处，文案里一个形容身体的字都不许有；拿不准的句子删掉，不要「写得含蓄一点」。
20. **名字按节点解禁**：定颜丹（4b）、岳麓殿（3）、血色试炼（8）、师祖（18）、菡云芝（28 战后）、南宫（31 问名之后）、李化元（33）——写到哪一节才许用哪一个。
21. **禁地里不给玩家「安全感」**：没有回头的门、没有 NPC 可以聊天；每一处先交代刚死过的人；打坐点只放在原著他真歇过的三处（树冠、树洞、山顶一带）。
22. **借来的威能当场收**：剑符、金光砖在耗尽的那一段脚本里 `take` ＋ `forget`，不拖到章末。
23. **账按年份下限算**：凡交药、换宝、上交，一律 `take_aged` / `count_aged`，不许用 `take(id, n, 恰好年份)` 猜玩家浇到了几年。
24. **幼苗与成熟药分开给**：同一味药，幼苗 `herb_age = 1`、成熟的 ≥ 300；嗅灵兽那一步靠这条线分。
25. **第 6 章的图只经 patch 改**：`ch06_huangfenggu`、`ch06_baiyaoyuan` 上的本章对象一律由 `genmaps_ch07.patch_*()` 加，不改 `genmaps_ch06.py`，不动第 6 章已有的对象。

---

## 18. 施工偏差（内容路，2026-09-29）

按落地的样子记下与本文不一样的地方：每条写「本文怎么写 → 落成什么 → 为什么」。没列在这里的，都按本文落地。

**地图与站位**

1. **第 6 章两张图的修补也进了第 6 章的生成**（第 17 节第 25 条）：本文写「只经 `genmaps_ch07.patch_*()` 加、不改 `genmaps_ch06.py`」。协调者派工单要求 `genmaps_ch06.build_all()` 也套上两处 patch（`ch06_huangfenggu.tmj` / `ch06_baiyaoyuan.tmj` 只有一份，两章共用；只在 `genmaps.py --check` 里套，重新生成第 6 章就会把第 7 章的对象冲掉）。`genmaps_ch06.py` 只加了两行调用，第 6 章已有的对象一个没动。后果：黄枫谷新开的两扇门（`portal_to_fangshi`、`portal_to_yuelu_dian`）**第 6 章里也在**，那时走上去会看到 `ch07.block.fangshi` / `ch07.block.yuelu_dian`——两句都写成不靠第 7 章剧情也读得通（不含「岳麓殿」、不点第 7 章的人名）。
2. **节点 6 不在「山头」**：黄枫谷那张图上没有山头。挂在他回园子必经的东口咽喉 (46,26)(46,27)，七个人站路两边（visible `ch07.chouhan` / hidden `ch07.murong`）。
3. **节点 16 的报名案摆在王师叔身边**：本文写「百机堂另一格」。王师叔的 NPC 第 6 章起一直站在大殿前 (26,12)、没有撤场旗标，案就摆在 (25,12)——脚本里说的与地图上站的是同一处（第 6 章校对 LOW-7 的教训）。
4. **出口不摆掩月宗的人、南宫婉、向之礼**：他们都在他之后才出来（32b 脚本里一个一个交代），落地时就站在出口，与脚本对不上。出口那一批只摆落地时已经在的人。

**脚本**

5. **节点 26b 不加 `{key.*}` 提示**：第 13 节第 9 条预计要一条。插刃做成 choice 循环（「插下一柄」按八次，中途取消 = 收手、不置旗标、回来重插），选项不点名按键，用不着新的键位提示。
6. **节点 32a 拨一天**：3.3 的日历在节点 28 之后不再拨日子，可出禁地是第五日下午。`xiashan.lua` 加 `advance_days(1)`，「第五日」的横幅落在这里。
7. **节点 35a 的钱一律「有则扣」**：本文只写传送阵一块灵石、定金一块中阶灵石。节点 34 刚发了中阶 15、低阶 300，可玩家把灵石花光、把中阶灵石在打坐时吃掉（`restoreMp`）都是合法的——没有灵石，守阵弟子记账放行；没有中阶，按公认兑率收一百块低阶；都没有，丑汉看在剑诀的份上许他先欠着。主线不因任何一样东西卡住。

**数据与文案**

8. **战斗开场白换了 key**：`ch07.battle.{lu,yixiantian,fengyue,xiaoshidian,mojiao}.intro`（蓄势那几句同前缀）。`data/text/battles.json` 里有第 7 章旧占位编成的同名 key，协调者合回时删旧的；新旧不撞。
9. **`item.desc.pill_zhuji_dan` 不在 `ch07_items.json` 里写**：`items.json` 已有这一条，重复 key 门禁报错；筑基丹的说明沿用旧的。
10. **路径行动**（16.3 十四条，落地十四条）：
    - `hanyunzhi_dating` **不落**：菡云芝在出口那一批里（visible `ch07.pojin` / hidden `ch07.chujindi`），可玩家从环形山回到禁地外时落在出口通道底 (23,34)，往外第一格就是 32b 的 `enter` 触发（3.1 定的 mode，测试路按它写判据）——这段时间里走不到她跟前。落了就是一条永远挂不出来的死条目。她那一句（沙地里躲过一劫）不另找地方放。
    - `chouhan_dating` **拆成前后两条**（`chouhan_dating` / `chouhan_dating2`）：「`ch07.dihuo` 之后换一句恭敬的」——路径行动没有按旗标换文案的字段，只能拆，时段首尾相接。
    - **when 按 NPC 在场改**：`xiang_dating` `ch07.xiang` → `ch07.qipai`（「又找了人」挂在坡下那一批）；`lingshou_dating`、`jujian_dating` `ch07.jihe` → `ch07.qipai`（这两人只在坡上那一批；进禁地即收起，揭的破绽出来以后就用不上了）。
    - **until 收窄**：吴风到 `ch07.lianqi`（节点 8 他自己想通同一件事）、于执事到 `ch07.chuling`、叶师叔到 `ch07.danbao`（药篓第二年已说出侄孙没成，之后不能再说「十拿九稳」）、王师叔到 `ch07.jihe`。
11. **`MAP_BGM` 只加 1 条**：契约 6.2 写「加 9 条登记」。`catalog.MAP_BGM` 是「地图上用到的曲目 id」一张集合，不是一图一条；九张新图里只有山洞的 `bgm_night` 是新 id（13 → 14），其余八张用的都已登记。`docs/audio.md` 同步 13 → 14，七首的「用在哪里」补上第 7 章的图，并注明 `bgm_night` 从此也挂在图上。
12. **六个非人形敌人的尺寸**：`mo_jiao` 40×32、`tuishan_shou` 36×28（比野猪大一圈）、`tiebi_yuan` 32×32，其余 32×24（火焰鼠画小，只占一角）。
13. **8 / 15 的挂点不在吴风「面前」**：吴风面朝西，面前那两格 (38,9)(39,9) 是第 6 章的 `trigger_wufeng`（已演过、惰性，但仍占格）。`trigger_chuangong` 挨在他南边 (40,10)；35a 的 `trigger_chouhan` 在丑汉面前 (43,5)。两处起初都压在 NPC 身上那一格，与 3.1「同一格只挂一个对象」冲突（`Ch06TriggerModeTests` 那条同名断言抓出了吴风那一处），落地前挪开。
14. **卖符少女的欠账在 19b 了结**（协调者 2026-09-29 补派，第 6 章 16.5 裁决）：本章她与他头一回照面是 19b 的队列。`liedui.lua` 在他冲络腮胡子做鬼脸之后：`ch06.qianyao > 0` 时两句旁白——他想起太南谷还欠着她几瓶养精丹；她隔着人群摆手作罢、看了看身上那件灵兽山的袍子（不要药、不另开买卖、不新建旗标），随即 `flag.set("ch06.qianyao", 0)`；`== 0` 一个字不提。队列里两人说不上话，所以写成手势。

**留给协调者裁的**

15. **墨蛟 `killable_by` 土也放行流沙术**：本文 8.2 ⑤ 说「只有金光砖破得了它」，可 `killable_by` 按攻击类别判，第 6 章学会的流沙术（土）一样算数。按本文的字面要收紧，得给金光砖一个专属类别——那是引擎路的事，本路没动。

### 18.16 校对整改（`docs/ch07-review.md` 3.0 / 5，裁决 16.5；整改路，2026-09-29）

> 只追加。平衡那一行（16.5 末行）归平衡路，本节不涉及；`data/flags.json`、`data/text/ui.json`、`tests/LexiconTests.cpp`、`tools/validate*.py` 一行没动。
> 文案改动面：第 7 章文案改值 330 条、新增 6 条（`ch07.yaolou.z1_ma_say`、`ch07.chouhan.change_say`、`ch07.zhuji.jianjue2`、三件替身物品的描述）、删除 7 条（`murong.boy2_curse`、`wanbaolou.brick_know`、`fanhui.leave`、`pojin.slope`、`liangshi.go`、`xiaoshidian.offer`、`sannian.quiet`，连同脚本里调它们的那一行）。

1. **HIGH-1 上游：3.2 真删了。** 3.2 整节重写，只剩脚本调用、分支结构与死局兜底这类机制要求，每段一律指回 13.1（节首有注）；3.1「为什么是这一个」一列里转述原著镜头的 11 格同改（26c「大摇大摆」、26b「抹泥」、35a「铃铛」、35b「玉牌一贴」、36「翠绿蒲团」、6「雷声」、7「逐字」、13「靠石壁」、26a「探头」、34「睡足一觉」、23 的日子）；3.4 三条 R-3 提示随下面 3、LOW-6/7/14 改了。
2. **HIGH-1 本体：§4.2(e) 80 行逐行改完**，做法照 13.1——框架件留着，那一行原著的质感（那句话、那个反应、那个过场、那个次序）拿掉，换上的都从韩立自己的三只眼睛里出，不写反话：
   - **删掉的**（那一项本来就不是框架件）：长老争着看兄弟、他暗自高兴、粗矮青年骂人、见金砖符想起剑符、先劝大汉各走各路、推断丝线怎么割的头、「他倒乐得清静」、「穹老怪」、「哪敢有意见」、她收元神后的笑、陆师兄「别怪我」那一开。
   - **换载体的**：许老筑基——看他两根指头隔空勾药罐（不再是天眼照不透）；两个时辰的钱上楼前一次付清（他自己的算计，`cangshi.lua` 把第二个 `take(1)` 挪到了前头）；丁老验药——舌尖抿须根；许老讲失传——从他自己铺子断货讲起、不用反问；马师伯拒绝——不给理由；陆师兄夸口——「看谁先没了法力」；罚兄弟——停一个月丹药；蓝衣女子传音——一句「拿孩子换你的清净」；陈师妹变冷——落在看他那一眼上；浮云子与李师祖——当场议注、不提「老规矩」「上回」，穹前辈在赌约说定之后才开口；正与邪——「看一株草，想拔就拔」、七家的来历只留结论（13 第 4 条补做）；胡子的反应——眯眼把他的脸记下；黄土坡——只留「寸草不生」；催人进洞——李师祖一甩袖；乌龙潭——碎石滩、铁锈色的水，听见拔草声才伏下；出手救黄衫师姐——牵引之术瞒不住，躲也白躲；镜子定刃——「扎进冻油里」；青绳——从树杈上兜头落下、她低头去扯时被截；封岳自报名号；铜门三尸——他在心里把死人又添三个；飞蛇——药香里混进腥气；钟吾的立场——封岳抢过他的药；雾——看得见树根看不见树梢；光柱——不说方位；巨蜈蚣——在石厅里打毒血会毁苗，所以引出去（`n32` 提示同改）；引它——在拐角外拿母刃敲石壁；救菡云芝——「这一回他没守规矩」一句；大汉认镜——只留认出与后果；他的算计——结丹长老找炼气弟子不问是谁杀的；「那……我呢」之后他先答一句不杀、再收拾；她的名字——掉在地上的木牌背面刻着；立誓那一支——他怕的是那根丝线被人知道；「师祖」——他把她的灵压量了一遍；再等等——数她扔出的符；紫液——只写人没了、地上多了坑；银剑——只说掺了银精、换人驱使不动；蛟腹那团东西——递过去、不问；他的应法——说出去对他没好处；金箱——她不问就收走；「再无交集」——「这是他跟她之间的最后一句话」；出口——不写穹前辈何时到、霓裳上下打量她、交药一株株报年份、李师祖从账上念出名字、「门下还缺一个记名的，你来不来」；谢师礼——只留「护短」；三年——大哥「又没成、回了家族」、名气「路上有人多看他一眼」；丑汉——他坐着等人醒；十九号——只留门号、拿枯枝试火、失败是药香一苦（13 第 6 条补做）；服丹——药力走骨头（13 第 6 条补做）；出关——丑汉掉了茶碗，他点头而去。
   - 另把校对列为「轻」的 50 多处顺手改了（清单见 §4.2(e) 末尾）。
3. **M1**：12.2 登记章节卡例外（理由：第 6 章改卡名是因为「升仙令」剧透牌子；「血色试炼」是本章大事件的名字、不是悬念，又是大纲章名）。`Ch07Acceptance.No6_TheChapterCardAndTheTwoGatesSeenInChapterSixSayNothingOfChapterSeven` 改判「卡片例外」、注「校对整改 16.5」：只放行 `ui.chapter.07.title` 这一个词，卡片两个 key 的值照旧逐字钉死。
4. **M2**：`ch07.block.yuelu_dian` 改成不带地名、不提担保（「门框上有一道空着的凹槽」）；`ch07.block.huanxingshan` 顺手去掉「宝珠」（第 28 步就读得到，比月阳宝珠早）。另在 `Ch07AcceptanceTests` 的 12.2 断言表（`gates()`，追加在表尾、不挪动已有下标）补「巫钧山」一行：节点 1、2 的 key 与第 6 章可见的拦门话不许含、头一回必须在步序 3。**表一要不要补「巫钧山」（153）归协调者。**
5. **M3**：`fudan.clash` 改成离进禁地只剩一个来月、闭关三个月就赶不上这一趟，与 3.3（≈880 → ≈916）对得上；`fudan.weigh` 同读过，改成两条路各赌一样。
6. **M4**：`herb_xuehong_zhi` 改 `tradeable: false`（note 同步，LOW-5）；收药摊与年份上限不动。`Ch07LedgerTests.TheMarketRoadNeverPaysMoreForOneHerbThanTheWholeChapterCap` 的判据跟着改：收得了的名单只剩黄精、紫参，血红芝挪进一条反面断言（`tradeable == false`），注「校对整改 16.5 M4」。第 9 节补第 7 条。
7. **LOW 16 条**：
   - LOW-1：`liedui.li_turn`、`guanzhan.soul`、`chuangong.idle` 说话人改空；`yaolou.z1_ma`、`chouhan.change` 拆成旁白＋台词两行（新 key `z1_ma_say`、`change_say`）。另有几句原先挂人名的旁白一并改空：`wanbaolou.boss_ask`、`murong.who`、`baoming.fruit_wang`、`pojin.go`、`xiaoshidian.after_name`、`jiaoshi.box`、`chukou.fuyun`、`chukou.li_iron`；`shulin.name` 改由封岳说。
   - LOW-2：`chuangong.fingers` 不再比手势。
   - LOW-3：点破之前一律用替身物品，点破那一节换正名（与 `maifu_shaonv` → `han_yunzhi` 同一个办法）：节点 18 给 `story_bubao_faqi`「布包」→ 34 换 `story_kuilei_gongshou`；节点 23 给 `story_xiao_yuanjing`「小圆镜」→ 28 大汉认出后换 `story_qingning_jing`；节点 23 给 `story_hei_xue`「黑靴」→ 24 钟吾叫出后换 `story_tayun_xue`。三处都是「`take` 成功才 `give`」。法术 `magic_ji_qingning` 节点 23 就要学（④ 要用），名字改成「祭镜」，前后都说得通；银辉剑的描述不再提驱使不了（节点 31 她点破）；「穹老怪」改「穹前辈」。**大殿前李师祖的 NPC 不动**：把 `jihe.li` 改成他一直站在殿门口、这时才迈进来，与图上对得上（原句「殿后转出」与站着的 NPC 相抵）；他挂名签的时段就是目标链第 21 步起，与 12.2「师祖」的步序同一口径。
   - LOW-4：`item.desc.pill_zhuji_dan` 改成「资质差的，一粒未必够」，不说只此一次。
   - LOW-5：`herb_xuehong_zhi` 的 note 与描述改掉「禁地出产的筑基丹主药」。**`flags.json` 里 `ch06.zhongzi` 的登记说明（「第 7 章符纸原料」）归协调者改。**
   - LOW-6：`chaibao.plan` 改成苗分好、等着下地；`yaolou.hj_less3` 改成苗得先下到田里、满一年才浇得进；n02 前面补同一句。
   - LOW-7：n15 不再说「不回头」（`Ch07Walkthrough` 里对应的 `expectHint` 同步）。
   - LOW-8：8.1 按脚本推的实际背包重写成表；8.2 ① 的「紫参」拿掉；`yeyu.drain` 改成「能回法力的东西抓到什么塞什么」，`yeyu.pain` 同步。
   - LOW-9：`ch07.path.xiang_dating.text` 不再说他身边多了个黄衫女修（站位不动）。
   - LOW-10：`qiong_bet` 补上输的一头（两家的彩头都归他）；`li_iron` 改成李师祖冲穹前辈应下铁精。
   - LOW-11：`chaibao.bundle` 改成第 6 章数过的是桌上那一堆，这一包当时没拆。
   - LOW-12：`shulin.lua` 把 `advance_days(1)` 与「第二日」横幅挪到第一夜之后。
   - LOW-13：记一笔、不改：`yuanjiao.months`「四个多月」、`chuangong.next`「半年以后」、`chukou.answer`「快三年」都是 1.1 的硬约束数字；玩家在蒲团上多坐的日子会让它们对不齐，本章没有倒计时（3.3 已接受）。
   - LOW-14：n02 补「瓶满了就不再凝」；第 14 节 B 桶药园改 7 分。
   - LOW-15：`zhuji.jianjue` 拆成两句（新 key `jianjue2`）；全章台词含标点最长 70 字（2026-10-09 复验订正原记的 62 字，最长项为 `yeyu.drain`）。**对白总量**（46 个脚本调用的全部文案，含各分支与选项，与校对 §六同一口径）**19,143 → 17,501 字**；第 14 节同步（A＋B ≈ 75 分）。
   - LOW-16：十个不足 5 字的选项全部补到 5–9 字（「跳下去救她」「照实说封岳死了」「再等她耗一阵」「现在就出手」「现在就动手」「先只亮出一株」「当场跪下叩拜」「说靴子是捡来的」「躲在树上不出声」「再插下一柄」）。
8. **机器口径**（校对 §4.1 那一把量具，阳性对照先行：嵌 12 字报 12、对照句报 2）：第 7 章文案 797 条、21,370 字；对 ch152-216 最长 **6**、对全本最长 **7**、≥10 字 **0**。同场同位的 5–6 字片段逐条看过，剩下的都是专名、数字或 13.1 的框架件；改的时候带出的两处（`yeyu.see`「发不出声」、`xieshili.relief` 那一句的骨架）已再改掉。
9. **测试**：`Ch07AcceptanceTests.cpp`——No6 章节卡改判例外、`gates()` 表尾补「巫钧山」、通关那只手的背包账补三件替身物品的进出（18 / 23 / 24 / 28 / 34 五步）、`keyOrders()` 补三条替身物品描述并把三件正名的描述挪到点破那一步、`expectHint` 的 n15、n32 两句；`Ch07LedgerTests.cpp`——收药摊封顶那条（上面第 6 条）。改动处都注了「校对整改 16.5」。新判据各做过一次变异（在文案副本上跑，不碰主树）：血红芝改回可卖、节点 2 的一句与北门拦门话里塞进「巫钧山」、南门拦门话里塞进「血色试炼」——各自变红，复原后转绿。

### 18.17 平衡路交来的三笔（`docs/ch07-balance.md` 第 8 节，协调者转；整改路，2026-09-29）

1. **8.2 ① 的意图一栏照实测改**：两回合、剑符收场、花掉四成多法力。原写「要吃中阶灵石与紫参才耗得过他」量不出来——那只手不吃回法力的东西。8.2 其余几场的实测数字不抄表，8.2 表下第二条指到 `docs/ch07-balance.md`。
2. **小刀符宝、黑水、紫液三门敌方法术在数值上不出手**：封岳、墨蛟的法力给了 0，三门法术留在 `data/magics`、E5 照登，它们按 8.2 作为蓄势重招（③ 小刀符宝、⑤ 紫液）的横幅演出，数值上是「普攻 × 蓄势倍率」（同一笔还有严的流沙术、火焰鼠的火弹术不出手，银辉剑只放开场一剑）。若将来要以法术真正施放，需要引擎给「蓄势那一手用指定法术」的口（`BattleAi`），本章不做。
3. **交接存档重写过一遍**：整改动了脚本里给的东西（替身物品在 18 / 23 / 24 / 28 / 34 五步进出）和 `advance_days` 的位置（`shulin.lua`），照 `tests/ChapterFixture.h` 的命令（`FANREN_WRITE_CH07_FIXTURES=1`，槽 `ch07fix`）重写了 `tests/fixtures/ch07-end-{first,second}.sav`。结果与平衡路写出的两份**逐字节相同**（441 个字段 0 处不同）：两侧筑基初期（21 / 21）、筑基丹 17、灵石 451 / 431、中阶灵石 18 / 17、日子 3834 / 3809；三件替身物品章末都是 0，傀儡弓手、青凝镜、踏云靴各 1。替身物品是「`take` 成功才 `give`」，一进一出，终局本就不该变。

### 18.18 续跑复验后的收尾（2026-10-09）

独立复验 `docs/ch07-reverify.md` 为 3.5 分：原清单 80 条中 77 条成立，剩下第 62、67、74 条反写与一处坊市事实错误。

1. `jiaoshi.box` 删去“她没问他”的反写引子，保留金箱归她的框架。
2. `chukou.pile` 删去“他没倒袋子”，用逐株报药名、年份与管事记账承接交药；少药分支同步改成清点记账。
3. `chouhan.wake` 恢复 13.1 第 35a 段“韩立把丑汉弄醒”的框架：韩立连续报来意，把人叫醒；不用铃声、弹起或坐等醒来。下一句 `grumble` 同步接上这次唤醒。
4. `fangshi.people` 改为两国修仙界并非仇敌、商客往来较多，订正原“不相往来”与跨境贸易相抵的说法。
5. 18.16 的最长台词记录由 62 订正为 70 字。上述收尾只改文案与记录，没有改调用、旗标、日历、奖励或存档判据。
