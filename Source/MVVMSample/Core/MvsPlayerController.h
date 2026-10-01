// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Input/MvsActionSource.h"
#include "MvsPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * Owns input setup (Enhanced Input assets are built in code so the project needs no binary input assets)
 * and creates the HUD. It routes intent to the pawn; it never touches view models directly.
 */
UCLASS()
class MVVMSAMPLE_API AMvsPlayerController : public APlayerController, public IMvsActionSource
{
	GENERATED_BODY()

public:
	/** IMvsActionSource: a gameplay action by name ("Attack", "Gadget1", "Pause"...) for glyphs and screen keys. */
	virtual const UInputAction* FindAction(FName Name) const override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;

private:
	/** Characterization tests build the input assets without a live player (Tests/CharacterizationTests.cpp). */
	friend struct FMvsInputTestAccess;

	void BuildInputAssets();
	void RegisterRebindableContext();
	void BindViewModelsToPawn();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnMoveDirection(FVector2D Direction);
	void OnAttack();
	void OnCounter();
	void OnGadget(int32 SlotIndex);
#if !UE_BUILD_SHIPPING
	void OnDebugDamage();
	void OnDebugHeal();
#endif
	void OnPause();
	void OnGadgetWheel();
	void OnForensic();
	void OnScan();
	void OnScanReleased();
	void OnClueLog();

	/** Always on. While a menu is open, Common UI's Menu input mode keeps its keys from reaching the game. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> GameplayContext;

	/** The menu keys (accept, back, tabs), always on above gameplay (UMvsUIInputData). */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> MenuContext;

	/** Every gameplay action by table name (Input/MvsActionTable). */
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UInputAction>> Actions;

};
