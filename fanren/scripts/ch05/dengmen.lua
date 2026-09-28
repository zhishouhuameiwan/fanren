-- @hook ch05_mofu trigger_dengmen interact once
-- 第五章节点 7a「登门：纹龙戒」。挂在 ch05_mofu 小楼东窗 (33,7)（mode=interact，once=true，
-- guard_flag=ch05.toutin，set_flag=ch05.dengmen）。扔戒指、敲门是他的决定（ch109）。
--
-- 原著 ch109-111 的次序：戒指隔着窗纸扔进屋、敲门报名、两枚戒指合纹、呈上亲笔信；严氏请来二、三、五夫人；
-- 三夫人的天狐大法让他一瞬失神，丹田一股凉意窜上来把他激醒（ch110，**自动的**——那股凉就是阴毒）；
-- 他暗运长春功稳住心神再抬头（ch111）；讲拜师经过，七分真三分假（ch110）；当夜宿后宅厢房（ch111）。
-- 暗信显形（ch112）发生在他走了之后，这一节不演；节点 9 严氏一句话交代（转写高风险点 4）。
--
-- 两处 take 都看返回值：纹龙戒与亲笔信是节点 1a 给的剧情物品，不可交易，正常流程里一定在；
-- 真不在（旧存档、手搭的起点），换一句话往下走，不卡死——登门这件事不靠那两样东西才成立。
-- 纹龙戒留在严氏手里（原著未交代去向，本作定为留下）。
--
-- ch05.toutin == 2（偷听时听到「冒牌货」就现身）：他先开口揭穿吴剑鸣，严氏只回一句「知道」——先输一着。
--
-- 二选一 → ch05.dengmen（这一位同时是南城→墨府正门的闸门钥匙，以及暗哨退场的旗标）：
--   1 暗运长春功稳住心神，再抬头把几位夫人看清（原著 ch111）：他看出五夫人内力精湛、三夫人那一手是功夫；
--     节点 9、11a 各多一句他顶回去的话。
--   2 一直低着头。取消按 2 算：什么也没做，就是一直低着头。

talk("", "ch05.dengmen.ring")
if not take("story_wenlong_jie", 1) then
    talk("", "ch05.dengmen.ring_missing")
end
talk("mo_caihuan", "ch05.dengmen.see")
talk("", "ch05.dengmen.knock")
talk("", "ch05.dengmen.in")
talk("", "ch05.dengmen.match")

if take("story_mo_qinbixin", 1) then
    talk("", "ch05.dengmen.letter")
    talk("", "ch05.dengmen.keep")
else
    talk("", "ch05.dengmen.noletter")
end

if flag.get("ch05.toutin") == 2 then
    talk("", "ch05.dengmen.expose")
end

talk("yan_shi", "ch05.dengmen.call")
talk("", "ch05.dengmen.four")
talk("", "ch05.dengmen.fox")
talk("", "ch05.dengmen.cold")
talk("", "ch05.dengmen.q")

local pick = choice{
    "ch05.dengmen.opt_steady",
    "ch05.dengmen.opt_down",
}

local dengmen = 2
if pick == 1 then
    talk("", "ch05.dengmen.steady")
    dengmen = 1
else
    talk("", "ch05.dengmen.down")
end

talk("", "ch05.dengmen.story")
talk("yan_shi", "ch05.dengmen.rest")

flag.set("ch05.dengmen", dengmen)
