-- @hook ch06_huangfenggu trigger_wufeng interact once
-- 第六章节点 16b「传功阁」。三场必打之三：b06_wufeng_qiecuo。挂在 ch06_huangfenggu 传功阁前的案 (38,9)(39,9)
-- （mode=interact，once=true，guard_flag=ch06.chuwudai，set_flag=ch06.wufeng）。去请教、应他一句「切磋领悟」的是他。
--
-- 传功阁后半截伸进山腹；右手第三间；吴风，三十来岁，青衣，初级功法在小一辈里排得进前十。
-- **改编**（施工图 8.3）：钩子是他自己那句「可以和韩师弟共同切磋领悟」——当场切磋一回。
-- can_escape 假、defeat_is_fatal 假。赢 / 输只改 ch06.wufeng = 1 / 2 与吴风的一句话，没有别的后果。
-- 之后王师叔带他认几处地方、几位执事；可以搬去玄坤山要十层（路径行动·打探王师叔再讲一遍）。

talk("", "ch06.wufeng.hall")
talk("wang_shishu", "ch06.wufeng.intro")
talk("", "ch06.wufeng.bow")
talk("wu_feng", "ch06.wufeng.modest")
talk("", "ch06.wufeng.now")
talk("wu_feng", "ch06.wufeng.ok")

local won = battle("b06_wufeng_qiecuo")

local wufeng = 2
if won then
    wufeng = 1
    talk("wu_feng", "ch06.wufeng.won")
else
    talk("wu_feng", "ch06.wufeng.lost")
end
talk("", "ch06.wufeng.after")
talk("", "ch06.wufeng.tour")

flag.set("ch06.wufeng", wufeng)
