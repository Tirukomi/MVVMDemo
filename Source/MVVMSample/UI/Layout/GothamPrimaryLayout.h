// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "CommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Layout/GothamUITypes.h"
#include "GothamPrimaryLayout.generated.h"

class UOverlay;

/**
 * A layer stack with the shared screen transition: a short fade (GothamMotion::ScreenSeconds) on both push and pop, so
 * every screen gets an intro and an outro from Common UI itself. Screens add their own content slide on activation.
 */
UCLASS()
class MVVMSAMPLE_API UGothamScreenStack : public UCommonActivatableWidgetStack
{
	GENERATED_BODY()

public:
	UGothamScreenStack(const FObjectInitializer& ObjectInitializer);
};

/**
 * Root of all UI for a local player: one activatable-widget stack per EGothamUILayer, stacked in z-order, inside a DPI
 * scaler that applies the player's UI scale setting to HUD and menus together.
 */
UCLASS()
class MVVMSAMPLE_API UGothamPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayer(EGothamUILayer Layer) const;

	/**
	 * A layer that is not interactive stays on screen but leaves hit-testing: neither the mouse nor navigation (arrows,
	 * d-pad) reaches its widgets. The stacks set their own visibility, so this goes on a box around each one.
	 */
	void SetLayerInteractive(EGothamUILayer Layer, bool bInteractive);
	bool IsLayerInteractive(EGothamUILayer Layer) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Reduced motion turns the transition off (duration 0); UI scale sets the DPI scale. */
	void ApplySettings(const FGothamSettingsData& Data);

	/** The player's UI scale, read by the DPI scaler every layout pass. */
	float UIScale = 1.f;
	FGothamSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonActivatableWidgetStack>> Layers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOverlay>> LayerBoxes;
};
