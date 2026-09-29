// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/** Which corners are cut. Combine with |. */
namespace EGothamChamfer
{
	enum : uint8
	{
		None = 0,
		TopLeft = 1 << 0,
		TopRight = 1 << 1,
		BottomRight = 1 << 2,
		BottomLeft = 1 << 3,
		Opposite = TopRight | BottomLeft,
		All = TopLeft | TopRight | BottomRight | BottomLeft,
	};
}

/** Corner points of a chamfered rectangle, clockwise from the top-left. Pure, so the shape is unit-testable. */
MVVMSAMPLE_API TArray<FVector2f> GothamChamferedRect(const FVector2f& Size, float Corner, uint8 ChamferMask);

/** Everything that describes how a panel is drawn. Shared by SGothamPanel and the menu highlight bar. */
struct FGothamPanelLook
{
	float Corner = 10.f;
	uint8 ChamferMask = EGothamChamfer::Opposite;
	FLinearColor Fill = FLinearColor::Transparent;
	FLinearColor Edge = FLinearColor::Transparent;
	float EdgeThickness = 1.f;
	FLinearColor Accent = FLinearColor::Transparent;
	float AccentWidth = 0.f;
	/** Soft outer glow (focus). Zero size draws none. */
	FLinearColor Glow = FLinearColor::Transparent;
	float GlowSize = 0.f;
};

/** Paints a panel at Offset/Size inside Geometry. Uses LayerId (glow, fill) and LayerId + 1 (accent, edge). */
MVVMSAMPLE_API void GothamPaintPanel(FSlateWindowElementList& OutDrawElements, int32 LayerId, const FGeometry& Geometry,
	const FVector2f& Offset, const FVector2f& Size, const FGothamPanelLook& Look, float Opacity);

/**
 * The HUD's panel shape: a chamfered rectangle with a fill, a thin edge and an optional accent bar on the left,
 * drawn with custom vertices (no textures, crisp at any UI scale). Content is laid out inside Padding.
 */
class MVVMSAMPLE_API SGothamPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SGothamPanel)
		: _Padding(FMargin(12.f, 8.f))
		, _Corner(10.f)
		, _ChamferMask(EGothamChamfer::Opposite)
		, _FillColor(FLinearColor(0.012f, 0.015f, 0.02f, 0.8f))
		, _EdgeColor(FLinearColor(0.32f, 0.38f, 0.46f, 0.9f))
		, _EdgeThickness(1.f)
		, _AccentColor(FLinearColor::Transparent)
		, _AccentWidth(0.f)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_ARGUMENT(FMargin, Padding)
		SLATE_ARGUMENT(float, Corner)
		SLATE_ARGUMENT(uint8, ChamferMask)
		SLATE_ARGUMENT(FLinearColor, FillColor)
		SLATE_ARGUMENT(FLinearColor, EdgeColor)
		SLATE_ARGUMENT(float, EdgeThickness)
		SLATE_ARGUMENT(FLinearColor, AccentColor)
		SLATE_ARGUMENT(float, AccentWidth)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetContent(const TSharedRef<SWidget>& InContent);
	void SetPadding(const FMargin& InPadding);
	void SetShape(float InCorner, uint8 InChamferMask);
	void SetColors(const FLinearColor& InFill, const FLinearColor& InEdge, float InEdgeThickness);
	void SetAccent(const FLinearColor& InColor, float InWidth);
	void SetGlow(const FLinearColor& InColor, float InSize);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	TSharedPtr<class SBox> ContentBox;
	float Corner = 10.f;
	uint8 ChamferMask = EGothamChamfer::Opposite;
	FLinearColor FillColor;
	FLinearColor EdgeColor;
	float EdgeThickness = 1.f;
	FLinearColor AccentColor;
	float AccentWidth = 0.f;
	FLinearColor GlowColor = FLinearColor::Transparent;
	float GlowSize = 0.f;
};
