# 第 11 章内容路 A（地图与美术）施工偏差

集成时并进 `docs/ch11-design.md` 第 18 节。两份文档矛盾处以契约 `docs/interfaces-p3-ch11.md` 为准。

## A1 同格叠放的三处挂点改成相邻格（施工图 3.1 的 O-7 已许）

引擎 `WorldScene::tryStep` / `interact` 对触发器都走 `map->objectAt(cell, "trigger")`，同一格只取对象表里第一个，
不分 `mode`；`validate.py` 规则 16 把同格第二个 `enter` 触发报成「踏不响」。所以：

| 施工图 | 落地 |
| --- | --- |
| 红瑚岛 `trigger_huicheng` 与 `trigger_buzhen` 同在登岸格 (4,14) | `trigger_huicheng` 挪到登岸格旁的海面礁石 (2,13)，`interact`、面朝海（收手回城），`guard ch11.buzhen` 不变 |
| 礁岛 n22/n23/n24 同格三连 | 登岛小径 (13,16..19) 连续三格：spawn `haishang` (13,19) → `trigger_guiju` (13,18) → `trigger_wuchou` (13,17) → `trigger_daoge` (13,16)；小径 1 格宽，绕不过 |
| 炼器台 `trigger_lianjian` 与 `facility_lianqi_39` 同格 | 炼器台 (8,18) 只从 (8,17) 够得着，`trigger_lianjian`（enter）就在 (8,17)：到台前必先踩它 |

## A2 `deny_text_key` 按规则 20 用目标图命名

契约第 7 节写「`deny_text_key` 按规则 20 成对」，规则 20（`validate.py` `expected_deny_key`）要求
`<目标图章号>.block.<目标图短名>`。施工图里的 `ch11.fangshi.shangshan_deny` / `ch11.dongfu.xingkong_deny` 不用：

| 门 | require_flag | deny_text_key（落地） | 施工图写法 |
| --- | --- | --- | --- |
| 坊市北口 `portal_shangshan` | `ch11.zufu` | `ch11.block.dongfu` | `ch11.fangshi.shangshan_deny` |
| 洞府北口 `portal_xingkong` | `ch11.nichang` | `ch11.block.xingkong` | `ch11.dongfu.xingkong_deny` |
| 青竹小轩门 `portal_xiaoxuan` | `ch11.kaidian` | `ch11.block.xiaoxuan` | 「门关着」 |
| 茶馆门 `portal_chaguan` | `ch11.lianer` | `ch11.block.fangshi` | 「门关着」 |
| 凝翠岛东口 `portal_chuhai` | `ch11.ningcui` | `ch11.block.honghu` | 「门关着」 |

五条文案写在 `data/text/ch11_maps.json`（与七条地名 key 同文件）。`ch11.block.xingkong` 不含「星空殿」「传送」
（施工图 981 行词表规则）。若内容路 C 另写了 `ch11.fangshi.shangshan_deny` 之类的 key，那是没人引用的闲 key，协调者删。

## A3 茶馆雅间的门：同图一对 portal

雅间在坊市图里（三区一张图），门闸只能用 portal 的 `require_flag`。规则 13 要求雅间里四个 NPC 从城门走得到，
所以进门那一格 (42,7) `portal_chaguan`（→ 同图 spawn `yajian` (44,8) 朝右）在地形上与雅间连着：
BFS 走得通、玩家踩上去却被 `require_flag ch11.lianer` 拦住。出门是雅间内侧 (43,7) `portal_chaguan_chu`
→ 同图 spawn `chaguan_men` (40,10) 朝左。规则 29 两头都核过：两个落点按住进门方向都撞墙，不会弹回。
坊市因此多两个 spawn（`yajian`、`chaguan_men`），施工图 spawn 表只列了 `chengmen / shanjiao / xiaoxuan_men`。

## A4 `trigger_chujian` 加 `guard_flag ch11.liugen`

施工图 3.1 n30b 的 guard 列写的是 `item.count("weapon_qingzhu_fengyun_jian") >= 1`（脚本判）。剑只能在
`facility_lianqi_39`（`require_flag ch11.liugen`）炼出，所以加 `guard_flag ch11.liugen` 不改语义，
只免得 `ch11.liugen` 之前每次踏过密室门口 (10,15) 都空跑一次脚本。

## A5 李氏杂货与青竹小轩是同一栋铺面

街西 (17..23, 23..26) 一栋：门 (20,26) 是 `portal_xiaoxuan`（`require_flag ch11.kaidian`），
`npc_li_dianzhu` 站在门旁 (21,27)（`ch11.jinlei → ch11.kaidian`）——盘店之前店主在门口、门不开；盘下之后店主退场、门开。

## A6 规则 30：`npc_jiedan_linju_a` / `_b` 同图同 role（O-4，施工图已列）

`validate.py` 唯一一条落在本路文件上的错：`maps/ch11_dongfu.tmj 规则 30：2 个 npc 用了同一个 role jiedan_linju`。
两人要同时在场（`_a` 挂路径行动 `linju_dating_miaoyin`、`_b` 挂 Z2 `linju_z2.lua`），写不成互补旗标。
解法二选一，都在本路白名单外：① 协调者把 `jiedan_linju` 登进 `validate.py` 的 `CROWD_ROLES`；
② 内容路 B 另建一个 role（如 `jiedan_linju_b`，名牌不同），`_b` 改用它、本路改 `genmaps_ch11.py` 一行。

## A7 煞妖变身态要接 `BATTLE_OVERRIDES`

`yinsha_shayao` 按施工图第 5 节给了人形外观（`looks.json` `ch11_yinsha_shayao`，arch_demonic_m）；变身态画在
`sprites_enemies.py`（`enemies/yinsha_shayao_bianshen.png`）。运行时取图先查 `roles`，所以战斗③现在画人形；
要在战斗里换成变身态，`tools/artgen/sprites.py` 的 `BATTLE_OVERRIDES` 要加
`"b11_yinsha_shayao": {"yinsha_shayao": "yinsha_shayao_bianshen"}`（白名单外，交协调者）。
另：非人形敌人图一律面朝右、不翻转（`docs/art-sprites.md` §4）；友军 `xueyu_zhizhu_ally` 站在右侧会背对敌人，
引擎若不给友军兽图翻转，这是引擎路的一处小事。

## A8 其它

- `region` 一律 `luanxinghai_zhongbu`；洞府、星空殿、青竹小轩 `outdoor false`，其余 true。
- 七张图的 BGM 全是旧曲：`catalog.MAP_BGM` 元组本就含这五个 id，只加了七行登记注释；`docs/audio.md` 曲目表「用在哪里」列补了七张图。
- `looks.json` 新加 31 条外观（全部 `base` 原型 + 配色，不编外貌）与 32 条 `roles`（含 `kuilei_yuan → arch_puppet`）；
  `siji_haishou` 本就没有外观（运行时退回旧画法），没动。
- `sprite_preview.py --coverage`：缺图 0（交付前跑的）。
