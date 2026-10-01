// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Slate/SMvsWorldOverlay.h"

/** One hostile, as the threat layer needs it for one frame. */
struct FMvsThreatIndicator
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
 * thug that is off screen (bright and pulsing while it warns). A world overlay: active only while hostiles exist.
 */
class MVVMSAMPLE_API SThreatIndicatorLayer : public SMvsWorldOverlay<FMvsThreatIndicator>
{
public:
	SLATE_BEGIN_ARGS(SThreatIndicatorLayer) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** What a screen reader announces for the threat prompts and arrows. */
	FText GetAccessibleSummary() const;
	void SetColors(const FLinearColor& InDanger, const FLinearColor& InIdle, const FLinearColor& InPanel, const FLinearColor& InText);
	void SetKeyLabel(const FText& InLabel) { KeyLabel = InLabel; }
	void SetReducedMotion(bool bInReduced) { bReducedMotion = bInReduced; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void PaintPrompt(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FMvsThreatIndicator& Threat, float Opacity) const;
	void PaintArrow(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FMvsThreatIndicator& Threat, float Opacity) const;

	FText KeyLabel;
	FLinearColor Danger = FLinearColor::Red;
	FLinearColor Idle = FLinearColor::Gray;
	FLinearColor Panel = FLinearColor::Black;
	FLinearColor Text = FLinearColor::White;
	bool bReducedMotion = false;
};
