-- @hook ch12_tushan trigger_dengdao enter once
-- Facts and participants: docs/ch12-design.md 13.1 n01. 3.2：take 珍珠看返回值（0 也照走）；advance_days(62)；二选一只换一句。
-- 抉择放在一切状态改动之前：取消即 return，重踩时不会重复拨日、重复扣珠。
talk("", "ch12.dengdao.invite")
talk("", "ch12.dengdao.voyage")
talk("", "ch12.dengdao.island")
talk("", "ch12.dengdao.house")
talk("hu_yue", "ch12.dengdao.greet")
talk("hanli", "ch12.dengdao.shixiong")
talk("shi_die", "ch12.dengdao.condition")
talk("jin_qing", "ch12.dengdao.promise")
local pick = choice({"ch12.dengdao.opt_nod", "ch12.dengdao.opt_warn"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch12.dengdao.nod")
else
    talk("hanli", "ch12.dengdao.warn")
end
advance_days(62)
talk("jin_qing", "ch12.dengdao.pearl")
if take("story_bai_zhenzhu", 1) then
    talk("", "ch12.dengdao.pearl_given")
else
    talk("", "ch12.dengdao.pearl_none")
end
talk("", "ch12.dengdao.night")
flag.set("ch12.dengdao")
