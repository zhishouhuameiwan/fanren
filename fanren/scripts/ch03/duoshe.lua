-- @hook ch02_jusuo trigger_duoshe enter once
-- 第三章节点 10「夺舍：识海之战」。本章高潮的上半。
-- 挂在 ch02_jusuo 屋里床边那一排格子上的 trigger
-- （mode=**enter**，once=true，set_flag=ch03.shihai_done）。
--
-- **mode 是这一节的设计本身，不是接线细节。** 玩家走到床边就发生，
-- 不按任何一个键——这一节的失重感一半来自没有 choice()，另一半就来自这里：
-- 他连「要不要开始」都没得选。反过来，下一节（chujue.lua）挂的是 interact，
-- 玩家要自己走过去、面朝它、按下确认。两个 mode 装反过一次，
-- 现由 tests/Ch03TriggerModeTests.cpp 直接钉住这两个属性。
-- 顺带记一笔：`touch` 不是引擎认的取值，引擎只认 `enter` 与 `interact`
-- （src/game/WorldScene.cpp 的 tryStep 与 interact）。这一行从前写的正是 `touch`。
--
-- ---------------------------------------------------------------------------
-- 两种杀的第一种：他睡着的时候，事情就已经办完了。
-- ---------------------------------------------------------------------------
-- **这一节从头到尾没有一次 choice()。** 这是有意的，也是本章最要紧的一处设计：
-- 玩家在这一节唯一能操作的东西，是一场规则和他刚学会的那一套完全不搭界的仗——
-- 没有法术、没有物品、没有地形、不能逃，只有谁大谁小、谁吞了谁。
-- 他被人提着走过自己管了四年的药圃，脸朝天，一畦一畦倒着过去；
-- 额头上贴着的纸边扫他的眼皮，他连眨一下躲开都做不到。
-- 这种失重感就是这场戏要的（设计第 2 节），而失重感是靠**取消选择**做出来的，
-- 不是靠台词说「他身不由己」。
--
-- 与之对照的是下一个脚本 chujue.lua：那一节玩家要自己走过去、自己选、自己动手。
-- 两节的操作感必须完全不同，否则这一章的道德分量就没了（大纲注解）。
--
-- 贪心那一层留住了（设计 1.1）：第二团本来是要退回去的，是他先扑上去的。
-- 但这句话写成**事后的回想**——「这件事他后来想了很多年也想不明白」——
-- 而不是写成一次弹窗。梦里那个东西是他，可他当时并没有在做决定。
--
-- 战斗数据 b03_shihai_duoshe 由引擎方按 ch56 重做成两团摆在同一场：
-- 近的一团（黄光球 = 墨居仁元神）一照面就咬，远的一团（绿光球 = 余子童元神）
-- 要玩家自己走过去。所以脚本这边只调一次 battle()。
--
-- **必须看第二个返回值。** 这一场 won 永远是 false：绿光球设计上打不死，
-- 体积掉到三分之二以下必然逃走，落点是 enemy_fled。
-- 只看 won 会把「敌人带着伤跑了」读成「韩立输了」，把本章高潮写反。

if flag.get("ch03.jieyao_xuan") < 1 then
    talk("", "ch03.duoshe.gate")
    return
end

if flag.get("ch03.shihai_done") == 1 then
    talk("", "ch03.duoshe.again")
    return
end

talk("", "ch03.duoshe.call")
advance_days(4)

talk("", "ch03.duoshe.walk")
talk("", "ch03.duoshe.paper")
talk("", "ch03.duoshe.hut")
talk("", "ch03.duoshe.lamp")
talk("", "ch03.duoshe.blow")

-- 屋里的第二个声音。韩立这时才把「余子童」三个字和它对上——
-- 那三个字是他在节点 8 从手札的落款里认出来的，不是这会儿才听说的。
talk("", "ch03.duoshe.voice")
talk("", "ch03.duoshe.talk")
talk("", "ch03.duoshe.hanli")
talk("", "ch03.duoshe.chant")
talk("", "ch03.duoshe.sleep")

talk("", "ch03.duoshe.dream1")
talk("", "ch03.duoshe.dream2")
talk("", "ch03.duoshe.second")

local won, how, spoils = battle("b03_shihai_duoshe")

if how == "lost" then
    -- 设计第 2 节：识海这一场不可逃、**败即结束**。
    -- b03_shihai_duoshe 的 defeat_is_fatal 是 true，可这个字段引擎不自己消费，
    -- 落地归脚本，所以 game_over 写在这里。
    talk("", "ch03.duoshe.lost1")
    talk("", "ch03.duoshe.lost2")
    game_over()
    return
end

if how == "enemy_fled" then
    -- 咬下了多少（0-100）。章末余子童的状态与台词要对得上这个数。
    flag.set("ch03.shihai_yaoxia", spoils)
else
    -- 走到这里说明引擎报了一个本场不该出现的落点（"" / "escaped" / "unfinished"）。
    -- 契约 3.4 与 5.3 都钉死了这一场必然 enemy_fled：绿光球的体积单调下降，
    -- 剩三分之二时就走，绝不可能先被打到 0，也不可能赢或逃。
    -- 真出现了，剧情这边按原著口径记三分之一往下走——**不是静默吞掉**：
    -- 这条分支在正常流程下永远走不到，走到了就是引擎侧回归，
    -- 记在旗标上的那个 33 会和实战数对不上，一测就露。
    -- 这里曾经写死成 33。那样一来光看旗标分不出
    -- 「它带着伤跑了」与「它被打死了」——验收测试当时
    -- 只能另加一条数据先验来补这一口。
    -- 没人逃走就是 0：旗标记的是「咬下了多少」，
    -- 没咬下东西就该是零，而不是一个好看的数。
    flag.set("ch03.shihai_yaoxia", 0)
end

talk("", "ch03.duoshe.won1")
talk("", "ch03.duoshe.won2")
talk("", "ch03.duoshe.won3")

talk("", "ch03.duoshe.greedy")
talk("", "ch03.duoshe.flee1")
talk("", "ch03.duoshe.flee2")

-- ---------------------------------------------------------------------------
-- 这里本该按「咬下了多少」分两句话，**没有接**（主控裁决 C3-2 的前提没成立）。
-- ---------------------------------------------------------------------------
-- 裁决原话是「接上，但只在两条分支都可达时」，阈值取 40，依据是「实测咬下的
-- 比例落在 33-45%」。那个区间不对。
--
-- tests/Ch03TutorialBattleTests.cpp 的 EveryPlanLandsOnOneOfTheTwoBranches...
-- 把玩家在识海里的打法穷举了一遍（字母表「咬黄／咬绿／等」，前 14 个我方回合
-- 全组合，绿光球脱身即剪枝，共 4,728,837 条），实测：
--
--   敌人逃走 1,857,484 条，韩立输 2,871,353 条，没人逃走 0 条；
--   战果区间 **33%-38%**，分布 33×148424 34×738690 35×582773
--   36×263858 37×112282 38×11457。
--
-- **40 一条也到不了。** 按裁决那一句「若枚举证明某一条 0 次可达，就不要接」，
-- 这里保持不接，等主控定一个落在 33-38 里的数。届时这一段换成：
--
--   local bitten = flag.get("ch03.shihai_yaoxia")
--   if bitten >= <新阈值> then talk("", "ch03.duoshe.flee_deep")
--   else talk("", "ch03.duoshe.flee_shallow") end
--
-- 两条文案与那次穷举的实测数一并写在 docs/tech-debt.md 的 C3-2 里。
-- **不许**为了「看起来接上了」而让两条分支念同一句话。
-- 咬下多少，章末它的样子就不同。阈值 36 不是拍的：
-- 穷举 470 万条真引擎路径，战果落在 33-38 之间，各档条数
-- 33→148424 / 34→738690 / 35→582773 / 36→263858 / 37→112282 / 38→11457。
-- 取 36 时两侧都有量（约 147 万对 39 万）；取 33 或 39 以上会让某一侧变成 0，
-- 那就是个假分叉——本项目对每一个分支的硬标准是两侧都走得到。
local bitten = flag.get("ch03.shihai_yaoxia")
if bitten >= 36 then
    talk("", "ch03.duoshe.flee_deep")
else
    talk("", "ch03.duoshe.flee_shallow")
end

talk("", "ch03.duoshe.flee3")
talk("", "ch03.duoshe.wait")

flag.set("ch03.shihai_done")
