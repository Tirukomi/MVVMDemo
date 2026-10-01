// Copyright IG. All Rights Reserved.

#include "Gameplay/ThreatTypes.h"

bool FMvsThugBrain::BeginWarning()
{
	if (State != EMvsThugState::Idle)
	{
		return false;
	}
	State = EMvsThugState::Warning;
	Elapsed = 0.f;
	return true;
}

bool FMvsThugBrain::Counter()
{
	if (State != EMvsThugState::Warning)
	{
		return false;
	}
	State = EMvsThugState::Stunned;
	Elapsed = 0.f;
	return true;
}

EMvsThugEvent FMvsThugBrain::Advance(float DeltaSeconds)
{
	Elapsed += FMath::Max(DeltaSeconds, 0.f);
	switch (State)
	{
	case EMvsThugState::Warning:
		if (Elapsed >= WarningSeconds)
		{
			State = EMvsThugState::Recover;
			Elapsed = 0.f;
			return EMvsThugEvent::Strike;
		}
		break;
	case EMvsThugState::Stunned:
	case EMvsThugState::Recover:
		if (Elapsed >= (State == EMvsThugState::Stunned ? StunSeconds : RecoverSeconds))
		{
			State = EMvsThugState::Idle;
			Elapsed = 0.f;
			return EMvsThugEvent::Ready;
		}
		break;
	default:
		break;
	}
	return EMvsThugEvent::None;
}

float FMvsThugBrain::GetWarningProgress() const
{
	return State == EMvsThugState::Warning ? FMath::Clamp(Elapsed / WarningSeconds, 0.f, 1.f) : 0.f;
}

int32 FMvsAttackDirector::Advance(float DeltaSeconds, const TArray<EMvsThugState>& States, const TArray<float>& Distances)
{
	if (States.Contains(EMvsThugState::Warning))
	{
		// Someone is already telegraphing; the breather starts once they are done.
		Cooldown = FMath::Max(Cooldown, MinGapSeconds);
		return INDEX_NONE;
	}
	Cooldown -= FMath::Max(DeltaSeconds, 0.f);
	if (Cooldown > 0.f)
	{
		return INDEX_NONE;
	}

	TArray<int32> Candidates;
	for (int32 i = 0; i < States.Num() && i < Distances.Num(); ++i)
	{
		if (States[i] == EMvsThugState::Idle && Distances[i] <= EngageRange)
		{
			Candidates.Add(i);
		}
	}
	if (Candidates.IsEmpty())
	{
		return INDEX_NONE; // try again next frame, no new breather
	}
	Cooldown = Random.FRandRange(MinGapSeconds, MaxGapSeconds);
	return Candidates[Random.RandHelper(Candidates.Num())];
}

namespace MvsThreat
{
	void EdgeArrow(const FVector& ViewDirection, const FVector2D& Viewport, float Inset, FVector2D& OutPosition, float& OutAngle)
	{
		// Screen-space direction from the centre: right is +X, down is +Y.
		FVector2D Dir(ViewDirection.Y, -ViewDirection.Z);
		if (Dir.SizeSquared() < KINDA_SMALL_NUMBER)
		{
			Dir = FVector2D(0.f, 1.f); // dead ahead or dead behind with nothing sideways: point down ("behind you")
		}
		Dir.Normalize();

		const FVector2D Centre = Viewport * 0.5f;
		const FVector2D Half = FVector2D(FMath::Max(Centre.X - Inset, 1.f), FMath::Max(Centre.Y - Inset, 1.f));
		// Ray from the centre to the inset rectangle: scale so the larger axis ratio touches its edge.
		const float Scale = 1.f / FMath::Max(FMath::Abs(Dir.X) / Half.X, FMath::Abs(Dir.Y) / Half.Y);
		OutPosition = Centre + Dir * Scale;
		OutAngle = FMath::Atan2(Dir.Y, Dir.X);
	}

	bool IsOnScreen(const FVector2D& Screen, const FVector2D& Viewport, float Margin)
	{
		return Screen.X >= Margin && Screen.Y >= Margin && Screen.X <= Viewport.X - Margin && Screen.Y <= Viewport.Y - Margin;
	}
}

namespace MvsCombo
{
	int32 MilestoneReached(int32 Previous, int32 Current, int32 Step)
	{
		if (Step <= 0 || Current <= Previous)
		{
			return 0;
		}
		const int32 Reached = (Current / Step) * Step;
		return Reached > Previous && Reached > 0 ? Reached : 0;
	}
}

bool FMvsTrauma::Advance(float DeltaSeconds)
{
	Trauma = FMath::Max(0.f, Trauma - DrainPerSecond * FMath::Max(DeltaSeconds, 0.f));
	return Trauma > 0.f;
}
