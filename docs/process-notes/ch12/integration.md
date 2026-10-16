# 第 12 章集成记录（协调者，2026-10-13，构建槽 integ12）

## 契约 5.2 协调者项

- `tools/validate.py`：`CHAPTER_MEANS[12]`（第 11 章八类 ＋ 金 `magic_shijin_chongqun` ch12）、`BATTLE_EXTRA_MEANS` 四场照契约、`MEANS_LAST_CHAPTER = 12`。登之前逐条用 `role_means_available` 核过：五条 role 来路全为 True（`zi_ling_xutian.weapons` 含剑；魔火 8/12/FoundationEarly 对黑袍元瑶 CoreEarly 470、玄骨 CoreLate 860；青火雷 8/24/CoreEarly 对结丹元瑶 470；冰箭术 4/7 对乌丑 CoreMid 600）；`talisman_bixie_shenlei` 的 castMagic 是木（element 2）、`give` 在 guiwang.lua / xuangu.lua；`magic.learn("magic_shijin_chongqun")` 在 guairen.lua。
- `tools/validate_selftest.py`：两处乌丑探针挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`（第 13 章应静、移入第 12 章必报），断言末章 11 → 12。
- `data/text/battles.json`：删 `ch12.battle.xutian_hudian_kuilei.intro`（`neidian_duoding.intro` 那条开工前已不在）。
- 删占位：`data/roles/xutian_hudian_kuilei.json`、`data/roles/duoding_xiushi.json`、`data/battles/b12_xutian_hudian_kuilei.json`（契约 5.2 / 6.5 归协调者；派工白名单没列，按契约做，报告里写明）。
- `data/flags.json` 的 13 条 `ch12.path.*`：集成开工时已在（第 545–557 行，写法同第 11 章），不动。
- `tests/Ch12EngineTests.cpp`：11 处 `GTEST_SKIP()` 改 `FAIL()`（数据已齐，缺了就红，不再跳）。
- O-13 采药灵草：认定内容路 C 暂用的 `herb_huangjing_zhi` ×2、年份 300，不改。
- `docs/handoff.md` 第 4 节拍卖那一行：第 12 章 0 店，面板留第 13 章。
- KeyPrompts：本章 0 处 `{key.` / `{pad.`，不触发。

## 合缝修补

无。四门与全量构建里没有一处红灯出自本章数据 / 脚本的引用对不上，未改四路交付的任何文件。

## 终证

- validate：VALIDATE_OK（776 项、5549 条文案、556 个旗标，0 错 0 警告）；genmaps --check：MAPGEN_IN_SYNC（75 张）；validate_selftest：363 条、未如期抓住 0、SELFTEST_OK；artgen --check：ARTGEN_IN_SYNC（678 个文件）。
- `build-integ12.log`（07:08）：EXITCODE=1，0 警告；`99% tests passed, 13 tests failed out of 1885`；Ch12Engine 23 例全过、0 跳过。
- 13 红全属测试路登记：Ch07BattleData ×1（MEANS_LAST_CHAPTER 放行表）、Ch11Walkthrough ×2 ＋ Ch11Acceptance ×1 ＋ Ch11TriggerMode ×2（zhuzhong 终点 / 末章）、ObjectiveRoute ×3（kScriptTransfers 9 条三元正是缺的 9 步）、PathActionData ×1（realmCapOfChapter 缺 12）、ChapterTableData ×1 ＋ ChapterCards ×2。
