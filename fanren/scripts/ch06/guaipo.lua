-- @hook ch06_tainan_cun trigger_guaipo interact once
-- 第六章节点 2「怪坡前：灵根之说、通音符」。挂在 ch06_tainan_cun 怪坡坡脚的界石 (24,5)(25,5)
-- （mode=interact，once=true，guard_flag=ch06.wan_met，set_flag=ch06.rugu）。领他来、让他放通音符，是韩立定的（ch126）。
--
-- ch127 那一整章的扫盲压成三段，每段不超过 8 句（施工图 3.2 节点 2）。**三段的次序是他自己的问法**（校对 HIGH-1 整改，
-- 施工图 16.4 / 13.1）：先问「是不是人人修得」——③ 灵根；再顺着灵根随血脉问到修仙家族与七派——②；最后才问往上几道坎、
-- 能活多久——① 境界与寿元。原著的讲法是境界 → 寿元 → 七派 → 家族 → 灵根，这里不跟。key 名照旧（realm / sect / root）。
-- 境界名、寿元、七派名单是事实照说；比方与讲法全自出（第 13 节第 1 条）。
-- ch128：他照实说自己是散修；万家祖训不许看轻散修；他是新人这件事被点破。万小山带的东西不列清单（第 13 节第 6 条）。
-- 太南小会与升仙大会的名字在这里第一次出现。镜头与台词以施工图 13.1 为准。
--
-- **story.xiuxian_known 在此置**（施工图第 7 节、验收 12：本章只此一处）：他刚听完「炼气」「筑基」这些词，
-- 面板从这一刻起说炼气、法术、法力、灵石。「灵石」二字本节文案里不出现（12.2：5a 才讲）。
-- 末尾 teleport 太南谷谷口雾道底 (23,33)（genmaps_ch06.py 的 GU_GUKOU）；登 tests/ObjectiveTests.cpp 的 kScriptTransfers。

talk("", "ch06.guaipo.walk")
talk("wan_xiaoshan", "ch06.guaipo.root1")
talk("", "ch06.guaipo.root2")
talk("", "ch06.guaipo.root3")
talk("", "ch06.guaipo.root4")
talk("", "ch06.guaipo.sect2")
talk("wan_xiaoshan", "ch06.guaipo.sect3")
talk("wan_xiaoshan", "ch06.guaipo.sect1")
talk("", "ch06.guaipo.realm1")
talk("wan_xiaoshan", "ch06.guaipo.realm2")
talk("", "ch06.guaipo.realm3")
talk("", "ch06.guaipo.realm4")
talk("", "ch06.guaipo.arrive")

talk("wan_xiaoshan", "ch06.guaipo.which")
talk("", "ch06.guaipo.sanxiu")
talk("wan_xiaoshan", "ch06.guaipo.zuxun")
talk("wan_xiaoshan", "ch06.guaipo.newbie")
talk("", "ch06.guaipo.admit")

talk("", "ch06.guaipo.fu")
talk("wan_xiaoshan", "ch06.guaipo.goods")
talk("", "ch06.guaipo.pale")
talk("wan_xiaoshan", "ch06.guaipo.huiyi")
talk("", "ch06.guaipo.nothing")
talk("", "ch06.guaipo.open")
talk("", "ch06.guaipo.in")

-- 他从这一刻起算进了修仙界：面板用词切换（src/game/Wording.h）。
flag.set("story.xiuxian_known")
flag.set("ch06.rugu")
teleport("ch06_tainan_gu", 23, 33)
