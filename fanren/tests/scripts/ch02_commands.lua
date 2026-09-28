-- 四条新命令的 Lua 封装各发一次，验证 api.lua 造出的表能被解析表认出来，
-- 且参数落在契约规定的槽位上（advance_days 用 x，field_unlock 用 a + x）。
advance_days(30)
bottle.grant()
bottle.unlock_mature()
field.unlock("shenshougu_yaopu", 8)
-- 省略槽位数：api.lua 该补上默认的 4。
field.unlock("mo_yuan")
