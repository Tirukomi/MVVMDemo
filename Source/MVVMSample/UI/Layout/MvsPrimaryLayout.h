// Copyright IG. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/MvsSettingsListener.h"
#include "CommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Layout/MvsUITypes.h"
#include "MvsPrimaryLayout.generated.h"

class UOverlay;

/**
 * A layer stack with the shared screen transition: a short fade (MvsMotion::ScreenSeconds) on both push and pop, so
 * every screen gets an intro and an outro from Common UI itself. Screens add their own content slide on activation.
 */
UCLASS()
class MVVMSAMPLE_API UMvsScreenStack : public UCommonActivatableWidgetStack
{
	GENERATED_BODY()

public:
	UMvsScreenStack(const FObjectInitializer& ObjectInitializer);
};

/**
 * Root of all UI for a local player: one activatable-widget stack per EMvsUILayer, stacked in z-order, inside a DPI
 * scaler that applies the player's UI scale setting to HUD and menus together.
 */
UCLASS()
class MVVMSAMPLE_API UMvsPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayer(EMvsUILayer Layer) const;

	/**
	 * A layer that is not interactive stays on screen but leaves hit-testing: neither the mouse nor navigation (arrows,
	 * d-pad) reaches its widgets. The stacks set their own visibility, so this goes on a box around each one.
	 */
	void SetLayerInteractive(EMvsUILayer Layer, bool bInteractive);
	bool IsLayerInteractive(EMvsUILayer Layer) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Reduced motion turns the transition off (duration 0); UI scale sets the DPI scale. */
	void ApplySettings(const FMvsSettingsData& Data);

	/** The player's UI scale, read by the DPI scaler every layout pass. */
	float UIScale = 1.f;
	FMvsSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonActivatableWidgetStack>> Layers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UOverlay>> LayerBoxes;
};
