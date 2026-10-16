-- @hook ch13_wuzhong trigger_lidao enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 30。3.2 n30：choice「这就走 / 再留几日」（确认，不计入主动抉择，16.1 第 52 条），
-- 取消与「再留」都在顶格 return（置旗前；要再触发得先走下这条带）；advance_days(20)；
-- 顶格 flag.set("ch13.lidao")、teleport("ch13_waihai", 14, 46)（kScriptTransfers #5；落点与 genmaps_ch13.py 的 MISHI_SHORE 同一字面量）。
-- 确认文案写明此后回不了雾中岛（Z3 收口），不点名按键。「换形诀」「黄明礼」「南黎岛」「秘市」自本段起（12.2）。
talk("", "ch13.lidao.wall")
talk("", "ch13.lidao.ginseng")
talk("", "ch13.lidao.confirm")
local go = choice({"ch13.lidao.opt_go", "ch13.lidao.opt_stay"})
if go ~= 1 then return end
advance_days(20)
talk("", "ch13.lidao.face")
talk("", "ch13.lidao.rescue")
talk("huang_mingli", "ch13.lidao.nanli")
talk("huang_mingli", "ch13.lidao.mishi")
talk("", "ch13.lidao.land")
flag.set("ch13.lidao")
teleport("ch13_waihai", 14, 46)
