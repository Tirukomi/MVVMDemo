// Copyright IG. All Rights Reserved.

#include "UI/MvsWorldOverlayLayer.h"

#include "UI/Style/MvsStyle.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"

namespace MvsWorldOverlayPrivate
{
	/** Stands in for a missing MakeOverlay override: never active, paints nothing. */
	class SEmptyWorldOverlay : public SMvsWorldOverlayBase
	{
	public:
		SLATE_BEGIN_ARGS(SEmptyWorldOverlay) {}
		SLATE_END_ARGS()

		void Construct(const FArguments&) { ConstructOverlay(); }
		virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32 LayerId,
			const FWidgetStyle&, bool) const override { return LayerId; }

	private:
		virtual bool RefreshItems() override { return false; }
		virtual void ClearItems() override {}
	};
}

TSharedRef<SMvsWorldOverlayBase> UMvsWorldOverlayLayer::MakeOverlayPlaceholder()
{
	return SNew(MvsWorldOverlayPrivate::SEmptyWorldOverlay);
}

TSharedRef<SWidget> UMvsWorldOverlayLayer::RebuildWidget()
{
	Overlay = MakeOverlay();
	SettingsListener.Bind(this, [this](const FMvsSettingsData& Data) { ApplyTheme(FMvsTheme::FromSettings(Data)); });
	ApplyTheme(MvsStyle::Theme(this));
	UpdateActive();
	return Overlay.ToSharedRef();
}

void UMvsWorldOverlayLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	SettingsListener.Reset();
	Super::ReleaseSlateResources(bReleaseChildren);
	Overlay.Reset();
}

void UMvsWorldOverlayLayer::UpdateActive()
{
	if (Overlay.IsValid())
	{
		Overlay->SetActive(ShouldBeActive());
	}
}

bool UMvsWorldOverlayLayer::ProjectToLayer(const FVector& World, FVector2D& OutPosition) const
{
	// Screen pixels, then into this layer's own space: correct however the UI above it is scaled (the primary layout
	// applies the player's UI scale itself, which the viewport's DPI scale does not include).
	APlayerController* PC = GetOwningPlayer();
	FVector2D ScreenPosition;
	if (!PC || !PC->ProjectWorldLocationToScreen(World, ScreenPosition, true))
	{
		return false;
	}
	USlateBlueprintLibrary::ScreenToWidgetLocal(this, GetCachedGeometry(), ScreenPosition, OutPosition);
	return true;
}
