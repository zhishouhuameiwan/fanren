-- @hook ch02_yaopu trigger_caiyao interact
-- 第二章节点 3 上半「采第一株药」。
-- 挂在 ch02_yaopu 晾药棚旁的 trigger（mode=interact）。
--
-- 这里是段一的推进闸门：不是「点一下继续」，而是这一段该做的事做完了没有。
-- 两条条件，都挂在玩家自己的操作上：
--   1. 灵田里至少种下过一株（field.planted）；
--   2. 从入谷第一课那天算起，至少过了三十日（today() 减 ch02.diyike_day）。
-- 第二条只能靠玩家去打坐才推得动——修炼面板每坐一回都会推日历，
-- 于是「熬过了一段日子」是真熬的，不是旁白说的。
--
-- 采下来的这一株是一年份的。它在本章的用处只有一个：当段五那株四十四年份的
-- 对照组。所以这里不许写得像什么成就，越平淡越好。

if flag.get("ch02.dazuo_done") ~= 1 then
    talk("", "ch02.caiyao.gate_sit")
    return
end

if flag.get("ch02.caiyao_done") == 1 then
    talk("", "ch02.caiyao.again")
    return
end

if field.planted("shenshougu_yaopu") < 1 then
    talk("", "ch02.caiyao.gate_plant")
    return
end

-- 三十日是按入谷第一课那天起算的。玩家一直不打坐，日历就不走，这一关就过不去。
if today() - flag.get("ch02.diyike_day") < 30 then
    talk("", "ch02.caiyao.gate_sit")
    return
end

talk("", "ch02.caiyao.arrive")
talk("", "ch02.caiyao.hold")
talk("", "ch02.caiyao.look")

-- 东头那两畦是管事说的「去年秋里下的」，与玩家自己种的那几株无关：
-- 玩家刚下的种要满一年才足年，段一只有半年，拿自己种的当第一株是对不上的。
give("herb_huangjing_cao", 1, 1)

talk("yaopu_guanshi", "ch02.caiyao.hint")

flag.set("ch02.caiyao_done")
