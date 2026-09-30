// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Gameplay/ThreatTypes.h"
#include "GothamThug.generated.h"

/**
 * A training-dummy thug. It exists to drive the combat UI: it stands, turns to face the player, and when the attack
 * director picks it, telegraphs an attack (the counter window) and then strikes. A counter knocks it back and stuns
 * it. Drawn as a hostile in Detective Mode (custom-depth stencil 3). No AI controller: UGothamThreatSubsystem runs it.
 */
UCLASS()
class MVVMSAMPLE_API AGothamThug : public ACharacter
{
	GENERATED_BODY()

public:
	AGothamThug();

	/** Damage dealt when a strike lands. */
	UPROPERTY(EditAnywhere, Category = "Thug", meta = (ClampMin = "0"))
	float StrikeDamage = 10.f;

	/** A strike only lands if the player is this close when the warning runs out. */
	UPROPERTY(EditAnywhere, Category = "Thug", meta = (ClampMin = "0"))
	float StrikeRange = 320.f;

	FGothamThugBrain& GetBrain() { return Brain; }
	const FGothamThugBrain& GetBrain() const { return Brain; }

	/** Called by the threat subsystem when the brain reports a strike. */
	void Strike(APawn* Target);
	/** Called by the threat subsystem when the player counters this thug. */
	void OnCountered(const APawn* By);
	/** The point the HUD anchors this thug's prompt to. */
	FVector GetPromptLocation() const;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FGothamThugBrain Brain;
};
