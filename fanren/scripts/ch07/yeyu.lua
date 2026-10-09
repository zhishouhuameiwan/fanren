-- @hook ch07_shandong trigger_yeyu interact once
-- 第七章节点 13「夜遇：陆师兄」。战斗①。挂在 ch07_shandong 洞里的石壁 (7,5)
-- （mode=interact，once=true，guard_flag=ch07.likai，set_flag=ch07.yeyu）。在洞里歇下的是他；洞外的人声是后来的。
--
-- 框架件（施工图 13.1 第 13 段）：下半夜陆师兄带着被制住的陈师妹落在洞外；陆要杀她（有人许他攀高枝、条件是与她断绝；
-- 她是陈家家主独生女、留着会报复；她身上藏着一粒筑基丹，红木盒）——董家、红拂不点名；她被法术捆着、说不出话，
-- 药力让她神志渐乱；他敛气旁观；陆早察觉了他，偷袭被水罩符挡住；陆认出他是东口坡上见过的那个。
-- **暗场口径**（16.1 第 3 条、13 第 8 条）：只写「被捆」「神情迷乱」「扑来」「被定住」，不写任何身体与衣着，
-- 药的名字与效用一概不提。
-- 二选一：1 等他露出破绽（原著）/ 2 这就动手——两条都是水罩符挡住偷袭、陆认出他；取消按 1 算。
-- **战斗①** b07_lu_shixiong：can_escape 假、defeat_is_fatal 真；输 → game_over（原著这一仗输了就是死）。
-- 胜：比法力的那几样（中阶灵石、生嚼年份草药、撤掉水罩——符激发后仍连着一丝灵线耗法力，吴风说过）写成战后一段；
--   **剑符报废**（ch170）：take("talisman_jianfu") ＋ magic.forget("magic_ji_jianfu")（验收 7：恰各一处、同在本文件）；
--   精钢环碎了：take("story_jinggang_huan")；搜身：筑基丹 2、青蛟旗（＋ magic.learn 祭青蛟旗）、青索、银钩、火球符 4、护身符 2、灵石 20
--   （第 9 节来源表；五张必打编成都不掉东西，第 9 节第 3 条）；丹田里淤着草药渣；她药力发作扑来，定神符有则扣，
--   没有就改成他榨出最后一点法力、以定神术定住她——不设死局。

talk("", "ch07.yeyu.rest")
talk("", "ch07.yeyu.steps")
talk("", "ch07.yeyu.lianqi")
talk("", "ch07.yeyu.see")
talk("", "ch07.yeyu.box")
talk("lu_shixiong", "ch07.yeyu.lu1")
talk("lu_shixiong", "ch07.yeyu.lu2")
talk("", "ch07.yeyu.daze")
talk("", "ch07.yeyu.weigh")

local pick = choice{
    "ch07.yeyu.opt_wait",
    "ch07.yeyu.opt_now",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.yeyu.wait")
else
    talk("", "ch07.yeyu.now")
end
talk("", "ch07.yeyu.ambush")
talk("lu_shixiong", "ch07.yeyu.know")
talk("", "ch07.yeyu.cold")

local won = battle("b07_lu_shixiong")
if not won then
    talk("", "ch07.yeyu.lost")
    game_over()
    return
end

talk("", "ch07.yeyu.stalemate")
talk("", "ch07.yeyu.drain")
talk("", "ch07.yeyu.last")
talk("", "ch07.yeyu.cut")
talk("", "ch07.yeyu.ash")
take("talisman_jianfu", 1)
magic.forget("magic_ji_jianfu")
talk("", "ch07.yeyu.ring")
take("story_jinggang_huan", 1)

talk("", "ch07.yeyu.loot")
give("pill_zhuji_dan", 2)
give("story_qingjiao_qi")
magic.learn("magic_ji_qingjiao")
give("story_qing_suo")
give("story_yin_gou")
give("talisman_huoqiu_fu", 4)
give("talisman_hushen_fu", 2)
give("material_lingshi", 20)
talk("", "ch07.yeyu.pain")

talk("", "ch07.yeyu.lunge")
if take("talisman_dingshen_fu", 1) then
    talk("", "ch07.yeyu.freeze_fu")
else
    talk("", "ch07.yeyu.freeze_shu")
end

flag.set("ch07.yeyu", pick)
