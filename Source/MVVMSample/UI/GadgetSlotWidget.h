// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GadgetSlotWidget.generated.h"

class UBorder;
class UGadgetSlotViewModel;
class UProgressBar;
class UTextBlock;

/** One gadget button: name, key hint, and a cooldown sweep. Presentation only. */
UCLASS()
class MVVMSAMPLE_API UGadgetSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UGadgetSlotViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UGadgetSlotViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> Frame;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> Cooldown;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> KeyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TimeText;
};
