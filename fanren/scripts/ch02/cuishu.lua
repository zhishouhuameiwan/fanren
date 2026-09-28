-- @hook ch02_yaopu trigger_cuishu interact
-- 第二章节点 10 下半「发现催熟」与节点 11 上半「攒满一瓶浇同一株」。
-- 挂在 ch02_yaopu 东头那畦的 trigger（mode=interact, once=false,
-- guard_flag=ch02.shiyao_done）。
--
-- 一个 trigger 承两场戏，靠 once=false 与旗标分派：
--   第一次交互 → 发现催熟（解锁），第二次交互 → 攒满三滴浇同一株（算账的准备）。
-- 中间那二十来天要玩家自己走开去过日子，所以不能写在一段脚本里一气呵成——
-- 一段脚本没办法在中途把控制权交还给玩家再收回来，这是第 1 章踩出来的限制。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 1 条）：
-- 拾瓶（段二）与发现催熟（此处）相隔四年，是两个独立解锁节点，不可合并。
-- 引擎侧也是两条独立调用：bottle.grant() 在 shiping.lua，
-- bottle.unlock_mature() 在这里。
--
-- 解锁事件必须足够显眼（设计文档第 3 节）：面板实现方指出，玩家有瓶子、有
-- 绿液，点催熟却被拒，极易当成 bug。所以这一场是药香先撞过来、药草当场变样、
-- 他数了两遍纹路、愣住——解锁的那一刻玩家不可能错过。
--
-- 两场戏的分量不同，不要写反：
--   第一场是「原来这东西有用」。一滴把年份从 1 推到 11，照 herbPrice 算是
--   六块变二十二块，三点几倍。不许写成一夜暴富，也不许把单滴吹成几十倍。
--   第二场才是教学。三滴摞在一株上，1 → 11 → 22 → 44，卖价二十四倍。
--
-- 数字照 src/core/rules/Bottle.cpp 实算：matureHerb 是「当前年份 +
-- max(当前年份, 10)」，herbMaxAgeForGrade(1) == 44，所以三滴正好到顶，
-- 第四滴再浇也不添年份。「再往下滴，纹路不添了」那一句必须留着——
-- 它把一阶药的上限当场演给玩家看，断掉「一直浇同一株就能无限涨」的念想。
-- 更高的年份要靠后面的章节拿到更好的种子。

if flag.get("ch02.shiyao_done") ~= 1 then
    talk("", "ch02.cuishu.gate")
    return
end

-- ---------------------------------------------------------------------------
-- 第二场：攒满三滴，浇在同一株上。
-- ---------------------------------------------------------------------------
if flag.get("ch02.cuishou_unlocked") == 1 then
    if flag.get("ch02.cuanman_done") == 1 then
        talk("", "ch02.cuishu.again")
        talk("", "ch02.cuanman.again")
        return
    end

    -- 炼气期瓶子容量 3 滴，凝一滴七天，攒满要二十来天。
    -- 玩家若已经自己攒够了，这里一天也不推。
    if bottle.drops() < 3 then
        talk("", "ch02.cuanman.wait")
        -- 满瓶之后 refill 会把计时拨到当天，多推的天数不会白攒也不会倒欠，
        -- 见 core/rules/Bottle.cpp 里 refill 对满瓶的处理。
        advance_days(21)
    end

    -- 三滴不齐就不动手。中途断在第二滴上会在背包里留下一株二十二年的半成品，
    -- 玩家再进来一次又会另起一株，桌上就多出一株没人认得的药。
    -- 七天一滴、容量三滴，二十一天必然攒满，所以这一条在本章不该成立；
    -- 它挡的是凝液算错时的连锁后果，不是玩家的正常路径。
    if bottle.drops() < 3 then
        return
    end

    talk("", "ch02.cuanman.plan")

    -- 他挑中的那一株：东头刚满一年的黄精，就是段一那批种苗里的一株。
    -- 脚本指不到具体的畦位（契约第 6 节说明了为什么不做），所以「地里的
    -- 那一株」在数据上就是背包里这一株；玩家看得见的差别只有年份和绿液。
    give("herb_huangjing_cao", 1, 1)

    talk("", "ch02.cuanman.count")

    -- 三滴摞在同一株上：1 → 11 → 22 → 44。
    --
    -- 年份不是这里写死的，是 rules::matureHerb 一滴一滴算出来的，每一滴都
    -- 真的从瓶里扣掉。从前这一段写作 give(..., 44)：台词说浇了三滴，灵田
    -- 面板上的「绿液 3 / 3」一动不动，本章的核心教学在机制层是假的。
    -- 上限也由规则层按这一味药自己的 maxAge（黄精 44 年）卡，不是全局上限。
    local age = 1
    for drop = 1, 3 do
        if drop == 3 then
            talk("", "ch02.cuanman.third")
        end
        local ok, new_age, why = bottle.mature("herb_huangjing_cao", age)
        if not ok then
            -- 各种失败各有各的下文，这里按本章能发生的那一种分：
            --   no_drops       —— 绿液不够，让他再数几天日子；旗标不置，
            --                     玩家过几日回来还能接着浇。
            --   at_max_age     —— 这一株到顶了，不硬把它推成四十四年。
            --   not_ripe       —— 未足年浇不进去。上面给的就是足年那一株，
            --                     走不到；真走到了说明足年那条线改了。
            --   no_bottle / mature_unknown —— 本章走不到（瓶子四年前就在手上，
            --                     催熟上一场刚解锁），也没有对应文案。
            -- 不管哪一种，都不再往下演：宁可这一场没演完，也不能让台词说了
            -- 一件机制上没发生的事——这一整条修的就是那个毛病。
            if why == "no_drops" then
                talk("", "ch02.cuanman.wait")
            end
            return
        end
        age = new_age
    end

    talk("", "ch02.cuanman.stop")
    talk("", "ch02.cuanman.pick")
    talk("", "ch02.cuanman.go")

    flag.set("ch02.cuanman_done")
    return
end

-- ---------------------------------------------------------------------------
-- 第一场：次日清晨，发现催熟。本章高潮。
-- ---------------------------------------------------------------------------
talk("", "ch02.cuishu.morning")
talk("", "ch02.cuishu.smell")
talk("", "ch02.cuishu.see")
talk("", "ch02.cuishu.count")
talk("", "ch02.cuishu.stun")
talk("", "ch02.cuishu.recount")
talk("", "ch02.cuishu.link")
talk("", "ch02.cuishu.realize")
talk("", "ch02.cuishu.sit")

-- 会催熟了。这一条不隐含 owned——瓶子四年前就在他手里了。
bottle.unlock_mature()

talk("", "ch02.cuishu.pick")

-- 被泼到的那几株里最壮的一株，十一年份。段末三株对照里的中间那一株。
give("herb_huangjing_cao", 1, 11)

talk("", "ch02.cuishu.bottle")
talk("", "ch02.cuishu.wait")
talk("", "ch02.cuishu.drop")
talk("", "ch02.cuishu.think")

flag.set("ch02.cuishou_unlocked")
