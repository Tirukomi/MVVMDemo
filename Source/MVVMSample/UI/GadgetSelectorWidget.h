// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "GadgetSelectorWidget.generated.h"

class UGadgetBarViewModel;
class UGadgetIcon;
class UHorizontalBox;
class UTextBlock;

/**
 * Top-right gadget readout: the selected gadget (last used) large with its cooldown ring, name and state; the rest of
 * the loadout small underneath with their keys. Rebuilds entries only when the loadout changes.
 */
UCLASS()
class MVVMSAMPLE_API UGadgetSelectorWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UGadgetBarViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void ApplyTheme() override { Refresh(); }

private:
	void RebuildEntries();
	void Refresh();
	void OnBarChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void OnSlotChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	/** Per-frame cooldown path: ring progress and the seconds readout only. */
	void RefreshCooldowns();
	void UnbindSlots();

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetIcon> SelectedIcon;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedName;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SelectedState;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> OthersRow;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UGadgetIcon>> SmallIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SmallKeys;

	int32 LastSelected = INDEX_NONE;
	int32 LastSeconds = -1;
};
