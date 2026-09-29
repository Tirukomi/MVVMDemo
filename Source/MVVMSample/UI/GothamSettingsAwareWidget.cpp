// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamSettingsAwareWidget.h"

#include "Accessibility/GothamSettingsSubsystem.h"

void UGothamSettingsAwareWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		SettingsHandle = Settings->OnSettingsChanged.AddUObject(this, &UGothamSettingsAwareWidget::HandleSettingsChanged);
	}
	OnSettingsApplied();
}

void UGothamSettingsAwareWidget::NativeDestruct()
{
	if (UGothamSettingsSubsystem* Settings = UGothamSettingsSubsystem::Get(this))
	{
		Settings->OnSettingsChanged.Remove(SettingsHandle);
	}
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
