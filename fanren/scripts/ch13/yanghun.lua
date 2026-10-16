-- @hook ch13_wuzhong trigger_yanghun interact
-- Z3 养魂木珠（docs/ch13-design.md 13.1 末行、第 6 节、3.2 末）：链外、once 否、无 guard，脚本判旗。
-- ch13.buzhen 未置 → 一句（木未成形）return；ch13.z3 已置 → 一句 return；
-- 先判根须、再判绿液：根须 >= 1 且 bottle.drops >= 3 → bottle.spend(3)（看返回值）、advance_days(60)、
-- take story_yanghun_mugen 1（看返回值）、give story_yanghun_zhu 1、置 ch13.z3；不足一句。ch13.lidao 后不可达（不另判）。
if flag.get("ch13.buzhen") == 0 then talk("", "ch13.yanghun.young"); return end
if flag.get("ch13.z3") ~= 0 then talk("", "ch13.yanghun.done"); return end
if item.count("story_yanghun_mugen") < 1 then talk("", "ch13.yanghun.no_root"); return end
if bottle.drops() < 3 then talk("", "ch13.yanghun.no_drop"); return end
if not bottle.spend(3) then talk("", "ch13.yanghun.no_drop"); return end
advance_days(60)
talk("", "ch13.yanghun.grow")
if take("story_yanghun_mugen", 1) then
    talk("", "ch13.yanghun.carve")
else
    talk("", "ch13.yanghun.carve_fail")
end
give("story_yanghun_zhu", 1)
flag.set("ch13.z3")
