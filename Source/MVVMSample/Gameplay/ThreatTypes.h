// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

/** What a thug is doing, as the UI and the attack director see it. */
enum class EMvsThugState : uint8
{
	Idle,
	/** Telegraphing an attack: the counter window. */
	Warning,
	/** Countered: knocked back and harmless for a while. */
	Stunned,
	/** Just struck (or whiffed); catching its breath before it can be picked again. */
	Recover,
};

/** What the HUD needs about one threat for one frame. */
struct FMvsThreatSnapshot
{
	/** Above the head: where the counter prompt anchors. */
	FVector PromptLocation = FVector::ZeroVector;
	EMvsThugState State = EMvsThugState::Idle;
	float WarningProgress = 0.f;
	float Distance = 0.f;
};

/** Something that happened during FMvsThugBrain::Advance. */
enum class EMvsThugEvent : uint8
{
	None,
	/** The warning ran out uncountered: the thug strikes now. */
	Strike,
	/** Back to idle (after stun or recovery). */
	Ready,
};

/**
 * One thug's attack cycle. Pure (no actor, no world): Idle -> Warning -> Strike -> Recover -> Idle, or
 * Warning -> Counter -> Stunned -> Idle. The attack director decides when a warning starts.
 */
struct MVVMSAMPLE_API FMvsThugBrain
{
	static constexpr float WarningSeconds = 1.1f;
	static constexpr float StunSeconds = 2.2f;
	static constexpr float RecoverSeconds = 0.8f;

	EMvsThugState State = EMvsThugState::Idle;
	float Elapsed = 0.f;

	/** Starts a telegraph. Only from Idle; returns false otherwise. */
	bool BeginWarning();
	/** The player countered. Only lands during Warning; returns false otherwise. */
	bool Counter();
	EMvsThugEvent Advance(float DeltaSeconds);

	/** 0 at the start of a warning, 1 at the moment it strikes. 0 outside Warning. */
	float GetWarningProgress() const;
	bool IsWarning() const { return State == EMvsThugState::Warning; }
};

/**
 * Picks who attacks next, the way brawler combat usually reads: one telegraph at a time, a short breather between them, only thugs that are
 * close enough to reach the player. Pure; deterministic with a seed so the rules are testable.
 */
struct MVVMSAMPLE_API FMvsAttackDirector
{
	float MinGapSeconds = 1.6f;
	float MaxGapSeconds = 3.0f;
	/** Thugs further than this from the player never start a warning. */
	float EngageRange = 1100.f;

	explicit FMvsAttackDirector(int32 Seed = 0x7a11) : Random(Seed) { Cooldown = MinGapSeconds; }

	/**
	 * Advances the breather. Returns the index (into States / Distances) of the thug that should start warning now,
	 * or INDEX_NONE. Never picks while any thug is already warning.
	 */
	int32 Advance(float DeltaSeconds, const TArray<EMvsThugState>& States, const TArray<float>& Distances);

	float GetCooldown() const { return Cooldown; }

private:
	FRandomStream Random;
	float Cooldown = 0.f;
};

/** Screen-space rules for threat indicators. Pure, for tests. */
namespace MvsThreat
{
	/**
	 * Where an off-screen threat's arrow sits on the screen edge, and which way it points.
	 * @param ViewDirection  the threat relative to the camera, in camera axes: X forward, Y right, Z up
	 * @param Viewport       viewport size in the layer's units
	 * @param Inset          distance of the arrow from the screen edge
	 * @param OutPosition    arrow centre
	 * @param OutAngle       radians, 0 = pointing right, increasing clockwise (screen Y is down)
	 * A threat straight behind the camera (no sideways component) points down, toward "behind you".
	 */
	MVVMSAMPLE_API void EdgeArrow(const FVector& ViewDirection, const FVector2D& Viewport, float Inset, FVector2D& OutPosition, float& OutAngle);

	/** True if a projected point is inside the viewport minus a margin (the prompt is drawn there, not an arrow). */
	MVVMSAMPLE_API bool IsOnScreen(const FVector2D& Screen, const FVector2D& Viewport, float Margin);
}

namespace MvsCombo
{
	/** The milestone crossed going from Previous to Current hits (10, 20, ...), or 0 if none. */
	MVVMSAMPLE_API int32 MilestoneReached(int32 Previous, int32 Current, int32 Step = 10);
}

/**
 * Camera "trauma": hits add to it, it drains over time, and the shake is its square, so small hits barely move the
 * camera and big ones clearly do. Pure; the character applies GetShake() to the camera.
 */
struct MVVMSAMPLE_API FMvsTrauma
{
	static constexpr float DrainPerSecond = 1.8f;

	float Trauma = 0.f;

	void Add(float Amount) { Trauma = FMath::Clamp(Trauma + Amount, 0.f, 1.f); }
	/** Returns true while there is still trauma left. */
	bool Advance(float DeltaSeconds);
	float GetShake() const { return Trauma * Trauma; }
};
