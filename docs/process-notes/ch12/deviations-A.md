# 第 12 章内容路 A1（地图）施工偏差与交接（2026-10-13）

交付：`tools/mapgen/genmaps_ch12.py`（新，十图各一函数，确定性）、`tools/mapgen/genmaps.py`（check() 里第 11 章之后登记 genmaps_ch12）、
`maps/ch12_*.tmj` ×10、`maps/tilesets/terrain_ch12_*.tsj` ×10、`data/text/ch12_maps.json`（10 个地名 key ＋ `ch12.block.dixue`）。

## 别路必须照抄的数字

- **鬼雾三处岔口坐标**（`zoupian_a/b/c.lua` 的 `teleport("ch12_guiwu", x, y)`，测试路 `kScriptTransfers` 注释）：
  - a：`(27, 33)`（错路尽头 `trigger_zoupian_a` 在 (36, 28)）
  - b：`(18, 24)`（`trigger_zoupian_b` 在 (8, 16)）
  - c：`(30, 16)`（`trigger_zoupian_c` 在 (44, 26)）
  三格都可走、都不压任何对象；生成器顶部 `FORK_A/B/C` 常量同值。
- **BGM**（A2 写 `tools/audiogen/catalog.py` 注释表与 `docs/audio.md`）：tushan→bgm_wild、dixue→bgm_cave、dadian→bgm_indoor、
  guiwu→bgm_night、xiaojie→bgm_valley、binghuo→bgm_cliff、baoguang→bgm_indoor、taqian→bgm_cave、hanlitai→bgm_cliff、
  lingquan→bgm_cave。全是 `MAP_BGM` 元组里已有的曲子，元组不动。

## 偏差（施工图没写死、由本路定的）

1. **`region` = `luanxinghai_xutian`**：施工图 4.2 写「由协调者定一个值」，协调者备注没给；沿 `luanxinghai_*` 前缀自定。引擎与 validate 只要求非空字符串，协调者要改只动生成器 `REGION` 一处、重跑。
2. **`data/text/ch12_maps.json` 归属**：契约 6.4 把 `data/text/ch12*.json` 整体划给内容路 C；本次派工把这一份划给 A1。C 不要再建同名 key（重复 key 会让 `loadGameData` 起不来）。
3. **四条横贯触发带**都在 spawn 前方第 3 格（照第 11 章 `trigger_chudao` 先例），不是紧贴的下一行：tushan `trigger_dengdao` (22,28) 5×1、dadian `trigger_rudian` (19,34) 5×1、guiwu `trigger_tupo` (24,42) 7×1、binghuo `trigger_guangbi` (6,10) **1×8 竖带**（光壁在西、往东走）。核过：把带当墙，从 default spawn 出发够不到该图任何别的挂点 / NPC / 门。
4. **为让单格 enter 挂点必经而收窄的地形**（施工图只说位置，没说必经）：tushan 光罩 `trigger_guaizhao` (24,10) 在洞口前一格宽坡道南口；binghuo 黑沙漠西缘用沙丘留一格沙口放 `trigger_shamo` (55,13)；石殿门一格宽、`trigger_shidian` (71,13) 在门内；taqian 塔前一格宽石径，`trigger_taqian` (30,10) 与塔门 `trigger_tamen` (30,7) 隔 3 格；寒骊台 `trigger_taijie` 放在台壁唯一缺口 (20,24)（＝台阶顶）。
5. **interact 挂点画成实心石栏 / 石台（building gid 8）**，站旁边按（第 11 章生成器同写法）。寒骊台「洞口」画成岩隙（实心 gid 5），不画成水面。
6. `trigger_huayuan` 脚本名 `yuanyao.lua`、`trigger_tingzi` → `xiaojie.lua`、`trigger_guaizhao` → `guairen.lua`、`trigger_huangwu` → `pozhen.lua`、`trigger_shuichi` → `qixialian.lua`、`trigger_shizhu` → `duoshe.lua`、`trigger_chuansongzhen` → `chuansong.lua`、`trigger_shishan` → `lingshen.lua`、`trigger_guangbi` → `rongyan.lua`、`trigger_shamo` → `tihun.lua`、`trigger_jingzhu` → `lianjing.lua`、`trigger_yuqiao` → `quba.lua`、`trigger_changlang` → `huanjing.lua`、`trigger_taqian` → `neidian.lua`、`trigger_tamen` → `cengjie.lua`、`trigger_qingshi` → `juanzhou.lua`、`trigger_taijie` → `ruyi.lua`、`trigger_jitan` → `butian.lua`、`trigger_dongkou` → `xuangu.lua`、`trigger_xiaojian` → `canhun.lua`；NPC：`npc_wen_siyue` → `lidao.lua`、`npc_shaonian` → `xiaocha.lua`、dadian `npc_xuan_gu` → `jiumeng.lua`、xiaojie `npc_jiyin` → `baishi.lua`、`npc_yuan_yao_lq` → `lingquan.lua`（按 3.2「脚本名＝done_flag 去前缀」，三处例外 tuizhu→guairen、canyuan→xiaocha、done→lingquan）。C 的 `@hook` 照地图写。

## 门禁证据

- `genmaps.py --check`：MAPGEN_IN_SYNC（75 张）。
- `validate.py`（真树）：本路文件引起的错 0；剩下全属别路：39 条「脚本不存在 scripts/ch12/*.lua」（C）、10 条「从起始地图走不到这张图」（C 的 teleport 与 zhuzhong.lua patch 未落）、文案 key 缺（C）、quest 谓词旗标「没有任何脚本会置」（C）。
- 草稿副本（scratchpad）里补上按 3.2 写的空壳脚本（只 `flag.set` / `teleport`，`@hook` 照地图）与 zhuzhong.lua 末行 teleport 后跑 validate：除文案 key 缺与 13 条 `ch12.path.*` 旗标未登记（协调者）外 0 错——规则 13、16、17、18、19、29、30 与路径行动的 NPC 对象引用全过。

# 第 12 章内容路 A2（美术与音频登记）施工偏差（2026-10-13）

交付：`data/visual/maps.json`（ch12 十条）、`data/visual/battles.json`（b12 四条）、`data/visual/looks.json`（新 look 21 条、roles 23 条；删 `duoding_xiushi`、`xutian_hudian_kuilei` 两条 roles 映射）、
`tools/artgen/sprites_enemies.py`（`_chapter12_rows` / `CHAPTER12_BEASTS` 九只）、`assets/art/**`（全量重烘）、`tools/audiogen/catalog.py` 注释表 10 行、`docs/audio.md`（第 12 节新表 10 行 ＋ 曲目表「用在哪里」列补图名）。

1. **两个 role 不另做 look，直接指向同一人的旧 look**：`zi_ling_xutian` → `ch11_zi_ling_zhenrong`、`yuan_yao_jiedan` → `ch10_yuan_yao`（施工图只说「按名牌身份配」，同一个人换张脸反而认不出）。所以人形 23 个 role、新 look 21 条。
2. **玄骨两段**：`ch12_xuan_gu` 以 `ch12_xuangu_shaonian` 为 base（同一张脸，A23），只换松绿袍（A55）；少年版穿黑袍茶缘（曲魂躯壳的斗篷色）。
3. **黑袍人**：`ch12_yuan_yao_heipao` 以 `ch10_yuan_yao` 为 base，黑斗篷 ＋ 低兜帽（cowl，盖住眉眼），名牌之外不露身份。
4. **熔岩画成了水**：冰火道两条熔岩是 gid 3 挡路格，烘焙器把挡路的 gid 3 一律画成水面，`zones` 不覆写水格；现有主题里没有熔岩地面，16.1 第 29 条不许加新主题——只把夹道两侧与黑沙漠铺成 `scorched`、粒子加 `ember`。要真熔岩得加主题或水面变体（白名单外，留给协调者定）。
5. **荒岛北坡岩顶是林冠**：`wild` 主题的岩顶固定是 `forest`，`structures.style` 只有 cave / valley 能改岩顶，都不对；地面已用 `zones` 铺成枯土 / 礁滩 / 沙滩（A3「草木不生」），北坡岩顶的绿留着。
6. **推断成房子的岩体改指认为 rock**：tushan 北坡（3,3–44,10）、南岸两道岩坎、东岸三块礁石；xiaojie 东南小石山两块；hanlitai 台面洞口岩隙（11,8–12,9）。tushan 中部石屋保留为房子（石屋）。
7. **宝光阁玉桥**：路格挨水被画成栈桥木板，用 `zones.road = flagstone` 改成石桥。
8. **战斗背景的 map 字段**：③ `b12_neidian_duoding`、④ `b12_hanlitai_xuangu` 都填 `ch12_hanlitai`（n29、n31 都在寒骊台）；② 填 `ch12_binghuo`。
