-- @hook ch08_yuejing trigger_zhulinxin enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.huanggong") == 0 or flag.get("ch08.yuehuang") ~= 0 then return end
talk("", "ch08.zhulinxin.f1")
talk("", "ch08.zhulinxin.t1")
local won = battle("b08_yuehuang")
if not won then
    game_over()
    return
end
local n = math.min(item.count("talisman_qingchi_fubao"), 1)
if n > 0 and not take("talisman_qingchi_fubao", n) then return end
if magic.knows("magic_ji_qingchi") and not magic.forget("magic_ji_qingchi") then return end
talk("", "ch08.zhulinxin.t2")
teleport("ch08_yuejing", 45, 30)
flag.set("ch08.yuehuang", 1)
