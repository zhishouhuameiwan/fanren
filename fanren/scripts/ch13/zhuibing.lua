-- @hook ch13_nanming trigger_chugang enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 5。3.2 n05：choice「随行 / 不随」，两支各自开打（「随行」不写成
-- 假意应下、随即出手，13 第 3 条）；取消在战前 return；battle("b13_nanming_zhuibing")，败 game_over；
-- 胜：give story_mihuan_yan 1、story_baiyu_dayin 1、material_lingshi 600；advance_days(1)；置 ch13.zhuibing。
-- 死亡只写倒下；「迷幻砚」不进本段文案（物品名由物品表给，12.2）；剑上无神雷（A26）。
talk("", "ch13.zhuibing.chase")
talk("nixing_rusheng", "ch13.zhuibing.demand")
local pick = choice({"ch13.zhuibing.opt_go", "ch13.zhuibing.opt_refuse"})
if not pick then return end
if pick == 1 then
    talk("", "ch13.zhuibing.go")
    talk("nixing_dahan", "ch13.zhuibing.bag")
    talk("hanli", "ch13.zhuibing.nobag")
else
    talk("hanli", "ch13.zhuibing.refuse")
    talk("", "ch13.zhuibing.block")
end
local won = battle("b13_nanming_zhuibing")
if not won then game_over(); return end
talk("", "ch13.zhuibing.win")
give("story_mihuan_yan", 1)
give("story_baiyu_dayin", 1)
give("material_lingshi", 600)
talk("", "ch13.zhuibing.loot")
advance_days(1)
talk("hanli", "ch13.zhuibing.call")
talk("", "ch13.zhuibing.surface")
flag.set("ch13.zhuibing")
