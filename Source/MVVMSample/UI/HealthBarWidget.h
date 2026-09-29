// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "HealthBarWidget.generated.h"

class UComboMeter;
class UPlayerVitalsViewModel;
class UTextBlock;

/**
 * Health, top-left: a slanted segmented bar with a damage ghost, and a small readout underneath. Low health is
 * spelled out ("LOW") as well as coloured, so it never depends on colour alone.
 */
UCLASS()
class MVVMSAMPLE_API UHealthBarWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UPlayerVitalsViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void OnSettingsApplied() override { Refresh(); }

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UComboMeter> Bar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ValueText;
};
