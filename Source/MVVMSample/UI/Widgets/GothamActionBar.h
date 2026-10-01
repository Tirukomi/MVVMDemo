// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionBar.h"
#include "GothamActionBar.generated.h"

/**
 * A screen's prompt row ("[Enter] Select   [Esc] Back"): Common UI's bound action bar, so the prompts are exactly the
 * actions the screen has registered, with the keys of the current device, and each prompt is a clickable
 * UGothamHintButton that runs its action.
 *
 * Common UI's bar shows the actions of whatever screen is active. A screen stays visible under a modal, so this bar
 * keeps only the prompts that belong to its own screen: under a modal it shows nothing rather than the modal's keys.
 */
UCLASS()
class MVVMSAMPLE_API UGothamActionBar : public UCommonBoundActionBar
{
	GENERATED_BODY()

public:
	UGothamActionBar(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeOnActionButtonCreated(ICommonBoundActionButtonInterface* ActionButton, const FUIActionBindingHandle& RepresentedAction) override;
};
