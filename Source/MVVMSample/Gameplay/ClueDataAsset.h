// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClueDataAsset.generated.h"

class UTexture2D;

/** One investigable clue. Authored as a data asset so designers add clues without touching code. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UClueDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Stable identifier used to match world actors, saved progress and UI entries. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clue")
	FName ClueId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clue")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clue", meta = (MultiLine = "true"))
	FText Description;

	/** Soft so the clue log can stream thumbnails in on demand instead of loading them all up front. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Clue")
	TSoftObjectPtr<UTexture2D> Thumbnail;
};
