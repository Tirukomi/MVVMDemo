// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ComboViewModel.generated.h"

/** Presentation state for the combo counter. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UComboViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetCombo(int32 InHitCount, float InMultiplier, float InDecayAlpha);

	int32 GetHitCount() const { return HitCount; }
	float GetMultiplier() const { return Multiplier; }
	float GetDecayAlpha() const { return DecayAlpha; }
	bool GetIsActive() const { return bIsActive; }
	FText GetMultiplierText() const { return MultiplierText; }
	int32 GetMilestoneCount() const { return MilestoneCount; }
	FText GetMilestoneText() const { return MilestoneText; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 HitCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float Multiplier = 1.f;

	/** 1 = full decay window remaining, 0 = combo about to drop. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float DecayAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsActive, meta = (AllowPrivateAccess = "true"))
	bool bIsActive = false;

	/** Pre-formatted "x3" so views never format numbers themselves (localization-safe). */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText MultiplierText;

	/** Bumps each time the streak crosses 10, 20, 30...: views show a callout when it changes. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 MilestoneCount = 0;

	/** "10-hit combo", set before MilestoneCount changes. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText MilestoneText;
};
