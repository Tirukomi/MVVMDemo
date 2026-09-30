# ADR 0002: Widgets build their own tree in C++

**Status:** accepted; amended 2026-09-30 for static screens (see Amendment)

## Context
UMG layouts are normally authored in the designer as binary assets. Those are hard to review in git and cannot be
generated or diffed. This project is also a portfolio piece whose value is the code.

## Decision
Widgets construct their tree in `RebuildWidget` (guarded by `!WidgetTree->RootWidget`) and bind to view models with
field-notify delegates. The two exceptions are `WBP_ClueEntry` (the list view requires a Blueprint entry class in
editor builds) and the generated materials.

## Consequences
- Layout is reviewable text; behaviour and layout cannot drift apart.
- Designers cannot restyle in the designer without a Blueprint subclass. Styling is therefore pushed into data
  (colour tokens, `UGothamButton` kinds, `FGothamGadgetWheelStyle`) and the Slate wrappers expose designer properties.
- Editor-authored MVVM bindings remain available: the resolver is in place, and any widget can be subclassed in a
  `WBP_`.

## Amendment: designer-authored static screens (refactoring pass P10)

Static screens (pause, the settings and case-file frames) may move their layout into UMG assets, one screen at a
time, when a layout next needs real rework. Behaviour stays in C++ and the custom Slate (wheel, meters, overlays)
is not affected.

- **Already supported:** every screen builds its code layout only when `WidgetTree->RootWidget` is empty, so a
  `WBP_` subclass with a designer tree replaces the code layout without changes to the base class.
- **Per screen:**
  1. In the C++ base, declare the widgets its behaviour needs as `UPROPERTY(meta = (BindWidgetOptional))` (for the
     pause menu: the menu list, its buttons and the status line), and use them when bound instead of building.
  2. Create `WBP_<Screen>` parented to the C++ class, with widgets of those names, and point the matching
     `UGothamUISettings` screen class at it.
  3. The screen's G4 shots are the check: the asset must reproduce them, or the baselines change on purpose.
- **Not done yet:** step 2 is designer work in the editor. Until a screen has its asset, it keeps its code layout,
  and nothing about it changes.
- **Cost to keep in mind:** layout in a binary asset is no longer reviewable as text, which is why this is per
  screen and only when the layout itself is being reworked.

