-- @hook ch07_zhaoze_shenchu trigger_chahua_kumu enter once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.6）插话：西南岔道里一截一截的枯木。挂在 沼泽深处西南岔道口 (8,26)
-- （mode=enter，once=true，set_flag=ch07.chahua_kumu，require_party=nangong_wan——她不在队里就不触发，契约 3.8）。
-- 摆在岔路上，不在必经的路上（4.6）；一两句话，末尾默契 +2（脚本自己保证只演一次：once ＋ 完成旗标）。

talk("", "ch07.chahua_kumu.see")
talk("baiyi_shaonv", "ch07.chahua_kumu.say")
talk("", "ch07.chahua_kumu.after")
bond.add("nangong_wan", 2)

flag.set("ch07.chahua_kumu")
