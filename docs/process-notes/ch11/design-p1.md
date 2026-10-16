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

1. **一座不准打架的城。** 他第一次走进一座由「星宫」管着的巨城（ch389）：不准城内斗法、长住缴灵石、星宫不管城里的事（A4）；红环蓝环（A9）、圣山八十一层（A14）、百年一付的洞府（A15–A17）——规矩全写在钱上。**本章城内 0 场战斗**，路径行动 `spar` 0 条（第 6 节）；他花的每一笔都「有几块扣几块」、没有一处门闸要钱（E13、N.1）。
2. **猎妖是一门生意。** 交灵石乘传送阵去外星海的妖兽岛、杀妖取丹、回城换灵石（A6），没有「猎妖令」（B9）。**幕二做成一个可重复的循环**：坊市买霓裳草 → 星空殿传送 → 凝翠岛 → 红瑚岛阵心诱妖 → 战斗 ① → 收购行卖丹（E15、L.4）；五级妖丹值多少（A7）要让玩家第一次打赢就看见。六级鸡冠蛟那一场**逃是正解之一**（I 表 ②）。
3. **六十年一关。** 回城两年炼丹、散功、生死关，六十年后结丹，全城可感（A41–A43）。**不做面板冲结丹**（E3）：三段蒙太奇、六次 `advance_days(3600)`、一处 `realm.advance(realm.CORE_EARLY)`（E5）；跳过的岁月不计游玩时长。降尘丹与雪灵水「有则扣」，旁白三版（E4）。出关之后（n17 起）外人才直接跟他说话（E27）。
4. **乱星海的另一张脸。** 妙音门请他助拳，集合岛上十几人；隐煞门岛上他第一次以结丹之身压制筑基（③）；乌丑现身自认劫货嫁祸，极阴祖师附身，妖尸拦路——打不死的东西只能困住（④，E25）。**不演他不知道的事**：范赵密谋、极阴与赤火分赃、三女议事（3.0 表三）；虚天殿只许在 n23 由乌丑与孙姓中年人口中说出一句（第 12 章钩子，O）。
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
| 56 | 乌丑现身自认劫货嫁祸；极阴祖师附身乌丑（只能发挥三分之一）；虚天殿三百年一现、虚天残图在孙师叔手 | ch405 | n23 只让玩家听见这一句；第 12 章钩子（O）|
| 57 | 天都尸火焚人成灰；禁神牌交三分之一元神；双圣禁令已松 | ch406 | n23 旁白；韩立不交 |
| 58 | 赤火倒戈困住孙姓中年；众人四散；隐煞门四结丹是煞丹分身 | ch407 | n24 旁白；与曲魂同类——玩家知道的只有这一句 |
| 59 | 妖尸（天都尸）不怕法宝、伤口自愈、空中瞬移；韩立放一百多只青色巨猿傀儡与两头血玉蜘蛛，以蛛网与紫火困住后遁走 | ch408 | 战斗 ④：胜利写「困住」不写杀死 |
| 60 | 入地一个月才出；范夫人传音符邀至隆兴客栈 | ch409 | n25 地点改到清河茶馆（改编，B23）|
| 61 | 紫灵识破曲魂是身外化身、与本体同为结丹初期 | ch410 | 台词一句；不改 `qu_hun_shadan` |
| 62 | 天雷竹分两节，带根的一节在紫灵手；以此竹加每年三百灵石聘名义长老 | ch410 | n25 给 `material_tianleizhu` + `story_miaoyin_yaopai`；年俸见 N |
| 63 | 竹两寸长、小指粗；腰牌一块；白雷竹（两千年以下）、蓝雷（五千以上）、金雷竹（万年以上，辟邪之雷）| ch411 | n26 三阶：白→蓝→金各 1 滴绿液 |
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

**名字按首见章号解禁**（第 12 节 M 表）。全作禁用：银月（ch615）、紫罗极火（ch731）、黄龙（乱星海篇查无）、猎妖令（0 命中）。本章不许提前：虚天残图 / 虚天殿只许在 n23 由乌丑与孙姓中年人口中说出一次；极阴岛只许作为乌丑的来历；元瑶、文思月（ch430）、明珠轩（ch513）、交换会（ch535）本章 0 处。

### 1.2 大纲、lore、人物总表与占位数据里对不上的地方（已核，待回写）

| 在哪 | 原说法 | 复核 | 本章以什么为准 |
| --- | --- | --- | --- |
| 大纲第 11 章人物 | 「文樯」 | 文樯见 ch369–371（第 10 章）与 ch430（第 12 章）；ch389–415 区间 0 命中 | **回写**大纲：本章人物删「文樯」；本章不出场 |
| 大纲第 11 章人物 | 「木龙真人」 | 木龙真人仅 ch371 一处；本章区间只有「木龙碑」（ch409，妙音门镇派之宝）| **回写**大纲：删「木龙真人」，加注「木龙碑 ch409 只在传闻一句」；本章 0 出场 |
| 人物总表 `fan_furen` | 「先与韩立以妖兽材料换天雷竹」 | ch401 是材料换中阶灵石、天雷竹只是「线索」；ch402 天雷竹是助拳酬劳；ch410 由紫灵以竹聘长老 | **回写**总表 summary；本章按原著三步走（n20 / n21 / n25）|
| 人物总表 `zi_ling` | 首见 ch403、「紫灵仙子」 | ch403 首见；「汪凝」真名 ch403 口中自称、ch409 真容 | 不改；本章名牌「紫灵仙子」→ ch409 起「紫灵」（第 5 节）|
| 人物总表 `wu_chou`、`jiyin_zushi` | 首见 ch405 | 乌丑 ch382 首见（第 10 章已记）；「极阴祖师」名号 ch382 已由六连殿长老口中说出 | 第 10 章 16.1 第 1 条已回写乌丑；**本章回写极阴祖师首见 ch382** |
| 人物总表 `zhao_meng_zhanglao` | 「赵长老 / 孟长老」 | 叛徒是赵长老与一名儒生长老（ch404 紫灵呼「符长老」）；孟姓结丹修士是外请助拳者（ch402–403）；ch405/410 范夫人与紫灵又称「赵孟两位长老」——原文称谓前后不一 | **回避**：本作只写「赵长老」「符长老」，孟道友不叛；总表加注 |
| lore 4.3 | 「长住者按月缴灵石」vs 同节「租洞府百年计费」 | ch389 典籍写「每月」；ch391 租赁写「百年」——两句原文都有 | **本作**：红环一次 800、洞府百年 2700；「每月」只在典籍台词，不做机制 |
| lore 第七节 | 「韩立在乱星海开丹药店」已标部分属实 | ch412 只卖符与药材、后加低阶法器，ch413 再加中阶法器与中级符箓 | 商店条目照此（L 表）；不卖丹药 |
| 大纲 | 「猎妖令」 | 0 命中 | 已删；本章接单只有任务板悬赏与收购行订单，`origin` 写「改编」|
| 大纲人物 | 「天星双圣」 | 本章区间只在典籍 / 传闻（ch389、406、408、415）| **不建 role、不上图**；人物总表那一行保留，加注「传闻」|
| 占位 `b11_ningcuidao_youyao` | 敌方 `siji_haishou` + `sanji_haishou`，奖励修为 700 / 灵石 150 / 四级妖丹 | 首杀是五级千目妖（ch396）；妖丹是五级 | **就地改写**：敌 `qianmu_yao` ×1 + `siji_haishou` ×2（第二波，海兽随草香而来，改编）；掉落 `material_yaodan_wuji`；探针挪到第 12 章（E19）|
| `siji_haishou` note、`wuji_haishou` note | 「四级大致对应筑基后期」「五级≈结丹初期 ch503」 | 与 lore 4.4 一致 | 不改；`wuji_haishou` 本章不用（它是 `b13_waixinghai_lieyao` 的）|
| `material_jinleizhu` note | 「紫灵以半截天雷竹加年俸 300 聘」 | ch410：两节中带根的一节；年俸三百是妙音门付韩立 | note 措辞改「带根一节」；价值锚不动 |
| `recipe_qingzhu_fengyun_jian` | `productCount 12`、金雷竹 ×4、灵石 ×20、`requiredProficiency 80` | ch413 六根；ch415 七十二口「一件法宝收入体内」；他是一次炼成、无材料再试 | **改**：`productCount 1`、金雷竹 ×6、新增辅料 `material_lianjian_fuliao` ×1、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`（E16）|
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
| 章末落点（`ch10_xiaohuan` (50,39)，`ch10.done`）| ch389 一月赶路到天星城 | **patch `scripts/ch10/zhifadui.lua` 末尾**：在 `flag.set("ch10.done")` 之后补一行 `teleport("ch11_fangshi", 城门 spawn)`（E1，与第 9→10 先例一致；契约第 3 节「开局」第一支）；`ch10-end` 两份 fixture 按 patch 后重生成（测试路，O-10）；第 10 章图一个字不动 |
| 通缉（契约第 3 节「通缉」）| ch389 只在他心里一句 | 不设状态字段；只在 n01 旁白与 K2 一句（E28）|
| `formerRealm` = 22 | 已越过 | 不读、不写；重修加速不再生效 |
| `qu_hun_shadan` 在队（结丹初期）| ch390–394 入城、租府、传送、小街都由它出面；ch410 被识破 | 全章在队、不 `party.remove`；曲魂代言到 n17 为止（E27）|
| 混元钵（`story_hunyuan_bo`，化身用）| ch392 曲魂炼化 | n06 一句；不扣 |
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
| 小瓶：容量 6、筑基五日一滴 | ch395 以绿液试草、ch411 三滴催竹、ch412 幼竹 | 不动瓶子代码；结丹后 3 日一滴由引擎按境界给（`Application.cpp` 第 130 行）；诱妖与培竹都 `bottle.spend(1)` |
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
| 关键 story | 绿煌剑、混元钵（化身用）、血玉蜘蛛、傀儡弓手、噬金虫、海域图、储物袋、聚魂钵、神风舟、乱星海丹方、丹道评鉴、土离决、真元丹方、大衍诀玉简 | 同 | 不扣；1.4 引用 |
| `learnedMagics`（11 门）| 火弹术、御风决、天眼术、流沙术、冰冻术、祭金符、祭青蛟、青元剑芒、祭青火掌、祭小刀、祭红线针 | 同 | 本章不新学（E17）；青元剑芒 `needRealm FoundationEarly` 已可用 |
| `party` | `qu_hun_shadan`（active）| 同 | 全章在队；不 `party.remove` |
| `bottle` | capacity 6、drops 6、matureKnown、owned | 同 | 筑基 5 日一滴、结丹后 3 日一滴（引擎给）|
| `fields` | field_baiyaoyuan 6、_jiao 2、field_dongfu 4、_nei 2、field_xiaohuan 4（second 侧有 1 槽血红芝熟）| — | 都走不回去；本章新起 `field_tianxing_39` |
| `knownWeaknesses` | 36 个 role | 同 | 本章新 role 的破绽都未揭 |
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
**同格按旗标换触发器**的三处：红瑚岛登岸格（n11 → n14，`guard ch11.buzhen`）、礁岛登岛格（n22 → n23 → n24 三连）、密室门外（n16 置旗后 n17）——第 10 章 `zhenwei_*` 的写法；validate 若不许同格两个 trigger 则挪一格（O-7）。

| # | 挂点 | 地图 | `mode` | `once` | `guard_flag` | `set_flag` | 战斗 | 为什么是这一个 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| n01 | `trigger_chudao` | `ch11_fangshi` | `enter` | ✓ | `ch10.done` | `ch11.chudao` | — | 城落在他眼前。城门 spawn `chengmen` 那一格（E1 的 teleport 落点）|
| n02 | NPC `npc_baiyi_xiushi`（`zhihuan.lua`）| `ch11_fangshi` | NPC | — | `ch11.chudao` | `ch11.zhihuan`（1 / 2）| — | 走到星宫修士面前的是他。城门内一格；曲魂先答话（E27）|
| n03 | NPC `npc_lu_er`（`luer.lua`）| `ch11_fangshi` | NPC | — | `ch11.zhihuan` | `ch11.luer`（1 / 2）| — | 鲁二凑上来，答不答应的是他。坊市街南端 |
| n04 | NPC `npc_xinggong_zhishi`（`zufu.lua`）| `ch11_fangshi` | NPC | — | `ch11.luer` | `ch11.zufu` | — | 进山脚楼阁租府的是他。北缘楼阁内。**teleport `ch11_dongfu` 洞府门 `men`** |
| n05 | `trigger_anju` | `ch11_dongfu` | `enter` | ✓ | `ch11.zufu` | `ch11.anju` | — | 一进门就撞上的安顿。洞府门内一格。**`field.unlock("field_tianxing_39", 6)`**；设施 `require_flag ch11.anju` 自此开 |
| n06 | `trigger_kuilei` | `ch11_dongfu` | `interact` | ✓ | `ch11.anju` | `ch11.kuilei` | — | 关起门造傀儡的是他。密室一格。**`advance_days(180)`、`advance_days(720)`** |
| n07 | NPC `npc_baimen_zhanggui`（`ziliao.lua`）| `ch11_fangshi` | NPC | — | `ch11.kuilei` | `ch11.ziliao` | — | 进白门阁买资料的是他。坊市街北段 |
| n08 | `trigger_nichang` | `ch11_dongfu` | `interact` | ✓ | `ch11.ziliao` | `ch11.nichang` | — | 在药园以绿液试草的是他。药园旁一格。**`bottle.spend(1)`；`advance_days(180)`**；坊市杂货与星空殿北口自此开 |
| n09 | NPC `npc_xingkong_xiushi`（`xingkong.lua`）| `ch11_xingkong` | NPC | ✗ | `ch11.nichang` | `ch11.chuanshong`（首次）| — | 交灵石上阵的是他。殿内传送阵（NPC 就是阵）。**每次 teleport `ch11_ningcui_jie` `shiwu`**；`ch11.huicheng ≥ 1` 时只说一句 `return` |
| n10 | `trigger_ningcui` | `ch11_ningcui_jie` | `enter` | ✓ | `ch11.chuanshong` | `ch11.ningcui` | — | 一出石屋就遇上的人。石屋门外一格。东口 `portal_chuhai` 自此开 |
| n11 | `trigger_buzhen` | `ch11_honghu` | `enter` | ✓ | `ch11.ningcui` | `ch11.buzhen` | — | 登岸即布阵（旁白）。岛西登岸格。阵心 `trigger_youyao` 自此可用 |
| n12 | `trigger_youyao` | `ch11_honghu` | `interact` | ✗ | `ch11.buzhen` | —（脚本累加 `ch11.lieyao`）| ① | 催草诱妖的是他。环心珊瑚礁一格。**`advance_days(3)`；败 game over** |
| n13 | （n12 脚本内分支）| `ch11_honghu` | — | — | `ch11.lieyao == 4` 且 `ch11.jiguan == 0` | `ch11.jiguan` | ② | 第 5 次诱妖改走鸡冠蛟；胜 / 逃 / 败都置旗 |
| n14 | `trigger_huicheng` | `ch11_honghu` | `interact` | ✓ | `ch11.lieyao ≥ 3`（同格 `guard ch11.buzhen`）| `ch11.huicheng` | — | 收手回城的是他。岛西登岸格（n11 之后同格换）。**`advance_days(1800)`；teleport `ch11_dongfu` `men`**；出海自此关 |
| n15 | `trigger_liandan` | `ch11_dongfu` | `interact` | ✓ | `ch11.huicheng` | `ch11.sangong` | — | 闭府炼丹、散功的是他。丹房一格（与丹房设施不同格）。**`advance_days(720)`、`advance_days(15)`** |
| n16 | `trigger_biguan` | `ch11_dongfu` | `interact` | ✓ | `ch11.sangong` | `ch11.jiedan` | — | 入生死关的是他。密室另一格（与 n06 不同格）。**6 × `advance_days(3600)`；`realm.advance(31)`** |
| n17 | `trigger_chuguan` | `ch11_dongfu` | `enter` | ✓ | `ch11.jiedan` | `ch11.chuguan` | — | 出关是走出来撞上的。密室门外一格（n16 置旗后同格）|
| n18 | `trigger_zhaolan` | `ch11_dongfu` | `enter` | ✓ | `ch11.chuguan` | `ch11.zhaolan` | — | 传音符撞在禁制上。洞府门禁制一格。**扣全部中阶** |
| n19 | NPC `npc_lian_er`（`lianer.lua`）| `ch11_fangshi` | NPC | — | `ch11.zhaolan` | `ch11.lianer` | — | 莲儿拦路——但开口的是他。坊市街中段。茶馆门 `require_flag ch11.lianer` |
| n20 | NPC `npc_fan_furen`（`chaguan.lua`）| `ch11_fangshi` | NPC | — | `ch11.lianer` | `ch11.chaguan` | — | 进雅间坐下的是他。东北角茶馆雅间（赵长老 NPC 另一格）|
| n21 | `trigger_zhuzhan` | `ch11_dongfu` | `enter` | ✓ | `ch11.chaguan` | `ch11.zhuzhan` | — | 传音符送到门口。洞府门一格（与 n18 不同格）。**`advance_days(15)`；teleport `ch11_yinsha_dao` `haishang`** |
| n22 | `trigger_guiju` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.zhuzhan` | `ch11.guiju` | ③ | 登岛即开打。登岛格。**败 game over** |
| n23 | `trigger_wuchou` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.guiju` | `ch11.jiyin` | — | 乌丑是找上门的。n22 置旗后同格 |
| n24 | `trigger_daoge` | `ch11_yinsha_dao` | `enter` | ✓ | `ch11.jiyin` | `ch11.yaoshi` | ④ | 妖尸拦路。n23 置旗后同格。**`advance_days(30)`；teleport `ch11_dongfu` `men`；败 game over** |
| n25 | NPC `npc_zi_ling`（`zhanglao.lua`）| `ch11_fangshi` | NPC | — | `ch11.yaoshi` | `ch11.zhanglao` | — | 赴约进雅间的是他。茶馆雅间（改编地点 B23；范、卓 NPC 另两格）|
| n26 | `trigger_peizhu` | `ch11_dongfu` | `interact` | ✗ | `ch11.zhanglao` | —（脚本写 `ch11.zhu`、`ch11.jinlei`）| — | 滴绿液催竹的是他。药园珊瑚礁旁一格（与 n08 不同格）。**`zhu == 3` 时 `advance_days(720)`** |
| n27 | NPC `npc_li_dianzhu`（`kaidian.lua`）| `ch11_fangshi` | NPC | — | `ch11.jinlei` | `ch11.kaidian` | — | 盘店的是他。李氏杂货门口（街西）。记 `ch11.kaidian_ri`、`ch11.jiesuan_ri` |
| n28 | `trigger_jiesuan` | `ch11_xiaoxuan` | `interact` | ✗ | `ch11.kaidian` | —（脚本累加 `ch11.jiesuan`、写 `ch11.jiesuan_ri`）| — | 坐到柜台后的是他。柜台一格（与 `facility_guitai` 不同格）|
| n29 | `trigger_peizhu`（n26 同一触发器，`ch11.jinlei` 后换分支）| `ch11_dongfu` | `interact` | ✗ | `ch11.jinlei` | —（脚本累加 `ch11.zhu_gen`；6 时置 `ch11.liugen`）| — | 每根 **`advance_days(1080)`**；第六根收店：补足到开店起 7200 日、`give` 中阶 20 |
| n30a | `trigger_lianjian` | `ch11_dongfu` | `enter` | ✓ | `ch11.liugen` | —（演在面板前，不置主旗）| — | 走到炼器台前撞上的蒙太奇。密室炼器台同格（`facility_lianqi_39` kind forge，`require_flag ch11.liugen`）。**`advance_days(120)`** |
| n30b | `trigger_chujian` | `ch11_dongfu` | `enter` | ✓ | `item.count("weapon_qingzhu_fengyun_jian") >= 1` | `ch11.lianjian` | — | 出关是走出来的。密室门外一格（与 n17 不同格）。**`advance_days(60)`** |
| n31 | NPC `npc_jin_qing`（`zhuzhong.lua`）| `ch11_dongfu` | NPC | — | `ch11.lianjian` | `ch11.done` | — | 金青等在门外，上前搭话的是他。洞府门外（`visible_flag ch11.lianjian`）。**不 `ending()`** |
| Z1 | NPC `npc_fan_xiaozi`（`shougou_dingdan.lua`）| `ch11_ningcui_jie` | NPC | — | `ch11.ningcui` | `ch11.z1`（1 / 2）| — | 接单的是他；**不在目标链上**；`ch11.huicheng` 后 fail |
| Z2 | NPC `npc_jiedan_linju_b`（`linju_z2.lua`）| `ch11_dongfu` | NPC | — | `ch11.zhaolan` | `ch11.z2` | — | 以定颜丹换珍稀物；**不在目标链上** |
| 回城 | `trigger_huicheng_chuansong` | `ch11_ningcui_jie` | `interact` | ✗ | `ch11.chuanshong` | — | — | 石屋传送阵回星空殿。**teleport `ch11_xingkong` `dian`** |
| — | 各新图存档点 `facility_cunji_*` | 七张新图 | 设施 | — | — | — | — | ①② 之前（红瑚岛、凝翠岛）、③④ 之前（礁岛无——n21 teleport 前在洞府存）|

- **`once=true` 的挂点每一条都写 `set_flag`**；条件不够（绿液不足、草不够、`realm.advance` 失败）的路都在置旗标之前 `return`。**n13 的三种结局都置 `ch11.jiguan`**（它是「一场已演」，不是「胜」）。
- **同一格只挂一个对象**：洞府里 n05、n06、n08、n15、n16、n18、n21、n26、n30a 九格与静室、丹房、炼器台、药园四处设施互不相同（n30a 与 `facility_lianqi_39` 例外：同格，trigger `enter` 先演、面板后开——validate 若不许则 trigger 挪到台前一格）；密室门外 n17 与 n30b 不同格；礁岛 n22/n23/n24 是同格三连（按旗标换）。
- **四场战斗**：① 挂在循环挂点上（n12，目标链靠 `ch11.lieyao ≥ 3` 过 n14）；③④ 挂在目标链的必经挂点上（n22、n24）；② 是 n12 的分支，可逃。
- **门钥匙**（`kOffChainDoorKeys`）：青竹小轩门（`require_flag ch11.kaidian`、`hidden ch11.liugen`）、茶馆门（`require_flag ch11.lianer`）、星空殿北口（`require_flag ch11.nichang`）、凝翠岛东口（`require_flag ch11.ningcui`）——`ch11.kaidian`、`ch11.lianer`、`ch11.nichang`、`ch11.ningcui` 都在链上，不必另登；链外门钥匙：无。
- **脚本 teleport 登进 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`（七条，N.4）**：

| done_flag | 脚本 | 落点图 |
| --- | --- | --- |
| `ch10.done`（patch）| `scripts/ch10/zhifadui.lua` | `ch11_fangshi` |
| `ch11.zufu` | `scripts/ch11/zufu.lua` | `ch11_dongfu` |
| `ch11.chuanshong`（及其后每次）| `scripts/ch11/xingkong.lua` | `ch11_ningcui_jie` |
| （回城，无新旗标）| `scripts/ch11/huicheng_chuansong.lua` | `ch11_xingkong` |
| `ch11.huicheng` | `scripts/ch11/huicheng.lua` | `ch11_dongfu` |
| `ch11.zhuzhan` | `scripts/ch11/zhuzhan.lua` | `ch11_yinsha_dao` |
| `ch11.yaoshi` | `scripts/ch11/daoge.lua` | `ch11_dongfu` |

`kOffChainFlags` 候选（规则 30）：`ch11.jiguan`、`ch11.dan_fu`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen`（E21）。

### 3.2 脚本动作与分支

本节仅规定状态操作、控制流和挂点，不提供叙事镜头、示例台词或转述顺序。剧情事实与质感禁用项只按 13.1 核验；对白另行原创。挂点属性以 3.1、日期以 3.3 为准。脚本一律 `scripts/ch11/<id>.lua`；每一处 `take` / `give` / `realm.*` / `party.*` / `bottle.*` / `field.*` 看返回值；扣东西「有几块扣几块」（E13）。

| 节点 / 脚本 | 在场 | 状态动作 | 分支 / 门闸 |
| --- | --- | --- | --- |
| n01 chudao | 韩立、曲魂 | 置 ch11.chudao | enter；通缉只一句；曲魂代言交代一句（E27）|
| n02 zhihuan | 星宫修士、曲魂先答 | take 中阶 8 成 → 置 ch11.zhihuan = 1；否则 take 低阶 3 → = 2；再不成也 = 2 | 三版旁白；无死局 |
| n03 luer | 鲁二 | choice 雇 / 不雇；雇 → take 低阶 5（不足照雇）、= 1；不雇 → = 2 | 取消 return；两支信息量相同（不雇则 n04 执事多说两句）|
| n04 zufu | 星宫执事、曲魂出面 | take 中阶 27（有几扣几）；give story_jinzhi_lingpai；置 ch11.zufu；teleport ch11_dongfu men | 零块时「记在结丹前辈名下」换句（E13）；三十一 / 四十七层只在台词，不可选 |
| n05 anju | 韩立、曲魂 | field.unlock("field_tianxing_39", 6)；置 ch11.anju | enter；unlock 查返回值；鲁二 hidden |
| n06 kuilei | 韩立、曲魂 | advance_days(180)；advance_days(720)；give story_kuilei_qun；置 ch11.kuilei | 混元钵一句；噬金虫物品不扣 |
| n07 ziliao | 白门阁掌柜 | take 中阶 1 或 take 低阶 100；give story_yaoshou_ziliao；置 ch11.ziliao | 都不成则「赊」（旁白）；必给资料 |
| n08 nichang | 韩立 | bottle.spend(1)（不成 → advance_days(5) 再 spend）；advance_days(180)；置 ch11.nichang | 第二次仍不成照置旗（不设死局）；坊市杂货 facility 与星空殿北口 require_flag ch11.nichang |
| n09 xingkong | 星空殿执事、首演另有妖艳妇人一伙与枣红老者 | 首次：take 中阶 2、置 ch11.chuanshong；此后每次：take 中阶 7；teleport ch11_ningcui_jie shiwu | once=false；ch11.huicheng ≥ 1 → 一句 return；不足 → 「对方全出 / 搭别人的阵」（E14），**永远走得通** |
| n10 ningcui | 枣红老者（旁白）、郝远天、范小子、杂货掌柜 | choice 买海图：take 中阶 1 → give story_waihai_tu；置 ch11.ningcui | enter；不买也置旗；东口 portal require_flag ch11.ningcui |
| n11 buzhen | 韩立、曲魂 | 置 ch11.buzhen | enter；不发物品（A35）|
| n12 youyao | 韩立、曲魂 | 前置：item.count("herb_nichang_cao") ≥ 1 且 bottle.drops ≥ 1；take 草 1、bottle.spend(1)；advance_days(3)；若 ch11.lieyao == 4 且 ch11.jiguan == 0 → 转 n13；否则 battle ①，胜 → ch11.lieyao += 1 | once=false；缺草 → 旁白指凝翠岛杂货 return；缺绿液 → advance_days(5) return；败 game_over；逃不扣草（can_escape true） |
| n13 jiguan | 韩立、曲魂 | battle ②；胜 → give material_yaodan_liuji 1；胜 / 逃 / 败都置 ch11.jiguan | can_escape true、defeat 不致命；逃或败同一句旁白、不给 |
| n14 huicheng | 韩立、曲魂 | choice 收手 / 再猎；收手：take story_kuilei_qun 1、give story_kuilei_can 1、give material_yaodan_wuji 8、give story_yaoshou_cailiao 1；advance_days(1800)；置 ch11.huicheng；teleport ch11_dongfu men | 门闸 ch11.lieyao ≥ 3；再猎 → return；置旗后星空殿只说一句 |
| n15 liandan | 韩立、曲魂 | advance_days(720)；give 中阶 30；advance_days(15)；置 ch11.sangong | 散功只旁白，**不 realm.demote** |
| n16 biguan | 韩立、曲魂隔门 | take pill_jiangchen_dan 5 → dan_fu 位 1；take material_xuelingshui 2 → 位 2；置 ch11.dan_fu；fade.out；advance_days(3600) ×2、talk、×2、talk、×2；realm.advance(realm.CORE_EARLY)；fade.in；置 ch11.jiedan | 门闸无；advance 失败 → 占位一句 return（不置旗）；旁白按 dan_fu 三版 |
| n17 chuguan | 韩立、曲魂 | 置 ch11.chuguan | enter；物品不动；E27 起外人直接与韩立说话 |
| n18 zhaolan | 韩立、曲魂 | take 中阶 全部（有几扣几）；give story_kuilei_yuan_qun 1；置 ch11.zhaolan | enter；零块时换句；结丹邻居 NPC visible |
| n19 lianer | 莲儿、范静梅 | 置 ch11.lianer | 茶馆门 require_flag ch11.lianer |
| n20 chaguan | 范夫人、赵长老、曲魂 | take story_yaoshou_cailiao 1 → give 中阶 40；置 ch11.chaguan | 材料缺则仍 give 40（n14 必给，缺只会是异常）；范、赵 hidden |
| n21 zhuzhan | 韩立、曲魂；旁白里莲儿、集合岛众人 | advance_days(15)；置 ch11.zhuzhan；teleport ch11_yinsha_dao haishang | enter；礁岛无门、只进不出 |
| n22 guiju | 紫灵仙子、范、卓、赵、符、孟、赤火、曲魂 | battle ③；胜 → 置 ch11.guiju | enter；败 game_over；门主 NPC visible |
| n23 wuchou | 乌丑（只说话）、隐煞门门主、众人 | 置 ch11.jiyin | enter；名牌换 jiyin_fushen；韩立不交元神；虚天殿一句 |
| n24 daoge | 极阴附身、赤火、妖尸、曲魂、血玉蜘蛛 ×2、猿傀 ×2 | battle ④；胜 = 困住；advance_days(30)；置 ch11.yaoshi；teleport ch11_dongfu men | enter；败 game_over；礁岛全员 hidden；末尾一句传音符 |
| n25 zhanglao | 紫灵（真容名牌）、范夫人、卓如婷 | give material_tianleizhu 1；give story_miaoyin_yaopai 1；置 ch11.zhanglao | 年俸三百只在台词；三人 hidden |
| n26 peizhu（前半）| 韩立 | bottle.spend(1) 成 → ch11.zhu += 1；zhu == 3 → take material_tianleizhu 1、give material_jinleizhu 1、take 中阶 全部、take material_yaodan_wuji 全部、advance_days(720)、give material_lianjian_fuliao 1、置 ch11.jinlei | once=false；spend 不成 → 「等几日」return；辅料**必给**，不看扣到多少 |
| n27 kaidian | 李氏店主、曲魂 | take 低阶 50（有几扣几）；flag.set("ch11.kaidian_ri", today())；flag.set("ch11.jiesuan_ri", today())；置 ch11.kaidian | 店门 portal require_flag ch11.kaidian；何仙师、邻店主 visible |
| n28 jiesuan | 韩立、（首次）何仙师 | n = (today() − ch11.jiesuan_ri) / 30，上限 240；give 低阶 15 × n；ch11.jiesuan_ri = today()；ch11.jiesuan += 1 | once=false；n == 0 → 一句 return；首次附旁白、第二次起只结算（E22）|
| n29 peizhu（后半，ch11.jinlei 后）| 韩立 | bottle.spend(1) → advance_days(1080)、ch11.zhu_gen += 1、give material_jinleizhu 1；zhu_gen == 6 → advance_days(max(360, 7200 − (today() − kaidian_ri)))、give 中阶 20、置 ch11.liugen | spend 不成 → return；补足写法照 `sanzhuan.lua`；店门与柜台 hidden_flag ch11.liugen |
| n30a lianjian | 韩立 | advance_days(120) | enter、once；不置主旗；面板由 facility_lianqi_39 开（require_flag ch11.liugen），配方 recipe_qingzhu_fengyun_jian 必成（E16）|
| n30b chujian | 韩立、曲魂 | advance_days(60)；置 ch11.lianjian | enter；门闸 item.count("weapon_qingzhu_fengyun_jian") ≥ 1，否则 return |
| n31 zhuzhong | 金青、曲魂 | give story_bai_zhenzhu 1；置 ch11.done | 不 ending()；不写去向；谣言只到谣言 |
| Z1 shougou_dingdan | 范小子 | 接单 → ch11.z1 = 1；交付 take material_yaodan_wuji 3 → give 中阶 10、material_lianqi_yao 2、ch11.z1 = 2 | accept ch11.ningcui；fail ch11.huicheng；`origin` 改编 |
| Z2 linju_z2 | 结丹邻居乙 | take pill_dingyan_dan 1 → give material_lianqi_yao 3；ch11.z2 = 1 | 没丹 → 一句 return；accept ch11.zhaolan |
| 回城 huicheng_chuansong | 守阵修士 | teleport ch11_xingkong dian | once=false；不扣钱（回程费在 n09 一并算，E14）|

- 只使用既有脚本接口（`ScriptHost.cpp` 命令清单：advance_days、battle、bottle_*、choice、fade_*、field_*、flag_get、give_item、item_count、realm_*、set_flag、take_item、talk、teleport），无新增命令；不读取或模拟 formerRealm，不从脚本发修为，本章 `realm.demote` 0 处、`realm.cap` 0 处、`party.remove` 0 处。
- 所有扣物、升境、滴液、开田都核返回值。普通资源不足的内容支路照常可走：**本章唯一的主线门闸是 n14 的 `ch11.lieyao ≥ 3`（打赢三场 ①）与 n30b 的「剑在身上」**，两处都不花钱。
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
| n26 三滴（结丹后 3 日一滴，瓶内 6 滴够用，缺则各 ＋5）；两年凑辅料 | 720 | 25995 |
| n27 盘店（记 `ch11.kaidian_ri`）| 0 | 25995 |
| n28 回访（不拨日）| 0 | — |
| n29 六根各三年 | 1080 × 6 | 32475 |
| n29 第六根补足到开店起 7200 日（至少 360）| 7200 − 6480 ＝ 720 | **25995 ＋ 7200 ＝ 33195** |
| n30a 打坐三日、炼液三月、布阵半月、择日 | 120 | 33315 |
| n30b 两月出关 | 60 | 33375 |
| n31 | 0 | **33375** |

原著的锚：一个多月（ch389）、七天（ch390）、半年 / 两年整（ch392）、一个月（ch395）、两三日 / 一个多月（ch396）、五年 / 整整两年 / 半月（ch397）、六十年（ch397–398）、四年 / 五年多（ch398）、两月前（ch402）、半月（ch402）、次日清晨（ch403）、一个月（ch409）、两年 / 数月（ch411–412）、第九个年头 / 二十余年（ch413）、三个月 / 半月 / 两日 / 一天一夜（ch414）、两个月（ch415）。
**二十余年的算法**：开店（n27）到收店（n29 第六根）恰 7200 日（二十年）——玩家回访结算几次、培竹之间隔多久，都由第六根补齐，不叠加（**不许写成「多等一根就晚收店三年」**）。全章意图玩家约 **33375 日**（约九十二年半），其中六十年在 n16 一处；本章**没有倒计时**，横幅照剧情写。
**3.2 里没写 `advance_days` 的挂点，按本表拨**（本表是 `advance_days` 的唯一出处）。回访结算 n28 按 `ch11.jiesuan_ri` 到 `today()` 的差算，本身不拨日。

### 3.4 目标链（`data/objectives/ch11.json`）

31 步照 F 表 done_flag 次序（笔记 N.4）。整数旗标（`ch11.lieyao`、`ch11.jiesuan`）在链上按「≥ 1」判完成。

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
| 9 | `n09_xingkong` | `ch11.chuanshong` | `ch11_xingkong` | `npc_xingkong_xiushi` | **「坊市杂货开始卖霓裳草了；星空殿在洞府北面——传送阵按次收钱，拼阵便宜」** |
| 10 | `n10_ningcui` | `ch11.ningcui` | `ch11_ningcui_jie` | `trigger_ningcui` | |
| 11 | `n11_buzhen` | `ch11.buzhen` | `ch11_honghu` | `trigger_buzhen` | |
| 12 | `n12_youyao` | `ch11.lieyao` | `ch11_honghu` | `trigger_youyao` | **「到阵心催草诱妖——每次一株霓裳草、一滴绿液；妖丹拿回凝翠岛收购行换灵石」** |
| 13 | `n13_jiguan` | `ch11.jiguan` | `ch11_honghu` | `trigger_youyao` | **「草催到四百年份会引来六级的东西——打不过就走」** |
| 14 | `n14_huicheng` | `ch11.huicheng` | `ch11_honghu` | `trigger_huicheng` | **「猎够三头五级再收手；回城之后本章不再出海」** |
| 15 | `n15_liandan` | `ch11.sangong` | `ch11_dongfu` | `trigger_liandan` | |
| 16 | `n16_biguan` | `ch11.jiedan` | `ch11_dongfu` | `trigger_biguan` | **「进密室之前，降尘丹与雪灵水都在身上」** |
| 17 | `n17_chuguan` | `ch11.chuguan` | `ch11_dongfu` | `trigger_chuguan` | |
| 18 | `n18_zhaolan` | `ch11.zhaolan` | `ch11_dongfu` | `trigger_zhaolan` | |
| 19 | `n19_lianer` | `ch11.lianer` | `ch11_fangshi` | `npc_lian_er` | |
| 20 | `n20_chaguan` | `ch11.chaguan` | `ch11_fangshi` | `npc_fan_furen` | **「清河茶馆的雅间——把那包妖兽材料带上」** |
| 21 | `n21_zhuzhan` | `ch11.zhuzhan` | `ch11_dongfu` | `trigger_zhuzhan` | **「出发前存个档——礁岛上没有退路」** |
| 22 | `n22_guiju` | `ch11.guiju` | `ch11_yinsha_dao` | `trigger_guiju` | |
| 23 | `n23_wuchou` | `ch11.jiyin` | `ch11_yinsha_dao` | `trigger_wuchou` | |
| 24 | `n24_daoge` | `ch11.yaoshi` | `ch11_yinsha_dao` | `trigger_daoge` | **「那东西打不死——让蜘蛛与傀儡困住它」** |
| 25 | `n25_zhanglao` | `ch11.zhanglao` | `ch11_fangshi` | `npc_zi_ling` | |
| 26 | `n26_peizhu` | `ch11.jinlei` | `ch11_dongfu` | `trigger_peizhu` | **「药园边上一滴一滴催：白、蓝、金，三滴」** |
| 27 | `n27_kaidian` | `ch11.kaidian` | `ch11_fangshi` | `npc_li_dianzhu` | |
| 28 | `n28_jiesuan` | `ch11.jiesuan` | `ch11_xiaoxuan` | `trigger_jiesuan` | **「店开着就有进项——隔些日子回柜台结一结」** |
| 29 | `n29_liugen` | `ch11.liugen` | `ch11_dongfu` | `trigger_peizhu` | **「六根金雷竹，一根三年」** |
| 30 | `n30_lianjian` | `ch11.lianjian` | `ch11_dongfu` | `facility_lianqi_39` | **「密室炼器台：青竹蜂云剑——材料齐了就不会失手」** |
| 31 | `n31_zhuzhong` | `ch11.done` | `ch11_dongfu` | `npc_jin_qing` | |

- 文案进 `data/text/ch11_objectives.json`。**第 13、28 步的去留交协调者**：两步的 done_flag（`ch11.jiguan`、`ch11.jiesuan`）同时是 `kOffChainFlags` 的候选（E21、N.4）——n14 的门闸只要 `ch11.lieyao ≥ 3`（鸡冠蛟在第 5 次诱妖才来，玩家可以三胜就收手），店铺回访也不是必经；若目标链要求每一步都按序到达，**删这两步（链 29 步），两旗登 `kOffChainFlags`**；若链允许「可跳过的步」，保留 31 步。不为这两步加引擎。
- `ObjectiveRoute` 沿这条链逐步走门：坊市 ↔ 洞府一对门、洞府 → 星空殿一口、凝翠岛 ↔ 红瑚岛一对门、坊市 ↔ 青竹小轩一对门；星空殿 → 凝翠岛、凝翠岛石屋 → 星空殿、洞府 → 礁岛、礁岛 → 洞府都是脚本 teleport（3.1 末表），礁岛没有门（规则 29 对它无要求）。
- 第 12 步的提示在 `ch11.huicheng` 之后不再显示（链已过）；第 9 步在 `ch11.huicheng ≥ 1` 后若玩家再去星空殿，脚本只说一句「本章不再出海」（G 表附注 2），目标链不回退。
- **本章新增的按键提示：0 条**（诱妖、结算、炼剑面板都用既有的键，不点名物理键）。
- 第 1、5、11、17 等步不写 R-3 提示：它们是 `enter` 挂点，走到就演。

---

