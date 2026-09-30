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
   default (no Blueprint class), so call `GothamUI::DisableTick(this)` in `NativeConstruct` (widgets deriving
   `UGothamSettingsAwareWidget` / `UGothamScreen` already do). Exception: list / tile view entry widgets. The list
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

- Colours that carry meaning come from `GothamPalette` tokens, never literals.
- Code-built text is `UGothamText`, and capitals go through `GothamText::SetUpperCase`, never
  `ETextTransformPolicy::ToUpper`: Slate's transform cannot change a string's length, so German "ß" ("SS") ensures and
  stays in mixed case.
- Anything with a rule worth testing is UObject-free or exposes an `Advance(DeltaTime)`-style entry point.
- Reflection to reach a protected engine property is allowed only in one documented helper (see ADR 0005).
- Dev-only code sits under `#if !UE_BUILD_SHIPPING` and is enabled by a `-Gotham...` flag, never by default.
- Measure before optimising; record before/after in `Docs/Performance.md`, including changes that turned out not to help.
