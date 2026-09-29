// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ComboComponent.generated.h"

/** Fired on change: (HitCount, Multiplier, DecayAlpha 1 = full window remaining, 0 = combo dropped). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnComboChanged, int32, float, float);

/** Tracks a hit streak that decays if the player stops hitting. Ticks only while a combo is live. */
UCLASS(ClassGroup = (Gotham), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UComboComponent();

	FOnComboChanged OnComboChanged;

	UFUNCTION(BlueprintCallable, Category = "Combo")
	void RegisterHit();

	int32 GetHitCount() const { return HitCount; }
	float GetMultiplier() const;
	float GetDecayAlpha() const { return HitCount > 0 ? TimeRemaining / DecaySeconds : 0.f; }
	void BroadcastCurrent() const { OnComboChanged.Broadcast(HitCount, GetMultiplier(), GetDecayAlpha()); }

	/** Steps the decay timer. Split from TickComponent so it can run on unregistered components in tests. */
	void Advance(float DeltaTime);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** Time the player has to land the next hit before the streak drops. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "0.1"))
	float DecaySeconds = 3.f;

	/** Every N hits adds +1x to the multiplier. */
	UPROPERTY(EditDefaultsOnly, Category = "Combo", meta = (ClampMin = "1"))
	int32 HitsPerMultiplierStep = 5;

	int32 HitCount = 0;
	float TimeRemaining = 0.f;
};
