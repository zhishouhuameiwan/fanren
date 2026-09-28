-- @hook ch03_cangshu trigger_miji interact once
-- 第三章节点 6「偷秘籍」。
-- 挂在 ch03_cangshu 书架处的 trigger（mode=interact，once=true，set_flag=ch03.miji_done）。
-- 走明路撞见的那一条是另一个脚本：cangshu_mingdao.lua（once=false，可反复撞）。
--
-- ---------------------------------------------------------------------------
-- 偷的是谁的书：**墨居仁自己的**，不是七绝堂的。
-- ---------------------------------------------------------------------------
-- 硬约束第 3 条只要求「是他自己的盘算，不是被谁指点」，没有规定偷谁的。
-- 改成偷师父自己那面书架，有三条理由：
--   1. 旗标 ch03.miji_done 登记的原话是「备毒与识海一战都以它为前置」。
--      七绝堂的武学供不上这两样；墨居仁的那面架子两样都供得上——
--      后半页的毒方（缺的那两味）与被截断的长春功后几层。
--      韩立最后能在识海里吞掉对方，正因为他早已练过对方以为他没有的那几层。
--   2. 关卡侧的 ch03_cangshu 在谷里，厉飞雨进不来；设计第 6 节要「被发现的风险感」，
--      而风险最大的地方就是在囚他的人自己屋里翻他的东西。
--   3. 原著 ch35 是好友把半堂藏书扔给他、两人在水潭边打闹。那一场的取材角度
--      （包裹、死结、缠丝手、哈哈大笑）与本作要的压迫感南辕北辙，照搬只会打架。
--
-- 取材角度另换了三处，全用来替代原著那一场的欢快：
--   * 他挑的不是最黑的一夜，是**下雨**的一夜——水声盖脚步，站岗的也不在院里；
--   * 门**没有锁**，四年都没锁过，因为谷里没人敢进，这比锁管用；
--   * 走的是水沟，脚印第二天早上就没了，这条路他白天先走过两趟。
-- 通篇不写他害怕，只写他数架子数了两遍、雨小了一阵他整个人贴在架上没敢动。
--
-- 分支 ch03.miji_juewu：1 当场看 / 2 先藏起来。
-- 两条都读到同一页（口诀不止四层），差别是当场那一条多一次惊吓、
-- 藏起来那一条多三天。**不能让「稳」的那条一无所获**，那样选择就是罚款。

if flag.get("ch03.yingdui_xuan") < 1 then
    talk("", "ch03.miji.gate")
    return
end

if flag.get("ch03.miji_done") == 1 then
    talk("", "ch03.miji.again")
    return
end

talk("", "ch03.miji.night")
talk("", "ch03.miji.door")
talk("", "ch03.miji.in")
talk("", "ch03.miji.rain")
talk("", "ch03.miji.find")
talk("", "ch03.miji.thin")

-- 四册书落到手上。story_mo_shouzha 是那一册薄的（手札），
-- 另外三册方子并成同一条物品记录：它们在玩法上只有一个用处，
-- 分成四件只会让背包里多三行没人看的东西。
give("story_mo_shouzha", 1)

talk("", "ch03.miji.out")

local pick = choice{
    "ch03.miji.opt_now",
    "ch03.miji.opt_hide",
}

-- 本节点一共走 60 天，两条路都一样。藏起来的那一条先过三天再翻开，
-- 那三天从这 60 天里扣，不外加。
local elapsed = 60

if pick == 2 then
    flag.set("ch03.miji_juewu", 2)
    talk("", "ch03.miji.hide1")
    talk("", "ch03.miji.hide2")
    -- 藏起来的那一条晚三天才翻开。三天在这一章不是惩罚，
    -- 是他自己给自己定的规矩——这一年他活下来靠的就是这种规矩。
    advance_days(3)
    talk("", "ch03.miji.hide3")
    -- 日历不随选择漂移，是为了让节点 9「纸包上那个日子还剩一个月」
    -- 这句话在所有分支下都成立——台词断言了一个数，引擎里就得是那个数。
    elapsed = elapsed - 3
else
    -- 当场看（pick == 1）与取消（pick == nil）走同一条。
    -- 没做选择按当场看算：一个等了四年的人，东西到手就在檐下翻开，
    -- 比抱着它走回屋更像他这个年纪。
    flag.set("ch03.miji_juewu", 1)
    talk("", "ch03.miji.now1")
    talk("", "ch03.miji.now2")
    talk("", "ch03.miji.now3")
end

-- 以下两条合流：方子上短的那两味，以及那两味里的土菇花。
-- 这里直接回指节点 3 墨居仁说过的「照方子该是七味」——
-- 玩家若在摊牌时把铁筒交了出去，这一刻会自己把两句话接上。
talk("", "ch03.miji.poison")
talk("", "ch03.miji.tugu")
talk("", "ch03.miji.end")

advance_days(elapsed)

flag.set("ch03.miji_done")
