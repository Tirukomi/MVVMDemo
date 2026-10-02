// Copyright IG. All Rights Reserved.

#include "UI/Widgets/MvsText.h"

void UMvsText::SetText(FText InText)
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

void UMvsText::SetUpperCase(bool bInUpperCase)
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

void UMvsText::ApplyText()
{
	Super::SetText(bUpperCase ? SourceText.ToUpper() : SourceText);
}

void UMvsText::SetTextStyle(EMvsTextStyle InStyle)
{
	TextStyle = InStyle;
	ApplyTheme(true);
}

void UMvsText::SetColorToken(EMvsColorToken InToken, float InAlpha)
{
	ColorToken = InToken;
	ColorAlpha = InAlpha;
	ApplyTheme(true);
}

void UMvsText::ApplyTheme(bool bForce)
{
	if (!TextStyle && !ColorToken)
	{
		return;
	}
	const FMvsTheme Theme = MvsStyle::Theme(this);
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

void UMvsText::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	static_assert(static_cast<int32>(EMvsDesignerTextStyle::Key) == static_cast<int32>(EMvsTextStyle::Key) + 1, "EMvsDesignerTextStyle mirrors EMvsTextStyle");
	static_assert(static_cast<int32>(EMvsDesignerColor::Accent) == static_cast<int32>(EMvsColorToken::Accent) + 1, "EMvsDesignerColor mirrors EMvsColorToken");
	if (Style != EMvsDesignerTextStyle::None)
	{
		const EMvsTextStyle TypeStyle = static_cast<EMvsTextStyle>(static_cast<uint8>(Style) - 1);
		SetTextStyle(TypeStyle);
		SetUpperCase(MvsStyle::IsUpperCase(TypeStyle));
	}
	if (Color != EMvsDesignerColor::None)
	{
		SetColorToken(static_cast<EMvsColorToken>(static_cast<uint8>(Color) - 1), ColorOpacity);
	}
}

TSharedRef<SWidget> UMvsText::RebuildWidget()
{
	// Subscribed once the text is first built (the world, and with it the settings, is known by now). The subscription
	// is scoped to this object, so it lasts while the text exists, across list rows being released and rebuilt.
	if (!SettingsListener.IsBound())
	{
		SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyTheme(); });
	}
	ApplyTheme();
	return Super::RebuildWidget();
}

namespace MvsText
{
	void SetUpperCase(UTextBlock* Text, bool bUpperCase)
	{
		if (UMvsText* Mvs = Cast<UMvsText>(Text))
		{
			Mvs->SetTextTransformPolicy(ETextTransformPolicy::None);
			Mvs->SetUpperCase(bUpperCase);
		}
		else if (Text)
		{
			Text->SetTextTransformPolicy(bUpperCase ? ETextTransformPolicy::ToUpper : ETextTransformPolicy::None);
		}
	}
}
