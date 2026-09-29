// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ObjectiveTrackerWidget.generated.h"

class UObjectivesViewModel;
class UProgressBar;
class UTextBlock;

/** Small HUD panel: current objective, "2 / 5", and a thin progress bar. */
UCLASS()
class MVVMSAMPLE_API UObjectiveTrackerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UObjectivesViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UObjectivesViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> Bar;
};
