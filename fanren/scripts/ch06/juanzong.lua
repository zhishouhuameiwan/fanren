-- @hook ch06_huangfenggu trigger_juanzong interact once
-- 第六章节点 17b「内殿卷宗；悔诺」。挂在 ch06_huangfenggu 百机堂内殿的卷宗架 (4,3)(5,3)
-- （mode=interact，once=true，guard_flag=ch06.zawu，set_flag=ch06.huinuo）。跟叶堂主进内殿翻卷宗的是他。
--
-- 卷宗：前几任管园弟子的自述与辩解，三条各一句（本作自拟，不照原著）；「奖励丰厚但不是普通弟子做得来的」；他坚持。
-- **悔诺**：「交易的东西能不能缓一缓……要炼一炉合气丹，手头紧」。二选一（施工图 3.2 节点 17）：取消按 1 算。
--   1 忍：就算晚辈的孝敬（原著）——叶师叔板起脸「老夫岂是悔约的人」，他改口「暂放师叔那里」；
--   2 追问：师叔说过的东西一样也不能少——叶师叔脸一板，气氛僵；节点 18 送来的东西更少（16.1 第 11 条保留）。
-- 玉牌 story_yaoyuan_yupai。通百药园的门 require_flag=ch06.huinuo。
-- 殿口叶师叔与吴师兄那段话韩立不在场——**不演**，也不写「他不知道殿口两个人在说他」（施工图 3.2 节点 17）。

talk("", "ch06.juanzong.room")
talk("", "ch06.juanzong.scroll")
talk("", "ch06.juanzong.first")
talk("", "ch06.juanzong.second")
talk("", "ch06.juanzong.third")
talk("ye_shishu", "ch06.juanzong.change")
talk("", "ch06.juanzong.insist")
talk("ye_shishu", "ch06.juanzong.delay")
talk("", "ch06.juanzong.sink")

local pick = choice{
    "ch06.juanzong.opt_bear",
    "ch06.juanzong.opt_press",
}

local huinuo = 1
if pick == 2 then
    huinuo = 2
    talk("", "ch06.juanzong.press")
    talk("ye_shishu", "ch06.juanzong.stiff")
    talk("", "ch06.juanzong.frost")
else
    talk("", "ch06.juanzong.bear")
    talk("ye_shishu", "ch06.juanzong.face")
    talk("", "ch06.juanzong.keep")
    talk("ye_shishu", "ch06.juanzong.nod")
end
talk("", "ch06.juanzong.weak")
give("story_yaoyuan_yupai", 1)
talk("", "ch06.juanzong.pai")

flag.set("ch06.huinuo", huinuo)
