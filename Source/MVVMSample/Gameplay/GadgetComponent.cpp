// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/GadgetComponent.h"

#define LOCTEXT_NAMESPACE "Gotham.Gadgets"

UGadgetComponent::UGadgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	auto Make = [](FText Name, float Cooldown, FLinearColor Tint, int32 Icon)
	{
		FGothamGadgetDefinition Def;
		Def.DisplayName = Name;
		Def.CooldownSeconds = Cooldown;
		Def.Tint = Tint;
		Def.IconIndex = Icon;
		return Def;
	};
	Gadgets = {
		Make(LOCTEXT("WingBlade", "Wing-Blade"), 3.f, FLinearColor(0.9f, 0.75f, 0.2f), 0),
		Make(LOCTEXT("Grapnel", "Grapnel"), 6.f, FLinearColor(0.3f, 0.7f, 1.f), 1),
		Make(LOCTEXT("Smoke", "Smoke Pellet"), 10.f, FLinearColor(0.7f, 0.4f, 1.f), 2),
	};
	Remaining.Init(0.f, Gadgets.Num());
}

void UGadgetComponent::SetGadgets(const TArray<FGothamGadgetDefinition>& NewGadgets)
{
	Gadgets = NewGadgets;
	Remaining.Init(0.f, Gadgets.Num());
	BroadcastAll();
}

float UGadgetComponent::GetCooldownRemaining(int32 SlotIndex) const
{
	return Remaining.IsValidIndex(SlotIndex) ? Remaining[SlotIndex] : 0.f;
}

bool UGadgetComponent::UseGadget(int32 SlotIndex)
{
	if (!Gadgets.IsValidIndex(SlotIndex) || Remaining.Num() != Gadgets.Num() || Remaining[SlotIndex] > 0.f)
	{
		return false;
	}

	Remaining[SlotIndex] = Gadgets[SlotIndex].CooldownSeconds;
	OnGadgetUsed.Broadcast(SlotIndex);
	OnCooldownChanged.Broadcast(SlotIndex, Remaining[SlotIndex], Gadgets[SlotIndex].CooldownSeconds);
	if (IsRegistered())
	{
		SetComponentTickEnabled(true);
	}
	return true;
}

void UGadgetComponent::BroadcastAll() const
{
	for (int32 i = 0; i < Gadgets.Num(); ++i)
	{
		OnCooldownChanged.Broadcast(i, GetCooldownRemaining(i), Gadgets[i].CooldownSeconds);
	}
}

void UGadgetComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UGadgetComponent::Advance(float DeltaTime)
{
	bool bAnyCooling = false;
	for (int32 i = 0; i < Remaining.Num(); ++i)
	{
		if (Remaining[i] > 0.f)
		{
			Remaining[i] = FMath::Max(0.f, Remaining[i] - DeltaTime);
			OnCooldownChanged.Broadcast(i, Remaining[i], Gadgets[i].CooldownSeconds);
			bAnyCooling |= Remaining[i] > 0.f;
		}
	}
	if (!bAnyCooling)
	{
		if (IsRegistered())
		{
			SetComponentTickEnabled(false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
