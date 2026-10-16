-- @hook ch13_nanming npc_ling_yuling npc
-- Facts and participants: docs/ch13-design.md 13.1 段 6。3.2 n06：choice「以妙音门之恩婉拒 / 不答」，两支都不入星宫；
-- 取消在置旗前 return；advance_days(2)；顶格 flag.set("ch13.yinjian")、teleport("ch13_tianxing")（kScriptTransfers #2）。
-- 男女辨不出至多一句；黄脸修士献丹那一场不写（她在城外码头先走，赵护法在城门，13.1 段 7）。
if flag.get("ch13.yinjian") ~= 0 then talk("", "ch13.yinjian.gone"); return end
if flag.get("ch13.zhuibing") == 0 then talk("", "ch13.yinjian.early"); return end
talk("ling_yuling", "ch13.yinjian.thanks")
talk("", "ch13.yinjian.voice")
talk("hanli", "ch13.yinjian.name")
talk("ling_yuling", "ch13.yinjian.invite")
local pick = choice({"ch13.yinjian.opt_decline", "ch13.yinjian.opt_silent"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch13.yinjian.decline")
else
    talk("", "ch13.yinjian.silent")
end
talk("ling_yuling", "ch13.yinjian.escort")
advance_days(2)
talk("", "ch13.yinjian.dock")
flag.set("ch13.yinjian")
teleport("ch13_tianxing")
