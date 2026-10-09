-- @hook ch07_jindi_zhongxin trigger_yueyang enter once
-- 第七章节点 25「月阳宝珠」。挂在 ch07_jindi_zhongxin 上山小路口 (40,7)(41,7)
-- （mode=enter，once=true，guard_flag=ch07.zhongwu，set_flag=ch07.yueyang）。光雨是看见的。
--
-- 框架件（施工图 13.1 第 25 段）：上山只走资料里几条安全的小路，他在一处路口等；一道白光冲天、化作光雨落进雾里，雾散了；
-- 本次执掌的是天阙堡；他第一次见识法宝的威力（写羡慕）；环形山现形，有人从各处进山；他最后一个进山。
-- 进山的那几个人不点名、不写样貌（原著那三个人的样子是质感）。置 ch07.yueyang：北缘通环形山的门开、环形山遭遇区开。

talk("", "ch07.yueyang.banner")
talk("", "ch07.yueyang.wait")
talk("", "ch07.yueyang.light")
talk("", "ch07.yueyang.rain")
talk("", "ch07.yueyang.envy")
talk("", "ch07.yueyang.mountain")
talk("", "ch07.yueyang.others")
talk("", "ch07.yueyang.last")

flag.set("ch07.yueyang")
