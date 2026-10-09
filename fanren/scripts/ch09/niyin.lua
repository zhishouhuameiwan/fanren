-- @hook ch09_milin trigger_niyin enter once
-- Facts and participants: docs/ch09-design.md 13.1.
talk("", "ch09.niyin.hide")
talk("", "ch09.niyin.white")
talk("", "ch09.niyin.pursuers")
talk("", "ch09.niyin.dong")
talk("mengmian_nvzi", "ch09.niyin.voice")
talk("", "ch09.niyin.likeness")
if flag.get("ch08.nangong") == 2 then
    talk("", "ch09.niyin.denial")
end
talk("", "ch09.niyin.rank")
talk("", "ch09.niyin.lure")
talk("", "ch09.niyin.needles")
talk("", "ch09.niyin.ebb")
flag.set("ch09.niyin")
