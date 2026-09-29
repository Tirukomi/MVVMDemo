// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * Segmented meter that drains as the combo window closes. The displayed fill eases toward the target on an
 * active timer that stops once it catches up, so a full or empty meter is free.
 */
class MVVMSAMPLE_API SComboMeter : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SComboMeter)
		: _SegmentCount(10)
		, _DesiredSize(FVector2D(180.f, 12.f))
		, _FilledColor(FLinearColor(0.95f, 0.75f, 0.2f))
		, _EmptyColor(FLinearColor(1.f, 1.f, 1.f, 0.12f))
	{}
		SLATE_ARGUMENT(int32, SegmentCount)
		SLATE_ARGUMENT(FVector2D, DesiredSize)
		SLATE_ARGUMENT(FLinearColor, FilledColor)
		SLATE_ARGUMENT(FLinearColor, EmptyColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Target fill in [0,1]; the shown fill animates toward it. */
	void SetPercent(float InPercent);
	void SetSegmentCount(int32 InCount);
	/** Snap instead of easing when reduced motion is on. */
	void SetReduceMotion(bool bInReduce) { bReduceMotion = bInReduce; }
	void SetColors(const FLinearColor& Filled, const FLinearColor& Empty);
	void SetDesiredSize(const FVector2D& InSize);

	/** How many whole segments a fill fraction lights. Static so the rule is testable. */
	static int32 GetLitSegments(float Fill, int32 SegmentCount);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override { return DesiredSize; }

private:
	EActiveTimerReturnType TickAnimation(double InCurrentTime, float InDeltaTime);

	bool bReduceMotion = false;
	int32 SegmentCount = 10;
	FVector2D DesiredSize = FVector2D(180.f, 12.f);
	FLinearColor FilledColor;
	FLinearColor EmptyColor;

	float TargetFill = 0.f;
	float DisplayedFill = 0.f;
	TSharedPtr<FActiveTimerHandle> AnimationTimer;
};
