-- @hook ch05_mofu trigger_huayuan enter once
-- 第五章节点 8「花园：燕歌与吴剑鸣（伪装）」。挂在 ch05_mofu 夹道靠厢房的一格 (42,8)
-- （mode=enter，once=true，guard_flag=ch05.jianmianli，set_flag=ch05.huayuan）。
-- 撞见吴剑鸣是碰巧（ch114）——事情找上他，所以是踏入型。7b 把人放在厢房床前，他第二天出门必踩这一格。
--
-- 燕歌来叫；园中撞见墨玉珠与吴剑鸣；吴剑鸣问他是谁，燕歌看墨玉珠看呆了，没替他答（ch114）。
-- 二选一 → ch05.huayuan：
--   1 自称三夫人的远房堂侄，来求个差事（原著）
--   2 报上师门，当面戳吴剑鸣的身份：吴剑鸣当场变了脸，回头给什么人捎了话——节点 12c 刺探多三天。
--     这时还不说他师父是谁（吴剑鸣是欧阳飞天的七弟子，ch123，节点 11a 严氏才说破）。
--   取消按 1 算：原著里他就是这么答的。
-- 燕歌痴情那几句留着（ch114）——它是节点 11b 那一仗的伏笔。

talk("yan_ge", "ch05.huayuan.knock")
talk("yan_ge", "ch05.huayuan.frank")
talk("", "ch05.huayuan.walk")
talk("", "ch05.huayuan.meet")
talk("", "ch05.huayuan.dumb")
talk("", "ch05.huayuan.q")

local pick = choice{
    "ch05.huayuan.opt_nephew",
    "ch05.huayuan.opt_master",
}

local huayuan = 1
if pick == 2 then
    talk("", "ch05.huayuan.master")
    huayuan = 2
else
    talk("", "ch05.huayuan.nephew")
end

talk("", "ch05.huayuan.leave")
talk("yan_ge", "ch05.huayuan.yange1")
talk("", "ch05.huayuan.yange2")

flag.set("ch05.huayuan", huayuan)
