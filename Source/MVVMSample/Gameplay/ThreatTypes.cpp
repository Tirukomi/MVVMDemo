// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ThreatTypes.h"

bool FGothamThugBrain::BeginWarning()
{
	if (State != EGothamThugState::Idle)
	{
		return false;
	}
	State = EGothamThugState::Warning;
	Elapsed = 0.f;
	return true;
}

bool FGothamThugBrain::Counter()
{
	if (State != EGothamThugState::Warning)
	{
		return false;
	}
	State = EGothamThugState::Stunned;
	Elapsed = 0.f;
	return true;
}

EGothamThugEvent FGothamThugBrain::Advance(float DeltaSeconds)
{
	Elapsed += FMath::Max(DeltaSeconds, 0.f);
	switch (State)
	{
	case EGothamThugState::Warning:
		if (Elapsed >= WarningSeconds)
		{
			State = EGothamThugState::Recover;
			Elapsed = 0.f;
			return EGothamThugEvent::Strike;
		}
		break;
	case EGothamThugState::Stunned:
	case EGothamThugState::Recover:
		if (Elapsed >= (State == EGothamThugState::Stunned ? StunSeconds : RecoverSeconds))
		{
			State = EGothamThugState::Idle;
			Elapsed = 0.f;
			return EGothamThugEvent::Ready;
		}
		break;
	default:
		break;
	}
	return EGothamThugEvent::None;
}

float FGothamThugBrain::GetWarningProgress() const
{
	return State == EGothamThugState::Warning ? FMath::Clamp(Elapsed / WarningSeconds, 0.f, 1.f) : 0.f;
}

int32 FGothamAttackDirector::Advance(float DeltaSeconds, const TArray<EGothamThugState>& States, const TArray<float>& Distances)
{
	if (States.Contains(EGothamThugState::Warning))
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
		if (States[i] == EGothamThugState::Idle && Distances[i] <= EngageRange)
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

namespace GothamThreat
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

namespace GothamCombo
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

bool FGothamTrauma::Advance(float DeltaSeconds)
{
	Trauma = FMath::Max(0.f, Trauma - DrainPerSecond * FMath::Max(DeltaSeconds, 0.f));
	return Trauma > 0.f;
}
