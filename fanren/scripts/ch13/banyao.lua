-- @hook ch13_waihai trigger_dongkou interact once
-- Facts and participants: docs/ch13-design.md 13.1 段 34。3.2 n34：give story_banyao_cao 1；末尾置 ch13.done。
-- 不 ending()、不 teleport（E 表 28；第 14 章 patch 本脚本末尾）；本段日数 0（3.3）。
-- 章末停在人声：ch13.banyao.* 的最后一条就是身后那个男子的声音——不写他的样子、名字、境界，不写他回身之后的任何事（13 第 14 条）。
talk("", "ch13.banyao.dawn")
talk("", "ch13.banyao.cave")
talk("", "ch13.banyao.pool")
talk("", "ch13.banyao.grass")
give("story_banyao_cao", 1)
talk("", "ch13.banyao.box")
talk("", "ch13.banyao.behind")
talk("", "ch13.banyao.voice")
flag.set("ch13.done")
