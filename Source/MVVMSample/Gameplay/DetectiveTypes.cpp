// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/DetectiveTypes.h"

void FGothamScanPulse::Fire(const FVector& InCenter, float InMaxRadius)
{
	Center = InCenter;
	Radius = 0.f;
	MaxRadius = FMath::Max(1.f, InMaxRadius);
	Strength = 1.f;
	bActive = true;
}

bool FGothamScanPulse::Advance(float DeltaTime)
{
	if (!bActive)
	{
		return false;
	}
	Radius = FMath::Min(MaxRadius, Radius + Speed * DeltaTime);
	// Hold full strength for the first half, then fade so the ring dissolves rather than stopping at a hard edge.
	const float Life = Radius / MaxRadius;
	Strength = Life < 0.5f ? 1.f : FMath::Clamp(1.f - (Life - 0.5f) * 2.f, 0.f, 1.f);
	if (Radius >= MaxRadius)
	{
		Stop();
		return false;
	}
	return true;
}

bool FGothamAnalysis::Advance(float DeltaTime, float DurationSeconds)
{
	if (!bRunning)
	{
		return false;
	}
	Progress = FMath::Min(1.f, Progress + DeltaTime / FMath::Max(DurationSeconds, KINDA_SMALL_NUMBER));
	if (Progress >= 1.f)
	{
		bRunning = false;
		return true;
	}
	return false;
}
