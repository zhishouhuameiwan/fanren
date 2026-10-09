-- @hook ch07_jindi_zhongxin trigger_shulin interact once
-- 第七章节点 23「林中一夜；多宝女；封岳」。战斗③。挂在 ch07_jindi_zhongxin 林边那棵大树 (30,25)
-- （mode=interact，once=true，guard_flag=ch07.yixiantian，set_flag=ch07.fengyue）。爬上大树歇息的是他；后面的事找上门。
--
-- 框架件（施工图 13.1 第 23 段）：树上打坐回法力；禁地的夜亮如白昼、天灰蒙蒙；头一天的清洗（弱的多半死在头一天，讲法本作自出；
-- 萧二、寒天涯、钟吾杀人那几幕不演）；第二日早，同门黄衫师姐逃到树下求救（牵引之术，十日有效）；追来的是掩月宗白衣女子（十二层）；
-- 二选一：1 跳下去救她（原著）/ 2 躲在树上不出声——她那一路也藏不住他（原著那张火球符烧树冠是质感，不用；本作换成她的镜光扫到了他）；取消按 1 算；
-- 多宝女**做成演出、不开战**（8.3）：小镜子定住他的金刃、粉红水晶球锈坏了银钩，他用青索捆住她——一道黄芒穿透青索、护罩与她本人：
-- 天阙堡的狂人封岳；take 青索、银钩（看返回值：没有就不提）；封岳拿走她的储物袋、想收镜子与水晶球，他一枚火球打断；
-- 黄衫师姐想逃，被封岳杀了（只写倒下）；封岳问他想怎么个死法——他回了一句要他命的话（**说法自拟，13 第 7 条**）。
-- **战斗③** b07_fengyue（致命、不可逃）。胜：储物袋被天雷子一起化灰（没用天雷子的那一路，旁白换一句：打烂了）；
-- 捡回小镜子（青凝镜，名字这里不说）＋ magic.learn 祭青凝镜、水晶球、小刀符宝；靴子完好——踏云靴（名字这里也不说）；
-- 草草埋了师姐；第一天过后禁地里只剩七十多人。
-- advance_days(1) 在第一夜之后、第二日早之前（校对 LOW-12：横幅不再先于第一夜上屏）；横幅照剧情写，不读 today()（施工图 3.3）。
-- 校对整改 16.5：封岳的名号由他自己报（§4.2(e) 第 38 行），不再由黄衫师姐惊叫出来。

talk("", "ch07.shulin.tree")
talk("", "ch07.shulin.night")
talk("", "ch07.shulin.purge")

advance_days(1)

talk("", "ch07.shulin.banner")
talk("", "ch07.shulin.morning")
talk("huangshan_shijie", "ch07.shulin.help")
talk("", "ch07.shulin.chaser")

local pick = choice{
    "ch07.shulin.opt_help",
    "ch07.shulin.opt_hide",
}
if pick ~= 2 then
    pick = 1
end
if pick == 1 then
    talk("", "ch07.shulin.help_why")
else
    talk("", "ch07.shulin.hide")
end
talk("duobao_nv", "ch07.shulin.duobao_sneer")
talk("", "ch07.shulin.mirror")
talk("", "ch07.shulin.orb")
if take("story_yin_gou", 1) then
    talk("", "ch07.shulin.hook")
end
talk("", "ch07.shulin.rope")
talk("", "ch07.shulin.flash")
if take("story_qing_suo", 1) then
    talk("", "ch07.shulin.rope_cut")
end
talk("", "ch07.shulin.fengyue")
talk("feng_yue", "ch07.shulin.name")
talk("", "ch07.shulin.grab")
talk("", "ch07.shulin.shijie")
talk("feng_yue", "ch07.shulin.ask")
talk("", "ch07.shulin.answer")

local had_lei = item.count("talisman_tianleizi") > 0
local won = battle("b07_fengyue")
if not won then
    talk("", "ch07.shulin.lost")
    game_over()
    return
end

if had_lei and item.count("talisman_tianleizi") == 0 then
    talk("", "ch07.shulin.after_lei")
else
    talk("", "ch07.shulin.after_plain")
end
talk("", "ch07.shulin.pick")
give("story_xiao_yuanjing")
magic.learn("magic_ji_qingning")
give("story_shuijing_qiu")
give("talisman_xiaodao_fubao")
talk("", "ch07.shulin.boots")
give("story_hei_xue")
talk("", "ch07.shulin.bury")
talk("", "ch07.shulin.seventy")

flag.set("ch07.fengyue", pick)
