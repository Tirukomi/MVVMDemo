// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/Layout/GothamUITypes.h"
#include "GothamPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Owns input setup (Enhanced Input assets are built in code so the project needs no binary input assets)
 * and creates the HUD. It routes intent to the pawn; it never touches view models directly.
 */
UCLASS()
class MVVMSAMPLE_API AGothamPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Looks up a gameplay action by name ("Attack", "Gadget1", "Pause"...) for glyphs and rebinding UIs. */
	const UInputAction* FindAction(FName Name) const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

private:
	void BuildInputAssets();
	void RegisterRebindableContext();
	void BindViewModelsToPawn();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnMoveDirection(FVector2D Direction);
	void OnAttack();
	void OnCounter();
	void OnGadget(int32 SlotIndex);
	void OnDebugDamage();
	void OnDebugHeal();
	void OnPause();
	void OnGadgetWheel();
	void OnDetective();
	void OnScan();
	void OnScanReleased();
	void OnClueLog();
	void RunDevAids(class UGothamUISubsystem* UI);
	void ApplyInputContext(EGothamInputContext Context);

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GameplayContext;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> MoveDirectionActions;
	TArray<FVector2D> MoveDirections;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> CounterAction;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputAction>> GadgetActions;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugDamageAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DebugHealAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> PauseAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> GadgetWheelAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> DetectiveAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ScanAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ClueLogAction;

};
