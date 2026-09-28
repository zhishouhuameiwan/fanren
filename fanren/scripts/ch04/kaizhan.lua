-- @hook ch04_yanwuchang trigger_kaizhan enter once
-- 第四章节点 6「野狼帮来犯，开战」。本章的三选一在这里。
-- 挂在 ch04_yanwuchang 西辕门甬道最里那一格 (8,20)(8,21) 的 trigger
-- （mode=enter，once=true，guard_flag=ch04.fawu_bingyong，set_flag=ch04.kaizhan）。
-- 甬道八格深两格宽、两侧砌墙（genmaps_ch04.py：墙在 y=19 与 y=22，路在 y=20-21）。
-- 切磋完从台上回西门，出甬道之前必踩这一格 —— **号角是在他背后响的**，
-- 这一层是关卡侧摆位置时就想好的，文案 ch04.kaizhan.horn 照着写。
--
-- 节点 9 的 trigger_gongfang 在 (9,20)(9,21)，是甬道口外**第一格**，与这一处
-- 前后相邻而**不同格**。两个 enter 触发绝不同格：引擎的 tryStep 走
-- objectAt(target, "trigger")，不分 mode 只取第一个命中的，叠在一格上时
-- 排在后面的那个永远轮不到，而校验器规则 17 对带 guard 的这一类一个字都不说。
--
-- ---------------------------------------------------------------------------
-- 三选一：怎么布置（ch04.bushu = 1 守辕门 / 2 放进来围 / 3 先下药）
-- ---------------------------------------------------------------------------
-- **这一处不是口吻分叉，三条各自对应节点 9 的一份真编成**：
--   1 → data/battles/b04_yelangbang_laifan.json  四波九人，口子窄，一波一波耗
--   2 → data/battles/b04_gongfang_weijian.json   三波十人，场子最大，收获最高
--   3 → data/battles/b04_gongfang_xiadu.json     三波七人，人最少，代价在剧情里
-- 挑哪一张由 gongfang.lua 读这个旗标决定。三条的分野是**波数与每波人数的对调**，
-- 不是难度滑条 —— 详见那三份 note。
--
-- 第 3 条兑现设计文档硬约束 7：**全歼来犯者靠的是算计与毒，不是境界压制。**
-- 它也带着这一章最重的一笔账：东坡那口井供着半山的人吃水，药下去分不出敌我。
-- 所以本节先让厉飞雨把这句话说出来（ch04.kaizhan.xiadu1），韩立答的是「是」，
-- 而不是辩解。**这一条不设物品前置、不调 take()**：药是他丹房里的，
-- 门里这半年的药都出自他手，他要下药不必先向谁讨。
-- 拿一件玩家未必有的毒药去卡这一条，会让第 3 条在很多档存档里变成一个假选项，
-- 而本项目对每个分支的硬标准是三侧都走得到。
--
-- **ch04.bushu 必须置在末尾那一行之前**（施工纪律第 1 条：once 的判据只看
-- set_flag 指的 ch04.kaizhan）。先置后说，理由与第 3 章 tanpai.lua 同：
-- 写在分支台词前面，日后有人往分支里加 return 也漏不掉。
--
-- 三棱的伤口那一段（ch04.kaizhan.arrow / guess）是原著 ch81 军用连珠弩的落地：
-- 原著从贾天龙那一侧交代来路（堂兄是副将，两万多两银子换三百多张），
-- 本作的镜头跟着韩立，所以他只看得见伤口，看不见来路 —— 他把形状画下来压在药案下面，
-- 至于那是什么，这一章不给答案。

-- 本节**不另写前置**：地图上这个触发器带着 guard_flag=ch04.fawu_bingyong，
-- 节点 5 没演过它根本不 ready（WorldScene::triggerReady 第一件事就是查 guard）。
-- 再在脚本里补一道，那条台词永远走不到 —— 走不到的台词是负担，不是保险。
if flag.get("ch04.kaizhan") ~= 0 then
    talk("", "ch04.kaizhan.again")
    return
end

talk("", "ch04.kaizhan.horn")
talk("", "ch04.kaizhan.news")
talk("", "ch04.kaizhan.decide")
talk("", "ch04.kaizhan.wood")
talk("", "ch04.kaizhan.arrow")
talk("", "ch04.kaizhan.guess")
talk("", "ch04.kaizhan.war")
talk("", "ch04.kaizhan.mine")
talk("", "ch04.kaizhan.ask")

local pick = choice{
    "ch04.kaizhan.opt_yuanmen",
    "ch04.kaizhan.opt_weijian",
    "ch04.kaizhan.opt_xiadu",
}

-- 先置后说。取消（pick == nil）按第 1 条守辕门算：
-- 厉飞雨当面问他主意，一个答不上来的人，外刃堂会照最稳当的那一条自己办。
local bushu = pick or 1
flag.set("ch04.bushu", bushu)

if bushu == 2 then
    talk("", "ch04.kaizhan.weijian1")
    talk("", "ch04.kaizhan.weijian2")
elseif bushu == 3 then
    talk("", "ch04.kaizhan.xiadu1")
    talk("", "ch04.kaizhan.xiadu2")
else
    talk("", "ch04.kaizhan.yuanmen1")
    talk("", "ch04.kaizhan.yuanmen2")
end

talk("", "ch04.kaizhan.end")
-- 「三天备战」原先在这里 advance_days(3)，排在坊门那一战**之前**，于是攻防战紧接着
-- 坊门之战、中间 0 天，静养（Application::restFor，一天回一成）一刀也落不到攻防战上——
-- 曲魂带着坊门那一仗剩下的一两点气血进了攻防战（第 4 章复验 N-11）。
-- 这三天挪到了 jinguang.lua 末尾：殿上那一场之后、夜袭之前。全章的日子一天不差。
-- 台词跟着挪（2026-09-23）：ch04.kaizhan.end 不再说「三日后」，只写当夜；
-- 「三天」与「最后一处哨卡丢了」改由 ch04.jinguang.end 交代，那里才真的过这三天。

flag.set("ch04.kaizhan")
