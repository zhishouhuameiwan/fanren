-- @hook ch08_yuejing trigger_xiqu interact once
if flag.get("ch08.xiao_jiaoyi") == 0 or flag.get("ch08.qb_yuanbing") ~= 0 or flag.get("ch08.xiao_tuijian") ~= 0 then return end
talk("", "ch08.xiqu.t1")
give("story_canque_daoshu", 1)
talk("", "ch08.xiqu.t2")
flag.set("ch08.xiao_tuijian", 1)
