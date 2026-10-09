-- @hook ch06_baiyaoyuan trigger_ma_songyao enter once
-- 第七章节点 17「马师伯的两瓶丹药」。挂在 ch06_baiyaoyuan 园门内一格 (8,11)(8,12)（与第 6 章 trigger_jinzhi 不同格）
-- （mode=enter，once=true，guard_flag=ch07.baoming，set_flag=ch07.ma_songyao）。马师伯在园门等他。
--
-- 框架件（施工图 13.1 第 17 段）：他来卸下药园差事；马师伯看他像看一个要去送死的人；临走丢下两瓶：一瓶内服、一瓶外敷。
-- 支线 Z1 结算（施工图第 6 节）：ch07.baigong 置过 → 养精丹 5、金疮药 5，多一句「这一年的药一株没少」；没做 → 各 3。
-- 置 ch07.ma_songyao：Z1 从此过期；议事大殿前李师祖从这时起站着（节点 18）。
-- advance_days(20)：到出发前三天（施工图 3.3）。

talk("", "ch07.ma_songyao.gate")
talk("", "ch07.ma_songyao.quit")
talk("", "ch07.ma_songyao.look")

local count = 3
if flag.get("ch07.baigong") ~= 0 then
    count = 5
end
talk("ma_shibo", "ch07.ma_songyao.bottles")
give("pill_yangjing_dan", count)
give("pill_jinchuang_yao", count)
if count == 5 then
    talk("ma_shibo", "ch07.ma_songyao.baigong")
end
talk("", "ch07.ma_songyao.gone")

advance_days(20)

talk("", "ch07.ma_songyao.days")

flag.set("ch07.ma_songyao")
