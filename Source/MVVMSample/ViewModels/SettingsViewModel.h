// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "MVVMViewModelBase.h"
#include "SettingsViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsDataEvent, const FGothamSettingsData&);

/**
 * Working copy of the settings for the settings screen. Edits take effect live (preview) through OnPreview;
 * Apply commits them, Revert returns to the last committed values. Pure data plus text: no world or widgets.
 */
UCLASS(BlueprintType)
class MVVMSAMPLE_API USettingsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	FOnSettingsDataEvent OnPreview;
	FOnSettingsDataEvent OnCommitted;

	/** Sets both the working copy and the committed baseline (used at start-up). */
	void Initialize(const FGothamSettingsData& Saved);

	/** Steps one option. Previews immediately. */
	void Cycle(EGothamSetting Setting, int32 Direction);

	void Apply();
	void Revert();
	void ResetDefaults();

	/** Re-reads every display string, e.g. after the language changed. Broadcasts even if the text object is identical. */
	void RefreshTexts();

	const FGothamSettingsData& GetCurrent() const { return Current; }
	bool GetIsDirty() const { return bIsDirty; }

	static FText GetLabel(EGothamSetting Setting);
	FText GetValueText(EGothamSetting Setting) const;

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsDirty, meta = (AllowPrivateAccess = "true"))
	bool bIsDirty = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText LanguageValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText ColorVisionValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText UIScaleValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText HighContrastValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText ReducedMotionValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText WheelModeValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText ScanModeValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText SubtitleSizeValue;
	UPROPERTY(BlueprintReadOnly, FieldNotify, meta = (AllowPrivateAccess = "true"))
	FText SubtitleBackgroundValue;

private:
	void Recompute();

	FGothamSettingsData Current;
	FGothamSettingsData Baseline;
};
