-- @hook ch10_jinhai trigger_ruzhen interact once
talk("", "ch10.ruzhen.chase")
talk("", "ch10.ruzhen.tools")
talk("", "ch10.ruzhen.destroy")
local mirror = item.count("story_qingning_jing") > 0
if mirror and not take("story_qingning_jing", 1) then return end
local mirror_spell = magic.forget("magic_ji_qingning")
local dragon = item.count("story_wulong_duo") > 0
if dragon and not take("story_wulong_duo", 1) then return end
local dragon_spell = magic.forget("magic_ji_wulongduo")
local knife = item.count("story_baizhu_feidao") > 0
if knife and not take("story_baizhu_feidao", 1) then return end
local knife_spell = magic.forget("magic_ji_baizhudao")
if not mirror_spell or not dragon_spell or not knife_spell then
    talk("", "ch10.ruzhen.missing")
end
local removed = party.remove("kuilei_shou")
if not removed then talk("", "ch10.ruzhen.no_puppet") end
talk("", "ch10.ruzhen.array")
talk("", "ch10.ruzhen.resolve")
local won = battle("b10_gu_zhanglao")
if not won then game_over(); return end
give("story_hunyuan_bo")
talk("", "ch10.ruzhen.fall")
talk("", "ch10.ruzhen.sea")
talk("", "ch10.ruzhen.hide")
give("material_lingshi_zhong", 75)
give("pill_jiangchen_dan", 5)
give("talisman_jinjian_fubao")
give("talisman_lanjiao_fu")
give("story_tulijue")
give("material_yaodan_wuji")
talk("", "ch10.ruzhen.loot")
talk("", "ch10.ruzhen.book")
talk("", "ch10.ruzhen.unknown")
talk("", "ch10.ruzhen.home")
talk("", "ch10.ruzhen.route")
advance_days(75)
flag.set("ch10.zhangu")
teleport("ch10_haiyuandao", 8, 18)
