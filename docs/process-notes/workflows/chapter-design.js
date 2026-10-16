export const meta = {
  name: 'chapter-design',
  description: '章节施工图（参数化）：原著摘要 → 研究笔记 → 四路分节写作 → 组装 → 四镜头对抗核对 → 整改',
  phases: [
    { title: '摘要', detail: '本章与下一章的原著逐章摘要（sonnet）' },
    { title: '研究', detail: '核原著、核引擎、定拍板，写 research.md（规范表）' },
    { title: '写作', detail: '施工图三段 ∥ 契约，各自分块写' },
    { title: '组装', detail: '拼成 docs/chNN-design.md，查节次' },
    { title: '核对', detail: '原著 / 引擎可行性 / 流程合规 / 一致性 四镜头' },
    { title: '整改', detail: '按核对结果改文档' },
  ],
}

// args: { ch, title, a, b, minutes, outlineLines, loreRoleLines, digests:[[a,b]...], nextDigests:{ch, ranges:[[a,b]...]},
//         prevContract, prevContractSection, prevDesign, prevDisciplineLast, decisions, extraReading }
const A = args
const NN = String(A.ch).padStart(2, '0')
const NOTES = `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch${NN}`
const DESIGN = `H:/Work/Kys/fanren/docs/ch${NN}-design.md`
const CONTRACT = `H:/Work/Kys/fanren/docs/interfaces-p3-ch${NN}.md`
const RESEARCH = `${NOTES}/research.md`

const COMMON = `
环境纪律（必须遵守）：
- 仓库在 H:/Work/Kys，游戏工程在 H:/Work/Kys/fanren。原著检索只能用 \`PYTHONIOENCODING=utf-8 python H:/Work/Kys/tools/novel/novel.py {find|read|toc|where}\`；\`read <章号>\` 输出 JSON，字段 chapter_lines 是总行数，默认只给前 40 行，用 \`--start N --lines M\` 分段读到末尾。原著是 GB18030，Grep 搜不到中文，别用 grep 搜 novel/。
- 原著细节一律以 novel.py 读到的原文为准，不凭记忆写；拿不准就去读。
- 终端代码页是 GBK，UTF-8 中文在终端里显示成乱码是正常的，不要据此判断文件坏了。写含中文或反斜杠的文件用 Write / Edit 工具或 Python，不要用 bash heredoc。
- 不要派子代理；不要整份重读超过 1500 行的文件（先 grep "^#" 看节次再按行号读）；不要碰 git；不要动 fanren/build* 目录。
- **前面的章节正在同一棵工作树里施工 / 审查**：你只许写本派工单点名的文件，fanren/ 下别的文件一律只读。
- **最终回复就是你的报告**：不要把命令丢到后台然后结束回合；所有命令跑完、读完结果再写报告。报告精炼，不贴文件内容。
`
const CHUNKED = `
**分块写文件（硬规定）**：每一次 Write 或 Edit 的内容不超过 180 行。第一次用 Write 写文件头与第一块，块末放一行 \`<!-- 续写点 -->\`；之后每一块用 Edit 把那一行换成「新内容 + 同一行标记」；最后一块把标记换成空。每写完一块就接着写下一块，不要停下来总结。`

const KNOWN = `
**已核实的引擎与门禁口径（第 11、12 章踩过，直接照用）**：
1. 脚本 \`teleport(map_id)\` 只落该图的 default spawn；\`teleport(map_id, x, y)\` 给坐标，(0,0) 会被当成「没给坐标」。不认 spawn 名。teleport / loadMap 之后**不会**触发脚下的 enter 触发器（引擎只在 tryStep 走一步时触发；map_spec 的 auto 模式没有实现）——剧情入口挂在落点前方的通道触发带上，不压在 spawn 上。
2. portal 与 facility **不读** visible_flag / hidden_flag，只有 NPC 读；门闸只能用 portal / facility 的 require_flag ＋ deny_text_key，或在脚本开头判旗标一句 return。
3. deny_text_key 按规则 20 命名：\`<目标图章号>.block.<目标图短名>\`。
4. 同一格只取对象表里第一个 trigger（不分 mode）；同格叠两个 enter 触发器被规则 16 报「踏不响」——挂点要错开格子。
5. once=true 的触发器必须有 set_flag。
6. 规则 30：同一张图上两个 NPC 不许顶着同一个 role 可能同时在场；只认「一个的 hidden_flag 恰是另一个的 visible_flag」这一种互补，「时段前后不重叠」不算；名牌同名也报。
7. 规则 13：只从 default spawn 出发 BFS，图上所有 npc / facility / portal / interact 触发器都必须走得到——脚本落点不算起点。一张图上被脚本隔开的几个区要拆成几张图。
8. 路径行动 id 必须以 \`_dating[序号]\` 结尾；when 里要写上那个 NPC 的 visible_flag；同一 NPC 两条 inquire 的时段要证明不重叠；until 都含 chNN.done；reveal 的类别必须在那个 role 的 weaknesses 里。
9. 编成：killable_by 不许空数组，且 ⊆ weaknesses；胜负只看气血归零。友军（faction ally）不吃 toughness / weaknesses / killable_by / charge。遭遇表的掉落只许灵草。规则 25 的 BATTLE_EXTRA_MEANS 只认编成里写死的友军，队员（party）不算。
10. kScriptTransfers 五元写法要求那个触发器 type trigger / mode enter / once false / guard_flag == doneFlag / set_flag 空、脚本 0 处 flag.set；挂在 NPC 上或首次置旗的传送走三元写法。一份 Lua 脚本拿不到玩家坐标、也拿不到是哪个触发器起的事件——要按来处分别处理就拆成几份脚本。
11. 配方 successChance 夹在 [5,95]；guaranteed:true（全仓只许一张）才 100。炼器失败折损一半材料。
12. advance_days 单次 ≤ 3650，超了静默夹——长跳时分段。打坐只能从 meditate 设施进；advance_days 每日回一成气血。
13. realm.advance 跨大境界一步到位并顺带抬 realmCap；breakthroughPillBonus 仍恒返回 0；结丹后小瓶 3 日一滴、容量 9。
14. 引擎项编号：契约 E1…En 只放要动 src/ 或要写引擎判据的项；施工图 10.1 其余项用 C / T / B 前缀，开头写一行编号对照。
15. 名字首见：tests/LexiconTests.cpp 按「首见章号 × 章节覆盖到原著第几章」自动判；名牌跟 role 走；物品显示名里的词也算。
16. 每一章「同一件事实」的写法只留一处权威（施工图主表），别处引用它；拍板改了就把所有出现处一起改（第 12 章审查：规则 30、修罗圣火、走偏三件事在四处写成了三种）。
`

// ---------------------------------------------------------------- 摘要
const DIGEST_SCHEMA = {
  type: 'object',
  properties: { file: { type: 'string' }, chapters: { type: 'string' }, hanzi: { type: 'number' }, notes: { type: 'string' } },
  required: ['file', 'chapters', 'hanzi', 'notes'],
}
const digestJobs = [
  ...A.digests.map(([a, b]) => ({ ch: A.ch, a, b, out: `${NOTES}/digest-${a}-${b}.md` })),
  ...((A.nextDigests && A.nextDigests.ranges) || []).map(([a, b]) => ({
    ch: A.nextDigests.ch, a, b,
    out: `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch${String(A.nextDigests.ch).padStart(2, '0')}/digest-${a}-${b}.md`,
  })),
]
phase('摘要')
// 已有摘要（上一章的工作流顺带做好的）：直接用，不再派摘要员
const preDigests = (A.existingDigests || []).map(f => ({ file: f, chapters: '', hanzi: 0, notes: A.existingNotes || '（摘要员冲突备注见同目录 digest-notes.md）' }))
const digests = preDigests.length ? preDigests : (await parallel(digestJobs.map(d => () =>
  agent(`你是原著摘要员。逐章通读《凡人修仙传》第 ${d.a} 到第 ${d.b} 章（共 ${d.b - d.a + 1} 章），写成摘要文件 ${d.out}（UTF-8）。
${COMMON}
${CHUNKED}
摘要格式：每章一节，标题写「## chNNN 章题」。每节包含：1. 事件：按发生顺序 5–12 条，写清谁对谁做了什么、结果如何、在哪里。2. 本章首次出现的人名 / 势力 / 地名 / 物品 / 功法 / 法术 / 妖兽，逐个标「首见」（用 novel.py find 核过的才标首见，否则写「本章提到」）。3. 原文里明确的数字原样抄，带上是谁说的。4. 韩立的境界、法宝、同伴与处境的变化。5. 与游戏玩法可能相关的机制细节（交易、价格、封锁、传送、猎妖、炼制、功法、禁令）。
不抄整段原文（每章至多引一句不超过 25 字的原话）；不写评价；不补原著没写的。总长 6000–11000 汉字。写完用 Python 数一下文件汉字数（U+4E00–U+9FFF）。回 StructuredOutput：file、chapters、hanzi、notes（读到的与 H:/Work/Kys/docs/大纲.md 第 ${d.ch} 章条目或 H:/Work/Kys/docs/lore/乱星海篇-校对.md 明显冲突的事实，一行一条；没有写「无」）。`,
    { label: `digest:${d.a}-${d.b}`, phase: '摘要', model: 'sonnet', effort: 'medium', schema: DIGEST_SCHEMA })))).filter(Boolean)
const mine = digests.filter(d => d.file.includes(`/ch${NN}/`))
log(`摘要：本章 ${mine.length}/${(A.existingDigests || A.digests).length}，下一章 ${digests.length - mine.length}`)
if (mine.length < (A.existingDigests || A.digests).length) throw new Error('本章摘要未齐，停止')

// ---------------------------------------------------------------- 研究
phase('研究')
const READING = `先读这些（按序，节选读）：
1. H:/Work/Kys/docs/大纲.md 第 ${A.outlineLines} 行（第 ${A.ch} 章条目，章节内容的唯一权威）与第 185–245 行（全局硬约束、年龄、境界曲线、改编标注、内容量修订表：全作 52 场剧情战、65–75 张子场景地图、支线 45 条 / 必经 14 条）。
2. 本章原著摘要：${mine.map(d => d.file).join('、')}。摘要是线索，硬约束表里的每一条章号出处都要自己用 novel.py find / read 核过再写。摘要员标出的冲突逐条核：${mine.map(d => d.notes).join(' / ').slice(0, 2500)}
3. H:/Work/Kys/docs/lore/乱星海篇-校对.md 第 163–215 行（地理、势力、落脚点）、第 213–306 行（主线骨架与 5.1 主干）、第 307–352 行（数字设定）、第 352–390 行（查无清单与硬约束）；H:/Work/Kys/docs/lore/人物总表.md 第 ${A.loreRoleLines} 行（本章人物）与第 205–230 行（缺口与备注）。
4. **上一章留给本章的接口**：${A.prevContract} 的「${A.prevContractSection}」一节（grep -n "^## " 找行号，逐行照接，要改先写明）；上一章施工图 ${A.prevDesign} 的第 7 节（系统与章末对照）、第 15 节（验收里的章末断言）、第 16 节（拍板与开放问题，凡是交给下一章的）。**上一章的终档还没生成**，本章第 2 节起点资源按上一章施工图的章末口径写，每个数标「估，待真档核」，并写明哪几项一定要拿真档核。
5. H:/Work/Kys/fanren/docs/handoff.md 第 95–135 行（开新章固定流程、三样齐备）。
6. 引擎现状自己核（grep 签名，不猜）：src/core/rules/Realm.h（结丹档锚点注释第 170–190 行）、Cultivation.h（冲关、breakthroughBlock、StoryCap）、Economy.h（Shop、拍卖只有规则层）、Encounter.h、Crafting.h、Field.h、Bottle.h、Calendar.h；src/core/battle/ 下编成字段；src/game/ 的 ShopScene / CultivationScene / AlchemyScene / FieldScene / WorldScene；src/script 下 ScriptHost（Lua 命令清单）；H:/Work/Kys/fanren/docs/interfaces-p3-script.md；docs/octopath-battle.md 第 5 节；本章相关的已有占位数据（data/battles/b${NN}_*.json 与它们引用的 role，看 note）；tools/validate.py 里 CHAPTER_MEANS / BATTLE_EXTRA_MEANS / MEANS_LAST_CHAPTER 那一段与 tools/validate_selftest.py 的 outside_role / outside_battle 探针（上一章把它挪到本章的某个 role——本章写实那张编成后要再挪到下一章某个只在一张编成里出现的 role，grep data/battles 核唯一性；若本章是最后一章，写明探针怎么处理）。
7. 上一章施工图的口径（学格式）：${A.prevDesign} 第 0 节、3.0 判据与表一、10.1 编号对照、16.1；最后一条施工纪律的编号是 ${A.prevDisciplineLast}。
${A.extraReading || ''}`

const researchReport = await agent(`你是第 ${A.ch} 章「${A.title}」的研究者。你的产出是**一份研究笔记** ${RESEARCH}（UTF-8，600–900 行），后面四位写手只凭这份笔记（加格式模板）各写施工图的一段与契约，所以笔记里的每一张表都是**规范**：id、名字、旗标、章号在笔记里定下来，写手只许照抄。
${COMMON}
${CHUNKED}
${KNOWN}
${READING}

本章要定的事（都要给推荐，并落到对应的表里）：
${A.decisions}

笔记必须含下面这些表（每张表一节，节名照抄，用 Markdown 表格；id 用拼音小写，旗标前缀 ch${NN}.）：
A. 硬约束表（≥ 50 行）：事实 / 原著章号 / 原文片段（≤ 25 字）/ 本章怎么用。每一行的章号都用 novel.py 核过。
B. 大纲、lore、人物总表与占位数据里对不上的地方：出处 / 原著怎么写 / 处置（回写 or 回避）。
C. 境界与数值锚点：本章起止境界（realm 编号与名称）、Realm.h 里相关档的攻防锚点、起点资源（按上一章章末口径估，逐项标「估」）。
D. 引擎现状核查表：模块 / 现有签名（文件与行）/ 本章要它做什么 / 缺什么（规则层 / 加载器 / 游戏层哪一段）/ 建议契约 E 编号（不需要动 src/ 的写「无」）。
E. 拍板表（≥ 25 条）：问题 / 推荐 / 不选推荐会怎样。全部按「已拍板（按推荐，预授权）」口径。
F. 主线节点表（五幕、20–35 个节点）：节点号 / id / 幕 / 原著章 / 地图 id / 挂点对象名 / mode / once / 前置旗标 / done_flag / 一句话框架件（谁在场、做什么、置什么旗标、分支条件——**不写原著镜头顺序、不抄台词**）。
G. 地图表：地图 id / 中文名 / 主题（现有）/ 时辰 / 尺寸量级 / 出入口（目的图、方向）/ 挂点与设施（对象名，错开格子，剧情入口不压 spawn）/ 在场 NPC / default spawn。
H. 角色表：role id / 名牌 / 首见章 / 境界 / 新建还是就地改写 / 出场旗标 / 退场旗标 / 在哪张图。
I. 战斗表：battle id / 节点 / 由头 / 敌方 role 与数量 / 波次 / hero_absent / 友军 / defeat_is_fatal / can_escape / 意图玩家 / 与前章「不冲突」说明 / 玩家此时的我方手段类别（规则 25）/ 破绽与 killable_by / 战前最近的回血设施。
J. 旗标清单：旗标名 / 类型 / 谁置 / 谁读。含 ch${NN}.done；路径行动旗标 ch${NN}.path.<id> 单列。
K. 路径行动表（10–16 条）：id（以 _dating 结尾）/ 图 / npc 对象名 / kind / when（含 NPC 的 visible_flag）/ until（含 ch${NN}.done）/ 效果。
L. 物品 / 法术 / 配方 / 商店表：id / 新建或已有 / 来源章 / 用途 / 数量或价格。
M. 首见章号词表（交 tests/LexiconTests.cpp）：词 / 首见章 / 备注（含物品显示名里的词）。
N. 经济账要点、时长估算要点（按幕）、支线（id / 可选或必经 / 内容）、teleport 清单（三元 / 五元写法各标明）。
O. 下一章接口约定（本章留下什么、下一章怎么用）与开放问题。
写完后回一份不超过 40 行的报告：契约 E1…En 各一行、新图张数、战斗场数、旗标数、路径行动数、支线数、拍板条数、开放问题编号、需要回写大纲 / lore 的条目。`, { label: `research:ch${NN}`, phase: '研究', effort: 'high' })
if (!researchReport) throw new Error('研究代理未返回')

// ---------------------------------------------------------------- 写作
const TEMPLATE = 'H:/Work/Kys/fanren/docs/ch10-design.md'
const WRITER_COMMON = `${COMMON}
${CHUNKED}
${KNOWN}
你唯一的事实来源是研究笔记 ${RESEARCH}（整份读完）；id、旗标、章号、数字一律照抄笔记，不自创、不改名。要写原著事实而笔记没有的，用 novel.py 核过再写并注明章号。格式模板是 ${TEMPLATE}（第 10 章施工图），只读下面指定的行号段落学格式与口径，不要整份读。写法要求：3.2 一类的动作表只写框架件（谁在场、要做什么动作、置什么旗标、分支条件），**不摘原著镜头顺序、不抄原著台词**；13.1 每段写「剧情事实」与「质感边界」。节号与节名照第 10 章。发现笔记内部矛盾或与引擎口径冲突：不要各写各的——在你那一段写成推荐做法，并在报告里列出，供整改时统一。`

const PARTS = [
  { key: 'p1', label: 'write:p1(0-3)', prompt: `你是第 ${A.ch} 章施工图的写手一，写 ${NOTES}/design-p1.md：文档标题行「# 第 ${A.ch} 章设计：${A.title}」、开头引言（原著范围 ch${A.a}–${A.b}、大纲时长 ${A.minutes} min、起止境界、与上一章的衔接、本稿日期 2026-10-13）、第 0 节到第 3 节（0 这一章要做到什么；1 硬约束（1.1、1.2、1.3 境界曲线与上限、1.4 上一章留下的线头）；2 起点资源事实（估）与入口边界；3 主线节点（3.0 三条判据与表一 / 表二 / 表三、主线节点表，3.1 挂点 mode/once（末尾列 teleport 清单，分三元 / 五元），3.2 脚本动作与分支，3.3 日历，3.4 目标链 data/objectives/ch${NN}.json））。目标 360–480 行。
模板行号：${TEMPLATE} 第 1–28 行、29–60 行、178–295 行、297–373 行、419–472 行、472–560 行、556–598 行。` },
  { key: 'p2', label: 'write:p2(4-9)', prompt: `你是第 ${A.ch} 章施工图的写手二，写 ${NOTES}/design-p2.md：第 4 节到第 9 节（4 地图（每张：主题 / 时辰 / 尺寸 / 出入口 / 挂点 / NPC / default spawn）；5 角色；6 支线；7 系统（本章各玩法怎么用现有机制落、门闸一览、起点 / 章末对照）；8 战斗编成与平衡（8.0 剧情战斗怎么数、8.1 意图玩家、8.2 每场编成的设计意图、8.3 由头与「不冲突」、8.4 野外遭遇）；9 经济账）。目标 300–420 行。
模板行号：${TEMPLATE} 第 598–700 行、700–731 行、731–842 行。` },
  { key: 'p3', label: 'write:p3(10-17)', prompt: `你是第 ${A.ch} 章施工图的写手三，写 ${NOTES}/design-p3.md：第 10 节到第 17 节（10 引擎前置（10.1 清单：开头一行编号对照；10.2 判据要点）；11 旗标清单；12 禁词与首见章号（12.1 交协调者的词表，12.2 规则放行、本章另有专项断言的）；13 文案要求与 13.1 每段框架件清单；14 时长估算（按幕，给可压的三处）；15 验收（判据可测、独立于驱动、写明从哪份存档起）；16 拍板记录（16.1 已拍板（按推荐，预授权）表、16.2 与旧拍板的冲突与开放问题、16.3 路径行动条目表、16.4 拍板前的原表）；17 施工纪律（编号接 ${A.prevDisciplineLast} 之后）；最后一行写「## 18. 施工偏差（施工阶段填）」）。目标 320–440 行。
模板行号：${TEMPLATE} 第 842–874 行、874–1000 行、1000–1089 行、1089–1124 行、1124–1243 行、1243–1256 行。` },
  { key: 'contract', label: 'write:contract', prompt: `你是第 ${A.ch} 章的契约写手，写 ${CONTRACT}（目标 220–360 行）：标题「# P3 第 ${A.ch} 章引擎增补契约」；0 各引擎项现状；每个引擎项 E1…En 一节（一句话口径；规则层 / 加载器 / 游戏层各改哪个文件、签名与字段；数据形状；判据——tests/Ch${NN}EngineTests.cpp 要钉什么，判据不经驱动直接读数据，每条新检查配负向；实现记录留空）——若本章不需要动 src/，写「本章 0 个代码项」并把验证项照第 11 章契约 E2–E5 的写法列；下一章接口约定；脚本与数据口径；门禁与既有测试的依赖（CHAPTER_MEANS[${A.ch}] 与 BATTLE_EXTRA_MEANS、MEANS_LAST_CHAPTER=${A.ch}、selftest 探针、LexiconTests 词表、flags.json、chapters.json 与 ui.json 章节卡、kScriptTransfers / kOffChainDoorKeys、kOffChainFlags 与 kLastChapter、PathActionTests 按章上限、KeyPrompts 白名单；依赖表：依赖 / 现状 / 改法 / 谁）；白名单（引擎路 / 内容路 A 地图美术 / 内容路 B 数据 / 内容路 C 脚本文案 / 协调者 / 测试路，按文件列，两两不重叠）；环境与纪律（构建槽 ch${NN}eng / ch${NN}content / ch${NN}test、PowerShell 调 build_logged.bat、门禁先于编译、旧图不 patch、旧脚本只上一章章末脚本补一行 teleport）。
格式模板：H:/Work/Kys/fanren/docs/interfaces-p3-ch11.md 第 1–40 行与第 6–10 节（grep -n "^## " 找行号）。` },
]
phase('写作')
const partReports = (await parallel(PARTS.map(p => () =>
  agent(`${p.prompt}\n${WRITER_COMMON}`, { label: p.label, phase: '写作', effort: 'high' }).then(r => ({ key: p.key, report: r }))))).filter(Boolean)
const missing = PARTS.filter(p => !partReports.some(r => r.key === p.key))
if (missing.length) throw new Error('写作未齐：' + missing.map(p => p.key).join(','))

phase('组装')
const assembleReport = await agent(`把三段施工图拼成一份：用 Python 把 ${NOTES}/design-p1.md、design-p2.md、design-p3.md 按序拼接（p2、p3 若各自带了文档标题行「# 第 ${A.ch} 章…」就去掉，只保留 p1 的），写到 ${DESIGN}（UTF-8，LF，无 BOM）。然后用 Python 检查：顶层节「## N.」从 0 到 18 一个不缺、不重复、次序递增；没有残留的「<!-- 续写点 -->」；统计行数与汉字数。缺节或重复就在回复里列出来（不要自己补写内容）。
${COMMON}
回复不超过 15 行。`, { label: 'assemble', phase: '组装', effort: 'low' })

// ---------------------------------------------------------------- 核对
const FINDINGS_SCHEMA = {
  type: 'object',
  properties: {
    findings: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          severity: { type: 'string', enum: ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'] },
          doc: { type: 'string' }, section: { type: 'string' }, claim: { type: 'string' },
          problem: { type: 'string' }, evidence: { type: 'string' }, fix: { type: 'string' },
        },
        required: ['severity', 'doc', 'section', 'claim', 'problem', 'evidence', 'fix'],
      },
    },
    checked: { type: 'number' },
  },
  required: ['findings', 'checked'],
}
phase('核对')
const LENSES = [
  { key: 'novel', prompt: `你是第 ${A.ch} 章施工图的**原著核对**审查者，只管文档里写的原著事实对不对。读 ${DESIGN}（先 grep "^#"；重点读 1.1 硬约束表、1.2、第 3 节表一/二/三与节点表、第 5 节角色、第 7 节、第 12 节首见章号表、16.3）与 ${CONTRACT} 的下一章接口一节。对每一条带章号出处的事实用 novel.py 到原文核：章号、有没有写反写多、名字首见章有没有提前。专门查「银月」「紫罗极火」「黄龙」「猎妖令」与本章大纲「注意」栏点名的事。至少核 50 条，只报有问题的，每条给原文证据（章号 + ≤ 25 字片段）。` },
  { key: 'engine', prompt: `你是第 ${A.ch} 章契约的**引擎可行性**审查者。读 ${CONTRACT} 全文与 ${DESIGN} 第 4、7、8、10、11 节。逐条核：每个「已有」说法在 H:/Work/Kys/fanren/src 下 grep 属实；新接口不与现有签名冲突、规则层 → 加载器 → 游戏层齐备、判据能直接读数据；旗标名与 data/flags.json 现有条目不撞；上一章契约「下一章接口」一节（${A.prevContract}）逐条有交代；白名单两两不重叠、共享文件归唯一一路；CHAPTER_MEANS[${A.ch}] 与本章我方手段对得上、selftest 探针挪去的 role 唯一；编成字段是加载器认的；地图命名与生成器写法、规则 13 / 16 / 20 / 29 / 30 对得上，主题与时辰在 data/visual/maps.json 里已有；第 11、12 章踩过的坑（落点不触发 enter、portal 不读 hidden_flag、同格叠触发器、脚本拿不到坐标）有没有再犯。至少核 35 条，只报有问题的，每条给文件与行号。` },
  { key: 'process', prompt: `你是第 ${A.ch} 章施工图的**流程合规**审查者。先读 H:/Work/Kys/fanren/docs/handoff.md 第 95–135 行与第 150–215 行；${A.prevDesign} 第 16、17 节。再读 ${DESIGN} 全文（按节）与 ${CONTRACT}。逐项核：3.2 只写框架件、13.1 齐全；每处付出都有退路、门闸只靠时间或有兜底、每场必打仗之前玩家回得了血；16.1 写明「已拍板（按推荐，预授权）」、没推翻第 6 章起拍过的板；战斗场数在全作 52 场的剩余预算内并写意图玩家；目标链与门闸对得上、teleport 清单全、路径行动形状合规、NPC 一人一时一地；时长 ≈ ${A.minutes} min 且给可压三处；支线配额（全作 45 / 必经 14）这章分担多少；章节卡不剧透；禁词词表带章号；{key.<id>} / .pad 占位规则；验收判据可测、不从被测物推导、起点写明是上一章真档。至少核 35 条，只报有问题的，给出处与建议。` },
  { key: 'consistency', prompt: `你是第 ${A.ch} 章施工图的**一致性**审查者。读 ${DESIGN} 全文、${CONTRACT} 全文，以研究笔记 ${RESEARCH} 的各表为基准，核：节点表与 3.1 / 3.2 / 3.4 / 第 11 节 / 第 15 节逐一对应；地图 id 在各节与契约白名单一致、出入口两头互指、default spawn 与 teleport 落点一致；role id 在第 5、8 节、16.3、第 11 节一致；战斗 id 在 3.2、第 8 节、契约 BATTLE_EXTRA_MEANS 一致；经济账数字与第 2、7 节、契约同一组；契约 E 编号与第 10 节对照一致；16.1 每条的落地处真有；同一件事实在几处写法是否相同（不许一处一个说法）。至少核 35 条，只报不一致的，写明两处各自怎么写、以哪个为准。` },
]
const lensResults = (await parallel(LENSES.map(l => () =>
  agent(`${l.prompt}\n${COMMON}\n${KNOWN}`, { label: `verify:${l.key}`, phase: '核对', effort: 'high', schema: FINDINGS_SCHEMA })
    .then(r => ({ key: l.key, ...r }))))).filter(Boolean)
const allFindings = lensResults.flatMap(r => r.findings.map(f => ({ lens: r.key, ...f })))
const counts = allFindings.reduce((m, f) => ({ ...m, [f.severity]: (m[f.severity] || 0) + 1 }), {})
log(`核对：${lensResults.map(r => `${r.key} 核 ${r.checked}`).join('、')}；问题 ${allFindings.length} ${JSON.stringify(counts)}`)

phase('整改')
let fixReport = '（无问题，未整改）'
if (allFindings.length > 0) {
  fixReport = await agent(`你是第 ${A.ch} 章施工图的整改者。四路独立审查对 ${DESIGN} 与 ${CONTRACT} 提出了下面这些问题（JSON）。逐条处理：成立的改到位（用 Edit 局部替换，每次 ≤ 120 行）；不成立的写明理由与证据。改原著事实时自己再用 novel.py 核一次。CRITICAL / HIGH 必须处理，MEDIUM 尽量，LOW 酌情。同一件事实改了就 grep 全文把所有出现处一起改；研究笔记 ${RESEARCH} 一并改，保持它仍是规范。整改中新定的板写进 16.1（接着编号），与旧条冲突处写明「以后者为准」。
${COMMON}
${KNOWN}
问题清单：
${JSON.stringify(allFindings, null, 1)}
回报告（≤ 60 行）：每条一行（编号 / 严重度 / 改了什么或为何不改）；末尾写两份文档的行数与需要协调者知道的事。`, { label: `fix:ch${NN}-design`, phase: '整改', effort: 'high' })
}

return {
  digests: digests.map(d => ({ file: d.file, hanzi: d.hanzi, notes: d.notes })),
  researchReport,
  partReports: partReports.map(r => ({ key: r.key, report: String(r.report).slice(0, 1500) })),
  assembleReport,
  findingsCount: counts,
  highs: allFindings.filter(f => f.severity === 'CRITICAL' || f.severity === 'HIGH').map(f => ({ lens: f.lens, section: f.section, problem: f.problem })),
  fixReport,
}
