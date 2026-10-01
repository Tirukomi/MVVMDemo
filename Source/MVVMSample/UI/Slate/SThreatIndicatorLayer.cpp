// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Slate/SThreatIndicatorLayer.h"
#include "UI/GothamAccessibility.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Gameplay/ThreatTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "UI/Slate/SGothamPanel.h"
#include "UI/Style/GothamStyle.h"

namespace
{
	constexpr float EdgeInset = 48.f;
	constexpr float OnScreenMargin = 24.f;
}

void SThreatIndicatorLayer::Construct(const FArguments& InArgs)
{
	ConstructOverlay();
	GothamAccessibility::SetText(SharedThis(this), TAttribute<FText>::CreateSP(this, &SThreatIndicatorLayer::GetAccessibleSummary));
}

FText SThreatIndicatorLayer::GetAccessibleSummary() const
{
	int32 Warnings = 0;
	for (const FGothamThreatIndicator& Indicator : GetItems())
	{
		Warnings += Indicator.bWarning ? 1 : 0;
	}
	return Warnings > 0
		? FText::Format(NSLOCTEXT("Gotham.Accessibility", "ThreatWarning", "{0} {0}|plural(one=attack,other=attacks) incoming: counter now"), Warnings)
		: FText::Format(NSLOCTEXT("Gotham.Accessibility", "Threats", "{0} {0}|plural(one=enemy,other=enemies) nearby"), GetItems().Num());
}

void SThreatIndicatorLayer::SetColors(const FLinearColor& InDanger, const FLinearColor& InIdle, const FLinearColor& InPanel, const FLinearColor& InText)
{
	Danger = InDanger;
	Idle = InIdle;
	Panel = InPanel;
	Text = InText;
	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SThreatIndicatorLayer::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const float Opacity = InWidgetStyle.GetColorAndOpacityTint().A;
	for (const FGothamThreatIndicator& Threat : GetItems())
	{
		const bool bOnScreen = Threat.bProjected && GothamThreat::IsOnScreen(Threat.Screen, Size, OnScreenMargin);
		if (bOnScreen)
		{
			if (Threat.bWarning)
			{
				PaintPrompt(OutDrawElements, LayerId, AllottedGeometry, Threat, Opacity * Threat.Opacity);
			}
		}
		else
		{
			PaintArrow(OutDrawElements, LayerId, AllottedGeometry, Threat, Opacity * Threat.Opacity);
		}
	}
	return LayerId + 2;
}

void SThreatIndicatorLayer::PaintPrompt(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FGothamThreatIndicator& Threat, float Opacity) const
{
	auto Faded = [Opacity](FLinearColor C, float Scale = 1.f) { C.A *= Opacity * Scale; return C; };
	const FVector2f Anchor = FVector2f(Threat.Screen);

	// Three alert strokes fanning up from the head. They pulse unless motion is reduced; colour and shape carry the
	// warning either way, and the key below says what to press.
	const float Pulse = bReducedMotion ? 1.f : 1.f + 0.18f * FMath::Sin(static_cast<float>(GetRefreshTime()) * 18.f);
	const float Length = 24.f * Pulse;
	for (const float Degrees : { -34.f, 0.f, 34.f })
	{
		const float A = FMath::DegreesToRadians(Degrees);
		const FVector2f Dir(FMath::Sin(A), -FMath::Cos(A));
		const FVector2f Start = Anchor + Dir * 14.f + FVector2f(0.f, -34.f);
		FSlateDrawElement::MakeLines(Out, LayerId + 1, Geometry.ToPaintGeometry(), { Start, Start + Dir * Length },
			ESlateDrawEffect::None, Faded(Danger), true, 4.f);
	}

	// Key cap with the Counter binding, and a bar under it that empties as the window closes.
	const FSlateFontInfo Font = GothamStyle::Font(EGothamTextStyle::Numeric);
	const FVector2f TextSize = FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(KeyLabel, Font));
	const FVector2f CapSize(FMath::Max(TextSize.X + 22.f, 44.f), TextSize.Y + 6.f);
	const FVector2f CapPos = Anchor + FVector2f(-CapSize.X * 0.5f, -22.f);
	FGothamPanelLook Cap;
	Cap.Corner = 6.f;
	Cap.ChamferMask = EGothamChamfer::Opposite;
	Cap.Fill = Faded(Panel, 0.9f);
	Cap.Edge = Faded(Danger);
	Cap.EdgeThickness = 1.5f;
	Cap.Glow = Faded(Danger, 0.35f);
	Cap.GlowSize = 5.f;
	GothamPaintPanel(Out, LayerId, Geometry, CapPos, CapSize, Cap, 1.f);
	FSlateDrawElement::MakeText(Out, LayerId + 1,
		Geometry.ToPaintGeometry(TextSize, FSlateLayoutTransform(CapPos + (CapSize - TextSize) * 0.5f)), KeyLabel, Font, ESlateDrawEffect::None, Faded(Text));

	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	const float Remaining = 1.f - FMath::Clamp(Threat.Progress, 0.f, 1.f);
	const FVector2f BarPos = CapPos + FVector2f(0.f, CapSize.Y + 5.f);
	FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2f(CapSize.X, 4.f), FSlateLayoutTransform(BarPos)),
		White, ESlateDrawEffect::None, Faded(Idle, 0.35f));
	FSlateDrawElement::MakeBox(Out, LayerId + 1, Geometry.ToPaintGeometry(FVector2f(CapSize.X * Remaining, 4.f), FSlateLayoutTransform(BarPos)),
		White, ESlateDrawEffect::None, Faded(Danger));
}

void SThreatIndicatorLayer::PaintArrow(FSlateWindowElementList& Out, int32 LayerId, const FGeometry& Geometry, const FGothamThreatIndicator& Threat, float Opacity) const
{
	FVector2D Position;
	float Angle = 0.f;
	GothamThreat::EdgeArrow(Threat.ViewDirection, Geometry.GetLocalSize(), EdgeInset, Position, Angle);

	const float Pulse = (!Threat.bWarning || bReducedMotion) ? 1.f : 1.f + 0.2f * FMath::Sin(static_cast<float>(GetRefreshTime()) * 16.f);
	const float Size = (Threat.bWarning ? 28.f : 14.f) * Pulse;
	FLinearColor Color = Threat.bWarning ? Danger : Idle;
	Color.A *= Opacity * (Threat.bWarning ? 1.f : 0.7f);

	// A filled arrowhead pointing at the threat, with a notch so it reads as an arrow rather than a triangle.
	const FVector2f Forward(FMath::Cos(Angle), FMath::Sin(Angle));
	const FVector2f Side(-Forward.Y, Forward.X);
	const FVector2f C = FVector2f(Position);
	const FVector2f Tip = C + Forward * Size;
	const FVector2f Left = C - Forward * Size * 0.6f + Side * Size * 0.8f;
	const FVector2f Notch = C - Forward * Size * 0.2f;
	const FVector2f Right = C - Forward * Size * 0.6f - Side * Size * 0.8f;

	const FColor Vertex = Color.ToFColor(true);
	TArray<FSlateVertex> Verts;
	Verts.AddZeroed(4);
	const FVector2f Points[] = { Tip, Left, Notch, Right };
	for (int32 i = 0; i < 4; ++i)
	{
		Verts[i].Position = FVector2f(Geometry.LocalToAbsolute(FVector2D(Points[i])));
		Verts[i].Color = Vertex;
	}
	TArray<SlateIndex> Indices = { 0, 1, 2, 0, 2, 3 };
	const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
	FSlateDrawElement::MakeCustomVerts(Out, LayerId, White->GetRenderingResource(), Verts, Indices, nullptr, 0, 0);

	if (Threat.bWarning)
	{
		// A second chevron behind the first: "incoming", readable without the colour.
		const FVector2f Back = C - Forward * Size * 0.9f;
		FSlateDrawElement::MakeLines(Out, LayerId + 1, Geometry.ToPaintGeometry(),
			{ Back - Forward * Size * 0.5f + Side * Size * 0.7f, Back, Back - Forward * Size * 0.5f - Side * Size * 0.7f },
			ESlateDrawEffect::None, Color, true, 2.5f);
	}
}
