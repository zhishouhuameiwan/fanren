# 第 12 章引擎与门禁独立审查

> 2026-10-13 · 只审不改 · 对象：`src/`（本章应为零改动）、`tests/Ch12EngineTests.cpp`（E1 空队伍、E2 同一 role 先友后敌）、`tools/validate.py` 规则 25 三张表、`tools/validate_selftest.py` 探针搬家、本章 `tools/` 改动（`mapgen/genmaps.py`、`mapgen/genmaps_ch12.py`、`artgen/sprites_enemies.py`、`audiogen/catalog.py`）。规范：`docs/interfaces-p3-ch12.md`（契约）、`docs/ch12-design.md`（施工图，含第 18 节偏差）、协调者笔记 `notes/ch12/{coordinator-notes,integration,tests-summary,balance}.md`。
> 阴性对照全部在 scratchpad 的临时副本里做（`gate12/ctree` C++ 副本、`gate12/vtree` 与 `ptree_*` 门禁副本），仓库里只写了本文件。

## 结论：Approve

没有 CRITICAL / HIGH。`src/` 零改动，与契约「本章 0 个代码项」一致。`Ch12EngineTests` 的 23 条在独立构建上 23 / 23 绿、0 跳过，E1 第 1–6 条、E2 第 1–6 条都有对应用例，引擎路交付的四次负向自检（A–D）日志与契约预期逐条对得上。规则 25 第 12 章的表与四场编成、韩立和友军的实际手段逐场核过，全部打得到；测试路 B 把 `xuan_gu` 法力 860 → 96 之后仍 ≥ 魔火耗蓝 12，门禁在 11 / 12 这条边界上有牙（阴性对照 V5 / V5b）。selftest 探针挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao` 后有效（P1–P4 四种写坏全被抓）。四门全过。

下面三条 MEDIUM 都是「现在的数据是对的，但判据 / 门禁没牙」：G-1 是契约 2.1 明说平衡路倚仗的「友军照样两手」没有任何一条用例钉着（实证：把它改坏，全量 1944 条照样全绿）；G-2 是本章第一次用 `give` 一件施法符登记五行类别，门禁不核那件符打的是什么；G-3 是第 11 章留下的 G-29 盲区，本章 ③④ 两个首领都只有一样破绽，其来路恰好都落在这两处盲区里。都不挡合入。

### 本审实跑

| 项 | 结果 |
| --- | --- |
| `git diff -- fanren/src`、`git status -- fanren/src fanren/CMakeLists.txt` | 空：`src/` 与 CMake 零改动 |
| `python tools/validate.py` | `VALIDATE_OK`：776 项、5549 条文案、556 个旗标，0 错 0 警告 |
| `python tools/mapgen/genmaps.py --check` | `MAPGEN_IN_SYNC`：75 张；`genmaps_ch12.py --check` 单跑 10 张一致 |
| `python tools/artgen/artgen.py --check` | `ARTGEN_IN_SYNC`：678 个文件逐像素一致（59.8 s） |
| `python tools/validate_selftest.py` | 见文末「门禁输出」 |
| 独立 C++ 构建（scratch 副本 `gate12/ctree`，`src/ tests/ CMakeLists.txt` 照仓库复制，`vendor` 等目录联接；构建 233 步、0 警告） | `--gtest_filter=Ch12Fixture.*:Ch12TwoSided.*:Ch12AllyKillable.*:Ch12KillableSubset.*:Ch12AlliesCastMetal.*:Ch12RealFriendFoe.*` 23 / 23 PASSED、0 跳过；`Ch12*` 82 / 82 PASSED |

## 一、`src/` 与契约 E 项

契约头部：「本章 0 个代码项……`src/` 一行不改」。核实：`git diff -- fanren/src` 为空、`fanren/src` 下没有未跟踪文件，CMake 未动（测试按 `tests/*.cpp` 通配收进来）。E1、E2 都是验证项，下面只看判据。

顺带核了契约第 0、2 节引用的几处引擎口径，与代码一致：`unitFromRole`（`BattleScene.cpp` 第 356–384 行）把 `magics / weapons / actions` 放在 `if (!ally)` 之外、把架势 / 破绽 / 已揭 / 蓄势 / `killableBy` 放在之内；`settle` 只在 `heroPresent_` 时写回韩立与在队同伴的气血，破绽只记 `!unit.ally`（第 995–1009 行）；`enemyDecision`（`BattleAi.cpp` 第 56–90 行）对每个目标「有伤人法术且合法就在法术里挑，否则普攻」——测试路 B 把玄骨法力压到 96 的理由属实。

## 二、判据覆盖（`tests/Ch12EngineTests.cpp`）

| 契约 | 覆盖 | 有没有牙 |
| --- | --- | --- |
| 1.3-1 离队 | `RemovingQuHun…` | 有：旗标先写哨兵 99，第二遍即负向；负向自检 A（`PartyRemove` 恒 ok）只红这一条 |
| 1.3-2 空队伍进仗 | `AnEmptyParty…`、`PuttingQuHunBack…` | 有：5 名 → 放回曲魂 6 名 → `active false` 回 5 名 |
| 1.3-3 存读档 | 两条 | 有：空队伍 A / B 字节相同；对照组 `hp 345` 往返不变且与空存档字节不同 |
| 1.3-4 菜单 | 两条 | 比契约多一层：显式实例化探针读出私有 `members_`，钉「恰 1 行 hanli」；负向自检 B（去掉 `role == nullptr` 跳过）→ 访问违例被收成红 |
| 1.3-5 四场真编成 | 四条 `TEST_F` | 有：`heroAbsent false`、ally 多重集合、空队伍建场 ally ＝ 1 ＋ n、无曲魂 |
| 2.3-1 先例两边 | `AsAnAlly…`、`AsAnEnemy…` | 架势 / 破绽 / 已揭 / 蓄势 / `killableBy` 有牙（负向自检 C、D 都红）；**`actions` 一项没牙，见 G-1** |
| 2.3-2 友军打得死 | `Ch12AllyKillable` 两条 | 有：先验敌方先手、伤害 ≥ 10；`killableBy 0` → 0 血，「木」→ 压在 1 |
| 2.3-3 破绽记账 | `KnownWeaknesses…` | 有：学全破绽后敌方亮、友军仍 0 |
| 2.3-4 真数据 | 四条 | 有：`xuan_gu` 木 / 2 手 / 每 3 / 只魔火；五个带 `killable_by` 的敌人恰契约那五个、子集为真，内存加一位必报；③ 两名友军四样敌方字段为 0，银形恰「金」；④ 玄骨是唯一敌人。**③ 里不断友军的 `actions`，见 G-1** |
| 2.3-5 友军不配金 | 真数据一条 ＋ 手造一条 | 有：空 → 塞祭金符恰报那一门；做敌方不报 |
| 1.3-6 / 2.3-6 负向自检 | `build-ch12eng-neg-{A,B,C,D}.log`（04:41，按 gb18030 读） | A：1 红 22 绿；B：1 红；C：3 红（③ 真场景、友军先例、破绽记账）；D：2 红。与契约预期一致 |

协调者集成时把 11 处 `GTEST_SKIP()` 改成 `FAIL()`（`notes/ch12/integration.md`），合理：数据齐了，缺了就该红。但文件头与几段注释还写着「skip」，见 G-4。

## 三、规则 25：四场逐一核对

起点从 `tests/fixtures/ch11-end-{first,second}.sav` 实读：`realm 31`（CoreEarly）、`maxMp 470`；`learnedMagics` 含火弹术（火，Q1，8）、流沙术（土，Q1，12）、冰冻术（水，Q1，10）、祭金符（金，Q1，12）、青元剑芒（木，FoundationEarly，10）；背包有 `weapon_lieyang_jian`（剑）、`weapon_wuming_sixian`（暗器）。噬金虫群（金，FoundationLate，16）在 n03 `guairen.lua` 无条件 `magic.learn`（抉择之后、`flag.set("ch12.tuizhu")` 之前，`portal_dixue` 要这面旗，必经）；`ch12-end-*.sav` 的 `learnedMagics` 确有它。本章 `scripts/ch12/` 的 `take(` 只扣灵石、白珍珠、画轴、神雷余数，不碰兵刃；本章 0 店。

| 场次（节点） | 敌人：破绽（`killable_by`） | 这一场打得到的手段 | 结论 |
| --- | --- | --- | --- |
| ① `b12_guiwu_guiwang`（n13） | 炜吾 ×2：火、剑；鬼夜叉：木、火；灰影：金、火；双头怪虎：剑、木；三首鬼物：木、火（木、火） | 火弹术、烈阳剑、青元剑芒、祭金符、噬金虫、神雷（木）；友军紫灵剑（`weapons ["剑"]`）、黑袍元瑶魔火（CoreEarly ≥ FoundationEarly，12 ≤ 470） | ✓ 每个敌人至少两样 |
| ② `b12_heishamo_tiehuoyi`（n21） | 铁火蚁群 ×5、大群：水、金（水、金） | 冰冻术（水）、祭金符与噬金虫（金）；友军青火雷（火，CoreEarly ≥ CoreEarly，24 ≤ 470）打不到破绽，登记无害 | ✓ |
| ③ `b12_neidian_duoding`（n29） | 双头火形：水、金；独角银形：金（金） | 冰冻术、祭金符、噬金虫；友军乌丑冰箭（水，CoreMid ≥ Q1，7 ≤ 600）；玄骨魔火（火，CoreLate ≥ FoundationEarly，12 ≤ **96**）打不到破绽 | ✓ 银形**只有金**，来路只有韩立自学的两门（G-3） |
| ④ `b12_hanlitai_xuangu`（n31） | 玄骨：木（木） | 青元剑芒（木）、神雷（`castMagic magic_bixie_shenlei`：木、power 70、needMp 0、Q1） | ✓ 玄骨**只有木**，两条来路分别落在 G-3 与 G-2 的盲区 |

友军五条来路逐条按 `role_means_available` 复核为真（编成里是 ally、法术在 `magics`、五行 / `power > 0` / 无 `effect` / 境界 / `needMp ≤ maxMp`）；②③ 那两条火打不到任何破绽，只是让表与编成对齐。`MEANS_LAST_CHAPTER = 12`；第 13 章 `b13_waixinghai_lieyao` 不在范围内。

**测试路 B 改的 10 个敌方 role**：只动了气血 / 攻 / 防 / 架势与 `xuan_gu.maxMp`，破绽、`killable_by`、境界、`magics`、`actions`、蓄势都没动（`balance.md` 与四张编成 `note` 逐条核对；数据实读一致）。规则 24 只要架势 ≥ 1（调后最低 3）；规则 25 只读破绽与友军的 `maxMp`：玄骨 96 ≥ 魔火 12，能放八手。阴性对照 V5：改成 11 → 门禁报「友军 xuan_gu 的「火」来路 magic_moyan_huo 不在编成或不可施展」；V5b：改成 12 → 不报。第 12 章没有野外遭遇表，规则 27 的「凶」不涉及。没有别的门禁读这几项数值。

## 四、本章 `tools/` 改动

- `mapgen/genmaps.py`：只在 `check()` 里加 `import genmaps_ch12` 与 `built.update(genmaps_ch12.build_all())`，排在第 11 章之后，与第 10、11 章同一写法；`genmaps_ch05.py`–`genmaps_ch11.py` 零改动，`maps/` 下除 10 张新图与 10 个新 tileset 外无改动。`--check` 75 张一致。
- `mapgen/genmaps_ch12.py`（新）：只读 `genmaps` 的公共构件，写盘只在非 `--check` 分支；`--check` 单跑 10 张一致。十张图的 `bgm` 属性与 `catalog.py` 注释表、`docs/audio.md` 第 12 章表逐张一致。
- `artgen/sprites_enemies.py`：纯追加 `_chapter12_rows` / `_chapter12_beast` / `CHAPTER12_BEASTS`（9 个非人形），`BEASTS` 元组只在末尾接上；旧 678 个产物逐像素一致，说明没有回归。`looks.json` 删的只有 `duoding_xiushi`、`xutian_hudian_kuilei` 两条占位（`ch11_jin_qing` / `jin_qing` 两行只是逗号位置变了，仍在）。
- `audiogen/catalog.py`：只在 `MAP_BGM` 上方注释表加 10 行，元组不动；`import catalog` 通过（第 58 行断言不炸）。
- 已知、不计入：冰火道熔岩被画成水（施工图 18.2 第 9 条、`tests-summary.md`「整改轮要做」），不是回归。

## 五、问题清单

**G-1 · MEDIUM · `tests/Ch12EngineTests.cpp` 第 469–481 行与第 701–718 行：契约 2.1「`actions` 对友军照样生效」没有一条有牙的判据**
- 问题：契约 2.1 写明「③ 里玩家一回合能让玄骨动两手，平衡路按此量」，2.3 第 1 条用 `xuan_le` 断 `unit.actions == role.actions`。可是 `xuan_le.json` 的 `actions` 是 1，恰等于 `Unit::actions` 的默认值（`core/battle/Battle.h` 第 135 行）：把 `BattleScene.cpp` 第 372 行 `unit.actions = role.actions;` 挪进 `if (!ally)`，这一断言照样成立。③ 的真场景用例只断友军的 `killableBy / maxToughness / weaknesses / chargeEvery`，不断 `actions`。契约 2.3 第 6 条的负向自检也没列这一处。
- 证据（scratch 副本，仓库未动）：变异 E＝把第 372 行挪进守卫 → 增量构建 → `Ch12*` 82 / 82 绿，**全量 1944 / 1944 绿**（210 s）。在副本的 ③ 用例里补两句（循环里 `EXPECT_EQ(unit->actions, app_.data().roles.at(id).actions)`，循环外 `EXPECT_EQ(xuan_gu 的 actions, 2)`）→ 现状下绿；变异 E 下红，报「Which is: 1 / 2」。还原后与仓库 `BattleScene.cpp` 哈希相同。
- 改法（归引擎路白名单）：把上面两句写进 `InTheNeidianXuanGuAndWuChouAreAllies…`；契约 2.3 第 4 条补「友军 `actions == role.actions`，玄骨恰 2」，第 6 条补「E：`actions` 挪进守卫 → ③ 真场景红」。先例 `xuan_le` 那一句可留，但要注明它在 1 上是空转。

**G-2 · MEDIUM · `tools/validate.py` 第 2573–2587 行（`check_means_sources` 的 `give` 分支）与第 2397、2403 行：用施法符登记五行类别，门禁不核那件符打的是什么**
- 问题：本章是第一次在 `BATTLE_EXTRA_MEANS` 里用 `give` 登记一个五行类别（`("木","give","talisman_bixie_shenlei","ch12")`，①④ 两场）。`give` 分支只核三件事：物品存在、「毒」类真带毒、兵刃类的 `weapon` 字段对得上；五行类别什么都不核——不看 `castMagic` 指向哪门法术、那门法术是什么五行、`power` 是否大于 0、有没有 `effect`。
- 证据：副本里把 `bixie_shenlei.json` 的 `castMagic` 改成 `magic_huodan_shu`（火）→ 全量 validate 0 错；把 `magic_bixie_shenlei` 的 `element` 改成 8（火）→ 0 错（对照 B1、B1b）。现数据是对的（木、power 70、needMp 0、Q1，人工核过）。
- 影响：①里每个敌人另有别的来路，神雷这条只是冗余；但 ④ 玄骨只有「木」一样破绽，两条来路一条是神雷（本条），一条是青元剑芒（落在 G-3）。两处一起写坏，门禁照样全绿。运行期测试只覆盖一半：测试路 B 的意图手不会用单体 `castMagic` 道具，神雷在四场里从没被量过。
- 改法（归协调者，`validate.py` 那一段）：`give` 来路的类别若属 `ELEMENT_CATEGORIES`，要求物品有 `castMagic`、那门法术存在、五行含该类、`power > 0` 或带毒、无 `effect`；selftest 补一条「神雷 `castMagic` 改成火弹术 → 必报」。

**G-3 · MEDIUM（沿用 `docs/tech-debt.md` G-29）· `tools/validate.py` 第 2557–2572 行（`learn` 分支）：韩立自学法术的 `needRealm` / `needMp` 仍只有第 10 章特判核**
- 问题：第 11 章审查 G-1 已记成技术债 G-29。本章分量更重：③ 银形只怕「金」，来路只有祭金符、噬金虫两门自学法术；④ 玄骨只怕「木」，来路之一是青元剑芒。三门都是 `learn`，境界与法力门禁一概不核。
- 证据：B2 两门金术 `needRealm` 拔到 CoreMid（韩立 CoreEarly 施展不了）→ 只报第 10 章那一条特判，第 12 章 0 错；B3 两门金术 `needMp` 拔到 999（韩立 470）→ 0 错；B4 青元剑芒 `needRealm` 拔到 CoreLate → 0 错。现数据都施展得出（Q1 / FoundationLate / FoundationEarly，12 / 16 / 10）。
- 改法：照 G-29 原定修法，第 12 章起点写 `CoreEarly` / 470；selftest 配「噬金虫 needMp 超出 → 必报」。

**G-4 · LOW · `tests/Ch12EngineTests.cpp` 第 1–4、358–360、616–618 行：注释还说「落地前 skip」**
- 问题：协调者已把 11 处 `GTEST_SKIP()` 改成 `FAIL()`，注释仍写「Cases … skip until those files land」「Each skips until」「Skips until」；契约 1.3 第 2、5 条与 2.3 第 4、5 条正文、1.4 / 2.4「跳过条件」也还写 `GTEST_SKIP`。读者会以为缺文件时这些用例会静悄悄跳过。
- 改法：注释改成「缺了就 FAIL（集成后）」；契约实现记录补一句「集成时 SKIP → FAIL」。

**G-5 · LOW（沿用第 11 章审查 G-2）· `tools/validate_selftest.py` 第 1825 行与第 2522 行：「必报」只按编成 id ＋ 措辞匹配，不要求报错里出现 `wuji_haishou`**
- 问题：眼下 `b13_waixinghai_lieyao` 只有两只 `wuji_haishou`，探针没空转（P1–P4 都抓得住）。第 13 章若往这张编成里加一个第 12 章手段打不到的敌人，探针就会因为别人而绿。
- 改法：两处判据各加 `"wuji_haishou" in error`。

**G-6 · LOW · `docs/ch12-design.md` 18.3 第 2、3 条：数值仍是平衡前的**
- 问题：18.3 写玄骨 1400 / 860、攻防 88 / 64、架势 8，三首架势 8、蚁群 4、大群 7；数据已是 450 / 96、36 / 40、4，6、3、3。法力 96 的裁定（`tests-summary.md`「写入施工图 18 节」）也还没进第 18 节。协调者已把「按 balance.md 并入第 18 节」列为待办，这里只是确认它还没落。
- 改法：照 `balance.md` 并入；契约 2.2 可补一句「`maxMp` 由平衡路定，须 ≥ 魔火 12（规则 25）」。

**G-7 · LOW · 测试路 B 交出的三条引擎观察没进 `docs/tech-debt.md`**
- 问题：(a) `enemyDecision`（`BattleAi.cpp` 第 65–89 行）只要有一门伤人法术合法就不看普攻，弱法术 ＋ 大法力的首领会被废掉，玄骨是靠把法力压到 96 绕过去的，下一个施法首领还会踩；(b) `settle`（`BattleScene.cpp` 第 995–1003 行）只写回气血、不写回法力；(c) `tests/BattleHand.h` 不会用单体 `castMagic` 道具，神雷这类手段运行期无人量。三条都不是本章改的，也不挡本章，但 `grep` 技术债文档 0 处。
- 改法：协调者各记一条技术债（(a) 给出「法术期望伤害 ≥ 普攻才放」之类的修法方向，(c) 与 G-2 一并）。

## 六、阴性对照记录

全部在 scratchpad 副本里做（脚本 `gate12/mutate_cpp.py`、`gate12/neg_validate.py`、`gate12/neg_probe.py`），每条改完即还原；仓库里除本文件外一个字节没写。

**C++（`gate12/ctree`，照仓库复制 `src/ tests/ CMakeLists.txt`，`vendor / data / maps / scripts / assets / docs / tools / saves` 目录联接，MSVC Release）**

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| base | 不动 | 绿 | 构建 233 步 0 警告；Ch12 引擎 23 / 23，`Ch12*` 82 / 82 |
| E | `BattleScene.cpp` 第 372 行 `unit.actions = role.actions;` 挪进 `if (!ally)` | 应有红 | ✗ `Ch12*` 82 / 82 绿，全量 1944 / 1944 绿 → **G-1** |
| E＋补丁 | 同上，③ 用例补两句 `actions` 断言 | 红 | ✓ 1 条红（「Which is: 1」对「2」）；不变异时同一补丁绿 |
| 还原 | — | 与仓库一致 | `BattleScene.cpp` 哈希相同，重建后 `Ch12*` 82 / 82 |

**规则 25（`gate12/vtree`，复制 `tools / data / scripts / maps / assets`，跑全量 `validate.py`）**

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| V0 | 不动 | 0 错 | ✓ 0 错 |
| V1 | 银形破绽与 `killable_by` 改成「毒」 | 报 ③ | ✓ |
| V2 | 铁火蚁大群改成「刀」 | 报 ② | ✓ |
| V3 | 三首鬼物改成「毒」 | 报 ① | ✓ |
| V4 | 玄骨改成「毒」 | 报 ④ | ✓ |
| V5 | 玄骨 `maxMp` 11（< 魔火 12） | 报 ③ 友军来路 | ✓ |
| V5b | 玄骨 `maxMp` 12 | 不报 | ✓ 0 错（边界） |
| V6 | 结丹元瑶降到 FoundationLate（青火雷要 CoreEarly） | 报 ② 友军来路 | ✓ |
| V7 | 紫灵 `weapons ["拳"]` | 报 ① `role_weapon` | ✓ |
| V8 | 乌丑 `magics` 去掉冰箭术 | 报 ③ 友军来路 | ✓ |
| V9 | `guairen.lua` 去掉 `magic.learn("magic_shijin_chongqun")` | 报第 12 章 learn | ✓ |
| V10 | `guiwang.lua`、`xuangu.lua` 去掉 `give("talisman_bixie_shenlei", 2)` | 报 ①④ give | ✓ 两条 |
| B1 | 神雷 `castMagic` 改成火弹术 | 应报 | ✗ 0 错 → **G-2** |
| B1b | `magic_bixie_shenlei.element` 改成 8（火） | 应报 | ✗ 0 错 → **G-2** |
| B2 | 祭金符、噬金虫 `needRealm` 改成 CoreMid | 应报第 12 章 | ✗ 只报第 10 章特判那一条 → **G-3** |
| B3 | 两门金术 `needMp` 改成 999 | 应报 | ✗ 0 错 → **G-3** |
| B4 | 青元剑芒 `needRealm` 改成 CoreLate | 应报 | ✗ 0 错 → **G-3** |

**selftest 探针（`gate12/ptree_*`，复制 `tools / data / scripts / maps` 后在副本里写坏，再从副本 import `validate_selftest` 跑对应函数；先断言 import 到的 `validate` 是副本那份）**

| # | 写坏 | 应 FAIL 的用例 | 实际 |
| --- | --- | --- | --- |
| P0 | 不动 | 无 | ✓ `selftest_ch10_means` 23 条 0 FAIL |
| P1 | `MEANS_LAST_CHAPTER = 11` | 「末章12」「同一海兽移入第12章：确实必报」「…且只怕毒 → 必报」 | ✓ means 段 2 条 FAIL；N 段 46 条中恰这 1 条 FAIL |
| P2 | `b13_waixinghai_lieyao.chapter` 改成 12 | 「outside 探针实际是第13章…」「第 13 章范围探针实际引用无极海兽」 | ✓ 两段各恰 1 条 FAIL |
| P3 | `wuji_haishou` 另进 `b12_hanlitai_xuangu` | 「…角色只在指定一场」 | ✓ 恰 1 条 FAIL |
| P4 | 删掉 `CHAPTER_MEANS[12]` | 「同一海兽移入第12章：确实必报」 | ✓ 恰 1 条 FAIL |

结论：探针搬家有效，范围上沿（11 / 12）、探针位置（第 13 章、唯一一场）、表本身缺失三种回归都抓得住；残余只有 G-5 那种「别人替它报」的可能。

## 七、门禁输出

| 门 | 命令（`fanren/` 下） | 输出 |
| --- | --- | --- |
| validate | `python tools/validate.py` | `数据条目 776 项，文案 5549 条，旗标 556 个` / `错误 0，警告 0` / `VALIDATE_OK`，RC 0 |
| mapgen | `python tools/mapgen/genmaps.py --check` | `MAPGEN_IN_SYNC：75 张图与生成器一字不差`，RC 0 |
| artgen | `python tools/artgen/artgen.py --check` | backdrops 76、title 4、sprites 348、fx 25；`ARTGEN_IN_SYNC：678 个文件与生成器逐像素一致（59.8s）`，RC 0 |
| selftest | `python tools/validate_selftest.py`（前台等完，与本审的构建并行，故 1699 s） | `用例 363 条，未如期抓住 0 条` / `SELFTEST_OK`，RC 0。与本章相关的 8 条全 PASS：第 13 章的敌人不在范围不报、探针文件及角色存在、实际引用无极海兽、同一海兽移入第 12 章且只怕毒必报、第 8/9 章批准表已接且末章 12、outside 探针是第 13 章且角色只在一场、第 13 章只怕毒范围外不报、移入第 12 章确实必报 |

日志：`scratchpad/selftest.log`、`scratchpad/gate12/{neg_validate.out,neg_probe_means.out,neg_probe_P1n.out,neg_probe_P2n.out,negE-all.txt,probeE.txt}`。

## 第 2 轮复核（2026-10-13 夜，整改第 1 轮·测试与门禁路之后）

对象：`tools/validate.py` 新表 `CHAPTER_START` 与新函数 `check_learned_castable`、`check_given_spell`；`tools/validate_selftest.py` 新段 `selftest_ch12_means`（28 条，CLI `--ch12-means`）与两处「必报」的严格匹配；`tests/Ch12EngineTests.cpp`；`docs/interfaces-p3-ch12.md`、`docs/tech-debt.md`、`docs/ch12-design.md` 18.3 / 18.5 / 18.6。照协调者要求没跑整套 selftest。

### 结论：Approve，G-1 到 G-7 全部关闭；新发现一条 LOW（G-8）

| 问题 | 状态 | 依据 |
| --- | --- | --- |
| G-1 友军 `actions` 没牙 | **关** | ③ 真场景用例循环里补 `EXPECT_EQ(unit->actions, role.actions)`，循环外断玄骨恰 2，`continue` 改成 `ASSERT_NE`。用仓库现有的 `src/ tests/` 重新复制到 scratch 树构建（0 警告）：基线 Ch12 引擎 23 / 23、`Ch12*` 84 / 84；变异 E → 恰这一条红（第 720、726 行，「Which is: 1」对「2」）；还原后哈希与仓库一致。契约 2.3 第 4、6 条与 2.4 末条已补 |
| G-2 施法符的五行不核 | **关** | `check_given_spell` 只对五行类 `give` 生效：castMagic 存在、五行含该类、`power > 0`、无 `effect`。不核 `needRealm` / `needMp` 是对的——引擎 `checkItem`（`BattleAction.cpp` 第 165–172 行）写明符箓「不查法力、不查习得」。阴性对照 G1–G10 全如期，见下表 |
| G-3 / G-29 自学法术境界与法力 | **关** | `check_learned_castable` 按 `CHAPTER_START` 核 `needRealm ≤ 起点`、`needMp ≤ 起点上限`，原第 10 章特判并入；整章表与按场登记的 learn 都核（按场的取编成的 `chapter`）。边界与第 4–12 章全扫描见下表 |
| G-4 注释与契约还说 skip | **关** | 文件头、第 358–361 行、第 619–622 行、契约 1.3 第 2 / 5 条、2.3 第 4 条、1.4 / 2.4、5.1、7 节、实现记录都改成「集成后 FAIL」 |
| G-5 探针「必报」不点名 | **关** | 第 1825 行与第 2525 行都加了 `"wuji_haishou" in e`。对照：N 段不动 46 条 0 FAIL；`MEANS_LAST_CHAPTER = 11` 时恰「同一海兽移入第12章且只怕毒 → 必报」FAIL；`--ch10-means` 23 / 23 |
| G-6 施工图数值陈旧 | **关** | 18.3 第 2、3 条已是现值（玄骨 450 / 96、36 / 40 / 34，架势三首 6、蚁群 3、大群 3、玄骨 4），调前数移到 18.5；契约 2.2 补「`maxMp ≥ 12`」 |
| G-7 引擎观察未入账 | **关** | 技术债 G-34（AI 法术优先，MEDIUM）、G-35（战后不写回法力）、G-36（BattleHand 不用单体 castMagic，另补切片用例经真菜单放神雷） |

### `CHAPTER_START` 各章取值（Python 读 `tests/fixtures/ch{N-1}-end-{first,second}.sav`）

| 章 | 表 | 上一章终档 first / second（realm, maxMp） | 一致 |
| --- | --- | --- | --- |
| 4 | QiRefining3 / 30 | ch03：(3, 30) / (3, 30) | ✓ |
| 5 | QiRefining8 / 80 | ch04：(8, 80) / (8, 80) | ✓ |
| 6 | QiRefining8 / 80 | ch05：(8, 80) / (8, 80) | ✓ |
| 7 | QiRefining9 / 90 | ch06：(9, 90) / (9, 90) | ✓ |
| 8 | FoundationEarly / 180 | ch07：(21, 180) / (21, 180) | ✓ |
| 9 | FoundationMid / 250 | ch08：(22, 250) / (22, 250) | ✓ |
| 10 | QiRefining3 / 30 | ch09：(3, 30) / (3, 30) | ✓ |
| 11 | FoundationLate / 320 | ch10：(23, 320) / (23, 320) | ✓ |
| 12 | CoreEarly / 470 | ch11：(31, 470) / (31, 470) | ✓ |

「起点即全章最低」这个前提也核了：第 4–12 章脚本里 `realm.advance` 只升；唯一的 `realm.demote` 在 `scripts/ch09/shudong.lua`（目标链 n19），第 9 章三场仗（n04、n06、n18）都在它之前，跌落后的炼气三层正是第 10 章起点。第 3 章表里没有 learn 来路，不登是对的。

### 阴性对照（scratch 副本 `gate12/r2tree`，脚本 `gate12/neg_r2.py`，在进程内调副本的 `check_means_sources`；45 条，意外 0）

| # | 写坏 | 应 | 实际 |
| --- | --- | --- | --- |
| R0 | 不动 | 0 错 | ✓ 0 错（第 4–11 章无误报） |
| G1 / G2 | 神雷 `castMagic` 改成火弹术 / 祭金符 | 报「五行不是「木」」 | ✓ |
| G3 / G4 | `castMagic` 改成不存在的 id / 删掉 | 报「不是一门存在的法术」 | ✓ |
| G5 | `magic_bixie_shenlei.element` 8 | 报 | ✓ |
| G6 | `element` 10（木 ＋ 火） | 不报 | ✓ |
| G7 / G8 | `power` 0 / 1 | 报 / 不报 | ✓ / ✓ |
| G9 | 挂 `effect stagger` | 报 | ✓ |
| G10 | 神雷那门法术 `needMp 9999`、`needRealm CoreLate` | 不报（引擎不查） | ✓ |
| L1 / L2 | 噬金虫 `needMp` 470 / 471 | 不报 / 报第 12 章「付不起」 | ✓ / ✓ |
| L3 / L4 | 噬金虫 `needRealm` CoreEarly / CoreMid | 不报 / 报「施展不了」 | ✓ / ✓ |
| L5 / L6 | `needMp "16"`（字符串）/ `needRealm "Core"`（认不出） | 报 | ✓ / ✓ |
| L7 | 青元剑芒 `needRealm CoreLate` | 恰第 8、9、11、12 章报 | ✓ |
| S-mp ×14 | 火弹术 `needMp` 取各章上限及其加一（30、31、80、81 … 470、471） | 恰「上限 < 值」的那几章报 | ✓ 14 / 14，例如 31 → {4, 10}、321 → {4–11}、471 → {4–12} |
| S-realm ×10 | 火弹术 `needRealm` 取各章起点及其上一档 | 恰「起点 < 值」的那几章报 | ✓ 10 / 10 |
| B5 / B5b | 祭剑符 `needMp` 81 / 80（第 5、6 章按场登记） | 报 b05 / 不报 | ✓ / ✓ |
| B7 | 祭青蛟 `needRealm QiRefining9` | 只报第 10 章；b07 各场（起点炼气九层）不报 | ✓ |

结论：两张新检查在第 4–12 章没有误报，各章的上下沿都抓得住；按场登记的 learn 用的是编成的章号。

### 新发现

**G-8 · LOW · `tools/validate.py` 第 2412–2422、2538–2542 行：`CHAPTER_START` 本身没有守卫**
- 问题：(a) `check_learned_castable` 遇到没登记起点的章直接 `return`，一个字不报。第 13 章登 `CHAPTER_MEANS[13]`、`MEANS_LAST_CHAPTER = 13` 时只要漏了 `CHAPTER_START[13]`，G-29 那道门就对整章静悄悄失效。(b) 表里的数是从终档手抄的，validate 和 selftest 都不读 `tests/fixtures/`；终档一旦重生成、数变了（比如某章补一次 `realm.advance`），表不会跟着红。selftest 新段只断 `CHAPTER_START.get(12) == ("CoreEarly", 470)`。
- 证据：副本里删掉 `12: ("CoreEarly", 470)`，同时把噬金虫改成 `needMp 999`、`needRealm CoreLate` → `check_means_sources` 0 错（`gate12/neg_r2_start.out`）。现状 9 章全登、与终档逐一相等，所以不挡。
- 改法：`check_means_sources` 里，凡是表里有 `learn` 来路、章号 ≥ 4 的章都必须有 `CHAPTER_START`，否则报错（第 3 章例外写死）；selftest 补一条「删掉 `CHAPTER_START[12]` → 必报」。再加一条读 `tests/fixtures/ch{N-1}-end-{first,second}.sav` 的 `realm` / `maxMp`、与表逐章比对的自检（selftest 或 C++ 夹具测试都行）。

### 本轮实跑

| 项 | 结果 |
| --- | --- |
| `python tools/validate.py`（21:48） | `数据条目 776 项，文案 5550 条，旗标 556 个` / `错误 0，警告 0` / `VALIDATE_OK`。这一刻文案整改路的 `ch12_*.json` 没有引出任何报错，无须归因 |
| `python tools/validate_selftest.py --ch12-means` | 28 条，未如期抓住 0 条，`CH12_MEANS_SELFTEST_OK` |
| `python tools/validate_selftest.py --ch10-means` | 23 条，0，`CH10_MEANS_SELFTEST_OK`（第 10 章特判并入后措辞改成「第 10 章起点 QiRefining3 施展不了」，断言已跟上） |
| N 段（`gate12/neg_probe.py`，从副本 import） | 不动：46 条 0 FAIL；`MEANS_LAST_CHAPTER = 11`：恰 1 条 FAIL |
| C++ scratch 树（仓库现有 `src/ tests/`） | 0 警告；Ch12 引擎 23 / 23，`Ch12*` 84 / 84；变异 E 恰红 ③ 真场景一条 |

附记一处本审自己踩过的坑：scratch 树第一次重同步后的基线「红」是假的。`copytree` 保留了仓库源文件较旧的 mtime，ninja 于是沿用上一轮变异 E 编出的 `BattleScene.cpp.obj`。`touch` 之后重编，基线就绿了。以后在 scratch 树里做变异，还原后要先 touch 再编。

日志：`scratchpad/gate12/{neg_r2.out,neg_r2_start.out,r2-base.txt,r2-negE.txt,r2_P0n.out,r2_P1n.out,r2-validate.txt}`。

## 协调者收尾（2026-10-13，第 2 轮剩余 LOW）

- **G-8 已关**：`tools/validate.py::check_learned_castable` 遇到「整数章号、有 learn 来路、`CHAPTER_START` 却没登这一章」直接报错（不再整章静默跳过）；第 3 章表里没有 learn 来路，不需要例外。`tools/validate_selftest.py` 的 CH12 段补两样：
  - 「`CHAPTER_START` 删掉第 12 章 → 必报」（报错须同时含「第 12 章」与「CHAPTER_START」）；
  - 逐章读 `tests/fixtures/ch{N-1}-end-{first,second}.sav` 的 `realm` / `maxMp` 与表比对（第 4–12 章 18 条），另配一条必不一致的对照（第 12 章法力改 471）。
- 实跑：`python tools/validate.py` → VALIDATE_OK；`python tools/validate_selftest.py --ch12-means` → 48 条、未如期抓住 0，`CH12_MEANS_SELFTEST_OK`（原 28 条 ＋ 删表 1 ＋ 对照 1 ＋ 终档比对 18）。全量 selftest 条数随之 391 → 411，见终验日志。
