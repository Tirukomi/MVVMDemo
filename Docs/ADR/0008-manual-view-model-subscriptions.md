# ADR 0008: Views subscribe to view models in code, until their layout moves to UMG

**Status:** accepted (review fix R3, finding 17)

## Context
The project uses the MVVM plugin's view models (`UMVVMViewModelBase`, field notify) but not its binding system: every
view subscribes to the fields it shows by hand (`GothamMVVM::Bind`). That reads as half an adoption. The plugin's
bindings, though, are authored in the widget editor and compiled into the widget Blueprint (`UMVVMView`). Widgets in
this project build their tree in C++ (ADR 0002), and a native widget has no compiled view to hold bindings.

## Decision
- Code-built views keep explicit subscriptions through `GothamMVVM::Bind(ViewModel, this, &Handler, { Fields })`: one
  line per view, field-specific, removed with `GothamMVVM::Unbind`. That is the same field-notify mechanism the plugin's
  bindings use, without the compiled view.
- Declarative bindings come with the designer-authored screens of ADR 0002's amendment: when a screen's layout moves
  into a `WBP_`, its view-model bindings move into that asset's View Bindings, and `UGothamViewModelResolver` (already
  in place) hands it the player's view models by class, all of them since R1.
- Gameplay-to-view-model wiring is not a view concern and stays in code either way: one binder per feature
  (`ViewModels/GothamViewModelBinders.h`), each owning its view models and its subscriptions (`FGothamSubscriptions`).

## Consequences
- A reviewer sees every subscription in the view's source and every gameplay link in one binder per feature.
- Moving a screen to UMG changes how its bindings are written, not the view models: they are already plugin view
  models with field notify, and the resolver already finds them.
- Commands go the other way through the view models too (the gadget bar's `RequestUse`, the controls view model's
  `RequestRebind`), so a designer-built view would need no gameplay access either.
