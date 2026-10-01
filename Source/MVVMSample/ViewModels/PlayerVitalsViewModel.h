// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "PlayerVitalsViewModel.generated.h"

/** Presentation state for the player's health. Pure data: no world, widget or component references. */
UCLASS(BlueprintType)
class MVVMSAMPLE_API UPlayerVitalsViewModel : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	/** Below this fraction the HUD shows a low-health warning. */
	static constexpr float LowHealthThreshold = 0.25f;

	/** The single entry point gameplay uses; derives percent and the low-health flag. */
	void SetVitals(float InHealth, float InMaxHealth);

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHealthPercent() const { return HealthPercent; }
	bool GetIsLowHealth() const { return bIsLowHealth; }
	/** Increments on every drop in health, so views can flash without keeping their own history. */
	int32 GetDamageCount() const { return DamageCount; }

protected:
	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float Health = 100.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float MaxHealth = 100.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	float HealthPercent = 1.f;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter=GetIsLowHealth, meta = (AllowPrivateAccess = "true"))
	bool bIsLowHealth = false;

	UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess = "true"))
	int32 DamageCount = 0;

private:
	bool bHasValue = false;
};
