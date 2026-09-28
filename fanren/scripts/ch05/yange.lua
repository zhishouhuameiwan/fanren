-- @hook ch05_mofu trigger_yange enter once
-- 第五章节点 11b「燕歌讨教」。十场必打之六：b05_yange_qiecuo。
-- 挂在 ch05_mofu 月洞门外格 (24,23)（mode=enter，once=true，guard_flag=ch05.jiaoyi，
-- set_flag=ch05.yange）。燕歌在他出府的路上拦下他——是燕歌找他，所以是踏入型；
-- 月洞门是后头通前院唯一的口子，从小楼出来出府必过。
--
-- **改编**（施工图 3.2 / 8.3）：他听说四师娘把三位小姐都许了这个师弟。ch114 他亲口说过，玉珠不喜欢他，
-- 他只盼她找个好夫君；ch106 墨玉珠定亲后，不服的求亲者纷纷上门向那位未婚夫挑战——那是一桩事，
-- 不是城里的规矩（第 5 章校对 LOW-2；台词 ch05.yange.why 只说「这一年上门找人讨教的不少」）。
-- 他要讨教一招，看看这个人配不配。
-- 不冲突的依据：燕歌在 ch115 之后原著再没出现；ch124 韩立出府一句带过，中间没有写。
-- **他是认真的**（施工图第 5 节语气卡），别写成笑料。
--
-- 切磋：can_escape 真、defeat_is_fatal 假（同第 4 章 b04_qiecuo_feiyu）。三种落点，本节并成两条：
--   赢 → ch05.yange = 1；输或逃 → 2（施工图 3.2「2 输或走」）。
-- 打完韩立告诉他：他没答应。燕歌愣了很久。12e 他送不送，就看这一位。

talk("", "ch05.yange.stop")
talk("yan_ge", "ch05.yange.heard")
talk("yan_ge", "ch05.yange.why")
talk("yan_ge", "ch05.yange.ask")

local won = battle("b05_yange_qiecuo")

local yange = 2
if won then
    talk("", "ch05.yange.won")
    yange = 1
else
    -- "lost" 与 "escaped" 走同一条：他没把这一场打完，燕歌也没弄明白是赢了还是被让了。
    talk("", "ch05.yange.lost")
end

talk("", "ch05.yange.tell")
talk("", "ch05.yange.silent")

flag.set("ch05.yange", yange)
