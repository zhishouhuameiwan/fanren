-- @hook ch01_shenshougu npc_mo_daifu npc
-- 第一章节点 9「授口诀」，章末。挂在 ch01_shenshougu 的 npc_mo_daifu 上。
-- （从前这里写的是「（interact）」。npc 对象没有 mode 属性——面朝它按确认是引擎
-- 对 npc 的固定做法，不是地图上的一个取值。首行的 @hook 因此只写 npc。）
--
-- 硬约束：这段口诀此时没有名字。「长春功」之名是后来才出现的，
-- 本文件与它引用的每一条文案都不得出现那三个字。韩立当面问了，
-- 墨大夫也只说「练出点样子来自然会告诉你」——悬念留到后面章节。
--
-- 同样不得出现的还有：修仙、灵根、法术。口诀的内容只讲呼吸与时辰，
-- 在此刻的玩家眼里，这就是个凡人武馆的师父在传吐纳法子。
--
-- 章末的二选一定下 ch01.xiulian_taidu，第 2 章「绿瓶四年」的苦修节奏接着用。

if flag.get("ch01.dazuo_done") ~= 1 then
    talk("mo_daifu", "ch01.koujue.not_yet")
    return
end

talk("", "ch01.koujue.night")
talk("mo_daifu", "ch01.koujue.mo_open")
talk("", "ch01.koujue.mo_recite")
talk("", "ch01.koujue.content")
talk("", "ch01.koujue.recite_done")

talk("", "ch01.koujue.ask_name")
talk("mo_daifu", "ch01.koujue.no_name")

local pick = choice{
    "ch01.koujue.opt_daily",
    "ch01.koujue.opt_more",
}

if pick == 2 then
    flag.set("ch01.xiulian_taidu", 2)
    talk("mo_daifu", "ch01.koujue.more_reply")
else
    -- 每日一遍（pick == 1）与取消（pick == nil）走同一条：默认是稳妥的那个打算。
    flag.set("ch01.xiulian_taidu", 1)
    talk("mo_daifu", "ch01.koujue.daily_reply")
end

-- 口诀到手，打坐面板上的「第一层」从这一刻起才按得上去（剧情境界上限，技术债 G-14；
-- 第 2 章段四才到第二层，见 docs/ch02-design.md 第 2 节）。放在二选一之后：两条分支都要走到。
realm.cap(realm.QI_REFINING_1)

talk("mo_daifu", "ch01.koujue.mo_last")
talk("", "ch01.koujue.bed")

-- 章末落在他自己的算盘上：谷里管三顿饭，家里少一张嘴。
-- 不给他任何志向，原著里的韩立此刻想的就是这个。
talk("", "ch01.koujue.end")

flag.set("ch01.koujue_received")
flag.set("ch01.done")
