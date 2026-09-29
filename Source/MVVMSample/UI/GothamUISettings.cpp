// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamUISettings.h"

#include "UI/GothamHudWidget.h"
#include "UI/Screens/GadgetWheelScreen.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "UI/Screens/SettingsScreen.h"

UGothamUISettings::UGothamUISettings()
{
	CategoryName = TEXT("Game");
	HudScreenClass = UGothamHudWidget::StaticClass();
	PauseMenuClass = UPauseMenuScreen::StaticClass();
	SettingsScreenClass = USettingsScreen::StaticClass();
	GadgetWheelClass = UGadgetWheelScreen::StaticClass();
}
