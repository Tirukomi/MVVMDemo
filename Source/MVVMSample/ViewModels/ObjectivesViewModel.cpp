// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/ObjectivesViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Objectives"

void UObjectivesViewModel::SetProgress(int32 InFound, int32 InTotal)
{
	const int32 Total = FMath::Max(InTotal, 0);
	const int32 Found = FMath::Clamp(InFound, 0, Total);
	const bool bComplete = Total > 0 && Found >= Total;

	UE_MVVM_SET_PROPERTY_VALUE(ObjectiveTitle, bComplete ? LOCTEXT("Complete", "Case solved") : LOCTEXT("Find", "Find the clues"));
	UE_MVVM_SET_PROPERTY_VALUE(FoundCount, Found);
	UE_MVVM_SET_PROPERTY_VALUE(TotalCount, Total);
	UE_MVVM_SET_PROPERTY_VALUE(ProgressPercent, Total > 0 ? static_cast<float>(Found) / Total : 0.f);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsComplete, bComplete);
	UE_MVVM_SET_PROPERTY_VALUE(ProgressText,
		FText::Format(LOCTEXT("ProgressFmt", "{0} / {1}"), FText::AsNumber(Found), FText::AsNumber(Total)));
}

#undef LOCTEXT_NAMESPACE
