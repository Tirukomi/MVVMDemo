// Copyright IG. All Rights Reserved.

#include "Gameplay/ThreatSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/MvsThug.h"

void UMvsThreatSubsystem::Register(AMvsThug* Thug)
{
	if (Thug)
	{
		Thugs.AddUnique(Thug);
	}
}

void UMvsThreatSubsystem::Unregister(AMvsThug* Thug)
{
	Thugs.Remove(Thug);
	if (Thugs.IsEmpty())
	{
		Snapshots.Reset();
		OnThreatsUpdated.Broadcast(Snapshots);
	}
}

bool UMvsThreatSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UMvsThreatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMvsThreatSubsystem, STATGROUP_Tickables);
}

APawn* UMvsThreatSubsystem::GetPlayerPawn() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

void UMvsThreatSubsystem::Tick(float DeltaTime)
{
	Thugs.RemoveAll([](const TWeakObjectPtr<AMvsThug>& Thug) { return !Thug.IsValid(); });
	APawn* Player = GetPlayerPawn();

	TArray<EMvsThugState> States;
	TArray<float> Distances;
	States.Reserve(Thugs.Num());
	Distances.Reserve(Thugs.Num());
	for (const TWeakObjectPtr<AMvsThug>& Weak : Thugs)
	{
		AMvsThug* Thug = Weak.Get();
		if (Thug->GetBrain().Advance(DeltaTime) == EMvsThugEvent::Strike)
		{
			Thug->Strike(Player);
		}
		States.Add(Thug->GetBrain().State);
		Distances.Add(Player ? FVector::Dist(Thug->GetActorLocation(), Player->GetActorLocation()) : TNumericLimits<float>::Max());
	}

	if (bDirectorEnabled && Player)
	{
		const int32 Next = Director.Advance(DeltaTime, States, Distances);
		if (Thugs.IsValidIndex(Next))
		{
			Thugs[Next]->GetBrain().BeginWarning();
		}
	}
	Publish(Player);
}

void UMvsThreatSubsystem::Publish(const APawn* Player)
{
	Snapshots.Reset(Thugs.Num());
	for (const TWeakObjectPtr<AMvsThug>& Weak : Thugs)
	{
		const AMvsThug* Thug = Weak.Get();
		FMvsThreatSnapshot& Snap = Snapshots.AddDefaulted_GetRef();
		Snap.PromptLocation = Thug->GetPromptLocation();
		Snap.State = Thug->GetBrain().State;
		Snap.WarningProgress = Thug->GetBrain().GetWarningProgress();
		Snap.Distance = Player ? FVector::Dist(Thug->GetActorLocation(), Player->GetActorLocation()) : 0.f;
	}
	OnThreatsUpdated.Broadcast(Snapshots);
}

AMvsThug* UMvsThreatSubsystem::TryCounter(APawn* Player)
{
	if (!Player)
	{
		return nullptr;
	}
	AMvsThug* Best = nullptr;
	float BestDistance = CounterRange;
	for (const TWeakObjectPtr<AMvsThug>& Weak : Thugs)
	{
		AMvsThug* Thug = Weak.Get();
		if (!Thug || !Thug->GetBrain().IsWarning())
		{
			continue;
		}
		const float Distance = FVector::Dist(Thug->GetActorLocation(), Player->GetActorLocation());
		if (Distance <= BestDistance)
		{
			Best = Thug;
			BestDistance = Distance;
		}
	}
	if (Best && Best->GetBrain().Counter())
	{
		Best->OnCountered(Player);
		return Best;
	}
	return nullptr;
}

AMvsThug* UMvsThreatSubsystem::ForceWarningOnVisible()
{
	return ForceWarning(true);
}

AMvsThug* UMvsThreatSubsystem::ForceWarningBehind()
{
	return ForceWarning(false);
}

AMvsThug* UMvsThreatSubsystem::ForceWarning(bool bMostInView)
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return nullptr;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	AMvsThug* Best = nullptr;
	float BestScore = -TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AMvsThug>& Weak : Thugs)
	{
		AMvsThug* Thug = Weak.Get();
		if (!Thug || Thug->GetBrain().State != EMvsThugState::Idle)
		{
			continue;
		}
		const float Dot = FVector::DotProduct(ViewRotation.Vector(), (Thug->GetActorLocation() - ViewLocation).GetSafeNormal());
		const float Score = bMostInView ? Dot : -Dot;
		if (Score > BestScore)
		{
			Best = Thug;
			BestScore = Score;
		}
	}
	if (Best)
	{
		Best->GetBrain().BeginWarning();
	}
	return Best;
}
