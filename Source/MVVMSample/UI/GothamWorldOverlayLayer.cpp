// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/GothamWorldOverlayLayer.h"

#include "Blueprint/WidgetLayoutLibrary.h"

namespace GothamWorldOverlayPrivate
{
	/** Stands in for a missing MakeOverlay override: never active, paints nothing. */
	class SEmptyWorldOverlay : public SGothamWorldOverlayBase
	{
	public:
		SLATE_BEGIN_ARGS(SEmptyWorldOverlay) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&) { ConstructOverlay(); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32 LayerId,
			const FWidgetStyle&, bool) const override { return LayerId; }

	private:
		virtual void RefreshItems() override {}
		virtual void ClearItems() override {}
	};
}

TSharedRef<SGothamWorldOverlayBase> UGothamWorldOverlayLayer::MakeOverlayPlaceholder()
{
	return SNew(GothamWorldOverlayPrivate::SEmptyWorldOverlay);
}

TSharedRef<SWidget> UGothamWorldOverlayLayer::RebuildWidget()
{
	Overlay = MakeOverlay();
	UpdateActive();
	return Overlay.ToSharedRef();
}

void UGothamWorldOverlayLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Overlay.Reset();
}

void UGothamWorldOverlayLayer::UpdateActive()
{
	if (Overlay.IsValid())
	{
		Overlay->SetActive(ShouldBeActive());
	}
}

bool UGothamWorldOverlayLayer::ProjectToLayer(const FVector& World, FVector2D& OutPosition) const
{
	APlayerController* PC = GetOwningPlayer();
	return PC && UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, World, OutPosition, true);
}
