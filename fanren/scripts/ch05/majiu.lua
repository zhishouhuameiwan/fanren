-- @hook ch05_mofu trigger_majiu interact once
-- 第五章节点 12a「墨府尸傀」。十场必打之七：b05_mofu_shigui（遗留编成，编成与数值不动）。
-- 挂在 ch05_mofu 马厩的门 (5,27)（mode=interact，once=true，guard_flag=ch05.anpai，set_flag=ch05.shigui）。
-- **他自己去马厩牵马**，这才撞上被撬开的地窖（施工图 3.1）。前院的存档点在马厩院之前。
--
-- **改编**（遗留编成接入，用户拍板留下；由头见施工图 8.3）：
-- 修仙者的遗物——墨居仁年轻时在城外亲眼见过修仙者斗法（ch116 严氏：老爷也是见过的人之一），
-- 从那场争斗里拖回一具尸傀，封在马厩下的地窖，谁也不许下去。吴剑鸣夜里撬开石门查墨府的底；
-- 门一开，它挨近一个有法力的人就醒了，头一个走近它的有法力的人就是韩立。
--   不是墨居仁炼的（ch64：他只来得及炼成张铁一具）；不是独霸山庄的（ch283：山庄与修仙界毫无往来）；
--   不是五色门的（五色门背后的实力 ch251 才揭开，本章不抢）。
--   ch283 那一句说的是「当初的岚州墨府、独霸山庄」**两家**都没接触过修仙界的修士（第 5 章校对 LOW-3）：
--   墨府地窖里封着一件修仙者斗法留下的东西、严氏知情，不等于墨府与修士有往来——墨居仁只是见过斗法，
--   从现场拖回了一样东西；没有一个修士知道它在这儿，也没有一个修士来找过墨府。
--   **不认领**三夫人 ch123 那句「欧阳飞天不敢进攻此地，自然有他的道理」——那个道理原著没说，本作也不替它说。
-- **转写高风险点 9**：尸傀出场不套原著别处任何炼尸、傀儡的描写，句子全是自己的。
-- 撬门的人是谁，本节只留一串通到侧门的脚印；节点 12b 他从侧门夺路，才照上面。
-- 输即 game_over（defeat_is_fatal 真；can_escape 假）。

talk("", "ch05.majiu.morning")
talk("", "ch05.majiu.door")
talk("", "ch05.majiu.below")
talk("", "ch05.majiu.guards")
talk("", "ch05.majiu.feel")

local won = battle("b05_mofu_shigui")
if not won then
    talk("", "ch05.majiu.lost")
    game_over()
    return
end

talk("", "ch05.majiu.won")
talk("yan_shi", "ch05.majiu.yan")
talk("", "ch05.majiu.who")

flag.set("ch05.shigui")
