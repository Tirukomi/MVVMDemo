// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsTypes.h"
#include "MVVMViewModelBase.h"
#include "SettingsViewModel.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSettingsDataEvent, const FMvsSettingsData&);

/** One settings tab: an id, its label and the options it holds, in display order. */
struct FMvsSettingsTab
{
	FName Id;
	FText Label;
	TArray<EMvsSetting> Settings;
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
	void Initialize(const FMvsSettingsData& Saved);

	/** Steps one option. Previews immediately. */
	void Cycle(EMvsSetting Setting, int32 Direction);

	void Apply();
	void Revert();
	void ResetDefaults();

	/** Re-reads every display string, e.g. after the language changed. Broadcasts even if the text object is identical. */
	void RefreshTexts();

	const FMvsSettingsData& GetCurrent() const { return Current; }
	bool GetIsDirty() const { return bIsDirty; }
	int32 GetRevision() const { return Revision; }

	static FText GetLabel(EMvsSetting Setting);
	/** One or two sentences for the settings screen's detail pane. */
	static FText GetDescription(EMvsSetting Setting);
	/** How the settings screen groups options. Every setting is in exactly one tab (tested). */
	static const TArray<FMvsSettingsTab>& GetTabs();
	FText GetValueText(EMvsSetting Setting) const;

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


private:
	void Recompute();
	void BumpRevision();

	FMvsSettingsData Current;
	FMvsSettingsData Baseline;
	/** The values the last Revision described. */
	FMvsSettingsData Shown;
	bool bHasShown = false;
};
