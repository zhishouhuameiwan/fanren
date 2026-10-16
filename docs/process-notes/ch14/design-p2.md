<!-- 第 14 章施工图·第二段（第 4–9 节；写手二，2026-10-13）。唯一事实来源：notes/ch14/research.md（下称「笔记」）；格式照 docs/ch10-design.md 第 4–9 节、细分照 docs/ch13-design.md 第 4–9 节。原著补核处注明章号（novel.py read 585）。旗标在表里一律省 `ch14.` 前缀。「拍板 N」＝ 16.1 第 N 条（照笔记 E 表编号）；「A N / B N / O-N」＝ 笔记 A、B 表行与 O.3 开放问题；teleport「#N」＝ 笔记 N.4 编号；K1–K14 ＝ 16.3 路径行动。 -->

## 4. 地图（新建 5 张，旧图 0 处修补）

| 地图 id | 名称（`display_name_key`）| 尺寸 | 室外 | 视觉预设（`data/visual/maps.json`）| default spawn | 出入口 |
| --- | --- | --- | --- | --- | --- | --- |
| `ch14_linhuotan` | 磷火潭（4.4 第 3 条）| 40×30 | ✗ | `indoor_stone` / `indoor` / backdrop `secret_room` | `tongdao`（黑石门内）| 无门；脚本入口 C1（`scripts/ch13/banyao.lua` 末行 `teleport("ch14_linhuotan")`，teleport #1）、n02 同图带坐标 #6；脚本出口 n07 #2 → `ch14_shuangfeng` |
| `ch14_shuangfeng` | 双峰岛·地下坊市与西岛 | 52×36 | ✗ | `cave` / `indoor` / backdrop `cave_tunnel` | `shudi`（树洞阶底）| 无门；脚本入口 #2（落 `shudi`）、n11 同图带坐标 #7、同图五元 #8 / #9；脚本出口 n12 #3 → `ch14_yaomu` |
| `ch14_yaomu` | 妖兽墓穴岛 | 44×36 | ✓ | `cliff` / `dusk` / backdrop `cliff_top_dusk` | `andi`（岸边）| 无门；脚本入口 #3；脚本出口 n15 #4 → `ch14_yinming` |
| `ch14_yinming` | 阴冥之地 | 96×80 | ✓ | `wild` / `night` / backdrop `mountain_forest_dusk` | `shanfu`（山腹鱼虾堆）| 无门；脚本入口 #4；脚本出口 n28 #5 → `ch14_minzhou` |
| `ch14_minzhou` | 海边小城（4.4 第 3 条）| 30×24 | ✓ | `dock` / `dusk` / backdrop `dock_river` | `haian`（海岸）| 无门；脚本入口 #5；**出口：无**（通关后的落脚处）|

**图与图之间没有一扇门**（笔记 G 表）：跨图全是脚本传送，三元 7 条（#1–#7，其中 #6、#7 同图带坐标）、五元 2 条（#8、#9，同图），编号照笔记 N.4，登 `tests/ObjectiveTests.cpp` 的 `kScriptTransfers`（T2）。主题、时辰、背景全是现有组合（2026-10-13 grep：`indoor_stone`、`secret_room`、`cave_tunnel`、`cliff_top_dusk`、`mountain_forest_dusk`、`dock_river` 都在 `maps.json` 或 `tools/artgen/backdrops.py` 第 703–713 行的 `BACKDROPS` 里）。全作地图 79 → **84**（≤ 85，笔记 B29；退路见 O-7）。

**为什么是这五张**（拍板 16）：皇明岛一段（561–566）不建图，`chuansong.lua` 一份脚本从入阵演到请护法（拍板 17、O-7）；双峰岛与西岛合一张，岛底地道是**改编**（原著飞过去，拍板 18、O-6）——**图鉴不标地道**；阴冥之地与暴风山合一张（暴风山在东北角，山道上行）；闵州是通关后的落脚处（拍板 29）。

**落地不触发 enter**（引擎口径 1）：五个 default spawn 与 #6、#7、#8、#9 的落点格上都不放任何对象；每张图第一个剧情带是落点前方横贯通道的触发带（n01、n08、n13、n16、n29），玩家迈出一步才响。通关驱动落地后要先迈一步。

### 4.1 每张图的挂点与在场 NPC

各 NPC 的出场 / 退场旗标以 5.1 为准，这里只列对象与位置；各挂点的 mode / once / guard / 完成旗标以第 3 节主线节点表为准。

**`ch14_linhuotan` 磷火潭·潭底洞府**（第一幕：n01–n07）
- 布局：黑石门内的通道（spawn `tongdao`）→ 嵌珍珠的通道 → 珊瑚大厅（玉桌）→ 珊瑚石壁后的密室（玉榻）；大厅另一侧是偏门石屋，再往里是炼器室（地火台与合炼法阵，A22）。**全图走得通**：密室、石屋、炼器室都有路连着大厅，门闸靠 guard 惰性、不靠隔断（规则 13）——这正是导读第 1 句「哪里都能走、哪里都有人看着」。
- 挂点：`trigger_tongdao`（n01，enter，spawn 北两格横贯带）；`trigger_shimen_baimu`（黑石门内侧，enter、once 否、guard `fengmou`、无 `set_flag`，只一句「白幕不开」；**与 spawn 不同格**，放在 spawn 南一格的门槛上）；`trigger_yuta`（n03，interact，玉榻）；`trigger_mishi_men`（n04，enter，密室门内横贯带）；`trigger_shiwu_zhenqi`（n05，interact，石屋内）；`trigger_zhenjiao`（n06，interact，阵角）；`trigger_lianqi_men`（n07，enter，炼器室石门内横贯带）。n02 挂 NPC `npc_feng_xi_ting`。
- 落点：#6（n02 → 密室）落在玉榻前，**不压 `trigger_yuta`、与 `trigger_mishi_men` 隔 ≥ 2 格**（笔记 N.4 #6）。
- 设施：`facility_dazuo_shiwu`（meditate，石屋内，与 `trigger_shiwu_zhenqi` 不同格；① 之前的回血处）。
- NPC：大厅 `npc_feng_xi_ting`、`npc_feng_xi_qun`、`npc_du_jiao_ting`、`npc_gui_yao_ting`；炼器室 `npc_feng_xi_lianqi`、`npc_du_jiao_lianqi`、`npc_gui_yao_lianqi`。K1–K3 挂 `npc_feng_xi_qun`、`npc_du_jiao_ting`、`npc_gui_yao_ting`。
- 走位次序靠 guard：n04 的门带在 n02 落进密室之前就在路上，`houqi` 未置、踩了不响；n07 的门带在玩家进炼器室赴 n06 时同样不响。

**`ch14_shuangfeng` 双峰岛·地下坊市与西岛**（第二幕：n08–n12）
- 布局：巨树树洞下的石阶底（spawn `shudi`）→ 横贯带 → 地下石厅（坊市，秘市未开，0 店）→ 客房一排 → 末间石屋；石厅另一头是地道口 → 地道（中段一道地裂，两岸本有绕行路）→ 西岛传送大厅。
- 挂点：`trigger_dixia`（n08，enter，spawn 前横贯带）；`trigger_moshi`（n11，interact，末间石屋内）；`trigger_xidao`（n10，enter，地道口横贯带）；`trigger_leidun_dong` / `trigger_leidun_xi`（雷遁捷径，五元 #8 / #9，见 4.4 第 4 条）；`trigger_zhenqian`（n12，enter，阵前横贯带）。n09 挂 NPC `npc_fan_furen`。
- 落点：#7（n11 雷遁）落在 `trigger_zhenqian` 南侧 ≥ 2 格；#8 / #9 落在对岸、不压对岸那一格触发器、离它 ≥ 1 格。
- 设施：`facility_dazuo_kefang`（meditate，客房；② 之前的回血处）。
- NPC：石厅 `npc_shoumen_laozhe`、`npc_yun_qin`、`npc_fan_furen`；西岛大厅 `npc_yaoyan_nvxiu`。K4–K6 挂守门老者、范夫人、妙音门女修。

**`ch14_yaomu` 妖兽墓穴岛**（第三幕：n13–n15）
- 布局：岸边（spawn `andi`）→ 一条上坡的路（≥ 8 格）→ 谷口 → 谷中法坛（寒玉棺、法阵，装饰）；谷口另一侧岔出一条去岛缘外礁的路。
- 挂点：`trigger_fatan`（n13，enter，谷口横贯带，离 spawn ≥ 8 格）；`trigger_gukou`（n14，interact，谷口盘坐处）；`trigger_daoyuan`（n15，enter，岛缘横贯带——**放在谷口岔出的那条路上，不在岸边到谷口的路上**，玩家打坐回血之后再走过去）。
- 设施：`facility_dazuo_gukou`（meditate，`require_flag ch14.fatan`，不写 deny，4.4 第 2 条；③ 之前的回血处）。
- NPC：岸边 `npc_yuan_yao_andi`（K7）；谷中 `npc_yuan_yao_fatan`；谷口一侧 `npc_juyuan_kuilei`（**图上只放 1 个**，十几个巨猿傀儡只在文案，A66；不压通路）。

**`ch14_yinming` 阴冥之地**（第四、五幕：n16–n28）
- 布局（西南 → 东北）：山腹鱼虾堆（spawn `shanfu`）→ 出山腹的横贯带 → 沙漠与乱石 → 村门横贯带 → 高墙村落（长老石屋、石台、仓库、住处）；村东沙漠一路到红土村；村西七里是猎兽处；东北暴风山，一条山道自山脚经雾区到山巅巨岩。
- 挂点：`trigger_shanfu`（n16）、`trigger_cunmen`（n17，乱石尽头）、`trigger_zhuchu_men`（n21，住处门外）、`trigger_hongtu`（n24，红土村外）、`trigger_shanjiao`（n26）、`trigger_wuqu`（n27）、`trigger_shandian`（n28）都是 enter 横贯带，后三条依次横在山道上；`trigger_shitai`（n19，石台）、`trigger_shibei`（n20，仓库第六块碑）、`trigger_shibei_yaowen`（Z2，另一块碑，与前者隔 ≥ 2 格）、`trigger_zhuchu_ting`（n23，住处石桌，与梅凝 NPC 不同格）、`trigger_lieshou`（n25，村西）都是 interact。n18 挂 NPC `npc_dazhanglao`，n22 挂 `npc_mei_ning_zhuchu`。
- 设施：**无打坐设施**（法力被封，纪律 80）；存档点见 4.4 第 1 条。
- NPC：村门内 `npc_shouhan`；村中 `npc_dazhanglao`、`npc_heishao`、`npc_baohuanzi`、`npc_cunmin`、`npc_baimian_zhanglao` / `npc_feng_tianji`（同一格先后两人）、`npc_yun_daoyou`、`npc_jin_daoyou`；住处内 `npc_mei_ning_zhuchu`；出村东路旁 `npc_mei_ning_hongtu`；红土村 `npc_hongtu_zhuanghan`；`npc_mei_ning_shan` 在红土村往村西猎场的路旁（他猎兽、她等裂缝的那段日子）；山巅 `npc_zi_ling_shan`。K8–K14 挂大长老、少年、抱还子、村民、金道友、梅凝三段、红土村壮汉。具体格位由关卡路定，**不压任何带**。
- 火鳞兽、狰狡兽、碧蟾兽、飞行阴兽只在文案（O-11），图上 0 个敌对对象、0 个遭遇区。

**`ch14_minzhou` 海边小城**（第五幕 n29 与通关后）
- 布局：海岸（spawn `haian`）→ 横贯带 → 小城街口 → 客栈（雅间）。
- 挂点：`trigger_anbian`（n29，enter，spawn 前横贯带）。设施：`facility_dazuo_kezhan`（meditate）。
- NPC：客栈雅间 `npc_zi_ling_minzhou`、`npc_mei_ning_minzhou`（`visible_flag ch14.done`，各一句）。**0 个 portal**：不开放旧图（拍板 29）。

### 4.2 命名、属性与美术

- `chapter=14`；`display_name_key = ch14.map.<短名>.name`（`linhuotan`、`shuangfeng`、`yaomu`、`yinming`、`minzhou`）；`region` 由协调者定（笔记无）：前四张照第 13 章外海图的值，`ch14_minzhou` 是天南的地，**推荐**照第 1–9 章「州名作 region」的先例取 `minzhou`（第 9 章 `genmaps_ch09.py` 有 `qingzhou` / `jianzhou` / `lanzhou`）。
- `bgm`：**只用已有曲子**（`docs/audio.md`），建议磷火潭 `bgm_cave`、双峰岛 `bgm_cave`、墓穴岛 `bgm_cliff`、阴冥之地 `bgm_wild`、闵州 `bgm_inn`；`tools/audiogen/catalog.py` 的 `MAP_BGM` 注释表与 `docs/audio.md` 各加 5 行。
- `tools/mapgen/genmaps_ch14.py`（5 张）接进 `genmaps.py --check`；`tools/artgen/artgen.py` 烘 5 张并 `--check` 过；**本章不 patch 任何旧图**（C1 是改一行脚本）。
- 打坐设施 4 处（石屋、客房、谷口、客栈），**都不写 `effectiveness`**（缺省 100，4.4 第 2 条）；存档点 5 处（4.4 第 1 条）。
- **同一格只取第一个 trigger**（引擎口径 4）：`trigger_shimen_baimu` 与 spawn、`trigger_yuta` 与 #6 落点、`trigger_shibei` 与 `trigger_shibei_yaowen`、`trigger_zhuchu_ting` 与梅凝、两条雷遁格与各自落点，全部错开。

### 4.3 地图里的改编（如实）

- 双峰岛到西岛的**地道**（拍板 18）：原著两人飞过去（557）；地道只为把 n10–n12 留在一张图上。
- 地裂与**雷遁捷径**（拍板 10）：原著没有这道裂谷；它是「风雷翅在世界里也是腿」的落点，只省路、不挡路（纪律 84）。
- 墓穴岛的「谷口岔路」与阴冥之地的「村西七里」都是距离的压缩；暴风山「千丈风」「冰坡」只在 n26 文案。

### 4.4 地图的整改拍板（与笔记不一致或笔记未写处，推荐做法）

1. **存档点**：笔记 G 表五张图都没列存档点；第 2–13 章每图都有（`ch13-design.md` 4.1 五处）。推荐每图一处 `facility_cunji_<短名>`（kind `save`，紧挨 spawn、在第一条横贯带南侧）；阴冥之地另在山脚 `trigger_shanjiao` 之前加一处 `facility_cunji_baofeng`（⑤ 致命、山路长）。id 由协调者认可后登 C4。
2. **打坐设施不写 `effectiveness`、设施不写 `deny_text_key`**：笔记 G 表写了「effectiveness 100」；第 10、13 章口径是不写（缺省即 100）。引擎拦设施只播 `ui.facility.locked`（第 13 章施工图 4.3 第 1 条核过），所以谷口那一处只写 `require_flag ch14.fatan`。
3. **两处显示名**：笔记 G 表中文名「磷火潭·风希洞府」「溪国闵州·海边小城」——地名横幅在落地时就出，而「风希」n02 才通名（M 表章内先后：「风希」只在 `fengmou` 之后）、「溪国」「闵州」只许在 n29。推荐横幅取「磷火潭」「海边小城」，「风希洞府」「溪国闵州」只进 n02 / n29 的文案。「阴冥之地」（580）比落地（578）早一个节点出横幅，判据是章级的、M 表没有它的章内专项，**照用**。
4. **雷遁格的位置**：五元触发器 `once false`，每踩必跳。推荐两格都放在崖边岔出的一格死胡同里（绕行路不经过），玩家要主动走上去才雷遁；落点见 4.1。
5. **巨猿傀儡只放 1 个**（规则 30：同图同 role 只认首尾相接的两段，十几个同时在场过不了），笔记 G.1 已写，照用。

---

## 5. 角色

**新建 40、就地改写 1（`feng_xi`）、照用 4（`fan_furen`、`yun_tianxiao`、`yuan_yao_jiedan`、`zi_ling`）＋ 主角**（笔记 H 表）。凡说话、上场的人都有 `data/roles/` 条目；境界只在本作枚举里取（上限 `CoreLate`），数值归平衡路，锚点见笔记 C.2；名牌就是 role 的 `name`，**一律是他此刻知道的叫法**（名牌跟 role 走，所以同一个人前后换名就是两个 role）。

### 5.1 剧情角色（地图上有对象的；**在场旗标的权威表**）

| id | 名牌 | 首见章 | 境界 | 新建 / 改写 | 在场（图 `对象`：出场 → 退场）| 其他 |
| --- | --- | --- | --- | --- | --- | --- |
| `feng_xi` | 风希 | 543（裂风兽 538）| CoreLate | **就地改写 note**（数值不动，B19）| linhuotan `npc_feng_xi_ting`：`fengmou → houqi`；`npc_feng_xi_qun`：`houqi → shiwu`；`npc_feng_xi_lianqi`：`shiwu → shijiao` | 不进任何编成；n11、n12 说话人；n02 挂 `biyanjiu.lua`（`yinjiu` 已置 → 一句「安心炼酒」）|
| `du_jiao` | 毒蛟 | 521 | CoreLate | 新建 | linhuotan `npc_du_jiao_ting`：`houqi → shiwu`；`npc_du_jiao_lianqi`：`shiwu → shijiao` | ① 第 0 波 |
| `gui_yao` | 龟妖 | 545 | CoreLate | 新建 | linhuotan `npc_gui_yao_ting`：`houqi → shiwu`；`npc_gui_yao_lianqi`：`shiwu → shijiao` | 自治之体，不进编成（A32）|
| `shuangfeng_shoumen` | 守门老者 | 554 | FoundationLate | 新建 | shuangfeng `npc_shoumen_laozhe`：`taoni → shefu` | |
| `yun_qin` | 韵琴 | 555 | FoundationMid | 新建 | shuangfeng `npc_yun_qin`：`rushi → shefu` | 一句（冷淡）|
| `fan_furen` | 范夫人 | 400 | FoundationLate | **照用**（一字不动）| shuangfeng `npc_fan_furen`：`rushi → shefu` | n09 挂 `fanfuren.lua`（B26）|
| `miaoyin_nvxiu` | 妙音门女修 | 560 | FoundationMid | 新建 | shuangfeng `npc_yaoyan_nvxiu`：`xidao → jieyao` | |
| `yuan_yao_jiedan` | 元瑶 | 375 | CoreEarly | 照用（第 12 章）| yaomu `npc_yuan_yao_andi`：`jieyao → fatan`；`npc_yuan_yao_fatan`：`fatan → guiwu` | 后者一句「施术中，不应」；② 友军；n12 说话人；n15 末起全作 0 处（O-10）|
| `juyuan_kuilei` | 巨猿傀儡 | 567 | CoreEarly | 新建（不说话）| yaomu `npc_juyuan_kuilei`：`fatan → guiwu` | 无脚本 |
| `yinming_shouhan` | 精瘦汉子 | 579 | Mortal | 新建 | yinming `npc_shouhan`：`heian → chucun` | |
| `yinming_dazhanglao` | 大长老 | 581–582 | FoundationEarly | 新建 | yinming `npc_dazhanglao`：`rucun → tuoli` | n18 挂 `dazhanglao.lua` |
| `yinming_shaonian` | 黝黑少年 | 583 | Mortal | 新建 | yinming `npc_heishao`：`zhanglao → chucun` | |
| `bao_huanzi` | 抱还子 | 584 | FoundationMid | 新建 | yinming `npc_baohuanzi`：`zhanglao → chucun` | 一句（n19 前判前置）|
| `yinming_cunmin` | 绿衫村民 | 579 | Mortal | 新建 | yinming `npc_cunmin`：`rucun → chucun` | |
| `baimian_zhanglao` | 白面长老 | 581–582 | Mortal | 新建 | yinming `npc_baimian_zhanglao`：`rucun → zhifu` | 冷眼一句；n21 前他还不知其名（B15）|
| `feng_tianji` | 封天极 | 586 | Mortal | 新建 | yinming `npc_feng_tianji`：`zhifu → tongling` | 折臂不语一句 |
| `mei_ning` | 梅凝 | 580 | FoundationMid | 新建（`weapons ["暗器"]`、`magics []`）| yinming `npc_mei_ning_zhuchu`：`rucun → chucun`；`npc_mei_ning_hongtu`：`chucun → hongtu`；`npc_mei_ning_shan`：`hongtu → tuoli`；minzhou `npc_mei_ning_minzhou`：`done →` | 首段挂 `tongling.lua`（n22）；⑤ 友军 |
| `yun_tianfu` | 云道友 | 589 | FoundationLate | 新建（红脸、天符门掌门，B14）| yinming `npc_yun_daoyou`：`tongling → chucun` | `jiaohun == 1` 时多一句（笔记 J.2）|
| `jin_sihai` | 金道友 | 589 | FoundationMid | 新建（驼背、四海真院）| yinming `npc_jin_daoyou`：`tongling → chucun` | |
| `hongtu_zhuanghan` | 红土村壮汉 | 592 | Mortal | 新建 | yinming `npc_hongtu_zhuanghan`：`chucun → tuoli` | |
| `zi_ling_jueling` | 紫灵 | 403 | CoreEarly | 新建（`weapons ["暗器"]`、`magics []`）| yinming `npc_zi_ling_shan`：`zhanwen → tuoli` | ⑤ 友军 |
| `zi_ling` | 紫灵 | 403 | CoreEarly | 照用（第 11 章起）| minzhou `npc_zi_ling_minzhou`：`done →` | 紫灵才结丹不久（A133）|

### 5.2 只说话的（无地图对象，节点脚本 `talk` 用）

| id | 名牌 | 首见章 | 境界 | 在哪一段说话 |
| --- | --- | --- | --- | --- |
| `yun_tianxiao` | 云天啸 | 536 | CoreEarly | n11（照用；五行环套住、一剑斩首是旁白）|
| `miao_he` | 妙鹤真人 | 525 | CoreLate（元婴，本作无此档，只说话）| n11、n12（新建，第 13 章接口约定）|
| `ke_yu` | 柯宇 | 562 | FoundationLate | n12 |
| `ding_hufa` | 丁姓护法 | 562（「丁某」）| CoreEarly | n12（「丁护法」原文 0 命中）|
| `kulou_xiushi` | 骷髅脸修士 | 561 | CoreMid | n12；② 敌 |
| `meishi_xiong` / `meishi_mei` / `baiyi_nanzi` | 梅姓男子 / 梅姓女子 / 白衣男子 | 568 | FoundationMid ×3 | n14 |
| `wen_tianren` | 温天仁 | 570 | CoreLate | n15、n27；③ 第 1 波 |
| `zi_ling_chezhong` | 车中少女 | 570（紫灵 403）| CoreEarly | n15（金焰中听出是紫灵之前）|
| `mei_ning_nvzi` | 女子 | 578 | FoundationMid | n16（580 通名前）|

### 5.3 只在战斗里的（编成用，无地图对象）

| id | 名牌 | 首见章 | 境界 | 编成 |
| --- | --- | --- | --- | --- |
| `du_jiao_xueguang` | 毒蛟 | 551 | CoreLate | ① 第 1 波（血光；`magics ["magic_dujiao_xueguang"]`）|
| `xiong_dahan` | 凶恶大汉 | 561–562 | CoreEarly | ② |
| `baiying_xiushi` | 白影 | 569 | CoreEarly | ③ 第 0 波（`magic_hushen_gang` 只作样子）|
| `liuji_zhenmo` | 真魔虚影 | 573 | CoreMid | ③ 第 1 波 ×2（`magic_moyan_huo`）|
| `wen_tianren_jinguang` | 温天仁 | 574 | CoreLate | ③ 第 2 波（同名第二段，第 12 章狼形器灵先例）|
| `tihun_shou_yin` | 啼魂兽 | 437 | CoreMid | ④ 友军（`magics ["magic_tihun_xihun"]`、`weapons ["拳"]`）|
| `juzhua_shou` / `yinming_jushou` | 巨爪兽 / 巨型阴兽 | 592 / 593 | CoreLate ×2 | ④ 第 0 / 1 波 |
| `tihun_yinyuan` | 银色巨猿 | 593 | CoreLate | ⑤ 友军（`magics ["magic_tihun_yinguang"]`、`weapons ["拳"]`）|
| `hanli_jueling` | 韩立 | — | CoreLate | ⑤ 友军（`magics []`、`weapons ["剑"]`、`actions 2`；契约 E2、O-2）|
| `feixing_yinshou` / `sanshou_yinshou` | 飞行阴兽 / 三首阴兽 | 587 / 595 | CoreEarly / CoreMid | ⑤ 敌 |

另：`du_jiao`（① 第 0 波）、`kulou_xiushi`（②）、`wen_tianren`（③ 第 1 波）、`yuan_yao_jiedan`（② 友军）、`mei_ning` / `zi_ling_jueling`（⑤ 友军）同时登在 5.1 / 5.2。破绽、架势、蓄势、`killable_by` 以 8.2 为准。

### 5.4 不建、外观

- **不建 role、只旁白**（笔记 H 表末）：易姓大汉、许云（n08）；云天啸之外的魔道守卫（n10）；火鳞兽、狰狡兽、碧蟾兽（不做仗）；妍丽（0 处）；狼形器灵（B4，不出场）；曲魂（第 12 章已离队，本章 0 处）。**判据③ 的人一律不建**：昭樵、步颠空、泰阳宗弟子、离火蛟。
- **外观**（`data/visual/looks.json`）：温天仁麻衣高冠赤足（A71）；风希青袍；毒蛟蛟首钢尾红鳞、龟妖龟壳青面（544）。**非人形 8 个**——毒蛟（两段共一张）、真魔虚影、巨爪兽、巨型阴兽、飞行阴兽、三首阴兽、银色巨猿、啼魂兽——进 `tools/artgen/sprites_enemies.py`，待机 ×2 ＋ 出招 ＋ 受击 ＋ 倒下（第 8、10–13 章同口径）；`sprite_preview.py --coverage` 缺图 0。`hanli_jueling` 外观照主角（无法力，不另画）。

### 5.5 撤场 / 出场与规则 30（`NpcPresenceTests`、`validate.py` 规则 30）

- **同图同 role**：`feng_xi`（三段）、`du_jiao`、`gui_yao`、`yuan_yao_jiedan`、`mei_ning`（阴冥三段）——每一对都是「前一段的 hidden 恰是后一段的 visible」，规则 30 放行的唯一一种互补；`juyuan_kuilei` 只一个对象（4.4 第 5 条）。
- **跨图**：`mei_ning` 阴冥第三段以 `tuoli` 撤、闵州以 `done` 出；`zi_ling_jueling` 与 `zi_ling` 是两个 role、不同图。
- **不同 role 同名牌**：同图可能同时在场的名牌两两不同（逐图核过：洞府大厅三人、炼器室三人；双峰岛四人；墓穴岛元瑶与傀儡；阴冥之地最多十一人同在——「梅凝」「紫灵」「白面长老」「封天极」「云道友」「金道友」各不同；闵州两人）。同名牌只出现在不同时段：「风希」三段、「毒蛟」（NPC 与 ① 两段）、「温天仁」（③ 两段）、「紫灵」（阴冥与闵州）。
- **链外在场旗标**：无——本章 NPC 的出场 / 退场旗标全在目标链上（笔记 J.1）；J.2 四面链外旗标都不作在场用。
- **章内名牌先后**（交 C3 专项，照笔记 M 表末）：「风希」只在 `fengmou` 之后；「梅凝」只在 n17 及以后（n16 为「女子」）；「封天极」只在 n21 及以后；「紫灵」只在 n15 末及以后（劝降段为「车中少女」）；「结丹第一人」只在 n27 及以后。

---

## 6. 支线（3 条：必经 1、可选 2）

| | Z1 `q14_xunxiong`「寻兄」| Z2 `q14_yaowen`「妖文碑」| Z3 `q14_guhe`「天符骨盒」|
| --- | --- | --- | --- |
| 必经 / 可选 | **必经** | 可选 | 可选 |
| 出处 | 587 他应允三个月内尽力寻兄（A106）→ 592 红土村仓库遗物（A114）| 585 仓库另一块碑专讲妖族文字，他通篇默记（novel.py read 585 第 27–31 段）| 589–591 云道友（天符门掌门）托付降灵符口诀（A109–A112）|
| 接取 | `tongling`（n22）| `shibei`（n20）| `tongling` |
| 步骤与挂点 | 无独立挂点，全在主线（n22 接、n24 成）| `trigger_shibei_yaowen`（interact、once、`set_flag ch14.z_yaowen`、guard `shibei`）→ `yaowen.lua`：`give("story_yaowen_zhujie", 1)`、末尾 `flag.set` | n23 `laike.lua` 夜里一段：抉择「接下 / 婉拒」→ 接下 `give("story_tianfu_guhe", 1)`、`z_guhe = 1`；婉拒 `z_guhe = 2` |
| 完成 | `hongtu`（n24）| `z_yaowen >= 1` | `z_guhe == 1` |
| 失败 / 过期 | — | `chucun` | `chucun`（且 `z_guhe != 1`）|
| 奖励 | 不进 rewards | 不进 rewards（脚本 `give`）| 同左 |
| 文件 / origin | `data/quests/q14_xunxiong.json`，原著 | `q14_yaowen.json`，原著 | `q14_guhe.json`，原著 |

- **完成压过失败**（`Quests.cpp` 先判 complete）：Z2、Z3 都在 `chucun`（n23 别大长老出村）过期。Z3 的完成旗标在 n23 同一份脚本里先于 `chucun` 落地，无后补可能。**Z2 有后补口子**：阴冥之地是一张图，出村之后村子仍走得进去（guard 惰性、不设隔断），`trigger_shibei_yaowen` 没烧过就还响——过期的任务会被补成「完成」。**推荐**（笔记未写）：`yaowen.lua` 开头判 `chucun` 已置 → 一句（已无暇细看）return、不置旗；`once` 触发器的记号由脚本末尾 `flag.set` 落地（`WorldScene.cpp` 第 56–64 行），提前 return 不会烧掉它，无副作用。
- **配额**（笔记 N.3、B29）：全作 21 / 必经 3 → **24 / 必经 4**；回写大纲第 5 节支线目标为 24 / 4（C7）。**不凑数**：秘市（他不等，A37）、幻梦石（范夫人的筹码）、夺权之议（他不掺和）、封天极（旁白）、元瑶的下落（判据③）都不做成支线。
- 「蛟魂玉瓶」「凝婴心得」「妖文注解」「骨盒」都是**以后的东西**：本章 0 处读、0 处结算（笔记 O.2「未用的线头」）。

---

## 7. 系统：意外破境、挟制、风雷翅双用、封法局、凡人的规矩、终章收束

**先查了已有的，再定做多少**（笔记 D 表，2026-10-13 grep）：`realm.advance` 顺带抬上限（第 4 章先例）、面板 `BreakthroughBlock::SeriesMax`、`magic.learn` / `magic.forget` 看返回值、`effect "stagger"` 与 `target "all"`（`validate.py` 只许 `target "all"` 用于伤人法术、不与 `effect` 同写）、编成的 `hero_absent` / `wave` / `charge` / `actions` / `killable_by` / `can_escape` / `defeat_is_fatal` / `drops`、`teleport(map_id, x, y)` 与五元 `kScriptTransfers`、NPC `visible_flag` / `hidden_flag`、路径行动 `when` / `until`、`ending()` 与末章章节卡 `Closing` ＋ `ToBeContinued`、标题「继续旅程」。**本章不加 Lua 命令、不加存档字段（仍 v9）、不改 `src/`**；契约三条都是验证项：**E1 结丹后期档**、**E2 封法局**、**E3 终章收束**（10.1）。商店、配方、灵田、遭遇、拍卖、小瓶本章全 0（笔记 D 表）。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **意外破境**（契约 E1）| n03 `houqi.lua`：`local ok = realm.advance(realm.CORE_LATE)`（全仓本章恰 1 处，false 一句照走），32 → 33、上限同抬；之后面板报 `SeriesMax`、修为条画满写「已至上限」——**这就是「后期顶峰」**（拍板 4）；结局卡文案写「结丹后期顶峰」| 不 `realm.cap` 让玩家坐满（与「一惊一乍中突破」相反，A10）；不另立「顶峰」字段；`realm.demote` 0 处；`breakthroughPillBonus` 仍恒 0；`aptitude` 不改 | 33 是本作上限；破境是风希逼出来的，不是他坐出来的——玩家手里没有按钮才对 |
| **挟制**（第一幕）| 洞府全图走得通（4.1），三妖按旗标在大厅 / 炼器室换段（5.1），K1–K3 是能听到的闲话；三处抉择只换一句：n02「饮 / 迟疑」（迟疑者风希再逼一次、终归饮下）、n03 试壁「剑芒 / 神雷 / 放虫」（虫一支他自己作罢，A8）、n04「装作初见 / 不答」（A13）；`trigger_shimen_baimu` 一句「白幕不开」| 不做逃跑分支、不做「被识破」、不做混沌邪气倒计时（风希的威胁，邪气始终未发，A16）| 压迫感靠「哪里都能走、哪里都有人看着」（笔记 N.2 第一幕要点）；他唯一的还手是 ① |
| **暗算三妖** | n05 配稀释灵液 `give("story_xishi_lingye", 1)`；n06 风希夺瓶 `take` 看返回值（false 一句照走，L.1）→ 三妖分饮（文案，A27）→ ① → `give("story_fenglei_chi", 1)`；蛟魂抉择「收入玉瓶 / 由它散去」→ `give("story_jiaohun_ping", 1)` 或一句，`ch14.jiaohun` 1 / 2（拍板 12）| 不做「下药成功率」；灵液描述照原著是「掺了有年份的灵药」（B5），不写掺虫 | 原著胜负在灵液与噬金虫两件事上（A26、A30）|
| **两门新剑法** | n03 `magic.learn("magic_jianying_fenguang")`（剑影分光，新建，木、`target "all"`、`boost "hits"`、`needRealm CoreLate`）与 `magic.learn("magic_bixie_shenlei")`（第 12 章所建，**全仓第一处 learn**），都看返回值（拍板 11、13）| 不改 `magic_fengyun_heji` 数值（第 13 章 ② 已按它平衡）；`talisman_bixie_shenlei` 不再给 | 「二十四柄」要落成能感到的东西：② 是它第一次全体出手；神雷是 ③ 灭虚影的钥匙，只走道具则库存 0 即死局 |
| **风雷翅双用**（拍板 10）| **战斗**：`magic_fenglei_chi` 就地改写（木、`effect "stagger"`、`stagger 2`、`power 0`、`needRealm CoreLate`；note「雷遁绕身削架势」），n07 `magic.learn` 看返回值；`feng_xi.magics` 照留。**世界**：n11 雷遁脱身（#7 同图带坐标）＋ 双峰岛地裂两条可选捷径（#8 / #9 五元，开头 `if not magic.knows("magic_fenglei_chi") then` 一句 `return end`，`teleport` 顶格，0 处 `flag.set`）；n28 最后一次雷遁冲出鬼雾（#5，文案）| 不做先手 / 位移的战斗新机制（要新引擎项）；捷径不当门（纪律 84，两岸本有绕行路）；`story_fenglei_chi` 不做 `castMagic` | 风雷翅是他从妖修手里抢来的命（导读第 2 句）：战斗里削架势、世界里省路、剧情里两次救命 |
| **封法局**（契约 E2）| ④⑤ `hero_absent true`：不建 `hanli`、不登记背包、战后主角 hp / mp 不变；④ 只有啼魂兽，⑤ 韩立以写死友军 `hanli_jueling`（名牌「韩立」、`magics []`、只有「剑」）与银猿、二女上场；阴冥段 0 处 `magic.learn`、`ch14_yinming` 0 个打坐设施（纪律 80）| 不加「封法」字段（要动加载器、`BattleState` 与菜单，只为两场，拍板 9）；不让主角在阴冥之地打仗 | 「阴冥之地里他是凡人」（导读第 4 句）：仗只能交给别人打，玩家亲手感到这一点 |
| **凡人的规矩**（第四幕）| 口粮、封门、阴风日、沉水、兽晶归长老、七八处村落——全落在 n17–n23 文案与 K8–K14 的情报里；檀云石符（n19）、石碑（n20、Z2）、通灵之气（n22，尺度见拍板 25）、兽皮衣（n22 `give`，n26 只读）| 不做口粮 / 任务的经济循环（这里没有钱，以鱼虾与任务换口粮）；不做狰狡兽冲村那一仗（他屋顶旁观，O-11）；封天极制服与了结都是旁白（拍板 24）| 一章 50 分钟，第四幕 12 分钟（笔记 N.2）——规矩是背景，不是玩法 |
| **猎兽攒晶** | n25 `trigger_lieshou`（interact、once、`set_flag ch14.shoujing`）放啼魂兽 → ④；**败：一句、不置旗、可再来**（`defeat_is_fatal false`，O-13）；胜：编成 `drops` 兽晶 1、蒙太奇 `give("story_yinming_shoujing", 99)`、啼魂兽毛转银白、能化巨猿（A116、A119）| 不做遭遇区刷晶；兽晶不做消耗（出去后难得，攒下即止）| 全章唯一一处「可输」：凡人局里输一次不该是 game over；once 记号由脚本置（`WorldScene.cpp` 第 56–64 行），败的那一支走不到 `flag.set`，触发器不烧 |
| **五行环收尾** | n15 战后 `magic.forget("magic_ji_wuxinghuan")` 与 `take("story_wuse_tonghuan", 1)` 各恰 1 处、都看返回值（纪律 85，A80）；金镜、银钟 `give`（A87）| 五行环不登规则 25（第 13 章口径：`power 0` 要不了命）| 一样东西＝一门法术，毁了就忘（第 8、10 章先例）|
| **终章收束**（契约 E3）| n29 `guixiang.lua`：`advance_days(105)` 南飞 → 雅间一席 → `ending("ui.ending.ch14.title", "ch14.ending.body")`（「乱星海篇·终」）→ 最后一条可执行语句 `flag.set("ch14.done")`；画面依次结局卡 →「第十四章　终」→「未完待续」；之后留在 `ch14_minzhou` 行走，紫灵、梅凝在场，客栈可打坐，0 portal（拍板 28、29）| 不回标题；不开放旧图；正文不写下一篇的任何名字（「落云宗」597、「结婴」597 等 0 处，M.2）| 全篇要有一张收束卡；通关档读进来要有地方站 |
| **队伍** | `party.*` 0 处；同伴只以写死友军进编成（② 元瑶、⑤ 紫灵与梅凝）| 不 `party.add` 元瑶 / 紫灵 / 梅凝 | `party.add` 要配对 remove，末章也不例外（拍板 30）|

### 7.1 门闸一览（**本章门闸的权威表**）

| 门闸 | 挂在哪 | 怎么拦 | 不到时 | 为什么不会卡死 |
| --- | --- | --- | --- | --- |
| 黑石门 | `trigger_shimen_baimu` | guard `fengmou`；永远一句 | 一句「白幕不开」| 只是被困的样子，不挡任何挂点 |
| 节点 NPC（n02、n09、n18、n22）| `biyanjiu.lua`、`fanfuren.lua`、`dazhanglao.lua`、`tongling.lua` | 脚本开头判前置未置 → 一句 return；done 已置 → 一句 return（笔记 F 表写法）| 一句、不置旗 | NPC 自出场旗标起就在图上，前置都是上一节点 |
| 谷口打坐 | `facility_dazuo_gukou` | `require_flag ch14.fatan`（不写 deny，4.4 第 2 条）| 引擎播 `ui.facility.locked` | `fatan` 是谷口的带，走到就置；③ 之前一定开着 |
| 雷遁捷径 | `trigger_leidun_dong` / `_xi`（#8 / #9）| guard `taoni`；脚本开头 `magic.knows("magic_fenglei_chi")` | 一句 return | 只省路；n07 已学，数据上几乎碰不到 |
| 回赠元瑶 | `chuansong.lua`（n12）| 三支只列有的：六级妖丹 ≥ 2 / 至元丹 ≥ 3 / 只道谢（永远在），`take` 看返回值（拍板 14）| — | 第三支零成本 |
| 旧物 `take` | n06 灵液、n15 铜环 | 看返回值 | false 只换一句照走 | 两样都由前文给、中途 0 处扣（笔记 C.3 要真档核铜环）|
| 旧物只读 | n04 铜片、n27 婆罗珠、n26 兽皮衣 | `item.count` | 0 时只换一句照走（**推荐**：笔记只写「读不扣」，缺物的写法同 `take` 口径）| 不因缺物 return |
| 法术 learn / forget | n03、n07、n15 | 看返回值 | false 只换一句照走 | 境界在 n03 已到 33（`needRealm CoreLate`）|
| 猎兽 | `trigger_lieshou`（n25）| ④ 胜才置 `shoujing` | 败一句、不置旗 | 可再来（7 节「猎兽攒晶」）|
| Z2 碑 | `yaowen.lua` | 开头判 `chucun`（6 节推荐）| 一句 return | 可选 |
| 通关后 | `ch14_minzhou` | 0 portal、`trigger_anbian` 已烧 | — | 有客栈、有两人、主菜单「返回首页」照常（E3）|

区域挂点（洞府各室、双峰岛地道、墓穴岛岔路、阴冥之地各区、暴风山山道）**不设门闸**：未到时 guard 未置、踩了不响（引擎口径 2：portal / facility 不读 visible / hidden，门闸只用 `require_flag` 或脚本开头判旗标）。

### 7.2 起点与章末对照（写测试时读表，不抄数；起点全部「估」，待 `ch13-end` 真档，O-3）

| | 起点（ch13-end，估）| ① 时（n06）| ②③ 时（n12–n15）| 阴冥（n16–n28）| 章末 |
| --- | --- | --- | --- | --- | --- |
| 境界 / 上限 / `formerRealm` | 32 / 32 / 22 | **33 / 33** / 22 | 同左 | 同左 | 33 / 33 / 22 |
| 气血 / 法力上限 | 900 / 620 | **1300 / 860** | 同左 | 同左（主角不上场）| 1300 / 860 |
| 面板冲关回报 | `StoryCap`（上限 32）| `SeriesMax`（「已至上限」）| 同左 | 同左 | 同左 |
| 最近的打坐处 | — | 洞府石屋 | 双峰岛客房 / 墓穴岛谷口 | 无 | 闵州客栈 |
| 法术 | 14 门（估）| 16（＋剑影分光、辟邪神雷）| 17（＋风雷翅）| 17（封着；④⑤ 主角不上场）| **16**（− 五行环）|
| 队伍 | 空 | 空 | 空 | 空 | 空 |
| 小瓶 | 容量 9、3 日一滴 | 同左 | 同左 | 同左 | 同左（本章 0 处 `bottle.*`）|
| 关键旧物 | 冰珠、铜片、五色铜环、婆罗珠各 1（估，必须在）| 同左；稀释灵液 0（n06 被夺）| ＋风雷翅、蛟魂玉瓶（0 或 1）、龟壳、血鳞、万年灵液、两玉简 | 铜环 0；＋金镜、银钟、玉尺 | ＋凝婴心得、兽皮衣、兽晶 100、四象蟠龙带、碧绿内甲；妖文注解 / 骨盒各 0 或 1 |
| 日历（D ＝ 起点 `day`）| D | D ＋ 391 | ② D ＋ 504；③ D ＋ 507 | D ＋ 507 → D ＋ 575 | **D ＋ 680**（玩家打坐另计）|

日历列只是把笔记 N.2 的日数按节点累加（① 在 n06 两笔之后：180 ＋ 2 ＋ 180 ＋ 29；② 在 n12 第一笔之后；③ 在 n12 第二笔之后；n29 的 105 在最后），**日数的权威在 3.3**，本表不另立数。

---

## 8. 战斗编成与平衡

### 8.0 「剧情战斗」怎么数

沿用第 5 章 8.0 的口径：**必打** ＝ 挂在目标链必经挂点上、完成旗标是下一节点的 guard。本章 **5 场必打**，全是原著的仗（拍板 6）：① n06 `b14_fengxi_xiezhi`（**就地改写**占位，B18、拍板 15）、② n12 `b14_qingyang_zhifashi`、③ n15 `b14_hufa_wentianren`、④ n25 `b14_yinming_tihun`（可输可重来，**按一场计**）、⑤ n28 `b14_baofeng_feiyao`。**可选：无**（0 遭遇区、0 条 `challenge` 路径行动）。全作 47 → **52**，恰合方案 P3 的 52 场。

**这五场就是「他护的是别人的法，打的是别人的仗」的说明书**（导读第 3、4 句）：

| # | 谁上场 | 这一仗教玩家什么 |
| --- | --- | --- |
| ① | 韩立一人 | **第一次在结丹后期打仗**：剑破得了鳞甲、破不了命，只有噬金虫要得了毒蛟的命 |
| ② | 韩立 ＋ 元瑶（写死友军）| **剑影分光第一次全体出手**：二十四道青芒化九十六道，两个人一刀扫 |
| ③ | 韩立一人（傀儡守谷口，不进编成）| **全作第一次与同阶名人一对一**：三波各有各的钥匙，最后一波是扛 |
| ④ | 啼魂兽一只（`hero_absent`）| **他是凡人**：他什么也做不了，只有啼魂吸魂要得了阴兽的命 |
| ⑤ | 韩立（无法力）、银猿、紫灵、梅凝（`hero_absent`，四个写死友军）| **全作唯一一场四人并肩**：银光扫场、兽筋小剑补刀、二女小弩 |

### 8.1 意图玩家

- **共同**：结丹后期（1300 / 860，攻 102、防 82——`Realm.cpp` 第 219、236、259、277 行，攻防「暂定」，本章 ①②③ 标定，10.1 平衡项 B1）；结丹中期锚点全作 0 场用到（B22、O-5）。兵刃：剑（`weapon_qingzhu_fengyun_jian`，照第 13 章 8.1）、拳、暗器（`weapon_wuming_sixian`）。道具：中阶灵石约 20（估，回法力）。**队伍为空**（拍板 30）。法术与旧物以 7.2 为准。
- **①**：法术 16 门（第 13 章章末 14 门 ＋ n03 学的剑影分光、辟邪神雷）；噬金虫群（金）是收尾的钥匙。进场前 n05 石屋可打坐、n06 脚本拨日 209 天（每日回一成气血，满血进场）。
- **②**：法术 17 门（＋ n07 学的风雷翅）；元瑶在旁（`yuan_yao_jiedan`，结丹初期）。双峰岛客房可打坐；② 在 `chuansong.lua` 中段，进脚本前的血量就是进场血量。
- **③**：法术 17 门（五行环此时还在，战后才忘）；谷口打坐（n13 后开）。三波一口气打完，中间不回血。
- **④**（`hero_absent`）：啼魂兽一只（结丹中期，拳 ＋ 啼魂吸魂〔水〕）；主角 hp / mp 不动、不登记背包（契约 E2）。
- **⑤**（`hero_absent`）：`hanli_jueling`（结丹后期，剑、`actions 2`、无法术）、银色巨猿（结丹后期，拳 ＋ 银猿银光〔水，全体〕）、紫灵（结丹初期，暗器）、梅凝（筑基中期，暗器）。
- **修为**（`rewards.cultivation`）：①②③ 各 > 0（数归平衡路；结丹后期修为条已满，只作记账）；④⑤ 照发（第 5 章口径，笔记 I 表）。
- **最低合法起点**（笔记 N.1 第 5 条）：妖丹 0、至元丹 0、灵石 0、气血不满。①③ 之前各有打坐处，① 还有脚本拨日；④ 可输可重来；⑤ 友军按编成满血进场——**没有「存在仗前、打不过、也回不了血」的死局**。平衡路两种起点都要量（10.1 平衡项 B1、B2）。

### 8.2 五张必打编成（**编成的权威表**）

| # | 编成 id | 敌方（按波次；破绽 / `killable_by`）| 我方 | 逃 | 败 | 意图 |
| --- | --- | --- | --- | --- | --- | --- |
| ① | `b14_fengxi_xiezhi`（**就地改写**：`name`「炼器室·噬蛟」、`backdrop secret_room`）| **第 0 波** 毒蛟 ×1（`du_jiao`，结丹后期；`["金","剑"]` / `["金"]`）；**第 1 波** 毒蛟 ×1（`du_jiao_xueguang`，同名第二段；`["金","剑"]` / `["金"]`；`magics ["magic_dujiao_xueguang"]`——水、带毒、全体；蓄势血光毒 `charge {every 3, mult <平衡路定>, all true, text_key "ch14.battle.fengxi_xiezhi.charge"}`）| 韩立一人 | 否 | **致命** | **剑削势、虫收命**：剑与五行环削得动鳞甲的架势，收尾只有噬金虫（金）；第二段的血光是全体带毒，拖久了磨的是气血。神雷对妖无克（552），木不是它的破绽——刚学的两门剑法在这里不是钥匙 |
| ② | `b14_qingyang_zhifashi`（新）| **第 0 波** 骷髅脸修士 ×1（`kulou_xiushi`，结丹中期；`["木","剑"]` / `["木","剑"]`）＋ 凶恶大汉 ×1（`xiong_dahan`，结丹初期；`["木","剑","金"]` / `["木","剑"]`）| 韩立 ＋ 元瑶（写死友军，`faction ally`）| 否 | **致命** | **一刀扫两人**：剑影分光（木、全体、多段）打两人的破绽，一回合削两份架势；元瑶在一旁挡刀。原著两人收宝散逃、被分两股追杀（A57）——这是全体法术的说明书 |
| ③ | `b14_hufa_wentianren`（新）| **第 0 波** 白影 ×1（`baiying_xiushi`，结丹初期；`["金","木"]` / `["金"]`）；**第 1 波** 温天仁 ×1（`wen_tianren`，结丹后期；`["木","剑"]` / `["木","剑"]`；`actions 2`；`magics ["magic_wen_jinzhen"]`——金丝针）＋ 真魔虚影 ×2（`liuji_zhenmo`，结丹中期；`["木"]` / `["木"]`；`magic_moyan_huo`）；**第 2 波** 温天仁 ×1（`wen_tianren_jinguang`，同名第二段；`["木","剑"]` / `["木","剑"]`；`actions 2`；`magics ["magic_bamen_jinguang"]`——金、全体；蓄势金光柱 `charge {every 3, mult <平衡路定>, all true, text_key "ch14.battle.hufa_wentianren.charge"}`）| 韩立一人 | 否 | **致命** | **三波三把钥匙**：0 波噬金虫凝矛（金）收白影；1 波**神雷灭虚影**（木）、剑压本体——金丝针克虫，**金不是他的破绽**（A73）；2 波金光柱每三回合打全体，**在蓄势间隙把他的气血打到零**（8.3 ③、O-8）。全章最重的一仗 |
| ④ | `b14_yinming_tihun`（新）| **第 0 波** 巨爪兽 ×1（`juzhua_shou`，结丹后期；`["水","拳"]` / `["水"]`）；**第 1 波** 巨型阴兽 ×1（`yinming_jushou`，结丹后期；`["水","拳"]` / `["水"]`；蓄势冰寒阴风 `charge {every 3, mult <平衡路定>, all true, text_key "ch14.battle.yinming_tihun.charge"}`）| **啼魂兽一只**（`tihun_shou_yin`，写死友军；`hero_absent true`）| **可** | **不致命**（输了一句、可再来）| **他看着**：拳削得动势、要不了命，只有啼魂吸魂（水）要得了；阴风是全体，只有一个友军的局里它就是单体重击——要在攒风那一回合前打破它的势 |
| ⑤ | `b14_baofeng_feiyao`（新）| **第 0 波** 飞行阴兽 ×3（`feixing_yinshou`，结丹初期；`["剑","暗器","水"]` / 同）；**第 1 波** 飞行阴兽 ×2 ＋ 三首阴兽 ×1（`sanshou_yinshou`，结丹中期，首领；`["水","剑"]` / `["水"]`；蓄势俯冲 `charge {every 3, mult <平衡路定>, all true, text_key "ch14.battle.baofeng_feiyao.charge"}`）| **四名写死友军**：`hanli_jueling`、`tihun_yinyuan`、`zi_ling_jueling`、`mei_ning`（`hero_absent true`）| 否 | **致命** | **多、薄、快**：银猿银光（水、全体）扫场，兽筋小剑（剑）与二女小弩（暗器）补刀；首领只有银猿要得了命——别让它攒起俯冲 |

- **必须守住的**（数值归平衡路，按 `tests/BattleHand.h` 那只手量；本表只写意图与破绽）：五张 `rewards.spirit_stones 0`、`can_escape` 只有 ④ 为 true、`defeat_is_fatal` 只有 ④ 为 false；`drops` 只有 ④ 恰 `story_yinming_shoujing` ×1（`rate 100`），其余 `[]`；①②③ `hero_absent false`，④⑤ `hero_absent true` 且第 0 波有友军；④ 友军恰一名 `tihun_shou_yin`、⑤ 恰四名；② 友军恰一名 `yuan_yao_jiedan`；①③ 无友军。`killable_by` 一律非空且 ⊆ `weaknesses`；友军不吃 `toughness / weaknesses / killable_by / charge`；**胜负只看气血归零**；`charge` 四键必写、`mult` 缺了或 ≤ 0 加载器整份拒（第 13 章 8.2 核过）；友军 `wave` 缺省（＝ 0）。
- **① 占位就地改写的几处**（B18）：敌方 `feng_xi` 换成 `du_jiao` 两段；`rewards.spirit_stones 600 → 0`、`cultivation 2200` 改平衡路定值；`name` 与 `note` 改写成炼器室噬蛟（550–551）；id 不变。`feng_xi` 自此不在任何编成里（O-1，与 selftest 探针挪位同一批落）。
- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，C2；**本段是规则 25 表的唯一权威**，10.1 C2 与契约只引用）：`CHAPTER_MEANS[14]` ＝ 第 13 章那 8 类原样——拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07，另 `("金","learn","magic_shijin_chongqun","ch12")`）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`weapon_wuming_sixian`，ch07）、剑（`weapon_lieyang_jian`，ch06）——木外加 `("木","learn","magic_jianying_fenguang","ch14")`、`("木","learn","magic_bixie_shenlei","ch14")`（n03，① 之前）；**五行环不登**（n15 忘掉，且 `power 0` 要不了命）。`BATTLE_EXTRA_MEANS["b14_yinming_tihun"] = [("水","role_magic","magic_tihun_xihun","tihun_shou_yin")]`、`["b14_baofeng_feiyao"] = [("水","role_magic","magic_tihun_yinguang","tihun_yinyuan")]`；`MEANS_LAST_CHAPTER = 14`。`hero_absent` 的场不继承通表、只认写死友军：友军兵刃自动计入（`validate.py` `battle_means`，2026-10-13 第 2573–2582 行），所以 ④ ＝ 拳 ＋ 水、⑤ ＝ 剑 ＋ 暗器 ＋ 拳 ＋ 水，不必另登兵刃。
- **selftest 探针**（笔记 O.2 第 3 行，C2）：末章之后无处可挪，改在测试副本里临时写一张合成编成 `data/battles/b15_fanwei_tanzhen.json`（`chapter 15`，敌方复用 `sanshou_yinshou`、`weaknesses` 改 `["毒"]`）：`chapter 15` 不报、改 14 必报；`finally` 删文件、还原 role 字节、再验目录清单；「末章 12 / 13」用例名与断言改 14。
- **战斗背景**（`data/visual/battles.json`，全是现有名）：① `secret_room`、③ `cliff_top_dusk`、④ `mountain_forest_dusk`、⑤ `cliff_top_dusk`（笔记 I、G 表）；② 笔记未写，**推荐** `cave_tunnel`（`ch14_shuangfeng` 的背景，② 由这张图上的挂点起）。每条都要指得出读它的那一行代码、有走真实入口的测试。
- **开场白与文案**（`data/text/ch14_battle.json`，key `ch14.battle.<短名>.intro / .charge / .win`，短名去掉 `b14_`）：① 开场白写风希一爪、皇鳞甲破了大半（A29），胜利写噬金虫噬杀毒蛟；② 开场白写头颅甩出、两人收宝要逃；③ 开场白写八面金镜是天镜散人本命法宝的复制品（A79），蓄势写金光神焰困身、狼首玉如意的护罩转银（A81–A82，器灵不出场），**胜利只许写到他精血与金角耗尽、坐在金焰上骑虎难下**（O-8，不写退走、不写负伤倒地——n27 才死，纪律 81）；④ 蓄势写冰寒阴风；⑤ 蓄势写俯冲。**旧占位 `ch14.battle.fengxi_xiezhi.intro` 在 `data/text/battles.json` 第 9 行**，由协调者删（重复 key 让 `loadGameData` 起不来，B20）。原著台词一句不抄。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 噬蛟 | 550–551：灵液里的药发作，风希一爪抓破皇鳞甲大半；十余万三色噬金虫以命换血光，噬杀毒蛟、收其蛟魂；龟妖是自治之体，虫奈何不得（A29–A32）| 两波：毒蛟本体 → 血光；龟妖与风希都不进编成；蛟魂是战后抉择 | 原著他从没与风希正面打过（B18），唯一一仗就是炼器室噬蛟；**只有「金」要得了它的命**原样落在 `killable_by` 上。龟妖不列（A32），二妖被踢进地火是 n07 演出 |
| ② 执法使 | 563–564：「借头颅」斩丁护法、柯宇与带路弟子透心；两名执法使收宝散逃，二十四道青芒化九十六道、分两股追杀（A56–A57）| 一波两敌；元瑶写死友军；借头颅与柯宇之死是 n12 旁白 | 第 12 章 ② 他与元瑶并肩是改编；这一次原著就是两人各在一边、他出剑追杀——元瑶在编成里只是「在场」，钥匙是他的剑影分光 |
| ③ 温天仁 | 569–575：白影闯谷被三色巨矛所制；温天仁劝降不成；金针克虫、折损近万；阴魔斩破化劫大法的替身，金色电弧一击灭两个真魔虚影；八门金光镜八面、金光柱冲垮剑光、五行环寸断；金光神焰困身，他以灵液对耗；鬼雾到（A68–A85）| 三波：白影 → 温天仁与两个真魔虚影 → 温天仁·金光镜；**胜 ＝ 第 2 波温天仁气血归零 ＝ 他精血与金角耗尽、骑虎难下**，紧接 n15 鬼雾演出 | **原著这一仗没有胜负**（O-8）。引擎只认气血归零，所以「赢」落在他那一边的力竭上，不落在倒地上；笔记 I 表「扛住即胜」按引擎口径读作「在金光柱蓄势的间隙把他打到零」，不是撑满回合（与笔记措辞不同处，整改时统一）。斩杀只在 n27、一剑、旁白（拍板 7）；阴魔斩、化劫大法、金光神焰只在蓄势文案 |
| ④ 啼魂猎兽 | 592–593：为攒兽晶，他放啼魂兽猎强阴兽，吸魂、颅中得兽晶；兽晶攒到百余块，啼魂兽化十丈银色巨猿（A115–A119）| `hero_absent`：啼魂兽一只对两波；败可再来；胜后蒙太奇 `give` 99 | 他法力被封，原著就是啼魂兽在打；百余块兽晶不是一仗打出来的——**一仗 ＋ 蒙太奇**（第 13 章 ② 诱妖 ＋ n25 收手同口径）；「可输」是游戏的让步（O-13）|
| ⑤ 山巅飞兽 | 595：山巅巨岩上千只飞行阴兽、三首阴兽领头；十口小剑以兽筋连指如绳镖，给二女红色小弩与兽皮软盾（A124–A126）| `hero_absent`：四名写死友军对两波；兽皮软盾只在文案 | 「上千只」压成两波六只（笔记 I 表）；他无法力，所以是 `hanli_jueling` 而不是主角——名牌同名的风险在契约 E2、O-2 |
| 不打：风希 | 542–553：被擒在先、饮酒在后；追逐一月余 | n01–n07 演出 | 九级≈元婴中期以上，本作无此档（B19）；他只逃 |
| 不打：守阵魔修、云天啸、妙鹤 | 557–559：守阵者一招了事；五行环套住云天啸斩首；妙鹤两击、他雷遁脱身 | n10、n11 旁白 | 一招之事（笔记 F 表 n10、n11）；妙鹤是元婴，他只有逃 |
| 不打：丁护法与柯宇 | 563：借头颅 | n12 旁白 | 一剑之事 |
| 不打：梅氏兄妹与白衣男子 | 568：一见傀儡就退 | n14 抉择，同归于退 | 拍板 8 |
| 不打：火鳞兽、狰狡兽、碧蟾兽 | 579、581–582、591–592 | n16、n18、n24 旁白 | 他看着、或一剑了事；5 场已用满（O-11）|
| 不打：封天极 | 586–587：小剑抵喉、折臂；后来噬金虫了结 | n21、n22 旁白，画面 0 处血腥 | 拍板 24、O-9 |
| 不打：温天仁之死 | 594：兽筋牵剑，小剑自颈后洞穿 | n27 旁白 | 「温天仁只死一次」（纪律 81）|

### 8.4 野外遭遇

**本章不放遭遇区**（笔记 D 表）：洞府里他是囚徒、双峰岛是地下坊市、墓穴岛是护法之地，都没有可打的东西；阴冥之地的阴兽原著要么是他旁观（火鳞兽、狰狡兽）、要么是一剑了事（碧蟾兽）；而遭遇表的掉落只许灵草——做遭遇区只会让一个法力被封的人白白看几场打了没兽晶的仗。攒晶的循环就是 ④。修为本章没有用处（33 是上限，7.2）。

---

## 9. 经济账（**数目的权威表**）

**四本账：低阶 / 中阶灵石（`material_lingshi` / `material_lingshi_zhong`）、妖丹（`material_yaodan_wuji` / `material_yaodan_liuji`）、至元丹（`pill_zhiyuan_dan`）、阴冥兽晶（`story_yinming_shoujing`）**，另记 story 物品。起点全部「估」（`tests/fixtures/ch13-end-*.sav` 尚未生成，笔记 C.3）：低阶约 2000–6000、中阶约 20、五级 / 六级妖丹约 13 / 7、至元丹 0–3。下面只写本章的进出与恒等式，不先宣称终局数。

**一句话：钱不进不出**（笔记 N.1 第 1 条）——五场 `spirit_stones 0`、0 张商店、0 张配方、0 处灵石 `give` / `take`；本章唯一一处付出是 n12 回赠，且有零成本一支。

**来源**

| 项 | 低阶 / 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- |
| 起点（ch13-end，**估**）| 约 2000–6000 / 约 20 | 约 13 / 7 | 至元丹 0–3；旧物见 7.2（**必须拿真档核**：铜环、婆罗珠、妖丹、至元丹）|
| n05 石屋 | | | 稀释灵液 1 |
| n06 炼器室 | | | 风雷翅 1；蛟魂玉瓶 1（抉择收瓶时）|
| n07 两妖储物袋 | | ＋6 / ＋3 | 乌黑龟壳 1、血红鳞片 1（原著「众多妖丹」，按比例收）|
| n12 皇明岛 | | | 万年灵液（半瓶）1、青阳门阵法玉简 1、啼魂兽祭炼玉简 1 |
| n15 护法之后 | | | 紫色玉尺 1、八面金镜 1、银色小钟 1 |
| n20 / Z2 | | | 凝婴心得 1 / 妖文注解 1 |
| n22 / Z3 | | | 火属性兽皮衣 1 / 天符门骨盒 1 |
| ④ 编成 `drops` ＋ n25 蒙太奇 | | | 阴冥兽晶 1 ＋ 99 |
| n27 | | | 四象蟠龙带 1、碧绿内甲 1 |

**去向**

| 项 | 低阶 / 中阶灵石 | 五级 / 六级妖丹 | 其他 |
| --- | --- | --- | --- |
| n06 风希夺瓶 | | | 稀释灵液 −1（看返回值）|
| n12 回赠（三支只列有的）| | 第一支 — / −2 | 第二支至元丹 −3；第三支只道谢，0 |
| n15 五行环寸断 | | | 五色铜环 −1（看返回值）；法术 − 五行环 |
| 战斗中的道具（玩家）| 中阶灵石回法力，按真实用量 | | |

**账要对得上的几条**（交测试路 `Ch14LedgerTests`，T3；直接读 `scripts/ch14/*.lua` 与 `data/battles/b14_*.json`）：

1. **灵石**：`give("material_lingshi"` / `"material_lingshi_zhong"` 0 处；`take` 0 处；五场 `rewards.spirit_stones` 都是 0。**没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训，本章根本不收钱）。
2. **妖丹**：`give` 恰 2 处、都在 `taoni.lua`（五级 6、六级 3）；`take` 恰 1 处——`chuansong.lua` 六级 2，且只在「六级 ≥ 2」时才列出那一支；编成 0 处掉妖丹。妖丹本章无处可卖，是留给以后的身家（笔记 N.1 第 3 条）。
3. **至元丹**：`take` 恰 1 处（`chuansong.lua` 3，只在「≥ 3」时才列）；`give` 0 处、配方 0 张。
4. **story**：新建 18 样各 `give` 1 处（L.1；兽晶除外）；**兽晶** `give` 恰 1 处（n25，99）＋ ④ `drops` 1 ＝ **100**（A116「百余块」）；`take` 恰 2 处（稀释灵液 n06、五色铜环 n15），都看返回值；`story_banyao_cao`、`story_yanghun_mugen`、`story_yanghun_zhu`、`story_wannian_lingru` 0 处读写（B21、B23）；乾蓝冰珠、铜片、玉如意、花篮、青冥针、皇鳞甲、血色披风、啼魂兽、鸣魂珠、婆罗珠、虚天鼎 0 处 `take`。
5. **法术**：`magic.learn` 恰 3 处（`houqi.lua` 剑影分光、辟邪神雷；`taoni.lua` 风雷翅）、`magic.forget` 恰 1 处（`hufa.lua` 五行环），都看返回值；阴冥段（n16–n28）0 处 `magic.learn`。
6. **境界**：`realm.advance` 恰 1 处（`houqi.lua`）；`realm.cap` / `realm.demote` 0 处；`bottle.*` 0 处；`field.*` 0 处；`shop(` 0 处；`party.*` 0 处。
7. **编成**：五张 `spirit_stones 0`；`drops` 只有 ④ 恰兽晶 1（`rate 100`）。全仓 `guaranteed:true` 仍恰 1 张（本章 0 配方）。
8. **意图玩家章末账**（两侧同）：低阶 ＝ 起点、中阶 ＝ 起点 − 战斗用量；五级 ＝ 起点 ＋ 6；六级 ＝ 起点 ＋ 3 − 2（n12 第一支）＝ 起点 ＋ 1；至元丹 ＝ 起点；兽晶 100；本章新 story 各 1（蛟魂玉瓶、妖文注解、骨盒按两侧所选）；稀释灵液 0、五色铜环 0。
9. **两侧同账**：通关两侧走同一组选项。**推荐原著那一支**（笔记未定，交 T3 拍）：n06 收瓶（A31）、n12 回赠第一支（原著回赠以六级妖丹为主料的丹药，A59）、n22 只应寻兄三月（原著，笔记 F 表 n22）、Z2 做（585 他通篇默记了那块妖文碑，novel.py read 585 第 31 段）、Z3 接下（A111）；其余只换一句的抉择任选、两侧一致。章末 `day` − 起点 `day` ＝ **680 ＋ 玩家打坐日数**（3.3）；两侧低阶灵石差 ＝ 起点差。
10. **退路**：本章门闸全在 7.1；没有一处会因物品或钱不够而卡死（笔记 N.1 第 5 条）。

**对照：原著这一段的量级**。这一章他几乎没碰过钱：被擒、被抽灵力、逃命、借阵、护法、落进一个没有灵气也没有灵石的地方——到手的是「命里的东西」：一对风雷翅、半瓶万年灵液、一块碑上的凝婴体验、百余块出去之后难得的兽晶、一条四象蟠龙带。所以本章的账不在灵石，在**留给以后**：妖丹、蛟魂、骨盒、妖文注解一样都不结算（笔记 O.2「未用的线头」），全作在溪国闵州停下。

