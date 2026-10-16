-- @hook ch13_nanming npc_cang_yunlong npc
-- Facts and participants: docs/ch13-design.md 13.1 段 4。3.2 n04：已置 → 一句 return；前置 ch13.gongdao 开头判；
-- item.count("story_miaoyin_yaopai") >= 1 择「出示腰牌」版，否则「报名号」版（只读不扣，不提画像，16.1 第 41 条）；
-- choice「应下 / 不置可否」，取消在置旗前 return；置 ch13.panzha。
-- 判据③：他飞走之后苍、刘二人的私话 0 处；只写他自己不易容的推断（ch503 第 24–31 段）。
-- 壮汉（liu_hufa，描述名牌）当面一句「少主是圣祖唯一传人」的意思，0 处出名（16.1 第 57 条）。
if flag.get("ch13.panzha") ~= 0 then talk("", "ch13.panzha.gone"); return end
if flag.get("ch13.gongdao") == 0 then talk("", "ch13.panzha.early"); return end
talk("", "ch13.panzha.rule")
talk("", "ch13.panzha.xinming")
talk("", "ch13.panzha.others")
if item.count("story_miaoyin_yaopai") >= 1 then
    talk("hanli", "ch13.panzha.token")
else
    talk("hanli", "ch13.panzha.name")
end
talk("cang_yunlong", "ch13.panzha.elders")
talk("hanli", "ch13.panzha.han")
talk("cang_yunlong", "ch13.panzha.message")
talk("liu_hufa", "ch13.panzha.shaozhu")
local pick = choice({"ch13.panzha.opt_promise", "ch13.panzha.opt_vague"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch13.panzha.promise")
else
    talk("", "ch13.panzha.vague")
end
talk("", "ch13.panzha.sweep")
talk("", "ch13.panzha.face")
flag.set("ch13.panzha")
