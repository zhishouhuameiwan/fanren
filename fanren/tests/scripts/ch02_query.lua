-- 六个新查询各读一次，结果拼进文案 key 带回 C++。
-- 查询是 C++ 注册的函数：它们同步返回、不挂起，因此可以任意穿插在 yield 之间。
talk("probe", "d." .. tostring(today()))
talk("probe", "b." .. tostring(bottle.owned()) .. "." .. tostring(bottle.mature_known()) ..
     "." .. tostring(bottle.drops()))
talk("probe", "one." .. tostring(field.planted("shenshougu_yaopu")) ..
     "." .. tostring(field.ripe("shenshougu_yaopu")))
-- 省略 field_id：按契约统计所有田的合计。
talk("probe", "all." .. tostring(field.planted()) .. "." .. tostring(field.ripe()))
-- 不存在的田返回 0，不报错：剧情闸门问「种了几株」，田还没开时答案本就是零。
talk("probe", "none." .. tostring(field.planted("no_such_field")) ..
     "." .. tostring(field.ripe("no_such_field")))
