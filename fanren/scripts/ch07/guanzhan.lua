-- @hook ch07_dixia_zhaoze trigger_guanzhan enter once
-- 第七章节点 30「观战；墨蛟」。战斗⑤。挂在 ch07_dixia_zhaoze 两堆黑土之间 (3,7)(4,7)
-- （mode=enter，once=true，guard_flag=ch07.didao，set_flag=ch07.mojiao）。伏在黑土堆后，外面的事发生在他眼前。
--
-- 框架件（施工图 13.1 第 30 段）：地下百余丈；他藏起来（敛气＋匿形）；掩月宗一群人到，领头的是个白衣少女，弟子叫她「师祖」；
-- 她祭出法宝朱雀环；沼泽里出来的是墨蛟（她认出来）；墨蛟蜕皮——雪白、生角生爪，进了二阶；
-- 她叫众人撤、自己拖住；要走时通道被一道禁法封死（小五行须弥禁法；「南宫」二字这里不出现，12.2）；
-- 她靠符箓撑了几个时辰；二选一：1 再等等（原著）/ 2 这就出手（战后多一句冷言；编成同一张）；取消按 1 算；
-- 他出手的算计；她问他为何不早出手（说法自拟，13 第 7 条）；她叫他小家伙；**战斗⑤** b07_zhaoze_shouyao（致命、不可逃）；
-- 胜：她收了墨蛟的元神，蛟尸归他。
--
-- 伙伴改版样板（docs/ch07-companions-design.md 4.1、4.2）：
--   · 4.1 观战压到原来大半长短：沼泽、金箱、阴阳牵引、火环套蛟、紫液化人不再单演（金箱在节点 31 那一段里还会见到）；
--     灵药并进藏身那一句（他要的三味就在这里）、「师祖」的惊疑留一句（约束 2：结丹的称呼、炼气顶峰的灵压）、
--     她转身往地道口去并进封口那一句（内容审查 2026-11-18 第 2 条）。
--   · 4.2 她在出手这一刻入队：party.add("baiyi_shaonv")，编成里写死的那名友军删掉，改由队伍带入（否则场上两个她）。
--     这一仗默契是 0，没有追击；打赢了「并肩打赢一仗」+2 由战斗结算记（契约 2.2）。名牌仍是白衣少女（问名在 4.5）。

talk("", "ch07.guanzhan.stairs")
talk("", "ch07.guanzhan.hide")
talk("", "ch07.guanzhan.come")
talk("yanyue_dizi", "ch07.guanzhan.shizu")
talk("", "ch07.guanzhan.shock")
talk("baiyi_shaonv", "ch07.guanzhan.order")
talk("", "ch07.guanzhan.ring")
talk("baiyi_shaonv", "ch07.guanzhan.mojiao")
talk("", "ch07.guanzhan.shed")
talk("baiyi_shaonv", "ch07.guanzhan.retreat")
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

-- 4.2：她入队。拼错角色 id 时 party.add 返回 false——这里错了整段就不对，直接停下。
assert(party.add("baiyi_shaonv"), "party.add(baiyi_shaonv) 失败")
talk("", "ch07.guanzhan.join")

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
