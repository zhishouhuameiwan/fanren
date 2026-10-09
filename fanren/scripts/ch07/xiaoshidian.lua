-- @hook ch07_huanxingshan trigger_xiaoshidian enter once
-- 第七章节点 28「小石殿：菡云芝与赤脚大汉」。战斗④。挂在 ch07_huanxingshan 小石殿那一圈山石南边的豁口 (23,11)(24,11)
-- （mode=enter，once=true，guard_flag=ch07.sichu，set_flag=ch07.wuyou）。打斗声是传过来的。
--
-- 框架件（施工图 13.1 第 28 段）：巨剑门的赤脚大汉（银色巨剑）逼卖符少女让出烈阳花，她护身的法器快撑不住（她带进来的灵兽已经死了）；
-- 法器被击碎的一刻，他的金刃救下她——这一回他没守自己的规矩；大汉是武痴，只认他的顶级法器：打赢了，人和药都归他；
-- **战斗④** b07_zhongxinqu_duoyao（致命、不可逃）；战后大汉认出青凝镜（「青凝镜」「掩月双娇」从这里起出现）：掩月双娇的护身法器，
-- 她们的祖母是掩月宗结丹长老——传出去必遭追杀；少女提议都守口如瓶；两个男人都明白了；大汉拼命——十丈之内、没放护罩，
-- 丝线取了他的首级；少女问他是不是也要灭口；他烧尸、收银剑与金刃；互报姓名：她叫菡云芝；她只要烈阳花；
-- 二选一：1 让她忘了这半日（原著）/ 2 先让她立誓（她立了誓，他还是那样做了）；取消按 1 算；
-- 她去采花时一掌打晕（**只写打晕**，13 第 8 条；原著那一吻不演）；扫空石殿灵药（天灵果幼苗 4，烈阳花留给她）；
-- 抱到山顶石洞，无忧针法＋忘尘丸，她只会忘掉半日；他躲在树上看她捧着花茫然下山。
-- 名字：「菡云芝」只在 ch07.xiaoshidian.after* 里出现（12.2 按 key 前缀断言）；她报名之前说话人是 maifu_shaonv（名牌卖符少女），
-- 之后换 han_yunzhi。末尾 advance_days(1)：第三日过去（施工图 3.3）。

talk("", "ch07.xiaoshidian.sound")
talk("", "ch07.xiaoshidian.scene")
talk("yan_wuchi", "ch07.xiaoshidian.dahan")
talk("maifu_shaonv", "ch07.xiaoshidian.girl")
talk("", "ch07.xiaoshidian.break")
talk("", "ch07.xiaoshidian.save")
talk("maifu_shaonv", "ch07.xiaoshidian.girl_you")
talk("yan_wuchi", "ch07.xiaoshidian.chi")
talk("", "ch07.xiaoshidian.go")

local won = battle("b07_zhongxinqu_duoyao")
if not won then
    talk("", "ch07.xiaoshidian.lost")
    game_over()
    return
end

talk("yan_wuchi", "ch07.xiaoshidian.after_mirror")
if take("story_xiao_yuanjing", 1) then
    give("story_qingning_jing")
end
talk("yan_wuchi", "ch07.xiaoshidian.after_shuang")
talk("", "ch07.xiaoshidian.after_heavy")
talk("maifu_shaonv", "ch07.xiaoshidian.after_secret")
talk("", "ch07.xiaoshidian.after_look")
talk("", "ch07.xiaoshidian.after_ten")
talk("maifu_shaonv", "ch07.xiaoshidian.after_me")
talk("", "ch07.xiaoshidian.after_burn")
give("story_yinhui_jian")
talk("", "ch07.xiaoshidian.after_sorry")
talk("", "ch07.xiaoshidian.after_han")
talk("", "ch07.xiaoshidian.after_name")
talk("han_yunzhi", "ch07.xiaoshidian.after_flower")

local pick = choice{
    "ch07.xiaoshidian.after_opt_forget",
    "ch07.xiaoshidian.after_opt_oath",
}
if pick ~= 2 then
    pick = 1
end
if pick == 2 then
    talk("han_yunzhi", "ch07.xiaoshidian.after_oath")
    talk("", "ch07.xiaoshidian.after_oath_sigh")
else
    talk("", "ch07.xiaoshidian.after_forget")
end
talk("", "ch07.xiaoshidian.after_knock")
talk("", "ch07.xiaoshidian.after_sweep")
give("herb_tianling_guo", 4, 1)
talk("", "ch07.xiaoshidian.after_cave")
talk("", "ch07.xiaoshidian.after_needle")
talk("", "ch07.xiaoshidian.after_tree")

advance_days(1)

talk("", "ch07.xiaoshidian.after_day4")

flag.set("ch07.wuyou", pick)
