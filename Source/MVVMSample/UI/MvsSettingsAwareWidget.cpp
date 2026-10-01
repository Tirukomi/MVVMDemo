// Copyright IG. All Rights Reserved.

#include "UI/MvsSettingsAwareWidget.h"

#include "Accessibility/MvsSettingsSubsystem.h"
#include "UI/MvsWidgetTick.h"

void UMvsSettingsAwareWidget::NativeConstruct()
{
	MvsUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FMvsSettingsData&) { ApplyTheme(); });
	ApplyTheme();
}

void UMvsSettingsAwareWidget::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

const FMvsSettingsData& UMvsSettingsAwareWidget::GetMvsSettings() const
{
	static const FMvsSettingsData Defaults;
	const UMvsSettingsSubsystem* Settings = UMvsSettingsSubsystem::Get(this);
	return Settings ? Settings->GetSettings() : Defaults;
}

FLinearColor UMvsSettingsAwareWidget::GetToken(EMvsColorToken Token) const
{
	const FMvsSettingsData& Data = GetMvsSettings();
	return MvsPalette::Resolve(Token, Data.ColorMode, Data.bHighContrast);
}

float UMvsSettingsAwareWidget::GetPanelAlpha() const
{
	return MvsPalette::PanelAlpha(GetMvsSettings().bHighContrast);
}
