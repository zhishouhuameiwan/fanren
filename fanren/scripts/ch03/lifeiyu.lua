-- @hook ch02_wairentang npc_li_feiyu_ch03 npc
-- 厉飞雨的闲聊（第 3 章版本）。挂在 ch02_wairentang 台阶处的 npc_li_feiyu_ch03 上
-- （role_id=li_feiyu，visible_flag=ch03.mo_gui_gu）。
-- 这一段从前写的是「**关卡侧尚未给挂点**」——那句话早已过时，关卡侧挂好了没人回来改。
-- 同格的第 2 章版本 npc_li_feiyu 带 hidden_flag=ch03.mo_gui_gu，两个人按章交棒。
-- npc 对象没有 mode，也没有 once：首行的 @hook 因此只写 npc 一个取值。
--
-- 这一节存在的理由只有一条：**让韩立对朋友说一次假话。**
-- 第 2 章他为厉飞雨扎过针、挨过刀架脖子、收过他一箱药材，那条人情线是干净的。
-- 本章他被人捏住了家人和性命，而他选择一个字也不说——
-- ch03.npc.lifeiyu_hide 那一条写的是：「走出去老远他才想：这是他头一回对厉飞雨说假话。」
-- 他不是不信朋友，是算过了：说了，厉飞雨会去找墨大夫，然后死的人就是两个。
-- 这句话本节一个字也不点破，留给玩家自己算。
--
-- 厉飞雨这边保持第 2 章立住的性子：直爽、话密、讲义气、伸手按人肩膀
-- 按到一半想起手上有泥又缩回去。他还欠着一个大人情，说了三年，
-- 一回也没让韩立开过口——这一句现在读起来刺人，他自己不知道。
--
-- 分段仍旧只用本章已登记的旗标，不另立（同 guanshi.lua）。
-- 摊牌之前他只说山下的风声；摊牌之后多出那一句假话。

if flag.get("ch03.tanpai_done") ~= 1 then
    talk("li_feiyu", "ch03.npc.lifeiyu_2")
    return
end

if flag.get("ch03.shihai_done") == 1 then
    talk("li_feiyu", "ch03.npc.lifeiyu_again")
    return
end

talk("li_feiyu", "ch03.npc.lifeiyu_1")
talk("li_feiyu", "ch03.npc.lifeiyu_3")
talk("", "ch03.npc.lifeiyu_hide")
