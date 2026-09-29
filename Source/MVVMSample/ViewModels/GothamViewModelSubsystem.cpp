// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/GothamViewModelSubsystem.h"

#include "Core/GothamCharacter.h"
#include "Gameplay/ComboComponent.h"
#include "Gameplay/GadgetComponent.h"
#include "Gameplay/HealthComponent.h"
#include "ViewModels/ComboViewModel.h"
#include "ViewModels/GadgetViewModels.h"
#include "ViewModels/PlayerVitalsViewModel.h"

void UGothamViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Vitals = NewObject<UPlayerVitalsViewModel>(this);
	GadgetBar = NewObject<UGadgetBarViewModel>(this);
	Combo = NewObject<UComboViewModel>(this);
}

void UGothamViewModelSubsystem::Deinitialize()
{
	Unbind();
	Super::Deinitialize();
}

UObject* UGothamViewModelSubsystem::FindViewModel(const UClass* ViewModelClass) const
{
	for (UObject* Candidate : { static_cast<UObject*>(Vitals), static_cast<UObject*>(GadgetBar), static_cast<UObject*>(Combo) })
	{
		if (Candidate && Candidate->GetClass()->IsChildOf(ViewModelClass))
		{
			return Candidate;
		}
	}
	return nullptr;
}

void UGothamViewModelSubsystem::Unbind()
{
	if (UHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.Remove(HealthHandle);
	}
	if (UGadgetComponent* Gadgets = BoundGadgets.Get())
	{
		Gadgets->OnCooldownChanged.Remove(GadgetHandle);
	}
	if (UComboComponent* ComboComp = BoundCombo.Get())
	{
		ComboComp->OnComboChanged.Remove(ComboHandle);
	}
	BoundHealth.Reset();
	BoundGadgets.Reset();
	BoundCombo.Reset();
}

void UGothamViewModelSubsystem::BindToCharacter(AGothamCharacter* Character)
{
	Unbind();
	if (!Character)
	{
		return;
	}

	BoundHealth = Character->GetHealthComponent();
	BoundGadgets = Character->GetGadgetComponent();
	BoundCombo = Character->GetComboComponent();

	HealthHandle = BoundHealth->OnHealthChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleHealth);
	GadgetHandle = BoundGadgets->OnCooldownChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleGadgetCooldown);
	ComboHandle = BoundCombo->OnComboChanged.AddUObject(this, &UGothamViewModelSubsystem::HandleCombo);

	// Static slot data (name, key hint, tint) is set once per bind; cooldowns stream in afterwards.
	const TArray<FGothamGadgetDefinition>& Defs = BoundGadgets->GetGadgets();
	GadgetBar->SetSlotCount(Defs.Num());
	for (int32 i = 0; i < Defs.Num(); ++i)
	{
		GadgetBar->GetSlot(i)->SetDefinition(Defs[i].DisplayName, FText::AsNumber(i + 1), Defs[i].Tint);
	}

	BoundHealth->BroadcastCurrent();
	BoundGadgets->BroadcastAll();
	BoundCombo->BroadcastCurrent();
}

void UGothamViewModelSubsystem::HandleHealth(float Health, float MaxHealth)
{
	Vitals->SetVitals(Health, MaxHealth);
}

void UGothamViewModelSubsystem::HandleGadgetCooldown(int32 Slot, float Remaining, float Total)
{
	if (UGadgetSlotViewModel* SlotVM = GadgetBar->GetSlot(Slot))
	{
		SlotVM->SetCooldown(Remaining, Total);
	}
}

void UGothamViewModelSubsystem::HandleCombo(int32 Hits, float Multiplier, float DecayAlpha)
{
	Combo->SetCombo(Hits, Multiplier, DecayAlpha);
}
