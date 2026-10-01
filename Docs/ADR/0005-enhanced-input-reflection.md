# ADR 0005: Rebinding through Enhanced Input user settings, with one reflection hack

**Status:** accepted, with a known workaround

## Context
Enhanced Input's `UEnhancedInputUserSettings` is the engine's rebinding system. It only exposes actions that carry
`UPlayerMappableKeySettings`, and that property (on `UInputAction` and per key mapping) is protected: it is meant to be
authored on assets. Our actions are created in code.

## Decision
Give each rebindable thing its own `UInputAction` (WASD became four actions) and set the action-level settings object
through reflection in one helper (`MakeActionMappable`). All mappings of an action become slots of one row: keyboard
and mouse first, gamepad second. Registration must happen in `BeginPlay`, because the user-settings object does not
exist yet in `SetupInputComponent`.

## Consequences
- Persistence, profiles and apply/save come from the engine. Conflict handling is ours (`MvsBindings::PlanRebind`,
  pure and tested): a conflicting key swaps between the two actions.
- If the engine ever makes the property public, `MakeActionMappable` shrinks to two lines.
- Authoring the input actions as assets would remove the hack entirely; code-built assets were chosen to keep the
  project free of binary input assets.
