-- @hook ch05_mofu trigger_jianmianli enter once
-- 第五章节点 7b「见面礼」。挂在 ch05_mofu 小楼通后宅厢房那段夹道靠小楼的一格 (42,9)
-- （mode=enter，once=true，guard_flag=ch05.dengmen，set_flag=ch05.jianmianli）。
-- 墨彩环半路站住不走了——是她拦他（ch112-113），所以是踏入型。
-- 靠厢房的那一格 (42,8) 是节点 8（前后相邻、不同格；两个 enter 叠在一格上，排后面的永远轮不到）。
--
-- **转写高风险点 5**：ch113 墨彩环的「三板斧」与萦香丸的来历——留意象，不留条目。
-- 2026-09-25 整改（第 5 章校对 HIGH-1）：三板斧、他想起谁、她疑心是迷药、拿去给二姐验，一样不留。
-- 现在是她伸出五根指头、一根一根往回收着讨价，他一只手按着钱袋不挪；她闻了萦香丸先问能卖多少钱
-- （语气卡：财迷、看人准）。萦香丸这个名字要出现：节点 11a 墨凤舞会问起它。
--
-- 三选一 → ch05.jianmianli（节点 11a 墨凤舞开口先问什么，就看这一位）：
--   1 药箱里那只绿瓷瓶（萦香丸，原著；**不做成物品**）
--   2 一袋碎银（take(20)，**钱不够就不列这一项**——施工图 3.2 原话）
--   3 什么也不给。取消按 3 算。
-- 末尾宿厢房：拨一天，把人放到厢房床前 (40,3)（genmaps_ch05.py 的 MOFU_BED_FRONT）。
-- 第二天他出门必踩夹道那一格，节点 8 就从那里起。

talk("mo_caihuan", "ch05.jianmian.stop")
talk("mo_caihuan", "ch05.jianmian.ask")
talk("", "ch05.jianmian.purse")
talk("", "ch05.jianmian.stand")
talk("", "ch05.jianmian.q")

-- 不够一袋，第二项就不出现（两张选项表都写成字面量，门禁才查得到每一个 key）。
local jianmian = 3
if item.count("material_lingshi") >= 20 then
    local pick = choice{
        "ch05.jianmian.opt_pill",
        "ch05.jianmian.opt_silver",
        "ch05.jianmian.opt_none",
    }
    if pick == 1 then
        jianmian = 1
    elseif pick == 2 then
        jianmian = 2
    end
else
    local pick = choice{
        "ch05.jianmian.opt_pill",
        "ch05.jianmian.opt_none",
    }
    if pick == 1 then
        jianmian = 1
    end
end

if jianmian == 1 then
    talk("", "ch05.jianmian.pill1")
    talk("mo_caihuan", "ch05.jianmian.pill2")
elseif jianmian == 2 then
    if take("material_lingshi", 20) then
        talk("", "ch05.jianmian.silver")
    else
        -- 列出来的时候够，扣的时候不够——这一支在正常流程里走不到（中间没有花钱的地方）。
        -- 真走到了，钱没给出去，就按什么也没给记账。
        talk("", "ch05.jianmian.none")
        jianmian = 3
    end
else
    talk("", "ch05.jianmian.none")
end

talk("", "ch05.jianmian.room")

advance_days(1)

flag.set("ch05.jianmianli", jianmian)
teleport("ch05_mofu", 40, 3)
