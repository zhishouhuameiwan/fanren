-- @hook ch07_jindi_waiwei trigger_yixiantian enter once
-- 第七章节点 22「一线天」。战斗②。挂在 ch07_jindi_waiwei 一线天北口 (22,10)(23,10)
-- （mode=enter，once=true，guard_flag=ch07.sixian，set_flag=ch07.yixiantian）。一出路口就被堵。
--
-- 框架件（施工图 13.1 第 22 段）：两侧峭壁一条小路；禁地里御器飞行等于当靶子；络腮胡子（十三层）与天阙堡严姓（十二层顶峰）
-- 前后堵住——两人借融灵符落在一处、专杀人夺宝；胡子记恨禁地外那个鬼脸；**战斗②** b07_yixiantian（致命、不可逃）；
-- 毁尸，带走五六个储物袋（东西在节点 34 清点，这里不给）。原著那几句骂人的话与「像被蚊子叮了一下」不用。

talk("", "ch07.yixiantian.path")
talk("", "ch07.yixiantian.noflight")
talk("", "ch07.yixiantian.block")
talk("luosai_huzi", "ch07.yixiantian.huzi")
talk("yan_xiongdi", "ch07.yixiantian.yan")
talk("", "ch07.yixiantian.fu")

local won = battle("b07_yixiantian")
if not won then
    talk("", "ch07.yixiantian.lost")
    game_over()
    return
end

talk("", "ch07.yixiantian.after")
talk("", "ch07.yixiantian.bags")
talk("", "ch07.yixiantian.burn")

flag.set("ch07.yixiantian")
