-- @hook ch07_jindi_zhongxin trigger_tongmen enter once
-- 第七章节点 24「铜门；钟吾；秘闻；树洞」。挂在 ch07_jindi_zhongxin 青铜门内第一格 (23,13)(24,13)
-- （mode=enter，once=true，guard_flag=ch07.fengyue，set_flag=ch07.zhongwu）。铜门前的三具尸与门里的飞蛇都是撞上的。
--
-- 框架件（施工图 13.1 第 24 段）：石墙、四扇青铜门之一，门开着；墙上钉着三具尸——**动手的人不点名**（寒天涯 0 处，12.2）；
-- 门内第一层是花园（花木本作自出）；灵兽山钟吾放飞蛇偷袭，认出踏云靴（「踏云靴」「钟吾」从这里起出现）；
-- 二选一：1 照实说封岳已死（原著：钟吾退开、改口称兄弟）/ 2 说是捡的（钟吾不信，也退开、更忌惮）；取消按 1 算；
-- 交换玉简资料（各派明令不许）→ 置 ch07.yujian_qiu（支线 Z2 接取）；中心区秘闻：占禁地三分之一多、由外向里三层——
-- 花园、终年浓雾的环形山（妖兽多、灵药长在洞谷石殿里）、百丈巨塔（谁也进不去）；月阳宝珠驱雾、七派轮掌、本次约定第三日早上。
-- **原著「果皮果肉果核」那个比方不用**（13 第 5 条）。末尾 advance_days(1)：树洞里睡到第三日凌晨。

talk("", "ch07.tongmen.wall")
talk("", "ch07.tongmen.bodies")
talk("", "ch07.tongmen.garden")
talk("", "ch07.tongmen.snakes")
talk("", "ch07.tongmen.dodge")
talk("zhong_wu", "ch07.tongmen.boots")
if take("story_hei_xue", 1) then
    give("story_tayun_xue")
end
talk("", "ch07.tongmen.man")

local pick = choice{
    "ch07.tongmen.opt_true",
    "ch07.tongmen.opt_found",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.tongmen.truth")
    talk("zhong_wu", "ch07.tongmen.truth_zw")
else
    talk("", "ch07.tongmen.found")
    talk("zhong_wu", "ch07.tongmen.found_zw")
end
talk("zhong_wu", "ch07.tongmen.wall_who")
talk("zhong_wu", "ch07.tongmen.swap")
talk("", "ch07.tongmen.swap_do")
talk("", "ch07.tongmen.layers")
talk("", "ch07.tongmen.ring")
talk("", "ch07.tongmen.pearl")
talk("", "ch07.tongmen.tower")
talk("", "ch07.tongmen.part")
talk("", "ch07.tongmen.hollow")

advance_days(1)

flag.set("ch07.yujian_qiu")
flag.set("ch07.zhongwu", pick)
