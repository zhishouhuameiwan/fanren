-- @hook ch07_zhaoze_shenchu trigger_chahua_kumu enter once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.6）插话：西南岔道里一截一截的枯木。挂在 沼泽深处西南岔道口 (8,26)
-- （mode=enter，once=true，set_flag=ch07.chahua_kumu，require_party=nangong_wan——她不在队里就不触发，契约 3.8）。
-- 摆在岔路上，不在必经的路上（4.6）；一两句话，末尾默契 +2（脚本自己保证只演一次：once ＋ 完成旗标）。

-- 地基包路 D（docs/ch07-foundation.md 4.4）：插话改成她头顶的气泡，不停下脚步、不收输入；她不在画面上时
-- 引擎退回成普通对白（话不丢）。原来前后两句旁白删去，景物交给地图本身，她那一句把看见的东西带出来。
bubble("baiyi_shaonv", "ch07.chahua_kumu.say", 4)
bond.add("nangong_wan", 2)

flag.set("ch07.chahua_kumu")
