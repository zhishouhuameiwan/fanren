-- @hook ch05_mofu trigger_huanyu interact once
-- 第五章节点 12e「换玉」，**章末**。挂在 ch05_mofu 小楼楼门右格 (32,9)（mode=interact，once=true，
-- guard_flag=ch05.cisha，set_flag=ch05.done）。他提着首级回来交换。**寒毒检查点之五，也是最后一个**：
-- 宝玉到手即停表（施工图 3.3）。除毒是第 6 章开头半个月的事（ch126），本章不演。
--
-- 原著 ch126 开篇追叙：交首级给严氏检验；严氏说破欧阳飞天练的是霸王甲，刀枪不入——他这才明白欧阳飞天
-- 是把剑符当成了暗器才没躲；换解药、拿宝玉；谢绝挽留，离城。
--   ch05.jiaoyi == 1（11a 请严氏吞了药）→ 给解药。
--   「当成暗器」那一句人人都说：能拿着首级回来的，都是用剑符取的（火弹要不了他的命，
--   第 5 章校对 MEDIUM-1 的拍板；从前火弹那一句 huanyu.fire 已删）。
--   吴剑鸣总会被擒（12b，MEDIUM-2）→ 严氏一句「还关在后院」；他的下场仍然不写（施工图 17 第 13 条）。
--   ch05.yange（11b 赢 / 输或走）→ 出府时燕歌在不在门口。
--   ch05.fengwu_yigao（支线 Z1 交了）→ 墨凤舞托人送一包药出来。
-- 暖阳宝玉：本章给出的最后一样东西。
-- **章末不给志向**（沿用第 3、4 章的口径）：落在一笔账上。
-- **不换图、不收结局**：第 6 章从哪儿开局归关卡侧与主控定。

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

talk("", "ch05.huanyu.head")
talk("yan_shi", "ch05.huanyu.jia")

talk("", "ch05.huanyu.fu")

if flag.get("ch05.jiaoyi") == 1 then
    talk("", "ch05.huanyu.antidote")
end

talk("", "ch05.huanyu.jade")
give("story_nuanyang_baoyu", 1)

talk("yan_shi", "ch05.huanyu.wu_caught")

talk("yan_shi", "ch05.huanyu.stay")

if flag.get("ch05.fengwu_yigao") ~= 0 then
    talk("", "ch05.huanyu.feng")
end

if flag.get("ch05.yange") == 1 then
    talk("", "ch05.huanyu.yan_door")
else
    talk("", "ch05.huanyu.yan_none")
end

talk("", "ch05.huanyu.go")
talk("", "ch05.huanyu.end")

flag.set("ch05.done")
