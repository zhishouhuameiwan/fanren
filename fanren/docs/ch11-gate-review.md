# 第 11 章引擎与门禁独立审查

> 2026-10-13 · 只审不改 · 对象：`src/core/rules/Crafting.{h,cpp}`、`src/io/RecipeLoader.{h,cpp}`（E1）、`tests/Ch11EngineTests.cpp`、`tools/validate.py` 规则 25 三张表、`tools/validate_selftest.py` 探针搬家。规范：`docs/interfaces-p3-ch11.md`（契约）、`docs/ch11-design.md`（施工图，含第 18 节偏差）。

## 结论：Approve

没有 CRITICAL / HIGH。E1 的落地与契约 1.2 / 1.3 逐字一致，`craft` 一字未改，加载器能拒收坏形状。`Ch11EngineTests` 的 28 条用例把 1.5、2.2、3.2、4.2、5.2 都钉住了，0 处 `GTEST_SKIP`，与契约的出入都已记进实现记录。规则 25 第 11 章那张表与四场编成、韩立的实际手段逐场对得上；阴性对照 13 条全被抓到。selftest 探针挪到 `wu_chou` + `b12_neidian_duoding` 后有效。下面 G-1 是门禁本来就有的盲区，G-2 到 G-8 是小事，都不挡合入。

### 本审实跑

| 项 | 结果 |
| --- | --- |
| `python tools/validate.py` | `VALIDATE_OK`，4.5 s |
| `python tools/validate_selftest.py` | `SELFTEST_OK`：363 PASS、0 FAIL，「探针漏抓 0 条」，16 分 10 秒。第 11 / 12 章相关的 8 条全 PASS：第 12 章范围不报、探针文件存在、引用乌丑、乌丑移入第 11 章只怕毒必报、末章 11、outside 是第 12 章且乌丑只在一场、`scoped(12)` 不报、`scoped(11)` 必报 |
| `build-ch11walk/fanren_tests.exe --gtest_filter=Ch11Guaranteed*:Ch11ForgePanel*:Ch11Fixture*:Ch11TrappedCorpse*:Ch11BuyOnlyShop*:Ch11CoreEarly*` | 28 / 28 PASSED，RC=0。exe 的时间 01:24，晚于四个 src 文件（22:10）和测试文件（22:57） |
| `build-ch11walk.log`（01:49，按 gb18030 读） | `VALIDATE_OK` / `MAPGEN_IN_SYNC` / `SELFTEST_OK` / `ARTGEN_IN_SYNC`；`100% tests passed, 0 tests failed out of 1851`；`EXITCODE=0` |

### E1 对照契约第 1 节

- **1.2 规则层**：`Recipe::guaranteed = false` 加在末尾，聚合初始化不受影响。另有 `kGuaranteedSuccessChance = 100` 和 `static_assert(> kMaxSuccessChance)`。`successChance` 的第一句就是早退，在四个 `std::clamp` 之前。第 84 行注释和声明注释都补了「例外只有 guaranteed」。`craft`、`canCraft`、`proficiencyGain`、`failurePolicyOf`、`planConsumption` 都没动。`craft` 掷 `uniform_int_distribution(1,100)`，判定是 `<= chance`，chance 是 100 时失败分支确实走不到。`successChance` 在 src 里只有 `AlchemyScene.cpp:122` 一处调用（只显示数字），没有别处假设它不超过 95。
- **1.3 加载器**：先 `contains` 再判 `is_boolean`，`"yes"`、`1`、`null` 都会被拒，报错写「guaranteed」和路径；缺省读成 false；`guaranteed && requireFlag.empty()` 被拒，报错写「requireFlag」。`requireFlag` 是空串的情况在前一段已经拒掉。
- **内存安全**：`root["guaranteed"]` 前面都有 `contains` 守着，和既有的 `readBool` / `presentButNotInt` 是同一写法。没有新的分配、指针或下标。`successChance` 仍是 `noexcept`。
- **数据**：`data/recipes/forge/qingzhu_fengyun_jian.json` 符合 1.4 的形状：`guaranteed true`、`requireFlag ch11.liugen`、金雷竹 6、辅料 1、没有灵石。全仓 `guaranteed` 为真的集合就是 `{recipe_qingzhu_fengyun_jian}`：`Ch11ForgePanel` 断言它为真，`TheWholeRepository…` 断言集合不超过它，两条合起来就是「恰好这一张」。

### 判据覆盖（`tests/Ch11EngineTests.cpp`）

| 契约 | 覆盖 | 备注 |
| --- | --- | --- |
| 1.5 第 1–7 条 | 全覆盖 | 第 1 条比的是常量，另有三组极端入参，证明早退在 clamp 之前；第 3 条对照组用 `failures > 0` 防止空转，失败那次金雷竹恰 3、辅料恰 1；第 4 条在契约三条之外又加了 `1`、`null`、`false 无门闸放行`、正向形状 |
| 1.5 第 8 条（负向自检） | 只有记录，证据不全 | 见 G-5 |
| 2.2 第 1–6 条 | 全覆盖 | 探针脚本挂在临时 scriptRoot，这个偏差已记在 2.3 |
| 3.2 第 1–4 条 | 全覆盖 | 先断言伤害不小于 10，免得「压到 1」靠伤害不够来成立 |
| 4.2 第 1–5 条 | 第 4 条换了等价判据 | 已记在 4.3（`ShopScene` 的主菜单是 private） |
| 5.2 第 1–7 条 | 全覆盖 | 第 5 条面板只按子串比，见 G-6；`bankMeditation` 用 `x > 0` 防止 0/0 空转 |

### 规则 25：四场逐一核对（起点 `ch10-end-*.sav` 实读）

韩立起点 `realm 23`、`maxMp 320`。`learnedMagics` 有火弹术（火，needMp 8）、流沙术（土，12）、冰冻术（水，10）、祭金符（金，12）、青元剑芒（木，needRealm FoundationEarly，10）。背包里有 `weapon_lieyang_jian`（剑）和 `weapon_wuming_sixian`（暗器，`tradeable false`）。n16 之后 `realm 31`、`maxMp 470`。

| 场次 | 敌人破绽 | 这一场打得到的手段 | 结论 |
| --- | --- | --- | --- |
| ① `b11_ningcuidao_youyao`（n12 前后，23） | 千目妖：土、剑；四级海兽 ×2：土、暗器 | 流沙术（土）、烈阳剑（剑）、无名丝线（暗器） | ✓，每个敌人至少两样 |
| ② `b11_honghu_jiguanjiao`（n13，23） | 鸡冠蛟：只有金 | 祭金符（needRealm QiRefining1，needMp 12 ≤ 320）；曲魂的金剑引擎里打得出，但门禁看不到它，见 G-4 | ✓，**本章唯一只有一样破绽的一场** |
| ③ `b11_yinsha_shayao`（n16 之后，31） | 煞妖 ×4：剑、木 | 烈阳剑；青元剑芒（FoundationEarly ≤ 31） | ✓ |
| ④ `b11_tiandu_shi`（31） | 妖尸：水、火（`killable_by` 也是这两样） | 冰冻术、火弹术；友军吐网（水，needRealm FoundationLate = 蜘蛛的境界，needMp 20 ≤ 240）；紫火（火，FoundationMid = 猿傀的境界，18 ≤ 200） | ✓，通表和友军两条来路都有 |

`BATTLE_EXTRA_MEANS` 只给 `b11_tiandu_shi` 登了两条友军 `role_magic`，曲魂没登。这和契约 8.2 不一样，但施工图 18.4 第 1 条写明了理由：`role_means_available` 只认编成里写死的友军。这次偏离站得住，但契约正文没跟着改，见 G-4。`MEANS_LAST_CHAPTER = 11`，第 12 章的 `b12_neidian_duoding` 不在范围内。

## 问题清单

**G-1 · MEDIUM · `tools/validate.py` 的 `check_means_sources`（learn 分支）：韩立自己学的法术，`needRealm` 和 `needMp` 只有第 10 章核过**
- 问题：契约 8.2 的前提「青元剑芒……筑基后期与结丹初期都不低于它的 needRealm」，门禁并不查。`owner == "第 10 章"` 那个分支是唯一核 needRealm 的地方。needMp 和韩立 maxMp 的比较，任何一章都不查。友军走的 `role_means_available` 却两样都查，两边口径不一致。
- 证据：在临时副本里把 `qingyuan_jianmang.json` 的 needRealm 改成 `CoreMid`（韩立全章是 23 / 31，施展不了），全量 validate 报 0 错；把 `ji_jinfu.json` 的 needMp 改成 999（韩立 maxMp 是 320 / 470），也是 0 错。② 那一场唯一的破绽「金」就压在祭金符上，这一处一旦改坏，门禁会照样放行。
- 改法（归协调者）：在 `CHAPTER_MEANS` 旁边加一张「本章韩立最低境界 / 最低 maxMp」表，第 11 章写 `FoundationLate` / 320；learn 分支统一核 needRealm 和 needMp，把第 10 章的特判并进来。selftest 补两条负向（needRealm 拔高、needMp 拔高必须报）。

**G-2 · LOW · `tools/validate_selftest.py` 第 1798–1828 行与第 2502–2527 行：「必报」只按编成 id 加措辞匹配，没指名乌丑**
- 问题：`any(outside_battle in error and "没有一样是这一场" in error …)` 和 `scoped()` 的过滤都不要求报错里出现 `wu_chou`。`b12_neidian_duoding` 里还有一个敌人 `duoding_xiushi`，它现在只怕水，第 11 章打得到，所以探针眼下没空转。可哪天它的破绽改成第 11 章打不到的，乌丑这条就会白绿。
- 证据：临时副本里乌丑不动，只把 `duoding_xiushi` 改成只怕毒，再把编成移入第 11 章：宽匹配命中 1 条，按 `wu_chou` 严格匹配命中 0 条。
- 改法：两处判据都加上 `"wu_chou" in error`。

**G-3 · LOW · `tools/validate.py` 的 `CHAPTER_MEANS[11]`「剑」：靠的是能卖掉的烈阳剑**
- 问题：`weapon_lieyang_jian` 是 `tradeable true`、价 60，第 6 章起任何收购店都能卖。本章青竹小轩（0.6）和凝翠岛收购行（0.8）也收。`give` 来路不查 `tradeable`，「必有」这个说法就宽了。暗器的无名丝线是 `tradeable false`，没有这个问题。
- 影响：本章没有一场只靠「剑」：① 还有土，③ 还有木，所以不会卡死。第 12 章以后哪个敌人要是只怕剑，就会漏过去。
- 改法：二选一。`give` 兵刃来路要求 `tradeable false`（会牵动前几章的表，先跑 selftest）；或者等 `role_means_available` 能认队员以后，改由曲魂的「剑」或青竹蜂云剑（`tradeable false`）来担保。

**G-4 · LOW · 契约 `docs/interfaces-p3-ch11.md` 8.2：曲魂四条不登，契约正文没跟着改**
- 问题：施工图 18.4 第 1 条写了偏离理由，理由站得住，但契约 8.2 还写着「四张各登 `qu_hun_shadan` 的……」。契约头部定的规矩是「真要改，先改本文」。另外，门禁看不到队员，② 的「金」在门禁眼里只有祭金符这一条来路（G-1 那处盲区因此更要紧）。
- 改法（归协调者）：8.2 那一句改成「不登，理由见施工图 18.4-1」；`role_means_available` 认队员这一条进 `docs/tech-debt.md`。

**G-5 · LOW · 引擎路 1.5 第 8 条负向自检：日志里没有测试结果**
- 问题：`build-ch11eng-neg-{A,B,C,D}.log`（22:08–22:10，各 300–450 字节）只有增量编译输出和 `BUILD_RC=0`。契约 1.6 写的「A 3 红 / 25 绿」「D 14 红 / 14 绿」，从产物里核不出来。
- 本审的佐证：改坏的时候编译确实只碰了 `Crafting.cpp` / `RecipeLoader.cpp`，说明改坏的位置和契约说的一致。用例写法上，第 1 条比的是常量、第 3 条用 `failures > 0`、第 4 条判报错子串，这三处负向在逻辑上会红。
- 改法：以后负向自检要把 `fanren_tests.exe --gtest_filter=…` 的输出也写进同一份日志。

**G-6 · LOW · `tests/Ch11EngineTests.cpp` 第 733 行（5.2 第 5 条）：面板加成只按子串「25」比**
- 问题：`siteBonusLine(state).find("25")` 一碰到「125」或别的数字就会误中。面板交给 `bankMeditation` 的 sitePercent 到底是不是 125，这一条钉得不实；`bankMeditation` 那一半是直接传 125 进去的。
- 改法：写一行字面量全文比对（判据不从被测物推导），或者给 `CultivationScene` 加一个只读 `sitePercent()`（白名单外，协调者定）。

**G-7 · LOW · 白名单外的旧 bug：`src/game/Application.cpp` 第 561–577 行 `continueJourney` 读档后不恢复朝向**
- 证据：它只把 `position` 放回去。`loadMap` 第 323–327 行会用出生点的 `facing` 覆盖 `state_.facing`。测试路 B 已经绕过去了：章末落点选了一格朝向和出生点一致的。这不是本章的 diff。
- 改法：在 `state_.position = at;` 旁边把 `facing` 一起存下、再放回；登进 `docs/tech-debt.md`。

**G-8 · LOW · `tools/validate.py` 第 2332 行注释写「n17 结丹」，应为 n16**
- 契约 5.1 和施工图第 3 节都是 n16 `biguan.lua`。注释改一个字就行。

## 阴性对照记录

方法：临时目录复制 `maps/` `scripts/` `data/`，在副本上跑真 `validate.main()`（全量）；表的改动只在进程内做 monkeypatch，仓库一个字节没动。脚本在本会话的 scratchpad（`negctl.py`），跑完就删了副本。原样副本基线：0 错。

| # | 改坏 | 期望 | 实际 |
| --- | --- | --- | --- |
| N1 | 鸡冠蛟破绽 金 → 毒（② 唯一一样） | 报 | ✓ 报 `b11_honghu_jiguanjiao` / `jiguan_jiao`「没有一样是这一场」 |
| N2 | 鸡冠蛟破绽 金 → 刀（背包里有冷月刀，但不在通表里） | 报 | ✓ |
| N3 | 通表删掉「金」 | 报 | ✓（只有这 1 条错：金的来路就这一条） |
| N4 | 祭金符五行 金 → 木 | 报 | ✓ 来源检查「五行不是金」 |
| N5 | `MEANS_LAST_CHAPTER` 退回 10，鸡冠蛟只怕毒 | 不报 | ✓ 0 条命中（证明是范围在起作用） |
| N6 | 妖尸破绽 水火 → 毒 | 报 | ✓ |
| N7 | 妖尸只剩水，且通表删掉水 | 不报 | ✓（友军吐网顶上） |
| N8 | 千目妖 土剑 → 毒 | 报 | ✓ |
| N9 | 四级海兽 土暗器 → 毒 | 报 | ✓ 两只都报 |
| N10 | 煞妖 剑木 → 毒 | 报 | ✓ 四只都报 |
| N11 | 煞妖只剩木，且通表删掉木 | 报 | ✓ |
| N12 | 猿傀 maxMp 改成 10（付不起紫火） | 报 | ✓ 来源检查报 `magic_kuilei_zihuo` |
| N13 | 血玉蜘蛛境界降到筑基中期（吐网要筑基后期） | 报 | ✓ 来源检查报 `magic_zhizhu_tuwang` |
| GAP1 | 青元剑芒 needRealm 改成 CoreMid | 应报 | ✗ 0 错，记为 G-1 |
| GAP2 | 祭金符 needMp 改成 999 | 应报 | ✗ 0 错，记为 G-1 |
| PROBE | 乌丑不动，只把同场 `duoding_xiushi` 改成只怕毒，移入第 11 章 | 严格应不中 | 宽匹配中 1、严格中 0，记为 G-2 |

selftest 那一侧：`第 12 章只怕毒：范围外确实不报`、`同一乌丑移入第11章：确实必报` 都 PASS。加上 N1 / N3 / N5 这一组，可以确认：只要把本章某场唯一能打的破绽改掉，门禁就会报；把范围退回第 10 章，就不报。
