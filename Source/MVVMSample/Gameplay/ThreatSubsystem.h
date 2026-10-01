// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Gameplay/ThreatTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "ThreatSubsystem.generated.h"

class AMvsThug;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnThreatsUpdated, const TArray<FMvsThreatSnapshot>&);

/**
 * Runs every thug in the world: advances their brains, lets FMvsAttackDirector pick who telegraphs next, resolves
 * strikes and counters, and publishes one snapshot list per frame for the HUD (via the threat view model). Ticks only
 * while thugs exist.
 */
UCLASS()
class MVVMSAMPLE_API UMvsThreatSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Fired every frame while thugs exist, and once with an empty list when the last one goes. */
	FOnThreatsUpdated OnThreatsUpdated;

	/** Nearest warning thug the player can reach gets countered. */
	UPROPERTY(EditAnywhere, Category = "Threats")
	float CounterRange = 650.f;

	void Register(AMvsThug* Thug);
	void Unregister(AMvsThug* Thug);

	/** The player pressed Counter. Returns the countered thug, or null if nobody was in a counterable warning. */
	AMvsThug* TryCounter(APawn* Player);

	/** Off stops new warnings (screenshots, perf baselines). Warnings in flight still play out. */
	void SetDirectorEnabled(bool bEnabled) { bDirectorEnabled = bEnabled; }
	/** Dev aid: makes the thug nearest the camera's view centre start warning now. */
	AMvsThug* ForceWarningOnVisible();
	/** Dev aid: the idle thug most behind the camera starts warning now (shows the edge arrow). */
	AMvsThug* ForceWarningBehind();

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	/** The base class already ticks conditionally (and never for the class default object): tick only with thugs. */
	virtual bool IsTickable() const override { return !Thugs.IsEmpty(); }

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	APawn* GetPlayerPawn() const;
	/** Idle thug with the highest (or lowest) dot product against the camera's view direction. */
	AMvsThug* ForceWarning(bool bMostInView);
	void Publish(const APawn* Player);

	TArray<TWeakObjectPtr<AMvsThug>> Thugs;
	TArray<FMvsThreatSnapshot> Snapshots;
	FMvsAttackDirector Director;
	bool bDirectorEnabled = true;
};
