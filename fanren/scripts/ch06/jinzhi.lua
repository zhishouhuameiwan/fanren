-- @hook ch06_baiyaoyuan trigger_jinzhi enter once
-- 第六章节点 18a「百药园：禁制、认药」。挂在 ch06_baiyaoyuan 西口两山之间的夹道 (6,11)(6,12)
-- （mode=enter，once=true，guard_flag=ch06.huinuo，set_flag=ch06.renyao）。禁制拦住他、马师伯的声音从里头传出来。
--
-- 玉牌一照绿芒，「进来吧」；茅屋数间、方田沟槽、药香。马师伯：派来的一次不如一次；知不知道完不成要罚多重。
-- 他答：园子不缩、药不夭折、按月上交——有些信心（ch151 的三条内容，话自己写）。
-- **认药三题**（施工图 3.2 节点 18、16.1 第 10 条）：马师伯指一块田，四个选项里认出那一味。题面照原著的四味草名，
-- 这里取三味（子夜花、白鹤芝、望月草），黄球草放进旁白；干扰项用本作已有的黄精、紫参、土骨花与一味瞎编的名字。
-- 选错：马师伯冷笑一声，同一题重来——不扣任何东西、不置旗标（验收 19）。取消同选错。三题都过——「够了」。
-- 墨绿木牌 story_mupai；培药心得（旁白）；私人药地，胜任了绝不亏待；他飞走了。马师伯撤场（hidden_flag=ch06.renyao）。

talk("", "ch06.jinzhi.wall")
talk("", "ch06.jinzhi.pai")
talk("ma_shibo", "ch06.jinzhi.come")
talk("", "ch06.jinzhi.garden")
talk("ma_shibo", "ch06.jinzhi.worse")
talk("ma_shibo", "ch06.jinzhi.fine")
talk("", "ch06.jinzhi.answer")
talk("ma_shibo", "ch06.jinzhi.test")

-- 第一题：子夜花（正确项放在第 3 位）
talk("ma_shibo", "ch06.jinzhi.q1")
while choice{
    "ch06.jinzhi.q1_a",
    "ch06.jinzhi.q1_b",
    "ch06.jinzhi.q1_c",
    "ch06.jinzhi.q1_d",
} ~= 3 do
    talk("ma_shibo", "ch06.jinzhi.wrong1")
end
talk("", "ch06.jinzhi.right1")

-- 第二题：白鹤芝（正确项放在第 1 位）
talk("ma_shibo", "ch06.jinzhi.q2")
while choice{
    "ch06.jinzhi.q2_a",
    "ch06.jinzhi.q2_b",
    "ch06.jinzhi.q2_c",
    "ch06.jinzhi.q2_d",
} ~= 1 do
    talk("ma_shibo", "ch06.jinzhi.wrong2")
end
talk("", "ch06.jinzhi.right2")

-- 第三题：望月草（正确项放在第 4 位）
talk("ma_shibo", "ch06.jinzhi.q3")
while choice{
    "ch06.jinzhi.q3_a",
    "ch06.jinzhi.q3_b",
    "ch06.jinzhi.q3_c",
    "ch06.jinzhi.q3_d",
} ~= 4 do
    talk("ma_shibo", "ch06.jinzhi.wrong3")
end
talk("", "ch06.jinzhi.right3")

talk("", "ch06.jinzhi.more")
talk("ma_shibo", "ch06.jinzhi.enough")
give("story_mupai", 1)
talk("ma_shibo", "ch06.jinzhi.notes")
talk("ma_shibo", "ch06.jinzhi.private")
talk("", "ch06.jinzhi.away")

flag.set("ch06.renyao")
