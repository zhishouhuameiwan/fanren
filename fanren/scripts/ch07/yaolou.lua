-- @hook ch06_baiyaoyuan trigger_yaolou interact
-- 第七章节点 2、3 与支线 Z1「药篓」。挂在 ch06_baiyaoyuan 茅屋前的药篓 (23,8)
-- （mode=interact，once=false，guard_flag=ch07.chaibao，无 set_flag——地图规范 4.4）。
-- 把药摆进篓里、等马师伯来取，是他做的。一处药篓管三次交药，脚本看旗标决定这一回收什么：
--   节点 2（ch07.jiaoyao1 未置）：三株 ≥ 44 年的黄精 → 一年月例 24（每月两块）；置 ch07.jiaoyao1。
--   节点 3（ch07.danbao 未置）：两株 ≥ 100 年的紫参 → 一年月例 60（每月五块）；realm.advance(11)；
--     问方、岳麓殿第一次出口（施工图 12.2）；担保玉符；置 ch07.danbao 与 ch07.baigong_qiu（支线 Z1 接取）。
--   Z1（接了、没交、马师伯还没送药）：一株 ≥ 176 年的血红芝，置 ch07.baigong；月例这一年本来就没有。
--   都交过了：一句「这一季的药交过了」。
-- 账按年份下限算（第 17 节第 23 条）：一律 item.count_aged / take_aged（契约 4.1），不猜玩家浇到了几年。
-- 不够就说还差几株、return；药不会凭空少（这一段之前没有商店、没有仗），种下去、坐一阵、浇几滴总走得到。

if flag.get("ch07.jiaoyao1") == 0 then
    local have = item.count_aged("herb_huangjing_cao", 44)
    if have < 3 then
        if have == 2 then
            talk("", "ch07.yaolou.hj_less1")
        elseif have == 1 then
            talk("", "ch07.yaolou.hj_less2")
        else
            talk("", "ch07.yaolou.hj_less3")
        end
        return
    end
    if not take_aged("herb_huangjing_cao", 3, 44) then
        talk("", "ch07.yaolou.hj_less3")
        return
    end
    talk("", "ch07.yaolou.y1_basket")
    talk("", "ch07.yaolou.y1_ma")
    talk("ma_shibo", "ch07.yaolou.y1_ma_ok")
    talk("", "ch07.yaolou.y1_pay")
    give("material_lingshi", 24)
    talk("ma_shibo", "ch07.yaolou.y1_news")
    talk("ma_shibo", "ch07.yaolou.y1_murong")
    talk("", "ch07.yaolou.y1_self")
    flag.set("ch07.jiaoyao1")
    return
end

if flag.get("ch07.danbao") == 0 then
    local have = item.count_aged("herb_zishen_cao", 100)
    if have < 2 then
        if have == 1 then
            talk("", "ch07.yaolou.zs_less1")
        else
            talk("", "ch07.yaolou.zs_less2")
        end
        return
    end
    if not take_aged("herb_zishen_cao", 2, 100) then
        talk("", "ch07.yaolou.zs_less2")
        return
    end
    talk("", "ch07.yaolou.y2_basket")
    talk("", "ch07.yaolou.y2_ma")
    talk("", "ch07.yaolou.y2_pay")
    give("material_lingshi", 60)
    talk("", "ch07.yaolou.y2_pills")
    local ok = realm.advance(11)
    if ok then
        talk("", "ch07.yaolou.y2_level")
    end
    talk("", "ch07.yaolou.y2_useless")
    talk("", "ch07.yaolou.y2_nephew")
    talk("", "ch07.yaolou.y2_ask")
    talk("ma_shibo", "ch07.yaolou.y2_ma_no")
    talk("ma_shibo", "ch07.yaolou.y2_ma_where")
    talk("", "ch07.yaolou.y2_look")
    talk("ma_shibo", "ch07.yaolou.y2_ma_price")
    talk("", "ch07.yaolou.y2_count")
    talk("", "ch07.yaolou.y2_yufu")
    talk("ma_shibo", "ch07.yaolou.y2_z1")
    flag.set("ch07.baigong_qiu")
    flag.set("ch07.danbao")
    return
end

if flag.get("ch07.baigong_qiu") ~= 0 and flag.get("ch07.baigong") == 0 and flag.get("ch07.ma_songyao") == 0 then
    if item.count_aged("herb_xuehong_zhi", 176) < 1 then
        talk("", "ch07.yaolou.z1_less")
        return
    end
    if not take_aged("herb_xuehong_zhi", 1, 176) then
        talk("", "ch07.yaolou.z1_less")
        return
    end
    talk("", "ch07.yaolou.z1_basket")
    talk("", "ch07.yaolou.z1_ma")
    talk("ma_shibo", "ch07.yaolou.z1_ma_say")
    flag.set("ch07.baigong")
    return
end

talk("", "ch07.yaolou.done")
