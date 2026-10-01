// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SGothamPanel.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"

namespace
{
	/** At most 8 corners; inline storage, so building an outline allocates nothing. */
	using FOutline = TArray<FVector2f, TInlineAllocator<8>>;

	template<typename TAllocator>
	void BuildChamferedRect(const FVector2f& Size, float Corner, uint8 Mask, TArray<FVector2f, TAllocator>& P)
	{
		const float C = FMath::Clamp(Corner, 0.f, 0.5f * FMath::Min(Size.X, Size.Y));
		const float W = Size.X;
		const float H = Size.Y;
		P.Reset();
		auto Cut = [Mask, C](uint8 Bit) { return (Mask & Bit) && C > 0.f; };

		if (Cut(EGothamChamfer::TopLeft))     { P.Add({ 0.f, C }); P.Add({ C, 0.f }); }     else { P.Add({ 0.f, 0.f }); }
		if (Cut(EGothamChamfer::TopRight))    { P.Add({ W - C, 0.f }); P.Add({ W, C }); }   else { P.Add({ W, 0.f }); }
		if (Cut(EGothamChamfer::BottomRight)) { P.Add({ W, H - C }); P.Add({ W - C, H }); } else { P.Add({ W, H }); }
		if (Cut(EGothamChamfer::BottomLeft))  { P.Add({ C, H }); P.Add({ 0.f, H - C }); }   else { P.Add({ 0.f, H }); }
	}

	/** The outline as a closed line, offset: the one array per line that Slate's draw element keeps. */
	TArray<FVector2f> ClosedLine(const FOutline& Outline, const FVector2f& Shift)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Outline.Num() + 1);
		for (const FVector2f& P : Outline)
		{
			Points.Add(P + Shift);
		}
		Points.Add(Outline[0] + Shift);
		return Points;
	}
}

TArray<FVector2f> GothamChamferedRect(const FVector2f& Size, float Corner, uint8 Mask)
{
	TArray<FVector2f> P;
	P.Reserve(8);
	BuildChamferedRect(Size, Corner, Mask, P);
	return P;
}

void GothamPaintPanel(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
	const FVector2f& Offset, const FVector2f& Size, const FGothamPanelLook& Look, float Opacity)
{
	if (Size.X <= 0.f || Size.Y <= 0.f)
	{
		return;
	}
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	FOutline Outline;
	BuildChamferedRect(Size, Look.Corner, Look.ChamferMask, Outline);
	auto Faded = [Opacity](FLinearColor Color, float Scale = 1.f) { Color.A *= Opacity * Scale; return Color; };

	// Glow: a few outlines stepping outward, each fainter. Cheap, and it scales with the shape.
	if (Look.GlowSize > 0.f && Look.Glow.A > 0.f)
	{
		constexpr int32 Rings = 3;
		for (int32 i = 1; i <= Rings; ++i)
		{
			const float E = Look.GlowSize * i / Rings;
			FOutline Ring;
			BuildChamferedRect(Size + FVector2f(2.f * E), Look.Corner + E * 0.4f, Look.ChamferMask, Ring);
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId, Geometry.ToPaintGeometry(), ClosedLine(Ring, Offset - FVector2f(E)),
				ESlateDrawEffect::None, Faded(Look.Glow, 1.f - (i - 1.f) / Rings), true, Look.GlowSize / Rings + 0.5f);
		}
	}

	// Fill: the outline is convex, so a fan around its centroid covers it exactly.
	if (Look.Fill.A > 0.f && Outline.Num() >= 3)
	{
		const FColor Vertex = Faded(Look.Fill).ToFColor(true);
		FVector2f Centre = FVector2f::ZeroVector;
		for (const FVector2f& P : Outline) { Centre += P; }
		Centre /= Outline.Num();

		// Scratch kept between paints (Slate paints on the game thread); the draw element copies what it needs.
		static TArray<FSlateVertex> Verts;
		static TArray<SlateIndex> Indices;
		Verts.Reset();
		Indices.Reset();
		Verts.AddZeroed(Outline.Num() + 1);
		Verts[0].Position = FVector2f(Geometry.LocalToAbsolute(FVector2D(Centre + Offset)));
		Verts[0].Color = Vertex;
		for (int32 i = 0; i < Outline.Num(); ++i)
		{
			Verts[i + 1].Position = FVector2f(Geometry.LocalToAbsolute(FVector2D(Outline[i] + Offset)));
			Verts[i + 1].Color = Vertex;
			Indices.Append({ 0, static_cast<SlateIndex>(i + 1), static_cast<SlateIndex>((i + 1) % Outline.Num() + 1) });
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
	}

	// Accent bar hugging the left edge, inside any chamfer.
	if (Look.AccentWidth > 0.f && Look.Accent.A > 0.f)
	{
		const float TopInset = (Look.ChamferMask & EGothamChamfer::TopLeft) ? FMath::Min(Look.Corner, Size.Y * 0.5f) : 0.f;
		const float BottomInset = (Look.ChamferMask & EGothamChamfer::BottomLeft) ? FMath::Min(Look.Corner, Size.Y * 0.5f) : 0.f;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			Geometry.ToPaintGeometry(FVector2f(Look.AccentWidth, FMath::Max(0.f, Size.Y - TopInset - BottomInset)), FSlateLayoutTransform(Offset + FVector2f(0.f, TopInset))),
			White, ESlateDrawEffect::None, Faded(Look.Accent));
	}

	if (Look.EdgeThickness > 0.f && Look.Edge.A > 0.f && Outline.Num() >= 2)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, Geometry.ToPaintGeometry(), ClosedLine(Outline, Offset),
			ESlateDrawEffect::None, Faded(Look.Edge), true, Look.EdgeThickness);
	}
}

void SGothamPanel::Construct(const FArguments& InArgs)
{
	Corner = InArgs._Corner;
	ChamferMask = InArgs._ChamferMask;
	FillColor = InArgs._FillColor;
	EdgeColor = InArgs._EdgeColor;
	EdgeThickness = InArgs._EdgeThickness;
	AccentColor = InArgs._AccentColor;
	AccentWidth = InArgs._AccentWidth;

	ChildSlot
	[
		SAssignNew(ContentBox, SBox)
		.Padding(InArgs._Padding)
		[
			InArgs._Content.Widget
		]
	];
}

void SGothamPanel::SetContent(const TSharedRef<SWidget>& InContent)
{
	ContentBox->SetContent(InContent);
}

void SGothamPanel::SetPadding(const FMargin& InPadding)
{
	ContentBox->SetPadding(InPadding);
}

void SGothamPanel::SetShape(float InCorner, uint8 InChamferMask)
{
	Corner = InCorner;
	ChamferMask = InChamferMask;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGothamPanel::SetColors(const FLinearColor& InFill, const FLinearColor& InEdge, float InEdgeThickness)
{
	FillColor = InFill;
	EdgeColor = InEdge;
	EdgeThickness = InEdgeThickness;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGothamPanel::SetAccent(const FLinearColor& InColor, float InWidth)
{
	AccentColor = InColor;
	AccentWidth = InWidth;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGothamPanel::SetGlow(const FLinearColor& InColor, float InSize)
{
	GlowColor = InColor;
	GlowSize = InSize;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SGothamPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FGothamPanelLook Look;
	Look.Corner = Corner;
	Look.ChamferMask = ChamferMask;
	Look.Fill = FillColor;
	Look.Edge = EdgeColor;
	Look.EdgeThickness = EdgeThickness;
	Look.Accent = AccentColor;
	Look.AccentWidth = AccentWidth;
	Look.Glow = GlowColor;
	Look.GlowSize = GlowSize;
	GothamPaintPanel(OutDrawElements, LayerId, AllottedGeometry, FVector2f::ZeroVector, FVector2f(AllottedGeometry.GetLocalSize()),
		Look, InWidgetStyle.GetColorAndOpacityTint().A);
	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
}
