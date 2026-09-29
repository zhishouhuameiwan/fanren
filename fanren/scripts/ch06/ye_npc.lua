-- @hook ch06_huangfenggu npc_ye_shishu npc
-- 第六章百机堂内殿里的叶师叔（npc_ye_shishu，visible_flag=ch06.rangdan）。17b 之前只说一句（施工图 3.1 末段）；
-- 悔诺之后看 ch06.huinuo：忍了的那一支他和颜悦色，追问的那一支他爱理不理。不置旗标。

local huinuo = flag.get("ch06.huinuo")
if huinuo == 0 then
    talk("ye_shishu", "ch06.npc.ye.busy")
elseif huinuo == 2 then
    talk("ye_shishu", "ch06.npc.ye.cold")
else
    talk("ye_shishu", "ch06.npc.ye.warm")
end
