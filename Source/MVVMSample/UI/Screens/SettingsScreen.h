// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "SettingsScreen.generated.h"

/** Placeholder until M5 (rebinding, accessibility, language). Exists so the navigation flow is complete. */
UCLASS()
class MVVMSAMPLE_API USettingsScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
