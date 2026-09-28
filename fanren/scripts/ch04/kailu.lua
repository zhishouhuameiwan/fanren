-- @hook ch04_luorifeng trigger_kailu interact once
-- 第四章节点 3「开炉：药物买卖」。炼丹在这一节教给玩家。
-- 挂在 ch04_luorifeng 丹房里那座丹炉上 (7,9) 起两格见方的 trigger
-- （mode=interact，once=true，set_flag=ch04.kailu）。
--
-- ---------------------------------------------------------------------------
-- **它与 facility_danlu（kind=alchemy，grade=1）故意同格**，这是本节的接线。
-- ---------------------------------------------------------------------------
-- WorldScene::interact 依次问 npc → interact 触发 → facility，而 once=true 的
-- 触发器**烧掉之后就让位**（src/game/WorldScene.cpp 的注释；tools/validate.py
-- 的 trigger_verdict 因此把这一类判成 maybe，不算遮蔽）。于是同一座炉子：
--   演完这一节之前按它 → 本脚本应声，演「开炉」；
--   演完之后按它       → 触发器烧掉了，炼丹面板应声。
--
-- **所以开头这道前置必须写在 flag.set 之前，并且直接 return。**
-- 走不到末尾那一行，ch04.kailu 就没置上，once 不烧，触发器留着 ——
-- 玩家学会火弹术再回来按一次，这一节照样演得出来。这条口径是关卡侧点名交代的
-- （genmaps_ch04.py 节点 3 那一段），也是 map_spec 4.4 改动的用意。
-- 被拦住的玩家听得见自己还差什么，不会以为按键没反应。
--
-- ---------------------------------------------------------------------------
-- 二选一：怎么报价（ch04.kailu = 1 照门中的价 / 2 只收药钱）
-- ---------------------------------------------------------------------------
-- 两条都开得成炉，分的是这门买卖的形状，而且**两条各发一份不同的俸禄药材**：
--   照价（1）：钱多人少 —— 黄精草 6 株 24 年、紫参草 3 株 24 年；
--   只收药钱（2）：钱少人多，门里因用量大而多拨 —— 黄精草 10 株 22 年、紫参草 4 株 22 年。
-- 两份的年份都压过 data/recipes/alchemy/yangjing_dan.json 那道 20 年门槛
--（黄精 2 株 + 紫参 1 株，都要 20 年以上），所以两条布置都配得出养精丹。
--
-- **这一份只是头一个月的，本身远不够用**（独立校对 MEDIUM-2、复验 N-1 / 技术债 G-12）。
-- 按 Crafting.cpp 的公式：养精丹难度 26、熟练度门槛 6（先得垫清灵散）、失败材料尽毁，
-- 光这一份连火候都未必垫得够，一瓶养精丹也炼不出。
-- 病根不是数字小，是**发放次数**：`ch04.kailu.salary` 的原话是「**按月**抬到峰下」，
-- 而 6 / 3 恰好就是一个月的份，节点 3 到节点 4 中间那一整年的俸禄从前在机制上
-- 从来没有发生过，通关测试靠无限补料把这件事盖住了。
--
-- **现在分次发（2026-09-23 第 4 章二次整改·平衡路）**：这里发头一个月；
-- yufeng.lua 在那 360 天过完时发月例十二个月加年底两个月的年例（黄精 84、紫参 42，
-- 两条报价一样）。照价这一条全章共 45 炉的料，只收药钱这一条 46 炉；按设计口径
-- 期望炼出 23 瓶、坏情况（5% 分位）16 瓶，而本章要 13 瓶（攻防战最费的那一档 8 瓶
-- ＋ 少两瓶也不翻的余量 2 ＋ 节点 11 留药 3）。总账在 docs/ch04-design.md 3.3，
-- tests/Ch04SalaryTests.cpp 直接读这里与 yufeng.lua 的 give 核对它。
-- **这里的两行 give 是分支发的，必须缩进在 if 里**；那一组测试据此认它们是两条报价各一批。
--
-- 从前这一条没修，是因为补料一进背包，通关测试里那只手（吃药取「物品栏第一条能点的」）
-- 的胜负就翻。那只手已改成按物品 id 只吃养精丹，并由 Ch04HandSweep 扫过
-- 门槛 / 药数 / 背包三个方向一格不翻（docs/tech-debt.md G-11）。
--
-- 「他留了一味方子不配好」那一句（ch04.kailu.bad / why）是本节的暗桩：
-- 样样都好得没道理，早晚有人要问他师父究竟教了他什么。这与节点 8 他躲在最后一排
-- 是同一件事的两次落笔。

if flag.get("ch04.huodan_xue") == 0 then
    talk("", "ch04.kailu.gate")
    return
end

if flag.get("ch04.kailu") ~= 0 then
    talk("", "ch04.kailu.again")
    return
end

talk("", "ch04.kailu.room")
talk("", "ch04.kailu.stove")
talk("", "ch04.kailu.desk")

talk("", "ch04.kailu.first")
advance_days(2)
talk("", "ch04.kailu.second")

talk("", "ch04.kailu.salary")
talk("", "ch04.kailu.count")
talk("", "ch04.kailu.think")

local pick = choice{
    "ch04.kailu.opt_price",
    "ch04.kailu.opt_cost",
}

if pick == 2 then
    talk("", "ch04.kailu.cost1")
    talk("", "ch04.kailu.cost2")
    -- 来的人多一倍，门里按用量多拨。年份 22 年压着养精丹那道 20 年的门槛，
    -- 不是随手写的：低于它这一份俸禄就只配得出清灵散，节点 11 那条分支会瘸。
    give("herb_huangjing_cao", 10, 22)
    give("herb_zishen_cao", 4, 22)
else
    -- 照价（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按照价算：门里当面问价，一个在药铺待了四年的人不会临时改规矩。
    talk("", "ch04.kailu.price1")
    talk("", "ch04.kailu.price2")
    give("herb_huangjing_cao", 6, 24)
    give("herb_zishen_cao", 3, 24)
end

talk("", "ch04.kailu.bad")
talk("", "ch04.kailu.why")
talk("", "ch04.kailu.stock")
talk("", "ch04.kailu.end")

flag.set("ch04.kailu", pick == 2 and 2 or 1)
