// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ComboWidget.generated.h"

class UComboViewModel;
class UComboMeter;
class UTextBlock;

/** Combo counter with a decay timer bar. Hidden while no combo is active. */
UCLASS()
class MVVMSAMPLE_API UComboWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UComboViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HitsText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MultiplierText;

	UPROPERTY(Transient)
	TObjectPtr<UComboMeter> DecayBar;
};
