-- @hook ch07_zhaoze_shenchu trigger_huolingshi enter once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.3、4.6）「替她找来火灵石」。挂在 ch07_zhaoze_shenchu 东口岩缝里 (40,22)(40,23)
-- （mode=enter，once=true，guard_flag=ch07.kundi，set_flag=ch07.huolingshi）。岩缝那头的门（通地底石窟）要 ch07.huolingshi 才开。
--
-- 火灵石是剧情物，不是回气的机制（施工图第 4 节末段：引擎不存同伴的法力，她每一场开打都是满的）：
-- 这一步是「查背包、收走一枚、加默契、置旗标」。身上没有火灵石就只说一句，不置旗标——下回再踩上来还会演。
-- 交完提示玩家队伍页里可以送她东西（4.6）。默契 +5。
-- 原著依据：她在地下一手驱使朱雀环、一手握一枚红色火灵石吸纳灵气回法力（ch206）；矿脉长在地下是改编。

if item.count("material_huolingshi") < 1 then
    talk("baiyi_shaonv", "ch07.huolingshi.none")
    return
end

talk("", "ch07.huolingshi.give")
take("material_huolingshi", 1)
talk("baiyi_shaonv", "ch07.huolingshi.thanks")
talk("", "ch07.huolingshi.breath")
bond.add("nangong_wan", 5)

flag.set("ch07.huolingshi")
