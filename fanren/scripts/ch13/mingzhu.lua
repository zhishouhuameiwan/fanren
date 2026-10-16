-- @hook ch13_waihai npc_ming_furen npc
-- Facts and participants: docs/ch13-design.md 13.1 段 14。3.2 n14：开头 ch13.daoqiyuan 未置 → 一句 return（不置旗，16.1 第 47 条）；
-- 首谈：choice「看下去 / 离开」，取消在置旗前 return；置 ch13.mingzhu；shop("ch13_mingzhu_shougou")。
-- **首谈分支写在再谈 / Z1 分支之前**（kOffChainFlags：flag.set("ch13.mingzhu") 先于 flag.set("ch13.z1")，第 11 节末表）。
-- 再谈：Z1（ch13.z1 未置、六级妖丹 >= 1 才列「交货」）→ take 1、give material_lingshi_zhong 30、置 ch13.z1；否则开收购。
-- 许云、易敬的对象站在祭坛下，本段不让他们在明珠轩里说话（一人一时一地）；炉鼎拍卖 0 处（13.1 段 14）。
if flag.get("ch13.daoqiyuan") == 0 then talk("ming_furen", "ch13.mingzhu.early"); return end
if flag.get("ch13.mingzhu") == 0 then
    talk("", "ch13.mingzhu.hall")
    talk("", "ch13.mingzhu.charm")
    talk("ming_furen", "ch13.mingzhu.trade")
    talk("hanli", "ch13.mingzhu.ask")
    talk("ming_furen", "ch13.mingzhu.abyss")
    talk("ming_furen", "ch13.mingzhu.invite")
    local pick = choice({"ch13.mingzhu.opt_watch", "ch13.mingzhu.opt_leave"})
    if not pick then return end
    if pick == 1 then
        talk("", "ch13.mingzhu.stone")
        talk("jingbao_paimaishi", "ch13.mingzhu.eggs")
        talk("", "ch13.mingzhu.bid")
    else
        talk("", "ch13.mingzhu.leave")
    end
    talk("ming_furen", "ch13.mingzhu.order")
    flag.set("ch13.mingzhu")
    shop("ch13_mingzhu_shougou")
    return
end
talk("ming_furen", "ch13.mingzhu.again")
if flag.get("ch13.z1") == 0 and item.count("material_yaodan_liuji") >= 1 then
    local pick = choice({"ch13.mingzhu.opt_deliver", "ch13.mingzhu.opt_sell"})
    if not pick then return end
    if pick == 1 then
        if take("material_yaodan_liuji", 1) then
            give("material_lingshi_zhong", 30)
            talk("ming_furen", "ch13.mingzhu.deliver")
            flag.set("ch13.z1")
        else
            talk("", "ch13.mingzhu.deliver_fail")
        end
        return
    end
end
shop("ch13_mingzhu_shougou")
