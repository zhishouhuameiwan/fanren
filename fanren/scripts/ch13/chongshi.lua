-- @hook ch13_wuzhong trigger_chongshi interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 18。3.2 n18：give story_wuse_yuanzhu 1；advance_days(5)；置 ch13.chongshi。
-- item.count("story_jinsi_can") > 0 多一段（不扣）。补天丹只是他的猜想；「五色圆珠」「幻焰蛾」自本段起（12.2）。
talk("", "ch13.chongshi.picture")
talk("", "ch13.chongshi.guess")
give("story_wuse_yuanzhu", 1)
talk("", "ch13.chongshi.box")
talk("", "ch13.chongshi.rooms")
if item.count("story_jinsi_can") > 0 then
    talk("", "ch13.chongshi.moth")
    talk("", "ch13.chongshi.feed")
end
talk("", "ch13.chongshi.ginseng")
talk("", "ch13.chongshi.wood")
advance_days(5)
flag.set("ch13.chongshi")
