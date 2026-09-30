# Refactoring proposal: simpler ways to do what the code already does

A review of the whole module after V5 (about 16,000 lines of C++, 41 tests). Nothing here fixes a bug. Each item
removes repetition or a special case, so the next feature touches fewer places. Items are ranked by payoff for the
effort, and each is independent, so they can land one at a time behind the existing tests.

Counts are from the current tree (`01494e6`).

## Summary

| # | Change | Removes | Effort | Risk |
|---|---|---|---|---|
| 1 | Settings as a descriptor table | about 45 `switch` cases across 2 files; 11 touch points per new setting become 2 | M | Low (well tested) |
| 2 | One input-action table | three parallel lists (12 `FindAction` ifs, 8 `MapPair` calls, 14 binding defs) | S | Low |
| 3 | A settings-listener helper | 11 hand-written subscribe/unsubscribe pairs | S | Low |
| 4 | A view-model binding helper, or one `Revision` field | 57 `AddFieldValueChangedDelegate` calls in 18 files | S to M | Low |
| 5 | Dev aids out of the player controller | 285 lines of `RunDevAids`, plus a debug-clue special case in the view-model subsystem | M | Low |
| 6 | One scripted-scenario runner for dev aids, perf and input tests | three sequencing mechanisms (tickers with delays, the perf run, the menu-test run) | M | Medium |
| 7 | A shared world-overlay base for the Slate layers | duplicated provider / active-timer / projection code in the clue-marker and threat layers | S | Low |
| 8 | Option rows as Common UI buttons | a third hand-rolled hover / focus / press state machine | S | Medium (navigation) |
| 9 | Layout helpers, or UMG assets for static layouts | 152 `ConstructWidget` calls, 128 slot setters | M to L | Medium |
| 10 | PO files instead of the translation script | the source-text-keyed tables and the archive re-sync workaround | S | Low |
| 11 | Promote dev-flag checks to automation tests | manual log reading for `-GothamMenuInputTest` and screenshots | M | Low |

Recommended order: 3, 2, 1, 4, 7 first (small, mechanical, tests already cover them). Then 5 and 6 together, then
10 and 11. Leave 8 and 9 for when the layouts next change.

## 1. Settings as a descriptor table

**Now.** Adding one setting touches about 11 places:
- a field on `FGothamSettingsData`
- `Cycle`, `GetOptionPosition`, `operator==`, `LoadFromConfig` and `SaveToConfig`
- `USettingsViewModel::GetLabel`, `GetDescription` and `GetValueText`, plus a field-notify text property and its
  update in `Recompute`
- `GetTabs`
- `UGothamOptionRow::Setup`, which subscribes to all nine value fields one by one

The DesignerGuide's "Add a setting" recipe is five steps long for this reason, and it is the part of the code most
likely to drift (the option-position switch was added late).

**Simpler.** One static table of descriptors, each with an id, label, description, choice count, a function that
formats a choice, and get / set accessors onto `FGothamSettingsData`. `Cycle`, `GetOptionPosition`, the label and
description lookups and the config round-trip become loops over the table. The view model exposes one field-notify
`Revision` counter instead of nine text fields, and rows re-read their own descriptor when it changes.

**Payoff.** A new setting means one table row and one field. The existing `SettingsTests` and `MenuV4Tests`
(every setting in one tab, positions in range, cycling) already pin the behaviour.

## 2. One input-action table

**Now.** Each gameplay action appears in three lists that must agree:
- `BuildInputAssets` (action, keys, mappable name)
- `FindAction` (a chain of `if (Name == ...)`)
- `GothamBindings::GetDefinitions` (display name, whether it has a gamepad slot)

`ToggleActionName` and glyph lookups depend on these names matching.

**Simpler.** A single `FGothamActionDef` table (name, display text, default keyboard and gamepad keys, rebindable,
value type) that builds the actions, fills a `TMap<FName, UInputAction*>` for `FindAction`, and is the list the
Controls screen shows. Binding handlers stay where they are, keyed by name.

## 3. A settings-listener helper

**Now.** Eleven classes repeat the same pattern: `Get` the settings subsystem, `OnSettingsChanged.AddWeakLambda`,
store the handle, and remove it in `NativeDestruct`. `UGothamSettingsAwareWidget` already wraps this for
`UUserWidget`, but Common UI buttons, the layout and the screens can't derive from it.

**Simpler.** A small member type, `FGothamSettingsListener Listener;`, with `Bind(Owner, Callback)` in construct
and an automatic unbind in its destructor (or on `Reset`). Every copy of the pattern collapses to one line, and a
forgotten unsubscribe becomes impossible.

## 4. A view-model binding helper

**Now.** There are 57 `AddFieldValueChangedDelegate` calls. Most widgets subscribe several fields to the same
handler and later call `RemoveAllFieldValueChangedDelegates`. The option row subscribes to nine fields to learn that
"something changed".

**Simpler, in two parts.**
- `GothamMVVM::Bind(VM, this, &Handler, { Field1, Field2, ... })` for the common many-fields-to-one-handler case,
  with the matching unbind.
- Where a widget only needs "refresh", give the view model a single `Revision` field (as item 1 does for settings).

The UE MVVM plugin's own view bindings (`UMVVMView`) would remove this entirely for asset-built widgets, but they
are designed around Blueprint widgets, so they fit better if item 9 happens.

## 5. Dev aids out of the player controller

**Now.** `AGothamPlayerController.cpp` is the largest file (742 lines), and 285 of them are `RunDevAids`: screenshot
staging, demos and timing constants. `UGothamViewModelSubsystem` carries `DebugClueCount` so fake clues don't count
toward the objective. That's a production class special-cased for a dev aid.

**Simpler.** Move dev aids to `Core/GothamDevAids.cpp` (a `UGameInstanceSubsystem` or a plain namespace, and not
built in Shipping), one function per flag. Fake clues become a tagged entry type (or live in a separate view
model) that the objective simply ignores, with no counter to keep in sync. The controller goes back to input
and possession.

## 6. One scripted-scenario runner

**Now.** Three ways to "do X, wait, check Y" have grown up side by side:
- `RunDevAids` chains `FTSTicker` delays (`Delay - 0.45f`, `Delay - 0.6f`, ...).
- The perf harness has a scenario list with setup, per-frame and teardown.
- The menu input test has a timed step list with PASS / FAIL.

**Simpler.** One runner with named steps: `Wait(seconds)`, `Do(fn)`, `Expect(pred, rule)`, `Sample(stats)`,
`Screenshot(name)`. The screenshot flags, perf scenarios and input checks become scripts for it. Timing lives in
one place, and each new demo or check is a short list instead of a new mechanism.

## 7. A shared world-overlay base for the Slate layers

**Now.** `SClueMarkerLayer` and `SThreatIndicatorLayer` (and their UMG wrappers) each implement the same things:
- a provider callback
- an active timer that runs only while there is something to show
- projection with the owning player's camera
- a per-frame invalidate

**Simpler.** A `SGothamWorldOverlay<TItem>` base (provider, `SetActive`, timer) and a matching UMG base that does the
projection. Each layer keeps only its paint code. A third layer (objective waypoints, say) would then cost only its
drawing.

## 8. Option rows as Common UI buttons

**Now.** Hover, focus and press state is hand-written three times: `UGothamButton`, `UGothamHintButton` and
`UGothamOptionRow` (a focusable `UUserWidget` with its own mouse, focus and colour handling).

**Simpler.** Make the option row a `UCommonButtonBase`, keeping its `NativeOnNavigation` override for left / right.
It gets Common UI's focus handling, click routing, input-method awareness and sounds for free, and shares one
state-to-colour function with the other buttons.

**Risk.** Navigation must still treat left / right as value changes, and `-GothamMenuInputTest` should cover it.

## 9. Layout helpers, or UMG assets for static layouts

**Now.** Every screen builds its tree in C++ (ADR 0002): 152 `ConstructWidget` calls and 128 slot setters, much of
it three-line "add child, set padding, set alignment" sequences. Layout tweaks mean a C++ rebuild.

**Simpler, in two steps.**
- **Near term:** a fluent helper, for example `GothamLayout::Add(Box, Widget).Pad(0, 8).Fill().VCenter()`, cuts the
  boilerplate roughly in half with no change in approach.
- **Longer term:** revisit ADR 0002 for static screens (pause, settings, case file). Designer-authored UMG assets
  over the existing C++ base classes (`BindWidget`) put layout in content, where it is cheap to change. Custom Slate
  (wheel, meters, overlays) and all behaviour stay in C++.

This is the largest change, so do it only when a layout next needs real rework.

## 10. PO files instead of the translation script

**Now.** `TranslateLocalization.py` holds translations as dictionaries keyed by English source text, and has to
re-point archive entries whose source changed under the same key. That workaround was added in V4, after the
titles silently stayed in English.

**Simpler.** Use the engine's own round-trip: GatherText can export and import `.po` files
(`ExportTextToPO` / `ImportTextFromPO`). Keep one `.po` per culture in the repo and let translators, or a script,
edit those. Stale entries are then the engine's problem, and the key/source mismatch class of bug goes away.

## 11. Promote dev-flag checks to automation tests

**Now.** The menu input test and every screenshot are verified by reading logs and images by hand.
`Scripts/run_tests.py` runs only pure logic.

**Simpler.** Run the input rules as UE functional or latent automation tests: open a map, push the screen, send the
Slate events, assert. That way `run_tests.py` (the CI entry point) covers focus, hit-testing and routing too.
Screenshots can stay manual, or use automation screenshot comparison.

## What not to change

- **The pure-logic structs** (`FGothamThugBrain`, `FGothamAttackDirector`, `FGothamSlideRect`, `FGothamScanPulse`,
  the ghost fill, the edge-arrow maths). They are small, tested and easy to reason about. The pattern works.
- **The one-pass Slate layers.** Drawing all markers and threats in one widget, with no widget per enemy or clue,
  is what keeps the UI budget flat. Item 7 only shares their plumbing.
- **The view-model boundary.** Gameplay components never see widgets, and widgets never see actors. Every item
  above keeps that.
