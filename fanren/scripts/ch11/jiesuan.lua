-- @hook ch11_xiaoxuan trigger_jiesuan interact
-- Facts and participants: docs/ch11-design.md 13.1 n28. 回访只是结算（E22）：每满 30 日 15 低阶，一次上限 240 个月。
if flag.get("ch11.liugen") >= 1 then talk("", "ch11.jiesuan.closed"); return end
local n = math.floor((today() - flag.get("ch11.jiesuan_ri")) / 30)
if n > 240 then n = 240 end
if n <= 0 then talk("", "ch11.jiesuan.none"); return end
give("material_lingshi", 15 * n)
flag.set("ch11.jiesuan_ri", today())
local times = flag.get("ch11.jiesuan") + 1
flag.set("ch11.jiesuan", times)
if times == 1 then
    talk("", "ch11.jiesuan.book")
    talk("he_xianshi", "ch11.jiesuan.visit")
    talk("", "ch11.jiesuan.first")
else
    talk("", "ch11.jiesuan.sum")
end
if today() - flag.get("ch11.kaidian_ri") >= 3240 then talk("", "ch11.jiesuan.ninth") end
