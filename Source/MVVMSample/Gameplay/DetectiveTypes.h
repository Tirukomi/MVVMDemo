// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Custom-stencil classes the Detective Mode post-process colours by. */
namespace EGothamStencil
{
	enum : int32
	{
		None = 0,
		ClueUnscanned = 1,
		ClueScanned = 2,
		Hostile = 3,
		Interactable = 4,
	};
}

/**
 * An expanding ring through the world (radius and fading strength). Fired from the player when Detective Mode opens
 * (it also drives the radial reveal) and from a clue when its analysis completes. Pure, so timing is testable.
 */
struct MVVMSAMPLE_API FGothamScanPulse
{
	FVector Center = FVector::ZeroVector;
	float Radius = 0.f;
	float MaxRadius = 0.f;
	float Strength = 0.f;
	bool bActive = false;

	static constexpr float Speed = 3200.f;   // cm per second

	void Fire(const FVector& InCenter, float InMaxRadius);
	/** Returns true while still expanding. Strength fades from 1 to 0 across the pulse's life. */
	bool Advance(float DeltaTime);
	void Stop() { bActive = false; Strength = 0.f; }
};

/**
 * Hold-to-analyse progress. Begin starts it, Cancel (releasing early) abandons it, Advance reports completion once.
 * Pure: the component supplies the target and what "complete" means.
 */
struct MVVMSAMPLE_API FGothamAnalysis
{
	float Progress = 0.f;
	bool bRunning = false;

	void Begin() { Progress = 0.f; bRunning = true; }
	void Cancel() { Progress = 0.f; bRunning = false; }
	/** Returns true exactly once, on the frame the analysis completes. */
	bool Advance(float DeltaTime, float DurationSeconds);
};
