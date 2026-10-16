# 第 13 章测试路交付摘要（协调者整理，2026-10-14 17:40）

## 集成
四门全绿（validate 610 旗标、genmaps 79 张、selftest 413 条、artgen 721 个）；规则 25 登 CHAPTER_MEANS[13]（五行环不登）、BATTLE_EXTRA_MEANS b13_waixinghai_lieyao（蜂云合剑 learn ch13）、MEANS_LAST_CHAPTER 13、CHAPTER_START[13]=(CoreEarly,470)；selftest 探针挪到 feng_xi ＋ b14_fengxi_xiezhi（含第 12 章两处「必报」判据改 feng_xi）；删 battles.json 旧占位；合缝 0。

## 测试路 A（build-ch13test.log 03:21，2008/2008，0 警告）
- 第 12 章终档重生成：只差 mapId（ch12_lingquan → ch13_nanming）、position（(12,11) → (6,29)）、计时、checksum。Ch12Driver 的 settleAtSpring 改 landedOnNanming；Ch12TriggerMode 章末判据改「仅末行一处 teleport("ch13_nanming")」。
- 登记类：UiStyle 章节表 13 项、ChapterCards；kScriptTransfers 第 13 章 7 条；kOffChainFlags J.2 七条、kLastChapter 13；realmCapOfChapter {13, CoreMid}；Ch07 放行表 13。
- 第 13 章静态用例 35 条：Ch13Data.h、Ch13TriggerMode 10、Ch13BattleData 7、Ch13Ledger 8、Ch13Lexicon 10。
- 措辞口径（待并入施工图 18 节）：商店「脚本 shop( 恰 4 处」实为 4 个脚本 6 处调用；12.2 未编号类的补编口径（见 Ch13LexiconTests.cpp 文件头）；caigou.lua 首句判 ch13.wenxun 是死代码。

## 测试路 B（build-ch13walk.log 04:49，2028/2028，EXITCODE=0）
- 平衡记录 notes/ch13/balance.md：调 3 个敌方 role 5 个数（nixing_rusheng 气血 480→360；wuji_haishou 法力 54→18、攻 34→30；qingse_juxia 气血 820→700、攻 36→30）。① 胜 16 回合 700→312、蓄势被打断 1 次；② 第一轮胜 13 回合 700→343；② 第二轮胜 10 回合 553→283。回合窗 ① 10–22、② 6–18。
- 终档 ch13-end：day 62070 / 62045（起点 +12510）；realm 32/32/22（结丹中期）；HP 900/900、MP 620/620；修为 235 / 228；低阶灵石 2931 / 2911、中阶 20；五级妖丹 13、六级 7；party 空；法术 14 门；mapId ch13_waihai (60,44)。
- 坐关实测 N = 2 / 2（C0 4452 / 4391，两年后 5235 / 5228，第一次冲关即成）；章末 day 差两侧 12510 = 11790 + 360×2。
- 反证：① 捂住金和剑败（儒生压在 1）、捂住木和剑败（大汉压在 1）、人人只防败；② 捂住木和剑巨虾压在 1、无路可进而逃。最低合法起点 ①② 全胜。
- 新增用例 20 条：Ch13Acceptance 10（两侧通关 2、对账 1、起点负向 1、终点与逐节点账负向 1、平衡 5）、Ch13Slice 10（验收 5–13、27）。
- 施工图要改：8.1 / 1.3 / 3.3「约 6 次以内」→ 实测 2 次、M=720；n29「回话」驱动取第一支 opt_terms；验收 5 种子由面板按 day / 修为 / 境界算、切片先坐 1 日再写 5000；验收 6、12、13、27 切片起点按图上实际可达改了；BattleHand 不按五行环（power 0）、② 巨虾蓄势一次没打断（建议登技术债，同 G-36）；章末朝东但 continueJourney 读档朝下（G-31）。
- 交整改 0 处。
