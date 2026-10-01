// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SClueMarkerLayer.h"

#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UI/Style/GothamStyle.h"

namespace GothamMarkers
{
	float ScaleForDistance(float DistanceCm)
	{
		return FMath::Clamp(1200.f / FMath::Max(DistanceCm, 1.f), 0.6f, 2.f);
	}

	float OpacityForDistance(float DistanceCm, float MaxDistanceCm)
	{
		const float FadeStart = MaxDistanceCm * 0.7f;
		if (DistanceCm <= FadeStart)
		{
			return 1.f;
		}
		return FMath::Clamp(1.f - (DistanceCm - FadeStart) / FMath::Max(MaxDistanceCm - FadeStart, 1.f), 0.f, 1.f);
	}
}

void SClueMarkerLayer::Construct(const FArguments& InArgs)
{
	ConstructOverlay();
}

void SClueMarkerLayer::SetColors(const FLinearColor& InUnknown, const FLinearColor& InKnown, const FLinearColor& InAnalysing, const FLinearColor& InMuted)
{
	UnknownColor = InUnknown;
	KnownColor = InKnown;
	AnalysingColor = InAnalysing;
	MutedColor = InMuted;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SClueMarkerLayer::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const TArray<FGothamClueMarker>& Markers = GetItems();
	if (Markers.IsEmpty())
	{
		return LayerId;
	}
	const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FSlateFontInfo LabelFont = GothamStyle::Font(EGothamTextStyle::Label);
	const FSlateFontInfo DistanceFont = GothamStyle::Font(EGothamTextStyle::Key);
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;

	auto Lines = [&](TArray<FVector2f> Points, const FLinearColor& Color, float Thickness)
	{
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Color, true, Thickness);
	};
	auto Text = [&](const FText& InText, const FSlateFontInfo& Font, const FVector2f& At, const FLinearColor& Color)
	{
		const FVector2D Size = Measure->Measure(InText, Font);
		FSlateDrawElement::MakeText(OutDrawElements, LayerId + 1,
			AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(At)), InText, Font, ESlateDrawEffect::None, Color);
		return FVector2f(Size);
	};

	for (const FGothamClueMarker& M : Markers)
	{
		FLinearColor Color = M.State == FGothamClueMarker::EState::Analysing ? AnalysingColor
			: M.State == FGothamClueMarker::EState::Known ? KnownColor : UnknownColor;
		Color.A *= M.Opacity * Opacity;
		FLinearColor Muted = MutedColor;
		Muted.A *= M.Opacity * Opacity;

		const FVector2f C = FVector2f(M.Position);
		const float H = 26.f * M.Scale;       // bracket half-size: always wider than a clue at that distance
		const float L = H * 0.5f;             // corner arm length
		const float T = FMath::Max(1.5f, 2.f * M.Scale);

		// Four corner brackets.
		for (const FVector2f& Corner : { FVector2f(-1, -1), FVector2f(1, -1), FVector2f(1, 1), FVector2f(-1, 1) })
		{
			const FVector2f P = C + Corner * H;
			Lines({ P - FVector2f(Corner.X * L, 0.f), P, P - FVector2f(0.f, Corner.Y * L) }, Color, T);
		}
		// Centre tick.
		Lines({ C - FVector2f(3.f, 0.f), C + FVector2f(3.f, 0.f) }, Color, T);

		if (M.State == FGothamClueMarker::EState::Analysing && M.Progress > 0.f)
		{
			// Progress arc just outside the brackets, clockwise from the top.
			const float R = H * 1.55f;
			const int32 Steps = FMath::Max(3, FMath::CeilToInt(40 * M.Progress));
			TArray<FVector2f> Arc;
			Arc.Reserve(Steps + 1);
			for (int32 i = 0; i <= Steps; ++i)
			{
				const float A = FMath::DegreesToRadians(360.f * M.Progress * i / Steps);
				Arc.Add(C + FVector2f(FMath::Sin(A), -FMath::Cos(A)) * R);
			}
			Lines(MoveTemp(Arc), Color, T + 1.f);
		}

		// Label and distance to the right of the bracket, joined by a short rule.
		// Clear of the analysis arc (radius 1.55 H) so the label never sits under it.
		const FVector2f LabelAt = C + FVector2f(H * 1.7f + 10.f, -H);
		Lines({ C + FVector2f(H, -H), LabelAt + FVector2f(-4.f, 0.f) }, Muted, 1.f);
		const FVector2f LabelSize = Text(M.Label, LabelFont, LabelAt + FVector2f(0.f, -8.f), Color);
		Text(M.Distance, DistanceFont, LabelAt + FVector2f(0.f, LabelSize.Y - 6.f), Muted);
	}
	return LayerId + 1;
}
