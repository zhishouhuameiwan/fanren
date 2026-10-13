-- @hook ch11_honghu trigger_buzhen enter once
-- Facts and participants: docs/ch11-design.md 13.1 n11. 布阵不发物品（A35）。
talk("", "ch11.buzhen.land")
talk("", "ch11.buzhen.survey")
talk("", "ch11.buzhen.three")
talk("", "ch11.buzhen.center")
talk("qu_hun_shadan", "ch11.buzhen.ready")
flag.set("ch11.buzhen")
