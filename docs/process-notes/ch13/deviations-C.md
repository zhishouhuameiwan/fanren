# 第 13 章 内容路 C（脚本与主线文案）施工记录

2026-10-14。白名单：`scripts/ch13/**`、`scripts/ch12/lingquan.lua`（末行一行）、`data/text/ch13_{nanming,tianxing,qiyuan,wuzhong,mishi,battle}.json`、`docs/ch13-design.md` 18.1。

## 落地

- 脚本 40 份：主线 34（文件名照 3.1 的 id 去前缀；`trigger_shoushou` → `chongmo.lua`、`trigger_jingshimen` → `biguan.lua` 等照地图）＋ `fangshi_bimen`、`xuanshang`（Z2）、`yanghun`（Z3）、`feixu`、`qiyuan_hang`（#6，落 `ch13_waihai` (7,12)，与 `genmaps_ch13.py` QIYUAN_REEF 同字面量）、`hui_wuzhong`（#7）。
- `lingquan.lua`：第 25 行 `flag.set("ch12.done")` 之后补 `teleport("ch13_nanming")`；头注释第 4 行「不 teleport（第 13 章 patch 本脚本）」半句改为现状，其余一字未动。
- 文案 405 条，汉字约 11.3k。按幕分文件：nanming（n01–n06）、tianxing（n07–n12 ＋ 坊市闭门）、qiyuan（n13–n16 ＋ Z2、废墟、#7）、wuzhong（n17–n30 ＋ Z3、#6）、mishi（n31–n34）、battle（两场的 intro / charge 四条）。
- 胜利文案放在调用它的场景前缀下（`ch13.zhuibing.win`、`ch13.youyao.win`），与第 12 章 `guiwang.win` 同法。

## 门闸与退路（脚本侧）

- 置旗前取消即 return：n03、n04、n05、n06、n07、n11、n12（扣物前）、n14、n27、n29（扣物前）、n30、n31（第一次扣物前）、#6、#7。
- NPC 节点开头判 done_flag 与前置（n02 wenxun、n04 gongdao、n06 zhuibing、n12 yirong、n14 daoqiyuan、n15 mingzhu、n29 chuguan、n32 jiaohuan），未到一句 return、不置旗。
- 数量门闸：n24（biguan / 缺草两档 / 缺液）、n25（lieyao < 2）、n28（realm.at_least）、Z2、Z3；三处付出（n12 / n29 / n31）都只列付得起的支，零成本支永远在。
- 返回值：每处 take、bottle.spend、battle、magic.learn、realm.cap 都分支；battle 输了读 won 自调 game_over，② 读 how == "escaped" 不扣草。

## 偏差（同 docs/ch13-design.md 18.1）

1. `field.unlock` 无返回值，n17「unlock false 换一句」无从判。
2. 许云、易敬只在 n13（祭坛下）说话；n14 明珠轩他独自一人，许云导览拆成 n13 许云 / n14 明夫人 / 竞宝厅老者三处；n15 分手一句旁白。
3. n32 洞府开在秘市荒岛东侧（照地图 4.1），不写「两日后另一座荒岛」。
4. n31 抉择次序照 3.2（铜片 → 换她 → 范夫人），叙事改成先买铜片；`advance_days(5)` 挪到铜片抉择之后；第一次扣物后的两处取消按默认支。
5. 只剩零成本一支时仍出单选项（n12、n29、n31）。
6. n29「回话」两支由本路定（入阵为仆 / 先问想清楚）。
7. n05「随行」支：途中索要储物袋、他不交、对方先动手。
8. n01 舞台：从海上问话说到南明岛礁石；三仙宗老者自报去南明岛歇脚。
9. n24 胜后 take 草 false 仍记一胜（换一句）；Z3 take 根须 false 照给木珠。
10. n31 主路 16 句，超「4–12 句」；n29「先问」支 13 句；其余 ≤ 12。
11. 契约 6.4 与派工对 `ch13_{items,path,quests,objectives}` 的归属不一致，照派工没写。
12. ② intro 与 `data/text/battles.json` 旧占位重名，待协调者删旧占位（契约 5.2）。

## 自检

- `luac -p`（vendor/lua-5.4.9 临时编的 luac，产物在 scratchpad）：40 份 ＋ lingquan.lua 全过。
- validate：27 错 ＝ 26 条 `ch13.path.*` 未登记（等协调者登 `data/pathactions/ch13.json` 的旗标）＋ 1 条上面第 12 项的重复 key；本路脚本的 @hook、文案 key、说话人、物品、旗标、传送目标 0 错。
- 禁词与首见次序（12.2 全表 ＋ LexiconTests 表一首见 > 542 的 14 词 ＋ 13 第 11 条点名台词关键词）：LEXCHECK_OK；选项 5–20 字、单条 ≤ 80 字。
- 原创性：`lcs_check.py --from 500 --to 542` → SELF_CHECK_OK，checked=405 over=0。首轮 over=26，逐条改说法；之后又按 13.1 质感边界逐段对照原著 499–542（novel.py 全读）改掉约 50 处镜头 / 台词结构照搬（开价、易敬拍板、看护那句、离寻「招待」、乾姓「七人」、儒生验脸、凌玉灵邀约与婉拒、许云导览、明夫人收媚、兽卵、虫魔三人、黄明礼引荐、文思月警告、章末入洞等）。
