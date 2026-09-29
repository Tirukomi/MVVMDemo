// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Screens/GothamScreen.h"
#include "GothamHudWidget.generated.h"

class UComboWidget;
class UDetectiveOverlayWidget;
class UObjectiveTrackerWidget;
class UGadgetBarViewModel;
class UGadgetSlotWidget;
class UHealthBarWidget;
class UHorizontalBox;

/** Combat HUD root. Lays out the three HUD widgets and hands each its view model. */
UCLASS()
class MVVMSAMPLE_API UGothamHudWidget : public UGothamScreen
{
	GENERATED_BODY()

public:
	UGothamHudWidget(const FObjectInitializer& ObjectInitializer);

	/** Set from tests or content to override which widgets are spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TSubclassOf<UGadgetSlotWidget> GadgetSlotClass;

	/** The HUD keeps the game in control: mouse captured, no cursor. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void RebuildGadgetSlots();
	void OnGadgetBarChanged(UObject* Source, UE::FieldNotification::FFieldId FieldId) { RebuildGadgetSlots(); }

	UPROPERTY(Transient)
	TObjectPtr<UHealthBarWidget> HealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UDetectiveOverlayWidget> DetectiveOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UObjectiveTrackerWidget> ObjectiveTracker;

	UPROPERTY(Transient)
	TObjectPtr<UComboWidget> ComboCounter;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> GadgetBox;

	UPROPERTY(Transient)
	TObjectPtr<UGadgetBarViewModel> GadgetBarVM;
};
