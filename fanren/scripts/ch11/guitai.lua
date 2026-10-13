-- @hook ch11_xiaoxuan trigger_guitai interact
-- Facts and participants: docs/ch11-design.md 3.2「柜台」。卖出口是脚本不是 shop facility（O-6）。
if flag.get("ch11.liugen") >= 1 then talk("", "ch11.guitai.closed"); return end
talk("", "ch11.guitai.open")
shop("ch11_qingzhu_xiaoxuan")
