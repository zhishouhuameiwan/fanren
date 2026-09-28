-- @hook ch04_yanwuchang trigger_gongfang enter once
-- 第四章节点 9「门派攻防战」。**本章高潮，多波次。**
-- 挂在 ch04_yanwuchang 西辕门甬道口外第一格 (9,20)(9,21) 的 trigger
-- （mode=enter，once=true，guard_flag=ch04.jinguang_jian，set_flag=ch04.gongfang_zhan）。
-- 与节点 6 的 trigger_kaizhan (8,20)(8,21) **前后相邻但不同格** —— 原委见
-- kaizhan.lua 首部那一段（两个 enter 叠在一格上，排后面的永远轮不到）。
--
-- ---------------------------------------------------------------------------
-- 三张编成由节点 6 的 ch04.bushu 挑。**这一处是本章唯一一个改变战斗数据的分支。**
-- ---------------------------------------------------------------------------
--   1 守辕门   → b04_yelangbang_laifan  四波九人
--   2 放进来围 → b04_gongfang_weijian   三波十人，场子最大
--   3 先下药   → b04_gongfang_xiadu     三波七人
-- 这三行的波数与人数（2026-09-23 核对）以 data/battles/ 那三份编成的 units 为准，
-- 与 docs/ch04-design.md 3.2 那张表一致（四波九人 490 血 / 三波十人 488 血 / 三波七人 362 血）。
-- 从前这里写的人数是 HIGH-2 重调之前那张旧编成（596 血），已作废。编成再改，这三行跟着改。
-- 三张都是 can_escape 假、defeat_is_fatal 真，最后一波都是金光上人 ＋ 贾天龙。
-- 波与波之间不回血、不回法力（契约 docs/interfaces-p3-ch04.md 2.1），
-- 所以前几波省不省得下法力，直接决定最后一波打不打得动那层金 —— 这一场的算计全在这里。
--
-- **ch04.bushu 读不出来时按 1 兜底。** kaizhan.lua 一定会置它（取消也置 1），
-- 所以这一支在正常流程下走不到；写它是为了不让一个读出 0 的存档拿到一个空的
-- battle id ——那会掉进 BattleScene 的兜底遭遇，玩家会在门派攻防战里打一场杂兵。
--
-- ---------------------------------------------------------------------------
-- 硬约束 2 与 5：门派存续，非倾覆；厉飞雨活着
-- ---------------------------------------------------------------------------
-- 所以这一场**赢是唯一一种往下走的落点**，输就是输（game_over）。
-- 这不是偷懒：设计文档第 0 节写明本章的落点是「守住了门派」，硬约束 5 写明门派存续。
-- 若让输也能往下走，就得回答「那门是谁守住的、金光上人是谁烧的」，
-- 而任何一个答案都会把节点 10 的战利品与第 6 章那条伏笔一起拆掉。
-- 第 3 章识海那一场是同一口径（defeat_is_fatal 由脚本落 game_over，引擎不自己消费）。
--
-- **必须看第二个返回值。** 三张的 can_escape 都是假，所以 "escaped" 不该出现；
-- 真出现了按输算而不是当成赢往下走 —— 静默吞掉一个不该出现的落点，
-- 正是第 3 章契约反复点名的那种错。
--
-- ch04.gongfang.dwarf / think 两条是留给玩家的**机制提示，不是教程**：
-- 刀砍上去只溅火星（金光上人防 30，韩立平砍破不了防），所以他把火压到最小、绕到侧后。
-- 这两句不说「你该用法术」，只写他做了什么 —— 玩家自己对得上。
--
-- 曲魂由 BattleScene::addPartyUnits 自动上场，ch04.gongfang.ready 里那根「柱子」
-- 就是他。他不说话、不成长（第 3 章设计第 4 节），本节没有一句 talk 的说话人是他。

if flag.get("ch04.gongfang_zhan") ~= 0 then
    talk("", "ch04.gongfang.again")
    return
end

-- 身上一瓶养精丹也没有：厉飞雨在甬道口问一句（二次复验 R-3、5.5 第 5 条）。
-- 这一场输即 game_over，而一炉不炼几乎必输；全章从前没有一句话提醒这件事。
-- 选「回峰上开炉」就在这里 return：ch04.gongfang_zhan 没置，once 不烧，挂点留着，
-- 炼完回来再踩一次照演（与 kailu.lua 开头那道闸同一个办法）。
-- 选「就这样上」照打——不许把这里写成死闸：药卖光了、料也用光了的玩家会被锁死在门外。
-- 看的是身上的瓶数，不看 ch04.liandan：切磋时有药、后来卖光了的人，同样该被问到。
if item.count("pill_yangjing_dan") == 0 then
    talk("li_feiyu", "ch04.gongfang.nopill")
    local back = choice{
        "ch04.gongfang.opt_back",
        "ch04.gongfang.opt_go",
    }
    -- 取消（back == nil）按回峰上算：那是安全的一侧——挂点留着，想打随时回来再踩；
    -- 按「就这样上」算的话，一下手滑就是一场几乎必输、输即 game_over 的仗。
    if back ~= 2 then
        talk("", "ch04.gongfang.back")
        return
    end
end

talk("", "ch04.gongfang.night")
talk("", "ch04.gongfang.first")
talk("", "ch04.gongfang.waste")
talk("", "ch04.gongfang.ready")
talk("", "ch04.gongfang.count")

local bushu = flag.get("ch04.bushu")
local battle_id = "b04_yelangbang_laifan"

if bushu == 2 then
    battle_id = "b04_gongfang_weijian"
    talk("", "ch04.gongfang.plan_weijian")
elseif bushu == 3 then
    battle_id = "b04_gongfang_xiadu"
    talk("", "ch04.gongfang.plan_xiadu")
else
    talk("", "ch04.gongfang.plan_yuanmen")
end

talk("", "ch04.gongfang.wave")
talk("", "ch04.gongfang.dwarf")
talk("", "ch04.gongfang.think")

local won, how = battle(battle_id)

if not won or how == "lost" or how == "escaped" then
    -- 输、逃、以及任何不是「赢」的落点，全部收在这里。
    -- 三张编成的 can_escape 都是假，"escaped" 走到了就是引擎侧回归；
    -- 按输算是安全的那一侧 —— 把一个没打完的攻防战当成守住了，
    -- 会让节点 10 从一堆不存在的灰里翻出两件东西来。
    talk("", "ch04.gongfang.lost1")
    talk("", "ch04.gongfang.lost2")
    game_over()
    return
end

talk("", "ch04.gongfang.win1")
talk("", "ch04.gongfang.win2")
talk("", "ch04.gongfang.win3")
talk("", "ch04.gongfang.feiyu")
talk("wang_juechu", "ch04.gongfang.wang")
talk("", "ch04.gongfang.gate")
talk("", "ch04.gongfang.end")

flag.set("ch04.gongfang_zhan")
