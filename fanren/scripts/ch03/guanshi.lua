-- @hook ch02_yaopu npc_yaopu_guanshi_ch03 npc
-- 药圃管事的闲聊（第 3 章版本）。挂在 ch02_yaopu 的 npc_yaopu_guanshi_ch03 上
-- （role_id=yaopu_guanshi，visible_flag=ch03.mo_gui_gu）。
-- 这一段从前写的是「**关卡侧尚未给挂点**」——那句话早已过时，关卡侧挂好了没人回来改。
-- 同格的第 2 章版本 npc_yaopu_guanshi 带 hidden_flag=ch03.mo_gui_gu，两个人按章交棒。
-- npc 对象没有 mode，也没有 once：首行的 @hook 因此只写 npc 一个取值。
--
-- 管事在第 2 章是「只讲规矩不讲道理」的那种人（该浇的时候浇、该等的时候等）。
-- 本章他一句规矩也没多讲，讲的是**别问**：
--   「谷里的事，能不问的都别问，这是他在这儿待了十几年学会的。」
-- 这条与韩立正在做的事恰好相反——他这一年干的每一件事都是在问——
-- 于是管事越是劝，玩家越明白韩立已经回不了头。
--
-- 他还顺口交待了东头那两畦今年不派人、谁要用谁自己起，
-- 这一句是节点 7 备毒那次二选一的引信；说完他瞥了韩立一眼，
-- 说这话不是说给你一个人听的——把「他记得住谁动过东西」也一并立住了。
--
-- 分段用的是本章已登记的旗标，**没有另立**：备毒之前是那三条，
-- 备毒之后换成 again 那一条。编剧不得往 data/flags.json 里加旗标，
-- 所以这里宁可复用一个语义相近的，也不自造一个 ch03.liao_guanshi。
--
-- 脚本无从得知是哪个 NPC 触发了它，所以药圃里若还有别人说话，各挂各的脚本。
-- 三条路都有话说，没有一条是静默返回。

if flag.get("ch03.mo_gui_gu") ~= 1 then
    talk("yaopu_guanshi", "ch03.npc.guanshi_pre")
    return
end

if flag.get("ch03.beidu_done") == 1 then
    talk("yaopu_guanshi", "ch03.npc.guanshi_again")
    return
end

talk("yaopu_guanshi", "ch03.npc.guanshi_1")
talk("yaopu_guanshi", "ch03.npc.guanshi_2")
talk("yaopu_guanshi", "ch03.npc.guanshi_3")
