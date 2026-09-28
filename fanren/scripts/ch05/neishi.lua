-- @hook ch05_kezhan trigger_neishi interact
-- 第五章「内视」。挂在 ch05_kezhan 上房蒲团左格 (11,7)（mode=interact，**once=false**，
-- 无 guard、不写 set_flag——规则 15；全章唯一一处可重复的挂点）。蒲团右格是打坐的 facility。
--
-- 看阴毒到了哪一段，**只说不扣**（施工图 3.3：发作只在五个检查点上）。三段的界与检查点同一组数：
-- 这一份不算检查点，数写在这里只为说得对哪一段；改检查点那五处时这里一并改。
--   段一 d < 65：花椒籽大，还在长
--   段二 65 ≤ d < 80：枣核大，夜里冻醒
--   段三 d ≥ 80：压不住了；d ≥ 90 再添一句「过了期限，不知还能撑几天」
--   宝玉到手（ch05.done）：停表，黑团不长了
-- 文案里不说「寒毒」：客栈在节点 4 就进得来，而那个词首见 ch117，本作放在节点 9 才说出口。

local HANDU_DUAN2 = 65
local HANDU_DUAN3 = 80
local HANDU_QIXIAN = 90

if flag.get("ch05.done") ~= 0 then
    talk("", "ch05.neishi.stop")
    return
end

local qi = flag.get("ch05.yindu_qi")
if qi <= 0 then
    talk("", "ch05.neishi.none")
    return
end

local d = today() - qi
if d >= HANDU_DUAN3 then
    talk("", "ch05.neishi.d3")
    if d >= HANDU_QIXIAN then
        talk("", "ch05.neishi.over")
    end
elseif d >= HANDU_DUAN2 then
    talk("", "ch05.neishi.d2")
else
    talk("", "ch05.neishi.d1")
end
