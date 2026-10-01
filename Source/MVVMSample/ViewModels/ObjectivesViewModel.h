// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "ObjectivesViewModel.generated.h"

/** The current objective ("Find the clues") and how far along it is. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UObjectivesViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetProgress(int32 InFound, int32 InTotal);

	const FText& GetObjectiveTitle() const { return ObjectiveTitle; }
	int32 GetFoundCount() const { return FoundCount; }
	int32 GetTotalCount() const { return TotalCount; }
	float GetProgressPercent() const { return ProgressPercent; }
	bool GetIsComplete() const { return bIsComplete; }
	const FText& GetProgressText() const { return ProgressText; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText ObjectiveTitle;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 FoundCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 TotalCount = 0;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float ProgressPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsComplete, meta = (AllowPrivateAccess = "true"))
	bool bIsComplete = false;

	/** "2 / 5", already localized, so views never format numbers. */
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText ProgressText;
};
