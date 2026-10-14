-- @hook ch12_tushan npc_wen_siyue npc
-- Facts and participants: docs/ch12-design.md 13.1 n07（地点改编为荒岛东岸，拍板 33）。不插手、不打。
-- kScriptTransfers 三元行：锦帕门闸、flag.set 与 teleport 都写在顶格（契约第 4 节、16.1 第 44 条）。
if flag.get("ch12.lidao") ~= 0 then return end
if item.count("story_jinpa_ditu") < 1 then talk("", "ch12.lidao.no_map"); return end
talk("", "ch12.lidao.shore")
talk("wen_siyue", "ch12.lidao.call")
talk("", "ch12.lidao.cut")
talk("wen_qiang_zhuji", "ch12.lidao.qiang")
talk("", "ch12.lidao.recog")
talk("wen_qiang_zhuji", "ch12.lidao.years")
talk("wen_qiang_zhuji", "ch12.lidao.trouble")
talk("", "ch12.lidao.hint")
talk("hanli", "ch12.lidao.reply")
talk("wen_siyue", "ch12.lidao.thanks")
talk("", "ch12.lidao.leave")
advance_days(4)
flag.set("ch12.lidao")
teleport("ch12_dadian")
