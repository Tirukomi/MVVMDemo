// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/** One hostile, as the threat layer needs it for one frame. */
struct FGothamThreatIndicator
{
	/** Projected position in layer space; valid only if bProjected (false when behind the camera). */
	FVector2D Screen = FVector2D::ZeroVector;
	bool bProjected = false;
	/** Direction to the threat in camera axes (X forward, Y right, Z up), for the edge arrow. */
	FVector ViewDirection = FVector::ForwardVector;
	bool bWarning = false;
	/** Warning progress 0..1 (the counter window closing). */
	float Progress = 0.f;
	float Opacity = 1.f;
};

/**
 * Combat indicators in one pass, no widget per enemy: a counter prompt (alert strokes, the Counter key, a closing
 * timer bar) above every thug that is telegraphing an attack, and an arrow on the screen edge for every nearby
 * thug that is off screen (bright and pulsing while it warns). Active only while hostiles exist.
 */
class MVVMSAMPLE_API SThreatIndicatorLayer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SThreatIndicatorLayer) {}
	SLATE_END_ARGS()

	using FProvider = TFunction<void(TArray<FGothamThreatIndicator>&)>;

	void Construct(const FArguments& InArgs);
	void SetProvider(FProvider InProvider) { Provider = MoveTemp(InProvider); }
	void SetActive(bool bInActive);
	void SetColors(const FLinearColor& InDanger, const FLinearColor& InIdle, const FLinearColor& InPanel, const FLinearColor& InText);
	void SetKeyLabel(const FText& InLabel) { KeyLabel = InLabel; }
	void SetReducedMotion(bool bInReduced) { bReducedMotion = bInReduced; }

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Refresh(double InCurrentTime, float InDeltaTime);
	void PaintPrompt(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FGothamThreatIndicator& Threat, float Opacity) const;
	void PaintArrow(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FGothamThreatIndicator& Threat, float Opacity) const;

	FProvider Provider;
	TArray<FGothamThreatIndicator> Threats;
	TSharedPtr<FActiveTimerHandle> Timer;
	FText KeyLabel;
	FLinearColor Danger = FLinearColor::Red;
	FLinearColor Idle = FLinearColor::Gray;
	FLinearColor Panel = FLinearColor::Black;
	FLinearColor Text = FLinearColor::White;
	double Time = 0.0;
	bool bReducedMotion = false;
};
