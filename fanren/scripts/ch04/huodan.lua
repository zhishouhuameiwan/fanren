-- @hook ch04_luorifeng trigger_huodan enter once
-- 第四章节点 2「习火弹术」。全作第一门真正学会的法术。
-- 挂在 ch04_luorifeng 峰道口 (19,27)(20,27) 的 trigger
-- （mode=enter，once=true，set_flag=ch04.huodan_xue）。
-- 峰道是南崖上唯一的两格口子，四面是崖，踏上峰顶必响 —— 它是本章第一个必过的
-- 教学，漏掉玩家就推不动节点 3（kailu.lua 开头查的正是 ch04.huodan_xue）。
-- 戏演在峰道口一进来的那片石坪上，坪上立着四根石桩
-- （tools/mapgen/genmaps_ch04.py make_luorifeng：石桩在 (14,22)(16,25)(24,22)(26,25)）——
-- 那四根石桩是这一节唯一用到的地物，文案里的炭条记号就划在它们上面。
--
-- ---------------------------------------------------------------------------
-- 取材角度换过：原著那一段的**戏眼在威力上，本作的戏眼在方法上**
-- ---------------------------------------------------------------------------
-- 原著 ch67 的次序是：秘籍末页录着五种法术 → 古文难懂，钻研三个月 → 上手奇笨 →
-- 只成火弹术与天眼术 → 试威力（精钢熔成铁汁、水面被点燃）→ 悟出修仙者为何轻视凡人。
-- 那一串**细节的种类、数量与出场顺序**照抄过来就是转写，第 2 章为此被打回过一次。
--
-- 本作只留下骨架（先难在文法上，再难在上手上，最后成了一门），把重心挪到
-- **他是怎么排查的**：手势、口诀、力道三样里有一样不对就没有动静，而他分不出
-- 是哪一样 —— 于是他一次只改一样，别的两样原样不动，改完在石桩上划一道。
-- 这是个药铺学徒的法子（他第 2 章就是这么试药的），不是修仙者的法子，
-- 也正因如此它是韩立的。
--
-- 威力那一场试验**整段删掉**，只在 data/text/ch04_items.json 的法术描述里留一句
-- 「碰上什么都是一样的下场」。理由：原著那一场是给读者看的，而游戏里玩家会自己
-- 在节点 5、7、9 打三场，威力不必先说。省下的篇幅给了「他数自己的心跳」
-- 与「喘匀之后先搭自己的脉」—— 赢了先清点，这是本章主角的语气卡。
--
-- **看 magic.learn 的返回值。** 契约 docs/interfaces-p3-ch04.md 1.3 节明写：
-- 传一个 data/magics/ 里没有的 id 会回填 ok = false（code = "no_magic"），
-- 不静默收下。真走到失败那一条说明 data/magics/huodan_shu.json 出了事，
-- 那时玩家看到的是一条如实的台词而不是一个空的战斗菜单。
-- 旗标照置 —— 卡死在这一节比拿不到法术更糟，而门禁与通关测试会先一步发现它。

if flag.get("ch04.huodan_xue") ~= 0 then
    talk("", "ch04.huodan.again")
    return
end

talk("", "ch04.huodan.arrive")
talk("", "ch04.huodan.book")
talk("", "ch04.huodan.five")
talk("", "ch04.huodan.ancient")

-- 一个冬天啃古文。时间真的走完：玩家回药圃时田里也确实长了这一冬。
talk("", "ch04.huodan.months")
advance_days(150)

talk("", "ch04.huodan.careful")
talk("", "ch04.huodan.fail1")
talk("", "ch04.huodan.method")

-- 四百来道记号。这一段是本节的重心，时间给足。
advance_days(120)
talk("", "ch04.huodan.tally")

-- ---------------------------------------------------------------------------
-- 本章第二次涨境界：炼气五层 → 七层（设计文档 1.2 节，依据原著 ch65）
-- ---------------------------------------------------------------------------
-- ch65 原话：「不断的服用灵药，他的长春功再次有了突破的迹象，不久后就会进入到
-- 第七层境界」——与那一段同期的正是这一节（ch67 章题即「火弹术」）。
-- 所以这一涨压在记号那一段的末尾：**法术是在他这一年里练成的，境界也是。**
-- 顺序不能倒过来：口诀先顺了，第一发火弹才放得出去（下面 huodan.first 那句）。
--
-- 看返回值，理由见 liuxia.lua 同一处的注释。
talk("", "ch04.huodan.jinjie")
if not realm.advance(realm.QI_REFINING_7) then
    talk("", "ch04.jinjie.fail")
end

talk("", "ch04.huodan.first")
talk("", "ch04.huodan.grow")
talk("", "ch04.huodan.hold")
talk("", "ch04.huodan.out")
talk("", "ch04.huodan.pulse")
talk("", "ch04.huodan.know")

local ok = magic.learn("magic_huodan_shu")

if ok then
    talk("", "ch04.huodan.learn")
else
    talk("", "ch04.huodan.fail_learn")
end

talk("", "ch04.huodan.end")

flag.set("ch04.huodan_xue")
