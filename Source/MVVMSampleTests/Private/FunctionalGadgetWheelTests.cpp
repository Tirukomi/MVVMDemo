// Copyright IG. All Rights Reserved.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Accessibility/MvsSettingsTypes.h"
#include "Core/MvsPlayerController.h"
#include "Gameplay/MvsFeel.h"
#include "Kismet/GameplayStatics.h"
#include "UI/MvsAccessibility.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Style/MvsStyle.h"
#include "UI/Widgets/GadgetWheel.h"
#include "UI/Widgets/MvsInputGlyph.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/SettingsViewModel.h"

namespace MvsGadgetWheelTests
{
	using namespace MvsMenuTest;

	// Review findings 3, 4, 6 and 10: the gadget wheel and gadget keys follow rebinding, the wheel's slow motion survives
	// a hit-stop, and the wheel follows reduced motion live. Rebinds are in memory only (never saved) and undone.
	void GadgetWheel(FMvsScript& Script, const FRig& Rig)
	{
		TSharedPtr<TArray<TPair<FName, FKey>>> Undo = MakeShared<TArray<TPair<FName, FKey>>>();
		Script.Do([Rig, Undo]()
			{
				Rebind(Rig.PC, TEXT("GadgetWheel"), EKeys::Z, *Undo);
				Rebind(Rig.PC, TEXT("Gadget1"), EKeys::X, *Undo);
			})
			.Do([Rig]()
			{
				const UMvsViewModelSubsystem* ViewModels = Rig.ViewModels();
				const UGadgetSlotViewModel* First = ViewModels ? ViewModels->GetGadgetBar()->GetSlot(0) : nullptr;
				Rig.Check(First && First->GetHotkey().ToString() == UMvsInputGlyph::GetKeyLabel(EKeys::X).ToString(), TEXT("the HUD's gadget key hint follows rebinding (review 4)"));
			})
			.Do([Rig]()
			{
				MvsFeel::HitStop(Rig.PC.Get(), MvsFeel::HitStopSeconds);
				if (Rig.UI.IsValid()) { Rig.UI->OpenGadgetWheel(); }
			})
			.WaitUntil([Rig]() { return Rig.Settled(ActiveScreen<UGadgetWheelScreen>()); }, Open, TEXT("the gadget wheel opens (precondition)"))
			.Do([Rig]()
			{
				const UGadgetWheel* Wheel = FindIn<UGadgetWheel>(ActiveScreen<UGadgetWheelScreen>());
				const FString Spoken = MvsAccessibility::GetText(Wheel ? Wheel->GetCachedWidget() : nullptr).ToString();
				Rig.Check(Spoken.StartsWith(TEXT("Gadget wheel")), FString::Printf(TEXT("the gadget wheel tells screen readers what it would use (review 31) [%s]"), *Spoken));
				// Second review finding 2: the wheel's text follows the theme (palette and text size), like every other screen.
				const FMvsTheme Theme = MvsStyle::Theme(Wheel);
				Rig.Check(Wheel && Wheel->WheelStyle.LabelColor.Equals(Theme.Color(EMvsColorToken::TextPrimary))
					&& Wheel->WheelStyle.CoolingLabelColor.Equals(Theme.Color(EMvsColorToken::TextMuted))
					&& Wheel->WheelStyle.LabelFont.Size == Theme.Font(EMvsTextStyle::Header).Size,
					TEXT("the gadget wheel's labels use the theme's colours and text size (second review 2)"));
			})
			.Wait(MvsFeel::HitStopSeconds * 3.f)
			.Do([Rig]()
			{
				Rig.Check(Rig.PC.IsValid() && UGameplayStatics::GetGlobalTimeDilation(Rig.PC.Get()) < 0.5f, TEXT("the wheel's slow motion survives a hit-stop (review 6)"));
				if (USettingsViewModel* VM = Rig.Settings()) { VM->Cycle(EMvsSetting::ReducedMotion, +1); }
			})
			.Do([Rig]()
			{
				const UGadgetWheel* Wheel = FindIn<UGadgetWheel>(ActiveScreen<UGadgetWheelScreen>());
				Rig.Check(Wheel && Wheel->bReduceMotion, TEXT("the open wheel follows reduced motion live (review 10)"));
				if (USettingsViewModel* VM = Rig.Settings()) { VM->Revert(); }
				ReleaseKey(EKeys::Z);
			})
			.Wait(0.5f)
			.Do([Rig]() { Rig.Check(ActiveScreen<UGadgetWheelScreen>() == nullptr, TEXT("releasing the rebound wheel key closes the wheel (review 3)")); })
			.Do([Rig, Undo]()
			{
				if (UGadgetWheelScreen* Wheel = ActiveScreen<UGadgetWheelScreen>()) { Wheel->DeactivateWidget(); }
				RestoreBindings(Rig.PC, *Undo);
			})
			.WaitUntil([Rig]() { return Rig.Closed(ActiveScreen<UGadgetWheelScreen>()); }, Quick, TEXT("the wheel closes"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalGadgetWheelTest, "Mvs.Functional.GadgetWheel", MvsMenuTest::Flags)
bool FMvsFunctionalGadgetWheelTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("GadgetWheel"), &MvsGadgetWheelTests::GadgetWheel);
	return true;
}

#endif
