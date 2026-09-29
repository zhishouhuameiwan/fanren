-- @hook ch06_sanxiu_lou npc_qingwen_daoshi npc
-- 第六章散修小楼二楼走廊上的青纹道士（npc_qingwen_daoshi，visible_flag=ch06.xunxin）。① 那天傍晚他在议事屋门口
-- 等人；① 之前他跟大伙都在广场上，不在楼里（复验 N-L7，裁决 16.5：从前 ① 之前也站在门口说「人都齐了」，推门却
-- 什么也不发生）。议事之后是团伙的领头人，说话客气。拍肩撒粉是节点 11 之后的暗场，这里一个字也不露。不置旗标。

if flag.get("ch06.yishi") == 0 then
    talk("qingwen_daoshi", "ch06.npc.qingwen.wait")
    return
end
talk("qingwen_daoshi", "ch06.npc.qingwen.kind")
