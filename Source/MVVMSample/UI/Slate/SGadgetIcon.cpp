// Copyright IG. All Rights Reserved.

#include "UI/Slate/SGadgetIcon.h"

#include "Rendering/DrawElements.h"

namespace
{
	TArray<FVector2f> Arc(const FVector2f& Centre, float Radius, float FromDeg, float ToDeg, int32 Steps)
	{
		TArray<FVector2f> Points;
		Points.Reserve(Steps + 1);
		for (int32 i = 0; i <= Steps; ++i)
		{
			const float A = FMath::DegreesToRadians(FMath::Lerp(FromDeg, ToDeg, static_cast<float>(i) / Steps));
			Points.Add(Centre + FVector2f(FMath::Sin(A), -FMath::Cos(A)) * Radius);
		}
		return Points;
	}
}

namespace
{
	TArray<TArray<FVector2f>> BuildIconStrokes(EMvsGadgetIcon Icon)
	{
		TArray<TArray<FVector2f>> S;
		switch (Icon)
		{
		case EMvsGadgetIcon::Glaive:
			// A plain chevron throwing blade with a centre rivet, drawn from scratch: no wings or scallops, nothing
			// that resembles an existing logo.
			S.Add({ { -0.85f, 0.45f }, { 0.f, -0.55f }, { 0.85f, 0.45f }, { 0.5f, 0.5f }, { 0.f, -0.02f }, { -0.5f, 0.5f }, { -0.85f, 0.45f } });
			S.Add({ { 0.f, 0.16f }, { 0.1f, 0.26f }, { 0.f, 0.36f }, { -0.1f, 0.26f }, { 0.f, 0.16f } });
			break;
		case EMvsGadgetIcon::Grapple:
			// A launcher body with a line running to a three-pronged hook.
			S.Add({ { -0.8f, 0.75f }, { -0.45f, 0.4f }, { -0.3f, 0.55f }, { -0.65f, 0.9f }, { -0.8f, 0.75f } });
			S.Add({ { -0.38f, 0.48f }, { 0.35f, -0.25f } });
			S.Add({ { 0.35f, -0.25f }, { 0.75f, -0.65f } });
			S.Add({ { 0.35f, -0.25f }, { 0.3f, -0.75f }, { 0.45f, -0.85f } });
			S.Add({ { 0.35f, -0.25f }, { 0.85f, -0.3f }, { 0.95f, -0.15f } });
			break;
		case EMvsGadgetIcon::Smoke:
			// A canister with a cloud rising from it.
			S.Add({ { -0.25f, 0.85f }, { -0.25f, 0.25f }, { 0.25f, 0.25f }, { 0.25f, 0.85f }, { -0.25f, 0.85f } });
			S.Add({ { -0.3f, 0.25f }, { 0.3f, 0.25f } });
			S.Add(Arc({ -0.35f, -0.15f }, 0.3f, 200.f, 380.f, 10));
			S.Add(Arc({ 0.05f, -0.45f }, 0.35f, 250.f, 470.f, 12));
			S.Add(Arc({ 0.42f, -0.12f }, 0.28f, 330.f, 520.f, 10));
			break;
		default:
			break;
		}
		return S;
	}
}

const TArray<TArray<FVector2f>>& MvsGadgetIconStrokes(EMvsGadgetIcon Icon)
{
	// Built once: the strokes are fixed shapes in unit space, scaled when painted.
	static const TArray<TArray<TArray<FVector2f>>> Table = []()
	{
		TArray<TArray<TArray<FVector2f>>> All;
		for (int32 i = 0; i < static_cast<int32>(EMvsGadgetIcon::Count); ++i)
		{
			All.Add(BuildIconStrokes(static_cast<EMvsGadgetIcon>(i)));
		}
		return All;
	}();
	static const TArray<TArray<FVector2f>> None;
	return Table.IsValidIndex(static_cast<int32>(Icon)) ? Table[static_cast<int32>(Icon)] : None;
}

void SGadgetIcon::Construct(const FArguments& InArgs)
{
	Icon = InArgs._Icon;
	Size = InArgs._Size;
	Color = InArgs._Color;
	RingColor = InArgs._RingColor;
	SetCanTick(false);
}

void SGadgetIcon::SetIcon(EMvsGadgetIcon InIcon)
{
	Icon = InIcon;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGadgetIcon::SetSize(float InSize)
{
	Size = InSize;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SGadgetIcon::SetColors(const FLinearColor& InColor, const FLinearColor& InRingColor)
{
	Color = InColor;
	RingColor = InRingColor;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SGadgetIcon::SetCooldown(float InPercent)
{
	const float Clamped = FMath::Clamp(InPercent, 0.f, 1.f);
	if (!FMath::IsNearlyEqual(Clamped, Cooldown, 0.002f))
	{
		Cooldown = Clamped;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

int32 SGadgetIcon::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f LocalSize = FVector2f(AllottedGeometry.GetLocalSize());
	const FVector2f Centre = LocalSize * 0.5f;
	const float Extent = 0.5f * FMath::Min(LocalSize.X, LocalSize.Y);
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	const float Stroke = FMath::Max(1.5f, Extent * 0.07f);
	const bool bReady = Cooldown <= 0.f;

	auto Tinted = [Opacity](FLinearColor C, float Alpha = 1.f) { C.A *= Opacity * Alpha; return C; };
	auto Draw = [&](TArray<FVector2f> Points, const FLinearColor& C, float Thickness, int32 Layer)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, C, true, Thickness);
	};

	// Ring: a dim full circle, then the recharged portion (clockwise from the top).
	const float RingRadius = Extent - Stroke;
	Draw(Arc(Centre, RingRadius, 0.f, 360.f, 48), Tinted(RingColor), Stroke * 0.6f, LayerId);
	const float Recharged = 1.f - Cooldown;
	if (Recharged > 0.f)
	{
		Draw(Arc(Centre, RingRadius, 0.f, 360.f * Recharged, FMath::Max(2, FMath::CeilToInt(48 * Recharged))),
			Tinted(Color, bReady ? 1.f : 0.55f), Stroke, LayerId + 1);
	}

	// Icon strokes, dimmed while recharging.
	const float IconScale = Extent * 0.55f;
	for (const TArray<FVector2f>& Line : MvsGadgetIconStrokes(Icon))
	{
		TArray<FVector2f> Points;
		Points.Reserve(Line.Num());
		for (const FVector2f& P : Line)
		{
			Points.Add(Centre + P * IconScale);
		}
		Draw(MoveTemp(Points), Tinted(Color, bReady ? 1.f : 0.4f), Stroke, LayerId + 1);
	}
	return LayerId + 1;
}
