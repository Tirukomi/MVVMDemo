# ADR 0002: Widgets build their own tree in C++

**Status:** accepted, revisit if a designer-heavy workflow appears

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
