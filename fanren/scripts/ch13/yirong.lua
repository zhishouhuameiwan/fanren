-- @hook ch13_tianxing trigger_lanlu enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 11。3.2 n11：choice「直说同去 / 先亮把柄」，取消在置旗前 return；
-- advance_days(1)（3.3 第三日清晨）写在抉择之后，免得取消再踏时重复拨日；置 ch13.yirong。
-- 化名「曲」自本段起、到 cunjin 为止（12.2、16.1 第 43 条）；只在他自称与他人口中，名牌不变。
talk("", "ch13.yirong.dawn")
talk("", "ch13.yirong.meet")
talk("yi_jing", "ch13.yirong.who")
local pick = choice({"ch13.yirong.opt_direct", "ch13.yirong.opt_leverage"})
if not pick then return end
if pick == 1 then
    talk("hanli", "ch13.yirong.direct")
else
    talk("hanli", "ch13.yirong.leverage")
end
talk("", "ch13.yirong.threat")
talk("hanli", "ch13.yirong.shout")
talk("yi_jing", "ch13.yirong.agree")
talk("hanli", "ch13.yirong.enough")
advance_days(1)
talk("", "ch13.yirong.march")
flag.set("ch13.yirong")
