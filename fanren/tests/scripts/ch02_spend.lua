-- 由 ScriptApiTests 驱动：倒出绿液本身，不催熟任何东西。
--
-- shiyao.lua 试药那一碗走的就是这一条：此刻 matureKnown 还是假的，
-- 走催熟只会被规则层挡下而一滴不扣。
local count = flag.get("test_drops")
local ok = bottle.spend(count)

flag.set("spend_ok", ok and 1 or 0)
flag.set("drops_after", bottle.drops())
