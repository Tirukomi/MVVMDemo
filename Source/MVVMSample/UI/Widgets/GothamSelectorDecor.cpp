// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/Widgets/GothamSelectorDecor.h"

#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

class SGothamSelectorDecor : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SGothamSelectorDecor) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&)
	{
		SetCanTick(false);
		SetVisibility(EVisibility::HitTestInvisible);
	}

	void Set(int32 InIndex, int32 InCount, bool bInWraps, const FLinearColor& InActive, const FLinearColor& InIdle, bool bInFocused)
	{
		Index = InIndex;
		Count = FMath::Max(1, InCount);
		bWraps = bInWraps;
		Active = InActive;
		Idle = InIdle;
		bFocused = bInFocused;
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(120.f, 34.f); }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&, FSlateWindowElementList& Out,
		int32 LayerId, const FWidgetStyle& Style, bool) const override
	{
		const FVector2f Size = FVector2f(Geometry.GetLocalSize());
		const float Opacity = Style.GetColorAndOpacityTint().A;
		auto Faded = [Opacity](FLinearColor C, float Scale = 1.f) { C.A *= Opacity * Scale; return C; };

		// Chevrons, vertically centred on the text line (above the pips). Dimmed at a non-wrapping end.
		const float Mid = (Size.Y - 6.f) * 0.5f;
		const float H = 7.f;
		const float W = 5.f;
		const bool bCanPrev = bWraps || Index > 0;
		const bool bCanNext = bWraps || Index < Count - 1;
		const FLinearColor ChevronColor = bFocused ? Active : Idle;
		const float T = bFocused ? 2.f : 1.5f;
		FSlateDrawElement::MakeLines(Out, LayerId, Geometry.ToPaintGeometry(),
			{ FVector2f(6.f + W, Mid - H), FVector2f(6.f, Mid), FVector2f(6.f + W, Mid + H) },
			ESlateDrawEffect::None, Faded(ChevronColor, bCanPrev ? 1.f : 0.25f), true, T);
		FSlateDrawElement::MakeLines(Out, LayerId, Geometry.ToPaintGeometry(),
			{ FVector2f(Size.X - 6.f - W, Mid - H), FVector2f(Size.X - 6.f, Mid), FVector2f(Size.X - 6.f - W, Mid + H) },
			ESlateDrawEffect::None, Faded(ChevronColor, bCanNext ? 1.f : 0.25f), true, T);

		// Pips: one per choice, the current one full width and lit. Skipped for on/off (two pips say nothing).
		if (Count > 2)
		{
			const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox");
			const float Gap = 3.f;
			const float Pip = FMath::Min(14.f, (Size.X - 60.f - Gap * (Count - 1)) / Count);
			const float Total = Pip * Count + Gap * (Count - 1);
			float X = (Size.X - Total) * 0.5f;
			for (int32 i = 0; i < Count; ++i)
			{
				const bool bCurrent = i == Index;
				FSlateDrawElement::MakeBox(Out, LayerId, Geometry.ToPaintGeometry(FVector2f(Pip, bCurrent ? 3.f : 2.f),
					FSlateLayoutTransform(FVector2f(X, Size.Y - (bCurrent ? 4.f : 3.5f)))), White, ESlateDrawEffect::None,
					bCurrent ? Faded(Active) : Faded(Idle, 0.6f));
				X += Pip + Gap;
			}
		}
		return LayerId + 1;
	}

private:
	int32 Index = 0;
	int32 Count = 1;
	bool bWraps = true;
	FLinearColor Active;
	FLinearColor Idle;
	bool bFocused = false;
};

TSharedRef<SWidget> UGothamSelectorDecor::RebuildWidget()
{
	MyDecor = SNew(SGothamSelectorDecor);
	MyDecor->Set(Index, Count, bWraps, Active, Idle, bFocused);
	return MyDecor.ToSharedRef();
}

void UGothamSelectorDecor::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyDecor.Reset();
}

void UGothamSelectorDecor::SetPosition(int32 InIndex, int32 InCount, bool bInWraps)
{
	Index = InIndex;
	Count = InCount;
	bWraps = bInWraps;
	if (MyDecor.IsValid()) { MyDecor->Set(Index, Count, bWraps, Active, Idle, bFocused); }
}

void UGothamSelectorDecor::SetColors(const FLinearColor& InActive, const FLinearColor& InIdle, bool bInFocused)
{
	Active = InActive;
	Idle = InIdle;
	bFocused = bInFocused;
	if (MyDecor.IsValid()) { MyDecor->Set(Index, Count, bWraps, Active, Idle, bFocused); }
}
