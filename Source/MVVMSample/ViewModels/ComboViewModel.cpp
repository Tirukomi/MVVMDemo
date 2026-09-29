// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/ComboViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Combo"

void UComboViewModel::SetCombo(int32 InHitCount, float InMultiplier, float InDecayAlpha)
{
	UE_MVVM_SET_PROPERTY_VALUE(HitCount, InHitCount);
	UE_MVVM_SET_PROPERTY_VALUE(Multiplier, InMultiplier);
	UE_MVVM_SET_PROPERTY_VALUE(DecayAlpha, InDecayAlpha);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsActive, InHitCount > 0);

	UE_MVVM_SET_PROPERTY_VALUE(MultiplierText,
		FText::Format(LOCTEXT("MultiplierFmt", "x{0}"), FText::AsNumber(FMath::RoundToInt(InMultiplier))));
}

#undef LOCTEXT_NAMESPACE
