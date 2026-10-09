# 第 10 章共享门禁集成定向审查

最终结论（2026-10-09，支付修复 SOURCE_FROZEN 定向复验）：**Approve**。唯一 HIGH 已关闭，支付邻接边界/default 兼容/多友军替代来源未发现 remaining findings。批准绑定文末支付修复的两工具 SHA，不覆盖后续文学修改后的内容/full 验收。

首审结论（返修前，以下保留历史）：**RequestChanges**。当时发现 1 项 HIGH；E3 shape、C9 已批准修复、来源其余边界与范围 11 探针的定向复验通过。旧工具 SHA 和内容 156 哈希结论均为首审时证据，新快照及关闭证据见文末。

## 首审 Finding（已关闭）

### [HIGH] 不能支付的友军法术仍被当作可打破绽的真实来源

File: `fanren/tools/validate.py:2470`，返回条件 `:2475`；消费端 `:2559`。

Issue: 新增 `role_means_available` 核对真实编成中的 ally、角色法术清单、法术五行/伤害/境界，却未检查 `needMp` 与该角色 `maxMp`。`load_break_roles` 也没有把 maxMp 放进来源信息。`hero_absent` 分支因而将任何满足上述字段的法术类别计入 means，即使这个法术在该角色最大法力下永远不能施展。

独立内存反例，仅改两处真实输入：

1. `data/magics/ch10_mufu.json` 的 needMp 从 16 改为 **100000**，其他攻击/五行/境界字段保持。
2. `data/roles/yingli_shou.json` 的 weaknesses 改为 **["木"]**，让错误来源不能被其他合法攻击掩盖。

青算子的真实 maxMp 是 **340**，围杀战 `b10_liuliandian_weisha` 为 `hero_absent=true`，其中木来源只有青算子的这门法术。独立实际结果：

| 对照 | 来源检查 | 破绽检查 | 木仍在 means | 真实 main |
| --- | --- | --- | --- | --- |
| needMp=16，敌人只怕木 | 0 错 | 0 错 | 是 | 退出 0，VALIDATE_OK |
| needMp=100000，敌人只怕木 | **0 错** | **0 错** | **是** | **退出 0，VALIDATE_OK** |

运行时证据：`src/game/BattleScene.cpp:361` 将该单位的 mp/maxMp 都初始化为角色 maxMp；`src/core/battle/BattleAction.cpp:138` 明确拒绝 `u.mp < magic->needMp`。`DataLoader.cpp:324` 读取 needMp，100000 是可加载的正常整数；本尊缺席的场没有本尊背包手段可补这个类别。这是共享门禁的确定错误放行，不是战斗平衡问题，也不声称冻结数据当前已出现该配置。

Fix: 将角色 maxMp 纳入来源数据，并按运行时缺省（Magic.needMp=5、RoleTemplate.maxMp=0）检查至少能支付一次；不能支付的法术不得进入友军攻击类别，来源检查应指明该法术不可施展。补 `needMp > maxMp` 必报且不进入 means、`needMp == maxMp` 可用、缺省值及还原后通过的正负对照。不要通过提高正式角色法力或修改敌人破绽来关闭这个门禁缺口。

## 基线与范围

- 工作树仅 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/wt10`；直接对比已最终批准的 WT9 两工具，不使用 WT10 的旧 Git HEAD 作批准基线。
- 已实读 `build-resume10eng/toolsdelivery-sha.json` 的 TOOLS_DELIVERY_FROZEN，时间 2026-10-09 18:35:12 +08:00；两工具和 `interfaces-p3-ch10.md` 实际 SHA 与冻结清单相同。
- WT9 实际 SHA 仍为 validate.py `4afe03bc388e42adeeb3d36e8ee7b46063773713fcc120b0717a8c980e74f5e2`、validate_selftest.py `2d6d57c15d7c310910a1dbd8b7e6cb9583899a4577c4eb9e38902682a8a94ab4`。
- 差异只定向审 E3 target/focused 入口、means10 与 friend role 来源、scope10/outside11 及其自检。未重复 C9 算法和原内容审查，未重审已获 125/34 Approve 的 E3 C++ 实现。
- 独立复核冻结清单七个 C++ 文件逐项一致；内容清单 156 个 source_files 哈希 0 差异。没有把作者总结当作这些结果的证据。

## 已核实的行为

### E3 与 C9 保留

- E3 `check_magic_target` 仍由 focused `--magic-targets` 和普通 `check_battle_break_data` 调用；非法类型/大小写/空白值及非伤人 all、reveal/stagger all 被拒绝，默认/single、合法伤人 all 和毒法 all 保留。原样 `selftest_magic_targets` 在内存文件层独立 **34/34**、0 失败；其普通入口正反对照也实际执行。
- C9 的九个相关来源/窗口函数与批准 WT9 的函数源码逐字一致；M2、M3、write_path_probe、selftest_p_path_actions 同样逐字一致。正式 selftest.main 仍接 M2/M3，没有用 focused 分支绕掉它们。
- WT10 原样 M2 **23/23**、M3 **22/22**，均失败 0、进程退出 0。这里只复验移植没有回退，不重做窗口算法审查或 36130 枚举；pending/invalid/selfcycle/取值/死窗/下游/合法脚本值及多步链的原断言均保留。

### 本尊与友军来源

- means10 木取 `magic_ji_qingjiao`：真实 needRealm=QiRefining1，C7 `yeyu.lua:64` 真实 learn；两份 WT10 正式 ch09-end 都是 realm=3 且 learnedMagics 含祭青蛟。其余火/土/水/金四门对应数据也均需炼气一层；拳及丝线来源沿原规则核对。
- 青元剑芒真实需 FoundationEarly；C10 从炼气期重修开始，早期不能用它登记常驻木。C10 `ruzhen.lua:7/:10/:13` 忘掉的是青凝镜、乌龙夺、白蛛飞刀，没有忘祭青蛟；当前木的来源与早期阶段一致。这里只核门禁来源，不代替通关驱动的境界、回合或两窗口验收。
- 两场 hero_absent 的类别确实只来自实际编成：围杀为拳/剑/火/水/木，执法队为剑/火/金；不会借本尊七类通表。友军缺省拳仍按 runtime role 默认计入。
- 新 helper 的定向边界：来源角色变成 enemy、从编成删除、删除法术、magics 改成字符串、缺法术定义、错误元素、超角色境界、非攻击法术或 effect 法术，均返回不可用；实际剑来源存在时通过，删剑后拒绝。以上确认不包含 Findings 所述 MP 缺口。
- 原样 `selftest_ch10_means` 内存执行 **23/23**、失败 0；已核读作者 `toolsdelivery-means-final.log` 与早先一失败记录。首次错误仅漏算默认拳，最终集合包括拳，删除青算子的来源反例仍拒绝，没有改成期待错误配置成功。23 项全绿仍未覆盖 MP 缺口。

### 范围与原严判据

- MEANS_LAST_CHAPTER=10，outside 指 `siji_haishou` / `b11_ningcuidao_youyao`，测试先核两文件存在、章号、实际敌方引用及唯一编成，避免空过滤假绿。
- 独立在内存里把第11章同一角色设为只怕合法“毒”：范围外无该错；将同一编成移到第10章：实际报“没有一样是这一场”；第11章角色改成非法 `laser`：仍报类别/形状错误。因此范围迁移没有放松全仓 shape。
- 旧规则 23、26 的字节对照和 23+22 实跑保留；E3 34 和来源/范围 23 是实际 focused 检查，不是完整 selftest 的替代结论。

## 独立证据

量具和结构化结果全部在中央 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/work-notes/ch10gate`：

| 文件 | 证据 |
| --- | --- |
| `probe.py`、`critical-mp.json` | 真实 main 正向对照和 MP 错误放行；只在内存改样本 |
| `baseline.json` | 批准 WT9 字节对照、冻结 WT10 SHA、七 C++ 哈希 |
| `target34.log` / `.json` | E3 原样 34/34 与普通 shape 入口 |
| `means23.log` / `.json` | 来源/范围原样 23/23 |
| `c9-m2.log`、`c9-m3.log` / `.json` | WT10 的 C9 回归 23/23、22/22 |
| `boundaries.json` | friend role 来源边界及 outside11 合法/非法对照 |

内存层复用首审已有量具，源码/数据/样本目录没有磁盘写入。focused 测试中的 copytree 被替换成内存输入，不占复制窗口；真实校验函数与 main 未替换。MP 比较的 runtime guard 只定向核读，没有再编译或启动 C++，没有执行完整 selftest、全构建、原内容评审或正式测试。

冻结工具 SHA 在审查前后保持：

```text
tools/validate.py
9ed97a2453d7b85762707768fa5840bd7dcfd10b0a28e8f64de30adc3a77cb60
tools/validate_selftest.py
0cb33bea1a4fff507c6316aca43c22b7ab36da0297d6da5cfebe89d060208fef
```

限定 diff 空白检查通过；ruff/mypy/pylint/black/bandit 当前命令和 Python 模块均不可用，未安装。仅新增本报告和中央 notes，未改工具/data/正式 tests/合同、WT9 或 main，未做 Git 写操作，未派代理。**本报告不能替代 Maxwell 的真实两窗口、四战及 full 验收。**

## 支付修复最终复验

最终结论：**Approve**，取代首审 RequestChanges。仅复验唯一 HIGH、支付邻接边界、defaults 与多友军替代来源；未重复 C9/E3/全工具审查。实读 `build-resume10eng/ally-mp-source-frozen.json`，冻结时间 2026-10-09 19:01:23 +08:00。首次核读时三文件 SHA 全部与清单匹配，收口继续钉住作者拥有的两工具 SHA；不将共享合同或后续内容变化误判为工具越界。

修复入口：`tools/validate.py:2401` 保留角色 maxMp，缺省为 0；`:2477` 按 Magic.needMp 缺省 5 读取成本；`:2478/:2480` 要求普通整数且 `cost <= capacity`，随后才允许该 role_magic 提供攻击类别。role_weapon 分支在支付判断之前返回，零 MP 不会误删合法兵刃。默认值已独立与 `Types.h:123/:198`、`DataLoader.cpp:324/:478` 核对一致，没有依赖作者 summary。

独立原反例与正例，在内存中使用真实数据/来源/破绽/main：

| 用例 | 恢复旧支付遗漏的内存对照 | 冻结修复 |
| --- | --- | --- |
| 原 100000/340，婴鲤兽只怕木 | 木仍在 means，main 退出 0、VALIDATE_OK | 木移除；来源报不可施展、木破绽不可达；main 退出 1 |
| 恰等于上限 340/340 | main 退出 0 | 来源/破绽 0 错、木保留、main 退出 0 |
| 免费法术 0/0 | main 退出 0 | 来源/破绽 0 错、木保留、main 退出 0 |

上述旧行为只通过进程内暂时移除 `can_pay` 闸门自证，校验后恢复；工具源码未写入。旧量具 `probe.py critical` 保留为首审反例，返修复验入口为中央 notes 的 `reverify_payment.py`。

原样 `selftest_ch10_ally_mp` 独立内存实跑 **38/38、失败 0、进程退出 0**。12 组分别核 means、来源、真实 main，另有实际 ally 前提与逐字节还原断言；包括超额一分、零 MP 正成本、缺省 needMp5 可付/不可付、缺省 maxMp0、双方缺省、免费法术与缺省零 MP。作者 `ally-mp-before.log`/`ally-mp-after.log` 也独立解析，分别为 17 PASS+21 FAIL、38 PASS+0 FAIL，未把旧红结果或作者记录代替本次实际实跑。

支付相邻验证：

- 144 组成本/容量（0–10 和各自缺字段）与运行时默认 5/0 的整数支付关系一致，差异 0。
- 化身火法不可支付但严道友火法可支付：火仍在该场 means；原不可支付的声明来源仍报错，替代来源不会掩盖坏条目。
- 两名火法友军都不可支付：火从 means 移除；零 MP 的化身仍保留剑。
- 两角色各有 200 MP、两门各需 300：不会用合计 400 MP 冒充某个角色能支付，火不进入 means。
- 将同一伤人木法设为 target=all：340/340 通过、341/340 拒绝，支付边界没有按目标数扩大或缩小。本次没有重审 E3 C++ 行为。

支付 selftest 已在 focused `--ch10-ally-mp` 和正式完整 selftest 的 `:2703` 接入。此次没有重跑 target34、source-range23、C9 M2/M3、完整 selftest/data-full 或 C++；那些已完成的历史/作者邻接结果保持其原时效，不作为本次新实跑计数。

独立结果都在中央 `C:/Users/htx-lyh/Documents/FanrenChapterWorktrees/work-notes/ch10gate`：`reverify_payment.py`、`payment-reverify-critical.json`、`payment-reverify-focused.log`/`.json`、`payment-reverify-boundaries.json`、`payment-reverify-snapshot.json`。没有建立磁盘样本副本；输出仅写中央 notes。

本次批准的工具 SHA（复验前后相同）：

```text
tools/validate.py
e6ffc7566e71c0e4986500213a049d72cfaa0725a6927f04295b32cc7c5f5fa3
tools/validate_selftest.py
3d74c91abf7b6c9cd24bb839f947c00171d42abc21705284008b289f0c6b7dce
```

原七 C++/156 内容未变记录是作者支付修复冻结时的证据；此次没有重锁/重算内容 156。Nash 后续文学 R1/M4 修改不在本次工具 ownership 判定范围。只更新本报告及中央 notes，未改工具、data、正式 tests、合同、main/WT9/其他 WT，未派代理或做 Git 写操作。Approve 仅关闭支付来源 HIGH，不证明整场 MP 续航；最终完整 selftest/data-full、真实两窗口与四战仍由主控/Maxwell完成。
