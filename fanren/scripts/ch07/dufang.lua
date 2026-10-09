-- @hook ch06_baiyaoyuan trigger_dufang interact once
-- 第七章节点 7「读方；马师伯：没有种子」。挂在 ch06_baiyaoyuan 居室的书桌 (26,4)
-- （mode=interact，once=true，guard_flag=ch07.murong，set_flag=ch07.dufang）。回屋读方的是他。
--
-- 框架件（施工图 13.1 第 7 段）：辅药三十一味，园里都有、要数百年火候（他有瓶）——不点药名；
-- 主药玉髓芝、天灵果、紫猴花，园里一株也没有；定颜丹：药常见、要千年，往后再说（本章不炼）；
-- 六七日后马师伯来：三味主药天生自长、没有种子、幼苗离了原地难活；不肯说出处，只说去了等于送死。
-- advance_days(6)：「六七日后」（施工图 3.3）。马师伯本章不摆 NPC，只在挂点里说话（3.1 开头）。

talk("", "ch07.dufang.read")
talk("", "ch07.dufang.aux")
talk("", "ch07.dufang.bottle")
talk("", "ch07.dufang.main")
talk("", "ch07.dufang.dingyan")

advance_days(6)

talk("", "ch07.dufang.ma_came")
talk("ma_shibo", "ch07.dufang.ma_seed")
talk("ma_shibo", "ch07.dufang.ma_move")
talk("", "ch07.dufang.where")
talk("ma_shibo", "ch07.dufang.ma_death")
talk("", "ch07.dufang.think")

flag.set("ch07.dufang")
