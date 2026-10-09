-- @hook ch07_dixia_zhaoze trigger_guanzhan enter once
-- 第七章节点 30「观战；墨蛟」。战斗⑤。挂在 ch07_dixia_zhaoze 两堆黑土之间 (3,7)(4,7)
-- （mode=enter，once=true，guard_flag=ch07.didao，set_flag=ch07.mojiao）。伏在黑土堆后，外面的事发生在他眼前。
--
-- 框架件（施工图 13.1 第 30 段）：地下百余丈，一大片沼泽；边上长着数十株灵药（他要的三味都在）；中央亭子里悬着一口金箱；
-- 他藏起来（敛气＋匿形）；掩月宗一群人到，领头的是个白衣少女，弟子叫她「师祖」——结丹才当得起的称呼，她的法力却只有炼气顶峰；
-- 一对对男女合击（阴阳牵引术）；她祭出法宝朱雀环；沼泽里出来的是墨蛟（她认出来；原著「不是黑麟蟒」那一句不用）；
-- 套住、焚烧、打出血洞；墨蛟蜕皮——雪白、生角生爪，进了二阶（可比筑基中阶）；紫液化掉几名弟子（**只写地上多了坑**）；
-- 她叫众人撤、自己拖住；要走时通道被一道禁法封死（小五行须弥禁法；外面贴符的事是暗场，「南宫」二字这里不出现，12.2）；
-- 她靠符箓撑了几个时辰，法力将尽；二选一：1 再等等（原著）/ 2 这就出手（她还剩几分力气，战后多一句冷言；编成同一张）；取消按 1 算；
-- 他出手的算计：通道没了，只有她认得这道禁法；她问他为何不早出手——他答怕她事后杀人夺宝（**说法自拟，13 第 7 条**）；她叫他小家伙；
-- **战斗⑤** b07_zhaoze_shouyao（友军白衣少女，致命、不可逃）；胜：她收了墨蛟的元神，蛟尸归他。
-- 白衣少女说话人用 baiyi_shaonv（名牌「白衣少女」），不摆 NPC（施工图第 4 节末段）。

talk("", "ch07.guanzhan.stairs")
talk("", "ch07.guanzhan.swamp")
talk("", "ch07.guanzhan.herbs")
talk("", "ch07.guanzhan.box")
talk("", "ch07.guanzhan.hide")
talk("", "ch07.guanzhan.come")
talk("yanyue_dizi", "ch07.guanzhan.shizu")
talk("", "ch07.guanzhan.shock")
talk("baiyi_shaonv", "ch07.guanzhan.order")
talk("", "ch07.guanzhan.pairs")
talk("", "ch07.guanzhan.ring")
talk("", "ch07.guanzhan.rise")
talk("baiyi_shaonv", "ch07.guanzhan.mojiao")
talk("", "ch07.guanzhan.burn")
talk("", "ch07.guanzhan.shed")
talk("", "ch07.guanzhan.pit")
talk("baiyi_shaonv", "ch07.guanzhan.retreat")
talk("", "ch07.guanzhan.alone")
talk("", "ch07.guanzhan.sealed")
talk("baiyi_shaonv", "ch07.guanzhan.seal_name")
talk("", "ch07.guanzhan.hours")

local pick = choice{
    "ch07.guanzhan.opt_wait",
    "ch07.guanzhan.opt_now",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.guanzhan.wait")
else
    talk("", "ch07.guanzhan.now")
end
talk("", "ch07.guanzhan.calc")
talk("baiyi_shaonv", "ch07.guanzhan.why_late")
talk("", "ch07.guanzhan.honest")
talk("baiyi_shaonv", "ch07.guanzhan.xiaojia")
talk("", "ch07.guanzhan.brick")

local won = battle("b07_zhaoze_shouyao")
if not won then
    talk("", "ch07.guanzhan.lost")
    game_over()
    return
end

talk("", "ch07.guanzhan.down")
talk("", "ch07.guanzhan.soul")
if pick == 2 then
    talk("baiyi_shaonv", "ch07.guanzhan.cold")
end
talk("baiyi_shaonv", "ch07.guanzhan.corpse")

flag.set("ch07.mojiao", pick)
