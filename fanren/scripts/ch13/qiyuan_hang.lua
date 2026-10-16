-- @hook ch13_wuzhong trigger_qiyuan_hang enter
-- 往返 #6（docs/ch13-design.md 3.1 末表）：trigger enter、once 否、guard ch13.lifu、set_flag 空；本脚本 0 处 flag.set。
-- 开头 ch13.biguan 已置 → 一句「闭关在即」return；choice 去 / 不去，取消与「不去」都在顶格 return；
-- 顶格 advance_days(12)、teleport("ch13_waihai", 7, 12)——落点写死坐标，与 tools/mapgen/genmaps_ch13.py 的 QIYUAN_REEF 同一字面量
-- （黑石城西门外礁，在 trigger_hui_wuzhong (3,12) 与西门带 trigger_chucheng (10,11..13) 之间）。
if flag.get("ch13.biguan") ~= 0 then talk("", "ch13.qiyuan_hang.closed"); return end
talk("", "ch13.qiyuan_hang.ask")
local go = choice({"ch13.qiyuan_hang.opt_go", "ch13.qiyuan_hang.opt_stay"})
if go ~= 1 then return end
talk("", "ch13.qiyuan_hang.go")
advance_days(12)
teleport("ch13_waihai", 7, 12)
