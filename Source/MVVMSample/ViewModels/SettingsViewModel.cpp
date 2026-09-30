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
	// After a language switch the values are the same but every text reads differently, so bump unconditionally.
	Recompute();
	BumpRevision();
}

#undef LOCTEXT_NAMESPACE
