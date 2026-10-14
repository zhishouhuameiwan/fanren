-- @hook ch12_xiaojie trigger_caiyao interact once
-- 小空间采药（拍板 32，docs/ch12-design.md 3.2、13.1 末行）：现有灵草一种 ×2；置 ch12.caiyao。不在目标链上。
-- 灵草 id 待协调者定（16.2 O-13）；暂用年份上限 2000 的黄精芝、年份 300。文案不点名，换 id 只改下面这一行。
talk("", "ch12.caiyao.find")
give("herb_huangjing_zhi", 2, 300)
talk("", "ch12.caiyao.dig")
talk("", "ch12.caiyao.rest")
flag.set("ch12.caiyao")
