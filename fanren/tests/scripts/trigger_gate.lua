-- 夹具：本项目里最常见的那一种闸门脚本 —— 先查前置，不满足就说一句话再 return。
--
-- 缺陷 A 的全部要害就在这条提前 return 的路径上：引擎一度在 startEvent 之前就把
-- once 触发器的记号打上了，于是踩一次就永久烧掉，玩家后来满足条件再回来也没用。
-- 现在兑现 once 的是末尾那句 flag.set，提前 return 根本走不到，触发器留得住。
--
-- 写法与正式脚本一致（完成旗标置在末尾），这条测试才代表真实内容的形状。
--
-- 开头那株药是测试的计数器：背包里有几株，这个脚本就被起过几次。防重入要证明的
-- 「一次也没多起」，需要一个看得见的计数，而不是只看旗标（旗标只记最终状态，
-- 起两次和起一次是一样的）。

give("herb_qingfeng_cao", 1)

if flag.get("t.gate_open") ~= 1 then
    talk("", "t.gate.deny")
    return
end

talk("", "t.gate.pass")
flag.set("t.gate_done")
