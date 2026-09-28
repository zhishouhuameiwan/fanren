-- @hook ch02_yaopu trigger_zhitong interact
-- 第二章节点 8「配止痛药，结下人情」，兼段三的段末事件。
-- 挂在 ch02_yaopu 晾药棚石碾旁的 trigger（mode=interact）。
--
-- 本章唯一的三选一。三个配法各有代价，没有一个是白给的：
--   1 重剂麻沸：压得住疼，但人要昏半日，那半日握不稳刀。
--   2 温和不误事：只减三成，好处是发作完还能照常练功。
--   3 补养护底子：眼下最不解渴，日子却能拖得长些。
-- 三条都配得出药、都交得出去、都结得下人情。差别只在节点 9 厉飞雨回话的
-- 内容上——设计文档要的是「影响后续口吻」，不是分出优劣。
--
-- 配药要药材。这是段三的推进闸门：药圃的活计得成常态，柜子里才有东西。
-- 闸门不能变成死结，所以缺药材且地里也空着的时候由管事拨两株种苗过来
-- （欠着，来年出了药再还）——玩家永远有路可走，但得自己把药种出来。
--
-- 交药那一段照原著 ch24：次日午时，神手谷口。advance_days(1) 在脚本内部跨过
-- 这一夜，中途不需要把控制权交还给玩家，所以不必拆成两个脚本。

if flag.get("ch02.chousui_jian") ~= 1 then
    talk("", "ch02.zhitong.gate_jian")
    return
end

if flag.get("ch02.renqing_jiexia") == 1 then
    talk("", "ch02.zhitong.again")
    return
end

-- 手头没有现成的药材就配不出药。地里种着的也算数——那说明他只是还没到收的
-- 时候，让他自己去等；两样都空才由管事接济，免得把玩家堵死在这里。
if item.count("herb_huangjing_cao") < 1 then
    talk("", "ch02.zhitong.gate_yao")
    if field.planted("shenshougu_yaopu") < 1 then
        talk("yaopu_guanshi", "ch02.zhitong.gate_give")
        give("herb_huangjing_cao", 2)
    end
    return
end

talk("", "ch02.zhitong.start")
talk("", "ch02.zhitong.think")

local pick = choice{
    "ch02.zhitong.opt_meng",
    "ch02.zhitong.opt_wen",
    "ch02.zhitong.opt_bu",
}

if pick == 1 then
    flag.set("ch02.zhitong_fangzi", 1)
    talk("", "ch02.zhitong.meng_1")
    talk("", "ch02.zhitong.meng_2")
elseif pick == 3 then
    flag.set("ch02.zhitong_fangzi", 3)
    talk("", "ch02.zhitong.bu_1")
    talk("", "ch02.zhitong.bu_2")
else
    -- 温和那一条（pick == 2）与取消（pick == nil）走同一条：
    -- 没拿定主意的人，配出来的就是规规矩矩的那一副。
    flag.set("ch02.zhitong_fangzi", 2)
    talk("", "ch02.zhitong.wen_1")
    talk("", "ch02.zhitong.wen_2")
end

take("herb_huangjing_cao", 1)
talk("", "ch02.zhitong.grind")

-- 碾了一夜，次日午时谷口交药。
advance_days(1)

talk("", "ch02.zhitong.noon")
give("pill_zhitong_san", 1)
talk("", "ch02.zhitong.hand")

if flag.get("ch02.zhitong_fangzi") == 1 then
    talk("li_feiyu", "ch02.zhitong.lfy_meng")
elseif flag.get("ch02.zhitong_fangzi") == 3 then
    talk("li_feiyu", "ch02.zhitong.lfy_bu")
else
    talk("li_feiyu", "ch02.zhitong.lfy_wen")
end

-- 药交出去了。人情是这么结下的，不是磕头拜把子结下的。
take("pill_zhitong_san", 1)

talk("li_feiyu", "ch02.zhitong.lfy_owe")
talk("", "ch02.zhitong.hanli_end")
talk("", "ch02.zhitong.duan_end")

flag.set("ch02.renqing_jiexia")

advance_days(365)

flag.set("ch02.duan4_start")
