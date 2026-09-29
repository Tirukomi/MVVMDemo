// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "DetectiveViewModel.generated.h"

/** Detective Mode presentation state: on/off and the 0..1 transition alpha every effect keys off. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UDetectiveViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetState(bool bInActive, float InAlpha);

	bool GetIsActive() const { return bIsActive; }
	float GetAlpha() const { return Alpha; }
	/** True while anything of the effect should be on screen (active or still fading out). */
	bool GetIsVisible() const { return bIsVisible; }

	/** Hold-to-analyse state: which clue is being analysed (NAME_None for none) and how far along. */
	void SetAnalysis(FName InClueId, float InProgress);
	FName GetAnalysisTargetId() const { return AnalysisTargetId; }
	float GetAnalysisProgress() const { return AnalysisProgress; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsActive, meta = (AllowPrivateAccess = "true"))
	bool bIsActive = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float Alpha = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsVisible, meta = (AllowPrivateAccess = "true"))
	bool bIsVisible = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FName AnalysisTargetId;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float AnalysisProgress = 0.f;
};
