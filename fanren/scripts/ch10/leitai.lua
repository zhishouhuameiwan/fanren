-- @hook ch10_kuixing trigger_leitai interact once
talk("", "ch10.leitai.hard")
talk("", "ch10.leitai.pick")
local removed = party.remove("kuilei_shou")
if not removed then talk("", "ch10.error.party"); return end
talk("", "ch10.leitai.puppet")
local won = battle("b10_gujia_bidou")
local restored = party.add("kuilei_shou")
if not restored then talk("", "ch10.error.party"); return end
if not won then talk("", "ch10.leitai.loss"); return end
talk("", "ch10.leitai.win")
talk("", "ch10.leitai.others")
talk("", "ch10.leitai.handover")
if flag.get("ch10.chouqian") == 2 then talk("", "ch10.leitai.read") end
talk("gu_dongzhu", "ch10.leitai.gu")
talk("hanli", "ch10.leitai.warn")
flag.set("ch10.leitai")
teleport("ch10_kuixing", 43, 26)
