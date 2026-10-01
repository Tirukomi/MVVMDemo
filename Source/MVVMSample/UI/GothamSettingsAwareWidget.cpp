// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamSettingsAwareWidget.h"

#include "Accessibility/GothamSettingsSubsystem.h"
#include "UI/GothamWidgetTick.h"

void UGothamSettingsAwareWidget::NativeConstruct()
{
	GothamUI::DisableTick(this);
	Super::NativeConstruct();
	SettingsListener.Bind(this, [this](const FGothamSettingsData&) { ApplyTheme(); });
	ApplyTheme();
}

void UGothamSettingsAwareWidget::NativeDestruct()
{
	SettingsListener.Reset();
	Super::NativeDestruct();
}

const FGothamSettingsData& UGothamSettingsAwareWidget::GetGothamSettings() const
{
	static const FGothamSettingsData Defaults;
	const UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this);
	return Settings ? Settings->GetSettings() : Defaults;
}

FLinearColor UGothamSettingsAwareWidget::GetToken(EGothamColorToken Token) const
{
	const FGothamSettingsData& Data = GetGothamSettings();
	return GothamPalette::Resolve(Token, Data.ColorMode, Data.bHighContrast);
}

float UGothamSettingsAwareWidget::GetPanelAlpha() const
{
	return GothamPalette::PanelAlpha(GetGothamSettings().bHighContrast);
}
