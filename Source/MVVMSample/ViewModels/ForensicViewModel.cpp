// Copyright IG. All Rights Reserved.

#include "ViewModels/ForensicViewModel.h"

void UForensicViewModel::SetState(bool bInActive, float InAlpha)
{
	const float Clamped = FMath::Clamp(InAlpha, 0.f, 1.f);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsActive, bInActive);
	UE_MVVM_SET_PROPERTY_VALUE(Alpha, Clamped);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsVisible, bInActive || Clamped > 0.f);
}

void UForensicViewModel::SetAnalysis(FName InClueId, float InProgress)
{
	UE_MVVM_SET_PROPERTY_VALUE(AnalysisTargetId, InClueId);
	UE_MVVM_SET_PROPERTY_VALUE(AnalysisProgress, InClueId.IsNone() ? 0.f : FMath::Clamp(InProgress, 0.f, 1.f));
}
