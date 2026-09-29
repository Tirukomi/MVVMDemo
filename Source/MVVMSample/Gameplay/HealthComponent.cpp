// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/HealthComponent.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	BroadcastCurrent();
}

void UHealthComponent::ApplyDamage(float Amount)
{
	if (Amount > 0.f)
	{
		SetHealth(Health - Amount);
	}
}

void UHealthComponent::Heal(float Amount)
{
	if (Amount > 0.f)
	{
		SetHealth(Health + Amount);
	}
}

void UHealthComponent::SetMaxHealth(float NewMax, bool bRefill)
{
	MaxHealth = FMath::Max(1.f, NewMax);
	SetHealth(bRefill ? MaxHealth : Health);
	BroadcastCurrent();
}

void UHealthComponent::SetHealth(float NewHealth)
{
	const float Clamped = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	if (!FMath::IsNearlyEqual(Clamped, Health))
	{
		Health = Clamped;
		BroadcastCurrent();
	}
}
