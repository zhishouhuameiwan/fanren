-- @hook ch07_huanxingshan trigger_xiashan enter once
-- 第七章节点 32a「下山」。挂在 ch07_huanxingshan 地面破洞外一格 (39,35)
-- （mode=enter，once=true，guard_flag=ch07.nangong，set_flag=ch07.xiashan）。往山下奔。
--
-- 框架件（施工图 13.1 第 32a 段）：他在林梢飞奔下山；一个灵兽山弟子伏击，他一闪就取了那人首级（旁白一行，不做战斗，8.3）；
-- 第五日下午赶到出口。**advance_days(1)**：施工图 3.3 的日历表在节点 28 之后没有再拨日子，可出禁地是第五日下午（原著 ch209），
-- 这一天拨在这里（记第 18 节）。横幅照剧情写（「禁地　第五日」）。
-- 末尾 teleport ch07_jindi_wai 出口通道底 (23,34)（genmaps_ch07.py 的 JINDI_WAI_CHUKOU）；登 kScriptTransfers。

advance_days(1)

talk("", "ch07.xiashan.banner")
talk("", "ch07.xiashan.run")
talk("", "ch07.xiashan.ambush")
talk("", "ch07.xiashan.bag")
talk("", "ch07.xiashan.exit")

flag.set("ch07.xiashan")
teleport("ch07_jindi_wai", 23, 34)
