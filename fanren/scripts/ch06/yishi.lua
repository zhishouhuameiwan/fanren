-- @hook ch06_sanxiu_lou trigger_yishi interact once
-- 第六章节点 7「小楼议事：偷技、摆摊、失踪、升仙大会」。挂在 ch06_sanxiu_lou 二楼议事屋的门 (6,8)(7,8)
-- （mode=interact，once=true，guard_flag=ch06.xunxin，set_flag=ch06.yishi）。上二楼推门的是他（ch132）。
--
-- ch132-135 压成这一段（施工图 3.2 节点 7；镜头与台词以 13.1 为准，复验整改 16.5 又清了一轮）：
--   吴九指借拍胸口顺他的药袋，被他闻出来、把绳结重打成死结（不写抓不抓手）；熊大力倒茶、青纹劝；
--   ch06.ruhuo == 2 时吴九指多一句。摆摊商议——他不去，理由是他自己的（想一个人再把广场走一遍）。
--   **散修失踪**是节点 12 的伏笔，青纹自己说出口。
--   **升仙大会压成 6 句**（胡萍姑主讲，红莲散人起的头，黑金那句保留）：数字是事实，讲法与语气全换。
--   会后同行，他说等会散了再定；青纹先说不急，吴九指才嘟囔一句打赌。
-- 末尾 advance_days(1)（次日众人出摊）。吴九指、黄孝天从这一刻起在楼里（visible_flag=ch06.yishi）。

talk("", "ch06.yishi.door")
talk("qingwen_daoshi", "ch06.yishi.intro")
talk("wu_jiuzhi", "ch06.yishi.brother")
talk("", "ch06.yishi.wrist")
if flag.get("ch06.ruhuo") == 2 then
    talk("wu_jiuzhi", "ch06.yishi.easy")
end
talk("", "ch06.yishi.xiong")
talk("qingwen_daoshi", "ch06.yishi.calm")
talk("wu_jiuzhi", "ch06.yishi.sorry")
talk("qingwen_daoshi", "ch06.yishi.eighth")
talk("huang_xiaotian", "ch06.yishi.huang")
talk("qingwen_daoshi", "ch06.yishi.stall")
talk("", "ch06.yishi.nothing")
talk("qingwen_daoshi", "ch06.yishi.vanish")
talk("", "ch06.yishi.faces")

talk("honglian_sanren", "ch06.yishi.what")
talk("", "ch06.yishi.huang_sleep")
talk("hu_pinggu", "ch06.yishi.dan")
talk("hu_pinggu", "ch06.yishi.rule")
talk("hu_pinggu", "ch06.yishi.ring")
talk("hu_pinggu", "ch06.yishi.dead")
talk("qingwen_daoshi", "ch06.yishi.family")
talk("hei_jin", "ch06.yishi.heijin")

talk("qingwen_daoshi", "ch06.yishi.together")
talk("", "ch06.yishi.later")
talk("qingwen_daoshi", "ch06.yishi.smooth")
talk("wu_jiuzhi", "ch06.yishi.huh")
talk("", "ch06.yishi.night")

advance_days(1)

flag.set("ch06.yishi")
