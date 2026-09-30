// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Combat "juice" shared by the hero and the thugs. Everything here does nothing under reduced motion. */
namespace GothamFeel
{
	/** A few frames of near-freeze on impact. Real time, so it lasts the same at any frame rate. */
	inline constexpr float HitStopSeconds = 0.07f;
	inline constexpr float HitStopDilation = 0.03f;

	/**
	 * Freezes the world briefly through UGothamTimeScaleSubsystem, then gives time back to whatever else asks for it
	 * (the gadget wheel's slow motion, say). Skipped under reduced motion. A second hit-stop extends the first.
	 */
	MVVMSAMPLE_API void HitStop(const UObject* WorldContext, float Seconds);
}
