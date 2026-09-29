// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWidget;

/**
 * The motion language: short, sharp, nothing floaty. Runs on real time (unaffected by the gadget wheel's slow-mo),
 * one animation per widget at a time (a new one replaces the old), and does nothing under reduced motion.
 */
namespace GothamMotion
{
	/** Durations and curve shared by every HUD animation. */
	inline constexpr float PopSeconds = 0.14f;
	inline constexpr float FadeSeconds = 0.5f;

	/** Out-back style punch: 0 -> 1 -> 0 over T in [0,1], peaking early. Pure, for tests and custom Slate. */
	MVVMSAMPLE_API float PunchCurve(float T);

	/** Scale punch (e.g. combo counter on a hit). */
	MVVMSAMPLE_API void Pop(UWidget* Widget, float Peak = 1.18f, float Seconds = PopSeconds);

	/** Fades render opacity from From to To. Under reduced motion it jumps to To. */
	MVVMSAMPLE_API void Fade(UWidget* Widget, float From, float To, float Seconds = FadeSeconds);

	/** True if the player asked for reduced motion (defaults to false without a world). */
	MVVMSAMPLE_API bool IsReduced(const UObject* Context);
}
