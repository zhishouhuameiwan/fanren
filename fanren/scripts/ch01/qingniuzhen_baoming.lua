-- @hook ch01_qingniuzhen npc_qixuan_kaoguan npc
-- @hook ch01_qingniuzhen trigger_baoming enter once
-- 第一章节点 3「青牛镇报名」。进入报名处触发。
--
-- 教与 NPC 交互：结尾三叔支开他去街上打听，把玩家推向三个闲人 NPC
-- （zhenmin_jiuke / zhenmin_yaofan / zhenmin_jiaofu 三个脚本）。
-- 聊不聊是自由的，但聊过两个以上，节点 5 攀爬开场的叙述就不一样——
-- 这是本章唯一一处「肯多问的人有好处」的回响，刻意做得很轻。

-- 报名处那处触发是 once；考官本人（npc_qixuan_kaoguan）坐到次日领人上山为止
--（hidden_flag=ch01.zhangtie_met），挂在他身上的这个脚本没有 once：登记过了再找他，
-- 从前会把进镇、排队、登记整段重演。下面那句「明日天不亮在镇口等齐」只在报名当天说得通，
-- 所以他不能坐到考完：从前挂到 ch01.climb_done，次日走回镇上还能听到这句。
if flag.get("ch01.baoming_done") == 1 then
    talk("qixuan_kaoguan", "ch01.zhenshang.kg_done")
    return
end

talk("", "ch01.zhenshang.arrive")
talk("han_sanshu", "ch01.zhenshang.chunxiang")
talk("", "ch01.zhenshang.crowd")

talk("qixuan_kaoguan", "ch01.zhenshang.kg_register")
talk("qixuan_kaoguan", "ch01.zhenshang.kg_age")
talk("", "ch01.zhenshang.hanli_answer")
talk("qixuan_kaoguan", "ch01.zhenshang.kg_done")

talk("han_sanshu", "ch01.zhenshang.sanshu_dating")
talk("", "ch01.zhenshang.hint_talk")

flag.set("ch01.baoming_done")
