-- @hook ch03_guwai trigger_ligu interact once
-- 第三章节点 12 之三「离谷」，章末。置 ch03.done。
-- 挂在 ch03_guwai 谷口的 trigger（mode=interact，once=true，set_flag=ch03.done）。
--
-- 章末不给他任何志向。他清点行李、埋人、拆屋、回头看一眼那几畦地，什么也没拿。
-- 最后一段落在两件事上：
--   * 那只捡了四年的小瓶子——他用旧衣裳裹了三层放在最里头。
--     第 2 章他没跟任何人说过它，这一章他也不打算说。掌天瓶这条线从这里继续往下走。
--   * 「曲魂，跟上」——这句话说给一个不会答话的人听，往后他还要说很多年。
--     他说完自己笑了一下，这是他这一天第二次笑。
--
-- 最后一条（ch03.ligu.end）是全章的结账句：
--   「想活下去，就得比要你命的那个人早想一步。」
-- 这是他这一年学会的东西，不是他立下的志向；后半句「他那年十六岁」把它按回
-- 一个少年身上，免得读成箴言。
--
-- **本节不调 teleport()。** ch03_guwai / ch03_mishi 这几张图的出入口归关卡侧，
-- 且校验器会拿 teleport 的目标去比对 maps/ 下真实存在的地图 id；
-- 由脚本硬传一个坐标，等于把关卡的出口写死在编剧这一侧。

if flag.get("ch03.quhun_rudui") ~= 1 then
    talk("", "ch03.ligu.gate")
    return
end

if flag.get("ch03.done") == 1 then
    talk("", "ch03.ligu.again")
    return
end

talk("", "ch03.ligu.clean")
talk("", "ch03.ligu.field")
talk("", "ch03.ligu.pack")
talk("", "ch03.ligu.bottle")

advance_days(1)

talk("", "ch03.ligu.leave")
talk("", "ch03.ligu.look")
talk("", "ch03.ligu.last")
talk("", "ch03.ligu.end")

flag.set("ch03.done")
