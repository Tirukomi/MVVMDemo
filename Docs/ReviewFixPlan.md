# Review fix plan

A code review of the finished refactoring (2026-09-30), read as a senior UI engineer would read it, found 39 things
that look questionable. This plan fixes them in nine iterations. Numbers in brackets refer to the findings listed at
the end.

## Status (2026-10-01)

- **R0 done** except the packaged Shipping build check, which is still open. The GPU column is in the perf report;
  the baseline numbers are not written up in `Docs/Performance.md` yet.
- **R1 done:** findings 1 to 10 fixed, each guarded by a rule in `Gotham.Functional.MenuInput` (or
  `Gotham.Input.KeyLabels` for 5). Two more found and fixed on the way:
  - opening a screen over another left focus on the game viewport until the next key press (Common UI's router
    focuses through the local player's pending Slate operations); screens now focus their target directly
  - reopening Settings after changing tab put focus on a row of the hidden page (focus stayed on the viewport);
    Settings now focuses the page that is showing
  - German and Japanese translations for the key labels; the German and pseudo-locale screenshot baselines were
    refreshed for them.
- Merged after repeated test runs (both passes green three times in a row), without the full gate.
- **R2 done** (ADR 0007), findings 11 to 16:
  - Common UI's Enhanced Input support is on. The menu actions (accept, back, tabs) are code-built input data,
    `UGothamUIInputData`; their mapping context and the gameplay one stay on, and the context swap is gone.
  - Back is the activatable back handler. Toggle keys, the case file key, the wheel's press / release and the tab keys
    are Common UI bindings, so the key-profile lookup and every raw key check in the screens are gone.
  - Prompts are a bound action bar. Clicking "Select" clicks the focused item; no key is injected.
  - Glyphs resolve keys through Common UI and refresh when Enhanced Input rebuilds its mappings; the retry loop is gone.
    Key caps stay localized text, not Common Input icon brushes: there is no icon art.
  - The HUD is a plain activatable widget.
  - New rules: Esc closes only the top screen, gamepad Back closes the case file, the prompts switch to gamepad keys and
    back with the device. Under a modal, the screen behind no longer shows its prompts (`pause-quit` baseline refreshed).
- **R3 done** (ADR 0008), findings 17 to 20:
  - The view-model subsystem holds one binder per feature (vitals, gadgets, combo, detective, threats, clues). Each owns
    its view models and subscribes through `FGothamSubscriptions`, which cleans up after itself; the 11 delegate
    handles are gone.
  - The wheel uses gadgets through `UGadgetBarViewModel::RequestUse`; the gadget binder hands it to the character.
  - The controls view model applies rebinds and resets itself through `IGothamBindingStore`; `OnChangesPlanned` is
    gone, and the test uses a store in memory.
  - The glyph and screens ask `IGothamActionSource` for actions instead of casting to the player controller.
  - Manual view subscriptions are kept on purpose until screens move to UMG (ADR 0008).
- **R4 done**, findings 21 to 23:
  - UI scale is an `SDPIScaler` in the primary layout; `UUserInterfaceSettings`' class default object is never written.
    World overlays project through their own geometry, so they stay aligned at any scale.
  - The culture the game started with is restored when the settings subsystem shuts down; revert and closing settings
    already restored a previewed language, now guarded by a rule.
  - Screen classes and the case file's entry class load asynchronously with the layout; a late load logs a warning
    (which G6 fails on).
  - `DisableTick`: no public engine API (`TickFrequency` is private, the `DisableNativeTick` flag is Blueprint-only), so
    it is the second of three documented reflection exceptions in `Docs/CodingStandard.md`.

## Rules for every iteration

- Each iteration merges on its own, through the full gate (`Scripts/Verify.ps1`).
- Every bug fix comes with a test or menu-input rule that fails before the fix and passes after it. Nothing is fixed
  only on paper.
- Player-facing bugs go first. The large structural changes (R2, R3) run on top of R0's safety net. Naming and
  hygiene go last, where they conflict with nothing.

## Decisions

| Question | Decision |
|---|---|
| Naming [36, 37] | The module stays `MVVMSample`. The product is named "MVVM Sample" in the UI. The code prefix becomes `Mvs` (see R8). |
| Toggle keys [7] | A screen's own key closes only that screen. Another screen's key opens that screen on top. |
| Copyright holder [35] | IG: every file's header becomes `// Copyright IG. All Rights Reserved.` |
| Module split [38] | Yes: tests move to their own module (R7). |

## R0: Safety net and baselines (half a day)

- Add a `WaitUntil(predicate, timeout)` step to `FGothamScript`. Rewrite the menu input test to wait for events (a
  screen activated, focus arrived) instead of fixed times [33].
- Add menu-input rules for bugs 1, 2, 3 and 7, marked as expected failures, plus tests for the rest of R1 wherever a
  rule can express them.
- Take two baselines that have never been taken:
  - a packaged Shipping build, checking fonts, `.locres` files and UI scale [32]
  - a GPU measurement of the menus and HUD (`stat gpu` or Unreal Insights), so R5 has a before number [26]

## R1: Player-facing bugs (1 to 1.5 days) [1–10]

- **Covered versus closed [1]:** screens tell being covered by another screen apart from being removed from the stack.
  Settings reverts unapplied changes only when it is actually closed.
- **Pause and time scale have one owner each [2, 6]:**
  - Pause: the game is paused while any menu-layer screen is open, owned by the UI subsystem rather than by the pause
    screen.
  - Time: a small world-level arbiter takes time-scale requests (the wheel's slow motion, hit-stop) and resolves them,
    which fixes the wheel / hit-stop race and removes the file-static state in `GothamFeel`.
- **Keys come from bindings [3, 4]:** the wheel's open key comes from the `GadgetWheel` action's current bindings, and
  the HUD's gadget key hints come from the live bindings.
- **Key labels [5]:** localized, with a glyph set per platform (Xbox, PlayStation, Switch) picked from the controller.
- **Toggle keys [7]:** a screen's own key closes only that screen; another screen's key opens that screen on top.
- **Small fixes:**
  - the view-model resolver includes the subtitles view model [8]
  - the case file compares list identity, not count [9]
  - the wheel follows reduced motion live [10]

## R2: Adopt Common UI input (2 days, the riskiest step) [11–16]

- **Back and accept [11]:** Common UI actions (the back handler, a UI action data table) replace the raw key checks,
  so the platform accept / back swap and rebinding are handled.
- **Prompts [12, 13]:** the hint bar becomes a bound action bar. Prompts come from registered actions, stay clickable
  and match the current device. Clicking the accept prompt no longer injects an Enter key.
- **One input system [14]:** Common UI's Enhanced Input support replaces the manual mapping-context swap in the player
  controller. Toggle keys become action bindings, which removes the key-profile lookup in `UGothamScreen`.
- **Glyphs [15]:** Common UI's controller data and icons replace the custom label table and its retry loop.
- **HUD [16]:** a plain activatable widget, no longer derived from the menu screen base.
- **Guarded by:** R0's event-driven menu input test, which must cover every rule before this starts.

## R3: MVVM boundaries (1.5 days) [17–20]

- **Commands on view models [18]:** the wheel asks the gadget-bar view model to use a gadget instead of calling the
  character.
- **Controls [19]:** the controls view model applies rebinds itself through a small input-settings interface; the
  screen only shows results.
- **Split the view-model subsystem [20]:** one binder per feature (vitals, gadgets, combo, detective, threats, clues),
  each subscribing through a self-cleaning listener, like `FGothamSettingsListener`.
- **Glyph [18]:** it asks an interface for keys instead of casting to the concrete player controller.
- **An ADR for manual bindings [17]:** the MVVM plugin's declarative bindings need designer assets, so they come with
  the static screens that move to UMG (the path in ADR 0002's amendment). Until then, subscriptions stay manual on
  purpose.

## R4: Global state and loading (1 day) [21–23]

- **UI scale [21]:** applied as a DPI scale at the primary layout instead of on the `UUserInterfaceSettings` class
  default object, so it no longer leaks into the editor after a play-in-editor session.
- **Language preview [21]:** stays live, but the culture is restored when a preview is reverted or the screen closes.
- **Loading [23]:** screen classes load asynchronously when the layout is created, so no key press triggers a
  synchronous load.
- **`DisableTick` [22]:** replaced by a public engine API if one exists. Otherwise it becomes the second documented
  exception in `Docs/CodingStandard.md`.

## R5: Performance (1 day, measured against the R0 GPU baseline) [24–27]

- **Detective overlay [24]:** a fast path for the fade. The font is applied once; only alpha and material parameters
  change per frame.
- **Allocation-free paint [25]:**
  - clue marker text is re-formatted only when the rounded distance changes
  - gadget icon strokes are static data
  - panel outlines and arcs reuse their point arrays
- **Blur [26]:** one blur per layer instead of one per menu, so Settings over Pause does not stack two.
- **Tweens [27]:** Slate curve sequences instead of the core ticker and its static map.
- **Numbers:** perf and GPU, before and after, go in `Docs/Performance.md`.

## R6: Style system, layout metrics, accessibility (2 days) [28–31]

- **One style source [28]:** colours and named metrics (paddings, widths, radii) move into a style asset or Slate
  style set. Styled widgets restyle themselves from it, which retires the six hand-written recolour methods and the
  parallel arrays in `UGothamScreen`.
- **Metrics [29]:** the magic numbers become named metrics, high contrast is read directly instead of inferred from
  panel alpha, and the menu input test reads the selector width from the same metric.
- **Safe zones [30]:** the HUD and menus sit inside a safe zone, checked with the engine's debug safe-zone commands.
- **Accessibility [31]:**
  - accessible text on every interactive widget and on the custom Slate layers
  - a text size option separate from UI scale

## R7: Platform and tooling (1 day) [32, 34, 38]

- **Packaged build [32]:** repeat R0's Shipping check after R4 to R6.
- **Script paths [34]:** every script reads the engine and project roots from one config file, with today's paths as
  defaults.
- **Test module [38]:** the automation tests move from `Source/MVVMSample/Tests` into their own module,
  `MVVMSampleTests` (loaded in the editor and in development game builds, never in Shipping). The game module keeps no
  test code, and tests reach internals only through exported APIs or explicit test-access friends
  (`FGothamInputTestAccess` today). `run_tests.py` and the gate run unchanged against it. The dev aids (dev flags,
  perf harness, script runner) stay in the game module, compiled out of Shipping as now.

## R8: Naming and hygiene (1 day, last) [35–37, 39]

- **Prefix `Gotham` becomes `Mvs` [36, 37]:**
  - Classes and structs: `UGothamButton` becomes `UMvsButton`, `FGothamSettingsData` becomes `FMvsSettingsData`, and
    so on. Files follow.
  - Class renames get `CoreRedirects`, so `WBP_ClueEntry`, data assets and `DefaultGame.ini` keep loading.
  - Log categories (`LogGotham*` becomes `LogMvs*`) and G6's pattern change together.
  - Dev flags (`-Gotham*` becomes `-Mvs*`), with `Verify.ps1`, `CaptureScreens.ps1` and the README updated in the
    same change.
  - Test paths (`Gotham.*` becomes `Mvs.*`), with `run_tests.py`'s filters.
  - Saved settings: the config section `/Script/MVVMSample.GothamSettings` gets a one-time migration to the new
    section name, so existing players keep their settings.
  - Localization namespaces (`Gotham.*`): renamed together with the `msgctxt` lines in every `.po` file, and checked
    with the byte comparison from P8, so no translation is lost.
- **IP-adjacent names [37]:** "Detective Mode", "WingBlade" and the emblem comments get original names. The product
  name in the UI ("Blackwater Ops") becomes "MVVM Sample".
- **Copyright [35]:** replace the Epic Games template header on every file (source, `Build.cs`, target files and
  scripts) with `// Copyright IG. All Rights Reserved.`
- **Leftovers [39]:**
  - null checks for `GetLayer(...)` and `Pages[0]->GetItems()[0]`
  - a confirmation before resetting all controls
  - the stale "later" comment in `EGothamUILayer`
  - `.po` files that no longer change on every `Localize.bat` run

## Timeline

| Iteration | Size | Depends on |
|---|---|---|
| R0 | half a day | none |
| R1 | 1 to 1.5 days | R0 |
| R2 | 2 days | R0 |
| R3 | 1.5 days | R2 (same screens) |
| R4 | 1 day | none |
| R5 | 1 day | R0 (GPU baseline) |
| R6 | 2 days | R2 |
| R7 | 1 day | R4 to R6 |
| R8 | 1 day | everything else |

About 10 to 12 days in total. After R0: R1 first, then R2 and R3 together.

## Appendix: review findings

**Likely bugs**
1. Opening Key bindings reverts unapplied settings (pushing a screen deactivates the one underneath).
2. The game unpauses behind Settings and the case file when they are opened from Pause.
3. The gadget wheel's open key is hard-coded (Q / LB), so rebinding breaks hold mode.
4. HUD gadget key hints are hard-coded "1", "2", "3".
5. Key labels are English only (`FText::FromString`), and gamepad labels are Xbox only.
6. Opening the wheel during a hit-stop cancels the wheel's slow motion; hit-stop state is a file-static global.
7. A screen's toggle key closes whatever screen is on top.
8. The view-model resolver cannot find the subtitles view model.
9. The case file list rebuilds only when the entry count changes.
10. The wheel reads reduced motion only once, at construction.

**Reinventing Common UI**
11. Back and accept are raw key checks in `NativeOnKeyDown`.
12. The prompt bar is hand-built instead of a bound action bar.
13. Clicking the accept prompt injects an Enter key event on the next frame.
14. Two input-mode systems: Common UI input configs plus manual mapping-context swapping.
15. Custom glyphs: a key-name table and a 10-try retry loop.
16. The HUD derives from the menu screen base.

**MVVM and architecture**
17. The MVVM plugin is used without its binding system; every view subscribes by hand.
18. The wheel calls the character directly; the glyph casts to the concrete player controller.
19. The controls view model asks the view to apply rebinds (`OnChangesPlanned`).
20. The view-model subsystem is a god object with 11 hand-tracked delegate handles.
21. Global state changed from UI code: UI scale on a class default object, the global culture during preview, pause
    and time dilation owned by individual screens.
22. `DisableTick` writes a private engine property through reflection, against the coding standard.
23. Screens load synchronously (`LoadSynchronous`) on a key press.

**Performance**
24. The detective overlay re-applies its font every frame of its fade.
25. Per-frame allocation in paint code: marker text, icon strokes, panel outlines and arcs.
26. A full-screen blur per menu, stacking when menus stack; GPU cost never measured.
27. Tweens run on the core ticker with a static global map.

**Styling, layout and platform**
28. Colours are re-applied by hand through six differently named methods and parallel arrays.
29. Magic layout numbers; high contrast inferred from `PanelAlpha > 0.9`; the menu test hard-codes a widget width.
30. No safe-zone handling.
31. No accessible text for screen readers; no text size separate from UI scale (except subtitles).
32. Custom fonts load from raw `.ttf` paths and fall back silently; never checked in a packaged build.

**Tests, tooling and hygiene**
33. The menu input test waits fixed times instead of waiting for events.
34. Seven scripts hard-code the engine and project paths.
35. Every file carries the Epic Games template copyright header.
36. Three names for one project: `MVVMSample`, "Blackwater Ops" and the `Gotham` prefix.
37. IP-adjacent names: `Gotham`, "Detective Mode", "WingBlade", comments about emblems.
38. One module holds the game, UI, dev tools and tests.
39. Smaller oddities: unchecked `GetLayer(...)` and `Pages[0]->GetItems()[0]`, no confirmation on resetting controls,
    a stale enum comment, `.po` date headers that change on every localization run.
