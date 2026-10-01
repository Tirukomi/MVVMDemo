// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MvsGameMode.generated.h"

UCLASS()
class MVVMSAMPLE_API AMvsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMvsGameMode();

	/**
	 * Where the training thugs stand. Each entry lists fallbacks: the first spot whose floor is the bare roof (not a
	 * prop the level script happened to put there) is used. Z is found with a trace.
	 */
	UPROPERTY(EditAnywhere, Category = "Thugs")
	TArray<FVector2D> ThugSpots;

	virtual void StartPlay() override;

private:
	void SpawnThugs();
};
