-- @hook ch06_baiyaoyuan trigger_zhongsheng interact once
-- Facts and participants: docs/ch09-design.md 13.1.

talk("", "ch09.zhongsheng.summon")
talk("ma_shixiong", "ch09.zhongsheng.ma")
if flag.get("ch08.xiao_tuijian") == 1 then
    talk("xiao_cuier", "ch09.zhongsheng.xiao")
end
flag.set("ch09.zhongsheng")
teleport("ch06_huangfenggu", 26, 10)
