-- @hook ch02_jusuo trigger_chujue interact once
-- 第三章节点 11「身醒敌亡；处决余子童」。本章高潮的下半。
-- 挂在 ch02_jusuo 屋子西北角 (1,1)的 trigger（mode=interact，once=true，
-- set_flag=ch03.yuzitong_chujue）。
--
-- ---------------------------------------------------------------------------
-- 两种杀的分别，全章的要害。
-- ---------------------------------------------------------------------------
-- 上一个脚本（duoshe.lua）一次 choice 也没有：他睡着，事情就办完了。
-- 这一个脚本玩家必须**自己走到墙角、自己点开它**（关卡侧挂的是 interact 触发，
-- 不是 enter），进来之后还要自己选先问哪一件，最后那一下是他自己按的拇指。
-- 这一句从前与关卡对不上：地图那边挂的是 enter，玩家走过门洞就自动开演，
-- 而整段论证建立在这一句上。现由 tests/Ch03TriggerModeTests.cpp 直接钉住
-- 两个 trigger 的 mode 属性本身，不再靠注释与驱动方式间接表达。
-- ch03.chujue.after 把这句话摆在明面上：
--   「睡醒时事情已经办完了，这一回是他自己走过去、自己叫的名字、自己按下的拇指。」
--
-- 墨居仁的死**不是玩家动手**（大纲注解、硬约束第 6 条）。
-- 所以 ch03.mo_siwang 在这里置上时，韩立做的事是搭脉：
-- 四年学的手艺，头一回用在这种地方；按了三个位置，按完在衣襟上擦了两下。
-- 他甚至不知道对方是什么时候死的。**不许写他松一口气之后的胜利感。**
--
-- 处决则相反，从头到尾是主动的：走过去、先叫名字、问完、然后动手。
-- 决定他动手的那一下不是仇恨，是**同一句誓**——余子童对他发的毒誓，
-- 和它在石屋里对着墨居仁发的那一回一模一样，连停顿的地方都一样（ch03.chujue.oath）。
-- 这是原著 ch61 那句「我从不和以自己的双亲来发毒誓的人合作」的重写：
-- 原著是韩立事后讲道理，本作是他当场听出了复读。
--
-- 元神怕光那一条（原著 ch61）留着，但来路改了：韩立是从墨居仁一进屋就吹灯
-- 这件事上想到的，而那盏灯是玩家在节点 10 亲眼看着灭的。
--
-- 分支：先问哪一件。两条都只问得成一件，问完他就动手了。
--   问死因——他得到的是三条铁则里的第二条，但只是口头的，到节点 12 才在册子上坐实；
--   问巨汉——他得到的是「那句话在墨居仁身上」，省掉了节点 12 里翻找的力气。
-- 两条都不改走向：节点 12 搜尸时两样东西都在那具尸首上。
-- 这一处**没有旗标可置**（本章分到的 18 个里没有对应的一个），故不向后传。

if flag.get("ch03.shihai_done") ~= 1 then
    talk("", "ch03.shenxing.gate")
    return
end

if flag.get("ch03.yuzitong_chujue") == 1 then
    talk("", "ch03.chujue.again")
    return
end

talk("", "ch03.shenxing.cold")
talk("", "ch03.shenxing.eye")
talk("", "ch03.shenxing.first")
talk("", "ch03.shenxing.wait")
talk("", "ch03.shenxing.check")
talk("", "ch03.shenxing.dead")
talk("", "ch03.shenxing.notme")

-- 墨居仁已死。**不是玩家动手**，这条旗标登记的原话就是这么写的。
flag.set("ch03.mo_siwang")

talk("", "ch03.shenxing.paper")
talk("", "ch03.shenxing.corner")
talk("", "ch03.shenxing.know")
talk("", "ch03.shenxing.stand")
talk("", "ch03.shenxing.end")

talk("", "ch03.chujue.name")
talk("yu_zitong", "ch03.chujue.answer")
talk("yu_zitong", "ch03.chujue.admit")

local pick = choice{
    "ch03.chujue.opt_why",
    "ch03.chujue.opt_giant",
}

if pick == 2 then
    talk("yu_zitong", "ch03.chujue.giant1")
    talk("yu_zitong", "ch03.chujue.giant2")
    talk("", "ch03.chujue.giant3")
else
    -- 问死因（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按问死因算：他刚在一具尸首旁边醒过来，最想知道的就是这个。
    talk("yu_zitong", "ch03.chujue.why1")
    talk("yu_zitong", "ch03.chujue.why2")
    talk("", "ch03.chujue.why3")
end

talk("yu_zitong", "ch03.chujue.offer")
talk("", "ch03.chujue.oath")
talk("", "ch03.chujue.decide")

-- 那一筒。**必须看 take 的返回值。**
-- pill_qidu_shui 是节点 7 配的，玩家可能一筒也没配成（备毒那一节的 take 失败分支），
-- 也可能只配了一筒并在别处用掉。没有毒的那一条不是死路：
-- 原著 ch61 里真正灭掉那团元神的本来就是推开石门放进来的日头，
-- 毒只是让它施不出法术。所以无毒也处决得成，只是他手上没有那一手准备。
local has_poison = take("pill_qidu_shui", 1)

if has_poison then
    talk("yu_zitong", "ch03.chujue.ask_why")
    talk("", "ch03.chujue.answer2")
    -- 有剑没剑是节点 7 那次买卖的结果，一路接到第 2 章章末那次选择。
    if item.count("weapon_yudai_duanjian") > 0 then
        talk("", "ch03.chujue.sword")
    else
        talk("", "ch03.chujue.nosword")
    end
else
    talk("", "ch03.chujue.nopoison")
    talk("yu_zitong", "ch03.chujue.ask_why")
    talk("", "ch03.chujue.answer2")
end

talk("", "ch03.chujue.door")
talk("", "ch03.chujue.light")
talk("", "ch03.chujue.after")
talk("", "ch03.chujue.jump")
talk("", "ch03.chujue.age")
talk("", "ch03.chujue.end")

flag.set("ch03.yuzitong_chujue")
