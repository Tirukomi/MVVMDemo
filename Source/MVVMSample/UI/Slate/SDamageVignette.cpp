// Copyright IG. All Rights Reserved.

#include "UI/Slate/SDamageVignette.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SDamageVignette::Construct(const FArguments& InArgs)
{
	Color = InArgs._Color;
	SetCanTick(false);
	SetVisibility(EVisibility::HitTestInvisible);
}

void SDamageVignette::SetIntensity(float InIntensity)
{
	const float Clamped = FMath::Clamp(InIntensity, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Clamped, Intensity, 0.003f))
	{
		Intensity = Clamped;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void SDamageVignette::SetColor(const FLinearColor& InColor)
{
	Color = InColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SDamageVignette::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Intensity <= 0.f)
	{
		return LayerId;
	}
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Depth = 0.22f * FMath::Min(Size.X, Size.Y);
	FLinearColor Outer = Color;
	Outer.A = 0.6f * Intensity * InWidgetStyle.GetColorAndOpacityTint().A;
	FLinearColor Inner = Color;
	Inner.A = 0.f;
	const FColor OuterC = Outer.ToFColor(true);
	const FColor InnerC = Inner.ToFColor(true);

	// Eight points: the screen corners and the same corners pulled inward by Depth.
	const FVector2D O[4] = { { 0, 0 }, { Size.X, 0 }, { Size.X, Size.Y }, { 0, Size.Y } };
	const FVector2D I[4] = { { Depth, Depth }, { Size.X - Depth, Depth }, { Size.X - Depth, Size.Y - Depth }, { Depth, Size.Y - Depth } };
	TArray<FSlateVertex> Verts;
	TArray<SlateIndex> Indices;
	for (int32 k = 0; k < 4; ++k)
	{
		FSlateVertex& VO = Verts.AddZeroed_GetRef();
		VO.Position = FVector2f(AllottedGeometry.LocalToAbsolute(O[k]));
		VO.Color = OuterC;
	}
	for (int32 k = 0; k < 4; ++k)
	{
		FSlateVertex& VI = Verts.AddZeroed_GetRef();
		VI.Position = FVector2f(AllottedGeometry.LocalToAbsolute(I[k]));
		VI.Color = InnerC;
	}
	for (int32 k = 0; k < 4; ++k)
	{
		const SlateIndex A = static_cast<SlateIndex>(k), B = static_cast<SlateIndex>((k + 1) % 4);
		const SlateIndex IA = static_cast<SlateIndex>(k + 4), IB = static_cast<SlateIndex>((k + 1) % 4 + 4);
		Indices.Append({ A, B, IB, A, IB, IA });
	}
	FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, FCoreStyle::Get().GetBrush("GenericWhiteBox")->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
	return LayerId;
}
