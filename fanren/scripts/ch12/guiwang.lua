-- @hook ch12_guiwu trigger_guiwang enter once
-- Facts and participants: docs/ch12-design.md 13.1 n13. 3.2：「先自守」一句后回选单；战前 give 神雷 2；战斗 ①；战后 take 余数；败 game_over。
-- 神雷余数：余数为 0 时不调 take（take(id, 0) 会被引擎当成扣 1，契约第 4 节）。
-- 紫灵、葛笠在 tupo 留坡上等他探路（ch12-review.md C-3），开头一句旁白二人赶上来。
talk("", "ch12.guiwang.catchup")
talk("ge_li", "ch12.guiwang.geli")
talk("", "ch12.guiwang.go")
talk("", "ch12.guiwang.lost")
talk("", "ch12.guiwang.wake")
talk("zi_ling_xutian", "ch12.guiwang.ziling")
talk("", "ch12.guiwang.black")
while true do
    local pick = choice({"ch12.guiwang.opt_wait", "ch12.guiwang.opt_go"})
    if not pick then return end
    if pick == 2 then break end
    talk("", "ch12.guiwang.wait")
end
talk("", "ch12.guiwang.go2")
give("talisman_bixie_shenlei", 2)
talk("", "ch12.guiwang.thunder")
local won = battle("b12_guiwu_guiwang")
if not won then game_over(); return end
talk("", "ch12.guiwang.win")
talk("", "ch12.guiwang.soul")
local n = item.count("talisman_bixie_shenlei")
if n > 0 then
    if take("talisman_bixie_shenlei", n) then
        talk("", "ch12.guiwang.spent")
    else
        talk("", "ch12.guiwang.keep")
    end
else
    talk("", "ch12.guiwang.spent")
end
flag.set("ch12.guiwang")
