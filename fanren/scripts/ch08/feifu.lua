-- @hook ch08_dongfu trigger_feifu enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.zhongqi") == 0 or flag.get("ch08.zhaoji") ~= 0 then return end
talk("", "ch08.feifu.t1")
teleport("ch08_jinguyuan", 24, 32)
flag.set("ch08.zhaoji", 1)
