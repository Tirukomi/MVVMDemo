// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "GothamWheelTypes.generated.h"

/** One selectable entry on the radial wheel. */
USTRUCT(BlueprintType)
struct MVVMSAMPLE_API FGothamWheelItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor Tint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bReady = true;

	/** 0 = ready, 1 = just used. Drawn as a darkened wedge that recedes as the gadget recharges. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "1"))
	float CooldownPercent = 0.f;
};

/** Look of the wheel. A data struct so designers restyle it in UMG without touching the Slate widget. */
USTRUCT(BlueprintType)
struct MVVMSAMPLE_API FGothamGadgetWheelStyle
{
	GENERATED_BODY()

	/** Pointer offsets inside this radius select nothing (the centre dead zone). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float InnerRadius = 80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	float OuterRadius = 210.f;

	/** Angular gap between segments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0", ClampMax = "30"))
	float GapDegrees = 3.f;

	/** How far the hovered segment grows outward. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float HoverExpand = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor BackgroundColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.8f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FLinearColor OutlineColor = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSlateFontInfo LabelFont;

	FGothamGadgetWheelStyle()
	{
		LabelFont = FCoreStyle::GetDefaultFontStyle("Bold", 16);
	}
};

/** Wheel geometry helpers. Pure functions, so hit-testing is unit-tested without Slate. */
namespace GothamWheel
{
	/** Clockwise angle in degrees from "up" for a screen-space offset (Y grows downward). Result in [0, 360). */
	inline float AngleFromTopDeg(const FVector2D& Offset)
	{
		float Deg = FMath::RadiansToDegrees(FMath::Atan2(static_cast<float>(Offset.X), static_cast<float>(-Offset.Y)));
		return Deg < 0.f ? Deg + 360.f : Deg;
	}

	/** Centre angle of segment Index. Segment 0 is centred on "up", the rest follow clockwise. */
	inline float SegmentCenterDeg(int32 Index, int32 Count)
	{
		return Count > 0 ? Index * (360.f / Count) : 0.f;
	}

	/** Which segment an offset from the wheel centre falls in, or INDEX_NONE inside the dead zone. */
	inline int32 IndexFromOffset(const FVector2D& Offset, int32 Count, float DeadZoneRadius)
	{
		if (Count <= 0 || Offset.Size() < DeadZoneRadius)
		{
			return INDEX_NONE;
		}
		const float SegmentSize = 360.f / Count;
		const float Shifted = FMath::Fmod(AngleFromTopDeg(Offset) + SegmentSize * 0.5f, 360.f);
		return FMath::FloorToInt(Shifted / SegmentSize) % Count;
	}
}
