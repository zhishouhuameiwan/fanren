-- @hook ch01_shenshougu trigger_mogui enter once
-- 第三章节点 2「墨大夫归谷，神色不同往日」。
-- 挂在 ch01_shenshougu 谷中南北主径北段 (19,8) 起两格的 trigger
-- （mode=enter，once=true，guard_flag=ch03.elang_done，set_flag=ch03.mo_gui_gu）。
--
-- **这一行从前写的是「ch02_yaopu 畦头」，那是注释错了，不是地图错了。**
-- 裁决过一次，结论是不动地图，两条理由：
--   1. ch03.mo_gui_gu 是密室那道门（portal_to_ch03_mishi）的钥匙，规则 19 要求它
--      静态拿得到，所以置它的触发器必须落在一张玩家一定走得到的图上；
--      谷外那一仗打完，从谷西豁口回来、往北去药圃与居所，走的就是这一段主径。
--   2. 这一节的题目是**归谷**——他是走进谷里来的。谷中那条南北主径是进谷必经，
--      挂在那儿比挂在任何一畦地头都更贴这两个字。
--   （2026-09-27 神手谷南北对调：谷口从北墙挪到南墙，药圃改接北口。这一格没挪——
--   他从南边谷口跟进来，韩立正往北走，咳嗽声落在背后，比从前更贴 ch03.mogui.cough。）
--
-- 那本节这一身药圃气味（卯时畦头那一声没喊、锄头靠在老地方、水声是昨天忘了关的
-- 那道沟）怎么说：**它整节是追述，不是当场。** 从 ch03.mogui.morning 的
-- 「那天早上」起，到 ch03.mogui.weight 的「他后来留意过很多次」、
-- ch03.mogui.night 的「那夜」，通篇过去时，没有一句断言玩家此刻站在哪一格。
-- 神手谷自己也有一处 facility_yaopu（(28,7)，与触发格同一排、东九格），
-- 那几畦本就在这张图上。而 ch03.mogui.again 那句「往后他每天都要多看一眼
-- 谷口那条道」，说的正是玩家脚下这条道。
-- 这一点与独立校对 MEDIUM-2 那几条不同：tiezhe / quhun 的文案写的是
-- 「他哪儿也不去」「他走出去」，那是对当下位置的断言，所以那边改的是文案。
--
-- 这一节的难处是：**不许写他可疑**。第 1、2 章反复要求那位大夫不得显得可疑，
-- 为的就是下一节。所以这里一句打量、一个意味深长的停顿、一丝笑意都不许有。
--
-- 换的是取材角度：不描写他的脸，描写**规矩被破了**。
--   * 卯时畦头那一声没喊（四年没断过）；
--   * 咳嗽声该在谷口响，这一回响在背后，人已经进来了；
--   * 搭完脉不点头、不说下回再看，转身就走。
-- 玩家看见的每一样都能用「他病重了」解释得通，可每一样又都不对劲。
-- 这正是设计第 0 节要的：回头想得起来，当时不觉得被骗。
--
-- 两处伏笔在这里埋下，节点 3 与节点 8 各兑现一处：
--   1. **掌心朝上的手势**——从前是要他递药杵的。节点 3 墨居仁用同一个手势要那只铁筒。
--   2. **那个不出声的两只脚上分量一样**——活人站久了要换脚，它不换。
--      节点 8 暗道里第十一只笼中物走路也是这样，韩立就是从这一点推出巨汉是什么。
-- 这两笔都不许在本节做任何解释，写完就走。
--
-- 还有一处是给节点 8 的：那一夜两个人关在屋里，一句话也没传出来。
-- 玩家这时只会觉得气氛压人；到节点 8 他才会明白，另一个根本不用出声。

if flag.get("ch03.elang_done") ~= 1 then
    talk("", "ch03.mogui.gate")
    return
end

if flag.get("ch03.mo_gui_gu") == 1 then
    talk("", "ch03.mogui.again")
    return
end

talk("", "ch03.mogui.morning")
talk("", "ch03.mogui.quiet")
talk("", "ch03.mogui.cough")
talk("", "ch03.mogui.turn")
talk("", "ch03.mogui.giant")
talk("", "ch03.mogui.weight")

-- 摊牌之前他一律用 mo_daifu 说话，名牌上写的是「墨大夫」。
-- 玩家是从名牌换成「墨居仁」那一刻知道自己认识的人换了称呼的，
-- 所以这一节一个字也不能提前透（见 data/roles/mo_juren.json 的 note）。
talk("mo_daifu", "ch03.mogui.ask")
talk("", "ch03.mogui.answer")
talk("", "ch03.mogui.nonod")
talk("", "ch03.mogui.pulse")
talk("", "ch03.mogui.breath")
talk("", "ch03.mogui.nothing")

talk("", "ch03.mogui.hanli")
talk("", "ch03.mogui.counted")
talk("", "ch03.mogui.night")

advance_days(1)

talk("", "ch03.mogui.end")

flag.set("ch03.mo_gui_gu")
