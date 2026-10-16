-- @hook ch13_waihai trigger_hui_wuzhong enter
-- 往返 #7（docs/ch13-design.md 3.1 末表）：trigger enter、once 否、guard ch13.lifu、set_flag 空；本脚本 0 处 flag.set。
-- 开头 ch13.lidao 已置 → 一句 return；choice 去 / 不去，取消与「不去」都在顶格 return；
-- 顶格 advance_days(12)、teleport("ch13_wuzhong")（落 andeng；kScriptTransfers 五元行要顶格 teleport，16.1 第 60 条）。
if flag.get("ch13.lidao") ~= 0 then talk("", "ch13.hui_wuzhong.closed"); return end
talk("", "ch13.hui_wuzhong.ask")
local go = choice({"ch13.hui_wuzhong.opt_go", "ch13.hui_wuzhong.opt_stay"})
if go ~= 1 then return end
talk("", "ch13.hui_wuzhong.go")
advance_days(12)
teleport("ch13_wuzhong")
