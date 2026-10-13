-- @hook ch11_fangshi npc_lian_er npc
-- Facts and participants: docs/ch11-design.md 13.1 n19.
if flag.get("ch11.lianer") ~= 0 then return end
talk("", "ch11.lianer.block")
talk("lian_er", "ch11.lianer.greet")
talk("", "ch11.lianer.charm")
talk("hanli", "ch11.lianer.cold")
talk("", "ch11.lianer.fan")
talk("fan_furen", "ch11.lianer.sorry")
talk("fan_furen", "ch11.lianer.invite")
talk("hanli", "ch11.lianer.accept")
flag.set("ch11.lianer")
