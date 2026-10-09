# Chapter 10 Formal Test And BattleHand Review

Status: **APPROVE - OVERALL TESTING QUALITY**, 2026-10-09.
F1 HIGH and M1 MEDIUM are closed. No remaining finding or missing limited case
within this review. Final SRC_FILE_FROZEN contains 19 sources plus four fixtures;
all 23 current SHA256 values match. E3 production approval is unchanged.

## Early Findings

### F1 - HIGH (CLOSED) - All Action Anchor Cannot Always Select The Group Menu

Location at initial inspection: `tests/BattleHand.h:657` (`chooseTarget`),
with candidate creation at lines 419/437 and selection at lines 505/517.
All candidates are still created for every living opponent. An All spell which
can kill a later opponent can therefore be selected with that later anchor.
The real All target menu contains one group row anchored to the first live opponent.
`chooseTarget` searches its indices for the original `a.targetIndex`; a later
legal anchor is absent, so issue returns false. `act` then falls back to Defend
at line 123, dropping the planned damaging action and spending that turn on
defence. play can lose through this fallback; Ongoing is possible if no fallback
action can be submitted. It is not guaranteed to stop immediately at Ongoing.

Static counterexample: first opponent has high HP, second has 1 HP; an All spell
is legal against either. bestKill can select the second anchor while the menu
indices contain only the first. This is a test-hand/menu integration defect,
not a defect in the approved E3 production implementation.

Requested narrow repair: map All submissions to the unique group row, retain
exact Single target selection, and exercise hand.issue on a legal nonfirst All
anchor through the real menu. Semantic group candidate deduplication is permitted.
Runtime sensitivity is now confirmed on the captured initial helper (see H1 below).
Current fix: `tests/BattleHand.h:697` maps Cast/Item All to the unique group row
after checking the original action's legality; Single retains exact index lookup.
All candidates are deduplicated to the first live anchor and scoring aggregates
affected kills/breaks/targets. Independent fixed-header H1 is 4/4 GREEN below.
F1 is repaired on that captured SHA; the explicit freeze now matches it and
the F1-only repair is independently approved. No remaining HIGH/CRITICAL F1 issue.

### M1 - MEDIUM (CLOSED) - Purple Spell Menu Evidence

Location at initial inspection: `tests/Ch10BattleDataTests.cpp:206`,
`TheActualPurpleActionAndUtilityAgreeOnAllLiveEnemies`.
The test forces a purple action through issuePlayerAction after computing the
hand's preferred action. It counts four HP changes and checks MP, but does not
submit the selected action through hand.issue/menuChoose, assert the group row,
or assert BP payment. It can pass the core action while F1 remains broken.

Requested evidence: true menu/hand submission; one group target; MP/BP paid once;
per-target Hit events and observed HP deltas; utility compared with observed
total damage/kills/breaks. No Battle outcome may be supplied by a fake reply.
Current source path at `tests/Ch10BattleDataTests.cpp:387` / `:425` / `:431` now
uses a real reached final-battle checkpoint and hand.issue for purple, with
boost and expectActionEvidence checking actual HP, Hit targets, kills/breaks,
MP35 once and BP once. Separate formal Ch10HandUnit group controls inspect the
group row and exercise actual decide/menu/Item submission. The formal purple
checkpoint case actually passes in both r1-real-focused and r2-real-focused
log/XML with the same frozen source SHA. M1 is closed by this real checkpoint
evidence, in addition to the separate synthetic unit controls.

Both findings were sent to the controller before this report. The controller
assigned the narrow fixes to Maxwell. Lines describing the initial defect refer
to the retained old snapshot; final source/SHA and repaired behavior are verified.

## Static Evidence And Boundaries

- Ch10Driver loads the real C9 fixture at ch10_gudao(12,16); it does not relocate
  the chapter entry. Reached-save position/facing hydration is a separate slice helper.
- CultivationScene meditation/breakthrough, FieldScene planting/mature/harvest,
  and AlchemyScene crafting are actual entry points. No cultivation, herb age,
  products, or synthetic extra days are injected into the walkthrough driver.
- Both sides audit node account/day deltas, realm, party, 11 ending spells,
  three destroyed artifacts, 9212 relative days and the true ending (50,39).
  Both actual sides and the alternative fourth-year choice pass these assertions.
- All utility population is guarded by MagicTarget::All; Single/Single comparisons
  skip the new aggregate tie-break. Legacy suites pass in the 1768-case full run;
  actual old/new/guarded strategy contrast is verified below. Aggregate
  kills/breaks are checked against observed HP and event results.
- Formal Ch10Data.h/Ch10Driver.h/TextKeys.h reside in tests; checked formal
  includes do not depend on notes. Root CMake discovers the formal tests/helpers;
  the supplied frozen-source MSVC19.51 build and full run pass.
- TextKeys uses nlohmann JSON parsing. Local Ch05/06/07 changes preserve chapter
  prefixes, denominator assertions, required/unknown keys and malformed-JSON failure.
- C9 destination and Slice checks retain real teleport to (12,16); Objective
  transfer registration and Card10 are reviewed only at their chapter adaptation.
- Toolchain target is MSVC 19.51 from the current VS root/fresh directory. The old
  19.44 RawMacro C4129 diagnostics do not justify changing correct Ch08Ledger code.

## Raw Historical Evidence Read

Author-owned work-notes/ch10test raw logs/XML, read only:

| Run | Raw Result | Observed State |
| --- | --- | --- |
| c8-nowrite | 2/2, 0 failures | Two actual C8 endings |
| c9-write | 2/2, 0 failures | wrote=1; island12,16; days6791/6766 |
| c9-nowrite | 2/2, 0 failures | wrote=0; same island/day/resource endpoints |
| firstfull | 1760 tests, 1740 passed, 20 failures | Failure log/XML retained; not final acceptance |

Current C9 fixture SHA256 matches the author's replay evidence:
first `CDBFAE35EE128E8AD6DD7EE5AC055B33FF0E1DEF87C77E7BB06C85B351ED383E`;
second `6C9F8B19B2A41EBA9371AB01FC940B8347FA31F5E0ED53DB0C348A665A06A7ED`.
These are historical raw-run evidence plus current file hashes, not an independent
runtime replay by this reviewer. No new production-data C++ or full suite was run.

## Independent H1 Raw Evidence

Current-controller authorization allows isolated synthetic notes inputs with the
real Application/BattleScene. The captured initial BattleHand header has SHA256
`D735268394A65E41DA554F966B4C51B9741BDC1871522EC5E46B5E4331ADF2AF`,
copied and verified at 2026-10-09 19:50:51 +08:00; original source was not mutated.
All inputs are generated under notes/ch10testreview's temporary directories;
the real production chapter data is never used by these four probes.

Fresh VS18/MSVC19.51 configure/build succeeded, zero compiler warnings/errors.
Raw h1-initial.log / h1-initial.xml: 4 cases, 2 passed, 2 failed, process exit 1.

| Case | Result | Evidence |
| --- | --- | --- |
| LegalNonfirstAllAnchorMustIssueThroughTheGroup | RED | Legal anchor2; only group anchor1 exists; hand.issue false |
| RankedAllChoiceMustBeExecutableThroughTheRealMenu | RED | decide selected review_all, anchor2, boost1; hand.issue false |
| DirectGroupMenuIsTheIndependentPositiveControl | GREEN | Actual group menu, MP/BP paid once, two Hit targets, utility matches observed HP/Fall/Break |
| SingleStillIssuesToTheExactSecondOpponent | GREEN | Real menu hits only target2, target1 untouched, one MP payment and no BP payment |

These red results are valid compiled runtime failures with working controls,
not a compiler failure. The first reviewer probe had a wrong include path;
that C1083 diagnostic is retained separately as h1-probe-include-failure-* and
is not H1 evidence. Actual notes artifacts are under notes/ch10testreview.
The two All cases must become green after the author's narrow repair; preserve
the initial captured helper/red logs and compare a separately captured final helper.

## Independent Fixed-Header Recheck

Same unchanged HandMenuProbes.cpp, same MSVC19.51 build slot, separately captured
BattleHand-fixed.h: all four H1 cases pass, 0 failures/errors, actual exit0,
zero compiler warnings/errors. Ranked action is now review_all / anchor1 / boost1.
The exact Single target2 control is retained and passes. Original red snapshot,
logs/XML and executable are retained; no production data case was run here.

Artifacts: notes/ch10testreview/h1-fixed.log, h1-fixed.xml,
helper-fixed-before.json and h1-fixed-results.json. Snapshot/probe and both
source hashes match before/after verification as of 20:31:40 +08:00:

| Source | SHA256 |
| --- | --- |
| tests/BattleHand.h | `1626EF2A9B821AFDC1D8BD4F9D288F856CEE4B12AA53B187E7AECD21C5E5485C` |
| tests/Ch10BattleDataTests.cpp | `719EED3616E3DF20FA31F325E3C119DDDA992E736B2126339161C249BF52E273` |

Author raw f1-unit-red-confirmed log/XML independently read: 5 cases, 2 passed,
3 failed (nonfirst All, decided All, All item); f1-unit-green log/XML: 5/5,
0 failures/errors. Direct group and Single are the old-red positive controls.
These are author evidence; the 4/4 fixed run above is this reviewer's own evidence.
At the earlier 20:31:40 check, the F1/full-test freeze was not yet declared.
F1-only freeze has since been delivered; the overall test review still requires
the formal limited runtime evidence below.

## F1-Only Frozen Closure

Author manifest work-notes/ch10test/f1-only-source-frozen.json declares
F1_ONLY_SOURCE_FROZEN at 2026-10-09 20:33:35 +08:00. Its SHA256 is
`52124F7F7DC40C91E4AF54A1D6A37FCBCD86356CB588DB61293EC39B3CDA432A`.
At 20:35:28, all four listed file hashes matched: BattleHand.h,
Ch10BattleDataTests.cpp, Ch10Driver.h, Ch10Data.h. The first two exactly match
the independently compiled and run 4/4 version, both before and after that run.

Per the controller's explicit instruction, reuse that actual 4/4 / exit0 /
19.51 zero-warning result; no duplicate compile or test run was performed.
Binding evidence is notes/ch10testreview/f1-frozen-reused-evidence.json.
This closes the All nonfirst anchor / group submission finding, including
actual decided-menu submission and retained exact Single control.

Scoped static analysis on the same frozen helper snapshot used the VS18
clang-tidy with clang-analyzer/bugprone checks: actual exit0, no reported
diagnostic in the reviewed helper. 180 non-user diagnostics were suppressed;
this is not a zero-warning claim for all dependencies. cppcheck is unavailable
on PATH. Static-only outputs are f1-static-clang-tidy.log and f1-static-exit.json;
no object files or runtime tests were generated by this check.

## Final Raw Evidence And Verdict

Final manifest: work-notes/ch10test/src-file-frozen.json, SRC_FILE_FROZEN at
2026-10-09 21:01:50 +08:00, SHA256
`9E9A281C571EB4F7E262DF3A8F3A296665D89EA7F1651621C383B3D78A9EA82E`.
All 19 source and four fixture hashes match; the F1 four-file freeze remains
identical. Audit records and exact raw-log/XML hashes are stored in
notes/ch10testreview/final-raw-evidence.json. No duplicate test run was performed.

| Author Raw Run, Independently Read | Result |
| --- | --- |
| r1-real-focused | 78/76, exit1; only MapArt Kuixing16 and legacy ch010 formatter red |
| r2-real-focused | 78/78, exit0, no failures/errors/disabled tests; both former reds closed |
| matched1951-contrast | 1/1, exit0; same real day16003 checkpoint and shared core/game libraries |
| r2-end-write | 2/2, exit0; actual formal two-side endings generated |
| r2-end-nowrite | 2/2, exit0 after clearing write environment; strict save/continue checks |
| r2-full | 1768/1768, exit0, 107.694 seconds, no failures/errors/disabled tests |
| Final four gates | selftest363/363 missed0, validate648/text4574/flags449, maps58, art562; actual RC0 |

Final gate RCs are in work-notes/ch10test/final-results.json; raw selftest,
validate, mapgen and artgen completion markers were read. Supplemental approval
binding is notes/ch10testreview/final-approval.json. No gate was re-run here.

M1 real purple evidence: selected magic_xuelian_ziyan; four targets; observed
total damage580, kills4, breaks0; menu submission through hand.issue with MP35
and BP1 paid once. Hit nominal damage398 per enemy is not confused with HP loss.
The following wave deploys after the action. The source's expectActionEvidence
checks observed HP/event/utility equality; raw XML marks the formal case passed.

The 78-case focused run contains all 35 C10/TextKeys cases. Positive inputs
precede negative account/hook/battle mutations. TextKeys covers long/escaped/
unknown/missing keys and malformed/nonobject JSON; the eight local Ch05/06/07
parser replacements retain prefixes/counts, and those old suites also pass full.
No source-mirroring oracle or fabricated win is used for the reviewed evidence.

Both sides genuinely execute CultivationScene and FieldScene/bottle maturation/
harvest/AlchemyScene windows; strict node resource/day deltas and positive action
counters pass. They preserve realm23/cap23/former22, 11 spells, sole qu_hun_shadan,
three destroyed tools and bottle capacity6. Fixture readback confirms days
6791->16003 and 6766->15978, both +9212, actual ch10_xiaohuan(50,39), low stones
254/234 and middle stones117/116. No cultivation/age/product/extra-day injection.

Normal two-side fights are 3/4/3/2 rounds. No-needle elder replay passes;
actual guard-delay ending replay remains <=4 rounds. Matched compiler/core/game
contrast source uses the same reached save without tuning data: legacy7 rounds
(MP600/BP5), All2 (MP530/BP1), real menu Guard then All3 (MP530/BP2).

APPROVE applies to the frozen testing implementation and test-hand quality.
Root main-tree CTest/integration and chapter-wide final acceptance are separate;
no running job is counted as passed here. Original 1760/1740+20 failure logs,
F1 red controls and unrelated probe compiler/old nominal-damage mistakes remain
preserved and are not substituted for the successful frozen-source evidence.

Only docs/ch10-test-review.md and notes/ch10testreview are writable by this reviewer.
No source/test/data/tools edits, Git, delegation, C9 gate audit or E3 production re-review.
