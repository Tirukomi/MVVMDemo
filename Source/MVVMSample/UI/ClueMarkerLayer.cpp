// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ClueMarkerLayer.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/GothamMVVM.h"

#define LOCTEXT_NAMESPACE "Gotham.ClueMarkers"

TSharedRef<SGothamWorldOverlayBase> UClueMarkerLayer::MakeOverlay()
{
	TSharedRef<SClueMarkerLayer> Layer = SNew(SClueMarkerLayer);
	Layer->SetProvider(MakeProvider(&UClueMarkerLayer::BuildMarkers));
	return Layer;
}

bool UClueMarkerLayer::ShouldBeActive() const
{
	return Detective && Detective->GetIsVisible();
}

void UClueMarkerLayer::SetViewModels(UClueListViewModel* InClues, UDetectiveViewModel* InDetective)
{
	GothamMVVM::Unbind(Detective, this);
	Clues = InClues;
	Detective = InDetective;
	GothamMVVM::Bind(Detective, this, &UClueMarkerLayer::OnDetectiveChanged,
		{ UDetectiveViewModel::FFieldNotificationClassDescriptor::bIsVisible });
	UpdateActive();
}

void UClueMarkerLayer::SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted)
{
	if (SClueMarkerLayer* Layer = GetOverlay<SClueMarkerLayer>())
	{
		Layer->SetColors(InUnknown, InKnown, InAnalysing, InMuted);
	}
}

void UClueMarkerLayer::OnDetectiveChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	UpdateActive();
}

void UClueMarkerLayer::BuildMarkers(TArray<FGothamClueMarker>& Out) const
{
	APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Clues || !Detective || !Pawn)
	{
		return;
	}
	const float FadeIn = Detective->GetAlpha();
	for (const UClueEntryViewModel* Entry : Clues->GetEntries())
	{
		FVector World;
		if (!Entry || !Entry->GetWorldLocation(World))
		{
			continue;
		}
		const float Distance = FVector::Dist(World, Pawn->GetActorLocation());
		if (Distance > MaxDistance)
		{
			continue;
		}
		FVector2D Screen;
		if (!ProjectToLayer(World, Screen))
		{
			continue;
		}
		FGothamClueMarker& M = Out.AddDefaulted_GetRef();
		M.Position = Screen;
		M.Scale = GothamMarkers::ScaleForDistance(Distance);
		M.Opacity = GothamMarkers::OpacityForDistance(Distance, MaxDistance) * FadeIn;
		const bool bAnalysing = Detective->GetAnalysisTargetId() == Entry->GetClueId();
		M.State = bAnalysing ? FGothamClueMarker::EState::Analysing
			: Entry->GetIsDiscovered() ? FGothamClueMarker::EState::Known : FGothamClueMarker::EState::Unknown;
		M.Progress = bAnalysing ? Detective->GetAnalysisProgress() : 0.f;
		M.Label = bAnalysing ? LOCTEXT("Analysing", "Analysing")
			: Entry->GetIsDiscovered() ? Entry->GetDisplayTitle() : LOCTEXT("Unknown", "Unknown evidence");
		M.Label = M.Label.ToUpper();
		M.Distance = FText::Format(LOCTEXT("DistanceFmt", "{0} m"), FText::AsNumber(FMath::RoundToInt(Distance / 100.f)));
	}
}

#undef LOCTEXT_NAMESPACE
