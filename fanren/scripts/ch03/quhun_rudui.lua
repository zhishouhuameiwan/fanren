-- @hook ch03_andao trigger_quhun interact once
-- 第三章节点 12 之二「曲魂入队」。第一个可控同伴。
-- 挂在 ch03_andao 石室西壁石棺旁 (10,20) 起两格、那人站着的位置
-- （trigger，mode=interact，once=true，set_flag=ch03.quhun_rudui）。
--
-- **独立校对 MEDIUM-2 裁决过一次：地图是对的，错的是文案，所以改的是文案。**
-- ch03.quhun.hood 从前写的是「他**走出去**，站在那人面前，把帽兜掀了」——
-- 「走出去」指的是走出石屋，可这一幕在暗道里，与石屋隔着两三张图。
-- 现已改成「他顺着这条道走回去」：上一个脚本（tiezhe.lua）挂在南口内侧，
-- 这一处在石室里，念完纸条上那四个字之后玩家确实是**沿着这条道往回走**
-- 才走到他跟前的，两处的先后由 ch03.tiezhe_zhi 这道前置钉死。
--
-- 这一节的落点不是「多了个同伴」，是**韩立发现自己不够难过**。
-- 掀开帽兜之前他以为自己会发火，等了一会儿，火没有来；心里只有一点点难过，
-- 难过完就没有了。让他害怕的不是那张脸，是他自己——他今年十六。
-- 这一段紧接着上一个脚本搜尸时那句「摸的时候并没有觉得不该摸」。
-- **不许写成他强忍悲痛**，也不许写成他冷酷地宣布什么。他只是站在那儿等，
-- 等一件该来的事没有来。
--
-- 张铁这条线从第 1 章接下来：十岁那年一块从山下走过来的，一路上话最多的就是他；
-- 后来「出走」了三年，谷里谷外都说他是嫌苦跑了。硬约束第 8 条（ch12、ch63-64）。
-- 韩立对着天上说的那几句是祈祷，不是仪式；说完他觉得自己很傻，
-- 可说完心里确实松了一点——这一句是他这一天里唯一还像个孩子的地方。
--
-- 曲魂**不说话、不成长**（设计第 4 节），所以本节没有一句 talk 的说话人是他。
-- 他跟上来的凭据是册子最后那张纸条上的四个字，不是感情。

if flag.get("ch03.tiezhe_zhi") ~= 1 then
    talk("", "ch03.quhun.gate")
    return
end

if flag.get("ch03.quhun_rudui") == 1 then
    talk("", "ch03.quhun.again")
    return
end

talk("", "ch03.quhun.hood")
talk("", "ch03.quhun.face")
talk("", "ch03.quhun.cold")
talk("", "ch03.quhun.scared")
talk("", "ch03.quhun.pray")
talk("", "ch03.quhun.name")

-- 入队。**看返回值**：契约 1.3 明写传一个 data/roles/ 里不存在的 id 会回填
-- ok = false，而拼错的角色 id 会在战斗里变成一个没有属性的空单位，
-- 那时已经很难追回是谁写错的。
-- 失败分支照实说话，不静默——玩家点了没动静一律会被当成 bug。
if party.add("qu_hun") then
    talk("", "ch03.quhun.follow")
else
    talk("", "ch03.quhun.nofollow")
end

flag.set("ch03.quhun_rudui")
