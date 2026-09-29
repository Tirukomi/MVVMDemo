// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/SettingsViewModel.h"

#define LOCTEXT_NAMESPACE "Gotham.Settings"

namespace
{
	FText OnOff(bool bOn)
	{
		return bOn ? LOCTEXT("On", "On") : LOCTEXT("Off", "Off");
	}
}

void USettingsViewModel::Initialize(const FGothamSettingsData& Saved)
{
	Current = Saved;
	Baseline = Saved;
	Recompute();
}

void USettingsViewModel::Cycle(EGothamSetting Setting, int32 Direction)
{
	if (Current.Cycle(Setting, Direction))
	{
		Recompute();
		OnPreview.Broadcast(Current);
	}
}

void USettingsViewModel::Apply()
{
	Baseline = Current;
	Recompute();
	OnCommitted.Broadcast(Current);
}

void USettingsViewModel::Revert()
{
	if (Current != Baseline)
	{
		Current = Baseline;
		Recompute();
		OnPreview.Broadcast(Current);
	}
}

void USettingsViewModel::ResetDefaults()
{
	const FGothamSettingsData Defaults;
	if (Current != Defaults)
	{
		Current = Defaults;
		Recompute();
		OnPreview.Broadcast(Current);
	}
}

FText USettingsViewModel::GetLabel(EGothamSetting Setting)
{
	switch (Setting)
	{
	case EGothamSetting::Language:           return LOCTEXT("LanguageLabel", "Language");
	case EGothamSetting::ColorVision:        return LOCTEXT("ColorVisionLabel", "Colour vision");
	case EGothamSetting::UIScale:            return LOCTEXT("UIScaleLabel", "UI scale");
	case EGothamSetting::HighContrast:       return LOCTEXT("HighContrastLabel", "High contrast");
	case EGothamSetting::ReducedMotion:      return LOCTEXT("ReducedMotionLabel", "Reduced motion");
	case EGothamSetting::WheelMode:          return LOCTEXT("WheelModeLabel", "Gadget wheel");
	case EGothamSetting::ScanMode:           return LOCTEXT("ScanModeLabel", "Clue analysis");
	case EGothamSetting::SubtitleSize:       return LOCTEXT("SubtitleSizeLabel", "Subtitle size");
	case EGothamSetting::SubtitleBackground: return LOCTEXT("SubtitleBackgroundLabel", "Subtitle background");
	default: return FText::GetEmpty();
	}
}

FText USettingsViewModel::GetValueText(EGothamSetting Setting) const
{
	switch (Setting)
	{
	case EGothamSetting::Language:
		for (const FGothamLanguageOption& Option : FGothamSettingsData::GetLanguages())
		{
			if (Current.Language == Option.Culture)
			{
				return FText::FromString(Option.NativeName);
			}
		}
		return FText::FromString(Current.Language);
	case EGothamSetting::ColorVision:
		switch (Current.ColorMode)
		{
		case EGothamColorMode::Protanopia:   return LOCTEXT("Protanopia", "Protanopia");
		case EGothamColorMode::Deuteranopia: return LOCTEXT("Deuteranopia", "Deuteranopia");
		case EGothamColorMode::Tritanopia:   return LOCTEXT("Tritanopia", "Tritanopia");
		default:                             return LOCTEXT("ColorDefault", "Standard");
		}
	case EGothamSetting::UIScale:
		return FText::Format(LOCTEXT("PercentFmt", "{0}%"), FText::AsNumber(FMath::RoundToInt(Current.GetUIScale() * 100.f)));
	case EGothamSetting::HighContrast:       return OnOff(Current.bHighContrast);
	case EGothamSetting::ReducedMotion:      return OnOff(Current.bReducedMotion);
	case EGothamSetting::WheelMode:
		return Current.WheelMode == EGothamWheelMode::Hold ? LOCTEXT("WheelHold", "Hold") : LOCTEXT("WheelToggle", "Toggle");
	case EGothamSetting::ScanMode:
		return Current.ScanMode == EGothamScanMode::Hold ? LOCTEXT("ScanHold", "Hold") : LOCTEXT("ScanTap", "Tap");
	case EGothamSetting::SubtitleSize:
		switch (Current.SubtitleSize)
		{
		case EGothamSubtitleSize::Small: return LOCTEXT("SizeSmall", "Small");
		case EGothamSubtitleSize::Large: return LOCTEXT("SizeLarge", "Large");
		default:                         return LOCTEXT("SizeMedium", "Medium");
		}
	case EGothamSetting::SubtitleBackground: return OnOff(Current.bSubtitleBackground);
	default: return FText::GetEmpty();
	}
}

void USettingsViewModel::Recompute()
{
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(bIsDirty, Current != Baseline);
	UE_MVVM_SET_PROPERTY_VALUE(LanguageValue, GetValueText(EGothamSetting::Language));
	UE_MVVM_SET_PROPERTY_VALUE(ColorVisionValue, GetValueText(EGothamSetting::ColorVision));
	UE_MVVM_SET_PROPERTY_VALUE(UIScaleValue, GetValueText(EGothamSetting::UIScale));
	UE_MVVM_SET_PROPERTY_VALUE(HighContrastValue, GetValueText(EGothamSetting::HighContrast));
	UE_MVVM_SET_PROPERTY_VALUE(ReducedMotionValue, GetValueText(EGothamSetting::ReducedMotion));
	UE_MVVM_SET_PROPERTY_VALUE(WheelModeValue, GetValueText(EGothamSetting::WheelMode));
	UE_MVVM_SET_PROPERTY_VALUE(ScanModeValue, GetValueText(EGothamSetting::ScanMode));
	UE_MVVM_SET_PROPERTY_VALUE(SubtitleSizeValue, GetValueText(EGothamSetting::SubtitleSize));
	UE_MVVM_SET_PROPERTY_VALUE(SubtitleBackgroundValue, GetValueText(EGothamSetting::SubtitleBackground));
}

void USettingsViewModel::RefreshTexts()
{
	// After a language switch the FText objects can be identical while their displayed string changed,
	// so notify unconditionally.
	Recompute();
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(LanguageValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ColorVisionValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(UIScaleValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HighContrastValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ReducedMotionValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WheelModeValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ScanModeValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SubtitleSizeValue);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SubtitleBackgroundValue);
}

#undef LOCTEXT_NAMESPACE
