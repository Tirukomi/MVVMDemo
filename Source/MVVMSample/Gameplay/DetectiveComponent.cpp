// Copyright Epic Games, Inc. All Rights Reserved.

#include "Gameplay/DetectiveComponent.h"

#include "EngineUtils.h"
#include "Gameplay/ClueActor.h"
#include "Gameplay/ClueDataAsset.h"

UDetectiveComponent::UDetectiveComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UDetectiveComponent::BeginPlay()
{
	Super::BeginPlay();

	AllClues.Reset();
	ClueActors.Reset();
	for (TActorIterator<AClueActor> It(GetWorld()); It; ++It)
	{
		ClueActors.Add(*It);
		if (const UClueDataAsset* Clue = It->GetClue())
		{
			AllClues.Add(Clue);
		}
	}
	OnCluesCollected.Broadcast();
	BroadcastCurrent();
}

void UDetectiveComponent::ToggleDetective()
{
	bActive = !bActive;
	ApplyHighlights(bActive);
	if (IsRegistered())
	{
		SetComponentTickEnabled(true);
	}
	BroadcastCurrent();
}

void UDetectiveComponent::ApplyHighlights(bool bEnabled)
{
	for (const TWeakObjectPtr<AClueActor>& Actor : ClueActors)
	{
		if (AClueActor* Clue = Actor.Get())
		{
			Clue->SetDetectiveHighlight(bEnabled);
		}
	}
}

void UDetectiveComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UDetectiveComponent::Advance(float DeltaTime)
{
	const float Target = bActive ? 1.f : 0.f;
	const float Step = DeltaTime / TransitionSeconds;
	Alpha = Alpha < Target ? FMath::Min(Alpha + Step, Target) : FMath::Max(Alpha - Step, Target);
	BroadcastCurrent();

	if (FMath::IsNearlyEqual(Alpha, Target) && IsRegistered())
	{
		SetComponentTickEnabled(false);
	}
}

bool UDetectiveComponent::TryScan()
{
	if (!bActive || !GetOwner())
	{
		return false;
	}

	AClueActor* Best = nullptr;
	float BestDistSq = FMath::Square(ScanRadius);
	for (const TWeakObjectPtr<AClueActor>& Actor : ClueActors)
	{
		AClueActor* Clue = Actor.Get();
		if (!Clue || Clue->IsScanned())
		{
			continue;
		}
		const float DistSq = FVector::DistSquared(Clue->GetActorLocation(), GetOwner()->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Clue;
		}
	}
	if (!Best)
	{
		return false;
	}

	Best->MarkScanned();
	RegisterScan(Best->GetClue());
	return true;
}

void UDetectiveComponent::RegisterScan(const UClueDataAsset* Clue)
{
	if (Clue && !ScannedIds.Contains(Clue->ClueId))
	{
		ScannedIds.Add(Clue->ClueId);
		OnClueScanned.Broadcast(Clue);
	}
}
