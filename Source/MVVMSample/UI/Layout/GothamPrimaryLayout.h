// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "CommonUserWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/Layout/GothamUITypes.h"
#include "GothamPrimaryLayout.generated.h"

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

/** Root of all UI for a local player: one activatable-widget stack per EGothamUILayer, stacked in z-order. */
UCLASS()
class MVVMSAMPLE_API UGothamPrimaryLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UCommonActivatableWidgetStack* GetLayer(EGothamUILayer Layer) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Reduced motion turns the transition off (duration 0). */
	void ApplyMotionSetting();
	FGothamSettingsListener SettingsListener;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UCommonActivatableWidgetStack>> Layers;
};
