// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "DetectiveOverlayWidget.generated.h"

class UDetectiveViewModel;
class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;

/**
 * Full-screen Detective Mode overlay: an animated UI material (scanlines, vignette, wipe) driven by the
 * view model's transition alpha, plus the mode label and scan prompt.
 */
UCLASS()
class MVVMSAMPLE_API UDetectiveOverlayWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UDetectiveViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	/** Fonts, colours, the prompt text and the material's tint and motion: on settings changes and when shown. */
	virtual void ApplyTheme() override;

private:
	/** Per frame of the fade: visibility, opacity and the material's progress, nothing else. */
	void ApplyFade();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Scanlines;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Prompt;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ModeLabel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ScanLabel;
};
