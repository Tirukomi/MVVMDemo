// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float /*Health*/, float /*MaxHealth*/);

/** Owns an actor's health. Knows nothing about UI; observers subscribe to OnHealthChanged. */
UCLASS(ClassGroup = (Gotham), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	FOnHealthChanged OnHealthChanged;

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Re-broadcasts current state so late-bound observers get an initial value. */
	void BroadcastCurrent() const { OnHealthChanged.Broadcast(Health, MaxHealth); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Health", meta = (ClampMin = "1"))
	float MaxHealth = 100.f;

	float Health = 100.f;

	void SetHealth(float NewHealth);
};
