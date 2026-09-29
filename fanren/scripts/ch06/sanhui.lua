-- @hook ch06_tainan_gu trigger_sanhui enter once
-- 第六章节点 11「散会：拒邀、出谷」。挂在 ch06_tainan_gu 谷口雾道靠北 (23,29)(24,29)
-- （mode=enter，once=true，guard_flag=ch06.shengxianling，set_flag=ch06.chugu）。青纹一行是找上他的（ch142）；
-- 从广场往南出谷必踩。
--
-- 早上谷里的人少了一小半；下午几位前辈宣布散会（青颜真人在内）。青纹再邀。二选一（施工图 3.2 节点 11），两条都拒：
--   1 婉拒：路上还有别的事——青纹叹口气拍拍他的肩；
--   2 直说：不与人同行——吴九指、黑木不满，青纹脸色难看，同样拍肩。取消按 1 算。
-- **拍肩撒粉、出谷放火光符是暗场**：旁白只写「拍了拍他的肩膀」，不说破（施工图第 17 节第 16 条）。
-- 节点 12 搜身时，婉拒的那一支多一句「烧剩的符纸角」。
-- 支线 Z2「太南谷的账」：出谷之前数一遍，身上灵石 ≥ 10 就置 ch06.zhang_done（节点 12 多一句）。
-- 他又住了一晚，天蒙蒙亮溜出谷：advance_days(1)；御风决奔出百余里（第 4 章学的）。
-- 末尾 teleport 百里荒丘落脚处 (4,25)（genmaps_ch06.py 的 SHANQIU_LUODIAN）；登 kScriptTransfers。
-- 从此回不到太南谷（荒丘没有回谷的门）：制符桌与坊市都留在谷里了，支线 Z1 / Z2 随 ch06.chugu 过期。
-- 开头先拨 1 天：10a 说「会期只剩最后两天」、当夜 10b，散会是第二天的事——不拨，10a 到出谷在文案里隔两天、
-- 日历上只隔一天（复验 N-L6，裁决 16.5；3.3 补了这一行，全章 56 天）。

advance_days(1)
talk("", "ch06.sanhui.thin")
talk("", "ch06.sanhui.close")
talk("qingwen_daoshi", "ch06.sanhui.invite")

local pick = choice{
    "ch06.sanhui.opt_polite",
    "ch06.sanhui.opt_plain",
}

local jujue = 1
if pick == 2 then
    jujue = 2
    talk("", "ch06.sanhui.plain")
    talk("wu_jiuzhi", "ch06.sanhui.wu")
    talk("", "ch06.sanhui.cold")
else
    talk("", "ch06.sanhui.polite")
    talk("qingwen_daoshi", "ch06.sanhui.sigh")
end
talk("", "ch06.sanhui.pat")
talk("", "ch06.sanhui.gone")

if item.count("material_lingshi") >= 10 then
    flag.set("ch06.zhang_done")
    talk("", "ch06.sanhui.count")
end

talk("", "ch06.sanhui.wait")
advance_days(1)
talk("", "ch06.sanhui.dawn")
talk("", "ch06.sanhui.wind")

flag.set("ch06.jujue", jujue)
flag.set("ch06.chugu")
teleport("ch06_shanqiu", 4, 25)
