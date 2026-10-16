export const meta = {
  name: 'ch11-verify',
  description: '第 11 章测试路（登记类与静态用例 → 通关与平衡）→ 独立三路审查 → 整改 → 复验，循环到通过',
  phases: [
    { title: '测试', detail: '测试路 A：旧用例登记、ch10-end 重生成、静态用例；测试路 B：通关、平衡、ch11-end' },
    { title: '审查', detail: '内容 / 测试质量 / 引擎与门禁 三路独立审查' },
    { title: '整改', detail: '按审查裁决整改并重跑全量' },
    { title: '复验', detail: '原审查路复验，循环到通过' },
  ],
}

const NOTES = 'C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch11'
const DESIGN = 'H:/Work/Kys/fanren/docs/ch11-design.md'
const CONTRACT = 'H:/Work/Kys/fanren/docs/interfaces-p3-ch11.md'
const COMMON = `
环境纪律（必须遵守）：
- 仓库 H:/Work/Kys，游戏工程 H:/Work/Kys/fanren（下文相对路径都相对它）。施工图 ${DESIGN} 与契约 ${CONTRACT} 是第 11 章的规范（id、旗标、数字照抄）；施工图第 18 节是施工偏差，读它了解落地与设计的出入。
- 原著事实只能用 \`PYTHONIOENCODING=utf-8 python H:/Work/Kys/tools/novel/novel.py {find|read}\` 核。
- 终端代码页是 GBK，中文显示乱码是正常的；判断编码一律 Python 读字节。写含中文或反斜杠的文件用 Write / Edit 或 Python，不用 bash heredoc，不把中文写进 python -c。源码与数据 UTF-8 无 BOM、LF。每一次 Write / Edit 不超过 180 行（大文件分块写）。
- 不派子代理；不整份重读超过 1500 行的文件（先 grep 标题行再按行号读）；不碰 git；只改自己白名单里的文件，白名单外的问题写进报告。
- 另一个工作流在同一棵树里写 docs/ch12-design.md 与 docs/interfaces-p3-ch12.md——那两份与第 12 章笔记不归你，别碰，也别把它们当第 11 章的规范。
- 构建一律用 **PowerShell 工具** 调 \`& "H:/Work/Kys/fanren/build_logged.bat" <槽名>\`（bash 里 cmd.exe /c 会静默假绿），日志 build-<槽名>.log 按 gb18030 读字节、读前核 mtime；全量约 10–15 分钟。只跑部分测试时用 \`build-<槽名>/fanren_tests.exe --gtest_filter=...\`（工作目录设为 H:/Work/Kys/fanren）。
- **最终回复就是你的报告**：不要把构建 / 测试丢到后台然后结束回合；必须等它结束、读完结果再写报告。报告精炼，不贴文件内容。
- 判据纪律（docs/handoff.md 第 150–215 行教训，写测试前读一遍）：判据不许从被测物推导（设计写死的取值直接写字面量）；每条新检查配一条「故意写坏 → 确实被抓住」的负向；通关测试从真实上一章终档起步、不手搭起点；只钉一个数据点、比值分母为 0、找不到就算过——这几种空转都要堵。
`

// ---------------------------------------------------------------- 测试
phase('测试')
const testA = await agent(`你是第 11 章的**测试路 A**：既有用例的登记类改动、第 10 章终档重生成、第 11 章的静态（不走通关）用例。构建槽 **ch11test**。
${COMMON}
白名单：tests/Ch10AcceptanceTests.cpp（只改章节卡断言：第 11 章进表后 ch10.done 排的是 Closing(10) 接 Opening(11)，见 ${CONTRACT} 第 8.3 节）、tests/UiStyleTests.cpp（只改 ChapterCards / ChapterTableData 里「第 10 章是最后一章」的那几处）、tests/fixtures/ch10-end-{first,second}.sav（重生成）、tests/ObjectiveTests.cpp（kScriptTransfers：第 10 章那一行落点改 ch11_fangshi，加本章三元行与回城五元行）、tests/NpcPresenceTests.cpp（kOffChainFlags 八条、kLastChapter 改 11、以及群像 / 同 role 的白名单若需要）、tests/PathActionTests.cpp（realmCapOfChapter 加 {11, CoreEarly}）、tests/ChapterFixture.h（加 kChapterElevenEndingFirst / Second 与 kWriteChapterElevenFixturesEnv="FANREN_WRITE_CH11_FIXTURES"，照第 10 章三行）、tests/Ch03BattleKitTests.cpp（只改第 52 行一带那句注释）、tests/BattleRewardTests.cpp（只在它因妖丹掉落红时按剧情战 / 遭遇战分开判）、tests/Ch07BattleDataTests.cpp（只改第 318 行一带 MEANS_LAST_CHAPTER 的放行清单，加 11）、tests/MapLinkTests.cpp（只改 MapLinkWalk 对**同图门**的判法：ch11_fangshi 茶馆雅间是一对同图 portal（portal_chaguan → spawn yajian、portal_chaguan_chu → spawn chaguan_men），现判据「进门后按住方向、当前图仍是出发图即判弹回」对同图门必红；改成对同图门看「落点按住进门方向后是否又踩上任何 portal 被送回 / 位置回到门格」，并配一条负向：临时副本里造一对会回弹的同图门，必须被抓住——不许为了绿而对同图门整个跳过）、新文件 tests/Ch11Data.h、tests/Ch11TriggerModeTests.cpp、tests/Ch11BattleDataTests.cpp、tests/Ch11LedgerTests.cpp（只做静态部分：商店、物品、配方、脚本扫描），saves/README.md。
先读：${CONTRACT} 第 273–309 行（第 8 节依赖表）、310–349 行（白名单）；${DESIGN} 第 15 节验收（grep -n "^## 15" 后读到 ## 16）、3.1 节（挂点表与 teleport 清单）、第 11 节（旗标）、第 18 节（施工偏差，含集成记录）。模板：tests/Ch10TriggerModeTests.cpp、Ch10BattleDataTests.cpp、Ch10LedgerTests.cpp、Ch10Data.h、Ch10AcceptanceTests.cpp、tests/ChapterFixture.h 第 200–260 行。
步骤：
1. 集成者的终证（build-integ11.log）是 1808 条里 13 红，全部归本路：Ch10Walkthrough×2、Ch10Acceptance、Ch10Slice（ch10-end 待重生成）；Ch10Acceptance 章节卡改 Opening(11)；ObjectiveRoute×3（kScriptTransfers 七条）；kOffChainFlags 八条 + kLastChapter=11；PathActionTests realmCapOfChapter 加 {11, CoreEarly}；ChapterTableData / ChapterCards×2；Ch07BattleDataTests:318；MapLinkWalk（同图门）。集成记录在 ${NOTES}/integration.md 与施工图第 18.4 节（jiedan_linju 拆成 _jia / _yi、何仙师只在青竹小轩、范夫人雅间 hidden 改 ch11.zhuzhan 等，写用例时以落地为准）。先跑一次 \`& "H:/Work/Kys/fanren/build_logged.bat" ch11test\` 确认这份清单。
2. 改登记类用例；按 ChapterFixture.h 的说明重生成第 10 章终档：设环境变量 FANREN_WRITE_CH10_FIXTURES=1 只跑 Ch10Walkthrough.* 两条，再清掉变量跑一遍不带写开关的 Ch10*（必须绿）。核新终档：mapId == ch11_fangshi、站在 default spawn chengmen、ch10.done == 1，其余字段与旧终档比只差地图与坐标（用 Python 比 JSON payload，列出差异字段）。
3. 写静态用例：验收第 3、4、12、14、15、16、17、19、20、21（只做读数据 / 扫脚本 / 读地图那一半）条；每条配负向（在临时副本里改坏 → 必须红）。文件头写明对应的验收条号。
4. 全量构建到全绿（零警告）；剩下的红若是内容数据不符合施工图（不是你的判据写错），写进报告「交整改」，不改数据。
报告（≤ 60 行）：各文件改动、新增用例数（正向 / 负向）、ch10-end 新旧差异字段、终证日志名与 EXITCODE、tests passed 那一行、警告数、交整改清单。`, { label: 'test:A-registry-static', phase: '测试', effort: 'high' })
if (!testA) throw new Error('测试路 A 未返回')
log('测试路 A：' + String(testA).slice(0, 200))

const testB = await agent(`你是第 11 章的**测试路 B**：通关测试、平衡（BattleHand 量四场）、第 11 章终档。构建槽 **ch11walk**。
${COMMON}
测试路 A 已交付（报告附后），第 10 章终档已按开局 patch 重生成。
白名单：新文件 tests/Ch11Driver.h、tests/Ch11AcceptanceTests.cpp（两侧通关 Ch11Walkthrough + 终点断言 + 写终档）、tests/Ch11SliceTests.cpp（验收第 5–11、13 条的切片）、tests/fixtures/ch11-end-{first,second}.sav（新，由通关以 FANREN_WRITE_CH11_FIXTURES=1 生成）；**平衡**：data/battles/b11_*.json 四张与 data/roles/{qianmu_yao,jiguan_jiao,yinsha_shayao,tiandu_shi,xueyu_zhizhu_ally,kuilei_yuan}.json 的战斗数值（气血、攻防、架势、行动、蓄势倍率——不改破绽、不改 killable_by、不改编成人数与友军构成，那是施工图拍过的）；src/core/rules/Realm.h 只改第 175 行一带那句「筑基后期与结丹档仍待对应章节标定」注释（B1：结丹初期由第 11 章标定）；${NOTES}/balance.md（平衡记录，集成时并进施工图第 18 节）。
先读：${DESIGN} 第 15 节（验收，尤其第 2 条：意图玩家写死的选项、起点先验、终点先验与 day 差 33375 等字面量）、3.2 节（脚本动作与分支）、3.3 节（日历）、第 8 节（四场的意图玩家与平衡目标：① 意图三到五回合、② 三回合内逃得出、③ 四回合内清场、④ 只靠友军两技能也能把它气血打到 0）、第 18 节（偏差）；模板 tests/Ch10Driver.h（整份，驱动怎么走图、开设施、打仗、记账）、tests/Ch10AcceptanceTests.cpp、tests/Ch10SliceTests.cpp、tests/BattleHand.h（那只手；先证明它自己稳：同一存档三次跑结局相同）、docs/ch07-balance.md（平衡记录的写法）。
要求：
- 通关从 tests/fixtures/ch10-end-{first,second}.sav 起步（不手搭起点），两侧走施工图验收第 2 条写死的同一组选项；驱动走真实入口（WorldScene 走图、interact、openFacility → CultivationScene / FieldScene / AlchemyScene 的真实 API、BattleScene 真打），不直接改 state 注入结果。终点断言照验收第 2 条字面量写死（不从驱动推导）；把 day 差、灵石、物品逐项写进断言。
- 平衡：用 BattleHand 量四场（两侧起点、意图手），先证手稳；不达标就调白名单内的数值，记进 balance.md（调前 / 调后、回合数、剩余气血）。四场都要胜（② 是逃得出）。
- 通关绿之后，以 FANREN_WRITE_CH11_FIXTURES=1 写终档，再清掉变量重跑一遍比对（NOWRITE 必须绿），用 Python 读两份终档列出关键字段（day、realm / cap / former、HP / MP、灵石低 / 中、关键物品、party、mapId 与坐标）。
- 切片按验收各条写，每条写明起点存档；每条配负向。
- 最后全量构建到全绿（零警告）。
测试路 A 报告：
\${TEST_A}
报告（≤ 70 行）：通关两侧四场的回合数与胜负、平衡调了什么、终档关键字段表、新增用例数、终证日志名与 EXITCODE、tests passed 那一行、警告数、交整改清单（内容与施工图不符、你没改的）。`.replace('${TEST_A}', String(testA).slice(0, 3000)), { label: 'test:B-walkthrough-balance', phase: '测试', effort: 'high' })
if (!testB) throw new Error('测试路 B 未返回')
log('测试路 B：' + String(testB).slice(0, 200))

// ---------------------------------------------------------------- 审查
const VERDICT_SCHEMA = {
  type: 'object',
  properties: {
    score: { type: 'number', description: '内容审查给 1–5 分（0.5 一档）；测试 / 引擎审查给 5=Approve、3=RequestChanges' },
    verdict: { type: 'string', enum: ['PASS', 'FAIL'] },
    reportFile: { type: 'string' },
    findings: {
      type: 'array',
      items: {
        type: 'object',
        properties: {
          id: { type: 'string' },
          severity: { type: 'string', enum: ['CRITICAL', 'HIGH', 'MEDIUM', 'LOW'] },
          where: { type: 'string' },
          problem: { type: 'string' },
          evidence: { type: 'string' },
          fix: { type: 'string' },
        },
        required: ['id', 'severity', 'where', 'problem', 'evidence', 'fix'],
      },
    },
  },
  required: ['score', 'verdict', 'reportFile', 'findings'],
}

const REVIEWS = [
  {
    key: 'content', file: 'H:/Work/Kys/fanren/docs/ch11-review.md', pass: '分数 ≥ 4.0 且无 CRITICAL / HIGH',
    prompt: `你是第 11 章的**独立内容审查**（原著、文学质量、在场与名字、玩家够得着）。你没有参与施工，只审不改：唯一能写的文件是你的报告 H:/Work/Kys/fanren/docs/ch11-review.md。
先读前几章的审查当口径：H:/Work/Kys/fanren/docs/ch10-review.md 前 80 行（结构与评分口径）、docs/ch06-reverify.md 前 60 行（「换词不换骨」的判法）。
审：
1. 原著：施工图第 1.1 节硬约束逐条（至少抽 40 条）对照落地文案与数据，用 novel.py 读原文核；名字首见章（tests/LexiconTests.cpp 表一与施工图 12.1 / 12.2）在文案里是否被提前；「猎妖令」「紫罗极火」「银月」「黄龙」等禁词 0 处。
2. 原创性：逐段把 data/text/ch11_*.json 的主线文案与对应原著章节并排读（至少 15 段，按施工图 13.1 的节点挑），判「换词不换骨」（照原著镜头顺序逐句换词、或把原著动作反着说也算），给出处数与占比。
3. 文学质量：13.1 每段的「剧情事实」是否到位、「质感边界」是否守住；韩立的口吻、对白节奏、每段长度；有没有说明书腔、重复句式、现代词。
4. 玩家够得着：每个门闸与退路（施工图第 7 节门闸一览、契约第 7 节）在脚本里是否真有走得通的一支；NPC 一人一时一地、出场退场旗标首尾相接；目标链次序与门闸。
5. 用 Python 统计第 11 章文案条数、汉字数、最长一句、同一句式重复次数。
6. 第 12 章研究者（读过 ch416–499 原著）提出的一条：第 11 章文案里「那半幅虚天残图」的「半幅」与原著不合（第 12 章入殿凭的是第 8 章给的锦帕苏醒，见 C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/notes/ch12/research.md 里 O-12 与 A 表相关行）——用 novel.py 核原著第 11 章区间里乌丑 / 隐煞门对残图的说法，判这一句该怎么改，列进问题清单。
报告写进 ch11-review.md（结构：结论与分数 / 问题清单（编号 C-1…，严重度、位置、证据、改法）/ 抽查记录 / 统计）。评分口径：5 优秀；4 可交付、只剩 LOW；3 有 HIGH 需整改；≤2.5 大面积重写。PASS = ${'分数 ≥ 4.0 且无 CRITICAL / HIGH'}。`,
  },
  {
    key: 'tests', file: 'H:/Work/Kys/fanren/docs/ch11-test-review.md', pass: 'Approve（无 CRITICAL / HIGH）',
    prompt: `你是第 11 章的**独立测试质量审查**。你没有写这些测试，只审不改：唯一能写的文件是你的报告 H:/Work/Kys/fanren/docs/ch11-test-review.md。
先读 H:/Work/Kys/fanren/docs/handoff.md 第 150–215 行（判据会说谎的几种长相）与 docs/ch10-test-review.md 前 60 行（口径）。
审 tests/Ch11*.cpp、tests/Ch11*.h、以及本章改过的旧用例（git diff 看：git -C H:/Work/Kys diff --stat -- fanren/tests；只读，不做别的 git 操作）：
1. 通关是否从真实第 10 章终档起步、走真实入口；终点断言是否是施工图写死的字面量而不是驱动算出来的；两侧是否真不同。
2. 每条新检查是否有负向；有没有空转（找不到就算过、分母为 0、只钉一个点、判据从被测物推导）。
3. 施工图第 15 节 26 条验收逐条：有对应用例吗、用例真钉住了吗。列覆盖表。
4. 第 10 章终档重生成：新旧只差地图与坐标吗（用 Python 读两版：旧版用 git -C H:/Work/Kys show HEAD:fanren/tests/fixtures/ch10-end-first.sav）。
5. 自己跑一次相关测试确认（PowerShell：H:/Work/Kys/fanren/build-ch11walk/fanren_tests.exe --gtest_filter=Ch11*:Ch10*，工作目录 H:/Work/Kys/fanren；若该 exe 不存在就用 build-ch11test 的），记录结果。
报告结构：结论（Approve / RequestChanges）/ 问题清单（T-1…）/ 覆盖表 / 实跑记录。PASS = Approve。score 填 5（Approve）或 3（RequestChanges）。`,
  },
  {
    key: 'gate', file: 'H:/Work/Kys/fanren/docs/ch11-gate-review.md', pass: 'Approve（无 CRITICAL / HIGH）',
    prompt: `你是第 11 章的**独立引擎与门禁审查**（C++ 与工具）。只审不改：唯一能写的文件是你的报告 H:/Work/Kys/fanren/docs/ch11-gate-review.md。
审：src/core/rules/Crafting.h / .cpp、src/io/RecipeLoader.h / .cpp 的 E1 改动（git -C H:/Work/Kys diff -- fanren/src 看，只读）对照契约第 1 节；tests/Ch11EngineTests.cpp 是否钉住契约 1.5、2.2、3.2、4.2、5.2；tools/validate.py 的 CHAPTER_MEANS[11] / BATTLE_EXTRA_MEANS / MEANS_LAST_CHAPTER 与本章四场编成、我方实际手段对不对（逐场核：那一样破绽玩家此时真打得到吗，法术 needRealm / needMp、友军 maxMp）；tools/validate_selftest.py 探针挪到 wu_chou + b12_neidian_duoding 是否有效（自己在临时副本里做阴性对照：把 b11 某场的唯一可打破绽改掉，validate 必须报）；内存安全、错误处理、加载器拒收坏形状。跑 \`python tools/validate.py\`、\`python tools/validate_selftest.py\` 记录输出。
报告结构：结论（Approve / RequestChanges）/ 问题清单（G-1…）/ 阴性对照记录。PASS = Approve。score 填 5 或 3。`,
  },
]

let round = 0
let current = REVIEWS.map(r => ({ ...r, result: null }))
const history = []
while (round < 3) {
  round += 1
  phase(round === 1 ? '审查' : '复验')
  const pending = current.filter(r => !(r.result && r.result.verdict === 'PASS'))
  const results = await parallel(pending.map(r => () =>
    agent(`${r.prompt}
${COMMON}
${round > 1 ? `这是第 ${round} 轮**复验**：你上一轮的报告在 ${r.file}，整改方的回应附后。逐条复核你上一轮的问题是否真的关了（读改后的文件、重跑相关测试），再看整改有没有带出新问题；在报告末尾**追加**「第 ${round} 轮复验」一节（不删上一轮的内容），给新的分数与结论。findings 只列仍未关的与新发现的。
整改方回应：
${String(history[history.length - 1] && history[history.length - 1].fix || '').slice(0, 4000)}` : ''}
测试路交付摘要：
A：${String(testA).slice(0, 1500)}
B：${String(testB).slice(0, 2500)}`,
      { label: `review:${r.key}#${round}`, phase: round === 1 ? '审查' : '复验', effort: 'high', schema: VERDICT_SCHEMA })
      .then(res => ({ key: r.key, res }))))
  for (const { key, res } of results.filter(Boolean)) {
    const target = current.find(c => c.key === key)
    if (target && res) target.result = res
  }
  const summary = current.map(c => `${c.key}:${c.result ? `${c.result.verdict}/${c.result.score}/${c.result.findings.length}` : '无结果'}`).join('、')
  log(`第 ${round} 轮审查：${summary}`)
  const failing = current.filter(c => !(c.result && c.result.verdict === 'PASS'))
  if (failing.length === 0) { history.push({ round, summary, fix: null }); break }
  if (round >= 3) { history.push({ round, summary, fix: null }); break }

  phase('整改')
  const findings = failing.flatMap(c => (c.result ? c.result.findings : []).map(f => ({ review: c.key, ...f })))
  const fix = await agent(`你是第 11 章的**整改路**。三路独立审查里未通过的那几路提出了下面这些问题（JSON；报告全文在 docs/ch11-review.md、docs/ch11-test-review.md、docs/ch11-gate-review.md）。逐条整改：成立的改到位；不成立的写明理由与证据（原著章号、源码行、实跑输出）。构建槽 **ch11fix**。
${COMMON}
白名单：第 11 章的内容文件（scripts/ch11/**、data/text/ch11_*.json、data/**/ 第 11 章新建或改写的 role / item / battle / shop / quest / pathaction / objective、tools/mapgen/genmaps_ch11.py 与它生成的 maps/ch11_*.tmj、data/visual/*.json 的 ch11 条目与 assets/art/** 产物）、tests/Ch11*.cpp / .h、本章改过的旧用例、tests/fixtures/ch11-end-*.sav（通关改了就以写开关重生成再 NOWRITE 复核）、src/core/rules/Crafting.* 与 src/io/RecipeLoader.*（只为审查点名的问题）、tools/validate.py 与 validate_selftest.py 的第 11 章那几段、施工图 ${DESIGN} 第 18 节（追加「整改记录」小节）。**不许改审查报告**；不许为了让用例绿而放宽判据（判据错了要给证据）。
文案整改的口径：照施工图 13.1 的剧情事实写，自己的句子，不照原著镜头顺序；改完用 Python 扫禁词与首见次序。改了地图生成器或 data/visual 就重跑 genmaps 与 artgen 并 --check。
问题清单：
${JSON.stringify(findings, null, 1)}
做完跑四门与全量构建到全绿（零警告）。报告（≤ 70 行）：逐条（编号 / 改了什么或为何不改 / 证据）、终证日志名与 EXITCODE、tests passed 那一行。`, { label: `fix#${round}`, phase: '整改', effort: 'high' })
  history.push({ round, summary, fix })
}

return {
  testA: String(testA).slice(0, 3000),
  testB: String(testB).slice(0, 4000),
  final: current.map(c => ({ key: c.key, verdict: c.result && c.result.verdict, score: c.result && c.result.score, open: c.result ? c.result.findings : null, file: c.file })),
  history: history.map(h => ({ round: h.round, summary: h.summary, fix: h.fix ? String(h.fix).slice(0, 3000) : null })),
}
