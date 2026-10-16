-- @hook ch13_waihai npc_li_xun npc
-- Facts and participants: docs/ch13-design.md 13.1 段 15。3.2 n15：开头 ch13.mingzhu 未置 → 一句 return（不置旗、不给物，16.1 第 47 条）；
-- 首谈：give story_guyu_yujian 1；置 ch13.cunjin。再谈：shop("ch13_cunjin_ge")。
-- 判据③：他走后屋里另一人的事 0 处。「离寻」「古玉」「伴妖草」自本段起；化名「曲」到本段为止（12.2）。
if flag.get("ch13.mingzhu") == 0 then talk("li_xun", "ch13.cunjin.early"); return end
if flag.get("ch13.cunjin") ~= 0 then
    talk("li_xun", "ch13.cunjin.again")
    shop("ch13_cunjin_ge")
    return
end
talk("", "ch13.cunjin.street")
talk("", "ch13.cunjin.room")
talk("", "ch13.cunjin.today")
talk("hanli", "ch13.cunjin.ask")
talk("", "ch13.cunjin.guyu")
talk("", "ch13.cunjin.truth")
talk("", "ch13.cunjin.swap")
give("story_guyu_yujian", 1)
talk("li_xun", "ch13.cunjin.manao")
talk("li_xun", "ch13.cunjin.team")
talk("hanli", "ch13.cunjin.decline")
flag.set("ch13.cunjin")
