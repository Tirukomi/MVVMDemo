// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewModels/SettingsViewModel.h"

#include "Accessibility/GothamSettingsTable.h"

#define LOCTEXT_NAMESPACE "Gotham.Settings"

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
	return Setting < EGothamSetting::Count ? GothamSettingsTable::Find(Setting).Label : FText::GetEmpty();
}

FText USettingsViewModel::GetDescription(EGothamSetting Setting)
{
	return Setting < EGothamSetting::Count ? GothamSettingsTable::Find(Setting).Description : FText::GetEmpty();
}

const TArray<FGothamSettingsTab>& USettingsViewModel::GetTabs()
{
	static const TArray<FGothamSettingsTab> Tabs = {
		{ TEXT("Display"), LOCTEXT("TabDisplay", "Display"),
			{ EGothamSetting::UIScale, EGothamSetting::SubtitleSize, EGothamSetting::SubtitleBackground } },
		{ TEXT("Accessibility"), LOCTEXT("TabAccessibility", "Accessibility"),
			{ EGothamSetting::ColorVision, EGothamSetting::HighContrast, EGothamSetting::ReducedMotion } },
		{ TEXT("Controls"), LOCTEXT("TabControls", "Controls"),
			{ EGothamSetting::WheelMode, EGothamSetting::ScanMode } },
		{ TEXT("Language"), LOCTEXT("TabLanguage", "Language"),
			{ EGothamSetting::Language } },
	};
	return Tabs;
}

FText USettingsViewModel::GetValueText(EGothamSetting Setting) const
{
	return Setting < EGothamSetting::Count ? GothamSettingsTable::Find(Setting).FormatValue(Current) : FText::GetEmpty();
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
	if (!bHasShown || Shown != Current)
	{
		Shown = Current;
		bHasShown = true;
		BumpRevision();
	}
}

void USettingsViewModel::BumpRevision()
{
	UE_MVVM_SET_PROPERTY_VALUE_INLINE(Revision, Revision + 1);
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
	BumpRevision();
}

#undef LOCTEXT_NAMESPACE
