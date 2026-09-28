-- @hook ch05_kezhan trigger_anpai interact once
-- 第五章节点 11c「安排」。挂在 ch05_kezhan 上房八仙桌下两格 (6,6)(7,6)
-- （mode=interact，once=true，guard_flag=ch05.yange，set_flag=ch05.anpai）。
-- 他回客栈安排后路（ch124-125）。**寒毒检查点之二**（施工图 3.3）。
--
-- 原著 ch124-125：孙二狗已坐上帮主；席铁牛复述那一晚，想起女的说要先去太南谷；太南山在岚州最南，
-- 广贵城往西四十里；许席铁牛副帮主；给孙二狗解毒丹；把曲魂「长久寄放」在四平帮（ch125）。
-- 「太南谷」「太南山」本节首次出现（施工图 12.2：节点 1-10 不许有）。
-- 原著那「半日路」说的是他们当时离太南谷还有半日（ch125），台词照这个意思写（第 5 章校对 LOW-7）。
-- 2026-09-25 整改（校对 HIGH-1）：孙二狗与席铁牛同声叫出寺与山、韩立「换个帮主也不难」的敲打、
-- 「一南一北、多跑几趟」、给解毒丹的那番御下之术都不用了；解毒丹换了一个药师的理由（腐心丸吃久了伤肝）。
--
-- 孙二狗那一句「赏钱」按 ch05.qingbao 分两种（施工图第 7 节「孙二狗的忠心」）。
-- ch05.xiaoxiang == 2（潇湘院硬打）：孙二狗说天霸门的人来问过话——潇湘院是天霸门的产业（ch121）。
-- 二选一 → ch05.anpai：1 当场给解毒丹（原著）/ 2 按月给解药。取消按 2 算：缰绳照旧攥在手里。
-- 曲魂**不再 party.add**（施工图验收 11）；「跟上」那两个字这一回没有说。
-- 临出门孙二狗那句「药可带足了？」是吴剑鸣那一仗必败分支的提示二（施工图 8.4）。

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

talk("", "ch05.anpai.door")
talk("sun_ergou", "ch05.anpai.boss")

if flag.get("ch05.qingbao") == 1 then
    talk("sun_ergou", "ch05.anpai.sun_paid")
else
    talk("sun_ergou", "ch05.anpai.sun_unpaid")
end

talk("xi_tieniu", "ch05.anpai.xi")
talk("xi_tieniu", "ch05.anpai.xi2")
talk("", "ch05.anpai.where")
talk("", "ch05.anpai.han")

if flag.get("ch05.xiaoxiang") == 2 then
    talk("sun_ergou", "ch05.anpai.tian")
end

talk("", "ch05.anpai.vice")
talk("", "ch05.anpai.q")

local pick = choice{
    "ch05.anpai.opt_cure",
    "ch05.anpai.opt_month",
}

local anpai = 2
if pick == 1 then
    talk("", "ch05.anpai.cure")
    anpai = 1
else
    talk("", "ch05.anpai.month")
end

talk("", "ch05.anpai.qu")
talk("", "ch05.anpai.qu2")
talk("sun_ergou", "ch05.anpai.warn")
talk("", "ch05.anpai.alone")

flag.set("ch05.anpai", anpai)
