-- @hook ch06_sanxiu_lou trigger_bilu interact once
-- 第六章节点 10b「青溪笔录；识破」。挂在 ch06_sanxiu_lou 韩立房里的床 (27,2)(28,2)
-- （mode=interact，once=true，guard_flag=ch06.canpian，set_flag=ch06.shengxianling）。躺在床头翻书的是他（ch141）。
--
-- **那块牌子从这一刻起有了名字**（施工图 0 第 2 条、验收 7）：末几页七幅令牌图，认令不认人；他掏出黑糊糊的牌子对图——
-- 黄枫谷。本章收走黑牌子、给出升仙令各只此一处，都在这里；看返回值。
-- 牌子的来历（秦家、叶家、金光上人）韩立不知道（ch141 明写），**游戏不演**，只写在物品 note 里。
-- 「升仙令」三个字从本节起可以出现（12.2）。

talk("", "ch06.bilu.lie")
talk("", "ch06.bilu.what")
talk("", "ch06.bilu.odd")
talk("", "ch06.bilu.last")
talk("", "ch06.bilu.text")
talk("", "ch06.bilu.note")
talk("", "ch06.bilu.heart")

if not take("material_heipai", 1) then
    talk("", "ch06.bilu.nopai")
    return
end
give("story_shengxianling", 1)

talk("", "ch06.bilu.match")
talk("", "ch06.bilu.huangfeng")
talk("", "ch06.bilu.why")
talk("", "ch06.bilu.plan")

flag.set("ch06.shengxianling")
