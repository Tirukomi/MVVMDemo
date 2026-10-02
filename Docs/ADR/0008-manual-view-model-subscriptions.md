# ADR 0008: Views subscribe to view models in code, until their layout moves to UMG

**Status:** accepted (review fix R3, finding 17)

## Context
The project uses the MVVM plugin's view models (`UMVVMViewModelBase`, field notify) but not its binding system: every
view subscribes to the fields it shows by hand (`MvsMVVM::Bind`). That reads as half an adoption. The plugin's
bindings, though, are authored in the widget editor and compiled into the widget Blueprint (`UMVVMView`). Widgets in
this project build their tree in C++ (ADR 0002), and a native widget has no compiled view to hold bindings.

## Decision
- Code-built views keep explicit subscriptions through `MvsMVVM::Bind(ViewModel, this, &Handler, { Fields })`: one
  line per view, field-specific, removed with `MvsMVVM::Unbind`. That is the same field-notify mechanism the plugin's
  bindings use, without the compiled view.
- Declarative bindings come with the designer-authored screens of ADR 0002's amendment: when a screen's layout moves
  into a `WBP_`, its view-model bindings move into that asset's View Bindings, and `UMvsViewModelResolver` (already
  in place) hands it the player's view models by class, all of them since R1.
- Gameplay-to-view-model wiring is not a view concern and stays in code either way: one binder per feature
  (`ViewModels/MvsViewModelBinders.h`), each owning its view models and its subscriptions (`FMvsSubscriptions`).

## Consequences
- A reviewer sees every subscription in the view's source and every gameplay link in one binder per feature.
- Moving a screen to UMG changes how its bindings are written, not the view models: they are already plugin view
  models with field notify, and the resolver already finds them.
- Commands go the other way through the view models too (the gadget bar's `RequestUse`, the controls view model's
  `RequestRebind`), so a designer-built view would need no gameplay access either.

## Amendment: one screen through the plugin's bindings (second review S6)

`WBP_PauseMenu` (parented to `UPauseMenuScreen`) authors the pause status panel's content in the widget designer and
fills it with MVVM View Bindings (`ObjectivesViewModel.ObjectiveTitle`, `ClueListViewModel.ProgressText`, both
resolved by `UMvsViewModelResolver`). The frame, the menu and the panel stay code-built, so the screenshots could stay
pixel-identical; they are (`pause`, `pause-quit`: 0.00%).

What the plugin's bindings saved, against `MvsMVVM::Bind` on this screen:
- The C++ for the status content: the four texts, their two subscriptions, `RefreshStatus` and its unbinding. A
  designer changes the layout and the bindings without a build.
- The resolver needed no change: it had been in place since R1, untested until S3.

What they cost:
- Every value a view shows must be a bindable field of its type. The evidence line was a format over two ints in the
  view; it became `UClueListViewModel::ProgressText`, since the alternative (a conversion function) is more code than
  the subscription it replaces.
- Text typed in a Widget Blueprint gets a localization key of its own, which the existing translations do not have.
  The captions are therefore named widgets (`BindWidgetOptional`) that the screen fills from the game's keys, and the
  designer's preview texts are not localized. A team doing this at scale would want a string table.
- A Widget Blueprint is not in memory at load the way a C++ class is: the screen preload must cover it before
  anything opens it (it does; the screenshot dev aid opened pause on the first frame and loaded it on the spot).
- Mistakes surface late and in the editor: binding a widget instead of its Text fails to compile, and a missed Style or
  padding showed only as a screenshot diff.
- Run time, one session (pause-quit's UI cost, game thread over the paused reference): 0.073 ms, against 0.04 to 0.05 ms
  for the code-built screen in recent gates and a 0.15 ms budget; not separable from noise with one session. The live
  user-widget count in that run was 61 against 48 in earlier runs, not explained yet.

Decision unchanged: code-built views keep `MvsMVVM::Bind`; a screen whose layout a designer owns moves its bindings into
the asset, as this one did.
