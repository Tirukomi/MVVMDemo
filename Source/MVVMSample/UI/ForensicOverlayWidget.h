// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/MvsSettingsAwareWidget.h"
#include "ForensicOverlayWidget.generated.h"

class UForensicViewModel;
class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;

/**
 * Full-screen Forensic Mode overlay: an animated UI material (scanlines, vignette, wipe) driven by the
 * view model's transition alpha, plus the mode label and scan prompt.
 */
UCLASS()
class MVVMSAMPLE_API UForensicOverlayWidget : public UMvsSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UForensicViewModel* InViewModel);

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
	TObjectPtr<UForensicViewModel> ViewModel;

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
