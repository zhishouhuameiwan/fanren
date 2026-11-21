-- 同道录 2a·路 N：南宫婉第 2 档叙话（外传；通用线（第 7 章封在四档））。
-- 施工图 docs/ch07-tongdaolu.md 3.5；全文与写法依据见 docs/ch07-tongdaolu-n.md 第 4 节。
-- 由同道录页（路 U）在百药园、此档待演时调起；演完自己记账 tongdao.xuhua_mark。
-- 文案在 data/text/tongdao.json（不进 ch07*.json 的登记表，施工记录第 6 节）。

talk("", "tongdao.nangong_wan.t2.1")
talk("nangong_wan_ch07", "tongdao.nangong_wan.t2.2", "angry")
talk("hanli", "tongdao.nangong_wan.t2.3")
talk("nangong_wan_ch07", "tongdao.nangong_wan.t2.4", "smile")
talk("nangong_wan_ch07", "tongdao.nangong_wan.t2.5", "hurt")
talk("hanli", "tongdao.nangong_wan.t2.6")
talk("nangong_wan_ch07", "tongdao.nangong_wan.t2.7", "normal")
talk("", "tongdao.nangong_wan.t2.8")

local ok = tongdao.xuhua_mark("nangong_wan", 2)
