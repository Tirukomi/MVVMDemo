// Copyright IG. All Rights Reserved.

#include "ViewModels/ComboViewModel.h"

#include "Gameplay/ThreatTypes.h"

#define LOCTEXT_NAMESPACE "Mvs.Combo"

void UComboViewModel::SetCombo(int32 InHitCount, float InMultiplier, float InDecayAlpha)
{
	if (const int32 Milestone = MvsCombo::MilestoneReached(HitCount, InHitCount))
	{
		UE_MVVM_SET_PROPERTY_VALUE(MilestoneText, FText::Format(LOCTEXT("MilestoneFmt", "{0}-hit combo"), FText::AsNumber(Milestone)));
		UE_MVVM_SET_PROPERTY_VALUE(MilestoneCount, MilestoneCount + 1);
	}
	UE_MVVM_SET_PROPERTY_VALUE(HitCount, InHitCount);
	UE_MVVM_SET_PROPERTY_VALUE(Multiplier, InMultiplier);
	UE_MVVM_SET_PROPERTY_VALUE(DecayAlpha, InDecayAlpha);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsActive, InHitCount > 0);

	UE_MVVM_SET_PROPERTY_VALUE(MultiplierText,
		FText::Format(LOCTEXT("MultiplierFmt", "x{0}"), FText::AsNumber(FMath::RoundToInt(InMultiplier))));
}

#undef LOCTEXT_NAMESPACE
