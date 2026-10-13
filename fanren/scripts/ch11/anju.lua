-- @hook ch11_dongfu trigger_anju enter once
-- Facts and participants: docs/ch11-design.md 13.1 n05.
talk("", "ch11.anju.door")
talk("", "ch11.anju.array")
talk("", "ch11.anju.worms")
talk("", "ch11.anju.speed")
field.unlock("field_tianxing_39", 6)
talk("", "ch11.anju.garden")
talk("qu_hun_shadan", "ch11.anju.qu")
if flag.get("ch11.luer") == 1 then
    talk("", "ch11.anju.luer_go")
else
    talk("", "ch11.anju.settle")
end
flag.set("ch11.anju")
