-- @hook ch11_xingkong npc_xingkong_xiushi npc
-- Facts and participants: docs/ch11-design.md 13.1 n09.
-- kScriptTransfers 三元行：flag.set 与 teleport 都写在顶格。
if flag.get("ch11.huicheng") >= 1 then talk("xingkong_xiushi", "ch11.xingkong.closed"); return end
local first = flag.get("ch11.chuansong") == 0
if first then
    talk("", "ch11.xingkong.hall")
    talk("xingkong_xiushi", "ch11.xingkong.greet")
    talk("xingkong_xiushi", "ch11.xingkong.fee")
    talk("", "ch11.xingkong.group")
    talk("yaoyan_furen", "ch11.xingkong.share")
    talk("qu_hun_shadan", "ch11.xingkong.agree")
    if take("material_lingshi_zhong", 2) then
        talk("", "ch11.xingkong.paid_first")
    else
        talk("", "ch11.xingkong.covered")
    end
    talk("xingkong_xiushi", "ch11.xingkong.fu")
else
    talk("xingkong_xiushi", "ch11.xingkong.again")
    if take("material_lingshi_zhong", 7) then
        talk("", "ch11.xingkong.paid")
    else
        talk("", "ch11.xingkong.ride")
    end
end
talk("", "ch11.xingkong.light")
flag.set("ch11.chuansong")
teleport("ch11_ningcui_jie")
