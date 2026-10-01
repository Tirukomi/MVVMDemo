// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
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
	/** Characterization tests build the input assets without a live player (Tests/CharacterizationTests.cpp). */
	friend struct FGothamInputTestAccess;

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

	/** Always on. While a menu is open, Common UI's Menu input mode keeps its keys from reaching the game. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GameplayContext;

	/** The menu keys (accept, back, tabs), always on above gameplay (UGothamUIInputData). */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MenuContext;

	/** Every gameplay action by table name (Input/GothamActionTable). */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInputAction>> Actions;

};
