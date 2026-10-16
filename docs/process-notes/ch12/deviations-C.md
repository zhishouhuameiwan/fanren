# 第 12 章内容路 C（脚本与主线文案）偏差记录

2026-10-13。白名单：`scripts/ch12/**`（39 个：33 节点 ＋ `zoupian_a/b/c` ＋ `yihai` ＋ `caiyao` ＋ `yutai`）、`scripts/ch11/zhuzhong.lua` 末尾一行 `teleport("ch12_tushan")`、`data/text/ch12_{yizhi,dadian,guiwu,binghuo,neidian}.json`（五幕各一份，443 条）。

## 与施工图 / 契约的偏差

1. **白名单与契约不一致处（以派工为准）**：契约 5.2、施工图 8.2 写「③ 开场白由内容路 C 写进 `data/text/ch12_battle.json`」；本次派工把 `ch12_battle.json` 划给别路（内容路 B 已落地 16 条）。本路不碰它。四场胜利旁白另写在各自场景前缀里：`ch12.guiwang.win`（鬼物飞灰）、`ch12.tihun.win`（蚁群退散）、`ch12.ruyi.win`（「收服」）、`ch12.xuangu.win`（「修罗圣火反噬」），0 处「杀死玄骨」「自焚」。
2. **`ch12_battle.json` 的 `.win` / `.lose` 八条没有读者**：引擎不读战斗胜负 key（`src/` 只认 `intro_key`、`charge.text_key`，路径行动的 `win_key` 另是一套），第 11 章的同类 key 也没有脚本 talk。本路脚本不 talk 它们，以免与场景旁白重复。协调者若要改用那八条，把 `guiwang/tihun/ruyi/xuangu.lua` 里的 `.win` 那一句换成 `ch12.battle.<短名>.win` 即可（`xuangu.lua` 的 `ash` 一句随之删，否则重复「化灰」）。
3. **取消的口径**：凡抉择之前没有状态改动的，取消即 `return`（3.2 / 第 10 章 27 同口径）；为此把 n01 的 `advance_days(62)` 与 `take` 珍珠挪到抉择之后（动作不增不减，只换先后），n10 的 `advance_days(8)` 同理。抉择落在状态改动之后、`return` 会让重踩时重复拨日 / 领物 / 离队的四处，取消改为「按某一支照走」、不 `return`：n05（按「收剑」，免得重演 `party.remove`）、n20 两处（按罗烟步，免得 `binghuo_ri` 后挪、多拨日）、n23（四门合流，任选）、n24（按「先取披风」，灵石已付）。没有用 `repeat … until pick`：无头驱动不回填时 `choice` 恒为 nil，会死循环。
4. **n16 / n17 的「长约半尺、土黄」**：13.1 把它列在 n16 ⑥，但 n17 ④ 才是「玄骨交出灵参本体（金盒）」。本路 n16 只写「化身落网、本体玄骨去挖」并照 3.2 在 n16 `give("story_jiuqu_lingshen", 1)`；「半尺、土黄」的描述落在 n17 开金盒那一句。
5. **n33 已完成后的重入**：3.2 写「开头 done 已置 → return」。元瑶章末不撤（NPC 一直可对话），所以 `return` 之前补了一句旁白 `ch12.lingquan.again`，免得按了没反应。
6. **采药灵草 id 暂定**：O-13 的 id 协调者未给。`caiyao.lua` 暂写 `give("herb_huangjing_zhi", 2, 300)`（现有灵草、年份上限 2000）。文案不点名灵草，协调者改 id / 年份只动这一行。
7. **走偏坐标**：照抄 `tools/mapgen/genmaps_ch12.py` 的 `FORK_A/B/C` ＝ (27,33)、(18,24)、(30,16)，核过 `maps/ch12_guiwu.tmj`：三格都不与任何对象同格。脚本里 `teleport` 之后还有一句 `talk`（同图回岔口，演完再说「已站回岔口」）。
8. **`give` / `advance_days` 无返回值**：`api.lua` 里这两个只 emit、不回结果，无从「看返回值」；`take` / `party.remove` / `magic.learn` / `battle` 全部看了（`take` 神雷余数在 `n > 0` 分支里，余数 0 不调 `take`）。
9. **n18 两条回选单的传音**：「拜青易」接极阴一句，「都不拜」接玄骨一句（第 7 节「各接对方一句传音」的落法）。
10. **n05 少年自称「本座」、n06 自报名号**：名号句只出现一次（「萧诧」恰 1 处），不带「上人」。
11. **n29 讨要废蚕**：13.1 质感边界禁用「儒衫老者要灭废蚕、他的托词、老者错以为他穷」那一套，本路改成他向名义师尊极阴讨下、极阴一句「随你」。

## 别路的缺项（validate 2026-10-13 全仓 27 错，本路 0）

- `data/text/ch12_battle.json`：`ch12.battle.neidian_duoding.intro` 与 `data/text/battles.json` 旧占位重复（协调者删旧占位，契约 5.2）。
- `data/pathactions/ch12.json`：13 条 `ch12.path.*` 旗标未登 `data/flags.json`（协调者在路径行动落地后登记，契约 5.2），每条报两次，共 26 错。
- `genmaps.py --check`：MAPGEN_IN_SYNC（75 张）。
- `artgen.py --check`：失败，`data/visual/maps.json` 缺十张 ch12 图的预设（内容路 A）。
- `validate_selftest.py`：SELFTEST_FAIL（363 条、22 条未如期抓住）：基线不净（上面 27 错）连锁 + 规则 25 的乌丑探针待挪第 13 章（协调者 5.2）；全文 0 处提到 `scripts/ch12`、`zhuzhong.lua` 或本路五份文案。
- `build_logged.bat ch12C`：停在 validate（EXITCODE=1）。另在 `build-ch12C` 手工 cmake 编译（BUILDRC=0）；仓库原地跑测试全线因上面那条重复 key `loadGameData` 起不来。在 scratchpad 沙盒副本里删掉 `battles.json` 两条旧 intro 与三份占位（＝协调者 5.2 该做的）后跑全量：1859 过、26 败，败项全在别路：`Ch11Walkthrough` ×2、`Ch11Acceptance` ×1、`Ch11TriggerMode` ×2（契约 5.3：zhuzhong patch 后终点变 `ch12_tushan`，测试路改）、`ObjectiveRoute` ×3（`kScriptTransfers` 未登 12 条，测试路）、`PathActionData` ×15（`ch12.path.*` 未登记）、`ChapterTableData` / `ChapterCards` ×3（测试路）。
