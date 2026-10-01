// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gameplay/ThreatTypes.h"
#include "MvsCharacter.generated.h"

class UCameraComponent;
class UComboComponent;
class UForensicComponent;
class UForensicVisionComponent;
class UGadgetComponent;
class UHealthComponent;
class UPointLightComponent;
class USpringArmComponent;

/** Third-person hero (engine mannequin). Gameplay is intentionally thin: it exists to drive the UI. */
UCLASS()
class MVVMSAMPLE_API AMvsCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMvsCharacter();

	UHealthComponent* GetHealthComponent() const { return Health; }
	UGadgetComponent* GetGadgetComponent() const { return Gadgets; }
	UComboComponent* GetComboComponent() const { return Combo; }
	UForensicComponent* GetForensicComponent() const { return Forensic; }

	void MoveInput(const FVector2D& Axis);

protected:
	virtual void BeginPlay() override;

public:
	void LookInput(const FVector2D& Axis);

	/** Fake melee: registers a combo hit. */
	void Attack();

	void UseGadget(int32 SlotIndex);
	void ToggleForensic();
	void ScanClue();

#if !UE_BUILD_SHIPPING
	// Dev shortcuts (F1 damage, F2 heal), not in Shipping.
	void DebugDamage();
	void DebugHeal();
#endif

	/** Counters the nearest thug that is telegraphing an attack. Returns true if one was countered. */
	bool Counter();

	/** Shakes the camera: adds trauma that drains over time (skipped under reduced motion). */
	void AddCameraTrauma(float Amount);

	virtual void Tick(float DeltaSeconds) override;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Faint cold back light so the dark suit keeps a readable silhouette at night. Casts no shadows. */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPointLightComponent> RimLight;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGadgetComponent> Gadgets;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UComboComponent> Combo;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UForensicComponent> Forensic;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UForensicVisionComponent> ForensicVision;

	FMvsTrauma Trauma;
	float ShakeTime = 0.f;
	bool bShaking = false;
};
