// Copyright IG. All Rights Reserved.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Core/MvsPlayerController.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/MvsAccessibility.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Widgets/MvsActionBar.h"
#include "UI/Widgets/MvsHintButton.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "UI/Widgets/MvsOptionRow.h"
#include "UI/Widgets/MvsTabList.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/SettingsViewModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsFunctional, Log, All);

namespace MvsMenuTest
{
	void FRig::Check(bool bPassed, const FString& Rule) const
	{
		Self->Check(bPassed, Rule);
	}

	UMvsViewModelSubsystem* FRig::ViewModels() const
	{
		const ULocalPlayer* LocalPlayer = PC.IsValid() ? PC->GetLocalPlayer() : nullptr;
		return LocalPlayer ? LocalPlayer->GetSubsystem<UMvsViewModelSubsystem>() : nullptr;
	}

	USettingsViewModel* FRig::Settings() const
	{
		const UMvsSettingsSubsystem* Subsystem = UMvsSettingsSubsystem::Get(PC.Get());
		return Subsystem ? Subsystem->GetViewModel() : nullptr;
	}

	int32 FRig::Scale() const
	{
		const USettingsViewModel* VM = Settings();
		return VM ? VM->GetCurrent().UIScaleIndex : -1;
	}

	FMvsScript::FAction FRig::Push(TSoftClassPtr<UCommonActivatableWidget> UMvsUISettings::* Class) const
	{
		return [WeakUI = UI, Class]() { if (WeakUI.IsValid()) { WeakUI->PushScreen(EMvsUILayer::Menu, GetDefault<UMvsUISettings>()->*Class); } };
	}

	bool FRig::Settled(const UUserWidget* Screen) const
	{
		return Screen && UI.IsValid() && !UI->IsTransitioning() && HasFocusWithin(Screen);
	}

	bool FRig::Closed(const UUserWidget* Screen) const
	{
		return !Screen && UI.IsValid() && !UI->IsTransitioning();
	}

	namespace
	{
		AMvsPlayerController* FindPlayer()
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				UWorld* World = Context.World();
				if (World && World->IsGameWorld())
				{
					if (AMvsPlayerController* PC = Cast<AMvsPlayerController>(GEngine->GetFirstLocalPlayerController(World)))
					{
						return PC->GetPawn() ? PC : nullptr;
					}
				}
			}
			return nullptr;
		}

		/** Closes menus one per settled frame (a closed screen leaves the stack only after its transition). */
		void AddCloseMenus(FMvsScript& Script, const FRig& Rig, const FString& Rule)
		{
			Script.WaitUntil([Rig]()
				{
					if (!Rig.UI.IsValid() || Rig.UI->IsTransitioning())
					{
						return false;
					}
					return !Rig.UI->PopTopScreen();
				}, Open, Rule)
				.Do([Rig]() { if (USettingsViewModel* VM = Rig.Settings()) { VM->Revert(); } });
		}

		TSharedPtr<FMvsScript> MakeScript(AMvsPlayerController* Controller, FMvsScript::FReporter Reporter, const FString& Name, FBody Body)
		{
			ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
			UMvsUISubsystem* UI = LocalPlayer ? LocalPlayer->GetSubsystem<UMvsUISubsystem>() : nullptr;
			if (!UI)
			{
				return nullptr;
			}
			TSharedRef<FMvsScript> Script = MakeShared<FMvsScript>();
			Script->SetReporter(MoveTemp(Reporter));
			FRig Rig;
			Rig.Self = &Script.Get();
			Rig.UI = UI;
			Rig.PC = Controller;
			// Each test starts where a player would: no menu open, nothing previewed.
			AddCloseMenus(*Script, Rig, TEXT("no menu is open (setup)"));
			Body(*Script, Rig);
			AddCloseMenus(*Script, Rig, TEXT("menus close (cleanup)"));
			FMvsScript* Self = &Script.Get();
			Script->Do([Self, Name]()
			{
				UE_LOG(LogMvsFunctional, Display, TEXT("%s: %d passed, %d failed, %d known bugs"), *Name, Self->GetPassed(), Self->GetFailed(), Self->GetKnownBugs());
			});
			return Script;
		}

		class FRunScript : public IAutomationLatentCommand
		{
		public:
			FRunScript(FAutomationTestBase* InTest, const TCHAR* InName, FBody InBody, double InTimeoutSeconds)
				: Test(InTest), Name(InName), Body(InBody), Deadline(FPlatformTime::Seconds() + InTimeoutSeconds) {}

			virtual ~FRunScript() override
			{
				if (bInputChanged && FSlateApplication::IsInitialized())
				{
					FSlateApplication::Get().SetHandleDeviceInputWhenApplicationNotActive(bInputBefore);
				}
			}

			virtual bool Update() override
			{
				if (FPlatformTime::Seconds() > Deadline)
				{
					Test->AddError(Script ? TEXT("the script did not finish in time") : TEXT("no game world with a Mvs player (run in -game)"));
					return true;
				}
				if (!Script)
				{
					AMvsPlayerController* PC = FindPlayer();
					if (!PC)
					{
						return false;
					}
					if (ReadyAt == 0.0)
					{
						ReadyAt = FPlatformTime::Seconds();
					}
					// Once per game session: the tests after the first find the HUD's intro already over.
					static bool bSettled = false;
					if (!bSettled && FPlatformTime::Seconds() - ReadyAt < SettleSeconds)
					{
						return false;
					}
					bSettled = true;
					FAutomationTestBase* T = Test;
					Script = MakeScript(PC, [T](EMvsCheck Result, const FString& Rule)
					{
						// A known bug is recorded, not failed; a known bug that now passes fails, so it gets promoted.
						const FString Line = FString::Printf(TEXT("%s: %s"), FMvsScript::ResultLabel(Result), *Rule);
						// Also in the log: if the game hangs, the report is never written, but the log shows how far it got.
						UE_LOG(LogMvsFunctional, Display, TEXT("%s"), *Line);
						if (Result == EMvsCheck::Passed || Result == EMvsCheck::KnownBug) { T->AddInfo(Line); }
						else { T->AddError(Line); }
					}, Name, Body);
					if (!Script)
					{
						Test->AddError(TEXT("could not build the script (no UI subsystem)"));
						return true;
					}
					// Second review 18: Slate skips mouse capture while another application is active, so a click's
					// release never reached the button and the tests needed the PC left alone. The engine's own switch
					// for input while inactive lifts that for the test's duration.
					FSlateApplication& App = FSlateApplication::Get();
					bInputBefore = App.GetHandleDeviceInputWhenApplicationNotActive();
					bInputChanged = true;
					App.SetHandleDeviceInputWhenApplicationNotActive(true);
					Script->Start();
					return false;
				}
				if (!Script->IsFinished())
				{
					return false;
				}
				if (Script->GetPassed() + Script->GetFailed() + Script->GetKnownBugs() == 0)
				{
					Test->AddError(TEXT("the script finished without checking anything"));
				}
				return true;
			}

		private:
			static constexpr double SettleSeconds = 2.0;
			FAutomationTestBase* Test;
			FString Name;
			FBody Body;
			double Deadline;
			double ReadyAt = 0.0;
			bool bInputBefore = false;
			bool bInputChanged = false;
			TSharedPtr<FMvsScript> Script;
		};
	}

	void Run(FAutomationTestBase* Test, const TCHAR* Name, FBody Body)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FRunScript(Test, Name, Body, 60.0));
	}

	void SendKey(const FKey& Key)
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessKeyDownEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
		App.ProcessKeyUpEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
	}

	void ReleaseKey(const FKey& Key)
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessKeyUpEvent(FKeyEvent(Key, App.GetModifierKeys(), 0, false, 0, 0));
	}

	void MoveMouse()
	{
		FSlateApplication& App = FSlateApplication::Get();
		App.ProcessMouseMoveEvent(FPointerEvent(0, 0, FVector2D(60.0, 60.0), FVector2D(40.0, 40.0), TSet<FKey>(), EKeys::Invalid, 0.f, App.GetModifierKeys()));
	}

	namespace
	{
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
	}

	void AddClick(FMvsScript& Script, TFunction<const UWidget*()> Target, const FString& Rule, TFunction<FVector2D(const FGeometry&)> Fraction)
	{
		FMvsScript* Self = &Script;
		TSharedRef<FVector2D> At = MakeShared<FVector2D>(FVector2D::ZeroVector);
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

	UMvsHintButton* FindHint(const UObject* Screen, const FKey& Key)
	{
		for (TObjectIterator<UMvsActionBar> Bar; Bar && Screen; ++Bar)
		{
			if (!Bar->HasAnyFlags(RF_ClassDefaultObject) && Bar->IsIn(Screen))
			{
				for (UMvsHintButton* Hint : Bar->GetTypedEntries<UMvsHintButton>())
				{
					if (Hint->IsVisible() && Hint->GetKeyboardKey() == Key)
					{
						return Hint;
					}
				}
			}
		}
		for (TObjectIterator<UMvsTabList> Tabs; Tabs && Screen; ++Tabs)
		{
			if (!Tabs->HasAnyFlags(RF_ClassDefaultObject) && Tabs->IsIn(Screen) && Tabs->WidgetTree)
			{
				UMvsHintButton* Found = nullptr;
				Tabs->WidgetTree->ForEachWidget([&Found, &Key](UWidget* Widget)
				{
					UMvsHintButton* Hint = Cast<UMvsHintButton>(Widget);
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

	FString GlyphText(const UMvsHintButton* Hint)
	{
		const UMvsInputGlyph* Glyph = nullptr;
		if (Hint && Hint->WidgetTree)
		{
			Hint->WidgetTree->ForEachWidget([&Glyph](UWidget* W) { Glyph = Glyph ? Glyph : Cast<UMvsInputGlyph>(W); });
		}
		const UTextBlock* Text = nullptr;
		if (Glyph && Glyph->WidgetTree)
		{
			Glyph->WidgetTree->ForEachWidget([&Text](UWidget* W) { Text = Text ? Text : Cast<UTextBlock>(W); });
		}
		return Text ? Text->GetText().ToString() : FString();
	}

	FLinearColor PromptLabelColor(const UObject* Screen)
	{
		const UMvsHintButton* Hint = FindHint(Screen, EKeys::Escape);
		UTextBlock* Label = nullptr;
		if (Hint && Hint->WidgetTree)
		{
			Hint->WidgetTree->ForEachWidget([&Label](UWidget* W) { if (!Label) { Label = Cast<UTextBlock>(W); } });
		}
		return Label ? Label->GetColorAndOpacity().GetSpecifiedColor() : FLinearColor::Transparent;
	}

	UMvsOptionRow* FindRow(const UObject* Screen, EMvsSetting Setting)
	{
		for (TObjectIterator<UMvsOptionRow> It; It; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->GetSetting() == Setting && It->IsIn(Screen))
			{
				return *It;
			}
		}
		return nullptr;
	}

	FName SelectedTab(const UObject* Screen)
	{
		const UMvsTabList* Tabs = FindIn<UMvsTabList>(Screen);
		return Tabs ? Tabs->GetSelectedTabId() : NAME_None;
	}

	FString FocusedText()
	{
		return MvsAccessibility::GetText(FSlateApplication::Get().GetUserFocusedWidget(0)).ToString();
	}

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

	namespace
	{
		UEnhancedInputUserSettings* UserSettingsOf(const TWeakObjectPtr<AMvsPlayerController>& PC)
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
	}

	void Rebind(const TWeakObjectPtr<AMvsPlayerController>& PC, FName Action, const FKey& Key, TArray<TPair<FName, FKey>>& Undo)
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

	void RestoreBindings(const TWeakObjectPtr<AMvsPlayerController>& PC, TArray<TPair<FName, FKey>>& Undo)
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

#endif
