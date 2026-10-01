# Designer and content guide

How to change things without reading the C++ first. Everything here is checked against the current code.

## Add a clue

1. Add its text to `Source/MVVMSample/Gameplay/ClueStrings.cpp` (`LOCTABLE_SETSTRING("GothamClues", "<Id>.Title", ...)`
   and `<Id>.Body`).
2. Add a row to `CLUES` in `Scripts/CreateDetectiveAssets.py` (id, thumbnail, world position) and run
   `Scripts/CreateDetectiveAssets.bat` (editor closed). This creates the `UClueDataAsset`, places an `AClueActor`
   in `L_Arena` and references the string-table entries.
3. Run `Scripts/Localize.bat` so the new text is gathered into each culture's `.po`, then translate it there.

Or in the editor: create a *Clue Data Asset*, set `Clue Id`, `Title`, `Description` (use a string-table reference for
the text), optionally `Thumbnail`; drop an *AClueActor* into the level and assign the asset.

## Add or change a gadget

Gadgets are `FGothamGadgetDefinition`s on `UGadgetComponent` (name, cooldown, tint). The HUD slots, the wheel and the
subtitle/gadget bar view model all read from it, so adding an entry adds it everywhere. Slot count is dynamic.

## Restyle without code

| To change | Edit |
|---|---|
| Colours that carry meaning (health, warning, clue state) | `GothamPalette::Resolve` (`Accessibility/GothamSettingsTypes.cpp`) |
| Menu button look | `UGothamButton::ApplyState` draws from palette tokens; pick a kind with `SetKind` (`Standard`, `MenuItem`, `Tab`, `Danger`) |
| Menu highlight bar | `UGothamMenuList::ApplyLook` (shape, fill, accent bar, glow); slide speed is `FGothamSlideRect::Rate` |
| Menu backdrop | `BuildMenuFrame` blur strength per screen; `UGothamScrim` left/right alphas |
| Screen transitions | `GothamMotion::ScreenSeconds` / `ScreenSlide` (stack fade plus content slide; both off under reduced motion) |
| Thug placement | `AGothamGameMode::ThugSpots` (three candidate spots per thug; props are skipped) |
| Thug timing and reach | `FGothamThugBrain` (warning, stun, recover seconds), `FGothamAttackDirector` (gap, engage range), `AGothamThug` (damage, strike range), `UGothamThreatSubsystem::CounterRange` |
| Threat indicators | `SThreatIndicatorLayer` (prompt and arrow shapes), `UThreatIndicatorLayer::ArrowRange` |
| Hit-stop and shake | `GothamFeel::HitStopSeconds` / `HitStopDilation`, `FGothamTrauma::DrainPerSecond` |
| Settings tabs and grouping | `USettingsViewModel::GetTabs` (a test checks every setting is in exactly one tab) |
| Gadget wheel look | `FGothamGadgetWheelStyle` on the `UGadgetWheel` widget (radii, gap, colours, font) |
| Combo meter | `UComboMeter` properties (segments, colours, size) |
| Detective post-process | `M_DetectiveVision` (regenerate with `CreateDetectiveAssets.py`, or edit in the material editor) |
| Detective overlay | `M_DetectiveOverlay_UI`; parameters `Progress`, `Tint`, `MotionScale` |
| Case-file tile layout | subclass `WBP_ClueEntry` (a Blueprint over `UClueEntryWidget`, now an evidence-board tile; size in `UClueEntryWidget::TileWidth/TileHeight`) |

Never hard-code a colour that means something. Ask the palette for a token (`Good`, `Danger`, `Info`, `Warning`,
`Unscanned`, `Scanned`) so colour-blind presets and high contrast keep working.

## Add a setting

1. Add the value to `FGothamSettingsData` and an entry in `EGothamSetting` (`Accessibility/GothamSettingsTypes.h`),
   and extend `operator==`.
2. Add its row to `GothamSettingsTable::Get` (`Accessibility/GothamSettingsTable.cpp`), in enum order: config key,
   storage, label, description (under the `Gotham.Settings` localization namespace), choice count, whether it wraps,
   and get / set / format functions. Stepping, the selector pips, the value text and the config round trip all come
   from the row.
3. Put it in a tab in `USettingsViewModel::GetTabs`; the settings screen builds a row for it, and the row refreshes
   on the view model's `Revision` like every other.
4. Read it wherever it matters through `UGothamSettingsSubsystem::Get(this)`. To react to changes, derive a
   widget from `UGothamSettingsAwareWidget` and override `OnSettingsApplied`, or, in any other class, keep an
   `FGothamSettingsListener` member and `Bind` it (it unsubscribes itself).
5. Add a case to `Source/MVVMSampleTests/Private/SettingsTests.cpp`.

## Add a rebindable action

Add a row to `GothamActions::GetTable` (`Input/GothamActionTable.cpp`) with its name, display text (under the
`Gotham.Bindings` localization namespace), default keys and `bRebindable = true`, then bind its handler by name in
`AGothamPlayerController::SetupInputComponent`. The action, its mappings and its Controls-screen row all come from the
table. Add the name to `Gotham.Characterization.InputBindings`, and never rename an existing one: saved rebinds refer
to it.

## Add a language

1. Add the culture to every `Config/Localization/Game_*.ini` (`CulturesToGenerate`), to
   `FGothamSettingsData::GetLanguages`, and to `+CulturesToStage` in `DefaultGame.ini`.
2. Run `Scripts/Localize.bat` once: it creates `Content/Localization/Game/<culture>/Game.po` with every string.
3. Translate the `msgstr` lines in that `.po` (any PO editor works) and run `Scripts/Localize.bat` again.
   Untranslated strings stay English rather than blank, and so does any string whose English changed after it was
   translated, until its `.po` entry is updated.

Check the result with `-GothamLanguage=<culture>`; use `en-XA` to see whether a layout survives 40% longer text.

## Add a HUD widget

Derive `UGothamSettingsAwareWidget` (restyles on settings changes), build the tree in `RebuildWidget`, bind to a view
model with `GothamMVVM::Bind(ViewModel, this, &UMyWidget::OnFieldChanged, { FVM::Health, ... })` (and
`GothamMVVM::Unbind(ViewModel, this)` before swapping or dropping it), and call `GothamUI::DisableTick(this)` if it has no widget animations
(`UGothamSettingsAwareWidget` already does). Place it in `UGothamHudWidget::RebuildWidget` and hand it its view model
in `NativeConstruct`. Add the view model to `UGothamViewModelSubsystem`. Do not read gameplay components from a widget.
