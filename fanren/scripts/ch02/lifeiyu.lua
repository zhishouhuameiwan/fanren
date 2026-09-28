-- @hook ch02_wairentang npc_li_feiyu npc
-- 厉飞雨的闲聊。挂在 ch02_wairentang 的 npc 对象上（role_id=li_feiyu）。
--
-- 语气卡（docs/ch02-design.md 第 7 节）：直爽、话密、讲义气，吃了亏也不记仇。
-- 他拿刀架过韩立的脖子，回头照样跟他说家里开过铁铺、一场火烧光了。
-- 这种不记仇要写成本性，不要写成他在示好。
--
-- 硬约束（第 1 节第 6 条）：没有结义。他嘴里最亲近的说法到「韩师弟」为止，
-- 不结拜、不磕头、不称兄道弟。
--
-- 他说「学医的手该干净着，沾了血就抖，抖了就扎不准针」——
-- 这是他自己的江湖道理，顺带把韩立不学武这件事说圆了（原著 ch10：
-- 墨大夫严禁他碰刀枪，说会妨碍口诀的进度）。
--
-- 人情没结下之前这个 NPC 不该在外刃堂现身，关卡侧按 ch02.renqing_jiexia 控制；
-- 脚本这边也挡一道。

if flag.get("ch02.renqing_jiexia") ~= 1 then
    return
end

if flag.get("ch02.liao_lifeiyu") == 1 then
    talk("li_feiyu", "ch02.npc.lifeiyu_yao")
    talk("li_feiyu", "ch02.npc.lifeiyu_again")
    return
end

talk("li_feiyu", "ch02.npc.lifeiyu_1")
talk("li_feiyu", "ch02.npc.lifeiyu_2")
talk("li_feiyu", "ch02.npc.lifeiyu_3")

flag.set("ch02.liao_lifeiyu")
