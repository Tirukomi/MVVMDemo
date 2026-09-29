// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SGothamPanel.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"

TArray<FVector2f> GothamChamferedRect(const FVector2f& Size, float Corner, uint8 Mask)
{
	const float C = FMath::Clamp(Corner, 0.f, 0.5f * FMath::Min(Size.X, Size.Y));
	const float W = Size.X;
	const float H = Size.Y;
	TArray<FVector2f> P;
	P.Reserve(8);
	auto Cut = [Mask, C](uint8 Bit) { return (Mask & Bit) && C > 0.f; };

	if (Cut(EGothamChamfer::TopLeft))     { P.Add({ 0.f, C }); P.Add({ C, 0.f }); }     else { P.Add({ 0.f, 0.f }); }
	if (Cut(EGothamChamfer::TopRight))    { P.Add({ W - C, 0.f }); P.Add({ W, C }); }   else { P.Add({ W, 0.f }); }
	if (Cut(EGothamChamfer::BottomRight)) { P.Add({ W, H - C }); P.Add({ W - C, H }); } else { P.Add({ W, H }); }
	if (Cut(EGothamChamfer::BottomLeft))  { P.Add({ C, H }); P.Add({ 0.f, H - C }); }   else { P.Add({ 0.f, H }); }
	return P;
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

int32 SGothamPanel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	const TArray<FVector2f> Outline = GothamChamferedRect(Size, Corner, ChamferMask);
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");

	// Fill: the outline is convex, so a fan around its centroid covers it exactly.
	if (FillColor.A > 0.f && Outline.Num() >= 3)
	{
		FLinearColor Fill = FillColor;
		Fill.A *= Opacity;
		const FColor Vertex = Fill.ToFColor(true);
		FVector2f Centre = FVector2f::ZeroVector;
		for (const FVector2f& P : Outline) { Centre += P; }
		Centre /= Outline.Num();

		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		Verts.AddZeroed(Outline.Num() + 1);
		Verts[0].Position = FVector2f(AllottedGeometry.LocalToAbsolute(FVector2D(Centre)));
		Verts[0].Color = Vertex;
		for (int32 i = 0; i < Outline.Num(); ++i)
		{
			Verts[i + 1].Position = FVector2f(AllottedGeometry.LocalToAbsolute(FVector2D(Outline[i])));
			Verts[i + 1].Color = Vertex;
			Indices.Append({ 0, static_cast<SlateIndex>(i + 1), static_cast<SlateIndex>((i + 1) % Outline.Num() + 1) });
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
	}

	// Accent bar hugging the left edge, inside any chamfer.
	if (AccentWidth > 0.f && AccentColor.A > 0.f)
	{
		const float TopInset = (ChamferMask & EGothamChamfer::TopLeft) ? FMath::Min(Corner, Size.Y * 0.5f) : 0.f;
		const float BottomInset = (ChamferMask & EGothamChamfer::BottomLeft) ? FMath::Min(Corner, Size.Y * 0.5f) : 0.f;
		FLinearColor Accent = AccentColor;
		Accent.A *= Opacity;
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(AccentWidth, FMath::Max(0.f, Size.Y - TopInset - BottomInset)), FSlateLayoutTransform(FVector2f(0.f, TopInset))),
			White, ESlateDrawEffect::None, Accent);
	}

	if (EdgeThickness > 0.f && EdgeColor.A > 0.f && Outline.Num() >= 2)
	{
		TArray<FVector2f> Closed = Outline;
		Closed.Add(Outline[0]);
		FLinearColor Edge = EdgeColor;
		Edge.A *= Opacity;
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), MoveTemp(Closed),
			ESlateDrawEffect::None, Edge, true, EdgeThickness);
	}

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId + 2, InWidgetStyle, bParentEnabled);
}
