-- @hook ch07_yuelu_dian trigger_cangshi interact once
-- 第七章节点 4b「许老；藏室」。挂在 ch07_yuelu_dian 藏室的书桌 (8,5)(9,5)
-- （mode=interact，once=true，guard_flag=ch07.yuelu，set_flag=ch07.cangshi）。在书桌前翻玉筒的是他。
--
-- 框架件（施工图 13.1 第 4b 段）：许老筑基、不许人叫师伯（好明着要钱）；一个时辰一块、复制一份十块；
-- 楼上多是世俗医书与几捆同档丹方（不点书名）；两枚玉筒——筑基丹方须先天真火（筑基后才有：死循环），
-- 定颜丹方只驻颜、不要真火；复制两份共二十块；收了钱才说：大半丹方随天材地宝绝迹而失传，筑基丹是留下来的一种。
-- 置 ch07.cangshi 即两张配方在炼制面板上出现（E1，requireFlag）。「定颜丹」三个字从这一节起才出现（12.2）。
--
-- 灵石：一个时辰一块 ×2 ＋ 复制 20 = 22。先数够再动手，免得扣了一半卡住（第 9 节第 1 条：这时最低也有 166）；
-- 每一处 take 仍看返回值。不够就 return，触发器留着。
-- 校对整改 16.5（§4.2(e) 第 7 行）：两个时辰的钱上楼之前一次付清（他自己的算计），不再是看到一半有人催。

talk("", "ch07.cangshi.stairs")
talk("xu_lao", "ch07.cangshi.xu_price")
talk("xu_lao", "ch07.cangshi.xu_name")
if item.count("material_lingshi") < 22 then
    talk("", "ch07.cangshi.poor")
    return
end
if not take("material_lingshi", 1) then
    talk("", "ch07.cangshi.poor")
    return
end
talk("", "ch07.cangshi.hour2")
if not take("material_lingshi", 1) then
    talk("", "ch07.cangshi.poor")
    return
end
talk("", "ch07.cangshi.shelves")
talk("", "ch07.cangshi.tubes")
talk("", "ch07.cangshi.zhuji")
talk("", "ch07.cangshi.loop")
talk("", "ch07.cangshi.dingyan")
talk("", "ch07.cangshi.dingyan_keep")
talk("", "ch07.cangshi.copy")
if not take("material_lingshi", 20) then
    talk("", "ch07.cangshi.poor")
    return
end
talk("xu_lao", "ch07.cangshi.xu_lost")
talk("xu_lao", "ch07.cangshi.xu_left")
talk("", "ch07.cangshi.end")

flag.set("ch07.cangshi")
