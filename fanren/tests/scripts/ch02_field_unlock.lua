-- 槽位数由 C++ 预置的旗标给出，一个夹具覆盖「新开 / 补足 / 不缩小 / 槽位非法」
-- 四种情形。写死数字就得为每种情形多放一个几乎一样的 .lua，改口径时必漏一个。
field.unlock("shenshougu_yaopu", flag.get("test_slots"))
