# Blackwater Ops — UI Showcase Project Plan

A portfolio project targeting a **Senior UI Programmer** role on AAA titles. The product is a
production-style UI layer for a small third-person combat/detective sandbox in **Unreal Engine 5.8**.
Gameplay is deliberately thin; the UI architecture, tooling and polish are the deliverable.

> Original placeholder branding only. No third-party (WB/DC/Rocksteady) assets or trademarks.

---

## 1. Goals

| Goal | Evidence in the finished project |
|---|---|
| Advanced C++ | All view models, components, subsystems and custom widgets in C++ |
| MVVM at scale | UE MVVM plugin, field-notify view models, one-way data flow, tests |
| UMG + Common UI | Activatable widget stack, input routing, gamepad/KBM parity |
| Enhanced Input | Context-per-layer, rebinding, device-aware glyphs |
| Slate | Custom radial gadget wheel + combo meter, wrapped as `UWidget` |
| Shaders / materials | Scanner UI materials, Detective Mode post-process |
| Accessibility + localization | Colour-blind modes, text scale, reduced motion, 2+ languages incl. expansion stress test |
| Performance / memory | Budgets, Slate Insights captures, pooling, before/after numbers |
| Engineering standards | Automation tests, coding standard, review-style commit history, designer docs |

## 2. Architecture

```
Gameplay (C++ components / subsystems)      <- knows nothing about UI
        │ state changes (delegates)
        ▼
ViewModel layer (UMVVMViewModelBase, C++)   <- field-notify, testable without a world
        │ MVVM bindings
        ▼
View layer (CommonActivatableWidget, UMG, custom Slate)   <- no gameplay logic
```

Principles:

1. **One-way flow.** Gameplay -> ViewModel -> View. User intent returns via commands/events (input actions,
   `UFUNCTION` view-model commands), never by widgets mutating gameplay state directly.
2. **View models are plain C++ objects** with no widget or world dependency, so automation tests run headless.
3. **`UGothamViewModelSubsystem` (`ULocalPlayerSubsystem`)** owns and resolves view models per local player
   (split-screen safe). Widgets get them via the MVVM view-model resolver.
4. **Layered UI** via Common UI primary game layout: `Game`, `GameMenu`, `Menu`, `Modal` stacks. Screens are
   `UCommonActivatableWidget`s pushed by gameplay tag.
5. **Data-driven** content (gadgets, clues, accessibility presets) in `UPrimaryDataAsset` / data tables,
   loaded asynchronously.
6. **Designer-friendly.** Artists tweak widgets, styles and animations in the editor without touching C++.

### Module layout

```
Source/MVVMSample/
  Core/          GameMode, Character, PlayerController, GameInstance
  Gameplay/      HealthComponent, GadgetComponent, ComboComponent, ClueComponent
  ViewModels/    PlayerVitals, GadgetBar, GadgetSlot, Combo, Objectives, Clues, Settings
  UI/
    Layout/      PrimaryGameLayout, UI manager subsystem, screen tags
    Screens/     HUD, PauseMenu, Settings, GadgetWheelScreen, Detective screens
    Widgets/     Reusable C++ base widgets, styled buttons, list-entry widgets
    Slate/       SRadialMenu, SComboMeter (+ UWidget wrappers)
  Input/         Input action assets setup, glyph subsystem, rebinding
  Accessibility/ Presets, settings subsystem
  Tests/         Automation specs
Content/UI/      Widgets, Materials, Styles, Fonts, StringTables, Data
Docs/            This plan, ADRs, README assets
```

## 3. Milestones

Effort is rough part-time estimates. Each milestone ends with a demoable build, a captured GIF/clip, and a
short README/ADR update so the portfolio write-up grows as work proceeds.

### M0 — Project baseline (0.5 week)

- Strip heavy renderer settings (ray tracing, Substrate, Lumen, VSM) for fast iteration. **Done.**
- Enable plugins: `ModelViewViewModel`, `CommonUI`, `CommonInput`. Add modules: `UMG`, `Slate`, `SlateCore`,
  `ModelViewViewModel`, `CommonUI`, `CommonInput`, `GameplayTags`, `DeveloperSettings`.
- Create `L_Arena` (grey-box arena, player start, lighting) and set as default map.
- Set up folder conventions, naming standard, `Docs/CodingStandard.md`, `.gitignore`, and initialise git.

**Exit:** project builds, opens on `L_Arena`, repo initialised.

### M1 — Foundation + MVVM combat HUD (1–1.5 weeks)

Deliverable: playable map where damage, gadget use and hits update the HUD **purely through view-model bindings**.

1. Core classes: `AGothamGameMode`, `AGothamCharacter`, `AGothamPlayerController`.
2. Gameplay components: `UHealthComponent`, `UGadgetComponent` (3 gadgets, cooldowns), `UComboComponent`
   (multiplier that decays).
3. View models (`UE_MVVM_SET_PROPERTY_VALUE`, field-notify):
   - `UPlayerVitalsViewModel` — health, max, percent, low-health flag.
   - `UGadgetBarViewModel` / `UGadgetSlotViewModel` — icon, cooldown remaining/percent, ready.
   - `UComboViewModel` — count, multiplier, decay percent.
4. `UGothamViewModelSubsystem` creating/registering view models per local player and wiring them to components.
5. `WBP_CombatHUD` (C++ base class + UMG layout) with MVVM bindings authored in the widget editor:
   health bar, gadget bar, combo counter, simple animations.
6. Enhanced Input set up in C++: Input Actions + mapping context for move/look/attack/gadget 1–3.
7. Debug hooks: `F1` random damage, `F2` use gadget, `F3` add combo hit.
8. **Automation test:** damage applied to `UHealthComponent` yields the expected percent and low-health flag on
   the view model, with no world or widget.

**Exit:** builds clean, HUD reflects all three systems, view-model test passes, GIF captured.

**As built (deviations):** HUD widgets are C++-built UMG trees that bind through field-notify delegates rather than
editor-authored MVVM bindings; `UGothamViewModelResolver` is in place so designer-authored `WBP_` widgets with editor
bindings can be added in M2. Debug keys are F1 damage, F2 heal, F3 combo hit. Component tick logic lives in
`Advance()` so tests can drive unregistered components. Known M2 item: CommonUI logs that input routing needs a
`CommonGameViewportClient`.

### M2 — Common UI layer stack, menus, input (1.5 weeks)

1. `UPrimaryGameLayout` with `Game` / `GameMenu` / `Menu` / `Modal` layers; `UGothamUISubsystem` to push/pop
   screens by gameplay tag.
2. Screens: Pause menu, confirmation modal, placeholder Inventory/Settings.
3. Common UI styling: button/text/border styles as data assets, so a re-skin is a content change.
4. Enhanced Input **contexts per UI layer** (gameplay vs menu), input mode switching handled in one place.
5. `UCommonInputSubsystem` integration: mouse/keyboard <-> gamepad detection, **device-aware glyph** widget
   (`UGothamInputGlyph`) driven by a glyph data table.
6. Full gamepad navigation (focus, d-pad, back action, action bar with bound hints).
7. Tests: screen stack push/pop and input-context switching.

**Exit:** every screen fully usable with gamepad or keyboard, glyphs swap live when the device changes.

**As built (deviations):** layers are an `EGothamUILayer` enum rather than gameplay tags. `FGothamUIModeTracker` is a
pure struct that derives the input context from what is open, so the rules are unit-tested without a world. Back/Esc is
handled in `UGothamScreen::NativeOnKeyDown` (Esc / gamepad B) rather than through Common UI's input-data assets, and the
gameplay mapping context is removed while a menu owns input (both replaced in review fix R2, see ADR 0007). `UGothamButton` builds its content before
`UCommonButtonBase::Initialize` because the base only wires its internal button when a root already exists.
Glyphs follow Enhanced Input mappings (`QueryKeysMappedToAction`), so M5 rebinding is reflected automatically.
Dev aid: `-GothamOpenPause` opens the pause menu and saves `Saved/Screenshots/.../gotham_pause.png`.

### M3 — Custom Slate: radial gadget wheel + combo meter (1.5 weeks)

1. `SGadgetWheel` (`SLeafWidget`): custom `OnPaint` with arc segments, hover/selection state, stick and mouse
   hit-testing via angle math, animated selection.
2. `UGadgetWheel` `UWidget` wrapper with exposed properties, style struct, and events so designers can use it in
   UMG; MVVM-bindable item list.
3. Wheel screen: hold-to-open, slow-mo optional, release-to-select; gamepad stick + mouse + number keys.
4. `SComboMeter` custom Slate widget (segmented, animated fill) replacing the M1 UMG version.
5. Performance notes: invalidation behaviour, cached geometry, avoiding per-frame allocations. Slate Insights
   baseline captured.

**Exit:** wheel and meter usable from UMG by a designer; documented paint/hit-test approach.

**As built:** `SGadgetWheel` (`SLeafWidget`) draws ring sectors with `FSlateDrawElement::MakeCustomVerts` on a shared
white brush, so it needs no image assets; hit-testing is `GothamWheel::IndexFromOffset` (pure, unit-tested). Hover
animation and `SComboMeter`'s drain run on active timers that unregister once settled, so an idle wheel or full/empty
meter costs no per-frame work. `UGadgetWheel` / `UComboMeter` are the UMG wrappers (style struct, dynamic events,
`SetItems`). The wheel screen (`Q` / left shoulder to open, release to use, Esc / B cancels, 1-3 pick directly) slows
world time to 0.1x and feeds items from the gadget bar view model. Not yet captured: a Slate Insights baseline
(planned for M6 with the other profiling numbers). Dev aid: `-GothamOpenWheel` opens the wheel, hovers a segment,
builds a combo and saves `gotham_wheel.png`.

### M4 — Detective Mode: shaders, post-process, data-driven clues (2 weeks)

1. **UI materials:** scanline/hologram wipe, animated outline glow, distortion-in transition, driven by
   material parameter collections and dynamic instances from view-model values.
2. **Post-process:** Detective Mode pass (desaturate, edge highlight, custom-depth-based clue highlight).
3. Clue system: `UClueDataAsset`, placed clue actors, `UClueComponent`; scanning reveals clues in the world.
4. View models: `UObjectivesViewModel`, `UClueListViewModel`; UI for objective tracker and clue log using
   virtualised list views with pooled entry widgets (`UListView`).
5. Async widget/asset loading for clue thumbnails; no hitches on entering the mode.
6. Transition polish: mode-enter animation across HUD, materials and post-process together.

**Exit:** toggling Detective Mode is a single coordinated, performant transition; clue list scales to 500+
entries without hitching.

**As built:** `UDetectiveComponent` owns the on/off state and one eased transition alpha; everything else keys off
it. The world side (`UDetectiveVisionComponent`) drives the post-process material and a FOV pinch; the UI side
(`UDetectiveOverlayWidget`) drives a UI material through `UDetectiveViewModel`. Clues are `UClueDataAsset`s placed via
`AClueActor`, which turns on custom depth while highlighted so the post-process draws them through walls (orange =
unscanned, green = scanned, via stencil 1 / 2; `r.CustomDepth=3`). Materials are generated by
`Scripts/CreateDetectiveAssets.py` (which also builds the data assets, places the clues and creates `WBP_ClueEntry`),
so the graphs are reviewable as code. The clue log uses `UListView` (pooled rows) over `UClueEntryViewModel`s; row
thumbnails stream in with `FStreamableManager` only for discovered clues and are cancelled when a row is recycled.
The list view needs a Blueprint entry class in editor builds, hence `WBP_ClueEntry`. Keys: `V` detective, `E` scan,
`J` case file (gamepad D-pad up / right, Select). Dev aids: `-GothamDetective`, `-GothamClueLog[=N]`,
`-GothamShotDelay=S`.

### M5 — Settings, accessibility, localization (1.5 weeks)

1. Settings screen built on a **settings view model** (`UGothamSettingsViewModel`) with apply/revert/defaults.
2. **Input rebinding** (Enhanced Input user settings), conflict detection, gamepad + keyboard.
3. **Accessibility:**
   - colour-blind palettes (data-driven colour tokens applied through styles/materials)
   - text scale, high-contrast mode, reduced motion, hold-vs-toggle, subtitle sizing/background
   - full focus navigation and no colour-only information
4. **Localization:** string tables for all text, `FText` formatting for numbers/plurals, English + one longer
   language (German) + one non-Latin (Japanese) or RTL (Arabic); font fallback set up.
5. Layout stress tests: text-expansion pseudo-locale, ultra-wide/16:9/steam-deck-size screenshots via automation.

**Exit:** language and accessibility changes apply live; screenshot test set passes in all locales.

**As built:**
- **Settings:** `FGothamSettingsData` (pure, persisted in GameUserSettings.ini) is edited through `USettingsViewModel`
  (live preview, Apply / Revert / Defaults); `UGothamSettingsSubsystem` applies side effects (culture, DPI scale via
  `UUserInterfaceSettings::ApplicationScale`) and broadcasts to widgets. Leaving the screen reverts unapplied changes.
- **Accessibility:** colour tokens (`GothamPalette`) resolved per colour-vision preset (Okabe-Ito based) and high
  contrast, used by HUD widgets and the Detective post-process (clue colours are material parameters). Reduced motion
  snaps the wheel and combo meter, freezes overlay scanlines, drops the FOV pinch and shortens the detective
  transition. Gadget wheel supports hold or toggle. Subtitles (`USubtitleViewModel`) follow size and background
  settings. Low health is also spelled out in text, so it never depends on colour alone. Everything is a focusable
  button, so the settings screens are gamepad-navigable.
- **Rebinding:** Enhanced Input user settings. Each rebindable input action carries player-mappable settings (set
  through reflection, since the property is protected); mappings become keyboard / gamepad slots of one row.
  `GothamBindings::PlanRebind` (pure, tested) swaps keys on conflict. Escape / gamepad B are reserved to cancel.
- **Localization:** the standard UE pipeline (`Scripts/Localize.bat`: gather -> `TranslateLocalization.py` -> compile)
  with English, German, Japanese and a generated pseudo-locale (`en-XA`). Clue text lives in a code-registered string
  table (`GothamClues`) so the gatherer finds it. CJK renders through the engine's fallback font.
- **Verified by screenshot:** German / Japanese / pseudo settings screens, a live English -> German switch, a
  deuteranopia HUD, 150% UI scale in German at 720p, a 2560x1080 HUD, and a code-driven rebind (Scan E -> R).
- **Known gaps:** high contrast recolours tokens and subtitle panels but not every panel; text scale is a whole-UI
  scale rather than font-only; Arabic / RTL is not covered; key capture, gamepad focus navigation and hold/toggle
  wheel behaviour are unit-tested in logic but not exercised with real input.

### M6 — Optimisation, tests, docs, release (1.5 weeks)

1. **Budgets:** define UI frame-time and memory budgets; measure with Slate Insights, `stat slate`,
   `stat game`, Unreal Insights, `memreport`.
2. Fixes: invalidation boxes / retainers where measured to help, widget pooling, texture atlasing/streaming,
   removal of tick and expensive bindings. Record **before/after numbers**.
3. Automation: view-model specs, screen-stack tests, functional screenshot tests, CI-runnable command line.
4. Docs: architecture write-up, ADRs (why MVVM, trade-offs, where it hurt), "how a designer adds a widget"
   guide, coding standard.
5. Portfolio assets: 60-second video (gamepad + KBM, language switch, accessibility toggle, Detective Mode),
   README with GIFs, packaged Windows build.

**Exit:** packaged build, tagged release, README/video ready to link from a CV.

**As built:**
- **Measured, not guessed:** `FGothamPerfHarness` (`-GothamPerf=<label>`) with A/B switches; results and the honest
  reading (tick removal: no measurable change; global Slate invalidation: UI overhead down from ~0.28 to ~0.04 ms on
  the settings screen) are in `Docs/Performance.md`. Slate Insights, GPU and memreport were not captured.
- **Tests:** 20 automation tests; `Scripts/run_tests.py` is the CI entry point (exit code reflects failures).
- **Docs:** README, `Docs/Architecture.md`, six ADRs, `Docs/DesignerGuide.md`, `Docs/Performance.md`.
- **Screenshots:** `Scripts/CaptureScreens.ps1` regenerates every image in `Docs/img` from the game's own dev flags.
- **Package:** Windows Development build cooked and run (HUD, Detective Mode, materials, German localization). A
  Shipping build was not produced.
- **Not delivered:** a demo video (no way to assemble one without installing extra tooling).

## 4. Timeline (part-time)

| Milestone | Effort | Cumulative |
|---|---|---|
| M0 Baseline | 0.5 wk | 0.5 |
| M1 MVVM HUD | 1–1.5 wk | 2 |
| M2 Common UI + input | 1.5 wk | 3.5 |
| M3 Slate | 1.5 wk | 5 |
| M4 Detective Mode | 2 wk | 7 |
| M5 Settings / a11y / loc | 1.5 wk | 8.5 |
| M6 Perf / docs / release | 1.5 wk | 10 |

About 10 weeks part-time. M1–M3 alone already cover the core job requirements and make a presentable
portfolio piece if time runs short.

## 5. Risks and mitigations

| Risk | Mitigation |
|---|---|
| UE 5.8 MVVM/CommonUI API differences from tutorials | Check engine source for the exact 5.8 APIs; keep a `Docs/Notes` log of gotchas |
| Scope creep into gameplay | Fake gameplay only: mannequin, dummy damage, debug keys |
| Editor-only Blueprint/widget assets are hard to review in git | Keep logic in C++; use C++ base widget classes; document widget hierarchy in the README |
| Art quality | Grey-box + a consistent, simple visual language and a limited palette; focus on motion and clarity |
| Shader complexity | Start with UI materials, then post-process; keep effects modular |

## 6. Definition of done (whole project)

- Packaged Windows build runs at a stable frame rate on the target machine.
- All screens work on gamepad and keyboard/mouse with live glyph switching.
- At least three languages and four accessibility options work live.
- Automation tests pass from the command line.
- README, architecture doc, ADRs and a demo video exist.
- Profiling section shows measured before/after improvements.

## 7. Immediate next step

Complete **M0** (plugins, Build.cs, `L_Arena`, git), then start **M1** with the core classes and view models.
