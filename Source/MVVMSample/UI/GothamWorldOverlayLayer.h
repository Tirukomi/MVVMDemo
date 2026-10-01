// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Slate/SGothamWorldOverlay.h"
#include "GothamWorldOverlayLayer.generated.h"

/**
 * UMG side of a world overlay (see SGothamWorldOverlay): owns the Slate layer, switches it on and off from view-model
 * state, and projects world positions into it with the owning player's view. A derived layer builds its Slate widget
 * with a provider, says when it should be active, and calls UpdateActive when that may have changed.
 */
UCLASS(Abstract)
class MVVMSAMPLE_API UGothamWorldOverlayLayer : public UWidget
{
	GENERATED_BODY()

public:
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override final;

	/** Create the Slate layer (SNew) and give it a provider; MakeProvider wraps a const member safely. */
	virtual TSharedRef<SGothamWorldOverlayBase> MakeOverlay() PURE_VIRTUAL(UGothamWorldOverlayLayer::MakeOverlay, return MakeOverlayPlaceholder(););
	virtual bool ShouldBeActive() const PURE_VIRTUAL(UGothamWorldOverlayLayer::ShouldBeActive, return false;);

	/** Re-evaluates ShouldBeActive; call from the view-model handlers that feed it. */
	void UpdateActive();

	/** World position to layer position (this layer's local space, which fills the HUD canvas). False if behind the camera. */
	bool ProjectToLayer(const FVector& World, FVector2D& OutPosition) const;

	/** The Slate layer, as the type MakeOverlay created (null before RebuildWidget or after release). */
	template <typename TOverlay>
	TOverlay* GetOverlay() const { return static_cast<TOverlay*>(Overlay.Get()); }

	/** A provider that calls Build on this layer, and does nothing once the layer is gone. */
	template <typename TItem, typename TSelf>
	typename SGothamWorldOverlay<TItem>::FProvider MakeProvider(void (TSelf::*Build)(TArray<TItem>&) const) const
	{
		const TWeakObjectPtr<const TSelf> Weak(static_cast<const TSelf*>(this));
		return [Weak, Build](TArray<TItem>& Out)
		{
			if (const TSelf* Self = Weak.Get())
			{
				(Self->*Build)(Out);
			}
		};
	}

private:
	static TSharedRef<SGothamWorldOverlayBase> MakeOverlayPlaceholder();

	TSharedPtr<SGothamWorldOverlayBase> Overlay;
};
