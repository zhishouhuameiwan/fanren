-- @hook ch12_guiwu trigger_zoupian_a enter
-- 走偏（改编，docs/ch12-design.md 3.1、3.2、第 7 节，16.1 第 39 条）：第一处错路尽头，鬼雾把人推回第一岔口。
-- kScriptTransfers 五元行 {"ch12.tupo", "scripts/ch12/zoupian_a.lua", "ch12_guiwu", "trigger_zoupian_a", "ch12_guiwu"}。
-- 本脚本 0 处 flag.set；ch12.guiwang 已置（鬼王已灭）则什么也不做。
-- 落点坐标照抄 tools/mapgen/genmaps_ch12.py 的 FORK_A（第一岔口 (27, 33)），不许是 (0, 0)、不与任何触发器同格。
local FORK_X, FORK_Y = 27, 33
if flag.get("ch12.guiwang") >= 1 then return end
talk("", "ch12.zoupian.fog")
teleport("ch12_guiwu", FORK_X, FORK_Y)
talk("", "ch12.zoupian.back")
