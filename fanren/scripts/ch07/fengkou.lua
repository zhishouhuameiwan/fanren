-- @hook ch07_dixia_zhaoze trigger_fengkou interact once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.3）「困在地下」。挂在 ch07_dixia_zhaoze 石阶口最上面那一格的岩壁 (3,0)
-- （人站在 (3,1) 面朝上按确认；mode=interact，once=true，guard_flag=ch07.xinglai，set_flag=ch07.kundi）。
-- 蛟尸那一段演完交还操作，玩家自己走回石阶口去看那道封死的通道（内容审查 2026-11-18 第 5 条）。
--
-- 框架件：回头的路封死了（ch206 小五行须弥禁法，从里面拆不开）；她评他的银剑（ch207，约束 6）——默契 +8，
-- 演完是 22，到二档「相熟」，追击从这一路上就用得上（4.6）；两人商定出去要做的两件事：替她找火灵石回气（ch206 她握着
-- 火灵石回法力，那一块已经吸干）、找岩层最薄的地方（ch209 打穿地面）。她的法力被封是素女轮回功正在轮回（ch208、ch209），
-- 与禁地无关（禁地挡的是筑基以上的人进来，ch194）。这段「在地下找路」是改编：起点（通道被封）、终点（合力打穿）都是原著的。
-- 置 ch07.kundi：地下沼泽通沼泽深处的两道门从此开。

talk("", "ch07.kundi.passage")
talk("baiyi_shaonv", "ch07.kundi.no_way")
talk("baiyi_shaonv", "ch07.kundi.see_sword")
talk("", "ch07.kundi.hand_over")
talk("baiyi_shaonv", "ch07.jiaoshi.silver")
talk("", "ch07.kundi.take_back")
bond.add("nangong_wan", 8)
talk("baiyi_shaonv", "ch07.kundi.plan")
talk("baiyi_shaonv", "ch07.kundi.stone")
talk("", "ch07.kundi.ok")
talk("", "ch07.kundi.follow")

flag.set("ch07.kundi")
