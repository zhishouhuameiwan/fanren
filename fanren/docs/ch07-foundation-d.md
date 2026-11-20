# 第 7 章可玩性改版·第一期地基包·路 D：对白减负（2026-11-20）

> 上位文档：`docs/gameplay-overhaul.md`（方向与拍板）、`docs/ch07-foundation.md`（第一期施工图，本路是第 4 节）。
> 同期另外两路并行：路 G（指引）、路 M（地图与说明）。本路只动 `talk` 与演出，章末状态不变，存档版本仍是 10。

---

## 1. 用户原话与本路目标

> 还是感觉可玩性不高，各种信息指引不够，物品说明也不够，对话又很多

拍板（`gameplay-overhaul.md` 第 246 行）：对白**砍到约一半**（9,000–10,000 字），**连续对白不超过 8 句**，另加**自动播放**和**整段跳过**；伙伴插话改成**头顶气泡**。

## 2. 量具与口径

- **`tools/dialogue_stats.py`**（只读，不是门禁）：逐脚本打表（talk 处数、字数、最长连续句数），末行给总数。
- **`tests/DialogueBudgetTests.cpp`**：同一套规则的 C++ 版，把两道上限钉进 ctest；用例先自证量具（分支取最坏、选择/战斗清零、return 结束、注释不算、气泡不算、循环展开两次）。
- **字数口径**：沿用 `ch07-design.md` 第 14 节——主线脚本里引用到的全部文案 key（含各分支与选项），去重后只数汉字（U+4E00–U+9FFF）。
  第 14 节当年是 46 个脚本（3.1 表 44 个主线挂点 ＋ Z2 两处）；伙伴改版（2026-11-18）把节点 31 拆出困地、火灵石、破顶、问名四段，
  这四段也各置一个目标链 `done_flag`，所以本路的正式口径是**目标链上的 50 个脚本**（量具与单测里的 `MAIN_SCRIPTS_CH07` / `mainScripts()`）。
  两种口径都报：

  | 口径 | 改前 | 改后 |
  | --- | --- | --- |
  | 50 个脚本（目标链，正式口径） | 17,827 字 | **9,473 字** |
  | 46 个脚本（第 14 节原口径） | 17,156 字 | 9,085 字 |
  | 全章 56 个脚本 | 18,171 字 | 9,640 字 |
  | talk 处数（56 个脚本） | 675 | 405 |
  | 两次可操作之间最长连续 talk | 32 句（`chukou`） | **8 句**（`liedui` 欠账那一路、`shulin` 选择后最坏那一路；其余脚本 ≤ 7） |

  施工图写的「现为 17,501 字」是伙伴改版之前的数；按今天的脚本重算，46 个脚本是 17,156（节点 31 有一部分文案挪进了那四段）。
- **连续句数口径**：静态走脚本的控制流。「可操作」= `choice`、`battle`、`shop`，以及脚本头尾（进脚本前、出脚本后都是走图）。if 的每条分支都走、取最坏的那条，
  循环按零次、一次、两次展开，`return` 结束这条路。`bubble` 不压场景、不收输入，不算一句，也不清零。

## 3. 改前改后逐脚本对照（`tools/dialogue_stats.py` 实测）

| 脚本 | 主线 | talk 处数 改前→改后 | 字数 改前→改后 | 最长连续 改前→改后 |
| --- | --- | --- | --- | --- |
| `banian` | ✓ | 9 → 6 | 276 → 141 | 7 → 4 |
| `baoming` | ✓ | 11 → 10 | 300 → 192 | 5 → 4 |
| `cangshi` | ✓ | 18 → 11 | 513 → 265 | 14 → 7 |
| `chahua_jun` |  | 3 → 0（气泡 1） | 60 → 23 | 3 → 0 |
| `chahua_kumu` |  | 3 → 0（气泡 1） | 67 → 16 | 3 → 0 |
| `chahua_shishi` |  | 3 → 0（气泡 1） | 56 → 15 | 3 → 0 |
| `chahua_yanbi` |  | 3 → 0（气泡 1） | 55 → 12 | 3 → 0 |
| `chaibao` | ✓ | 12 → 8 | 484 → 257 | 11 → 7 |
| `charen` | ✓ | 11 → 10 | 154 → 93 | 3 → 2 |
| `chouhan` | ✓ | 16 → 10 | 346 → 210 | 13 → 7 |
| `chuangong` | ✓ | 31 → 15 | 890 → 486 | 18 → 7 |
| `chukou` | ✓ | 41 → 15 | 947 → 378 | 32 → 7 |
| `chuling` | ✓ | 4 → 3 | 99 → 55 | 4 → 3 |
| `dihuo_wen` | ✓ | 8 → 6 | 215 → 134 | 7 → 5 |
| `dufang` | ✓ | 11 → 6 | 332 → 180 | 11 → 6 |
| `fangshi_chu` | ✓ | 3 → 2 | 82 → 48 | 3 → 2 |
| `fangshi_ru` | ✓ | 5 → 4 | 207 → 81 | 5 → 4 |
| `fanhui` | ✓ | 14 → 8 | 397 → 196 | 13 → 7 |
| `fengkou` | ✓ | 10 → 5 | 260 → 135 | 10 → 5 |
| `guanzhan` | ✓ | 26 → 17 | 634 → 388 | 13 → 7 |
| `huolingshi` | ✓ | 5 → 4 | 83 → 48 | 4 → 3 |
| `jiaoshi` | ✓ | 11 → 7 | 298 → 177 | 11 → 7 |
| `jihe` | ✓ | 11 → 7 | 363 → 183 | 11 → 7 |
| `kuitan` | ✓ | 7 → 5 | 243 → 148 | 7 → 5 |
| `liangshi` | ✓ | 6 → 4 | 187 → 99 | 6 → 4 |
| `liedui` | ✓ | 23 → 8 | 751 → 297 | 23 → 8 |
| `lilian_chen` |  | 4 → 4 | 68 → 63 | 4 → 4 |
| `lilian_nangong` |  | 2 → 2 | 38 → 38 | 2 → 2 |
| `ma_songyao` | ✓ | 7 → 6 | 144 → 82 | 7 → 6 |
| `murong` | ✓ | 16 → 7 | 497 → 258 | 16 → 7 |
| `poding` | ✓ | 7 → 5 | 209 → 129 | 4 → 2 |
| `pojin` | ✓ | 10 → 5 | 261 → 117 | 10 → 5 |
| `qingshidian` | ✓ | 10 → 5 | 259 → 138 | 10 → 5 |
| `sannian` | ✓ | 16 → 8 | 516 → 232 | 15 → 7 |
| `shijiu` | ✓ | 9 → 6 | 260 → 135 | 9 → 6 |
| `shimen` | ✓ | 7 → 5 | 180 → 105 | 7 → 5 |
| `shulin` | ✓ | 29 → 20 | 781 → 485 | 14 → 8 |
| `sichu` | ✓ | 4 → 2 | 122 → 54 | 4 → 2 |
| `tongmen` | ✓ | 20 → 13 | 612 → 286 | 11 → 6 |
| `wanbaolou` | ✓ | 27 → 19 | 693 → 410 | 14 → 7 |
| `wenming` | ✓ | 7 → 4 | 119 → 76 | 7 → 4 |
| `wulongtan` | ✓ | 9 → 5 | 292 → 127 | 9 → 5 |
| `xiang` | ✓ | 9 → 7 | 210 → 131 | 4 → 3 |
| `xiaoshidian` | ✓ | 31 → 20 | 726 → 447 | 12 → 7 |
| `xiashan` | ✓ | 5 → 3 | 99 → 54 | 5 → 3 |
| `xieshili` | ✓ | 11 → 6 | 241 → 120 | 11 → 6 |
| `yaolou` | ✓ | 35 → 23 | 806 → 434 | 15 → 7 |
| `yeyu` | ✓ | 26 → 20 | 767 → 481 | 10 → 7 |
| `yixiantian` | ✓ | 10 → 7 | 251 → 140 | 6 → 5 |
| `youdi` | ✓ | 10 → 6 | 247 → 131 | 9 → 5 |
| `yuanjiao` | ✓ | 7 → 4 | 231 → 126 | 6 → 3 |
| `yuelu_ru` | ✓ | 8 → 5 | 281 → 150 | 8 → 5 |
| `yueyang` | ✓ | 8 → 5 | 229 → 117 | 8 → 5 |
| `yujian_dongpo` | ✓ | 5 → 3 | 128 → 69 | 2 → 1 |
| `yujian_hantan` | ✓ | 3 → 1 | 83 → 24 | 3 → 1 |
| `zhuji` | ✓ | 18 → 8 | 522 → 224 | 17 → 7 |

## 4. 删减怎么做的

### 4.1 手法与守住的东西

- 四组并行删稿（按文案文件与场次切开，互不碰同一批脚本），手法照施工图 4.1 的顺序：删重复、寒暄、复述 → 设定挪成见闻 → 短句并句 → 旁白省掉。
  **没有用到「拆场」**：所有超 8 句的段都靠删与并压了下来，不新增选择、不改地图挂点。
- **只动 `talk`**：Lua 里只删整行 `talk(...)`，四处插话把 `talk` 换成 `bubble`；其余语句一个字符没动。
  自查脚本（`build-d/check_integrity.py`，不入库）把 56 个脚本去掉注释、空行、`talk`/`bubble` 行之后与 HEAD 逐行比对：**差异 0**。
  于是 `give`、`take`、`flag.set`、`battle`、`realm.advance`、`magic.*`、`party.*`、`bond.add`、`teleport`、`advance_days`、`choice` 的集合、参数、相对顺序，
  选项个数，目标链 `done_flag` 的置位点都与改前相同。
- **文案 key**：266 个不再被引用的 key 全部从 JSON 删掉（不留孤儿）；脚本引用的 key 全部在；没有超过 80 字的句子；全章无「封闭前最后一次」。
  测试按全名引用的 key 全部保留（文案可缩）。
- **首见词的节点不变**（`Ch07Acceptance` 的 12.2 先后表）：中阶灵石（1）、岳麓殿（3）、定颜丹（4b）、血色试炼（8）、师祖（18）、菡云芝（28）、南宫（31，`ch07.jiaoshi.name*` 起）、李化元（33）。
  `ch07.fudan.judge` 同时含「五年」「六十年」「最后」；`ch07.chukou.ship` 同时含「南宫婉」「没」「看」。
- 改写只用原文案里已有的词，不照抄原著、不添情节。

### 4.2 逐场戏眼（留下了什么）

**幕一·百药园两年**
- `chaibao`：叶师叔一包五样逐件交代（「中阶灵石」首见）；黄铜瓶试液一刻钟→第二夜「终究存不住」与瓶空的另一路；残片照旧盖瓶；园角另开两畦「真正的用处没提」。
- `yaolou`：头一年交三株黄精、马师伯验根「一年不少」、月例；第二年两株百年紫参、连过两关到十一层养精丹失效、问丹方→「巫钧山的岳麓殿」（首见）、担保玉符、Z1 血红芝。
- `yuelu_ru`：巫钧山平台按玉符、红衣筑基验符、「马师兄」点破马师伯筑基、三条通道。
- `cangshi`：许老勾罐（筑基）、价钱与「别叫师伯」、筑基丹要先天真火的死循环、定颜丹方（首见）、收了钱才说丹方失传。
- `dihuo_wen` / `shimen`：地火代真火、银丝鼎三十二块；五色石门、剔牙汉子背后讥讽，他脚下没停。
- `murong`：慕容兄弟雷法、陆师兄夸口硬接、少年往他身后躲他让开、火鸟收场、蓝衣女子传音、陆师兄回头一眼。
- `dufang`：辅药三十一味＝几滴绿液；三味主药没种子；定颜丹压最底下；马师伯「替他收尸」；想起吴风。
- `chuangong`：吴风开口两路；「血色试炼」首见；四个回一个；几夜不眠「不去是园边一座坟，去是四个埋三个」；挑敛气术。第二场：直接吞、闭关三月、告示五年后封六十年、判断（`fudan.judge`）。
- `yuanjiao` / `chuling`：四个多月白天练敛气术夜里浇园角；于执事「一年领一回」。

**幕二·坊市与夜遇**
- `fangshi_ru` / `wanbaolou` / `fangshi_chu`：五里禁飞落地换装；化名「厉飞雨」、只换不卖、丁老尝须根「千年往上，刚离土」、两株换四件、他定下这种买卖少做；出南口绕路进山洞。
- `yeyu`：洞外一走一拖→敛气→陆师兄的话（「这粒丹，我替你收着」）→选择→青索撞水罩→拼法力（生吞中阶灵石、撤水罩）→斩成两截→剑符化灰→她扑来、定住（符/术两支）。暗场口径未破。
- `fanhui`：她记不清他的脸；手停在储物袋口又收回；焚尸往西；闭关三天逼渣；她那一粒不还。
- `baoming` / `ma_songyao`：「新人也来送死」、伪灵根、龙鳞果二选一；「别死得太难看」。

**幕三·血色试炼**
- `jihe`：陈巧倩认不出他、十一层只三个、李师祖入殿（「师祖」首见）「老夫不回头找」。
- `xiang`：向之礼拉他抱团、「一块更大的肉」、两支回话都在。
- `liedui`：浮云子打赌、穹前辈押无形针、李师祖讲正与邪（草芥之喻、只论强弱）、他不起拜师心、掩月宗神舟、卖笔姑娘与络腮胡子、欠账两句。
- `pojin` / `wulongtan` / `liangshi` / `yixiantian`：风刃之墙、七件法宝砸口子、「身边没有同门」；寒冰蟾偷袭、敛气不动；两尸与丝线；一线天截杀、收袋烧尸。
- `shulin`：树上打坐、黄衫师姐求救、选择两支、封岳夺袋、师姐被杀、封岳「怎么个死法」与**他那句回话原样保留**、符宝揭晓、踏云靴、埋师姐。
- `tongmen` / `yueyang`：铜门三尸、飞蛇、钟吾认出靴子、选择、换资料；月阳宝珠化光雨驱雾、他最后一个上山。
- `kuitan` / `charen` / `youdi` / `sichu`：「别人抢熟的，他找只剩苗的」内心两句；八柄刃口朝上的节拍句；引出、剖开、挖苗；四处只剩苗、小石殿有天灵果。
- `xiaoshidian`：大汉逼退卖符少女、金刃救人「这一回，他没守自己的规矩」、武痴赌斗、掩月双娇、十丈之内丝线取首级、「那……我呢？」、木牌「菡云芝」（首见）、二选一、立誓「知道的人多一个，它就钝一分」、无忧针与忘尘丸。
- `qingshidian` / `guanzhan`：金刃弹回、钟吾为何不来、地道；掩月宗到、「师祖，就是这里」、蜕皮、道口合上、「小五行须弥禁法」、选择（数到第十一张符/趁她还有力气）、「为什么现在才出来」、「小家伙」、入队并肩。
- `jiaoshi` / `fengkou` / `huolingshi` / `poding` / `wenming`：剖蛟、粉红烟雾「两个人都失去了知觉」（暗场一行）、她长大了好几岁、dream 威胁；封死的口子、评银剑、要火灵石；递石「……有劳」；「我出火，你出砖」、金光砖成废纸、破顶；问名摘面纱「……南宫婉」。
- `xiashan` / `chukou`：第五日奔出口；各派人数、掩月宗最后一刻钟出来、通道碎灭、嗅灵兽查验与那包苗没闻出来、南宫婉登船「始终没往他这边看一眼」、菡云芝回头一笑、李师祖收记名弟子、迟疑那支「老夫收你还委屈你了？」、碧光刀、骑蟒回谷。

**幕四·地火屋**
- `xieshili`：改口叫师兄、「没筑基，师伯就还是师伯」、「李化元那个人护短」（首见）、谢师礼、「心疼了？」、他反倒放心。
- `sannian`：王师叔送赏、清点、三年催药四十瓶、陈巧倩筑基、叶师叔补还（冷暖两支）、李师祖送《青元剑诀》。
- `chouhan` / `shijiu` / `banian`：叫醒丑汉、亮剑诀落款横肉全松、定金三支；三十六门十九号、二十几炉全败、走到门口又转身开一炉；第一颗泛蓝光、二十五粒。
- `zhuji`：二选一、十二层、十三层、第八粒炸开昏死、醒来筑基、「……师、师叔！」、回园。

### 4.3 删掉的规则类台词由谁接住

路 G 的教学卡清单（施工图 2.3）与目标要点（2.1）是主要着落；其余挪成见闻（第 5 节）或仍留在缩短后的台词里。

| 删掉 / 缩掉的台词（key） | 讲的规则 | 由谁接住 |
| --- | --- | --- |
| `chaibao.plan` | 头一年交三株年份黄精、第二年两株百年紫参 | 目标 n02、n03 行动句与要点；教学卡「交药看年份」「打坐回气」 |
| `chaibao.test_gone` | 绿液存不住、凝一滴用一滴 | 意思并进 `chaibao.test`；教学卡「掌天瓶绿液」 |
| `yaolou.hj_less3`（缩，未删） | 苗下田满一年才浇得进 | 仍在台词；目标 n02 要点；教学卡「掌天瓶绿液」 |
| `yaolou.zs_less2`（缩，未删） | 紫参一株约五滴 | 仍在台词；目标 n03 要点 |
| `yaolou.y2_z1` | 支线 Z1 交药条件 | 并进 `y2_ma_price`；年份在 `z1_less` 与支线文案 `ch07.quest.baigong.s1` |
| `cangshi.copy` | 抄玉筒的价钱 | 并进 `dingyan`；`xu_price` 留「抄一份十块」；目标 n05；见闻「许老」 |
| `dihuo_wen.where` | 借地火的地点与计价 | 并进 `dihuo_wen.fire`；目标 n07；见闻「地火与玄阳火地」 |
| `shimen.board` | 地火按间按日、先付 | 见闻「地火与玄阳火地」；**「过了时辰另算」无着落**（见第 11 节） |
| `chuangong.only` / `seal` / `name_what` / `fingers` | 禁地准入、风禁五年一松、名称来历、早年死亡率 | 见闻「血色试炼」；台词留「如今四个里回来一个」 |
| `chuangong.lianqi` / `reason` | 敛气术的性质 | 并进 `choose`；见闻「敛气术」（敛气术只是旗标，不是可用法术） |
| `fudan.swallow` | 筑基丹服法 | 并进 `fudan.close` |
| `fudan.notice_why` | 封禁来由 | 见闻「禁地圈封」 |
| `dufang.ma_move` | 主药不能移栽 | 并进 `ma_seed`；见闻「筑基丹三主药」 |
| `yuanjiao.next` | 出谷先领令牌 | 并进 `who`；目标 n12 |
| `fangshi.street` | 坊市摆摊规矩 | 见闻「坊市的规矩」；台词留「北头摆摊，南头是谷里的铺面」 |
| `wanbaolou.fubao2` / `fubao3` | 符宝谁都能用、用一回耗一回 | `fubao1` 留「谁都用得了」「用一回耗一回」；见闻「符宝」；教学卡「道具施法」 |
| `wanbaolou.jue` | 拿到驱使法诀 | 并进 `deal`；教学卡「道具施法」 |
| `xieshili.ma_math` / `ma_rule` 半句 | 奖赏折算、谢师礼只一次 | 「本该两粒」留在 `ma_rule`；见闻「谢师礼」 |
| `chouhan.rate` | 灵石兑率 | 见闻「灵石的兑率」；`deposit_low` 仍有「一百块低阶」 |
| `chouhan.grumble` | 借火先交钱、墙上有价目 | 定金三支台词仍在；**「墙上有价目」无着落**（见第 11 节） |
| `shijiu.door` | 地火间外人进不来 | 见闻「地火间」；`banian.here` 仍有「外人进不来」 |
| `banian.bigu` / `banian.rate`（缩） | 辟谷丹、三炉一凝 | 并进 / 缩写后留着；教学卡「炼丹成功率」 |
| `zhuji.jianjue` / `rest` | 功法层次、筑基后收药力 | 见闻「青元剑诀」；`rest` 并进 `foundation` |
| `yeyu.drain` 半句（缩，未删） | 符放出去后一直抽法力 | 仍在台词；**教学卡清单里无对应**（见第 11 节） |
| `shulin.night`、`tree` 半句 | 打坐回法力（剧情动作） | `shulin.tree` 仍有；教学卡「打坐回气」 |
| `charen.back` | 洞道窄只能贴地走（插刃的道理） | 并进 `charen.how`；目标 n32 |
| `liedui.rules` | 赌约三样比法 | 并进 `fuyun_bet`；见闻「荒山上的赌约」 |
| `yixiantian.noflight` | 禁地御器即靶子 | 并进 `path`；见闻「禁地里的规矩」 |
| `tongmen.pearl` / `ring` / `tower` | 中心区三层、月阳宝珠 | 要点并进 `layers`；见闻「中心区三层」「月阳宝珠」 |
| `huolingshi.hint` | 主菜单队伍页可以送她东西 | 教学卡「赠礼」 |
| `kundi.stone` | 去哪找火灵石、往东调息 | 目标要点 `objective.ch07.n37c_huolingshi`；「要火灵石」仍在 `kundi.plan` |
| `chukou.beast` / `sniff` | 嗅灵兽闻百年以上的药、苗闻不出 | 教学卡「交药看年份」；见闻「出禁地的规矩」；`pile` / `pile_small` 仍有 |
| `chukou.close` | 出口按时合拢 | 见闻「出禁地的规矩」（设定，清单里无卡） |
| `chukou.bet` / `fuyun` / `li_iron` | 赌约结算 | 见闻「赌约的结局」 |

其余删掉的 key 都是叙述、寒暄、复述与过渡旁白，意思并进同场保留的句子或直接省掉。

## 5. 见闻（`data/lore/ch07.json`、`data/text/ch07_lore.json`）

23 条，格式照施工图 2.5（`id`、`category`、`title_key`、`body_key`、`unlock_flag`、`role_id` 空串）；**没有妖兽类**（归路 G）。
标题至多 7 个字、正文至多 103 个字。解锁旗标是讲到这件事的那一场的完成旗标，所以页面上读到它不早于那一场。

| id | 类 | 解锁 | 标题 |
| --- | --- | --- | --- |
| `ch07_yuelu_ru_menjin` | 所闻 | `ch07.yuelu` | 岳麓殿的门禁 |
| `ch07_cangshi_xulao` | 人物 | `ch07.cangshi` | 许老 |
| `ch07_dihuo_wen_dihuo` | 所闻 | `ch07.yinsi` | 地火与玄阳火地 |
| `ch07_dufang_zhuyao` | 灵草 | `ch07.dufang` | 筑基丹三主药 |
| `ch07_chuangong_jindi` | 所闻 | `ch07.lianqi` | 血色试炼 |
| `ch07_chuangong_lianqishu` | 所闻 | `ch07.lianqi` | 敛气术 |
| `ch07_fangshi_guiju` | 所闻 | `ch07.fangshi` | 坊市的规矩 |
| `ch07_wanbaolou_fubao` | 所闻 | `ch07.wanbaolou` | 符宝 |
| `ch07_chuangong_fengjin` | 所闻 | `ch07.fudan_fa` | 禁地圈封 |
| `ch07_liedui_qipai` | 所闻 | `ch07.qipai` | 越国七派 |
| `ch07_liedui_duyue` | 所闻 | `ch07.qipai` | 荒山上的赌约 |
| `ch07_yixiantian_guiju` | 所闻 | `ch07.yixiantian` | 禁地里的规矩 |
| `ch07_shulin_qianyin` | 所闻 | `ch07.fengyue` | 牵引之术 |
| `ch07_tongmen_sanceng` | 所闻 | `ch07.zhongwu` | 中心区三层 |
| `ch07_yueyang_baozhu` | 所闻 | `ch07.yueyang` | 月阳宝珠 |
| `ch07_xiaoshidian_wangchen` | 灵草 | `ch07.wuyou` | 无忧针与忘尘丸 |
| `ch07_fengkou_sunv` | 所闻 | `ch07.kundi` | 素女轮回功 |
| `ch07_chukou_guiju` | 所闻 | `ch07.chujindi` | 出禁地的规矩 |
| `ch07_chukou_duyue` | 所闻 | `ch07.chujindi` | 赌约的结局 |
| `ch07_xieshili_xieshili` | 所闻 | `ch07.xieshili` | 谢师礼 |
| `ch07_chouhan_lingshi` | 所闻 | `ch07.dihuo` | 灵石的兑率 |
| `ch07_shijiu_dihuojian` | 所闻 | `ch07.feidan` | 地火间 |
| `ch07_zhuji_qingyuan` | 所闻 | `ch07.done` | 青元剑诀 |

- 删稿时另起过一条「血色试炼」（荒山那一场，七派各出二十五人），与吴风那一条重题，合并时去掉了。
- 见闻文案也过 12.2 的首见词表：`Ch07Acceptance` 的步序表按解锁那一场给每条登记了步序（见第 8 节）。
- **集成要做的**：路 G 也建了同名的两个文件（3–5 条妖兽条目）。合并时两份 `entries` 拼起来、两份文案并起来；
  路 G 那几条文案键也要在 `Ch07AcceptanceTests.cpp` 的 `keyOrders()` 里登记步序，否则「不在节点表里」判红。
- 没有用「打探」（`data/pathactions/ch07.json`）：设定类的话全部挪进了见闻，打探条目牵涉旗标登记与在场链，本期不值当。

## 6. 自动播放（4.2）

- 设置键 `auto_advance`（`io::Settings::autoAdvance`，缺省关）；设置面板「游戏」组在「手柄震动」之后加一行「自动播放」，后两行各往后挪一行（面板高 622 → 652、仍上下居中）。
- 对话中按**路径行动键**（默认 E / Q，手柄 X）开关，当场写进设置文件。这个键在对话框里原本空闲，已在改键面板与手柄表里，不用另开新键位（路 M 还要加开图键，两路都往 `Engine::Key` 追加会撞）。
- 一句全显之后停 `autoAdvanceDelay(字数)` = min(4, 0.8 + 0.06 × 字数) 秒就翻；**有选项的框里没有任何计时**，选择永远等玩家自己按。
- 框上沿右侧多一块键位牌，只写这一框真能做的事（审核返修 2，`DialogueScene::keyHint2Key`）：脚本里不带选项的对白写「E 自动　Esc 跳过」
  （手柄版「X 自动　B 跳过」）；带选项的框、不经脚本的框（路径行动、提示）只写「E 自动」（`ui.dialogue.keys_auto`）。
  自动播放开着时再左边一块金底「自动」小标。
- 文案在 `data/text/ui_dialogue.json`。

## 7. 整段跳过（4.3）与插话气泡（4.4）

**整段跳过**
- 对话中按**取消键**（Esc / X，手柄 B）→ 确认框「跳过这一段对话？」跳过 / 接着看，默认停在「跳过」。
  **防误触**（审核返修 1）：键盘上 X 紧挨 Z，确认框弹出后 0.35 秒内不收确认键（上下移、取消照收），X→Z 连按两下不会把一整场戏跳掉；
  无头模式缺省不设这个延时（测试不空等），单测用 `setSkipAskGuard` 显式打开它来钉。取消键在不带选项的对话框里原本空闲；带选项的框里它仍是选项自己的取消（返回 nil），不弹跳过。
  只在脚本的对白里能跳（`Application::canSkipDialogue`：有脚本在跑、正等这一句）；路径行动、提示那种不经脚本的框没有「一段」可跳。
- 确认后，后面的 `talk` 不压对话框，直接记进回看与 `spokenKeys`，就地完成；`give`、`flag.set`、`teleport` 等照常（得物卡片照弹），`wait` 与淡入淡出压成当场完成。
- 跳到下一条要玩家动手的命令（`choice`、`battle`、`shop`、结局卡、`game_over`）或脚本结束就收住。**选择永远不自动选**：跳到选择就停，选择框照常弹出。
- 收住时屏幕上半截正中弹一条「已跳过 N 句 · 按 Tab 回看」（键名跟着键位与设备走），停 4 秒、淡出半秒，不挡路、不用按键。
  跳到脚本结束时人已回到地图上：提示停着的那 4 秒里，世界层上按菜单键翻开的是回看面板（`DialogueLogScene`，与对话框里那一份同一块面板、同一份记录），不是主菜单。
- 回看记录上限 `kDialogueLogCap` 60 → 120。改后全章最长的一段是 23 处 talk（`yaolou`），跳过的这一段连同之前的话都翻得全。
- 现有的按住 Ctrl 逐框快进没动。

**插话气泡**
- 新脚本命令 `bubble(role_id, text_key, seconds)`（`CommandKind::Bubble` 追加在末尾，`api.lua` 追加）：在那个角色头顶浮一条气泡，不压场景、不收输入，人照走；同时记进回看与 `spokenKeys`。
- 角色找法：跟在身后的伙伴按画面上的滑步位置每帧重找（人走气泡跟着走）；图上在场的 NPC 按所在格。判断「在不在画面上」用逻辑镜头（与 `heroScreenRect` 同一个算法），无头下也成立。
- **不在画面上时退回成普通对白**（照 `talk` 压对话框、等确认），话不丢。整段跳过中与 `talk` 一样只记一笔。
- 四处 `chahua_*.lua` 改用它：原来「旁白—她一句—旁白」三框改成她头顶一只气泡（停 4 秒），前后两句旁白删去，景物交给地图本身，她那句改写成把看见的东西带出来；`bond.add`、`flag.set` 照旧。

## 8. 测试

**新增**
- `tests/DialogueBudgetTests.cpp`（3 条）：量具自证；第 7 章 56 个脚本每个最长连续 ≤ 8；主线对白 9,000–10,000 字。
- `tests/DialogueFlowTests.cpp`（15 条；审核返修加了 3 条：弹出后立刻按确认不跳、键位牌只写能做的事、默认键位下两块牌的字）：
  - 自动播放的停顿公式；设置文件往返、缺省关；开着自动播放时两句自己翻、**选择停 42 秒也不被自动选**、玩家按了才走；关着时十秒不动；对话里按 E 开关。
  - 整段跳过：确认框「接着看」不跳；「跳过」一路跳到选择、中间的 `give` 与 `flag.set` 照常、跳过的句子进了回看与 `spokenKeys`、**选择不被自动选**、提示写「2 句」「Tab」、选完之后照常逐框；跳到脚本结束、按 Tab 翻开回看、提示过后 Tab 照旧开主菜单；有选项时 Esc 是选项的取消；脚本外的提示框不给跳。
  - 气泡：说话人在画面上时脚本不等人、不压对话框、记进回看、到点自己消失；不在画面上时退回普通对白、等确认。气泡框贴边不出屏、尖角指着人。

**改了的旧测试（都因为本路的变化，逐条）**
| 测试 | 改了什么 | 为什么 |
| --- | --- | --- |
| `SettingsWiringTests.cpp` 行号的 `static_assert` 与「按键设置」那一条 | `kAutoAdvance == 9`、`kKeys == 10`、`kRestore == 11`、`kRowCount == 12`；从「手柄震动」下去多走一格 | 设置面板插了「自动播放」一行（新真值） |
| `UiStyleTests.cpp` 设置面板行名表 | 加「自动播放」 | 同上 |
| `KeyBindingTests.cpp` `KeyPrompts` 两条白名单 | 加 `ui.dialogue.keys2`、`ui.dialogue.keys_auto`、`ui.dialogue.skipped` 与三条 `.pad`；`{pad.` 处数 21 → 25 | 对白框第二块键位牌与跳过提示要跟着键位、设备走 |
| `Ch07AcceptanceTests.cpp` `keyOrders()` | 23 条 `lore.ch07.*` 按解锁那一场登记步序 | `ch07_lore.json` 以 ch07 开头，被章内先后表扫到；表里查不到的 key 判红 |
| `Ch07AcceptanceTests.cpp` `No6_TheNamePlatesFollowTheSameOrder` | 先验「核到 > 100 句带名牌的」→ > 60 | 带名牌的句子从一百多句降到 90 句（新真值） |
| `Ch07AcceptanceTests.cpp` `No22_…` 主线字数 | 「≥ 17300 的八成」→「≥ 9000 的八成、≤ 10000」 | 主线对白砍到 9,000–10,000（新真值） |

第 1–6、8–13 章的通关验收没有因本路改断言。

**结果**：全量 ctest（见交付汇报与第 10 节）。

## 9. 截图（`build-d/shots/`，截图口 `--scene dlg:*`，见 `game/ShotScenes.*`）

| 图 | 截图口 | 看什么 |
| --- | --- | --- |
| `d_auto_badge.png` | `--map ch06_baiyaoyuan --scene dlg:auto:ch07.chaibao.stones` | 框上沿：金底「自动」小标、「E 自动」、「Tab 回看　长按 Ctrl 快进」。截图口压的框不经脚本，所以第二块牌只写「E 自动」；脚本对白里写「E 自动　Esc 跳过」由单测钉 |
| `d_skip_ask.png` | `--map ch07_yuelu_dian --scene dlg:skipask:ch07.cangshi.dingyan:xu_lao` | 跳过确认框（框被抬高让开主角时，确认框挪到框下方） |
| `d_skipped_notice.png` | `--map ch07_jindi_wai --scene dlg:skipped:12` | 「已跳过 12 句 · 按 Tab 回看」 |
| `d_bubble.png` | `--map ch07_dixia_shiku --scene dlg:bubble:baiyi_shaonv:ch07.chahua_jun.say` | 跟在身后的白衣少女头顶的气泡 |

## 10. 验收判据（施工图 4.6）逐条

| # | 判据 | 结果 |
| --- | --- | --- |
| 1 | 主线 9,000–10,000 字；最长连续 talk ≤ 8；统计脚本或单测能复现 | ✓ 9,473 字（46 脚本原口径 9,085）；最长 8 句。`tools/dialogue_stats.py` 与 `DialogueBudget.*` 都复现 |
| 2 | 章末状态逐字段一致；现有通关验收与对账测试通过 | ✓ 非 talk 语句与 HEAD 逐行一致（差异 0）；`Ch07Acceptance`（两侧通关、终点逐项）、`Ch07Ledger`、`Ch07Slice`、`Ch07Companions` 等全过 |
| 3 | 每场戏眼都在；有「删掉的规则类台词由谁接住」的表 | ✓ 第 4.2、4.3 节；三条无着落列在第 11 节 |
| 4 | 自动播放、整段跳过可用；有单测证明选择不会被自动选 | ✓ `DialogueFlow.WithAutoAdvanceOn…NeverPickedForThePlayer`、`SkippingStopsAtTheChoice…` |
| 5 | 4 处插话改成气泡；角色不在场时退回普通对白 | ✓ 四个 `chahua_*.lua`；`DialogueFlow.ABubbleForSomeoneNotOnScreenFallsBackToAnOrdinaryTalk` |
| 6 | 见闻 20–30 条，格式合乎 2.5 | ✓ 23 条（见闻的读取与页面在路 G，本路只保证 JSON 合格式） |
| 7 | 全量 ctest 通过 | 见下 |

**全量 ctest**（`build-d`，`-j 4`）：
- 第一次全量：**2318 / 2324 通过**，6 条失败——
  - `Ch07Acceptance.No6_TheNamePlatesFollowTheSameOrder`、`Ch07Acceptance.No22_…`：本路带来的新真值（第 8 节表），已改断言；
  - `PadWiring.Holding{Tab,Esc,PadY}ForASecond…`、`SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond`：墙上时钟的用例
    （「一秒里至少走 30 帧」「十帧不超过 120 毫秒」）。跑的时候另外两路的测试也在这台机器上并行，单条小用例要 6 秒。
- 改完后把这 6 条连同全部对白用例、`KeyPrompts` 单独复跑（35 条）：34 条通过，VSync 那条在并行下仍超时；**单独跑 3 次 3 次通过**。
  这 4 条与本路改动无关（不碰引擎帧率、不碰世界层按住菜单键的路径）。**对照**：同一时刻用主检出现成的基线构建
  （`E:\fanren\fanren\build-s7\fanren_tests.exe`，不含本路改动，只执行、不改文件）跑 `PadWiring.Holding*`，同样挂
  （一秒只走到 10–18 帧，要求 30）。
- 第二次全量（改完之后，955 秒）：**2322 / 2324 通过**。只剩 `PadWiring.HoldingTab…` 与 `PadWiring.HoldingEsc…` 两条墙钟用例，
  即上面那条对照里基线构建同样会挂的那两条。本路新加的 15 条、Ch07 全部验收与对账、第 1–6、8–13 章通关都过了。

### 审核返修（2026-11-20，主会话）

1. 跳过确认框防误触：弹出后 0.35 秒内不收确认键，默认项仍是「跳过」；单测 `DialogueFlow.ConfirmRightAfterTheSkipAskOpensDoesNotSkip`。
2. 键位牌与能做的事对齐：只有 `!hasChoices_ && app.canSkipDialogue()` 时写「跳过」那一半；新文案 `ui.dialogue.keys_auto`(.pad) 进了 `KeyPrompts` 两张白名单，`{pad.` 处数 24 → 25。
3. 墙钟三条各单独跑三遍（机器仍被其他会话占着，单条用例 3–30 秒）：

   | 用例 | 三遍结果 |
   | --- | --- |
   | `PadWiring.HoldingTabForASecondOpensTheMainMenuOnceAndItStaysOpen` | 过、过、过 |
   | `PadWiring.HoldingEscForASecondOpensTheMainMenuOnceAndItStaysOpen` | 挂（一秒 24 帧，要 30）、过、过 |
   | `SoftwareEngine.WithVSyncOffAFrameStillTakesAtLeastA240thOfASecond` | 过、挂（十帧 172 毫秒，上界 120）、过 |

   挂的两遍都是墙钟超限，不是逻辑断言。干净机器上的全量复跑留给集成会话。

**返修后全量 ctest**：见交付汇报（下一次提交补进这里）。

## 11. 没做成的、拿不准的、交回主会话的

1. **规则清单里没有着落的三条**（路 G 的教学卡清单与目标要点都没有；主会话审核定为只是说法、不对应玩法，不补）：
   - `shimen.board` 的「过了时辰另算」（借地火超时另收钱）——引擎里借地火是脚本收定金，没有按时辰计费的机制，删掉不丢玩法；
   - `chouhan.grumble` 的「墙上有价目」——同上，只是一句说法；
   - `yeyu.drain` 的「符放出去后一直抽法力」——台词仍在（缩写），但如果引擎里水罩符真有持续耗法力，路 G 可能要补一张卡或一行要点。
2. **最长连续 8 句的两处**正好卡在上限：`liedui` 欠账那一路（不欠账 6 句）、`shulin` 选择后最坏那一路（银钩、青索两句挂在 `take` 的返回值上，缺一件就是 7 句）。
   再压就得让浮云子或穹前辈不开口（两人全章只在这里说话），没动。
3. **几个角色不再开口**：围观弟子（`murong.whisper`）、白衣女子「又来一个送死的」、`liedui` 里络腮胡子的呵斥（胡子在 `yixiantian` 照样开口）、霓裳仙子（`chukou`）。
   NPC 的显隐没动。`data/visual/looks.json` 有几处 note 还写着被删 key 作出处（`murong.whisper`、`chukou.sit`、`chukou.han`、`guanzhan.come`），只是注释，没改。
4. **几处信息挪了位置**（意思都在）：`liedui.fly`（飞到黄土坡）挪到 `pojin.test` 开头；`wulongtan.man` 不再明说那人死了，靠下一句「烧了尸首」交代；
   `zhuji` 的十二层、十三层、筑基三句吸收了前后叙述，都在 `if ok then` 里（正常流程恒为真）；丑汉摔茶碗并进了 teleport 之后的 `garden`；
   `sannian.book` 并进了「去了巫钧山」，所以剑诀的得物卡片出在这句之后。
5. 见闻「血色试炼」「赌约的结局」里有几处是从别场台词搬过来的说法（风禁五年一松、赌约兑现），不是新设定；「赌约的结局」只写浮云子「交出」内丹，没写交给谁（原文没明说）。
6. **门禁规则将来要补的**（本路按规矩没跑门禁）：`data/lore/*.json` 的结构（id 唯一、category 四选一、key 都在、解锁旗标在 `flags.json` 里登记过）；
   `bubble()` 的文案 key 与说话人角色要像 `talk()` 一样被 `validate.py` 认得；`ui_dialogue.json` 的 `{key.*}` / `{pad.*}` 占位白名单。
7. **集成时会撞的共享文件**：`SettingsScene` 行号（三路各插一行）、`SettingsFile` 字段、`Application.h/.cpp`（本路是成块追加：对白派发、跳过、气泡、`tick` 里提示停着时的菜单键、`drawScenes` 的两处叠画）、`api.lua` 末尾、`Command.h` 末尾、`ShotScenes.cpp` 末尾、`ch07_lore.json` 与 `data/lore/ch07.json`（两路同名文件）。
