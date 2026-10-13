# 第 11 章设计：天星城·结丹

> 对应原著 ch389–415（27 章）。目标时长 55 min（大纲第 11 章条目）。**乱星海篇第二章**：起**筑基后期**（`Realm::FoundationLate = 23`），止**结丹初期**（`Realm::CoreEarly = 31`），大纲 4.3。
> 权威依据：`H:/Work/Kys/docs/大纲.md` 第 151–157 行（第 11 章条目）与第 185–223 行（硬约束、年龄、境界曲线、改编标注）；大纲与原著冲突之处以原著为准，待回写（本稿查出大纲、lore、人物总表、占位数据对不上的二十五处，见 1.2）。
> 与第 10 章的衔接：第 10 章章末停在他与曲魂飞离小寰岛、不写去向（`ch10.done`，落点 `ch10_xiaohuan` (50,39)）；本章开局由 **patch `scripts/ch10/zhifadui.lua` 末尾补一行 teleport** 接到 `ch11_fangshi` 城门（1.4、16 节 E1），与第 9→10 章的先例同一做法。第 10 章交给本章的接口（`docs/interfaces-p3-ch10.md` 第 3 节）逐行照接，见 1.4 与第 2 节。
> 本文是编剧、关卡、引擎三路的共同施工图；事实来源只有研究笔记 `notes/ch11/research.md`（2026-10-12），表里的 id、旗标、章号、数字一律照抄笔记。
>
> P3 · 2026-10-12 · **施工图阶段：本文不改任何代码、脚本、数据、地图，也不碰 `data/flags.json`**（旗标清单见第 11 节，交协调者登记）。
> 第 16 节的拍板项**已按推荐定下**（用户预授权：按推荐走）；与前几章已拍的板冲突的，一律维持旧板。
> **3.2 只写框架件与脚本动作，不摘原著镜头顺序与台词**；每段必须落地的剧情事实与**不许碰的原著质感**都在 13.1，写手与复验共用那一张表。

---

## 0. 这一章要做到什么

1. **一座不准打架的城。** 他第一次走进一座由「星宫」管着的巨城（ch389）：不准城内斗法、长住缴灵石、星宫不管城里的事（A4）；红环蓝环（A9）、圣山八十一层（A14）、百年一付的洞府（A15–A17）——规矩全写在钱上。**本章城内 0 场战斗**，路径行动 `challenge` 0 条（第 6 节、16.3）；他花的每一笔都「有几块扣几块」、没有一处门闸要钱（E13、N.1）。
2. **猎妖是一门生意。** 交灵石乘传送阵去外星海的妖兽岛、杀妖取丹、回城换灵石（A6），没有「猎妖令」（B9）。**幕二做成一个可重复的循环**：坊市买霓裳草 → 星空殿传送 → 凝翠岛 → 红瑚岛阵心诱妖 → 战斗 ① → 收购行卖丹（E15、L.4）；五级妖丹值多少（A7）要让玩家第一次打赢就看见。六级鸡冠蛟那一场**逃是正解之一**（I 表 ②）。
3. **六十年一关。** 回城两年炼丹、散功、生死关，六十年后结丹，全城可感（A41–A43）。**不做面板冲结丹**（E3）：三段蒙太奇、六次 `advance_days(3600)`、一处 `realm.advance(realm.CORE_EARLY)`（E5）；跳过的岁月不计游玩时长。降尘丹与雪灵水「有则扣」，旁白三版（E4）。出关之后（n17 起）外人才直接跟他说话（E27）。
4. **乱星海的另一张脸。** 妙音门请他助拳，集合岛上十几人；隐煞门岛上他第一次以结丹之身压制筑基（③）；乌丑现身自认劫货嫁祸，极阴祖师附身，妖尸拦路——打不死的东西只能困住（④，E25；数据上 `killable_by ["水","火"]`、胜负仍是气血归零，「困住」是文案，契约 E3）。**不演他不知道的事**：范赵密谋、极阴与赤火分赃、三女议事（3.0 表三）；虚天殿只许在 n23 由乌丑与孙姓中年人口中说出一句（第 12 章钩子，O）。
5. **二十余年的一把剑。** 一节天雷竹（A62）、三阶绿液催成金雷竹（A63）、倾家荡产凑辅料（A64）、六根（A69）；坊市一间小店「青竹小轩」磨入世心境（A66–A68），店铺回访只是结算（E22）。**炼剑走面板**（E16）：洞府炼器台、配方必成，一件 `weapon_qingzhu_fengyun_jian` 代表七十二口。
6. **章末停在金青送来半幅图**（A74）：七八个势力一夜叛乱的消息只到谣言为止（A72）；置 `ch11.done`，不 `ending()`，不写去向。

---

## 1. 硬约束（原著核对）

章号由研究笔记用 `tools/novel/novel.py find / read` 逐条核过（2026-10-12）：**ch389–415 二十七章**；两份摘要 `digest-389-401.md`、`digest-402-415.md` 只当线索。A 表每一行的章号都是 `find` 的命中、片段是原文截取；本节照抄笔记 A 表（74 条），分幕排列。

### 1.1 硬约束表

**幕一　入城（ch389–392）**

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| 1 | 已探明海域叫内星海，外围未探明的叫外星海、别名妖海 | ch389 | 典籍玉简台词、路径行动 K1；「妖海」只在典籍里 |
| 2 | 星宫（亦称天星宫）管辖整个乱星海，年月久到典籍无确数 | ch389 | 世界观揭示节点 n01 旁白 |
| 3 | 天星双圣：两位城主，从不出城、不露面，名号流传近千年 | ch389 | 只在传闻与典籍；不建 role、不上图（E6）|
| 4 | 天星城规矩：不准城内打斗；长住者每月缴灵石；星宫不干涉城中事 | ch389 | 城内 0 场战斗；「每月」只在典籍台词，不做机制（B7）|
| 5 | 大商家设常年门面，各自开拍卖行与收购行 | ch389 | 坊市有收购行（商店卖出口）、拍卖行只作门面与路径行动（E9）|
| 6 | 猎妖流程：交灵石乘传送阵去妖兽岛、杀妖取丹、回城换灵石；无许可制度 | ch389 | 幕二整段循环；无「猎妖令」，接单形式是任务板悬赏与商行订单（改编标注）|
| 7 | 五级妖丹近千灵石；六级妖丹够一位筑基修士一生修炼 | ch389 | `material_yaodan_wuji` 价 950 已合锚；收购行 sellRate 0.8（L 表）|
| 8 | 摘星大会：每百年在天星城选外二十四星岛岛主 | ch389 | 词表 M；路径行动一句传闻，不做事件 |
| 9 | 蓝环临时停留，缴三块灵石、灵光闪七天；红环永久，普通修士缴八百 | ch390 | n02：红环扣 8 中阶（有则扣），否则蓝环扣 3 低阶 |
| 10 | 结丹修士可免费长久居住 | ch390 | 曲魂的红环不花钱；台词点明结丹与筑基待遇之别 |
| 11 | 韩立自己掏八块中品灵石买红环，滴血认主 | ch390 | 同上；「中品」即本作中阶灵石 |
| 12 | 凡人向导鲁二开价两枚灵石，韩立应「满意给五枚」 | ch390 | n03 二选一：雇（扣 5 低阶）/ 不雇；都走得通 |
| 13 | 城内无禁空禁制，可御器飞行 | ch390 | 旁白一句；地图上不做飞行 |
| 14 | 圣山八十一层：前五十层对外、后三十层星宫、顶层圣宫 | ch391 | 租赁台词与 K 表；地图只做第 1 层坊市、第 39 层、第 50 层（第 4 节）|
| 15 | 租期按百年计，到期重交，付不出被星宫收回 | ch391 | 租金一次付清，不做续租；旁白点「百年」|
| 16 | 可租洞府：三十一层三座、三十九层四座、四十七层两座 | ch391 | n04 三选一只在台词里列，韩立只能选三十九层（原著）|
| 17 | 三十九层洞府费用两千七百块灵石，韩立付二十几块中阶灵石 | ch391 | 扣 27 中阶灵石（有几块扣几块，E13）|
| 18 | 过三十层后修炼速度加快四分之一还多 | ch391 | 洞府静室 `effectiveness` 125（E11）|
| 19 | 噬金虫排奇虫榜第十二，靠数量取胜；此时数百只 | ch392 | 虫室旁白；物品 `story_shijin_chong` 不改 |
| 20 | 在天星城半年后闭关两年整，造出二百多只兽形傀儡 | ch392 | n06：advance_days(180+720)；给 `story_kuilei_qun` |
| 21 | 坊市在圣山第一层，占该层一半以上 | ch392 | G 表 `ch11_fangshi` 即第一层 |

**幕二　猎妖（ch393–397）**

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| 22 | 白门阁书铺：百余块灵石复制全部妖兽资料 | ch393 | n07 扣 1 中阶（或 100 低阶）|
| 23 | 两个大容量储物袋花上千灵石，容量约十倍 | ch393 | 只作路径行动「求购」一句，不扣钱（背包无容量）|
| 24 | 丰乐拍卖行号称天星城第一拍卖行，物品进拍卖行贵一多半；韩立身家数千灵石 | ch393 | 门面 + 路径行动；不做拍卖面板（E9）|
| 25 | 星空殿在第五十层，白色大理石大殿，一名白衣修士管理成排传送阵 | ch393 | G 表 `ch11_xingkong` |
| 26 | 传送按次收费不按人：一次七百灵石，最多七人 | ch393 | n09 首次拼阵付 200；以后每次 7 中阶（E14）|
| 27 | 首次同行：韩立付二百、妇人一伙付五百；发传送符 | ch393 | 同上；传送符只在台词 |
| 28 | 凝翠岛传送石屋有星宫修士守阵；小街有杂货、法器、符箓店与收购行 | ch393 | G 表 `ch11_ningcui_jie` 四间店两家可用 |
| 29 | 枣红老者邀联手猎妖均分妖丹，曲魂回绝 | ch393 | 路径行动 K7（切磋改「求购」口径不合，用 inquire）|
| 30 | 杂货铺精细海域图一百灵石（上面定价，丰乐盟名下）| ch394 | n10 扣 1 中阶，给 `story_waihai_tu` |
| 31 | 郝远天（结丹大汉）卖碧磷鱼妖尸给收购行「范小子」，邀合作被婉拒 | ch394 | 角色 `hao_yuantian`、`fan_xiaozi`；收购行是卖丹入口 |
| 32 | 外星海规矩：不贴海面、不飞太高；结丹修士也结伴；聚众太多会引来七级围攻 | ch394 | 旁白与路径行动 K8；本章不设「组队」机制 |
| 33 | 霓裳草：十三叶，每百年展一叶，展叶持续六到七天，散妖兽之诱香；别名诱妖草 | ch395 | 饵物 `herb_nichang_cao`；每次诱妖扣 1 株 + 1 滴绿液（E15）|
| 34 | 五级妖兽是最佳目标；六七级有人阵亡风险；八级以上只有元婴敢碰 | ch395 | 战斗 ② 鸡冠蛟「可逃、不致命」的依据 |
| 35 | 红瑚岛三处必经路各布一座大阵（颠倒五行阵、天风狂烈阵、幻形天罗阵）| ch395 | n11 一句旁白、不发物品（沿第 9 章 16.2 第 9 条口径）|
| 36 | 首杀千目妖：五级，滚圆带刺、数十只绿眼；一顿饭取丹 | ch396 | 战斗 ① 敌 `qianmu_yao` |
| 37 | 红瑚岛一个多月，每隔两三日一批，灭十几头五级 | ch396 | 诱妖循环 n12：每次 advance_days(3) |
| 38 | 霓裳草四百年份引来六级鸡冠蛟；此后只催到三百年 | ch396 | 战斗 ② 的触发：诱妖第 5 次 |
| 39 | 外星海五年：数百颗五级妖丹；傀儡损耗十之八九剩二三十只 | ch396 | n14 旁白；`story_kuilei_qun` 回收 |
| 40 | 数百颗五级妖丹最少值数十万灵石 | ch397 | 经济账 N：玩家实得按 5–10 颗算，其余「数百颗」只在旁白 |
| 41 | 回城后闭府炼丹整整两年；材料卖给不常驻的小商家 | ch397 | n15 advance_days(720)，给中阶灵石 30 |

**幕三　结丹（ch397–399）**

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| 42 | 散功半月降到筑基初期，再入生死关、辟谷 | ch397 | 散功只写旁白，**不调 `realm.demote`**（E4）|
| 43 | 六十年后结丹：黑云闪电、百余里灵气漩涡，全城可感 | ch397–398 | n16 三段蒙太奇 + `realm.advance(CORE_EARLY)`（E3、E5）|
| 44 | 金丹四色、拇指大小；曲魂结煞丹四年，韩立五年多 | ch398 | 文案「四色」可用；年数只写「数年」|
| 45 | 二转年数原文自相矛盾（三十多年 / 十余年）| ch398 | 回避具体年数（B16）|
| 46 | 结丹后寿元五六百年，可用丹火炼法宝；青元剑诀第七层得剑影分光术 | ch398 | 出关旁白；剑影分光术只提名、不做法术 |
| 47 | 噬金虫数万只、壳现金斑；血玉蜘蛛四级顶峰，收入灵兽袋 | ch398 | 战斗 ④ 友军 `xueyu_zhizhu` |
| 48 | 出关见七八道传音符：邻居恭贺与各方招揽，一律回绝 | ch399 | n18；路径行动 K10 |
| 49 | 三级傀儡一击相当筑基中期；辅料贵到一贫如洗 | ch399 | 友军 `kuilei_yuan`；n18 扣中阶（有几块扣几块）|

**幕四　妙音门（ch399–411）**

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| 50 | 莲儿（十六七岁炼气期）拦路，范静梅（妙音门）请到清河茶馆 | ch399 | n19；角色 `lian_er`、`fan_furen` |
| 51 | 赵长老结丹初期；范夫人压价一成再加半成；付一小堆中阶灵石 | ch400–401 | n20：收 `story_yaoshou_cailiao`，给 40 中阶 |
| 52 | 天雷竹为三大神木之一，范夫人称有一小节下落 | ch401 | n20 结尾的钩子 |
| 53 | 妙音门两月前押货遇劫，汪门主战死；天雷竹每千年增高寸许 | ch402 | n21 台词；助拳酬劳 = 天雷竹 + 另挑一件 |
| 54 | 集合岛：十几人，结丹五人，余皆筑基后期；赤火老怪（元龟岛，结丹后期，葵水魔功）带紫灵仙子来 | ch402–403 | n21 旁白；角色 `chihuo_laoguai`、`zi_ling` |
| 55 | 隐煞门岛上：筑基邪修化煞妖；门主结丹后期、四名结丹初期 | ch404 | 战斗 ③ 敌 `yinsha_shayao` ×4 |
| 56 | 乌丑现身自认劫货嫁祸；极阴祖师附身乌丑（只能发挥三分之一；ch405 原文称「极阴老祖」「极阴老魔」，「极阴祖师」四字 ch406 才复出）；虚天殿三百年一现、虚天残图**疑在**隐煞门孙姓门主（乌丑的孙师叔）手——乌丑只问「有没有带在身上」，未得确认 | ch405 | n23 只让玩家听见这一句；第 12 章钩子（O）；n23 台词可用「极阴老祖」|
| 57 | 天都尸火焚人成灰；禁神牌交三分之一元神；双圣禁令已松 | ch406 | n23 旁白；韩立不交 |
| 58 | 赤火倒戈困住孙姓中年；众人四散；隐煞门四结丹是煞丹分身 | ch407 | n24 旁白；与曲魂同类——玩家知道的只有这一句 |
| 59 | 妖尸（天都尸）不怕法宝、伤口自愈、空中瞬移；韩立放一百多只青色巨猿傀儡与两头血玉蜘蛛，以蛛网与紫火困住后遁走 | ch408 | 战斗 ④：胜利写「困住」不写杀死 |
| 60 | 入地一个月才出；范夫人传音符邀至隆兴客栈 | ch409 | n25 地点改到清河茶馆（改编，B23）|
| 61 | 紫灵识破曲魂是身外化身、与本体同为结丹初期 | ch410 | 台词一句；不改 `qu_hun_shadan` |
| 62 | 天雷竹分两节，带根的一节在紫灵手；以此竹加每年三百灵石聘名义长老 | ch410 | n25 给 `material_tianleizhu` + `story_miaoyin_yaopai`；年俸见 N |
| 63 | 竹两寸长、小指粗；腰牌一块；白雷竹（两千年以下，白色雷电）、五千年以上雷电变蓝、金雷竹（万年以上，辟邪之雷）——原著**没有「蓝雷竹」「蓝雷」这个词**（全本 0 命中）| ch411 | n26 三阶：白→蓝→金各 1 滴绿液；中间一阶的名牌「蓝」是本作改编命名，文案只说「雷电变蓝」、不出「蓝雷竹」三字（12.2）|
| 64 | 两年凑齐辅料，倾家荡产、卖掉仅剩妖丹；一滴万年份绿液催成金雷竹 | ch411 | n26 扣全部中阶灵石与剩余五级妖丹（有几扣几）|

**幕五　竹与剑（ch412–415）**

| # | 约束 | 出处 | 本章怎么用 |
| --- | --- | --- | --- |
| 65 | 再滴数月长出幼竹；决定炼七十二把一套（标准十二把）；二十余年计划 | ch412 | n29 根数计数到 6 |
| 66 | 坊市小店「李氏杂货」改名「青竹小轩」，只卖符与药材，后加低阶法器；邻居何仙师（炼气三四层）| ch412 | G 表 `ch11_xiaoxuan`；商店 `ch11_qingzhu_xiaoxuan`（L）|
| 67 | 开店目的：入世心境、炼器与阵法；大衍诀第四层需入世 | ch412 | 旁白；大衍诀不做机制 |
| 68 | 第九个年头炼阵旗阵盘；中级符箓二三十张成一次；店开二十余年，扮五十余岁 | ch413 | n28/n29：advance_days(7200) 分两次；回访结算 |
| 69 | 第六根金雷竹催到万年份即收店；炼前打坐三天三夜；一根竹炼成十二块液 | ch413 | n29 收店条件 = 6 根 |
| 70 | 六根三个月全炼成液；布阵半月；黄道吉日焚香两日；七十二根火柱、剑胚一天一夜；精血与辅料依次加入 | ch414 | n30 蒙太奇；配方改一套为一件（E16）|
| 71 | 两个月后出关，七十二口剑已入体内；二十多年心血 | ch415 | n30 给 `weapon_qingzhu_fengyun_jian` ×1 |
| 72 | 乱星海七八个势力一夜叛乱、六连殿在内；谣言双圣走火入魔 | ch415 | n31 旁白——只到谣言为止 |
| 73 | 结丹期进阶要六级妖丹炼的丹药 | ch415 | 第 12 章接口 O；本章不发六级妖丹入药 |
| 74 | 金青（邻近结丹修士、四十来岁）送白珍珠半幅古修士地图，请破阵 | ch415 | n31 章末；`story_bai_zhenzhu` |

**名字按首见章号解禁**（第 12 节 M 表）。全作禁用：银月（ch615）、紫罗极火（ch731）、黄龙（作独立专名查无；**黄龙丹 ch27 起即有**、乱星海篇 ch367/373 仍服，词表若登「黄龙」须放行「黄龙丹」）、猎妖令（0 命中）。本章不许提前：虚天残图 / 虚天殿只许在 n23 由乌丑与孙姓中年人口中说出一次；极阴岛只许作为乌丑的来历；元瑶、文思月（ch430）、明珠轩（ch513）、交换会（ch535）本章 0 处。

### 1.2 大纲、lore、人物总表与占位数据里对不上的地方（已核，待回写）

| 在哪 | 原说法 | 复核 | 本章以什么为准 |
| --- | --- | --- | --- |
| 大纲第 11 章人物 | 「文樯」 | 文樯见 ch369–371（第 10 章）与 ch430（第 12 章）；ch389–415 区间 0 命中 | **回写**大纲：本章人物删「文樯」；本章不出场 |
| 大纲第 11 章人物 | 「木龙真人」 | 木龙真人仅 ch371 一处；本章区间只有「木龙碑」（ch409，妙音门镇派之宝）| **回写**大纲：删「木龙真人」，加注「木龙碑 ch409 只在传闻一句」；本章 0 出场 |
| 人物总表 `fan_furen` | 「先与韩立以妖兽材料换天雷竹」 | ch401 是材料换中阶灵石、天雷竹只是「线索」；ch402 天雷竹是助拳酬劳；ch410 由紫灵以竹聘长老 | **回写**总表 summary；本章按原著三步走（n20 / n21 / n25）|
| 人物总表 `zi_ling` | 首见 ch403、「紫灵仙子」 | ch403 首见；「汪凝」真名 ch403 口中自称、ch409 真容 | 不改；本章名牌「紫灵仙子」→ ch409 起「紫灵」（第 5 节）|
| 人物总表 `wu_chou`、`jiyin_zushi` | 首见 ch405 | 乌丑 ch382 首见（第 10 章已记）；「极阴祖师」名号 ch382 已由六连殿长老口中说出 | 第 10 章 16.1 第 1 条已回写乌丑；**本章回写极阴祖师首见 ch382** |
| 人物总表 `zhao_meng_zhanglao` | 「赵长老 / 孟长老」 | 叛徒是赵长老与一名儒生长老（ch404 紫灵呼「赵长老，符长老」；ch405 称其为「中年儒生」）；孟姓结丹修士是外请助拳者（ch402–403）；ch405/410 范夫人与紫灵又称「赵孟两位长老」——原文称谓前后不一 | **回避**：本作只写「赵长老」「符长老」，孟道友不叛；总表加注 |
| lore 4.3 | 「长住者按月缴灵石」vs 同节「租洞府百年计费」 | ch389 典籍写「每月」；ch391 租赁写「百年」——两句原文都有 | **本作**：红环一次 800、洞府百年 2700；「每月」只在典籍台词，不做机制 |
| lore 第七节 | 「韩立在乱星海开丹药店」已标部分属实 | ch412 只卖符与药材、后加低阶法器，ch413 再加中阶、上阶法器与中级符箓 | 商店条目照此（L 表）；不卖丹药 |
| 大纲 | 「猎妖令」 | 0 命中 | 已删；本章接单只有任务板悬赏与收购行订单，`origin` 写「改编」|
| 大纲人物 | 「天星双圣」 | 本章区间只在典籍 / 传闻（ch389、406、408、415）| **不建 role、不上图**；人物总表那一行保留，加注「传闻」|
| 占位 `b11_ningcuidao_youyao` | 敌方 `siji_haishou` + `sanji_haishou`，奖励修为 700 / 灵石 150 / 四级妖丹 | 首杀是五级千目妖（ch396）；妖丹是五级 | **就地改写**：敌 `qianmu_yao` ×1 + `siji_haishou` ×2（第二波，海兽随草香而来，改编）；掉落 `material_yaodan_wuji`；探针挪到第 12 章（E19）|
| `siji_haishou` note、`wuji_haishou` note | 「四级大致对应筑基后期」「五级≈结丹初期 ch503」 | 与 lore 4.4 一致 | 不改；`wuji_haishou` 本章不用（它是 `b13_waixinghai_lieyao` 的）|
| `material_jinleizhu` note | 「紫灵以半截天雷竹加年俸 300 聘」 | ch410：两节中带根的一节；年俸三百是妙音门付韩立 | note 措辞改「带根一节」；价值锚不动；**同时把 `tradeable true` 改 `false`**（现文件 price 20000、tradeable true——青竹小轩 0.6 一根就卖 12000 低阶、收购行 0.8 卖 16000，章末「低阶数百到四千」与「只卖符与药材」都不成立；契约 4.1）|
| `recipe_qingzhu_fengyun_jian` | `productCount 12`、金雷竹 ×4、灵石 ×20、`requiredProficiency 80` | ch413 六根；ch415 七十二口「一件法宝收入体内」；他是一次炼成、无材料再试 | **改**：`productCount 1`、`inputs` 恰金雷竹 ×6 ＋ 新增辅料 `material_lianjian_fuliao` ×1（**去掉灵石 ×20**：炼剑是主线必经门闸，收店后无处挣低阶，留着就是第 6 章 HIGH-3 那种死局）、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`、**`guaranteed true`**（契约 E1：`successChance` 夹在 [5,95]，没有这个字段面板只有 65%，失败折损三根金雷竹即死局）（E16）|
| 摘要 389-401 | 「ch397 闭府炼丹整整两年」 | `find "整整两年"` 命中 ch397，属实 | 照用 |
| 原著 ch398 | 二转年数「三十多年」与「十余年」 | 原文自相矛盾 | 回避具体年数，只写「数十年」|
| 原著 ch404 | 韩立圈住的筑基修士「五六名」/「四五名」 | 原文不一 | 编成取 4 |
| lore 人物缺口表 | 「坊市商行掌柜（丰乐拍卖行 / 白门阁 / 明珠轩）」 | 明珠轩首见 ch513（黑石城）| **本章不许用「明珠轩」**；掌柜用改编名、`origin` 标改编 |
| 大纲 4.4 | 猎妖悬赏须标改编 | — | 任务板条目 `origin: "改编"`、图鉴注「改编」|
| 原著 ch402 | 一处「天音门」 | 笔误 | 回避；「天音门」0 处 |
| 大纲 vs 任务书 | 「六十年闭关…见待办 4」vs「待办 2」 | 编号出入 | 不管编号；表现方式见 E5 |
| handoff 第 4 节 | 「拍卖那一半缺数据目录 + 加载器 + 面板（第 11 章前后要）」 | — | 本章**不做**面板（E9）；留给第 12/13 章再议 |
| 原著 ch409 | 紫灵约在「隆兴客栈」三楼 | 原著如此 | 改编：地点并到清河茶馆雅间（省一张图）；文案不写「隆兴客栈」，图鉴注改编 |
| 原著 ch408 | 青色巨猿傀儡「一百多只」vs 本作编成 | 原著如此 | 友军 `kuilei_yuan` ×2 代表「一群」，台词写「上百只」|
| lore 4.6 第 5 条 | 「天星城：三十九层洞府、青竹小轩、闭关结丹」 | 与原著一致 | 照用 |

**回写归口**：B1–B3、B5、B6、B13 由协调者回写大纲 / lore / 总表 / 物品 note（E30）；本文不动仓库。

### 1.3 境界曲线与上限

- **原著**：ch389 筑基后期入城 → ch397 两年炼丹、散功半月降到筑基初期、生死关 → ch397–398 六十年后结丹（意外成功）→ ch398 结丹初期、二转 → ch415 仍是结丹初期（进阶要六级妖丹入药，A73）。
- **游戏**：起点**筑基后期 / 上限筑基后期**，`Realm::FoundationLate = 23`（`src/core/rules/Realm.h` 第 33 行）；`realmCap 23`、`formerRealm 22`（已越过，契约 E1 重修加速不再生效——契约第 3 节第 1 行）。
  - **n16（biguan）`local ok, code = realm.advance(realm.CORE_EARLY)`**——`realm.CORE_EARLY = 31`（`scripts/common/api.lua` 第 217 行附近），写法照 ch10：`ok` 为假时 `talk` 一句占位、`return`。这是本章**唯一一处** `realm.advance`；`realm.cap` 0 处；`realm.demote` 0 处（散功只写旁白，A42）。
  - **不做面板冲结丹**（E3，与第 10 章 16.1 第 25 条同一口径）：原著是六十年生死关、「意外成功」（ch398），面板摇骰子会把一次性的大事变成可刷的事；`breakthroughPillBonus` 仍是返回 0 的桩（`CultivationScene.cpp` 第 363 行），**不接**（E4）。降尘丹 5、雪灵水 2 在 n16「有则扣」，扣几颗在旁白里换一句（五颗全扣 / 部分 / 零颗三版）；天火液 0、血凝五行丹 0 不提。
  - 面板上 `storyCap` 保持 23 直到 n16；n16 之后 `realm.advance` 顺带把上限抬到 31（契约 script 7.2），玩家在面板上看到的下一关是「结丹中期」、所需修为 5000——本章不许他过（`realm.cap` 不抬到 32，面板按 `BreakthroughBlock` 回绝）。
  - **章末结丹初期 / 上限结丹初期**；`formerRealm` 仍是 22。
- **锚点**（Realm.h 注释第 110–115、175–185 行；冲关所需修为见 Realm.cpp 第 143–146 行；写测试时读表，不抄数）：筑基后期 气血 480、攻 44 防 30、冲关所需 3500；**结丹初期 气血 700、攻 62 防 46**、冲关所需 5000（→ 结丹中期，本章不用）；结丹中期 900、76 / 58、7000；结丹后期 1300、102 / 82。data/roles 同境界样本：结丹初期 64/47、66/50、62/56、61/48（`zi_ling` 61/48、`wuji_haishou` 66/50、`qu_hun_shadan` 86/52——化身偏高是第 10 章的平衡结果，不动）。注释第 175 行明写「筑基后期与结丹档仍待对应章节标定」——**本章就是标定结丹初期的那一章**：平衡路用 `tests/BattleHand.h` 量第 8 节四场后，把锚点注释改成「第 11 章标定」。
- **起点修为** 2092 / 2031（第 2 节），距 3500 尚远——佐证不做面板冲关。
- **曲魂**：`qu_hun_shadan` 结丹初期，全章在队、数值不动、名牌不动（E18）；紫灵 ch410 识破它是化身只是一句台词（A61）。
- **对手**：千目妖五级（①，CoreEarly）、四级海兽（①，FoundationMid）；鸡冠蛟六级（②，CoreMid）；隐煞门煞妖筑基后期 ×4（③）；妖尸 CoreEarly（④，困而不杀）。**只露面不打的**：星空殿执事结丹初期、枣红脸老者结丹初期、郝远天结丹初期、赵长老结丹初期、符长老结丹初期、孟道友结丹初期、赤火老怪结丹后期、隐煞门门主结丹后期、乌丑结丹中期（只说话）、极阴祖师（元婴；名牌 realm 写 CoreLate 只为不越本作上限，O-9）、结丹邻居、金青结丹初期。**只提到的**：天星双圣、汪门主、封某、木龙碑。

### 1.4 第 10 章留下的线头

| 线头 | 原著本章有没有下文 | 本章怎么交代 |
| --- | --- | --- |
| 章末落点（`ch10_xiaohuan` (50,39)，`ch10.done`）| ch389 一月赶路到天星城 | **patch `scripts/ch10/zhifadui.lua` 末尾**：在 `flag.set("ch10.done")` 之后补一行 `teleport("ch11_fangshi")`——**脚本 `teleport` 不认 spawn 名**（`scripts/common/api.lua` 第 63 行 `teleport(map_id, x, y)`，引擎 `loadMap(command.a, "")` 恒落 default spawn；第 10 章先例 `teleport("ch10_xiaohuan", 50, 39)` 是写坐标），所以城门 spawn `chengmen` 在 `genmaps_ch11.py` 里标 `default=True`（笔记 E1，与第 9→10 先例一致；契约第 3 节「开局」第一支）；`ch10-end` 两份 fixture 按 patch 后重生成（测试路，O-10）；第 10 章图一个字不动 |
| 通缉（契约第 3 节「通缉」）| ch389 只在他心里一句 | 不设状态字段；只在 n01 旁白与 K2 一句（E28）|
| `formerRealm` = 22 | 已越过 | 不读、不写；重修加速不再生效 |
| `qu_hun_shadan` 在队（结丹初期）| ch390–394 入城、租府、传送、小街都由它出面；ch410 被识破 | 全章在队、不 `party.remove`；曲魂代言到 n17 为止（E27）|
| 混元钵（`story_hunyuan_bo`，化身用）| ch392 曲魂炼化 | **不在背包**：第 10 章 `linshi.lua` 第 37–38 行已 `take` 掉，两份 fixture 里只有 `story_juhun_bo`（聚魂钵）；n06「曲魂炼化混元钵」只作旁白一句，不引物品、不 `take` |
| 噬金虫（`story_shijin_chong`）| ch392 数百只、ch398 数万只、ch415 产卵控神 | 全在旁白；物品不改 |
| 血玉蜘蛛（`story_xueyu_zhizhu`）| ch395 扑草、ch398 四级顶峰、ch408 吐网困妖尸 | 不扣；④ 友军 `xueyu_zhizhu_ally` ×2 |
| 傀儡弓手 | ch392 他另造二百余只兽形傀儡 | 本章不进编成（沿第 10 章 16.1 第 23 条）；`story_kuilei_qun` n06 给、n14 收 |
| 绿煌剑（`story_lvhuang_jian`）| ch398 结丹后护身 | n17 旁白一句；不动 |
| 乱星海丹方、丹道评鉴（story）| ch397 两年炼丹 | 配方不建（L.3）；猎妖的理由在台词里 |
| 真元丹方（`recipe_zhenyuan_dan`）| — | 不动；三十九层丹房沿用旧配方 |
| 降尘丹 5、雪灵水 2 | ch397 生死关前 | n16 有则扣（C.3）；不接 `breakthroughPillBonus` |
| 五级妖丹 1 | ch389 他知道它值近千灵石 | 可在收购行卖；n26 扣光 |
| 定颜丹 3 / 4 | ch399 邻居想换 | Z2（K12 `linju_z2.lua`）`take` 1 |
| 大衍诀玉简、土离决 | ch412 大衍诀第四层需入世 | 旁白；不做机制 |
| 海域图、储物袋、聚魂钵、神风舟 | ch393 另买精细海图与大储物袋 | `story_waihai_tu` 新物（n10）；储物袋只在 K4 一句；旧物不动 |
| 小瓶：容量 6、筑基五日一滴 | ch395 以绿液试草、ch411 三滴催竹、ch412 幼竹 | 不动瓶子代码；结丹后 3 日一滴由引擎按境界给（`Application.cpp` 第 130 行）；**n16 `realm.advance` 顺带把容量抬到 9**（`Application.cpp` 第 1337 行 `max(capacity, bottleCapacityFloor(Core))`，`Bottle.h` 结丹 9）；诱妖与培竹都 `bottle.spend(1)` |
| 旧灵田五块（`field_xiaohuan` 等）| 走不回去 | 不动；新开 `field_tianxing_39`（E12）|
| `learnedMagics` 11 门 | ch398 剑影分光术 | 本章不新学（E17）；剑影分光术只提名 |
| 乌丑（`wu_chou`，第 10 章只说话、一个字不动）| ch405 现身、自认劫货嫁祸 | 仍只说话、不上场（n23）；探针挪到 `wu_chou` + `b12_neidian_duoding`（E19、O-2）|
| 极阴祖师（名号 ch382 已出）| ch405 附身乌丑 | 新 role `jiyin_fushen` 只说话；首见章回写 382（B5）|
| 语言两版（第 10 章） | — | 本章不再分（契约第 3 节「语言」）|
| `b11_ningcuidao_youyao` 是第 10 章定下的 selftest 探针 | — | 本章改成真编成（8.2 ①）；探针改指第 12 章 `wu_chou` + `b12_neidian_duoding` |
| `CHAPTER_MEANS` / `MEANS_LAST_CHAPTER = 10` | — | 登 `CHAPTER_MEANS[11]`、`MEANS_LAST_CHAPTER = 11`（E19）|

---

## 2. 起点：正式 C10 档的资源事实与 C11 入口边界

本节逐字段读自 `tests/fixtures/ch10-end-first.sav`、`ch10-end-second.sav`（研究笔记 C.4，2026-10-12 用 Python 读出）；不从第 10 章施工图抄。**入口边界**：现档停在 `ch10_xiaohuan` (50,39)，E1 patch 之后两份 fixture 的 `mapId / position` 会变成 `ch11_fangshi` 城门格，须由测试路重生成并无 write 比对后交付（O-10）；本节现有档只供资源合同核对，不能手改位置冒称入口已验。开局 guard 只看 `ch10.done`，开局不推进日期。

| 字段 | first | second | 本章用法 |
| --- | --- | --- | --- |
| `day` | 16003 | 15978 | 开局不推进；时间账见 3.3 |
| `realm / realmCap / formerRealm` | 23 / 23 / 22 | 同 | 1.3；`former` 不由脚本改 |
| `hp / maxHp`、`mp / maxMp` | 480 / 480、320 / 320 | 同 | 满状态开局 |
| `cultivation / cultivationRemainder` | 2092 / 1350 | 2031 / 1290 | 距 3500 尚远，佐证不做面板冲关 |
| `aptitude` | 50 | 50 | — |
| `material_lingshi`（低阶）| 254 | 234 | 起点「数千灵石」只能靠中阶折算 |
| `material_lingshi_zhong`（中阶，价 100）| 117 | 116 | 本章所有大额付出都扣中阶（N.1）|
| 丹药 | 降尘丹 5、雪灵水 2、筑基丹 13、定颜丹 3 / 4、金疮药 7 / 8、回气丹 2、养精丹 3 / 1、清毒散 2 / 0、蚀心散 0 / 2、五毒水 0 / 1、七毒水 0 / 1 | — | 降尘丹 / 雪灵水见 1.3；定颜丹 K12「换珍稀物」；战斗资源两侧不拉平 |
| 材料 | 五级妖丹 1、丹砂 3、炼器药 2、血红芝 3（1 年）、紫参草 3（1 年）、土骨花 1 / 2、七星草种子 1 / 0、精铁 0 / 2 | — | 五级妖丹是丹道钩子；本章卖丹走收购行 |
| 符箓 | 护身符 4 / 3、火球符 16、土牢符 1 / 2、天雷子 0 / 1、金刺符 1 / 0、小刀符宝 1、蓝蛟符 1、血灵钻 1、魔焰初阶 1 | — | 战斗手段（规则 25，8.2 末列）|
| 兵刃 | 烈阳剑、冷月刀、碧光刀、玉带短剑、透明丝线 | 同 | 普攻类别：剑、刀、暗器 |
| 关键 story | 绿煌剑、血玉蜘蛛、傀儡弓手、噬金虫、海域图、储物袋、聚魂钵（`story_juhun_bo`）、神风舟、乱星海丹方、丹道评鉴、土离决、真元丹方、大衍诀玉简——**没有 `story_hunyuan_bo`**（第 10 章 `linshi.lua` 已消耗，实读两份 fixture 确认） | 同 | 不扣；1.4 引用 |
| `learnedMagics`（11 门）| 火弹术、御风决、天眼术、流沙术、冰冻术、祭金符、祭青蛟、青元剑芒、祭青火掌、祭小刀、祭红线针 | 同 | 本章不新学（E17）；青元剑芒 `needRealm FoundationEarly` 已可用 |
| `party` | `qu_hun_shadan`（active）| 同 | 全章在队；不 `party.remove` |
| `bottle` | capacity 6、drops 6、matureKnown、owned | 同 | 筑基 5 日一滴、结丹后 3 日一滴（引擎给）；n16 升境后 capacity 抬到 9（1.4）|
| `fields` | field_baiyaoyuan 6（second 侧第 6 槽血红芝熟）、_jiao 2、field_dongfu 4、_nei 2、field_xiaohuan 4（四槽都空）| — | 都走不回去；本章新起 `field_tianxing_39` |
| `knownWeaknesses` | 36 个 role | 35 个 | 本章新 role 的破绽都未揭 |
| 旗标 | `ch10.done 1`、`ch10.yuyan 2`、`ch10.shizi 1`、`ch10.kaifu_ri 6836 / 6811`、`ch09.done 1` | — | 开局 guard 只看 `ch10.done` |
| `mapId / position` | `ch10_xiaohuan` (50,39) | 同 | 开局 teleport 落点见 E1 |
| `alchemyProficiency / forge / talisman / formation` | 100 / 0 / 0 / 0 | 同 | 炼器熟练 0 → 炼剑不能走面板摇骰（E16：配方 `difficulty 0 / requiredProficiency 0`）|

两侧差异来自真实历程，本章不能据此另开分岔或拉平资源。入口版重新生成后须再核其 SHA、完整字段及变动原因，若与本表不同先查真驱动证据，不能先改 fixture 或规则。

---

## 3. 主线节点（五幕、31 个节点）

### 3.0 主干怎么定

**判据三条**（与第 8–10 章同一套，先后有序）：

1. **韩立亲身在场、而且后面的章要用到它的结果**——演成可玩场。
2. **韩立在场、但只是一段「事情就是这样发生的」**——进旁白 / 蒙太奇（三五句，交代因果，不演过程）。
3. **韩立不在场、或者只有作者知道**——**不演**。玩家只知道韩立知道的。

**表一　可玩场（31 条）**

| # | 原著事件 | 章 | 节点 | 为什么要演 |
| --- | --- | --- | --- | --- |
| 1 | 一月赶路、买典籍、远望巨城 | 389–390 | n01 | 内外星海与星宫的来历——乱星海篇的世界观在这里才揭开 |
| 2 | 城门指环：红环八块中品、蓝环三块 | 390 | n02 | 第一笔钱；结丹与筑基的待遇之别（曲魂免费）|
| 3 | 鲁二自荐向导 | 390 | n03 | 二选一；凡人与修士搅在一起的活法 |
| 4 | 租洞府：三十一 / 三十九 / 四十七层、两千七百 | 391 | n04 | 百年一付；第二笔大钱 |
| 5 | 安顿：布阵、密室、虫室、药园 | 391–392 | n05 | 立府种药（第 10 章模板）；静室 125 |
| 6 | 半年育虫、两年造傀儡 | 392 | n06 | 猎妖的本钱；混元钵 |
| 7 | 白门阁买妖兽资料 | 393 | n07 | 悟出霓裳草的前置 |
| 8 | 读资料一月、绿液试草 | 393–395 | n08 | **诱妖循环的钥匙**；坊市杂货与星空殿自此开 |
| 9 | 星空殿拼阵、传送符 | 393 | n09 | 传送费；出海的唯一入口（可重复）|
| 10 | 凝翠岛小街：枣红老者、郝远天、海图 | 393–394 | n10 | 收购行（卖丹口）与外星海规矩 |
| 11 | 红瑚岛布三阵 | 395 | n11 | 阵心诱妖的前提 |
| 12 | 诱妖、千目妖 | 395–396 | n12 | 战斗 ①（可重复）；第一次看见妖丹值多少 |
| 13 | 四百年份引来鸡冠蛟 | 396 | n13 | 战斗 ②；六级妖丹是第 12 章的钩子 |
| 14 | 收手回城：五年、数百颗、傀儡剩二三十 | 396–397 | n14 | 出海关闭；幕二的结算 |
| 15 | 两年炼丹、卖材料、散功 | 397 | n15 | 生死关前的准备；散功只旁白 |
| 16 | 生死关六十年、结丹 | 397–398 | n16 | **境界唯一一次抬**；三段蒙太奇 |
| 17 | 出关：数万噬金虫、血玉蜘蛛四级顶峰 | 398 | n17 | 结丹后的新身份；曲魂代言到此为止 |
| 18 | 七八道传音符、三级傀儡一贫如洗 | 399 | n18 | 一贫如洗 ①；④ 友军的由头 |
| 19 | 莲儿拦路、范静梅赔罪 | 399 | n19 | 妙音门的入口 |
| 20 | 清河茶馆：卖材料、狐媚术、天雷竹线索 | 400–401 | n20 | 第一笔必收；赵长老首见 |
| 21 | 打听底细、赶制傀儡、集合岛 | 402–403 | n21 | 助拳的由头；紫灵仙子首见 |
| 22 | 隐煞门岛：煞妖四名 | 404 | n22 | 战斗 ③；结丹压制筑基 |
| 23 | 乌丑现身、极阴附身、禁神牌 | 405–406 | n23 | 第 12 章钩子只此一句；韩立不交元神 |
| 24 | 赤火倒戈、妖尸拦路、入地一月 | 407–409 | n24 | 战斗 ④；困而不杀 |
| 25 | 紫灵真容、名义长老、天雷竹 | 409–411 | n25 | 竹与腰牌（第 12 章接口）|
| 26 | 三滴催竹、两年凑辅料 | 411 | n26 | 一贫如洗 ②；辅料必给 |
| 27 | 盘店：李氏杂货改青竹小轩 | 412 | n27 | 入世；店门开 |
| 28 | 店铺回访结算 | 412–413 | n28 | 二十余年只是结算（E22）|
| 29 | 六根金雷竹、收店 | 412–413 | n29 | 炼器台开 |
| 30 | 炼剑：布阵、面板、出关 | 413–415 | n30 | **炼器面板全作第一次用上**；七十二口入体 |
| 31 | 金青送白珍珠 | 415 | n31 | 章末；第 12 章开场 |

**表二　旁白 / 蒙太奇（12 条）**

| # | 原著事件 | 章 | 放在哪 | 为什么不演 |
| --- | --- | --- | --- | --- |
| a | 一月赶路、城内无禁空可御器 | 389–390 | n01 | 赶路；地图上不做飞行（A13）|
| b | 半年育虫试验、两年整造傀儡 | 392 | n06 | 他关起门来做工 |
| c | 读资料一个月、「又过半年」 | 393–395 | n08 | 过场 |
| d | 两日巡岛、三座大阵、环心小阵 | 395 | n11 | 布阵不发物品（A35）|
| e | 五年换岛、数百颗妖丹、傀儡损耗、颠倒五行阵弃于七级妖兽 | 396–397 | n14 | 玩家实得 8 颗，其余只在旁白（N.1）|
| f | 两年炼丹、零散卖材料、散功半月、噬金虫吞草 | 397 | n15 | 过程无抉择；散功不动数据 |
| g | 六十年生死关：三段 | 397–398 | n16 | 不加任何新画面（E5）|
| h | 传音符一律回绝、三级傀儡辅料 | 399 | n18 | 过场 |
| i | 打听妙音门底细、半月赶制傀儡、集合岛十几人、「次日清晨突袭」 | 402–403 | n21 | 赶路与群像 |
| j | 赤火倒戈困住孙姓中年人、众人四散、四结丹是煞丹分身 | 407 | n24 | 一句即可；玩家知道的只有这一句 |
| k | 入地一个月 | 409 | n24 末段 | 过场 |
| l | 六根炼液三月、布阵半月、黄道吉日、七十二根火柱、两月出关 | 413–415 | n30 | 面板前后各一段蒙太奇 |

**表三　不演（韩立不在场、无从得知，或按暗场口径不演；9 条）**

| # | 原著事件 | 章 | 玩家怎么知道 |
| --- | --- | --- | --- |
| α | 范夫人与赵长老的密谋 | 400 后半 | 不知道——他只在 n22 听见范夫人「中了圈套」一句 |
| β | 极阴祖师与赤火老怪分赃 | 408 后半–409 前半 | 不知道（他已入地）；「元磁神光」0 处（M 表）|
| γ | 妙音门三女的闭门议事 | 411 中段 | 不知道——n25 只有紫灵当面说的那几句 |
| δ | 鲁二以外的凡人店主群像 | 390–393 | 不演（E24）；K12 何仙师一句带过 |
| ε | 天星双圣、星宫长老的任何露面 | 389–415 | 只在典籍、传音符与 n31 谣言里（E6）|
| ζ | 隆兴客栈那一场的原址 | 409 | 改编并到清河茶馆（B23）；「隆兴客栈」0 处 |
| η | 乌丑对孙姓中年人索图之后的下落、虚天残图在谁手里 | 405–409 | 他只听见 n23 那一句；下文是第 12 章的事 |
| θ | 妖尸之后极阴祖师怎么处置紫灵等人 | 408 | 不知道——n25 紫灵只说「暂迁天星城」|
| ι | 七八个势力叛乱的内情、双圣走火入魔是真是假 | 415 | 只到谣言为止（A72）；**不许替原著补结论** |

**主干的一句话**：入城 → 指环 → 租府 → 安顿 → 傀儡 → 资料 → 霓裳草 → 星空殿 → 凝翠岛 → 布阵 → 诱妖（循环）→ 鸡冠蛟 → 收手 → 炼丹散功 → 六十年 → 结丹 → 出关 → 一贫如洗 → 莲儿 → 茶馆 → 集合岛 → 煞妖 → 乌丑 → 妖尸 → 名义长老 → 催竹 → 开店 → 结算 → 六根 → 炼剑 → 白珍珠。前半章他在一座不准打架的城里花钱、出海挣钱、关门六十年；后半章一次助拳把他卷进极阴岛的事，二十余年一把剑是他给自己的答案。

### 3.1 每一处挂点怎么开始（`mode` / `once`）

分法沿用前十章：**这件事是他做的（interact），还是发生在他身上的（enter）。** `mode` 只有 `enter` 与 `interact` 两种（`WorldScene.cpp` 第 154、197 行），不用 `auto`。
**三处 `once=false` 的循环挂点**（n09 星空殿、n12 诱妖、n26/n29 培竹）与一处回访结算（n28）**不写 `set_flag`**，脚本开头自己判「已演过 / 已关闭」，整数旗标自己累加（`ch11.lieyao`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.jiesuan`）——与第 10 章学话、两组阵位同一口径（`docs/handoff.md` 第 3 节第 6 条）。
**挂在 NPC 上的节点**（n02、n03、n04、n07、n09、n19、n20、n25、n27、n31）NPC 没有 once，脚本开头看 done_flag 判已演过；NPC 的出场 / 退场用 `visible_flag` / `hidden_flag`（第 4、5 节）。
**同格按旗标换触发器**的三组（全文只此一份清单，3.1 末 bullet 与验收 3 都指它）：红瑚岛登岸格（n11 → n14，`guard ch11.buzhen`）、礁岛登岛格（n22 → n23 → n24 三连）、炼器台（n30a `trigger_lianjian` 与 `facility_lianqi_39` 同格）——第 10 章 `zhenwei_*` 的写法；validate 若不许同格两个对象则挪一格（O-7）。密室门外的 n17 与 n30b **不同格**，n16 在密室内，都不算同格换。
**`guard_flag` 只是旗标真假**（map_spec 4.4），下表 guard 列只写旗标名；「`ch11.lieyao ≥ 3`」「`== 4`」这类比较一律在脚本开头判（3.2），不写进地图属性。
**脚本 `teleport` 只写图 id**（`teleport("ch11_xxx")`，落该图的 default spawn；`api.lua` 第 63 行是 `teleport(map_id, x, y)`，不认 spawn 名）：每张图把脚本落点那个 spawn 标 `default=True`（第 4 节 spawn 表），门走 portal 的 `target_spawn`。

| # | 挂点 | 地图 | `mode` | `once` | `guard_flag` | `set_flag` | 战斗 | 为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| n01 | `trigger_chudao` | `ch11_fangshi` | `enter` | ✓ | `ch10.done` | `ch11.chudao` | — | 城落在他眼前。城门 spawn `chengmen` 那一格（E1 的 teleport 落点）|
| n02 | NPC `npc_baiyi_xiushi`（`zhihuan.lua`）| `ch11_fangshi` | NPC | — | `ch11.chudao` | `ch11.zhihuan`（1 / 2）| — | 走到星宫修士面前的是他。城门内一格；曲魂先答话（E27）|
| n03 | NPC `npc_lu_er`（`luer.lua`）| `ch11_fangshi` | NPC | — | `ch11.zhihuan` | `ch11.luer`（1 / 2）| — | 鲁二凑上来，答不答应的是他。坊市街南端 |
| n04 | NPC `npc_xinggong_zhishi`（`zufu.lua`）| `ch11_fangshi` | NPC | — | `ch11.luer` | `ch11.zufu` | — | 进山脚楼阁租府的是他。北缘楼阁内。**`teleport("ch11_dongfu")`**（落 default spawn `men`）；坊市北口 `portal_shangshan` 自此开（`require_flag ch11.zufu`）|
| n05 | `trigger_anju` | `ch11_dongfu` | `enter` | ✓ | `ch11.zufu` | `ch11.anju` | — | 一进门就撞上的安顿。洞府门内一格。**`field.unlock("field_tianxing_39", 6)`**；设施 `require_flag ch11.anju` 自此开 |
| n06 | `trigger_kuilei` | `ch11_dongfu` | `interact` | ✓ | `ch11.anju` | `ch11.kuilei` | — | 关起门造傀儡的是他。密室一格。**`advance_days(180)`、`advance_days(720)`** |
| n07 | NPC `npc_baimen_zhanggui`（`ziliao.lua`）| `ch11_fangshi` | NPC | — | `ch11.kuilei` | `ch11.ziliao` | — | 进白门阁买资料的是他。坊市街北段 |
| n08 | `trigger_nichang` | `ch11_dongfu` | `interact` | ✓ | `ch11.ziliao` | `ch11.nichang` | — | 在药园以绿液试草的是他。药园旁一格。**`bottle.spend(1)`；`advance_days(180)`；`give("herb_nichang_cao", 3)`**（ch395 他从坊市收购了一批草做试验、带着幼苗上岛——这三株是诱妖的兜底，低阶耗尽也走得通）；坊市杂货与星空殿北口自此开 |
| n09 | NPC `npc_xingkong_xiushi`（`xingkong.lua`）| `ch11_xingkong` | NPC | ✗ | `ch11.nichang` | `ch11.chuansong`（首次）| — | 交灵石上阵的是他。殿内传送阵（NPC 就是阵）。**每次 `teleport("ch11_ningcui_jie")`**（落 default spawn `shiwu`）；`ch11.huicheng ≥ 1` 时只说一句 `return`。登 `kScriptTransfers` 普通三元行，所以 `flag.set("ch11.chuansong")` 与 `teleport(` 都写在**顶格**（首次与否用前置 `local first = flag.get("ch11.chuansong") == 0` 判，早退是顶格 `return`）|
| n10 | `trigger_ningcui` | `ch11_ningcui_jie` | `enter` | ✓ | `ch11.chuansong` | `ch11.ningcui` | — | 一出石屋就遇上的人。石屋门外一格。东口 `portal_chuhai` 自此开 |
| n11 | `trigger_buzhen` | `ch11_honghu` | `enter` | ✓ | `ch11.ningcui` | `ch11.buzhen` | — | 登岸即布阵（旁白）。岛西登岸格。阵心 `trigger_youyao` 自此可用 |
| n12 | `trigger_youyao` | `ch11_honghu` | `interact` | ✗ | `ch11.buzhen` | —（脚本累加 `ch11.lieyao`）| ① | 催草诱妖的是他。环心珊瑚礁一格。**`advance_days(3)`；败 game over** |
| n13 | （n12 脚本内分支）| `ch11_honghu` | — | — | （脚本判 `ch11.lieyao == 4` 且 `ch11.jiguan == 0`）| `ch11.jiguan` | ② | 第 5 次诱妖改走鸡冠蛟；胜 / 逃 / 败都置旗；**不在目标链上**（登 `kOffChainFlags`）|
| n14 | `trigger_huicheng` | `ch11_honghu` | `interact` | ✓ | `ch11.buzhen`（脚本判 `ch11.lieyao ≥ 3`）| `ch11.huicheng` | — | 收手回城的是他。岛西登岸格（n11 之后同格换）。**`advance_days(1800)`；`teleport("ch11_dongfu")`**（落 `men`）；出海自此关 |
| n15 | `trigger_liandan` | `ch11_dongfu` | `interact` | ✓ | `ch11.huicheng` | `ch11.sangong` | — | 闭府炼丹、散功的是他。丹房一格（与丹房设施不同格）。**`advance_days(720)`、`advance_days(15)`** |
| n16 | `trigger_biguan` | `ch11_dongfu` | `interact` | ✓ | `ch11.sangong` | `ch11.jiedan` | — | 入生死关的是他。密室另一格（与 n06 不同格）。**6 × `advance_days(3600)`；`realm.advance(31)`** |
| n17 | `trigger_chuguan` | `ch11_dongfu` | `enter` | ✓ | `ch11.jiedan` | `ch11.chuguan` | — | 出关是走出来撞上的。密室门外一格（n16 置旗后同格）|
| n18 | `trigger_zhaolan` | `ch11_dongfu` | `enter` | ✓ | `ch11.chuguan` | `ch11.zhaolan` | — | 传音符撞在禁制上。洞府门禁制一格。**扣全部中阶** |
| n19 | NPC `npc_lian_er`（`lianer.lua`）| `ch11_fangshi` | NPC | — | `ch11.zhaolan` | `ch11.lianer` | — | 莲儿拦路——但开口的是他。坊市街中段。茶馆门 `require_flag ch11.lianer` |
| n20 | NPC `npc_fan_furen`（`chaguan.lua`）| `ch11_fangshi` | NPC | — | `ch11.lianer` | `ch11.chaguan` | — | 进雅间坐下的是他。东北角茶馆雅间（赵长老 NPC 另一格）|
| n21 | `trigger_zhuzhan` | `ch11_dongfu` | `interact` | ✓ | `ch11.chaguan` | `ch11.zhuzhan` | — | 传音符贴在门上，上前取符的是他。洞府门内一格（**与 n18、spawn `men` 都不同格**——若写 `enter` 又与 spawn 同格，`ch11.chaguan` 置位后一进洞府就被送上礁岛，玩家走不到 `facility_cunji_39`；③④ 连打前唯一的存档点就落空了）。**进门先过存档点再到触发格**：`facility_cunji_39` 摆在 spawn `men` 与这一格之间。**`advance_days(15)`；`teleport("ch11_yinsha_dao")`**（落 `haishang`）|
| n22 | `trigger_guiju` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.zhuzhan` | `ch11.guiju` | ③ | 登岛即开打。登岛格。**败 game over** |
| n23 | `trigger_wuchou` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.guiju` | `ch11.jiyin` | — | 乌丑是找上门的。n22 置旗后同格 |
| n24 | `trigger_daoge` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.jiyin` | `ch11.yaoshi` | ④ | 妖尸拦路。n23 置旗后同格。**`advance_days(30)`；`teleport("ch11_dongfu")`（落 `men`）；败 game over** |
| n25 | NPC `npc_zi_ling`（`zhanglao.lua`）| `ch11_fangshi` | NPC | — | `ch11.yaoshi` | `ch11.zhanglao` | — | 赴约进雅间的是他。茶馆雅间（改编地点 B23；范、卓 NPC 另两格）|
| n26 | `trigger_peizhu` | `ch11_dongfu` | `interact` | ✗ | `ch11.zhanglao` | —（脚本写 `ch11.zhu`、`ch11.jinlei`，并把 `ch11.zhu_gen` 置 1——第一根在这里）| — | 滴绿液催竹的是他。药园珊瑚礁旁一格（与 n08 不同格）。**`zhu == 3` 时 `advance_days(720)`** |
| n27 | NPC `npc_li_dianzhu`（`kaidian.lua`）| `ch11_fangshi` | NPC | — | `ch11.jinlei` | `ch11.kaidian` | — | 盘店的是他。李氏杂货门口（街西）。记 `ch11.kaidian_ri`、`ch11.jiesuan_ri` |
| n28 | `trigger_jiesuan` | `ch11_xiaoxuan` | `interact` | ✗ | `ch11.kaidian` | —（脚本累加 `ch11.jiesuan`、写 `ch11.jiesuan_ri`）| — | 坐到柜台后的是他。柜台一格（与 `trigger_guitai` 不同格）。脚本开头判 `ch11.liugen` 已置 → 一句「店已盘出」`return`。**不在目标链上**（登 `kOffChainFlags`）|
| 柜台 | `trigger_guitai`（`guitai.lua`）| `ch11_xiaoxuan` | `interact` | ✗ | `ch11.kaidian` | — | — | 卖出口。portal / facility 都不读 `hidden_flag`（`WorldScene.cpp` 第 67–75 行只对 npc 判），所以柜台不做 shop facility，改成脚本：开头判 `ch11.liugen` → 一句「店已盘出」`return`；否则 `shop("ch11_qingzhu_xiaoxuan")`（`api.lua` 第 95 行既有命令）|
| n29 | `trigger_peizhu`（n26 同一触发器，`ch11.jinlei` 后换分支）| `ch11_dongfu` | `interact` | ✗ | `ch11.jinlei` | —（脚本累加 `ch11.zhu_gen`；6 时置 `ch11.liugen`）| — | 第 2–6 根（`zhu_gen` 从 1 累加到 6）每根 **`advance_days(1080)`**；第六根收店：补足到开店起 7200 日、`give` 中阶 20 |
| n30a | `trigger_lianjian` | `ch11_dongfu` | `enter` | ✓ | `ch11.liugen` | `ch11.lianjian_qian`（map_spec 4.4：`once=true` 必须有 `set_flag`，否则每次踏上炼器台都重演、再拨 120 日）| — | 走到炼器台前撞上的蒙太奇。密室炼器台同格（`facility_lianqi_39` kind forge，`require_flag ch11.liugen`）。**`advance_days(120)`**；`ch11.lianjian_qian` 不在目标链上（登 `kOffChainFlags`）|
| n30b | `trigger_chujian` | `ch11_dongfu` | `enter` | ✓ | `item.count("weapon_qingzhu_fengyun_jian") >= 1` | `ch11.lianjian` | — | 出关是走出来的。密室门外一格（与 n17 不同格）。**`advance_days(60)`** |
| n31 | NPC `npc_jin_qing`（`zhuzhong.lua`）| `ch11_dongfu` | NPC | — | `ch11.lianjian` | `ch11.done` | — | 金青等在门外，上前搭话的是他。洞府门外（`visible_flag ch11.lianjian`）。**不 `ending()`** |
| Z1 | NPC `npc_fan_xiaozi`（`shougou_dingdan.lua`）| `ch11_ningcui_jie` | NPC | — | `ch11.ningcui` | `ch11.z1`（1 / 2）| — | 接单的是他；**不在目标链上**；`ch11.huicheng` 后 fail |
| Z2 | NPC `npc_jiedan_linju_b`（`linju_z2.lua`）| `ch11_dongfu` | NPC | — | `ch11.zhaolan` | `ch11.z2` | — | 以定颜丹换珍稀物；**不在目标链上** |
| 回城 | `trigger_huicheng_chuansong` | `ch11_ningcui_jie` | `enter` | ✗ | `ch11.chuansong` | — | — | 石屋传送阵回星空殿，踏上阵就走。**`teleport("ch11_xingkong")`**（落 `dian`）；脚本里 **0 处 `flag.set(`**——它登 `kScriptTransfers` 的五元行（照 ch09 `trigger_suidao_hui`：`type trigger`、`mode enter`、`once false`、`guard_flag` 恰 `ch11.chuansong`、`set_flag` 空，`tests/ObjectiveTests.cpp` 第 862–873 行）|
| — | 各新图存档点 `facility_cunji_*` | 七张新图 | 设施 | — | — | — | — | ①② 之前（红瑚岛、凝翠岛）、③④ 之前（礁岛无——n21 是 interact，`facility_cunji_39` 在 spawn `men` 与 n21 触发格之间）|

- **`once=true` 的挂点每一条都写 `set_flag`**；条件不够（绿液不足、草不够、`realm.advance` 失败）的路都在置旗标之前 `return`。**n13 的三种结局都置 `ch11.jiguan`**（它是「一场已演」，不是「胜」）。
- **同一格只挂一个对象**：洞府里 n05、n06、n08、n15、n16、n18、n21、n26、n30a 九格与静室、丹房、炼器台、药园、存档点五处设施、spawn `men` 互不相同（n30a 与 `facility_lianqi_39` 例外：同格，trigger `enter` 先演、面板后开——validate 若不许则 trigger 挪到台前一格）；密室门外 n17 与 n30b 不同格；礁岛 n22/n23/n24 是同格三连（按旗标换）；青竹小轩 `trigger_jiesuan` 与 `trigger_guitai` 不同格。
- **四场战斗**：① 挂在循环挂点上（n12，目标链靠 `ch11.lieyao ≥ 3` 过 n14）；③④ 挂在目标链的必经挂点上（n22、n24）；② 是 n12 的分支，可逃。
- **门钥匙**（`kOffChainDoorKeys`）：坊市北口 `portal_shangshan`（`require_flag ch11.zufu`、`deny_text_key ch11.fangshi.shangshan_deny`——租府之前不许上三十九层）、青竹小轩门（`require_flag ch11.kaidian`；**不写 `hidden_flag`**——portal 不读它）、茶馆门（`require_flag ch11.lianer`）、星空殿北口（`require_flag ch11.nichang`）、凝翠岛东口（`require_flag ch11.ningcui`）——`ch11.zufu`、`ch11.kaidian`、`ch11.lianer`、`ch11.nichang`、`ch11.ningcui` 都在链上，不必另登；链外门钥匙：无。
- **脚本 teleport 登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`（七条，N.4）**。两种形式：**三元行**要求脚本里有顶格的 `flag.set("<done_flag>")` 与顶格的 `teleport("<落点图>"`；**五元行**（照第 652 行 `suidao_hui`）要求 `reusableObject` 是 `type trigger`、`mode enter`、`once false`、`guard_flag == done_flag`、`set_flag` 空，且脚本里 0 处 `flag.set(`。

| done_flag | 脚本 | 落点图 | 形式 |
| --- | --- | --- | --- |
| `ch10.done`（patch）| `scripts/ch10/zhifadui.lua` | `ch11_fangshi` | 三元（原第 668 行那条只改 `toMap`）|
| `ch11.zufu` | `scripts/ch11/zufu.lua` | `ch11_dongfu` | 三元 |
| `ch11.chuansong`（及其后每次）| `scripts/ch11/xingkong.lua` | `ch11_ningcui_jie` | **三元**（挂在 NPC 上、首次置旗，过不了五元那张表；`flag.set` 与 `teleport` 都顶格，见 3.1 n09）|
| `ch11.chuansong`（回城，不置旗）| `scripts/ch11/huicheng_chuansong.lua` | `ch11_xingkong` | **五元**：`reusableObject trigger_huicheng_chuansong`、`hostMap ch11_ningcui_jie` |
| `ch11.huicheng` | `scripts/ch11/huicheng.lua` | `ch11_dongfu` | 三元 |
| `ch11.zhuzhan` | `scripts/ch11/zhuzhan.lua` | `ch11_yinsha_dao` | 三元 |
| `ch11.yaoshi` | `scripts/ch11/daoge.lua` | `ch11_dongfu` | 三元 |

`kOffChainFlags`（规则 30，已拍板）：`ch11.jiguan`（youyao.lua，插在 `ch11.lieyao` 之后、`ch11.huicheng` 之前）、`ch11.dan_fu`（biguan.lua，`ch11.sangong` → `ch11.jiedan`）、`ch11.jiesuan`（jiesuan.lua，`ch11.kaidian` → `ch11.liugen`）、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`（peizhu.lua）、`ch11.lianjian_qian`（lianjian.lua，`ch11.liugen` → `ch11.lianjian`）（E21）。

### 3.2 脚本动作与分支

本节仅规定状态操作、控制流和挂点，不提供叙事镜头、示例台词或转述顺序。剧情事实与质感禁用项只按 13.1 核验；对白另行原创。挂点属性以 3.1、日期以 3.3 为准。脚本一律 `scripts/ch11/<id>.lua`；每一处 `take` / `give` / `realm.*` / `party.*` / `bottle.*` / `field.*` 看返回值；扣东西「有几块扣几块」（E13）。

| 节点 / 脚本 | 在场 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- | --- |
| n01 chudao | 韩立、曲魂 | 置 ch11.chudao | enter；通缉只一句；曲魂代言交代一句（E27）|
| n02 zhihuan | 星宫修士、曲魂先答 | take 中阶 8 成 → 置 ch11.zhihuan = 1；否则 take 低阶 3 → = 2；再不成也 = 2 | 三版旁白；无死局 |
| n03 luer | 鲁二 | choice 雇 / 不雇；雇 → take 低阶 5（不足照雇）、= 1；不雇 → = 2 | 取消 return；两支信息量相同（不雇则 n04 执事多说两句）|
| n04 zufu | 星宫执事、曲魂出面 | take 中阶 27（有几扣几）；give story_jinzhi_lingpai；置 ch11.zufu；teleport("ch11_dongfu") | 零块时「记在结丹前辈名下」换句（E13）；三十一 / 四十七层只在台词，不可选 |
| n05 anju | 韩立、曲魂 | field.unlock("field_tianxing_39", 6)；置 ch11.anju | enter；unlock 查返回值；鲁二 hidden |
| n06 kuilei | 韩立、曲魂 | advance_days(180)；advance_days(720)；give story_kuilei_qun；置 ch11.kuilei | 混元钵只旁白一句（物品不在背包，1.4）；噬金虫物品不扣 |
| n07 ziliao | 白门阁掌柜 | take 中阶 1 或 take 低阶 100；give story_yaoshou_ziliao；置 ch11.ziliao | 都不成则「赊」（旁白）；必给资料 |
| n08 nichang | 韩立 | bottle.spend(1)（不成 → advance_days(5) 再 spend）；advance_days(180)；give herb_nichang_cao 3；置 ch11.nichang | 第二次仍不成照置旗（不设死局）；三株草必给（诱妖门闸的兜底）；坊市杂货 facility 与星空殿北口 require_flag ch11.nichang |
| n09 xingkong | 星空殿执事、首演另有妖艳妇人一伙与枣红老者 | 顶格：若 ch11.huicheng ≥ 1 → 一句、return；local first = (flag.get("ch11.chuansong") == 0)；first → take 中阶 2，否则 take 中阶 7；顶格 flag.set("ch11.chuansong")（重复置无妨）；顶格 teleport("ch11_ningcui_jie") | once=false；不足 → 「对方全出 / 搭别人的阵」（E14），**永远走得通**；kScriptTransfers 三元行要顶格的 flag.set 与 teleport |
| n10 ningcui | 枣红老者（旁白）、郝远天、范小子、杂货掌柜 | choice 买海图：take 中阶 1 → give story_waihai_tu；置 ch11.ningcui | enter；不买也置旗；东口 portal require_flag ch11.ningcui |
| n11 buzhen | 韩立、曲魂 | 置 ch11.buzhen | enter；不发物品（A35）|
| n12 youyao | 韩立、曲魂 | 前置：item.count("herb_nichang_cao") ≥ 1 且 bottle.drops ≥ 1；bottle.spend(1)；advance_days(3)；若 ch11.lieyao == 4 且 ch11.jiguan == 0 → 转 n13；否则 battle ①；**battle 返回后**：胜 → take 草 1、ch11.lieyao += 1；逃 → 不扣草（绿液与三日不退） | once=false；缺草 → 旁白指凝翠岛杂货 return；缺绿液 → advance_days(5) return；败 game_over；**拍板：扣草挪到 battle 返回之后，逃不扣、败 game_over**（8.2 ①、验收 8 同此） |
| n13 jiguan | 韩立、曲魂 | battle ②；胜 → 六级妖丹由编成 `drops` 给、脚本 **0 处 give**（纪律 54）、take 草 1；胜 / 逃 / 败都置 ch11.jiguan | can_escape true、defeat 不致命；逃或败同一句旁白、不扣草 |
| n14 huicheng | 韩立、曲魂 | 顶格先判 ch11.lieyao < 3 → 一句 return；choice 收手 / 再猎；收手：take story_kuilei_qun 1、give story_kuilei_can 1、give material_yaodan_wuji 8、give story_yaoshou_cailiao 1；advance_days(1800)；置 ch11.huicheng；teleport("ch11_dongfu") | 门闸 ch11.lieyao ≥ 3 在脚本里判（地图 guard 只写 ch11.buzhen）；再猎 → return；置旗后星空殿只说一句 |
| n15 liandan | 韩立、曲魂 | advance_days(720)；give 中阶 30；advance_days(15)；置 ch11.sangong | 散功只旁白，**不 realm.demote** |
| n16 biguan | 韩立、曲魂隔门 | take pill_jiangchen_dan 5 → dan_fu 位 1；take material_xuelingshui 2 → 位 2；置 ch11.dan_fu；fade.out；advance_days(3600) ×2、talk、×2、talk、×2；realm.advance(realm.CORE_EARLY)；fade.in；置 ch11.jiedan | 门闸无；advance 失败 → 占位一句 return（不置旗）；旁白按 dan_fu 三版 |
| n17 chuguan | 韩立、曲魂 | 置 ch11.chuguan | enter；物品不动；E27 起外人直接与韩立说话 |
| n18 zhaolan | 韩立、曲魂 | take 中阶 全部（有几扣几）；give story_kuilei_yuan_qun 1；置 ch11.zhaolan | enter；零块时换句；结丹邻居 NPC visible |
| n19 lianer | 莲儿、范静梅 | 置 ch11.lianer | 茶馆门 require_flag ch11.lianer |
| n20 chaguan | 范夫人、赵长老、曲魂 | take story_yaoshou_cailiao 1 → give 中阶 40；置 ch11.chaguan | 材料缺则仍 give 40（n14 必给，缺只会是异常）；范、赵 hidden |
| n21 zhuzhan | 韩立、曲魂；旁白里莲儿、集合岛众人 | advance_days(15)；置 ch11.zhuzhan；teleport("ch11_yinsha_dao") | interact（门内一格，不与 spawn 同格，存档点在前）；礁岛无门、只进不出 |
| n22 guiju | 紫灵仙子、范、卓、赵、符、孟、赤火、曲魂 | battle ③；胜 → 置 ch11.guiju | enter；败 game_over；门主 NPC visible |
| n23 wuchou | 乌丑（只说话）、隐煞门门主、众人 | 置 ch11.jiyin | enter；名牌换 jiyin_fushen；韩立不交元神；虚天殿一句 |
| n24 daoge | 极阴附身、赤火、妖尸、曲魂、血玉蜘蛛 ×2、猿傀 ×2 | battle ④；胜（气血归零）文案写「困住」；advance_days(30)；置 ch11.yaoshi；teleport("ch11_dongfu") | enter；败 game_over；礁岛全员 hidden；末尾一句传音符 |
| n25 zhanglao | 紫灵（真容名牌）、范夫人、卓如婷 | give material_tianleizhu 1；give story_miaoyin_yaopai 1；置 ch11.zhanglao | 年俸三百只在台词；三人 hidden |
| n26 peizhu（前半）| 韩立 | bottle.spend(1) 成 → ch11.zhu += 1；zhu == 3 → take material_tianleizhu 1、give material_jinleizhu 1、**flag.set("ch11.zhu_gen", 1)**（第一根计数在这里，否则 n26 一根 ＋ n29 六根 ＝ 7 根、章末多出一根）、take 中阶 全部、take material_yaodan_wuji 全部、advance_days(720)、give material_lianjian_fuliao 1、置 ch11.jinlei | once=false；spend 不成 → 「等几日」return；辅料**必给**，不看扣到多少 |
| n27 kaidian | 李氏店主、曲魂 | take 低阶 50（有几扣几）；flag.set("ch11.kaidian_ri", today())；flag.set("ch11.jiesuan_ri", today())；置 ch11.kaidian | 店门 portal require_flag ch11.kaidian；何仙师、邻店主 visible |
| n28 jiesuan | 韩立、（首次）何仙师 | 顶格先判 ch11.liugen ≥ 1 → 一句「店已盘出」return；n = (today() − ch11.jiesuan_ri) / 30，上限 240；give 低阶 15 × n；ch11.jiesuan_ri = today()；ch11.jiesuan += 1 | once=false；n == 0 → 一句 return；首次附旁白、第二次起只结算（E22）|
| 柜台 guitai | 韩立 | 顶格先判 ch11.liugen ≥ 1 → 一句「店已盘出」return；shop("ch11_qingzhu_xiaoxuan") | once=false；不置旗；店门照开（portal 不读 hidden_flag）|
| n29 peizhu（后半，ch11.jinlei 后）| 韩立 | bottle.spend(1) → advance_days(1080)、ch11.zhu_gen += 1、give material_jinleizhu 1；zhu_gen == 6 → advance_days(max(360, 7200 − (today() − kaidian_ri)))（超 3650 分两句）、give 中阶 20、置 ch11.liugen | spend 不成 → return；补足写法照 `sanzhuan.lua`；zhu_gen 从 1 起（n26 置），本支跑第 2–6 根共 5 轮；收店后柜台与结算只说一句、何仙师与邻店主 hidden_flag ch11.liugen |
| n30a lianjian | 韩立 | advance_days(120)；置 ch11.lianjian_qian | enter、once；主旗 ch11.lianjian 由 n30b 置；面板由 facility_lianqi_39 开（require_flag ch11.liugen），配方 recipe_qingzhu_fengyun_jian `guaranteed true` 必成（契约 E1、E16）|
| n30b chujian | 韩立、曲魂 | advance_days(60)；置 ch11.lianjian | enter；门闸 item.count("weapon_qingzhu_fengyun_jian") ≥ 1，否则 return |
| n31 zhuzhong | 金青、曲魂 | give story_bai_zhenzhu 1；置 ch11.done | 不 ending()；不写去向；谣言只到谣言 |
| Z1 shougou_dingdan | 范小子 | 接单 → ch11.z1 = 1；交付 take material_yaodan_wuji 3 → give 中阶 10、material_lianqi_yao 2、ch11.z1 = 2 | accept ch11.ningcui；fail ch11.huicheng；`origin` 改编 |
| Z2 linju_z2 | 结丹邻居乙 | take pill_dingyan_dan 1 → give material_lianqi_yao 3；ch11.z2 = 1 | 没丹 → 一句 return；accept ch11.zhaolan |
| 回城 huicheng_chuansong | 守阵修士 | teleport("ch11_xingkong") | enter、once=false、guard ch11.chuansong；**0 处 flag.set**（kScriptTransfers 五元行）；不扣钱（回程费在 n09 一并算，E14）|

- 只使用既有脚本接口（`ScriptHost.cpp` 命令清单：advance_days、battle、bottle_*、choice、fade_*、field_*、flag_get、give_item、item_count、realm_*、set_flag、shop、take_item、talk、teleport），无新增命令；`teleport` 只传图 id（落 default spawn，不认 spawn 名）；不读取或模拟 formerRealm，不从脚本发修为，本章 `realm.demote` 0 处、`realm.cap` 0 处、`party.remove` 0 处。
- 所有扣物、升境、滴液、开田都核返回值。普通资源不足的内容支路照常可走：**本章唯一的主线门闸是 n14 的 `ch11.lieyao ≥ 3`（打赢三场 ①）与 n30b 的「剑在身上」**，两处都不花钱——三场 ① 要的三株草 n08 必给、绿液只拨日；配方不要灵石。
- 妖丹只从战斗 ① ② 掉落（编成 `drops`）与 n14 必给；遭遇表 0 张（E15）。
- 六次 `advance_days(3600)`：`kMaxScriptAdvanceDays = 3650`（`Application.h` 第 49 行），单次不越上限、不改引擎（E5）；`playSecondsGameplay` 只按真实秒累计，跳过的岁月天然不计游玩时长。

### 3.3 日历

**本节是相对日历 spec，不是通关实测。** 起点 `day` 16003 / 15978 已实读（第 2 节）；本表从第 10 章终局起累计，诱妖轮数与绿液等待按意图玩家算；不许写入 fixture 或拿 bot 结果补齐日期。

| 节点 | `advance_days` | 累计（从第 10 章终局起）|
| --- | --- | --- |
| n01–n05 | 0 | 0（一月赶路只在旁白，不拨日）|
| n06 半年育虫；两年傀儡 | 180 ＋ 720 | 900 |
| n08 试草（缺绿液 ＋5）；又过半年 | （5）＋ 180 | 1080 |
| n09–n11 | 0 | 1080 |
| n12 诱妖 ×5（含 ② 一次；意图玩家 4 胜 ＋ 1 逃）| 各 3（缺绿液 ＋5）| 1095 |
| n14 五年换岛 | 1800 | 2895 |
| n15 两年炼丹；散功半月 | 720 ＋ 15 | 3630 |
| n16 生死关六十年（三段）| 3600 × 6 | **25230** |
| n17–n20 | 0 | 25230 |
| n21 半月赶制傀儡 | 15 | 25245 |
| n22、n23 | 0 | 25245 |
| n24 入地一月 | 30 | 25275 |
| n25 | 0 | 25275 |
| n26 三滴（结丹后 3 日一滴，瓶容量已抬到 9、滴数够用，缺则各 ＋5）；两年凑辅料 | 720 | 25995 |
| n27 盘店（记 `ch11.kaidian_ri`）| 0 | 25995 |
| n28 回访（不拨日）| 0 | — |
| n29 第 2–6 根各三年（第一根在 n26 催成、不拨日）| 1080 × 5 | 31395 |
| n29 第六根补足到开店起 7200 日（至少 360）| 7200 − 5400 ＝ 1800 | **25995 ＋ 7200 ＝ 33195** |
| n30a 打坐三日、炼液三月、布阵半月、择日 | 120 | 33315 |
| n30b 两月出关 | 60 | 33375 |
| n31 | 0 | **33375** |

原著的锚：一个多月（ch389）、七天（ch390）、半年 / 两年整（ch392）、一个月（ch395）、两三日 / 一个多月（ch396）、五年 / 整整两年 / 半月（ch397）、六十年（ch397–398）、四年 / 五年多（ch398）、两月前（ch402）、半月（ch402）、次日清晨（ch403）、一个月（ch409）、两年 / 数月（ch411–412）、第九个年头 / 二十余年（ch413）、三个月 / 半月 / 两日 / 一天一夜（ch414）、两个月（ch415）。
**二十余年的算法**：开店（n27）到收店（n29 第六根）恰 7200 日（二十年）——玩家回访结算几次、培竹之间隔多久，都由第六根补齐，不叠加（**不许写成「多等一根就晚收店三年」**）。全章意图玩家约 **33375 日**（约九十二年半），其中六十年在 n16 一处；本章**没有倒计时**，横幅照剧情写。
**3.2 里没写 `advance_days` 的挂点，按本表拨**（本表是 `advance_days` 的唯一出处）。回访结算 n28 按 `ch11.jiesuan_ri` 到 `today()` 的差算，本身不拨日。

### 3.4 目标链（`data/objectives/ch11.json`）

**29 步**（已拍板：原 F 表 31 步里删去 n13 鸡冠蛟与 n28 回访结算两步——`ch11.jiguan` 不是任何节点的 guard、玩家三胜就能收手；`ch11.jiesuan` 则有死链：玩家可不回柜台先培竹六根，`liugen` 后店已盘出、`jiesuan` 永远置不上，`ObjectiveRoute` 沿链走门会卡死。两旗登 `kOffChainFlags`，与 `tests/ObjectiveTests.cpp` 第 884–888 行「已是目标链 done_flag 不该再登进」互斥的只能二选一）。整数旗标 `ch11.lieyao` 在链上按「≥ 1」判完成。

| # | step id | `done_flag` | `target_map` | `target_object` | R-3 提示 |
| --- | --- | --- | --- | --- | --- |
| 1 | `n01_chudao` | `ch11.chudao` | `ch11_fangshi` | `trigger_chudao` | |
| 2 | `n02_zhihuan` | `ch11.zhihuan` | `ch11_fangshi` | `npc_baiyi_xiushi` | **「进城先领指环——红环要八块中阶灵石，没有就领蓝环」** |
| 3 | `n03_luer` | `ch11.luer` | `ch11_fangshi` | `npc_lu_er` | |
| 4 | `n04_zufu` | `ch11.zufu` | `ch11_fangshi` | `npc_xinggong_zhishi` | **「山脚的楼阁管洞府租赁；三十九层那四座，百年一付」** |
| 5 | `n05_anju` | `ch11.anju` | `ch11_dongfu` | `trigger_anju` | |
| 6 | `n06_kuilei` | `ch11.kuilei` | `ch11_dongfu` | `trigger_kuilei` | |
| 7 | `n07_ziliao` | `ch11.ziliao` | `ch11_fangshi` | `npc_baimen_zhanggui` | **「要猎妖，先去白门阁把妖兽资料买齐」** |
| 8 | `n08_nichang` | `ch11.nichang` | `ch11_dongfu` | `trigger_nichang` | |
| 9 | `n09_xingkong` | `ch11.chuansong` | `ch11_xingkong` | `npc_xingkong_xiushi` | **「坊市杂货开始卖霓裳草了；星空殿在洞府北面——传送阵按次收钱，拼阵便宜」** |
| 10 | `n10_ningcui` | `ch11.ningcui` | `ch11_ningcui_jie` | `trigger_ningcui` | |
| 11 | `n11_buzhen` | `ch11.buzhen` | `ch11_honghu` | `trigger_buzhen` | |
| 12 | `n12_youyao` | `ch11.lieyao` | `ch11_honghu` | `trigger_youyao` | **「到阵心催草诱妖——每次一株霓裳草、一滴绿液；妖丹拿回凝翠岛收购行换灵石」**（鸡冠蛟 n13 是这一步里的分支，不单列；提示里的「六级的东西」口径见 12.2）|
| 13 | `n14_huicheng` | `ch11.huicheng` | `ch11_honghu` | `trigger_huicheng` | **「猎够三头五级再收手；回城之后本章不再出海」** |
| 14 | `n15_liandan` | `ch11.sangong` | `ch11_dongfu` | `trigger_liandan` | |
| 15 | `n16_biguan` | `ch11.jiedan` | `ch11_dongfu` | `trigger_biguan` | **「进密室之前，降尘丹与雪灵水都在身上」** |
| 16 | `n17_chuguan` | `ch11.chuguan` | `ch11_dongfu` | `trigger_chuguan` | |
| 17 | `n18_zhaolan` | `ch11.zhaolan` | `ch11_dongfu` | `trigger_zhaolan` | |
| 18 | `n19_lianer` | `ch11.lianer` | `ch11_fangshi` | `npc_lian_er` | |
| 19 | `n20_chaguan` | `ch11.chaguan` | `ch11_fangshi` | `npc_fan_furen` | **「清河茶馆的雅间——把那包妖兽材料带上」** |
| 20 | `n21_zhuzhan` | `ch11.zhuzhan` | `ch11_dongfu` | `trigger_zhuzhan` | **「出发前存个档——礁岛上没有退路」** |
| 21 | `n22_guiju` | `ch11.guiju` | `ch11_yinsha_dao` | `trigger_guiju` | |
| 22 | `n23_wuchou` | `ch11.jiyin` | `ch11_yinsha_dao` | `trigger_wuchou` | |
| 23 | `n24_daoge` | `ch11.yaoshi` | `ch11_yinsha_dao` | `trigger_daoge` | **「那东西打不死——让蜘蛛与傀儡困住它」** |
| 24 | `n25_zhanglao` | `ch11.zhanglao` | `ch11_fangshi` | `npc_zi_ling` | |
| 25 | `n26_peizhu` | `ch11.jinlei` | `ch11_dongfu` | `trigger_peizhu` | **「药园边上一滴一滴催：白、蓝、金，三滴」** |
| 26 | `n27_kaidian` | `ch11.kaidian` | `ch11_fangshi` | `npc_li_dianzhu` | **「店开着就有进项——隔些日子回柜台结一结」**（回访结算 n28 不是链上的一步，提示挂在这里）|
| 27 | `n29_liugen` | `ch11.liugen` | `ch11_dongfu` | `trigger_peizhu` | **「六根金雷竹，一根三年」** |
| 28 | `n30_lianjian` | `ch11.lianjian` | `ch11_dongfu` | `facility_lianqi_39` | **「密室炼器台：青竹蜂云剑——材料齐了就不会失手」** |
| 29 | `n31_zhuzhong` | `ch11.done` | `ch11_dongfu` | `npc_jin_qing` | |

- 文案进 `data/text/ch11_objectives.json`。step id 保留节点号（`n14_huicheng` 是第 13 步），免得与 3.1/3.2 的 n 编号错位。n13、n28 仍是节点（3.1、3.2 照演），只是不进链；两旗 `ch11.jiguan`、`ch11.jiesuan` 登 `kOffChainFlags`（3.1 末）。不为这两步加引擎。
- `ObjectiveRoute` 沿这条链逐步走门：坊市 ↔ 洞府一对门（上行 `portal_shangshan` 带 `require_flag ch11.zufu`）、洞府 ↔ 星空殿一对门（上行 `portal_xingkong` 带 `require_flag ch11.nichang`，下行 `portal_xiashan_50`）、凝翠岛 ↔ 红瑚岛一对门、坊市 ↔ 青竹小轩一对门；星空殿 → 凝翠岛、凝翠岛石屋 → 星空殿、洞府 → 礁岛、礁岛 → 洞府都是脚本 teleport（3.1 末表），礁岛没有门（规则 29 对它无要求）。
- 第 12 步的提示在 `ch11.huicheng` 之后不再显示（链已过）；第 9 步在 `ch11.huicheng ≥ 1` 后若玩家再去星空殿，脚本只说一句「本章不再出海」（G 表附注 2），目标链不回退。
- **本章新增的按键提示：0 条**（诱妖、结算、炼剑面板都用既有的键，不点名物理键）；文案若点名可改键，一律 `{key.<id>}` ＋ `.pad` 版、登 KeyPrompts 白名单（13 节第 19 条）。
- 第 1、5、11、16 等步不写 R-3 提示：它们是 `enter` 挂点，走到就演。

---

<!-- 第 11 章施工图·第二部分（第 4–9 节）。事实来源只有 notes/ch11/research.md；id、旗标、章号、数字照抄，表号（A/C/E/F/G/H/I/K/L/N/O）指该笔记。节号与节名照第 10 章施工图。 -->

## 4. 地图（新建 7 张，旧图 0 处修补）

主题 / 时辰 / 背景只用 `data/visual/maps.json` 已有组合（town/day/town_street、indoor_stone/indoor/secret_room、dock/day/dock_river、cliff/day/cliff_top、wild/day/cliff_top、indoor_wood/indoor/inn_hall）；`region` 一律 `luanxinghai_zhongbu`（新字符串，只是标签）；`chapter=11`；`display_name_key = ch11.map.<短名>.name`。出入口照规则 29「出门往哪走，进门就朝哪」，spawn 名在括号里。尺寸量级参照第 10 章（魁星岛 60×48、小寰岛 36×28）。

| 地图 id | 名称 | 主题 / 时辰 / 背景 | 尺寸 · 分区 | 出入口 | 挂点与设施 | 在场 NPC（出场 → 退场旗标） |
| --- | --- | --- | --- | --- | --- | --- |
| `ch11_fangshi` | 天星城坊市（圣山第一层） | town / day / town_street；粒子 dust | 56×44，三区：南缘城门、中部坊市街、北缘山脚楼阁；东北角茶馆雅间（室内 zone，地面 `wood_floor`） | **北**口 `portal_shangshan` → `ch11_dongfu`（spawn `men`，朝上）`require_flag ch11.zufu`、`deny_text_key ch11.fangshi.shangshan_deny`（租府之前上不去）；青竹小轩门（街西，朝上进店）→ `ch11_xiaoxuan`（spawn `dian`）`require_flag ch11.kaidian`（**不写 hidden_flag**，portal 不读它；收店后店门照开，柜台与结算脚本只说一句）；茶馆门（朝右进）`require_flag ch11.lianer`；城门南缘不可出（`can_leave_edge false`） | `trigger_chudao`（城门 spawn `chengmen`〔default〕，enter）；`facility_zahuo`（shop，`ref_id ch11_fangshi_zahuo`，`require_flag ch11.nichang`）；`facility_xuanshang`（board）；`facility_cunji_fangshi`（save）；茶馆两场都挂在 npc 上（n20、n25） | `npc_baiyi_xiushi`（ch11.chudao → ch11.zhihuan）；`npc_lu_er`（ch11.zhihuan → ch11.anju）；`npc_xinggong_zhishi`（山脚楼阁，ch11.luer → ch11.zufu）；`npc_baimen_zhanggui`（ch11.kuilei → ch11.done）；`npc_fengle_menmian`（ch11.kuilei → ch11.done）；`npc_chuwu_zhanggui`（ch11.kuilei → ch11.huicheng）；`npc_lian_er`（ch11.zhaolan → ch11.lianer）；`npc_fan_furen`（雅间，**ch11.lianer → ch11.zhanglao，一个对象顶 n20 与 n25 两场**——同图两个 npc 顶同一 role、旗标不互补会被规则 30 报，`validate.py` 第 3175–3210 行；`chaguan.lua` 开头：`ch11.chaguan` 已置且 `ch11.yaoshi` 未置 → 一句 return，`ch11.yaoshi` 已置 → 一句「请紫灵姑娘说」return）、`npc_zhao_zhanglao`（雅间，ch11.lianer → ch11.chaguan）；`npc_zi_ling`（role `zi_ling_zhenrong`）、`npc_zhuo_ruting`（雅间，ch11.yaoshi → ch11.zhanglao）；`npc_li_dianzhu`（ch11.jinlei → ch11.kaidian）；`npc_he_xianshi`（街，ch11.kaidian → ch11.liugen） |
| `ch11_dongfu` | 圣山第三十九层洞府 | indoor_stone / indoor / secret_room | 34×26：门厅、密室、丹房、虫室、药园（`tilled` 6 槽）、药园一角珊瑚礁 | **南**口 `portal_xiashan` → `ch11_fangshi`（spawn `shanjiao`，朝下）；**北**口 `portal_xingkong` → `ch11_xingkong`（spawn `dian`，朝上）`require_flag ch11.nichang`、`deny_text_key ch11.dongfu.xingkong_deny` | spawn `men`〔default〕；`facility_cunji_39`（save，**紧挨 spawn `men`**，在 n21 触发格之前）；`trigger_anju`（门内一格，enter）；`facility_jingshi_39`（meditate，`effectiveness 125`，`require_flag ch11.anju`）；`facility_danfang_39`（alchemy，grade 3，`require_flag ch11.anju`）；`facility_lianqi_39`（forge，grade 3，`require_flag ch11.liugen`）；`facility_yaoyuan_39`（field，**`ref_id`** `field_tianxing_39`——引擎读的键是 `ref_id`（`Application.cpp` 第 703 行），写 `ref` 面板不开，`require_flag ch11.anju`）；`trigger_kuilei`、`trigger_liandan`（丹房）、`trigger_biguan`（密室）、`trigger_chuguan`（密室门外）、`trigger_zhaolan`（门禁制，enter）、`trigger_zhuzhan`（门内一格，**interact**，与 spawn、n18 都不同格）、`trigger_nichang`（药园旁）、`trigger_peizhu`（珊瑚礁旁，n26/n29 同一触发器）、`trigger_lianjian`（炼器台同格）、`trigger_chujian`（密室门外） | `npc_jin_qing`（门外，ch11.lianjian → ch11.done）；`npc_jiedan_linju_a` / `_b`（门外小径，ch11.zhaolan → ch11.done） |
| `ch11_xingkong` | 星空殿（第五十层） | indoor_stone / indoor / secret_room | 24×18：大理石殿、成排传送阵 | **南**口 `portal_xiashan_50` → `ch11_dongfu`（spawn `beimen`，朝下） | spawn `dian`〔default〕；`npc_xingkong_xiushi` 挂 n09 脚本（传送阵就是他，`once` 否）；`facility_cunji_50`（save） | `npc_xingkong_xiushi`（ch11.nichang → ch11.done）；`npc_yaoyan_furen`、`npc_tongxing_xiushi`（**一个对象**，名牌「同行修士」，台词里说「两位」——同图两个对象顶同一 role 会被规则 30 报）、`npc_zaohong_laozhe`（ch11.nichang → ch11.chuansong） |
| `ch11_ningcui_jie` | 凝翠岛小街 | dock / day / dock_river；粒子 mist | 30×20：西端传送石屋、一条街四间铺、东端海岸 | **东**口 `portal_chuhai` → `ch11_honghu`（spawn `xi`，朝右）`require_flag ch11.ningcui`；石屋传送阵 `trigger_huicheng_chuansong`（**enter**，`once` 否，`guard_flag ch11.chuansong`，无 `set_flag`）→ `teleport("ch11_xingkong")`（落 default `dian`；脚本 0 处 `flag.set`，kScriptTransfers 五元行） | spawn `shiwu`〔default〕；`trigger_ningcui`（石屋门外，enter）；`facility_zahuo_nc`（shop `ch11_ningcui_zahuo`）；`facility_shougou`（shop `ch11_ningcui_shougou`）；`facility_cunji_nc`（save）；Z1 挂在 `npc_fan_xiaozi` 的 interact 脚本 | `npc_ningcui_shouzhen`（石屋，ch11.chuansong → ch11.done）；`npc_zahuo_nc`、`npc_fan_xiaozi`（ch11.chuansong → ch11.huicheng）；`npc_hao_yuantian`（ch11.ningcui → ch11.huicheng） |
| `ch11_honghu` | 红瑚岛 | cliff / day / cliff_top；粒子 mist | 36×30：环形珊瑚礁、三处必经路（三座大阵只在旁白）、环心孤礁 | **西**口 `portal_huijie` → `ch11_ningcui_jie`（spawn `dong`，朝左） | spawn `xi`；`trigger_buzhen`（登岸格，enter）；`trigger_huicheng`（同格，interact，`guard ch11.buzhen`；O-7）；`trigger_youyao`（环心，interact，`once` 否）；`facility_cunji_hh`（save，在登岸格旁） | 无具名 NPC（海兽与千目妖只在编成里） |
| `ch11_yinsha_dao` | 隐煞门礁岛 | wild / day / cliff_top | 28×22：礁石小岛、残阵 | **无门**（只进不出：n21 teleport 进、n24 teleport 出） | spawn `haishang`〔default〕；`trigger_guiju`、`trigger_wuchou`、`trigger_daoge`（同格三连，enter，按旗标换） | `npc_zi_ling`（role `zi_ling`，名牌「紫灵仙子」）、`npc_fan_furen_2`、`npc_zhuo_ruting_2`、`npc_zhao_zhanglao_2`、`npc_fu_zhanglao`、`npc_meng_daoyou`、`npc_chihuo_laoguai`（ch11.zhuzhan → ch11.yaoshi）；`npc_yinsha_menzhu`（ch11.guiju → ch11.yaoshi）；`npc_wu_chou`（**ch11.guiju → ch11.jiyin**）→ `npc_jiyin_fushen`（ch11.jiyin → ch11.yaoshi）同格换 NPC（规则 30「轮流在场」：前者的 hidden 恰是后者的 visible，一具身体不许两块名牌并立） |
| `ch11_xiaoxuan` | 青竹小轩 | indoor_wood / indoor / inn_hall | 20×14：铺面、柜台、后宅 | **南**口 `portal_chudian` → `ch11_fangshi`（spawn `xiaoxuan_men`，朝下） | spawn `dian`；`trigger_jiesuan`（柜台，interact，`once` 否；脚本开头判 `ch11.liugen` 则一句 return）；`trigger_guitai`（柜台另一格，interact，`once` 否，`guitai.lua`：判 `ch11.liugen` 则一句 return，否则 `shop("ch11_qingzhu_xiaoxuan")`——不做 shop facility，facility 不读 `hidden_flag`、收店后关不掉；O-6）；`facility_cunji_xx`（save） | `npc_he_xianshi_2`、`npc_linju_dianzhu`（ch11.kaidian → ch11.liugen） |

- **门的拓扑**：坊市 ↔ 洞府一对门（上行 `require_flag ch11.zufu`——租府之前不许上三十九层，与「洞府要租、百年一付」名实相符）；洞府 ↔ 星空殿一对门，上行 `require_flag ch11.nichang`（悟出霓裳草之前没有出海的理由）；凝翠岛 ↔ 红瑚岛一对门；青竹小轩一对门。星空殿传送阵与凝翠岛石屋是脚本 teleport（登 N.4 `kScriptTransfers`）；隐煞门礁岛没有门，规则 29 对它无要求。
- **为什么坊市是一张图、三区**：城门、坊市街、山脚楼阁与茶馆雅间在原著里相隔不远（ch390–391 一日之内走完），拆开要多两张图、超出 7 张预算（E7）；合成一张照第 10 章「魁星岛一张图四个区」。**区与区之间不设门闸**：茶馆雅间的门只在 `ch11.lianer` 后开，青竹小轩的门只在 `ch11.kaidian` 后开——其余时候那两扇门的 `deny` 文案是一句「门关着」。
- **洞府设施的门闸**：静室、丹房、药园 `require_flag ch11.anju`（n05 安顿之后）；炼器台 `require_flag ch11.liugen`（六根金雷竹齐之后，E16）——四处都是现成的设施 `require_flag`（`Application::openFacility` 已判，未置时说 `ui.facility.locked`）。
- **为什么出海的两张图在 `ch11.huicheng` 之后不再可去**：原著五年收手之后他再没去过凝翠岛（ch397 起闭府）；星空殿 n09 脚本在 `ch11.huicheng ≥ 1` 时只说一句「本章不再出海」并 `return`，不设回程门（同第 10 章 16.1 第 21 条口径）。
- **每一处挂战斗的格子前面都有存档点**：① ② 红瑚岛登岸格旁（`facility_cunji_hh`，环心触发器走两步就到）；③ ④ 隐煞门礁岛——**没有存档点**（只进不出、同格三连，中间玩家有控制权但不能离岛）：③ 之前的存档在 n21 之前的洞府（`facility_cunji_39`，紧挨 spawn `men`；n21 是 **interact**、不与 spawn 同格，所以从坊市上山一定先经过存档点才碰得到触发格），③ 与 ④ 之间 n23 的演出不可跳过——这是本章唯一「连打两场」的地方，④ 的 `defeat_is_fatal true` 要让平衡路按「刚打完 ③ 不回血」量。
- **命名与地图属性**：`bgm`：坊市 `bgm_town`、洞府 `bgm_valley`、星空殿 `bgm_valley`、凝翠岛 `bgm_river`、红瑚岛 `bgm_cliff`、隐煞门礁岛 `bgm_wild`、青竹小轩 `bgm_town`（**全是已有的曲子**；`tools/audiogen` 的 `MAP_BGM` 登记表跟着加 7 条、`docs/audio.md` 同一张表）；脚本点播一处（n23 极阴附身那一段 `bgm_night`，演完 `bgm("map")`）。
- **主题一律用现有的**（E7）：城用 `town`，洞府与星空殿用 `indoor_stone`（大理石殿靠 ambient 提亮），凝翠岛小街用 `dock`，红瑚岛与礁岛用 `cliff`/`wild` ＋ `mist`，店铺用 `indoor_wood`。**不加新主题、不新做战斗背景**（第 8 节四场都用 `cliff_top`）。
- `tools/mapgen/genmaps_ch11.py`（7 张）接进 `genmaps.py --check`；**本章不 patch 任何旧图**（E1 的开局接法是 patch 脚本 `scripts/ch10/zhifadui.lua`，不是地图）；美术 `tools/artgen/artgen.py` 烘焙 7 张新图并 `--check` 过。
- **spawn 与落点清单**（地图路与脚本路对表；规则 29 的朝向写在括号里）。**脚本 `teleport` 只传图 id**（`api.lua` 第 63 行 `teleport(map_id, x, y)`，x/y 是整数坐标；引擎 `Application.cpp` 第 1005 行 `loadMap(command.a, "")` 恒落 default spawn，传 `"chengmen"` 这种字符串会被 `ScriptHost.cpp` 第 178 行 `get_or("x", 0)` 当 0 或抛错）：每张图**脚本落点的那个 spawn 标 `default=True`**（`genmaps.py` 第 163 行 `spawn(..., default=False)`），门走 portal 的 `target_spawn`。按名落点要加引擎（Teleport 命令加 b 字段 → `loadMap(a, b)`），本章不要。

| 地图 | spawn | default | 谁用它 |
| --- | --- | --- | --- |
| `ch11_fangshi` | `chengmen`（城门，朝上） | **✓** | `scripts/ch10/zhifadui.lua` 末尾的 `teleport("ch11_fangshi")`（C1）；章节卡在这一格上屏 |
| `ch11_fangshi` | `shanjiao`（北口内，朝下） | | 从洞府下山（portal `target_spawn`） |
| `ch11_fangshi` | `xiaoxuan_men`（店门外，朝下） | | 从青竹小轩出来 |
| `ch11_dongfu` | `men`（南口内，朝上） | **✓** | 从坊市上山（portal）；n04 `zufu.lua`、n14 `huicheng.lua`、n24 `daoge.lua` 三处 `teleport("ch11_dongfu")` 落点；`facility_cunji_39` 紧挨它、n21 触发格在存档点之后 |
| `ch11_dongfu` | `beimen`（北口内，朝下） | | 从星空殿下来 |
| `ch11_xingkong` | `dian`（殿内，朝上） | **✓** | 从洞府北口上（portal）；凝翠岛石屋 `huicheng_chuansong.lua` 的 `teleport("ch11_xingkong")` 落点 |
| `ch11_ningcui_jie` | `shiwu`（石屋门外，不在传送阵格上） | **✓** | n09 `xingkong.lua` 的 `teleport("ch11_ningcui_jie")` 落点（每次出海都落这里） |
| `ch11_ningcui_jie` | `dong`（东口内，朝左） | | 从红瑚岛回来 |
| `ch11_honghu` | `xi`（登岸格，朝右） | ✓（唯一） | 从凝翠岛过来；`trigger_buzhen` / `trigger_huicheng` 就挂在这一格 |
| `ch11_yinsha_dao` | `haishang`（礁岛边缘） | **✓** | n21 `zhuzhan.lua` 的 `teleport("ch11_yinsha_dao")` 落点；三连触发器挂在它前面一格 |
| `ch11_xiaoxuan` | `dian`（铺面，朝上） | ✓（唯一） | 从坊市进店 |

- **只在挂点脚本里说话、不摆 NPC 的**：鲁二以外的凡人店主（E24）、白门阁以外的书铺伙计、传送符、汪门主、封某（鹰钩鼻修士，E25）、隐煞门其余弟子、天星双圣与星宫长老（E6）。

---

## 5. 角色

名牌按首见章解禁（M 表）。**新建** = `data/roles/<id>.json` 新文件 ＋ `data/visual/looks.json` 外观（人形用 archetype；妖兽与变身态在 `tools/artgen/sprites_enemies.py` 画，`sprite_preview.py --coverage` 缺图为 0）。非战斗 NPC 的 realm 只供名牌与天眼术，数值照第 10 章路人写法。**非人形四个**：千目妖（滚圆带刺、数十只绿眼）、鸡冠蛟、妖尸、血玉蜘蛛（友军）——待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下，第 8 章同口径；煞妖的变身态也进 `sprites_enemies.py`。

| role id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 战斗 | 出场旗标 → 退场旗标 | 在哪 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `hanli` | 韩立 | — | 存档 | 不动 | 是 | — | — |
| `qu_hun_shadan` | 曲魂（身外化身） | — | 结丹初期 | **不动**（E18） | 是（①②③④ 队友） | 全章在队，不 `party.remove` | 队伍 |
| `baiyi_xiushi` | 星宫修士 | ch390 | 筑基初期 | 新建（arch_disciple） | 否 | ch11.chudao → ch11.zhihuan | fangshi |
| `lu_er` | 鲁二 | ch390 | 凡人 | 新建（arch_townsman） | 否 | ch11.zhihuan → ch11.anju | fangshi |
| `xinggong_zhishi` | 星宫执事 | ch391 | 筑基中期 | 新建（arch_disciple） | 否 | ch11.luer → ch11.zufu | fangshi |
| `baimen_zhanggui` | 白门阁掌柜 | ch393 | 炼气九层 | 新建（arch_merchant；改编名） | 否 | ch11.kuilei → ch11.done | fangshi |
| `fengle_menmian` | 丰乐拍卖行伙计 | ch393 | 炼气七层 | 新建（arch_merchant；改编） | 否 | ch11.kuilei → ch11.done | fangshi |
| `chuwu_zhanggui` | 储物袋铺掌柜 | ch393 | 炼气十层 | 新建（arch_merchant；改编） | 否 | ch11.kuilei → ch11.huicheng | fangshi |
| `xingkong_xiushi` | 星空殿执事 | ch393 | 结丹初期 | 新建（arch_cultivator_m） | 否 | ch11.nichang → ch11.done | xingkong |
| `yaoyan_furen` | 妖艳妇人 | ch393 | 筑基后期 | 新建（arch_cultivator_f） | 否 | ch11.nichang → ch11.chuansong | xingkong |
| `tongxing_xiushi` | 同行修士 | ch393 | 结丹初期 | 新建（arch_cultivator_m；**一个对象** `npc_tongxing_xiushi`，台词说「两位」——同图两对象同 role 会被规则 30 报） | 否 | ch11.nichang → ch11.chuansong | xingkong |
| `zaohong_laozhe` | 枣红脸老者 | ch393 | 结丹初期 | 新建（arch_elder_m） | 否 | ch11.nichang → ch11.chuansong | xingkong |
| `ningcui_shouzhen` | 守阵修士 | ch393 | 筑基中期 | 新建（arch_disciple） | 否 | ch11.chuansong → ch11.done | ningcui_jie |
| `zahuo_nc` | 杂货铺掌柜 | ch394 | 炼气八层 | 新建（arch_merchant） | 否 | ch11.chuansong → ch11.huicheng | ningcui_jie |
| `fan_xiaozi` | 范小子 | ch394 | 炼气六层 | 新建（arch_merchant） | 否 | ch11.chuansong → ch11.huicheng | ningcui_jie |
| `hao_yuantian` | 郝远天 | ch394 | 结丹初期 | 新建（arch_cultivator_m） | 否 | ch11.ningcui → ch11.huicheng | ningcui_jie |
| `qianmu_yao` | 千目妖 | ch396 | 五级（CoreEarly） | 新建（妖兽，非人形） | 是（① 首领） | 编成 | 战斗 ① |
| `siji_haishou` | 四级海兽 | — | FoundationMid | **就地改写** `weaknesses`（见下表） | 是（① 第二波） | 编成 | 战斗 ① |
| `jiguan_jiao` | 鸡冠蛟 | ch396 | 六级（CoreMid） | 新建（妖兽，非人形） | 是（② 首领） | 编成 | 战斗 ② |
| `lian_er` | 莲儿 | ch399 | 炼气三层 | 新建（arch_maid） | 否 | ch11.zhaolan → ch11.lianer | fangshi |
| `fan_furen` | 范夫人 | ch399（范静梅）/ ch400 | 筑基后期 | **新建**（总表有、文件无；arch_noblewoman） | 否 | 雅间 `npc_fan_furen` **ch11.lianer → ch11.zhanglao**（一个对象顶 n20、n25 两场，中段由 `chaguan.lua` 开头一句打发——同图两对象同 role 且旗标不互补会被规则 30 报）；礁岛 `_2` ch11.zhuzhan → ch11.yaoshi | fangshi、yinsha_dao |
| `zhao_zhanglao` | 赵长老 | ch400 | 结丹初期 | 新建（arch_elder_m） | 否 | 雅间 ch11.lianer → ch11.chaguan；礁岛 `_2` ch11.zhuzhan → ch11.yaoshi | fangshi、yinsha_dao |
| `zi_ling` | 紫灵仙子 | ch403 | 结丹初期（已有） | **只用名牌、不上场、不改数值**（它是第 12 章 `b12_xutian_hudian_kuilei` 的） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `zi_ling_zhenrong` | 紫灵 | ch409 | 筑基后期 | 新建（arch_cultivator_f；真容，数值同 `zi_ling` 口径） | 否 | ch11.yaoshi → ch11.zhanglao | fangshi |
| `zhuo_ruting` | 卓如婷 | ch402 | 筑基后期 | 新建（arch_cultivator_f） | 否 | 礁岛 ch11.zhuzhan → ch11.yaoshi；雅间 ch11.yaoshi → ch11.zhanglao | yinsha_dao、fangshi |
| `fu_zhanglao` | 符长老 | ch404（紫灵点名）/ ch405（称「中年儒生」） | 结丹初期 | 新建（arch_cultivator_m；原著「儒生长老」，B6） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `meng_daoyou` | 孟道友 | ch402（「姓孟的结丹期修士」）/ ch403（「孟某」） | 结丹初期 | 新建（arch_cultivator_m；不叛，B6；**名牌「孟道友」是改编称谓**——原著区间只有「姓孟的」「孟某」，「孟道友」三字全本首见 ch1169，`origin` 标改编） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `chihuo_laoguai` | 赤火老怪 | ch403 | 结丹后期 | 新建（arch_demonic_m） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `yinsha_menzhu` | 隐煞门门主 | ch404 | 结丹后期 | 新建（arch_demonic_m；名牌不写姓） | 否 | ch11.guiju → ch11.yaoshi | yinsha_dao |
| `yinsha_shayao` | 隐煞门煞妖 | ch404 | 筑基后期 | 新建（arch_demonic_m，变身态用 enemies 画） | 是（③ ×4） | 编成 | 战斗 ③ |
| `wu_chou` | 乌丑 | ch382 | 结丹中期 | **一个字不动**；只说话（O-2） | 否 | ch11.guiju → **ch11.jiyin**（附身起换名牌；hidden 恰等于 `jiyin_fushen` 的 visible，规则 30「轮流在场」） | yinsha_dao |
| `jiyin_fushen` | 极阴祖师 | ch382（名号）/ ch405 附身（原文称「极阴老祖」「极阴老魔」）/ ch406 自称「本祖师」 | realm 写 CoreLate（O-9） | 新建（arch_demonic_m；只说话；n23 台词可用「极阴老祖」） | 否 | ch11.jiyin → ch11.yaoshi | yinsha_dao |
| `tiandu_shi` | 妖尸 | ch408 | CoreEarly | 新建（妖尸，非人形） | 是（④ 首领） | 编成 | 战斗 ④ |
| `xueyu_zhizhu_ally` | 血玉蜘蛛 | ch345 | 四级顶峰（FoundationLate） | 新建（友军，非人形） | 是（④ 友军 ×2） | 编成 | 战斗 ④ |
| `kuilei_yuan` | 猿形傀儡 | ch399 / ch408 | 筑基中期 | 新建（arch_puppet） | 是（④ 友军 ×2；H 表「在哪」写 ③④、I 表 ③ 无友军——以 I 表编成为准） | 编成 | 战斗 ④ |
| `li_dianzhu` | 李氏杂货店主 | ch412 | 凡人 | 新建（arch_merchant；改编） | 否 | ch11.jinlei → ch11.kaidian | fangshi |
| `he_xianshi` | 何仙师 | ch412 | 炼气三层 | 新建（arch_elder_m） | 否 | ch11.kaidian → ch11.liugen（两张图各一对象） | fangshi、xiaoxuan |
| `linju_dianzhu` | 邻店店主 | ch413 | 凡人 | 新建（arch_townsman） | 否 | ch11.kaidian → ch11.liugen | xiaoxuan |
| `jiedan_linju` | 结丹邻居 | ch399 | 结丹初期 | 新建（arch_cultivator_m；两对象 `_a`/`_b` 同 role，O-4） | 否 | ch11.zhaolan → ch11.done | dongfu |
| `jin_qing` | 金青 | ch415 | 结丹初期 | 新建（arch_cultivator_m） | 否 | ch11.lianjian → ch11.done | dongfu |

**就地改写**（只此一处）：

| id | 改前 | 改后 | 理由 |
| --- | --- | --- | --- |
| `siji_haishou` | `weaknesses` 「土」 | 「土、暗器」；其余数值、note（「四级大致对应筑基后期」）不动 | 全仓只有 `b11_ningcuidao_youyao` 一张编成含它（2026-10-12 grep 过），改破绽不牵动第 1–10 章；给 ① 第二波一条本章玩家手上有的「暗器」路（透明丝线，C.4） |

**沿用**：`hanli`、`qu_hun_shadan`（全章在队，**数值、名牌都不动**，E18——改它就牵动第 10 章 ④ 执法队的平衡）、`wu_chou`（只说话；数值是第 12 章的，16.2 第 7 条口径）、`zi_ling`（只借名牌「紫灵仙子」站在礁岛上）。
**不新建**：天星双圣、星宫长老（E6）、汪门主、封某（鹰钩鼻修士，旁白里被焚）、木龙真人与文樯（B1、B2，本章 0 出场）、传送符、鲁二以外的凡人店主群像（E24）、隐煞门其余弟子（只在编成里的四名煞妖之外不摆人）、乌丑身边的随从。
**不动**：`qu_hun`、`qu_hun_huashen`、`wuji_haishou`（它是 `b13_waixinghai_lieyao` 的）、`sanji_haishou`（从 ① 编成里去掉，文件不动）及其余旧 role。

**撤场 / 出场旗标**（`NpcPresenceTests`，规则 30「前一段的 hidden 恰是后一段的 visible」）：
- **首尾相接的**：卓如婷（礁岛 hidden `ch11.yaoshi` = 雅间 visible `ch11.yaoshi`）；范夫人雅间对象 `npc_fan_furen` 跨 `ch11.lianer → ch11.zhanglao` 一段（礁岛 `_2` 在另一张图，`ch11.zhuzhan → ch11.yaoshi` 嵌在这一段里——规则 30 只查同图，不冲）；乌丑 → 极阴祖师（礁岛 `wu_chou` hidden `ch11.jiyin` = `jiyin_fushen` visible `ch11.jiyin`，同格换 NPC）；紫灵两个 role 不同名牌（`zi_ling` → `zi_ling_zhenrong`），不算同 role；星空殿一行人与枣红老者同以 `ch11.chuansong` 撤场。
- **待协调者核的三处**（本图照 H/G 表抄，不改 id，若规则 30 用例红则按这里的建议挪旗标）：① `zhao_zhanglao` 在坊市雅间的 hidden 是 `ch11.chaguan`、在礁岛的 visible 是 `ch11.zhuzhan`——中间隔一个节点 n21（茶馆散了、半月之后才赴集合岛），hidden ≠ visible；两张图不同，规则 30 不查跨图；用例若查跨图则把礁岛对象的 visible 保持 `ch11.zhuzhan`（它本就靠 n21 的 teleport 才见得到）；② `he_xianshi` 两张图两个对象、同一对旗标（`ch11.kaidian → ch11.liugen`）同时在场——原著他是隔壁邻居、串门常客（ch412），两处同在不算矛盾，但若用例判「同 role 不得同时在两图」，去掉坊市街上的 `npc_he_xianshi`（打探 `hexianshi_dating_fanren` 挪到店内对象）；③ `jiedan_linju_a` / `_b` 同图同 role 同旗标——O-4 已列。
- **目标链之外的在场旗标**：`ch11.lianjian`（n30 由 `trigger_chujian` 置，金青以它出场）在目标链上；`ch11.jiguan`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.lianjian_qian` 不作在场旗标用（登 `kOffChainFlags`，N.4）——**在场旗标里没有一个不在目标链上的**。

---

## 6. 支线（2 条可选，0 条必经）

| | Z1 收购行订单（可选） | Z2 邻居的恭贺（可选） |
| --- | --- | --- |
| 出处 | ch389 猎妖回城换灵石的流程（无许可制度，A6）；接单形式是本作改编（替代大纲的「猎妖令」，B9、A19） | ch399 出关后邻居恭贺与招揽、以定颜丹换珍稀物（A48） |
| 触发（`accept`） | `ch11.ningcui`（到了凝翠岛小街，范小子常驻） | `ch11.zhaolan`（传音符之后，结丹邻居出场） |
| 步骤 | 一步：与 `npc_fan_xiaozi` 对话接单（`ch11.z1 = 1`），再带 3 颗五级妖丹回来交付（`take("material_yaodan_wuji", 3)` 成则 `ch11.z1 = 2`） | 一步：`npc_jiedan_linju_b` 的 interact 脚本 `linju_z2.lua`：`take("pill_dingyan_dan", 1)` 成则 `give("material_lianqi_yao", 3)`、`ch11.z2 = 1`；没丹则一句（路径行动 `linju_dating_dingyan` 只打探「邻居想要定颜丹」，O-3） |
| 完成 | `ch11.z1 == 2`：酬 中阶灵石 10 ＋ `material_lianqi_yao` 2（由脚本 `give`，不进 rewards——第 10 章口径） | `ch11.z2 == 1` |
| 失败 / 过期 | `ch11.huicheng`（五年收手回城，凝翠岛再到不了） | 无过期（邻居到 `ch11.done` 都在） |
| 任务板 | 坊市 `facility_xuanshang` 也列它（E10） | 不列 |
| 文件 | `data/quests/q11_shougou_dingdan.json`，`origin: "改编"` | `data/quests/q11_linju_dingyan.json` |

**为什么只有这两条、而且都可选**：本章目标链已 29 步（E26），必经支线要进目标链；原著里韩立在场、有来龙去脉的旁枝就这两件。Z1 给猎妖循环一个「订单」的由头（大纲 4.4 的改编标注落在 `origin` 与图鉴注上）；Z2 让起点背包里那三四颗定颜丹有一处去路（C.4）。**全作 45 条 / 必经 14 条的账由协调者核；若账要本章出 1 条必经，把 Z1 改必经并进目标链（O-8）。**
不做成支线的：鲁二雇不雇（n03 的二选一，信息量相同）、丰乐拍卖行与储物袋求购（路径行动 `fengle_dating_mulu`、`chuwu_dating_dai`，E9）、枣红老者的联手之邀（n10 旁白一句，曲魂回绝，A29）、郝远天的招揽（n10 旁白）、摘星大会（`baimen_dating_zhaixing` 一句传闻，A8）、妙音门名义长老的年俸（N.1：本章之后再无「年」的结算口）、金青请破阵（第 12 章的事，O.1）。

---

## 7. 系统：租赁与月费、星空殿传送、猎妖循环与妖丹经济、六十年闭关、青竹小轩、七十二剑、结丹口径、分身

**先查了已有的，再定做多少**（D 表）：境界枚举 `CoreEarly 31` 与 Lua `realm.advance/cap` 都在；冲关加成入口 `breakthroughPillBonus` 仍是返回 0 的桩（`CultivationScene.cpp` 第 363 行）；静室 `effectiveness` 属性已有；`advanceDays` 单次上限 `kMaxScriptAdvanceDays = 3650`；商店 `Shop{buyRate,sellRate,entries}` 与 `ShopScene` 的 Buy/Sell 两页、只收 `tradeable` 的；拍卖只有规则层 `bidStep`（数据目录、加载器、面板、Lua 入口全缺）；任务板 `BoardScene`；遭遇表的掉落只许灵草（`validate.py` 第 2716–2731 行）；灵田 `field.unlock`；小瓶 `bottle.spend/mature/drops`；配方 `requireFlag`、`difficulty`、`requiredProficiency`；脚本命令清单里没有加熟练的命令；`party.add/remove/has`；NPC `visible_flag`/`hidden_flag`（**只对 npc 生效**，portal 与 facility 不读）；路径行动 `when`/`until`；`today()`；脚本 `shop(id)`。**本章不加任何新脚本命令；引擎只动契约 E1 一处**（`Recipe::guaranteed`，`Crafting.h/.cpp` ＋ `RecipeLoader`）——`successChance` 夹在 [5, 95]（`Crafting.cpp` 第 239 行 `std::clamp`），`difficulty 0 / requiredProficiency 0` 在熟练 0、资质 50、炉鼎 3 品下只有 65%，失败折损三根金雷竹即死局。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **租赁与月费** | **红环 / 蓝环**（n02）：曲魂的红环免费（结丹待遇，A10）；韩立 `take("material_lingshi_zhong", 8)` 成则红环 `ch11.zhihuan = 1`，否则 `take("material_lingshi", 3)` 蓝环 `= 2`，再不成也给蓝环、旁白换句（A9、A11）。**洞府**（n04）：执事台词列三十一层三座、三十九层四座、四十七层两座（A16），只能选三十九层；`take("material_lingshi_zhong", 27)`，不足则有几块扣几块、执事「记在结丹前辈名下」（E13）；给 `story_jinzhi_lingpai`。**「每月缴灵石」**只在典籍与路径行动 `baiyi_dating_guiju` 的台词里（A4、B7）；**「百年」**只在 n04 旁白里（A15）。静室 `facility_jingshi_39` `effectiveness 125`（A18、E11）；药园 `field.unlock("field_tianxing_39", 6)`（E12） | 不做月费扣款、不做续租、不设任何付款门闸（E13）；不做层数选择的分支（三选一只在台词里） | 原著「每月」（ch389）与「百年」（ch391）两句都有，做机制只能择一而且与「结丹免费」撞车；付不起的玩家在 first 侧 117 中阶之外的任何合法花法上都不该死局 |
| **星空殿传送** | n09 挂在 `npc_xingkong_xiushi` 上、`once` 否。首次：拼阵，`take("material_lingshi_zhong", 2)`（A26–27 的二百），不足则「对方全出」；此后每次出 / 回 `take(..., 7)`（一次七百、最多七人，A26），不足则「搭别人的阵」免费（E14，改编一句旁白）。每次出海 `teleport("ch11_ningcui_jie")`（落 default spawn `shiwu`；`flag.set("ch11.chuansong")` 与 `teleport` 都顶格，kScriptTransfers 三元行）。回程走凝翠岛石屋 `trigger_huicheng_chuansong`（`enter`、`once=false`、`guard ch11.chuansong`，脚本 0 处 `flag.set`）→ `teleport("ch11_xingkong")`（落 `dian`）。`ch11.huicheng ≥ 1` 后 n09 只说「本章不再出海」并 `return` | 不做按人头计费、不做传送符物品（只在台词）、不做妖兽岛列表（只去凝翠岛 → 红瑚岛一条线）、不做组队（A32） | **传送永远走得通**：700 硬门闸 ＋ 一趟只回 1–2 颗丹，玩家可能卡在凝翠岛（E14） |
| **猎妖循环与妖丹经济** | **诱妖**（n12，`trigger_youyao` interact、`once=false`）：要 `herb_nichang_cao ≥ 1` 且 `bottle.drops ≥ 1`；`bottle.spend(1)` → `advance_days(3)`（A37「每隔两三日」）→ `battle("b11_ningcuidao_youyao")`；**battle 返回后**胜则扣 1 株、`ch11.lieyao += 1`，逃不扣草（拍板，3.2 n12），妖丹由编成 `drops` 给（五级妖丹 1 颗 rate 100）；缺草 → 旁白指凝翠岛杂货；缺绿液 → 「等几日」`advance_days(5)`。n08 必给 3 株（ch395 他本就从坊市收购了一批草、带幼苗上岛）——三场 ① 的草不靠钱。`ch11.lieyao == 4` 且 `ch11.jiguan == 0` → n13 鸡冠蛟（可逃、不致命；胜则六级妖丹由编成 `drops` 给，脚本不 `give`）。**收手**（n14，`ch11.lieyao ≥ 3`）：五年蒙太奇 `advance_days(1800)`，给五级妖丹 8、`story_yaoshou_cailiao`、`story_kuilei_can`，收 `story_kuilei_qun`。**卖丹**：凝翠岛收购行 `ch11_ningcui_shougou`（`entries` 空、`sellRate 0.8`，五级 760、六级 4800——由 `tradeable` 决定收什么，E8）；Z1 订单另给中阶 10。霓裳草两处杂货都卖（价 10；字段照 `data/shops/ch10_tiandujie_pu.json`：坊市 `stock 10 / restockDays 5 / restockAmount 5 / maxStock 10`，凝翠岛 `stock 5 / restockDays 5 / restockAmount 3 / maxStock 5`） | **不设遭遇区**（遭遇编成掉不了妖丹，E15）；不做猎妖令、不做许可（A6、B9）；不做「数百颗」的真实数量（只在旁白，玩家实得 5–10 颗，A40）；不做傀儡群的编成（旁白里「群射」，沿第 10 章 16.1 第 23 条）；不卖六级妖丹（只有 ② 一颗，第 12 章入药的钩子，A73） | 循环的动机要算得过来：收购行默认 `sellRate 0.5` 一颗只回 475，比一趟传送费（7 中阶）还薄；0.8 才有「一颗近千」（A7）的感觉。妖丹是本章唯一的「挣钱」——n26 会把它们扣光（A64），所以给多少都是原著的「倾家荡产」 |
| **六十年闭关的表现** | n16 `trigger_biguan`：先 `take("pill_jiangchen_dan", 5)`、`take("material_xuelingshui", 2)` 各看返回值，位掩码 `ch11.dan_fu`（1 降尘丹、2 雪灵水）选旁白三版（全扣 / 部分 / 零）；**三段蒙太奇**（E5）：`fade.out` → 旁白「十年」→ `advance_days(3600)×2` → 曲魂隔门一句（噬金虫互噬、产卵）→ `advance_days(3600)×2` → 旁白「三十年」→ `advance_days(3600)×2` → 天象旁白（黑云闪电、百余里灵气漩涡，A43）→ `local ok, code = realm.advance(realm.CORE_EARLY)`（不成则占位一句 `return`）→ `fade.in`；置 `ch11.jiedan`。n15 之前的「散功半月到筑基初期」只写旁白、**不 `realm.demote`**（A42） | 不抬 `kMaxScriptAdvanceDays`（引擎项）、不做新的「岁月流逝」画面（美术项）、不接 `breakthroughPillBonus`（E4）、不做面板冲结丹（E3）；二转年数回避（B16）；金丹「四色」可写、年数只写「数年」（A44） | 游玩时长 `playSecondsGameplay` 只按真实秒累计，`advance_days` 不碰它，「跳过的时间不计游玩时长」天然成立（文档注明即可）；六次 3600 的灵田 / `restFor` 结算是否溢出由引擎路跑 `Ch11EngineTests`（O-5） |
| **青竹小轩经营与回访结算** | n27 盘店 `take("material_lingshi", 50)`（有几扣几）；「李氏杂货」改「青竹小轩」（A66）；`flag.set("ch11.kaidian_ri", today())`、`ch11.jiesuan_ri = today()`；店门 portal 开。n28 柜台 `trigger_jiesuan`（`once=false`，E22）：读两枚日期，每满 30 日给 15 低阶灵石（一次上限 7200 日 = 3600），再 `ch11.jiesuan_ri = today()`，`ch11.jiesuan += 1`（首次附旁白）。商店 `ch11_qingzhu_xiaoxuan`（买入页空、卖出 `sellRate 0.6`：玩家卖符与药材）。卖出口不做 shop facility：柜台是 `trigger_guitai`（interact、`once` 否），`guitai.lua` 开头判 `ch11.liugen` → 一句「店已盘出」`return`，否则 `shop("ch11_qingzhu_xiaoxuan")`。收店（n29 `zhu_gen == 6`）：`advance_days` 补足到开店起 7200 日（照 `sanzhuan.lua` 的补足写法，至少 360）、`give("material_lingshi_zhong", 20)`；**店门照开**（portal 与 facility 都不读 `hidden_flag`，`WorldScene.cpp` 第 67–75 行只在 `npcVisible` 里判；`genmaps.py` 的 `portal()` 也没有这个参数），收店后 `trigger_jiesuan` 与 `trigger_guitai` 都只说一句，何仙师与邻店主以 `hidden_flag ch11.liugen` 撤场 | 不做真实库存经营（新模块）、不做雇工、不做店铺升级；不卖丹药（B8：ch412 只卖符与药材、后加低阶法器）；韩立易容、曲魂扮仆只在文案，名牌不改 | 开店在原著是磨入世心境（A67），本作把它做成「时间换灵石」的一口结算：培竹三年一根（n29）与回访结算共用同一本日历，玩家每回店一次就看见二十年怎么过去。**O-6**：玩家卖符给自己的店若怪，改成只结算、不设 shop facility |
| **七十二剑炼成** | **培竹**（n26，`trigger_peizhu`、`once` 否）：每次 `bottle.spend(1)` 成则 `ch11.zhu += 1`（1 白 → 2 蓝 → 3 金，A63），不成「等几日」；`zhu == 3` 置 `ch11.jinlei`：`take("material_tianleizhu", 1)`、`give("material_jinleizhu", 1)`、`flag.set("ch11.zhu_gen", 1)`（**第一根在这里计数**）；两年凑辅料 `take("material_lingshi_zhong", 全部)`、`take("material_yaodan_wuji", 全部)`、`advance_days(720)`、`give("material_lianjian_fuliao", 1)`（**必给**，不看扣到多少，A64）。**六根**（n29，同一触发器换分支，跑第 2–6 根共 5 轮）：每次 `bottle.spend(1)` → `advance_days(1080)` → `ch11.zhu_gen += 1`、`give("material_jinleizhu", 1)`；到 6 置 `ch11.liugen`（金雷竹恰 6 根进配方，不多炼一根）。**炼剑**（n30）：`trigger_lianjian`（同格 enter、once、`set_flag ch11.lianjian_qian`，演在面板前：打坐三日、六根炼液、布阵半月、黄道吉日，`advance_days(120)`）→ 玩家在 `facility_lianqi_39`（forge，grade 3，`require_flag ch11.liugen`）选 `recipe_qingzhu_fengyun_jian`（**就地改写**：`productCount 1`、`inputs` 恰金雷竹 6 ＋ `material_lianjian_fuliao` 1（**不要灵石**——炼剑是主线必经门闸，收店后低阶无来路，要钱就是死局）、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`、**`guaranteed true`**（契约 E1；没有它面板只有 65%），E16、B14）→ `trigger_chujian`（密室门外 enter，要 `item.count("weapon_qingzhu_fengyun_jian") >= 1`）：`advance_days(60)`、七十二口入体（A71）、置 `ch11.lianjian` | 天雷竹不走 `matureHerb`（年份跳万年不合它的曲线）；不加 Lua 加熟练命令（引擎项）；不做剑阵 / 剑影分光术的法术（E17，第 12 章定）；一件 `weapon_qingzhu_fengyun_jian` 代表七十二口（note 已这么写） | 剧情直接 `give` 则炼器面板全作没有用过一次（E16）；炼器熟练是 0（C.4），`difficulty 0 / requiredProficiency 0` 只到 65%，必成靠契约 E1 的 `guaranteed`；原著他是一次炼成、无材料再试（B14） |
| **结丹口径** | **剧情 `realm.advance(realm.CORE_EARLY)` 恰 1 处**（n16），`realm.cap` 0 处、`realm.demote` 0 处（C.1）；`storyCap` 保持 23 到 n16，之后由 `realm.advance` 顺带抬到 31；面板上的下一关「结丹中期」所需修为 5000，本章**不许过**（`realm.cap` 不抬到 32，面板按 `BreakthroughBlock` 回绝，C.3）。结丹初期锚点：气血 700、攻 62 / 防 46（`Realm.h` 注释第 110–115 行，C.2）——**本章就是标定结丹初期的那一章**：平衡路用 `tests/BattleHand.h` 量 I 表四场后，把锚点注释改成「第 11 章标定」。出关（n17）：小瓶 3 日一滴由引擎按境界给（`Application.cpp` 第 130 行）；E27 起外人直接与韩立说话 | 不做面板冲结丹（面板要 3500 修为、可反复摇骰，与「六十年一关」的重量相悖，E3）；降尘丹、雪灵水只「有则扣」、不接加成（E4）；天火液 0、血凝五行丹 0 不提（C.3） | 原著是六十年生死关、「意外成功」（ch398），面板摇骰子会把一次性的大事变成可刷的事；起点修为 2092 / 2031 距 3500 尚远（C.4），也佐证不该走面板 |
| **分身** | `qu_hun_shadan` 全章在队、数值与名牌不动（E18）。**曲魂代言**（E27）：入城、租府、星空殿、凝翠岛小街外人先跟曲魂说话（ch390–394 原著如此），第一次交代一句，不再解释；n17 结丹之后外人直接跟韩立说话。n25 紫灵识破曲魂是身外化身、与本体同为结丹初期——只是台词一句（A61）。战斗里四场都在队（第 8 节：①② 主攻、③ 并肩压制、④ 紫焰困妖） | 不做 `hero_absent`（本章四场韩立都在场，与第 10 章 ②④ 不同）；不改 `qu_hun_huashen`/`qu_hun`；不做化身的装备栏；n06 「炼化混元钵」只一句旁白 | 第 10 章把「分身的三种用法」演完了；本章它是一个稳定的结丹队友，分量在**他人眼里**——执事记账、紫灵识破、妙音门招的是「两位结丹」 |

**本章所有门闸一览**（没有一处是钱；全部是旗标或物品计数）：

| 门闸 | 在哪 | 条件 | 未满足时 |
| --- | --- | --- | --- |
| 坊市北口（上三十九层） | `portal_shangshan` | `ch11.zufu` | `ch11.fangshi.shangshan_deny`（租府之前上不去） |
| 洞府静室 / 丹房 / 药园 | `ch11_dongfu` 三处 facility | `ch11.anju` | `ui.facility.locked` |
| 坊市杂货铺 | `facility_zahuo` | `ch11.nichang` | 同上（悟出霓裳草之前没有买草的理由） |
| 星空殿北口 | `portal_xingkong` | `ch11.nichang` | `ch11.dongfu.xingkong_deny` |
| 出海 | n09 脚本 | `ch11.huicheng == 0` | 「本章不再出海」`return` |
| 凝翠岛东口 | `portal_chuhai` | `ch11.ningcui` | 门关着 |
| 诱妖 | `trigger_youyao` | `ch11.buzhen`、草 ≥ 1、绿液 ≥ 1（n08 必给 3 株，三场 ① 不靠买草） | 旁白指杂货 / `advance_days(5)` |
| 鸡冠蛟 | n12 内分支 | `ch11.lieyao == 4` 且 `ch11.jiguan == 0` | 继续 ① |
| 收手 | `trigger_huicheng` | `ch11.lieyao ≥ 3` | 「再猎几次」 |
| 茶馆雅间门 | 坊市东北角 portal | `ch11.lianer` | 门关着 |
| 青竹小轩门 | 坊市街西 portal | `ch11.kaidian` | 门关着（收店后**门照开**：portal 不读 `hidden_flag`） |
| 柜台与结算 | `trigger_guitai`、`trigger_jiesuan` | `ch11.liugen == 0`（脚本开头判） | 一句「店已盘出」`return` |
| 炼器台 | `facility_lianqi_39` | `ch11.liugen` | `ui.facility.locked` |
| 出剑 | `trigger_chujian` | `item.count("weapon_qingzhu_fengyun_jian") >= 1` | 不响 |
| 配方 | `recipe_qingzhu_fengyun_jian` | `requireFlag ch11.liugen`、金雷竹 6、辅料 1（不要灵石；`guaranteed true`） | 面板灰掉 |

**起点与章末对照**（写测试时读表，不抄数；起点读自 `tests/fixtures/ch10-end-first.sav`，章末待 `ch11-end` 真实验收）：

| | 起点（C.4 实读） | n16 之后 | 章末 spec |
| --- | --- | --- | --- |
| 境界 / 上限 | 23 / 23 | 31 / 31 | 31 / 31 |
| `formerRealm` | 22 | 22 | 22 |
| 气血 / 攻 / 防（Realm.h 锚点） | 480 / 44 / 30 | 700 / 62 / 46 | 700 / 62 / 46 |
| 静室效率 | — | 125（三十九层） | 125 |
| 瓶子 | 容量 6、五日一滴 | **容量 9**、三日一滴（`realm.advance` 顺带 `max(capacity, bottleCapacityFloor(Core))`，`Application.cpp` 第 1337 行） | 容量 9、三日一滴 |
| 法术 | 11 门 | 11 门 | 11 门（E17） |
| 队伍 | `qu_hun_shadan` | 同 | 同 |
| 中阶灵石 / 五级妖丹 | 117 / 1 | 0 ＋ 40（n18 扣光、n20 必收） | ≈ 20 / 0（n26 扣光、n29 必收） |
| 六级妖丹 / 七十二剑 | 0 / 0 | 0–1 / 0 | 0–1 / 1 |

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** = 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章按预算记 **4 场**（E23、I 表）：① n12 千目妖（循环仗，`ch11.lieyao` 是 n14 的 guard，至少打 3 次）、② n13 鸡冠蛟、③ n22 煞妖、④ n24 妖尸。严格按口径，② 是**可错过**的：它在 `ch11.lieyao == 4` 时才触发，而 n14 在 `≥ 3` 就能收手，`ch11.jiguan` 也不是任何节点的 guard（登 `kOffChainFlags`）——**预算仍按 4 场计**，协调者核 8.0 口径时若把 ② 记为「可选」，必打就是 3 场；两种记法都不改编成。①③ 是原著的仗、② 是改编（原著它徘徊离去）、④ 是「困而不杀」（8.3）。**可选：无**（没有遭遇区、没有切磋——城内不准斗法，A4）。
第 1–10 章合计 37 场（第 10 章施工图 8.0），方案 P3 的 52 场余 15 场给第 11–14 章；本章 4 场 → **剩 11 场给第 12–14 章**。

**这四场就是「结丹初期」的标定**（C.2）：

| # | 谁上场 | 这一仗量什么 |
| --- | --- | --- |
| ① | 韩立（筑基后期）＋ 曲魂（结丹初期） | 筑基后期对五级（结丹初期档）妖兽——玩家第一次看见五级妖丹值多少 |
| ② | 同上 | 筑基后期照面六级（结丹中期档）——逃是正解之一 |
| ③ | 韩立（**结丹初期**）＋ 曲魂 | 结丹对筑基的境界压制（`suppressionFactor`）——结丹的分量 |
| ④ | 韩立 ＋ 曲魂 ＋ 血玉蜘蛛 ×2 ＋ 猿傀 ×2 | 打不死的东西——破势是目的不是手段 |

### 8.1 意图玩家

- **①②**：筑基后期（480 / 320，攻 44、防 30；若第 8 章 E2 标定改了筑基档，以落地为准）；法术 11 门（火弹术、御风决、天眼术、流沙术、冰冻术、祭金符、祭青蛟、青元剑芒〔木，`needRealm FoundationEarly` 已可用〕、祭青火掌、祭小刀、祭红线针）；兵刃：烈阳剑、冷月刀、碧光刀、玉带短剑、透明丝线（剑、刀、暗器）；符箓：护身符 4/3、火球符 16、土牢符 1/2、天雷子 0/1、金刺符 1/0、小刀符宝 1、蓝蛟符 1、血灵钻 1、魔焰初阶 1（C.4，两侧不同处写「/」）；道具：中阶灵石（回法力）、金疮药 7/8、回气丹 2。队友曲魂（结丹初期，剑 ＋ 火 ＋ 金）。**① 开场破绽「土」可能已揭**（路径行动 `haoyuantian_dating_qianmu` 的 reveal）；**② 的「金」只在 n09 之前打探枣红老者才拿得到**（`zaohong_dating_jiguan`，可错过）。修为：2092 / 2031，① 是本章唯一给修为的仗（平衡路定数）。
- **③④**：结丹初期（700 / —，攻 62、防 46，C.2——法力上限由 `Realm.h` 锚点给，本表不抄）；法术、兵刃同上（青元剑芒照旧）；符箓按 ①② 的消耗剩下多少算多少（平衡路按「①×4 ② ×1 之后」量）；中阶灵石 ③ 之前是 40（n18 扣光、n20 必收，N.1）。队友曲魂；④ 另加写死的友军 `xueyu_zhizhu_ally` ×2（四级顶峰）、`kuilei_yuan` ×2（筑基中期，代表「上百只」，B24）。**③④ 之间没有存档点**（第 4 节）。

### 8.2 四张编成

| # | 编成 id | 敌方 | 我方 / 友军 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b11_ningcuidao_youyao`（**就地改写**） | **千目妖**（`qianmu_yao`，五级，首领；蓄势「刺球光罩」）；第二波 **四级海兽** ×2（`siji_haishou`，随草香而来，改编）；`sanji_haishou` 去掉 | 韩立 ＋ 曲魂 | **可逃**（`can_escape true`；**拍板**：扣草挪到 `battle` 返回之后——胜扣 1 株、逃不扣、败 game_over，3.2 n12 与验收 8 同此） | **致命**（A34 原著「几次陷入生死一线」） | **阵中先手、破势优先**：首领蓄势一次必须被破势打断，否则全体受伤；破绽「土」（流沙术、土牢符）与「暗器」（第二波，透明丝线）都是玩家手上现成的；曲魂以剑与血色光柱主攻（`qu_hun_shadan.json` 只有兵刃「剑」与血色光柱 / 紫焰 / 金剑三门法术，没有混元钵的动作）。打赢掉 1 颗五级妖丹——**让玩家第一次看见妖丹值多少**。`rewards.cultivation > 0`（本章唯一） |
| ② | `b11_honghu_jiguanjiao`（新） | **鸡冠蛟**（`jiguan_jiao`，六级，首领、多动；三回合内蓄势伤害明显高于五级） | 韩立 ＋ 曲魂 | **可逃**（`can_escape true`） | **不致命**（`defeat_is_fatal false`：原著它徘徊离去，A38） | **第一次照面六级——逃是正解之一**：破绽「金」要靠祭金符或曲魂金剑；打赢由编成 `drops` 掉 `material_yaodan_liuji` 1（第 12 章入药的钩子，A73；脚本 0 处 `give`，纪律 54），逃或败同一句旁白、不掉。胜后文案与图鉴注「改编」 |
| ③ | `b11_yinsha_shayao`（新） | **隐煞门煞妖** ×4（`yinsha_shayao`，筑基后期；第 2 回合「变身」用 `charge` 写成全体蓄劲） | 韩立 ＋ 曲魂 | 否 | **致命** | **结丹初期第一仗——连击清场**：境界压制让四个筑基后期打不动他；四敌同时蓄劲，教「拳 / 剑的蓄劲连击 1+N」在变身前打掉几个；破绽含「剑」（烈阳剑、青竹蜂云剑还没有）与「木」（青元剑芒）。编成取 4（B17） |
| ④ | `b11_tiandu_shi`（新） | **妖尸**（`tiandu_shi`，首领、多动、瞬移；「自愈」用 `charge` 表达；**`killable_by ["水","火"]`**——加载器 `DataLoader.cpp` 第 531–534 行与 `validate.py` 第 2435–2439 行都拒空数组；其余类别最多压到 1） | 韩立 ＋ 曲魂 ＋ `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2 | 否 | **致命**（A59 原著「狼狈亡命」，败即被追上） | **打不死的东西**：胜负在引擎里仍是气血归零（`BattleAction.cpp` 第 561–562 行，没有「破势两次即胜」），只有水、火那一击要得了命——`weaknesses` 含「水」（蛛网 `magic_zhizhu_tuwang` / 冰冻术）与「火」（紫火 `magic_kuilei_zihuo` / 紫焰）；友军 `actions` 引这两门法术，玩家要用它们把它困住。**胜 = 困住，文案与图鉴注不写杀死**（E25、契约 E3）。O-1 已答：`toughness` 低只是容易破势吃 ×2，不是胜利条件 |

- **每场的 `charge` / `actions` 框架**（只写名目与节拍，数归平衡路）：

| # | 首领蓄势（名目 / 节拍 / 目标） | 其余行动 | 友军 `actions` |
| --- | --- | --- | --- |
| ① | 千目妖「刺球光罩」：每 3 个出手回合一次、全体；被破势则取消 | 千目妖普攻单体；四级海兽 ×2 第二波入场、普攻 | 曲魂：剑、血色光柱（火）、金剑 |
| ② | 鸡冠蛟：多动（行动 2）；蓄势伤害明显高于五级、三回合内落下 | 鳞甲护体（破绽只有「金」打得透） | 同上 |
| ③ | 煞妖 ×4：第 2 回合「变身」全体蓄劲（`charge`）；变身后攻防抬一档 | 变身前普攻单体 | 同上 |
| ④ | 妖尸：多动、瞬移（行动顺序随机）；每回合「自愈」（`charge` 表达，O-1） | `killable_by ["水","火"]`；破绽恰「水、火」 | 血玉蜘蛛 ×2「吐网」（`magic_zhizhu_tuwang`，水，压架势）；猿傀 ×2「紫火」（`magic_kuilei_zihuo`，火）；曲魂：紫焰（火，全体） |

- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，E19）：`CHAPTER_MEANS[11]` = 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`weapon_wuming_sixian`，ch07）、剑（烈阳剑 give ch06 `lingqu.lua`）；`BATTLE_EXTRA_MEANS` 四场各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金（照 `b10_zhifadui` 写法）；`MEANS_LAST_CHAPTER = 11`。四场「至少一样破绽是本章我方手段打得到的」：① 土 ② 金 ③ 剑 ④ 水。
- **selftest 探针**：`validate_selftest.py` 第 1801–1820 行指 `siji_haishou` + `b11_ningcuidao_youyao`、要求敌方含 `siji_haishou`——本章写实这张编成后挪到 `wu_chou` + `b12_neidian_duoding`（全仓唯一含 `wu_chou` 的编成），措辞改「第 12 章范围探针实际引用乌丑」，第 1815 行 `probe["chapter"] = 10` 改 11（E19、O-2；推翻第 10 章 16.1 第 35 条，16.2 第 11 条）。
- **数值归战斗路按 `tests/BattleHand.h` 那只手量**；本表只写意图与破绽。必须守住的：① `qianmu_yao` ×1 ＋ `siji_haishou` ×2、破绽含「土」「暗器」、`can_escape true`、`defeat_is_fatal true`、`drops` 五级妖丹 1 颗 rate 100、`rewards.cultivation > 0`；② `jiguan_jiao` ×1、破绽含「金」、`can_escape true`、`defeat_is_fatal false`、`drops` 六级妖丹 1 颗 rate 100；③ `yinsha_shayao` 恰 4、破绽含「剑」、`can_escape false`、`defeat_is_fatal true`；④ 友军恰 `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2、`killable_by` 恰 `["水","火"]` 且 ⊆ `weaknesses`、破绽恰「水、火」、`can_escape false`、`defeat_is_fatal true`、胜 = 气血归零（文案写困住）；四张 `hero_absent false`、`spirit_stones == 0`；③④ `drops` 空、`cultivation 0`（③ 之后他已是结丹初期、上限 31，给了也按不动）；② `cultivation 0`。四场都要写 `weapons / toughness / weaknesses / actions / charge`（`docs/octopath-battle.md` 第 5 节），门禁规则 24、25。
- 战斗背景（`data/visual/battles.json`）：① ② `cliff_top`（红瑚岛珊瑚礁）、③ ④ `cliff_top`（隐煞门礁岛）——**不新做背景**（第 4 节）。每一条都要能指出读它的那一行代码并有走真实入口的测试（handoff 第 6 节第 5 条）。
- 开场白（**内容路写在 `data/text/ch11_battle.json`**，key `ch11.battle.<编成短名>.intro`）：① 写草香散开、绿眼从雾里亮起（**不写它怎么滚过来**）；② 写蛟来得比预想的早一百年；③ 写四人身形暴涨的一瞬（**不写门主的台词**）；④ 写空间扭曲、它站在那里不动——四条都自己写、不抄原著。`data/text/battles.json` 里旧的占位 `ch11.battle.ningcuidao_youyao.intro`（第 6 行）**由协调者删**（契约 9.3 白名单登 `data/text/battles.json`）；文件归属全文只此一种说法（10.1 C5 同）。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 千目妖 | ch396：霓裳草展叶，千目妖入幻形天罗阵，傀儡群射、一顿饭取丹；此后一个多月每隔两三日一批、灭十几头五级（A36–37） | 可重复的剧情战：每次扣草与滴、推三日、打一场、掉一颗丹；第二波海兽是改编 | 原著是「一批一批来」，本作每一批就是一场；傀儡群在旁白里「群射」、不进编成（第 10 章 16.1 第 23 条）；第二波四级海兽「随香而来」是为了让 `siji_haishou` 这个旧 role 有一条「暗器」的路、图鉴注改编 |
| ② 鸡冠蛟 | ch396：草到四百年份引来六级鸡冠蛟，他封住草香、它徘徊离去；此后只催到三百年（A38） | 一场可逃、不致命的仗；逃 = 原著；胜 = 改编（给六级妖丹） | 原著没有交手，「逃」就是原著的走法；给玩家一个「打赢」的支线结果是游戏的让步，胜后文案与图鉴都注改编，六级妖丹本章不入药（A73） |
| ③ 煞妖 | ch404：岛上混战，他圈住四五名筑基邪修、邪修化煞妖、被他与曲魂杀尽（B17） | 一场仗：四敌、第 2 回合变身蓄劲 | 原著他就是打了这四个人；煞妖「化为飞灰」可用（原文如此，E25） |
| ④ 妖尸 | ch408：妖尸不怕法宝、伤口自愈、空中瞬移；他放一百多只青色巨猿傀儡与两头血玉蜘蛛，以蛛网与紫火困住后遁走；入地一月（A59–60） | 一场「困而不杀」的仗：`killable_by ["水","火"]`、破绽水火、友军写死、胜仍是气血归零 | 原著他没有杀它，胜利文案写「困住」（E25、契约 E3）；友军 ×2 代表「一百多只」（B24）；与第 10 章 ④ 执法队（化身独战）不同，这是全作第一次韩立在场、四友军 |
| 赤火破阵 | ch404：赤火老怪一掌破岛上禁制 | 演出（n22 开头） | 他只是看 |
| 乌丑 / 极阴附身 / 封某被焚 | ch405–406：乌丑现身、极阴附身、天都尸火焚人、禁神牌 | 演出（n23），韩立退到最后、不交元神 | 他没有出手；封某写「惨叫后再无声息」（E25） |
| 赤火倒戈困住门主 | ch407 | 旁白一句（n24 开头） | 众人四散，他只看了一眼 |
| 枣红老者 / 郝远天 | ch393–394 邀联手、招揽 | 路径行动 K7 / n10 旁白 | 回绝，无交手 |
| 莲儿媚术 / 狐媚术试探 | ch399–400 | 演出（n19、n20） | 被拒 / 曲魂挡住赵长老，无交手 |
| 结丹异象后的邻居 | ch398–399 传音符 | 旁白（n18） | 一律回绝 |

**不做的仗**：范夫人与赵长老的密谋、极阴与赤火分赃、三女议事（判据③，E24）；隐煞门门主对众（韩立只在边上）；妙音门押货遇劫、汪门主战死（两月前的事，A53）；乱星海七八个势力的叛乱（n31 谣言，A72）。

### 8.4 野外遭遇

**本章不放遭遇区**（E15）：妖丹是本章的经济，而遭遇编成的掉落只许灵草（`validate.py` 第 2716–2731 行）——用遭遇区做猎妖，妖丹就只能在剧情里给、循环不成立；所以诱妖做成可重复的剧情战。城里不准打斗（A4）、凝翠岛小街上的人都要出海、红瑚岛的妖兽全靠草香引来（A33）、隐煞门礁岛只进不出——没有一张图有「野外」。外星海「不贴海面、不飞太高、不聚众」的规矩只在旁白与 `haoyuantian_dating_waihai`（A32）。修为的来路是静室打坐（`effectiveness 125`）与 ①，不是遭遇。

---

## 9. 经济账

**四本账：低阶灵石（`material_lingshi`）、中阶灵石（`material_lingshi_zhong`，价 100）、妖丹（五级 `material_yaodan_wuji` 价 950 / 六级 `material_yaodan_liuji` 价 6000）、竹与丹（`material_tianleizhu` → `material_jinleizhu` ×6 → 七十二剑；降尘丹 5、雪灵水 2 只在 n16 有则扣）**。起点直接读 `tests/fixtures/ch10-end-first.sav` / `ch10-end-second.sav`（C.4）：低阶 254 / 234、中阶 117 / 116、五级妖丹 1 / 1、降尘丹 5、雪灵水 2、定颜丹 3 / 4、`day` 16003 / 15978。本章所有大额付出都扣中阶；下面的去向与终局是待 `ch11-end` 真实验收的 spec。

**来源**

| 项 | 低阶 | 中阶 | 妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| 起点（实读 first / second） | 254 / 234 | 117 / 116 | 五级 1 / 1 | 降尘丹 5、雪灵水 2、定颜丹 3 / 4；瓶 6 滴 |
| n04 租府 | | | | `story_jinzhi_lingpai` |
| n06 傀儡 | | | | `story_kuilei_qun` |
| n07 白门阁 | | | | `story_yaoshou_ziliao` |
| n08 试草 | | | | `herb_nichang_cao` ＋3（必给；三场 ① 的草不靠钱） |
| n10 凝翠岛杂货（可选） | | | | `story_waihai_tu` |
| ① ×N（编成 `drops`） | | | 五级 ＋1 / 次 | ① 的 `rewards.cultivation`（本章唯一） |
| ②（可错过，编成 `drops`） | | | 六级 ＋1 | |
| 收购行卖丹（玩家定） | ＋760 / 颗五级、＋4800 / 颗六级 | | | |
| Z1 交付（可选） | | ＋10 | | `material_lianqi_yao` 2 |
| n14 收手 | | | 五级 ＋8 | `story_yaoshou_cailiao`、`story_kuilei_can` |
| n15 两年炼丹 | | ＋30 | | |
| n18 三级傀儡 | | | | `story_kuilei_yuan_qun` |
| n20 妙音门交易 | | ＋40 | | |
| Z2（可选） | | | | `material_lianqi_yao` 3 |
| n25 名义长老 | | | | `material_tianleizhu` 1、`story_miaoyin_yaopai` |
| n26 两年辅料 | | | | `material_jinleizhu` 1（第一根，`zhu_gen` 置 1）、`material_lianjian_fuliao` 1（**必给**） |
| n28 回访结算 ×M | ＋15 / 满 30 日（一次封顶 3600） | | | |
| n29 六根 / 收店 | | ＋20 | | `material_jinleizhu` 每轮 ＋1（第 2–6 根，共 5 轮；累计恰 6） |
| n30 炼剑（面板） | | | | `weapon_qingzhu_fengyun_jian` 1 |
| n31 章末 | | | | `story_bai_zhenzhu` |

**去向**

| 项 | 低阶 | 中阶 | 妖丹 | 其他 |
| --- | --- | --- | --- | --- |
| n02 指环 | −3（蓝环，只在中阶不足时） | −8（红环） | | 都没也给蓝环 |
| n03 鲁二（可选） | −5 | | | 没钱也成 |
| n04 租府 | | −27（有几块扣几块） | | |
| n07 资料 | −100（只在中阶不足时） | −1 | | 都不成则「赊」 |
| n09 首次传送 | | −2 | | 不足则对方全出 |
| n09 以后每次出 / 回 | | −7 | | 不足则搭阵免费 |
| n10 海图（可选） | | −1 | | |
| n12 诱妖 ×N | 草 10 / 株（n08 给的 3 株之外才买；胜才扣草，逃不扣） | | | 绿液 −1 滴 / 次 |
| Z1 交付 | | | 五级 −3 | |
| n14 收手 | | | | `story_kuilei_qun` −1 |
| n16 闭关 | | | | 降尘丹 −5、雪灵水 −2（有则扣，`ch11.dan_fu` 记位） |
| n18 三级傀儡 | | **−全部** | | 一贫如洗一句（A49） |
| n20 交易 | | | | `story_yaoshou_cailiao` −1 |
| n26 辅料 | | **−全部** | 五级 **−全部** | `material_tianleizhu` −1 |
| n27 盘店 | −50（有几扣几） | | | |
| n30 炼剑 | （配方不要灵石） | | | `material_jinleizhu` −6、`material_lianjian_fuliao` −1 |
| Z2 | | | | 定颜丹 −1 |

**账要对得上的几条**（`Ch11LedgerTests` 直接读 `scripts/ch11/*.lua`、`data/battles/b11_*.json`、`data/recipes`）：

1. **中阶灵石 `take` 恰 8 处**（`zhihuan.lua` 8、`zufu.lua` 27、`ziliao.lua` 1、`xingkong.lua` 2 与 7 两处、`ningcui.lua` 1、`zhaolan.lua` 全部、`peizhu.lua` 全部）——**每一处都是「有几块扣几块」或另有退路，没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训；E13、E14）。**`give` 恰 4 处**（`liandan.lua` 30、`chaguan.lua` 40、`peizhu.lua` 收店 20、Z1 脚本 10）。
2. **低阶灵石 `take` 恰 4 处**（`zhihuan.lua` 3、`luer.lua` 5、`ziliao.lua` 100、`kaidian.lua` 50）；**`give` 恰 1 处**（`jiesuan.lua`，15 × 满 30 日数，封顶 3600）。
3. **五级妖丹**：`give` 恰 1 处（`huicheng.lua` 8）；`take` 恰 2 处（`peizhu.lua` 全部、Z1 脚本 3）；此外只由 ① 的 `drops` 来。**章末 0**（n26 扣光）。**六级妖丹**：`give`/`take` 0 处，只由 ② 的 `drops` 来，章末 0–1——第 12 章若要门闸，别依赖它（O.1）。
4. **降尘丹、雪灵水**：各 `take` 恰 1 处（`biguan.lua` 5 / 2）、`give` 0 处，**章末都是 0**；**天火液、血凝五行丹 0 处**（起点就是 0，不提）。**定颜丹** `take` 恰 1 处（`linju_z2.lua` 1）。
5. **竹**：`material_tianleizhu` `give` 1（`zhanglao.lua`）/ `take` 1（`peizhu.lua`）；`material_jinleizhu` `give` 2 处（`peizhu.lua` n26 分支 1、n29 分支每轮 1）、累计恰 6 根进配方——**已定：n26 的第一根计 `ch11.zhu_gen = 1`**（`flag.set("ch11.zhu_gen", 1)` 在 n26 分支），n29 分支只跑第 2–6 根（F 表「整数 1–6」口径，J 表「0–6」是值域）；`material_lianjian_fuliao` `give` 1 处（**必给**，不看 n26 扣到多少）。配方 `recipe_qingzhu_fengyun_jian` `inputs` 恰 金雷竹 6 ＋ 辅料 1（**无灵石**）、`productCount 1`、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`、`guaranteed true`（B14、契约 E1）。**配方不要灵石**：炼剑是主线必经门闸（n30b 要剑在身上），收店后店已盘出、凝翠岛到不了、中阶 `tradeable false` 换不成低阶（`data/items/materials/lingshi_zhong.json` 第 8 行），留 20 低阶就是第 6 章 HIGH-3 那种死局；n30 面板前的 `trigger_lianjian` 旁白里也不许扣钱。
6. **`bottle.spend` 恰 4 处**（`nichang.lua`、`youyao.lua`、`peizhu.lua` 两分支）；每一处都看返回值，不成则「等几日」`advance_days(5)`。
7. **story 物品**：`give` 9 处（n04、n06、n07、n10、n14 ×2、n18、n25、n31）、`take` 2 处（n14 `story_kuilei_qun`、n20 `story_yaoshou_cailiao`）；`story_shijin_chong`、`story_xueyu_zhizhu`、`story_lvhuang_jian` 0 处（不扣不动）；`story_hunyuan_bo` 0 处（**它不在背包**，第 10 章 `linshi.lua` 已 `take`，本章只在旁白提一句）。
8. **四张编成 `rewards.spirit_stones` 都是 0**；① `drops` 五级妖丹 1 颗 rate 100、`cultivation > 0`；② `drops` 六级妖丹 1 颗 rate 100、`cultivation 0`；③④ `drops` 空、`cultivation 0`。
9. **`realm.advance` 恰 1 处**（`biguan.lua`）、`realm.cap` 0 处、`realm.demote` 0 处（C.1）；`party.add/remove` 0 处（E18）。
10. **两侧同账**：通关两侧走同一组选项（原著那一支：n02 红环、n03 雇、n07 付中阶、n09 拼阵、n10 买图、① 打 4 次、② 打一次、n14 收手、**两侧都做 Z1 与 Z2**、不卖丹、草只买够用的）。中阶两侧章末**都是 20**（n18、n26 两次扣光把起点差 1 抹平，之后只剩 n20 的 40 与 n29 的 20——`take` 全部之后再 `give`）。低阶：起点 254 / 234 − 5 − 50 − 草（10 × 买的株数，n08 给 3 株、意图玩家 ① ×4 ＋ ② 逃只扣 4 株、买 1 株 = 10）＋ 结算（≤ 3600）＝ 两侧差 **20** = 起点差（配方不要灵石）；卖丹、战斗用药、炼丹成败不能抹账。
11. **终局账 spec，未实测**：章末中阶 ≈ 20、低阶 ≈ 数百到四千（取决于结算回访次数与卖丹）、五级妖丹 0、六级 0–1、七十二剑 1、降尘丹 / 雪灵水 0、定颜丹 2 / 3——第 12 章起点「不富」与原著一致（N.1）；第 12 章仍只能从 `ch11-end` fixture 读，不把这些 spec 数写作正式终点。
12. **商店**：`ch11_fangshi_zahuo` 1.0 / 0.4、`ch11_ningcui_zahuo` 1.2 / 0.3、`ch11_ningcui_shougou` 1.0 / **0.8**（`entries` 空、只收 `tradeable` 的——五级妖丹 760、六级 4800）、`ch11_qingzhu_xiaoxuan` 1.0 / 0.6（买入页空）；霓裳草价 10（坊市 `stock 10 / restockDays 5 / restockAmount 5 / maxStock 10`，凝翠岛 `stock 5 / restockDays 5 / restockAmount 3 / maxStock 5`）；所有商店 note 写「初版经济待正式起点验账」（L.4）。**本章不卖六级妖丹、不卖丹药**（B8）；`material_jinleizhu` 改 `tradeable false`（现为 true、price 20000——不改则卖出页列它、一根 12000 低阶），回写 B13 时顺手（契约 4.1）。

**一趟出海的账**（first 侧 117 中阶起算，按 N.1 的次序；意图玩家出海一次、在红瑚岛把 ① 打 3–5 次、② 一次、再回城）：

| 步 | 支 | 收 | 累计中阶 |
| --- | --- | --- | --- |
| n02 红环 ＋ n04 租金 ＋ n07 资料 | 8 ＋ 27 ＋ 1 | | 81 |
| n09 首次拼阵出海 ＋ n10 海图 | 2 ＋ 1 | | 78 |
| ① ×4 ＋ ②（胜扣 4 株、② 逃不扣；n08 给 3 株、买 1 株 = 10 低阶） | 低阶 10 | 五级妖丹 4、六级 0–1 | 78 |
| 收购行卖 5 颗五级（可选） | | 低阶 ＋3,800（≈ 38 中阶当量） | 78（＋38 当量） |
| 回城 ＋ 再出海（各 7） | 7 × 2 | | 64 |
| n14 收手 | | 五级妖丹 ＋8 | 64 |
| n15 卖材料 | | ＋30 | 94 |
| n18 三级傀儡 | 全部 | | **0** |

要点：传送费一来一回 14 中阶，相当于两颗五级妖丹在收购行的价；玩家若一颗都不卖、也付不出 7，就「搭别人的阵」（E14）——账上永远走得通。n26 会把五级妖丹全部扣光，所以「留丹不卖」与「卖丹换低阶」只是低阶余额的差，不影响主线。

**时间账**（`day` 起点 16003 / 15978，开局不推进；`advance_days` 单次 ≤ 3650，E5）：

| 节点 | 推进 | 累计（约） |
| --- | --- | --- |
| n06 育虫 ＋ 造傀儡 | 180 ＋ 720 | 900 |
| n08 悟草 ＋ 半年 | （缺绿液时 5）＋ 180 | 1,080 |
| n12 诱妖 ×N（N = 3–5） | 3 / 次（缺绿液 ＋5） | ≈ 1,095 |
| n14 五年出海 | 1,800 | ≈ 2,895 |
| n15 两年炼丹 ＋ 散功半月 | 720 ＋ 15 | ≈ 3,630 |
| n16 六十年 | 3,600 × 6 = 21,600 | ≈ 25,230 |
| n21 半月赶制傀儡 | 15 | ≈ 25,245 |
| n24 入地一月 | 30 | ≈ 25,275 |
| n26 两年辅料（培竹等绿液另计） | 720 | ≈ 25,995 |
| n29 第 2–6 根（三年一根）＋ 补足到开店起 7,200 日 | 1,080 × 5 ＋ 补足 1,800（至少 360） | ≈ 33,195 |
| n30 布阵吉日 ＋ 两月出关 | 120 ＋ 60 | ≈ 33,375 |

全章约 **33,400 日**，其中 n16 一处就占 21,600；折合年数与韩立年龄由协调者按 `Calendar.h` 的年长与大纲第 185–223 行的年龄表核（本图不抄年龄）。游玩时长不受影响（E5）。

**对照：原著这一段的量级**。五级妖丹一颗近千灵石、六级够一位筑基修士一生修炼（A7）；五年数百颗五级妖丹最少值数十万灵石（A40）——玩家实得 5–10 颗，「数百颗」只在旁白；三级傀儡的辅料让他「一贫如洗」（A49）、七十二剑的辅料让他「倾家荡产」、卖掉仅剩的妖丹（A64）——本章两次 `take` 全部就是这两句；之后各有一笔必收（n20 的 40、n29 的 20），玩家永远有钱买草、付传送。章末他有一件七十二口的本命法宝、一块妙音门腰牌、半幅古修士地图，灵石所剩无几——第 12 章起点「不富」与原著一致。

<!-- 第 11 章「天星城·结丹」施工图 · 第 10–18 节（写手三，2026-10-12）。id、旗标、章号、数字全部照 notes/ch11/research.md；节号节名照 docs/ch10-design.md。 -->

## 10. 引擎前置

### 10.1 清单

**编号对照**（全文只此一套）：**契约 E1–E5** 指 `docs/interfaces-p3-ch11.md` 第 1–5 节（E1 必成配方、E2 `advance_days` 分段验证、E3 困而不杀、E4 只收商店、E5 结丹口径），由引擎路做；本表其余项按谁做分前缀：**C** 内容路 / 协调者、**T** 测试路、**B** 平衡路。本文别处不带「契约」二字的 E1–E30 一律指研究笔记 E 表（16.4 原表）的拍板编号，不是本表。

| # | 项 | 必须 / 可选 | 谁做 | 说明 |
| --- | --- | --- | --- | --- |
| 契约 E1 | **必成配方**：`Recipe::guaranteed`、`kGuaranteedSuccessChance = 100`、`successChance` 早退一句（`Crafting.h/.cpp`）；`RecipeLoader` 读 `guaranteed` 并拒「必成却无 `requireFlag`」；`recipe_qingzhu_fengyun_jian` 写 `"guaranteed": true` | **必须** | 引擎路 | 本章唯一动 `src/` 的事；`successChance` 夹在 [5,95]（`Crafting.cpp` 第 239 行），没有它面板只有 65%、失败折损三根金雷竹即死局 |
| 契约 E2–E5 | 验证项（不改代码）：六次 `advance_days(3600)` 与补足、`killable_by ["水","火"]` 的压到 1 / 要命、空 `entries` 商店与 `sellPrice`、`realm.advance(31)` 顺带抬上限与瓶容量 9、静室 125、炼器台门闸 | **必须** | 引擎路（`tests/Ch11EngineTests.cpp`，契约 9.1） | 契约 2–5 节判据；测试路不碰这个文件 |
| C1 | **开局接法**：patch `scripts/ch10/zhifadui.lua` 末尾——在 `flag.set("ch10.done")` 之后补一行 `teleport("ch11_fangshi")`（只传图 id，落 default spawn `chengmen`；此前一个字不动、不删）；`data/chapters.json` 加 `{"number": 11, ..., "done_flag": "ch11.done"}`；`ui.json` 加 `ui.chapter.11.numeral`「第十一章」、`.title`「结丹」 | **必须** | 内容路（patch 一行）＋ 协调者（两份数据） | 研究笔记 E1、E2；与第 9→10 先例一致；卡上不写「天星城」 |
| T1 | `tests/fixtures/ch10-end-{first,second}.sav` 按 C1 patch 之后重生成：`mapId` 变 `ch11_fangshi`、站在 `chengmen` 那一格；`Ch10AcceptanceTests` 的章节卡断言 `cards.back() == ToBeContinued` 改 `Opening(11)`（第 11 章进 `chapters.json` 后 `cardsForFlagChange` 排的是 Closing(10)＋Opening(11)，`ChapterCardScene.cpp` 第 88–99 行） | **必须** | 测试路 | 笔记 O-10；照第 10 章 16.2 第 2 条做法 |
| C2 | 规则 25 的表：`CHAPTER_MEANS[11]` ＝ 拳、火、土、水、金、木、暗器、剑；`BATTLE_EXTRA_MEANS` 四场各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金，④ 再登友军 `("水","role_magic","magic_zhizhu_tuwang","xueyu_zhizhu_ally")`、`("火","role_magic","magic_kuilei_zihuo","kuilei_yuan")`；`MEANS_LAST_CHAPTER = 11`；**selftest 探针挪到 `wu_chou` ＋ `b12_neidian_duoding`**（`probe["chapter"]` 改 11），措辞改「第 12 章范围探针实际引用乌丑」 | **必须** | 协调者（集成时） | 笔记 E19、D 表「规则 25」「selftest 探针」；探针不挪则本章写实 `b11` 后 selftest 红 |
| C3 | `tests/LexiconTests.cpp` 表一补词（12.1） | **必须** | 协调者 | 放在第 10 章集成之后 |
| T2 | `tests/ObjectiveTests.cpp` 的 `kScriptTransfers` 登 3.1 末表七条（六条三元、回城一条五元）；`tests/NpcPresenceTests.cpp` 的 `kOffChainFlags` 登 `ch11.jiguan`、`ch11.dan_fu`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.lianjian_qian` | **必须** | 测试路 | 笔记 E21；规则 30 用例 |
| C4 | 数据：角色新建 35、就地改写 2（`siji_haishou` 的 `weaknesses`；`material_jinleizhu` 的 `tradeable` 改 false）、一个字不动 3（`qu_hun_shadan`、`wu_chou`、`zi_ling`）；编成 1 改写（`b11_ningcuidao_youyao`）＋ 3 新；配方 1 改写（`recipe_qingzhu_fengyun_jian`，含 `guaranteed true`、去灵石）；**法术新建 2**（友军：`magic_zhizhu_tuwang` 水、`magic_kuilei_zihuo` 火；敌方的刺球光罩、变身、自愈全走 `charge`，不建法术；我方 0 门）；物品新建 13（`material_yaodan_liuji`、`herb_nichang_cao`、`material_tianleizhu`、`material_lianjian_fuliao` ＋ 9 件 story）；商店 4；任务 2；路径行动 14；地图 7（旧图 0 张修补） | **必须** | 内容路 | 清单在笔记 H、I、L、G、K、N.3；物品描述受第 12 节禁词约束 |
| B1 | **结丹初期锚点标定**：平衡路用 `tests/BattleHand.h` 量四场之后，把 `src/core/rules/Realm.h` 注释第 175 行「筑基后期与结丹档仍待对应章节标定」改成「第 11 章标定」；气血 700、攻防 62 / 46 不动 | **必须** | 平衡路 | 笔记 C.2；本章就是标定结丹初期的那一章 |
| T3 | 通关测试 `Ch11Walkthrough`：起点 T1 重生成的两份、终点写 `tests/fixtures/ch11-end-{first,second}.sav`（v9，本章不加存档字段）；`Ch11TriggerModeTests` / `Ch11BattleDataTests` / `Ch11LedgerTests` / `Ch11SliceTests`（**不含 `Ch11EngineTests`**——归引擎路，契约 9.1 / 9.4） | **必须** | 测试路（**另一个代理**） | 第 15 节 |
| C5 | `data/visual/maps.json` 七张图的主题 / 时辰 / 背景（只用已有组合）、`data/visual/battles.json` 四场背景（全 `cliff_top`）；四场开场白写 `data/text/ch11_battle.json`（内容路）；`data/text/battles.json` 里旧占位 `ch11.battle.ningcuidao_youyao.intro` 由协调者删 | **必须** | 内容路 ＋ 协调者（删旧占位） | 笔记 G 表附注 4、I 表附注；8.2 末 |
| C6 | `data/flags.json` 登第 11 节全部旗标（39 条，不含 `ch11.path.*`） | **必须** | 协调者 | 施工方不碰 |
| C7 | 回写：大纲第 11 章人物删「文樯」「木龙真人」、加注「木龙碑 ch409 只在传闻一句」；人物总表 `fan_furen` summary、`zhao_meng_zhanglao` 加注、极阴祖师首见 ch382、天星双圣加注「传闻」；`material_jinleizhu` note 改「带根一节」、`tradeable` 改 false | **必须** | 协调者 | 笔记 B1–B3、B5、B6、B10、B13；E30 |
| O1 | 接 `breakthroughPillBonus`（降尘丹 / 雪灵水加冲关把握） | 可选 | — | **不做**：它仍是返回 0 的桩（`CultivationScene.cpp` 第 363 行）；本章不走面板冲结丹，接了也没人读（E4） |
| O2 | 抬 `kMaxScriptAdvanceDays`（3650） | 可选 | — | **不做**：六十年 ＝ 6 × `advance_days(3600)`；二十余年 ＝ n29 每根 `advance_days(1080)` × 5 ＋ 第六根按 `ch11.kaidian_ri` 补足到 7200（超 3650 分两句）；Lua 分段调用（E5） |
| O3 | 丰乐拍卖行面板（数据目录 ＋ `io/AuctionLoader` ＋ `game/AuctionScene` ＋ Lua `auction(id)`） | 可选 | — | **不做**：门面 ＋ 路径行动一条；`bidStep` 继续只有规则层；留给第 12 / 13 章再议（E9、B22） |
| O4 | Lua 加炼器熟练命令 | 可选 | — | **不做**：必成走契约 E1 的 `guaranteed`（`difficulty 0 / requiredProficiency 0` 本身只到 65%）（E16） |
| O5 | 新法术（剑影分光术、丹火、青竹蜂云剑阵） | 可选 | — | **不做**：起点 11 门够打四场；剑只是兵刃（E17） |
| O6 | 遭遇区做诱妖循环 | 可选 | — | **不做**：`validate.py` 第 2716–2731 行遭遇编成掉落只许灵草，妖丹给不了（E15） |
| O7 | 名牌 realm 显示「？」（天眼术看不透） | 可选 | — | **不做**：`jiyin_fushen` 写 `CoreLate` 只为不越上限，文案说「看不透」（O-9） |
| O8 | 「岁月流逝」新画面 | 可选 | — | **不做**：三段蒙太奇只用 `fade` ＋ `talk`（E5） |
| O9 | 青竹小轩真实库存经营 | 可选 | — | **不做**：柜台触发器按日结算（E22） |
| — | 面板冲结丹、组队机制、通缉状态字段、拍卖面板、续租、大衍诀机制、「每月缴灵石」机制 | **不做** | — | 笔记 A4、A15、A32、A67、C.3、E28 |

**查过、本章不需要新做的**：`realm.advance` 跨大境界一步到位（23 → 31，`RealmAdvance` 直接写目标）且顺带抬 `storyCap`（契约 script 7.2）；`realm.cap` 只升不降；设施 `effectiveness` 属性（第 8 章 E1，门禁下限 100）；`field.unlock(id, slots)`；`bottle.spend / drops`、凝液间隔按境界（结丹后 3 日一滴，`Application.cpp` 第 130 行）；配方 `requireFlag`；facility `require_flag`；商店 `sellRate`、空 `entries`、只收 `tradeable`；任务板 `board` → `BoardScene`；`advance_days` 结算灵田、绿液、`restFor`；多波次、蓄势、破绽、`defeat_is_fatal false` 时 `battle()` 返回 `won=false`、`can_escape`；NPC `visible_flag / hidden_flag`；路径行动 `when / until / reveal`；`today()`；`flag.set(name, n)` 存整数日期（第 10 章 `ch10.kaifu_ri` 先例）；章节卡与「未完待续」。

### 10.2 判据的要点

- C1：第 10 章集成版 `zhifadui.lua` 与 patch 后的 diff 只有末尾一行；headless `Application` 演完它（临时 `.lua` 文件 ＋ 按帧泵到 `isRunning()==false`，照 `Ch02BottleTests`），`mapId == ch11_fangshi`、站在 `chengmen` 那一格（default spawn）、`ch10.done == 1`、章节卡队列是「第十章　终」接「第十一章　结丹」（`cardsForFlagChange` 只在完成的是表里最后一章时才排「未完待续」，`ChapterCardScene.cpp` 第 88–99 行）。**负向**：把那一行挪到 `flag.set` 之前 → 第 10 章验收 20 红。
- C2：`validate.py` 对四张编成「至少一样破绽是本章我方手段打得到的」全过（① 土 ② 金 ③ 剑 ④ 水）；selftest 探针用例把 `wu_chou.json` 临时写成 `weaknesses ["毒"]`、`probe["chapter"]` 改 11 时报、改回不报；**负向**：`MEANS_LAST_CHAPTER` 留 10 → `b11_*` 四张全红。
- T2：`kScriptTransfers` 七条逐条跑脚本到那一行，`mapId` 等于落点图；`kOffChainFlags` 八条都不在 `data/objectives/ch11.json` 的 `done_flag` 序列里、却都在 `data/flags.json` 里。
- B1：`BattleHand` 四场各跑两侧起点，① 意图三到五回合、② 三回合内逃得出、③ 四回合内清场、④ 友军两技能各至少放出一次、气血归零即胜；锚点注释改字之后 `Realm.h` 其余一行不动（diff 核）。
- 契约 E2（引擎路写，`Ch11EngineTests`）：用 `ch11.sangong` 刚置位的存档连调六次 `advance_days(3600)`，`day` 恰加 21600、`field_tianxing_39` 六槽无异常、小瓶 `drops == capacity`、`restFor` 无溢出（O-5）。

---

## 11. 旗标清单（交协调者登记；施工方不碰 `data/flags.json`）

| 旗标 | 值 | 谁置 | 谁读 |
| --- | --- | --- | --- |
| `ch11.chudao` | 1 | chudao.lua | 星宫修士出场；n02 guard；K1 `when` |
| `ch11.zhihuan` | 1 红环（扣 8 中阶）/ 2 蓝环 | zhihuan.lua | 星宫修士撤场、鲁二出场；n03 guard；K2 `when`；K1 `until`；n04 执事的一句 |
| `ch11.luer` | 1 雇 / 2 不雇 | luer.lua | 星宫执事出场；n04 guard；n04 执事多说两句（＝2） |
| `ch11.zufu` | 1 | zufu.lua | 执事撤场；n05 guard |
| `ch11.anju` | 1 | anju.lua | 鲁二撤场；静室、丹房、药园 `require_flag`；n06 guard；K2 `until` |
| `ch11.kuilei` | 1 | kuilei.lua | 白门阁掌柜、丰乐伙计、储物袋铺掌柜出场；n07 guard；K3、K4 `when` |
| `ch11.ziliao` | 1 | ziliao.lua | n08 guard；K5 `when` |
| `ch11.nichang` | 1 | nichang.lua | 坊市杂货 `require_flag`；洞府北口 `require_flag`；星空殿四类 NPC 出场；n09 guard；K10 `when` |
| `ch11.chuansong` | 1 | xingkong.lua（首次） | 妇人一伙、枣红老者撤场；凝翠岛守阵、杂货、范小子出场；n10 guard；K10 `until`；n09 「此后每次 7 中阶」那一支 |
| `ch11.ningcui` | 1 | ningcui.lua | 郝远天出场；东口 `require_flag`；n11 guard；K6–K9 `when`；Z1 `accept` |
| `ch11.buzhen` | 1 | buzhen.lua | `trigger_youyao` guard；`trigger_huicheng` guard（同格换触发器） |
| `ch11.lieyao` | 整数（累加） | youyao.lua | youyao.lua（`== 4` 且 `jiguan == 0` 转 ②）；n14 guard（`>= 3`） |
| `ch11.jiguan` | 1 | youyao.lua（胜 / 逃 / 败都置） | youyao.lua（不再转 ②） |
| `ch11.huicheng` | 1 | huicheng.lua | 凝翠岛杂货掌柜、范小子、郝远天、储物袋铺掌柜撤场（守阵修士到 `ch11.done`）；xingkong.lua「本章不再出海」那一支；n15 guard；K4、K6–K9 `until`；Z1 `fail` |
| `ch11.sangong` | 1 | liandan.lua | n16 guard |
| `ch11.dan_fu` | 0–7（位掩码：降尘丹 1、雪灵水 2） | biguan.lua | biguan.lua 旁白三版 |
| `ch11.jiedan` | 1 | biguan.lua（`realm.advance` 之后） | n17 guard |
| `ch11.chuguan` | 1 | chuguan.lua | n18 guard；E27 外人改与韩立说话 |
| `ch11.zhaolan` | 1 | zhaolan.lua | 结丹邻居 _a / _b、莲儿出场；n19 guard；K12 `when`；Z2 `accept` |
| `ch11.lianer` | 1 | lianer.lua | 莲儿撤场；茶馆门 `require_flag`；范夫人、赵长老（雅间）出场；n20 guard |
| `ch11.chaguan` | 1 | chaguan.lua | 赵长老（雅间）撤场（范夫人雅间对象留到 `ch11.zhanglao`，中段脚本一句打发）；n21 guard；K11 `when` |
| `ch11.zhuzhan` | 1 | zhuzhan.lua | 礁岛七名 NPC 出场（紫灵仙子、范夫人 `_2`、卓如婷 `_2`、赵长老 `_2`、符长老、孟道友、赤火老怪；门主与乌丑在 `guiju`、极阴在 `jiyin`）；n22 guard；K11 `until` |
| `ch11.guiju` | 1 | guiju.lua（胜） | 隐煞门门主、乌丑出场；`trigger_wuchou` guard |
| `ch11.jiyin` | 1 | wuchou.lua | 乌丑撤场、极阴祖师出场（同格换 NPC，互补旗标）；`trigger_daoge` guard |
| `ch11.yaoshi` | 1 | daoge.lua | 礁岛全部 NPC 撤场；紫灵、卓如婷（雅间）出场；n25 guard |
| `ch11.zhanglao` | 1 | zhanglao.lua | 雅间三人撤场；n26 guard |
| `ch11.zhu` | 0–3（1 白 2 蓝 3 金） | peizhu.lua | peizhu.lua |
| `ch11.jinlei` | 1 | peizhu.lua（`zhu == 3`） | 李氏店主出场；n27 guard；peizhu.lua 换 n29 分支 |
| `ch11.kaidian` | 1 | kaidian.lua | 李氏店主撤场；何仙师、邻店店主出场；店门 `require_flag`；n28 guard；K13 `when` |
| `ch11.kaidian_ri` | 整数日期（`today()`） | kaidian.lua（只此一处写） | peizhu.lua（n29 补足到 7200 日） |
| `ch11.jiesuan_ri` | 整数日期 | kaidian.lua（初值）、jiesuan.lua | jiesuan.lua |
| `ch11.jiesuan` | 整数（结算次数） | jiesuan.lua | jiesuan.lua（首次带旁白） |
| `ch11.zhu_gen` | 1–6（n26 `zhu == 3` 时置 1，n29 每根 ＋1） | peizhu.lua | peizhu.lua（`== 6` 收店） |
| `ch11.liugen` | 1 | peizhu.lua | 何仙师、邻店店主撤场（`hidden_flag`）；`jiesuan.lua`、`guitai.lua` 开头判它则一句 return（店门照开，portal 不读 `hidden_flag`）；炼器台 `require_flag`；配方 `requireFlag`；K13 `until` |
| `ch11.lianjian_qian` | 1 | lianjian.lua（n30a 演完） | `trigger_lianjian` 的 `set_flag`（once 兑现）；不在目标链上（`kOffChainFlags`） |
| `ch11.lianjian` | 1 | chujian.lua | 金青出场；n31 guard；K14 `when` |
| `ch11.done` | 1 | zhuzhong.lua | 章末；K 表全部 `until`；第 12 章入口 |
| `ch11.z1` | 0 / 1 接单 / 2 交付 | 收购行脚本（Z1） | Z1 `complete`；任务板条目 |
| `ch11.z2` | 0 / 1 | linju_z2.lua | Z2 `complete`；再按只一句 |
| `ch11.path.<id>` | 1 | 路径行动引擎 | 只由路径行动置（**等 `data/pathactions/ch11.json` 落地后登记**） |

不新增 `story.*`。本表共 **39 条**（不含 `ch11.path.*`）。**目标链之外的在场 / 分支旗标**：`ch11.jiguan`、`ch11.dan_fu`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.lianjian_qian`（登 `kOffChainFlags`，10.1 T2）。**两面日期旗标** `ch11.kaidian_ri`、`ch11.jiesuan_ri` 与 `ch10.kaifu_ri` 同一说明：「整数日期，不是开关；值约一万六千到四万」——`kaidian_ri` 只由 `kaidian.lua` 写、`peizhu.lua` 读；`jiesuan_ri` 由 `kaidian.lua` 给初值、`jiesuan.lua` 读写。开关一律 `flag.set(name)`，整数 `flag.set(name, n)`；`ch11.lieyao`、`ch11.zhu`、`ch11.zhu_gen`、`ch11.jiesuan` 的累加都是「`flag.get` ＋ 1 再 `set`」，脚本里不写别的算法。

---

## 12. 禁词与首见章号

### 12.1 交协调者：`tests/LexiconTests.cpp` 表一要补的词

逐个 `novel.py find` 查过首见（2026-10-12，笔记 M 表）。第 10 章 12.1 已列进表一的（天星宫、妖海、摘星大会、天星双圣、妖兽岛、星空殿、丰乐拍卖行、白门阁、凝翠岛、霓裳草、诱妖草、妙音门、范夫人、紫灵仙子、虚天残图、青竹小轩，以及第 8、9 章的天星城、星宫、内星海、外星海、天雷竹、虚天殿、金雷竹）**不重列**；协调者补表之前按落地的文案再扫一遍 `data/text/ch11*.json`。

**首见落在 ch389-415（第 1–10 章禁、本章放行）**：

| 首见 | 词 |
| --- | --- |
| ch389-393 | 鲁二、红色指环、蓝色指环（390）；圣山、圣宫（391）；传送符（393） |
| ch394-398 | 丰乐盟、碧磷鱼妖、郝远天（394）；红瑚岛、幻形天罗阵（395）；千目妖、鸡冠蛟（396） |
| ch399-404 | 范静梅、莲儿、清河茶馆、天晶木、三级傀儡（399）；三大神木、范左使（401）；卓如婷、右使、汪门主（402）；赤火老怪、元龟岛、葵水魔功（403）；隐煞门、符长老（404） |
| ch405-415 | 附身大法（405）；天都尸火、禁神牌（406）；煞丹分身（407）；妖尸（408）；木龙碑（409）；膏玉、名义长老（410）；白雷竹、金雷竹、辟邪之雷（411）；李氏杂货、何仙师（412）；镇店之宝（413）；金青、白珍珠、古修士遗址（415） |

**首见在 ch415 之后（第 1–11 章都禁）**——第 10 章 12.1 第二张表已列的（辟邪神雷 424、玄骨 428、蛮胡子 432、九曲灵参 433、虚天鼎 434、补天丹、六道极圣 435、啼魂 437、青阳门 464、乾蓝冰珠 494、逆星盟、万三谷 500 及以后）不重列；本章新查的：文思月（430）、元瑶再见（445，「元瑶」本章 0 处）、十八天都尸（409 但韩立不在场，判据③）、元磁神光（408，同前）。

**故意不进表的**（常用字组合）：星宫修士 / 星宫执事（「星宫」进表就够）；红环 / 蓝环（泛称，进表的是「红色指环」「蓝色指环」）；妖丹、五级 / 六级（泛用）；四色（「四色」ch266 早有，金丹四色由 12.2 管）；生死关（ch82 早有）；剑影分光术（ch217 早有）；名义长老、镇店之宝（句式）；茶馆（泛用，进表的是「清河茶馆」）；紫灵（会误伤「紫灵芝」）；左使 / 右使（「范左使」「卓如婷」进表就够）；传音符（ch152 前已有）；白衣修士、妖艳妇人、枣红脸老者、守阵修士、邻居（名牌，不是专名）；**赵长老**（ch69–70 墨府篇已有同名人物，进表会误判第 1–10 章，只留 12.2 的章内先后断言）；**白玉蜘蛛**（398）、**隆兴客栈**（409）——进表一即对本章解禁、LexiconTests 不再拦，而本章要求它们 0 处（回忆 / 改编），所以不进表、由 12.2 专项断言 0 处。
**本章解禁的**：上面第一张表的全部，外加第 8–10 章表里的天星城、天星宫、星宫、内星海、外星海、妖海、天星双圣、摘星大会、妖兽岛、星空殿、丰乐拍卖行、白门阁、凝翠岛、霓裳草、诱妖草、妙音门、范夫人、紫灵仙子、虚天残图、虚天殿、天雷竹、金雷竹、青竹小轩。**仍禁**：第二张表、乾蓝冰焰（**480**；494 是乾蓝冰珠）、银月（615）、紫罗极火（731）、明珠轩（513）、交换会（535）、黄龙（作独立专名；**「黄龙丹」ch27 起即有，须放行**）、猎妖令，外加聂盈、掌天瓶。

### 12.2 规则放行、本章另有专项断言的

**断言的写法**同第 8–10 章：把目标链（3.4 的 29 步 `done_flag` 次序，n13 的 `ch11.jiguan.*`〔若单独起前缀〕编在 `youyao` 之后、n28 的 `ch11.jiesuan.*` 编在 `kaidian` 之后）当成场景次序，给每个文案 key 的场景前缀编号——**前缀取 J 表「谁置」那一列的脚本名**（`ch11.chudao.*`、`ch11.liandan.*`、`ch11.jiesuan.*`……），不取挂点名；Z1 `ch11.z1.*` 编在 `ningcui` 之后、Z2 `ch11.z2.*` 编在 `zhaolan` 之后；路径行动文案 `ch11.path.<id>.*` 按它 `when` 里最晚的那面链上旗标编号；物品描述按 `give` 它的脚本编号；战斗开场白按调用它的脚本编号；地图名按第一次 teleport 进去的脚本编号；门的拒绝语按那扇门所在图的编号。「X 之前不含」＝编号小于 X 的前缀下 0 处。本章没有语言两版（K 表附注 3）。

| 词 | 首见 | 为什么规则管不住 | 本章的断言 |
| --- | --- | --- | --- |
| 天星城、天星宫、星宫、内星海、外星海、妖海、天星双圣、摘星大会、妖兽岛 | ch389 | 表一；章内先后 | 只许 `chudao.*` 起；章节卡 `ui.chapter.11.*` 两条 0 处（E2）；「妖海」只在 `chudao.*` 读典籍那一句与 K1 |
| 红色指环、蓝色指环、「每月缴灵石」 | ch390 | 章内先后 | `chudao.*` 不含；「每月」只在 K1 与 `zhihuan.*`，机制 0 处（B7） |
| 鲁二 | ch390 | 章内先后 | `chudao.*`、`zhihuan.*` 不含（他在 `luer.*` 里自报） |
| 圣山、圣宫、八十一层、三十九层 | ch391 | 章内先后 | `chudao.*`、`zhihuan.*` 不含；「三十九层」在 `zufu.*` 之前只许 K2 提「前五十层」 |
| 白门阁、丰乐拍卖行、储物袋铺 | ch393 | 章内先后 | `kuilei.*` 之前不含 |
| 星空殿、凝翠岛、传送符 | ch393 | 章内先后 | `nichang.*` 之前不含；洞府北口拒绝语 `ch11.dongfu.xingkong_deny` 不含「星空殿」「传送」 |
| 丰乐盟、碧磷鱼妖、郝远天、范小子 | ch394 | 章内先后 | `ningcui.*` 之前不含；`hao_yuantian` 的 `name` 就是「郝远天」（名牌跟 role） |
| 红瑚岛 | ch395 | 章内先后 | `ningcui.*` 之前不含（东口拒绝语也不含） |
| 霓裳草、诱妖草 | ch395 | 章内先后 | `ziliao.*` 之前不含；`herb_nichang_cao` 的描述按 `nichang` 编号 |
| 幻形天罗阵、天风狂烈阵、颠倒五行阵 | ch395 / 373 / 226 | 规则放行 | 只在 `buzhen.*` 一处旁白（不发物品，A35）；`youyao.*` 里只说「阵」 |
| 千目妖 | ch396 | 章内先后 | `ziliao.*` 之前不含（图册里才有它）；K7 可提 |
| 鸡冠蛟 | ch396 | 章内先后 | 「鸡冠蛟」三字在 `youyao.*` 鸡冠蛟那一支与 ② 开场白之外 0 处——K10 枣红老者只说「六级蛟类」；「六级」是泛用词（12.1「故意不进表的」），K8、K10、目标链提示、n01 典籍都可说，不断言 |
| 生死关、散功、筑基初期 | ch82 / — | 规则放行 | `liandan.*` 之前不含；`realm.demote` 0 处（A42） |
| 四色（金丹） | ch398 | 规则放行（「四色」ch266） | 只在 `biguan.*`、`chuguan.*` |
| 二转年数（三十多年 / 十余年） | ch398 | 原著自相矛盾（B16） | 「三十多年」「十余年」作二转年数讲 0 处；只写「数十年」 |
| 剑影分光术 | ch217 | 规则放行 | 至多 `chuguan.*` 一处，只提名（A46） |
| 白玉蜘蛛 | ch398 | 回忆（判据③） | 全章 0 处（本作只有血玉蜘蛛） |
| 妙音门、范静梅、莲儿、清河茶馆 | ch399 | 章内先后 | `lianer.*` 里范静梅自报之前不含；`zhaolan.*` 传音符那一段不含 |
| 三级傀儡、一贫如洗 | ch399 | 章内先后 | `zhaolan.*` 起；「一贫如洗」只此一处 |
| 天晶木 | ch399 | 不进文案 | 全章 0 处（辅料只写「辅料」） |
| 赵长老、范夫人（称谓）、范左使 | 赵长老 **ch69**（墨府篇同名人物，本章这一位 ch400 首见）/ 400 / 401 | 规则放行（赵长老不进表一）；章内先后 | `chaguan.*` 之前不含；`lianer.*` 只叫「范静梅」 |
| 天雷竹、三大神木 | ch401 | 章内先后 | `chaguan.*` 里范夫人说出之前不含；`material_tianleizhu` 描述按 `zhanglao` 编号 |
| 卓如婷、右使、汪门主、劫货 | ch402 | 章内先后 | `zhuzhan.*` 之前不含（`chaguan.*` 末尾「劫货之事」可提劫货，不提汪门主） |
| 天音门 | ch402 | 原著笔误（B20） | 全章 0 处 |
| 赤火老怪、元龟岛、葵水魔功、紫灵仙子、孟姓修士 | ch402-403 | 章内先后 | `zhuzhan.*` 之前不含；本作名牌「孟道友」是改编称谓（原著区间只有「姓孟的结丹期修士」「孟某」，「孟道友」三字全本首见 ch1169），role `origin` 标改编 |
| 隐煞门、符长老 | ch404 | 章内先后 | `zhuzhan.*` 之前不含（礁岛地图名按 `zhuzhan` 编号） |
| 煞妖 | ch311（黑煞教篇「煞妖化身」「煞妖诀」） | 规则放行；章内先后 | `zhuzhan.*` 之前不含（韩立「再熟悉不过」，ch404） |
| 孟长老、赵孟两位长老 | ch405 / 410 | 原著称谓不一（B6） | 全章 0 处；叛的只写「赵长老」「符长老」 |
| 乌丑、极阴祖师、极阴岛 | ch382 | 第 10 章已解禁；章内先后 | `wuchou.*` 之前不含；极阴岛只作乌丑来历 |
| 附身大法、虚天残图、虚天殿、三百年 | ch405 | 第 12 章钩子 | 「虚天残图」「虚天殿」合计恰在 `wuchou.*` 那一句里、其余 0 处（笔记 0 节、A56）；K 表与物品描述 0 处 |
| 天都尸火、禁神牌、封某 | ch406 | 章内先后 | `wuchou.*` 起；「封某」只在旁白一句、不建 role |
| 煞丹分身 | ch407 | 章内先后 | 只在 `daoge.*` 一句（A58） |
| 妖尸、天都尸 | ch408 | 章内先后 | `daoge.*` 与 ④ 开场白之外 0 处；「十八天都尸」0 处 |
| 元磁神光 | ch408 | 暗场（判据③） | 全章 0 处 |
| 隆兴客栈 | ch409 | 改编（B23） | 全章 0 处；图鉴注改编 |
| 木龙碑、汪凝 | ch409 / 403 | 传闻 / 自称 | 各至多 `zhanglao.*` 一处 |
| 膏玉、名义长老、年俸三百、白雷竹、金雷竹、辟邪之雷 | ch410-411 | 章内先后 | `zhanglao.*` 之前不含；「年俸」只在台词、结算 0 处（N.1）；**「蓝雷竹」「蓝雷」原著无此词**（ch411 只说「五千年以上的，雷电则变成了蓝色」）——文案 0 处，中间一阶只写「雷电变蓝」 |
| 倾家荡产 | ch27（本章区间 ch399、411 两处） | 质感 | 至多 `peizhu.*` 一处 |
| 青竹小轩、李氏杂货、何仙师、入世 | ch412 | 章内先后 | `kaidian.*` 之前不含；店门拒绝语不含「青竹小轩」 |
| 镇店之宝、第九个年头 | ch413 | 章内先后 | `jiesuan.*` 起 |
| 七十二、黄道吉日、火柱 | ch413-414 | 章内先后 | `lianjian.*` 起；「七十二」在 `zhanglao.*`、`peizhu.*` 里只许「一套」 |
| 金青、白珍珠、古修士遗址、六连殿（叛乱）、走火入魔 | ch415 | 章内先后 | `zhuzhong.*` 之前不含；「走火入魔」只到谣言（A72） |
| 文樯、木龙真人、天星双圣（人物） | B1、B2、B10 | 不出场 | 文樯、木龙真人 0 处；天星双圣只在典籍、传音符、`zhuzhong.*` 谣言 |
| 明珠轩、交换会、文思月、元瑶、银月、紫罗极火、黄龙、猎妖令、掌天瓶 | — | 表一 / 后章 | **全章 0 处**（接单口径是「悬赏」「订单」）；「黄龙」的断言按词边界放行「黄龙丹」（第 1 章起就有的丹药） |

---

## 13. 文案要求

与前十章同一标准：对 ch389-415 最长公共片段 ≤ 6 字、对全本 ≤ 7 字、≥ 10 字 0 条（汉字剥离、逐条精确最长公共片段、带正负对照）；换词不算重写；key 命名 `ch11.<场景>.<用途>`（场景＝脚本名）；台词 18-60 字为主、不超 80，选项 5-20 字；旁白每段不超 4 句。

**转写高风险点**：

1. **ch389 读典籍**：内外星海、星宫、双圣、猎妖流程、妖丹价——事实照 A1–A8；**典籍原文的讲法与次序全换**，做成他合上玉简之后自己理出来的几条，不是玉简念给玩家听。
2. **ch390 入城**：红蓝环的规矩、八块中阶灵石、滴血认主、结丹免费——事实；守门修士的穿戴、他对城高城阔的那一番比方、鲁二自荐时的神态与开价的说法，自写。
3. **ch391 租府**：三层各几座、两千七百、百年——事实；执事的口吻、三十九层那座洞府的内景描写，不照搬。
4. **ch393 星空殿**：七百一次、七人上限、拼阵付二百与五百——事实；殿的材质与那三个字的颜色（A25）只许提「白石大殿」，妇人一伙的衣饰与神情、执事点数的动作，自写。
5. **ch396 千目妖**：五级、滚圆带刺、数十只绿眼、一顿饭取丹——事实；它入阵的过程与取丹的手法、第二波海兽是本作改编（B11）——**开场白里不许出现它破阵的任何一句**。
6. **ch397-398 结丹**：六十年、黑云闪电、百余里漩涡、全城可感——事实；**天象只许一句旁白**，云电漩涡的整套写法与邻居议论的原话不用；「四色」可用，金丹的大小不写。
7. **ch399 莲儿**：炼气期少女拦路、媚术被拒、范静梅赔罪——事实；少女的年纪与相貌、媚术的手法、范静梅的衣着，一样不用。
8. **ch400-401 茶馆**：不饮茶、压价一成再加半成、一小堆中阶灵石、天雷竹线索——事实；赵长老的相貌、狐媚术试探的那几句、范夫人的笑，自写。
9. **ch405-407 乌丑与极阴**：劫货嫁祸、索残图、三百年一句、附身只发挥三分之一、禁神牌与三分之一元神、封某被焚、赤火倒戈、四结丹是煞丹分身——事实；**韩立退到最后、不交元神，是他唯一的动作**；乌丑的相貌、附身时的光与声、禁神牌的样子、众人的反应——只写「色变」一类。
10. **ch408 妖尸**：不怕法宝、自愈、瞬移、蛛网与紫火困住、遁走、入地一月——事实；它的形貌、十八之数、极阴的来历——0 处；胜利写「困住」。
11. **ch410-411 紫灵真容**：真容、识破化身、两节竹、名义长老与年俸、拒妾拒入门、匕首试竹——事实；她的容貌、地点（隆兴客栈）、三女议事——0 处；B23 改在茶馆雅间。
12. **ch412-415 开店与炼剑**：改名、只卖符与药材、何仙师、六根、三个月炼液、布阵半月、七十二根火柱、两个月出关——事实；店里的陈设、伙计、客人、火柱的高度、剑胚的颜色——自写或不写。
13. **名场面台词不许复刻**：他读完玉简的那一句感慨、鲁二开价与他应「满意给五枚」的那一来一回、执事提「结丹前辈免费」的那一句、枣红老者邀联手与曲魂回绝的那一句、郝远天自报名气的那一句、他第一次看见千目妖时的那个念头、邻居「又有人结丹了」的原话、范静梅赔罪的那一句、范夫人压价的说法、乌丑索残图的那一句、极阴说「三分之一元神」的那一句、紫灵说「以此竹加三百灵石」的那一句、金青送珠时的那一句——**意思保留，说法自己写**；F 表、13.1 为说明意图写的转述同样不许照抄进文案。
14. **暗场口径**：范赵密谋、极阴与赤火分赃、三女议事、乌丑怎么处置孙姓中年人、元磁神光——**一个字都不许替原著补**；玩家知道的只有 `daoge.*` 那一句「四结丹是煞丹分身」。
15. **死亡只写倒下**（E25）：封某「惨叫后再无声息」；煞妖「化为飞灰」可用（原文如此）；孙姓中年人「被困住」；妖尸「被困住」——不写杀死、不写灰烬（人）。
16. **曲魂代言**（E27）：n02–n10 外人先跟曲魂说话，第一次交代一句；n17 起外人直接跟韩立说话，**不再解释为什么**；曲魂的台词是韩立的意思。
17. **不写志向、不写去向**：章末停在金青递珠；古修士遗址只有金青一句；第 12 章的虚天殿只有 `wuchou.*` 那一句；「六级妖丹才有用」只在 `zhuzhong.*` 一句（O）。
18. **数字不进台词**：静室 125、`advance_days` 的天数、收购行 0.8、结算 15 低阶 / 30 日——文案里只写「快了四分之一还多」「六十年」「二十余年」「每月有些进项」这类原著口径的话。
19. **点名可改键一律写占位**（`docs/handoff.md` 第 3 节第 6 条）：文案里提到菜单、快进、存盘、路径行动、WASD、Z/X 这类可改的键，一律写 `{key.<id>}`、不写「按 E 键」；同时写一条 `<id>.pad` 手柄版（只许 `{pad.<id>}`），两条都登进 `tests/KeyBindingTests.cpp` 的 KeyPrompts 白名单（契约 8.2）。**本章预期 0 条**——诱妖、结算、炼剑面板都用既有的键；只含固定键（方向、Enter、Esc）的提示照写。

### 13.1 每段框架件清单（写手与复验共用的判据）

**判据**（沿用第 5 章 13.1）：

- **剧情事实**＝框架件＝删了戏就塌的东西——事实、硬约束、谁在场、落点。下表逐段列全；它们与原著对得上是应当的。专名不算质感。
- **质感边界**＝表外的一切——人数、岁数、长相、手势、道具、次要人物的反应、次要的一句话、谁先谁后——**一律自出**。原著该场的质感项不许出现，**换了词也不行**；把原著的一个动作反过来写也算留着。「质感边界」一栏点名的是最容易被搬进来的那几样，不是全部，**只点名、不复述**。
- 设计语气卡点名的意象可以留，每段至多两处，写法要换。
- 复验：逐段做「原著质感项 → 现文」的并排表，剩下的对应项必须全在「剧情事实」里。

| 段 | 原著 | 剧情事实（删了就塌） | 质感边界（不许出现，换词也不行） | 在场 | 落点 |
| --- | --- | --- | --- | --- | --- |
| n01 初到 | 389-390 | ①一月赶路到天星城；②典籍：内星海 / 外星海（妖海）、星宫管辖、双圣近千年不露面、城规三条、猎妖流程、五级妖丹近千；③远望巨城；④通缉只一句（E28）；⑤曲魂代言的交代一句 | 典籍玉简的售价与铺子；他对城墙高阔的比方；城中人流的形容；双圣传说的细节；读玉简时的神情 | 韩立、曲魂 | `ch11.chudao` |
| n02 指环 | 390 | ①守门修士先与曲魂说话；②曲魂结丹、红环免费；③韩立：红环八中阶 / 蓝环三低阶、灵光七天；④城内无禁空一句（A13） | 守门修士的穿戴与口吻；排队入城的其他修士；指环的样式与光色；滴血认主的动作与说法 | 韩立、曲魂、星宫修士 | `ch11.zhihuan` |
| n03 向导 | 390 | ①凡人鲁二自荐；②两枚 / 五枚只作二选一（雇 / 不雇），数目不进台词的原话；③雇则他补坊市与租赁门道，不雇则 n04 执事多说 | 鲁二的身量与堆笑；他自报身世的话；开价与应价的那一来一回；街市吃食与叫卖 | 韩立、曲魂、鲁二 | `ch11.luer` |
| n04 租府 | 391 | ①圣山八十一层、前五十层对外；②三十一 / 三十九 / 四十七层各几座；③只能选三十九层；④两千七百、百年计；⑤有几块扣几块、零块记账（改编）；⑥禁制令牌 | 山脚楼阁的陈设；执事的相貌与冷淡；上山的方式与沿途景致；洞府内景 | 韩立、曲魂、星宫执事、（鲁二） | `ch11.zufu`；teleport 洞府 |
| n05 安顿 | 391-392 | ①布阵、镶玉密室、虫室放噬金虫；②过三十层修炼快四分之一还多；③药园六槽；④鲁二退场 | 密室的数目与用料；噬金虫「数百只」之外的习性描写；他盘坐试灵气的那一刻 | 韩立、曲魂 | `ch11.anju` |
| n06 傀儡 | 392 | ①半年育虫试验；②闭关两年整造二百余只兽形傀儡；③曲魂炼化混元钵一句（旁白，物品不在背包） | 傀儡的形制与材质；育虫失败的细节（死卵、饲灵丸）；两年里的日常 | 韩立、曲魂 | `ch11.kuilei` |
| n07 资料 | 393 | ①白门阁书铺；②百余灵石复制全部妖兽资料；③赊账一支（改编） | 书铺的陈设与伙计；复制的手法；他翻阅的神情 | 韩立、曲魂、白门阁掌柜 | `ch11.ziliao` |
| n08 霓裳草 | 393-395 | ①读资料一个月悟出霓裳草（十三叶、百年一叶、展叶六七日、诱香）；②药园一滴试验，血玉蜘蛛扑草；③又过半年 | 草的形状与颜色；蜘蛛扑草的动作；他推演的过程 | 韩立 | `ch11.nichang` |
| n09 星空殿 | 393 | ①第五十层白石大殿、成排传送阵、白衣执事；②一次七百、最多七人；③首次拼阵付二百、妇人一伙付五百；④传送符一句；⑤此后每次七中阶、不足搭阵（改编） | 殿名三字的写法与颜色；妇人一伙的衣饰与神情；执事点数的动作；传送时的光 | 韩立、曲魂、星空殿执事、妖艳妇人一伙、枣红老者 | `ch11.chuansong`；teleport 凝翠岛 |
| n10 凝翠岛 | 393-394 | ①传送石屋有守阵修士；②小街四间铺；③枣红老者邀联手，曲魂回绝；④郝远天卖碧磷鱼妖尸给范小子、招揽被婉拒；⑤精细海图一中阶（可不买）；⑥外星海规矩一句（A32） | 石屋与小街的建材（A28）；妖尸的样子；郝远天自报名气的那一句；范小子验货的动作 | 韩立、曲魂、守阵修士、枣红老者、郝远天、范小子、杂货掌柜 | `ch11.ningcui` |
| n11 布阵 | 395 | ①两日巡岛；②三处必经路各一座大阵；③环心小阵；④不发物品（A35） | 三阵的样式与光；岛上的珊瑚与海鸟；他布阵的手法 | 韩立、曲魂 | `ch11.buzhen` |
| n12 诱妖 | 395-396 | ①要草 ≥1、绿液 ≥1；②每次隔三日；③千目妖入阵（五级）；④第二波海兽（改编）；⑤一顿饭取丹 | 草展叶的过程；妖兽循香而来的写法；取丹的手法；傀儡群射的场面 | 韩立、曲魂 | `ch11.lieyao` ＋1；① |
| n13 鸡冠蛟 | 396 | ①草到四百年份引来六级；②可逃、不致命；③此后只催到三百年 | 它的形貌与声；他封草香的动作；它徘徊离去的写法 | 韩立、曲魂 | `ch11.jiguan`；② |
| n14 回城 | 396-397 | ①收手 / 再猎二选一；②五年换岛、数百颗五级妖丹（只在旁白）；③傀儡剩二三十只；④颠倒五行阵弃于七级妖兽；⑤珍稀妖兽材料 | 换过哪几座岛；七级妖兽的样子；傀儡损耗的过程；数十万灵石的算法 | 韩立、曲魂 | `ch11.huicheng`；teleport 洞府 |
| n15 散功 | 397 | ①两年炼丹；②零散卖材料给小商家；③霓裳草移到药园珊瑚礁；④散功半月到筑基初期（旁白）；⑤噬金虫吞草 | 炼了哪些丹；散功的痛感写法；小商家是谁 | 韩立、曲魂 | `ch11.sangong` |
| n16 闭关 | 397-398 | ①降尘丹五、雪灵水二有则服（三版）；②生死关、辟谷；③六十年三段蒙太奇；④噬金虫互噬、产卵一句；⑤天象一句、全城可感；⑥结丹初期 | 云电漩涡的整套写法；邻居议论的原话；金丹的大小；他闭关时的心境独白 | 韩立、（曲魂隔门） | `ch11.jiedan`；结丹初期 |
| n17 出关 | 398 | ①寿元五六百年、丹火一句；②剑诀第七层得剑影分光术（只提名）；③噬金虫数万、壳现金斑；④血玉蜘蛛四级顶峰收入灵兽袋；⑤曲魂煞丹早成、年数只写「数年」 | 容貌仍二十许一类的描写；金斑的样子；蜘蛛的尺寸；他照镜一类的动作；二转年数 | 韩立、曲魂 | `ch11.chuguan` |
| n18 招揽 | 399 | ①七八道传音符：恭贺与招揽，一律回绝；②三级傀儡一击相当筑基中期；③辅料贵到一贫如洗；④猿形傀儡一百余 | 传音符的颜色；招揽者是谁；傀儡的材质与颜色 | 韩立、曲魂、结丹邻居 | `ch11.zhaolan` |
| n19 莲儿 | 399 | ①炼气期少女拦路、媚术被拒；②范静梅现身赔罪；③请去清河茶馆 | 少女的年纪与相貌；媚术的手法；范静梅的衣着与笑 | 韩立、曲魂、莲儿、范静梅 | `ch11.lianer` |
| n20 茶馆 | 400-401 | ①范夫人、赵长老（结丹初期，天眼可见）；②材料换一小堆中阶灵石（必收 40）；③赵长老出手试探、曲魂挡下；④天雷竹为三大神木、一小节下落；⑤劫货之事一句 | 雅间的陈设；赵长老的相貌；范夫人的笑与手势；茶的名目；不饮茶；压价一成再加半成的说法；狐媚术试探的那几句 | 韩立、曲魂、范夫人、赵长老 | `ch11.chaguan` |
| n21 助拳 | 402-403 | ①妙音门底细（左右双使、客卿）；②汪门主战死、押货遇劫；③酬劳＝天雷竹＋另挑一件；④半月赶制傀儡；⑤集合岛十几人、结丹五人；⑥赤火老怪携紫灵仙子；⑦次日清晨突袭 | 集合岛的样子；众人的相貌；赤火老怪的来头之外的描写；紫灵的面纱一类 | 韩立、曲魂、（莲儿带路） | `ch11.zhuzhan`；teleport 礁岛 |
| n22 规矩 | 404 | ①紫灵下令、赤火破阵；②韩立与曲魂圈住四名筑基邪修、化煞妖（编成取 4，B17）；③门主现身质问；④赵长老挑衅反噬、范夫人「中了圈套」 | 阵破的光与声；邪修的惨状；门主的相貌与姓氏 | 韩立、曲魂、礁岛众人、隐煞门门主 | `ch11.guiju`；③ |
| n23 乌丑 | 405-406 | ①乌丑现身、自认劫货嫁祸；②向孙姓门主索虚天残图（疑在其手，未得确认）、虚天殿三百年一现、将近——只此一句，说法自写（ch405 原文「三百年的虚天殿浮出时间可快到了」，照抄即触 ≤ 6 字红线）；③极阴附身、只能发挥三分之一（名牌换 `jiyin_fushen`，台词可称「极阴老祖」）；④禁神牌、三分之一元神；⑤封某被焚（E25）；⑥紫灵问双圣禁令已松；⑦韩立退到最后、不交 | 乌丑的相貌与自称；附身时的光与声；禁神牌的样子；天都尸火的颜色；众人求饶的话 | 韩立、曲魂、礁岛众人、乌丑、极阴祖师 | `ch11.jiyin` |
| n24 倒戈 | 407-408 | ①赤火倒戈困住孙姓中年人；②众人四散；③四结丹是煞丹分身一句；④空间扭曲出妖尸：不怕法宝、自愈、瞬移；⑤蛛网与紫火困住、遁走；⑥入地一月；⑦回府门口收范夫人传音符 | 妖尸的形貌与数目；赤火倒戈的说辞；众人逃散的先后；他入地的手法 | 韩立、曲魂、妖尸、（血玉蜘蛛、猿傀） | `ch11.yaoshi`；④；teleport 洞府 |
| n25 长老 | 409-411 | ①紫灵真容、名牌换「紫灵」；②识破曲魂是化身、与本体同为结丹初期；③妙音门暂迁天星城；④两节竹，带根一节在她手；⑤名义长老、年俸三百、不受令；⑥拒妾、拒入门；⑦竹两寸、小指粗；⑧腰牌 | 她的容貌；隆兴客栈；三女议事；木龙碑之外的镇派之宝描写；匕首试竹的动作与匕首的来历 | 韩立、曲魂、紫灵、范夫人、卓如婷 | `ch11.zhanglao` |
| n26 培竹 | 411 | ①白→蓝→金各一滴；②金雷竹辟邪之雷；③白光小剑化烟一句；④两年凑辅料、倾家荡产、卖掉仅剩妖丹；⑤辅料必给 | 竹变色的过程；小剑的样子；凑辅料跑了哪些铺子 | 韩立、曲魂 | `ch11.zhu`；`ch11.jinlei` |
| n27 开店 | 412 | ①盘下李氏杂货、改名青竹小轩；②只卖符与药材；③易容三十余岁、曲魂扮仆人；④入世心境、炼器与阵法；⑤何仙师炼气三四层 | 铺面的陈设与招牌写法；李氏店主的身世；何仙师的相貌 | 韩立、曲魂、李氏店主、何仙师 | `ch11.kaidian` |
| n28 经营 | 412-413 | ①首次：读绢书、何仙师串门；②第九个年头炼阵旗阵盘；③中级符箓二三十张成一次；④店开二十余年、扮五十余岁；⑤每次结算（E22） | 客人的样子；符箓炼制的过程；镇店之宝的名目之外的描写 | 韩立、曲魂、何仙师、邻店店主 | `ch11.jiesuan` ＋1 |
| n29 六根 | 412-413 | ①三年一根、六根二十年；②幼竹再滴数月；③第六根到万年份即收店；④噬金虫慢、产卵一句；⑤转让店铺 | 收店那一天的场面；何仙师的反应；竹的高度 | 韩立、曲魂 | `ch11.zhu_gen`；`ch11.liugen` |
| n30 炼剑 | 413-415 | ①打坐三日；②六根三个月炼成液；③布阵半月、黄道吉日焚香两日；④面板一次必成（E16）；⑤两月后出关、七十二口入体；⑥二十多年心血 | 七十二根火柱的高度；剑胚的颜色；精血与辅料的先后；出关时的神情 | 韩立、曲魂 | `ch11.lianjian`；面板 |
| n31 竹钟 | 415 | ①噬金虫卵控神一句；②七八个势力一夜叛乱、六连殿在内；③双圣走火入魔——只到谣言；④六级妖丹才有用一句；⑤金青：白珍珠半幅古修士地图、两颗合一、请破阵 | 叛乱的经过；金青的相貌与岁数；珍珠的光；地图的内容 | 韩立、曲魂、金青 | `ch11.done` |
| Z1 订单 | 改编 | ①范小子接单：三颗五级妖丹；②酬中阶十、炼器药二；③`ch11.huicheng` 后过期 | 订单的来头（不说是哪家商盟）；范小子的身世 | 韩立、曲魂、范小子 | `ch11.z1` |
| Z2 邻居 | 399 | ①结丹邻居想要定颜丹；②一颗换三份炼器药；③没丹则一句 | 邻居的相貌与岁数；他要定颜丹的理由 | 韩立、结丹邻居 | `ch11.z2` |

**名牌对照**（名牌跟着 role 走，换名牌是换 role）：紫灵仙子（`zi_ling`，只名牌不上场）→ 紫灵（`zi_ling_zhenrong`，n25 起）；乌丑（`wu_chou`，只说话，hidden `ch11.jiyin`）→ 极阴祖师（`jiyin_fushen`，visible `ch11.jiyin`，n23 附身起，同格换 NPC）；曲魂（身外化身）全章不换；隐煞门门主不写姓；范夫人在坊市雅间（一个对象顶 n20、n25 两场）与礁岛是两个对象名同一 role。

---

## 14. 时长估算

按幕（笔记 N.2），目标 55 min：

| 桶 | 幕 | 项 | 估 |
| --- | --- | --- | --- |
| A | 一·入城 | n01–n05：三段对话（典籍、指环、租府）、鲁二二选一、一次 teleport | 7 min |
| A | 二·猎妖 | n06–n14：两次传送、凝翠岛小街、布阵、诱妖 4–6 轮含 ① ×4、② ×1、收手二选一 | 18 min |
| A | 三·结丹 | n15–n17：蒙太奇为主（三段六次 `advance_days`）、出关 | 6 min |
| A | 四·妙音门 | n18–n25：③ ④ 两场、茶馆 / 礁岛 / 回访三段长对话 | 15 min |
| A | 五·竹与剑 | n26–n31：培竹循环 8 次（3 阶 ＋ 第 2–6 根 5 轮）、店铺两次回访、炼剑面板、金青 | 9 min |
| **A** | | | **≈ 55 min**（目标 55；`tools/audit.py` 落地后重算） |
| C | 可选 | Z1、Z2、路径行动 14 条、收购行卖丹、任务板 | 4-6 min |

四场仗各按 4 min（① 三到五回合、② 三回合内逃或胜、③ 四回合、④ 友军两技能）已含在幕内。**真要压，先压这三处**（都不动框架件）：n21 助拳的旁白（妙音门底细与集合岛两段并成一段，妙音门底细已有 K11 承担）、n25 长老的长对话（两节竹的来历与名义长老的条款合为一轮问答）、n01 初到的典籍旁白（五条压成三条，A3、A8 交给 K1、K5）。诱妖循环玩家若多打几轮、培竹每次「等几日」——那是玩家自己挑的节奏，记在 C 桶。

---

## 15. 验收

每一条都配一条**不经过驱动、直接读数据、判据写死成本文原文**的断言；每一条新检查都要配「故意写坏 → 确实被抓住」的用例。切片一律写明起点存档。

| # | 验收条目 | 直接读的断言 |
| --- | --- | --- |
| 1 | 四道门禁全过（validate / selftest / mapgen / artgen），零新增警告 | 门禁本身 |
| 2 | **一条自动化通关测试**，起点 `tests/fixtures/ch10-end-{first,second}.sav`（**E1 patch 之后重生成的那两份**），落在 `ch11.done`；终点写 `ch11-end-{first,second}.sav`（v9） | `SetUp` 先验 fixture：`ch10.done == 1`、`realm == realmCap == 23`、`formerRealm == 22`、`hp == maxHp == 480`、`mp == maxMp == 320`、`material_lingshi_zhong` 117 / 116、`material_lingshi` 254 / 234、`pill_jiangchen_dan == 5`、`material_xuelingshui == 2`、`pill_zhuji_dan == 13`、`material_yaodan_wuji == 1`、`learnedMagics` 恰 11 门、`party` 恰 `[qu_hun_shadan]`、`bottle.capacity == 6`、`story_shijin_chong == 1`、`story_xueyu_zhizhu == 1`、`story_juhun_bo == 1`（**没有 `story_hunyuan_bo`**，fixture 实读）、`mapId == ch11_fangshi` 且站在 `chengmen` 那一格；**意图玩家写死**（两侧同一组选项，9 节第 10 条）：n02 红环、n03 雇、n07 付中阶、n09 拼阵、n10 买图、① 恰 4 胜、② 恰 1 次且逃、等绿液 0 次、不卖丹、做 Z1 与 Z2、n28 回访恰 2 次；终点先验：`realm == realmCap == 31`、`formerRealm == 22`、`maxHp == rules::realmMaxHp(31)`（700）、`maxMp == rules::realmMaxMp(31)`、`bottle.capacity == 9`、`learnedMagics` 恰起点那 11 门、`party` 恰 `[qu_hun_shadan]`、`pill_jiangchen_dan == 0`、`material_xuelingshui == 0`、`material_yaodan_wuji == 0`（n26 扣光）、`material_yaodan_liuji == 0`（② 逃）、`material_tianleizhu == 0`、`material_jinleizhu == 0`、`material_lianjian_fuliao == 0`、`herb_nichang_cao == 0`（n08 给 3、买 1、胜扣 4）、`weapon_qingzhu_fengyun_jian == 1`、`story_jinzhi_lingpai == 1`、`story_kuilei_qun == 0`、`story_kuilei_can == 1`、`story_yaoshou_ziliao == 1`、`story_yaoshou_cailiao == 0`、`story_kuilei_yuan_qun == 1`、`story_miaoyin_yaopai == 1`、`story_bai_zhenzhu == 1`、`story_shijin_chong == 1`、`story_xueyu_zhizhu == 1`、`field_tianxing_39` 已开 6 槽、`ch11.lieyao == 4`、`ch11.jiguan == 1`、`ch11.zhu == 3`、`ch11.zhu_gen == 6`、`ch11.jiesuan == 2`、`ch11.lianjian_qian == 1`、**`material_lingshi_zhong` 两侧恰 20**（收购行付的是低阶，中阶本就不受卖丹影响；n18、n26 两次扣光把起点差抹平）、**低阶两侧差恰 20**（= 起点差）、**`day` − 起点 `day` 两侧恰 33375**（3.3：固定段 33360 ＋ 诱妖 5 轮 × 3；字面量写死，不从驱动推导）、`mapId == ch11_dongfu` 且站在洞府门外那一格 |
| 3 | 挂点 `mode` / `once` / guard / set_flag 与 3.1 表一致（n21 `interact`、n30a `set_flag ch11.lianjian_qian`、回城阵 `enter` 无 `set_flag`、guard 列只有旗标名）；无 `auto`；`trigger_youyao`、`trigger_peizhu`、`trigger_jiesuan`、`trigger_guitai`、`trigger_huicheng_chuansong`、`npc_xingkong_xiushi` 挂的脚本 `once=false`；同格换触发器的三组（红瑚岛登岸、礁岛三连、炼器台）按旗标互斥（O-7）；`facility_cunji_39` 与 spawn `men` 相邻、n21 触发格不与 spawn 同格 | `Ch11TriggerModeTests` 读七张新图逐条对表 |
| 4 | 四张编成与 I 表一致：① 敌恰 `qianmu_yao` ×1 ＋ `siji_haishou` ×2、`sanji_haishou` 0、`defeat_is_fatal true`、`can_escape true`、`rewards.cultivation > 0`、`drops` 恰 `material_yaodan_wuji` ×1 rate 100、`qianmu_yao.weaknesses` 含「土」；② `jiguan_jiao` ×1、`defeat_is_fatal false`、`can_escape true`、`drops` 恰 `material_yaodan_liuji` ×1、`weaknesses` 含「金」；③ `yinsha_shayao` ×4、`defeat_is_fatal true`、`can_escape false`、`weaknesses` 含「剑」；④ `tiandu_shi` ×1、**`killable_by` 恰 `["水","火"]` 且 ⊆ `weaknesses`**（契约 3.2 第 4 条同句）、`faction ally` 恰 `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2、`weaknesses` 恰 `["水","火"]`、`defeat_is_fatal true`；四场 `hero_absent false`、`spirit_stones == 0`；③④ `drops` 空、`cultivation == 0`；**② 三回合内逃得出、④ 只靠友军两技能也能把它气血打到 0**（平衡路实测，记施工偏差） | `Ch11BattleDataTests` ＋ `BattleHand` |
| 5 | **境界**：`realm.advance` 本章恰 1 处（`biguan.lua`，`realm.CORE_EARLY`，看返回值）；`realm.cap` 0 处；`realm.demote` 0 处（全仓仍恰 1 处）；切片（起点 `ch10-end-first.sav`，走到 `ch11.sangong`）：n16 之前面板 `storyCap == 23`、n16 之后 `realmCap == 31` 且面板下一关写「结丹中期」所需 5000、硬按被 `BreakthroughBlock` 回绝 | 扫 `scripts/` ＋ 切片 |
| 6 | **降尘丹 / 雪灵水有则扣**：`take("pill_jiangchen_dan", 5)`、`take("material_xuelingshui", 2)` 各恰 1 处（`biguan.lua`）；`ch11.dan_fu` 只写位 1、2；切片三版（起点 `ch10-end-first.sav` 改背包）：五颗两滴 → `dan_fu == 3`；零 → 0、旁白换 key、旗标照置；`breakthroughPillBonus` 仍返回 0 | 扫脚本 ＋ 切片 |
| 7 | **六十年**：`biguan.lua` 里 `advance_days(3600)` 恰 6 处、无别的 `advance_days`；`playSecondsGameplay` 在整段蒙太奇里只按真实秒累计（切片：headless 演完 n16，`day` ＋21600、`playSecondsGameplay` 增量 < 5 秒） | 扫脚本 ＋ 切片 |
| 8 | **诱妖循环**：`youyao.lua` 开头判草与绿液；缺草 → 不扣不拨日、旁白指凝翠岛；缺绿液 → `advance_days(5)`、不打；成立 → `bottle.spend(1)`、`advance_days(3)`、`battle("b11_ningcuidao_youyao")`，**`take("herb_nichang_cao", 1)` 在 `battle` 返回之后、只在胜那一支**；`lieyao == 4 ∧ jiguan == 0` → `battle("b11_honghu_jiguanjiao")`、三支都置 `jiguan`；`nichang.lua` 恰一处 `give("herb_nichang_cao", 3)`；切片（起点 `ch10-end-second.sav` 走到 `ch11.buzhen`）：草 0 → `lieyao` 不变；连打五轮 → 第五轮是 ②；① 逃 → 草数不变、`lieyao` 不加、绿液已扣、`day` ＋3；② 逃 → `jiguan == 1`、`material_yaodan_liuji == 0`、草数不变 | 扫脚本 ＋ 切片 |
| 9 | **传送永远走得通**：`xingkong.lua` 首次 `take(..., 2)`、此后 `take(..., 7)`，两处都有不足那一支且仍 `teleport`；`ch11.huicheng >= 1` 时只一句、`return`、不 teleport；切片：中阶 0 块到星空殿 → 落在 `ch11_ningcui_jie`；`huicheng` 后再按 → `mapId` 不变 | 扫脚本 ＋ 切片 |
| 10 | **有几块付几块**：n02、n04、n07、n10、n18、n26、n27 的每一处 `take` 都看返回值、都有走得通的一支；切片（起点 `ch10-end-first.sav` 把中阶改 0）：n02 → `zhihuan == 2`；n04 → `zufu == 1`、`story_jinzhi_lingpai == 1`、执事「记账」那一句；n18 → `story_kuilei_yuan_qun == 1`；n26 → `material_lianjian_fuliao == 1` | 切片 |
| 11 | **培竹与六根**：`peizhu.lua` 两个分支由 `ch11.jinlei` 换；`zhu` 只在 0–3、`zhu_gen` 只在 1–6；`zhu == 3` 时 `take("material_tianleizhu", 1)` ＋ `give("material_jinleizhu", 1)` ＋ `flag.set("ch11.zhu_gen", 1)` 恰各 1 处（**`zhu_gen` 初值 1 由 n26 写**）；n29 每次 `advance_days(1080)` ＋ `give("material_jinleizhu", 1)`、累加 `zhu_gen`，共 5 轮；`zhu_gen == 6` → 补足到 `today() − ch11.kaidian_ri >= 7200`（至少 360）、`give("material_lingshi_zhong", 20)`、置 `liugen`；切片（起点走到 `ch11.kaidian`）：多结算 1000 日与不多结算，`liugen` 之后 `day − kaidian_ri` 都恰 7200 或 7200 ＋ 余数（照 `sanzhuan.lua` 的补足写法核）；`material_jinleizhu == 6`（不是 7）且炼器台此时才开 | 扫脚本 ＋ 切片 |
| 12 | **炼剑走面板**：`facility_lianqi_39` kind `forge`、grade 3、`require_flag == "ch11.liugen"`；`recipe_qingzhu_fengyun_jian`：`productCount 1`、`inputs` 恰金雷竹 6 ＋ 辅料 1（**无灵石**）、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`、**`guaranteed == true`**；本章脚本 0 处 `give("weapon_qingzhu_fengyun_jian")`；`trigger_chujian` 要 `item.count("weapon_qingzhu_fengyun_jian") >= 1`；切片：炼器熟练 0 的存档走 `openFacility` → `AlchemyScene(Forge, 3)` 炼一次 → 必得 1 件、金雷竹 0、低阶灵石不变（契约 1.5 第 6 条） | 读数据 ＋ 扫脚本 ＋ 切片 |
| 13 | **店铺回访结算**：`jiesuan.lua` 每满 30 日给 15 低阶、单次封顶 3600（7200 日）、再写 `jiesuan_ri = today()`、`jiesuan += 1`；首次带旁白、第二次起只结算；切片（起点走到 `ch11.kaidian`）：开店当日按 → 0 灵石、`jiesuan == 1`；拨 90 日再按 → ＋45；拨 9000 日再按 → ＋3600 | 扫脚本 ＋ 切片 |
| 14 | **分身**：`party.add / remove` 本章 0 处；`qu_hun_shadan.json` 与第 10 章终点逐字节相同；`wu_chou.json`、`zi_ling.json` 与第 10 章集成版逐字节相同（selftest 临时副本不算，O-2） | 读数据 ＋ 扫脚本 |
| 15 | **经济账**（N.1）：收购行 `sellRate == 0.8`、`entries` 空；青竹小轩 `sellRate 0.6`、`entries` 空；坊市杂货 `herb_nichang_cao` `stock 10 / restockDays 5 / restockAmount 5 / maxStock 10`、`require_flag ch11.nichang`；凝翠岛杂货 `stock 5 / restockDays 5 / restockAmount 3 / maxStock 5`；五级妖丹在收购行卖 760、六级 4800（字面量，契约 4.2 第 2 条）；`material_jinleizhu.tradeable == false`；两侧通关走同一组选项，章末两侧低阶灵石差等于起点差（20），中阶两侧恰 20 | `Ch11LedgerTests` ＋ 读 `data/shops/`、`data/items/` |
| 16 | **必打 4 场绕不过去**：① ② 由 `youyao.lua` 一处调用，③ `guiju.lua`、④ `daoge.lua` 各一处；③④ 的 `set_flag` 在目标链里且是下一步 guard；通关两侧各记实际打到的编成，恰是 ① ×≥4 ＋ ② ＋ ③ ＋ ④；本章没有别的 `battle(`；遭遇区 0 处 | 读数据 ＋ 扫脚本 |
| 17 | **走不回去的路**：七张新图只有四对门（坊市 ↔ 洞府〔上行 `require_flag ch11.zufu`〕、洞府 ↔ 星空殿〔上行 `require_flag ch11.nichang`〕、凝翠岛 ↔ 红瑚岛、坊市 ↔ 青竹小轩）；礁岛无门；通第 10 章任何图的门 0 扇；切片：`ch11.zufu` 未置时 `portal_shangshan` 拦人、拒绝语 `ch11.fangshi.shangshan_deny`；`ch11.huicheng` 置位后从星空殿到不了凝翠岛；`ch11.liugen` 置位后店门照开、`trigger_guitai` 与 `trigger_jiesuan` 各只说一句（栈顶 `DialogueScene`、不开 `ShopScene`、低阶不变） | 读地图 ＋ 切片 |
| 18 | **NPC 在场链**：G 表每个 NPC「前一段的 hidden 恰是后一段的 visible」；范夫人坊市一个对象（`ch11.lianer → ch11.zhanglao`）＋ 礁岛一个对象同 role、紫灵两 role 首尾相接（`ch11.yaoshi`）；乌丑（hidden `ch11.jiyin`）→ 极阴祖师（visible `ch11.jiyin`）同格换 NPC；同图同 role 的对象 0 对（`tongxing_xiushi` 一个对象；`jiedan_linju` 两对象按 O-4 核） | 规则 30 ＋ `NpcPresenceTests` ＋ 读地图 |
| 19 | **支线 Z1、Z2** 谓词与 N.3 字面量相符：`q11_shougou_dingdan` `accept ch11.ningcui`、`fail ch11.huicheng`、交 3 颗五级妖丹、酬中阶 10 ＋ `material_lianqi_yao` 2、`origin "改编"`；`q11_linju_dingyan` `accept ch11.zhaolan`、`complete ch11.z2`、`linju_z2.lua` 的 `take("pill_dingyan_dan", 1)` 看返回值 | 读 `data/quests/q11_*.json` ＋ 扫脚本 |
| 20 | **路径行动 14 条**：全部 `realm 0`、`until` 含 `ch11.done`、`kind` 全是 `inquire`（`purchase` / `challenge` 0 条——引擎与门禁只认 inquire / purchase / challenge，`PathActionLoader.cpp` 第 82–84 行、`validate.py` 第 2019 行；id 按 `PATH_KIND_WORD` 一律 `<npc>_dating*`）；两条 `reveal` 的类别在那个 role 的 `weaknesses` 里（`qianmu_yao` 土、`jiguan_jiao` 金）；`zaohong_dating_jiguan` 的 `until` 含 `ch11.chuansong`（可错过） | `PathActionTests` ＋ 读 `data/pathactions/ch11.json` |
| 21 | **章末与开局**：`zhuzhong.lua` 最后置 `ch11.done`、不 `ending()`；`zhifadui.lua` 末尾一行 `teleport("ch11_fangshi")`（落 default spawn `chengmen`），与第 10 章集成版 diff 只此一行；`data/chapters.json` 第 11 章是表里最后一章——headless 演完 `zhuzhong.lua`（临时脚本文件 ＋ 泵帧），章节卡队列是「第十一章　终」接「未完待续」；演完 patch 后的 `zhifadui.lua` 则是「第十章　终」接「第十一章　结丹」（`Ch10AcceptanceTests` 第 25 行断言随 T1 改） | 扫脚本 ＋ 真实入口 |
| 22 | **禁词**：表一含 12.1 两张表的词；12.2 的章内先后按 key 前缀逐条断言；「虚天残图」「虚天殿」全章各恰 1 处且都在 `wuchou.*`；分母先验 > 250 条 | `LexiconTests` ＋ 专项 |
| 23 | **暗场与死亡口径**（13 第 14、15 条）逐 key 核；「明珠轩」「交换会」「文思月」「元瑶」「银月」「紫罗极火」「黄龙」「猎妖令」「隆兴客栈」「元磁神光」「十八天都尸」「天音门」「文樯」「木龙真人」「白玉蜘蛛」「天晶木」0 处 | 独立校对 ＋ 12.2 专项 |
| 24 | 独立校对：A 表七十四条硬约束逐条核验；13.1 逐段并排；原创性人工复核（13 第 13 条那十三处） | 校对报告 |
| 25 | 主线节点 31（n01–n31）、**目标链 29 步**（n13、n28 不进链）、可重复循环 3（诱妖、培竹、结算）、分支 ≥ 5（n02、n03、n13、n14、n16 三版）、新地图 7、战斗 4、物品新建 13、旗标 39 | `tools/audit.py` |
| 26 | **引擎**：契约 E1 落地（`Recipe::guaranteed`，契约 1.5 八条）；B1 锚点注释改字；`Ch11EngineTests` 的 21600 日用例（契约 2.2，引擎路写）；`tests/fixtures/*.sav` 与 `saves/*.sav` 在 v9 下全部读得进来 | 第 10 节 |

---

## 16. 拍板记录与开放问题

### 16.1 已拍板（按推荐，预授权——下表 30 条全部按推荐落地）

| # | 拍板 | 本稿怎么落 |
| --- | --- | --- |
| 1 | **开局 patch `scripts/ch10/zhifadui.lua` 末尾一行 `teleport("ch11_fangshi")`**（只传图 id，`chengmen` 标 default）；`chapters.json`、`ui.json` 加第 11 章；`ch10-end` 两份重生成 | 10.1 C1、T1；不在 `ch10_xiaohuan` 挂开局挂点（第 10 章纪律 46） |
| 2 | **章节卡写「第十一章」「结丹」**；设计名「天星城·结丹」保留；「天星城」三字不上卡 | 10.1 C1；12.2 第一行 |
| 3 | **结丹走剧情 `realm.advance(realm.CORE_EARLY)`**，不做面板冲结丹；本章 `realm.advance` 恰 1、`realm.cap` 0、`realm.demote` 0 | 验收 5；笔记 C.3 |
| 4 | **降尘丹 5、雪灵水 2 有则扣、旁白三版**；不接 `breakthroughPillBonus` | 11 节 `ch11.dan_fu`；验收 6；10.1 O1 |
| 5 | **六十年三段蒙太奇、6 × `advance_days(3600)`**；跳过的时间不计游玩时长（`playSecondsGameplay` 只按真实秒） | 验收 7；10.1 O2、O8 |
| 6 | **天星双圣、星宫长老不建 role、不上图** | 12.2「天星双圣（人物）」 |
| 7 | **新图 7 张、旧图 0 张修补、主题时辰全用现有**；坊市一图三区 | G 表；验收 17 |
| 8 | **商行四张商店**：坊市杂货、凝翠岛杂货、凝翠岛收购行（`sellRate 0.8`、只收）、青竹小轩（卖出 0.6） | L.4；验收 15 |
| 9 | **丰乐拍卖行不做面板**：门面 ＋ 路径行动一条（目录与加价两句合在 `fengle_dating_mulu` 一条里） | 10.1 O3；16.3 第 3 条 |
| 10 | **悬赏走任务板一处 ＋ 两条可选支线**；`origin "改编"` 替代猎妖令 | N.3；验收 19 |
| 11 | **三十九层静室 `effectiveness 125`** | 验收 26 之外由 `Ch11TriggerModeTests` 读设施属性 |
| 12 | **新药园 `field.unlock("field_tianxing_39", 6)`**；旧田走不回去 | 验收 2 |
| 13 | **红环 8 中阶、洞府 27 中阶，有几块扣几块、零块记账**；不设门闸 | 验收 10 |
| 14 | **传送费：首次 2 中阶、此后 7 中阶，不足搭阵免费——传送永远走得通** | 验收 9 |
| 15 | **诱妖循环走可重复触发器 ＋ 剧情战**，不设遭遇区；第 5 次转 ② | 验收 8、16 |
| 16 | **七十二剑走炼器面板**，配方改一件、`guaranteed true` 必成（契约 E1）、不要灵石；`weapon_qingzhu_fengyun_jian` 一件代表七十二口 | 验收 12；L.3 |
| 17 | **本章不新学法术**；剑影分光术、血灵钻、丹火不做法术 | L.2；12.2 |
| 18 | **`qu_hun_shadan` 全章在队、数值名牌不动** | 验收 14 |
| 19 | **规则 25 登本章八类 ＋ 四场化身手段 ＋ ④ 两门友军法术；探针挪到 `wu_chou` ＋ `b12_neidian_duoding`** | 10.1 C2 |
| 20 | **词表 M 表交 `LexiconTests`**；明珠轩、交换会、文思月、元瑶 0 处 | 12.1；验收 22、23 |
| 21 | **teleport 登 `kScriptTransfers`（回城阵一条五元、其余三元）、链外旗标八条登 `kOffChainFlags`** | 10.1 T2 |
| 22 | **店铺回访结算**：每满 30 日 15 低阶、封顶 3600；收店尾款 20 中阶 | 验收 13 |
| 23 | **必打 4 场**：① 诱妖（致命、可逃）、② 鸡冠蛟（不致命、可逃、胜为改编）、③ 煞妖 ×4（致命、境界压制）、④ 妖尸（致命、困而不杀、四友军）；余 11 场给第 12–14 章 | I 表；验收 4 |
| 24 | **不演的场**：范赵密谋、极阴赤火分赃、三女议事、凡人店主群像 | 13 第 14 条 |
| 25 | **死亡只写倒下**：封某「再无声息」、煞妖「飞灰」可用、人不写灰烬 | 13 第 15 条 |
| 26 | **支线 2 可选 0 必经** | N.3；16.2 第 8 条 |
| 27 | **曲魂代言到 n17 为止**，之后外人直接与韩立说话；交代一句、不再解释 | 13 第 16 条 |
| 28 | **通缉不设状态字段**；只在 n01 旁白与 K1 一句 | 12.2；10.1「不做」 |
| 29 | **路径行动 14 条、全部 `realm 0`、两条 reveal**（千目妖「土」、鸡冠蛟「金」） | 16.3；验收 20 |
| 30 | **回写由协调者做**（B1–B3、B5、B6、B13 含 `material_jinleizhu` `tradeable false`）；本稿不动仓库 | 10.1 C7 |

### 16.2 与旧拍板的冲突、开放问题（拿不准、需协调者核的）

1. **O-1 战斗 ④「困而不杀」的数据表达——已答**（契约 E3）：引擎胜负只看气血，没有「破两次架势即胜」，也不加；`killable_by` 不许空（`DataLoader.cpp` 第 531–534 行、`validate.py` 第 2435–2439 行都拒），写 `["水","火"]`——只有水、火那一击要得了命、其余类别最多压到 1；`toughness` 低只是容易破势吃 ×2。平衡路把 `tiandu_shi` 气血压到友军两技能各放一次就归零的量级、自愈用 `charge` 表达，`BattleHand` 证明（验收 4 末项）。
2. **O-2 探针挪到 `wu_chou`**：`validate_selftest.py` 第 1813 行把 role 文件临时写成 `weaknesses ["毒"]` 再改回；第 10 章 16.2 第 7 条「`wu_chou` 不动」指提交的数据，测试临时副本不算——协调者确认。
3. **O-3 路径行动 `give` 的返回值**：K12 改挂 interact 脚本 `linju_z2.lua`；若引擎已支持 `take` 条件，可挪回路径行动。
4. **O-4 `jiedan_linju` 两对象同 role**：规则 30 若判其为具名，拆 `jiedan_linju_jia` / `_yi`（验收 18）。
5. **O-5 `advance_days` 六次 3600**：`restFor` 与灵田按年推进在 21600 日内是否溢出——`Ch11EngineTests` 跑一遍（10.2 O2）。
6. **O-6 青竹小轩的卖出口**：卖出口已改成 `trigger_guitai` ＋ `guitai.lua`（`shop(id)` 既有命令），不设 shop facility——facility 不读 `hidden_flag`、收店后关不掉是硬原因；「玩家卖符给自己的店怪不怪」只剩文案问题，若怪则 `guitai.lua` 不调 `shop`、只说一句，验收 15 的青竹小轩 `sellRate` 那半句随之删。
7. **O-7 同格两个 trigger**（红瑚岛登岸、礁岛三连、炼器台三组）：按旗标换触发器是第 10 章 `zhenwei_*` 的写法；validate 若不许同格两个对象则各挪一格（验收 3）。
8. **O-8 支线配额**：本章 2 可选 0 必经；全作必经 14 条的账若要本章出 1 条，Z1 改必经并进目标链（链变 30 步，验收 25 随改）。
9. **O-9 「极阴祖师」的 realm**：写 `CoreLate` 只为不越上限，天眼术会显示「结丹后期」；文案里说「看不透」（10.1 O7）。
10. **O-10 `ch10-end` fixture 重生成**：E1 patch 之后两份 `mapId/position` 变 `ch11_fangshi` 城门格；第 10 章验收 2 末项措辞随改——是那条验收的一处落点改动，不是推翻那条板。
11. **推翻第 10 章 16.1 第 35 条**（「探针改指第 11 章：`siji_haishou` ＋ `b11_ningcuidao_youyao`」，在第 10 章用户预授权的 36 条之内）：**理由**——本章把 `b11_ningcuidao_youyao` 写实了（B11），它不再是「范围外」的占位，探针留在上面 selftest 必红；改指第 12 章 `wu_chou` ＋ `b12_neidian_duoding`（全仓唯一含 `wu_chou` 的编成），`probe["chapter"]` 改 11。与第 9→10 的处理同一个套路；记作推翻、不是否认它是板。
12. **第 10 章 16.2 第 9 条「降尘丹、雪灵水第 11 章定」**：本章定为有则扣、不接加成（16.1 第 4 条）；`breakthroughPillBonus` 桩留给要做面板冲结丹的章。
13. **lore 4.3「每月缴灵石」与 ch391「百年计费」**（B7）：两句原文都有；本作红环一次 800、洞府百年 2700，「每月」只在典籍台词——lore 加注。
14. **B6 赵 / 孟 / 符长老称谓**：原文前后不一；本作只写「赵长老」「符长老」，孟道友不叛——总表加注。
15. **B23 隆兴客栈并进清河茶馆雅间**：省一张图；图鉴注改编。
16. **战斗场数**：第 1–10 章 37 场 ＋ 本章 4 ＝ 41；方案 52 场余 11 给第 12–14 章（`data/battles/` 里 48 个 b 前缀文件含切磋与重复编成，口径以施工图为准）。
17. **时长 ≈ 55 min**（目标 55），真要压先压第 14 节那三处。
18. **`siji_haishou` 改破绽**：全仓仅 `b11_ningcuidao_youyao` 一张编成含它（2026-10-12 grep 过），不牵动第 1–10 章；`sanji_haishou` 从编成去掉后仍在 `data/roles/` 里（别处是否引用由协调者核）。
19. **目标链 29 步——已拍板**（整改时定）：原 F 表 31 步删 n13（`ch11.jiguan`）、n28（`ch11.jiesuan`）两步，两旗登 `kOffChainFlags`。理由：`ObjectiveTests` 对「已是目标链 done_flag 却登进表外」会报，两处只能二选一；n28 还有死链（可不回柜台先培竹六根，`liugen` 后店已盘出、`jiesuan` 永远置不上）。3.4、验收 25、契约 8.3 同此。
20. **研究笔记 id 以本图为准的两处**（笔记不在仓库里，本次无法复核，按审查所述记）：F 表 n28 的脚本 id「jingying」作废，一律 `jiesuan.lua` / `n28_jiesuan`（J 表已是 jiesuan）；F 表 n23 的 role id `jiyin_zushi_fushen` 作废，以 H 表 `jiyin_fushen` 为准。协调者回写笔记时改。
21. **n21 改 `interact`**：原写 `enter` 挂在洞府门一格、与 spawn `men` 同格，`ch11.chaguan` 置位后一进洞府就被送上礁岛、走不到 `facility_cunji_39`；改成门内一格 `interact`、存档点摆在 spawn 与触发格之间（3.1）。
22. **`advance_days` 与 teleport 的写法**：脚本 `teleport` 只传图 id、落 default spawn（`api.lua` 第 63 行不认 spawn 名）；按名落点是引擎项，本章不要。

### 16.3 路径行动条目（施工用；`ch11.path.*` 旗标**等 `data/pathactions/ch11.json` 落地后由协调者登记**）

形状照 `data/pathactions/ch10.json`；文案在 `data/text/ch11_path.json`，key `ch11.path.<id>.text`；每条 `until` 另含 `ch11.done`；**一律 `realm 0`**；`kind` 只认 `inquire / purchase / challenge`（`PathActionLoader.cpp` 第 82–84 行、`docs/interfaces-octo-pathactions.md` 第 62 行；id 按 `PATH_KIND_WORD` 用 `dating / qiugou / qiecuo`）——本章 **14 条全是 `inquire`**，`purchase` 0 条（它必填 `item_id / count / price / deal_key / poor_key`，「不扣钱不给物」「只打探」写不成 purchase）、`challenge` 0 条（城内不准斗法，A4）；本章不分语言两版。`when` 里一律写上链上旗标（12.2 的编号按它排）。

| id | 图 / NPC | kind | when | until | 效果 |
| --- | --- | --- | --- | --- | --- |
| `baiyi_dating_guiju` | 坊市 / `npc_baiyi_xiushi` | inquire | `ch11.chudao >= 1` | `ch11.zhihuan >= 1` | 城规三条（不斗法、每月缴灵石、不干涉）与红蓝环（A4、A9）；通缉一句（E28） |
| `luer_dating_shengshan` | 坊市 / `npc_lu_er` | inquire | `ch11.zhihuan >= 1` | `ch11.anju` | 圣山八十一层、前五十层对外、顶层圣宫（A14） |
| `fengle_dating_mulu` | 坊市 / `npc_fengle_menmian` | inquire | `ch11.kuilei >= 1` | — | 拍卖目录有五六级妖丹、进拍卖行贵一多半（A24）；无效果（E9） |
| `chuwu_dating_dai` | 坊市 / `npc_chuwu_zhanggui` | inquire | `ch11.kuilei >= 1` | `ch11.huicheng` | 大容量储物袋上千灵石——只一句打听，不扣钱、不给物（A23）；无效果字段 |
| `baimen_dating_zhaixing` | 坊市 / `npc_baimen_zhanggui` | inquire | `ch11.ziliao >= 1` | — | 摘星大会百年一次、双圣近千年不露面（A3、A8） |
| `haoyuantian_dating_waihai` | 凝翠岛 / `npc_hao_yuantian` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 外星海规矩：不贴海面、不飞太高、不聚众（A32） |
| `haoyuantian_dating_qianmu` | 凝翠岛 / `npc_hao_yuantian` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | **`reveal` `qianmu_yao` 土**（滚圆带刺、怕被埋住——改编的理由写在文案里） |
| `fanxiaozi_dating_jiage` | 凝翠岛 / `npc_fan_xiaozi` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 五级妖丹近千、六级够一生、收购行是丰乐盟名下（A7、A30） |
| `zahuo_dating_haiyu` | 凝翠岛 / `npc_zahuo_nc` | inquire | `ch11.ningcui >= 1` | `ch11.huicheng` | 海域图要常更新、精细图上面定价（A30）；只做一条，文案不提有无图 |
| `zaohong_dating_jiguan` | 星空殿 / `npc_zaohong_laozhe` | inquire | `ch11.nichang >= 1` | `ch11.chuansong` | **`reveal` `jiguan_jiao` 金**（六级蛟类鳞甲只惧金刃——改编；不出「鸡冠蛟」三字）；**可错过**：n09 首演前才打探得到 |
| `linju_dating_miaoyin` | 洞府 / `npc_jiedan_linju_a` | inquire | `ch11.chaguan >= 1` | `ch11.zhuzhan` | 妙音门底细：女弟子为主、左右双使、客卿长老、狐媚术（ch402） |
| `linju_dating_dingyan` | 洞府 / `npc_jiedan_linju_b` | inquire | `ch11.zhaolan >= 1` | — | 只打探「邻居想要定颜丹」；交换本身挂 `linju_z2.lua`（Z2；O-3） |
| `hexianshi_dating_fanren` | 坊市 / `npc_he_xianshi` | inquire | `ch11.kaidian >= 1` | `ch11.liugen` | 坊市凡人店主的活法、何仙师炼气三四层（A66） |
| `jinqing_dating_luanxing` | 洞府 / `npc_jin_qing` | inquire | `ch11.lianjian >= 1` | — | 七八个势力叛乱、双圣谣言（只到谣言）（A72） |

`npc` 字段是各图上那个 NPC 对象的名字（关卡路起）；`reveal` 的类别必须落在那个 role 的 `weaknesses` 里（门禁查），战斗路若改破绽，两边一起改。③④ 的破绽由战中试探，不设 reveal。

### 16.4 拍板前的原表（保留，供追溯；照笔记 E 表）

| # | 问题 | 推荐 | 不选推荐会怎样 |
| --- | --- | --- | --- |
| E1 | 开局怎么接 | **patch `zhifadui.lua` 末尾一行 teleport**；`chapters.json`、`ui.json` 加行；fixture 重生成 | 在 `ch10_xiaohuan` 挂 guard 触发器：第 10 章图要 patch，违反纪律 46 |
| E2 | 章节卡标题 | **「结丹」** | 写「天星城」会把 n01 的揭示提前 |
| E3 | 结丹怎么上 | **剧情 `realm.advance`** | 面板冲关要 3500 修为、可反复摇骰，与「六十年一关」的重量相悖 |
| E4 | 降尘丹、雪灵水 | **有则扣、不接加成** | 接了要动 `CultivationScene.cpp` 读背包、再登测试；本章不走面板，接了也没人读 |
| E5 | 六十年怎么跳 | **三段蒙太奇、6 × 3600** | 抬 `kMaxScriptAdvanceDays` 是引擎项；新画面是美术项 |
| E6 | 天星双圣、星宫长老 | **不建 role、不上图** | 上图就得给他们台词，原著全程不露面 |
| E7 | 新图张数 | **7 张、旧图 0 修补** | 8 张以上超大纲；少于 7 张星空殿并进洞府，传送费与出海循环混在一张图上 |
| E8 | 商行怎么做 | **四张商店；收购行 `sellRate 0.8`** | 默认 0.5 则一颗五级妖丹只回 475，比传送费还薄，循环没有动机 |
| E9 | 丰乐拍卖行 | **不做面板** | 做面板至少一周引擎工；韩立「买不起」，做出来玩家也只能看 |
| E10 | 悬赏与商行订单 | **任务板一处 ＋ 两条可选支线** | 不做任务板则「悬赏」只剩台词，大纲改编标注落空 |
| E11 | 三十九层静室 | **`effectiveness 125`** | 不写＝100，与「过三十层快四分之一」对不上 |
| E12 | 新药园 | **`field_tianxing_39` 6 槽** | 复用 `field_xiaohuan` 违反契约第 3 节 |
| E13 | 租金与红环 | **有几块扣几块、零块记账、不设门闸** | 设门闸会在 first 侧 117 中阶之外的任何合法花法上死局 |
| E14 | 传送费 | **首次 2、此后 7，不足搭阵免费** | 700 硬门闸 ＋ 一趟只回 1–2 颗丹，玩家可能卡在凝翠岛 |
| E15 | 诱妖循环 | **可重复触发器 ＋ 剧情战，不设遭遇区** | 遭遇编成掉不了妖丹，妖丹经济不成立 |
| E16 | 七十二剑 | **配方改一件、面板必成** | 剧情直接 `give` 则炼器面板全作没用过一次；加 Lua 熟练命令是引擎项 |
| E17 | 本章法术 | **不新学** | 新法术要走 `data/magics` ＋ 规则 25 ＋ 平衡，预算给了结丹锚点 |
| E18 | 分身 | **全章在队、数值名牌不动** | 改它就牵动第 10 章 ④ 执法队的平衡 |
| E19 | 规则 25 与探针 | **登本章八类；探针挪 `wu_chou`** | 探针不挪则本章写实 `b11` 后 selftest 红 |
| E20 | 词表 | **M 表交 `LexiconTests`** | 不登表则第 1–10 章文案里出现「星空殿」「鸡冠蛟」没人拦，首见章口径失守 |
| E21 | 测试登记 | **teleport 登 `kScriptTransfers`、链外旗标登 `kOffChainFlags`** | 规则 30 用例红 |
| E22 | 店铺回访结算 | **柜台触发器按日结算** | 做成真实库存经营要新模块 |
| E23 | 战斗场数 | **4 场** | 5 场以上挤第 12 章虚天殿 |
| E24 | 不演的场 | **四处** | 判据③ |
| E25 | 死亡口径 | **只写倒下** | 沿第 10 章 16.1 第 29 条 |
| E26 | 支线 | **2 可选 0 必经** | 必经支线要进目标链，本章链已 29 步 |
| E27 | 曲魂代言 | **到 n17 为止** | 不限到 n17 则结丹后仍由分身代言，与原著 ch398 起本尊出面（邻居恭贺、妙音门直接找韩立）相悖；全章都代言则紫灵「识破化身」那一句失去分量 |
| E28 | 通缉 | **不设状态字段** | 设字段就要有读它的地方（门闸、台词分支），原著 ch389 之后再没提过通缉，字段会变成永远为 1 的摆设 |
| E29 | 路径行动 | **14 条、`realm 0`、两条 reveal** | 写 `realm > 0` 会把结丹前的打探锁住（起点筑基后期）；不给 reveal 则 ①② 的破绽只能靠战中试探，第一次照面六级没有「逃」以外的解法 |
| E30 | 回写 | **协调者做** | 施工方各自回写大纲 / lore / 总表会撞文件（白名单两两不重叠），也没人核一致 |

---

## 17. 施工纪律（沿用前十章，外加本章的九条）

沿用 `docs/ch04-design.md` 第 8 节、`docs/ch05-design.md` 第 17 节、`docs/ch06-design.md` 第 17 节、`docs/ch07-design.md` 第 17 节、`docs/ch08-design.md` 第 17 节、`docs/ch09-design.md` 第 17 节与 `docs/ch10-design.md` 第 17 节（第 42–49 条）。本章另加：

50. **境界只经一处**：`realm.advance(realm.CORE_EARLY)` 只在 `biguan.lua`、看返回值；`realm.cap` 0 处、`realm.demote` 0 处——散功只写旁白；面板上 `storyCap` 保持 23 直到 n16，n16 之后不许过「结丹中期」。
51. **大跨度时间只用 `advance_days` 分段**：单次不超 3600、不抬 `kMaxScriptAdvanceDays`；六十年恰 6 次、二十余年按 `ch11.kaidian_ri` 补足到 7200（至少 360）；文案里不写天数。
52. **循环触发器自己判状态**：`trigger_youyao`、`trigger_peizhu`、`trigger_jiesuan`、`trigger_guitai`、星空殿与石屋传送阵都 `once=false`，脚本开头先读旗标与物品、每一条分支都有走得通的一支；缺草不扣、缺绿液只拨日、`huicheng` 后只一句、`liugen` 后柜台与结算只一句；`once=true` 的每一个都有 `set_flag`（n30a 是 `ch11.lianjian_qian`）；石屋传送阵脚本 0 处 `flag.set`。
53. **有几块扣几块**：n02、n04、n07、n09、n10、n18、n26、n27 的每一处 `take` 看返回值、零块换一句旁白、旗标照置；本章门闸 0 道花钱的——n26 的辅料不看扣到多少、必给，n08 的三株草必给，配方不要灵石。
53a. **脚本 `teleport` 只传图 id**，落 default spawn（每张图脚本落点那个 spawn 标 `default=True`）；不写 spawn 名、不写坐标。
54. **妖丹只从编成掉**：五级妖丹的来路只有 ① 的 `drops` 与 n14 的 8 颗、六级只有 ② 的 `drops`；脚本里不许别的 `give("material_yaodan_*")`；遭遇区 0 处。
55. **炼剑不剧情给**：`weapon_qingzhu_fengyun_jian` 只由面板产出；`trigger_chujian` 看 `item.count`；配方的 `requireFlag` 与炼器台的 `require_flag` 都是 `ch11.liugen`；配方 `guaranteed true`、`inputs` 只有金雷竹 6 ＋ 辅料 1。
56. **旧图不 patch、旧脚本只补一行**：本章 0 处地图 patch；`scripts/ch10/zhifadui.lua` 只在末尾补一行 teleport；`genmaps_ch05.py`–`genmaps_ch10.py` 一个字不改；`wu_chou.json`、`zi_ling.json`、`qu_hun_shadan.json` 一个字不动。
57. **名字按节点解禁、钩子只许一句**：12.2 就是清单；「虚天残图」「虚天殿」全章各恰 1 处、都在 `wuchou.*`；「隐煞门」在 `zhuzhan.*` 之前 0 处；「鸡冠蛟」只在 `jiguan.*` 与 ② 开场白；名牌跟着 role 走（紫灵仙子 → 紫灵、乌丑 → 极阴祖师是换 role）。
58. **韩立不知道的不写、章末停在递珠**：范赵密谋、分赃、三女议事、乌丑怎么处置孙姓中年人——情报、旁白、路径行动、物品描述都不许写到；`ch11.zhuzhong.*` 的最后一条是金青递珠；不写去向、不 `ending()`；「未完待续」是章节卡的事，文案里 0 处。

## 18. 施工偏差（施工阶段填）

整理自 `notes/ch11/deviations-{A,B,C}.md`（2026-10-12，集成时并入）。每一条都是「本文或契约这么写、落地改成了那样」；没列的就是照抄。两份文档矛盾处以契约 `docs/interfaces-p3-ch11.md` 为准。引擎路的三处契约偏差（2.2-1 探针脚本挂根、4.2-4 买入页判据、5.2-5 静室加成判据）写在契约末尾「实现记录」，这里不重抄。

### 18.1 内容路 A（地图与美术）

1. **同格叠放的三处挂点改成相邻格**（3.1 的 O-7 已许）。引擎 `WorldScene::tryStep` / `interact` 对触发器都走 `map->objectAt(cell, "trigger")`，同一格只取对象表里第一个、不分 `mode`；`validate.py` 规则 16 把同格第二个 `enter` 触发报成「踏不响」。落地：红瑚岛 `trigger_huicheng` 从登岸格 (4,14) 挪到旁边的海面礁石 (2,13)（`interact`、面朝海，`guard ch11.buzhen` 不变）；礁岛 n22/n23/n24 排在 1 格宽的登岛小径 (13,16..19) 上——spawn `haishang` (13,19) → `trigger_guiju` (13,18) → `trigger_wuchou` (13,17) → `trigger_daoge` (13,16)，绕不过；炼器台 (8,18) 只从 (8,17) 够得着，`trigger_lianjian`（enter）就在 (8,17)，到台前必先踩它。
2. **`deny_text_key` 按规则 20 用目标图命名**。规则 20（`expected_deny_key`）要求 `<目标图章号>.block.<目标图短名>`，本文的 `ch11.fangshi.shangshan_deny` / `ch11.dongfu.xingkong_deny` 不用。落地五条：坊市北口 `portal_shangshan`（`ch11.zufu`）→ `ch11.block.dongfu`；洞府北口 `portal_xingkong`（`ch11.nichang`）→ `ch11.block.xingkong`；青竹小轩门 `portal_xiaoxuan`（`ch11.kaidian`）→ `ch11.block.xiaoxuan`；茶馆门 `portal_chaguan`（`ch11.lianer`）→ `ch11.block.fangshi`；凝翠岛东口 `portal_chuhai`（`ch11.ningcui`）→ `ch11.block.honghu`。文案与七条地名 key 同在 `data/text/ch11_maps.json`；`ch11.block.xingkong` 不含「星空殿」「传送」。
3. **茶馆雅间的门是同图一对 portal**。雅间在坊市图里，门闸只能用 portal 的 `require_flag`；规则 13 要求雅间里四个 NPC 从城门走得到，所以进门格 (42,7) `portal_chaguan`（→ 同图 spawn `yajian` (44,8) 朝右）在地形上与雅间连着：BFS 走得通、踩上去被 `require_flag ch11.lianer` 拦住。出门是雅间内侧 (43,7) `portal_chaguan_chu` → 同图 spawn `chaguan_men` (40,10) 朝左。坊市因此比 3.1 的 spawn 表多两个 spawn（`yajian`、`chaguan_men`）。
4. **`trigger_chujian` 加 `guard_flag ch11.liugen`**。3.1 n30b 的 guard 列写的是脚本判 `item.count("weapon_qingzhu_fengyun_jian") >= 1`；剑只能在 `facility_lianqi_39`（`require_flag ch11.liugen`）炼出，加这道 guard 不改语义，只免得 `ch11.liugen` 之前每次踏过密室门口 (10,15) 都空跑一次脚本。
5. **李氏杂货与青竹小轩是同一栋铺面**：街西 (17..23, 23..26) 一栋，门 (20,26) 是 `portal_xiaoxuan`（`require_flag ch11.kaidian`），`npc_li_dianzhu` 站在门旁 (21,27)（`ch11.jinlei → ch11.kaidian`）——盘店之前店主在门口、门不开；盘下之后店主退场、门开。
6. **规则 30：`npc_jiedan_linju_a` / `_b` 同图同 role**（O-4）。两人要同时在场（`_a` 挂路径行动 `linju_miaoyin_dating`、`_b` 挂 Z2 `linju_z2.lua`），写不成互补旗标；`validate.py` 唯一一条落在本路文件上的错。**集成裁决见 18.4 第 3 条**（按 16.1 第 4 条拆 role）。
7. **煞妖变身态要接 `BATTLE_OVERRIDES`**。`yinsha_shayao` 按第 5 节给了人形外观（`looks.json` `ch11_yinsha_shayao`，arch_demonic_m）；变身态画在 `sprites_enemies.py`（`enemies/yinsha_shayao_bianshen.png`）。运行时取图先查 `roles`，所以战斗 ③ 现在画人形；要在战斗里换成变身态，`tools/artgen/sprites.py` 的 `BATTLE_OVERRIDES` 要加 `"b11_yinsha_shayao": {"yinsha_shayao": "yinsha_shayao_bianshen"}`（白名单外，待办）。另：非人形敌人图一律面朝右、不翻转（`docs/art-sprites.md` §4）；友军 `xueyu_zhizhu_ally` 站在右侧会背对敌人，引擎若不给友军兽图翻转，这是引擎路的一处小事。
8. **其它**：`region` 一律 `luanxinghai_zhongbu`；洞府、星空殿、青竹小轩 `outdoor false`，其余 true。七张图的 BGM 全是旧曲：`catalog.MAP_BGM` 元组本就含这五个 id，只加了七行登记注释，`docs/audio.md` 曲目表「用在哪里」列补了七张图。`looks.json` 新加 31 条外观（全部 `base` 原型 + 配色）与 32 条 `roles`（含 `kuilei_yuan → arch_puppet`）；`siji_haishou` 本就没有外观（运行时退回旧画法），没动。`sprite_preview.py --coverage`：缺图 0。

### 18.2 内容路 B（数据）

1. **路径行动 id 改形**（门禁 vs 16.3）。`validate.py` 的 `PATH_ID_RE` 要求 id 以 `_dating[序号]` 结尾，16.3 的 `<npc>_dating_<话题>` 全部过不了门禁；落地改成 `<npc>_<话题>_dating`，话题词一个不丢：`baiyi_guiju_dating`、`luer_shengshan_dating`、`fengle_mulu_dating`、`chuwu_dai_dating`、`baimen_zhaixing_dating`、`haoyuantian_waihai_dating`、`haoyuantian_qianmu_dating`、`fanxiaozi_jiage_dating`、`zahuo_haiyu_dating`、`zaohong_jiguan_dating`、`linju_miaoyin_dating`、`linju_dingyan_dating`、`hexianshi_fanren_dating`、`jinqing_luanxing_dating`。`done_flag` 是 `ch11.path.<id>`、text_key 是 `ch11.path.<id>.text`；`data/flags.json` 按这 14 个登；本文 12.2 / 16.3 引用旧名的地方以此为准。
2. **路径行动 `when` 多写了在场旗标**（门禁规则 26 要求 `when` 里写上那个 NPC 的 `visible_flag`，目标链先后它不推）。四条各加一条谓词、窗口实际不变：`baimen_zhaixing_dating` 加 `ch11.kuilei >= 1`；`fanxiaozi_jiage_dating`、`zahuo_haiyu_dating` 加 `ch11.chuansong >= 1`；`linju_miaoyin_dating` 加 `ch11.zhaolan >= 1`。
3. **郝远天两条打探的时段**（规则 26「同一 NPC 同为 inquire 时段须证明不重叠」）。本文两条都是 `when ch11.ningcui`、`until ch11.huicheng`；落地 `haoyuantian_waihai_dating` 的 `until` 另含自己的 `done_flag`，`haoyuantian_qianmu_dating` 的 `when` 另含 `ch11.path.haoyuantian_waihai_dating >= 1`——先问外海规矩，再问千目妖。reveal 不变。
4. **紫灵真容（`zi_ling_zhenrong`）的数值**。第 5 节写「筑基后期」「数值同 `zi_ling` 口径」，而 `zi_ling.json` 本身是结丹初期 700 / 61 / 48；落地按筑基后期取 `Realm.h` 锚点（480 / 44 / 30），名牌「紫灵」，不动 `zi_ling.json`。
5. **商店多加的条目**。本文只定了霓裳草；坊市杂货另列 `material_fuzhi`、`material_dansha`（照 `ch10_tiandujie_pu` 的货），凝翠岛杂货另列 `material_fuzhi`；不卖丹药（B8）、不卖妖丹。收购行与青竹小轩 `entries: []`。
6. **文案里给脚本路备的 key**（本文没点名）。`data/text/ch11_battle.json` 除四条 `.intro` 与四条 `.charge`（`ch11.battle.{qianmu,jiguan,shayao,tiandu}.charge`，role 的 `charge.text_key` 引着）外，另写了胜负文案 `ch11.battle.ningcuidao_youyao.{win,escape,lose}`、`ch11.battle.honghu_jiguanjiao.{win,away}`、`ch11.battle.yinsha_shayao.{win,lose}`、`ch11.battle.tiandu_shi.{win,lose}`（胜写「困住」）。商店名 key `ch11.shop.name.{fangshi_zahuo,ningcui_zahuo,ningcui_shougou,qingzhu_xiaoxuan}` 放在 `ch11_quests.json`、法术描述 `magic.desc.magic_{zhizhu_tuwang,kuilei_zihuo}` 放在 `ch11_items.json`（都照第 10 章）。
7. **初版战斗数值**（归平衡路，这里只记落地值；气血 / 攻 / 防 · 架势 · 破绽 · 行动 · 蓄势）：`qianmu_yao` 结丹初期 760 / 64 / 44 · 6 · 土、剑 · 1 · every 3 ×1.8 全体；`siji_haishou`（只改 weaknesses）筑基中期 428 / 52 / 36 不动 · 3 · 土、暗器 · 1；`jiguan_jiao` 结丹中期 1000 / 80 / 60 · 8 · 金 · 2 · every 2 ×2.2 全体；`yinsha_shayao` 筑基后期 470 / 46 / 32 · 3 · 剑、木 · 1 · every 2 ×1.7 全体；`tiandu_shi` 结丹初期 1600 / 70 / 50 · 4 · 水、火（`killable_by` 同）· 2 · every 2 ×1.5 单体；友军 `xueyu_zhizhu_ally` 筑基后期 460 / 42 / 30 · 法术 `magic_zhizhu_tuwang`（水 34）；`kuilei_yuan` 筑基中期 380 / 38 / 30 · 法术 `magic_kuilei_zihuo`（火 36）。① `rewards.cultivation 120`（本文只说 > 0）。
8. 引擎 E1 落地前本路没起构建，`RecipeLoader` 读 `qingzhu_fengyun_jian.json` 是否拒收未知字段没有编译核——集成时 `Ch11EngineTests` 28/28 绿，已核。

### 18.3 内容路 C（脚本与主线文案）

1. **`field.unlock` 没看返回值**（3.2 n05「unlock 查返回值」）。`scripts/common/api.lua` 的 `field.unlock` 只 emit、不返回任何值（与第 10 章 `lifu.lua` 同一写法），无从核对；`anju.lua` 照契约第 7 节写了一句 `field.unlock("field_tianxing_39", 6)`。要核得改 api.lua（引擎项）。
2. **n16 `realm.advance` 失败的早退**。契约要求「占位一句 `return`、不置旗」；`biguan.lua` 在 `return` 前补了一句 `fade.in_()`——不补，失败时屏幕停在黑场。降尘丹 / 雪灵水在那之前已扣、`ch11.dan_fu` 已写；重进时按当时背包重算（多半是 0 → 「none」那一版旁白）。
3. **n29 先于 n27 的边界**。`trigger_peizhu` 的地图 guard 是 `ch11.zhanglao`，玩家可以不盘店先催满六根：此时 `ch11.kaidian_ri` 为 0，补足式 `max(360, 7200 - (today() - 0))` 退化成 360 日（不会崩），但 `peizhu.close`「店开了二十余年」会在没开过店时出现，且 `ch11.liugen` 已置、之后再盘店柜台只说「店已盘出」。目标链 26 → 27 的顺序由提示引导，没加门闸（本文没要求）。
4. **n12 battle 返回值**。① 用 `won, how` 两个返回值——`how == "escaped"` 才算逃，其余非胜一律 `game_over()`（含理论上的 `enemy_fled`）；② 只看 `won`：逃与败同一句旁白，`ch11.jiguan` 在 battle 返回后立刻置，再处理扣草，免得扣草失败时漏置旗。
5. **Z1 多加一道前置**。`npc_fan_xiaozi` 从 `ch11.chuansong` 起在场而支线 `accept` 是 `ch11.ningcui`，脚本开头 `ch11.ningcui == 0` 时一句「忙着验货」`return`，不接单。
6. **n13 文案前缀用 `ch11.jiguan.*`**（12.2「若单独起前缀」那一支）：纪律 57「鸡冠蛟只在 `jiguan.*` 与 ② 开场白」靠它成立；`youyao.*` 里 0 处「鸡冠蛟」。
7. **「二十余年」与「十余年」的子串冲突**。`peizhu.close` 含「二十余年」（3.3、第 13 节第 18 条的口径）；12.2 断言「十余年」0 处若按子串扫会命中它，那一条管的是二转年数，本文案 0 处把它作年数讲。写断言请按词边界或排除「二十余年」。
8. **范夫人的名牌**。n19 `lianer.lua` 用 `fan_furen` 做说话人（13.1 n19 在场表有她），屏上名牌是 role 的「范夫人」，而 12.2 要求 `lianer.*` 文案只叫「范静梅」——文案已照办，名牌跟 role 走不是文案能改的。
9. 其余门闸与退路一律照 N.1 退路列落地（指环 take 中阶 8 → =1 否则低阶 3 → =2 再不成「结丹作保」仍 =2；租府 `min(中阶,27)` 有几扣几、三句；星空殿首次 take 2 否则「对方全出」、以后 take 7 否则「搭阵」；诱妖缺草 return 指杂货、缺绿液 +5 日 return；闭关 6×`advance_days(3600)`；培竹第六根补足 `max(360, 7200-(today-kaidian_ri))`、>3650 拆两句；茶馆材料缺仍 give 40；`huicheng_chuansong` 0 处 `flag.set`、顶格 `teleport("ch11_xingkong")`）。34 个脚本全部过 `luac -p`；文案五份 316 条、7361 汉字，脚本引用与文案一一对应；禁词与首见次序扫描 0 处违例。

### 18.4 集成（协调者，2026-10-12）

1. **规则 25 的表只登写死的友军，不登曲魂**（偏离 8.2 与 10.1 C2「四场各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金」）。`validate.py` 的 `role_means_available` 只认编成里 `faction == "ally"` 的单位；本章四场 `hero_absent false`，曲魂是队员、由 `BattleScene::addPartyUnits` 带进战斗，编成里没有它这一行——照 `b10_zhifadui` 写法登上去，来源检查当场 16 条红。与第 10 章 `b10_gu_zhanglao` / `b10_gujia_bidou` 两场（在队、不登）同一口径；它的 剑 / 火 / 金 第 11 章通表本来就有，四场「至少一样破绽打得到」（① 土 ② 金 ③ 剑 ④ 水）不受影响。`CHAPTER_MEANS[11]` 八项、`b11_tiandu_shi` 两条友军 `role_magic`、`MEANS_LAST_CHAPTER = 11` 照登；要让验证器认队员得改 `role_means_available`（白名单外，待办）。
2. **selftest 探针**两处都挪：第 1801–1828 行那组（`wu_chou` + `b12_neidian_duoding`、措辞「第 12 章范围探针实际引用乌丑」、`probe["chapter"] = 11`）与第 2502–2527 行那组 `future_id` / `scoped(12)` 不报、`scoped(11)` 必报（原来那组指 `b11_ningcuidao_youyao`，`MEANS_LAST_CHAPTER = 11` 后它就在范围内、必红）；第 2465 行断言改末章 11。
3. **O-4 拆 role**（16.1 第 4 条）：`jiedan_linju` 拆成 `jiedan_linju_jia`（名牌「左邻结丹修士」，`npc_jiedan_linju_a`）与 `jiedan_linju_yi`（「右舍结丹修士」，`npc_jiedan_linju_b`），两个 role 共用外观 `ch11_jiedan_linju`（`looks.json` `roles` 两行）；`genmaps_ch11.py` 两行改 role、`ch11_dongfu.tmj` 重生成；`linju_z2.lua` 四句 `talk` 说话人改 `jiedan_linju_yi`；`data/roles/jiedan_linju.json` 删除；美术全量重烘焙（`index.json` 多两条映射，图不变）。规则 30 不许名牌同名，所以两个名牌不能都叫「结丹邻居」；第 5 节人物表 `jiedan_linju` 那一行以此为准。
4. `data/text/battles.json` 旧占位 `ch11.battle.ningcuidao_youyao.intro` 已删（22:11）；`docs/handoff.md` 第 4 节拍卖那一行改成「本章只做门面 + 路径行动，面板留给第 12 / 13 章」。
5. **两处水面上的 building**（`MapArtTests.TheShippedMapsUseGid3BothAsWaterAndAsWalkableGround` 钉「水面 gid 3 只挡路、不画 building」）：红瑚岛 `trigger_huicheng` 挪到海面礁石 (2,13) 后，`event()` 的 interact 写法在那一格画 building 8——生成器先给那一格铺礁道 `floor(g, 2, 13, 2, 13, 2)`；凝翠岛小街街北第四间铺面 `shop_block(25,5,28,8)` 的东墙列 x=28 在岛（到 x=27）之外——补一列岸 `floor(g, 28, 3, 28, 9)`（不扩到 y ≥ 12，小船贴花在 (28,13)）。两张图重生成、重烘焙。
6. **茶馆雅间的同图 portal 对过不了 `MapLinkWalk`**（18.1 第 3 条）：`HoldingTheKeyThroughEveryPortalNeverBouncesBack` 进门后按住同一方向再走，只要走得动且当前图仍是出发图就判「来回弹」——同图传送的落点 `yajian` (44,8) / `chaguan_men` (40,10) 四邻都是地板，任一进门方向都红；「两头按住进门方向都撞墙」的核法与用例口径不同。改法要么把雅间做成单独一张小图（新图 + genmaps + 美术 + 文案 key），要么用例对 `target_map == 本图` 的门改钉「落点 ≠ 门格」——都超出合缝，**待办**（地图路 / 测试路共同定）。
7. **一人两地**（`NpcPresenceShipped.EveryNamedRoleStandsOnAtMostOneMapAtEveryStepOfTheMainLine`：具名 role 在时间线每一步最多在一张图）：① `he_xianshi` 按第 3 节「待协调者核的三处」②——去掉坊市街上的 `npc_he_xianshi`，打探 `hexianshi_fanren_dating` 挪到店内对象（`map ch11_xiaoxuan`、`npc npc_he_xianshi_2`）；② `fan_furen` 雅间对象 G 表写 `ch11.lianer → ch11.zhanglao`、礁岛对象 `ch11.zhuzhan → ch11.yaoshi`，n21–n23 三步两地同在——雅间对象的 hidden 改 `ch11.zhuzhan`（她赴礁岛就退场）；n25 她只作说话人（`zhanglao.lua` 的 `talk("fan_furen", …)` 走 role 名牌，不要对象），第 5 节 `fan_furen` 那一行「一个对象顶 n20、n25 两场」以此为准；`chaguan.lua` 开头 `ch11.yaoshi` 那一支从此走不到（无害）。再加第三个对象 `ch11.yaoshi → ch11.zhanglao` 过不了规则 30（它只认 hidden 恰等于 visible 的接班），没加。
8. **打探字数**（`PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters`：每条 40–150 汉字，标点不计）：`chuwu_dai`、`haoyuantian_waihai`、`zahuo_haiyu`、`zaohong_jiguan`、`linju_dingyan`、`jinqing_luanxing` 六条原 35–39 字，各补一短句 / 两三字到 40–47 字，意思不变（`data/text/ch11_path.json`）。
9. **待办（白名单外）**：测试路 T1 / T2 与契约 8.3 那张表（`ch10-end-*.sav` 重生成、`Ch10AcceptanceTests` 章节卡断言改 `Opening(11)`、`kScriptTransfers` 七条、`kOffChainFlags` 八条、`NpcPresenceTests` 的 `kLastChapter`、`PathActionTests.cpp` `realmCapOfChapter()` 加 `{11, CoreEarly}`、`ChapterCards` / `ChapterTableData` 钉末章 10 的用例、`tests/Ch07BattleDataTests.cpp` 第 318 行 `MEANS_LAST_CHAPTER` 的放行清单加 11——终证构建里这 13 条红全在此列）；美术 `BATTLE_OVERRIDES` 煞妖变身态（18.1 第 7 条）；`validate.py` `role_means_available` 认队员（第 1 条）；`api.lua` `field.unlock` 返回值（18.3 第 1 条）。

### 18.5 整改记录（整改路，2026-10-13；对应 `docs/ch11-review.md`、`docs/ch11-test-review.md`）

**内容（C-1 至 C-12）**

1. **机器口径（C-1）**：点名的 13 条与 C-2 的 27 条一并重写，事实不动。实量（第 13 节口径：只比汉字、逐条精确最长公共片段；探针嵌原著 12 字报 12）：全章 415 条对 ch389–415 的分布是 2 字 47、3 字 151、4 字 128、5 字 67、6 字 22，超 6 字 0 条；对全本超 7 字 0 条；≥ 10 字 0 条。
2. **换词不换骨（C-2）**：n01 典籍改成他按「能不能住 → 钱从哪来 → 谁说了算」自理的三条，「内外星海 / 妖海」放到最后作补记（`chudao.lua` 四句的调用次序随之改为 rule、hunt、gong、sea）；n02 守门修士先认出结丹、直接递红环，再问同行那位；n03 鲁二先说一句熟路人才知道的行情，不报身世、不报价；n04 分层改由执事指图说出、令牌只写交到曲魂手里；n09 妇人一伙报空位、开二百；n10 郝远天不说「面生」；n13 去掉封香、徘徊；n16 天象改成从坊市望去的一句、第三十年换内容；n17 出关从拂灰写起，`chuguan.lvhuang` 连同 `chuguan.lua` 那一句删去；n19 范静梅先喝退徒弟、名字放进邀约；n20 不写「推」、改成递灵石袋；n22 删推理句；n23 见下条；n24 改成「拖它多久」的估算；n25 紫灵从气息同源说起；n30 删「依次」与先后。
3. **虚天残图（C-3）**：`wuchou.map` 删「半幅」、先说虚天殿三百年一浮、再问残图还在不在他手里（两词仍同在这一句）；`wuchou.deny` 改成不认不否的反问，不出「残图」。
4. **金青（C-4）**：另一颗珍珠在与他同去的一位道友手上，「古修士」「遗址」改成金青的猜测；`zhuzhong.visitor` 改「相熟的邻居」；白珍珠描述改成珠中半幅地图、两颗合一。
5. **韩立不当众抗命（C-5）**：`wuchou.no` 改旁白（说话人留空），写他不动、不出声、暗记遁路。
6. **框架件（C-6）**：新增 `ch11.wuchou.ban_old`（极阴附身答禁令是早年的事，`wuchou.lua` 紧接 `ban` 一句）；`zhanglao.fan` 改由紫灵（`zi_ling_zhenrong`）说出赠一名女弟子为妾，`zhanglao.refuse` 回绝「人也不必送」；新增 `ch11.peizhu.sword`（金色那一滴之后小剑一探即散成烟）。
7. **先盘店再催第二根（C-7，即 18.3 第 3 条）**：`peizhu.lua` n29 支开头加 `if flag.get("ch11.kaidian") == 0 then talk("", "ch11.peizhu.shop_first"); return end`，在扣绿液、拨日之前；新 key `ch11.peizhu.shop_first`。切片 `Ch11Slice.ElevenNoSecondRootGrowsBeforeTheShopIsBought` 钉住（含对照），扫描另加两条与两条改坏负向。
8. **小处（C-8 至 C-12）**：「三成」改「三分之一」；星空殿那一伙改为五人（含枣红老者），加韩立、曲魂正好七人；`jiesuan.ninth` 的镇店之宝还给隔壁；`jinqing_luanxing` 改「一夜之间」；摘星大会删「都是后辈」；猿傀「百余只」；`chudao.road` 去掉方向；`chaguan.lua` 的 `ch11.yaoshi` 死分支与 `ch11.chaguan.ziling` 删去；目标提示 n04 不再提三十九层与百年、n16 改「有就带上」、n21 去掉「存个档」（三条偏离 3.4 R-3 列原文，以本条为准）；`anju.luer_go`、`kaidian.short` 不再写收没收灵石；`zhihuan.blue` 删七日悬线（`zhihuan.owe` 同改「日后补上」）；② 开场白删「早了一百年」、① 开场白改「曲魂已出剑」。
9. **未改、交协调者**：`npc_jin_qing` 的 `hidden_flag ch11.done` 与契约第 6 节「金青留在门外」不一致、`zhuzhong.again` 因此走不到——删 hidden 牵动第 12 章在场链，本路不动，待协调者定。

**测试（T-1 至 T-6）**

10. **12.2 专项（T-1）**：新增 `tests/Ch11LexiconTests.cpp`。节点按 3.1 的 n 号乘 2 编（脚本里演的记 2n，n 做完之后才露出的记 2n+1），覆盖章内先后 76 个词（含正向「头一回在哪一节」）、定点 15 条（鸡冠蛟、四色、三阵名、妖尸、煞丹分身、一贫如洗、每月、两个虚天词各恰 1 处……）、全章 0 处 24 个（黄龙放行黄龙丹、十余年放行二十余年）、虚天两词同一句、洞府北口拒绝语不提星空殿 / 传送、章节卡两条、分母 > 250 且 11 份文件；每类配注入负向（含 C-11、C-12 两处旧文回放）。**坊市地名「天星城坊市」**（第 4 节写死）落图即上屏、与 n01 同一刻，表里记 n01，不算违 12.2「天星城只许 chudao.* 起」；坊市三扇门在 `trigger_chudao`（城门北五格宽门洞）之后，记 n01 之后。
11. **验收 17 切片（T-2）**：`SeventeenTheMountainGateRefusesBeforeTheLeaseAndOpensAfterIt`（真 tryStep，拒绝语按 18.1 第 2 条是 `ch11.block.dongfu`）与 `SeventeenAfterTheShopIsSoldTheCounterAndTheLedgerSayOneLineEach`（liugen 前柜台开 ShopScene 作对照）。
12. **偏离登记（T-3，整改路按实际行为登记，协调者复核）**：① 意图玩家买霓裳草 **2** 株、章末剩 **1** 株、低阶 **+2085**（`youyao.lua` 开头判草 ≥ 1 对 ② 同样成立，② 逃不扣草）——验收第 2 条「买 1、章末 0」以此为准；② 开店当日回访 `jiesuan == 0`、只说 `jiesuan.none`——验收第 13 条「== 1」是笔误，以 3.2 n28 为准；③ `kOffChainFlags` 不登 `ch11.jiguan`（`offChainProblems` 按脚本原文先后核，② 分支写在 lieyao 累加之前，登上必报），替代判据是 Slice Eight「lieyao == 4 之后第五轮置 jiguan、huicheng 之前」与通关终点 `jiguan == 1`；④ `MapLinkWalk` 对同图门（雅间一对）改钉「落点 ≠ 门格」、跨图门判据不变——整改路追认测试路的写法，地图路无异议即定案；⑤ `Ch10Driver.h` 第 520–521 行改 `ch11_fangshi` (28,38) 不在契约 8.3 表里，需协调者补进契约（契约不在本路白名单）。
13. **扫描口子（T-4）**：先后判据两头都须找得到（`ordered()`），另加删后一头的三条负向；「take 看返回值」改成 take( 必须落在同一行 if / elseif 与 then 之间，配「挪到 then 之后」负向。
14. **对照与面板（T-5）**：删去恒真的常量比较；验收第 5 条「面板下一关写『结丹中期』所需 5000」——引擎在剧情上限处只给通用灰字 `pushAtStoryCap`，判据改为「面板灰字恰是它 ＋ `cultivationNeeded(CoreEarly) == 5000` ＋ 硬按被 StoryCap 回绝」。
15. **驱动自证（T-6）**：每个节点末断言选项队列已用完；删掉 `jiesuanVisits_` 计数（`ch11.jiesuan == 2` 才是判据）；章末站位写死 **`ch11_dongfu` (14,24)**（金青 (14,23) 正南、面朝北，n31 脚本不挪人）。

### 18.6 协调者收尾（2026-10-13）

- 内容复验第 2 轮 4.0 通过后，协调者收掉 6 条 LOW（`docs/ch11-review.md` 末节）：三句换说法（`wuchou.ban_old`、`zhanglao.fan`、`wuchou.map`）、`zufu.credit` 换句架、`zufu.lua` 补蓝环换红环一句（新 key `ch11.zufu.ring`）、`zhuzhong.lua` 删走不到的 `again` 分支；契约第 6 节金青一格改为「`ch11.done` 后离场」。
- 煞妖变身态的战斗贴图：`tools/artgen/sprites.py` 的 `BATTLE_OVERRIDES` 加 `b11_yinsha_shayao`（18.1 A7），只重烘 `sprites/index.json`。
- 审查与集成留下的 5 条非阻断问题登技术债 G-29 – G-33（`docs/tech-debt.md`）。

