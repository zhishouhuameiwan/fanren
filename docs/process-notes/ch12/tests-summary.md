# 第 12 章测试路交付摘要（协调者整理，2026-10-13 20:20）

## 测试路 A
全量 1925/1925 绿；第 11 章终档新旧只差 mapId（ch11_dongfu → ch12_tushan）、position（(14,24) → (24,31)）、checksum 与一帧计时；Ch12 静态用例 40 条（Ch12Engine 23、TriggerMode / BattleData / Ledger / Lexicon）。

## 测试路 B（构建槽 ch12walk，build-ch12walk.log 20:14，EXITCODE=0，1944/1944，零警告，四门全过）
- 两侧通关全绿，四场两侧都胜；ch12-end-{first,second}.sav 写出后清开关重跑仍绿。
- 四场：① 鬼王 16 回合胜；② 铁火蚁 10 回合胜；③ 器灵 10 回合胜；④ 玄骨 8 回合胜。每场两侧各打三遍结局指纹一字不差。
- 反证：② 捂住水、金打不赢；③ 韩立只防、只靠友军打不赢；④ 捂住木打不赢、人人只防就输。最低合法起点（无丹药、中阶灵石 0、静养十日）②③④ 全胜。
- 调了 10 个敌方 role（weiwu_hanggui、gui_yecha、hui_ying、shuangtou_guihu、sanshou_guiwu、tiehuo_yiqun、tiehuo_yiqun_da、langxing_qiling、langxing_qiling_yin、xuan_gu）的气血 / 攻 / 防 / 架势；xuan_gu 法力 860→96。全文 notes/ch12/balance.md。
- 意图手不吃药（养精丹 55、金疮药 15 对 700 气血不值一手）。
- 新增用例 19 条：Ch12Walkthrough 2、Ch12Acceptance 3、Ch12Balance 5、Ch12Slice 9（验收 5、6、8、9、10、11×2、15、16；第 7 条只读数据无切片）。
- 终档：day first 49560 / second 49535（日差都是 182）；realm 31；HP 614/700；mapId ch12_lingquan (12,11) 朝上。

## 测试路 B 交整改的事
1. 敌方 AI 有法力就只放法术（BattleState::enemyDecision 不跟普攻比）。xuan_gu 只配魔火（威力 12），860 法力能放 71 手、每手 1–2 点。**协调者裁定：接受法力降到 96**（放八手魔火后改拳脚；不低于 12，守规则 25），写入施工图 18 节。
2. BattleHand 不会用单体 castMagic 的道具，辟邪神雷没量过，四场胜负不靠它。
3. 战后只回写气血不回写法力。
4. 调后数值远低于境界锚点（玄骨 450/36/40 vs 结丹后期锚点 1300/102/82）：意图手先用法术逐类试探破绽、四场之间不静养、血线串起来。

## 施工图要改（测试路 B 提出）
1. 验收第 2 条「站在 quanbian」：n33 挂在元瑶 (15,9) 身上，驱动说完话走回 (12,11) 朝上才满足；措辞要说明。
2. 18.3 第 2 条（玄骨 1400/860、88/64/34 等）与 8.1 意图手描述按 balance.md 并入第 18 节。
3. 第 8 节没给回合目标，断言用实测放宽约四成：① 12–20、② 7–14、③ 6–14、④ 5–12。

## 协调者已知、整改轮要做
- 冰火道（ch12_binghuo）的熔岩被 artgen 画成水：给 artgen 水面加只作用于 ch12_binghuo 的熔岩变体（maps.json 由 artgen 与 FxDemoScene 读，运行时只读 meta.json），重生成后 artgen --check。
