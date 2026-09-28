-- @hook ch05_kezhan trigger_lian_jianfu interact once
-- @hook ch05_dubashanzhuang trigger_lian_jianfu_lin interact once
-- 第五章支线 Z2「那道剑符」。**同一个脚本挂两处**（施工图 3.1 Z2 / Z2′）：
--   ch05_kezhan 后院的木桩 (27,5)，与 ch05_dubashanzhuang 林子里的老树桩 (10,22)
--   （都是 mode=interact，once=true，guard_flag=ch05.jianfu_qiu，set_flag=ch05.lian_jianfu）。
-- 林子里那一处是给「没练就来了、在墙头看清了下不了手、悄悄退回来」的人留的路（施工图 8.4；
-- 第 5 章复验 MEDIUM-B 方案 1：没练成剑符就不照面，12d 不开战）。练符是他。
--
-- **改编的桥**：ch92 他催不动剑符；ch126 他祭起剑符一下取了首级；中间原著没写。
-- 练两天（advance_days(2)）。台词用「驱物术」（ch75）——法力从念头上走，念头到哪儿，东西到哪儿。
-- 第 4 章 ch04.jianfu_dedao：1 摊开试过（小剑抬起一指高就落回去）/ 2 收着没敢试——开头一句分两种。
--
-- ch05.cisha 已置位还来练：只说一句，在置旗标之前 return。剑符是取首级的唯一办法（校对 MEDIUM-1），
-- 能置 ch05.cisha 的人必已练成，正常流程走不到这一句——留着当防线；这一条支线也就不会过期。
-- 剑符（talisman_jianfu）是第 4 章给的、不可交易，正常流程里一定在；真不在就说一句、不练。

if flag.get("ch05.cisha") ~= 0 then
    talk("", "ch05.lianfu.late")
    return
end

if item.count("talisman_jianfu") == 0 then
    talk("", "ch05.lianfu.nofu")
    return
end

talk("", "ch05.lianfu.start")
if flag.get("ch04.jianfu_dedao") == 1 then
    talk("", "ch05.lianfu.tried")
else
    talk("", "ch05.lianfu.kept")
end
talk("", "ch05.lianfu.think")

advance_days(2)

talk("", "ch05.lianfu.work")
talk("", "ch05.lianfu.done")

flag.set("ch05.lian_jianfu")
