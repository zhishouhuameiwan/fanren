-- @hook ch11_yinsha_dao trigger_guiju enter once
-- Facts and participants: docs/ch11-design.md 13.1 n22. 败 game_over。
talk("zi_ling", "ch11.guiju.order")
talk("", "ch11.guiju.break")
talk("", "ch11.guiju.circle")
talk("", "ch11.guiju.shayao")
local won = battle("b11_yinsha_shayao")
if not won then game_over(); return end
talk("", "ch11.guiju.ash")
talk("yinsha_menzhu", "ch11.guiju.menzhu")
talk("zhao_zhanglao", "ch11.guiju.zhao")
talk("", "ch11.guiju.backfire")
talk("fan_furen", "ch11.guiju.trap")
flag.set("ch11.guiju")
