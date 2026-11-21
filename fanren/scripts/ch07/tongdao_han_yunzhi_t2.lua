-- 同道录 2a·路 N：菡云芝第 2 档叙话（外传；莫逆线）。
-- 施工图 docs/ch07-tongdaolu.md 3.5；全文与写法依据见 docs/ch07-tongdaolu-n.md 第 4 节。
-- 由同道录页（路 U）在百药园、此档待演时调起；演完自己记账 tongdao.xuhua_mark。
-- 文案在 data/text/tongdao.json（不进 ch07*.json 的登记表，施工记录第 6 节）。

talk("", "tongdao.han_yunzhi.t2.1")
talk("han_yunzhi", "tongdao.han_yunzhi.t2.2", "normal")
talk("hanli", "tongdao.han_yunzhi.t2.3")
talk("han_yunzhi", "tongdao.han_yunzhi.t2.4", "hurt")
talk("hanli", "tongdao.han_yunzhi.t2.5")
talk("han_yunzhi", "tongdao.han_yunzhi.t2.6", "angry")
talk("han_yunzhi", "tongdao.han_yunzhi.t2.7", "smile")
talk("", "tongdao.han_yunzhi.t2.8")

local ok = tongdao.xuhua_mark("han_yunzhi", 2)
-- 东西只在第一次演完时给（再演一遍 xuhua_mark 答 already，不再给）。
-- 施工图 3.5 原写「送驭兽牌」：驭兽牌是 2b 的东西，2a 改送一样现成的符；2b 做驭兽牌时再接进这一段。
if ok then
    give("talisman_huoqiu_fu", 3)
end
