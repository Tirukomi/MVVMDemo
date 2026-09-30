// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

/**
 * Fluent slot setup for code-built layouts, instead of three lines of "add child, set padding, set alignment":
 *
 *     GothamLayout::Add(Row, Label).Fill().VCenter().Pad(0.f, 0.f, 16.f, 0.f);
 *     GothamLayout::Add(Stack, Value).Center().Pad(24.f, 2.f);
 *
 * Works for horizontal boxes, vertical boxes and overlays; size methods exist only where the slot has a size.
 * Adopted file by file as code is touched for other reasons (see RefactoringPlan P10); there is no bulk conversion.
 */
namespace GothamLayout
{
	template <typename TSlot>
	struct TSlotRef
	{
		TSlot* Slot;

		TSlotRef& Pad(float Uniform) { Slot->SetPadding(FMargin(Uniform)); return *this; }
		TSlotRef& Pad(float Horizontal, float Vertical) { Slot->SetPadding(FMargin(Horizontal, Vertical)); return *this; }
		TSlotRef& Pad(float Left, float Top, float Right, float Bottom) { Slot->SetPadding(FMargin(Left, Top, Right, Bottom)); return *this; }
		TSlotRef& HAlign(EHorizontalAlignment Alignment) { Slot->SetHorizontalAlignment(Alignment); return *this; }
		TSlotRef& VAlign(EVerticalAlignment Alignment) { Slot->SetVerticalAlignment(Alignment); return *this; }
		TSlotRef& HCenter() { return HAlign(HAlign_Center); }
		TSlotRef& VCenter() { return VAlign(VAlign_Center); }
		TSlotRef& Center() { return HCenter().VCenter(); }
		TSlotRef& FillBoth() { return HAlign(HAlign_Fill).VAlign(VAlign_Fill); }

		/** Box slots only: take the remaining space (with a weight) or only what the content asks for. */
		TSlotRef& Fill(float Weight = 1.f) { Slot->SetSize(Sized(Weight)); return *this; }
		TSlotRef& Auto() { Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic)); return *this; }

		operator TSlot*() const { return Slot; }

	private:
		static FSlateChildSize Sized(float Weight)
		{
			FSlateChildSize Size(ESlateSizeRule::Fill);
			Size.Value = Weight;
			return Size;
		}
	};

	inline TSlotRef<UHorizontalBoxSlot> Add(UHorizontalBox* Box, UWidget* Child) { return { Box->AddChildToHorizontalBox(Child) }; }
	inline TSlotRef<UVerticalBoxSlot> Add(UVerticalBox* Box, UWidget* Child) { return { Box->AddChildToVerticalBox(Child) }; }
	inline TSlotRef<UOverlaySlot> Add(UOverlay* Box, UWidget* Child) { return { Box->AddChildToOverlay(Child) }; }
}
