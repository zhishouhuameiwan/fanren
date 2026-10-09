-- @hook ch07_dihuo trigger_banian interact once
-- 第七章节点 35c「半年」。挂在 ch07_dihuo 十九号圆墩旁的石凳 (24,2)
-- （mode=interact，once=true，guard_flag=ch07.feidan，set_flag=ch07.chengdan）。在鼎旁盘点、决定接着炼半年的是他。
--
-- 框架件（施工图 13.1 第 35c 段）：亲手成了第一颗；此后约三次成一次、取丹过半；辟谷丹一粒撑一月；丑汉那边从高兴到不安；半年得二十几颗。
-- **亲手炼出至少一颗才置旗标**：item.count("pill_zhuji_dan") ≥ 4（原有 3 ＋ 自炼 ≥ 1；筑基丹 tradeable false、不回血不回法，丢不掉）。
-- 否则一句「一颗也没成」；药粉用尽就再给 10 份（「储物袋里还备着一批」）return——**不设死局**（施工图 3.2 节点 35c）。
-- 往下：advance_days(180)；药粉余量 take 光；**筑基丹补足到 25**（give 差额，差额 ≤ 0 不给；第 9 节第 2 条）。

if item.count("pill_zhuji_dan") < 4 then
    if item.count("material_zhuji_yaofen") == 0 then
        give("material_zhuji_yaofen", 10)
        talk("", "ch07.banian.refill")
    else
        talk("", "ch07.banian.notyet")
    end
    return
end

talk("", "ch07.banian.first")
talk("", "ch07.banian.go_on")

advance_days(180)

talk("", "ch07.banian.rate")
talk("", "ch07.banian.bigu")
talk("", "ch07.banian.chouhan")
local rest = item.count("material_zhuji_yaofen")
if rest > 0 then
    take("material_zhuji_yaofen", rest)
end
local need = 25 - item.count("pill_zhuji_dan")
if need > 0 then
    give("pill_zhuji_dan", need)
end
talk("", "ch07.banian.count")
talk("", "ch07.banian.here")

flag.set("ch07.chengdan")
