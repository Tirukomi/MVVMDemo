# Refactoring plan: step by step, stable after every pass

This turns [RefactoringProposal.md](RefactoringProposal.md) into an ordered sequence of passes. Each pass is one
reviewable unit that leaves the project shippable. Behaviour does not change unless a pass says so, and any
intended behaviour change gets its own commit.

## Ground rules

- **One pass, one branch.** Work on `refactor/pN-<name>`, merge to `master` only when the gate below is green, then
  tag `refactor-pN`. Any pass can be reverted by resetting to the previous tag.
- **Small commits inside a pass.** Each commit builds. A commit is either "move / extract" (no logic change) or
  "switch callers over", never both at once. The old code path is deleted in the pass's last commit, after the
  new one has been proven.
- **Behaviour first, structure second.** Where a pass touches logic that tests do not pin yet, characterization
  tests go in first, in their own commit, and must pass against the old code.
- **Compatibility is frozen.** Saved user data keeps its format across every pass:
  - config keys in the `[/Script/MVVMSample.GothamSettings]` section (`Language`, `ColorMode`, `UIScaleIndex`, and the rest)
  - Enhanced Input mappable names (`Attack`, `Counter`, `ClueLog`, ...), which are what saved rebinds refer to
  - LOCTEXT namespaces and keys, which translations refer to
- **Project files.** Regenerate them in any pass that adds or removes source files, so Rider and Visual Studio see
  the change.

## The gate (run after every pass)

Pass 0 builds this as a single script, `Scripts/Verify.ps1`, which prints one PASS / FAIL line per check.

| # | Check | Catches |
|---|---|---|
| G1 | Full clean rebuild (`Rebuild.bat`), zero warnings in project code | unity-build clashes, shadowing; the two bugs adaptive non-unity builds hid |
| G2 | `python Scripts/run_tests.py`, all green | logic regressions |
| G3 | `-GothamMenuInputTest`, all PASS | focus, hit-testing, prompt clicks, toggle keys |
| G4 | Screenshot regression: recapture the set and diff against `Docs/img` in 8x8 blocks (so animated scanlines and rain average out). Deterministic images (no recorded noise) are held to 0.1%; animated ones get 3x their recorded noise (`Scripts/ScreenNoise.json`, from two captures of the same code) plus 0.5 points. Review any image over that | layout, colour and localization regressions, and stale baselines |
| G5 | Perf harness at 1080p: every scenario's game-thread cost within 0.05 ms of the pass-0 baseline (median of two runs) | accidental per-frame work |
| G6 | Logs of every run in the gate: no `Ensure condition failed`, no project (`LogGotham*`) errors or warnings, and no content-integrity warnings (materials, skeletal meshes, missing usage flags, failed package loads) | tick, focus and lifetime mistakes; content that silently renders wrong (a material without its skeletal-mesh flag showed the engine default on the thugs for all of V5) |

A pass that fails any check does not merge. Fix it on the branch, or drop the branch.

## Passes

### P0: Safety net (no production code changes)

1. **`Scripts/Verify.ps1`:** runs G1 to G6 in order, writes `Saved/Verify/<date>.md`, and exits non-zero on
   failure.
2. **`Scripts/DiffScreens.ps1`:** compares two image folders with `System.Drawing`. It reports the share of pixels
   that changed beyond a small colour distance, and writes a diff image for any over the threshold. Pillow isn't
   installed, and this needs no downloads.
3. **Baselines:** the current `Docs/img` set and two perf runs (`Saved/Perf/Baseline_*`).
4. **Characterization tests** for what later passes rewrite:
   - every setting survives `SaveToConfig` then `LoadFromConfig`, and a fixture string in today's exact config
     format still loads (P4 must keep it)
   - every binding definition has an action, a keyboard mapping, and a gamepad mapping where declared, using
     today's mappable names (P3)
   - `USettingsViewModel` label, description and value text are non-empty for every setting and every choice (P4)
5. **Gate:** G1 to G6 green on untouched code. This proves the gate itself works.

### P1: Settings-listener helper (proposal item 3)

1. Add `FGothamSettingsListener` (`Accessibility/GothamSettingsListener.h`): `Bind(const UObject* Owner,
   TFunction<void(const FGothamSettingsData&)>)`, `Reset()`, and an unbind in its destructor. Include a unit test
   that binds, broadcasts, resets, and checks no call after reset or destruction.
2. Convert the 11 subscribers one commit per class group: buttons (`UGothamButton`, `UGothamHintButton`), rows and
   lists, screens and layout, glyph and HUD. Behaviour is identical.
3. `UGothamSettingsAwareWidget` becomes a thin user of the helper (its public API is unchanged).
4. **Watch for:** unbind order in `NativeDestruct`, since the helper must reset before `Super`. Also, the settings
   screen previews high contrast live, so G4 must include the `colour-blind` and `combat-access` shots.

### P2: View-model binding helper (item 4, part a)

1. Add `GothamMVVM::Bind(VM, Owner, Handler, { Fields... })` and `Unbind(VM, Owner)`, with a test on a real view
   model.
2. Convert the widgets that bind several fields to one handler, file by file. Widgets that route fields to
   different handlers (the combo's decay fast path) keep their explicit calls.
3. **Watch for:** the per-frame fast paths (combo decay, gadget cooldowns). The G5 `hud-animating` row must not
   move.

### P3: One input-action table (item 2)

1. Add `Input/GothamActionTable` with one `FGothamActionDef` per action (name, display text, value type,
   default keyboard and gamepad keys, rebindable, has gamepad slot). The names are exactly today's mappable names.
2. `BuildInputAssets` loops over the table. `FindAction` becomes a `TMap` lookup, and
   `GothamBindings::GetDefinitions` is derived from the table (it keeps its signature, so the Controls screen and
   the rebinding tests don't change).
3. Handlers stay explicit (`BindAction` per action), because their signatures differ.
4. **Watch for:** saved rebinds. Before merging, run the rebind demo (`-GothamRebindDemo`), restart, and confirm the
   Controls screen still shows the rebound key. The P0 test pins the names.

### P4: Settings as a descriptor table (item 1, and item 4 part b for settings)

This is the biggest logic pass, so it's split into three merges. Each one is gated.

- **P4a: introduce the table beside the old code.** Add `FGothamSettingDescriptor` (id, config key, label,
  description, choice count, format-choice, get, set, wraps) and a table covering all nine settings. A test asserts
  that the table and the old switch statements agree for every setting and every choice (labels, values, positions,
  cycling). Nothing uses the table yet.
- **P4b: switch the data layer.**
  - `Cycle`, `GetOptionPosition`, `GetLabel`, `GetDescription`, `GetValueText` and the config round-trip loop
    over the table.
  - `operator==` stays a plain field comparison (it's clearer).
  - The old switches are deleted.
  - The P0 characterization tests (including the config-format fixture) must pass unchanged.
- **P4c: switch the view layer.**
  - `USettingsViewModel` gains a field-notify `Revision` counter.
  - `UGothamOptionRow` subscribes to it instead of nine text fields.
  - The nine per-setting text properties stay for one pass (for any Blueprint binding), marked deprecated in a
    comment, then are removed in a follow-up commit once nothing reads them.
- **Watch for:**
  - live language switching (the rows must re-read text when `Revision` bumps after a culture change); check with
    G4's `settings-de` / `settings-ja`
  - the UI-scale clamp, where it must not wrap (tested)
  - the "unapplied changes" note

### P5: Shared world-overlay base (item 7)

1. Extract `SGothamWorldOverlay<TItem>` (provider, `SetActive`, active timer, per-frame invalidate) and a UMG base
   `UGothamWorldOverlayLayer`, which holds the owning player, the projection helper and the view-model activation
   hook.
2. Port `SClueMarkerLayer` first. Detective shots go through G4, and the detective perf row through G5.
3. Port `SThreatIndicatorLayer`, then check the combat shots and combat perf row.
4. **Watch for:** the timer must still unregister when inactive. G5's `hud-idle` row is the check.

### P6: Dev aids out of the player controller (item 5)

1. **Move, no logic change:** `RunDevAids` and its helpers go to `Core/GothamDevAids.cpp` (compiled out of
   Shipping), one function per flag. The controller calls `GothamDevAids::Run(this, UI)`.
2. **Separate behaviour commit:** fake clues become a flagged entry kind (`UClueEntryViewModel::bIsDebug`, set by
   the dev aid). The objective counts non-debug entries. `DebugClueCount` and `RemoveDebugClues` go away, and the
   harness teardown filters by the flag instead.
3. **Watch for:** every flag in the README list still does what it did. G4 exercises all the screenshot flags, and
   the capture script is the regression suite for this pass.

### P7: One scripted-scenario runner (item 6)

1. Add `FGothamScript` (steps `Wait`, `Do`, `Expect`, `Sample`, `Screenshot`, `Quit`), with a unit test of step
   timing using a fake clock.
2. Port the menu input test first (it has the most asserts). G3 must still show every PASS line, word for word.
3. Port the perf harness scenarios. The report format stays identical so baselines remain comparable (G5).
4. Port the screenshot flags one by one. Command-line flags keep their names and meaning; only the sequencing
   underneath changes.
5. Delete the old tickers and run structs.
6. **Watch for:** real-time versus game-time waits. The pause menu and the wheel's slow-mo stop game time, so the
   runner uses the core ticker (real time), as today.

### P8: PO-based localization (item 10)

1. Export the current archives to `.po` with `ExportTextToPO` and commit the `.po` files (one per culture).
2. Change `Scripts/Localize.bat` to gather, import `.po` and compile, and retire `TranslateLocalization.py`. Its
   tables are already in the `.po` files.
3. **Gate:** `Game.locres` for each culture decodes to the same key/value set as before (a small comparison
   script), plus G4's localized shots.
4. **Watch for:** the pseudo-locale (`en-XA`), which is generated. Keep a tiny script for it, or use the engine's
   pseudolocalization at runtime.

### P9: Promote dev-flag checks to automation tests (item 11)

1. Wrap the P7 scripts for the menu input rules as latent automation tests (they need a game world), tagged
   `Gotham.Functional.*`.
2. Teach `Scripts/run_tests.py` to run them in a game-mode pass, so G3 becomes part of G2.
3. Screenshots stay a G4 review step; comparing screenshots automatically is out of scope.

### P10: Deferred, only when the code next changes for a feature

- **Option rows as Common UI buttons (item 8).** Needs P9 first, so navigation and left / right value changes are
  covered automatically.
- **Layout helpers (item 9, near-term part).** A fluent slot helper, adopted opportunistically in files a feature is
  already touching. There's no big-bang conversion.
- **UMG assets for static screens (item 9, long-term).** An ADR update first. Then one screen at a time behind its
  existing C++ base class, starting with the simplest (pause).

## Timeline

| Pass | Size | Depends on |
|---|---|---|
| P0 | 1 day | none |
| P1 | half a day | P0 |
| P2 | half a day | P0 |
| P3 | half to 1 day | P0 |
| P4a, P4b, P4c | 2 days | P0; easier after P2 |
| P5 | 1 day | P0 |
| P6 | 1 day | P0 |
| P7 | 2 days | P6 |
| P8 | 1 day | P0 |
| P9 | 1 to 2 days | P7 |

P1, P2, P3, P5 and P8 are independent and can go in any order. P4 is easiest after P2, and P7 needs P6.
