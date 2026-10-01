# ADR 0007: Menu input through Common UI's Enhanced Input support

**Status:** accepted (review fix R2, replaces the menu input described in ProjectPlan M4 "As built")

## Context
Menus first handled their own keys: Esc and gamepad B in `UMvsScreen::NativeOnKeyDown`, toggle keys looked up in
the Enhanced Input key profile, the gadget wheel's open key in `NativeOnKeyUp`, tab keys in a preview handler, and a
hand-built hint bar whose "Select" prompt sent a synthetic Enter on the next frame. The gameplay mapping context was
removed while a menu was open and added back after. This bypassed Common UI's back handling, its action bar and its
platform accept / back swap, and kept two input systems in step by hand.

## Decision
- `bEnableEnhancedInputSupport` is on, and Common UI's input data is a code class, `UMvsUIInputData`, whose accept,
  back and tab actions are built in code like the gameplay ones (no input assets).
- The player controller adds two mapping contexts once and never removes them: gameplay (priority 0) and the menu keys
  (priority 1, `UMvsUIInputData::BuildMappingContext`). Common UI finds a binding's keys through Enhanced Input's
  active mappings, and its Menu input mode blocks game input while a menu is open (and flushes held keys), so there is
  nothing to swap. The menu actions do not consume their keys, so Q and E still reach the wheel and scan in gameplay.
- Every screen key is a Common UI binding registered by the screen: back is the activatable back handler, the toggle
  key and the case file key are bindings on the gameplay actions, the wheel binds its action's press and release, and
  the tab list uses Common UI's tab actions. Bindings belong to the screen's node, so only the active screen answers.
- Prompts are a bound action bar (`UMvsActionBar`, `UMvsHintButton` as its button). The accept prompt runs the
  accept binding, which clicks the focused item through `IMvsAcceptable` instead of sending a key.
- Accept and back map the platform's virtual gamepad keys (`Virtual_Gamepad_Accept`, `Virtual_Gamepad_Back`), so
  platforms that swap the face buttons get the swap.

## Consequences
- Rebinding needs no notification: glyphs refresh on `ControlMappingsRebuiltDelegate`, and the bindings read the live
  mappings on each key. `UMvsUISubsystem::OnBindingsChanged` and the glyph's retry loop are gone.
- Common UI's bound action bar shows the active screen's actions. A screen stays visible under a modal, so
  `UMvsActionBar` hides prompts that are not its own screen's: under the quit confirmation, the pause prompts are
  gone (the `pause-quit` baseline was refreshed for it).
- Common UI's bound action bar keeps its button class private with no setter; `UMvsActionBar` sets that one
  designer property through reflection, as a designer would in the widget editor.
- Key caps stay text, from localized labels in the naming of the gamepad in use (`MvsBindings::GetKeyLabel`), rather
  than Common Input's per-controller icon brushes: the project has no icon art, and text caps follow UI scale, high
  contrast and language. The key itself is the one Common UI resolves (`CommonUI::GetFirstKeyForInputType`), so a
  prompt and its action always agree.
- Raw keys remain only where raw keys are the point: capturing a key to rebind (Controls), and the gadget wheel's own
  number keys and stick.
