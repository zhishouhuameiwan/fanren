-- @hook ch05_kezhan trigger_chaoxie interact once
-- 第五章支线 Z1「墨凤舞的医书」第一步：誊抄。挂在 ch05_kezhan 上房书案右格 (9,2)
-- （mode=interact，once=true，guard_flag=ch05.fengwu_qiu，set_flag=ch05.fengwu_chao）。誊抄是他。
--
-- ch124 墨凤舞求他把墨大夫的医道心得誊一份，他答应次日带来。本作让他在客栈书案上抄手札的**前半本**
-- （方子与医案；后半本记的是另一回事，第 3 章 ch03.tiezhe.book，他不抄），拨一天，得一份遗稿抄本。
-- 交付在墨府药圃（scripts/ch05/fengwu.lua）。
--
-- 过期：ch05.done 置位时还没交，任务过期（施工图第 6 节）。章末之后再来按这张桌子，
-- 只说一句「用不上了」，在置旗标之前 return——抄本不再发。
-- 手札（story_mo_shouzha）是第 3 章给的剧情物品、不可交易，正常流程里一定在；真不在就说一句、不抄。

if flag.get("ch05.done") ~= 0 then
    talk("", "ch05.chaoxie.late")
    return
end

if item.count("story_mo_shouzha") == 0 then
    talk("", "ch05.chaoxie.noshouzha")
    return
end

talk("", "ch05.chaoxie.start")

advance_days(1)

talk("", "ch05.chaoxie.work")
give("story_yigao_chaoben", 1)
talk("", "ch05.chaoxie.done")

flag.set("ch05.fengwu_chao")
