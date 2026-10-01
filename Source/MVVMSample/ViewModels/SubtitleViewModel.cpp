// Copyright IG. All Rights Reserved.

#include "ViewModels/SubtitleViewModel.h"

void USubtitleViewModel::SetLine(const FText& InSpeaker, const FText& InText)
{
	UE_MVVM_SET_PROPERTY_VALUE(Speaker, InSpeaker);
	UE_MVVM_SET_PROPERTY_VALUE(Line, InText);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsVisible, !InText.IsEmpty());
}

void USubtitleViewModel::Clear()
{
	SetLine(FText::GetEmpty(), FText::GetEmpty());
}

void USubtitleViewModel::SetPresentation(int32 InFontSize, bool bInBackground)
{
	UE_MVVM_SET_PROPERTY_VALUE(FontSize, InFontSize);
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bHasBackground, bInBackground);
}
