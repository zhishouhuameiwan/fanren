-- 事件脚本可用的全部 API。init 时由 ScriptHost 加载一次，之后所有事件脚本
-- 直接用这里定义的全局函数。
--
-- 为什么这些 API 是纯 Lua 写的，而不是 C++ 注册的：
-- lua_yield 只能在 Lua 帧之间挂起。若 talk() 是一个 C 函数，挂起时调用栈上
-- 夹着一层 C 帧，Lua 只会报 "attempt to yield across a C-call boundary"。
-- 把「造命令表 + yield」整段留在 Lua 侧，挂起点就永远只在 Lua 帧之间；
-- C++ 那边只剩 lua_resume 和读表两件事，碰不到挂起。
--
-- C++ 仍然注册了 __host.* 三个只读查询，但它们同步返回、不含任何挂起，
-- 因此不会把 C 帧留在挂起点上。

local yield = coroutine.yield

-- 所有事件 API 的唯一出口：造表 → 挂起 → 拿回 game 层回填的结果。
-- 结果表形如 { ok = boolean, index = 整数（0 起算，取消为 -1）, won = boolean }。
-- game 层若什么都没回填（如无头 bot 直接跳过），补一张空表，免得下游到处判 nil。
local function emit(command)
    return yield(command) or {}
end

-- ---------------------------------------------------------------------------
-- 对话与分支
-- ---------------------------------------------------------------------------

-- text_key 走 data/text/*.json，脚本里不出现任何中文字面量。
function talk(speaker_id, text_key)
    emit{ kind = "talk", a = text_key or "", b = speaker_id or "" }
end

-- option_keys 是文案 key 数组。返回选中序号（1 起算），取消返回 nil。
function choice(option_keys)
    assert(type(option_keys) == "table", "choice() 需要一个选项 key 数组")
    assert(#option_keys > 0, "choice() 至少要有一个选项")
    local result = emit{ kind = "choice", options = option_keys }
    local index = result.index or -1
    -- C++ 侧 0 起算、取消为 -1；这里统一成 Lua 习惯的 1 起算、取消为 nil，
    -- 让脚本可以直接写 if pick == 1 而不必到处 +1。
    -- 越界当取消。ScriptHost 已经挡过一道并写了日志，这里是第二道：
    -- 脚本宁可走 else，也不该拿着一个不存在的选项号往下算。
    if index < 0 or index >= #option_keys then return nil end
    return index + 1
end

-- 打一场。返回三个值：
--   won     是否打赢（布尔）
--   how     结局的机器码：""（赢）/ "lost" / "escaped"（我方逃了）/
--           "enemy_fled"（敌人逃了，识海第二场就是这个）
--   spoils  识海之战咬下的体积百分比（0-100）；其余战斗恒为 0
--
-- 只看第一个返回值的老脚本照旧能用。**识海第二场必须看第二个**：
-- 那一场设计上打不死，won 永远是 false，只看 won 会把「敌人带着伤跑了」
-- 读成「韩立输了」，而这两件事后面的剧情完全不同。
function battle(battle_id)
    local result = emit{ kind = "battle", a = battle_id or "" }
    return result.won == true, result.code or "", result.value or 0
end

-- ---------------------------------------------------------------------------
-- 场景与演出
-- ---------------------------------------------------------------------------

function teleport(map_id, x, y)
    emit{ kind = "teleport", a = map_id or "", x = x or 0, y = y or 0 }
end

local function fade_out(ms)
    emit{ kind = "fade_out", x = ms or 300 }
end

local function fade_in(ms)
    emit{ kind = "fade_in", x = ms or 300 }
end

-- in 是 Lua 关键字，fade.in() 写不出来。契约里的名字保留为 fade["in"]，
-- 另给一个能直接点出来的别名 fade.in_，脚本里优先用后者。
fade = { out = fade_out, ["in"] = fade_in, in_ = fade_in }

function wait(ms)
    emit{ kind = "wait", x = ms or 0 }
end

function sfx(id)
    emit{ kind = "play_sfx", a = id or "" }
end

-- 点播一首 BGM（assets/bgm/<id>.ogg），换图也不撤；bgm("map") 撤掉点播、放回这张图自己的曲子。
-- 点播了就要在这一段演完时 bgm("map")：点播不进存档，读档回来放的是地图曲。
-- 不许省参数：bgm() 看不出是想静音还是想恢复，宁可当场报错。
function bgm(id)
    assert(type(id) == "string" and id ~= "", "bgm() 需要曲子 id，恢复地图曲写 bgm(\"map\")")
    emit{ kind = "play_bgm", a = id }
end

function shop(shop_id)
    emit{ kind = "shop", a = shop_id or "" }
end

function game_over()
    emit{ kind = "game_over" }
end

function ending(title_key, text_key)
    emit{ kind = "ending", a = title_key or "", b = text_key or "" }
end

-- ---------------------------------------------------------------------------
-- 状态读写
--
-- 读走 __host.*（同步返回），写走命令队列（挂起，由 game 层落到 GameState）。
-- 这条分界不是洁癖：写操作要参与存档、要出动画、要能被回放，必须经过 game 层
-- 这一道；读操作没有副作用，直通最省事。
-- ---------------------------------------------------------------------------

flag = {
    set = function(name, value)
        assert(type(name) == "string" and name ~= "", "flag.set() 需要旗标名")
        emit{ kind = "set_flag", a = name, x = value or 1 }
    end,
    get = function(name)
        return __host.flag_get(name or "")
    end,
}

item = {
    count = function(id)
        return __host.item_count(id or "")
    end,
}

function give(item_id, count, herb_age)
    emit{ kind = "give_item", a = item_id or "", x = count or 1, y = herb_age or 0 }
end

-- 扣物品。herb_age 省略（或传 0）表示「哪一堆都行」，此时先扣年份低的；
-- 给了年份就精确扣那一堆，年份对不上按失败论，不会转头扣掉另一株。
--
-- 返回是否真扣掉了。东西不够时一件不扣、返回 false，脚本可以据此走
-- 「你手上没有这个」的分支，而不是钱照给、货没动。
function take(item_id, count, herb_age)
    local result = emit{ kind = "take_item", a = item_id or "", x = count or 1, y = herb_age or 0 }
    return result.ok == true
end

-- 境界编号与 src/core/rules/Realm.h 的 enum 一一对应。写成常量而不是让脚本
-- 里出现裸数字 21，是为了改 enum 时能一处改完、脚本读起来也是「筑基初期」。
realm = {
    at_least = function(value)
        return __host.realm_at_least(value or 0)
    end,

    -- 提升境界。只许升不许降：目标低于当前会**返回 false**（原因码 "not_higher"），
    -- 目标等于当前什么都不做但返回 true（结果已经成立，与 party.add 同一口径）。
    -- 升上去之后气血 / 法力上限会跟着长，当前值按差额上抬（**突破不是疗伤**：
    -- 带伤突破的人会带着那道伤出来，与打坐面板 applyRealmAttributes 是同一个结果）。
    --
    -- 原因码：
    --   "no_realm"    不是一个合法的境界编号
    --   "not_higher"  目标低于当前（跌落是 Realm::demote 的事，第 9 章才用得上）
    --
    -- **凡调请看返回值**，与 take() / party.add() 同一条规矩。真返回 false 说明
    -- 脚本里那个编号写错了，而静默收下的后果是这一章的数值全部对不上账——
    -- 第 4 章正因为「境界这一位指不出来路」被复审打回过一次。
    advance = function(value)
        assert(type(value) == "number", "realm.advance() 需要境界编号")
        local result = emit{ kind = "realm_advance", x = value }
        return result.ok == true, result.code or ""
    end,

    -- 抬剧情给的境界上限：玩家自己在打坐面板上按的突破越不过它（技术债 G-14，
    -- 契约 docs/interfaces-p3-script.md 第 7 节）。语义是「上限**至少**到这一层」，
    -- 只升不降；已经不低于就什么也不做、照样返回 true（读档重跑同一段不算错）。
    -- realm.advance 会顺带把上限抬到目标层，所以升境的节点不必再写这一句——
    -- 这一句是给「剧情说他到了这一层、但不替他把境界推上去」的节点用的。
    --
    -- 原因码：
    --   "no_realm"    不是一个合法的境界编号
    cap = function(value)
        assert(type(value) == "number", "realm.cap() 需要境界编号")
        local result = emit{ kind = "realm_cap", x = value }
        return result.ok == true, result.code or ""
    end,

    -- 当前境界编号。炼气期的编号就是层数（1-13）；筑基以上是 21/22/23、31/32/33，
    -- 要分档请用 at_least，不要拿它当「第几层」去算。
    level = function() return __host.realm_value() end,

    MORTAL = 0,
    QI_REFINING_1 = 1,
    QI_REFINING_2 = 2,
    QI_REFINING_3 = 3,
    QI_REFINING_4 = 4,
    QI_REFINING_5 = 5,
    QI_REFINING_6 = 6,
    QI_REFINING_7 = 7,
    QI_REFINING_8 = 8,
    QI_REFINING_9 = 9,
    QI_REFINING_10 = 10,
    QI_REFINING_11 = 11,
    QI_REFINING_12 = 12,
    QI_REFINING_13 = 13,
    FOUNDATION_EARLY = 21,
    FOUNDATION_MID = 22,
    FOUNDATION_LATE = 23,
    CORE_EARLY = 31,
    CORE_MID = 32,
    CORE_LATE = 33,
}

-- ---------------------------------------------------------------------------
-- 时间、掌天瓶、灵田（P3 第 2 章增补）
--
-- 沿用上面那条分界：写走命令队列，读走 __host。
-- 这三样是第 2 章核心循环闭合的前提——没有它们，整章只能退化成一串纯对话。
-- ---------------------------------------------------------------------------

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

    -- 倒出绿液，不催熟任何东西。第 2 章试药那一碗就是这么没的：
    -- 倒进碗里掺水喂兔子，与年份无关。
    --
    -- 返回是否真扣掉了。不够时一滴不扣、返回 false —— 「说倒了却没倒」正是
    -- 这一批接口要修的那个毛病，不能在自己身上再犯一次。
    spend = function(count)
        local result = emit{ kind = "bottle_spend", x = count or 1 }
        return result.ok == true
    end,

    -- 用一滴绿液催熟**背包里**的一株灵草：扣一滴、年份跃升、写回背包。
    --
    -- 走的是灵田面板同一条规则（rules::matureHerb），所以年份与上限都不在
    -- 脚本这边算：一株几年浇一滴变几年，问引擎要，别写死。上限按这一味药
    -- 自己的 maxAge 取（黄精 44 年），不是全局上限。
    --
    -- 返回 ok, 新年份, 失败原因码。原因码是 ASCII 机器码不是提示文案，取值：
    --   "no_bottle"       还没拿到瓶子
    --   "mature_unknown"  有瓶子，但还不知道绿液能催熟
    --   "no_drops"        瓶里没有绿液
    --   "not_ripe"        这一株还不满一年，浇不进去（与采收共用同一条足年判据）
    --   "at_max_age"      这一味药已到它自己的年份上限，再浇也不添年份
    --   "no_item"         背包里没有这个年份的这味药
    -- 催熟失败的理由是叙事的一部分（「绿瓶四年」里玩家会依次撞上前两条），
    -- 所以不合并成一个 bool。规则层将来再添一条理由时，这张表要跟着长。
    mature = function(item_id, herb_age)
        assert(type(item_id) == "string" and item_id ~= "", "bottle.mature() 需要物品 id")
        local result = emit{ kind = "bottle_mature", a = item_id, y = herb_age or 0 }
        return result.ok == true, result.value or 0, result.code or ""
    end,

    owned = function() return __host.bottle_owned() end,
    mature_known = function() return __host.bottle_mature_known() end,
    drops = function() return __host.bottle_drops() end,
}

-- ---------------------------------------------------------------------------
-- 队伍（P3 第 3 章增补，契约 docs/interfaces-p3-ch03.md 第 1.3 节）
--
-- 沿用同一条分界：写走命令队列，读走 __host。
-- **韩立不在队伍里**：他的状态散在 GameState 的既有字段里，把他也塞进来
-- 会立刻出现两处都记着他血量的双重真源。
-- ---------------------------------------------------------------------------

party = {
    -- 入队。已在队里则什么都不做（幂等）。
    --
    -- 返回是否成功。传一个 data/roles/ 里没有的角色 id 会**返回 false**，
    -- 不静默收下：拼错的 id 会在战斗里变成一个没有属性的空单位，而到那时
    -- 已经很难追回是谁写错的。凡调 party.add 请看返回值。
    add = function(role_id)
        assert(type(role_id) == "string" and role_id ~= "", "party.add() 需要角色 id")
        local result = emit{ kind = "party_add", a = role_id }
        return result.ok == true
    end,

    -- 离队。返回是否真有人离队（本来就不在队里时返回 false）。
    remove = function(role_id)
        assert(type(role_id) == "string" and role_id ~= "", "party.remove() 需要角色 id")
        local result = emit{ kind = "party_remove", a = role_id }
        return result.ok == true
    end,

    has = function(role_id) return __host.party_has(role_id or "") end,
    size = function() return __host.party_size() end,
}

-- ---------------------------------------------------------------------------
-- 已习得法术（P3 第 4 章增补，契约 docs/interfaces-p3-ch04.md 第 1.3 节）
--
-- 同一条分界：写走命令队列，读走 __host。
-- 这一组是技术债 G-4 的落点——在它之前，「韩立学会了火弹术」这件事无处存放：
-- magics 只在 RoleTemplate（data）上，而 data 是运行期只读的。
-- ---------------------------------------------------------------------------

magic = {
    -- 学会一门。已会则什么都不做（幂等），**仍然返回 true**：
    -- 「让他学会」这件事的结果已经成立，与 party.add 同一口径。
    --
    -- 传一个 data/magics/ 里没有的 id 会**返回 false**（原因码 "no_magic"），
    -- 不静默收下：拼错的法术 id 会让玩家在战斗里对着一个空菜单，
    -- 而到那时已经很难追回是谁写错的。**凡调 magic.learn 请看返回值。**
    --
    -- 返回 ok, code。code 为 ASCII 机器码，成功时是空串，取值：
    --   "no_magic"  data/magics/ 里没有这个 id（含传了空串）
    learn = function(magic_id)
        assert(type(magic_id) == "string" and magic_id ~= "", "magic.learn() 需要法术 id")
        local result = emit{ kind = "magic_learn", a = magic_id }
        return result.ok == true, result.code or ""
    end,

    -- 忘掉一门（第 9 章境界跌落可能用得上；本章不用，一并给）。
    --
    -- 返回是否**真的忘掉了一门**。本来就不会时返回 false（原因码
    -- "not_learned"），让脚本分得清「忘了」与「他本来就不会」——与
    -- party.remove、take() 同一口径。
    forget = function(magic_id)
        assert(type(magic_id) == "string" and magic_id ~= "", "magic.forget() 需要法术 id")
        local result = emit{ kind = "magic_forget", a = magic_id }
        return result.ok == true, result.code or ""
    end,

    -- 只读。knows 回答「他会不会这一门」，count 回答「他一共会几门」。
    -- 都不问 data：一个 data 里不存在的 id 根本进不了这份清单（learn 那道闸）。
    knows = function(magic_id) return __host.magic_knows(magic_id or "") end,
    count = function() return __host.magic_count() end,
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

-- ---------------------------------------------------------------------------
-- 按年份下限计数与扣物（第 7 章契约 docs/interfaces-p3-ch07.md 第 4 节）
--
-- 扣物三种写法各有用处，别混用：
--   take(id, n)          「哪一堆都行」：先扣年份低的（交一株黄精抵人情，哪一株都算数）；
--   take(id, n, age)     「恰好这一堆」：只扣年份正好是 age 的那一堆（商店卖的就是玩家点的那一株）；
--   take_aged(id, n, a)  「够格的里挑」：年份不低于 a 的堆里扣，够格里年份低的先扣
--                        （马师伯收四十四年以上的黄精：嫩苗不够格，更老的留给玩家）。
-- ---------------------------------------------------------------------------

-- 年份不低于 min_age 的有几件（第 7 章契约 4.1）。
item.count_aged = function(id, min_age)
    return __host.item_count_aged(id or "", min_age or 0)
end

-- 从年份不低于 min_age 的堆里扣 count 件，够格里年份低的先扣；不够就一件不扣。
function take_aged(item_id, count, min_age)
    local result = emit{ kind = "take_item_aged", a = item_id or "", x = count or 1, y = min_age or 0 }
    return result.ok == true
end
