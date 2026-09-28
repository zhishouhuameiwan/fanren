-- @hook ch04_getang trigger_liuxia enter once
-- 第四章节点 1「以韩神医的身份留下」。本章的开场。
-- 挂在 ch04_getang 二门那两格 (23,32)(24,32) 的 trigger
-- （mode=enter，once=true，set_flag=ch04.liuxia）。
-- 二门墙是外院进内院唯一的口子（tools/mapgen/genmaps_ch04.py 第三节），
-- 玩家从山下镇北上、穿过外院，一脚踏进内院这一幕就开演，他没有绕过去的余地。
-- mode=enter 是对的：这一节是门里叫他来的，不是他自己推门去问。
--
-- ---------------------------------------------------------------------------
-- 这一节要立住的三件事
-- ---------------------------------------------------------------------------
-- 1. **他是自己选择留下的，不是无处可去。** 设计文档第 0 节第 2 条写明这一点。
--    所以对话里不给他任何「走投无路」的台词，他谈的是条件：要哪座峰、药园归谁。
-- 2. **那封假信是他自己伪的**（原著 ch65）。本作把伪信写在明处——写坏六张才留下
--    一张——因为这一章往后他还要撒很多谎，头一个谎得让玩家看见他是怎么撒的。
-- 3. **他红起来靠的是催出来的药，这件事他不说。** 掌天瓶这条线从第 2 章接下来，
--    ch04.liuxia.cure 那一条写明了「这一节他不打算对谁说」，全章此后一个字也没有再提。
--
-- 二选一：ch04.liuxia = 1 应下 / 2 本想推辞但还是留下了。
-- **两条都留下**（旗标登记表原话：都会留，只改口吻）。这不是假分叉——
-- 分的是他为什么留：一条是他先算清了要什么，一条是他差点没算清。
-- 节点 11 回头看这一处时，两条的分量不同。
--
-- 旗标值必须非零：ch04_getang 的 portal_to_luorifeng 拿它当 require_flag，
-- 而引擎判的是 state.flag(x) != 0（src/game/WorldScene.cpp）。1 与 2 都过得去。

if flag.get("ch04.liuxia") ~= 0 then
    talk("", "ch04.liuxia.again")
    return
end

talk("", "ch04.liuxia.rain")
talk("", "ch04.liuxia.letter")
talk("", "ch04.liuxia.claim")
talk("", "ch04.liuxia.wait")
talk("", "ch04.liuxia.doubt")

-- 半年。门里先拿底下弟子试他的深浅（原著 ch65：长老们并未让他立刻接手）。
-- 时间真的走完，灵田与掌天瓶跟着结算——他这半年正是靠田里那些药红起来的。
talk("", "ch04.liuxia.trial")
advance_days(180)

talk("", "ch04.liuxia.cure")
talk("", "ch04.liuxia.fame")

-- ---------------------------------------------------------------------------
-- 本章第一次涨境界：炼气三层 → 五层（设计文档 1.2 节，依据原著 ch43）
-- ---------------------------------------------------------------------------
-- 第 3 章章末他是炼气三层，而原著到那时（ch43）已是第六层——**那三层是第 3 章欠的**，
-- 第 3 章已两轮复核并冻结，不回头动，差额由本章节点 1、2 补回来。
-- ch43 原话是「仗着两种圣药的效力……更加难炼的第五层、第六层，他毫不费力的就练成了」，
-- 而让那句话成立的条件恰好是这一节发生的事：**药房的钥匙到了他手上。**
-- 他这半年不是在打坐，是在随便吃药——所以上面那两句写的是柜子和药账，不是吐纳。
--
-- **看返回值。** 契约 docs/interfaces-p3-ch04.md 6.2：目标低于当前会返回 false
-- （原因码 not_higher），编号非法返回 no_realm。真走到失败那一条说明这个数写错了，
-- 而静默收下的后果是这一章的数值全部对不上账——第 4 章正因为「境界这一位指不出来路」
-- 被复审打回过一次。
talk("", "ch04.liuxia.jinjie1")
talk("", "ch04.liuxia.jinjie2")
if not realm.advance(realm.QI_REFINING_5) then
    talk("", "ch04.jinjie.fail")
end

talk("", "ch04.liuxia.summon")
talk("", "ch04.liuxia.offer")
talk("", "ch04.liuxia.money")

local pick = choice{
    "ch04.liuxia.opt_stay",
    "ch04.liuxia.opt_leave",
}

if pick == 2 then
    talk("", "ch04.liuxia.leave1")
    talk("", "ch04.liuxia.leave2")
else
    -- 应下（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按应下算：门里当面问他要什么，一个盘算了四年的人不会在这一刻发呆。
    talk("", "ch04.liuxia.stay1")
    talk("", "ch04.liuxia.stay2")
end

talk("", "ch04.liuxia.peak")
talk("", "ch04.liuxia.reason")
talk("", "ch04.liuxia.end")

-- 末尾这一行就是 once 的兑现判据（trigger 的 set_flag 指着它）。
-- 值写 1 / 2，不写 true：节点 11 的信与厉飞雨的口吻都要回读它。
flag.set("ch04.liuxia", pick == 2 and 2 or 1)
