// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SComboMeter.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SComboMeter::Construct(const FArguments& InArgs)
{
	SegmentCount = FMath::Max(1, InArgs._SegmentCount);
	DesiredSize = InArgs._DesiredSize;
	FilledColor = InArgs._FilledColor;
	EmptyColor = InArgs._EmptyColor;
	SetCanTick(false);
}

int32 SComboMeter::GetLitSegments(float Fill, int32 SegmentCount)
{
	return FMath::Clamp(FMath::CeilToInt(FMath::Clamp(Fill, 0.f, 1.f) * SegmentCount - KINDA_SMALL_NUMBER), 0, SegmentCount);
}

void SComboMeter::SetPercent(float InPercent)
{
	TargetFill = FMath::Clamp(InPercent, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(TargetFill, DisplayedFill, 0.001f) && !AnimationTimer.IsValid())
	{
		AnimationTimer = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SComboMeter::TickAnimation));
	}
}

void SComboMeter::SetSegmentCount(int32 InCount)
{
	SegmentCount = FMath::Max(1, InCount);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SComboMeter::SetColors(const FLinearColor& Filled, const FLinearColor& Empty)
{
	FilledColor = Filled;
	EmptyColor = Empty;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SComboMeter::SetDesiredSize(const FVector2D& InSize)
{
	DesiredSize = InSize;
	Invalidate(EInvalidateWidgetReason::Layout);
}

EActiveTimerReturnType SComboMeter::TickAnimation(double, float InDeltaTime)
{
	DisplayedFill = FMath::FInterpTo(DisplayedFill, TargetFill, InDeltaTime, 12.f);
	const bool bSettled = FMath::IsNearlyEqual(DisplayedFill, TargetFill, 0.001f);
	if (bSettled)
	{
		DisplayedFill = TargetFill;
	}
	Invalidate(EInvalidateWidgetReason::Paint);

	if (bSettled)
	{
		AnimationTimer.Reset();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

int32 SComboMeter::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	constexpr float Gap = 3.f;
	const float SegmentWidth = (Size.X - Gap * (SegmentCount - 1)) / SegmentCount;
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;

	// The last lit segment fades in proportionally so the drain reads as smooth, not stepped.
	const float Scaled = DisplayedFill * SegmentCount;
	for (int32 i = 0; i < SegmentCount; ++i)
	{
		const float Lit = FMath::Clamp(Scaled - i, 0.f, 1.f);
		FLinearColor Color = FMath::Lerp(EmptyColor, FilledColor, Lit);
		Color.A *= Opacity;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(FVector2f(SegmentWidth, Size.Y), FSlateLayoutTransform(FVector2f(i * (SegmentWidth + Gap), 0.f))),
			White, ESlateDrawEffect::None, Color);
	}
	return LayerId;
}
