# 第 7 章第二期「同道录」2a·路 K（规则与数据）施工记录

> 2026-11-21。施工图：`docs/ch07-tongdaolu.md`（第 2、3 节与第 4 节路 K 那一行）。
> 本文同时是**路 U、路 N 的接口文档**：第 1 节是伙伴表第二版的形状（带一份完整示例），第 2–6 节是规则、战斗、脚本、game 层的入口，
> 第 7 节是旗标，第 9 节是留给两路的注意事项。
> 分支 `claude/ch07-tongdao-k-c631ae`，不推送、不合 main。构建目录 `build-k`。

---

## 0. 做了什么（一句话一样）

| 施工要求 | 落在哪 |
| --- | --- |
| 伙伴表第二版（kind、线与档名、封顶、`roles[i]` 下的按档法术 / 被动 / 合招、在册加成、切磋数据） | `core/model/Types.h`（伙伴那一块末尾）、`io/CompanionLoader.cpp`、`tools/validate.py` 规则 31 |
| 档位效果：三档第二门法术 + 被动，四档第三门法术，五档被动加强一级；建场按档过滤 | `rules::companionKit`（`core/rules/Tongdao.*`）、`BattleScene::addPartyUnits` |
| 被动做成有限的种类表，数据只许引用表里的 | `rules::perkTable`；加载器与门禁脚本各查一遍 |
| 在册加成：解锁、按境界封顶、超额按勾选；建场时套上 | `rules::activeRosterBonuses` / `rules::battleBonuses`、`BattleScene::buildFromSetup` |
| 同修、切磋、请教、叙话的记账与每日限制；赠礼可对不入队的在册人物记账 | `core/rules/Tongdao.*`；`Application::tongxiu` / `startSpar` |
| 切磋编成 `spar: true` | `core::BattleSetup::spar`、`io/BattleLoader.cpp`、`SetupOptions::spar`、`BattleScene::settle` |
| 统一生效判据 | `rules::rosterEffectsActive(state)` |
| 脚本接口 + `show_cg` / `hide_cg` | `scripts/common/api.lua` 末尾两块、`script/Command.h`、`script/ScriptHost.*`、`Application::dispatch` |
| 南宫婉第 7 章封在四档（默契到 94 为止） | `rules::bondCap` + 每个记账入口的 `cap` 参数；`data/companions/nangong_wan.json` 的 `bond_cap` |
| 新法术要用的五种效果（两段伤、回血、护持、剑幕、定身；主会话 2026-11-21 答复：归路 K，做成有限的表） | `core::Magic::hits / amount`、`MagicEffect::Heal / Ward / Bulwark / Bind`，`core/battle/*`、`io/DataLoader.cpp`；战斗菜单给我方选目标的最小适配（`BattleScene`）；自动打法会回血（规则层 AI 与 `tests/BattleHand.h`） |

存档版本没升（仍是 10）；新状态全在 `flags` 与 `bonds`。最短路线上没有加任何旗标、物品、入队离队、默契收入。

---

## 1. 伙伴表第二版（`data/companions/<id>.json`）

### 1.1 字段

旧字段（`id, roles[{role, until_flag}], met_flag, field_skill, chase, duo, gifts, note`）意思不变。新字段**全部可不写**，不写就是改版前的行为。表外字段照旧报错（C++ 加载器与门禁规则 31 同一张表）。

| 字段 | 形状 | 缺省 | 意思 |
| --- | --- | --- | --- |
| `kind` | `"person"` / `"beast"` | `person` | 人物还是灵兽（灵兽 2b 才有内容）。灵兽不切磋、不做同修 / 请教 / 叙话 |
| `line` | `"general"` / `"hongyan"` / `"moni"` / `"beast"` | 人物 `general`，灵兽 `beast` | 档名走哪条线（施工图 3.2）。灵兽必须是 `beast`，人物不许是 `beast` |
| `bond_cap` | `{"until_flag": 旗标, "max_tier": 1–4}` | 不封顶 | `until_flag` 置位之前默契最高到第 `max_tier` 档（数值到下一档门槛减一：四档 = 94） |
| `roster_bonus` | `{"kind": 种类, "value": N, "element"?: 五行}` | 没有 | 在册加成（3.6）。只许第 2 节表里能当「在册加成」的种类 |
| `spar` | `{"battle": 编成 id, "prizes"?: [{"tier": 2–5, "items": [{"item": id, "count"?: 1–99}]}]}` | 不能切磋 | 切磋的编成（必须 `spar: true`）与各档首胜奖品。`prizes` 按 tier 严格升序，每档至多一条 |
| `roles[i].magics` | `[{"tier": 2–5, "magic": 法术 id}]` | 没有 | 这一段按档解锁的法术。一档就会的写在角色表的 `magics` 里，**不写在这里** |
| `roles[i].passive` | `{"kind": 种类, "values": [三档起, 五档起], "element"?}` | 没有 | 这一段三档起的被动；`values` 一项或两项，第二项是五档「加强一级」后的数（不得小于第一项）。只许能当「伙伴被动」的种类 |
| `roles[i].duo` | 法术 id | 用顶层 `duo` | 这一段的合招（第 7 章里覆盖顶层的；第 7 章以外照用顶层的） |

`element` 写 `metal` / `wood` / `water` / `fire` / `earth`，只给要它的种类（`element_stagger`），别的种类写了报错。

**`roles[i]` 下的东西只写给第 7 章的角色**（`chen_shimei`、`baiyi_shaonv`、`nangong_wan_ch07`、菡云芝第 7 章的角色）。第 8 章起的角色不写。
即使写了，第 7 章以外也不生效（第 3 节的判据）——`chen_shimei` 第 8 章还在用，靠的就是这一条。

### 1.2 完整示例

`tests/TongdaoLoaderTests.cpp` 的 `kFullExample` 是这一份，一字不差地读得进来（里面的法术 id 是占位，等路 N 起名）：

```json
{
  "id": "chen_qiaoqian",
  "kind": "person",
  "line": "hongyan",
  "roles": [
    {
      "role": "chen_shimei",
      "until_flag": "ch08.some_flag",
      "magics": [
        {"tier": 3, "magic": "magic_shuangjian"},
        {"tier": 4, "magic": "magic_jianmu"}
      ],
      "passive": {"kind": "chase_power", "values": [20, 30]},
      "duo": "magic_duo_jianhe"
    },
    {"role": "chen_qiaoqian"}
  ],
  "met_flag": "ch07.fanhui",
  "bond_cap": {"until_flag": "ch07.done", "max_tier": 4},
  "field_skill": "see_through_illusion",
  "chase": true,
  "duo": "magic_duo_old",
  "gifts": {"like": ["material_huolong_cao"], "dislike": ["material_yaoshou_cai"]},
  "roster_bonus": {"kind": "ambush_bp", "value": 1},
  "spar": {
    "battle": "spar_chen_qiaoqian",
    "prizes": [
      {"tier": 2, "items": [{"item": "pill_huiqi_dan", "count": 2}]},
      {"tier": 3, "items": [{"item": "talisman_feixing_fu"}]}
    ]
  },
  "note": "示例"
}
```

### 1.3 现有两份表迁成了什么

| 表 | 加了什么 | 行为 |
| --- | --- | --- |
| `chen_qiaoqian` | `kind: person`、`line: hongyan` | 不变（档名的 key 换了线，但菜单现在还用旧 key，见 9.1） |
| `nangong_wan` | `kind: person`、`line: general`、`bond_cap: {until_flag: ch07.done, max_tier: 4}` | 第 7 章里默契到 94 为止（用户 2026-11-21 定）；别的不变 |

两份都**没有**新法术、被动、`roles[i].duo`、在册加成、切磋——数值与文案归路 N。`tests/TongdaoLoaderTests.cpp` 的 `TheTwoCompanionsMigratedWithoutNewSpellsOrPassives` 钉着这一点，路 N 填数时把它改掉。
南宫婉的合招仍挂在顶层 `duo`（第 8 章起也在用），没有搬进 `roles[i]`。

### 1.4 切磋编成（`data/battles/spar_*.json`，归路 N）

多一个字段 `"spar": true`。开了的编成加载器要求：`defeat_is_fatal: false`、`can_escape: false`、不许 `hero_absent`、不许有 `ally`、`rewards` 全 0（奖品写在伙伴表的 `spar.prizes`）。敌方写她那一段的角色（例如 `chen_shimei`）；
她身上要有架势与破绽才打得出破势——角色表本来不写（我方用不上），**路 N 要么给她的角色表补 `toughness` / `weaknesses`（我方不读这两项，不影响她当伙伴），要么另建一份切磋用的角色**。另建的话那份角色不在伙伴的 `roles` 里，按档解锁的法术就带不上（见 4.4）。

---

## 2. 被动与在册加成的种类表

规则层 `rules::perkTable()`（`core/rules/Tongdao.cpp`）。数据只许写这六种；数值范围是「写成这样肯定写错了」的护栏，不是平衡。

| 名字 | 放哪 | 取值 | 效果 | 施工图出处 |
| --- | --- | --- | --- | --- |
| `chase_power` | 伙伴被动 | 1–30（%，主会话定的上限：破势本来就翻倍） | 她追击的伤害 +N%（加在公式基数上，再吃破势 ×2） | 3.3 陈巧倩「剑心」，改了，见 8.1 |
| `opening_bp` | 伙伴被动 | 1–2 | 她在场时我方全员开场多 N 点劲；与先手的 1 点、同修的 1 点**不叠加，取大** | 3.3 南宫婉 |
| `victory_heal` | 伙伴被动 | 1–50（%） | 打赢（Won）那一刻，我方站着的人各回 maxHp 的 N%（至少 1）；倒下的不回 | 3.3 菡云芝「灵符」 |
| `ambush_bp` | 在册加成 | 1–2 | 偷袭明雷得手（先手）时我方全员开场**再**多 N 点劲（叠在先手上）；不是先手不读 | 3.6 陈巧倩 |
| `element_stagger` | 在册加成，要 `element` | 1–2 | 主角自己施法（含他发起的合招）这一属打中破绽时，在 1 点之外多削 N 点架势；符不算 | 3.6 南宫婉（`fire`） |
| `talisman_power` | 在册加成 | 1–50（%） | 用符（带 `castMagic` 的物品）施展的威力 +N%（不论谁用） | 3.6 菡云芝 |

同种几位都有：伙伴被动取大（两位都会回血也不翻倍）；在册加成相加。开场劲封顶 5（`kBpMax`）。

---

## 3. 规则函数（`core/rules/Tongdao.h`、`Companions.h`）

全是纯函数，读写的只有传进去的 `GameState`。

| 函数 | 做什么 |
| --- | --- |
| `rosterEffectsActive(state)` | **统一生效判据**：`ch06.done` 置了、`ch07.done` 没置。按档法术、`roles[i].duo`、被动、在册加成、同修的劲都只在它为真时套用 |
| `bondLineName` / `bondLineFromName` | 线的数据名 |
| `bondTierNameKey(line, tier)` | 档名文案 key：通用线 `ui.bond.tier<N>`（沿用旧 key），其余 `ui.bond.<line>.tier<N>`（`data/text/ui_companions.json` 已加） |
| `bondCap(companion, state)` / `bondCapOf(companions, id, state)` | 此刻的默契上限（94 或 100） |
| `bondTierCap(companion, state)` | 此刻最高到第几档（同道录里「机缘未到」那一格据此画） |
| `addBond / addBattleBond / giveGift(..., cap = kBondMax)` | 旧的三个记账入口多一个 `cap`；不传就是旧行为。高过 cap 的旧账不往下压 |
| `perkTable / perkInfo / perkByName / passiveValueAtTier` | 种类表 |
| `companionRoleEntry(companion, roleId)` | 伙伴表里那一段 |
| `companionKit(companion, roleId, bond, active)` | 她用这一段、这个默契上场带的：够档的额外法术、合招、够三档的被动（值已换成这一档的） |
| `qingjiaoFlag / xuhuaFlag / sparDayFlag / sparPrizeFlag / bonusOnFlag(id)`、`kTongxiuDayFlag`、`kTongxiuBonusDayFlag` | 旗标名（第 7 节） |
| `rosterBonusSlots(realm)` | 名额：炼气（及以下）4，筑基及以上 6 |
| `rosterBonusUnlocked(companion, state)` | 在册、写了 `roster_bonus`，人物请教过 / 灵兽到二档 |
| `activeRosterBonuses(companions, state)` | 此刻生效的是谁的：不超名额全生效；超了先取勾上的，再按 id 次序补满 |
| `tongxiuBonusReady(state)` | 今天有没有同修的劲 |
| `battleBonuses(companions, state, fieldedPassives, spar, heroPresent)` | 一场仗开场的修正（`BattleBonuses`）。判据不成立、切磋、主角不在场时全 0 |
| `tongdaoCheckCode(check)` | 机器码（第 5 节） |
| `inRoster` | `met_flag` 置了 |
| `canTongxiu` / `tongxiu` | 同修：人物、今天没同修过（全局每天一次）。默契 +4（受封顶）、记今天、把劲挂到明天。**不过日子**（调用方 `advanceDays(1)`） |
| `canSpar` / `recordSpar(..., won)` | 切磋：人物、表里有编成、二档、今天没跟她切磋过。赢 +4 输 +2；赢了且开打时那一档的首胜没领过 → 发奖品、记下那一档 |
| `canQingjiao` / `recordQingjiao` | 请教：人物、三档、没请教过 |
| `xuhuaPending` / `recordXuhua(..., tier)` | 叙话：下一段该演第几档（2 起、不高于此刻的档、比演过的高的最低一档）；记「演过」只升不降 |
| `giveGiftTo(companions, id, item, state)` | 赠礼：在册就行，不必在队伍里；账照旧走 `giveGift`，带封顶 |

## 4. 战斗层与建场

### 4.1 法术的新东西（`core::Magic`，`data/magics/*.json`）

| 数据字段 | 意思 | 规矩 |
| --- | --- | --- |
| `hits` | 基础段数（1–5，缺省 1）。每段单独算伤害、单独判破绽；连发型蓄劲在它上面加 | 只许写在伤人的法术上 |
| `"effect": "heal"` + `amount` | 给一名我方（含主角、含她自己）回 `amount`（1–999）点气血；满血的、倒下的拦下来（救不起倒下的），不扣法力 | `power` 必须写 0 |
| `"effect": "ward"` | 给一名我方挂护持：挡下他受到的下一击（伤害记 0，破绽照判）；**到下一回合结束还没用上就散**；还挂着的不能再挂 | 同上 |
| `"effect": "bulwark"` + `amount` | 施法者这一边全员到**本回合结束**受伤减 `amount`%（1–90）；不挑目标 | 同上 |
| `"effect": "bind"` | 令一名敌人跳过它的**下一次**行动；首领（一回合多动或会蓄势）与不能逃的仗里的敌人（精英、剧情硬仗）定不住；**被定过的随后两回合不再吃定身** | 同上 |

效果的种类就是 `MagicEffect` 这张枚举表，数据里写表外的名字加载器报错。四种辅助效果都扣法力（符施展的不扣）、不吃蓄劲（蓄了也只当没蓄）、不是一击。

**状态现在怎么存**：每个单位身上一个 `Unit::Status`（`core/battle/Battle.h`）——护持记「还能免几次」（`wardCharges`，现在至多 1）与「到哪一回合为止」（`wardUntilRound`）；剑幕记百分比与那一回合；定身记「还要跳过几次」（`bindSkips`）与「到哪一回合为止不吃定身」（`bindImmuneUntilRound`）。都是「几回合、几次」的最小状态，不是增益减益框架；第三期做六种增益减益时把这个结构扩成那张表（定身、护罩直接接过去）。

**AI 与自动打法**：会回血法术的我方单位，场上有我方气血不到四成就先救最危的那一位（`BattleState::allyDecision` 与测试那只手 `tests/BattleHand.h` 同一条）；护持、剑幕、定身 AI 不用。没有这几种法术的单位一个字节也不多判，现有各章的打法不变。敌方 AI 照旧只用伤人的法术——切磋里的她也不会放辅助法术。

**菜单（`BattleScene`，最小适配）**：回血、护持的择敌列表只列我方（含自己）；剑幕像看破一样选中就出；法术行尾写「回气血 N」「挡一击」「全队减伤 N%」「定身」「N 段」。演法另见 4.2。

### 4.2 战斗事件（`BattleEventKind`，追加在末尾）

`Ward`（挂上护持）、`WardBlock`（护持挡下一击，紧跟那一击的 `Hit`，`value` 0）、`Bulwark`（`value` = 减几成）、`Bind`（定住）、`BoundSkip`（轮到它时被跳过）。回血走现成的 `Heal`（打赢回血那一条 `actor` 是 -1）。
`BattleScene::playEvent` 里现在只给它们飘一句字（「护持」「挡下」「剑幕」「定身」「动弹不得」）——**演法归路 U**。

### 4.3 开战参数与单位字段

`SetupOptions` 追加：`openingBp`、`ambushBp`、`victoryHealPercent`、`talismanPowerPercent`、`spar`。
`Unit` 追加：`chasePowerPercent`、`extraStagger`（只挂在主角身上）、`status`（4.1 末）。缺省全是「没有」。
开场劲 = `kBpStart + max(先手 ? 1 : 0, openingBp) + (先手 ? ambushBp : 0)`，封顶 5。
`BattleState::spar()`、`BattleState::bindImmune(i)` 是新的只读查询。

### 4.4 建场（`game/BattleScene.cpp`）

- 队伍里带进来的伙伴：`companionKit` 给的额外法术追加进她的 `magics`（菜单、登记表、测试的自动打法看的都是这一份，所以只看得见够档的）；`duo` 换成 kit 的；`chase_power` 挂到她身上。
- 开场修正：`rules::battleBonuses` 一处算，照抄进 `SetupOptions`；`element_stagger` 挂到主角单位上；用掉同修的劲就把 `kTongxiuBonusDayFlag` 清 0。识海、韩立不在场、切磋一律不套。
- 切磋：不带队伍；对面的她如果属于某位伙伴，按此刻的默契带够档的法术（被动不带——敌方没有劲，追击、合招也不对她生效）。编成里写死的别的单位照旧不看伙伴数据。
- 收场（`settle`）：切磋**只**做一件事——主角气血、法力回满。不写回伙伴气血、不记破绽、不发编成奖励、不记「并肩打赢」。输赢的账在 `Application::startSpar` 的回调里记。

## 5. 脚本接口（`scripts/common/api.lua` 末尾两块）

读走 `__host`，写走命令队列；参数是伙伴 id。动作返回 `ok, code`，`code` 是 ASCII 机器码，成功为空串：
`no_companion` / `not_met` / `not_person` / `tier_low` / `done_today` / `already`（只做一次的做过了、叙话没有待演的）/ `no_spar`。

| 调用 | 返回 | 说明 |
| --- | --- | --- |
| `tongdao.effects_active()` | bool | 生效判据 |
| `tongdao.roster()` | id 数组 | 同道录上的伙伴，按 id 排 |
| `tongdao.in_roster(id)` / `kind(id)` / `line(id)` | bool / `"person"`… / `"general"`… | 没有这位：false / 空串 |
| `tongdao.tier(id)` | 1–5 | 同 `bond.tier` |
| `tongdao.tier_name(id, tier?)` | 文案 key | tier 省略取此刻的档 |
| `tongdao.cap_tier(id)` | 0–5 | 此刻最高到第几档；没有这位 0 |
| `tongdao.can(action, id)` | ok, code | action：`"tongxiu"` / `"spar"` / `"qingjiao"` |
| `tongdao.qingjiao_done(id)` | bool | |
| `tongdao.xuhua_pending(id)` / `xuhua_done(id)` | 档 / 0 | 下一段该演第几档 / 已演到第几档 |
| `tongdao.bonus_unlocked(id)` / `bonus_active(id)` / `bonus_selected(id)` / `bonus_slots()` | | 在册加成 |
| `tongdao.tongxiu(id)` | ok, code, 加了多少 | 记账 + 过一天 |
| `tongdao.spar(id)` | won, code, 加了多少 | 开她那一场，**等打完**；开不成 won 为 false、code 是上面那几种；打完 code 与 `battle()` 相同（`""` 赢 / `"lost"` 输） |
| `tongdao.qingjiao(id)` | ok, code | 事件脚本演完对白后调 |
| `tongdao.xuhua_mark(id, tier)` | ok, code | 事件脚本演完调 |
| `tongdao.bonus_select(id, on)` | ok, code | 勾选 / 取消 |
| `show_cg(id)` / `hide_cg()` | — | 就地完成；只记下 id（`Application::currentCg()`），画面归路 U，现在屏幕上什么都不变 |

赠礼没有脚本口：界面直接调 `rules::giveGiftTo`。地点（只在百药园）由界面判，规则与脚本都不判。

## 6. game 层入口（给路 U）

| 入口 | 做什么 |
| --- | --- |
| `Application::tongxiu(id)` → `rules::BondGain` | 同修：记账，成了就 `advanceDays(1)`。蒲团的「同修」调它 |
| `Application::startSpar(id, onDone)` → `rules::TongdaoCheck` | 切磋：不成立什么都不做、返回那一条；成立就开仗，收场 `recordSpar` 记账、`announceItem` 报奖品，再调 `onDone(SparOutcome, CommandResult)` |
| `Application::currentCg()` | 事件图 id（空 = 没有）；读档、新开局、回首页清掉 |
| `rules::giveGiftTo` | 同道录里对在册人物赠礼（不必在队伍里） |
| `rules::bondTierNameKey(companion.line, tier)` | 档名 |

## 7. 新旗标（`data/flags.json` 已登记）

每位人物五面（`chen_qiaoqian`、`nangong_wan`、`han_yunzhi` 都已登记；菡云芝的伙伴表归路 N）：

| 旗标 | 值 | 谁置 |
| --- | --- | --- |
| `tongdao.<id>.qingjiao` | 1 = 请教过 | `rules::recordQingjiao` |
| `tongdao.<id>.xuhua` | 叙话演到第几档（0 / 2–5） | `rules::recordXuhua` |
| `tongdao.<id>.spar_day` | 上一次切磋的日子 | `rules::recordSpar` |
| `tongdao.<id>.spar_prize` | 首胜奖品领到第几档（0 / 2–5） | `rules::recordSpar` |
| `tongdao.<id>.bonus_on` | 1 = 在册加成勾上 | `tongdao.bonus_select` / 界面 |

全局两面：`tongdao.tongxiu_day`（上一次同修的日子）、`tongdao.tongxiu_bp_day`（同修的劲落在哪一天，开了那一场清 0）。
2b 的灵兽要用旗标时照同一个命名再登记。

## 8. 施工图里我觉得不合理或拿不准的地方

1. **「剑心：她追击时多削 1 点架势」没有效果。** 追击只打「刚进入破势」的敌人，那一刻它的架势已经是 0（`breakUnit`），破势期间不再削。改成了 `chase_power`（她的追击伤害 +N%，上限 30）。主会话 2026-11-21 同意，施工图 3.3 那一格已改。
2. **新法术要的效果施工图原先没分给任何一路**，开工时问了主会话；2026-11-21 答复：归路 K，做成有限的表，只做这五种，不做通用增益减益框架。已照办（第 4.1 节），施工图 3.3 已补一句。
3. **定身按主会话的口径**：跳过下一次行动，被定过的随后两回合不吃定身。「精英」规则层认不出（那是地图属性），用的是现成的标记——不能逃的仗（精英与剧情硬仗都写 `can_escape: false`）里的敌人一律定不住，没有另在角色表加开关。代价是第 7 章的剧情硬仗里定身符也没用。
4. **切磋打完「双方回满」**照写了：主角气血、法力回满。这等于百药园里多了一个免费的回满——要是不想要，改成「放回开打前的数」只动 `BattleScene::settle` 一行。
5. **切磋里不套在册加成、伙伴被动、同修的劲**（施工图没说）。理由：切磋是他一人对她一人，套了她自己的在册加成去打她不合情理；同修的劲留给真打的第一场。
6. **首胜奖品按开打时的档算**；跳过的档不补发（一次升两档，中间那一档的奖品就没了）。施工图说「每到一个新档位第一次打赢」，没说跳档怎么办。
7. **同修是全局每天一次**（不论跟谁）。施工图写「每天一次（做了就是第二天）」，做了就过一天，所以实际上不会撞到。
8. **在册加成超额时**：先取勾上的，名额没满再按 id 次序补没勾的。这样「勾了一位」不会让其余几条一下子全没了。2a 只有三位人物，到不了 4 条的名额。
9. **相处的动作不看生效判据**（第 7 章以外脚本照样能调 `tongdao.tongxiu`）。只有界面会调它们，而同道录的相处入口只在第 7 章的百药园。第 8 章以后要不要开，等样板定。
10. **南宫婉的封顶会改变第 7 章里一条可能的账**：默契到 95 以上的路线现在停在 94。全量测试里没有一条走到那里（见第 10 节）。

## 9. 留给路 U、路 N 的注意事项

### 9.1 路 U

- 档名：`MenuScene` 的名册页现在仍写 `ui.bond.tier<N>`（我没动界面）。同道录页请改用 `rules::bondTierNameKey(companion.line, tier)`，陈巧倩的三档、五档才会显示「信任」「红颜」。
- 「机缘未到」：`rules::bondTierCap(companion, state) < 5` 时最高那一格这样写。
- 四种辅助法术的菜单已能用（4.1 末：回血、护持只列我方，剑幕选中就出）；画面上的演法（4.2 的新事件）归你。
- 事件图：画 `Application::currentCg()`（空串不画）。
- 同修：`Application::tongxiu(id)`；切磋：`Application::startSpar(id, onDone)`；赠礼：`rules::giveGiftTo`。都会自己判能不能做，界面只需按返回的 `TongdaoCheck` 置灰写理由。
- 在百药园之外把动作置灰，是界面的事；规则层不判地点。

### 9.2 路 N

- 伙伴表的结构归我，数值归你（施工图 4 节末）。填数时：`roles[i].magics` / `passive` / `duo`、`roster_bonus`、`spar` 按第 1 节写；被动只许第 2 节的六种。
- 菡云芝：`kind` 不写（人物）、`line: "moni"`；她的 `met_flag` 施工图说借 `ch07.wuyou`，请你核实。她的五面旗我已登记。
- 南宫婉第 7 章的角色有两段（`baiyi_shaonv` 到问名、`nangong_wan_ch07` 到章末），历练里用的是 `nangong_wan_ch07`——被动与法术写在这一段上；前一段只在主线墨蛟那一仗里出现，写不写看你（写了在那一仗里也会生效，因为那时也在第 7 章）。
- 切磋编成见 1.4：她的角色表要有架势与破绽。
- 写完数后 `tests/TongdaoLoaderTests.cpp` 的 `TheTwoCompanionsMigratedWithoutNewSpellsOrPassives` 要跟着改（那条是给 2a 交接时钉「行为不变」用的）。
- 新法术的效果见 4.1；回血量别让必打战磨不死（施工图 3.3）。
- 叙话、请教的事件脚本：演完调 `tongdao.xuhua_mark(id, tier)` / `tongdao.qingjiao(id)`；先用 `tongdao.xuhua_pending(id)` 问该演哪一段。

## 10. 测试

- 新用例 56 条，四个文件：`tests/TongdaoRulesTests.cpp`（规则层）、`TongdaoBattleCoreTests.cpp`（战斗规则层，直接驱动 `BattleState`）、`TongdaoLoaderTests.cpp`（三个加载器与真树数据）、`TongdaoWiringTests.cpp`（`Application` 真入口：建场、同修、切磋、脚本）。
- 全量 ctest：基线 2443 条全过；改完 2499 条（`build-k`，`-j 4`），结果见交回报告。
- **没改任何旧测试的断言。**动过的测试代码只有测试那只手 `tests/BattleHand.h`：加了「会回血法术的先救四成以下的人」一条、剑幕经菜单选中就出（没有这几种法术的单位一个字节都不多判）。红线用例（`Ch07CompanionsTests` 的 `No5_…`、`Ch07AcceptanceTests`、`Ch08AcceptanceTests`）原样通过。
- 门禁脚本没跑（用户定的规矩）；`tools/validate.py` 规则 31 与法术、编成的检查已跟着改（`check_companion_tongdao`、`check_companion_perk`、法术的 `hits` / `amount` / 新效果、编成的 `spar`）。

## 11. 没做的

- 灵兽（2b）：只留了 `kind: beast`、灵兽线的档名、在册加成「二档解锁」这一条；收服、随从、喂养都没做。
- 界面、事件图的画面、战斗菜单里给我方选目标：归路 U（9.1）。
- 菡云芝的表、三位的数值与文案、切磋编成、事件脚本：归路 N。
