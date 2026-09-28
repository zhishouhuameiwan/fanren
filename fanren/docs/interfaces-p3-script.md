# P3 脚本 API 增补契约（第 2 章所需）

> 本文是**契约**：引擎实现方与编剧同时照它施工，双方不得私自改动。
> 需要改签名时改本文并知会另一方，不在代码里单方面偏移。
>
> P3 · 2026-09-20

---

## 0. 为什么需要这一批

P2 造了七个规则模块（掌天瓶、灵田、修炼、经济……），P2 末尾把它们接进了 `GameState`
与两块面板。但**脚本层至今碰不到它们**：`scripts/common/api.lua` 里只有对话、分支、
物品、旗标、战斗。

第 2 章的主线要求脚本能做三件当前做不到的事：

1. **把瓶子交给玩家**（拾瓶）与**解锁催熟**（四年后误洒绿液）。这是两个独立节点，
   必须是两条独立的脚本调用。
2. **推进时间**。得瓶第八日开盖、段与段之间跨半年到一年，都要脚本说了算。
3. **查询循环状态**作为剧情闸门条件，例如「种下过至少一株」「绿液攒够一滴」。

没有这三样，第 2 章只能退化成一串纯对话，核心循环就闭合不了。

---

## 1. 新增命令（`src/script/Command.h`）

追加到 `CommandKind` **末尾**，不得插在中间（现有存档与测试按序号比对）。

| kind | a | b | x | y | 语义 |
| --- | --- | --- | --- | --- | --- |
| `AdvanceDays` | - | - | 天数 | - | 推进日历，结算灵田生长与绿液凝聚 |
| `BottleGrant` | - | - | - | - | 交付掌天瓶（`owned = true`） |
| `BottleUnlockMature` | - | - | - | - | 解锁催熟之能（`matureKnown = true`） |
| `FieldUnlock` | 灵田 id | - | 槽位数 | - | 开一块灵田；已存在则只补足槽位，不清空已种的 |

四条都是纯状态写入，**不阻塞**，`dispatch` 里同步处理完直接回填 `ok = true`。

### 各条的实现要点

**`AdvanceDays`**：直接转调已有的 `Application::advanceDays(int)`，不要另写一份结算。
`x <= 0` 时什么都不做并回填 `ok = true`（脚本传 0 不该算错误）。上限夹在
一次 3650 天，防止脚本笔误写出一个让灵田循环跑几十万次的数。

**`BottleGrant`**：置 `owned = true`，并把 `lastChargeDay` 设为**当前日**。
这一条不能省：`lastChargeDay` 默认 0，若不重置，拾瓶当日就会按「已过 N 天」
一次性补发一堆绿液，原著「第八日方得一滴」当场作废。重复调用要幂等
（已 owned 时不重置计时，否则玩家已攒的零头被抹掉）。

**`BottleUnlockMature`**：置 `matureKnown = true`。幂等。**不**隐含 `owned`——
没瓶子却会催熟是无意义状态，但这里不替脚本兜底，写错了要能在测试里看出来。

**`FieldUnlock`**：`a` 为空或 `x <= 0` 时回填 `ok = false`。已存在同 id 的田时，
**只在槽位数不足时补足**，绝不重建 `slots`——重建会把玩家种了几年的药清空。

---

## 2. 新增 host 查询（同步返回，无挂起）

加在 `ScriptHost` 的 `__host` 表上，与现有 `flag_get` / `item_count` /
`realm_at_least` 并列。全部是纯读，不得有副作用。

| 函数 | 返回 | 说明 |
| --- | --- | --- |
| `__host.day()` | int | 当前绝对天数 |
| `__host.bottle_owned()` | bool | 是否已有瓶子 |
| `__host.bottle_mature_known()` | bool | 是否已会催熟 |
| `__host.bottle_drops()` | int | 当前绿液滴数 |
| `__host.field_planted(field_id)` | int | 该田已种（非空）槽位数；田不存在返回 0 |
| `__host.field_ripe(field_id)` | int | 该田已成熟槽位数；田不存在返回 0 |

`field_id` 传空字符串时，统计**所有**田的合计。第 2 章只有一块田，但第 7 章
百药园会有多块，接口先按多块写，免得到时改签名。

---

## 3. Lua 侧封装（`scripts/common/api.lua`）

沿用既有分界：**写走命令队列，读走 `__host`**。

```lua
-- 推进时间。会结算灵田生长与绿液凝聚，玩家跳过一年回来，田里的药真长了一年。
function advance_days(days)
    emit{ kind = "advance_days", x = days or 0 }
end

function today()
    return __host.day()
end

bottle = {
    -- 拾瓶。幂等：重复调用不会重置已攒的凝液零头。
    grant = function()
        emit{ kind = "bottle_grant" }
    end,

    -- 解锁催熟之能。原著里这一步比拾瓶晚四年，故意做成两个调用。
    unlock_mature = function()
        emit{ kind = "bottle_unlock_mature" }
    end,

    owned = function() return __host.bottle_owned() end,
    mature_known = function() return __host.bottle_mature_known() end,
    drops = function() return __host.bottle_drops() end,
}

field = {
    -- 开一块灵田。已存在时只补足槽位，不动已种的药。
    unlock = function(field_id, slots)
        assert(type(field_id) == "string" and field_id ~= "", "field.unlock() 需要灵田 id")
        emit{ kind = "field_unlock", a = field_id, x = slots or 4 }
    end,

    -- field_id 省略或传空串表示统计所有田。
    planted = function(field_id) return __host.field_planted(field_id or "") end,
    ripe = function(field_id) return __host.field_ripe(field_id or "") end,
}
```

**命令 kind 的字符串名**（`advance_days` / `bottle_grant` / `bottle_unlock_mature` /
`field_unlock`）由 `ScriptHost` 的解析表识别，拼错要在**加载期**就报错，
不能静默当成未知命令跳过。

> **更正（实现后回填）**：本文起草时担心现有解析表是「未知即忽略」，要求一并改成报错。
> 实现方核对后指出**现状本就是报错**（`toCommand()` 返回「未知的命令 kind = ...」并失败），
> 这个缺陷并不存在，改动无从谈起。最终做法是补一条负向测试把这个行为钉住：
> 塞一个不存在的 kind，断言事件启动失败、错误点名该 kind、且脚本确实没有继续往下跑。
> 记在这里，免得后人照契约去找一个不存在的问题。

---

## 4. 第 2 章约定的 id

编剧与关卡都按这套写，引擎方在测试里也用它们。

| 类别 | id | 说明 |
| --- | --- | --- |
| 灵田 | `shenshougu_yaopu` | 神手谷药圃，8 槽 |
| 地图 | `ch02_yaopu` / `ch02_jusuo` / `ch02_yabi` / `ch02_wairentang` | 见 ch02-design 第 5 节 |
| 商店 | `ch02_wairentang_yaoshang` | 外刃堂药商，收药 |

---

## 5. 测试要求

**每一条新命令与新查询都要有测试，且要有失败路径。** 具体最低要求：

1. `BottleGrant` 把 `lastChargeDay` 设成当日 —— 用一条测试证明**拾瓶当日不会立刻得到绿液**，
   且第 8 日得到第一滴。这条直接对应原著设定，是本章最容易写错的地方。
2. `BottleGrant` 重复调用不抹掉已攒零头。
3. `FieldUnlock` 对已存在的田**不清空已种槽位**。用「先种药 → 再 unlock 一次 → 药还在」证明。
4. `AdvanceDays` 传 0 与负数不崩、不倒退日历。
5. 未知命令 kind 在加载/执行期报错而非静默忽略（**负向测试**：塞一个不存在的 kind，
   断言它确实失败）。
6. `field_planted("")` 的合计口径：两块田各种一株，返回 2。

**第 1 章的教训写在这里再说一遍：通过的闸门不证明任何事，除非闸门被负向测试验证过。**
新加的每一处检查，都要有一条「故意写坏 → 确实被抓住」的测试。

---

## 6. 事后追加：把绿液用掉，以及 take() 的年份（2026-09-20）

> **这一节是起草时漏掉的，事后补。** 第 1 节那四条命令只管「把瓶子交出去」与
> 「解锁催熟」，**一条写接口也没给绿液本身**；同时 TakeItem 的 y 在第 1 节的表里
> 一直写着「灵草年份」，而实现从来没读过它。两处都不是实现方的偏移，是契约自己
> 的缺口，由第 2 章独立校对（`docs/ch02-review.md` BLOCKER-1 / BLOCKER-2）查出。
>
> 后果各自是：章末那场算账扣走的是背包里最便宜的那株种苗，四十四年那株原封不动
> 留着还能再卖一次；以及整章「倒出一滴」「浇了三滴」的动作在机制上一件也没发生，
> 旁白播着「瓶底空了」，灵田面板上写着「绿液 3 / 3」。

### 6.1 TakeItem 的 y（口径澄清，非新增）

`take(item_id, count, herb_age)` 的年份**必须**生效：

| y | 语义 | 走哪一条 |
| --- | --- | --- |
| `y > 0` | 扣「那一堆」——年份对不上就失败，不转头扣别的 | `GameState::removeItemOfAge` |
| `y <= 0` | 没指定年份，哪一堆都行；先扣年份低的 | `GameState::removeItem` |

**为什么 0 只能解释成「没指定」**：`api.lua` 把省略的年份补成 0，非灵草也一律
传 0，两者在命令里长得一模一样，无从分辨。而「确实只想扣 0 年那一株」并不会
因此写不出来——`removeItem` 本就先扣年份最低的，有 0 年那堆时结果与精确扣完全
一致。反过来把 0 当成精确年份，则会让 `zhitong.lua` 的
`take("herb_huangjing_cao", 1)`（交一株黄精抵人情，哪一株都算数）在玩家手上
只剩足年药时当场卡死。

`take()` 现在**返回是否真扣掉了**（此前返回 nil，无人使用）。东西不够时一件
不扣、返回 false，脚本可以据此走「你手上没有这个」的分支，而不是钱照给、货没动。

### 6.2 新增命令（追加在 `CommandKind` 末尾）

| kind | a | b | x | y | 语义 |
| --- | --- | --- | --- | --- | --- |
| `BottleSpend` | - | - | 滴数 | - | 倒出绿液本身，不催熟任何东西；不够则一滴不扣、`ok = false` |
| `BottleMature` | 物品 id | - | - | 当前年份 | 用一滴绿液催熟**背包里**那一堆中的一株 |

**`BottleSpend`**：`x <= 0` 视作 1（与 GiveItem / TakeItem 同一口径）。减法一律
走 `rules::spendDrops`，**全项目对 `drops` 做减法只有那一处**，`matureHerb` 自己
也走它。之所以要有这条「不催熟的消耗」：第 2 章试药那一碗是倒进碗里掺水喂兔子
的，与年份无关，而且那一刻 `matureKnown` 还是假的，走催熟只会被规则层挡下。

**`BottleMature`**：转调 `rules::matureHerb`，**年份跃升、上限与失败原因一个也
不在 game 层重算**。上限取这一味药自己的 `maxAge`（data 缺字段时按品阶推，见
`rules::herbMaxAgeForGrade`），不是全局的 `kMaxHerbAge`——传全局上限等于没有
上限，十一滴就能把一株一阶药推到一万年。成功后先 `removeItemOfAge` 扣掉原年份
那一株，再 `addItem` 放回新年份的一株：走 `removeItem` 的话，玩家浇的是这一株、
变年份的是另一株，而背包总数不变，谁也看不出来。

**为什么是「背包里的一株」而不是「灵田第几畦」**：脚本指不到槽位，要指得到就得
把畦位编号写进剧情脚本，而剧情里的药圃与面板里的八畦本就不是一套东西。本章
需要的只是「这一株药的年份被三滴推上去了」，背包足够表达。将来若要让脚本直接
催熟灵田，另加一条命令，不要把这条改成双重语义。

### 6.3 Lua 侧封装

```lua
bottle.spend(count)                      -- 返回 ok
bottle.mature(item_id, herb_age)         -- 返回 ok, 新年份, 失败原因码
```

失败原因码是 **ASCII 机器码，不是提示文案**：

| 码 | 含义 |
| --- | --- |
| `no_bottle` | 还没拿到瓶子 |
| `mature_unknown` | 有瓶子，但还不知道绿液能催熟 |
| `no_drops` | 瓶里没有绿液 |
| `not_ripe` | 这一株还不满一年，浇不进去（与采收共用 `rules::kRipeAge` 那条足年判据） |
| `at_max_age` | 这一味药已到它自己的年份上限，再浇也不添年份（**且不扣绿液**） |
| `no_item` | 背包里没有这个年份的这味药（`matureHerb` 不认识背包，这一条由 game 层给） |

不回填规则层那句中文 `reason`：那是给玩家看的提示，会跟着文案改，脚本拿它去
比对等于把 UI 文案钉进逻辑里，改一个字所有分支一起哑掉。四条催熟失败的理由是
叙事的一部分（「绿瓶四年」里玩家会依次撞上前两条），所以不合并成一个 bool。

回填通道：`CommandResult` 追加了 `int value`（目前只有 `BottleMature` 用，带回
新年份）与 `std::string code`。两者**恒定回填**，不是只在失败时塞进去——缺字段
与「字段是空的」在 Lua 里都是 nil，分不出来的差别迟早被当成偶发 bug。

### 6.4 测试要求（补在第 5 节之后）

7. `take(id, n, age)` 在背包同时有 1 年与 44 年两株时扣走 44 年那株，
   **外加负向**：手工跑一遍旧实现（`removeItem`）证明它扣的是另一株。
8. `take(id, n)` 不指定年份时仍先扣年份低的；包里一株 0 年的也没有时照样扣得动。
9. `bottle.spend` 不够时**一滴不扣**（负向：扣一半再报失败，绿液会凭空少掉）。
10. `bottle.mature` 成功时绿液恰好少一滴、年份按规则跃升、变的是被浇的那一堆。
11. **每一条失败路径各一条测试**，断言的是**具体是哪一个码**——全都回填成
    同一个码的实现也能骗过「非成功即可」的断言。规则层添一条理由，这里就加
    一条用例；`matureFailureCode()` 那个 switch 漏掉一个枚举值的后果是脚本
    收到一个**空**原因码，编译期不报、运行期不响。
12. 上限按种：同样 44 年、同样一滴，黄精（上限 44）报 `at_max_age` 且不扣绿液，
    紫参草（上限 100）推到 88。改回全局上限时这条会红。
13. 一条端到端：无头驱动真 `Application` 跑 `scripts/ch02/` 下真正会上线的脚本，
    断言绿液真的归零、年份真的落在这一味药自己的上限上、章末扣的是台词里说的
    那两株。**在此之前 `tests/` 对 `scripts/ch02/*.lua` 的引用数是 0**，而这两条
    BLOCKER 都是「各自都对、拼起来错了」的那一类，单测再多也挡不住。

---

## 7. 第 4 章二次整改：剧情境界上限（2026-09-23，技术债 G-14）

> 规则、存储与存档见 `docs/interfaces-p2.md` 第 7 节。本节只写脚本这一侧。

### 7.1 新增命令（追加在 `CommandKind` 末尾）

| kind | 字段 | 语义 |
| --- | --- | --- |
| `RealmCap`（`"realm_cap"`） | `x` = 上限境界编号 | 把 `GameState::realmCap` 抬到**至少** `x` |

回填：

| 情形 | `ok` | `code` |
| --- | --- | --- |
| `x` 不是合法境界编号（如炼气与筑基之间的 15） | `false` | `"no_realm"` |
| `x` ≤ 现有上限 | `true` | 空。**什么也不做**：结果（上限至少到这一层）已经成立 |
| `x` > 现有上限 | `true` | 空。上限抬到 `x` |

**只升不降。** 往下压上限等于替第 9 章的境界跌落做决定，这条命令决定不了那件事。
与 `RealmAdvance` 的 `not_higher` **故意不同**：境界往回拨一定是脚本写错了；
上限「已经更高」是正常的——读档重跑同一段、老档迁移上来的上限本就可能高于这一节。

### 7.2 `RealmAdvance` 顺带抬上限（行为增补，接口不变）

`realm.advance(t)` 成功（含「已经就在 t」那一种）时，同时把上限抬到**至少** `t`。
**升境本身不受上限约束**——它就是剧情在说话；说完之后上限若还停在原处，存档里就会是
「他已经是五层，上限却是三层」。所以第 4 章节点 1 / 2 / 4 那三句 `realm.advance`
**不必再写** `realm.cap`。回绝的情形（`no_realm` / `not_higher`）不动上限。

原命令的契约在 `docs/interfaces-p3-ch04.md` 第 6 节，那一节不在本批白名单里，没有改；以本节为准。

### 7.3 Lua 侧封装（`scripts/common/api.lua`，照 `realm.advance` 的写法）

```lua
realm.cap(value)    -- 返回 ok, code
```

用在「剧情说他到了这一层、但不替他把境界推上去」的节点上。**现行的三处**（各一行）：

| 脚本 | 位置 | 调用 | 口径出处 |
| --- | --- | --- | --- |
| `scripts/ch01/shenshougu_koujue.lua` | 二选一之后（两个分支都走到），`flag.set("ch01.koujue_received")` 之前 | `realm.cap(realm.QI_REFINING_1)` | `docs/ch01-design.md` 节点 9 授口诀 ＋ `docs/ch02-design.md` 第 2 节：段四才到第二层 |
| `scripts/ch02/ceng2.lua` | 紧跟 `flag.set("ch02.koujue_ceng", 2)` | `realm.cap(realm.QI_REFINING_2)` | `docs/ch02-design.md` 第 2 节段四 |
| `scripts/ch02/ceng3.lua` | 紧跟 `flag.set("ch02.koujue_ceng", 3)` | `realm.cap(realm.QI_REFINING_3)` | `docs/ch02-design.md` 第 1 节第 5 条、第 2 节段五 |

`ceng2` / `ceng3` 由剧情旗标 `ch02.duan4_start` / `duan5_start` 触发，与境界无关，
所以「上限由它们抬」不会死锁（协调者核过）。

### 7.4 测试（`tests/RealmCapTests.cpp`）

- 跑**真的** `api.lua` 与**真的**第 1、2 章脚本（夹具复制一份 `scripts/` 与 `data/`）：
  授口诀两条分支上限都到一层；`ceng2` 到二层、`ceng3` 到三层，并且抬起之后面板真的按得上去。
- `realm.cap`：低于现有上限不降、照样 ok；高于就抬；非法编号 `no_realm`；不动境界。
- `realm.advance`：从三层推到五层，上限跟到五层；上限在三层时推到八层照样成功（升境不受上限约束）。
