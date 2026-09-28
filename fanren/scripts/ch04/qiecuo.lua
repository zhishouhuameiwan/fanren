-- @hook ch04_yanwuchang trigger_qiecuo interact once
-- 第四章节点 5「法武并用：他自己摸出来的打法」。
-- 挂在 ch04_yanwuchang 演武台台面正中 (23,7)(24,7) 的 trigger
-- （mode=interact，once=true，set_flag=ch04.fawu_bingyong）。
-- 演武台四面石栏，只南面留两格台阶上得去（genmaps_ch04.py make_yanwuchang：
-- 石栏 (18,4)-(29,11)，台阶 (23,11)(24,11)，台面石板 (19,5)-(28,10)）。
-- **mode=interact 是设计本身**：这一打是他自己要打的。关卡侧点名交代过这一条 ——
-- 摆成踏入型就等于把「主动」摊薄成「路过」，而这一节的全部分量就在主动上。
--
-- ---------------------------------------------------------------------------
-- 硬约束 1（设计文档）：**法武并用是他自己摸出来的，不是谁教的。**
-- ---------------------------------------------------------------------------
-- 这一条在本节要经得起两种查法：
--   · 场上没有任何人提示。厉飞雨从头到尾没有说过一句「你何不……」，
--     战斗数据 b04_qiecuo_feiyu 的 note 里也钉了一句「这一场不教任何东西」。
--   · 连**他自己**也说不出那是什么。ch04.qiecuo.ask 里厉飞雨问他是哪一路的路数，
--     他张了张嘴答不上来；ch04.qiecuo.noname 点明「凑出来的东西还没有名字」。
--     没有名字，就没有人教得了。
--
-- 他怎么摸出来的，本作给的因果是**恼的**，不是悟的：
-- 火甩出去被让开了（原著 ch75 的道理：飞射的火球太慢，轻功高手躲得过），
-- 第二团生出来的时候他没撒手 —— 「他也说不清为什么没撒手，大约是不服气」。
-- 顿悟写成脾气，这一层是本作加的；它接着节点 4 那句「头一件没有理由的事」。
--
-- ---------------------------------------------------------------------------
-- 二选一：说不说实话（ch04.fawu_bingyong = 1 说 / 2 含糊）
-- ---------------------------------------------------------------------------
-- 这一次二选一不改战力，改的是**他和这个朋友之间还剩多少真话**。
-- 第 3 章他对厉飞雨撒过一次谎（ch03.npc.lifeiyu_hide），那一次是为了不连累人；
-- 这一次没有人逼他。选含糊的那一条里有一句「从前他只瞒事，如今开始瞒自己是谁了」,
-- 节点 11 写信时会回读这个旗标。
-- 选说实话那一条要付一记内劲拍肩的代价（原著 ch74 结尾厉飞雨那几巴掌），
-- 这一处是原著细节的错位使用：原著里那是报复他揭短，本作里那是他难得讲了半句真话。
--
-- 旗标值必须非零：ch04_yanwuchang 的 trigger_kaizhan 拿它当 guard_flag，
-- 引擎判的是 state.flag(x) != 0。1 与 2 都过得去。
--
-- **战斗的三个返回值这一场用得上两个。** b04_qiecuo_feiyu 可逃可败
-- （can_escape 真、defeat_is_fatal 假），所以赢、逃、败三种落点在剧情上分三条，
-- 逃**不并进败**：自己跳下台与被打趴下，对一个刚摸到点门道的人是两件事。

if flag.get("ch04.fawu_bingyong") ~= 0 then
    talk("", "ch04.qiecuo.again")
    return
end

-- 目标链「炼药」那一步（data/objectives/ch04.json 的 n4b_liandan）的完成旗标（二次复验 R-3）。
-- 炼丹面板不起脚本，没有地方在炼成的那一刻置旗标；御风决之后下一个起脚本的就是这一节，
-- 所以在这里看一眼：身上揣着养精丹来切磋，那一步就算做过了。没炼也照样往下演——
-- 目标行已经提醒过一次，攻防战前 gongfang.lua 还会再问一句。
if item.count("pill_yangjing_dan") > 0 then
    flag.set("ch04.liandan")
end

talk("", "ch04.qiecuo.come")
talk("", "ch04.qiecuo.stage")
talk("", "ch04.qiecuo.two")
talk("", "ch04.qiecuo.start")
talk("", "ch04.qiecuo.throw")
talk("", "ch04.qiecuo.slow")
talk("", "ch04.qiecuo.hold")
talk("", "ch04.qiecuo.melt")
talk("", "ch04.qiecuo.step")

local won, how = battle("b04_qiecuo_feiyu")

if won then
    talk("", "ch04.qiecuo.win")
elseif how == "escaped" then
    talk("", "ch04.qiecuo.escape")
else
    -- "lost" 与引擎可能回填的空落点走同一条。这一场不会出现 "enemy_fled"
    -- （厉飞雨不逃），真出现了按「没打完」算，与输同样处理 —— 不静默吞掉，
    -- 台词照样说得通，而回归测试里战果对不上一看就露。
    talk("", "ch04.qiecuo.lose")
end

talk("", "ch04.qiecuo.ask")
talk("", "ch04.qiecuo.noname")

local pick = choice{
    "ch04.qiecuo.opt_tell",
    "ch04.qiecuo.opt_hide",
}

if pick == 2 then
    talk("", "ch04.qiecuo.hide1")
    talk("", "ch04.qiecuo.hide2")
else
    -- 说实话（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按说实话算：朋友当面问一句，沉默在这个场合就是最重的那种含糊，
    -- 而他这时还没有打算对厉飞雨用最重的那一种。
    talk("", "ch04.qiecuo.tell1")
    talk("", "ch04.qiecuo.tell2")
end

talk("", "ch04.qiecuo.end")

flag.set("ch04.fawu_bingyong", pick == 2 and 2 or 1)
