-- @hook ch02_yaopu trigger_shiyao interact
-- 第二章节点 10 上半「试药兔死、绿液误洒」。
-- 挂在 ch02_yaopu 空地的 trigger（mode=interact）。
--
-- 硬约束（docs/ch02-design.md 第 1 节第 4 条，出处 ch23-24）：
-- 发现催熟是误打误撞，不是有意实验出来的。
--
-- 所以这一段的每一步都要写成「他本来想做的是另一件事」：
--   他想弄清那滴绿液是毒是药 → 拿兔子试 → 兔子胀破 → 他吓得把碗往身后一甩
--   → 碗摔在药畦上 → 稀释过的绿液泼湿了四五株苗。
-- 泼药那一下必须是甩碗的副产品，不许是他端着碗走过去浇的。
-- 他甚至想过把那几株拔了（怕有毒），最后决定先留着看看——
-- 留下来的理由是「横竖也值不了几个钱」，不是「说不定有用」。
--
-- 这一段与下一段（cuishu.lua）中间隔着一夜，拆成两个脚本：
-- 变化是第二天早上才看得见的，中间那一夜他睡过去了。
--
-- 兔子死得要让人不敢细看。这份恐怖是后面「原来它是催东西长的」那一下的底子——
-- 没有兔子死得这么难看，药草一夜长成十一年就只是个好消息，不是个发现。
--
-- 但**不要照原著的细节清单写**。这行注释原先列着「疙瘩连成片、身子撑成球、
-- 两声闷响、地上两个浅坑」四样，那正是原著用的四样、按原著的顺序——
-- 照着它写出来的文案在 2026-09-20 的独立校对里被判为逐句转写、整章打回。
-- 换词救不了这种问题：细节的种类、数量与出场顺序一样，就还是转写。
--
-- 现在的写法是换取材角度：膨胀本身一次也不直接描写，全靠外物反推
-- （绳子越绷越直、拴绳的石头被一寸一寸拖着走、他忘了自己数到哪儿），
-- 响声之后他没有回头，现场要到收拾时才出现。要改这一段，照这个路子改。

if flag.get("ch02.xiangqi_ping") ~= 1 then
    talk("", "ch02.shiyao.gate")
    return
end

if flag.get("ch02.shiyao_done") == 1 then
    talk("", "ch02.shiyao.again")
    return
end

-- 瓶里得有那一滴才试得成。
--
-- 这一条在当前设计下**恒为假**，是守卫不是分支，穷举证明如下：
-- 绿液的唯一消耗口是 matureHerb，而它要求 matureKnown 为真；
-- matureKnown 只在下一场（cuishu.lua）才置上。所以从拾瓶到这一刻，
-- 绿液只进不出，而段二到段五之间硬推了 180+1+365+365 天，
-- 早把三滴的容量灌满了——走到这里 drops 必然等于 3。
-- 7380 条路径的枚举里，这个 if 一次也没进去过。
--
-- 明知不可达仍然留着，是因为它挡的是引擎侧最容易写错的那一处：
-- P3 契约第 1 节特意交代过 BottleGrant 必须把 lastChargeDay 拨到当日，
-- 那里一旦回归，drops 就会算错。真到了那一天，宁可让它补七天，
-- 也好过让文案一本正经地描写他把空瓶子往碗里倒。
if bottle.drops() < 1 then
    talk("", "ch02.shiyao.nodrop")
    advance_days(7)
end

talk("", "ch02.shiyao.rabbit")
talk("", "ch02.shiyao.tie")
talk("", "ch02.shiyao.dilute")

-- 倒进碗里的那一滴，从这一刻起就不在瓶子里了。
--
-- 走 bottle.spend 而不是 bottle.mature：他这会儿是在试毒，不是在催熟——
-- 催熟之能要到明早（cuishu.lua）才被发现，此刻 matureKnown 还是假的，
-- 走 matureHerb 只会拿到「尚不知瓶中绿液有催熟之能」而一滴不扣。
-- 药畦上那几株是被甩出去的半碗水泼到的，那是这一滴的后效，不再另扣一滴。
--
-- 上面那道守卫刚保证过瓶里至少有一滴，所以这里不该失败；真失败了也只能
-- 照实记账，本章没有对应的文案可播。
bottle.spend(1)
talk("", "ch02.shiyao.cold")
talk("", "ch02.shiyao.drink")

talk("", "ch02.shiyao.wait")
talk("", "ch02.shiyao.swell")
talk("", "ch02.shiyao.run")
talk("", "ch02.shiyao.boom")
talk("", "ch02.shiyao.sit")

talk("", "ch02.shiyao.clean")
talk("", "ch02.shiyao.notice")
talk("", "ch02.shiyao.hesitate")
talk("", "ch02.shiyao.night")

-- 这一夜过去，变化才看得见。
advance_days(1)

flag.set("ch02.shiyao_done")
