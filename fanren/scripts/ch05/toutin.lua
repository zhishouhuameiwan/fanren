-- @hook ch05_mofu trigger_toutin interact once
-- 第五章节点 6b「偷听」。挂在 ch05_mofu 小楼西墙 (26,7)（mode=interact，once=true，
-- guard_flag=ch05.yeru，set_flag=ch05.toutin）。他自己贴到小楼墙上去听（ch108-109）。
--
-- 框架件（施工图 13.1）：后园有人种药（ch108——节点 11a 才知道是墨凤舞）；暗哨他都绕过去了（ch108）；
-- 伏在小楼二层窗外。原著这一场的花香夜色、守卫人数、贴墙隐身都不用（2026-09-25 整改）：
-- 他是捏一撮土闻出黄精、听着虫声绕开暗哨的。
--
-- 屋里是严氏与墨彩环。**转写高风险点 3**：暗舵解银的镇名与数目一个不照搬，最多一句概括；
-- ch05.yeru == 2（走屋顶来晚了）时连这一句也没赶上，暗舵与五娘那两句换成一句「话已经说到半截」。
-- 「冒牌货」「未来姐夫」（ch108 严氏原话「名义上是你未来姐夫」）照留意思，句子自己写。
--
-- 二选一 → ch05.toutin：
--   1 听下去（原著）：墨府早查明了姓吴的底细，大小姐自己提出陪他演下去拖时间，拖不下去就翻脸
--     拿人（ch109）；屋里的人还在等的那一个，是他亲手埋的。
--   2 听到「冒牌货」就现身：节点 7a 他当众揭穿吴剑鸣，严氏只淡淡回一句「知道」——他先输一着。
--   取消按 1 算：窗外那个人还没动，就是还在听。
-- **这一夜不打**：原著他绕过了所有暗哨，没人察觉（ch108）。

-- 夜探的曲子 yeru.lua 已经点上了；再点一次是给「在后园里存了档、读回来」的那一路：
-- 点播不进存档，读回来放的是墨府的地图曲。听完这一段（登门是敲门报名，不再是潜入）就撤。
bgm("bgm_night")
talk("", "ch05.toutin.garden")
talk("", "ch05.toutin.post")
talk("", "ch05.toutin.lou")

if flag.get("ch05.yeru") == 2 then
    talk("", "ch05.toutin.late")
else
    talk("", "ch05.toutin.acct")
    talk("", "ch05.toutin.wuniang")
end

talk("mo_caihuan", "ch05.toutin.fake")
talk("yan_shi", "ch05.toutin.fake2")
talk("", "ch05.toutin.know")
talk("", "ch05.toutin.q")

local pick = choice{
    "ch05.toutin.opt_listen",
    "ch05.toutin.opt_now",
}

local toutin = 1
if pick == 2 then
    talk("", "ch05.toutin.now")
    toutin = 2
else
    talk("", "ch05.toutin.listen1")
    talk("", "ch05.toutin.listen2")
    talk("", "ch05.toutin.listen3")
end

bgm("map")
flag.set("ch05.toutin", toutin)
