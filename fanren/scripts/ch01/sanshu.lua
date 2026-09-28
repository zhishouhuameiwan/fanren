-- @hook ch01_hanjiacun npc_han_sanshu npc
-- 第一章「三叔引荐」。韩立十岁，三叔上门提议带他去七玄门参加收徒考核。
--
-- 垂直切片用：一段对话 → 一次二选一 → 按选择走不同分支 → 设旗标 → 结束。
-- 文案一律引 key，实际文字在 data/text/ch01.json；说话人用 data/roles/ 里的 id。
-- 这里出现任何中文字面量都是错的：翻译与润色不该回头改脚本。
--
-- 只演一回：谈完（ch01.sanshu_met）他当晚就回去了，这个 npc 随即不在场
-- （地图上 hidden_flag=ch01.sanshu_met），天亮他在村口老槐树下（npc_han_sanshu_cunkou）。

talk("han_sanshu", "ch01.sanshu.greet")

local pick = choice{
    "ch01.sanshu.opt_yes",
    "ch01.sanshu.opt_no",
}

if pick == 1 then
    -- 当场答应。这一笔留进存档，节点 2 母亲转述三叔那句话的口吻就跟着变。
    flag.set("ch01.sanshu_accepted")
    talk("han_sanshu", "ch01.sanshu.accept")
else
    -- 迟疑（pick == 2）与取消（pick == nil）走同一条：三叔不勉强，但话没说死，
    -- 剧情后续仍要推进到收徒考核。
    talk("han_sanshu", "ch01.sanshu.decline")
end

-- 收尾把玩家推向村口：节点 2 在那里等着，走过去就是本作第一次自己走路。
talk("", "ch01.sanshu.hint_move")

-- 两条分支都算「谈过」。ch01.sanshu_accepted 记的是答没答应，已随本章其余
-- 旗标一并登记进 data/flags.json，上面那条待协调项就此了结。
-- 放在最后一句之后：村里的三叔、韩母按这个旗标撤场（天亮后换到村口老槐树下），
-- 先置的话，上面那句旁白还在屏上，两个人就先不见了。
flag.set("ch01.sanshu_met")
