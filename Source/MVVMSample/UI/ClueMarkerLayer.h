// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "ClueMarkerLayer.generated.h"

class SClueMarkerLayer;
class UClueListViewModel;
class UDetectiveViewModel;

/**
 * Detective Mode's world-anchored clue markers. Reads the clue list and detective view models, projects each clue into
 * the HUD with the owning player's view, and hands the markers to SClueMarkerLayer. Active only while Detective Mode is
 * visible; everything else about the markers (what to show, how far, which is being analysed) comes from view models.
 */
UCLASS()
class MVVMSAMPLE_API UClueMarkerLayer : public UWidget
{
	GENERATED_BODY()

public:
	/** Markers beyond this range are not drawn. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Markers", meta = (ClampMin = "100"))
	float MaxDistance = 3500.f;

	void SetViewModels(UClueListViewModel* InClues, UDetectiveViewModel* InDetective);
	void SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted);

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void OnDetectiveChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	void BuildMarkers(TArray<struct FGothamClueMarker>& Out) const;

	UPROPERTY(Transient)
	TObjectPtr<UClueListViewModel> Clues;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveViewModel> Detective;

	TSharedPtr<SClueMarkerLayer> SlateLayer;
};
