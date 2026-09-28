-- @hook ch05_dubashanzhuang trigger_tancha interact once
-- 第五章节点 12c「刺探与巡庄」。十场必打之九：b05_xunzhuang。
-- 挂在 ch05_dubashanzhuang 围墙西墙根 (20,12)（mode=interact，once=true，guard_flag=ch05.chuzheng，
-- set_flag=ch05.tancha）。刺探是他（ch126「经过数日的不停刺探和潜入」）。**寒毒检查点之四**。
--
-- 刺探的天数看独霸山庄的警觉（施工图第 7 节）：节点 8 他报了师门（ch05.huayuan == 2）→ 6 天，
-- 否则 3 天。（从前「12b 吴剑鸣走脱」也会让山庄警觉；他如今总会被擒，那一支没有了。）
-- 天亮前他伏在墙外看欧阳飞天练功：一个庄丁的刀收不住砍在他胳膊上，刀弹开了——
-- 改编的提示，接 11a「刀剑难伤」；欧阳飞天那一仗必败分支的提示三（施工图 8.4）。
-- **巡庄**（改编）：第二夜摸到墙根撞上一队巡庄的，得在他们出声之前放倒——**迷倒不杀**：
-- 一包药睡到天亮，醒来只当自己打了盹，不留尸体（第 5 章校对 MEDIUM-2 的拍板；山庄因此
-- 一点风声也没有，⑩「乘其不备」的前提还在）。
-- 不冲突的依据：ch126「一点波澜也没起」说的是刺杀那一下；ch283 的「无人追究」是修仙界的人，与庄丁无关。
-- 输即 game_over（defeat_is_fatal 真；can_escape 假）。

-- ---------------------------------------------------------------------------
-- 寒毒检查点（docs/ch05-design.md 3.3）。本章五个检查点（dingji / anpai / zhuwu / tancha /
-- huanyu）写的是同一段，改一处就改五处；ScriptHost 关了 package 与 dofile，脚本之间共用不了函数。
-- 发作一次吞一颗养精丹顶住是游戏规则，原著没有这一笔（改编）；超期不判输（用户拍板），只多一句话。
-- ---------------------------------------------------------------------------
local HANDU_DUAN2 = 65    -- 发作起第 65 日入第二段：进段后的第一个检查点发作一次
local HANDU_DUAN3 = 80    -- 第 80 日入第三段：此后每个检查点各发作一次
local HANDU_QIXIAN = 90   -- ch104「顶多再有两个月」是 d≈30 时说的，即 d = 90

local function handu_fazuo(chaoqi)
    talk("", "ch05.handu.attack")
    if item.count("pill_yangjing_dan") > 0 and take("pill_yangjing_dan", 1) then
        talk("", "ch05.handu.pill")
    else
        talk("", "ch05.handu.nopill")
    end
    if chaoqi then
        talk("", "ch05.handu.over")
    end
end

local function handu_jiancha()
    local qi = flag.get("ch05.yindu_qi")
    if qi <= 0 then return end
    local d = today() - qi
    if d >= HANDU_DUAN3 then
        flag.set("ch05.handu_fa", 3)
        handu_fazuo(d >= HANDU_QIXIAN)
    elseif d >= HANDU_DUAN2 and flag.get("ch05.handu_fa") < 2 then
        flag.set("ch05.handu_fa", 2)
        handu_fazuo(false)
    end
end

handu_jiancha()

-- 刺探是夜里的事（ch126「不停刺探和潜入」）：寒毒那一关过了才点夜探的曲子，演完撤回地图曲。
-- 巡庄那一仗换战斗曲归战斗画面；打输了 game_over，不必撤。
bgm("bgm_night")
talk("", "ch05.tancha.watch")

if flag.get("ch05.huayuan") == 2 then
    talk("", "ch05.tancha.alert")
    advance_days(6)
else
    advance_days(3)
end

talk("", "ch05.tancha.arm")
talk("", "ch05.tancha.think")
talk("", "ch05.tancha.patrol")

local won = battle("b05_xunzhuang")
if not won then
    talk("", "ch05.tancha.lost")
    game_over()
    return
end

talk("", "ch05.tancha.drugged")
talk("", "ch05.tancha.drugged2")

bgm("map")
flag.set("ch05.tancha")
