-- @hook ch06_huangfenggu trigger_zawu interact once
-- 第六章节点 17a「百机堂：杂务」。挂在 ch06_huangfenggu 百机堂的柜台 (6,12)
-- （mode=interact，once=true，guard_flag=ch06.wufeng，set_flag=ch06.zawu）。
-- 一个月的熟悉期都不等、当天就来领活的是他（ch149）。
--
-- 于执事：怎么这么快；按规矩先熟悉一个月；分到什么干什么，不许挑。叶师叔从身后出来：
--   ch06.rangdan == 1 → 让韩师侄随意挑；== 2 → 今年种植的活本来要抓签，他来得巧，正缺人（他没许过杂务任选）。
-- 竹简**四选一**：五花树、火云参、月梅草、接管青石岭百药园。前三个于执事各回一句，然后回到竹简——
-- 只有百药园走得下去（施工图 3.2 节点 17：不是真的四选一，是让玩家看一眼那张竹简）。
-- 竹简上的活计取一部分、打乱顺序、换数字（第 13 节第 6 条）。取消：他把竹简卷起来，return，不置旗标。

talk("", "ch06.zawu.fly")
talk("yu_zhishi", "ch06.zawu.fast")
talk("yu_zhishi", "ch06.zawu.rule")
talk("", "ch06.zawu.voice")
if flag.get("ch06.rangdan") == 2 then
    talk("ye_shishu", "ch06.zawu.lottery")
else
    talk("ye_shishu", "ch06.zawu.pick")
end
talk("", "ch06.zawu.slips")

while true do
    local pick = choice{
        "ch06.zawu.opt_tree",
        "ch06.zawu.opt_shen",
        "ch06.zawu.opt_mei",
        "ch06.zawu.opt_yuan",
    }
    if pick == nil then
        talk("", "ch06.zawu.roll")
        return
    end
    if pick == 4 then
        break
    end
    if pick == 1 then
        talk("yu_zhishi", "ch06.zawu.no_tree")
    elseif pick == 2 then
        talk("yu_zhishi", "ch06.zawu.no_shen")
    else
        talk("yu_zhishi", "ch06.zawu.no_mei")
    end
end

talk("yu_zhishi", "ch06.zawu.yuan")
talk("ye_shishu", "ch06.zawu.hard")   -- 这是叶师叔的话，名签跟着他（校对 LOW-1）
talk("ye_shishu", "ch06.zawu.inner")

flag.set("ch06.zawu")
