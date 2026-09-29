// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/DetectiveViewModel.h"

void UDetectiveViewModel::SetState(bool bInActive, float InAlpha)
{
	const float Clamped = FMath::Clamp(InAlpha, 0.f, 1.f);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsActive, bInActive);
	UE_MVVM_SET_PROPERTY_VALUE(Alpha, Clamped);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsVisible, bInActive || Clamped > 0.f);
}
