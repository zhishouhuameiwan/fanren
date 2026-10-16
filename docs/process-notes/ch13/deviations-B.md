# 第 13 章 内容路 B（数据）施工偏差

2026-10-14。规范：`fanren/docs/ch13-design.md`（施工图）与 `fanren/docs/interfaces-p3-ch13.md`（契约）；协调者备注 2026-10-14 节。
下列每一条都是「文档没写死 / 文档这么写、落地改成了那样」，没列的就是照抄。生成器留在本会话 scratchpad（`gen_*.py`），不进仓库。

## 1. 白名单口径

- 契约 6.4 把 `data/text/ch13*.json` 全归内容路 C；派工另给本路四份：`ch13_items.json`、`ch13_path.json`、`ch13_quests.json`、`ch13_objectives.json`。以派工为准。
  **C 路不得再写这几类 key**（重复 key 会让 `loadGameData` 整个起不来）：`item.desc.*`、`magic.desc.*`、`ch13.path.*`、`ch13.quest.*`、`ch13.shop.name.*`、`objective.ch13.*`。
- `ch13_battle.json` 归 C：契约第 4 节「开场白与蓄势文案在 ch13_battle.json」、5.2「② 的开场白由内容路 C 写进 ch13_battle.json」、6.4 白名单三处一致，施工图 8.2 未另写归属——无矛盾。本路编成 / role 引用的四个 key：
  `ch13.battle.nanming_zhuibing.intro`、`ch13.battle.nanming_zhuibing.charge`、`ch13.battle.waixinghai_lieyao.intro`、`ch13.battle.waixinghai_lieyao.charge`。
- `b13_waixinghai_lieyao` 的 `intro_key`：契约给的 key 名 `ch13.battle.waixinghai_lieyao.intro` 与旧占位**同名**，所以字面不变，只是出处从 `data/text/battles.json`（协调者删那一行）换到 C 的 `ch13_battle.json`。

## 2. 与施工图 / 契约不一致处（以契约为准）

1. **目标链文案 key**：施工图 3.4 写 `ch13.objective.<step id>`；契约 5.2 写 `objective.ch13.*`，第 1–12 章全部与 `Ch05/Ch11/Ch12LexiconTests` 都按 `objective.chNN.` 编号——取 `objective.ch13.<step id>`。
2. **公孙杏那条支线的标题**：施工图第 6 节标题「公孙杏之求」，但接取在 `chuguan`、12.2 要求 `gongsun.*` 之前 0 处「公孙杏」——任务文案（标题、摘要、步骤）0 处她的名字，标题写「雾海来客」；文件 `name` 字段仍照施工图写「公孙杏之求」（`name` 不进 `data/text`）。
3. **`q13_qianyelu` 不写 target**：接取在南明岛（caigou），指向外海的对象会让告示板提前报出外海地名（地图名编在 `chuansong`）。s1 / s2 文案只写到「千叶露」（伴妖草自 `cunjin` 起）。
4. **商店名 `ch13.shop.name.<短名>`** 写在 `ch13_quests.json`（第 11 章同放 `ch11_quests.json` 的先例）；`nameKey` 名为本路自定。
5. **`wuji_haishou` 就地改写不止补「剑」**（契约 6.3 只点了 weaknesses 与 note）：旧值是「写到那一章时重填」的占位，本路给了内容初值——气血 720→320、法力 430→54、攻 66→34、防 50→36、身法 30→24；`magics` 去掉冰箭术（对结丹防御只打 1 点，G-34 下会被反复放）、只留海潮诀三发。`toughness 3`、`element 4` 不动，不写 `killable_by`。
6. **`b13_waixinghai_lieyao` 去掉 `rewritten_by_lore`**（src / tests / tools 0 处读它）；`name`「珊瑚礁诱妖」。

## 3. 数字（施工图只写意图、不写数，这里是内容初值，归平衡路）

| role | 境界 | 气血 / 法力 | 攻 / 防 / 速 | 五行 | 架势 / 破绽 / killable_by | 其他 |
| --- | --- | --- | --- | --- | --- | --- |
| `nixing_rusheng` 白面儒生 | CoreEarly | 480 / 96 | 34 / 38 / 24 | 木 | 3 / 金剑 / 金剑 | 黑雾怪鸟四发 |
| `nixing_dahan` 黑壮大汉 | CoreEarly | 600 / 0 | 38 / 44 / 18 | 土 | 4 / 金木剑 / 木剑 | 蓄势 every 3 ×2.0 单体 |
| `wuji_haishou` 五级海兽 | CoreEarly | 320 / 54 | 34 / 36 / 24 | 水 | 3 / 土剑 / — | 海潮诀三发 |
| `qingse_juxia` 青色巨虾 | CoreMid | 820 / 54 | 36 / 48 / 26 | 水 | 5 / 金木剑 / 木剑 | 行动 2；蓄势 every 3 ×1.8 全体；海潮诀三发 |

- 理由：韩立结丹初期 700 / 攻 62 防 46，剑（青竹蜂云剑 +40）一刀约 160；第 12 章平衡实测敌方全落在锚点以下（`notes/ch12/balance.md`），且意图手先拿各类逐个试探破绽。② 是两波串打、第 0 波不回血，所以第 0 波压得很轻；巨虾攻防远低于结丹中期锚点 76 / 58。
- **G-34**：凡配了法术的敌人，法术一发与普攻相当（儒生怪鸟约 25 对普攻约 22；海潮诀约 25 对普攻约 22–27），法力只够三四发，放完落回拳脚——不会出现「整场只放一门弱法术」。大汉法力 0。
- `rewards.cultivation`：① 300（施工图 8.1 建议值）、② 40（≤ 60，O-10）。
- 法术：`magic_ji_wuxinghuan` 金、stagger 3、needMp 30、needRealm CoreEarly（不进规则 25）；`magic_fengyun_heji` 木、power 64、needMp 40、needRealm CoreEarly、boost power（`BATTLE_EXTRA_MEANS` 的 learn 来路，守第 13 章起点 CoreEarly / 470）；`magic_mihuan_guniao` 水、power 24、needMp 24、needRealm CoreEarly、target all、boost power。
- 配方 `recipe_zhiyuan_dan`：difficulty 60、requiredProficiency 40 → ch12-end 炼丹熟练 100、资质 50、丹炉 grade 4：40 + 50 + 10 + 20 − 60 ＝ **60%**（施工图第 7 节要 ≥ 50%）。
- `pill_zhiyuan_dan` grade 4、price 1500、restoreHp 300（笔记 L.1 估值照抄）。story 一律 price 0、tradeable false，唯 `story_manao_jiao` price 5000（买价）。
- 商店：南明岛 buyRate 0.9 / sellRate 0.5、限量不补；明珠轩 1.0 / 0.8、entries 空；寸金阁 1.0 / 0.5，霓裳草与黄精各 5 株、每 5 日补 3、上限 5，玛瑙角 1 件不补，回气丹 3 颗、每 10 日补 1、上限 3；秘市 1.0 / 0.6、限量不补。
- 路人境界：`gongsun_xing` 取 QiRefining10、`gu_yu` 取 QiRefining13（原文只说炼气期）；其余照施工图 5.1 / 5.2。数值一律照锚点（与第 12 章路人同写法）。
- 描述名牌：`liu_hufa`「逆星盟壮汉」、`min_xiushi`「淡金脸修士」、`xuan_xiushi`「凶相修士」（施工图 16.2 第 15 条建议原样取）。

## 4. 文案

- 目标链 R-3 提示 11 条照抄施工图 3.4 原句；其余 23 条自写。每步按「上一步的场景」编号自查（第 1 步按 `lingquan`）。
- 路径行动 13 条 59–88 个汉字（`PathActionTests` 40–150）。编号照 12.2「when 里最晚的链上旗标」：K5 编在 `yinjian`，赵护法不自报名号、也不提于护法；K8 编在 `chuansong`，不点易敬 / 黑石城、不叫「曲」；K2 只说「少主」与「圣祖唯一一脉」。
- 物品描述按 give 它的脚本编号：玛瑙角没有 give（寸金阁货架），按最早合法的 `caigou` 写、不点寸金阁；`guyu_yujian` 不提五色珠图（`chongshi` 才点名）；迷幻砚、白玉大印的描述不写原著的砚香、压断巨剑那几样质感。
- 支线文案按施工图 12.2 的编法（Z1 `mingzhu`、Z2 `daoqiyuan`、Z3 `buzhen`；千叶露按接取 `caigou`、公孙杏那条按接取 `chuguan`）。

## 5. 交别路 / 协调者

1. 协调者：登 `ch13.path.*` 13 条（id 与施工图 16.3 一字不差）；规则 25 照契约 5.2 ＋ 备注 2026-10-14 登（沙箱代登实测 0 报）；`battles.json` 删旧占位。
2. 内容路 C：`ch13_battle.json` 写上面四个 key；脚本置齐目标链与支线的旗标（现真仓 validate 里「没有任何脚本会置它」全是 C 未落地）。
3. 测试路：`PathActionTests.cpp` `realmCapOfChapter()` 补 `{13, Realm::CoreMid}`（沙箱实测 `EveryGateIsWithinWhatHanLiCanReachInThatChapter` 只红这一条，契约 5.3 已列）。
4. 美术路：`qingse_juxia`、`wuji_haishou` 两个非人形的立绘与 looks（施工图 5.4）。

## 6. 自检

- 真仓 validate（2026-10-14，几路并行中）：错误 108 条，本路文件引起的只有三类、全是别路未落地——13 条路径旗待登（协调者）、四个 `ch13.battle.*` 文案 key 待 C 写、旗标「没有任何脚本会置它」（C 的脚本）。其余是地图对象缺脚本（A / C）。
- 沙箱（复制 data / maps / scripts / tools；代登路径旗、删旧占位、四条桩战斗文案、一份置旗桩脚本、规则 25 表 ＋ `CHAPTER_START[13] = ("CoreEarly", 470)` ＋ `MEANS_LAST_CHAPTER = 13`）：validate 28 条，全是「地图对象的脚本不存在」与桩脚本缺 @hook，**本路 0 条**；规则 25 对两张编成与 `magic_fengyun_heji` 的施展门槛 0 报。
- 沙箱跑 `build-ch13eng/fanren_tests.exe`（src 未改）`LoadGameData.*:PathAction*.*:Quest*.*`：92 过、3 红——`realmCapOfChapter` 缺 13（测试路）、沙箱缺 `tests/fixtures` 与 `tests/Ch05LexiconTests.cpp`（环境）。
- `validate_selftest.py` 在真仓 15 分钟超时未跑完（输出缓冲、无结论），不作证据；读代码核过：wuji_haishou 仍只在 `b13_waixinghai_lieyao` 一张、`chapter 13`、敌方——两处探针的先验照旧成立，挪探针归协调者（契约 5.2）。
- 原创性：`lcs_check.py --from 500 --to 542` 四份 86 条，SELF_CHECK_OK；首轮 over 3（「以五级妖丹为主」7、「我们这样的小宗门」整本 8、「去看看是怎么回事」整本 8），改说法后 **over 0**。
- 章内先后：按 12.2 写了一个扫描（key → 场景号 → 段前禁词 ＋ 全章 0 处词表），四份 0 处；`{key.` / `{pad.` 0 处。本路 61 个数据文件（含 note）全章禁词 0 处（首轮 `q13_gongsun_qiuyao` 的 origin 里有一处，已改说法），全部 UTF-8 无 BOM、LF。
