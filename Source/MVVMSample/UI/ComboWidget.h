// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "UI/GothamSettingsAwareWidget.h"
#include "ComboWidget.generated.h"

class UComboMeter;
class UComboViewModel;
class UGothamPanel;
class UTextBlock;

/**
 * Combo counter under the health bar: big condensed numerals, a "HITS" label, an accent multiplier tag and a thin
 * decay rule. The count pops on every hit and the tag on every multiplier step (both skipped under reduced motion).
 */
UCLASS()
class MVVMSAMPLE_API UComboWidget : public UGothamSettingsAwareWidget
{
	GENERATED_BODY()

public:
	void SetViewModel(UComboViewModel* InViewModel);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void OnSettingsApplied() override { ApplyStyle(); Refresh(); }

private:
	/** Fonts and colours: only on construction and settings changes (setting a font invalidates layout). */
	void ApplyStyle();
	void Refresh();
	void OnFieldChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void OnMilestone(UObject* Source, UE::FieldNotification::FFieldId FieldId);

	static constexpr float MilestoneHoldSeconds = 1.2f;
	FTSTicker::FDelegateHandle MilestoneHandle;

	UPROPERTY(Transient)
	TObjectPtr<UComboViewModel> ViewModel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CountText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HitsLabel;

	UPROPERTY(Transient)
	TObjectPtr<UGothamPanel> MultiplierTag;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MultiplierText;

	UPROPERTY(Transient)
	TObjectPtr<UComboMeter> DecayBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MilestoneText;

	int32 LastHits = 0;
	float LastMultiplier = 1.f;
};
