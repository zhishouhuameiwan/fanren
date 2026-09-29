-- @hook ch06_tainan_gu trigger_ruhuo enter once
-- 第六章节点 4「入伙」。挂在 ch06_tainan_gu 雾道北口 (23,26)(24,26)
-- （mode=enter，once=true，guard_flag=ch06.qingyan，set_flag=ch06.ruhuo）。青纹道士一行是从背后找上来的（ch130）。
--
-- 原著 ch130 这位道士自报「松纹」，此后全写「青纹」——本作统一叫青纹道士，「松纹」0 处（施工图 12.2）。
-- 二选一（施工图 3.2 节点 4）：两条都入伙。
--   1 先问清楚再入（原著）——道士说问得好；
--   2 立刻入——胡萍姑多一句「这位倒爽快」。节点 7 吴九指的一句话按它分。取消按 1 算。
-- 给禁法符（story_jinfa_fu）；小楼的门 require_flag=ch06.ruhuo。交易的两种方式放到 5a 讲，这里只提一句忌讳与惯例。
-- 次序（复验整改 16.5，§1.4 第 3、4 行）：开场由道士一个人说完（妇人只打量、不接口）；引见照 13.1 的列法，
-- 青纹先报自己，再黑木黑金、红莲、苦桑，最后由他引见胡萍姑夫妇，胡萍姑只补一句熊大力不会说话。

talk("qingwen_daoshi", "ch06.ruhuo.hail")
talk("", "ch06.ruhuo.turn")
talk("", "ch06.ruhuo.why")
talk("qingwen_daoshi", "ch06.ruhuo.bully")

local pick = choice{
    "ch06.ruhuo.opt_ask",
    "ch06.ruhuo.opt_join",
}

local ruhuo = 1
if pick == 2 then
    ruhuo = 2
    talk("", "ch06.ruhuo.join")
    talk("hu_pinggu", "ch06.ruhuo.quick")
else
    talk("", "ch06.ruhuo.ask")
    talk("qingwen_daoshi", "ch06.ruhuo.laugh")
end

talk("qingwen_daoshi", "ch06.ruhuo.self")
talk("qingwen_daoshi", "ch06.ruhuo.names1")
talk("qingwen_daoshi", "ch06.ruhuo.names2")
talk("qingwen_daoshi", "ch06.ruhuo.names3")
talk("hu_pinggu", "ch06.ruhuo.names4")
talk("", "ch06.ruhuo.weigh")
talk("qingwen_daoshi", "ch06.ruhuo.rules")
talk("", "ch06.ruhuo.fu")
give("story_jinfa_fu", 1)
talk("", "ch06.ruhuo.alone")

flag.set("ch06.ruhuo", ruhuo)
