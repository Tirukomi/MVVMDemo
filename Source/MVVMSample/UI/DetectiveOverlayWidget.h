// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
class MVVMSAMPLE_API UDetectiveOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UDetectiveViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;

private:
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { Refresh(); }

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> Scanlines;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Material;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> Prompt;
};
