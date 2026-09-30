// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ClueMarkerLayer.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/DetectiveViewModel.h"
#include "ViewModels/GothamMVVM.h"

#define LOCTEXT_NAMESPACE "Gotham.ClueMarkers"

TSharedRef<SWidget> UClueMarkerLayer::RebuildWidget()
{
	SlateLayer = SNew(SClueMarkerLayer);
	const TWeakObjectPtr<UClueMarkerLayer> Weak(this);
	SlateLayer->SetProvider([Weak](TArray<FGothamClueMarker>& Out)
	{
		if (const UClueMarkerLayer* Self = Weak.Get())
		{
			Self->BuildMarkers(Out);
		}
	});
	SlateLayer->SetActive(Detective && Detective->GetIsVisible());
	return SlateLayer.ToSharedRef();
}

void UClueMarkerLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateLayer.Reset();
}

void UClueMarkerLayer::SetViewModels(UClueListViewModel* InClues, UDetectiveViewModel* InDetective)
{
	GothamMVVM::Unbind(Detective, this);
	Clues = InClues;
	Detective = InDetective;
	GothamMVVM::Bind(Detective, this, &UClueMarkerLayer::OnDetectiveChanged,
		{ UDetectiveViewModel::FFieldNotificationClassDescriptor::bIsVisible });
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetActive(Detective && Detective->GetIsVisible());
	}
}

void UClueMarkerLayer::SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted)
{
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetColors(InUnknown, InKnown, InAnalysing, InMuted);
	}
}

void UClueMarkerLayer::OnDetectiveChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	if (SlateLayer.IsValid())
	{
		SlateLayer->SetActive(Detective && Detective->GetIsVisible());
	}
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
		// Viewport-relative and DPI-adjusted: the same space as the full-screen HUD canvas this layer fills.
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, World, Screen, true))
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
