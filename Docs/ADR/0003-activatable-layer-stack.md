# ADR 0003: Common UI activatable stacks as layers

**Status:** accepted

## Decision
UI lives in four z-ordered `UCommonActivatableWidgetStack`s (Game, GameMenu, Menu, Modal) under one root widget.
`UGothamUISubsystem` is the single entry point for pushing and popping. `FGothamUIModeTracker` derives the input context
from which layers are occupied, and the controller adds or removes the gameplay mapping context accordingly.

## Consequences
- Back handling, focus and input mode follow the stack instead of ad-hoc flags.
- The rules ("a modal takes input", "the HUD alone keeps gameplay input", "Back closes the topmost thing, never the
  HUD") are a pure struct with unit tests.
- Back is handled in `UGothamScreen::NativeOnKeyDown` rather than Common UI's input-data assets, to avoid needing
  binary data assets. The trade-off is that rebinding "Back" would need the screen changed.
