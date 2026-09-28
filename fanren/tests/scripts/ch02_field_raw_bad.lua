-- 负向：绕开 api.lua 的 assert，把非法参数直接丢给 game 层。
--
-- 契约要求 dispatch 自己也判 ok = false，不能只靠 Lua 侧那一道 assert：
-- 命令表将来也可能由存档回放或编辑器生成，那些路径上没有 assert 兜着。
local no_id = coroutine.yield{ kind = "field_unlock", a = "", x = 4 }
local no_slots = coroutine.yield{ kind = "field_unlock", a = "shenshougu_yaopu", x = 0 }
local negative = coroutine.yield{ kind = "field_unlock", a = "shenshougu_yaopu", x = -8 }
-- 对照组：同一条命令参数合法时必须成功，否则上面三个 0 只能说明「全都失败」。
local good = coroutine.yield{ kind = "field_unlock", a = "shenshougu_yaopu", x = 2 }

flag.set("no_id_ok", no_id.ok and 1 or 0)
flag.set("no_slots_ok", no_slots.ok and 1 or 0)
flag.set("negative_ok", negative.ok and 1 or 0)
flag.set("good_ok", good.ok and 1 or 0)
