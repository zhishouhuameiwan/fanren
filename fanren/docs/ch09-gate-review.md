# 第 9 章共享门禁独立审查

最终结论（2026-10-09，SOURCE_FROZEN 定向复验）：**Approve**。原 2 项 HIGH 与 1 项 MEDIUM 均已关闭；本次限定在这三项及邻接 `path_window_can_open`，未发现 remaining findings。冻结工具哈希、独立复现及完整日志核读见文末“返修最终复验”。这不是第 9 章内容或整章交付的批准。

首审结论（返修前，以下保留历史证据）：**RequestChanges**。当时发现 2 项 HIGH 和 1 项 MEDIUM，错误放行由真实 `validate.main()` 的内存反例和 C++ 规则独立复现。

## 首审 Findings（返修后已关闭）

### [HIGH] PathAction 来源只记名字，放行无法达到的旗标值

File: `fanren/tools/validate.py:2148`；Quest 消费端 `:1608`。

Issue: 闭包将 `when` 中的非零条件降为 `c[1] in available`，丢掉 `op/value`。PathAction 的 done_flag 和 set_flags 都由 SetFlag 置为 **1**，重复打探也不会累加；脚本置路径旗标则被既有跨条目规则禁止。因此只由 PathAction 提供的旗标不可能满足 `>= 2` 或 `== 2`，却会被闭包当作可执行的前置来源，Quest 自身的同类谓词也被允许。

独立复现使用 WT9 真实 `ch09.json` 和 `q09_baichi.json`，A/B 分别为 `baichi_sanxiu_a_dating` / `baichi_sanxiu_b_dating`：

- 只给 A.when 增加 `B.done_flag >= 2`，真实 main 输出 0 错、退出 0、`VALIDATE_OK`，依赖 A 完成旗的 Quest 也不报错。
- 只把真实 Quest 的 `complete[0].value` 从 1 改为 2，真实 main 同样退出 0。
- C++ 从真实 `loadGameData` 的 `pathActions` 加载条目，执行 B 的真实效果后 done_flag 为 1，再次打探无 SetFlag；A 在 B=0/1 时均不显示，Quest 要求 A>=2 时也不成立。

Fix: 在路径来源闭包及 Quest 谓词检查中保留已知 producer 的可写值。对只有路径 producer 的旗标，拒绝已知不可能的比较并排除它的下游来源；不要用单纯的名字集合代替值可达性。补齐 `>= 2` / `== 2` 的负例以及 `>= 1` / `== 1` 的正例。这不要求展开 Lua 控制流，只需尊重 PathAction 已确定的置 1 语义。

### [HIGH] 来源闭包忽略 until，接纳永远关闭的行动窗口

File: `fanren/tools/validate.py:2148`。

Issue: 闭包只读 `record["when"]`，没有使用 `record["until"]`。在场与跨条目时段检查没有证明单条行动自身的窗口可开启。于是一个 when 成立就必然被 until 关闭的条目仍向 Quest 提供来源，依赖它的链也会进入 sources。

独立复现：在同一真实 A 上，分别向 when 和 until 增加 `B.done_flag >= 1`，其他字段不改。真实 main 退出 0、`VALIDATE_OK`。运行时 `pathWindowOpen` 要求 when 全成立且 until 无一成立：B=0 时前置不成立，B>=1 时窗口关闭。C++ 实跑 B=0/1/2/99 均不显示 A；同一谓词同时打开和关闭窗口使该条目在所有状态下均不可执行。

Fix: 在收集 producer 前拒绝可证明为空的自身窗口，至少覆盖 when 蕴含任一 until、when 内部互斥这类确定死窗；保留旧跨条目重叠、NPC 在场、章节公理检查。非法窗口的完成旗及附带旗也不得为下游提供来源。补反例及真实可开启链的正向对照。

### [MEDIUM] M2 把永不成立的切磋完成谓词当作合法正例

File: `fanren/tools/validate_selftest.py:1380`；继承值来自 `:1089`。

Issue: `good_quest()` 的第二步谓词是 `ch04.kailu == 2`。M2 只将 flag 改成 challenge.done_flag，留下 `== 2`。胜利切磋只置 1，故该所谓合法任务包含一个永不成立的步骤。`:1394` 的四个正例及随后的链正例共用一次 `source_errors` 检查，也没有验证真实效果后的谓词值。这会把 HIGH 第一项的错误放行计为 PASS。

Fix: 正例将整条谓词明确改为真实胜利可满足的 `>= 1` 或 `== 1`；另设 `== 2` / `>= 2` 负例，验证运行时效果后的谓词而不只验证来源名字。当前源码实际计数为 **23**：5 项来源正例、16 项负例、1 项还原检查、1 项真实 ch09 字节保留检查；旧 22 项记录不能代替冻结版本的实际计数。

## 范围与基线

- 工作树：`C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/wt09`。工作树 HEAD 为 `f077537226890eaa0df0ecfd1c50229436105e2a`；按用户要求，以 main `H:/Work/Kys` 的 `f6e89e0fb3ae9990e93e993890cbd86a05a14b5c` 对比两份实际工具文件，未将 WT9 的旧 HEAD 当作审查基线。main 当时 HEAD 与指定提交相同，同文件相对该提交无差异。
- 相对指定基线：validate.py 为 39 增 / 10 删；validate_selftest.py 为 121 增 / 4 删。
- 仅审 producer→Quest 来源收集、pending/invalid/selfcycle/链、真实运行时效果、M2、规则 25 范围 9 与 ch99 文件探针。未重复第 9 章内容原创性审查，未纳入 WT10 的 shapeE3。
- 用户通知 CONTENT_FROZEN 已实读、内容/工具于 17:29:40 冻结，Euclid 正串行最终 selftest、窗口未释放。本审查未申请复制窗口、未启动磁盘副本 selftest、未干预其进程。

## 已核实的正确行为

1. main 在全量入口先运行 check_path_actions，再将来源集合交给 check_quests；Quest 合并脚本来源。来源全空不再跳过无人置旗检查。没有 ch09 专用 whitelist。
2. producer 收集保留文件、局部条目、done_flag/set_flags 登记、引用、NPC 在场检查；pending 不进入 sources。M2 的 pending/非法 producer 下游、局部非法字段/引用、非法顶层 id、未登记旗标、无来源旗标均按当前函数独立复验。
3. 新闭包从脚本名字来源起步并迭代收敛；无起点自环与两节点环，真实 main 分别报 Quest 无来源 2 错 / 4 错，退出 1。合法的名字前置链被接纳；其正确性不包含上面两种已证实的不可执行边界。
4. check_path_cross 的 NPC 在场检查移到逐条校验处；旧重叠、揭破绽上阵/真实类别、孤儿路径旗标、剧情脚本禁止置路径旗标，以及章节公理前提仍在 main 执行路径中。跨条目错误仍阻断 main；新 sources 的候选是在跨条目检查前建立的，不能将其描述为对每一种跨条目非法性的完全过滤。
5. runtimePathActions 实际为 `loadGameData(...).value.pathActions` 的 **86** 条，不是从作者摘要或原始 JSON 手工拼的来源。DataLoader.cpp:734 使用 shape loader；引用门禁另由 Python/PathActionLoader 的引用入口承担。C++ 量具调用真实 PathActionRules 与 QuestRules，确认首次打探有完成/附带 SetFlag、重看不重发，求购效果完成旗在扣款/给物后，切磋起战/败战无完成旗、胜战有完成旗、pending 隐藏。
6. Application.cpp:772 的效果应用与 :812 的 SetFlag 分支、:819 的战果回调已静态核对：SetFlag 调用 `state_.setFlag(effect.flag)`，默认值为 1；扣款失败停止后续效果。本次未启动图形应用，未把效果清单的验证冒充整场购买或战斗游玩验收。

## 原严判据与独立量具

量具在 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/work-notes/ch09gate`，只在进程内虚拟化 fixture 读写与目录隐藏，不建立样本副本，不写正式 data/maps/scripts/tests。

| 核对项 | 实际结果 |
| --- | --- |
| 冻结内容 `python -B tools/validate.py` | 575 数据 / 4117 文案 / 394 旗标，0 错 0 警告，退出 0 |
| 当前原样 M2 函数 + 内存文件层 + 真实 main | 23/23，失败 0，退出 0；不是完整 selftest，也不表示 HIGH 已关闭 |
| 第 3 章狼拿掉拳、保留剑/火 | b03_gu_wai_elang 报不可达破绽；第 5 章同狼有火弹，不报该错 |
| 第 9 章付家老者只剩刀破绽 | b09_fujia 报不可达破绽，规则 25 确已扩展到 9 |
| 第 10 章 daoyu_husui 破绽改木 | 规则 25 不报，仍在范围外 |
| 第 10 章同角色破绽改非法 laser | 规则 24 仍报，形状判据没有随范围边界放松 |
| check_means_sources | 0 错；范围末章 9，新增七类手段真实来源通过原检查 |
| ch04 正文件对照 / ch99 坏文件名 | 退出 0 / 1；ch99 必有“一章一个文件、chapter 与文件名一致”的旧诊断 |
| 上述两种文件探针中的真实 ch09 | 字节均保持不变 |
| f6e89e0 原 validator 在同一 WT9 内容上 | q09_baichi 的三个步骤/三个完成谓词共 6 个无人来源错误，退出 1；证明接线差异真实存在 |
| C++ runtime_probe | 退出 0，RUNTIME_PROBE_OK；两种死窗、置 1 与 Quest >=2/==2 的语义均复现 |
| 两工具 AST / git diff --check | 通过；ruff/mypy/pylint/black/bandit 的命令和当前 Python 模块均不可用，未安装或冒称完成这些检查 |

复现入口见量具目录 README。C++ 量具的 PathActions.cpp、Quests.cpp、Objectives.cpp、Types.cpp 从当前源码直接重编，输出限于量具目录；只读数据加载复用既有 resume9eng 的 io/core 库，未触其构建槽。

## 冻结与交付边界

两工具审查前后 SHA256 一致：

```text
tools/validate.py
729b100b5ef1ae7940dedaf8c786807d77ac586da09e6d266b4666625c50d836
tools/validate_selftest.py
f2751d5a7058bf6daa7566a5062d7cf02c6d93a85abf775ea2facae1934c455b
```

作者实现记录中追加配置前的 242/242 仅作历史线索；追加配置后的最终完整 selftest 本次由 Euclid 占用窗口运行，本报告不宣称已独立验证或通过。无论最终完整结果是否全绿，上述确定的错误放行仍需修复及负例覆盖才能改为 Approve。

本审查只新增本文和指定量具目录文件；没有改 src/data/scripts/正式 tests/工具/合同，没有派代理，没有 Git 写操作，没有改 main 或 WT10。

## 返修最终复验

最终结论：**Approve**，取代首审 RequestChanges；三项原 finding 均关闭。实读 `interfaces-p3-ch09.md:408` 的返修记录和最终数字后，独立核对冻结源码、实际命令记录，并重跑原反例及正负关键。未将作者 summary 当作结论依据。

| 原 finding | 冻结返修入口 | 独立关闭证据 |
| --- | --- | --- |
| HIGH：路径旗值不可达 | `tools/validate.py:1612`；`:1886` | PathAction/Quest `>=2` 和 `==2` 均拒绝，`>=1/==1` 保留。原真实 A 前置 `B.done>=2` 报 4 错、退出 1；真实 Quest `A.done>=2` 报 1 错、退出 1 |
| HIGH：永闭窗口作为来源 | `tools/validate.py:2094`；`:2187` | 条目校验及每轮来源收敛都检查窗口。原同一前置同时出现在 when/until 的反例报 3 错、退出 1；互斥 when、路径旗置 1 后恒闭、多条 until 覆盖整数范围均被拒绝 |
| MEDIUM：切磋正例误用 ==2 | `tools/validate_selftest.py:1380` | 完整替换为 `done_flag ==1`。独立读取原样 M2 实际输出的 Quest 谓词，确认值为 1；量具重编当前 C++ 规则，胜利效果后真实 `conditionHolds(done==1)` 为真，`done==2` 为假 |

复验先自证量具的拒错能力：在进程内临时恢复原名字闭包、去掉新增可写值 guard，三类旧反例均再次退出 0、`VALIDATE_OK`；恢复冻结函数后分别 4/3/1 错、退出 1。没有将这些临时替换写入工具文件。原 `review_probe.py counterexamples` 入口也原样重跑，干净内容退出 0，自环/两节点环继续分别报 2/4 错并退出 1。

新增窗口函数单独用直接整数枚举作对照，oracle 没调用被审 `path_implies/path_contradicts`：共 **36,130** 组，差异 **0**。其中路径旗可用/不可用各 11,880 组、普通脚本旗 11,880 组、合法物品谓词 490 组；覆盖 when 的合取、until 的析取取反、等值排除与范围空集。常数为 0–4，普通脚本/物品的值 5 代表无界上尾；路径旗真实域是 0/1，无来源旗域是 0。旧名字闭包在两条确定死窗上与 oracle 相反，证明对照能暴露首审问题。这是单个主语整数约束的验证，不声称全局剧情可达性求解。

还额外观察真实 main 中 `check_path_actions` 的返回集合：永闭 A 的完成旗、附带旗，以及依赖该附带旗的 B 和依赖 B 的 C，**四者均未进入 sources**，Quest 四种引用逐一报无来源。合法 B→A→C 链倒排条目后仍全部进入 sources、main 退出 0。这补强 M3 原本只要求“存在 Quest 错误”的阴性断言，直接核实下游没有被其他报错掩盖。

聚焦原样函数的本地实跑结果（仅内存样本）：

- M2：**23/23**，失败 0；pending、非法 producer、登记、空来源、自依赖、合法来源与 ch09 字节保留检查均保留。
- M3：**22/22**，失败 0；9 项反例、12 项正例、1 项逐字节还原。正例包含脚本置 1、置 2、变量写旗，首次路径旗 ==0、until==0 后开启，合法值 2 的窗口及多步链。实际修改/还原均在内存层，没有磁盘样本写入。
- `runtime_probe.cpp` 限量重编及运行均退出 0，保留 `runtimePathActions=86` 与置 1/重复不累加/胜负/pending 证据，新增真实切磋正例 `done==1` 断言通过。

完整 **265/265** 结果没有重跑，核读的是作者原命令的实际执行记录：`python -B -X utf8 -u tools/validate_selftest.py`，cwd 为 WT9/fanren，耗时 663 秒、退出码 0。独立解析 stdout 数得 **265 个 PASS、0 个 FAIL**，末尾为“用例 265 条，未如期抓住 0 条”和 `SELFTEST_OK`；M2/M3 组分别为 23/22，余 220 项。作者 M3 修前实际执行记录为 22 项、9 失败、退出 1；修后记录为 22/22、退出 0。作者冻结时 127 项内容哈希 0 差异的实际命令输出也已核读；该数字是冻结时证据，本次未重审或重新冻结内容。

旧严判据另以 AST 对照指定基线 `f6e89e0`：**61 个旧顶层函数未变**，改变的旧函数仅 main、write_path_probe、selftest_p_path_actions、selftest_n_battle_break，对应已审查的接线与 ch99/范围迁移；没有删减旧负例或改成期待成功。M2/M3 在正式 main 的 `:2419/:2420` 接入。范围 9、ch99 的旧结论由首审独立量具及此次完整日志保留，不重复扩展审查其他工具规则。

复验量具与原始输出副本全部位于中央 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/work-notes/ch09gate`：

| 文件 | 用途 |
| --- | --- |
| `reverify_probe.py`、`reverify-oracle.json` | 独立 oracle 与负向自证 |
| `reverify-main.json` | 原三类 main 反例修前行为/修后实际诊断 |
| `reverify-m2.log`、`reverify-m3.log`、`reverify-source-selftests.json` | 原样函数 23+22 项聚焦实跑 |
| `reverify-chain-sources.json` | 确认死窗附带旗和两级下游不进来源、合法倒排链通过 |
| `reverify-author-selftest.log`、`.json` | 完整 265 命令 stdout 副本及真实退出码/计数；原记录为作者 session `01a11fdc-b477-7da0-8efa-fd33831c770e` 的 jsonl 第 548 行 |
| `reverify-old-selftests.json` | 旧函数 AST 保留证据 |

本次复验前后工具 SHA256 一致，并与作者 SOURCE_FROZEN 记录逐项相同：

```text
tools/validate.py
4afe03bc388e42adeeb3d36e8ee7b46063773713fcc120b0717a8c980e74f5e2
tools/validate_selftest.py
2d6d57c15d7c310910a1dbd8b7e6cb9583899a4577c4eb9e38902682a8a94ab4
```

两工具 AST 与限定 diff 空白检查通过；ruff/mypy/pylint/black/bandit 仍未安装。仅更新本审查报告及中央 notes，未修改工具、合同、内容、正式 tests、main 或 WT10，未派代理或做 Git 写操作。普通脚本旗仍沿用“有置旗调用即可能提供来源”的既有保守口径；此次批准限于已核实的值/局部窗口/来源闭包修复，不扩展为全局 Lua 控制流或运行时购买/战斗验收。
