-- @hook ch12_binghuo trigger_shidian enter once
-- Facts and participants: docs/ch12-design.md 13.1 n23. 3.2：领婆罗珠、青冥针、皇鳞甲各 1；四门合流（皆通宝光阁）；
-- 寒冰珠转赠只改文案、不 give；置 ch12.shidian；teleport("ch12_baoguang")。
-- kScriptTransfers 三元行：flag.set 与 teleport 写在顶格。四门合流、选哪扇都一样：取消也照走（不 return），免得重踩时重复领物。
talk("", "ch12.shidian.hall")
talk("", "ch12.shidian.curse")
talk("xinggong_zhifa_ci", "ch12.shidian.elder")
talk("", "ch12.shidian.silent")
talk("", "ch12.shidian.last")
talk("hanli", "ch12.shidian.return")
talk("man_huzi", "ch12.shidian.keep")
talk("xinggong_zhifa_leng", "ch12.shidian.rule")
give("story_poluo_zhu", 1)
give("story_qingming_zhen", 1)
give("story_huanglin_jia", 1)
talk("", "ch12.shidian.lend")
choice({"ch12.shidian.opt_east", "ch12.shidian.opt_south", "ch12.shidian.opt_west", "ch12.shidian.opt_north"})
talk("", "ch12.shidian.door")
flag.set("ch12.shidian")
teleport("ch12_baoguang")
