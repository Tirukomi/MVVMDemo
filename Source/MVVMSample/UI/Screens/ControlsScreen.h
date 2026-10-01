// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/GothamBindings.h"
#include "UI/Screens/GothamScreen.h"
#include "ControlsScreen.generated.h"

class UControlsViewModel;
class UGothamButton;
class UTextBlock;

/**
 * Rebinding screen. Pick a slot, press the new key; conflicts swap keys and changes persist immediately. Keyboard/mouse
 * and gamepad each have their own slot per action. The screen captures the key and shows the result; the controls
 * view model validates and applies it (to Enhanced Input's user settings, through a binding store).
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
