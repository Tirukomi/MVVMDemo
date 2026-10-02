// Copyright IG. All Rights Reserved.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/MvsPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/Widgets/MvsTabList.h"
#include "UI/Widgets/MvsText.h"
#include "Components/TextBlock.h"
#include "MvsTestDesignerPause.h"
#include "Components/VerticalBox.h"
#include "UI/Widgets/MvsPanel.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/SettingsViewModel.h"

namespace MvsPauseTests
{
	using namespace MvsMenuTest;

	/** Whether a visible text inside Screen reads Text (as set: before a UMvsText's capitals). */
	bool ShowsText(const UObject* Screen, const FString& Text)
	{
		for (TObjectIterator<UTextBlock> It; It && Screen; ++It)
		{
			if (!It->HasAnyFlags(RF_ClassDefaultObject) && It->IsIn(Screen) && It->IsVisible())
			{
				const UMvsText* Mvs = Cast<UMvsText>(*It);
				if ((Mvs ? Mvs->GetSourceText() : It->GetText()).ToString() == Text)
				{
					return true;
				}
			}
		}
		return false;
	}

	void Pause(FMvsScript& Script, const FRig& Rig)
	{
		// The pause key on the gamepad (Start) closes pause.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->HandleShortcut(TEXT("Pause")); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens (precondition)"))
			// Second review 11: whichever pause class is configured (code-built, or a designer's WBP_PauseMenu filled by
			// MVVM View Bindings), the status panel reads the view models.
			.Do([Rig]()
			{
				const UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>();
				const UObjectivesViewModel* Objectives = Rig.ViewModels() ? Rig.ViewModels()->GetObjectives() : nullptr;
				const UClueListViewModel* Clues = Rig.ViewModels() ? Rig.ViewModels()->GetClues() : nullptr;
				Rig.Check(Objectives && ShowsText(Pause, Objectives->GetObjectiveTitle().ToString()),
					FString::Printf(TEXT("pause shows the current objective (%s, second review 11)"), *GetNameSafe(Pause ? Pause->GetClass() : nullptr)));
				Rig.Check(Clues && ShowsText(Pause, Clues->GetProgressText().ToString()),
					FString::Printf(TEXT("pause shows the evidence found (%s, second review 11)"), *GetNameSafe(Pause ? Pause->GetClass() : nullptr)));
			})
			.Do([]() { SendKey(EKeys::Gamepad_Special_Right); })
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UPauseMenuScreen>()); }, Quick, TEXT("Start closes pause"));

		// Review findings 2 and 7: screens opened from pause keep the game paused, and the case file's key opens the case
		// file over pause instead of closing pause.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->HandleShortcut(TEXT("Pause")); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens again (precondition)"))
			.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return ActiveScreen<USettingsScreen>() && Rig.UI.IsValid() && !Rig.UI->IsTransitioning(); }, Open, TEXT("settings open over pause (precondition)"))
			// Seen on some runs: focus lands on the game viewport, so a gamepad has nothing to move. The pause screen
			// deactivating underneath (finding 2) is the likely cause.
			.Do([Rig]() { Rig.Check(HasFocusWithin(ActiveScreen<USettingsScreen>()), TEXT("settings opened from pause receive focus (review 2)")); })
			.Do([Rig]() { Rig.Check(Rig.PC.IsValid() && UGameplayStatics::IsGamePaused(Rig.PC.Get()), TEXT("the game stays paused under settings opened from pause (review 2)")); })
			.Do([]() { if (USettingsScreen* Screen = ActiveScreen<USettingsScreen>()) { Screen->DeactivateWidget(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("back on pause (precondition)"));

		// Second review 30: closing settings back onto pause destructs its tab list, and Common UI drops a tab list's tabs
		// then; the reused screen came back without any, so Q / E and the tab prompts did nothing. Found by splitting
		// these tests: the [E] click used to run only on the first opening.
		TSharedPtr<FName> TabBefore = MakeShared<FName>();
		Script.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open from pause a second time (precondition)"))
			.Do([Rig]()
			{
				const UMvsTabList* Tabs = FindIn<UMvsTabList>(ActiveScreen<USettingsScreen>());
				Rig.Check(Tabs && Tabs->GetTabCount() == USettingsViewModel::GetTabs().Num() && !Tabs->GetSelectedTabId().IsNone(),
					TEXT("settings reopened from pause still have their tabs, one selected (second review 30)"));
			})
			.Do([TabBefore]() { *TabBefore = SelectedTab(ActiveScreen<USettingsScreen>()); SendKey(EKeys::E); })
			.WaitUntil([TabBefore]() { const FName Now = SelectedTab(ActiveScreen<USettingsScreen>()); return Now != NAME_None && Now != *TabBefore; },
				Quick, TEXT("E switches tabs on settings reopened from pause (second review 30)"))
			.Do([]() { if (USettingsScreen* Screen = ActiveScreen<USettingsScreen>()) { Screen->DeactivateWidget(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("back on pause again (precondition)"))
			.Do([]() { SendKey(EKeys::J); })
			.WaitUntil([Rig]() { return Rig.UI.IsValid() && !Rig.UI->IsTransitioning(); }, Quick, TEXT("the case file key is handled (precondition)"))
			.Do([Rig]() { Rig.Check(ActiveScreen<UClueLogScreen>() != nullptr, TEXT("the case file key opens the case file over pause (review 7)")); })
			.WaitUntil([Rig]() { return Rig.UI.IsValid() && !Rig.UI->IsTransitioning() && !Rig.UI->PopTopScreen(); }, Open, TEXT("menus close (precondition)"));

		// Review finding 26: one background blur on screen. The quit confirmation over pause blurs; pause stops blurring
		// under it and blurs again once it closes.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->HandleShortcut(TEXT("Pause")); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open, TEXT("pause opens for the quit confirmation (precondition)"))
			.Do([]() { if (UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>()) { Pause->RequestQuit(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UConfirmModalScreen>()); }, Open, TEXT("the quit confirmation opens (precondition)"))
			.Do([Rig]()
			{
				const UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>();
				const UConfirmModalScreen* Modal = ActiveScreen<UConfirmModalScreen>();
				Rig.Check(Pause && Modal && Modal->IsBackdropBlurEnabled() && !Pause->IsBackdropBlurEnabled(),
					TEXT("under the quit confirmation only the confirmation blurs (review 26)"));
			})
			.Do([]() { SendKey(EKeys::Escape); })
			.WaitUntil([Rig]() { return !ActiveScreen<UConfirmModalScreen>() && Rig.Settled(ActiveScreen<UPauseMenuScreen>()); }, Open,
				TEXT("Esc answers the confirmation (precondition)"))
			.Do([Rig]()
			{
				const UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>();
				Rig.Check(Pause && Pause->IsBackdropBlurEnabled(), TEXT("pause blurs again once the confirmation closes (review 26)"));
			})
			.Do([]() { if (UPauseMenuScreen* Pause = ActiveScreen<UPauseMenuScreen>()) { Pause->DeactivateWidget(); } })
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UPauseMenuScreen>()); }, Quick, TEXT("pause closes"))
			.Do([Rig]() { Rig.Check(Rig.PC.IsValid() && !UGameplayStatics::IsGamePaused(Rig.PC.Get()), TEXT("closing pause resumes the game")); });

		// Second review 11, the C++ half of the designer path: a pause class whose widget tree already has a root (as a
		// Widget Blueprint's does) gets that tree as its status panel's content, inside the code-built frame and menu,
		// and a designer's UMvsText takes its style from the Mvs settings in the Details panel.
		Script.Do([Rig]() { if (Rig.UI.IsValid()) { Rig.UI->PushScreen(EMvsUILayer::Menu, TSubclassOf<UCommonActivatableWidget>(UMvsTestDesignerPause::StaticClass())); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UMvsTestDesignerPause>()); }, Open, TEXT("a pause class with a designer's tree opens, focused (precondition)"))
			.Do([Rig]()
			{
				const UMvsTestDesignerPause* Pause = ActiveScreen<UMvsTestDesignerPause>();
				const UWidget* Panel = Pause ? Pause->GetStatusPanel() : nullptr;
				Rig.Check(Panel && Pause->DesignerRoot && Pause->DesignerRoot->GetParent() == Panel,
					TEXT("a designer's tree becomes the pause status panel's content (second review 11)"));
				Rig.Check(Pause && ShowsText(Pause, NSLOCTEXT("Mvs.PauseMenu", "Resume", "Resume").ToString()),
					TEXT("and the code-built menu is still there (second review 11)"));
				const FMvsTheme Theme = MvsStyle::Theme(Pause);
				const UMvsText* Label = Pause ? Pause->DesignerLabel.Get() : nullptr;
				Rig.Check(Label && Label->GetFont().Size == Theme.Font(EMvsTextStyle::Label).Size && Label->IsUpperCase()
					&& Label->GetColorAndOpacity().GetSpecifiedColor().Equals(Theme.Color(EMvsColorToken::TextMuted)),
					TEXT("a designer's text takes the style, capitals and colour set under Mvs (second review 11)"));
			})
			.Do([]() { if (UMvsTestDesignerPause* Pause = ActiveScreen<UMvsTestDesignerPause>()) { Pause->DeactivateWidget(); } })
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UMvsTestDesignerPause>()); }, Quick, TEXT("it closes (cleanup)"));
	}
}

// Pause's keys, the screens opened from it, and its confirmation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalPauseTest, "Mvs.Functional.Pause", MvsMenuTest::Flags)
bool FMvsFunctionalPauseTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("Pause"), &MvsPauseTests::Pause);
	return true;
}

#endif
