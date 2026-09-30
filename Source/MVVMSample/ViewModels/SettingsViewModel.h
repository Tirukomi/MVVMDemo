// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsTypes.h"
#include "MVVMViewModelBase.h"
#include "SettingsViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsDataEvent, const FGothamSettingsData&);

/** One settings tab: an id, its label and the options it holds, in display order. */
struct FGothamSettingsTab
{
	FName Id;
	FText Label;
	TArray<EGothamSetting> Settings;
};

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
	int32 GetRevision() const { return Revision; }

	static FText GetLabel(EGothamSetting Setting);
	/** One or two sentences for the settings screen's detail pane. */
	static FText GetDescription(EGothamSetting Setting);
	/** How the settings screen groups options. Every setting is in exactly one tab (tested). */
	static const TArray<FGothamSettingsTab>& GetTabs();
	FText GetValueText(EGothamSetting Setting) const;

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsDirty, meta = (AllowPrivateAccess = "true"))
	bool bIsDirty = false;

	/**
	 * Bumps whenever any displayed value may have changed: a value stepped, reverted or reset, or the language
	 * changed (which re-texts everything). Views subscribe to this one field and re-read what they show
	 * (GetValueText, GetLabel, GetCurrent).
	 */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetRevision, meta = (AllowPrivateAccess = "true"))
	int32 Revision = 0;

	// Deprecated: one text per setting, superseded by Revision + GetValueText. Kept for one pass, then removed.
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
	void BumpRevision();

	FGothamSettingsData Current;
	FGothamSettingsData Baseline;
	/** The values the last Revision described. */
	FGothamSettingsData Shown;
	bool bHasShown = false;
};
