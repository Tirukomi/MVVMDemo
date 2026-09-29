// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "SettingsScreen.generated.h"

class UGothamOptionRow;
class USettingsViewModel;
class UTextBlock;

/**
 * Language and accessibility options. Changes preview live; Apply keeps them, and leaving the screen any other way
 * (Back, Esc, B) discards unapplied changes. Every control is a focusable button, so it is fully gamepad-navigable.
 */
UCLASS()
class MVVMSAMPLE_API USettingsScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeOnDeactivated() override;

private:
	void RefreshDirtyNote();
	void OnViewModelChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshDirtyNote(); }
	void OpenControls();

	UPROPERTY(Transient)
	TObjectPtr<USettingsViewModel> ViewModel;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGothamOptionRow>> Rows;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DirtyNote;
};
