-- @hook ch06_huangfenggu trigger_ye_maidan interact once
-- 第六章节点 14「迎宾楼：叶师叔买丹」。挂在 ch06_huangfenggu 迎宾楼房里的床 (4,23)(5,23)
-- （mode=interact，once=true，guard_flag=ch06.linggen，set_flag=ch06.rangdan）。
-- 叶师叔是来的，但他在床上躺着出神、听见脚步起身迎门——按的是那张床（施工图 3.1）。
--
-- 王师叔带叶姓老者进屋；「叫我叶师叔即可」；掌门同意收他、筑基丹也备好了一粒；王师叔回避。
-- 叶师叔开门见山要买筑基丹；开价：灵石、灵符、法器、精进法力的丹药（不说「中阶灵石」，施工图第 7 节）。
-- 他问替谁讨要——侄孙。「凭师侄的资质，筑基的指望渺茫得很」在这里说（节点 13 挪过来的）。
-- 二选一（施工图 3.2 节点 14）：他都答应让丹，差在要什么。取消按 1 算。
--   1 要杂务任选（原著）——叶师叔拍胸脯：分派杂务的管事就是他；
--   2 只要东西，多要些——叶师叔当场多许二十块灵石，杂务一条不提；节点 17 的悔诺按它扣得更狠。
-- 末句「明面上的交易可以说，暗地里允诺的别提」。

talk("", "ch06.maidan.steps")
talk("wang_shishu", "ch06.maidan.intro")
talk("ye_shishu", "ch06.maidan.shishu")
talk("wang_shishu", "ch06.maidan.good")
talk("", "ch06.maidan.alone")
talk("ye_shishu", "ch06.maidan.buy")
talk("ye_shishu", "ch06.maidan.offer")
talk("", "ch06.maidan.think")
talk("", "ch06.maidan.who")
talk("ye_shishu", "ch06.maidan.nephew")
talk("ye_shishu", "ch06.maidan.hopeless")
talk("", "ch06.maidan.weigh")

local pick = choice{
    "ch06.maidan.opt_zawu",
    "ch06.maidan.opt_goods",
}

local rangdan = 1
if pick == 2 then
    rangdan = 2
    talk("", "ch06.maidan.goods")
    talk("ye_shishu", "ch06.maidan.twenty")
else
    talk("ye_shishu", "ch06.maidan.zawu1")
    talk("", "ch06.maidan.zawu2")
end
talk("", "ch06.maidan.yield")
talk("ye_shishu", "ch06.maidan.quiet")

flag.set("ch06.rangdan", rangdan)
