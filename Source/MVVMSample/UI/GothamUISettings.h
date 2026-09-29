// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GothamUISettings.generated.h"

class UCommonActivatableWidget;
class UMaterialInterface;
class UUserWidget;

/** Project UI configuration (Project Settings > Game > Gotham UI). Lets content swap the HUD without code. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Gotham UI"))
class MVVMSAMPLE_API UGothamUISettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UGothamUISettings();

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> HudScreenClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> PauseMenuClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> SettingsScreenClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> GadgetWheelClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> ClueLogClass;

	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UCommonActivatableWidget> ControlsScreenClass;

	/**
	 * Row widget for the clue log. The list view requires a Blueprint subclass of UClueEntryWidget in the editor,
	 * which also gives designers an asset to restyle. Falls back to the native class (fine in cooked builds).
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Screens")
	TSoftClassPtr<UUserWidget> ClueEntryClass;

	/** Post-process material for Detective Mode (domain: Post Process). Created by Scripts/CreateDetectiveAssets.py. */
	UPROPERTY(Config, EditAnywhere, Category = "Detective")
	TSoftObjectPtr<UMaterialInterface> DetectiveVisionMaterial;

	/** Full-screen UI material for the Detective Mode overlay (domain: User Interface). */
	UPROPERTY(Config, EditAnywhere, Category = "Detective")
	TSoftObjectPtr<UMaterialInterface> DetectiveOverlayMaterial;
};
