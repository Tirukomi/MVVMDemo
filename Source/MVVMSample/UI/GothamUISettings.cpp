// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamUISettings.h"

#include "UI/GothamHudWidget.h"
#include "UI/Screens/ClueLogScreen.h"
#include "UI/Screens/ControlsScreen.h"
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
	ClueLogClass = UClueLogScreen::StaticClass();
	ControlsScreenClass = UControlsScreen::StaticClass();
	ClueEntryClass = FSoftClassPath(TEXT("/Game/UI/WBP_ClueEntry.WBP_ClueEntry_C"));
	DetectiveVisionMaterial = FSoftObjectPath(TEXT("/Game/Materials/M_DetectiveVision.M_DetectiveVision"));
	DetectiveOverlayMaterial = FSoftObjectPath(TEXT("/Game/Materials/M_DetectiveOverlay_UI.M_DetectiveOverlay_UI"));
}
