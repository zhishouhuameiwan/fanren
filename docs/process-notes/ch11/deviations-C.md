# 第 11 章内容路 C（脚本与主线文案）偏差记录

2026-10-12。白名单：`scripts/ch11/**`（34 个）、`scripts/ch10/zhifadui.lua` 末尾一行、`data/text/ch11_{main,haiwai,biguan,tianxing,xiaoxuan}.json`。

## 与施工图 / 契约的偏差

1. **`field.unlock` 没看返回值**（施工图 3.2 n05「unlock 查返回值」）：`scripts/common/api.lua` 的 `field.unlock` 只 emit、不返回任何值（与第 10 章 `lifu.lua` 同一写法），无从核对；`anju.lua` 照契约第 7 节写了一句 `field.unlock("field_tianxing_39", 6)`。若要核，得改 api.lua（引擎项，不在本路白名单）。
2. **n16 `realm.advance` 失败的早退**：契约要求「占位一句 `return`、不置旗」。`biguan.lua` 在 `return` 前补了一句 `fade.in_()`——不补，失败时屏幕停在黑场。降尘丹 / 雪灵水在那之前已扣、`ch11.dan_fu` 已写；重进时按当时背包重算（多半是 0 → 「none」那一版旁白）。
3. **n29 先于 n27 的边界**：`trigger_peizhu` 的地图 guard 是 `ch11.zhanglao`，玩家可以不盘店先催满六根。此时 `ch11.kaidian_ri` 为 0，补足式 `max(360, 7200 - (today() - 0))` 退化成 360 日，不会崩，但 `peizhu.close`「店开了二十余年」那句会在没开过店时出现，且 `ch11.liugen` 已置、之后再盘店柜台只说「店已盘出」。目标链 26 → 27 的顺序由提示引导，没有加门闸（施工图没要求）。
4. **n12 battle 返回值**：① 用 `won, how` 两个返回值——`how == "escaped"` 才算逃，其余非胜一律 `game_over()`（含理论上的 `enemy_fled`）。② 只看 `won`：逃与败同一句旁白，`ch11.jiguan` 在 battle 返回后立刻置，再处理扣草，免得扣草失败时漏置旗。
5. **Z1 多加一道前置**：`npc_fan_xiaozi` 从 `ch11.chuansong` 起在场，支线 `accept` 是 `ch11.ningcui`；脚本开头 `ch11.ningcui == 0` 时一句「忙着验货」`return`，不接单。
6. **n13 文案前缀**用 `ch11.jiguan.*`（12.2「若单独起前缀」那一支）：纪律 57「鸡冠蛟只在 `jiguan.*` 与 ② 开场白」靠它成立；`youyao.*` 里 0 处「鸡冠蛟」。
7. **「二十余年」**：`peizhu.close` 含「二十余年」（施工图 3.3、13 第 18 条的口径）。12.2 断言「十余年」0 处若按子串扫会命中它；那一条管的是二转年数，本文案 0 处把它作年数讲。协调者写断言时请按词边界或排除「二十余年」。
8. **范夫人的名牌**：n19 `lianer.lua` 用 `fan_furen` 做说话人（13.1 n19 在场表有她），屏上名牌是 role 的「范夫人」，而 12.2 要求 `lianer.*` 文案只叫「范静梅」——文案已照办，名牌跟 role 走不是文案能改的。

## 别路的缺项（validate 2026-10-12 全仓 32 错，本路 0）

- `data/pathactions/ch11.json`：14 条 `ch11.path.*` 旗标未登 `data/flags.json`（协调者 8.2）；另有 4 条 `when` 与 NPC 在场旗标对不上（路径行动路）。
- `data/text/ch11_battle.json`：`ch11.battle.ningcuidao_youyao.intro` 与 `data/text/battles.json` 旧占位重复（协调者删旧占位，契约 8.2）。
- `maps/ch11_dongfu.tmj`：规则 30 报 `npc_jiedan_linju_a` / `_b` 同 role 同旗标（施工图 O-4，地图路）。
- `tools/artgen/artgen.py --check`：21 个文件不同步、缺 `maps/ch11_yinsha_dao/meta.json`（美术路）。
- `genmaps.py --check`：MAPGEN_IN_SYNC。
- `validate_selftest.py`：SELFTEST_FAIL（363 条、26 条未如期抓住）。第 0 条「未改动的副本 0 错误」被 `maps/ch11_dongfu.tmj` 的规则 30 错顶红，其后依赖干净基线的用例连锁红（路径行动前置、MAGIC_TARGETS 等）；全部 FAIL 详情里 0 处提到 `scripts/ch11`、`zhifadui.lua` 或本路五份文案文件（全文输出留在本会话 scratchpad `selftest_full.txt`）。地图路修掉那一条后应重跑。
- 本路自检：34 个脚本 ＋ patch 后的 `zhifadui.lua` 全部过 `luac -p`（用 `vendor/lua-5.4.9` 临时编的 luac，产物只在 scratchpad）。
