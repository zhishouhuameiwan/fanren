-- @hook ch13_tianxing trigger_xingkong enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 9。3.2 n09：advance_days(1)（3.3 次日）；置 ch13.tanzhen。
-- 张姓修士的同伴不建 role，只在旁白里转述；两句开价的原话不用（13.1 段 9）。
advance_days(1)
talk("", "ch13.tanzhen.day")
talk("", "ch13.tanzhen.probe")
talk("", "ch13.tanzhen.ward")
talk("", "ch13.tanzhen.overhear")
talk("", "ch13.tanzhen.three")
talk("zhang_xiushi", "ch13.tanzhen.five")
talk("", "ch13.tanzhen.rule")
talk("zhang_xiushi", "ch13.tanzhen.years")
talk("", "ch13.tanzhen.part")
flag.set("ch13.tanzhen")
