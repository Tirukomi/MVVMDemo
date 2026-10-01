// Copyright IG. All Rights Reserved.

#include "Gameplay/ComboComponent.h"

UComboComponent::UComboComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

float UComboComponent::GetMultiplier() const
{
	return 1.f + static_cast<float>(HitCount / HitsPerMultiplierStep);
}

void UComboComponent::RegisterHit()
{
	++HitCount;
	TimeRemaining = DecaySeconds;
	if (IsRegistered())
	{
		SetComponentTickEnabled(true);
	}
	BroadcastCurrent();
}

void UComboComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UComboComponent::Advance(float DeltaTime)
{
	TimeRemaining -= DeltaTime;
	if (TimeRemaining <= 0.f)
	{
		HitCount = 0;
		TimeRemaining = 0.f;
		if (IsRegistered())
		{
			SetComponentTickEnabled(false);
		}
	}
	BroadcastCurrent();
}
