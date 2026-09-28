-- @hook ch02_wairentang npc_wairentang_yaoshang npc
-- 外刃堂药商的闲聊与开市。挂在 ch02_wairentang 的 npc 对象上
-- （role_id=wairentang_yaoshang）。
--
-- 他的三句闲话把「年份即价值」这条规则从另一个角度再说一遍，
-- 但一次也不报倍数：门里这几年收上来的都是一两年的薄货、真正值钱的他一年
-- 见不着两株、小秤只称寻常货，值钱的要请大秤出来。
-- 第三句是给段末那场算账埋的引子——玩家看见他起身进屋搬大秤时，
-- 自己心里那个数就跳出来了，不必有人替他说。
--
-- 主线的三次交易走脚本直接结算（六块、二十二、一百四十五），
-- 这里的 shop() 是给玩家在剧情之外自由买卖用的，
-- 商店定义见 data/shops/ch02_wairentang_yaoshang.json。

if flag.get("ch02.liao_yaoshang") == 1 then
    talk("wairentang_yaoshang", "ch02.npc.yaoshang_again")
    shop("ch02_wairentang_yaoshang")
    return
end

talk("wairentang_yaoshang", "ch02.npc.yaoshang_1")
talk("wairentang_yaoshang", "ch02.npc.yaoshang_2")
talk("wairentang_yaoshang", "ch02.npc.yaoshang_3")

flag.set("ch02.liao_yaoshang")

shop("ch02_wairentang_yaoshang")
