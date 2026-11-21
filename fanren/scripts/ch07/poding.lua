-- @hook ch07_dixia_shiku trigger_poding interact once
-- 伙伴改版样板（docs/ch07-companions-design.md 4.4、第 6 节）「破顶」。挂在 ch07_dixia_shiku 北头岩层最薄处 (21,2)（实体格，
-- 人站在正下方 (21,3) 面朝上按确认；mode=interact，once=true，guard_flag=ch07.huolingshi，set_flag=ch07.poding）。
--
-- 框架件（原施工图 13.1 第 31 段后半，原著 ch209 一笔回述）：破禁要两人合力；她把被压住的法力暂传给他（他一时有了十三层的法力，
-- 面板不动）；朱雀环与金光砖轮番往上打；**金光砖耗尽成了废纸**：take("talisman_jinguangzhuan") ＋ magic.forget("magic_ji_jinguangzhuan")
-- （验收 7：恰各一处、同在本文件——从 jiaoshi.lua 挪过来）；她损了二三十年功力。
-- 样板把这一段做成一场不会死人的机关战 b07_poding（defeat_is_fatal false、不能逃、修为 0、不掉东西）：
--   · 打输：从石窟的打坐点重来（genmaps_ch07.py 的 SHIKU_DAZUO_PANG），不置旗标——这一格再按还能再打；
--   · 打赢：上面破开一个口子，碎石塌下来、两人退开一步（SHIKU_TUIBU），再往上一迈踩 trigger_wenming（4.5 问名）。
-- 「并肩打赢」+2 由战斗结算记，受每天至多 +6 管（4.6）。

talk("baiyi_shaonv", "ch07.jiaoshi.two")
talk("", "ch07.jiaoshi.lend")

local won = battle("b07_poding")
if not won then
    talk("", "ch07.poding.lost")
    teleport("ch07_dixia_shiku", 24, 15)
    return
end

talk("", "ch07.jiaoshi.brick_gone")
take("talisman_jinguangzhuan", 1)
magic.forget("magic_ji_jinguangzhuan")
talk("", "ch07.poding.collapse")

flag.set("ch07.poding")
teleport("ch07_dixia_shiku", 21, 4)
