// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/** Original line-art gadget icons (no textures, no borrowed iconography). */
enum class EGothamGadgetIcon : uint8
{
	WingBlade,
	Grapple,
	Smoke,
	Count
};

/** Icon strokes for a unit box ([-1,1] in both axes, +Y down). Each inner array is one open polyline. Built once. */
MVVMSAMPLE_API const TArray<TArray<FVector2f>>& GothamGadgetIconStrokes(EGothamGadgetIcon Icon);

/**
 * A gadget icon inside a cooldown ring. The ring is dim while recharging and fills clockwise from the top as the
 * gadget comes back; when ready the ring is solid in the icon colour.
 */
class MVVMSAMPLE_API SGadgetIcon : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGadgetIcon)
		: _Icon(EGothamGadgetIcon::WingBlade)
		, _Size(64.f)
		, _Color(FLinearColor::White)
		, _RingColor(FLinearColor(1.f, 1.f, 1.f, 0.2f))
	{}
		SLATE_ARGUMENT(EGothamGadgetIcon, Icon)
		SLATE_ARGUMENT(float, Size)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(FLinearColor, RingColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetIcon(EGothamGadgetIcon InIcon);
	void SetSize(float InSize);
	void SetColors(const FLinearColor& InColor, const FLinearColor& InRingColor);
	/** 0 = ready, 1 = just used. */
	void SetCooldown(float InPercent);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(Size, Size); }

private:
	EGothamGadgetIcon Icon = EGothamGadgetIcon::WingBlade;
	float Size = 64.f;
	FLinearColor Color;
	FLinearColor RingColor;
	float Cooldown = 0.f;
};
