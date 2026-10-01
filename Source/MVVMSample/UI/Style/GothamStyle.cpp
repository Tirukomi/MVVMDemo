// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Style/GothamStyle.h"

#include "Accessibility/GothamSettingsSubsystem.h"

#include "Components/TextBlock.h"
#include "Fonts/CompositeFont.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UI/Widgets/GothamText.h"

namespace
{
	FString FontPath(const TCHAR* File)
	{
		return FPaths::ProjectContentDir() / TEXT("UI/Fonts") / File;
	}

	/** One family (a typeface with named weights) plus the Japanese sub-font. */
	TSharedPtr<const FCompositeFont> MakeFamily(TArrayView<const TPair<const TCHAR*, const TCHAR*>> Weights)
	{
		for (const auto& Weight : Weights)
		{
			if (!IFileManager::Get().FileExists(*FontPath(Weight.Value)))
			{
				return nullptr;
			}
		}
		TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>();
		for (const auto& Weight : Weights)
		{
			Font->DefaultTypeface.AppendFont(Weight.Key, FontPath(Weight.Value), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}

		const FString Japanese = FontPath(TEXT("NotoSansJP.ttf"));
		if (IFileManager::Get().FileExists(*Japanese))
		{
			FCompositeSubFont& Sub = Font->SubTypefaces.AddDefaulted_GetRef();
			Sub.CharacterRanges.Add(FInt32Range(0x3000, 0x30FF));   // CJK punctuation, hiragana, katakana
			Sub.CharacterRanges.Add(FInt32Range(0x3400, 0x4DBF));   // CJK extension A
			Sub.CharacterRanges.Add(FInt32Range(0x4E00, 0x9FFF));   // CJK unified ideographs
			Sub.CharacterRanges.Add(FInt32Range(0xFF00, 0xFFEF));   // half- and full-width forms
			for (const auto& Weight : Weights)
			{
				Sub.Typeface.AppendFont(Weight.Key, Japanese, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
			}
		}
		return Font;
	}

	const TSharedPtr<const FCompositeFont>& Condensed()
	{
		static const TPair<const TCHAR*, const TCHAR*> Weights[] = {
			{ TEXT("Medium"), TEXT("BarlowCondensed-Medium.ttf") },
			{ TEXT("SemiBold"), TEXT("BarlowCondensed-SemiBold.ttf") },
			{ TEXT("Bold"), TEXT("BarlowCondensed-Bold.ttf") },
		};
		static const TSharedPtr<const FCompositeFont> Font = MakeFamily(Weights);
		return Font;
	}

	const TSharedPtr<const FCompositeFont>& BodyFamily()
	{
		static const TPair<const TCHAR*, const TCHAR*> Weights[] = {
			{ TEXT("Regular"), TEXT("Barlow-Regular.ttf") },
			{ TEXT("SemiBold"), TEXT("Barlow-SemiBold.ttf") },
		};
		static const TSharedPtr<const FCompositeFont> Font = MakeFamily(Weights);
		return Font;
	}

	struct FStyleSpec
	{
		bool bCondensed;
		const TCHAR* Weight;
		float Size;
		int32 LetterSpacing;   // 1/1000 em
	};

	FStyleSpec Spec(EGothamTextStyle Style)
	{
		switch (Style)
		{
		case EGothamTextStyle::Display:    return { true, TEXT("Bold"), 60.f, 0 };
		case EGothamTextStyle::Title:      return { true, TEXT("SemiBold"), 38.f, 180 };
		case EGothamTextStyle::Header:     return { true, TEXT("SemiBold"), 24.f, 80 };
		case EGothamTextStyle::Label:      return { true, TEXT("Medium"), 14.f, 260 };
		case EGothamTextStyle::Numeric:    return { true, TEXT("Bold"), 22.f, 40 };
		case EGothamTextStyle::BodyStrong: return { false, TEXT("SemiBold"), 18.f, 0 };
		case EGothamTextStyle::Key:        return { true, TEXT("SemiBold"), 15.f, 40 };
		default:                           return { false, TEXT("Regular"), 18.f, 0 };
		}
	}
}

FGothamTheme FGothamTheme::FromSettings(const FGothamSettingsData& Data)
{
	FGothamTheme Theme;
	Theme.ColorMode = Data.ColorMode;
	Theme.bHighContrast = Data.bHighContrast;
	Theme.bReducedMotion = Data.bReducedMotion;
	Theme.TextScale = Data.GetTextScale();
	return Theme;
}

FLinearColor FGothamTheme::Color(EGothamColorToken Token, float Alpha) const
{
	FLinearColor Result = GothamPalette::Resolve(Token, ColorMode, bHighContrast);
	Result.A = Alpha;
	return Result;
}

float FGothamTheme::PanelAlpha() const
{
	return GothamPalette::PanelAlpha(bHighContrast);
}

FSlateFontInfo FGothamTheme::Font(EGothamTextStyle Style) const
{
	return GothamStyle::Font(Style, TextScale);
}

namespace GothamStyle
{
	FGothamTheme Theme(const UObject* Context)
	{
		const UGothamSettingsSubsystem* Settings = Context ? UGothamSettingsSubsystem::Get(Context) : nullptr;
		return Settings ? FGothamTheme::FromSettings(Settings->GetSettings()) : FGothamTheme();
	}

	FSlateFontInfo Font(EGothamTextStyle Style, float Scale)
	{
		const FStyleSpec S = Spec(Style);
		const float Size = S.Size * Scale;
		const TSharedPtr<const FCompositeFont>& Family = S.bCondensed ? Condensed() : BodyFamily();
		FSlateFontInfo Info = Family.IsValid()
			? FSlateFontInfo(Family, Size, S.Weight)
			: FCoreStyle::GetDefaultFontStyle(FCString::Strcmp(S.Weight, TEXT("Regular")) == 0 ? "Regular" : "Bold", FMath::RoundToInt(Size));
		Info.LetterSpacing = S.LetterSpacing;
		return Info;
	}

	bool IsUpperCase(EGothamTextStyle Style)
	{
		return Style == EGothamTextStyle::Title || Style == EGothamTextStyle::Label || Style == EGothamTextStyle::Header;
	}

	FLinearColor Token(const UObject* Context, EGothamColorToken InToken, float Alpha)
	{
		return Theme(Context).Color(InToken, Alpha);
	}

	FLinearColor ItemText(const UObject* Context, bool bHot)
	{
		return Token(Context, bHot ? EGothamColorToken::TextPrimary : EGothamColorToken::TextMuted);
	}

	float PanelAlpha(const UObject* Context)
	{
		return Theme(Context).PanelAlpha();
	}

	void SetTextStyle(UTextBlock* Text, EGothamTextStyle Style)
	{
		if (UGothamText* Gotham = Cast<UGothamText>(Text))
		{
			Gotham->SetTextStyle(Style);
		}
		else if (Text)
		{
			Text->SetFont(Font(Style));
		}
	}

	void ApplyText(UTextBlock* Text, EGothamTextStyle Style, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		SetTextStyle(Text, Style);
		GothamText::SetUpperCase(Text, IsUpperCase(Style));
		Text->SetColorAndOpacity(FSlateColor(Color));
	}
}
