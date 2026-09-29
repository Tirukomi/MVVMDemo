// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "PauseMenuScreen.generated.h"

class UGothamButton;

/** Pause menu: Resume / Settings / Quit (quit asks for confirmation). Pauses the game while open. */
UCLASS()
class MVVMSAMPLE_API UPauseMenuScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

private:
	void OnResume();
	void OnSettings();
	void OnQuit();
	void OnQuitConfirmed(bool bConfirmed);
};
