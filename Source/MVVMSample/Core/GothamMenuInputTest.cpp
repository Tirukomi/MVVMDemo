// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamMenuInputTest.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Slate/SObjectWidget.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/Widgets/GothamHintButton.h"
#include "UI/Widgets/GothamOptionRow.h"
#include "UI/Widgets/GothamTabList.h"
#include "UObject/UObjectIterator.h"
#include "ViewModels/SettingsViewModel.h"

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

	/** A left click through Slate's hit-testing (move, press, release), at a point given as a fraction of the widget. */
	bool Click(const UWidget* Widget, const FVector2D& Fraction = FVector2D(0.5, 0.5))
	{
		if (!Widget || !Widget->GetCachedWidget().IsValid())
		{
			return false;
		}
		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FVector2D Centre = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * Fraction;
		FSlateApplication& App = FSlateApplication::Get();
		const TSet<FKey> Pressed = { EKeys::LeftMouseButton };
		const TSet<FKey> Released;
		App.ProcessMouseMoveEvent(FPointerEvent(0, 0, Centre, Centre, Released, EKeys::Invalid, 0.f, App.GetModifierKeys()));
		App.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, 0, Centre, Centre, Pressed, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
		App.ProcessMouseButtonUpEvent(FPointerEvent(0, 0, Centre, Centre, Released, EKeys::LeftMouseButton, 0.f, App.GetModifierKeys()));
		return true;
	}

	/** The prompt with this keyboard key inside Screen (including nested widgets such as the tab list). */
	UGothamHintButton* FindHint(const UObject* Screen, const FKey& Key)
	{
		for (TObjectIterator<UGothamHintButton> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetKeyboardKey() == Key && It->IsIn(Screen))
			{
				return *It;
			}
		}
		return nullptr;
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
}

void FGothamMenuInputTest::Start(AGothamPlayerController* Controller)
{
	if (const TSharedPtr<FGothamScript> Script = Build(Controller, [](bool bPassed, const FString& Rule)
	{
		UE_LOG(LogGothamMenuTest, Display, TEXT("%s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Rule);
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
	TSharedPtr<FLinearColor> LabelBefore = MakeShared<FLinearColor>(FLinearColor::Transparent);
	auto Settings = [WeakPC]() { const UGothamSettingsSubsystem* S = UGothamSettingsSubsystem::Get(WeakPC.Get()); return S ? S->GetViewModel() : nullptr; };

	// 1. The case file's own key (J) closes it.
	Script->At(1.0f).Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->ToggleClueLog(); } });
	Script->At(2.5f).Do([Self]() { Self->Check(ActiveScreen<UClueLogScreen>() != nullptr, TEXT("J opens the case file (precondition)")); SendKey(EKeys::J); });
	Script->At(3.2f).Do([Self]() { Self->Check(ActiveScreen<UClueLogScreen>() == nullptr, TEXT("J again closes the case file")); });

	// 2. The pause key on the gamepad (Start) closes pause.
	Script->At(3.5f).Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } });
	Script->At(5.0f).Do([Self]() { Self->Check(ActiveScreen<UPauseMenuScreen>() != nullptr, TEXT("pause opens (precondition)")); SendKey(EKeys::Gamepad_Special_Right); });
	Script->At(5.7f).Do([Self]() { Self->Check(ActiveScreen<UPauseMenuScreen>() == nullptr, TEXT("Start closes pause")); });

	// 3. Settings: each prompt does what its key does.
	Script->At(6.0f).Do([WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass.LoadSynchronous()); } });
	// Keys straight to the focused row first: separates "the key never reaches the row" from "the click fails".
	Script->At(6.9f).Do([Self, Settings, ScaleBefore]() { *ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1; SendKey(EKeys::Right); });
	Script->At(7.1f).Do([Self, Settings, ScaleBefore]()
	{
		Self->Check(Settings() && Settings()->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("Right on a focused option row steps it"));
		*ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1;
		SendKey(EKeys::Enter);
	});
	Script->At(7.3f).Do([Self, Settings, ScaleBefore]()
	{
		Self->Check(Settings() && Settings()->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("Enter on a focused option row steps it"));
		if (Settings()) { Settings()->Revert(); }
	});
	// The selector's left half steps back. The selector (260 wide, inset 14) sits at the row's right edge; click a
	// quarter of the way into it.
	Script->At(7.35f).Do([Self, Settings, ScaleBefore]()
	{
		*ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1;
		UGothamOptionRow* Row = FindRow(ActiveScreen<USettingsScreen>(), EGothamSetting::UIScale);
		const FGeometry Geometry = Row ? Row->GetCachedGeometry() : FGeometry();
		const float SelectorLeft = Geometry.GetLocalSize().X - 14.f - 260.f;
		const float LeftQuarter = Geometry.GetLocalSize().X > 0.f ? (SelectorLeft + 260.f * 0.25f) / Geometry.GetLocalSize().X : 0.5f;
		Self->Check(Click(Row, FVector2D(LeftQuarter, 0.5)), TEXT("the UI scale row is clickable"));
	});
	Script->At(7.45f).Do([Self, Settings, ScaleBefore]()
	{
		Self->Check(Settings() && Settings()->GetCurrent().UIScaleIndex == *ScaleBefore - 1, TEXT("clicking the left half of a selector steps it back"));
		if (Settings()) { Settings()->Revert(); }
	});
	Script->At(7.5f).Do([Self, Settings, ScaleBefore]()
	{
		const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
		Self->Check(Screen != nullptr, TEXT("settings open (precondition)"));
		*ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1;
		const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetUserFocusedWidget(0);
		const UObject* FocusedObject = Focused.IsValid() && Focused->GetType() == TEXT("SObjectWidget") ? StaticCastSharedPtr<SObjectWidget>(Focused)->GetWidgetObject() : nullptr;
		UE_LOG(LogGothamMenuTest, Display, TEXT("focus before clicking [Enter]: %s"), FocusedObject ? *FocusedObject->GetName() : TEXT("not a user widget"));
		Self->Check(Click(FindHint(Screen, EKeys::Enter)), TEXT("the [Enter] prompt is clickable"));
	});
	Script->At(8.0f).Do([Self, Settings, ScaleBefore]()
	{
		USettingsViewModel* VM = Settings();
		Self->Check(VM && VM->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("clicking [Enter] Change steps the focused option (UI scale)"));
		if (VM) { VM->Revert(); }
		Self->Check(Click(FindHint(ActiveScreen<USettingsScreen>(), EKeys::E)), TEXT("the [E] tab prompt is clickable"));
	});
	Script->At(8.6f).Do([Self, Settings, LabelBefore]()
	{
		FName Tab;
		for (TObjectIterator<UGothamTabList> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsIn(ActiveScreen<USettingsScreen>())) { Tab = It->GetSelectedTabId(); }
		}
		Self->Check(Tab == TEXT("Accessibility"), TEXT("clicking [E] switches to the next tab"));

		// Live restyle: every widget subscribes to settings changes through FGothamSettingsListener. Turning high
		// contrast on must recolour an open screen's prompts right away (a missed subscription would leave them).
		*LabelBefore = PromptLabelColor(ActiveScreen<USettingsScreen>());
		if (USettingsViewModel* VM = Settings()) { VM->Cycle(EGothamSetting::HighContrast, +1); }
	});
	Script->At(8.9f).Do([Self, Settings, LabelBefore]()
	{
		const FLinearColor After = PromptLabelColor(ActiveScreen<USettingsScreen>());
		Self->Check(LabelBefore->A > 0.f && !After.Equals(*LabelBefore, 0.01f), TEXT("turning high contrast on restyles an open screen live"));
		if (USettingsViewModel* VM = Settings()) { VM->Revert(); }
		Self->Check(Click(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape)), TEXT("the [Esc] prompt is clickable"));
	});
	Script->At(9.6f).Do([Self]() { Self->Check(ActiveScreen<USettingsScreen>() == nullptr, TEXT("clicking [Esc] Back closes settings")); });

	Script->At(10.1f).Do([Self]()
	{
		UE_LOG(LogGothamMenuTest, Display, TEXT("Menu input test: %d passed, %d failed"), Self->GetPassed(), Self->GetFailed());
	});
	return Script;
}
