// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/GothamWorldOverlayLayer.h"
#include "ClueMarkerLayer.generated.h"

class UClueListViewModel;
class UDetectiveViewModel;

/**
 * Detective Mode's world-anchored clue markers. Reads the clue list and detective view models, projects each clue into
 * the HUD with the owning player's view, and hands the markers to SClueMarkerLayer. Active only while Detective Mode is
 * visible; everything else about the markers (what to show, how far, which is being analysed) comes from view models.
 */
UCLASS()
class MVVMSAMPLE_API UClueMarkerLayer : public UGothamWorldOverlayLayer
{
	GENERATED_BODY()

public:
	/** Markers beyond this range are not drawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markers", meta = (ClampMin = "100"))
	float MaxDistance = 3500.f;

	void SetViewModels(UClueListViewModel* InClues, UDetectiveViewModel* InDetective);
	void SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted);

protected:
	virtual TSharedRef<SGothamWorldOverlayBase> MakeOverlay() override;
	virtual bool ShouldBeActive() const override;

private:
	void OnDetectiveChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void BuildMarkers(TArray<struct FGothamClueMarker>& Out) const;

	/**
	 * A marker's text, kept between frames: formatting the distance and upper-casing the label allocate, so they are
	 * redone only when what they show changes (another metre, another state, another language).
	 */
	struct FMarkerText
	{
		int32 Meters = INDEX_NONE;
		uint8 State = 0xFF;
		FText Distance;
		FText Label;
	};
	mutable TMap<FName, FMarkerText> TextCache;
	mutable uint16 TextCacheRevision = 0;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> Detective;
};
