-- @hook ch06_huangfenggu trigger_chuangong interact
-- 第七章节点 8 与 15「传功阁：吴风」。挂在 ch06_huangfenggu 吴风跟前一格 (40,10)（他站 (40,9)；面前那两格是第 6 章的 trigger_wufeng）
-- （mode=interact，once=false，guard_flag=ch07.dufang，无 set_flag——地图规范 4.4）。
-- 施工图 3.1「同一格只挂一个对象」：触发不压在 npc_wu_feng 身上。一处管两场，脚本看旗标：
--
-- 节点 8（ch07.lianqi 未置）——框架件（施工图 13.1 第 8 段）：吴风服过筑基丹仍在炼气顶峰、对谁都肯教（开口按 ch06.wufeng 分）；
--   禁地：风属性古禁、每五年五天衰弱期、数名结丹合力开路；只有炼气期进得去；各派在里头为药互相下手；
--   近百年活着出来的不足三分之一，重赏之后不足四分之一；报名先赐中阶灵石一块、灵器一件；带出灵药按量按质给赏，
--   最高可换筑基丹；下一次在半年后；「血色试炼」四个字第一次出现；几夜不眠——写他怕；挑中敛气术与它的理由。
--   **讲述次序不照原著**（13 第 2 条）：先名字、再谁进得去、再给什么、再死几个、最后才是禁制与日子。
--   advance_days(3)：几夜辗转（施工图 3.3）。敛气术只置旗标 ch07.lianqi，不进法术表（16.1 第 13 条）。
--
-- 节点 15（ch07.fanhui 已置、ch07.fudan_fa 未置）——框架件（13.1 第 15 段）：借学法术的由头套出服法（直接吞、不要药引；
--   服了须闭关三个月化药）；与试炼不可兼得；近半月的思量（advance_days(14)）；告示：五年之后起封闭六十年、七派共派人看守；
--   三四百年一次的圈封；他的判断——五年后那一届是封山前最后一回、精锐尽出，这一届是他唯一的机会（验收 17：
--   ch07.fudan.* 里有一条同时含「五年」「六十年」「最后」；大纲那句错话「封闭前最后一次」0 处）。
--
-- 其余时候：吴风一句闲话。

if flag.get("ch07.lianqi") == 0 then
    if flag.get("ch06.wufeng") == 2 then
        talk("wu_feng", "ch07.chuangong.open_lost")
    else
        talk("wu_feng", "ch07.chuangong.open_won")
    end
    talk("", "ch07.chuangong.who")
    talk("", "ch07.chuangong.ask")
    talk("wu_feng", "ch07.chuangong.name")
    talk("", "ch07.chuangong.name_what")
    talk("wu_feng", "ch07.chuangong.only")
    talk("wu_feng", "ch07.chuangong.reward")
    talk("", "ch07.chuangong.fingers")
    talk("wu_feng", "ch07.chuangong.dead")
    talk("wu_feng", "ch07.chuangong.seal")
    talk("wu_feng", "ch07.chuangong.next")
    talk("", "ch07.chuangong.leave")

    advance_days(3)

    talk("", "ch07.chuangong.night1")
    talk("", "ch07.chuangong.night2")
    talk("", "ch07.chuangong.night3")
    talk("", "ch07.chuangong.choose")
    talk("", "ch07.chuangong.lianqi")
    talk("", "ch07.chuangong.reason")
    flag.set("ch07.lianqi")
    return
end

if flag.get("ch07.fanhui") ~= 0 and flag.get("ch07.fudan_fa") == 0 then
    talk("", "ch07.fudan.pretext")
    talk("", "ch07.fudan.ask")
    talk("wu_feng", "ch07.fudan.swallow")
    talk("wu_feng", "ch07.fudan.close")
    talk("", "ch07.fudan.clash")

    advance_days(14)

    talk("", "ch07.fudan.weigh")
    talk("", "ch07.fudan.notice")
    talk("", "ch07.fudan.notice_seal")
    talk("", "ch07.fudan.notice_why")
    talk("", "ch07.fudan.judge")
    talk("", "ch07.fudan.decide")
    flag.set("ch07.fudan_fa")
    return
end

talk("", "ch07.chuangong.idle")
