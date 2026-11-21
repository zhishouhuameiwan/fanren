# 第 7 章第二期「同道录」2a 集成记录

> 2026-11-21。施工图：`docs/ch07-tongdaolu.md`（第 2、3、4 节与 4.1 验收）；三路记录：`docs/ch07-tongdaolu-k.md`（规则与数据）、`docs/ch07-tongdaolu-u.md`（界面）、`docs/ch07-tongdaolu-n.md`（内容与美术）。
> 分支 `claude/wizardly-kilby-28c726`，不推送、不合 main。构建目录 `build-i`（不入库）。

---

## 1. 合并情况

| 步 | 做法 | 结果 |
| --- | --- | --- |
| 带上路 U（连同路 K） | `git merge --ff-only claude/musing-darwin-bc16f7` | 快进到 `ac094c4`（路 K `4d8f7c1`、`995b02f`，路 U `2e363bc`、`ac094c4` 都在） |
| 并入路 N | `git merge --no-ff claude/strange-mestorf-ab5b20`，提交「merge: 同道录 2a 路 N 并入路 U」 | `8eaff0c`，**没有冲突**（路 N 的 `dfac9f8`、`1989b7c` 都在）。44 个文件、+1848 −22，换行符没被改（`--stat` 与 `--ignore-cr-at-eol` 两个数一样） |

合并后、动手之前的全量 ctest（`build-i`，`-j 4`）：**2540 条全过**（路 U 交来时 2529 + 路 N 的 11 条），`CTEST_EXIT=0`。

## 2. 接缝走查（真程序、真数据）

两种拍法，图都在 `build-i/shots/`（不入库）：

- **截图口**（`build-i/fanren.exe --scene …`，脚本 `build-i/shots-i.sh`）：一屏一屏定格，文件名 `iNN_*.png`。存档用检查点 `tests/fixtures/companions/ch07-chujindi-first.sav`，战斗那几张用 `build-i/saves/tongdao-shot-party.sav`（三位都在队里、陈 95 / 南宫 94 / 菡 95）。
- **走查**（`tests/TongdaoWalkShots.cpp`，`FANREN_TONGDAO_WALK=<目录>` 才跑，`build-i/walk.bat`）：截图口拍不到「走着走着才出来」的东西——切磋那张教学卡、战斗里给我方择人的菜单、叙话一句句的立绘、事件图淡入淡出、升档提示、蒲团同修。它用真树（`data/`、`scripts/`、`assets/`），非无头、软件渲染，按键照真入口发 SDL 键盘事件，开关与 `main.cpp` 正常游玩时同一组（遭遇、教学卡、蒲团两项都开）。图在 `build-i/shots/walk/`，`w*` 是乙那条路（三档门口）、`x*` `y*` 是丙那条路（最高档门口与历练一仗）。两条用例跑通，断言全过（见 2.4 末）。

### 2.1 同道录页

| 查什么 | 结果 | 图 |
| --- | --- | --- |
| 三位的卡片（立绘头肩、档名、默契条） | 对。刚出禁地：陈巧倩、菡云芝「初识」，南宫婉「相熟」（37）带「可叙话」角标 | `i01_cards_chujindi` |
| 路 N 的 `tongdao.<id>.intro` 接上（卡片一行、详情一行） | 接上了；卡片上长的截断加「…」，详情里整句放得下。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 6 条**（卡片改用短句 `tongdao.<id>.brief`） | `i01`、`i02_detail_han_tier1` |
| `tongdao.<id>.hint`（剪影） | 真数据里三位在检查点上都已结识，剪影这一屏拍不到；hint 的接法由路 U 的 `TheRosterPageIsNowTongdaoAndListsEveryCompanionWithSilhouettesForTheUnmet` 钉着，三条文案由路 N 的测试钉着「都有、不点名」 | — |
| 菡云芝的卡片、立绘、莫逆线档名 | 立绘（手捧符纸那张）、档名「初识 / 相熟 / 信任 / 知己 / 莫逆」都对 | `i02`、`i07_detail_han_tier3`、`i08_detail_han_tier5_moni` |
| 陈巧倩红颜线档名 | 「信任」「红颜」对 | `i04_detail_chen_tier3_qingjiao`、`i05_detail_chen_tier5` |
| 「已有 / 下一档」从路 N 的表生成的句子 | 通。例：菡云芝二档的下一档「「信任」：法术「回春符」、被动：打赢后全队回 10% 气血、可请教、叙话一段」。**最高一档时「已有」截断难看——修了**（4.1） | `i09_detail_han_tier2`、`w06_han_detail_tier3`、`i05`、`i08` |
| 三条在册加成的说明与「带上」 | 说明对（偷袭得手时全队开场再多 1 点劲 / 你的火属法术打中破绽多削 1 点架势 / 用符的威力 +20%），请教后「生效中」、名额 n/4。「带上」那一行在名额没满时写「未带上」而说明写「生效中」，**交主会话**（5.1） | `i04`、`i06`、`i19_detail_han_bonus`、`x07_chen_t5_after` |
| 南宫婉五档那格「机缘未到」 | 对；默契 94 时进度写「机缘未到」，下一档「机缘未到，眼下只能到这里。」 | `i06_detail_nangong_tier4_capped` |
| 不在百药园 | 四项灰着写「回百药园后可相处」 | `i13_detail_han_away` |
| 喜好（一档不露、二档全列、整段换行） | 对；南宫婉四样喜欢放不下一行时「不喜：兽材」整段到第二行 | `i02`、`i06` |

### 2.2 相处四个动作（百药园）

| 查什么 | 结果 | 图 |
| --- | --- | --- |
| 赠礼 | 二档列表行尾标「合意」；送寒蟾酥 42 → 47，回话「菡云芝收下了，很合心意。默契 +5。」，菜单停在详情，赠礼那行灰「今天已经送过了」。**背包多过一页时「1/25」刻度压在最后一行的数量上——修了**（4.2） | `w03_han_gift_list`、`w04_han_gift_pick`、`w05_han_gift_done`、`i11_gift_han_tier2`、`i12_gift_chen_tier1` |
| 升档提示 | 屏幕上沿「默契渐深：菡云芝 · 信任」「默契渐深：陈巧倩 · 红颜」 | `i16_tierup_han`、`i17_tierup_chen_hongyan`、`x04_world_tierup_notice` |
| 入册提示 | 「已记入同道录：菡云芝」 | `i18_joined_han` |
| 叙话（一次升两档要演两回） | 菡云芝从二档直接到三档：先演二档（金竺笔，8 句），再演三档（8 句）；立绘表情随句换（normal / hurt / angry / smile）；每段演完记账（`xuhua` 2 → 3）；「可叙话」角标在两段都演完后消掉 | `w07_han_t2_01…08`、`w09_han_t3_01…08`、`w08_han_detail_after_t2`、`w11_han_detail_bonus_on` |
| 菡云芝二档给 3 张火球符 | 演完火球符 8 → 11（走查断言）；重演不再给由路 N 的 `ElevenTalksAndThreeLessonsRunHeadlessAndKeepTheirBooks` 钉着（界面上叙话演过就不能再演，重演只有脚本口） | `w07_han_t2_08` |
| 请教 | 8 句演完，`qingjiao` 记 1，在册加成随之「生效中」 | `w10_han_qingjiao_01…08`、`w11` |
| 切磋 | 菡云芝（三档）：头一回轮到下令弹「切磋」教学卡；演出、破绽卡、蓄劲卡照常；只会普攻 5 回合赢（路 N 9.1 已裁定「最温和的一场」）；打完**气血法力回到开打前的数**（走查断言）；默契 47 → 51；回话「切磋胜了菡云芝。默契 +4。首胜之礼：定神符 ×3、火球符 ×2。」——她开打时已在三档，二、三两档的首胜奖品一起发（跳档不丢），得物卡片照常推 | `w12_spar_tutorial_tongdao_spar`、`w13_spar_first_command`、`w14_spar_target`、`w15_spar_midway`、`w15b_spar_result`、`w16_spar_after` |
| 蒲团「打坐 / 同修」 | 头一回弹「同修」教学卡；两项、选人（档名在行尾）；同修陈巧倩：默契 +4、过一天、劲挂到第二天（走查断言） | `w17_cushion_tutorial_tongdao_tongxiu`、`w17_cushion_choose`、`w18_cushion_pick`、`w19_tongxiu_done`、`i14_cushion_choose`、`i15_cushion_pick` |

### 2.3 陈巧倩五档事件图 `chen_qiaoqian_hongyan`

| 查什么 | 结果 | 图 |
| --- | --- | --- |
| 淡入 | `show_cg` 之后 0.3 秒铺满 | `x05_cg_on_mid` |
| 垫在对话框下 | 对；名牌、对话框、小头像照常 | `i20_cg_chen_hurt`、`i21_cg_narration`、`i22_cg_chen_smile`、`x06_chen_t5_03…05` |
| 图上不叠立绘 | 对：她开口的「韩师弟……你若肯说一句，我就——」「……罢了。…」两句只有小头像 | `i20`、`i22` |
| `hide_cg` 后淡出 | 对；淡出后下一句「今夜的话……」立绘回来 | `x05_cg_off_mid`、`x06_chen_t5_06`、`x06_chen_t5_07` |
| 记账 | `xuhua` 记 5，图撤干净（`currentCg()` 空，走查断言） | `x07_chen_t5_after` |
| 画面底线 | 两人衣着整齐、隔一步多、无接触，韩立只有背影 | `i20` |

### 2.4 战斗里的新东西（真数据）

历练明雷「火焰鼠」（`bl07_huoyan_shu_t1`）带三位，走查与截图口各拍一遍：

| 查什么 | 结果 | 图 |
| --- | --- | --- |
| 按档的法术进菜单、行尾说明 | 陈巧倩：飞剑术、双剑并出（2 段）、剑幕（全队减伤 30%）；南宫婉：朱雀环、流火（敌方全体）、环护（挡一击）；菡云芝：寒冰符、回春符（回气血 36）、定身符（定身） | `i31_battle_magic`、`y_*_0_magic` |
| 法术说明区的字（`ac094c4` 补的五种） | 光标挪到那一门上拍：定身符、回春符、环护、剑幕各一张，字与数取法术表。**上一手的战报压在说明区时，法术说明只剩一行、句子半截截断且没有「…」**（定身符看不到「不能逃的仗里无效」），交主会话（5.9） | `y_han_yunzhi_0_magic_dingshen`、`y_han_yunzhi_1_magic_huichun`、`y_nangong_wan_ch07_0_magic_huanhu`、`y_chen_shimei_0_magic_jianmu` |
| 择人菜单（路 U 记录 10.9，没有自动测试的那一样） | 回春符：标题「择人」，说明区写指着那位的气血，光标落在气血比例最低、还能选的那位（菡云芝 120/144）；环护：只列我方，挂着护持的灰着 | `y_han_yunzhi_1_target_huichun`、`y_nangong_wan_ch07_0_target_huanhu` |
| 环护 / 挡下 | 挂上：青光、头顶「护持」、面板小字；挡下：碎玉、飘「挡下」、标记散掉 | `y_nangong_wan_ch07_0_cast_huanhu`、`i32_battle_ward`、`i33_battle_wardblock` |
| 剑幕 | 全队身前风旋、面板「剑幕」、日志「本回合众人受伤减 30%」。**剑幕只管到本回合结束，她排在回合末出手时等于白放**，交主会话（5.2） | `y_chen_shimei_0_cast_jianmu`、`i34_battle_bulwark` |
| 定身符 / 动弹不得 | 金光、脚下「定身」、三颗金菱；轮到它时「动弹不得」、标记散掉 | `y_han_yunzhi_0_cast_dingshen`、`i35_battle_bind`、`i36_battle_boundskip` |
| 回春符 | 回 36、飘字 | `y_han_yunzhi_1_cast_huichun` |
| 合招 | 韩立菜单第六项「合招」，列表里剑刃交辉 / 环火合击 / 连珠火符 | `y_hanli_1_duo_list` |
| 被动 | 南宫婉在场开场每人 2 点劲（基础 1 + `opening_bp` 1） | `i30_battle_opening` |
| 切磋教学卡（路 U 记录 10.8，没有自动测试的那一样） | 头一回轮到下令弹出，三行字照 `ui_tongdao.json` | `w12_spar_tutorial_tongdao_spar` |
| 历练组队面板 | 三位都在（菡云芝第一次出现在这里） | `i23_lilian_party_three` |
| 打完 | 胜，战果卡照常 | `y99_battle_result` |

走查两条用例的断言（软件渲染、按键走真入口）：赠礼 +5、二档叙话给 3 张火球符、叙话 / 请教记账、在册加成解锁、切磋打完气血法力回到开打前、同修 +4 过一天、陈巧倩五档叙话记 5 且事件图撤干净——全过。

### 2.5 四张教学卡的字

读过一遍（`data/text/ui_tongdao.json` 的 `tutorial.tongdao*`），句子通、口径与规则一致（切磋「打完气血法力照开打前的数，用掉的东西照扣」与路 K 复审后的规则一致）。四张在真程序里都弹出来过：`w01_tutorial_tongdao`、`w02_tutorial_tongdao_xiangchu`、`w17_cushion_tutorial_tongdao_tongxiu`、`w12_spar_tutorial_tongdao_spar`。

### 2.6 对白（施工图 2.2）

14 段全文在真程序里演过一遍、`data/text/tongdao.json` 逐句读过：没有发现越线（身体接触、定情、对南宫婉 / 菡云芝的暧昧）或前后矛盾；没有错字，**一个字没改**。

## 3. 改过的旧测试

**没有。** 红线用例（`Ch07CompanionsTests` 的 `No5_…`、`Ch07AcceptanceTests`、`Ch08AcceptanceTests`）原样通过。

新加两个测试文件，都是**环境变量开着才跑**的辅助，没设时 `GTEST_SKIP`（全量 ctest 里记为 Skipped、不算失败）：

| 文件 | 用例 | 开关 | 做什么 |
| --- | --- | --- | --- |
| `tests/TongdaoPlaySaves.cpp` | `TongdaoPlaySaves.WriteTheTier3AndTopGateSaves` | `FANREN_TONGDAO_PLAYSAVES=<目录>`（另设 `FANREN_TONGDAO_SHOTSAVE=<文件>` 多写一份截图用的） | 写第 7 节入库的乙、丙两份；核甲与检查点逐字节相同 |
| `tests/TongdaoWalkShots.cpp` | `TongdaoWalk.TogetherInTheGardenAndTheSpar`、`TongdaoWalk.TheHongyanEventPictureAndAFightWithAllThree` | `FANREN_TONGDAO_WALK=<目录>` | 第 2 节的走查与拍图 |

## 4. 发现的问题与修法

两处都在 `src/game/TongdaoPage.cpp`（路 U 的页），都是只有真数据才露出来的版面问题，各是最小改动：

### 4.1 最高一档时「已有」截断，正好截掉五档加强的那一条被动

- 现象：陈巧倩五档「已有」要列七项（飞剑术、追击、双剑并出、被动：追击伤害 +20%、剑幕、合招、被动加强：+30%），两行放不下，末尾「…」截掉的恰是五档新得的那一条（修前的图已被修后重拍覆盖）。菡云芝同样。
- 修法：① 五档「被动加强」顶替三档那一条被动（「已有」写此刻的数，同一条被动不写两遍）；② 「已有」这一块至多三行，多用的一行把下面几块往下推——四块说明之下到动作列表之间本来空着百来像素，别的块仍是两行。
- 结果：陈巧倩五档「已有」两行放下，菡云芝三行放下，喜好、在册加成、动作列表都不重叠（`i05`、`i08`、`x07` 修后）。

### 4.2 赠礼列表多过一页时，「1/25」刻度压在最后一行的数量上

- 现象：检查点的储物袋 25 样，赠礼列表 12 行排满，列表底下的位置刻度与第 12 行的「×4」叠字（修前的图已被修后重拍覆盖）。
- 修法：照第一期集成在 `BoardScene` 里的同一个办法——条目多过一页时少排一行让给刻度（`TongdaoPage::rebuildGifts`）。
- 结果：11 行 + 刻度，不叠（`i11` 修后）。

## 5. 遗留与交主会话定的事

1. **「带上在册加成」那一行与说明打架。** 名额没满时（2a 只有三位，到不了炼气的 4 个名额）三条全都生效，说明写「生效中」，可动作行尾写「未带上」；勾上也看不出任何变化。规则层这样定是路 K 8.8 的「先取勾上的，名额没满按 id 补满」，界面照实写了旗。改法要主会话定：比如名额没满时行尾写「名额未满，自动生效」并把这一行置灰，或者只在超额时才出这一行。图 `i05`、`i08`、`i06`。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 1 条。**
2. **剑幕只管到本回合结束。** 陈巧倩在回合末出手时放剑幕，这一回合已经没人再挨打，等于白放（`i34`：她是本回合最后一个）。是数值 / 规则取舍（改成「到下一回合结束」或改成她出手时提前），交主会话。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 2 条**（管到她下一次出手前）。
3. **切磋的战果卡写「此战别无所得」**，紧接着回话写「首胜之礼：…」。编成奖励本来就是 0（奖品走伙伴表），字面没错，读起来前后两句像打架。要不要给切磋换一句（比如「点到为止」）由主会话定。图 `w15b_spar_result`、`w16_spar_after`。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 3 条。**
4. **赠礼列表按储物袋的次序排**，合意的东西不靠前（`i11`：丹砂在第 6 行，符纸、寒蟾酥在第 2 页）。没改，是否要「合意的排前面」是设计取舍。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 4 条。**
5. **乙、丙两份试玩档会先弹第一期的几张教学卡**（按键、伙伴、赠礼……）：检查点是无头通关测试写的，没有「看过」旗。与甲、与现有的 `saves/ch07-*.sav` 一样，不是这一期的问题；试玩说明里提了一句。
6. 路 N 记录第 11 节：门禁脚本（`tools/validate.py` 规则 40 一类）要不要认 `data/visual/cg.json` 并核 sha256——**门禁不跑，这一条没做**。sha256 现在由路 N 的 `TongdaoCg.TheTableMatchesThePicture` 在 ctest 里核。
7. 剪影那一屏用真数据拍不到（三位在任何第 7 章检查点上都已结识）；要看得新开局走到结识陈巧倩之前，或用截图口记号 `unmet.<id>`。
8. 路 N 9.1 的「菡云芝切磋太温和」在走查里再看了一遍：三档时只会普攻 5 回合赢。照主会话已裁定的现状。
9. **战斗选单的说明区：上一手的战报占着时，法术说明只剩一行。** 说明区先画上一手的战报（常常两行），法术说明挤到最后一行，放不下的半句直接截掉、没有「…」。这是战斗界面原有的排法（各章都一样，寒冰符的「每点劲多…」也被截），2a 的新说明更长（定身符「令一名敌人跳过下一次行动；首领与不能逃的仗里无效」）所以更显眼。要改得动共用的战斗界面与它的像素测试，没在集成里动，交主会话定（例如择法术时说明区不画战报，或给说明留两行）。图 `y_han_yunzhi_0_magic_dingshen`、`y_chen_shimei_0_magic_jianmu`；没有战报时（每仗头一个行动者）说明是整句，见 `i31_battle_magic`。**已在返修第一批修掉，见 `docs/ch07-tongdaolu-fix1.md` 第 5 条。**

## 6. 构建与测试

- 构建：这台电脑上用 `build-i/` 里的 `run.bat`（配置 + 构建）、`build-only.bat`、`test.bat`（全量 ctest，`-j 4`）；工作树里的 `vendor` 是指向主检出 `vendor` 的目录联接（`docs/ch07-foundation.md` 1.5）。换台电脑照那里的办法建自己的构建目录即可。
- 合并后、修之前：2540 条全过。
- 集成修正之后（提交前的最后一遍，`build-i`，`-j 4`）：**2543 条，全过**（`CTEST_EXIT=0`）——2540 条照旧全过，新加的 3 条辅助没设环境变量，记为 Skipped。
- 门禁没跑（用户定的规矩）。

## 7. 给用户试玩的东西

### 7.1 存档与启动（入库，换台电脑照样能玩）

乙、丙两份入库在 `fanren/saves/`（2026-11-21 用户定：要换电脑接着做，试玩档必须入库）；甲与现成检查点逐字节相同，不另存。
`saves/README.md` 的「检查点」表里各有一行（在哪、什么状态、进去先看什么），表后写了怎么重生成。
启动一律在 `fanren/` 下，`<槽>` 是你自己的构建目录：

| 试玩 | 存档 | 启动 | 是什么 |
| --- | --- | --- | --- |
| 从第 7 章开头 | `saves/ch07-start.sav`（现成） | `build-<槽>\fanren.exe --assets . --load saves\ch07-start.sav` | 第 6 章章末交过来的那一份 |
| 甲「刚出禁地」 | `tests/fixtures/companions/ch07-chujindi-first.sav`（现成检查点，不另存） | `build-<槽>\fanren.exe --assets . --load tests\fixtures\companions\ch07-chujindi-first.sav` | 真实进度。默契照主线的账：南宫婉 37，陈巧倩、菡云芝 0。站在百药园，进茅屋时先演谢师礼 |
| 乙「三位都在三档门口」 | `saves/ch07-tongdao-t3.sav` | `build-<槽>\fanren.exe --assets . --load saves\ch07-tongdao-t3.sav` | 调试档：演过谢师礼、站在茅屋蒲团前。三位默契各 42（合意赠礼 +5 或同修 +4 就到 45「三档」），二档叙话都还没演。背包加了三位喜欢的礼物各 3 份、养精丹 8、金疮药 6、回气丹 3 |
| 丙「最高档门口」 | `saves/ch07-tongdao-top.sav` | `build-<槽>\fanren.exe --assets . --load saves\ch07-tongdao-top.sav` | 调试档：同乙的站位。陈巧倩 93、菡云芝 93（差一次合意赠礼到五档），南宫婉 67（差一次到四档，她第 7 章封在四档）；各人前面各档的叙话、请教都记过账、首胜奖品记到此刻的档，菡云芝二档的 3 张火球符照账发了；三条在册加成生效。礼物、丹药同乙 |

两份都是存档版本 10。重生成：在 `fanren/` 下设 `FANREN_TONGDAO_PLAYSAVES=saves` 跑 `build-<槽>\fanren_tests.exe --gtest_filter=TongdaoPlaySaves.*`（`tests/TongdaoPlaySaves.cpp`；同一棵树上重跑逐字节相同，命令写在 `saves/README.md`）。
游戏里 F5 快速存档写 `saves/quick.sav`（不入库），不会改这几份。

本机（这台电脑）另有现成的双击入口，只是方便，不是必需：`build-i/` 里四个 `玩同道录2a-*.bat`（由 `build-i/make-bats.py` 写，GBK）读的就是上表这几份；截图用的那份（三位推到最高档、都在队里）在 `build-i/saves/tongdao-shot-party.sav`，由 `build-i/make-saves.bat` 加 `FANREN_TONGDAO_SHOTSAVE` 写，不入库。

### 7.2 试玩说明（十到十五分钟）

通用：方向键走、Z / 回车确认、Esc 取消或开主菜单。主菜单「同道录」里方向键挑人、确认看详情、详情里左右换人。乙、丙一进游戏可能先弹几张第一期的教学卡（按键、伙伴……），按确认收掉即可。

**乙「三位都在三档门口」（约 8 分钟）**
1. Esc →「同道录」：弹「同道录」教学卡；三张卡都是「相熟」，右上角「可叙话」。
2. 选菡云芝 → 详情（弹「相处」教学卡）：看「已有 / 下一档」、喜好、在册加成「请教之后生效」。
3. 动作「叙话」：二档那段（金竺笔），演完得 3 张火球符。
4. 回同道录选菡云芝 →「赠礼」→ 选行尾标「合意」的寒蟾酥（或丹砂、符纸）：默契 47，进「信任」，屏幕上沿「默契渐深」。
5. 「叙话」演三档、「请教」演一段：在册加成变「生效中」。
6. 「切磋」：弹「切磋」教学卡；她最温和，普攻也赢；打完气血法力回到开打前，报两档的首胜之礼。
7. 回世界按 Esc 收菜单，面朝蒲团按 Z：「打坐 / 同修」（弹「同修」教学卡）→ 同修 → 选陈巧倩：默契 +4、过一天、明天第一场多 1 点劲。
8. 有空再试南宫婉的切磋：她朱雀环一下四十多，找出「水」（冰冻术）当场破势才赢得了。

**丙「最高档门口」（约 7 分钟）**
1. 同道录 → 陈巧倩（知己 93）→「赠礼」火龙草（合意）→ 98「红颜」，上沿提示「默契渐深：陈巧倩 · 红颜」；详情「已有」写到「被动加强：追击伤害 +30%」。
2. 「叙话」五档：夜里药园，第 2 句后事件图淡入、她开口的两句不叠立绘，第 5 句后淡出。
3. 菡云芝 →「赠礼」寒蟾酥 → 98「莫逆」→「叙话」五档（竹哨）。
4. 南宫婉 →「赠礼」火灵石 → 72「知己」→「叙话」四档（托他养火性灵草）；五档那一格写「机缘未到」。
5. 走到右上的历练石碑按 Z，三位都带上进「禁地旧境」，撞一只明雷：南宫婉在场开场多 1 点劲（偷袭得手再多 1，是陈巧倩的在册加成）；轮到各人开「法术」看双剑并出 / 剑幕、流火 / 环护、回春符 / 定身符；回春符、环护是「择人」菜单；韩立菜单第六项「合招」。历练里出来过一天，里面不能存档。

**甲「刚出禁地」**：真实进度，同道录里只有南宫婉在二档（可叙话），陈巧倩、菡云芝一档——适合看「从头一点点处起来」的节奏：赠礼一天一次，同修一天一次（同修会过一天）。

## 8. 提交

| 提交 | 内容 |
| --- | --- |
| `8eaff0c` | merge: 同道录 2a 路 N 并入路 U |
| `65e10c5` 集成修正 | `src/game/TongdaoPage.cpp` 两处（第 4 节）；`tests/TongdaoPlaySaves.cpp`、`tests/TongdaoWalkShots.cpp`（第 3 节，环境变量开着才跑） |
| `7831fc9` 集成记录 | 本文初稿 |
| 试玩档入库 | `saves/ch07-tongdao-t3.sav`、`saves/ch07-tongdao-top.sav`、`saves/README.md` 三行与重生成一段；`tests/TongdaoPlaySaves.cpp` 改成写这两个文件名、核甲；本文第 7 节改成通用启动写法 |
