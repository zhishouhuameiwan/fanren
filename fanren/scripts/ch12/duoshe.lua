-- @hook ch12_dixue trigger_shizhu enter once
-- Facts and participants: docs/ch12-design.md 13.1 n05. 3.2：take 画轴看返回值；party.remove 看返回值（契约第 4 节：false 时 talk 一句照走）；
-- 二选一（敲钟必落空、钟不扣）合流。抉择在扣画轴、离队之后，取消按「收剑」一支走，不 return，免得重踩时重演离队。
talk("", "ch12.duoshe.pillar")
talk("dixue_guiying", "ch12.duoshe.want")
if take("story_zhiyang_huazhou", 1) then
    talk("", "ch12.duoshe.burn")
else
    talk("", "ch12.duoshe.scroll_gone")
end
talk("jin_qing", "ch12.duoshe.jin")
talk("", "ch12.duoshe.jin_fall")
talk("dixue_guiying", "ch12.duoshe.ask")
talk("hanli", "ch12.duoshe.answer")
talk("", "ch12.duoshe.strike")
talk("", "ch12.duoshe.seized")
local ok = party.remove("qu_hun_shadan")
if not ok then talk("", "ch12.duoshe.not_in_party") end
local pick = choice({"ch12.duoshe.opt_bell", "ch12.duoshe.opt_hold"})
if pick == 1 then
    talk("", "ch12.duoshe.bell")
else
    talk("", "ch12.duoshe.hold")
end
talk("", "ch12.duoshe.shape")
talk("xuangu_shaonian", "ch12.duoshe.boast")
flag.set("ch12.duoshe")
