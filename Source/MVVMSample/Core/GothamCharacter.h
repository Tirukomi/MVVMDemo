// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GothamCharacter.generated.h"

class UCameraComponent;
class UComboComponent;
class UDetectiveComponent;
class UDetectiveVisionComponent;
class UGadgetComponent;
class UHealthComponent;
class USpringArmComponent;
class UStaticMeshComponent;

/** Placeholder third-person hero. Gameplay is intentionally thin: it exists to drive the UI. */
UCLASS()
class MVVMSAMPLE_API AGothamCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AGothamCharacter();

	UHealthComponent* GetHealthComponent() const { return Health; }
	UGadgetComponent* GetGadgetComponent() const { return Gadgets; }
	UComboComponent* GetComboComponent() const { return Combo; }
	UDetectiveComponent* GetDetectiveComponent() const { return Detective; }

	void MoveInput(const FVector2D& Axis);
	void LookInput(const FVector2D& Axis);

	/** Fake melee: registers a combo hit. */
	void Attack();

	void UseGadget(int32 SlotIndex);
	void ToggleDetective();
	void ScanClue();

	// Debug helpers bound to F1-F3.
	void DebugDamage();
	void DebugHeal();

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> Body;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UHealthComponent> Health;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UGadgetComponent> Gadgets;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UComboComponent> Combo;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDetectiveComponent> Detective;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDetectiveVisionComponent> DetectiveVision;
};
