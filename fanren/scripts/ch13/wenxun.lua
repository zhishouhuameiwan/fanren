-- @hook ch13_nanming trigger_wenxun enter once
-- Facts and participants: docs/ch13-design.md 13.1 段 1。3.2 n01：magic.learn("magic_ji_wuxinghuan") 看返回值
-- （false 换一句照走）；advance_days(15)（3.3 半月海路）；置 ch13.wenxun。
-- 落地 haishang 不触发 enter：这条带在落点北面，文案从海上问话一路说到脚下这块礁石。
-- 「万三谷」「六道极圣」只许出现在 wenxun.*（12.2）；五行环是改编（13.1 段 1 第 5 条）。
talk("", "ch13.wenxun.stop")
talk("hanli", "ch13.wenxun.ask")
talk("sanxian_laozhe", "ch13.wenxun.ling")
talk("hanli", "ch13.wenxun.why")
talk("sanxian_laozhe", "ch13.wenxun.meng")
talk("sanxian_laozhe", "ch13.wenxun.niling")
talk("", "ch13.wenxun.face")
talk("", "ch13.wenxun.infer")
talk("", "ch13.wenxun.weigh")
talk("", "ch13.wenxun.plan")
local learned = magic.learn("magic_ji_wuxinghuan")
if learned then
    talk("", "ch13.wenxun.ring")
else
    talk("", "ch13.wenxun.ring_fail")
end
talk("", "ch13.wenxun.road")
advance_days(15)
flag.set("ch13.wenxun")
