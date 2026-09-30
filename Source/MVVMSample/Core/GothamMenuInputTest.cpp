// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/GothamMenuInputTest.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "Containers/Ticker.h"
#include "Core/GothamPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformMisc.h"
#include "Slate/SObjectWidget.h"
#include "UI/GothamUISettings.h"
#include "UI/Layout/GothamUISubsystem.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/Widgets/GothamHintButton.h"
#include "UI/Widgets/GothamTabList.h"
#include "UObject/UObjectIterator.h"
#include "ViewModels/SettingsViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogGothamMenuTest, Log, All);

namespace
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

	/** A left click at the widget's centre through Slate's hit-testing (move, press, release). */
	bool Click(const UWidget* Widget)
	{
		if (!Widget || !Widget->GetCachedWidget().IsValid())
		{
			return false;
		}
		const FGeometry& Geometry = Widget->GetCachedGeometry();
		const FVector2D Centre = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
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

	struct FRun : TSharedFromThis<FRun>
	{
		struct FStep { float At; TFunction<void()> Do; };
		TArray<FStep> Steps;
		int32 Next = 0;
		float Elapsed = 0.f;
		int32 Passed = 0;
		int32 Failed = 0;
		FTSTicker::FDelegateHandle Handle;

		void Check(bool bOk, const TCHAR* Rule)
		{
			(bOk ? Passed : Failed)++;
			UE_LOG(LogGothamMenuTest, Display, TEXT("%s: %s"), bOk ? TEXT("PASS") : TEXT("FAIL"), Rule);
		}

		bool Tick(float Dt)
		{
			Elapsed += Dt;
			while (Steps.IsValidIndex(Next) && Elapsed >= Steps[Next].At)
			{
				Steps[Next++].Do();
			}
			return Steps.IsValidIndex(Next);
		}
	};
}

void FGothamMenuInputTest::Start(AGothamPlayerController* Controller)
{
	ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
	UGothamUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UGothamUISubsystem>() : nullptr;
	if (!UI)
	{
		return;
	}
	const TWeakObjectPtr<UGothamUISubsystem> WeakUI(UI);
	const TWeakObjectPtr<AGothamPlayerController> WeakPC(Controller);
	TSharedRef<FRun> Run = MakeShared<FRun>();
	// Steps use a raw pointer: the ticker below owns the run, and steps capturing the shared ref would form a cycle.
	FRun* Self = &Run.Get();
	TSharedPtr<int32> ScaleBefore = MakeShared<int32>(0);
	auto Settings = [WeakPC]() { const UGothamSettingsSubsystem* S = UGothamSettingsSubsystem::Get(WeakPC.Get()); return S ? S->GetViewModel() : nullptr; };

	// 1. The case file's own key (J) closes it.
	Run->Steps.Add({ 1.0f, [WeakUI]() { if (WeakUI.IsValid()) { WeakUI->ToggleClueLog(); } } });
	Run->Steps.Add({ 2.5f, [Self]() { Self->Check(ActiveScreen<UClueLogScreen>() != nullptr, TEXT("J opens the case file (precondition)")); SendKey(EKeys::J); } });
	Run->Steps.Add({ 3.2f, [Self]() { Self->Check(ActiveScreen<UClueLogScreen>() == nullptr, TEXT("J again closes the case file")); } });

	// 2. The pause key on the gamepad (Start) closes pause.
	Run->Steps.Add({ 3.5f, [WeakUI]() { if (WeakUI.IsValid()) { WeakUI->TogglePauseMenu(); } } });
	Run->Steps.Add({ 5.0f, [Self]() { Self->Check(ActiveScreen<UPauseMenuScreen>() != nullptr, TEXT("pause opens (precondition)")); SendKey(EKeys::Gamepad_Special_Right); } });
	Run->Steps.Add({ 5.7f, [Self]() { Self->Check(ActiveScreen<UPauseMenuScreen>() == nullptr, TEXT("Start closes pause")); } });

	// 3. Settings: each prompt does what its key does.
	Run->Steps.Add({ 6.0f, [WeakUI]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EGothamUILayer::Menu, GetDefault<UGothamUISettings>()->SettingsScreenClass.LoadSynchronous()); } } });
	// Keys straight to the focused row first: separates "the key never reaches the row" from "the click fails".
	Run->Steps.Add({ 6.9f, [Self, Settings, ScaleBefore]() { *ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1; SendKey(EKeys::Right); } });
	Run->Steps.Add({ 7.1f, [Self, Settings, ScaleBefore]()
	{
		Self->Check(Settings() && Settings()->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("Right on a focused option row steps it"));
		*ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1;
		SendKey(EKeys::Enter);
	} });
	Run->Steps.Add({ 7.3f, [Self, Settings, ScaleBefore]()
	{
		Self->Check(Settings() && Settings()->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("Enter on a focused option row steps it"));
		if (Settings()) { Settings()->Revert(); }
	} });
	Run->Steps.Add({ 7.5f, [Self, Settings, ScaleBefore]()
	{
		const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
		Self->Check(Screen != nullptr, TEXT("settings open (precondition)"));
		*ScaleBefore = Settings() ? Settings()->GetCurrent().UIScaleIndex : -1;
		const TSharedPtr<SWidget> Focused = FSlateApplication::Get().GetUserFocusedWidget(0);
		const UObject* FocusedObject = Focused.IsValid() && Focused->GetType() == TEXT("SObjectWidget") ? StaticCastSharedPtr<SObjectWidget>(Focused)->GetWidgetObject() : nullptr;
		UE_LOG(LogGothamMenuTest, Display, TEXT("focus before clicking [Enter]: %s"), FocusedObject ? *FocusedObject->GetName() : TEXT("not a user widget"));
		Self->Check(Click(FindHint(Screen, EKeys::Enter)), TEXT("the [Enter] prompt is clickable"));
	} });
	Run->Steps.Add({ 8.0f, [Self, Settings, ScaleBefore]()
	{
		USettingsViewModel* VM = Settings();
		Self->Check(VM && VM->GetCurrent().UIScaleIndex != *ScaleBefore, TEXT("clicking [Enter] Change steps the focused option (UI scale)"));
		if (VM) { VM->Revert(); }
		Self->Check(Click(FindHint(ActiveScreen<USettingsScreen>(), EKeys::E)), TEXT("the [E] tab prompt is clickable"));
	} });
	Run->Steps.Add({ 8.6f, [Self]()
	{
		FName Tab;
		for (TObjectIterator<UGothamTabList> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsIn(ActiveScreen<USettingsScreen>())) { Tab = It->GetSelectedTabId(); }
		}
		Self->Check(Tab == TEXT("Accessibility"), TEXT("clicking [E] switches to the next tab"));
		Self->Check(Click(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape)), TEXT("the [Esc] prompt is clickable"));
	} });
	Run->Steps.Add({ 9.3f, [Self]() { Self->Check(ActiveScreen<USettingsScreen>() == nullptr, TEXT("clicking [Esc] Back closes settings")); } });

	Run->Steps.Add({ 9.8f, [Self]()
	{
		UE_LOG(LogGothamMenuTest, Display, TEXT("Menu input test: %d passed, %d failed"), Self->Passed, Self->Failed);
		FPlatformMisc::RequestExit(false);
	} });

	Run->Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Dt) { return Run->Tick(Dt); }));
}
