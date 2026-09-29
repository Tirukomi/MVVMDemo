// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"
#include "UI/Slate/GothamWheelTypes.h"

DECLARE_DELEGATE_OneParam(FOnWheelIndex, int32 /*ItemIndex*/);

/**
 * Radial gadget wheel drawn directly with custom vertices (no image assets).
 * Hit-testing is angle math shared with the tests; hover animation runs on an active timer that
 * unregisters itself once every segment has settled, so an idle wheel costs nothing per frame.
 */
class MVVMSAMPLE_API SGadgetWheel : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGadgetWheel)
		: _Style()
	{}
		SLATE_ARGUMENT(FGothamGadgetWheelStyle, Style)
		/** Fired when an item is chosen by click or number key, or by CommitHovered(). */
		SLATE_EVENT(FOnWheelIndex, OnItemSelected)
		SLATE_EVENT(FOnWheelIndex, OnItemHovered)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SetItems(const TArray<FGothamWheelItem>& InItems);
	void SetStyle(const FGothamGadgetWheelStyle& InStyle);

	/** Analog stick input, +Y up as gamepads report it. Selects by direction once past the dead zone. */
	void SetStickInput(const FVector2D& Stick);

	int32 GetHoveredIndex() const { return HoveredIndex; }

	/** Fires OnItemSelected for the hovered segment. Returns false if nothing is hovered. */
	bool CommitHovered();

	//~ SWidget
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnAnalogValueChanged(const FGeometry& MyGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;

private:
	void SetHovered(int32 NewIndex);
	EActiveTimerReturnType TickAnimation(double InCurrentTime, float InDeltaTime);
	void EnsureAnimating();

	FGothamGadgetWheelStyle Style;
	TArray<FGothamWheelItem> Items;

	int32 HoveredIndex = INDEX_NONE;
	TArray<float> HoverAlpha;
	FVector2D StickValue = FVector2D::ZeroVector;

	FOnWheelIndex OnItemSelected;
	FOnWheelIndex OnItemHovered;
	TSharedPtr<FActiveTimerHandle> AnimationTimer;
};
