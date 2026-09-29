// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/** One world-anchored clue marker, already projected into the layer's local space. */
struct FGothamClueMarker
{
	enum class EState : uint8 { Unknown, Known, Analysing };

	FVector2D Position = FVector2D::ZeroVector;
	float Scale = 1.f;
	float Opacity = 1.f;
	EState State = EState::Unknown;
	float Progress = 0.f;
	FText Label;
	FText Distance;
};

/** Marker sizing and fading by distance. Pure, so the rules are testable. */
namespace GothamMarkers
{
	/** Bracket scale: larger up close, clamped so far markers stay readable and near ones never swamp the screen. */
	MVVMSAMPLE_API float ScaleForDistance(float DistanceCm);
	/** Fades markers out toward MaxDistanceCm (1 inside 70% of the range, 0 at and beyond it). */
	MVVMSAMPLE_API float OpacityForDistance(float DistanceCm, float MaxDistanceCm);
}

/**
 * Draws every clue marker in one paint pass: corner brackets, a label and distance, and an analysis arc on the clue being
 * analysed. While active it refreshes markers through a provider on an active timer (positions change with the camera);
 * when inactive it unregisters the timer and paints nothing, so it costs nothing outside Detective Mode.
 */
class MVVMSAMPLE_API SClueMarkerLayer : public SLeafWidget
{
public:
	using FProvider = TFunction<void(TArray<FGothamClueMarker>& OutMarkers)>;

	SLATE_BEGIN_ARGS(SClueMarkerLayer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetProvider(FProvider InProvider) { Provider = MoveTemp(InProvider); }
	void SetActive(bool bInActive);
	void SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

private:
	EActiveTimerReturnType Refresh(double InCurrentTime, float InDeltaTime);

	FProvider Provider;
	TArray<FGothamClueMarker> Markers;
	TSharedPtr<FActiveTimerHandle> Timer;
	FLinearColor UnknownColor = FLinearColor(1.f, 0.55f, 0.1f);
	FLinearColor KnownColor = FLinearColor(0.35f, 0.7f, 0.9f);
	FLinearColor AnalysingColor = FLinearColor(0.96f, 0.68f, 0.22f);
	FLinearColor MutedColor = FLinearColor(0.5f, 0.56f, 0.64f);
};
