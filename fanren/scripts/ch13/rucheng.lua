-- @hook ch13_tianxing trigger_rucheng enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 7。3.2 n07：choice「报名 / 安分」，两支都 give story_huangse_zhihuan 1、
-- 只换一句；取消在给物前 return；置 ch13.rucheng。① 不抵传送阵条件（先前所杀不算，A32）。
-- 赵、于二人的对象自 ch13.yinjian 起就站在城门（5.1），本段不写谁离开。
talk("", "ch13.rucheng.gate")
talk("zhao_hufa", "ch13.rucheng.check")
talk("", "ch13.rucheng.portrait")
talk("", "ch13.rucheng.yu")
talk("yu_hufa", "ch13.rucheng.face")
talk("hanli", "ch13.rucheng.dingyan")
talk("yu_hufa", "ch13.rucheng.two_ways")
talk("yu_hufa", "ch13.rucheng.array")
local pick = choice({"ch13.rucheng.opt_join", "ch13.rucheng.opt_stay"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch13.rucheng.join")
else
    talk("hanli", "ch13.rucheng.stay")
end
give("story_huangse_zhihuan", 1)
talk("", "ch13.rucheng.ring")
flag.set("ch13.rucheng")
