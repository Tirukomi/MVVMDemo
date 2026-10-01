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

void UGothamText::SetTextStyle(EGothamTextStyle InStyle)
{
	TextStyle = InStyle;
	ApplyTheme(true);
}

void UGothamText::SetColorToken(EGothamColorToken InToken, float InAlpha)
{
	ColorToken = InToken;
	ColorAlpha = InAlpha;
	ApplyTheme(true);
}

void UGothamText::ApplyTheme(bool bForce)
{
	if (!TextStyle && !ColorToken)
	{
		return;
	}
	const FGothamTheme Theme = GothamStyle::Theme(this);
	// List rows rebuild their Slate widgets as they scroll back into view; the style is still right then.
	if (!bForce && AppliedTheme == Theme)
	{
		return;
	}
	AppliedTheme = Theme;
	if (TextStyle)
	{
		const FSlateFontInfo Styled = Theme.Font(*TextStyle);
		if (!GetFont().IsIdenticalTo(Styled))
		{
			SetFont(Styled);
		}
	}
	if (ColorToken)
	{
		SetColorAndOpacity(FSlateColor(Theme.Color(*ColorToken, ColorAlpha)));
	}
}

TSharedRef<SWidget> UGothamText::RebuildWidget()
{
	// Subscribed once the text is first built (the world, and with it the settings, is known by now). The subscription
	// is scoped to this object, so it lasts while the text exists, across list rows being released and rebuilt.
	if (!SettingsListener.IsBound())
	{
		SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyTheme(); });
	}
	ApplyTheme();
	return Super::RebuildWidget();
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
