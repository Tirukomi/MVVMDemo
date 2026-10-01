// Copyright IG. All Rights Reserved.

// Functional tests: they need the running game (a world, a player, painted widgets), so they carry only the client
// context and run in Scripts/run_tests.py's game pass, never in the editor pass.

#include "MvsMenuTestKit.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "HAL/IConsoleManager.h"
#include "UI/MvsHudWidget.h"
#include "UI/MvsUISettings.h"
#include "UI/Layout/MvsUISubsystem.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/ForensicViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/MvsViewModelResolver.h"
#include "ViewModels/MvsViewModelSubsystem.h"
#include "ViewModels/ObjectivesViewModel.h"
#include "ViewModels/PlayerVitalsViewModel.h"
#include "ViewModels/SettingsViewModel.h"
#include "ViewModels/SubtitleViewModel.h"
#include "ViewModels/ThreatViewModel.h"

namespace MvsHudTests
{
	using namespace MvsMenuTest;

	void Hud(FMvsScript& Script, const FRig& Rig)
	{
		// Review finding 30: the HUD's edge-anchored elements follow the platform safe zone. A PC has none, so the
		// engine's debug ratio stands in for a TV's: at 90% the safe area starts 5% in from the left edge.
		TSharedPtr<FVector2D> SafeOrigin = MakeShared<FVector2D>(FVector2D::ZeroVector);
		auto SafeArea = []() { const UMvsHudWidget* Hud = ActiveScreen<UMvsHudWidget>(); return Hud ? Hud->GetSafeArea() : nullptr; };
		auto SetSafeRatio = [](float Ratio)
		{
			if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(TEXT("r.DebugSafeZone.TitleRatio")))
			{
				Var->Set(Ratio, ECVF_SetByCode);
			}
		};
		Script.Do([SafeArea, SafeOrigin, SetSafeRatio]()
			{
				const UWidget* Area = SafeArea();
				*SafeOrigin = Area ? Area->GetCachedGeometry().GetAbsolutePosition() : FVector2D::ZeroVector;
				SetSafeRatio(0.9f);
			})
			.WaitUntil([SafeArea, SafeOrigin]()
			{
				const UWidget* Area = SafeArea();
				return Area && Area->GetCachedGeometry().GetAbsolutePosition().X > SafeOrigin->X + 10.0;
			}, Quick, TEXT("the HUD moves inside a 90% safe zone (review 30)"))
			.Do([SetSafeRatio]() { SetSafeRatio(1.f); })
			.WaitUntil([SafeArea, SafeOrigin]()
			{
				const UWidget* Area = SafeArea();
				return Area && FMath::IsNearlyEqual(Area->GetCachedGeometry().GetAbsolutePosition().X, SafeOrigin->X, 1.0);
			}, Quick, TEXT("and back to the screen edge without one"));

		// Review finding 23: screens load when the layout is created, before any key opens one.
		Script.Do([Rig]()
		{
			Rig.Check(Rig.UI.IsValid() && Rig.UI->AreScreensLoaded() && GetDefault<UMvsUISettings>()->ClueEntryClass.Get() != nullptr,
				TEXT("the screen classes are loaded before the first key press (review 23)"));
		});

		// Review finding 18: views use gadgets through the gadget bar's command, which the gadget binder hands to gameplay.
		Script.Do([Rig]()
			{
				UGadgetBarViewModel* Bar = Rig.ViewModels() ? Rig.ViewModels()->GetGadgetBar() : nullptr;
				Rig.Check(Bar && Bar->GetSlot(0) && Bar->GetSlot(0)->GetIsReady(), TEXT("the first gadget is ready (precondition)"));
				if (Bar) { Bar->RequestUse(0); }
			})
			.WaitUntil([Rig]()
			{
				const UGadgetBarViewModel* Bar = Rig.ViewModels() ? Rig.ViewModels()->GetGadgetBar() : nullptr;
				return Bar && Bar->GetSlot(0) && !Bar->GetSlot(0)->GetIsReady();
			}, Quick, TEXT("the gadget bar's use command reaches gameplay (review 18)"));
	}

	// Second review 21: the resolver a designer picks for a widget's view model context hands it the owning player's
	// view model of that class, and nothing for a class the player has none of.
	void Resolver(FMvsScript& Script, const FRig& Rig)
	{
		Script.Do([Rig]()
		{
			const UMvsViewModelSubsystem* ViewModels = Rig.ViewModels();
			const UMvsHudWidget* Hud = ActiveScreen<UMvsHudWidget>();
			Rig.Check(ViewModels && Hud && Hud->GetOwningLocalPlayer(), TEXT("the HUD belongs to the player (precondition)"));
			if (!ViewModels || !Hud)
			{
				return;
			}
			const UMvsViewModelResolver* Resolver = GetDefault<UMvsViewModelResolver>();
			const TPair<const UClass*, const UObject*> Expected[] = {
				{ UPlayerVitalsViewModel::StaticClass(), ViewModels->GetVitals() },
				{ UGadgetBarViewModel::StaticClass(), ViewModels->GetGadgetBar() },
				{ UComboViewModel::StaticClass(), ViewModels->GetCombo() },
				{ UForensicViewModel::StaticClass(), ViewModels->GetForensic() },
				{ UObjectivesViewModel::StaticClass(), ViewModels->GetObjectives() },
				{ UClueListViewModel::StaticClass(), ViewModels->GetClues() },
				{ USubtitleViewModel::StaticClass(), ViewModels->GetSubtitles() },
				{ UThreatViewModel::StaticClass(), ViewModels->GetThreats() },
			};
			for (const TPair<const UClass*, const UObject*>& Entry : Expected)
			{
				const UObject* Resolved = Resolver->CreateInstance(Entry.Key, Hud, nullptr);
				Rig.Check(Entry.Value && Resolved == Entry.Value,
					FString::Printf(TEXT("the resolver hands a widget the player's %s (second review 21)"), *Entry.Key->GetName()));
			}
			// The settings view model belongs to the settings subsystem, not to the player's HUD view models.
			Rig.Check(Resolver->CreateInstance(USettingsViewModel::StaticClass(), Hud, nullptr) == nullptr,
				TEXT("the resolver hands nothing for a class the player has no view model of (second review 21)"));
			Rig.Check(Resolver->CreateInstance(USubtitleViewModel::StaticClass(), nullptr, nullptr) == nullptr,
				TEXT("the resolver hands nothing without a widget (second review 21)"));
		});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalHudTest, "Mvs.Functional.Hud", MvsMenuTest::Flags)
bool FMvsFunctionalHudTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("Hud"), &MvsHudTests::Hud);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFunctionalResolverTest, "Mvs.Functional.ViewModelResolver", MvsMenuTest::Flags)
bool FMvsFunctionalResolverTest::RunTest(const FString& Parameters)
{
	MvsMenuTest::Run(this, TEXT("ViewModelResolver"), &MvsHudTests::Resolver);
	return true;
}

#endif
