// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Accessibility/GothamSettingsListener.h"
#include "Containers/Ticker.h"
#include "CommonActivatableWidget.h"
#include "GothamHudWidget.generated.h"

class UClueMarkerLayer;
class UComboWidget;
class UDamageVignette;
class UDetectiveOverlayWidget;
class UGadgetSelectorWidget;
class UHealthBarWidget;
class UObjectiveTrackerWidget;
class UPlayerVitalsViewModel;
class USubtitleWidget;
class UThreatIndicatorLayer;

/**
 * Combat HUD root. Layout: health and combo top-left, gadget selector top-right with the objective under it,
 * subtitles bottom-centre; Detective overlay, clue markers, threat indicators and the danger vignette full-screen behind
 * everything.
 *
 * A plain activatable widget on the Game layer: it shares nothing with the menu screens (no back handling, no focus,
 * no menu frame) beyond keeping the game in control while it is the top widget.
 */
UCLASS()
class MVVMSAMPLE_API UGothamHudWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UGothamHudWidget(const FObjectInitializer& ObjectInitializer);

	/** The HUD keeps the game in control: mouse captured, no cursor. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void OnVitalsChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId);
	/** Flash to full, then settle at the low-health level (or zero) over half a second. */
	void FlashVignette();
	void UpdateVignetteRest();
	void ApplyMarkerColors();

	UPROPERTY(Transient)
	TObjectPtr<UDamageVignette> Vignette;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveOverlayWidget> DetectiveOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UClueMarkerLayer> ClueMarkers;

	UPROPERTY(Transient)
	TObjectPtr<UThreatIndicatorLayer> ThreatIndicators;

	UPROPERTY(Transient)
	TObjectPtr<UHealthBarWidget> HealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UComboWidget> ComboCounter;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetSelectorWidget> GadgetSelector;

	UPROPERTY(Transient)
	TObjectPtr<UObjectiveTrackerWidget> ObjectiveTracker;

	UPROPERTY(Transient)
	TObjectPtr<USubtitleWidget> Subtitles;

	UPROPERTY(Transient)
	TObjectPtr<UPlayerVitalsViewModel> VitalsVM;

	FTSTicker::FDelegateHandle FlashHandle;
	FGothamSettingsListener SettingsListener;
	float FlashElapsed = 0.f;
	int32 LastDamageCount = 0;
};
