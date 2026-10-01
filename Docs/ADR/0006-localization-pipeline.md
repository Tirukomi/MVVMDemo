# ADR 0006: Standard localization pipeline plus a string table for data-driven text

**Status:** accepted

## Decision
Use UE's own gather -> translate -> compile (`Config/Localization/Game_Gather.ini`, `Game_Compile.ini`) so the result
is real `.locres` files, live-switchable with `FInternationalization::SetCurrentCulture`. Code text uses `LOCTEXT`.
Text that lives in data assets (clue titles and descriptions) is a code-registered string table (`MvsClues`,
`Gameplay/ClueStrings.cpp`) that the gatherer understands and the assets reference by key. An `en-XA` pseudo-locale is
generated (accents, +40% length) to stress layouts.

## Consequences
- Adding a language is: add it to the localization configs and `FMvsSettingsData::GetLanguages`, run
  `Scripts/Localize.bat`, translate the new `.po`, run it again.
- **Update (refactoring pass P8):** translations live in `Content/Localization/Game/<culture>/Game.po`, imported and
  exported by the engine (`Game_ImportPO.ini`, `Game_ExportPO.ini`). This replaced `TranslateLocalization.py`, whose
  tables were keyed by English text and had to re-point entries whose source changed. The engine now handles that
  case: a translation made against old English is not compiled in, so the string shows in English until its `.po`
  entry is updated (verified). `Scripts/PseudoLocalize.py` still generates the `en-XA` `.po`. Switching produced
  byte-identical `.locres` files for every culture.
- CJK renders through the engine's fallback font; a shipped title would set an explicit composite font.
- Widgets re-read text when the language changes (`USettingsViewModel::RefreshTexts` bumps `Revision`, which every
  settings row and the settings screen follow), so a language switch updates open screens.
