-- @hook ch07_dixia_shiku trigger_wenming enter once
-- 第七章节点 31 末「南宫婉」，伙伴改版样板（docs/ch07-companions-design.md 4.5）「问名与分别」。挂在 ch07_dixia_shiku
-- 岩层最薄处正下方那一格 (21,3)（mode=enter，once=true，guard_flag=ch07.poding，set_flag=ch07.nangong）。
-- 破顶之后 poding.lua 把人送回下一格，他再往上一迈就从破口爬出去。
--
-- 框架件：出到地面；分别时他问她的名字：南宫婉（「南宫」从 ch07.jiaoshi.name* 起才许出现，12.2）；他清楚两人不会再有交集。
-- 面纱（用户 2026-11-18 定）：问名之前戴面纱是改编（原著禁地里她露着脸，ch182、ch204；「常年戴面纱」说的是平日在外，ch210），
-- 问名这一拍她摘下面纱，名签与头像从这一句起换成露脸的 nangong_wan_ch07。
-- 样板：问名默契 +8（4.6）；名签从这一句起换成「南宫婉」（说话人 nangong_wan_ch07——问名之后到第 7 章完的那一份角色表，
-- 伙伴数据 roles 第二段）；她离队（party.remove）、进名册（met_flag 就是 ch07.nangong）。
-- 末尾 teleport ch07_huanxingshan 地面破洞旁 (38,35)（genmaps_ch07.py 的 HXS_POKONG）；登 kScriptTransfers。

talk("", "ch07.wenming.climb")
talk("", "ch07.wenming.light")
talk("", "ch07.jiaoshi.name_ask")
talk("", "ch07.wenming.veil")
talk("nangong_wan_ch07", "ch07.jiaoshi.name_say")
bond.add("nangong_wan", 8)
talk("", "ch07.jiaoshi.name_after")
party.remove("baiyi_shaonv")
talk("", "ch07.wenming.part")

flag.set("ch07.nangong")
teleport("ch07_huanxingshan", 38, 35)
