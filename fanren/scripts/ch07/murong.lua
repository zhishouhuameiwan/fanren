-- @hook ch06_huangfenggu trigger_murong enter once
-- 第七章节点 6「东口坡上：慕容兄弟与陆师兄」。挂在 ch06_huangfenggu 往百药园的东口咽喉 (46,26)(46,27)
-- （mode=enter，once=true，guard_flag=ch07.chouhan，set_flag=ch07.murong）。坡上的热闹是撞上的。
--
-- 施工图写的是「山头」：本作把这一场放在他回园子必经的东口那片坡上（黄枫谷那张图上没有山头），
-- 七个人站在路两边（visible ch07.chouhan / hidden ch07.murong，genmaps_ch07.patch_huangfenggu）。
-- 框架件（施工图 13.1 第 6 段）：慕容兄弟（雷灵根）当众演雷法；陆师兄（风灵根，原本低阶弟子里唯一的异灵根）
-- 不服、夸口硬接；陆的女伴陈师妹在场；陆以护罩扛下、反手青弧斩追人；一个少年往他身后躲，他闪开；
-- 另一少年躲到粗矮青年身后，土墙挨了一刀；蓝衣女子以火焰鸟收场，罚了兄弟、谢了粗矮青年，临走传音说他
-- 只顾自己——他不以为然；陆师兄看清了他的脸（节点 13 的由头）。蓝衣女子不点名（聂盈 ch708，施工图 12.1）。

talk("", "ch07.murong.crowd")
talk("", "ch07.murong.twins")
talk("huangfeng_weiguan", "ch07.murong.whisper")
talk("", "ch07.murong.lu")
talk("lu_shixiong", "ch07.murong.lu_boast")
talk("", "ch07.murong.who")
talk("", "ch07.murong.clash")
talk("", "ch07.murong.back")
talk("", "ch07.murong.boy1")
talk("", "ch07.murong.boy2")
talk("", "ch07.murong.bird")
talk("lanyi_nvzi", "ch07.murong.stop")
talk("", "ch07.murong.thank")
talk("lanyi_nvzi", "ch07.murong.voice")
talk("", "ch07.murong.shrug")
talk("", "ch07.murong.lu_eye")

flag.set("ch07.murong")
