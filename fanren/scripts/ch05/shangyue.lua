-- @hook ch05_dubashanzhuang trigger_shangyue interact once
-- 第五章节点 12d「欧阳飞天」。十场必打之十：b05_ouyang_feitian。
-- 挂在 ch05_dubashanzhuang 后园赏月亭 (24,5)(25,5)（mode=interact，once=true，guard_flag=ch05.tancha，
-- set_flag=ch05.cisha）。挑欧阳飞天独自赏月那一夜下手——是他挑的时候（ch126）。
--
-- **没练成剑符就不照面**（第 5 章复验 MEDIUM-B，协调者拍板方案 1，施工图 16.1 第 16 条）：
--   剑符是取他首级的唯一办法（MEDIUM-1；霸王甲在数据里是「只有金要得了他的命」，
--   data/roles/ouyang_feitian.json 的 killable_by）。11a 严氏说过刀剑难伤，12c 他亲眼看见刀弹开，
--   手里没有要得了命的东西就不出手：伏在墙头看一夜，悄悄退回林子，**不开这一仗、不被发现**
--   （nofu / repelled / repelled_hint），在置旗标之前 return；林子里另挂一处练符（Z2′）。
--   不拨日子：当夜就退，练符那两天在 Z2′ 里拨。于是 ⑩ 每个人都打、只打一次——持符的那一次，
--   两侧都合原著的「乘其不备」、把剑符当暗器才没躲（12e huanyu.fu）。
-- 练成了剑符（ch05.lian_jianfu == 1）：开战前把「祭剑符」借给他（看返回值），
--   战后不论胜负 magic.forget（施工图 10.3：只借这一仗）。欧阳飞天当它是暗器，没躲（ch126）——
--   (40×2−32×0.5)×1.6 = 102 ≥ 90，第一回合一招取首级，照原著。借不到（引擎拒绝）也按没练成办。
--   can_escape 假：持符上了亭子就只有这一仗，逃了再来就拆了「乘其不备」（复验 §5.2 末句）；
--   输 → game_over（defeat_is_fatal 真；韩立先手，只有不用剑符的人才输得了）。
-- 得手之后骑马回城的那十天拨在这里、然后 teleport 进墨府前院 (23,30)
-- （genmaps_ch05.py 的 MOFU_FRONT_YARD）：人落在墨府时，日子已经是回城之后的日子了。
-- 施工图 3.2 把这十天写在 12e 开头；挪到这里是为了「人在哪儿」与「今天是哪天」不打架，
-- 12e 开头的寒毒检查点看到的仍是回城之后的 d（施工图 3.3 那张表的 59 / 62）。已记进施工偏差。

local borrowed = false
if flag.get("ch05.lian_jianfu") == 1 then
    borrowed = magic.learn("magic_ji_jianfu")
end

if not borrowed then
    talk("", "ch05.shangyue.nofu")
    talk("", "ch05.shangyue.repelled")
    talk("", "ch05.shangyue.repelled_hint")
    return
end

talk("", "ch05.shangyue.night")
talk("", "ch05.shangyue.who")
talk("", "ch05.shangyue.fu")

local won = battle("b05_ouyang_feitian")

-- 不论胜负先还回去：这一招只借这一仗。
magic.forget("magic_ji_jianfu")

if not won then
    talk("", "ch05.shangyue.lost")
    game_over()
    return
end

talk("", "ch05.shangyue.won_fu")

talk("", "ch05.shangyue.head")

advance_days(10)

talk("", "ch05.shangyue.back")

flag.set("ch05.cisha")
teleport("ch05_mofu", 23, 30)
