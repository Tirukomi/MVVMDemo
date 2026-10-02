# Second review: findings and plan

A second senior-level review, after review fixes R0 to R8 (2026-10-01, `master` at `705f2d7`). It looks for what the
first review missed and for what the fixes left behind. Numbers in brackets refer to the findings at the end. Each
finding says whether it was checked in the code or still needs measuring.

## Summary

The code is in good shape. Every review finding has a test or menu-input rule. Gameplay never touches widgets,
custom Slate is event-driven, and the gate (build, tests, screenshots, perf A/B, logs) catches regressions. What is
left is mostly four things:

1. **Two player-facing gaps** that the tests do not see: the HUD's gadget key hints never show gamepad buttons [1],
   and the gadget wheel ignores the colour and contrast settings [2].
2. **Shipping hygiene**: the menu-input test (750 lines) and the debug keys F1 to F3 are compiled into Shipping [5, 6].
   R7 said the game module had no test code left; that holds for the automation tests, not for this one.
3. **The MVVM story is half-proven**: no screen uses the designer path that ADRs 0002 and 0008 promise, and the
   view-model resolver it relies on has no test and no user [11]. The settings view model's single `Revision` field
   leaves nothing per option to bind to [14].
4. **The gate guards change, not level**: G5 allows +0.05 ms per iteration against the last one, with no absolute
   budget [19], and the idle HUD's 0.19 ms has never been broken down [23].

## Status

- **S0 done** (findings 19, 20, 23; found 27 to 29):
  - `Scripts/ProfileUI.ps1` profiles every perf scenario with Unreal Insights (one timing region per scenario) and
    `Scripts/profile_diff.py` lists what each adds over its reference, timer by timer.
  - The profile showed the harness itself was off [27]: `case-file-505` never opened the case file until R4, and
    `pause-quit` never opened pause; and a scenario that pauses the world was compared with a running one. Each
    scenario now checks its own state (`Valid` column; G5 fails on `no`), and paused scenarios have their own
    reference (`no-ui-paused`).
  - The idle HUD's ~0.15 ms is mostly world work; Slate adds about 0.04 ms, the biggest piece the threat layer's
    every-frame timer [29]. The real outlier is the scrolling case file, +0.55 to +0.65 ms, over the 0.3 ms target:
    S7 starts there.
  - G5 checks absolute budgets per scenario (`Scripts/PerfBudgets.json`), and G7 checks that every string has a
    current translation (`Scripts/CheckTranslations.py`; it catches the missing and the stale).
  - Numbers and method: `Docs/Performance.md`, "Second review S0".
- **S1 done** (findings 1 to 3):
  - The HUD's gadget hints show the key of the device in use (the gamepad button, in the connected pad's naming) and
    change with it, guarded by two menu-input rules; checked to fail without the fix.
  - The gadget wheel's labels, hub text and outline take the theme's colours, and its font follows the Text size
    setting; a menu-input rule compares them with the theme. The `hud-wheel` baseline was refreshed (labels are the
    palette's primary text colour now, a shade under white).
  - The "clue found" subtitle counts down only while the game runs: real time, so a hit-stop does not stretch it, but
    not while paused (`USubtitleViewModel::ShowFor` / `Advance`, tested in `Mvs.ViewModels.Subtitles`).
  - G5 judged single rounds, and the run order (whichever build ran second was slower) decided the median: S1 failed
    twice on scenarios it does not touch. G5 now runs six rounds and judges the median of order-balanced pairs
    (`Docs/Performance.md`). Its self-test still catches an injected 0.1 ms, but it reads as +0.067 ms, so the
    margin for regressions near the 0.05 ms tolerance is thin: worth more samples per scenario if that matters.

- **S2 done** (findings 5 to 8):
  - The menu-input rules are in the test module (`Source/MVVMSampleTests/Private/MvsMenuInputTest.*`); the
    `-MvsMenuInputTest` dev aid is gone (the automation test runs the same rules). `FMvsScript`, the debug actions
    (F1 damage, F2 heal, the F3 attack shortcut) and `AddDebugClues` are compiled out of Shipping.
  - Checked in the binary: a Shipping build of master contained the debug actions; after S2 it contains none of the
    probe strings (rules, script log, debug actions, debug clues). Findings 5 and 7 were overstated: master compiled
    that code for Shipping, but nothing referenced it there, so the linker had already dropped it. Only the debug keys
    really shipped.
  - `MVVMSample.Build.cs`: template comments gone, RenderCore and RHI only outside Shipping (the perf harness), and
    the public module root kept on purpose (documented: the test module is its only consumer).
- **S3 done** (findings 17, 18, 21; found 30):
  - `Mvs.Functional.MenuInput` is seven functional tests: `Hud`, `CaseFile`, `Pause`, `Settings`, `Controls`,
    `GadgetWheel` and `ViewModelResolver`, sharing `MvsMenuTestKit.h`. Each starts and ends with no menu open and the
    settings reverted, so one bug fails one test, and any one runs alone (`run_tests.py Mvs.Functional.Pause`).
  - Splitting found a player-facing bug [30]: settings reopened from pause had no tabs. It only showed once the
    rules stopped running in one fixed order. Fixed in `UMvsTabList`; two rules guard it, checked to fail without the fix.
  - The "game window must be the active application" precondition is gone [18]: Slate skipped mouse capture while
    another application was active, so a click's release never reached the button. The tests turn on the engine's
    switch for input while inactive (`SetHandleDeviceInputWhenApplicationNotActive`), and one rule clicks a prompt with
    the application marked inactive.
  - The resolver test [21] checks that every HUD view model class resolves to the player's instance, and that a class
    the player has none of, or no widget, resolves to nothing.
  - 61 tests now (54 editor, 7 functional); the functional pass still takes about a minute.
  - Gate: the first full run failed G5 on `pause-quit` alone (+0.051 ms against +0.05; rounds -0.065 to +0.089), a
    scenario S3's code does not run in (pause and its quit confirmation; the tab list is in settings). The G5 rerun
    passed (+0.018 ms). `pause-quit` is the noisiest scenario: its pairs spread about twice as wide as the others'.
- **S4 done** (findings 9, 10, 12, 13, 16):
  - Screen shortcuts are data [9]: `UMvsUISettings::GetShortcuts` lists action, layer, screen-class setting and key
    behaviour (open or back, toggle, hold); `UMvsUISubsystem::HandleShortcut` replaces `ToggleClueLog`,
    `TogglePauseMenu` and `OpenGadgetWheel`; the controller binds every row, and `UMvsScreen` takes its own closing key
    and the other screens' toggle keys from the table (no more `ToggleActionName` or the hard-coded case-file key).
    The table points at the screen-class settings rather than copying them, so S6's `WBP_PauseMenu` swap needs no
    second edit. `Mvs.UI.Shortcuts` checks the table; the functional rules for 7 and 11 still pass.
  - Settings ownership [10]: `UMvsSettingsSubsystem` is the model (`Preview`, `Commit`, `Revert`, `GetSaved`,
    `HasUnsavedChanges`) and knows no view model. The settings screen creates its own `USettingsViewModel`, forwards
    its edits, and `Sync`s it on every broadcast (re-texting when the language changed). The view model stays pure,
    so its unit tests did not change; `Mvs.Settings.Model` covers the subsystem side. The functional tests' rig reads
    the model, and previews through it when no settings screen is open.
  - Theme passed in [13]: screens' and overlays' `ApplyTheme` receive the theme, resolved once from the settings
    broadcast; `UMvsButton` keeps its theme from the last change, so `ApplyState` (every focus, hover and press) makes
    no lookups. `MvsStyle::Token(Context)` stays for one-off styling and says what each call costs.
  - Button metrics [12]: the menu item, tab and button paddings and corners are in `MvsMetrics`. The other half of the
    finding (an empty `UMvsButtonStyle`, Common UI's style assets bypassed) stays as ADR 0002 decided: code-built.
  - Reflection [16]: the three engine-property lookups are in `Core/MvsEngineProperties.h` and raise an ensure when a
    property is gone or retyped; `DisableTick` also ensures on an unexpected property type. `Mvs.Engine.Properties`
    checks all three.
  - A HUD precondition (the first gadget ready) was seen false once right after the gadget wheel test, with no gadget
    used; it is now waited for (up to 3 s) instead of checked in one frame. The cause was not found.
  - 64 tests (57 editor, 7 functional).
  - Gate: G4 failed once on `hud-wheel` (5.1%): the wheel had followed the real cursor, which the screenshot runs'
    `-MvsIgnoreHover` did not stop [31]. Fixed in `SGadgetWheel`; G4 then passed.
- **S5 done** (finding 14):
  - `USettingRowViewModel`, one per option, owned by `USettingsViewModel` (`GetRow`): label, value text, description,
    choice index, choice count and whether it wraps, each field notify, and `Step`. The coarse `Revision` is gone.
  - Texts notify when they would read differently (formatted values are new `FText`s every time, so "identical" would
    notify on every update), and all of them on a language refresh (a localized text is the same object in every
    language, so comparing would miss the switch).
  - `UMvsOptionRow` binds only its row's fields; the settings screen binds only `bIsDirty`, and its detail pane reads
    the focused row's view model. `Mvs.Settings.Rows` replaces `Mvs.Settings.Revision`: a step notifies only the row it
    touched, and only the fields that changed. 64 tests.
  - `UControlsViewModel` keeps its own `Revision`: finding 14 is about settings.
- **S6 done** (finding 11):
  - `WBP_PauseMenu` (built in the editor by the project owner) authors the pause status panel's content and fills it
    with MVVM View Bindings through `UMvsViewModelResolver`; it is the configured `PauseMenuClass`. The frame, menu and
    panel stay code-built, so the screenshots stay pixel-identical: `pause` and `pause-quit` 0.00%.
  - C++ side: `UMvsText` designer settings (Mvs: Style, Color, opacity), `UClueListViewModel::ProgressText`, the pause
    screen takes a designer tree as its status content, `BindWidgetOptional` captions filled from the game's
    translation keys, and `UMvsTestDesignerPause` for the tests. Steps in `Docs/DesignerGuide.md`.
  - The gate found three things: the perf worktree did not see the untracked asset (commit before G5), the screenshot
    dev aid opened pause before the Widget Blueprint had preloaded (it now finishes the preload first), and two Details
    settings in the asset (an evidence Style and Color, two paddings).
  - Checked with G1 (incremental), G2 (64), G4, G6 and G7, and one harness session instead of G5 (pause-quit 0.073 ms of
    UI, budget 0.15 ms). ADR 0008 has the cost and benefit notes.
- **S7 done** (findings 23 to 26, 29; numbers in `Docs/Performance.md`, "Second review S7"):
  - Case file: the scrolling cost is the engine tile view re-adding every visible tile each frame the offset moves
    (0.55 ms scrolling, 0.13 ms still). Tiles are now one widget each (`SClueTile`) and not hit-testable inside, which
    cut it from about 0.60 to 0.55 ms, pixel-identical. The 0.3 ms target needs a custom grid instead of `UTileView`:
    left open as a decision.
  - World overlays repaint only when their items change or they animate [29], with a test.
  - Findings 24 and 25 do not show in the capture, so they are not done; 26 was out of date (the components already
    stop ticking when idle).
- **S8 done** (finding 22): one test count in the README (65: 58 editor, 7 functional); `ProjectPlan`,
  `RefactoringPlan`, `RefactoringProposal`, `ReviewFixPlan` and `VisualPlan` are in `Docs/History/` (with an index),
  and every path to them is updated; no other stale paths found in the docs, source or scripts.

**Open after the second review:**
- (Decided after S8) The case file keeps `UTileView`: continuous scrolling costs about 0.55 ms (budget 0.75 ms), and
  browsing it the way a player does costs about 0.06 ms, measured by the new `case-file-browse` scenario (budget
  0.15 ms). A custom grid for the 0.3 ms target is not worth re-doing the tile view's input handling.
- (Resolved after S8) The pause-quit user-widget count "61 against 48" was a wrong comparison: 48 came from the S6 gate
  runs where pause never opened. Every valid run shows 61, and both pause classes give the same widgets class for class.
- (Closed after S8) Navigation in a windowed game searching at an offset equal to the window's position (finding 4):
  a known engine issue in windowed mode, according to the project owner; fullscreen is not affected. Not verified here.
- **Gate skip rule** (after S1): G4 and G5 run only when something that can change their result differs from master
  (G5: the game's code, config, content, project file; G4: those plus the screenshot baselines and capture scripts;
  test-only code counts for neither). The gate prints the decision and the files behind it first. From the measured
  noise, G5 stays at three order-balanced pairs (fewer either fails clean code too often or missed the injected
  0.1 ms), so a full gate is ~30 minutes, ~6 without G5 and ~2 without G4 as well.

## Rules

- Same as the first plan: each iteration merges on its own through the full gate, and every fix comes with a test or
  rule that fails before it.
- Measure before optimising: an iteration on performance starts from a profile, not from a guess.

## S0: Measure and add the missing checks (half a day) [19, 20, 23]

- One Unreal Insights capture of `hud-idle` and `hud-animating` (Slate and game-thread channels). Write down what
  invalidates or paints every frame. That list decides S7.
- G5 also checks an absolute budget per scenario (UI cost over `no-ui`), written in `Docs/Performance.md`, so small
  regressions cannot add up across iterations. Start from today's numbers plus a margin.
- A translation check in the gate: every `msgid` in the English `.po` has a non-empty `msgstr` in de and ja. R8's two
  new strings were caught by hand.

## S1: Player-facing fixes (half to one day) [1, 2, 3]

- **Gamepad key hints [1]:** the HUD's gadget hints follow the input device in use (keyboard slot or gamepad slot) and
  refresh when it changes, like the menu prompts do. New menu-input rule: a gamepad press turns the hints into face
  buttons.
- **Wheel colours [2]:** label, hub and outline colours come from palette tokens, so high contrast and the colour modes
  apply. Check with a `-MvsHighContrast -MvsOpenWheel` screenshot.
- **Subtitle timer [3]:** decide whether a subtitle should wait out a pause. If so, move the timer off real time.

## S2: Shipping hygiene (half a day) [5, 6, 7, 8]

- Move `FMvsMenuInputTest` into `MVVMSampleTests`. The dev aid `-MvsMenuInputTest` can call it through the test module
  in Development builds. Guard `FMvsScript` with `!UE_BUILD_SHIPPING` like the other dev aids.
- Bind the debug actions (F1 damage, F2 heal, F3 attack) only outside Shipping, and the same for `AddDebugClues`.
- Check: a Shipping build whose binary contains none of the rule strings (`Mvs.Functional`, "review 39") and no
  debug actions.
- Tidy `MVVMSample.Build.cs`: remove the template comments, make RHI a dependency of non-Shipping builds only, and
  decide on `PublicIncludePaths` (everything is public today).

## S3: Tests that fail one at a time (one day) [17, 18, 21]

- Split `Mvs.Functional.MenuInput` into one functional test per screen (pause, settings, controls, case file,
  confirmations, HUD), sharing the helpers. One bug then fails one test, not the rest of the script after it (in R8,
  one bug gave seven failures).
- Investigate removing the "game window must be the active application" precondition: drive the clicks through the
  viewport's widget path, or activate the window from the test. It failed the gate more than once when the PC was in use.
  Keep the precondition if no clean way exists.
- A test for `UMvsViewModelResolver`: given a local player, it returns that player's view model for each class.

## S4: Architecture (one and a half days) [9, 10, 12, 13, 16]

- **Screen shortcuts as data [9]:** `UMvsUISettings` lists which gameplay action opens which screen on which layer. The
  base screen and the UI subsystem iterate that list, which replaces the hard-coded case-file key in `UMvsScreen` and
  `ToggleClueLog` / `OpenGadgetWheel` in the subsystem. Menu-input rules 7 and 11 guard it.
- **Settings ownership [10]:** the settings subsystem keeps the data and applies it (preview, commit, revert as calls).
  The settings screen creates its own view model over it. Today the subsystem creates the view model and listens to
  its events, so the model depends on the view model.
- **Theme passed in [13]:** `ApplyState` and `ApplyTheme` take one `FMvsTheme`, resolved once per change, instead of
  each colour lookup walking world, game instance and subsystem.
- **Button metrics [12]:** the paddings and corner sizes in `UMvsButton::ApplyKind` move into `MvsMetrics`, which R6
  did not reach.
- **Reflection exceptions [16]:** each of the three writes into engine internals fails loudly (an `ensure` plus a test)
  when the property is missing after an engine upgrade. Today `DisableTick` returns silently.

## S5: Settings rows as view models (one day) [14]

- One row view model per option (label, value text, description, choice index, choice count, all field notify),
  owned by the settings view model. The option row binds to its row; the coarse `Revision` field goes.
- This gives designer-built screens something to bind per option, and it is the prerequisite for S6 on the settings
  screen.

## S6: Prove the designer path (one to two days, needs the editor) [11]

- Build `WBP_PauseMenu` in the editor, parented to `UPauseMenuScreen`. Its widgets use `BindWidgetOptional`, and the
  objective panel's text comes from MVVM View Bindings through `UMvsViewModelResolver`. Point
  `UMvsUISettings::PauseMenuClass` at it.
- The `pause` and `pause-quit` screenshots must stay pixel-identical; that is the check.
- Write down in ADR 0008 what the plugin's bindings cost and save compared with `MvsMVVM::Bind`, from this one screen.
- This is designer work in the editor: I can prepare the C++ side and the steps, but someone has to author the asset.

## S7: Performance, from the S0 profile (one day) [23 to 26, 29]

- **The case file first:** +0.55 to +0.65 ms while scrolling 505 clues, over the 0.3 ms target. The profile points at
  the tile view rebuilding rows every frame of the scroll, the tiles' own paint, and their ticking. Try fewer, cheaper
  tiles (a retainer or cached panel per tile, no per-tile tick), and check how far the scenario's scroll speed is from
  real use.
- **The threat layer [29]:** repaint only when an indicator changed, not every frame while a thug exists.
- Small items, done only if they show in the capture:
  - the threat subsystem allocates three arrays a frame and copies the snapshot list into the view model [24]
  - the wheel's `OnPaint` allocates vertex arrays per sector while open [25]
  - gameplay components tick while idle (gadget cooldowns, combo, forensic transition) [26]
- Numbers before and after go into `Docs/Performance.md`, against the S0 budgets.

## S8: Docs (two hours) [22]

- README: one correct test count (64 since S4: 57 in the editor pass, 7 functional).
- Move the process documents (`ProjectPlan`, `RefactoringPlan`, `RefactoringProposal`, `ReviewFixPlan`, `VisualPlan`)
  into `Docs/History/`, so `Docs/` shows the architecture, ADRs, standards, performance and designer guide first.
- Fix stale paths, e.g. `MvsScript.h` still points at `Tests/ScriptTests.cpp`.

## Timeline

| Iteration | Size | Depends on |
|---|---|---|
| S0 | half a day | none |
| S1 | half to one day | none |
| S2 | half a day | none |
| S3 | one day | S2 (the test moves module) |
| S4 | one and a half days | S3 (rules split per screen) |
| S5 | one day | S4 (settings ownership) |
| S6 | one to two days, editor | S5 for the settings screen; pause can go after S4 |
| S7 | one day | S0 |
| S8 | two hours | last |

About 7 to 9 days. S0, S1 and S2 are cheap and independent, so they go first.

## Findings

**Player-facing**

1. HUD gadget key hints read only the keyboard slot (`UMvsGadgetBinder::RefreshHotkeys`, `EPlayerMappableKeySlot::First`),
   so a gamepad player sees "1 2 3". *Checked in code.*
2. The gadget wheel's label, hub and outline colours are literals (`FLinearColor::White`, a fixed grey) in
   `SGadgetWheel::OnPaint`, so high contrast and the colour modes do not reach them. *Checked in code.*
3. The "clue found" subtitle hides on a real-time core ticker, so it can run out while the game is paused.
   *Checked in code; may be intended.*
4. Arrow and d-pad navigation in a windowed game away from the desktop origin searched at an offset equal to the
   window's position (seen in R8, `Docs/History/ReviewFixPlan.md`). *Not reproduced in fullscreen yet; may be the engine.*

**Shipping hygiene**

5. `Core/MvsMenuInputTest.cpp` and `Core/MvsScript.cpp` have no `!UE_BUILD_SHIPPING` guard (the dev aids and perf
   harness do), so the menu-input test is compiled into Shipping. *Checked in code. Corrected in S2: compiled, but
   unreferenced in Shipping, so the linker dropped it; the binary did not contain it.*
6. The debug actions F1 (damage), F2 (heal) and F3 (attack) are mapped and bound in every build
   (`MvsActionTable.cpp`, `AMvsPlayerController::BuildInputAssets`). *Checked in code.*
7. `AddDebugClues` (view-model subsystem and clue binder) ships too. *Checked in code. Corrected in S2: compiled
   for Shipping but dropped by the linker, as in 5.*
8. `MVVMSample.Build.cs` keeps the template comments and links RHI and RenderCore for the perf harness alone, and
   `PublicIncludePaths.Add(ModuleDirectory)` makes every header public. *Checked in code.*

**Architecture**

9. `UMvsScreen` hard-codes the case-file key (`"ClueLog"`) and `UMvsUISubsystem` has feature methods
   (`ToggleClueLog`, `OpenGadgetWheel`, `TogglePauseMenu`): the generic layer knows the features. *Checked in code.*
10. `UMvsSettingsSubsystem` creates `USettingsViewModel` and acts on its `OnPreview` / `OnCommitted`, so the model
    depends on its view model. *Checked in code.*
11. No screen uses a `WBP_` with MVVM View Bindings, and `UMvsViewModelResolver` has no test and no user. ADRs 0002
    and 0008 defer the designer path to "when a layout next needs rework". *Checked in code.*
12. `UMvsButton` bypasses Common UI's style assets (an empty `UMvsButtonStyle`) and keeps hard-coded paddings and corner
    sizes in `ApplyKind`, outside `MvsMetrics`. *Checked in code.*
13. Each palette lookup (`MvsStyle::Token`) resolves the theme through world, game instance and subsystem. A button's
    `ApplyState` does that about five times per state change. *Checked in code; cost not measured.*
14. `USettingsViewModel` exposes one coarse `Revision` field: views re-read everything on any change, and nothing per
    option is bindable. *Checked in code.*
15. Binders bind to the concrete `AMvsCharacter`, and the view-model subsystem keeps eight typed getters next to
    `Get<T>()`. *Checked in code; low priority.*
16. Three reflection writes into engine internals (`DisableTick`, the action bar's button class, Enhanced Input's
    `PlayerMappableKeySettings`) are documented, but `DisableTick` returns silently if the property disappears.
    *Checked in code.*

**Tests and tooling**

17. `Mvs.Functional.MenuInput` is one 750-line script: a failure cascades into the rules after it, and one rule
    cannot run alone. *Seen in R8.*
18. The functional test needs the game window to be the active application, and failed the gate more than once while
    the PC was in use. *Seen in R6 and R7.*
19. G5 is relative only (+0.05 ms against the last merge), so regressions under that can add up across iterations.
    *Checked in `Scripts/Verify.ps1`.*
20. Nothing checks that every string is translated; R8's two new strings were found by hand. *Seen in R8.*
21. `UMvsViewModelResolver` is untested (see 11).
22. The README gives three different test counts (54, 45; there are 55), five process documents sit next to the
    architecture docs, and `MvsScript.h` points at a test path that moved in R7. *Checked.*

**Performance** (baseline: R5 table in `Docs/Performance.md`, idle HUD +0.185 ms game thread)

23. The idle HUD costs about 0.19 ms of game thread with global invalidation on, and that has never been broken down
    per widget. *Measured total; per-widget split needs S0.*
24. `UMvsThreatSubsystem::Tick` builds three arrays a frame, and `UThreatViewModel::SetThreats` copies the snapshot
    list every frame. *Checked in code; cost not measured.*
25. `SGadgetWheel::OnPaint` allocates two arrays per sector per frame while the wheel is open (R5 fixed the panels and
    arcs, not the wheel). *Checked in code.*
26. Gadget, combo and forensic components tick every frame even when idle. *Checked in code; cost not measured.*

**Found in S0**

27. The perf harness measured scenarios that were not in their state: `case-file-505` never opened the case file
    until R4 (10 widgets), `pause-quit` never opened pause (a toggle popped settings again), and a paused world was
    compared with a running one. The V4 to R0 case-file numbers and R5's pause-quit numbers did not measure those
    screens. *Measured; fixed in S0.*
28. The R1 plan says the UI subsystem owns pause; the pause screen still pauses and unpauses the game itself (R1 fixed
    the bug through covered-versus-closed instead). Settings or the case file opened without pause do not pause.
    *Checked in code; the plan's wording, not a bug.*
29. The threat indicator layer runs an active timer and repaints every frame whenever a thug exists, even when nothing
    moved. *Measured: the largest Slate item of the idle HUD.*
30. Settings reopened from pause have no tabs: closing them back onto pause destructs the tab list, and Common UI's
    tab list removes its tabs on destruct, while the screen registered them only once. Q / E and the tab prompts did
    nothing. *Found by S3's split; fixed in S3.*
31. The gadget wheel's segments follow the mouse even under `-MvsIgnoreHover`, so the `hud-wheel` screenshot depends on
    where the real cursor rests when the game window opens (S4's gate showed the top segment hovered instead of the
    staged stick choice). *Found by S4's gate; fixed in S4.*
