-- @hook ch06_tainan_cun trigger_chudu enter once
-- 第六章节点 1a「太南山脚：除毒、打听、碎银、配丹」。挂在 ch06_tainan_cun 林中小径出生点东边一格
-- (2,20)(2,21)（mode=enter，once=true，set_flag=ch06.chudu）。出生点就在树林里：南城东门出来一睁眼，
-- 已是除毒的最后一天——毒是自己散尽的，所以是踏入型（docs/ch06-design.md 3.1）。
--
-- 不重演第 5 章：huanyu.lua 已演到「离城」，这里只追叙一句谢绝挽留就走、一路解毒半月（ch126）。
-- 暖阳宝玉提一句，本章不收回（他贴身藏好）。
--
-- 两侧同账（施工图第 2 节末、第 9 节；用户拍板 16.1 第 2、3 条）：
--   · 碎银：借住的村民家，走之前整包留下——改编（原著没写他怎么处置银子）。take 全额，看返回值。
--   · 养精丹：半月里就地配丹，**补足到 15 瓶**。第 5 章两侧交来的是 12 / 0，本章以物易物要 14 瓶
--     （5b 五瓶、8b 两瓶加七瓶），不补足次侧走不到金竺笔。次侧的料不够真炼出 15 瓶，所以是「补足」
--     不是「炼出」：give 的是 15 − 现有 的差额，已有 ≥ 15 就不给。**本章 give 养精丹只此一处。**
--     包里的黄精紫参（不论年份）一并用掉，掌天瓶三滴全倒。
-- 日历：+15（施工图 3.3）。

advance_days(15)

talk("", "ch06.chudu.jade")
talk("", "ch06.chudu.halfmonth")
talk("", "ch06.chudu.back")
talk("", "ch06.chudu.keep")
talk("", "ch06.chudu.ask1")
talk("", "ch06.chudu.ask2")
talk("", "ch06.chudu.ask3")

-- 碎银：整包留给借住的那户人家。
local silver = item.count("material_lingshi")
if silver > 0 and take("material_lingshi", silver) then
    talk("", "ch06.chudu.silver")
else
    talk("", "ch06.chudu.nosilver")
end

-- 配丹：黄精紫参用掉，瓶里三滴倒尽，养精丹补足到 15 瓶。
local huangjing = item.count("herb_huangjing_cao")
if huangjing > 0 then
    take("herb_huangjing_cao", huangjing)
end
local zishen = item.count("herb_zishen_cao")
if zishen > 0 then
    take("herb_zishen_cao", zishen)
end
bottle.spend(3)

local have = item.count("pill_yangjing_dan")
if have < 15 then
    give("pill_yangjing_dan", 15 - have)
    talk("", "ch06.chudu.pill")
else
    talk("", "ch06.chudu.pill_enough")
end
talk("", "ch06.chudu.pill_why")
talk("", "ch06.chudu.go")

flag.set("ch06.chudu")
