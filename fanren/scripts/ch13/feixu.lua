-- @hook ch13_waihai trigger_feixu enter
-- 兽潮后的北区（docs/ch13-design.md 16.1 第 49 条）：guard ch13.shouchao、once 否、无 set_flag；
-- 每次走进只一句废墟旁白，不拦路；0 处 flag.set、0 处 teleport。文案编在 gongsun（12.2）。
talk("", "ch13.feixu.ruin")
