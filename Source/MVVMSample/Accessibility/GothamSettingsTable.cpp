// Copyright Epic Games, Inc. All Rights Reserved.

#include "Accessibility/GothamSettingsTable.h"

// Same namespace and keys as the settings view model's texts, so every string keeps its translations.
#define LOCTEXT_NAMESPACE "Gotham.Settings"

namespace GothamSettingsTablePrivate
{
	FText OnOff(bool bOn)
	{
		return bOn ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off");
	}

	int32 LanguageIndex(const FGothamSettingsData& Data)
	{
		return FGothamSettingsData::GetLanguages().IndexOfByPredicate([&Data](const FGothamLanguageOption& L) { return Data.Language == L.Culture; });
	}

	template<typename TEnum>
	constexpr int32 CountOf() { return static_cast<int32>(TEnum::Count); }
}

int32 FGothamSettingDescriptor::Stepped(int32 Index, int32 Direction) const
{
	return bWraps
		? ((Index + Direction) % ChoiceCount + ChoiceCount) % ChoiceCount
		: FMath::Clamp(Index + Direction, 0, ChoiceCount - 1);
}

namespace GothamSettingsTable
{
	const TArray<FGothamSettingDescriptor>& Get()
	{
		using namespace GothamSettingsTablePrivate;
		using EStore = EGothamSettingStorage;
		using FData = FGothamSettingsData;
		static const TArray<FGothamSettingDescriptor> Table = {
			{ EGothamSetting::Language, TEXT("Language"), EStore::Culture,
				LOCTEXT("LanguageLabel", "Language"),
				LOCTEXT("LanguageDesc", "Language for all menus, the HUD and subtitles. Changes preview immediately."),
				FData::GetLanguages().Num(), true,
				[](const FData& D) { return LanguageIndex(D); },
				[](FData& D, int32 I) { D.Language = FData::GetLanguages()[I].Culture; },
				[](const FData& D)
				{
					// Languages are named in their own language, never translated.
					const int32 I = LanguageIndex(D);
					return FText::FromString(I >= 0 ? FString(FData::GetLanguages()[I].NativeName) : D.Language);
				} },
			{ EGothamSetting::ColorVision, TEXT("ColorMode"), EStore::Int,
				LOCTEXT("ColorVisionLabel", "Colour vision"),
				LOCTEXT("ColorVisionDesc", "Swaps the clue, danger and highlight colours for palettes that stay distinct with protanopia, deuteranopia or tritanopia."),
				CountOf<EGothamColorMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.ColorMode); },
				[](FData& D, int32 I) { D.ColorMode = static_cast<EGothamColorMode>(I); },
				[](const FData& D)
				{
					switch (D.ColorMode)
					{
					case EGothamColorMode::Protanopia:   return LOCTEXT("Protanopia", "Protanopia");
					case EGothamColorMode::Deuteranopia: return LOCTEXT("Deuteranopia", "Deuteranopia");
					case EGothamColorMode::Tritanopia:   return LOCTEXT("Tritanopia", "Tritanopia");
					default:                             return LOCTEXT("ColorDefault", "Standard");
					}
				} },
			{ EGothamSetting::UIScale, TEXT("UIScaleIndex"), EStore::Int,
				LOCTEXT("UIScaleLabel", "UI scale"),
				LOCTEXT("UIScaleDesc", "Scales every menu and HUD element. Layouts reflow, so nothing is cut off at larger sizes."),
				FData::GetUIScaleSteps().Num(), false,
				[](const FData& D) { return D.UIScaleIndex; },
				[](FData& D, int32 I) { D.UIScaleIndex = I; },
				[](const FData& D) { return FText::Format(LOCTEXT("PercentFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(D.GetUIScale() * 100.f))); } },
			{ EGothamSetting::TextSize, TEXT("TextSize"), EStore::Int,
				LOCTEXT("TextSizeLabel", "Text size"),
				LOCTEXT("TextSizeDesc", "Makes all menu and HUD text larger without scaling the rest of the layout. Subtitles have their own size."),
				CountOf<EGothamTextSize>(), false,
				[](const FData& D) { return static_cast<int32>(D.TextSize); },
				[](FData& D, int32 I) { D.TextSize = static_cast<EGothamTextSize>(I); },
				[](const FData& D)
				{
					switch (D.TextSize)
					{
					case EGothamTextSize::Large:  return LOCTEXT("SizeLarge", "Large");
					case EGothamTextSize::Larger: return LOCTEXT("SizeLarger", "Larger");
					default:                      return LOCTEXT("SizeStandard", "Standard");
					}
				} },
			{ EGothamSetting::HighContrast, TEXT("HighContrast"), EStore::Bool,
				LOCTEXT("HighContrastLabel", "High contrast"),
				LOCTEXT("HighContrastDesc", "Solid panels and brighter text and edges, for readability over busy scenes."),
				2, true,
				[](const FData& D) { return D.bHighContrast ? 1 : 0; },
				[](FData& D, int32 I) { D.bHighContrast = I != 0; },
				[](const FData& D) { return OnOff(D.bHighContrast); } },
			{ EGothamSetting::ReducedMotion, TEXT("ReducedMotion"), EStore::Bool,
				LOCTEXT("ReducedMotionLabel", "Reduced motion"),
				LOCTEXT("ReducedMotionDesc", "Turns off pops, slides, pulses, screen transitions and rain streaks. Colour cues stay on."),
				2, true,
				[](const FData& D) { return D.bReducedMotion ? 1 : 0; },
				[](FData& D, int32 I) { D.bReducedMotion = I != 0; },
				[](const FData& D) { return OnOff(D.bReducedMotion); } },
			{ EGothamSetting::WheelMode, TEXT("WheelMode"), EStore::Int,
				LOCTEXT("WheelModeLabel", "Gadget wheel"),
				LOCTEXT("WheelModeDesc", "Hold: the gadget wheel stays open while the button is held. Toggle: press once to open and again to close."),
				CountOf<EGothamWheelMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.WheelMode); },
				[](FData& D, int32 I) { D.WheelMode = static_cast<EGothamWheelMode>(I); },
				[](const FData& D) { return D.WheelMode == EGothamWheelMode::Hold ? LOCTEXT("WheelHold", "Hold") : LOCTEXT("WheelToggle", "Toggle"); } },
			{ EGothamSetting::ScanMode, TEXT("ScanMode"), EStore::Int,
				LOCTEXT("ScanModeLabel", "Clue analysis"),
				LOCTEXT("ScanModeDesc", "Hold: keep the button held to analyse a clue. Tap: a single press analyses it."),
				CountOf<EGothamScanMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.ScanMode); },
				[](FData& D, int32 I) { D.ScanMode = static_cast<EGothamScanMode>(I); },
				[](const FData& D) { return D.ScanMode == EGothamScanMode::Hold ? LOCTEXT("ScanHold", "Hold") : LOCTEXT("ScanTap", "Tap"); } },
			{ EGothamSetting::SubtitleSize, TEXT("SubtitleSize"), EStore::Int,
				LOCTEXT("SubtitleSizeLabel", "Subtitle size"),
				LOCTEXT("SubtitleSizeDesc", "Text size for subtitles and speaker names."),
				CountOf<EGothamSubtitleSize>(), true,
				[](const FData& D) { return static_cast<int32>(D.SubtitleSize); },
				[](FData& D, int32 I) { D.SubtitleSize = static_cast<EGothamSubtitleSize>(I); },
				[](const FData& D)
				{
					switch (D.SubtitleSize)
					{
					case EGothamSubtitleSize::Small: return LOCTEXT("SizeSmall", "Small");
					case EGothamSubtitleSize::Large: return LOCTEXT("SizeLarge", "Large");
					default:                         return LOCTEXT("SizeMedium", "Medium");
					}
				} },
			{ EGothamSetting::SubtitleBackground, TEXT("SubtitleBackground"), EStore::Bool,
				LOCTEXT("SubtitleBackgroundLabel", "Subtitle background"),
				LOCTEXT("SubtitleBackgroundDesc", "Draws a solid panel behind subtitles so they read over any scene."),
				2, true,
				[](const FData& D) { return D.bSubtitleBackground ? 1 : 0; },
				[](FData& D, int32 I) { D.bSubtitleBackground = I != 0; },
				[](const FData& D) { return OnOff(D.bSubtitleBackground); } },
		};
		return Table;
	}

	const FGothamSettingDescriptor& Find(EGothamSetting Setting)
	{
		const TArray<FGothamSettingDescriptor>& Table = Get();
		const int32 Index = static_cast<int32>(Setting);
		check(Table.IsValidIndex(Index) && Table[Index].Id == Setting);
		return Table[Index];
	}
}

#undef LOCTEXT_NAMESPACE
