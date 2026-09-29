-- @hook ch06_sanxiu_lou npc_kusang npc
-- 第六章散修小楼一楼念经的苦桑（npc_kusang，全程在场）。节点 9a 制符之道那段问答之后可以再问（施工图 3.1 末段），
-- 这里说短版；更长的一段在路径行动·打探（kusang_dating）。不置旗标。
-- 议事之前分两段（复验 N-L7，裁决 16.5）：① 之前（ch06.xunxin 未置）一伙人都还在广场上，楼里只有他；
-- ① 那天傍晚众人都在二楼议事屋里，他才说「都在上头」。青纹一行的 NPC 在地图上也照这个时刻出场
-- （青纹 visible ch06.xunxin，其余 visible ch06.yishi；genmaps_ch06.py，复验 N-L1）。

if flag.get("ch06.zhifu") ~= 0 then
    talk("kusang", "ch06.npc.kusang.fu")
    return
end
if flag.get("ch06.yishi") == 0 then
    if flag.get("ch06.xunxin") == 0 then
        talk("kusang", "ch06.npc.kusang.out")
        return
    end
    talk("kusang", "ch06.npc.kusang.upstairs")
    return
end
talk("kusang", "ch06.npc.kusang.chant")
