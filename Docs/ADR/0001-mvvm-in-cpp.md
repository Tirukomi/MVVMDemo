# ADR 0001: MVVM with C++ view models

**Status:** accepted

## Context
The UI shows state that changes constantly (health, cooldowns, combo, clues) and must stay testable, localizable and
safe to restyle. Options: widgets reading gameplay directly, Blueprint-only view models, or C++ view models.

## Decision
View models are C++ `UMVVMViewModelBase` subclasses with `FieldNotify` properties, set with
`UE_MVVM_SET_PROPERTY_VALUE` so a notification fires only when a value actually changes. They hold presentation state
(percentages, formatted `FText`, visibility flags) and never reference widgets or the world. A per-player
`UGothamViewModelSubsystem` owns them and wires gameplay components in.

## Consequences
- Logic is unit-testable headlessly (`NewObject<UPlayerVitalsViewModel>` in an automation test).
- Views stay dumb; formatting and derived state (`HealthPercent`, `bIsLowHealth`, "x2") live in one place.
- Cost: more classes than widgets-reading-gameplay, and a small amount of wiring code in the subsystem.
- `UGothamViewModelResolver` lets designer-authored widgets resolve these view models in the MVVM editor.
