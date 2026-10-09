-- @hook ch07_yuelu_dian trigger_dihuo_wen interact once
-- 第七章节点 5a「地肺之火；银丝鼎」。挂在 ch07_yuelu_dian 许老的柜台 (8,15)
-- （mode=interact，once=true，guard_flag=ch07.cangshi，set_flag=ch07.yinsi）。回头问许老地火、挑丹炉的是他。
--
-- 框架件（施工图 13.1 第 5a 段）：地肺之火可以替真火；玄阳火地在没标记的那条通道尽头，交灵石借用；
-- 他买最小的银丝鼎（储物袋只装得下它），三十二块；在许老这里前后花了五十几块。
-- 本作由他开口问（原著是许老推销丹炉时顺嘴一句）；许老从袖里抖鼎那一类原著动作不用。

talk("", "ch07.dihuo_wen.ask")
talk("xu_lao", "ch07.dihuo_wen.fire")
talk("xu_lao", "ch07.dihuo_wen.where")
talk("", "ch07.dihuo_wen.half")
talk("xu_lao", "ch07.dihuo_wen.ding")
talk("", "ch07.dihuo_wen.pick")
if not take("material_lingshi", 32) then
    talk("xu_lao", "ch07.dihuo_wen.poor")
    return
end
give("story_yinsi_ding")
talk("", "ch07.dihuo_wen.sum")

flag.set("ch07.yinsi")
