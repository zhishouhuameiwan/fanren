export const meta = {
  name: 'chapter-pipeline',
  description: '章节施工到验收（参数化）：登记 → 引擎 ∥ 数据 ∥ 脚本文案 ∥（地图 → 美术）→ 集成 → 测试 A → 测试 B（通关与平衡）→ 三路审查 ⇄ 整改',
  phases: [
    { title: '登记', detail: '旗标 / 章节卡 / 词表 / 回写 / 删占位' },
    { title: '施工', detail: '引擎路 ∥ 内容 B 数据 ∥ 内容 C 脚本文案 ∥ 内容 A1 地图 → A2 美术' },
    { title: '集成', detail: '规则 25 表、探针、四门禁、全量构建与合缝' },
    { title: '测试', detail: '测试路 A：旧用例登记、上一章终档重生成、静态用例；测试路 B：通关、平衡、本章终档' },
    { title: '审查', detail: '内容 / 测试质量 / 引擎与门禁 三路独立审查' },
    { title: '整改', detail: '按审查裁决整改并重跑全量' },
    { title: '复验', detail: '未通过的审查路复验，循环到通过' },
  ],
}

// args: { ch, title, a, b, prevCh, prevEndScript, startMap, isLast }
const A = args
const NN = String(A.ch).padStart(2, '0')
const PP = String(A.prevCh).padStart(2, '0')
const NOTES = `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch${NN}`
const DESIGN = `H:/Work/Kys/fanren/docs/ch${NN}-design.md`
const CONTRACT = `H:/Work/Kys/fanren/docs/interfaces-p3-ch${NN}.md`
const LCS = `python -I C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/lcs_check.py --from ${A.a} --to ${A.b} <文件…>`

const COMMON = `
环境纪律（必须遵守）：
- 仓库 H:/Work/Kys，游戏工程 H:/Work/Kys/fanren（下文相对路径都相对它）。施工图 ${DESIGN} 与契约 ${CONTRACT} 是第 ${A.ch} 章唯一规范：id、旗标名、地图名、对象名、数字一律照抄，不自创、不改名；两份文档矛盾时以契约为准、报告里写明。两份文档都长，先 \`grep -n "^## \\|^### "\` 看节次再按行号读需要的节；16.1 拍板表里后编号的条目覆盖前面的。
- 原著事实只能用 \`PYTHONIOENCODING=utf-8 python H:/Work/Kys/tools/novel/novel.py {find|read}\` 核；对白与描述要自己写，不许抄原著句子，换词不算重写。
- 协调者备注 ${NOTES}/coordinator-notes.md（若存在）先读：上一章真档核过的起点数值与开工前的更正都在那里。
- 终端代码页是 GBK，中文显示乱码是正常的；判断编码一律 Python 读字节。写含中文或反斜杠的文件用 Write / Edit 或 Python，不用 bash heredoc，不把中文写进 python -c。源码与数据 UTF-8 无 BOM、LF。
- **每一次 Write / Edit 的内容不超过 180 行**（大文件分块：先写一个小的合法骨架或第一块，块末放续写标记，后续 Edit 把标记换成新内容 + 标记，最后去掉标记）。
- 不派子代理；不碰 git（只读的 git diff / git show 可以）；只改自己白名单里的文件，白名单外的问题写进报告。别的章节的施工图（docs/ch1[2-4]-design.md、interfaces-p3-ch1[2-4].md 中不属于本章的）不归你，别碰。
- 门禁：\`python tools/validate.py\`（VALIDATE_OK）、\`python tools/mapgen/genmaps.py --check\`（MAPGEN_IN_SYNC）、\`python tools/validate_selftest.py\`（SELFTEST_OK）、\`python tools/artgen/artgen.py --check\`（ARTGEN_IN_SYNC）。几路并行施工期间 validate 会报别路还没落地的引用——只对自己白名单内文件引起的错负责，别路的缺项列在报告里。
- 构建一律用 **PowerShell 工具** 调 \`& "H:/Work/Kys/fanren/build_logged.bat" <槽名>\`（bash 里 cmd.exe /c 会静默假绿），日志 build-<槽名>.log 按 gb18030 读字节、读前核 mtime；全量约 10–15 分钟。部分测试用 \`build-<槽名>/fanren_tests.exe --gtest_filter=...\`（工作目录 H:/Work/Kys/fanren）。
- **最终回复就是你的报告**：不要把构建 / 门禁丢到后台然后结束回合；必须等结束、读完结果再写报告。报告精炼，不贴文件内容。
`
const KNOWN = `
已核实的引擎口径（前几章踩过）：teleport(map) 落 default spawn、(map,x,y) 给坐标且 (0,0) 视同没给、teleport 之后不触发脚下 enter 触发器；portal / facility 不读 visible / hidden_flag；同格只取第一个 trigger；once=true 必有 set_flag；规则 30 只认 hidden==visible 的互补；规则 13 只从 default spawn 出发 BFS；路径行动 id 以 _dating 结尾、when 含 NPC visible_flag；killable_by 非空且 ⊆ weaknesses；规则 25 的 BATTLE_EXTRA_MEANS 只认编成里的友军；advance_days 单次 ≤ 3650；一份 Lua 拿不到玩家坐标与触发它的对象；facility 拒绝时引擎只播 ui.facility.locked；编成的 defeat_is_fatal 运行期没有代码读——输了要不要结束游戏由脚本读 battle() 返回的 won 自己调 game_over()；flag.set(chNN.done) 只把章节卡放进待播队列，脚本跑完、栈顶回到世界层才播。
`

const PREV_END = `${A.prevEndScript}`
const report = (r) => String(r || '（未返回）')

// ---------------------------------------------------------------- 登记
phase('登记')
const register = await agent(`你是第 ${A.ch} 章的协调者助手，做「登记」这一步（契约第 5 节「协调者：集成时落」与第 6 节协调者白名单里，开工前就能做的那些）。
${COMMON}
白名单：data/flags.json（本章主线旗标，**不含** ch${NN}.path.*——那些等路径行动文件落地后由集成者登）、data/chapters.json（加第 ${A.ch} 章，done_flag ch${NN}.done，照上一章那一行的写法）、data/text/ui.json（ui.chapter.${NN}.numeral / .title，按施工图拍板的卡名）、tests/LexiconTests.cpp（表一补施工图 12.1 的词；表二若缺 {${A.ch}, ${A.b}} 就补）、H:/Work/Kys/docs/大纲.md、H:/Work/Kys/docs/lore/**（回写，按施工图 1.2 / 10.1 回写那一项与契约协调者清单，改动最小），以及契约协调者白名单里写明「只删」的占位文件（删之前 grep 全仓确认没有别处引用；有引用就不删、报告里写明）。
做法：旗标用 Python 读写，核总数 = 原数 + 新增、无重复键、原有条目逐字节不变、格式与原文件一致（json.dumps indent=2 ensure_ascii=False 往返等于原文）；整数 / 日期旗标的说明照施工图第 11 节。登完跑 \`python tools/validate.py\` 应 VALIDATE_OK。
报告（≤ 40 行）：登了多少旗标（核数结果）、各文件改了什么、删了哪些占位、validate 结果、拿不准没改的。`, { label: `register:ch${NN}`, phase: '登记', effort: 'medium' })
if (!register) throw new Error('登记未完成')

// ---------------------------------------------------------------- 施工
phase('施工')
const laneEngine = () => agent(`你是第 ${A.ch} 章的**引擎路**。契约 ${CONTRACT} 第 1 节起各个 E 项（到「下一章接口」那一节之前）是你的全部任务。白名单照契约第 6 节「引擎路」小节；构建槽 **ch${NN}eng**。
${COMMON}
${KNOWN}
要求：判据按契约写成直接读数据 / 直接构 BattleState / 直接调规则函数的断言；依赖内容路尚未落地的本章数据文件的用例，先写好并用 GTEST_SKIP 加一句「等内容路 <文件> 落地」（集成时去掉）；每条新检查配负向（故意写坏 → 必须被抓住）。若契约要改 src/，严格按契约的签名改、零警告；若契约写「本章 0 个代码项」，src/ 一行不动，判据红了就在报告里写明（是引擎真有缺口还是判据写错），不要自己改 src/。先跑 \`python tools/validate_selftest.py\`，再用 PowerShell 跑全量构建（别路同时在改 data/，与本章数据有关的红写明归因）。把实现记录填进契约各 E 项的实现记录小节。
报告（≤ 50 行）：改了哪些文件、用例数（正向 / 负向 / SKIP）、构建日志与 EXITCODE、测试通过数、红灯归因。`, { label: `engine:ch${NN}`, phase: '施工', effort: 'high' })

const laneData = () => agent(`你是第 ${A.ch} 章的**内容路 B：数据**（角色、物品、法术、编成、配方、商店、任务、路径行动、目标链，以及它们的文案）。白名单照契约第 6 节「内容路 B」小节，另加 data/text/ch${NN}_items.json、ch${NN}_path.json、ch${NN}_quests.json、ch${NN}_objectives.json、ch${NN}_battle.json 五份文案（其余 ch${NN}*.json 归内容路 C，ch${NN}_maps.json 归地图路）。记录 ${NOTES}/deviations-B.md。
${COMMON}
${KNOWN}
先读施工图第 5 节（角色）、第 8 节（编成：敌我、波次、友军、hero_absent、defeat_is_fatal、can_escape、drops、charge / actions、破绽、killable_by、规则 25）、第 7 节（系统、商店、配方）、第 9 节（经济账）、第 6 节（支线）、3.4（目标链）、16.3（路径行动）、第 12–13 节（禁词与文案要求）、第 2 节（起点）；契约「脚本与数据口径」一节。模板：上一章的同类文件（data/roles、data/battles/b${PP}_*.json、data/pathactions/ch${PP}.json、data/objectives/ch${PP}.json、data/quests、data/text/ch${PP}_*.json）与 docs/octopath-battle.md 第 5 节、docs/interfaces-octo-pathactions.md 第 2 节。
要求：role 的境界 / 攻防 / 气血照施工图与 src/core/rules/Realm.h 第 100–190 行锚点（数字归平衡，先给合理初值）；reveal 的类别必须在 role 的 weaknesses 里；路径行动 id 以 _dating 结尾、when 含 NPC 的 visible_flag、until 含 ch${NN}.done、同一 NPC 多条时段不重叠；目标链 text_key 写进 ch${NN}_objectives.json；文案自己写、0 处 {key. / {pad.、禁词按第 12 节。写完跑 validate 分清我的 / 别路的错；再跑原创性自检 \`${LCS}\`（只传你写的那五份文案；SELF_CHECK_OK 后 over 必须 0，超的句子改说法再测）。
报告（≤ 60 行）：各类文件件数、各场编成概要（境界 / 气血 / 破绽 / 掉落）、validate 我方 0 错的证据、原创性自检输出、偏差清单。`, { label: `content:B-data:ch${NN}`, phase: '施工', effort: 'high' })

const laneScripts = () => agent(`你是第 ${A.ch} 章的**内容路 C：脚本与主线文案**。白名单照契约第 6 节「内容路 C」小节（scripts/ch${NN}/**、${PREV_END} 只在末尾补一行 \`teleport("${A.startMap}")\`、data/text/ch${NN}*.json 中**除** ch${NN}_items / _path / _quests / _objectives / _battle / _maps 之外的文案、施工图只追加「施工偏差」）。记录 ${NOTES}/deviations-C.md。
${COMMON}
${KNOWN}
先读施工图第 3 节全部（判据、节点表、3.1 挂点与脚本名、3.2 脚本动作与分支、3.3 日历、3.4 目标链）、第 13 节（文案要求与 13.1 每段框架件清单——写手判据）、第 7 节、第 11 节（旗标谁置谁读）、第 12 节（禁词与首见章号）、第 2 节、第 6 节、第 17 节（纪律）；契约「脚本与数据口径」与「门禁与既有测试的依赖」两节。若施工图 16.2 写了「派写手前须补读某些原著章」，先用 novel.py 读完那些章再写对白。模板：scripts/common/api.lua 全文、H:/Work/Kys/fanren/docs/interfaces-p3-script.md、上一章 scripts/ch${PP}/ 里几份同类脚本、data/text/ch${PP}_main.json 的 key 命名。
要求：每个节点一个脚本，文件名照 3.1；动作、置旗、分支、门闸照 3.2，不多不少；每一处 give / take / bottle / advance_days / battle / realm.* / party.* 都看返回值并有走得通的一支；文案按 13.1 的「剧情事实 / 质感边界」写，自己的句子，**不照原著镜头顺序逐句换词**，每段 4–12 句，韩立谨慎少言；名字按第 12 节解禁次序；0 处 {key. / {pad.。写完跑 validate 分清我的 / 别路的错；用 Python 扫你写的文案的禁词与首见次序；再跑原创性自检 \`${LCS}\`（传你写的全部文案文件；SELF_CHECK_OK 后 over 必须 0，超的句子改说法再测）；所有 Lua 用 vendor 里的 Lua 过一遍语法（luac -p 或等价）。
报告（≤ 60 行）：脚本数、文案条数与汉字数、门闸与退路一览、validate 我方 0 错的证据、禁词扫描、原创性自检输出、偏差清单。`, { label: `content:C-scripts:ch${NN}`, phase: '施工', effort: 'high' })

const laneMaps = () => agent(`你是第 ${A.ch} 章的**内容路 A1：地图**。白名单：tools/mapgen/genmaps_ch${NN}.py（新）、tools/mapgen/genmaps.py（只加登记，排在第 ${A.prevCh} 章之后）、maps/ch${NN}_*.tmj 与 maps/tilesets/terrain_ch${NN}_*.tsj（生成物）、data/text/ch${NN}_maps.json（地名 key 与 deny_text_key 文案）。记录 ${NOTES}/deviations-A.md。美术（data/visual、sprites、assets/art）归 A2，不归你。
${COMMON}
${KNOWN}
先读施工图第 4 节（地图：每张的主题 / 时辰 / 尺寸 / 出入口 / 挂点 / NPC / default spawn）、3.1 节（挂点对象名、mode / once / guard_flag / set_flag、teleport 落点）、第 5 节（role 与出场退场旗标、在哪张图）、16.3（路径行动的 npc 对象名）、第 6 节（支线挂点）；契约第 6 节「内容路 A」小节。模板：tools/mapgen/genmaps_ch${PP}.py（整份，写法模板）、tools/mapgen/genmaps.py 第 120–220 行（对象辅助函数签名）与末尾登记处；H:/Work/Kys/docs/map_spec.md 第 4 节与第 7 节规则 13、16、17、18、19、20、29、30；data/text/ch${PP}_maps.json。
要求：对象名与属性一律照施工图；脚本、role 文件由别路写，validate 报「不存在」属别路；出入口规则 29 两头互指、按住不回弹；同图 NPC 不重名不同 role 同时在场（规则 30）；设施不压唯一走廊；剧情入口不压 spawn；同格不叠两个触发器；脚本落点那个 spawn 标 default。分块写生成器（每图一个函数），确定性。跑 \`python tools/mapgen/genmaps.py\` 生成、\`--check\` 必须 MAPGEN_IN_SYNC；跑 validate 把错分成我的 / 别路的，我的必须 0。
报告（≤ 50 行）：每张图的尺寸、对象数、门的两头、default spawn、validate 我方 0 错的证据、偏差清单。`, { label: `content:A1-maps:ch${NN}`, phase: '施工', effort: 'high' })

const laneArt = (mapsReport) => agent(`你是第 ${A.ch} 章的**内容路 A2：美术与音频登记**。地图已由 A1 生成（报告附后）。白名单：data/visual/maps.json（本章各图的视觉预设，只用已有主题与时辰组合）、data/visual/battles.json（本章各场的战斗背景）、data/visual/looks.json（本章新 role 的外观；契约写明要删的占位条目一并删）、tools/artgen/sprites_enemies.py（只加施工图写明的非人形敌人 / 友军）、tools/artgen/sprites.py 的 BATTLE_OVERRIDES（只在施工图要求地图形态与战斗形态不同时加）、assets/art/**（artgen 产物）、tools/audiogen/catalog.py（只在 MAP_BGM 上方注释表加登记行，元组不动——全用已有曲子）、docs/audio.md（同一张表）。记录追加到 ${NOTES}/deviations-A.md。
${COMMON}
先读施工图第 4 节（主题 / 时辰 / BGM）、第 5 节（role 外观描述）、第 8 节（敌人外形与战斗背景）；模板 docs/art-maps.md、docs/art-sprites.md、data/visual/maps.json 里 ch${PP}_* 条目、looks.json 里 ch${PP}_* 条目、sprites_enemies.py 里上一章加的几只。
要求：跑 \`python tools/artgen/artgen.py\` 全量烘焙，然后 \`--check\` 必须 ARTGEN_IN_SYNC；\`python tools/artgen/sprite_preview.py --coverage\` 缺图 0（role 文件由内容路 B 写，若尚未落地导致数不到，等几分钟再跑一次；最后一次的输出写进报告）；跑 validate 记录。
A1 报告：
${'${MAPS}'}
报告（≤ 40 行）：外观条数、新精灵、ARTGEN / coverage 的输出行、偏差。`.replace('${MAPS}', report(mapsReport).slice(0, 2500)), { label: `content:A2-art:ch${NN}`, phase: '施工', effort: 'medium' })

const lanes = await parallel([
  () => laneEngine().then(r => ({ key: 'engine', report: r })),
  () => laneData().then(r => ({ key: 'data', report: r })),
  () => laneScripts().then(r => ({ key: 'scripts', report: r })),
  () => laneMaps().then(async r => {
    const art = await laneArt(r)
    return { key: 'maps+art', report: `【A1】${report(r)}\n【A2】${report(art)}` }
  }),
])
const laneResults = lanes.filter(Boolean)
log(`施工 ${laneResults.length}/4 路回报`)

// ---------------------------------------------------------------- 集成
phase('集成')
const integrate = await agent(`你是第 ${A.ch} 章的**集成者**（协调者角色）。四路已交付（报告附后）。你的事：契约第 5 节「协调者：集成时落」剩下的项、合缝、四门禁全绿、全量构建与测试——**除了本章通关 / 登记类测试（测试路的事）以外**全绿。构建槽 **integ${NN}**。
${COMMON}
${KNOWN}
白名单：tools/validate.py（只 CHAPTER_MEANS / BATTLE_EXTRA_MEANS / MEANS_LAST_CHAPTER 那一段）、tools/validate_selftest.py（只挪探针与断言末章号那几处${A.isLast ? '；本章是全作最后一章，范围外探针无下一章可挪——按契约写的办法（测试副本里临时合成一张下一章编成）改，阴性对照照旧要有' : ''}）、data/flags.json（登 ch${NN}.path.*，照上一章 path 旗标的说明写法）、data/text/battles.json（只删契约点名的旧占位）、tests/Ch${NN}EngineTests.cpp（只去掉 GTEST_SKIP）、docs/handoff.md 第 4 节那张表（契约点名的那一行）；**合缝修补**：四路交付的本章文件里引用对不上的小错（key、对象名、role id、脚本路径、字段形状）可以直接修，以施工图与契约为准，每处记进 ${NOTES}/integration.md；超过 20 行或要改设计的写成待办不动手。
步骤：
1. 契约第 5 节协调者项逐条做（规则 25 表：登之前核编成友军、角色法术、五行与境界能施展；探针挪位并保留阳性 / 阴性对照）。
2. 四门：validate → genmaps --check → validate_selftest → artgen --check，红的逐条归因与合缝。
3. 全量构建（PowerShell，后台起、轮询日志直到 EXITCODE 出现）。零警告；红灯逐条归因：本章数据 / 脚本引起的合缝修；属测试路登记类的（上一章通关终档、章节卡、kScriptTransfers、kOffChainFlags、kLastChapter、按章上限、MEANS_LAST_CHAPTER 放行表）列待办；第 1–${A.prevCh} 章的其它红先看是不是本章改坏了共享文件。修一轮重跑受影响测试，最后完整构建一次作终证。
4. 把 ${NOTES}/deviations-{A,B,C}.md 整理进施工图第 18 节（按路分小节，去重，保留原意）。
四路报告：
${laneResults.map(r => `--- ${r.key} ---\n${report(r.report).slice(0, 3000)}`).join('\n')}
报告（≤ 70 行）：协调者项各做了什么；四门最终输出行；终证构建日志名、EXITCODE、tests passed 那一行、警告数、剩余红灯清单与归属；合缝修补清单；待办。`, { label: `integrate:ch${NN}`, phase: '集成', effort: 'high' })

// ---------------------------------------------------------------- 测试
phase('测试')
const testA = await agent(`你是第 ${A.ch} 章的**测试路 A**：既有用例的登记类改动、第 ${A.prevCh} 章终档重生成、第 ${A.ch} 章的静态（不走通关）用例与专项词表用例。构建槽 **ch${NN}test**。
${COMMON}
${KNOWN}
白名单：契约第 6 节「测试路」小节列的旧用例改法（上一章通关终点断言——注意第 ${A.prevCh} 章驱动 tests/Ch${PP}Driver.h 里可能写死了终点 mapId 与坐标，一并改；上一章章节卡断言；tests/UiStyleTests.cpp 的 ChapterTableData / ChapterCards；tests/ObjectiveTests.cpp 的 kScriptTransfers（上一章章末那一行落点改 ${A.startMap}）与 kOffChainDoorKeys；tests/NpcPresenceTests.cpp 的 kOffChainFlags 与 kLastChapter；tests/PathActionTests.cpp 按章上限；tests/Ch07BattleDataTests.cpp 的 MEANS_LAST_CHAPTER 放行表；tests/MapLinkTests.cpp 只在本章新图触发它的已知假设时）、tests/ChapterFixture.h（加 kChapter<本章>Ending… 与 FANREN_WRITE_CH${NN}_FIXTURES，照上一章三行）、tests/fixtures/ch${PP}-end-{first,second}.sav（重生成）、saves/README.md，以及新文件 tests/Ch${NN}Data.h、Ch${NN}TriggerModeTests.cpp、Ch${NN}BattleDataTests.cpp、Ch${NN}LedgerTests.cpp、**Ch${NN}LexiconTests.cpp**（施工图 12.2 的章内先后、本章专项「只许出现在某处 / 全章 0 处」断言、分母先验——第 11 章审查判过「没写专项词表用例」是 HIGH，照 tests/Ch11LexiconTests.cpp 的写法）。
先读：集成者报告（附后）列的待办；契约「门禁与既有测试的依赖」一节；施工图第 15 节验收、3.1、第 11 节、第 12 节、第 18 节。模板：tests/Ch${PP}TriggerModeTests.cpp、Ch${PP}BattleDataTests.cpp、Ch${PP}LedgerTests.cpp、Ch${PP}Data.h、Ch11LexiconTests.cpp、Ch${PP}AcceptanceTests.cpp、tests/ChapterFixture.h 末尾几章的那几行。
步骤：1. 先跑一次全量确认红灯清单。2. 改登记类用例；重生成第 ${A.prevCh} 章终档：设 FANREN_WRITE_CH${PP}_FIXTURES=1 只跑 Ch${PP}Walkthrough.*，再清掉变量跑一遍不带写开关的 Ch${PP}*（必须绿）；用 Python 比新旧终档（旧版用 git -C H:/Work/Kys show HEAD:fanren/tests/fixtures/ch${PP}-end-first.sav），差异应只在地图与坐标。3. 写静态用例（只做读数据 / 扫脚本 / 读地图那一半的验收条目），每条配负向；文件头写明对应验收条号；判据写施工图的字面量，不从被测物推导。4. 全量构建到全绿（零警告）；剩下的红若是内容不符合施工图（不是判据写错），写进报告「交整改」，不改数据。
集成者报告：
${report(integrate).slice(0, 3500)}
报告（≤ 60 行）：各文件改动、新增用例数（正向 / 负向）、上一章终档新旧差异字段、终证日志名与 EXITCODE、tests passed 那一行、警告数、交整改清单。`, { label: `test:A:ch${NN}`, phase: '测试', effort: 'high' })

const testB = await agent(`你是第 ${A.ch} 章的**测试路 B**：两侧通关测试、平衡（BattleHand 量本章各场）、本章终档、切片。构建槽 **ch${NN}walk**。测试路 A 已交付（报告附后），上一章终档已重生成。
${COMMON}
${KNOWN}
白名单：新文件 tests/Ch${NN}Driver.h、tests/Ch${NN}AcceptanceTests.cpp（两侧通关 + 终点断言 + 写终档）、tests/Ch${NN}SliceTests.cpp（验收里要切片的条目）、tests/fixtures/ch${NN}-end-{first,second}.sav（新，由通关以 FANREN_WRITE_CH${NN}_FIXTURES=1 生成）；**平衡**：data/battles/b${NN}_*.json 与本章新建的敌方 / 友军 role 的战斗数值（气血、攻防、架势、行动、蓄势倍率——不改破绽、不改 killable_by、不改编成人数与友军构成）；src/core/rules/Realm.h 只改锚点注释里本章标定的那一句；${NOTES}/balance.md。
先读：施工图第 15 节（验收，尤其通关那一条：意图玩家写死的选项、起点先验、终点先验、日差与资源字面量）、3.2、3.3、第 8 节（各场意图与平衡目标、战前回血）、第 18 节；模板 tests/Ch${PP}Driver.h（整份）、Ch${PP}AcceptanceTests.cpp、Ch${PP}SliceTests.cpp、tests/BattleHand.h（先证它自己稳：同一存档三次跑结局相同）、${'C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch11/balance.md'}（平衡记录写法）。
要求：通关从 tests/fixtures/ch${PP}-end-{first,second}.sav 起步（不手搭起点），两侧走施工图写死的同一组选项；驱动走真实入口（WorldScene 走图、interact、openFacility → 各面板的真实 API、BattleScene 真打），不直接改 state 注入结果。终点断言照验收字面量写死（不从驱动推导）；施工图字面量与实际不符时，判断是施工图算错还是内容错——施工图算错就按实际写并在报告里列「施工图要改」，内容错就列「交整改」。平衡不达标就调白名单内数值，记 balance.md（调前 / 调后、回合数、剩余气血）。通关绿后以写开关写终档，再清掉变量重跑比对（NOWRITE 必须绿），用 Python 列终档关键字段（day、realm / cap / former、HP / MP、修为、灵石、关键物品、party、mapId 与坐标与朝向）。切片每条写明起点存档、配负向。最后全量构建到全绿（零警告）。
测试路 A 报告：
${report(testA).slice(0, 3000)}
报告（≤ 70 行）：两侧各场回合数与胜负、平衡调了什么、终档关键字段表、新增用例数、终证日志名与 EXITCODE、tests passed 那一行、警告数、交整改 / 施工图要改清单。`, { label: `test:B:ch${NN}`, phase: '测试', effort: 'high' })

// ---------------------------------------------------------------- 审查 ⇄ 整改
const VERDICT_SCHEMA = {
  type: 'object',
  properties: {
    score: { type: 'number' }, verdict: { type: 'string', enum: ['PASS', 'FAIL'] }, reportFile: { type: 'string' },
    findings: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          id: { type: 'string' }, severity: { type: 'string', enum: ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'] },
          where: { type: 'string' }, problem: { type: 'string' }, evidence: { type: 'string' }, fix: { type: 'string' },
        },
        required: ['id', 'severity', 'where', 'problem', 'evidence', 'fix'],
      },
    },
  },
  required: ['score', 'verdict', 'reportFile', 'findings'],
}
const REVIEWS = [
  {
    key: 'content', file: `H:/Work/Kys/fanren/docs/ch${NN}-review.md`,
    prompt: `你是第 ${A.ch} 章的**独立内容审查**（原著、原创性、文学质量、在场与名字、玩家够得着）。你没有参与施工，只审不改：唯一能写的文件是 H:/Work/Kys/fanren/docs/ch${NN}-review.md。口径参考 H:/Work/Kys/fanren/docs/ch11-review.md（结构、评分、换词不换骨的判法）。
审：1. 原著：施工图 1.1 硬约束至少抽 40 条，对照落地文案与数据，用 novel.py 核原文；名字首见章是否被提前；禁词 0 处。2. 原创性：先跑机器口径 \`${LCS}\`（传 data/text/ch${NN}_*.json 全部；SELF_CHECK_OK 后 over 必须 0），再按 13.1 挑至少 15 段与原著并排读，判「换词不换骨」（照原著镜头顺序逐句换词、或把原著动作反着说也算），给处数与占比。3. 文学质量：13.1 剧情事实到位、质感边界守住、韩立口吻、节奏、说明书腔、重复句式、现代词。4. 玩家够得着：每个门闸与退路在脚本里真有走得通的一支；NPC 一人一时一地；目标链次序与门闸。5. 统计：条数、汉字数、最长一句、句式重复。
报告结构：结论与分数 / 问题清单（C-1…，严重度、位置、证据、改法）/ 抽查记录 / 统计。评分：5 优秀；4 可交付只剩 LOW；3 有 HIGH；≤2.5 大面积重写。PASS = 分数 ≥ 4.0 且无 CRITICAL / HIGH。`,
  },
  {
    key: 'tests', file: `H:/Work/Kys/fanren/docs/ch${NN}-test-review.md`,
    prompt: `你是第 ${A.ch} 章的**独立测试质量审查**。只审不改：唯一能写的文件是 H:/Work/Kys/fanren/docs/ch${NN}-test-review.md。口径参考 docs/handoff.md 第 150–215 行（判据会说谎的几种长相）与 docs/ch11-test-review.md。
审 tests/Ch${NN}*.cpp / .h 与本章改过的旧用例（git -C H:/Work/Kys diff -- fanren/tests 只读）：1. 通关是否从真实上一章终档起步、走真实入口；终点断言是否施工图写死的字面量；两侧是否真不同。2. 每条新检查是否有负向；有没有空转（缺项即过、分母为 0、只钉一个点、常量比较、判据从被测物推导、顺序断言在缺项时 npos 恒真）。3. 施工图第 15 节验收逐条列覆盖表。4. 上一章终档重生成是否只差地图与坐标（Python 读新旧两版）。5. 自己跑一次相关测试（PowerShell：build-ch${NN}walk 或 build-ch${NN}test 下的 fanren_tests.exe --gtest_filter=Ch${NN}*:Ch${PP}*，工作目录 H:/Work/Kys/fanren）。
报告结构：结论（Approve / RequestChanges）/ 问题清单（T-1…）/ 覆盖表 / 实跑记录。PASS = Approve；score 填 5 或 3。`,
  },
  {
    key: 'gate', file: `H:/Work/Kys/fanren/docs/ch${NN}-gate-review.md`,
    prompt: `你是第 ${A.ch} 章的**独立引擎与门禁审查**（C++ 与工具）。只审不改：唯一能写的文件是 H:/Work/Kys/fanren/docs/ch${NN}-gate-review.md。
审：本章 src/ 改动（git -C H:/Work/Kys diff -- fanren/src，若有）对照契约 E 项；tests/Ch${NN}EngineTests.cpp 是否钉住契约各 E 项的判据、负向是否真有牙；tools/validate.py 的 CHAPTER_MEANS[${A.ch}] / BATTLE_EXTRA_MEANS / MEANS_LAST_CHAPTER 与本章各场编成、我方实际手段对不对（逐场：那一样破绽玩家此时真打得到吗——法术 needRealm / needMp、友军 maxMp、物品来路）；tools/validate_selftest.py 探针挪位是否有效（在临时副本里做阴性对照）；跑 validate 与 selftest 记录输出。
报告结构：结论（Approve / RequestChanges）/ 问题清单（G-1…）/ 阴性对照记录。PASS = Approve；score 填 5 或 3。`,
  },
]
const current = REVIEWS.map(r => ({ ...r, result: null }))
const history = []
let round = 0
while (round < 3) {
  round += 1
  phase(round === 1 ? '审查' : '复验')
  const pending = current.filter(r => !(r.result && r.result.verdict === 'PASS'))
  const prevFix = history.length ? history[history.length - 1].fix : ''
  const results = await parallel(pending.map(r => () =>
    agent(`${r.prompt}
${COMMON}
${round > 1 ? `这是第 ${round} 轮**复验**：你上一轮的报告在 ${r.file}，整改方的回应附后。逐条复核上一轮的问题是否真的关了（读改后的文件、重跑相关测试 / 自检），再看整改有没有带出新问题；在报告末尾**追加**「第 ${round} 轮复验」一节（不删上一轮），给新的分数与结论。findings 只列仍未关的与新发现的。
整改方回应：
${report(prevFix).slice(0, 4000)}` : ''}
测试路交付摘要：A：${report(testA).slice(0, 1200)}
B：${report(testB).slice(0, 2000)}`,
      { label: `review:${r.key}#${round}`, phase: round === 1 ? '审查' : '复验', effort: 'high', schema: VERDICT_SCHEMA })
      .then(res => ({ key: r.key, res }))))
  for (const x of results.filter(Boolean)) {
    const t = current.find(c => c.key === x.key)
    if (t && x.res) t.result = x.res
  }
  const summary = current.map(c => `${c.key}:${c.result ? `${c.result.verdict}/${c.result.score}/${c.result.findings.length}` : '无结果'}`).join('、')
  log(`第 ${round} 轮审查：${summary}`)
  const failing = current.filter(c => !(c.result && c.result.verdict === 'PASS'))
  if (failing.length === 0 || round >= 3) { history.push({ round, summary, fix: null }); break }

  phase('整改')
  const findings = failing.flatMap(c => (c.result ? c.result.findings : []).map(f => ({ review: c.key, ...f })))
  const fix = await agent(`你是第 ${A.ch} 章的**整改路**。未通过的审查提出了下面这些问题（JSON；报告全文在 docs/ch${NN}-review.md、docs/ch${NN}-test-review.md、docs/ch${NN}-gate-review.md）。逐条整改：成立的改到位；不成立的写明理由与证据。构建槽 **ch${NN}fix**。
${COMMON}
${KNOWN}
白名单：本章内容文件（scripts/ch${NN}/**、data/text/ch${NN}_*.json、本章新建或改写的 data 文件、tools/mapgen/genmaps_ch${NN}.py 与 maps/ch${NN}_*.tmj、data/visual 的本章条目与 assets/art 产物）、tests/Ch${NN}*、本章改过的旧用例、tests/fixtures/ch${NN}-end-*.sav（通关改了就以写开关重生成再 NOWRITE 复核）、tools/validate.py 与 validate_selftest.py 的本章那几段、src/ 只在审查点名且契约允许时、施工图第 18 节（追加「整改记录」小节）。**不许改审查报告**；不许为了让用例绿而放宽判据。文案整改：照 13.1 的剧情事实写自己的句子，改完跑 \`${LCS}\`（over 必须 0）并扫禁词与首见次序。改了地图生成器或 data/visual 就重跑 genmaps 与 artgen 并 --check。
问题清单：
${JSON.stringify(findings, null, 1)}
做完跑四门与全量构建到全绿（零警告）。报告（≤ 70 行）：逐条（编号 / 改了什么或为何不改 / 证据）、原创性自检输出、终证日志名与 EXITCODE、tests passed 那一行。`, { label: `fix#${round}:ch${NN}`, phase: '整改', effort: 'high' })
  history.push({ round, summary, fix })
}

return {
  register: report(register).slice(0, 1500),
  lanes: laneResults.map(r => ({ key: r.key, report: report(r.report).slice(0, 2500) })),
  integrate: report(integrate).slice(0, 3500),
  testA: report(testA).slice(0, 2500),
  testB: report(testB).slice(0, 3500),
  final: current.map(c => ({ key: c.key, verdict: c.result && c.result.verdict, score: c.result && c.result.score, open: c.result ? c.result.findings : null, file: c.file })),
  history: history.map(h => ({ round: h.round, summary: h.summary, fix: h.fix ? report(h.fix).slice(0, 2500) : null })),
}
