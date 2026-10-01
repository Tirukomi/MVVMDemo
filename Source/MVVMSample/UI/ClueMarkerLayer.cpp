// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ClueMarkerLayer.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/TextLocalizationManager.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "UI/Style/GothamStyle.h"
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

void UClueMarkerLayer::ApplyTheme()
{
	if (SClueMarkerLayer* Layer = GetOverlay<SClueMarkerLayer>())
	{
		const FGothamTheme Theme = GothamStyle::Theme(this);
		Layer->SetColors(Theme.Color(EGothamColorToken::Unscanned), Theme.Color(EGothamColorToken::Scanned),
			Theme.Color(EGothamColorToken::Accent), Theme.Color(EGothamColorToken::TextMuted));
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
	// Cached texts are in the language they were made in.
	const uint16 Revision = FTextLocalizationManager::Get().GetTextRevision();
	if (Revision != TextCacheRevision)
	{
		TextCache.Reset();
		TextCacheRevision = Revision;
	}
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

		FMarkerText& Text = TextCache.FindOrAdd(Entry->GetClueId());
		if (Text.State != static_cast<uint8>(M.State))
		{
			Text.State = static_cast<uint8>(M.State);
			Text.Label = (bAnalysing ? LOCTEXT("Analysing", "Analysing")
				: Entry->GetIsDiscovered() ? Entry->GetDisplayTitle() : LOCTEXT("Unknown", "Unknown evidence")).ToUpper();
		}
		const int32 Meters = FMath::RoundToInt(Distance / 100.f);
		if (Text.Meters != Meters)
		{
			Text.Meters = Meters;
			Text.Distance = FText::Format(LOCTEXT("DistanceFmt", "{0} m"), FText::AsNumber(Meters));
		}
		M.Label = Text.Label;
		M.Distance = Text.Distance;
	}
}

#undef LOCTEXT_NAMESPACE
