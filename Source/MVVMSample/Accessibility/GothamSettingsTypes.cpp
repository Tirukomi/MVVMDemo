// Copyright Epic Games, Inc. All Rights Reserved.

#include "Accessibility/GothamSettingsTypes.h"

#include "Accessibility/GothamSettingsTable.h"

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
	if (Setting >= EGothamSetting::Count)
	{
		return false;
	}
	const FGothamSettingDescriptor& Desc = GothamSettingsTable::Find(Setting);
	const FGothamSettingsData Before = *this;
	// A value outside the choices (a language set by a dev flag) steps from the first choice.
	Desc.SetIndex(*this, Desc.Stepped(FMath::Max(Desc.GetIndex(*this), 0), Direction));
	return *this != Before;
}

void FGothamSettingsData::GetOptionPosition(EGothamSetting Setting, int32& OutIndex, int32& OutCount) const
{
	if (Setting >= EGothamSetting::Count)
	{
		OutIndex = 0;
		OutCount = 1;
		return;
	}
	const FGothamSettingDescriptor& Desc = GothamSettingsTable::Find(Setting);
	OutCount = Desc.ChoiceCount;
	OutIndex = FMath::Clamp(Desc.GetIndex(*this), 0, OutCount - 1);
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
	for (const FGothamSettingDescriptor& Desc : GothamSettingsTable::Get())
	{
		switch (Desc.Storage)
		{
		case EGothamSettingStorage::Culture:
		{
			// Only a language the game offers; anything else keeps the default.
			FString Culture;
			const int32 Index = File.GetString(Section, Desc.ConfigKey, Culture)
				? GetLanguages().IndexOfByPredicate([&Culture](const FGothamLanguageOption& L) { return Culture == L.Culture; })
				: INDEX_NONE;
			if (Index != INDEX_NONE)
			{
				Desc.SetIndex(*this, Index);
			}
			break;
		}
		case EGothamSettingStorage::Bool:
		{
			bool bValue = false;
			if (File.GetBool(Section, Desc.ConfigKey, bValue))
			{
				Desc.SetIndex(*this, bValue ? 1 : 0);
			}
			break;
		}
		case EGothamSettingStorage::Int:
		{
			// Out of range: a wrapping option keeps its default, UI scale clamps to its nearest step.
			int32 Value = 0;
			if (File.GetInt(Section, Desc.ConfigKey, Value))
			{
				if (Value >= 0 && Value < Desc.ChoiceCount)
				{
					Desc.SetIndex(*this, Value);
				}
				else if (!Desc.bWraps)
				{
					Desc.SetIndex(*this, FMath::Clamp(Value, 0, Desc.ChoiceCount - 1));
				}
			}
			break;
		}
		}
	}
}

void FGothamSettingsData::SaveToConfig(FConfigFile& File, const TCHAR* Section) const
{
	for (const FGothamSettingDescriptor& Desc : GothamSettingsTable::Get())
	{
		switch (Desc.Storage)
		{
		case EGothamSettingStorage::Culture: File.SetString(Section, Desc.ConfigKey, *Language); break;
		case EGothamSettingStorage::Bool:    File.SetString(Section, Desc.ConfigKey, Desc.GetIndex(*this) != 0 ? TEXT("True") : TEXT("False")); break;
		case EGothamSettingStorage::Int:     File.SetInt64(Section, Desc.ConfigKey, Desc.GetIndex(*this)); break;
		}
	}
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
