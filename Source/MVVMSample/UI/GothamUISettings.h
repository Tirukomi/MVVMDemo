// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GothamUISettings.generated.h"

class UCommonActivatableWidget;

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
};
