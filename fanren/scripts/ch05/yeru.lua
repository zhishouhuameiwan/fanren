-- @hook ch05_nancheng trigger_yeru interact once
-- 第五章节点 6a「夜入墨府」。挂在 ch05_nancheng 墨府后巷的墙根 (24,3)(25,3)
-- （mode=interact，once=true，guard_flag=ch05.jiulou，set_flag=ch05.yeru）。
-- 三更出门翻墙是他定的（ch107）。潜入不造机制：trigger ＋ 对话 ＋ 选择（handoff 第 4 节）。
--
-- 二选一 → ch05.yeru：
--   1 绕到后园翻墙（原著 ch107）
--   2 从正街屋顶直穿过去——被暗处一个人抬头瞥见一个影子，伏了半个时辰；到小楼时屋里已说完
--     暗舵那一段（节点 6b 回读这一位，少听一段）。取消按 1 算：原著走的就是后墙。
--
-- 然后 teleport 进墨府后园西北角 (3,3)（genmaps_ch05.py 的 MOFU_BACK_GARDEN）。
-- 这时他还没登门：月洞门里站着暗哨（npc_anshao，只在 ch05.yeru 置了而 ch05.dengmen 没置时在场），
-- 夜里他走不到前院、出不了正门——出去了就再也进不来（genmaps_ch05.py 首部第四节）。

-- 三更出门，夜探的曲子（docs/audio.md 的 bgm_night）从这里起。**这一段不撤**：teleport 进墨府后园，
-- 他还伏在园子里摸向小楼，潜入没完；toutin.lua 听完了才撤（bgm("map")）。
bgm("bgm_night")
talk("", "ch05.yeru.night")
talk("", "ch05.yeru.q")

local pick = choice{
    "ch05.yeru.opt_wall",
    "ch05.yeru.opt_roof",
}

local yeru = 1
if pick == 2 then
    talk("", "ch05.yeru.roof")
    yeru = 2
else
    talk("", "ch05.yeru.wall")
end

flag.set("ch05.yeru", yeru)
teleport("ch05_mofu", 3, 3)
