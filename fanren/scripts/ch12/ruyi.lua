-- @hook ch12_hanlitai trigger_taijie enter once
-- Facts and participants: docs/ch12-design.md 13.1 n29. 3.2：讨要 / 不要废金丝蚕（讨要支 give 3、ch12.jincan = 1，拍板 35）；
-- 战斗 ③（乌丑、玄骨是争夺者友军，拍板 17）；领狼首玉如意；置 ch12.ruyi；败 game_over。胜利文案写「收服」。
-- 抉择在一切状态改动之前，取消即 return。元婴大战不点名谁胜。
talk("", "ch12.ruyi.break")
talk("", "ch12.ruyi.altar")
talk("", "ch12.ruyi.fail")
talk("", "ch12.ruyi.left")
local pick = choice({"ch12.ruyi.opt_ask", "ch12.ruyi.opt_skip"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch12.ruyi.ask")
    talk("jiyin_zushi", "ch12.ruyi.ask_ok")
    give("story_jinsi_can", 3)
    flag.set("ch12.jincan", 1)
else
    talk("", "ch12.ruyi.skip")
end
talk("", "ch12.ruyi.fire")
talk("", "ch12.ruyi.war")
talk("", "ch12.ruyi.wolf")
local won = battle("b12_neidian_duoding")
if not won then game_over(); return end
talk("", "ch12.ruyi.win")
give("story_yu_ruyi", 1)
talk("", "ch12.ruyi.glare")
flag.set("ch12.ruyi")
