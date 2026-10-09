-- @hook ch07_yuelu_dian trigger_chouhan interact once
-- 第七章节点 35a「丑汉与石门」。挂在 ch07_yuelu_dian 丑汉面前一格 (43,5)（他站 (43,4)、面朝南；施工图 3.1：同一格只挂一个对象）
-- （mode=interact，once=true，guard_flag=ch07.sannian，set_flag=ch07.dihuo）。把丑汉弄醒的是他。
--
-- 框架件（施工图 13.1 第 35a 段）：再进岳麓殿（传送阵一块）；丑汉在睡，他把人弄醒（怎么弄醒本作自出，原著那只铃铛不用）；
-- 报出李师祖门下，丑汉不信——亮出署名的剑诀，丑汉认得笔迹，立刻换了脸色；要一间火力稳的；一块中阶灵石作定金
-- （公认一百低阶兑一块，却没人肯换）；令箭开石门。置 ch07.dihuo：五彩石门（portal_to_dihuo）从此开着。
-- **死局自查**：传送阵那一块、定金那一块中阶都「有则扣」——节点 34 刚给了中阶 15 块、低阶 300 块，可玩家在坊市把灵石花光、
-- 把中阶灵石在打坐时吃掉（restoreMp）都是合法的：没有灵石，守阵的弟子记一笔账；没有中阶，按公认的兑率收一百块低阶；
-- 都没有，丑汉看在剑诀的份上许他先欠着（记第 18 节）。主线不因任何一样东西卡住。

if take("material_lingshi", 1) then
    talk("", "ch07.chouhan.fee")
else
    talk("", "ch07.chouhan.nofee")
end
talk("", "ch07.chouhan.snore")
talk("", "ch07.chouhan.wake")
talk("chou_han", "ch07.chouhan.grumble")
talk("", "ch07.chouhan.who")
talk("chou_han", "ch07.chouhan.doubt")
talk("", "ch07.chouhan.book")
talk("", "ch07.chouhan.change")
talk("chou_han", "ch07.chouhan.change_say")
talk("", "ch07.chouhan.room")
if take("material_lingshi_zhong", 1) then
    talk("", "ch07.chouhan.deposit")
elseif take("material_lingshi", 100) then
    talk("", "ch07.chouhan.deposit_low")
else
    talk("chou_han", "ch07.chouhan.deposit_owe")
end
talk("", "ch07.chouhan.rate")
talk("", "ch07.chouhan.gate")

flag.set("ch07.dihuo")
