// Copyright IG. All Rights Reserved.

#include "UI/Style/MvsStyle.h"

#include "Accessibility/MvsSettingsSubsystem.h"

#include "Components/TextBlock.h"
#include "Fonts/CompositeFont.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UI/Widgets/MvsText.h"

DEFINE_LOG_CATEGORY_STATIC(LogMvsStyle, Log, All);

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
				// Not silent: a missing font is a staging mistake (Config/DefaultGame.ini stages Content/UI/Fonts).
				UE_LOG(LogMvsStyle, Warning, TEXT("Font %s is missing; UI text falls back to the engine font."), *FontPath(Weight.Value));
				return nullptr;
			}
		}
		TSharedRef<FStandaloneCompositeFont> Font = MakeShared<FStandaloneCompositeFont>();
		for (const auto& Weight : Weights)
		{
			Font->DefaultTypeface.AppendFont(Weight.Key, FontPath(Weight.Value), EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
		}

		const FString Japanese = FontPath(TEXT("NotoSansJP.ttf"));
		if (!IFileManager::Get().FileExists(*Japanese))
		{
			UE_LOG(LogMvsStyle, Warning, TEXT("Font %s is missing; Japanese falls back to the engine's CJK font."), *Japanese);
		}
		else
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

	FStyleSpec Spec(EMvsTextStyle Style)
	{
		switch (Style)
		{
		case EMvsTextStyle::Display:    return { true, TEXT("Bold"), 60.f, 0 };
		case EMvsTextStyle::Title:      return { true, TEXT("SemiBold"), 38.f, 180 };
		case EMvsTextStyle::Header:     return { true, TEXT("SemiBold"), 24.f, 80 };
		case EMvsTextStyle::Label:      return { true, TEXT("Medium"), 14.f, 260 };
		case EMvsTextStyle::Numeric:    return { true, TEXT("Bold"), 22.f, 40 };
		case EMvsTextStyle::BodyStrong: return { false, TEXT("SemiBold"), 18.f, 0 };
		case EMvsTextStyle::Key:        return { true, TEXT("SemiBold"), 15.f, 40 };
		default:                           return { false, TEXT("Regular"), 18.f, 0 };
		}
	}
}

FMvsTheme FMvsTheme::FromSettings(const FMvsSettingsData& Data)
{
	FMvsTheme Theme;
	Theme.ColorMode = Data.ColorMode;
	Theme.bHighContrast = Data.bHighContrast;
	Theme.bReducedMotion = Data.bReducedMotion;
	Theme.TextScale = Data.GetTextScale();
	return Theme;
}

FLinearColor FMvsTheme::Color(EMvsColorToken Token, float Alpha) const
{
	FLinearColor Result = MvsPalette::Resolve(Token, ColorMode, bHighContrast);
	Result.A = Alpha;
	return Result;
}

float FMvsTheme::PanelAlpha() const
{
	return MvsPalette::PanelAlpha(bHighContrast);
}

FSlateFontInfo FMvsTheme::Font(EMvsTextStyle Style) const
{
	return MvsStyle::Font(Style, TextScale);
}

namespace MvsStyle
{
	FMvsTheme Theme(const UObject* Context)
	{
		const UMvsSettingsSubsystem* Settings = Context ? UMvsSettingsSubsystem::Get(Context) : nullptr;
		return Settings ? FMvsTheme::FromSettings(Settings->GetSettings()) : FMvsTheme();
	}

	FSlateFontInfo Font(EMvsTextStyle Style, float Scale)
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

	bool IsUpperCase(EMvsTextStyle Style)
	{
		return Style == EMvsTextStyle::Title || Style == EMvsTextStyle::Label || Style == EMvsTextStyle::Header;
	}

	FLinearColor Token(const UObject* Context, EMvsColorToken InToken, float Alpha)
	{
		return Theme(Context).Color(InToken, Alpha);
	}

	FLinearColor ItemText(const UObject* Context, bool bHot)
	{
		return Theme(Context).ItemText(bHot);
	}

	float PanelAlpha(const UObject* Context)
	{
		return Theme(Context).PanelAlpha();
	}

	void SetTextStyle(UTextBlock* Text, EMvsTextStyle Style)
	{
		if (UMvsText* Mvs = Cast<UMvsText>(Text))
		{
			Mvs->SetTextStyle(Style);
		}
		else if (Text)
		{
			Text->SetFont(Font(Style));
		}
	}

	void ApplyText(UTextBlock* Text, EMvsTextStyle Style, const FLinearColor& Color)
	{
		if (!Text)
		{
			return;
		}
		SetTextStyle(Text, Style);
		MvsText::SetUpperCase(Text, IsUpperCase(Style));
		Text->SetColorAndOpacity(FSlateColor(Color));
	}
}
