-- @hook ch13_waihai trigger_xuanshang interact
-- Z2 海兽悬赏（改编；docs/ch13-design.md 第 6 节、3.2 末）：链外、once 否、无 guard，脚本判旗。
-- ch13.shouchao 已置 → 一句 return；ch13.z2 已置 → 一句 return；未接（ch13.daoqiyuan 未置）→ 一句 return；
-- 五级妖丹 >= 3 才列「交」→ take 3（看返回值）、give material_lingshi_zhong 15、herb_nichang_cao 3、置 ch13.z2；不足一句。
-- 本脚本不置链上旗（kOffChainFlags 窗口 daoqiyuan → mingzhu）。文案编在 daoqiyuan（12.2）。
if flag.get("ch13.shouchao") ~= 0 then talk("", "ch13.xuanshang.ruin"); return end
if flag.get("ch13.z2") ~= 0 then talk("", "ch13.xuanshang.done"); return end
if flag.get("ch13.daoqiyuan") == 0 then talk("", "ch13.xuanshang.early"); return end
talk("", "ch13.xuanshang.notice")
if item.count("material_yaodan_wuji") < 3 then talk("", "ch13.xuanshang.short"); return end
local pick = choice({"ch13.xuanshang.opt_hand", "ch13.xuanshang.opt_later"})
if pick ~= 1 then talk("", "ch13.xuanshang.later"); return end
if not take("material_yaodan_wuji", 3) then talk("", "ch13.xuanshang.hand_fail"); return end
give("material_lingshi_zhong", 15)
give("herb_nichang_cao", 3)
talk("", "ch13.xuanshang.hand")
flag.set("ch13.z2")
