# ADR 0006: Standard localization pipeline plus a string table for data-driven text

**Status:** accepted

## Decision
Use UE's own gather -> translate -> compile (`Config/Localization/Game_Gather.ini`, `Game_Compile.ini`) so the result
is real `.locres` files, live-switchable with `FInternationalization::SetCurrentCulture`. Code text uses `LOCTEXT`.
Text that lives in data assets (clue titles and descriptions) is a code-registered string table (`GothamClues`,
`Gameplay/ClueStrings.cpp`) that the gatherer understands and the assets reference by key. An `en-XA` pseudo-locale is
generated (accents, +40% length) to stress layouts.

## Consequences
- Adding a language is: add it to the gather config and `FGothamSettingsData::GetLanguages`, add a table to
  `TranslateLocalization.py`, run `Scripts/Localize.bat`.
- CJK renders through the engine's fallback font; a shipped title would set an explicit composite font.
- Widgets re-read text when settings change (`RefreshTexts` re-broadcasts every value even if the `FText` object is
  identical), so a language switch updates open screens.
