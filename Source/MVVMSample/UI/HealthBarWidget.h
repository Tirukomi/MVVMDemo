// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "HealthBarWidget.generated.h"

class UPlayerVitalsViewModel;
class UProgressBar;
class UTextBlock;

/** Health bar + numeric readout. Presentation only: everything it shows comes from the view model. */
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
	TObjectPtr<UProgressBar> Bar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Label;
};
