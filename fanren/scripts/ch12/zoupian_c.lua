-- @hook ch12_guiwu trigger_zoupian_c enter
-- 走偏（改编，docs/ch12-design.md 3.1、3.2、第 7 节，16.1 第 39 条）：第三处错路尽头，鬼雾把人推回第三岔口。
-- kScriptTransfers 五元行 {"ch12.tupo", "scripts/ch12/zoupian_c.lua", "ch12_guiwu", "trigger_zoupian_c", "ch12_guiwu"}。
-- 本脚本 0 处 flag.set；ch12.guiwang 已置（鬼王已灭）则什么也不做。
-- 落点坐标照抄 tools/mapgen/genmaps_ch12.py 的 FORK_C（第三岔口 (30, 16)），不许是 (0, 0)、不与任何触发器同格。
local FORK_X, FORK_Y = 30, 16
if flag.get("ch12.guiwang") >= 1 then return end
talk("", "ch12.zoupian.fog")
teleport("ch12_guiwu", FORK_X, FORK_Y)
talk("", "ch12.zoupian.back")
