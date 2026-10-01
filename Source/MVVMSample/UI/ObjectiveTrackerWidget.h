// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "ObjectiveTrackerWidget.generated.h"

class UComboMeter;
class UGothamPanel;
class UObjectivesViewModel;
class UTextBlock;

/** Objective panel: a muted "OBJECTIVE" label, the objective, "2 / 5" and a segmented progress rule. */
UCLASS()
class MVVMSAMPLE_API UObjectiveTrackerWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UObjectivesViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void ApplyTheme() override { Refresh(); }

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> Panel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UComboMeter> Bar;

	int32 LastFound = -1;
};
