-- @hook ch05_dukou npc_chuanjia npc
-- 第五章渡口的船家（npc_chuanjia，草棚门口）。闲话，不推进任何东西，不置旗标。
-- 节点 1a 没打完（逃了那一仗、峡口的狼还在）时他说天黑不开船；打完了，他等着客人上船。
-- 上了船人就到了西城，这张图回不来，所以没有第三句。

if flag.get("ch05.kaipian") == 0 then
    talk("chuanjia", "ch05.npc.chuanjia.night")
    return
end

talk("chuanjia", "ch05.npc.chuanjia.ready")
