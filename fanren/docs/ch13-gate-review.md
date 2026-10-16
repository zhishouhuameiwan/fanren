# 第 13 章引擎与门禁独立审查

> 2026-10-14 · 只审不改 · 对象：`src/`（本章应为零改动）、`tests/Ch13EngineTests.cpp`（E1 结丹档面板冲关、E2 跨图同一块田，27 条）、`tools/validate.py` 规则 25 四张表（`CHAPTER_MEANS[13]`、`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]`、`MEANS_LAST_CHAPTER`、`CHAPTER_START[13]`）、`tools/validate_selftest.py` 探针搬家、本章 `tools/` 改动（`artgen/mapwater.py`、`artgen/sprites_enemies.py`、`mapgen/genmaps.py`、`mapgen/genmaps_ch13.py`、`audiogen/catalog.py`）。规范：`docs/interfaces-p3-ch13.md`（契约）、`docs/ch13-design.md`（施工图 8.2、16.1、第 18 节）、协调者笔记 `notes/ch13/{coordinator-notes,integration,tests-summary,balance}.md`；口径参照 `docs/ch12-gate-review.md`。
> 阴性对照全部在 scratchpad 的 `gate13/` 临时副本里做（`ctree` C++ 副本、`vtree` 门禁副本、`ptree_P0…P5c` 探针副本），每条改完即还原、还原后与仓库逐字节比对；仓库里只写了本文件。

## 结论：Approve

没有 CRITICAL / HIGH / MEDIUM。`src/` 零改动，与契约「本章 0 个代码项」一致。`Ch13EngineTests` 的 27 条在独立构建上 27 / 27 绿、0 跳过，契约 1.3 第 1–8 条、2.3 第 1–6 条都有对应用例且带负向；引擎路自报的 8 处变异，本审复做 A、C、H 三处（另加一处自己的 R），全部只红在预期的用例上。规则 25 第 13 章四张表与两场编成、韩立的实际手段逐场核过：每个敌人的破绽至少两样打得到，蜂云合剑在 ② 之前必然学会且按第 13 章起点（CoreEarly / 470）施展得出，四个敌人的法力都够放自己的法术；五行环不登的理由成立（削架势不判破绽、不伤人）。selftest 探针挪到 `feng_xi` ＋ `b14_fengxi_xiezhi` 后有效：末章改回 12 恰 3 条 FAIL，第 12 章整改加的点名判据挪过来之后仍有牙（P5 / P5c）。本章 `tools/` 改动无回归，`lotus_of` 只动 `ch13_wuzhong`。四门全过。

四条 LOW 都是「现在的数据是对的，门禁或文档没跟上」：G-1 `learn` 来路不核 `power / effect`（五行环登上去门禁不拦，不登全靠人记）；G-2 敌方法力 ≥ 耗蓝无人钉着，平衡笔记误称规则 25 守得住；G-3 施工图 18.3 仍是平衡前的数；G-4 契约两句 `GTEST_SKIP` 措辞没跟上集成。都不挡合入。

### 本审实跑

| 项 | 结果 |
| --- | --- |
| `git diff -- fanren/src fanren/CMakeLists.txt`、`git status -- fanren/src` | 空：`src/` 与 CMake 零改动 |
| `python tools/validate.py` | `VALIDATE_OK`：835 项、6046 条文案、610 个旗标，0 错 0 警告 |
| `python tools/mapgen/genmaps.py --check` | `MAPGEN_IN_SYNC`：79 张 |
| `python tools/artgen/artgen.py --check` | `ARTGEN_IN_SYNC`：721 个文件（68.5 s） |
| `python tools/validate_selftest.py` | `SELFTEST_OK`：413 条，未如期抓住 0 条（31 分 14 秒） |
| 独立 C++ 构建（`gate13/ctree`，238 步 0 警告） | E1–E2 27 / 27、`Ch13*` 82 / 82、全量 2028 / 2028；变异 A / C / H / R 各如期红 |
| 阴性对照 | 规则 25 共 18 条（V0–V11 含 V7b 边界，B1、B2b、B2c 对照、B3、B4）、selftest 探针 7 种（P0–P5c）、`lotus_of` 3 条；意外只有标 ✗ 的 B1、B2b、B3、B4 四条，记成 G-1、G-2 |

## 一、`src/` 与契约 E 项

契约头部：「本章 0 个代码项……`src/` 一行不改」。核实：`git diff -- fanren/src fanren/CMakeLists.txt` 为空、`git status -- fanren/src` 无未跟踪文件。E1、E2 都是验证项，只看判据。

顺带核了契约第 0 节引用的几处引擎口径，与代码一致：

- `rules::breakthroughBlock`（`src/core/rules/Cultivation.cpp` 第 151–159 行）依次判 `SeriesMax` → `StoryCap` → `NotEnough` → `None`，**上限先于修为**；`kBreakthroughMinChance 5` / `kBreakthroughMaxChance 95`（`Cultivation.h` 第 73–74 行）。
- `Application::raiseRealmCap`（`Application.cpp` 第 1400–1402 行）只升不降，`realm.cap` 走第 1392 行这一处、不动 `realm` 与气血。
- `Application::openFacility`（第 682 行起）先判 `require_flag`（未置 → `showMessage("ui.facility.locked")`，第 690–694 行）再分 kind；`field` 分支（第 703–720 行）`findField(ref_id)` 已有就直接开 `FieldScene`，没有才按 `slots`（缺省 `kDefaultFieldSlots`）新建——已有的田不按这一处的 `slots` 改尺寸。
- `BattleState::applyStagger`（`BattleAction.cpp` 第 519–548 行）：削架势「不判破绽、不揭开、不伤人」，与类别无关——这是第三节五行环不登的真正依据。

## 二、判据覆盖（`tests/Ch13EngineTests.cpp`，27 条：E1 17、E2 10）

独立构建（`gate13/ctree`：仓库 `src/ tests/ CMakeLists.txt` 逐字节复制，`vendor / data / maps / scripts / assets / docs / tools / saves` 目录联接；MSVC Release，238 步、0 警告）：E1–E2 27 / 27 绿、0 跳过，`Ch13*` 82 / 82 绿。

| 契约 | 覆盖 | 有没有牙 |
| --- | --- | --- |
| 1.3-1 起点被上限挡 | `FiveThousandAtThirtyOne…` ＋ 负向 `…CapAtThirtyTwoIsNotBlocked` | 有：五颗种子 `blocked / success / cultivationLost / cultivationSpent / backlash` 全查；负向证明挡它的是上限 |
| 1.3-2 剧情只抬上限 | `StoryCapToCoreMidLiftsOnlyTheCap` ＋ 负向 `…InvalidCapIsRefused…` | 有：`hp` 先写 555、探针旗标先写哨兵 99；`realm.cap(99)` 在上限 31、32 各跑一次，`realm.cap(CORE_EARLY)` 只升不降。引擎路变异 B（`raiseRealmCap` 改直接赋值）只红负向这一条 |
| 1.3-3 门槛与成功率 | `Ch13CoreMidRule` 两条 | 有：4999 / 0、5000 / 85、6000 / 89、10000 / 95 全是字面量；负向 200 颗种子不足不算尝试 |
| 1.3-4 面板冲成 | `ThePanelBreaksThrough…` ＋ 负向 `WithoutTheCap…` | 有：经真 `openFacility(meditate)` 开面板；每按前复位 5000 / 300；失败那几次顺带钉 1000 / 2000 倒扣；成功 `realm 32`、修为 0、900 / 620、`hp` 300 → 500；再按 `StoryCap`。本审复做变异 A、H 均只红这一条（见第七节） |
| 1.3-5 面板那一行 | `ThePushRowIsOff…On…` | 有：两态成对，另核行名是 `pushLabel` |
| 1.3-6 静室 150 | 三条 | 有：十颗种子 `2y == 3x` 且各等于 `rules::meditate`；对照组余数账 1350 时 32 颗种子逐颗等于差分、且至少一颗 ≠ `meditate(360)`（对照自证区分得开）；真设施 `ch13.lifu` 0 → `Dialogue`、置旗 → `Cultivation`、加成行含 50、面板自己坐一年仍 `2y == 3x`。引擎路变异 F（丢 `sitePercent`）三条全红 |
| 1.3-7 出关门闸 | `TheLeavingGate…` | 有：哨兵 99；变异 G（`>=` 改 `>`）只红这一条 |
| 1.3-8 真数据 | 三条读 `maps/`、`scripts/` ＋ 扫描器负向 | 有：`effectiveness "150" / "125"`、`require_flag`；`realm.cap(` 全章恰 1 处且只在 `biguan.lua`、`advance / demote` 0 处、`chuguan.lua` 首条活代码行含门闸；扫描器去注释（`biguan.lua` 头注释里就写着「`realm.advance` / `realm.demote` 0 处」，不去会误报）且自带负向 |
| 2.3-1 已有的田原样打开 | `TheFieldInTheSaveOpensAsItIs…` ＋ 负向 `…AnIdTheSaveLacksIsBuilt…` | 有：本审复做变异 C（不论有没有都新建）→ 1、2、3 三条红（`fields` 7 ≠ 6） |
| 2.3-2 不改尺寸 | `ADifferentSlotsOnTheSecondMap…` ＋ 负向 `…SlotsFourIsBuiltWithExactlyFour` | 有：本审自加变异 R（已有的田按这一处 `slots` 改尺寸）→ 恰这一条红（4 ≠ 6） |
| 2.3-3 门闸 | `RuchengUnsetShowsTheLockedMessage…` | 有：两态成对，另钉「门闸未开也不建田」；引擎路变异 E（facility 不判 `require_flag`）红这条与 1.3-6 真设施那条 |
| 2.3-4 采收带年份 | `Harvesting…` ＋ 负向 `…UnripeSlotAndAnEmptySlotRefuse…` | 有：`itemCountOfAge(…, 60)` ＋1、槽腾空；变异 D（采收不带年份）只红这一条 |
| 2.3-5 真数据 | `TheHelperSees…`、`TheTianxingFacilityCarriesExactlyTheFourProperties…` | 有：`ch13_tianxing` 那一处恰 `kind / ref_id / slots / require_flag` 四个属性、全仓 `field_tianxing_39` 恰 `{6}`；内存改 8 → `{6, 8}`；先例 `shenshougu_yaopu {6, 8}` |
| 2.3-6 门禁不读 `ref_id` | `TheGateNeverReadsRefId…` ＋ 本审实跑 validate | 有：`validate.py` 全文 0 处 `ref_id`（本审 `grep -c` 复核为 0）、计数器自带负向；两张图同名设施对象名图内唯一、`require_flag` 已登记；本审 validate 0 错，与该田相关的行自然为 0 |
| 1.3-9 / 2.4 负向自检 | `build-ch13eng-neg-{base,A..H}.log`（10-13 23:54–23:55，按 gb18030 读） | base 27 / 27；A、B、D、G、H 各只红 1 条，C 红 3 条，E 红 2 条，F 红 3 条——与契约 1.4、2.4 的记述逐条一致 |

集成时协调者把 5 处 `GTEST_SKIP()` 改成 `FAIL()`（第 571、586、604、803、859 行），文件头第 1–6 行同步改成「a missing file is a FAIL(), not a skip」，合理。契约 1.3 第 8 条与 2.3 第 5 条正文还写「任一文件不存在 `GTEST_SKIP`」，见 G-4。

夹具仍读 `ch11-end-first.sav`（契约写于 `ch12-end` 生成之前）。两份终档在这些判据读的字段上相同（realm 31 / 31、formerRealm 22、aptitude 50、700 / 470），第 13 章测试路的通关与切片另从 `ch12-end` 起跑，不构成缺口。

## 三、规则 25：两场逐一核对

**起点**从 `tests/fixtures/ch12-end-{first,second}.sav` 实读（两侧相同）：`realm 31`（CoreEarly）、`realmCap 31`、`maxMp 470`；`learnedMagics` 12 门，含火弹术（火，QiRefining1，8）、流沙术（土，Q1，12）、冰冻术（水，Q1，10）、祭金符（金，Q1，12）、青元剑芒（木，FoundationEarly，10）、噬金虫群（金，FoundationLate，16）；背包有 `weapon_lieyang_jian`（剑）、`weapon_wuming_sixian`（暗器），另有 `weapon_yudai_duanjian`、`weapon_qingzhu_fengyun_jian` 两柄剑。`CHAPTER_START[13] = ("CoreEarly", 470)` 与之相等。`scripts/ch13/` 里 0 处 `magic.forget`；`take(` 只扣灵石、妖丹、至元丹、霓裳草与剧情物（`chonglian`、`chuansong`、`gongsun`、`mingzhu`、`mishi`、`wusezhu`、`xuanshang`、`xuanyin`、`yanghun`、`youyao` 逐处看过），不碰兵刃与法术。章内境界只经 n27 一处上限、面板自冲 31 → 32，0 处 `realm.demote`，② 的挂点 `youyao.lua` 首句「`ch13.biguan` 已置 → return」，所以两场都发生在起点境界上，「起点即全章最低」成立。

| 场次（节点） | 敌人：破绽（`killable_by`） | 这一场打得到的手段 | 结论 |
| --- | --- | --- | --- |
| ① `b13_nanming_zhuibing`（n05 `zhuibing.lua`） | 白面儒生 `nixing_rusheng`：金、剑（金、剑）；黑壮大汉 `nixing_dahan`：金、木、剑（木、剑） | 祭金符、噬金虫群（金）；青元剑芒（木）；烈阳剑（剑） | ✓ 每人至少两样，`killable_by` 每一类都有伤人的来路 |
| ② `b13_waixinghai_lieyao`（n24 `youyao.lua`，循环） | 五级海兽 `wuji_haishou` ×2：土、剑（不写）；青色巨虾 `qingse_juxia`：金、木、剑（木、剑） | 流沙术（土）；祭金符、噬金虫群（金）；青元剑芒、蜂云合剑（木）；烈阳剑（剑） | ✓ |

**`BATTLE_EXTRA_MEANS["b13_waixinghai_lieyao"]`**：`magic_fengyun_heji` 木（element 2）、power 64、needRealm CoreEarly、needMp 40、`boost power`、无 `effect`、无别的前置；`scripts/ch13/chonglian.lua` 第 14 行无条件 `magic.learn`（与 `take("material_lianjing")` 成败无关）。挂点链从地图实读：`trigger_lianjianshi`（guard `ch13.xuanyin` → set `ch13.chonglian`）→ `trigger_dizhen`（`chonglian` → `dujie`）→ `trigger_biguanshi`（`dujie` → `wusezhu`）→ `trigger_buzhen`（`wusezhu` → `buzhen`）→ `trigger_youyao`（guard `ch13.buzhen`）：② 只能在学会之后开。编成三个单位全是敌方，不需要 `role_magic` 行。这一条的「木」在通表里已有（青元剑芒），所以它对规则 25 的判定结果是冗余的出处，但 `check_means_sources` 照样按第 13 章起点核它（阴性对照 V7–V10 全抓住）。

**敌人的法力 ≥ 法术耗蓝**（G-34：有法术合法就只放法术）：

| role | 境界 | 法力 | 法术（needRealm / needMp） | 放得出几发 |
| --- | --- | --- | --- | --- |
| `nixing_rusheng` | CoreEarly | 96 | 黑雾怪鸟 `magic_mihuan_guniao`（水、`target all`、power 24；CoreEarly / 24） | 4 |
| `nixing_dahan` | CoreEarly | 0 | 无 | —（蓄势 every 3 ×2.0 单体） |
| `wuji_haishou` | CoreEarly | 18 | 海潮诀 `magic_haichao_jue`（水、power 24；FoundationEarly / 18） | 1 |
| `qingse_juxia` | CoreMid | 54 | 海潮诀 | 3 |

境界与法力都够，引擎 `checkCast`（`BattleAction.cpp` 第 131、138 行）放得出。测试路 B 动的 5 个数（儒生气血 480 → 360，海兽法力 54 → 18、攻 34 → 30，巨虾气血 820 → 700、攻 36 → 30）只碰气血、攻、法力，破绽、`killable_by`、境界、`magics`、`actions`、蓄势、编成、波次、`rewards` 均未动（数据实读与 `balance.md`、两张编成的 `note` 一致）。注意：`balance.md` 说「规则 25（敌人法力 ≥ 其法术耗蓝）守得住」——规则 25 只核**友军**的 `maxMp`（`role_means_available`），敌方这一条没有任何门禁或用例钉着（阴性对照 B3、B4），见 G-2。

**五行环不登的理由成立**，而且比 `validate.py` 注释写的更硬：`magic_ji_wuxinghuan` 是 `power 0`、`effect stagger`、`stagger 3`，引擎 `applyStagger` 削架势时「不判破绽、不揭开、不伤人」，与它的五行无关——它既不算一次「金」的破绽打击，也要不了 `killable_by` 含金的儒生的命。登成「金」在两方面都不真。眼下「金」另有祭金符、噬金虫两条真来路，登不登对规则 25 的结果没有影响；但门禁 `learn` 分支不核 `power / effect`，登上去不会被拦（阴性对照 B1、B2b），见 G-1。

## 四、selftest 探针搬家

`tools/validate_selftest.py` 两处改动（`git diff` 逐行核过）：

- `selftest_n_battle_break`（第 1801–1830 行一带）：`outside_role` → `data/roles/feng_xi.json`、`outside_battle` → `b14_fengxi_xiezhi`；先验「`chapter == 14` 且敌方含 `feng_xi`」；`probe["chapter"] = 13`；措辞「第 14 章」「同一风希移入第13章」；必报判据 `outside_battle in error and "feng_xi" in error and "没有一样是这一场" in error`。
- `selftest_ch10_means`（第 2464–2533 行一带）：断言 `MEANS_LAST_CHAPTER == 13`、用例名「末章13」；`future_id = "b14_fengxi_xiezhi"`，引用扫描与 `scoped` 改读 `feng_xi`，必报判据 `future_id in e and "feng_xi" in e and "没有一样是这一场" in e`；`scoped(14)` 应静、`scoped(13)` 应报。

探针的落点稳：`b14_fengxi_xiezhi.json` 与 `feng_xi.json` 自初始导入后没改过（`git log` 只有 84f30df、4e62089 两笔），全仓 `data/battles` 只有这一张含 `feng_xi`，单位只有它一个、`chapter 14`、敌方；`feng_xi` 破绽「金」。第 12 章整改加的两处 `"wuji_haishou" in error` 已一并改成 `"feng_xi"`，第七节 P5 / P5c 证明这两处判据仍有牙：同一场里另塞一个第 13 章打不到的敌人时，没有这一句的旧判据会被别人替它报绿，有这一句才红。

## 五、本章 `tools/` 改动

- `artgen/mapwater.py`：只加 `lotus_of(sc)`（读 `maps.json` 的 `lotus`，缺省 `True`，非布尔当场 `ValueError`）与 `paint_water` 里 `valley / manor` 分支加 `and lotus_of(sc)`；全仓只有 `ch13_wuzhong` 写了 `"lotus": false`（valley 主题）。没写的图走原路径：`artgen --check` 721 个文件逐像素一致，而仓库里已跟踪的 `assets/art/**` 除 `sprites/index.json`（纯追加 1823 行、0 删）外一个没改，说明旧产物逐字节不变。阴性对照 L1（改 `true`）→ 恰 `maps/ch13_wuzhong/below.png` 变；L2（字符串 `"false"`）→ `ValueError` 报出图名。`lotus` 与第 12 章的 `liquid` 一样只由 artgen 读，`validate.py` 与 C++ 都不读，不需要登记。
- `artgen/sprites_enemies.py`：纯追加 `_chapter13_rows` / `_chapter13_beast` / `CHAPTER13_BEASTS`，`BEASTS` 元组只在末尾接 `+ CHAPTER13_BEASTS`；新产物只有 `enemies/qingse_juxia.png`、`enemies/wuji_haishou.png` 两张（未跟踪），旧精灵逐像素一致。
- `mapgen/genmaps.py`：只在 `check()` 里加 `import genmaps_ch13` 与 `built.update(genmaps_ch13.build_all())`，排在第 12 章之后，与前几章同一写法。`maps/` 下已跟踪文件 0 处改动（只多了 4 张图与 4 个 tileset），`genmaps_ch05.py`–`genmaps_ch12.py` 零改动。
- `mapgen/genmaps_ch13.py`（新，390 行）：只从 `genmaps` 取公共构件，`build_all` 纯函数、不导别章生成器、不用随机数，写盘只在非 `--check` 分支；单跑 `--check` 4 张一致。四张图的 `bgm`（`ch13_nanming` / `ch13_waihai` → `bgm_river`、`ch13_tianxing` → `bgm_town`、`ch13_wuzhong` → `bgm_valley`）与 `catalog.py` 注释表、`docs/audio.md` 第 255–258 行逐张一致。
- `audiogen/catalog.py`：只在 `MAP_BGM` 上方注释表加 5 行（一行说明 ＋ 四张图），元组不动；`import catalog` 通过（`MAP_BGM` 仍 14 首）。
- `data/visual/looks.json` 的两处「删除」只是 `ch12_xuangu_lvying`、`xuangu_lvying` 两行补了逗号，旧条目逐条相同、新增 58 条。

## 六、问题清单

**G-1 · LOW · `tools/validate.py` 第 2638–2650 行（`check_means_sources` 的 `learn` 分支）与第 2353–2355 行注释：`learn` 来路不核 `power / effect`，五行环登上去门禁不拦；注释写的理由不准**
- 问题：`learn` 分支只核法术存在、五行、本章起点施展得出（`check_learned_castable`）、`scripts/<章>/` 里真有 `magic.learn`，不核这门法术伤不伤人。友军一支 `role_means_available`（第 2529 行起）要求 `power > 0` 或带毒且无 `effect`，`give` 的五行类 `check_given_spell`（第 2577 行起）同样要求——三条来路口径不一。契约 5.2 自己写明「不查 `power`——正因如此……所以不登」：五行环不登全靠人记。另：注释说登上去会「对 `killable_by` 含金的敌人放水」，可规则 25 判的是 `weaknesses`、不读 `killable_by`；真正的理由是 `applyStagger` 不判破绽、不伤人（`BattleAction.cpp` 第 519–523 行；`data/magics/ji_wuxinghuan.json` 的 note 写得对）。
- 证据（`gate13/vtree`）：B1 把五行环加进 `CHAPTER_MEANS[13]` 的「金」→ 0 错；B2b 儒生改成只怕金、第 13 章的「金」只剩五行环 → 0 错；对照 B2c（同样只怕金、第 13 章不登金）→ 报 `b13_nanming_zhuibing` 的儒生。现状全仓 54 条 `learn` 来路逐条扫过，0 条 `power 0` 或挂 `effect`，所以不挡。
- 改法（归协调者，`validate.py` 那一段）：`learn` 分支补「`power > 0` 或带毒、无 `effect`」，与 `check_given_spell` 同一口径；注释改成「削架势不判破绽、不伤人，不是这一类的一击」；selftest 补一条「五行环登进第 13 章的金 → 必报」。第 14 章若把「同御二十四柄」做成 effect 类，这一条就会用上。

**G-2 · LOW · 敌方「法力 ≥ 法术耗蓝」没有任何门禁或用例钉着；`notes/ch13/balance.md` 第 1 节末句说「规则 25 守得住」属误述**
- 问题：G-34 下敌人有法术合法就只放法术，法力决定放几发；本章海兽法力 18 ＝ 海潮诀 18（恰一发）是平衡路刻意定的数。规则 25 只核**友军**的 `maxMp`（`role_means_available` 第 2546–2549 行），敌方一个不核；规则 24 也不核；`Ch13BattleData` 只断了 `magics` 列表、没断 `maxMp`。法力低于耗蓝时敌人只是永远不放法术、不报错——「一发」会静悄悄变成「零发」，平衡路量出来的回合与余血随之失真。
- 证据：B3 海兽 `maxMp 17` → 0 错；B4 巨虾 `maxMp 0` → 0 错。现数据四个敌人都放得出（第三节表），本审人工核过。
- 改法：并入施工图第 18 节时把那句改成「人工核过」；要钉的话在 `Ch13BattleData` 断四个 role 的 `maxMp` 与法术耗蓝（96 / 24、18 / 18、54 / 18、0 / 无），或在规则 24 加一条警告「敌方 `magics` 每一门都 `needMp > maxMp`（或境界不够）——这些法术永远放不出」。

**G-3 · LOW · `docs/ch13-design.md` 18.3 第 6 条的数表仍是平衡前的内容初值，平衡结果还没并进施工图**
- 问题：18.3 表里儒生 480 / 96、海兽 320 / 54、攻 34、「海潮诀三发」，巨虾 820 / 54、攻 36；数据已是儒生 360、海兽法力 18 攻 30（一发）、巨虾 700 攻 30。18.3 自己注明「内容初值，归平衡路」，平衡结果目前只在两张编成的 `note` 与仓外的 `notes/ch13/balance.md`；`tests-summary.md` 另列了一串「施工图要改」（8.1 / 1.3 / 3.3「约 6 次以内」→ 实测 2 次、BattleHand 不按五行环等）。与第 12 章 G-6 同类，协调者提交时一并做。
- 改法：照 `balance.md` 在第 18 节另起「平衡路」一条（调前 → 调后、三场实测、坐关 N = 2），8.1 / 1.3 / 3.3 的坐关次数以实测为准；BattleHand 不用 effect 类法术建议照测试路 B 的提法登技术债（与 G-36 同类）。

**G-4 · LOW · `docs/interfaces-p3-ch13.md` 1.3 第 8 条、2.3 第 5 条正文仍写「任一文件不存在 `GTEST_SKIP`」**
- 问题：集成时已把 5 处守卫改成 `FAIL()`，测试文件头与实现记录（1.4、2.4、末节）都已改成现状，但判据正文两句没跟上（第 62、102 行）；第 169、249 行是「之前 `GTEST_SKIP`」的施工期叙述，可不动。与第 12 章 G-4 同类。
- 改法：两句改成「任一文件不存在即 `FAIL()`（集成后；施工期为 `GTEST_SKIP`）」。

没有 CRITICAL / HIGH / MEDIUM。上面四条都是「现在的数据是对的，门禁或文档没跟上」。

## 七、阴性对照记录

全部在 scratchpad 副本里做（脚本 `gate13/mutate13.py`、`run.ps1`、`neg_validate13.py`、`probe13.py`、`lotus_neg.py`），每条改完即还原；仓库里除本文件外一个字节没写。

**C++（`gate13/ctree`，仓库 `src/ tests/ CMakeLists.txt` 逐字节复制，其余目录联接；只编 `fanren_tests`，跑 `--gtest_filter=Ch13*`，工作目录是副本根）**

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| base | 不动 | 绿 | 238 步 0 警告；`Ch13*` 82 / 82（E1–E2 27 / 27），0 跳过；全量 `fanren_tests.exe` 2028 / 2028（250 s） |
| A（契约） | `breakthroughBlock` 里 `StoryCap`、`NotEnough` 两句对调 | 第 4 条「再按一次」红 | ✓ 引擎红恰 `ThePanelBreaksThrough…`（第 434 行，报 `NotEnough`）；另有测试路 `Ch13Slice.Five…` 一条同红 |
| C（引擎路自加） | `openFacility` 不论田在不在都新建 | 2.3 第 1–3 条红 | ✓ 恰这三条（`fields` 7 ≠ 6）；另有 `Ch13Slice.Six…` 同红 |
| H（引擎路自加） | 突破后 `hp` 补满 | 第 4 条红 | ✓ 恰这一条（第 422 行，900 ≠ 500） |
| R（本审自加） | 已有的田按这一处 `slots` 改尺寸 | 2.3 第 2 条红 | ✓ 恰 `ADifferentSlotsOnTheSecondMap…`（第 713 行，4 ≠ 6） |
| 还原 | — | 与仓库一致 | 三个被改文件 SHA-256 与仓库相同；还原时先 touch 再编，重建后 82 / 82 |

**规则 25（`gate13/vtree`，复制 `tools / data / scripts / maps / assets / docs / tests/fixtures`，每条跑全量 `validate.py`）**

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| V0 | 不动 | 0 错 | ✓ 0 错 0 警告 |
| V1 | 儒生破绽与 `killable_by` 改「毒」 | 报 ① | ✓ 恰 1 条，点名 `nixing_rusheng` |
| V2 | 大汉同上 | 报 ① | ✓ 恰 1 条 |
| V3 | 海兽破绽改「毒」 | 报 ② | ✓ 2 条（编成里两只） |
| V4 | 巨虾同上 | 报 ② | ✓ 恰 1 条 |
| V5 | V1 ＋ `MEANS_LAST_CHAPTER = 12` | 不报（范围外） | ✓ 0 错——范围上沿真在 13 |
| V6 | 删 `CHAPTER_START[13]` | 报（G-8） | ✓ 7 条：通表 6 条 learn ＋ `b13_waixinghai_lieyao` 的蜂云合剑各报「CHAPTER_START 却没登第 13 章起点」 |
| V7 / V7b | 蜂云合剑 `needMp` 471 / 470 | 报 / 不报 | ✓「第 13 章起点法力上限 470 付不起」/ ✓ 0 错（边界） |
| V8 | 蜂云合剑 `needRealm CoreMid` | 报 | ✓「第 13 章起点 CoreEarly 施展不了」 |
| V9 | 蜂云合剑五行改水 | 报 | ✓「五行不是「木」」 |
| V10 | `chonglian.lua` 去掉 `magic.learn("magic_fengyun_heji")` | 报 | ✓「scripts/ch13/ 里没有 magic.learn」 |
| V11 | `CHAPTER_START[13]` 写成 FoundationLate / 320 | 报 | ✓ 恰蜂云合剑（CoreEarly）一条——表里的起点真被读 |
| B1 | 五行环加进 `CHAPTER_MEANS[13]` 的「金」 | 应报 | ✗ 0 错 → **G-1** |
| B2b | 儒生只怕金 ＋ 第 13 章「金」只剩五行环 | 应报 | ✗ 0 错 → **G-1** |
| B2c | 儒生只怕金 ＋ 第 13 章不登「金」（对照） | 报 | ✓ 报儒生——B2b 的静默确是五行环替它挡了 |
| B3 / B4 | 海兽 `maxMp 17` / 巨虾 `maxMp 0`（低于海潮诀 18） | 应有一处拦 | ✗ 0 错 → **G-2** |
| 还原 | — | — | 8 个被改文件与仓库逐字节相同 |

**selftest 探针（`gate13/ptree_<tag>`：复制 `tools / data / scripts / maps / tests/fixtures` 后在副本里写坏，从副本 import `validate_selftest` 跑 N 段 ＋ CH10 段，先断言 import 到的 `validate_selftest` 与 `validate` 都在副本里）**

| # | 写坏 | 应 FAIL 的用例 | 实际 |
| --- | --- | --- | --- |
| P0 | 不动 | 无 | ✓ 69 条 0 FAIL（N 46 ＋ CH10 23；与 `integration.md` 的阳性对照 69 / 0 一致） |
| P1 | `MEANS_LAST_CHAPTER = 12`（末章改回 12） | 「同一风希移入第13章且只怕毒 → 必报」「第8/9章批准表已接且末章13」「同一风希移入第13章：确实必报」 | ✓ 69 条恰这 3 条 FAIL（与 `integration.md` 的阴性对照 3 FAIL 一致） |
| P2 | `b14_fengxi_xiezhi.chapter` 改 13 | 「第 14 章范围探针实际引用风希」「outside探针实际是第14章且角色只在指定一场」 | ✓ 恰这 2 条。附注：「第 14 章的敌人不在范围 → 不报」此时照样 PASS——探针被拉进第 13 章后风希的「木」打得到，静是对的；挡这种回归的是两条先验，它们都红了 |
| P3 | `feng_xi` 另进 `b13_waixinghai_lieyao` | 「…且角色只在指定一场」 | ✓ CH10 段 23 条恰这 1 条 FAIL |
| P4 | 去掉 `CHAPTER_MEANS[13]`（键改成 1013） | 「同一风希移入第13章：确实必报」 | ✓ 该条 FAIL；同段另 9 条 FAIL 是改键手法的噪声（1013 章的 learn 来路没有起点，`check_means_sources` 底子不净，「还原后来源通过」一类连锁红），不是探针的问题 |
| P5 | 第 13 章通表加「毒」（`pill_shixin_san`，ch03 真有 give）＋ `b14_fengxi_xiezhi` 再塞一个只怕「刀」的敌人 | 两处「必报」（风希此刻不该被报，被报的只有那个刀客） | ✓ 69 条恰这 2 条 FAIL；N0「不动的副本一条不报」仍 PASS（加的两样本身干净） |
| P5c | P5 ＋ 把 selftest 两处 `"feng_xi" in error` / `in e` 删掉（第 12 章整改前的判据） | —（看旧判据会不会被别人替它报绿） | 69 条 0 FAIL：没有那一句，两条「必报」被刀客的报错顶替成绿。P5 / P5c 合起来证明第 12 章 G-5 加的点名判据挪到 `feng_xi` 之后仍有牙 |

结论：探针搬家有效。范围上沿（12 / 13）、探针位置（第 14 章、唯一一场）、表本身缺失、别人替它报四种回归都抓得住；P2 那条 `expect_quiet` 的「静」不单独构成牙，靠先验兜住，可接受。

**`lotus_of`（`gate13/vtree`，`artgen.py --check --map ch13_wuzhong`）**

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| L0 | 不动 | 一致 | ✓ `ARTGEN_IN_SYNC：3 个文件` |
| L1 | `ch13_wuzhong` 的 `lotus` 改 `true` | 有差 | ✓ 恰 `maps/ch13_wuzhong/below.png` 变（荷叶只画在 below 层） |
| L2 | `lotus` 写成字符串 `"false"` | 当场报错 | ✓ `ValueError: data/visual/maps.json 里 ch13_wuzhong 的 lotus 只能是 true/false，写的是 'false'` |

## 八、门禁输出

| 门 | 命令（`fanren/` 下） | 输出 |
| --- | --- | --- |
| validate | `python tools/validate.py`（17:44，18:05 复跑一次相同） | `数据条目 835 项，文案 6046 条，旗标 610 个` / `错误 0，警告 0` / `VALIDATE_OK`，RC 0（8 s） |
| mapgen | `python tools/mapgen/genmaps.py --check` | `MAPGEN_IN_SYNC：79 张图与生成器一字不差`，RC 0；`genmaps_ch13.py --check` 单跑 `4 张`一致 |
| artgen | `python tools/artgen/artgen.py --check` | maps 237、backdrops 76、title 4、sprites 379、fx 25；`ARTGEN_IN_SYNC：721 个文件与生成器逐像素一致（68.5s）`，RC 0 |
| selftest | `python -u tools/validate_selftest.py`（17:34:14 – 18:05:28，31 分 14 秒，与本审的构建、探针副本并行；等完才读） | `用例 413 条，未如期抓住 0 条` / `SELFTEST_OK`，RC 0。与本章相关的 10 条全 PASS：第 14 章的敌人不在规则 25 的范围不报、探针文件及角色存在、实际引用风希、同一风希移入第 13 章且只怕毒必报、第 8/9 章批准表已接且末章 13、outside 探针是第 14 章且角色只在一场、第 14 章只怕毒范围外不报、移入第 13 章确实必报、`CHAPTER_START[13]` 与 `ch12-end-first` / `-second` 一致 |
| 独立 C++ | scratch `gate13/ctree`（仓库 `src/ tests/` 逐字节，238 步 0 警告） | `Ch13*` 82 / 82、E1–E2 27 / 27、0 跳过；全量 `fanren_tests.exe` 2028 / 2028（250 s） |

日志：`scratchpad/gate13/{selftest.log,validate.out,mapgen.out,mapgen13.out,artgen.out,build-*.log,neg-*.txt,run-base.out,run-mut.out,full-tests.txt,neg_validate13.out,neg_validate13_b2.out,probe-P*.out,lotus_neg.out}`（selftest.log 是 GBK，其余 UTF-8）。
