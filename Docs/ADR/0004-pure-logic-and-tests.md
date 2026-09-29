# ADR 0004: Keep rules in UObject-free code and test them

**Status:** accepted

## Decision
Anything with a rule worth testing (settings cycling and persistence, palette, binding conflict planning, wheel
hit-testing, layer/input-context tracking, cooldown/combo advance) is a plain function or struct, or exposes an
`Advance(DeltaTime)`-style method, so a test needs no world.

## Consequences
- 20+ fast automation tests with no map load. Unregistered components can be driven directly because `Advance()` is
  split from `TickComponent` (which asserts registration).
- Visual behaviour is verified separately with dev flags and screenshots, not asserted in tests.
