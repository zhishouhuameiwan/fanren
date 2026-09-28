-- @hook ch01_liangu_ya npc_qixuan_kaoguan npc
-- @hook ch01_liangu_ya trigger_kaohe_start interact once
-- 第一章节点 5「炼骨崖考核」。本章唯一的机制性玩法。
--
-- 不新增任何引擎功能：高度与体力只是这个文件里的两个局部变量，玩家永远看不到
-- 数字，看到的是每轮三条不同的结果叙述。规则见 docs/ch01-design.md 第 3 节。
--
-- 三档结算都通往同一个后续节点（墨大夫收徒）。原著里韩立没有登顶却被选中，
-- 这个反差是本章的情绪支点，所以玩法只决定墨大夫那句话怎么说，以及两个记录
-- 风格的旗标，绝不改变剧情走向。
--
-- 每一轮的 choice 都写成字面量而不是查表循环，是为了让 tools/validate.py 的
-- 文案 key 检查能扫到它们。表驱动会短一些，但那样写出去的 key 门禁看不见。

local STYLE_STEADY, STYLE_BOLD, STYLE_CAREFUL = 1, 2, 3

local height = 0
local stamina = 10
local counts = { 0, 0, 0 }

-- 把一次选择折算成高度与体力的增减，并记一笔风格。
-- 返回实际采用的风格编号，调用处据此播对应的那条结果文案。
local function step(pick)
    local style = STYLE_STEADY
    if pick == 2 then
        style = STYLE_BOLD
        height = height + 4
        stamina = stamina - 4
    elseif pick == 3 then
        style = STYLE_CAREFUL
        height = height + 1
        -- 借力歇息：耗 1、歇回 2，体力净涨 1，代价全在高度只涨 1。
        -- 净回 +1 是修正后的数值：原先净耗 0 时 height 恒等于 drain + 歇息次数，
        -- 登顶所需的 drain 必然超过初始体力，「从容登顶」在数学上是 0 条路径。
        stamina = stamina - 1 + 2
    else
        -- pick == 1 与取消（pick == nil）走同一条。迟疑按稳妥算：
        -- 没做出选择的玩家不该比做了选择的更吃亏。
        height = height + 2
        stamina = stamina - 2
    end
    counts[style] = counts[style] + 1
    return style
end

local function round1()
    talk("", "ch01.climb.r1_scene")
    local act = step(choice{
        "ch01.climb.r1_steady",
        "ch01.climb.r1_bold",
        "ch01.climb.r1_careful",
    })
    if act == STYLE_BOLD then
        talk("", "ch01.climb.r1_bold_r")
    elseif act == STYLE_CAREFUL then
        talk("", "ch01.climb.r1_careful_r")
    else
        talk("", "ch01.climb.r1_steady_r")
    end
end

local function round2()
    talk("", "ch01.climb.r2_scene")
    local act = step(choice{
        "ch01.climb.r2_steady",
        "ch01.climb.r2_bold",
        "ch01.climb.r2_careful",
    })
    if act == STYLE_BOLD then
        talk("", "ch01.climb.r2_bold_r")
    elseif act == STYLE_CAREFUL then
        talk("", "ch01.climb.r2_careful_r")
    else
        talk("", "ch01.climb.r2_steady_r")
    end
end

local function round3()
    talk("", "ch01.climb.r3_scene")
    local act = step(choice{
        "ch01.climb.r3_steady",
        "ch01.climb.r3_bold",
        "ch01.climb.r3_careful",
    })
    if act == STYLE_BOLD then
        talk("", "ch01.climb.r3_bold_r")
    elseif act == STYLE_CAREFUL then
        talk("", "ch01.climb.r3_careful_r")
    else
        talk("", "ch01.climb.r3_steady_r")
    end
end

local function round4()
    talk("", "ch01.climb.r4_scene")
    local act = step(choice{
        "ch01.climb.r4_steady",
        "ch01.climb.r4_bold",
        "ch01.climb.r4_careful",
    })
    if act == STYLE_BOLD then
        talk("", "ch01.climb.r4_bold_r")
    elseif act == STYLE_CAREFUL then
        talk("", "ch01.climb.r4_careful_r")
    else
        talk("", "ch01.climb.r4_steady_r")
    end
end

-- 攀爬途中的二选一。放在第二轮之后，是因为两轮最多耗掉 8 点体力，
-- 到这里体力必然还大于 0，这一次选择于是保证触发，不会被中止规则吞掉。
local function help_moment()
    talk("", "ch01.climb.help_scene")
    local pick = choice{
        "ch01.climb.opt_help",
        "ch01.climb.opt_shout",
    }
    if pick == 1 then
        -- 伸手要付代价：这一下高度一寸不涨，还掉一点体力。
        -- 没有代价的善举不算选择，只算奖励。
        stamina = stamina - 1
        flag.set("ch01.climb_helped")
        talk("", "ch01.climb.help_result")
        talk("zhang_tie", "ch01.climb.help_zt")
    else
        talk("", "ch01.climb.shout_result")
        talk("", "ch01.climb.shout_zt")
    end
end

-- 取四轮里选得最多的一类。并列时偏向更激进的一端（猛攀 > 稳扎 > 歇息），
-- 因为收尾时的姿态比平均值更像一个人的性格。
local function dominant_style()
    if counts[STYLE_BOLD] >= counts[STYLE_STEADY] and counts[STYLE_BOLD] >= counts[STYLE_CAREFUL] then
        return STYLE_BOLD
    end
    if counts[STYLE_CAREFUL] > counts[STYLE_STEADY] then
        return STYLE_CAREFUL
    end
    return STYLE_STEADY
end

-- 起攀那处触发是 once；考官本人（npc_qixuan_kaoguan）站到收徒为止（hidden_flag=ch01.shoutu_done），
-- 挂在他身上的这个脚本没有 once：考完再找他，从前会把四轮攀爬整个重考一遍、
-- 重设 ch01.climb_style 这几个旗标——而墨大夫点名时的那句评价正是照它们说的。
if flag.get("ch01.climb_done") == 1 then
    talk("qixuan_kaoguan", "ch01.climb.kg_again")
    return
end

talk("", "ch01.climb.arrive")
talk("qixuan_kaoguan", "ch01.climb.kg_rule")
talk("qixuan_kaoguan", "ch01.climb.kg_rule2")

-- 节点 3 在镇上打听过几个人，开场的心态就不一样。聊过两个以上算「心里有底」。
local heard = 0
if flag.get("ch01.zhenmin_jiuke") == 1 then heard = heard + 1 end
if flag.get("ch01.zhenmin_yaofan") == 1 then heard = heard + 1 end
if flag.get("ch01.zhenmin_jiaofu") == 1 then heard = heard + 1 end

if heard >= 2 then
    talk("", "ch01.climb.knew_rule")
else
    talk("", "ch01.climb.blank_rule")
end

-- 节点 4 分没分干粮，决定张铁临上崖时说的是哪一句。
if flag.get("ch01.zhangtie_shared") == 1 then
    talk("zhang_tie", "ch01.climb.zt_shared")
else
    talk("zhang_tie", "ch01.climb.zt_plain")
end

round1()
round2()
help_moment()

-- 体力归零强制中止：后面的轮次直接不给，按当前高度结算。
-- 这条规则是「咬牙猛攀」的代价所在，去掉它三个选项就等于一个。
if stamina > 0 then round3() end
if stamina > 0 then round4() end

if stamina <= 0 then
    flag.set("ch01.climb_exhausted")
    talk("", "ch01.climb.exhausted")
end

talk("", "ch01.climb.bell")

-- 登顶门槛 10（修正后）。高度始终是唯一的判据，体力只决定同一档里怎么措辞——
-- 这样即便日后再调数值，中低两档也不会跟着塌掉。
if height >= 10 then
    flag.set("ch01.climb_reached_top", 1)
    if stamina <= 0 then
        talk("", "ch01.climb.res_top_exhausted")
    else
        talk("", "ch01.climb.res_top")
    end
elseif height >= 6 then
    flag.set("ch01.climb_reached_top", 0)
    talk("", "ch01.climb.res_mid")
else
    flag.set("ch01.climb_reached_top", 0)
    talk("", "ch01.climb.res_low")
end

flag.set("ch01.climb_style", dominant_style())
talk("", "ch01.climb.down")

flag.set("ch01.climb_done")
