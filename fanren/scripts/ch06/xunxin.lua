-- @hook ch06_tainan_gu trigger_ye_xunxin enter once
-- 第六章节点 6「叶家寻衅」。三场必打之一：b06_yejia_xunxin。挂在 ch06_tainan_gu 摊位区口子里头一格 (19,18)(19,19)
-- （mode=enter，once=true，guard_flag=ch06.feixingfu，set_flag=ch06.xunxin）。换完飞行符出摊位区必踩。
--
-- **改编**（施工图 3.2 节点 6、8.3）：原著没写叶豹回来。钩子是 ch130 青纹说散修常被大家族欺辱才结伙、
-- ch131 叶豹当众下不来台撂下狠话。这里补一场：第二天叶豹带两个族中子弟回摊位区寻那草帽青年，青年已收摊，
-- 他们把气撒在附近的散修头上；青纹道士领头拦下（入伙时说过的「外部纠纷统一行动」），韩立站在其中——
-- 他不是挑头的，不站出去就是退伙。
-- 我方韩立 + 友军三人（青纹、黑木、熊大力；编成写死）。友军原写吴九指——他节点 7 才头一回见面，节点 7 之前任何文案
-- 不点他的名（校对 HIGH-4，施工图 16.4）；熊大力节点 4 已引见、背刀、天哑，不用给台词。can_escape 假、defeat_is_fatal 假：
-- 赢了输了都是青颜真人到场叫停、两边各退一步，**两条结局都不置分岔旗标**（叶家此后不回来）。
-- 两条结局的旁白都不替原著补下文。开头拨 1 天：「隔了一夜」是他换完飞行符的次日一早（校对 LOW-8，施工图 3.3 补了这一行）。

advance_days(1)

talk("", "ch06.xunxin.morning")
talk("ye_bao", "ch06.xunxin.shout")
talk("", "ch06.xunxin.shove")
talk("qingwen_daoshi", "ch06.xunxin.step")
talk("ye_bao", "ch06.xunxin.sneer")
talk("", "ch06.xunxin.stand")

local won = battle("b06_yejia_xunxin")

if won then
    talk("", "ch06.xunxin.won")
else
    talk("", "ch06.xunxin.lost")
end

talk("qingyan_zhenren", "ch06.xunxin.stop")
talk("", "ch06.xunxin.part")
talk("qingwen_daoshi", "ch06.xunxin.lou")
talk("", "ch06.xunxin.lesson")

flag.set("ch06.xunxin")
