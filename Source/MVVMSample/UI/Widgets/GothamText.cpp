// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamText.h"

void UGothamText::SetText(FText InText)
{
	// FText::ToUpper makes a new text every call, which Slate would treat as a change and lay out again; widgets
	// that re-set the same text (most refreshes) must stay free, as they are for a plain text block.
	if (bUpperCase && bHasSource && SourceText.IdenticalTo(InText, ETextIdenticalModeFlags::DeepCompare | ETextIdenticalModeFlags::LexicalCompareInvariants))
	{
		return;
	}
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
