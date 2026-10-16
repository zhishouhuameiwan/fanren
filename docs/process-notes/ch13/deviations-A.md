# 第 13 章内容路 A1（地图）施工偏差与交接（2026-10-13）

交付：`tools/mapgen/genmaps_ch13.py`（新，四图各一函数，确定性、不用随机数）、`tools/mapgen/genmaps.py`（check() 里第 12 章之后登记
genmaps_ch13，只加两行）、`maps/ch13_{nanming,tianxing,waihai,wuzhong}.tmj`、`maps/tilesets/terrain_ch13_*.tsj` ×4、
`data/text/ch13_maps.json`（4 个地名 key；本章 0 个 portal，所以 0 条 deny 文案）。美术（data/visual、sprites、assets/art）归 A2，没碰。

## 别路必须照抄的数字

- **带坐标的两处落点**（生成器顶部常量 `MISHI_SHORE`、`QIYUAN_REEF`；格上不放任何对象、可走、可达）：
  - #5 `lidao.lua` → `teleport("ch13_waihai", 14, 46)`（秘市岸，照施工图建议值；存档点 `facility_cunji_mishi` 在 (13, 46)）。
  - #6 `qiyuan_hang.lua` → `teleport("ch13_waihai", 7, 12)`（黑石城西门外礁）。`trigger_hui_wuzhong` 在礁头尖 (3, 12)，
    西门带 `trigger_chucheng` 是 (10, 11) 起的 1×3 竖带（x=10，y 11–13）；两者之间隔 6 格，落点左隔 3 格、右隔 2 格，不压任何一条。
    **C 写 `qiyuan_hang.lua`、测试路写 `kScriptTransfers` 注释与切片断言都用 (7, 12)。**
- **default spawn**：nanming `haishang` (6, 29) 朝上；tianxing `chengwai` (27, 36) 朝上；waihai `shiwu` (34, 7) 朝下（门在南）；
  wuzhong `andeng` (59, 37) 朝上。
- **BGM**（A2 写 `tools/audiogen/catalog.py` 注释表与 `docs/audio.md`）：nanming→bgm_river、tianxing→bgm_town、waihai→bgm_river、
  wuzhong→bgm_valley（施工图 4.2 建议值，全是已有曲子）。
- **挂点 → 脚本名**（施工图「脚本名＝节点 id 去 nNN_」）：nanming `trigger_wenxun`→wenxun、`trigger_qiuling`→gongdao、
  `trigger_chugang`→zhuibing；tianxing `trigger_rucheng`→rucheng、`trigger_dongfu39`→qifu、`trigger_xingkong`→tanzhen、
  `trigger_zhaiyuan`→qianru、`trigger_lanlu`→yirong、`trigger_fangshi_bimen`→fangshi_bimen；waihai `trigger_shiwu`→daoqiyuan、
  `trigger_chucheng`→chucheng、`trigger_hui_wuzhong`→hui_wuzhong、`trigger_xuanshang`→xuanshang、`trigger_feixu`→feixu、
  `trigger_mishi`→mishi、`trigger_jiaoshi`→jianshi、`trigger_dongkou`→banyao；wuzhong `trigger_lifu`→lifu、`trigger_chongshi`→chongshi、
  `trigger_shichuang`→xuanyin、`trigger_lianjianshi`→chonglian、`trigger_dizhen`→dujie、`trigger_biguanshi`→wusezhu、
  `trigger_buzhen`→buzhen、`trigger_youyao`→youyao、`trigger_shoushou`→chongmo、`trigger_danfang`→liandan、`trigger_jingshimen`→biguan、
  `trigger_chuguan`→chuguan、`trigger_lidao`→lidao、`trigger_qiyuan_hang`→qiyuan_hang、`trigger_yanghun`→yanghun。
  NPC：`npc_yuanliao_zhanggui`→caigou、`npc_cang_yunlong`→panzha、`npc_ling_yuling`→yinjian、`npc_gu_laozhe`→chuansong、
  `npc_ming_furen`→mingzhu、`npc_li_xun`→cunjin、`npc_gongsun_xing`→gongsun、`npc_wen_siyue`→wenshi；其余 NPC 无脚本。
  C 已落地的 20 份脚本的 `@hook` 与地图逐条对上（validate 规则「@hook 声明」0 报）。

## 偏差（施工图没写死、由本路定的）

1. **`region` = `luanxinghai_nixing`**：施工图 4.2「由协调者定一个值」，协调者备注没给；沿 `luanxinghai_*` 前缀自定，四张同值。
   引擎与 validate 只要非空字符串；要改只动生成器 `REGION` 一处、重跑。
2. **`trigger_fangshi_bimen` 的脚本名 `fangshi_bimen.lua`**：施工图只给对象名，没给脚本名；按「对象名去 trigger_」定。C 已同名落地。
3. **横贯带的形状**：nanming `trigger_wenxun` (5,28) 3×1、tianxing `trigger_rucheng` (26,35) 3×1、wuzhong `trigger_lifu` (58,36) 3×1
   都紧贴 spawn 北面一格（迈一步即响）；waihai `shiwu` 在屋内，门 (34,8) 一格，`trigger_shiwu` (33,9) 3×1 是门外台阶第一行（两步）。
   东西走向的通道用**竖带**：nanming `trigger_chugang` (42,13) 1×3、waihai `trigger_chucheng` (10,11) 1×3、
   wuzhong `trigger_lidao` (55,15) 1×3、`trigger_buzhen` (12,30) 1×3。其余带：`trigger_qiuling` (25,7) 3×1（坡坎唯一坡口）、
   `trigger_lanlu` (24,9) 3×1、`trigger_xingkong` (24,5) 3×1、`trigger_feixu` (29,22) 3×1（南门外礁路第一行）、
   `trigger_jiaoshi` (54,36) 3×1、`trigger_dizhen` (30,15) 3×1、`trigger_chuguan` (30,19) 3×1。
4. **interact 挂点画成实心格（building gid 8：门 / 院墙 / 告示墙 / 石台），站旁边按**（第 11、12 章生成器同写法）。由此：
   - 天星城三十九层：洞府门 `trigger_dongfu39` (10,12) 是实心门，**旧灵田与静室在洞府院内的台地上**（门外同一层），
     `facility_yaoyuan_39` (16,11) 在翻土田里、`facility_jingshi_39` (21,12) 在静室石屋门前；两者只靠 `require_flag ch13.rucheng` 拦，
     n08 前后都进得去（施工图只要求与门不同格）。
   - 雾中岛六扇门都在凹谷北壁 (22/26/30/34/38, 3) 与东壁 (44, 8)，门后石室不画可走地面；静室 `facility_jingshi_wz` (42,10)、
     丹炉 `facility_danlu_wz` (40,5)、药园 `facility_yaoyuan_wz` (23,9)、`trigger_yanghun` (27,10) 都摆在凹谷院里，全在内侧带之内。
5. **雾中岛「山脉凹谷洞府」的画法**：山体中间凿一块凹谷院（x 20–43，y 4–12），经一条三格宽南北甬道（x 30–32，y 13–21）出到谷外；
   甬道内侧 `trigger_dizhen` y=15、外侧 `trigger_chuguan` y=19，两行之间隔两行。林边光幕前的公孙杏 (35,24)、孙师叔 (35,26) 在甬道口东侧林子边。
6. **外星海三区的连法**：黑石城（城墙一圈，西门、南门）→ 南门外礁路主干 → 往西南、东南各一支。`trigger_feixu` 是北区与礁路之间唯一的
   一行（核过：把它当墙，北区与两岛互不相通）；西门外礁头是死路，只通 `trigger_hui_wuzhong`。东南小岛只有 `trigger_jiaoshi` 一个口；
   洞口 `trigger_dongkou` (61,44) 在半山平台东壁、巨石 (57–58, 44–45) 背后，章末玩家站 (60,44)。
7. 南明岛港后丘陵是死路：坡坎一行实心，只留 `trigger_qiuling` 那三格坡口。打坐 `facility_dazuo_nanming` (40,12) 在客栈南墙根，出海口带以西。
8. 天星城第四层宅院 `trigger_zhaiyuan` (36,18) 是院墙西面一格（站 (35,18) 按）；宅院画成实心院墙，不画院内。

## 本路自核（草稿脚本，不进仓库）

- 四图同格 0 处两个对象；任何对象都不压 spawn 格；两处带坐标落点格无对象、可走、可达。
- 把 `trigger_wenxun / rucheng / shiwu / lifu` 当墙，从各自 default spawn 出发只够得着存档点（施工图验收 3 的「横贯带」判据）；存档点与落点四邻相接。
- tianxing：大厅四人要跨 `trigger_lanlu` 与 `trigger_xingkong` 两条带，`lanlu` 在 `xingkong` 之下；三十九层、第四层、坊市门都在 `lanlu` 之下。
- wuzhong：把 `trigger_dizhen` 当墙，从凹谷院内够得着六扇门、三设施与 `trigger_yanghun`，从 `andeng` 一样都够不着；`dizhen` 在 `chuguan` 之后；
  `trigger_qiyuan_hang` 与 `trigger_lidao` 隔 5 格；阵心与收阵石曼哈顿距 12；珊瑚礁要跨 `trigger_buzhen`。
- 把所有 NPC 都当墙（最坏情形：都在场）四图仍全部可达——没有 NPC 站在唯一通路上；设施一律占格，也都不压走廊。
- MapArt 口径：ground 只用 1/2/3、overlay 6/7、building 5/8、front 4；挡路的 gid 3（海、水潭）格上 0 处 building；有 building 处 collision 全非 0。

## 门禁证据

- `python tools/mapgen/genmaps.py --check`：`MAPGEN_IN_SYNC：79 张图与生成器一字不差`；`genmaps_ch13.py --check`：4 张一字不差。
  （没跑不带 `--check` 的 `genmaps.py`：它只重写第 1 章六张，与本章无关；第 13 章四张由 `genmaps_ch13.py` 生成。）
- `python tools/validate.py`（真树，2026-10-13 晚）：错 108，其中提到 ch13 地图的 27 条全是别路未落地——20 条「脚本不存在 scripts/ch13/*.lua」
  （mishi、jianshi、banyao、wenshi、lifu、chongshi、xuanyin、chonglian、wusezhu、liandan、biguan、yanghun、dujie、chuguan、buzhen、youyao、
  chongmo、lidao、qiyuan_hang、gongsun）与 7 条「旗标从开局永远置不上」（置它们的正是这些缺的脚本）。本路文件引起的错 0。
- 草稿副本（scratchpad/sbx）里给缺的脚本补空壳（只 `flag.set` / `teleport`，`@hook` 照地图，#5、#6 带上述坐标）后跑 validate：29 错，
  0 条与地图有关（26 条 `ch13.path.*` 旗标未登记 / 文案缺——协调者与 C；3 条战斗文案 key 缺——C）。规则 13、16、17、18、19、29、30
  与路径行动的 NPC 对象引用全过（B 已落地的 role 名牌在同图里两两不同）。

## 白名单外、交别路

- **A2**：`python tools/artgen/artgen.py --check` 现报 `KeyError: data/visual/maps.json 缺这些图的预设：ch13_nanming, ch13_tianxing,
  ch13_waihai, ch13_wuzhong`——预设照施工图第 4 节表（`dock/day/mist/dock_river`、`town/dusk/dust/dock_river`、`dock/day/mist/dock_river`、
  `valley/day/mist/herb_valley`），补完重烘 `assets/art/**`。BGM 注释表四行见上。
- **C**：上列 20 份脚本；`qiyuan_hang.lua` 的坐标 (7, 12)、`lidao.lua` 的 (14, 46)。
- **协调者**：`ch13.path.*` 13 面旗标（`data/pathactions/ch13.json` 已落地）待登记；`region` 若要统一另定，改 `REGION` 一处。
- **测试路**：`kScriptTransfers` #5 / #6 的坐标照本文；验收 3 的各项几何判据本路已按上节自核。

---

# 第 13 章内容路 A2（美术与音频登记）施工偏差与交接（2026-10-14）

交付：`data/visual/maps.json`（4 张图预设）、`data/visual/battles.json`（b13 两场）、`data/visual/looks.json`（29 条外观 ＋ 29 条 roles）、
`tools/artgen/sprites_enemies.py`（`CHAPTER13_BEASTS`：`wuji_haishou`、`qingse_juxia`）、`assets/art/**`（全量重烘）、
`tools/audiogen/catalog.py`（MAP_BGM 上方注释表 4 行，元组不动）、`docs/audio.md`（同一张表 4 行）。`sprites.py` 没改（兽图只要进 BEASTS 就自动登进 index.json 的 enemies）。

## 预设（照施工图第 4 节表）

- ch13_nanming `dock/day/mist/dock_river`；ch13_tianxing `town/dusk/dust/dock_river`；ch13_waihai `dock/day/mist/dock_river`；ch13_wuzhong `valley/day/mist/herb_valley`。
- battles.json：`b13_nanming_zhuibing` → `dock_river`（map ch13_nanming）；`b13_waixinghai_lieyao` → `cliff_top`（map ch13_wuzhong，trigger_youyao 在雾中岛西海珊瑚礁）。
- BGM 照 A1 与施工图 4.2：nanming/waihai `bgm_river`、tianxing `bgm_town`、wuzhong `bgm_valley`（与 tmj 的 bgm 属性逐张核过一致）。
- 没加 ch12 那两个单图开关（`liquid`、`grass_tint`）。

## 偏差 / 本路定的

1. **人工指认（structures / zones）**：A1 的实心块在 natural 主题里会被 mapinfer 认成「屋顶」（不贴图边、不中空 → roofed），
   所以每张图都写了 `structures` 指认岩体：南明岛坡坎、天星城圣山与城墙（坊市门、第四层宅院单列 built）、外星海祭坛高台 / 黑石城城墙
   （fortress）/ 乱石堆 / 绿山 / 东南半山、雾中岛山脉与两片林子；`zones` 把礁路（gid 2 挨海会画成栈桥）改成 gravel、石阶改 steps、
   祭坛石屋地面 stone_floor、雾中岛洞府院子 cave_floor ＋ 甬道 cave_path（施工图「第 10 章小寰山的画法」）。每条都写了 `why`。
2. **A1 生成器的「树」认不成树**：南明岛坡上 4 株（单格实心＋北面一格 front）、雾中岛谷外 6 处「散树」（2×2 实心＋北面一行 front）——
   mapinfer.find_trees 要求树干格本身带 front、且 3×3 里 front 占满大半（`genmaps_ch09.trees()` 的画法：front 盖 x-1..x+1、y-2..y）。
   本路不改地图，在 maps.json 里把它们逐个指认成 `rockery`（一堆山石），否则会画成一格的小茅屋。要画成真树需 A1 改生成器，交协调者定。
3. **雾中岛海面满是荷叶（未解决，交协调者拍板）**：`mapwater.paint_water` 对 valley / manor 主题的**所有**水面撒荷叶（第 93–94 行），
   雾中岛四面是海，烘出来整片海都是荷叶带粉花（施工图定的 valley 主题本身没错，是 artgen 没想到 valley 图会临海）。
   本路试过把主题换成 `wild/day`（现有组合，ch10_haiyuandao / ch11_yinsha_dao 用过）：海面干净，但药园要加 `special: tilled` zone、
   门格的道具从石灯笼变成石块——**推翻施工图拍板，本路没落地**，现交付仍是 valley。两条路：(a) 施工图改 wild；
   (b) 给 mapwater 加一个单图开关（maps.json 写一项让本图不撒荷叶），mapwater.py 不在本路白名单。
4. 外星海祭坛石屋先试 `hall_stone`（一圈青石方块，像屋顶不像台），改成 rock＋石屋地面 stone_floor＋门外台阶 steps，读作「高台上凿出的石屋」。
5. **外观 29 条**：人形只按名牌、role note 与本章文案里写明的外貌（苍云龙「绿袍老者」、逆星盟「银衫 / 绿袍」、城门「白衣修士」、
   传送大厅「两名白衣结丹」、易敬「锦衣大汉」、公孙杏「少女」）类型化，其余按身份；同图同时在场的两两分得开（预览核过）。
   名牌里的「白面 / 黑壮 / 鸠面 / 淡金脸 / 凶相」落成肤色与脸型（淡金脸取偏黄的 `tan`）。
6. **非人形 2 张**（`sprites_enemies.py`）：五级海兽 40×32（伏低、背鳍一排、四鳍足、金眼），青色巨虾 48×40（分节弓背、尾扇、长须甩过背、
   一对螯、六只碧绿的眼、身上一缕缕黑气）；待机×2＋出招＋受击＋倒下。两张都补（施工图 5.4 的疑问）：`--coverage` 只数地图 NPC、
   b03–b05 编成与主角同伴，只在编成里的 role 不计入缺图，但 BattleView 取图是 roles → enemies，按 role id 找得到就有图。
   `siji_haishou`（第 11 章）仍无图，不在本章白名单。
7. docs/audio.md 只加了「地图 → BGM」那张表；各曲「用在」一栏（第 12 章顺手补过）没动——不在「同一张表」的口径里。

## 门禁证据

- `python tools/artgen/artgen.py`（全量）→ `--check`：`ARTGEN_IN_SYNC：721 个文件与生成器逐像素一致`，rc 0。
- sha256 改前 / 改后（判据先对一份改了一字节的 battle 背景副本报 `DIFFERENT changed 1`，确认有牙）：改前 678 个、改后 721 个；
  新增 43（4 图 ×3 ＋ 29 张人物表 ＋ 2 张兽图），删除 0，变化 1（`sprites/index.json`）；index.json 与 HEAD 语义比对：
  sheets ＋29、roles ＋29、enemies ＋2，其余键 0 处改动。其余章节产物逐字节不变。
- `python tools/artgen/sprite_preview.py --coverage`：rc 0，`缺图 0 个`（本章 21 个地图 NPC role 全部是 ch13_* 人物表，文思月沿用 ch12_wen_siyue）。
- `python tools/validate.py`：错误 27、警告 0；26 条 `ch13.path.*` 旗标未登记（协调者）、1 条 `ch13.battle.waixinghai_lieyao.intro` 文案 key 重复
  （data/text/battles.json 旧占位，协调者删）。本路文件引起的 0。
- 改过的 6 个文件 Python 读字节：UTF-8 无 BOM、0 个 CR、LF 结尾；JSON 全部可解析。

## 2026-10-14 协调者拍板后的补做：偏差 3「雾中岛海面荷叶」走 (b) 单图开关

- 协调者拍板：主题仍是 valley；白名单临时加 `tools/artgen/mapwater.py`。偏差 2（树画成山石）按 LOW 接受、不改生成器；偏差 7（audio.md「用在」一栏）不归本章。
- `tools/artgen/mapwater.py`：照 `liquid_of` 的读法加 `lotus_of(sc)`——读 maps.json 的 `lotus`，缺省 true，不是布尔当场 ValueError
  （免得写成字符串 "false" 静默当真）；`paint_water` 末尾改成 `elif sc.theme in ("valley", "manor") and lotus_of(sc):` 才撒荷叶。
  荷叶是 paint_water 的最后一步，跳过它不影响前面任何随机种子。模块头注释补一段说明。
- `data/visual/maps.json` 的 ch13_wuzhong 加 `"lotus": false` 与 `lotus_why`。
- 自检：`lotus_of` 对 `{}` 返回 True、对 `{"lotus": false}` 返回 False、对 `{"lotus": "false"}` 抛 ValueError。
- 全量重烘前后 sha256：721 → 721，新增 0、删除 0，变化 1（`maps/ch13_wuzhong/below.png`），其余产物（含 ch12_xiaojie 等 valley / manor 图）逐字节不变。
- `artgen.py --check`：`ARTGEN_IN_SYNC：721 个文件与生成器逐像素一致`，rc 0。
- Read 看过 ch13_wuzhong/below.png：海面与礁间水潭干净无荷叶，药园仍是翻土垄沟。
- mapwater.py、maps.json 读字节：UTF-8 无 BOM、0 个 CR。
