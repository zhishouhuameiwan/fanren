-- @hook ch12_hanlitai trigger_qingshi interact once
-- Facts and participants: docs/ch12-design.md 13.1 n28. 3.2：领内殿卷轴；置 ch12.juanzhou。卷轴收起不看（拍板 34）。
talk("", "ch12.juanzhou.wait")
talk("", "ch12.juanzhou.shield")
talk("", "ch12.juanzhou.knock")
talk("", "ch12.juanzhou.cut")
give("story_neidian_juanzhou", 1)
talk("jiyin_zushi", "ch12.juanzhou.call")
talk("", "ch12.juanzhou.keep")
flag.set("ch12.juanzhou")
