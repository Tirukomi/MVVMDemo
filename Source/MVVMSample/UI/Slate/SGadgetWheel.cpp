// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SGadgetWheel.h"
#include "UI/GothamAccessibility.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

namespace
{
	constexpr float AnimationSpeed = 14.f;
	constexpr float StickDeadZone = 0.35f;
	constexpr float ArcStepDegrees = 3.f;

	FVector2D PointOnCircle(const FVector2D& Center, float Radius, float DegFromTop)
	{
		const float Rad = FMath::DegreesToRadians(DegFromTop);
		return Center + FVector2D(FMath::Sin(Rad), -FMath::Cos(Rad)) * Radius;
	}
}

void SGadgetWheel::Construct(const FArguments& InArgs)
{
	Style = InArgs._Style;
	OnItemSelected = InArgs._OnItemSelected;
	OnItemHovered = InArgs._OnItemHovered;
	SetCanTick(false);
	GothamAccessibility::SetText(SharedThis(this), TAttribute<FText>::CreateSP(this, &SGadgetWheel::GetAccessibleSummary));
}

FText SGadgetWheel::GetAccessibleSummary() const
{
	return Items.IsValidIndex(HoveredIndex)
		? FText::Format(NSLOCTEXT("Gotham.Accessibility", "WheelSelected", "Gadget wheel: {0}"), Items[HoveredIndex].Label)
		: NSLOCTEXT("Gotham.Accessibility", "WheelNone", "Gadget wheel: nothing selected");
}

void SGadgetWheel::SetStyle(const FGothamGadgetWheelStyle& InStyle)
{
	Style = InStyle;
	Invalidate(EInvalidateWidgetReason::LayoutAndVolatility);
}

void SGadgetWheel::SetItems(const TArray<FGothamWheelItem>& InItems)
{
	const bool bCountChanged = InItems.Num() != Items.Num();
	Items = InItems;
	if (bCountChanged)
	{
		HoverAlpha.Init(0.f, Items.Num());
		if (!Items.IsValidIndex(HoveredIndex))
		{
			HoveredIndex = INDEX_NONE;
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SGadgetWheel::ComputeDesiredSize(float) const
{
	const float Diameter = 2.f * (Style.OuterRadius + Style.HoverExpand);
	return FVector2D(Diameter, Diameter);
}

void SGadgetWheel::SetHovered(int32 NewIndex)
{
	if (NewIndex == HoveredIndex)
	{
		return;
	}
	HoveredIndex = NewIndex;
	OnItemHovered.ExecuteIfBound(HoveredIndex);
	EnsureAnimating();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGadgetWheel::EnsureAnimating()
{
	if (bReduceMotion)
	{
		for (int32 i = 0; i < HoverAlpha.Num(); ++i)
		{
			HoverAlpha[i] = (i == HoveredIndex) ? 1.f : 0.f;
		}
		return;
	}
	if (!AnimationTimer.IsValid())
	{
		AnimationTimer = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SGadgetWheel::TickAnimation));
	}
}

EActiveTimerReturnType SGadgetWheel::TickAnimation(double, float InDeltaTime)
{
	bool bSettled = true;
	for (int32 i = 0; i < HoverAlpha.Num(); ++i)
	{
		const float Target = (i == HoveredIndex) ? 1.f : 0.f;
		HoverAlpha[i] = FMath::FInterpTo(HoverAlpha[i], Target, InDeltaTime, AnimationSpeed);
		if (FMath::Abs(HoverAlpha[i] - Target) < 0.005f)
		{
			HoverAlpha[i] = Target;
		}
		else
		{
			bSettled = false;
		}
	}
	Invalidate(EInvalidateWidgetReason::Paint);

	if (bSettled)
	{
		AnimationTimer.Reset();
		return EActiveTimerReturnType::Stop;
	}
	return EActiveTimerReturnType::Continue;
}

FReply SGadgetWheel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const FVector2D Offset = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()) - MyGeometry.GetLocalSize() * 0.5f;
	SetHovered(GothamWheel::IndexFromOffset(Offset, Items.Num(), Style.InnerRadius));
	return FReply::Unhandled();
}

FReply SGadgetWheel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && HoveredIndex != INDEX_NONE)
	{
		OnItemSelected.ExecuteIfBound(HoveredIndex);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply SGadgetWheel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	// Number keys pick a gadget directly, same as outside the wheel.
	const FKey Key = InKeyEvent.GetKey();
	const FKey NumberKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
	for (int32 i = 0; i < UE_ARRAY_COUNT(NumberKeys); ++i)
	{
		if (Key == NumberKeys[i] && Items.IsValidIndex(i))
		{
			OnItemSelected.ExecuteIfBound(i);
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply SGadgetWheel::OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	if (Key == EKeys::Gamepad_LeftX)
	{
		SetStickInput(FVector2D(InAnalogInputEvent.GetAnalogValue(), StickValue.Y));
		return FReply::Handled();
	}
	if (Key == EKeys::Gamepad_LeftY)
	{
		SetStickInput(FVector2D(StickValue.X, InAnalogInputEvent.GetAnalogValue()));
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SGadgetWheel::SetStickInput(const FVector2D& Stick)
{
	StickValue = Stick;
	// Past the dead zone the stick picks a segment; back at centre it keeps the last choice so that
	// releasing the wheel button still commits it.
	if (Stick.Size() >= StickDeadZone)
	{
		SetHovered(GothamWheel::IndexFromOffset(FVector2D(Stick.X, -Stick.Y), Items.Num(), 0.f));
	}
}

bool SGadgetWheel::CommitHovered()
{
	if (HoveredIndex == INDEX_NONE)
	{
		return false;
	}
	OnItemSelected.ExecuteIfBound(HoveredIndex);
	return true;
}

int32 SGadgetWheel::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 Count = Items.Num();
	if (Count == 0)
	{
		return LayerId;
	}

	const FVector2D Center = AllottedGeometry.GetLocalSize() * 0.5f;
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float SegmentSize = 360.f / Count;
	const float HalfSpan = FMath::Max(0.5f, SegmentSize * 0.5f - Style.GapDegrees * 0.5f);

	// Fills a ring sector [R0,R1] x [A0,A1] as a triangle strip.
	auto DrawSector = [&](float R0, float R1, float A0, float A1, const FLinearColor& Color, int32 Layer)
	{
		FLinearColor Final = Color;
		Final.A *= Opacity;
		const FColor Vertex = Final.ToFColor(true);

		const int32 Steps = FMath::Max(1, FMath::CeilToInt((A1 - A0) / ArcStepDegrees));
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;
		Verts.Reserve((Steps + 1) * 2);
		Indices.Reserve(Steps * 6);

		for (int32 s = 0; s <= Steps; ++s)
		{
			const float Angle = FMath::Lerp(A0, A1, static_cast<float>(s) / Steps);
			for (const float Radius : { R1, R0 })
			{
				Verts.AddZeroed();
				FSlateVertex& V = Verts.Last();
				V.Position = FVector2f(AllottedGeometry.LocalToAbsolute(PointOnCircle(Center, Radius, Angle)));
				V.Color = Vertex;
			}
		}
		for (int32 s = 0; s < Steps; ++s)
		{
			const SlateIndex Base = static_cast<SlateIndex>(s * 2);
			Indices.Append({ Base, static_cast<SlateIndex>(Base + 1), static_cast<SlateIndex>(Base + 2),
				static_cast<SlateIndex>(Base + 1), static_cast<SlateIndex>(Base + 3), static_cast<SlateIndex>(Base + 2) });
		}
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, Layer, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);
	};

	auto Alpha = [this](int32 i) { return HoverAlpha.IsValidIndex(i) ? HoverAlpha[i] : 0.f; };

	// Segment bodies, cooldown wedges and outlines each get their own layer so ordering never depends on segment order.
	for (int32 i = 0; i < Count; ++i)
	{
		const FGothamWheelItem& Item = Items[i];
		const float Hover = Alpha(i);
		const float Mid = GothamWheel::SegmentCenterDeg(i, Count);
		const float A0 = Mid - HalfSpan;
		const float A1 = Mid + HalfSpan;
		const float R1 = Style.OuterRadius + Style.HoverExpand * Hover;

		const FLinearColor Base = Style.BackgroundColor;
		const FLinearColor Lit = FLinearColor(Item.Tint.R, Item.Tint.G, Item.Tint.B, 0.95f);
		const FLinearColor Body = FMath::Lerp(FLinearColor(Base.R + Item.Tint.R * 0.12f, Base.G + Item.Tint.G * 0.12f, Base.B + Item.Tint.B * 0.12f, Base.A),
			Lit * 0.75f + FLinearColor(0.f, 0.f, 0.f, 0.2f), Hover);
		DrawSector(Style.InnerRadius, R1, A0, A1, Body, LayerId);

		if (!Item.bReady && Item.CooldownPercent > 0.f)
		{
			const float WedgeOuter = FMath::Lerp(Style.InnerRadius, R1, FMath::Clamp(Item.CooldownPercent, 0.f, 1.f));
			DrawSector(Style.InnerRadius, WedgeOuter, A0, A1, FLinearColor(0.f, 0.f, 0.f, 0.6f), LayerId + 1);
		}

		if (Hover > 0.01f)
		{
			TArray<FVector2f> Outline;
			const int32 Steps = FMath::Max(1, FMath::CeilToInt((A1 - A0) / ArcStepDegrees));
			for (int32 s = 0; s <= Steps; ++s)
			{
				Outline.Add(FVector2f(PointOnCircle(Center, R1, FMath::Lerp(A0, A1, static_cast<float>(s) / Steps))));
			}
			FLinearColor Line = Style.OutlineColor;
			Line.A *= Hover * Opacity;
			FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), Outline,
				ESlateDrawEffect::None, Line, true, 3.f);
		}
	}

	// Labels, plus the hovered gadget's name in the hub.
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	auto DrawCentered = [&](const FText& Text, const FSlateFontInfo& Font, const FVector2D& At, const FLinearColor& Color)
	{
		const FVector2D Size = Measure->Measure(Text, Font);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 3,
			AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(At - Size * 0.5f))),
			Text, Font, ESlateDrawEffect::None, Color);
	};

	for (int32 i = 0; i < Count; ++i)
	{
		const float Radius = (Style.InnerRadius + Style.OuterRadius) * 0.5f + Style.HoverExpand * Alpha(i) * 0.5f;
		FLinearColor TextColor = Items[i].bReady ? FLinearColor::White : FLinearColor(0.7f, 0.7f, 0.7f);
		TextColor.A *= Opacity;
		DrawCentered(Items[i].Label, Style.LabelFont, PointOnCircle(Center, Radius, GothamWheel::SegmentCenterDeg(i, Count)), TextColor);
	}

	if (Items.IsValidIndex(HoveredIndex))
	{
		FSlateFontInfo HubFont = Style.LabelFont;
		HubFont.Size = FMath::RoundToInt(HubFont.Size * 1.3f);
		DrawCentered(Items[HoveredIndex].Label, HubFont, Center, FLinearColor(1.f, 1.f, 1.f, Opacity));
	}

	return LayerId + 3;
}
