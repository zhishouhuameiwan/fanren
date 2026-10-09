# 第9章引擎独立审查
范围：仅 WT9/fanren，相对 `f077537` 的源/测试补丁（含未跟踪的 Ch09EngineTests.cpp），依据 interfaces-p3-ch09.md。
结论：Warning；未发现 CRITICAL/HIGH 必修项，以下两项 P2 建议在集成前修正。

- **P2 正确性：非法大整数可别名成合法跌落目标。** `scripts/common/api.lua:190` 只查 number，`src/script/ScriptHost.cpp:164` 以 int 读取 x，新增入口 `src/game/Application.cpp:1330` 只能校验缩窄后的编号。
  静态复现条件：当前境界22时调用 `realm.demote(4294967299)`（2^32+3）；Lua保存64位整数，sol2直接转int得到3，命令返回成功并清法力/修为，违反非法目标应 no_realm 且不动状态的合同。
  依据：`vendor/sol2-3.3.0/include/sol/stack_check_unqualified.hpp:143` 只判Lua整数；`stack_get_unqualified.hpp:132` 直接 static_cast。共享宿主的缩窄问题已存在，新增跌落入口继承它；应在缩窄前检查整数及范围，非法回 false/no_realm，并补此边界测试。
- **P2 测试回归：兼容扫描会误拒合法v9跌落档。** `tests/Ch09EngineTests.cpp:458` 对扫描到的所有 .sav 都要求 formerRealm=Mortal。
  可复现条件：将合法 `realm=3/realmCap=3/formerRealm=22` 的v9档放入 tests/fixtures 或 saves；loadGame成功，扫描测试却失败；合同第6.3节明确第9章章末夹具为v9。
  建议：仅缺字段的旧档断言默认Mortal；含 formerRealm 的v9档按落盘值验证，保持至少10份的非空判据。

已核对的正确性与回归边界：
- `src/core/rules/Realm.cpp:152` 的非法/凡人→相等→升境→降境顺序符合合同；原按级数 demote 未改。
- `src/game/Application.cpp:1346` 先取历史最大值，再同步 realm/cap/maxHp/hp/maxMp/mp/修为/余数；成功路径无挂起、回调或跌落音效，失败/相等在赋值前返回。
- 上述分支未写法术、瓶子五项、队伍、背包、flags及其他保留字段；`tests/Ch09EngineTests.cpp:216` 用非默认完整状态和序列化快照覆盖它们。
- `src/io/SaveFile.cpp:361` 在读取/迁移上限后交叉校验；`:367` 在缩窄前检查 formerRealm 类型、范围及枚举空洞，不错误比较历史境界与当前境界；`:550` 显式登记8→9。
- `src/game/MenuScene.cpp:194` 只在修仙阶段且境界不足时替换文字，查看行保持可选；战斗仍由 `src/core/battle/BattleAction.cpp:131` 拒绝、`src/game/BattleScene.cpp:800` 阻止禁用项操作。
- 四个已批准版本同步文件仍使用严格相等/精确版本文本；其余修改仅补合法 realmCap 夹具，原行为断言保留，未见放宽判据。

测试质量：实际源码定义27条，覆盖真Lua、连续跌落、伤血夹值、读写往返、老档链、菜单和战斗列表；此处不将报告的9组负向或正在跑的全量结果视为独立验证。
剩余缺口：上述大整数回绝；瓶/队/背包/flags应增加独立逐字段对照（快照双方共用序列化器，可共同漏字段）；战斗拒绝用例尚未实际点禁用项或提交施法并断言HP/MP不变（`:307`）。
验证：git diff --check通过；匹配当前MSVC的clang-tidy对Realm.cpp、SaveFile.cpp退出0，诊断未揭示修改行的安全问题；PATH未发现cppcheck。未跑全构建、测试或变异，未采用旧Ninja失败日志。
本审查仅新增本文，未改src/tests/工具/数据/合同，未操作git写入、主树、WT8/WT10或再派代理。
P2整改后最终结论（2026-10-09）：Approve，取代首审Warning；两个P2关闭，可接WT10（仅两个P2及邻接保留字段的限量结论）。`src/script/ScriptHost.cpp:166` 在缩窄前检查有限整数及int范围，非法走no_realm且不动状态；`tests/Ch09EngineTests.cpp:158` 按原档字段校验，`:636` 覆盖合法v9跌落档；`:48` 独立逐字段保留、`:487` 真Cast拒绝后所有单位HP/MP及GameState不变，已补齐首审相关缺口，未发现残留问题。
复验证据：`build-resume9eng/p2-before.log:4` 编译退出0、`:502` 两个复现测试失败退出1；`p2-after.log:2` 确实重编ScriptHost；`p2-final-subset.log:85` 的33/33测试名与当前源码逐一一致、`:87` 退出0；`p2-final-ctest.log:3495` 为1657/1663通过，六条Ch07失败与整改前 `final-ctest.log:3487` 同名一致。此次仅只读修补、33例和日志并追加本文，未重跑构建/测试、未改源/测试/合同；六条既有Ch07失败仍未解决，其基线归因不在本次复验范围。
