-- @hook ch01_liangu_ya trigger_fangbang enter once
-- 第一章节点 6「放榜」。本章的情绪低点。
--
-- 攀爬结果在这里只改叙述，不改走向：登顶的挂在名单最末，没登顶的不在名单上。
-- 两条都接着走节点 7（墨大夫点名），因为墨大夫挑的不是名次。

talk("", "ch01.fangbang.evening")
talk("qixuan_kaoguan", "ch01.fangbang.kg_read")
talk("", "ch01.fangbang.zt_in")

if flag.get("ch01.climb_reached_top") == 1 then
    talk("", "ch01.fangbang.top_listed")
    talk("han_sanshu", "ch01.fangbang.sanshu_relief")
else
    talk("", "ch01.fangbang.no_name")
    talk("", "ch01.fangbang.hanli_count")
    talk("han_sanshu", "ch01.fangbang.sanshu_face")
    talk("", "ch01.fangbang.hanli_stand")
    -- 张铁隔着人群找他，只在他没被念到时才有意义：两个人这会儿站在两边。
    talk("zhang_tie", "ch01.fangbang.zt_look")
end

talk("", "ch01.fangbang.crowd_leave")

flag.set("ch01.fangbang_done")
