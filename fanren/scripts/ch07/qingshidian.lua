-- @hook ch07_huanxingshan trigger_qingshidian interact once
-- 第七章节点 29「青石殿与地道」。挂在 ch07_huanxingshan 青石殿的门 (37,31)
-- （mode=interact，once=true，guard_flag=ch07.wuyou，set_flag=ch07.didao）。拿金刃试殿门的是他。
--
-- 框架件（施工图 13.1 第 29 段）：第四日不顺——守护兽多、资料有误，法力体力都吃紧；药量勉强够；钟吾的玉简上近处还有一处：
-- 小盆地里的青石殿；殿门弹开他的金刃；钟吾自己为什么不来——他起疑；一大批人逼近，他闪身进殿；殿中央的地道口往外吹潮热的风，
-- 他钻了下去。置 ch07.didao：支线 Z2 从此过期（掉进地道就回不来了）。
-- 末尾 teleport ch07_dixia_zhaoze 石阶口 (3,2)（genmaps_ch07.py 的 ZHAOZE_SHIJIE）；登 kScriptTransfers。

talk("", "ch07.qingshidian.bad")
talk("", "ch07.qingshidian.enough")
talk("", "ch07.qingshidian.jade")
talk("", "ch07.qingshidian.hall")
talk("", "ch07.qingshidian.door")
talk("", "ch07.qingshidian.doubt")
talk("", "ch07.qingshidian.people")
talk("", "ch07.qingshidian.inside")
talk("", "ch07.qingshidian.hole")
talk("", "ch07.qingshidian.down")

flag.set("ch07.didao")
teleport("ch07_dixia_zhaoze", 3, 2)
