-- @hook ch06_baiyaoyuan trigger_shixiong interact once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch07.done") == 0 or flag.get("ch08.shixiong") ~= 0 then return end
talk("", "ch08.shixiong.t1")
talk("ma_shixiong", "ch08.shixiong.t2")
talk("ma_shixiong", "ch08.shixiong.t3")
talk("ma_shixiong", "ch08.shixiong.t4")
talk("ma_shixiong", "ch08.shixiong.t5")
if not magic.learn("magic_qingyuan_jianmang") then return end
flag.set("ch08.shixiong", 1)
