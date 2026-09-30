# UI performance

What was measured, how, what changed, and what was **not** measured.

## Method

A dev-only harness (`Source/MVVMSample/Core/GothamPerfHarness.cpp`) steps through fixed UI scenarios and samples each
for 8 seconds after a 2 second warm-up. It runs the game at 1280x720 with the frame rate uncapped and VSync off, so
frame time reflects work done rather than a display refresh.

```bash
# one run; writes Saved/Perf/<label>.md and exits
UnrealEditor.exe MVVMSample.uproject /Game/Maps/L_Arena -game -windowed -ResX=1280 -ResY=720 -GothamPerf=<label>
```

A/B switches in the same binary keep the comparison fair:

| Flag | Effect |
|---|---|
| `-GothamKeepTick` | Skip `GothamUI::DisableTick`, i.e. the engine default where every C++ widget ticks |
| `-GothamInvalidation` | Turn on `Slate.EnableGlobalInvalidation` for the run |

Scenarios: `no-ui` (the whole UI layer hidden, the reference), `hud-idle`, `hud-animating` (combo meter and gadget
cooldowns updating every frame), `detective` (post-process, overlay material, tracker), `gadget-wheel` (stick sweeping
around), `case-file-505` (505 clues, scrolled continuously), `settings`.

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
| detective | 0.837 (+0.125) | 0.902 (+0.149) | 0.895 (+0.149) | 0.779 (**+0.038**) |
| gadget-wheel | 0.892 (+0.180) | 0.892 (+0.139) | 0.941 (+0.195) | 0.829 (**+0.088**) |
| case-file-505 | 0.781 (+0.069) | 0.825 (+0.072) | 0.845 (+0.099) | 0.761 (**+0.020**) |
| settings | 1.011 (+0.299) | 1.016 (+0.263) | 1.024 (+0.278) | 0.782 (**+0.041**) |
| widgets ticking (settings) | 44 | 44 | **0** | 0 |

Run-to-run noise is about 0.05 ms (compare the two "before" runs), so differences smaller than that are not real.

### What the changes did

1. **Stop widgets ticking (`GothamUI::DisableTick`).** UUserWidget's default `Auto` tick treats any class without a
   Blueprint asset as needing a native tick every frame; every C++-only widget here was ticking (44 with the settings
   screen open) although all of them are driven by view-model delegates. Now 0 tick. **Measured effect: none beyond
   noise.** It is still the right default (it removes per-frame work that scales with widget count, and it avoids
   surprising costs on lower-end hardware), but it did not show on this machine at this widget count. The honest
   summary is "correct, cheap, unmeasurable here".
2. **Slate global invalidation (`Slate.EnableGlobalInvalidation=1`).** Slate caches painted widgets and repaints only
   what a view-model change invalidates. **Measured effect: real and well outside noise**: UI overhead falls from
   ~0.28 to ~0.04 ms with the settings screen open, ~0.14 to ~0.04 ms in Detective Mode, ~0.15 to ~0.09 ms for the
   idle HUD, ~0.08 to ~0.02 ms for the 505-clue case file. It is enabled in `Config/DefaultEngine.ini`.
   All custom Slate widgets (`SGadgetWheel`, `SComboMeter`) already call `Invalidate(...)` when their state changes,
   which is what makes this safe; screenshots of the wheel, Detective Mode, case file and a live language switch were
   checked after enabling it and render correctly.

### Already in the design (not a change, but measured to hold)

- **Virtualised clue log.** 505 clues create 5 row widgets, not 505 (`UListView` pooling, rows rebind and cancel
  in-flight thumbnail loads when recycled). `case-file-505` costs about as much as the empty HUD.
- **Idle custom Slate widgets are free.** `SGadgetWheel` and `SComboMeter` register an active timer only while
  animating and unregister once settled; they never tick.
- **Detective overlay collapses when off**, so it costs nothing outside the mode.

## V1 night scene

The night rooftop (volumetric fog, many shadowed spot lights, SSR on wet surfaces, post-process rain) and the
animated mannequin raise the baseline. Lumen stays off by project decision.

| | Grey box (M6) | Night scene, 720p | Night scene, 1080p |
|---|---|---|---|
| Avg frame, no UI | 2.35 ms | 5.15 ms | 6.01 ms |
| Avg game thread, no UI | 0.74 ms | 1.53 ms | 1.45 ms |
| Worst UI game-thread overhead (any screen) | 0.09 ms | 0.09 ms (hud-animating) | 0.16 ms (hud-animating) |
| Detective Mode extra frame cost | ~0.1 ms | ~0.47 ms | ~0.44 ms |

1080p stays near 166 fps on the dev machine. The game-thread rise is the skeletal mesh and animation Blueprint, not
the UI. The Detective Mode cost grew because its post-process now runs over a heavier scene; V3 rewrites that pass.

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
viewport's worth), logged by `-GothamClueLog=200`.

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

## Budgets

| Budget | Target | Status |
|---|---|---|
| UI game-thread cost, any single screen | <= 0.3 ms at 60 fps (under 2% of a 16.6 ms frame) | Met: worst case +0.25 ms (Detective Mode markers, V3); menus up to +0.12 ms |
| Ticking widgets while idle | 0 | Met (0) |
| Row widgets for any list | bounded by viewport, not item count | Met (case-file tile view: 20 tiles for 205 clues) |
| Widget objects, all screens | no unbounded growth | Steady: 9 (HUD) to 38 (tabbed settings) |

## Not measured

Be sceptical of anything not in the table above:

- **GPU cost of the UI and post-process** (`stat gpu`, RenderDoc). Detective Mode's post-process pass adds roughly
  0.1 ms to total frame time here, but this is not a proper GPU profile.
- **Slate Insights / Unreal Insights captures.** The harness gives before/after totals; it does not attribute cost
  to individual widgets. That is the next step for anything that regresses.
- **`memreport` and texture streaming.** Memory is reported as process working set and UObject count only.
- **Lower-end hardware and other platforms.** The gains from removing ticks are expected to matter more there.
- **Frame-time hitches** (max frame). Averages and P95 are recorded; maximum is dominated by warm-up.
