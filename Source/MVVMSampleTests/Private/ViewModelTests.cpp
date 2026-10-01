// Copyright IG. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Gameplay/ComboComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/PlayerVitalsViewModel.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace MvsTests
{
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::ClientContext | EAutomationTestFlags::ProductFilter;
}

// Damage on the health component reaches the view model as the right percent and low-health flag,
// with no world and no widget involved.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsVitalsFlowTest, "Mvs.ViewModels.Vitals.DamageFlow", MvsTests::Flags)
bool FMvsVitalsFlowTest::RunTest(const FString& Parameters)
{
	UHealthComponent* Health = NewObject<UHealthComponent>(GetTransientPackage());
	UPlayerVitalsViewModel* VM = NewObject<UPlayerVitalsViewModel>(GetTransientPackage());
	Health->OnHealthChanged.AddLambda([VM](float H, float M) { VM->SetVitals(H, M); });

	Health->ApplyDamage(50.f);
	TestEqual("50 damage leaves half health", VM->GetHealthPercent(), 0.5f);
	TestFalse("half health is not low", VM->GetIsLowHealth());

	Health->ApplyDamage(30.f);
	TestEqual("percent after 80 damage", VM->GetHealthPercent(), 0.2f);
	TestTrue("20% is low health", VM->GetIsLowHealth());

	Health->ApplyDamage(500.f);
	TestEqual("health clamps at zero", VM->GetHealthPercent(), 0.f);

	Health->Heal(1000.f);
	TestEqual("health clamps at max", VM->GetHealthPercent(), 1.f);
	TestFalse("full health clears the low flag", VM->GetIsLowHealth());
	return true;
}

// Field-notify only fires when a value really changes, so bindings don't update needlessly.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsFieldNotifyTest, "Mvs.ViewModels.Vitals.FieldNotify", MvsTests::Flags)
bool FMvsFieldNotifyTest::RunTest(const FString& Parameters)
{
	UPlayerVitalsViewModel* VM = NewObject<UPlayerVitalsViewModel>(GetTransientPackage());
	int32 PercentNotifies = 0;
	VM->AddFieldValueChangedDelegate(UPlayerVitalsViewModel::FFieldNotificationClassDescriptor::HealthPercent,
		INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda([&PercentNotifies](UObject*, UE::FieldNotification::FFieldId) { ++PercentNotifies; }));

	VM->SetVitals(50.f, 100.f);
	TestEqual("first change notifies", PercentNotifies, 1);

	VM->SetVitals(50.f, 100.f);
	TestEqual("identical value does not notify", PercentNotifies, 1);

	VM->SetVitals(25.f, 100.f);
	TestEqual("new value notifies again", PercentNotifies, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsGadgetFlowTest, "Mvs.ViewModels.Gadgets.CooldownFlow", MvsTests::Flags)
bool FMvsGadgetFlowTest::RunTest(const FString& Parameters)
{
	UGadgetComponent* Gadgets = NewObject<UGadgetComponent>(GetTransientPackage());
	FMvsGadgetDefinition Def;
	Def.CooldownSeconds = 4.f;
	Gadgets->SetGadgets({ Def });

	UGadgetSlotViewModel* Slot = NewObject<UGadgetSlotViewModel>(GetTransientPackage());
	Gadgets->OnCooldownChanged.AddLambda([Slot](int32, float Remaining, float Total) { Slot->SetCooldown(Remaining, Total); });

	TestTrue("gadget starts ready", Gadgets->UseGadget(0));
	TestFalse("cannot reuse while cooling down", Gadgets->UseGadget(0));
	TestFalse("slot shows not ready", Slot->GetIsReady());
	TestEqual("cooldown starts full", Slot->GetCooldownPercent(), 1.f);

	Gadgets->Advance(2.f);
	TestEqual("half the cooldown elapsed", Slot->GetCooldownPercent(), 0.5f);

	Gadgets->Advance(2.f);
	TestTrue("slot ready again", Slot->GetIsReady());
	TestFalse("invalid slot index is rejected", Gadgets->UseGadget(7));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMvsComboFlowTest, "Mvs.ViewModels.Combo.DecayFlow", MvsTests::Flags)
bool FMvsComboFlowTest::RunTest(const FString& Parameters)
{
	UComboComponent* Combo = NewObject<UComboComponent>(GetTransientPackage());
	UComboViewModel* VM = NewObject<UComboViewModel>(GetTransientPackage());
	Combo->OnComboChanged.AddLambda([VM](int32 Hits, float Mult, float Alpha) { VM->SetCombo(Hits, Mult, Alpha); });

	TestFalse("no combo initially", VM->GetIsActive());
	for (int32 i = 0; i < 5; ++i)
	{
		Combo->RegisterHit();
	}
	TestTrue("combo active after hits", VM->GetIsActive());
	TestEqual("five hits recorded", VM->GetHitCount(), 5);
	TestEqual("multiplier steps up every five hits", VM->GetMultiplier(), 2.f);
	TestEqual("multiplier text is formatted by the view model", VM->GetMultiplierText().ToString(), FString(TEXT("x2")));

	Combo->Advance(10.f);
	TestFalse("combo drops after the decay window", VM->GetIsActive());
	TestEqual("hits reset", VM->GetHitCount(), 0);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
