-- @hook ch11_fangshi npc_zi_ling npc
-- Facts and participants: docs/ch11-design.md 13.1 n25（B23：地点改在茶馆雅间）。
if flag.get("ch11.zhanglao") ~= 0 then return end
talk("", "ch11.zhanglao.room")
talk("zi_ling_zhenrong", "ch11.zhanglao.face")
talk("zi_ling_zhenrong", "ch11.zhanglao.seen")
talk("qu_hun_shadan", "ch11.zhanglao.qu")
talk("zi_ling_zhenrong", "ch11.zhanglao.bamboo")
talk("zi_ling_zhenrong", "ch11.zhanglao.offer")
talk("zi_ling_zhenrong", "ch11.zhanglao.fan")
talk("hanli", "ch11.zhanglao.refuse")
talk("zhuo_ruting", "ch11.zhanglao.zhuo")
talk("hanli", "ch11.zhanglao.accept")
give("material_tianleizhu", 1)
give("story_miaoyin_yaopai", 1)
talk("", "ch11.zhanglao.take")
flag.set("ch11.zhanglao")
