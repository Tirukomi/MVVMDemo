// Copyright IG. All Rights Reserved.

#include "ViewModels/SettingsViewModel.h"

#include "Accessibility/MvsSettingsTable.h"

#define LOCTEXT_NAMESPACE "Mvs.Settings"

void USettingsViewModel::Initialize(const FMvsSettingsData& Saved)
{
	Current = Saved;
	Baseline = Saved;
	TextsLanguage = Saved.Language;
	Recompute();
}

void USettingsViewModel::Sync(const FMvsSettingsData& Saved, const FMvsSettingsData& Live)
{
	Current = Live;
	Baseline = Saved;
	if (Live.Language != TextsLanguage)
	{
		TextsLanguage = Live.Language;
		RefreshTexts();
	}
	else
	{
		Recompute();
	}
}

void USettingsViewModel::Cycle(EMvsSetting Setting, int32 Direction)
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
	const FMvsSettingsData Defaults;
	if (Current != Defaults)
	{
		Current = Defaults;
		Recompute();
		OnPreview.Broadcast(Current);
	}
}

FText USettingsViewModel::GetLabel(EMvsSetting Setting)
{
	return Setting < EMvsSetting::Count ? MvsSettingsTable::Find(Setting).Label : FText::GetEmpty();
}

FText USettingsViewModel::GetDescription(EMvsSetting Setting)
{
	return Setting < EMvsSetting::Count ? MvsSettingsTable::Find(Setting).Description : FText::GetEmpty();
}

const TArray<FMvsSettingsTab>& USettingsViewModel::GetTabs()
{
	static const TArray<FMvsSettingsTab> Tabs = {
		{ TEXT("Display"), LOCTEXT("TabDisplay", "Display"),
			{ EMvsSetting::UIScale, EMvsSetting::SubtitleSize, EMvsSetting::SubtitleBackground } },
		{ TEXT("Accessibility"), LOCTEXT("TabAccessibility", "Accessibility"),
			{ EMvsSetting::ColorVision, EMvsSetting::HighContrast, EMvsSetting::TextSize, EMvsSetting::ReducedMotion } },
		{ TEXT("Controls"), LOCTEXT("TabControls", "Controls"),
			{ EMvsSetting::WheelMode, EMvsSetting::ScanMode } },
		{ TEXT("Language"), LOCTEXT("TabLanguage", "Language"),
			{ EMvsSetting::Language } },
	};
	return Tabs;
}

FText USettingsViewModel::GetValueText(EMvsSetting Setting) const
{
	return Setting < EMvsSetting::Count ? MvsSettingsTable::Find(Setting).FormatValue(Current) : FText::GetEmpty();
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
