# P3 第 12 章引擎增补契约

> 本文是**契约**：引擎方照它写判据，编剧、关卡、数据照它写数据与调用，双方不得私自改动。
> **字段名一经落盘不再改**；真要改，先改本文、写明原因，并通知协调者，不在代码或数据里单方面偏移。
>
> P3 · 2026-10-12 · 依据第 12 章研究笔记（`notes/ch12/research.md`）D 表（引擎现状）、E 表（拍板）、F 表（节点）、H 表（角色）、I 表（战斗）、J 表（旗标）、N.4（teleport）、O 表（第 13 章接口与开放问题）；格式沿 `docs/interfaces-p3-ch11.md`。
> **本章 0 个代码项**：笔记 D 表逐行核过，本章要的引擎行为全部已有，`src/` 一行不改。**E1–E2 是验证项**：查过、不新做代码，只在 `tests/Ch12EngineTests.cpp` 把内容路要倚仗、而此前从没有一章跑过的两种局面钉住，红了再议（先改笔记、再改本文、最后才动 `src/`）。
> 门禁：引擎路不改 `tools/`。规则 25 的 `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER`、selftest 探针搬家归协调者（第 5.2 节）。
> **不做**（笔记 E 表已拍板）：第三阵营（E 表 17：多方混战用「争夺者当友军」表达，不动 `BattleState` 与 AI）；按 spawn 名落点（脚本只传图 id 或图 id ＋ 坐标）；剑阵 / 分光法术化（E 表 21：「九柄」只在文案）；玉如意做兵刃或施法（E 表 20）；九曲灵参配方（E 表 31）；殿内商店（E 表 28）；`breakthroughPillBonus`（仍恒 0）；新 Lua 命令（O.1 末行）；存档字段（仍 v9）。
> **本章在第 11 章集成之后开工**：第 11 章 E1（`Recipe::guaranteed`）与此前各章引擎项都当成已有的用。
> **2026-10-13 整改**（施工图 16.1 第 36–48 条）：内殿拆成 `ch12_baoguang` / `ch12_taqian` / `ch12_hanlitai` / `ch12_lingquan` 四张图（新图 10 张）、enter 挂点不压 spawn、走偏拆 `zoupian_a/b/c.lua` 三份、`xuan_gu.magics` 只放魔火（新法术 3 门）、路径行动 0 条 reveal、锦帕门闸前移到 n07；本文第 0、2、3、4、5、6 节已照此改，字段名以本版为准。

---

## 0. 各引擎项现状（开工前）

| # | 要的东西 | 现状 |
| --- | --- | --- |
| E1 | n05 曲魂被夺舍，`party.remove("qu_hun_shadan")` 之后全章队伍为空；空队伍下照常进地图、进四场战斗（`hero_absent` 全 false）、存读档、开菜单队伍页（笔记 D 表「队伍」行、E 表 5–6、O-2） | 命令 `PartyRemove`（`src/game/Application.cpp` 第 1231–1235 行）：`outcome.ok = state_.partyRemove(id)`，不在队时 `ok false`、`code "not_in_party"`；Lua `party.remove` 只回 `result.ok == true`（`scripts/common/api.lua` 第 304–309 行），`party.size()` 读 `__host.party_size()`（`ScriptHost.cpp` 第 316–317 行）。战斗：`BattleScene::buildFromSetup`（`BattleScene.cpp` 第 397–433 行）先放韩立（0 号，`id "hanli"`）、再按编成建 ally / enemy、最后 `if (heroPresent_) addPartyUnits`（第 418、460–477 行，空队伍即什么也不加）；战后回写气血遍历 `state.party`（第 995–1003 行，空即不写）。菜单：`MenuScene::loadMembers`（`MenuScene.cpp` 第 214–280 行）先放韩立一行、再遍历 `state.party`，`!active` 或 `findRole == nullptr` 跳过。存档：`io::saveGame / loadGame`（`src/io/SaveFile.h` 第 59–60 行）。行走：`src/game/` 下读 `party` 的只有 `Application` / `BattleScene` / `MenuScene` 三处（2026-10-12 grep），`WorldScene` 不画随从。**空队伍本身不是新局面**：第 5 章 `scripts/ch05/dingji.lua` 第 59 行让曲魂离队之后，第 5–7 章整章都是空队伍，`ch05`–`ch07-end-{first,second}.sav` 的 `payload.party` 都是 `[]`（2026-10-13 读存档核过），地图、战斗、存读档已由这三章的通关测试跑过。本章新的只是「曾有队员（`ch10`/`ch11-end` 恰 `[qu_hun_shadan]`）、章中途离队，接着进带写死友军的仗」：E1 的新增价值在第 2 条（建场人数正负向）与第 4 条（菜单不崩）；第 1、3 条保留，钉的是 `qu_hun_shadan` 这一具体 id 的 remove 与「有人 → 变空」的存读档 |
| E2 | 同一个 role 先友后敌：`xuan_gu` 在 ③ `b12_neidian_duoding` 是友军、在 ④ `b12_hanlitai_xuangu` 是敌人；`wu_chou`（`toughness 3`、带 `weaknesses`）在 ③ 是友军（笔记 D 表「友军用带架势的 role」行、E 表 17、O-4） | `unitFromRole(state, role, ally)`（`BattleScene.cpp` 第 354–385 行）：`magics`、`weapons`、`actions`、攻防气血敌我都照抄；`if (!ally)` 才写 `maxToughness`、`weaknesses`、`revealed`、`chargeEvery / chargeMult / chargeAll / chargeTextKey`、`killableBy`——友军这五样恒为 0 / 空。战后破绽记账只记 `!unit.ally`（第 1006–1008 行）。胜负：`BattleState::refreshPhase`（`BattleState.cpp` 第 362 行起）按「场上还有没有活着的 ally / 敌人」判，友军倒下不改阵营。**先例**：`xuan_le`（`toughness 6`、带 `charge every 3`）在 `b08_lingkuang_shouzhen` 是 ally、在 `b08_zhongrudong` 是 enemy；`song_meng`、`matou_dashou` 同类——但三者都没有 `killable_by`，「友军忽略 `killable_by`」没有用例钉着 |

一个模块「可用」要三段齐备（`docs/handoff.md` 第 4 节）：规则层 → 加载器 → 游戏层入口。本批三段**都不改**；E1、E2 各自只有判据。

**查过、本章不需要新做的**（笔记 D 表「缺什么：无」那几行）：`teleport(map_id, x, y)`——`Application.cpp` 第 1004–1008 行先 `loadMap(a, "")` 落 default spawn，`x`、`y` 不同时为 0 才改 `position`，**同图带坐标**（只有三处走偏回岔口）走的是同一条路；teleport / loadMap **不判脚下的触发器**，enter 只在 `WorldScene::tryStep` 迈步时判（第 151–157 行）；`(0, 0)` 被当成「不给坐标」，落点格不许取 `(0, 0)`（四边是墙，本来也站不上）。多波次 `deployNextWave`（不分阵营，本章友军全在第 0 波）；首领 `charge{every, mult, all, text_key}` 与 `actions`；`killable_by` 非空（加载器 `DataLoader.cpp` 第 531–535 行与门禁规则 24 都拒空；**两处都不查 ⊆ `weaknesses`**，见 2.3 第 4 条）；道具施法 `castMagic`（`talisman_tianleizi` 先例）；`advance_days`（单次 ≤ 3650，本章单次 ≤ 90）与 `flag.set(name, today())`（第 11 章 E2 已钉）；`choice`（取消为 nil）；`item.count` 与 `take` 看返回值；NPC `visible_flag / hidden_flag`；portal `require_flag` ＋ `deny_text_key`；路径行动 `when / until / reveal`；章节卡按完成旗标排、最后一章之后接「未完待续」；`realm.*` 本章 0 处调用。

**不是引擎的事、写在这里免得找错人**：「结丹初期单人 ＋ 临时友军对结丹后期首领」的量法（笔记 C.2）归平衡路，用 `tests/BattleHand.h` 量，`Realm.cpp` 锚点数一个不动；`wu_chou.json`、`qu_hun_shadan.json`、`zi_ling.json`、`yuan_yao.json`、`wen_qiang.json`、`jin_qing.json`、`miao_zhanglao.json`、`jiyin_fushen.json` 一个字不动（笔记 B4、B8、B10、B11、H 表）；`wu_chou.json` 的 note 与原著相反（B4），由协调者两章都集成后回写，本章不碰。

---

## 1. E1 空队伍（验证项，不改代码）

### 1.1 一句话口径

**`party.remove("qu_hun_shadan")` 返回 true 之后 `party.size() == 0`；空队伍下战斗的我方只有韩立（0 号）＋ 编成里写死的友军，战后不写回任何同伴、不凭空长出同伴；存档→读档→再存档字节相同；菜单队伍页只有韩立一行、不崩。本章 `party.add` 0 处，章末队伍为空交给第 13 章。** 任一处崩，才改 `src/`（先改笔记 O-2、再改本文）。

- 规则层（`core::GameState::partyRemove / partyHas`）、加载器（`io::SaveFile`）、游戏层（`Application`、`BattleScene`、`MenuScene`）：**都不改**。
- 内容路要知道的一句：`party.remove` 返回 false（存档里本来就没有曲魂）只记一句旁白、照走（笔记 N.1 第 3 条），**不许据此 `return`**——否则 n05 卡死。

### 1.2 数据形状（内容路写；只是形状，id 以笔记为准）

编成里的友军只写 `faction "ally"`，`wave` 不写（＝ 0，开场就在场上）；本章四场 `hero_absent false`：

```json
{ "id": "b12_guiwu_guiwang", "chapter": 12, "hero_absent": false, "defeat_is_fatal": true, "can_escape": false,
  "units": [ {"role_id": "zi_ling_xutian", "faction": "ally"}, {"role_id": "yuan_yao_heipao", "faction": "ally"},
             {"role_id": "weiwu_hanggui", "faction": "enemy"}, {"role_id": "weiwu_hanggui", "faction": "enemy"},
             {"role_id": "gui_yecha", "faction": "enemy"},
             {"role_id": "hui_ying", "faction": "enemy", "wave": 1}, {"role_id": "shuangtou_guihu", "faction": "enemy", "wave": 1},
             {"role_id": "sanshou_guiwu", "faction": "enemy", "wave": 2} ] }
```

### 1.3 判据（`tests/Ch12EngineTests.cpp`，引擎路写；每条直接读 `GameState` / `BattleState::units()` / 存档字节，不经 `Driver`）

夹具照 `Ch11Fixture`：headless `Application` 载入 `tests/fixtures/ch10-end-first.sav`（先验 `party` 恰 1 名、`roleId "qu_hun_shadan"`、`active true`）；`ch11-end` 落地后**不换**夹具——判据不依赖章节进度。探针脚本写法照第 11 章契约 2.3 的偏差记录（`TempDir/scripts/` 重挂 scriptRoot、`startEvent` 后泵帧）。

1. **离队**：探针脚本 `local ok = party.remove("qu_hun_shadan")`，把 `ok` 与 `party.size()` 写进两个探针旗标 → 旗标读回 `1` 与 `0`，`state.party.empty()`。**负向**：同一脚本再跑一次 → `ok` 旗标 `0`（不在队里如实报 false），`party` 仍空、不崩。
2. **空队伍进战斗**（用现成编成，不等内容路）：`state.party` 清空后 `BattleScene("b11_tiandu_shi").onEnter(app)` → `battle().units()` 里 `ally` 恰 5 名（0 号 `id "hanli"` ＋ `xueyu_zhizhu_ally` ×2 ＋ `kuilei_yuan` ×2），没有任何 `id "qu_hun_shadan"` 的单位。**负向**：`party` 放回 `qu_hun_shadan`（`active true`）同样建场 → `ally` 恰 6 名、含曲魂——证明第 2 条的「没有」来自空队伍而不是编成；再把它置 `active false` → 回到 5 名。编成文件不存在时 `FAIL()`（交付时写的是 `GTEST_SKIP`，集成时协调者改成 `FAIL()`，见本文末「实现记录」）。
3. **存读档**：空队伍 `io::saveGame(state, A)` → `io::loadGame(A)` 的 `party.empty()`；再 `saveGame(loaded, B)`，`A`、`B` 字节相同。**负向对照**：`party` 恰 1 名时同样走一遍，读回恰 1 名、`roleId` 与 `hp` 不变（证明比的是真字段，不是「什么都没存」）。
4. **菜单队伍页**：空队伍下 `MenuScene` `onEnter` ＋ 一帧 `update` / `render`（headless）不崩。`members_` 是私有的、没有访问器，本条**只钉不崩**；要钉「恰 1 行」需另给 `MenuScene` 一个只读访问器，`src/` 不在本章白名单，由协调者定。**负向**：`party` 里放一个 `findRole` 查不到的 id（如 `"no_such_role"`）→ 同样不崩（`loadMembers` 的 `role == nullptr` 跳过）。
5. **真数据（内容路落地后）**：`io::loadBattle(data/battles/b12_guiwu_guiwang.json)` `hero_absent false`、ally 恰 `{zi_ling_xutian, yuan_yao_heipao}`；空队伍下真 `BattleScene` 建场后 ally 恰 3 名（韩立 ＋ 两名）。另三场 `b12_heishamo_tiehuoyi`（ally 恰 `{yuan_yao_jiedan}`）、`b12_neidian_duoding`（ally 恰 `{wu_chou, xuan_gu}`）、`b12_hanlitai_xuangu`（ally 空）同法各一条。文件不存在（或 ③ 仍是旧占位）时 `FAIL()`（交付时是 `GTEST_SKIP`，集成后改）。
6. **负向自检**（改坏 → 增量构建 → 确实红 → 还原，日志 `build-ch12eng-neg-*.log`）：`PartyRemove` 改成恒 `ok = true` → 第 1 条负向红；`MenuScene::loadMembers` 去掉 `role == nullptr` 的跳过 → 第 4 条负向崩 / 红。

### 1.4 实现记录（引擎路交付时填）

2026-10-13 引擎路交付，`src/` 零改动，判据全在 `tests/Ch12EngineTests.cpp`（夹具 `Ch12Fixture`，照 `Ch11Fixture`）。

- **第 1 条** `RemovingQuHunReportsTrueAndEmptiesThePartyAndASecondRemoveReportsFalse`：两个探针旗标先写哨兵 99，读回 0 才算脚本写的；第二遍即负向。
- **第 2 条** `AnEmptyPartyEntersTheBorrowedBattleWithTheHeroAndItsFourWrittenAllies`（5 名、0 号 `hanli`、蛛 ×2、傀儡 ×2、无曲魂）；负向 `PuttingQuHunBackAddsExactlyHimAndAnInactiveQuHunAddsNobody`（6 名 → `active false` 回到 5 名）。
- **第 3 条** `AnEmptyPartySavesLoadsAndSavesAgainByteForByte`；负向对照 `AOneMemberPartyRoundTripsItsIdAndHitPointsAndDiffersFromTheEmptySave`（`hp 345` 往返不变，且与空队伍存档字节不同）。
- **第 4 条** 比契约多钉了一层：`members_` 用显式实例化探针读出（`Ch09EngineTests` 的手法；`MenuScene::Member` 是私有类型，探针在模板里把 `roleId / hero` 拷出来，不需要访问器、`src/` 不动）。`TheMenuWithAnEmptyPartyShowsOnlyTheHeroAndSurvivesAFrame`：`onEnter` ＋ 一帧 `update` / `render` 不崩且恰 1 行 `hanli`；`TheMenuListsAnActiveCompanionAndSkipsAnUnknownRoleWithoutCrashing`：对照（曲魂在队恰 2 行）＋ 负向 `no_such_role` 恰 1 行、不崩。
- **第 5 条** 四场各一条 `TEST_F`（`The{Guiwu,Heishamo,Neidian,Hanlitai}Setup…`）：`io::loadBattle` 的 `heroAbsent false` 与 ally 多重集合，再空队伍真 `BattleScene` 建场 ally ＝ 1 ＋ n、无曲魂。守卫条件：文件不存在，或 `b12_neidian_duoding` 的编成里仍有 `duoding_xiushi`（旧占位）。交付时这道守卫是 `GTEST_SKIP`（内容路 B 四张都已落地，实跑不跳过）；**集成时协调者把本文件 11 处 `GTEST_SKIP()` 全改成 `FAIL()`**（`notes/ch12/integration.md`），现状是缺文件即红、不再跳过。
- **第 6 条负向自检**：为不碰共享 `src/`，在 scratch 里复制一份 `src/ tests/ CMakeLists.txt`（`vendor` 用目录联接）另建构建目录，改坏 → 增量构建 → 跑 `Ch12*` → 还原，日志 `build-ch12eng-neg-{A,B}.log`（末尾附 `TESTRC` 与失败用例名）。A `PartyRemove` 恒 `ok = true` → 只有第 1 条红（1 红 / 22 绿）；B 去掉 `role == nullptr` 跳过 → 第 4 条负向那条 SEH 0xc0000005（访问违例，被 gtest 收成红；1 红 / 22 绿）。仓库里的 `src/` 自始至终没改过。
- **集成注意**：`data/text/battles.json` 第 6–7 行两条旧 intro 未删时（契约 5.2，协调者），`loadGameData` 报「文案 key 重复」，`Application::init` 失败，本文件 20 条依赖整仓数据的用例全红；在删掉这两条的数据快照上 23 条全绿。

---

## 2. E2 同一 role 先友后敌（验证项，不改代码）

### 2.1 一句话口径

**站哪一边由编成说了算，role 文件只写一份：`xuan_gu` 带 `toughness / weaknesses / killable_by ["木"] / charge / actions 2 / magics`；做 ally 时（③）架势、破绽、蓄势、要命类别四样一概不生效（`killableBy 0`，谁都打得死它），`magics` 照样施展（③ 里是魔火 `magic_moyan_huo`；修罗圣火不是法术，只是 ④ 的蓄势文案，施工图 16.1 第 40 条）；`actions` 对友军**照样生效**（`BattleScene.cpp` 第 372 行 `unit.actions = role.actions` 在 `if (!ally)` 守卫之外，`BattleState` 按 actions 排行动槽不分敌我，友军又由玩家操纵——③ 里玩家一回合能让玄骨动两手，平衡路按此量）；做 enemy 时（④）四样照常生效。`wu_chou` 做 ally 同此。友军被打倒只是少一个人，不改胜负口径；战后破绽记账只记敌方，③ 里看见的不会算到 ④ 的玄骨头上。** 若友军吃了 `killable_by`（被压到 1 打不死），③ 照样打得完（敌方只有狼），但要在本节判据里红出来（笔记 O-4）。

- 规则层（`core::battle::BattleState`、`BattleAction`）、加载器（`io::DataLoader` 的 role、`io::BattleLoader`）、游戏层（`BattleScene::unitFromRole`）：**都不改**。
- 内容路要知道的：③ 的「争夺者」是写死友军，**不配金类法术**（笔记 I 表；第 1 波银形只有金要得了命，金得是玩家的）；③ 两名友军战后不写回任何存档字段（它们不在 `party` 里）。

### 2.2 数据形状（内容路写；数字由平衡路定）

```json
{ "id": "xuan_gu", "name": "玄骨", "realm": "CoreLate", "toughness": "<平衡路>", "weaknesses": ["木"],
  "killable_by": ["木"], "actions": 2, "magics": ["magic_moyan_huo"],
  "charge": { "every": 3, "mult": "<平衡路>", "all": "<平衡路>", "text_key": "ch12.battle.hanlitai_xuangu.charge" } }
```

写 `"<平衡路>"` 的几格与气血攻防由平衡路实测定（落盘时是数字 / 布尔，不是字符串）；`maxMp` 也由平衡路定，**须 ≥ 魔火 `needMp` 12**（规则 25 `role_magic` 要 `needMp ≤ maxMp`，③ 那一条友军来路才站得住；平衡路定为 96，来由见施工图 18.5 第 2 条）；`magics` 只放现有的魔火（火、`power 12`、`needMp 12`、`needRealm FoundationEarly`、无 `effect`——满足规则 25 `role_means_available` 的全部条件），`magic_xiuluo_shenghuo` 不建；`killable_by` ⊆ `weaknesses`——**规则 24 与加载器都只拒空数组、不查子集**，子集由 2.3 第 4 条断；③ 编成里 `{"role_id": "xuan_gu", "faction": "ally"}`、`{"role_id": "wu_chou", "faction": "ally"}`，④ 里 `{"role_id": "xuan_gu", "faction": "enemy"}`。

### 2.3 判据（`tests/Ch12EngineTests.cpp`；建场用真 `BattleScene`，打击用直接构的 `BattleState`，都不经 `Driver`）

1. **先例两边对照**（不等内容路）：headless `Application`，内存里把 `app.data().roles.at("xuan_le").killableBy` 设成「木」的位掩码（用 `core/model/AttackCategory.h` 的名字表换算，不写数字；只改内存、不写文件）。`BattleScene("b08_lingkuang_shouzhen")` 里 `xuan_le` 是 ally → `maxToughness 0`、`weaknesses 0`、`revealed 0`、`chargeEvery 0`、`chargeTextKey` 空、`killableBy 0`，且 `magics == role.magics`、`actions == role.actions`（友军照抄 actions，2.1；`xuan_le` 的 `actions` 是 1，恰等于 `Unit::actions` 的缺省值，这一句在 1 上空转，有牙的那一句在第 4 条的玄骨上）。**负向（同一 role 做敌人）**：`BattleScene("b08_zhongrudong")` 里 `xuan_le` 是 enemy → `maxToughness == role.toughness`（6）、`weaknesses == role.weaknesses`、`chargeEvery == 3`、`killableBy ==` 刚设的掩码。两场都用同一份内存数据，证明差别只来自 `faction`。
2. **友军打得死**：直接构 `BattleState`：我方韩立 ＋ 一名 `killableBy 0`、`hp 10` 的友军，敌方一名；敌方「剑」类别一击（基础伤害足够，先验 `estimateHitDamage ≥ 10`）→ 友军 `hp == 0`、`!alive()`、`phase() == Ongoing`（韩立还在）。**负向**：同一编成把友军的 `killableBy` 改成「木」→ 同一击后 `hp == 1`、仍 `alive()`——这就是 2.1 说的「友军若吃了 `killable_by`」的样子，钉住它免得日后有人把 `if (!ally)` 挪走还全绿。
3. **破绽记账只记敌方**：第 1 条两场建场之前各把 `state.knownWeaknesses` 里 `xuan_le` 清空；ally 那一场 `revealed` 恒 0（第 1 条已钉）、enemy 那一场 `revealed == knownWeaknessesOf("xuan_le") & weaknesses`。**负向**：先 `learnWeaknesses("xuan_le", 全部 weaknesses)` 再建 ally 那一场 → `revealed` 仍 0（记过的破绽不会亮在友军头上）。
4. **真数据（内容路落地后）**：`data/roles/xuan_gu.json` 的 `killable_by` 集合恰 `{木}` 且 ⊆ `weaknesses`、`actions == 2`、`charge.every == 3`、`magics` 恰 `["magic_moyan_huo"]`；**四张编成里所有带 `killable_by` 的敌方 role**（`sanshou_guiwu`、`tiehuo_yiqun`、`tiehuo_yiqun_da`、`langxing_qiling_yin`、`xuan_gu`）逐一断 `killableBy & ~weaknesses == 0`（规则 24 与加载器都不查子集，这里是唯一一道；负向：内存里给其中一个的 `killableBy` 多加一位 → 红）；真 `BattleScene("b12_neidian_duoding")` 里 `xuan_gu`、`wu_chou` 都在场（找不到即红，不跳过）、都是 ally 且 `killableBy 0`、`maxToughness 0`、`chargeEvery 0`，**且 `actions == role.actions`、玄骨恰 2**（门禁审查 G-1：友军照抄 actions，2.1 说的「③ 里玩家一回合能让玄骨动两手」就钉在这里）；敌方 `langxing_qiling_yin` 的 `killableBy` 恰「金」；真 `BattleScene("b12_hanlitai_xuangu")` 里 `xuan_gu` 是唯一敌人、`killableBy` 恰「木」、`chargeEvery 3`。`xuan_gu.json`、两张编成任一不存在时 `FAIL()`（交付时是 `GTEST_SKIP`，集成后改）。
5. **③ 友军不配金**（I 表）：写成辅助函数 `alliesCastMetal(setup, data)`——遍历编成里 ally 的 `magics`，返回 `element` 为「金」的那些；对真 `b12_neidian_duoding` 返回空。**负向**：同一份数据在内存里给 `wu_chou` 的 `magics` 塞一门现有的金类法术（`magic_ji_jinfu`）→ 返回恰那一门。编成不存在时 `FAIL()`（交付时是 `GTEST_SKIP`，集成后改）。
6. **负向自检**（改坏 → 构建 → 红 → 还原）：`unitFromRole` 去掉 `if (!ally)` 的守卫（敌我一样抄）→ 第 1、3、4 条红（第 2 条直接构 `BattleState`，不经 `unitFromRole`，不受影响）；守卫里漏掉 `killableBy` 那一行（友军照抄）→ 第 1、4 条红；**E：`unit.actions = role.actions` 挪进 `if (!ally)`（友军不抄 actions）→ 第 4 条 ③ 真场景红**（整改第 1 轮补，见 2.4 末条）。

### 2.4 实现记录（引擎路交付时填）

2026-10-13 引擎路交付，`src/` 零改动。

- **第 1 条** 夹具 `Ch12TwoSided`（继承 `Ch12Fixture`）先验 `xuan_le` 架势 6、蓄势 3、破绽非空，再只在内存里把 `killableBy` 设成「木」：`AsAnAllyXuanLeHasNoStanceWeaknessChargeOrKillableByButKeepsMagicsAndActions` ／ `AsAnEnemyTheSameRoleCarriesStanceWeaknessChargeAndKillableBy`（敌方另断 `chargeTextKey == role.charge.textKey`）。
- **第 2 条** `Ch12AllyKillable` 两条：直接构 `BattleState`（韩立、10 血友军、速度 100 的敌方先手，先验 `currentActor == 2`、`estimateHitDamage ≥ 10`）；`killableBy 0` → 0 血、`Ongoing`；负向「木」→ 1 血、仍活。
- **第 3 条** `KnownWeaknessesLightTheEnemyAndNeverTheAlly`：清掉 `xuan_le` 的记录 → 两边都 0；`learnWeaknesses` 全部破绽 → 敌方 `revealed == weaknesses`、友军仍 0。
- **第 4 条** 真数据三处：`XuanGuIsKillableByWoodOnlyActsTwiceChargesEveryThreeAndCastsOnlyMoyanHuo`；`EveryKillableEnemyOfTheFourSetupsStaysInsideItsWeaknesses`（四张编成里带 `killable_by` 的敌方 role 恰契约那五个、子集为真；负向：内存里给 `xuan_gu` 加一位破绽之外的类别 → 恰报 `xuan_gu`）；`InTheNeidianXuanGuAndWuChouAreAlliesWithoutEnemyOnlyFieldsAndTheSilverWolfNeedsMetal`、`OnTheHanlitaiXuanGuIsTheOnlyEnemyKillableByWoodAndChargingEveryThree`（另断 `maxToughness > 0`）。辅助函数 `killableOutsideWeaknesses` 另有一条不依赖内容路的手造数据用例 `KillableByOutsideWeaknessesIsReportedForEnemiesOnly`（正向、负向、做友军时不报）。
- **第 5 条** `alliesCastMetal(setup, data)`：真数据 `TheNeidianAlliesCastNoMetalUntilWuChouIsHandedJinfu`（空 → 塞 `magic_ji_jinfu` 后恰那一门）；另有手造编成 `AMetalSpellOnAnAllyIsReportedAndOnAnEnemyIsNot`（现有乌丑不配金、塞金后报、做敌方不报），内容路落地前就能跑。
- 第 4、5 条真数据用例的守卫条件：`xuan_gu.json` 或四张编成任一未落地（③ 仍是旧占位也算未落地）；交付时是 `GTEST_SKIP`（都已落地，实跑不跳过），集成时改成 `FAIL()`（同 1.4 第 5 条）。
- **第 6 条负向自检**（scratch 复制树，同 1.4）：C 去掉 `if (!ally)` 守卫 → 第 1（友军那条）、3、4（③ 真场景）条红（3 红 / 20 绿），第 2 条不受影响，与契约预期一致；D 把 `killableBy` 挪出守卫 → 第 1、4 条红（2 红 / 21 绿）。日志 `build-ch12eng-neg-{C,D}.log`。
- **整改第 1 轮（测试与门禁路，2026-10-13，门禁审查 G-1 / 测试审查 T-7）**：③ 真场景用例 `InTheNeidian…` 的循环里 `if (unit == nullptr) continue;` 改成 `ASSERT_NE`，并补 `EXPECT_EQ(unit->actions, role.actions)`、循环外 `EXPECT_EQ(玄骨 actions, 2)`。阴性对照 E 在 scratch 复制树（`src/ tests/ CMakeLists.txt` 照仓库复制、其余目录联接）里做：`BattleScene.cpp` 第 372 行挪进 `if (!ally)` → `Ch12*` 84 条恰这 1 条红（「Which is: 1」对 2），83 绿；还原后与仓库、`git HEAD` 的 `BattleScene.cpp` SHA-256 相同，重建后 84 / 84 绿。仓库里的 `src/` 自始至终没改过。

---

## 3. 第 13 章接口（本批**不实现**，写成约定；笔记 O.2 逐行照抄）

第 13 章施工图与它的契约照这张表接；要改表里任何一行，先改本文。

| 接口 | 本章留下什么 | 第 13 章怎么用 |
| --- | --- | --- |
| **开局** | 章末在 `ch12_lingquan` 灵泉密室（spawn `quanbian`，n33 `lingquan.lua`），`ch12.done` 已置；不 `ending()`、不 teleport；第 12 章是 `data/chapters.json` 最后一章，「未完待续」自动接 | patch `scripts/ch12/lingquan.lua` 末尾补一行 `teleport("ch13_…")`（照本章 `zhuzhong.lua` 先例，只传图 id、落 default spawn）；ch499「天星城出了大事」只许在第 13 章开场说 |
| **境界** | 31 / 31，`formerRealm 22`；本章 `realm.*` 0 处 | ch529 结丹中期：`realm.advance(realm.CORE_MID)` 一处（第 13 章定）；`realm.cap` 本章没动 |
| **队伍** | **空**（E1）；`party.add` 0 处 | 第 13 章独行；曲魂之躯已在 ch493 随玄骨化灰，只许旁白提 |
| **法术** | 起点 11 门 ＋ `magic_shijin_chongqun`（金） | 第 13 章若加剑阵法术，自己定 |
| **炼晶** | `material_lianjing` ×1（`tradeable false`） | ch519–520 掺入重炼七十二剑（第 13 章定配方或剧情；要必成先改第 11 章契约 1.1「只许一张」） |
| **玄骨肋骨** | `story_xuangu_leigu` | ch518 起破出功法——名字第 13 章才许点出（本章全文 0 处「玄阴经」） |
| **血色披风、五色铜环** | 两件 story | ch500–504 逃命、ch504 铜环（第 13 章定是否做成手段） |
| **啼魂兽、鸣魂珠** | story ×2 | 第 14 章阴冥之地 |
| **乾蓝冰珠** | story | ch544 破结丹后期（第 14 章） |
| **虚天鼎** | story，打不开 | 本篇不开（ch641 才清点） |
| **玉如意** | story，器灵未炼化 | 名字永远是「狼形器灵」；「银月」全作禁用 |
| **九曲灵参、丹方** | story ×2，0 配方 | 凝婴用；本篇不炼 |
| **其余 story** | 万年灵乳、养魂木根须、皇鳞甲、青冥针符宝、婆罗珠、白犀佩、寒冰珠、玄阴环各 1 | 按需；极阴追踪暗记**已被噬金虫除去**（n32），第 13 章不必再演追杀 |
| **金丝蚕** | `ch12.jincan` 为 1 时 story ×3 | ch522：服下五色圆珠的两只撑到此时、其余渐死；喂霓裳草后产下上百枚死卵，那一对也双双毙命（2026-10-13 协调者按原文第 24648、24663 行更正，第 13 章研究笔记 B19 / B20 提出）——第 13 章演完这一段即把 3 件收回 |
| **星宫** | 执法长老化身符斩蜘蛛；本章 0 处「逆星盟」 | 逆星盟（ch500）由第 13 章起 |
| **规则 25** | `MEANS_LAST_CHAPTER = 12`；探针在 `wuji_haishou` ＋ `b13_waixinghai_lieyao` | 第 13 章写实它时探针再挪到 `feng_xi` ＋ `b14_fengxi_xiezhi`（2026-10-12 grep 唯一） |
| **时间** | `advance_days` 单次仍 3650；本章合计 182 ＋ 0–4 日（常速 0–2、灵石兜底 0–2；打坐静养另计，玩家自选）| 起点 `day` 从 `ch12-end-{first,second}.sav` 读，不从施工图抄 |
| **灵石** | 低阶意图玩家 −8、中阶 0（笔记 N.1） | 从 `ch12-end` 读 |
| **词表** | 本章补 83 词（含胡月、石蝶）＋ 「紫罗极火」禁词；`{12, 499}` 已在 | 逆星盟 ch500、明珠轩 ch513、交换会 ch535 照表二放行 |
| **Lua 只读** | 不加任何 Lua 命令 | 同第 11 章契约第 6 节末行 |
| **洞府 / 灵田** | 本章单向进殿，回不去 `ch11_dongfu`；`field_tianxing_39` 的 6 槽作物、静室、炼器台随之搁置（本章 0 处读写） | 第 13 章开局是否回府、灵田作物怎么算由第 13 章定（原著 ch499 他起意弃府） |
| **妖丹、妙音门腰牌、金青** | 妖丹（五级 0、六级 0–1）、`story_miaoyin_yaopai` 本章不用、不读；金青 ch426 倒下（`jin_qing.json` 不动，本章之后不再上图） | 第 13 章按需；金青不再出场 |

---

## 4. 脚本与数据口径（内容路照写，引擎路不改）

以下都是已有接口，列在这里是为了让几路对同一件事只有一种写法（id、旗标、数字一律以笔记为准）：

- **境界**：`realm.advance`、`realm.cap`、`realm.demote` 全章 **0 处**；脚本不读 `formerRealm`（笔记 C.1、E 表 7）。
- **队伍**：`party.remove("qu_hun_shadan")` 恰 1 处（`scripts/ch12/duoshe.lua`），写 `local ok = party.remove("qu_hun_shadan")`，`ok` 为假时 `talk` 一句、照走；`party.add` 0 处（E 表 5–6）。`qu_hun_shadan.json` 一个字不动；夺舍后的躯体是新 role `xuangu_shaonian` → `xuan_gu`。
- **teleport**：只写图 id 或图 id ＋ 整数坐标，不传 spawn 名；坐标不许是 `(0, 0)`（第 0 节）。笔记 N.4 共 12 条（三元 9、五元 3）；三元 9 条都只传图 id，每张图把脚本落点那个 spawn 标 default（`andeng`、`tongdao`、`tupo`、`caoyuan`、`guangbi`、`yuqiao`、`shishi`、`taijiao`、`quanbian`；`ch12_dixue` 的 `taijie` 是门的落点）。**teleport 落地不触发 enter**：每张图的首个 enter 挂点（n01、n08、n12、n20）是 spawn 前方横贯通道的触发带，不压在 spawn 格上（施工图 16.1 第 36 条）。挂 NPC 的传送（`lidao.lua`）与首次置旗的传送，`flag.set` 与 `teleport` 写在顶格、早退用顶格 `return`。
- **走偏**：`trigger_zoupian_a/b/c` 一律 `type trigger`、`mode enter`、`once false`、`guard_flag ch12.tupo`、`set_flag` 空，分别挂 `scripts/ch12/zoupian_a.lua` / `zoupian_b.lua` / `zoupian_c.lua`（三份，内容只差坐标）；每份 0 处 `flag.set(`，开头判 `ch12.guiwang` 已置则 `return`，否则旁白 ＋ `teleport("ch12_guiwu", x, y)` 回自己那一处岔口（E 表 10）。理由：Lua 拿不到是哪个触发器起的事件、也读不到玩家坐标（`ScriptHost.cpp` 注册的 host 函数里没有，`Application::startEvent` 只传脚本路径），一份脚本分不出三条错路。三处落点不与任何触发器同格。
- **触发器**：同一格只取对象表里第一个 trigger，挂点错开格子；`once true` 的必有 `set_flag`（＝ done_flag）——**`set_flag` 指向的旗标由脚本末尾自己 `flag.set`**，引擎从不替脚本写（`WorldScene.cpp` 第 32–65 行）；挂 NPC 的节点（n06、n07、n09、n18、n33）脚本开头看 done_flag 判已演过。portal / facility 不读 `visible_flag / hidden_flag`，门闸只用 `require_flag` ＋ `deny_text_key`（规则 20：`ch12.block.dixue`）。**规则 30**：同图同 role 的两个 NPC 必须一个的 hidden 恰等于另一个的 visible（鬼雾两个紫灵恰接于 `ch12.guiwang`），不同 role 同图不许同名牌（两名执法长老「慈眉长老」「冷脸长老」），见施工图 5.5。
- **锦帕门闸**：在 n07 `lidao.lua` 的 `advance_days`、`flag.set("ch12.lidao")`、`teleport` 之前，顶格 `if item.count("story_jinpa_ditu") < 1 then … return end`；`rudian.lua` 不设门闸（落点是无门的大厅）。
- **日历**：每句 `advance_days` ≤ 90（笔记 N.2）；n20 `flag.set("ch12.binghuo_ri", today())`，n21 按 `today() - flag.get("ch12.binghuo_ri")` 判，固定段 ＋ 抉择至多 4 日，永不超时（E 表 12）。
- **整数旗标**：`ch12.huanqi`（1 / 2）、`ch12.quba_xian`（1 / 2）、`ch12.xinjing`（位掩码 1 色、2 惧）、`ch12.jincan`、`ch12.binghuo_ri`（日期，不是开关）用 `flag.set(name, n)`；开关旗标 `flag.set(name)`（笔记 J.2）。
- **扣东西**：`take("story_bai_zhenzhu", 1)`、`take("story_zhiyang_huazhou", 1)`、两次神雷余数全看返回值；n24、n32 两处 `take("material_lingshi", 4)` → 不成 `take("material_lingshi_zhong", 1)` → 再不成 `advance_days(1)` 照走（N.1 第 2 条）。余数写成 `local n = item.count("talisman_bixie_shenlei"); if n > 0 then take("talisman_bixie_shenlei", n) end`——`take(id, 0)` 在引擎里会被当成扣 1（`Application.cpp` 第 1030 行 `command.x <= 0 ? 1 : command.x`），余数为 0 时不许调 `take`（判据：两个脚本里 `take("talisman_bixie_shenlei"` 都在 `n > 0` 的分支里）。
- **辟邪神雷**：`talisman_bixie_shenlei`（kind `talisman`、`castMagic magic_bixie_shenlei`、`tradeable false`、`price 0`），n13、n31 战前各 `give` 2、战后收尽；全仓 `magic.learn("magic_bixie_shenlei")` 0 处（E 表 19、O-6）。噬金虫 `magic.learn("magic_shijin_chongqun")` 恰 1 处（n03），看返回值。
- **四场编成**：全部 `hero_absent false`、`defeat_is_fatal true`、`can_escape false`、`rewards.spirit_stones 0`、`drops []`、`cultivation > 0`；友军写死、`wave` 缺省；`b12_neidian_duoding` **就地改写**（乌丑改友军，E 表 18）；开场白与蓄势文案在 `data/text/ch12_battle.json`，key `ch12.battle.<短名>.intro / .charge`；胜利文案 ③ 写「收服」、④ 写「圣火反噬」，不写「杀死玄骨」（I 表：肉身是圣火失控焚尽、他以剑芒封了去路，残魂是 n32 他令啼魂兽吞灭）。
- **章末**：`lingquan.lua` 最后 `flag.set("ch12.done")`，不 `ending()`、不 teleport（E 表 24）。
- **文案**：`data/text/ch12*.json` 0 处 `{key.` 与 `{pad.`；全章 0 处「银月」「紫罗极火」「逆星盟」「玄阴经」；名牌不出现「萧诧」（n06 台词里自报一次）；章内先后照笔记 M 表末段。

---

## 5. 门禁与既有测试的依赖

### 5.1 引擎路

本章不改 `src/`、不改 `tools/`。共享内容不齐时只报告 `tests/Ch12EngineTests.cpp` 的证据，不报全量 data 绿色；内容路未落地的那几条按 `GTEST_SKIP` 交付，集成后由协调者重跑一次看实跑未跳过。（现状：集成时协调者已把这 11 处 `GTEST_SKIP()` 改成 `FAIL()`，缺文件即红，见「实现记录」。）

### 5.2 协调者：集成时落

- **规则 25 的表**（`tools/validate.py` 第 2289–2385 行）：`CHAPTER_MEANS[12]` ＝ 拳（`hand`）、火（`learn magic_huodan_shu ch04`）、土（`learn magic_liusha_shu ch06`）、水（`learn magic_bingdong_shu ch06`）、金（`learn magic_ji_jinfu ch07`，外加 `("金","learn","magic_shijin_chongqun","ch12")`）、木（`learn magic_qingyuan_jianmang ch08`）、暗器（`give weapon_wuming_sixian ch07`）、剑（`give weapon_lieyang_jian ch06`）；`BATTLE_EXTRA_MEANS`：① `("木","give","talisman_bixie_shenlei","ch12")`、`("剑","role_weapon","剑","zi_ling_xutian")`、`("火","role_magic","magic_moyan_huo","yuan_yao_heipao")`；② `("火","role_magic","magic_qinghuo_lei","yuan_yao_jiedan")`；③ `("水","role_magic","magic_bingjian_shu","wu_chou")`、`("火","role_magic","magic_moyan_huo","xuan_gu")`；④ `("木","give","talisman_bixie_shenlei","ch12")`；`MEANS_LAST_CHAPTER = 12`。登之前按 `role_means_available`（`validate.py` 第 2475–2498 行）核：role 在该编成里是 ally；`role_weapon` 要 `zi_ling_xutian.weapons` 含「剑」；`role_magic` 要法术在该 role 的 `magics` 里、五行对得上、`power > 0`、无 `effect`、`needRealm` ≤ role 境界、`needMp` ≤ role 的 `maxMp`（魔火 12 / 12、`FoundationEarly`；青火雷由内容路 B 新建时照这几条定）。
- **selftest 探针**（`tools/validate_selftest.py`）：③ 写实后乌丑是友军，两处「全仓唯一含 `wu_chou` 敌方的编成」都会红，一起挪到 `wuji_haishou` ＋ `b13_waixinghai_lieyao`（2026-10-12 grep：全仓唯一含它的编成）——第 1801–1830 行：`outside_role` → `data/roles/wuji_haishou.json`、`outside_battle` → `b13_waixinghai_lieyao`，先验改「`chapter == 13` 且敌方含 `wuji_haishou`」，`probe["chapter"] = 12`，措辞改「第 13 章」；第 2495–2526 行：`future_id`、`wu_chou` 同法替换，`scoped(13)` 应静、`scoped(12)` 应报，用例名里的「第12章」「移入第11章」随之改；第 2466 行 `MEANS_LAST_CHAPTER == 11` 与用例名「末章11」改 12。临时写回的是测试副本，还原字节相同。
- `tests/LexiconTests.cpp` 表一补笔记 M 表 83 词（形如 `{"词", 首见章, "出处"}`）＋ `{"紫罗极火", 731, …}` 禁词；表二 `{12, 499}`（第 441 行）已在，不动。
- `data/flags.json`：笔记 J 表 **41 条**（J.1 链上 33 ＋ J.2 链外 8）在内容路派工之前登记，现有 `ch12.*` 0 条（2026-10-12 grep）；`ch12.binghuo_ri` 说明写「整数日期，不是开关」、`ch12.xinjing` 写「位掩码：1 色、2 惧」；`ch12.path.<id>` 13 条等 `data/pathactions/ch12.json` 落地后登记。
- `data/chapters.json` 加 `{"number":12,"numeral_key":"ui.chapter.12.numeral","title_key":"ui.chapter.12.title","done_flag":"ch12.done"}`；`data/text/ui.json` 加 `ui.chapter.12.numeral`「第十二章」、`ui.chapter.12.title`「虚天殿」（E 表 2）。
- `data/text/battles.json`：**删**第 6 行 `ch12.battle.xutian_hudian_kuilei.intro` 与第 7 行 `ch12.battle.neidian_duoding.intro`——③ 的开场白由内容路 C 写进 `data/text/ch12_battle.json`，旧行不删就是重复 key，`loadGameData` 整个起不来（第 11 章实现记录同一个坑）。
- **删占位**（E 表 18、O-7）：`data/roles/xutian_hudian_kuilei.json`、`data/roles/duoding_xiushi.json`、`data/battles/b12_xutian_hudian_kuilei.json`（协调者）；与内容路 A 删 `looks.json` 两条、内容路 B 就地改写 `b12_neidian_duoding.json` **同一批落**，免得门禁中途报悬空引用。
- `trigger_caiyao` 的灵草 id（O-13）：从 `data/items/herbs/` 选一种年份高的现有灵草，派工前告诉内容路 B（地图对象）与 C（脚本 `give` 2）。
- **KeyPrompts 白名单**（`tests/KeyBindingTests.cpp` 第 556、605 行两组 `allowed`）：本章文案 0 处占位则不动；若某条 `objective.ch12.*` 要带 `{key.*}`，两组各加一条、`.pad` 版同写、先验计数随之加。
- 回写（笔记 O.4）：大纲第 12 章四处（B1、B2、B3、B5）、lore 三处（B2、B6、B18）、人物总表四处（B7、B12、B13、B14）；第 11 章集成之后再做的两件：`wu_chou.json` note（O-11）、第 11 章文案「半幅」二字（O-12）。`docs/handoff.md` 第 3–4 节按惯例更新。

### 5.3 与既有测试的依赖（谁改哪一处）

| 依赖 | 现状 | 改法 | 谁 |
| --- | --- | --- | --- |
| `tests/Ch11*`（第 11 章测试路写的通关 / 验收）| 与 `ch11-end-{first,second}.sav` 逐字段比；章节卡断「未完待续」 | `scripts/ch11/zhuzhong.lua` 末尾补 `teleport("ch12_tushan")` 之后重生成 `ch11-end-*`（`mapId ch12_tushan`、`position` 为 `andeng`）；章节卡断言改 `Opening(12)` | 测试路 |
| `tests/UiStyleTests.cpp` `ChapterTableData` / `ChapterCards` | 已钉第 11 章（2026-10-13：第 203 行旗标表到 `ch11.done`、第 241–242 行 `ch11.done` → `{Closing,11}` ＋ `ToBeContinued`）；**第 232 行断言 `ch12.done` 不排卡** | 表长 12、末项「第十二章」「虚天殿」`ch12.done`；最后一章改 `ch12.done` → `{Closing,12}` ＋ `ToBeContinued`，`headline`「第十二章　终」、`kicker`「虚天殿」；**第 232 行那条改成断 `ch13.done` 不排卡**（不改就红） | 测试路 |
| `tests/ObjectiveTests.cpp` `kScriptTransfers` | 第 594 行起；第 11 章 6 条（第 670–675 行：三元 5、五元 1） | 三元 9 条：`ch11.done`（`scripts/ch11/zhuzhong.lua` → `ch12_tushan`）、`ch12.lidao`（`lidao.lua` → `ch12_dadian`）、`ch12.chuansong`（`chuansong.lua` → `ch12_guiwu`）、`ch12.xiaojie`（→ `ch12_xiaojie`）、`ch12.xiagu`（→ `ch12_binghuo`）、`ch12.shidian`（→ `ch12_baoguang`）、`ch12.quba`（→ `ch12_taqian`）、`ch12.cengjie`（→ `ch12_hanlitai`）、`ch12.canhun`（→ `ch12_lingquan`）；五元 3 条 `{"ch12.tupo", "scripts/ch12/zoupian_a.lua", "ch12_guiwu", "trigger_zoupian_a", "ch12_guiwu"}`、`_b`（`zoupian_b.lua`）、`_c`（`zoupian_c.lua`）——`tableRowProblems` 按「一行一个触发器、脚本名对上」核（第 872–886 行）。O-5：先核 `ObjectiveRoute` 不把同图带坐标的走偏当成链上一步 | 测试路 |
| `tests/ObjectiveTests.cpp` `kOffChainDoorKeys` | 只有 `ch01.done` | 不动：唯一一扇门 `portal_dixue` 的钥匙 `ch12.tuizhu` 在链上 | — |
| `tests/NpcPresenceTests.cpp` `kOffChainFlags` / `kLastChapter` | 第 103–127 行表已含第 11 章各条；第 133 行 `kLastChapter = 11` | 登笔记 J.2 八条（`after` / `before` 按脚本里置旗先后核）；`kLastChapter` 改 12 | 测试路 |
| `tests/PathActionTests.cpp` `realmCapOfChapter()` | 第 895–903 行已到 `{11, Realm::CoreEarly}` | 加 `{12, Realm::CoreEarly}`；本章 13 条全 `realm 0` | 测试路 |
| `tests/ChapterFixture.h` | 读 `ch0N-end` 夹具 | 加 `ch12-end` 两份（v9，本章不加存档字段）| 测试路 |
| `tests/Ch12EngineTests.cpp` 借用的现成编成 | E1 第 2 条借 `b11_tiandu_shi`，E2 第 1、3 条借 `b08_lingkuang_shouzhen` / `b08_zhongrudong` | 日后谁改这三张编成的友军或 `xuan_le` 的架势 / 蓄势，先改本文对应判据 | 引擎路 |
| `tests/RealmTests.cpp` 第 210 行（`wu_chou` 900）、`tests/Ch11EngineTests.cpp` | 锚点与第 11 章判据 | 不动（`wu_chou.json` 一个字不改）| — |
| `tests/BattleRewardTests.cpp` 全目录扫描 | 扫 `data/battles/**` 的掉落 | 本章四场 `drops []`，不受影响 | — |

---

## 6. 白名单（按文件分；两两不重叠，共享文件归唯一一路）

### 6.1 引擎路

- `tests/Ch12EngineTests.cpp`（新；E1–E2 全部判据）
- `docs/interfaces-p3-ch12.md`（只填 1.4、2.4 与末尾「实现记录」）
- `src/**`：**不在白名单**。判据红了先报协调者、改笔记与本文，再另派。

### 6.2 内容路 A（地图美术）

- `maps/ch12_{tushan,dixue,dadian,guiwu,xiaojie,binghuo,baoguang,taqian,hanlitai,lingquan}.tmj`（新 10 张）、`maps/tilesets/terrain_ch12_*.tsj`（生成器要的话，新）
- `tools/mapgen/genmaps_ch12.py`（新）、`tools/mapgen/genmaps.py`（只加登记，排在第 11 章之后）
- `data/visual/maps.json`、`data/visual/battles.json`（只加条目，主题时辰背景只用现有名）、`data/visual/looks.json`（加本章条目；**删** `duoding_xiushi`、`xutian_hudian_kuilei` 两条，与 5.2 删占位同一批）
- `tools/artgen/sprites_enemies.py`（只加 H 表那 9 个非人形）、`assets/art/**`（artgen 产物）
- `tools/audiogen/catalog.py`（只在 `MAP_BGM` **上方的注释表**加 10 行 `# ch12_xxx -> bgm_yyy`；`MAP_BGM` 是曲子 id 元组、第 58 行断言每项都在 `BGM` 里，全用已有曲子时元组不动——把地图 id 加进元组，模块一导入就断言失败）、`docs/audio.md`（同一张表）

### 6.3 内容路 B（数据）

- `data/roles/*.json`（新建 H 表 32 个；旧 role 一个不动，删除归协调者）
- `data/items/**`（新建 L.1 那 27 件）、`data/magics/*.json`（新建 L.2 那 3 门；不建 `magic_xiuluo_shenghuo`）
- `data/battles/b12_{guiwu_guiwang,heishamo_tiehuoyi,hanlitai_xuangu}.json`（新）、`data/battles/b12_neidian_duoding.json`（就地改写）
- `data/quests/q12_{maigu,kuilei_canhai}.json`、`data/pathactions/ch12.json`、`data/objectives/ch12.json`
- **平衡路**不另开白名单、不与 B / C 并行：**B、C 两路都收尾之后串行接手**，只改上面这些 role / battle 文件的数值，测量记录写进编成的 `note` 字段；施工图第 18 节（归 C）的「施工偏差」由协调者从 `note` 并入。

### 6.4 内容路 C（脚本文案）

- `scripts/ch12/**`（新）；`scripts/ch11/zhuzhong.lua`（**只在末尾 `flag.set("ch11.done")` 之后补一行 `teleport("ch12_tushan")`**，此前一个字不动）
- `data/text/ch12*.json`（新，按前几章的拆法：正文、`ch12_battle.json`、`ch12_path.json` 等；0 处 `{key.` / `{pad.`）
- `docs/ch12-design.md`（**只追加「施工偏差」一节**；平衡路实测不写这里，见 6.3 末条）

### 6.5 协调者

- `tools/validate.py`（**只** `CHAPTER_MEANS` / `BATTLE_EXTRA_MEANS` / `MEANS_LAST_CHAPTER` 那一段）、`tools/validate_selftest.py`（只挪两处探针与第 2466 行那一处断言）
- `data/flags.json`；`data/chapters.json`；`data/text/ui.json`（「第十二章」「虚天殿」）；`data/text/battles.json`（**只删**两条旧占位）
- `data/roles/xutian_hudian_kuilei.json`、`data/roles/duoding_xiushi.json`、`data/battles/b12_xutian_hudian_kuilei.json`（**只删**）
- `tests/LexiconTests.cpp`（表一补词）；`tests/KeyBindingTests.cpp`（只在 5.2 那一条触发时）
- `docs/handoff.md`、`docs/tech-debt.md`；仓库根 `docs/大纲.md`、`docs/lore/**`（含 `人物总表.md`，回写）

### 6.6 测试路（引擎路与内容路都落地、协调者 5.2 之后另派）

- `tests/Ch12*.cpp`（除 `Ch12EngineTests.cpp`）：通关、挂点、编成、账、切片、验收、章内先后词表专项（笔记 M 表末段）
- `tests/Ch11*.cpp`（除 `Ch11EngineTests.cpp`；只改通关终点与章节卡断言）、`tests/UiStyleTests.cpp`（只改 `ChapterTableData` / `ChapterCards` 那两处）
- `tests/ObjectiveTests.cpp`、`tests/NpcPresenceTests.cpp`、`tests/PathActionTests.cpp`（只按 5.3）
- `tests/ChapterFixture.h`、`tests/fixtures/ch11-end-{first,second}.sav`（重生成）、`tests/fixtures/ch12-end-{first,second}.sav`（新）、`saves/README.md`

---

## 7. 环境与纪律（沿用 `docs/handoff.md` 第 8 节与附录第 9、10 节）

- **次序**：协调者登记 J 表旗标、选灵草 id → 引擎路（E1–E2 判据）∥ 内容路 A / B / C → 协调者 5.2 与集成（删占位同一批）→ 测试路（先改第 11 章通关终点、重生成 `ch11-end`，再写第 12 章通关）→ 独立校对。**本章在第 11 章集成之后开工**；第 11 章仍在同一棵工作树里集成时，各路只碰自己白名单里的文件。
- **跨路依赖**：引擎路与内容路之间没有字段依赖（本章 0 个代码项）；E1 第 5 条、E2 第 4–5 条要等内容路 B 落地，之前 `GTEST_SKIP`（集成后已改成 `FAIL()`）。
- 每路用自己的构建槽：引擎路 `ch12eng`、内容路 `ch12content`、测试路 `ch12test`；**用 PowerShell 调 `build_logged.bat <槽名>`**，bash 里 `cmd.exe /c` 会静默假绿；日志 `build-<槽名>.log` 按 gb18030 读字节，读之前核对 mtime。几路共用一棵工作树，验证时序相关的结论用自己槽位的构建目录，别信公共的 `build\`。
- **门禁先于编译跑**：内容路每一批落地前单跑 `python tools/validate.py`、`python tools/mapgen/genmaps.py --check`、`python tools/artgen/artgen.py --check`；引擎路交付前跑 `python tools/validate_selftest.py`（仓库本身不净时它连锁 FAIL，照第 11 章实现记录写明与本路无关的条目）。
- 改了 `tools/artgen/` 或 `data/visual/` 必须重生成 `assets/art/**` 一并落地；`--check` 逐像素比。
- **旧图不 patch**：本章 0 张；`genmaps_ch05.py`–`genmaps_ch11.py` 一个字不改。**旧脚本只** `scripts/ch11/zhuzhong.lua` 末尾补那一行 `teleport`；`scripts/ch12/lingquan.lua` 不 teleport（留给第 13 章 patch）。
- 源码一律 UTF-8 无 BOM、LF；含反斜杠的文件不用 heredoc 写；不把中文写进 `python -c`。
- 原著文本只在本地核对事实（`tools/novel/novel.py`），不复制、不进仓库；对白字句自己写，换词不算重写。
- **不派子代理**，不整份重读大文件，报告精炼。**不碰 git**：各路不 add / commit / stash / checkout，提交由协调者按 `docs/handoff.md` 第 3 节的流程做；不动 `fanren/build*` 目录。

---

## 实现记录

- **引擎路（2026-10-13）**：只新增 `tests/Ch12EngineTests.cpp`（23 条：E1 11、E2 12；其中负向 / 对照 9 条单列，另有用例内负向段），`src/`、`tools/` 零改动。构建槽 `ch12eng`：`build_logged.bat ch12eng` 停在门禁（`build-ch12eng.log`，`EXITCODE=1`：`validate.py` 27 错，全是别路在途——`ch12.path.*` 13 面未登记 ×2、`ch12_battle.json` 与 `battles.json` 重复 key 1，均归协调者 5.2）；绕过门禁的编译 `build-ch12eng-compile.log` `EXITCODE=0`、零警告。`Ch12*` 在仓库数据上 3 绿 20 红（全是上面那条重复 key 让 `Application::init` / `loadGameData` 失败），在删掉两条旧 intro 的数据快照上 23/23 绿、0 跳过。负向自检四处见 1.4、2.4。
- **协调者集成（2026-10-13）**：数据已齐，`tests/Ch12EngineTests.cpp` 的 11 处 `GTEST_SKIP()` 全改成 `FAIL()`（`notes/ch12/integration.md`）：缺文件或 ③ 仍是旧占位即红，不再静悄悄跳过。1.3、1.4、2.3、2.4 与文件头注释的「跳过」说法在整改第 1 轮（门禁审查 G-4）改成了这一现状。
- **整改第 1 轮·测试与门禁路（2026-10-13，构建槽 `ch12tfix`）**：2.3 第 4 条补「友军 `actions == role.actions`、玄骨恰 2」（③ 真场景用例，`continue` 改 `ASSERT_NE`），第 6 条补阴性对照 E（记录见 2.4 末条）；2.2 补 `maxMp ≥ 12`。`src/` 零改动。

