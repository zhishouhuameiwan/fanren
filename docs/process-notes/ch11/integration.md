# 第 11 章集成记录（协调者，构建槽 integ11，2026-10-12）

规范：`fanren/docs/ch11-design.md`（施工图）与 `fanren/docs/interfaces-p3-ch11.md`（契约）；矛盾处以契约为准。未碰 git。

## 契约 8.2 协调者项

| 项 | 文件 | 做了什么 |
| --- | --- | --- |
| C2 规则 25 | `tools/validate.py` | `CHAPTER_MEANS[11]` 八项（拳 hand；火 `magic_huodan_shu` ch04；土 `magic_liusha_shu` ch06；水 `magic_bingdong_shu` ch06；金 `magic_ji_jinfu` ch07；木 `magic_qingyuan_jianmang` ch08；暗器 `weapon_wuming_sixian` ch07；剑 `weapon_lieyang_jian` ch06）；`BATTLE_EXTRA_MEANS["b11_tiandu_shi"]` 两条友军 `role_magic`（水 `magic_zhizhu_tuwang` / `xueyu_zhizhu_ally`，火 `magic_kuilei_zihuo` / `kuilei_yuan`）；`MEANS_LAST_CHAPTER = 11` |
| C2 偏差 | 同上 | **没登 `qu_hun_shadan` 的 剑 / 火 / 火 / 金**：`role_means_available` 只认编成里 `faction == "ally"` 的单位，四场 `hero_absent false`、曲魂走 `BattleScene::addPartyUnits`（队员）不在编成里，照 `b10_zhifadui` 写法登上去来源检查 16 条红。与第 10 章 `b10_gu_zhanglao` / `b10_gujia_bidou` 在队不登同口径。登之前核过：曲魂 `weapons ["剑"]`、三门法术五行与 `needRealm`（CoreEarly）都能施展；两友军法术 `needRealm` FoundationLate / FoundationMid ≤ 角色境界，来源检查通过 |
| 探针 | `tools/validate_selftest.py` | 第 1801–1828 行：`siji_haishou` + `b11_ningcuidao_youyao` → `wu_chou` + `b12_neidian_duoding`，`probe["chapter"] == 12`、措辞「第 12 章范围探针实际引用乌丑」、移入第 11 章必报；第 2502–2527 行同一探针的第二组（`future_id` / `scoped`）同样挪：`scoped(12)` 不报、`scoped(11)` 必报（原指 `b11`，末章 11 后它在范围内必红）；第 2464–2465 行断言「末章 11」 |
| 旧占位 | `data/text/battles.json` | `ch11.battle.ningcuidao_youyao.intro` 已于 22:11 删除（本次核：文件里 0 处 `ch11`） |
| GTEST_SKIP | `tests/Ch11EngineTests.cpp` | 6 处守卫改成 `ASSERT_*`（配方 guaranteed、妖尸 role/battle 文件、三件物品、四张店、`ch11_dongfu.tmj`）；头注释同步 |
| handoff | `docs/handoff.md` 第 4 节 | 拍卖那一行：本章只做门面 + 路径行动，`bidStep` 仍只有规则层，面板留给第 12 / 13 章 |

## 合缝修补

| # | 文件 | 改了什么 | 依据 |
| --- | --- | --- | --- |
| 1 | `data/roles/jiedan_linju_jia.json`（新）、`data/roles/jiedan_linju_yi.json`（新）、`data/roles/jiedan_linju.json`（删） | 规则 30 报 `ch11_dongfu` 两个 npc 同 role 同旗标；拆成两个 role，名牌「左邻结丹修士」/「右舍结丹修士」（规则 30 也不许不同 role 同名），数值照原件 | 施工图 16.1 第 4 条（O-4：拆 `jiedan_linju_jia` / `_yi`）；deviations-A A6 |
| 2 | `data/visual/looks.json` `roles` | `jiedan_linju` 一行换成 `jiedan_linju_jia` / `jiedan_linju_yi` 两行，共用外观 `ch11_jiedan_linju` | 同上；looks 已有多 role 共用一个 look 的先例（`qu_hun` 等） |
| 3 | `tools/mapgen/genmaps_ch11.py` 第 221–222 行 | 两个 `person(...)` 的 role 改 `_jia` / `_yi`；`python tools/mapgen/genmaps_ch11.py` 重生成七张图（只有 `ch11_dongfu` 内容变） | 同上 |
| 4 | `scripts/ch11/linju_z2.lua` | 四句 `talk("jiedan_linju", …)` → `talk("jiedan_linju_yi", …)`（`_b` 挂的脚本） | 同上 |
| 5 | `assets/art/sprites/index.json` 等 | `python tools/artgen/artgen.py` 全量重烘焙（618 文件；只有 index.json 的 role→look 映射多两条，图不变） | looks.json 改了 |
| 6 | `tools/mapgen/genmaps_ch11.py` `make_honghu` | `trigger_huicheng` (2,13) 是 interact 触发，`event()` 在那一格画 building 8 并挡路，而那一格是海面 gid 3；`MapArtTests.TheShippedMapsUseGid3…` 钉「水面不画 building」。在 event 前补一行 `floor(g, 2, 13, 2, 13, 2)`（礁道），再重生成 | 引擎判据（`MapArtTests.cpp` 第 129–159 行）；deviations-A A1 把触发挪到了海面礁石 |
| 7 | `tools/mapgen/genmaps_ch11.py` `make_ningcui_jie` | 街北第四间铺面 `shop_block(25,5,28,8)` 的东墙列 x=28 在岛（到 x=27）之外、压在海面上，同一条用例红；补 `floor(g, 28, 3, 28, 9)` 一列岸（不扩到 y≥12：`data/visual/maps.json` 的小船贴花在 (28,13) 2×2，压到可走格 artgen 会拒）。重生成两张图后 `artgen --map ch11_honghu` / `--map ch11_ningcui_jie` 重烘焙 | 同上 |

| 8 | `tools/mapgen/genmaps_ch11.py` `make_fangshi`、`data/pathactions/ch11.json` | `NpcPresenceShipped` 不许具名 role 一人两地：去掉坊市街上的 `npc_he_xianshi`（店内 `npc_he_xianshi_2` 留），打探 `hexianshi_fanren_dating` 的 `map` → `ch11_xiaoxuan`、`npc` → `npc_he_xianshi_2`（`when ch11.kaidian >= 1` 本就是它的 visible_flag，规则 26 过） | 施工图第 3 节「待协调者核的三处」② 原话 |
| 9 | `tools/mapgen/genmaps_ch11.py` `make_fangshi` | 同一用例：`fan_furen` 雅间对象 hidden `ch11.zhanglao` → `ch11.zhuzhan`（n21–n23 与礁岛 `npc_fan_furen_2` 两地同在）；n25 她只作说话人。第三个对象（yaoshi → zhanglao）过不了规则 30，没加 | 施工图第 3 节同一段的「挪旗标」授权；偏离 G 表「一个对象顶 n20、n25 两场」，记在施工图 18.4 第 7 条 |
| 10 | `data/text/ch11_path.json` | 六条打探（chuwu_dai / haoyuantian_waihai / zahuo_haiyu / zaohong_jiguan / linju_dingyan / jinqing_luanxing）35–39 汉字 → 各补一短句到 40–47 字，意思不变 | `PathActionData.EveryInquiryRunsFortyToOneHundredFiftyCharacters`（派工原话 40–150 字） |

## 门禁

- `python tools/validate.py` → VALIDATE_OK（数据条目 710、文案 4994、地图对象 502；错 0 警 0）
- `python tools/mapgen/genmaps.py --check` → MAPGEN_IN_SYNC（65 张图）
- `python tools/artgen/artgen.py --check` → ARTGEN_IN_SYNC（618 文件）
- `python tools/validate_selftest.py` → SELFTEST_OK（363 条探针、漏抓 0；单独跑，快照早于合缝 6/7 的两张图，终证构建再跑一遍）

## 终证构建

`build_logged.bat integ11` → `fanren/build-integ11.log`（mtime 2026-10-13 00:04:09）：四门全绿（VALIDATE_OK / MAPGEN_IN_SYNC 65 图 / SELFTEST_OK / ARTGEN_IN_SYNC 618 文件）、`warning C` 0、「99% tests passed, 13 tests failed out of 1808」、EXITCODE=1。第一轮（23:34）15 红，合缝 6–10 之后 `MapArt` 与 `NpcPresenceShipped` 与 `PathActionData.EveryInquiry…` 回绿；剩 13 条全是下面待办里测试路的表 / 断言与同图 portal 那一条，无一涉及 ch11 数据、脚本或 `Ch11EngineTests`（28/28 绿）。

## 待办（白名单外，没动手）

- **测试路 T1 / T2**：`tests/fixtures/ch10-end-{first,second}.sav` 按 `zhifadui.lua` 末尾 teleport 重生成（`Ch10Walkthrough` ×2、`Ch10Acceptance.BothActualInheritedPurses…`、`Ch10Slice.TheOtherLateChoice…`）；`Ch10AcceptanceTests` 章节卡断言 `ToBeContinued` → `Opening(11)`；`ChapterCards` ×2 / `ChapterTableData` 钉末章 10 的用例改 11；`tests/ObjectiveTests.cpp` `kScriptTransfers` 登七条（含 `ch10.done` → `ch11_fangshi`）；`tests/NpcPresenceTests.cpp` `kOffChainFlags` 登八条、`kLastChapter` 改 11；`tests/PathActionTests.cpp` `realmCapOfChapter()` 加 `{11, Realm::CoreEarly}`（`PathActionData.EveryGateIsWithinWhatHanLiCanReachInThatChapter`，契约 8.3）；`tests/Ch07BattleDataTests.cpp` 第 318 行 `MEANS_LAST_CHAPTER` 的放行清单（7/8/9/10）加 11（`Ch07BattleData.RuleTwentyFiveKnowsTheMeansOfChapterSeven`——引擎路跑时表还是 10 所以没红）。
- **地图路 / 测试路**：`ch11_fangshi` 茶馆雅间同图 portal 对（`portal_chaguan` / `portal_chaguan_chu`）过不了 `MapLinkWalk.HoldingTheKeyThroughEveryPortalNeverBouncesBack`——用例进门后按住方向再走一步、当前图仍是出发图即判弹回，同图传送必红；要么雅间单独成图，要么用例对同图门改判据。
- **美术路**：`tools/artgen/sprites.py` `BATTLE_OVERRIDES` 加 `"b11_yinsha_shayao": {"yinsha_shayao": "yinsha_shayao_bianshen"}`（deviations-A A7）。
- **引擎 / 门禁**：`validate.py` `role_means_available` 认队员（曲魂在队的四场才登得上 BATTLE_EXTRA_MEANS）；`api.lua` `field.unlock` 返回值（deviations-C 1）；`ShopScene` 只读 `mainItems()`（契约 4.2-4 实现记录）。
- **文档回写（C7）**：大纲 / 人物总表的第 11 章注记已在工作树里（git status M，10 行 / 2 行，本次未核内容）；人物总表里没有 `jiedan_linju` 条目，拆 role 不必回写。
