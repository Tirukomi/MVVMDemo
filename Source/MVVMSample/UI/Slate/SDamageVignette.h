// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * Screen-edge danger vignette: four gradient strips from the edges inward, drawn with vertex colours. Intensity comes
 * from a damage flash plus a steady level while health is low. Hit-test invisible and paint-only.
 */
class MVVMSAMPLE_API SDamageVignette : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDamageVignette) : _Color(FLinearColor(0.9f, 0.1f, 0.1f)) {}
		SLATE_ARGUMENT(FLinearColor, Color)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetIntensity(float InIntensity);
	void SetColor(const FLinearColor& InColor);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

private:
	FLinearColor Color;
	float Intensity = 0.f;
};
