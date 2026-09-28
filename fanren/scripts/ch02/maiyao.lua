-- @hook ch02_wairentang trigger_maiyao interact
-- 第二章节点 3 下半「第一次卖药」，兼段一的段末事件。
-- 挂在 ch02_wairentang 收药处的 trigger（mode=interact）。
--
-- 六块灵石。这个数字是照 src/core/rules/Bottle.h 的 herbPrice 实算的：
-- 黄精基价 5，(1 + 10)^2 / 100 = 1.21 倍，得 6。低得让人无所谓正是要的效果——
-- 它是段五那一百四十五的对照组，两个数摆在一起，玩家自己就把账算明白了。
--
-- 药商不讲道理也不报倍数，只报价。机制不靠人解释，靠数字之间的落差。
--
-- 段末推进半年。advance_days 会一并结算灵田生长与绿液凝聚，玩家隔了半年回来，
-- 田里的药是真长了半年——「四年真的过去了」最实在的证据就在这里。

if flag.get("ch02.caiyao_done") ~= 1 then
    talk("wairentang_yaoshang", "ch02.suanzhang.gate")
    return
end

if flag.get("ch02.duan2_start") == 1 then
    talk("wairentang_yaoshang", "ch02.maiyao.again")
    return
end

talk("", "ch02.maiyao.arrive")

-- 先把药扣掉，扣不掉就不往下演（docs/ch02-review.md 的 LOW-5）。
--
-- 采药那一步只保证他采过一株，不保证这一株还在手上：商店设施能把它卖掉，
-- 炼丹也吃药材。从前这里不看 take 的返回值，货没动、六块碎银照给、段末照样
-- 推半年——玩家点了一下，什么也没少，钱却多了。静默失败在本项目是明令禁止的。
--
-- 扣药要排在药商那句「把那株拿到眼前扫了一眼」之前：他得先接着东西，才谈得上
-- 看货报价。失败时不置 ch02.duan2_start，也不推那半年，玩家凑齐一株足年的黄精
-- 再来还能接着卖——灵田面板自己就能起药，不是死路。
-- 两种失败要分开说。
--
-- 闸门要的是「足一年的那一株」（价钱 6 块钉在一年份上）。若玩家手上有黄精、
-- 只是年份不对（比如搁到了两年），却告诉他「空着手来的」，那是句假话，
-- 他低头一看背包就知道游戏在骗他。
if not take("herb_huangjing_cao", 1, 1) then
    if item.count("herb_huangjing_cao") > 0 then
        talk("wairentang_yaoshang", "ch02.maiyao.wrong_age")
    else
        talk("wairentang_yaoshang", "ch02.maiyao.nothing")
    end
    return
end

talk("wairentang_yaoshang", "ch02.maiyao.yaoshang_look")
give("material_lingshi", 6)

talk("wairentang_yaoshang", "ch02.maiyao.price")
talk("", "ch02.maiyao.hanli_count")
talk("wairentang_yaoshang", "ch02.maiyao.yaoshang_more")
talk("", "ch02.maiyao.walk")
talk("", "ch02.maiyao.duan_end")

-- 半年。段一到段二之间的跳时，秋末正好接上崖壁那条落满叶子的山道。
advance_days(180)

flag.set("ch02.duan2_start")
