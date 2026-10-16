-- @hook ch13_wuzhong trigger_shoushou interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 25。3.2 n25：ch13.lieyao < 2 → 一句 return（不置旗，once 不兑现）；
-- advance_days(2415)（3.3「七年后」）；give material_yaodan_liuji 6、material_yaodan_wuji 10、story_liuqi_cailiao 1；置 ch13.chongmo。
-- 判据③：三人现身前的私语、暗场人物与假虫魔杀人那一幕 0 处；只写三人当面说的与他自己的猜测。
-- 闵、宣两人的姓他没听到过（描述名牌，16.1 第 44 条）；「琉璃兽」「虫魔」「碧云门」「四法上人」「三色噬金虫」自本段起（12.2）。
if flag.get("ch13.lieyao") < 2 then talk("", "ch13.chongmo.early"); return end
advance_days(2415)
talk("", "ch13.chongmo.years")
give("material_yaodan_liuji", 6)
give("material_yaodan_wuji", 10)
give("story_liuqi_cailiao", 1)
talk("", "ch13.chongmo.liuli")
talk("", "ch13.chongmo.spears")
talk("qiu_laozhe", "ch13.chongmo.beg")
talk("hanli", "ch13.chongmo.what")
talk("min_xiushi", "ch13.chongmo.rumor")
talk("xuan_xiushi", "ch13.chongmo.fake")
talk("", "ch13.chongmo.release")
talk("qiu_laozhe", "ch13.chongmo.abyss")
talk("", "ch13.chongmo.guess")
talk("", "ch13.chongmo.home")
flag.set("ch13.chongmo")
