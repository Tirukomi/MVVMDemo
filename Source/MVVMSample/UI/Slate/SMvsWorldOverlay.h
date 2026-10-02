// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * A full-screen layer that draws world-anchored items (clue markers, threat indicators) in one paint pass, with no
 * widget per item. While active, an active timer asks for fresh items every frame (they move with the camera) and
 * repaints when they changed or the layer is animating (second review 29: it used to repaint every frame); while
 * inactive the timer is unregistered and the layer paints nothing, so it costs nothing. It never ticks and never
 * takes input.
 *
 * The non-template part, so owners can switch it on and off without knowing the item type.
 */
class MVVMSAMPLE_API SMvsWorldOverlayBase : public SLeafWidget
{
public:
	void SetActive(bool bInActive);
	bool IsActive() const { return Timer.IsValid(); }
	/** What the active timer does each frame: fetch the items, and repaint if they changed or the layer animates.
	 *  Returns whether it repainted (tests drive it directly). */
	bool Refresh(double InCurrentTime);

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

protected:
	/** Call from the derived widget's Construct. */
	void ConstructOverlay();

	/** Slate time of the last refresh, for pulses and other animation in OnPaint. */
	double GetRefreshTime() const { return RefreshTime; }

	/** Fetches this frame's items. Returns true if they differ from last frame's (the layer must repaint). */
	virtual bool RefreshItems() = 0;
	virtual void ClearItems() = 0;
	/** True while the layer's paint changes with time alone (a pulse), so it repaints with unchanged items too. */
	virtual bool IsAnimating() const { return false; }

private:
	EActiveTimerReturnType OnRefreshTimer(double InCurrentTime, float InDeltaTime);

	TSharedPtr<FActiveTimerHandle> Timer;
	double RefreshTime = 0.0;
};

/** The typed part: a provider fills this frame's items, and OnPaint draws GetItems(). TItem has an operator==. */
template <typename TItem>
class SMvsWorldOverlay : public SMvsWorldOverlayBase
{
public:
	using FProvider = TFunction<void(TArray<TItem>& OutItems)>;

	void SetProvider(FProvider InProvider) { Provider = MoveTemp(InProvider); }

protected:
	const TArray<TItem>& GetItems() const { return Items; }

private:
	virtual bool RefreshItems() override
	{
		Fresh.Reset();
		if (Provider)
		{
			Provider(Fresh);
		}
		const bool bChanged = Fresh != Items;
		Swap(Items, Fresh);
		return bChanged;
	}
	virtual void ClearItems() override { Items.Reset(); }

	FProvider Provider;
	TArray<TItem> Items;
	/** Last frame's items while comparing, kept to reuse its allocation. */
	TArray<TItem> Fresh;
};
