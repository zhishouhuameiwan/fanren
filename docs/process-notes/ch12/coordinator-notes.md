# 第 12 章协调者备注（2026-10-13，第 11 章已提交 6ec8073）

## 第 2 节「必须拿真档核」九项——已用真档核过（tests/fixtures/ch11-end-{first,second}.sav）

| 字段 | first | second | 与施工图估值 |
| --- | --- | --- | --- |
| day | 49378 | 49353 | 一致 |
| realm / realmCap / formerRealm | 31 / 31 / 22 | 同左 | 一致 |
| hp / maxHp、mp / maxMp | 700/700、470/470 | 同左 | 一致 |
| cultivation | 2572 | 2511 | 估「数百到两千」，实为两千五 |
| 低阶灵石 | 2339 | 2319 | 两侧差 20，一致 |
| 中阶灵石 | 20 | 20 | 一致 |
| party | qu_hun_shadan（active，hp 820） | 同左 | 一致 |
| learnedMagics | 11 门 | 同左 | 一致 |
| story_jinpa_ditu / story_bai_zhenzhu / weapon_qingzhu_fengyun_jian | 1 / 1 / 1 | 同左 | 一致 |
| story_xueyu_zhizhu / story_shijin_chong / story_yinhun_zhong / lvhuang_jian / miaoyin_yaopai / jinzhi_lingpai | 各 1 | 同左 | 一致 |
| bottle | 容量 9、9 滴 | 同左 | 一致 |
| mapId / position | ch11_dongfu (14,24) 朝北（patch 前） | 同左 | 入口版由测试路 A 重生成后应为 ch12_tushan 的 andeng |

## 其它

- 第 11 章契约第 6 节金青一格已改：金青在 ch11.done 后离场（hidden_flag），zhuzhong.lua 首行只剩防重入 return；第 12 章 patch 是在 zhuzhong.lua 末尾 flag.set("ch11.done") 之后补 teleport("ch12_tushan")。
- 第 12 章契约第 3 节「金丝蚕」一格已按 ch522 原文更正（两只撑到 522 后产死卵、双双毙命）。
- 原创性自检脚本：python -I C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/lcs_check.py --from 416 --to 499 <文案 json…>（第 11 章 415 条全过）。
