// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamText.h"

void UGothamText::SetText(FText InText)
{
	SourceText = MoveTemp(InText);
	bHasSource = true;
	ApplyText();
}

void UGothamText::SetUpperCase(bool bInUpperCase)
{
	if (!bHasSource)
	{
		// Text assigned before the first SetText (defaults, designer values) is the source.
		SourceText = GetText();
		bHasSource = true;
	}
	if (bUpperCase != bInUpperCase)
	{
		bUpperCase = bInUpperCase;
		ApplyText();
	}
}

void UGothamText::ApplyText()
{
	Super::SetText(bUpperCase ? SourceText.ToUpper() : SourceText);
}

namespace GothamText
{
	void SetUpperCase(UTextBlock* Text, bool bUpperCase)
	{
		if (UGothamText* Gotham = Cast<UGothamText>(Text))
		{
			Gotham->SetTextTransformPolicy(ETextTransformPolicy::None);
			Gotham->SetUpperCase(bUpperCase);
		}
		else if (Text)
		{
			Text->SetTextTransformPolicy(bUpperCase ? ETextTransformPolicy::ToUpper : ETextTransformPolicy::None);
		}
	}
}
