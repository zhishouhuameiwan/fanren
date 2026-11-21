-- 同道录 2a·路 N：陈巧倩第 2 档叙话（外传；红颜线）。
-- 施工图 docs/ch07-tongdaolu.md 3.5；全文与写法依据见 docs/ch07-tongdaolu-n.md 第 4 节。
-- 由同道录页（路 U）在百药园、此档待演时调起；演完自己记账 tongdao.xuhua_mark。
-- 文案在 data/text/tongdao.json（不进 ch07*.json 的登记表，施工记录第 6 节）。

talk("", "tongdao.chen_qiaoqian.t2.1")
talk("chen_shimei", "tongdao.chen_qiaoqian.t2.2", "normal")
talk("chen_shimei", "tongdao.chen_qiaoqian.t2.3", "hurt")
talk("hanli", "tongdao.chen_qiaoqian.t2.4")
talk("chen_shimei", "tongdao.chen_qiaoqian.t2.5", "angry")
talk("", "tongdao.chen_qiaoqian.t2.6")
talk("chen_shimei", "tongdao.chen_qiaoqian.t2.7", "smile")
talk("", "tongdao.chen_qiaoqian.t2.8")

local ok = tongdao.xuhua_mark("chen_qiaoqian", 2)
