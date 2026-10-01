// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Slate/SMvsPanel.h"
#include "UI/Style/MvsMotion.h"
#include "Widgets/SLeafWidget.h"

/**
 * The menu highlight bar. Sits behind a list of items (same parent geometry) and slides to whichever item is current.
 * It reads the target's geometry each frame while it is active, so it also follows reflow (UI scale, language) and
 * scrolling. The active timer only runs while the owning list holds focus or the bar is still moving.
 */
class MVVMSAMPLE_API SMvsHighlight : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SMvsHighlight) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Moves the bar to Target. bSnap jumps there (reduced motion, or the first placement). */
	void SetTarget(const TSharedPtr<SWidget>& InTarget, bool bSnap);
	/** Active: full strength and following the target. Inactive: dimmed in place (focus left the list). */
	void SetActive(bool bInActive);
	void SetLook(const FMvsPanelLook& InLook);
	/** Under reduced motion the bar jumps instead of sliding. */
	void SetReducedMotion(bool bInReduced) { bReducedMotion = bInReduced; }

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	EActiveTimerReturnType Update(double InCurrentTime, float InDeltaTime);
	void EnsureTimer();
	/** Target rect in this widget's local space; false if the target has no layout yet. */
	bool ReadTargetRect(FVector2f& OutPosition, FVector2f& OutSize) const;

	TWeakPtr<SWidget> Target;
	TSharedPtr<FActiveTimerHandle> Timer;
	FMvsSlideRect Slide;
	FMvsPanelLook Look;
	bool bActive = false;
	bool bSnapNext = true;
	bool bReducedMotion = false;
};
