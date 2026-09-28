-- 负向：灵田 id 为空。api.lua 侧的 assert 该当场炸掉，脚本不再往下走。
field.unlock("", 4)

-- 探针：assert 没拦住的话脚本会继续，留下这个旗标。测试断言它不存在。
flag.set("empty_field_id_slipped_through")
