// Copyright IG. All Rights Reserved.

#include "UI/ClueMarkerLayer.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Internationalization/TextLocalizationManager.h"
#include "UI/Slate/SClueMarkerLayer.h"
#include "UI/Style/MvsStyle.h"
#include "ViewModels/ClueViewModels.h"
#include "ViewModels/ForensicViewModel.h"
#include "ViewModels/MvsMVVM.h"

#define LOCTEXT_NAMESPACE "Mvs.ClueMarkers"

TSharedRef<SMvsWorldOverlayBase> UClueMarkerLayer::MakeOverlay()
{
	TSharedRef<SClueMarkerLayer> Layer = SNew(SClueMarkerLayer);
	Layer->SetProvider(MakeProvider(&UClueMarkerLayer::BuildMarkers));
	return Layer;
}

bool UClueMarkerLayer::ShouldBeActive() const
{
	return Forensic && Forensic->GetIsVisible();
}

void UClueMarkerLayer::SetViewModels(UClueListViewModel* InClues, UForensicViewModel* InForensic)
{
	MvsMVVM::Unbind(Forensic, this);
	Clues = InClues;
	Forensic = InForensic;
	MvsMVVM::Bind(Forensic, this, &UClueMarkerLayer::OnForensicChanged,
		{ UForensicViewModel::FFieldNotificationClassDescriptor::bIsVisible });
	UpdateActive();
}

void UClueMarkerLayer::ApplyTheme()
{
	if (SClueMarkerLayer* Layer = GetOverlay<SClueMarkerLayer>())
	{
		const FMvsTheme Theme = MvsStyle::Theme(this);
		Layer->SetColors(Theme.Color(EMvsColorToken::Unscanned), Theme.Color(EMvsColorToken::Scanned),
			Theme.Color(EMvsColorToken::Accent), Theme.Color(EMvsColorToken::TextMuted));
	}
}

void UClueMarkerLayer::OnForensicChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId)
{
	UpdateActive();
}

void UClueMarkerLayer::BuildMarkers(TArray<FMvsClueMarker>& Out) const
{
	APlayerController* PC = GetOwningPlayer();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Clues || !Forensic || !Pawn)
	{
		return;
	}
	const float FadeIn = Forensic->GetAlpha();
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
		FMvsClueMarker& M = Out.AddDefaulted_GetRef();
		M.Position = Screen;
		M.Scale = MvsMarkers::ScaleForDistance(Distance);
		M.Opacity = MvsMarkers::OpacityForDistance(Distance, MaxDistance) * FadeIn;
		const bool bAnalysing = Forensic->GetAnalysisTargetId() == Entry->GetClueId();
		M.State = bAnalysing ? FMvsClueMarker::EState::Analysing
			: Entry->GetIsDiscovered() ? FMvsClueMarker::EState::Known : FMvsClueMarker::EState::Unknown;
		M.Progress = bAnalysing ? Forensic->GetAnalysisProgress() : 0.f;

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
