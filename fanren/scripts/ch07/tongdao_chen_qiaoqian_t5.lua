-- 同道录 2a·路 N：陈巧倩第 5 档叙话（外传；红颜线）。
-- 施工图 docs/ch07-tongdaolu.md 3.5；全文与写法依据见 docs/ch07-tongdaolu-n.md 第 4 节。
-- 由同道录页（路 U）在百药园、此档待演时调起；演完自己记账 tongdao.xuhua_mark。
-- 文案在 data/text/tongdao.json（不进 ch07*.json 的登记表，施工记录第 6 节）。
-- 事件图 chen_qiaoqian_hongyan（data/visual/cg.json）：两人隔一步远、衣着整齐、无肢体接触。

talk("", "tongdao.chen_qiaoqian.t5.1")
talk("chen_shimei", "tongdao.chen_qiaoqian.t5.2", "normal")
show_cg("chen_qiaoqian_hongyan")
talk("chen_shimei", "tongdao.chen_qiaoqian.t5.3", "hurt")
talk("", "tongdao.chen_qiaoqian.t5.4")
talk("chen_shimei", "tongdao.chen_qiaoqian.t5.5", "smile")
hide_cg()
talk("chen_shimei", "tongdao.chen_qiaoqian.t5.6", "angry")
talk("", "tongdao.chen_qiaoqian.t5.7")

local ok = tongdao.xuhua_mark("chen_qiaoqian", 5)
