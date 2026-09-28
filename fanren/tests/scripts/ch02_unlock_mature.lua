-- 只解锁催熟，不拾瓶。用来证明 bottle_unlock_mature 不替脚本兜底置 owned：
-- 「没瓶子却会催熟」是个写错了的状态，它必须在测试里看得见，而不是被悄悄补上。
bottle.unlock_mature()
