# 地图出入口与人物总表

> 2026-09-27 整理。起因是一份试玩反馈：韩家村出口往下走，进青牛镇却落在最下面，多按一下「下」就在两张图之间来回弹；青牛镇街上有三个「卖药郎」。顺着这两条把全部 24 张图的出入口与人物各盘了一遍，改完的现状列在第 4、5 节。
> 第 4、5 节的表是从 `maps/*.tmj` 直接导出的**快照**；以后改了地图，以 tmj 与门禁为准，本表随手更新。
> 2026-09-28 追加第 7 节：第 1 章目标链与门闸次序相反（目标说先上山道，门要先报名），按地图次序改了目标链与 8 句台词，并加了沿目标链逐步走门的用例。

---

## 1. 两条规矩（门禁规则 29、30，`H:/Work/Kys/docs/map_spec.md` 第 7 节）

**规则 29：出门往哪走，进门就朝哪；按住那个方向，走不回原来那张图。**

- 从 A 图北口（往上走）出去，就落在 B 图南口、面朝北；东口出去落在西口、面朝东。内门同理：往北走进去的门，里头从南墙进屋。
- 落点的 `facing` 必须等于走进门的方向；从落点按住这个方向一直走，不许踩上回 A 的那道门。
- 违反了会怎样：衔接「掉头」时，落点离回程门只有一两格，玩家本来就按着方向键（`WorldScene::update` 里按住就一直走），一进门就又走回去——这正是那份反馈里的来回弹。「拐 90°」不会弹，但方向感是乱的。
- 脚本 `teleport()` 不在此列（船、马、翻墙这种剧情转场）。

**规则 30：同一张图上同时在场的 NPC 不能用同一个 role（名牌重名）。**

- 同格或同图两个对象挂同一个 role、在场旗标互补（一个的 `hidden_flag` 等于另一个的 `visible_flag`）是合法写法：那是同一个人换了一副面孔或换了个站位。
- 刻意的群像只放白名单：目前只有墨府护院（`mofu_huyuan`，南城墨府门口站四个）。
- 另有两组 C++ 用例走真实入口：`tests/MapLinkTests.cpp` 用真 `WorldScene::tryStep` 按住方向把每道门走穿，断言不回弹（「只开这道门」与「门全开」两种存档状态各走一遍）；`tests/NpcPresenceTests.cpp` 按目标链逐步推进剧情，断言**同一个具名人物任何时刻只在一张图上**，七玄门考官（通用考官）与墨府护院列为群像。

**人物在场的写法（没有门禁能全管住，靠这几条自觉）**

- 在场只由地图对象的 `visible_flag` / `hidden_flag` 决定，Lua 没有显隐接口。出场、退场的旗标照着这个人的剧情走：演完这一幕就撤场，换了地方就在新地方另摆一个对象、旗标首尾相接。
- 挂在 NPC 身上的剧情脚本**没有 once**（`WorldScene::interact` 对 NPC 不查任何旗标），同一段戏若还挂在一处 once 触发器上，NPC 那一路要么随场撤掉，要么脚本开头判「已演过」只说一句。否则走过去一按就把旧剧情整段重演，还会把当时的选择旗标重写一遍。
- 路径行动（`data/pathactions/`）的时段必须落在 NPC 的在场区间里（规则 26 查）。

---

## 2. 这一次改了什么

### 2.1 出入口

| 衔接 | 原来 | 现在 |
| --- | --- | --- |
| 韩家村 ↔ 青牛镇 | 两头都在南边，来回弹（反馈里那一对） | 村口挪到北墙（补了文案里那棵老槐树）；青牛镇→彩霞山道→七玄门一路向北 |
| 炼骨崖 ↔ 神手谷 | 崖顶北出、落在谷北口，来回弹 | 神手谷南北对调：南口接炼骨崖，北口接药圃；药圃的谷口从北墙挪到南墙（入谷第一课挪进新门洞） |
| 神手谷堂屋门 ↔ 密室 | 往北进门、落在密室北头，出口也在北，来回弹 | 密室石阶甬道挪到南墙，摊牌触发照旧把两格占满；旧甬道留成壁龛 |
| 南城 ↔ 汇源客栈 | 往南进门、落在客栈南门口，来回弹 | 客栈门挪到南陵街北侧朝南的门面上，往北进门 |
| 药圃 → 崖壁 | 东口出去落在崖脚南口、面朝北（拐 90°） | 崖壁入口挪到西侧 |
| 暗道 ↔ 密室 | 暗道北口 ↔ 密室西墙暗门（拐 90°） | 暗道通密室的口子挪到东头，加一段北廊；旧北口留成死胡同 |
| 西城 ↔ 南城 | 西城南口 ↔ 南城西口（拐 90°） | 南城这头挪到北口（西头窄弄堂北口），与文案「往南走了一刻钟，街面宽了」一致 |

从可走改成不可走的格子：各处旧门洞（踩上去就换图，只有被闸门拦下的那一刻人才站在门格上），加上村口老槐的树干 (16,3)、药圃新门洞两侧的门垛 (17,28)(20,28)。这几格四邻都有可走的格子，旧存档就算正落在上面，读档后也能自己走出来（`tryStep` 只查目标格）。旧夹道、旧甬道一律留成空地或壁龛，不砌死——被关在一整段石头里的那种才会卡死。

### 2.2 人物

| 问题 | 原来 | 现在 |
| --- | --- | --- |
| 青牛镇三个「卖药郎」 | 六个路人借用三个打探 NPC 的 role：卖药郎、脚夫、酒客各三个 | 六个路人各换一个新 role：镇上老汉、镇上妇人、车把式、茶摊伙计、镇上孩童、磨刀匠（新角色 + 外观 + 精灵） |
| 南城两个「南城街坊」 | 路人借用街坊的 role | 按他自己的打探文案改成「过路汉子」 |
| 张铁一人三地 | 彩霞山道、炼骨崖、居所三处都不挂旗标 | 山道到考核为止 → 崖底从考完到收徒 → 居所只在第 2 章 |
| 墨大夫一人多地、死后还在 | 炼骨崖、神手谷两处不挂旗标；药圃、密室只挂了退场 | 崖底放榜后才下来、收徒后离开 → 神手谷入谷安顿到第 1 章末 → 药圃第 2 章 → 密室从归谷到暴毙 |
| 厉飞雨、卖药郎第 4 章一人两地 | 七玄门各堂、山下镇两处从开局就在 | 两处都从第 3 章末（`ch03.done`）起才在；外刃堂的第 3 章版到 `ch03.done` 为止；青牛镇卖药郎第 1 章末就走 |
| 第 1 章剧情重演 | 找三叔、韩母、山道张铁、镇上考官、崖下考官说话，会把那一幕整段重演（连二选一带旗标） | 三叔、韩母谈完就从村里撤场，天亮在村口老槐树下各有一处（话别）；另外三处脚本开头判「已演过」只说一句 |
| 剧情能跳过 | 没见三叔先找韩母→直接话别出村；考核前找崖下墨大夫→先被收徒 | 村里的韩母改挂 `hanmu.lua`（只说「三叔等你」）；崖下墨大夫放榜后才出现 |
| 第 1 章那批人第 4、5 章还站着 | 考官、三个打探 NPC、三叔、韩母到第 5 章仍在原地（路径行动的时段收了，人没收） | 各带退场旗标；青牛镇之后只剩六个常住镇民 |

新文案两句：`ch01.cunkou.mu_wait`（没见三叔时的韩母）、`ch01.climb.kg_again`（考完之后的崖下考官）。

---

## 3. 世界连通（按章）

- **第 1 章一线，一路向北**：韩家村 →北 青牛镇 →北 彩霞山道 →北 七玄门 →西北 炼骨崖 →北（翻过崖顶）神手谷南口。剧情次序同此：村口话别 → 进镇报名 → 次日山道遇张铁 → 崖下应考（第 7 节）。
- **神手谷一带（第 2、3 章）**：神手谷 →北 药圃；药圃 →西 居所、→东 崖壁、→北（东北角小道出谷）外刃堂。神手谷 →西 谷外 →北 暗道 →东 密室；神手谷堂屋门（往北进）→ 密室南口；密室 →东 藏书处。神手谷 → 谷外 → 暗道 → 密室 → 神手谷是一个环，方向首尾对得上（往西出谷、往北进暗道、往东进密室，再往南出密室回到谷里）。
- **第 4 章**：韩家村 →东 山下镇 →北 七玄门各堂；各堂 →西北 落日峰、→东 演武场。
- **第 5 章**：韩家村 →东（下口）岚州渡口 ~船~> 嘉元城西城 →南 南城；南城墨府大门（往北进）→ 墨府；南城客栈门（往北进）→ 汇源客栈；墨府 ~马~> 独霸山庄（只进不出，得手后脚本送回墨府）。
- 第 3 章末到第 4 章开头要从谷外一路走回韩家村东口（谷外 → 神手谷 → 炼骨崖 → 七玄门 → 彩霞山道 → 青牛镇 → 韩家村），这是既有设计，目标提示会跨图指路。

---

## 4. 出入口总表（44 道门）

| 图 | 出口 | 通往 | 落点（朝向） | 闸门 |
| --- | --- | --- | --- | --- |
| 彩霞山道 `ch01_caixiashan` | 南口（4,29） | 青牛镇 | 青牛镇 `spawn_from_caixiashan`（23,2）朝南 | — |
| 彩霞山道 `ch01_caixiashan` | 北口（33,0） | 七玄门 | 七玄门 `spawn_main`（12,10）朝北 | 山道遇张铁（`ch01.zhangtie_met`） |
| 韩家村 `ch01_hanjiacun` | 北口（19,0） | 青牛镇 | 青牛镇 `spawn_from_hanjiacun`（23,33）朝北 | 村口话别（`ch01.muqin_bie`） |
| 韩家村 `ch01_hanjiacun` | 东口（39,14） | 山下镇 | 山下镇 `spawn_from_hanjiacun`（1,14）朝东 | 第 3 章末（离谷）（`ch03.done`） |
| 韩家村 `ch01_hanjiacun` | 东口（39,20） | 岚州渡口 | 岚州渡口 `spawn_from_hanjiacun`（1,14）朝东 | 第 4 章末（东去）（`ch04.done`） |
| 炼骨崖 `ch01_liangu_ya` | 南口（15,39） | 七玄门 | 七玄门 `spawn_from_liangu_ya`（4,3）朝南 | — |
| 炼骨崖 `ch01_liangu_ya` | 北口（15,0） | 神手谷 | 神手谷 `spawn_from_liangu_ya`（19,28）朝北 | 放榜（`ch01.fangbang_done`） |
| 青牛镇 `ch01_qingniuzhen` | 南口（23,35） | 韩家村 | 韩家村 `spawn_cunkou`（20,2）朝南 | — |
| 青牛镇 `ch01_qingniuzhen` | 北口（23,0） | 彩霞山道 | 彩霞山道 `spawn_from_qingniuzhen`（4,27）朝北 | 青牛镇报名（`ch01.baoming_done`） |
| 七玄门 `ch01_qixuanmen` | 门（11,16） | 彩霞山道 | 彩霞山道 `spawn_from_qixuanmen`（33,2）朝南 | — |
| 七玄门 `ch01_qixuanmen` | 门（3,1） | 炼骨崖 | 炼骨崖 `spawn_from_qixuanmen`（15,37）朝北 | — |
| 神手谷 `ch01_shenshougu` | 南口（19,29） | 炼骨崖 | 炼骨崖 `spawn_from_shenshougu`（15,2）朝南 | — |
| 神手谷 `ch01_shenshougu` | 北口（19,0） | 神手谷药圃 | 神手谷药圃 `spawn_from_shenshougu`（18,28）朝北 | 第 1 章末（`ch01.done`） |
| 神手谷 `ch01_shenshougu` | 西口（0,13） | 神手谷外 | 神手谷外 `spawn_from_shenshougu`（38,14）朝西 | 第 2 章末（`ch02.done`） |
| 神手谷 `ch01_shenshougu` | 门（8,9） | 神手谷密室 | 神手谷密室 `spawn_from_shenshougu`（11,16）朝北 | 墨大夫归谷（`ch03.mo_gui_gu`） |
| 谷中居所 `ch02_jusuo` | 东口（23,8） | 神手谷药圃 | 神手谷药圃 `spawn_from_jusuo`（1,13）朝东 | — |
| 七玄门外刃堂 `ch02_wairentang` | 南口（23,35） | 神手谷药圃 | 神手谷药圃 `spawn_from_wairentang`（33,2）朝南 | — |
| 谷中崖壁 `ch02_yabi` | 西口（0,32） | 神手谷药圃 | 神手谷药圃 `spawn_from_yabi`（38,21）朝西 | — |
| 神手谷药圃 `ch02_yaopu` | 南口（18,29） | 神手谷 | 神手谷 `spawn_from_ch02_yaopu`（19,1）朝南 | — |
| 神手谷药圃 `ch02_yaopu` | 西口（0,13） | 谷中居所 | 谷中居所 `spawn_from_yaopu`（22,8）朝西 | 入谷第一课（`ch02.renyao_done`） |
| 神手谷药圃 `ch02_yaopu` | 北口（33,0） | 七玄门外刃堂 | 七玄门外刃堂 `spawn_from_yaopu`（23,33）朝北 | 第一次打坐（`ch02.dazuo_done`） |
| 神手谷药圃 `ch02_yaopu` | 东口（39,21） | 谷中崖壁 | 谷中崖壁 `spawn_from_yaopu`（1,32）朝东 | 段二开始（`ch02.duan2_start`） |
| 谷中暗道 `ch03_andao` | 东口（39,5） | 神手谷密室 | 神手谷密室 `spawn_from_andao`（4,11）朝东 | — |
| 谷中暗道 `ch03_andao` | 南口（19,31） | 神手谷外 | 神手谷外 `spawn_from_andao`（6,1）朝南 | — |
| 谷中藏书处 `ch03_cangshu` | 西口（0,12） | 神手谷密室 | 神手谷密室 `spawn_from_cangshu`（19,8）朝西 | — |
| 神手谷外 `ch03_guwai` | 东口（39,14） | 神手谷 | 神手谷 `spawn_from_ch03_guwai`（2,13）朝东 | — |
| 神手谷外 `ch03_guwai` | 北口（6,0） | 谷中暗道 | 谷中暗道 `spawn_from_guwai`（19,29）朝北 | 暗道一战（`ch03.andao_zhan`） |
| 神手谷密室 `ch03_mishi` | 南口（11,17） | 神手谷 | 神手谷 `spawn_from_ch03_mishi`（8,10）朝南 | — |
| 神手谷密室 `ch03_mishi` | 门（21,8） | 谷中藏书处 | 谷中藏书处 `spawn_from_mishi`（2,12）朝东 | 服尸虫丸（`ch03.shichong_wan`） |
| 神手谷密室 `ch03_mishi` | 门（2,11） | 谷中暗道 | 谷中暗道 `spawn_from_mishi`（38,5）朝西 | 备毒（`ch03.beidu_done`） |
| 七玄门各堂 `ch04_getang` | 南口（23,35） | 山下镇 | 山下镇 `spawn_from_getang`（19,2）朝南 | — |
| 七玄门各堂 `ch04_getang` | 北口（2,0） | 落日峰 | 落日峰 `spawn_from_getang`（19,28）朝北 | 留在门中（`ch04.liuxia`） |
| 七玄门各堂 `ch04_getang` | 东口（47,20） | 演武场 | 演武场 `spawn_from_getang`（1,20）朝东 | 习御风决（`ch04.yufeng_xue`） |
| 落日峰 `ch04_luorifeng` | 南口（19,29） | 七玄门各堂 | 七玄门各堂 `spawn_from_luorifeng`（2,1）朝南 | — |
| 山下镇 `ch04_shanxiazhen` | 西口（0,14） | 韩家村 | 韩家村 `spawn_from_ch04_shanxiazhen`（38,14）朝西 | — |
| 山下镇 `ch04_shanxiazhen` | 北口（19,0） | 七玄门各堂 | 七玄门各堂 `spawn_from_shanxiazhen`（23,34）朝北 | — |
| 演武场 `ch04_yanwuchang` | 西口（0,20） | 七玄门各堂 | 七玄门各堂 `spawn_from_yanwuchang`（46,20）朝西 | — |
| 岚州渡口 `ch05_dukou` | 西口（0,14） | 韩家村 | 韩家村 `spawn_from_ch05_dukou`（38,20）朝西 | — |
| 汇源客栈 `ch05_kezhan` | 南口（15,23） | 嘉元城南城 | 嘉元城南城 `spawn_from_kezhan`（41,16）朝南 | — |
| 墨府 `ch05_mofu` | 南口（23,35） | 嘉元城南城 | 嘉元城南城 `spawn_from_mofu`（23,15）朝南 | — |
| 嘉元城南城 `ch05_nancheng` | 北口（12,0） | 嘉元城西城 | 嘉元城西城 `spawn_from_nancheng`（8,33）朝北 | — |
| 嘉元城南城 `ch05_nancheng` | 门（23,14） | 墨府 | 墨府 `spawn_from_nancheng`（23,33）朝北 | 登门（`ch05.dengmen`） |
| 嘉元城南城 `ch05_nancheng` | 门（41,15） | 汇源客栈 | 汇源客栈 `spawn_from_nancheng`（15,21）朝北 | — |
| 嘉元城西城 `ch05_xicheng` | 南口（8,35） | 嘉元城南城 | 嘉元城南城 `spawn_from_xicheng`（12,1）朝南 | 码头盯梢一战（`ch05.zhuishao`） |

脚本传送（5 处，不受规则 29 约束）：

| 脚本 | 目标图 | 落点 |
| --- | --- | --- |
| `ch05/jianmian.lua` | 墨府 `ch05_mofu` | （40,3） |
| `ch05/shangchuan.lua` | 嘉元城西城 `ch05_xicheng` | （44,17） |
| `ch05/shangyue.lua` | 墨府 `ch05_mofu` | （23,30） |
| `ch05/yeru.lua` | 墨府 `ch05_mofu` | （3,3） |
| `ch05/zhuwu.lua` | 独霸山庄 `ch05_dubashanzhuang` | （4,25） |

---

## 5. 各图人物（42 个 NPC 对象）

「出场 / 退场」是地图对象上的 `visible_flag` / `hidden_flag`；「开局即在」「常驻」表示没有那一侧的旗标。图本身什么时候走得到，看第 4 节的闸门。

| 图 | 对象 | 名牌（role） | 位置 | 出场 | 退场 | 脚本 |
| --- | --- | --- | --- | --- | --- | --- |
| 彩霞山道 | `npc_zhang_tie` | 张铁（`zhang_tie`） | （18,16） | 开局即在 | 炼骨崖考完（`ch01.climb_done`） | `ch01/shandao_zhangtie.lua` |
| 韩家村 | `npc_han_sanshu` | 韩三叔（`han_sanshu`） | （20,18） | 开局即在 | 见过三叔（`ch01.sanshu_met`） | `ch01/sanshu.lua` |
| 韩家村 | `npc_han_mu` | 韩母（`han_mu`） | （18,21） | 开局即在 | 见过三叔（`ch01.sanshu_met`） | `ch01/hanmu.lua` |
| 韩家村 | `npc_han_mu_cunkou` | 韩母（`han_mu`） | （18,2） | 见过三叔（`ch01.sanshu_met`） | 村口话别（`ch01.muqin_bie`） | `ch01/cunkou_bie.lua` |
| 韩家村 | `npc_han_sanshu_cunkou` | 韩三叔（`han_sanshu`） | （21,2） | 见过三叔（`ch01.sanshu_met`） | 村口话别（`ch01.muqin_bie`） | `ch01/cunkou_bie.lua` |
| 炼骨崖 | `npc_qixuan_kaoguan` | 七玄门考官（`qixuan_kaoguan`） | （13,34） | 开局即在 | 墨大夫收徒（`ch01.shoutu_done`） | `ch01/lianguya_kaohe.lua` |
| 炼骨崖 | `npc_zhang_tie` | 张铁（`zhang_tie`） | （11,35） | 炼骨崖考完（`ch01.climb_done`） | 墨大夫收徒（`ch01.shoutu_done`） | — |
| 炼骨崖 | `npc_mo_daifu` | 墨大夫（`mo_daifu`） | （22,35） | 放榜（`ch01.fangbang_done`） | 墨大夫收徒（`ch01.shoutu_done`） | `ch01/modaifu_shoutu.lua` |
| 青牛镇 | `npc_qixuan_kaoguan` | 七玄门考官（`qixuan_kaoguan`） | （37,21） | 开局即在 | 山道遇张铁（`ch01.zhangtie_met`，次日领人上了山；2026-09-28 前是考完） | `ch01/qingniuzhen_baoming.lua` |
| 青牛镇 | `npc_zhen_min_jiuke` | 镇上酒客（`zhen_min_jiuke`） | （15,8） | 开局即在 | 第 1 章末（`ch01.done`） | `ch01/zhenmin_jiuke.lua` |
| 青牛镇 | `npc_zhen_min_yaofan` | 卖药郎（`zhen_min_yaofan`） | （10,9） | 开局即在 | 第 1 章末（`ch01.done`） | `ch01/zhenmin_yaofan.lua` |
| 青牛镇 | `npc_zhen_min_jiaofu` | 脚夫（`zhen_min_jiaofu`） | （24,28） | 开局即在 | 第 1 章末（`ch01.done`） | `ch01/zhenmin_jiaofu.lua` |
| 青牛镇 | `npc_lu_ren_a` | 镇上老汉（`qingniu_laohan`） | （23,12） | 开局即在 | 常驻 | — |
| 青牛镇 | `npc_lu_ren_b` | 镇上妇人（`qingniu_furen`） | （24,21） | 开局即在 | 常驻 | — |
| 青牛镇 | `npc_lu_ren_c` | 车把式（`qingniu_chebashi`） | （23,30） | 开局即在 | 常驻 | — |
| 青牛镇 | `npc_lu_ren_d` | 茶摊伙计（`qingniu_chatan_huoji`） | （40,23） | 开局即在 | 常驻 | — |
| 青牛镇 | `npc_lu_ren_e` | 镇上孩童（`qingniu_haitong`） | （34,24） | 开局即在 | 常驻 | — |
| 青牛镇 | `npc_lu_ren_f` | 磨刀匠（`qingniu_modaojiang`） | （8,18） | 开局即在 | 常驻 | — |
| 神手谷 | `npc_mo_daifu` | 墨大夫（`mo_daifu`） | （12,10） | 入谷安顿（`ch01.shenshougu_arrived`） | 第 1 章末（`ch01.done`） | `ch01/shenshougu_koujue.lua` |
| 谷中居所 | `npc_zhang_tie` | 张铁（`zhang_tie`） | （10,6） | 第 1 章末（`ch01.done`） | 第 2 章末（`ch02.done`） | `ch02/zhangtie.lua` |
| 七玄门外刃堂 | `npc_wairentang_yaoshang` | 外刃堂药商（`wairentang_yaoshang`） | （9,21） | 开局即在 | 常驻 | `ch02/yaoshang.lua` |
| 七玄门外刃堂 | `npc_tongmen_luchun` | 卢春（`tongmen_luchun`） | （18,17） | 开局即在 | 常驻 | `ch02/luchun.lua` |
| 七玄门外刃堂 | `npc_tongmen_maliu` | 马六（`tongmen_maliu`） | （29,17） | 开局即在 | 常驻 | `ch02/maliu.lua` |
| 七玄门外刃堂 | `npc_li_feiyu` | 厉飞雨（`li_feiyu`） | （20,22） | 厉飞雨人情结下（`ch02.renqing_jiexia`） | 墨大夫归谷（`ch03.mo_gui_gu`） | `ch02/lifeiyu.lua` |
| 七玄门外刃堂 | `npc_li_feiyu_ch03` | 厉飞雨（`li_feiyu`） | （20,22） | 墨大夫归谷（`ch03.mo_gui_gu`） | 第 3 章末（离谷）（`ch03.done`） | `ch03/lifeiyu.lua` |
| 神手谷药圃 | `npc_yaopu_guanshi` | 药圃管事（`yaopu_guanshi`） | （17,9） | 开局即在 | 墨大夫归谷（`ch03.mo_gui_gu`） | `ch02/guanshi.lua` |
| 神手谷药圃 | `npc_yaopu_guanshi_ch03` | 药圃管事（`yaopu_guanshi`） | （17,9） | 墨大夫归谷（`ch03.mo_gui_gu`） | 常驻 | `ch03/guanshi.lua` |
| 神手谷药圃 | `npc_mo_daifu` | 墨大夫（`mo_daifu`） | （12,17） | 第 1 章末（`ch01.done`） | 段五想起瓶子（`ch02.xiangqi_ping`） | `ch02/modaifu.lua` |
| 神手谷密室 | `npc_mo_daifu` | 墨大夫（`mo_daifu`） | （11,9） | 墨大夫归谷（`ch03.mo_gui_gu`） | 墨居仁暴毙（`ch03.mo_siwang`） | `ch03/mojuren.lua` |
| 七玄门各堂 | `npc_wang_juechu` | 王绝楚（`wang_juechu`） | （24,4） | 开局即在 | 常驻 | `ch04/wang_juechu.lua` |
| 七玄门各堂 | `npc_li_feiyu` | 厉飞雨（`li_feiyu`） | （20,23） | 第 3 章末（离谷）（`ch03.done`） | 常驻 | `ch04/feiyu.lua` |
| 山下镇 | `npc_zhen_min_yaofan` | 卖药郎（`zhen_min_yaofan`） | （14,22） | 第 3 章末（离谷）（`ch03.done`） | 常驻 | `ch04/yaofan.lua` |
| 岚州渡口 | `npc_chuanjia` | 船家（`chuanjia`） | （24,10） | 开局即在 | 常驻 | `ch05/chuanjia.lua` |
| 墨府 | `npc_anshao` | 墨府护院（`mofu_huyuan`） | （24,22） | 夜入墨府（`ch05.yeru`） | 登门（`ch05.dengmen`） | — |
| 墨府 | `npc_mo_fengwu` | 墨凤舞（`mo_fengwu`） | （8,7） | 登门（`ch05.dengmen`） | 第 5 章末（`ch05.done`） | `ch05/fengwu.lua` |
| 墨府 | `npc_yan_ge` | 燕歌（`yan_ge`） | （30,16） | 花园见吴剑鸣（`ch05.huayuan`） | 燕歌讨教（`ch05.yange`） | `ch05/yange_npc.lua` |
| 嘉元城南城 | `npc_huyuan_a` | 墨府护院（`mofu_huyuan`） | （20,15） | 开局即在 | 常驻 | — |
| 嘉元城南城 | `npc_huyuan_b` | 墨府护院（`mofu_huyuan`） | （21,15） | 开局即在 | 常驻 | — |
| 嘉元城南城 | `npc_huyuan_c` | 墨府护院（`mofu_huyuan`） | （26,15） | 开局即在 | 常驻 | — |
| 嘉元城南城 | `npc_huyuan_d` | 墨府护院（`mofu_huyuan`） | （27,15） | 开局即在 | 常驻 | — |
| 嘉元城南城 | `npc_jiefang` | 南城街坊（`jiayuan_jiefang`） | （30,19） | 开局即在 | 常驻 | `ch05/jiefang.lua` |
| 嘉元城南城 | `npc_luren` | 过路汉子（`jiayuan_guolu_hanzi`） | （40,17） | 开局即在 | 常驻 | — |

---

## 6. 开新图、摆新人物时照这几条做

1. 摆出入口时先想「玩家是往哪个方向走出上一张图的」，落点放在这张图对应的那一边、面朝同一个方向；跑 `python tools/validate.py`，规则 29 会拦住掉头与拐弯。贴边的门若左右两格也走得通，会多出两个进门方向、各查一遍——只想算一个方向就把门两侧封墙。
2. 每个具名人物写清出场、退场两个旗标，与他在别的图上的站位首尾相接；同一个人换地方就在新地方另摆一个对象。`tests/NpcPresenceTests.cpp` 会按目标链查「一人两地」；用了不在目标链上的旗标，要在它的 `kOffChainFlags` 表里补一行（插在哪两步之间、哪个脚本置它），测试会回头核那个脚本。
3. 路人、群众一人一个 role，名牌不重名（规则 30）；确实是一群人（护院、兵丁）才进白名单。**规则 30 比的是 role_id 不是显示名**：新 role 照抄旧 role 的 `name`，照样会冒出第二个「卖药郎」。新 role 要在 `data/visual/looks.json` 给外观、跑 `tools/artgen/artgen.py` 出精灵，`sprite_preview.py --coverage` 缺图为 0。
4. 剧情脚本同时挂在 NPC 与 once 触发器上时，NPC 那一路要么随场撤掉、要么开头判「已演过」只说一句。
5. 第 4、5 节是本次用一次性脚本从 tmj 导出的快照，没有门禁盯着它漂不漂；改了地图照表头手改。
6. 交接存档与手测检查点也是「存档」：人物按旗标撤场，缺旗标的存档里他们会又站出来。手搭章首章末时从目标链读完成旗标，不要手列（`tests/Ch03SliceTests.cpp` 的 `applyChapterTwoEnding`）。
7. 目标链的次序要与门闸对得上：每一步从上一步的目标图出发，只穿当时开着的门也得走得到（`tests/ObjectiveTests.cpp` 的 `ObjectiveRoute` 那三条，第 7 节）。脚本演完把人 `teleport` 走的，登进它的 `kScriptTransfers`；门的钥匙若不是哪一步的 done_flag，登进 `kOffChainDoorKeys`。漏登用例会红，并说出是哪个脚本、哪把钥匙。

---

## 7. 第 1 章目标链与门闸（2026-09-28）

**问题**（独立审查 M3，改动前就有）：目标链把「山道遇张铁」（`ch01.zhangtie_met`，彩霞山道）排在「青牛镇报名」（`ch01.baoming_done`）前面，台词也是先上山道、日落前进镇报名；地图却要先报名，青牛镇通山道的门（`portal_to_caixiashan`）才开。于是话别之后目标指着山道，指路只走开着的门（`rules::nextPortalToward`），韩家村、青牛镇一道门都不亮；报了名，进度按「走得最远的那一步」往后一跳，「到青牛镇报名处登名」那一行从没露过面。来历：2026-09-20 的第 1 章校对（`docs/ch01-review.md`）开了两个方子，阻断-2（给各图补门闸，按地图次序挂）落实了，应改-1（按地图次序改山道台词、对调设计文档节点表的 3/4 编号）没有；后来写目标链照的是那张没改的节点表。

**原著**（novel.py 读 ch1–5）：ch2 三叔带韩立坐车三天到青牛镇（他看着的春香酒楼在那儿）；ch3 七玄门的车从青牛镇往西走，第五天傍晚到彩霞山、先歇一宿；ch4 日出开考、正午为限；张铁 ch5 才出场，就在崖上。村 → 青牛镇 → 彩霞山是原著的地理，「山道结识」「镇上报名桌」和游戏里的时辰（未时点人、午后开考）都是改编（`docs/ch01-design.md` 第 2 节修订）。

**改法**（设计拍板：按地图次序改目标链与台词，门与美术不动；复核后又改了镇上考官的撤场旗标，见本节末）：

- 目标链 n3、n4 对调并改名：`n3_baoming`（青牛镇）→ `n4_shandao`（彩霞山道），文案 key 跟着改。进度只看旗标，存档不存目标 id，老存档不受影响：报过名的存档读进来，目标直接是「沿彩霞山道往七玄门去」，那道门已经开了。
- 时间线统一成：当天进镇报名、宿在镇上 → 次日天不亮从镇口动身，晌午在山道遇张铁 → 午后崖下应考 → 傍晚放榜。改了 8 句（前 7 句是设计拍板时列的，第 8 句是文案复核时查出的同类）：

| key | 原来 | 现在 | 为什么 |
| --- | --- | --- | --- |
| `ch01.shandao.sanshu_cui` | 日头偏西之前得进镇，晚了报名的桌子就收了 | 崖下未时点人，误了时辰，昨天名字登得再早也不作数 | 名已经报过了 |
| `ch01.shandao.opt_keep` | 前头就到镇上了 | 前头就是山门了 | 镇子已在身后 |
| `ch01.shandao.zt_ask` | 是不是也去赶明天那一场 | 昨天报名排的是哪一队 | 考核就在当天 |
| `ch01.zhenshang.chunxiang` | 今日歇了半天生意就为领他们上山 | 明日歇一天生意，就为领他上山 | 进镇时还没遇见张铁；上山是次日 |
| `ch01.shandao.noon` | 山道往上拐了…… | 开头加「天不亮从镇口动身。」，删掉「往上」 | 交代过了一夜、是从镇上来的；全句 49 字，对话框一行放得下（截图口实拍时 53 字那一版第二行只剩一个「片。」） |
| `ch01.shandao.sanshu_qixuan` | 指着远处一座大山，说那就是彩霞山 | 仰头看了看头顶那座压着云的高峰，说连它带脚下这一片山统叫彩霞山 | 人已经站在彩霞山道上（校对应改-1 附带的那处别扭） |
| `ch01.shandao.zt_hungry` | 早上出门急，干粮忘在灶台上了 | 天不亮出门走得急，干粮落在昨晚借宿的那户人家了 | 他爹昨天就把他送到了岔路口，他昨夜宿在镇上 |
| `ch01.shandao.opt_share` | 何况路还长着，谁也说不准前头有没有地方讨饭 | 何况后半天还要上崖，空着肚子，手上使不出劲 | 同一个二选一里另一支说「前头就是山门了」 |

附带对上的一处：上崖前张铁说「方才那半块饼他记着」（`ch01.climb.zt_shared`），旧次序下其实隔了一夜，现在是同一天。脚本只改了注释里的节点编号。

**新用例**（`tests/ObjectiveTests.cpp` 末尾，`ObjectiveRoute` 三条）：

- `EveryObjectiveStepCanBeReachedThroughTheDoorsOpenAtThatStep`：真 `Application` 读真 `data/` 与 `maps/`，从空存档起沿整条目标链（第 1–5 章，78 步）逐步推进；每一步从「做完上一步时人在的那张图」出发，跟着 `WorldScene::guidePortal`（描门、报「先到」问的都是它）一道门一道门走到这一步的目标图。改数据之前跑，1406 条里只红这一条、只报第 1 章 `n3_shandao`：「在 ch01_hanjiacun 上一道门也不亮。此刻走得到的图：ch01_hanjiacun、ch01_qingniuzhen；关着的门：……ch01_qingniuzhen/portal_to_caixiashan → ch01_caixiashan（差 ch01.baoming_done）」。
- `TheRouteWalkCatchesTheOldChapterOneOrder`：判据自己的牙——在内存里把山道挪回报名之前，必须报出、而且只报出山道那一步。
- `TheTransferAndDoorKeyTablesMatchTheScripts`：第 5 章 5 处脚本传送（上船、夜入、见面礼、出征、赏月回府）登在 `kScriptTransfers`，唯一一把不在链上的门钥匙 `ch01.done` 登在 `kOffChainDoorKeys`；逐条核脚本原文（顶格的 `flag.set` / `teleport`），并且与扫出来的集合比相等，漏登、多登都红。真数据里两个方向的对照都有：5 个传送脚本必须扫得出来；`ch03/ligu.lua`、`ch04/huicun.lua`、`ch04/dongqu.lua` 置着链上的旗标、只在注释里写了「不调 teleport()」，必须扫不出来。
- 变异验证（验完已原样恢复）：删掉上船那条登记、把 `ch01.done` 改成不存在的钥匙，三条全红——走门那条报出第 2 章 n1_renyao、第 3 章三步（都要穿神手谷北门）与第 5 章 n03_matou（从渡口到不了西城），核表那条三个方向各报一处。

**没改的 / 取舍**：门、位置、美术零改动。原先「路上结识、同去报名」的同行感没了；张铁在镇上不露面（本来镇上也没摆他）。

**文案复核之后追加（用户拍板，2026-09-28）**：

- 镇上考官（`npc_qixuan_kaoguan`）的退场旗标 `ch01.climb_done` → `ch01.zhangtie_met`：次日遇过张铁再走回镇上，他从前还坐着说「明日天不亮在镇口等齐」。地图里只变了这一个值（`ch01_qingniuzhen.tmj`），路径行动 `kaoguan_dating` 的时段跟着收（规则 26）。
- 与次序无关、复核时顺带查出的几处：饼数「拢共只有一块半」（两块留一块再掰一角，算不出来）改成「只剩大半块」；「春香楼」→「春香酒楼」、「原先叫七绝门」→「又叫七绝门」（原著 ch2）；目标 n8「随墨大夫进神手谷」→「到神手谷见墨大夫」（放榜时考官说的是「让他们两个自己去，不必人送」）；`flags.json` 里 `liu_bing` 的「弟妹」、`data/roles/zhang_tie.json` 备注里的「首次登场 ch4、同乡、十岁」、山道脚本注释里的「分别时」一并改对。
- 原著「五年一次」收徒，游戏按每年一考写（7 处台词）：只在 `docs/ch01-design.md` 第 1 节登记为改编，台词不动。
- 复核时还查出两处台词与地图不合，随后也按「台词跟地图走」改了（用户拍板，地图与美术不动）：
  - **报名桌在哪**：原来有四种说法——「明日在镇口收徒」（`ch01.sanshu.greet`）、「镇口空地上支着两张长桌」（`ch01.zhenshang.crowd`）、「镇东那张桌子」（`ch01.block.caixiashan`）、生成器注释「镇北大院」；地图上报名广场在主街以东（x 32–42，y 19–25），离南北两道镇门都远。统一成「镇东那片空场」，「镇口」只指镇门：`sanshu.greet` 改「明日在青牛镇收徒」，`zhenshang.crowd` 改「镇东那片空场上」，`zhenmin.jiaofu_1` 的「从镇东替人挑行李挑过来」改「从镇口」（原句读着像行李是从报名处挑出来的），生成器注释改「镇东大院」。原著 ch2 里七玄门的车是开到镇子东头三叔的春香酒楼接人的，报名放在镇东与此相合；游戏地图上的春香酒楼另摆在主街西侧（`facility_chunxiang_lou`，16,8），这次没动。
  - **报名的两张长桌**（截图口实拍时查出，用户拍板补地图、文案不动）：台词「镇东那片空场上支着两张长桌」「桌后坐着……面前摊着册子，笔头在砚台里顿了顿」，广场上却只有告示板和考官。`tools/mapgen/genmaps.py` 在 (35–36, 22)、(38–39, 22) 各补一张 2 格长案（道具格，不可走），考官 (37,21) 站在两案后头，正前方 (37,22) 留一格过道，正面、两侧都找得到他说话；`data/visual/maps.json` 把两块指成 `desk`（案上纸、砚、笔），并用一块 `zones` 把案子底下也指成广场石——推断只给能走的格铺石板，不指的话案子四周露一圈草。案子各 2 格，是因为 3 格长的会被美术工具当成「篱笆线」，两案之间那格过道就被画成一道门。地图只变了这 4 格，美术产物只变了 `assets/art/maps/ch01_qingniuzhen/below.png`。案子烘在底图里，第 4、5 章路过青牛镇时还在（与告示板同一类，见 `docs/handoff.md` 第 7 节）。
  - **入谷路线**：`ch01.shenshougu.road` 原写「从崖底绕了过去……山门在半山腰上……沿墙根往东走」，地图却是从崖底顺石阶上崖顶、从北口翻进谷南口，根本不经过山门。改成「顺着昨日墨大夫走的那道石阶往崖上去」（墨大夫点完名正是「转身上了石阶」）；`ch01.shenshougu.gate` 改成到了崖顶回望山门，「门开着，里头有人喊号子」这一眼保留。
