// Copyright IG. All Rights Reserved.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Accessibility/MvsSettingsSubsystem.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "Core/MvsPlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/UserInterfaceSettings.h"
#include "Framework/Application/SlateApplication.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "UI/MvsAccessibility.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/ConfirmModalScreen.h"
#include "UI/Screens/ControlsScreen.h"
#include "UI/Screens/SettingsScreen.h"
#include "UI/Style/MvsMetrics.h"
#include "UI/Widgets/MvsButton.h"
#include "UI/Widgets/MvsHintButton.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "UI/Widgets/MvsOptionRow.h"
#include "UI/Widgets/MvsTabList.h"
#include "UI/Widgets/MvsText.h"
#include "ViewModels/ControlsViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/SettingsViewModel.h"

namespace MvsSettingsTests
{
	using namespace MvsMenuTest;

	void Revert(const FRig& Rig)
	{
		if (USettingsViewModel* VM = Rig.Settings()) { VM->Revert(); }
	}

	void Settings(FMvsScript& Script, const FRig& Rig)
	{
		TSharedPtr<int32> ScaleBefore = MakeShared<int32>(0);
		TSharedPtr<float> LayoutScaleBefore = MakeShared<float>(0.f);
		TSharedPtr<FLinearColor> LabelBefore = MakeShared<FLinearColor>(FLinearColor::Transparent);
		TSharedPtr<bool> WasActive = MakeShared<bool>(true);
		TSharedPtr<FName> TabBefore = MakeShared<FName>();
		// The HUD's first gadget hint, as its view model holds it.
		auto GadgetHint = [Rig]()
		{
			const UMvsViewModelSubsystem* ViewModels = Rig.ViewModels();
			const UGadgetSlotViewModel* First = ViewModels && ViewModels->GetGadgetBar() ? ViewModels->GetGadgetBar()->GetSlot(0) : nullptr;
			return First ? First->GetHotkey().ToString() : FString();
		};

		// Keys on a focused row, then each prompt does what its key does.
		Script.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open (precondition)"))
			// The screen keeps its last tab between openings (another test may have left it elsewhere): start on the first
			// tab, whose first row (UI scale) takes focus. The page fades in, and a click during the fade misses.
			.Do([]()
			{
				if (UMvsTabList* Tabs = FindIn<UMvsTabList>(ActiveScreen<USettingsScreen>())) { Tabs->SelectTabByID(USettingsViewModel::GetTabs()[0].Id); }
			})
			.WaitUntil([Rig]()
				{
					const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
					const UMvsSwitcher* Pages = FindIn<UMvsSwitcher>(Screen);
					return Rig.Settled(Screen) && Pages && !Pages->IsTransitionPlaying() && HasFocusWithin(FindRow(Screen, EMvsSetting::UIScale));
				},
				Open, TEXT("settings show the first tab, UI scale focused (precondition)"))
			// Keys straight to the focused row first: separates "the key never reaches the row" from "the click fails".
			.Do([Rig, ScaleBefore, LayoutScaleBefore]()
			{
				*ScaleBefore = Rig.Scale();
				const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
				*LayoutScaleBefore = Screen ? Screen->GetCachedGeometry().Scale : 0.f;
				SendKey(EKeys::Right);
			})
			.WaitUntil([Rig, ScaleBefore]() { return Rig.Scale() != *ScaleBefore; }, Quick, TEXT("Right on a focused option row steps it"))
			// Review finding 21: the preview scales the layout itself; the engine's UI settings are never written.
			.WaitUntil([LayoutScaleBefore]()
			{
				const USettingsScreen* Screen = ActiveScreen<USettingsScreen>();
				return Screen && !FMath::IsNearlyEqual(Screen->GetCachedGeometry().Scale, *LayoutScaleBefore)
					&& GetDefault<UUserInterfaceSettings>()->ApplicationScale == 1.f;
			}, Quick, TEXT("a UI scale preview scales the layout, not the engine's UI settings (review 21)"))
			.Do([Rig, ScaleBefore]() { *ScaleBefore = Rig.Scale(); SendKey(EKeys::Enter); })
			.WaitUntil([Rig, ScaleBefore]() { return Rig.Scale() != *ScaleBefore; }, Quick, TEXT("Enter on a focused option row steps it"))
			// Changing the UI scale re-lays out everything; clicks wait for the new geometry.
			.Do([Rig]() { Revert(Rig); })
			.WaitUntil(Stable([]() { return FindRow(ActiveScreen<USettingsScreen>(), EMvsSetting::UIScale); }), Quick, TEXT("the UI scale row settles (precondition)"))
			.Do([Rig, ScaleBefore]() { *ScaleBefore = Rig.Scale(); });
		// The selector's left half steps back. The selector (MvsMetrics::SelectorWidth, inset SelectorInset) sits at the
		// row's right edge; click a quarter of the way into it.
		AddClick(Script, []() { return FindRow(ActiveScreen<USettingsScreen>(), EMvsSetting::UIScale); }, TEXT("the UI scale row is clickable"),
			[](const FGeometry& Geometry)
			{
				const float Width = Geometry.GetLocalSize().X;
				const float SelectorLeft = Width - MvsMetrics::SelectorInset - MvsMetrics::SelectorWidth;
				return FVector2D(Width > 0.f ? (SelectorLeft + MvsMetrics::SelectorWidth * 0.25f) / Width : 0.5f, 0.5);
			});
		Script.WaitUntil([Rig, ScaleBefore]() { return Rig.Scale() == *ScaleBefore - 1; }, Quick, TEXT("clicking the left half of a selector steps it back"))
			.Do([Rig]() { Revert(Rig); })
			.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter); }), Quick, TEXT("the [Enter] prompt settles (precondition)"))
			.Do([Rig, ScaleBefore]() { *ScaleBefore = Rig.Scale(); });
		AddClick(Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter); }, TEXT("the [Enter] prompt is clickable"));
		Script.WaitUntil([Rig, ScaleBefore]() { return Rig.Scale() != *ScaleBefore; }, Quick, TEXT("clicking [Enter] Change steps the focused option (UI scale)"))
			.Do([Rig]() { Revert(Rig); })
			.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::E); }), Quick, TEXT("the [E] prompt settles (precondition)"))
			// Second review 18: clicks work while another application is active, so the tests no longer need the PC left
			// alone. Slate is told the application lost activation (what the OS reports when the user switches away).
			.Do([WasActive, TabBefore]()
			{
				*TabBefore = SelectedTab(ActiveScreen<USettingsScreen>());
				*WasActive = FSlateApplication::Get().IsActive();
				FSlateApplication::Get().ProcessApplicationActivationEvent(false);
			});
		AddClick(Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::E); }, TEXT("the [E] tab prompt is clickable"));
		// The screen keeps its last tab between openings, so the rule is "another tab", not a particular one.
		Script.WaitUntil([Rig, TabBefore]()
				{
					const FName Now = SelectedTab(ActiveScreen<USettingsScreen>());
					return Rig.UI.IsValid() && !Rig.UI->IsTransitioning() && Now != NAME_None && Now != *TabBefore;
				},
				Quick, TEXT("clicking [E] switches to the next tab, with another application active (second review 18)"))
			.Do([WasActive]() { FSlateApplication::Get().ProcessApplicationActivationEvent(*WasActive); })
			// Live restyle: every widget subscribes to settings changes through FMvsSettingsListener. Turning high contrast
			// on must recolour an open screen's prompts right away (a missed subscription would leave them).
			.Do([Rig, LabelBefore]()
			{
				*LabelBefore = PromptLabelColor(ActiveScreen<USettingsScreen>());
				if (USettingsViewModel* VM = Rig.Settings()) { VM->Cycle(EMvsSetting::HighContrast, +1); }
			})
			.WaitUntil([LabelBefore]() { const FLinearColor Now = PromptLabelColor(ActiveScreen<USettingsScreen>()); return LabelBefore->A > 0.f && !Now.Equals(*LabelBefore, 0.01f); },
				Quick, TEXT("turning high contrast on restyles an open screen live"))
			.Do([Rig]() { Revert(Rig); })
			// Prompts are the screen's bound actions, with the keys of the device in use (review 12, 15).
			.Do([]() { SendKey(EKeys::Gamepad_DPad_Down); })
			.WaitUntil([]() { return GlyphText(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter)) == UMvsInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Bottom).ToString(); },
				Quick, TEXT("a gamepad press turns the accept prompt into the gamepad's accept button"))
			// Second review finding 1: the HUD's gadget hints follow the device too (Gadget 1 is X on a pad by default).
			.WaitUntil([GadgetHint, Rig]() { return GadgetHint() == UMvsInputGlyph::GetKeyLabel(EKeys::Gamepad_FaceButton_Left, Rig.PC.IsValid() ? Rig.PC->GetLocalPlayer() : nullptr).ToString(); },
				Quick, TEXT("a gamepad press turns the HUD's gadget hints into gamepad buttons (second review 1)"))
			.Do([]() { SendKey(EKeys::Gamepad_DPad_Up); })
			.Do([]() { MoveMouse(); })
			.WaitUntil([]() { return GlyphText(FindHint(ActiveScreen<USettingsScreen>(), EKeys::Enter)) == UMvsInputGlyph::GetKeyLabel(EKeys::Enter).ToString(); },
				Quick, TEXT("moving the mouse turns it back into Enter"))
			.WaitUntil([GadgetHint]() { return GadgetHint() == UMvsInputGlyph::GetKeyLabel(EKeys::One).ToString(); },
				Quick, TEXT("and the gadget hints back into keys (second review 1)"))
			.WaitUntil(Stable([]() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape); }), Quick, TEXT("the [Esc] prompt settles (precondition)"));
		AddClick(Script, []() { return FindHint(ActiveScreen<USettingsScreen>(), EKeys::Escape); }, TEXT("the [Esc] prompt is clickable"));
		Script.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<USettingsScreen>()); }, Quick, TEXT("clicking [Esc] Back closes settings"));

		// Review finding 21: a language preview is live, and leaving settings without applying puts the culture back.
		TSharedPtr<FString> CultureBefore = MakeShared<FString>();
		Script.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open for the language preview (precondition)"))
			.Do([Rig, CultureBefore]()
			{
				*CultureBefore = FInternationalization::Get().GetCurrentCulture()->GetName();
				if (USettingsViewModel* VM = Rig.Settings()) { VM->Cycle(EMvsSetting::Language, +1); }
			})
			.WaitUntil([CultureBefore]() { return FInternationalization::Get().GetCurrentCulture()->GetName() != *CultureBefore; }, Quick,
				TEXT("a language preview switches the language live"))
			.Do([]() { if (USettingsScreen* Screen = ActiveScreen<USettingsScreen>()) { Screen->DeactivateWidget(); } })
			.WaitUntil([Rig, CultureBefore]() { return Rig.Closed(ActiveScreen<USettingsScreen>()) && FInternationalization::Get().GetCurrentCulture()->GetName() == *CultureBefore; },
				Open, TEXT("closing settings without applying restores the language (review 21)"));

		// Review finding 31: Text size enlarges menu text on its own (UI scale stays), and a focused settings row tells a
		// screen reader its label and value.
		TSharedPtr<float> FontBefore = MakeShared<float>(0.f);
		auto TitleSize = []() { const UMvsText* Text = FindIn<UMvsText>(ActiveScreen<USettingsScreen>()); return Text ? Text->GetFont().Size : 0.f; };
		Script.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open for text size (precondition)"))
			.Do([Rig]()
			{
				const FString Spoken = FocusedText();
				// Whichever row has focus (the screen keeps its last tab): "<label>: <value>".
				bool bRow = false;
				for (int32 i = 0; i < static_cast<int32>(EMvsSetting::Count); ++i)
				{
					bRow |= Spoken.StartsWith(USettingsViewModel::GetLabel(static_cast<EMvsSetting>(i)).ToString() + TEXT(": "));
				}
				Rig.Check(bRow && Spoken.Len() > 4,
					FString::Printf(TEXT("a focused settings row tells screen readers its label and value (review 31) [%s]"), *Spoken));
			})
			.Do([Rig, FontBefore, ScaleBefore, TitleSize]()
			{
				*FontBefore = TitleSize();
				*ScaleBefore = Rig.Scale();
				if (USettingsViewModel* VM = Rig.Settings()) { VM->Cycle(EMvsSetting::TextSize, +1); }
			})
			.WaitUntil([Rig, FontBefore, ScaleBefore, TitleSize]() { return *FontBefore > 0.f && TitleSize() > *FontBefore * 1.1f && Rig.Scale() == *ScaleBefore; },
				Quick, TEXT("Text size enlarges menu text and leaves UI scale alone (review 31)"))
			.Do([Rig]() { Revert(Rig); })
			.WaitUntil([FontBefore, TitleSize]() { return FMath::IsNearlyEqual(TitleSize(), *FontBefore); }, Quick, TEXT("reverting puts the text size back"));
	}

	void Controls(FMvsScript& Script, const FRig& Rig)
	{
		// Review finding 1: opening Key bindings must keep unapplied settings.
		Script.Do(Rig.Push(&UMvsUISettings::SettingsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open, TEXT("settings open (precondition)"))
			.Do([Rig]() { if (USettingsViewModel* VM = Rig.Settings()) { VM->Cycle(EMvsSetting::SubtitleSize, +1); } })
			.Do(Rig.Push(&UMvsUISettings::ControlsScreenClass))
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UControlsScreen>()); }, Open, TEXT("key bindings open over settings (precondition)"))
			.Do([Rig]() { Rig.Check(Rig.Model() && Rig.Model()->HasUnsavedChanges(), TEXT("opening Key bindings keeps unapplied settings (review 1)")); });

		// Review finding 39: "Reset to defaults" asks first; No keeps the bindings, Yes resets them. (Confirming resets this
		// machine's saved bindings for the sample.)
		const FString WasReset = NSLOCTEXT("Mvs.ControlsScreen", "WasReset", "Controls reset to defaults.").ToString();
		auto ResetShown = [WasReset]()
		{
			const UControlsScreen* Controls = ActiveScreen<UControlsScreen>();
			return Controls && Controls->GetViewModel() && Controls->GetViewModel()->GetStatusText().ToString() == WasReset;
		};
		Script.Do([]() { if (UControlsScreen* Controls = ActiveScreen<UControlsScreen>()) { Controls->RequestResetAll(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UConfirmModalScreen>()); }, Open, TEXT("Reset to defaults asks for confirmation (review 39)"))
			.Do([Rig, ResetShown]() { Rig.Check(!ResetShown(), TEXT("asking does not reset the controls yet (review 39)")); })
			.Do([]() { SendKey(EKeys::Escape); })
			.WaitUntil([Rig]() { return !ActiveScreen<UConfirmModalScreen>() && Rig.Settled(ActiveScreen<UControlsScreen>()); }, Open,
				TEXT("Esc answers the reset confirmation (precondition)"))
			.Do([Rig, ResetShown]() { Rig.Check(!ResetShown(), TEXT("answering No keeps the controls (review 39)")); })
			.Do([]() { if (UControlsScreen* Controls = ActiveScreen<UControlsScreen>()) { Controls->RequestResetAll(); } })
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UConfirmModalScreen>()); }, Open, TEXT("the reset confirmation opens again (precondition)"))
			.WaitUntil([]() { return FocusedText() == NSLOCTEXT("Mvs.ConfirmModal", "No", "No").ToString(); }, Quick,
				TEXT("the reset confirmation focuses No (precondition)"))
			// Found with this rule: the arrows moved focus out of the confirmation, onto the key-binding slots behind it.
			.Do([]() { SendKey(EKeys::Left); SendKey(EKeys::Up); SendKey(EKeys::Down); SendKey(EKeys::Right); })
			.Do([Rig]() { Rig.Check(HasFocusWithin(ActiveScreen<UConfirmModalScreen>()), TEXT("the arrows never move focus out of a confirmation (review 39)")); });
		auto FindYes = []() -> UMvsButton*
		{
			const FString Yes = NSLOCTEXT("Mvs.ConfirmModal", "Yes", "Yes").ToString();
			const UConfirmModalScreen* Modal = ActiveScreen<UConfirmModalScreen>();
			for (TObjectIterator<UMvsButton> Button; Button && Modal; ++Button)
			{
				if (Button->IsIn(Modal) && MvsAccessibility::GetText(MvsAccessibility::FindButton(**Button)).ToString() == Yes)
				{
					return *Button;
				}
			}
			return nullptr;
		};
		Script.WaitUntil(Stable(FindYes), Quick, TEXT("the reset confirmation's Yes settles (precondition)"));
		AddClick(Script, FindYes, TEXT("the reset confirmation's Yes is clickable"));
		Script.WaitUntil([Rig]() { return !ActiveScreen<UConfirmModalScreen>() && Rig.Settled(ActiveScreen<UControlsScreen>()); }, Open,
				TEXT("Yes answers the reset confirmation (precondition)"))
			// Found with this rule: the stack reuses a closed confirmation, which kept its first answer and ignored the second.
			.WaitUntil(ResetShown, Quick, TEXT("answering Yes resets the controls, also on a reused confirmation (review 39)"))
			.Do([]() { SendKey(EKeys::Escape); })
			.WaitUntil([Rig]() { return !ActiveScreen<UControlsScreen>() && Rig.Settled(ActiveScreen<USettingsScreen>()); }, Open,
				TEXT("Esc closes key bindings and only key bindings (review 11)"));
	}
}

// Settings: keys and clicks on rows and prompts, live previews, and prompts that follow the device.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalSettingsTest, "Mvs.Functional.Settings", MvsMenuTest::Flags)
bool FMvsFunctionalSettingsTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("Settings"), &MvsSettingsTests::Settings);
	return true;
}

// Key bindings over settings, and its reset confirmation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalControlsTest, "Mvs.Functional.Controls", MvsMenuTest::Flags)
bool FMvsFunctionalControlsTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("Controls"), &MvsSettingsTests::Controls);
	return true;
}

#endif
