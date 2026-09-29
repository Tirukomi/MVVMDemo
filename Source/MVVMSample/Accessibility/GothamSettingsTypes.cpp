// Copyright Epic Games, Inc. All Rights Reserved.

#include "Accessibility/GothamSettingsTypes.h"

namespace
{
	template<typename TEnum>
	TEnum ClampEnum(int32 Value, TEnum Fallback)
	{
		return (Value >= 0 && Value < static_cast<int32>(TEnum::Count)) ? static_cast<TEnum>(Value) : Fallback;
	}

	template<typename TEnum>
	TEnum Step(TEnum Value, int32 Direction)
	{
		const int32 Count = static_cast<int32>(TEnum::Count);
		return static_cast<TEnum>(((static_cast<int32>(Value) + Direction) % Count + Count) % Count);
	}
}

const TArray<float>& FGothamSettingsData::GetUIScaleSteps()
{
	static const TArray<float> Steps = { 0.85f, 1.f, 1.15f, 1.3f, 1.5f };
	return Steps;
}

const TArray<FGothamLanguageOption>& FGothamSettingsData::GetLanguages()
{
	static const TArray<FGothamLanguageOption> Languages = {
		{ TEXT("en"), TEXT("English") },
		{ TEXT("de"), TEXT("Deutsch") },
		{ TEXT("ja"), TEXT("日本語") },
#if !UE_BUILD_SHIPPING
		// Accented, lengthened pseudo-locale for layout stress testing.
		{ TEXT("en-XA"), TEXT("Pseudo (test)") },
#endif
	};
	return Languages;
}

float FGothamSettingsData::GetUIScale() const
{
	const TArray<float>& Steps = GetUIScaleSteps();
	return Steps[FMath::Clamp(UIScaleIndex, 0, Steps.Num() - 1)];
}

int32 FGothamSettingsData::GetSubtitleFontSize() const
{
	switch (SubtitleSize)
	{
	case EGothamSubtitleSize::Small: return 16;
	case EGothamSubtitleSize::Large: return 30;
	default: return 22;
	}
}

bool FGothamSettingsData::Cycle(EGothamSetting Setting, int32 Direction)
{
	const FGothamSettingsData Before = *this;
	switch (Setting)
	{
	case EGothamSetting::Language:
	{
		const TArray<FGothamLanguageOption>& Languages = GetLanguages();
		int32 Index = Languages.IndexOfByPredicate([this](const FGothamLanguageOption& L) { return Language == L.Culture; });
		Index = ((FMath::Max(Index, 0) + Direction) % Languages.Num() + Languages.Num()) % Languages.Num();
		Language = Languages[Index].Culture;
		break;
	}
	case EGothamSetting::ColorVision:        ColorMode = Step(ColorMode, Direction); break;
	case EGothamSetting::UIScale:            UIScaleIndex = FMath::Clamp(UIScaleIndex + Direction, 0, GetUIScaleSteps().Num() - 1); break;
	case EGothamSetting::HighContrast:       bHighContrast = !bHighContrast; break;
	case EGothamSetting::ReducedMotion:      bReducedMotion = !bReducedMotion; break;
	case EGothamSetting::WheelMode:          WheelMode = Step(WheelMode, Direction); break;
	case EGothamSetting::ScanMode:           ScanMode = Step(ScanMode, Direction); break;
	case EGothamSetting::SubtitleSize:       SubtitleSize = Step(SubtitleSize, Direction); break;
	case EGothamSetting::SubtitleBackground: bSubtitleBackground = !bSubtitleBackground; break;
	default: break;
	}
	return *this != Before;
}

void FGothamSettingsData::GetOptionPosition(EGothamSetting Setting, int32& OutIndex, int32& OutCount) const
{
	auto Enum = [&OutIndex, &OutCount](auto Value)
	{
		OutIndex = static_cast<int32>(Value);
		OutCount = static_cast<int32>(decltype(Value)::Count);
	};
	auto Bool = [&OutIndex, &OutCount](bool bValue) { OutIndex = bValue ? 1 : 0; OutCount = 2; };
	switch (Setting)
	{
	case EGothamSetting::Language:
		OutCount = GetLanguages().Num();
		OutIndex = FMath::Max(0, GetLanguages().IndexOfByPredicate([this](const FGothamLanguageOption& L) { return Language == L.Culture; }));
		break;
	case EGothamSetting::ColorVision:        Enum(ColorMode); break;
	case EGothamSetting::UIScale:            OutCount = GetUIScaleSteps().Num(); OutIndex = FMath::Clamp(UIScaleIndex, 0, OutCount - 1); break;
	case EGothamSetting::HighContrast:       Bool(bHighContrast); break;
	case EGothamSetting::ReducedMotion:      Bool(bReducedMotion); break;
	case EGothamSetting::WheelMode:          Enum(WheelMode); break;
	case EGothamSetting::ScanMode:           Enum(ScanMode); break;
	case EGothamSetting::SubtitleSize:       Enum(SubtitleSize); break;
	case EGothamSetting::SubtitleBackground: Bool(bSubtitleBackground); break;
	default:                                 OutIndex = 0; OutCount = 1; break;
	}
}

bool FGothamSettingsData::operator==(const FGothamSettingsData& Other) const
{
	return Language == Other.Language && ColorMode == Other.ColorMode && UIScaleIndex == Other.UIScaleIndex
		&& bHighContrast == Other.bHighContrast && bReducedMotion == Other.bReducedMotion && WheelMode == Other.WheelMode && ScanMode == Other.ScanMode
		&& SubtitleSize == Other.SubtitleSize && bSubtitleBackground == Other.bSubtitleBackground;
}

void FGothamSettingsData::LoadFromConfig(const FConfigFile& File, const TCHAR* Section)
{
	*this = FGothamSettingsData();

	FString Lang;
	if (File.GetString(Section, TEXT("Language"), Lang) && GetLanguages().ContainsByPredicate([&Lang](const FGothamLanguageOption& L) { return Lang == L.Culture; }))
	{
		Language = Lang;
	}

	int32 Int = 0;
	bool bBool = false;
	if (File.GetInt(Section, TEXT("ColorMode"), Int)) { ColorMode = ClampEnum(Int, ColorMode); }
	if (File.GetInt(Section, TEXT("UIScaleIndex"), Int)) { UIScaleIndex = FMath::Clamp(Int, 0, GetUIScaleSteps().Num() - 1); }
	if (File.GetBool(Section, TEXT("HighContrast"), bBool)) { bHighContrast = bBool; }
	if (File.GetBool(Section, TEXT("ReducedMotion"), bBool)) { bReducedMotion = bBool; }
	if (File.GetInt(Section, TEXT("WheelMode"), Int)) { WheelMode = ClampEnum(Int, WheelMode); }
	if (File.GetInt(Section, TEXT("ScanMode"), Int)) { ScanMode = ClampEnum(Int, ScanMode); }
	if (File.GetInt(Section, TEXT("SubtitleSize"), Int)) { SubtitleSize = ClampEnum(Int, SubtitleSize); }
	if (File.GetBool(Section, TEXT("SubtitleBackground"), bBool)) { bSubtitleBackground = bBool; }
}

void FGothamSettingsData::SaveToConfig(FConfigFile& File, const TCHAR* Section) const
{
	File.SetString(Section, TEXT("Language"), *Language);
	File.SetInt64(Section, TEXT("ColorMode"), static_cast<int64>(ColorMode));
	File.SetInt64(Section, TEXT("UIScaleIndex"), UIScaleIndex);
	File.SetString(Section, TEXT("HighContrast"), bHighContrast ? TEXT("True") : TEXT("False"));
	File.SetString(Section, TEXT("ReducedMotion"), bReducedMotion ? TEXT("True") : TEXT("False"));
	File.SetInt64(Section, TEXT("WheelMode"), static_cast<int64>(WheelMode));
	File.SetInt64(Section, TEXT("ScanMode"), static_cast<int64>(ScanMode));
	File.SetInt64(Section, TEXT("SubtitleSize"), static_cast<int64>(SubtitleSize));
	File.SetString(Section, TEXT("SubtitleBackground"), bSubtitleBackground ? TEXT("True") : TEXT("False"));
}

namespace GothamPalette
{
	FLinearColor Resolve(EGothamColorToken Token, EGothamColorMode Mode, bool bHighContrast)
	{
		switch (Token)
		{
		case EGothamColorToken::Panel:       return bHighContrast ? FLinearColor(0.f, 0.f, 0.f) : FLinearColor(0.012f, 0.015f, 0.02f);
		case EGothamColorToken::PanelEdge:   return bHighContrast ? FLinearColor(0.85f, 0.88f, 0.92f) : FLinearColor(0.32f, 0.38f, 0.46f);
		case EGothamColorToken::TextPrimary: return bHighContrast ? FLinearColor(1.f, 1.f, 1.f) : FLinearColor(0.88f, 0.91f, 0.95f);
		case EGothamColorToken::TextMuted:   return bHighContrast ? FLinearColor(0.85f, 0.88f, 0.92f) : FLinearColor(0.5f, 0.56f, 0.64f);
		case EGothamColorToken::Accent:      return bHighContrast ? FLinearColor(1.f, 0.8f, 0.2f) : FLinearColor(0.96f, 0.68f, 0.22f);
		default: break;
		}
		// Indexed [mode][token]. The red/green presets lean on the Okabe-Ito colour-blind-safe set (blue / orange /
		// sky / yellow); the tritan preset moves away from blue-yellow confusion toward teal / red / magenta.
		static const FLinearColor Table[static_cast<int32>(EGothamColorMode::Count)][6] = {
			// Default:      Good                           Danger                          Info                           Warning                         Unscanned                      Scanned
			{ FLinearColor(0.20f, 0.80f, 0.30f), FLinearColor(0.90f, 0.10f, 0.10f), FLinearColor(0.40f, 0.85f, 1.00f), FLinearColor(0.95f, 0.75f, 0.20f), FLinearColor(1.00f, 0.55f, 0.10f), FLinearColor(0.20f, 1.00f, 0.40f) },
			// Protanopia
			{ FLinearColor(0.00f, 0.45f, 0.70f), FLinearColor(0.90f, 0.60f, 0.00f), FLinearColor(0.35f, 0.70f, 0.90f), FLinearColor(0.95f, 0.90f, 0.25f), FLinearColor(0.90f, 0.60f, 0.00f), FLinearColor(0.35f, 0.70f, 0.90f) },
			// Deuteranopia
			{ FLinearColor(0.00f, 0.45f, 0.70f), FLinearColor(0.90f, 0.60f, 0.00f), FLinearColor(0.35f, 0.70f, 0.90f), FLinearColor(0.95f, 0.90f, 0.25f), FLinearColor(0.90f, 0.60f, 0.00f), FLinearColor(0.35f, 0.70f, 0.90f) },
			// Tritanopia
			{ FLinearColor(0.00f, 0.62f, 0.45f), FLinearColor(0.84f, 0.20f, 0.20f), FLinearColor(0.80f, 0.47f, 0.65f), FLinearColor(0.95f, 0.50f, 0.60f), FLinearColor(0.84f, 0.20f, 0.20f), FLinearColor(0.00f, 0.62f, 0.45f) },
		};

		const int32 ModeIndex = FMath::Clamp(static_cast<int32>(Mode), 0, static_cast<int32>(EGothamColorMode::Count) - 1);
		FLinearColor Color = Table[ModeIndex][static_cast<int32>(Token)];
		if (bHighContrast)
		{
			// Push toward full brightness and saturation so colours stay legible against any background.
			const float Peak = FMath::Max3(Color.R, Color.G, Color.B);
			if (Peak > KINDA_SMALL_NUMBER)
			{
				Color = FLinearColor(Color.R / Peak, Color.G / Peak, Color.B / Peak, 1.f);
			}
		}
		return Color;
	}

	float PanelAlpha(bool bHighContrast)
	{
		return bHighContrast ? 0.97f : 0.8f;
	}
}
