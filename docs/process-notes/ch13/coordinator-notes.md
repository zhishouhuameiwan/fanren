# 第 13 章协调者备注（给施工路与整改路）

- 2026-10-13 第 11 章真档（tests/fixtures/ch11-end-{first,second}.sav）章末修为只有 2572 / 2511（第 11 章修为只来自诱妖 ① 的 4×120），结丹初期冲结丹中期要 5000（Realm.cpp cultivationNeeded）。第 12 章施工图写全章 realm 0 处变动；第 13 章若让玩家自己打坐冲结丹中期，起点修为按「第 11 章章末 ≈2500 ＋ 第 12 章所得」算，真档出来后再核。
- 第 11 章章末低阶灵石 2339 / 2319、中阶 20 / 20；day 49378 / 49353；境界 31/31；party 只有 qu_hun_shadan（第 12 章 n05 离队）。

## 2026-10-13 20:40 第 12 章真档已出（tests/fixtures/ch12-end-{first,second}.sav，测试路 B，1944/1944 绿）——第 2 节「估」逐项核
- day 49560 / 49535 ✓（与估值一致）；realm / cap / former 31 / 31 / 22 ✓。
- hp 614 / 700、mp 470 / 470（两侧相同）；party 空 ✓。
- **cultivation 4072 / 4011**（第 2 节估「≥ 2872 / 2811」，实测高出约 1200——第 12 章四场修为比预想多）。→ C0 ＝ 起点 ＋ ① 300 ＝ 4372 / 4311，坐到 5000 还差 628 / 689，按期望每日 1.02 点约 616 / 676 日 → **意图玩家「闭关一年」2 次**（不是 8.1 写的「约 6 次以内」）。8.1、1.3、3.3、第 14 节、验收 2、契约第 0 节里引用坐关次数的地方，施工时以本条为准；施工图 18 节记一笔。② 的修为若 > 0，次数可能降到 1–2 次，以测试路 B 实测为准。
- material_lingshi 2331 / 2311、material_lingshi_zhong 20 / 20 ✓。
- **material_yaodan_wuji / _liuji 背包里都没有（0 / 0）** ——偷渡第二支意图玩家走不了，第三支兜底，与估值一致。
- learnedMagics 12 门 ＝ 第 10 章 11 门 ＋ magic_shijin_chongqun ✓。
- story_jinsi_can 3（两侧 ch12.jincan == 1）；story_miaoyin_yaopai、story_xuangu_leigu、material_lianjing、story_yanghun_mugen、story_wannian_lingru、story_xuese_pifeng、story_wuse_tonghuan、story_jiuqu_lingshen、story_xutian_ding、story_qianlan_zhu、story_tihun_shou、story_hualan 各 1 ✓。另有 story_wucai_xiaozhu 8、story_yaolang_luan 12。
- bottle capacity 9、drops 9、owned。
- mapId ch12_lingquan、position (12,11)、朝上；章末脚本是 scripts/ch12/lingquan.lua（第 25 行 flag.set("ch12.done")、头注明「不 teleport，第 13 章 patch 本脚本」）。本章 patch：在 flag.set("ch12.done") 之后末尾加一行 teleport("ch13_nanming")；然后 FANREN_WRITE_CH12_FIXTURES=1 重生成 ch12-end 两份，Python 核新旧只差 mapId / position / checksum / 计时。
- 第 12 章测试路 B 平衡调过 10 个敌方 role、xuan_gu 法力 860→96（BattleState::enemyDecision 有法力就只放法术）；第 13 章的敌人配法力时同样注意：只配一门弱法术又给大法力，敌人会整场只放那一门。意图手不吃药（养精丹 55、金疮药 15 对 700 气血不值一手）。

## 2026-10-14 开工前：第 12 章提交（46f6d37）后门禁的新口径（契约写于这些改动之前，以本节为准）
- **validate 的 learn 来路现在核施展门槛**（技术债 G-29 已解决）：`CHAPTER_START[N]`（第 N 章起点境界、法力上限，取上一章终档）＋ `check_learned_castable`——needRealm ≤ 起点境界、needMp ≤ 起点法力上限。**有 learn 来路的章漏登 `CHAPTER_START` 直接报错**（G-8）。第 13 章集成时登 `CHAPTER_MEANS[13]` 的同时必须登 `CHAPTER_START[13] = ("CoreEarly", 470)`（ch12-end 两侧 realm 31、maxMp 470）。`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]` 的 `magic_fengyun_heji`（learn，ch13）也按第 13 章起点核：它的 needRealm 必须 ≤ CoreEarly、needMp ≤ 470——内容路 B 定这门法术的数值时守住。
- **give 一件施法符登记五行类**时核 castMagic 存在、五行对、power > 0、无 effect（`check_given_spell`）。
- validate_selftest.py 的 CH12 段（`selftest_ch12_means`）会逐章读 `tests/fixtures/ch{N-1}-end-*.sav` 与 `CHAPTER_START` 比对：登了 `CHAPTER_START[13]` 就会读 `ch12-end-*`，它重生成后只差 mapId / 坐标，realm / maxMp 不变。
- **selftest 探针挪位**（契约 5.2）：除了契约写的 `outside_role` / `outside_battle` / 先验 / `MEANS_LAST_CHAPTER` 断言，第 12 章整改又给两处「必报」判据加了 `"wuji_haishou" in error`（selftest 第 1825、2525 行一带，G-5）——挪到 `feng_xi` 时这两处一并改成 `"feng_xi" in error`。selftest 全量现为 411 条。
- 第 12 章技术债新增 G-34（enemyDecision 有法术合法就只放法术、不比普攻——敌人只配一门弱法术又给大法力会整场只放那一门）、G-35（settle 不回写法力）、G-36（BattleHand 不会用单体 castMagic 道具）。第 13 章配敌人法力时注意 G-34。
- 第 12 章 `scripts/ch12/lingquan.lua` 现状：第 25 行 `flag.set("ch12.done")` 是最后一个动作，头注释写「不 teleport（第 13 章 patch 本脚本）」；补 teleport 时头注释那半句一并改成现状。
- 第 12 章测试里钉「lingquan.lua 不 teleport」的断言要随 patch 改（契约 5.3 第一行；测试路）。
