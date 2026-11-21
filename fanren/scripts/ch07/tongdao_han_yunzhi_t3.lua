-- 同道录 2a·路 N：菡云芝第 3 档叙话（外传；莫逆线）。
-- 施工图 docs/ch07-tongdaolu.md 3.5；全文与写法依据见 docs/ch07-tongdaolu-n.md 第 4 节。
-- 由同道录页（路 U）在百药园、此档待演时调起；演完自己记账 tongdao.xuhua_mark。
-- 文案在 data/text/tongdao.json（不进 ch07*.json 的登记表，施工记录第 6 节）。

talk("", "tongdao.han_yunzhi.t3.1")
talk("han_yunzhi", "tongdao.han_yunzhi.t3.2", "smile")
talk("han_yunzhi", "tongdao.han_yunzhi.t3.3", "hurt")
talk("han_yunzhi", "tongdao.han_yunzhi.t3.4", "normal")
talk("hanli", "tongdao.han_yunzhi.t3.5")
talk("han_yunzhi", "tongdao.han_yunzhi.t3.6", "angry")
talk("hanli", "tongdao.han_yunzhi.t3.7")
talk("", "tongdao.han_yunzhi.t3.8")

local ok = tongdao.xuhua_mark("han_yunzhi", 3)
