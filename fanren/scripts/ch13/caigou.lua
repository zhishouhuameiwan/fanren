-- @hook ch13_nanming npc_yuanliao_zhanggui npc
-- Facts and participants: docs/ch13-design.md 13.1 段 2。3.2 n02：前置 ch13.wenxun 由脚本开头判（未到一句 return）；
-- 首谈置 ch13.caigou、shop("ch13_nanming_fangshi")；再谈：ch13.gongdao 未置 → 再开店，已置 → 一句「封市」return（7.1）。
if flag.get("ch13.wenxun") == 0 then talk("", "ch13.caigou.busy"); return end
if flag.get("ch13.caigou") ~= 0 then
    if flag.get("ch13.gongdao") ~= 0 then talk("", "ch13.caigou.closed"); return end
    talk("nanming_zhanggui", "ch13.caigou.again")
    shop("ch13_nanming_fangshi")
    return
end
talk("", "ch13.caigou.market")
talk("hanli", "ch13.caigou.ask")
talk("nanming_zhanggui", "ch13.caigou.unknown")
talk("nanming_zhanggui", "ch13.caigou.hint")
talk("", "ch13.caigou.stock")
flag.set("ch13.caigou")
shop("ch13_nanming_fangshi")
