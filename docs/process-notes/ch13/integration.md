# 第 13 章集成记录（协调者·集成者，构建槽 integ13，2026-10-14）

## 协调者项（契约 5.2）

1. `data/text/battles.json` 删旧占位 `ch13.battle.waixinghai_lieyao.intro`（原第 6 行）。删前核过：
   `data/battles/b13_waixinghai_lieyao.json` 的 `intro_key` 就是这个 key，`data/text/ch13_battle.json` 第 4 行有同名新文案。
   字节级删一行，其余 5 条逐字节不变，0 个 CR。
2. `tools/validate.py` 规则 25：
   - `CHAPTER_MEANS[13]` ＝ 第 12 章九条照抄（拳 / 火 huodan ch04 / 土 liusha ch06 / 水 bingdong ch06 / 金 ji_jinfu ch07 ＋ 金 shijin_chongqun ch12 /
     木 qingyuan_jianmang ch08 / 暗器 wuming_sixian ch07 / 剑 lieyang_jian ch06）；五行环 `magic_ji_wuxinghuan` 不登（注释写明理由）。
     核过 `scripts/ch13/` 没有收回这几样（无 magic.forget；take 的全是灵石 / 妖丹 / 剧情物）。
   - `BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"] = [("木","learn","magic_fengyun_heji","ch13")]`。核过：编成 3 个单位全是敌方（无友军，
     不需要 role_magic 行）；`magic_fengyun_heji` element 2（木）、power 64、needRealm CoreEarly、needMp 40、无 effect；
     `scripts/ch13/chonglian.lua` 第 14 行 `magic.learn("magic_fengyun_heji")`；② 的挂点 `trigger_youyao` guard_flag `ch13.buzhen`，
     目标链里 chonglian（第 19 步）在 buzhen（第 22 步）之前。
   - `MEANS_LAST_CHAPTER = 13`；`CHAPTER_START[13] = ("CoreEarly", 470)`（ch12-end 两侧 realm 31 / maxMp 470）。
3. `tools/validate_selftest.py` 探针挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`（全仓唯一一张含 feng_xi 的编成，chapter 14，敌方）：
   - `selftest_n_battle_break`：`outside_role` / `outside_battle`、先验「chapter == 14 且敌方含 feng_xi」、`probe["chapter"] = 13`、
     措辞「第 14 章」「同一风希移入第13章」、必报判据 `"feng_xi" in error`（原 `"wuji_haishou" in error`）。
   - `selftest_ch10_means`：断言 `MEANS_LAST_CHAPTER == 13`、用例名「末章13」；`future_id = "b14_fengxi_xiezhi"`、引用扫描与 scoped 改读
     `data/roles/feng_xi.json`、必报判据 `"feng_xi" in e`；`scoped(14)` 应静、`scoped(13)` 应报。
   - 阳性 / 阴性对照（`scratchpad/integ13/probe_control.py`，只在内存里改 `validate.MEANS_LAST_CHAPTER`，两段自检各在自己的临时副本里跑）：
     末章 13 → 69 条 0 失败；末章改回 12 → 69 条 3 失败（两处「必报」＋「末章13」断言）。
4. `data/flags.json` 登 `ch13.path.*` 13 面，id 照抄 `data/pathactions/ch13.json` 的 `entries`（每条 `done_flag` 核过等于 `ch13.path.<id>`），
   说明照第 12 章写法「只由路径行动置；第13章打探：<id>」，追加在 `ch13.z3` 之后；原 597 条逐字节不变（新文件以旧文件去掉末尾 `\n}\n` 为前缀），
   总数 597 → 610。
5. `tests/Ch13EngineTests.cpp` 5 处 `GTEST_SKIP() << "等内容路 <文件> 落地"` 改 `FAIL() << "缺 <文件>（内容路 X 已交付，第 13 章已集成）"`；
   文件头、两处分节注释同步成现状。契约 `docs/interfaces-p3-ch13.md` 实现记录三处（1.4 第 8 条、2.4 第 5 条、引擎路条目）补「集成时改 FAIL()」。
   单跑 `Ch13*`：27 / 27 绿、0 跳过。

## 合缝修补

（无）——validate 0 错 0 警告；全量测试 25 红逐条归因后全属测试路登记类（见下），没有一处是四路交付文件之间的引用对不上。
顺手核过、无需改的：`scripts/ch13/` 的 teleport 目标与契约 5.3 `kScriptTransfers` 七条一致（lidao (14,46)、qiyuan_hang (7,12) 与 A1 常量同）；
`lingquan.lua` 末行 `teleport("ch13_nanming")`、头注释已改现状；`data/text/*.json` 全仓 0 个重复 key；`ch13*.json` 0 处 `{key.` / `{pad.`
（KeyPrompts 白名单不动）。

## 全量测试红灯归因（第一轮，build-integ13 直接跑 fanren_tests.exe）

1948 绿 / 25 红 / 0 跳过，25 红全属测试路（契约 5.3）：
- `Ch07BattleData.RuleTwentyFiveKnowsTheMeansOfChapterSeven`：`MEANS_LAST_CHAPTER` 放行表到 12，要加 13。
- `Ch12Walkthrough` ×2、`Ch12Acceptance` ×2、`Ch12Balance` ×3、`Ch12Slice` ×7：`Node failed: 33 ch12.done`——驱动的 `settleAtSpring()`
  在 `lingquan.lua` teleport 到 `ch13_nanming` (6,29) 之后对不上；改通关终点 ＋ 重生成 `ch12-end-*`。
- `Ch12TriggerMode` ×2：「lingquan.lua does not end with ch12.done」——第 12 章「不 teleport」断言改「仅末行一处 teleport("ch13_nanming")」。
- `NpcPresenceShipped.EveryNamedRoleStandsOnAtMostOneMapAtEveryStepOfTheMainLine`：`kOffChainFlags` 缺 `ch13.shouchao` 等（J.2 七条）、`kLastChapter` 13。
- `ObjectiveRoute` ×3：`kScriptTransfers` 缺第 13 章七条（ch12.done → ch13_nanming 起）。
- `PathActionData.EveryGateIsWithinWhatHanLiCanReachInThatChapter`：`realmCapOfChapter()` 缺 `{13, CoreMid}`。
- `ChapterTableData.TheShippedTableIsWhatTheWorkOrderAsked`、`ChapterCards` ×2：`UiStyleTests` 章节表 / 卡钉在第 12 章。

## 四门与终证

- 单跑：`VALIDATE_OK`（错 0、警告 0、旗标 610）；`MAPGEN_IN_SYNC：79 张`；`SELFTEST_OK`（用例 413 条，未如期抓住 0 条，29 分 25 秒）；
  `ARTGEN_IN_SYNC：721 个`。
- 终证 `build-integ13.log`（mtime 2026-10-14 01:57:25，gb18030 读）：四门同上（selftest 413 / 0）；`ninja: no work to do`
  （同槽 `build-integ13-compile.log` 从零编 236 步，0 警告 0 错）；`99% tests passed, 25 tests failed out of 1973`；`EXITCODE=1`。
  25 红与上节第一轮逐条相同，全属测试路。

## 文档

- `docs/ch13-design.md` 第 18 节：节首一段（各路原稿出处 ＋ 两条共有项只记在 18.1）、18.2 内容路 A（A1 1–7、A2 8–12）、18.3 内容路 B（1–7，
  含四个敌方 role 的数表）、18.4 集成。18.1 未动。
- `docs/interfaces-p3-ch13.md` 实现记录三处补「集成时改 FAIL()」。

## 待办（不归本路）

- 测试路（契约 5.3 / 6.6）：上面 25 红对应的九处登记——ch12-end 两份重生成与 Ch12 通关终点、第 12 章章节卡断言、「lingquan.lua 不 teleport」断言、
  UiStyleTests 章节表 / 卡、kScriptTransfers 七条、kOffChainFlags 七条 ＋ kLastChapter 13、realmCapOfChapter {13, CoreMid}、
  Ch07 MEANS_LAST_CHAPTER 放行表加 13；另 ch13-end 两份夹具与本章通关 / 登记类用例。
- 协调者提交时：docs/handoff.md、tech-debt。
