// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/ThreatSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Gameplay/GothamThug.h"

void UGothamThreatSubsystem::Register(AGothamThug* Thug)
{
	if (Thug)
	{
		Thugs.AddUnique(Thug);
	}
}

void UGothamThreatSubsystem::Unregister(AGothamThug* Thug)
{
	Thugs.Remove(Thug);
	if (Thugs.IsEmpty())
	{
		Snapshots.Reset();
		OnThreatsUpdated.Broadcast(Snapshots);
	}
}

bool UGothamThreatSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UGothamThreatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UGothamThreatSubsystem, STATGROUP_Tickables);
}

APawn* UGothamThreatSubsystem::GetPlayerPawn() const
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	return PC ? PC->GetPawn() : nullptr;
}

void UGothamThreatSubsystem::Tick(float DeltaTime)
{
	Thugs.RemoveAll([](const TWeakObjectPtr<AGothamThug>& Thug) { return !Thug.IsValid(); });
	APawn* Player = GetPlayerPawn();

	TArray<EGothamThugState> States;
	TArray<float> Distances;
	States.Reserve(Thugs.Num());
	Distances.Reserve(Thugs.Num());
	for (const TWeakObjectPtr<AGothamThug>& Weak : Thugs)
	{
		AGothamThug* Thug = Weak.Get();
		if (Thug->GetBrain().Advance(DeltaTime) == EGothamThugEvent::Strike)
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

void UGothamThreatSubsystem::Publish(const APawn* Player)
{
	Snapshots.Reset(Thugs.Num());
	for (const TWeakObjectPtr<AGothamThug>& Weak : Thugs)
	{
		const AGothamThug* Thug = Weak.Get();
		FGothamThreatSnapshot& Snap = Snapshots.AddDefaulted_GetRef();
		Snap.PromptLocation = Thug->GetPromptLocation();
		Snap.State = Thug->GetBrain().State;
		Snap.WarningProgress = Thug->GetBrain().GetWarningProgress();
		Snap.Distance = Player ? FVector::Dist(Thug->GetActorLocation(), Player->GetActorLocation()) : 0.f;
	}
	OnThreatsUpdated.Broadcast(Snapshots);
}

AGothamThug* UGothamThreatSubsystem::TryCounter(APawn* Player)
{
	if (!Player)
	{
		return nullptr;
	}
	AGothamThug* Best = nullptr;
	float BestDistance = CounterRange;
	for (const TWeakObjectPtr<AGothamThug>& Weak : Thugs)
	{
		AGothamThug* Thug = Weak.Get();
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

AGothamThug* UGothamThreatSubsystem::ForceWarningOnVisible()
{
	return ForceWarning(true);
}

AGothamThug* UGothamThreatSubsystem::ForceWarningBehind()
{
	return ForceWarning(false);
}

AGothamThug* UGothamThreatSubsystem::ForceWarning(bool bMostInView)
{
	const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return nullptr;
	}
	FVector ViewLocation;
	FRotator ViewRotation;
	PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	AGothamThug* Best = nullptr;
	float BestScore = -TNumericLimits<float>::Max();
	for (const TWeakObjectPtr<AGothamThug>& Weak : Thugs)
	{
		AGothamThug* Thug = Weak.Get();
		if (!Thug || Thug->GetBrain().State != EGothamThugState::Idle)
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
