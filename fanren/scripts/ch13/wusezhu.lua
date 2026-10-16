-- @hook ch13_wuzhong trigger_biguanshi interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 22。3.2 n22：take story_wuse_yuanzhu 1（看返回值，false 换一句照走）；
-- 金丝蚕余数 > 0 才 take 余数（take(id, 0) 会被引擎当成扣 1，契约第 4 节）；advance_days(15 ＋ 360)；置 ch13.wusezhu。
-- 资质不改数值（E 表 38）；卵与新虫只写「有斑」「样子变了」，颜色花纹不写（13.1 段 22）。
talk("", "ch13.wusezhu.tools")
advance_days(15)
talk("", "ch13.wusezhu.eggs")
if take("story_wuse_yuanzhu", 1) then
    talk("", "ch13.wusezhu.refine")
else
    talk("", "ch13.wusezhu.refine_fail")
end
advance_days(360)
local n = item.count("story_jinsi_can")
if n > 0 then
    talk("", "ch13.wusezhu.silk")
    if take("story_jinsi_can", n) then
        talk("", "ch13.wusezhu.silk_gone")
    else
        talk("", "ch13.wusezhu.silk_fail")
    end
end
talk("", "ch13.wusezhu.hatch")
talk("", "ch13.wusezhu.guard")
flag.set("ch13.wusezhu")
