-- @hook ch11_honghu trigger_youyao interact
-- Facts and participants: docs/ch11-design.md 13.1 n12、n13.
-- 循环挂点（once=false）：缺草不扣、缺绿液只拨日；扣草在 battle 返回之后，逃不扣、败 game_over。
if item.count("herb_nichang_cao") < 1 then talk("", "ch11.youyao.no_herb"); return end
if bottle.drops() < 1 then talk("", "ch11.youyao.no_drop"); advance_days(5); return end
if not bottle.spend(1) then talk("", "ch11.youyao.no_drop"); advance_days(5); return end
talk("", "ch11.youyao.drip")
advance_days(3)
if flag.get("ch11.lieyao") == 4 and flag.get("ch11.jiguan") == 0 then
    talk("", "ch11.jiguan.old")
    talk("", "ch11.jiguan.come")
    talk("qu_hun_shadan", "ch11.jiguan.warn")
    local won2 = battle("b11_honghu_jiguanjiao")
    flag.set("ch11.jiguan")
    if won2 then
        if not take("herb_nichang_cao", 1) then talk("", "ch11.error.take"); return end
        talk("", "ch11.jiguan.win")
    else
        talk("", "ch11.jiguan.gone")
    end
    talk("", "ch11.jiguan.after")
    return
end
talk("", "ch11.youyao.scent")
local won, how = battle("b11_ningcuidao_youyao")
if not won and how == "escaped" then talk("", "ch11.youyao.escaped"); return end
if not won then game_over(); return end
if not take("herb_nichang_cao", 1) then talk("", "ch11.error.take"); return end
talk("", "ch11.youyao.dan")
local lieyao = flag.get("ch11.lieyao") + 1
flag.set("ch11.lieyao", lieyao)
if lieyao == 1 then talk("qu_hun_shadan", "ch11.youyao.first") end
