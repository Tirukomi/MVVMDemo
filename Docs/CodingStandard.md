# Coding Standard

Follows Epic's Unreal coding standard, plus these project rules.

## Naming
- Epic prefixes: `U` (UObject), `A` (Actor), `F` (struct), `E` (enum), `I` (interface), `S` (Slate), `T` (template).
- Assets: `WBP_` widget, `M_`/`MI_` material, `MPC_` material parameter collection, `DA_` data asset,
  `DT_` data table, `IA_` input action, `IMC_` mapping context, `ST_` string table, `L_` level.
- View models: `U<Name>ViewModel`. Screens: `U<Name>Screen`. Slate widgets: `S<Name>`.

## Architecture rules
1. Gameplay code never includes UI headers.
2. View models never reference widgets or the world; they are constructible in automation tests.
3. Widgets contain presentation logic only; state changes go through view-model commands.
4. Set view-model properties with `UE_MVVM_SET_PROPERTY_VALUE` so field notifications fire.
5. All user-facing text is `FText` from a string table (`LOCTEXT` for code-only labels).
6. No Tick in widgets unless justified in a comment; prefer events, timers and animations. C++-only widgets tick by
   default (no Blueprint class), so call `MvsUI::DisableTick(this)` in `NativeConstruct` (widgets deriving
   `UMvsSettingsAwareWidget` / `UMvsScreen` already do). Exception: list / tile view entry widgets. The list
   forces their rows to tick (`UListViewBase::HandleGenerateRow`), and an entry flagged Never trips UMG's
   "mismatching tick states" ensure. Custom Slate widgets animate with an active timer that unregisters when settled.
7. C++ base classes for every screen; Blueprint/UMG only for layout, styling and animation.

## Style
- Tabs for indentation, braces on their own line (Epic style), `const` correctness, `TObjectPtr` for members.
- Comments explain why, not what. Keep headers light: forward declare where possible.

## Review checklist
- Does it build and pass automation tests?
- Any new per-frame allocation or binding on a hot path?
- Gamepad + keyboard navigable? Localizable? Colour-independent?

## Additional rules

- Colours that carry meaning come from `MvsPalette` tokens, never literals. Text that should follow settings is a
  `UMvsText` with a type style (and a token when its colour is fixed); layout numbers that recur are named in
  `MvsMetrics`.
- Code-built text is `UMvsText`, and capitals go through `MvsText::SetUpperCase`, never
  `ETextTransformPolicy::ToUpper`: Slate's transform cannot change a string's length, so German "ß" ("SS") ensures and
  stays in mixed case.
- Anything with a rule worth testing is UObject-free or exposes an `Advance(DeltaTime)`-style entry point.
- Reflection to write a non-public engine property is allowed only for properties the engine exposes to the editor
  (a designer would set them in a widget or asset), when there is no setter and the object is built in code, and only
  in these three documented places:
  - `MakeActionMappable` (`Core/MvsPlayerController.cpp`): an input action's player-mappable key settings (ADR 0005).
  - `MvsUI::DisableTick` (`UI/MvsWidgetTick.h`): a widget's `TickFrequency` (class defaults, "Performance").
    Native widget classes have no Blueprint class, so the engine ticks them every frame unless it is `Never`, and the
    `DisableNativeTick` class flag is read only for Blueprint classes. Checked in 5.8: no setter.
  - `UMvsActionBar`'s constructor: the bound action bar's `ActionButtonClass` (ADR 0007).
  Each names its property, says why there is no alternative, and is re-checked when the engine is upgraded. The
  lookups live in `Core/MvsEngineProperties.h`: a missing or retyped property raises an ensure (which the gate's log
  scan fails on) instead of skipping the write silently, and `Mvs.Engine.Properties` checks all three.
- Dev-only code sits under `#if !UE_BUILD_SHIPPING` and is enabled by a `-Mvs...` flag, never by default.
- Measure before optimising; record before/after in `Docs/Performance.md`, including changes that turned out not to help.
