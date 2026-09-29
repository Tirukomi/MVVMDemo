// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "SubtitleViewModel.generated.h"

/** The current subtitle line and how to present it (size and backing panel come from accessibility settings). */
UCLASS(BlueprintType)
class MVVMSAMPLE_API USubtitleViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void SetLine(const FText& InSpeaker, const FText& InText);
	void Clear();
	void SetPresentation(int32 InFontSize, bool bInBackground);

	const FText& GetSpeaker() const { return Speaker; }
	const FText& GetLine() const { return Line; }
	bool GetIsVisible() const { return bIsVisible; }
	int32 GetFontSize() const { return FontSize; }
	bool GetHasBackground() const { return bHasBackground; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText Speaker;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	FText Line;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsVisible, meta = (AllowPrivateAccess = "true"))
	bool bIsVisible = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 FontSize = 22;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetHasBackground, meta = (AllowPrivateAccess = "true"))
	bool bHasBackground = true;
};
