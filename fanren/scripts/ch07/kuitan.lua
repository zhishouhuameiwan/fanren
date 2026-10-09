-- @hook ch07_huanxingshan trigger_kuitan enter once
-- 第七章节点 26a「紫猴花洞：拐角」。挂在 ch07_huanxingshan 竖洞道的拐角 (8,18)(9,18)
-- （mode=enter，once=true，guard_flag=ch07.yueyang，set_flag=ch07.kuitan）。拐角处看见巨蜈蚣。
--
-- 框架件（施工图 13.1 第 26 段 ①–③）：他的算计在这里说出来（全章最重要的一段内心话，五句以内）——别人抢熟的，他专找刚被采过、
-- 只剩幼苗的地方；幼苗离了地还能活一两年，瓶子催几轮就够；主药四五百年即可入药（**不提嗅灵兽**，节点 32b 兑现）；
-- 资料里一处出紫猴花的洞，百余年前被采过；洞里一只巨蜈蚣守着几株紫猴花幼苗。目标链第 32 步的 R-3 提示在 ch07_objectives.json。

talk("", "ch07.kuitan.plan1")
talk("", "ch07.kuitan.plan2")
talk("", "ch07.kuitan.plan3")
talk("", "ch07.kuitan.cave")
talk("", "ch07.kuitan.peek")
talk("", "ch07.kuitan.bug")
talk("", "ch07.kuitan.cost")

flag.set("ch07.kuitan")
