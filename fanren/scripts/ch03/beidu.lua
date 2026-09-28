-- @hook ch02_yaopu trigger_beidu interact once
-- 第三章节点 7「备毒」。用毒系统在这一节教给玩家。
-- 挂在 ch02_yaopu 灵田东侧那道畦背上的 trigger（(10,6) 起两格，mode=interact，
-- once=true，set_flag=ch03.beidu_done）。
--
-- **挂点挪过一次，挪的是地图不是文案。** 它从前落在 ch03_mishi 里紧挨着
-- portal_to_andao 的那两格，而本节从头到尾是药圃的戏：卯时畦头、第七畦的畦背、
-- 管事按畦记的账、东头两畦土色不一样（ch03.beidu.ledger / opt_one / opt_two /
-- two1 / two2）。密室是一间石屋，屋里既没有畦也没有管事。挂在那儿的**机制**
-- 理由本来是成立的——ch03.beidu_done 正是 portal_to_andao 的 require_flag，
-- 「没备毒不许进暗道」——可设计第 5 节节点 7 写的是「药圃/密室」，
-- 而这一整节是药圃，所以挪的是地点，一个字的戏也没动。
--
-- 挪完旗标顺序仍旧合法，这一条是必须先验的（校验器规则 19 会拦）：
-- 本节的前置 ch03.miji_done 在藏书处拿，藏书处→密室→神手谷→药圃三跳
-- 反向都没有闸门（谷口那道 portal_to_ch02_yaopu 只要 ch01.done，第 1 章就有了），
-- 挖完再原路回密室推暗道那道门，ch03.beidu_done 已在手上。规则 19 是静态推演，
-- 药圃从第 2 章起一直走得到，这道门因此比从前更早就推得出「拿得到」。
--
-- 因果链在这一节合拢，三处伏笔一起兑现：
--   节点 3 墨居仁掂过那只铁筒，说「五味。照方子该是七味。后两味我没让你抄」；
--   节点 6 他在偷来的方子里对出短的那两味，其中一味叫土菇花；
--   这一节他把它挖出来配进去，整碗东西由黄转黑。
-- 「色转如墨，方成」那一句是方子上的原话，不是他的感想——一个刚满十五的学徒
-- 在做一件他做过很多遍的事，只是这一次配的东西是给人喝的。
--
-- 土菇花对修仙者元神有妨碍，这是原著 ch61 的设定，也是节点 11 处决余子童
-- 时那一筒管用的全部道理。本节一个字也不点破——玩家到那时自己会接上。
--
-- 分支：只挖畦背那一丛 / 把东头两畦一并薅了。
-- 两条的代价不是资源，是**被人记住**。东头那两畦本来就是公中的（第 2 章
-- ch02.renyao.guanshi_dong 交待过，不在他名下的八畦里），划不划走于他无损，
-- 可从此药圃里多了一个人记着他动过东西。这一章他最输不起的正是这个。
-- 收益则是实的：两筒七毒水与四包蚀心散，够他在节点 11 出一次意外。
--
-- 他同时配了清毒散。这一样方子上原本就有，师父教过，教的时候说过一句
-- 「学医先学不害人」——第 2 章那条 modaifu.lua 的注释点名要第 3 章来接的，
-- 接在这里：他把这句话想了一遍，接着配。不表态，不忏悔，接着配。

if flag.get("ch03.miji_done") ~= 1 then
    talk("", "ch03.beidu.gate")
    return
end

if flag.get("ch03.beidu_done") == 1 then
    talk("", "ch03.beidu.again")
    return
end

talk("", "ch03.beidu.ledger")

local pick = choice{
    "ch03.beidu.opt_one",
    "ch03.beidu.opt_two",
}

-- 挖多少、配几筒，两条路分头定数，后面的配药共用同一段。
local dug, brewed
if pick == 2 then
    dug, brewed = 4, 2
    talk("", "ch03.beidu.two1")
    talk("", "ch03.beidu.two2")
    talk("", "ch03.beidu.two3")
else
    -- 只挖一丛（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按只挖一丛算：这一章他做每一件事都先问会不会被看出来。
    dug, brewed = 2, 1
    talk("", "ch03.beidu.one1")
    talk("", "ch03.beidu.one2")
end

give("herb_tugu_hua", dug)

talk("", "ch03.beidu.make1")

-- **必须看 take 的返回值。** 挖出来的那几丛里，一半下进毒里，一半留着——
-- 留下的那一半是实的，背包里看得见，第 4 章之后还配得出第二筒。
--
-- 这一处的 take 在正常流程下不会失败（上一行刚给了 dug = 2*brewed 株，
-- 而 herb_tugu_hua 不可交易、本章没有第二个消耗口），但仍旧写了失败分支：
-- 不写的话，一旦将来有人给土菇花开了别的用处，这里就会变成
-- 「文案说配成了、背包里什么也没多」——第 2 章在这上面栽过两次。
if take("herb_tugu_hua", brewed) then
    talk("", "ch03.beidu.make2")
    talk("", "ch03.beidu.test")
    give("pill_qidu_shui", brewed)
    talk("", "ch03.beidu.mark")
    give("pill_shixin_san", brewed * 2)
    talk("", "ch03.beidu.powder")
else
    talk("", "ch03.beidu.notugu")
end

-- 解毒的那一样与毒一起配，一次三包。暗道那一仗（andao_zhan.lua）要用。
give("pill_qingdu_san", 3)
talk("", "ch03.beidu.antidote")

-- 玉带短剑（原著 ch60）。三十块碎银，走 take 并看返回值。
--
-- 这一处是第 2 章章末那次选择的回响，而且是**数值上的**回响：
--   ch02.qian_quxiang == 1（把钱寄回家）——主线三笔收入 173 块，寄走三分之二，
--     余 58 块，买得起；
--   ch02.qian_quxiang == 2（全换成药材）——余 173 - 28*6 = 5 块，买不起。
-- 两条都走得到，且都不卡关：没有剑的那一条在节点 11 改用推门见光（原著 ch61
-- 本来就是日头灭掉那团元神的），处决照样成立。
-- 暗道那一仗刻意不掉钱（b03_andao_shishou 的 spirit_stones 给 0），
-- 免得把这条分支冲平。
if take("material_lingshi", 30) then
    give("weapon_yudai_duanjian", 1)
    talk("", "ch03.beidu.sword")
else
    talk("", "ch03.beidu.nosword")
end

talk("", "ch03.beidu.end")

advance_days(45)

flag.set("ch03.beidu_done")
