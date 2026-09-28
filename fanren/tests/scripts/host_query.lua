-- 只读查询穿插在挂起之间。查询是 C++ 注册的函数，它们同步返回、不挂起，
-- 所以 yield 依然只发生在 Lua 帧之间。
local f = flag.get("probe_flag")
local n = item.count("probe_item")
local r = realm.at_least(realm.QI_REFINING_3)
talk("probe", "q." .. tostring(f) .. "." .. tostring(n) .. "." .. tostring(r))
talk("probe", "again." .. tostring(flag.get("probe_flag")))
