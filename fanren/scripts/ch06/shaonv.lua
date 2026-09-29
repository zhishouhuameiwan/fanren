-- @hook ch06_tainan_gu npc_maifu_shaonv npc
-- 第六章节点 8b「长春功；金竺笔」。挂在 ch06_tainan_gu 摊位区卖符少女身上（npc，**不撤场**：换完她还站着卖别的，
-- 直到出谷——hidden_flag ch06.chugu，第 7 章施工图要她在第 6 章之后离开太南谷这张图）。
-- 停在她摊前的是他（ch136 末）。施工图 3.1 这一格写 interact + once + guard ch06.shuangshou + set_flag ch06.jinzhubi：
-- NPC 对象没有这几个属性，由脚本自己判——双首鹜飞过之前只说一句闲话；换完笔之后按欠账分两支（见下）。
--
-- 她**不姓燕**，原著此处无名、易羞（施工图 1.1 第 9 条）；本节文案不带「燕」字（12.2）。镜头与台词以 13.1 为准。
-- 两段以物易物（第 9 节第 2 条，需要的数量是字面量 2、7）：
--   ① 《长春功》全本，两颗——书的用处在 9b，不做物品（ch06.changchungong）。
--      七星草种子二选一：要（ch06.zhongzi = 1，笔换成时一并给）/ 不要（什么也不发生）。取消按不要算。
--   ② 金竺笔：她拿通音符叫兄长把笔送来（兄长不落地图，旁白带过，施工图第 5 节）；七瓶换笔（连种子）。
-- 拿药去问那句只说药性、不报方子（怕露底；抽查 N3-L2）；验药按手上有没有药分两句：有就要一颗、指甲挑药末看成色，
-- 一瓶不剩就说定照药价拿灵石折（sniff_none；复验 N-L4、抽查 N3-L1）。
-- **补差（校对 HIGH-3，施工图 16.4）**：药不够不再 return——先扣手上有的药，差的每瓶按 STONES_PER_PILL 块灵石补
-- （与坊市收药同价），灵石也不够就先欠着，欠的瓶数累加进 ch06.qianyao。**ch06.qianyao 只记她一人的账**
-- （复验整改 16.5 MEDIUM-N1 ②：5b 的草帽青年补不齐是免了差额，不记账）。
-- **还账（MEDIUM-N1 ②）**：8b 换完之后、出谷之前再找她——欠着账就问还不还：有药先还药，没药按 STONES_PER_PILL 块
-- 灵石一瓶还，还多少减多少；不欠了才说那句闲话。欠着账的时候，打探（shaonv_dating，走 E 键那个入口）不挂出来
-- （路径行动 when 另含 ch06.qianyao == 0），她也不说「换得太便宜」、不白送金疮药（MEDIUM-N1 ③）。
-- 还账是按确认键找她（这个脚本），打探是 E 键（路径行动），8b 的交易只在 jinzhubi 未置时走——三条路互不相让。
-- 金疮药不算数（原著 ch137 她明说疗伤的药不值钱）。
-- 丹砂：他在别的摊换来——原著没说拿什么换，本作写成拿一瓶金疮药换的，有就扣、没有就白得，不设死局（改编）。
-- 旧档里①做完、②没做完的（changchungong 已置、jinzhubi 未置），再来时从②接着演。
-- 还账那段写成 repay()、定义在两道 guard 之前（jinzhubi 已置的那一支要用它）；它只在 ch06.qianyao > 0 时才被叫到。
-- 买卖的帮手 barter 定义在两道 guard 之后：没满足时要在买卖的任何 take 之前 return（Ch06TriggerModeTests 认的是
-- 去掉 repay() 之后的那一份）。

local STONES_PER_PILL = 7

local function repay()
    talk("maifu_shaonv", "ch06.npc.shaonv.after_owe")
    local pick = choice{
        "ch06.jinzhubi.opt_repay",
        "ch06.jinzhubi.opt_later",
    }
    if pick ~= 1 then
        talk("maifu_shaonv", "ch06.jinzhubi.repay_later")
        return
    end
    local owed = flag.get("ch06.qianyao")
    local left = owed
    local pills = math.min(item.count("pill_yangjing_dan"), left)
    if pills > 0 and take("pill_yangjing_dan", pills) then
        left = left - pills
        talk("", "ch06.jinzhubi.repay_pill")
    end
    local paid = math.min(left, math.floor(item.count("material_lingshi") / STONES_PER_PILL))
    if paid > 0 and take("material_lingshi", paid * STONES_PER_PILL) then
        left = left - paid
        talk("", "ch06.jinzhubi.repay_stone")
    end
    if left == owed then
        talk("", "ch06.jinzhubi.repay_none")
        return
    end
    flag.set("ch06.qianyao", left)
    if left == 0 then
        talk("maifu_shaonv", "ch06.jinzhubi.repay_clear")
    else
        talk("maifu_shaonv", "ch06.jinzhubi.repay_part")
    end
end

if flag.get("ch06.jinzhubi") ~= 0 then
    if flag.get("ch06.qianyao") > 0 then
        repay()
        return
    end
    talk("maifu_shaonv", "ch06.npc.shaonv.after")
    return
end

if flag.get("ch06.shuangshou") == 0 then
    talk("maifu_shaonv", "ch06.npc.shaonv.idle")
    return
end

local function barter(need, shortKey, topupKey, oweKey)
    local pills = math.min(item.count("pill_yangjing_dan"), need)
    if pills > 0 and not take("pill_yangjing_dan", pills) then
        pills = 0
    end
    local short = need - pills
    if short <= 0 then
        return
    end
    talk("", shortKey)
    local paid = math.min(short, math.floor(item.count("material_lingshi") / STONES_PER_PILL))
    if paid > 0 and take("material_lingshi", paid * STONES_PER_PILL) then
        talk("", topupKey)
    else
        paid = 0
    end
    local owe = short - paid
    if owe > 0 then
        flag.set("ch06.qianyao", flag.get("ch06.qianyao") + owe)
        talk("maifu_shaonv", oweKey)
    end
end

if flag.get("ch06.changchungong") == 0 then
    talk("", "ch06.jinzhubi.book")
    talk("maifu_shaonv", "ch06.jinzhubi.price")
    talk("", "ch06.jinzhubi.ask")
    if item.count("pill_yangjing_dan") > 0 then
        talk("maifu_shaonv", "ch06.jinzhubi.sniff")
    else
        talk("maifu_shaonv", "ch06.jinzhubi.sniff_none")
    end
    talk("maifu_shaonv", "ch06.jinzhubi.many")
    talk("", "ch06.jinzhubi.sister")
    talk("maifu_shaonv", "ch06.jinzhubi.two")

    barter(2, "ch06.jinzhubi.short_book", "ch06.jinzhubi.topup_book", "ch06.jinzhubi.owe_book")
    flag.set("ch06.changchungong")
    talk("", "ch06.jinzhubi.gotbook")

    talk("", "ch06.jinzhubi.seedbag")
    talk("maifu_shaonv", "ch06.jinzhubi.seed")
    local pick = choice{
        "ch06.jinzhubi.opt_seed",
        "ch06.jinzhubi.opt_noseed",
    }
    if pick == 1 then
        talk("", "ch06.jinzhubi.seed_yes")
        flag.set("ch06.zhongzi")
    else
        talk("", "ch06.jinzhubi.seed_no")
    end
else
    talk("maifu_shaonv", "ch06.jinzhubi.again")
end

talk("maifu_shaonv", "ch06.jinzhubi.more")
talk("", "ch06.jinzhubi.need")
talk("", "ch06.jinzhubi.hesitate")
talk("maifu_shaonv", "ch06.jinzhubi.brush")
talk("", "ch06.jinzhubi.wait")
talk("", "ch06.jinzhubi.brother")
talk("", "ch06.jinzhubi.box")
talk("maifu_shaonv", "ch06.jinzhubi.make")
talk("maifu_shaonv", "ch06.jinzhubi.reason")
talk("", "ch06.jinzhubi.seven")

barter(7, "ch06.jinzhubi.short_brush", "ch06.jinzhubi.topup_brush", "ch06.jinzhubi.owe_brush")

give("story_jinzhu_bi", 1)
if flag.get("ch06.zhongzi") == 1 then
    give("material_qixingcao_zhongzi", 1)
end
talk("maifu_shaonv", "ch06.jinzhubi.thanks")

-- 丹砂：拿一瓶金疮药换；没有就是摊主看他顺眼，白给的。
if item.count("pill_jinchuang_yao") > 0 and take("pill_jinchuang_yao", 1) then
    talk("", "ch06.jinzhubi.dansha")
else
    talk("", "ch06.jinzhubi.dansha_free")
end
give("material_dansha", 6)
talk("", "ch06.jinzhubi.home")

flag.set("ch06.jinzhubi")
