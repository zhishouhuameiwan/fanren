-- @hook ch07_dixia_shiku trigger_chahua_shishi enter once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.6）插话：残破的石室。挂在 地底石窟西北残破石室的甬道口 (5,13)(6,13)
-- （mode=enter，once=true，set_flag=ch07.chahua_shishi，require_party=nangong_wan——她不在队里就不触发，契约 3.8）。
-- 摆在岔路上，不在必经的路上（4.6）；一两句话，末尾默契 +2（脚本自己保证只演一次：once ＋ 完成旗标）。

talk("", "ch07.chahua_shishi.see")
talk("baiyi_shaonv", "ch07.chahua_shishi.say")
talk("", "ch07.chahua_shishi.after")
bond.add("nangong_wan", 2)

flag.set("ch07.chahua_shishi")
