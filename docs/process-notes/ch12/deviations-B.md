# 第 12 章 内容路 B（数据）施工偏差

2026-10-13。规范：`fanren/docs/ch12-design.md`（施工图）与 `fanren/docs/interfaces-p3-ch12.md`（契约）。下列每一条都是「文档没写死 / 文档这么写、落地改成了那样」，没列的就是照抄。生成器留在本会话 scratchpad（`gen_b12_*.py`），不进仓库。

## 1. 白名单口径

- 契约 6.3 / 6.4 把 `data/text/ch12*.json` 全归内容路 C；本路派工另给了五份：`ch12_items.json`、`ch12_path.json`、`ch12_quests.json`、`ch12_objectives.json`、`ch12_battle.json`。以派工为准；C 路不得再写 `ch12.battle.*`、`item.desc.*`、`magic.desc.*`、`ch12.path.*`、`ch12.quest.*`、`objective.ch12.*` 这几类 key（重复 key 会让 `loadGameData` 整个起不来）。
- `ch12_battle.json` 除契约要的 `.intro` / `.charge` 外另写了 `.win` / `.lose`（照 `ch11_battle.json` 的拆法；引擎不读、脚本可选用）。③ 胜利写「收服」，④ 写「圣火反噬」，全文 0 处「杀死玄骨」「自焚」。C 路若在场景 key 里另写战后旁白，不冲突。

## 2. 交协调者（不在本路白名单）

1. `data/text/battles.json` 第 7 行旧 `ch12.battle.neidian_duoding.intro` 与本路 `ch12_battle.json` 重复——**协调者删旧行之前，`loadGameData` 起不来**（契约 5.2 第 6 条，同第 11 章的坑）。沙箱里删掉旧两行、删三份占位之后 `LoadGameData.*` 11 条全过。
2. `ch12.path.*` 13 条待登 `data/flags.json`：`jinqing_yizhi_dating`、`shidie_hongyue_dating`、`huyue_sanxiu_dating`、`wenqiang_kuixing_dating`、`xuangu_jinzhi_dating`、`ziling_canyuan_dating`、`zhifa_guiju_dating`、`ziling_guiwang_dating`、`geli_guiwu_dating`、`xuangu_lingshen_dating`、`qingyi_binghuo_dating`、`wuchou_canyuan_dating`、`qingyi_qianlan_dating`（id 与施工图 16.3 一字不差）。
3. **契约 5.2 漏了一处**：`tests/Ch07BattleDataTests.cpp` 第 319 行断言 `MEANS_LAST_CHAPTER` ∈ {7…11}，协调者改成 12 时这条会红（沙箱实测），要同批加上 `= 12`。
4. 规则 25 表照契约 5.2 登进沙箱副本后，四张编成的 `role_means_available` 全过（`zi_ling_xutian.weapons` 含剑；魔火、青火雷、冰箭术的五行 / 境界 / 法力都对得上）；剩下的 3 条只是桩脚本没写 `magic.learn("magic_shijin_chongqun")` 与两处 `give("talisman_bixie_shenlei")`，C 路落地即消。

## 3. 数字（施工图只写意图、不写数，这里是内容初值，归平衡路）

| role | 境界 | 气血 / 法力 | 攻 / 防 / 速 | 说明 |
| --- | --- | --- | --- | --- |
| `xuan_gu` | CoreLate | 1400 / 860 | 88 / 64 / 34 | 攻防低于结丹后期锚点（102 / 82）：④ 是结丹初期独战、它 `actions 2`；③ 做友军时同一份数 |
| `sanshou_guiwu` | CoreLate | 1500 / 0 | 88 / 62 / 28 | 同上理由 |
| `tiehuo_yiqun_da` | CoreMid | 1200 / 0 | 72 / 58 / 22 | `actions 2` |
| 其余 7 个敌人 | 见文件 | — | 均在本境界锚点上下 | — |

- 蓄势：① 三首 `every 3 ×1.8 all`；② 大群 `every 3 ×1.6 all`；③ 银形 `every 3 ×1.8 all`；④ 玄骨 `every 3 ×2.0`（单人，`all false`）。架势：炜吾 3、鬼夜叉 4、灰影 5、怪虎 5、三首 8、蚁群 4、大群 7、火形 5、银形 6、玄骨 8。
- `rewards.cultivation`（施工图只要求 > 0）：① 360、② 360、③ 300、④ 480。
- 战斗 `terrain`：① `forest`、② `field`、③④ `secret`；`name`：鬼雾三波 / 黑沙漠蚁群 / 器灵出鼎 / 修罗圣火（③ 旧名「内殿夺鼎混战」换掉，名字里的词都不早于各自场景）。
- 五行：`langxing_qiling_yin` 的 `element` 取 0 不取金——取金则火克金，③ 里玄骨的魔火会对它加成，与「只有玩家的金收得了」相背；`langxing_qiling`、两种蚁群取火（水克火，冰冻术有加成）。元婴群像的 `element` 是随手配的，不参战。
- 法术：`magic_shijin_chongqun` 金、power 22、needMp 16、needRealm FoundationLate、`boost hits`；`magic_bixie_shenlei` 木、power 70、needMp 0（只经道具）；`magic_qinghuo_lei` 火、power 30、needMp 24、needRealm CoreEarly。
- `material_lianjing` price 6000、`tradeable false`（同 `material_jinleizhu` 的写法）；story 一律 price 0、`tradeable false`。
- `zi_ling_xutian.magics` 留空（规则 25 只要她的剑）。

## 4. 文案

- 路径行动文案先写短了：`tests/PathActionTests.cpp` 第 1123 行要求打探 40–150 个汉字（施工图第 13 节没写这条），沙箱实测四条红，已加长；现 43–58 字。
- K12 `wuchou_canyuan_dating` 不让乌丑叫「韩师弟」：改口是 n26 的剧情事实，K12 的窗口在 `huanjing → neidian`，早于它。
- 目标链 R-3 提示七条照抄施工图 3.4 原句；其余 26 条自写。n17 不写「玄骨还在那里」（`npc_xuan_gu` 在 `lingshen` 撤场），n30 不写「鼎破了」（狼是破鼎而出，鼎还在）。
- 物品描述按施工图 13.1 的质感边界去掉了几样：辟火宝衣不写冰蚕丝，披风不写两层，虚天鼎不写缩到掌上，金雷竹小箭不写插在白骨上、不写残魂（n31 给时他还不知道）。
- Z2 任务文案不点名狼首傀儡、蛇卫（接取在 `neidian`、早于 `cengjie`）。

## 5. 自检

- 门禁（真仓，2026-10-13 第 12 章并行施工中）：validate 75 错，本路名下只剩「13 条路径旗待登记」「battles.json 旧 intro 重复」两类（协调者）与「旗标没有脚本会置」（C 路未落完）；mapgen IN_SYNC；artgen 因 `data/visual/maps.json` 尚缺 ch12 预设报错（A 路）；selftest 363 例 30 例未抓住：两处乌丑探针是 ③ 写实的预期后果（契约 5.2 由协调者挪到 wuji_haishou），其余是仓库不净的连锁（「不动的副本→干净」「真实 main」那几类）。
- 沙箱（复制 data/maps/scripts/tools，给 ch12 写只置旗 / teleport / battle 的桩脚本，再代做协调者 5.2）：validate 只剩桩脚本缺 `@hook` 的 39 条；`fanren_tests.exe`（build-ch12coord 的现成二进制，src 未改）在沙箱里跑 LoadGameData / PathAction / Quest / Objective / Lexicon / NpcPresence / Battle / Realm 共 614 例，余下的红全是沙箱缺 assets、saves、`tests/scripts` 或测试路的表（kScriptTransfers、`realmCapOfChapter` 缺 12）。
- 原创性：`lcs_check.py --from 416 --to 499` 五份 100 条，SELF_CHECK_OK，首轮 over 2（「只有纯金之物」7 字、「长长出了一口气」整本 9 字），改说法后 over 0。
- 章内先后：按 12.2 写了一个扫描（key → 场景编号 → 段前禁词），五份 0 处；全章禁词与 `{key.` / `{pad.` 在本路 75 个文件里 0 处（一处「杀死玄骨」出现在编成 `note` 里，已改说法）。
