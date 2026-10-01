// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWidget;

/**
 * The motion language: short, sharp, nothing floaty. Runs on real time (unaffected by the gadget wheel's slow-mo),
 * one animation per widget at a time (a new one replaces the old), and does nothing under reduced motion.
 */
namespace MvsMotion
{
	/** Durations and curve shared by every HUD animation. */
	inline constexpr float PopSeconds = 0.14f;
	inline constexpr float FadeSeconds = 0.5f;
	/** Screen intro/outro: short enough to never feel like waiting. */
	inline constexpr float ScreenSeconds = 0.15f;
	/** How far a screen's content slides in from. */
	inline constexpr float ScreenSlide = 36.f;

	/** Out-back style punch: 0 -> 1 -> 0 over T in [0,1], peaking early. Pure, for tests and custom Slate. */
	MVVMSAMPLE_API float PunchCurve(float T);

	/** Scale punch (e.g. combo counter on a hit). */
	MVVMSAMPLE_API void Pop(UWidget* Widget, float Peak = 1.18f, float Seconds = PopSeconds);

	/** Fades render opacity from From to To. Under reduced motion it jumps to To. */
	MVVMSAMPLE_API void Fade(UWidget* Widget, float From, float To, float Seconds = FadeSeconds);

	/** Slides a widget from an offset back to its layout position (a screen intro). Under reduced motion it does nothing. */
	MVVMSAMPLE_API void SlideIn(UWidget* Widget, const FVector2D& From, float Seconds = ScreenSeconds);

	/** True if the player asked for reduced motion (defaults to false without a world). */
	MVVMSAMPLE_API bool IsReduced(const UObject* Context);
}

/**
 * A rectangle easing toward a target, for the menu highlight bar. Exponential approach (fast start, no overshoot),
 * snaps once within half a pixel. The first target, and every target under reduced motion, is taken immediately so
 * the bar never flies in from the corner. Pure, for tests.
 */
struct MVVMSAMPLE_API FMvsSlideRect
{
	/** 1/s. At 30 the bar covers 95% of the way in 0.1 s. */
	float Rate = 30.f;

	FVector2f Position = FVector2f::ZeroVector;
	FVector2f Size = FVector2f::ZeroVector;

	void SetTarget(const FVector2f& InPosition, const FVector2f& InSize, bool bSnap);
	/** Returns true while still moving. */
	bool Advance(float DeltaSeconds);
	bool HasValue() const { return bHasValue; }
	bool IsMoving() const { return bMoving; }

private:
	FVector2f TargetPosition = FVector2f::ZeroVector;
	FVector2f TargetSize = FVector2f::ZeroVector;
	bool bHasValue = false;
	bool bMoving = false;
};
