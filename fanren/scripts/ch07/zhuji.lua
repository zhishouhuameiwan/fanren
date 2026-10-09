-- @hook ch07_dihuo trigger_zhuji interact once
-- 第七章节点 36「服丹筑基；出关」。挂在 ch07_dihuo 十九号墙角的蒲团 (28,1)
-- （mode=interact，once=true，guard_flag=ch07.chengdan，set_flag=ch07.done）。就地服丹的是他。
--
-- 框架件（施工图 13.1 第 36 段）：就地服丹；第一粒洗髓、排出一身杂质 → 十二层；十几日后第二粒 → 十三层；第三到第七粒：洗髓渐弱、
-- 真元由气化液、周身灼热；第八粒药力爆发、昏死，醒来已是筑基；残余药力要功法吸纳，手上只有《青元剑诀》（九层；前三层掌发剑芒、
-- 中三层护体剑盾；**后三层不说**——那是 ch217，第 8 章，施工图 12.1）；在屋里共十一个月；出来时丑汉改口称师叔，他没为难丑汉。
-- 服丹那几粒写痛不写奇，**原著那一串感受（烈火、刀绞、奇痒、灰色杂质）不用**（13.1 判据第 3 条）。
-- 二选一：1 就在这里闭关（原著）/ 2 回药园再说（他想了想，还是这里）——两条都就地，不另置旗标；取消按 1 算。
-- realm.advance 恰三处、目标 12 → 13 → 21，次序钉死（验收 5），每一处看返回值；take("pill_zhuji_dan", 8) 本章唯一一处（第 9 节第 2 条，
-- 章末剩 17）。advance_days(150)：连同 35c 的 180 天，在屋里共十一个月（施工图 3.3）。
-- 末尾 teleport ch06_baiyaoyuan (24,10)（BAIYAOYUAN_LUKOU）、章末一句落在他自己的感觉上（不给志向、不用面板式说法）。不 ending()。
-- 登 kScriptTransfers（地火屋 → 百药园）。

local pick = choice{
    "ch07.zhuji.opt_here",
    "ch07.zhuji.opt_home",
}
if pick == 2 then
    talk("", "ch07.zhuji.home_no")
else
    talk("", "ch07.zhuji.here")
end

talk("", "ch07.zhuji.first")
local ok = realm.advance(12)
if ok then
    talk("", "ch07.zhuji.twelve")
end
talk("", "ch07.zhuji.second")
ok = realm.advance(13)
if ok then
    talk("", "ch07.zhuji.thirteen")
end
talk("", "ch07.zhuji.more")
talk("", "ch07.zhuji.liquid")
talk("", "ch07.zhuji.eighth")
ok = realm.advance(21)
if ok then
    talk("", "ch07.zhuji.foundation")
end
take("pill_zhuji_dan", 8)
talk("", "ch07.zhuji.eight")
talk("", "ch07.zhuji.rest")
talk("", "ch07.zhuji.jianjue")
talk("", "ch07.zhuji.jianjue2")

advance_days(150)

talk("", "ch07.zhuji.out")
talk("chou_han", "ch07.zhuji.shishu")
talk("", "ch07.zhuji.leave")

flag.set("ch07.done")
teleport("ch06_baiyaoyuan", 24, 10)
talk("", "ch07.zhuji.garden")
