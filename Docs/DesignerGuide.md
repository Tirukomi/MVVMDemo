# Designer and content guide

How to change things without reading the C++ first. Everything here is checked against the current code.

## Add a clue

1. Add its text to `Source/MVVMSample/Gameplay/ClueStrings.cpp` (`LOCTABLE_SETSTRING("MvsClues", "<Id>.Title", ...)`
   and `<Id>.Body`).
2. Add a row to `CLUES` in `Scripts/CreateForensicAssets.py` (id, thumbnail, world position) and run
   `Scripts/CreateForensicAssets.bat` (editor closed). This creates the `UClueDataAsset`, places an `AClueActor`
   in `L_Arena` and references the string-table entries.
3. Run `Scripts/Localize.bat` so the new text is gathered into each culture's `.po`, then translate it there.

Or in the editor: create a *Clue Data Asset*, set `Clue Id`, `Title`, `Description` (use a string-table reference for
the text), optionally `Thumbnail`; drop an *AClueActor* into the level and assign the asset.

## Add or change a gadget

Gadgets are `FMvsGadgetDefinition`s on `UGadgetComponent` (name, cooldown, tint). The HUD slots, the wheel and the
subtitle/gadget bar view model all read from it, so adding an entry adds it everywhere. Slot count is dynamic.

## Restyle without code

| To change | Edit |
|---|---|
| Colours that carry meaning (health, warning, clue state) | `MvsPalette::Resolve` (`Accessibility/MvsSettingsTypes.cpp`) |
| Menu button look | `UMvsButton::ApplyState` draws from palette tokens; pick a kind with `SetKind` (`Standard`, `MenuItem`, `Tab`, `Danger`) |
| Menu highlight bar | `UMvsMenuList::ApplyLook` (shape, fill, accent bar, glow); slide speed is `FMvsSlideRect::Rate` |
| Menu backdrop | `BuildMenuFrame` blur strength per screen; `UMvsScrim` left/right alphas |
| Screen transitions | `MvsMotion::ScreenSeconds` / `ScreenSlide` (stack fade plus content slide; both off under reduced motion) |
| Thug placement | `AMvsGameMode::ThugSpots` (three candidate spots per thug; props are skipped) |
| Thug timing and reach | `FMvsThugBrain` (warning, stun, recover seconds), `FMvsAttackDirector` (gap, engage range), `AMvsThug` (damage, strike range), `UMvsThreatSubsystem::CounterRange` |
| Threat indicators | `SThreatIndicatorLayer` (prompt and arrow shapes), `UThreatIndicatorLayer::ArrowRange` |
| Hit-stop and shake | `MvsFeel::HitStopSeconds` / `HitStopDilation`, `FMvsTrauma::DrainPerSecond` |
| Settings tabs and grouping | `USettingsViewModel::GetTabs` (a test checks every setting is in exactly one tab) |
| Gadget wheel look | `FMvsGadgetWheelStyle` on the `UGadgetWheel` widget (radii, gap, colours, font) |
| Combo meter | `UComboMeter` properties (segments, colours, size) |
| Forensic post-process | `M_ForensicVision` (regenerate with `CreateForensicAssets.py`, or edit in the material editor) |
| Forensic overlay | `M_ForensicOverlay_UI`; parameters `Progress`, `Tint`, `MotionScale` |
| Case-file tile layout | subclass `WBP_ClueEntry` (a Blueprint over `UClueEntryWidget`, now an evidence-board tile; size in `UClueEntryWidget::TileWidth/TileHeight`) |

Never hard-code a colour that means something. Ask the palette for a token (`Good`, `Danger`, `Info`, `Warning`,
`Unscanned`, `Scanned`) so colour-blind presets and high contrast keep working.

## Add a setting

1. Add the value to `FMvsSettingsData` and an entry in `EMvsSetting` (`Accessibility/MvsSettingsTypes.h`),
   and extend `operator==`.
2. Add its row to `MvsSettingsTable::Get` (`Accessibility/MvsSettingsTable.cpp`), in enum order: config key,
   storage, label, description (under the `Mvs.Settings` localization namespace), choice count, whether it wraps,
   and get / set / format functions. Stepping, the selector pips, the value text and the config round trip all come
   from the row.
3. Put it in a tab in `USettingsViewModel::GetTabs`; the settings screen builds a row for it, bound to the option's
   own `USettingRowViewModel` (`USettingsViewModel::GetRow`), like every other. A designer-built row binds the same
   fields.
4. Read it wherever it matters through `UMvsSettingsSubsystem::Get(this)`. To react to changes, derive a
   widget from `UMvsSettingsAwareWidget` and override `OnSettingsApplied`, or, in any other class, keep an
   `FMvsSettingsListener` member and `Bind` it (it unsubscribes itself).
5. Add a case to `Source/MVVMSampleTests/Private/SettingsTests.cpp`.

## Add a rebindable action

Add a row to `MvsActions::GetTable` (`Input/MvsActionTable.cpp`) with its name, display text (under the
`Mvs.Bindings` localization namespace), default keys and `bRebindable = true`, then bind its handler by name in
`AMvsPlayerController::SetupInputComponent`. The action, its mappings and its Controls-screen row all come from the
table. Add the name to `Mvs.Characterization.InputBindings`, and never rename an existing one: saved rebinds refer
to it.

## Add a language

1. Add the culture to every `Config/Localization/Game_*.ini` (`CulturesToGenerate`), to
   `FMvsSettingsData::GetLanguages`, and to `+CulturesToStage` in `DefaultGame.ini`.
2. Run `Scripts/Localize.bat` once: it creates `Content/Localization/Game/<culture>/Game.po` with every string.
3. Translate the `msgstr` lines in that `.po` (any PO editor works) and run `Scripts/Localize.bat` again.
   Untranslated strings stay English rather than blank, and so does any string whose English changed after it was
   translated, until its `.po` entry is updated.

Check the result with `-MvsLanguage=<culture>`; use `en-XA` to see whether a layout survives 40% longer text.

## Add a HUD widget

Derive `UMvsSettingsAwareWidget` (restyles on settings changes), build the tree in `RebuildWidget`, bind to a view
model with `MvsMVVM::Bind(ViewModel, this, &UMyWidget::OnFieldChanged, { FVM::Health, ... })` (and
`MvsMVVM::Unbind(ViewModel, this)` before swapping or dropping it), and call `MvsUI::DisableTick(this)` if it has no widget animations
(`UMvsSettingsAwareWidget` already does). Place it in `UMvsHudWidget::RebuildWidget` and hand it its view model
in `NativeConstruct`. Add the view model to `UMvsViewModelSubsystem`. Do not read gameplay components from a widget.
