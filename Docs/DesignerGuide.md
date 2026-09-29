# Designer and content guide

How to change things without reading the C++ first. Everything here is checked against the current code.

## Add a clue

1. Add its text to `Source/MVVMSample/Gameplay/ClueStrings.cpp` (`LOCTABLE_SETSTRING("GothamClues", "<Id>.Title", ...)`
   and `<Id>.Body`).
2. Add a row to `CLUES` in `Scripts/CreateDetectiveAssets.py` (id, thumbnail, world position) and run
   `Scripts/CreateDetectiveAssets.bat` (editor closed). This creates the `UClueDataAsset`, places an `AClueActor`
   in `L_Arena` and references the string-table entries.
3. Run `Scripts/Localize.bat` so the new text is gathered and can be translated.

Or in the editor: create a *Clue Data Asset*, set `Clue Id`, `Title`, `Description` (use a string-table reference for
the text), optionally `Thumbnail`; drop an *AClueActor* into the level and assign the asset.

## Add or change a gadget

Gadgets are `FGothamGadgetDefinition`s on `UGadgetComponent` (name, cooldown, tint). The HUD slots, the wheel and the
subtitle/gadget bar view model all read from it, so adding an entry adds it everywhere. Slot count is dynamic.

## Restyle without code

| To change | Edit |
|---|---|
| Colours that carry meaning (health, warning, clue state) | `GothamPalette::Resolve` (`Accessibility/GothamSettingsTypes.cpp`) |
| Menu button look | `UGothamButtonStyle` (or subclass it in a Blueprint and set it on the button) |
| Gadget wheel look | `FGothamGadgetWheelStyle` on the `UGadgetWheel` widget (radii, gap, colours, font) |
| Combo meter | `UComboMeter` properties (segments, colours, size) |
| Detective post-process | `M_DetectiveVision` (regenerate with `CreateDetectiveAssets.py`, or edit in the material editor) |
| Detective overlay | `M_DetectiveOverlay_UI`; parameters `Progress`, `Tint`, `MotionScale` |
| Clue log row layout | subclass `WBP_ClueEntry` (it already exists as a Blueprint over `UClueEntryWidget`) |

Never hard-code a colour that means something. Ask the palette for a token (`Good`, `Danger`, `Info`, `Warning`,
`Unscanned`, `Scanned`) so colour-blind presets and high contrast keep working.

## Add a setting

1. Add the value to `FGothamSettingsData` and an entry in `EGothamSetting` (`Accessibility/GothamSettingsTypes.h`);
   extend `Cycle`, `operator==`, `LoadFromConfig` / `SaveToConfig`.
2. Add its label and value text in `USettingsViewModel` (`GetLabel`, `GetValueText`, a `FieldNotify` text property).
3. Add a row in `USettingsScreen` (both lists: construction and `NativeConstruct`), and subscribe the row to the new
   field in `UGothamOptionRow::Setup`.
4. Read it wherever it matters through `UGothamSettingsSubsystem::Get(this)` or, for widgets, derive from
   `UGothamSettingsAwareWidget` and override `OnSettingsApplied`.
5. Add a case to `Tests/SettingsTests.cpp`.

## Add a rebindable action

Create the `UInputAction` in `AGothamPlayerController::BuildInputAssets`, register it with `MakeActionMappable` (or
`MapPair` for a keyboard + gamepad pair), add a row to `GothamBindings::GetDefinitions`, and bind it. The controls
screen picks it up from the definition list.

## Add a language

1. Add the culture to `Config/Localization/Game_Gather.ini` and `Game_Compile.ini` (`CulturesToGenerate`), to
   `FGothamSettingsData::GetLanguages`, and to `+CulturesToStage` in `DefaultGame.ini`.
2. Add a translation table to `Scripts/TranslateLocalization.py` (keyed by source text).
3. Run `Scripts/Localize.bat`. Untranslated strings stay English rather than blank.

Check the result with `-GothamLanguage=<culture>`; use `en-XA` to see whether a layout survives 40% longer text.

## Add a HUD widget

Derive `UGothamSettingsAwareWidget` (restyles on settings changes), build the tree in `RebuildWidget`, bind to a view
model with `AddFieldValueChangedDelegate`, and call `GothamUI::DisableTick(this)` if it has no widget animations
(`UGothamSettingsAwareWidget` already does). Place it in `UGothamHudWidget::RebuildWidget` and hand it its view model
in `NativeConstruct`. Add the view model to `UGothamViewModelSubsystem`. Do not read gameplay components from a widget.
