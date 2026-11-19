-- @hook ch07_zhaoze_shenchu trigger_chahua_jun enter once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.6）插话：东北岔道里一片发光的菌。挂在 沼泽深处东北岔道口 (35,9)
-- （mode=enter，once=true，set_flag=ch07.chahua_jun，require_party=nangong_wan——她不在队里就不触发，契约 3.8）。
-- 摆在岔路上，不在必经的路上（4.6）；一两句话，末尾默契 +2（脚本自己保证只演一次：once ＋ 完成旗标）。

talk("", "ch07.chahua_jun.see")
talk("baiyi_shaonv", "ch07.chahua_jun.say")
talk("", "ch07.chahua_jun.after")
bond.add("nangong_wan", 2)

flag.set("ch07.chahua_jun")
