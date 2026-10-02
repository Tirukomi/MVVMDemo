# UI performance

What was measured, how, what changed, and what was **not** measured.

## Method

A dev-only harness (`Source/MVVMSample/Core/MvsPerfHarness.cpp`) steps through fixed UI scenarios and samples each
for 8 seconds after a 2 second warm-up. It runs the game at 1280x720 with the frame rate uncapped and VSync off, so
frame time reflects work done rather than a display refresh.

```bash
# one run; writes Saved/Perf/<label>.md and exits
UnrealEditor.exe MVVMSample.uproject /Game/Maps/L_Arena -game -windowed -ResX=1280 -ResY=720 -MvsPerf=<label>
```

A/B switches in the same binary keep the comparison fair:

| Flag | Effect |
|---|---|
| `-MvsKeepTick` | Skip `MvsUI::DisableTick`, i.e. the engine default where every C++ widget ticks |
| `-MvsInvalidation` | Turn on `Slate.EnableGlobalInvalidation` for the run |

Scenarios: `no-ui` (the whole UI layer hidden, the reference), `hud-idle`, `hud-animating` (combo meter and gadget
cooldowns updating every frame), `forensic` (post-process, overlay material, tracker), `gadget-wheel` (stick sweeping
around), `case-file-505` (505 clues, scrolled continuously), `settings`, `pause-quit` (the quit confirmation over
pause: a modal over a menu), `combat`.

The metric that isolates UI cost is **game-thread time** (`GGameThreadTime`): Slate ticks and paints on the game
thread. Read each scenario **minus `no-ui` from the same run**. Total frame time is dominated by rendering the 3D scene
and barely moves.

Machine: one Windows 11 development PC, UE 5.8 editor-target `-game` build, D3D12. Numbers compare configurations on
this machine; they are not portable absolutes.

## Results

Average game-thread milliseconds, then UI overhead (the same scenario minus `no-ui` in the same run).

| Scenario | A: before | A: repeat | B: tick fix | C: tick fix + global invalidation |
|---|---|---|---|---|
| no-ui (reference) | 0.712 | 0.753 | 0.746 | 0.741 |
| hud-idle | 0.857 (+0.145) | 0.904 (+0.151) | 0.896 (+0.150) | 0.829 (**+0.088**) |
| hud-animating | 0.811 (+0.099) | 0.867 (+0.114) | 0.865 (+0.119) | 0.814 (**+0.073**) |
| forensic | 0.837 (+0.125) | 0.902 (+0.149) | 0.895 (+0.149) | 0.779 (**+0.038**) |
| gadget-wheel | 0.892 (+0.180) | 0.892 (+0.139) | 0.941 (+0.195) | 0.829 (**+0.088**) |
| case-file-505 | 0.781 (+0.069) | 0.825 (+0.072) | 0.845 (+0.099) | 0.761 (**+0.020**) |
| settings | 1.011 (+0.299) | 1.016 (+0.263) | 1.024 (+0.278) | 0.782 (**+0.041**) |
| widgets ticking (settings) | 44 | 44 | **0** | 0 |

Run-to-run noise is about 0.05 ms (compare the two "before" runs), so differences smaller than that are not real.

### What the changes did

1. **Stop widgets ticking (`MvsUI::DisableTick`).** UUserWidget's default `Auto` tick treats any class without a
   Blueprint asset as needing a native tick every frame; every C++-only widget here was ticking (44 with the settings
   screen open) although all of them are driven by view-model delegates. Now 0 tick. **Measured effect: none beyond
   noise.** It is still the right default (it removes per-frame work that scales with widget count, and it avoids
   surprising costs on lower-end hardware), but it did not show on this machine at this widget count. The honest
   summary is "correct, cheap, unmeasurable here".
2. **Slate global invalidation (`Slate.EnableGlobalInvalidation=1`).** Slate caches painted widgets and repaints only
   what a view-model change invalidates. **Measured effect: real and well outside noise**: UI overhead falls from
   ~0.28 to ~0.04 ms with the settings screen open, ~0.14 to ~0.04 ms in Forensic Mode, ~0.15 to ~0.09 ms for the
   idle HUD, ~0.08 to ~0.02 ms for the 505-clue case file. It is enabled in `Config/DefaultEngine.ini`.
   All custom Slate widgets (`SGadgetWheel`, `SComboMeter`) already call `Invalidate(...)` when their state changes,
   which is what makes this safe; screenshots of the wheel, Forensic Mode, case file and a live language switch were
   checked after enabling it and render correctly.

### Already in the design (not a change, but measured to hold)

- **Virtualised clue log.** 505 clues create 5 row widgets, not 505 (`UListView` pooling, rows rebind and cancel
  in-flight thumbnail loads when recycled). Since V4 it is a tile view: 20 tiles for 505 clues. `case-file-505` costs about as much as the empty HUD.
- **Idle custom Slate widgets are free.** `SGadgetWheel` and `SComboMeter` register an active timer only while
  animating and unregister once settled; they never tick.
- **Forensic overlay collapses when off**, so it costs nothing outside the mode.

## V1 night scene

The night rooftop (volumetric fog, many shadowed spot lights, SSR on wet surfaces, post-process rain) and the
animated mannequin raise the baseline. Lumen stays off by project decision.

| | Grey box (M6) | Night scene, 720p | Night scene, 1080p |
|---|---|---|---|
| Avg frame, no UI | 2.35 ms | 5.15 ms | 6.01 ms |
| Avg game thread, no UI | 0.74 ms | 1.53 ms | 1.45 ms |
| Worst UI game-thread overhead (any screen) | 0.09 ms | 0.09 ms (hud-animating) | 0.16 ms (hud-animating) |
| Forensic Mode extra frame cost | ~0.1 ms | ~0.47 ms | ~0.44 ms |

1080p stays near 166 fps on the dev machine. The game-thread rise is the skeletal mesh and animation Blueprint, not
the UI. The Forensic Mode cost grew because its post-process now runs over a heavier scene; V3 rewrites that pass.

## V2 HUD

| 1080p, game thread over `no-ui` | V1 HUD | V2 first pass | V2 after fix (two runs) |
|---|---|---|---|
| hud-idle | +0.08 ms | +0.07 ms | -0.01 / +0.09 ms |
| hud-animating | +0.16 ms | **+0.41 ms** (over budget) | +0.09 / +0.20 ms |
| settings | -0.01 ms | +0.15 ms | -0.08 / +0.03 ms |

The first pass re-applied fonts, colours and bar properties on every combo-decay and cooldown notification, so
layout was invalidated every frame. The fix is `ApplyStyle()` on construction and settings changes only, with a
per-frame path that touches just the changing value (the decay bar's percent, the cooldown rings, the seconds readout
when the whole second changes).

## V4 menus

1080p, over `no-ui` in the same run (`Saved/Perf/V4_menus_1080p*.md`; the third run is after the tab list stopped
ticking).

| Scenario | Frame time | Game thread |
|---|---|---|
| settings (blur, scrim, tabs, highlight list, detail pane) | +0.10 / +0.12 ms | +0.03 / +0.06 / +0.01 ms |
| case-file-505 (tile view, scrolling across the board) | +0.01 / +0.02 ms | +0.06 / +0.12 / +0.08 ms |

The frame-time cost is the background blur. The highlight bar's active timer runs only while its list holds focus
and invalidates paint only when the bar moves. The evidence board keeps 20 tile widgets alive for 205 clues (the
viewport's worth), logged by `-MvsClueLog=200`.

## V5 combat

1080p, two runs (`Saved/Perf/V5_combat_1080p*.md`). Four thugs are in the world for every row. The attack director is
off except in `combat`, where two thugs telegraph on a loop (a counter prompt in view and an edge arrow) while
attacks build the combo past its milestones.

| | Run A | Run B |
|---|---|---|
| no-ui game thread (V4: 1.50 to 1.57 ms) | 1.60 ms | 1.68 ms |
| hud-idle over no-ui | +0.14 ms | +0.14 ms |
| combat over no-ui (game thread) | +0.12 ms | +0.19 ms |
| combat over no-ui (frame) | +0.03 ms | +0.00 ms |

- **Thugs:** four animated characters cost about 0.1 ms of game thread; that's gameplay, not UI.
- **Idle HUD:** it rose from about +0.10 to +0.14 ms, because the threat layer projects each thug every frame while
  any exist (it has to, to know when to show an arrow).
- **Combat:** drawing the prompts and arrows is cheap next to that, since everything is one Slate pass with no
  widgets per enemy.

## R5 review fixes (2026-10-01)

Measured at 1920x1080 (the gate's resolution, not the 1280x720 of the sections above), three runs before and three
after on the same machine, median per scenario. Game thread and GPU in ms; "UI" is the scenario minus `no-ui` from the
same set of runs. GPU is `RHIGetGPUFrameCycles` (whole frame), the baseline R0 added.

| Scenario | Game before | Game after | UI before | UI after | GPU before | GPU after |
|---|---|---|---|---|---|---|
| no-ui (reference) | 1.644 | 1.633 | | | 5.569 | 5.588 |
| hud-idle | 1.846 | 1.818 | +0.202 | +0.185 | 5.577 | 5.609 |
| hud-animating | 1.879 | 1.865 | +0.235 | +0.232 | 5.582 | 5.615 |
| forensic | 1.876 | 1.834 | +0.232 | +0.201 | 6.084 | 6.084 |
| gadget-wheel | 1.824 | 1.799 | +0.180 | +0.166 | 5.740 | 5.742 |
| case-file-505 | 2.289 | 2.264 | +0.645 | +0.631 | 5.714 | 5.718 |
| settings | 1.692 | 1.655 | +0.048 | +0.022 | 5.703 | 5.707 |
| pause-quit (new) | 1.803 | 1.809 | +0.159 | +0.176 | 5.616 | 5.600 |
| combat | 1.880 | 1.860 | +0.236 | +0.227 | 5.615 | 5.611 |

What changed, and what it did:

1. **Forensic overlay fade [24].** The fade now sets only visibility, opacity and the material's progress; fonts,
   colours, the prompt text and the material's tint are applied on settings changes and when the overlay appears.
   UI cost in `forensic` went from +0.232 to +0.201 ms: in the right direction, inside run-to-run noise (~0.05 ms).
2. **Paint allocations [25].** Clue-marker text is formatted only when the rounded distance, the marker state or the
   language changes (it was formatted and upper-cased per marker per frame); gadget icon strokes are a table built
   once (they were rebuilt every paint); panel outlines are built once per paint in inline storage, the fill uses
   scratch vertex arrays, and arcs reserve their points. Slate's draw elements own their point arrays
   (`MakeLines` takes them by value), so one allocation per drawn line remains: that one is the engine's. No change
   beyond noise on this machine.
3. **One background blur on screen [26].** The premise in the review plan was off: Settings opened from Pause does not
   stack two blurs, because a layer stack shows only its top screen. The case that does is a modal over a menu (the
   quit confirmation over Pause), now measured as `pause-quit`. `UMvsUISubsystem` keeps the blur only on the
   topmost blurring screen. GPU over `no-ui`: +0.047 before, +0.012 ms after, so the second blur cost about 0.035 ms
   at 1080p here: real, and small. The confirmation's own blur now covers the world on its own (the world behind is
   less blurred than with both).
4. **Tweens [27].** Pop, fade and slide run as an active timer on the animated widget's Slate widget, with the
   per-channel handle kept as Slate metadata on that widget, instead of on the core ticker with a static map. Same
   real-time behaviour, nothing global, and an animation ends with its widget. (Not Slate curve sequences as the plan
   said: `FCurveSequence::Play` also only registers an active timer, and the values still need applying each frame.)
   Not a performance change, and none measured.

The gate's A/B check (G5: five alternating rounds against the previous master, median of paired UI-cost
differences) agrees and is slightly more favourable: combat -0.070 ms, forensic -0.048 ms, the rest within
+-0.03 ms (hud-idle +0.006). Combat and Forensic Mode are where markers and panels paint every frame.

Summary: R5 is mostly hygiene that a reviewer would expect, not a big speed-up. On this machine the UI's game-thread cost was
already small and dominated by the 505-clue list and the HUD; none of these changes moves it outside noise, and the
GPU frame is the scene's.

## Second review S0: what the numbers were measuring (2026-10-01)

An Unreal Insights capture of every scenario (`Scripts/ProfileUI.ps1`: the harness marks each scenario's sampled frames
as a timing region; `Scripts/profile_diff.py` compares each with its reference timer by timer) showed three problems
with the method, all fixed in the harness:

- **Two scenarios never showed what they measure.** Until R4, `case-file-505` never opened the case file (10 widgets,
  the HUD's), so its +0.06 to +0.15 ms in the V4 to R0 tables measured the HUD. `pause-quit` (new in R5) never opened
  pause: settings' teardown left settings on the stack for its outro, and the toggle popped it again. Each scenario now
  has a check that runs on its first sampled frame; the report's `Valid` column records it, and G5 fails on `no`.
- **The pause menu pauses the world**, which takes about 0.8 ms of game-thread work away, so against `no-ui` a paused
  scenario reads as negative UI cost. A `no-ui-paused` reference and a `Paused` column fix that: G5, the budgets and
  `profile_diff.py` compare a paused scenario with the paused reference.
- **"Scenario minus no-ui" is not all UI.** For the idle HUD, the Slate timers (tick, paint, active timers) add about
  0.04 ms of the ~0.15 ms; the rest is world work that differs between scenarios (`FXSystemPreRender` alone varies by
  0.04 to 0.15 ms). The totals stay useful for A/B, but attributing cost needs the profile.

What the profile says the UI costs (exclusive game-thread time over the reference, per frame, traced):

| Scenario | Slate and widget timers | Biggest items |
|---|---|---|
| hud-idle | ~0.04 ms | the threat indicator layer's active timer (repaints every frame whenever a thug exists) |
| case-file-505 | ~0.65 ms | the tile view rebuilding rows while it scrolls (`STableViewBase::Tick` 0.22 ms incl., 160 hit-grid removals and 16 rows removed from the tree per frame), tile paints 0.17 ms, prepass 0.06 ms |
| settings | ~0.11 ms | two active timers, 16 ticking widgets |
| pause-quit | ~0.05 ms | one active timer |

`case-file-505` scrolls about one row of four tiles per frame at its fastest, so it is the worst case by design; but at
+0.55 to +0.65 ms it is over the 0.3 ms target below, and its 35 tiles tick (the list view requires it). Second review
S7 takes it on. G5's absolute budgets (`Scripts/PerfBudgets.json`) start from today's numbers so nothing gets worse
meanwhile; for the case file that is a ceiling, not the target.

**G5's rounds come in pairs (S1).** G5 runs the reference and the current build alternately (ref, cur; cur, ref; ...)
so drift cancels. With five rounds, three ran the reference first and two the current build first, and whichever ran
second in a round measured up to 0.2 ms slower. The median of single rounds then followed the order, not the code: S1
failed twice on a different scenario each time (`case-file-505` and `hud-animating`, then `hud-idle`), each with rounds
alternating in sign and order-balanced averages within +-0.05 ms. G5 now runs six rounds and judges the median of the
three pair averages (one round of each order). S0's real shift (`settings`, pairs +0.14 and +0.19 ms) still fails it.

## Second review S7: the case file and the world overlays (2026-10-02)

Traces with `Scripts/ProfileUI.ps1`, single harness sessions for the split (game thread, UI cost = scenario minus its
reference in the same run).

**The case file (`case-file-505`).** Scrolling costs about 0.55 ms of UI; the same screen held still costs 0.13 ms.
The difference is the engine's tile view: on every frame the offset moves it clears its panel and re-adds every
visible tile (16 a frame here), so each tile's widgets are invalidated, prepassed and hit-test registered again. That
happens at any scroll speed, so it is the per-frame cost of scrolling, not of this scenario's speed (it scrolls at up
to 120 rows a second; a gamepad steps about 7). The tiles' own work is small: one rebind a frame, 0.009 ms.

| Change | What it cut |
|---|---|
| Tile content hit-test invisible (the list row takes the input) | hit-test grid additions 70 to 6 a frame |
| One widget per tile (`SClueTile`: frame and thumbnail painted, three text blocks laid out by hand), 11 Slate widgets to 6 | tile view tick 0.232 to 0.183 ms, removal invalidation 0.083 to 0.050 ms, widget ticks 39 to 23 a frame |
| One theme per tile refresh, the case number from the entry instead of a search of the list | (inside the 0.009 ms) |

Result: about 0.55 ms against 0.60 to 0.63 ms in recent gates; the `case-file` screenshot is pixel-identical. The
0.3 ms target while scrolling is out of reach without replacing `UTileView` with a grid that moves its children
instead of re-adding them (and re-doing its navigation and selection): a decision, not a fix, so it is left open.

**World overlays (finding 29).** They repainted every frame while active. Now the active timer still fetches items
each frame (cheap: projecting a few positions) but repaints only when the items differ or the layer animates (a
warning's pulse, off under reduced motion); `Mvs.Slate.WorldOverlay.Repaint` checks it.

**Not done, because the capture does not show them** (findings 24 to 26): the threat subsystem's per-frame arrays and
the wheel's per-sector vertex arrays do not appear among the timers, and the gadget, combo and forensic components
already switch their tick off when idle (none ticks in `hud-idle`; finding 26 was out of date). The combo component's
0.039 ms a frame in combat is the combo meter's updates, which run through it, inside the 0.30 ms combat budget.

## Budgets

| Budget | Target | Status |
|---|---|---|
| UI game-thread cost, any single screen | <= 0.3 ms at 60 fps (under 2% of a 16.6 ms frame) | Met except the case file: +0.55 to +0.65 ms while scrolling 505 clues continuously (measured since S0; before that the scenario never opened it). Others up to +0.22 ms (Forensic Mode). Gated per scenario by `Scripts/PerfBudgets.json` |
| Ticking widgets while idle | 0 | Met for the HUD and menus; the case file's tiles tick (the list view requires it) |
| Row widgets for any list | bounded by viewport, not item count | Met (case-file tile view: 20 tiles for 205 clues) |
| Widget objects, all screens | no unbounded growth | Steady: 9 (HUD) to 38 (tabbed settings) |

## Not measured

Be sceptical of anything not in the table above:

- **Per-pass GPU cost** (`stat gpu`, RenderDoc). Since R0 the harness records whole-frame GPU time
  (`RHIGetGPUFrameCycles`): Forensic Mode adds about 0.5 ms to the GPU frame at 1080p, a second background blur about
  0.035 ms. Which pass costs what inside a frame is still not measured.
- **Per-widget attribution in G5.** The gate compares totals; `Scripts/ProfileUI.ps1` attributes cost per timer and
  widget (see the S0 section), but it is run by hand.
- **`memreport` and texture streaming.** Memory is reported as process working set and UObject count only.
- **Lower-end hardware and other platforms.** The gains from removing ticks are expected to matter more there.
- **Frame-time hitches** (max frame). Averages and P95 are recorded; maximum is dominated by warm-up.
