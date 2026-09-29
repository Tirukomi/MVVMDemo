// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SComboMeter.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void FGothamGhostFill::SetTarget(float Target)
{
	if (Target >= Ghost)
	{
		Ghost = Target;
		HoldRemaining = 0.f;
	}
	else
	{
		// Every new drop restarts the hold, so rapid hits read as one chunk before it drains.
		HoldRemaining = HoldSeconds;
	}
}

bool FGothamGhostFill::Advance(float Target, float DeltaTime)
{
	if (Ghost <= Target)
	{
		Ghost = Target;
		return false;
	}
	if (HoldRemaining > 0.f)
	{
		HoldRemaining = FMath::Max(0.f, HoldRemaining - DeltaTime);
		return true;
	}
	Ghost = FMath::Max(Target, Ghost - DrainPerSecond * DeltaTime);
	return Ghost > Target;
}

void SComboMeter::Construct(const FArguments& InArgs)
{
	SegmentCount = FMath::Max(1, InArgs._SegmentCount);
	DesiredSize = InArgs._DesiredSize;
	FilledColor = InArgs._FilledColor;
	EmptyColor = InArgs._EmptyColor;
	GhostColor = InArgs._GhostColor;
	Skew = InArgs._Skew;
	Gap = InArgs._Gap;
	SetCanTick(false);
}

int32 SComboMeter::GetLitSegments(float Fill, int32 InSegmentCount)
{
	return FMath::Clamp(FMath::CeilToInt(FMath::Clamp(Fill, 0.f, 1.f) * InSegmentCount - KINDA_SMALL_NUMBER), 0, InSegmentCount);
}

void SComboMeter::SetPercent(float InPercent)
{
	TargetFill = FMath::Clamp(InPercent, 0.f, 1.f);
	Ghost.SetTarget(TargetFill);
	if (bReduceMotion)
	{
		// No easing and no trailing drain: the bar simply shows the new value.
		DisplayedFill = TargetFill;
		Ghost.Ghost = TargetFill;
		Invalidate(EInvalidateWidgetReason::Paint);
		return;
	}
	EnsureAnimating();
}

void SComboMeter::EnsureAnimating()
{
	const bool bNeedsFill = !FMath::IsNearlyEqual(TargetFill, DisplayedFill, 0.001f);
	const bool bNeedsGhost = GhostColor.A > 0.f && Ghost.Ghost > TargetFill;
	if ((bNeedsFill || bNeedsGhost) && !AnimationTimer.IsValid())
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

void SComboMeter::SetGhostColor(const FLinearColor& InGhost)
{
	GhostColor = InGhost;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SComboMeter::SetDesiredSize(const FVector2D& InSize)
{
	DesiredSize = InSize;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SComboMeter::SetShape(float InSkew, float InGap)
{
	Skew = InSkew;
	Gap = InGap;
	Invalidate(EInvalidateWidgetReason::Paint);
}

EActiveTimerReturnType SComboMeter::TickAnimation(double, float InDeltaTime)
{
	if (bReduceMotion)
	{
		DisplayedFill = TargetFill;
	}
	else
	{
		DisplayedFill = FMath::FInterpTo(DisplayedFill, TargetFill, InDeltaTime, 12.f);
		if (FMath::IsNearlyEqual(DisplayedFill, TargetFill, 0.001f))
		{
			DisplayedFill = TargetFill;
		}
	}
	const bool bGhostBusy = GhostColor.A > 0.f && !bReduceMotion && Ghost.Advance(TargetFill, InDeltaTime);
	if (GhostColor.A <= 0.f)
	{
		Ghost.Ghost = TargetFill;
	}
	Invalidate(EInvalidateWidgetReason::Paint);

	if (DisplayedFill == TargetFill && !bGhostBusy)
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
	const float Slant = FMath::Min(Skew, Size.X * 0.2f);
	const float SegmentWidth = (Size.X - Slant - Gap * (SegmentCount - 1)) / SegmentCount;
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;

	// The last lit segment fades in proportionally so the drain reads as smooth, not stepped.
	const float Scaled = DisplayedFill * SegmentCount;
	const float GhostScaled = Ghost.Ghost * SegmentCount;

	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indices;
	Verts.Reserve(SegmentCount * 4);
	Indices.Reserve(SegmentCount * 6);
	for (int32 i = 0; i < SegmentCount; ++i)
	{
		const float Lit = FMath::Clamp(Scaled - i, 0.f, 1.f);
		const float Ghosted = GhostColor.A > 0.f ? FMath::Clamp(GhostScaled - i, 0.f, 1.f) : 0.f;
		FLinearColor Color = FMath::Lerp(FMath::Lerp(EmptyColor, GhostColor, Ghosted), FilledColor, Lit);
		Color.A *= Opacity;
		const FColor Vertex = Color.ToFColor(true);

		// Parallelogram: bottom edge at x0, top edge shifted right by the slant.
		const float X0 = i * (SegmentWidth + Gap);
		const FVector2D Corners[4] = {
			{ X0 + Slant, 0.0 }, { X0 + Slant + SegmentWidth, 0.0 }, { X0 + SegmentWidth, Size.Y }, { X0, Size.Y } };
		const SlateIndex Base = static_cast<SlateIndex>(Verts.Num());
		for (const FVector2D& C : Corners)
		{
			FSlateVertex& V = Verts.AddZeroed_GetRef();
			V.Position = FVector2f(AllottedGeometry.LocalToAbsolute(C));
			V.Color = Vertex;
		}
		Indices.Append({ Base, static_cast<SlateIndex>(Base + 1), static_cast<SlateIndex>(Base + 2),
			Base, static_cast<SlateIndex>(Base + 2), static_cast<SlateIndex>(Base + 3) });
	}
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
	return LayerId;
}
