// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DetectiveComponent.generated.h"

class AClueActor;
class UClueDataAsset;

/** (bActive, TransitionAlpha 0..1) whenever detective mode or its transition changes. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDetectiveChanged, bool, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnClueScanned, const UClueDataAsset*);

/**
 * Detective Mode state: on/off, an eased transition alpha that drives every visual effect, and scanning of
 * nearby clues. Ticks only while the transition is running.
 */
UCLASS(ClassGroup = (Gotham), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UDetectiveComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDetectiveComponent();

	FOnDetectiveChanged OnDetectiveChanged;
	FOnClueScanned OnClueScanned;
	/** Fired once the level's clues have been gathered (BeginPlay), so late observers can rebuild. */
	FSimpleMulticastDelegate OnCluesCollected;

	UFUNCTION(BlueprintCallable, Category = "Detective")
	void ToggleDetective();

	/** Scans the nearest unscanned clue in range. Only works while detective mode is on. */
	UFUNCTION(BlueprintCallable, Category = "Detective")
	bool TryScan();

	bool IsActive() const { return bActive; }

	/** Reduced motion makes the transition near-instant. */
	void SetReducedMotion(bool bInReduced) { bReducedMotion = bInReduced; }
	float GetAlpha() const { return Alpha; }

	/** Every clue definition in the level, in placement order. */
	const TArray<const UClueDataAsset*>& GetAllClues() const { return AllClues; }
	int32 GetScannedCount() const { return ScannedIds.Num(); }
	bool IsScanned(FName ClueId) const { return ScannedIds.Contains(ClueId); }

	/** Steps the transition. Split from TickComponent so tests can run it on unregistered components. */
	void Advance(float DeltaTime);

	/** Records a clue as scanned (used by TryScan and by tests). */
	void RegisterScan(const UClueDataAsset* Clue);

	/** Setup hook for tests: supplies the clue list without a world. */
	void SetClueDefinitions(const TArray<const UClueDataAsset*>& Clues) { AllClues = Clues; }

	void BroadcastCurrent() const { OnDetectiveChanged.Broadcast(bActive, Alpha); }

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	void ApplyHighlights(bool bEnabled);

	/** Seconds to fully enter or leave detective mode. */
	UPROPERTY(EditDefaultsOnly, Category = "Detective", meta = (ClampMin = "0.05"))
	float TransitionSeconds = 0.45f;

	/** Clues farther than this cannot be scanned. */
	UPROPERTY(EditDefaultsOnly, Category = "Detective", meta = (ClampMin = "0"))
	float ScanRadius = 600.f;

	bool bReducedMotion = false;
	bool bActive = false;
	float Alpha = 0.f;

	TArray<const UClueDataAsset*> AllClues;
	TSet<FName> ScannedIds;
	TArray<TWeakObjectPtr<AClueActor>> ClueActors;
};
