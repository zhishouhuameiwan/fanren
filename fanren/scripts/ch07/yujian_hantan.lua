-- @hook ch07_huanxingshan trigger_yujian_hantan interact once
-- 第七章支线 Z2「钟吾的玉简」之二：北坡寒潭。挂在 ch07_huanxingshan 寒潭边那一格 (33,6)
-- （mode=interact，once=true，guard_flag=ch07.yujian_qiu，set_flag=ch07.yujian_b）。全部自出（施工图第 6 节）。
-- 给成熟的天灵果 2（300 年）。过期：ch07.didao。

talk("", "ch07.yujian_hantan.pond")
talk("", "ch07.yujian_hantan.find")
give("herb_tianling_guo", 2, 300)
talk("", "ch07.yujian_hantan.cold")

flag.set("ch07.yujian_b")
