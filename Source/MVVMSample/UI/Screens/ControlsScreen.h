// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/GothamBindings.h"
#include "UI/Screens/GothamScreen.h"
#include "ControlsScreen.generated.h"

class UControlsViewModel;
class UEnhancedInputUserSettings;
class UGothamButton;
class UTextBlock;

/**
 * Rebinding screen built on Enhanced Input user settings. Pick a slot, press the new key; conflicts swap keys and
 * changes persist immediately. Keyboard/mouse and gamepad each have their own slot per action.
 */
UCLASS()
class MVVMSAMPLE_API UControlsScreen : public UGothamScreen
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	struct FSlotButton
	{
		FName Name;
		int32 Slot = 0;
		TObjectPtr<UGothamButton> Button;
	};

	UEnhancedInputUserSettings* GetUserSettings() const;
	void PullSnapshot();
	void ApplyChanges(const TArray<FGothamBindingChange>& Changes);
	void BeginCapture(FName Name, int32 Slot);
	void EndCapture();
	bool HandleCapturedKey(const FKey& Key);
	void ResetAll();
	void RefreshLabels();
	void OnViewModelChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RefreshLabels(); }

	UPROPERTY(Transient)
	TObjectPtr<UControlsViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> Status;

	TArray<FSlotButton> SlotButtons;
	bool bCapturing = false;
	FName CaptureName;
	int32 CaptureSlot = 0;
};
