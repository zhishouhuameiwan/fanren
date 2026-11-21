-- 调试用：女角色立绘巡览（docs/heroine-art-build.md 3.5.3；试看脚本 build-s7\看立绘-七位巡览.bat 用 --event 演它）。
-- 不挂在任何地图上，不进正式流程：七张登场卡依次出，每张之后那个人说一句（借用第 7 章现成的话，不新增文案；
-- 霓裳仙子、白衣女子在第 7 章没有自己的话，借的是写她们的那一句旁白）。不置旗标、不给东西。

intro_card("chen_shimei")
talk("chen_shimei", "lilian.jindi.chen.say")
intro_card("nichang_xianzi")
talk("nichang_xianzi", "ch07.liedui.yanyue")
intro_card("duobao_nv")
talk("duobao_nv", "ch07.shulin.chaser", "angry")
intro_card("huangshan_shijie")
talk("huangshan_shijie", "ch07.shulin.help", "hurt")
intro_card("baiyi_shaonv")
talk("baiyi_shaonv", "ch07.guanzhan.retreat")
intro_card("nangong_wan_ch07")
talk("nangong_wan_ch07", "ch07.jiaoshi.name_say")
intro_card("han_yunzhi")
talk("han_yunzhi", "ch07.xiaoshidian.after_flower", "smile")
