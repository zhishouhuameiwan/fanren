-- @hook ch05_mofu trigger_duizhi interact once
-- 第五章节点 9「孝服：对质、千人醉、两条路」。挂在 ch05_mofu 小楼楼门左格 (28,9)
-- （mode=interact，once=true，guard_flag=ch05.huayuan，set_flag=ch05.duizhi）。
-- 推不推这扇门是他的事，他先放开灵识数了人才进（ch115）。**谈判桌上一仗也不打**：原著屋里没有埋伏。
--
-- 原著 ch115-119 这一场的骨架（施工图 13.1 的框架件），本节照留：
--   楼下放开灵识数人（ch115）→ 推门，满屋缟素孝服 → 二夫人问是不是他害的，他答得模棱两可
--   → 指尖一团火烧掉一件家具，墨府第一次说出「修仙者」（ch116）
--   → 他看破蜡烛里的千人醉 → 严氏打出阴毒这张牌，**「寒毒」一词在这里第一次出现**（ch117）
--   → 二选一 → 他把墨大夫的死讲一遍、严氏推给余子童、王氏绞着的手指（ch117）
--   → 条件：灭五色门与独霸山庄，宝玉在心腹手里（ch118）→ 「修仙者也不敢随便对凡人下手」的盘算（ch118）
--   → 两条路、给一天（ch118-119）。「五色门」「独霸山庄」本节首次出现（施工图 12.2：节点 1-8 不许有）。
-- 暗信怎么显形（ch112）他没看见，这里由严氏一句话带过：四个人各拿出一样东西，信上的字就变了
-- （转写高风险点 4）。
-- 骨架之外的质感（2026-09-25 整改，第 5 章校对 HIGH-1 表 C）：原著的停步、坐椅子、一对点了一小半的
-- 白蜡烛、没有烛香、脸不红心不跳、「双手奉上」「风吹草动」「百分之一」一概不用。蜡烛是他进门那一口
-- 烛烟就闻出来的（药师的鼻子）；王氏只留十根缠成一团的手指（语气卡），写法换了。
--
-- **自动核清灵散**（施工图 3.2）→ ch05.qianrenzui：
--   身上有一份就在楼下 take 一颗（看返回值）——原著他事先吃了一颗，ch122 才点破 → 1 防住；
--   没有（或扣不下）→ 千人醉起效，他抢先拍灭蜡烛、推开窗 → 2 没防住。
-- ch05.dengmen == 1（登门那夜他看清了三夫人那一手是功夫）：多一句他顶回去的话。
-- 二选一 → ch05.duizhi：1 冷冷一句「毒发之前先把你们杀干净」（原著）/ 2「那就谈」。取消按 2 算。

talk("", "ch05.duizhi.below")

local qianrenzui = 2
if item.count("pill_qingling_san") > 0 and take("pill_qingling_san", 1) then
    talk("", "ch05.duizhi.pill")
    qianrenzui = 1
end

talk("", "ch05.duizhi.up")
talk("", "ch05.duizhi.angry")
talk("li_shi", "ch05.duizhi.li")
talk("", "ch05.duizhi.answer")
talk("", "ch05.duizhi.fire")
talk("li_shi", "ch05.duizhi.xiuxian")

if flag.get("ch05.dengmen") == 1 then
    talk("", "ch05.duizhi.retort")
end

talk("", "ch05.duizhi.candle")
talk("yan_shi", "ch05.duizhi.yan1")

if qianrenzui == 1 then
    talk("", "ch05.duizhi.safe")
else
    talk("", "ch05.duizhi.dizzy")
end

talk("yan_shi", "ch05.duizhi.card")
talk("", "ch05.duizhi.card2")
talk("", "ch05.duizhi.q")

local pick = choice{
    "ch05.duizhi.opt_hard",
    "ch05.duizhi.opt_soft",
}

local duizhi = 2
if pick == 1 then
    talk("", "ch05.duizhi.hard")
    duizhi = 1
else
    talk("", "ch05.duizhi.soft")
end

talk("", "ch05.duizhi.tell")
talk("yan_shi", "ch05.duizhi.anxin")
talk("", "ch05.duizhi.blame")
talk("", "ch05.duizhi.wang")
talk("yan_shi", "ch05.duizhi.terms")
talk("", "ch05.duizhi.why")
talk("", "ch05.duizhi.roads")
talk("", "ch05.duizhi.react")
talk("", "ch05.duizhi.day")

flag.set("ch05.qianrenzui", qianrenzui)
flag.set("ch05.duizhi", duizhi)
