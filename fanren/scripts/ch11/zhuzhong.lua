-- @hook ch11_dongfu npc_jin_qing npc
-- Facts and participants: docs/ch11-design.md 13.1 n31. 章末停在递珠：不 ending()，不写去向。
if flag.get("ch11.done") ~= 0 then return end  -- 金青在 ch11.done 后离场（hidden_flag），这一句只防重入
talk("", "ch11.zhuzhong.eggs")
talk("", "ch11.zhuzhong.visitor")
talk("jin_qing", "ch11.zhuzhong.greet")
talk("jin_qing", "ch11.zhuzhong.news")
talk("jin_qing", "ch11.zhuzhong.rumor")
talk("hanli", "ch11.zhuzhong.doubt")
talk("jin_qing", "ch11.zhuzhong.dan")
talk("jin_qing", "ch11.zhuzhong.pearl")
talk("jin_qing", "ch11.zhuzhong.ask")
give("story_bai_zhenzhu", 1)
talk("", "ch11.zhuzhong.hand")
flag.set("ch11.done")
