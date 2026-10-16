-- @hook ch13_wuzhong trigger_buzhen enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 23。3.2 n23：advance_days(30)（3.3 西飞一月）；置 ch13.buzhen。
-- 诱妖的阵只叫「法阵」「阵」（12.1 禁词）；养魂木雕珠归 Z3（yanghun.lua）。
talk("", "ch13.buzhen.wood")
talk("", "ch13.buzhen.abyss")
talk("", "ch13.buzhen.west")
advance_days(30)
talk("", "ch13.buzhen.array")
flag.set("ch13.buzhen")
