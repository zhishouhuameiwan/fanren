-- 写走命令队列、读走 __host 的端到端验证：
-- flag.set 挂起后由 game 层落到 GameState，随后的 flag.get 才读得到新值。
talk("a", "before." .. tostring(flag.get("story_bit")))
flag.set("story_bit", 7)
talk("a", "after." .. tostring(flag.get("story_bit")))
