// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * "Damage ghost" behind a bar: when the value drops, the ghost holds at the old value briefly, then drains to the new
 * one, so the player sees how much was lost. Rises are immediate. Pure, so the timing is unit-tested.
 */
struct MVVMSAMPLE_API FMvsGhostFill
{
	float Ghost = 0.f;
	float HoldRemaining = 0.f;

	static constexpr float HoldSeconds = 0.45f;
	static constexpr float DrainPerSecond = 0.9f;

	void SetTarget(float Target);
	/** Returns true while the ghost is still catching up (the owner keeps animating). */
	bool Advance(float Target, float DeltaTime);
};

/**
 * Segmented, slanted bar used for health, the combo decay and objective progress. The shown fill eases toward the
 * target and an optional ghost trails drops; both run on an active timer that stops once settled, so a static bar costs
 * nothing per frame.
 */
class MVVMSAMPLE_API SComboMeter : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SComboMeter)
		: _SegmentCount(10)
		, _DesiredSize(FVector2D(180.f, 12.f))
		, _FilledColor(FLinearColor(0.95f, 0.75f, 0.2f))
		, _EmptyColor(FLinearColor(1.f, 1.f, 1.f, 0.12f))
		, _GhostColor(FLinearColor::Transparent)
		, _Skew(0.f)
		, _Gap(3.f)
	{}
		SLATE_ARGUMENT(int32, SegmentCount)
		SLATE_ARGUMENT(FVector2D, DesiredSize)
		SLATE_ARGUMENT(FLinearColor, FilledColor)
		SLATE_ARGUMENT(FLinearColor, EmptyColor)
		/** Transparent disables the ghost. */
		SLATE_ARGUMENT(FLinearColor, GhostColor)
		/** Horizontal slant of each segment in pixels (top edge shifted right). */
		SLATE_ARGUMENT(float, Skew)
		SLATE_ARGUMENT(float, Gap)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Target fill in [0,1]; the shown fill animates toward it. */
	void SetPercent(float InPercent);
	void SetSegmentCount(int32 InCount);
	/** Snap instead of easing when reduced motion is on. */
	void SetReduceMotion(bool bInReduce) { bReduceMotion = bInReduce; }
	void SetColors(const FLinearColor& Filled, const FLinearColor& Empty);
	void SetGhostColor(const FLinearColor& InGhost);
	void SetDesiredSize(const FVector2D& InSize);
	void SetShape(float InSkew, float InGap);

	/** How many whole segments a fill fraction lights. Static so the rule is testable. */
	static int32 GetLitSegments(float Fill, int32 SegmentCount);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return DesiredSize; }

private:
	EActiveTimerReturnType TickAnimation(double InCurrentTime, float InDeltaTime);
	void EnsureAnimating();

	bool bReduceMotion = false;
	int32 SegmentCount = 10;
	FVector2D DesiredSize = FVector2D(180.f, 12.f);
	FLinearColor FilledColor;
	FLinearColor EmptyColor;
	FLinearColor GhostColor;
	float Skew = 0.f;
	float Gap = 3.f;

	float TargetFill = 0.f;
	float DisplayedFill = 0.f;
	FMvsGhostFill Ghost;
	TSharedPtr<FActiveTimerHandle> AnimationTimer;
};
