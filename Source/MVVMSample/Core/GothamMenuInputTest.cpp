// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamMenuInputTest.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Slate/SObjectWidget.h"
#include "Engine/UserInterfaceSettings.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/Widgets/GothamActionBar.h"
#include "UI/Widgets/GothamHintButton.h"
#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Widgets/GothamTabList.h"
#include "UObject/UObjectIterator.h"
#include "ViewModels/SettingsViewModel.h"
#include "Algo/Reverse.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/GothamFeel.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ClueEntryWidget.h"
#include "UI/Screens/ControlsScreen.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Widgets/GadgetWheel.h"
#include "UI/Widgets/GothamInputGlyph.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/GothamViewModelSubsystem.h"
#include "ViewModels/SubtitleViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamMenuTest, Log, All);

// Named (not anonymous) namespace: unity builds merge this file with others.
namespace GothamMenuInputTestPrivate
{
	template<typename T>
	T* ActiveScreen()
	{
		for (TObjectIterator<T> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsActivated())
			{
				return *It;
			}
		}
		return nullptr;
	}

	void SendKey(const FKey& Key)
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessKeyDownEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
		App.ProcessKeyUpEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
	}

	void MouseEvent(const FVector2D& At, bool bDown, bool bUp)
	{
		FSlateApplication& App = FSlateApplication::Get();
		const TSet<FKey> Pressed = { EKeys::LeftMouseButton };
		const TSet<FKey> Released;
		if (bDown)
		{
			App.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, 0, At, At, Pressed, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
		}
		else if (bUp)
		{
			App.ProcessMouseButtonUpEvent(FPointerEvent(0, 0, At, At, Released, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
		}
		else
		{
			App.ProcessMouseMoveEvent(FPointerEvent(0, 0, At, At, Released, EKeys::Invalid, 0.f, App.GetModifierKeys()));
		}
	}

	/**
	 * A left click the way a mouse makes one, through Slate's hit-testing: the pointer arrives, the button goes down on
	 * the next frame and up on the frame after. (Move, press and release in one instant is not something real input
	 * does, and Common UI buttons then miss the click now and then.) Fraction picks the point inside Target.
	 */
	void AddClick(FGothamScript& Script, TFunction<const UWidget*()> Target, const FString& Rule, TFunction<FVector2D(const FGeometry&)> Fraction = nullptr)
	{
		FGothamScript* Self = &Script;
		TSharedRef<FVector2D> At = MakeShared<FVector2D>(FVector2D::ZeroVector);
		// Slate treats the first click on an inactive window differently, so clicks need the game to be the active
		// application: leave the machine alone while this test runs.
		Script.WaitUntil([]() { return FSlateApplication::Get().IsActive(); }, 3.f, TEXT("the game window is the active application (precondition; leave the PC idle)"));
		Script.Do([Self, Target = MoveTemp(Target), Fraction = MoveTemp(Fraction), At, Rule]()
			{
				const UWidget* Widget = Target();
				const bool bClickable = Widget && Widget->GetCachedWidget().IsValid();
				if (bClickable)
				{
					const FGeometry& Geometry = Widget->GetCachedGeometry();
					*At = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * (Fraction ? Fraction(Geometry) : FVector2D(0.5, 0.5));
					MouseEvent(*At, false, false);
				}
				Self->Check(bClickable, Rule);
			})
			.WaitFrames(1)
			.Do([At]() { MouseEvent(*At, true, false); })
			.WaitFrames(1)
			.Do([At]() { MouseEvent(*At, false, true); });
	}

	/**
	 * The prompt showing this keyboard key inside Screen: one the screen's action bar is showing (the bar pools the
	 * prompts it replaces, so only its current entries count), or one of the tab list's.
	 */
	UGothamHintButton* FindHint(const UObject* Screen, const FKey& Key)
	{
		for (TObjectIterator<UGothamActionBar> Bar; Bar && Screen; ++Bar)
		{
			if (!Bar->HasAnyFlags(RF_ClassDefaultObject) && Bar->IsIn(Screen))
			{
				for (UGothamHintButton* Hint : Bar->GetTypedEntries<UGothamHintButton>())
				{
					if (Hint->IsVisible() && Hint->GetKeyboardKey() == Key)
					{
						return Hint;
					}
				}
			}
		}
		for (TObjectIterator<UGothamTabList> Tabs; Tabs && Screen; ++Tabs)
		{
			if (!Tabs->HasAnyFlags(RF_ClassDefaultObject) && Tabs->IsIn(Screen) && Tabs->WidgetTree)
			{
				UGothamHintButton* Found = nullptr;
				Tabs->WidgetTree->ForEachWidget([&Found, &Key](UWidget* Widget)
				{
					UGothamHintButton* Hint = Cast<UGothamHintButton>(Widget);
					Found = !Found && Hint && Hint->GetKeyboardKey() == Key ? Hint : Found;
				});
				if (Found)
				{
					return Found;
				}
			}
		}
		return nullptr;
	}

	/** The text of Hint's key cap. */
	FString GlyphText(const UGothamHintButton* Hint)
	{
		const UGothamInputGlyph* Glyph = nullptr;
		if (Hint && Hint->WidgetTree)
		{
			Hint->WidgetTree->ForEachWidget([&Glyph](UWidget* W) { Glyph = Glyph ? Glyph : Cast<UGothamInputGlyph>(W); });
		}
		const UTextBlock* Text = nullptr;
		if (Glyph && Glyph->WidgetTree)
		{
			Glyph->WidgetTree->ForEachWidget([&Text](UWidget* W) { Text = Text ? Text : Cast<UTextBlock>(W); });
		}
		return Text ? Text->GetText().ToString() : FString();
	}

	/** The settings row for one option, inside Screen. */
	UGothamOptionRow* FindRow(const UObject* Screen, EGothamSetting Setting)
	{
		for (TObjectIterator<UGothamOptionRow> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetSetting() == Setting && It->IsIn(Screen))
			{
				return *It;
			}
		}
		return nullptr;
	}

	/** The label colour of the [Esc] prompt in Screen (transparent if not found). */
	FLinearColor PromptLabelColor(const UObject* Screen)
	{
		const UGothamHintButton* Hint = FindHint(Screen, EKeys::Escape);
		UTextBlock* Label = nullptr;
		if (Hint && Hint->WidgetTree)
		{
			Hint->WidgetTree->ForEachWidget([&Label](UWidget* W) { if (!Label) { Label = Cast<UTextBlock>(W); } });
		}
		return Label ? Label->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Transparent;
	}

	/** True if the user's keyboard focus is Screen's widget or inside it. */
	bool HasFocusWithin(const UUserWidget* Screen)
	{
		const TSharedPtr<SWidget> ScreenSlate = Screen ? Screen->GetCachedWidget() : nullptr;
		for (TSharedPtr<SWidget> Widget = FSlateApplication::Get().GetUserFocusedWidget(0); Widget.IsValid() && ScreenSlate.IsValid(); Widget = Widget->GetParentWidget())
		{
			if (Widget == ScreenSlate)
			{
				return true;
			}
		}
		return false;
	}

	/**
	 * A condition that holds once Get's widget has kept the same on-screen position and size for two frames in a row:
	 * layout changes (UI scale), animated scrolling and sliding highlights take a few frames to settle, and a click
	 * aimed before that misses.
	 */
	TFunction<bool()> Stable(TFunction<const UWidget*()> Get)
	{
		TSharedRef<FVector4> Last = MakeShared<FVector4>(-1.0, -1.0, -1.0, -1.0);
		TSharedRef<int32> SameFrames = MakeShared<int32>(0);
		return [Get = MoveTemp(Get), Last, SameFrames]()
		{
			const UWidget* Widget = Get();
			if (!Widget || !Widget->GetCachedWidget().IsValid())
			{
				return false;
			}
			const FGeometry& Geometry = Widget->GetCachedGeometry();
			const FVector4 Now(Geometry.GetAbsolutePosition().X, Geometry.GetAbsolutePosition().Y, Geometry.GetAbsoluteSize().X, Geometry.GetAbsoluteSize().Y);
			if (Now.Equals(*Last, 0.01) && Now.Z > 0.0)
			{
				++*SameFrames;
			}
			else
			{
				*SameFrames = 0;
				*Last = Now;
			}
			return *SameFrames >= 2;
		};
	}

	void ReleaseKey(const FKey& Key)
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessKeyUpEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
	}

	FName SelectedTab(const UObject* Screen)
	{
		for (TObjectIterator<UGothamTabList> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && Screen && It->IsIn(Screen))
			{
				return It->GetSelectedTabId();
			}
		}
		return NAME_None;
	}

	template<typename T>
	T* FindIn(const UObject* Screen)
	{
		for (TObjectIterator<T> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && Screen && It->IsIn(Screen))
			{
				return *It;
			}
		}
		return nullptr;
	}
	const UGothamClueTileView* FindTileView(const UObject* Screen) { return FindIn<UGothamClueTileView>(Screen); }
	const UGadgetWheel* FindWheel(const UObject* Screen) { return FindIn<UGadgetWheel>(Screen); }

	UGothamViewModelSubsystem* ViewModelsOf(const TWeakObjectPtr<AGothamPlayerController>& PC)
	{
		const ULocalPlayer* LocalPlayer = PC.IsValid() ? PC->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UGothamViewModelSubsystem>() : nullptr;
	}

	UEnhancedInputUserSettings* UserSettingsOf(const TWeakObjectPtr<AGothamPlayerController>& PC)
	{
		const ULocalPlayer* LocalPlayer = PC.IsValid() ? PC->GetLocalPlayer() : nullptr;
		const auto* Input = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
		return Input ? Input->GetUserSettings() : nullptr;
	}

	void MapKeyboardSlot(UEnhancedInputUserSettings* UserSettings, FName Action, const FKey& Key)
	{
		FMapPlayerKeyArgs Args;
		Args.MappingName = Action;
		Args.Slot = EPlayerMappableKeySlot::First;
		Args.NewKey = Key;
		FGameplayTagContainer Failure;
		UserSettings->MapPlayerKey(Args, Failure);
	}

	/** Rebinds Action's keyboard slot in memory (never saved), remembering the key it had in Undo. */
	void Rebind(const TWeakObjectPtr<AGothamPlayerController>& PC, FName Action, const FKey& Key, TArray<TPair<FName, FKey>>& Undo)
	{
		UEnhancedInputUserSettings* UserSettings = UserSettingsOf(PC);
		const UEnhancedPlayerMappableKeyProfile* Profile = UserSettings ? UserSettings->GetActiveKeyProfile() : nullptr;
		const FKeyMappingRow* Row = Profile ? Profile->FindKeyMappingRow(Action) : nullptr;
		if (!Row)
		{
			return;
		}
		for (const FPlayerKeyMapping& Mapping : Row->Mappings)
		{
			if (Mapping.GetSlot() == EPlayerMappableKeySlot::First)
			{
				Undo.Add({ Action, Mapping.GetCurrentKey() });
			}
		}
		MapKeyboardSlot(UserSettings, Action, Key);
		UserSettings->ApplySettings();
	}

	/** Puts back exactly the keys Rebind replaced, so the player's own rebinds are untouched. */
	void RestoreBindings(const TWeakObjectPtr<AGothamPlayerController>& PC, TArray<TPair<FName, FKey>>& Undo)
	{
		if (UEnhancedInputUserSettings* UserSettings = UserSettingsOf(PC))
		{
			for (const TPair<FName, FKey>& Entry : Undo)
			{
				MapKeyboardSlot(UserSettings, Entry.Key, Entry.Value);
			}
			UserSettings->ApplySettings();
		}
		Undo.Reset();
	}
}

void FGothamMenuInputTest::Start(AGothamPlayerController* Controller)
{
	if (const TSharedPtr<FGothamScript> Script = Build(Controller, [](EGothamCheck Result, const FString& Rule)
	{
		UE_LOG(LogGothamMenuTest, Display, TEXT("%s: %s"), FGothamScript::ResultLabel(Result), *Rule);
	}))
	{
		Script->Quit();
		Script->Start();
	}
}

TSharedPtr<FGothamScript> FGothamMenuInputTest::Build(AGothamPlayerController* Controller, FGothamScript::FReporter Reporter)
{
	using namespace GothamMenuInputTestPrivate;
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UGothamUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamUISubsystem>() : nullptr;
	if (!UI)
	{
		return nullptr;
	}
	const TWeakObjectPtr<UGothamUISubsystem> WeakUI(UI);
	const TWeakObjectPtr<AGothamPlayerController> WeakPC(Controller);
	TSharedRef<FGothamScript> Script = MakeShared<FGothamScript>();
	// Steps use a raw pointer: the ticker owns the script, and steps capturing the shared ref would form a cycle.
	FGothamScript* Self = &Script.Get();
	Script->SetReporter(MoveTemp(Reporter));
	TSharedPtr<int32> ScaleBefore = MakeShared<int32>(0);
	TSharedPtr<float> LayoutScaleBefore = MakeShared<float>(0.f);
	TSharedPtr<FString> CultureBefore = MakeShared<FString>();
	TSharedPtr<FLinearColor> LabelBefore = MakeShared<FLinearColor>(FLinearColor::Transparent);
	auto Settings = [WeakPC]() { const UGothamSettingsSubsystem* S = UGothamSettingsSubsystem::Get(WeakPC.Get()); return S ? S->GetViewModel() : nullptr; };
	TSharedPtr<TArray<TPair<FName, FKey>>> Undo = MakeShared<TArray<TPair<FName, FKey>>>();
	auto Scale = [Settings]() { return Settings() ? Settings()->GetCurrent().UIScaleIndex : -1; };
	auto Push = [WeakUI](TSoftClassPtr<UCommonActivatableWidget> UGothamUISettings::* Class)
	{
		return [WeakUI, Class]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->*Class); } };
	};
	// A screen is ready for input once it is active, no layer is mid-transition (Common UI blocks input meanwhile),
	// and focus has landed inside it.
	auto Settled = [WeakUI](auto* Screen) { return Screen && WeakUI.IsValid() && !WeakUI->IsTransitioning() && HasFocusWithin(Screen); };
	auto Closed = [WeakUI](auto* Screen) { return !Screen && WeakUI.IsValid() && !WeakUI->IsTransitioning(); };
	constexpr float Open = 3.f;
	constexpr float Quick = 1.5f;

	// Review finding 23: screens load when the layout is created, before any key opens one.
	Script->Do([Self, WeakUI]()
	{
		Self->Check(WeakUI.IsValid() && WeakUI->AreScreensLoaded() && GetDefault<UGothamUISettings>()->ClueEntryClass.Get() != nullptr,
			TEXT("the screen classes are loaded before the first key press (review 23)"));
	});

	// 1. The case file's own key (J) closes it.
	Script->Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->ToggleClueLog(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UClueLogScreen>()); }, Open, TEXT("J opens the case file (precondition)"))
		.Do([]() { SendKey(EKeys::J); })
		.WaitUntil([Closed]() { return Closed(ActiveScreen<UClueLogScreen>()); }, Quick, TEXT("J again closes the case file"));

	// 2. The pause key on the gamepad (Start) closes pause.
	Script->Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens (precondition)"))
		.Do([]() { SendKey(EKeys::Gamepad_Special_Right); })
		.WaitUntil([Closed]() { return Closed(ActiveScreen<UPauseMenuScreen>()); }, Quick, TEXT("Start closes pause"));

	// 3. Settings: keys on a focused row, then each prompt does what its key does.
	Script->Do(Push(&UGothamUISettings::SettingsScreenClass))
		.WaitUntil([Settled]() { return Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open (precondition)"))
		// Keys straight to the focused row first: separates "the key never reaches the row" from "the click fails".
		.Do([Scale, ScaleBefore, LayoutScaleBefore]()
		{
			*ScaleBefore = Scale();
			const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
			*LayoutScaleBefore = Screen ? Screen->GetCachedGeometry().Scale : 0.f;
			SendKey(EKeys::Right);
		})
		.WaitUntil([Scale, ScaleBefore]() { return Scale() != *ScaleBefore; }, Quick, TEXT("Right on a focused option row steps it"))
		// Review finding 21: the preview scales the layout itself; the engine's UI settings are never written.
		.WaitUntil([LayoutScaleBefore]()
		{
			const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
			return Screen && !FMath::IsNearlyEqual(Screen->GetCachedGeometry().Scale, *LayoutScaleBefore)
				&& GetDefault<UUserInterfaceSettings>()->ApplicationScale == 1.f;
		}, Quick, TEXT("a UI scale preview scales the layout, not the engine's UI settings (review 21)"))
		.Do([Scale, ScaleBefore]() { *ScaleBefore = Scale(); SendKey(EKeys::Enter); })
		.WaitUntil([Scale, ScaleBefore]() { return Scale() != *ScaleBefore; }, Quick, TEXT("Enter on a focused option row steps it"))
		// The selector's left half steps back. The selector (260 wide, inset 14) sits at the row's right edge; click a
		// quarter of the way into it.
		// Changing the UI scale re-lays out everything; clicks wait for the new geometry.
		.Do([Settings]() { if (Settings()) { Settings()->Revert(); } })
		.WaitUntil(Stable([]() { return FindRow(ActiveScreen<USettingsScreen>(), EGothamSetting::UIScale); }), Quick, TEXT("the UI scale row settles (precondition)"))
		.Do([Scale, ScaleBefore]() { *ScaleBefore = Scale(); });
	AddClick(*Script, []() { return FindRow(ActiveScreen<USettingsScreen>(), EGothamSetting::UIScale); }, TEXT("the UI scale row is clickable"),
		[](const FGeometry& Geometry)
		{
			const float Width = Geometry.GetLocalSize().X;
			const float SelectorLeft = Width - 14.f - 260.f;
			return FVector2D(Width > 0.f ? (SelectorLeft + 260.f * 0.25f) / Width : 0.5f, 0.5);
		});
	Script->WaitUntil([Scale, ScaleBefore]() { return Scale() == *ScaleBefore - 1; }, Quick, TEXT("clicking the left half of a selector steps it back"))
		.Do([Settings]() { if (Settings()) { Settings()->Revert(); } })
		.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter); }), Quick, TEXT("the [Enter] prompt settles (precondition)"))
		.Do([Scale, ScaleBefore]() { *ScaleBefore = Scale(); });
	AddClick(*Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter); }, TEXT("the [Enter] prompt is clickable"));
	Script->WaitUntil([Scale, ScaleBefore]() { return Scale() != *ScaleBefore; }, Quick, TEXT("clicking [Enter] Change steps the focused option (UI scale)"))
		.Do([Settings]() { if (Settings()) { Settings()->Revert(); } })
		.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::E); }), Quick, TEXT("the [E] prompt settles (precondition)"));
	AddClick(*Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::E); }, TEXT("the [E] tab prompt is clickable"));
	Script->WaitUntil([WeakUI]() { return WeakUI.IsValid() && !WeakUI->IsTransitioning() && SelectedTab(ActiveScreen<USettingsScreen>()) == TEXT("Accessibility"); },
			Quick, TEXT("clicking [E] switches to the next tab"))
		// Live restyle: every widget subscribes to settings changes through FGothamSettingsListener. Turning high contrast
		// on must recolour an open screen's prompts right away (a missed subscription would leave them).
		.Do([Settings, LabelBefore]()
		{
			*LabelBefore = PromptLabelColor(ActiveScreen<USettingsScreen>());
			if (USettingsViewModel* VM = Settings()) { VM->Cycle(EGothamSetting::HighContrast, +1); }
		})
		.WaitUntil([LabelBefore]() { const FLinearColor Now = PromptLabelColor(ActiveScreen<USettingsScreen>()); return LabelBefore->A > 0.f && !Now.Equals(*LabelBefore, 0.01f); },
			Quick, TEXT("turning high contrast on restyles an open screen live"))
		.Do([Settings]() { if (Settings()) { Settings()->Revert(); } })
		// Prompts are the screen's bound actions, with the keys of the device in use (review 12, 15).
		.Do([]() { SendKey(EKeys::Gamepad_DPad_Down); })
		.WaitUntil([]() { return GlyphText(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter)) == UGothamInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom).ToString(); },
			Quick, TEXT("a gamepad press turns the accept prompt into the gamepad's accept button"))
		.Do([]() { SendKey(EKeys::Gamepad_DPad_Up); })
		.Do([]()
		{
			// A real move (the cursor travelled), which is what Common Input counts as switching to the mouse.
			FSlateApplication& App = FSlateApplication::Get();
			App.ProcessMouseMoveEvent(FPointerEvent(0, 0, FVector2D(60.0, 60.0), FVector2D(40.0, 40.0), TSet<FKey>(), EKeys::Invalid, 0.f, App.GetModifierKeys()));
		})
		.WaitUntil([]() { return GlyphText(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter)) == UGothamInputGlyph::GetKeyLabel(EKeys::Enter).ToString(); },
			Quick, TEXT("moving the mouse turns it back into Enter"));

	// Review finding 1: opening Key bindings must keep unapplied settings.
	Script->Do([Settings]() { if (USettingsViewModel* VM = Settings()) { VM->Cycle(EGothamSetting::SubtitleSize, +1); } })
		.Do(Push(&UGothamUISettings::ControlsScreenClass))
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UControlsScreen>()); }, Open, TEXT("key bindings open over settings (precondition)"))
		.Do([Self, Settings]() { Self->Check(Settings() && Settings()->GetIsDirty(), TEXT("opening Key bindings keeps unapplied settings (review 1)")); })
		.Do([]() { SendKey(EKeys::Escape); })
		.WaitUntil([Settled]() { return !ActiveScreen<UControlsScreen>() && Settled(ActiveScreen<USettingsScreen>()); }, Open,
			TEXT("Esc closes key bindings and only key bindings (review 11)"))
		.Do([Settings]() { if (Settings()) { Settings()->Revert(); } })
		.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape); }), Quick, TEXT("the [Esc] prompt settles (precondition)"));
	AddClick(*Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape); }, TEXT("the [Esc] prompt is clickable"));
	Script->WaitUntil([Closed]() { return Closed(ActiveScreen<USettingsScreen>()); }, Quick, TEXT("clicking [Esc] Back closes settings"));

	// Review finding 21: a language preview is live, and leaving settings without applying puts the culture back.
	Script->Do(Push(&UGothamUISettings::SettingsScreenClass))
		.WaitUntil([Settled]() { return Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open for the language preview (precondition)"))
		.Do([Settings, CultureBefore]()
		{
			*CultureBefore = FInternationalization::Get().GetCurrentCulture()->GetName();
			if (USettingsViewModel* VM = Settings()) { VM->Cycle(EGothamSetting::Language, +1); }
		})
		.WaitUntil([CultureBefore]() { return FInternationalization::Get().GetCurrentCulture()->GetName() != *CultureBefore; }, Quick,
			TEXT("a language preview switches the language live"))
		.Do([]() { if (USettingsScreen* Screen = ActiveScreen<USettingsScreen>()) { Screen->DeactivateWidget(); } })
		.WaitUntil([Closed, CultureBefore]() { return Closed(ActiveScreen<USettingsScreen>()) && FInternationalization::Get().GetCurrentCulture()->GetName() == *CultureBefore; },
			Open, TEXT("closing settings without applying restores the language (review 21)"));

	// Review findings 2 and 7: screens opened from pause keep the game paused, and the case file's key opens the case
	// file over pause instead of closing pause.
	Script->Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens again (precondition)"))
		.Do(Push(&UGothamUISettings::SettingsScreenClass))
		.WaitUntil([Closed, WeakUI]() { return ActiveScreen<USettingsScreen>() && WeakUI.IsValid() && !WeakUI->IsTransitioning(); }, Open, TEXT("settings open over pause (precondition)"))
		// Seen on some runs: focus lands on the game viewport, so a gamepad has nothing to move. The pause screen
		// deactivating underneath (finding 2) is the likely cause.
		.Do([Self]() { Self->Check(HasFocusWithin(ActiveScreen<USettingsScreen>()), TEXT("settings opened from pause receive focus (review 2)")); })
		.Do([Self, WeakPC]() { Self->Check(WeakPC.IsValid() && UGameplayStatics::IsGamePaused(WeakPC.Get()), TEXT("the game stays paused under settings opened from pause (review 2)")); })
		.Do([]() { if (USettingsScreen* Screen = ActiveScreen<USettingsScreen>()) { Screen->DeactivateWidget(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("back on pause (precondition)"))
		.Do([]() { SendKey(EKeys::J); })
		.WaitUntil([WeakUI]() { return WeakUI.IsValid() && !WeakUI->IsTransitioning(); }, Quick, TEXT("the case file key is handled (precondition)"))
		.Do([Self]() { Self->Check(ActiveScreen<UClueLogScreen>() != nullptr, TEXT("the case file key opens the case file over pause (review 7)")); })
		// One screen per settled frame: a closed screen leaves the stack only after its transition.
		.WaitUntil([WeakUI]()
		{
			if (!WeakUI.IsValid() || WeakUI->IsTransitioning())
			{
				return false;
			}
			return !WeakUI->PopTopScreen();
		}, Open, TEXT("menus close (cleanup)"));

	// Review finding 26: one background blur on screen. The quit confirmation over pause blurs; pause stops blurring
	// under it and blurs again once it closes.
	Script->Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens for the quit confirmation (precondition)"))
		.Do([]() { if (UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>()) { Pause->RequestQuit(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UConfirmModalScreen>()); }, Open, TEXT("the quit confirmation opens (precondition)"))
		.Do([Self]()
		{
			const UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>();
			const UConfirmModalScreen* Modal = ActiveScreen<UConfirmModalScreen>();
			Self->Check(Pause && Modal && Modal->IsBackdropBlurEnabled() && !Pause->IsBackdropBlurEnabled(),
				TEXT("under the quit confirmation only the confirmation blurs (review 26)"));
		})
		.Do([]() { SendKey(EKeys::Escape); })
		.WaitUntil([Settled]() { return !ActiveScreen<UConfirmModalScreen>() && Settled(ActiveScreen<UPauseMenuScreen>()); }, Open,
			TEXT("Esc answers the confirmation (precondition)"))
		.Do([Self]()
		{
			const UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>();
			Self->Check(Pause && Pause->IsBackdropBlurEnabled(), TEXT("pause blurs again once the confirmation closes (review 26)"));
		})
		.Do([]() { if (UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>()) { Pause->DeactivateWidget(); } })
		.WaitUntil([Closed]() { return Closed(ActiveScreen<UPauseMenuScreen>()); }, Quick, TEXT("pause closes (cleanup)"));

	// Review finding 9: the case file follows a changed list of the same length.
	Script->Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->ToggleClueLog(); } })
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UClueLogScreen>()); }, Open, TEXT("the case file opens again (precondition)"))
		.Do([Self, WeakPC]()
		{
			UClueListViewModel* Clues = ViewModelsOf(WeakPC) ? ViewModelsOf(WeakPC)->GetClues() : nullptr;
			if (!Clues || Clues->GetEntries().Num() < 2)
			{
				Self->Check(false, TEXT("the level has at least two clues (precondition)"));
				return;
			}
			TArray<TObjectPtr<UClueEntryViewModel>> Reversed = Clues->GetEntries();
			Algo::Reverse(Reversed);
			Clues->SetEntries(Reversed);
			const UGothamClueTileView* Tiles = FindTileView(ActiveScreen<UClueLogScreen>());
			Self->Check(Tiles && Tiles->GetItemAt(0) == Reversed[0], TEXT("the case file shows a reordered clue list (review 9)"));
			Algo::Reverse(Reversed);
			Clues->SetEntries(Reversed);
		})
		.Do([]() { SendKey(EKeys::Virtual_Gamepad_Back.GetVirtualKey()); })
		.WaitUntil([Closed]() { return Closed(ActiveScreen<UClueLogScreen>()); }, Quick, TEXT("the gamepad's back button closes the case file (review 11)"));

	// Review finding 8: every HUD view model can be resolved by class (for designer bindings).
	Script->Do([Self, WeakPC]()
	{
		const UGothamViewModelSubsystem* ViewModels = ViewModelsOf(WeakPC);
		Self->Check(ViewModels && ViewModels->FindViewModel(USubtitleViewModel::StaticClass()) != nullptr, TEXT("the resolver finds the subtitles view model (review 8)"));
	});

	// Review finding 18: views use gadgets through the gadget bar's command, which the gadget binder hands to gameplay.
	Script->Do([Self, WeakPC]()
		{
			UGadgetBarViewModel* Bar = ViewModelsOf(WeakPC) ? ViewModelsOf(WeakPC)->GetGadgetBar() : nullptr;
			Self->Check(Bar && Bar->GetSlot(0) && Bar->GetSlot(0)->GetIsReady(), TEXT("the first gadget is ready (precondition)"));
			if (Bar) { Bar->RequestUse(0); }
		})
		.WaitUntil([WeakPC]()
		{
			const UGadgetBarViewModel* Bar = ViewModelsOf(WeakPC) ? ViewModelsOf(WeakPC)->GetGadgetBar() : nullptr;
			return Bar && Bar->GetSlot(0) && !Bar->GetSlot(0)->GetIsReady();
		}, Quick, TEXT("the gadget bar's use command reaches gameplay (review 18)"));

	// Review findings 3, 4, 6 and 10: the gadget wheel and gadget keys follow rebinding, the wheel's slow motion survives
	// a hit-stop, and the wheel follows reduced motion live. Rebinds are in memory only (never saved) and undone.
	Script->Do([WeakPC, Undo]()
		{
			Rebind(WeakPC, TEXT("GadgetWheel"), EKeys::Z, *Undo);
			Rebind(WeakPC, TEXT("Gadget1"), EKeys::X, *Undo);
		})
		.Do([Self, WeakPC]()
		{
			const UGothamViewModelSubsystem* ViewModels = ViewModelsOf(WeakPC);
			const UGadgetSlotViewModel* First = ViewModels ? ViewModels->GetGadgetBar()->GetSlot(0) : nullptr;
			Self->Check(First && First->GetHotkey().ToString() == UGothamInputGlyph::GetKeyLabel(EKeys::X).ToString(), TEXT("the HUD's gadget key hint follows rebinding (review 4)"));
		})
		.Do([WeakPC, WeakUI]()
		{
			GothamFeel::HitStop(WeakPC.Get(), GothamFeel::HitStopSeconds);
			if (WeakUI.IsValid()) { WeakUI->OpenGadgetWheel(); }
		})
		.WaitUntil([Settled]() { return Settled(ActiveScreen<UGadgetWheelScreen>()); }, Open, TEXT("the gadget wheel opens (precondition)"))
		.Wait(GothamFeel::HitStopSeconds * 3.f)
		.Do([Self, WeakPC, Settings]()
		{
			Self->Check(WeakPC.IsValid() && UGameplayStatics::GetGlobalTimeDilation(WeakPC.Get()) < 0.5f, TEXT("the wheel's slow motion survives a hit-stop (review 6)"));
			if (USettingsViewModel* VM = Settings()) { VM->Cycle(EGothamSetting::ReducedMotion, +1); }
		})
		.Do([Self, Settings]()
		{
			const UGadgetWheel* Wheel = FindWheel(ActiveScreen<UGadgetWheelScreen>());
			Self->Check(Wheel && Wheel->bReduceMotion, TEXT("the open wheel follows reduced motion live (review 10)"));
			if (Settings()) { Settings()->Revert(); }
			ReleaseKey(EKeys::Z);
		})
		.Wait(0.5f)
		.Do([Self]() { Self->Check(ActiveScreen<UGadgetWheelScreen>() == nullptr, TEXT("releasing the rebound wheel key closes the wheel (review 3)")); })
		.Do([WeakPC, Undo]()
		{
			if (UGadgetWheelScreen* Wheel = ActiveScreen<UGadgetWheelScreen>()) { Wheel->DeactivateWidget(); }
			RestoreBindings(WeakPC, *Undo);
		})
		.WaitUntil([Closed]() { return Closed(ActiveScreen<UGadgetWheelScreen>()); }, Quick, TEXT("the wheel closes (cleanup)"));

	Script->Do([Self]()
	{
		UE_LOG(LogGothamMenuTest, Display, TEXT("Menu input test: %d passed, %d failed, %d known bugs"), Self->GetPassed(), Self->GetFailed(), Self->GetKnownBugs());
	});
	return Script;
}
