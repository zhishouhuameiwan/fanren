-- @hook ch08_yuejing trigger_houmen interact once
if flag.get("ch08.xiao_qiu") == 0 or flag.get("ch08.qb_yuanbing") ~= 0 or flag.get("ch08.xiao_jiaoyi") ~= 0 then return end
talk("xiao_zhen", "ch08.houmen.t1")
talk("", "ch08.houmen.t2")
flag.set("ch08.xiao_jiaoyi", 1)
