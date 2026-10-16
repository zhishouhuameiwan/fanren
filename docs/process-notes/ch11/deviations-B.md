# 第 11 章 内容路 B（数据）施工偏差

2026-10-12。规范：`fanren/docs/ch11-design.md`（施工图）与 `fanren/docs/interfaces-p3-ch11.md`（契约）。下列每一条都是「文档这么写、落地改成了那样」，没有列的就是照抄。

## 1. 路径行动 id 改形（门禁 vs 施工图 16.3）

`tools/validate.py` 的 `PATH_ID_RE` 要求 id 以 `_dating[序号]` 结尾（`<npc 短名>_dating[序号]`），施工图 16.3 的 `<npc>_dating_<话题>` 全部过不了门禁。落地改成 `<npc>_<话题>_dating`，话题词一个不丢：

| 施工图 | 落地 id / done_flag（`ch11.path.<id>`）/ text_key（`ch11.path.<id>.text`） |
| --- | --- |
| baiyi_dating_guiju | baiyi_guiju_dating |
| luer_dating_shengshan | luer_shengshan_dating |
| fengle_dating_mulu | fengle_mulu_dating |
| chuwu_dating_dai | chuwu_dai_dating |
| baimen_dating_zhaixing | baimen_zhaixing_dating |
| haoyuantian_dating_waihai | haoyuantian_waihai_dating |
| haoyuantian_dating_qianmu | haoyuantian_qianmu_dating |
| fanxiaozi_dating_jiage | fanxiaozi_jiage_dating |
| zahuo_dating_haiyu | zahuo_haiyu_dating |
| zaohong_dating_jiguan | zaohong_jiguan_dating |
| linju_dating_miaoyin | linju_miaoyin_dating |
| linju_dating_dingyan | linju_dingyan_dating |
| hexianshi_dating_fanren | hexianshi_fanren_dating |
| jinqing_dating_luanxing | jinqing_luanxing_dating |

协调者登记 `data/flags.json` 时按右列 14 个 `ch11.path.*`；施工图 12.2 / 16.3 引用左列名字的地方同步改。

## 2. 路径行动 when 多写了在场旗标（门禁规则 26）

门禁要求 `when` 里写上那个 NPC 的 `visible_flag`（或它能直接推出的旗标；目标链先后它不推）。四条各加一条谓词，窗口实际不变：
- `baimen_zhaixing_dating`：加 `ch11.kuilei >= 1`（掌柜 `ch11.kuilei` 出场）
- `fanxiaozi_jiage_dating`、`zahuo_haiyu_dating`：加 `ch11.chuansong >= 1`
- `linju_miaoyin_dating`：加 `ch11.zhaolan >= 1`

## 3. 郝远天两条打探的时段（门禁规则 26「同一 NPC 同为 inquire 时段须证明不重叠」）

施工图两条都是 `when ch11.ningcui`、`until ch11.huicheng`。落地：`haoyuantian_waihai_dating` 的 `until` 另含自己的 `done_flag`；`haoyuantian_qianmu_dating` 的 `when` 另含 `ch11.path.haoyuantian_waihai_dating >= 1`——先问外海规矩，再问千目妖。reveal 不变。

## 4. 紫灵真容（`zi_ling_zhenrong`）的数值

第 5 节表：境界「筑基后期」、「数值同 `zi_ling` 口径」；`zi_ling.json` 本身是结丹初期 700 / 61 / 48。落地按**筑基后期**取 `Realm.h` 锚点（480 / 44 / 30），名牌「紫灵」；不动 `zi_ling.json`。

## 5. 商店多加的条目

施工图只定了霓裳草。坊市杂货另列 `material_fuzhi`、`material_dansha`（照 `ch10_tiandujie_pu` 的货），凝翠岛杂货另列 `material_fuzhi`；不卖丹药（B8）、不卖妖丹。收购行与青竹小轩 `entries: []`。

## 6. 文案里给脚本路备的 key（施工图没点名）

`data/text/ch11_battle.json` 除四条 `.intro` 与四条 `.charge`（`ch11.battle.{qianmu,jiguan,shayao,tiandu}.charge`，role 的 `charge.text_key` 引着）外，另写了胜负文案：
`ch11.battle.ningcuidao_youyao.{win,escape,lose}`、`ch11.battle.honghu_jiguanjiao.{win,away}`（逃或败同一句）、`ch11.battle.yinsha_shayao.{win,lose}`、`ch11.battle.tiandu_shi.{win,lose}`（胜写「困住」）。脚本路要用就引，不用也不碍门禁。
商店名 key `ch11.shop.name.{fangshi_zahuo,ningcui_zahuo,ningcui_shougou,qingzhu_xiaoxuan}` 放在 `ch11_quests.json`（照第 10 章）；法术描述 `magic.desc.magic_{zhizhu_tuwang,kuilei_zihuo}` 放在 `ch11_items.json`（照第 10 章）。

## 7. 初版战斗数值（归平衡路，这里只记落地值）

| role | 境界 | 气血 / 攻 / 防 | 架势 | 破绽 | 行动 | 蓄势 |
| --- | --- | --- | --- | --- | --- | --- |
| qianmu_yao | CoreEarly | 760 / 64 / 44 | 6 | 土、剑 | 1 | every 3 ×1.8 全体 |
| siji_haishou（只改 weaknesses） | FoundationMid | 428 / 52 / 36（不动） | 3 | 土、暗器 | 1 | — |
| jiguan_jiao | CoreMid | 1000 / 80 / 60 | 8 | 金 | 2 | every 2 ×2.2 全体 |
| yinsha_shayao | FoundationLate | 470 / 46 / 32 | 3 | 剑、木 | 1 | every 2 ×1.7 全体 |
| tiandu_shi | CoreEarly | 1600 / 70 / 50 | 4 | 水、火（killable_by 同） | 2 | every 2 ×1.5 单体 |
| xueyu_zhizhu_ally（友军） | FoundationLate | 460 / 42 / 30 | — | — | 1 | 法术 magic_zhizhu_tuwang（水 34） |
| kuilei_yuan（友军） | FoundationMid | 380 / 38 / 30 | — | — | 1 | 法术 magic_kuilei_zihuo（火 36） |

① `rewards.cultivation 120`（施工图只说 > 0，数归平衡路）。

## 8. 白名单外、留给别路的

- `data/text/battles.json` 第 6 行旧占位 `ch11.battle.ningcuidao_youyao.intro` 与本路 `ch11_battle.json` 同 key，门禁报「文案 key 重复」——协调者删（契约 9.3）。
- `data/flags.json` 登 14 个 `ch11.path.*`（第 1 节右列）——协调者。
- `tools/validate.py` 的 `CHAPTER_MEANS[11]` / `BATTLE_EXTRA_MEANS`（四张各登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金）/ `MEANS_LAST_CHAPTER = 11`，`validate_selftest.py` 探针挪到 `wu_chou` + `b12_neidian_duoding`——协调者（E19、O-2）。
- `maps/ch11_dongfu.tmj` 规则 30：`npc_jiedan_linju_a` / `_b` 同 role 同旗标（O-4）——关卡路或协调者；role 侧按施工图只建一个 `jiedan_linju`。
- `data/visual/looks.json`、`sprites_enemies.py`、`battles.json` 背景——美术路。
- 引擎 E1（`Recipe::guaranteed`、`requireFlag`）落地前，`RecipeLoader` 读 `qingzhu_fengyun_jian.json` 是否拒收未知字段，本路没有编译核（没起构建）。
