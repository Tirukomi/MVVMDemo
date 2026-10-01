// Copyright IG. All Rights Reserved.

#include "Accessibility/MvsSettingsTable.h"

// Same namespace and keys as the settings view model's texts, so every string keeps its translations.
#define LOCTEXT_NAMESPACE "Mvs.Settings"

namespace MvsSettingsTablePrivate
{
	FText OnOff(bool bOn)
	{
		return bOn ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off");
	}

	int32 LanguageIndex(const FMvsSettingsData& Data)
	{
		return FMvsSettingsData::GetLanguages().IndexOfByPredicate([&Data](const FMvsLanguageOption& L) { return Data.Language == L.Culture; });
	}

	template<typename TEnum>
	constexpr int32 CountOf() { return static_cast<int32>(TEnum::Count); }
}

int32 FMvsSettingDescriptor::Stepped(int32 Index, int32 Direction) const
{
	return bWraps
		? ((Index + Direction) % ChoiceCount + ChoiceCount) % ChoiceCount
		: FMath::Clamp(Index + Direction, 0, ChoiceCount - 1);
}

namespace MvsSettingsTable
{
	const TArray<FMvsSettingDescriptor>& Get()
	{
		using namespace MvsSettingsTablePrivate;
		using EStore = EMvsSettingStorage;
		using FData = FMvsSettingsData;
		static const TArray<FMvsSettingDescriptor> Table = {
			{ EMvsSetting::Language, TEXT("Language"), EStore::Culture,
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
			{ EMvsSetting::ColorVision, TEXT("ColorMode"), EStore::Int,
				LOCTEXT("ColorVisionLabel", "Colour vision"),
				LOCTEXT("ColorVisionDesc", "Swaps the clue, danger and highlight colours for palettes that stay distinct with protanopia, deuteranopia or tritanopia."),
				CountOf<EMvsColorMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.ColorMode); },
				[](FData& D, int32 I) { D.ColorMode = static_cast<EMvsColorMode>(I); },
				[](const FData& D)
				{
					switch (D.ColorMode)
					{
					case EMvsColorMode::Protanopia:   return LOCTEXT("Protanopia", "Protanopia");
					case EMvsColorMode::Deuteranopia: return LOCTEXT("Deuteranopia", "Deuteranopia");
					case EMvsColorMode::Tritanopia:   return LOCTEXT("Tritanopia", "Tritanopia");
					default:                             return LOCTEXT("ColorDefault", "Standard");
					}
				} },
			{ EMvsSetting::UIScale, TEXT("UIScaleIndex"), EStore::Int,
				LOCTEXT("UIScaleLabel", "UI scale"),
				LOCTEXT("UIScaleDesc", "Scales every menu and HUD element. Layouts reflow, so nothing is cut off at larger sizes."),
				FData::GetUIScaleSteps().Num(), false,
				[](const FData& D) { return D.UIScaleIndex; },
				[](FData& D, int32 I) { D.UIScaleIndex = I; },
				[](const FData& D) { return FText::Format(LOCTEXT("PercentFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(D.GetUIScale() * 100.f))); } },
			{ EMvsSetting::TextSize, TEXT("TextSize"), EStore::Int,
				LOCTEXT("TextSizeLabel", "Text size"),
				LOCTEXT("TextSizeDesc", "Makes all menu and HUD text larger without scaling the rest of the layout. Subtitles have their own size."),
				CountOf<EMvsTextSize>(), false,
				[](const FData& D) { return static_cast<int32>(D.TextSize); },
				[](FData& D, int32 I) { D.TextSize = static_cast<EMvsTextSize>(I); },
				[](const FData& D)
				{
					switch (D.TextSize)
					{
					case EMvsTextSize::Large:  return LOCTEXT("SizeLarge", "Large");
					case EMvsTextSize::Larger: return LOCTEXT("SizeLarger", "Larger");
					default:                      return LOCTEXT("SizeStandard", "Standard");
					}
				} },
			{ EMvsSetting::HighContrast, TEXT("HighContrast"), EStore::Bool,
				LOCTEXT("HighContrastLabel", "High contrast"),
				LOCTEXT("HighContrastDesc", "Solid panels and brighter text and edges, for readability over busy scenes."),
				2, true,
				[](const FData& D) { return D.bHighContrast ? 1 : 0; },
				[](FData& D, int32 I) { D.bHighContrast = I != 0; },
				[](const FData& D) { return OnOff(D.bHighContrast); } },
			{ EMvsSetting::ReducedMotion, TEXT("ReducedMotion"), EStore::Bool,
				LOCTEXT("ReducedMotionLabel", "Reduced motion"),
				LOCTEXT("ReducedMotionDesc", "Turns off pops, slides, pulses, screen transitions and rain streaks. Colour cues stay on."),
				2, true,
				[](const FData& D) { return D.bReducedMotion ? 1 : 0; },
				[](FData& D, int32 I) { D.bReducedMotion = I != 0; },
				[](const FData& D) { return OnOff(D.bReducedMotion); } },
			{ EMvsSetting::WheelMode, TEXT("WheelMode"), EStore::Int,
				LOCTEXT("WheelModeLabel", "Gadget wheel"),
				LOCTEXT("WheelModeDesc", "Hold: the gadget wheel stays open while the button is held. Toggle: press once to open and again to close."),
				CountOf<EMvsWheelMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.WheelMode); },
				[](FData& D, int32 I) { D.WheelMode = static_cast<EMvsWheelMode>(I); },
				[](const FData& D) { return D.WheelMode == EMvsWheelMode::Hold ? LOCTEXT("WheelHold", "Hold") : LOCTEXT("WheelToggle", "Toggle"); } },
			{ EMvsSetting::ScanMode, TEXT("ScanMode"), EStore::Int,
				LOCTEXT("ScanModeLabel", "Clue analysis"),
				LOCTEXT("ScanModeDesc", "Hold: keep the button held to analyse a clue. Tap: a single press analyses it."),
				CountOf<EMvsScanMode>(), true,
				[](const FData& D) { return static_cast<int32>(D.ScanMode); },
				[](FData& D, int32 I) { D.ScanMode = static_cast<EMvsScanMode>(I); },
				[](const FData& D) { return D.ScanMode == EMvsScanMode::Hold ? LOCTEXT("ScanHold", "Hold") : LOCTEXT("ScanTap", "Tap"); } },
			{ EMvsSetting::SubtitleSize, TEXT("SubtitleSize"), EStore::Int,
				LOCTEXT("SubtitleSizeLabel", "Subtitle size"),
				LOCTEXT("SubtitleSizeDesc", "Text size for subtitles and speaker names."),
				CountOf<EMvsSubtitleSize>(), true,
				[](const FData& D) { return static_cast<int32>(D.SubtitleSize); },
				[](FData& D, int32 I) { D.SubtitleSize = static_cast<EMvsSubtitleSize>(I); },
				[](const FData& D)
				{
					switch (D.SubtitleSize)
					{
					case EMvsSubtitleSize::Small: return LOCTEXT("SizeSmall", "Small");
					case EMvsSubtitleSize::Large: return LOCTEXT("SizeLarge", "Large");
					default:                         return LOCTEXT("SizeMedium", "Medium");
					}
				} },
			{ EMvsSetting::SubtitleBackground, TEXT("SubtitleBackground"), EStore::Bool,
				LOCTEXT("SubtitleBackgroundLabel", "Subtitle background"),
				LOCTEXT("SubtitleBackgroundDesc", "Draws a solid panel behind subtitles so they read over any scene."),
				2, true,
				[](const FData& D) { return D.bSubtitleBackground ? 1 : 0; },
				[](FData& D, int32 I) { D.bSubtitleBackground = I != 0; },
				[](const FData& D) { return OnOff(D.bSubtitleBackground); } },
		};
		return Table;
	}

	const FMvsSettingDescriptor& Find(EMvsSetting Setting)
	{
		const TArray<FMvsSettingDescriptor>& Table = Get();
		const int32 Index = static_cast<int32>(Setting);
		check(Table.IsValidIndex(Index) && Table[Index].Id == Setting);
		return Table[Index];
	}
}

#undef LOCTEXT_NAMESPACE
