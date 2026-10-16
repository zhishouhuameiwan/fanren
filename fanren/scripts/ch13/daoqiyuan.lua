-- @hook ch13_waihai trigger_shiwu enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 13。3.2 n13：读 ch13.toudu 择一句（易敬看他的眼神）；置 ch13.daoqiyuan。
-- 落点 shiwu 在石屋内、不触发 enter：这条带在屋门外，文案从屋里那几句说到门外。
-- 许云、易敬的对象自 ch13.chuansong 起就站在祭坛下（5.1），本段就在那里说话；许云的导览拆开讲
-- （共管、禁斗、元婴在此；海渊在 mingzhu、兽卵在竞宝厅，13 第 6 条）。「易敬」「许云」「黑石城」自本段起（12.2）。
talk("", "ch13.daoqiyuan.room")
talk("qiyuan_kanhu", "ch13.daoqiyuan.oneway")
talk("", "ch13.daoqiyuan.ask")
talk("yi_jing", "ch13.daoqiyuan.trade")
talk("xu_yun", "ch13.daoqiyuan.xuyun")
local toudu = flag.get("ch13.toudu")
if toudu == 1 then
    talk("", "ch13.daoqiyuan.look_stone")
elseif toudu == 2 then
    talk("", "ch13.daoqiyuan.look_dan")
else
    talk("", "ch13.daoqiyuan.look_blade")
end
talk("", "ch13.daoqiyuan.city")
talk("xu_yun", "ch13.daoqiyuan.rule")
talk("xu_yun", "ch13.daoqiyuan.law")
talk("", "ch13.daoqiyuan.listen")
flag.set("ch13.daoqiyuan")
