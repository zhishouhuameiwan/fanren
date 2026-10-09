# Chapter 10 E3 independent C++ review

Status: **APPROVE (E3 only)**, 2026-10-09 18:10:20 +08:00.

Scope: WT10 only, contract `docs/interfaces-p3-ch10.md` E3. Reviewed files are
`src/core/model/Types.h`, `src/core/battle/Battle.h`,
`src/core/battle/BattleAction.cpp`, `src/io/DataLoader.cpp`,
`src/game/BattleScene.h`, `src/game/BattleScene.cpp`,
`tests/Ch10MagicEngineTests.cpp`, and the new target-shape boundary only.
E1/E2, C9 engine, C9 PathAction source gate, and chapter completion are excluded.

## Findings

No confirmed CRITICAL, HIGH, or MEDIUM E3 finding remains. The author-declared
freeze has been independently verified, re-read, clean-recompiled, and re-run.
No production source, formal test, tools, contract, or content file was edited.

## Final Static Evidence

- `BattleAction.cpp:361`: normal all and charged all use one live-opponent collection.
- `Battle.h:149`: `alive()` excludes dead, fled, and off-field units.
- `BattleAction.cpp:432`: MP and BP are paid once before the per-target loop.
- `BattleAction.cpp:461`: each target retains existing damage, weakness, poison, and events.
- `BattleAction.cpp:242`: survival/wave advancement runs after the whole action.
- `DataLoader.cpp:401`: exact single/all strings; all rejects non-damage effects.
- `BattleScene.cpp:668`: all-target spells and castMagic items use one group row.
- `BattleAction.cpp:53`: actual actor/current turn, legal anchor, learned spell,
  realm, MP and boost are checked before mutation; invalid actions preserve state.
- `BattleAction.cpp:473`: poison applies once to each surviving target; dead targets
  stop subsequent hits. Per-target realm, defence, guard and known weakness reuse
  the single-target resolution. Default/explicit single and old effects stay intact.
- `BattleScene.cpp:798`: real menu exit uses the existing player action entry,
  retaining one item deduction and feedback; Hit/Break/Fall/End events remain shared.

## Final Independent Verification

Independent probes and build outputs are confined to `work-notes/ch10magicreview/`.
The author build slot `build-resume10eng` is not used.

| Evidence | Result | Artifact |
| --- | --- | --- |
| Independent MSVC Release clean build | Exit 0, zero compiler warnings/errors | `final-build.log` |
| Focused C++ run | 125/125, 0 failures/errors, exit 0 | `final-focused.log`, `final-focused.xml` |
| Target-shape positive/negative selftest | 34/34, exit 0 | `final-target-selftest.log` |
| Current data, target-only validation | Exit 0, MAGIC_TARGETS_OK | `final-target-shape.log` |
| Frozen source SHA before/after verification | 10/10 match, including all seven C++ files | `final-source-before.json`, `final-source-after.json` |
| clang-tidy, all checks except llvmlibc | Four translation units exit 0; no compile errors | `clang-tidy-results.json`, `*.clang-tidy.log` |
| cppcheck | Not available on PATH; not run | Availability checked |

The 125 cases comprise 15 formal E3 cases, 9 independent probes, and 101 adjacent
Battle/loader/menu regression cases. Formal E3 cases exercise the real loader,
enemy AI, real headless Application/BattleScene cast and item menus, and isolated
copies of the two actual spell files. They are useful entry-point evidence,
but not a chapter playthrough or on-screen animation acceptance.

Independent probes add lethal wave transition without hitting the newly deployed
wave, final Won/Lost and Fall/End events, per-target realm/defence/known weakness
equivalence to single-target casts, guard, poison-only and death stopping,
dead/fled/future anchor refusal, item boost/effect refusal, and loader compatibility.
The final added probe uses caster index 2 on both factions, confirming actor/target
event indices, live-opponent filtering, and resource ownership/payment once.

The author's 15/15, adjacent 131/131, mutation/red/restore history and original
14-case/3-failure logs are author evidence. This verdict rests on the independent
125-case final run and SHA checks above. No additional author-source mutations
were performed by this reviewer.

clang-tidy read an independent copy of all seven reviewed C++ files, each verified
against `source-before.json`, with the independent compile database and C++20/MSVC
environment. All seven snapshot hashes match the final frozen C++ hashes, so
this existing analysis remains applicable without repeating it. Files analysed:
BattleAction.cpp, DataLoader.cpp, BattleScene.cpp,
Ch10MagicEngineTests.cpp, including the reviewed headers. No `--fix` was used.
All-checks output is warning-heavy: enum size, packing, naming, header style,
function complexity, parameter similarity, and existing implicit-constructor/RNG
diagnostics were assessed by behavior and relevance, not promoted into E3 findings.
No confirmed new memory-safety, security, concurrency, or gameplay defect emerged.

## Freeze Boundary

The initial 124/124 run occurred during the author's mutation window and remains
preliminary history; the final clean 125/125 run above supersedes it.
Freeze manifest: `build-resume10eng/e3-source-frozen.json`, modified
2026-10-09 18:04:53 +08:00, SHA256
`EF9F3EC9E8A6E91295959279180C637900E94F14FEDD69D7CA69BA80CA9896F3`.
The exact seven C++ file hashes are pinned in the before/after JSON artifacts.

Contract/tools hashes verified both before and after, valid as of
2026-10-09 18:10:20 +08:00:

| File | SHA256 |
| --- | --- |
| `docs/interfaces-p3-ch10.md` | `56D0F508B9D607029545F0359A998BB47C26CC60325BA39C15C0B669649D66D9` |
| `tools/validate.py` | `EC78408B92F44440E1884D7A4810470089DA8A8AC4B1FA660D3F9B8BF899008E` |
| `tools/validate_selftest.py` | `057E285EB3C30D504BE2EBA02F884692EE6F0327D8ACEEBD89D59697DAF4999F` |

The later C9 producer/means/probe merge is not included in these tools hashes or
this approval. Once merged, verify the seven C++ hashes remain identical and
re-run E3 target-only checks/selftest against the new tools, recording new hashes.
This is an integration follow-up, not a remaining E3 source finding. C9 tool
review and chapter full acceptance stay separate and do not gate this E3 verdict.

The system-required read-only C++ `git diff` was run once in WT10;
no other Git operation was performed. No source, formal test, contract, content,
or tools file is edited by this reviewer. No agents are delegated.
No full chapter playthrough, data/full gate, or full-suite acceptance is claimed.
