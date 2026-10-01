// Copyright IG. All Rights Reserved.

#include "ViewModels/PlayerVitalsViewModel.h"

void UPlayerVitalsViewModel::SetVitals(float InHealth, float InMaxHealth)
{
	const float SafeMax = FMath::Max(InMaxHealth, 1.f);
	const float Percent = FMath::Clamp(InHealth / SafeMax, 0.f, 1.f);

	// The first value is the initial state, not damage.
	const bool bDamaged = bHasValue && InHealth < Health;
	bHasValue = true;

	UE_MVVM_SET_PROPERTY_VALUE(MaxHealth, SafeMax);
	UE_MVVM_SET_PROPERTY_VALUE(Health, InHealth);
	UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, Percent);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsLowHealth, Percent <= LowHealthThreshold);
	// Last, so a view reacting to the damage event already sees the new health.
	if (bDamaged)
	{
		UE_MVVM_SET_PROPERTY_VALUE(DamageCount, DamageCount + 1);
	}
}
