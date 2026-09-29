// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamUISettings.h"

#include "UI/GothamHudWidget.h"

UGothamUISettings::UGothamUISettings()
{
	CategoryName = TEXT("Game");
	HudWidgetClass = UGothamHudWidget::StaticClass();
}
