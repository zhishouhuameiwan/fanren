-- @hook ch05_mofu trigger_jiaoyi interact once
-- 第五章节点 11a「交易」。挂在 ch05_mofu 小楼楼门中格 (30,9)（mode=interact，once=true，
-- guard_flag=ch05.duobang，set_flag=ch05.jiaoyi）。他上门要答复（ch119「最迟明天一早」）。
--
-- 原著 ch122-124：墨氏三娇都在；严氏提「灭两家、三个女儿都给你」——他拒绝（**不做选项**）；
-- 定约：杀欧阳飞天换宝玉；吴剑鸣是欧阳飞天的七弟子；马空天、赵坤闹分立的旧事（ch123）；
-- 他要严氏吞药作保；下楼时墨凤舞拦住他，求他把墨大夫的医道心得誊一份（ch124）；
-- 墨玉珠临走看他一眼，意思不明（ch124）——保持不明。
--
-- **「刀剑难伤」这一句提前到这里**（用户已同意；原著 ch126 事后才说破霸王甲，「霸王甲」之名仍留到 12e）
--   → 支线 Z2 挂起：ch05.jianfu_qiu = 1。欧阳飞天必败分支的提示一（施工图 8.4）。
-- 墨凤舞拦下他 → 支线 Z1 挂起：ch05.fengwu_qiu = 1。她开口先问什么，看 ch05.jianmianli（施工图第 7 节）：
--   1 给了萦香丸 → 问那瓶药（原著）；2 给了碎银 / 3 什么也没给 → 直接问他跟她父亲学没学医。
-- 严氏开场的口气看墨府的账（施工图第 7 节）：节点 9 他说了狠话、或者千人醉没防住让她们看见了他的狼狈，
-- 茶是凉的；否则她亲手倒茶。
-- ch05.dengmen == 1：多一句他顶回去的话（节点 7a 他看清了三夫人那一手是功夫）。
-- 二选一 → ch05.jiaoyi：1 请四夫人吞一颗（原著）/ 2 不必。取消按 2 算。12e 给不给解药就看这一位。

talk("", "ch05.jiaoyi.in")

if flag.get("ch05.duizhi") == 1 or flag.get("ch05.qianrenzui") == 2 then
    talk("", "ch05.jiaoyi.tone_cold")
else
    talk("", "ch05.jiaoyi.tone_warm")
end

talk("yan_shi", "ch05.jiaoyi.offer")
talk("", "ch05.jiaoyi.girls")
talk("", "ch05.jiaoyi.no")
talk("", "ch05.jiaoyi.relief")
talk("yan_shi", "ch05.jiaoyi.deal")
talk("yan_shi", "ch05.jiaoyi.wu")
talk("yan_shi", "ch05.jiaoyi.past")
talk("yan_shi", "ch05.jiaoyi.hard")

if flag.get("ch05.dengmen") == 1 then
    talk("", "ch05.jiaoyi.retort")
end

talk("", "ch05.jiaoyi.need")
talk("", "ch05.jiaoyi.guar")
talk("", "ch05.jiaoyi.q")

local pick = choice{
    "ch05.jiaoyi.opt_take",
    "ch05.jiaoyi.opt_skip",
}

local jiaoyi = 2
if pick == 1 then
    talk("", "ch05.jiaoyi.take")
    jiaoyi = 1
else
    talk("", "ch05.jiaoyi.skip")
end

talk("", "ch05.jiaoyi.bye")
talk("mo_caihuan", "ch05.jiaoyi.feng_run")

local jianmian = flag.get("ch05.jianmianli")
if jianmian == 1 then
    talk("mo_fengwu", "ch05.jiaoyi.feng_pill")
elseif jianmian == 2 then
    talk("mo_fengwu", "ch05.jiaoyi.feng_silver")
else
    talk("mo_fengwu", "ch05.jiaoyi.feng_none")
end

talk("mo_fengwu", "ch05.jiaoyi.feng_ask")
talk("", "ch05.jiaoyi.feng_yes")
talk("", "ch05.jiaoyi.zhu")

flag.set("ch05.jianfu_qiu")
flag.set("ch05.fengwu_qiu")
flag.set("ch05.jiaoyi", jiaoyi)
