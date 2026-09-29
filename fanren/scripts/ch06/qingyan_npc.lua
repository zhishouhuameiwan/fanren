-- @hook ch06_tainan_gu npc_qingyan_zhenren npc
-- 第六章太南谷楼阁前的青颜真人（npc_qingyan_zhenren，全程在场）。主办者之一；节点 3 那一眼之后，
-- 他对韩立客气而冷淡。节点 6 叶家寻衅是他叫停的；散会也是他们几位前辈宣布的。不置旗标。

if flag.get("ch06.xunxin") ~= 0 then
    talk("qingyan_zhenren", "ch06.npc.qingyan.after")
    return
end

talk("qingyan_zhenren", "ch06.npc.qingyan.cool")
