// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimeScaleSubsystem.generated.h"

/**
 * The one owner of the world's time dilation. Systems that bend time (the gadget wheel's slow motion, hit-stop) ask
 * for a scale under their own name instead of setting global time dilation themselves; the slowest request wins,
 * and clearing one restores whatever the others still ask for. So a hit-stop that ends while the wheel is open
 * lands back on the wheel's slow motion, not on normal speed.
 */
UCLASS()
class MVVMSAMPLE_API UMvsTimeScaleSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UMvsTimeScaleSubsystem* Get(const UObject* WorldContext);

	/** Asks for Scale until Clear(Source). A second request from the same source replaces the first. */
	void Request(FName Source, float Scale);
	/** Asks for Scale for RealSeconds of real time (not slowed by the scale itself); a new request extends it. */
	void RequestFor(FName Source, float Scale, float RealSeconds);
	void Clear(FName Source);

	/** The scale in effect (1 when nothing asks). */
	float GetScale() const;

	virtual void Deinitialize() override;

private:
	void Apply();
	bool TickTimed(float DeltaTime);

	TMap<FName, float> Requests;
	/** Real-time expiry (FPlatformTime::Seconds) of the timed requests. */
	TMap<FName, double> Expiry;
	FTSTicker::FDelegateHandle TimedHandle;
};
