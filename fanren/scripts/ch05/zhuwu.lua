-- @hook ch05_mofu trigger_zhuwu enter once
-- 第五章节点 12b「吴剑鸣」。十场必打之八：b05_wu_jianming。
-- 挂在 ch05_mofu 马厩院院门 (11,29)(11,30)（mode=enter，once=true，guard_flag=ch05.shigui，
-- set_flag=ch05.chuzheng）。吴剑鸣从侧门夺路，是撞上的——所以是踏入型；
-- 院门是马厩院唯一的出口，打完尸傀出来必过。**寒毒检查点之三**（施工图 3.3）。
--
-- **改编**（施工图 3.2 / 8.3）：撬地窖的人从侧门夺路——是吴剑鸣。ch106 韩立就判定他是来「试探」的；
-- ch109 严氏说过实在拖不下去就「翻脸擒下他」。**真假当面对上的只有这一次。**
-- **他总会被擒**（第 5 章校对 MEDIUM-2 的拍板）：走脱了去给师父报信，就拆了 ⑩「乘其不备」的原著前提。
--   赢 → 韩立亲手按倒，擒下交给严氏；
--   逃 → 韩立抽身退开，吴剑鸣扭头奔侧门，被侧门口的护院堵住、围在墙角按倒（ch05.zhuwu.blocked）；
--   输 → 韩立单膝跪倒，护院从三面围上来把吴剑鸣压住（ch05.zhuwu.felled）。按 how 分两句
--     （第 5 章复验 LOW-b：玩家的逃不许叙述成对方的逃，输的时候也谈不上「见势不好」）。
--     「多挨的那几下」就是这一仗本身打掉的血：输了只剩一口气，逃的人本来就伤了一半；不另扣——
--     下面骑马十天，到山庄外时也都养回来了（defeat_is_fatal 假，输了不是 game_over）。
--   两条路都记 ch05.qinwu = 1，**下场不写**（原著 ch123 之后不再提他）。
-- 然后严氏给画像与一匹好马（ch126），拨一天（他第二天一早动身）＋ 骑马十日，teleport 到山庄外林子，
-- 落在 genmaps_ch05.py 的 SHANZHUANG_LINZI (4,25)，置 ch05.chuzheng。

-- ---------------------------------------------------------------------------
-- 寒毒检查点（docs/ch05-design.md 3.3）。本章五个检查点（dingji / anpai / zhuwu / tancha /
-- huanyu）写的是同一段，改一处就改五处；ScriptHost 关了 package 与 dofile，脚本之间共用不了函数。
-- 发作一次吞一颗养精丹顶住是游戏规则，原著没有这一笔（改编）；超期不判输（用户拍板），只多一句话。
-- ---------------------------------------------------------------------------
local HANDU_DUAN2 = 65    -- 发作起第 65 日入第二段：进段后的第一个检查点发作一次
local HANDU_DUAN3 = 80    -- 第 80 日入第三段：此后每个检查点各发作一次
local HANDU_QIXIAN = 90   -- ch104「顶多再有两个月」是 d≈30 时说的，即 d = 90

local function handu_fazuo(chaoqi)
    talk("", "ch05.handu.attack")
    if item.count("pill_yangjing_dan") > 0 and take("pill_yangjing_dan", 1) then
        talk("", "ch05.handu.pill")
    else
        talk("", "ch05.handu.nopill")
    end
    if chaoqi then
        talk("", "ch05.handu.over")
    end
end

local function handu_jiancha()
    local qi = flag.get("ch05.yindu_qi")
    if qi <= 0 then return end
    local d = today() - qi
    if d >= HANDU_DUAN3 then
        flag.set("ch05.handu_fa", 3)
        handu_fazuo(d >= HANDU_QIXIAN)
    elseif d >= HANDU_DUAN2 and flag.get("ch05.handu_fa") < 2 then
        flag.set("ch05.handu_fa", 2)
        handu_fazuo(false)
    end
end

handu_jiancha()

talk("", "ch05.zhuwu.run")
talk("", "ch05.zhuwu.face")
talk("", "ch05.zhuwu.true")

local won, how = battle("b05_wu_jianming")

if won then
    talk("", "ch05.zhuwu.won")
elseif how == "escaped" then
    talk("", "ch05.zhuwu.blocked")
else
    talk("", "ch05.zhuwu.felled")
end

talk("yan_shi", "ch05.zhuwu.horse")

advance_days(1)
talk("", "ch05.zhuwu.ride")
advance_days(10)

flag.set("ch05.qinwu", 1)
flag.set("ch05.chuzheng")
teleport("ch05_dubashanzhuang", 4, 25)
