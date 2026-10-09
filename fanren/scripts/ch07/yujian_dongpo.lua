-- @hook ch07_huanxingshan trigger_yujian_dongpo interact once
-- 第七章支线 Z2「钟吾的玉简」之一：东坡石屋。挂在 ch07_huanxingshan 东坡石屋的门 (42,21)
-- （mode=interact，once=true，guard_flag=ch07.yujian_qiu，set_flag=ch07.yujian_a）。原著没有这一处（施工图第 6 节：从 ch193 互换玉简、
-- ch203 他照玉简去了好几处长出来的支线），全部自出。
-- 门口两只铁臂猿：be07_tiebi_yuan（可逃）；逃了、输了都不置旗标（输不致死，触发器留着，回来再打）。
-- 赢：成熟的玉髓芝 2（300 年）——节点 32b 嗅灵兽一并交上去，门派多给一份赏。过期：ch07.didao（掉进地道就回不来了）。

talk("", "ch07.yujian_dongpo.hut")
talk("", "ch07.yujian_dongpo.apes")

local won = battle("be07_tiebi_yuan")
if not won then
    talk("", "ch07.yujian_dongpo.back")
    return
end

talk("", "ch07.yujian_dongpo.inside")
give("herb_yusui_zhi", 2, 300)
talk("", "ch07.yujian_dongpo.leave")

flag.set("ch07.yujian_a")
