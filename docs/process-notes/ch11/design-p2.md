<!-- 第 11 章施工图·第二部分（第 4–9 节）。事实来源只有 notes/ch11/research.md；id、旗标、章号、数字照抄，表号（A/C/E/F/G/H/I/K/L/N/O）指该笔记。节号与节名照第 10 章施工图。 -->

## 4. 地图（新建 7 张，旧图 0 处修补）

主题 / 时辰 / 背景只用 `data/visual/maps.json` 已有组合（town/day/town_street、indoor_stone/indoor/secret_room、dock/day/dock_river、cliff/day/cliff_top、wild/day/cliff_top、indoor_wood/indoor/inn_hall）；`region` 一律 `luanxinghai_zhongbu`（新字符串，只是标签）；`chapter=11`；`display_name_key = ch11.map.<短名>.name`。出入口照规则 29「出门往哪走，进门就朝哪」，spawn 名在括号里。尺寸量级参照第 10 章（魁星岛 60×48、小寰岛 36×28）。

| 地图 id | 名称 | 主题 / 时辰 / 背景 | 尺寸 · 分区 | 出入口 | 挂点与设施 | 在场 NPC（出场 → 退场旗标） |
| --- | --- | --- | --- | --- | --- | --- |
| `ch11_fangshi` | 天星城坊市（圣山第一层） | town / day / town_street；粒子 dust | 56×44，三区：南缘城门、中部坊市街、北缘山脚楼阁；东北角茶馆雅间（室内 zone，地面 `wood_floor`） | **北**口 `portal_shangshan` → `ch11_dongfu`（spawn `men`，朝上）；青竹小轩门（街西，朝上进店）→ `ch11_xiaoxuan`（spawn `dian`）`require_flag ch11.kaidian`、`hidden ch11.liugen`；茶馆门（朝右进）`require_flag ch11.lianer`；城门南缘不可出（`can_leave_edge false`） | `trigger_chudao`（城门 spawn `chengmen`，enter）；`facility_zahuo`（shop `ch11_fangshi_zahuo`，`require_flag ch11.nichang`）；`facility_xuanshang`（board）；`facility_cunji_fangshi`（save）；茶馆两场都挂在 npc 上（n20、n25） | `npc_baiyi_xiushi`（ch11.chudao → ch11.zhihuan）；`npc_lu_er`（ch11.zhihuan → ch11.anju）；`npc_xinggong_zhishi`（山脚楼阁，ch11.luer → ch11.zufu）；`npc_baimen_zhanggui`（ch11.kuilei → ch11.done）；`npc_fengle_menmian`（ch11.kuilei → ch11.done）；`npc_chuwu_zhanggui`（ch11.kuilei → ch11.huicheng）；`npc_lian_er`（ch11.zhaolan → ch11.lianer）；`npc_fan_furen`、`npc_zhao_zhanglao`（雅间，ch11.lianer → ch11.chaguan）；`npc_zi_ling`（role `zi_ling_zhenrong`）、`npc_fan_furen_2`、`npc_zhuo_ruting`（雅间，ch11.yaoshi → ch11.zhanglao）；`npc_li_dianzhu`（ch11.jinlei → ch11.kaidian）；`npc_he_xianshi`（街，ch11.kaidian → ch11.liugen） |
| `ch11_dongfu` | 圣山第三十九层洞府 | indoor_stone / indoor / secret_room | 34×26：门厅、密室、丹房、虫室、药园（`tilled` 6 槽）、药园一角珊瑚礁 | **南**口 `portal_xiashan` → `ch11_fangshi`（spawn `shanjiao`，朝下）；**北**口 `portal_xingkong` → `ch11_xingkong`（spawn `dian`，朝上）`require_flag ch11.nichang`、`deny_text_key ch11.dongfu.xingkong_deny` | spawn `men`；`trigger_anju`（门内一格，enter）；`facility_jingshi_39`（meditate，`effectiveness 125`，`require_flag ch11.anju`）；`facility_danfang_39`（alchemy，grade 3，`require_flag ch11.anju`）；`facility_lianqi_39`（forge，grade 3，`require_flag ch11.liugen`）；`facility_yaoyuan_39`（field，ref `field_tianxing_39`，`require_flag ch11.anju`）；`trigger_kuilei`、`trigger_liandan`（丹房）、`trigger_biguan`（密室）、`trigger_chuguan`（密室门外）、`trigger_zhaolan`（门禁制）、`trigger_zhuzhan`（门）、`trigger_nichang`（药园旁）、`trigger_peizhu`（珊瑚礁旁，n26/n29 同一触发器）、`trigger_lianjian`（炼器台同格）、`trigger_chujian`（密室门外）；`facility_cunji_39`（save） | `npc_jin_qing`（门外，ch11.lianjian → ch11.done）；`npc_jiedan_linju_a` / `_b`（门外小径，ch11.zhaolan → ch11.done） |
| `ch11_xingkong` | 星空殿（第五十层） | indoor_stone / indoor / secret_room | 24×18：大理石殿、成排传送阵 | **南**口 `portal_xiashan_50` → `ch11_dongfu`（spawn `beimen`，朝下） | spawn `dian`；`npc_xingkong_xiushi` 挂 n09 脚本（传送阵就是他，`once` 否）；`facility_cunji_50`（save） | `npc_xingkong_xiushi`（ch11.nichang → ch11.done）；`npc_yaoyan_furen`、`npc_tongxing_xiushi` ×2、`npc_zaohong_laozhe`（ch11.nichang → ch11.chuanshong） |
| `ch11_ningcui_jie` | 凝翠岛小街 | dock / day / dock_river；粒子 mist | 30×20：西端传送石屋、一条街四间铺、东端海岸 | **东**口 `portal_chuhai` → `ch11_honghu`（spawn `xi`，朝右）`require_flag ch11.ningcui`；石屋传送阵 `trigger_huicheng_chuansong`（interact，`once` 否）→ `teleport("ch11_xingkong", "dian")` | spawn `shiwu`；`trigger_ningcui`（石屋门外，enter）；`facility_zahuo_nc`（shop `ch11_ningcui_zahuo`）；`facility_shougou`（shop `ch11_ningcui_shougou`）；`facility_cunji_nc`（save）；Z1 挂在 `npc_fan_xiaozi` 的 interact 脚本 | `npc_ningcui_shouzhen`（石屋，ch11.chuanshong → ch11.done）；`npc_zahuo_nc`、`npc_fan_xiaozi`（ch11.chuanshong → ch11.huicheng）；`npc_hao_yuantian`（ch11.ningcui → ch11.huicheng） |
| `ch11_honghu` | 红瑚岛 | cliff / day / cliff_top；粒子 mist | 36×30：环形珊瑚礁、三处必经路（三座大阵只在旁白）、环心孤礁 | **西**口 `portal_huijie` → `ch11_ningcui_jie`（spawn `dong`，朝左） | spawn `xi`；`trigger_buzhen`（登岸格，enter）；`trigger_huicheng`（同格，interact，`guard ch11.buzhen`；O-7）；`trigger_youyao`（环心，interact，`once` 否）；`facility_cunji_hh`（save，在登岸格旁） | 无具名 NPC（海兽与千目妖只在编成里） |
| `ch11_yinsha_dao` | 隐煞门礁岛 | wild / day / cliff_top | 28×22：礁石小岛、残阵 | **无门**（只进不出：n21 teleport 进、n24 teleport 出） | spawn `haishang`；`trigger_guiju`、`trigger_wuchou`、`trigger_daoge`（同格三连，enter，按旗标换） | `npc_zi_ling`（role `zi_ling`，名牌「紫灵仙子」）、`npc_fan_furen_3`、`npc_zhuo_ruting_2`、`npc_zhao_zhanglao_2`、`npc_fu_zhanglao`、`npc_meng_daoyou`、`npc_chihuo_laoguai`（ch11.zhuzhan → ch11.yaoshi）；`npc_yinsha_menzhu`、`npc_wu_chou`（ch11.guiju → ch11.yaoshi）；`npc_jiyin_fushen`（ch11.jiyin → ch11.yaoshi） |
| `ch11_xiaoxuan` | 青竹小轩 | indoor_wood / indoor / inn_hall | 20×14：铺面、柜台、后宅 | **南**口 `portal_chudian` → `ch11_fangshi`（spawn `xiaoxuan_men`，朝下） | spawn `dian`；`trigger_jiesuan`（柜台，interact，`once` 否）；`facility_guitai`（shop `ch11_qingzhu_xiaoxuan`；O-6）；`facility_cunji_xx`（save） | `npc_he_xianshi_2`、`npc_linju_dianzhu`（ch11.kaidian → ch11.liugen） |

- **门的拓扑**：坊市 ↔ 洞府一对门；星空殿只从洞府北口上（`require_flag ch11.nichang`——悟出霓裳草之前没有出海的理由）；凝翠岛 ↔ 红瑚岛一对门；青竹小轩一对门。星空殿传送阵与凝翠岛石屋是脚本 teleport（登 N.4 `kScriptTransfers`）；隐煞门礁岛没有门，规则 29 对它无要求。
- **为什么坊市是一张图、三区**：城门、坊市街、山脚楼阁与茶馆雅间在原著里相隔不远（ch390–391 一日之内走完），拆开要多两张图、超出 7 张预算（E7）；合成一张照第 10 章「魁星岛一张图四个区」。**区与区之间不设门闸**：茶馆雅间的门只在 `ch11.lianer` 后开，青竹小轩的门只在 `ch11.kaidian` 后开——其余时候那两扇门的 `deny` 文案是一句「门关着」。
- **洞府设施的门闸**：静室、丹房、药园 `require_flag ch11.anju`（n05 安顿之后）；炼器台 `require_flag ch11.liugen`（六根金雷竹齐之后，E16）——四处都是现成的设施 `require_flag`（`Application::openFacility` 已判，未置时说 `ui.facility.locked`）。
- **为什么出海的两张图在 `ch11.huicheng` 之后不再可去**：原著五年收手之后他再没去过凝翠岛（ch397 起闭府）；星空殿 n09 脚本在 `ch11.huicheng ≥ 1` 时只说一句「本章不再出海」并 `return`，不设回程门（同第 10 章 16.1 第 21 条口径）。
- **每一处挂战斗的格子前面都有存档点**：① ② 红瑚岛登岸格旁（`facility_cunji_hh`，环心触发器走两步就到）；③ ④ 隐煞门礁岛——**没有存档点**（只进不出、同格三连，中间玩家有控制权但不能离岛）：③ 之前的存档在 n21 之前的洞府（`facility_cunji_39`），③ 与 ④ 之间 n23 的演出不可跳过——这是本章唯一「连打两场」的地方，④ 的 `defeat_is_fatal true` 要让平衡路按「刚打完 ③ 不回血」量。
- **命名与地图属性**：`bgm`：坊市 `bgm_town`、洞府 `bgm_valley`、星空殿 `bgm_valley`、凝翠岛 `bgm_river`、红瑚岛 `bgm_cliff`、隐煞门礁岛 `bgm_wild`、青竹小轩 `bgm_town`（**全是已有的曲子**；`tools/audiogen` 的 `MAP_BGM` 登记表跟着加 7 条、`docs/audio.md` 同一张表）；脚本点播一处（n23 极阴附身那一段 `bgm_night`，演完 `bgm("map")`）。
- **主题一律用现有的**（E7）：城用 `town`，洞府与星空殿用 `indoor_stone`（大理石殿靠 ambient 提亮），凝翠岛小街用 `dock`，红瑚岛与礁岛用 `cliff`/`wild` ＋ `mist`，店铺用 `indoor_wood`。**不加新主题、不新做战斗背景**（第 8 节四场都用 `cliff_top`）。
- `tools/mapgen/genmaps_ch11.py`（7 张）接进 `genmaps.py --check`；**本章不 patch 任何旧图**（E1 的开局接法是 patch 脚本 `scripts/ch10/zhifadui.lua`，不是地图）；美术 `tools/artgen/artgen.py` 烘焙 7 张新图并 `--check` 过。
- **spawn 与落点清单**（地图路与脚本路对表；规则 29 的朝向写在括号里）：

| 地图 | spawn | 谁用它 |
| --- | --- | --- |
| `ch11_fangshi` | `chengmen`（城门，朝上） | `scripts/ch10/zhifadui.lua` 末尾的 teleport（E1）；章节卡在这一格上屏 |
| `ch11_fangshi` | `shanjiao`（北口内，朝下） | 从洞府下山 |
| `ch11_fangshi` | `xiaoxuan_men`（店门外，朝下） | 从青竹小轩出来 |
| `ch11_dongfu` | `men`（南口内，朝上） | 从坊市上山；n04 `zufu.lua`、n14 `huicheng.lua`、n24 `daoge.lua` 三处 teleport 落点 |
| `ch11_dongfu` | `beimen`（北口内，朝下） | 从星空殿下来 |
| `ch11_xingkong` | `dian`（殿内，朝上） | 从洞府北口上；凝翠岛石屋 `huicheng_chuansong.lua` 的 teleport 落点 |
| `ch11_ningcui_jie` | `shiwu`（石屋门外） | n09 `xingkong.lua` 的 teleport 落点（每次出海都落这里） |
| `ch11_ningcui_jie` | `dong`（东口内，朝左） | 从红瑚岛回来 |
| `ch11_honghu` | `xi`（登岸格，朝右） | 从凝翠岛过来；`trigger_buzhen` / `trigger_huicheng` 就挂在这一格 |
| `ch11_yinsha_dao` | `haishang`（礁岛边缘） | n21 `zhuzhan.lua` 的 teleport 落点；三连触发器挂在它前面一格 |
| `ch11_xiaoxuan` | `dian`（铺面，朝上） | 从坊市进店 |

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
| `yaoyan_furen` | 妖艳妇人 | ch393 | 筑基后期 | 新建（arch_cultivator_f） | 否 | ch11.nichang → ch11.chuanshong | xingkong |
| `tongxing_xiushi` | 同行修士 | ch393 | 结丹初期 | 新建（arch_cultivator_m；两对象同 role） | 否 | ch11.nichang → ch11.chuanshong | xingkong |
| `zaohong_laozhe` | 枣红脸老者 | ch393 | 结丹初期 | 新建（arch_elder_m） | 否 | ch11.nichang → ch11.chuanshong | xingkong |
| `ningcui_shouzhen` | 守阵修士 | ch393 | 筑基中期 | 新建（arch_disciple） | 否 | ch11.chuanshong → ch11.done | ningcui_jie |
| `zahuo_nc` | 杂货铺掌柜 | ch394 | 炼气八层 | 新建（arch_merchant） | 否 | ch11.chuanshong → ch11.huicheng | ningcui_jie |
| `fan_xiaozi` | 范小子 | ch394 | 炼气六层 | 新建（arch_merchant） | 否 | ch11.chuanshong → ch11.huicheng | ningcui_jie |
| `hao_yuantian` | 郝远天 | ch394 | 结丹初期 | 新建（arch_cultivator_m） | 否 | ch11.ningcui → ch11.huicheng | ningcui_jie |
| `qianmu_yao` | 千目妖 | ch396 | 五级（CoreEarly） | 新建（妖兽，非人形） | 是（① 首领） | 编成 | 战斗 ① |
| `siji_haishou` | 四级海兽 | — | FoundationMid | **就地改写** `weaknesses`（见下表） | 是（① 第二波） | 编成 | 战斗 ① |
| `jiguan_jiao` | 鸡冠蛟 | ch396 | 六级（CoreMid） | 新建（妖兽，非人形） | 是（② 首领） | 编成 | 战斗 ② |
| `lian_er` | 莲儿 | ch399 | 炼气三层 | 新建（arch_maid） | 否 | ch11.zhaolan → ch11.lianer | fangshi |
| `fan_furen` | 范夫人 | ch399（范静梅）/ ch400 | 筑基后期 | **新建**（总表有、文件无；arch_noblewoman） | 否 | 雅间 ch11.lianer → ch11.chaguan；礁岛 `_3` ch11.zhuzhan → ch11.yaoshi；雅间 `_2` ch11.yaoshi → ch11.zhanglao | fangshi、yinsha_dao |
| `zhao_zhanglao` | 赵长老 | ch400 | 结丹初期 | 新建（arch_elder_m） | 否 | 雅间 ch11.lianer → ch11.chaguan；礁岛 `_2` ch11.zhuzhan → ch11.yaoshi | fangshi、yinsha_dao |
| `zi_ling` | 紫灵仙子 | ch403 | 结丹初期（已有） | **只用名牌、不上场、不改数值**（它是第 12 章 `b12_xutian_hudian_kuilei` 的） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `zi_ling_zhenrong` | 紫灵 | ch409 | 筑基后期 | 新建（arch_cultivator_f；真容，数值同 `zi_ling` 口径） | 否 | ch11.yaoshi → ch11.zhanglao | fangshi |
| `zhuo_ruting` | 卓如婷 | ch402 | 筑基后期 | 新建（arch_cultivator_f） | 否 | 礁岛 ch11.zhuzhan → ch11.yaoshi；雅间 ch11.yaoshi → ch11.zhanglao | yinsha_dao、fangshi |
| `fu_zhanglao` | 符长老 | ch404 | 结丹初期 | 新建（arch_cultivator_m；原著「儒生长老」，B6） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `meng_daoyou` | 孟道友 | ch402 | 结丹初期 | 新建（arch_cultivator_m；不叛，B6） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `chihuo_laoguai` | 赤火老怪 | ch403 | 结丹后期 | 新建（arch_demonic_m） | 否 | ch11.zhuzhan → ch11.yaoshi | yinsha_dao |
| `yinsha_menzhu` | 隐煞门门主 | ch404 | 结丹后期 | 新建（arch_demonic_m；名牌不写姓） | 否 | ch11.guiju → ch11.yaoshi | yinsha_dao |
| `yinsha_shayao` | 隐煞门煞妖 | ch404 | 筑基后期 | 新建（arch_demonic_m，变身态用 enemies 画） | 是（③ ×4） | 编成 | 战斗 ③ |
| `wu_chou` | 乌丑 | ch382 | 结丹中期 | **一个字不动**；只说话（O-2） | 否 | ch11.guiju → ch11.yaoshi | yinsha_dao |
| `jiyin_fushen` | 极阴祖师 | ch382（名号）/ ch405 | realm 写 CoreLate（O-9） | 新建（arch_demonic_m；只说话） | 否 | ch11.jiyin → ch11.yaoshi | yinsha_dao |
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
- **首尾相接的**：卓如婷（礁岛 hidden `ch11.yaoshi` = 雅间 visible `ch11.yaoshi`）；范夫人 `_3` → `_2`（礁岛 hidden `ch11.yaoshi` = 雅间 `_2` visible `ch11.yaoshi`）；紫灵两个 role 不同名牌（`zi_ling` → `zi_ling_zhenrong`），不算同 role；星空殿一行人与枣红老者同以 `ch11.chuanshong` 撤场。
- **待协调者核的三处**（本图照 H/G 表抄，不改 id，若规则 30 用例红则按这里的建议挪旗标）：① `fan_furen` / `zhao_zhanglao` 在坊市雅间的 hidden 是 `ch11.chaguan`、在礁岛的 visible 是 `ch11.zhuzhan`——中间隔一个节点 n21（茶馆散了、半月之后才赴集合岛），hidden ≠ visible；建议礁岛那两个对象的 visible 保持 `ch11.zhuzhan`（它们本就靠 n21 的 teleport 才见得到），用例若只查同图则不红；② `he_xianshi` 两张图两个对象、同一对旗标（`ch11.kaidian → ch11.liugen`）同时在场——原著他是隔壁邻居、串门常客（ch412），两处同在不算矛盾，但若用例判「同 role 不得同时在两图」，去掉坊市街上的 `npc_he_xianshi`（打探 `hexianshi_dating_fanren` 挪到店内对象）；③ `jiedan_linju_a` / `_b` 同图同 role 同旗标——O-4 已列。
- **目标链之外的在场旗标**：`ch11.lianjian`（n30 由 `trigger_chujian` 置，金青以它出场）在目标链上；`ch11.jiguan`、`ch11.jiesuan`、`ch11.z1`、`ch11.z2`、`ch11.zhu`、`ch11.zhu_gen` 不作在场旗标用（登 `kOffChainFlags`，N.4）——**在场旗标里没有一个不在目标链上的**。

---

## 6. 支线（2 条可选，0 条必经）

| | Z1 收购行订单（可选） | Z2 邻居的恭贺（可选） |
| --- | --- | --- |
| 出处 | ch389 猎妖回城换灵石的流程（无许可制度，A6）；接单形式是本作改编（替代大纲的「猎妖令」，B9、A19） | ch399 出关后邻居恭贺与招揽、以定颜丹换珍稀物（A48） |
| 触发（`accept`） | `ch11.ningcui`（到了凝翠岛小街，范小子常驻） | `ch11.zhaolan`（传音符之后，结丹邻居出场） |
| 步骤 | 一步：与 `npc_fan_xiaozi` 对话接单（`ch11.z1 = 1`），再带 3 颗五级妖丹回来交付（`take("material_yaodan_wuji", 3)` 成则 `ch11.z1 = 2`） | 一步：`npc_jiedan_linju_b` 的 interact 脚本 `linju_z2.lua`：`take("pill_dingyan_dan", 1)` 成则 `give("material_lianqi_yao", 3)`、`ch11.z2 = 1`；没丹则一句（路径行动 `linju_qiugou_dingyan` 只打探「邻居想要定颜丹」，O-3） |
| 完成 | `ch11.z1 == 2`：酬 中阶灵石 10 ＋ `material_lianqi_yao` 2（由脚本 `give`，不进 rewards——第 10 章口径） | `ch11.z2 == 1` |
| 失败 / 过期 | `ch11.huicheng`（五年收手回城，凝翠岛再到不了） | 无过期（邻居到 `ch11.done` 都在） |
| 任务板 | 坊市 `facility_xuanshang` 也列它（E10） | 不列 |
| 文件 | `data/quests/q11_shougou_dingdan.json`，`origin: "改编"` | `data/quests/q11_linju_dingyan.json` |

**为什么只有这两条、而且都可选**：本章目标链已 31 步（E26），必经支线要进目标链；原著里韩立在场、有来龙去脉的旁枝就这两件。Z1 给猎妖循环一个「订单」的由头（大纲 4.4 的改编标注落在 `origin` 与图鉴注上）；Z2 让起点背包里那三四颗定颜丹有一处去路（C.4）。**全作 45 条 / 必经 14 条的账由协调者核；若账要本章出 1 条必经，把 Z1 改必经并进目标链（O-8）。**
不做成支线的：鲁二雇不雇（n03 的二选一，信息量相同）、丰乐拍卖行与储物袋求购（路径行动 `fengle_dating_mulu`、`chuwu_qiugou_dai`，E9）、枣红老者的联手之邀（n10 旁白一句，曲魂回绝，A29）、郝远天的招揽（n10 旁白）、摘星大会（`baimen_dating_zhaixing` 一句传闻，A8）、妙音门名义长老的年俸（N.1：本章之后再无「年」的结算口）、金青请破阵（第 12 章的事，O.1）。

---

## 7. 系统：租赁与月费、星空殿传送、猎妖循环与妖丹经济、六十年闭关、青竹小轩、七十二剑、结丹口径、分身

**先查了已有的，再定做多少**（D 表）：境界枚举 `CoreEarly 31` 与 Lua `realm.advance/cap` 都在；冲关加成入口 `breakthroughPillBonus` 仍是返回 0 的桩（`CultivationScene.cpp` 第 363 行）；静室 `effectiveness` 属性已有；`advanceDays` 单次上限 `kMaxScriptAdvanceDays = 3650`；商店 `Shop{buyRate,sellRate,entries}` 与 `ShopScene` 的 Buy/Sell 两页、只收 `tradeable` 的；拍卖只有规则层 `bidStep`（数据目录、加载器、面板、Lua 入口全缺）；任务板 `BoardScene`；遭遇表的掉落只许灵草（`validate.py` 第 2716–2731 行）；灵田 `field.unlock`；小瓶 `bottle.spend/mature/drops`；配方 `requireFlag`、`difficulty`、`requiredProficiency`；脚本命令清单里没有加熟练的命令；`party.add/remove/has`；NPC `visible_flag`/`hidden_flag`；路径行动 `when`/`until`；`today()`。**本章不加任何新脚本命令、不动引擎**。

| 解锁 | 做什么 | 不做什么 | 为什么 |
| --- | --- | --- | --- |
| **租赁与月费** | **红环 / 蓝环**（n02）：曲魂的红环免费（结丹待遇，A10）；韩立 `take("material_lingshi_zhong", 8)` 成则红环 `ch11.zhihuan = 1`，否则 `take("material_lingshi", 3)` 蓝环 `= 2`，再不成也给蓝环、旁白换句（A9、A11）。**洞府**（n04）：执事台词列三十一层三座、三十九层四座、四十七层两座（A16），只能选三十九层；`take("material_lingshi_zhong", 27)`，不足则有几块扣几块、执事「记在结丹前辈名下」（E13）；给 `story_jinzhi_lingpai`。**「每月缴灵石」**只在典籍与路径行动 `baiyi_dating_guiju` 的台词里（A4、B7）；**「百年」**只在 n04 旁白里（A15）。静室 `facility_jingshi_39` `effectiveness 125`（A18、E11）；药园 `field.unlock("field_tianxing_39", 6)`（E12） | 不做月费扣款、不做续租、不设任何付款门闸（E13）；不做层数选择的分支（三选一只在台词里） | 原著「每月」（ch389）与「百年」（ch391）两句都有，做机制只能择一而且与「结丹免费」撞车；付不起的玩家在 first 侧 117 中阶之外的任何合法花法上都不该死局 |
| **星空殿传送** | n09 挂在 `npc_xingkong_xiushi` 上、`once` 否。首次：拼阵，`take("material_lingshi_zhong", 2)`（A26–27 的二百），不足则「对方全出」；此后每次出 / 回 `take(..., 7)`（一次七百、最多七人，A26），不足则「搭别人的阵」免费（E14，改编一句旁白）。回程走凝翠岛石屋 `trigger_huicheng_chuansong` → `teleport("ch11_xingkong", "dian")`。`ch11.huicheng ≥ 1` 后 n09 只说「本章不再出海」并 `return` | 不做按人头计费、不做传送符物品（只在台词）、不做妖兽岛列表（只去凝翠岛 → 红瑚岛一条线）、不做组队（A32） | **传送永远走得通**：700 硬门闸 ＋ 一趟只回 1–2 颗丹，玩家可能卡在凝翠岛（E14） |
| **猎妖循环与妖丹经济** | **诱妖**（n12，`trigger_youyao` interact、`once=false`）：要 `herb_nichang_cao ≥ 1` 且 `bottle.drops ≥ 1`；扣 1 株 ＋ `bottle.spend(1)` → `advance_days(3)`（A37「每隔两三日」）→ `battle("b11_ningcuidao_youyao")`；胜则 `ch11.lieyao += 1`，妖丹由编成 `drops` 给（五级妖丹 1 颗 rate 100）；缺草 → 旁白指凝翠岛杂货；缺绿液 → 「等几日」`advance_days(5)`。`ch11.lieyao == 4` 且 `ch11.jiguan == 0` → n13 鸡冠蛟（可逃、不致命；胜给 `material_yaodan_liuji` 1）。**收手**（n14，`ch11.lieyao ≥ 3`）：五年蒙太奇 `advance_days(1800)`，给五级妖丹 8、`story_yaoshou_cailiao`、`story_kuilei_can`，收 `story_kuilei_qun`。**卖丹**：凝翠岛收购行 `ch11_ningcui_shougou`（`entries` 空、`sellRate 0.8`，五级 760、六级 4800——由 `tradeable` 决定收什么，E8）；Z1 订单另给中阶 10。霓裳草两处杂货都卖（价 10；坊市 stock 10 / 凝翠岛 stock 5，各 5 日补货） | **不设遭遇区**（遭遇编成掉不了妖丹，E15）；不做猎妖令、不做许可（A6、B9）；不做「数百颗」的真实数量（只在旁白，玩家实得 5–10 颗，A40）；不做傀儡群的编成（旁白里「群射」，沿第 10 章 16.1 第 23 条）；不卖六级妖丹（只有 ② 一颗，第 12 章入药的钩子，A73） | 循环的动机要算得过来：收购行默认 `sellRate 0.5` 一颗只回 475，比一趟传送费（7 中阶）还薄；0.8 才有「一颗近千」（A7）的感觉。妖丹是本章唯一的「挣钱」——n26 会把它们扣光（A64），所以给多少都是原著的「倾家荡产」 |
| **六十年闭关的表现** | n16 `trigger_biguan`：先 `take("pill_jiangchen_dan", 5)`、`take("material_xuelingshui", 2)` 各看返回值，位掩码 `ch11.dan_fu`（1 降尘丹、2 雪灵水）选旁白三版（全扣 / 部分 / 零）；**三段蒙太奇**（E5）：`fade.out` → 旁白「十年」→ `advance_days(3600)×2` → 曲魂隔门一句（噬金虫互噬、产卵）→ `advance_days(3600)×2` → 旁白「三十年」→ `advance_days(3600)×2` → 天象旁白（黑云闪电、百余里灵气漩涡，A43）→ `local ok, code = realm.advance(realm.CORE_EARLY)`（不成则占位一句 `return`）→ `fade.in`；置 `ch11.jiedan`。n15 之前的「散功半月到筑基初期」只写旁白、**不 `realm.demote`**（A42） | 不抬 `kMaxScriptAdvanceDays`（引擎项）、不做新的「岁月流逝」画面（美术项）、不接 `breakthroughPillBonus`（E4）、不做面板冲结丹（E3）；二转年数回避（B16）；金丹「四色」可写、年数只写「数年」（A44） | 游玩时长 `playSecondsGameplay` 只按真实秒累计，`advance_days` 不碰它，「跳过的时间不计游玩时长」天然成立（文档注明即可）；六次 3600 的灵田 / `restFor` 结算是否溢出由引擎路跑 `Ch11EngineTests`（O-5） |
| **青竹小轩经营与回访结算** | n27 盘店 `take("material_lingshi", 50)`（有几扣几）；「李氏杂货」改「青竹小轩」（A66）；`flag.set("ch11.kaidian_ri", today())`、`ch11.jiesuan_ri = today()`；店门 portal 开。n28 柜台 `trigger_jiesuan`（`once=false`，E22）：读两枚日期，每满 30 日给 15 低阶灵石（一次上限 7200 日 = 3600），再 `ch11.jiesuan_ri = today()`，`ch11.jiesuan += 1`（首次附旁白）。商店 `ch11_qingzhu_xiaoxuan`（买入页空、卖出 `sellRate 0.6`：玩家卖符与药材）。收店（n29 `zhu_gen == 6`）：`advance_days` 补足到开店起 7200 日（照 `sanzhuan.lua` 的补足写法，至少 360）、`give("material_lingshi_zhong", 20)`、店门与柜台关（`hidden_flag ch11.liugen`） | 不做真实库存经营（新模块）、不做雇工、不做店铺升级；不卖丹药（B8：ch412 只卖符与药材、后加低阶法器）；韩立易容、曲魂扮仆只在文案，名牌不改 | 开店在原著是磨入世心境（A67），本作把它做成「时间换灵石」的一口结算：培竹三年一根（n29）与回访结算共用同一本日历，玩家每回店一次就看见二十年怎么过去。**O-6**：玩家卖符给自己的店若怪，改成只结算、不设 shop facility |
| **七十二剑炼成** | **培竹**（n26，`trigger_peizhu`、`once` 否）：每次 `bottle.spend(1)` 成则 `ch11.zhu += 1`（1 白 → 2 蓝 → 3 金，A63），不成「等几日」；`zhu == 3` 置 `ch11.jinlei`：`take("material_tianleizhu", 1)`、`give("material_jinleizhu", 1)`；两年凑辅料 `take("material_lingshi_zhong", 全部)`、`take("material_yaodan_wuji", 全部)`、`advance_days(720)`、`give("material_lianjian_fuliao", 1)`（**必给**，不看扣到多少，A64）。**六根**（n29，同一触发器换分支）：每次 `bottle.spend(1)` → `advance_days(1080)` → `ch11.zhu_gen += 1`、`give("material_jinleizhu", 1)`；到 6 置 `ch11.liugen`。**炼剑**（n30）：`trigger_lianjian`（同格 enter，演在面板前：打坐三日、六根炼液、布阵半月、黄道吉日，`advance_days(120)`）→ 玩家在 `facility_lianqi_39`（forge，grade 3，`require_flag ch11.liugen`）选 `recipe_qingzhu_fengyun_jian`（**就地改写**：`productCount 1`、`inputs` 金雷竹 6 ＋ `material_lianjian_fuliao` 1 ＋ `material_lingshi` 20、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`，面板必成，E16、B14）→ `trigger_chujian`（密室门外 enter，要 `item.count("weapon_qingzhu_fengyun_jian") >= 1`）：`advance_days(60)`、七十二口入体（A71）、置 `ch11.lianjian` | 天雷竹不走 `matureHerb`（年份跳万年不合它的曲线）；不加 Lua 加熟练命令（引擎项）；不做剑阵 / 剑影分光术的法术（E17，第 12 章定）；一件 `weapon_qingzhu_fengyun_jian` 代表七十二口（note 已这么写） | 剧情直接 `give` 则炼器面板全作没有用过一次（E16）；炼器熟练是 0（C.4），配方 `difficulty 0 / requiredProficiency 0` 才必成；原著他是一次炼成、无材料再试（B14） |
| **结丹口径** | **剧情 `realm.advance(realm.CORE_EARLY)` 恰 1 处**（n16），`realm.cap` 0 处、`realm.demote` 0 处（C.1）；`storyCap` 保持 23 到 n16，之后由 `realm.advance` 顺带抬到 31；面板上的下一关「结丹中期」所需修为 5000，本章**不许过**（`realm.cap` 不抬到 32，面板按 `BreakthroughBlock` 回绝，C.3）。结丹初期锚点：气血 700、攻 62 / 防 46（`Realm.h` 注释第 110–115 行，C.2）——**本章就是标定结丹初期的那一章**：平衡路用 `tests/BattleHand.h` 量 I 表四场后，把锚点注释改成「第 11 章标定」。出关（n17）：小瓶 3 日一滴由引擎按境界给（`Application.cpp` 第 130 行）；E27 起外人直接与韩立说话 | 不做面板冲结丹（面板要 3500 修为、可反复摇骰，与「六十年一关」的重量相悖，E3）；降尘丹、雪灵水只「有则扣」、不接加成（E4）；天火液 0、血凝五行丹 0 不提（C.3） | 原著是六十年生死关、「意外成功」（ch398），面板摇骰子会把一次性的大事变成可刷的事；起点修为 2092 / 2031 距 3500 尚远（C.4），也佐证不该走面板 |
| **分身** | `qu_hun_shadan` 全章在队、数值与名牌不动（E18）。**曲魂代言**（E27）：入城、租府、星空殿、凝翠岛小街外人先跟曲魂说话（ch390–394 原著如此），第一次交代一句，不再解释；n17 结丹之后外人直接跟韩立说话。n25 紫灵识破曲魂是身外化身、与本体同为结丹初期——只是台词一句（A61）。战斗里四场都在队（第 8 节：①② 主攻、③ 并肩压制、④ 紫焰困妖） | 不做 `hero_absent`（本章四场韩立都在场，与第 10 章 ②④ 不同）；不改 `qu_hun_huashen`/`qu_hun`；不做化身的装备栏；n06 「炼化混元钵」只一句旁白 | 第 10 章把「分身的三种用法」演完了；本章它是一个稳定的结丹队友，分量在**他人眼里**——执事记账、紫灵识破、妙音门招的是「两位结丹」 |

**本章所有门闸一览**（没有一处是钱；全部是旗标或物品计数）：

| 门闸 | 在哪 | 条件 | 未满足时 |
| --- | --- | --- | --- |
| 洞府静室 / 丹房 / 药园 | `ch11_dongfu` 三处 facility | `ch11.anju` | `ui.facility.locked` |
| 坊市杂货铺 | `facility_zahuo` | `ch11.nichang` | 同上（悟出霓裳草之前没有买草的理由） |
| 星空殿北口 | `portal_xingkong` | `ch11.nichang` | `ch11.dongfu.xingkong_deny` |
| 出海 | n09 脚本 | `ch11.huicheng == 0` | 「本章不再出海」`return` |
| 凝翠岛东口 | `portal_chuhai` | `ch11.ningcui` | 门关着 |
| 诱妖 | `trigger_youyao` | `ch11.buzhen`、草 ≥ 1、绿液 ≥ 1 | 旁白指杂货 / `advance_days(5)` |
| 鸡冠蛟 | n12 内分支 | `ch11.lieyao == 4` 且 `ch11.jiguan == 0` | 继续 ① |
| 收手 | `trigger_huicheng` | `ch11.lieyao ≥ 3` | 「再猎几次」 |
| 茶馆雅间门 | 坊市东北角 portal | `ch11.lianer` | 门关着 |
| 青竹小轩门 | 坊市街西 portal | `ch11.kaidian`（`hidden ch11.liugen`） | 门关着 / 收店后消失 |
| 炼器台 | `facility_lianqi_39` | `ch11.liugen` | `ui.facility.locked` |
| 出剑 | `trigger_chujian` | `item.count("weapon_qingzhu_fengyun_jian") >= 1` | 不响 |
| 配方 | `recipe_qingzhu_fengyun_jian` | `requireFlag ch11.liugen`、金雷竹 6、辅料 1、灵石 20 | 面板灰掉 |

**起点与章末对照**（写测试时读表，不抄数；起点读自 `tests/fixtures/ch10-end-first.sav`，章末待 `ch11-end` 真实验收）：

| | 起点（C.4 实读） | n16 之后 | 章末 spec |
| --- | --- | --- | --- |
| 境界 / 上限 | 23 / 23 | 31 / 31 | 31 / 31 |
| `formerRealm` | 22 | 22 | 22 |
| 气血 / 攻 / 防（Realm.h 锚点） | 480 / 44 / 30 | 700 / 62 / 46 | 700 / 62 / 46 |
| 静室效率 | — | 125（三十九层） | 125 |
| 瓶子 | 容量 6、五日一滴 | 容量 6、三日一滴 | 同左 |
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
| ① | `b11_ningcuidao_youyao`（**就地改写**） | **千目妖**（`qianmu_yao`，五级，首领；蓄势「刺球光罩」）；第二波 **四级海兽** ×2（`siji_haishou`，随草香而来，改编）；`sanji_haishou` 去掉 | 韩立 ＋ 曲魂 | **可逃**（`can_escape true`；I 表附注「逃了不扣草」而 F 表 n12 在 `battle` 之前已扣草——脚本按 `battle` 返回值在逃时 `give` 回 1 株，或把扣草挪到胜后，由脚本路定、协调者核） | **致命**（A34 原著「几次陷入生死一线」） | **阵中先手、破势优先**：首领蓄势一次必须被破势打断，否则全体受伤；破绽「土」（流沙术、土牢符）与「暗器」（第二波，透明丝线）都是玩家手上现成的；曲魂混元钵为主攻。打赢掉 1 颗五级妖丹——**让玩家第一次看见妖丹值多少**。`rewards.cultivation > 0`（本章唯一） |
| ② | `b11_honghu_jiguanjiao`（新） | **鸡冠蛟**（`jiguan_jiao`，六级，首领、多动；三回合内蓄势伤害明显高于五级） | 韩立 ＋ 曲魂 | **可逃**（`can_escape true`） | **不致命**（`defeat_is_fatal false`：原著它徘徊离去，A38） | **第一次照面六级——逃是正解之一**：破绽「金」要靠祭金符或曲魂金剑；打赢给 `material_yaodan_liuji` 1（第 12 章入药的钩子，A73），逃或败同一句旁白、不给。胜后文案与图鉴注「改编」 |
| ③ | `b11_yinsha_shayao`（新） | **隐煞门煞妖** ×4（`yinsha_shayao`，筑基后期；第 2 回合「变身」用 `charge` 写成全体蓄劲） | 韩立 ＋ 曲魂 | 否 | **致命** | **结丹初期第一仗——连击清场**：境界压制让四个筑基后期打不动他；四敌同时蓄劲，教「拳 / 剑的蓄劲连击 1+N」在变身前打掉几个；破绽含「剑」（烈阳剑、青竹蜂云剑还没有）与「木」（青元剑芒）。编成取 4（B17） |
| ④ | `b11_tiandu_shi`（新） | **妖尸**（`tiandu_shi`，首领、多动、瞬移；每回合自愈；`killable_by` 空——**只能被困**） | 韩立 ＋ 曲魂 ＋ `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2 | 否 | **致命**（A59 原著「狼狈亡命」，败即被追上） | **打不死的东西**：胜利条件在数据上是「它的架势被破两次」——`toughness` 低、`weaknesses` 只有「水」（蛛网 / 冰冻术）与「火」（紫火 / 紫焰）；友军 `actions` 写「吐网」「紫火」，玩家要用它们把它困住。**胜 = 困住，不写杀死**（E25）。O-1：若引擎胜负只看气血，平衡路把它气血压低、自愈用 `charge` 表达，先用 `BattleHand` 证明 |

- **每场的 `charge` / `actions` 框架**（只写名目与节拍，数归平衡路）：

| # | 首领蓄势（名目 / 节拍 / 目标） | 其余行动 | 友军 `actions` |
| --- | --- | --- | --- |
| ① | 千目妖「刺球光罩」：每 3 个出手回合一次、全体；被破势则取消 | 千目妖普攻单体；四级海兽 ×2 第二波入场、普攻 | 曲魂：剑、血色光柱（火）、金剑 |
| ② | 鸡冠蛟：多动（行动 2）；蓄势伤害明显高于五级、三回合内落下 | 鳞甲护体（破绽只有「金」打得透） | 同上 |
| ③ | 煞妖 ×4：第 2 回合「变身」全体蓄劲（`charge`）；变身后攻防抬一档 | 变身前普攻单体 | 同上 |
| ④ | 妖尸：多动、瞬移（行动顺序随机）；每回合「自愈」（`charge` 表达，O-1） | `killable_by` 空；破绽恰「水、火」 | 血玉蜘蛛 ×2「吐网」（水，压架势）；猿傀 ×2「紫火」（火）；曲魂：紫焰（火，全体） |

- **规则 25 的必有手段**（交协调者登进 `tools/validate.py`，E19）：`CHAPTER_MEANS[11]` = 拳（hand）、火（`magic_huodan_shu`，ch04）、土（`magic_liusha_shu`，ch06）、水（`magic_bingdong_shu`，ch06）、金（`magic_ji_jinfu`，ch07）、木（`magic_qingyuan_jianmang`，ch08）、暗器（`weapon_wuming_sixian`，ch07）、剑（烈阳剑 give ch06 `lingqu.lua`）；`BATTLE_EXTRA_MEANS` 四场各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金（照 `b10_zhifadui` 写法）；`MEANS_LAST_CHAPTER = 11`。四场「至少一样破绽是本章我方手段打得到的」：① 土 ② 金 ③ 剑 ④ 水。
- **selftest 探针**：`validate_selftest.py` 第 1801–1820 行指 `siji_haishou` + `b11_ningcuidao_youyao`、要求敌方含 `siji_haishou`——本章写实这张编成后挪到 `wu_chou` + `b12_neidian_duoding`（全仓唯一含 `wu_chou` 的编成），措辞改「第 12 章范围探针实际引用乌丑」（E19、O-2）。
- **数值归战斗路按 `tests/BattleHand.h` 那只手量**；本表只写意图与破绽。必须守住的：① `qianmu_yao` ×1 ＋ `siji_haishou` ×2、破绽含「土」「暗器」、`can_escape true`、`defeat_is_fatal true`、`drops` 五级妖丹 1 颗 rate 100、`rewards.cultivation > 0`；② `jiguan_jiao` ×1、破绽含「金」、`can_escape true`、`defeat_is_fatal false`、`drops` 六级妖丹 1 颗 rate 100；③ `yinsha_shayao` 恰 4、破绽含「剑」、`can_escape false`、`defeat_is_fatal true`；④ 友军恰 `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2、`killable_by` 空、破绽恰「水、火」、`can_escape false`、`defeat_is_fatal true`；四张 `hero_absent false`、`spirit_stones == 0`；③④ `drops` 空、`cultivation 0`（③ 之后他已是结丹初期、上限 31，给了也按不动）；② `cultivation 0`。四场都要写 `weapons / toughness / weaknesses / actions / charge`（`docs/octopath-battle.md` 第 5 节），门禁规则 24、25。
- 战斗背景（`data/visual/battles.json`）：① ② `cliff_top`（红瑚岛珊瑚礁）、③ ④ `cliff_top`（隐煞门礁岛）——**不新做背景**（第 4 节）。每一条都要能指出读它的那一行代码并有走真实入口的测试（handoff 第 6 节第 5 条）。
- 开场白（`data/text/ch11_battle.json`）：① 写草香散开、绿眼从雾里亮起（**不写它怎么滚过来**）；② 写蛟来得比预想的早一百年；③ 写四人身形暴涨的一瞬（**不写门主的台词**）；④ 写空间扭曲、它站在那里不动——四条都自己写、不抄原著。旧的 `b11_ningcuidao_youyao` 那条占位开场白请协调者删。

### 8.3 每场仗的由头与「不冲突」

| # | 原著怎么说 | 本稿怎么做 | 为什么不冲突 |
| --- | --- | --- | --- |
| ① 千目妖 | ch396：霓裳草展叶，千目妖入幻形天罗阵，傀儡群射、一顿饭取丹；此后一个多月每隔两三日一批、灭十几头五级（A36–37） | 可重复的剧情战：每次扣草与滴、推三日、打一场、掉一颗丹；第二波海兽是改编 | 原著是「一批一批来」，本作每一批就是一场；傀儡群在旁白里「群射」、不进编成（第 10 章 16.1 第 23 条）；第二波四级海兽「随香而来」是为了让 `siji_haishou` 这个旧 role 有一条「暗器」的路、图鉴注改编 |
| ② 鸡冠蛟 | ch396：草到四百年份引来六级鸡冠蛟，他封住草香、它徘徊离去；此后只催到三百年（A38） | 一场可逃、不致命的仗；逃 = 原著；胜 = 改编（给六级妖丹） | 原著没有交手，「逃」就是原著的走法；给玩家一个「打赢」的支线结果是游戏的让步，胜后文案与图鉴都注改编，六级妖丹本章不入药（A73） |
| ③ 煞妖 | ch404：岛上混战，他圈住四五名筑基邪修、邪修化煞妖、被他与曲魂杀尽（B17） | 一场仗：四敌、第 2 回合变身蓄劲 | 原著他就是打了这四个人；煞妖「化为飞灰」可用（原文如此，E25） |
| ④ 妖尸 | ch408：妖尸不怕法宝、伤口自愈、空中瞬移；他放一百多只青色巨猿傀儡与两头血玉蜘蛛，以蛛网与紫火困住后遁走；入地一月（A59–60） | 一场「困而不杀」的仗：`killable_by` 空、破绽水火、友军写死 | 原著他没有杀它，胜利写「困住」（E25）；友军 ×2 代表「一百多只」（B24）；与第 10 章 ④ 执法队（化身独战）不同，这是全作第一次韩立在场、四友军 |
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
| n26 两年辅料 | | | | `material_jinleizhu` 1（第一根）、`material_lianjian_fuliao` 1（**必给**） |
| n28 回访结算 ×M | ＋15 / 满 30 日（一次封顶 3600） | | | |
| n29 六根 / 收店 | | ＋20 | | `material_jinleizhu` 每轮 ＋1 |
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
| n12 诱妖 ×N | 草 10 / 株（两处杂货买） | | | 绿液 −1 滴 / 次 |
| Z1 交付 | | | 五级 −3 | |
| n14 收手 | | | | `story_kuilei_qun` −1 |
| n16 闭关 | | | | 降尘丹 −5、雪灵水 −2（有则扣，`ch11.dan_fu` 记位） |
| n18 三级傀儡 | | **−全部** | | 一贫如洗一句（A49） |
| n20 交易 | | | | `story_yaoshou_cailiao` −1 |
| n26 辅料 | | **−全部** | 五级 **−全部** | `material_tianleizhu` −1 |
| n27 盘店 | −50（有几扣几） | | | |
| n30 炼剑 | −20（配方 `inputs`） | | | `material_jinleizhu` −6、`material_lianjian_fuliao` −1 |
| Z2 | | | | 定颜丹 −1 |

**账要对得上的几条**（`Ch11LedgerTests` 直接读 `scripts/ch11/*.lua`、`data/battles/b11_*.json`、`data/recipes`）：

1. **中阶灵石 `take` 恰 8 处**（`zhihuan.lua` 8、`zufu.lua` 27、`ziliao.lua` 1、`xingkong.lua` 2 与 7 两处、`ningcui.lua` 1、`zhaolan.lua` 全部、`peizhu.lua` 全部）——**每一处都是「有几块扣几块」或另有退路，没有一处因为钱不够而 `return`**（第 6 章 HIGH-3 的教训；E13、E14）。**`give` 恰 4 处**（`liandan.lua` 30、`chaguan.lua` 40、`peizhu.lua` 收店 20、Z1 脚本 10）。
2. **低阶灵石 `take` 恰 4 处**（`zhihuan.lua` 3、`luer.lua` 5、`ziliao.lua` 100、`kaidian.lua` 50）；**`give` 恰 1 处**（`jiesuan.lua`，15 × 满 30 日数，封顶 3600）。
3. **五级妖丹**：`give` 恰 1 处（`huicheng.lua` 8）；`take` 恰 2 处（`peizhu.lua` 全部、Z1 脚本 3）；此外只由 ① 的 `drops` 来。**章末 0**（n26 扣光）。**六级妖丹**：`give`/`take` 0 处，只由 ② 的 `drops` 来，章末 0–1——第 12 章若要门闸，别依赖它（O.1）。
4. **降尘丹、雪灵水**：各 `take` 恰 1 处（`biguan.lua` 5 / 2）、`give` 0 处，**章末都是 0**；**天火液、血凝五行丹 0 处**（起点就是 0，不提）。**定颜丹** `take` 恰 1 处（`linju_z2.lua` 1）。
5. **竹**：`material_tianleizhu` `give` 1（`zhanglao.lua`）/ `take` 1（`peizhu.lua`）；`material_jinleizhu` `give` 2 处（`peizhu.lua` n26 分支 1、n29 分支每轮 1）、累计 6 根进配方——**n26 的第一根是否算 `ch11.zhu_gen = 1`**（F 表 n29 写「整数 1–6」、J 表写「0–6」），协调者定一个，否则会多炼一根；`material_lianjian_fuliao` `give` 1 处（**必给**，不看 n26 扣到多少）。配方 `recipe_qingzhu_fengyun_jian` `inputs` 恰 金雷竹 6 ＋ 辅料 1 ＋ `material_lingshi` 20、`productCount 1`、`difficulty 0`、`requiredProficiency 0`、`requireFlag "ch11.liugen"`（B14）。**配方要 20 低阶灵石**：n27 盘店之后玩家至少还有回访结算的 3600 上限可拿，不设死局，但 n30 面板前的 `trigger_lianjian` 旁白里不许扣钱。
6. **`bottle.spend` 恰 4 处**（`nichang.lua`、`youyao.lua`、`peizhu.lua` 两分支）；每一处都看返回值，不成则「等几日」`advance_days(5)`。
7. **story 物品**：`give` 9 处（n04、n06、n07、n10、n14 ×2、n18、n25、n31）、`take` 2 处（n14 `story_kuilei_qun`、n20 `story_yaoshou_cailiao`）；`story_shijin_chong`、`story_xueyu_zhizhu`、`story_lvhuang_jian`、`story_hunyuan_bo` 0 处（不扣不动）。
8. **四张编成 `rewards.spirit_stones` 都是 0**；① `drops` 五级妖丹 1 颗 rate 100、`cultivation > 0`；② `drops` 六级妖丹 1 颗 rate 100、`cultivation 0`；③④ `drops` 空、`cultivation 0`。
9. **`realm.advance` 恰 1 处**（`biguan.lua`）、`realm.cap` 0 处、`realm.demote` 0 处（C.1）；`party.add/remove` 0 处（E18）。
10. **两侧同账**：通关两侧走同一组选项（原著那一支：n02 红环、n03 雇、n07 付中阶、n09 拼阵、n10 买图、① 打 4 次、② 打一次、n14 收手、**两侧都做 Z1 与 Z2**、不卖丹、草只买够用的）。中阶两侧章末**都是 20**（n18、n26 两次扣光把起点差 1 抹平，之后只剩 n20 的 40 与 n29 的 20——`take` 全部之后再 `give`）。低阶：起点 254 / 234 − 5 − 50 − 草（10 × N）− 20（配方）＋ 结算（≤ 3600）＝ 两侧差 **20** = 起点差；卖丹、战斗用药、炼丹成败不能抹账。
11. **终局账 spec，未实测**：章末中阶 ≈ 20、低阶 ≈ 数百到四千（取决于结算回访次数与卖丹）、五级妖丹 0、六级 0–1、七十二剑 1、降尘丹 / 雪灵水 0、定颜丹 2 / 3——第 12 章起点「不富」与原著一致（N.1）；第 12 章仍只能从 `ch11-end` fixture 读，不把这些 spec 数写作正式终点。
12. **商店**：`ch11_fangshi_zahuo` 1.0 / 0.4、`ch11_ningcui_zahuo` 1.2 / 0.3、`ch11_ningcui_shougou` 1.0 / **0.8**（`entries` 空、只收 `tradeable` 的——五级妖丹 760、六级 4800）、`ch11_qingzhu_xiaoxuan` 1.0 / 0.6（买入页空）；霓裳草价 10（坊市 stock 10、凝翠岛 stock 5，各 5 日补货）；所有商店 note 写「初版经济待正式起点验账」（L.4）。**本章不卖六级妖丹、不卖丹药**（B8）。

**一趟出海的账**（first 侧 117 中阶起算，按 N.1 的次序；意图玩家出海一次、在红瑚岛把 ① 打 3–5 次、② 一次、再回城）：

| 步 | 支 | 收 | 累计中阶 |
| --- | --- | --- | --- |
| n02 红环 ＋ n04 租金 ＋ n07 资料 | 8 ＋ 27 ＋ 1 | | 81 |
| n09 首次拼阵出海 ＋ n10 海图 | 2 ＋ 1 | | 78 |
| ① ×4 ＋ ②（草 5 株 = 50 低阶） | 低阶 50 | 五级妖丹 4、六级 0–1 | 78 |
| 收购行卖 5 颗五级（可选） | | 低阶 ＋3,800（≈ 38 中阶当量） | 78（＋38 当量） |
| 回城 ＋ 再出海 ＋ 再回城（各 7） | 7 × 2 | | 64 |
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
| n29 六根（三年一根）＋ 补足到开店起 7,200 日 | 1,080 × 每轮 ＋ 补足（至少 360） | ≈ 33,195 |
| n30 布阵吉日 ＋ 两月出关 | 120 ＋ 60 | ≈ 33,375 |

全章约 **33,400 日**，其中 n16 一处就占 21,600；折合年数与韩立年龄由协调者按 `Calendar.h` 的年长与大纲第 185–223 行的年龄表核（本图不抄年龄）。游玩时长不受影响（E5）。

**对照：原著这一段的量级**。五级妖丹一颗近千灵石、六级够一位筑基修士一生修炼（A7）；五年数百颗五级妖丹最少值数十万灵石（A40）——玩家实得 5–10 颗，「数百颗」只在旁白；三级傀儡的辅料让他「一贫如洗」（A49）、七十二剑的辅料让他「倾家荡产」、卖掉仅剩的妖丹（A64）——本章两次 `take` 全部就是这两句；之后各有一笔必收（n20 的 40、n29 的 20），玩家永远有钱买草、付传送。章末他有一件七十二口的本命法宝、一块妙音门腰牌、半幅古修士地图，灵石所剩无几——第 12 章起点「不富」与原著一致。

