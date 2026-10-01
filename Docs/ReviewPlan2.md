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

## S7: Performance, from the S0 profile (one day) [23 to 26]

- Fix what S0's capture shows paints or invalidates the idle HUD every frame.
- Small items, done only if they show in the capture:
  - the threat subsystem allocates three arrays a frame and copies the snapshot list into the view model [24]
  - the wheel's `OnPaint` allocates vertex arrays per sector while open [25]
  - gameplay components tick while idle (gadget cooldowns, combo, forensic transition) [26]
- Numbers before and after go into `Docs/Performance.md`, against the S0 budgets.

## S8: Docs (two hours) [22]

- README: one correct test count (55 today: 54 in the editor pass, 1 functional).
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
   window's position (seen in R8, `Docs/ReviewFixPlan.md`). *Not reproduced in fullscreen yet; may be the engine.*

**Shipping hygiene**

5. `Core/MvsMenuInputTest.cpp` and `Core/MvsScript.cpp` have no `!UE_BUILD_SHIPPING` guard (the dev aids and perf
   harness do), so the menu-input test is compiled into Shipping. *Checked in code.*
6. The debug actions F1 (damage), F2 (heal) and F3 (attack) are mapped and bound in every build
   (`MvsActionTable.cpp`, `AMvsPlayerController::BuildInputAssets`). *Checked in code.*
7. `AddDebugClues` (view-model subsystem and clue binder) ships too. *Checked in code.*
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
