-- @hook ch01_liangu_ya npc_mo_daifu npc
-- 第一章节点 7「墨大夫出面点名」。
--
-- 这一节的分寸最难拿：墨大夫此刻只能是个说话讲究、待人客气的门中大夫。
-- 他的反常要留到第 3 章回头看才成立，所以这里不写阴影、不写打量、不写笑意，
-- 他给出的理由句句都得站得住脚。玩家现在就觉得他可疑，第 3 章就没有落差了。
--
-- 攀爬玩法的全部回响集中在这里，只体现在他怎么评价这个孩子。
--
-- 他的年纪与体貌依原著 ch5：六十余岁、白发、久咳不止，门人称他「墨老」。
-- 久咳不是装饰，是第 3 章的伏笔，也是他挑孩子那套标准的世俗由头——
-- 走不动山路的老大夫要个身子结实、坐得住、记得住的孩子，句句都站得住。
-- 写成四十来岁的壮年人，这层伏笔就没了。

talk("", "ch01.modaifu.appear")
talk("qixuan_kaoguan", "ch01.modaifu.kg_greet")
talk("qixuan_kaoguan", "ch01.modaifu.kg_react")

talk("mo_daifu", "ch01.modaifu.mo_ask1")
talk("mo_daifu", "ch01.modaifu.mo_ask2")
talk("mo_daifu", "ch01.modaifu.mo_ask3")
talk("mo_daifu", "ch01.modaifu.mo_self")

-- ch01.climb_style 的取值：1=稳扎稳打，2=咬牙猛攀，3=借力歇息（见 data/flags.json）。
--
-- 主评语按「力竭 > 风格」选一条，登顶另给一句额外的赞许——设计文档第 3 节要的
-- 本来就是「一句额外的」，不是拿登顶把风格评语替换掉。
-- 顺序不能反：修正后的数值下力竭必然发生在崖顶，把登顶判在力竭前面，
-- echo_exhausted 就永远打不出来。162 条路径穷举确认过这一点。
if flag.get("ch01.climb_exhausted") == 1 then
    talk("mo_daifu", "ch01.modaifu.echo_exhausted")
elseif flag.get("ch01.climb_style") == 2 then
    talk("mo_daifu", "ch01.modaifu.echo_bold")
elseif flag.get("ch01.climb_style") == 3 then
    talk("mo_daifu", "ch01.modaifu.echo_careful")
else
    talk("mo_daifu", "ch01.modaifu.echo_steady")
end

if flag.get("ch01.climb_reached_top") == 1 then
    talk("mo_daifu", "ch01.modaifu.echo_top")
end

-- 拉过张铁一把的话，他会顺口提一句。只是顺口——绝不能让玩家以为
-- 「因为救人所以被选中」，否则第 3 章揭开他挑人的真实标准时会自相矛盾。
if flag.get("ch01.climb_helped") == 1 then
    talk("mo_daifu", "ch01.modaifu.echo_helped")
end

local pick = choice{
    "ch01.modaifu.opt_why",
    "ch01.modaifu.opt_thanks",
}

if pick == 1 then
    flag.set("ch01.modaifu_asked")
    talk("mo_daifu", "ch01.modaifu.why_reply")
    talk("mo_daifu", "ch01.modaifu.why_body")
    talk("mo_daifu", "ch01.modaifu.why_reply2")
    talk("", "ch01.modaifu.why_after")
else
    -- 只道谢（pick == 2）与取消（pick == nil）走同一条。
    talk("mo_daifu", "ch01.modaifu.thanks_reply")
    talk("", "ch01.modaifu.thanks_after")
end

talk("mo_daifu", "ch01.modaifu.mo_pair")

talk("han_sanshu", "ch01.modaifu.sanshu_thanks")
talk("mo_daifu", "ch01.modaifu.mo_stop")
talk("qixuan_kaoguan", "ch01.modaifu.kg_write")
talk("qixuan_kaoguan", "ch01.modaifu.kg_time")
talk("", "ch01.modaifu.close")

flag.set("ch01.shoutu_done")
