// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SLeafWidget.h"

/**
 * A full-screen layer that draws world-anchored items (clue markers, threat indicators) in one paint pass, with no
 * widget per item. While active, an active timer asks for fresh items every frame (they move with the camera) and
 * repaints; while inactive the timer is unregistered and the layer paints nothing, so it costs nothing. It never
 * ticks and never takes input.
 *
 * The non-template part, so owners can switch it on and off without knowing the item type.
 */
class MVVMSAMPLE_API SMvsWorldOverlayBase : public SLeafWidget
{
public:
	void SetActive(bool bInActive);
	bool IsActive() const { return Timer.IsValid(); }

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

protected:
	/** Call from the derived widget's Construct. */
	void ConstructOverlay();

	/** Slate time of the last refresh, for pulses and other animation in OnPaint. */
	double GetRefreshTime() const { return RefreshTime; }

	virtual void RefreshItems() = 0;
	virtual void ClearItems() = 0;

private:
	EActiveTimerReturnType OnRefreshTimer(double InCurrentTime, float InDeltaTime);

	TSharedPtr<FActiveTimerHandle> Timer;
	double RefreshTime = 0.0;
};

/** The typed part: a provider fills this frame's items, and OnPaint draws GetItems(). */
template <typename TItem>
class SMvsWorldOverlay : public SMvsWorldOverlayBase
{
public:
	using FProvider = TFunction<void(TArray<TItem>& OutItems)>;

	void SetProvider(FProvider InProvider) { Provider = MoveTemp(InProvider); }

protected:
	const TArray<TItem>& GetItems() const { return Items; }

private:
	virtual void RefreshItems() override
	{
		Items.Reset();
		if (Provider)
		{
			Provider(Items);
		}
	}
	virtual void ClearItems() override { Items.Reset(); }

	FProvider Provider;
	TArray<TItem> Items;
};
