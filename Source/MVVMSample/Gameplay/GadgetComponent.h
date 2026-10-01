// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GadgetComponent.generated.h"

USTRUCT(BlueprintType)
struct FGothamGadgetDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.1"))
	float CooldownSeconds = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FLinearColor Tint = FLinearColor::White;

	/** Which line-art icon the HUD draws: 0 wing-blade, 1 grapple, 2 smoke. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "2"))
	int32 IconIndex = 0;
};

/** Fired when a gadget's cooldown state changes: (SlotIndex, RemainingSeconds, TotalSeconds). */
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGadgetCooldownChanged, int32, float, float);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGadgetUsed, int32);

/** Owns the gadget loadout and cooldown timers. Ticks only while something is cooling down. */
UCLASS(ClassGroup = (Gotham), meta = (BlueprintSpawnableComponent))
class MVVMSAMPLE_API UGadgetComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGadgetComponent();

	FOnGadgetCooldownChanged OnCooldownChanged;
	FOnGadgetUsed OnGadgetUsed;

	UFUNCTION(BlueprintCallable, Category = "Gadgets")
	bool UseGadget(int32 SlotIndex);

	const TArray<FGothamGadgetDefinition>& GetGadgets() const { return Gadgets; }
	float GetCooldownRemaining(int32 SlotIndex) const;
	void BroadcastAll() const;

	/** Replaces the loadout (used by tests). */
	void SetGadgets(const TArray<FGothamGadgetDefinition>& NewGadgets);

	/** Steps cooldown timers. Split from TickComponent so it can run on unregistered components in tests. */
	void Advance(float DeltaTime);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(EditAnywhere, Category = "Gadgets")
	TArray<FGothamGadgetDefinition> Gadgets;

	TArray<float> Remaining;
};
