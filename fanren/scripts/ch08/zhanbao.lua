-- @hook ch08_jinguyuan trigger_zhanbao interact
if flag.get("ch08.yinian") == 0 then return end
if flag.get("ch08.daji") ~= 0 then talk("", "ch08.zhanbao.t2")
else talk("", "ch08.zhanbao.t1") end
