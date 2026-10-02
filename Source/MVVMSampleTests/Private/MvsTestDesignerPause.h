// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/PauseMenuScreen.h"
#include "MvsTestDesignerPause.generated.h"

class UMvsText;
class UVerticalBox;

/**
 * Stands in for a designer's WBP_PauseMenu in the functional tests: its widget tree has a root before the screen builds
 * itself, as a Widget Blueprint's does, made of the same widgets a designer would place (a vertical box of UMvsText
 * with the designer's Style and Color settings). Checks the C++ half of the designer path without the asset.
 */
UCLASS(NotBlueprintable, HideDropdown)
class UMvsTestDesignerPause : public UPauseMenuScreen
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> DesignerRoot;

	UPROPERTY(Transient)
	TObjectPtr<UMvsText> DesignerLabel;
};
