-- @hook ch11_dongfu npc_jiedan_linju_b npc
-- Facts and participants: docs/ch11-design.md 13.1 Z2.
if flag.get("ch11.z2") ~= 0 then talk("jiedan_linju_yi", "ch11.z2.again"); return end
talk("jiedan_linju_yi", "ch11.z2.greet")
talk("jiedan_linju_yi", "ch11.z2.want")
if item.count("pill_dingyan_dan") < 1 then talk("hanli", "ch11.z2.none"); return end
local pick = choice({"ch11.z2.opt_give", "ch11.z2.opt_keep"})
if pick ~= 1 then talk("hanli", "ch11.z2.keep"); return end
if not take("pill_dingyan_dan", 1) then talk("", "ch11.error.take"); return end
give("material_lianqi_yao", 3)
talk("jiedan_linju_yi", "ch11.z2.thanks")
flag.set("ch11.z2", 1)
