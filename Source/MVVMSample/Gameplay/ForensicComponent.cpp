// Copyright IG. All Rights Reserved.

#include "Gameplay/ForensicComponent.h"

#include "EngineUtils.h"
#include "Gameplay/ClueActor.h"
#include "Gameplay/ClueDataAsset.h"

UForensicComponent::UForensicComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UForensicComponent::BeginPlay()
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

void UForensicComponent::ToggleForensic()
{
	bActive = !bActive;
	ApplyHighlights(bActive);
	if (bActive && GetOwner())
	{
		OnScanPulse.Broadcast(GetOwner()->GetActorLocation(), OpenPulseRadius);
	}
	else
	{
		EndAnalyse();
	}
	if (IsRegistered())
	{
		SetComponentTickEnabled(true);
	}
	BroadcastCurrent();
}

void UForensicComponent::ApplyHighlights(bool bEnabled)
{
	for (const TWeakObjectPtr<AClueActor>& Actor : ClueActors)
	{
		if (AClueActor* Clue = Actor.Get())
		{
			Clue->SetForensicHighlight(bEnabled);
		}
	}
}

void UForensicComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Advance(DeltaTime);
}

void UForensicComponent::Advance(float DeltaTime)
{
	const float Target = bActive ? 1.f : 0.f;
	const float Step = DeltaTime / (bReducedMotion ? 0.05f : TransitionSeconds);
	Alpha = Alpha < Target ? FMath::Min(Alpha + Step, Target) : FMath::Max(Alpha - Step, Target);
	BroadcastCurrent();

	if (Analysis.bRunning)
	{
		if (!AnalysisTarget.IsValid() || AnalysisTarget->IsScanned())
		{
			EndAnalyse();
		}
		else if (Analysis.Advance(DeltaTime, AnalyseSeconds))
		{
			CompleteScan(AnalysisTarget.Get());
			AnalysisTarget.Reset();
			BroadcastAnalysis();
		}
		else
		{
			BroadcastAnalysis();
		}
	}

	if (FMath::IsNearlyEqual(Alpha, Target) && !Analysis.bRunning && IsRegistered())
	{
		SetComponentTickEnabled(false);
	}
}

AClueActor* UForensicComponent::FindNearestUnscanned() const
{
	if (!GetOwner())
	{
		return nullptr;
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
	return Best;
}

bool UForensicComponent::TryScan()
{
	AClueActor* Best = bActive ? FindNearestUnscanned() : nullptr;
	if (!Best)
	{
		return false;
	}
	CompleteScan(Best);
	return true;
}

void UForensicComponent::CompleteScan(AClueActor* Clue)
{
	Clue->MarkScanned();
	RegisterScan(Clue->GetClue());
	OnScanPulse.Broadcast(Clue->GetActorLocation(), ScanPulseRadius);
}

bool UForensicComponent::BeginAnalyse()
{
	AClueActor* Best = bActive ? FindNearestUnscanned() : nullptr;
	if (!Best)
	{
		return false;
	}
	AnalysisTarget = Best;
	Analysis.Begin();
	if (IsRegistered())
	{
		SetComponentTickEnabled(true);
	}
	BroadcastAnalysis();
	return true;
}

void UForensicComponent::EndAnalyse()
{
	if (Analysis.bRunning)
	{
		Analysis.Cancel();
		AnalysisTarget.Reset();
		BroadcastAnalysis();
	}
}

FName UForensicComponent::GetAnalysisTargetId() const
{
	const AClueActor* Target = AnalysisTarget.Get();
	return Target && Target->GetClue() ? Target->GetClue()->ClueId : NAME_None;
}

void UForensicComponent::BroadcastAnalysis() const
{
	OnAnalysisChanged.Broadcast(Analysis.bRunning ? GetAnalysisTargetId() : NAME_None, Analysis.bRunning ? Analysis.Progress : 0.f);
}

bool UForensicComponent::GetClueLocation(FName ClueId, FVector& OutLocation) const
{
	for (const TWeakObjectPtr<AClueActor>& Actor : ClueActors)
	{
		const AClueActor* Clue = Actor.Get();
		if (Clue && Clue->GetClue() && Clue->GetClue()->ClueId == ClueId)
		{
			OutLocation = Clue->GetActorLocation();
			return true;
		}
	}
	return false;
}

void UForensicComponent::RegisterScan(const UClueDataAsset* Clue)
{
	if (Clue && !ScannedIds.Contains(Clue->ClueId))
	{
		ScannedIds.Add(Clue->ClueId);
		OnClueScanned.Broadcast(Clue);
	}
}
