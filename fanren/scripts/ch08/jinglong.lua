-- @hook ch06_baiyaoyuan trigger_jinglong enter once
-- Chapter 8; framework boundary: docs/ch08-design.md 13.1.
if flag.get("ch08.huifu") == 0 or flag.get("ch08.done") ~= 0 then return end
if flag.get("ch08.xiao_tuijian") ~= 0 then
talk("xiao_cuier", "ch08.jinglong.t1")
end
talk("ma_shixiong", "ch08.jinglong.t2")
talk("", "ch08.jinglong.t3")
flag.set("ch08.done", 1)
